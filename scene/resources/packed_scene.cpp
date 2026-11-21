/**************************************************************************/
/*  packed_scene.cpp                                                      */
/**************************************************************************/
/*                         This file is part of:                          */
/*                            VOLTAIRE ENGINE                             */
/*                https://github.com/BendyLand/Voltaire                   */
/**************************************************************************/
/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
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

static int _nm_get_string(const String& p_string, HashMap<String, int>& name_map)
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

bool SceneState::is_node_in_group(int p_node, const String& p_group) const
{
	ERR_FAIL_COND_V(p_node < 0, false);

	if (p_node < nodes.size()) {
		const String* namep = names.ptr();
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
	int p_node, const String& p_signal, int p_to_node, const String& p_to_method) const
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

String SceneState::get_node_type(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, nodes.size(), String());
	if (nodes[p_idx].type == TYPE_INSTANTIATED) {
		return String();
	}
	return names[nodes[p_idx].type];
}

String SceneState::get_node_name(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, nodes.size(), String());
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

Vector<String> SceneState::get_node_groups(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, nodes.size(), Vector<String>());
	Vector<String> groups;
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

	Vector<String> sub_path;
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

String SceneState::get_node_property_name(int p_idx, int p_prop) const
{
	ERR_FAIL_INDEX_V(p_idx, nodes.size(), String());
	ERR_FAIL_INDEX_V(p_prop, nodes[p_idx].properties.size(), String());
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

String SceneState::get_connection_signal(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, connections.size(), String());
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

String SceneState::get_connection_method(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, connections.size(), String());
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

bool SceneState::has_connection(const NodePath& p_node_from, const String& p_signal,
	const NodePath& p_node_to, const String& p_method, bool p_no_inheritance)
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

			String sn_signal = ss->names[c.signal];
			String sn_method = ss->names[c.method];

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

int SceneState::add_name(const String& p_name)
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

bool SceneState::remove_group_references(const String& p_name)
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

bool SceneState::rename_group_references(const String& p_old_name, const String& p_new_name)
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

HashSet<String> SceneState::get_all_groups()
{
	HashSet<String> ret;
	for (const NodeData& node : nodes) {
		for (const int& group : node.groups) {
			ret.insert(names[group]);
		}
	}
	return ret;
}

Vector<String> SceneState::_get_node_groups(int p_idx) const
{
	Vector<String> groups = get_node_groups(p_idx);
	Vector<String> ret;

	for (int i = 0; i < groups.size(); i++) {
		ret.push_back(groups[i]);
	}

	return ret;
}

Error PackedScene::pack(Node*
p_scene) { return state->pack(p_scene); }

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

Ref<SceneState> SceneState::get_base_scene_state() const
{
	Ref<SceneState> ss = memnew(SceneState);
	return ss;
}

Node* PackedScene::instantiate(PackedScene::GenEditState) const {}

Error SceneState::pack(Node*) {}

void SceneState::clear() {}

void PackedScene::reload_from_file() {}

#ifdef PACKED_SCENE_VERSION
#undef PACKED_SCENE_VERSION
#endif

// --- resource_format_text.cpp ---

#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/missing_resource.h"
#include "resource_format_text.h"
#include "scene/property_utils.h"

void ResourceLoaderText::_printerr()
{
	ERR_PRINT(vformat("%s:%d - Parse Error: %s.", res_path, lines, error_text));
}

Ref<Resource> ResourceLoaderText::get_resource() { return resource; }

void ResourceLoaderText::_count_resources()
{
	Ref<FileAccess> scan_f = FileAccess::open(f->get_path(), FileAccess::READ);
	if (scan_f.is_null()) {
		return;
	}

	resources_total = 0;
	resource_current = 0;

	bool has_main_resource = false;
	while (!scan_f->eof_reached()) {
		String line = scan_f->get_line().strip_edges();

		// Only count resources that contribute to progress
		// (ext_resources are loaded asynchronously and don't count).
		// Note: nodes are all parsed together as part of the main resource (PackedScene),
		// so they only contribute 1 to the progress count, not one per node.
		if (line.begins_with("[sub_resource ")) {
			resources_total++;
		}
		else if (line.begins_with("[resource]") || line.begins_with("[node ")) {
			// Main resource or scene with nodes - only count once.
			if (!has_main_resource) {
				resources_total++;
				has_main_resource = true;
			}
		}
	}
}

int ResourceLoaderText::get_stage() const { return resource_current; }

int ResourceLoaderText::get_stage_count() const
{
	return resources_total; //+ext_resources;
}

Ref<Resource> ResourceFormatLoaderText::load(const String& p_path, const String& p_original_path,
	Error* r_error, bool p_use_sub_threads, float* r_progress, CacheMode p_cache_mode)
{
	if (r_error) {
		*r_error = ERR_CANT_OPEN;
	}

	Error err;

	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ, &err);

	ERR_FAIL_COND_V_MSG(err != OK, Ref<Resource>(), "Cannot open file '" + p_path + "'.");

	ResourceLoaderText loader;
	String path = !p_original_path.is_empty() ? p_original_path : p_path;
	switch (p_cache_mode) {
	case CACHE_MODE_IGNORE:
	case CACHE_MODE_REUSE:
	case CACHE_MODE_REPLACE:
		loader.cache_mode = p_cache_mode;
		loader.cache_mode_for_external = CACHE_MODE_REUSE;
		break;
	case CACHE_MODE_IGNORE_DEEP:
		loader.cache_mode = ResourceFormatLoader::CACHE_MODE_IGNORE;
		loader.cache_mode_for_external = p_cache_mode;
		break;
	case CACHE_MODE_REPLACE_DEEP:
		loader.cache_mode = ResourceFormatLoader::CACHE_MODE_REPLACE;
		loader.cache_mode_for_external = p_cache_mode;
		break;
	}
	loader.use_sub_threads = p_use_sub_threads;
	loader.local_path = ProjectSettings::localize_path(path);
	loader.progress = r_progress;
	loader.res_path = loader.local_path;
	loader.open(f);
	err = loader.load();
	if (r_error) {
		*r_error = err;
	}
	if (err == OK) {
		return loader.get_resource();
	}
	else {
		return Ref<Resource>();
	}
}

void ResourceFormatLoaderText::get_recognized_extensions_for_type(
	const String& p_type, List<String>* p_extensions) const
{
	if (p_type.is_empty()) {
		get_recognized_extensions(p_extensions);
		return;
	}

	// Don't allow .tres for PackedScenes or GDExtension.
	if (p_type != "PackedScene" && p_type != "GDExtension") {
		p_extensions->push_back("tres");
	}
}

void ResourceFormatLoaderText::get_recognized_extensions(List<String>* p_extensions) const
{
	p_extensions->push_back("tscn");
	p_extensions->push_back("tres");
}

bool ResourceFormatLoaderText::handles_type(const String& p_type) const { return true; }

void ResourceFormatLoaderText::get_classes_used(
	const String& p_path, HashSet<String>* r_classes)
{
	const String type = get_resource_type(p_path);
	if (!type.is_empty()) {
		r_classes->insert(type);
	}

	// ...for anything else must test...

	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	if (f.is_null()) {
		return; // Could not read.
	}

	ResourceLoaderText loader;
	loader.local_path = ProjectSettings::localize_path(p_path);
	loader.res_path = loader.local_path;
	loader.open(f);
	loader.get_classes_used(r_classes);
}

String ResourceFormatLoaderText::get_resource_type(const String& p_path) const
{
	const String ext = p_path.get_extension().to_lower();
	if (ext == "tscn") {
		return "PackedScene";
	}
	else if (ext != "tres") {
		return String();
	}

	// ...for anything else must test...

	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	if (f.is_null()) {
		return ""; // could not read
	}

	ResourceLoaderText loader;
	loader.local_path = ProjectSettings::localize_path(p_path);
	loader.res_path = loader.local_path;
	String r = loader.recognize(f);
	return r;
}

String ResourceFormatLoaderText::get_resource_script_class(const String& p_path) const
{
	if (!p_path.has_extension("tres")) {
		return String();
	}

	// ...for anything else must test...

	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	if (f.is_null()) {
		return ""; // could not read
	}

	ResourceLoaderText loader;
	loader.local_path = ProjectSettings::localize_path(p_path);
	loader.res_path = loader.local_path;
	return loader.recognize_script_class(f);
}

ResourceUID::ID ResourceFormatLoaderText::get_resource_uid(const String& p_path) const
{
	const String ext = p_path.get_extension().to_lower();
	if (ext != "tscn" && ext != "tres") {
		return ResourceUID::INVALID_ID;
	}

	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	if (f.is_null()) {
		return ResourceUID::INVALID_ID; // could not read
	}

	ResourceLoaderText loader;
	loader.local_path = ProjectSettings::localize_path(p_path);
	loader.res_path = loader.local_path;
	return loader.get_uid(f);
}

bool ResourceFormatLoaderText::has_custom_uid_support() const { return true; }

void ResourceFormatLoaderText::get_dependencies(
	const String& p_path, List<String>* p_dependencies, bool p_add_types)
{
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	if (f.is_null()) {
		ERR_FAIL();
	}

	ResourceLoaderText loader;
	loader.local_path = ProjectSettings::localize_path(p_path);
	loader.res_path = loader.local_path;
	loader.get_dependencies(f, p_dependencies, p_add_types);
}

Error ResourceFormatLoaderText::rename_dependencies(
	const String& p_path, const HashMap<String, String>& p_map)
{
	Error err = OK;
	{
		Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
		if (f.is_null()) {
			ERR_FAIL_V(ERR_CANT_OPEN);
		}

		ResourceLoaderText loader;
		loader.local_path = ProjectSettings::localize_path(p_path);
		loader.res_path = loader.local_path;
		err = loader.rename_dependencies(f, p_path, p_map);
	}

	Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
	if (err == OK && da->file_exists(p_path + ".depren")) {
		da->remove(p_path);
		da->rename(p_path + ".depren", p_path);
	}

	return err;
}

String ResourceFormatSaverTextInstance::_write_resources(void* ud, const Ref<Resource>& p_resource)
{
	ResourceFormatSaverTextInstance* rsi = static_cast<ResourceFormatSaverTextInstance*>(ud);
	return rsi->_write_resource(p_resource);
}

Error ResourceLoaderText::set_uid(Ref<FileAccess> p_f, ResourceUID::ID p_uid)
{
	open(p_f, true);
	ERR_FAIL_COND_V(error != OK, error);
	ignore_resource_parsing = true;

	Ref<FileAccess> fw;

	fw = FileAccess::open(local_path + ".uidren", FileAccess::WRITE);
	if (is_scene) {
		fw->store_string("[gd_scene format=" + itos(format_version) + " uid=\"" +
						 ResourceUID::id_to_text(p_uid) + "\"]");
	}
	else {
		String script_res_text;
		if (!script_class.is_empty()) {
			script_res_text = "script_class=\"" + script_class + "\" ";
		}

		fw->store_string("[gd_resource type=\"" + res_type + "\" " + script_res_text +
						 "format=" + itos(format_version) + " uid=\"" +
						 ResourceUID::id_to_text(p_uid) + "\"]");
	}

	uint8_t c = f->get_8();
	while (!f->eof_reached()) {
		fw->store_8(c);
		c = f->get_8();
	}

	bool all_ok = fw->get_error() == OK;

	if (!all_ok) {
		return ERR_CANT_CREATE;
	}

	return OK;
}

Error ResourceFormatSaverText::save(
	const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags)
{
	if (p_path.ends_with(".tscn") && Ref<PackedScene>(p_resource).is_null()) {
		return ERR_FILE_UNRECOGNIZED;
	}

	ResourceFormatSaverTextInstance saver;
	return saver.save(p_path, p_resource, p_flags);
}

Error ResourceFormatSaverText::set_uid(const String& p_path, ResourceUID::ID p_uid)
{
	String lc = p_path.to_lower();
	if (!lc.ends_with(".tscn") && !lc.ends_with(".tres")) {
		return ERR_FILE_UNRECOGNIZED;
	}

	String local_path = ProjectSettings::localize_path(p_path);
	Error err = OK;
	{
		Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ);
		if (file.is_null()) {
			ERR_FAIL_V(ERR_CANT_OPEN);
		}

		ResourceLoaderText loader;
		loader.local_path = local_path;
		loader.res_path = loader.local_path;
		err = loader.set_uid(file, p_uid);
	}

	if (err == OK) {
		Ref<DirAccess> da = DirAccess::create(DirAccess::ACCESS_RESOURCES);
		da->remove(local_path);
		da->rename(local_path + ".uidren", local_path);
	}

	return err;
}

bool ResourceFormatSaverText::recognize(const Ref<Resource>& p_resource) const
{
	return true; // All resources recognized!
}

void ResourceFormatSaverText::get_recognized_extensions(
	const Ref<Resource>& p_resource, List<String>* p_extensions) const
{
	if (Ref<PackedScene>(p_resource).is_valid()) {
		p_extensions->push_back("tscn"); // Text scene.
	}
	else {
		p_extensions->push_back("tres"); // Text resource.
	}
}

void ResourceLoaderText::open(Ref<FileAccess>, bool) {}

Error ResourceLoaderText::rename_dependencies(Ref<FileAccess>, String const&,
	HashMap<String, String, HashMapHasherDefault, HashMapComparatorDefault<String, void>,
		DefaultTypedAllocator<HashMapElement<String, String>>> const&)
{
	return OK;
}

String ResourceLoaderText::recognize_script_class(Ref<FileAccess>) { return String(); }

String ResourceLoaderText::recognize(Ref<FileAccess>) { return String(); }

Error ResourceLoaderText::load() { return OK; }

ResourceUID::ID ResourceLoaderText::get_uid(Ref<FileAccess>) { return ResourceUID::ID(); }

void ResourceLoaderText::get_dependencies(Ref<FileAccess>, List<String, DefaultAllocator>*, bool) {}

Error ResourceLoaderText::get_classes_used(
	HashSet<String, HashMapHasherDefault, HashMapComparatorDefault<String, void>>*)
{
	return OK;
}

String ResourceFormatSaverTextInstance::_write_resource(Ref<Resource> const&) { return String(); }

Error ResourceFormatSaverTextInstance::save(String const&, Ref<Resource> const&, unsigned int)
{
	return OK;
}

// --- audio_stream_wav.cpp ---

#include "audio_stream_wav.h"
#include "core/io/file_access_wrappers.h"
#include "core/io/marshalls.h"

const float TRIM_DB_LIMIT = -50;
const int TRIM_FADE_OUT_FRAMES = 500;

void AudioStreamPlaybackWAV::start(double p_from_pos)
{
	if (base->format == AudioStreamWAV::FORMAT_IMA_ADPCM) {
		// no seeking in IMA_ADPCM
		for (int i = 0; i < 2; i++) {
			ima_adpcm[i].step_index = 0;
			ima_adpcm[i].predictor = 0;
			ima_adpcm[i].loop_step_index = 0;
			ima_adpcm[i].loop_predictor = 0;
			ima_adpcm[i].last_nibble = -1;
			ima_adpcm[i].loop_pos = 0x7FFFFFFF;
			ima_adpcm[i].window_ofs = 0;
		}

		offset = 0;
	}
	else {
		seek(p_from_pos);
	}

	sign = 1;
	active = true;
	begin_resample();
}

void AudioStreamPlaybackWAV::stop() { active = false; }

bool AudioStreamPlaybackWAV::is_playing() const { return active; }

int AudioStreamPlaybackWAV::get_loop_count() const { return 0; }

double AudioStreamPlaybackWAV::get_playback_position() const
{
	return double(offset) / base->mix_rate;
}

void AudioStreamPlaybackWAV::seek(double p_time)
{
	if (base->format == AudioStreamWAV::FORMAT_IMA_ADPCM) {
		return; // no seeking in ima-adpcm
	}

	double max = base->get_length();
	if (p_time < 0) {
		p_time = 0;
	}
	else if (p_time >= max) {
		p_time = max - 0.001;
	}

	offset = int64_t(p_time * base->mix_rate);
}

template <typename Depth, bool is_stereo, bool is_ima_adpcm, bool is_qoa>
void AudioStreamPlaybackWAV::decode_samples(const Depth* p_src, AudioFrame* p_dst,
	int64_t& p_offset, int8_t& p_increment, uint32_t p_amount, IMA_ADPCM_State* p_ima_adpcm,
	QOA_State* p_qoa)
{
	// this function will be compiled branchless by any decent compiler

	int32_t final = 0, final_r = 0;
	while (p_amount) {
		p_amount--;
		int64_t pos = p_offset << (is_stereo && !is_ima_adpcm && !is_qoa ? 1 : 0);

		if (is_ima_adpcm) {
			int64_t sample_pos = pos + p_ima_adpcm[0].window_ofs;

			while (sample_pos > p_ima_adpcm[0].last_nibble) {
				static const int16_t _ima_adpcm_step_table[89] = {7, 8, 9, 10, 11, 12, 13, 14, 16,
					17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107,
					118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494,
					544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878,
					2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484,
					7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350,
					22385, 24623, 27086, 29794, 32767};

				static const int8_t _ima_adpcm_index_table[16] = {
					-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8};

				for (int i = 0; i < (is_stereo ? 2 : 1); i++) {
					int16_t nibble, diff, step;

					p_ima_adpcm[i].last_nibble++;

					uint8_t nbb =
						p_src[(p_ima_adpcm[i].last_nibble >> 1) * (is_stereo ? 2 : 1) + i];
					nibble = (p_ima_adpcm[i].last_nibble & 1) ? (nbb >> 4) : (nbb & 0xF);
					step = _ima_adpcm_step_table[p_ima_adpcm[i].step_index];

					p_ima_adpcm[i].step_index += _ima_adpcm_index_table[nibble];
					if (p_ima_adpcm[i].step_index < 0) {
						p_ima_adpcm[i].step_index = 0;
					}
					if (p_ima_adpcm[i].step_index > 88) {
						p_ima_adpcm[i].step_index = 88;
					}

					diff = step >> 3;
					if (nibble & 1) {
						diff += step >> 2;
					}
					if (nibble & 2) {
						diff += step >> 1;
					}
					if (nibble & 4) {
						diff += step;
					}
					if (nibble & 8) {
						diff = -diff;
					}

					p_ima_adpcm[i].predictor += diff;
					if (p_ima_adpcm[i].predictor < -0x8000) {
						p_ima_adpcm[i].predictor = -0x8000;
					}
					else if (p_ima_adpcm[i].predictor > 0x7FFF) {
						p_ima_adpcm[i].predictor = 0x7FFF;
					}

					/* store loop if there */
					if (p_ima_adpcm[i].last_nibble == p_ima_adpcm[i].loop_pos) {
						p_ima_adpcm[i].loop_step_index = p_ima_adpcm[i].step_index;
						p_ima_adpcm[i].loop_predictor = p_ima_adpcm[i].predictor;
					}

					// printf("%i - %i - pred
					// %i\n",int(p_ima_adpcm[i].last_nibble),int(nibble),int(p_ima_adpcm[i].predictor));
				}
			}

			final = p_ima_adpcm[0].predictor;
			if (is_stereo) {
				final_r = p_ima_adpcm[1].predictor;
			}

		}
		else if (is_qoa) {
			uint64_t new_data_ofs = 8 + pos / QOA_FRAME_LEN * p_qoa->frame_len;

			if (p_qoa->data_ofs != new_data_ofs) {
				p_qoa->data_ofs = new_data_ofs;
				const uint8_t* ofs_src = (uint8_t*)p_src + p_qoa->data_ofs;
				qoa_decode_frame(
					ofs_src, p_qoa->frame_len, &p_qoa->desc, p_qoa->dec.ptr(), nullptr);
			}

			uint32_t dec_idx = pos % QOA_FRAME_LEN << (is_stereo ? 1 : 0);

			final = p_qoa->dec[dec_idx];
			if (is_stereo) {
				final_r = p_qoa->dec[dec_idx + 1];
			}

		}
		else {
			final = p_src[pos];
			if (is_stereo) {
				final_r = p_src[pos + 1];
			}
			if constexpr (sizeof(Depth) ==
						  1) { /* conditions will not exist anymore when compiled! */
				final <<= 8;
				if (is_stereo) {
					final_r <<= 8;
				}
			}
		}

		if (!is_stereo) {
			final_r = final; // copy to right channel if stereo
		}

		p_dst->left = final / 32767.0;
		p_dst->right = final_r / 32767.0;
		p_dst++;

		p_offset += p_increment;
	}
}

int AudioStreamPlaybackWAV::_mix_internal(AudioFrame* p_buffer, int p_frames)
{
	if (base->data.is_empty() || !active) {
		for (int i = 0; i < p_frames; i++) {
			p_buffer[i] = AudioFrame(0, 0);
		}
		return 0;
	}

	uint64_t len = base->data.size();
	switch (base->format) {
	case AudioStreamWAV::FORMAT_8_BITS:
		len /= 1;
		break;
	case AudioStreamWAV::FORMAT_16_BITS:
		len /= 2;
		break;
	case AudioStreamWAV::FORMAT_IMA_ADPCM:
		len *= 2;
		break;
	case AudioStreamWAV::FORMAT_QOA:
		len = qoa.desc.samples * qoa.desc.channels;
		break;
	}

	if (base->stereo) {
		len /= 2;
	}

	int64_t loop_begin = base->loop_begin;
	int64_t loop_end = base->loop_end;
	int64_t begin_limit = (base->loop_mode != AudioStreamWAV::LOOP_DISABLED) ? loop_begin : 0;
	int64_t end_limit = (base->loop_mode != AudioStreamWAV::LOOP_DISABLED) ? loop_end : len - 1;
	bool is_stereo = base->stereo;

	int32_t todo = p_frames;

	if (base->loop_mode == AudioStreamWAV::LOOP_BACKWARD) {
		sign = -1;
	}

	int8_t increment = sign;

	// looping

	AudioStreamWAV::LoopMode loop_format = base->loop_mode;
	AudioStreamWAV::Format format = base->format;

	/* audio data */

	const uint8_t* data = base->data.ptr();
	AudioFrame* dst_buff = p_buffer;

	if (format == AudioStreamWAV::FORMAT_IMA_ADPCM) {
		if (loop_format != AudioStreamWAV::LOOP_DISABLED) {
			ima_adpcm[0].loop_pos = loop_begin;
			ima_adpcm[1].loop_pos = loop_begin;
			loop_format = AudioStreamWAV::LOOP_FORWARD;
		}
	}

	while (todo > 0) {
		int64_t limit = 0;
		int32_t target = 0, aux = 0;

		/** LOOP CHECKING **/

		if (increment < 0) {
			/* going backwards */

			if (loop_format != AudioStreamWAV::LOOP_DISABLED && offset < loop_begin) {
				/* loopstart reached */
				if (loop_format == AudioStreamWAV::LOOP_PINGPONG) {
					/* bounce ping pong */
					offset = loop_begin + (loop_begin - offset);
					increment = -increment;
					sign *= -1;
				}
				else {
					/* go to loop-end */
					offset = loop_end - (loop_begin - offset);
				}
			}
			else {
				/* check for sample not reaching beginning */
				if (offset < 0) {
					active = false;
					break;
				}
			}
		}
		else {
			/* going forward */
			if (loop_format != AudioStreamWAV::LOOP_DISABLED && offset >= loop_end) {
				/* loopend reached */

				if (loop_format == AudioStreamWAV::LOOP_PINGPONG) {
					/* bounce ping pong */
					offset = loop_end - (offset - loop_end);
					increment = -increment;
					sign *= -1;
				}
				else {
					/* go to loop-begin */

					if (format == AudioStreamWAV::FORMAT_IMA_ADPCM) {
						for (int i = 0; i < 2; i++) {
							ima_adpcm[i].step_index = ima_adpcm[i].loop_step_index;
							ima_adpcm[i].predictor = ima_adpcm[i].loop_predictor;
							ima_adpcm[i].last_nibble = loop_begin;
						}
						offset = loop_begin;
					}
					else {
						offset = loop_begin + (offset - loop_end);
					}
				}
			}
			else {
				/* no loop, check for end of sample */
				if ((uint64_t)offset >= len) {
					active = false;
					break;
				}
			}
		}

		/** MIXCOUNT COMPUTING **/

		/* next possible limit (looppoints or sample begin/end */
		limit = (increment < 0) ? begin_limit : end_limit;

		/* compute what is shorter, the todo or the limit? */
		aux = (limit - offset) / increment + 1;
		target = (aux < todo) ? aux : todo; /* mix target is the shorter buffer */

		/* check just in case */
		if (target <= 0) {
			active = false;
			break;
		}

		todo -= target;

		switch (base->format) {
		case AudioStreamWAV::FORMAT_8_BITS: {
			if (is_stereo) {
				decode_samples<int8_t, true, false, false>(
					(int8_t*)data, dst_buff, offset, increment, target, ima_adpcm, &qoa);
			}
			else {
				decode_samples<int8_t, false, false, false>(
					(int8_t*)data, dst_buff, offset, increment, target, ima_adpcm, &qoa);
			}
		} break;
		case AudioStreamWAV::FORMAT_16_BITS: {
			if (is_stereo) {
				decode_samples<int16_t, true, false, false>(
					(int16_t*)data, dst_buff, offset, increment, target, ima_adpcm, &qoa);
			}
			else {
				decode_samples<int16_t, false, false, false>(
					(int16_t*)data, dst_buff, offset, increment, target, ima_adpcm, &qoa);
			}

		} break;
		case AudioStreamWAV::FORMAT_IMA_ADPCM: {
			if (is_stereo) {
				decode_samples<int8_t, true, true, false>(
					(int8_t*)data, dst_buff, offset, increment, target, ima_adpcm, &qoa);
			}
			else {
				decode_samples<int8_t, false, true, false>(
					(int8_t*)data, dst_buff, offset, increment, target, ima_adpcm, &qoa);
			}

		} break;
		case AudioStreamWAV::FORMAT_QOA: {
			if (is_stereo) {
				decode_samples<uint8_t, true, false, true>(
					(uint8_t*)data, dst_buff, offset, increment, target, ima_adpcm, &qoa);
			}
			else {
				decode_samples<uint8_t, false, false, true>(
					(uint8_t*)data, dst_buff, offset, increment, target, ima_adpcm, &qoa);
			}
		} break;
		}

		dst_buff += target;
	}

	if (todo) {
		int mixed_frames = p_frames - todo;
		// bit was missing from mix
		int todo_ofs = p_frames - todo;
		for (int i = todo_ofs; i < p_frames; i++) {
			p_buffer[i] = AudioFrame(0, 0);
		}
		return mixed_frames;
	}
	return p_frames;
}

float AudioStreamPlaybackWAV::get_stream_sampling_rate() { return base->mix_rate; }

void AudioStreamPlaybackWAV::tag_used_streams() { base->tag_used(get_playback_position()); }

void AudioStreamPlaybackWAV::set_is_sample(bool p_is_sample) { _is_sample = p_is_sample; }

bool AudioStreamPlaybackWAV::get_is_sample() const { return _is_sample; }

Ref<AudioSamplePlayback> AudioStreamPlaybackWAV::get_sample_playback() const
{
	return sample_playback;
}

void AudioStreamPlaybackWAV::set_sample_playback(const Ref<AudioSamplePlayback>& p_playback)
{
	sample_playback = p_playback;
	if (sample_playback.is_valid()) {
		sample_playback->stream_playback = Ref<AudioStreamPlayback>(this);
	}
}

/////////////////////

void AudioStreamWAV::set_format(Format p_format) { format = p_format; }

AudioStreamWAV::Format AudioStreamWAV::get_format() const { return format; }

void AudioStreamWAV::set_loop_mode(LoopMode p_loop_mode) { loop_mode = p_loop_mode; }

AudioStreamWAV::LoopMode AudioStreamWAV::get_loop_mode() const { return loop_mode; }

void AudioStreamWAV::set_loop_begin(int p_frame) { loop_begin = p_frame; }

int AudioStreamWAV::get_loop_begin() const { return loop_begin; }

void AudioStreamWAV::set_loop_end(int p_frame) { loop_end = p_frame; }

int AudioStreamWAV::get_loop_end() const { return loop_end; }

void AudioStreamWAV::set_mix_rate(int p_hz)
{
	ERR_FAIL_COND(p_hz == 0);
	mix_rate = p_hz;
}

int AudioStreamWAV::get_mix_rate() const { return mix_rate; }

void AudioStreamWAV::set_stereo(bool p_enable) { stereo = p_enable; }

bool AudioStreamWAV::is_stereo() const { return stereo; }

double AudioStreamWAV::get_length() const
{
	uint64_t len = data.size();
	switch (format) {
	case AudioStreamWAV::FORMAT_8_BITS:
		len /= 1;
		break;
	case AudioStreamWAV::FORMAT_16_BITS:
		len /= 2;
		break;
	case AudioStreamWAV::FORMAT_IMA_ADPCM:
		len *= 2;
		break;
	case AudioStreamWAV::FORMAT_QOA:
		qoa_desc desc = {};
		qoa_decode_header(data.ptr(), len, &desc);
		len = desc.samples * desc.channels;
		break;
	}

	if (stereo) {
		len /= 2;
	}

	return double(len) / mix_rate;
}

bool AudioStreamWAV::is_monophonic() const { return false; }

void AudioStreamWAV::set_data(const Vector<uint8_t>& p_data)
{
	AudioServer::lock();

	data = p_data;

	AudioServer::unlock();
}

Vector<uint8_t> AudioStreamWAV::get_data() const { return Vector<uint8_t>(data); }

Error AudioStreamWAV::save_to_wav(const String& p_path)
{
	if (format == AudioStreamWAV::FORMAT_IMA_ADPCM || format == AudioStreamWAV::FORMAT_QOA) {
		WARN_PRINT("Saving IMA_ADPCM and QOA samples is not supported yet");
		return ERR_UNAVAILABLE;
	}

	uint64_t sub_chunk_2_size = data.size(); // Subchunk2Size = Size of data in bytes
	ERR_FAIL_COND_V_MSG(sub_chunk_2_size > UINT32_MAX - 36, ERR_FILE_CANT_WRITE,
		"Data size exceeds maximum WAV file size of 4 GiB.");

	// Format code
	// 1:PCM format (for 8 or 16 bit)
	// 3:IEEE float format
	int format_code = (format == FORMAT_IMA_ADPCM) ? 3 : 1;

	int n_channels = stereo ? 2 : 1;

	long sample_rate = mix_rate;

	int byte_pr_sample = 0;
	switch (format) {
	case AudioStreamWAV::FORMAT_8_BITS:
		byte_pr_sample = 1;
		break;
	case AudioStreamWAV::FORMAT_16_BITS:
	case AudioStreamWAV::FORMAT_QOA:
		byte_pr_sample = 2;
		break;
	case AudioStreamWAV::FORMAT_IMA_ADPCM:
		byte_pr_sample = 4;
		break;
	}

	String file_path = p_path;
	if (file_path.substr(file_path.length() - 4, 4).to_lower() != ".wav") {
		file_path += ".wav";
	}

	Ref<FileAccess> file =
		FileAccess::open(file_path, FileAccess::WRITE); // Overrides existing file if present

	ERR_FAIL_COND_V(file.is_null(), ERR_FILE_CANT_WRITE);

	// Create WAV Header
	file->store_string("RIFF");			   // ChunkID
	file->store_32(sub_chunk_2_size + 36); // ChunkSize = 36 + SubChunk2Size (size of entire file
										   // minus the 8 bytes for this and previous header)
	file->store_string("WAVE");			   // Format
	file->store_string("fmt ");			   // Subchunk1ID
	file->store_32(16);					   // Subchunk1Size = 16
	file->store_16(format_code);		   // AudioFormat
	file->store_16(n_channels);			   // Number of Channels
	file->store_32(sample_rate);		   // SampleRate
	file->store_32(sample_rate * n_channels * byte_pr_sample); // ByteRate
	file->store_16(n_channels * byte_pr_sample); // BlockAlign = NumChannels * BytePrSample
	file->store_16(byte_pr_sample * 8);			 // BitsPerSample
	file->store_string("data");					 // Subchunk2ID
	file->store_32(sub_chunk_2_size);			 // Subchunk2Size

	// Add data
	const uint8_t* read_data = data.ptr();
	switch (format) {
	case AudioStreamWAV::FORMAT_8_BITS:
		for (uint64_t i = 0; i < sub_chunk_2_size; i++) {
			uint8_t data_point = (read_data[i] + 128);
			file->store_8(data_point);
		}
		break;
	case AudioStreamWAV::FORMAT_16_BITS:
	case AudioStreamWAV::FORMAT_QOA:
		for (uint64_t i = 0; i < sub_chunk_2_size / 2; i++) {
			uint16_t data_point = decode_uint16(&read_data[i * 2]);
			file->store_16(data_point);
		}
		break;
	case AudioStreamWAV::FORMAT_IMA_ADPCM:
		// Unimplemented
		break;
	}

	return OK;
}

Ref<AudioStreamPlayback> AudioStreamWAV::instantiate_playback()
{
	Ref<AudioStreamPlaybackWAV> sample;
	sample.instantiate();
	sample->base = Ref<AudioStreamWAV>(this);

	if (format == AudioStreamWAV::FORMAT_QOA) {
		uint32_t ffp = qoa_decode_header(data.ptr(), data.size(), &sample->qoa.desc);
		ERR_FAIL_COND_V(ffp != 8, Ref<AudioStreamPlaybackWAV>());
		sample->qoa.frame_len = qoa_max_frame_size(&sample->qoa.desc);
		uint32_t samples_len = MIN(sample->qoa.desc.samples + 1, (uint32_t)QOA_FRAME_LEN);
		uint32_t dec_len = sample->qoa.desc.channels * samples_len;
		sample->qoa.dec.resize(dec_len);
	}

	return sample;
}

Ref<AudioSample> AudioStreamWAV::generate_sample() const
{
	Ref<AudioSample> as = memnew(AudioSample);
	return as;
}

// --- video_stream.cpp ---

#include "video_stream.h"

#include "modules/modules_enabled.gen.h"

void VideoStreamPlayback::set_mix_callback(AudioMixCallback p_callback, void* p_userdata)
{
	mix_callback = p_callback;
	mix_udata = p_userdata;
}

int VideoStreamPlayback::mix_audio(int num_frames, PackedFloat32Array buffer, int offset)
{
	if (num_frames <= 0) {
		return 0;
	}
	if (!mix_callback) {
		return -1;
	}
	ERR_FAIL_INDEX_V(offset, buffer.size(), -1);
	ERR_FAIL_INDEX_V(
		(_channel_count < 1 ? 1 : _channel_count) * num_frames - 1, buffer.size() - offset, -1);
	return mix_callback(mix_udata, buffer.ptr() + offset, num_frames);
}

void VideoStream::set_file(const String& p_file)
{
	file = p_file;
	emit_changed();
}

String VideoStream::get_file() { return file; }

VideoStream::VideoStream() {}

VideoStream::~VideoStream() {}

void VideoStream::set_audio_track(int p_track) { audio_track = p_track; }

#ifndef MODULE_THEORA_ENABLED

VideoStreamPlayback::VideoStreamPlayback() {}

VideoStreamPlayback::~VideoStreamPlayback() {}

Ref<VideoStreamPlayback> VideoStream::instantiate_playback()
{
	ERR_FAIL_V_MSG(Ref<VideoStreamPlayback>(), "Theora module is disabled.");
}

void VideoStreamPlayback::stop() {}

void VideoStreamPlayback::play() {}

bool VideoStreamPlayback::is_playing() const { return false; }

void VideoStreamPlayback::set_paused(bool p_paused) {}

bool VideoStreamPlayback::is_paused() const { return false; }

double VideoStreamPlayback::get_length() const { return 0.0; }

double VideoStreamPlayback::get_playback_position() const { return 0.0; }

void VideoStreamPlayback::seek(double p_time) {}

void VideoStreamPlayback::set_file(const String& p_file) {}

void VideoStreamPlayback::set_audio_track(int p_idx) {}

Ref<Texture2D> VideoStreamPlayback::get_texture() const { return Ref<Texture2D>(); }

void VideoStreamPlayback::update(double p_delta) {}

int VideoStreamPlayback::get_channels() const { return 0; }

int VideoStreamPlayback::get_mix_rate() const { return 0; }

#endif // MODULE_THEORA_ENABLED
