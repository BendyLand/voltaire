/**************************************************************************/
/*  mesh.h                                                                */
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

#include "core/io/resource.h"
#include "core/types.h"
#include "core/math/geometry_3d.h"
#include "scene/resources/material.h"
#include "servers/rendering/rendering_server_enums.h"
#include "servers/rendering/rendering_server_types.h"

#ifndef PHYSICS_3D_DISABLED
#include "scene/resources/3d/shape_3d.h"

class Shape3D;
#endif // PHYSICS_3D_DISABLED
class MeshConvexDecompositionSettings;

class Mesh : public Resource
{
	mutable Ref<TriangleMesh> triangle_mesh;				   // cached
	mutable Vector<Ref<TriangleMesh>> surface_triangle_meshes; // cached
	mutable Vector<Vector3> debug_lines;
	Size2i lightmap_size_hint;

	Vector<Vector3> _get_faces() const;

public:
	enum PrimitiveType
	{
		PRIMITIVE_POINTS = RSE::PRIMITIVE_POINTS,
		PRIMITIVE_LINES = RSE::PRIMITIVE_LINES,
		PRIMITIVE_LINE_STRIP = RSE::PRIMITIVE_LINE_STRIP,
		PRIMITIVE_TRIANGLES = RSE::PRIMITIVE_TRIANGLES,
		PRIMITIVE_TRIANGLE_STRIP = RSE::PRIMITIVE_TRIANGLE_STRIP,
		PRIMITIVE_MAX = RSE::PRIMITIVE_MAX,
	};

protected:

public:
	enum
	{
		NO_INDEX_ARRAY = RSE::NO_INDEX_ARRAY,
		ARRAY_WEIGHTS_SIZE = RSE::ARRAY_WEIGHTS_SIZE
	};

	enum BlendShapeMode
	{
		BLEND_SHAPE_MODE_NORMALIZED = RSE::BLEND_SHAPE_MODE_NORMALIZED,
		BLEND_SHAPE_MODE_RELATIVE = RSE::BLEND_SHAPE_MODE_RELATIVE,
	};

	enum ArrayType
	{
		ARRAY_VERTEX = RSE::ARRAY_VERTEX,
		ARRAY_NORMAL = RSE::ARRAY_NORMAL,
		ARRAY_TANGENT = RSE::ARRAY_TANGENT,
		ARRAY_COLOR = RSE::ARRAY_COLOR,
		ARRAY_TEX_UV = RSE::ARRAY_TEX_UV,
		ARRAY_TEX_UV2 = RSE::ARRAY_TEX_UV2,
		ARRAY_CUSTOM0 = RSE::ARRAY_CUSTOM0,
		ARRAY_CUSTOM1 = RSE::ARRAY_CUSTOM1,
		ARRAY_CUSTOM2 = RSE::ARRAY_CUSTOM2,
		ARRAY_CUSTOM3 = RSE::ARRAY_CUSTOM3,
		ARRAY_BONES = RSE::ARRAY_BONES,
		ARRAY_WEIGHTS = RSE::ARRAY_WEIGHTS,
		ARRAY_INDEX = RSE::ARRAY_INDEX,
		ARRAY_MAX = RSE::ARRAY_MAX

	};

	enum ArrayCustomFormat
	{
		ARRAY_CUSTOM_RGBA8_UNORM = RSE::ARRAY_CUSTOM_RGBA8_UNORM,
		ARRAY_CUSTOM_RGBA8_SNORM = RSE::ARRAY_CUSTOM_RGBA8_SNORM,
		ARRAY_CUSTOM_RG_HALF = RSE::ARRAY_CUSTOM_RG_HALF,
		ARRAY_CUSTOM_RGBA_HALF = RSE::ARRAY_CUSTOM_RGBA_HALF,
		ARRAY_CUSTOM_R_FLOAT = RSE::ARRAY_CUSTOM_R_FLOAT,
		ARRAY_CUSTOM_RG_FLOAT = RSE::ARRAY_CUSTOM_RG_FLOAT,
		ARRAY_CUSTOM_RGB_FLOAT = RSE::ARRAY_CUSTOM_RGB_FLOAT,
		ARRAY_CUSTOM_RGBA_FLOAT = RSE::ARRAY_CUSTOM_RGBA_FLOAT,
		ARRAY_CUSTOM_MAX = RSE::ARRAY_CUSTOM_MAX
	};

	enum ArrayFormat : uint64_t
	{
		ARRAY_FORMAT_VERTEX = RSE::ARRAY_FORMAT_VERTEX,
		ARRAY_FORMAT_NORMAL = RSE::ARRAY_FORMAT_NORMAL,
		ARRAY_FORMAT_TANGENT = RSE::ARRAY_FORMAT_TANGENT,
		ARRAY_FORMAT_COLOR = RSE::ARRAY_FORMAT_COLOR,
		ARRAY_FORMAT_TEX_UV = RSE::ARRAY_FORMAT_TEX_UV,
		ARRAY_FORMAT_TEX_UV2 = RSE::ARRAY_FORMAT_TEX_UV2,
		ARRAY_FORMAT_CUSTOM0 = RSE::ARRAY_FORMAT_CUSTOM0,
		ARRAY_FORMAT_CUSTOM1 = RSE::ARRAY_FORMAT_CUSTOM1,
		ARRAY_FORMAT_CUSTOM2 = RSE::ARRAY_FORMAT_CUSTOM2,
		ARRAY_FORMAT_CUSTOM3 = RSE::ARRAY_FORMAT_CUSTOM3,
		ARRAY_FORMAT_BONES = RSE::ARRAY_FORMAT_BONES,
		ARRAY_FORMAT_WEIGHTS = RSE::ARRAY_FORMAT_WEIGHTS,
		ARRAY_FORMAT_INDEX = RSE::ARRAY_FORMAT_INDEX,

		ARRAY_FORMAT_BLEND_SHAPE_MASK = RSE::ARRAY_FORMAT_BLEND_SHAPE_MASK,

		ARRAY_FORMAT_CUSTOM_BASE = RSE::ARRAY_FORMAT_CUSTOM_BASE,
		ARRAY_FORMAT_CUSTOM_BITS = RSE::ARRAY_FORMAT_CUSTOM_BITS,
		ARRAY_FORMAT_CUSTOM0_SHIFT = RSE::ARRAY_FORMAT_CUSTOM0_SHIFT,
		ARRAY_FORMAT_CUSTOM1_SHIFT = RSE::ARRAY_FORMAT_CUSTOM1_SHIFT,
		ARRAY_FORMAT_CUSTOM2_SHIFT = RSE::ARRAY_FORMAT_CUSTOM2_SHIFT,
		ARRAY_FORMAT_CUSTOM3_SHIFT = RSE::ARRAY_FORMAT_CUSTOM3_SHIFT,

		ARRAY_FORMAT_CUSTOM_MASK = RSE::ARRAY_FORMAT_CUSTOM_MASK,
		ARRAY_COMPRESS_FLAGS_BASE = RSE::ARRAY_COMPRESS_FLAGS_BASE,

		ARRAY_FLAG_USE_2D_VERTICES = RSE::ARRAY_FLAG_USE_2D_VERTICES,
		ARRAY_FLAG_USE_DYNAMIC_UPDATE = RSE::ARRAY_FLAG_USE_DYNAMIC_UPDATE,
		ARRAY_FLAG_USE_8_BONE_WEIGHTS = RSE::ARRAY_FLAG_USE_8_BONE_WEIGHTS,

		ARRAY_FLAG_USES_EMPTY_VERTEX_ARRAY = RSE::ARRAY_FLAG_USES_EMPTY_VERTEX_ARRAY,
		ARRAY_FLAG_COMPRESS_ATTRIBUTES = RSE::ARRAY_FLAG_COMPRESS_ATTRIBUTES,
		ARRAY_FLAG_USE_STORAGE_BUFFER = RSE::ARRAY_FLAG_USE_STORAGE_BUFFER,

		ARRAY_FLAG_FORMAT_VERSION_BASE = RSE::ARRAY_FLAG_FORMAT_VERSION_BASE,
		ARRAY_FLAG_FORMAT_VERSION_SHIFT = RSE::ARRAY_FLAG_FORMAT_VERSION_SHIFT,
		ARRAY_FLAG_FORMAT_VERSION_1 = RSE::ARRAY_FLAG_FORMAT_VERSION_1,
		ARRAY_FLAG_FORMAT_VERSION_2 = (uint64_t)RSE::ARRAY_FLAG_FORMAT_VERSION_2,
		ARRAY_FLAG_FORMAT_CURRENT_VERSION = (uint64_t)RSE::ARRAY_FLAG_FORMAT_CURRENT_VERSION,
		ARRAY_FLAG_FORMAT_VERSION_MASK = RSE::ARRAY_FLAG_FORMAT_VERSION_MASK,
	};

	virtual int get_surface_count() const;
	virtual int surface_get_array_len(int p_idx) const;
	virtual int surface_get_array_index_len(int p_idx) const;
	virtual uint32_t surface_get_format(int p_idx) const;
	virtual PrimitiveType surface_get_primitive_type(int p_idx) const;
	virtual void surface_set_material(int p_idx, const Ref<Material>& p_material);
	virtual Ref<Material> surface_get_material(int p_idx) const;
	virtual int get_blend_shape_count() const;
	virtual String get_blend_shape_name(int p_index) const;
	virtual void set_blend_shape_name(int p_index, const String& p_name);
	virtual AABB get_aabb() const;

	Vector<Face3> get_faces() const;
	Vector<Face3> get_surface_faces(int p_surface) const;
	Ref<TriangleMesh> generate_triangle_mesh() const;
	Ref<TriangleMesh> generate_surface_triangle_mesh(int p_surface) const;
	void generate_debug_mesh_lines(Vector<Vector3>& r_lines);
	void generate_debug_mesh_indices(Vector<Vector3>& r_points);

	Ref<Mesh> create_outline(float p_margin) const;

	void set_lightmap_size_hint(const Size2i& p_size);
	Size2i get_lightmap_size_hint() const;
	void clear_cache() const;

#ifndef PHYSICS_3D_DISABLED
	typedef Vector<Vector<Vector3>> (*ConvexDecompositionFunc)(const real_t* p_vertices,
		int p_vertex_count, const uint32_t* p_triangles, int p_triangle_count,
		const Ref<MeshConvexDecompositionSettings>& p_settings,
		Vector<Vector<uint32_t>>* r_convex_indices);

	static ConvexDecompositionFunc convex_decomposition_function;

	Vector<Ref<Shape3D>> convex_decompose(
		const Ref<MeshConvexDecompositionSettings>& p_settings) const;
	Ref<Shape3D> create_convex_shape(
		bool p_clean = true, bool p_simplify = false) const;
	Ref<Shape3D> create_trimesh_shape() const;
#endif // PHYSICS_3D_DISABLED

	virtual int get_builtin_bind_pose_count() const;
	virtual Transform3D get_builtin_bind_pose(int p_index) const;

	virtual Ref<Resource> create_placeholder() const;

	Mesh();
};

class MeshConvexDecompositionSettings : public RefCounted
{
public:
	enum Mode : int
	{
		CONVEX_DECOMPOSITION_MODE_VOXEL = 0,
		CONVEX_DECOMPOSITION_MODE_TETRAHEDRON = 1
	};

private:
	Mode mode = CONVEX_DECOMPOSITION_MODE_VOXEL;

	/// Maximum concavity. [Range: 0.0 -> 1.0]
	real_t max_concavity = 1.0;
	/// Controls the bias toward clipping along symmetry planes. [Range: 0.0 -> 1.0]
	real_t symmetry_planes_clipping_bias = 0.05;
	/// Controls the bias toward clipping along revolution axes. [Range: 0.0 -> 1.0]
	real_t revolution_axes_clipping_bias = 0.05;
	real_t min_volume_per_convex_hull = 0.0001;
	/// Maximum number of voxels generated during the voxelization stage.
	uint32_t resolution = 10'000;
	uint32_t max_num_vertices_per_convex_hull = 32;
	/// Controls the granularity of the search for the "best" clipping plane.
	/// [Range: 1 -> 16]
	uint32_t plane_downsampling = 4;
	/// Controls the precision of the convex-hull generation process during the
	/// clipping plane selection stage.
	/// [Range: 1 -> 16]
	uint32_t convex_hull_downsampling = 4;
	/// enable/disable normalizing the mesh before applying the convex decomposition.
	bool normalize_mesh = false;

	bool convex_hull_approximation = true;
	/// This is the maximum number of convex hulls to produce from the merge operation.
	uint32_t max_convex_hulls = 1;
	bool project_hull_vertices = true;

protected:

public:
	void set_max_concavity(real_t p_max_concavity);
	real_t get_max_concavity() const;

	void set_symmetry_planes_clipping_bias(real_t p_symmetry_planes_clipping_bias);
	real_t get_symmetry_planes_clipping_bias() const;

	void set_revolution_axes_clipping_bias(real_t p_revolution_axes_clipping_bias);
	real_t get_revolution_axes_clipping_bias() const;

	void set_min_volume_per_convex_hull(real_t p_min_volume_per_convex_hull);
	real_t get_min_volume_per_convex_hull() const;

	void set_resolution(uint32_t p_resolution);
	uint32_t get_resolution() const;

	void set_max_num_vertices_per_convex_hull(uint32_t p_max_num_vertices_per_convex_hull);
	uint32_t get_max_num_vertices_per_convex_hull() const;

	void set_plane_downsampling(uint32_t p_plane_downsampling);
	uint32_t get_plane_downsampling() const;

	void set_convex_hull_downsampling(uint32_t p_convex_hull_downsampling);
	uint32_t get_convex_hull_downsampling() const;

	void set_normalize_mesh(bool p_normalize_mesh);
	bool get_normalize_mesh() const;

	void set_mode(Mode p_mode);
	Mode get_mode() const;

	void set_convex_hull_approximation(bool p_convex_hull_approximation);
	bool get_convex_hull_approximation() const;

	void set_max_convex_hulls(uint32_t p_max_convex_hulls);
	uint32_t get_max_convex_hulls() const;

	void set_project_hull_vertices(bool p_project_hull_vertices);
	bool get_project_hull_vertices() const;
};

class ArrayMesh : public Mesh
{
	PackedStringArray _get_blend_shape_names() const;
	void _set_blend_shape_names(const PackedStringArray& p_names);

	Ref<ArrayMesh> shadow_mesh;

private:
	struct Surface
	{
		uint64_t format = 0;
		int array_length = 0;
		int index_array_length = 0;
		PrimitiveType primitive = PrimitiveType::PRIMITIVE_MAX;

		String name;
		AABB aabb;
		Ref<Material> material;
		bool is_2d = false;
	};

	Vector<Surface> surfaces;
	mutable RID mesh;
	AABB aabb;
	BlendShapeMode blend_shape_mode = BLEND_SHAPE_MODE_RELATIVE;
	Vector<String> blend_shapes;
	AABB custom_aabb;

	_FORCE_INLINE_ void _create_if_empty() const;
	void _recompute_aabb();

protected:
	virtual bool _is_generated() const { return false; }

	bool surface_index_0 = false;

	virtual void reset_state() override;


public:
	void add_surface(uint32_t p_format, PrimitiveType p_primitive,
		const Vector<uint8_t>& p_array, const Vector<uint8_t>& p_attribute_array,
		const Vector<uint8_t>& p_skin_array, int p_vertex_count,
		const Vector<uint8_t>& p_index_array, int p_index_count, const AABB& p_aabb,
		const Vector<uint8_t>& p_blend_shape_data = Vector<uint8_t>(),
		const Vector<AABB>& p_bone_aabbs = Vector<AABB>(),
		const Vector<RenderingServerTypes::SurfaceData::LOD>& p_lods =
			Vector<RenderingServerTypes::SurfaceData::LOD>(),
		const Vector4 p_uv_scale = Vector4());

	void add_blend_shape(const String& p_name);
	int get_blend_shape_count() const override;
	String get_blend_shape_name(int p_index) const override;
	void set_blend_shape_name(int p_index, const String& p_name) override;
	void clear_blend_shapes();

	void set_blend_shape_mode(BlendShapeMode p_mode);
	BlendShapeMode get_blend_shape_mode() const;

	void surface_update_vertex_region(int p_surface, int p_offset, const Vector<uint8_t>& p_data);
	void surface_update_attribute_region(
		int p_surface, int p_offset, const Vector<uint8_t>& p_data);
	void surface_update_skin_region(int p_surface, int p_offset, const Vector<uint8_t>& p_data);

	int get_surface_count() const override;

	void surface_remove(int p_surface);
	void clear_surfaces();

	void surface_set_custom_aabb(int p_idx, const AABB& p_aabb); // only recognized by driver

	int surface_get_array_len(int p_idx) const override;
	int surface_get_array_index_len(int p_idx) const override;
	uint32_t surface_get_format(int p_idx) const override;
	PrimitiveType surface_get_primitive_type(int p_idx) const override;

	virtual void surface_set_material(int p_idx, const Ref<Material>& p_material) override;
	virtual Ref<Material> surface_get_material(int p_idx) const override;

	int surface_find_by_name(const String& p_name) const;
	void surface_set_name(int p_idx, const String& p_name);
	String surface_get_name(int p_idx) const;

	void set_custom_aabb(const AABB& p_custom);
	AABB get_custom_aabb() const;

	AABB get_aabb() const override;
	virtual RID get_rid() const override;

	void regen_normal_maps();

	Error lightmap_unwrap(
		const Transform3D& p_base_transform = Transform3D(), float p_texel_size = 0.05);
	Error lightmap_unwrap_cached(const Transform3D& p_base_transform, float p_texel_size,
		const Vector<uint8_t>& p_src_cache, Vector<uint8_t>& r_dst_cache,
		bool p_generate_cache = true);

	virtual void reload_from_file() override;

	void set_shadow_mesh(const Ref<ArrayMesh>& p_mesh);
	Ref<ArrayMesh> get_shadow_mesh() const;

	ArrayMesh();

	~ArrayMesh();
};

class PlaceholderMesh : public Mesh
{
	RID rid;
	AABB aabb;

protected:

public:
	virtual int get_surface_count() const override { return 0; }

	virtual int surface_get_array_len(int p_idx) const override { return 0; }

	virtual int surface_get_array_index_len(int p_idx) const override { return 0; }

	virtual uint32_t surface_get_format(int p_idx) const override { return 0; }

	virtual PrimitiveType surface_get_primitive_type(int p_idx) const override
	{
		return PRIMITIVE_TRIANGLES;
	}

	virtual void surface_set_material(int p_idx, const Ref<Material>& p_material) override {}

	virtual Ref<Material> surface_get_material(int p_idx) const override { return Ref<Material>(); }

	virtual int get_blend_shape_count() const override { return 0; }

	virtual String get_blend_shape_name(int p_index) const override { return String(); }

	virtual void set_blend_shape_name(int p_index, const String& p_name) override {}

	virtual RID get_rid() const override { return rid; }

	virtual AABB get_aabb() const override { return aabb; }

	void set_aabb(const AABB& p_aabb) { aabb = p_aabb; }

	virtual int get_builtin_bind_pose_count() const override { return 0; }

	virtual Transform3D get_builtin_bind_pose(int p_index) const override { return Transform3D(); }

	PlaceholderMesh();
	~PlaceholderMesh();
};

// --- Folded from scene/resources/immediate_mesh.h ---
#include "core/templates/local_vector.h"


class ImmediateMesh : public Mesh
{
	RID mesh;

	bool uses_colors = false;
	bool uses_normals = false;
	bool uses_tangents = false;
	bool uses_uvs = false;
	bool uses_uv2s = false;

	Color current_color;
	Vector3 current_normal;
	Plane current_tangent;
	Vector2 current_uv;
	Vector2 current_uv2;

	LocalVector<Color> colors;
	LocalVector<Vector3> normals;
	LocalVector<Plane> tangents;
	LocalVector<Vector2> uvs;
	LocalVector<Vector2> uv2s;
	LocalVector<Vector3> vertices;

	struct Surface
	{
		PrimitiveType primitive;
		Ref<Material> material;
		bool vertex_2d = false;
		int array_len = 0;
		uint64_t format = 0;
		AABB aabb;
	};

	LocalVector<Surface> surfaces;

	bool surface_active = false;
	Surface active_surface_data;

	Vector<uint8_t> surface_vertex_create_cache;
	Vector<uint8_t> surface_attribute_create_cache;

	const Vector3 SMALL_VEC3 = Vector3(CMP_EPSILON, CMP_EPSILON, CMP_EPSILON);

protected:

public:
	void surface_begin(
		PrimitiveType p_primitive, const Ref<Material>& p_material = Ref<Material>());
	void surface_set_color(const Color& p_color);
	void surface_set_normal(const Vector3& p_normal);
	void surface_set_tangent(const Plane& p_tangent);
	void surface_set_uv(const Vector2& p_uv);
	void surface_set_uv2(const Vector2& p_uv2);
	void surface_add_vertex(const Vector3& p_vertex);
	void surface_add_vertex_2d(const Vector2& p_vertex);
	void surface_end();

	void clear_surfaces();

	virtual int get_surface_count() const override;
	virtual int surface_get_array_len(int p_idx) const override;
	virtual int surface_get_array_index_len(int p_idx) const override;
	virtual uint32_t surface_get_format(int p_idx) const override;
	virtual PrimitiveType surface_get_primitive_type(int p_idx) const override;
	virtual void surface_set_material(int p_idx, const Ref<Material>& p_material) override;
	virtual Ref<Material> surface_get_material(int p_idx) const override;
	virtual int get_blend_shape_count() const override;
	virtual String get_blend_shape_name(int p_index) const override;
	virtual void set_blend_shape_name(int p_index, const String& p_name) override;

	virtual AABB get_aabb() const override;

	virtual RID get_rid() const override;

	ImmediateMesh();
	~ImmediateMesh();
};



// --- End scene/resources/immediate_mesh.h ---

// --- Folded from scene/resources/surface_tool.h ---
#include "core/math/projection.h"
#include "core/templates/local_vector.h"

#include "servers/rendering/rendering_server_enums.h"

class SurfaceTool : public RefCounted
{
	static const uint32_t custom_mask[RSE::ARRAY_CUSTOM_COUNT];
	static const uint32_t custom_shift[RSE::ARRAY_CUSTOM_COUNT];

public:
	struct Vertex
	{
		// Trivial data for which the hash is computed using hash_buffer.
		// ----------------------------------------------------------------
		uint32_t smooth_group = 0; // Must be first.

		Color color;
		Vector3 normal;
		Vector4 tangent; // xyz tangent, w orientation.
		Vector2 uv;
		Vector2 uv2;
		Color custom[RSE::ARRAY_CUSTOM_COUNT];

		Vector3 vertex; // Must be last.
		// ----------------------------------------------------------------

		Vector<int> bones;
		Vector<float> weights;

		bool operator==(const Vertex& p_vertex) const;
	};

	enum CustomFormat
	{
		CUSTOM_RGBA8_UNORM = RSE::ARRAY_CUSTOM_RGBA8_UNORM,
		CUSTOM_RGBA8_SNORM = RSE::ARRAY_CUSTOM_RGBA8_SNORM,
		CUSTOM_RG_HALF = RSE::ARRAY_CUSTOM_RG_HALF,
		CUSTOM_RGBA_HALF = RSE::ARRAY_CUSTOM_RGBA_HALF,
		CUSTOM_R_FLOAT = RSE::ARRAY_CUSTOM_R_FLOAT,
		CUSTOM_RG_FLOAT = RSE::ARRAY_CUSTOM_RG_FLOAT,
		CUSTOM_RGB_FLOAT = RSE::ARRAY_CUSTOM_RGB_FLOAT,
		CUSTOM_RGBA_FLOAT = RSE::ARRAY_CUSTOM_RGBA_FLOAT,
		CUSTOM_MAX = RSE::ARRAY_CUSTOM_MAX
	};

	enum SkinWeightCount
	{
		SKIN_4_WEIGHTS,
		SKIN_8_WEIGHTS
	};

	enum
	{
		/* Do not move vertices that are located on the topological border (vertices on triangle
		   edges that don't have a paired triangle). Useful for simplifying portions of the larger
		   mesh. */
		SIMPLIFY_LOCK_BORDER = 1 << 0, // From meshopt_SimplifyLockBorder
		/* Improve simplification performance assuming input indices are a sparse subset of the
		   mesh. Note that error becomes relative to subset extents. */
		SIMPLIFY_SPARSE = 1 << 1, // From meshopt_SimplifySparse
		/* Treat error limit and resulting error as absolute instead of relative to mesh extents. */
		SIMPLIFY_ERROR_ABSOLUTE = 1 << 2, // From meshopt_SimplifyErrorAbsolute
		/* Remove disconnected parts of the mesh during simplification incrementally, regardless of
		   the topological restrictions inside components. */
		SIMPLIFY_PRUNE = 1 << 3, // From meshopt_SimplifyPrune
		/* Produce more regular triangle sizes and shapes during simplification, at some cost to
		   geometric quality. */
		SIMPLIFY_REGULARIZE = 1 << 4, // From meshopt_SimplifyRegularize
		/* Allow collapses across attribute discontinuities, except for vertices that are tagged
		   with 0x02 in vertex_lock. */
		SIMPLIFY_PERMISSIVE = 1 << 5, // From meshopt_SimplifyPermissive
		/* Produce tangents compatible with MikkTSpace (same weighting and fallbacks) at the cost of
		   reduced quality. Not recommended unless normal maps are baked. */
		TANGENT_COMPATIBLE = 1 << 0, // From meshopt_TangentCompatible
	};

	typedef void (*OptimizeVertexCacheFunc)(unsigned int* destination, const unsigned int* indices,
		size_t index_count, size_t vertex_count);
	static OptimizeVertexCacheFunc optimize_vertex_cache_func;
	typedef size_t (*OptimizeVertexFetchRemapFunc)(unsigned int* destination,
		const unsigned int* indices, size_t index_count, size_t vertex_count);
	static OptimizeVertexFetchRemapFunc optimize_vertex_fetch_remap_func;
	typedef size_t (*SimplifyFunc)(unsigned int* destination, const unsigned int* indices,
		size_t index_count, const float* vertex_positions, size_t vertex_count,
		size_t vertex_positions_stride, size_t target_index_count, float target_error,
		unsigned int options, float* r_error);
	static SimplifyFunc simplify_func;
	typedef size_t (*SimplifyWithAttribFunc)(unsigned int* destination, const unsigned int* indices,
		size_t index_count, const float* vertex_data, size_t vertex_count, size_t vertex_stride,
		const float* attributes, size_t attribute_stride, const float* attribute_weights,
		size_t attribute_count, const unsigned char* vertex_lock, size_t target_index_count,
		float target_error, unsigned int options, float* result_error);
	static SimplifyWithAttribFunc simplify_with_attrib_func;
	typedef float (*SimplifyScaleFunc)(
		const float* vertex_positions, size_t vertex_count, size_t vertex_positions_stride);
	static SimplifyScaleFunc simplify_scale_func;
	typedef size_t (*GenerateRemapFunc)(unsigned int* destination, const unsigned int* indices,
		size_t index_count, const void* vertices, size_t vertex_count, size_t vertex_size);
	static GenerateRemapFunc generate_remap_func;
	typedef void (*RemapVertexFunc)(void* destination, const void* vertices, size_t vertex_count,
		size_t vertex_size, const unsigned int* remap);
	static RemapVertexFunc remap_vertex_func;
	typedef void (*RemapIndexFunc)(unsigned int* destination, const unsigned int* indices,
		size_t index_count, const unsigned int* remap);
	static RemapIndexFunc remap_index_func;
	typedef void (*GenerateTangentsFunc)(float* result, const unsigned int* indices,
		size_t index_count, const float* vertex_positions, size_t vertex_count,
		size_t vertex_positions_stride, const float* vertex_normals, size_t vertex_normals_stride,
		const float* vertex_uvs, size_t vertex_uvs_stride, unsigned int options);
	static GenerateTangentsFunc generate_tangents_func;
	static void strip_mesh_arrays(PackedVector3Array& r_vertices, PackedInt32Array& r_indices);

private:
	struct VertexHasher
	{
		static _FORCE_INLINE_ uint32_t hash(const Vertex& p_vtx);
	};

	struct SmoothGroupVertex
	{
		Vector3 vertex;
		uint32_t smooth_group = 0;
		bool operator==(const SmoothGroupVertex& p_vertex) const;

		SmoothGroupVertex(const Vertex& p_vertex)
		{
			vertex = p_vertex.vertex;
			smooth_group = p_vertex.smooth_group;
		}
	};

	struct SmoothGroupVertexHasher
	{
		static _FORCE_INLINE_ uint32_t hash(const SmoothGroupVertex& p_vtx);
	};

	struct TriangleHasher
	{
		static _FORCE_INLINE_ uint32_t hash(const int* p_triangle);
		static _FORCE_INLINE_ bool compare(const int* p_lhs, const int* p_rhs);
	};

	struct WeightSort
	{
		int index = 0;
		float weight = 0.0;

		bool operator<(const WeightSort& p_right) const { return weight < p_right.weight; }
	};

	bool begun = false;
	bool first = false;
	Mesh::PrimitiveType primitive = Mesh::PRIMITIVE_LINES;
	uint64_t format = 0;
	Ref<Material> material;
	// arrays
	LocalVector<Vertex> vertex_array;
	LocalVector<int> index_array;

	// memory
	Color last_color;
	Vector3 last_normal;
	Vector2 last_uv;
	Vector2 last_uv2;
	Vector<int> last_bones;
	Vector<float> last_weights;
	Vector4 last_tangent;
	uint32_t last_smooth_group = 0;

	SkinWeightCount skin_weights = SKIN_4_WEIGHTS;

	Color last_custom[RSE::ARRAY_CUSTOM_COUNT];

	CustomFormat last_custom_format[RSE::ARRAY_CUSTOM_COUNT];

	void _create_list(const Ref<Mesh>& p_existing, int p_surface, LocalVector<Vertex>* r_vertex,
		LocalVector<int>* r_index, uint64_t& lformat);

	void _generate_tangents_bind();

protected:

public:
	void set_skin_weight_count(SkinWeightCount p_weights);
	SkinWeightCount get_skin_weight_count() const;

	void set_custom_format(int p_channel_index, CustomFormat p_format);
	CustomFormat get_custom_format(int p_channel_index) const;

	Mesh::PrimitiveType get_primitive_type() const;

	void begin(Mesh::PrimitiveType p_primitive);

	void set_color(Color p_color);
	void set_normal(const Vector3& p_normal);
	void set_tangent(const Plane& p_tangent);
	void set_uv(const Vector2& p_uv);
	void set_uv2(const Vector2& p_uv2);
	void set_custom(int p_channel_index, const Color& p_custom);
	void set_bones(const Vector<int>& p_bones);
	void set_weights(const Vector<float>& p_weights);
	void set_smooth_group(uint32_t p_group);

	void add_vertex(const Vector3& p_vertex);

	void add_triangle_fan(const Vector<Vector3>& p_vertices,
		const Vector<Vector2>& p_uvs = Vector<Vector2>(),
		const Vector<Color>& p_colors = Vector<Color>(),
		const Vector<Vector2>& p_uv2s = Vector<Vector2>(),
		const Vector<Vector3>& p_normals = Vector<Vector3>(),
		const Vector<Plane>& p_tangents = Vector<Plane>());

	void add_index(int p_index);

	void index();
	void deindex();
	void generate_normals(bool p_flip = false);
	void generate_tangents(bool p_split = false);

	void optimize_indices_for_cache();
	AABB get_aabb() const;
	Vector<int> generate_lod(float p_threshold, int p_target_index_count = 3);

	void set_material(const Ref<Material>& p_material);
	Ref<Material> get_material() const;

	void clear();

	LocalVector<Vertex>& get_vertex_array() { return vertex_array; }

	void create_from(const Ref<Mesh>& p_existing, int p_surface);
	void create_from_blend_shape(
		const Ref<Mesh>& p_existing, int p_surface, const String& p_blend_shape_name);
	void append_from(const Ref<Mesh>& p_existing, int p_surface, const Transform3D& p_xform);
	Ref<ArrayMesh> commit(
		const Ref<ArrayMesh>& p_existing = Ref<ArrayMesh>(), uint64_t p_compress_flags = 0);

	SurfaceTool();
};
// --- End scene/resources/surface_tool.h ---
