/**************************************************************************/
/*  grid_map_editor_plugin.h                                              */
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

#include "../grid_map.h"
#include "editor/docks/editor_dock.h"
#include "editor/plugins/editor_plugin.h"
#include "scene/gui/box_container.h"

class BaseButton;
class Button;
class ButtonGroup;
class ConfirmationDialog;
class EditorZoomWidget;
class FilterLineEdit;
class HSlider;
class ItemList;
class MenuButton;
class Node3DEditorPlugin;
class Node3DEditorViewport;
class SpinBox;
class Tree;
class TreeItem;

class GridMapEditor : public EditorDock
{
	static constexpr int32_t GRID_CURSOR_SIZE = 50;

	enum InputAction
	{
		INPUT_NONE,
		INPUT_TRANSFORM,
		INPUT_PAINT,
		INPUT_ERASE,
		INPUT_PICK,
		INPUT_SELECT,
		INPUT_PASTE,
	};

	enum DisplayMode
	{
		DISPLAY_THUMBNAIL,
		DISPLAY_LIST
	};

	InputAction input_action = INPUT_NONE;
	bool valid_mb_press = false;
	Panel* panel = nullptr;
	MenuButton* options = nullptr;
	SpinBox* floor = nullptr;
	double accumulated_floor_delta = 0.0;

	HBoxContainer* toolbar = nullptr;
	TightLocalVector<BaseButton*> viewport_shortcut_buttons;
	Ref<ButtonGroup> mode_buttons_group;
	// mode
	Button* transform_mode_button = nullptr;
	Button* select_mode_button = nullptr;
	Button* erase_mode_button = nullptr;
	Button* paint_mode_button = nullptr;
	Button* pick_mode_button = nullptr;
	// action
	Button* fill_action_button = nullptr;
	Button* move_action_button = nullptr;
	Button* duplicate_action_button = nullptr;
	Button* delete_action_button = nullptr;
	// rotation
	Button* rotate_x_button = nullptr;
	Button* rotate_y_button = nullptr;
	Button* rotate_z_button = nullptr;
	Button* clear_rotation_button = nullptr;

	EditorZoomWidget* zoom_widget = nullptr;
	Button* mode_thumbnail = nullptr;
	Button* mode_list = nullptr;
	FilterLineEdit* search_box = nullptr;
	HSlider* size_slider = nullptr;
	ConfirmationDialog* settings_dialog = nullptr;
	VBoxContainer* settings_vbc = nullptr;
	SpinBox* settings_pick_distance = nullptr;
	Label* spin_box_label = nullptr;

	struct SetItem
	{
		Vector3i position;
		int new_value = 0;
		int new_orientation = 0;
		int old_value = 0;
		int old_orientation = 0;
	};

	LocalVector<SetItem> set_items;

	GridMap* node = nullptr;
	Ref<MeshLibrary> mesh_library = nullptr;

	Transform3D grid_xform;
	Transform3D edit_grid_xform;
	Vector3::Axis edit_axis_select = Vector3::AXIS_Y;
	int edit_floor[3];
	int edit_main_vp = 0;
	Vector3 grid_ofs;

	bool allow_viewport_override = true;
	Viewport* last_viewport = nullptr;
	Vector3::Axis viewport_axis = edit_axis_select;

	RID grid[3];
	RID grid_instance[3];
	RID cursor_mesh;
	RID cursor_instance;
	RID selection_mesh;
	RID selection_instance;
	RID selection_level_mesh[3];
	RID selection_level_instance[3];
	RID paste_mesh;
	RID paste_instance;

	struct ClipboardItem
	{
		int cell_item = 0;
		Vector3 grid_offset;
		int orientation = 0;
		RID instance;
	};

	LocalVector<ClipboardItem> clipboard_items;
	bool clipboard_is_move = false;

	Color default_color;
	Color erase_color;
	Color pick_color;
	Ref<StandardMaterial3D> indicator_mat;
	Ref<StandardMaterial3D> cursor_inner_mat;
	Ref<StandardMaterial3D> cursor_outer_mat;
	Ref<StandardMaterial3D> inner_mat;
	Ref<StandardMaterial3D> outer_mat;
	Ref<StandardMaterial3D> selection_floor_mat;

	bool updating = false;

	struct Selection
	{
		Vector3 click;
		Vector3 current;
		Vector3 begin;
		Vector3 end;
		bool active = false;
	};

	Selection selection;

	Selection last_selection;

	struct PasteIndicator
	{
		Vector3 click;
		Vector3 current;
		Vector3 begin;
		Vector3 end;
		Vector3 distance_from_cursor;
		int orientation = 0;
	};

	PasteIndicator paste_indicator;

	bool cursor_visible = false;
	Transform3D cursor_transform;

	Vector3 cursor_origin;
	Vector3i cursor_gridpos;

	int display_mode = DISPLAY_THUMBNAIL;
	int selected_palette = -1;
	int cursor_rot = 0;

	enum Menu
	{
		MENU_OPTION_NEXT_LEVEL,
		MENU_OPTION_PREV_LEVEL,
		MENU_OPTION_LOCK_VIEW,
		MENU_OPTION_X_AXIS,
		MENU_OPTION_Y_AXIS,
		MENU_OPTION_Z_AXIS,
		MENU_OPTION_VIEWPORT_OVERRIDE,
		MENU_OPTION_CURSOR_ROTATE_Y,
		MENU_OPTION_CURSOR_ROTATE_X,
		MENU_OPTION_CURSOR_ROTATE_Z,
		MENU_OPTION_CURSOR_BACK_ROTATE_Y,
		MENU_OPTION_CURSOR_BACK_ROTATE_X,
		MENU_OPTION_CURSOR_BACK_ROTATE_Z,
		MENU_OPTION_CURSOR_CLEAR_ROTATION,
		MENU_OPTION_PASTE_SELECTS,
		MENU_OPTION_SELECTION_DUPLICATE,
		MENU_OPTION_SELECTION_MOVE,
		MENU_OPTION_SELECTION_CLEAR,
		MENU_OPTION_SELECTION_FILL,
		MENU_OPTION_GRIDMAP_SETTINGS

	};

	struct AreaDisplay
	{
		RID mesh;
		RID instance;
	};

	Tree* categories = nullptr;
	MarginContainer* item_palette_mc = nullptr;
	ItemList* mesh_library_palette = nullptr;
	Label* info_message = nullptr;

	void update_grid(); // Change which and where the grid is displayed.
	void _update_resource_preview(const String& p_path, const Ref<Texture2D>& p_preview,
		const Ref<Texture2D>& p_small_preview, int p_idx);

	void _clear_clipboard_data();
	void _set_clipboard_data();
	void _update_paste_indicator();
	void _cancel_pending_move();
	void _update_selection_transform();
	void _validate_selection();
	AABB _get_selection() const;
	bool _has_selection() const;

	Vector3::Axis _get_facing_axis(const Basis& p_grid_basis, const Vector3& p_direction) const;

	Vector3::Axis _get_edit_axis() const
	{
		return allow_viewport_override ? viewport_axis : edit_axis_select;
	}

	String _get_cursor_coordinates() const;

	void _floor_mouse_exited();

	void _delete_selection();
	void _setup_paste_mode();

	friend class GridMapEditorPlugin;

	void _on_categories_item_activated();

protected:
	virtual void update_layout(EditorDock::DockLayout p_layout, int p_slot) override;

public:
	GridMapEditor() = default;
	~GridMapEditor();
};

class GridMapEditorPlugin : public EditorPlugin
{
	GridMapEditor* grid_map_editor = nullptr;

public:
	virtual void forward_3d_draw_over_viewport(Control* p_overlay) override;

	virtual String get_plugin_name() const override { return "GridMap"; }

	GridMap* get_current_grid_map() const;
	AABB get_selection() const;
	bool has_selection() const;
	int get_selected_palette_item() const;
};


