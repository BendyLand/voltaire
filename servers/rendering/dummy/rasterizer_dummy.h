/**************************************************************************/
/*  rasterizer_dummy.h                                                    */
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

#include "core/math/aabb.h"
#include "core/math/transform_3d.h"
#include "core/math/vector3.h"
#include "core/math/vector3i.h"
#include "core/templates/paged_allocator.h"
#include "core/templates/rid.h"
#include "core/templates/rid_owner.h"
#include "core/types.h"
#include "servers/rendering/environment/renderer_fog.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/renderer_canvas_render.h"
#include "servers/rendering/renderer_compositor.h"
#include "servers/rendering/renderer_scene_render.h"
#include "servers/rendering/rendering_server_enums.h"
#include "servers/rendering/rendering_server_types.h"
#include "servers/rendering/shader_compiler.h"
#include "servers/rendering/shader_language.h"
#include "servers/rendering/storage/light_storage.h"
#include "servers/rendering/storage/material_storage.h"
#include "servers/rendering/storage/mesh_storage.h"
#include "servers/rendering/storage/particles_storage.h"
#include "servers/rendering/storage/texture_storage.h"
#include "servers/rendering/storage/utilities.h"

class DependencyTracker;
class Image;

namespace RendererDummy
{

class Fog final
{
public:
	Fog() = delete;
	~Fog() = delete;
	Fog(const Fog&) = delete;
	Fog& operator=(const Fog&) = delete;

	static void initialize() {}

	static void finalize() {}

	static bool is_initialized() { return true; }

	static bool free(RID p_rid) { return false; }

	/* FOG VOLUMES */

	static bool owns_fog_volume(RID p_rid) { return false; }

	static RID fog_volume_allocate() { return RID(); }

	static void fog_volume_initialize(RID p_rid) {}

	static void fog_volume_free(RID p_rid) {}

	static void fog_volume_set_shape(RID p_fog_volume, RSE::FogVolumeShape p_shape) {}

	static void fog_volume_set_size(RID p_fog_volume, const Vector3& p_size) {}

	static void fog_volume_set_material(RID p_fog_volume, RID p_material) {}

	static AABB fog_volume_get_aabb(RID p_fog_volume) { return AABB(); }

	static RSE::FogVolumeShape fog_volume_get_shape(RID p_fog_volume)
	{
		return RSE::FOG_VOLUME_SHAPE_ELLIPSOID;
	}
};

class GI final
{
public:
	GI() = delete;
	~GI() = delete;
	GI(const GI&) = delete;
	GI& operator=(const GI&) = delete;

	static void initialize() {}

	static void finalize() {}

	static bool is_initialized() { return true; }

	static bool free(RID p_rid) { return false; }

	/* VOXEL GI API */

	static bool owns_voxel_gi(RID p_rid) { return false; }

	static RID voxel_gi_allocate() { return RID(); }

	static void voxel_gi_free(RID p_rid) {}

	static void voxel_gi_initialize(RID p_rid) {}

	static void voxel_gi_allocate_data(RID p_voxel_gi, const Transform3D& p_to_cell_xform,
		const AABB& p_aabb, const Vector3i& p_octree_size, const Vector<uint8_t>& p_octree_cells,
		const Vector<uint8_t>& p_data_cells, const Vector<uint8_t>& p_distance_field,
		const Vector<int>& p_level_counts)
	{
	}

	static AABB voxel_gi_get_bounds(RID p_voxel_gi) { return AABB(); }

	static Vector3i voxel_gi_get_octree_size(RID p_voxel_gi) { return Vector3i(); }

	static Vector<uint8_t> voxel_gi_get_octree_cells(RID p_voxel_gi) { return Vector<uint8_t>(); }

	static Vector<uint8_t> voxel_gi_get_data_cells(RID p_voxel_gi) { return Vector<uint8_t>(); }

	static Vector<uint8_t> voxel_gi_get_distance_field(RID p_voxel_gi) { return Vector<uint8_t>(); }

	static Vector<int> voxel_gi_get_level_counts(RID p_voxel_gi) { return Vector<int>(); }

	static Transform3D voxel_gi_get_to_cell_xform(RID p_voxel_gi) { return Transform3D(); }

	static void voxel_gi_set_dynamic_range(RID p_voxel_gi, float p_range) {}

	static float voxel_gi_get_dynamic_range(RID p_voxel_gi) { return 0.0f; }

	static void voxel_gi_set_propagation(RID p_voxel_gi, float p_range) {}

	static float voxel_gi_get_propagation(RID p_voxel_gi) { return 0.0f; }

	static void voxel_gi_set_energy(RID p_voxel_gi, float p_range) {}

	static float voxel_gi_get_energy(RID p_voxel_gi) { return 0.0f; }

	static void voxel_gi_set_baked_exposure_normalization(RID p_voxel_gi, float p_baked_exposure) {}

	static float voxel_gi_get_baked_exposure_normalization(RID p_voxel_gi) { return 1.0f; }

	static void voxel_gi_set_bias(RID p_voxel_gi, float p_range) {}

	static float voxel_gi_get_bias(RID p_voxel_gi) { return 0.0f; }

	static void voxel_gi_set_normal_bias(RID p_voxel_gi, float p_range) {}

	static float voxel_gi_get_normal_bias(RID p_voxel_gi) { return 0.0f; }

	static void voxel_gi_set_interior(RID p_voxel_gi, bool p_enable) {}

	static bool voxel_gi_is_interior(RID p_voxel_gi) { return false; }

	static void voxel_gi_set_use_two_bounces(RID p_voxel_gi, bool p_enable) {}

	static bool voxel_gi_is_using_two_bounces(RID p_voxel_gi) { return false; }

	static uint32_t voxel_gi_get_version(RID p_voxel_gi) { return 0; }

	static void sdfgi_reset() {}
};

class LightStorage final
{
public:
	struct Lightmap
	{
		// dummy lightmap, no data
	};

	struct LightmapInstance
	{
		RID lightmap;
	};

private:
	struct Data
	{
		RID_Owner<Lightmap, true> lightmap_owner;
		RID_Owner<LightmapInstance> lightmap_instance_owner;
	};

	static inline Data* data = nullptr;

	_FORCE_INLINE_ static Data* _get_data()
	{
		if (unlikely(!data)) {
			initialize();
		}
		return data;
	}

public:
	LightStorage() = delete;
	LightStorage(const LightStorage&) = delete;
	LightStorage& operator=(const LightStorage&) = delete;
	~LightStorage() = delete;

	static void initialize();
	static void finalize();
	static bool free(RID p_rid);

	/* Light API */
	static RID directional_light_allocate() { return RID(); }

	static void directional_light_initialize(RID p_rid) {}

	static RID omni_light_allocate() { return RID(); }

	static void omni_light_initialize(RID p_rid) {}

	static RID spot_light_allocate() { return RID(); }

	static void spot_light_initialize(RID p_rid) {}

	static RID area_light_allocate() { return RID(); }

	static void area_light_initialize(RID p_rid) {}

	static void light_free(RID p_rid) {}

	static void light_set_color(RID p_light, const Color& p_color) {}

	static void light_set_param(RID p_light, RSE::LightParam p_param, float p_value) {}

	static void light_set_shadow(RID p_light, bool p_enabled) {}

	static void light_set_projector(RID p_light, RID p_texture) {}

	static void light_set_negative(RID p_light, bool p_enable) {}

	static void light_set_cull_mask(RID p_light, uint32_t p_mask) {}

	static void light_set_distance_fade(
		RID p_light, bool p_enabled, float p_begin, float p_shadow, float p_length)
	{
	}

	static void light_set_reverse_cull_face_mode(RID p_light, bool p_enabled) {}

	static void light_set_shadow_caster_mask(RID p_light, uint32_t p_caster_mask) {}

	static uint32_t light_get_shadow_caster_mask(RID p_light) { return 0xFFFFFFFF; }

	static void light_set_bake_mode(RID p_light, RSE::LightBakeMode p_bake_mode) {}

	static void light_set_max_sdfgi_cascade(RID p_light, uint32_t p_cascade) {}

	static void light_omni_set_shadow_mode(RID p_light, RSE::LightOmniShadowMode p_mode) {}

	static void light_directional_set_shadow_mode(
		RID p_light, RSE::LightDirectionalShadowMode p_mode)
	{
	}

	static void light_directional_set_blend_splits(RID p_light, bool p_enable) {}

	static bool light_directional_get_blend_splits(RID p_light) { return false; }

	static void light_directional_set_sky_mode(RID p_light, RSE::LightDirectionalSkyMode p_mode) {}

	static RSE::LightDirectionalSkyMode light_directional_get_sky_mode(RID p_light)
	{
		return RSE::LIGHT_DIRECTIONAL_SKY_MODE_LIGHT_AND_SKY;
	}

	static RSE::LightDirectionalShadowMode light_directional_get_shadow_mode(RID p_light)
	{
		return RSE::LIGHT_DIRECTIONAL_SHADOW_ORTHOGONAL;
	}

	static RSE::LightOmniShadowMode light_omni_get_shadow_mode(RID p_light)
	{
		return RSE::LIGHT_OMNI_SHADOW_DUAL_PARABOLOID;
	}

	static void light_area_set_size(RID p_light, const Vector2& p_size) {}

	static Vector2 light_area_get_size(RID p_light) { return Vector2(); }

	static void light_area_set_normalize_energy(RID p_light, bool p_enabled) {}

	static bool light_area_get_normalize_energy(RID p_light) { return true; }

	static void light_area_set_texture(RID p_light, RID p_texture) {}

	static RID light_area_get_texture(RID p_light) { return RID(); }

	static bool light_has_shadow(RID p_light) { return false; }

	static bool light_has_projector(RID p_light) { return false; }

	static RSE::LightType light_get_type(RID p_light) { return RSE::LIGHT_OMNI; }

	static AABB light_get_aabb(RID p_light) { return AABB(); }

	static float light_get_param(RID p_light, RSE::LightParam p_param) { return 0.0f; }

	static Color light_get_color(RID p_light) { return Color(); }

	static bool light_get_reverse_cull_face_mode(RID p_light) { return false; }

	static RSE::LightBakeMode light_get_bake_mode(RID p_light) { return RSE::LIGHT_BAKE_DISABLED; }

	static uint32_t light_get_max_sdfgi_cascade(RID p_light) { return 0; }

	static uint64_t light_get_version(RID p_light) { return 0; }

	static uint32_t light_get_cull_mask(RID p_light) { return 0; }

	/* LIGHT INSTANCE API */
	static RID light_instance_create(RID p_light) { return RID(); }

	static void light_instance_free(RID p_light) {}

	static void light_instance_set_transform(RID p_light_instance, const Transform3D& p_transform)
	{
	}

	static void light_instance_set_aabb(RID p_light_instance, const AABB& p_aabb) {}

	static void light_instance_set_shadow_transform(RID p_light_instance,
		const Projection& p_projection, const Transform3D& p_transform, float p_far, float p_split,
		int p_pass, float p_shadow_texel_size, float p_bias_scale = 1.0, float p_range_begin = 0,
		const Vector2& p_uv_scale = Vector2())
	{
	}

	static void light_instance_mark_visible(RID p_light_instance) {}

	static bool light_instance_is_shadow_visible_at_position(
		RID p_light_instance, const Vector3& p_position)
	{
		return false;
	}

	/* PROBE API */
	static RID reflection_probe_allocate() { return RID(); }

	static void reflection_probe_initialize(RID p_rid) {}

	static void reflection_probe_free(RID p_rid) {}

	static void reflection_probe_set_update_mode(RID p_probe, RSE::ReflectionProbeUpdateMode p_mode)
	{
	}

	static void reflection_probe_set_intensity(RID p_probe, float p_intensity) {}

	static void reflection_probe_set_blend_distance(RID p_probe, float p_blend_distance) {}

	static void reflection_probe_set_ambient_mode(
		RID p_probe, RSE::ReflectionProbeAmbientMode p_mode)
	{
	}

	static void reflection_probe_set_ambient_color(RID p_probe, const Color& p_color) {}

	static void reflection_probe_set_ambient_energy(RID p_probe, float p_energy) {}

	static void reflection_probe_set_max_distance(RID p_probe, float p_distance) {}

	static void reflection_probe_set_size(RID p_probe, const Vector3& p_size) {}

	static void reflection_probe_set_origin_offset(RID p_probe, const Vector3& p_offset) {}

	static void reflection_probe_set_as_interior(RID p_probe, bool p_enable) {}

	static void reflection_probe_set_enable_box_projection(RID p_probe, bool p_enable) {}

	static void reflection_probe_set_enable_shadows(RID p_probe, bool p_enable) {}

	static void reflection_probe_set_cull_mask(RID p_probe, uint32_t p_layers) {}

	static void reflection_probe_set_reflection_mask(RID p_probe, uint32_t p_layers) {}

	static void reflection_probe_set_resolution(RID p_probe, int p_resolution) {}

	static void reflection_probe_set_mesh_lod_threshold(RID p_probe, float p_ratio) {}

	static float reflection_probe_get_mesh_lod_threshold(RID p_probe) { return 0.0f; }

	static AABB reflection_probe_get_aabb(RID p_probe) { return AABB(); }

	static RSE::ReflectionProbeUpdateMode reflection_probe_get_update_mode(RID p_probe)
	{
		return RSE::REFLECTION_PROBE_UPDATE_ONCE;
	}

	static uint32_t reflection_probe_get_cull_mask(RID p_probe) { return 0; }

	static uint32_t reflection_probe_get_reflection_mask(RID p_probe) { return 0; }

	static Vector3 reflection_probe_get_size(RID p_probe) { return Vector3(); }

	static Vector3 reflection_probe_get_origin_offset(RID p_probe) { return Vector3(); }

	static float reflection_probe_get_origin_max_distance(RID p_probe) { return 0.0f; }

	static bool reflection_probe_renders_shadows(RID p_probe) { return false; }

	/* REFLECTION ATLAS */
	static RID reflection_atlas_create() { return RID(); }

	static void reflection_atlas_free(RID p_ref_atlas) {}

	static int reflection_atlas_get_size(RID p_ref_atlas) { return 0; }

	static void reflection_atlas_set_size(
		RID p_ref_atlas, int p_reflection_size, int p_reflection_count)
	{
	}

	/* REFLECTION PROBE INSTANCE */
	static RID reflection_probe_instance_create(RID p_probe) { return RID(); }

	static void reflection_probe_instance_free(RID p_instance) {}

	static void reflection_probe_instance_set_transform(
		RID p_instance, const Transform3D& p_transform)
	{
	}

	static bool reflection_probe_has_atlas_index(RID p_instance) { return false; }

	static void reflection_probe_release_atlas_index(RID p_instance) {}

	static bool reflection_probe_instance_needs_redraw(RID p_instance) { return false; }

	static bool reflection_probe_instance_has_reflection(RID p_instance) { return false; }

	static bool reflection_probe_instance_begin_render(RID p_instance, RID p_reflection_atlas)
	{
		return false;
	}

	static bool reflection_probe_instance_end_render(RID p_instance, RID p_reflection_atlas)
	{
		return false;
	}

	static Ref<RenderSceneBuffers> reflection_probe_atlas_get_render_buffers(RID p_reflection_atlas)
	{
		return Ref<RenderSceneBuffers>();
	}

	static bool reflection_probe_instance_postprocess_step(RID p_instance) { return true; }

	/* LIGHTMAP CAPTURE */
	static bool owns_lightmap(RID p_rid) { return data && data->lightmap_owner.owns(p_rid); }

	static RID lightmap_allocate();
	static void lightmap_initialize(RID p_rid);
	static void lightmap_free(RID p_rid);

	static void lightmap_set_textures(RID p_lightmap, RID p_light, bool p_uses_spherical_harmonics)
	{
	}

	static void lightmap_set_probe_bounds(RID p_lightmap, const AABB& p_bounds) {}

	static void lightmap_set_probe_interior(RID p_lightmap, bool p_interior) {}

	static void lightmap_set_probe_capture_data(RID p_lightmap, const PackedVector3Array& p_points,
		const PackedColorArray& p_point_sh, const PackedInt32Array& p_tetrahedra,
		const PackedInt32Array& p_bsp_tree)
	{
	}

	static void lightmap_set_baked_exposure_normalization(RID p_lightmap, float p_exposure) {}

	static PackedVector3Array lightmap_get_probe_capture_points(RID p_lightmap)
	{
		return PackedVector3Array();
	}

	static PackedColorArray lightmap_get_probe_capture_sh(RID p_lightmap)
	{
		return PackedColorArray();
	}

	static PackedInt32Array lightmap_get_probe_capture_tetrahedra(RID p_lightmap)
	{
		return PackedInt32Array();
	}

	static PackedInt32Array lightmap_get_probe_capture_bsp_tree(RID p_lightmap)
	{
		return PackedInt32Array();
	}

	static AABB lightmap_get_aabb(RID p_lightmap) { return AABB(); }

	static void lightmap_tap_sh_light(RID p_lightmap, const Vector3& p_point, Color* r_sh) {}

	static bool lightmap_is_interior(RID p_lightmap) { return false; }

	static void lightmap_set_probe_capture_update_speed(float p_speed) {}

	static float lightmap_get_probe_capture_update_speed() { return 0.0f; }

	static void lightmap_set_shadowmask_textures(RID p_lightmap, RID p_shadow) {}

	static RSE::ShadowmaskMode lightmap_get_shadowmask_mode(RID p_lightmap)
	{
		return RSE::SHADOWMASK_MODE_NONE;
	}

	static void lightmap_set_shadowmask_mode(RID p_lightmap, RSE::ShadowmaskMode p_mode) {}

	/* LIGHTMAP INSTANCE */
	static bool owns_lightmap_instance(RID p_rid)
	{
		return data && data->lightmap_instance_owner.owns(p_rid);
	}

	static RID lightmap_instance_create(RID p_lightmap);
	static void lightmap_instance_free(RID p_lightmap);

	static void lightmap_instance_set_transform(RID p_lightmap, const Transform3D& p_transform) {}

	/* SHADOW ATLAS API */
	static RID shadow_atlas_create() { return RID(); }

	static void shadow_atlas_free(RID p_atlas) {}

	static void shadow_atlas_set_size(RID p_atlas, int p_size, bool p_16_bits = true) {}

	static void shadow_atlas_set_quadrant_subdivision(
		RID p_atlas, int p_quadrant, int p_subdivision)
	{
	}

	static bool shadow_atlas_update_light(
		RID p_atlas, RID p_light_instance, float p_coverage, uint64_t p_light_version)
	{
		return false;
	}

	static void shadow_atlas_update(RID p_atlas) {}

	static void directional_shadow_atlas_set_size(int p_size, bool p_16_bits = true) {}

	static int get_directional_light_shadow_size(RID p_light_instance) { return 0; }

	static void set_directional_shadow_count(int p_count) {}
};

class MaterialStorage final
{
private:
	struct DummyShader
	{
		HashMap<String, ShaderLanguage::ShaderNode::Uniform> uniforms;
	};

	struct DummyMaterial
	{
		RID shader;
		RID next_pass;
	};

	struct Data
	{
		HashMap<String, RSE::GlobalShaderParameterType> global_shader_variables;
		RID_Owner<DummyShader> shader_owner;
		ShaderCompiler* dummy_compiler = nullptr;
		HashSet<RID> dummy_embedded_set;
		RID_Owner<DummyMaterial> material_owner;
	};

	static inline Data* data = nullptr;

	_FORCE_INLINE_ static Data* _get_data()
	{
		if (unlikely(!data)) {
			initialize();
		}
		return data;
	}

public:
	MaterialStorage() = delete;
	MaterialStorage(const MaterialStorage&) = delete;
	MaterialStorage& operator=(const MaterialStorage&) = delete;
	~MaterialStorage() = delete;

	static void initialize();
	static void finalize();

	/* GLOBAL SHADER UNIFORM API */
	static void global_shader_parameter_remove(const String& p_name);
	static Vector<String> global_shader_parameter_get_list();

	static RSE::GlobalShaderParameterType global_shader_parameter_get_type(const String& p_name);

	static void global_shader_parameters_load_settings(bool p_load_textures = true);

	static void global_shader_parameters_clear() {}

	static int32_t global_shader_parameters_instance_allocate(RID p_instance) { return 0; }

	static void global_shader_parameters_instance_free(RID p_instance) {}

	/* SHADER API */
	static bool owns_shader(RID p_rid) { return data && data->shader_owner.owns(p_rid); }

	static RID shader_allocate();
	static void shader_initialize(RID p_rid, bool p_embedded = true);
	static void shader_free(RID p_rid);

	static void shader_set_code(RID p_shader, const String& p_code);

	static void shader_set_path_hint(RID p_shader, const String& p_code) {}

	static String shader_get_code(RID p_shader) { return ""; }

	static void shader_set_default_texture_parameter(
		RID p_shader, const String& p_name, RID p_texture, int p_index)
	{
	}

	static RID shader_get_default_texture_parameter(
		RID p_shader, const String& p_name, int p_index)
	{
		return RID();
	}

	static RenderingServerTypes::ShaderNativeSourceCode shader_get_native_source_code(RID p_shader)
	{
		return RenderingServerTypes::ShaderNativeSourceCode();
	}

	static void shader_embedded_set_lock() {}

	static const HashSet<RID>& shader_embedded_set_get()
	{
		return _get_data()->dummy_embedded_set;
	}

	static void shader_embedded_set_unlock() {}

	/* MATERIAL API */
	static bool owns_material(RID p_rid) { return data && data->material_owner.owns(p_rid); }

	static RID material_allocate();
	static void material_initialize(RID p_rid);
	static void material_free(RID p_rid);

	static void material_set_render_priority(RID p_material, int priority) {}

	static void material_set_shader(RID p_shader_material, RID p_shader);

	static void material_set_next_pass(RID p_material, RID p_next_material);

	static bool material_is_animated(RID p_material) { return false; }

	static bool material_casts_shadows(RID p_material) { return false; }

	static RSE::CullMode material_get_cull_mode(RID p_material) { return RSE::CULL_MODE_DISABLED; }

	static void material_get_instance_shader_parameters(
		RID p_material, List<InstanceShaderParam>* r_parameters);

	static void material_update_dependency(RID p_material, DependencyTracker* p_instance) {}
};

struct DummyMesh
{
	Vector<RenderingServerTypes::SurfaceData> surfaces;
	int blend_shape_count = 0;
	RSE::BlendShapeMode blend_shape_mode = RSE::BLEND_SHAPE_MODE_NORMALIZED;
	PackedFloat32Array blend_shape_values;
	Dependency dependency;
};

class MeshStorage final
{
private:
	struct DummyMultiMesh
	{
		PackedFloat32Array buffer;
	};

	struct Data
	{
		RID_Owner<DummyMesh> mesh_owner;
		RID_Owner<DummyMultiMesh> multimesh_owner;
	};

	static inline Data* data = nullptr;

	_FORCE_INLINE_ static Data* _get_data()
	{
		if (unlikely(!data)) {
			data = memnew(Data);
		}
		return data;
	}

public:
	MeshStorage() = delete;
	MeshStorage(const MeshStorage&) = delete;
	MeshStorage& operator=(const MeshStorage&) = delete;
	~MeshStorage() = delete;

	static void initialize()
	{
		if (!data) {
			data = memnew(Data);
		}
	}

	static void finalize()
	{
		if (data) {
			memdelete(data);
			data = nullptr;
		}
	}

	/* MESH API */
	static _FORCE_INLINE_ DummyMesh* get_mesh(RID p_rid)
	{
		return data ? data->mesh_owner.get_or_null(p_rid) : nullptr;
	}
	static bool owns_mesh(RID p_rid) { return data && data->mesh_owner.owns(p_rid); }

	static RID mesh_allocate() { return _get_data()->mesh_owner.allocate_rid(); }
	static void mesh_initialize(RID p_rid)
	{
		_get_data()->mesh_owner.initialize_rid(p_rid, DummyMesh());
	}
	static void mesh_free(RID p_rid)
	{
		if (!data) {
			return;
		}
		data->mesh_owner.free(p_rid);
	}

	static void mesh_set_blend_shape_count(RID p_mesh, int p_blend_shape_count)
	{
		DummyMesh* m = _get_data()->mesh_owner.get_or_null(p_mesh);
		ERR_FAIL_NULL(m);
		m->blend_shape_count = p_blend_shape_count;
	}

	static bool mesh_needs_instance(RID p_mesh, bool p_has_skeleton) { return false; }

	static void mesh_add_surface(RID p_mesh, const RenderingServerTypes::SurfaceData& p_surface)
	{
		DummyMesh* m = _get_data()->mesh_owner.get_or_null(p_mesh);
		ERR_FAIL_NULL(m);
		m->surfaces.push_back(p_surface);
	}

	static int mesh_get_blend_shape_count(RID p_mesh)
	{
		DummyMesh* m = _get_data()->mesh_owner.get_or_null(p_mesh);
		ERR_FAIL_NULL_V(m, 0);
		return m->blend_shape_count;
	}

	static void mesh_set_blend_shape_mode(RID p_mesh, RSE::BlendShapeMode p_mode)
	{
		DummyMesh* m = _get_data()->mesh_owner.get_or_null(p_mesh);
		ERR_FAIL_NULL(m);
		m->blend_shape_mode = p_mode;
	}

	static RSE::BlendShapeMode mesh_get_blend_shape_mode(RID p_mesh)
	{
		DummyMesh* m = _get_data()->mesh_owner.get_or_null(p_mesh);
		ERR_FAIL_NULL_V(m, RSE::BLEND_SHAPE_MODE_NORMALIZED);
		return m->blend_shape_mode;
	}

	static void mesh_surface_update_vertex_region(
		RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data)
	{
	}
	static void mesh_surface_update_attribute_region(
		RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data)
	{
	}
	static void mesh_surface_update_skin_region(
		RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data)
	{
	}
	static void mesh_surface_update_index_region(
		RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data)
	{
	}

	static void mesh_surface_set_material(RID p_mesh, int p_surface, RID p_material)
	{
		DummyMesh* m = _get_data()->mesh_owner.get_or_null(p_mesh);
		ERR_FAIL_NULL(m);
		ERR_FAIL_INDEX(p_surface, m->surfaces.size());
		m->surfaces.write[p_surface].material = p_material;
	}

	static RID mesh_surface_get_material(RID p_mesh, int p_surface)
	{
		DummyMesh* m = _get_data()->mesh_owner.get_or_null(p_mesh);
		ERR_FAIL_NULL_V(m, RID());
		ERR_FAIL_INDEX_V(p_surface, m->surfaces.size(), RID());
		return m->surfaces[p_surface].material;
	}

	static RenderingServerTypes::SurfaceData mesh_get_surface(RID p_mesh, int p_surface)
	{
		DummyMesh* m = _get_data()->mesh_owner.get_or_null(p_mesh);
		ERR_FAIL_NULL_V(m, RenderingServerTypes::SurfaceData());
		ERR_FAIL_INDEX_V(p_surface, m->surfaces.size(), RenderingServerTypes::SurfaceData());
		return m->surfaces[p_surface];
	}

	static RID mesh_surface_get_vertex_buffer_rd_rid(RID p_mesh, int p_surface) { return RID(); }
	static RID mesh_surface_get_attribute_buffer_rd_rid(RID p_mesh, int p_surface) { return RID(); }
	static RID mesh_surface_get_skin_buffer_rd_rid(RID p_mesh, int p_surface) { return RID(); }
	static RID mesh_surface_get_index_buffer_rd_rid(RID p_mesh, int p_surface) { return RID(); }

	static int mesh_get_surface_count(RID p_mesh)
	{
		DummyMesh* m = _get_data()->mesh_owner.get_or_null(p_mesh);
		ERR_FAIL_NULL_V(m, 0);
		return m->surfaces.size();
	}

	static void mesh_set_custom_aabb(RID p_mesh, const AABB& p_aabb) {}
	static AABB mesh_get_custom_aabb(RID p_mesh) { return AABB(); }
	static AABB mesh_get_aabb(RID p_mesh, RID p_skeleton) { return AABB(); }

	static void mesh_set_path(RID p_mesh, const String& p_path) {}
	static String mesh_get_path(RID p_mesh) { return String(); }
	static void mesh_set_shadow_mesh(RID p_mesh, RID p_shadow_mesh) {}

	static void mesh_clear(RID p_mesh)
	{
		DummyMesh* m = _get_data()->mesh_owner.get_or_null(p_mesh);
		ERR_FAIL_NULL(m);
		m->surfaces.clear();
	}

	static void mesh_surface_remove(RID p_mesh, int p_surface)
	{
		DummyMesh* m = _get_data()->mesh_owner.get_or_null(p_mesh);
		ERR_FAIL_NULL(m);
		ERR_FAIL_INDEX(p_surface, m->surfaces.size());
		m->surfaces.remove_at(p_surface);
	}

	static void mesh_debug_usage(List<RenderingServerTypes::MeshInfo>* r_info) {}

	/* MESH INSTANCE API */

	static RID mesh_instance_create(RID p_base) { return RID(); }
	static void mesh_instance_free(RID p_rid) {}
	static void mesh_instance_set_skeleton(RID p_mesh_instance, RID p_skeleton) {}
	static void mesh_instance_set_blend_shape_weight(
		RID p_mesh_instance, int p_shape, float p_weight)
	{
	}
	static void mesh_instance_check_for_update(RID p_mesh_instance) {}
	static void mesh_instance_set_canvas_item_transform(
		RID p_mesh_instance, const Transform2D& p_transform)
	{
	}
	static void update_mesh_instances() {}

	/* MULTIMESH API */
	static _FORCE_INLINE_ void multimesh_free(RID p_rid) { _multimesh_free(p_rid); }
	static bool owns_multimesh(RID p_rid) { return data && data->multimesh_owner.owns(p_rid); }

	static RID _multimesh_allocate() { return _get_data()->multimesh_owner.allocate_rid(); }
	static void _multimesh_initialize(RID p_rid)
	{
		_get_data()->multimesh_owner.initialize_rid(p_rid, DummyMultiMesh());
	}
	static void _multimesh_free(RID p_rid)
	{
		if (!data) {
			return;
		}
		data->multimesh_owner.free(p_rid);
	}

	static void _multimesh_allocate_data(RID p_multimesh, int p_instances,
		RSE::MultimeshTransformFormat p_transform_format, bool p_use_colors = false,
		bool p_use_custom_data = false, bool p_use_indirect = false)
	{
	}
	static int _multimesh_get_instance_count(RID p_multimesh) { return 0; }

	static void _multimesh_set_mesh(RID p_multimesh, RID p_mesh) {}
	static void _multimesh_instance_set_transform(
		RID p_multimesh, int p_index, const Transform3D& p_transform)
	{
	}
	static void _multimesh_instance_set_transform_2d(
		RID p_multimesh, int p_index, const Transform2D& p_transform)
	{
	}
	static void _multimesh_instance_set_color(RID p_multimesh, int p_index, const Color& p_color) {}
	static void _multimesh_instance_set_custom_data(
		RID p_multimesh, int p_index, const Color& p_color)
	{
	}

	static void _multimesh_set_custom_aabb(RID p_multimesh, const AABB& p_aabb) {}
	static AABB _multimesh_get_custom_aabb(RID p_multimesh) { return AABB(); }
	static RID _multimesh_get_mesh(RID p_multimesh) { return RID(); }

	static Transform3D _multimesh_instance_get_transform(RID p_multimesh, int p_index)
	{
		return Transform3D();
	}
	static Transform2D _multimesh_instance_get_transform_2d(RID p_multimesh, int p_index)
	{
		return Transform2D();
	}
	static Color _multimesh_instance_get_color(RID p_multimesh, int p_index) { return Color(); }
	static Color _multimesh_instance_get_custom_data(RID p_multimesh, int p_index)
	{
		return Color();
	}

	static void _multimesh_set_buffer(RID p_multimesh, const Vector<float>& p_buffer) {}
	static RID _multimesh_get_command_buffer_rd_rid(RID p_multimesh) { return RID(); }
	static RID _multimesh_get_buffer_rd_rid(RID p_multimesh) { return RID(); }
	static Vector<float> _multimesh_get_buffer(RID p_multimesh) { return Vector<float>(); }

	static void _multimesh_set_visible_instances(RID p_multimesh, int p_visible) {}
	static int _multimesh_get_visible_instances(RID p_multimesh) { return 0; }

	static AABB _multimesh_get_aabb(RID p_multimesh) { return AABB(); }

	static RendererMeshStorage::MultiMeshInterpolator* _multimesh_get_interpolator(RID p_multimesh)
	{
		return nullptr;
	}

	/* SKELETON API */

	static RID skeleton_allocate() { return RID(); }
	static void skeleton_initialize(RID p_rid) {}
	static void skeleton_free(RID p_rid) {}

	static void skeleton_allocate_data(RID p_skeleton, int p_bones, bool p_2d_skeleton = false) {}
	static void skeleton_set_base_transform_2d(
		RID p_skeleton, const Transform2D& p_base_transform)
	{
	}
	static int skeleton_get_bone_count(RID p_skeleton) { return 0; }
	static void skeleton_bone_set_transform(
		RID p_skeleton, int p_bone, const Transform3D& p_transform)
	{
	}
	static Transform3D skeleton_bone_get_transform(RID p_skeleton, int p_bone)
	{
		return Transform3D();
	}
	static void skeleton_bone_set_transform_2d(
		RID p_skeleton, int p_bone, const Transform2D& p_transform)
	{
	}
	static Transform2D skeleton_bone_get_transform_2d(RID p_skeleton, int p_bone)
	{
		return Transform2D();
	}

	static void skeleton_update_dependency(RID p_base, DependencyTracker* p_instance) {}
};

class ParticlesStorage final
{
public:
	ParticlesStorage() = delete;
	~ParticlesStorage() = delete;
	ParticlesStorage(const ParticlesStorage&) = delete;
	ParticlesStorage& operator=(const ParticlesStorage&) = delete;

	static void initialize() {}

	static void finalize() {}

	static bool is_initialized() { return true; }

	static bool free(RID p_rid) { return false; }

	/* PARTICLES */

	static bool owns_particles(RID p_rid) { return false; }

	static RID particles_allocate() { return RID(); }

	static void particles_initialize(RID p_rid) {}

	static void particles_free(RID p_rid) {}

	static void particles_set_mode(RID p_particles, RSE::ParticlesMode p_mode) {}

	static void particles_emit(RID p_particles, const Transform3D& p_transform,
		const Vector3& p_velocity, const Color& p_color, const Color& p_custom,
		uint32_t p_emit_flags)
	{
	}

	static void particles_set_emitting(RID p_particles, bool p_emitting) {}

	static void particles_set_amount(RID p_particles, int p_amount) {}

	static void particles_set_amount_ratio(RID p_particles, float p_amount_ratio) {}

	static void particles_set_lifetime(RID p_particles, double p_lifetime) {}

	static void particles_set_one_shot(RID p_particles, bool p_one_shot) {}

	static void particles_set_pre_process_time(RID p_particles, double p_time) {}

	static void particles_request_process_time(
		RID p_particles, real_t p_request_process_time, real_t p_particles_request_process_time)
	{
	}

	static void particles_set_explosiveness_ratio(RID p_particles, real_t p_ratio) {}

	static void particles_set_randomness_ratio(RID p_particles, real_t p_ratio) {}

	static void particles_set_seed(RID p_particles, uint32_t p_seed) {}

	static void particles_set_custom_aabb(RID p_particles, const AABB& p_aabb) {}

	static void particles_set_speed_scale(RID p_particles, double p_scale) {}

	static void particles_set_use_local_coordinates(RID p_particles, bool p_enable) {}

	static void particles_set_process_material(RID p_particles, RID p_material) {}

	static RID particles_get_process_material(RID p_particles) { return RID(); }

	static void particles_set_fixed_fps(RID p_particles, int p_fps) {}

	static void particles_set_interpolate(RID p_particles, bool p_enable) {}

	static void particles_set_fractional_delta(RID p_particles, bool p_enable) {}

	static void particles_set_subemitter(RID p_particles, RID p_subemitter_particles) {}

	static void particles_set_view_axis(
		RID p_particles, const Vector3& p_axis, const Vector3& p_up_axis)
	{
	}

	static void particles_set_collision_base_size(RID p_particles, real_t p_size) {}

	static void particles_set_transform_align(
		RID p_particles, RSE::ParticlesTransformAlign p_transform_align)
	{
	}

	static void particles_set_transform_align_channel_filter(
		RID p_particles, RSE::ParticlesTransformAlignCustomSrc p_transform_align_channel_filter)
	{
	}

	static void particles_set_transform_align_axis(
		RID p_particles, RSE::ParticlesTransformAlignAxis p_rotation_axis)
	{
	}

	static void particles_set_trails(RID p_particles, bool p_enable, double p_length) {}

	static void particles_set_trail_bind_poses(
		RID p_particles, const Vector<Transform3D>& p_bind_poses)
	{
	}

	static void particles_restart(RID p_particles) {}

	static void particles_set_draw_order(RID p_particles, RSE::ParticlesDrawOrder p_order) {}

	static void particles_set_draw_passes(RID p_particles, int p_count) {}

	static void particles_set_draw_pass_mesh(RID p_particles, int p_pass, RID p_mesh) {}

	static void particles_request_process(RID p_particles) {}

	static AABB particles_get_current_aabb(RID p_particles) { return AABB(); }

	static AABB particles_get_aabb(RID p_particles) { return AABB(); }

	static void particles_set_emission_transform(RID p_particles, const Transform3D& p_transform) {}

	static void particles_set_emitter_velocity(RID p_particles, const Vector3& p_velocity) {}

	static void particles_set_interp_to_end(RID p_particles, float p_interp) {}

	static bool particles_get_emitting(RID p_particles) { return false; }

	static int particles_get_draw_passes(RID p_particles) { return 0; }

	static RID particles_get_draw_pass_mesh(RID p_particles, int p_pass) { return RID(); }

	static void particles_add_collision(RID p_particles, RID p_instance) {}

	static void particles_remove_collision(RID p_particles, RID p_instance) {}

	static void update_particles() {}

	/* PARTICLES COLLISION */

	static bool owns_particles_collision(RID p_rid) { return false; }

	static RID particles_collision_allocate() { return RID(); }

	static void particles_collision_initialize(RID p_rid) {}

	static void particles_collision_free(RID p_rid) {}

	static void particles_collision_set_collision_type(
		RID p_particles_collision, RSE::ParticlesCollisionType p_type)
	{
	}

	static void particles_collision_set_cull_mask(RID p_particles_collision, uint32_t p_cull_mask)
	{
	}

	static void particles_collision_set_sphere_radius(RID p_particles_collision, real_t p_radius) {}

	static void particles_collision_set_box_extents(
		RID p_particles_collision, const Vector3& p_extents)
	{
	}

	static void particles_collision_set_attractor_strength(
		RID p_particles_collision, real_t p_strength)
	{
	}

	static void particles_collision_set_attractor_directionality(
		RID p_particles_collision, real_t p_directionality)
	{
	}

	static void particles_collision_set_attractor_attenuation(
		RID p_particles_collision, real_t p_curve)
	{
	}

	static void particles_collision_set_field_texture(RID p_particles_collision, RID p_texture) {}

	static void particles_collision_height_field_update(RID p_particles_collision) {}

	static void particles_collision_set_height_field_resolution(
		RID p_particles_collision, RSE::ParticlesCollisionHeightfieldResolution p_resolution)
	{
	}

	static AABB particles_collision_get_aabb(RID p_particles_collision) { return AABB(); }

	static bool particles_collision_is_heightfield(RID p_particles_collision) { return false; }

	static uint32_t particles_collision_get_height_field_mask(RID p_particles_collision)
	{
		return 0;
	}

	static void particles_collision_set_height_field_mask(
		RID p_particles_collision, uint32_t p_heightfield_mask)
	{
	}

	static uint32_t particles_collision_get_cull_mask(RID p_particles_collision) { return 0; }

	static bool owns_particles_collision_instance(RID p_rid) { return false; }

	static RID particles_collision_instance_create(RID p_collision) { return RID(); }

	static void particles_collision_instance_free(RID p_rid) {}

	static void particles_collision_instance_set_transform(
		RID p_collision_instance, const Transform3D& p_transform)
	{
	}

	static void particles_collision_instance_set_active(RID p_collision_instance, bool p_active) {}

	static bool particles_is_inactive(RID p_particles) { return false; }
};

struct DummyTexture
{
	String path;
	int width = 0;
	int height = 0;
	int depth = 0;
	int layers = 0;
	Image::Format format = Image::FORMAT_L8;
	Ref<Image> image;
};

struct DummyRenderTarget
{
	Point2i position;
	Size2i size;
	RID texture;
	bool direct_to_screen = false;
	bool is_transparent = false;
	bool was_used = false;
	bool clear_requested = false;
	Color clear_color;
	RSE::ViewportMSAA msaa = RSE::VIEWPORT_MSAA_DISABLED;
	bool use_hdr = false;
};

struct DummyCanvasTexture
{
	RID diffuse;
	RID normal_map;
	RID specular;
	Color specular_color = Color(1, 1, 1, 1);
	real_t shininess = 1.0;
	RSE::CanvasItemTextureFilter filter = RSE::CANVAS_ITEM_TEXTURE_FILTER_DEFAULT;
	RSE::CanvasItemTextureRepeat repeat = RSE::CANVAS_ITEM_TEXTURE_REPEAT_DEFAULT;
};

class TextureStorage final
{
private:
	struct Data
	{
		RID_Owner<DummyTexture> texture_owner;
		RID_Owner<DummyRenderTarget> render_target_owner;
		RID_Owner<DummyCanvasTexture> canvas_texture_owner;
		Color default_clear_color = Color(0, 0, 0, 1);
	};

	static inline Data* data = nullptr;

	_FORCE_INLINE_ static Data* _get_data()
	{
		if (unlikely(!data)) {
			data = memnew(Data);
		}
		return data;
	}

public:
	TextureStorage() = delete;
	TextureStorage(const TextureStorage&) = delete;
	TextureStorage& operator=(const TextureStorage&) = delete;
	~TextureStorage() = delete;

	static void initialize()
	{
		if (!data) {
			data = memnew(Data);
		}
	}

	static void finalize()
	{
		if (data) {
			memdelete(data);
			data = nullptr;
		}
	}

	static bool owns_texture(RID p_rid) { return data && data->texture_owner.owns(p_rid); }
	static bool owns_render_target(RID p_rid)
	{
		return data && data->render_target_owner.owns(p_rid);
	}
	static bool owns_canvas_texture(RID p_rid)
	{
		return data && data->canvas_texture_owner.owns(p_rid);
	}

	/* CANVAS TEXTURE API */

	static RID canvas_texture_allocate() { return _get_data()->canvas_texture_owner.allocate_rid(); }

	static void canvas_texture_initialize(RID p_rid)
	{
		_get_data()->canvas_texture_owner.initialize_rid(p_rid, DummyCanvasTexture());
	}

	static void canvas_texture_free(RID p_rid)
	{
		if (!data) {
			return;
		}
		data->canvas_texture_owner.free(p_rid);
	}

	static void canvas_texture_set_channel(
		RID p_canvas_texture, RSE::CanvasTextureChannel p_channel, RID p_texture)
	{
		DummyCanvasTexture* ct = _get_data()->canvas_texture_owner.get_or_null(p_canvas_texture);
		ERR_FAIL_NULL(ct);
		switch (p_channel) {
		case RSE::CANVAS_TEXTURE_CHANNEL_DIFFUSE:
			ct->diffuse = p_texture;
			break;
		case RSE::CANVAS_TEXTURE_CHANNEL_NORMAL:
			ct->normal_map = p_texture;
			break;
		case RSE::CANVAS_TEXTURE_CHANNEL_SPECULAR:
			ct->specular = p_texture;
			break;
		}
	}

	static void canvas_texture_set_shading_filter(
		RID p_canvas_texture, RSE::CanvasItemTextureFilter p_filter)
	{
		DummyCanvasTexture* ct = _get_data()->canvas_texture_owner.get_or_null(p_canvas_texture);
		ERR_FAIL_NULL(ct);
		ct->filter = p_filter;
	}

	static void canvas_texture_set_shading_parameters(
		RID p_canvas_texture, const Color& p_base_color, float p_shininess)
	{
		DummyCanvasTexture* ct = _get_data()->canvas_texture_owner.get_or_null(p_canvas_texture);
		ERR_FAIL_NULL(ct);
		ct->specular_color = p_base_color;
		ct->shininess = p_shininess;
	}

	static void canvas_texture_set_texture_filter(
		RID p_canvas_texture, RSE::CanvasItemTextureFilter p_filter)
	{
		DummyCanvasTexture* ct = _get_data()->canvas_texture_owner.get_or_null(p_canvas_texture);
		ERR_FAIL_NULL(ct);
		ct->filter = p_filter;
	}

	static void canvas_texture_set_texture_repeat(
		RID p_canvas_texture, RSE::CanvasItemTextureRepeat p_repeat)
	{
		DummyCanvasTexture* ct = _get_data()->canvas_texture_owner.get_or_null(p_canvas_texture);
		ERR_FAIL_NULL(ct);
		ct->repeat = p_repeat;
	}

	/* TEXTURE 2D / GENERAL API */

	static RID texture_allocate() { return _get_data()->texture_owner.allocate_rid(); }

	static void texture_initialize(RID p_rid)
	{
		_get_data()->texture_owner.initialize_rid(p_rid, DummyTexture());
	}

	static void texture_free(RID p_rid)
	{
		if (!data) {
			return;
		}
		data->texture_owner.free(p_rid);
	}

	static RID texture_2d_create(const Ref<Image>& p_image)
	{
		RID rid = _get_data()->texture_owner.allocate_rid();
		texture_2d_initialize(rid, p_image);
		return rid;
	}

	static void texture_2d_initialize(RID p_texture, const Ref<Image>& p_image)
	{
		DummyTexture t;
		if (p_image.is_valid()) {
			t.image = p_image;
			t.width = p_image->get_width();
			t.height = p_image->get_height();
			t.format = p_image->get_format();
		}
		_get_data()->texture_owner.initialize_rid(p_texture, t);
	}

	static void texture_2d_update(RID p_texture, const Ref<Image>& p_image, int p_layer)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL(t);
		t->image = p_image;
	}

	static Ref<Image> texture_2d_get(RID p_texture)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL_V(t, Ref<Image>());
		return t->image;
	}

	static Ref<Image> texture_2d_layer_get(RID p_texture, int p_layer)
	{
		return texture_2d_get(p_texture);
	}

	static void texture_2d_placeholder_initialize(RID p_texture) { texture_initialize(p_texture); }

	static void texture_2d_layered_initialize(
		RID p_texture, const Vector<Ref<Image>>& p_layers, RSE::TextureLayeredType p_layered_type)
	{
		DummyTexture t;
		t.layers = p_layers.size();
		if (!p_layers.is_empty() && p_layers[0].is_valid()) {
			t.image = p_layers[0];
			t.width = p_layers[0]->get_width();
			t.height = p_layers[0]->get_height();
			t.format = p_layers[0]->get_format();
		}
		_get_data()->texture_owner.initialize_rid(p_texture, t);
	}

	static void texture_2d_layered_placeholder_initialize(
		RID p_texture, RSE::TextureLayeredType p_layered_type)
	{
		texture_initialize(p_texture);
	}

	static void texture_3d_initialize(RID p_texture, Image::Format p_format, int p_width,
		int p_height, int p_depth, bool p_mipmaps, const Vector<Ref<Image>>& p_data)
	{
		DummyTexture t;
		t.width = p_width;
		t.height = p_height;
		t.depth = p_depth;
		t.format = p_format;
		_get_data()->texture_owner.initialize_rid(p_texture, t);
	}

	static void texture_3d_update(RID p_texture, const Vector<Ref<Image>>& p_data) {}

	static Vector<Ref<Image>> texture_3d_get(RID p_texture) { return Vector<Ref<Image>>(); }

	static void texture_3d_placeholder_initialize(RID p_texture) { texture_initialize(p_texture); }

	static void texture_proxy_initialize(RID p_texture, RID p_base)
	{
		texture_initialize(p_texture);
	}

	static void texture_proxy_update(RID p_proxy, RID p_base) {}

	static void texture_drawable_initialize(RID p_texture, int p_width, int p_height,
		RSE::TextureDrawableFormat p_format, const Color& p_color, bool p_has_mipmaps)
	{
		DummyTexture t;
		t.width = p_width;
		t.height = p_height;
		_get_data()->texture_owner.initialize_rid(p_texture, t);
	}

	static void texture_drawable_generate_mipmaps(RID p_texture) {}

	static RID texture_drawable_get_default_material() { return RID(); }

	static void texture_external_initialize(
		RID p_texture, int p_width, int p_height, uint64_t p_external_buffer)
	{
		DummyTexture t;
		t.width = p_width;
		t.height = p_height;
		_get_data()->texture_owner.initialize_rid(p_texture, t);
	}

	static void texture_external_update(
		RID p_texture, int p_width, int p_height, uint64_t p_external_buffer)
	{
	}

	static uint64_t texture_get_native_handle(RID p_texture, bool p_srgb) { return 0; }

	static void texture_set_format(RID p_texture, Image::Format p_format)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL(t);
		t->format = p_format;
	}

	static Image::Format texture_get_format(RID p_texture)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL_V(t, Image::FORMAT_L8);
		return t->format;
	}

	static void texture_set_width(RID p_texture, int p_width)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL(t);
		t->width = p_width;
	}

	static int texture_get_width(RID p_texture)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL_V(t, 0);
		return t->width;
	}

	static void texture_set_height(RID p_texture, int p_height)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL(t);
		t->height = p_height;
	}

	static int texture_get_height(RID p_texture)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL_V(t, 0);
		return t->height;
	}

	static void texture_set_depth(RID p_texture, int p_depth)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL(t);
		t->depth = p_depth;
	}

	static int texture_get_depth(RID p_texture)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL_V(t, 0);
		return t->depth;
	}

	static void texture_set_layers(RID p_texture, int p_layers)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL(t);
		t->layers = p_layers;
	}

	static int texture_get_layers(RID p_texture)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL_V(t, 0);
		return t->layers;
	}

	static void texture_set_path(RID p_texture, const String& p_path)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL(t);
		t->path = p_path;
	}

	static String texture_get_path(RID p_texture)
	{
		DummyTexture* t = _get_data()->texture_owner.get_or_null(p_texture);
		ERR_FAIL_NULL_V(t, String());
		return t->path;
	}

	static void texture_set_size_override(RID p_texture, int p_width, int p_height) {}

	static void texture_replace(RID p_texture, RID p_by_texture) {}

	static void texture_set_force_redraw_if_visible(RID p_texture, bool p_enable) {}

	static Size2 texture_size_with_proxy(RID p_proxy)
	{
		return Size2(texture_get_width(p_proxy), texture_get_height(p_proxy));
	}

	/* DECAL API */

	static RID decal_allocate() { return RID(); }
	static void decal_initialize(RID p_rid) {}
	static void decal_free(RID p_rid) {}
	static void decal_set_size(RID p_decal, const Vector3& p_size) {}
	static void decal_set_texture(RID p_decal, RSE::DecalTexture p_type, RID p_texture) {}
	static void decal_set_emission_energy(RID p_decal, float p_energy) {}
	static void decal_set_albedo_mix(RID p_decal, float p_mix) {}
	static void decal_set_modulate(RID p_decal, const Color& p_modulate) {}
	static void decal_set_cull_mask(RID p_decal, uint32_t p_layers) {}
	static void decal_set_distance_fade(RID p_decal, bool p_enabled, float p_begin, float p_length)
	{
	}
	static void decal_set_fade(RID p_decal, float p_above, float p_below) {}
	static void decal_set_normal_fade(RID p_decal, float p_fade) {}
	static AABB decal_get_aabb(RID p_decal) { return AABB(); }
	static uint32_t decal_get_cull_mask(RID p_decal) { return 0; }

	static RID decal_instance_create(RID p_decal) { return RID(); }
	static void decal_instance_free(RID p_decal_instance) {}
	static void decal_instance_set_transform(RID p_decal_instance, const Transform3D& p_transform)
	{
	}
	static void decal_instance_set_sorting_offset(RID p_decal_instance, float p_sorting_offset) {}

	/* RENDER TARGET API */

	static RID render_target_create()
	{
		Data* d = _get_data();
		RID rid = d->render_target_owner.allocate_rid();
		DummyRenderTarget rt;
		rt.texture = texture_allocate();
		texture_initialize(rt.texture);
		d->render_target_owner.initialize_rid(rid, rt);
		return rid;
	}

	static void render_target_free(RID p_render_target)
	{
		if (!data) {
			return;
		}
		DummyRenderTarget* rt = data->render_target_owner.get_or_null(p_render_target);
		if (rt) {
			if (rt->texture.is_valid()) {
				texture_free(rt->texture);
			}
			data->render_target_owner.free(p_render_target);
		}
	}

	static void render_target_set_position(RID p_render_target, int p_x, int p_y)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL(rt);
		rt->position = Point2i(p_x, p_y);
	}

	static Point2i render_target_get_position(RID p_render_target)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL_V(rt, Point2i());
		return rt->position;
	}

	static void render_target_set_size(
		RID p_render_target, int p_width, int p_height, uint32_t p_view_count)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL(rt);
		rt->size = Size2i(p_width, p_height);
	}

	static Size2i render_target_get_size(RID p_render_target)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL_V(rt, Size2i());
		return rt->size;
	}

	static void render_target_set_transparent(RID p_render_target, bool p_is_transparent)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL(rt);
		rt->is_transparent = p_is_transparent;
	}

	static bool render_target_is_transparent(RID p_render_target)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL_V(rt, false);
		return rt->is_transparent;
	}

	static void render_target_set_direct_to_screen(RID p_render_target, bool p_direct_to_screen)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL(rt);
		rt->direct_to_screen = p_direct_to_screen;
	}

	static bool render_target_is_direct_to_screen(RID p_render_target)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL_V(rt, false);
		return rt->direct_to_screen;
	}

	static RID render_target_get_texture(RID p_render_target)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL_V(rt, RID());
		return rt->texture;
	}

	static void render_target_set_msaa(RID p_render_target, RSE::ViewportMSAA p_msaa)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL(rt);
		rt->msaa = p_msaa;
	}

	static RSE::ViewportMSAA render_target_get_msaa(RID p_render_target)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL_V(rt, RSE::VIEWPORT_MSAA_DISABLED);
		return rt->msaa;
	}

	static bool render_target_get_msaa_needs_resolve(RID p_render_target) { return false; }

	static void render_target_do_msaa_resolve(RID p_render_target) {}

	static void render_target_set_use_hdr(RID p_render_target, bool p_use_hdr)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL(rt);
		rt->use_hdr = p_use_hdr;
	}

	static bool render_target_is_using_hdr(RID p_render_target)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL_V(rt, false);
		return rt->use_hdr;
	}

	static void render_target_set_use_debanding(RID p_render_target, bool p_use_debanding) {}

	static void render_target_set_vrs_mode(RID p_render_target, RSE::ViewportVRSMode p_mode) {}

	static void render_target_set_vrs_update_mode(
		RID p_render_target, RSE::ViewportVRSUpdateMode p_mode)
	{
	}

	static void render_target_set_vrs_texture(RID p_render_target, RID p_texture) {}

	static void render_target_request_clear(RID p_render_target, const Color& p_clear_color)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL(rt);
		rt->clear_requested = true;
		rt->clear_color = p_clear_color;
	}

	static bool render_target_is_clear_requested(RID p_render_target)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL_V(rt, false);
		return rt->clear_requested;
	}

	static Color render_target_get_clear_request_color(RID p_render_target)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL_V(rt, Color());
		return rt->clear_color;
	}

	static void render_target_do_clear_request(RID p_render_target)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL(rt);
		rt->clear_requested = false;
	}

	static void render_target_set_sdf_size_and_scale(
		RID p_render_target, RSE::ViewportSDFOversize p_size, RSE::ViewportSDFScale p_scale)
	{
	}

	static Rect2i render_target_get_sdf_rect(RID p_render_target) { return Rect2i(); }

	static void render_target_mark_sdf_enabled(RID p_render_target, bool p_enabled) {}

	static void render_target_set_override(RID p_render_target, RID p_color_texture,
		RID p_depth_texture, RID p_velocity, RID p_velocity_depth)
	{
	}

	static RID render_target_get_override_color(RID p_render_target) { return RID(); }
	static RID render_target_get_override_depth(RID p_render_target) { return RID(); }
	static RID render_target_get_override_velocity(RID p_render_target) { return RID(); }
	static RID render_target_get_override_velocity_depth(RID p_render_target) { return RID(); }

	static void render_target_set_render_region(RID p_render_target, const Rect2i& p_render_region)
	{
	}

	static Rect2i render_target_get_render_region(RID p_render_target) { return Rect2i(); }

	static void render_target_set_velocity_target_size(
		RID p_render_target, const Size2i& p_target_size)
	{
	}

	static Size2i render_target_get_velocity_target_size(RID p_render_target) { return Size2i(); }

	static bool render_target_was_used(RID p_render_target)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL_V(rt, false);
		return rt->was_used;
	}

	static void render_target_set_as_unused(RID p_render_target)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL(rt);
		rt->was_used = false;
	}

	static void render_target_set_as_current(RID p_render_target)
	{
		DummyRenderTarget* rt = _get_data()->render_target_owner.get_or_null(p_render_target);
		ERR_FAIL_NULL(rt);
		rt->was_used = true;
	}

	static void set_default_clear_color(const Color& p_color)
	{
		_get_data()->default_clear_color = p_color;
	}

	static Color get_default_clear_color() { return _get_data()->default_clear_color; }
};

class Utilities final
{
public:
	Utilities() = delete;
	Utilities(const Utilities&) = delete;
	Utilities& operator=(const Utilities&) = delete;
	~Utilities() = delete;

	static void initialize() {}
	static void finalize() {}

	static RSE::InstanceType get_base_type(RID p_rid);
	static bool free(RID p_rid);

	static void base_update_dependency(RID p_base, DependencyTracker* p_instance);

	static RID visibility_notifier_allocate() { return RID(); }
	static void visibility_notifier_initialize(RID p_notifier) {}
	static void visibility_notifier_free(RID p_notifier) {}
	static void visibility_notifier_set_aabb(RID p_notifier, const AABB& p_aabb) {}
	static AABB visibility_notifier_get_aabb(RID p_notifier) { return AABB(); }
	static void visibility_notifier_call(RID p_notifier, bool p_enter, bool p_deferred) {}

	static void capture_timestamps_begin() {}
	static void capture_timestamp(const String& p_name) {}
	static uint32_t get_captured_timestamps_count() { return 0; }
	static uint64_t get_captured_timestamps_frame() { return 0; }
	static uint64_t get_captured_timestamp_gpu_time(uint32_t p_index) { return 0; }
	static uint64_t get_captured_timestamp_cpu_time(uint32_t p_index) { return 0; }
	static String get_captured_timestamp_name(uint32_t p_index) { return String(); }

	static void update_dirty_resources() {}
	static void set_debug_generate_wireframes(bool p_generate) {}

	static bool has_os_feature(const String& p_feature)
	{
		return p_feature == "rgtc" || p_feature == "bptc" || p_feature == "s3tc" ||
			   p_feature == "etc2";
	}

	static void update_memory_info() {}

	static uint64_t get_rendering_info(RSE::RenderingInfo p_info) { return 0; }

	static String get_video_adapter_name() { return String(); }
	static String get_video_adapter_vendor() { return String(); }
	static RenderingDeviceEnums::DeviceType get_video_adapter_type()
	{
		return RenderingDeviceEnums::DeviceType::DEVICE_TYPE_OTHER;
	}
	static String get_video_adapter_api_version() { return String(); }

	static Size2i get_maximum_viewport_size() { return Size2i(); }

	static uint32_t get_maximum_shader_varyings() { return 31; }

	static uint64_t get_maximum_uniform_buffer_size() { return 65536; }
};

} // namespace RendererDummy

class RasterizerCanvasDummy final
{
public:
	static void initialize() {}

	static void finalize() {}

	static void set_time(double p_time) {}

	static void update() {}

	static bool free(RID p_rid) { return false; }

	static PolygonID request_polygon(const Vector<int>& p_indices, const Vector<Point2>& p_points,
		const Vector<Color>& p_colors, const Vector<Point2>& p_uvs = Vector<Point2>(),
		const Vector<int>& p_bones = Vector<int>(),
		const Vector<float>& p_weights = Vector<float>(), int p_count = -1)
	{
		return 0;
	}

	static void free_polygon(PolygonID p_polygon) {}

	static void canvas_render_items(RID p_to_render_target, RCR::Item* p_item_list,
		const Color& p_modulate, RendererCanvasRender::Light* p_light_list,
		RendererCanvasRender::Light* p_directional_list, const Transform2D& p_canvas_transform,
		RSE::CanvasItemTextureFilter p_default_filter,
		RSE::CanvasItemTextureRepeat p_default_repeat, bool p_snap_2d_vertices_to_pixel,
		bool& r_sdf_used, RenderingServerTypes::RenderInfo* r_render_info = nullptr)
	{
	}

	static void set_debug_redraw(bool p_enabled, double p_time, const Color& p_color) {}

	static uint32_t get_pipeline_compilations(RSE::PipelineSource p_source) { return 0; }

	static RID light_create() { return RID(); }

	static void light_set_texture(RID p_rid, RID p_texture) {}

	static void light_set_use_shadow(RID p_rid, bool p_enable) {}

	static void light_update_shadow(RID p_rid, int p_shadow_index, const Transform2D& p_light_xform,
		int p_light_mask, float p_near, float p_far,
		RendererCanvasRender::LightOccluderInstance* p_occluders, const Rect2& p_light_rect)
	{
	}

	static void light_update_directional_shadow(RID p_rid, int p_shadow_index,
		const Transform2D& p_light_xform, int p_light_mask, float p_cull_distance,
		const Rect2& p_clip_rect, RendererCanvasRender::LightOccluderInstance* p_occluders)
	{
	}

	static void render_sdf(
		RID p_render_target, RendererCanvasRender::LightOccluderInstance* p_occluders)
	{
	}

	static RID occluder_polygon_create() { return RID(); }

	static void occluder_polygon_set_shape(
		RID p_occluder, const Vector<Vector2>& p_points, bool p_closed)
	{
	}

	static void occluder_polygon_set_cull_mode(
		RID p_occluder, RSE::CanvasOccluderPolygonCullMode p_mode)
	{
	}

	static void set_shadow_texture_size(int p_size) {}

	RasterizerCanvasDummy() = delete;
	~RasterizerCanvasDummy() = delete;
};

class RasterizerSceneDummy final : public RendererSceneRender
{
public:
	class GeometryInstanceDummy final : public RenderGeometryInstance
	{
	public:
		GeometryInstanceDummy() {}

		virtual void set_transform(const Transform3D& p_transform, const AABB& p_aabb,
			const AABB& p_transformed_aabb) override
		{
		}
	};

	PagedAllocator<GeometryInstanceDummy> geometry_instance_alloc;

public:
	RenderGeometryInstance* geometry_instance_create(RID p_base) override
	{
		RSE::InstanceType type = RendererDummy::Utilities::get_base_type(p_base);
		ERR_FAIL_COND_V(!((1 << type) & RSE::INSTANCE_GEOMETRY_MASK), nullptr);

		GeometryInstanceDummy* ginstance = geometry_instance_alloc.alloc();

		return ginstance;
	}

	void geometry_instance_free(RenderGeometryInstance* p_geometry_instance) override
	{
		GeometryInstanceDummy* ginstance = static_cast<GeometryInstanceDummy*>(p_geometry_instance);
		ERR_FAIL_NULL(ginstance);

		geometry_instance_alloc.free(ginstance);
	}

	uint32_t geometry_instance_get_pair_mask() override { return 0; }

	virtual uint32_t get_max_lights_total() override { return 0; }

	virtual uint32_t get_max_lights_per_mesh() override { return 0; }

	/* PIPELINES */

	virtual void mesh_generate_pipelines(RID p_mesh, bool p_background_compilation) override {}

	virtual uint32_t get_pipeline_compilations(RSE::PipelineSource p_source) override { return 0; }

	/* SDFGI UPDATE */

	void sdfgi_update(const Ref<RenderSceneBuffers>& p_render_buffers, RID p_environment,
		const Vector3& p_world_position) override
	{
	}

	int sdfgi_get_pending_region_count(
		const Ref<RenderSceneBuffers>& p_render_buffers) const override
	{
		return 0;
	}

	AABB sdfgi_get_pending_region_bounds(
		const Ref<RenderSceneBuffers>& p_render_buffers, int p_region) const override
	{
		return AABB();
	}

	uint32_t sdfgi_get_pending_region_cascade(
		const Ref<RenderSceneBuffers>& p_render_buffers, int p_region) const override
	{
		return 0;
	}

	/* SKY API */

	RID sky_allocate() override { return RID(); }

	void sky_initialize(RID p_rid) override {}

	void sky_set_radiance_size(RID p_sky, int p_radiance_size) override {}

	void sky_set_mode(RID p_sky, RSE::SkyMode p_samples) override {}

	void sky_set_material(RID p_sky, RID p_material) override {}

	Ref<Image> sky_bake_panorama(
		RID p_sky, float p_energy, bool p_bake_irradiance, const Size2i& p_size) override
	{
		return Ref<Image>();
	}

	/* ENVIRONMENT API */

	void environment_glow_set_use_bicubic_upscale(bool p_enable) override {}

	void environment_set_ssr_half_size(bool p_half_size) override {}

	void environment_set_ssr_roughness_quality(
		RSE::EnvironmentSSRRoughnessQuality p_quality) override
	{
	}

	void environment_set_ssao_quality(RSE::EnvironmentSSAOQuality p_quality, bool p_half_size,
		float p_adaptive_target, int p_blur_passes, float p_fadeout_from,
		float p_fadeout_to) override
	{
	}

	void environment_set_ssil_quality(RSE::EnvironmentSSILQuality p_quality, bool p_half_size,
		float p_adaptive_target, int p_blur_passes, float p_fadeout_from,
		float p_fadeout_to) override
	{
	}

	void environment_set_sdfgi_ray_count(RSE::EnvironmentSDFGIRayCount p_ray_count) override {}

	void environment_set_sdfgi_frames_to_converge(
		RSE::EnvironmentSDFGIFramesToConverge p_frames) override
	{
	}

	void environment_set_sdfgi_frames_to_update_light(
		RSE::EnvironmentSDFGIFramesToUpdateLight p_update) override
	{
	}

	void environment_set_volumetric_fog_volume_size(int p_size, int p_depth) override {}

	void environment_set_volumetric_fog_filter_active(bool p_enable) override {}

	Ref<Image> environment_bake_panorama(
		RID p_env, bool p_bake_irradiance, const Size2i& p_size) override
	{
		return Ref<Image>();
	}

	void positional_soft_shadow_filter_set_quality(RSE::ShadowQuality p_quality) override {}

	void directional_soft_shadow_filter_set_quality(RSE::ShadowQuality p_quality) override {}

	RID fog_volume_instance_create(RID p_fog_volume) override { return RID(); }

	void fog_volume_instance_set_transform(
		RID p_fog_volume_instance, const Transform3D& p_transform) override
	{
	}

	void fog_volume_instance_set_active(RID p_fog_volume_instance, bool p_active) override {}

	RID fog_volume_instance_get_volume(RID p_fog_volume_instance) const override { return RID(); }

	Vector3 fog_volume_instance_get_position(RID p_fog_volume_instance) const override
	{
		return Vector3();
	}

	RID voxel_gi_instance_create(RID p_voxel_gi) override { return RID(); }

	void voxel_gi_instance_set_transform_to_data(RID p_probe, const Transform3D& p_xform) override
	{
	}

	bool voxel_gi_needs_update(RID p_probe) const override { return false; }

	void voxel_gi_update(RID p_probe, bool p_update_light_instances,
		const Vector<RID>& p_light_instances,
		const PagedArray<RenderGeometryInstance*>& p_dynamic_objects) override
	{
	}

	void voxel_gi_set_quality(RSE::VoxelGIQuality) override {}

	void render_scene(const Ref<RenderSceneBuffers>& p_render_buffers,
		const CameraData* p_camera_data, const CameraData* p_prev_camera_data,
		const PagedArray<RenderGeometryInstance*>& p_instances, const PagedArray<RID>& p_lights,
		const PagedArray<RID>& p_reflection_probes, const PagedArray<RID>& p_voxel_gi_instances,
		const PagedArray<RID>& p_decals, const PagedArray<RID>& p_lightmaps,
		const PagedArray<RID>& p_fog_volumes, RID p_environment, RID p_camera_attributes,
		RID p_compositor, RID p_shadow_atlas, RID p_occluder_debug_tex, RID p_reflection_atlas,
		RID p_reflection_probe, int p_reflection_probe_pass, float p_screen_mesh_lod_threshold,
		const RenderShadowData* p_render_shadows, int p_render_shadow_count,
		const RenderSDFGIData* p_render_sdfgi_regions, int p_render_sdfgi_region_count,
		float p_window_output_max_value, const RenderSDFGIUpdateData* p_sdfgi_update_data = nullptr,
		RenderingServerTypes::RenderInfo* r_info = nullptr) override
	{
	}

	void render_material(const Transform3D& p_cam_transform, const Projection& p_cam_projection,
		bool p_cam_orthogonal, const PagedArray<RenderGeometryInstance*>& p_instances,
		RID p_framebuffer, const Rect2i& p_region) override
	{
	}

	void render_particle_collider_heightfield(RID p_collider, const Transform3D& p_transform,
		const PagedArray<RenderGeometryInstance*>& p_instances) override
	{
	}

	void set_scene_pass(uint64_t p_pass) override {}

	void set_time(double p_time, double p_step) override {}

	void set_debug_draw_mode(RSE::ViewportDebugDraw p_debug_draw) override {}

	Ref<RenderSceneBuffers> render_buffers_create() override { return Ref<RenderSceneBuffers>(); }

	void gi_set_use_half_resolution(bool p_enable) override {}

	void screen_space_roughness_limiter_set_active(
		bool p_enable, float p_amount, float p_curve) override
	{
	}

	bool screen_space_roughness_limiter_is_active() const { return false; }

	void sub_surface_scattering_set_quality(RSE::SubSurfaceScatteringQuality p_quality) override {}

	void sub_surface_scattering_set_scale(float p_scale, float p_depth_scale) override {}

	bool free(RID p_rid) override;

	void update() override {}

	void sdfgi_set_debug_probe_select(const Vector3& p_position, const Vector3& p_dir) override {}

	virtual void decals_set_filter(RSE::DecalFilter p_filter) override {}

	virtual void light_projectors_set_filter(RSE::LightProjectorFilter p_filter) override {}

	virtual void lightmaps_set_bicubic_filter(bool p_enable) override {}

	virtual void material_set_use_debanding(bool p_enable) override {}

	RasterizerSceneDummy() {}

	~RasterizerSceneDummy() {}
};

class RasterizerDummy final
{
private:
	static inline uint64_t frame = 1;
	static inline double delta = 0.0;
	static inline double time = 0.0;

	static inline RasterizerCanvasDummy* canvas = nullptr;
	static inline RasterizerSceneDummy* scene = nullptr;

	static inline RendererDummy::Fog* fog = nullptr;
	static inline RendererDummy::GI* gi = nullptr;
	static inline RendererDummy::LightStorage* light_storage = nullptr;
	static inline RendererDummy::MaterialStorage* material_storage = nullptr;
	static inline RendererDummy::MeshStorage* mesh_storage = nullptr;
	static inline RendererDummy::ParticlesStorage* particles_storage = nullptr;
	static inline RendererDummy::TextureStorage* texture_storage = nullptr;
	static inline RendererDummy::Utilities* utilities = nullptr;

public:
	static RendererSceneRender* get_scene();

	static RendererFog* get_fog();

	static RendererDummy::Utilities* get_utilities();

	static void set_boot_image_with_stretch(const Ref<Image>& p_image, const Color& p_color,
		RSE::SplashStretchMode p_stretch_mode, bool p_use_filter = true)
	{
	}

	static void initialize();

	static void begin_frame(double frame_step);

	static void blit_render_targets_to_screen(DisplayServerEnums::WindowID p_screen,
		const RenderingServerTypes::BlitToScreen* p_render_targets, int p_amount)
	{
	}

	_ALWAYS_INLINE_ static bool is_opengl() { return false; }

	_ALWAYS_INLINE_ static void gl_end_frame(bool p_swap_buffers) {}

	static void end_frame(bool p_present);

	static void finalize();

	static Error _create_current()
	{
		RasterizerDummy::initialize();
		return OK;
	}

	static void make_current();

	_ALWAYS_INLINE_ static uint64_t get_frame_number() { return frame; }

	_ALWAYS_INLINE_ static double get_frame_delta_time() { return delta; }

	_ALWAYS_INLINE_ static double get_total_time() { return time; }

	_ALWAYS_INLINE_ static bool can_create_resources_async() { return false; }

public:
	static void bind_mesh_storage()
	{
		RendererMeshStorage::mesh_allocate = &RendererDummy::MeshStorage::mesh_allocate;
		RendererMeshStorage::mesh_initialize = &RendererDummy::MeshStorage::mesh_initialize;
		RendererMeshStorage::mesh_free = &RendererDummy::MeshStorage::mesh_free;
		RendererMeshStorage::mesh_set_blend_shape_count =
			&RendererDummy::MeshStorage::mesh_set_blend_shape_count;
		RendererMeshStorage::mesh_needs_instance = &RendererDummy::MeshStorage::mesh_needs_instance;
		RendererMeshStorage::mesh_add_surface = &RendererDummy::MeshStorage::mesh_add_surface;
		RendererMeshStorage::mesh_get_blend_shape_count =
			&RendererDummy::MeshStorage::mesh_get_blend_shape_count;
		RendererMeshStorage::mesh_set_blend_shape_mode =
			&RendererDummy::MeshStorage::mesh_set_blend_shape_mode;
		RendererMeshStorage::mesh_get_blend_shape_mode =
			&RendererDummy::MeshStorage::mesh_get_blend_shape_mode;
		RendererMeshStorage::mesh_surface_update_vertex_region =
			&RendererDummy::MeshStorage::mesh_surface_update_vertex_region;
		RendererMeshStorage::mesh_surface_update_attribute_region =
			&RendererDummy::MeshStorage::mesh_surface_update_attribute_region;
		RendererMeshStorage::mesh_surface_update_skin_region =
			&RendererDummy::MeshStorage::mesh_surface_update_skin_region;
		RendererMeshStorage::mesh_surface_update_index_region =
			&RendererDummy::MeshStorage::mesh_surface_update_index_region;
		RendererMeshStorage::mesh_surface_set_material =
			&RendererDummy::MeshStorage::mesh_surface_set_material;
		RendererMeshStorage::mesh_surface_get_material =
			&RendererDummy::MeshStorage::mesh_surface_get_material;
		RendererMeshStorage::mesh_get_surface = &RendererDummy::MeshStorage::mesh_get_surface;
		RendererMeshStorage::mesh_surface_get_vertex_buffer_rd_rid =
			&RendererDummy::MeshStorage::mesh_surface_get_vertex_buffer_rd_rid;
		RendererMeshStorage::mesh_surface_get_attribute_buffer_rd_rid =
			&RendererDummy::MeshStorage::mesh_surface_get_attribute_buffer_rd_rid;
		RendererMeshStorage::mesh_surface_get_skin_buffer_rd_rid =
			&RendererDummy::MeshStorage::mesh_surface_get_skin_buffer_rd_rid;
		RendererMeshStorage::mesh_surface_get_index_buffer_rd_rid =
			&RendererDummy::MeshStorage::mesh_surface_get_index_buffer_rd_rid;
		RendererMeshStorage::mesh_get_surface_count =
			&RendererDummy::MeshStorage::mesh_get_surface_count;
		RendererMeshStorage::mesh_set_custom_aabb =
			&RendererDummy::MeshStorage::mesh_set_custom_aabb;
		RendererMeshStorage::mesh_get_custom_aabb =
			&RendererDummy::MeshStorage::mesh_get_custom_aabb;
		RendererMeshStorage::mesh_get_aabb = &RendererDummy::MeshStorage::mesh_get_aabb;
		RendererMeshStorage::mesh_set_path = &RendererDummy::MeshStorage::mesh_set_path;
		RendererMeshStorage::mesh_get_path = &RendererDummy::MeshStorage::mesh_get_path;
		RendererMeshStorage::mesh_set_shadow_mesh =
			&RendererDummy::MeshStorage::mesh_set_shadow_mesh;
		RendererMeshStorage::mesh_clear = &RendererDummy::MeshStorage::mesh_clear;
		RendererMeshStorage::mesh_surface_remove = &RendererDummy::MeshStorage::mesh_surface_remove;
		RendererMeshStorage::mesh_debug_usage = &RendererDummy::MeshStorage::mesh_debug_usage;

		RendererMeshStorage::mesh_instance_create =
			&RendererDummy::MeshStorage::mesh_instance_create;
		RendererMeshStorage::mesh_instance_free = &RendererDummy::MeshStorage::mesh_instance_free;
		RendererMeshStorage::mesh_instance_set_skeleton =
			&RendererDummy::MeshStorage::mesh_instance_set_skeleton;
		RendererMeshStorage::mesh_instance_set_blend_shape_weight =
			&RendererDummy::MeshStorage::mesh_instance_set_blend_shape_weight;
		RendererMeshStorage::mesh_instance_check_for_update =
			&RendererDummy::MeshStorage::mesh_instance_check_for_update;
		RendererMeshStorage::mesh_instance_set_canvas_item_transform =
			&RendererDummy::MeshStorage::mesh_instance_set_canvas_item_transform;
		RendererMeshStorage::update_mesh_instances =
			&RendererDummy::MeshStorage::update_mesh_instances;

		RendererMeshStorage::_multimesh_allocate = &RendererDummy::MeshStorage::_multimesh_allocate;
		RendererMeshStorage::_multimesh_initialize =
			&RendererDummy::MeshStorage::_multimesh_initialize;
		RendererMeshStorage::_multimesh_free = &RendererDummy::MeshStorage::_multimesh_free;
		RendererMeshStorage::_multimesh_allocate_data =
			&RendererDummy::MeshStorage::_multimesh_allocate_data;
		RendererMeshStorage::_multimesh_get_instance_count =
			&RendererDummy::MeshStorage::_multimesh_get_instance_count;
		RendererMeshStorage::_multimesh_set_mesh = &RendererDummy::MeshStorage::_multimesh_set_mesh;
		RendererMeshStorage::_multimesh_instance_set_transform =
			&RendererDummy::MeshStorage::_multimesh_instance_set_transform;
		RendererMeshStorage::_multimesh_instance_set_transform_2d =
			&RendererDummy::MeshStorage::_multimesh_instance_set_transform_2d;
		RendererMeshStorage::_multimesh_instance_set_color =
			&RendererDummy::MeshStorage::_multimesh_instance_set_color;
		RendererMeshStorage::_multimesh_instance_set_custom_data =
			&RendererDummy::MeshStorage::_multimesh_instance_set_custom_data;
		RendererMeshStorage::_multimesh_set_custom_aabb =
			&RendererDummy::MeshStorage::_multimesh_set_custom_aabb;
		RendererMeshStorage::_multimesh_get_custom_aabb =
			&RendererDummy::MeshStorage::_multimesh_get_custom_aabb;
		RendererMeshStorage::_multimesh_get_mesh = &RendererDummy::MeshStorage::_multimesh_get_mesh;
		RendererMeshStorage::_multimesh_instance_get_transform =
			&RendererDummy::MeshStorage::_multimesh_instance_get_transform;
		RendererMeshStorage::_multimesh_instance_get_transform_2d =
			&RendererDummy::MeshStorage::_multimesh_instance_get_transform_2d;
		RendererMeshStorage::_multimesh_instance_get_color =
			&RendererDummy::MeshStorage::_multimesh_instance_get_color;
		RendererMeshStorage::_multimesh_instance_get_custom_data =
			&RendererDummy::MeshStorage::_multimesh_instance_get_custom_data;
		RendererMeshStorage::_multimesh_set_buffer =
			&RendererDummy::MeshStorage::_multimesh_set_buffer;
		RendererMeshStorage::_multimesh_get_command_buffer_rd_rid =
			&RendererDummy::MeshStorage::_multimesh_get_command_buffer_rd_rid;
		RendererMeshStorage::_multimesh_get_buffer_rd_rid =
			&RendererDummy::MeshStorage::_multimesh_get_buffer_rd_rid;
		RendererMeshStorage::_multimesh_get_buffer =
			&RendererDummy::MeshStorage::_multimesh_get_buffer;
		RendererMeshStorage::_multimesh_set_visible_instances =
			&RendererDummy::MeshStorage::_multimesh_set_visible_instances;
		RendererMeshStorage::_multimesh_get_visible_instances =
			&RendererDummy::MeshStorage::_multimesh_get_visible_instances;
		RendererMeshStorage::_multimesh_get_aabb = &RendererDummy::MeshStorage::_multimesh_get_aabb;
		RendererMeshStorage::_multimesh_get_interpolator =
			&RendererDummy::MeshStorage::_multimesh_get_interpolator;

		RendererMeshStorage::skeleton_allocate = &RendererDummy::MeshStorage::skeleton_allocate;
		RendererMeshStorage::skeleton_initialize = &RendererDummy::MeshStorage::skeleton_initialize;
		RendererMeshStorage::skeleton_free = &RendererDummy::MeshStorage::skeleton_free;
		RendererMeshStorage::skeleton_allocate_data =
			&RendererDummy::MeshStorage::skeleton_allocate_data;
		RendererMeshStorage::skeleton_set_base_transform_2d =
			&RendererDummy::MeshStorage::skeleton_set_base_transform_2d;
		RendererMeshStorage::skeleton_get_bone_count =
			&RendererDummy::MeshStorage::skeleton_get_bone_count;
		RendererMeshStorage::skeleton_bone_set_transform =
			&RendererDummy::MeshStorage::skeleton_bone_set_transform;
		RendererMeshStorage::skeleton_bone_get_transform =
			&RendererDummy::MeshStorage::skeleton_bone_get_transform;
		RendererMeshStorage::skeleton_bone_set_transform_2d =
			&RendererDummy::MeshStorage::skeleton_bone_set_transform_2d;
		RendererMeshStorage::skeleton_bone_get_transform_2d =
			&RendererDummy::MeshStorage::skeleton_bone_get_transform_2d;
		RendererMeshStorage::skeleton_update_dependency =
			&RendererDummy::MeshStorage::skeleton_update_dependency;
	}

	static void bind_texture_storage();

	static void bind_material_storage();

	static void bind_canvas();

	static void bind_light_storage() { RendererLightStorage::bind<RendererDummy::LightStorage>(); }

	static void bind_particles_storage();

	static void bind_utilities();

	static void bind_gi() { RendererGI::bind<RendererDummy::GI>(); }

	static void bind_fog() { RendererFog::bind<RendererDummy::Fog>(); }

	RasterizerDummy() = delete;
	RasterizerDummy(const RasterizerDummy&) = delete;
	~RasterizerDummy() = delete;
};

using RDummy = RasterizerDummy;
