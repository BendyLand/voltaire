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
#include "core/templates/vector.h"
#include "node.h"
#include "scene/resources/environment.h"

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

Node::ProcessMode Node::get_process_mode() const { return data.process_mode; }

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

Ref<MultiplayerAPI> Node::get_multiplayer() const
{
	if (!is_inside_tree()) {
		return Ref<MultiplayerAPI>();
	}
	return data.tree->get_multiplayer(get_path());
}

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

uint32_t Node::get_process_thread_messages() const { return data.process_thread_messages; }

bool Node::is_processing_input() const { return data.input; }

bool Node::is_processing_shortcut_input() const { return data.shortcut_input; }

bool Node::is_processing_unhandled_input() const { return data.unhandled_input; }

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

StringName Node::get_name() const { return data.name; }

void Node::_set_name_nocheck(const StringName& p_name) { data.name = p_name; }

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

Node::InternalMode Node::get_internal_mode() const { return data.internal_mode; }

void Node::add_child(Node* rp_child, bool p_force_readable_name, InternalMode p_internal)
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
	ERR_FAIL_COND_MSG(data.blocked > 0,
		"Parent node is busy setting up children, `add_child()` failed. Consider "
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
		const String desc = "";
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
		vformat("No path can be resolved between the nodes and as they share no common ancestor."));

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

#endif

static void find_owned_by(Node* p_by, Node* p_node, List<Node*>* p_owned)
{
	if (p_node->get_owner() == p_by) {
		p_owned->push_back(p_node);
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		find_owned_by(p_by, p_node->get_child(i), p_owned);
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

#endif

PackedStringArray Node::get_configuration_warnings() const
{
	ERR_THREAD_GUARD_V(PackedStringArray());
	PackedStringArray ret;
	return ret;
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

void Node::input(const Ref<InputEvent>& p_event) {}

void Node::unhandled_input(const Ref<InputEvent>& p_event) {}

void Node::unhandled_key_input(const Ref<InputEvent>& p_key_event) {}

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


