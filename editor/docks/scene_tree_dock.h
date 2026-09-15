/**************************************************************************/
/*  scene_tree_dock.h                                                     */
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

#include "editor/docks/editor_dock.h"
#include "editor/scene/scene_tree_editor.h"
#include "editor/script/script_create_dialog.h"
#include "scene/resources/animation.h"

class CheckBox;
class EditorData;
class EditorSelection;
class MenuButton;
class PanelContainer;
class RenameDialog;
class ReparentDialog;
class Shader;
class ShaderCreateDialog;
class ShaderMaterial;
class TextureRect;
class VBoxContainer;

class SceneTreeDock : public EditorDock
{
	enum Tool
	{
		TOOL_NEW,
		TOOL_INSTANTIATE,
		TOOL_EXPAND_COLLAPSE,
		TOOL_CUT,
		TOOL_COPY,
		TOOL_PASTE,
		TOOL_PASTE_AS_SIBLING,
		TOOL_PASTE_AS_REPLACEMENT,
		TOOL_RENAME,
		TOOL_BATCH_RENAME,
		TOOL_CHANGE_TYPE,
		TOOL_EXTEND_SCRIPT,
		TOOL_ATTACH_SCRIPT,
		TOOL_DETACH_SCRIPT,
		TOOL_MOVE_UP,
		TOOL_MOVE_DOWN,
		TOOL_DUPLICATE,
		TOOL_REPARENT,
		TOOL_REPARENT_TO_NEW_NODE,
		TOOL_MAKE_ROOT,
		TOOL_NEW_SCENE_FROM,
		TOOL_MULTI_EDIT,
		TOOL_ERASE,
		TOOL_COPY_NODE_PATH,
		TOOL_SHOW_IN_FILE_SYSTEM,
		TOOL_OPEN_DOCUMENTATION,
		TOOL_AUTO_EXPAND,
		TOOL_SCENE_EDITABLE_CHILDREN,
		TOOL_SCENE_USE_PLACEHOLDER,
		TOOL_SCENE_MAKE_LOCAL,
		TOOL_SCENE_OPEN,
		TOOL_SCENE_CLEAR_INHERITANCE,
		TOOL_SCENE_CLEAR_INHERITANCE_CONFIRM,
		TOOL_SCENE_OPEN_INHERITED,
		TOOL_TOGGLE_SCENE_UNIQUE_NAME,
		TOOL_CREATE_2D_SCENE,
		TOOL_CREATE_3D_SCENE,
		TOOL_CREATE_USER_INTERFACE,
		TOOL_CREATE_FAVORITE,
		TOOL_CENTER_PARENT,
		TOOL_HIDE_FILTERED_OUT_PARENTS,
		TOOL_ACCESSIBILITY_WARNINGS,
	};

	enum
	{
		EDIT_SUBRESOURCE_BASE = 100
	};

	bool reset_create_dialog = false;

	int current_option = 0;

	MarginContainer* main_mc = nullptr;

	CreateDialog* create_dialog = nullptr;
	RenameDialog* rename_dialog = nullptr;

	Button* button_add = nullptr;
	Button* button_instance = nullptr;
	Button* button_create_script = nullptr;
	Button* button_detach_script = nullptr;
	Button* button_extend_script = nullptr;
	MenuButton* button_tree_menu = nullptr;

	Button* node_shortcuts_toggle = nullptr;
	VBoxContainer* beginner_node_shortcuts = nullptr;
	VBoxContainer* favorite_node_shortcuts = nullptr;

	Button* button_2d = nullptr;
	Button* button_3d = nullptr;
	Button* button_ui = nullptr;
	Button* button_custom = nullptr;
	Button* button_clipboard = nullptr;

	PanelContainer* button_panel = nullptr;
	Button *edit_local, *edit_remote;
	SceneTreeEditor* scene_tree = nullptr;
	Tree* remote_tree = nullptr;

	Node* property_drop_node = nullptr;
	String resource_drop_path;

	EditorData* editor_data = nullptr;
	EditorSelection* editor_selection = nullptr;
	bool update_script_button_queued = false;

	List<Node*> node_clipboard;
	HashSet<Node*> node_clipboard_edited_scene_owned;
	String clipboard_source_scene;
	HashMap<String, HashMap<Node*, HashMap<Ref<Resource>, Ref<Resource>>>> clipboard_resource_remap;

	ScriptCreateDialog* script_create_dialog = nullptr;
	ShaderCreateDialog* shader_create_dialog = nullptr;
	AcceptDialog* accept = nullptr;
	ConfirmationDialog* delete_dialog = nullptr;
	Label* delete_dialog_label = nullptr;
	CheckBox* delete_tracks_checkbox = nullptr;
	ConfirmationDialog* editable_instance_remove_dialog = nullptr;
	ConfirmationDialog* placeholder_editable_instance_remove_dialog = nullptr;

	ReparentDialog* reparent_dialog = nullptr;
	EditorFileDialog* new_scene_from_dialog = nullptr;

	enum FilterMenuItems
	{
		FILTER_BY_TYPE = 64, // Used in the same menus as the Tool enum.
		FILTER_BY_GROUP,
	};

	LineEdit* filter = nullptr;
	PopupMenu* filter_quick_menu = nullptr;

	PopupMenu* menu = nullptr;
	PopupMenu* menu_subresources = nullptr;
	PopupMenu* menu_properties = nullptr;
	ConfirmationDialog* clear_inherit_confirm = nullptr;

	bool first_enter = true;

	Node* scene_root = nullptr;
	Node* edited_scene = nullptr;
	Node* pending_click_select = nullptr;
	bool tree_clicked = false;

	VBoxContainer* create_root_dialog = nullptr;
	String selected_favorite_root;

	Ref<ShaderMaterial> selected_shader_material;

	enum ReplaceOwnerMode
	{
		MODE_BIDI,
		MODE_DO,
		MODE_UNDO
	};

	bool _cyclical_dependency_exists(const String& p_target_scene_path, Node* p_desired_node);
	bool _track_inherit(const String& p_target_scene_path, Node* p_desired_node);

	void _reset_hovering_timer();
	Timer* inspect_hovered_node_delay = nullptr;
	TreeItem* tree_item_inspected = nullptr;
	Node* node_hovered_now = nullptr;
	Node* node_hovered_previously = nullptr;
	bool scene_tree_drag_active = false;

	void _scene_tree_gui_input(Ref<InputEvent> p_event);

	void _set_node_owner_recursive(
		Node* p_node, Node* p_owner, const HashMap<const Node*, Node*>& p_inverse_duplimap);

	void _fill_path_renames(Vector<StringName> base_path, Vector<StringName> new_base_path,
		Node* p_node, HashMap<Node*, NodePath>* p_renames);

	void _normalize_drop(Node*& to_node, int& to_pos, int p_type);

	void _filter_option_selected(int option);
	void _append_filter_options_to(PopupMenu* p_menu);

	void _clear_clipboard();

	bool profile_allow_editing = true;
	bool profile_allow_script_editing = true;
	bool determine_path_automatically = true;

	bool _update_node_path(
		Node* p_root_node, NodePath& r_node_path, HashMap<Node*, NodePath>* p_renames) const;

private:
	static SceneTreeDock* singleton;

public:
	static SceneTreeDock* get_singleton() { return singleton; }

	String get_filter();
	void set_filter(const String& p_filter);

	void set_selected(Node* p_node, bool p_emit_selected = false);
	void fill_path_renames(Node* p_node, Node* p_new_parent, HashMap<Node*, NodePath>* p_renames);

	SceneTreeEditor* get_tree_editor() { return scene_tree; }

	EditorData* get_editor_data() { return editor_data; }

	List<Node*> get_node_clipboard() const;

	ScriptCreateDialog* get_script_create_dialog() { return script_create_dialog; }

	SceneTreeDock(
		Node* p_scene_root, EditorSelection* p_editor_selection, EditorData& p_editor_data) : scene_root(p_scene_root), editor_selection(p_editor_selection), editor_data(&p_editor_data) {}
	~SceneTreeDock();
};


