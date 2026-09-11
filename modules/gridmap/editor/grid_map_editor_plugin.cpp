/**************************************************************************/
/*  grid_map_editor_plugin.cpp                                            */
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

#include "core/input/input.h"
#include "core/math/geometry_2d.h"
#include "core/os/keyboard.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/editor_main_screen.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_zoom_widget.h"
#include "editor/gui/filter_line_edit.h"
#include "editor/inspector/editor_resource_preview.h"
#include "editor/scene/3d/mesh_library_editor_plugin.h"
#include "editor/scene/3d/node_3d_editor_plugin.h"
#include "editor/scene/3d/node_3d_editor_viewport.h"
#include "editor/settings/editor_command_palette.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "grid_map_editor_plugin.h"
#include "scene/3d/camera_3d.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/item_list.h"
#include "scene/gui/label.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/separator.h"
#include "scene/gui/spin_box.h"
#include "scene/gui/split_container.h"
#include "scene/gui/tree.h"
#include "scene/main/node.h"
#include "scene/main/scene_tree.h"
#include "scene/main/window.h"
#include "servers/rendering/rendering_server.h"

void GridMapEditor::_update_cursor_transform()
{
	cursor_transform = Transform3D();
	cursor_transform.origin = cursor_origin;
	cursor_transform.basis *= node->get_cell_scale();
	cursor_transform = node->get_global_transform() * cursor_transform;

	if (mode_buttons_group->get_pressed_button() == paint_mode_button) {
		// Auto-deselect the selection when painting.
		if (selection.active) {
			_set_selection(false);
		}
		// Rotation is only applied in paint mode, we don't want the cursor box to rotate otherwise.
		cursor_transform.basis *= node->get_basis_with_orthogonal_index(cursor_rot);
		if (selected_palette >= 0 && node && node->get_mesh_library().is_valid()) {
			cursor_transform *= node->get_mesh_library()->get_item_mesh_transform(selected_palette);
		}
	}
	else {
		Transform3D xf;
		xf.scale(node->get_cell_size());
		xf.origin.x = node->get_center_x() ? -node->get_cell_size().x / 2 : 0;
		xf.origin.y = node->get_center_y() ? -node->get_cell_size().y / 2 : 0;
		xf.origin.z = node->get_center_z() ? -node->get_cell_size().z / 2 : 0;
		cursor_transform *= xf;
	}

	if (cursor_instance.is_valid()) {
		RenderingServer::get_singleton()->instance_set_transform(cursor_instance, cursor_transform);
		RenderingServer::get_singleton()->instance_set_visible(cursor_instance, cursor_visible);
	}
}

void GridMapEditor::_update_selection_transform()
{
	Transform3D xf_zero;
	xf_zero.basis.set_zero();

	if (!selection.active) {
		RenderingServer::get_singleton()->instance_set_transform(selection_instance, xf_zero);
		for (int i = 0; i < 3; i++) {
			RenderingServer::get_singleton()->instance_set_transform(
				selection_level_instance[i], xf_zero);
		}
		return;
	}

	Transform3D xf;
	xf.scale((Vector3(1, 1, 1) + (selection.end - selection.begin)) * node->get_cell_size());
	xf.origin = selection.begin * node->get_cell_size();

	RenderingServer::get_singleton()->instance_set_transform(
		selection_instance, node->get_global_transform() * xf);

	Vector3::Axis edit_axis = _get_edit_axis();
	for (int i = 0; i < 3; i++) {
		if (i != edit_axis || (edit_floor[edit_axis] < selection.begin[edit_axis]) ||
			(edit_floor[edit_axis] > selection.end[edit_axis] + 1)) {
			RenderingServer::get_singleton()->instance_set_transform(
				selection_level_instance[i], xf_zero);
		}
		else {
			Vector3 scale = (selection.end - selection.begin + Vector3(1, 1, 1));
			scale[edit_axis] = 1.0;
			Vector3 position = selection.begin;
			position[edit_axis] = edit_floor[edit_axis];

			scale *= node->get_cell_size();
			position *= node->get_cell_size();

			Transform3D xf2;
			xf2.basis.scale(scale);
			xf2.origin = position;

			RenderingServer::get_singleton()->instance_set_transform(
				selection_level_instance[i], node->get_global_transform() * xf2);
		}
	}
}

void GridMapEditor::_validate_selection()
{
	if (!selection.active) {
		return;
	}
	selection.begin = selection.click;
	selection.end = selection.current;

	if (selection.begin.x > selection.end.x) {
		SWAP(selection.begin.x, selection.end.x);
	}
	if (selection.begin.y > selection.end.y) {
		SWAP(selection.begin.y, selection.end.y);
	}
	if (selection.begin.z > selection.end.z) {
		SWAP(selection.begin.z, selection.end.z);
	}

	_update_selection_transform();
}

AABB GridMapEditor::_get_selection() const
{
	AABB ret;
	if (selection.active) {
		ret.position = selection.begin;
		ret.size = selection.end - selection.begin;
	}
	else {
		ret.position.zero();
		ret.size.zero();
	}
	return ret;
}

bool GridMapEditor::_has_selection() const { return node != nullptr && selection.active; }

Vector3::Axis GridMapEditor::_get_facing_axis(
	const Basis& p_grid_basis, const Vector3& p_direction) const
{
	float dir_yz = Math::abs(p_grid_basis.get_column(0).dot(p_direction));
	float dir_xz = Math::abs(p_grid_basis.get_column(1).dot(p_direction));
	float dir_xy = Math::abs(p_grid_basis.get_column(2).dot(p_direction));

	if (dir_xy >= dir_xz && dir_xy >= dir_yz) {
		return Vector3::AXIS_Z;
	}
	if (dir_xz >= dir_xy && dir_xz >= dir_yz) {
		return Vector3::AXIS_Y;
	}
	return Vector3::AXIS_X;
}

void GridMapEditor::_view_state_changed(Node3DEditorViewport* p_viewport)
{
	if (node && last_viewport == p_viewport->get_viewport_node()) {
		_update_edit_axis();
	}
}

String GridMapEditor::_get_cursor_coordinates() const
{
	String text;
	if (cursor_visible || !set_items.is_empty() || !clipboard_items.is_empty()) {
		if (selection.active) {
			if (selection.begin == selection.end) {
				text = vformat(String::utf8(u8"(%d, %d, %d)  \u2317  (%d, %d, %d)"),
					(int)cursor_gridpos.x, (int)cursor_gridpos.y, (int)cursor_gridpos.z,
					(int)selection.begin.x, (int)selection.begin.y, (int)selection.begin.z);
			}
			else {
				text = vformat(
					String::utf8(u8"(%d, %d, %d)  \u2317  (%d, %d, %d) \u2192 (%d, %d, %d)"),
					(int)cursor_gridpos.x, (int)cursor_gridpos.y, (int)cursor_gridpos.z,
					(int)selection.begin.x, (int)selection.begin.y, (int)selection.begin.z,
					(int)selection.end.x, (int)selection.end.y, (int)selection.end.z);
			}
		}
		else {
			text = vformat("(%d, %d, %d)", (int)cursor_gridpos.x, (int)cursor_gridpos.y,
				(int)cursor_gridpos.z);
		}
	}
	return text;
}

void GridMapEditor::_delete_selection()
{
	if (!selection.active) {
		return;
	}

	for (int i = selection.begin.x; i <= selection.end.x; i++) {
		for (int j = selection.begin.y; j <= selection.end.y; j++) {
			for (int k = selection.begin.z; k <= selection.end.z; k++) {
				Vector3i selected = Vector3i(i, j, k);
				node->set_cell_item(selected, GridMap::INVALID_CELL_ITEM);
			}
		}
	}
}

void GridMapEditor::_setup_paste_mode()
{
	input_action = INPUT_PASTE;
	paste_indicator.click = selection.click;
	paste_indicator.current = cursor_gridpos;
	paste_indicator.begin = selection.begin;
	paste_indicator.end = selection.end;
	paste_indicator.distance_from_cursor = cursor_gridpos - paste_indicator.begin;
	paste_indicator.orientation = 0;
	_update_paste_indicator();
}

void GridMapEditor::_clear_clipboard_data()
{
	for (const ClipboardItem& E : clipboard_items) {
		if (E.instance.is_null()) {
			continue;
		}
		RenderingServer::get_singleton()->free_rid(E.instance);
	}

	clipboard_items.clear();
	clipboard_is_move = false;
}

void GridMapEditor::_set_clipboard_data()
{
	_clear_clipboard_data();

	Ref<MeshLibrary> meshLibrary = node->get_mesh_library();

	const RID scenario = get_tree()->get_root()->get_world_3d()->get_scenario();

	for (int i = selection.begin.x; i <= selection.end.x; i++) {
		for (int j = selection.begin.y; j <= selection.end.y; j++) {
			for (int k = selection.begin.z; k <= selection.end.z; k++) {
				Vector3i selected = Vector3i(i, j, k);
				int itm = node->get_cell_item(selected);
				if (itm == GridMap::INVALID_CELL_ITEM) {
					continue;
				}

				Ref<Mesh> mesh = meshLibrary->get_item_mesh(itm);

				ClipboardItem item;
				item.cell_item = itm;
				item.grid_offset = Vector3(selected) - selection.begin;
				item.orientation = node->get_cell_item_orientation(selected);

				if (mesh.is_valid()) {
					item.instance = RenderingServer::get_singleton()->instance_create2(
						mesh->get_rid(), scenario);
				}

				clipboard_items.push_back(item);
			}
		}
	}
}

void GridMapEditor::_update_paste_indicator()
{
	if (input_action != INPUT_PASTE) {
		Transform3D xf;
		xf.basis.set_zero();
		RenderingServer::get_singleton()->instance_set_transform(paste_instance, xf);
		return;
	}

	Vector3 center = 0.5 * Vector3(real_t(node->get_center_x()), real_t(node->get_center_y()),
							   real_t(node->get_center_z()));
	Vector3 scale =
		(Vector3(1, 1, 1) + (paste_indicator.end - paste_indicator.begin)) * node->get_cell_size();
	Transform3D xf;
	xf.scale(scale);
	xf.origin = (paste_indicator.current - paste_indicator.distance_from_cursor + center) *
				node->get_cell_size();
	Basis rot;
	rot = node->get_basis_with_orthogonal_index(paste_indicator.orientation);
	xf.basis = rot * xf.basis;
	xf.translate_local((-center * node->get_cell_size()) / scale);

	RenderingServer::get_singleton()->instance_set_transform(
		paste_instance, node->get_global_transform() * xf);

	for (const ClipboardItem& item : clipboard_items) {
		if (item.instance.is_null()) {
			continue;
		}
		xf = Transform3D();
		xf.origin = (paste_indicator.current - paste_indicator.distance_from_cursor + center) *
					node->get_cell_size();
		xf.basis = rot;
		xf.translate_local(item.grid_offset * node->get_cell_size());

		Basis item_rot;
		item_rot = node->get_basis_with_orthogonal_index(item.orientation);
		xf.basis *= item_rot * node->get_cell_scale();

		RenderingServer::get_singleton()->instance_set_transform(
			item.instance, node->get_global_transform() * xf);
	}
}

void GridMapEditor::_cancel_pending_move()
{
	if (input_action == INPUT_PASTE) {
		if (clipboard_is_move) {
			for (const ClipboardItem& item : clipboard_items) {
				Vector3 original_position = paste_indicator.begin + item.grid_offset;
				node->set_cell_item(Vector3i(original_position), item.cell_item, item.orientation);
			}
		}
		_clear_clipboard_data();
		input_action = INPUT_NONE;
		_update_paste_indicator();
	}
}

struct _CGMEItemSort
{
	String name;
	int id = 0;

	_FORCE_INLINE_ bool operator<(const _CGMEItemSort& r_it) const { return name < r_it.name; }
};

void GridMapEditor::_set_display_mode(int p_mode)
{
	if (display_mode == p_mode) {
		return;
	}

	if (p_mode == DISPLAY_LIST) {
		mode_list->set_pressed(true);
		mode_thumbnail->set_pressed(false);
	}
	else { // DISPLAY_THUMBNAIL
		mode_list->set_pressed(false);
		mode_thumbnail->set_pressed(true);
	}

	display_mode = p_mode;

	update_palette();
}

void GridMapEditor::_text_changed(const String& p_text) { update_palette(); }

void GridMapEditor::_icon_size_changed(float p_value)
{
	mesh_library_palette->set_icon_scale(p_value);
	update_palette();
}

void GridMapEditor::update_layout(EditorDock::DockLayout p_layout, int p_slot)
{
	if (categories->is_visible()) {
		item_palette_mc->set_theme_type_variation("");
		mesh_library_palette->set_scroll_hint_mode(ItemList::SCROLL_HINT_MODE_DISABLED);
		mesh_library_palette->set_theme_type_variation("ItemListSecondary");
	}
	else {
		bool is_bottom = p_slot == EditorDock::DOCK_SLOT_BOTTOM;
		item_palette_mc->set_theme_type_variation(
			is_bottom ? "NoBorderHorizontal" : "NoBorderHorizontalBottom");
		mesh_library_palette->set_scroll_hint_mode(
			is_bottom ? ItemList::SCROLL_HINT_MODE_BOTH : ItemList::SCROLL_HINT_MODE_TOP);
		mesh_library_palette->set_theme_type_variation("");
	}
}

void GridMapEditor::_on_categories_item_activated()
{
	TreeItem* ti_selected = categories->get_selected();
	if (ti_selected) {
		bool collapsed = ti_selected->is_collapsed();
		ti_selected->set_collapsed(!collapsed);
	}
}

void GridMapEditor::_update_resource_preview(const String& p_path, const Ref<Texture2D>& p_preview,
	const Ref<Texture2D>& p_small_preview, int p_idx)
{
	if (p_idx < mesh_library_palette->get_item_count()) {
		mesh_library_palette->set_item_icon(p_idx, p_preview);
	}
}

void GridMapEditor::update_grid()
{
	grid_xform.origin.x -= 1; // Force update in hackish way.

	Vector3::Axis edit_axis = _get_edit_axis();
	grid_ofs[edit_axis] = edit_floor[edit_axis] * node->get_cell_size()[edit_axis];

	edit_grid_xform.origin = grid_ofs;
	edit_grid_xform.basis = Basis();

	for (int i = 0; i < 3; i++) {
		RenderingServer::get_singleton()->instance_set_visible(grid_instance[i], i == edit_axis);
	}

	updating = true;
	floor->set_value(edit_floor[edit_axis]);
	updating = false;
}

void GridMapEditor::_update_theme()
{
	transform_mode_button->set_button_icon(
		get_theme_icon(SNAME("ToolMove"), EditorStringName(EditorIcons)));
	select_mode_button->set_button_icon(
		get_theme_icon(SNAME("ToolSelect"), EditorStringName(EditorIcons)));
	erase_mode_button->set_button_icon(
		get_theme_icon(SNAME("Eraser"), EditorStringName(EditorIcons)));
	paint_mode_button->set_button_icon(
		get_theme_icon(SNAME("Paint"), EditorStringName(EditorIcons)));
	pick_mode_button->set_button_icon(
		get_theme_icon(SNAME("ColorPick"), EditorStringName(EditorIcons)));
	fill_action_button->set_button_icon(
		get_theme_icon(SNAME("Bucket"), EditorStringName(EditorIcons)));
	move_action_button->set_button_icon(
		get_theme_icon(SNAME("ActionCut"), EditorStringName(EditorIcons)));
	duplicate_action_button->set_button_icon(
		get_theme_icon(SNAME("ActionCopy"), EditorStringName(EditorIcons)));
	delete_action_button->set_button_icon(
		get_theme_icon(SNAME("Clear"), EditorStringName(EditorIcons)));
	rotate_x_button->set_button_icon(
		get_theme_icon(SNAME("RotateLeft"), EditorStringName(EditorIcons)));
	rotate_y_button->set_button_icon(
		get_theme_icon(SNAME("ToolRotate"), EditorStringName(EditorIcons)));
	rotate_z_button->set_button_icon(
		get_theme_icon(SNAME("RotateRight"), EditorStringName(EditorIcons)));
	clear_rotation_button->set_button_icon(
		get_theme_icon(SNAME("UndoRedo"), EditorStringName(EditorIcons)));
	mode_thumbnail->set_button_icon(
		get_theme_icon(SNAME("FileThumbnail"), EditorStringName(EditorIcons)));
	mode_list->set_button_icon(get_theme_icon(SNAME("FileList"), EditorStringName(EditorIcons)));
	options->set_button_icon(get_theme_icon(SNAME("Tools"), EditorStringName(EditorIcons)));
}

void GridMapEditor::_on_tool_mode_changed()
{
	_show_viewports_transform_gizmo(
		mode_buttons_group->get_pressed_button() == transform_mode_button);
	_update_cursor_instance();
}

void GridMapEditor::_floor_mouse_exited() { floor->get_line_edit()->release_focus(); }

GridMapEditor::~GridMapEditor()
{
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	_clear_clipboard_data();

	for (int i = 0; i < 3; i++) {
		if (grid[i].is_valid()) {
			RenderingServer::get_singleton()->free_rid(grid[i]);
		}
		if (grid_instance[i].is_valid()) {
			RenderingServer::get_singleton()->free_rid(grid_instance[i]);
		}
		if (selection_level_instance[i].is_valid()) {
			RenderingServer::get_singleton()->free_rid(selection_level_instance[i]);
		}
		if (selection_level_mesh[i].is_valid()) {
			RenderingServer::get_singleton()->free_rid(selection_level_mesh[i]);
		}
	}

	RenderingServer::get_singleton()->free_rid(cursor_mesh);
	if (cursor_instance.is_valid()) {
		RenderingServer::get_singleton()->free_rid(cursor_instance);
	}

	RenderingServer::get_singleton()->free_rid(selection_mesh);
	if (selection_instance.is_valid()) {
		RenderingServer::get_singleton()->free_rid(selection_instance);
	}

	RenderingServer::get_singleton()->free_rid(paste_mesh);
	if (paste_instance.is_valid()) {
		RenderingServer::get_singleton()->free_rid(paste_instance);
	}
}

void GridMapEditorPlugin::forward_3d_draw_over_viewport(Control* p_overlay)
{
	Point2 msgpos = Point2(20 * EDSCALE, p_overlay->get_size().y - 20 * EDSCALE);
	String text = grid_map_editor->_get_cursor_coordinates();
	if (text.is_empty()) {
		return;
	}

	Ref<Font> font = p_overlay->get_theme_font(SceneStringName(font), SNAME("Label"));
	int font_size = p_overlay->get_theme_font_size(SceneStringName(font_size), SNAME("Label"));

	p_overlay->draw_string(font.ptr(), msgpos + Point2(1, 1), text, HORIZONTAL_ALIGNMENT_LEFT, -1,
		font_size, Color(0, 0, 0, 0.8));
	p_overlay->draw_string(font.ptr(), msgpos + Point2(-1, -1), text, HORIZONTAL_ALIGNMENT_LEFT, -1,
		font_size, Color(0, 0, 0, 0.8));
	p_overlay->draw_string(
		font.ptr(), msgpos, text, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, Color(1, 1, 1, 1));
}

GridMap* GridMapEditorPlugin::get_current_grid_map() const
{
	ERR_FAIL_NULL_V(grid_map_editor, nullptr);
	return grid_map_editor->node;
}

void GridMapEditorPlugin::set_selection(const Vector3i& p_begin, const Vector3i& p_end)
{
	ERR_FAIL_NULL(grid_map_editor);
	grid_map_editor->_set_selection(true, p_begin, p_end);
}

void GridMapEditorPlugin::clear_selection()
{
	ERR_FAIL_NULL(grid_map_editor);
	grid_map_editor->_set_selection(false);
}

AABB GridMapEditorPlugin::get_selection() const
{
	ERR_FAIL_NULL_V(grid_map_editor, AABB());
	return grid_map_editor->_get_selection();
}

bool GridMapEditorPlugin::has_selection() const
{
	ERR_FAIL_NULL_V(grid_map_editor, false);
	return grid_map_editor->_has_selection();
}

void GridMapEditorPlugin::set_selected_palette_item(int p_item) const
{
	ERR_FAIL_NULL(grid_map_editor);
	if (grid_map_editor->node && grid_map_editor->node->get_mesh_library().is_valid()) {
		if (p_item < -1) {
			p_item = -1;
		}
		else if (p_item >= grid_map_editor->node->get_mesh_library()->get_item_list().size()) {
			p_item = grid_map_editor->node->get_mesh_library()->get_item_list().size() - 1;
		}
		if (p_item != grid_map_editor->selected_palette) {
			grid_map_editor->selected_palette = p_item;
			grid_map_editor->_update_cursor_instance();
			grid_map_editor->update_palette();
		}
	}
}

int GridMapEditorPlugin::get_selected_palette_item() const
{
	ERR_FAIL_NULL_V(grid_map_editor, 0);
	if (grid_map_editor->selected_palette >= 0 && grid_map_editor->node &&
		grid_map_editor->node->get_mesh_library().is_valid()) {
		return grid_map_editor->selected_palette;
	}
	else {
		return -1;
	}
}


