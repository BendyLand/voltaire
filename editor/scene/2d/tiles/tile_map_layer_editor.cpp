/**************************************************************************/
/*  tile_map_layer_editor.cpp                                             */
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
#include "core/math/random_pcg.h"
#include "core/os/keyboard.h"
#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/inspector/editor_resource_preview.h"
#include "editor/inspector/multi_node_edit.h"
#include "editor/scene/2d/tiles/tiles_editor_plugin.h"
#include "editor/scene/canvas_item_editor_plugin.h"
#include "editor/settings/editor_command_palette.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/2d/tile_map.h"
#include "scene/2d/tile_map_layer.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/split_container.h"
#include "scene/main/scene_tree.h"
#include "tile_map_layer_editor.h"

void SwitchSeparator::set_vertical(bool p_vertical)
{
	h_separator->set_visible(p_vertical);
	v_separator->set_visible(!p_vertical);
}

SwitchSeparator::SwitchSeparator()
{
	h_separator = memnew(HSeparator);
	h_separator->hide();
	add_child(h_separator);

	v_separator = memnew(VSeparator);
	add_child(v_separator);
}

void TileMapLayerSubEditorPlugin::_add_to_output_if_tile_changed(
	HashMap<Vector2i, TileMapCell>& p_output, const TileMapLayer* p_layer, Vector2i p_coords,
	const TileMapCell& p_cell)
{
	if (p_cell != p_layer->get_cell(p_coords)) {
		p_output[p_coords] = p_cell;
	}
}

void TileMapLayerSubEditorPlugin::draw_tile_coords_over_viewport(Control* p_overlay,
	const TileMapLayer* p_edited_layer, Ref<TileSet> p_tile_set, bool p_show_rectangle_size,
	const Vector2i& p_rectangle_origin)
{
	Point2 msgpos = Point2(20 * EDSCALE, p_overlay->get_size().y - 20 * EDSCALE);
	String text = String(p_tile_set->local_to_map(p_edited_layer->get_local_mouse_position()));

	if (p_show_rectangle_size) {
		Vector2i rect_size = p_tile_set->local_to_map(p_edited_layer->get_local_mouse_position()) -
							 p_tile_set->local_to_map(p_rectangle_origin);
		text += vformat(" %s (%dx%d)", TTR("Drawing Rect:"), Math::abs(rect_size.x) + 1,
			Math::abs(rect_size.y) + 1);
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

void TileMapLayerEditorTilesPlugin::tile_set_changed()
{
	_update_fix_selected_and_hovered();
	_update_tile_set_sources_list();
	_update_source_display();
	_update_patterns_list();
}

void TileMapLayerEditorTilesPlugin::_on_random_tile_checkbox_toggled(bool p_pressed)
{
	scatter_controls_container->set_visible(p_pressed);
}

void TileMapLayerEditorTilesPlugin::_on_scattering_spinbox_changed(double p_value)
{
	scattering = p_value;
}

void TileMapLayerEditorTilesPlugin::_update_toolbar()
{
	// Stop dragging if needed.
	_stop_dragging();

	// Show only the correct settings.
	const BaseButton* pressed_tool = tool_buttons_group->get_pressed_button();
	bool using_select = (pressed_tool == select_tool_button);
	tools_settings_vsep->set_visible(!using_select);
	picker_button->set_visible(!using_select);
	erase_button->set_visible(!using_select);
	random_tile_toggle->set_visible(!using_select);
	bucket_contiguous_checkbox->set_visible(!using_select && pressed_tool == bucket_tool_button);
	scatter_controls_container->set_visible(!using_select && random_tile_toggle->is_pressed());
	CanvasItemEditor::get_singleton()->set_current_tool(CanvasItemEditor::TOOL_SELECT);
}

void TileMapLayerEditorTilesPlugin::_update_transform_buttons()
{
	TileMapLayer* edited_layer = _get_edited_layer();
	if (!edited_layer) {
		return;
	}

	Ref<TileSet> tile_set = edited_layer->get_tile_set();
	if (tile_set.is_null() || selection_pattern.is_null()) {
		return;
	}

	if (tile_set->get_tile_shape() != TileSet::TILE_SHAPE_SQUARE &&
		selection_pattern->get_size() != Vector2i(1, 1)) {
		_set_transform_buttons_state({transform_button_flip_h, transform_button_flip_v},
			{transform_button_rotate_left, transform_button_rotate_right},
			TTRC("Can't rotate patterns when using non-square tile grid."));
	}
	else {
		_set_transform_buttons_state({transform_button_rotate_left, transform_button_rotate_right,
										 transform_button_flip_h, transform_button_flip_v},
			{}, "");
	}
}

void TileMapLayerEditorTilesPlugin::_set_transform_buttons_state(
	const Vector<Button*>& p_enabled_buttons, const Vector<Button*>& p_disabled_buttons,
	const String& p_why_disabled)
{
	for (Button* button : p_enabled_buttons) {
		button->set_disabled(false);
		button->set_tooltip_text("");
	}
	for (Button* button : p_disabled_buttons) {
		button->set_disabled(true);
		button->set_tooltip_text(p_why_disabled);
	}
}

void TileMapLayerEditorTilesPlugin::_update_translation()
{
	paint_tool_button->set_tooltip_text(TTR("Shift: Draw line.") + "\n" +
										vformat(TTR("%s+Shift: Draw rectangle."),
											keycode_get_string((Key)KeyModifierMask::CMD_OR_CTRL)));

	if (scene_tiles_list->is_visible_in_tree()) {
		_update_scenes_collection_view();
		_update_tile_set_sources_list();
	}
	_update_patterns_list();
}

Vector<TileMapLayerSubEditorPlugin::TabData> TileMapLayerEditorTilesPlugin::get_tabs() const
{
	Vector<TileMapLayerSubEditorPlugin::TabData> tabs;
	Vector<Control*> toolbar_controls;
	toolbar_controls.push_back(tilemap_tiles_tools_buttons);
	toolbar_controls.push_back(tools_settings);
	toolbar_controls.push_back(tools_settings_vsep);
	tabs.push_back({toolbar_controls, wide_toolbar, tiles_bottom_panel});
	tabs.push_back({toolbar_controls, wide_toolbar, patterns_mc});
	return tabs;
}

void TileMapLayerEditorTilesPlugin::_tab_changed()
{
	if (tiles_bottom_panel->is_visible_in_tree()) {
		_update_selection_pattern_from_tileset_tiles_selection();
	}
	else if (patterns_mc->is_visible_in_tree()) {
		_update_selection_pattern_from_tileset_pattern_selection();
	}
}

void TileMapLayerEditorTilesPlugin::_scene_thumbnail_done(const String& p_path,
	const Ref<Texture2D>& p_preview, const Ref<Texture2D>& p_small_preview, int p_index)
{
	if (p_index >= 0 && p_index < scene_tiles_list->get_item_count()) {
		scene_tiles_list->set_item_icon(p_index, p_preview);
	}
}

void TileMapLayerEditorTilesPlugin::_scenes_list_lmb_empty_clicked(
	const Vector2& p_pos, MouseButton p_mouse_button_index)
{
	if (p_mouse_button_index != MouseButton::LEFT) {
		return;
	}

	scene_tiles_list->deselect_all();
	tile_set_selection.clear();
	tile_map_selection.clear();
	selection_pattern.instantiate();
	_update_selection_pattern_from_tileset_tiles_selection();
}

void TileMapLayerEditorTilesPlugin::_update_theme()
{
	source_sort_button->set_button_icon(tiles_bottom_panel->get_editor_theme_icon(SNAME("Sort")));
	select_tool_button->set_button_icon(
		tiles_bottom_panel->get_editor_theme_icon(SNAME("ToolSelect")));
	paint_tool_button->set_button_icon(tiles_bottom_panel->get_editor_theme_icon(SNAME("Edit")));
	line_tool_button->set_button_icon(tiles_bottom_panel->get_editor_theme_icon(SNAME("Line")));
	rect_tool_button->set_button_icon(
		tiles_bottom_panel->get_editor_theme_icon(SNAME("Rectangle")));
	bucket_tool_button->set_button_icon(tiles_bottom_panel->get_editor_theme_icon(SNAME("Bucket")));

	picker_button->set_button_icon(tiles_bottom_panel->get_editor_theme_icon(SNAME("ColorPick")));
	erase_button->set_button_icon(tiles_bottom_panel->get_editor_theme_icon(SNAME("Eraser")));
	random_tile_toggle->set_button_icon(
		tiles_bottom_panel->get_editor_theme_icon(SNAME("RandomNumberGenerator")));

	transform_button_rotate_left->set_button_icon(
		tiles_bottom_panel->get_editor_theme_icon("RotateLeft"));
	transform_button_rotate_right->set_button_icon(
		tiles_bottom_panel->get_editor_theme_icon("RotateRight"));
	transform_button_flip_h->set_button_icon(tiles_bottom_panel->get_editor_theme_icon("MirrorX"));
	transform_button_flip_v->set_button_icon(tiles_bottom_panel->get_editor_theme_icon("MirrorY"));

	missing_atlas_texture_icon = tiles_bottom_panel->get_editor_theme_icon(SNAME("TileSet"));
	_update_tile_set_sources_list();
}

void TileMapLayerEditorTilesPlugin::_mouse_exited_viewport()
{
	has_mouse = false;
	CanvasItemEditor::get_singleton()->update_viewport();
}

void TileMapLayerEditorTilesPlugin::_apply_transform(TileTransformType p_type)
{
	if (selection_pattern.is_null() || selection_pattern->is_empty()) {
		return;
	}

	Ref<TileMapPattern> transformed_pattern;
	transformed_pattern.instantiate();

	Vector2i size = selection_pattern->get_size();
	for (int y = 0; y < size.y; y++) {
		for (int x = 0; x < size.x; x++) {
			Vector2i src_coords = Vector2i(x, y);
			if (!selection_pattern->has_cell(src_coords)) {
				continue;
			}

			Vector2i dst_coords;

			if (p_type == TRANSFORM_ROTATE_LEFT) {
				dst_coords = Vector2i(y, size.x - x - 1);
			}
			else if (p_type == TRANSFORM_ROTATE_RIGHT) {
				dst_coords = Vector2i(size.y - y - 1, x);
			}
			else if (p_type == TRANSFORM_FLIP_H) {
				dst_coords = Vector2i(size.x - x - 1, y);
			}
			else if (p_type == TRANSFORM_FLIP_V) {
				dst_coords = Vector2i(x, size.y - y - 1);
			}

			transformed_pattern->set_cell(dst_coords,
				selection_pattern->get_cell_source_id(src_coords),
				selection_pattern->get_cell_atlas_coords(src_coords),
				_get_transformed_alternative(
					selection_pattern->get_cell_alternative_tile(src_coords), p_type));
		}
	}
	selection_pattern = transformed_pattern;
	CanvasItemEditor::get_singleton()->update_viewport();
}

int TileMapLayerEditorTilesPlugin::_get_transformed_alternative(
	int p_alternative_id, TileTransformType p_transform)
{
	bool transform_flip_h = p_alternative_id & TileSetAtlasSource::TRANSFORM_FLIP_H;
	bool transform_flip_v = p_alternative_id & TileSetAtlasSource::TRANSFORM_FLIP_V;
	bool transform_transpose = p_alternative_id & TileSetAtlasSource::TRANSFORM_TRANSPOSE;

	switch (p_transform) {
	case TRANSFORM_ROTATE_LEFT: { // (h, v, t) -> (v, !h, !t)
		bool negated_flip_h = !transform_flip_h;
		transform_flip_h = transform_flip_v;
		transform_flip_v = negated_flip_h;
		transform_transpose = !transform_transpose;
	} break;
	case TRANSFORM_ROTATE_RIGHT: { // (h, v, t) -> (!v, h, !t)
		bool negated_flip_v = !transform_flip_v;
		transform_flip_v = transform_flip_h;
		transform_flip_h = negated_flip_v;
		transform_transpose = !transform_transpose;
	} break;
	case TRANSFORM_FLIP_H: { // (h, v, t) -> (!h, v, t)
		transform_flip_h = !transform_flip_h;
	} break;
	case TRANSFORM_FLIP_V: { // (h, v, t) -> (h, !v, t)
		transform_flip_v = !transform_flip_v;
	} break;
	}

	return TileSetAtlasSource::alternative_no_transform(p_alternative_id) |
		   int(transform_flip_h) * TileSetAtlasSource::TRANSFORM_FLIP_H |
		   int(transform_flip_v) * TileSetAtlasSource::TRANSFORM_FLIP_V |
		   int(transform_transpose) * TileSetAtlasSource::TRANSFORM_TRANSPOSE;
}

void TileMapLayerEditorTilesPlugin::_fix_invalid_tiles_in_tile_map_selection()
{
	TileMapLayer* edited_layer = _get_edited_layer();
	if (!edited_layer) {
		return;
	}

	RBSet<Vector2i> to_remove;
	for (Vector2i selected : tile_map_selection) {
		TileMapCell cell = edited_layer->get_cell(selected);
		if (cell.source_id == TileSet::INVALID_SOURCE &&
			cell.get_atlas_coords() == TileSetSource::INVALID_ATLAS_COORDS &&
			cell.alternative_tile == TileSetAtlasSource::INVALID_TILE_ALTERNATIVE) {
			to_remove.insert(selected);
		}
	}

	for (Vector2i cell : to_remove) {
		tile_map_selection.erase(cell);
	}
}

void TileMapLayerEditorTilesPlugin::patterns_item_list_empty_clicked(
	const Vector2& p_pos, MouseButton p_mouse_button_index)
{
	if (p_mouse_button_index == MouseButton::LEFT) {
		_update_selection_pattern_from_tileset_pattern_selection();
	}
}

void TileMapLayerEditorTilesPlugin::_tile_atlas_control_mouse_exited()
{
	hovered_tile.source_id = TileSet::INVALID_SOURCE;
	hovered_tile.set_atlas_coords(TileSetSource::INVALID_ATLAS_COORDS);
	hovered_tile.alternative_tile = TileSetSource::INVALID_TILE_ALTERNATIVE;
	tile_atlas_control->queue_redraw();
}

void TileMapLayerEditorTilesPlugin::_tile_alternatives_control_mouse_exited()
{
	hovered_tile.source_id = TileSet::INVALID_SOURCE;
	hovered_tile.set_atlas_coords(TileSetSource::INVALID_ATLAS_COORDS);
	hovered_tile.alternative_tile = TileSetSource::INVALID_TILE_ALTERNATIVE;
	alternative_tiles_control->queue_redraw();
}

void TileMapLayerEditorTilesPlugin::_bind_methods() {}

void TileMapLayerEditorTilesPlugin::update_layout(EditorDock::DockLayout p_layout, int p_slot)
{
	bool is_vertical = (p_layout == EditorDock::DOCK_LAYOUT_VERTICAL);
	atlas_sources_split_container->set_vertical(is_vertical);
	atlas_sources_split_container->move_child(split_container_left_side, is_vertical ? -1 : 0);
	split_container_left_side->set_vertical(!is_vertical);

	tilemap_tiles_tools_buttons->set_vertical(is_vertical);
	transform_toolbar->set_vertical(is_vertical);
	tools_settings->set_vertical(is_vertical);
	tools_settings_vsep->set_vertical(is_vertical);
	transform_separator->set_vertical(is_vertical);

	wide_toolbar->set_visible(is_vertical);
	bucket_contiguous_checkbox->reparent(is_vertical ? wide_toolbar : tools_settings);
	scatter_controls_container->reparent(is_vertical ? wide_toolbar : tools_settings);

	if (p_layout == EditorDock::DOCK_LAYOUT_FLOATING ||
		(!is_vertical && p_slot != EditorDock::DOCK_SLOT_BOTTOM)) {
		patterns_mc->set_theme_type_variation("NoBorderHorizontalBottom");
		patterns_item_list->set_scroll_hint_mode(ItemList::SCROLL_HINT_MODE_TOP);
	}
	else {
		patterns_mc->set_theme_type_variation(is_vertical ? "" : "NoBorderHorizontal");
		patterns_item_list->set_scroll_hint_mode(
			is_vertical ? ItemList::SCROLL_HINT_MODE_DISABLED : ItemList::SCROLL_HINT_MODE_BOTH);
	}
	patterns_item_list->set_theme_type_variation(is_vertical ? "ItemListSecondary" : "");
}

void TileMapLayerEditorTerrainsPlugin::tile_set_changed()
{
	_update_terrains_cache();
	_update_terrains_tree();
	_update_tiles_list();
}

void TileMapLayerEditorTerrainsPlugin::_update_toolbar()
{
	bucket_contiguous_checkbox->set_visible(
		tool_buttons_group->get_pressed_button() == bucket_tool_button);
}

Vector<TileMapLayerSubEditorPlugin::TabData> TileMapLayerEditorTerrainsPlugin::get_tabs() const
{
	Vector<TileMapLayerSubEditorPlugin::TabData> tabs;
	Vector<Control*> toolbar_controls;
	toolbar_controls.push_back(tilemap_tiles_tools_buttons);
	toolbar_controls.push_back(tools_settings);
	tabs.push_back({toolbar_controls, wide_toolbar, main_box_container});
	return tabs;
}

void TileMapLayerEditorTerrainsPlugin::_mouse_exited_viewport()
{
	has_mouse = false;
	CanvasItemEditor::get_singleton()->update_viewport();
}

bool TileMapLayerEditorTerrainsPlugin::forward_canvas_gui_input(const Ref<InputEvent>& p_event)
{
	if (!main_box_container->is_visible_in_tree()) {
		// If the bottom editor is not visible, we ignore inputs.
		return false;
	}

	if (CanvasItemEditor::get_singleton()->get_current_tool() != CanvasItemEditor::TOOL_SELECT) {
		return false;
	}

	TileMapLayer* edited_layer = _get_edited_layer();
	if (!edited_layer) {
		return false;
	}

	Ref<TileSet> tile_set = edited_layer->get_tile_set();
	if (tile_set.is_null()) {
		return false;
	}

	_update_selection();

	Ref<InputEventKey> k = p_event;
	if (k.is_valid() && k->is_pressed() && !k->is_echo()) {
		for (BaseButton* b : viewport_shortcut_buttons) {
			if (b->get_shortcut().is_valid() && b->get_shortcut()->matches_event(p_event)) {
				b->set_pressed(b->get_button_group().is_valid() || !b->is_pressed());
				return true;
			}
		}
	}

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		has_mouse = true;
		Transform2D xform = CanvasItemEditor::get_singleton()->get_canvas_transform() *
							edited_layer->get_global_transform_with_canvas();
		Vector2 mpos = xform.affine_inverse().xform(mm->get_position());

		switch (drag_type) {
		case DRAG_TYPE_PAINT: {
			if (selected_terrain_set >= 0) {
				HashMap<Vector2i, TileMapCell> to_draw =
					_draw_line(tile_set->local_to_map(drag_last_mouse_pos),
						tile_set->local_to_map(mpos), drag_erasing);
				for (const KeyValue<Vector2i, TileMapCell>& E : to_draw) {
					if (!drag_modified.has(E.key)) {
						drag_modified[E.key] = edited_layer->get_cell(E.key);
					}
					edited_layer->set_cell(E.key, E.value.source_id, E.value.get_atlas_coords(),
						E.value.alternative_tile);
				}
			}
		} break;
		default:
			break;
		}
		drag_last_mouse_pos = mpos;
		CanvasItemEditor::get_singleton()->update_viewport();

		return true;
	}

	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid()) {
		has_mouse = true;
		Transform2D xform = CanvasItemEditor::get_singleton()->get_canvas_transform() *
							edited_layer->get_global_transform_with_canvas();
		Vector2 mpos = xform.affine_inverse().xform(mb->get_position());

		if (mb->get_button_index() == MouseButton::LEFT ||
			mb->get_button_index() == MouseButton::RIGHT) {
			if (mb->is_pressed()) {
				// Pressed
				if (erase_button->is_pressed() || mb->get_button_index() == MouseButton::RIGHT) {
					drag_erasing = true;
				}

				if (picker_button->is_pressed()) {
					drag_type = DRAG_TYPE_PICK;
				}
				else {
					// Paint otherwise.
					const BaseButton* pressed_tool = tool_buttons_group->get_pressed_button();
					if (pressed_tool == paint_tool_button &&
						!Input::get_singleton()->is_key_pressed(Key::CMD_OR_CTRL) &&
						!Input::get_singleton()->is_key_pressed(Key::SHIFT)) {
						if (selected_terrain_set < 0 || selected_terrain < 0 ||
							(selected_type == SELECTED_TYPE_PATTERN &&
								!selected_terrains_pattern.is_valid())) {
							return true;
						}

						drag_type = DRAG_TYPE_PAINT;
						drag_start_mouse_pos = mpos;

						drag_modified.clear();
						Vector2i cell = tile_set->local_to_map(mpos);
						HashMap<Vector2i, TileMapCell> to_draw =
							_draw_line(cell, cell, drag_erasing);
						for (const KeyValue<Vector2i, TileMapCell>& E : to_draw) {
							drag_modified[E.key] = edited_layer->get_cell(E.key);
							edited_layer->set_cell(E.key, E.value.source_id,
								E.value.get_atlas_coords(), E.value.alternative_tile);
						}
					}
					else if (pressed_tool == line_tool_button ||
							   (pressed_tool == paint_tool_button &&
								   Input::get_singleton()->is_key_pressed(Key::SHIFT) &&
								   !Input::get_singleton()->is_key_pressed(Key::CMD_OR_CTRL))) {
						if (selected_terrain_set < 0 || selected_terrain < 0 ||
							(selected_type == SELECTED_TYPE_PATTERN &&
								!selected_terrains_pattern.is_valid())) {
							return true;
						}
						drag_type = DRAG_TYPE_LINE;
						drag_start_mouse_pos = mpos;
						drag_modified.clear();
					}
					else if (pressed_tool == rect_tool_button ||
							   (pressed_tool == paint_tool_button &&
								   Input::get_singleton()->is_key_pressed(Key::SHIFT) &&
								   Input::get_singleton()->is_key_pressed(Key::CMD_OR_CTRL))) {
						if (selected_terrain_set < 0 || selected_terrain < 0 ||
							(selected_type == SELECTED_TYPE_PATTERN &&
								!selected_terrains_pattern.is_valid())) {
							return true;
						}
						drag_type = DRAG_TYPE_RECT;
						drag_start_mouse_pos = mpos;
						drag_modified.clear();
					}
					else if (pressed_tool == bucket_tool_button) {
						if (selected_terrain_set < 0 || selected_terrain < 0 ||
							(selected_type == SELECTED_TYPE_PATTERN &&
								!selected_terrains_pattern.is_valid())) {
							return true;
						}
						drag_type = DRAG_TYPE_BUCKET;
						drag_start_mouse_pos = mpos;
						drag_modified.clear();
						Vector<Vector2i> line = TileMapLayerEditor::get_line(edited_layer,
							tile_set->local_to_map(drag_last_mouse_pos),
							tile_set->local_to_map(mpos));
						for (int i = 0; i < line.size(); i++) {
							if (!drag_modified.has(line[i])) {
								HashMap<Vector2i, TileMapCell> to_draw = _draw_bucket_fill(line[i],
									bucket_contiguous_checkbox->is_pressed(), drag_erasing);
								for (const KeyValue<Vector2i, TileMapCell>& E : to_draw) {
									if (!drag_erasing &&
										E.value.source_id == TileSet::INVALID_SOURCE) {
										continue;
									}
									Vector2i coords = E.key;
									if (!drag_modified.has(coords)) {
										drag_modified.insert(
											coords, edited_layer->get_cell(coords));
									}
									edited_layer->set_cell(coords, E.value.source_id,
										E.value.get_atlas_coords(), E.value.alternative_tile);
								}
							}
						}
					}
				}
			}
			else {
				// Released
				_stop_dragging();
				drag_erasing = false;
			}

			CanvasItemEditor::get_singleton()->update_viewport();

			return true;
		}
		drag_last_mouse_pos = mpos;
	}

	return false;
}

void TileMapLayerEditorTerrainsPlugin::_update_terrains_cache()
{
	const TileMapLayer* edited_layer = _get_edited_layer();
	if (!edited_layer) {
		return;
	}

	Ref<TileSet> tile_set = edited_layer->get_tile_set();
	if (tile_set.is_null()) {
		return;
	}

	// Organizes tiles into structures.
	per_terrain_terrains_patterns.resize(tile_set->get_terrain_sets_count());
	for (int i = 0; i < tile_set->get_terrain_sets_count(); i++) {
		per_terrain_terrains_patterns[i].resize(tile_set->get_terrains_count(i));
		for (RBSet<TileSet::TerrainsPattern>& pattern : per_terrain_terrains_patterns[i]) {
			pattern.clear();
		}
	}

	for (int source_index = 0; source_index < tile_set->get_source_count(); source_index++) {
		int source_id = tile_set->get_source_id(source_index);
		Ref<TileSetSource> source = tile_set->get_source(source_id);

		Ref<TileSetAtlasSource> atlas_source = source;
		if (atlas_source.is_valid()) {
			for (int tile_index = 0; tile_index < source->get_tiles_count(); tile_index++) {
				Vector2i tile_id = source->get_tile_id(tile_index);
				for (int alternative_index = 0;
					 alternative_index < source->get_alternative_tiles_count(tile_id);
					 alternative_index++) {
					int alternative_id =
						source->get_alternative_tile_id(tile_id, alternative_index);

					TileData* tile_data = atlas_source->get_tile_data(tile_id, alternative_id);
					int terrain_set = tile_data->get_terrain_set();
					if (terrain_set >= 0) {
						ERR_FAIL_INDEX(terrain_set, (int)per_terrain_terrains_patterns.size());

						TileMapCell cell;
						cell.source_id = source_id;
						cell.set_atlas_coords(tile_id);
						cell.alternative_tile = alternative_id;

						TileSet::TerrainsPattern terrains_pattern =
							tile_data->get_terrains_pattern();

						// Terrain center bit
						int terrain = terrains_pattern.get_terrain();
						if (terrain >= 0 &&
							terrain < (int)per_terrain_terrains_patterns[terrain_set].size()) {
							per_terrain_terrains_patterns[terrain_set][terrain].insert(
								terrains_pattern);
						}

						// Terrain bits.
						for (int i = 0; i < TileSet::CELL_NEIGHBOR_MAX; i++) {
							TileSet::CellNeighbor bit = TileSet::CellNeighbor(i);
							if (tile_set->is_valid_terrain_peering_bit(terrain_set, bit)) {
								terrain = terrains_pattern.get_terrain_peering_bit(bit);
								if (terrain >= 0 &&
									terrain <
										(int)per_terrain_terrains_patterns[terrain_set].size()) {
									per_terrain_terrains_patterns[terrain_set][terrain].insert(
										terrains_pattern);
								}
							}
						}
					}
				}
			}
		}
	}
}

void TileMapLayerEditorTerrainsPlugin::_update_theme()
{
	paint_tool_button->set_button_icon(main_box_container->get_editor_theme_icon(SNAME("Edit")));
	line_tool_button->set_button_icon(main_box_container->get_editor_theme_icon(SNAME("Line")));
	rect_tool_button->set_button_icon(
		main_box_container->get_editor_theme_icon(SNAME("Rectangle")));
	bucket_tool_button->set_button_icon(main_box_container->get_editor_theme_icon(SNAME("Bucket")));

	picker_button->set_button_icon(main_box_container->get_editor_theme_icon(SNAME("ColorPick")));
	erase_button->set_button_icon(main_box_container->get_editor_theme_icon(SNAME("Eraser")));

	_update_tiles_list();
}

void TileMapLayerEditorTerrainsPlugin::_update_translation() { _update_terrains_tree(); }

void TileMapLayerEditorTerrainsPlugin::update_layout(EditorDock::DockLayout p_layout, int p_slot)
{
	bool is_vertical = (p_layout == EditorDock::DockLayout::DOCK_LAYOUT_VERTICAL);
	// Main Panel.
	main_box_container->set_vertical(is_vertical);
	tilemap_tab_terrains->move_child(terrains_tree, is_vertical ? 1 : 0);
	tilemap_tab_terrains->set_vertical(is_vertical);

	// Toolbar.
	tilemap_tiles_tools_buttons->set_vertical(is_vertical);
	tools_settings->set_vertical(is_vertical);
	tools_settings_vsep->set_vertical(is_vertical);

	wide_toolbar->set_visible(is_vertical);
	bucket_contiguous_checkbox->reparent(is_vertical ? wide_toolbar : tools_settings);
}

void TileMapLayerEditor::_update_tile_map_layers_in_scene_list_cache()
{
	if (!layers_in_scene_list_cache_needs_update) {
		return;
	}
	EditorNode* en = EditorNode::get_singleton();
	Node* edited_scene_root = en->get_edited_scene();
	if (!edited_scene_root) {
		return;
	}

	tile_map_layers_in_scene_cache.clear();
	_find_tile_map_layers_in_scene(
		edited_scene_root, edited_scene_root, tile_map_layers_in_scene_cache);
	layers_in_scene_list_cache_needs_update = false;
}

void TileMapLayerEditor::_select_previous_layer_pressed()
{
	_layers_select_next_or_previous(false);
}

void TileMapLayerEditor::_select_next_layer_pressed() { _layers_select_next_or_previous(true); }

void TileMapLayerEditor::_update_bottom_panel()
{
	const TileMapLayer* edited_layer = _get_edited_layer();
	Ref<TileSet> tile_set;
	if (edited_layer) {
		tile_set = edited_layer->get_tile_set();
	}

	// Update state labels.
	if (is_multi_node_edit) {
		cant_edit_label->set_text(TTRC("Can't edit multiple layers at once."));
		cant_edit_label->show();
	}
	else if (!edited_layer) {
		cant_edit_label->set_text(TTRC("The selected TileMap has no layer to edit."));
		cant_edit_label->show();
	}
	else if (!edited_layer->is_enabled() || !edited_layer->is_visible_in_tree()) {
		cant_edit_label->set_text(TTRC("The edited layer is disabled or invisible"));
		cant_edit_label->show();
	}
	else if (tile_set.is_null()) {
		cant_edit_label->set_text(
			TTRC("The edited TileMap or TileMapLayer node has no TileSet resource.\nCreate or load "
				 "a TileSet resource in the Tile Set property in the inspector."));
		cant_edit_label->show();
	}
	else {
		cant_edit_label->hide();
	}

	// Update tabs visibility.
	for (int i = 0; i < int(tabs_data.size()); i++) {
		TileMapLayerSubEditorPlugin::TabData& tab_data = tabs_data[i];
		if (i == tabs_bar->get_current_tab()) {
			tab_data.panel->set_visible(!cant_edit_label->is_visible());
		}
		else {
			tab_data.panel->hide();
		}
	}
}

Vector<Vector2i> TileMapLayerEditor::get_line(
	const TileMapLayer* p_tile_map_layer, Vector2i p_from_cell, Vector2i p_to_cell)
{
	ERR_FAIL_NULL_V(p_tile_map_layer, Vector<Vector2i>());

	Ref<TileSet> tile_set = p_tile_map_layer->get_tile_set();
	ERR_FAIL_COND_V(tile_set.is_null(), Vector<Vector2i>());

	if (tile_set->get_tile_shape() == TileSet::TILE_SHAPE_SQUARE) {
		return Geometry2D::bresenham_line(p_from_cell, p_to_cell);
	}
	else {
		// Adapt the bresenham line algorithm to half-offset shapes.
		// See this blog post:
		// http://zvold.blogspot.com/2010/01/bresenhams-line-drawing-algorithm-on_26.html
		Vector<Point2i> points;

		bool transposed = tile_set->get_tile_offset_axis() == TileSet::TILE_OFFSET_AXIS_VERTICAL;
		p_from_cell =
			TileSet::transform_coords_layout(p_from_cell, tile_set->get_tile_offset_axis(),
				tile_set->get_tile_layout(), TileSet::TILE_LAYOUT_STACKED);
		p_to_cell = TileSet::transform_coords_layout(p_to_cell, tile_set->get_tile_offset_axis(),
			tile_set->get_tile_layout(), TileSet::TILE_LAYOUT_STACKED);
		if (transposed) {
			SWAP(p_from_cell.x, p_from_cell.y);
			SWAP(p_to_cell.x, p_to_cell.y);
		}

		Vector2i delta = p_to_cell - p_from_cell;
		delta = Vector2i(
			2 * delta.x + Math::abs(p_to_cell.y % 2) - Math::abs(p_from_cell.y % 2), delta.y);
		Vector2i sign = delta.sign();

		Vector2i current = p_from_cell;
		points.push_back(TileSet::transform_coords_layout(
			transposed ? Vector2i(current.y, current.x) : current, tile_set->get_tile_offset_axis(),
			TileSet::TILE_LAYOUT_STACKED, tile_set->get_tile_layout()));

		int err = 0;
		if (Math::abs(delta.y) < Math::abs(delta.x)) {
			Vector2i err_step = 3 * delta.abs();
			while (current != p_to_cell) {
				err += err_step.y;
				if (err > Math::abs(delta.x)) {
					if (sign.x == 0) {
						current += Vector2(sign.y, 0);
					}
					else {
						current +=
							Vector2(bool(current.y % 2) != (sign.x < 0) ? sign.x : 0, sign.y);
					}
					err -= err_step.x;
				}
				else {
					current += Vector2i(sign.x, 0);
					err += err_step.y;
				}
				points.push_back(TileSet::transform_coords_layout(
					transposed ? Vector2i(current.y, current.x) : current,
					tile_set->get_tile_offset_axis(), TileSet::TILE_LAYOUT_STACKED,
					tile_set->get_tile_layout()));
			}
		}
		else {
			Vector2i err_step = delta.abs();
			while (current != p_to_cell) {
				err += err_step.x;
				if (err > 0) {
					if (sign.x == 0) {
						current += Vector2(0, sign.y);
					}
					else {
						current +=
							Vector2(bool(current.y % 2) != (sign.x < 0) ? sign.x : 0, sign.y);
					}
					err -= err_step.y;
				}
				else {
					if (sign.x == 0) {
						current += Vector2(0, sign.y);
					}
					else {
						current +=
							Vector2(bool(current.y % 2) ^ (sign.x > 0) ? -sign.x : 0, sign.y);
					}
					err += err_step.y;
				}
				points.push_back(TileSet::transform_coords_layout(
					transposed ? Vector2i(current.y, current.x) : current,
					tile_set->get_tile_offset_axis(), TileSet::TILE_LAYOUT_STACKED,
					tile_set->get_tile_layout()));
			}
		}

		return points;
	}
}

void TileMapLayerEditor::_tile_map_layer_changed() { tile_map_layer_changed_needs_update = true; }

bool TileMapLayerEditor::forward_canvas_gui_input(const Ref<InputEvent>& p_event)
{
	if (ED_IS_SHORTCUT("tiles_editor/select_next_layer", p_event) && p_event->is_pressed()) {
		_layers_select_next_or_previous(true);
		return true;
	}

	if (ED_IS_SHORTCUT("tiles_editor/select_previous_layer", p_event) && p_event->is_pressed()) {
		_layers_select_next_or_previous(false);
		return true;
	}

	return tabs_plugins[tabs_bar->get_current_tab()]->forward_canvas_gui_input(p_event);
}

void TileMapLayerEditor::set_show_layer_selector(bool p_show_layer_selector)
{
	show_layers_selector = p_show_layer_selector;
	_update_layers_selector();
}

void TileMapLayerEditor::_update_layer_selector_layout(bool p_is_vertical)
{
	if (p_is_vertical && show_layers_selector) {
		layer_selection_hbox->reparent(tile_map_wide_toolbar);
		tile_map_wide_toolbar->move_child(layer_selection_hbox, 1);
		layer_selection_hbox->set_vertical(false);
	}
	else {
		layer_selection_hbox->reparent(tile_map_toolbar);
		tile_map_toolbar->move_child(layer_selection_hbox, -5);
		layer_selection_hbox->set_vertical(p_is_vertical);
	}
}

void TileMapLayerEditor::update_layout(DockLayout p_layout, int p_slot)
{
	bool is_vertical = (p_layout == EditorDock::DockLayout::DOCK_LAYOUT_VERTICAL);
	tile_map_toolbar->set_vertical(is_vertical);
	layer_selector_separator->set_vertical(is_vertical);
	tile_map_toolbar->set_h_size_flags(is_vertical ? SIZE_SHRINK_BEGIN : SIZE_EXPAND_FILL);
	tile_map_toolbar->set_v_size_flags(is_vertical ? SIZE_EXPAND_FILL : SIZE_SHRINK_BEGIN);

	main_box_container->move_child(padding_control, is_vertical ? 0 : 2);
	if (is_vertical) {
		main_box_container->remove_theme_constant_override(SNAME("h_separation"));
	}
	else {
		main_box_container->add_theme_constant_override(SNAME("h_separation"), 0);
	}

	if (is_vertical) {
		tabs_panel->reparent(tile_map_wide_toolbar);
		tile_map_wide_toolbar->move_child(tabs_panel, 0);
	}
	else {
		tabs_panel->reparent(tile_map_toolbar);
		tile_map_toolbar->move_child(tabs_panel, 0);
	}

	_update_layer_selector_layout(is_vertical);

	// Propagate layout change to sub plugins
	for (TileMapLayerSubEditorPlugin* tab_plugin : tabs_plugins) {
		tab_plugin->update_layout(p_layout, p_slot);
	}
}

TileMapLayerEditor::~TileMapLayerEditor()
{
	for (int i = 0; i < tile_map_editor_plugins.size(); i++) {
		memdelete(tile_map_editor_plugins[i]);
	}
}


