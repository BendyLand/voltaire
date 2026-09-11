/**************************************************************************/
/*  packed_scene.cpp                                                      */
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
#include "core/io/file_access.h"
#include "core/io/missing_resource.h"
#include "core/io/resource_loader.h"
#include "core/templates/local_vector.h"
#include "packed_scene.h"
#include "scene/2d/node_2d.h"
#include "scene/gui/control.h"
#include "scene/main/instance_placeholder.h"
#include "scene/main/missing_node.h"
#include "scene/property_utils.h"

#ifndef _3D_DISABLED
#include "scene/3d/node_3d.h"
#endif // _3D_DISABLED

#define PACKED_SCENE_VERSION 3

#ifdef TOOLS_ENABLED
SceneState::InstantiationWarningNotify SceneState::instantiation_warn_notify = nullptr;
#endif

bool SceneState::can_instantiate() const { return nodes.size() > 0; }

static Node* _find_node_by_id(Node* p_owner, Node* p_node, int32_t p_id)
{
	if (p_owner == p_node || p_node->get_owner() == p_owner) {
		if (p_node->get_unique_scene_id() == p_id) {
			return p_node;
		}
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		Node* found = _find_node_by_id(p_owner, p_node->get_child(i), p_id);
		if (found) {
			return found;
		}
	}

	return nullptr;
}

static int _nm_get_string(const String& p_string, HashMap<StringName, int>& name_map)
{
	if (name_map.has(p_string)) {
		return name_map[p_string];
	}

	int idx = name_map.size();
	name_map[p_string] = idx;
	return idx;
}

void SceneState::set_path(const String& p_path) { path = p_path; }

String SceneState::get_path() const { return path; }

int SceneState::find_node_by_path(const NodePath& p_node) const
{
	ERR_FAIL_COND_V_MSG(node_path_cache.is_empty(), -1,
		"This operation requires the node cache to have been built.");

	if (!node_path_cache.has(p_node)) {
		// If not in this scene state, find node path by scene inheritance.
		if (get_base_scene_state().is_valid()) {
			int idx = get_base_scene_state()->find_node_by_path(p_node);
			if (idx != -1) {
				int rkey = _find_base_scene_node_remap_key(idx);
				if (rkey == -1) {
					rkey = nodes.size() + base_scene_node_remap.size();
					base_scene_node_remap[rkey] = idx;
				}
				return rkey;
			}
		}
		return -1;
	}

	int nid = node_path_cache[p_node];

	if (get_base_scene_state().is_valid() && !base_scene_node_remap.has(nid)) {
		// for nodes that _do_ exist in current scene, still try to look for
		// the node in the instantiated scene, as a property may be missing
		// from the local one
		int idx = get_base_scene_state()->find_node_by_path(p_node);
		if (idx != -1) {
			base_scene_node_remap[nid] = idx;
		}
	}

	return nid;
}

int SceneState::_find_base_scene_node_remap_key(int p_idx) const
{
	for (const KeyValue<int, int>& E : base_scene_node_remap) {
		if (E.value == p_idx) {
			return E.key;
		}
	}
	return -1;
}

bool SceneState::is_node_in_group(int p_node, const StringName& p_group) const
{
	ERR_FAIL_COND_V(p_node < 0, false);

	if (p_node < nodes.size()) {
		const StringName* namep = names.ptr();
		for (int i = 0; i < nodes[p_node].groups.size(); i++) {
			if (namep[nodes[p_node].groups[i]] == p_group) {
				return true;
			}
		}
	}

	if (base_scene_node_remap.has(p_node)) {
		return get_base_scene_state()->is_node_in_group(base_scene_node_remap[p_node], p_group);
	}

	return false;
}

bool SceneState::disable_placeholders = false;

void SceneState::set_disable_placeholders(bool p_disable) { disable_placeholders = p_disable; }

bool SceneState::is_connection(
	int p_node, const StringName& p_signal, int p_to_node, const StringName& p_to_method) const
{
	ERR_FAIL_COND_V(p_node < 0, false);
	ERR_FAIL_COND_V(p_to_node < 0, false);

	if (p_node < nodes.size() && p_to_node < nodes.size()) {
		int signal_idx = -1;
		int method_idx = -1;
		for (int i = 0; i < names.size(); i++) {
			if (names[i] == p_signal) {
				signal_idx = i;
			}
			else if (names[i] == p_to_method) {
				method_idx = i;
			}
		}

		if (signal_idx >= 0 && method_idx >= 0) {
			// signal and method strings are stored..

			for (int i = 0; i < connections.size(); i++) {
				if (connections[i].from == p_node && connections[i].to == p_to_node &&
					connections[i].signal == signal_idx && connections[i].method == method_idx) {
					return true;
				}
			}
		}
	}

	if (base_scene_node_remap.has(p_node) && base_scene_node_remap.has(p_to_node)) {
		return get_base_scene_state()->is_connection(
			base_scene_node_remap[p_node], p_signal, base_scene_node_remap[p_to_node], p_to_method);
	}

	return false;
}

int SceneState::get_node_count() const { return nodes.size(); }

StringName SceneState::get_node_type(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, nodes.size(), StringName());
	if (nodes[p_idx].type == TYPE_INSTANTIATED) {
		return StringName();
	}
	return names[nodes[p_idx].type];
}

StringName SceneState::get_node_name(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, nodes.size(), StringName());
	return names[nodes[p_idx].name];
}

int SceneState::get_node_index(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, nodes.size(), -1);
	return nodes[p_idx].index;
}

bool SceneState::is_node_instance_placeholder(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, nodes.size(), false);

	return nodes[p_idx].instance >= 0 && (nodes[p_idx].instance & FLAG_INSTANCE_IS_PLACEHOLDER);
}

Vector<StringName> SceneState::get_node_groups(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, nodes.size(), Vector<StringName>());
	Vector<StringName> groups;
	for (int i = 0; i < nodes[p_idx].groups.size(); i++) {
		groups.push_back(names[nodes[p_idx].groups[i]]);
	}
	return groups;
}

int32_t SceneState::get_node_unique_id(int p_idx) const
{
	if (p_idx >= ids.size()) {
		return Node::UNIQUE_SCENE_ID_UNASSIGNED;
	}
	return ids[p_idx];
}

NodePath SceneState::get_node_path(int p_idx, bool p_for_parent) const
{
	ERR_FAIL_INDEX_V(p_idx, nodes.size(), NodePath());

	if (nodes[p_idx].parent < 0 || nodes[p_idx].parent == NO_PARENT_SAVED) {
		if (p_for_parent) {
			return NodePath();
		}
		else {
			return NodePath(".");
		}
	}

	Vector<StringName> sub_path;
	NodePath base_path;
	int nidx = p_idx;
	while (true) {
		if (nodes[nidx].parent == NO_PARENT_SAVED || nodes[nidx].parent < 0) {
			sub_path.insert(0, ".");
			break;
		}

		if (!p_for_parent || p_idx != nidx) {
			sub_path.insert(0, names[nodes[nidx].name]);
		}

		if (nodes[nidx].parent & FLAG_ID_IS_PATH) {
			base_path = node_paths[nodes[nidx].parent & FLAG_MASK];
			break;
		}
		else {
			nidx = nodes[nidx].parent & FLAG_MASK;
		}
	}

	for (int i = base_path.get_name_count() - 1; i >= 0; i--) {
		sub_path.insert(0, base_path.get_name(i));
	}

	if (sub_path.is_empty()) {
		return NodePath(".");
	}

	return NodePath(sub_path, false);
}

PackedInt32Array SceneState::get_node_id_path(int p_idx) const
{
	PackedInt32Array pp = get_node_parent_id_path(p_idx);
	if (pp.is_empty()) {
		return pp;
	}

	if (p_idx < ids.size()) {
		pp.push_back(ids[p_idx]);
		return pp;
	}

	return PackedInt32Array();
}

PackedInt32Array SceneState::get_node_parent_id_path(int p_idx) const
{
	if (nodes[p_idx].parent < 0 || nodes[p_idx].parent == NO_PARENT_SAVED) {
		return PackedInt32Array();
	}

	if (nodes[p_idx].parent & FLAG_ID_IS_PATH) {
		int id = nodes[p_idx].parent & FLAG_MASK;
		if (id >= id_paths.size()) {
			return PackedInt32Array();
		}
		return id_paths[id];
	}

	return PackedInt32Array();
}

PackedInt32Array SceneState::get_node_owner_id_path(int p_idx) const
{
	if (nodes[p_idx].owner < 0) {
		return PackedInt32Array();
	}

	if (nodes[p_idx].owner & FLAG_ID_IS_PATH) {
		int id = nodes[p_idx].owner & FLAG_MASK;
		if (id >= id_paths.size()) {
			return PackedInt32Array();
		}
		return id_paths[id];
	}

	return PackedInt32Array();
}

int SceneState::get_node_property_count(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, nodes.size(), -1);
	return nodes[p_idx].properties.size();
}

StringName SceneState::get_node_property_name(int p_idx, int p_prop) const
{
	ERR_FAIL_INDEX_V(p_idx, nodes.size(), StringName());
	ERR_FAIL_INDEX_V(p_prop, nodes[p_idx].properties.size(), StringName());
	return names[nodes[p_idx].properties[p_prop].name & FLAG_PROP_NAME_MASK];
}

Vector<String> SceneState::get_node_deferred_nodepath_properties(int p_idx) const
{
	Vector<String> ret;
	ERR_FAIL_COND_V(p_idx < 0, ret);

	if (p_idx < nodes.size()) {
		// Find in built-in nodes.
		for (int i = 0; i < nodes[p_idx].properties.size(); i++) {
			uint32_t idx = nodes[p_idx].properties[i].name;
			if (idx & FLAG_PATH_PROPERTY_IS_NODE) {
				ret.push_back(names[idx & FLAG_PROP_NAME_MASK]);
			}
		}
		return ret;
	}

	// Property not found, try on instance.
	HashMap<int, int>::ConstIterator I = base_scene_node_remap.find(p_idx);
	if (I) {
		return get_base_scene_state()->get_node_deferred_nodepath_properties(I->value);
	}

	return ret;
}

NodePath SceneState::get_node_owner_path(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, nodes.size(), NodePath());
	if (nodes[p_idx].owner < 0 || nodes[p_idx].owner == NO_PARENT_SAVED) {
		return NodePath(); // root likely
	}
	if (nodes[p_idx].owner & FLAG_ID_IS_PATH) {
		return node_paths[nodes[p_idx].owner & FLAG_MASK];
	}
	else {
		return get_node_path(nodes[p_idx].owner & FLAG_MASK);
	}
}

int SceneState::get_connection_count() const { return connections.size(); }

NodePath SceneState::get_connection_source(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, connections.size(), NodePath());
	if (connections[p_idx].from & FLAG_ID_IS_PATH) {
		return node_paths[connections[p_idx].from & FLAG_MASK];
	}
	else {
		return get_node_path(connections[p_idx].from & FLAG_MASK);
	}
}

StringName SceneState::get_connection_signal(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, connections.size(), StringName());
	return names[connections[p_idx].signal];
}

NodePath SceneState::get_connection_target(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, connections.size(), NodePath());
	if (connections[p_idx].to & FLAG_ID_IS_PATH) {
		return node_paths[connections[p_idx].to & FLAG_MASK];
	}
	else {
		return get_node_path(connections[p_idx].to & FLAG_MASK);
	}
}

PackedInt32Array SceneState::get_connection_target_id_path(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, connections.size(), PackedInt32Array());
	if (connections[p_idx].to & FLAG_ID_IS_PATH && connections[p_idx].to < id_paths.size()) {
		return id_paths[connections[p_idx].to];
	}
	else {
		return PackedInt32Array();
	}
}

PackedInt32Array SceneState::get_connection_source_id_path(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, connections.size(), PackedInt32Array());
	if (connections[p_idx].from & FLAG_ID_IS_PATH && connections[p_idx].from < id_paths.size()) {
		return id_paths[connections[p_idx].from];
	}
	else {
		return PackedInt32Array();
	}
}

StringName SceneState::get_connection_method(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, connections.size(), StringName());
	return names[connections[p_idx].method];
}

int SceneState::get_connection_flags(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, connections.size(), -1);
	return connections[p_idx].flags;
}

int SceneState::get_connection_unbinds(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, connections.size(), -1);
	return connections[p_idx].unbinds;
}

bool SceneState::has_connection(const NodePath& p_node_from, const StringName& p_signal,
	const NodePath& p_node_to, const StringName& p_method, bool p_no_inheritance)
{
	// this method cannot be const because of this
	Ref<SceneState> ss = this;

	do {
		for (int i = 0; i < ss->connections.size(); i++) {
			const ConnectionData& c = ss->connections[i];

			NodePath np_from;

			if (c.from & FLAG_ID_IS_PATH) {
				np_from = ss->node_paths[c.from & FLAG_MASK];
			}
			else {
				np_from = ss->get_node_path(c.from);
			}

			NodePath np_to;

			if (c.to & FLAG_ID_IS_PATH) {
				np_to = ss->node_paths[c.to & FLAG_MASK];
			}
			else {
				np_to = ss->get_node_path(c.to);
			}

			StringName sn_signal = ss->names[c.signal];
			StringName sn_method = ss->names[c.method];

			if (np_from == p_node_from && sn_signal == p_signal && np_to == p_node_to &&
				sn_method == p_method) {
				return true;
			}
		}

		if (p_no_inheritance) {
			break;
		}

		ss = ss->get_base_scene_state();
	} while (ss.is_valid());

	return false;
}

Vector<NodePath> SceneState::get_editable_instances() const { return editable_instances; }

int SceneState::add_name(const StringName& p_name)
{
	names.push_back(p_name);
	return names.size() - 1;
}

int SceneState::add_node_path(const NodePath& p_path, const PackedInt32Array& p_uid_path)
{
	node_paths.push_back(p_path);
	id_paths.push_back(p_uid_path);
	return (node_paths.size() - 1) | FLAG_ID_IS_PATH;
}

int SceneState::add_node(int p_parent, int p_owner, int p_type, int p_name, int p_instance,
	int p_index, int32_t p_unique_id)
{
	NodeData nd;
	nd.parent = p_parent;
	nd.owner = p_owner;
	nd.type = p_type;
	nd.name = p_name;
	nd.instance = p_instance;
	nd.index = p_index;

	nodes.push_back(nd);

	ids.push_back(p_unique_id);

	return nodes.size() - 1;
}

void SceneState::add_node_group(int p_node, int p_group)
{
	ERR_FAIL_INDEX(p_node, nodes.size());
	ERR_FAIL_INDEX(p_group, names.size());
	nodes.write[p_node].groups.push_back(p_group);
}

void SceneState::add_editable_instance(const NodePath& p_path)
{
	editable_instances.push_back(p_path);
}

bool SceneState::remove_group_references(const StringName& p_name)
{
	bool edited = false;
	for (NodeData& node : nodes) {
		for (const int& group : node.groups) {
			if (names[group] == p_name) {
				node.groups.erase(group);
				edited = true;
				break;
			}
		}
	}
	return edited;
}

bool SceneState::rename_group_references(const StringName& p_old_name, const StringName& p_new_name)
{
	bool edited = false;
	for (const NodeData& node : nodes) {
		for (const int& group : node.groups) {
			if (names[group] == p_old_name) {
				names.write[group] = p_new_name;
				edited = true;
				break;
			}
		}
	}
	return edited;
}

HashSet<StringName> SceneState::get_all_groups()
{
	HashSet<StringName> ret;
	for (const NodeData& node : nodes) {
		for (const int& group : node.groups) {
			ret.insert(names[group]);
		}
	}
	return ret;
}

Vector<String> SceneState::_get_node_groups(int p_idx) const
{
	Vector<StringName> groups = get_node_groups(p_idx);
	Vector<String> ret;

	for (int i = 0; i < groups.size(); i++) {
		ret.push_back(groups[i]);
	}

	return ret;
}

SceneState::SceneState() {}

Error PackedScene::pack(Node* p_scene) { return state->pack(p_scene); }

void PackedScene::clear() { state->clear(); }

bool PackedScene::can_instantiate() const { return state->can_instantiate(); }

Ref<SceneState> PackedScene::get_state() const { return state; }

void PackedScene::set_path(const String& p_path, bool p_take_over)
{
	state->set_path(p_path);
	Resource::set_path(p_path, p_take_over);
}

void PackedScene::set_path_cache(const String& p_path)
{
	state->set_path(p_path);
	Resource::set_path_cache(p_path);
}

void PackedScene::reset_state() { clear(); }

PackedScene::PackedScene() { state.instantiate(); }


