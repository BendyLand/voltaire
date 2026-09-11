/**************************************************************************/
/*  tile_data_editors.cpp                                                 */
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

#include "core/math/geometry_2d.h"
#include "core/math/random_pcg.h"
#include "core/os/keyboard.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/inspector/editor_properties.h"
#include "editor/scene/2d/tiles/tile_set_editor.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/control.h"
#include "scene/gui/label.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/option_button.h"
#include "scene/gui/separator.h"
#include "scene/gui/spin_box.h"
#include "scene/main/scene_tree.h"
#include "servers/navigation_2d/navigation_server_2d.h"
#include "servers/rendering/rendering_server.h"
#include "tile_data_editors.h"

void TileDataEditor::_tile_set_changed_deferred_update()
{
	if (_tile_set_changed_update_needed) {
		_tile_set_changed();
		_tile_set_changed_update_needed = false;
	}
}

void GenericTilePolygonEditor::_center_view()
{
	panning = Vector2();
	base_control->queue_redraw();
	button_center_view->set_disabled(true);
}

void GenericTilePolygonEditor::_zoom_changed() { base_control->queue_redraw(); }

void GenericTilePolygonEditor::_snap_to_tile_shape(
	Point2& r_point, float& r_current_snapped_dist, float p_snap_dist)
{
	ERR_FAIL_COND(tile_set.is_null());

	Vector<Point2> polygon = tile_set->get_tile_shape_polygon();
	for (int i = 0; i < polygon.size(); i++) {
		polygon.write[i] = polygon[i] * tile_set->get_tile_size();
	}
	Point2 snapped_point = r_point;

	// Snap to polygon vertices.
	bool snapped = false;
	for (int i = 0; i < polygon.size(); i++) {
		float distance = r_point.distance_to(polygon[i]);
		if (distance < p_snap_dist && distance < r_current_snapped_dist) {
			snapped_point = polygon[i];
			r_current_snapped_dist = distance;
			snapped = true;
		}
	}

	// Snap to edges if we did not snap to vertices.
	if (!snapped) {
		for (int i = 0; i < polygon.size(); i++) {
			const Vector2 segment_a = polygon[i];
			const Vector2 segment_b = polygon[(i + 1) % polygon.size()];
			Point2 point = Geometry2D::get_closest_point_to_segment(r_point, segment_a, segment_b);
			float distance = r_point.distance_to(point);
			if (distance < p_snap_dist && distance < r_current_snapped_dist) {
				snapped_point = point;
				r_current_snapped_dist = distance;
			}
		}
	}

	r_point = snapped_point;
}

void GenericTilePolygonEditor::_snap_point(Point2& r_point)
{
	switch (current_snap_option) {
	case SNAP_NONE:
		break;

	case SNAP_HALF_PIXEL:
		r_point = r_point.snappedf(0.5);
		break;

	case SNAP_GRID: {
		const Vector2 tile_size = tile_set->get_tile_size();
		r_point = (r_point + tile_size / 2).snapped(tile_size / snap_subdivision->get_value()) -
				  tile_size / 2;
	} break;
	}
}

void GenericTilePolygonEditor::_set_snap_option(int p_index)
{
	current_snap_option = p_index;
	button_pixel_snap->set_button_icon(button_pixel_snap->get_popup()->get_item_icon(p_index));
	snap_subdivision->set_visible(p_index == SNAP_GRID);

	if (initializing) {
		return;
	}

	base_control->queue_redraw();
	_store_snap_options();
}

void GenericTilePolygonEditor::_toggle_expand(bool p_expand)
{
	if (p_expand) {
		TileSetEditor::get_singleton()->add_expanded_editor(this);
	}
	else {
		TileSetEditor::get_singleton()->remove_expanded_editor();
	}
}

void GenericTilePolygonEditor::set_use_undo_redo(bool p_use_undo_redo)
{
	use_undo_redo = p_use_undo_redo;
}

int GenericTilePolygonEditor::get_polygon_count() { return polygons.size(); }

int GenericTilePolygonEditor::add_polygon(const Vector<Point2>& p_polygon, int p_index)
{
	ERR_FAIL_COND_V(p_polygon.size() < 3, -1);
	ERR_FAIL_COND_V(!multiple_polygon_mode && polygons.size() >= 1, -1);

	if (p_index < 0) {
		polygons.push_back(p_polygon);
		base_control->queue_redraw();
		button_edit->set_pressed(true);
		return polygons.size() - 1;
	}
	else {
		polygons.insert(p_index, p_polygon);
		button_edit->set_pressed(true);
		base_control->queue_redraw();
		return p_index;
	}
}

void GenericTilePolygonEditor::remove_polygon(int p_index)
{
	ERR_FAIL_INDEX(p_index, (int)polygons.size());
	polygons.remove_at(p_index);

	if (polygons.is_empty()) {
		button_create->set_pressed(true);
	}
	base_control->queue_redraw();
}

void GenericTilePolygonEditor::clear_polygons()
{
	polygons.clear();
	base_control->queue_redraw();
}

void GenericTilePolygonEditor::set_polygon(int p_polygon_index, const Vector<Point2>& p_polygon)
{
	ERR_FAIL_INDEX(p_polygon_index, (int)polygons.size());
	ERR_FAIL_COND(p_polygon.size() < 3);
	polygons[p_polygon_index] = p_polygon;
	button_edit->set_pressed(true);
	base_control->queue_redraw();
}

Vector<Point2> GenericTilePolygonEditor::get_polygon(int p_polygon_index)
{
	ERR_FAIL_INDEX_V(p_polygon_index, (int)polygons.size(), Vector<Point2>());
	return polygons[p_polygon_index];
}

void GenericTilePolygonEditor::set_polygons_color(Color p_color)
{
	polygon_color = p_color;
	base_control->queue_redraw();
}

void GenericTilePolygonEditor::set_multiple_polygon_mode(bool p_multiple_polygon_mode)
{
	multiple_polygon_mode = p_multiple_polygon_mode;
}

void TileDataDefaultEditor::forward_draw_over_alternatives(TileAtlasView* p_tile_atlas_view,
	TileSetAtlasSource* p_tile_set_atlas_source, CanvasItem* p_canvas_item, Transform2D p_transform)
{
}

void TileDataDefaultEditor::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		picker_button->set_button_icon(get_editor_theme_icon(SNAME("ColorPick")));
		tile_bool_checked = get_editor_theme_icon(SNAME("TileChecked"));
		tile_bool_unchecked = get_editor_theme_icon(SNAME("TileUnchecked"));
	} break;
	}
}

TileDataDefaultEditor::TileDataDefaultEditor()
{
	label = memnew(Label);
	label->set_text(TTR("Painting:"));
	label->set_theme_type_variation("HeaderSmall");
	add_child(label);

	picker_button = memnew(Button);
	picker_button->set_theme_type_variation(SceneStringName(FlatButton));
	picker_button->set_toggle_mode(true);
	picker_button->set_shortcut(ED_GET_SHORTCUT("tiles_editor/picker"));
	toolbar->add_child(picker_button);
}

TileDataDefaultEditor::~TileDataDefaultEditor()
{
	toolbar->queue_free();
	memdelete(dummy_object);
}

void TileDataOcclusionShapeEditor::_set_painted_value(
	TileSetAtlasSource* p_tile_set_atlas_source, Vector2 p_coords, int p_alternative_tile)
{
	TileData* tile_data = p_tile_set_atlas_source->get_tile_data(p_coords, p_alternative_tile);
	ERR_FAIL_NULL(tile_data);

	polygon_editor->clear_polygons();
	for (int i = 0; i < tile_data->get_occluder_polygons_count(occlusion_layer); i++) {
		Ref<OccluderPolygon2D> occluder_polygon =
			tile_data->get_occluder_polygon(occlusion_layer, i);
		if (occluder_polygon.is_valid()) {
			polygon_editor->add_polygon(occluder_polygon->get_polygon());
		}
	}
	polygon_editor->set_background_tile(p_tile_set_atlas_source, p_coords, p_alternative_tile);
}

void TileDataOcclusionShapeEditor::_tile_set_changed() { polygon_editor->set_tile_set(tile_set); }

void TileDataOcclusionShapeEditor::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		polygon_editor->set_polygons_color(get_tree()->get_debug_collisions_color());
	} break;
	}
}

TileDataOcclusionShapeEditor::TileDataOcclusionShapeEditor()
{
	polygon_editor = memnew(GenericTilePolygonEditor);
	polygon_editor->set_multiple_polygon_mode(true);
	add_child(polygon_editor);
}

void TileDataCollisionEditor::_property_selected(const StringName& p_path, int p_focusable)
{
	// Deselect all other properties
	for (KeyValue<StringName, EditorProperty*>& editor : property_editors) {
		if (editor.key != p_path) {
			editor.value->deselect();
		}
	}
}

void TileDataCollisionEditor::_tile_set_changed()
{
	polygon_editor->set_tile_set(tile_set);
	_polygons_changed();
}

void TileDataCollisionEditor::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		polygon_editor->set_polygons_color(get_tree()->get_debug_collisions_color());
	} break;
	case NOTIFICATION_TRANSLATION_CHANGED: {
		if (is_ready()) {
			_polygons_changed();
		}
	} break;
	}
}

TileDataCollisionEditor::~TileDataCollisionEditor() { memdelete(dummy_object); }

void TileDataTerrainsEditor::draw_over_tile(
	CanvasItem* p_canvas_item, Transform2D p_transform, TileMapCell p_cell, bool p_selected)
{
	TileData* tile_data = _get_tile_data(p_cell);
	ERR_FAIL_NULL(tile_data);

	tile_set->draw_terrains(p_canvas_item, p_transform, tile_data);
}

void TileDataTerrainsEditor::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_THEME_CHANGED: {
		picker_button->set_button_icon(get_editor_theme_icon(SNAME("ColorPick")));
	} break;
	}
}

TileDataTerrainsEditor::~TileDataTerrainsEditor()
{
	toolbar->queue_free();
	memdelete(dummy_object);
}

void TileDataNavigationEditor::_set_painted_value(
	TileSetAtlasSource* p_tile_set_atlas_source, Vector2 p_coords, int p_alternative_tile)
{
	TileData* tile_data = p_tile_set_atlas_source->get_tile_data(p_coords, p_alternative_tile);
	ERR_FAIL_NULL(tile_data);

	Ref<NavigationPolygon> nav_polygon = tile_data->get_navigation_polygon(navigation_layer);
	polygon_editor->clear_polygons();
	if (nav_polygon.is_valid()) {
		for (int i = 0; i < nav_polygon->get_outline_count(); i++) {
			polygon_editor->add_polygon(nav_polygon->get_outline(i));
		}
	}
	polygon_editor->set_background_tile(p_tile_set_atlas_source, p_coords, p_alternative_tile);
}

void TileDataNavigationEditor::_tile_set_changed() { polygon_editor->set_tile_set(tile_set); }

void TileDataNavigationEditor::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
#ifdef DEBUG_ENABLED
		polygon_editor->set_polygons_color(
			NavigationServer2D::get_singleton()->get_debug_navigation_geometry_face_color());
#endif // DEBUG_ENABLED
	} break;
	}
}

TileDataNavigationEditor::TileDataNavigationEditor()
{
	polygon_editor = memnew(GenericTilePolygonEditor);
	polygon_editor->set_multiple_polygon_mode(true);
	add_child(polygon_editor);
}


