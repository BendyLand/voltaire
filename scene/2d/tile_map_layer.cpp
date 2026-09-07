/**************************************************************************/
/*  tile_map_layer.cpp                                                    */
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
#include "core/math/geometry_2d.h"
#include "core/math/random_pcg.h"
#include "core/templates/a_hash_map.h"
#include "scene/2d/tile_map.h"
#include "scene/gui/control.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/material.h"
#include "scene/resources/world_2d.h"
#include "servers/rendering/rendering_server.h"
#include "tile_map_layer.h"

#ifndef PHYSICS_2D_DISABLED
#include "servers/physics_2d/physics_server_2d.h"
#endif // PHYSICS_3D_DISABLED

#ifndef NAVIGATION_2D_DISABLED
#include "scene/resources/2d/navigation_mesh_source_geometry_data_2d.h"
#include "servers/navigation_2d/navigation_server_2d.h"
RID TileMapLayer::_navmesh_source_geometry_parser;
#endif // NAVIGATION_2D_DISABLED

Vector2i TileMapLayer::_coords_to_quadrant_coords(
	const Vector2i& p_coords, const int p_quadrant_size) const
{
	return Vector2i(p_coords.x > 0 ? p_coords.x / p_quadrant_size
								   : (p_coords.x - (p_quadrant_size - 1)) / p_quadrant_size,
		p_coords.y > 0 ? p_coords.y / p_quadrant_size
					   : (p_coords.y - (p_quadrant_size - 1)) / p_quadrant_size);
}

#ifdef DEBUG_ENABLED
/////////////////////////////// Debug //////////////////////////////////////////
constexpr int TILE_MAP_DEBUG_QUADRANT_SIZE = 16;

void TileMapLayer::_debug_update(bool p_force_cleanup)
{
	RenderingServer* rs = RenderingServer::get_singleton();

	// Check if we should cleanup everything.
	bool forced_cleanup =
		p_force_cleanup || !enabled || tile_set.is_null() || !is_visible_in_tree();
	if (forced_cleanup && _debug_was_cleaned_up) {
		return;
	}

	if (forced_cleanup) {
		for (KeyValue<Vector2i, Ref<DebugQuadrant>>& kv : debug_quadrant_map) {
			// Free the quadrant.
			Ref<DebugQuadrant>& debug_quadrant = kv.value;
			if (debug_quadrant->canvas_item.is_valid()) {
				rs->free_rid(debug_quadrant->canvas_item);
			}
			if (debug_quadrant->physics_mesh.is_valid()) {
				rs->free_rid(debug_quadrant->physics_mesh);
			}
		}
		debug_quadrant_map.clear();
		_debug_was_cleaned_up = true;
		return;
	}

	// Check if anything is dirty, in such a case, redraw debug.
	bool anything_changed = false;
	for (int i = 0; i < DIRTY_FLAGS_MAX; i++) {
		if (dirty.flags[i]) {
			anything_changed = true;
			break;
		}
	}

	// List all debug quadrants to update.
	HashSet<Vector2i> quadrants_to_updates;
	if (_debug_was_cleaned_up || anything_changed) {
		// Update all cells.
		for (KeyValue<Vector2i, CellData>& kv : tile_map_layer_data) {
			CellData& cell_data = kv.value;
			quadrants_to_updates.insert(
				_coords_to_quadrant_coords(cell_data.coords, TILE_MAP_DEBUG_QUADRANT_SIZE));
#ifndef PHYSICS_2D_DISABLED
			// Physics quadrants are drawn from their origin.
			Vector2i physics_quadrant_origin =
				_coords_to_quadrant_coords(cell_data.coords, physics_quadrant_size) *
				physics_quadrant_size;
			quadrants_to_updates.insert(
				_coords_to_quadrant_coords(physics_quadrant_origin, TILE_MAP_DEBUG_QUADRANT_SIZE));
#endif // PHYSICS_2D_DISABLED
		}
	}
	else {
		// Update dirty cells.
		for (SelfList<CellData>* cell_data_list_element = dirty.cell_list.first();
			 cell_data_list_element; cell_data_list_element = cell_data_list_element->next()) {
			CellData& cell_data = *cell_data_list_element->self();
			quadrants_to_updates.insert(
				_coords_to_quadrant_coords(cell_data.coords, TILE_MAP_DEBUG_QUADRANT_SIZE));
#ifndef PHYSICS_2D_DISABLED
			// Physics quadrants are drawn from their origin.
			Vector2i physics_quadrant_origin =
				_coords_to_quadrant_coords(cell_data.coords, physics_quadrant_size) *
				physics_quadrant_size;
			quadrants_to_updates.insert(
				_coords_to_quadrant_coords(physics_quadrant_origin, TILE_MAP_DEBUG_QUADRANT_SIZE));
#endif // PHYSICS_2D_DISABLED
		}
	}

	// Create new quadrants if needed.
	for (const Vector2i& quadrant_coords : quadrants_to_updates) {
		if (!debug_quadrant_map.has(quadrant_coords)) {
			// Create a new quadrant and add it to the quadrant map.
			Ref<DebugQuadrant> new_quadrant;
			new_quadrant.instantiate();
			new_quadrant->quadrant_coords = quadrant_coords;
			debug_quadrant_map[quadrant_coords] = new_quadrant;
		}
	}

	// Second pass on modified cells to update the list of cells per quandrant.
	if (_debug_was_cleaned_up || anything_changed) {
		for (KeyValue<Vector2i, CellData>& kv : tile_map_layer_data) {
			CellData& cell_data = kv.value;
			Ref<DebugQuadrant> debug_quadrant = debug_quadrant_map[_coords_to_quadrant_coords(
				cell_data.coords, TILE_MAP_DEBUG_QUADRANT_SIZE)];
			if (!cell_data.debug_quadrant_list_element.in_list()) {
				debug_quadrant->cells.add(&cell_data.debug_quadrant_list_element);
			}
		}
	}
	else {
		for (SelfList<CellData>* cell_data_list_element = dirty.cell_list.first();
			 cell_data_list_element; cell_data_list_element = cell_data_list_element->next()) {
			CellData& cell_data = *cell_data_list_element->self();
			Ref<DebugQuadrant> debug_quadrant = debug_quadrant_map[_coords_to_quadrant_coords(
				cell_data.coords, TILE_MAP_DEBUG_QUADRANT_SIZE)];
			if (!cell_data.debug_quadrant_list_element.in_list()) {
				debug_quadrant->cells.add(&cell_data.debug_quadrant_list_element);
			}
		}
	}

	// Update those quadrants.
	bool needs_set_not_interpolated = is_inside_tree() &&
									  get_tree()->is_physics_interpolation_enabled() &&
									  !is_physics_interpolated();
	for (const Vector2i& quadrant_coords : quadrants_to_updates) {
		Ref<DebugQuadrant> debug_quadrant = debug_quadrant_map[quadrant_coords];

		// Update the quadrant's canvas item.
		RID& ci = debug_quadrant->canvas_item;
		if (ci.is_valid()) {
			rs->canvas_item_clear(ci);
		}
		else {
			ci = rs->canvas_item_create();
			if (needs_set_not_interpolated) {
				rs->canvas_item_set_interpolated(ci, false);
			}
			rs->canvas_item_set_z_index(ci, RSE::CANVAS_ITEM_Z_MAX - 1);
			rs->canvas_item_set_parent(ci, get_canvas_item());
		}
		const Vector2 quadrant_pos =
			tile_set->map_to_local(debug_quadrant->quadrant_coords * TILE_MAP_DEBUG_QUADRANT_SIZE);
		Transform2D xform(0, quadrant_pos);
		rs->canvas_item_set_transform(ci, xform);

#ifndef PHYSICS_2D_DISABLED
		// Draw physics.
		_physics_draw_quadrant_debug(ci, *debug_quadrant.ptr());
#endif // PHYSICS_2D_DISABLED

		// Draw debug info.
		for (SelfList<CellData>* cell_data_list_element = debug_quadrant->cells.first();
			 cell_data_list_element; cell_data_list_element = cell_data_list_element->next()) {
			CellData& cell_data = *cell_data_list_element->self();
			if (cell_data.cell.source_id != TileSet::INVALID_SOURCE) {
				_rendering_draw_cell_debug(ci, quadrant_pos, cell_data);
#ifndef NAVIGATION_2D_DISABLED
				_navigation_draw_cell_debug(ci, quadrant_pos, cell_data);
#endif // NAVIGATION_2D_DISABLED
				_scenes_draw_cell_debug(ci, quadrant_pos, cell_data);
				debug_quadrant->drawn_to = true;
			}
		}

		// Free the quadrants that were not drawn to.
		if (!debug_quadrant->drawn_to) {
			// Free the quadrant.
			if (ci.is_valid()) {
				rs->free_rid(ci);
			}
			debug_quadrant_map.erase(quadrant_coords);
		}
	}

	_debug_was_cleaned_up = false;
}

#endif // DEBUG_ENABLED

Color TileMapLayer::_highlight_color(const Color& p_modulate) const
{
	if (highlight_mode == HIGHLIGHT_MODE_BELOW) {
		return p_modulate.darkened(0.5);
	}
	if (highlight_mode == HIGHLIGHT_MODE_ABOVE) {
		Color c = p_modulate.darkened(0.5);
		c.a *= 0.3;
		return c;
	}
	return p_modulate;
}

/////////////////////////////// Rendering //////////////////////////////////////

void TileMapLayer::_rendering_notification(int p_what)
{
	RenderingServer* rs = RenderingServer::get_singleton();
	if (p_what == NOTIFICATION_TRANSFORM_CHANGED || p_what == NOTIFICATION_ENTER_CANVAS ||
		p_what == NOTIFICATION_VISIBILITY_CHANGED) {
		if (tile_set.is_valid()) {
			Transform2D tilemap_xform = get_global_transform();
			for (KeyValue<Vector2i, CellData>& kv : tile_map_layer_data) {
				const CellData& cell_data = kv.value;
				for (const LocalVector<RID>& polygons : cell_data.occluders) {
					for (const RID& rid : polygons) {
						if (rid.is_null()) {
							continue;
						}
						Transform2D xform(0, tile_set->map_to_local(kv.key));
						rs->canvas_light_occluder_attach_to_canvas(rid, get_canvas());
						rs->canvas_light_occluder_set_transform(rid, tilemap_xform * xform);
					}
				}
			}
		}
	}
	else if (p_what == NOTIFICATION_RESET_PHYSICS_INTERPOLATION) {
		if (is_physics_interpolated_and_enabled() && is_visible_in_tree()) {
			for (const KeyValue<Vector2i, Ref<RenderingQuadrant>>& kv : rendering_quadrant_map) {
				for (const RID& ci : kv.value->canvas_items) {
					if (ci.is_valid()) {
						rs->canvas_item_reset_physics_interpolation(ci);
					}
				}
			}
		}
	}
}

void TileMapLayer::_rendering_occluders_clear_cell(CellData& r_cell_data)
{
	RenderingServer* rs = RenderingServer::get_singleton();

	// Free the occluders.
	for (const LocalVector<RID>& polygons : r_cell_data.occluders) {
		for (const RID& rid : polygons) {
			rs->free_rid(rid);
		}
	}
	r_cell_data.occluders.clear();
}

#ifndef NAVIGATION_2D_DISABLED

void TileMapLayer::_navigation_update(bool p_force_cleanup)
{
	ERR_FAIL_NULL(NavigationServer2D::get_singleton());
	NavigationServer2D* ns = NavigationServer2D::get_singleton();

	// Check if we should cleanup everything.
	bool forced_cleanup = p_force_cleanup || !enabled || !navigation_enabled || !is_inside_tree() ||
						  tile_set.is_null();
	if (forced_cleanup && _navigation_was_cleaned_up) {
		return;
	}

	// ----------- Layer level processing -----------
	// All this processing is kept for compatibility with the TileMap node.
	// Otherwise, layers shall use the World2D navigation map or define a custom one with
	// set_navigation_map(...).
	if (tile_map_node) {
		if (forced_cleanup) {
			if (navigation_map_override.is_valid()) {
				ns->free_rid(navigation_map_override);
				navigation_map_override = RID();
			}
		}
		else {
			// Update navigation maps.
			if (!navigation_map_override.is_valid()) {
				if (layer_index_in_tile_map_node > 0) {
					// Create a dedicated map for each layer.
					RID new_layer_map = ns->map_create();
					// Set the default NavigationPolygon cell_size on the new map as a mismatch
					// causes an error.
					ns->map_set_cell_size(new_layer_map, NavigationDefaults2D::NAV_MESH_CELL_SIZE);
					ns->map_set_active(new_layer_map, true);
					navigation_map_override = new_layer_map;
				}
			}
		}
	}

	// ----------- Navigation regions processing -----------
	if (forced_cleanup) {
		// Clean everything.
		for (KeyValue<Vector2i, CellData>& kv : tile_map_layer_data) {
			_navigation_clear_cell(kv.value);
		}
	}
	else {
		if (_navigation_was_cleaned_up || dirty.flags[DIRTY_FLAGS_TILE_SET] ||
			dirty.flags[DIRTY_FLAGS_LAYER_IN_TREE] ||
			dirty.flags[DIRTY_FLAGS_LAYER_NAVIGATION_MAP]) {
			// Update all cells.
			for (KeyValue<Vector2i, CellData>& kv : tile_map_layer_data) {
				_navigation_update_cell(kv.value);
			}
		}
		else {
			// Update dirty cells.
			for (SelfList<CellData>* cell_data_list_element = dirty.cell_list.first();
				 cell_data_list_element; cell_data_list_element = cell_data_list_element->next()) {
				CellData& cell_data = *cell_data_list_element->self();
				_navigation_update_cell(cell_data);
			}
		}
	}

	// -----------
	// Mark the navigation state as up to date.
	_navigation_was_cleaned_up = forced_cleanup;
}

void TileMapLayer::_navigation_notification(int p_what)
{
	if (p_what == NOTIFICATION_TRANSFORM_CHANGED) {
		if (tile_set.is_valid()) {
			Transform2D tilemap_xform = get_global_transform();
			for (KeyValue<Vector2i, CellData>& kv : tile_map_layer_data) {
				const CellData& cell_data = kv.value;
				// Update navigation regions transform.
				for (const RID& region : cell_data.navigation_regions) {
					if (!region.is_valid()) {
						continue;
					}
					Transform2D tile_transform;
					tile_transform.set_origin(tile_set->map_to_local(kv.key));
					NavigationServer2D::get_singleton()->region_set_transform(
						region, tilemap_xform * tile_transform);
				}
			}
		}
	}
}

void TileMapLayer::_navigation_clear_cell(CellData& r_cell_data)
{
	NavigationServer2D* ns = NavigationServer2D::get_singleton();
	// Clear navigation shapes.
	for (uint32_t i = 0; i < r_cell_data.navigation_regions.size(); i++) {
		const RID& region = r_cell_data.navigation_regions[i];
		if (region.is_valid()) {
			ns->region_set_map(region, RID());
			ns->free_rid(region);
		}
	}
	r_cell_data.navigation_regions.clear();
}

#ifdef DEBUG_ENABLED

#endif // DEBUG_ENABLED
#endif // NAVIGATION_2D_DISABLED

void TileMapLayer::_scenes_update(bool p_force_cleanup)
{
	// Check if we should cleanup everything.
	bool forced_cleanup = p_force_cleanup || !enabled || !is_inside_tree() || tile_set.is_null();
	if (forced_cleanup && _scenes_was_cleaned_up) {
		return;
	}

	if (forced_cleanup) {
		// Clean everything.
		for (KeyValue<Vector2i, CellData>& kv : tile_map_layer_data) {
			_scenes_clear_cell(kv.value);
		}
	}
	else {
		if (_scenes_was_cleaned_up || dirty.flags[DIRTY_FLAGS_TILE_SET] ||
			dirty.flags[DIRTY_FLAGS_LAYER_IN_TREE] ||
			dirty.flags[DIRTY_FLAGS_LAYER_HIGHLIGHT_MODE]) {
			// Update all cells.
			for (KeyValue<Vector2i, CellData>& kv : tile_map_layer_data) {
				_scenes_update_cell(kv.value);
			}
		}
		else {
			// Update dirty cells.
			for (SelfList<CellData>* cell_data_list_element = dirty.cell_list.first();
				 cell_data_list_element; cell_data_list_element = cell_data_list_element->next()) {
				CellData& cell_data = *cell_data_list_element->self();
				_scenes_update_cell(cell_data);
			}
		}
	}

	// -----------
	// Mark the scenes state as up to date.
	_scenes_was_cleaned_up = forced_cleanup;
}

void TileMapLayer::_scenes_clear_cell(CellData& r_cell_data)
{
	// Cleanup existing scene.
	Node* node = nullptr;
	if (tile_map_node) {
		// Compatibility with TileMap.
		node = tile_map_node->get_node_or_null(r_cell_data.scene);
	}
	else {
		node = get_node_or_null(r_cell_data.scene);
	}
	if (node) {
		node->queue_free();
	}
	r_cell_data.scene = "";
}

#ifdef DEBUG_ENABLED

#endif // DEBUG_ENABLED

void TileMapLayer::_set_scene_transform_with_alternative(
	Node2D* p_scene, const Vector2& p_cell_position, const int p_alternative_id)
{
	// Determine the transformations based on the alternative ID.
	bool transform_flip_h = p_alternative_id & TileSetAtlasSource::TRANSFORM_FLIP_H;
	bool transform_flip_v = p_alternative_id & TileSetAtlasSource::TRANSFORM_FLIP_V;
	bool transform_transpose = p_alternative_id & TileSetAtlasSource::TRANSFORM_TRANSPOSE;

	int axis_h = transform_transpose ? 1 : 0;
	int axis_v = 1 - axis_h;

	Transform2D xform;
	xform[axis_h].x = transform_flip_h ? -1.0f : 1.0f;
	xform[axis_h].y = 0.0f;
	xform[axis_v].x = 0.0f;
	xform[axis_v].y = transform_flip_v ? -1.0f : 1.0f;
	xform.set_origin(p_cell_position);

	p_scene->set_transform(xform * p_scene->get_transform());
}

/////////////////////////////////////////////////////////////////////

void TileMapLayer::_build_runtime_update_tile_data(bool p_force_cleanup)
{
	// Check if we should cleanup everything.
	bool forced_cleanup =
		p_force_cleanup || !enabled || tile_set.is_null() || !is_visible_in_tree();
	if (!forced_cleanup) {
		if (_runtime_update_tile_data_was_cleaned_up || dirty.flags[DIRTY_FLAGS_TILE_SET]) {
			_runtime_update_needs_all_cells_cleaned_up = true;
			for (KeyValue<Vector2i, CellData>& E : tile_map_layer_data) {
				_build_runtime_update_tile_data_for_cell(E.value, false);
			}
		}
		else if (dirty.flags[DIRTY_FLAGS_LAYER_RUNTIME_UPDATE]) {
			for (KeyValue<Vector2i, CellData>& E : tile_map_layer_data) {
				_build_runtime_update_tile_data_for_cell(E.value, false, true);
			}
		}
		else {
			for (SelfList<CellData>* cell_data_list_element = dirty.cell_list.first();
				 cell_data_list_element; cell_data_list_element = cell_data_list_element->next()) {
				CellData& cell_data = *cell_data_list_element->self();
				_build_runtime_update_tile_data_for_cell(cell_data, false);
			}
		}
	}

	// -----------
	// Mark the tile data state as up to date.
	_runtime_update_tile_data_was_cleaned_up = forced_cleanup;
}

void TileMapLayer::_clear_runtime_update_tile_data()
{
	if (_runtime_update_needs_all_cells_cleaned_up) {
		for (KeyValue<Vector2i, CellData>& E : tile_map_layer_data) {
			_clear_runtime_update_tile_data_for_cell(E.value);
		}
		_runtime_update_needs_all_cells_cleaned_up = false;
	}
	else {
		for (SelfList<CellData>* cell_data_list_element = dirty.cell_list.first();
			 cell_data_list_element; cell_data_list_element = cell_data_list_element->next()) {
			CellData& r_cell_data = *cell_data_list_element->self();
			_clear_runtime_update_tile_data_for_cell(r_cell_data);
		}
	}
}

void TileMapLayer::_clear_runtime_update_tile_data_for_cell(CellData& r_cell_data)
{
	// Clear the runtime tile data.
	if (r_cell_data.runtime_tile_data_cache) {
		memdelete(r_cell_data.runtime_tile_data_cache);
		r_cell_data.runtime_tile_data_cache = nullptr;
	}
}

TileSet::TerrainsPattern TileMapLayer::_get_best_terrain_pattern_for_constraints(int p_terrain_set,
	const Vector2i& p_position, const RBSet<TerrainConstraint>& p_constraints,
	TileSet::TerrainsPattern p_current_pattern) const
{
	if (tile_set.is_null()) {
		return TileSet::TerrainsPattern();
	}
	// Returns all tiles compatible with the given constraints.
	RBMap<TileSet::TerrainsPattern, int> terrain_pattern_score;
	RBSet<TileSet::TerrainsPattern> pattern_set = tile_set->get_terrains_pattern_set(p_terrain_set);
	ERR_FAIL_COND_V(pattern_set.is_empty(), TileSet::TerrainsPattern());
	for (TileSet::TerrainsPattern& terrain_pattern : pattern_set) {
		int score = 0;

		// Check the center bit constraint.
		TerrainConstraint terrain_constraint =
			TerrainConstraint(tile_set, p_position, terrain_pattern.get_terrain());
		const RBSet<TerrainConstraint>::Element* in_set_constraint_element =
			p_constraints.find(terrain_constraint);
		if (in_set_constraint_element) {
			if (in_set_constraint_element->get().get_terrain() !=
				terrain_constraint.get_terrain()) {
				score += in_set_constraint_element->get().get_priority();
			}
		}
		else if (p_current_pattern.get_terrain() != terrain_pattern.get_terrain()) {
			continue; // Ignore a pattern that cannot keep bits without constraints unmodified.
		}

		// Check the surrounding bits
		bool invalid_pattern = false;
		for (int i = 0; i < TileSet::CELL_NEIGHBOR_MAX; i++) {
			TileSet::CellNeighbor bit = TileSet::CellNeighbor(i);
			if (tile_set->is_valid_terrain_peering_bit(p_terrain_set, bit)) {
				// Check if the bit is compatible with the constraints.
				TerrainConstraint terrain_bit_constraint = TerrainConstraint(
					tile_set, p_position, bit, terrain_pattern.get_terrain_peering_bit(bit));
				in_set_constraint_element = p_constraints.find(terrain_bit_constraint);
				if (in_set_constraint_element) {
					if (in_set_constraint_element->get().get_terrain() !=
						terrain_bit_constraint.get_terrain()) {
						score += in_set_constraint_element->get().get_priority();
					}
				}
				else if (p_current_pattern.get_terrain_peering_bit(bit) !=
						   terrain_pattern.get_terrain_peering_bit(bit)) {
					invalid_pattern = true; // Ignore a pattern that cannot keep bits without
											// constraints unmodified.
					break;
				}
			}
		}
		if (invalid_pattern) {
			continue;
		}

		terrain_pattern_score[terrain_pattern] = score;
	}

	// Compute the minimum score.
	TileSet::TerrainsPattern min_score_pattern = p_current_pattern;
	int min_score = INT32_MAX;
	for (KeyValue<TileSet::TerrainsPattern, int> E : terrain_pattern_score) {
		if (E.value < min_score) {
			min_score_pattern = E.key;
			min_score = E.value;
		}
	}

	return min_score_pattern;
}

RBSet<TerrainConstraint> TileMapLayer::_get_terrain_constraints_from_added_pattern(
	const Vector2i& p_position, int p_terrain_set,
	TileSet::TerrainsPattern p_terrains_pattern) const
{
	if (tile_set.is_null()) {
		return RBSet<TerrainConstraint>();
	}

	// Compute the constraints needed from the surrounding tiles.
	RBSet<TerrainConstraint> output;
	output.insert(TerrainConstraint(tile_set, p_position, p_terrains_pattern.get_terrain()));

	for (uint32_t i = 0; i < TileSet::CELL_NEIGHBOR_MAX; i++) {
		TileSet::CellNeighbor side = TileSet::CellNeighbor(i);
		if (tile_set->is_valid_terrain_peering_bit(p_terrain_set, side)) {
			TerrainConstraint c = TerrainConstraint(
				tile_set, p_position, side, p_terrains_pattern.get_terrain_peering_bit(side));
			output.insert(c);
		}
	}

	return output;
}

RBSet<TerrainConstraint> TileMapLayer::_get_terrain_constraints_from_painted_cells_list(
	const RBSet<Vector2i>& p_painted, int p_terrain_set, bool p_ignore_empty_terrains) const
{
	if (tile_set.is_null()) {
		return RBSet<TerrainConstraint>();
	}

	ERR_FAIL_INDEX_V(p_terrain_set, tile_set->get_terrain_sets_count(), RBSet<TerrainConstraint>());

	// Build a set of dummy constraints to get the constrained points.
	RBSet<TerrainConstraint> dummy_constraints;
	for (const Vector2i& E : p_painted) {
		for (int i = 0; i < TileSet::CELL_NEIGHBOR_MAX; i++) { // Iterates over neighbor bits.
			TileSet::CellNeighbor bit = TileSet::CellNeighbor(i);
			if (tile_set->is_valid_terrain_peering_bit(p_terrain_set, bit)) {
				dummy_constraints.insert(TerrainConstraint(tile_set, E, bit, -1));
			}
		}
	}

	// For each constrained point, we get all overlapping tiles, and select the most adequate
	// terrain for it.
	RBSet<TerrainConstraint> constraints;
	for (const TerrainConstraint& E_constraint : dummy_constraints) {
		HashMap<int, int> terrain_count;

		// Count the number of occurrences per terrain.
		HashMap<Vector2i, TileSet::CellNeighbor> overlapping_terrain_bits =
			E_constraint.get_overlapping_coords_and_peering_bits();
		for (const KeyValue<Vector2i, TileSet::CellNeighbor>& E_overlapping :
			overlapping_terrain_bits) {
			TileData* neighbor_tile_data = nullptr;
			TileMapCell neighbor_cell = get_cell(E_overlapping.key);
			if (neighbor_cell.source_id != TileSet::INVALID_SOURCE) {
				Ref<TileSetSource> source = tile_set->get_source(neighbor_cell.source_id);
				Ref<TileSetAtlasSource> atlas_source = source;
				if (atlas_source.is_valid()) {
					TileData* tile_data = atlas_source->get_tile_data(
						neighbor_cell.get_atlas_coords(), neighbor_cell.alternative_tile);
					if (tile_data && tile_data->get_terrain_set() == p_terrain_set) {
						neighbor_tile_data = tile_data;
					}
				}
			}

			int terrain = neighbor_tile_data ? neighbor_tile_data->get_terrain_peering_bit(
												   TileSet::CellNeighbor(E_overlapping.value))
											 : -1;
			if (!p_ignore_empty_terrains || terrain >= 0) {
				if (!terrain_count.has(terrain)) {
					terrain_count[terrain] = 0;
				}
				terrain_count[terrain] += 1;
			}
		}

		// Get the terrain with the max number of occurrences.
		int max = 0;
		int max_terrain = -1;
		for (const KeyValue<int, int>& E_terrain_count : terrain_count) {
			if (E_terrain_count.value > max) {
				max = E_terrain_count.value;
				max_terrain = E_terrain_count.key;
			}
		}

		// Set the adequate terrain.
		if (max > 0) {
			TerrainConstraint c = E_constraint;
			c.set_terrain(max_terrain);
			constraints.insert(c);
		}
	}

	// Add the centers as constraints.
	for (Vector2i E_coords : p_painted) {
		TileData* tile_data = nullptr;
		TileMapCell cell = get_cell(E_coords);
		if (cell.source_id != TileSet::INVALID_SOURCE) {
			Ref<TileSetSource> source = tile_set->get_source(cell.source_id);
			Ref<TileSetAtlasSource> atlas_source = source;
			if (atlas_source.is_valid()) {
				tile_data =
					atlas_source->get_tile_data(cell.get_atlas_coords(), cell.alternative_tile);
			}
		}

		int terrain = (tile_data && tile_data->get_terrain_set() == p_terrain_set)
						  ? tile_data->get_terrain()
						  : -1;
		if (!p_ignore_empty_terrains || terrain >= 0) {
			constraints.insert(TerrainConstraint(tile_set, E_coords, terrain));
		}
	}

	return constraints;
}

void TileMapLayer::_update_notify_local_transform()
{
	bool notify = is_using_kinematic_bodies() || is_y_sort_enabled();
	if (!notify) {
		if (is_y_sort_enabled()) {
			notify = true;
		}
	}
	set_notify_local_transform(notify);
}

void TileMapLayer::_deferred_internal_update()
{
	// Other updates.
	if (!pending_update) {
		return;
	}

	// Update dirty quadrants on layers.
	_internal_update(false);
}

void TileMapLayer::_internal_update(bool p_force_cleanup)
{
	// Find TileData that need a runtime modification.
	// This may add cells to the dirty list if a runtime modification has been notified.
	_build_runtime_update_tile_data(p_force_cleanup);

	// Callback for implementing custom subsystems.
	// This may add to the dirty list if some cells are changed inside _update_cells.
	_update_cells_callback(p_force_cleanup);

	// Update all subsystems.
	_rendering_update(p_force_cleanup);
#ifndef PHYSICS_2D_DISABLED
	_physics_update(p_force_cleanup);
#endif // PHYSICS_2D_DISABLED
#ifndef NAVIGATION_2D_DISABLED
	_navigation_update(p_force_cleanup);
#endif // NAVIGATION_2D_DISABLED
	_scenes_update(p_force_cleanup);
#ifdef DEBUG_ENABLED
	_debug_update(p_force_cleanup);
#endif // DEBUG_ENABLED

	_clear_runtime_update_tile_data();

	// Clear the "what is dirty" flags.
	for (int i = 0; i < DIRTY_FLAGS_MAX; i++) {
		dirty.flags[i] = false;
	}

	// List the cells to delete definitely.
	Vector<Vector2i> to_delete;
	for (SelfList<CellData>* cell_data_list_element = dirty.cell_list.first();
		 cell_data_list_element; cell_data_list_element = cell_data_list_element->next()) {
		CellData& cell_data = *cell_data_list_element->self();
		// Select the cell from tile_map if it is invalid.
		if (cell_data.cell.source_id == TileSet::INVALID_SOURCE) {
			to_delete.push_back(cell_data.coords);
		}
	}

	// Remove cells that are empty after the cleanup.
	for (const Vector2i& coords : to_delete) {
		tile_map_layer_data.erase(coords);
	}

	// Clear the dirty cells list.
	dirty.cell_list.clear();

	pending_update = false;
}

void TileMapLayer::_physics_interpolated_changed()
{
	RenderingServer* rs = RenderingServer::get_singleton();

	bool interpolated = is_physics_interpolated();
	bool needs_reset = interpolated && is_visible_in_tree();

	for (const KeyValue<Vector2i, Ref<RenderingQuadrant>>& kv : rendering_quadrant_map) {
		for (const RID& ci : kv.value->canvas_items) {
			if (ci.is_valid()) {
				rs->canvas_item_set_interpolated(ci, interpolated);
				if (needs_reset) {
					rs->canvas_item_reset_physics_interpolation(ci);
				}
			}
		}
	}

	for (const KeyValue<Vector2i, CellData>& E : tile_map_layer_data) {
		for (const LocalVector<RID>& polygons : E.value.occluders) {
			for (const RID& occluder_id : polygons) {
				if (occluder_id.is_valid()) {
					rs->canvas_light_occluder_set_interpolated(occluder_id, interpolated);
					if (needs_reset) {
						rs->canvas_light_occluder_reset_physics_interpolation(occluder_id);
					}
				}
			}
		}
	}
}

#ifdef TOOLS_ENABLED
bool TileMapLayer::_edit_is_selected_on_click(const Point2& p_point, double p_tolerance) const
{
	return tile_set.is_valid() &&
		   get_cell_source_id(local_to_map(p_point)) != TileSet::INVALID_SOURCE;
}
#endif

Rect2 TileMapLayer::get_rect(bool& r_changed) const
{
	if (tile_set.is_null()) {
		r_changed = rect_cache != Rect2();
		return Rect2();
	}

	// Compute the displayed area of the tilemap.
	r_changed = false;
#ifdef DEBUG_ENABLED

	if (rect_cache_dirty) {
		Rect2 r_total;
		bool first = true;
		for (const KeyValue<Vector2i, CellData>& E : tile_map_layer_data) {
			Rect2 r;
			r.position = tile_set->map_to_local(E.key);
			r.size = Size2();
			if (first) {
				r_total = r;
				first = false;
			}
			else {
				r_total = r_total.merge(r);
			}
		}

		r_changed = rect_cache != r_total;

		rect_cache = r_total;
		rect_cache_dirty = false;
	}
#endif
	return rect_cache;
}

HashMap<Vector2i, TileSet::TerrainsPattern> TileMapLayer::terrain_fill_connect(
	const Vector<Vector2i>& p_coords_array, int p_terrain_set, int p_terrain,
	bool p_ignore_empty_terrains) const
{
	ERR_FAIL_COND_V(tile_set.is_null(), (HashMap<Vector2i, TileSet::TerrainsPattern>()));
	ERR_FAIL_INDEX_V(p_terrain_set, tile_set->get_terrain_sets_count(),
		(HashMap<Vector2i, TileSet::TerrainsPattern>()));

	// Build list and set of tiles that can be modified (painted and their surroundings).
	Vector<Vector2i> can_modify_list;
	RBSet<Vector2i> can_modify_set;
	RBSet<Vector2i> painted_set;
	for (int i = p_coords_array.size() - 1; i >= 0; i--) {
		const Vector2i& coords = p_coords_array[i];
		can_modify_list.push_back(coords);
		can_modify_set.insert(coords);
		painted_set.insert(coords);
	}
	for (Vector2i coords : p_coords_array) {
		// Find the adequate neighbor.
		for (int j = 0; j < TileSet::CELL_NEIGHBOR_MAX; j++) {
			TileSet::CellNeighbor bit = TileSet::CellNeighbor(j);
			if (tile_set->is_existing_neighbor(bit)) {
				Vector2i neighbor = tile_set->get_neighbor_cell(coords, bit);
				if (!can_modify_set.has(neighbor)) {
					can_modify_list.push_back(neighbor);
					can_modify_set.insert(neighbor);
				}
			}
		}
	}

	// Build a set, out of the possibly modified tiles, of the one with a center bit that is set (or
	// will be) to the painted terrain.
	RBSet<Vector2i> cells_with_terrain_center_bit;
	for (Vector2i coords : can_modify_set) {
		bool connect = false;
		if (painted_set.has(coords)) {
			connect = true;
		}
		else {
			// Get the center bit of the cell.
			TileData* tile_data = nullptr;
			TileMapCell cell = get_cell(coords);
			if (cell.source_id != TileSet::INVALID_SOURCE) {
				Ref<TileSetSource> source = tile_set->get_source(cell.source_id);
				Ref<TileSetAtlasSource> atlas_source = source;
				if (atlas_source.is_valid()) {
					tile_data =
						atlas_source->get_tile_data(cell.get_atlas_coords(), cell.alternative_tile);
				}
			}

			if (tile_data && tile_data->get_terrain_set() == p_terrain_set &&
				tile_data->get_terrain() == p_terrain) {
				connect = true;
			}
		}
		if (connect) {
			cells_with_terrain_center_bit.insert(coords);
		}
	}

	RBSet<TerrainConstraint> constraints;

	// Add new constraints from the path drawn.
	for (Vector2i coords : p_coords_array) {
		// Constraints on the center bit.
		TerrainConstraint c = TerrainConstraint(tile_set, coords, p_terrain);
		c.set_priority(10);
		constraints.insert(c);

		// Constraints on the connecting bits.
		for (int j = 0; j < TileSet::CELL_NEIGHBOR_MAX; j++) {
			TileSet::CellNeighbor bit = TileSet::CellNeighbor(j);
			if (tile_set->is_valid_terrain_peering_bit(p_terrain_set, bit)) {
				c = TerrainConstraint(tile_set, coords, bit, p_terrain);
				c.set_priority(10);
				if ((int(bit) % 2) == 0) {
					// Side peering bits: add the constraint if the center is of the same terrain.
					Vector2i neighbor = tile_set->get_neighbor_cell(coords, bit);
					if (cells_with_terrain_center_bit.has(neighbor)) {
						constraints.insert(c);
					}
				}
				else {
					// Corner peering bits: add the constraint if all tiles on the constraint has
					// the same center bit.
					HashMap<Vector2i, TileSet::CellNeighbor> overlapping_terrain_bits =
						c.get_overlapping_coords_and_peering_bits();
					bool valid = true;
					for (KeyValue<Vector2i, TileSet::CellNeighbor> kv : overlapping_terrain_bits) {
						if (!cells_with_terrain_center_bit.has(kv.key)) {
							valid = false;
							break;
						}
					}
					if (valid) {
						constraints.insert(c);
					}
				}
			}
		}
	}

	// Fills in the constraint list from existing tiles.
	for (TerrainConstraint c : _get_terrain_constraints_from_painted_cells_list(
			 painted_set, p_terrain_set, p_ignore_empty_terrains)) {
		constraints.insert(c);
	}

	// Fill the terrains.
	return terrain_fill_constraints(can_modify_list, p_terrain_set, constraints);
}

HashMap<Vector2i, TileSet::TerrainsPattern> TileMapLayer::terrain_fill_path(
	const Vector<Vector2i>& p_coords_array, int p_terrain_set, int p_terrain,
	bool p_ignore_empty_terrains) const
{
	ERR_FAIL_COND_V(tile_set.is_null(), (HashMap<Vector2i, TileSet::TerrainsPattern>()));
	ERR_FAIL_INDEX_V(p_terrain_set, tile_set->get_terrain_sets_count(),
		(HashMap<Vector2i, TileSet::TerrainsPattern>()));
	HashMap<Vector2i, TileSet::TerrainsPattern> output;

	// Make sure the path is correct and build the peering bit list while doing it.
	Vector<TileSet::CellNeighbor> neighbor_list;
	for (int i = 0; i < p_coords_array.size() - 1; i++) {
		// Find the adequate neighbor.
		TileSet::CellNeighbor found_bit = TileSet::CELL_NEIGHBOR_MAX;
		for (int j = 0; j < TileSet::CELL_NEIGHBOR_MAX; j++) {
			TileSet::CellNeighbor bit = TileSet::CellNeighbor(j);
			if (tile_set->is_existing_neighbor(bit)) {
				if (tile_set->get_neighbor_cell(p_coords_array[i], bit) == p_coords_array[i + 1]) {
					found_bit = bit;
					break;
				}
			}
		}
		ERR_FAIL_COND_V_MSG(found_bit == TileSet::CELL_NEIGHBOR_MAX, output,
			vformat("Invalid terrain path, %s is not a neighboring tile of %s",
				p_coords_array[i + 1], p_coords_array[i]));
		neighbor_list.push_back(found_bit);
	}

	// Build list and set of tiles that can be modified (painted and their surroundings).
	Vector<Vector2i> can_modify_list;
	RBSet<Vector2i> can_modify_set;
	RBSet<Vector2i> painted_set;
	for (int i = p_coords_array.size() - 1; i >= 0; i--) {
		const Vector2i& coords = p_coords_array[i];
		can_modify_list.push_back(coords);
		can_modify_set.insert(coords);
		painted_set.insert(coords);
	}
	for (Vector2i coords : p_coords_array) {
		// Find the adequate neighbor.
		for (int j = 0; j < TileSet::CELL_NEIGHBOR_MAX; j++) {
			TileSet::CellNeighbor bit = TileSet::CellNeighbor(j);
			if (tile_set->is_valid_terrain_peering_bit(p_terrain_set, bit)) {
				Vector2i neighbor = tile_set->get_neighbor_cell(coords, bit);
				if (!can_modify_set.has(neighbor)) {
					can_modify_list.push_back(neighbor);
					can_modify_set.insert(neighbor);
				}
			}
		}
	}

	RBSet<TerrainConstraint> constraints;

	// Add new constraints from the path drawn.
	for (Vector2i coords : p_coords_array) {
		// Constraints on the center bit.
		TerrainConstraint c = TerrainConstraint(tile_set, coords, p_terrain);
		c.set_priority(10);
		constraints.insert(c);
	}
	for (int i = 0; i < p_coords_array.size() - 1; i++) {
		// Constraints on the peering bits.
		TerrainConstraint c =
			TerrainConstraint(tile_set, p_coords_array[i], neighbor_list[i], p_terrain);
		c.set_priority(10);
		constraints.insert(c);
	}

	// Fills in the constraint list from existing tiles.
	for (TerrainConstraint c : _get_terrain_constraints_from_painted_cells_list(
			 painted_set, p_terrain_set, p_ignore_empty_terrains)) {
		constraints.insert(c);
	}

	// Fill the terrains.
	return terrain_fill_constraints(can_modify_list, p_terrain_set, constraints);
}

HashMap<Vector2i, TileSet::TerrainsPattern> TileMapLayer::terrain_fill_pattern(
	const Vector<Vector2i>& p_coords_array, int p_terrain_set,
	TileSet::TerrainsPattern p_terrains_pattern, bool p_ignore_empty_terrains) const
{
	ERR_FAIL_COND_V(tile_set.is_null(), (HashMap<Vector2i, TileSet::TerrainsPattern>()));
	ERR_FAIL_INDEX_V(p_terrain_set, tile_set->get_terrain_sets_count(),
		(HashMap<Vector2i, TileSet::TerrainsPattern>()));

	// Build list and set of tiles that can be modified (painted and their surroundings).
	Vector<Vector2i> can_modify_list;
	RBSet<Vector2i> can_modify_set;
	RBSet<Vector2i> painted_set;
	for (int i = p_coords_array.size() - 1; i >= 0; i--) {
		const Vector2i& coords = p_coords_array[i];
		can_modify_list.push_back(coords);
		can_modify_set.insert(coords);
		painted_set.insert(coords);
	}
	for (Vector2i coords : p_coords_array) {
		// Find the adequate neighbor.
		for (int j = 0; j < TileSet::CELL_NEIGHBOR_MAX; j++) {
			TileSet::CellNeighbor bit = TileSet::CellNeighbor(j);
			if (tile_set->is_existing_neighbor(bit)) {
				Vector2i neighbor = tile_set->get_neighbor_cell(coords, bit);
				if (!can_modify_set.has(neighbor)) {
					can_modify_list.push_back(neighbor);
					can_modify_set.insert(neighbor);
				}
			}
		}
	}

	// Add constraint by the new ones.
	RBSet<TerrainConstraint> constraints;

	// Add new constraints from the path drawn.
	for (Vector2i coords : p_coords_array) {
		// Constraints on the center bit.
		RBSet<TerrainConstraint> added_constraints =
			_get_terrain_constraints_from_added_pattern(coords, p_terrain_set, p_terrains_pattern);
		for (TerrainConstraint c : added_constraints) {
			c.set_priority(10);
			constraints.insert(c);
		}
	}

	// Fills in the constraint list from modified tiles border.
	for (TerrainConstraint c : _get_terrain_constraints_from_painted_cells_list(
			 painted_set, p_terrain_set, p_ignore_empty_terrains)) {
		constraints.insert(c);
	}

	// Fill the terrains.
	return terrain_fill_constraints(can_modify_list, p_terrain_set, constraints);
}

TileMapCell TileMapLayer::get_cell(const Vector2i& p_coords) const
{
	if (!tile_map_layer_data.has(p_coords)) {
		return TileMapCell();
	}
	else {
		return tile_map_layer_data.find(p_coords)->value.cell;
	}
}

void TileMapLayer::compute_transformed_tile_dest_rect(Rect2& r_dest_rect, bool& r_transpose,
	const Vector2& p_position, const Vector2& p_dest_rect_size, const TileData* p_tile_data,
	int p_alternative_tile)
{
	DEV_ASSERT(p_tile_data);
	// Conceptually the order of transformations is (starting from the tile centered at the origin):
	// - Per TileSet-tile transforms (transpose then flips).
	// - Translation so texture origin is at the origin.
	// - Per TileMapLayer-cell transforms (transpose then flips).
	// - Translation to target position.

	const bool tile_transpose = p_tile_data->get_transpose();
	const bool tile_flip_h = p_tile_data->get_flip_h();
	const bool tile_flip_v = p_tile_data->get_flip_v();

	const Vector2 texture_origin = p_tile_data->get_texture_origin();

	const bool cell_transpose = bool(p_alternative_tile & TileSetAtlasSource::TRANSFORM_TRANSPOSE);
	const bool cell_flip_h = bool(p_alternative_tile & TileSetAtlasSource::TRANSFORM_FLIP_H);
	const bool cell_flip_v = bool(p_alternative_tile & TileSetAtlasSource::TRANSFORM_FLIP_V);

	const bool final_transpose = tile_transpose != cell_transpose;
	const bool final_flip_h = cell_flip_h != (cell_transpose ? tile_flip_v : tile_flip_h);
	const bool final_flip_v = cell_flip_v != (cell_transpose ? tile_flip_h : tile_flip_v);

	// Rect draw commands swap the size based on the passed transpose, so the size is left
	// non-tranposed here. Position calculations need to use transposed size though.
	Rect2 dest_rect;
	dest_rect.size = p_dest_rect_size;
	dest_rect.size.x += FP_ADJUST;
	dest_rect.size.y += FP_ADJUST;
	Vector2 transposed_size =
		final_transpose ? Vector2(dest_rect.size.y, dest_rect.size.x) : dest_rect.size;
	if (final_flip_h) {
		dest_rect.size.x = -dest_rect.size.x;
	}
	if (final_flip_v) {
		dest_rect.size.y = -dest_rect.size.y;
	}

	dest_rect.position = -0.5f * transposed_size;
	dest_rect.position -=
		cell_transpose ? Vector2(texture_origin.y, texture_origin.x) : texture_origin;
	if (cell_flip_h) {
		dest_rect.position.x = -(dest_rect.position.x + transposed_size.x);
	}
	if (cell_flip_v) {
		dest_rect.position.y = -(dest_rect.position.y + transposed_size.y);
	}
	dest_rect.position += p_position;

	r_dest_rect = dest_rect;
	r_transpose = final_transpose;
}

void TileMapLayer::set_cell(const Vector2i& p_coords, int p_source_id,
	const Vector2i& p_atlas_coords, int p_alternative_tile)
{
	// Set the current cell tile (using integer position).
	Vector2i pk(p_coords);
	HashMap<Vector2i, CellData>::Iterator E = tile_map_layer_data.find(pk);

	int source_id = p_source_id;
	Vector2i atlas_coords = p_atlas_coords;
	int alternative_tile = p_alternative_tile;

	if ((source_id == TileSet::INVALID_SOURCE ||
			atlas_coords == TileSetSource::INVALID_ATLAS_COORDS ||
			alternative_tile == TileSetSource::INVALID_TILE_ALTERNATIVE) &&
		(source_id != TileSet::INVALID_SOURCE ||
			atlas_coords != TileSetSource::INVALID_ATLAS_COORDS ||
			alternative_tile != TileSetSource::INVALID_TILE_ALTERNATIVE)) {
		source_id = TileSet::INVALID_SOURCE;
		atlas_coords = TileSetSource::INVALID_ATLAS_COORDS;
		alternative_tile = TileSetSource::INVALID_TILE_ALTERNATIVE;
	}

	if (!E) {
		if (source_id == TileSet::INVALID_SOURCE) {
			return; // Nothing to do, the tile is already empty.
		}

		// Insert a new cell in the tile map.
		CellData new_cell_data;
		new_cell_data.coords = pk;
		E = tile_map_layer_data.insert(pk, new_cell_data);
	}
	else {
		if (E->value.cell.source_id == source_id &&
			E->value.cell.get_atlas_coords() == atlas_coords &&
			E->value.cell.alternative_tile == alternative_tile) {
			return; // Nothing changed.
		}
	}

	TileMapCell& c = E->value.cell;
	c.source_id = source_id;
	c.set_atlas_coords(atlas_coords);
	c.alternative_tile = alternative_tile;

	// Make the given cell dirty.
	if (!E->value.dirty_list_element.in_list()) {
		dirty.cell_list.add(&(E->value.dirty_list_element));
	}
	_queue_internal_update();

	used_rect_cache_dirty = true;
}

void TileMapLayer::erase_cell(const Vector2i& p_coords)
{
	set_cell(p_coords, TileSet::INVALID_SOURCE, TileSetSource::INVALID_ATLAS_COORDS,
		TileSetSource::INVALID_TILE_ALTERNATIVE);
}

void TileMapLayer::clear()
{
	// Remove all tiles.
	for (KeyValue<Vector2i, CellData>& kv : tile_map_layer_data) {
		erase_cell(kv.key);
	}
	used_rect_cache_dirty = true;
}

int TileMapLayer::get_cell_source_id(const Vector2i& p_coords) const
{
	// Get a cell source id from position.
	HashMap<Vector2i, CellData>::ConstIterator E = tile_map_layer_data.find(p_coords);

	if (!E) {
		return TileSet::INVALID_SOURCE;
	}

	return E->value.cell.source_id;
}

Vector2i TileMapLayer::get_cell_atlas_coords(const Vector2i& p_coords) const
{
	// Get a cell source id from position.
	HashMap<Vector2i, CellData>::ConstIterator E = tile_map_layer_data.find(p_coords);

	if (!E) {
		return TileSetSource::INVALID_ATLAS_COORDS;
	}

	return E->value.cell.get_atlas_coords();
}

int TileMapLayer::get_cell_alternative_tile(const Vector2i& p_coords) const
{
	// Get a cell source id from position.
	HashMap<Vector2i, CellData>::ConstIterator E = tile_map_layer_data.find(p_coords);

	if (!E) {
		return TileSetSource::INVALID_TILE_ALTERNATIVE;
	}

	return E->value.cell.alternative_tile;
}

TileData* TileMapLayer::get_cell_tile_data(const Vector2i& p_coords) const
{
	int source_id = get_cell_source_id(p_coords);
	if (source_id == TileSet::INVALID_SOURCE) {
		return nullptr;
	}

	Ref<TileSetAtlasSource> source = tile_set->get_source(source_id);
	if (source.is_valid()) {
		return source->get_tile_data(
			get_cell_atlas_coords(p_coords), get_cell_alternative_tile(p_coords));
	}

	return nullptr;
}

Rect2i TileMapLayer::get_used_rect() const
{
	// Return the rect of the currently used area.
	if (used_rect_cache_dirty) {
		used_rect_cache = Rect2i();

		bool first = true;
		for (const KeyValue<Vector2i, CellData>& E : tile_map_layer_data) {
			const TileMapCell& c = E.value.cell;
			if (c.source_id == TileSet::INVALID_SOURCE) {
				continue;
			}
			if (first) {
				used_rect_cache = Rect2i(E.key, Size2i());
				first = false;
			}
			else {
				used_rect_cache.expand_to(E.key);
			}
		}
		if (!first) {
			// Only if we have at least one cell.
			// The cache expands to top-left coordinate, so we add one full tile.
			used_rect_cache.size += Vector2i(1, 1);
		}
		used_rect_cache_dirty = false;
	}

	return used_rect_cache;
}

bool TileMapLayer::is_cell_flipped_h(const Vector2i& p_coords) const
{
	return get_cell_alternative_tile(p_coords) & TileSetAtlasSource::TRANSFORM_FLIP_H;
}

bool TileMapLayer::is_cell_flipped_v(const Vector2i& p_coords) const
{
	return get_cell_alternative_tile(p_coords) & TileSetAtlasSource::TRANSFORM_FLIP_V;
}

bool TileMapLayer::is_cell_transposed(const Vector2i& p_coords) const
{
	return get_cell_alternative_tile(p_coords) & TileSetAtlasSource::TRANSFORM_TRANSPOSE;
}

#ifndef PHYSICS_2D_DISABLED
bool TileMapLayer::has_body_rid(RID p_physics_body) const
{
	return bodies_coords.has(p_physics_body);
}

Vector2i TileMapLayer::get_coords_for_body_rid(RID p_physics_body) const
{
	const Vector2i* found = bodies_coords.getptr(p_physics_body);
	ERR_FAIL_NULL_V(found, Vector2i());
	return *found;
}
#endif // PHYSICS_2D_DISABLED

void TileMapLayer::update_internals() { _internal_update(false); }

Vector2i TileMapLayer::map_pattern(const Vector2i& p_position_in_tilemap,
	const Vector2i& p_coords_in_pattern, Ref<TileMapPattern> p_pattern)
{
	ERR_FAIL_COND_V(tile_set.is_null(), Vector2i());
	return tile_set->map_pattern(p_position_in_tilemap, p_coords_in_pattern, p_pattern);
}

Vector2i TileMapLayer::get_neighbor_cell(
	const Vector2i& p_coords, TileSet::CellNeighbor p_cell_neighbor) const
{
	ERR_FAIL_COND_V(tile_set.is_null(), Vector2i());
	return tile_set->get_neighbor_cell(p_coords, p_cell_neighbor);
}

Vector2 TileMapLayer::map_to_local(const Vector2i& p_pos) const
{
	ERR_FAIL_COND_V(tile_set.is_null(), Vector2());
	return tile_set->map_to_local(p_pos);
}

Vector2i TileMapLayer::local_to_map(const Vector2& p_pos) const
{
	ERR_FAIL_COND_V(tile_set.is_null(), Vector2i());
	return tile_set->local_to_map(p_pos);
}

bool TileMapLayer::is_enabled() const { return enabled; }

Ref<TileSet> TileMapLayer::get_tile_set() const { return tile_set; }

void TileMapLayer::set_highlight_mode(HighlightMode p_highlight_mode)
{
	if (p_highlight_mode == highlight_mode) {
		return;
	}
	highlight_mode = p_highlight_mode;
	dirty.flags[DIRTY_FLAGS_LAYER_HIGHLIGHT_MODE] = true;
	_queue_internal_update();
}

TileMapLayer::HighlightMode TileMapLayer::get_highlight_mode() const { return highlight_mode; }

void TileMapLayer::set_tile_map_data_from_array(const Vector<uint8_t>& p_data)
{
	if (p_data.is_empty()) {
		clear();
		return;
	}

	const int cell_data_struct_size = 12;

	int size = p_data.size();
	const uint8_t* ptr = p_data.ptr();

	// Index in the array.
	int index = 0;

	// First extract the data version.
	ERR_FAIL_COND_MSG(size < 2, "Corrupted tile map data: not enough bytes.");
	uint16_t format = decode_uint16(&ptr[index]);
	index += 2;
	ERR_FAIL_COND_MSG(format >= TileMapLayerDataFormat::TILE_MAP_LAYER_DATA_FORMAT_MAX,
		vformat("Unsupported tile map data format: %s. Expected format ID lower or equal to: %s",
			format, TileMapLayerDataFormat::TILE_MAP_LAYER_DATA_FORMAT_MAX - 1));

	// Clear the TileMap.
	clear();

	while (index < size) {
		ERR_FAIL_COND_MSG(index + cell_data_struct_size > size,
			vformat("Corrupted tile map data: tiles might be missing."));

		// Get a pointer at the start of the cell data.
		const uint8_t* cell_data_ptr = &ptr[index];

		// Extracts position in TileMap.
		int16_t x = decode_uint16(&cell_data_ptr[0]);
		int16_t y = decode_uint16(&cell_data_ptr[2]);

		// Extracts the tile identifiers.
		uint16_t source_id = decode_uint16(&cell_data_ptr[4]);
		uint16_t atlas_coords_x = decode_uint16(&cell_data_ptr[6]);
		uint16_t atlas_coords_y = decode_uint16(&cell_data_ptr[8]);
		uint16_t alternative_tile = decode_uint16(&cell_data_ptr[10]);

		set_cell(
			Vector2i(x, y), source_id, Vector2i(atlas_coords_x, atlas_coords_y), alternative_tile);
		index += cell_data_struct_size;
	}
}

Vector<uint8_t> TileMapLayer::get_tile_map_data_as_array() const
{
	const int cell_data_struct_size = 12;

	Vector<uint8_t> tile_map_data_array;
	if (tile_map_layer_data.is_empty()) {
		return tile_map_data_array;
	}

	tile_map_data_array.resize(2 + tile_map_layer_data.size() * cell_data_struct_size);
	uint8_t* ptr = tile_map_data_array.ptrw();

	// Index in the array.
	int index = 0;

	// Save the version.
	encode_uint16(TileMapLayerDataFormat::TILE_MAP_LAYER_DATA_FORMAT_MAX - 1, &ptr[index]);
	index += 2;

	// Save in highest format.
	for (const KeyValue<Vector2i, CellData>& E : tile_map_layer_data) {
		// Get a pointer at the start of the cell data.
		uint8_t* cell_data_ptr = (uint8_t*)&ptr[index];

		// Store position in TileMap.
		encode_uint16((int16_t)(E.key.x), &cell_data_ptr[0]);
		encode_uint16((int16_t)(E.key.y), &cell_data_ptr[2]);

		// Store the tile identifiers.
		encode_uint16(E.value.cell.source_id, &cell_data_ptr[4]);
		encode_uint16(E.value.cell.coord_x, &cell_data_ptr[6]);
		encode_uint16(E.value.cell.coord_y, &cell_data_ptr[8]);
		encode_uint16(E.value.cell.alternative_tile, &cell_data_ptr[10]);

		index += cell_data_struct_size;
	}

	return tile_map_data_array;
}

int TileMapLayer::get_y_sort_origin() const { return y_sort_origin; }

bool TileMapLayer::is_x_draw_order_reversed() const { return x_draw_order_reversed; }

int TileMapLayer::get_rendering_quadrant_size() const { return rendering_quadrant_size; }

bool TileMapLayer::is_collision_enabled() const { return collision_enabled; }

bool TileMapLayer::is_using_kinematic_bodies() const { return use_kinematic_bodies; }

TileMapLayer::DebugVisibilityMode TileMapLayer::get_collision_visibility_mode() const
{
	return collision_visibility_mode;
}

int TileMapLayer::get_physics_quadrant_size() const { return physics_quadrant_size; }

bool TileMapLayer::is_occlusion_enabled() const { return occlusion_enabled; }

#ifndef NAVIGATION_2D_DISABLED

bool TileMapLayer::is_navigation_enabled() const { return navigation_enabled; }

RID TileMapLayer::get_navigation_map() const
{
	if (navigation_map_override.is_valid()) {
		return navigation_map_override;
	}
	else if (is_inside_tree()) {
		return get_world_2d()->get_navigation_map();
	}
	return RID();
}

TileMapLayer::DebugVisibilityMode TileMapLayer::get_navigation_visibility_mode() const
{
	return navigation_visibility_mode;
}

#endif // NAVIGATION_2D_DISABLED

TileMapLayer::TileMapLayer() { set_notify_transform(true); }

TileMapLayer::~TileMapLayer()
{
	clear();
	_internal_update(true);
}

HashMap<Vector2i, TileSet::CellNeighbor>
TerrainConstraint::get_overlapping_coords_and_peering_bits() const
{
	ERR_FAIL_COND_V(is_center_bit(), (HashMap<Vector2i, TileSet::CellNeighbor>()));
	ERR_FAIL_COND_V(tile_set.is_null(), (HashMap<Vector2i, TileSet::CellNeighbor>()));
	HashMap<Vector2i, TileSet::CellNeighbor> output;

	TileSet::TileShape shape = tile_set->get_tile_shape();
	if (shape == TileSet::TILE_SHAPE_SQUARE) {
		switch (bit) {
		case 1:
			output[base_cell_coords] = TileSet::CELL_NEIGHBOR_RIGHT_SIDE;
			output[tile_set->get_neighbor_cell(base_cell_coords,
				TileSet::CELL_NEIGHBOR_RIGHT_SIDE)] = TileSet::CELL_NEIGHBOR_LEFT_SIDE;
			break;
		case 2:
			output[base_cell_coords] = TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER;
			output[tile_set->get_neighbor_cell(base_cell_coords,
				TileSet::CELL_NEIGHBOR_RIGHT_SIDE)] = TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER;
			output[tile_set->get_neighbor_cell(
				base_cell_coords, TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER)] =
				TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER;
			output[tile_set->get_neighbor_cell(base_cell_coords,
				TileSet::CELL_NEIGHBOR_BOTTOM_SIDE)] = TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER;
			break;
		case 3:
			output[base_cell_coords] = TileSet::CELL_NEIGHBOR_BOTTOM_SIDE;
			output[tile_set->get_neighbor_cell(base_cell_coords,
				TileSet::CELL_NEIGHBOR_BOTTOM_SIDE)] = TileSet::CELL_NEIGHBOR_TOP_SIDE;
			break;
		default:
			ERR_FAIL_V(output);
		}
	}
	else if (shape == TileSet::TILE_SHAPE_ISOMETRIC) {
		switch (bit) {
		case 1:
			output[base_cell_coords] = TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE;
			output[tile_set->get_neighbor_cell(base_cell_coords,
				TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE)] = TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE;
			break;
		case 2:
			output[base_cell_coords] = TileSet::CELL_NEIGHBOR_BOTTOM_CORNER;
			output[tile_set->get_neighbor_cell(base_cell_coords,
				TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE)] = TileSet::CELL_NEIGHBOR_LEFT_CORNER;
			output[tile_set->get_neighbor_cell(base_cell_coords,
				TileSet::CELL_NEIGHBOR_BOTTOM_CORNER)] = TileSet::CELL_NEIGHBOR_TOP_CORNER;
			output[tile_set->get_neighbor_cell(base_cell_coords,
				TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE)] = TileSet::CELL_NEIGHBOR_RIGHT_CORNER;
			break;
		case 3:
			output[base_cell_coords] = TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE;
			output[tile_set->get_neighbor_cell(base_cell_coords,
				TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE)] = TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE;
			break;
		default:
			ERR_FAIL_V(output);
		}
	}
	else {
		// Half offset shapes.
		TileSet::TileOffsetAxis offset_axis = tile_set->get_tile_offset_axis();
		if (offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
			switch (bit) {
			case 1:
				output[base_cell_coords] = TileSet::CELL_NEIGHBOR_RIGHT_SIDE;
				output[tile_set->get_neighbor_cell(base_cell_coords,
					TileSet::CELL_NEIGHBOR_RIGHT_SIDE)] = TileSet::CELL_NEIGHBOR_LEFT_SIDE;
				break;
			case 2:
				output[base_cell_coords] = TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER;
				output[tile_set->get_neighbor_cell(base_cell_coords,
					TileSet::CELL_NEIGHBOR_RIGHT_SIDE)] = TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER;
				output[tile_set->get_neighbor_cell(base_cell_coords,
					TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE)] = TileSet::CELL_NEIGHBOR_TOP_CORNER;
				break;
			case 3:
				output[base_cell_coords] = TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE;
				output[tile_set->get_neighbor_cell(
					base_cell_coords, TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE)] =
					TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE;
				break;
			case 4:
				output[base_cell_coords] = TileSet::CELL_NEIGHBOR_BOTTOM_CORNER;
				output[tile_set->get_neighbor_cell(
					base_cell_coords, TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE)] =
					TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER;
				output[tile_set->get_neighbor_cell(
					base_cell_coords, TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE)] =
					TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER;
				break;
			case 5:
				output[base_cell_coords] = TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE;
				output[tile_set->get_neighbor_cell(
					base_cell_coords, TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE)] =
					TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE;
				break;
			default:
				ERR_FAIL_V(output);
			}
		}
		else {
			switch (bit) {
			case 1:
				output[base_cell_coords] = TileSet::CELL_NEIGHBOR_RIGHT_CORNER;
				output[tile_set->get_neighbor_cell(
					base_cell_coords, TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE)] =
					TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER;
				output[tile_set->get_neighbor_cell(
					base_cell_coords, TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE)] =
					TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER;
				break;
			case 2:
				output[base_cell_coords] = TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE;
				output[tile_set->get_neighbor_cell(
					base_cell_coords, TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE)] =
					TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE;
				break;
			case 3:
				output[base_cell_coords] = TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER;
				output[tile_set->get_neighbor_cell(base_cell_coords,
					TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE)] = TileSet::CELL_NEIGHBOR_LEFT_CORNER;
				output[tile_set->get_neighbor_cell(base_cell_coords,
					TileSet::CELL_NEIGHBOR_BOTTOM_SIDE)] = TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER;
				break;
			case 4:
				output[base_cell_coords] = TileSet::CELL_NEIGHBOR_BOTTOM_SIDE;
				output[tile_set->get_neighbor_cell(base_cell_coords,
					TileSet::CELL_NEIGHBOR_BOTTOM_SIDE)] = TileSet::CELL_NEIGHBOR_TOP_SIDE;
				break;
			case 5:
				output[base_cell_coords] = TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE;
				output[tile_set->get_neighbor_cell(
					base_cell_coords, TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE)] =
					TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE;
				break;
			default:
				ERR_FAIL_V(output);
			}
		}
	}
	return output;
}

TerrainConstraint::TerrainConstraint(
	Ref<TileSet> p_tile_set, const Vector2i& p_position, int p_terrain)
{
	ERR_FAIL_COND(p_tile_set.is_null());
	tile_set = p_tile_set;
	bit = 0;
	base_cell_coords = p_position;
	terrain = p_terrain;
}

TerrainConstraint::TerrainConstraint(Ref<TileSet> p_tile_set, const Vector2i& p_position,
	const TileSet::CellNeighbor& p_bit, int p_terrain)
{
	// The way we build the constraint make it easy to detect conflicting constraints.
	ERR_FAIL_COND(p_tile_set.is_null());
	tile_set = p_tile_set;

	TileSet::TileShape shape = tile_set->get_tile_shape();
	if (shape == TileSet::TILE_SHAPE_SQUARE) {
		switch (p_bit) {
		case TileSet::CELL_NEIGHBOR_RIGHT_SIDE:
			bit = 1;
			base_cell_coords = p_position;
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER:
			bit = 2;
			base_cell_coords = p_position;
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_SIDE:
			bit = 3;
			base_cell_coords = p_position;
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER:
			bit = 2;
			base_cell_coords =
				tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_LEFT_SIDE);
			break;
		case TileSet::CELL_NEIGHBOR_LEFT_SIDE:
			bit = 1;
			base_cell_coords =
				tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_LEFT_SIDE);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER:
			bit = 2;
			base_cell_coords =
				tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_SIDE:
			bit = 3;
			base_cell_coords =
				tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_SIDE);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER:
			bit = 2;
			base_cell_coords =
				tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_SIDE);
			break;
		default:
			ERR_FAIL();
			break;
		}
	}
	else if (shape == TileSet::TILE_SHAPE_ISOMETRIC) {
		switch (p_bit) {
		case TileSet::CELL_NEIGHBOR_RIGHT_CORNER:
			bit = 2;
			base_cell_coords =
				tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE);
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE:
			bit = 1;
			base_cell_coords = p_position;
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_CORNER:
			bit = 2;
			base_cell_coords = p_position;
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE:
			bit = 3;
			base_cell_coords = p_position;
			break;
		case TileSet::CELL_NEIGHBOR_LEFT_CORNER:
			bit = 2;
			base_cell_coords =
				tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE:
			bit = 1;
			base_cell_coords =
				tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_CORNER:
			bit = 2;
			base_cell_coords =
				tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_CORNER);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE:
			bit = 3;
			base_cell_coords =
				tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE);
			break;
		default:
			ERR_FAIL();
			break;
		}
	}
	else {
		// Half-offset shapes.
		TileSet::TileOffsetAxis offset_axis = tile_set->get_tile_offset_axis();
		if (offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
			switch (p_bit) {
			case TileSet::CELL_NEIGHBOR_RIGHT_SIDE:
				bit = 1;
				base_cell_coords = p_position;
				break;
			case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER:
				bit = 2;
				base_cell_coords = p_position;
				break;
			case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE:
				bit = 3;
				base_cell_coords = p_position;
				break;
			case TileSet::CELL_NEIGHBOR_BOTTOM_CORNER:
				bit = 4;
				base_cell_coords = p_position;
				break;
			case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE:
				bit = 5;
				base_cell_coords = p_position;
				break;
			case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER:
				bit = 2;
				base_cell_coords =
					tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_LEFT_SIDE);
				break;
			case TileSet::CELL_NEIGHBOR_LEFT_SIDE:
				bit = 1;
				base_cell_coords =
					tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_LEFT_SIDE);
				break;
			case TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER:
				bit = 4;
				base_cell_coords =
					tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE);
				break;
			case TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE:
				bit = 3;
				base_cell_coords =
					tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE);
				break;
			case TileSet::CELL_NEIGHBOR_TOP_CORNER:
				bit = 2;
				base_cell_coords =
					tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE);
				break;
			case TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE:
				bit = 5;
				base_cell_coords =
					tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE);
				break;
			case TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER:
				bit = 4;
				base_cell_coords =
					tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE);
				break;
			default:
				ERR_FAIL();
				break;
			}
		}
		else {
			switch (p_bit) {
			case TileSet::CELL_NEIGHBOR_RIGHT_CORNER:
				bit = 1;
				base_cell_coords = p_position;
				break;
			case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE:
				bit = 2;
				base_cell_coords = p_position;
				break;
			case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER:
				bit = 3;
				base_cell_coords = p_position;
				break;
			case TileSet::CELL_NEIGHBOR_BOTTOM_SIDE:
				bit = 4;
				base_cell_coords = p_position;
				break;
			case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER:
				bit = 1;
				base_cell_coords = tile_set->get_neighbor_cell(
					p_position, TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE);
				break;
			case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE:
				bit = 5;
				base_cell_coords = p_position;
				break;
			case TileSet::CELL_NEIGHBOR_LEFT_CORNER:
				bit = 3;
				base_cell_coords =
					tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE);
				break;
			case TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE:
				bit = 2;
				base_cell_coords =
					tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE);
				break;
			case TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER:
				bit = 1;
				base_cell_coords =
					tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE);
				break;
			case TileSet::CELL_NEIGHBOR_TOP_SIDE:
				bit = 4;
				base_cell_coords =
					tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_SIDE);
				break;
			case TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER:
				bit = 3;
				base_cell_coords =
					tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_SIDE);
				break;
			case TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE:
				bit = 5;
				base_cell_coords =
					tile_set->get_neighbor_cell(p_position, TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE);
				break;
			default:
				ERR_FAIL();
				break;
			}
		}
	}
	terrain = p_terrain;
}


