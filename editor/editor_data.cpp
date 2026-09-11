/**************************************************************************/
/*  editor_data.cpp                                                       */
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
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/os/time.h"
#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/inspector/editor_context_menu_plugin.h"
#include "editor/inspector/multi_node_edit.h"
#include "editor/plugins/editor_plugin.h"
#include "editor_data.h"
#include "scene/main/scene_tree.h"
#include "scene/property_utils.h"
#include "scene/resources/packed_scene.h"

int EditorSelectionHistory::get_history_len() { return history.size(); }

int EditorSelectionHistory::get_history_pos() { return current_elem_idx; }

bool EditorSelectionHistory::is_at_beginning() const { return current_elem_idx <= 0; }

bool EditorSelectionHistory::is_at_end() const
{
	return ((current_elem_idx + 1) >= history.size());
}

bool EditorSelectionHistory::next()
{
	cleanup_history();

	if ((current_elem_idx + 1) < history.size()) {
		current_elem_idx++;
	}
	else {
		return false;
	}

	return true;
}

bool EditorSelectionHistory::previous()
{
	cleanup_history();

	if (current_elem_idx > 0) {
		current_elem_idx--;
	}
	else {
		return false;
	}

	return true;
}

bool EditorSelectionHistory::is_current_inspector_only() const
{
	if (current_elem_idx < 0 || current_elem_idx >= history.size()) {
		return false;
	}

	const HistoryElement& h = history[current_elem_idx];
	return h.path[h.level].inspector_only;
}

int EditorSelectionHistory::get_path_size() const
{
	if (current_elem_idx < 0 || current_elem_idx >= history.size()) {
		return 0;
	}

	return history[current_elem_idx].path.size();
}

String EditorSelectionHistory::get_path_property(int p_index) const
{
	if (current_elem_idx < 0 || current_elem_idx >= history.size()) {
		return "";
	}

	ERR_FAIL_INDEX_V(p_index, history[current_elem_idx].path.size(), "");
	return history[current_elem_idx].path[p_index].property;
}

void EditorSelectionHistory::clear()
{
	history.clear();
	current_elem_idx = -1;
}

EditorSelectionHistory::EditorSelectionHistory() { current_elem_idx = -1; }

////////////////////////////////////////////////////////////

EditorPlugin* EditorData::get_editor_by_name(const String& p_name)
{
	for (int i = editor_plugins.size() - 1; i > -1; i--) {
		if (editor_plugins[i]->get_plugin_name() == p_name) {
			return editor_plugins[i];
		}
	}

	return nullptr;
}

void EditorData::get_editor_breakpoints(List<String>* p_breakpoints)
{
	for (int i = 0; i < editor_plugins.size(); i++) {
		editor_plugins[i]->get_breakpoints(p_breakpoints);
	}
}

void EditorData::notify_edited_scene_changed()
{
	for (int i = 0; i < editor_plugins.size(); i++) {
		editor_plugins[i]->edited_scene_changed();
		editor_plugins[i]->notify_scene_changed(get_edited_scene_root());
	}
}

void EditorData::notify_resource_saved(const Ref<Resource>& p_resource)
{
	for (int i = 0; i < editor_plugins.size(); i++) {
		editor_plugins[i]->notify_resource_saved(p_resource);
	}
}

void EditorData::notify_scene_saved(const String& p_path)
{
	for (int i = 0; i < editor_plugins.size(); i++) {
		editor_plugins[i]->notify_scene_saved(p_path);
	}
}

void EditorData::save_editor_external_data()
{
	for (int i = 0; i < editor_plugins.size(); i++) {
		editor_plugins[i]->save_external_data();
	}
}

void EditorData::apply_changes_in_editors()
{
	for (int i = 0; i < editor_plugins.size(); i++) {
		editor_plugins[i]->apply_changes();
	}
}

bool EditorData::call_build()
{
	bool result = true;

	for (int i = 0; i < editor_plugins.size() && result; i++) {
		result &= editor_plugins[i]->build();
	}

	return result;
}

void EditorData::set_scene_as_saved(int p_idx)
{
	if (p_idx == -1) {
		p_idx = current_edited_scene;
	}
	ERR_FAIL_INDEX(p_idx, edited_scene.size());

	undo_redo_manager->set_history_as_saved(edited_scene[p_idx].history_id);
}

int EditorData::get_scene_history_id_from_path(const String& p_path) const
{
	for (const EditedScene& E : edited_scene) {
		if (E.path == p_path) {
			return E.history_id;
		}
	}
	return 0;
}

int EditorData::get_current_edited_scene_history_id() const
{
	if (current_edited_scene != -1) {
		return edited_scene[current_edited_scene].history_id;
	}
	return 0;
}

int EditorData::get_scene_history_id(int p_idx) const { return edited_scene[p_idx].history_id; }

void EditorData::remove_editor_plugin(EditorPlugin* p_plugin) { editor_plugins.erase(p_plugin); }

void EditorData::add_editor_plugin(EditorPlugin* p_plugin) { editor_plugins.push_back(p_plugin); }

int EditorData::get_editor_plugin_count() const { return editor_plugins.size(); }

EditorPlugin* EditorData::get_editor_plugin(int p_idx)
{
	ERR_FAIL_INDEX_V(p_idx, editor_plugins.size(), nullptr);
	return editor_plugins[p_idx];
}

void EditorData::add_extension_editor_plugin(const StringName& p_class_name, EditorPlugin* p_plugin)
{
	ERR_FAIL_COND(extension_editor_plugins.has(p_class_name));
	extension_editor_plugins.insert(p_class_name, p_plugin);
}

void EditorData::remove_extension_editor_plugin(const StringName& p_class_name)
{
	extension_editor_plugins.erase(p_class_name);
}

bool EditorData::has_extension_editor_plugin(const StringName& p_class_name)
{
	return extension_editor_plugins.has(p_class_name);
}

EditorPlugin* EditorData::get_extension_editor_plugin(const StringName& p_class_name)
{
	EditorPlugin** plugin = extension_editor_plugins.getptr(p_class_name);
	return plugin == nullptr ? nullptr : *plugin;
}

const EditorData::CustomType* EditorData::get_custom_type_by_name(const String& p_type) const
{
	for (const KeyValue<String, Vector<CustomType>>& E : custom_types) {
		for (const CustomType& F : E.value) {
			if (F.name == p_type) {
				return &F;
			}
		}
	}
	return nullptr;
}

void EditorData::remove_custom_type(const String& p_type)
{
	for (KeyValue<String, Vector<CustomType>>& E : custom_types) {
		for (int i = 0; i < E.value.size(); i++) {
			if (E.value[i].name == p_type) {
				E.value.remove_at(i);
				if (E.value.is_empty()) {
					custom_types.erase(E.key);
				}
				return;
			}
		}
	}
}

int EditorData::add_edited_scene(int p_at_pos)
{
	if (p_at_pos < 0) {
		p_at_pos = edited_scene.size();
	}
	EditedScene es;
	es.root = nullptr;
	es.path = String();
	es.file_modified_time = 0;
	es.history_current = -1;
	es.live_edit_root = NodePath(String("/root"));
	es.history_id = last_created_scene++;
	es.time_opened = Time::get_singleton()->get_unix_time_from_system();

	if (p_at_pos == edited_scene.size()) {
		edited_scene.push_back(es);
	}
	else {
		edited_scene.insert(p_at_pos, es);
	}

	if (current_edited_scene < 0) {
		current_edited_scene = 0;
	}
	return p_at_pos;
}

void EditorData::set_scene_root(int p_idx, Node* p_root)
{
	ERR_FAIL_INDEX(p_idx, edited_scene.size());
	EditedScene& scene_info = edited_scene.write[p_idx];

	scene_info.root = p_root;
	if (p_root) {
		if (p_root->is_instance()) {
			scene_info.path = p_root->get_scene_file_path();
		}
		else {
			p_root->set_scene_file_path(scene_info.path);
		}
	}

	if (!scene_info.path.is_empty()) {
		scene_info.file_modified_time = FileAccess::get_modified_time(scene_info.path);
	}
}

bool EditorData::_find_updated_instances(Node* p_root, Node* p_node, HashSet<String>& checked_paths)
{
	Ref<SceneState> ss;

	if (p_node == p_root) {
		ss = p_node->get_scene_inherited_state();
	}
	else if (p_node->is_instance()) {
		ss = p_node->get_scene_instance_state();
	}

	if (ss.is_valid()) {
		String path = ss->get_path();

		if (!checked_paths.has(path)) {
			uint64_t modified_time = FileAccess::get_modified_time(path);
			if (modified_time != ss->get_last_modified_time()) {
				return true; // external scene changed
			}

			checked_paths.insert(path);
		}
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		bool found = _find_updated_instances(p_root, p_node->get_child(i), checked_paths);
		if (found) {
			return true;
		}
	}

	return false;
}

bool EditorData::check_and_update_scene(int p_idx)
{
	ERR_FAIL_INDEX_V(p_idx, edited_scene.size(), false);
	if (!edited_scene[p_idx].root) {
		return false;
	}

	HashSet<String> checked_scenes;

	bool must_reload =
		_find_updated_instances(edited_scene[p_idx].root, edited_scene[p_idx].root, checked_scenes);

	if (must_reload) {
		reload_scene_from_memory(p_idx, false);

		return true;
	}

	return false;
}

bool EditorData::reload_scene_from_memory(int p_idx, bool p_mark_unsaved)
{
	ERR_FAIL_INDEX_V(p_idx, edited_scene.size(), false);
	if (!edited_scene[p_idx].root) {
		return false;
	}

	Ref<PackedScene> pscene;
	pscene.instantiate();

	EditorProgress ep("update_scene", TTR("Updating Scene"), 2);
	ep.step(TTR("Storing local changes..."), 0);
	// Pack first, so it stores diffs to previous version of saved scene.
	Error err = pscene->pack(edited_scene[p_idx].root);
	ERR_FAIL_COND_V(err != OK, false);
	ep.step(TTR("Updating scene..."), 1);
	Node* new_scene = pscene->instantiate(PackedScene::GEN_EDIT_STATE_MAIN);
	ERR_FAIL_NULL_V(new_scene, false);

	// Transfer selection.
	List<Node*> new_selection;
	for (const Node* E : edited_scene.write[p_idx].selection) {
		NodePath p = edited_scene[p_idx].root->get_path_to(E);
		Node* new_node = new_scene->get_node(p);
		if (new_node) {
			new_selection.push_back(new_node);
		}
	}

	new_scene->set_scene_file_path(edited_scene[p_idx].root->get_scene_file_path());
	Node* old_root = edited_scene[p_idx].root;
	EditorNode::get_singleton()->set_edited_scene(new_scene);
	memdelete(old_root);
	edited_scene.write[p_idx].selection = new_selection;

	if (p_mark_unsaved) {
		EditorUndoRedoManager::get_singleton()->clear_history(get_scene_history_id(p_idx));
	}
	return true;
}

void EditorData::move_scene_to_index(int p_idx, int p_to_idx)
{
	ERR_FAIL_INDEX(p_idx, edited_scene.size());
	ERR_FAIL_INDEX(p_to_idx, edited_scene.size());

	EditedScene es = edited_scene[p_idx];
	edited_scene.remove_at(p_idx);
	edited_scene.insert(p_to_idx, es);
}

int EditorData::get_edited_scene() const { return current_edited_scene; }

int EditorData::get_edited_scene_from_path(const String& p_path) const
{
	for (int i = 0; i < edited_scene.size(); i++) {
		if (edited_scene[i].path == p_path) {
			return i;
		}
	}

	return -1;
}

void EditorData::set_edited_scene(int p_idx)
{
	ERR_FAIL_INDEX(p_idx, edited_scene.size());
	current_edited_scene = p_idx;
}

Node* EditorData::get_edited_scene_root(int p_idx)
{
	if (p_idx < 0) {
		ERR_FAIL_INDEX_V(current_edited_scene, edited_scene.size(), nullptr);
		return edited_scene[current_edited_scene].root;
	}
	else {
		ERR_FAIL_INDEX_V(p_idx, edited_scene.size(), nullptr);
		return edited_scene[p_idx].root;
	}
}

void EditorData::set_edited_scene_root(Node* p_root)
{
	set_scene_root(current_edited_scene, p_root);
}

int EditorData::get_edited_scene_count() const { return edited_scene.size(); }

Vector<EditorData::EditedScene> EditorData::get_edited_scenes() const
{
	Vector<EditedScene> out_edited_scenes_list = Vector<EditedScene>();

	for (int i = 0; i < edited_scene.size(); i++) {
		out_edited_scenes_list.push_back(edited_scene[i]);
	}

	return out_edited_scenes_list;
}

uint64_t EditorData::get_scene_time_opened(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, edited_scene.size(), 0);
	return edited_scene[p_idx].time_opened;
}

void EditorData::set_scene_modified_time(int p_idx, uint64_t p_time)
{
	if (p_idx == -1) {
		p_idx = current_edited_scene;
	}
	ERR_FAIL_INDEX(p_idx, edited_scene.size());

	edited_scene.write[p_idx].file_modified_time = p_time;
}

uint64_t EditorData::get_scene_modified_time(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, edited_scene.size(), 0);
	return edited_scene[p_idx].file_modified_time;
}

void EditorData::move_edited_scene_to_index(int p_idx)
{
	ERR_FAIL_INDEX(current_edited_scene, edited_scene.size());
	ERR_FAIL_INDEX(p_idx, edited_scene.size());

	move_scene_to_index(current_edited_scene, p_idx);
	current_edited_scene = p_idx;
}

String EditorData::get_scene_title(int p_idx, bool p_always_strip_extension) const
{
	ERR_FAIL_INDEX_V(p_idx, edited_scene.size(), String());
	if (!edited_scene[p_idx].root) {
		return TTR("[empty]");
	}
	if (edited_scene[p_idx].root->get_scene_file_path().is_empty()) {
		return TTR("[unsaved]");
	}

	const String filename = edited_scene[p_idx].root->get_scene_file_path().get_file();
	const String basename = filename.get_basename();

	if (p_always_strip_extension) {
		return basename;
	}

	// Return the filename including the extension if there's ambiguity (e.g. both `foo.tscn` and
	// `foo.scn` are being edited).
	for (int i = 0; i < edited_scene.size(); i++) {
		if (i == p_idx) {
			// Don't compare the edited scene against itself.
			continue;
		}

		if (edited_scene[i].root &&
			basename == edited_scene[i].root->get_scene_file_path().get_file().get_basename()) {
			return filename;
		}
	}

	// Else, return just the basename as there's no ambiguity.
	return basename;
}

void EditorData::set_scene_path(int p_idx, const String& p_path)
{
	ERR_FAIL_INDEX(p_idx, edited_scene.size());
	edited_scene.write[p_idx].path = p_path;

	if (!edited_scene[p_idx].root) {
		return;
	}
	edited_scene[p_idx].root->set_scene_file_path(p_path);
}

String EditorData::get_scene_path(int p_idx) const
{
	ERR_FAIL_INDEX_V(p_idx, edited_scene.size(), String());

	if (edited_scene[p_idx].root) {
		if (edited_scene[p_idx].root->get_scene_file_path().is_empty() &&
			!edited_scene[p_idx].path.is_empty()) {
			edited_scene[p_idx].root->set_scene_file_path(edited_scene[p_idx].path);
		}
		else {
			return edited_scene[p_idx].root->get_scene_file_path();
		}
	}

	return edited_scene[p_idx].path;
}

void EditorData::set_edited_scene_live_edit_root(const NodePath& p_root)
{
	ERR_FAIL_INDEX(current_edited_scene, edited_scene.size());

	edited_scene.write[current_edited_scene].live_edit_root = p_root;
}

NodePath EditorData::get_edited_scene_live_edit_root()
{
	ERR_FAIL_INDEX_V(current_edited_scene, edited_scene.size(), String());

	return edited_scene[current_edited_scene].live_edit_root;
}

void EditorData::clear_edited_scenes()
{
	for (int i = 0; i < edited_scene.size(); i++) {
		memdelete(edited_scene[i].root);
	}
	edited_scene.clear();
	SceneTree::get_singleton()->set_edited_scene_root(nullptr);
}

void EditorData::set_plugin_window_layout
(Ref<ConfigFile> p_layout)
{
	for (int i = 0; i < editor_plugins.size(); i++) {
		editor_plugins[i]->set_window_layout(p_layout);
	}
}

void EditorData::get_plugin_window_layout(Ref<ConfigFile> p_layout)
{
	for (int i = 0; i < editor_plugins.size(); i++) {
		editor_plugins[i]->get_window_layout(p_layout);
	}
}

void EditorData::script_class_set_icon_path(const String& p_class, const String& p_icon_path)
{
	_script_class_icon_paths[p_class] = p_icon_path;
}

StringName EditorData::script_class_get_name(const String& p_path) const
{
	return _script_class_file_to_path.has(p_path) ? _script_class_file_to_path[p_path]
												  : StringName();
}

void EditorData::script_class_set_name(const String& p_path, const StringName& p_class)
{
	_script_class_file_to_path[p_path] = p_class;
}

Ref<Texture2D> EditorData::_load_script_icon(const String& p_path) const
{
	if (!p_path.is_empty() && ResourceLoader::exists(p_path)) {
		Ref<Texture2D> icon = ResourceLoader::load(p_path);
		if (icon.is_valid()) {
			return icon;
		}
	}
	return nullptr;
}

void EditorData::clear_script_icon_cache() { _script_icon_cache.clear(); }

EditorData::EditorData()
{
	undo_redo_manager = memnew(EditorUndoRedoManager);
	script_class_load_icon_paths();
}

EditorData::~EditorData() { memdelete(undo_redo_manager); }

///////////////////////////////////////////////////////////////////////////////

Ref<Texture2D> EditorData::extension_class_get_icon(const String& p_class) const
{
	return Ref<Texture2D>();
}

EditorSelection::~EditorSelection() { clear(); }


