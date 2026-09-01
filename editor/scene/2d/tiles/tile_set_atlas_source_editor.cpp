/**************************************************************************/
/*  tile_set_atlas_source_editor.cpp                                      */
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
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_toaster.h"
#include "editor/inspector/editor_inspector.h"
#include "editor/scene/2d/tiles/tile_set_editor.h"
#include "editor/scene/2d/tiles/tiles_editor_plugin.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/control.h"
#include "servers/navigation_2d/navigation_server_2d.h"
#include "tile_set_atlas_source_editor.h"

int TileSetAtlasSourceEditor::TileSetAtlasSourceProxyObject::get_id() const { return source_id; }

// -- Proxy object used by the tile inspector --

void TileSetAtlasSourceEditor::_inspector_property_selected(const String& p_property)
{
	selected_property = p_property;
	_update_atlas_view();
	_update_current_tile_data_editor();
}

void TileSetAtlasSourceEditor::_update_tile_id_label()
{
	if (selection.size() == 1) {
		TileSelection selected = selection.front()->get();
		tool_tile_id_label->set_text(
			vformat("%d, %s, %d", tile_set_atlas_source_id, selected.tile, selected.alternative));
		tool_tile_id_label->set_tooltip_text(
			vformat(TTR("Selected tile:\nSource: %d\nAtlas coordinates: %s\nAlternative: %d"),
				tile_set_atlas_source_id, selected.tile, selected.alternative));
		tool_tile_id_label->show();
	}
	else {
		tool_tile_id_label->hide();
	}
}

void TileSetAtlasSourceEditor::_update_fix_selected_and_hovered_tiles()
{
	// Fix selected.
	for (RBSet<TileSelection>::Element* E = selection.front(); E;) {
		RBSet<TileSelection>::Element* N = E->next();
		TileSelection selected = E->get();
		if (!tile_set_atlas_source->has_tile(selected.tile) ||
			!tile_set_atlas_source->has_alternative_tile(selected.tile, selected.alternative)) {
			selection.erase(E);
		}
		E = N;
	}

	// Fix hovered.
	if (!tile_set_atlas_source->has_tile(hovered_base_tile_coords)) {
		hovered_base_tile_coords = TileSetSource::INVALID_ATLAS_COORDS;
	}
	Vector2i coords =
		Vector2i(hovered_alternative_tile_coords.x, hovered_alternative_tile_coords.y);
	int alternative = hovered_alternative_tile_coords.z;
	if (!tile_set_atlas_source->has_tile(coords) ||
		!tile_set_atlas_source->has_alternative_tile(coords, alternative)) {
		hovered_alternative_tile_coords = Vector3i(TileSetSource::INVALID_ATLAS_COORDS.x,
			TileSetSource::INVALID_ATLAS_COORDS.y, TileSetSource::INVALID_TILE_ALTERNATIVE);
	}
}

void TileSetAtlasSourceEditor::_update_atlas_source_inspector()
{
	// Update visibility.
	bool inspector_visible =
		tools_button_group->get_pressed_button() == tool_setup_atlas_source_button;
	atlas_source_inspector->set_visible(inspector_visible);
	atlas_source_inspector->set_read_only(read_only);
}

void TileSetAtlasSourceEditor::_tile_data_editor_dropdown_button_draw()
{
	if (!has_theme_icon(SNAME("arrow"), SNAME("OptionButton"))) {
		return;
	}

	RID ci = tile_data_editor_dropdown_button->get_canvas_item();
	Ref<Texture2D> arrow = Control::get_theme_icon(SNAME("arrow"), SNAME("OptionButton"));
	Color clr = Color(1, 1, 1);
	if (get_theme_constant(SNAME("modulate_arrow"))) {
		switch (tile_data_editor_dropdown_button->get_draw_mode()) {
		case BaseButton::DRAW_PRESSED:
			clr = get_theme_color(SNAME("font_pressed_color"));
			break;
		case BaseButton::DRAW_HOVER:
			clr = get_theme_color(SNAME("font_hover_color"));
			break;
		case BaseButton::DRAW_DISABLED:
			clr = get_theme_color(SNAME("font_disabled_color"));
			break;
		default:
			if (tile_data_editor_dropdown_button->has_focus()) {
				clr = get_theme_color(SNAME("font_focus_color"));
			}
			else {
				clr = get_theme_color(SceneStringName(font_color));
			}
		}
	}

	Size2 size = tile_data_editor_dropdown_button->get_size();

	Point2 ofs;
	if (is_layout_rtl()) {
		ofs = Point2(get_theme_constant(SNAME("arrow_margin"), SNAME("OptionButton")),
			int(Math::abs((size.height - arrow->get_height()) / 2)));
	}
	else {
		ofs = Point2(size.width - arrow->get_width() -
						 get_theme_constant(SNAME("arrow_margin"), SNAME("OptionButton")),
			int(Math::abs((size.height - arrow->get_height()) / 2)));
	}
	arrow->draw(ci, ofs, clr);
}

void TileSetAtlasSourceEditor::_tile_data_editor_dropdown_button_pressed()
{
	Size2 size = tile_data_editor_dropdown_button->get_size();
	tile_data_editors_popup->set_position(
		tile_data_editor_dropdown_button->get_screen_position() +
		Size2(0, size.height * get_global_transform().get_scale().y));
	tile_data_editors_popup->set_size(Size2(size.width, 0));
	tile_data_editors_popup->popup();
}

void TileSetAtlasSourceEditor::_update_toolbar()
{
	// Show the tools and settings.
	Control* current_tile_data_editor_toolbar = nullptr;
	if (current_tile_data_editor) {
		current_tile_data_editor_toolbar = current_tile_data_editor->get_toolbar();
	}
	if (tools_button_group->get_pressed_button() == tool_setup_atlas_source_button) {
		if (current_tile_data_editor_toolbar) {
			current_tile_data_editor_toolbar->hide();
		}
		tools_settings_erase_button->show();
		tool_advanced_menu_button->show();
	}
	else if (tools_button_group->get_pressed_button() == tool_select_button) {
		if (current_tile_data_editor_toolbar) {
			current_tile_data_editor_toolbar->hide();
		}
		tools_settings_erase_button->hide();
		tool_advanced_menu_button->hide();
	}
	else if (tools_button_group->get_pressed_button() == tool_paint_button) {
		if (current_tile_data_editor_toolbar) {
			current_tile_data_editor_toolbar->show();
		}
		tools_settings_erase_button->hide();
		tool_advanced_menu_button->hide();
	}
}

void TileSetAtlasSourceEditor::_update_buttons()
{
	tool_paint_button->set_disabled(read_only);
	tool_paint_button->set_tooltip_text(read_only
											? TTRC("TileSet is in read-only mode. Make the "
												   "resource unique to edit TileSet properties.")
											: TTRC("Paint properties."));
	tools_settings_erase_button->set_disabled(read_only);
	tool_advanced_menu_button->set_disabled(read_only);
}

void TileSetAtlasSourceEditor::_tile_atlas_control_mouse_exited()
{
	hovered_base_tile_coords = TileSetSource::INVALID_ATLAS_COORDS;
	tile_atlas_control->queue_redraw();
	tile_atlas_control_unscaled->queue_redraw();
	tile_atlas_view->queue_redraw();
}

void TileSetAtlasSourceEditor::_tile_atlas_view_transform_changed()
{
	tile_atlas_control->queue_redraw();
	tile_atlas_control_unscaled->queue_redraw();
}

void TileSetAtlasSourceEditor::_tile_atlas_control_gui_input(const Ref<InputEvent>& p_event)
{
	// Update the hovered coords.
	hovered_base_tile_coords = tile_atlas_view->get_atlas_tile_coords_at_pos(
		tile_atlas_control->get_local_mouse_position());

	// Forward the event to the current tile data editor if we are in the painting mode.
	if (tools_button_group->get_pressed_button() == tool_paint_button) {
		if (current_tile_data_editor) {
			current_tile_data_editor->forward_painting_atlas_gui_input(
				tile_atlas_view, tile_set_atlas_source, p_event);
		}
		// Update only what's needed.
		tile_set_changed_needs_update = false;

		tile_atlas_control->queue_redraw();
		tile_atlas_control_unscaled->queue_redraw();
		alternative_tiles_control->queue_redraw();
		alternative_tiles_control_unscaled->queue_redraw();
		tile_atlas_view->queue_redraw();
		return;
	}
	else {
		// Handle the event.
		Ref<InputEventMouseMotion> mm = p_event;
		if (mm.is_valid()) {
			Vector2i start_base_tiles_coords =
				tile_atlas_view->get_atlas_tile_coords_at_pos(drag_start_mouse_pos, true);
			Vector2i last_base_tiles_coords =
				tile_atlas_view->get_atlas_tile_coords_at_pos(drag_last_mouse_pos, true);
			Vector2i new_base_tiles_coords = tile_atlas_view->get_atlas_tile_coords_at_pos(
				tile_atlas_control->get_local_mouse_position());

			Vector2i grid_size = tile_set_atlas_source->get_atlas_grid_size();

			if (drag_type == DRAG_TYPE_CREATE_BIG_TILE) {
				// Create big tile.
				new_base_tiles_coords =
					new_base_tiles_coords.maxi(0).min(grid_size - Vector2i(1, 1));

				Rect2i new_rect =
					Rect2i(start_base_tiles_coords, new_base_tiles_coords - start_base_tiles_coords)
						.abs();
				new_rect.size += Vector2i(1, 1);
				// Check if the new tile can fit in the new rect.
				if (tile_set_atlas_source->has_room_for_tile(new_rect.position, new_rect.size,
						tile_set_atlas_source->get_tile_animation_columns(drag_current_tile),
						tile_set_atlas_source->get_tile_animation_separation(drag_current_tile),
						tile_set_atlas_source->get_tile_animation_frames_count(drag_current_tile),
						drag_current_tile)) {
					// Move and resize the tile.
					tile_set_atlas_source->move_tile_in_atlas(
						drag_current_tile, new_rect.position, new_rect.size);
					drag_current_tile = new_rect.position;
				}
			}
			else if (drag_type == DRAG_TYPE_CREATE_TILES) {
				// Create tiles.
				last_base_tiles_coords =
					last_base_tiles_coords.maxi(0).min(grid_size - Vector2i(1, 1));
				new_base_tiles_coords =
					new_base_tiles_coords.maxi(0).min(grid_size - Vector2i(1, 1));

				Vector<Point2i> line =
					Geometry2D::bresenham_line(last_base_tiles_coords, new_base_tiles_coords);
				for (int i = 0; i < line.size(); i++) {
					if (tile_set_atlas_source->get_tile_at_coords(line[i]) ==
						TileSetSource::INVALID_ATLAS_COORDS) {
						tile_set_atlas_source->create_tile(line[i]);
						drag_modified_tiles.insert(line[i]);
					}
				}

				drag_last_mouse_pos = tile_atlas_control->get_local_mouse_position();

			}
			else if (drag_type == DRAG_TYPE_REMOVE_TILES) {
				// Remove tiles.
				last_base_tiles_coords =
					last_base_tiles_coords.maxi(0).min(grid_size - Vector2i(1, 1));
				new_base_tiles_coords =
					new_base_tiles_coords.maxi(0).min(grid_size - Vector2i(1, 1));

				Vector<Point2i> line =
					Geometry2D::bresenham_line(last_base_tiles_coords, new_base_tiles_coords);
				for (int i = 0; i < line.size(); i++) {
					Vector2i base_tile_coords = tile_set_atlas_source->get_tile_at_coords(line[i]);
					if (base_tile_coords != TileSetSource::INVALID_ATLAS_COORDS) {
						drag_modified_tiles.insert(base_tile_coords);
					}
				}

				drag_last_mouse_pos = tile_atlas_control->get_local_mouse_position();
			}
			else if (drag_type == DRAG_TYPE_MOVE_TILE) {
				// Move tile.
				Vector2 mouse_offset =
					(Vector2(tile_set_atlas_source->get_tile_size_in_atlas(drag_current_tile)) /
							2.0 -
						Vector2(0.5, 0.5)) *
					tile_set->get_tile_size();
				Vector2i coords = tile_atlas_view->get_atlas_tile_coords_at_pos(
					tile_atlas_control->get_local_mouse_position() - mouse_offset);
				if (drag_current_tile != coords &&
					tile_set_atlas_source->has_room_for_tile(coords,
						tile_set_atlas_source->get_tile_size_in_atlas(drag_current_tile),
						tile_set_atlas_source->get_tile_animation_columns(drag_current_tile),
						tile_set_atlas_source->get_tile_animation_separation(drag_current_tile),
						tile_set_atlas_source->get_tile_animation_frames_count(drag_current_tile),
						drag_current_tile)) {
					tile_set_atlas_source->move_tile_in_atlas(drag_current_tile, coords);
					selection.clear();
					selection.insert({coords, 0});
					drag_current_tile = coords;

					// Update only what's needed.
					tile_set_changed_needs_update = false;
					_update_tile_inspector();
					_update_atlas_view();
					_update_tile_id_label();
					_update_current_tile_data_editor();
				}
			}
			else if (drag_type == DRAG_TYPE_MAY_POPUP_MENU) {
				if (Vector2(drag_start_mouse_pos)
						.distance_to(tile_atlas_control->get_local_mouse_position()) >
					5.0 * EDSCALE) {
					drag_type = DRAG_TYPE_NONE;
				}
			}
			else if (drag_type >= DRAG_TYPE_RESIZE_TOP_LEFT &&
					   drag_type <= DRAG_TYPE_RESIZE_LEFT) {
				// Resizing a tile.
				Rect2i old_rect = Rect2i(drag_current_tile,
					tile_set_atlas_source->get_tile_size_in_atlas(drag_current_tile));
				Rect2i new_rect = old_rect;

				if (drag_type == DRAG_TYPE_RESIZE_LEFT || drag_type == DRAG_TYPE_RESIZE_TOP_LEFT ||
					drag_type == DRAG_TYPE_RESIZE_BOTTOM_LEFT) {
					new_base_tiles_coords = _get_drag_offset_tile_coords(Vector2i(-1, 0));
					new_rect.position.x =
						MIN(new_base_tiles_coords.x + 1, old_rect.get_end().x - 1);
					new_rect.size.x = old_rect.get_end().x - new_rect.position.x;
				}
				if (drag_type == DRAG_TYPE_RESIZE_TOP || drag_type == DRAG_TYPE_RESIZE_TOP_LEFT ||
					drag_type == DRAG_TYPE_RESIZE_TOP_RIGHT) {
					new_base_tiles_coords = _get_drag_offset_tile_coords(Vector2i(0, -1));
					new_rect.position.y =
						MIN(new_base_tiles_coords.y + 1, old_rect.get_end().y - 1);
					new_rect.size.y = old_rect.get_end().y - new_rect.position.y;
				}

				if (drag_type == DRAG_TYPE_RESIZE_RIGHT ||
					drag_type == DRAG_TYPE_RESIZE_TOP_RIGHT ||
					drag_type == DRAG_TYPE_RESIZE_BOTTOM_RIGHT) {
					new_base_tiles_coords = _get_drag_offset_tile_coords(Vector2i(1, 0));
					new_rect.set_end(Vector2i(MAX(new_base_tiles_coords.x, old_rect.position.x + 1),
						new_rect.get_end().y));
				}
				if (drag_type == DRAG_TYPE_RESIZE_BOTTOM ||
					drag_type == DRAG_TYPE_RESIZE_BOTTOM_LEFT ||
					drag_type == DRAG_TYPE_RESIZE_BOTTOM_RIGHT) {
					new_base_tiles_coords = _get_drag_offset_tile_coords(Vector2i(0, 1));
					new_rect.set_end(Vector2i(new_rect.get_end().x,
						MAX(new_base_tiles_coords.y, old_rect.position.y + 1)));
				}

				if (tile_set_atlas_source->has_room_for_tile(new_rect.position, new_rect.size,
						tile_set_atlas_source->get_tile_animation_columns(drag_current_tile),
						tile_set_atlas_source->get_tile_animation_separation(drag_current_tile),
						tile_set_atlas_source->get_tile_animation_frames_count(drag_current_tile),
						drag_current_tile)) {
					tile_set_atlas_source->move_tile_in_atlas(
						drag_current_tile, new_rect.position, new_rect.size);
					selection.clear();
					selection.insert({new_rect.position, 0});
					drag_current_tile = new_rect.position;

					// Update only what's needed.
					tile_set_changed_needs_update = false;
					_update_tile_inspector();
					_update_atlas_view();
					_update_tile_id_label();
					_update_current_tile_data_editor();
				}
			}

			// Redraw for the hovered tile.
			tile_atlas_control->queue_redraw();
			tile_atlas_control_unscaled->queue_redraw();
			alternative_tiles_control->queue_redraw();
			alternative_tiles_control_unscaled->queue_redraw();
			tile_atlas_view->queue_redraw();
			return;
		}

		Ref<InputEventMouseButton> mb = p_event;
		if (mb.is_valid()) {
			Vector2 mouse_local_pos = tile_atlas_control->get_local_mouse_position();
			if (mb->get_button_index() == MouseButton::LEFT) {
				if (mb->is_pressed()) {
					// Left click pressed.
					if (tools_button_group->get_pressed_button() ==
						tool_setup_atlas_source_button) {
						if (tools_settings_erase_button->is_pressed()) {
							// Erasing
							if (mb->is_command_or_control_pressed() || mb->is_shift_pressed()) {
								// Remove tiles using rect.

								// Setup the dragging info.
								drag_type = DRAG_TYPE_REMOVE_TILES_USING_RECT;
								drag_start_mouse_pos = mouse_local_pos;
								drag_last_mouse_pos = drag_start_mouse_pos;
							}
							else {
								// Remove tiles.

								// Setup the dragging info.
								drag_type = DRAG_TYPE_REMOVE_TILES;
								drag_start_mouse_pos = mouse_local_pos;
								drag_last_mouse_pos = drag_start_mouse_pos;

								// Remove a first tile.
								Vector2i coords = tile_atlas_view->get_atlas_tile_coords_at_pos(
									drag_start_mouse_pos);
								if (coords != TileSetSource::INVALID_ATLAS_COORDS) {
									coords = tile_set_atlas_source->get_tile_at_coords(coords);
								}
								if (coords != TileSetSource::INVALID_ATLAS_COORDS) {
									drag_modified_tiles.insert(coords);
								}
							}
						}
						else {
							// Creating
							if (mb->is_shift_pressed()) {
								// Create a big tile.
								Vector2i coords =
									tile_atlas_view->get_atlas_tile_coords_at_pos(mouse_local_pos);
								if (coords != TileSetSource::INVALID_ATLAS_COORDS &&
									tile_set_atlas_source->get_tile_at_coords(coords) ==
										TileSetSource::INVALID_ATLAS_COORDS) {
									// Setup the dragging info, only if we start on an empty tile.
									drag_type = DRAG_TYPE_CREATE_BIG_TILE;
									drag_start_mouse_pos = mouse_local_pos;
									drag_last_mouse_pos = drag_start_mouse_pos;
									drag_current_tile = coords;

									// Create a tile.
									tile_set_atlas_source->create_tile(coords);
								}
							}
							else if (mb->is_command_or_control_pressed()) {
								// Create tiles using rect.
								drag_type = DRAG_TYPE_CREATE_TILES_USING_RECT;
								drag_start_mouse_pos = mouse_local_pos;
								drag_last_mouse_pos = drag_start_mouse_pos;
							}
							else {
								// Create tiles.

								// Setup the dragging info.
								drag_type = DRAG_TYPE_CREATE_TILES;
								drag_start_mouse_pos = mouse_local_pos;
								drag_last_mouse_pos = drag_start_mouse_pos;

								// Create a first tile if needed.
								Vector2i coords = tile_atlas_view->get_atlas_tile_coords_at_pos(
									drag_start_mouse_pos);
								if (coords != TileSetSource::INVALID_ATLAS_COORDS &&
									tile_set_atlas_source->get_tile_at_coords(coords) ==
										TileSetSource::INVALID_ATLAS_COORDS) {
									tile_set_atlas_source->create_tile(coords);
									drag_modified_tiles.insert(coords);
								}
							}
						}
					}
					else if (tools_button_group->get_pressed_button() == tool_select_button) {
						// Dragging a handle.
						drag_type = DRAG_TYPE_NONE;
						if (selection.size() == 1) {
							TileSelection selected = selection.front()->get();
							if (selected.tile != TileSetSource::INVALID_ATLAS_COORDS &&
								selected.alternative == 0) {
								Vector2i size_in_atlas =
									tile_set_atlas_source->get_tile_size_in_atlas(selected.tile);
								Rect2 region =
									tile_set_atlas_source->get_tile_texture_region(selected.tile);
								Size2 zoomed_size =
									resize_handle->get_size() / tile_atlas_view->get_zoom();
								Rect2 rect =
									region.grow_individual(zoomed_size.x, zoomed_size.y, 0, 0);
								const Vector2i coords[] = {
									Vector2i(0, 0), Vector2i(1, 0), Vector2i(1, 1), Vector2i(0, 1)};
								const Vector2i directions[] = {Vector2i(0, -1), Vector2i(1, 0),
									Vector2i(0, 1), Vector2i(-1, 0)};
								bool can_grow[4];
								for (int i = 0; i < 4; i++) {
									can_grow[i] = tile_set_atlas_source->has_room_for_tile(
										selected.tile + directions[i],
										tile_set_atlas_source->get_tile_size_in_atlas(
											selected.tile),
										tile_set_atlas_source->get_tile_animation_columns(
											selected.tile),
										tile_set_atlas_source->get_tile_animation_separation(
											selected.tile),
										tile_set_atlas_source->get_tile_animation_frames_count(
											selected.tile),
										selected.tile);
									can_grow[i] |=
										(i % 2 == 0) ? size_in_atlas.y > 1 : size_in_atlas.x > 1;
								}
								for (int i = 0; i < 4; i++) {
									Vector2 pos = rect.position + rect.size * coords[i];
									if (can_grow[i] && can_grow[(i + 3) % 4] &&
										Rect2(pos, zoomed_size).has_point(mouse_local_pos)) {
										drag_type =
											(DragType)((int)DRAG_TYPE_RESIZE_TOP_LEFT + i * 2);
										drag_start_mouse_pos = mouse_local_pos;
										drag_last_mouse_pos = drag_start_mouse_pos;
										drag_current_tile = selected.tile;
										drag_start_tile_shape = Rect2i(selected.tile,
											tile_set_atlas_source->get_tile_size_in_atlas(
												selected.tile));
									}
									Vector2 next_pos =
										rect.position + rect.size * coords[(i + 1) % 4];
									if (can_grow[i] && Rect2((pos + next_pos) / 2.0, zoomed_size)
														   .has_point(mouse_local_pos)) {
										drag_type = (DragType)((int)DRAG_TYPE_RESIZE_TOP + i * 2);
										drag_start_mouse_pos = mouse_local_pos;
										drag_last_mouse_pos = drag_start_mouse_pos;
										drag_current_tile = selected.tile;
										drag_start_tile_shape = Rect2i(selected.tile,
											tile_set_atlas_source->get_tile_size_in_atlas(
												selected.tile));
									}
								}
							}
						}

						// Selecting then dragging a tile.
						if (drag_type == DRAG_TYPE_NONE) {
							TileSelection selected = {TileSetSource::INVALID_ATLAS_COORDS,
								TileSetSource::INVALID_TILE_ALTERNATIVE};
							Vector2i coords =
								tile_atlas_view->get_atlas_tile_coords_at_pos(mouse_local_pos);
							if (coords != TileSetSource::INVALID_ATLAS_COORDS) {
								coords = tile_set_atlas_source->get_tile_at_coords(coords);
								if (coords != TileSetSource::INVALID_ATLAS_COORDS) {
									selected = {coords, 0};
								}
							}

							bool shift = mb->is_shift_pressed();
							if (!shift && selection.size() == 1 &&
								selected.tile != TileSetSource::INVALID_ATLAS_COORDS &&
								selection.has(selected)) {
								// Start move dragging.
								drag_type = DRAG_TYPE_MOVE_TILE;
								drag_start_mouse_pos = mouse_local_pos;
								drag_last_mouse_pos = drag_start_mouse_pos;
								drag_current_tile = selected.tile;
								drag_start_tile_shape = Rect2i(selected.tile,
									tile_set_atlas_source->get_tile_size_in_atlas(selected.tile));
							}
							else {
								// Start selection dragging.
								drag_type = DRAG_TYPE_RECT_SELECT;
								drag_start_mouse_pos = mouse_local_pos;

								drag_last_mouse_pos = drag_start_mouse_pos;
							}
						}
					}
				}
				else {
					// Left click released.
					_end_dragging();
				}
				tile_atlas_control->queue_redraw();
				tile_atlas_control_unscaled->queue_redraw();
				alternative_tiles_control->queue_redraw();
				alternative_tiles_control_unscaled->queue_redraw();
				tile_atlas_view->queue_redraw();
				return;
			}
			else if (mb->get_button_index() == MouseButton::RIGHT) {
				// Right click pressed.
				if (mb->is_pressed()) {
					drag_type = DRAG_TYPE_MAY_POPUP_MENU;
					drag_start_mouse_pos = tile_atlas_control->get_local_mouse_position();
				}
				else {
					// Right click released.
					_end_dragging();
				}
				tile_atlas_control->queue_redraw();
				tile_atlas_control_unscaled->queue_redraw();
				alternative_tiles_control->queue_redraw();
				alternative_tiles_control_unscaled->queue_redraw();
				tile_atlas_view->queue_redraw();
				return;
			}
		}
	}
}

void TileSetAtlasSourceEditor::_tile_atlas_control_draw()
{
	// Draw the selected tile.
	if (tools_button_group->get_pressed_button() == tool_select_button) {
		for (const TileSelection& E : selection) {
			TileSelection selected = E;
			if (selected.alternative == 0) {
				// Draw the rect.
				for (int frame = 0;
					 frame < tile_set_atlas_source->get_tile_animation_frames_count(selected.tile);
					 frame++) {
					Color color = Color(0.0, 1.0, 0.0, frame == 0 ? 1.0 : 0.3);
					Rect2 region =
						tile_set_atlas_source->get_tile_texture_region(selected.tile, frame);
					TilesEditorUtils::draw_selection_rect(tile_atlas_control, region, color);
				}
			}
		}

		if (selection.size() == 1) {
			// Draw the resize handles (only when it's possible to expand).
			TileSelection selected = selection.front()->get();
			if (selected.alternative == 0) {
				Vector2i size_in_atlas =
					tile_set_atlas_source->get_tile_size_in_atlas(selected.tile);
				Size2 zoomed_size = resize_handle->get_size() / tile_atlas_view->get_zoom();
				Rect2 region = tile_set_atlas_source->get_tile_texture_region(selected.tile);
				Rect2 rect = region.grow_individual(zoomed_size.x, zoomed_size.y, 0, 0);
				const Vector2i coords[] = {
					Vector2i(0, 0), Vector2i(1, 0), Vector2i(1, 1), Vector2i(0, 1)};
				const Vector2i directions[] = {
					Vector2i(0, -1), Vector2i(1, 0), Vector2i(0, 1), Vector2i(-1, 0)};
				bool can_grow[4];
				for (int i = 0; i < 4; i++) {
					can_grow[i] =
						tile_set_atlas_source->has_room_for_tile(selected.tile + directions[i],
							tile_set_atlas_source->get_tile_size_in_atlas(selected.tile),
							tile_set_atlas_source->get_tile_animation_columns(selected.tile),
							tile_set_atlas_source->get_tile_animation_separation(selected.tile),
							tile_set_atlas_source->get_tile_animation_frames_count(selected.tile),
							selected.tile);
					can_grow[i] |= (i % 2 == 0) ? size_in_atlas.y > 1 : size_in_atlas.x > 1;
				}
				for (int i = 0; i < 4; i++) {
					Vector2 pos = rect.position + rect.size * coords[i];
					if (can_grow[i] && can_grow[(i + 3) % 4]) {
						tile_atlas_control->draw_texture_rect(
							resize_handle.ptr(), Rect2(pos, zoomed_size), false);
					}
					else {
						tile_atlas_control->draw_texture_rect(
							resize_handle_disabled.ptr(), Rect2(pos, zoomed_size), false);
					}
					Vector2 next_pos = rect.position + rect.size * coords[(i + 1) % 4];
					if (can_grow[i]) {
						tile_atlas_control->draw_texture_rect(
							resize_handle.ptr(), Rect2((pos + next_pos) / 2.0, zoomed_size), false);
					}
					else {
						tile_atlas_control->draw_texture_rect(resize_handle_disabled.ptr(),
							Rect2((pos + next_pos) / 2.0, zoomed_size), false);
					}
				}
			}
		}
	}

	if (drag_type == DRAG_TYPE_REMOVE_TILES) {
		// Draw the tiles to be removed.
		for (const Vector2i& E : drag_modified_tiles) {
			for (int frame = 0; frame < tile_set_atlas_source->get_tile_animation_frames_count(E);
				 frame++) {
				TilesEditorUtils::draw_selection_rect(tile_atlas_control,
					tile_set_atlas_source->get_tile_texture_region(E, frame), Color(0.0, 0.0, 0.0));
			}
		}
	}
	else if (drag_type == DRAG_TYPE_RECT_SELECT ||
			   drag_type == DRAG_TYPE_REMOVE_TILES_USING_RECT) {
		// Draw tiles to be removed.
		Vector2i start_base_tiles_coords =
			tile_atlas_view->get_atlas_tile_coords_at_pos(drag_start_mouse_pos, true);
		Vector2i new_base_tiles_coords = tile_atlas_view->get_atlas_tile_coords_at_pos(
			tile_atlas_control->get_local_mouse_position(), true);
		Rect2i area =
			Rect2i(start_base_tiles_coords, new_base_tiles_coords - start_base_tiles_coords).abs();
		area.set_end(
			(area.get_end() + Vector2i(1, 1)).min(tile_set_atlas_source->get_atlas_grid_size()));

		Color color = Color(0.0, 0.0, 0.0);
		if (drag_type == DRAG_TYPE_RECT_SELECT) {
			color = Color(1.0, 1.0, 0.0);
		}

		RBSet<Vector2i> to_paint;
		for (int x = area.get_position().x; x < area.get_end().x; x++) {
			for (int y = area.get_position().y; y < area.get_end().y; y++) {
				Vector2i coords = tile_set_atlas_source->get_tile_at_coords(Vector2i(x, y));
				if (coords != TileSetSource::INVALID_ATLAS_COORDS) {
					to_paint.insert(coords);
				}
			}
		}

		for (const Vector2i& E : to_paint) {
			Vector2i coords = E;
			TilesEditorUtils::draw_selection_rect(
				tile_atlas_control, tile_set_atlas_source->get_tile_texture_region(coords), color);
		}
	}
	else if (drag_type == DRAG_TYPE_CREATE_TILES_USING_RECT) {
		// Draw tiles to be created.
		Vector2i margins = tile_set_atlas_source->get_margins();
		Vector2i separation = tile_set_atlas_source->get_separation();
		Vector2i tile_size = tile_set_atlas_source->get_texture_region_size();

		Vector2i start_base_tiles_coords =
			tile_atlas_view->get_atlas_tile_coords_at_pos(drag_start_mouse_pos, true);
		Vector2i new_base_tiles_coords = tile_atlas_view->get_atlas_tile_coords_at_pos(
			tile_atlas_control->get_local_mouse_position(), true);
		Rect2i area =
			Rect2i(start_base_tiles_coords, new_base_tiles_coords - start_base_tiles_coords).abs();
		area.set_end(
			(area.get_end() + Vector2i(1, 1)).min(tile_set_atlas_source->get_atlas_grid_size()));
		for (int x = area.get_position().x; x < area.get_end().x; x++) {
			for (int y = area.get_position().y; y < area.get_end().y; y++) {
				Vector2i coords = Vector2i(x, y);
				if (tile_set_atlas_source->get_tile_at_coords(coords) ==
					TileSetSource::INVALID_ATLAS_COORDS) {
					Vector2i origin = margins + (coords * (tile_size + separation));
					TilesEditorUtils::draw_selection_rect(
						tile_atlas_control, Rect2i(origin, tile_size));
				}
			}
		}
	}

	// Draw the hovered tile.
	if (drag_type == DRAG_TYPE_REMOVE_TILES_USING_RECT ||
		drag_type == DRAG_TYPE_CREATE_TILES_USING_RECT) {
		// Draw the rect.
		Vector2i start_base_tiles_coords =
			tile_atlas_view->get_atlas_tile_coords_at_pos(drag_start_mouse_pos, true);
		Vector2i new_base_tiles_coords = tile_atlas_view->get_atlas_tile_coords_at_pos(
			tile_atlas_control->get_local_mouse_position(), true);
		Rect2i area =
			Rect2i(start_base_tiles_coords, new_base_tiles_coords - start_base_tiles_coords).abs();
		area.set_end(
			(area.get_end() + Vector2i(1, 1)).min(tile_set_atlas_source->get_atlas_grid_size()));
		Vector2i margins = tile_set_atlas_source->get_margins();
		Vector2i separation = tile_set_atlas_source->get_separation();
		Vector2i tile_size = tile_set_atlas_source->get_texture_region_size();
		Vector2i origin = margins + (area.position * (tile_size + separation));
		Vector2i size = area.size * tile_size + (area.size - Vector2i(1, 1)).maxi(0) * separation;
		TilesEditorUtils::draw_selection_rect(tile_atlas_control, Rect2i(origin, size));
	}
	else {
		Vector2i grid_size = tile_set_atlas_source->get_atlas_grid_size();
		if (hovered_base_tile_coords.x >= 0 && hovered_base_tile_coords.y >= 0 &&
			hovered_base_tile_coords.x < grid_size.x && hovered_base_tile_coords.y < grid_size.y) {
			Vector2i hovered_tile =
				tile_set_atlas_source->get_tile_at_coords(hovered_base_tile_coords);
			if (hovered_tile != TileSetSource::INVALID_ATLAS_COORDS) {
				// Draw existing hovered tile.
				for (int frame = 0;
					 frame < tile_set_atlas_source->get_tile_animation_frames_count(hovered_tile);
					 frame++) {
					Color color = Color(1.0, 0.8, 0.0, frame == 0 ? 0.6 : 0.3);
					TilesEditorUtils::draw_selection_rect(tile_atlas_control,
						tile_set_atlas_source->get_tile_texture_region(hovered_tile, frame), color);
				}
			}
			else {
				// Draw empty tile, only in add/remove tiles mode.
				if (tools_button_group->get_pressed_button() == tool_setup_atlas_source_button) {
					Vector2i margins = tile_set_atlas_source->get_margins();
					Vector2i separation = tile_set_atlas_source->get_separation();
					Vector2i tile_size = tile_set_atlas_source->get_texture_region_size();
					Vector2i origin =
						margins + (hovered_base_tile_coords * (tile_size + separation));
					TilesEditorUtils::draw_selection_rect(
						tile_atlas_control, Rect2i(origin, tile_size));
				}
			}
		}
	}
}

void TileSetAtlasSourceEditor::_tile_atlas_control_unscaled_draw()
{
	if (current_tile_data_editor) {
		// Draw the preview of the selected property.
		for (int i = 0; i < tile_set_atlas_source->get_tiles_count(); i++) {
			Vector2i coords = tile_set_atlas_source->get_tile_id(i);
			Rect2i texture_region = tile_set_atlas_source->get_tile_texture_region(coords);
			Vector2 position =
				((Rect2)texture_region).get_center() +
				tile_set_atlas_source->get_tile_data(coords, 0)->get_texture_origin();

			Transform2D xform = tile_atlas_control->get_parent_control()->get_transform();
			xform.translate_local(position);

			if (tools_button_group->get_pressed_button() == tool_select_button &&
				selection.has({coords, 0})) {
				continue;
			}

			TileMapCell cell;
			cell.source_id = tile_set_atlas_source_id;
			cell.set_atlas_coords(coords);
			cell.alternative_tile = 0;
			current_tile_data_editor->draw_over_tile(tile_atlas_control_unscaled, xform, cell);
		}

		// Draw the selection on top of other.
		if (tools_button_group->get_pressed_button() == tool_select_button) {
			for (const TileSelection& E : selection) {
				if (E.alternative != 0) {
					continue;
				}
				Rect2i texture_region = tile_set_atlas_source->get_tile_texture_region(E.tile);
				Vector2 position =
					((Rect2)texture_region).get_center() +
					tile_set_atlas_source->get_tile_data(E.tile, 0)->get_texture_origin();

				Transform2D xform = tile_atlas_control->get_parent_control()->get_transform();
				xform.translate_local(position);

				TileMapCell cell;
				cell.source_id = tile_set_atlas_source_id;
				cell.set_atlas_coords(E.tile);
				cell.alternative_tile = 0;
				current_tile_data_editor->draw_over_tile(
					tile_atlas_control_unscaled, xform, cell, true);
			}
		}

		// Call the TileData's editor custom draw function.
		if (tools_button_group->get_pressed_button() == tool_paint_button) {
			Transform2D xform = tile_atlas_control->get_parent_control()->get_transform();
			current_tile_data_editor->forward_draw_over_atlas(
				tile_atlas_view, tile_set_atlas_source, tile_atlas_control_unscaled, xform);
		}
	}
}

void TileSetAtlasSourceEditor::_tile_alternatives_control_gui_input(const Ref<InputEvent>& p_event)
{
	// Update the hovered alternative tile.
	hovered_alternative_tile_coords = tile_atlas_view->get_alternative_tile_at_pos(
		alternative_tiles_control->get_local_mouse_position());

	// Forward the event to the current tile data editor if we are in the painting mode.
	if (tools_button_group->get_pressed_button() == tool_paint_button) {
		if (current_tile_data_editor) {
			current_tile_data_editor->forward_painting_alternatives_gui_input(
				tile_atlas_view, tile_set_atlas_source, p_event);
		}
		tile_atlas_control->queue_redraw();
		tile_atlas_control_unscaled->queue_redraw();
		alternative_tiles_control->queue_redraw();
		alternative_tiles_control_unscaled->queue_redraw();
		tile_atlas_view->queue_redraw();
		return;
	}

	Ref<InputEventMouseMotion> mm = p_event;
	if (mm.is_valid()) {
		tile_atlas_control->queue_redraw();
		tile_atlas_control_unscaled->queue_redraw();
		alternative_tiles_control->queue_redraw();
		alternative_tiles_control_unscaled->queue_redraw();

		if (drag_type == DRAG_TYPE_MAY_POPUP_MENU) {
			if (Vector2(drag_start_mouse_pos)
					.distance_to(alternative_tiles_control->get_local_mouse_position()) >
				5.0 * EDSCALE) {
				drag_type = DRAG_TYPE_NONE;
			}
		}
	}

	Ref<InputEventMouseButton> mb = p_event;
	if (mb.is_valid()) {
		Vector2 mouse_local_pos = alternative_tiles_control->get_local_mouse_position();
		if (mb->get_button_index() == MouseButton::LEFT) {
			if (mb->is_pressed()) {
				// Left click pressed.
				if (tools_button_group->get_pressed_button() == tool_select_button) {
					Vector3 tile = tile_atlas_view->get_alternative_tile_at_pos(mouse_local_pos);

					selection.clear();
					TileSelection selected = {Vector2i(tile.x, tile.y), int(tile.z)};
					if (selected.tile != TileSetSource::INVALID_ATLAS_COORDS) {
						selection.insert(selected);
					}

					_update_tile_inspector();
					_update_tile_id_label();
				}
			}
		}
		else if (mb->get_button_index() == MouseButton::RIGHT) {
			if (mb->is_pressed()) {
				drag_type = DRAG_TYPE_MAY_POPUP_MENU;
				drag_start_mouse_pos = alternative_tiles_control->get_local_mouse_position();
			}
			else {
				if (drag_type == DRAG_TYPE_MAY_POPUP_MENU) {
					// Right click released and wasn't dragged too far
					Vector3 tile = tile_atlas_view->get_alternative_tile_at_pos(mouse_local_pos);

					selection.clear();
					TileSelection selected = {Vector2i(tile.x, tile.y), int(tile.z)};
					if (selected.tile != TileSetSource::INVALID_ATLAS_COORDS) {
						selection.insert(selected);
					}

					_update_tile_inspector();
					_update_tile_id_label();

					if (selection.size() == 1) {
						selected = selection.front()->get();
						menu_option_coords = selected.tile;
						menu_option_alternative = selected.alternative;
						alternative_tile_popup_menu->popup(Rect2i(
							get_screen_transform().xform(get_local_mouse_position()), Size2i()));
					}
				}

				drag_type = DRAG_TYPE_NONE;
			}
		}
		tile_atlas_control->queue_redraw();
		tile_atlas_control_unscaled->queue_redraw();
		alternative_tiles_control->queue_redraw();
		alternative_tiles_control_unscaled->queue_redraw();
	}
}

void TileSetAtlasSourceEditor::_tile_alternatives_control_mouse_exited()
{
	hovered_alternative_tile_coords = Vector3i(TileSetSource::INVALID_ATLAS_COORDS.x,
		TileSetSource::INVALID_ATLAS_COORDS.y, TileSetSource::INVALID_TILE_ALTERNATIVE);
	tile_atlas_control->queue_redraw();
	tile_atlas_control_unscaled->queue_redraw();
	alternative_tiles_control->queue_redraw();
	alternative_tiles_control_unscaled->queue_redraw();
}

void TileSetAtlasSourceEditor::_tile_alternatives_control_draw()
{
	// Update the hovered alternative tile.
	if (tools_button_group->get_pressed_button() == tool_select_button) {
		// Draw hovered tile.
		Vector2i coords =
			Vector2(hovered_alternative_tile_coords.x, hovered_alternative_tile_coords.y);
		if (coords != TileSetSource::INVALID_ATLAS_COORDS) {
			Rect2i rect = tile_atlas_view->get_alternative_tile_rect(
				coords, hovered_alternative_tile_coords.z);
			if (rect != Rect2i()) {
				TilesEditorUtils::draw_selection_rect(
					alternative_tiles_control, rect, Color(1.0, 0.8, 0.0, 0.5));
			}
		}

		// Draw selected tile.
		for (const TileSelection& E : selection) {
			TileSelection selected = E;
			if (selected.alternative >= 1) {
				Rect2i rect =
					tile_atlas_view->get_alternative_tile_rect(selected.tile, selected.alternative);
				if (rect != Rect2i()) {
					TilesEditorUtils::draw_selection_rect(alternative_tiles_control, rect);
				}
			}
		}
	}
}

void TileSetAtlasSourceEditor::_tile_alternatives_control_unscaled_draw()
{
	// Draw the preview of the selected property.
	if (current_tile_data_editor) {
		// Draw the preview of the currently selected property.
		for (int i = 0; i < tile_set_atlas_source->get_tiles_count(); i++) {
			Vector2i coords = tile_set_atlas_source->get_tile_id(i);
			for (int j = 0; j < tile_set_atlas_source->get_alternative_tiles_count(coords); j++) {
				int alternative_tile = tile_set_atlas_source->get_alternative_tile_id(coords, j);
				if (alternative_tile == 0) {
					continue;
				}
				Rect2i rect = tile_atlas_view->get_alternative_tile_rect(coords, alternative_tile);
				Vector2 position = rect.get_center() +
								   tile_set_atlas_source->get_tile_data(coords, alternative_tile)
									   ->get_texture_origin();

				Transform2D xform =
					alternative_tiles_control->get_parent_control()->get_transform();
				xform.translate_local(position);

				if (tools_button_group->get_pressed_button() == tool_select_button &&
					selection.has({coords, alternative_tile})) {
					continue;
				}

				TileMapCell cell;
				cell.source_id = tile_set_atlas_source_id;
				cell.set_atlas_coords(coords);
				cell.alternative_tile = alternative_tile;
				current_tile_data_editor->draw_over_tile(
					alternative_tiles_control_unscaled, xform, cell);
			}
		}

		// Draw the selection on top of other.
		if (tools_button_group->get_pressed_button() == tool_select_button) {
			for (const TileSelection& E : selection) {
				if (E.alternative == 0) {
					continue;
				}
				Rect2i rect = tile_atlas_view->get_alternative_tile_rect(E.tile, E.alternative);
				Vector2 position =
					rect.get_center() + tile_set_atlas_source->get_tile_data(E.tile, E.alternative)
											->get_texture_origin();

				Transform2D xform =
					alternative_tiles_control->get_parent_control()->get_transform();
				xform.translate_local(position);

				TileMapCell cell;
				cell.source_id = tile_set_atlas_source_id;
				cell.set_atlas_coords(E.tile);
				cell.alternative_tile = E.alternative;
				current_tile_data_editor->draw_over_tile(
					alternative_tiles_control_unscaled, xform, cell, true);
			}
		}

		// Call the TileData's editor custom draw function.
		if (tools_button_group->get_pressed_button() == tool_paint_button) {
			Transform2D xform = tile_atlas_control->get_parent_control()->get_transform();
			current_tile_data_editor->forward_draw_over_alternatives(
				tile_atlas_view, tile_set_atlas_source, alternative_tiles_control_unscaled, xform);
		}
	}
}

void TileSetAtlasSourceEditor::_tile_proxy_object_changed(const String& p_what)
{
	tile_set_changed_needs_update = false; // Avoid updating too many things.
	_update_atlas_view();
}

Vector2i TileSetAtlasSourceEditor::_get_drag_offset_tile_coords(const Vector2i& p_offset) const
{
	Vector2i half_tile_size = tile_set->get_tile_size() / 2;
	Vector2i new_base_tiles_coords = tile_atlas_view->get_atlas_tile_coords_at_pos(
		tile_atlas_control->get_local_mouse_position() + half_tile_size * p_offset);
	return new_base_tiles_coords.maxi(-1).min(tile_set_atlas_source->get_atlas_grid_size());
}

void TileSetAtlasSourceEditor::init_new_atlases(const Vector<Ref<TileSetAtlasSource>>& p_atlases)
{
	tool_setup_atlas_source_button->set_pressed(true);
	atlases_to_auto_create_tiles = p_atlases;
	confirm_auto_create_tiles->popup_centered();
}

void TileSetAtlasSourceEditor::_check_outside_tiles()
{
	ERR_FAIL_NULL(tile_set_atlas_source);
	bool has_tiles_outside = tile_set_atlas_source->has_tiles_outside_texture();
	outside_tiles_warning->set_visible(!read_only && has_tiles_outside);
	tool_advanced_menu_button->get_popup()->set_item_disabled(
		tool_advanced_menu_button->get_popup()->get_item_index(ADVANCED_CLEANUP_TILES),
		!has_tiles_outside);
}

void TileSetAtlasSourceEditor::_cancel_auto_create_tiles() { atlases_to_auto_create_tiles.clear(); }

void TileSetAtlasSourceEditor::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_READY: {
		atlas_source_inspector->add_custom_property_description("TileSetAtlasSourceProxyObject",
			"id",
			TTRC("The tile's unique identifier within this TileSet. Each tile stores its source "
				 "ID, so changing one may make tiles invalid."));
		atlas_source_inspector->add_custom_property_description("TileSetAtlasSourceProxyObject",
			"name",
			TTRC("The human-readable name for the atlas. Use a descriptive name here for "
				 "organizational purposes (such as \"terrain\", \"decoration\", etc.)."));
		atlas_source_inspector->add_custom_property_description("TileSetAtlasSourceProxyObject",
			"texture", TTRC("The image from which the tiles will be created."));
		atlas_source_inspector->add_custom_property_description("TileSetAtlasSourceProxyObject",
			"margins",
			TTRC("The margins on the image's edges that should not be selectable as tiles (in "
				 "pixels). Increasing this can be useful if you download a tilesheet image that "
				 "has margins on the edges (e.g. for attribution)."));
		atlas_source_inspector->add_custom_property_description("TileSetAtlasSourceProxyObject",
			"separation",
			TTRC("The separation between each tile on the atlas in pixels. Increasing this can be "
				 "useful if the tilesheet image you're using contains guides (such as outlines "
				 "between every tile)."));
		atlas_source_inspector->add_custom_property_description("TileSetAtlasSourceProxyObject",
			"texture_region_size",
			TTRC("The size of each tile on the atlas in pixels. In most cases, this should match "
				 "the tile size defined in the TileMap property (although this is not strictly "
				 "necessary)."));
		atlas_source_inspector->add_custom_property_description("TileSetAtlasSourceProxyObject",
			"use_texture_padding",
			TTRC("If checked, adds a 1-pixel transparent edge around each tile to prevent texture "
				 "bleeding when filtering is enabled. It's recommended to leave this enabled "
				 "unless you're running into rendering issues due to texture padding."));

		tile_inspector->add_custom_property_description("AtlasTileProxyObject", "atlas_coords",
			TTRC("The position of the tile's top-left corner in the atlas. The position and size "
				 "must be within the atlas and can't overlap another tile.\nEach painted tile has "
				 "associated atlas coords, so changing this property may cause your TileMaps to "
				 "not display properly."));
		tile_inspector->add_custom_property_description(
			"AtlasTileProxyObject", "size_in_atlas", TTRC("The unit size of the tile."));
		tile_inspector->add_custom_property_description("AtlasTileProxyObject", "animation_columns",
			TTRC("Number of columns for the animation grid. If number of columns is lower than "
				 "number of frames, the animation will automatically adjust row count."));
		tile_inspector->add_custom_property_description("AtlasTileProxyObject",
			"animation_separation",
			TTRC("The space (in tiles) between each frame of the animation."));
		tile_inspector->add_custom_property_description("AtlasTileProxyObject", "animation_speed",
			TTRC("Animation speed in frames per second."));
		tile_inspector->add_custom_property_description("AtlasTileProxyObject", "animation_mode",
			TTRC("Determines how animation will start. In \"Default\" mode all tiles start "
				 "animating at the same frame. In \"Random Start Times\" mode, each tile starts "
				 "animation with a random offset."));
		tile_inspector->add_custom_property_description("AtlasTileProxyObject", "flip_h",
			TTRC("If [code]true[/code], the tile is horizontally flipped."));
		tile_inspector->add_custom_property_description("AtlasTileProxyObject", "flip_v",
			TTRC("If [code]true[/code], the tile is vertically flipped."));
		tile_inspector->add_custom_property_description("AtlasTileProxyObject", "transpose",
			TTRC("If [code]true[/code], the tile is rotated 90 degrees [i]counter-clockwise[/i] "
				 "and then flipped vertically. In practice, this means that to rotate a tile by 90 "
				 "degrees clockwise without flipping it, you should enable [b]Flip H[/b] and "
				 "[b]Transpose[/b]. To rotate a tile by 180 degrees clockwise, enable [b]Flip "
				 "H[/b] and [b]Flip V[/b]. To rotate a tile by 270 degrees clockwise, enable "
				 "[b]Flip V[/b] and [b]Transpose[/b]."));
		tile_inspector->add_custom_property_description("AtlasTileProxyObject", "texture_origin",
			TTRC("The origin to use for drawing the tile. This can be used to visually offset the "
				 "tile compared to the base tile."));
		tile_inspector->add_custom_property_description("AtlasTileProxyObject", "modulate",
			TTRC("The color multiplier to use when rendering the tile."));
		tile_inspector->add_custom_property_description("AtlasTileProxyObject", "material",
			TTRC("The material to use for this tile. This can be used to apply a different blend "
				 "mode or custom shaders to a single tile."));
		tile_inspector->add_custom_property_description("AtlasTileProxyObject", "z_index",
			TTRC("The sorting order for this tile. Higher values will make the tile render in "
				 "front of others on the same layer. The index is relative to the TileMap's own Z "
				 "index."));
		tile_inspector->add_custom_property_description("AtlasTileProxyObject", "y_sort_origin",
			TTRC("The vertical offset to use for tile sorting based on its Y coordinate (in "
				 "pixels). This allows using layers as if they were on different height for "
				 "top-down games. Adjusting this can help alleviate issues with sorting certain "
				 "tiles. Only effective if Y Sort Enabled is true on the TileMap layer the tile is "
				 "placed on."));
		tile_inspector->add_custom_property_description("AtlasTileProxyObject", "terrain_set",
			TTRC("The index of the terrain set this tile belongs to. [code]-1[/code] means it will "
				 "not be used in terrains."));
		tile_inspector->add_custom_property_description("AtlasTileProxyObject", "terrain",
			TTRC("The index of the terrain inside the terrain set this tile belongs to. "
				 "[code]-1[/code] means it will not be used in terrains."));
		tile_inspector->add_custom_property_description("AtlasTileProxyObject", "probability",
			TTRC("The relative probability of this tile appearing when painting with \"Place "
				 "Random Tile\" enabled."));
	} break;

	case NOTIFICATION_TRANSLATION_CHANGED: {
		outside_tiles_warning->set_tooltip_text(
			vformat(TTR("The current atlas source has tiles outside the texture.\nYou can clear it "
						"using \"%s\" option in the 3 dots menu."),
				TTR("Remove Tiles Outside the Texture")));
		if (tile_set.is_valid()) {
			_update_tile_data_editors();
			_update_atlas_view();
		}
	} break;

	case NOTIFICATION_THEME_CHANGED: {
		tool_setup_atlas_source_button->set_button_icon(get_editor_theme_icon(SNAME("Tools")));
		tool_select_button->set_button_icon(get_editor_theme_icon(SNAME("ToolSelect")));
		tool_paint_button->set_button_icon(get_editor_theme_icon(SNAME("Paint")));

		tools_settings_erase_button->set_button_icon(get_editor_theme_icon(SNAME("Eraser")));
		tool_advanced_menu_button->set_button_icon(get_editor_theme_icon(SNAME("GuiTabMenuHl")));
		outside_tiles_warning->set_texture(get_editor_theme_icon(SNAME("StatusWarning")));

		resize_handle = get_editor_theme_icon(SNAME("EditorHandle"));
		resize_handle_disabled = get_editor_theme_icon(SNAME("EditorHandleDisabled"));

		tile_data_editors_tree->add_theme_style_override(
			SceneStringName(panel), get_theme_stylebox(SceneStringName(panel), "PopupPanel").ptr());
	} break;

	case NOTIFICATION_INTERNAL_PROCESS: {
		if (tile_set_changed_needs_update) {
			// Read-only is off by default
			read_only = false;
			// Add the listener again and check for read-only status.
			if (tile_set.is_valid()) {
				read_only = EditorNode::get_singleton()->is_resource_read_only(tile_set);
			}

			_update_buttons();

			// Update everything.
			_update_source_inspector();

			// Update the selected tile.
			_update_fix_selected_and_hovered_tiles();
			_update_tile_id_label();
			_update_atlas_view();
			_update_atlas_source_inspector();
			_update_tile_inspector();
			_update_tile_data_editors();
			_update_current_tile_data_editor();

			tile_set_changed_needs_update = false;
		}
	} break;

	case NOTIFICATION_EXIT_TREE: {
		for (KeyValue<String, TileDataEditor*>& E : tile_data_editors) {
			Control* toolbar = E.value->get_toolbar();
			if (toolbar->get_parent() == tool_settings_tile_data_toolbar_container) {
				tool_settings_tile_data_toolbar_container->remove_child(toolbar);
			}
		}
	} break;
	}
}

////// EditorPropertyTilePolygon //////

////// EditorInspectorPluginTileData //////

Control::CursorShape TileSetAtlasSourceEditor::TileAtlasControl::get_cursor_shape(
	const Point2& p_pos) const
{
	Control::CursorShape cursor_shape = get_default_cursor_shape();
	if (editor->drag_type == DRAG_TYPE_NONE) {
		if (editor->selection.size() == 1) {
			// Change the cursor depending on the hovered thing.
			TileSelection selected = editor->selection.front()->get();
			if (selected.tile != TileSetSource::INVALID_ATLAS_COORDS && selected.alternative == 0) {
				Transform2D xform =
					editor->tile_atlas_control->get_global_transform().affine_inverse() *
					get_global_transform();
				Vector2 mouse_local_pos = xform.xform(p_pos);
				Vector2i size_in_atlas =
					editor->tile_set_atlas_source->get_tile_size_in_atlas(selected.tile);
				Rect2 region =
					editor->tile_set_atlas_source->get_tile_texture_region(selected.tile);
				Size2 zoomed_size =
					editor->resize_handle->get_size() / editor->tile_atlas_view->get_zoom();
				Rect2 rect = region.grow_individual(zoomed_size.x, zoomed_size.y, 0, 0);
				const Vector2i coords[] = {
					Vector2i(0, 0), Vector2i(1, 0), Vector2i(1, 1), Vector2i(0, 1)};

	const Vector2i directions[] = {
					Vector2i(0, -1), Vector2i(1, 0), Vector2i(0, 1), Vector2i(-1, 0)};
				bool can_grow[4];
				for (int i = 0; i < 4; i++) {
					can_grow[i] = editor->tile_set_atlas_source->has_room_for_tile(
						selected.tile + directions[i],
						editor->tile_set_atlas_source->get_tile_size_in_atlas(selected.tile),
						editor->tile_set_atlas_source->get_tile_animation_columns(selected.tile),
						editor->tile_set_atlas_source->get_tile_animation_separation(selected.tile),
						editor->tile_set_atlas_source->get_tile_animation_frames_count(
							selected.tile),
						selected.tile);
					can_grow[i] |= (i % 2 == 0) ? size_in_atlas.y > 1 : size_in_atlas.x > 1;
				}
				for (int i = 0; i < 4; i++) {
					Vector2 pos = rect.position + rect.size * coords[i];
					if (can_grow[i] && can_grow[(i + 3) % 4] &&
						Rect2(pos, zoomed_size).has_point(mouse_local_pos)) {
						cursor_shape = (i % 2) ? CURSOR_BDIAGSIZE : CURSOR_FDIAGSIZE;
					}
					Vector2 next_pos = rect.position + rect.size * coords[(i + 1) % 4];
					if (can_grow[i] &&
						Rect2((pos + next_pos) / 2.0, zoomed_size).has_point(mouse_local_pos)) {
						cursor_shape = (i % 2) ? CURSOR_HSIZE : CURSOR_VSIZE;
					}
				}
			}
		}
	}
	else {
		switch (editor->drag_type) {
		case DRAG_TYPE_RESIZE_TOP_LEFT:
		case DRAG_TYPE_RESIZE_BOTTOM_RIGHT:
			cursor_shape = CURSOR_FDIAGSIZE;
			break;
		case DRAG_TYPE_RESIZE_TOP:
		case DRAG_TYPE_RESIZE_BOTTOM:
			cursor_shape = CURSOR_VSIZE;
			break;
		case DRAG_TYPE_RESIZE_TOP_RIGHT:
		case DRAG_TYPE_RESIZE_BOTTOM_LEFT:
			cursor_shape = CURSOR_BDIAGSIZE;
			break;
		case DRAG_TYPE_RESIZE_LEFT:
		case DRAG_TYPE_RESIZE_RIGHT:
			cursor_shape = CURSOR_HSIZE;
			break;
		case DRAG_TYPE_MOVE_TILE:
			cursor_shape = CURSOR_MOVE;
			break;
		default:
			break;
		}
	}
	return cursor_shape;
}


