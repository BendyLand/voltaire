/**************************************************************************/
/*  navigation_mesh.cpp                                                   */
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

#include "navigation_mesh.h"

#ifdef DEBUG_ENABLED
#include "servers/navigation_3d/navigation_server_3d.h"
#endif // DEBUG_ENABLED

void NavigationMesh::set_sample_partition_type(SamplePartitionType p_value)
{
	ERR_FAIL_INDEX(p_value, SAMPLE_PARTITION_MAX);
	partition_type = p_value;
}

NavigationMesh::SamplePartitionType NavigationMesh::get_sample_partition_type() const
{
	return partition_type;
}

NavigationMesh::ParsedGeometryType NavigationMesh::get_parsed_geometry_type() const
{
	return parsed_geometry_type;
}

void NavigationMesh::set_collision_mask(uint32_t p_mask) { collision_mask = p_mask; }

uint32_t NavigationMesh::get_collision_mask() const { return collision_mask; }

void NavigationMesh::set_collision_mask_value(int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 32, "Collision layer number must be between 1 and 32 inclusive.");
	uint32_t mask = get_collision_mask();
	if (p_value) {
		mask |= 1 << (p_layer_number - 1);
	}
	else {
		mask &= ~(1 << (p_layer_number - 1));
	}
	set_collision_mask(mask);
}

bool NavigationMesh::get_collision_mask_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Collision layer number must be between 1 and 32 inclusive.");
	return get_collision_mask() & (1 << (p_layer_number - 1));
}

NavigationMesh::SourceGeometryMode NavigationMesh::get_source_geometry_mode() const
{
	return source_geometry_mode;
}

void NavigationMesh::set_source_group_name(const StringName& p_group_name)
{
	source_group_name = p_group_name;
}

StringName NavigationMesh::get_source_group_name() const { return source_group_name; }

void NavigationMesh::set_cell_size(float p_value)
{
	ERR_FAIL_COND(p_value <= 0);
	cell_size = p_value;
}

float NavigationMesh::get_cell_size() const { return cell_size; }

void NavigationMesh::set_cell_height(float p_value)
{
	ERR_FAIL_COND(p_value <= 0);
	cell_height = p_value;
}

float NavigationMesh::get_cell_height() const { return cell_height; }

void NavigationMesh::set_border_size(float p_value)
{
	ERR_FAIL_COND(p_value < 0);
	border_size = p_value;
}

float NavigationMesh::get_border_size() const { return border_size; }

void NavigationMesh::set_agent_height(float p_value)
{
	ERR_FAIL_COND(p_value < 0);
	agent_height = p_value;
}

float NavigationMesh::get_agent_height() const { return agent_height; }

void NavigationMesh::set_agent_radius(float p_value)
{
	ERR_FAIL_COND(p_value < 0);
	agent_radius = p_value;
}

float NavigationMesh::get_agent_radius() { return agent_radius; }

void NavigationMesh::set_agent_max_climb(float p_value)
{
	ERR_FAIL_COND(p_value < 0);
	agent_max_climb = p_value;
}

float NavigationMesh::get_agent_max_climb() const { return agent_max_climb; }

void NavigationMesh::set_agent_max_slope(float p_value)
{
	ERR_FAIL_COND(p_value < 0 || p_value > 90);
	agent_max_slope = p_value;
}

float NavigationMesh::get_agent_max_slope() const { return agent_max_slope; }

void NavigationMesh::set_region_min_size(float p_value)
{
	ERR_FAIL_COND(p_value < 0);
	region_min_size = p_value;
}

float NavigationMesh::get_region_min_size() const { return region_min_size; }

void NavigationMesh::set_region_merge_size(float p_value)
{
	ERR_FAIL_COND(p_value < 0);
	region_merge_size = p_value;
}

float NavigationMesh::get_region_merge_size() const { return region_merge_size; }

void NavigationMesh::set_edge_max_length(float p_value)
{
	ERR_FAIL_COND(p_value < 0);
	edge_max_length = p_value;
}

float NavigationMesh::get_edge_max_length() const { return edge_max_length; }

void NavigationMesh::set_edge_max_error(float p_value)
{
	ERR_FAIL_COND(p_value < 0);
	edge_max_error = p_value;
}

float NavigationMesh::get_edge_max_error() const { return edge_max_error; }

void NavigationMesh::set_vertices_per_polygon(float p_value)
{
	ERR_FAIL_COND(p_value < 3);
	vertices_per_polygon = p_value;
}

float NavigationMesh::get_vertices_per_polygon() const { return vertices_per_polygon; }

void NavigationMesh::set_detail_sample_distance(float p_value)
{
	ERR_FAIL_COND(p_value < 0.1);
	detail_sample_distance = p_value;
}

float NavigationMesh::get_detail_sample_distance() const { return detail_sample_distance; }

void NavigationMesh::set_detail_sample_max_error(float p_value)
{
	ERR_FAIL_COND(p_value < 0);
	detail_sample_max_error = p_value;
}

float NavigationMesh::get_detail_sample_max_error() const { return detail_sample_max_error; }

void NavigationMesh::set_filter_low_hanging_obstacles(bool p_value)
{
	filter_low_hanging_obstacles = p_value;
}

bool NavigationMesh::get_filter_low_hanging_obstacles() const
{
	return filter_low_hanging_obstacles;
}

void NavigationMesh::set_filter_ledge_spans(bool p_value) { filter_ledge_spans = p_value; }

bool NavigationMesh::get_filter_ledge_spans() const { return filter_ledge_spans; }

void NavigationMesh::set_filter_walkable_low_height_spans(bool p_value)
{
	filter_walkable_low_height_spans = p_value;
}

bool NavigationMesh::get_filter_walkable_low_height_spans() const
{
	return filter_walkable_low_height_spans;
}

void NavigationMesh::set_filter_baking_aabb(const AABB& p_aabb)
{
	filter_baking_aabb = p_aabb;
	emit_changed();
}

AABB NavigationMesh::get_filter_baking_aabb() const { return filter_baking_aabb; }

void NavigationMesh::set_filter_baking_aabb_offset(const Vector3& p_aabb_offset)
{
	filter_baking_aabb_offset = p_aabb_offset;
	emit_changed();
}

Vector3 NavigationMesh::get_filter_baking_aabb_offset() const { return filter_baking_aabb_offset; }

Vector<Vector3> NavigationMesh::get_vertices() const
{
	RWLockRead read_lock(rwlock);
	return vertices;
}

Vector<Vector<int>> NavigationMesh::get_polygons() const
{
	RWLockRead read_lock(rwlock);
	return polygons;
}

int NavigationMesh::get_polygon_count() const
{
	RWLockRead read_lock(rwlock);
	return polygons.size();
}

Vector<int> NavigationMesh::get_polygon(int p_idx)
{
	RWLockRead read_lock(rwlock);
	ERR_FAIL_INDEX_V(p_idx, polygons.size(), Vector<int>());
	return polygons[p_idx];
}

void NavigationMesh::clear_polygons()
{
	RWLockWrite write_lock(rwlock);
	polygons.clear();
}

void NavigationMesh::clear()
{
	RWLockWrite write_lock(rwlock);
	polygons.clear();
	vertices.clear();
}

void NavigationMesh::set_data(
	const Vector<Vector3>& p_vertices, const Vector<Vector<int>>& p_polygons)
{
	RWLockWrite write_lock(rwlock);
	vertices = p_vertices;
	polygons = p_polygons;
}

void NavigationMesh::get_data(Vector<Vector3>& r_vertices, Vector<Vector<int>>& r_polygons)
{
	RWLockRead read_lock(rwlock);
	r_vertices = vertices;
	r_polygons = polygons;
}


