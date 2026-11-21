/**************************************************************************/
/*  tile_set.cpp                                                          */
/**************************************************************************/
/*                         This file is part of:                          */
/*                            VOLTAIRE ENGINE                             */
/*                https://github.com/BendyLand/Voltaire                   */
/**************************************************************************/
/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
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
#include "core/templates/local_vector.h"
#include "core/templates/rb_set.h"
#include "scene/gui/control.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/mesh.h"
#include "servers/rendering/renderer.h"
#include "tile_set.h"

Vector<int> TileMapPattern::_get_tile_data() const
{
	// Export tile data to raw format
	Vector<int> data;
	data.resize(pattern.size() * 3);
	int* w = data.ptrw();

	// Save in highest format

	int idx = 0;
	for (const KeyValue<Vector2i, TileMapCell>& E : pattern) {
		uint8_t* ptr = (uint8_t*)&w[idx];
		encode_uint16((int16_t)(E.key.x), &ptr[0]);
		encode_uint16((int16_t)(E.key.y), &ptr[2]);
		encode_uint16(E.value.source_id, &ptr[4]);
		encode_uint16(E.value.coord_x, &ptr[6]);
		encode_uint16(E.value.coord_y, &ptr[8]);
		encode_uint16(E.value.alternative_tile, &ptr[10]);
		idx += 3;
	}

	return data;
}

void TileMapPattern::set_cell(const Vector2i& p_coords, int p_source_id,
	const Vector2i p_atlas_coords, int p_alternative_tile)
{
	ERR_FAIL_COND_MSG(p_coords.x < 0 || p_coords.y < 0,
		vformat("Cannot set cell with negative coords in a TileMapPattern. Wrong coords: %s",
			p_coords));

	size = size.max(p_coords + Vector2i(1, 1));
	pattern[p_coords] = TileMapCell(p_source_id, p_atlas_coords, p_alternative_tile);
	emit_changed();
}

bool TileMapPattern::has_cell(const Vector2i& p_coords) const { return pattern.has(p_coords); }

void TileMapPattern::remove_cell(const Vector2i& p_coords, bool p_update_size)
{
	ERR_FAIL_COND(!pattern.has(p_coords));

	pattern.erase(p_coords);
	if (p_update_size) {
		size = Size2i();
		for (const KeyValue<Vector2i, TileMapCell>& E : pattern) {
			size = size.max(E.key + Vector2i(1, 1));
		}
	}
	emit_changed();
}

int TileMapPattern::get_cell_source_id(const Vector2i& p_coords) const
{
	ERR_FAIL_COND_V(!pattern.has(p_coords), TileSet::INVALID_SOURCE);

	return pattern[p_coords].source_id;
}

Vector2i TileMapPattern::get_cell_atlas_coords(const Vector2i& p_coords) const
{
	ERR_FAIL_COND_V(!pattern.has(p_coords), TileSetSource::INVALID_ATLAS_COORDS);

	return pattern[p_coords].get_atlas_coords();
}

int TileMapPattern::get_cell_alternative_tile(const Vector2i& p_coords) const
{
	ERR_FAIL_COND_V(!pattern.has(p_coords), TileSetSource::INVALID_TILE_ALTERNATIVE);

	return pattern[p_coords].alternative_tile;
}

Size2i TileMapPattern::get_size() const { return size; }

void TileMapPattern::set_size(const Size2i& p_size)
{
	for (const KeyValue<Vector2i, TileMapCell>& E : pattern) {
		Vector2i coords = E.key;
		if (p_size.x <= coords.x || p_size.y <= coords.y) {
			ERR_FAIL_MSG(vformat("Cannot set pattern size to %s, it contains a tile at %s. Size "
								 "can only be increased.",
				p_size, coords));
		};
	}

	size = p_size;
	emit_changed();
}

bool TileMapPattern::is_empty() const { return pattern.is_empty(); }

void TileMapPattern::clear()
{
	size = Size2i();
	pattern.clear();
	emit_changed();
}

bool TileSet::TerrainsPattern::is_valid() const { return valid; }

bool TileSet::TerrainsPattern::is_erase_pattern() const { return not_empty_terrains_count == 0; }

bool TileSet::TerrainsPattern::operator<(const TerrainsPattern& p_terrains_pattern) const
{
	for (int i = 0; i < TileSet::CELL_NEIGHBOR_MAX; i++) {
		if (is_valid_bit[i] != p_terrains_pattern.is_valid_bit[i]) {
			return is_valid_bit[i] < p_terrains_pattern.is_valid_bit[i];
		}
	}
	if (terrain != p_terrains_pattern.terrain) {
		return terrain < p_terrains_pattern.terrain;
	}
	for (int i = 0; i < TileSet::CELL_NEIGHBOR_MAX; i++) {
		if (is_valid_bit[i] && bits[i] != p_terrains_pattern.bits[i]) {
			return bits[i] < p_terrains_pattern.bits[i];
		}
	}
	return false;
}

bool TileSet::TerrainsPattern::operator==(const TerrainsPattern& p_terrains_pattern) const
{
	for (int i = 0; i < TileSet::CELL_NEIGHBOR_MAX; i++) {
		if (is_valid_bit[i] != p_terrains_pattern.is_valid_bit[i]) {
			return false;
		}
		if (is_valid_bit[i] && bits[i] != p_terrains_pattern.bits[i]) {
			return false;
		}
	}
	if (terrain != p_terrains_pattern.terrain) {
		return false;
	}
	return true;
}

void TileSet::TerrainsPattern::set_terrain(int p_terrain)
{
	ERR_FAIL_COND(p_terrain < -1);

	terrain = p_terrain;
}

int TileSet::TerrainsPattern::get_terrain() const { return terrain; }

void TileSet::TerrainsPattern::set_terrain_peering_bit(
	TileSet::CellNeighbor p_peering_bit, int p_terrain)
{
	ERR_FAIL_COND(p_peering_bit == TileSet::CELL_NEIGHBOR_MAX);
	ERR_FAIL_COND(!is_valid_bit[p_peering_bit]);
	ERR_FAIL_COND(p_terrain < -1);

	// Update the "is_erase_pattern" status.
	if (p_terrain >= 0 && bits[p_peering_bit] < 0) {
		not_empty_terrains_count++;
	}
	else if (p_terrain < 0 && bits[p_peering_bit] >= 0) {
		not_empty_terrains_count--;
	}

	bits[p_peering_bit] = p_terrain;
}

int TileSet::TerrainsPattern::get_terrain_peering_bit(TileSet::CellNeighbor p_peering_bit) const
{
	ERR_FAIL_COND_V(p_peering_bit == TileSet::CELL_NEIGHBOR_MAX, -1);
	ERR_FAIL_COND_V(!is_valid_bit[p_peering_bit], -1);
	return bits[p_peering_bit];
}

TileSet::TerrainsPattern::TerrainsPattern(const TileSet* p_tile_set, int p_terrain_set)
{
	ERR_FAIL_COND(p_terrain_set < 0);
	for (int i = 0; i < TileSet::CELL_NEIGHBOR_MAX; i++) {
		is_valid_bit[i] =
			(p_tile_set->is_valid_terrain_peering_bit(p_terrain_set, TileSet::CellNeighbor(i)));
		bits[i] = -1;
	}
	valid = true;
}

const int TileSet::INVALID_SOURCE = -1;

const char* TileSet::CELL_NEIGHBOR_ENUM_TO_TEXT[] = {
	PNAME("right_side"),
	PNAME("right_corner"),
	PNAME("bottom_right_side"),
	PNAME("bottom_right_corner"),
	PNAME("bottom_side"),
	PNAME("bottom_corner"),
	PNAME("bottom_left_side"),
	PNAME("bottom_left_corner"),
	PNAME("left_side"),
	PNAME("left_corner"),
	PNAME("top_left_side"),
	PNAME("top_left_corner"),
	PNAME("top_side"),
	PNAME("top_corner"),
	PNAME("top_right_side"),
	PNAME("top_right_corner"),
};

TileSet::TileShape TileSet::get_tile_shape() const { return tile_shape; }

void TileSet::set_tile_layout(TileSet::TileLayout p_layout)
{
	tile_layout = p_layout;
	emit_changed();
}

TileSet::TileLayout TileSet::get_tile_layout() const { return tile_layout; }

void TileSet::set_tile_offset_axis(TileSet::TileOffsetAxis p_alignment)
{
	tile_offset_axis = p_alignment;

	for (KeyValue<int, Ref<TileSetSource>>& E_source : sources) {
		E_source.value->notify_tile_data_properties_should_change();
	}

	terrain_bits_meshes_dirty = true;
	tile_meshes_dirty = true;
	emit_changed();
}

TileSet::TileOffsetAxis TileSet::get_tile_offset_axis() const { return tile_offset_axis; }

void TileSet::set_tile_size(Size2i p_size)
{
	ERR_FAIL_COND(p_size.x < 1 || p_size.y < 1);
	tile_size = p_size;
	terrain_bits_meshes_dirty = true;
	tile_meshes_dirty = true;
	emit_changed();
}

Size2i TileSet::get_tile_size() const { return tile_size; }

int TileSet::get_next_source_id() const { return next_source_id; }

void TileSet::_update_terrains_cache()
{
	if (terrains_cache_dirty) {
		// Organizes tiles into structures.
		per_terrain_pattern_tiles.resize(terrain_sets.size());
		for (RBMap<TileSet::TerrainsPattern, RBSet<TileMapCell>>& tiles :
			per_terrain_pattern_tiles) {
			tiles.clear();
		}

		for (const KeyValue<int, Ref<TileSetSource>>& kv : sources) {
			Ref<TileSetSource> source = kv.value;
			Ref<TileSetAtlasSource> atlas_source = source;
			if (atlas_source.is_valid()) {
				for (int tile_index = 0; tile_index < source->get_tiles_count(); tile_index++) {
					Vector2i tile_id = source->get_tile_id(tile_index);
					for (int alternative_index = 0;
						 alternative_index < source->get_alternative_tiles_count(tile_id);
						 alternative_index++) {
						int alternative_id =
							source->get_alternative_tile_id(tile_id, alternative_index);

						// Executed for each tile_data.
						TileData* tile_data = atlas_source->get_tile_data(tile_id, alternative_id);
						int terrain_set = tile_data->get_terrain_set();
						if (terrain_set >= 0) {
							TileMapCell cell;
							cell.source_id = kv.key;
							cell.set_atlas_coords(tile_id);
							cell.alternative_tile = alternative_id;

							TileSet::TerrainsPattern terrains_pattern =
								tile_data->get_terrains_pattern();

							// Main terrain.
							if (terrains_pattern.get_terrain() >= 0) {
								per_terrain_pattern_tiles[terrain_set][terrains_pattern].insert(
									cell);
							}

							// Terrain bits.
							for (int i = 0; i < TileSet::CELL_NEIGHBOR_MAX; i++) {
								CellNeighbor bit = CellNeighbor(i);
								if (is_valid_terrain_peering_bit(terrain_set, bit)) {
									int terrain = terrains_pattern.get_terrain_peering_bit(bit);
									if (terrain >= 0) {
										per_terrain_pattern_tiles[terrain_set][terrains_pattern]
											.insert(cell);
									}
								}
							}
						}
					}
				}
			}
		}

		// Add the empty cell in the possible patterns and cells.
		for (int i = 0; i < terrain_sets.size(); i++) {
			TileSet::TerrainsPattern empty_pattern(this, i);

			TileMapCell empty_cell;
			empty_cell.source_id = TileSet::INVALID_SOURCE;
			empty_cell.set_atlas_coords(TileSetSource::INVALID_ATLAS_COORDS);
			empty_cell.alternative_tile = TileSetSource::INVALID_TILE_ALTERNATIVE;
			per_terrain_pattern_tiles[i][empty_pattern].insert(empty_cell);
		}
		terrains_cache_dirty = false;
	}
}

void TileSet::_compute_next_source_id()
{
	while (sources.has(next_source_id)) {
		next_source_id = (next_source_id + 1) % 1073741824; // 2 ** 30
	};
}

void TileSet::remove_source_ptr(TileSetSource* p_tile_set_source)
{
	for (const KeyValue<int, Ref<TileSetSource>>& kv : sources) {
		if (kv.value.ptr() == p_tile_set_source) {
			remove_source(kv.key);
			return;
		}
	}
	ERR_FAIL_MSG(
		vformat("Attempting to remove source from a tileset, but the tileset doesn't have it: %s",
			p_tile_set_source));
}

void TileSet::set_source_id(int p_source_id, int p_new_source_id)
{
	ERR_FAIL_COND(p_new_source_id < 0);
	ERR_FAIL_COND_MSG(!sources.has(p_source_id),
		vformat("Cannot change TileSet atlas source ID. No tileset atlas source with id %d.",
			p_source_id));
	if (p_source_id == p_new_source_id) {
		return;
	}

	ERR_FAIL_COND_MSG(sources.has(p_new_source_id),
		vformat("Cannot change TileSet atlas source ID. Another atlas source exists with id %d.",
			p_new_source_id));

	sources[p_new_source_id] = sources[p_source_id];
	sources.erase(p_source_id);

	source_ids.erase(p_source_id);
	source_ids.push_back(p_new_source_id);
	source_ids.sort();

	_compute_next_source_id();

	terrains_cache_dirty = true;
	emit_changed();
}

bool TileSet::has_source(int p_source_id) const { return sources.has(p_source_id); }

Ref<TileSetSource> TileSet::get_source(int p_source_id) const
{
	ERR_FAIL_COND_V_MSG(!sources.has(p_source_id), nullptr,
		vformat("No TileSet atlas source with id %d.", p_source_id));

	return sources[p_source_id];
}

int TileSet::get_source_count() const { return source_ids.size(); }

int TileSet::get_source_id(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, source_ids.size(), TileSet::INVALID_SOURCE);
	return source_ids[p_index];
}

void TileSet::set_uv_clipping(bool p_uv_clipping)
{
	if (uv_clipping == p_uv_clipping) {
		return;
	}
	uv_clipping = p_uv_clipping;
	emit_changed();
}

bool TileSet::is_uv_clipping() const { return uv_clipping; }

int TileSet::get_occlusion_layers_count() const { return occlusion_layers.size(); }

void TileSet::set_occlusion_layer_light_mask(int p_layer_index, int p_light_mask)
{
	ERR_FAIL_INDEX(p_layer_index, occlusion_layers.size());
	occlusion_layers.write[p_layer_index].light_mask = p_light_mask;
	emit_changed();
}

int TileSet::get_occlusion_layer_light_mask(int p_layer_index) const
{
	ERR_FAIL_INDEX_V(p_layer_index, occlusion_layers.size(), 0);
	return occlusion_layers[p_layer_index].light_mask;
}

void TileSet::set_occlusion_layer_sdf_collision(int p_layer_index, bool p_sdf_collision)
{
	ERR_FAIL_INDEX(p_layer_index, occlusion_layers.size());
	occlusion_layers.write[p_layer_index].sdf_collision = p_sdf_collision;
	emit_changed();
}

bool TileSet::get_occlusion_layer_sdf_collision(int p_layer_index) const
{
	ERR_FAIL_INDEX_V(p_layer_index, occlusion_layers.size(), false);
	return occlusion_layers[p_layer_index].sdf_collision;
}

#ifndef PHYSICS_2D_DISABLED
int TileSet::get_physics_layers_count() const { return physics_layers.size(); }

void TileSet::set_physics_layer_collision_layer(int p_layer_index, uint32_t p_layer)
{
	ERR_FAIL_INDEX(p_layer_index, physics_layers.size());
	physics_layers.write[p_layer_index].collision_layer = p_layer;
	emit_changed();
}

uint32_t TileSet::get_physics_layer_collision_layer(int p_layer_index) const
{
	ERR_FAIL_INDEX_V(p_layer_index, physics_layers.size(), 0);
	return physics_layers[p_layer_index].collision_layer;
}

void TileSet::set_physics_layer_collision_mask(int p_layer_index, uint32_t p_mask)
{
	ERR_FAIL_INDEX(p_layer_index, physics_layers.size());
	physics_layers.write[p_layer_index].collision_mask = p_mask;
	emit_changed();
}

uint32_t TileSet::get_physics_layer_collision_mask(int p_layer_index) const
{
	ERR_FAIL_INDEX_V(p_layer_index, physics_layers.size(), 0);
	return physics_layers[p_layer_index].collision_mask;
}

void TileSet::set_physics_layer_collision_priority(int p_layer_index, real_t p_priority)
{
	ERR_FAIL_INDEX(p_layer_index, physics_layers.size());
	physics_layers.write[p_layer_index].collision_priority = p_priority;
	emit_changed();
}

real_t TileSet::get_physics_layer_collision_priority(int p_layer_index) const
{
	ERR_FAIL_INDEX_V(p_layer_index, physics_layers.size(), 0);
	return physics_layers[p_layer_index].collision_priority;
}

void TileSet::set_physics_layer_physics_material(
	int p_layer_index, Ref<PhysicsMaterial> p_physics_material)
{
	ERR_FAIL_INDEX(p_layer_index, physics_layers.size());
	physics_layers.write[p_layer_index].physics_material = p_physics_material;
}

Ref<PhysicsMaterial> TileSet::get_physics_layer_physics_material(int p_layer_index) const
{
	ERR_FAIL_INDEX_V(p_layer_index, physics_layers.size(), Ref<PhysicsMaterial>());
	return physics_layers[p_layer_index].physics_material;
}
#endif // PHYSICS_2D_DISABLED

int TileSet::get_terrain_sets_count() const { return terrain_sets.size(); }

TileSet::TerrainMode TileSet::get_terrain_set_mode(int p_terrain_set) const
{
	ERR_FAIL_INDEX_V(
		p_terrain_set, terrain_sets.size(), TileSet::TERRAIN_MODE_MATCH_CORNERS_AND_SIDES);
	return terrain_sets[p_terrain_set].mode;
}

int TileSet::get_terrains_count(int p_terrain_set) const
{
	ERR_FAIL_INDEX_V(p_terrain_set, terrain_sets.size(), -1);
	return terrain_sets[p_terrain_set].terrains.size();
}

void TileSet::set_terrain_name(int p_terrain_set, int p_terrain_index, String p_name)
{
	ERR_FAIL_INDEX(p_terrain_set, terrain_sets.size());
	ERR_FAIL_INDEX(p_terrain_index, terrain_sets[p_terrain_set].terrains.size());
	terrain_sets.write[p_terrain_set].terrains.write[p_terrain_index].name = p_name;
	emit_changed();
}

String TileSet::get_terrain_name(int p_terrain_set, int p_terrain_index) const
{
	ERR_FAIL_INDEX_V(p_terrain_set, terrain_sets.size(), String());
	ERR_FAIL_INDEX_V(p_terrain_index, terrain_sets[p_terrain_set].terrains.size(), String());
	return terrain_sets[p_terrain_set].terrains[p_terrain_index].name;
}

void TileSet::set_terrain_color(int p_terrain_set, int p_terrain_index, Color p_color)
{
	ERR_FAIL_INDEX(p_terrain_set, terrain_sets.size());
	ERR_FAIL_INDEX(p_terrain_index, terrain_sets[p_terrain_set].terrains.size());
	if (p_color.a != 1.0) {
		WARN_PRINT("Terrain color should have alpha == 1.0");
		p_color.a = 1.0;
	}
	terrain_sets.write[p_terrain_set].terrains.write[p_terrain_index].color = p_color;
	emit_changed();
}

Color TileSet::get_terrain_color(int p_terrain_set, int p_terrain_index) const
{
	ERR_FAIL_INDEX_V(p_terrain_set, terrain_sets.size(), Color());
	ERR_FAIL_INDEX_V(p_terrain_index, terrain_sets[p_terrain_set].terrains.size(), Color());
	return terrain_sets[p_terrain_set].terrains[p_terrain_index].color;
}

bool TileSet::is_valid_terrain_peering_bit_for_mode(
	TileSet::TerrainMode p_terrain_mode, TileSet::CellNeighbor p_peering_bit) const
{
	if (tile_shape == TileSet::TILE_SHAPE_SQUARE) {
		if (p_terrain_mode == TileSet::TERRAIN_MODE_MATCH_CORNERS_AND_SIDES ||
			p_terrain_mode == TileSet::TERRAIN_MODE_MATCH_SIDES) {
			if (p_peering_bit == TileSet::CELL_NEIGHBOR_RIGHT_SIDE ||
				p_peering_bit == TileSet::CELL_NEIGHBOR_BOTTOM_SIDE ||
				p_peering_bit == TileSet::CELL_NEIGHBOR_LEFT_SIDE ||
				p_peering_bit == TileSet::CELL_NEIGHBOR_TOP_SIDE) {
				return true;
			}
		}
		if (p_terrain_mode == TileSet::TERRAIN_MODE_MATCH_CORNERS_AND_SIDES ||
			p_terrain_mode == TileSet::TERRAIN_MODE_MATCH_CORNERS) {
			if (p_peering_bit == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER ||
				p_peering_bit == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER ||
				p_peering_bit == TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER ||
				p_peering_bit == TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER) {
				return true;
			}
		}
	}
	else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC) {
		if (p_terrain_mode == TileSet::TERRAIN_MODE_MATCH_CORNERS_AND_SIDES ||
			p_terrain_mode == TileSet::TERRAIN_MODE_MATCH_SIDES) {
			if (p_peering_bit == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE ||
				p_peering_bit == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE ||
				p_peering_bit == TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE ||
				p_peering_bit == TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE) {
				return true;
			}
		}
		if (p_terrain_mode == TileSet::TERRAIN_MODE_MATCH_CORNERS_AND_SIDES ||
			p_terrain_mode == TileSet::TERRAIN_MODE_MATCH_CORNERS) {
			if (p_peering_bit == TileSet::CELL_NEIGHBOR_RIGHT_CORNER ||
				p_peering_bit == TileSet::CELL_NEIGHBOR_BOTTOM_CORNER ||
				p_peering_bit == TileSet::CELL_NEIGHBOR_LEFT_CORNER ||
				p_peering_bit == TileSet::CELL_NEIGHBOR_TOP_CORNER) {
				return true;
			}
		}
	}
	else {
		if (get_tile_offset_axis() == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
			if (p_terrain_mode == TileSet::TERRAIN_MODE_MATCH_CORNERS_AND_SIDES ||
				p_terrain_mode == TileSet::TERRAIN_MODE_MATCH_SIDES) {
				if (p_peering_bit == TileSet::CELL_NEIGHBOR_RIGHT_SIDE ||
					p_peering_bit == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE ||
					p_peering_bit == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE ||
					p_peering_bit == TileSet::CELL_NEIGHBOR_LEFT_SIDE ||
					p_peering_bit == TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE ||
					p_peering_bit == TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE) {
					return true;
				}
			}
			if (p_terrain_mode == TileSet::TERRAIN_MODE_MATCH_CORNERS_AND_SIDES ||
				p_terrain_mode == TileSet::TERRAIN_MODE_MATCH_CORNERS) {
				if (p_peering_bit == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER ||
					p_peering_bit == TileSet::CELL_NEIGHBOR_BOTTOM_CORNER ||
					p_peering_bit == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER ||
					p_peering_bit == TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER ||
					p_peering_bit == TileSet::CELL_NEIGHBOR_TOP_CORNER ||
					p_peering_bit == TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER) {
					return true;
				}
			}
		}
		else {
			if (p_terrain_mode == TileSet::TERRAIN_MODE_MATCH_CORNERS_AND_SIDES ||
				p_terrain_mode == TileSet::TERRAIN_MODE_MATCH_SIDES) {
				if (p_peering_bit == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE ||
					p_peering_bit == TileSet::CELL_NEIGHBOR_BOTTOM_SIDE ||
					p_peering_bit == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE ||
					p_peering_bit == TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE ||
					p_peering_bit == TileSet::CELL_NEIGHBOR_TOP_SIDE ||
					p_peering_bit == TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE) {
					return true;
				}
			}
			if (p_terrain_mode == TileSet::TERRAIN_MODE_MATCH_CORNERS_AND_SIDES ||
				p_terrain_mode == TileSet::TERRAIN_MODE_MATCH_CORNERS) {
				if (p_peering_bit == TileSet::CELL_NEIGHBOR_RIGHT_CORNER ||
					p_peering_bit == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER ||
					p_peering_bit == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER ||
					p_peering_bit == TileSet::CELL_NEIGHBOR_LEFT_CORNER ||
					p_peering_bit == TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER ||
					p_peering_bit == TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER) {
					return true;
				}
			}
		}
	}
	return false;
}

bool TileSet::is_valid_terrain_peering_bit(
	int p_terrain_set, TileSet::CellNeighbor p_peering_bit) const
{
	if (p_terrain_set < 0 || p_terrain_set >= get_terrain_sets_count()) {
		return false;
	}

	TileSet::TerrainMode terrain_mode = get_terrain_set_mode(p_terrain_set);
	return is_valid_terrain_peering_bit_for_mode(terrain_mode, p_peering_bit);
}

#ifndef NAVIGATION_2D_DISABLED
int TileSet::get_navigation_layers_count() const { return navigation_layers.size(); }

void TileSet::set_navigation_layer_layers(int p_layer_index, uint32_t p_layers)
{
	ERR_FAIL_INDEX(p_layer_index, navigation_layers.size());
	navigation_layers.write[p_layer_index].layers = p_layers;
	emit_changed();
}

uint32_t TileSet::get_navigation_layer_layers(int p_layer_index) const
{
	ERR_FAIL_INDEX_V(p_layer_index, navigation_layers.size(), 0);
	return navigation_layers[p_layer_index].layers;
}

void TileSet::set_navigation_layer_layer_value(int p_layer_index, int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Navigation layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 32, "Navigation layer number must be between 1 and 32 inclusive.");

	uint32_t _navigation_layers = get_navigation_layer_layers(p_layer_index);

	if (p_value) {
		_navigation_layers |= 1 << (p_layer_number - 1);
	}
	else {
		_navigation_layers &= ~(1 << (p_layer_number - 1));
	}

	set_navigation_layer_layers(p_layer_index, _navigation_layers);
}

bool TileSet::get_navigation_layer_layer_value(int p_layer_index, int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Navigation layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Navigation layer number must be between 1 and 32 inclusive.");

	return get_navigation_layer_layers(p_layer_index) & (1 << (p_layer_number - 1));
}
#endif // NAVIGATION_2D_DISABLED

int TileSet::get_custom_data_layer_by_name(String p_value) const
{
	if (custom_data_layers_by_name.has(p_value)) {
		return custom_data_layers_by_name[p_value];
	}
	else {
		return -1;
	}
}

bool TileSet::has_custom_data_layer_by_name(const String& p_value) const
{
	return custom_data_layers_by_name.has(p_value);
}

void TileSet::set_source_level_tile_proxy(int p_source_from, int p_source_to)
{
	ERR_FAIL_COND(
		p_source_from == TileSet::INVALID_SOURCE || p_source_to == TileSet::INVALID_SOURCE);

	source_level_proxies[p_source_from] = p_source_to;

	emit_changed();
}

int TileSet::get_source_level_tile_proxy(int p_source_from)
{
	ERR_FAIL_COND_V(!source_level_proxies.has(p_source_from), TileSet::INVALID_SOURCE);

	return source_level_proxies[p_source_from];
}

bool TileSet::has_source_level_tile_proxy(int p_source_from)
{
	return source_level_proxies.has(p_source_from);
}

void TileSet::remove_source_level_tile_proxy(int p_source_from)
{
	ERR_FAIL_COND(!source_level_proxies.has(p_source_from));

	source_level_proxies.erase(p_source_from);

	emit_changed();
}

int TileSet::add_pattern(Ref<TileMapPattern> p_pattern, int p_index)
{
	ERR_FAIL_COND_V(p_pattern.is_null(), -1);
	ERR_FAIL_COND_V_MSG(p_pattern->is_empty(), -1, "Cannot add an empty pattern to the TileSet.");
	for (const Ref<TileMapPattern>& pattern : patterns) {
		ERR_FAIL_COND_V_MSG(pattern == p_pattern, -1, "TileSet has already this pattern.");
	}
	ERR_FAIL_COND_V(p_index > (int)patterns.size(), -1);
	if (p_index < 0) {
		p_index = patterns.size();
	}
	patterns.insert(p_index, p_pattern);
	emit_changed();
	return p_index;
}

Ref<TileMapPattern> TileSet::get_pattern(int p_index)
{
	ERR_FAIL_INDEX_V(p_index, (int)patterns.size(), Ref<TileMapPattern>());
	return patterns[p_index];
}

void TileSet::remove_pattern(int p_index)
{
	ERR_FAIL_INDEX(p_index, (int)patterns.size());
	patterns.remove_at(p_index);
	emit_changed();
}

int TileSet::get_patterns_count() { return patterns.size(); }

RBSet<TileSet::TerrainsPattern> TileSet::get_terrains_pattern_set(int p_terrain_set)
{
	ERR_FAIL_INDEX_V(p_terrain_set, terrain_sets.size(), RBSet<TileSet::TerrainsPattern>());
	_update_terrains_cache();

	RBSet<TileSet::TerrainsPattern> output;
	for (KeyValue<TileSet::TerrainsPattern, RBSet<TileMapCell>> kv :
		per_terrain_pattern_tiles[p_terrain_set]) {
		output.insert(kv.key);
	}
	return output;
}

RBSet<TileMapCell> TileSet::get_tiles_for_terrains_pattern(
	int p_terrain_set, TerrainsPattern p_terrain_tile_pattern)
{
	ERR_FAIL_INDEX_V(p_terrain_set, terrain_sets.size(), RBSet<TileMapCell>());
	_update_terrains_cache();
	return RBSet<TileMapCell>(per_terrain_pattern_tiles[p_terrain_set][p_terrain_tile_pattern]);
}

TileMapCell TileSet::get_random_tile_from_terrains_pattern(
	int p_terrain_set, TileSet::TerrainsPattern p_terrain_tile_pattern)
{
	ERR_FAIL_INDEX_V(p_terrain_set, terrain_sets.size(), TileMapCell());
	_update_terrains_cache();

	// Count the sum of probabilities.
	double sum = 0.0;
	RBSet<TileMapCell> set(per_terrain_pattern_tiles[p_terrain_set][p_terrain_tile_pattern]);
	for (const TileMapCell& E : set) {
		if (E.source_id >= 0) {
			Ref<TileSetSource> source = sources[E.source_id];
			Ref<TileSetAtlasSource> atlas_source = source;
			if (atlas_source.is_valid()) {
				TileData* tile_data =
					atlas_source->get_tile_data(E.get_atlas_coords(), E.alternative_tile);
				sum += tile_data->get_probability();
			}
			else {
				sum += 1.0;
			}
		}
		else {
			sum += 1.0;
		}
	}

	// Generate a random number.
	double count = 0.0;
	double picked = Math::random(0.0, sum);

	// Pick the tile.
	for (const TileMapCell& E : set) {
		if (E.source_id >= 0) {
			Ref<TileSetSource> source = sources[E.source_id];

			Ref<TileSetAtlasSource> atlas_source = source;
			if (atlas_source.is_valid()) {
				TileData* tile_data =
					atlas_source->get_tile_data(E.get_atlas_coords(), E.alternative_tile);
				count += tile_data->get_probability();
			}
			else {
				count += 1.0;
			}
		}
		else {
			count += 1.0;
		}

		if (count >= picked) {
			return E;
		}
	}

	ERR_FAIL_V(TileMapCell());
}

Vector<Vector2> TileSet::get_tile_shape_polygon() const
{
	Vector<Vector2> points;
	if (tile_shape == TileSet::TILE_SHAPE_SQUARE) {
		points.push_back(Vector2(-0.5, -0.5));
		points.push_back(Vector2(0.5, -0.5));
		points.push_back(Vector2(0.5, 0.5));
		points.push_back(Vector2(-0.5, 0.5));
	}
	else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC) {
		points.push_back(Vector2(0.0, -0.5));
		points.push_back(Vector2(-0.5, 0.0));
		points.push_back(Vector2(0.0, 0.5));
		points.push_back(Vector2(0.5, 0.0));
	}
	else {
		float overlap = 0.0;
		switch (tile_shape) {
		case TileSet::TILE_SHAPE_HEXAGON:
			overlap = 0.25;
			break;
		case TileSet::TILE_SHAPE_HALF_OFFSET_SQUARE:
			overlap = 0.0;
			break;
		default:
			break;
		}

		points.push_back(Vector2(0.0, -0.5));
		points.push_back(Vector2(-0.5, overlap - 0.5));
		points.push_back(Vector2(-0.5, 0.5 - overlap));
		points.push_back(Vector2(0.0, 0.5));
		points.push_back(Vector2(0.5, 0.5 - overlap));
		points.push_back(Vector2(0.5, overlap - 0.5));

		if (get_tile_offset_axis() == TileSet::TILE_OFFSET_AXIS_VERTICAL) {
			for (int i = 0; i < points.size(); i++) {
				points.write[i] = Vector2(points[i].y, points[i].x);
			}
		}
	}
	return points;
}

Vector2 TileSet::map_to_local(const Vector2i& p_pos) const
{
	// SHOULD RETURN THE CENTER OF THE CELL.
	Vector2 ret = p_pos;

	if (tile_shape == TileSet::TILE_SHAPE_HALF_OFFSET_SQUARE ||
		tile_shape == TileSet::TILE_SHAPE_HEXAGON || tile_shape == TileSet::TILE_SHAPE_ISOMETRIC) {
		// Technically, those 3 shapes are equivalent, as they are basically half-offset, but with
		// different levels or overlap. square = no overlap, hexagon = 0.25 overlap, isometric = 0.5
		// overlap.
		if (tile_offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
			switch (tile_layout) {
			case TileSet::TILE_LAYOUT_STACKED:
				ret = Vector2(ret.x + (Math::posmod(ret.y, 2) == 0 ? 0.0 : 0.5), ret.y);
				break;
			case TileSet::TILE_LAYOUT_STACKED_OFFSET:
				ret = Vector2(ret.x + (Math::posmod(ret.y, 2) == 1 ? 0.0 : 0.5), ret.y);
				break;
			case TileSet::TILE_LAYOUT_STAIRS_RIGHT:
				ret = Vector2(ret.x + ret.y / 2, ret.y);
				break;
			case TileSet::TILE_LAYOUT_STAIRS_DOWN:
				ret = Vector2(ret.x / 2, ret.y * 2 + ret.x);
				break;
			case TileSet::TILE_LAYOUT_DIAMOND_RIGHT:
				ret = Vector2((ret.x + ret.y) / 2, ret.y - ret.x);
				break;
			case TileSet::TILE_LAYOUT_DIAMOND_DOWN:
				ret = Vector2((ret.x - ret.y) / 2, ret.y + ret.x);
				break;
			}
		}
		else { // TILE_OFFSET_AXIS_VERTICAL.
			switch (tile_layout) {
			case TileSet::TILE_LAYOUT_STACKED:
				ret = Vector2(ret.x, ret.y + (Math::posmod(ret.x, 2) == 0 ? 0.0 : 0.5));
				break;
			case TileSet::TILE_LAYOUT_STACKED_OFFSET:
				ret = Vector2(ret.x, ret.y + (Math::posmod(ret.x, 2) == 1 ? 0.0 : 0.5));
				break;
			case TileSet::TILE_LAYOUT_STAIRS_RIGHT:
				ret = Vector2(ret.x * 2 + ret.y, ret.y / 2);
				break;
			case TileSet::TILE_LAYOUT_STAIRS_DOWN:
				ret = Vector2(ret.x, ret.y + ret.x / 2);
				break;
			case TileSet::TILE_LAYOUT_DIAMOND_RIGHT:
				ret = Vector2(ret.x + ret.y, (ret.y - ret.x) / 2);
				break;
			case TileSet::TILE_LAYOUT_DIAMOND_DOWN:
				ret = Vector2(ret.x - ret.y, (ret.y + ret.x) / 2);
				break;
			}
		}
	}

	// Multiply by the overlapping ratio.
	double overlapping_ratio = 1.0;
	if (tile_offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
		if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC) {
			overlapping_ratio = 0.5;
		}
		else if (tile_shape == TileSet::TILE_SHAPE_HEXAGON) {
			overlapping_ratio = 0.75;
		}
		ret.y *= overlapping_ratio;
	}
	else { // TILE_OFFSET_AXIS_VERTICAL.
		if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC) {
			overlapping_ratio = 0.5;
		}
		else if (tile_shape == TileSet::TILE_SHAPE_HEXAGON) {
			overlapping_ratio = 0.75;
		}
		ret.x *= overlapping_ratio;
	}

	return (ret + Vector2(0.5, 0.5)) * tile_size;
}

Vector2i TileSet::local_to_map(const Vector2& p_local_position) const
{
	Vector2 ret = p_local_position;
	ret /= tile_size;

	// Divide by the overlapping ratio.
	double overlapping_ratio = 1.0;
	if (tile_offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
		if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC) {
			overlapping_ratio = 0.5;
		}
		else if (tile_shape == TileSet::TILE_SHAPE_HEXAGON) {
			overlapping_ratio = 0.75;
		}
		ret.y /= overlapping_ratio;
	}
	else { // TILE_OFFSET_AXIS_VERTICAL.
		if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC) {
			overlapping_ratio = 0.5;
		}
		else if (tile_shape == TileSet::TILE_SHAPE_HEXAGON) {
			overlapping_ratio = 0.75;
		}
		ret.x /= overlapping_ratio;
	}

	// For each half-offset shape, we check if we are in the corner of the tile, and thus should
	// correct the local position accordingly.
	if (tile_shape == TileSet::TILE_SHAPE_HALF_OFFSET_SQUARE ||
		tile_shape == TileSet::TILE_SHAPE_HEXAGON || tile_shape == TileSet::TILE_SHAPE_ISOMETRIC) {
		// Technically, those 3 shapes are equivalent, as they are basically half-offset, but with
		// different levels or overlap. square = no overlap, hexagon = 0.25 overlap, isometric = 0.5
		// overlap.
		if (tile_offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
			// Smart floor of the position
			Vector2 raw_pos = ret;
			if (Math::posmod(Math::floor(ret.y), 2) ^
				(tile_layout == TileSet::TILE_LAYOUT_STACKED_OFFSET)) {
				ret = Vector2(Math::floor(ret.x + 0.5) - 0.5, Math::floor(ret.y));
			}
			else {
				ret = ret.floor();
			}

			// Compute the tile offset, and if we might the output for a neighbor top tile.
			Vector2 in_tile_pos = raw_pos - ret;
			bool in_top_left_triangle = (in_tile_pos - Vector2(0.5, 0.0))
											.cross(Vector2(-0.5, 1.0 / overlapping_ratio - 1)) <= 0;
			bool in_top_right_triangle =
				(in_tile_pos - Vector2(0.5, 0.0)).cross(Vector2(0.5, 1.0 / overlapping_ratio - 1)) >
				0;

			switch (tile_layout) {
			case TileSet::TILE_LAYOUT_STACKED:
				ret = ret.floor();
				if (in_top_left_triangle) {
					ret += Vector2i(Math::posmod(Math::floor(ret.y), 2) ? 0 : -1, -1);
				}
				else if (in_top_right_triangle) {
					ret += Vector2i(Math::posmod(Math::floor(ret.y), 2) ? 1 : 0, -1);
				}
				break;
			case TileSet::TILE_LAYOUT_STACKED_OFFSET:
				ret = ret.floor();
				if (in_top_left_triangle) {
					ret += Vector2i(Math::posmod(Math::floor(ret.y), 2) ? -1 : 0, -1);
				}
				else if (in_top_right_triangle) {
					ret += Vector2i(Math::posmod(Math::floor(ret.y), 2) ? 0 : 1, -1);
				}
				break;
			case TileSet::TILE_LAYOUT_STAIRS_RIGHT:
				ret = Vector2(ret.x - ret.y / 2, ret.y).floor();
				if (in_top_left_triangle) {
					ret += Vector2i(0, -1);
				}
				else if (in_top_right_triangle) {
					ret += Vector2i(1, -1);
				}
				break;
			case TileSet::TILE_LAYOUT_STAIRS_DOWN:
				ret = Vector2(ret.x * 2, ret.y / 2 - ret.x).floor();
				if (in_top_left_triangle) {
					ret += Vector2i(-1, 0);
				}
				else if (in_top_right_triangle) {
					ret += Vector2i(1, -1);
				}
				break;
			case TileSet::TILE_LAYOUT_DIAMOND_RIGHT:
				ret = Vector2(ret.x - ret.y / 2, ret.y / 2 + ret.x).floor();
				if (in_top_left_triangle) {
					ret += Vector2i(0, -1);
				}
				else if (in_top_right_triangle) {
					ret += Vector2i(1, 0);
				}
				break;
			case TileSet::TILE_LAYOUT_DIAMOND_DOWN:
				ret = Vector2(ret.x + ret.y / 2, ret.y / 2 - ret.x).floor();
				if (in_top_left_triangle) {
					ret += Vector2i(-1, 0);
				}
				else if (in_top_right_triangle) {
					ret += Vector2i(0, -1);
				}
				break;
			}
		}
		else { // TILE_OFFSET_AXIS_VERTICAL.
			// Smart floor of the position.
			Vector2 raw_pos = ret;
			if (Math::posmod(Math::floor(ret.x), 2) ^
				(tile_layout == TileSet::TILE_LAYOUT_STACKED_OFFSET)) {
				ret = Vector2(Math::floor(ret.x), Math::floor(ret.y + 0.5) - 0.5);
			}
			else {
				ret = ret.floor();
			}

			// Compute the tile offset, and if we might the output for a neighbor top tile.
			Vector2 in_tile_pos = raw_pos - ret;
			bool in_top_left_triangle = (in_tile_pos - Vector2(0.0, 0.5))
											.cross(Vector2(1.0 / overlapping_ratio - 1, -0.5)) > 0;
			bool in_bottom_left_triangle =
				(in_tile_pos - Vector2(0.0, 0.5))
					.cross(Vector2(1.0 / overlapping_ratio - 1, 0.5)) <= 0;

			switch (tile_layout) {
			case TileSet::TILE_LAYOUT_STACKED:
				ret = ret.floor();
				if (in_top_left_triangle) {
					ret += Vector2i(-1, Math::posmod(Math::floor(ret.x), 2) ? 0 : -1);
				}
				else if (in_bottom_left_triangle) {
					ret += Vector2i(-1, Math::posmod(Math::floor(ret.x), 2) ? 1 : 0);
				}
				break;
			case TileSet::TILE_LAYOUT_STACKED_OFFSET:
				ret = ret.floor();
				if (in_top_left_triangle) {
					ret += Vector2i(-1, Math::posmod(Math::floor(ret.x), 2) ? -1 : 0);
				}
				else if (in_bottom_left_triangle) {
					ret += Vector2i(-1, Math::posmod(Math::floor(ret.x), 2) ? 0 : 1);
				}
				break;
			case TileSet::TILE_LAYOUT_STAIRS_RIGHT:
				ret = Vector2(ret.x / 2 - ret.y, ret.y * 2).floor();
				if (in_top_left_triangle) {
					ret += Vector2i(0, -1);
				}
				else if (in_bottom_left_triangle) {
					ret += Vector2i(-1, 1);
				}
				break;
			case TileSet::TILE_LAYOUT_STAIRS_DOWN:
				ret = Vector2(ret.x, ret.y - ret.x / 2).floor();
				if (in_top_left_triangle) {
					ret += Vector2i(-1, 0);
				}
				else if (in_bottom_left_triangle) {
					ret += Vector2i(-1, 1);
				}
				break;
			case TileSet::TILE_LAYOUT_DIAMOND_RIGHT:
				ret = Vector2(ret.x / 2 - ret.y, ret.y + ret.x / 2).floor();
				if (in_top_left_triangle) {
					ret += Vector2i(0, -1);
				}
				else if (in_bottom_left_triangle) {
					ret += Vector2i(-1, 0);
				}
				break;
			case TileSet::TILE_LAYOUT_DIAMOND_DOWN:
				ret = Vector2(ret.x / 2 + ret.y, ret.y - ret.x / 2).floor();
				if (in_top_left_triangle) {
					ret += Vector2i(-1, 0);
				}
				else if (in_bottom_left_triangle) {
					ret += Vector2i(0, 1);
				}
				break;
			}
		}
	}
	else {
		ret = (ret + Vector2(0.00005, 0.00005)).floor();
	}
	return Vector2i(ret);
}

bool TileSet::is_existing_neighbor(TileSet::CellNeighbor p_cell_neighbor) const
{
	if (tile_shape == TileSet::TILE_SHAPE_SQUARE) {
		return p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_SIDE ||
			   p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER ||
			   p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_SIDE ||
			   p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER ||
			   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_SIDE ||
			   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER ||
			   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_SIDE ||
			   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER;

	}
	else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC) {
		return p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_CORNER ||
			   p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE ||
			   p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_CORNER ||
			   p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE ||
			   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_CORNER ||
			   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE ||
			   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_CORNER ||
			   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE;
	}
	else {
		if (tile_offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
			return p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_SIDE ||
				   p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE ||
				   p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE ||
				   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_SIDE ||
				   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE ||
				   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE;
		}
		else {
			return p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE ||
				   p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_SIDE ||
				   p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE ||
				   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE ||
				   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_SIDE ||
				   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE;
		}
	}
}

Vector2i TileSet::get_neighbor_cell(
	const Vector2i& p_coords, TileSet::CellNeighbor p_cell_neighbor) const
{
	if (tile_shape == TileSet::TILE_SHAPE_SQUARE) {
		switch (p_cell_neighbor) {
		case TileSet::CELL_NEIGHBOR_RIGHT_SIDE:
			return p_coords + Vector2i(1, 0);
		case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER:
			return p_coords + Vector2i(1, 1);
		case TileSet::CELL_NEIGHBOR_BOTTOM_SIDE:
			return p_coords + Vector2i(0, 1);
		case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER:
			return p_coords + Vector2i(-1, 1);
		case TileSet::CELL_NEIGHBOR_LEFT_SIDE:
			return p_coords + Vector2i(-1, 0);
		case TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER:
			return p_coords + Vector2i(-1, -1);
		case TileSet::CELL_NEIGHBOR_TOP_SIDE:
			return p_coords + Vector2i(0, -1);
		case TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER:
			return p_coords + Vector2i(1, -1);
		default:
			ERR_FAIL_V(p_coords);
		}
	}
	else { // Half-offset shapes (square and hexagon).
		switch (tile_layout) {
		case TileSet::TILE_LAYOUT_STACKED: {
			if (tile_offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
				bool is_offset = p_coords.y % 2;
				if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
						p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_CORNER) ||
					(tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
						p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_SIDE)) {
					return p_coords + Vector2i(1, 0);
				}
				else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE) {
					return p_coords + Vector2i(is_offset ? 1 : 0, 1);
				}
				else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
						   p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_CORNER) {
					return p_coords + Vector2i(0, 2);
				}
				else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE) {
					return p_coords + Vector2i(is_offset ? 0 : -1, 1);
				}
				else if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_CORNER) ||
						   (tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_SIDE)) {
					return p_coords + Vector2i(-1, 0);
				}
				else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE) {
					return p_coords + Vector2i(is_offset ? 0 : -1, -1);
				}
				else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
						   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_CORNER) {
					return p_coords + Vector2i(0, -2);
				}
				else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE) {
					return p_coords + Vector2i(is_offset ? 1 : 0, -1);
				}
				else {
					ERR_FAIL_V(p_coords);
				}
			}
			else {
				bool is_offset = p_coords.x % 2;

				if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
						p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_CORNER) ||
					(tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
						p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_SIDE)) {
					return p_coords + Vector2i(0, 1);
				}
				else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE) {
					return p_coords + Vector2i(1, is_offset ? 1 : 0);
				}
				else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
						   p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_CORNER) {
					return p_coords + Vector2i(2, 0);
				}
				else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE) {
					return p_coords + Vector2i(1, is_offset ? 0 : -1);
				}
				else if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_CORNER) ||
						   (tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_SIDE)) {
					return p_coords + Vector2i(0, -1);
				}
				else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE) {
					return p_coords + Vector2i(-1, is_offset ? 0 : -1);
				}
				else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
						   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_CORNER) {
					return p_coords + Vector2i(-2, 0);
				}
				else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE) {
					return p_coords + Vector2i(-1, is_offset ? 1 : 0);
				}
				else {
					ERR_FAIL_V(p_coords);
				}
			}
		} break;
		case TileSet::TILE_LAYOUT_STACKED_OFFSET: {
			if (tile_offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
				bool is_offset = p_coords.y % 2;

				if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
						p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_CORNER) ||
					(tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
						p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_SIDE)) {
					return p_coords + Vector2i(1, 0);
				}
				else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE) {
					return p_coords + Vector2i(is_offset ? 0 : 1, 1);
				}
				else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
						   p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_CORNER) {
					return p_coords + Vector2i(0, 2);
				}
				else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE) {
					return p_coords + Vector2i(is_offset ? -1 : 0, 1);
				}
				else if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_CORNER) ||
						   (tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_SIDE)) {
					return p_coords + Vector2i(-1, 0);
				}
				else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE) {
					return p_coords + Vector2i(is_offset ? -1 : 0, -1);
				}
				else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
						   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_CORNER) {
					return p_coords + Vector2i(0, -2);
				}
				else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE) {
					return p_coords + Vector2i(is_offset ? 0 : 1, -1);
				}
				else {
					ERR_FAIL_V(p_coords);
				}
			}
			else {
				bool is_offset = p_coords.x % 2;

				if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
						p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_CORNER) ||
					(tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
						p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_SIDE)) {
					return p_coords + Vector2i(0, 1);
				}
				else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE) {
					return p_coords + Vector2i(1, is_offset ? 0 : 1);
				}
				else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
						   p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_CORNER) {
					return p_coords + Vector2i(2, 0);
				}
				else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE) {
					return p_coords + Vector2i(1, is_offset ? -1 : 0);
				}
				else if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_CORNER) ||
						   (tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_SIDE)) {
					return p_coords + Vector2i(0, -1);
				}
				else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE) {
					return p_coords + Vector2i(-1, is_offset ? -1 : 0);
				}
				else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
						   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_CORNER) {
					return p_coords + Vector2i(-2, 0);
				}
				else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE) {
					return p_coords + Vector2i(-1, is_offset ? 0 : 1);
				}
				else {
					ERR_FAIL_V(p_coords);
				}
			}
		} break;
		case TileSet::TILE_LAYOUT_STAIRS_RIGHT:
		case TileSet::TILE_LAYOUT_STAIRS_DOWN: {
			if ((tile_layout == TileSet::TILE_LAYOUT_STAIRS_RIGHT) ^
				(tile_offset_axis == TileSet::TILE_OFFSET_AXIS_VERTICAL)) {
				if (tile_offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
					if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_CORNER) ||
						(tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
							p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_SIDE)) {
						return p_coords + Vector2i(1, 0);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE) {
						return p_coords + Vector2i(0, 1);
					}
					else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_CORNER) {
						return p_coords + Vector2i(-1, 2);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE) {
						return p_coords + Vector2i(-1, 1);
					}
					else if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
								   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_CORNER) ||
							   (tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
								   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_SIDE)) {
						return p_coords + Vector2i(-1, 0);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE) {
						return p_coords + Vector2i(0, -1);
					}
					else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_CORNER) {
						return p_coords + Vector2i(1, -2);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE) {
						return p_coords + Vector2i(1, -1);
					}
					else {
						ERR_FAIL_V(p_coords);
					}

				}
				else {
					if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_CORNER) ||
						(tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
							p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_SIDE)) {
						return p_coords + Vector2i(0, 1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE) {
						return p_coords + Vector2i(1, 0);
					}
					else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_CORNER) {
						return p_coords + Vector2i(2, -1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE) {
						return p_coords + Vector2i(1, -1);
					}
					else if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
								   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_CORNER) ||
							   (tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
								   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_SIDE)) {
						return p_coords + Vector2i(0, -1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE) {
						return p_coords + Vector2i(-1, 0);
					}
					else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_CORNER) {
						return p_coords + Vector2i(-2, 1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE) {
						return p_coords + Vector2i(-1, 1);
					}
					else {
						ERR_FAIL_V(p_coords);
					}
				}
			}
			else {
				if (tile_offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
					if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_CORNER) ||
						(tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
							p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_SIDE)) {
						return p_coords + Vector2i(2, -1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE) {
						return p_coords + Vector2i(1, 0);
					}
					else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_CORNER) {
						return p_coords + Vector2i(0, 1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE) {
						return p_coords + Vector2i(-1, 1);
					}
					else if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
								   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_CORNER) ||
							   (tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
								   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_SIDE)) {
						return p_coords + Vector2i(-2, 1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE) {
						return p_coords + Vector2i(-1, 0);
					}
					else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_CORNER) {
						return p_coords + Vector2i(0, -1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE) {
						return p_coords + Vector2i(1, -1);
					}
					else {
						ERR_FAIL_V(p_coords);
					}

				}
				else {
					if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_CORNER) ||
						(tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
							p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_SIDE)) {
						return p_coords + Vector2i(-1, 2);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE) {
						return p_coords + Vector2i(0, 1);
					}
					else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_CORNER) {
						return p_coords + Vector2i(1, 0);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE) {
						return p_coords + Vector2i(1, -1);
					}
					else if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
								   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_CORNER) ||
							   (tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
								   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_SIDE)) {
						return p_coords + Vector2i(1, -2);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE) {
						return p_coords + Vector2i(0, -1);
					}
					else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_CORNER) {
						return p_coords + Vector2i(-1, 0);

					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE) {
						return p_coords + Vector2i(-1, 1);
					}
					else {
						ERR_FAIL_V(p_coords);
					}
				}
			}
		} break;
		case TileSet::TILE_LAYOUT_DIAMOND_RIGHT:
		case TileSet::TILE_LAYOUT_DIAMOND_DOWN: {
			if ((tile_layout == TileSet::TILE_LAYOUT_DIAMOND_RIGHT) ^
				(tile_offset_axis == TileSet::TILE_OFFSET_AXIS_VERTICAL)) {
				if (tile_offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
					if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_CORNER) ||
						(tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
							p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_SIDE)) {
						return p_coords + Vector2i(1, 1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE) {
						return p_coords + Vector2i(0, 1);
					}
					else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_CORNER) {
						return p_coords + Vector2i(-1, 1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE) {
						return p_coords + Vector2i(-1, 0);
					}
					else if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
								   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_CORNER) ||
							   (tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
								   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_SIDE)) {
						return p_coords + Vector2i(-1, -1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE) {
						return p_coords + Vector2i(0, -1);
					}
					else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_CORNER) {
						return p_coords + Vector2i(1, -1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE) {
						return p_coords + Vector2i(1, 0);
					}
					else {
						ERR_FAIL_V(p_coords);
					}

				}
				else {
					if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_CORNER) ||
						(tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
							p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_SIDE)) {
						return p_coords + Vector2i(1, 1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE) {
						return p_coords + Vector2i(1, 0);
					}
					else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_CORNER) {
						return p_coords + Vector2i(1, -1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE) {
						return p_coords + Vector2i(0, -1);
					}
					else if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
								   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_CORNER) ||
							   (tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
								   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_SIDE)) {
						return p_coords + Vector2i(-1, -1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE) {
						return p_coords + Vector2i(-1, 0);
					}
					else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_CORNER) {
						return p_coords + Vector2i(-1, 1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE) {
						return p_coords + Vector2i(0, 1);
					}
					else {
						ERR_FAIL_V(p_coords);
					}
				}
			}
			else {
				if (tile_offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
					if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_CORNER) ||
						(tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
							p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_SIDE)) {
						return p_coords + Vector2i(1, -1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE) {
						return p_coords + Vector2i(1, 0);
					}
					else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_CORNER) {
						return p_coords + Vector2i(1, 1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE) {
						return p_coords + Vector2i(0, 1);
					}
					else if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
								   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_CORNER) ||
							   (tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
								   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_SIDE)) {
						return p_coords + Vector2i(-1, 1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE) {
						return p_coords + Vector2i(-1, 0);
					}
					else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_CORNER) {
						return p_coords + Vector2i(-1, -1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE) {
						return p_coords + Vector2i(0, -1);
					}
					else {
						ERR_FAIL_V(p_coords);
					}

				}
				else {
					if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_CORNER) ||
						(tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
							p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_SIDE)) {
						return p_coords + Vector2i(-1, 1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE) {
						return p_coords + Vector2i(0, 1);
					}
					else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_RIGHT_CORNER) {
						return p_coords + Vector2i(1, 1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE) {
						return p_coords + Vector2i(1, 0);
					}
					else if ((tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
								   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_CORNER) ||
							   (tile_shape != TileSet::TILE_SHAPE_ISOMETRIC &&
								   p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_SIDE)) {
						return p_coords + Vector2i(1, -1);
					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE) {
						return p_coords + Vector2i(0, -1);
					}
					else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC &&
							   p_cell_neighbor == TileSet::CELL_NEIGHBOR_LEFT_CORNER) {
						return p_coords + Vector2i(-1, -1);

					}
					else if (p_cell_neighbor == TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE) {
						return p_coords + Vector2i(-1, 0);
					}
					else {
						ERR_FAIL_V(p_coords);
					}
				}
			}
		} break;
		default:
			ERR_FAIL_V(p_coords);
		}
	}
}

Vector2i TileSet::map_pattern(const Vector2i& p_position_in_tilemap,
	const Vector2i& p_coords_in_pattern, Ref<TileMapPattern> p_pattern) const
{
	ERR_FAIL_COND_V(p_pattern.is_null(), Vector2i());
	ERR_FAIL_COND_V(!p_pattern->has_cell(p_coords_in_pattern), Vector2i());

	Vector2i output = p_position_in_tilemap + p_coords_in_pattern;
	if (tile_shape != TileSet::TILE_SHAPE_SQUARE) {
		if (tile_layout == TileSet::TILE_LAYOUT_STACKED) {
			if (tile_offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL &&
				bool(p_position_in_tilemap.y % 2) && bool(p_coords_in_pattern.y % 2)) {
				output.x += 1;
			}
			else if (tile_offset_axis == TileSet::TILE_OFFSET_AXIS_VERTICAL &&
					   bool(p_position_in_tilemap.x % 2) && bool(p_coords_in_pattern.x % 2)) {
				output.y += 1;
			}
		}
		else if (tile_layout == TileSet::TILE_LAYOUT_STACKED_OFFSET) {
			if (tile_offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL &&
				bool(p_position_in_tilemap.y % 2) && bool(p_coords_in_pattern.y % 2)) {
				output.x -= 1;
			}
			else if (tile_offset_axis == TileSet::TILE_OFFSET_AXIS_VERTICAL &&
					   bool(p_position_in_tilemap.x % 2) && bool(p_coords_in_pattern.x % 2)) {
				output.y -= 1;
			}
		}
	}

	return output;
}

void TileSet::draw_cells_outline(CanvasItem* p_canvas_item, const RBSet<Vector2i>& p_cells,
	Color p_color, Transform2D p_transform) const
{
	Vector<Vector2> polygon = get_tile_shape_polygon();
	for (const Vector2i& E : p_cells) {
		Vector2 center = map_to_local(E);

#define DRAW_SIDE_IF_NEEDED(side, polygon_index_from, polygon_index_to)                            \
	if (!p_cells.has(get_neighbor_cell(E, side))) {                                                \
		Vector2 from = p_transform.xform(center + polygon[polygon_index_from] * tile_size);        \
		Vector2 to = p_transform.xform(center + polygon[polygon_index_to] * tile_size);            \
		p_canvas_item->draw_line(from, to, p_color);                                               \
	}

		if (tile_shape == TileSet::TILE_SHAPE_SQUARE) {
			DRAW_SIDE_IF_NEEDED(TileSet::CELL_NEIGHBOR_RIGHT_SIDE, 1, 2);
			DRAW_SIDE_IF_NEEDED(TileSet::CELL_NEIGHBOR_BOTTOM_SIDE, 2, 3);
			DRAW_SIDE_IF_NEEDED(TileSet::CELL_NEIGHBOR_LEFT_SIDE, 3, 0);
			DRAW_SIDE_IF_NEEDED(TileSet::CELL_NEIGHBOR_TOP_SIDE, 0, 1);
		}
		else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC) {
			DRAW_SIDE_IF_NEEDED(TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE, 2, 3);
			DRAW_SIDE_IF_NEEDED(TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE, 1, 2);
			DRAW_SIDE_IF_NEEDED(TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE, 0, 1);
			DRAW_SIDE_IF_NEEDED(TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE, 3, 0);
		}
		else {
			if (tile_offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
				DRAW_SIDE_IF_NEEDED(TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE, 3, 4);
				DRAW_SIDE_IF_NEEDED(TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE, 2, 3);
				DRAW_SIDE_IF_NEEDED(TileSet::CELL_NEIGHBOR_LEFT_SIDE, 1, 2);
				DRAW_SIDE_IF_NEEDED(TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE, 0, 1);
				DRAW_SIDE_IF_NEEDED(TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE, 5, 0);
				DRAW_SIDE_IF_NEEDED(TileSet::CELL_NEIGHBOR_RIGHT_SIDE, 4, 5);
			}
			else {
				DRAW_SIDE_IF_NEEDED(TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE, 3, 4);
				DRAW_SIDE_IF_NEEDED(TileSet::CELL_NEIGHBOR_BOTTOM_SIDE, 4, 5);
				DRAW_SIDE_IF_NEEDED(TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE, 5, 0);
				DRAW_SIDE_IF_NEEDED(TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE, 0, 1);
				DRAW_SIDE_IF_NEEDED(TileSet::CELL_NEIGHBOR_TOP_SIDE, 1, 2);
				DRAW_SIDE_IF_NEEDED(TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE, 2, 3);
			}
		}
	}
#undef DRAW_SIDE_IF_NEEDED
}

Vector<Point2> TileSet::get_terrain_polygon(int p_terrain_set)
{
	if (tile_shape == TileSet::TILE_SHAPE_SQUARE) {
		return _get_square_terrain_polygon(tile_size);
	}
	else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC) {
		return _get_isometric_terrain_polygon(tile_size);
	}
	else {
		float overlap = 0.0;
		switch (tile_shape) {
		case TileSet::TILE_SHAPE_HEXAGON:
			overlap = 0.25;
			break;
		case TileSet::TILE_SHAPE_HALF_OFFSET_SQUARE:
			overlap = 0.0;
			break;
		default:
			break;
		}
		return _get_half_offset_terrain_polygon(tile_size, overlap, tile_offset_axis);
	}
}

Vector<Point2> TileSet::get_terrain_peering_bit_polygon(
	int p_terrain_set, TileSet::CellNeighbor p_bit)
{
	ERR_FAIL_COND_V(
		p_terrain_set < 0 || p_terrain_set >= get_terrain_sets_count(), Vector<Point2>());

	TileSet::TerrainMode terrain_mode = get_terrain_set_mode(p_terrain_set);

	if (tile_shape == TileSet::TILE_SHAPE_SQUARE) {
		if (terrain_mode == TileSet::TERRAIN_MODE_MATCH_CORNERS_AND_SIDES) {
			return _get_square_corner_or_side_terrain_peering_bit_polygon(tile_size, p_bit);
		}
		else if (terrain_mode == TileSet::TERRAIN_MODE_MATCH_CORNERS) {
			return _get_square_corner_terrain_peering_bit_polygon(tile_size, p_bit);
		}
		else { // TileData::TERRAIN_MODE_MATCH_SIDES
			return _get_square_side_terrain_peering_bit_polygon(tile_size, p_bit);
		}
	}
	else if (tile_shape == TileSet::TILE_SHAPE_ISOMETRIC) {
		if (terrain_mode == TileSet::TERRAIN_MODE_MATCH_CORNERS_AND_SIDES) {
			return _get_isometric_corner_or_side_terrain_peering_bit_polygon(tile_size, p_bit);
		}
		else if (terrain_mode == TileSet::TERRAIN_MODE_MATCH_CORNERS) {
			return _get_isometric_corner_terrain_peering_bit_polygon(tile_size, p_bit);
		}
		else { // TileData::TERRAIN_MODE_MATCH_SIDES
			return _get_isometric_side_terrain_peering_bit_polygon(tile_size, p_bit);
		}
	}
	else {
		float overlap = 0.0;
		switch (tile_shape) {
		case TileSet::TILE_SHAPE_HEXAGON:
			overlap = 0.25;
			break;
		case TileSet::TILE_SHAPE_HALF_OFFSET_SQUARE:
			overlap = 0.0;
			break;
		default:
			break;
		}
		if (terrain_mode == TileSet::TERRAIN_MODE_MATCH_CORNERS_AND_SIDES) {
			return _get_half_offset_corner_or_side_terrain_peering_bit_polygon(
				tile_size, overlap, tile_offset_axis, p_bit);
		}
		else if (terrain_mode == TileSet::TERRAIN_MODE_MATCH_CORNERS) {
			return _get_half_offset_corner_terrain_peering_bit_polygon(
				tile_size, overlap, tile_offset_axis, p_bit);
		}
		else { // TileData::TERRAIN_MODE_MATCH_SIDES
			return _get_half_offset_side_terrain_peering_bit_polygon(
				tile_size, overlap, tile_offset_axis, p_bit);
		}
	}
}

#define TERRAIN_ALPHA 0.6

Vector<Vector<Ref<Texture2D>>> TileSet::generate_terrains_icons(Size2i p_size)
{
	// Counts the number of matching terrain tiles and find the best matching icon.
	struct Count
	{
		int count = 0;
		float probability = 0.0;
		Ref<Texture2D> texture;
		Rect2i region;
	};

	Vector<Vector<Ref<Texture2D>>> output;
	LocalVector<LocalVector<Count>> counts;
	output.resize(get_terrain_sets_count());
	counts.resize(get_terrain_sets_count());
	for (int terrain_set = 0; terrain_set < get_terrain_sets_count(); terrain_set++) {
		output.write[terrain_set].resize(get_terrains_count(terrain_set));
		counts[terrain_set].resize(get_terrains_count(terrain_set));
	}

	for (int source_index = 0; source_index < get_source_count(); source_index++) {
		int source_id = get_source_id(source_index);
		Ref<TileSetSource> source = get_source(source_id);

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
						ERR_FAIL_INDEX_V(terrain_set, get_terrain_sets_count(),
							Vector<Vector<Ref<Texture2D>>>());

						LocalVector<int> bit_counts;
						bit_counts.resize(get_terrains_count(terrain_set));
						for (int terrain = 0; terrain < get_terrains_count(terrain_set);
							 terrain++) {
							bit_counts[terrain] = 0;
						}
						if (tile_data->get_terrain() >= 0) {
							bit_counts[tile_data->get_terrain()] += 10;
						}
						for (int terrain_bit = 0; terrain_bit < TileSet::CELL_NEIGHBOR_MAX;
							 terrain_bit++) {
							TileSet::CellNeighbor cell_neighbor =
								TileSet::CellNeighbor(terrain_bit);
							if (is_valid_terrain_peering_bit(terrain_set, cell_neighbor)) {
								int terrain = tile_data->get_terrain_peering_bit(cell_neighbor);
								if (terrain >= 0) {
									if (terrain >= (int)bit_counts.size()) {
										WARN_PRINT(
											vformat("Invalid terrain peering bit: %d", terrain));
									}
									else {
										bit_counts[terrain] += 1;
									}
								}
							}
						}

						for (int terrain = 0; terrain < get_terrains_count(terrain_set);
							 terrain++) {
							if ((bit_counts[terrain] > counts[terrain_set][terrain].count) ||
								(bit_counts[terrain] == counts[terrain_set][terrain].count &&
									tile_data->get_probability() >
										counts[terrain_set][terrain].probability)) {
								counts[terrain_set][terrain].count = bit_counts[terrain];
								counts[terrain_set][terrain].probability =
									tile_data->get_probability();
								counts[terrain_set][terrain].texture = atlas_source->get_texture();
								counts[terrain_set][terrain].region =
									atlas_source->get_tile_texture_region(tile_id);
							}
						}
					}
				}
			}
		}
	}

	// Generate the icons.
	for (int terrain_set = 0; terrain_set < get_terrain_sets_count(); terrain_set++) {
		for (int terrain = 0; terrain < get_terrains_count(terrain_set); terrain++) {
			Ref<Image> dst_image;
			dst_image.instantiate();
			if (counts[terrain_set][terrain].count > 0) {
				// Get the best tile.
				Ref<Texture2D> src_texture = counts[terrain_set][terrain].texture;
				ERR_FAIL_COND_V(src_texture.is_null(), output);
				Ref<Image> src_image = src_texture->get_image();
				ERR_FAIL_COND_V(src_image.is_null(), output);
				Rect2i region = counts[terrain_set][terrain].region;

				dst_image->initialize_data(
					region.size.x, region.size.y, false, src_image->get_format());
				dst_image->blit_rect(src_image, region, Point2i());
				dst_image->convert(Image::FORMAT_RGBA8);
				dst_image->resize(p_size.x, p_size.y, Image::INTERPOLATE_NEAREST);
			}
			else {
				dst_image->initialize_data(1, 1, false, Image::FORMAT_RGBA8);
				dst_image->set_pixel(0, 0, get_terrain_color(terrain_set, terrain));
			}
			Ref<ImageTexture> icon = ImageTexture::create_from_image(dst_image);
			icon->set_size_override(p_size);
			output.write[terrain_set].write[terrain] = icon;
		}
	}
	return output;
}

void TileSet::_source_changed()
{
	terrains_cache_dirty = true;
	emit_changed();
}

Vector<Point2> TileSet::_get_square_terrain_polygon(Vector2i p_size)
{
	Rect2 rect(-Vector2(p_size) / 6.0, Vector2(p_size) / 3.0);
	return {rect.position, Vector2(rect.get_end().x, rect.position.y), rect.get_end(),
		Vector2(rect.position.x, rect.get_end().y)};
}

Vector<Point2> TileSet::_get_square_corner_or_side_terrain_peering_bit_polygon(
	Vector2i p_size, TileSet::CellNeighbor p_bit)
{
	Rect2 bit_rect;
	bit_rect.size = Vector2(p_size) / 3;
	switch (p_bit) {
	case TileSet::CELL_NEIGHBOR_RIGHT_SIDE:
		bit_rect.position = Vector2(1, -1);
		break;
	case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER:
		bit_rect.position = Vector2(1, 1);
		break;
	case TileSet::CELL_NEIGHBOR_BOTTOM_SIDE:
		bit_rect.position = Vector2(-1, 1);
		break;
	case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER:
		bit_rect.position = Vector2(-3, 1);
		break;
	case TileSet::CELL_NEIGHBOR_LEFT_SIDE:
		bit_rect.position = Vector2(-3, -1);
		break;
	case TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER:
		bit_rect.position = Vector2(-3, -3);
		break;
	case TileSet::CELL_NEIGHBOR_TOP_SIDE:
		bit_rect.position = Vector2(-1, -3);
		break;
	case TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER:
		bit_rect.position = Vector2(1, -3);
		break;
	default:
		break;
	}
	bit_rect.position *= Vector2(p_size) / 6.0;

	Vector<Vector2> polygon = {bit_rect.position,
		Vector2(bit_rect.get_end().x, bit_rect.position.y), bit_rect.get_end(),
		Vector2(bit_rect.position.x, bit_rect.get_end().y)};

	return polygon;
}

Vector<Point2> TileSet::_get_square_corner_terrain_peering_bit_polygon(
	Vector2i p_size, TileSet::CellNeighbor p_bit)
{
	Vector2 unit = Vector2(p_size) / 6.0;
	Vector<Vector2> polygon;
	switch (p_bit) {
	case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER:
		polygon.push_back(Vector2(0, 3) * unit);
		polygon.push_back(Vector2(3, 3) * unit);
		polygon.push_back(Vector2(3, 0) * unit);
		polygon.push_back(Vector2(1, 0) * unit);
		polygon.push_back(Vector2(1, 1) * unit);
		polygon.push_back(Vector2(0, 1) * unit);
		break;
	case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER:
		polygon.push_back(Vector2(0, 3) * unit);
		polygon.push_back(Vector2(-3, 3) * unit);
		polygon.push_back(Vector2(-3, 0) * unit);
		polygon.push_back(Vector2(-1, 0) * unit);
		polygon.push_back(Vector2(-1, 1) * unit);
		polygon.push_back(Vector2(0, 1) * unit);
		break;
	case TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER:
		polygon.push_back(Vector2(0, -3) * unit);
		polygon.push_back(Vector2(-3, -3) * unit);
		polygon.push_back(Vector2(-3, 0) * unit);
		polygon.push_back(Vector2(-1, 0) * unit);
		polygon.push_back(Vector2(-1, -1) * unit);
		polygon.push_back(Vector2(0, -1) * unit);
		break;
	case TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER:
		polygon.push_back(Vector2(0, -3) * unit);
		polygon.push_back(Vector2(3, -3) * unit);
		polygon.push_back(Vector2(3, 0) * unit);
		polygon.push_back(Vector2(1, 0) * unit);
		polygon.push_back(Vector2(1, -1) * unit);
		polygon.push_back(Vector2(0, -1) * unit);
		break;
	default:
		break;
	}
	return polygon;
}

Vector<Point2> TileSet::_get_square_side_terrain_peering_bit_polygon(
	Vector2i p_size, TileSet::CellNeighbor p_bit)
{
	Vector2 unit = Vector2(p_size) / 6.0;
	Vector<Vector2> polygon;
	switch (p_bit) {
	case TileSet::CELL_NEIGHBOR_RIGHT_SIDE:
		polygon.push_back(Vector2(1, -1) * unit);
		polygon.push_back(Vector2(3, -3) * unit);
		polygon.push_back(Vector2(3, 3) * unit);
		polygon.push_back(Vector2(1, 1) * unit);
		break;
	case TileSet::CELL_NEIGHBOR_BOTTOM_SIDE:
		polygon.push_back(Vector2(-1, 1) * unit);
		polygon.push_back(Vector2(-3, 3) * unit);
		polygon.push_back(Vector2(3, 3) * unit);
		polygon.push_back(Vector2(1, 1) * unit);
		break;
	case TileSet::CELL_NEIGHBOR_LEFT_SIDE:
		polygon.push_back(Vector2(-1, -1) * unit);
		polygon.push_back(Vector2(-3, -3) * unit);
		polygon.push_back(Vector2(-3, 3) * unit);
		polygon.push_back(Vector2(-1, 1) * unit);
		break;
	case TileSet::CELL_NEIGHBOR_TOP_SIDE:
		polygon.push_back(Vector2(-1, -1) * unit);
		polygon.push_back(Vector2(-3, -3) * unit);
		polygon.push_back(Vector2(3, -3) * unit);
		polygon.push_back(Vector2(1, -1) * unit);
		break;
	default:
		break;
	}
	return polygon;
}

Vector<Point2> TileSet::_get_isometric_terrain_polygon(Vector2i p_size)
{
	Vector2 unit = Vector2(p_size) / 6.0;
	return {
		Vector2(1, 0) * unit,
		Vector2(0, 1) * unit,
		Vector2(-1, 0) * unit,
		Vector2(0, -1) * unit,
	};
}

Vector<Point2> TileSet::_get_isometric_corner_or_side_terrain_peering_bit_polygon(
	Vector2i p_size, TileSet::CellNeighbor p_bit)
{
	Vector2 unit = Vector2(p_size) / 6.0;
	Vector<Vector2> polygon;
	switch (p_bit) {
	case TileSet::CELL_NEIGHBOR_RIGHT_CORNER:
		polygon.push_back(Vector2(1, 0) * unit);
		polygon.push_back(Vector2(2, -1) * unit);
		polygon.push_back(Vector2(3, 0) * unit);
		polygon.push_back(Vector2(2, 1) * unit);
		break;
	case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE:
		polygon.push_back(Vector2(0, 1) * unit);
		polygon.push_back(Vector2(1, 2) * unit);
		polygon.push_back(Vector2(2, 1) * unit);
		polygon.push_back(Vector2(1, 0) * unit);
		break;
	case TileSet::CELL_NEIGHBOR_BOTTOM_CORNER:
		polygon.push_back(Vector2(0, 1) * unit);
		polygon.push_back(Vector2(-1, 2) * unit);
		polygon.push_back(Vector2(0, 3) * unit);
		polygon.push_back(Vector2(1, 2) * unit);
		break;
	case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE:
		polygon.push_back(Vector2(0, 1) * unit);
		polygon.push_back(Vector2(-1, 2) * unit);
		polygon.push_back(Vector2(-2, 1) * unit);
		polygon.push_back(Vector2(-1, 0) * unit);
		break;
	case TileSet::CELL_NEIGHBOR_LEFT_CORNER:
		polygon.push_back(Vector2(-1, 0) * unit);
		polygon.push_back(Vector2(-2, -1) * unit);
		polygon.push_back(Vector2(-3, 0) * unit);
		polygon.push_back(Vector2(-2, 1) * unit);
		break;
	case TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE:
		polygon.push_back(Vector2(0, -1) * unit);
		polygon.push_back(Vector2(-1, -2) * unit);
		polygon.push_back(Vector2(-2, -1) * unit);
		polygon.push_back(Vector2(-1, 0) * unit);
		break;
	case TileSet::CELL_NEIGHBOR_TOP_CORNER:
		polygon.push_back(Vector2(0, -1) * unit);
		polygon.push_back(Vector2(-1, -2) * unit);
		polygon.push_back(Vector2(0, -3) * unit);
		polygon.push_back(Vector2(1, -2) * unit);
		break;
	case TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE:
		polygon.push_back(Vector2(0, -1) * unit);
		polygon.push_back(Vector2(1, -2) * unit);
		polygon.push_back(Vector2(2, -1) * unit);
		polygon.push_back(Vector2(1, 0) * unit);
		break;
	default:
		break;
	}
	return polygon;
}

Vector<Point2> TileSet::_get_isometric_corner_terrain_peering_bit_polygon(
	Vector2i p_size, TileSet::CellNeighbor p_bit)
{
	Vector2 unit = Vector2(p_size) / 6.0;
	Vector<Vector2> polygon;
	switch (p_bit) {
	case TileSet::CELL_NEIGHBOR_RIGHT_CORNER:
		polygon.push_back(Vector2(0.5, -0.5) * unit);
		polygon.push_back(Vector2(1.5, -1.5) * unit);
		polygon.push_back(Vector2(3, 0) * unit);
		polygon.push_back(Vector2(1.5, 1.5) * unit);
		polygon.push_back(Vector2(0.5, 0.5) * unit);
		polygon.push_back(Vector2(1, 0) * unit);
		break;
	case TileSet::CELL_NEIGHBOR_BOTTOM_CORNER:
		polygon.push_back(Vector2(-0.5, 0.5) * unit);
		polygon.push_back(Vector2(-1.5, 1.5) * unit);
		polygon.push_back(Vector2(0, 3) * unit);
		polygon.push_back(Vector2(1.5, 1.5) * unit);
		polygon.push_back(Vector2(0.5, 0.5) * unit);
		polygon.push_back(Vector2(0, 1) * unit);
		break;
	case TileSet::CELL_NEIGHBOR_LEFT_CORNER:
		polygon.push_back(Vector2(-0.5, -0.5) * unit);
		polygon.push_back(Vector2(-1.5, -1.5) * unit);
		polygon.push_back(Vector2(-3, 0) * unit);
		polygon.push_back(Vector2(-1.5, 1.5) * unit);
		polygon.push_back(Vector2(-0.5, 0.5) * unit);
		polygon.push_back(Vector2(-1, 0) * unit);
		break;
	case TileSet::CELL_NEIGHBOR_TOP_CORNER:
		polygon.push_back(Vector2(-0.5, -0.5) * unit);
		polygon.push_back(Vector2(-1.5, -1.5) * unit);
		polygon.push_back(Vector2(0, -3) * unit);
		polygon.push_back(Vector2(1.5, -1.5) * unit);
		polygon.push_back(Vector2(0.5, -0.5) * unit);
		polygon.push_back(Vector2(0, -1) * unit);
		break;
	default:
		break;
	}
	return polygon;
}

Vector<Point2> TileSet::_get_isometric_side_terrain_peering_bit_polygon(
	Vector2i p_size, TileSet::CellNeighbor p_bit)
{
	Vector2 unit = Vector2(p_size) / 6.0;
	Vector<Vector2> polygon;
	switch (p_bit) {
	case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE:
		polygon.push_back(Vector2(1, 0) * unit);
		polygon.push_back(Vector2(3, 0) * unit);
		polygon.push_back(Vector2(0, 3) * unit);
		polygon.push_back(Vector2(0, 1) * unit);
		break;
	case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE:
		polygon.push_back(Vector2(-1, 0) * unit);
		polygon.push_back(Vector2(-3, 0) * unit);
		polygon.push_back(Vector2(0, 3) * unit);
		polygon.push_back(Vector2(0, 1) * unit);
		break;
	case TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE:
		polygon.push_back(Vector2(-1, 0) * unit);
		polygon.push_back(Vector2(-3, 0) * unit);
		polygon.push_back(Vector2(0, -3) * unit);
		polygon.push_back(Vector2(0, -1) * unit);
		break;
	case TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE:
		polygon.push_back(Vector2(1, 0) * unit);
		polygon.push_back(Vector2(3, 0) * unit);
		polygon.push_back(Vector2(0, -3) * unit);
		polygon.push_back(Vector2(0, -1) * unit);
		break;
	default:
		break;
	}
	return polygon;
}

Vector<Point2> TileSet::_get_half_offset_terrain_polygon(
	Vector2i p_size, float p_overlap, TileSet::TileOffsetAxis p_offset_axis)
{
	Vector2 unit = Vector2(p_size) / 6.0;
	if (p_offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
		return {
			Vector2(1, 1.0 - p_overlap * 2.0) * unit,
			Vector2(0, 1) * unit,
			Vector2(-1, 1.0 - p_overlap * 2.0) * unit,
			Vector2(-1, -1.0 + p_overlap * 2.0) * unit,
			Vector2(0, -1) * unit,
			Vector2(1, -1.0 + p_overlap * 2.0) * unit,
		};
	}
	else {
		return {
			Vector2(1, 0) * unit,
			Vector2(1.0 - p_overlap * 2.0, -1) * unit,
			Vector2(-1.0 + p_overlap * 2.0, -1) * unit,
			Vector2(-1, 0) * unit,
			Vector2(-1.0 + p_overlap * 2.0, 1) * unit,
			Vector2(1.0 - p_overlap * 2.0, 1) * unit,
		};
	}
}

Vector<Point2> TileSet::_get_half_offset_corner_or_side_terrain_peering_bit_polygon(Vector2i p_size,
	float p_overlap, TileSet::TileOffsetAxis p_offset_axis, TileSet::CellNeighbor p_bit)
{
	Vector<Vector2> point_list = {Vector2(3, (3.0 * (1.0 - p_overlap * 2.0)) / 2.0),
		Vector2(3, 3.0 * (1.0 - p_overlap * 2.0)),
		Vector2(2, 3.0 * (1.0 - (p_overlap * 2.0) * 2.0 / 3.0)), Vector2(1, 3.0 - p_overlap * 2.0),
		Vector2(0, 3), Vector2(-1, 3.0 - p_overlap * 2.0),
		Vector2(-2, 3.0 * (1.0 - (p_overlap * 2.0) * 2.0 / 3.0)),
		Vector2(-3, 3.0 * (1.0 - p_overlap * 2.0)),
		Vector2(-3, (3.0 * (1.0 - p_overlap * 2.0)) / 2.0),
		Vector2(-3, -(3.0 * (1.0 - p_overlap * 2.0)) / 2.0),
		Vector2(-3, -3.0 * (1.0 - p_overlap * 2.0)),
		Vector2(-2, -3.0 * (1.0 - (p_overlap * 2.0) * 2.0 / 3.0)),
		Vector2(-1, -(3.0 - p_overlap * 2.0)), Vector2(0, -3), Vector2(1, -(3.0 - p_overlap * 2.0)),
		Vector2(2, -3.0 * (1.0 - (p_overlap * 2.0) * 2.0 / 3.0)),
		Vector2(3, -3.0 * (1.0 - p_overlap * 2.0)),
		Vector2(3, -(3.0 * (1.0 - p_overlap * 2.0)) / 2.0)};

	Vector2 unit = Vector2(p_size) / 6.0;
	Vector<Vector2> polygon;
	if (p_offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
		for (int i = 0; i < point_list.size(); i++) {
			point_list.write[i] = point_list[i] * unit;
		}
		switch (p_bit) {
		case TileSet::CELL_NEIGHBOR_RIGHT_SIDE:
			polygon.push_back(point_list[17]);
			polygon.push_back(point_list[0]);
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER:
			polygon.push_back(point_list[0]);
			polygon.push_back(point_list[1]);
			polygon.push_back(point_list[2]);
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE:
			polygon.push_back(point_list[2]);
			polygon.push_back(point_list[3]);
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_CORNER:
			polygon.push_back(point_list[3]);
			polygon.push_back(point_list[4]);
			polygon.push_back(point_list[5]);
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE:
			polygon.push_back(point_list[5]);
			polygon.push_back(point_list[6]);
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER:
			polygon.push_back(point_list[6]);
			polygon.push_back(point_list[7]);
			polygon.push_back(point_list[8]);
			break;
		case TileSet::CELL_NEIGHBOR_LEFT_SIDE:
			polygon.push_back(point_list[8]);
			polygon.push_back(point_list[9]);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER:
			polygon.push_back(point_list[9]);
			polygon.push_back(point_list[10]);
			polygon.push_back(point_list[11]);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE:
			polygon.push_back(point_list[11]);
			polygon.push_back(point_list[12]);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_CORNER:
			polygon.push_back(point_list[12]);
			polygon.push_back(point_list[13]);
			polygon.push_back(point_list[14]);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE:
			polygon.push_back(point_list[14]);
			polygon.push_back(point_list[15]);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER:
			polygon.push_back(point_list[15]);
			polygon.push_back(point_list[16]);
			polygon.push_back(point_list[17]);
			break;
		default:
			break;
		}
	}
	else {
		for (int i = 0; i < point_list.size(); i++) {
			point_list.write[i] = Vector2(point_list[i].y, point_list[i].x) * unit;
		}
		switch (p_bit) {
		case TileSet::CELL_NEIGHBOR_RIGHT_CORNER:
			polygon.push_back(point_list[3]);
			polygon.push_back(point_list[4]);
			polygon.push_back(point_list[5]);
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE:
			polygon.push_back(point_list[2]);
			polygon.push_back(point_list[3]);
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER:
			polygon.push_back(point_list[0]);
			polygon.push_back(point_list[1]);
			polygon.push_back(point_list[2]);
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_SIDE:
			polygon.push_back(point_list[17]);
			polygon.push_back(point_list[0]);
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER:
			polygon.push_back(point_list[15]);
			polygon.push_back(point_list[16]);
			polygon.push_back(point_list[17]);
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE:
			polygon.push_back(point_list[14]);
			polygon.push_back(point_list[15]);
			break;
		case TileSet::CELL_NEIGHBOR_LEFT_CORNER:
			polygon.push_back(point_list[12]);
			polygon.push_back(point_list[13]);
			polygon.push_back(point_list[14]);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE:
			polygon.push_back(point_list[11]);
			polygon.push_back(point_list[12]);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER:
			polygon.push_back(point_list[9]);
			polygon.push_back(point_list[10]);
			polygon.push_back(point_list[11]);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_SIDE:
			polygon.push_back(point_list[8]);
			polygon.push_back(point_list[9]);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER:
			polygon.push_back(point_list[6]);
			polygon.push_back(point_list[7]);
			polygon.push_back(point_list[8]);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE:
			polygon.push_back(point_list[5]);
			polygon.push_back(point_list[6]);
			break;
		default:
			break;
		}
	}

	int half_polygon_size = polygon.size();
	for (int i = 0; i < half_polygon_size; i++) {
		polygon.push_back(polygon[half_polygon_size - 1 - i] / 3.0);
	}

	return polygon;
}

Vector<Point2> TileSet::_get_half_offset_corner_terrain_peering_bit_polygon(Vector2i p_size,
	float p_overlap, TileSet::TileOffsetAxis p_offset_axis, TileSet::CellNeighbor p_bit)
{
	Vector<Vector2> point_list = {Vector2(3, 0), Vector2(3, 3.0 * (1.0 - p_overlap * 2.0)),
		Vector2(1.5, (3.0 * (1.0 - p_overlap * 2.0) + 3.0) / 2.0), Vector2(0, 3),
		Vector2(-1.5, (3.0 * (1.0 - p_overlap * 2.0) + 3.0) / 2.0),
		Vector2(-3, 3.0 * (1.0 - p_overlap * 2.0)), Vector2(-3, 0),
		Vector2(-3, -3.0 * (1.0 - p_overlap * 2.0)),
		Vector2(-1.5, -(3.0 * (1.0 - p_overlap * 2.0) + 3.0) / 2.0), Vector2(0, -3),
		Vector2(1.5, -(3.0 * (1.0 - p_overlap * 2.0) + 3.0) / 2.0),
		Vector2(3, -3.0 * (1.0 - p_overlap * 2.0))};

	Vector2 unit = Vector2(p_size) / 6.0;
	Vector<Vector2> polygon;
	if (p_offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
		for (int i = 0; i < point_list.size(); i++) {
			point_list.write[i] = point_list[i] * unit;
		}
		switch (p_bit) {
		case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER:
			polygon.push_back(point_list[0]);
			polygon.push_back(point_list[1]);
			polygon.push_back(point_list[2]);
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_CORNER:
			polygon.push_back(point_list[2]);
			polygon.push_back(point_list[3]);
			polygon.push_back(point_list[4]);
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER:
			polygon.push_back(point_list[4]);
			polygon.push_back(point_list[5]);
			polygon.push_back(point_list[6]);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER:
			polygon.push_back(point_list[6]);
			polygon.push_back(point_list[7]);
			polygon.push_back(point_list[8]);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_CORNER:
			polygon.push_back(point_list[8]);
			polygon.push_back(point_list[9]);
			polygon.push_back(point_list[10]);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER:
			polygon.push_back(point_list[10]);
			polygon.push_back(point_list[11]);
			polygon.push_back(point_list[0]);
			break;
		default:
			break;
		}
	}
	else {
		for (int i = 0; i < point_list.size(); i++) {
			point_list.write[i] = Vector2(point_list[i].y, point_list[i].x) * unit;
		}
		switch (p_bit) {
		case TileSet::CELL_NEIGHBOR_RIGHT_CORNER:
			polygon.push_back(point_list[2]);
			polygon.push_back(point_list[3]);
			polygon.push_back(point_list[4]);
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER:
			polygon.push_back(point_list[0]);
			polygon.push_back(point_list[1]);
			polygon.push_back(point_list[2]);
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_CORNER:
			polygon.push_back(point_list[10]);
			polygon.push_back(point_list[11]);
			polygon.push_back(point_list[0]);
			break;
		case TileSet::CELL_NEIGHBOR_LEFT_CORNER:
			polygon.push_back(point_list[8]);
			polygon.push_back(point_list[9]);
			polygon.push_back(point_list[10]);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_LEFT_CORNER:
			polygon.push_back(point_list[6]);
			polygon.push_back(point_list[7]);
			polygon.push_back(point_list[8]);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_RIGHT_CORNER:
			polygon.push_back(point_list[4]);
			polygon.push_back(point_list[5]);
			polygon.push_back(point_list[6]);
			break;
		default:
			break;
		}
	}

	int half_polygon_size = polygon.size();
	for (int i = 0; i < half_polygon_size; i++) {
		polygon.push_back(polygon[half_polygon_size - 1 - i] / 3.0);
	}

	return polygon;
}

Vector<Point2> TileSet::_get_half_offset_side_terrain_peering_bit_polygon(Vector2i p_size,
	float p_overlap, TileSet::TileOffsetAxis p_offset_axis, TileSet::CellNeighbor p_bit)
{
	Vector<Vector2> point_list = {Vector2(3, 3.0 * (1.0 - p_overlap * 2.0)), Vector2(0, 3),
		Vector2(-3, 3.0 * (1.0 - p_overlap * 2.0)), Vector2(-3, -3.0 * (1.0 - p_overlap * 2.0)),
		Vector2(0, -3), Vector2(3, -3.0 * (1.0 - p_overlap * 2.0))};

	Vector2 unit = Vector2(p_size) / 6.0;
	Vector<Vector2> polygon;
	if (p_offset_axis == TileSet::TILE_OFFSET_AXIS_HORIZONTAL) {
		for (int i = 0; i < point_list.size(); i++) {
			point_list.write[i] = point_list[i] * unit;
		}
		switch (p_bit) {
		case TileSet::CELL_NEIGHBOR_RIGHT_SIDE:
			polygon.push_back(point_list[5]);
			polygon.push_back(point_list[0]);
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE:
			polygon.push_back(point_list[0]);
			polygon.push_back(point_list[1]);
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE:
			polygon.push_back(point_list[1]);
			polygon.push_back(point_list[2]);
			break;
		case TileSet::CELL_NEIGHBOR_LEFT_SIDE:
			polygon.push_back(point_list[2]);
			polygon.push_back(point_list[3]);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE:
			polygon.push_back(point_list[3]);
			polygon.push_back(point_list[4]);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE:
			polygon.push_back(point_list[4]);
			polygon.push_back(point_list[5]);
			break;
		default:
			break;
		}
	}
	else {
		for (int i = 0; i < point_list.size(); i++) {
			point_list.write[i] = Vector2(point_list[i].y, point_list[i].x) * unit;
		}
		switch (p_bit) {
		case TileSet::CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE:
			polygon.push_back(point_list[0]);
			polygon.push_back(point_list[1]);
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_SIDE:
			polygon.push_back(point_list[5]);
			polygon.push_back(point_list[0]);
			break;
		case TileSet::CELL_NEIGHBOR_BOTTOM_LEFT_SIDE:
			polygon.push_back(point_list[4]);
			polygon.push_back(point_list[5]);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_LEFT_SIDE:
			polygon.push_back(point_list[3]);
			polygon.push_back(point_list[4]);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_SIDE:
			polygon.push_back(point_list[2]);
			polygon.push_back(point_list[3]);
			break;
		case TileSet::CELL_NEIGHBOR_TOP_RIGHT_SIDE:
			polygon.push_back(point_list[1]);
			polygon.push_back(point_list[2]);
			break;
		default:
			break;
		}
	}

	int half_polygon_size = polygon.size();
	for (int i = 0; i < half_polygon_size; i++) {
		polygon.push_back(polygon[half_polygon_size - 1 - i] / 3.0);
	}

	return polygon;
}

Vector2i TileSet::transform_coords_layout(const Vector2i& p_coords,
	TileSet::TileOffsetAxis p_offset_axis, TileSet::TileLayout p_from_layout,
	TileSet::TileLayout p_to_layout)
{
	// Transform to stacked layout.
	Vector2i output = p_coords;
	if (p_offset_axis == TileSet::TILE_OFFSET_AXIS_VERTICAL) {
		SWAP(output.x, output.y);
	}
	switch (p_from_layout) {
	case TileSet::TILE_LAYOUT_STACKED:
		break;
	case TileSet::TILE_LAYOUT_STACKED_OFFSET:
		if (output.y % 2) {
			output.x -= 1;
		}
		break;
	case TileSet::TILE_LAYOUT_STAIRS_RIGHT:
	case TileSet::TILE_LAYOUT_STAIRS_DOWN:
		if ((p_from_layout == TileSet::TILE_LAYOUT_STAIRS_RIGHT) ^
			(p_offset_axis == TileSet::TILE_OFFSET_AXIS_VERTICAL)) {
			if (output.y < 0 && bool(output.y % 2)) {
				output = Vector2i(output.x + output.y / 2 - 1, output.y);
			}
			else {
				output = Vector2i(output.x + output.y / 2, output.y);
			}
		}
		else {
			if (output.x < 0 && bool(output.x % 2)) {
				output = Vector2i(output.x / 2 - 1, output.x + output.y * 2);
			}
			else {
				output = Vector2i(output.x / 2, output.x + output.y * 2);
			}
		}
		break;
	case TileSet::TILE_LAYOUT_DIAMOND_RIGHT:
	case TileSet::TILE_LAYOUT_DIAMOND_DOWN:
		if ((p_from_layout == TileSet::TILE_LAYOUT_DIAMOND_RIGHT) ^
			(p_offset_axis == TileSet::TILE_OFFSET_AXIS_VERTICAL)) {
			if ((output.x + output.y) < 0 && (output.x - output.y) % 2) {
				output = Vector2i((output.x + output.y) / 2 - 1, output.y - output.x);
			}
			else {
				output = Vector2i((output.x + output.y) / 2, -output.x + output.y);
			}
		}
		else {
			if ((output.x - output.y) < 0 && (output.x + output.y) % 2) {
				output = Vector2i((output.x - output.y) / 2 - 1, output.x + output.y);
			}
			else {
				output = Vector2i((output.x - output.y) / 2, output.x + output.y);
			}
		}
		break;
	}

	switch (p_to_layout) {
	case TileSet::TILE_LAYOUT_STACKED:
		break;
	case TileSet::TILE_LAYOUT_STACKED_OFFSET:
		if (output.y % 2) {
			output.x += 1;
		}
		break;
	case TileSet::TILE_LAYOUT_STAIRS_RIGHT:
	case TileSet::TILE_LAYOUT_STAIRS_DOWN:
		if ((p_to_layout == TileSet::TILE_LAYOUT_STAIRS_RIGHT) ^
			(p_offset_axis == TileSet::TILE_OFFSET_AXIS_VERTICAL)) {
			if (output.y < 0 && (output.y % 2)) {
				output = Vector2i(output.x - output.y / 2 + 1, output.y);
			}
			else {
				output = Vector2i(output.x - output.y / 2, output.y);
			}
		}
		else {
			if (output.y % 2) {
				if (output.y < 0) {
					output = Vector2i(2 * output.x + 1, -output.x + output.y / 2 - 1);
				}
				else {
					output = Vector2i(2 * output.x + 1, -output.x + output.y / 2);
				}
			}
			else {
				output = Vector2i(2 * output.x, -output.x + output.y / 2);
			}
		}
		break;
	case TileSet::TILE_LAYOUT_DIAMOND_RIGHT:
	case TileSet::TILE_LAYOUT_DIAMOND_DOWN:
		if ((p_to_layout == TileSet::TILE_LAYOUT_DIAMOND_RIGHT) ^
			(p_offset_axis == TileSet::TILE_OFFSET_AXIS_VERTICAL)) {
			if (output.y % 2) {
				if (output.y > 0) {
					output = Vector2i(output.x - output.y / 2, output.x + output.y / 2 + 1);
				}
				else {
					output = Vector2i(output.x - output.y / 2 + 1, output.x + output.y / 2);
				}
			}
			else {
				output = Vector2i(output.x - output.y / 2, output.x + output.y / 2);
			}
		}
		else {
			if (output.y % 2) {
				if (output.y < 0) {
					output = Vector2i(output.x + output.y / 2, -output.x + output.y / 2 - 1);
				}
				else {
					output = Vector2i(output.x + output.y / 2 + 1, -output.x + output.y / 2);
				}
			}
			else {
				output = Vector2i(output.x + output.y / 2, -output.x + output.y / 2);
			}
		}
		break;
	}

	if (p_offset_axis == TileSet::TILE_OFFSET_AXIS_VERTICAL) {
		SWAP(output.x, output.y);
	}

	return output;
}

const Vector2i TileSetSource::INVALID_ATLAS_COORDS = Vector2i(-1, -1);
const int TileSetSource::INVALID_TILE_ALTERNATIVE = -1;

void TileSet::reset_state() {}

TileSet::TileSet()
{
	// Instantiate the tile meshes.
	tile_lines_mesh.instantiate();
	tile_filled_mesh.instantiate();
}

TileSet::~TileSet()
{
#ifndef DISABLE_DEPRECATED
	for (const KeyValue<int, CompatibilityTileData*>& E : compatibility_data) {
		memdelete(E.value);
	}
#endif // DISABLE_DEPRECATED
	while (!source_ids.is_empty()) {
		remove_source(source_ids[0]);
	}
}

void TileSetSource::set_tile_set(const TileSet* p_tile_set) { tile_set = p_tile_set; }

TileSet* TileSetSource::get_tile_set() const { return (TileSet*)tile_set; }

void TileSetSource::reset_state() { tile_set = nullptr; }

void TileSetAtlasSource::set_tile_set(const TileSet* p_tile_set)
{
	tile_set = p_tile_set;

	// Set the TileSet on all TileData.
	for (KeyValue<Vector2i, TileAlternativesData>& E_tile : tiles) {
		for (KeyValue<int, TileData*>& E_alternative : E_tile.value.alternatives) {
			E_alternative.value->set_tile_set(tile_set);
		}
	}
}

const TileSet* TileSetAtlasSource::get_tile_set() const { return tile_set; }

void TileSetAtlasSource::notify_tile_data_properties_should_change()
{
	// Set the TileSet on all TileData.
	for (KeyValue<Vector2i, TileAlternativesData>& E_tile : tiles) {
		for (KeyValue<int, TileData*>& E_alternative : E_tile.value.alternatives) {
			E_alternative.value->notify_tile_data_properties_should_change();
		}
	}
}

void TileSetAtlasSource::add_occlusion_layer(int p_to_pos)
{
	for (KeyValue<Vector2i, TileAlternativesData> E_tile : tiles) {
		for (KeyValue<int, TileData*> E_alternative : E_tile.value.alternatives) {
			E_alternative.value->add_occlusion_layer(p_to_pos);
		}
	}
}

void TileSetAtlasSource::move_occlusion_layer(int p_from_index, int p_to_pos)
{
	for (KeyValue<Vector2i, TileAlternativesData> E_tile : tiles) {
		for (KeyValue<int, TileData*> E_alternative : E_tile.value.alternatives) {
			E_alternative.value->move_occlusion_layer(p_from_index, p_to_pos);
		}
	}
}

void TileSetAtlasSource::remove_occlusion_layer(int p_index)
{
	for (KeyValue<Vector2i, TileAlternativesData> E_tile : tiles) {
		for (KeyValue<int, TileData*> E_alternative : E_tile.value.alternatives) {
			E_alternative.value->remove_occlusion_layer(p_index);
		}
	}
}

#ifndef PHYSICS_2D_DISABLED
void TileSetAtlasSource::add_physics_layer(int p_to_pos)
{
	for (KeyValue<Vector2i, TileAlternativesData> E_tile : tiles) {
		for (KeyValue<int, TileData*> E_alternative : E_tile.value.alternatives) {
			E_alternative.value->add_physics_layer(p_to_pos);
		}
	}
}

void TileSetAtlasSource::move_physics_layer(int p_from_index, int p_to_pos)
{
	for (KeyValue<Vector2i, TileAlternativesData> E_tile : tiles) {
		for (KeyValue<int, TileData*> E_alternative : E_tile.value.alternatives) {
			E_alternative.value->move_physics_layer(p_from_index, p_to_pos);
		}
	}
}

void TileSetAtlasSource::remove_physics_layer(int p_index)
{
	for (KeyValue<Vector2i, TileAlternativesData> E_tile : tiles) {
		for (KeyValue<int, TileData*> E_alternative : E_tile.value.alternatives) {
			E_alternative.value->remove_physics_layer(p_index);
		}
	}
}
#endif // PHYSICS_2D_DISABLED

void TileSetAtlasSource::add_terrain_set(int p_to_pos)
{
	for (KeyValue<Vector2i, TileAlternativesData> E_tile : tiles) {
		for (KeyValue<int, TileData*> E_alternative : E_tile.value.alternatives) {
			E_alternative.value->add_terrain_set(p_to_pos);
		}
	}
}

void TileSetAtlasSource::move_terrain_set(int p_from_index, int p_to_pos)
{
	for (KeyValue<Vector2i, TileAlternativesData> E_tile : tiles) {
		for (KeyValue<int, TileData*> E_alternative : E_tile.value.alternatives) {
			E_alternative.value->move_terrain_set(p_from_index, p_to_pos);
		}
	}
}

void TileSetAtlasSource::remove_terrain_set(int p_index)
{
	for (KeyValue<Vector2i, TileAlternativesData> E_tile : tiles) {
		for (KeyValue<int, TileData*> E_alternative : E_tile.value.alternatives) {
			E_alternative.value->remove_terrain_set(p_index);
		}
	}
}

void TileSetAtlasSource::add_terrain(int p_terrain_set, int p_to_pos)
{
	for (KeyValue<Vector2i, TileAlternativesData> E_tile : tiles) {
		for (KeyValue<int, TileData*> E_alternative : E_tile.value.alternatives) {
			E_alternative.value->add_terrain(p_terrain_set, p_to_pos);
		}
	}
}

void TileSetAtlasSource::move_terrain(int p_terrain_set, int p_from_index, int p_to_pos)
{
	for (KeyValue<Vector2i, TileAlternativesData> E_tile : tiles) {
		for (KeyValue<int, TileData*> E_alternative : E_tile.value.alternatives) {
			E_alternative.value->move_terrain(p_terrain_set, p_from_index, p_to_pos);
		}
	}
}

void TileSetAtlasSource::remove_terrain(int p_terrain_set, int p_index)
{
	for (KeyValue<Vector2i, TileAlternativesData> E_tile : tiles) {
		for (KeyValue<int, TileData*> E_alternative : E_tile.value.alternatives) {
			E_alternative.value->remove_terrain(p_terrain_set, p_index);
		}
	}
}

#ifndef NAVIGATION_2D_DISABLED
void TileSetAtlasSource::add_navigation_layer(int p_to_pos)
{
	for (KeyValue<Vector2i, TileAlternativesData> E_tile : tiles) {
		for (KeyValue<int, TileData*> E_alternative : E_tile.value.alternatives) {
			E_alternative.value->add_navigation_layer(p_to_pos);
		}
	}
}

void TileSetAtlasSource::move_navigation_layer(int p_from_index, int p_to_pos)
{
	for (KeyValue<Vector2i, TileAlternativesData> E_tile : tiles) {
		for (KeyValue<int, TileData*> E_alternative : E_tile.value.alternatives) {
			E_alternative.value->move_navigation_layer(p_from_index, p_to_pos);
		}
	}
}

void TileSetAtlasSource::remove_navigation_layer(int p_index)
{
	for (KeyValue<Vector2i, TileAlternativesData> E_tile : tiles) {
		for (KeyValue<int, TileData*> E_alternative : E_tile.value.alternatives) {
			E_alternative.value->remove_navigation_layer(p_index);
		}
	}
}
#endif // NAVIGATION_2D_DISABLED

void TileSetAtlasSource::add_custom_data_layer(int p_to_pos)
{
	for (KeyValue<Vector2i, TileAlternativesData> E_tile : tiles) {
		for (KeyValue<int, TileData*> E_alternative : E_tile.value.alternatives) {
			E_alternative.value->add_custom_data_layer(p_to_pos);
		}
	}
}

void TileSetAtlasSource::move_custom_data_layer(int p_from_index, int p_to_pos)
{
	for (KeyValue<Vector2i, TileAlternativesData> E_tile : tiles) {
		for (KeyValue<int, TileData*> E_alternative : E_tile.value.alternatives) {
			E_alternative.value->move_custom_data_layer(p_from_index, p_to_pos);
		}
	}
}

void TileSetAtlasSource::remove_custom_data_layer(int p_index)
{
	for (KeyValue<Vector2i, TileAlternativesData> E_tile : tiles) {
		for (KeyValue<int, TileData*> E_alternative : E_tile.value.alternatives) {
			E_alternative.value->remove_custom_data_layer(p_index);
		}
	}
}

void TileSetAtlasSource::reset_state()
{
	tile_set = nullptr;

	for (KeyValue<Vector2i, TileAlternativesData>& E_tile : tiles) {
		for (const KeyValue<int, TileData*>& E_tile_data : E_tile.value.alternatives) {
			memdelete(E_tile_data.value);
		}
	}
	_coords_mapping_cache.clear();
	tiles.clear();
	tiles_ids.clear();
	_queue_update_padded_texture();
}

Ref<Texture2D> TileSetAtlasSource::get_texture() const { return texture; }

void TileSetAtlasSource::set_margins(Vector2i p_margins)
{
	if (p_margins.x < 0 || p_margins.y < 0) {
		WARN_PRINT("Atlas source margins should be positive.");
		margins = p_margins.maxi(0);
	}
	else {
		margins = p_margins;
	}

	_queue_update_padded_texture();
	emit_changed();
}

Vector2i TileSetAtlasSource::get_margins() const { return margins; }

void TileSetAtlasSource::set_separation(Vector2i p_separation)
{
	if (p_separation.x < 0 || p_separation.y < 0) {
		WARN_PRINT("Atlas source separation should be positive.");
		separation = p_separation.maxi(0);
	}
	else {
		separation = p_separation;
	}

	_queue_update_padded_texture();
	emit_changed();
}

Vector2i TileSetAtlasSource::get_separation() const { return separation; }

void TileSetAtlasSource::set_texture_region_size(Vector2i p_tile_size)
{
	if (p_tile_size.x <= 0 || p_tile_size.y <= 0) {
		WARN_PRINT("Atlas source tile_size should be strictly positive.");
		texture_region_size = p_tile_size.maxi(1);
	}
	else {
		texture_region_size = p_tile_size;
	}

	_queue_update_padded_texture();
	emit_changed();
}

Vector2i TileSetAtlasSource::get_texture_region_size() const { return texture_region_size; }

void TileSetAtlasSource::set_use_texture_padding(bool p_use_padding)
{
	if (use_texture_padding == p_use_padding) {
		return;
	}
	use_texture_padding = p_use_padding;
	_queue_update_padded_texture();
	emit_changed();
}

bool TileSetAtlasSource::get_use_texture_padding() const { return use_texture_padding; }

Vector2i TileSetAtlasSource::get_atlas_grid_size() const
{
	Ref<Texture2D> txt = get_texture();
	if (txt.is_null()) {
		return Vector2i();
	}

	ERR_FAIL_COND_V(texture_region_size.x <= 0 || texture_region_size.y <= 0, Vector2i());

	Size2i valid_area = txt->get_size() - margins;

	// Compute the number of valid tiles in the tiles atlas
	Size2i grid_size;
	if (valid_area.x >= texture_region_size.x && valid_area.y >= texture_region_size.y) {
		valid_area -= texture_region_size;
		grid_size = Size2i(1, 1) + valid_area / (texture_region_size + separation);
	}
	return grid_size;
}

void TileSetAtlasSource::remove_tile(Vector2i p_atlas_coords)
{
	ERR_FAIL_COND_MSG(!tiles.has(p_atlas_coords),
		vformat("TileSetAtlasSource has no tile at %s.", String(p_atlas_coords)));

	// Remove all covered positions from the mapping cache
	_clear_coords_mapping_cache(p_atlas_coords);

	// Free tile data.
	for (const KeyValue<int, TileData*>& E_tile_data : tiles[p_atlas_coords].alternatives) {
		memdelete(E_tile_data.value);
	}

	// Delete the tile
	tiles.erase(p_atlas_coords);
	tiles_ids.erase(p_atlas_coords);
	tiles_ids.sort();

	_queue_update_padded_texture();

	_try_emit_changed();
}

bool TileSetAtlasSource::has_tile(Vector2i p_atlas_coords) const
{
	return tiles.has(p_atlas_coords);
}

Vector2i TileSetAtlasSource::get_tile_at_coords(Vector2i p_atlas_coords) const
{
	if (!_coords_mapping_cache.has(p_atlas_coords)) {
		return INVALID_ATLAS_COORDS;
	}

	return _coords_mapping_cache[p_atlas_coords];
}

void TileSetAtlasSource::set_tile_animation_columns(
	const Vector2i p_atlas_coords, int p_frame_columns)
{
	ERR_FAIL_COND_MSG(!tiles.has(p_atlas_coords),
		vformat("TileSetAtlasSource has no tile at %s.", Vector2i(p_atlas_coords)));
	ERR_FAIL_COND(p_frame_columns < 0);

	TileAlternativesData& tad = tiles[p_atlas_coords];
	bool room_for_tile = has_room_for_tile(p_atlas_coords, tad.size_in_atlas, p_frame_columns,
		tad.animation_separation, tad.animation_frames_durations.size(), p_atlas_coords);
	ERR_FAIL_COND_MSG(!room_for_tile, "Cannot set animation columns count, tiles are already "
									  "present in the space the tile would cover.");

	_clear_coords_mapping_cache(p_atlas_coords);

	tiles[p_atlas_coords].animation_columns = p_frame_columns;

	_create_coords_mapping_cache(p_atlas_coords);
	_queue_update_padded_texture();

	_try_emit_changed();
}

int TileSetAtlasSource::get_tile_animation_columns(const Vector2i p_atlas_coords) const
{
	ERR_FAIL_COND_V_MSG(!tiles.has(p_atlas_coords), 1,
		vformat("TileSetAtlasSource has no tile at %s.", Vector2i(p_atlas_coords)));
	return tiles[p_atlas_coords].animation_columns;
}

void TileSetAtlasSource::set_tile_animation_separation(
	const Vector2i p_atlas_coords, const Vector2i p_separation)
{
	ERR_FAIL_COND_MSG(!tiles.has(p_atlas_coords),
		vformat("TileSetAtlasSource has no tile at %s.", Vector2i(p_atlas_coords)));
	ERR_FAIL_COND(p_separation.x < 0 || p_separation.y < 0);

	TileAlternativesData& tad = tiles[p_atlas_coords];
	bool room_for_tile = has_room_for_tile(p_atlas_coords, tad.size_in_atlas, tad.animation_columns,
		p_separation, tad.animation_frames_durations.size(), p_atlas_coords);
	ERR_FAIL_COND_MSG(!room_for_tile, "Cannot set animation columns count, tiles are already "
									  "present in the space the tile would cover.");

	_clear_coords_mapping_cache(p_atlas_coords);

	tiles[p_atlas_coords].animation_separation = p_separation;

	_create_coords_mapping_cache(p_atlas_coords);
	_queue_update_padded_texture();

	_try_emit_changed();
}

Vector2i TileSetAtlasSource::get_tile_animation_separation(const Vector2i p_atlas_coords) const
{
	ERR_FAIL_COND_V_MSG(!tiles.has(p_atlas_coords), Vector2i(),
		vformat("TileSetAtlasSource has no tile at %s.", Vector2i(p_atlas_coords)));
	return tiles[p_atlas_coords].animation_separation;
}

void TileSetAtlasSource::set_tile_animation_speed(const Vector2i p_atlas_coords, real_t p_speed)
{
	ERR_FAIL_COND_MSG(!tiles.has(p_atlas_coords),
		vformat("TileSetAtlasSource has no tile at %s.", Vector2i(p_atlas_coords)));
	ERR_FAIL_COND(p_speed <= 0);

	tiles[p_atlas_coords].animation_speed = p_speed;

	_try_emit_changed();
}

real_t TileSetAtlasSource::get_tile_animation_speed(const Vector2i p_atlas_coords) const
{
	ERR_FAIL_COND_V_MSG(!tiles.has(p_atlas_coords), 1.0,
		vformat("TileSetAtlasSource has no tile at %s.", Vector2i(p_atlas_coords)));
	return tiles[p_atlas_coords].animation_speed;
}

void TileSetAtlasSource::set_tile_animation_mode(
	const Vector2i p_atlas_coords, TileSetAtlasSource::TileAnimationMode p_mode)
{
	ERR_FAIL_COND_MSG(!tiles.has(p_atlas_coords),
		vformat("TileSetAtlasSource has no tile at %s.", Vector2i(p_atlas_coords)));

	tiles[p_atlas_coords].animation_mode = p_mode;

	_try_emit_changed();
}

TileSetAtlasSource::TileAnimationMode TileSetAtlasSource::get_tile_animation_mode(
	const Vector2i p_atlas_coords) const
{
	ERR_FAIL_COND_V_MSG(!tiles.has(p_atlas_coords), TILE_ANIMATION_MODE_DEFAULT,
		vformat("TileSetAtlasSource has no tile at %s.", Vector2i(p_atlas_coords)));

	return tiles[p_atlas_coords].animation_mode;
}

int TileSetAtlasSource::get_tile_animation_frames_count(const Vector2i p_atlas_coords) const
{
	ERR_FAIL_COND_V_MSG(!tiles.has(p_atlas_coords), 1,
		vformat("TileSetAtlasSource has no tile at %s.", Vector2i(p_atlas_coords)));
	return tiles[p_atlas_coords].animation_frames_durations.size();
}

void TileSetAtlasSource::set_tile_animation_frame_duration(
	const Vector2i p_atlas_coords, int p_frame_index, real_t p_duration)
{
	ERR_FAIL_COND_MSG(!tiles.has(p_atlas_coords),
		vformat("TileSetAtlasSource has no tile at %s.", Vector2i(p_atlas_coords)));
	ERR_FAIL_INDEX(p_frame_index, (int)tiles[p_atlas_coords].animation_frames_durations.size());
	ERR_FAIL_COND(p_duration <= 0.0);

	tiles[p_atlas_coords].animation_frames_durations[p_frame_index] = p_duration;

	_try_emit_changed();
}

real_t TileSetAtlasSource::get_tile_animation_frame_duration(
	const Vector2i p_atlas_coords, int p_frame_index) const
{
	ERR_FAIL_COND_V_MSG(!tiles.has(p_atlas_coords), 1,
		vformat("TileSetAtlasSource has no tile at %s.", Vector2i(p_atlas_coords)));
	ERR_FAIL_INDEX_V(
		p_frame_index, (int)tiles[p_atlas_coords].animation_frames_durations.size(), 0.0);
	return tiles[p_atlas_coords].animation_frames_durations[p_frame_index];
}

real_t TileSetAtlasSource::get_tile_animation_total_duration(const Vector2i p_atlas_coords) const
{
	ERR_FAIL_COND_V_MSG(!tiles.has(p_atlas_coords), 1,
		vformat("TileSetAtlasSource has no tile at %s.", Vector2i(p_atlas_coords)));

	real_t sum = 0.0;
	for (const real_t& duration : tiles[p_atlas_coords].animation_frames_durations) {
		sum += duration;
	}
	return sum;
}

Vector2i TileSetAtlasSource::get_tile_size_in_atlas(Vector2i p_atlas_coords) const
{
	ERR_FAIL_COND_V_MSG(!tiles.has(p_atlas_coords), Vector2i(-1, -1),
		vformat("TileSetAtlasSource has no tile at %s.", String(p_atlas_coords)));

	return tiles[p_atlas_coords].size_in_atlas;
}

int TileSetAtlasSource::get_tiles_count() const { return tiles_ids.size(); }

Vector2i TileSetAtlasSource::get_tile_id(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, tiles_ids.size(), TileSetSource::INVALID_ATLAS_COORDS);
	return tiles_ids[p_index];
}

bool TileSetAtlasSource::has_room_for_tile(Vector2i p_atlas_coords, Vector2i p_size,
	int p_animation_columns, Vector2i p_animation_separation, int p_frames_count,
	Vector2i p_ignored_tile) const
{
	if (p_atlas_coords.x < 0 || p_atlas_coords.y < 0) {
		return false;
	}
	if (p_size.x <= 0 || p_size.y <= 0) {
		return false;
	}
	if (p_frames_count <= 0) {
		return false;
	}
	Size2i atlas_grid_size = get_atlas_grid_size();
	for (int frame = 0; frame < p_frames_count; frame++) {
		Vector2i frame_coords =
			p_atlas_coords + (p_size + p_animation_separation) *
								 ((p_animation_columns > 0) ? Vector2i(frame % p_animation_columns,
																  frame / p_animation_columns)
															: Vector2i(frame, 0));
		for (int x = 0; x < p_size.x; x++) {
			for (int y = 0; y < p_size.y; y++) {
				Vector2i coords = frame_coords + Vector2i(x, y);
				if (_coords_mapping_cache.has(coords) &&
					_coords_mapping_cache[coords] != p_ignored_tile) {
					return false;
				}
				if (coords.x >= atlas_grid_size.x || coords.y >= atlas_grid_size.y) {
					return false;
				}
			}
		}
	}
	return true;
}

bool TileSetAtlasSource::has_tiles_outside_texture() const
{
	for (const KeyValue<Vector2i, TileSetAtlasSource::TileAlternativesData>& E : tiles) {
		if (!has_room_for_tile(E.key, E.value.size_in_atlas, E.value.animation_columns,
				E.value.animation_separation, E.value.animation_frames_durations.size(), E.key)) {
			return true;
		}
	}
	return false;
}

Vector<Vector2i> TileSetAtlasSource::get_tiles_outside_texture() const
{
	Vector<Vector2i> to_return;

	for (const KeyValue<Vector2i, TileSetAtlasSource::TileAlternativesData>& E : tiles) {
		if (!has_room_for_tile(E.key, E.value.size_in_atlas, E.value.animation_columns,
				E.value.animation_separation, E.value.animation_frames_durations.size(), E.key)) {
			to_return.push_back(E.key);
		}
	}
	return to_return;
}

void TileSetAtlasSource::clear_tiles_outside_texture()
{
	LocalVector<Vector2i> to_remove;

	for (const KeyValue<Vector2i, TileSetAtlasSource::TileAlternativesData>& E : tiles) {
		if (!has_room_for_tile(E.key, E.value.size_in_atlas, E.value.animation_columns,
				E.value.animation_separation, E.value.animation_frames_durations.size(), E.key)) {
			to_remove.push_back(E.key);
		}
	}

	for (const Vector2i& v : to_remove) {
		remove_tile(v);
	}
}

PackedVector2Array TileSetAtlasSource::get_tiles_to_be_removed_on_change(Ref<Texture2D> p_texture,
	Vector2i p_margins, Vector2i p_separation, Vector2i p_texture_region_size)
{
	ERR_FAIL_COND_V(p_margins.x < 0 || p_margins.y < 0, PackedVector2Array());
	ERR_FAIL_COND_V(p_separation.x < 0 || p_separation.y < 0, PackedVector2Array());
	ERR_FAIL_COND_V(
		p_texture_region_size.x <= 0 || p_texture_region_size.y <= 0, PackedVector2Array());

	// Compute the new atlas grid size.
	Size2 new_grid_size;
	if (p_texture.is_valid()) {
		Size2i valid_area = p_texture->get_size() - p_margins;

		// Compute the number of valid tiles in the tiles atlas
		if (valid_area.x >= p_texture_region_size.x && valid_area.y >= p_texture_region_size.y) {
			valid_area -= p_texture_region_size;
			new_grid_size = Size2i(1, 1) + valid_area / (p_texture_region_size + p_separation);
		}
	}

	Vector<Vector2> output;
	for (KeyValue<Vector2i, TileAlternativesData>& E : tiles) {
		for (unsigned int frame = 0; frame < E.value.animation_frames_durations.size(); frame++) {
			Vector2i frame_coords =
				E.key + (E.value.size_in_atlas + E.value.animation_separation) *
							((E.value.animation_columns > 0)
									? Vector2i(frame % E.value.animation_columns,
										  frame / E.value.animation_columns)
									: Vector2i(frame, 0));
			frame_coords += E.value.size_in_atlas;
			if (frame_coords.x > new_grid_size.x || frame_coords.y > new_grid_size.y) {
				output.push_back(E.key);
				break;
			}
		}
	}
	return output;
}

Rect2i TileSetAtlasSource::get_tile_texture_region(Vector2i p_atlas_coords, int p_frame) const
{
	ERR_FAIL_COND_V_MSG(!tiles.has(p_atlas_coords), Rect2i(),
		vformat("TileSetAtlasSource has no tile at %s.", String(p_atlas_coords)));
	ERR_FAIL_INDEX_V(
		p_frame, (int)tiles[p_atlas_coords].animation_frames_durations.size(), Rect2i());

	const TileAlternativesData& tad = tiles[p_atlas_coords];

	Vector2i size_in_atlas = tad.size_in_atlas;
	Vector2 region_size =
		texture_region_size * size_in_atlas + separation * (size_in_atlas - Vector2i(1, 1));

	Vector2i frame_coords = p_atlas_coords + (size_in_atlas + tad.animation_separation) *
												 ((tad.animation_columns > 0)
														 ? Vector2i(p_frame % tad.animation_columns,
															   p_frame / tad.animation_columns)
														 : Vector2i(p_frame, 0));
	Vector2 origin = margins + (frame_coords * (texture_region_size + separation));

	return Rect2(origin, region_size);
}

bool TileSetAtlasSource::is_position_in_tile_texture_region(
	const Vector2i p_atlas_coords, int p_alternative_tile, Vector2 p_position) const
{
	Size2 size = get_tile_texture_region(p_atlas_coords).size;
	TileData* tile_data = get_tile_data(p_atlas_coords, p_alternative_tile);
	if (tile_data->get_transpose()) {
		size = Size2(size.y, size.x);
	}
	Rect2 rect = Rect2(-size / 2 - tile_data->get_texture_origin(), size);

	return rect.has_point(p_position);
}

bool TileSetAtlasSource::is_rect_in_tile_texture_region(
	const Vector2i p_atlas_coords, int p_alternative_tile, Rect2 p_rect) const
{
	Size2 size = get_tile_texture_region(p_atlas_coords).size;
	TileData* tile_data = get_tile_data(p_atlas_coords, p_alternative_tile);

	if (tile_data->get_transpose()) {
		size = Size2(size.y, size.x);
	}
	Rect2 rect = Rect2(-size / 2 - tile_data->get_texture_origin(), size);

	return p_rect.intersection(rect) == p_rect;
}

int TileSetAtlasSource::alternative_no_transform(int p_alternative_id)
{
	return p_alternative_id & ~(TRANSFORM_FLIP_H | TRANSFORM_FLIP_V | TRANSFORM_TRANSPOSE);
}

// Getters for texture and tile region (padded or not)
Ref<Texture2D> TileSetAtlasSource::get_runtime_texture() const
{
	if (use_texture_padding) {
		return padded_texture;
	}
	else {
		return texture;
	}
}

Rect2i TileSetAtlasSource::get_runtime_tile_texture_region(
	Vector2i p_atlas_coords, int p_frame) const
{
	ERR_FAIL_COND_V_MSG(!tiles.has(p_atlas_coords), Rect2i(),
		vformat("TileSetAtlasSource has no tile at %s.", String(p_atlas_coords)));
	ERR_FAIL_INDEX_V(
		p_frame, (int)tiles[p_atlas_coords].animation_frames_durations.size(), Rect2i());

	Rect2i src_rect = get_tile_texture_region(p_atlas_coords, p_frame);
	if (use_texture_padding) {
		const TileAlternativesData& tad = tiles[p_atlas_coords];
		Vector2i frame_coords =
			p_atlas_coords +
			(tad.size_in_atlas + tad.animation_separation) *
				((tad.animation_columns > 0)
						? Vector2i(p_frame % tad.animation_columns, p_frame / tad.animation_columns)
						: Vector2i(p_frame, 0));
		Vector2i base_pos = frame_coords * (texture_region_size + Vector2i(2, 2)) + Vector2i(1, 1);

		return Rect2i(base_pos, src_rect.size);
	}
	else {
		return src_rect;
	}
}

void TileSetAtlasSource::move_tile_in_atlas(
	Vector2i p_atlas_coords, Vector2i p_new_atlas_coords, Vector2i p_new_size)
{
	ERR_FAIL_COND_MSG(!tiles.has(p_atlas_coords),
		vformat("TileSetAtlasSource has no tile at %s.", String(p_atlas_coords)));

	TileAlternativesData& tad = tiles[p_atlas_coords];

	// Compute the actual new rect from arguments.
	Vector2i new_atlas_coords =
		(p_new_atlas_coords != INVALID_ATLAS_COORDS) ? p_new_atlas_coords : p_atlas_coords;
	Vector2i new_size = (p_new_size != Vector2i(-1, -1)) ? p_new_size : tad.size_in_atlas;

	if (new_atlas_coords == p_atlas_coords && new_size == tad.size_in_atlas) {
		return;
	}

	bool room_for_tile = has_room_for_tile(new_atlas_coords, new_size, tad.animation_columns,
		tad.animation_separation, tad.animation_frames_durations.size(), p_atlas_coords);
	ERR_FAIL_COND_MSG(!room_for_tile,
		vformat("Cannot move tile at position %s with size %s. Tile already present.",
			new_atlas_coords, new_size));

	_clear_coords_mapping_cache(p_atlas_coords);

	// Move the tile and update its size.
	if (new_atlas_coords != p_atlas_coords) {
		tiles[new_atlas_coords] = tiles[p_atlas_coords];
		tiles.erase(p_atlas_coords);

		tiles_ids.erase(p_atlas_coords);
		tiles_ids.push_back(new_atlas_coords);
		tiles_ids.sort();
	}
	tiles[new_atlas_coords].size_in_atlas = new_size;

	_create_coords_mapping_cache(new_atlas_coords);
	_queue_update_padded_texture();

	_try_emit_changed();
}

void TileSetAtlasSource::remove_alternative_tile(
	const Vector2i p_atlas_coords, int p_alternative_tile)
{
	ERR_FAIL_COND_MSG(!tiles.has(p_atlas_coords),
		vformat("TileSetAtlasSource has no tile at %s.", String(p_atlas_coords)));
	ERR_FAIL_COND_MSG(!tiles[p_atlas_coords].alternatives.has(p_alternative_tile),
		vformat("TileSetAtlasSource has no alternative with id %d for tile coords %s.",
			p_alternative_tile, String(p_atlas_coords)));
	p_alternative_tile = alternative_no_transform(p_alternative_tile);
	ERR_FAIL_COND_MSG(p_alternative_tile == 0,
		"Cannot remove the alternative with id 0, the base tile alternative cannot be removed.");

	memdelete(tiles[p_atlas_coords].alternatives[p_alternative_tile]);
	tiles[p_atlas_coords].alternatives.erase(p_alternative_tile);
	tiles[p_atlas_coords].alternatives_ids.erase(p_alternative_tile);
	tiles[p_atlas_coords].alternatives_ids.sort();

	_try_emit_changed();
}

void TileSetAtlasSource::set_alternative_tile_id(
	const Vector2i p_atlas_coords, int p_alternative_tile, int p_new_id)
{
	ERR_FAIL_COND_MSG(!tiles.has(p_atlas_coords),
		vformat("TileSetAtlasSource has no tile at %s.", String(p_atlas_coords)));
	ERR_FAIL_COND_MSG(!tiles[p_atlas_coords].alternatives.has(p_alternative_tile),
		vformat("TileSetAtlasSource has no alternative with id %d for tile coords %s.",
			p_alternative_tile, String(p_atlas_coords)));
	p_alternative_tile = alternative_no_transform(p_alternative_tile);
	ERR_FAIL_COND_MSG(p_alternative_tile == 0,
		"Cannot change the alternative with id 0, the base tile alternative cannot be modified.");

	ERR_FAIL_COND_MSG(tiles[p_atlas_coords].alternatives.has(p_new_id),
		vformat("TileSetAtlasSource has already an alternative with id %d at %s.", p_new_id,
			String(p_atlas_coords)));

	tiles[p_atlas_coords].alternatives[p_new_id] =
		tiles[p_atlas_coords].alternatives[p_alternative_tile];
	tiles[p_atlas_coords].alternatives_ids.push_back(p_new_id);

	tiles[p_atlas_coords].alternatives.erase(p_alternative_tile);
	tiles[p_atlas_coords].alternatives_ids.erase(p_alternative_tile);
	tiles[p_atlas_coords].alternatives_ids.sort();

	_try_emit_changed();
}

bool TileSetAtlasSource::has_alternative_tile(
	const Vector2i p_atlas_coords, int p_alternative_tile) const
{
	ERR_FAIL_COND_V_MSG(!tiles.has(p_atlas_coords), false,
		vformat("The TileSetAtlasSource atlas has no tile at %s.", String(p_atlas_coords)));
	return tiles[p_atlas_coords].alternatives.has(alternative_no_transform(p_alternative_tile));
}

int TileSetAtlasSource::get_next_alternative_tile_id(const Vector2i p_atlas_coords) const
{
	ERR_FAIL_COND_V_MSG(!tiles.has(p_atlas_coords), TileSetSource::INVALID_TILE_ALTERNATIVE,
		vformat("The TileSetAtlasSource atlas has no tile at %s.", String(p_atlas_coords)));
	return tiles[p_atlas_coords].next_alternative_id;
}

int TileSetAtlasSource::get_alternative_tiles_count(const Vector2i p_atlas_coords) const
{
	ERR_FAIL_COND_V_MSG(!tiles.has(p_atlas_coords), -1,
		vformat("The TileSetAtlasSource atlas has no tile at %s.", String(p_atlas_coords)));
	return tiles[p_atlas_coords].alternatives_ids.size();
}

int TileSetAtlasSource::get_alternative_tile_id(const Vector2i p_atlas_coords, int p_index) const
{
	ERR_FAIL_COND_V_MSG(!tiles.has(p_atlas_coords), TileSetSource::INVALID_TILE_ALTERNATIVE,
		vformat("The TileSetAtlasSource atlas has no tile at %s.", String(p_atlas_coords)));
	p_index = alternative_no_transform(p_index);
	ERR_FAIL_INDEX_V(p_index, tiles[p_atlas_coords].alternatives_ids.size(),
		TileSetSource::INVALID_TILE_ALTERNATIVE);

	return tiles[p_atlas_coords].alternatives_ids[p_index];
}

TileData* TileSetAtlasSource::get_tile_data(
	const Vector2i p_atlas_coords, int p_alternative_tile) const
{
	ERR_FAIL_COND_V_MSG(!tiles.has(p_atlas_coords), nullptr,
		vformat("The TileSetAtlasSource atlas has no tile at %s.", String(p_atlas_coords)));
	p_alternative_tile = alternative_no_transform(p_alternative_tile);
	ERR_FAIL_COND_V_MSG(!tiles[p_atlas_coords].alternatives.has(p_alternative_tile), nullptr,
		vformat("TileSetAtlasSource has no alternative with id %d for tile coords %s.",
			p_alternative_tile, String(p_atlas_coords)));

	return tiles[p_atlas_coords].alternatives[p_alternative_tile];
}

TileSetAtlasSource::~TileSetAtlasSource()
{
	// Free everything needed.
	for (KeyValue<Vector2i, TileAlternativesData>& E_alternatives : tiles) {
		for (KeyValue<int, TileData*>& E_tile_data : E_alternatives.value.alternatives) {
			memdelete(E_tile_data.value);
		}
	}
}

TileData* TileSetAtlasSource::_get_atlas_tile_data(Vector2i p_atlas_coords, int p_alternative_tile)
{
	ERR_FAIL_COND_V_MSG(!tiles.has(p_atlas_coords), nullptr,
		vformat("TileSetAtlasSource has no tile at %s.", String(p_atlas_coords)));
	p_alternative_tile = alternative_no_transform(p_alternative_tile);
	ERR_FAIL_COND_V_MSG(!tiles[p_atlas_coords].alternatives.has(p_alternative_tile), nullptr,
		vformat("TileSetAtlasSource has no alternative with id %d for tile coords %s.",
			p_alternative_tile, String(p_atlas_coords)));

	return tiles[p_atlas_coords].alternatives[p_alternative_tile];
}

const TileData* TileSetAtlasSource::_get_atlas_tile_data(
	Vector2i p_atlas_coords, int p_alternative_tile) const
{
	ERR_FAIL_COND_V_MSG(!tiles.has(p_atlas_coords), nullptr,
		vformat("TileSetAtlasSource has no tile at %s.", String(p_atlas_coords)));
	ERR_FAIL_COND_V_MSG(!tiles[p_atlas_coords].alternatives.has(p_alternative_tile), nullptr,
		vformat("TileSetAtlasSource has no alternative with id %d for tile coords %s.",
			p_alternative_tile, String(p_atlas_coords)));

	return tiles[p_atlas_coords].alternatives[p_alternative_tile];
}

void TileSetAtlasSource::_compute_next_alternative_id(const Vector2i p_atlas_coords)
{
	ERR_FAIL_COND_MSG(!tiles.has(p_atlas_coords),
		vformat("TileSetAtlasSource has no tile at %s.", String(p_atlas_coords)));

	while (tiles[p_atlas_coords].alternatives.has(tiles[p_atlas_coords].next_alternative_id)) {
		tiles[p_atlas_coords].next_alternative_id =
			(tiles[p_atlas_coords].next_alternative_id % 1073741823) + 1; // 2 ** 30
	};
}

void TileSetAtlasSource::_clear_coords_mapping_cache(Vector2i p_atlas_coords)
{
	ERR_FAIL_COND_MSG(!tiles.has(p_atlas_coords),
		vformat("TileSetAtlasSource has no tile at %s.", Vector2i(p_atlas_coords)));
	TileAlternativesData& tad = tiles[p_atlas_coords];
	for (int frame = 0; frame < (int)tad.animation_frames_durations.size(); frame++) {
		Vector2i frame_coords =
			p_atlas_coords +
			(tad.size_in_atlas + tad.animation_separation) *
				((tad.animation_columns > 0)
						? Vector2i(frame % tad.animation_columns, frame / tad.animation_columns)
						: Vector2i(frame, 0));
		for (int x = 0; x < tad.size_in_atlas.x; x++) {
			for (int y = 0; y < tad.size_in_atlas.y; y++) {
				Vector2i coords = frame_coords + Vector2i(x, y);
				if (!_coords_mapping_cache.has(coords)) {
					WARN_PRINT(vformat("TileSetAtlasSource has no cached tile at position %s, the "
									   "position cache might be corrupted.",
						coords));
				}
				else {
					if (_coords_mapping_cache[coords] != p_atlas_coords) {
						WARN_PRINT(vformat("The position cache at position %s is pointing to a "
										   "wrong tile, the position cache might be corrupted.",
							coords));
					}
					_coords_mapping_cache.erase(coords);
				}
			}
		}
	}
}

void TileSetAtlasSource::_create_coords_mapping_cache(Vector2i p_atlas_coords)
{
	ERR_FAIL_COND_MSG(!tiles.has(p_atlas_coords),
		vformat("TileSetAtlasSource has no tile at %s.", Vector2i(p_atlas_coords)));

	TileAlternativesData& tad = tiles[p_atlas_coords];
	for (int frame = 0; frame < (int)tad.animation_frames_durations.size(); frame++) {
		Vector2i frame_coords =
			p_atlas_coords +
			(tad.size_in_atlas + tad.animation_separation) *
				((tad.animation_columns > 0)
						? Vector2i(frame % tad.animation_columns, frame / tad.animation_columns)
						: Vector2i(frame, 0));
		for (int x = 0; x < tad.size_in_atlas.x; x++) {
			for (int y = 0; y < tad.size_in_atlas.y; y++) {
				Vector2i coords = frame_coords + Vector2i(x, y);
				if (_coords_mapping_cache.has(coords)) {
					WARN_PRINT(vformat("The cache already has a tile for position %s, the position "
									   "cache might be corrupted.",
						coords));
				}
				_coords_mapping_cache[coords] = p_atlas_coords;
			}
		}
	}
}

void TileSetAtlasSource::_try_emit_changed()
{
	if (!initializing) {
		emit_changed();
	}
}

void TileSetScenesCollectionSource::_compute_next_alternative_id()
{
	while (scenes.has(next_scene_id)) {
		next_scene_id = (next_scene_id % 1073741823) + 1; // 2 ** 30
	};
}

void TileSetScenesCollectionSource::_try_emit_changed()
{
	if (!initializing) {
		emit_changed();
	}
}

int TileSetScenesCollectionSource::get_tiles_count() const { return 1; }

Vector2i TileSetScenesCollectionSource::get_tile_id(int p_tile_index) const
{
	ERR_FAIL_COND_V(p_tile_index != 0, TileSetSource::INVALID_ATLAS_COORDS);
	return Vector2i();
}

bool TileSetScenesCollectionSource::has_tile(Vector2i p_atlas_coords) const
{
	return p_atlas_coords == Vector2i();
}

int TileSetScenesCollectionSource::get_alternative_tiles_count(const Vector2i p_atlas_coords) const
{
	return scenes_ids.size();
}

int TileSetScenesCollectionSource::get_alternative_tile_id(
	const Vector2i p_atlas_coords, int p_index) const
{
	ERR_FAIL_COND_V(p_atlas_coords != Vector2i(), TileSetSource::INVALID_TILE_ALTERNATIVE);
	ERR_FAIL_INDEX_V(p_index, scenes_ids.size(), TileSetSource::INVALID_TILE_ALTERNATIVE);

	return scenes_ids[p_index];
}

bool TileSetScenesCollectionSource::has_alternative_tile(
	const Vector2i p_atlas_coords, int p_alternative_tile) const
{
	ERR_FAIL_COND_V(p_atlas_coords != Vector2i(), false);
	return scenes.has(TileSetAtlasSource::alternative_no_transform(p_alternative_tile));
}

int TileSetScenesCollectionSource::create_scene_tile(
	Ref<PackedScene> p_packed_scene, int p_id_override)
{
	ERR_FAIL_COND_V_MSG(p_id_override >= 0 && scenes.has(p_id_override), INVALID_TILE_ALTERNATIVE,
		vformat("Cannot create scene tile. Another scene tile exists with id %d.", p_id_override));

	int new_scene_id = p_id_override >= 0 ? p_id_override : next_scene_id;

	scenes[new_scene_id] = SceneData();
	scenes_ids.push_back(new_scene_id);
	scenes_ids.sort();
	set_scene_tile_scene(new_scene_id, p_packed_scene);
	_compute_next_alternative_id();

	_try_emit_changed();

	return new_scene_id;
}

void TileSetScenesCollectionSource::set_scene_tile_id(int p_id, int p_new_id)
{
	ERR_FAIL_COND(p_new_id < 0);
	ERR_FAIL_COND(!has_scene_tile_id(p_id));
	ERR_FAIL_COND(has_scene_tile_id(p_new_id));

	scenes[p_new_id] = SceneData();
	scenes[p_new_id] = scenes[p_id];
	scenes_ids.push_back(p_new_id);
	scenes_ids.sort();

	_compute_next_alternative_id();

	scenes.erase(p_id);
	scenes_ids.erase(p_id);

	_try_emit_changed();
}

void TileSetScenesCollectionSource::set_scene_tile_scene(int p_id, Ref<PackedScene> p_packed_scene)
{

	ERR_FAIL_COND(!scenes.has(p_id));
	if (p_packed_scene.is_valid()) {
		// Check if it extends CanvasItem.
		Ref<SceneState> scene_state = p_packed_scene->get_state();
		String type;
		while (scene_state.is_valid() && type.is_empty()) {
			// Make sure we have a root node. Supposed to be at 0 index because find_node_by_path()
			// does not seem to work.
			ERR_FAIL_COND(scene_state->get_node_count() < 1);

			type = scene_state->get_node_type(0);
			scene_state = scene_state->get_base_scene_state();
		}
		ERR_FAIL_COND_EDMSG(
			type.is_empty(), vformat("Invalid PackedScene for TileSetScenesCollectionSource: %s. "
									 "Could not get the type of the root node.",
								 p_packed_scene->get_path()));
		scenes[p_id].scene = p_packed_scene;
	}
	else {
		scenes[p_id].scene = Ref<PackedScene>();
	}
	_try_emit_changed();
}

Ref<PackedScene> TileSetScenesCollectionSource::get_scene_tile_scene(int p_id) const
{
	int scene_tile = TileSetAtlasSource::alternative_no_transform(p_id);
	ERR_FAIL_COND_V(!scenes.has(scene_tile), Ref<PackedScene>());
	return scenes[scene_tile].scene;
}

void TileSetScenesCollectionSource::set_scene_tile_display_placeholder(
	int p_id, bool p_display_placeholder)
{
	ERR_FAIL_COND(!scenes.has(p_id));

	scenes[p_id].display_placeholder = p_display_placeholder;

	_try_emit_changed();
}

bool TileSetScenesCollectionSource::get_scene_tile_display_placeholder(int p_id) const
{
	p_id = TileSetAtlasSource::alternative_no_transform(p_id);
	ERR_FAIL_COND_V(!scenes.has(p_id), false);
	return scenes[p_id].display_placeholder;
}

void TileSetScenesCollectionSource::remove_scene_tile(int p_id)
{
	ERR_FAIL_COND(!scenes.has(p_id));

	scenes.erase(p_id);
	scenes_ids.erase(p_id);
	_try_emit_changed();
}

int TileSetScenesCollectionSource::get_next_scene_tile_id() const { return next_scene_id; }

void TileData::set_tile_set(const TileSet* p_tile_set)
{
	tile_set = p_tile_set;
	notify_tile_data_properties_should_change();
}

void TileData::add_occlusion_layer(int p_to_pos)
{
	if (p_to_pos < 0) {
		p_to_pos = occluders.size();
	}
	ERR_FAIL_INDEX(p_to_pos, occluders.size() + 1);
	occluders.insert(p_to_pos, OcclusionLayerTileData());
}

void TileData::move_occlusion_layer(int p_from_index, int p_to_pos)
{
	ERR_FAIL_INDEX(p_from_index, occluders.size());
	ERR_FAIL_INDEX(p_to_pos, occluders.size() + 1);
	occluders.insert(p_to_pos, occluders[p_from_index]);
	occluders.remove_at(p_to_pos < p_from_index ? p_from_index + 1 : p_from_index);
}

void TileData::remove_occlusion_layer(int p_index)
{
	ERR_FAIL_INDEX(p_index, occluders.size());
	occluders.remove_at(p_index);
}

#ifndef PHYSICS_2D_DISABLED
void TileData::add_physics_layer(int p_to_pos)
{
	if (p_to_pos < 0) {
		p_to_pos = physics.size();
	}
	ERR_FAIL_INDEX(p_to_pos, physics.size() + 1);
	physics.insert(p_to_pos, PhysicsLayerTileData());
}

void TileData::move_physics_layer(int p_from_index, int p_to_pos)
{
	ERR_FAIL_INDEX(p_from_index, physics.size());
	ERR_FAIL_INDEX(p_to_pos, physics.size() + 1);
	physics.insert(p_to_pos, physics[p_from_index]);
	physics.remove_at(p_to_pos < p_from_index ? p_from_index + 1 : p_from_index);
}

void TileData::remove_physics_layer(int p_index)
{
	ERR_FAIL_INDEX(p_index, physics.size());
	physics.remove_at(p_index);
}
#endif // PHYSICS_2D_DISABLED

void TileData::add_terrain_set(int p_to_pos)
{
	if (p_to_pos >= 0 && p_to_pos <= terrain_set) {
		terrain_set += 1;
	}
}

void TileData::move_terrain_set(int p_from_index, int p_to_pos)
{
	if (p_from_index == terrain_set) {
		terrain_set = (p_from_index < p_to_pos) ? p_to_pos - 1 : p_to_pos;
	}
	else {
		if (p_from_index < terrain_set) {
			terrain_set -= 1;
		}
		if (p_to_pos <= terrain_set) {
			terrain_set += 1;
		}
	}
}

void TileData::remove_terrain_set(int p_index)
{
	if (p_index == terrain_set) {
		terrain_set = -1;
		for (int i = 0; i < 16; i++) {
			terrain_peering_bits[i] = -1;
		}
	}
	else if (terrain_set > p_index) {
		terrain_set -= 1;
	}
}

void TileData::add_terrain(int p_terrain_set, int p_to_pos)
{
	if (terrain_set == p_terrain_set) {
		for (int i = 0; i < 16; i++) {
			if (p_to_pos >= 0 && p_to_pos <= terrain_peering_bits[i]) {
				terrain_peering_bits[i] += 1;
			}
		}
	}
}

void TileData::move_terrain(int p_terrain_set, int p_from_index, int p_to_pos)
{
	if (terrain_set == p_terrain_set) {
		for (int i = 0; i < 16; i++) {
			if (p_from_index == terrain_peering_bits[i]) {
				terrain_peering_bits[i] = (p_from_index < p_to_pos) ? p_to_pos - 1 : p_to_pos;
			}
			else {
				if (p_from_index < terrain_peering_bits[i]) {
					terrain_peering_bits[i] -= 1;
				}
				if (p_to_pos <= terrain_peering_bits[i]) {
					terrain_peering_bits[i] += 1;
				}
			}
		}
	}
}

void TileData::remove_terrain(int p_terrain_set, int p_index)
{
	if (terrain_set == p_terrain_set) {
		if (terrain == p_index) {
			terrain = -1;
		}
		else if (terrain > p_index) {
			terrain -= 1;
		}

		for (int i = 0; i < 16; i++) {
			if (terrain_peering_bits[i] == p_index) {
				terrain_peering_bits[i] = -1;
			}
			else if (terrain_peering_bits[i] > p_index) {
				terrain_peering_bits[i] -= 1;
			}
		}
	}
}

#ifndef NAVIGATION_2D_DISABLED
void TileData::add_navigation_layer(int p_to_pos)
{
	if (p_to_pos < 0) {
		p_to_pos = navigation.size();
	}
	ERR_FAIL_INDEX(p_to_pos, navigation.size() + 1);
	navigation.insert(p_to_pos, NavigationLayerTileData());
}

void TileData::move_navigation_layer(int p_from_index, int p_to_pos)
{
	ERR_FAIL_INDEX(p_from_index, navigation.size());
	ERR_FAIL_INDEX(p_to_pos, navigation.size() + 1);
	navigation.insert(p_to_pos, navigation[p_from_index]);
	navigation.remove_at(p_to_pos < p_from_index ? p_from_index + 1 : p_from_index);
}

void TileData::remove_navigation_layer(int p_index)
{
	ERR_FAIL_INDEX(p_index, navigation.size());
	navigation.remove_at(p_index);
}
#endif // NAVIGATION_2D_DISABLED

void TileData::set_allow_transform(bool p_allow_transform) { allow_transform = p_allow_transform; }

bool TileData::is_allowing_transform() const { return allow_transform; }

bool TileData::get_flip_h() const { return flip_h; }

bool TileData::get_flip_v() const { return flip_v; }

bool TileData::get_transpose() const { return transpose; }

Vector2i TileData::get_texture_origin() const { return texture_origin; }

Ref<Material> TileData::get_material() const { return material; }

Color TileData::get_modulate() const { return modulate; }

int TileData::get_z_index() const { return z_index; }

int TileData::get_y_sort_origin() const { return y_sort_origin; }

#ifndef DISABLE_DEPRECATED

Ref<OccluderPolygon2D> TileData::get_occluder(
	int p_layer_id, bool p_flip_h, bool p_flip_v, bool p_transpose) const
{
	ERR_FAIL_INDEX_V(p_layer_id, occluders.size(), Ref<OccluderPolygon2D>());
	if (get_occluder_polygons_count(p_layer_id) == 0) {
		return Ref<OccluderPolygon2D>();
	}
	return get_occluder_polygon(p_layer_id, 0, p_flip_h, p_flip_v, p_transpose);
}
#endif // DISABLE_DEPRECATED

int TileData::get_occluder_polygons_count(int p_layer_id) const
{
	ERR_FAIL_INDEX_V(p_layer_id, occluders.size(), 0);
	return occluders[p_layer_id].polygons.size();
}

Ref<OccluderPolygon2D> TileData::get_occluder_polygon(
	int p_layer_id, int p_polygon_index, bool p_flip_h, bool p_flip_v, bool p_transpose) const
{
	ERR_FAIL_INDEX_V(p_layer_id, occluders.size(), Ref<OccluderPolygon2D>());
	ERR_FAIL_INDEX_V(
		p_polygon_index, occluders[p_layer_id].polygons.size(), Ref<OccluderPolygon2D>());

	const OcclusionLayerTileData& layer_tile_data = occluders[p_layer_id];
	const Ref<OccluderPolygon2D>& occluder_polygon =
		layer_tile_data.polygons[p_polygon_index].occluder_polygon;

	int key = int(p_flip_h) | int(p_flip_v) << 1 | int(p_transpose) << 2;
	if (key == 0) {
		return occluder_polygon;
	}

	if (occluder_polygon.is_null()) {
		return Ref<OccluderPolygon2D>();
	}

	HashMap<int, Ref<OccluderPolygon2D>>::Iterator I =
		layer_tile_data.polygons[p_polygon_index].transformed_polygon_occluders.find(key);
	if (!I) {
		Ref<OccluderPolygon2D> transformed_polygon;
		transformed_polygon.instantiate();
		transformed_polygon->set_polygon(get_transformed_vertices(
			occluder_polygon->get_polygon(), p_flip_h, p_flip_v, p_transpose));
		layer_tile_data.polygons[p_polygon_index].transformed_polygon_occluders[key] =
			transformed_polygon;
		return transformed_polygon;
	}
	else {
		return I->value;
	}
}

#ifndef PHYSICS_2D_DISABLED

Vector2 TileData::get_constant_linear_velocity(int p_layer_id) const
{
	ERR_FAIL_INDEX_V(p_layer_id, physics.size(), Vector2());
	return physics[p_layer_id].linear_velocity;
}

real_t TileData::get_constant_angular_velocity(int p_layer_id) const
{
	ERR_FAIL_INDEX_V(p_layer_id, physics.size(), 0.0);
	return physics[p_layer_id].angular_velocity;
}

int TileData::get_collision_polygons_count(int p_layer_id) const
{
	ERR_FAIL_INDEX_V(p_layer_id, physics.size(), 0);
	return physics[p_layer_id].polygons.size();
}

Vector<Vector2> TileData::get_collision_polygon_points(int p_layer_id, int p_polygon_index) const
{
	ERR_FAIL_INDEX_V(p_layer_id, physics.size(), Vector<Vector2>());
	ERR_FAIL_INDEX_V(p_polygon_index, physics[p_layer_id].polygons.size(), Vector<Vector2>());
	return Vector<Vector2>(physics[p_layer_id].polygons[p_polygon_index].polygon);
}

bool TileData::is_collision_polygon_one_way(int p_layer_id, int p_polygon_index) const
{
	ERR_FAIL_INDEX_V(p_layer_id, physics.size(), false);
	ERR_FAIL_INDEX_V(p_polygon_index, physics[p_layer_id].polygons.size(), false);
	return physics[p_layer_id].polygons[p_polygon_index].one_way;
}

float TileData::get_collision_polygon_one_way_margin(int p_layer_id, int p_polygon_index) const
{
	ERR_FAIL_INDEX_V(p_layer_id, physics.size(), 0.0);
	ERR_FAIL_INDEX_V(p_polygon_index, physics[p_layer_id].polygons.size(), 0.0);
	return physics[p_layer_id].polygons[p_polygon_index].one_way_margin;
}

int TileData::get_collision_polygon_shapes_count(int p_layer_id, int p_polygon_index) const
{
	ERR_FAIL_INDEX_V(p_layer_id, physics.size(), 0);
	ERR_FAIL_INDEX_V(p_polygon_index, physics[p_layer_id].polygons.size(), 0);
	return physics[p_layer_id].polygons[p_polygon_index].shapes.size();
}

Ref<Shape2D> TileData::get_collision_polygon_shape(int p_layer_id, int p_polygon_index,
	int shape_index, bool p_flip_h, bool p_flip_v, bool p_transpose) const
{
	ERR_FAIL_INDEX_V(p_layer_id, physics.size(), Ref<Shape2D>());
	ERR_FAIL_INDEX_V(
		p_polygon_index, physics[p_layer_id].polygons.size(), Ref<Shape2D>());
	ERR_FAIL_INDEX_V(shape_index, (int)physics[p_layer_id].polygons[p_polygon_index].shapes.size(),
		Ref<Shape2D>());

	const PhysicsLayerTileData& layer_tile_data = physics[p_layer_id];
	const PhysicsLayerTileData::PolygonShapeTileData& shapes_data =
		layer_tile_data.polygons[p_polygon_index];

	int key = int(p_flip_h) | int(p_flip_v) << 1 | int(p_transpose) << 2;
	if (key == 0) {
		return shapes_data.shapes[shape_index];
	}
	if (shapes_data.shapes[shape_index].is_null()) {
		return Ref<Shape2D>();
	}

	HashMap<int, LocalVector<Ref<Shape2D>>>::Iterator I =

		shapes_data.transformed_shapes.find(key);
	if (!I) {
		int size = shapes_data.shapes.size();
		shapes_data.transformed_shapes[key].resize(size);
		for (int i = 0; i < size; i++) {
			Ref<Shape2D> transformed_polygon = memnew(Shape2D(Shape2D::SHAPE_CONVEX_POLYGON));
			transformed_polygon->set_points(get_transformed_vertices(
				shapes_data.shapes[i]->get_points(), p_flip_h, p_flip_v, p_transpose));
			shapes_data.transformed_shapes[key][i] = transformed_polygon;
		}
		return shapes_data.transformed_shapes[key][shape_index];
	}
	else {
		return I->value[shape_index];
	}
}
#endif // PHYSICS_2D_DISABLED

int TileData::get_terrain_set() const { return terrain_set; }

int TileData::get_terrain() const { return terrain; }

int TileData::get_terrain_peering_bit(TileSet::CellNeighbor p_peering_bit) const
{
	ERR_FAIL_COND_V(!is_valid_terrain_peering_bit(p_peering_bit), -1);
	return terrain_peering_bits[p_peering_bit];
}

bool TileData::is_valid_terrain_peering_bit(TileSet::CellNeighbor p_peering_bit) const
{
	ERR_FAIL_NULL_V(tile_set, false);

	return tile_set->is_valid_terrain_peering_bit(terrain_set, p_peering_bit);
}

TileSet::TerrainsPattern TileData::get_terrains_pattern() const
{
	ERR_FAIL_NULL_V(tile_set, TileSet::TerrainsPattern());

	TileSet::TerrainsPattern output(tile_set, terrain_set);
	output.set_terrain(terrain);
	for (int i = 0; i < TileSet::CELL_NEIGHBOR_MAX; i++) {
		if (tile_set->is_valid_terrain_peering_bit(terrain_set, TileSet::CellNeighbor(i))) {
			output.set_terrain_peering_bit(
				TileSet::CellNeighbor(i), get_terrain_peering_bit(TileSet::CellNeighbor(i)));
		}
	}
	return output;
}

#ifndef NAVIGATION_2D_DISABLED

Ref<NavigationPolygon> TileData::get_navigation_polygon(
	int p_layer_id, bool p_flip_h, bool p_flip_v, bool p_transpose) const
{
	ERR_FAIL_INDEX_V(p_layer_id, navigation.size(), Ref<NavigationPolygon>());

	const NavigationLayerTileData& layer_tile_data = navigation[p_layer_id];

	int key = int(p_flip_h) | int(p_flip_v) << 1 | int(p_transpose) << 2;
	if (key == 0) {
		return layer_tile_data.navigation_polygon;
	}

	if (layer_tile_data.navigation_polygon.is_null()) {
		return Ref<NavigationPolygon>();
	}

	HashMap<int, Ref<NavigationPolygon>>::Iterator I =
		layer_tile_data.transformed_navigation_polygon.find(key);
	if (!I) {
		Ref<NavigationPolygon> transformed_polygon;
		transformed_polygon.instantiate();

		// Winding order:
		// - Preserve for outlines.
		// - If there are no polygons provided, preserve for vertices.
		// - If there are polygons provided, preserve for polygons, don't preserve for vertices (so
		// the vertex order is unchanged and polygons don't need reindexing).

		Vector<Vector<int>> new_polygons = layer_tile_data.navigation_polygon->get_polygons();
		if ((p_flip_h != p_flip_v) != p_transpose) {
			for (Vector<int>& polygon : new_polygons) {
				polygon.reverse();
			}
		}

		PackedVector2Array new_points =
			get_transformed_vertices(layer_tile_data.navigation_polygon->get_vertices(), p_flip_h,
				p_flip_v, p_transpose, new_polygons.is_empty());

		const Vector<Vector<Vector2>> outlines = layer_tile_data.navigation_polygon->get_outlines();
		int outline_count = outlines.size();

		Vector<Vector<Vector2>> new_outlines;
		new_outlines.resize(outline_count);

		for (int i = 0; i < outline_count; i++) {
			new_outlines.write[i] =
				get_transformed_vertices(outlines[i], p_flip_h, p_flip_v, p_transpose, true);
		}

		transformed_polygon->set_data(new_points, new_polygons, new_outlines);

		layer_tile_data.transformed_navigation_polygon[key] = transformed_polygon;
		return transformed_polygon;
	}
	else {
		return I->value;
	}
}
#endif // NAVIGATION_2D_DISABLED

float TileData::get_probability() const { return probability; }

bool TileData::has_custom_data(const String& p_layer_name) const
{
	ERR_FAIL_NULL_V(tile_set, false);
	return tile_set->has_custom_data_layer_by_name(p_layer_name);
}

PackedVector2Array TileData::get_transformed_vertices(const PackedVector2Array& p_vertices,
	bool p_flip_h, bool p_flip_v, bool p_transpose, bool p_preserve_winding_order)
{
	const Vector2* r = p_vertices.ptr();
	int size = p_vertices.size();

	PackedVector2Array new_points;
	new_points.resize_uninitialized(size);
	Vector2* w = new_points.ptrw();

	bool reverse_vertex_order = p_preserve_winding_order && ((p_flip_h != p_flip_v) != p_transpose);
	for (int i = 0; i < size; i++) {
		Vector2 v;
		if (p_transpose) {
			v = Vector2(r[i].y, r[i].x);
		}
		else {
			v = r[i];
		}

		if (p_flip_h) {
			v.x *= -1;
		}
		if (p_flip_v) {
			v.y *= -1;
		}
		w[reverse_vertex_order ? (size - 1 - i) : i] = v;
	}
	return new_points;
}

void TileSetAtlasSource::_queue_update_padded_texture() {}

void TileData::notify_tile_data_properties_should_change() {}

void TileData::remove_custom_data_layer(int) {}

void TileData::move_custom_data_layer(int p_from_index, int p_to_pos) {}

void TileData::add_custom_data_layer(int p_index) {}

void TileSet::remove_source(int p_source_id) {}

// --- tile_set.compat.inc ---

#ifndef DISABLE_DEPRECATED
#ifndef NAVIGATION_2D_DISABLED
Ref<NavigationPolygon> TileData::_get_navigation_polygon_bind_compat_84660(int p_layer_id) const
{
	return get_navigation_polygon(p_layer_id, false, false, false);
}
#endif // NAVIGATION_2D_DISABLED

Ref<OccluderPolygon2D> TileData::_get_occluder_bind_compat_84660(int p_layer_id) const
{
	return get_occluder_polygon(p_layer_id, 0, false, false, false);
}

void TileData::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED

// ============================================================================
// Merged from scene/resources/2d/skeleton/skeleton_modification_2d.cpp
// ============================================================================

#include "scene/resources/2d/skeleton/skeleton_modification_2d.h"

#include "core/config/engine.h"
#include "scene/2d/sprite_2d.h"

// --- Per-mode payload structs ---

struct SkeletonModification2D::LookAtData {
	int bone_idx = -1;
	NodePath bone2d_node;
	float additional_rotation = 0.0f;
	bool enable_constraint = false;
	float constraint_angle_min = 0.0f;
	float constraint_angle_max = float(Math::TAU);
	bool constraint_angle_invert = false;
	bool constraint_in_localspace = true;
};

struct SkeletonModification2D::CCDIKData {
	struct JointData {
		int bone_idx = -1;
		NodePath bone2d_node;
		bool rotate_from_joint = false;
		bool enable_constraint = false;
		float constraint_angle_min = 0.0f;
		float constraint_angle_max = float(Math::TAU);
		bool constraint_angle_invert = false;
		bool constraint_in_localspace = true;
		bool joint_draw_gizmo = true;
	};

	Vector<JointData> chain;
	NodePath tip_node;
};

struct SkeletonModification2D::FABRIKData {
	struct JointData {
		int bone_idx = -1;
		NodePath bone2d_node;
		Vector2 magnet_position;
		bool use_target_rotation = false;
		bool joint_draw_gizmo = true;
	};

	Vector<JointData> chain;
	Vector<Transform2D> transform_chain;
	float chain_tolerance = 0.01f;
	int chain_max_iterations = 10;
	Transform2D target_global_pose;
	Transform2D origin_global_pose;
};

struct SkeletonModification2D::JiggleData {
	struct JointData {
		int bone_idx = -1;
		NodePath bone2d_node;
		bool override_defaults = false;
		float stiffness = 3.0f;
		float mass = 0.75f;
		float damping = 0.75f;
		bool use_gravity = false;
		Vector2 gravity = Vector2(0, 6.0f);
		Vector2 force;
		Vector2 acceleration;
		Vector2 velocity;
		Vector2 last_position;
		Vector2 dynamic_position;
		Vector2 last_noncollision_position;
	};

	Vector<JointData> chain;
	float stiffness = 3.0f;
	float mass = 0.75f;
	float damping = 0.75f;
	bool use_gravity = false;
	Vector2 gravity = Vector2(0, 6.0f);
	bool use_colliders = false;
	uint32_t collision_mask = 1;
};

struct SkeletonModification2D::TwoBoneIKData {
	float target_minimum_distance = 0.0f;
	float target_maximum_distance = 0.0f;
	bool flip_bend_direction = false;
	NodePath joint_one_bone2d_node;
	int joint_one_bone_idx = -1;
	NodePath joint_two_bone2d_node;
	int joint_two_bone_idx = -1;
#ifdef TOOLS_ENABLED
	bool editor_draw_min_max = false;
#endif // TOOLS_ENABLED
};

struct SkeletonModification2D::PhysicalBonesData {
	struct JointData {
		NodePath physical_bone_node;
		int bone_idx = -1;
		bool simulating = false;
	};

	Vector<JointData> chain;
	bool simulation_active = false;
};

struct SkeletonModification2D::StackHolderData {
	Ref<SkeletonModificationStack2D> held_modification_stack;
};

// --- Lifecycle & mode payload management ---

SkeletonModification2D::SkeletonModification2D(Type p_type) {
	stack = nullptr;
	is_setup = false;
	_alloc_mode_data(p_type);
}

SkeletonModification2D::~SkeletonModification2D() {
	_free_mode_data();
}

void SkeletonModification2D::_alloc_mode_data(Type p_type) {
	modification_type = p_type;
	mode_data = nullptr;
	switch (p_type) {
		case MODIFICATION_NONE: {
			editor_draw_gizmo = false;
		} break;
		case MODIFICATION_LOOKAT: {
			mode_data = memnew(LookAtData);
			editor_draw_gizmo = true;
		} break;
		case MODIFICATION_CCDIK: {
			mode_data = memnew(CCDIKData);
			editor_draw_gizmo = true;
		} break;
		case MODIFICATION_FABRIK: {
			mode_data = memnew(FABRIKData);
			editor_draw_gizmo = false;
		} break;
		case MODIFICATION_JIGGLE: {
			mode_data = memnew(JiggleData);
			editor_draw_gizmo = false;
		} break;
		case MODIFICATION_TWOBONEIK: {
			mode_data = memnew(TwoBoneIKData);
			editor_draw_gizmo = true;
		} break;
		case MODIFICATION_PHYSICALBONES: {
			mode_data = memnew(PhysicalBonesData);
			editor_draw_gizmo = false;
		} break;
		case MODIFICATION_STACKHOLDER: {
			mode_data = memnew(StackHolderData);
			editor_draw_gizmo = false;
		} break;
	}
}

void SkeletonModification2D::_free_mode_data() {
	if (!mode_data) {
		modification_type = MODIFICATION_NONE;
		return;
	}
	switch (modification_type) {
		case MODIFICATION_NONE:
			break;
		case MODIFICATION_LOOKAT:
			memdelete(static_cast<LookAtData *>(mode_data));
			break;
		case MODIFICATION_CCDIK:
			memdelete(static_cast<CCDIKData *>(mode_data));
			break;
		case MODIFICATION_FABRIK:
			memdelete(static_cast<FABRIKData *>(mode_data));
			break;
		case MODIFICATION_JIGGLE:
			memdelete(static_cast<JiggleData *>(mode_data));
			break;
		case MODIFICATION_TWOBONEIK:
			memdelete(static_cast<TwoBoneIKData *>(mode_data));
			break;
		case MODIFICATION_PHYSICALBONES:
			memdelete(static_cast<PhysicalBonesData *>(mode_data));
			break;
		case MODIFICATION_STACKHOLDER:
			memdelete(static_cast<StackHolderData *>(mode_data));
			break;
	}
	mode_data = nullptr;
	modification_type = MODIFICATION_NONE;
}

void SkeletonModification2D::set_modification_type(Type p_type) {
	if (modification_type == p_type) {
		return;
	}
	_free_mode_data();
	_alloc_mode_data(p_type);
	if (stack) {
		_setup_modification(stack);
	}
}

void SkeletonModification2D::_ensure_mode(Type p_expected) {
	if (modification_type == MODIFICATION_NONE) {
		set_modification_type(p_expected);
	}
}

SkeletonModification2D::LookAtData *SkeletonModification2D::_get_lookat() {
	return modification_type == MODIFICATION_LOOKAT ? static_cast<LookAtData *>(mode_data) : nullptr;
}

const SkeletonModification2D::LookAtData *SkeletonModification2D::_get_lookat() const {
	return modification_type == MODIFICATION_LOOKAT ? static_cast<const LookAtData *>(mode_data) : nullptr;
}

SkeletonModification2D::CCDIKData *SkeletonModification2D::_get_ccdik() {
	return modification_type == MODIFICATION_CCDIK ? static_cast<CCDIKData *>(mode_data) : nullptr;
}

const SkeletonModification2D::CCDIKData *SkeletonModification2D::_get_ccdik() const {
	return modification_type == MODIFICATION_CCDIK ? static_cast<const CCDIKData *>(mode_data) : nullptr;
}

SkeletonModification2D::FABRIKData *SkeletonModification2D::_get_fabrik() {
	return modification_type == MODIFICATION_FABRIK ? static_cast<FABRIKData *>(mode_data) : nullptr;
}

const SkeletonModification2D::FABRIKData *SkeletonModification2D::_get_fabrik() const {
	return modification_type == MODIFICATION_FABRIK ? static_cast<const FABRIKData *>(mode_data) : nullptr;
}

SkeletonModification2D::JiggleData *SkeletonModification2D::_get_jiggle() {
	return modification_type == MODIFICATION_JIGGLE ? static_cast<JiggleData *>(mode_data) : nullptr;
}

const SkeletonModification2D::JiggleData *SkeletonModification2D::_get_jiggle() const {
	return modification_type == MODIFICATION_JIGGLE ? static_cast<const JiggleData *>(mode_data) : nullptr;
}

SkeletonModification2D::TwoBoneIKData *SkeletonModification2D::_get_twoboneik() {
	return modification_type == MODIFICATION_TWOBONEIK ? static_cast<TwoBoneIKData *>(mode_data) : nullptr;
}

const SkeletonModification2D::TwoBoneIKData *SkeletonModification2D::_get_twoboneik() const {
	return modification_type == MODIFICATION_TWOBONEIK ? static_cast<const TwoBoneIKData *>(mode_data) : nullptr;
}

SkeletonModification2D::PhysicalBonesData *SkeletonModification2D::_get_physicalbones() {
	return modification_type == MODIFICATION_PHYSICALBONES ? static_cast<PhysicalBonesData *>(mode_data) : nullptr;
}

const SkeletonModification2D::PhysicalBonesData *SkeletonModification2D::_get_physicalbones() const {
	return modification_type == MODIFICATION_PHYSICALBONES ? static_cast<const PhysicalBonesData *>(mode_data) : nullptr;
}

SkeletonModification2D::StackHolderData *SkeletonModification2D::_get_stackholder() {
	return modification_type == MODIFICATION_STACKHOLDER ? static_cast<StackHolderData *>(mode_data) : nullptr;
}

const SkeletonModification2D::StackHolderData *SkeletonModification2D::_get_stackholder() const {
	return modification_type == MODIFICATION_STACKHOLDER ? static_cast<const StackHolderData *>(mode_data) : nullptr;
}

// --- Common base helpers ---

Node2D *SkeletonModification2D::_resolve_node_2d(const NodePath &p_path) const {
	if (!stack || !stack->skeleton || !stack->skeleton->is_inside_tree() || p_path.is_empty()) {
		return nullptr;
	}
	return dynamic_cast<Node2D *>(stack->skeleton->get_node_or_null(p_path));
}

Bone2D *SkeletonModification2D::_resolve_bone_2d(const NodePath &p_path, int &r_bone_idx) const {
	Bone2D *bone_node = dynamic_cast<Bone2D *>(_resolve_node_2d(p_path));
	if (bone_node) {
		r_bone_idx = bone_node->get_index_in_skeleton();
		return bone_node;
	}
	if (stack && stack->skeleton && r_bone_idx >= 0 && r_bone_idx < stack->skeleton->get_bone_count()) {
		return stack->skeleton->get_bone(r_bone_idx);
	}
	return nullptr;
}

bool SkeletonModification2D::_print_execution_error(bool p_condition, const String &p_message) {
	if (p_condition) {
		ERR_PRINT_ONCE(p_message);
	}
	return p_condition;
}

void SkeletonModification2D::reset_state() {
	stack = nullptr;
	is_setup = false;
}

void SkeletonModification2D::set_enabled(bool p_enabled) {
	enabled = p_enabled;
#ifdef TOOLS_ENABLED
	if (editor_draw_gizmo && stack) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

bool SkeletonModification2D::get_enabled() const {
	return enabled;
}

Ref<SkeletonModificationStack2D> SkeletonModification2D::get_modification_stack() {
	return stack;
}

void SkeletonModification2D::set_is_setup(bool p_setup) {
	is_setup = p_setup;
}

bool SkeletonModification2D::get_is_setup() const {
	return is_setup;
}

void SkeletonModification2D::set_execution_mode(int p_mode) {
	execution_mode = p_mode;
}

int SkeletonModification2D::get_execution_mode() const {
	return execution_mode;
}

void SkeletonModification2D::set_editor_draw_gizmo(bool p_draw_gizmo) {
	editor_draw_gizmo = p_draw_gizmo;
#ifdef TOOLS_ENABLED
	if (is_setup && stack) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

bool SkeletonModification2D::get_editor_draw_gizmo() const {
	return editor_draw_gizmo;
}

float SkeletonModification2D::clamp_angle(float p_angle, float p_min_bound, float p_max_bound, bool p_invert) {
	if (p_angle < 0) {
		p_angle = Math::TAU + p_angle;
	}
	if (p_min_bound < 0) {
		p_min_bound = Math::TAU + p_min_bound;
	}
	if (p_max_bound < 0) {
		p_max_bound = Math::TAU + p_max_bound;
	}
	if (p_min_bound > p_max_bound) {
		SWAP(p_min_bound, p_max_bound);
	}

	bool is_beyond_bounds = (p_angle < p_min_bound || p_angle > p_max_bound);
	bool is_within_bounds = (p_angle > p_min_bound && p_angle < p_max_bound);

	if ((!p_invert && is_beyond_bounds) || (p_invert && is_within_bounds)) {
		Vector2 min_bound_vec = Vector2(Math::cos(p_min_bound), Math::sin(p_min_bound));
		Vector2 max_bound_vec = Vector2(Math::cos(p_max_bound), Math::sin(p_max_bound));
		Vector2 angle_vec = Vector2(Math::cos(p_angle), Math::sin(p_angle));

		if (angle_vec.distance_squared_to(min_bound_vec) <= angle_vec.distance_squared_to(max_bound_vec)) {
			p_angle = p_min_bound;
		} else {
			p_angle = p_max_bound;
		}
	}

	return p_angle;
}

void SkeletonModification2D::editor_draw_angle_constraints(Bone2D *p_operation_bone, float p_min_bound, float p_max_bound, bool p_constraint_enabled, bool p_constraint_in_localspace, bool p_constraint_inverted) {
	if (!p_operation_bone || !p_constraint_enabled || !stack || !stack->skeleton) {
		return;
	}
	const Color bone_ik_color = Color(1.0, 0.65, 0.0, 0.4);

	float arc_angle_min = p_min_bound;
	float arc_angle_max = p_max_bound;
	if (arc_angle_min < 0) {
		arc_angle_min = Math::TAU + arc_angle_min;
	}
	if (arc_angle_max < 0) {
		arc_angle_max = Math::TAU + arc_angle_max;
	}
	if (arc_angle_min > arc_angle_max) {
		SWAP(arc_angle_min, arc_angle_max);
	}
	arc_angle_min += p_operation_bone->get_bone_angle();
	arc_angle_max += p_operation_bone->get_bone_angle();

	if (p_constraint_in_localspace) {
		Node2D *parent_node = dynamic_cast<Node2D *>(p_operation_bone->get_parent());
		if (parent_node) {
			stack->skeleton->draw_set_transform(stack->skeleton->to_local(p_operation_bone->get_global_position()), parent_node->get_global_rotation() - stack->skeleton->get_global_rotation());
		} else {
			stack->skeleton->draw_set_transform(stack->skeleton->to_local(p_operation_bone->get_global_position()));
		}
	} else {
		stack->skeleton->draw_set_transform(stack->skeleton->to_local(p_operation_bone->get_global_position()));
	}

	float radius = p_operation_bone->get_length() * 0.6f;
	if (p_constraint_inverted) {
		stack->skeleton->draw_arc(Vector2(0, 0), radius, arc_angle_min + float(Math::TAU), arc_angle_max, 32, bone_ik_color, 1.5f);
	} else {
		stack->skeleton->draw_arc(Vector2(0, 0), radius, arc_angle_min, arc_angle_max, 32, bone_ik_color, 1.5f);
	}
	stack->skeleton->draw_line(Vector2(0, 0), Vector2(Math::cos(arc_angle_min), Math::sin(arc_angle_min)) * radius, bone_ik_color, 1.5f);
	stack->skeleton->draw_line(Vector2(0, 0), Vector2(Math::cos(arc_angle_max), Math::sin(arc_angle_max)) * radius, bone_ik_color, 1.5f);
}

// --- Non-virtual dispatch: setup, gizmos, execute ---

void SkeletonModification2D::_setup_modification(SkeletonModificationStack2D *p_stack) {
	stack = p_stack;
	if (!stack) {
		is_setup = false;
		WARN_PRINT("Could not setup modification with name " + get_name());
		return;
	}
	is_setup = true;

	switch (modification_type) {
		case MODIFICATION_NONE:
			break;
		case MODIFICATION_LOOKAT: {
			LookAtData *d = _get_lookat();
			if (d && stack->skeleton && stack->skeleton->is_inside_tree()) {
				_resolve_bone_2d(d->bone2d_node, d->bone_idx);
			}
		} break;
		case MODIFICATION_CCDIK: {
			CCDIKData *d = _get_ccdik();
			if (d && stack->skeleton && stack->skeleton->is_inside_tree()) {
				for (int i = 0; i < d->chain.size(); i++) {
					_resolve_bone_2d(d->chain[i].bone2d_node, d->chain.write[i].bone_idx);
				}
			}
		} break;
		case MODIFICATION_FABRIK: {
			FABRIKData *d = _get_fabrik();
			if (d && stack->skeleton && stack->skeleton->is_inside_tree()) {
				for (int i = 0; i < d->chain.size(); i++) {
					_resolve_bone_2d(d->chain[i].bone2d_node, d->chain.write[i].bone_idx);
				}
			}
		} break;
		case MODIFICATION_JIGGLE: {
			JiggleData *d = _get_jiggle();
			if (d && stack->skeleton && stack->skeleton->is_inside_tree()) {
				for (int i = 0; i < d->chain.size(); i++) {
					Bone2D *bone_node = _resolve_bone_2d(d->chain[i].bone2d_node, d->chain.write[i].bone_idx);
					if (bone_node) {
						Vector2 pos = bone_node->get_global_position();
						d->chain.write[i].dynamic_position = pos;
						d->chain.write[i].last_position = pos;
						d->chain.write[i].last_noncollision_position = pos;
					}
				}
			}
		} break;
		case MODIFICATION_TWOBONEIK: {
			TwoBoneIKData *d = _get_twoboneik();
			if (d && stack->skeleton && stack->skeleton->is_inside_tree()) {
				_resolve_bone_2d(d->joint_one_bone2d_node, d->joint_one_bone_idx);
				_resolve_bone_2d(d->joint_two_bone2d_node, d->joint_two_bone_idx);
			}
		} break;
		case MODIFICATION_PHYSICALBONES: {
			PhysicalBonesData *d = _get_physicalbones();
			if (d && stack->skeleton && stack->skeleton->is_inside_tree()) {
				for (int i = 0; i < d->chain.size(); i++) {
					_resolve_bone_2d(d->chain[i].physical_bone_node, d->chain.write[i].bone_idx);
				}
			}
		} break;
		case MODIFICATION_STACKHOLDER: {
			StackHolderData *d = _get_stackholder();
			if (d && d->held_modification_stack.is_valid()) {
				d->held_modification_stack->set_skeleton(stack->get_skeleton());
				d->held_modification_stack->setup();
			}
		} break;
	}
}

void SkeletonModification2D::_draw_editor_gizmo() {
	if (!enabled || !is_setup || !stack || !stack->skeleton) {
		return;
	}

	switch (modification_type) {
		case MODIFICATION_NONE:
		case MODIFICATION_FABRIK:
		case MODIFICATION_JIGGLE:
		case MODIFICATION_PHYSICALBONES:
			break;
		case MODIFICATION_LOOKAT: {
			const LookAtData *d = _get_lookat();
			if (d && d->bone_idx >= 0 && d->bone_idx < stack->skeleton->get_bone_count()) {
				Bone2D *operation_bone = stack->skeleton->get_bone(d->bone_idx);
				editor_draw_angle_constraints(operation_bone, d->constraint_angle_min, d->constraint_angle_max, d->enable_constraint, d->constraint_in_localspace, d->constraint_angle_invert);
			}
		} break;
		case MODIFICATION_CCDIK: {
			const CCDIKData *d = _get_ccdik();
			if (d) {
				for (int i = 0; i < d->chain.size(); i++) {
					if (!d->chain[i].joint_draw_gizmo || d->chain[i].bone_idx < 0 || d->chain[i].bone_idx >= stack->skeleton->get_bone_count()) {
						continue;
					}
					Bone2D *operation_bone = stack->skeleton->get_bone(d->chain[i].bone_idx);
					editor_draw_angle_constraints(operation_bone, d->chain[i].constraint_angle_min, d->chain[i].constraint_angle_max, d->chain[i].enable_constraint, d->chain[i].constraint_in_localspace, d->chain[i].constraint_angle_invert);
				}
			}
		} break;
		case MODIFICATION_TWOBONEIK: {
#ifdef TOOLS_ENABLED
			const TwoBoneIKData *d = _get_twoboneik();
			if (d && d->editor_draw_min_max && d->joint_one_bone_idx >= 0 && d->joint_one_bone_idx < stack->skeleton->get_bone_count()) {
				Bone2D *bone_one = stack->skeleton->get_bone(d->joint_one_bone_idx);
				if (bone_one) {
					const Color bone_ik_color = Color(1.0, 0.65, 0.0, 0.4);
					Vector2 origin = stack->skeleton->to_local(bone_one->get_global_position());
					stack->skeleton->draw_set_transform(origin);
					if (d->target_minimum_distance > 0.0f) {
						stack->skeleton->draw_arc(Vector2(0, 0), d->target_minimum_distance, 0.0f, float(Math::TAU), 32, bone_ik_color, 1.5f);
					}
					if (d->target_maximum_distance > 0.0f) {
						stack->skeleton->draw_arc(Vector2(0, 0), d->target_maximum_distance, 0.0f, float(Math::TAU), 32, bone_ik_color, 1.5f);
					}
				}
			}
#endif // TOOLS_ENABLED
		} break;
		case MODIFICATION_STACKHOLDER: {
			const StackHolderData *d = _get_stackholder();
			if (d && d->held_modification_stack.is_valid()) {
				d->held_modification_stack->draw_editor_gizmos();
			}
		} break;
	}
}

void SkeletonModification2D::_execute(float p_delta) {
	if (!enabled || !is_setup || !stack || !stack->skeleton) {
		return;
	}

	switch (modification_type) {
		case MODIFICATION_NONE:
			break;
		case MODIFICATION_LOOKAT:
			_execute_lookat(p_delta);
			break;
		case MODIFICATION_CCDIK:
			_execute_ccdik(p_delta);
			break;
		case MODIFICATION_FABRIK:
			_execute_fabrik(p_delta);
			break;
		case MODIFICATION_JIGGLE:
			_execute_jiggle(p_delta);
			break;
		case MODIFICATION_TWOBONEIK:
			_execute_twoboneik(p_delta);
			break;
		case MODIFICATION_PHYSICALBONES:
			_execute_physicalbones(p_delta);
			break;
		case MODIFICATION_STACKHOLDER:
			_execute_stackholder(p_delta);
			break;
	}
}

// --- Solver implementations ---

void SkeletonModification2D::_execute_lookat(float p_delta) {
	LookAtData *d = _get_lookat();
	if (!d) {
		return;
	}
	Bone2D *operation_bone = _resolve_bone_2d(d->bone2d_node, d->bone_idx);
	Node2D *target = _resolve_node_2d(target_node);
	if (!operation_bone || !target) {
		return;
	}

	Transform2D operation_transform = operation_bone->get_global_transform();
	operation_bone->look_at(target->get_global_position());
	operation_bone->set_rotation(operation_bone->get_rotation() - operation_bone->get_bone_angle() + d->additional_rotation);

	if (d->enable_constraint) {
		if (d->constraint_in_localspace) {
			operation_bone->set_rotation(clamp_angle(operation_bone->get_rotation(), d->constraint_angle_min, d->constraint_angle_max, d->constraint_angle_invert));
		} else {
			operation_bone->set_global_rotation(clamp_angle(operation_bone->get_global_rotation(), d->constraint_angle_min, d->constraint_angle_max, d->constraint_angle_invert));
		}
	}

	Transform2D solved_local = operation_bone->get_transform();
	operation_bone->set_global_transform(operation_transform.interpolate_with(operation_bone->get_global_transform(), stack->strength));
	stack->skeleton->set_bone_local_pose_override(d->bone_idx, solved_local, stack->strength, false);
}

void SkeletonModification2D::_execute_ccdik(float p_delta) {
	CCDIKData *d = _get_ccdik();
	if (!d || d->chain.is_empty()) {
		return;
	}
	Node2D *target = _resolve_node_2d(target_node);
	Node2D *tip = _resolve_node_2d(d->tip_node);
	if (!target || !tip) {
		return;
	}

	for (int i = 0; i < d->chain.size(); i++) {
		_execute_ccdik_joint(i, target, tip);
	}
}

void SkeletonModification2D::_execute_ccdik_joint(int p_joint_idx, Node2D *p_target, Node2D *p_tip) {
	CCDIKData *d = _get_ccdik();
	if (!d || p_joint_idx < 0 || p_joint_idx >= d->chain.size()) {
		return;
	}
	CCDIKData::JointData &j = d->chain.write[p_joint_idx];
	Bone2D *operation_bone = _resolve_bone_2d(j.bone2d_node, j.bone_idx);
	if (!operation_bone) {
		return;
	}

	Transform2D prev_global = operation_bone->get_global_transform();
	if (j.rotate_from_joint) {
		operation_bone->look_at(p_target->get_global_position());
		operation_bone->set_rotation(operation_bone->get_rotation() - operation_bone->get_bone_angle());
	} else {
		Vector2 joint_pos = operation_bone->get_global_position();
		float tip_angle = joint_pos.angle_to_point(p_tip->get_global_position());
		float target_angle = joint_pos.angle_to_point(p_target->get_global_position());
		operation_bone->set_global_rotation(operation_bone->get_global_rotation() + (target_angle - tip_angle));
	}

	if (j.enable_constraint) {
		if (j.constraint_in_localspace) {
			operation_bone->set_rotation(clamp_angle(operation_bone->get_rotation(), j.constraint_angle_min, j.constraint_angle_max, j.constraint_angle_invert));
		} else {
			operation_bone->set_global_rotation(clamp_angle(operation_bone->get_global_rotation(), j.constraint_angle_min, j.constraint_angle_max, j.constraint_angle_invert));
		}
	}

	Transform2D solved_local = operation_bone->get_transform();
	operation_bone->set_global_transform(prev_global.interpolate_with(operation_bone->get_global_transform(), stack->strength));
	stack->skeleton->set_bone_local_pose_override(j.bone_idx, solved_local, stack->strength, false);
}

void SkeletonModification2D::_execute_fabrik(float p_delta) {
	FABRIKData *d = _get_fabrik();
	if (!d || d->chain.size() < 2) {
		return;
	}
	Node2D *target = _resolve_node_2d(target_node);
	if (!target) {
		return;
	}

	const int chain_len = d->chain.size();
	d->transform_chain.resize(chain_len);
	for (int i = 0; i < chain_len; i++) {
		Bone2D *b = _resolve_bone_2d(d->chain[i].bone2d_node, d->chain.write[i].bone_idx);
		if (!b) {
			return;
		}
		d->transform_chain.write[i] = b->get_global_transform();
	}

	d->origin_global_pose = d->transform_chain[0];
	d->target_global_pose = target->get_global_transform();

	Bone2D *final_bone = stack->skeleton->get_bone(d->chain[chain_len - 1].bone_idx);
	float final_length = final_bone->get_length();
	float final_angle = d->transform_chain[chain_len - 1].get_rotation() + final_bone->get_bone_angle();
	Vector2 final_tip = d->transform_chain[chain_len - 1].get_origin() + Vector2(Math::cos(final_angle), Math::sin(final_angle)) * final_length;

	int iterations = 0;
	while (final_tip.distance_to(d->target_global_pose.get_origin()) > d->chain_tolerance && iterations < d->chain_max_iterations) {
		_fabrik_chain_backwards();
		_fabrik_chain_forwards();
		final_angle = d->transform_chain[chain_len - 1].get_rotation() + final_bone->get_bone_angle();
		final_tip = d->transform_chain[chain_len - 1].get_origin() + Vector2(Math::cos(final_angle), Math::sin(final_angle)) * final_length;
		iterations++;
	}

	for (int i = 0; i < chain_len; i++) {
		Bone2D *b = stack->skeleton->get_bone(d->chain[i].bone_idx);
		Transform2D prev_global = b->get_global_transform();
		Transform2D next_global = d->transform_chain[i];
		if (i == chain_len - 1 && d->chain[i].use_target_rotation) {
			next_global.set_rotation(d->target_global_pose.get_rotation());
		}
		b->set_global_transform(prev_global.interpolate_with(next_global, stack->strength));
		stack->skeleton->set_bone_local_pose_override(d->chain[i].bone_idx, b->get_transform(), stack->strength, false);
	}
}

void SkeletonModification2D::_fabrik_chain_backwards() {
	FABRIKData *d = _get_fabrik();
	const int last_idx = d->chain.size() - 1;
	Bone2D *last_bone = stack->skeleton->get_bone(d->chain[last_idx].bone_idx);
	float last_angle = d->transform_chain[last_idx].get_rotation() + last_bone->get_bone_angle();
	Vector2 dir = Vector2(Math::cos(last_angle), Math::sin(last_angle));
	d->transform_chain.write[last_idx].set_origin(d->target_global_pose.get_origin() - dir * last_bone->get_length());

	for (int i = last_idx; i >= 1; i--) {
		Vector2 next_origin = d->transform_chain[i].get_origin();
		Vector2 curr_origin = d->transform_chain[i - 1].get_origin() + d->chain[i - 1].magnet_position;
		Bone2D *prev_bone = stack->skeleton->get_bone(d->chain[i - 1].bone_idx);
		Vector2 to_prev = (curr_origin - next_origin).normalized();
		d->transform_chain.write[i - 1].set_origin(next_origin + to_prev * prev_bone->get_length());
	}
}

void SkeletonModification2D::_fabrik_chain_forwards() {
	FABRIKData *d = _get_fabrik();
	d->transform_chain.write[0].set_origin(d->origin_global_pose.get_origin());
	const int chain_len = d->chain.size();

	for (int i = 0; i < chain_len - 1; i++) {
		Bone2D *curr_bone = stack->skeleton->get_bone(d->chain[i].bone_idx);
		Vector2 curr_origin = d->transform_chain[i].get_origin();
		Vector2 next_origin = d->transform_chain[i + 1].get_origin();
		float angle = curr_origin.angle_to_point(next_origin);
		d->transform_chain.write[i].set_rotation(angle - curr_bone->get_bone_angle());
		d->transform_chain.write[i + 1].set_origin(curr_origin + Vector2(Math::cos(angle), Math::sin(angle)) * curr_bone->get_length());
	}

	Bone2D *last_bone = stack->skeleton->get_bone(d->chain[chain_len - 1].bone_idx);
	Vector2 last_origin = d->transform_chain[chain_len - 1].get_origin();
	float target_angle = last_origin.angle_to_point(d->target_global_pose.get_origin());
	d->transform_chain.write[chain_len - 1].set_rotation(target_angle - last_bone->get_bone_angle());
}

void SkeletonModification2D::_execute_jiggle(float p_delta) {
	JiggleData *d = _get_jiggle();
	if (!d || d->chain.is_empty()) {
		return;
	}
	Node2D *target = _resolve_node_2d(target_node);
	if (!target) {
		return;
	}
	for (int i = 0; i < d->chain.size(); i++) {
		_execute_jiggle_joint(i, target, p_delta);
	}
}

void SkeletonModification2D::_execute_jiggle_joint(int p_joint_idx, Node2D *p_target, float p_delta) {
	JiggleData *d = _get_jiggle();
	if (!d || p_joint_idx < 0 || p_joint_idx >= d->chain.size()) {
		return;
	}
	JiggleData::JointData &j = d->chain.write[p_joint_idx];
	Bone2D *operation_bone = _resolve_bone_2d(j.bone2d_node, j.bone_idx);
	if (!operation_bone) {
		return;
	}

	Vector2 target_pos = p_target->get_global_position();
	float eff_mass = MAX(j.mass, CMP_EPSILON);
	j.force = (target_pos - j.dynamic_position) * j.stiffness * p_delta;
	if (j.use_gravity) {
		j.force += j.gravity * p_delta;
	}
	j.acceleration = j.force / eff_mass;
	j.velocity += j.acceleration * (1.0f - j.damping);
	j.dynamic_position += j.velocity + j.force;
	j.dynamic_position = j.dynamic_position - j.last_position + operation_bone->get_global_position();
	j.last_position = operation_bone->get_global_position();

	Transform2D prev_global = operation_bone->get_global_transform();
	operation_bone->look_at(j.dynamic_position);
	operation_bone->set_rotation(operation_bone->get_rotation() - operation_bone->get_bone_angle());
	Transform2D solved_local = operation_bone->get_transform();
	operation_bone->set_global_transform(prev_global.interpolate_with(operation_bone->get_global_transform(), stack->strength));
	stack->skeleton->set_bone_local_pose_override(j.bone_idx, solved_local, stack->strength, false);
}

void SkeletonModification2D::_execute_twoboneik(float p_delta) {
	TwoBoneIKData *d = _get_twoboneik();
	if (!d) {
		return;
	}
	Node2D *target = _resolve_node_2d(target_node);
	Bone2D *bone_one = _resolve_bone_2d(d->joint_one_bone2d_node, d->joint_one_bone_idx);
	Bone2D *bone_two = _resolve_bone_2d(d->joint_two_bone2d_node, d->joint_two_bone_idx);
	if (!target || !bone_one || !bone_two) {
		return;
	}

	Transform2D prev_one = bone_one->get_global_transform();
	Transform2D prev_two = bone_two->get_global_transform();

	Vector2 origin = bone_one->get_global_position();
	Vector2 target_diff = target->get_global_position() - origin;
	float target_dist = target_diff.length();
	if (d->target_minimum_distance > 0.0f && target_dist < d->target_minimum_distance) {
		target_dist = d->target_minimum_distance;
	}
	if (d->target_maximum_distance > 0.0f && target_dist > d->target_maximum_distance) {
		target_dist = d->target_maximum_distance;
	}

	float len_one = bone_one->get_length();
	float len_two = bone_two->get_length();
	float total_len = len_one + len_two;

	float base_angle = target_diff.angle();
	if (target_dist >= total_len || Math::is_zero_approx(len_one) || Math::is_zero_approx(len_two)) {
		bone_one->set_global_rotation(base_angle - bone_one->get_bone_angle());
		bone_two->set_global_rotation(base_angle - bone_two->get_bone_angle());
	} else {
		float clamped_dist = MAX(target_dist, Math::abs(len_one - len_two) + CMP_EPSILON);
		float cos_a = CLAMP((clamped_dist * clamped_dist + len_one * len_one - len_two * len_two) / (2.0f * clamped_dist * len_one), -1.0f, 1.0f);
		float cos_b = CLAMP((len_one * len_one + len_two * len_two - clamped_dist * clamped_dist) / (2.0f * len_one * len_two), -1.0f, 1.0f);
		float angle_a = Math::acos(cos_a);
		float angle_b = Math::acos(cos_b);
		if (d->flip_bend_direction) {
			angle_a = -angle_a;
			angle_b = -angle_b;
		}
		float rot_one = base_angle - angle_a;
		float rot_two = rot_one + float(Math::PI) - angle_b;
		bone_one->set_global_rotation(rot_one - bone_one->get_bone_angle());
		bone_two->set_global_rotation(rot_two - bone_two->get_bone_angle());
	}

	Transform2D solved_one = bone_one->get_transform();
	Transform2D solved_two = bone_two->get_transform();
	bone_one->set_global_transform(prev_one.interpolate_with(bone_one->get_global_transform(), stack->strength));
	bone_two->set_global_transform(prev_two.interpolate_with(bone_two->get_global_transform(), stack->strength));
	stack->skeleton->set_bone_local_pose_override(d->joint_one_bone_idx, solved_one, stack->strength, false);
	stack->skeleton->set_bone_local_pose_override(d->joint_two_bone_idx, solved_two, stack->strength, false);
}

void SkeletonModification2D::_execute_physicalbones(float p_delta) {
	PhysicalBonesData *d = _get_physicalbones();
	if (!d || !d->simulation_active) {
		return;
	}
	for (int i = 0; i < d->chain.size(); i++) {
		PhysicalBonesData::JointData &j = d->chain.write[i];
		if (!j.simulating) {
			continue;
		}
		Node2D *phys_node = _resolve_node_2d(j.physical_bone_node);
		if (!phys_node || j.bone_idx < 0 || j.bone_idx >= stack->skeleton->get_bone_count()) {
			continue;
		}
		Bone2D *bone_node = stack->skeleton->get_bone(j.bone_idx);
		if (!bone_node) {
			continue;
		}
		Transform2D prev_global = bone_node->get_global_transform();
		bone_node->set_global_transform(prev_global.interpolate_with(phys_node->get_global_transform(), stack->strength));
		stack->skeleton->set_bone_local_pose_override(j.bone_idx, bone_node->get_transform(), stack->strength, false);
	}
}

void SkeletonModification2D::_execute_stackholder(float p_delta) {
	StackHolderData *d = _get_stackholder();
	if (d && d->held_modification_stack.is_valid()) {
		d->held_modification_stack->execute(p_delta, execution_mode);
	}
}

// --- Shared target node ---

void SkeletonModification2D::set_target_node(const NodePath &p_target_node) {
	target_node = p_target_node;
}

NodePath SkeletonModification2D::get_target_node() const {
	return target_node;
}

// --- LookAt accessors ---

void SkeletonModification2D::set_bone2d_node(const NodePath &p_target_node) {
	_ensure_mode(MODIFICATION_LOOKAT);
	LookAtData *d = _get_lookat();
	ERR_FAIL_NULL(d);
	d->bone2d_node = p_target_node;
	if (is_setup) {
		_resolve_bone_2d(d->bone2d_node, d->bone_idx);
	}
}

NodePath SkeletonModification2D::get_bone2d_node() const {
	const LookAtData *d = _get_lookat();
	return d ? d->bone2d_node : NodePath();
}

void SkeletonModification2D::set_bone_index(int p_idx) {
	_ensure_mode(MODIFICATION_LOOKAT);
	LookAtData *d = _get_lookat();
	ERR_FAIL_NULL(d);
	d->bone_idx = p_idx;
	if (is_setup && stack && stack->skeleton && p_idx >= 0 && p_idx < stack->skeleton->get_bone_count()) {
		Bone2D *b = stack->skeleton->get_bone(p_idx);
		if (b) {
			d->bone2d_node = stack->skeleton->get_path_to(b);
		}
	}
}

int SkeletonModification2D::get_bone_index() const {
	const LookAtData *d = _get_lookat();
	return d ? d->bone_idx : -1;
}

void SkeletonModification2D::set_additional_rotation(float p_rotation) {
	_ensure_mode(MODIFICATION_LOOKAT);
	LookAtData *d = _get_lookat();
	ERR_FAIL_NULL(d);
	d->additional_rotation = p_rotation;
}

float SkeletonModification2D::get_additional_rotation() const {
	const LookAtData *d = _get_lookat();
	return d ? d->additional_rotation : 0.0f;
}

void SkeletonModification2D::set_enable_constraint(bool p_constraint) {
	_ensure_mode(MODIFICATION_LOOKAT);
	LookAtData *d = _get_lookat();
	ERR_FAIL_NULL(d);
	d->enable_constraint = p_constraint;
#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

bool SkeletonModification2D::get_enable_constraint() const {
	const LookAtData *d = _get_lookat();
	return d ? d->enable_constraint : false;
}

void SkeletonModification2D::set_constraint_angle_min(float p_angle_min) {
	_ensure_mode(MODIFICATION_LOOKAT);
	LookAtData *d = _get_lookat();
	ERR_FAIL_NULL(d);
	d->constraint_angle_min = p_angle_min;
#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

float SkeletonModification2D::get_constraint_angle_min() const {
	const LookAtData *d = _get_lookat();
	return d ? d->constraint_angle_min : 0.0f;
}

void SkeletonModification2D::set_constraint_angle_max(float p_angle_max) {
	_ensure_mode(MODIFICATION_LOOKAT);
	LookAtData *d = _get_lookat();
	ERR_FAIL_NULL(d);
	d->constraint_angle_max = p_angle_max;
#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

float SkeletonModification2D::get_constraint_angle_max() const {
	const LookAtData *d = _get_lookat();
	return d ? d->constraint_angle_max : float(Math::TAU);
}

void SkeletonModification2D::set_constraint_angle_invert(bool p_invert) {
	_ensure_mode(MODIFICATION_LOOKAT);
	LookAtData *d = _get_lookat();
	ERR_FAIL_NULL(d);
	d->constraint_angle_invert = p_invert;
#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

bool SkeletonModification2D::get_constraint_angle_invert() const {
	const LookAtData *d = _get_lookat();
	return d ? d->constraint_angle_invert : false;
}

void SkeletonModification2D::set_constraint_in_localspace(bool p_constraint_in_localspace) {
	_ensure_mode(MODIFICATION_LOOKAT);
	LookAtData *d = _get_lookat();
	ERR_FAIL_NULL(d);
	d->constraint_in_localspace = p_constraint_in_localspace;
#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

bool SkeletonModification2D::get_constraint_in_localspace() const {
	const LookAtData *d = _get_lookat();
	return d ? d->constraint_in_localspace : true;
}

// --- CCDIK accessors ---

void SkeletonModification2D::set_tip_node(const NodePath &p_tip_node) {
	_ensure_mode(MODIFICATION_CCDIK);
	CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL(d);
	d->tip_node = p_tip_node;
}

NodePath SkeletonModification2D::get_tip_node() const {
	const CCDIKData *d = _get_ccdik();
	return d ? d->tip_node : NodePath();
}

int SkeletonModification2D::get_ccdik_data_chain_length() const {
	const CCDIKData *d = _get_ccdik();
	return d ? d->chain.size() : 0;
}

void SkeletonModification2D::set_ccdik_data_chain_length(int p_new_length) {
	ERR_FAIL_COND(p_new_length < 0);
	_ensure_mode(MODIFICATION_CCDIK);
	CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL(d);
	d->chain.resize(p_new_length);
}

void SkeletonModification2D::set_ccdik_joint_bone2d_node(int p_joint_idx, const NodePath &p_target_node) {
	CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "CCDIK joint out of range!");
	d->chain.write[p_joint_idx].bone2d_node = p_target_node;
	if (is_setup) {
		_resolve_bone_2d(p_target_node, d->chain.write[p_joint_idx].bone_idx);
	}
}

NodePath SkeletonModification2D::get_ccdik_joint_bone2d_node(int p_joint_idx) const {
	const CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL_V(d, NodePath());
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), NodePath(), "CCDIK joint out of range!");
	return d->chain[p_joint_idx].bone2d_node;
}

void SkeletonModification2D::set_ccdik_joint_bone_index(int p_joint_idx, int p_bone_idx) {
	CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "CCDIK joint out of range!");
	d->chain.write[p_joint_idx].bone_idx = p_bone_idx;
	if (is_setup && stack && stack->skeleton && p_bone_idx >= 0 && p_bone_idx < stack->skeleton->get_bone_count()) {
		Bone2D *b = stack->skeleton->get_bone(p_bone_idx);
		if (b) {
			d->chain.write[p_joint_idx].bone2d_node = stack->skeleton->get_path_to(b);
		}
	}
}

int SkeletonModification2D::get_ccdik_joint_bone_index(int p_joint_idx) const {
	const CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL_V(d, -1);
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), -1, "CCDIK joint out of range!");
	return d->chain[p_joint_idx].bone_idx;
}

void SkeletonModification2D::set_ccdik_joint_rotate_from_joint(int p_joint_idx, bool p_rotate_from_joint) {
	CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "CCDIK joint out of range!");
	d->chain.write[p_joint_idx].rotate_from_joint = p_rotate_from_joint;
}

bool SkeletonModification2D::get_ccdik_joint_rotate_from_joint(int p_joint_idx) const {
	const CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL_V(d, false);
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), false, "CCDIK joint out of range!");
	return d->chain[p_joint_idx].rotate_from_joint;
}

void SkeletonModification2D::set_ccdik_joint_enable_constraint(int p_joint_idx, bool p_constraint) {
	CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "CCDIK joint out of range!");
	d->chain.write[p_joint_idx].enable_constraint = p_constraint;
#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

bool SkeletonModification2D::get_ccdik_joint_enable_constraint(int p_joint_idx) const {
	const CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL_V(d, false);
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), false, "CCDIK joint out of range!");
	return d->chain[p_joint_idx].enable_constraint;
}

void SkeletonModification2D::set_ccdik_joint_constraint_angle_min(int p_joint_idx, float p_angle_min) {
	CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "CCDIK joint out of range!");
	d->chain.write[p_joint_idx].constraint_angle_min = p_angle_min;
#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

float SkeletonModification2D::get_ccdik_joint_constraint_angle_min(int p_joint_idx) const {
	const CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL_V(d, 0.0f);
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), 0.0f, "CCDIK joint out of range!");
	return d->chain[p_joint_idx].constraint_angle_min;
}

void SkeletonModification2D::set_ccdik_joint_constraint_angle_max(int p_joint_idx, float p_angle_max) {
	CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "CCDIK joint out of range!");
	d->chain.write[p_joint_idx].constraint_angle_max = p_angle_max;
#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

float SkeletonModification2D::get_ccdik_joint_constraint_angle_max(int p_joint_idx) const {
	const CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL_V(d, 0.0f);
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), 0.0f, "CCDIK joint out of range!");
	return d->chain[p_joint_idx].constraint_angle_max;
}

void SkeletonModification2D::set_ccdik_joint_constraint_angle_invert(int p_joint_idx, bool p_invert) {
	CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "CCDIK joint out of range!");
	d->chain.write[p_joint_idx].constraint_angle_invert = p_invert;
#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

bool SkeletonModification2D::get_ccdik_joint_constraint_angle_invert(int p_joint_idx) const {
	const CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL_V(d, false);
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), false, "CCDIK joint out of range!");
	return d->chain[p_joint_idx].constraint_angle_invert;
}

void SkeletonModification2D::set_ccdik_joint_constraint_in_localspace(int p_joint_idx, bool p_constraint_in_localspace) {
	CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "CCDIK joint out of range!");
	d->chain.write[p_joint_idx].constraint_in_localspace = p_constraint_in_localspace;
#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

bool SkeletonModification2D::get_ccdik_joint_constraint_in_localspace(int p_joint_idx) const {
	const CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL_V(d, false);
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), false, "CCDIK joint out of range!");
	return d->chain[p_joint_idx].constraint_in_localspace;
}

void SkeletonModification2D::set_ccdik_joint_editor_draw_gizmo(int p_joint_idx, bool p_draw_gizmo) {
	CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "CCDIK joint out of range!");
	d->chain.write[p_joint_idx].joint_draw_gizmo = p_draw_gizmo;
#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

bool SkeletonModification2D::get_ccdik_joint_editor_draw_gizmo(int p_joint_idx) const {
	const CCDIKData *d = _get_ccdik();
	ERR_FAIL_NULL_V(d, false);
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), false, "CCDIK joint out of range!");
	return d->chain[p_joint_idx].joint_draw_gizmo;
}

// --- FABRIK accessors ---

int SkeletonModification2D::get_fabrik_data_chain_length() const {
	const FABRIKData *d = _get_fabrik();
	return d ? d->chain.size() : 0;
}

void SkeletonModification2D::set_fabrik_data_chain_length(int p_new_length) {
	ERR_FAIL_COND(p_new_length < 0);
	_ensure_mode(MODIFICATION_FABRIK);
	FABRIKData *d = _get_fabrik();
	ERR_FAIL_NULL(d);
	d->chain.resize(p_new_length);
}

void SkeletonModification2D::set_fabrik_joint_bone2d_node(int p_joint_idx, const NodePath &p_target_node) {
	FABRIKData *d = _get_fabrik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "FABRIK joint out of range!");
	d->chain.write[p_joint_idx].bone2d_node = p_target_node;
	if (is_setup) {
		_resolve_bone_2d(p_target_node, d->chain.write[p_joint_idx].bone_idx);
	}
}

NodePath SkeletonModification2D::get_fabrik_joint_bone2d_node(int p_joint_idx) const {
	const FABRIKData *d = _get_fabrik();
	ERR_FAIL_NULL_V(d, NodePath());
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), NodePath(), "FABRIK joint out of range!");
	return d->chain[p_joint_idx].bone2d_node;
}

void SkeletonModification2D::set_fabrik_joint_bone_index(int p_joint_idx, int p_bone_idx) {
	FABRIKData *d = _get_fabrik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "FABRIK joint out of range!");
	d->chain.write[p_joint_idx].bone_idx = p_bone_idx;
	if (is_setup && stack && stack->skeleton && p_bone_idx >= 0 && p_bone_idx < stack->skeleton->get_bone_count()) {
		Bone2D *b = stack->skeleton->get_bone(p_bone_idx);
		if (b) {
			d->chain.write[p_joint_idx].bone2d_node = stack->skeleton->get_path_to(b);
		}
	}
}

int SkeletonModification2D::get_fabrik_joint_bone_index(int p_joint_idx) const {
	const FABRIKData *d = _get_fabrik();
	ERR_FAIL_NULL_V(d, -1);
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), -1, "FABRIK joint out of range!");
	return d->chain[p_joint_idx].bone_idx;
}

void SkeletonModification2D::set_fabrik_joint_magnet_position(int p_joint_idx, Vector2 p_magnet_position) {
	FABRIKData *d = _get_fabrik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "FABRIK joint out of range!");
	d->chain.write[p_joint_idx].magnet_position = p_magnet_position;
}

Vector2 SkeletonModification2D::get_fabrik_joint_magnet_position(int p_joint_idx) const {
	const FABRIKData *d = _get_fabrik();
	ERR_FAIL_NULL_V(d, Vector2());
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), Vector2(), "FABRIK joint out of range!");
	return d->chain[p_joint_idx].magnet_position;
}

void SkeletonModification2D::set_fabrik_joint_use_target_rotation(int p_joint_idx, bool p_use_target_rotation) {
	FABRIKData *d = _get_fabrik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "FABRIK joint out of range!");
	d->chain.write[p_joint_idx].use_target_rotation = p_use_target_rotation;
}

bool SkeletonModification2D::get_fabrik_joint_use_target_rotation(int p_joint_idx) const {
	const FABRIKData *d = _get_fabrik();
	ERR_FAIL_NULL_V(d, false);
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), false, "FABRIK joint out of range!");
	return d->chain[p_joint_idx].use_target_rotation;
}

void SkeletonModification2D::set_chain_tolerance(float p_tolerance) {
	_ensure_mode(MODIFICATION_FABRIK);
	FABRIKData *d = _get_fabrik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_COND(p_tolerance <= 0.0f);
	d->chain_tolerance = p_tolerance;
}

float SkeletonModification2D::get_chain_tolerance() const {
	const FABRIKData *d = _get_fabrik();
	return d ? d->chain_tolerance : 0.01f;
}

void SkeletonModification2D::set_chain_max_iterations(int p_iterations) {
	_ensure_mode(MODIFICATION_FABRIK);
	FABRIKData *d = _get_fabrik();
	ERR_FAIL_NULL(d);
	ERR_FAIL_COND(p_iterations < 1);
	d->chain_max_iterations = p_iterations;
}

int SkeletonModification2D::get_chain_max_iterations() const {
	const FABRIKData *d = _get_fabrik();
	return d ? d->chain_max_iterations : 10;
}

// --- Jiggle accessors ---

void SkeletonModification2D::_update_jiggle_joint_data() {
	JiggleData *d = _get_jiggle();
	if (!d) {
		return;
	}
	for (int i = 0; i < d->chain.size(); i++) {
		if (!d->chain[i].override_defaults) {
			d->chain.write[i].stiffness = d->stiffness;
			d->chain.write[i].mass = d->mass;
			d->chain.write[i].damping = d->damping;
			d->chain.write[i].use_gravity = d->use_gravity;
			d->chain.write[i].gravity = d->gravity;
		}
	}
}

void SkeletonModification2D::reset() {
	JiggleData *d = _get_jiggle();
	if (!d || !is_setup || !stack || !stack->skeleton) {
		return;
	}
	for (int i = 0; i < d->chain.size(); i++) {
		const int b_idx = d->chain[i].bone_idx;
		if (b_idx < 0 || b_idx >= stack->skeleton->get_bone_count()) {
			continue;
		}
		Bone2D *b = stack->skeleton->get_bone(b_idx);
		if (b) {
			Vector2 bone_pos = b->get_global_position();
			d->chain.write[i].dynamic_position = bone_pos;
			d->chain.write[i].last_position = bone_pos;
			d->chain.write[i].last_noncollision_position = bone_pos;
			d->chain.write[i].velocity = Vector2(0, 0);
			d->chain.write[i].acceleration = Vector2(0, 0);
			d->chain.write[i].force = Vector2(0, 0);
		}
	}
}

void SkeletonModification2D::set_stiffness(float p_stiffness) {
	ERR_FAIL_COND_MSG(p_stiffness < 0, "Stiffness cannot be set to a negative value!");
	_ensure_mode(MODIFICATION_JIGGLE);
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	d->stiffness = p_stiffness;
	_update_jiggle_joint_data();
}

float SkeletonModification2D::get_stiffness() const {
	const JiggleData *d = _get_jiggle();
	return d ? d->stiffness : 3.0f;
}

void SkeletonModification2D::set_mass(float p_mass) {
	ERR_FAIL_COND_MSG(p_mass < 0, "Mass cannot be set to a negative value!");
	_ensure_mode(MODIFICATION_JIGGLE);
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	d->mass = p_mass;
	_update_jiggle_joint_data();
}

float SkeletonModification2D::get_mass() const {
	const JiggleData *d = _get_jiggle();
	return d ? d->mass : 0.75f;
}

void SkeletonModification2D::set_damping(float p_damping) {
	ERR_FAIL_COND_MSG(p_damping < 0, "Damping cannot be set to a negative value!");
	ERR_FAIL_COND_MSG(p_damping > 1, "Damping cannot be more than one!");
	_ensure_mode(MODIFICATION_JIGGLE);
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	d->damping = p_damping;
	_update_jiggle_joint_data();
}

float SkeletonModification2D::get_damping() const {
	const JiggleData *d = _get_jiggle();
	return d ? d->damping : 0.75f;
}

void SkeletonModification2D::set_use_gravity(bool p_use_gravity) {
	_ensure_mode(MODIFICATION_JIGGLE);
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	d->use_gravity = p_use_gravity;
	_update_jiggle_joint_data();
}

bool SkeletonModification2D::get_use_gravity() const {
	const JiggleData *d = _get_jiggle();
	return d ? d->use_gravity : false;
}

void SkeletonModification2D::set_gravity(Vector2 p_gravity) {
	_ensure_mode(MODIFICATION_JIGGLE);
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	d->gravity = p_gravity;
	_update_jiggle_joint_data();
}

Vector2 SkeletonModification2D::get_gravity() const {
	const JiggleData *d = _get_jiggle();
	return d ? d->gravity : Vector2(0, 6.0f);
}

void SkeletonModification2D::set_use_colliders(bool p_use_colliders) {
	_ensure_mode(MODIFICATION_JIGGLE);
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	d->use_colliders = p_use_colliders;
}

bool SkeletonModification2D::get_use_colliders() const {
	const JiggleData *d = _get_jiggle();
	return d ? d->use_colliders : false;
}

void SkeletonModification2D::set_collision_mask(int p_mask) {
	_ensure_mode(MODIFICATION_JIGGLE);
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	d->collision_mask = uint32_t(p_mask);
}

int SkeletonModification2D::get_collision_mask() const {
	const JiggleData *d = _get_jiggle();
	return d ? int(d->collision_mask) : 1;
}

int SkeletonModification2D::get_jiggle_data_chain_length() const {
	const JiggleData *d = _get_jiggle();
	return d ? d->chain.size() : 0;
}

void SkeletonModification2D::set_jiggle_data_chain_length(int p_new_length) {
	ERR_FAIL_COND(p_new_length < 0);
	_ensure_mode(MODIFICATION_JIGGLE);
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	d->chain.resize(p_new_length);
	_update_jiggle_joint_data();
}

void SkeletonModification2D::set_jiggle_joint_bone2d_node(int p_joint_idx, const NodePath &p_target_node) {
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "Jiggle joint out of range!");
	d->chain.write[p_joint_idx].bone2d_node = p_target_node;
	if (is_setup) {
		_resolve_bone_2d(p_target_node, d->chain.write[p_joint_idx].bone_idx);
	}
}

NodePath SkeletonModification2D::get_jiggle_joint_bone2d_node(int p_joint_idx) const {
	const JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL_V(d, NodePath());
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), NodePath(), "Jiggle joint out of range!");
	return d->chain[p_joint_idx].bone2d_node;
}

void SkeletonModification2D::set_jiggle_joint_bone_index(int p_joint_idx, int p_bone_idx) {
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "Jiggle joint out of range!");
	d->chain.write[p_joint_idx].bone_idx = p_bone_idx;
	if (is_setup && stack && stack->skeleton && p_bone_idx >= 0 && p_bone_idx < stack->skeleton->get_bone_count()) {
		Bone2D *b = stack->skeleton->get_bone(p_bone_idx);
		if (b) {
			d->chain.write[p_joint_idx].bone2d_node = stack->skeleton->get_path_to(b);
		}
	}
}

int SkeletonModification2D::get_jiggle_joint_bone_index(int p_joint_idx) const {
	const JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL_V(d, -1);
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), -1, "Jiggle joint out of range!");
	return d->chain[p_joint_idx].bone_idx;
}

void SkeletonModification2D::set_jiggle_joint_override(int p_joint_idx, bool p_override) {
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX(p_joint_idx, d->chain.size());
	d->chain.write[p_joint_idx].override_defaults = p_override;
	_update_jiggle_joint_data();
}

bool SkeletonModification2D::get_jiggle_joint_override(int p_joint_idx) const {
	const JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL_V(d, false);
	ERR_FAIL_INDEX_V(p_joint_idx, d->chain.size(), false);
	return d->chain[p_joint_idx].override_defaults;
}

void SkeletonModification2D::set_jiggle_joint_stiffness(int p_joint_idx, float p_stiffness) {
	ERR_FAIL_COND_MSG(p_stiffness < 0, "Stiffness cannot be set to a negative value!");
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX(p_joint_idx, d->chain.size());
	d->chain.write[p_joint_idx].stiffness = p_stiffness;
}

float SkeletonModification2D::get_jiggle_joint_stiffness(int p_joint_idx) const {
	const JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL_V(d, -1.0f);
	ERR_FAIL_INDEX_V(p_joint_idx, d->chain.size(), -1.0f);
	return d->chain[p_joint_idx].stiffness;
}

void SkeletonModification2D::set_jiggle_joint_mass(int p_joint_idx, float p_mass) {
	ERR_FAIL_COND_MSG(p_mass < 0, "Mass cannot be set to a negative value!");
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX(p_joint_idx, d->chain.size());
	d->chain.write[p_joint_idx].mass = p_mass;
}

float SkeletonModification2D::get_jiggle_joint_mass(int p_joint_idx) const {
	const JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL_V(d, -1.0f);
	ERR_FAIL_INDEX_V(p_joint_idx, d->chain.size(), -1.0f);
	return d->chain[p_joint_idx].mass;
}

void SkeletonModification2D::set_jiggle_joint_damping(int p_joint_idx, float p_damping) {
	ERR_FAIL_COND_MSG(p_damping < 0, "Damping cannot be set to a negative value!");
	ERR_FAIL_COND_MSG(p_damping > 1, "Damping cannot be more than one!");
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX(p_joint_idx, d->chain.size());
	d->chain.write[p_joint_idx].damping = p_damping;
}

float SkeletonModification2D::get_jiggle_joint_damping(int p_joint_idx) const {
	const JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL_V(d, -1.0f);
	ERR_FAIL_INDEX_V(p_joint_idx, d->chain.size(), -1.0f);
	return d->chain[p_joint_idx].damping;
}

void SkeletonModification2D::set_jiggle_joint_use_gravity(int p_joint_idx, bool p_use_gravity) {
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX(p_joint_idx, d->chain.size());
	d->chain.write[p_joint_idx].use_gravity = p_use_gravity;
}

bool SkeletonModification2D::get_jiggle_joint_use_gravity(int p_joint_idx) const {
	const JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL_V(d, false);
	ERR_FAIL_INDEX_V(p_joint_idx, d->chain.size(), false);
	return d->chain[p_joint_idx].use_gravity;
}

void SkeletonModification2D::set_jiggle_joint_gravity(int p_joint_idx, Vector2 p_gravity) {
	JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX(p_joint_idx, d->chain.size());
	d->chain.write[p_joint_idx].gravity = p_gravity;
}

Vector2 SkeletonModification2D::get_jiggle_joint_gravity(int p_joint_idx) const {
	const JiggleData *d = _get_jiggle();
	ERR_FAIL_NULL_V(d, Vector2(0, 0));
	ERR_FAIL_INDEX_V(p_joint_idx, d->chain.size(), Vector2(0, 0));
	return d->chain[p_joint_idx].gravity;
}

// --- TwoBoneIK accessors ---

void SkeletonModification2D::set_target_minimum_distance(float p_distance) {
	ERR_FAIL_COND_MSG(p_distance < 0, "Target minimum distance cannot be less than zero!");
	_ensure_mode(MODIFICATION_TWOBONEIK);
	TwoBoneIKData *d = _get_twoboneik();
	ERR_FAIL_NULL(d);
	d->target_minimum_distance = p_distance;
}

float SkeletonModification2D::get_target_minimum_distance() const {
	const TwoBoneIKData *d = _get_twoboneik();
	return d ? d->target_minimum_distance : 0.0f;
}

void SkeletonModification2D::set_target_maximum_distance(float p_distance) {
	ERR_FAIL_COND_MSG(p_distance < 0, "Target maximum distance cannot be less than zero!");
	_ensure_mode(MODIFICATION_TWOBONEIK);
	TwoBoneIKData *d = _get_twoboneik();
	ERR_FAIL_NULL(d);
	d->target_maximum_distance = p_distance;
}

float SkeletonModification2D::get_target_maximum_distance() const {
	const TwoBoneIKData *d = _get_twoboneik();
	return d ? d->target_maximum_distance : 0.0f;
}

void SkeletonModification2D::set_flip_bend_direction(bool p_flip_direction) {
	_ensure_mode(MODIFICATION_TWOBONEIK);
	TwoBoneIKData *d = _get_twoboneik();
	ERR_FAIL_NULL(d);
	d->flip_bend_direction = p_flip_direction;
#ifdef TOOLS_ENABLED
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
#endif // TOOLS_ENABLED
}

bool SkeletonModification2D::get_flip_bend_direction() const {
	const TwoBoneIKData *d = _get_twoboneik();
	return d ? d->flip_bend_direction : false;
}

void SkeletonModification2D::set_joint_one_bone2d_node(const NodePath &p_node) {
	_ensure_mode(MODIFICATION_TWOBONEIK);
	TwoBoneIKData *d = _get_twoboneik();
	ERR_FAIL_NULL(d);
	d->joint_one_bone2d_node = p_node;
	if (is_setup) {
		_resolve_bone_2d(p_node, d->joint_one_bone_idx);
	}
}

NodePath SkeletonModification2D::get_joint_one_bone2d_node() const {
	const TwoBoneIKData *d = _get_twoboneik();
	return d ? d->joint_one_bone2d_node : NodePath();
}

void SkeletonModification2D::set_joint_one_bone_idx(int p_bone_idx) {
	_ensure_mode(MODIFICATION_TWOBONEIK);
	TwoBoneIKData *d = _get_twoboneik();
	ERR_FAIL_NULL(d);
	d->joint_one_bone_idx = p_bone_idx;
	if (is_setup && stack && stack->skeleton && p_bone_idx >= 0 && p_bone_idx < stack->skeleton->get_bone_count()) {
		Bone2D *b = stack->skeleton->get_bone(p_bone_idx);
		if (b) {
			d->joint_one_bone2d_node = stack->skeleton->get_path_to(b);
		}
	}
}

int SkeletonModification2D::get_joint_one_bone_idx() const {
	const TwoBoneIKData *d = _get_twoboneik();
	return d ? d->joint_one_bone_idx : -1;
}

void SkeletonModification2D::set_joint_two_bone2d_node(const NodePath &p_node) {
	_ensure_mode(MODIFICATION_TWOBONEIK);
	TwoBoneIKData *d = _get_twoboneik();
	ERR_FAIL_NULL(d);
	d->joint_two_bone2d_node = p_node;
	if (is_setup) {
		_resolve_bone_2d(p_node, d->joint_two_bone_idx);
	}
}

NodePath SkeletonModification2D::get_joint_two_bone2d_node() const {
	const TwoBoneIKData *d = _get_twoboneik();
	return d ? d->joint_two_bone2d_node : NodePath();
}

void SkeletonModification2D::set_joint_two_bone_idx(int p_bone_idx) {
	_ensure_mode(MODIFICATION_TWOBONEIK);
	TwoBoneIKData *d = _get_twoboneik();
	ERR_FAIL_NULL(d);
	d->joint_two_bone_idx = p_bone_idx;
	if (is_setup && stack && stack->skeleton && p_bone_idx >= 0 && p_bone_idx < stack->skeleton->get_bone_count()) {
		Bone2D *b = stack->skeleton->get_bone(p_bone_idx);
		if (b) {
			d->joint_two_bone2d_node = stack->skeleton->get_path_to(b);
		}
	}
}

int SkeletonModification2D::get_joint_two_bone_idx() const {
	const TwoBoneIKData *d = _get_twoboneik();
	return d ? d->joint_two_bone_idx : -1;
}

#ifdef TOOLS_ENABLED
void SkeletonModification2D::set_editor_draw_min_max(bool p_draw) {
	_ensure_mode(MODIFICATION_TWOBONEIK);
	TwoBoneIKData *d = _get_twoboneik();
	ERR_FAIL_NULL(d);
	d->editor_draw_min_max = p_draw;
	if (stack && is_setup) {
		stack->set_editor_gizmos_dirty(true);
	}
}

bool SkeletonModification2D::get_editor_draw_min_max() const {
	const TwoBoneIKData *d = _get_twoboneik();
	return d ? d->editor_draw_min_max : false;
}
#endif // TOOLS_ENABLED

// --- PhysicalBones accessors ---

int SkeletonModification2D::get_physical_bone_chain_length() const {
	const PhysicalBonesData *d = _get_physicalbones();
	return d ? d->chain.size() : 0;
}

void SkeletonModification2D::set_physical_bone_chain_length(int p_new_length) {
	ERR_FAIL_COND(p_new_length < 0);
	_ensure_mode(MODIFICATION_PHYSICALBONES);
	PhysicalBonesData *d = _get_physicalbones();
	ERR_FAIL_NULL(d);
	d->chain.resize(p_new_length);
}

void SkeletonModification2D::set_physical_bone_node(int p_joint_idx, const NodePath &p_nodepath) {
	PhysicalBonesData *d = _get_physicalbones();
	ERR_FAIL_NULL(d);
	ERR_FAIL_INDEX_MSG(p_joint_idx, d->chain.size(), "Joint index out of range!");
	d->chain.write[p_joint_idx].physical_bone_node = p_nodepath;
	if (is_setup) {
		_resolve_bone_2d(p_nodepath, d->chain.write[p_joint_idx].bone_idx);
	}
}

NodePath SkeletonModification2D::get_physical_bone_node(int p_joint_idx) const {
	const PhysicalBonesData *d = _get_physicalbones();
	ERR_FAIL_NULL_V(d, NodePath());
	ERR_FAIL_INDEX_V_MSG(p_joint_idx, d->chain.size(), NodePath(), "Joint index out of range!");
	return d->chain[p_joint_idx].physical_bone_node;
}

void SkeletonModification2D::fetch_physical_bones() {
	_ensure_mode(MODIFICATION_PHYSICALBONES);
	PhysicalBonesData *d = _get_physicalbones();
	if (!d || !stack || !stack->skeleton) {
		return;
	}
	d->chain.clear();
	for (int i = 0; i < stack->skeleton->get_bone_count(); i++) {
		Bone2D *b = stack->skeleton->get_bone(i);
		if (!b) {
			continue;
		}
		PhysicalBonesData::JointData jd;
		jd.physical_bone_node = stack->skeleton->get_path_to(b);
		jd.bone_idx = i;
		d->chain.push_back(jd);
	}
}

void SkeletonModification2D::start_simulation(const Vector<String> &p_bones) {
	_ensure_mode(MODIFICATION_PHYSICALBONES);
	PhysicalBonesData *d = _get_physicalbones();
	ERR_FAIL_NULL(d);
	d->simulation_active = true;
	for (int i = 0; i < d->chain.size(); i++) {
		if (p_bones.is_empty()) {
			d->chain.write[i].simulating = true;
		} else {
			Node2D *n = _resolve_node_2d(d->chain[i].physical_bone_node);
			d->chain.write[i].simulating = (n && p_bones.has(n->get_name()));
		}
	}
}

void SkeletonModification2D::stop_simulation(const Vector<String> &p_bones) {
	PhysicalBonesData *d = _get_physicalbones();
	if (!d) {
		return;
	}
	if (p_bones.is_empty()) {
		d->simulation_active = false;
		for (int i = 0; i < d->chain.size(); i++) {
			d->chain.write[i].simulating = false;
		}
		return;
	}
	bool any_active = false;
	for (int i = 0; i < d->chain.size(); i++) {
		Node2D *n = _resolve_node_2d(d->chain[i].physical_bone_node);
		if (n && p_bones.has(n->get_name())) {
			d->chain.write[i].simulating = false;
		}
		if (d->chain[i].simulating) {
			any_active = true;
		}
	}
	d->simulation_active = any_active;
}

// --- StackHolder accessors ---

void SkeletonModification2D::set_held_modification_stack(Ref<SkeletonModificationStack2D> p_held_stack) {
	_ensure_mode(MODIFICATION_STACKHOLDER);
	StackHolderData *d = _get_stackholder();
	ERR_FAIL_NULL(d);
	d->held_modification_stack = p_held_stack;
	if (is_setup && stack && d->held_modification_stack.is_valid()) {
		d->held_modification_stack->set_skeleton(stack->get_skeleton());
		d->held_modification_stack->setup();
	}
}

Ref<SkeletonModificationStack2D> SkeletonModification2D::get_held_modification_stack() const {
	const StackHolderData *d = _get_stackholder();
	return d ? d->held_modification_stack : Ref<SkeletonModificationStack2D>();
}

// --- skeleton_modification_stack_2d.cpp ---

#include "scene/resources/2d/skeleton/skeleton_modification_2d.h"

#include "scene/2d/sprite_2d.h"
#include "scene/resources/2d/skeleton/skeleton_modification_2d.h"

void SkeletonModificationStack2D::setup() {
	if (is_setup) {
		return;
	}

	if (skeleton != nullptr) {
		is_setup = true;
		for (int i = 0; i < modifications.size(); i++) {
			if (modifications[i].is_null()) {
				continue;
			}
			modifications.get(i)->_setup_modification(this);
		}

#ifdef TOOLS_ENABLED
		set_editor_gizmos_dirty(true);
#endif // TOOLS_ENABLED
	} else {
		WARN_PRINT("Cannot setup SkeletonModificationStack2D: no Skeleton2D set!");
	}
}

void SkeletonModificationStack2D::execute(float p_delta, int p_execution_mode) {
	if (!is_setup || !enabled || skeleton == nullptr) {
		return;
	}

	for (int i = 0; i < modifications.size(); i++) {
		if (modifications[i].is_null()) {
			continue;
		}
		if (modifications[i]->get_enabled() && modifications[i]->get_execution_mode() == p_execution_mode) {
			modifications.get(i)->_execute(p_delta);
		}
	}
}

void SkeletonModificationStack2D::draw_editor_gizmos() {
	if (!is_setup || !skeleton) {
		return;
	}

	if (editor_gizmo_dirty) {
		for (int i = 0; i < modifications.size(); i++) {
			if (modifications[i].is_null()) {
				continue;
			}
			if (modifications[i]->editor_draw_gizmo) {
				modifications.get(i)->_draw_editor_gizmo();
			}
		}
		skeleton->draw_set_transform(Vector2(0, 0));
		editor_gizmo_dirty = false;
	}
}

void SkeletonModificationStack2D::enable_all_modifications(bool p_enabled) {
	for (int i = 0; i < modifications.size(); i++) {
		if (modifications[i].is_null()) {
			continue;
		}
		modifications.get(i)->set_enabled(p_enabled);
	}
}

Ref<SkeletonModification2D> SkeletonModificationStack2D::get_modification(int p_mod_idx) const {
	ERR_FAIL_INDEX_V(p_mod_idx, modifications.size(), nullptr);
	return modifications[p_mod_idx];
}

void SkeletonModificationStack2D::add_modification(Ref<SkeletonModification2D> p_mod) {
	ERR_FAIL_COND(p_mod.is_null());

	p_mod->_setup_modification(this);
	modifications.push_back(p_mod);

#ifdef TOOLS_ENABLED
	set_editor_gizmos_dirty(true);
#endif // TOOLS_ENABLED
}

void SkeletonModificationStack2D::delete_modification(int p_mod_idx) {
	ERR_FAIL_INDEX(p_mod_idx, modifications.size());
	modifications.remove_at(p_mod_idx);

#ifdef TOOLS_ENABLED
	set_editor_gizmos_dirty(true);
#endif // TOOLS_ENABLED
}

void SkeletonModificationStack2D::set_modification(int p_mod_idx, Ref<SkeletonModification2D> p_mod) {
	ERR_FAIL_INDEX(p_mod_idx, modifications.size());

	if (p_mod.is_null()) {
		modifications.write[p_mod_idx] = Ref<SkeletonModification2D>();
	} else {
		modifications.write[p_mod_idx] = p_mod;
		p_mod->_setup_modification(this);
	}

#ifdef TOOLS_ENABLED
	set_editor_gizmos_dirty(true);
#endif // TOOLS_ENABLED
}

void SkeletonModificationStack2D::set_modification_count(int p_count) {
	ERR_FAIL_COND_MSG(p_count < 0, "SkeletonModificationStack2D modification count cannot be less than zero.");
	modifications.resize(p_count);
#ifdef TOOLS_ENABLED
	set_editor_gizmos_dirty(true);
#endif // TOOLS_ENABLED
}

int SkeletonModificationStack2D::get_modification_count() const {
	return modifications.size();
}

void SkeletonModificationStack2D::set_skeleton(Skeleton2D *p_skeleton) {
	skeleton = p_skeleton;
}

Skeleton2D *SkeletonModificationStack2D::get_skeleton() const {
	return skeleton;
}

bool SkeletonModificationStack2D::get_is_setup() const {
	return is_setup;
}

void SkeletonModificationStack2D::set_enabled(bool p_enabled) {
	enabled = p_enabled;
}

bool SkeletonModificationStack2D::get_enabled() const {
	return enabled;
}

void SkeletonModificationStack2D::set_editor_gizmos_dirty(bool p_dirty) {
	if (!is_setup) {
		return;
	}
	editor_gizmo_dirty = p_dirty;
	if (skeleton && editor_gizmo_dirty) {
		skeleton->queue_redraw();
	}
}

void SkeletonModificationStack2D::set_strength(float p_strength) {
	ERR_FAIL_COND_MSG(p_strength < 0, "Strength cannot be less than zero!");
	ERR_FAIL_COND_MSG(p_strength > 1, "Strength cannot be more than one!");
	strength = p_strength;
}

float SkeletonModificationStack2D::get_strength() const {
	return strength;
}

SkeletonModificationStack2D::SkeletonModificationStack2D() {}

void Skeleton2D::execute_modifications(real_t p_delta, int p_execution_mode) {
	if (modification_stack.is_null()) {
		return;
	}
	if (bone_setup_dirty) {
		_update_bone_setup();
	}
	modification_stack->execute(p_delta, p_execution_mode);
}
