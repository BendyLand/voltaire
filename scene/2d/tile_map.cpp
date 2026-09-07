/**************************************************************************/
/*  tile_map.cpp                                                          */
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
#include "core/io/marshalls.h"
#include "tile_map.compat.inc"
#include "tile_map.h"

#ifndef NAVIGATION_2D_DISABLED
#include "scene/resources/2d/navigation_mesh_source_geometry_data_2d.h"
#include "servers/navigation_2d/navigation_server_2d.h"
#endif // NAVIGATION_2D_DISABLED

#define TILEMAP_CALL_FOR_LAYER(layer, function, ...)                                               \
	if (layer < 0) {                                                                               \
		layer = layers.size() + layer;                                                             \
	};                                                                                             \
	ERR_FAIL_INDEX(layer, (int)layers.size());                                                     \
	layers[layer]->function(__VA_ARGS__);

#define TILEMAP_CALL_FOR_LAYER_V(layer, err_value, function, ...)                                  \
	if (layer < 0) {                                                                               \
		layer = layers.size() + layer;                                                             \
	};                                                                                             \
	ERR_FAIL_INDEX_V(layer, (int)layers.size(), err_value);                                        \
	return layers[layer]->function(__VA_ARGS__);

#ifndef NAVIGATION_2D_DISABLED
RID TileMap::_navmesh_source_geometry_parser;
#endif // NAVIGATION_2D_DISABLED

void TileMap::_tile_set_changed() { update_configuration_warnings(); }

Vector<int> TileMap::_get_tile_map_data_using_compatibility_format(int p_layer) const
{
	ERR_FAIL_INDEX_V(p_layer, (int)layers.size(), Vector<int>());

	// Export tile data to raw format.
	const HashMap<Vector2i, CellData> tile_map_layer_data(
		layers[p_layer]->get_tile_map_layer_data());
	Vector<int> tile_data;
	tile_data.resize(tile_map_layer_data.size() * 3);
	int* w = tile_data.ptrw();

	// Save in highest format.

	int idx = 0;
	for (const KeyValue<Vector2i, CellData>& E : tile_map_layer_data) {
		uint8_t* ptr = (uint8_t*)&w[idx];
		encode_uint16((int16_t)(E.key.x), &ptr[0]);
		encode_uint16((int16_t)(E.key.y), &ptr[2]);
		encode_uint16(E.value.cell.source_id, &ptr[4]);
		encode_uint16(E.value.cell.coord_x, &ptr[6]);
		encode_uint16(E.value.cell.coord_y, &ptr[8]);
		encode_uint16(E.value.cell.alternative_tile, &ptr[10]);
		idx += 3;
	}

	return tile_data;
}

void TileMap::_set_layer_tile_data(int p_layer, const PackedInt32Array& p_data)
{
	_set_tile_map_data_using_compatibility_format(p_layer, format, p_data);
}

void TileMap::_notification(int p_what)
{
	switch (p_what) {
	case TileMap::NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
		// This is only executed when collision_animatable is enabled.

		bool in_editor = false;
#ifdef TOOLS_ENABLED
		in_editor = Engine::get_singleton()->is_editor_hint();
#endif // TOOLS_ENABLED
		if (is_inside_tree() && collision_animatable && !in_editor) {
			// Update transform on the physics tick when in animatable mode.
			last_valid_transform = new_transform;
			set_notify_local_transform(false);
			set_global_transform(new_transform);
			set_notify_local_transform(true);
		}
	} break;

	case TileMap::NOTIFICATION_LOCAL_TRANSFORM_CHANGED: {
		// This is only executed when collision_animatable is enabled.

		bool in_editor = false;
#ifdef TOOLS_ENABLED
		in_editor = Engine::get_singleton()->is_editor_hint();
#endif // TOOLS_ENABLED

		if (is_inside_tree() && collision_animatable && !in_editor) {
			// Store last valid transform.
			new_transform = get_global_transform();

			// ... but then revert changes.
			set_notify_local_transform(false);
			set_global_transform(last_valid_transform);
			set_notify_local_transform(true);
		}
	} break;
	}
}

#ifndef DISABLE_DEPRECATED
// Deprecated methods.
void TileMap::force_update(int p_layer)
{
	notify_runtime_tile_data_update(p_layer);
	update_internals();
}
#endif // DISABLE_DEPRECATED

void TileMap::set_rendering_quadrant_size(int p_size)
{
	ERR_FAIL_COND_MSG(p_size < 1, "TileMapQuadrant size cannot be smaller than 1.");

	rendering_quadrant_size = p_size;
	for (TileMapLayer* layer : layers) {
		layer->set_rendering_quadrant_size(p_size);
	}
	_emit_changed();
}

int TileMap::get_rendering_quadrant_size() const { return rendering_quadrant_size; }

Ref<TileSet> TileMap::get_tileset() const { return tile_set; }

int TileMap::get_layers_count() const { return layers.size(); }

void TileMap::set_layer_name(int p_layer, String p_name)
{
	TILEMAP_CALL_FOR_LAYER(p_layer, set_name, p_name);
}

String TileMap::get_layer_name(int p_layer) const
{
	TILEMAP_CALL_FOR_LAYER_V(p_layer, "", get_name);
}

void TileMap::set_layer_enabled(int p_layer, bool p_enabled)
{
	TILEMAP_CALL_FOR_LAYER(p_layer, set_enabled, p_enabled);
}

bool TileMap::is_layer_enabled(int p_layer) const
{
	TILEMAP_CALL_FOR_LAYER_V(p_layer, false, is_enabled);
}

void TileMap::set_layer_modulate(int p_layer, Color p_modulate)
{
	TILEMAP_CALL_FOR_LAYER(p_layer, set_modulate, p_modulate);
}

Color TileMap::get_layer_modulate(int p_layer) const
{
	TILEMAP_CALL_FOR_LAYER_V(p_layer, Color(), get_modulate);
}

void TileMap::set_layer_y_sort_enabled(int p_layer, bool p_y_sort_enabled)
{
	TILEMAP_CALL_FOR_LAYER(p_layer, set_y_sort_enabled, p_y_sort_enabled);
	update_configuration_warnings();
}

bool TileMap::is_layer_y_sort_enabled(int p_layer) const
{
	TILEMAP_CALL_FOR_LAYER_V(p_layer, false, is_y_sort_enabled);
}

void TileMap::set_layer_y_sort_origin(int p_layer, int p_y_sort_origin)
{
	TILEMAP_CALL_FOR_LAYER(p_layer, set_y_sort_origin, p_y_sort_origin);
	update_configuration_warnings();
}

int TileMap::get_layer_y_sort_origin(int p_layer) const
{
	TILEMAP_CALL_FOR_LAYER_V(p_layer, 0, get_y_sort_origin);
}

void TileMap::set_layer_z_index(int p_layer, int p_z_index)
{
	TILEMAP_CALL_FOR_LAYER(p_layer, set_z_index, p_z_index);
}

int TileMap::get_layer_z_index(int p_layer) const
{
	TILEMAP_CALL_FOR_LAYER_V(p_layer, 0, get_z_index);
}

#ifndef NAVIGATION_2D_DISABLED
void TileMap::set_layer_navigation_enabled(int p_layer, bool p_enabled)
{
	TILEMAP_CALL_FOR_LAYER(p_layer, set_navigation_enabled, p_enabled);
}

bool TileMap::is_layer_navigation_enabled(int p_layer) const
{
	TILEMAP_CALL_FOR_LAYER_V(p_layer, false, is_navigation_enabled);
}

void TileMap::set_layer_navigation_map(int p_layer, RID p_map)
{
	TILEMAP_CALL_FOR_LAYER(p_layer, set_navigation_map, p_map);
}

RID TileMap::get_layer_navigation_map(int p_layer) const
{
	TILEMAP_CALL_FOR_LAYER_V(p_layer, RID(), get_navigation_map);
}
#endif // NAVIGATION_2D_DISABLED

void TileMap::set_collision_animatable(bool p_collision_animatable)
{
	if (collision_animatable == p_collision_animatable) {
		return;
	}
	collision_animatable = p_collision_animatable;
	set_notify_local_transform(p_collision_animatable);
	set_physics_process_internal(p_collision_animatable);
	for (TileMapLayer* layer : layers) {
		layer->set_use_kinematic_bodies(layer);
	}
}

bool TileMap::is_collision_animatable() const { return collision_animatable; }

void TileMap::set_collision_visibility_mode(TileMap::VisibilityMode p_show_collision)
{
	if (collision_visibility_mode == p_show_collision) {
		return;
	}
	collision_visibility_mode = p_show_collision;
	for (TileMapLayer* layer : layers) {
		layer->set_collision_visibility_mode(TileMapLayer::DebugVisibilityMode(p_show_collision));
	}
	_emit_changed();
}

TileMap::VisibilityMode TileMap::get_collision_visibility_mode() const
{
	return collision_visibility_mode;
}

#ifndef NAVIGATION_2D_DISABLED
void TileMap::set_navigation_visibility_mode(TileMap::VisibilityMode p_show_navigation)
{
	if (navigation_visibility_mode == p_show_navigation) {
		return;
	}
	navigation_visibility_mode = p_show_navigation;
	for (TileMapLayer* layer : layers) {
		layer->set_navigation_visibility_mode(TileMapLayer::DebugVisibilityMode(p_show_navigation));
	}
	_emit_changed();
}

TileMap::VisibilityMode TileMap::get_navigation_visibility_mode() const
{
	return navigation_visibility_mode;
}
#endif // NAVIGATION_2D_DISABLED

void TileMap::set_y_sort_enabled(bool p_enable)
{
	if (is_y_sort_enabled() == p_enable) {
		return;
	}
	Node2D::set_y_sort_enabled(p_enable);
	_emit_changed();
	update_configuration_warnings();
}

void TileMap::set_cell(int p_layer, const Vector2i& p_coords, int p_source_id,
	const Vector2i p_atlas_coords, int p_alternative_tile)
{
	TILEMAP_CALL_FOR_LAYER(
		p_layer, set_cell, p_coords, p_source_id, p_atlas_coords, p_alternative_tile);
}

void TileMap::erase_cell(int p_layer, const Vector2i& p_coords)
{
	TILEMAP_CALL_FOR_LAYER(p_layer, set_cell, p_coords, TileSet::INVALID_SOURCE,
		TileSetSource::INVALID_ATLAS_COORDS, TileSetSource::INVALID_TILE_ALTERNATIVE);
}

bool TileMap::is_cell_flipped_h(int p_layer, const Vector2i& p_coords, bool p_use_proxies) const
{
	return get_cell_alternative_tile(p_layer, p_coords, p_use_proxies) &
		   TileSetAtlasSource::TRANSFORM_FLIP_H;
}

bool TileMap::is_cell_flipped_v(int p_layer, const Vector2i& p_coords, bool p_use_proxies) const
{
	return get_cell_alternative_tile(p_layer, p_coords, p_use_proxies) &
		   TileSetAtlasSource::TRANSFORM_FLIP_V;
}

bool TileMap::is_cell_transposed(int p_layer, const Vector2i& p_coords, bool p_use_proxies) const
{
	return get_cell_alternative_tile(p_layer, p_coords, p_use_proxies) &
		   TileSetAtlasSource::TRANSFORM_TRANSPOSE;
}

Vector2i TileMap::map_pattern(const Vector2i& p_position_in_tilemap,
	const Vector2i& p_coords_in_pattern, Ref<TileMapPattern> p_pattern)
{
	ERR_FAIL_COND_V(tile_set.is_null(), Vector2i());
	return tile_set->map_pattern(p_position_in_tilemap, p_coords_in_pattern, p_pattern);
}

void TileMap::set_pattern(
	int p_layer, const Vector2i& p_position, const Ref<TileMapPattern> p_pattern)
{
	TILEMAP_CALL_FOR_LAYER(p_layer, set_pattern, p_position, p_pattern);
}

HashMap<Vector2i, TileSet::TerrainsPattern> TileMap::terrain_fill_constraints(int p_layer,
	const Vector<Vector2i>& p_to_replace, int p_terrain_set,
	const RBSet<TerrainConstraint>& p_constraints)
{
	TILEMAP_CALL_FOR_LAYER_V(p_layer, (HashMap<Vector2i, TileSet::TerrainsPattern>()),
		terrain_fill_constraints, p_to_replace, p_terrain_set, p_constraints);
}

HashMap<Vector2i, TileSet::TerrainsPattern> TileMap::terrain_fill_connect(int p_layer,
	const Vector<Vector2i>& p_coords_array, int p_terrain_set, int p_terrain,
	bool p_ignore_empty_terrains)
{
	TILEMAP_CALL_FOR_LAYER_V(p_layer, (HashMap<Vector2i, TileSet::TerrainsPattern>()),
		terrain_fill_connect, p_coords_array, p_terrain_set, p_terrain, p_ignore_empty_terrains);
}

HashMap<Vector2i, TileSet::TerrainsPattern> TileMap::terrain_fill_path(int p_layer,
	const Vector<Vector2i>& p_coords_array, int p_terrain_set, int p_terrain,
	bool p_ignore_empty_terrains)
{
	TILEMAP_CALL_FOR_LAYER_V(p_layer, (HashMap<Vector2i, TileSet::TerrainsPattern>()),
		terrain_fill_path, p_coords_array, p_terrain_set, p_terrain, p_ignore_empty_terrains);
}

HashMap<Vector2i, TileSet::TerrainsPattern> TileMap::terrain_fill_pattern(int p_layer,
	const Vector<Vector2i>& p_coords_array, int p_terrain_set,
	TileSet::TerrainsPattern p_terrains_pattern, bool p_ignore_empty_terrains)
{
	TILEMAP_CALL_FOR_LAYER_V(p_layer, (HashMap<Vector2i, TileSet::TerrainsPattern>()),
		terrain_fill_pattern, p_coords_array, p_terrain_set, p_terrains_pattern,
		p_ignore_empty_terrains);
}

TileMapCell TileMap::get_cell(int p_layer, const Vector2i& p_coords, bool p_use_proxies) const
{
	if (p_use_proxies) {
		WARN_DEPRECATED_MSG("use_proxies is deprecated.");
	}
	TILEMAP_CALL_FOR_LAYER_V(p_layer, TileMapCell(), get_cell, p_coords);
}

#ifndef PHYSICS_2D_DISABLED
Vector2i TileMap::get_coords_for_body_rid(RID p_physics_body)
{
	for (const TileMapLayer* layer : layers) {
		if (layer->has_body_rid(p_physics_body)) {
			return layer->get_coords_for_body_rid(p_physics_body);
		}
	}
	ERR_FAIL_V_MSG(
		Vector2i(), vformat("No tiles for the given body RID %d.", p_physics_body.get_id()));
}

int TileMap::get_layer_for_body_rid(RID p_physics_body)
{
	for (uint32_t i = 0; i < layers.size(); i++) {
		if (layers[i]->has_body_rid(p_physics_body)) {
			return i;
		}
	}
	ERR_FAIL_V_MSG(-1, vformat("No tiles for the given body RID %d.", p_physics_body.get_id()));
}
#endif // PHYSICS_2D_DISABLED

void TileMap::fix_invalid_tiles()
{
	for (TileMapLayer* layer : layers) {
		layer->fix_invalid_tiles();
	}
}

void TileMap::clear_layer(int p_layer) { TILEMAP_CALL_FOR_LAYER(p_layer, clear) }

void TileMap::clear()
{
	for (TileMapLayer* layer : layers) {
		layer->clear();
	}
}

void TileMap::update_internals()
{
	for (TileMapLayer* layer : layers) {
		layer->update_internals();
	}
}

void TileMap::notify_runtime_tile_data_update(int p_layer)
{
	if (p_layer >= 0) {
		TILEMAP_CALL_FOR_LAYER(p_layer, notify_runtime_tile_data_update);
	}
	else {
		for (TileMapLayer* layer : layers) {
			layer->notify_runtime_tile_data_update();
		}
	}
}

#ifdef DEBUG_ENABLED
Rect2 TileMap::_edit_get_rect() const
{
	// Return the visible rect of the tilemap.
	if (layers.is_empty()) {
		return Rect2();
	}

	bool any_changed = false;
	bool changed = false;
	Rect2 rect = layers[0]->get_rect(changed);
	any_changed |= changed;
	for (uint32_t i = 1; i < layers.size(); i++) {
		rect = rect.merge(layers[i]->get_rect(changed));
		any_changed |= changed;
	}
	const_cast<TileMap*>(this)->item_rect_changed(any_changed);
	return rect;
}
#endif // DEBUG_ENABLED

Vector2 TileMap::map_to_local(const Vector2i& p_pos) const
{
	ERR_FAIL_COND_V(tile_set.is_null(), Vector2());
	return tile_set->map_to_local(p_pos);
}

Vector2i TileMap::local_to_map(const Vector2& p_pos) const
{
	ERR_FAIL_COND_V(tile_set.is_null(), Vector2i());
	return tile_set->local_to_map(p_pos);
}

bool TileMap::is_existing_neighbor(TileSet::CellNeighbor p_cell_neighbor) const
{
	ERR_FAIL_COND_V(tile_set.is_null(), false);
	return tile_set->is_existing_neighbor(p_cell_neighbor);
}

Vector2i TileMap::get_neighbor_cell(
	const Vector2i& p_coords, TileSet::CellNeighbor p_cell_neighbor) const
{
	ERR_FAIL_COND_V(tile_set.is_null(), Vector2i());
	return tile_set->get_neighbor_cell(p_coords, p_cell_neighbor);
}

Rect2i TileMap::get_used_rect() const
{
	// Return the visible rect of the tilemap.
	bool first = true;
	Rect2i rect = Rect2i();
	for (const TileMapLayer* layer : layers) {
		Rect2i layer_rect = layer->get_used_rect();
		if (layer_rect == Rect2i()) {
			continue;
		}
		if (first) {
			rect = layer_rect;
			first = false;
		}
		else {
			rect = rect.merge(layer_rect);
		}
	}
	return rect;
}

// --- Override some methods of the CanvasItem class to pass the changes to the quadrants
// CanvasItems ---

void TileMap::set_light_mask(int p_light_mask)
{
	// Set light mask for occlusion and applies it to all layers too.
	CanvasItem::set_light_mask(p_light_mask);
	for (TileMapLayer* layer : layers) {
		layer->set_light_mask(p_light_mask);
	}
}

void TileMap::set_self_modulate(const Color& p_self_modulate)
{
	// Set self_modulation and applies it to all layers too.
	CanvasItem::set_self_modulate(p_self_modulate);
	for (TileMapLayer* layer : layers) {
		layer->set_self_modulate(p_self_modulate);
	}
}

void TileMap::set_texture_filter(TextureFilter p_texture_filter)
{
	// Set a default texture filter and applies it to all layers too.
	CanvasItem::set_texture_filter(p_texture_filter);
	for (TileMapLayer* layer : layers) {
		layer->set_texture_filter(p_texture_filter);
	}
}

void TileMap::set_texture_repeat(CanvasItem::TextureRepeat p_texture_repeat)
{
	// Set a default texture repeat and applies it to all layers too.
	CanvasItem::set_texture_repeat(p_texture_repeat);
	for (TileMapLayer* layer : layers) {
		layer->set_texture_repeat(p_texture_repeat);
	}
}

PackedStringArray TileMap::get_configuration_warnings() const
{
	PackedStringArray warnings = Node2D::get_configuration_warnings();

	warnings.push_back(
		RTR("The TileMap node is deprecated as it is superseded by the use of multiple "
			"TileMapLayer nodes.\nTo convert a TileMap to a set of TileMapLayer nodes, open the "
			"TileMap bottom panel with this node selected, click the toolbox icon in the top-right "
			"corner and choose \"Extract TileMap layers as individual TileMapLayer nodes\"."));

	// Retrieve the set of Z index values with a Y-sorted layer.
	RBSet<int> y_sorted_z_index;
	for (const TileMapLayer* layer : layers) {
		if (layer->is_y_sort_enabled()) {
			y_sorted_z_index.insert(layer->get_z_index());
		}
	}

	// Check if we have a non-sorted layer in a Z-index with a Y-sorted layer.
	for (const TileMapLayer* layer : layers) {
		if (!layer->is_y_sort_enabled() && y_sorted_z_index.has(layer->get_z_index())) {
			warnings.push_back(
				RTR("A Y-sorted layer has the same Z-index value as a not Y-sorted layer.\nThis "
					"may lead to unwanted behaviors, as a layer that is not Y-sorted will be "
					"Y-sorted as a whole with tiles from Y-sorted layers."));
			break;
		}
	}

	if (!is_y_sort_enabled()) {
		// Check if Y-sort is enabled on a layer but not on the node.
		for (const TileMapLayer* layer : layers) {
			if (layer->is_y_sort_enabled()) {
				warnings.push_back(RTR("A TileMap layer is set as Y-sorted, but Y-sort is not "
									   "enabled on the TileMap node itself."));
				break;
			}
		}
	}
	else {
		// Check if Y-sort is enabled on the node, but not on any of the layers.
		bool need_warning = true;
		for (const TileMapLayer* layer : layers) {
			if (layer->is_y_sort_enabled()) {
				need_warning = false;
				break;
			}
		}
		if (need_warning) {
			warnings.push_back(
				RTR("The TileMap node is set as Y-sorted, but Y-sort is not enabled on any of the "
					"TileMap's layers.\nThis may lead to unwanted behaviors, as a layer that is "
					"not Y-sorted will be Y-sorted as a whole."));
		}
	}

	// Check if we are in isometric mode without Y-sort enabled.
	if (tile_set.is_valid() && tile_set->get_tile_shape() == TileSet::TILE_SHAPE_ISOMETRIC) {
		bool warn = !is_y_sort_enabled();
		if (!warn) {
			for (const TileMapLayer* layer : layers) {
				if (!layer->is_y_sort_enabled()) {
					warn = true;
					break;
				}
			}
		}
		if (warn) {
			warnings.push_back(RTR("Isometric TileSet will likely not look as intended without "
								   "Y-sort enabled for the TileMap and all of its layers."));
		}
	}
	return warnings;
}

#undef TILEMAP_CALL_FOR_LAYER
#undef TILEMAP_CALL_FOR_LAYER_V


