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
#include "scene/resources/texture.h"
#include "servers/rendering/rendering_device.h"

#ifndef PHYSICS_3D_DISABLED
// --- Folded from scene/resources/3d/shape_3d.h ---
#include "core/io/resource.h"

class ArrayMesh;
class Image;
class Material;

class Shape3D : public Resource
{
public:
	enum ShapeType
	{
		SHAPE_WORLD_BOUNDARY,
		SHAPE_SEPARATION_RAY,
		SHAPE_SPHERE,
		SHAPE_BOX,
		SHAPE_CAPSULE,
		SHAPE_CYLINDER,
		SHAPE_CONVEX_POLYGON,
		SHAPE_CONCAVE_POLYGON,
		SHAPE_HEIGHTMAP,
	};

private:
	struct DrawEdge
	{
		Vector3 a;
		Vector3 b;

		static uint32_t hash(const DrawEdge& p_edge)
		{
			uint32_t h = hash_murmur3_one_32(HashMapHasherDefault::hash(p_edge.a));
			return hash_murmur3_one_32(HashMapHasherDefault::hash(p_edge.b), h);
		}

		bool operator==(const DrawEdge& p_edge) const { return (a == p_edge.a && b == p_edge.b); }

		DrawEdge(const Vector3& p_a = Vector3(), const Vector3& p_b = Vector3())
		{
			a = p_a;
			b = p_b;
			if (a < b) {
				SWAP(a, b);
			}
		}
	};

	ShapeType shape_type = SHAPE_BOX;
	RID shape;
	real_t custom_bias = 0.0;
	real_t margin = 0.04;

	Ref<ArrayMesh> debug_mesh_cache;

	// Not wrapped in `#ifdef DEBUG_ENABLED` as it is used for rendering.
	Color debug_color = Color(0.0, 0.0, 0.0, 0.0);
	bool debug_fill = true;
#ifdef DEBUG_ENABLED
	bool debug_properties_edited = false;
#endif // DEBUG_ENABLED

	// World boundary parameters
	Plane plane = Plane(0, 1, 0, 0);

	// Separation ray parameters
	float length = 1.0f;
	bool slide_on_slope = false;

	// Sphere / Capsule / Cylinder parameters
	float radius = 0.5f;
	float height = 2.0f;

	// Box parameters
	Vector3 size = Vector3(1, 1, 1);

	// Convex polygon parameters
	Vector<Vector3> points;

	// Concave polygon parameters
	Vector<Vector3> faces;
	bool backface_collision = false;

	// Heightmap parameters
	int map_width = 2;
	int map_depth = 2;
	Vector<real_t> map_data;
	real_t min_height = 0.0;
	real_t max_height = 0.0;

	static RID _create_shape_rid(ShapeType p_type);
	void _ensure_heightmap_initialized();
	void _update_shape();

public:
	ShapeType get_shape_type() const { return shape_type; }
	void set_shape_type(ShapeType p_type);

	virtual RID get_rid() const override { return shape; }

	// World boundary
	void set_plane(const Plane& p_plane);
	const Plane& get_plane() const;

	// Separation ray
	void set_length(float p_length);
	float get_length() const;
	void set_slide_on_slope(bool p_active);
	bool get_slide_on_slope() const;

	// Sphere / Capsule / Cylinder
	void set_radius(float p_radius);
	float get_radius() const;
	void set_height(float p_height);
	float get_height() const;
	void set_mid_height(real_t p_mid_height);
	real_t get_mid_height() const;

	// Box
	void set_size(const Vector3& p_size);
	Vector3 get_size() const;

	// Convex polygon
	void set_points(const Vector<Vector3>& p_points);
	Vector<Vector3> get_points() const;

	// Concave polygon
	void set_faces(const Vector<Vector3>& p_faces);
	Vector<Vector3> get_faces() const;
	void set_backface_collision_enabled(bool p_enabled);
	bool is_backface_collision_enabled() const;

	// Heightmap
	void set_map_width(int p_new);
	int get_map_width() const;
	void set_map_depth(int p_new);
	int get_map_depth() const;
	void set_map_data(Vector<real_t> p_new);
	Vector<real_t> get_map_data() const;
	real_t get_min_height() const;
	real_t get_max_height() const;
	void update_map_data_from_image(
		const Ref<Image>& p_image, real_t p_height_min, real_t p_height_max);

	Ref<ArrayMesh> get_debug_mesh();
	Vector<Vector3> get_debug_mesh_lines() const;
	Ref<ArrayMesh> get_debug_arraymesh_faces(const Color& p_modulate) const;
	/// Returns the radius of a sphere that fully encloses this shape.
	real_t get_enclosing_radius() const;

	void add_vertices_to_array(Vector<Vector3>& array, const Transform3D& p_xform);

	void set_custom_solver_bias(real_t p_bias);
	real_t get_custom_solver_bias() const;

	real_t get_margin() const;
	void set_margin(real_t p_margin);

	void set_debug_color(const Color& p_color);
	Color get_debug_color() const;

	void set_debug_fill(bool p_fill);
	bool get_debug_fill() const;

#ifdef DEBUG_ENABLED
	_FORCE_INLINE_ bool are_debug_properties_edited() const { return debug_properties_edited; }
#endif // DEBUG_ENABLED

	Shape3D(ShapeType p_type = SHAPE_BOX);
	~Shape3D();
};
// --- End scene/resources/3d/shape_3d.h ---

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
#include "core/types.h"
#include "core/templates/local_vector.h"

#include "scene/resources/texture.h"

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

// --- Folded from scene/resources/navigation_mesh.h ---
#include "core/os/mutex.h"


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


#include "drivers/gles3/shader_gles3.h"

#ifndef PHYSICS_3D_DISABLED

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
// --- End scene/resources/navigation_mesh.h ---

// --- Folded from scene/resources/3d/primitive_meshes.h ---
#include <memory>

#include "scene/resources/material.h"
#include "scene/resources/theme.h"


/**
	Parameterized procedural single-surface mesh covering box, capsule, cylinder,
	plane, quad, prism, sphere, torus, point, tube trail, ribbon trail, and 3D text.
*/
class PrimitiveMesh : public Mesh
{
public:
	enum MeshType
	{
		MESH_BOX,
		MESH_CAPSULE,
		MESH_CYLINDER,
		MESH_PLANE,
		MESH_QUAD,
		MESH_PRISM,
		MESH_SPHERE,
		MESH_TORUS,
		MESH_POINT,
		MESH_TUBE_TRAIL,
		MESH_RIBBON_TRAIL,
		MESH_TEXT,
	};

	enum Orientation
	{
		FACE_X,
		FACE_Y,
		FACE_Z,
	};

	enum RibbonShape
	{
		SHAPE_FLAT,
		SHAPE_CROSS,
	};

private:
	struct BoxPrismParams
	{
		Vector3 size = Vector3(1.0f, 1.0f, 1.0f);
		float left_to_right = 0.5f;
		int subdivide_w = 0;
		int subdivide_h = 0;
		int subdivide_d = 0;
	};

	struct PlaneParams
	{
		Size2 size = Size2(2.0f, 2.0f);
		Vector3 center_offset;
		Orientation orientation = FACE_Y;
	};

	struct RadialParams
	{
		float radius = 0.5f;
		float top_radius = 0.5f;
		float bottom_radius = 0.5f;
		float inner_radius = 0.5f;
		float outer_radius = 1.0f;
		float height = 2.0f;
		int radial_segments = 64;
		int rings = 8;
		int ring_segments = 32;
		bool cap_top = true;
		bool cap_bottom = true;
		bool is_hemisphere = false;
	};

	struct TrailParams
	{
		float ribbon_size = 1.0f;
		int radial_steps = 8;
		int sections = 5;
		float section_length = 0.2f;
		int section_rings = 3;
		int section_segments = 3;
		RibbonShape ribbon_shape = SHAPE_CROSS;
		Ref<Curve> curve;
	};

	struct TextMeshState
	{
		struct ContourPoint
		{
			Vector2 point;
			bool sharp = false;

			ContourPoint() = default;
			ContourPoint(const Vector2& p_pt, bool p_sharp) : point(p_pt), sharp(p_sharp) {}
		};

		struct ContourInfo
		{
			real_t length = 0.0;
			bool ccw = true;

			ContourInfo() = default;
			ContourInfo(real_t p_len, bool p_ccw) : length(p_len), ccw(p_ccw) {}
		};

		struct GlyphMeshKey
		{
			uint64_t font_id = 0;
			uint32_t gl_id = 0;

			bool operator==(const GlyphMeshKey& p_b) const
			{
				return (font_id == p_b.font_id) && (gl_id == p_b.gl_id);
			}

			GlyphMeshKey(uint64_t p_font_id, uint32_t p_gl_id) :
					font_id(p_font_id), gl_id(p_gl_id) {}
		};

		struct GlyphMeshKeyHasher
		{
			_FORCE_INLINE_ static uint32_t hash(const GlyphMeshKey& p_a)
			{
				return hash_murmur3_buffer(&p_a, sizeof(GlyphMeshKey));
			}
		};

		struct GlyphMeshData
		{
			Vector<Vector2> triangles;
			Vector<Vector<ContourPoint>> contours;
			Vector<ContourInfo> contours_info;
			Vector2 min_p = Vector2(Math::INF, Math::INF);
			Vector2 max_p = Vector2(-Math::INF, -Math::INF);
		};

		mutable HashMap<GlyphMeshKey, GlyphMeshData, GlyphMeshKeyHasher> cache;

		mutable RID text_rid;
		mutable Vector<RID> lines_rid;

		String text;
		String xl_text;

		int font_size = 16;
		Ref<Font> font_override;

		TextServer::AutowrapMode autowrap_mode = TextServer::AUTOWRAP_OFF;
		uint32_t jst_flags = TextServer::JUSTIFICATION_WORD_BOUND |
							 TextServer::JUSTIFICATION_KASHIDA |
							 TextServer::JUSTIFICATION_SKIP_LAST_LINE |
							 TextServer::JUSTIFICATION_DO_NOT_SKIP_SINGLE_LINE;
		float width = 500.0f;
		float line_spacing = 0.0f;
		Point2 lbl_offset;

		HorizontalAlignment horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER;
		VerticalAlignment vertical_alignment = VERTICAL_ALIGNMENT_CENTER;
		bool uppercase = false;
		String language;
		TextServer::Direction text_direction = TextServer::DIRECTION_AUTO;
		TextServer::StructuredTextParser st_parser = TextServer::STRUCTURED_TEXT_DEFAULT;

		real_t depth = 0.05;
		real_t pixel_size = 0.01;
		real_t curve_step = 0.5;

		mutable bool dirty_lines = true;
		mutable bool dirty_text = true;
		mutable bool dirty_font = true;
		mutable bool dirty_cache = true;

		RID ensure_shaped_text() const;
		~TextMeshState();
	};

	MeshType mesh_type = MESH_BOX;

	RID mesh;
	mutable AABB aabb;
	AABB custom_aabb;

	mutable int array_len = 0;
	mutable int index_array_len = 0;

	Ref<Material> material;
	bool flip_faces = false;

	bool add_uv2 = false;
	float uv2_padding = 2.0f;

	mutable bool pending_request = true;
	Mesh::PrimitiveType primitive_type = Mesh::PRIMITIVE_TRIANGLES;
	float texel_size = 0.2f;

	BoxPrismParams box_prism;
	PlaneParams plane;
	RadialParams radial;
	TrailParams trail;
	std::unique_ptr<TextMeshState> text_state;

	void _apply_mesh_type_defaults(MeshType p_type);
	TextMeshState& _ensure_text_state();
	const TextMeshState& _get_text_state() const;

	Vector2 get_uv2_scale(Vector2 p_margin_scale = Vector2(1.0f, 1.0f)) const;
	float get_lightmap_texel_size() const;

	void _update() const;
	void _update_lightmap_size();
	void _on_settings_changed();
	void _curve_changed();
	void _font_changed();

public:
	PrimitiveMesh(MeshType p_type = MESH_BOX);
	virtual ~PrimitiveMesh() override;

	void set_mesh_type(MeshType p_type);
	MeshType get_mesh_type() const { return mesh_type; }

	virtual int get_surface_count() const override;
	virtual int surface_get_array_len(int p_idx) const override;
	virtual int surface_get_array_index_len(int p_idx) const override;
	virtual uint32_t surface_get_format(int p_idx) const override;
	virtual Mesh::PrimitiveType surface_get_primitive_type(int p_idx) const override;
	virtual void surface_set_material(int p_idx, const Ref<Material>& p_material) override;
	virtual Ref<Material> surface_get_material(int p_idx) const override;
	virtual int get_blend_shape_count() const override;
	virtual String get_blend_shape_name(int p_index) const override;
	virtual void set_blend_shape_name(int p_index, const String& p_name) override;
	virtual AABB get_aabb() const override;
	virtual RID get_rid() const override;
	virtual int get_builtin_bind_pose_count() const override;
	virtual Transform3D get_builtin_bind_pose(int p_index) const override;

	void set_material(const Ref<Material>& p_material);
	Ref<Material> get_material() const;

	void set_custom_aabb(const AABB& p_custom);
	AABB get_custom_aabb() const;

	void set_flip_faces(bool p_enable);
	bool get_flip_faces() const;

	void set_add_uv2(bool p_enable);
	bool get_add_uv2() const { return add_uv2; }

	void set_uv2_padding(float p_padding);
	float get_uv2_padding() const { return uv2_padding; }

	void request_update();

	// Box / Prism / Plane / Quad parameters.
	void set_size(const Vector3& p_size);
	Vector3 get_size() const;

	void set_plane_size(const Size2& p_size);
	Size2 get_plane_size() const;
	void set_size(const Size2& p_size) { set_plane_size(p_size); }

	void set_subdivide_width(int p_divisions);
	int get_subdivide_width() const;

	void set_subdivide_height(int p_divisions);
	int get_subdivide_height() const;

	void set_subdivide_depth(int p_divisions);
	int get_subdivide_depth() const;

	void set_center_offset(const Vector3& p_offset);
	Vector3 get_center_offset() const;

	void set_orientation(Orientation p_orientation);
	Orientation get_orientation() const;

	void set_left_to_right(float p_left_to_right);
	float get_left_to_right() const;

	// Capsule / Cylinder / Sphere / Torus parameters.
	void set_radius(float p_radius);
	float get_radius() const;

	void set_top_radius(float p_radius);
	float get_top_radius() const;

	void set_bottom_radius(float p_radius);
	float get_bottom_radius() const;

	void set_inner_radius(float p_inner_radius);
	float get_inner_radius() const;

	void set_outer_radius(float p_outer_radius);
	float get_outer_radius() const;

	void set_height(float p_height);
	float get_height() const;

	void set_radial_segments(int p_segments);
	int get_radial_segments() const;

	void set_rings(int p_rings);
	int get_rings() const;

	void set_ring_segments(int p_ring_segments);
	int get_ring_segments() const;

	void set_cap_top(bool p_cap_top);
	bool is_cap_top() const;

	void set_cap_bottom(bool p_cap_bottom);
	bool is_cap_bottom() const;

	void set_is_hemisphere(bool p_is_hemisphere);
	bool get_is_hemisphere() const;

	// TubeTrail / RibbonTrail parameters.
	void set_radial_steps(int p_radial_steps);
	int get_radial_steps() const;

	void set_sections(int p_sections);
	int get_sections() const;

	void set_section_length(float p_section_length);
	float get_section_length() const;

	void set_section_rings(int p_section_rings);
	int get_section_rings() const;

	void set_section_segments(int p_section_segments);
	int get_section_segments() const;

	void set_ribbon_size(float p_size);
	float get_ribbon_size() const;

	void set_ribbon_shape(RibbonShape p_shape);
	RibbonShape get_ribbon_shape() const;
	void set_shape(RibbonShape p_shape) { set_ribbon_shape(p_shape); }
	RibbonShape get_shape() const { return get_ribbon_shape(); }

	void set_curve(const Ref<Curve>& p_curve);
	Ref<Curve> get_curve() const;

	// Text parameters.
	void set_horizontal_alignment(HorizontalAlignment p_alignment);
	HorizontalAlignment get_horizontal_alignment() const;

	void set_vertical_alignment(VerticalAlignment p_alignment);
	VerticalAlignment get_vertical_alignment() const;

	void set_text(const String& p_string);
	String get_text() const;

	void set_font(const Ref<Font>& p_font);
	Ref<Font> get_font() const;
	Ref<Font> _get_font_or_default() const;

	void set_font_size(int p_size);
	int get_font_size() const;

	void set_line_spacing(float p_line_spacing);
	float get_line_spacing() const;

	void set_autowrap_mode(TextServer::AutowrapMode p_mode);
	TextServer::AutowrapMode get_autowrap_mode() const;

	void set_justification_flags(uint32_t p_flags);
	uint32_t get_justification_flags() const;

	void set_text_direction(TextServer::Direction p_text_direction);
	TextServer::Direction get_text_direction() const;

	void set_language(const String& p_language);
	String get_language() const;

	void set_structured_text_bidi_override(TextServer::StructuredTextParser p_parser);
	TextServer::StructuredTextParser get_structured_text_bidi_override() const;

	void set_uppercase(bool p_uppercase);
	bool is_uppercase() const;

	void set_width(real_t p_width);
	real_t get_width() const;

	void set_depth(real_t p_depth);
	real_t get_depth() const;

	void set_curve_step(real_t p_step);
	real_t get_curve_step() const;

	void set_pixel_size(real_t p_amount);
	real_t get_pixel_size() const;

	void set_offset(const Point2& p_offset);
	Point2 get_offset() const;
};

// --- Folded from scene/resources/3d/mesh_library.h ---
#include "core/io/resource.h"
#include "core/templates/hash_map.h"
#include "scene/main/node.h"


#include "scene/resources/texture.h"

#ifndef PHYSICS_3D_DISABLED

#endif // PHYSICS_3D_DISABLED

class MeshLibrary : public Resource
{
	static inline PropertyListHelper base_property_helper;
	PropertyListHelper property_helper;
	bool init_property = false;

	bool _validate_index(int p_idx);

public:
#ifndef PHYSICS_3D_DISABLED
	struct ShapeData
	{
		Ref<Shape3D> shape;
		Transform3D local_transform;
	};
#endif // PHYSICS_3D_DISABLED

	struct Item
	{
		String name;
		String category;
		Ref<Mesh> mesh;
		Transform3D mesh_transform;
		RSE::ShadowCastingSetting mesh_cast_shadow = RSE::SHADOW_CASTING_SETTING_ON;
#ifndef PHYSICS_3D_DISABLED
		Vector<ShapeData> shapes;
#endif // PHYSICS_3D_DISABLED
		Ref<Texture2D> preview;
		Ref<NavigationMesh> navigation_mesh;
		Transform3D navigation_mesh_transform;
		uint32_t navigation_layers = 1;
	};

private:
	RBMap<int, Item> item_map;

protected:
	bool _property_can_revert(const String& p_name) const
	{
		return property_helper.property_can_revert(p_name);
	}

	virtual void reset_state() override;

public:
	void create_item(int p_item);
	void set_item_name(int p_item, const String& p_name);
	void set_item_category(int p_item, const String& p_category);
	void set_item_mesh(int p_item, const Ref<Mesh>& p_mesh);
	void set_item_mesh_transform(int p_item, const Transform3D& p_transform);
	void set_item_mesh_cast_shadow(int p_item, RSE::ShadowCastingSetting p_shadow_casting_setting);
#ifndef NAVIGATION_3D_DISABLED
	void set_item_navigation_mesh(int p_item, const Ref<NavigationMesh>& p_navigation_mesh);
	void set_item_navigation_mesh_transform(int p_item, const Transform3D& p_transform);
	void set_item_navigation_layers(int p_item, uint32_t p_navigation_layers);
#endif // NAVIGATION_3D_DISABLED

#ifndef PHYSICS_3D_DISABLED
	void set_item_shapes(int p_item, const Vector<ShapeData>& p_shapes);
#endif // PHYSICS_3D_DISABLED
	void set_item_preview(int p_item, const Ref<Texture2D>& p_preview);

	String get_item_name(int p_item) const;
	String get_item_category(int p_item) const;
	Ref<Mesh> get_item_mesh(int p_item) const;
	Transform3D get_item_mesh_transform(int p_item) const;
	RSE::ShadowCastingSetting get_item_mesh_cast_shadow(int p_item) const;
	Ref<NavigationMesh> get_item_navigation_mesh(int p_item) const;
	Transform3D get_item_navigation_mesh_transform(int p_item) const;
	uint32_t get_item_navigation_layers(int p_item) const;
#ifndef PHYSICS_3D_DISABLED
	Vector<ShapeData> get_item_shapes(int p_item) const;
#endif // PHYSICS_3D_DISABLED
	Ref<Texture2D> get_item_preview(int p_item) const;

	void remove_item(int p_item);
	bool has_item(int p_item) const;

	void clear();

	int find_item_by_name(const String& p_name) const;

	Vector<int> get_item_list() const;

	int get_item_count() const { return item_map.size(); }

	int get_last_unused_item_id() const;

	MeshLibrary();
};



// --- End scene/resources/3d/mesh_library.h ---
// --- End scene/resources/3d/primitive_meshes.h ---
