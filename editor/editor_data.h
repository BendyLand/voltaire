/**************************************************************************/
/*  editor_data.h                                                         */
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

#pragma once

#include "core/templates/list.h"
#include "core/templates/mem_unique_ptr.h"
#include "scene/main/node.h"
#include "scene/resources/texture.h"

class ConfigFile;
class EditorPlugin;
class EditorUndoRedoManager;
class PopupMenu;

/**
 * Stores the history of objects which have been selected for editing in the Editor & the Inspector.
 *
 * Used in the editor to set & access the currently edited object, as well as the history of objects
 * which have been edited.
 */
class EditorSelectionHistory
{
	// Stores the object & property (if relevant).
	struct _Object
	{
		Ref<RefCounted> ref;
		String property;
		bool inspector_only = false;
	};

	// Represents the selection of an object for editing.
	struct HistoryElement
	{
		// The sub-resources of the parent object (first in the path) that have been edited.
		// For example, Node2D -> nested resource -> nested resource, if edited each individually in
		// their own inspector.
		Vector<_Object> path;
		// The current point in the path. This is always equal to the last item in the path - it is
		// never decremented.
		int level = 0;
	};
	friend class EditorData;

	Vector<HistoryElement> history;
	int current_elem_idx; // The current history element being edited.

public:
	void cleanup_history();

	bool is_at_beginning() const;
	bool is_at_end() const;

	// Adds an object to the selection history. A property name can be passed if the target is a
	// subresource of the given object. If the object should not change the main screen plugin, it
	// can be set as inspector only.

	int get_history_len();
	int get_history_pos();

	bool next();
	bool previous();
	bool is_current_inspector_only() const;

	// Gets the size of the path of the current history item.
	int get_path_size() const;
	// Gets the property of the current history item.
	String get_path_property(int p_index) const;

	void clear();

	EditorSelectionHistory();
};

class EditorSelection;

class EditorData
{
public:
	struct CustomType
	{
		String name;
		Ref<Texture2D> icon;
	};

	struct EditedScene
	{
		Node* root = nullptr;
		String path;
		uint64_t file_modified_time = 0;
		List<Node*> selection;
		Vector<EditorSelectionHistory::HistoryElement> history_stored;
		int history_current = 0;
		NodePath live_edit_root;
		int history_id = 0;
		uint64_t last_checked_version = 0;
		uint64_t time_opened = 0;
	};

private:
	Vector<EditorPlugin*> editor_plugins;
	HashMap<StringName, EditorPlugin*> extension_editor_plugins;

	struct PropertyData
	{
		String name;
	};

	HashMap<String, Vector<CustomType>> custom_types;

	List<PropertyData> clipboard;
	EditorUndoRedoManager* undo_redo_manager;

	Vector<EditedScene> edited_scene;
	int current_edited_scene = -1;
	int last_created_scene = 1;

	bool _find_updated_instances(Node* p_root, Node* p_node, HashSet<String>& checked_paths);

	HashMap<StringName, String> _script_class_icon_paths;
	HashMap<String, StringName> _script_class_file_to_path;
	HashMap<String, Ref<Texture2D>> _script_icon_cache;

	Ref<Texture2D> _load_script_icon(const String& p_path) const;

public:
	EditorPlugin* get_editor_by_name(const String& p_name);

	void get_editor_breakpoints(List<String>* p_breakpoints);
	void clear_editor_states();
	void save_editor_external_data();
	void apply_changes_in_editors();

	void add_editor_plugin(EditorPlugin* p_plugin);
	void remove_editor_plugin(EditorPlugin* p_plugin);

	int get_editor_plugin_count() const;
	EditorPlugin* get_editor_plugin(int p_idx);

	void add_extension_editor_plugin(const StringName& p_class_name, EditorPlugin* p_plugin);
	void remove_extension_editor_plugin(const StringName& p_class_name);
	bool has_extension_editor_plugin(const StringName& p_class_name);
	EditorPlugin* get_extension_editor_plugin(const StringName& p_class_name);

	void remove_move_array_element_function(const StringName& p_class);

	void remove_custom_type(const String& p_type);

	const HashMap<String, Vector<CustomType>>& get_custom_types() const { return custom_types; }

	const CustomType* get_custom_type_by_name(const String& p_name) const;
	const CustomType* get_custom_type_by_path(const String& p_path) const;
	bool is_type_recognized(const String& p_type) const;

	int add_edited_scene(int p_at_pos);
	void remove_scene(int p_idx);
	void set_scene_root(int p_idx, Node* p_root);
	void set_edited_scene(int p_idx);
	void set_edited_scene_root(Node* p_root);
	int get_edited_scene() const;
	int get_edited_scene_from_path(const String& p_path) const;
	Node* get_edited_scene_root(int p_idx = -1);
	int get_edited_scene_count() const;
	Vector<EditedScene> get_edited_scenes() const;

	String get_scene_title(int p_idx, bool p_always_strip_extension = false) const;
	String get_scene_path(int p_idx) const;
	String get_scene_type(int p_idx) const;
	void set_scene_path(int p_idx, const String& p_path);
	uint64_t get_scene_time_opened(int p_idx) const;
	void set_scene_modified_time(int p_idx, uint64_t p_time);
	uint64_t get_scene_modified_time(int p_idx) const;
	void clear_edited_scenes();
	void set_edited_scene_live_edit_root(const NodePath& p_root);
	NodePath get_edited_scene_live_edit_root();
	bool check_and_update_scene(int p_idx);
	bool reload_scene_from_memory(int p_idx, bool p_mark_unsaved);
	void move_scene_to_index(int p_idx, int p_to_idx);
	void move_edited_scene_to_index(int p_idx);

	bool call_build();

	void set_scene_as_saved(int p_idx);
	bool is_scene_changed(int p_idx);

	int get_scene_history_id_from_path(const String& p_path) const;
	int get_current_edited_scene_history_id() const;
	int get_scene_history_id(int p_idx) const;

	void set_plugin_window_layout(Ref<ConfigFile> p_layout);
	void get_plugin_window_layout(Ref<ConfigFile> p_layout);

	void notify_edited_scene_changed();
	void notify_resource_saved(const Ref<Resource>& p_resource);
	void notify_scene_saved(const String& p_path);
	void load_editor_plugin_states_from_config(const Ref<ConfigFile>& p_config_file, int p_idx);

	bool script_class_is_parent(const String& p_class, const String& p_inherits);

	StringName script_class_get_name(const String& p_path) const;
	void script_class_set_name(const String& p_path, const StringName& p_class);

	String script_class_get_icon_path(const String& p_class, bool* r_valid = nullptr) const;
	void script_class_set_icon_path(const String& p_class, const String& p_icon_path);

	void script_class_clear_icon_paths() { _script_class_icon_paths.clear(); }

	void script_class_save_global_classes();
	void script_class_load_icon_paths();

	Ref<Texture2D> extension_class_get_icon(const String& p_class) const;

	Ref<Texture2D> get_script_icon(const String& p_script_path);
	void clear_script_icon_cache();

	EditorData();
	~EditorData();
};

/**
 * Stores and provides access to the nodes currently selected in the editor.
 *
 * This provides a central location for storing "selected" nodes, as a selection can be triggered
 * from multiple places, such as the SceneTreeDock or a main screen editor plugin (e.g.
 * CanvasItemEditor).
 */
class EditorSelection
{
	// Tracks whether the selection change signal has been emitted.
	// Prevents multiple signals being called in one frame.
	bool emitted = false;

	bool changed = false;
	bool node_list_changed = false;

	void _node_removed(Node* p_node);

	void _update_node_list();
	void _emit_change();

protected:
	static void _bind_methods();

public:
	void add_node(Node* p_node);
	void remove_node(Node* p_node);
	bool is_selected(Node* p_node) const;

	void update(bool p_deferred = true);
	void clear();

	// Returns only the top level selected nodes.
	// That is, if the selection includes some node and a child of that node, only the parent is
	// returned.
	List<Node*> get_top_selected_node_list();
	// Returns all the selected nodes (list version of "get_selected_nodes").
	List<Node*> get_full_selected_node_list();

	~EditorSelection();
};


