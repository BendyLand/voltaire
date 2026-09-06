/**************************************************************************/
/*  node.cpp                                                              */
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

#include "core/templates/mem_unique_ptr.h"
#include "node.compat.inc"
#include "node.h"
#include "scene/resources/environment.h"
#include "core/templates/vector.h"

STATIC_ASSERT_INCOMPLETE_TYPE(class, Mesh);
STATIC_ASSERT_INCOMPLETE_TYPE(class, RenderingServer);
STATIC_ASSERT_INCOMPLETE_TYPE(class, DisplayServer);
STATIC_ASSERT_INCOMPLETE_TYPE(class, Shader);
STATIC_ASSERT_INCOMPLETE_TYPE(class, OS);
STATIC_ASSERT_INCOMPLETE_TYPE(class, Engine);

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/resource.h"
#include "core/io/resource_loader.h"
#include "core/string/print_string.h"
#include "scene/animation/tween.h"
#include "scene/main/instance_placeholder.h"
#include "scene/main/multiplayer_api.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"
#include "scene/main/window.h"
#include "scene/resources/packed_scene.h"
#include "servers/display/accessibility_server.h"

#ifdef DEBUG_ENABLED
#endif

#ifdef DEBUG_ENABLED
SafeNumeric<uint64_t> Node::total_node_count{0};
#endif

thread_local Node* Node::current_process_thread_group = nullptr;

void Node::_notification(int p_notification)
{
	switch (p_notification) {
	case NOTIFICATION_ACCESSIBILITY_INVALIDATE: {
		if (data.accessibility_element.is_valid()) {
			AccessibilityServer::get_singleton()->free_element(data.accessibility_element);
			data.accessibility_element = RID();
		}
	} break;

	case NOTIFICATION_ACCESSIBILITY_UPDATE: {
		RID ae = get_accessibility_element();
		ERR_FAIL_COND(ae.is_null());

		AccessibilityServer::get_singleton()->update_set_name(ae, get_name());

		// Node children.
		if (!accessibility_override_tree_hierarchy()) {
			for (int i = 0; i < get_child_count(); i++) {
				Node* child_node = get_child(i);
				Window* child_wnd = Object::cast_to<Window>(child_node);
				if (child_wnd && !(child_wnd->is_visible() &&
									 (child_wnd->is_embedded() || child_wnd->is_popup()))) {
					continue;
				}
				if (child_node->is_part_of_edited_scene()) {
					continue;
				}
				AccessibilityServer::get_singleton()->update_add_child(
					ae, child_node->get_accessibility_element());
			}
		}
	} break;

	case NOTIFICATION_ENTER_TREE: {
		ERR_FAIL_NULL(get_viewport());
		ERR_FAIL_NULL(data.tree);

		if (data.tree->is_accessibility_supported() && !is_part_of_edited_scene()) {
			data.tree->_accessibility_force_update();
			data.tree->_accessibility_notify_change(this);
			if (data.parent) {
				data.tree->_accessibility_notify_change(data.parent);
			}
			else {
				data.tree->_accessibility_notify_change(get_window()); // Root node.
			}
		}

		// Update process mode.
		if (data.process_mode == PROCESS_MODE_INHERIT) {
			if (data.parent) {
				data.process_owner = data.parent->data.process_owner;
			}
			else {
				ERR_PRINT("The root node can't be set to Inherit process mode, reverting to "
						  "Pausable instead.");
				data.process_mode = PROCESS_MODE_PAUSABLE;
				data.process_owner = this;
			}
		}
		else {
			data.process_owner = this;
		}

		{ // Update threaded process mode.
			if (data.process_thread_group == PROCESS_THREAD_GROUP_INHERIT) {
				if (data.parent) {
					data.process_thread_group_owner = data.parent->data.process_thread_group_owner;
				}

				if (data.process_thread_group_owner) {
					data.process_group = data.process_thread_group_owner->data.process_group;
				}
				else {
					data.process_group = &data.tree->default_process_group;
				}
			}
			else {
				data.process_thread_group_owner = this;
				_add_process_group();
			}

			if (_is_any_processing()) {
				_add_to_process_thread_group();
			}
		}

		if (data.physics_interpolation_mode == PHYSICS_INTERPOLATION_MODE_INHERIT) {
			bool interpolate = true; // Root node default is for interpolation to be on.
			if (data.parent) {
				interpolate = data.parent->is_physics_interpolated();
			}
			_propagate_physics_interpolated(interpolate);
		}

		// Update auto translate mode.
		if (data.auto_translate_mode == AUTO_TRANSLATE_MODE_INHERIT && !data.parent) {
			ERR_PRINT("The root node can't be set to Inherit auto translate mode, reverting to "
					  "Always instead.");
			data.auto_translate_mode = AUTO_TRANSLATE_MODE_ALWAYS;
		}
		data.is_auto_translate_dirty = true;
		data.is_translation_domain_dirty = true;

		if (data.input) {
			add_to_group("_vp_input" + itos(get_viewport()->obj->get_instance_id()));
		}
		if (data.shortcut_input) {
			add_to_group("_vp_shortcut_input" + itos(get_viewport()->obj->get_instance_id()));
		}
		if (data.unhandled_input) {
			add_to_group("_vp_unhandled_input" + itos(get_viewport()->obj->get_instance_id()));
		}
		if (data.unhandled_key_input) {
			add_to_group("_vp_unhandled_key_input" + itos(get_viewport()->obj->get_instance_id()));
		}

		data.tree->nodes_in_tree_count++;

	} break;

	case NOTIFICATION_POST_ENTER_TREE: {
		this->obj->notification(NOTIFICATION_TRANSLATION_CHANGED);
	} break;

	case NOTIFICATION_EXIT_TREE: {
		ERR_FAIL_NULL(get_viewport());
		ERR_FAIL_NULL(data.tree);

		if (data.tree->is_accessibility_supported() && !is_part_of_edited_scene()) {
			if (data.accessibility_element.is_valid()) {
				AccessibilityServer::get_singleton()->free_element(data.accessibility_element);
				data.accessibility_element = RID();
			}
			data.tree->_accessibility_notify_change(this, true);
			if (data.parent) {
				data.tree->_accessibility_notify_change(data.parent);
			}
			else {
				data.tree->_accessibility_notify_change(get_window()); // Root node.
			}
		}

		data.tree->nodes_in_tree_count--;

		if (data.input) {
			remove_from_group("_vp_input" + itos(get_viewport()->obj->get_instance_id()));
		}
		if (data.shortcut_input) {
			remove_from_group("_vp_shortcut_input" + itos(get_viewport()->obj->get_instance_id()));
		}
		if (data.unhandled_input) {
			remove_from_group("_vp_unhandled_input" + itos(get_viewport()->obj->get_instance_id()));
		}
		if (data.unhandled_key_input) {
			remove_from_group(
				"_vp_unhandled_key_input" + itos(get_viewport()->obj->get_instance_id()));
		}

		// Remove from processing first.
		if (_is_any_processing()) {
			_remove_from_process_thread_group();
		}
		// Remove the process group.
		if (data.process_thread_group_owner == this) {
			_remove_process_group();
		}
		data.process_thread_group_owner = nullptr;
		data.process_owner = nullptr;

		if (data.path_cache) {
			memdelete(data.path_cache);
			data.path_cache = nullptr;
		}
	} break;

	case NOTIFICATION_SUSPENDED:
	case NOTIFICATION_PAUSED: {
		if (is_physics_interpolated_and_enabled() && is_inside_tree()) {
			reset_physics_interpolation();
		}
	} break;

	case NOTIFICATION_PATH_RENAMED: {
		if (data.path_cache) {
			memdelete(data.path_cache);
			data.path_cache = nullptr;
		}
	} break;

	case Object::NOTIFICATION_PREDELETE: {
		if (data.tree && !Thread::is_main_thread()) {
			this->obj->cancel_free();
			ERR_PRINT(
				"Attempted to free a node that is currently added to the SceneTree from a thread. "
				"This is not permitted, use queue_free() instead. Node has not been freed.");
			return;
		}
#ifdef TOOLS_ENABLED
		if (Engine::get_singleton()->is_editor_hint() && data.tree &&
			this == data.tree->get_edited_scene_root()) {
			this->obj->cancel_free();
			ERR_PRINT(vformat("Something attempted to free the root Node of a scene (\"%s\"). This "
							  "is not supported inside the editor, so the Node was not freed.",
				get_name()));
			return;
		}
#endif
		if (data.owner) {
			_clean_up_owner();
		}

		while (!data.owned.is_empty()) {
			Node* n = data.owned.back()->get();
			n->_clean_up_owner(); // This will change data.owned. So it's impossible to loop over
								  // the list in the usual manner.
		}

		if (data.parent) {
			data.parent->remove_child(this);
		}

		// kill children as cleanly as possible
		while (data.children.size()) {
			Node* child = data.children.last()->value; // begin from the end because its faster and
													   // more consistent with creation
			memdelete(child);
		}
	} break;

	case NOTIFICATION_TRANSLATION_CHANGED: {
		if (data.tree) {
			data.is_auto_translate_dirty = true;
		}
	} break;
	}
}

void Node::_propagate_ready()
{
	data.ready_notified = true;
	data.blocked++;
	for (KeyValue<StringName, Node*>& K : data.children) {
		K.value->_propagate_ready();
	}

	data.blocked--;

	this->obj->notification(NOTIFICATION_POST_ENTER_TREE);

	if (data.ready_first) {
		data.ready_first = false;
		this->obj->notification(NOTIFICATION_READY);
		this->obj->emit_signal(SceneStringName(ready));
	}
}

void Node::_propagate_enter_tree()
{
	// this needs to happen to all children before any enter_tree

	if (data.parent) {
		data.tree = data.parent->data.tree;
		data.depth = data.parent->data.depth + 1;
	}
	else {
		data.depth = 1;
	}

	data.viewport = Object::cast_to<Viewport>(this);
	if (!data.viewport && data.parent) {
		data.viewport = data.parent->data.viewport;
	}

	for (KeyValue<StringName, GroupData>& E : data.grouped) {
		E.value.group = data.tree->add_to_group(E.key, this);
	}

	this->obj->notification(NOTIFICATION_ENTER_TREE);

	this->obj->emit_signal(SceneStringName(tree_entered));

	data.tree->node_added(this);

	data.blocked++;
	// block while adding children

	for (KeyValue<StringName, Node*>& K : data.children) {
		if (!K.value->is_inside_tree()) { // could have been added in enter_tree
			K.value->_propagate_enter_tree();
		}
	}

	data.blocked--;

#ifdef DEBUG_ENABLED
	SceneDebugger::add_to_cache(data.scene_file_path, this);
#endif
	// enter groups
}

void Node::_propagate_after_exit_tree()
{
	// Clear owner if it was not part of the pruned branch
	if (data.owner) {
		if (!data.owner->is_ancestor_of(this)) {
			_clean_up_owner();
		}
	}

	data.blocked++;

	for (HashMap<StringName, Node*>::Iterator I = data.children.last(); I; --I) {
		I->value->_propagate_after_exit_tree();
	}

	data.blocked--;

	this->obj->emit_signal(SceneStringName(tree_exited));
}

void Node::_propagate_exit_tree()
{
	// block while removing children

#ifdef DEBUG_ENABLED
	if (!data.scene_file_path.is_empty()) {
		// Only remove if file path is set (optimization).
		SceneDebugger::remove_from_cache(data.scene_file_path, this);
	}
#endif
	data.blocked++;

	for (HashMap<StringName, Node*>::Iterator I = data.children.last(); I; --I) {
		I->value->_propagate_exit_tree();
	}

	data.blocked--;

	this->obj->emit_signal(SceneStringName(tree_exiting));

	this->obj->notification(NOTIFICATION_EXIT_TREE, true);
	if (data.tree) {
		data.tree->node_removed(this);
	}

	// exit groups
	for (KeyValue<StringName, GroupData>& E : data.grouped) {
		data.tree->remove_from_group(E.key, this);
		E.value.group = nullptr;
	}

	data.viewport = nullptr;

	if (data.tree) {
		data.tree->tree_changed();
	}

	data.ready_notified = false;
	data.tree = nullptr;
	data.depth = -1;
}

void Node::_propagate_physics_interpolated(bool p_interpolated)
{
	switch (data.physics_interpolation_mode) {
	case PHYSICS_INTERPOLATION_MODE_INHERIT:
		// Keep the parent p_interpolated.
		break;
	case PHYSICS_INTERPOLATION_MODE_OFF: {
		p_interpolated = false;
	} break;
	case PHYSICS_INTERPOLATION_MODE_ON: {
		p_interpolated = true;
	} break;
	}

	// No change? No need to propagate further.
	if (data.physics_interpolated == p_interpolated) {
		return;
	}

	data.physics_interpolated = p_interpolated;

	// Allow a call to the RenderingServer etc. in derived classes.
	_physics_interpolated_changed();

	update_configuration_warnings();

	data.blocked++;
	for (KeyValue<StringName, Node*>& K : data.children) {
		K.value->_propagate_physics_interpolated(p_interpolated);
	}
	data.blocked--;
}

void Node::_propagate_physics_interpolation_reset_requested(bool p_requested)
{
	if (is_physics_interpolated()) {
		data.physics_interpolation_reset_requested = p_requested;
	}

	data.blocked++;
	for (KeyValue<StringName, Node*>& K : data.children) {
		K.value->_propagate_physics_interpolation_reset_requested(p_requested);
	}
	data.blocked--;
}

void Node::move_child(Node* rp_child, int p_index)
{
	ERR_FAIL_COND_MSG(data.tree && !Thread::is_main_thread(),
		"Moving child node positions inside the SceneTree is only allowed from the main thread. "
		"Use call_deferred(\"move_child\",child,index).");

	_update_children_cache();
	// We need to check whether node is internal and move it only in the relevant node range.
	if (rp_child->data.internal_mode == INTERNAL_MODE_FRONT) {
		if (p_index < 0) {
			p_index += data.internal_children_front_count_cache;
		}
		ERR_FAIL_INDEX_MSG(p_index, data.internal_children_front_count_cache,
			vformat("Invalid new child index: %d. Child is internal.", p_index));
		_move_child(rp_child, p_index);
	}
	else if (rp_child->data.internal_mode == INTERNAL_MODE_BACK) {
		if (p_index < 0) {
			p_index += data.internal_children_back_count_cache;
		}
		ERR_FAIL_INDEX_MSG(p_index, data.internal_children_back_count_cache,
			vformat("Invalid new child index: %d. Child is internal.", p_index));
		_move_child(rp_child,
			(int)data.children_cache.size() - data.internal_children_back_count_cache + p_index);
	}
	else {
		if (p_index < 0) {
			p_index += get_child_count(false);
		}
		ERR_FAIL_INDEX_MSG(p_index,
			(int)data.children_cache.size() + 1 - data.internal_children_front_count_cache -
				data.internal_children_back_count_cache,
			vformat("Invalid new child index: %d.", p_index));
		_move_child(rp_child, p_index + data.internal_children_front_count_cache);
	}
}

void Node::_move_child(Node* p_child, int p_index, bool p_ignore_end)
{
	ERR_FAIL_COND_MSG(
		data.blocked > 0, "Parent node is busy setting up children, `move_child()` failed. "
						  "Consider using `move_child.call_deferred(child, index)` instead (or "
						  "`popup.call_deferred()` if this is from a popup).");

	// Specifying one place beyond the end
	// means the same as moving to the last index
	if (!p_ignore_end) { // p_ignore_end is a little hack to make back internal children work
						 // properly.
		if (p_child->data.internal_mode == INTERNAL_MODE_FRONT) {
			if (p_index == data.internal_children_front_count_cache) {
				p_index--;
			}
		}
		else if (p_child->data.internal_mode == INTERNAL_MODE_BACK) {
			if (p_index == (int)data.children_cache.size()) {
				p_index--;
			}
		}
		else {
			if (p_index ==
				(int)data.children_cache.size() - data.internal_children_back_count_cache) {
				p_index--;
			}
		}
	}

	int child_index = p_child->get_index();

	if (child_index == p_index) {
		return; // do nothing
	}

	int motion_from = MIN(p_index, child_index);
	int motion_to = MAX(p_index, child_index);

	data.children_cache.remove_at(child_index);
	data.children_cache.insert(p_index, p_child);

	if (data.tree) {
		data.tree->tree_changed();
	}

	data.blocked++;
	// new pos first
	for (int i = motion_from; i <= motion_to; i++) {
		if (data.children_cache[i]->data.internal_mode == INTERNAL_MODE_DISABLED) {
			data.children_cache[i]->data.index = i - data.internal_children_front_count_cache;
		}
		else if (data.children_cache[i]->data.internal_mode == INTERNAL_MODE_BACK) {
			data.children_cache[i]->data.index =
				i - data.internal_children_front_count_cache - data.external_children_count_cache;
		}
		else {
			data.children_cache[i]->data.index = i;
		}
	}
	// notification second
	move_child_notify(p_child);
	this->obj->notification(NOTIFICATION_CHILD_ORDER_CHANGED);
	this->obj->emit_signal(SNAME("child_order_changed"));
	p_child->_propagate_groups_dirty();

	data.blocked--;
}

void Node::_propagate_groups_dirty()
{
	for (const KeyValue<StringName, GroupData>& E : data.grouped) {
		if (E.value.group) {
			E.value.group->changed = true;
		}
	}

	for (KeyValue<StringName, Node*>& K : data.children) {
		K.value->_propagate_groups_dirty();
	}
}

void Node::add_child_notify(Node* p_child)
{
	// to be used when not wanted
}

void Node::remove_child_notify(Node* p_child)
{
	// to be used when not wanted
}

void Node::move_child_notify(Node* p_child)
{
	// to be used when not wanted
}

void Node::owner_changed_notify() {}

void Node::_physics_interpolated_changed() {}

void Node::set_physics_process(bool p_process)
{
	ERR_THREAD_GUARD
	if (data.physics_process == p_process) {
		return;
	}

	if (!is_inside_tree()) {
		data.physics_process = p_process;
		return;
	}

	if (_is_any_processing()) {
		_remove_from_process_thread_group();
	}

	data.physics_process = p_process;

	if (_is_any_processing()) {
		_add_to_process_thread_group();
	}
}

bool Node::is_physics_processing() const { return data.physics_process; }

void Node::set_physics_process_internal(bool p_process_internal)
{
	ERR_THREAD_GUARD
	if (data.physics_process_internal == p_process_internal) {
		return;
	}

	if (!is_inside_tree()) {
		data.physics_process_internal = p_process_internal;
		return;
	}

	if (_is_any_processing()) {
		_remove_from_process_thread_group();
	}

	data.physics_process_internal = p_process_internal;

	if (_is_any_processing()) {
		_add_to_process_thread_group();
	}
}

bool Node::is_physics_processing_internal() const { return data.physics_process_internal; }

void Node::set_process_mode(ProcessMode p_mode)
{
	ERR_THREAD_GUARD
	if (data.process_mode == p_mode) {
		return;
	}

	if (!is_inside_tree()) {
		data.process_mode = p_mode;
		return;
	}

	bool prev_can_process = can_process();
	bool prev_enabled = _is_enabled();

	if (p_mode == PROCESS_MODE_INHERIT) {
		if (data.parent) {
			data.process_owner = data.parent->data.process_owner;
		}
		else {
			ERR_FAIL_MSG("The root node can't be set to Inherit process mode.");
		}
	}
	else {
		data.process_owner = this;
	}

	data.process_mode = p_mode;

	bool next_can_process = can_process();
	bool next_enabled = _is_enabled();

	int pause_notification = 0;

	if (prev_can_process && !next_can_process) {
		pause_notification = NOTIFICATION_PAUSED;
	}
	else if (!prev_can_process && next_can_process) {
		pause_notification = NOTIFICATION_UNPAUSED;
	}

	int enabled_notification = 0;

	if (prev_enabled && !next_enabled) {
		enabled_notification = NOTIFICATION_DISABLED;
	}
	else if (!prev_enabled && next_enabled) {
		enabled_notification = NOTIFICATION_ENABLED;
	}

	_propagate_process_owner(data.process_owner, pause_notification, enabled_notification);

#ifdef TOOLS_ENABLED
	// This is required for the editor to update the visibility of disabled nodes
	// It's very expensive during runtime to change, so editor-only
	if (Engine::get_singleton()->is_editor_hint()) {
		data.tree->obj->emit_signal(SNAME("tree_process_mode_changed"));
	}

	_emit_editor_state_changed();
#endif
}

void Node::_propagate_pause_notification(bool p_enable)
{
	bool prev_can_process = _can_process(!p_enable);
	bool next_can_process = _can_process(p_enable);

	if (prev_can_process && !next_can_process) {
		this->obj->notification(NOTIFICATION_PAUSED);
	}
	else if (!prev_can_process && next_can_process) {
		this->obj->notification(NOTIFICATION_UNPAUSED);
	}

	data.blocked++;
	for (KeyValue<StringName, Node*>& K : data.children) {
		K.value->_propagate_pause_notification(p_enable);
	}
	data.blocked--;
}

void Node::_propagate_suspend_notification(bool p_enable)
{
	this->obj->notification(p_enable ? NOTIFICATION_SUSPENDED : NOTIFICATION_UNSUSPENDED);

	data.blocked++;
	for (KeyValue<StringName, Node*>& KV : data.children) {
		KV.value->_propagate_suspend_notification(p_enable);
	}
	data.blocked--;
}

Node::ProcessMode Node::get_process_mode() const { return data.process_mode; }

void Node::_propagate_process_owner(
	Node* p_owner, int p_pause_notification, int p_enabled_notification)
{
	data.process_owner = p_owner;

	if (p_pause_notification != 0) {
		this->obj->notification(p_pause_notification);
	}

	if (p_enabled_notification != 0) {
		this->obj->notification(p_enabled_notification);
	}

	data.blocked++;
	for (KeyValue<StringName, Node*>& K : data.children) {
		Node* c = K.value;
		if (c->data.process_mode == PROCESS_MODE_INHERIT) {
			c->_propagate_process_owner(p_owner, p_pause_notification, p_enabled_notification);
		}
	}
	data.blocked--;
}

void Node::set_multiplayer_authority(int p_peer_id, bool p_recursive)
{
	ERR_THREAD_GUARD
	data.multiplayer_authority = p_peer_id;

	if (p_recursive) {
		for (KeyValue<StringName, Node*>& K : data.children) {
			K.value->set_multiplayer_authority(p_peer_id, true);
		}
	}
}

int Node::get_multiplayer_authority() const { return data.multiplayer_authority; }

bool Node::is_multiplayer_authority() const
{
	ERR_FAIL_COND_V(!is_inside_tree(), false);

	Ref<MultiplayerAPI> api = get_multiplayer();
	return api.is_valid() && (api->get_unique_id() == data.multiplayer_authority);
}

/***** RPC CONFIG ********/

void Node::rpc_config(const StringName& p_method, const Variant& p_config)
{
	ERR_THREAD_GUARD
	if (data.rpc_config.get_type() != Variant::DICTIONARY) {
		data.rpc_config = Dictionary();
	}
	Dictionary node_config = data.rpc_config;
	if (p_config.get_type() == Variant::NIL) {
		node_config.erase(p_method);
	}
	else {
		ERR_FAIL_COND(p_config.get_type() != Variant::DICTIONARY);
		node_config[p_method] = p_config;
	}
}

const Variant Node::get_node_rpc_config() const { return data.rpc_config; }

/***** RPC FUNCTIONS ********/

Error Node::_rpc_bind(const Variant** p_args, int p_argcount, Callable::CallError& r_error)
{
	if (p_argcount < 1) {
		r_error.error = Callable::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
		r_error.expected = 1;
		return ERR_INVALID_PARAMETER;
	}

	if (!p_args[0]->is_string()) {
		r_error.error = Callable::CallError::CALL_ERROR_INVALID_ARGUMENT;
		r_error.argument = 0;
		r_error.expected = Variant::STRING_NAME;
		return ERR_INVALID_PARAMETER;
	}

	StringName method = (*p_args[0]).operator StringName();

	Error err = rpcp(0, method, &p_args[1], p_argcount - 1);
	r_error.error = Callable::CallError::CALL_OK;
	return err;
}

Error Node::_rpc_id_bind(const Variant** p_args, int p_argcount, Callable::CallError& r_error)
{
	if (p_argcount < 2) {
		r_error.error = Callable::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
		r_error.expected = 2;
		return ERR_INVALID_PARAMETER;
	}

	if (p_args[0]->get_type() != Variant::INT) {
		r_error.error = Callable::CallError::CALL_ERROR_INVALID_ARGUMENT;
		r_error.argument = 0;
		r_error.expected = Variant::INT;
		return ERR_INVALID_PARAMETER;
	}

	if (!p_args[1]->is_string()) {
		r_error.error = Callable::CallError::CALL_ERROR_INVALID_ARGUMENT;
		r_error.argument = 1;
		r_error.expected = Variant::STRING_NAME;
		return ERR_INVALID_PARAMETER;
	}

	int peer_id = *p_args[0];
	StringName method = (*p_args[1]).operator StringName();

	Error err = rpcp(peer_id, method, &p_args[2], p_argcount - 2);
	r_error.error = Callable::CallError::CALL_OK;
	return err;
}

Error Node::rpcp(int p_peer_id, const StringName& p_method, const Variant** p_arg, int p_argcount)
{
	ERR_FAIL_COND_V(!is_inside_tree(), ERR_UNCONFIGURED);

	Ref<MultiplayerAPI> api = get_multiplayer();
	if (api.is_null()) {
		return ERR_UNCONFIGURED;
	}
	return api->rpcp(this->obj.get(), p_peer_id, p_method, p_arg, p_argcount);
}

Ref<MultiplayerAPI> Node::get_multiplayer() const
{
	if (!is_inside_tree()) {
		return Ref<MultiplayerAPI>();
	}
	return data.tree->get_multiplayer(get_path());
}

//////////// end of rpc

bool Node::can_process_notification(int p_what) const
{
	switch (p_what) {
	case NOTIFICATION_PHYSICS_PROCESS:
		return data.physics_process;
	case NOTIFICATION_PROCESS:
		return data.process;
	case NOTIFICATION_INTERNAL_PROCESS:
		return data.process_internal;
	case NOTIFICATION_INTERNAL_PHYSICS_PROCESS:
		return data.physics_process_internal;
	}

	return true;
}

bool Node::can_process() const
{
	return is_inside_tree() && !data.tree->is_suspended() && _can_process(data.tree->is_paused());
}

bool Node::_can_process(bool p_paused) const
{
	ProcessMode process_mode;

	if (data.process_mode == PROCESS_MODE_INHERIT) {
		if (!data.process_owner) {
			process_mode = PROCESS_MODE_PAUSABLE;
		}
		else {
			process_mode = data.process_owner->data.process_mode;
		}
	}
	else {
		process_mode = data.process_mode;
	}

	// The owner can't be set to inherit, must be a bug.
	ERR_FAIL_COND_V(process_mode == PROCESS_MODE_INHERIT, false);

	if (process_mode == PROCESS_MODE_DISABLED) {
		return false;
	}
	else if (process_mode == PROCESS_MODE_ALWAYS) {
		return true;
	}

	if (p_paused) {
		return process_mode == PROCESS_MODE_WHEN_PAUSED;
	}
	else {
		return process_mode == PROCESS_MODE_PAUSABLE;
	}
}

void Node::set_physics_interpolation_mode(PhysicsInterpolationMode p_mode)
{
	ERR_THREAD_GUARD
	if (data.physics_interpolation_mode == p_mode) {
		return;
	}

	data.physics_interpolation_mode = p_mode;

	bool interpolate = true; // Default for root node.

	switch (p_mode) {
	case PHYSICS_INTERPOLATION_MODE_INHERIT: {
		if (is_inside_tree() && data.parent) {
			interpolate = data.parent->is_physics_interpolated();
		}
	} break;
	case PHYSICS_INTERPOLATION_MODE_OFF: {
		interpolate = false;
	} break;
	case PHYSICS_INTERPOLATION_MODE_ON: {
		interpolate = true;
	} break;
	}

	_propagate_physics_interpolated(interpolate);

	// Auto-reset on changing interpolation mode.
	if (is_physics_interpolated() && is_inside_tree()) {
		propagate_notification(NOTIFICATION_RESET_PHYSICS_INTERPOLATION);
	}
}

bool Node::is_physics_interpolated_and_enabled() const
{
	return SceneTree::is_fti_enabled() && is_physics_interpolated();
}

void Node::reset_physics_interpolation()
{
	if (SceneTree::is_fti_enabled() && is_inside_tree()) {
		propagate_notification(NOTIFICATION_RESET_PHYSICS_INTERPOLATION);

		// If `reset_physics_interpolation()` is called explicitly by the user
		// (e.g. from scripts) then we prevent deferred auto-resets taking place.
		// The user is trusted to call reset in the right order, and auto-reset
		// will interfere with their control of prev / curr, so should be turned off.
		_propagate_physics_interpolation_reset_requested(false);
	}
}

bool Node::_is_enabled() const
{
	ProcessMode process_mode;

	if (data.process_mode == PROCESS_MODE_INHERIT) {
		if (!data.process_owner) {
			process_mode = PROCESS_MODE_PAUSABLE;
		}
		else {
			process_mode = data.process_owner->data.process_mode;
		}
	}
	else {
		process_mode = data.process_mode;
	}

	return (process_mode != PROCESS_MODE_DISABLED);
}

bool Node::is_enabled() const
{
	ERR_FAIL_COND_V(!is_inside_tree(), false);
	return _is_enabled();
}

double Node::get_physics_process_delta_time() const
{
	if (data.tree) {
		return data.tree->get_physics_process_time();
	}
	else {
		return 0;
	}
}

double Node::get_process_delta_time() const
{
	if (data.tree) {
		return data.tree->get_process_time();
	}
	else {
		return 0;
	}
}

void Node::set_process(bool p_process)
{
	ERR_THREAD_GUARD
	if (data.process == p_process) {
		return;
	}

	if (!is_inside_tree()) {
		data.process = p_process;
		return;
	}

	if (_is_any_processing()) {
		_remove_from_process_thread_group();
	}

	data.process = p_process;

	if (_is_any_processing()) {
		_add_to_process_thread_group();
	}
}

bool Node::is_processing() const { return data.process; }

void Node::set_process_internal(bool p_process_internal)
{
	ERR_THREAD_GUARD
	if (data.process_internal == p_process_internal) {
		return;
	}

	if (!is_inside_tree()) {
		data.process_internal = p_process_internal;
		return;
	}

	if (_is_any_processing()) {
		_remove_from_process_thread_group();
	}

	data.process_internal = p_process_internal;

	if (_is_any_processing()) {
		_add_to_process_thread_group();
	}
}

void Node::_add_process_group() { data.tree->_add_process_group(this); }

void Node::_remove_process_group() { data.tree->_remove_process_group(this); }

void Node::_remove_from_process_thread_group()
{
	data.tree->_remove_node_from_process_group(this, data.process_thread_group_owner);
}

void Node::_add_to_process_thread_group()
{
	data.tree->_add_node_to_process_group(this, data.process_thread_group_owner);
}

void Node::_remove_tree_from_process_thread_group()
{
	if (!is_inside_tree()) {
		return; // May not be initialized yet.
	}

	for (KeyValue<StringName, Node*>& K : data.children) {
		if (K.value->data.process_thread_group != PROCESS_THREAD_GROUP_INHERIT) {
			continue;
		}

		K.value->_remove_tree_from_process_thread_group();
	}

	if (_is_any_processing()) {
		_remove_from_process_thread_group();
	}
}

void Node::_add_tree_to_process_thread_group(Node* p_owner)
{
	data.process_thread_group_owner = p_owner;
	if (p_owner != nullptr) {
		data.process_group = p_owner->data.process_group;
	}
	else {
		data.process_group = &data.tree->default_process_group;
	}

	if (_is_any_processing()) {
		_add_to_process_thread_group();
	}

	for (KeyValue<StringName, Node*>& K : data.children) {
		if (K.value->data.process_thread_group != PROCESS_THREAD_GROUP_INHERIT) {
			continue;
		}

		K.value->_add_tree_to_process_thread_group(p_owner);
	}
}

bool Node::is_processing_internal() const { return data.process_internal; }

void Node::set_process_thread_group_order(int p_order)
{
	ERR_THREAD_GUARD
	if (data.process_thread_group_order == p_order) {
		return;
	}

	data.process_thread_group_order = p_order;

	// Not yet in the tree (or not a group owner, in whose case this is pointless but harmless);
	// trivial update.
	if (!is_inside_tree() || data.process_thread_group_owner != this) {
		return;
	}

	data.tree->process_groups_dirty = true;
}

int Node::get_process_thread_group_order() const { return data.process_thread_group_order; }

void Node::set_process_priority(int p_priority)
{
	ERR_THREAD_GUARD
	if (data.process_priority == p_priority) {
		return;
	}
	if (!is_inside_tree()) {
		// Not yet in the tree; trivial update.
		data.process_priority = p_priority;
		return;
	}

	if (_is_any_processing()) {
		_remove_from_process_thread_group();
	}

	data.process_priority = p_priority;

	if (_is_any_processing()) {
		_add_to_process_thread_group();
	}
}

int Node::get_process_priority() const { return data.process_priority; }

void Node::set_physics_process_priority(int p_priority)
{
	ERR_THREAD_GUARD
	if (data.physics_process_priority == p_priority) {
		return;
	}
	if (!is_inside_tree()) {
		// Not yet in the tree; trivial update.
		data.physics_process_priority = p_priority;
		return;
	}

	if (_is_any_processing()) {
		_remove_from_process_thread_group();
	}

	data.physics_process_priority = p_priority;

	if (_is_any_processing()) {
		_add_to_process_thread_group();
	}
}

int Node::get_physics_process_priority() const { return data.physics_process_priority; }

void Node::set_process_thread_group(ProcessThreadGroup p_mode)
{
	ERR_FAIL_COND_MSG(data.tree && !Thread::is_main_thread(),
		"Changing the process thread group can only be done from the main thread. Use "
		"call_deferred(\"set_process_thread_group\",mode).");
	if (data.process_thread_group == p_mode) {
		return;
	}

	if (!is_inside_tree()) {
		// Not yet in the tree; trivial update.
		data.process_thread_group = p_mode;
		return;
	}

	_remove_tree_from_process_thread_group();
	if (data.process_thread_group != PROCESS_THREAD_GROUP_INHERIT) {
		_remove_process_group();
	}

	data.process_thread_group = p_mode;

	if (p_mode == PROCESS_THREAD_GROUP_INHERIT) {
		if (data.parent) {
			data.process_thread_group_owner = data.parent->data.process_thread_group_owner;
		}
		else {
			data.process_thread_group_owner = nullptr;
		}
	}
	else {
		data.process_thread_group_owner = this;
		_add_process_group();
	}

	_add_tree_to_process_thread_group(data.process_thread_group_owner);

	this->obj->notify_property_list_changed();
}

Node::ProcessThreadGroup Node::get_process_thread_group() const
{
	return data.process_thread_group;
}

void Node::set_process_thread_messages(uint32_t p_flags)
{
	ERR_THREAD_GUARD
	if (data.process_thread_messages == p_flags) {
		return;
	}

	data.process_thread_messages = p_flags;
}

uint32_t Node::get_process_thread_messages() const
{
	return data.process_thread_messages;
}

void Node::set_process_input(bool p_enable)
{
	ERR_THREAD_GUARD
	if (p_enable == data.input) {
		return;
	}

	data.input = p_enable;
	if (!is_inside_tree()) {
		return;
	}

	if (p_enable) {
		add_to_group("_vp_input" + itos(get_viewport()->obj->get_instance_id()));
	}
	else {
		remove_from_group("_vp_input" + itos(get_viewport()->obj->get_instance_id()));
	}
}

bool Node::is_processing_input() const { return data.input; }

void Node::set_process_shortcut_input(bool p_enable)
{
	ERR_THREAD_GUARD
	if (p_enable == data.shortcut_input) {
		return;
	}
	data.shortcut_input = p_enable;
	if (!is_inside_tree()) {
		return;
	}

	if (p_enable) {
		add_to_group("_vp_shortcut_input" + itos(get_viewport()->obj->get_instance_id()));
	}
	else {
		remove_from_group("_vp_shortcut_input" + itos(get_viewport()->obj->get_instance_id()));
	}
}

bool Node::is_processing_shortcut_input() const { return data.shortcut_input; }

void Node::set_process_unhandled_input(bool p_enable)
{
	ERR_THREAD_GUARD
	if (p_enable == data.unhandled_input) {
		return;
	}
	data.unhandled_input = p_enable;
	if (!is_inside_tree()) {
		return;
	}

	if (p_enable) {
		add_to_group("_vp_unhandled_input" + itos(get_viewport()->obj->get_instance_id()));
	}
	else {
		remove_from_group("_vp_unhandled_input" + itos(get_viewport()->obj->get_instance_id()));
	}
}

bool Node::is_processing_unhandled_input() const { return data.unhandled_input; }

void Node::set_process_unhandled_key_input(bool p_enable)
{
	ERR_THREAD_GUARD
	if (p_enable == data.unhandled_key_input) {
		return;
	}
	data.unhandled_key_input = p_enable;
	if (!is_inside_tree()) {
		return;
	}

	if (p_enable) {
		add_to_group("_vp_unhandled_key_input" + itos(get_viewport()->obj->get_instance_id()));
	}
	else {
		remove_from_group("_vp_unhandled_key_input" + itos(get_viewport()->obj->get_instance_id()));
	}
}

bool Node::is_processing_unhandled_key_input() const { return data.unhandled_key_input; }

void Node::set_auto_translate_mode(AutoTranslateMode p_mode)
{
	ERR_THREAD_GUARD
	if (data.auto_translate_mode == p_mode) {
		return;
	}

	if (p_mode == AUTO_TRANSLATE_MODE_INHERIT && data.tree && !data.parent) {
		ERR_FAIL_MSG("The root node can't be set to Inherit auto translate mode.");
	}

	data.auto_translate_mode = p_mode;
	data.is_auto_translating = p_mode != AUTO_TRANSLATE_MODE_DISABLED;
	data.is_auto_translate_dirty = true;

	propagate_notification(NOTIFICATION_TRANSLATION_CHANGED);
}

Node::AutoTranslateMode Node::get_auto_translate_mode() const { return data.auto_translate_mode; }

bool Node::can_auto_translate() const
{
	ERR_READ_THREAD_GUARD_V(false);
	if (!data.is_auto_translate_dirty || data.auto_translate_mode != AUTO_TRANSLATE_MODE_INHERIT) {
		return data.is_auto_translating;
	}

	data.is_auto_translate_dirty = false;

	Node* parent = data.parent;
	while (parent) {
		if (parent->data.auto_translate_mode == AUTO_TRANSLATE_MODE_INHERIT) {
			parent = parent->data.parent;
			continue;
		}

		data.is_auto_translating = parent->data.auto_translate_mode == AUTO_TRANSLATE_MODE_ALWAYS;
		break;
	}

	return data.is_auto_translating;
}

StringName Node::get_translation_domain() const
{
	ERR_READ_THREAD_GUARD_V(StringName());

	if (data.is_translation_domain_inherited && data.is_translation_domain_dirty) {
		const_cast<Node*>(this)->obj->_translation_domain =
			data.parent ? data.parent->get_translation_domain() : StringName();
		data.is_translation_domain_dirty = false;
	}
	return this->obj->_translation_domain;
}

void Node::set_translation_domain(const StringName& p_domain)
{
	ERR_THREAD_GUARD

	if (!data.is_translation_domain_inherited && this->obj->_translation_domain == p_domain) {
		return;
	}

	this->obj->_translation_domain = p_domain;
	data.is_translation_domain_inherited = false;
	data.is_translation_domain_dirty = false;
	_propagate_translation_domain_dirty();
}

void Node::set_translation_domain_inherited()
{
	ERR_THREAD_GUARD

	if (data.is_translation_domain_inherited) {
		return;
	}
	data.is_translation_domain_inherited = true;
	data.is_translation_domain_dirty = true;
	_propagate_translation_domain_dirty();
}

void Node::_propagate_translation_domain_dirty()
{
	for (KeyValue<StringName, Node*>& K : data.children) {
		Node* child = K.value;
		if (child->data.is_translation_domain_inherited) {
			child->data.is_translation_domain_dirty = true;
			child->_propagate_translation_domain_dirty();
		}
	}

	if (is_inside_tree() && data.auto_translate_mode != AUTO_TRANSLATE_MODE_DISABLED) {
		this->obj->notification(NOTIFICATION_TRANSLATION_CHANGED);
	}
}

StringName Node::get_name() const { return data.name; }

void Node::_set_name_nocheck(const StringName& p_name) { data.name = p_name; }

void Node::set_name(const StringName& p_name)
{
	ERR_FAIL_COND_MSG(data.tree && !Thread::is_main_thread(),
		"Changing the name to nodes inside the SceneTree is only allowed from the main thread. Use "
		"`set_name.call_deferred(new_name)`.");
	ERR_FAIL_COND(p_name.is_empty());

	const StringName old_name = data.name;
	if (data.unique_name_in_owner && data.owner) {
		_release_unique_name_in_owner();
	}

	{
		const String input_name_str = String(p_name);
		const String validated_node_name_string = input_name_str.validate_node_name();
		if (input_name_str == validated_node_name_string) {
			data.name = p_name;
		}
		else {
			data.name = StringName(validated_node_name_string);
		}
	}

	if (data.parent) {
		data.parent->_validate_child_name(this, true);
		bool success = data.parent->data.children.replace_key(old_name, data.name);
		ERR_FAIL_COND_MSG(!success, "Renaming child in hashtable failed, this is a bug.");
	}

	if (data.unique_name_in_owner && data.owner) {
		_acquire_unique_name_in_owner();
	}

	propagate_notification(NOTIFICATION_PATH_RENAMED);

	if (is_inside_tree()) {
		this->obj->emit_signal(SNAME("renamed"));
		data.tree->node_renamed(this);
		data.tree->tree_changed();
	}
}

// Returns a clear description of this node depending on what is available. Useful for error
// messages.
String Node::get_description(bool p_show_not_in_tree) const
{
	String description;
	if (is_inside_tree()) {
		description = String(get_path());
	}
	else {
		description = get_name();
		if (description.is_empty()) {
			description = this->obj->get_class();
		}
		if (p_show_not_in_tree) {
			description += " (not inside tree)";
		}
	}
	return description;
}

static SafeRefCount node_hrcr_count;

void Node::init_node_hrcr() { node_hrcr_count.init(1); }

#ifdef TOOLS_ENABLED
String Node::validate_child_name(Node* p_child)
{
	StringName name = p_child->data.name;
	_generate_serial_child_name(p_child, name);
	return name;
}

String Node::prevalidate_child_name(Node* p_child, StringName p_name)
{
	_generate_serial_child_name(p_child, p_name);
	return p_name;
}
#endif

String Node::adjust_name_casing(const String& p_name)
{
	switch (GLOBAL_GET("editor/naming/node_name_casing").operator int()) {
	case NAME_CASING_PASCAL_CASE:
		return p_name.to_pascal_case();
	case NAME_CASING_CAMEL_CASE:
		return p_name.to_camel_case();
	case NAME_CASING_SNAKE_CASE:
		return p_name.to_snake_case();
	case NAME_CASING_KEBAB_CASE:
		return p_name.to_kebab_case();
	}
	return p_name;
}

void Node::_validate_child_name(Node* p_child, bool p_force_human_readable)
{
	/* Make sure the name is unique */

	if (p_force_human_readable) {
		// this approach to autoset node names is human readable but very slow

		StringName name = p_child->data.name;
		_generate_serial_child_name(p_child, name);
		p_child->data.name = name;

	}
	else {
		// this approach to autoset node names is fast but not as readable
		// it's the default and reserves the '@' character for unique names.

		bool unique = true;

		if (p_child->data.name == StringName()) {
			// new unique name must be assigned
			unique = false;
		}
		else {
			const Node* const* existing = data.children.getptr(p_child->data.name);
			unique = !existing || *existing == p_child;
		}

		if (!unique) {
			ERR_FAIL_COND(!node_hrcr_count.ref());
			// Optimized version of the code below:
			// String name = "@" + String(p_child->get_name()) + "@" + itos(node_hrcr_count.get());
			uint32_t c = node_hrcr_count.get();
			String cn = p_child->obj->get_class_name().string();
			const char32_t* cn_ptr = cn.ptr();
			uint32_t cn_length = cn.length();
			uint32_t c_chars = String::num_characters(c);
			uint32_t len = 2 + cn_length + c_chars;
			char32_t* str = (char32_t*)alloca(sizeof(char32_t) * (len + 1));
			uint32_t idx = 0;
			str[idx++] = '@';
			for (uint32_t i = 0; i < cn_length; i++) {
				str[idx++] = cn_ptr[i];
			}
			str[idx++] = '@';
			idx += c_chars;
			ERR_FAIL_COND(idx != len);
			str[idx] = 0;
			while (c) {
				str[--idx] = '0' + (c % 10);
				c /= 10;
			}
			p_child->data.name = String(str);
		}
	}
}

// Return s + 1 as if it were an integer
String increase_numeric_string(const String& s)
{
	String res = s;
	bool carry = res.length() > 0;

	for (int i = res.length() - 1; i >= 0; i--) {
		if (!carry) {
			break;
		}
		char32_t n = s[i];
		if (n == '9') { // keep carry as true: 9 + 1
			res[i] = '0';
		}
		else {
			res[i] = s[i] + 1;
			carry = false;
		}
	}

	if (carry) {
		res = "1" + res;
	}

	return res;
}

void Node::_generate_serial_child_name(const Node* p_child, StringName& name) const
{
	if (name == StringName()) {
		// No name and a new name is needed, create one.

		name = p_child->obj->get_class();
	}

	const Node* const* existing = data.children.getptr(name);
	if (!existing || *existing == p_child) { // Unused, or is current node.
		return;
	}

	// Extract trailing number
	String name_string = name;
	String nums;
	for (int i = name_string.length() - 1; i >= 0; i--) {
		char32_t n = name_string[i];
		if (is_digit(n)) {
			nums = String::chr(name_string[i]) + nums;
		}
		else {
			break;
		}
	}

	String nnsep = _get_name_num_separator();
	int name_last_index = name_string.length() - nnsep.length() - nums.length();

	// Assign the base name + separator to name if we have numbers preceded by a separator
	if (nums.length() > 0 && name_string.substr(name_last_index, nnsep.length()) == nnsep) {
		name_string = name_string.substr(0, name_last_index + nnsep.length());
	}
	else {
		nums = "";
	}

	for (;;) {
		StringName attempt = name_string + nums;

		existing = data.children.getptr(attempt);
		bool exists = existing != nullptr && *existing != p_child;

		if (!exists) {
			name = attempt;
			return;
		}
		else {
			if (nums.length() == 0) {
				// Name was undecorated so skip to 2 for a more natural result
				nums = "2";
				name_string += nnsep; // Add separator because nums.length() > 0 was false
			}
			else {
				nums = increase_numeric_string(nums);
			}
		}
	}
}

Node::InternalMode Node::get_internal_mode() const { return data.internal_mode; }

void Node::_add_child_nocheck(Node* p_child, const StringName& p_name, InternalMode p_internal_mode)
{
	// add a child node quickly, without name validation

	p_child->data.name = p_name;
	data.children.insert(p_name, p_child);

	p_child->data.internal_mode = p_internal_mode;

	bool can_push_back = false;
	switch (p_internal_mode) {
	case INTERNAL_MODE_FRONT: {
		p_child->data.index = data.internal_children_front_count_cache++;
		// Safe to push back when ordinary and back children are empty.
		can_push_back =
			(data.external_children_count_cache + data.internal_children_back_count_cache) == 0;
	} break;
	case INTERNAL_MODE_BACK: {
		p_child->data.index = data.internal_children_back_count_cache++;
		// Safe to push back when cache is valid.
		can_push_back = true;
	} break;
	case INTERNAL_MODE_DISABLED: {
		p_child->data.index = data.external_children_count_cache++;
		// Safe to push back when back children are empty.
		can_push_back = data.internal_children_back_count_cache == 0;
	} break;
	}

	p_child->data.parent = this;

	if (!data.children_cache_dirty && can_push_back) {
		data.children_cache.push_back(p_child);
	}
	else {
		data.children_cache_dirty = true;
	}

	p_child->obj->notification(NOTIFICATION_PARENTED);

	if (data.tree) {
		p_child->_set_tree(data.tree);
	}

	/* Notify */
	add_child_notify(p_child);
	this->obj->notification(NOTIFICATION_CHILD_ORDER_CHANGED);
	this->obj->emit_signal(SNAME("child_order_changed"));
}

void Node::add_child(
	Node* rp_child, bool p_force_readable_name, InternalMode p_internal)
{
	ERR_FAIL_COND_MSG(data.tree && !Thread::is_main_thread(),
		"Adding children to a node inside the SceneTree is only allowed from the main thread. Use "
		"call_deferred(\"add_child\",node).");

	ERR_THREAD_GUARD
	ERR_FAIL_COND_MSG(rp_child->data.parent,
		vformat("Can't add child '%s' to '%s', already has a parent '%s'.", rp_child->get_name(),
			get_name(), rp_child->data.parent->get_name())); // Fail if node has a parent
#ifdef DEBUG_ENABLED
	ERR_FAIL_COND_MSG(rp_child->is_ancestor_of(this),
		vformat("Can't add child '%s' to '%s' as it would result in a cyclic dependency since '%s' "
				"is already a parent of '%s'.",
			rp_child->get_name(), get_name(), rp_child->get_name(), get_name()));
#endif
	ERR_FAIL_COND_MSG(
		data.blocked > 0, "Parent node is busy setting up children, `add_child()` failed. Consider "
						  "using `add_child.call_deferred(child)` instead.");

	_validate_child_name(rp_child, p_force_readable_name);

#ifdef DEBUG_ENABLED
	if (rp_child->data.owner && !rp_child->data.owner->is_ancestor_of(rp_child)) {
		// Owner of rp_child should be ancestor of rp_child.
		WARN_PRINT(vformat("Adding '%s' as child to '%s' will make owner '%s' inconsistent. "
						   "Consider unsetting the owner beforehand.",
			rp_child->get_name(), get_name(), rp_child->data.owner->get_name()));
	}
#endif // DEBUG_ENABLED

	_add_child_nocheck(rp_child, rp_child->data.name, p_internal);
}

void Node::add_sibling(Node* rp_sibling, bool p_force_readable_name)
{
	ERR_FAIL_COND_MSG(data.tree && !Thread::is_main_thread(),
		"Adding a sibling to a node inside the SceneTree is only allowed from the main thread. Use "
		"call_deferred(\"add_sibling\",node).");
	data.parent->add_child(rp_sibling, p_force_readable_name, data.internal_mode);
	data.parent->_update_children_cache();
	data.parent->_move_child(rp_sibling, get_index() + 1);
}

void Node::remove_child(Node* rp_child)
{
	ERR_FAIL_COND_MSG(data.tree && !Thread::is_main_thread(),
		"Removing children from a node inside the SceneTree is only allowed from the main thread. "
		"Use call_deferred(\"remove_child\",node).");
	ERR_FAIL_COND_MSG(data.blocked > 0,
		"Parent node is busy adding/removing children, `remove_child()` can't be called at this "
		"time. Consider using `remove_child.call_deferred(child)` instead.");

	/**
	 *  Do not change the data.internal_children*cache counters here.
	 *  Because if nodes are re-added, the indices can remain
	 *  greater-than-everything indices and children added remain
	 *  properly ordered.
	 *
	 *  All children indices and counters will be updated next time the
	 *  cache is re-generated.
	 */

	data.blocked++;
	rp_child->_set_tree(nullptr);

	remove_child_notify(rp_child);
	rp_child->obj->notification(NOTIFICATION_UNPARENTED);

	data.blocked--;

	if (!data.children_cache_dirty && !data.children_cache.is_empty() &&
		data.children_cache[data.children_cache.size() - 1] == rp_child) {
		// Removing the last child keeps the cache and the counters valid, so pop it
		// instead of dirtying the cache. This keeps interleaving removals with reads
		// (like get_child()) linear, as long as children are removed back to front.
		data.children_cache.resize(data.children_cache.size() - 1);
		switch (rp_child->data.internal_mode) {
		case INTERNAL_MODE_DISABLED: {
			data.external_children_count_cache--;
		} break;
		case INTERNAL_MODE_FRONT: {
			data.internal_children_front_count_cache--;
		} break;
		case INTERNAL_MODE_BACK: {
			data.internal_children_back_count_cache--;
		} break;
		}
	}
	else {
		data.children_cache_dirty = true;
	}
	bool success = data.children.erase(rp_child->data.name);
	ERR_FAIL_COND_MSG(
		!success, "Children name does not match parent name in hashtable, this is a bug.");

	rp_child->data.parent = nullptr;
	rp_child->data.index = -1;

	this->obj->notification(NOTIFICATION_CHILD_ORDER_CHANGED);
	this->obj->emit_signal(SNAME("child_order_changed"));

	if (data.tree) {
		rp_child->_propagate_after_exit_tree();
	}
}

void Node::_update_children_cache_impl() const
{
	// Assign children
	data.children_cache.resize(data.children.size());
	int idx = 0;
	for (const KeyValue<StringName, Node*>& K : data.children) {
		data.children_cache[idx] = K.value;
		idx++;
	}
	// Sort them
	data.children_cache.sort_custom<ComparatorByIndex>();
	// Update indices
	data.external_children_count_cache = 0;
	data.internal_children_back_count_cache = 0;
	data.internal_children_front_count_cache = 0;

	for (uint32_t i = 0; i < data.children_cache.size(); i++) {
		switch (data.children_cache[i]->data.internal_mode) {
		case INTERNAL_MODE_DISABLED: {
			data.children_cache[i]->data.index = data.external_children_count_cache++;
		} break;
		case INTERNAL_MODE_FRONT: {
			data.children_cache[i]->data.index = data.internal_children_front_count_cache++;
		} break;
		case INTERNAL_MODE_BACK: {
			data.children_cache[i]->data.index = data.internal_children_back_count_cache++;
		} break;
		}
	}
	data.children_cache_dirty = false;
}

template <bool p_include_internal> Iterable<Node::ChildrenIterator> Node::iterate_children() const
{
	// The thread guard is omitted for performance reasons.
	// ERR_THREAD_GUARD_V(Iterable<ChildrenIterator>(nullptr, nullptr));

	_update_children_cache();
	const uint32_t size = data.children_cache.size();
	// Might be null, but then size and internal counts are also 0.
	Node** ptr = data.children_cache.ptr();

	if constexpr (p_include_internal) {
		return Iterable(ChildrenIterator(ptr), ChildrenIterator(ptr + size));
	}
	else {
		return Iterable(ChildrenIterator(ptr + data.internal_children_front_count_cache),
			ChildrenIterator(ptr + size - data.internal_children_back_count_cache));
	}
}

template Iterable<Node::ChildrenIterator> Node::iterate_children<true>() const;
template Iterable<Node::ChildrenIterator> Node::iterate_children<false>() const;

int Node::get_child_count(bool p_include_internal) const
{
	ERR_THREAD_GUARD_V(0);
	if (p_include_internal) {
		return data.children.size();
	}

	_update_children_cache();
	return data.children_cache.size() - data.internal_children_front_count_cache -
		   data.internal_children_back_count_cache;
}

Node* Node::get_child(int p_index, bool p_include_internal) const
{
	ERR_THREAD_GUARD_V(nullptr);
	_update_children_cache();

	if (p_include_internal) {
		if (p_index < 0) {
			p_index += data.children_cache.size();
		}
		ERR_FAIL_INDEX_V(p_index, (int)data.children_cache.size(), nullptr);
		return data.children_cache[p_index];
	}
	else {
		if (p_index < 0) {
			p_index += (int)data.children_cache.size() - data.internal_children_front_count_cache -
					   data.internal_children_back_count_cache;
		}
		ERR_FAIL_INDEX_V(p_index,
			(int)data.children_cache.size() - data.internal_children_front_count_cache -
				data.internal_children_back_count_cache,
			nullptr);
		p_index += data.internal_children_front_count_cache;
		return data.children_cache[p_index];
	}
}

Vector<Node*> Node::get_children(bool p_include_internal) const
{
	ERR_THREAD_GUARD_V(Vector<Node*>());
	_update_children_cache();

	Vector<Node*> children;

	if (p_include_internal) {
		children.resize(data.children_cache.size());
		Vector<Node*>::Iterator itr = children.begin();
		for (Node* child : data.children_cache) {
			*itr = child;
			++itr;
		}
	}
	else {
		const int size = data.children_cache.size() - data.internal_children_back_count_cache;
		children.resize(size - data.internal_children_front_count_cache);

		Vector<Node*>::Iterator itr = children.begin();
		for (int i = data.internal_children_front_count_cache; i < size; i++) {
			*itr = data.children_cache[i];
			++itr;
		}
	}

	return children;
}

Node* Node::_get_child_by_name(const StringName& p_name) const
{
	const Node* const* node = data.children.getptr(p_name);
	if (node) {
		return const_cast<Node*>(*node);
	}
	else {
		return nullptr;
	}
}

Node* Node::get_node_or_null(const NodePath& p_path) const
{
	ERR_THREAD_GUARD_V(nullptr);
	if (p_path.is_empty()) {
		return nullptr;
	}

	ERR_FAIL_COND_V_MSG(!data.tree && p_path.is_absolute(), nullptr,
		"Can't use get_node() with absolute paths from outside the active scene tree.");

	Node* current = nullptr;
	Node* root = nullptr;

	if (!p_path.is_absolute()) {
		current = const_cast<Node*>(this); // start from this
	}
	else {
		root = const_cast<Node*>(this);
		while (root->data.parent) {
			root = root->data.parent; // start from root
		}
	}

	for (int i = 0; i < p_path.get_name_count(); i++) {
		StringName name = p_path.get_name(i);
		Node* next = nullptr;

		if (name == SNAME(".")) {
			next = current;

		}
		else if (name == SNAME("..")) {
			if (current == nullptr || !current->data.parent) {
				return nullptr;
			}

			next = current->data.parent;
		}
		else if (current == nullptr) {
			if (name == root->get_name()) {
				next = root;
			}

		}
		else if (name.is_node_unique_name()) {
			Node** unique = current->data.owned_unique_nodes.getptr(name);
			if (!unique && current->data.owner) {
				unique = current->data.owner->data.owned_unique_nodes.getptr(name);
			}
			if (!unique) {
				return nullptr;
			}
			next = *unique;
		}
		else {
			next = nullptr;
			const Node* const* node = current->data.children.getptr(name);
			if (node) {
				next = const_cast<Node*>(*node);
			}
			else {
				return nullptr;
			}
		}
		current = next;
	}

	return current;
}

Node* Node::get_node(const NodePath& p_path) const
{
	Node* node = get_node_or_null(p_path);

	if (unlikely(!node)) {
		const String desc = get_description();
		if (p_path.is_absolute()) {
			ERR_FAIL_V_MSG(
				nullptr, vformat(R"(Node not found: "%s" (absolute path attempted from "%s").)",
							 p_path, desc));
		}
		else {
			ERR_FAIL_V_MSG(
				nullptr, vformat(R"(Node not found: "%s" (relative to "%s").)", p_path, desc));
		}
	}

	return node;
}

bool Node::has_node(const NodePath& p_path) const { return get_node_or_null(p_path) != nullptr; }

// Finds the first child node (in tree order) whose name matches the given pattern.
// Can be recursive or not, and limited to owned nodes.
Node* Node::find_child(const String& p_pattern, bool p_recursive, bool p_owned) const
{
	ERR_THREAD_GUARD_V(nullptr);
	ERR_FAIL_COND_V(p_pattern.is_empty(), nullptr);
	_update_children_cache();
	Node* const* cptr = data.children_cache.ptr();
	int ccount = data.children_cache.size();
	for (int i = 0; i < ccount; i++) {
		if (p_owned && !cptr[i]->data.owner) {
			continue;
		}
		if (cptr[i]->data.name.string().match(p_pattern)) {
			return cptr[i];
		}

		if (!p_recursive) {
			continue;
		}

		Node* ret = cptr[i]->find_child(p_pattern, true, p_owned);
		if (ret) {
			return ret;
		}
	}
	return nullptr;
}

// Finds child nodes based on their name using pattern matching, or class name,
// or both (either pattern or type can be left empty).
// Can be recursive or not, and limited to owned nodes.
Vector<Node*> Node::find_children(
	const String& p_pattern, const String& p_type, bool p_recursive, bool p_owned) const
{
	ERR_THREAD_GUARD_V(Vector<Node*>());
	Vector<Node*> ret;
	ERR_FAIL_COND_V(p_pattern.is_empty() && p_type.is_empty(), ret);
	_update_children_cache();
	Node* const* cptr = data.children_cache.ptr();
	int ccount = data.children_cache.size();
	for (int i = 0; i < ccount; i++) {
		if (p_owned && !cptr[i]->data.owner) {
			continue;
		}

		if (p_pattern.is_empty() || cptr[i]->data.name.string().match(p_pattern)) {
			if (p_type.is_empty() || cptr[i]->obj->is_class(p_type)) {
				ret.append(cptr[i]);
			}
			else if (cptr[i]->obj->get_script_instance()) {
				Ref<Script> scr = cptr[i]->obj->get_script_instance()->get_script();
				while (scr.is_valid()) {
					if ((ScriptServer::is_global_class(p_type) &&
							ScriptServer::get_global_class_path(p_type) == scr->get_path()) ||
						p_type == scr->get_path()) {
						ret.append(cptr[i]);
						break;
					}

					scr = scr->get_base_script();
				}
			}
		}

		if (p_recursive) {
			ret.append_array(cptr[i]->find_children(p_pattern, p_type, true, p_owned));
		}
	}

	return ret;
}

void Node::reparent(Node* rp_parent, bool p_keep_global_transform)
{
	ERR_THREAD_GUARD
	ERR_FAIL_NULL_MSG(data.parent, "Node needs a parent to be reparented.");
	ERR_FAIL_COND_MSG(
		rp_parent == this, vformat("Can't reparent '%s' to itself.", rp_parent->get_name()));

	if (rp_parent == data.parent) {
		return;
	}

	bool preserve_owner =
		data.owner && (data.owner == rp_parent || data.owner->is_ancestor_of(rp_parent));
	Node* owner_temp = data.owner;
	LocalVector<Node*> common_parents;

	// If the new parent is related to the owner, find all children of the reparented node who have
	// the same owner so that we can reassign them.
	if (preserve_owner) {
		LocalVector<Node*> to_visit;

		to_visit.push_back(this);
		common_parents.push_back(this);

		while (to_visit.size() > 0) {
			Node* check = to_visit[to_visit.size() - 1];
			to_visit.resize(to_visit.size() - 1);

			for (int i = 0; i < check->get_child_count(false); i++) {
				Node* child = check->get_child(i, false);
				to_visit.push_back(child);
				if (child->data.owner == owner_temp) {
					common_parents.push_back(child);
				}
			}
		}
	}

	data.parent->remove_child(this);
	rp_parent->add_child(this);
	// Reassign the old owner to those found nodes.
	if (preserve_owner) {
		for (Node* E : common_parents) {
			E->set_owner(owner_temp);
		}
	}
}

Node* Node::get_parent() const { return data.parent; }

Node* Node::find_parent(const String& p_pattern) const
{
	ERR_THREAD_GUARD_V(nullptr);
	Node* p = data.parent;
	while (p) {
		if (p->data.name.string().match(p_pattern)) {
			return p;
		}
		p = p->data.parent;
	}

	return nullptr;
}

void Node::set_unique_scene_id(int32_t p_unique_id) { data.unique_scene_id = p_unique_id; }

int32_t Node::get_unique_scene_id() const { return data.unique_scene_id; }

Window* Node::get_window() const
{
	ERR_THREAD_GUARD_V(nullptr);
	Viewport* vp = get_viewport();
	if (vp) {
		return vp->get_base_window();
	}
	return nullptr;
}

Window* Node::get_non_popup_window() const
{
	Window* w = get_window();
	while (w && w->is_popup()) {
		w = w->get_parent_visible_window();
	}
	return w;
}

Window* Node::get_last_exclusive_window() const
{
	Window* w = get_window();
	while (w && w->get_exclusive_child()) {
		w = w->get_exclusive_child();
	}

	return w;
}

bool Node::is_ancestor_of(const Node* rp_node) const
{
	const Node* n = rp_node;

	if (is_inside_tree() && rp_node->data.tree == data.tree) {
		const int depth = data.depth;
		while (n->data.depth > depth) {
			n = n->data.parent;
		}
		return n == this;
	}

	n = rp_node->data.parent;
	while (n) {
		if (n == this) {
			return true;
		}
		n = n->data.parent;
	}

	return false;
}

bool Node::is_greater_than(const Node* rp_node) const
{
	// parent->get_child(1) > parent->get_child(0) > parent

	ERR_FAIL_COND_V(!data.tree, false);
	ERR_FAIL_COND_V(data.depth < 0, false);

	_update_children_cache();

	bool this_is_deeper = this->data.depth > rp_node->data.depth;

	const Node* deep = this;
	const Node* shallow = rp_node;
	if (!this_is_deeper) {
		deep = rp_node;
		shallow = this;
	}

	while (deep->data.depth > shallow->data.depth) {
		deep = deep->data.parent;
	}

	if (deep == shallow) { // Shallow is ancestor of deep.
		return this_is_deeper;
	}

	while (deep->data.parent != shallow->data.parent) {
		deep = deep->data.parent;
		shallow = shallow->data.parent;
	}

	return (deep->get_index() > shallow->get_index()) == this_is_deeper;
}

void Node::get_owned_by(Node* p_by, List<Node*>* p_owned)
{
	if (data.owner == p_by) {
		p_owned->push_back(this);
	}

	for (KeyValue<StringName, Node*>& K : data.children) {
		K.value->get_owned_by(p_by, p_owned);
	}
}

void Node::_set_owner_nocheck(Node* p_owner)
{
	if (data.owner == p_owner) {
		return;
	}

	ERR_FAIL_COND(data.owner);
	data.owner = p_owner;
	data.owner->data.owned.push_back(this);
	data.OW = data.owner->data.owned.back();

	owner_changed_notify();
}

void Node::_release_unique_name_in_owner()
{
	ERR_FAIL_NULL(data.owner); // Safety check.
	StringName key = StringName(UNIQUE_NODE_PREFIX + data.name.string());
	Node** which = data.owner->data.owned_unique_nodes.getptr(key);
	if (which == nullptr || *which != this) {
		return; // Ignore.
	}
	data.owner->data.owned_unique_nodes.erase(key);
}

void Node::_acquire_unique_name_in_owner()
{
	ERR_FAIL_NULL(data.owner); // Safety check.
	StringName key = StringName(UNIQUE_NODE_PREFIX + data.name.string());
	Node** which = data.owner->data.owned_unique_nodes.getptr(key);
	if (which != nullptr && *which != this) {
		String which_path =
			String(is_inside_tree() ? (*which)->get_path() : data.owner->get_path_to(*which));
		WARN_PRINT(
			vformat("Setting node name '%s' to be unique within scene for '%s', but it's already "
					"claimed by '%s'.\n'%s' is no longer set as having a unique name.",
				get_name(), is_inside_tree() ? get_path() : data.owner->get_path_to(this),
				which_path, which_path));
		data.unique_name_in_owner = false;
		return;
	}
	data.owner->data.owned_unique_nodes[key] = this;
}

void Node::set_unique_name_in_owner(bool p_enabled)
{
	ERR_MAIN_THREAD_GUARD
	if (data.unique_name_in_owner == p_enabled) {
		return;
	}

	if (data.unique_name_in_owner && data.owner != nullptr) {
		_release_unique_name_in_owner();
	}
	data.unique_name_in_owner = p_enabled;

	if (data.unique_name_in_owner && data.owner != nullptr) {
		_acquire_unique_name_in_owner();
	}

	update_configuration_warnings();
	_emit_editor_state_changed();
}

bool Node::is_unique_name_in_owner() const { return data.unique_name_in_owner; }

void Node::set_owner(Node* p_owner)
{
	ERR_MAIN_THREAD_GUARD
	if (data.owner) {
		_clean_up_owner();
	}

	ERR_FAIL_COND(p_owner == this);

	if (!p_owner) {
		return;
	}

	bool owner_valid = p_owner->is_ancestor_of(this);

	ERR_FAIL_COND_MSG(!owner_valid, "Invalid owner. Owner must be an ancestor in the tree.");

	_set_owner_nocheck(p_owner);

	if (data.unique_name_in_owner) {
		_acquire_unique_name_in_owner();
	}

	_emit_editor_state_changed();
}

Node* Node::get_owner() const { return data.owner; }

void Node::_clean_up_owner()
{
	ERR_FAIL_NULL(data.owner); // Safety check.

	if (data.unique_name_in_owner) {
		_release_unique_name_in_owner();
	}
	data.owner->data.owned.erase(data.OW);
	data.owner = nullptr;
	data.OW = nullptr;
}

Node* Node::find_common_parent_with(const Node* p_node) const
{
	if (this == p_node) {
		return const_cast<Node*>(p_node);
	}

	HashSet<const Node*> visited;

	const Node* n = this;

	while (n) {
		visited.insert(n);
		n = n->data.parent;
	}

	const Node* common_parent = p_node;

	while (common_parent) {
		if (visited.has(common_parent)) {
			break;
		}
		common_parent = common_parent->data.parent;
	}

	if (!common_parent) {
		return nullptr;
	}

	return const_cast<Node*>(common_parent);
}

NodePath Node::get_path_to(const Node* rp_node, bool p_use_unique_path) const
{
	if (this == rp_node) {
		return NodePath(".");
	}

	HashSet<const Node*> visited;

	const Node* n = this;

	while (n) {
		visited.insert(n);
		n = n->data.parent;
	}

	const Node* common_parent = rp_node;

	while (common_parent) {
		if (visited.has(common_parent)) {
			break;
		}
		common_parent = common_parent->data.parent;
	}

	ERR_FAIL_NULL_V_MSG(common_parent, NodePath(),
		vformat(
			"No path can be resolved between the nodes %s and %s as they share no common ancestor.",
			get_description(true), rp_node->get_description(true)));

	visited.clear();

	Vector<StringName> path;
	StringName up = String("..");

	if (p_use_unique_path) {
		n = rp_node;

		bool is_detected = false;
		while (n != common_parent) {
			if (n->is_unique_name_in_owner() && n->get_owner() == get_owner()) {
				path.push_back(UNIQUE_NODE_PREFIX + String(n->get_name()));
				is_detected = true;
				break;
			}
			path.push_back(n->get_name());
			n = n->data.parent;
		}

		if (!is_detected) {
			n = this;

			String detected_name;
			int up_count = 0;
			while (n != common_parent) {
				if (n->is_unique_name_in_owner() && n->get_owner() == get_owner()) {
					detected_name = n->get_name();
					up_count = 0;
				}
				up_count++;
				n = n->data.parent;
			}

			for (int i = 0; i < up_count; i++) {
				path.push_back(up);
			}

			if (!detected_name.is_empty()) {
				path.push_back(UNIQUE_NODE_PREFIX + detected_name);
			}
		}
	}
	else {
		n = rp_node;

		while (n != common_parent) {
			path.push_back(n->get_name());
			n = n->data.parent;
		}

		n = this;

		while (n != common_parent) {
			path.push_back(up);
			n = n->data.parent;
		}
	}

	path.reverse();

	return NodePath(path, false);
}

NodePath Node::get_path() const
{
	ERR_FAIL_COND_V_MSG(
		!is_inside_tree(), NodePath(), "Cannot get path of node as it is not in a scene tree.");

	if (data.path_cache) {
		return *data.path_cache;
	}

	const Node* n = this;

	Vector<StringName> path;
	path.resize(data.depth);

	StringName* ptrw = path.ptrw();
	while (n) {
		ptrw[n->data.depth - 1] = n->get_name();
		n = n->data.parent;
	}

	data.path_cache = memnew(NodePath(path, true));

	return *data.path_cache;
}

bool Node::is_in_group(const StringName& p_identifier) const
{
	ERR_THREAD_GUARD_V(false);
	return data.grouped.has(p_identifier);
}

void Node::add_to_group(const StringName& p_identifier, bool p_persistent)
{
	ERR_THREAD_GUARD
	ERR_FAIL_COND_MSG(p_identifier.is_empty(),
		vformat("Cannot add node '%s' to a group with an empty name.", get_name()));

	if (data.grouped.has(p_identifier)) {
		return;
	}

	GroupData gd;

	if (data.tree) {
		gd.group = data.tree->add_to_group(p_identifier, this);
	}
	else {
		gd.group = nullptr;
	}

	gd.persistent = p_persistent;

	data.grouped[p_identifier] = gd;
	if (p_persistent) {
		_emit_editor_state_changed();
	}
}

void Node::remove_from_group(const StringName& p_identifier)
{
	ERR_THREAD_GUARD
	HashMap<StringName, GroupData>::Iterator E = data.grouped.find(p_identifier);

	if (!E) {
		return;
	}

#ifdef TOOLS_ENABLED
	bool persistent = E->value.persistent;
#endif

	if (data.tree) {
		data.tree->remove_from_group(E->key, this);
	}

	data.grouped.remove(E);

#ifdef TOOLS_ENABLED
	if (persistent) {
		_emit_editor_state_changed();
	}
#endif
}

TypedArray<StringName> Node::_get_groups() const
{
	TypedArray<StringName> groups;
	List<GroupInfo> gi;
	get_groups(&gi);
	for (const GroupInfo& E : gi) {
		groups.push_back(E.name);
	}

	return groups;
}

void Node::get_groups(List<GroupInfo>* p_groups) const
{
	ERR_THREAD_GUARD
	for (const KeyValue<StringName, GroupData>& E : data.grouped) {
		GroupInfo gi;
		gi.name = E.key;
		gi.persistent = E.value.persistent;
		p_groups->push_back(gi);
	}
}

int Node::get_persistent_group_count() const
{
	ERR_THREAD_GUARD_V(0);
	int count = 0;

	for (const KeyValue<StringName, GroupData>& E : data.grouped) {
		if (E.value.persistent) {
			count += 1;
		}
	}

	return count;
}

void Node::print_tree_pretty() { print_line(_get_tree_string_pretty("", true)); }

void Node::print_tree() { print_line(_get_tree_string(this)); }

String Node::_get_tree_string_pretty(const String& p_prefix, bool p_last)
{
	String new_prefix = p_last ? String::utf8(" ┖╴") : String::utf8(" ┠╴");
	_update_children_cache();
	String return_tree = p_prefix + new_prefix + String(get_name()) + "\n";
	for (uint32_t i = 0; i < data.children_cache.size(); i++) {
		new_prefix = p_last ? String::utf8("   ") : String::utf8(" ┃ ");
		return_tree += data.children_cache[i]->_get_tree_string_pretty(
			p_prefix + new_prefix, i == data.children_cache.size() - 1);
	}
	return return_tree;
}

String Node::get_tree_string_pretty() { return _get_tree_string_pretty("", true); }

String Node::_get_tree_string(const Node* p_node)
{
	_update_children_cache();
	String return_tree = String(p_node->get_path_to(this)) + "\n";
	for (uint32_t i = 0; i < data.children_cache.size(); i++) {
		return_tree += data.children_cache[i]->_get_tree_string(p_node);
	}
	return return_tree;
}

String Node::get_tree_string() { return _get_tree_string(this); }

void Node::propagate_notification(int p_notification)
{
	ERR_THREAD_GUARD
	data.blocked++;
	this->obj->notification(p_notification);

	for (KeyValue<StringName, Node*>& K : data.children) {
		K.value->propagate_notification(p_notification);
	}
	data.blocked--;
}

void Node::propagate_call(
	const StringName& p_method, const Array& p_args, const bool p_parent_first)
{
	ERR_THREAD_GUARD
	data.blocked++;

	if (p_parent_first && this->obj->has_method(p_method)) {
		this->obj->callv(p_method, p_args);
	}

	for (KeyValue<StringName, Node*>& K : data.children) {
		K.value->propagate_call(p_method, p_args, p_parent_first);
	}

	if (!p_parent_first && this->obj->has_method(p_method)) {
		this->obj->callv(p_method, p_args);
	}

	data.blocked--;
}

void Node::_propagate_replace_owner(Node* p_owner, Node* p_by_owner)
{
	if (get_owner() == p_owner) {
		set_owner(p_by_owner);
	}

	data.blocked++;
	for (KeyValue<StringName, Node*>& K : data.children) {
		K.value->_propagate_replace_owner(p_owner, p_by_owner);
	}
	data.blocked--;
}

Tween* Node::create_tween()
{
	ERR_THREAD_GUARD_V(Ref<Tween>().ptr());

	SceneTree* tree = data.tree;
	if (!tree) {
		tree = SceneTree::get_singleton();
	}
	ERR_FAIL_NULL_V_MSG(tree, Ref<Tween>().ptr(), "No available SceneTree to create the Tween.");

	Ref<Tween> tween = tree->create_tween();
	tween->bind_node(this);
	return tween.ptr();
}

void Node::set_scene_file_path(const String& p_scene_file_path)
{
	ERR_THREAD_GUARD
	data.scene_file_path = p_scene_file_path;
	_emit_editor_state_changed();
}

String Node::get_scene_file_path() const { return data.scene_file_path; }

void Node::set_editor_description(const String& p_editor_description)
{
	ERR_THREAD_GUARD
	if (data.editor_description == p_editor_description) {
		return;
	}

	data.editor_description = p_editor_description;
	this->obj->emit_signal(SNAME("editor_description_changed"), this);
}

String Node::get_editor_description() const { return data.editor_description; }

void Node::set_editable_instance(Node* rp_node, bool p_editable)
{
	ERR_THREAD_GUARD
	ERR_FAIL_COND(!is_ancestor_of(rp_node));
	if (!p_editable) {
		rp_node->data.editable_instance = false;
		// Avoid this flag being needlessly saved;
		// also give more visual feedback if editable children are re-enabled
		set_display_folded(false);
	}
	else {
		rp_node->data.editable_instance = true;
	}

	rp_node->_emit_editor_state_changed();
}

bool Node::is_editable_instance(const Node* p_node) const
{
	if (!p_node) {
		return false; // Easier, null is never editable. :)
	}
	ERR_FAIL_COND_V(!is_ancestor_of(p_node), false);
	return p_node->data.editable_instance;
}

Node* Node::get_deepest_editable_node(Node* p_start_node) const
{
	ERR_THREAD_GUARD_V(nullptr);
	ERR_FAIL_NULL_V(p_start_node, nullptr);
	ERR_FAIL_COND_V(!is_ancestor_of(p_start_node), p_start_node);

	const Node* iterated_item = p_start_node;
	Node* node = p_start_node;

	while (iterated_item->get_owner() && iterated_item->get_owner() != this) {
		if (!is_editable_instance(iterated_item->get_owner())) {
			node = iterated_item->get_owner();
		}

		iterated_item = iterated_item->get_owner();
	}

	return node;
}

#ifdef TOOLS_ENABLED
void Node::set_property_pinned(const String& p_property, bool p_pinned)
{
	ERR_THREAD_GUARD
	bool current_pinned = false;
	Array pinned = get_meta("_edit_pinned_properties_", Array());
	StringName psa = get_property_store_alias(p_property);
	current_pinned = pinned.has(psa);

	if (current_pinned != p_pinned) {
		if (p_pinned) {
			pinned.append(psa);
		}
		else {
			pinned.erase(psa);
		}
	}

	if (pinned.is_empty()) {
		remove_meta("_edit_pinned_properties_");
	}
	else {
		set_meta("_edit_pinned_properties_", pinned);
	}
}

bool Node::is_property_pinned(const StringName& p_property) const
{
	Array pinned = get_meta("_edit_pinned_properties_", Array());
	StringName psa = get_property_store_alias(p_property);
	return pinned.has(psa);
}

StringName Node::get_property_store_alias(const StringName& p_property) const { return p_property; }

bool Node::is_part_of_edited_scene() const
{
	return Engine::get_singleton()->is_editor_hint() && is_inside_tree() &&
		   data.tree->get_edited_scene_root() &&
		   data.tree->get_edited_scene_root()
			   ->get_parent() && // Defend against edge cases when creating new scenes and they are
								 // not fully added to the tree yet.
		   data.tree->get_edited_scene_root()->get_parent()->is_ancestor_of(this);
}
#endif

void Node::get_storable_properties(HashSet<StringName>& r_storable_properties) const
{
	ERR_THREAD_GUARD
	List<PropertyInfo> property_list;
	this->obj->get_property_list(&property_list);
	for (const PropertyInfo& pi : property_list) {
		if ((pi.usage & PROPERTY_USAGE_STORAGE)) {
			r_storable_properties.insert(pi.name);
		}
	}
}

void Node::set_scene_instance_state(const Ref<SceneState>& p_state)
{
	ERR_THREAD_GUARD
	data.instance_state = p_state;
}

Ref<SceneState> Node::get_scene_instance_state() const { return data.instance_state; }

void Node::set_scene_inherited_state(const Ref<SceneState>& p_state)
{
	ERR_THREAD_GUARD
	data.inherited_state = p_state;
	_emit_editor_state_changed();
}

Ref<SceneState> Node::get_scene_inherited_state() const { return data.inherited_state; }

void Node::set_scene_instance_load_placeholder(bool p_enable) { data.use_placeholder = p_enable; }

bool Node::get_scene_instance_load_placeholder() const { return data.use_placeholder; }

Node* Node::_duplicate(int p_flags, HashMap<const Node*, Node*>* r_duplimap) const
{
	ERR_THREAD_GUARD_V(nullptr);
	Node* node = nullptr;

	bool instantiated = false;

	if (Object::cast_to<InstancePlaceholder>(this)) {
		const InstancePlaceholder* ip = Object::cast_to<const InstancePlaceholder>(this);
		InstancePlaceholder* nip = memnew(InstancePlaceholder);
		nip->set_instance_path(ip->get_instance_path());
		node = nip;

	}
	else if ((p_flags & DUPLICATE_USE_INSTANTIATION) && is_instance()) {
		Ref<PackedScene> res = ResourceLoader::load(get_scene_file_path());
		ERR_FAIL_COND_V(res.is_null(), nullptr);
		PackedScene::GenEditState edit_state = PackedScene::GEN_EDIT_STATE_DISABLED;
#ifdef TOOLS_ENABLED
		if (p_flags & DUPLICATE_FROM_EDITOR) {
			edit_state = PackedScene::GEN_EDIT_STATE_INSTANCE;
		}
#endif
		node = res->instantiate(edit_state);
		ERR_FAIL_NULL_V(node, nullptr);
		node->set_scene_instance_load_placeholder(get_scene_instance_load_placeholder());

		instantiated = true;

	}
	else {
		Node* n = memnew(Node);
		ERR_FAIL_NULL_V(n, nullptr);
		if (!node) {
			memdelete(n);
		}
		node = n;
		ERR_FAIL_NULL_V(node, nullptr);
	}

	if (is_instance()) {
		node->set_scene_file_path(get_scene_file_path());
		node->data.editable_instance = data.editable_instance;
	}

	List<const Node*> hidden_roots;
	List<const Node*> node_tree;
	node_tree.push_front(this);

	if (instantiated) {
		// Since nodes in the instantiated hierarchy won't be duplicated explicitly, we need to make
		// an inventory of all the nodes in the tree of the instantiated scene in order to transfer
		// the values of the properties

		Vector<const Node*> instance_roots;
		instance_roots.push_back(this);

		for (List<const Node*>::Element* N = node_tree.front(); N; N = N->next()) {
			for (int i = 0; i < N->get()->get_child_count(false); ++i) {
				Node* descendant = N->get()->get_child(i, false);

				// Skip nodes not really belonging to the instantiated hierarchy; they'll be
				// processed normally later but remember non-instantiated nodes that are hidden
				// below instantiated ones
				if (!instance_roots.has(descendant->get_owner())) {
					if (descendant->get_parent() && descendant->get_parent() != this &&
						descendant->data.owner != descendant->get_parent()) {
						hidden_roots.push_back(descendant);
					}
					continue;
				}

				node_tree.push_back(descendant);

				if (descendant->is_instance() && instance_roots.has(descendant->get_owner())) {
					instance_roots.push_back(descendant);
				}
			}
		}

		for (List<const Node*>::Element* N = node_tree.front(); N; N = N->next()) {
			const Node* source_node = N->get();
			if (source_node->get_scene_file_path().is_empty()) {
				continue;
			}

			NodePath relative_path = get_path_to(source_node);
			Node* duplicated_node = node->get_node_or_null(relative_path);
			ERR_CONTINUE(!duplicated_node);

			duplicated_node->data.editable_instance = source_node->data.editable_instance;
		}
	}

	if (get_name() != String()) {
		node->set_name(get_name());
	}

#ifdef TOOLS_ENABLED
	if ((p_flags & DUPLICATE_FROM_EDITOR) && r_duplimap) {
		r_duplimap->insert(this, node);
	}
#endif

	if (p_flags & DUPLICATE_GROUPS) {
		List<GroupInfo> gi;
		get_groups(&gi);
		for (const GroupInfo& E : gi) {
#ifdef TOOLS_ENABLED
			if ((p_flags & DUPLICATE_FROM_EDITOR) && !E.persistent) {
				continue;
			}
#endif

			node->add_to_group(E.name, E.persistent);
		}
	}

	for (int i = 0; i < get_child_count(false); i++) {
		if (instantiated && get_child(i, false)->data.owner == this) {
			continue; // part of instance
		}

		Node* dup = get_child(i, false)->_duplicate(p_flags, r_duplimap);
		if (!dup) {
			memdelete(node);
			return nullptr;
		}

		node->add_child(dup);
		if (i < node->get_child_count(false) - 1) {
			node->move_child(dup, i);
		}
	}

	for (const Node*& E : hidden_roots) {
		Node* parent = node->get_node(get_path_to(E->data.parent));
		if (!parent) {
			memdelete(node);
			return nullptr;
		}

		Node* dup = E->_duplicate(p_flags, r_duplimap);
		if (!dup) {
			memdelete(node);
			return nullptr;
		}

		parent->add_child(dup);
		int pos = E->get_index(false);

		if (pos < parent->get_child_count(false) - 1) {
			parent->move_child(dup, pos);
		}
	}
	return node;
}

Node* Node::duplicate(int p_flags) const
{
	ERR_THREAD_GUARD_V(nullptr);
	Node* dupe = _duplicate(p_flags);

	ERR_FAIL_NULL_V_MSG(dupe, nullptr, "Failed to duplicate node.");

	if (p_flags & DUPLICATE_SCRIPTS) {
		_duplicate_scripts(this, dupe);
	}

	_duplicate_properties(this, this, dupe, p_flags);

	if (p_flags & DUPLICATE_SIGNALS) {
		_duplicate_signals(this, dupe);
	}

	return dupe;
}

#ifdef TOOLS_ENABLED
Node* Node::duplicate_from_editor(HashMap<const Node*, Node*>& r_duplimap) const
{
	HashMap<Node*, HashMap<Ref<Resource>, Ref<Resource>>> tmp;
	return duplicate_from_editor(r_duplimap, nullptr, tmp);
}

Node* Node::duplicate_from_editor(HashMap<const Node*, Node*>& r_duplimap, Node* p_scene_root,
	HashMap<Node*, HashMap<Ref<Resource>, Ref<Resource>>>& p_resource_remap) const
{
	int flags = DUPLICATE_SIGNALS | DUPLICATE_GROUPS | DUPLICATE_SCRIPTS |
				DUPLICATE_USE_INSTANTIATION | DUPLICATE_FROM_EDITOR;
	Node* dupe = _duplicate(flags, &r_duplimap);

	ERR_FAIL_NULL_V_MSG(dupe, nullptr, "Failed to duplicate node.");

	if (flags & DUPLICATE_SCRIPTS) {
		_duplicate_scripts(this, dupe);
	}

	_duplicate_properties(this, this, dupe, flags);

	// This is used by SceneTreeDock's paste functionality. When pasting to foreign scene, resources
	// are duplicated.
	if (p_scene_root) {
		remap_node_resources(dupe, p_scene_root, p_resource_remap);
	}

	// Duplication of signals must happen after all the node descendants have been copied,
	// because re-targeting of connections from some descendant to another is not possible
	// if the emitter node comes later in tree order than the receiver
	_duplicate_signals(this, dupe);

	return dupe;
}

void Node::remap_node_resources(Node* p_node, Node* p_scene_root,
	HashMap<Node*, HashMap<Ref<Resource>, Ref<Resource>>>& p_resource_remap) const
{
	Node* local_scene =
		p_node->is_instance() ? p_node : (p_node->get_owner() ? p_node->get_owner() : p_scene_root);

	List<PropertyInfo> props;
	p_node->obj->get_property_list(&props);

	for (const PropertyInfo& E : props) {
		if (!(E.usage & PROPERTY_USAGE_STORAGE)) {
			continue;
		}

		Variant v = p_node->obj->get(E.name);
		if (!v.is_ref_counted()) {
			continue;
		}
		Ref<Resource> res = v;
		if (res.is_null()) {
			continue;
		}

		if (res->is_local_to_scene()) {
			if (local_scene == res->get_local_scene()) {
				continue;
			}
			Ref<Resource> dup =
				SceneState::get_remap_resource(res, p_resource_remap, nullptr, local_scene);
			p_node->obj->set(E.name, dup);
			continue;
		}

		if (res->is_built_in()) {
			// Use nullptr instead of a specific node (current scene root node) to represent the
			// scene, as the Make Scene Root operation may be executed.
			if (p_resource_remap[nullptr].has(res)) {
				p_node->obj->set(E.name, p_resource_remap[nullptr][res]);
				remap_nested_resources(res, p_resource_remap[nullptr]);
			}
		}
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		remap_node_resources(p_node->get_child(i), p_scene_root, p_resource_remap);
	}
}

void Node::remap_nested_resources(
	Ref<Resource> p_resource, HashMap<Ref<Resource>, Ref<Resource>>& p_resource_remap) const
{
	List<PropertyInfo> props;
	p_resource->obj->get_property_list(&props);

	for (const PropertyInfo& E : props) {
		if (!(E.usage & PROPERTY_USAGE_STORAGE)) {
			continue;
		}

		Variant v = p_resource->obj->get(E.name);
		if (v.is_ref_counted()) {
			Ref<Resource> res = v;
			if (res.is_valid()) {
				if (p_resource_remap.has(res)) {
					p_resource->obj->set(E.name, p_resource_remap[res]);
					remap_nested_resources(res, p_resource_remap);
				}
			}
		}
	}
}

void Node::_emit_editor_state_changed()
{
	// This is required for the SceneTreeEditor to properly keep track of when an update is needed.
	// This signal might be expensive and not needed for anything outside of the editor.
	if (Engine::get_singleton()->is_editor_hint()) {
		this->obj->emit_signal(SNAME("editor_state_changed"));
	}
}
#endif

void Node::_duplicate_scripts(const Node* p_original, Node* p_copy) const
{
	bool is_valid = false;
	Variant scr = p_original->obj->get(CoreStringName(script), &is_valid);
	if (is_valid) {
		p_copy->obj->set(CoreStringName(script), scr);
	}

	for (int i = 0; i < p_original->get_child_count(false); i++) {
		Node* copy_child = p_copy->get_child(i, false);
		ERR_FAIL_NULL_MSG(copy_child, "Child node disappeared while duplicating.");
		_duplicate_scripts(p_original->get_child(i, false), copy_child);
	}
}

// Duplicate node's properties.
// This has to be called after nodes have been duplicated since there might be properties
// of type Node that can be updated properly only if duplicated node tree is complete.
void Node::_duplicate_properties(
	const Node* p_root, const Node* p_original, Node* p_copy, int p_flags) const
{
	List<PropertyInfo> props;
	p_original->obj->get_property_list(&props);
	const StringName& script_property_name = CoreStringName(script);
	for (const PropertyInfo& E : props) {
		if (!(p_flags & DUPLICATE_INTERNAL_STATE) && !(E.usage & PROPERTY_USAGE_STORAGE)) {
			continue;
		}
		const StringName name = E.name;

		if (name == script_property_name) {
			continue;
		}

		Variant value = p_original->obj->get(name);
		// To keep classic behavior, because, in contrast, nowadays a resource would be duplicated.
		if (value.get_type() != Variant::OBJECT) {
			value = value.duplicate(true);
		}

		if (E.usage & PROPERTY_USAGE_ALWAYS_DUPLICATE) {
			Resource* res = Object::cast_to<Resource>(value);
			if (res) { // Duplicate only if it's a resource
				p_copy->obj->set(name, res->duplicate());
			}
		}
		else {
			if (value.get_type() == Variant::OBJECT) {
				Node* property_node = Object::cast_to<Node>(value);
				Variant out_value = value;
				if (property_node &&
					(p_root == property_node || p_root->is_ancestor_of(property_node))) {
					out_value = p_copy->get_node_or_null(p_original->get_path_to(property_node));
				}
				p_copy->obj->set(name, out_value);
			}
			else if (value.get_type() == Variant::ARRAY) {
				Array arr = value;
				if (arr.get_typed_builtin() == Variant::OBJECT) {
					for (int i = 0; i < arr.size(); i++) {
						Node* property_node = Object::cast_to<Node>(arr[i]);
						if (property_node &&
							(p_root == property_node || p_root->is_ancestor_of(property_node))) {
							arr[i] =
								p_copy->get_node_or_null(p_original->get_path_to(property_node));
						}
					}
				}
				p_copy->obj->set(name, arr);
			}
			else {
				p_copy->obj->set(name, value);
			}
		}
	}

	for (int i = 0; i < p_original->get_child_count(false); i++) {
		Node* copy_child = p_copy->get_child(i, false);
		ERR_FAIL_NULL_MSG(copy_child, "Child node disappeared while duplicating.");
		_duplicate_properties(p_root, p_original->get_child(i, false), copy_child, p_flags);
	}
}

// Duplication of signals must happen after all the node descendants have been copied,
// because re-targeting of connections from some descendant to another is not possible
// if the emitter node comes later in tree order than the receiver
void Node::_duplicate_signals(const Node* p_original, Node* p_copy) const
{
	if ((this != p_original) && !(p_original->is_ancestor_of(this))) {
		return;
	}

	List<const Node*> process_list;
	process_list.push_back(this);
	while (!process_list.is_empty()) {
		const Node* n = process_list.front()->get();
		process_list.pop_front();

		List<Object::Connection> conns;
		n->get_all_signal_connections(&conns);

		for (const Object::Connection& E : conns) {
			if (E.flags & Object::CONNECT_PERSIST) {
				// user connected
				NodePath p = p_original->get_path_to(n);
				Node* copy = p_copy->get_node(p);

				Node* target = Object::cast_to<Node>(E.callable.get_object());
				if (!target) {
					continue;
				}

				NodePath ptarget = p_original->get_path_to(target);
				if (ptarget.is_empty()) {
					continue;
				}

				Node* copytarget = target;

				// Attempt to find a path to the duplicate target, if it seems it's not part
				// of the duplicated and not yet parented hierarchy then at least try to connect
				// to the same target as the original

				if (p_copy->has_node(ptarget)) {
					copytarget = p_copy->get_node(ptarget);
				}

				if (copy && copytarget && E.callable.get_method() != StringName()) {
					Callable copy_callable = Callable(copytarget->obj.get(), E.callable.get_method());
					if (!copy->is_connected(E.signal.get_name(), copy_callable)) {
						int unbound_arg_count = E.callable.get_unbound_arguments_count();
						if (unbound_arg_count > 0) {
							copy_callable = copy_callable.unbind(unbound_arg_count);
						}
						if (E.callable.get_bound_arguments_count() > 0) {
							copy_callable = copy_callable.bindv(E.callable.get_bound_arguments());
						}
						copy->connect(E.signal.get_name(), copy_callable, E.flags);
					}
				}
			}
		}

		for (int i = 0; i < n->get_child_count(); i++) {
			process_list.push_back(n->get_child(i));
		}
	}
}

static void find_owned_by(Node* p_by, Node* p_node, List<Node*>* p_owned)
{
	if (p_node->get_owner() == p_by) {
		p_owned->push_back(p_node);
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		find_owned_by(p_by, p_node->get_child(i), p_owned);
	}
}

void Node::replace_by(Node* rp_node, bool p_keep_groups)
{
	ERR_THREAD_GUARD

	List<Node*> owned(data.owned);
	List<Node*> owned_by_owner;
	Node* owner = (data.owner == this) ? rp_node : data.owner;

	if (p_keep_groups) {
		List<GroupInfo> groups;
		get_groups(&groups);

		for (const GroupInfo& E : groups) {
			rp_node->add_to_group(E.name, E.persistent);
		}
	}

	_replace_connections_target(rp_node);

	if (data.owner) {
		for (int i = 0; i < get_child_count(); i++) {
			find_owned_by(data.owner, get_child(i), &owned_by_owner);
		}

		_clean_up_owner();
	}

	Node* parent = data.parent;
	int index_in_parent = get_index(false);

	if (data.parent) {
		parent->remove_child(this);
		parent->add_child(rp_node);
		parent->move_child(rp_node, index_in_parent);
	}

	this->obj->emit_signal(SNAME("replacing_by"), rp_node);

	// Move non-internal children to `rp_node`.
	while (get_child_count(false)) {
		Node* child = get_child(0, false);
		remove_child(child);
		Node* child_owner = child->get_owner() == this ? rp_node : child->get_owner();
		child->set_owner(nullptr);
		rp_node->add_child(child);
		child->set_owner(child_owner);
	}

	rp_node->set_owner(owner);
	for (Node* E : owned) {
		if (E->data.owner != rp_node) {
			E->set_owner(rp_node);
		}
	}

	for (Node* E : owned_by_owner) {
		if (E->data.owner != owner) {
			E->set_owner(owner);
		}
	}

	rp_node->set_scene_file_path(get_scene_file_path());
}

void Node::_replace_connections_target(Node* p_new_target)
{
	List<Object::Connection> cl;
	get_signals_connected_to_this(&cl);

	for (const Object::Connection& c : cl) {
		if (c.flags & Object::CONNECT_PERSIST) {
			c.signal.get_object()->disconnect(
				c.signal.get_name(), Callable(this->obj.get(), c.callable.get_method()));
			bool valid =
				p_new_target->obj->has_method(c.callable.get_method()) ||
				Ref<Script>(p_new_target->get_script()).is_null() ||
				Ref<Script>(p_new_target->get_script())->has_method(c.callable.get_method());
			ERR_CONTINUE_MSG(
				!valid, vformat("Attempt to connect signal '%s.%s' to nonexistent method '%s.%s'.",
							c.signal.get_object()->get_class(), c.signal.get_name(),
							c.callable.get_object()->get_class(), c.callable.get_method()));
			c.signal.get_object()->connect(
				c.signal.get_name(), Callable(p_new_target->obj.get(), c.callable.get_method()), c.flags);
		}
	}
}

bool Node::has_node_and_resource(const NodePath& p_path) const
{
	ERR_THREAD_GUARD_V(false);
	if (!has_node(p_path)) {
		return false;
	}
	Ref<Resource> res;
	Vector<StringName> leftover_path;
	Node* node = get_node_and_resource(p_path, res, leftover_path, false);

	return node;
}

Array Node::_get_node_and_resource(const NodePath& p_path)
{
	Ref<Resource> res;
	Vector<StringName> leftover_path;
	Node* node = get_node_and_resource(p_path, res, leftover_path, false);
	Array result;

	if (node) {
		result.push_back(node);
	}
	else {
		result.push_back(Variant());
	}

	if (res.is_valid()) {
		result.push_back(res);
	}
	else {
		result.push_back(Variant());
	}

	result.push_back(NodePath(Vector<StringName>(), leftover_path, false));

	return result;
}

Node* Node::get_node_and_resource(const NodePath& p_path, Ref<Resource>& r_res,
	Vector<StringName>& r_leftover_subpath, bool p_last_is_property) const
{
	ERR_THREAD_GUARD_V(nullptr);
	r_res = Ref<Resource>();
	r_leftover_subpath = Vector<StringName>();
	Node* node = get_node_or_null(p_path);
	if (!node) {
		return nullptr;
	}

	if (p_path.get_subname_count()) {
		int j = 0;
		// If not p_last_is_property, we shouldn't consider the last one as part of the resource
		for (; j < p_path.get_subname_count() - (int)p_last_is_property; j++) {
			bool is_valid = false;
			Variant new_res_v = j == 0 ? node->obj->get(p_path.get_subname(j), &is_valid)
									   : r_res->obj->get(p_path.get_subname(j), &is_valid);

			if (!is_valid) { // Found nothing on that path
				return nullptr;
			}

			Ref<Resource> new_res = new_res_v;

			if (new_res.is_null()) { // No longer a resource, assume property
				break;
			}

			r_res = new_res;
		}
		for (; j < p_path.get_subname_count(); j++) {
			// Put the rest of the subpath in the leftover path
			r_leftover_subpath.push_back(p_path.get_subname(j));
		}
	}

	return node;
}

void Node::_set_tree(SceneTree* p_tree)
{
	SceneTree* tree_changed_a = nullptr;
	SceneTree* tree_changed_b = nullptr;

	// ERR_FAIL_COND(p_scene && data.parent && !data.parent->data.scene); //nobug if both are null

	if (data.tree) {
		_propagate_exit_tree();

		tree_changed_a = data.tree;
	}

	data.tree = p_tree;

	if (data.tree) {
		_propagate_enter_tree();
		if (!data.parent || data.parent->data.ready_notified) { // No parent (root) or parent ready
			_propagate_ready(); // reverse_notification(NOTIFICATION_READY);
		}

		tree_changed_b = data.tree;
	}

	if (tree_changed_a) {
		tree_changed_a->tree_changed();
	}
	if (tree_changed_b) {
		tree_changed_b->tree_changed();
	}
}

#ifdef DEBUG_ENABLED
static HashMap<ObjectID, List<String>> _print_orphan_nodes_map;

static void _print_orphan_nodes_routine(Object* p_obj, void* p_user_data)
{
	Node* n = Object::cast_to<Node>(p_obj);
	if (!n) {
		return;
	}

	if (n->is_inside_tree()) {
		return;
	}

	Node* p = n;
	while (p->get_parent()) {
		p = p->get_parent();
	}

	String path;
	if (p == n) {
		path = n->get_name();
	}
	else {
		path = String(p->get_name()) + "/" + String(p->get_path_to(n));
	}

	String source;
	Variant script = n->get_script();
	if (!script.is_null()) {
		Resource* obj = Object::cast_to<Resource>(script);
		source = obj->get_path();
	}

	List<String> info_strings;
	info_strings.push_back(path);
	info_strings.push_back(n->obj->get_class());
	info_strings.push_back(source);

	_print_orphan_nodes_map[p_obj->get_instance_id()] = info_strings;
}
#endif // DEBUG_ENABLED

void Node::print_orphan_nodes()
{
#ifdef DEBUG_ENABLED
	// Make sure it's empty.
	_print_orphan_nodes_map.clear();

	// Collect and print information about orphan nodes.
	ObjectDB::debug_objects(_print_orphan_nodes_routine, nullptr);

	for (const KeyValue<ObjectID, List<String>>& E : _print_orphan_nodes_map) {
		print_line(itos(E.key) + " - Stray Node: " + E.value.get(0) + " (Type: " + E.value.get(1) +
				   ") (Source:" + E.value.get(2) + ")");
	}

	// Flush it after use.
	_print_orphan_nodes_map.clear();
#endif
}

TypedArray<int> Node::get_orphan_node_ids()
{
	TypedArray<int> ret;
#ifdef DEBUG_ENABLED
	// Make sure it's empty.
	_print_orphan_nodes_map.clear();

	// Collect and return information about orphan nodes.
	ObjectDB::debug_objects(_print_orphan_nodes_routine, nullptr);

	for (const KeyValue<ObjectID, List<String>>& E : _print_orphan_nodes_map) {
		ret.push_back(E.key);
	}

	// Flush it after use.
	_print_orphan_nodes_map.clear();
#endif
	return ret;
}

void Node::queue_free()
{
	// There are users which instantiate multiple scene trees for their games.
	// Use the node's own tree to handle its deletion when relevant.
	if (data.tree) {
		data.tree->queue_delete(this->obj.get());
	}
	else {
		SceneTree* tree = SceneTree::get_singleton();
		ERR_FAIL_NULL_MSG(tree, "Can't queue free a node when no SceneTree is available.");
		tree->queue_delete(this->obj.get());
	}
}

#ifdef TOOLS_ENABLED
static void _add_nodes_to_options(const Node* p_base, const Node* p_node, List<String>* r_options)
{
	if (p_node != p_base && !p_node->get_owner()) {
		return;
	}
	if (p_node->is_unique_name_in_owner() && p_node->get_owner() == p_base) {
		String n = "%" + p_node->get_name();
		r_options->push_back(n.quote());
	}
	String n = String(p_base->get_path_to(p_node));
	r_options->push_back(n.quote());
	for (int i = 0; i < p_node->get_child_count(); i++) {
		_add_nodes_to_options(p_base, p_node->get_child(i), r_options);
	}
}

void Node::get_argument_options(
	const StringName& p_function, int p_idx, List<String>* r_options) const
{
	const String pf = p_function;
	if (p_idx == 0 && (pf == "has_node" || pf == "get_node" || pf == "get_node_or_null")) {
		_add_nodes_to_options(this, this, r_options);
	}
	else if (p_idx == 0 &&
			   (pf == "add_to_group" || pf == "remove_from_group" || pf == "is_in_group")) {
		HashMap<StringName, String> global_groups(
			ProjectSettings::get_singleton()->get_global_groups_list());
		for (const KeyValue<StringName, String>& E : global_groups) {
			r_options->push_back(E.key.string().quote());
		}
	}
	this->obj->get_argument_options(p_function, p_idx, r_options);
}
#endif

void Node::clear_internal_tree_resource_paths()
{
	this->obj->clear_internal_resource_paths();
	for (KeyValue<StringName, Node*>& K : data.children) {
		K.value->clear_internal_tree_resource_paths();
	}
}

PackedStringArray Node::get_accessibility_configuration_warnings() const
{
	ERR_THREAD_GUARD_V(PackedStringArray());
	PackedStringArray ret;
	return ret;
}

PackedStringArray Node::get_configuration_warnings() const
{
	ERR_THREAD_GUARD_V(PackedStringArray());
	PackedStringArray ret;
	return ret;
}

void Node::update_configuration_warnings()
{
	ERR_THREAD_GUARD
#ifdef TOOLS_ENABLED
	if (!data.tree) {
		return;
	}
	if (data.tree->get_edited_scene_root() &&
		(data.tree->get_edited_scene_root() == this ||
			data.tree->get_edited_scene_root()->is_ancestor_of(this))) {
		data.tree->obj->emit_signal(SceneStringName(node_configuration_warning_changed), this);
	}
#endif
}

void Node::set_display_folded(bool p_folded)
{
	ERR_THREAD_GUARD
	data.display_folded = p_folded;
}

bool Node::is_displayed_folded() const { return data.display_folded; }

bool Node::is_ready() const { return !data.ready_first; }

void Node::request_ready()
{
	ERR_THREAD_GUARD
	data.ready_first = true;
}

void Node::_call_input(const Ref<InputEvent>& p_event)
{
	if (!is_inside_tree() || !get_viewport() || get_viewport()->is_input_handled()) {
		return;
	}
	input(p_event);
}

void Node::_call_shortcut_input(const Ref<InputEvent>& p_event)
{
	if (!is_inside_tree() || !get_viewport() || get_viewport()->is_input_handled()) {
		return;
	}
	shortcut_input(p_event);
}

void Node::_call_unhandled_input(const Ref<InputEvent>& p_event)
{
	if (!is_inside_tree() || !get_viewport() || get_viewport()->is_input_handled()) {
		return;
	}
	unhandled_input(p_event);
}

void Node::_call_unhandled_key_input(const Ref<InputEvent>& p_event)
{
	if (!is_inside_tree() || !get_viewport() || get_viewport()->is_input_handled()) {
		return;
	}
	unhandled_key_input(p_event);
}

void Node::_validate_property(PropertyInfo& p_property) const
{
	if ((p_property.name == "process_thread_group_order" ||
			p_property.name == "process_thread_messages") &&
		data.process_thread_group == PROCESS_THREAD_GROUP_INHERIT) {
		p_property.usage = PROPERTY_USAGE_NONE;
	}
}

String Node::_to_string()
{
	ERR_THREAD_GUARD_V(String());
	return (get_name() ? String(get_name()) + ":" : "") + this->obj->_to_string();
}

void Node::input(const Ref<InputEvent>& p_event) {}

void Node::shortcut_input(const Ref<InputEvent>& p_key_event) {}

void Node::unhandled_input(const Ref<InputEvent>& p_event) {}

void Node::unhandled_key_input(const Ref<InputEvent>& p_key_event) {}

Variant Node::_call_deferred_thread_group_bind(
	const Variant** p_args, int p_argcount, Callable::CallError& r_error)
{
	if (p_argcount < 1) {
		r_error.error = Callable::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
		r_error.expected = 1;
		return Variant();
	}

	if (!p_args[0]->is_string()) {
		r_error.error = Callable::CallError::CALL_ERROR_INVALID_ARGUMENT;
		r_error.argument = 0;
		r_error.expected = Variant::STRING_NAME;
		return Variant();
	}

	r_error.error = Callable::CallError::CALL_OK;

	StringName method = *p_args[0];

	call_deferred_thread_groupp(method, &p_args[1], p_argcount - 1, true);

	return Variant();
}

Variant Node::_call_thread_safe_bind(
	const Variant** p_args, int p_argcount, Callable::CallError& r_error)
{
	if (p_argcount < 1) {
		r_error.error = Callable::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
		r_error.expected = 1;
		return Variant();
	}

	if (!p_args[0]->is_string()) {
		r_error.error = Callable::CallError::CALL_ERROR_INVALID_ARGUMENT;
		r_error.argument = 0;
		r_error.expected = Variant::STRING_NAME;
		return Variant();
	}

	r_error.error = Callable::CallError::CALL_OK;

	StringName method = *p_args[0];

	call_thread_safep(method, &p_args[1], p_argcount - 1, true);

	return Variant();
}

void Node::call_deferred_thread_groupp(
	const StringName& p_method, const Variant** p_args, int p_argcount, bool p_show_error)
{
	ERR_FAIL_COND(!is_inside_tree());
	SceneTree::ProcessGroup* pg = (SceneTree::ProcessGroup*)data.process_group;
	pg->call_queue.push_callp(this->obj.get(), p_method, p_args, p_argcount, p_show_error);
}

void Node::set_deferred_thread_group(const StringName& p_property, const Variant& p_value)
{
	ERR_FAIL_COND(!is_inside_tree());
	SceneTree::ProcessGroup* pg = (SceneTree::ProcessGroup*)data.process_group;
	pg->call_queue.push_set(this->obj.get(), p_property, p_value);
}

void Node::notify_deferred_thread_group(int p_notification)
{
	ERR_FAIL_COND(!is_inside_tree());
	SceneTree::ProcessGroup* pg = (SceneTree::ProcessGroup*)data.process_group;
	pg->call_queue.push_notification(this->obj.get(), p_notification);
}

void Node::call_thread_safep(
	const StringName& p_method, const Variant** p_args, int p_argcount, bool p_show_error)
{
	if (is_accessible_from_caller_thread()) {
		Callable::CallError ce;
		this->obj->callp(p_method, p_args, p_argcount, ce);
		if (p_show_error && ce.error != Callable::CallError::CALL_OK) {
			ERR_FAIL_MSG("Error calling method from 'call_threadp': " +
						 Variant::get_call_error_text(this->obj.get(), p_method, p_args, p_argcount, ce) +
						 ".");
		}
	}
	else {
		call_deferred_thread_groupp(p_method, p_args, p_argcount, p_show_error);
	}
}

void Node::set_thread_safe(const StringName& p_property, const Variant& p_value)
{
	if (is_accessible_from_caller_thread()) {
		this->obj->set(p_property, p_value);
	}
	else {
		set_deferred_thread_group(p_property, p_value);
	}
}

void Node::notify_thread_safe(int p_notification)
{
	if (is_accessible_from_caller_thread()) {
		this->obj->notification(p_notification);
	}
	else {
		notify_deferred_thread_group(p_notification);
	}
}

RID Node::get_focused_accessibility_element() const { return get_accessibility_element(); }

Transform2D Node::get_accessibility_transform() const
{
	if (is_inside_tree() && data.parent) {
		return data.parent->get_accessibility_transform();
	}
	else {
		return Transform2D();
	}
}

void Node::queue_accessibility_update()
{
	if (is_inside_tree() && !is_part_of_edited_scene()) {
		data.tree->_accessibility_notify_change(this);
	}
}

RID Node::get_accessibility_element() const
{
	if (is_part_of_edited_scene()) {
		return RID();
	}
	if (unlikely(data.accessibility_element.is_null())) {
		Window* w = get_non_popup_window();
		if (w && w->get_window_id() != DisplayServerEnums::INVALID_WINDOW_ID &&
			get_window()->is_visible()) {
			data.accessibility_element = AccessibilityServer::get_singleton()->create_element(
				w->get_window_id(), AccessibilityServerEnums::ROLE_CONTAINER);
		}
	}
	return data.accessibility_element;
}

void Node::_bind_methods() {}

String Node::_get_name_num_separator()
{
	switch (GLOBAL_GET("editor/naming/node_name_num_separator").operator int()) {
	case 0:
		return "";
	case 1:
		return " ";
	case 2:
		return "_";
	case 3:
		return "-";
	}
	return " ";
}

Node::Node()
{
	this->obj = mem_make_unique<Object>();
	this->obj->_define_ancestry(Object::AncestralClass::NODE);
#ifdef DEBUG_ENABLED
	total_node_count.increment();
#endif
	// Default member initializer for bitfield is a C++20 extension, so:

	data.process_mode = PROCESS_MODE_INHERIT;
	data.physics_interpolation_mode = PHYSICS_INTERPOLATION_MODE_INHERIT;

	data.physics_process = false;
	data.process = false;

	data.physics_process_internal = false;
	data.process_internal = false;

	data.input = false;
	data.shortcut_input = false;
	data.unhandled_input = false;
	data.unhandled_key_input = false;

	data.physics_interpolated = true;
	data.physics_interpolation_reset_requested = false;
	data.physics_interpolated_client_side = false;
	data.use_identity_transform = false;

	data.use_placeholder = false;

	data.display_folded = false;
	data.editable_instance = false;

	data.ready_notified = false; // This is a small hack, so if a node is added during _ready() to
								 // the tree, it correctly gets the _ready() notification.
	data.ready_first = true;

	data.auto_translate_mode = AUTO_TRANSLATE_MODE_INHERIT;
	data.is_auto_translating = true;
	data.is_auto_translate_dirty = true;

	data.is_translation_domain_inherited = true;
	data.is_translation_domain_dirty = true;
}

Node::~Node()
{
	data.grouped.clear();
	data.owned.clear();
	data.children.clear();
	data.children_cache.clear();

	ERR_FAIL_COND(data.parent);
	ERR_FAIL_COND(data.children_cache.size());

#ifdef DEBUG_ENABLED
	total_node_count.decrement();
#endif
}

////////////////////////////////
// Multithreaded locked version of Object functions.

#ifdef DEBUG_ENABLED

void Node::set_script(const Variant& p_script)
{
	ERR_THREAD_GUARD;
	this->obj->set_script(p_script);
}

Variant Node::get_script() const
{
	ERR_THREAD_GUARD_V(Variant());
	return this->obj->get_script();
}

bool Node::has_meta(const StringName& p_name) const
{
	ERR_THREAD_GUARD_V(false);
	return this->obj->has_meta(p_name);
}

void Node::set_meta(const StringName& p_name, const Variant& p_value)
{
	ERR_THREAD_GUARD;
	this->obj->set_meta(p_name, p_value);
	_emit_editor_state_changed();
}

void Node::remove_meta(const StringName& p_name)
{
	ERR_THREAD_GUARD;
	this->obj->remove_meta(p_name);
	_emit_editor_state_changed();
}

Variant Node::get_meta(const StringName& p_name, const Variant& p_default) const
{
	ERR_THREAD_GUARD_V(Variant());
	return this->obj->get_meta(p_name, p_default);
}

void Node::get_meta_list(List<StringName>* p_list) const
{
	ERR_THREAD_GUARD;
	this->obj->get_meta_list(p_list);
}

Error Node::emit_signalp(const StringName& p_name, const Variant** p_args, int p_argcount)
{
	ERR_THREAD_GUARD_V(ERR_INVALID_PARAMETER);
	return this->obj->emit_signalp(p_name, p_args, p_argcount);
}

bool Node::has_signal(const StringName& p_name) const
{
	ERR_THREAD_GUARD_V(false);
	return this->obj->has_signal(p_name);
}

void Node::get_signal_list(List<MethodInfo>* p_signals) const
{
	ERR_THREAD_GUARD;
	this->obj->get_signal_list(p_signals);
}

void Node::get_signal_connection_list(
	const StringName& p_signal, List<Object::Connection>* p_connections) const
{
	ERR_THREAD_GUARD;
	this->obj->get_signal_connection_list(p_signal, p_connections);
}

void Node::get_all_signal_connections(List<Object::Connection>* p_connections) const
{
	ERR_THREAD_GUARD;
	this->obj->get_all_signal_connections(p_connections);
}

int Node::get_persistent_signal_connection_count() const
{
	ERR_THREAD_GUARD_V(0);
	return this->obj->get_persistent_signal_connection_count();
}

uint32_t Node::get_signal_connection_flags(
	const StringName& p_signal, const Callable& p_callable) const
{
	ERR_THREAD_GUARD_V(0);
	return this->obj->get_signal_connection_flags(p_signal, p_callable);
}

void Node::get_signals_connected_to_this(List<Object::Connection>* p_connections) const
{
	ERR_THREAD_GUARD;
	this->obj->get_signals_connected_to_this(p_connections);
}

Error Node::connect(const StringName& p_signal, const Callable& p_callable, uint32_t p_flags)
{
	ERR_THREAD_GUARD_V(ERR_INVALID_PARAMETER);

	Error retval = this->obj->connect(p_signal, p_callable, p_flags);
#ifdef TOOLS_ENABLED
	if (p_flags & Object::CONNECT_PERSIST) {
		_emit_editor_state_changed();
	}
#endif

	return retval;
}

void Node::disconnect(const StringName& p_signal, const Callable& p_callable)
{
	ERR_THREAD_GUARD;

#ifdef TOOLS_ENABLED
	// Already under thread guard, don't check again.
	uint32_t connection_flags = this->obj->get_signal_connection_flags(p_signal, p_callable);
#endif

	[[maybe_unused]] bool changed = this->obj->_disconnect(p_signal, p_callable);

#ifdef TOOLS_ENABLED
	if (changed && connection_flags & Object::CONNECT_PERSIST) {
		_emit_editor_state_changed();
	}
#endif
}

bool Node::is_connected(const StringName& p_signal, const Callable& p_callable) const
{
	ERR_THREAD_GUARD_V(false);
	return this->obj->is_connected(p_signal, p_callable);
}

bool Node::has_connections(const StringName& p_signal) const
{
	ERR_THREAD_GUARD_V(false);
	return this->obj->has_connections(p_signal);
}

#endif


