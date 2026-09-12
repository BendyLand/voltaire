/**************************************************************************/
/*  tile_set.cpp                                                          */
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
#include "core/templates/local_vector.h"
#include "core/templates/rb_set.h"
#include "scene/gui/control.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/mesh.h"
#include "servers/rendering/rendering_server.h"
#include "tile_set.compat.inc"
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

/////////////////////////////// TileSet //////////////////////////////////////

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

// Rendering
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
// Navigation
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

/////////////////////////////// TileSetSource //////////////////////////////////////

void TileSetSource::set_tile_set(const TileSet* p_tile_set) { tile_set = p_tile_set; }

TileSet* TileSetSource::get_tile_set() const { return (TileSet*)tile_set; }

void TileSetSource::reset_state() { tile_set = nullptr; }


/////////////////////////////// TileSetAtlasSource //////////////////////////////////////

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

Ref<ConvexPolygonShape2D> TileData::get_collision_polygon_shape(int p_layer_id, int p_polygon_index,
	int shape_index, bool p_flip_h, bool p_flip_v, bool p_transpose) const
{
	ERR_FAIL_INDEX_V(p_layer_id, physics.size(), Ref<ConvexPolygonShape2D>());
	ERR_FAIL_INDEX_V(
		p_polygon_index, physics[p_layer_id].polygons.size(), Ref<ConvexPolygonShape2D>());
	ERR_FAIL_INDEX_V(shape_index, (int)physics[p_layer_id].polygons[p_polygon_index].shapes.size(),
		Ref<ConvexPolygonShape2D>());

	const PhysicsLayerTileData& layer_tile_data = physics[p_layer_id];
	const PhysicsLayerTileData::PolygonShapeTileData& shapes_data =
		layer_tile_data.polygons[p_polygon_index];

	int key = int(p_flip_h) | int(p_flip_v) << 1 | int(p_transpose) << 2;
	if (key == 0) {
		return shapes_data.shapes[shape_index];
	}
	if (shapes_data.shapes[shape_index].is_null()) {
		return Ref<ConvexPolygonShape2D>();
	}

	HashMap<int, LocalVector<Ref<ConvexPolygonShape2D>>>::Iterator I =

		shapes_data.transformed_shapes.find(key);
	if (!I) {
		int size = shapes_data.shapes.size();
		shapes_data.transformed_shapes[key].resize(size);
		for (int i = 0; i < size; i++) {
			Ref<ConvexPolygonShape2D> transformed_polygon;
			transformed_polygon.instantiate();
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


