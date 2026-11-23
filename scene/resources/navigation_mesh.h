/**************************************************************************/
/*  navigation_mesh.h                                                     */
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

#pragma once

#include "core/os/mutex.h"
#include "scene/resources/mesh.h"

class NavigationMesh : public Resource
{
	RWLock rwlock;

	Vector<Vector3> vertices;
	Vector<Vector<int>> polygons;
	Ref<ArrayMesh> debug_mesh;

protected:

public:
	enum SamplePartitionType
	{
		SAMPLE_PARTITION_WATERSHED = 0,
		SAMPLE_PARTITION_MONOTONE,
		SAMPLE_PARTITION_LAYERS,
		SAMPLE_PARTITION_MAX
	};

	enum ParsedGeometryType
	{
		PARSED_GEOMETRY_MESH_INSTANCES = 0,
		PARSED_GEOMETRY_STATIC_COLLIDERS,
		PARSED_GEOMETRY_BOTH,
		PARSED_GEOMETRY_MAX
	};

	enum SourceGeometryMode
	{
		SOURCE_GEOMETRY_ROOT_NODE_CHILDREN = 0,
		SOURCE_GEOMETRY_GROUPS_WITH_CHILDREN,
		SOURCE_GEOMETRY_GROUPS_EXPLICIT,
		SOURCE_GEOMETRY_MAX
	};

protected:
	float cell_size = 0.25f;
	float cell_height = 0.25f;
	float border_size = 0.0f;
	float agent_height = 1.5f;
	float agent_radius = 0.5f;
	float agent_max_climb = 0.25f;
	float agent_max_slope = 45.0f;
	float region_min_size = 2.0f;
	float region_merge_size = 20.0f;
	float edge_max_length = 0.0f;
	float edge_max_error = 1.3f;
	float vertices_per_polygon = 6.0f;
	float detail_sample_distance = 6.0f;
	float detail_sample_max_error = 1.0f;

	SamplePartitionType partition_type = SAMPLE_PARTITION_WATERSHED;
	ParsedGeometryType parsed_geometry_type = PARSED_GEOMETRY_BOTH;
	uint32_t collision_mask = 0xFFFFFFFF;

	SourceGeometryMode source_geometry_mode = SOURCE_GEOMETRY_ROOT_NODE_CHILDREN;
	String source_group_name = "navigation_mesh_source_group";

	bool filter_low_hanging_obstacles = false;
	bool filter_ledge_spans = false;
	bool filter_walkable_low_height_spans = false;
	AABB filter_baking_aabb;
	Vector3 filter_baking_aabb_offset;

public:
	NavigationMesh();

	// Recast settings
	void set_sample_partition_type(SamplePartitionType p_value);
	SamplePartitionType get_sample_partition_type() const;

	void set_parsed_geometry_type(ParsedGeometryType p_value);
	ParsedGeometryType get_parsed_geometry_type() const;

	void set_collision_mask(uint32_t p_mask);
	uint32_t get_collision_mask() const;

	void set_collision_mask_value(int p_layer_number, bool p_value);
	bool get_collision_mask_value(int p_layer_number) const;

	void set_source_geometry_mode(SourceGeometryMode p_geometry_mode);
	SourceGeometryMode get_source_geometry_mode() const;

	void set_source_group_name(const String& p_group_name);
	String get_source_group_name() const;

	void set_cell_size(float p_value);
	float get_cell_size() const;

	void set_cell_height(float p_value);
	float get_cell_height() const;

	void set_border_size(float p_value);
	float get_border_size() const;

	void set_agent_height(float p_value);
	float get_agent_height() const;

	void set_agent_radius(float p_value);
	float get_agent_radius();

	void set_agent_max_climb(float p_value);
	float get_agent_max_climb() const;

	void set_agent_max_slope(float p_value);
	float get_agent_max_slope() const;

	void set_region_min_size(float p_value);
	float get_region_min_size() const;

	void set_region_merge_size(float p_value);
	float get_region_merge_size() const;

	void set_edge_max_length(float p_value);
	float get_edge_max_length() const;

	void set_edge_max_error(float p_value);
	float get_edge_max_error() const;

	void set_vertices_per_polygon(float p_value);
	float get_vertices_per_polygon() const;

	void set_detail_sample_distance(float p_value);
	float get_detail_sample_distance() const;

	void set_detail_sample_max_error(float p_value);
	float get_detail_sample_max_error() const;

	void set_filter_low_hanging_obstacles(bool p_value);
	bool get_filter_low_hanging_obstacles() const;

	void set_filter_ledge_spans(bool p_value);
	bool get_filter_ledge_spans() const;

	void set_filter_walkable_low_height_spans(bool p_value);
	bool get_filter_walkable_low_height_spans() const;

	void set_filter_baking_aabb(const AABB& p_aabb);
	AABB get_filter_baking_aabb() const;

	void set_filter_baking_aabb_offset(const Vector3& p_aabb_offset);
	Vector3 get_filter_baking_aabb_offset() const;

	void create_from_mesh(const Ref<Mesh>& p_mesh);

	void set_vertices(const Vector<Vector3>& p_vertices);
	Vector<Vector3> get_vertices() const;

	void add_polygon(const Vector<int>& p_polygon);
	int get_polygon_count() const;
	Vector<int> get_polygon(int p_idx);
	void clear_polygons();
	void set_polygons(const Vector<Vector<int>>& p_polygons);
	Vector<Vector<int>> get_polygons() const;

	void clear();

	void set_data(const Vector<Vector3>& p_vertices, const Vector<Vector<int>>& p_polygons);
	void get_data(Vector<Vector3>& r_vertices, Vector<Vector<int>>& r_polygons);

#ifdef DEBUG_ENABLED
	Ref<ArrayMesh> get_debug_mesh();
#endif // DEBUG_ENABLED
};

// --- Folded from scene/resources/3d/importer_mesh.h ---
#include "core/io/resource.h"
#include "scene/resources/mesh.h"

#include "drivers/gles3/rasterizer_gles3.h"

#ifndef PHYSICS_3D_DISABLED
#include "scene/resources/3d/shape_3d.h"
#endif // PHYSICS_3D_DISABLED

// The following classes are used by importers instead of ArrayMesh and MeshInstance3D
// so the data is not registered (hence, quality loss), importing happens faster and
// its easier to modify before saving

using BlendShape = GLES3::Mesh::Surface::BlendShape;

class ImporterMesh : public Resource
{
	struct Surface
	{
		Mesh::PrimitiveType primitive;

		Vector<BlendShape> blend_shape_data;

		struct LOD
		{
			Vector<int> indices;
			float distance = 0.0f;
		};

		Vector<LOD> lods;
		Ref<Material> material;
		String name;
		uint64_t flags = 0;

		struct LODComparator
		{
			_FORCE_INLINE_ bool operator()(const LOD& l, const LOD& r) const
			{
				return l.distance < r.distance;
			}
		};
	};

	Vector<Surface> surfaces;
	Vector<String> blend_shapes;
	Mesh::BlendShapeMode blend_shape_mode = Mesh::BLEND_SHAPE_MODE_NORMALIZED;

	Ref<ArrayMesh> mesh;

	Ref<ImporterMesh> shadow_mesh;

	Size2i lightmap_size_hint;

protected:

public:
	void add_blend_shape(const String& p_name);
	int get_blend_shape_count() const;
	String get_blend_shape_name(int p_blend_shape) const;

	static String validate_blend_shape_name(const String& p_name);

	int get_surface_count() const;

	void set_blend_shape_mode(Mesh::BlendShapeMode p_blend_shape_mode);
	Mesh::BlendShapeMode get_blend_shape_mode() const;

	Mesh::PrimitiveType get_surface_primitive_type(int p_surface);
	String get_surface_name(int p_surface) const;
	void set_surface_name(int p_surface, const String& p_name);
	int get_surface_lod_count(int p_surface) const;
	Vector<int> get_surface_lod_indices(int p_surface, int p_lod) const;
	float get_surface_lod_size(int p_surface, int p_lod) const;
	Ref<Material> get_surface_material(int p_surface) const;
	uint64_t get_surface_format(int p_surface) const;

	void set_surface_material(int p_surface, const Ref<Material>& p_material);

	void optimize_indices();

	void create_shadow_mesh();
	Ref<ImporterMesh> get_shadow_mesh() const;

	Vector<Face3> get_faces() const;
#ifndef PHYSICS_3D_DISABLED
	Vector<Ref<Shape3D>> convex_decompose(
		const Ref<MeshConvexDecompositionSettings>& p_settings) const;
	Ref<Shape3D> create_convex_shape(
		bool p_clean = true, bool p_simplify = false) const;
	Ref<Shape3D> create_trimesh_shape() const;
#endif // PHYSICS_3D_DISABLED
	Ref<NavigationMesh> create_navigation_mesh();
	Error lightmap_unwrap_cached(const Transform3D& p_base_transform, float p_texel_size,
		const Vector<uint8_t>& p_src_cache, Vector<uint8_t>& r_dst_cache);

	void set_lightmap_size_hint(const Size2i& p_size);
	Size2i get_lightmap_size_hint() const;

	bool has_mesh() const;
	Ref<ArrayMesh> get_mesh(const Ref<ArrayMesh>& p_base = Ref<ArrayMesh>());
	static Ref<ImporterMesh> from_mesh(const Ref<Mesh>& p_mesh);
	void clear();
};
// --- End scene/resources/3d/importer_mesh.h ---
