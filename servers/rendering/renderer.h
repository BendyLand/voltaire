/**************************************************************************/
/*  renderer.h                                                            */
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

#include "core/io/image.h"
#include "core/os/thread_safe.h"
#include "core/templates/local_vector.h"
#include "core/templates/rid.h"
#include "core/templates/rid_owner.h"
#include "servers/display/display_server_enums.h"
// --- Inlined/Folded from servers/rendering/environment/renderer_fog.h ---
#include "core/math/aabb.h"
#include "core/math/vector3.h"
#include "core/templates/rid.h"
#include "servers/rendering/rendering_server_enums.h"

class RendererFog
{
public:
	/* FOG VOLUMES FUNCTION POINTERS */
	inline static bool (*owns_fog_volume)(RID p_rid) = nullptr;

	inline static RID (*fog_volume_allocate)() = nullptr;
	inline static void (*fog_volume_initialize)(RID p_rid) = nullptr;
	inline static void (*fog_volume_free)(RID p_rid) = nullptr;

	inline static void (*fog_volume_set_shape)(RID p_fog_volume, RSE::FogVolumeShape p_shape) = nullptr;
	inline static void (*fog_volume_set_size)(RID p_fog_volume, const Vector3 &p_size) = nullptr;
	inline static void (*fog_volume_set_material)(RID p_fog_volume, RID p_material) = nullptr;
	inline static AABB (*fog_volume_get_aabb)(RID p_fog_volume) = nullptr;
	inline static RSE::FogVolumeShape (*fog_volume_get_shape)(RID p_fog_volume) = nullptr;

	/* BIND TEMPLATE */
	template <typename T>
	static void bind()
	{
		owns_fog_volume = T::owns_fog_volume;

		fog_volume_allocate = T::fog_volume_allocate;
		fog_volume_initialize = T::fog_volume_initialize;
		fog_volume_free = T::fog_volume_free;

		fog_volume_set_shape = T::fog_volume_set_shape;
		fog_volume_set_size = T::fog_volume_set_size;
		fog_volume_set_material = T::fog_volume_set_material;
		fog_volume_get_aabb = T::fog_volume_get_aabb;
		fog_volume_get_shape = T::fog_volume_get_shape;
	}
};
// --- Folded from servers/rendering/environment/renderer_gi.h ---
#include "core/math/projection.h"
#include "core/math/transform_3d.h"
#include "core/templates/rid.h"
#include "servers/rendering/rendering_server_enums.h"

class RendererGI
{
public:
	/* VOXEL GI FUNCTION POINTERS */
	inline static bool (*owns_voxel_gi)(RID p_rid) = nullptr;
	inline static RID (*voxel_gi_allocate)() = nullptr;
	inline static void (*voxel_gi_free)(RID p_voxel_gi) = nullptr;
	inline static void (*voxel_gi_initialize)(RID p_voxel_gi) = nullptr;

	inline static void (*voxel_gi_allocate_data)(RID p_voxel_gi, const Transform3D& p_to_cell_xform,
		const AABB& p_aabb, const Vector3i& p_octree_size, const Vector<uint8_t>& p_octree_cells,
		const Vector<uint8_t>& p_data_cells, const Vector<uint8_t>& p_distance_field,
		const Vector<int>& p_level_counts) = nullptr;

	inline static AABB (*voxel_gi_get_bounds)(RID p_voxel_gi) = nullptr;
	inline static Vector3i (*voxel_gi_get_octree_size)(RID p_voxel_gi) = nullptr;
	inline static Vector<uint8_t> (*voxel_gi_get_octree_cells)(RID p_voxel_gi) = nullptr;
	inline static Vector<uint8_t> (*voxel_gi_get_data_cells)(RID p_voxel_gi) = nullptr;
	inline static Vector<uint8_t> (*voxel_gi_get_distance_field)(RID p_voxel_gi) = nullptr;
	inline static Vector<int> (*voxel_gi_get_level_counts)(RID p_voxel_gi) = nullptr;
	inline static Transform3D (*voxel_gi_get_to_cell_xform)(RID p_voxel_gi) = nullptr;

	inline static void (*voxel_gi_set_dynamic_range)(RID p_voxel_gi, float p_range) = nullptr;
	inline static float (*voxel_gi_get_dynamic_range)(RID p_voxel_gi) = nullptr;

	inline static void (*voxel_gi_set_propagation)(RID p_voxel_gi, float p_range) = nullptr;
	inline static float (*voxel_gi_get_propagation)(RID p_voxel_gi) = nullptr;

	inline static void (*voxel_gi_set_energy)(RID p_voxel_gi, float p_energy) = nullptr;
	inline static float (*voxel_gi_get_energy)(RID p_voxel_gi) = nullptr;

	inline static void (*voxel_gi_set_baked_exposure_normalization)(
		RID p_voxel_gi, float p_baked_exposure) = nullptr;
	inline static float (*voxel_gi_get_baked_exposure_normalization)(RID p_voxel_gi) = nullptr;

	inline static void (*voxel_gi_set_bias)(RID p_voxel_gi, float p_bias) = nullptr;
	inline static float (*voxel_gi_get_bias)(RID p_voxel_gi) = nullptr;

	inline static void (*voxel_gi_set_normal_bias)(RID p_voxel_gi, float p_range) = nullptr;
	inline static float (*voxel_gi_get_normal_bias)(RID p_voxel_gi) = nullptr;

	inline static void (*voxel_gi_set_interior)(RID p_voxel_gi, bool p_enable) = nullptr;
	inline static bool (*voxel_gi_is_interior)(RID p_voxel_gi) = nullptr;

	inline static void (*voxel_gi_set_use_two_bounces)(RID p_voxel_gi, bool p_enable) = nullptr;
	inline static bool (*voxel_gi_is_using_two_bounces)(RID p_voxel_gi) = nullptr;

	inline static uint32_t (*voxel_gi_get_version)(RID p_probe) = nullptr;

	/* SDFGI FUNCTION POINTERS */
	inline static void (*sdfgi_reset)() = nullptr;

	/* BIND TEMPLATE */
	template <typename T> static void bind()
	{
		owns_voxel_gi = T::owns_voxel_gi;
		voxel_gi_allocate = T::voxel_gi_allocate;
		voxel_gi_free = T::voxel_gi_free;
		voxel_gi_initialize = T::voxel_gi_initialize;
		voxel_gi_allocate_data = T::voxel_gi_allocate_data;
		voxel_gi_get_bounds = T::voxel_gi_get_bounds;
		voxel_gi_get_octree_size = T::voxel_gi_get_octree_size;
		voxel_gi_get_octree_cells = T::voxel_gi_get_octree_cells;
		voxel_gi_get_data_cells = T::voxel_gi_get_data_cells;
		voxel_gi_get_distance_field = T::voxel_gi_get_distance_field;
		voxel_gi_get_level_counts = T::voxel_gi_get_level_counts;
		voxel_gi_get_to_cell_xform = T::voxel_gi_get_to_cell_xform;
		voxel_gi_set_dynamic_range = T::voxel_gi_set_dynamic_range;
		voxel_gi_get_dynamic_range = T::voxel_gi_get_dynamic_range;
		voxel_gi_set_propagation = T::voxel_gi_set_propagation;
		voxel_gi_get_propagation = T::voxel_gi_get_propagation;
		voxel_gi_set_energy = T::voxel_gi_set_energy;
		voxel_gi_get_energy = T::voxel_gi_get_energy;
		voxel_gi_set_baked_exposure_normalization = T::voxel_gi_set_baked_exposure_normalization;
		voxel_gi_get_baked_exposure_normalization = T::voxel_gi_get_baked_exposure_normalization;
		voxel_gi_set_bias = T::voxel_gi_set_bias;
		voxel_gi_get_bias = T::voxel_gi_get_bias;
		voxel_gi_set_normal_bias = T::voxel_gi_set_normal_bias;
		voxel_gi_get_normal_bias = T::voxel_gi_get_normal_bias;
		voxel_gi_set_interior = T::voxel_gi_set_interior;
		voxel_gi_is_interior = T::voxel_gi_is_interior;
		voxel_gi_set_use_two_bounces = T::voxel_gi_set_use_two_bounces;
		voxel_gi_is_using_two_bounces = T::voxel_gi_is_using_two_bounces;
		voxel_gi_get_version = T::voxel_gi_get_version;
		sdfgi_reset = T::sdfgi_reset;
	}
};

// --- End servers/rendering/environment/renderer_gi.h ---
#include "servers/rendering/renderer_compositor.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/rendering_device_commons.h"
#include "servers/rendering/rendering_server_enums.h"
#include "servers/rendering/rendering_server_types.h"
// --- Folded from servers/rendering/storage/camera_attributes_storage.h ---
#include "core/templates/rid_owner.h"
#include "servers/rendering/rendering_server_enums.h"

class RendererCameraAttributes final
{
public:
	RendererCameraAttributes() = delete;
	~RendererCameraAttributes() = delete;
	RendererCameraAttributes(const RendererCameraAttributes&) = delete;
	RendererCameraAttributes& operator=(const RendererCameraAttributes&) = delete;

	struct CameraAttributes
	{
		float exposure_multiplier = 1.0;
		float exposure_normalization = 1.0;
		float exposure_sensitivity = 100.0; // In ISO.

		bool use_auto_exposure = false;
		float auto_exposure_min_sensitivity = 50.0;
		float auto_exposure_max_sensitivity = 800.0;
		float auto_exposure_adjust_speed = 1.0;
		float auto_exposure_scale = 1.0;
		uint64_t auto_exposure_version = 0;

		bool dof_blur_far_enabled = false;
		float dof_blur_far_distance = 10;
		float dof_blur_far_transition = 5;
		bool dof_blur_near_enabled = false;
		float dof_blur_near_distance = 2;
		float dof_blur_near_transition = 1;
		float dof_blur_amount = 0.1;
	};

private:
	static inline bool _initialized = false;

	static inline RSE::DOFBlurQuality dof_blur_quality = RSE::DOF_BLUR_QUALITY_MEDIUM;
	static inline RSE::DOFBokehShape dof_blur_bokeh_shape = RSE::DOF_BOKEH_HEXAGON;
	static inline bool dof_blur_use_jitter = false;
	static inline uint64_t auto_exposure_counter = 2;

	static inline RID_Owner<CameraAttributes, true> camera_attributes_owner;

public:
	static void initialize();
	static void finalize();

	static bool is_initialized() { return _initialized; }

	static CameraAttributes* get_camera_attributes(RID p_rid)
	{
		return camera_attributes_owner.get_or_null(p_rid);
	}
	static bool owns_camera_attributes(RID p_rid) { return camera_attributes_owner.owns(p_rid); }

	static RID camera_attributes_allocate();
	static void camera_attributes_initialize(RID p_rid);
	static void camera_attributes_free(RID p_rid);

	static void camera_attributes_set_dof_blur_quality(
		RSE::DOFBlurQuality p_quality, bool p_use_jitter);
	static void camera_attributes_set_dof_blur_bokeh_shape(RSE::DOFBokehShape p_shape);

	static void camera_attributes_set_dof_blur(RID p_camera_attributes, bool p_far_enable,
		float p_far_distance, float p_far_transition, bool p_near_enable, float p_near_distance,
		float p_near_transition, float p_amount);
	static bool camera_attributes_get_dof_far_enabled(RID p_camera_attributes);
	static float camera_attributes_get_dof_far_distance(RID p_camera_attributes);
	static float camera_attributes_get_dof_far_transition(RID p_camera_attributes);
	static bool camera_attributes_get_dof_near_enabled(RID p_camera_attributes);
	static float camera_attributes_get_dof_near_distance(RID p_camera_attributes);
	static float camera_attributes_get_dof_near_transition(RID p_camera_attributes);
	static float camera_attributes_get_dof_blur_amount(RID p_camera_attributes);

	_FORCE_INLINE_ static bool camera_attributes_uses_dof(RID p_camera_attributes)
	{
		CameraAttributes* cam_attributes = camera_attributes_owner.get_or_null(p_camera_attributes);

		return cam_attributes &&
			   (cam_attributes->dof_blur_near_enabled || cam_attributes->dof_blur_far_enabled) &&
			   cam_attributes->dof_blur_amount > 0.0;
	}

	static void camera_attributes_set_exposure(
		RID p_camera_attributes, float p_multiplier, float p_exposure_normalization);
	static float camera_attributes_get_exposure_normalization_factor(RID p_camera_attributes);

	static void camera_attributes_set_auto_exposure(RID p_camera_attributes, bool p_enable,
		float p_min_sensitivity, float p_max_sensitivity, float p_speed, float p_scale);
	static float camera_attributes_get_auto_exposure_min_sensitivity(RID p_camera_attributes);
	static float camera_attributes_get_auto_exposure_max_sensitivity(RID p_camera_attributes);
	static float camera_attributes_get_auto_exposure_adjust_speed(RID p_camera_attributes);
	static float camera_attributes_get_auto_exposure_scale(RID p_camera_attributes);
	static uint64_t camera_attributes_get_auto_exposure_version(RID p_camera_attributes);

	_FORCE_INLINE_ static bool camera_attributes_uses_auto_exposure(RID p_camera_attributes)
	{
		CameraAttributes* cam_attributes = camera_attributes_owner.get_or_null(p_camera_attributes);

		return cam_attributes && cam_attributes->use_auto_exposure;
	}

	_FORCE_INLINE_ static RSE::DOFBlurQuality camera_attributes_get_dof_blur_quality()
	{
		return dof_blur_quality;
	}

	_FORCE_INLINE_ static RSE::DOFBokehShape camera_attributes_get_dof_blur_bokeh_shape()
	{
		return dof_blur_bokeh_shape;
	}

	_FORCE_INLINE_ static bool camera_attributes_get_dof_blur_use_jitter()
	{
		return dof_blur_use_jitter;
	}
};

// --- End servers/rendering/storage/camera_attributes_storage.h ---
// --- Inlined/Folded from servers/rendering/storage/light_storage.h ---
#include "core/math/aabb.h"
#include "core/math/color.h"
#include "core/math/projection.h"
#include "core/math/transform_3d.h"
#include "core/math/vector2.h"
#include "core/math/vector3.h"
#include "core/templates/rid.h"
#include "servers/rendering/rendering_server_enums.h"
#include "servers/rendering/renderer_scene_render.h"

namespace LightStorage
{

struct API
{
	/* Light API */
	RID (*directional_light_allocate)() = nullptr;
	void (*directional_light_initialize)(RID p_rid) = nullptr;

	RID (*omni_light_allocate)() = nullptr;
	void (*omni_light_initialize)(RID p_rid) = nullptr;

	RID (*spot_light_allocate)() = nullptr;
	void (*spot_light_initialize)(RID p_rid) = nullptr;

	RID (*area_light_allocate)() = nullptr;
	void (*area_light_initialize)(RID p_rid) = nullptr;

	void (*light_free)(RID p_rid) = nullptr;

	void (*light_set_color)(RID p_light, const Color& p_color) = nullptr;
	void (*light_set_param)(RID p_light, RSE::LightParam p_param, float p_value) = nullptr;
	void (*light_set_shadow)(RID p_light, bool p_enabled) = nullptr;
	void (*light_set_projector)(RID p_light, RID p_texture) = nullptr;
	void (*light_set_negative)(RID p_light, bool p_enable) = nullptr;
	void (*light_set_cull_mask)(RID p_light, uint32_t p_mask) = nullptr;
	void (*light_set_distance_fade)(
		RID p_light, bool p_enabled, float p_begin, float p_shadow, float p_length) = nullptr;
	void (*light_set_reverse_cull_face_mode)(RID p_light, bool p_enabled) = nullptr;
	void (*light_set_shadow_caster_mask)(RID p_light, uint32_t p_caster_mask) = nullptr;
	uint32_t (*light_get_shadow_caster_mask)(RID p_light) = nullptr;
	void (*light_set_bake_mode)(RID p_light, RSE::LightBakeMode p_bake_mode) = nullptr;
	void (*light_set_max_sdfgi_cascade)(RID p_light, uint32_t p_cascade) = nullptr;

	void (*light_omni_set_shadow_mode)(RID p_light, RSE::LightOmniShadowMode p_mode) = nullptr;

	void (*light_directional_set_shadow_mode)(
		RID p_light, RSE::LightDirectionalShadowMode p_mode) = nullptr;
	void (*light_directional_set_blend_splits)(RID p_light, bool p_enable) = nullptr;
	bool (*light_directional_get_blend_splits)(RID p_light) = nullptr;
	void (*light_directional_set_sky_mode)(
		RID p_light, RSE::LightDirectionalSkyMode p_mode) = nullptr;
	RSE::LightDirectionalSkyMode (*light_directional_get_sky_mode)(RID p_light) = nullptr;

	RSE::LightDirectionalShadowMode (*light_directional_get_shadow_mode)(RID p_light) = nullptr;
	RSE::LightOmniShadowMode (*light_omni_get_shadow_mode)(RID p_light) = nullptr;

	void (*light_area_set_size)(RID p_light, const Vector2& p_size) = nullptr;
	Vector2 (*light_area_get_size)(RID p_light) = nullptr;
	void (*light_area_set_normalize_energy)(RID p_light, bool p_enabled) = nullptr;
	bool (*light_area_get_normalize_energy)(RID p_light) = nullptr;
	void (*light_area_set_texture)(RID p_light, RID p_texture) = nullptr;
	RID (*light_area_get_texture)(RID p_light) = nullptr;

	bool (*light_has_shadow)(RID p_light) = nullptr;
	bool (*light_has_projector)(RID p_light) = nullptr;

	RSE::LightType (*light_get_type)(RID p_light) = nullptr;
	AABB (*light_get_aabb)(RID p_light) = nullptr;
	float (*light_get_param)(RID p_light, RSE::LightParam p_param) = nullptr;
	Color (*light_get_color)(RID p_light) = nullptr;
	bool (*light_get_reverse_cull_face_mode)(RID p_light) = nullptr;
	RSE::LightBakeMode (*light_get_bake_mode)(RID p_light) = nullptr;
	uint32_t (*light_get_max_sdfgi_cascade)(RID p_light) = nullptr;
	uint64_t (*light_get_version)(RID p_light) = nullptr;
	uint32_t (*light_get_cull_mask)(RID p_light) = nullptr;

	/* Light Instance API */
	RID (*light_instance_create)(RID p_light) = nullptr;
	void (*light_instance_free)(RID p_light_instance) = nullptr;
	void (*light_instance_set_transform)(
		RID p_light_instance, const Transform3D& p_transform) = nullptr;
	void (*light_instance_set_aabb)(RID p_light_instance, const AABB& p_aabb) = nullptr;
	void (*light_instance_set_shadow_transform)(RID p_light_instance,
		const Projection& p_projection, const Transform3D& p_transform, float p_far, float p_split,
		int p_pass, float p_shadow_texel_size, float p_bias_scale, float p_range_begin,
		const Vector2& p_uv_scale) = nullptr;
	void (*light_instance_mark_visible)(RID p_light_instance) = nullptr;
	bool (*light_instance_is_shadow_visible_at_position)(
		RID p_light, const Vector3& p_position) = nullptr;

	/* Probe API */
	RID (*reflection_probe_allocate)() = nullptr;
	void (*reflection_probe_initialize)(RID p_rid) = nullptr;
	void (*reflection_probe_free)(RID p_rid) = nullptr;

	void (*reflection_probe_set_update_mode)(
		RID p_probe, RSE::ReflectionProbeUpdateMode p_mode) = nullptr;
	void (*reflection_probe_set_resolution)(RID p_probe, int p_resolution) = nullptr;
	void (*reflection_probe_set_intensity)(RID p_probe, float p_intensity) = nullptr;
	void (*reflection_probe_set_blend_distance)(RID p_probe, float p_blend_distance) = nullptr;
	void (*reflection_probe_set_ambient_mode)(
		RID p_probe, RSE::ReflectionProbeAmbientMode p_mode) = nullptr;
	void (*reflection_probe_set_ambient_color)(RID p_probe, const Color& p_color) = nullptr;
	void (*reflection_probe_set_ambient_energy)(RID p_probe, float p_energy) = nullptr;
	void (*reflection_probe_set_max_distance)(RID p_probe, float p_distance) = nullptr;
	void (*reflection_probe_set_size)(RID p_probe, const Vector3& p_size) = nullptr;
	void (*reflection_probe_set_origin_offset)(RID p_probe, const Vector3& p_offset) = nullptr;
	void (*reflection_probe_set_as_interior)(RID p_probe, bool p_enable) = nullptr;
	void (*reflection_probe_set_enable_box_projection)(RID p_probe, bool p_enable) = nullptr;
	void (*reflection_probe_set_enable_shadows)(RID p_probe, bool p_enable) = nullptr;
	void (*reflection_probe_set_cull_mask)(RID p_probe, uint32_t p_layers) = nullptr;
	void (*reflection_probe_set_reflection_mask)(RID p_probe, uint32_t p_layers) = nullptr;
	void (*reflection_probe_set_mesh_lod_threshold)(RID p_probe, float p_ratio) = nullptr;

	AABB (*reflection_probe_get_aabb)(RID p_probe) = nullptr;
	RSE::ReflectionProbeUpdateMode (*reflection_probe_get_update_mode)(RID p_probe) = nullptr;
	uint32_t (*reflection_probe_get_cull_mask)(RID p_probe) = nullptr;
	uint32_t (*reflection_probe_get_reflection_mask)(RID p_probe) = nullptr;
	Vector3 (*reflection_probe_get_size)(RID p_probe) = nullptr;
	Vector3 (*reflection_probe_get_origin_offset)(RID p_probe) = nullptr;
	float (*reflection_probe_get_origin_max_distance)(RID p_probe) = nullptr;
	bool (*reflection_probe_renders_shadows)(RID p_probe) = nullptr;
	float (*reflection_probe_get_mesh_lod_threshold)(RID p_probe) = nullptr;

	/* Reflection Atlas */
	RID (*reflection_atlas_create)() = nullptr;
	void (*reflection_atlas_free)(RID p_ref_atlas) = nullptr;
	void (*reflection_atlas_set_size)(
		RID p_ref_atlas, int p_reflection_size, int p_reflection_count) = nullptr;
	int (*reflection_atlas_get_size)(RID p_ref_atlas) = nullptr;

	/* Reflection Probe Instance */
	RID (*reflection_probe_instance_create)(RID p_probe) = nullptr;
	void (*reflection_probe_instance_free)(RID p_instance) = nullptr;
	void (*reflection_probe_instance_set_transform)(
		RID p_instance, const Transform3D& p_transform) = nullptr;
	bool (*reflection_probe_has_atlas_index)(RID p_instance) = nullptr;
	void (*reflection_probe_release_atlas_index)(RID p_instance) = nullptr;
	bool (*reflection_probe_instance_needs_redraw)(RID p_instance) = nullptr;
	bool (*reflection_probe_instance_has_reflection)(RID p_instance) = nullptr;
	bool (*reflection_probe_instance_begin_render)(
		RID p_instance, RID p_reflection_atlas) = nullptr;
	bool (*reflection_probe_instance_end_render)(RID p_instance, RID p_reflection_atlas) = nullptr;
	Ref<RenderSceneBuffers> (*reflection_probe_atlas_get_render_buffers)(
		RID p_reflection_atlas) = nullptr;
	bool (*reflection_probe_instance_postprocess_step)(RID p_instance) = nullptr;

	/* Lightmap */
	RID (*lightmap_allocate)() = nullptr;
	void (*lightmap_initialize)(RID p_rid) = nullptr;
	void (*lightmap_free)(RID p_rid) = nullptr;

	void (*lightmap_set_textures)(
		RID p_lightmap, RID p_light, bool p_uses_spherical_harmonics) = nullptr;
	void (*lightmap_set_probe_bounds)(RID p_lightmap, const AABB& p_bounds) = nullptr;
	void (*lightmap_set_probe_interior)(RID p_lightmap, bool p_interior) = nullptr;
	void (*lightmap_set_probe_capture_data)(RID p_lightmap, const PackedVector3Array& p_points,
		const PackedColorArray& p_point_sh, const PackedInt32Array& p_tetrahedra,
		const PackedInt32Array& p_bsp_tree) = nullptr;
	void (*lightmap_set_baked_exposure_normalization)(RID p_lightmap, float p_exposure) = nullptr;
	PackedVector3Array (*lightmap_get_probe_capture_points)(RID p_lightmap) = nullptr;
	PackedColorArray (*lightmap_get_probe_capture_sh)(RID p_lightmap) = nullptr;
	PackedInt32Array (*lightmap_get_probe_capture_tetrahedra)(RID p_lightmap) = nullptr;
	PackedInt32Array (*lightmap_get_probe_capture_bsp_tree)(RID p_lightmap) = nullptr;
	AABB (*lightmap_get_aabb)(RID p_lightmap) = nullptr;
	void (*lightmap_tap_sh_light)(RID p_lightmap, const Vector3& p_point, Color* r_sh) = nullptr;
	bool (*lightmap_is_interior)(RID p_lightmap) = nullptr;
	void (*lightmap_set_probe_capture_update_speed)(float p_speed) = nullptr;
	float (*lightmap_get_probe_capture_update_speed)() = nullptr;

	void (*lightmap_set_shadowmask_textures)(RID p_lightmap, RID p_shadow) = nullptr;
	RSE::ShadowmaskMode (*lightmap_get_shadowmask_mode)(RID p_lightmap) = nullptr;
	void (*lightmap_set_shadowmask_mode)(RID p_lightmap, RSE::ShadowmaskMode p_mode) = nullptr;

	/* Lightmap Instance */
	RID (*lightmap_instance_create)(RID p_lightmap) = nullptr;
	void (*lightmap_instance_free)(RID p_lightmap) = nullptr;
	void (*lightmap_instance_set_transform)(
		RID p_lightmap, const Transform3D& p_transform) = nullptr;

	/* Shadow Atlas */
	RID (*shadow_atlas_create)() = nullptr;
	void (*shadow_atlas_free)(RID p_atlas) = nullptr;
	void (*shadow_atlas_set_size)(RID p_atlas, int p_size, bool p_use_16_bits) = nullptr;
	void (*shadow_atlas_set_quadrant_subdivision)(
		RID p_atlas, int p_quadrant, int p_subdivision) = nullptr;
	bool (*shadow_atlas_update_light)(
		RID p_atlas, RID p_light_instance, float p_coverage, uint64_t p_light_version) = nullptr;
	void (*shadow_atlas_update)(RID p_atlas) = nullptr;

	void (*directional_shadow_atlas_set_size)(int p_size, bool p_16_bits) = nullptr;
	int (*get_directional_light_shadow_size)(RID p_light_instance) = nullptr;
	void (*set_directional_shadow_count)(int p_count) = nullptr;
}; // struct API

}; // namespace LightStorage

class RendererLightStorage final
{
private:
	static inline LightStorage::API api;

public:
	RendererLightStorage() = delete;
	RendererLightStorage(const RendererLightStorage&) = delete;
	RendererLightStorage& operator=(const RendererLightStorage&) = delete;
	~RendererLightStorage() = delete;

	template <typename StorageImpl> static void bind()
	{
		/* Light API */
		api.directional_light_allocate = &StorageImpl::directional_light_allocate;
		api.directional_light_initialize = &StorageImpl::directional_light_initialize;
		api.omni_light_allocate = &StorageImpl::omni_light_allocate;
		api.omni_light_initialize = &StorageImpl::omni_light_initialize;
		api.spot_light_allocate = &StorageImpl::spot_light_allocate;
		api.spot_light_initialize = &StorageImpl::spot_light_initialize;
		api.area_light_allocate = &StorageImpl::area_light_allocate;
		api.area_light_initialize = &StorageImpl::area_light_initialize;
		api.light_free = &StorageImpl::light_free;

		api.light_set_color = &StorageImpl::light_set_color;
		api.light_set_param = &StorageImpl::light_set_param;
		api.light_set_shadow = &StorageImpl::light_set_shadow;
		api.light_set_projector = &StorageImpl::light_set_projector;
		api.light_set_negative = &StorageImpl::light_set_negative;
		api.light_set_cull_mask = &StorageImpl::light_set_cull_mask;
		api.light_set_distance_fade = &StorageImpl::light_set_distance_fade;
		api.light_set_reverse_cull_face_mode = &StorageImpl::light_set_reverse_cull_face_mode;
		api.light_set_shadow_caster_mask = &StorageImpl::light_set_shadow_caster_mask;
		api.light_get_shadow_caster_mask = &StorageImpl::light_get_shadow_caster_mask;
		api.light_set_bake_mode = &StorageImpl::light_set_bake_mode;
		api.light_set_max_sdfgi_cascade = &StorageImpl::light_set_max_sdfgi_cascade;

		api.light_omni_set_shadow_mode = &StorageImpl::light_omni_set_shadow_mode;

		api.light_directional_set_shadow_mode = &StorageImpl::light_directional_set_shadow_mode;
		api.light_directional_set_blend_splits = &StorageImpl::light_directional_set_blend_splits;
		api.light_directional_get_blend_splits = &StorageImpl::light_directional_get_blend_splits;
		api.light_directional_set_sky_mode = &StorageImpl::light_directional_set_sky_mode;
		api.light_directional_get_sky_mode = &StorageImpl::light_directional_get_sky_mode;

		api.light_directional_get_shadow_mode = &StorageImpl::light_directional_get_shadow_mode;
		api.light_omni_get_shadow_mode = &StorageImpl::light_omni_get_shadow_mode;

		api.light_area_set_size = &StorageImpl::light_area_set_size;
		api.light_area_get_size = &StorageImpl::light_area_get_size;
		api.light_area_set_normalize_energy = &StorageImpl::light_area_set_normalize_energy;
		api.light_area_get_normalize_energy = &StorageImpl::light_area_get_normalize_energy;
		api.light_area_set_texture = &StorageImpl::light_area_set_texture;
		api.light_area_get_texture = &StorageImpl::light_area_get_texture;

		api.light_has_shadow = &StorageImpl::light_has_shadow;
		api.light_has_projector = &StorageImpl::light_has_projector;

		api.light_get_type = &StorageImpl::light_get_type;
		api.light_get_aabb = &StorageImpl::light_get_aabb;
		api.light_get_param = &StorageImpl::light_get_param;
		api.light_get_color = &StorageImpl::light_get_color;
		api.light_get_reverse_cull_face_mode = &StorageImpl::light_get_reverse_cull_face_mode;
		api.light_get_bake_mode = &StorageImpl::light_get_bake_mode;
		api.light_get_max_sdfgi_cascade = &StorageImpl::light_get_max_sdfgi_cascade;
		api.light_get_version = &StorageImpl::light_get_version;
		api.light_get_cull_mask = &StorageImpl::light_get_cull_mask;

		/* Light Instance API */
		api.light_instance_create = &StorageImpl::light_instance_create;
		api.light_instance_free = &StorageImpl::light_instance_free;
		api.light_instance_set_transform = &StorageImpl::light_instance_set_transform;
		api.light_instance_set_aabb = &StorageImpl::light_instance_set_aabb;
		api.light_instance_set_shadow_transform = &StorageImpl::light_instance_set_shadow_transform;
		api.light_instance_mark_visible = &StorageImpl::light_instance_mark_visible;
		api.light_instance_is_shadow_visible_at_position =
			&StorageImpl::light_instance_is_shadow_visible_at_position;

		/* Probe API */
		api.reflection_probe_allocate = &StorageImpl::reflection_probe_allocate;
		api.reflection_probe_initialize = &StorageImpl::reflection_probe_initialize;
		api.reflection_probe_free = &StorageImpl::reflection_probe_free;

		api.reflection_probe_set_update_mode = &StorageImpl::reflection_probe_set_update_mode;
		api.reflection_probe_set_resolution = &StorageImpl::reflection_probe_set_resolution;
		api.reflection_probe_set_intensity = &StorageImpl::reflection_probe_set_intensity;
		api.reflection_probe_set_blend_distance = &StorageImpl::reflection_probe_set_blend_distance;
		api.reflection_probe_set_ambient_mode = &StorageImpl::reflection_probe_set_ambient_mode;
		api.reflection_probe_set_ambient_color = &StorageImpl::reflection_probe_set_ambient_color;
		api.reflection_probe_set_ambient_energy = &StorageImpl::reflection_probe_set_ambient_energy;
		api.reflection_probe_set_max_distance = &StorageImpl::reflection_probe_set_max_distance;
		api.reflection_probe_set_size = &StorageImpl::reflection_probe_set_size;
		api.reflection_probe_set_origin_offset = &StorageImpl::reflection_probe_set_origin_offset;
		api.reflection_probe_set_as_interior = &StorageImpl::reflection_probe_set_as_interior;
		api.reflection_probe_set_enable_box_projection =
			&StorageImpl::reflection_probe_set_enable_box_projection;
		api.reflection_probe_set_enable_shadows = &StorageImpl::reflection_probe_set_enable_shadows;
		api.reflection_probe_set_cull_mask = &StorageImpl::reflection_probe_set_cull_mask;
		api.reflection_probe_set_reflection_mask =
			&StorageImpl::reflection_probe_set_reflection_mask;
		api.reflection_probe_set_mesh_lod_threshold =
			&StorageImpl::reflection_probe_set_mesh_lod_threshold;

		api.reflection_probe_get_aabb = &StorageImpl::reflection_probe_get_aabb;
		api.reflection_probe_get_update_mode = &StorageImpl::reflection_probe_get_update_mode;
		api.reflection_probe_get_cull_mask = &StorageImpl::reflection_probe_get_cull_mask;
		api.reflection_probe_get_reflection_mask =
			&StorageImpl::reflection_probe_get_reflection_mask;
		api.reflection_probe_get_size = &StorageImpl::reflection_probe_get_size;
		api.reflection_probe_get_origin_offset = &StorageImpl::reflection_probe_get_origin_offset;
		api.reflection_probe_get_origin_max_distance =
			&StorageImpl::reflection_probe_get_origin_max_distance;
		api.reflection_probe_renders_shadows = &StorageImpl::reflection_probe_renders_shadows;
		api.reflection_probe_get_mesh_lod_threshold =
			&StorageImpl::reflection_probe_get_mesh_lod_threshold;

		/* Reflection Atlas */
		api.reflection_atlas_create = &StorageImpl::reflection_atlas_create;
		api.reflection_atlas_free = &StorageImpl::reflection_atlas_free;
		api.reflection_atlas_set_size = &StorageImpl::reflection_atlas_set_size;
		api.reflection_atlas_get_size = &StorageImpl::reflection_atlas_get_size;

		/* Reflection Probe Instance */
		api.reflection_probe_instance_create = &StorageImpl::reflection_probe_instance_create;
		api.reflection_probe_instance_free = &StorageImpl::reflection_probe_instance_free;
		api.reflection_probe_instance_set_transform =
			&StorageImpl::reflection_probe_instance_set_transform;
		api.reflection_probe_has_atlas_index = &StorageImpl::reflection_probe_has_atlas_index;
		api.reflection_probe_release_atlas_index =
			&StorageImpl::reflection_probe_release_atlas_index;
		api.reflection_probe_instance_needs_redraw =
			&StorageImpl::reflection_probe_instance_needs_redraw;
		api.reflection_probe_instance_has_reflection =
			&StorageImpl::reflection_probe_instance_has_reflection;
		api.reflection_probe_instance_begin_render =
			&StorageImpl::reflection_probe_instance_begin_render;
		api.reflection_probe_instance_end_render =
			&StorageImpl::reflection_probe_instance_end_render;
		api.reflection_probe_atlas_get_render_buffers =
			&StorageImpl::reflection_probe_atlas_get_render_buffers;
		api.reflection_probe_instance_postprocess_step =
			&StorageImpl::reflection_probe_instance_postprocess_step;

		/* Lightmap */
		api.lightmap_allocate = &StorageImpl::lightmap_allocate;
		api.lightmap_initialize = &StorageImpl::lightmap_initialize;
		api.lightmap_free = &StorageImpl::lightmap_free;

		api.lightmap_set_textures = &StorageImpl::lightmap_set_textures;
		api.lightmap_set_probe_bounds = &StorageImpl::lightmap_set_probe_bounds;
		api.lightmap_set_probe_interior = &StorageImpl::lightmap_set_probe_interior;
		api.lightmap_set_probe_capture_data = &StorageImpl::lightmap_set_probe_capture_data;
		api.lightmap_set_baked_exposure_normalization =
			&StorageImpl::lightmap_set_baked_exposure_normalization;
		api.lightmap_get_probe_capture_points = &StorageImpl::lightmap_get_probe_capture_points;
		api.lightmap_get_probe_capture_sh = &StorageImpl::lightmap_get_probe_capture_sh;
		api.lightmap_get_probe_capture_tetrahedra =
			&StorageImpl::lightmap_get_probe_capture_tetrahedra;
		api.lightmap_get_probe_capture_bsp_tree = &StorageImpl::lightmap_get_probe_capture_bsp_tree;
		api.lightmap_get_aabb = &StorageImpl::lightmap_get_aabb;
		api.lightmap_tap_sh_light = &StorageImpl::lightmap_tap_sh_light;
		api.lightmap_is_interior = &StorageImpl::lightmap_is_interior;
		api.lightmap_set_probe_capture_update_speed =
			&StorageImpl::lightmap_set_probe_capture_update_speed;
		api.lightmap_get_probe_capture_update_speed =
			&StorageImpl::lightmap_get_probe_capture_update_speed;

		api.lightmap_set_shadowmask_textures = &StorageImpl::lightmap_set_shadowmask_textures;
		api.lightmap_get_shadowmask_mode = &StorageImpl::lightmap_get_shadowmask_mode;
		api.lightmap_set_shadowmask_mode = &StorageImpl::lightmap_set_shadowmask_mode;

		/* Lightmap Instance */
		api.lightmap_instance_create = &StorageImpl::lightmap_instance_create;
		api.lightmap_instance_free = &StorageImpl::lightmap_instance_free;
		api.lightmap_instance_set_transform = &StorageImpl::lightmap_instance_set_transform;

		/* Shadow Atlas */
		api.shadow_atlas_create = &StorageImpl::shadow_atlas_create;
		api.shadow_atlas_free = &StorageImpl::shadow_atlas_free;
		api.shadow_atlas_set_size = &StorageImpl::shadow_atlas_set_size;
		api.shadow_atlas_set_quadrant_subdivision =
			&StorageImpl::shadow_atlas_set_quadrant_subdivision;
		api.shadow_atlas_update_light = &StorageImpl::shadow_atlas_update_light;
		api.shadow_atlas_update = &StorageImpl::shadow_atlas_update;

		api.directional_shadow_atlas_set_size = &StorageImpl::directional_shadow_atlas_set_size;
		api.get_directional_light_shadow_size = &StorageImpl::get_directional_light_shadow_size;
		api.set_directional_shadow_count = &StorageImpl::set_directional_shadow_count;
	}

	/* Light API */
	_FORCE_INLINE_ static RID directional_light_allocate()
	{
		return api.directional_light_allocate();
	}

	_FORCE_INLINE_ static void directional_light_initialize(RID p_rid)
	{
		api.directional_light_initialize(p_rid);
	}

	_FORCE_INLINE_ static RID omni_light_allocate() { return api.omni_light_allocate(); }

	_FORCE_INLINE_ static void omni_light_initialize(RID p_rid)
	{
		api.omni_light_initialize(p_rid);
	}

	_FORCE_INLINE_ static RID spot_light_allocate() { return api.spot_light_allocate(); }

	_FORCE_INLINE_ static void spot_light_initialize(RID p_rid)
	{
		api.spot_light_initialize(p_rid);
	}

	_FORCE_INLINE_ static RID area_light_allocate() { return api.area_light_allocate(); }

	_FORCE_INLINE_ static void area_light_initialize(RID p_rid)
	{
		api.area_light_initialize(p_rid);
	}

	_FORCE_INLINE_ static void light_free(RID p_rid) { api.light_free(p_rid); }

	_FORCE_INLINE_ static void light_set_color(RID p_light, const Color& p_color)
	{
		api.light_set_color(p_light, p_color);
	}

	_FORCE_INLINE_ static void light_set_param(RID p_light, RSE::LightParam p_param, float p_value)
	{
		api.light_set_param(p_light, p_param, p_value);
	}

	_FORCE_INLINE_ static void light_set_shadow(RID p_light, bool p_enabled)
	{
		api.light_set_shadow(p_light, p_enabled);
	}

	_FORCE_INLINE_ static void light_set_projector(RID p_light, RID p_texture)
	{
		api.light_set_projector(p_light, p_texture);
	}

	_FORCE_INLINE_ static void light_set_negative(RID p_light, bool p_enable)
	{
		api.light_set_negative(p_light, p_enable);
	}

	_FORCE_INLINE_ static void light_set_cull_mask(RID p_light, uint32_t p_mask)
	{
		api.light_set_cull_mask(p_light, p_mask);
	}

	_FORCE_INLINE_ static void light_set_distance_fade(
		RID p_light, bool p_enabled, float p_begin, float p_shadow, float p_length)
	{
		api.light_set_distance_fade(p_light, p_enabled, p_begin, p_shadow, p_length);
	}

	_FORCE_INLINE_ static void light_set_reverse_cull_face_mode(RID p_light, bool p_enabled)
	{
		api.light_set_reverse_cull_face_mode(p_light, p_enabled);
	}

	_FORCE_INLINE_ static void light_set_shadow_caster_mask(RID p_light, uint32_t p_caster_mask)
	{
		api.light_set_shadow_caster_mask(p_light, p_caster_mask);
	}

	_FORCE_INLINE_ static uint32_t light_get_shadow_caster_mask(RID p_light)
	{
		return api.light_get_shadow_caster_mask(p_light);
	}

	_FORCE_INLINE_ static void light_set_bake_mode(RID p_light, RSE::LightBakeMode p_bake_mode)
	{
		api.light_set_bake_mode(p_light, p_bake_mode);
	}

	_FORCE_INLINE_ static void light_set_max_sdfgi_cascade(RID p_light, uint32_t p_cascade)
	{
		api.light_set_max_sdfgi_cascade(p_light, p_cascade);
	}

	_FORCE_INLINE_ static void light_omni_set_shadow_mode(
		RID p_light, RSE::LightOmniShadowMode p_mode)
	{
		api.light_omni_set_shadow_mode(p_light, p_mode);
	}

	_FORCE_INLINE_ static void light_directional_set_shadow_mode(
		RID p_light, RSE::LightDirectionalShadowMode p_mode)
	{
		api.light_directional_set_shadow_mode(p_light, p_mode);
	}

	_FORCE_INLINE_ static void light_directional_set_blend_splits(RID p_light, bool p_enable)
	{
		api.light_directional_set_blend_splits(p_light, p_enable);
	}

	_FORCE_INLINE_ static bool light_directional_get_blend_splits(RID p_light)
	{
		return api.light_directional_get_blend_splits(p_light);
	}

	_FORCE_INLINE_ static void light_directional_set_sky_mode(
		RID p_light, RSE::LightDirectionalSkyMode p_mode)
	{
		api.light_directional_set_sky_mode(p_light, p_mode);
	}

	_FORCE_INLINE_ static RSE::LightDirectionalSkyMode light_directional_get_sky_mode(RID p_light)
	{
		return api.light_directional_get_sky_mode(p_light);
	}

	_FORCE_INLINE_ static RSE::LightDirectionalShadowMode light_directional_get_shadow_mode(
		RID p_light)
	{
		return api.light_directional_get_shadow_mode(p_light);
	}

	_FORCE_INLINE_ static RSE::LightOmniShadowMode light_omni_get_shadow_mode(RID p_light)
	{
		return api.light_omni_get_shadow_mode(p_light);
	}

	_FORCE_INLINE_ static void light_area_set_size(RID p_light, const Vector2& p_size)
	{
		api.light_area_set_size(p_light, p_size);
	}

	_FORCE_INLINE_ static Vector2 light_area_get_size(RID p_light)
	{
		return api.light_area_get_size(p_light);
	}

	_FORCE_INLINE_ static void light_area_set_normalize_energy(RID p_light, bool p_enabled)
	{
		api.light_area_set_normalize_energy(p_light, p_enabled);
	}

	_FORCE_INLINE_ static bool light_area_get_normalize_energy(RID p_light)
	{
		return api.light_area_get_normalize_energy(p_light);
	}

	_FORCE_INLINE_ static void light_area_set_texture(RID p_light, RID p_texture)
	{
		api.light_area_set_texture(p_light, p_texture);
	}

	_FORCE_INLINE_ static RID light_area_get_texture(RID p_light)
	{
		return api.light_area_get_texture(p_light);
	}

	_FORCE_INLINE_ static bool light_has_shadow(RID p_light)
	{
		return api.light_has_shadow(p_light);
	}

	_FORCE_INLINE_ static bool light_has_projector(RID p_light)
	{
		return api.light_has_projector(p_light);
	}

	_FORCE_INLINE_ static RSE::LightType light_get_type(RID p_light)
	{
		return api.light_get_type(p_light);
	}

	_FORCE_INLINE_ static AABB light_get_aabb(RID p_light) { return api.light_get_aabb(p_light); }

	_FORCE_INLINE_ static float light_get_param(RID p_light, RSE::LightParam p_param)
	{
		return api.light_get_param(p_light, p_param);
	}

	_FORCE_INLINE_ static Color light_get_color(RID p_light)
	{
		return api.light_get_color(p_light);
	}

	_FORCE_INLINE_ static bool light_get_reverse_cull_face_mode(RID p_light)
	{
		return api.light_get_reverse_cull_face_mode(p_light);
	}

	_FORCE_INLINE_ static RSE::LightBakeMode light_get_bake_mode(RID p_light)
	{
		return api.light_get_bake_mode(p_light);
	}

	_FORCE_INLINE_ static uint32_t light_get_max_sdfgi_cascade(RID p_light)
	{
		return api.light_get_max_sdfgi_cascade(p_light);
	}

	_FORCE_INLINE_ static uint64_t light_get_version(RID p_light)
	{
		return api.light_get_version(p_light);
	}

	_FORCE_INLINE_ static uint32_t light_get_cull_mask(RID p_light)
	{
		return api.light_get_cull_mask(p_light);
	}

	/* Light Instance API */
	_FORCE_INLINE_ static RID light_instance_create(RID p_light)
	{
		return api.light_instance_create(p_light);
	}

	_FORCE_INLINE_ static void light_instance_free(RID p_light_instance)
	{
		api.light_instance_free(p_light_instance);
	}

	_FORCE_INLINE_ static void light_instance_set_transform(
		RID p_light_instance, const Transform3D& p_transform)
	{
		api.light_instance_set_transform(p_light_instance, p_transform);
	}

	_FORCE_INLINE_ static void light_instance_set_aabb(RID p_light_instance, const AABB& p_aabb)
	{
		api.light_instance_set_aabb(p_light_instance, p_aabb);
	}

	_FORCE_INLINE_ static void light_instance_set_shadow_transform(RID p_light_instance,
		const Projection& p_projection, const Transform3D& p_transform, float p_far, float p_split,
		int p_pass, float p_shadow_texel_size, float p_bias_scale = 1.0, float p_range_begin = 0,
		const Vector2& p_uv_scale = Vector2())
	{
		api.light_instance_set_shadow_transform(p_light_instance, p_projection, p_transform, p_far,
			p_split, p_pass, p_shadow_texel_size, p_bias_scale, p_range_begin, p_uv_scale);
	}

	_FORCE_INLINE_ static void light_instance_mark_visible(RID p_light_instance)
	{
		api.light_instance_mark_visible(p_light_instance);
	}

	_FORCE_INLINE_ static bool light_instances_can_render_shadow_cube() { return true; }

	_FORCE_INLINE_ static bool light_instance_is_shadow_visible_at_position(
		RID p_light, const Vector3& p_position)
	{
		return api.light_instance_is_shadow_visible_at_position(p_light, p_position);
	}

	/* Probe API */
	_FORCE_INLINE_ static RID reflection_probe_allocate()
	{
		return api.reflection_probe_allocate();
	}

	_FORCE_INLINE_ static void reflection_probe_initialize(RID p_rid)
	{
		api.reflection_probe_initialize(p_rid);
	}

	_FORCE_INLINE_ static void reflection_probe_free(RID p_rid)
	{
		api.reflection_probe_free(p_rid);
	}

	_FORCE_INLINE_ static void reflection_probe_set_update_mode(
		RID p_probe, RSE::ReflectionProbeUpdateMode p_mode)
	{
		api.reflection_probe_set_update_mode(p_probe, p_mode);
	}

	_FORCE_INLINE_ static void reflection_probe_set_resolution(RID p_probe, int p_resolution)
	{
		api.reflection_probe_set_resolution(p_probe, p_resolution);
	}

	_FORCE_INLINE_ static void reflection_probe_set_intensity(RID p_probe, float p_intensity)
	{
		api.reflection_probe_set_intensity(p_probe, p_intensity);
	}

	_FORCE_INLINE_ static void reflection_probe_set_blend_distance(
		RID p_probe, float p_blend_distance)
	{
		api.reflection_probe_set_blend_distance(p_probe, p_blend_distance);
	}

	_FORCE_INLINE_ static void reflection_probe_set_ambient_mode(
		RID p_probe, RSE::ReflectionProbeAmbientMode p_mode)
	{
		api.reflection_probe_set_ambient_mode(p_probe, p_mode);
	}

	_FORCE_INLINE_ static void reflection_probe_set_ambient_color(RID p_probe, const Color& p_color)
	{
		api.reflection_probe_set_ambient_color(p_probe, p_color);
	}

	_FORCE_INLINE_ static void reflection_probe_set_ambient_energy(RID p_probe, float p_energy)
	{
		api.reflection_probe_set_ambient_energy(p_probe, p_energy);
	}

	_FORCE_INLINE_ static void reflection_probe_set_max_distance(RID p_probe, float p_distance)
	{
		api.reflection_probe_set_max_distance(p_probe, p_distance);
	}

	_FORCE_INLINE_ static void reflection_probe_set_size(RID p_probe, const Vector3& p_size)
	{
		api.reflection_probe_set_size(p_probe, p_size);
	}

	_FORCE_INLINE_ static void reflection_probe_set_origin_offset(
		RID p_probe, const Vector3& p_offset)
	{
		api.reflection_probe_set_origin_offset(p_probe, p_offset);
	}

	_FORCE_INLINE_ static void reflection_probe_set_as_interior(RID p_probe, bool p_enable)
	{
		api.reflection_probe_set_as_interior(p_probe, p_enable);
	}

	_FORCE_INLINE_ static void reflection_probe_set_enable_box_projection(
		RID p_probe, bool p_enable)
	{
		api.reflection_probe_set_enable_box_projection(p_probe, p_enable);
	}

	_FORCE_INLINE_ static void reflection_probe_set_enable_shadows(RID p_probe, bool p_enable)
	{
		api.reflection_probe_set_enable_shadows(p_probe, p_enable);
	}

	_FORCE_INLINE_ static void reflection_probe_set_cull_mask(RID p_probe, uint32_t p_layers)
	{
		api.reflection_probe_set_cull_mask(p_probe, p_layers);
	}

	_FORCE_INLINE_ static void reflection_probe_set_reflection_mask(RID p_probe, uint32_t p_layers)
	{
		api.reflection_probe_set_reflection_mask(p_probe, p_layers);
	}

	_FORCE_INLINE_ static void reflection_probe_set_mesh_lod_threshold(RID p_probe, float p_ratio)
	{
		api.reflection_probe_set_mesh_lod_threshold(p_probe, p_ratio);
	}

	_FORCE_INLINE_ static AABB reflection_probe_get_aabb(RID p_probe)
	{
		return api.reflection_probe_get_aabb(p_probe);
	}

	_FORCE_INLINE_ static RSE::ReflectionProbeUpdateMode reflection_probe_get_update_mode(
		RID p_probe)
	{
		return api.reflection_probe_get_update_mode(p_probe);
	}

	_FORCE_INLINE_ static uint32_t reflection_probe_get_cull_mask(RID p_probe)
	{
		return api.reflection_probe_get_cull_mask(p_probe);
	}

	_FORCE_INLINE_ static uint32_t reflection_probe_get_reflection_mask(RID p_probe)
	{
		return api.reflection_probe_get_reflection_mask(p_probe);
	}

	_FORCE_INLINE_ static Vector3 reflection_probe_get_size(RID p_probe)
	{
		return api.reflection_probe_get_size(p_probe);
	}

	_FORCE_INLINE_ static Vector3 reflection_probe_get_origin_offset(RID p_probe)
	{
		return api.reflection_probe_get_origin_offset(p_probe);
	}

	_FORCE_INLINE_ static float reflection_probe_get_origin_max_distance(RID p_probe)
	{
		return api.reflection_probe_get_origin_max_distance(p_probe);
	}

	_FORCE_INLINE_ static bool reflection_probe_renders_shadows(RID p_probe)
	{
		return api.reflection_probe_renders_shadows(p_probe);
	}

	_FORCE_INLINE_ static float reflection_probe_get_mesh_lod_threshold(RID p_probe)
	{
		return api.reflection_probe_get_mesh_lod_threshold(p_probe);
	}

	/* Reflection Atlas */
	_FORCE_INLINE_ static RID reflection_atlas_create() { return api.reflection_atlas_create(); }

	_FORCE_INLINE_ static void reflection_atlas_free(RID p_ref_atlas)
	{
		api.reflection_atlas_free(p_ref_atlas);
	}

	_FORCE_INLINE_ static void reflection_atlas_set_size(
		RID p_ref_atlas, int p_reflection_size, int p_reflection_count)
	{
		api.reflection_atlas_set_size(p_ref_atlas, p_reflection_size, p_reflection_count);
	}

	_FORCE_INLINE_ static int reflection_atlas_get_size(RID p_ref_atlas)
	{
		return api.reflection_atlas_get_size(p_ref_atlas);
	}

	/* Reflection Probe Instance */
	_FORCE_INLINE_ static RID reflection_probe_instance_create(RID p_probe)
	{
		return api.reflection_probe_instance_create(p_probe);
	}

	_FORCE_INLINE_ static void reflection_probe_instance_free(RID p_instance)
	{
		api.reflection_probe_instance_free(p_instance);
	}

	_FORCE_INLINE_ static void reflection_probe_instance_set_transform(
		RID p_instance, const Transform3D& p_transform)
	{
		api.reflection_probe_instance_set_transform(p_instance, p_transform);
	}

	_FORCE_INLINE_ static bool reflection_probe_has_atlas_index(RID p_instance)
	{
		return api.reflection_probe_has_atlas_index(p_instance);
	}

	_FORCE_INLINE_ static void reflection_probe_release_atlas_index(RID p_instance)
	{
		api.reflection_probe_release_atlas_index(p_instance);
	}

	_FORCE_INLINE_ static bool reflection_probe_instance_needs_redraw(RID p_instance)
	{
		return api.reflection_probe_instance_needs_redraw(p_instance);
	}

	_FORCE_INLINE_ static bool reflection_probe_instance_has_reflection(RID p_instance)
	{
		return api.reflection_probe_instance_has_reflection(p_instance);
	}

	_FORCE_INLINE_ static bool reflection_probe_instance_begin_render(
		RID p_instance, RID p_reflection_atlas)
	{
		return api.reflection_probe_instance_begin_render(p_instance, p_reflection_atlas);
	}

	_FORCE_INLINE_ static bool reflection_probe_instance_end_render(
		RID p_instance, RID p_reflection_atlas)
	{
		return api.reflection_probe_instance_end_render(p_instance, p_reflection_atlas);
	}

	_FORCE_INLINE_ static Ref<RenderSceneBuffers> reflection_probe_atlas_get_render_buffers(
		RID p_reflection_atlas)
	{
		return api.reflection_probe_atlas_get_render_buffers(p_reflection_atlas);
	}

	_FORCE_INLINE_ static bool reflection_probe_instance_postprocess_step(RID p_instance)
	{
		return api.reflection_probe_instance_postprocess_step(p_instance);
	}

	/* Lightmap */
	_FORCE_INLINE_ static RID lightmap_allocate() { return api.lightmap_allocate(); }

	_FORCE_INLINE_ static void lightmap_initialize(RID p_rid) { api.lightmap_initialize(p_rid); }

	_FORCE_INLINE_ static void lightmap_free(RID p_rid) { api.lightmap_free(p_rid); }

	_FORCE_INLINE_ static void lightmap_set_textures(
		RID p_lightmap, RID p_light, bool p_uses_spherical_harmonics)
	{
		api.lightmap_set_textures(p_lightmap, p_light, p_uses_spherical_harmonics);
	}

	_FORCE_INLINE_ static void lightmap_set_probe_bounds(RID p_lightmap, const AABB& p_bounds)
	{
		api.lightmap_set_probe_bounds(p_lightmap, p_bounds);
	}

	_FORCE_INLINE_ static void lightmap_set_probe_interior(RID p_lightmap, bool p_interior)
	{
		api.lightmap_set_probe_interior(p_lightmap, p_interior);
	}

	_FORCE_INLINE_ static void lightmap_set_probe_capture_data(RID p_lightmap,
		const PackedVector3Array& p_points, const PackedColorArray& p_point_sh,
		const PackedInt32Array& p_tetrahedra, const PackedInt32Array& p_bsp_tree)
	{
		api.lightmap_set_probe_capture_data(
			p_lightmap, p_points, p_point_sh, p_tetrahedra, p_bsp_tree);
	}

	_FORCE_INLINE_ static void lightmap_set_baked_exposure_normalization(
		RID p_lightmap, float p_exposure)
	{
		api.lightmap_set_baked_exposure_normalization(p_lightmap, p_exposure);
	}

	_FORCE_INLINE_ static PackedVector3Array lightmap_get_probe_capture_points(RID p_lightmap)
	{
		return api.lightmap_get_probe_capture_points(p_lightmap);
	}

	_FORCE_INLINE_ static PackedColorArray lightmap_get_probe_capture_sh(RID p_lightmap)
	{
		return api.lightmap_get_probe_capture_sh(p_lightmap);
	}

	_FORCE_INLINE_ static PackedInt32Array lightmap_get_probe_capture_tetrahedra(RID p_lightmap)
	{
		return api.lightmap_get_probe_capture_tetrahedra(p_lightmap);
	}

	_FORCE_INLINE_ static PackedInt32Array lightmap_get_probe_capture_bsp_tree(RID p_lightmap)
	{
		return api.lightmap_get_probe_capture_bsp_tree(p_lightmap);
	}

	_FORCE_INLINE_ static AABB lightmap_get_aabb(RID p_lightmap)
	{
		return api.lightmap_get_aabb(p_lightmap);
	}

	_FORCE_INLINE_ static void lightmap_tap_sh_light(
		RID p_lightmap, const Vector3& p_point, Color* r_sh)
	{
		api.lightmap_tap_sh_light(p_lightmap, p_point, r_sh);
	}

	_FORCE_INLINE_ static bool lightmap_is_interior(RID p_lightmap)
	{
		return api.lightmap_is_interior(p_lightmap);
	}

	_FORCE_INLINE_ static void lightmap_set_probe_capture_update_speed(float p_speed)
	{
		api.lightmap_set_probe_capture_update_speed(p_speed);
	}

	_FORCE_INLINE_ static float lightmap_get_probe_capture_update_speed()
	{
		return api.lightmap_get_probe_capture_update_speed();
	}

	_FORCE_INLINE_ static void lightmap_set_shadowmask_textures(RID p_lightmap, RID p_shadow)
	{
		api.lightmap_set_shadowmask_textures(p_lightmap, p_shadow);
	}

	_FORCE_INLINE_ static RSE::ShadowmaskMode lightmap_get_shadowmask_mode(RID p_lightmap)
	{
		return api.lightmap_get_shadowmask_mode(p_lightmap);
	}

	_FORCE_INLINE_ static void lightmap_set_shadowmask_mode(
		RID p_lightmap, RSE::ShadowmaskMode p_mode)
	{
		api.lightmap_set_shadowmask_mode(p_lightmap, p_mode);
	}

	/* Lightmap Instance */

	_FORCE_INLINE_ static RID lightmap_instance_create(RID p_lightmap)
	{
		return api.lightmap_instance_create(p_lightmap);
	}

	_FORCE_INLINE_ static void lightmap_instance_free(RID p_lightmap)
	{
		api.lightmap_instance_free(p_lightmap);
	}

	_FORCE_INLINE_ static void lightmap_instance_set_transform(
		RID p_lightmap, const Transform3D& p_transform)
	{
		api.lightmap_instance_set_transform(p_lightmap, p_transform);
	}

	/* Shadow Atlas */
	_FORCE_INLINE_ static RID shadow_atlas_create() { return api.shadow_atlas_create(); }

	_FORCE_INLINE_ static void shadow_atlas_free(RID p_atlas) { api.shadow_atlas_free(p_atlas); }

	_FORCE_INLINE_ static void shadow_atlas_set_size(
		RID p_atlas, int p_size, bool p_use_16_bits = true)
	{
		api.shadow_atlas_set_size(p_atlas, p_size, p_use_16_bits);
	}

	_FORCE_INLINE_ static void shadow_atlas_set_quadrant_subdivision(
		RID p_atlas, int p_quadrant, int p_subdivision)
	{
		api.shadow_atlas_set_quadrant_subdivision(p_atlas, p_quadrant, p_subdivision);
	}

	_FORCE_INLINE_ static bool shadow_atlas_update_light(
		RID p_atlas, RID p_light_instance, float p_coverage, uint64_t p_light_version)
	{
		return api.shadow_atlas_update_light(
			p_atlas, p_light_instance, p_coverage, p_light_version);
	}

	_FORCE_INLINE_ static void shadow_atlas_update(RID p_atlas)
	{
		api.shadow_atlas_update(p_atlas);
	}

	_FORCE_INLINE_ static void directional_shadow_atlas_set_size(int p_size, bool p_16_bits = true)
	{
		api.directional_shadow_atlas_set_size(p_size, p_16_bits);
	}

	_FORCE_INLINE_ static int get_directional_light_shadow_size(RID p_light_instance)
	{
		return api.get_directional_light_shadow_size(p_light_instance);
	}

	_FORCE_INLINE_ static void set_directional_shadow_count(int p_count)
	{
		api.set_directional_shadow_count(p_count);
	}
};
// --- Inlined/Folded from servers/rendering/storage/material_storage.h ---
#include "servers/rendering/rendering_server_enums.h"
#include "servers/rendering/rendering_server_types.h"
#include "servers/rendering/storage/utilities.h"

struct InstanceShaderParam
{
	int index;
};

namespace MaterialStorage
{

struct API
{
	/* GLOBAL SHADER UNIFORM API */
	void (*global_shader_parameter_remove)(const String& p_name) = nullptr;
	Vector<String> (*global_shader_parameter_get_list)() = nullptr;
	RSE::GlobalShaderParameterType (*global_shader_parameter_get_type)(
		const String& p_name) = nullptr;
	void (*global_shader_parameters_load_settings)(bool p_load_textures) = nullptr;
	void (*global_shader_parameters_clear)() = nullptr;
	int32_t (*global_shader_parameters_instance_allocate)(RID p_instance) = nullptr;
	void (*global_shader_parameters_instance_free)(RID p_instance) = nullptr;
	void (*finalize)() = nullptr;

	/* SHADER API */
	RID (*shader_allocate)() = nullptr;
	void (*shader_initialize)(RID p_rid, bool p_embedded) = nullptr;
	void (*shader_free)(RID p_rid) = nullptr;
	void (*shader_set_code)(RID p_shader, const String& p_code) = nullptr;
	void (*shader_set_path_hint)(RID p_shader, const String& p_path) = nullptr;
	String (*shader_get_code)(RID p_shader) = nullptr;
	void (*shader_set_default_texture_parameter)(
		RID p_shader, const String& p_name, RID p_texture, int p_index) = nullptr;
	RID(*shader_get_default_texture_parameter)
	(RID p_shader, const String& p_name, int p_index) = nullptr;
	RenderingServerTypes::ShaderNativeSourceCode (*shader_get_native_source_code)(
		RID p_shader) = nullptr;
	void (*shader_embedded_set_lock)() = nullptr;
	const HashSet<RID>& (*shader_embedded_set_get)() = nullptr;
	void (*shader_embedded_set_unlock)() = nullptr;

	/* MATERIAL API */
	RID (*material_allocate)() = nullptr;
	void (*material_initialize)(RID p_rid) = nullptr;
	void (*material_free)(RID p_rid) = nullptr;
	void (*material_set_render_priority)(RID p_material, int priority) = nullptr;
	void (*material_set_shader)(RID p_shader_material, RID p_shader) = nullptr;
	void (*material_set_next_pass)(RID p_material, RID p_next_material) = nullptr;
	bool (*material_is_animated)(RID p_material) = nullptr;
	bool (*material_casts_shadows)(RID p_material) = nullptr;
	RSE::CullMode (*material_get_cull_mode)(RID p_material) = nullptr;
	void (*material_get_instance_shader_parameters)(
		RID p_material, List<InstanceShaderParam>* r_parameters) = nullptr;
	void (*material_update_dependency)(RID p_material, DependencyTracker* p_instance) = nullptr;
};

}; // namespace MaterialStorage

class RendererMaterialStorage final
{
private:
    static inline MaterialStorage::API api;

public:
    RendererMaterialStorage() = delete;
    RendererMaterialStorage(const RendererMaterialStorage&) = delete;
    RendererMaterialStorage& operator=(const RendererMaterialStorage&) = delete;
    ~RendererMaterialStorage() = delete;

    template <typename StorageImpl>
    static void bind()
    {
        /* GLOBAL SHADER UNIFORM API */
        api.global_shader_parameter_remove = &StorageImpl::global_shader_parameter_remove;
        api.global_shader_parameter_get_list = &StorageImpl::global_shader_parameter_get_list;
        api.global_shader_parameter_get_type = &StorageImpl::global_shader_parameter_get_type;
        api.global_shader_parameters_load_settings = &StorageImpl::global_shader_parameters_load_settings;
        api.global_shader_parameters_clear = &StorageImpl::global_shader_parameters_clear;
        api.global_shader_parameters_instance_allocate = &StorageImpl::global_shader_parameters_instance_allocate;
        api.global_shader_parameters_instance_free = &StorageImpl::global_shader_parameters_instance_free;
        api.finalize = &StorageImpl::finalize;

        /* SHADER API */
        api.shader_allocate = &StorageImpl::shader_allocate;
        api.shader_initialize = &StorageImpl::shader_initialize;
        api.shader_free = &StorageImpl::shader_free;
        api.shader_set_code = &StorageImpl::shader_set_code;
        api.shader_set_path_hint = &StorageImpl::shader_set_path_hint;
        api.shader_get_code = &StorageImpl::shader_get_code;
        api.shader_set_default_texture_parameter = &StorageImpl::shader_set_default_texture_parameter;
        api.shader_get_default_texture_parameter = &StorageImpl::shader_get_default_texture_parameter;
        api.shader_get_native_source_code = &StorageImpl::shader_get_native_source_code;
        api.shader_embedded_set_lock = &StorageImpl::shader_embedded_set_lock;
        api.shader_embedded_set_get = &StorageImpl::shader_embedded_set_get;
        api.shader_embedded_set_unlock = &StorageImpl::shader_embedded_set_unlock;

        /* MATERIAL API */
        api.material_allocate = &StorageImpl::material_allocate;
        api.material_initialize = &StorageImpl::material_initialize;
        api.material_free = &StorageImpl::material_free;
        api.material_set_render_priority = &StorageImpl::material_set_render_priority;
        api.material_set_shader = &StorageImpl::material_set_shader;
        api.material_set_next_pass = &StorageImpl::material_set_next_pass;
        api.material_is_animated = &StorageImpl::material_is_animated;
        api.material_casts_shadows = &StorageImpl::material_casts_shadows;
        api.material_get_cull_mode = &StorageImpl::material_get_cull_mode;
        api.material_get_instance_shader_parameters = &StorageImpl::material_get_instance_shader_parameters;
        api.material_update_dependency = &StorageImpl::material_update_dependency;
    }

    /* GLOBAL SHADER UNIFORM API */
    _FORCE_INLINE_ static void global_shader_parameter_remove(const String& p_name)
    {
        api.global_shader_parameter_remove(p_name);
    }

    _FORCE_INLINE_ static Vector<String> global_shader_parameter_get_list()
    {
        return api.global_shader_parameter_get_list();
    }

    _FORCE_INLINE_ static RSE::GlobalShaderParameterType global_shader_parameter_get_type(
        const String& p_name)
    {
        return api.global_shader_parameter_get_type(p_name);
    }

    _FORCE_INLINE_ static void global_shader_parameters_load_settings(bool p_load_textures = true)
    {
        api.global_shader_parameters_load_settings(p_load_textures);
    }

    _FORCE_INLINE_ static void global_shader_parameters_clear()
    {
        api.global_shader_parameters_clear();
    }

    _FORCE_INLINE_ static int32_t global_shader_parameters_instance_allocate(RID p_instance)
    {
        return api.global_shader_parameters_instance_allocate(p_instance);
    }

    _FORCE_INLINE_ static void global_shader_parameters_instance_free(RID p_instance)
    {
        api.global_shader_parameters_instance_free(p_instance);
    }

    /* SHADER API */
    _FORCE_INLINE_ static RID shader_allocate()
    {
        return api.shader_allocate();
    }

    _FORCE_INLINE_ static void shader_initialize(RID p_rid, bool p_embedded = true)
    {
        api.shader_initialize(p_rid, p_embedded);
    }

    _FORCE_INLINE_ static void shader_free(RID p_rid)
    {
        api.shader_free(p_rid);
    }

    _FORCE_INLINE_ static void shader_set_code(RID p_shader, const String& p_code)
    {
        api.shader_set_code(p_shader, p_code);
    }

    _FORCE_INLINE_ static void shader_set_path_hint(RID p_shader, const String& p_path)
    {
        api.shader_set_path_hint(p_shader, p_path);
    }

    _FORCE_INLINE_ static String shader_get_code(RID p_shader)
    {
        return api.shader_get_code(p_shader);
    }

    _FORCE_INLINE_ static void shader_set_default_texture_parameter(
        RID p_shader, const String& p_name, RID p_texture, int p_index)
    {
        api.shader_set_default_texture_parameter(p_shader, p_name, p_texture, p_index);
    }

    _FORCE_INLINE_ static RID shader_get_default_texture_parameter(
        RID p_shader, const String& p_name, int p_index)
    {
        return api.shader_get_default_texture_parameter(p_shader, p_name, p_index);
    }

    _FORCE_INLINE_ static RenderingServerTypes::ShaderNativeSourceCode shader_get_native_source_code(
        RID p_shader)
    {
        return api.shader_get_native_source_code(p_shader);
    }

    _FORCE_INLINE_ static void shader_embedded_set_lock()
    {
        api.shader_embedded_set_lock();
    }

    _FORCE_INLINE_ static const HashSet<RID>& shader_embedded_set_get()
    {
        return api.shader_embedded_set_get();
    }

    _FORCE_INLINE_ static void shader_embedded_set_unlock()
    {
        api.shader_embedded_set_unlock();
    }

    /* MATERIAL API */
    _FORCE_INLINE_ static RID material_allocate()
    {
        return api.material_allocate();
    }

    _FORCE_INLINE_ static void material_initialize(RID p_rid)
    {
        api.material_initialize(p_rid);
    }

    _FORCE_INLINE_ static void material_free(RID p_rid)
    {
        api.material_free(p_rid);
    }

    _FORCE_INLINE_ static void material_set_render_priority(RID p_material, int priority)
    {
        api.material_set_render_priority(p_material, priority);
    }

    _FORCE_INLINE_ static void material_set_shader(RID p_shader_material, RID p_shader)
    {
        api.material_set_shader(p_shader_material, p_shader);
    }

    _FORCE_INLINE_ static void material_set_next_pass(RID p_material, RID p_next_material)
    {
        api.material_set_next_pass(p_material, p_next_material);
    }

    _FORCE_INLINE_ static bool material_is_animated(RID p_material)
    {
        return api.material_is_animated(p_material);
    }

    _FORCE_INLINE_ static bool material_casts_shadows(RID p_material)
    {
        return api.material_casts_shadows(p_material);
    }

    _FORCE_INLINE_ static RSE::CullMode material_get_cull_mode(RID p_material)
    {
        return api.material_get_cull_mode(p_material);
    }

    _FORCE_INLINE_ static void material_get_instance_shader_parameters(
        RID p_material, List<InstanceShaderParam>* r_parameters)
    {
        api.material_get_instance_shader_parameters(p_material, r_parameters);
    }

    _FORCE_INLINE_ static void material_update_dependency(RID p_material, DependencyTracker* p_instance)
    {
        api.material_update_dependency(p_material, p_instance);
    }

    _FORCE_INLINE_ static void finalize()
    {
        if (api.finalize) {
            api.finalize();
        }
    }
};
// --- Inlined/Folded from servers/rendering/storage/mesh_storage.h ---
#include "core/math/color.h"
#include "core/math/transform_2d.h"
#include "core/templates/local_vector.h"
#include "servers/rendering/rendering_server_enums.h"
#include "servers/rendering/rendering_server_types.h"
#include "servers/rendering/storage/utilities.h"

struct InterpolationData
{
    void notify_free_multimesh(RID p_rid);
    LocalVector<RID> multimesh_interpolate_update_list;
    LocalVector<RID> multimesh_transform_update_lists[2];
    LocalVector<RID>* multimesh_transform_update_list_curr =
        &multimesh_transform_update_lists[0];
    LocalVector<RID>* multimesh_transform_update_list_prev =
        &multimesh_transform_update_lists[1];
};

class RendererMeshStorage final
{
public:
    RendererMeshStorage() = delete;
    RendererMeshStorage(const RendererMeshStorage&) = delete;
    RendererMeshStorage& operator=(const RendererMeshStorage&) = delete;
    ~RendererMeshStorage() = delete;

    /* MESH API */

    inline static RID (*mesh_allocate)() = nullptr;
    inline static void (*mesh_initialize)(RID p_rid) = nullptr;
    inline static void (*mesh_free)(RID p_rid) = nullptr;

    inline static void (*mesh_set_blend_shape_count)(RID p_mesh, int p_blend_shape_count) = nullptr;
    inline static bool (*mesh_needs_instance)(RID p_mesh, bool p_has_skeleton) = nullptr;

    inline static void (*mesh_add_surface)(
        RID p_mesh, const RenderingServerTypes::SurfaceData& p_surface) = nullptr;

    inline static int (*mesh_get_blend_shape_count)(RID p_mesh) = nullptr;

    inline static void (*mesh_set_blend_shape_mode)(RID p_mesh, RSE::BlendShapeMode p_mode) = nullptr;
    inline static RSE::BlendShapeMode (*mesh_get_blend_shape_mode)(RID p_mesh) = nullptr;

    inline static void (*mesh_surface_update_vertex_region)(
        RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data) = nullptr;
    inline static void (*mesh_surface_update_attribute_region)(
        RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data) = nullptr;
    inline static void (*mesh_surface_update_skin_region)(
        RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data) = nullptr;
    inline static void (*mesh_surface_update_index_region)(
        RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data) = nullptr;

    inline static void (*mesh_surface_set_material)(RID p_mesh, int p_surface, RID p_material) = nullptr;
    inline static RID (*mesh_surface_get_material)(RID p_mesh, int p_surface) = nullptr;

    inline static RenderingServerTypes::SurfaceData (*mesh_get_surface)(RID p_mesh, int p_surface) = nullptr;

    inline static RID (*mesh_surface_get_vertex_buffer_rd_rid)(RID p_mesh, int p_surface) = nullptr;
    inline static RID (*mesh_surface_get_attribute_buffer_rd_rid)(RID p_mesh, int p_surface) = nullptr;
    inline static RID (*mesh_surface_get_skin_buffer_rd_rid)(RID p_mesh, int p_surface) = nullptr;
    inline static RID (*mesh_surface_get_index_buffer_rd_rid)(RID p_mesh, int p_surface) = nullptr;

    inline static int (*mesh_get_surface_count)(RID p_mesh) = nullptr;

    inline static void (*mesh_set_custom_aabb)(RID p_mesh, const AABB& p_aabb) = nullptr;
    inline static AABB (*mesh_get_custom_aabb)(RID p_mesh) = nullptr;
    inline static AABB (*mesh_get_aabb)(RID p_mesh, RID p_skeleton) = nullptr;

    inline static void (*mesh_set_path)(RID p_mesh, const String& p_path) = nullptr;
    inline static String (*mesh_get_path)(RID p_mesh) = nullptr;
    inline static void (*mesh_set_shadow_mesh)(RID p_mesh, RID p_shadow_mesh) = nullptr;

    inline static void (*mesh_clear)(RID p_mesh) = nullptr;
    inline static void (*mesh_surface_remove)(RID p_mesh, int p_surface) = nullptr;
    inline static void (*mesh_debug_usage)(List<RenderingServerTypes::MeshInfo>* r_info) = nullptr;

    /* MESH INSTANCE API */

    inline static RID (*mesh_instance_create)(RID p_base) = nullptr;
    inline static void (*mesh_instance_free)(RID p_mesh_instance) = nullptr;
    inline static void (*mesh_instance_set_skeleton)(RID p_mesh_instance, RID p_skeleton) = nullptr;
    inline static void (*mesh_instance_set_blend_shape_weight)(
        RID p_mesh_instance, int p_shape, float p_weight) = nullptr;
    inline static void (*mesh_instance_check_for_update)(RID p_mesh_instance) = nullptr;
    inline static void (*mesh_instance_set_canvas_item_transform)(
        RID p_mesh_instance, const Transform2D& p_transform) = nullptr;
    inline static void (*update_mesh_instances)() = nullptr;

    /* MULTIMESH STRUCTS & BACKEND DISPATCH HOOKS */

    struct MultiMeshInterpolator
    {
        RSE::MultimeshTransformFormat _transform_format = RSE::MULTIMESH_TRANSFORM_3D;
        bool _use_colors = false;
        bool _use_custom_data = false;

        int _stride = 0;
        int _vf_size_xform = 0;
        int _vf_size_color = 0;
        int _vf_size_data = 0;
        int _num_instances = 0;

        int quality = 0;
        bool interpolated = false;
        bool on_interpolate_update_list = false;
        bool on_transform_update_list = false;

        Vector<float> _data_prev;
        Vector<float> _data_curr;
        Vector<float> _data_interpolated;
    };

    inline static RID (*_multimesh_allocate)() = nullptr;
    inline static void (*_multimesh_initialize)(RID p_rid) = nullptr;
    inline static void (*_multimesh_free)(RID p_rid) = nullptr;

    inline static void (*_multimesh_allocate_data)(RID p_multimesh, int p_instances,
        RSE::MultimeshTransformFormat p_transform_format, bool p_use_colors,
        bool p_use_custom_data, bool p_use_indirect) = nullptr;

    inline static int (*_multimesh_get_instance_count)(RID p_multimesh) = nullptr;

    inline static void (*_multimesh_set_mesh)(RID p_multimesh, RID p_mesh) = nullptr;
    inline static void (*_multimesh_instance_set_transform)(
        RID p_multimesh, int p_index, const Transform3D& p_transform) = nullptr;
    inline static void (*_multimesh_instance_set_transform_2d)(
        RID p_multimesh, int p_index, const Transform2D& p_transform) = nullptr;
    inline static void (*_multimesh_instance_set_color)(
        RID p_multimesh, int p_index, const Color& p_color) = nullptr;
    inline static void (*_multimesh_instance_set_custom_data)(
        RID p_multimesh, int p_index, const Color& p_color) = nullptr;

    inline static void (*_multimesh_set_custom_aabb)(RID p_multimesh, const AABB& p_aabb) = nullptr;
    inline static AABB (*_multimesh_get_custom_aabb)(RID p_multimesh) = nullptr;

    inline static RID (*_multimesh_get_mesh)(RID p_multimesh) = nullptr;

    inline static Transform3D (*_multimesh_instance_get_transform)(RID p_multimesh, int p_index) = nullptr;
    inline static Transform2D (*_multimesh_instance_get_transform_2d)(RID p_multimesh, int p_index) = nullptr;
    inline static Color (*_multimesh_instance_get_color)(RID p_multimesh, int p_index) = nullptr;
    inline static Color (*_multimesh_instance_get_custom_data)(RID p_multimesh, int p_index) = nullptr;

    inline static void (*_multimesh_set_buffer)(RID p_multimesh, const Vector<float>& p_buffer) = nullptr;
    inline static RID (*_multimesh_get_command_buffer_rd_rid)(RID p_multimesh) = nullptr;
    inline static RID (*_multimesh_get_buffer_rd_rid)(RID p_multimesh) = nullptr;
    inline static Vector<float> (*_multimesh_get_buffer)(RID p_multimesh) = nullptr;

    inline static void (*_multimesh_set_visible_instances)(RID p_multimesh, int p_visible) = nullptr;
    inline static int (*_multimesh_get_visible_instances)(RID p_multimesh) = nullptr;

    inline static AABB (*_multimesh_get_aabb)(RID p_multimesh) = nullptr;

    inline static MultiMeshInterpolator* (*_multimesh_get_interpolator)(RID p_multimesh) = nullptr;

    /* MULTIMESH PUBLIC FRONTEND API */

    static _FORCE_INLINE_ RID multimesh_allocate() { return _multimesh_allocate(); }
    static _FORCE_INLINE_ void multimesh_initialize(RID p_rid) { _multimesh_initialize(p_rid); }
    static _FORCE_INLINE_ void multimesh_free(RID p_rid) { _multimesh_free(p_rid); }

    static void multimesh_allocate_data(RID p_multimesh, int p_instances,
        RSE::MultimeshTransformFormat p_transform_format, bool p_use_colors = false,
        bool p_use_custom_data = false, bool p_use_indirect = false);

    static int multimesh_get_instance_count(RID p_multimesh);

    static void multimesh_set_mesh(RID p_multimesh, RID p_mesh);
    static void multimesh_instance_set_transform(
        RID p_multimesh, int p_index, const Transform3D& p_transform);
    static void multimesh_instance_set_transform_2d(
        RID p_multimesh, int p_index, const Transform2D& p_transform);
    static void multimesh_instance_set_color(RID p_multimesh, int p_index, const Color& p_color);
    static void multimesh_instance_set_custom_data(
        RID p_multimesh, int p_index, const Color& p_color);

    static void multimesh_set_custom_aabb(RID p_multimesh, const AABB& p_aabb);
    static AABB multimesh_get_custom_aabb(RID p_multimesh);

    static RID multimesh_get_mesh(RID p_multimesh);

    static Transform3D multimesh_instance_get_transform(RID p_multimesh, int p_index);
    static Transform2D multimesh_instance_get_transform_2d(RID p_multimesh, int p_index);
    static Color multimesh_instance_get_color(RID p_multimesh, int p_index);
    static Color multimesh_instance_get_custom_data(RID p_multimesh, int p_index);

    static void multimesh_set_buffer(RID p_multimesh, const Vector<float>& p_buffer);
    static RID multimesh_get_command_buffer_rd_rid(RID p_multimesh);
    static RID multimesh_get_buffer_rd_rid(RID p_multimesh);
    static Vector<float> multimesh_get_buffer(RID p_multimesh);

    static void multimesh_set_buffer_interpolated(
        RID p_multimesh, const Vector<float>& p_buffer, const Vector<float>& p_buffer_prev);
    static void multimesh_set_physics_interpolated(RID p_multimesh, bool p_interpolated);
    static void multimesh_set_physics_interpolation_quality(
        RID p_multimesh, RSE::MultimeshPhysicsInterpolationQuality p_quality);
    static void multimesh_instance_reset_physics_interpolation(RID p_multimesh, int p_index);
    static void multimesh_instances_reset_physics_interpolation(RID p_multimesh);

    static void multimesh_set_visible_instances(RID p_multimesh, int p_visible);
    static int multimesh_get_visible_instances(RID p_multimesh);

    static AABB multimesh_get_aabb(RID p_multimesh);

private:
    static void _multimesh_add_to_interpolation_lists(RID p_multimesh, MultiMeshInterpolator& r_mmi);

public:
    /* SKELETON API */

    inline static RID (*skeleton_allocate)() = nullptr;
    inline static void (*skeleton_initialize)(RID p_rid) = nullptr;
    inline static void (*skeleton_free)(RID p_rid) = nullptr;

    inline static void (*skeleton_allocate_data)(
        RID p_skeleton, int p_bones, bool p_2d_skeleton) = nullptr;
    inline static void (*skeleton_set_base_transform_2d)(
        RID p_skeleton, const Transform2D& p_base_transform) = nullptr;
    inline static int (*skeleton_get_bone_count)(RID p_skeleton) = nullptr;
    inline static void (*skeleton_bone_set_transform)(
        RID p_skeleton, int p_bone, const Transform3D& p_transform) = nullptr;
    inline static Transform3D (*skeleton_bone_get_transform)(RID p_skeleton, int p_bone) = nullptr;
    inline static void (*skeleton_bone_set_transform_2d)(
        RID p_skeleton, int p_bone, const Transform2D& p_transform) = nullptr;
    inline static Transform2D (*skeleton_bone_get_transform_2d)(RID p_skeleton, int p_bone) = nullptr;

    inline static void (*skeleton_update_dependency)(RID p_base, DependencyTracker* p_instance) = nullptr;

    /* INTERPOLATION */

    inline static InterpolationData _interpolation_data;

    static void update_interpolation_tick(bool p_process = true);
    static void update_interpolation_frame(bool p_process = true);
};
// --- Inlined/Folded from servers/rendering/storage/particles_storage.h ---
#include "core/math/aabb.h"
#include "core/math/color.h"
#include "core/math/math_defs.h"
#include "core/math/transform_3d.h"
#include "core/math/vector3.h"
#include "core/templates/rid.h"
#include "core/templates/vector.h"
#include "servers/rendering/rendering_server_enums.h"

class RendererParticlesStorage
{
public:
	/* PARTICLES FUNCTION POINTERS */
	inline static RID (*particles_allocate)() = nullptr;
	inline static void (*particles_initialize)(RID p_rid) = nullptr;
	inline static void (*particles_free)(RID p_rid) = nullptr;

	inline static void (*particles_set_mode)(RID p_particles, RSE::ParticlesMode p_mode) = nullptr;

	inline static void (*particles_set_emitting)(RID p_particles, bool p_emitting) = nullptr;
	inline static bool (*particles_get_emitting)(RID p_particles) = nullptr;

	inline static void (*particles_set_amount)(RID p_particles, int p_amount) = nullptr;
	inline static void (*particles_set_amount_ratio)(RID p_particles, float p_amount_ratio) = nullptr;
	inline static void (*particles_set_lifetime)(RID p_particles, double p_lifetime) = nullptr;
	inline static void (*particles_set_one_shot)(RID p_particles, bool p_one_shot) = nullptr;
	inline static void (*particles_set_pre_process_time)(RID p_particles, double p_time) = nullptr;
	inline static void (*particles_request_process_time)(
		RID p_particles, real_t p_request_process_time, real_t p_request_process_time_residual) = nullptr;
	inline static void (*particles_set_explosiveness_ratio)(RID p_particles, real_t p_ratio) = nullptr;
	inline static void (*particles_set_randomness_ratio)(RID p_particles, real_t p_ratio) = nullptr;
	inline static void (*particles_set_custom_aabb)(RID p_particles, const AABB &p_aabb) = nullptr;
	inline static void (*particles_set_speed_scale)(RID p_particles, double p_scale) = nullptr;
	inline static void (*particles_set_use_local_coordinates)(RID p_particles, bool p_enable) = nullptr;
	inline static void (*particles_set_process_material)(RID p_particles, RID p_material) = nullptr;
	inline static RID (*particles_get_process_material)(RID p_particles) = nullptr;
	inline static void (*particles_set_fixed_fps)(RID p_particles, int p_fps) = nullptr;
	inline static void (*particles_set_interpolate)(RID p_particles, bool p_enable) = nullptr;
	inline static void (*particles_set_fractional_delta)(RID p_particles, bool p_enable) = nullptr;
	inline static void (*particles_set_collision_base_size)(RID p_particles, real_t p_size) = nullptr;

	inline static void (*particles_set_transform_align)(
		RID p_particles, RSE::ParticlesTransformAlign p_transform_align) = nullptr;
	inline static void (*particles_set_transform_align_channel_filter)(
		RID p_particles, RSE::ParticlesTransformAlignCustomSrc p_channel_filter) = nullptr;
	inline static void (*particles_set_transform_align_axis)(
		RID p_particles, RSE::ParticlesTransformAlignAxis p_rotation_axis) = nullptr;

	inline static void (*particles_set_seed)(RID p_particles, uint32_t p_seed) = nullptr;

	inline static void (*particles_set_trails)(RID p_particles, bool p_enable, double p_length) = nullptr;
	inline static void (*particles_set_trail_bind_poses)(
		RID p_particles, const Vector<Transform3D> &p_bind_poses) = nullptr;

	inline static void (*particles_restart)(RID p_particles) = nullptr;
	inline static void (*particles_emit)(
		RID p_particles, const Transform3D &p_transform, const Vector3 &p_velocity,
		const Color &p_color, const Color &p_custom, uint32_t p_emit_flags) = nullptr;
	inline static void (*particles_set_subemitter)(RID p_particles, RID p_subemitter_particles) = nullptr;

	inline static bool (*particles_is_inactive)(RID p_particles) = nullptr;

	inline static void (*particles_set_draw_order)(RID p_particles, RSE::ParticlesDrawOrder p_order) = nullptr;

	inline static void (*particles_set_draw_passes)(RID p_particles, int p_count) = nullptr;
	inline static void (*particles_set_draw_pass_mesh)(RID p_particles, int p_pass, RID p_mesh) = nullptr;

	inline static void (*particles_request_process)(RID p_particles) = nullptr;
	inline static AABB (*particles_get_current_aabb)(RID p_particles) = nullptr;
	inline static AABB (*particles_get_aabb)(RID p_particles) = nullptr;

	inline static void (*particles_set_emission_transform)(
		RID p_particles, const Transform3D &p_transform) = nullptr;
	inline static void (*particles_set_emitter_velocity)(RID p_particles, const Vector3 &p_velocity) = nullptr;
	inline static void (*particles_set_interp_to_end)(RID p_particles, float p_interp_to_end) = nullptr;

	inline static int (*particles_get_draw_passes)(RID p_particles) = nullptr;
	inline static RID (*particles_get_draw_pass_mesh)(RID p_particles, int p_pass) = nullptr;

	inline static void (*particles_set_view_axis)(
		RID p_particles, const Vector3 &p_axis, const Vector3 &p_up_axis) = nullptr;

	inline static void (*particles_add_collision)(RID p_particles, RID p_particles_collision_instance) = nullptr;
	inline static void (*particles_remove_collision)(RID p_particles, RID p_particles_collision_instance) = nullptr;

	inline static void (*update_particles)() = nullptr;

	/* PARTICLES COLLISION FUNCTION POINTERS */
	inline static RID (*particles_collision_allocate)() = nullptr;
	inline static void (*particles_collision_initialize)(RID p_rid) = nullptr;
	inline static void (*particles_collision_free)(RID p_rid) = nullptr;

	inline static void (*particles_collision_set_collision_type)(
		RID p_particles_collision, RSE::ParticlesCollisionType p_type) = nullptr;
	inline static void (*particles_collision_set_cull_mask)(
		RID p_particles_collision, uint32_t p_cull_mask) = nullptr;
	inline static void (*particles_collision_set_sphere_radius)(
		RID p_particles_collision, real_t p_radius) = nullptr;
	inline static void (*particles_collision_set_box_extents)(
		RID p_particles_collision, const Vector3 &p_extents) = nullptr;
	inline static void (*particles_collision_set_attractor_strength)(
		RID p_particles_collision, real_t p_strength) = nullptr;
	inline static void (*particles_collision_set_attractor_directionality)(
		RID p_particles_collision, real_t p_directionality) = nullptr;
	inline static void (*particles_collision_set_attractor_attenuation)(
		RID p_particles_collision, real_t p_curve) = nullptr;
	inline static void (*particles_collision_set_field_texture)(
		RID p_particles_collision, RID p_texture) = nullptr;
	inline static void (*particles_collision_height_field_update)(RID p_particles_collision) = nullptr;
	inline static void (*particles_collision_set_height_field_resolution)(
		RID p_particles_collision, RSE::ParticlesCollisionHeightfieldResolution p_resolution) = nullptr;
	inline static AABB (*particles_collision_get_aabb)(RID p_particles_collision) = nullptr;
	inline static bool (*particles_collision_is_heightfield)(RID p_particles_collision) = nullptr;
	inline static uint32_t (*particles_collision_get_height_field_mask)(RID p_particles_collision) = nullptr;
	inline static void (*particles_collision_set_height_field_mask)(
		RID p_particles_collision, uint32_t p_heightfield_mask) = nullptr;
	inline static uint32_t (*particles_collision_get_cull_mask)(RID p_particles_collision) = nullptr;

	inline static RID (*particles_collision_instance_create)(RID p_collision) = nullptr;
	inline static void (*particles_collision_instance_free)(RID p_rid) = nullptr;
	inline static void (*particles_collision_instance_set_transform)(
		RID p_collision_instance, const Transform3D &p_transform) = nullptr;
	inline static void (*particles_collision_instance_set_active)(
		RID p_collision_instance, bool p_active) = nullptr;

	/* BIND TEMPLATE */
	template <typename T>
	static void bind()
	{
		particles_allocate = T::particles_allocate;
		particles_initialize = T::particles_initialize;
		particles_free = T::particles_free;

		particles_set_mode = T::particles_set_mode;

		particles_set_emitting = T::particles_set_emitting;
		particles_get_emitting = T::particles_get_emitting;

		particles_set_amount = T::particles_set_amount;
		particles_set_amount_ratio = T::particles_set_amount_ratio;
		particles_set_lifetime = T::particles_set_lifetime;
		particles_set_one_shot = T::particles_set_one_shot;
		particles_set_pre_process_time = T::particles_set_pre_process_time;
		particles_request_process_time = T::particles_request_process_time;
		particles_set_explosiveness_ratio = T::particles_set_explosiveness_ratio;
		particles_set_randomness_ratio = T::particles_set_randomness_ratio;
		particles_set_custom_aabb = T::particles_set_custom_aabb;
		particles_set_speed_scale = T::particles_set_speed_scale;
		particles_set_use_local_coordinates = T::particles_set_use_local_coordinates;
		particles_set_process_material = T::particles_set_process_material;
		particles_get_process_material = T::particles_get_process_material;
		particles_set_fixed_fps = T::particles_set_fixed_fps;
		particles_set_interpolate = T::particles_set_interpolate;
		particles_set_fractional_delta = T::particles_set_fractional_delta;
		particles_set_collision_base_size = T::particles_set_collision_base_size;

		particles_set_transform_align = T::particles_set_transform_align;
		particles_set_transform_align_channel_filter = T::particles_set_transform_align_channel_filter;
		particles_set_transform_align_axis = T::particles_set_transform_align_axis;

		particles_set_seed = T::particles_set_seed;

		particles_set_trails = T::particles_set_trails;
		particles_set_trail_bind_poses = T::particles_set_trail_bind_poses;

		particles_restart = T::particles_restart;
		particles_emit = T::particles_emit;
		particles_set_subemitter = T::particles_set_subemitter;

		particles_is_inactive = T::particles_is_inactive;

		particles_set_draw_order = T::particles_set_draw_order;

		particles_set_draw_passes = T::particles_set_draw_passes;
		particles_set_draw_pass_mesh = T::particles_set_draw_pass_mesh;

		particles_request_process = T::particles_request_process;
		particles_get_current_aabb = T::particles_get_current_aabb;
		particles_get_aabb = T::particles_get_aabb;

		particles_set_emission_transform = T::particles_set_emission_transform;
		particles_set_emitter_velocity = T::particles_set_emitter_velocity;
		particles_set_interp_to_end = T::particles_set_interp_to_end;

		particles_get_draw_passes = T::particles_get_draw_passes;
		particles_get_draw_pass_mesh = T::particles_get_draw_pass_mesh;

		particles_set_view_axis = T::particles_set_view_axis;

		particles_add_collision = T::particles_add_collision;
		particles_remove_collision = T::particles_remove_collision;

		update_particles = T::update_particles;

		particles_collision_allocate = T::particles_collision_allocate;
		particles_collision_initialize = T::particles_collision_initialize;
		particles_collision_free = T::particles_collision_free;

		particles_collision_set_collision_type = T::particles_collision_set_collision_type;
		particles_collision_set_cull_mask = T::particles_collision_set_cull_mask;
		particles_collision_set_sphere_radius = T::particles_collision_set_sphere_radius;
		particles_collision_set_box_extents = T::particles_collision_set_box_extents;
		particles_collision_set_attractor_strength = T::particles_collision_set_attractor_strength;
		particles_collision_set_attractor_directionality = T::particles_collision_set_attractor_directionality;
		particles_collision_set_attractor_attenuation = T::particles_collision_set_attractor_attenuation;
		particles_collision_set_field_texture = T::particles_collision_set_field_texture;
		particles_collision_height_field_update = T::particles_collision_height_field_update;
		particles_collision_set_height_field_resolution = T::particles_collision_set_height_field_resolution;
		particles_collision_get_aabb = T::particles_collision_get_aabb;
		particles_collision_is_heightfield = T::particles_collision_is_heightfield;
		particles_collision_get_height_field_mask = T::particles_collision_get_height_field_mask;
		particles_collision_set_height_field_mask = T::particles_collision_set_height_field_mask;
		particles_collision_get_cull_mask = T::particles_collision_get_cull_mask;

		particles_collision_instance_create = T::particles_collision_instance_create;
		particles_collision_instance_free = T::particles_collision_instance_free;
		particles_collision_instance_set_transform = T::particles_collision_instance_set_transform;
		particles_collision_instance_set_active = T::particles_collision_instance_set_active;
	}
};
// --- Inlined/Folded from servers/rendering/storage/texture_storage.h ---
#include "core/io/image.h"
#include "core/math/aabb.h"
#include "core/math/color.h"
#include "core/math/rect2.h"
#include "core/math/transform_3d.h"
#include "core/math/vector3.h"
#include "core/templates/rid.h"
#include "servers/rendering/rendering_server_enums.h"
#include "servers/rendering/rendering_server_types.h"

struct RenderTargetSetOverrideFn
{
	void (*ptr)(RID, RID, RID, RID, RID) = nullptr;

	void operator=(void (*p_ptr)(RID, RID, RID, RID, RID)) { ptr = p_ptr; }

	_FORCE_INLINE_ void operator()(RID p_render_target, RID p_color_texture, RID p_depth_texture,
		RID p_velocity = RID(), RID p_velocity_depth = RID()) const
	{
		if (ptr) {
			ptr(p_render_target, p_color_texture, p_depth_texture, p_velocity, p_velocity_depth);
		}
	}
};

namespace TextureStorage
{

struct API
{
	Color (*get_default_clear_color)() = nullptr;
	/* CANVAS TEXTURE API */
	RID (*canvas_texture_allocate)() = nullptr;
	void (*canvas_texture_initialize)(RID p_rid) = nullptr;
	void (*canvas_texture_free)(RID p_rid) = nullptr;

	void (*canvas_texture_set_channel)(
		RID p_canvas_texture, RSE::CanvasTextureChannel p_channel, RID p_texture) = nullptr;
	void (*canvas_texture_set_shading_parameters)(
		RID p_canvas_texture, const Color& p_base_color, float p_shininess) = nullptr;
	void (*canvas_texture_set_texture_filter)(
		RID p_canvas_texture, RSE::CanvasItemTextureFilter p_filter) = nullptr;
	void (*canvas_texture_set_texture_repeat)(
		RID p_canvas_texture, RSE::CanvasItemTextureRepeat p_repeat) = nullptr;
	void (*set_default_clear_color)(const Color& p_color) = nullptr;

	/* TEXTURE 2D / GENERAL API */
	RID (*texture_allocate)() = nullptr;
	void (*texture_free)(RID p_rid) = nullptr;

	void (*texture_2d_initialize)(RID p_texture, const Ref<Image>& p_image) = nullptr;
	void (*texture_2d_update)(RID p_texture, const Ref<Image>& p_image, int p_layer) = nullptr;
	Ref<Image> (*texture_2d_get)(RID p_texture) = nullptr;
	Ref<Image> (*texture_2d_layer_get)(RID p_texture, int p_layer) = nullptr;

	void (*texture_2d_placeholder_initialize)(RID p_texture) = nullptr;

	void (*texture_2d_layered_initialize)(RID p_texture, const Vector<Ref<Image>>& p_layers,
		RSE::TextureLayeredType p_layered_type) = nullptr;
	void (*texture_2d_layered_placeholder_initialize)(
		RID p_texture, RSE::TextureLayeredType p_layered_type) = nullptr;

	void (*texture_3d_initialize)(RID p_texture, Image::Format p_format, int p_width, int p_height,
		int p_depth, bool p_mipmaps, const Vector<Ref<Image>>& p_data) = nullptr;
	void (*texture_3d_update)(RID p_texture, const Vector<Ref<Image>>& p_data) = nullptr;
	Vector<Ref<Image>> (*texture_3d_get)(RID p_texture) = nullptr;
	void (*texture_3d_placeholder_initialize)(RID p_texture) = nullptr;

	void (*texture_proxy_initialize)(RID p_texture, RID p_base) = nullptr;
	void (*texture_proxy_update)(RID p_proxy, RID p_base) = nullptr;

	void (*texture_drawable_initialize)(RID p_texture, int p_width, int p_height,
		RSE::TextureDrawableFormat p_format, const Color& p_color, bool p_has_mipmaps) = nullptr;
	void (*texture_drawable_generate_mipmaps)(RID p_texture) = nullptr;
	RID (*texture_drawable_get_default_material)() = nullptr;

	void (*texture_external_initialize)(
		RID p_texture, int p_width, int p_height, uint64_t p_external_buffer) = nullptr;
	void (*texture_external_update)(
		RID p_texture, int p_width, int p_height, uint64_t p_external_buffer) = nullptr;

	uint64_t (*texture_get_native_handle)(RID p_texture, bool p_srgb) = nullptr;

	void (*texture_set_format)(RID p_texture, Image::Format p_format) = nullptr;
	Image::Format (*texture_get_format)(RID p_texture) = nullptr;

	void (*texture_set_path)(RID p_texture, const String& p_path) = nullptr;
	String (*texture_get_path)(RID p_texture) = nullptr;
	void (*texture_set_size_override)(RID p_texture, int p_width, int p_height) = nullptr;

	void (*texture_replace)(RID p_texture, RID p_by_texture) = nullptr;
	void (*texture_set_force_redraw_if_visible)(RID p_texture, bool p_enable) = nullptr;
	Size2 (*texture_size_with_proxy)(RID p_proxy) = nullptr;

	/* DECAL API */
	RID (*decal_allocate)() = nullptr;
	void (*decal_initialize)(RID p_rid) = nullptr;
	void (*decal_free)(RID p_rid) = nullptr;

	void (*decal_set_size)(RID p_decal, const Vector3& p_size) = nullptr;
	void (*decal_set_texture)(RID p_decal, RSE::DecalTexture p_type, RID p_texture) = nullptr;
	void (*decal_set_emission_energy)(RID p_decal, float p_energy) = nullptr;
	void (*decal_set_albedo_mix)(RID p_decal, float p_mix) = nullptr;
	void (*decal_set_modulate)(RID p_decal, const Color& p_modulate) = nullptr;
	void (*decal_set_cull_mask)(RID p_decal, uint32_t p_layers) = nullptr;
	void (*decal_set_distance_fade)(
		RID p_decal, bool p_enabled, float p_begin, float p_length) = nullptr;
	void (*decal_set_fade)(RID p_decal, float p_above, float p_below) = nullptr;
	void (*decal_set_normal_fade)(RID p_decal, float p_fade) = nullptr;

	AABB (*decal_get_aabb)(RID p_decal) = nullptr;
	uint32_t (*decal_get_cull_mask)(RID p_decal) = nullptr;

	RID (*decal_instance_create)(RID p_decal) = nullptr;
	void (*decal_instance_free)(RID p_decal_instance) = nullptr;
	void (*decal_instance_set_transform)(
		RID p_decal_instance, const Transform3D& p_transform) = nullptr;
	void (*decal_instance_set_sorting_offset)(
		RID p_decal_instance, float p_sorting_offset) = nullptr;

	/* RENDER TARGET API */
	RID (*render_target_create)() = nullptr;
	void (*render_target_free)(RID p_render_target) = nullptr;

	void (*render_target_set_position)(RID p_render_target, int p_x, int p_y) = nullptr;
	Point2i (*render_target_get_position)(RID p_render_target) = nullptr;

	void (*render_target_set_size)(
		RID p_render_target, int p_width, int p_height, uint32_t p_view_count) = nullptr;
	Size2i (*render_target_get_size)(RID p_render_target) = nullptr;

	void (*render_target_set_transparent)(RID p_render_target, bool p_is_transparent) = nullptr;
	void (*render_target_set_direct_to_screen)(
		RID p_render_target, bool p_direct_to_screen) = nullptr;

	RID (*render_target_get_texture)(RID p_render_target) = nullptr;

	void (*render_target_set_msaa)(RID p_render_target, RSE::ViewportMSAA p_msaa) = nullptr;
	RSE::ViewportMSAA (*render_target_get_msaa)(RID p_render_target) = nullptr;
	bool (*render_target_get_msaa_needs_resolve)(RID p_render_target) = nullptr;
	void (*render_target_do_msaa_resolve)(RID p_render_target) = nullptr;

	void (*render_target_set_use_hdr)(RID p_render_target, bool p_use_hdr) = nullptr;
	bool (*render_target_is_using_hdr)(RID p_render_target) = nullptr;

	void (*render_target_set_use_debanding)(RID p_render_target, bool p_use_debanding) = nullptr;

	void (*render_target_set_vrs_mode)(RID p_render_target, RSE::ViewportVRSMode p_mode) = nullptr;
	void (*render_target_set_vrs_update_mode)(
		RID p_render_target, RSE::ViewportVRSUpdateMode p_mode) = nullptr;
	void (*render_target_set_vrs_texture)(RID p_render_target, RID p_texture) = nullptr;

	void (*render_target_request_clear)(RID p_render_target, const Color& p_clear_color) = nullptr;
	bool (*render_target_is_clear_requested)(RID p_render_target) = nullptr;
	Color (*render_target_get_clear_request_color)(RID p_render_target) = nullptr;
	void (*render_target_do_clear_request)(RID p_render_target) = nullptr;

	void (*render_target_set_sdf_size_and_scale)(RID p_render_target,
		RSE::ViewportSDFOversize p_size, RSE::ViewportSDFScale p_scale) = nullptr;
	Rect2i (*render_target_get_sdf_rect)(RID p_render_target) = nullptr;
	void (*render_target_mark_sdf_enabled)(RID p_render_target, bool p_enabled) = nullptr;

	RenderTargetSetOverrideFn render_target_set_override;

	RID (*render_target_get_override_color)(RID p_render_target) = nullptr;
	RID (*render_target_get_override_depth)(RID p_render_target) = nullptr;
	RID (*render_target_get_override_velocity)(RID p_render_target) = nullptr;
	RID (*render_target_get_override_velocity_depth)(RID p_render_target) = nullptr;

	void (*render_target_set_render_region)(
		RID p_render_target, const Rect2i& p_render_region) = nullptr;
	Rect2i (*render_target_get_render_region)(RID p_render_target) = nullptr;

	void (*render_target_set_velocity_target_size)(
		RID p_render_target, const Size2i& p_target_size) = nullptr;
	Size2i (*render_target_get_velocity_target_size)(RID p_render_target) = nullptr;

	bool (*render_target_was_used)(RID p_render_target) = nullptr;
	void (*render_target_set_as_unused)(RID p_render_target) = nullptr;
};

}; // namespace TextureStorage

class RendererTextureStorage final
{
private:
	static inline TextureStorage::API api;

public:
	RendererTextureStorage() = delete;
	RendererTextureStorage(const RendererTextureStorage&) = delete;
	RendererTextureStorage& operator=(const RendererTextureStorage&) = delete;
	~RendererTextureStorage() = delete;

	template <typename StorageImpl> static void bind()
	{
		api.set_default_clear_color = &StorageImpl::set_default_clear_color;
		api.get_default_clear_color = &StorageImpl::get_default_clear_color;
		/* CANVAS TEXTURE API */
		api.canvas_texture_allocate = &StorageImpl::canvas_texture_allocate;
		api.canvas_texture_initialize = &StorageImpl::canvas_texture_initialize;
		api.canvas_texture_free = &StorageImpl::canvas_texture_free;
		api.canvas_texture_set_channel = &StorageImpl::canvas_texture_set_channel;
		api.canvas_texture_set_shading_parameters =
			&StorageImpl::canvas_texture_set_shading_parameters;
		api.canvas_texture_set_texture_filter = &StorageImpl::canvas_texture_set_texture_filter;
		api.canvas_texture_set_texture_repeat = &StorageImpl::canvas_texture_set_texture_repeat;

		/* TEXTURE 2D / GENERAL API */
		api.texture_allocate = &StorageImpl::texture_allocate;
		api.texture_free = &StorageImpl::texture_free;
		api.texture_2d_initialize = &StorageImpl::texture_2d_initialize;
		api.texture_2d_update = &StorageImpl::texture_2d_update;
		api.texture_2d_get = &StorageImpl::texture_2d_get;
		api.texture_2d_layer_get = &StorageImpl::texture_2d_layer_get;
		api.texture_2d_placeholder_initialize = &StorageImpl::texture_2d_placeholder_initialize;
		api.texture_2d_layered_initialize = &StorageImpl::texture_2d_layered_initialize;
		api.texture_2d_layered_placeholder_initialize =
			&StorageImpl::texture_2d_layered_placeholder_initialize;
		api.texture_3d_initialize = &StorageImpl::texture_3d_initialize;
		api.texture_3d_update = &StorageImpl::texture_3d_update;
		api.texture_3d_get = &StorageImpl::texture_3d_get;
		api.texture_3d_placeholder_initialize = &StorageImpl::texture_3d_placeholder_initialize;
		api.texture_proxy_initialize = &StorageImpl::texture_proxy_initialize;
		api.texture_proxy_update = &StorageImpl::texture_proxy_update;
		api.texture_drawable_initialize = &StorageImpl::texture_drawable_initialize;
		api.texture_drawable_generate_mipmaps = &StorageImpl::texture_drawable_generate_mipmaps;
		api.texture_drawable_get_default_material =
			&StorageImpl::texture_drawable_get_default_material;
		api.texture_external_initialize = &StorageImpl::texture_external_initialize;
		api.texture_external_update = &StorageImpl::texture_external_update;
		api.texture_get_native_handle = &StorageImpl::texture_get_native_handle;
		api.texture_set_format = &StorageImpl::texture_set_format;
		api.texture_get_format = &StorageImpl::texture_get_format;
		api.texture_set_path = &StorageImpl::texture_set_path;
		api.texture_get_path = &StorageImpl::texture_get_path;
		api.texture_set_size_override = &StorageImpl::texture_set_size_override;
		api.texture_replace = &StorageImpl::texture_replace;
		api.texture_set_force_redraw_if_visible = &StorageImpl::texture_set_force_redraw_if_visible;
		api.texture_size_with_proxy = &StorageImpl::texture_size_with_proxy;

		/* DECAL API */
		api.decal_allocate = &StorageImpl::decal_allocate;
		api.decal_initialize = &StorageImpl::decal_initialize;
		api.decal_free = &StorageImpl::decal_free;
		api.decal_set_size = &StorageImpl::decal_set_size;
		api.decal_set_texture = &StorageImpl::decal_set_texture;
		api.decal_set_emission_energy = &StorageImpl::decal_set_emission_energy;
		api.decal_set_albedo_mix = &StorageImpl::decal_set_albedo_mix;
		api.decal_set_modulate = &StorageImpl::decal_set_modulate;
		api.decal_set_cull_mask = &StorageImpl::decal_set_cull_mask;
		api.decal_set_distance_fade = &StorageImpl::decal_set_distance_fade;
		api.decal_set_fade = &StorageImpl::decal_set_fade;
		api.decal_set_normal_fade = &StorageImpl::decal_set_normal_fade;
		api.decal_get_aabb = &StorageImpl::decal_get_aabb;
		api.decal_get_cull_mask = &StorageImpl::decal_get_cull_mask;
		api.decal_instance_create = &StorageImpl::decal_instance_create;
		api.decal_instance_free = &StorageImpl::decal_instance_free;
		api.decal_instance_set_transform = &StorageImpl::decal_instance_set_transform;
		api.decal_instance_set_sorting_offset = &StorageImpl::decal_instance_set_sorting_offset;

		/* RENDER TARGET API */
		api.render_target_create = &StorageImpl::render_target_create;
		api.render_target_free = &StorageImpl::render_target_free;
		api.render_target_set_position = &StorageImpl::render_target_set_position;
		api.render_target_get_position = &StorageImpl::render_target_get_position;
		api.render_target_set_size = &StorageImpl::render_target_set_size;
		api.render_target_get_size = &StorageImpl::render_target_get_size;
		api.render_target_set_transparent = &StorageImpl::render_target_set_transparent;
		api.render_target_set_direct_to_screen = &StorageImpl::render_target_set_direct_to_screen;
		api.render_target_get_texture = &StorageImpl::render_target_get_texture;
		api.render_target_set_msaa = &StorageImpl::render_target_set_msaa;
		api.render_target_get_msaa = &StorageImpl::render_target_get_msaa;
		api.render_target_get_msaa_needs_resolve =
			&StorageImpl::render_target_get_msaa_needs_resolve;
		api.render_target_do_msaa_resolve = &StorageImpl::render_target_do_msaa_resolve;
		api.render_target_set_use_hdr = &StorageImpl::render_target_set_use_hdr;
		api.render_target_is_using_hdr = &StorageImpl::render_target_is_using_hdr;
		api.render_target_set_use_debanding = &StorageImpl::render_target_set_use_debanding;
		api.render_target_set_vrs_mode = &StorageImpl::render_target_set_vrs_mode;
		api.render_target_set_vrs_update_mode = &StorageImpl::render_target_set_vrs_update_mode;
		api.render_target_set_vrs_texture = &StorageImpl::render_target_set_vrs_texture;
		api.render_target_request_clear = &StorageImpl::render_target_request_clear;
		api.render_target_is_clear_requested = &StorageImpl::render_target_is_clear_requested;
		api.render_target_get_clear_request_color =
			&StorageImpl::render_target_get_clear_request_color;
		api.render_target_do_clear_request = &StorageImpl::render_target_do_clear_request;
		api.render_target_set_sdf_size_and_scale =
			&StorageImpl::render_target_set_sdf_size_and_scale;
		api.render_target_get_sdf_rect = &StorageImpl::render_target_get_sdf_rect;
		api.render_target_mark_sdf_enabled = &StorageImpl::render_target_mark_sdf_enabled;
		api.render_target_set_override = &StorageImpl::render_target_set_override;
		api.render_target_get_override_color = &StorageImpl::render_target_get_override_color;
		api.render_target_get_override_depth = &StorageImpl::render_target_get_override_depth;
		api.render_target_get_override_velocity = &StorageImpl::render_target_get_override_velocity;
		api.render_target_get_override_velocity_depth =
			&StorageImpl::render_target_get_override_velocity_depth;
		api.render_target_set_render_region = &StorageImpl::render_target_set_render_region;
		api.render_target_get_render_region = &StorageImpl::render_target_get_render_region;
		api.render_target_set_velocity_target_size =
			&StorageImpl::render_target_set_velocity_target_size;
		api.render_target_get_velocity_target_size =
			&StorageImpl::render_target_get_velocity_target_size;
		api.render_target_was_used = &StorageImpl::render_target_was_used;
		api.render_target_set_as_unused = &StorageImpl::render_target_set_as_unused;
	}

	/* CANVAS TEXTURE API */
	_FORCE_INLINE_ static RID canvas_texture_allocate() { return api.canvas_texture_allocate(); }

	_FORCE_INLINE_ static void canvas_texture_initialize(RID p_rid)
	{
		api.canvas_texture_initialize(p_rid);
	}

	_FORCE_INLINE_ static void canvas_texture_free(RID p_rid) { api.canvas_texture_free(p_rid); }

	_FORCE_INLINE_ static void canvas_texture_set_channel(
		RID p_canvas_texture, RSE::CanvasTextureChannel p_channel, RID p_texture)
	{
		api.canvas_texture_set_channel(p_canvas_texture, p_channel, p_texture);
	}

	_FORCE_INLINE_ static void canvas_texture_set_shading_parameters(
		RID p_canvas_texture, const Color& p_base_color, float p_shininess)
	{
		api.canvas_texture_set_shading_parameters(p_canvas_texture, p_base_color, p_shininess);
	}

	_FORCE_INLINE_ static void canvas_texture_set_texture_filter(
		RID p_canvas_texture, RSE::CanvasItemTextureFilter p_filter)
	{
		api.canvas_texture_set_texture_filter(p_canvas_texture, p_filter);
	}

	_FORCE_INLINE_ static void canvas_texture_set_texture_repeat(
		RID p_canvas_texture, RSE::CanvasItemTextureRepeat p_repeat)
	{
		api.canvas_texture_set_texture_repeat(p_canvas_texture, p_repeat);
	}

	/* TEXTURE 2D / GENERAL API */
	_FORCE_INLINE_ static RID texture_allocate() { return api.texture_allocate(); }

	_FORCE_INLINE_ static void texture_free(RID p_rid) { api.texture_free(p_rid); }

	_FORCE_INLINE_ static void texture_2d_initialize(RID p_texture, const Ref<Image>& p_image)
	{
		api.texture_2d_initialize(p_texture, p_image);
	}

	_FORCE_INLINE_ static void texture_2d_update(
		RID p_texture, const Ref<Image>& p_image, int p_layer)
	{
		api.texture_2d_update(p_texture, p_image, p_layer);
	}

	_FORCE_INLINE_ static Ref<Image> texture_2d_get(RID p_texture)
	{
		return api.texture_2d_get(p_texture);
	}

	_FORCE_INLINE_ static Ref<Image> texture_2d_layer_get(RID p_texture, int p_layer)
	{
		return api.texture_2d_layer_get(p_texture, p_layer);
	}

	_FORCE_INLINE_ static void texture_2d_placeholder_initialize(RID p_texture)
	{
		api.texture_2d_placeholder_initialize(p_texture);
	}

	_FORCE_INLINE_ static void texture_2d_layered_initialize(
		RID p_texture, const Vector<Ref<Image>>& p_layers, RSE::TextureLayeredType p_layered_type)
	{
		api.texture_2d_layered_initialize(p_texture, p_layers, p_layered_type);
	}

	_FORCE_INLINE_ static void texture_2d_layered_placeholder_initialize(
		RID p_texture, RSE::TextureLayeredType p_layered_type)
	{
		api.texture_2d_layered_placeholder_initialize(p_texture, p_layered_type);
	}

	_FORCE_INLINE_ static void texture_3d_initialize(RID p_texture, Image::Format p_format,
		int p_width, int p_height, int p_depth, bool p_mipmaps, const Vector<Ref<Image>>& p_data)
	{
		api.texture_3d_initialize(
			p_texture, p_format, p_width, p_height, p_depth, p_mipmaps, p_data);
	}

	_FORCE_INLINE_ static void texture_3d_update(RID p_texture, const Vector<Ref<Image>>& p_data)
	{
		api.texture_3d_update(p_texture, p_data);
	}

	_FORCE_INLINE_ static Vector<Ref<Image>> texture_3d_get(RID p_texture)
	{
		return api.texture_3d_get(p_texture);
	}

	_FORCE_INLINE_ static void texture_3d_placeholder_initialize(RID p_texture)
	{
		api.texture_3d_placeholder_initialize(p_texture);
	}

	_FORCE_INLINE_ static void texture_proxy_initialize(RID p_texture, RID p_base)
	{
		api.texture_proxy_initialize(p_texture, p_base);
	}

	_FORCE_INLINE_ static void texture_proxy_update(RID p_proxy, RID p_base)
	{
		api.texture_proxy_update(p_proxy, p_base);
	}

	_FORCE_INLINE_ static void texture_drawable_initialize(RID p_texture, int p_width, int p_height,
		RSE::TextureDrawableFormat p_format, const Color& p_color, bool p_has_mipmaps)
	{
		api.texture_drawable_initialize(
			p_texture, p_width, p_height, p_format, p_color, p_has_mipmaps);
	}

	_FORCE_INLINE_ static void texture_drawable_generate_mipmaps(RID p_texture)
	{
		api.texture_drawable_generate_mipmaps(p_texture);
	}

	_FORCE_INLINE_ static RID texture_drawable_get_default_material()
	{
		return api.texture_drawable_get_default_material();
	}

	_FORCE_INLINE_ static void texture_external_initialize(
		RID p_texture, int p_width, int p_height, uint64_t p_external_buffer)
	{
		api.texture_external_initialize(p_texture, p_width, p_height, p_external_buffer);
	}

	_FORCE_INLINE_ static void texture_external_update(
		RID p_texture, int p_width, int p_height, uint64_t p_external_buffer)
	{
		api.texture_external_update(p_texture, p_width, p_height, p_external_buffer);
	}

	_FORCE_INLINE_ static uint64_t texture_get_native_handle(RID p_texture, bool p_srgb)
	{
		return api.texture_get_native_handle(p_texture, p_srgb);
	}

	_FORCE_INLINE_ static void texture_set_format(RID p_texture, Image::Format p_format)
	{
		api.texture_set_format(p_texture, p_format);
	}

	_FORCE_INLINE_ static Image::Format texture_get_format(RID p_texture)
	{
		return api.texture_get_format(p_texture);
	}

	_FORCE_INLINE_ static void texture_set_path(RID p_texture, const String& p_path)
	{
		api.texture_set_path(p_texture, p_path);
	}

	_FORCE_INLINE_ static String texture_get_path(RID p_texture)
	{
		return api.texture_get_path(p_texture);
	}

	_FORCE_INLINE_ static void texture_set_size_override(RID p_texture, int p_width, int p_height)
	{
		api.texture_set_size_override(p_texture, p_width, p_height);
	}

	_FORCE_INLINE_ static void texture_replace(RID p_texture, RID p_by_texture)
	{
		api.texture_replace(p_texture, p_by_texture);
	}

	_FORCE_INLINE_ static void texture_set_force_redraw_if_visible(RID p_texture, bool p_enable)
	{
		api.texture_set_force_redraw_if_visible(p_texture, p_enable);
	}

	_FORCE_INLINE_ static Size2 texture_size_with_proxy(RID p_proxy)
	{
		return api.texture_size_with_proxy(p_proxy);
	}

	/* DECAL API */
	_FORCE_INLINE_ static RID decal_allocate() { return api.decal_allocate(); }

	_FORCE_INLINE_ static void decal_initialize(RID p_rid) { api.decal_initialize(p_rid); }

	_FORCE_INLINE_ static void decal_free(RID p_rid) { api.decal_free(p_rid); }

	_FORCE_INLINE_ static void decal_set_size(RID p_decal, const Vector3& p_size)
	{
		api.decal_set_size(p_decal, p_size);
	}

	_FORCE_INLINE_ static void decal_set_texture(
		RID p_decal, RSE::DecalTexture p_type, RID p_texture)
	{
		api.decal_set_texture(p_decal, p_type, p_texture);
	}

	_FORCE_INLINE_ static void decal_set_emission_energy(RID p_decal, float p_energy)
	{
		api.decal_set_emission_energy(p_decal, p_energy);
	}

	_FORCE_INLINE_ static void decal_set_albedo_mix(RID p_decal, float p_mix)
	{
		api.decal_set_albedo_mix(p_decal, p_mix);
	}

	_FORCE_INLINE_ static void decal_set_modulate(RID p_decal, const Color& p_modulate)
	{
		api.decal_set_modulate(p_decal, p_modulate);
	}

	_FORCE_INLINE_ static void decal_set_cull_mask(RID p_decal, uint32_t p_layers)
	{
		api.decal_set_cull_mask(p_decal, p_layers);
	}

	_FORCE_INLINE_ static void decal_set_distance_fade(
		RID p_decal, bool p_enabled, float p_begin, float p_length)
	{
		api.decal_set_distance_fade(p_decal, p_enabled, p_begin, p_length);
	}

	_FORCE_INLINE_ static void decal_set_fade(RID p_decal, float p_above, float p_below)
	{
		api.decal_set_fade(p_decal, p_above, p_below);
	}

	_FORCE_INLINE_ static void decal_set_normal_fade(RID p_decal, float p_fade)
	{
		api.decal_set_normal_fade(p_decal, p_fade);
	}

	_FORCE_INLINE_ static AABB decal_get_aabb(RID p_decal) { return api.decal_get_aabb(p_decal); }

	_FORCE_INLINE_ static uint32_t decal_get_cull_mask(RID p_decal)
	{
		return api.decal_get_cull_mask(p_decal);
	}

	_FORCE_INLINE_ static RID decal_instance_create(RID p_decal)
	{
		return api.decal_instance_create(p_decal);
	}

	_FORCE_INLINE_ static void decal_instance_free(RID p_decal_instance)
	{
		api.decal_instance_free(p_decal_instance);
	}

	_FORCE_INLINE_ static void decal_instance_set_transform(
		RID p_decal_instance, const Transform3D& p_transform)
	{
		api.decal_instance_set_transform(p_decal_instance, p_transform);
	}

	_FORCE_INLINE_ static void decal_instance_set_sorting_offset(
		RID p_decal_instance, float p_sorting_offset)
	{
		api.decal_instance_set_sorting_offset(p_decal_instance, p_sorting_offset);
	}

	/* RENDER TARGET API */
	_FORCE_INLINE_ static RID render_target_create() { return api.render_target_create(); }

	_FORCE_INLINE_ static void render_target_free(RID p_render_target)
	{
		api.render_target_free(p_render_target);
	}

	_FORCE_INLINE_ static void render_target_set_position(RID p_render_target, int p_x, int p_y)
	{
		api.render_target_set_position(p_render_target, p_x, p_y);
	}

	_FORCE_INLINE_ static Point2i render_target_get_position(RID p_render_target)
	{
		return api.render_target_get_position(p_render_target);
	}

	_FORCE_INLINE_ static void render_target_set_size(
		RID p_render_target, int p_width, int p_height, uint32_t p_view_count)
	{
		api.render_target_set_size(p_render_target, p_width, p_height, p_view_count);
	}

	_FORCE_INLINE_ static Size2i render_target_get_size(RID p_render_target)
	{
		return api.render_target_get_size(p_render_target);
	}

	_FORCE_INLINE_ static void render_target_set_transparent(
		RID p_render_target, bool p_is_transparent)
	{
		api.render_target_set_transparent(p_render_target, p_is_transparent);
	}

	_FORCE_INLINE_ static void render_target_set_direct_to_screen(
		RID p_render_target, bool p_direct_to_screen)
	{
		api.render_target_set_direct_to_screen(p_render_target, p_direct_to_screen);
	}

	_FORCE_INLINE_ static RID render_target_get_texture(RID p_render_target)
	{
		return api.render_target_get_texture(p_render_target);
	}

	_FORCE_INLINE_ static void render_target_set_msaa(RID p_render_target, RSE::ViewportMSAA p_msaa)
	{
		api.render_target_set_msaa(p_render_target, p_msaa);
	}

	_FORCE_INLINE_ static RSE::ViewportMSAA render_target_get_msaa(RID p_render_target)
	{
		return api.render_target_get_msaa(p_render_target);
	}

	_FORCE_INLINE_ static bool render_target_get_msaa_needs_resolve(RID p_render_target)
	{
		return api.render_target_get_msaa_needs_resolve(p_render_target);
	}

	_FORCE_INLINE_ static void render_target_do_msaa_resolve(RID p_render_target)
	{
		api.render_target_do_msaa_resolve(p_render_target);
	}

	_FORCE_INLINE_ static void render_target_set_use_hdr(RID p_render_target, bool p_use_hdr)
	{
		api.render_target_set_use_hdr(p_render_target, p_use_hdr);
	}

	_FORCE_INLINE_ static bool render_target_is_using_hdr(RID p_render_target)
	{
		return api.render_target_is_using_hdr(p_render_target);
	}

	_FORCE_INLINE_ static void render_target_set_use_debanding(
		RID p_render_target, bool p_use_debanding)
	{
		api.render_target_set_use_debanding(p_render_target, p_use_debanding);
	}

	_FORCE_INLINE_ static void render_target_set_vrs_mode(
		RID p_render_target, RSE::ViewportVRSMode p_mode)
	{
		api.render_target_set_vrs_mode(p_render_target, p_mode);
	}

	_FORCE_INLINE_ static void render_target_set_vrs_update_mode(
		RID p_render_target, RSE::ViewportVRSUpdateMode p_mode)
	{
		api.render_target_set_vrs_update_mode(p_render_target, p_mode);
	}

	_FORCE_INLINE_ static void render_target_set_vrs_texture(RID p_render_target, RID p_texture)
	{
		api.render_target_set_vrs_texture(p_render_target, p_texture);
	}

	_FORCE_INLINE_ static void render_target_request_clear(
		RID p_render_target, const Color& p_clear_color)
	{
		api.render_target_request_clear(p_render_target, p_clear_color);
	}

	_FORCE_INLINE_ static bool render_target_is_clear_requested(RID p_render_target)
	{
		return api.render_target_is_clear_requested(p_render_target);
	}

	_FORCE_INLINE_ static Color render_target_get_clear_request_color(RID p_render_target)
	{
		return api.render_target_get_clear_request_color(p_render_target);
	}

	_FORCE_INLINE_ static void render_target_do_clear_request(RID p_render_target)
	{
		api.render_target_do_clear_request(p_render_target);
	}

	_FORCE_INLINE_ static void render_target_set_sdf_size_and_scale(
		RID p_render_target, RSE::ViewportSDFOversize p_size, RSE::ViewportSDFScale p_scale)
	{
		api.render_target_set_sdf_size_and_scale(p_render_target, p_size, p_scale);
	}

	_FORCE_INLINE_ static Rect2i render_target_get_sdf_rect(RID p_render_target)
	{
		return api.render_target_get_sdf_rect(p_render_target);
	}

	_FORCE_INLINE_ static void render_target_mark_sdf_enabled(RID p_render_target, bool p_enabled)
	{
		api.render_target_mark_sdf_enabled(p_render_target, p_enabled);
	}

	_FORCE_INLINE_ static void render_target_set_override(RID p_render_target, RID p_color_texture,
		RID p_depth_texture, RID p_velocity = RID(), RID p_velocity_depth = RID())
	{
		api.render_target_set_override(
			p_render_target, p_color_texture, p_depth_texture, p_velocity, p_velocity_depth);
	}

	_FORCE_INLINE_ static RID render_target_get_override_color(RID p_render_target)
	{
		return api.render_target_get_override_color(p_render_target);
	}

	_FORCE_INLINE_ static RID render_target_get_override_depth(RID p_render_target)
	{
		return api.render_target_get_override_depth(p_render_target);
	}

	_FORCE_INLINE_ static RID render_target_get_override_velocity(RID p_render_target)
	{
		return api.render_target_get_override_velocity(p_render_target);
	}

	_FORCE_INLINE_ static RID render_target_get_override_velocity_depth(RID p_render_target)
	{
		return api.render_target_get_override_velocity_depth(p_render_target);
	}

	_FORCE_INLINE_ static void render_target_set_render_region(
		RID p_render_target, const Rect2i& p_render_region)
	{
		api.render_target_set_render_region(p_render_target, p_render_region);
	}

	_FORCE_INLINE_ static Rect2i render_target_get_render_region(RID p_render_target)
	{
		return api.render_target_get_render_region(p_render_target);
	}

	_FORCE_INLINE_ static void render_target_set_velocity_target_size(
		RID p_render_target, const Size2i& p_target_size)
	{
		api.render_target_set_velocity_target_size(p_render_target, p_target_size);
	}

	_FORCE_INLINE_ static Size2i render_target_get_velocity_target_size(RID p_render_target)
	{
		return api.render_target_get_velocity_target_size(p_render_target);
	}

	_FORCE_INLINE_ static bool render_target_was_used(RID p_render_target)
	{
		return api.render_target_was_used(p_render_target);
	}

	_FORCE_INLINE_ static void render_target_set_as_unused(RID p_render_target)
	{
		api.render_target_set_as_unused(p_render_target);
	}

	_FORCE_INLINE_ static void set_default_clear_color(const Color& p_color)
	{
		if (api.set_default_clear_color) {
			api.set_default_clear_color(p_color);
		}
	}

	_FORCE_INLINE_ static Color get_default_clear_color()
	{
		if (api.get_default_clear_color) {
			return api.get_default_clear_color();
		}
		return Color();

	}
};

namespace Geometry3D
{
struct MeshData;
}

#ifdef DEBUG_ENABLED
#define ERR_NOT_ON_RENDER_THREAD ERR_FAIL_COND(!Renderer::is_on_render_thread());
#define ERR_NOT_ON_RENDER_THREAD_V(m_ret) ERR_FAIL_COND_V(!Renderer::is_on_render_thread(), m_ret);
#else
#define ERR_NOT_ON_RENDER_THREAD
#define ERR_NOT_ON_RENDER_THREAD_V(m_ret)
#endif

class RenderingDevice;
class RendererCanvasRender;
class RendererCanvasCull;
class RendererViewport;
class RenderingMethod;

class Renderer final
{
	static inline BinaryMutex _thread_safe_mutex;

public:
	static inline const Vector2 SMALL_VEC2 = Vector2(CMP_EPSILON, CMP_EPSILON);
	static inline const Vector3 SMALL_VEC3 = Vector3(CMP_EPSILON, CMP_EPSILON, CMP_EPSILON);

private:
	struct Data
	{
		struct Backend
		{
			enum
			{
				MAX_INSTANCE_CULL = 8192,
				MAX_INSTANCE_LIGHTS = 4,
				LIGHT_CACHE_DIRTY = -1,
				MAX_LIGHTS_CULLED = 256,
				MAX_ROOM_CULL = 32,
				MAX_EXTERIOR_PORTALS = 128,
				MAX_LIGHT_SAMPLERS = 256,
				INSTANCE_ROOMLESS_MASK = (1 << 20)
			};

			// Active subsystem driver
			RendererCompositor* compositor = nullptr;

			// Frame profiling & debug tracking
			uint64_t frame_profile_frame = 0;
			Vector<RenderingServerTypes::FrameProfileArea> frame_profile;
			double frame_setup_time = 0.0;
			bool print_gpu_profile = false;
			HashMap<String, float> print_gpu_profile_task_time;
			uint64_t print_frame_profile_ticks_from = 0;
			uint32_t print_frame_profile_frame_count = 0;

			int changes = 0;
			RID test_cube;
		};

		int mm_policy = 0;
		bool render_loop_enabled = true;

		RID test_texture;
		RID white_texture;
		RID test_material;

		Backend backend;
	};

	static RID _make_test_cube();
	static void _free_internal_rids();

public:
	static inline Data* data = nullptr;

	/* SUBSYSTEM & STORAGE POINTERS */
	static inline bool threaded = false;

	static inline RendererParticlesStorage* particles_storage = nullptr;
	static inline RendererFog* fog = nullptr;
	static inline RendererCanvasRender* canvas_render = nullptr;

	static inline RendererCanvasCull* canvas = nullptr;
	static inline RendererViewport* viewport = nullptr;

	static void fix_surface_compatibility(
		RenderingServerTypes::SurfaceData& p_surface, const String& p_path = String());
	static void set_boot_image(
		const Ref<Image>& p_image, const Color& p_color, bool p_scale, bool p_use_filter);

	/* SERVER LIFECYCLE */

	static void initialize();
	static void finalize();

	static bool is_initialized() { return data != nullptr; }

	/* TEXTURE API */

	static RID texture_2d_create(const Ref<Image>& p_image);
	static RID texture_2d_layered_create(
		const Vector<Ref<Image>>& p_layers, RSE::TextureLayeredType p_layered_type);
	static RID texture_3d_create(Image::Format p_format, int p_width, int p_height, int p_depth,
		bool p_mipmaps, const Vector<Ref<Image>>& p_data);
	static RID texture_external_create(int p_width, int p_height, uint64_t p_external_buffer = 0);
	static RID texture_proxy_create(RID p_base);
	static RID texture_drawable_create(int p_width, int p_height,
		RSE::TextureDrawableFormat p_format, const Color& p_color = Color(1, 1, 1, 1),
		bool p_with_mipmaps = false);

	static RID texture_create_from_native_handle(RSE::TextureType p_type, Image::Format p_format,
		uint64_t p_native_handle, int p_width, int p_height, int p_depth, int p_layers = 1,
		RSE::TextureLayeredType p_layered_type = RSE::TEXTURE_LAYERED_2D_ARRAY);

	static void texture_2d_update(RID p_texture, const Ref<Image>& p_image, int p_layer = 0);
	static void texture_3d_update(RID p_texture, const Vector<Ref<Image>>& p_data);
	static void texture_external_update(
		RID p_texture, int p_width, int p_height, uint64_t p_external_buffer = 0);
	static void texture_proxy_update(RID p_texture, RID p_proxy_to);

	static RID texture_2d_placeholder_create();
	static RID texture_2d_layered_placeholder_create(RSE::TextureLayeredType p_layered_type);
	static RID texture_3d_placeholder_create();

	static Ref<Image> texture_2d_get(RID p_texture);
	static Ref<Image> texture_2d_layer_get(RID p_texture, int p_layer);
	static Vector<Ref<Image>> texture_3d_get(RID p_texture);

	static void texture_replace(RID p_texture, RID p_by_texture);
	static void texture_set_size_override(RID p_texture, int p_width, int p_height);

	static void texture_set_path(RID p_texture, const String& p_path);
	static String texture_get_path(RID p_texture);

	static void texture_drawable_generate_mipmaps(RID p_texture);
	static RID texture_drawable_get_default_material();

	static Image::Format texture_get_format(RID p_texture);

	static void texture_set_detect_3d_callback(
		RID p_texture, RenderingServerTypes::TextureDetectCallback p_callback, void* p_userdata);
	static void texture_set_detect_normal_callback(
		RID p_texture, RenderingServerTypes::TextureDetectCallback p_callback, void* p_userdata);
	static void texture_set_detect_roughness_callback(RID p_texture,
		RenderingServerTypes::TextureDetectRoughnessCallback p_callback, void* p_userdata);

	static void texture_debug_usage(List<RenderingServerTypes::TextureInfo>* r_info);

	static void texture_set_force_redraw_if_visible(RID p_texture, bool p_enable);

	static RID texture_rd_create(const RID& p_rd_texture,
		const RSE::TextureLayeredType p_layer_type = RSE::TEXTURE_LAYERED_2D_ARRAY);
	static RID texture_get_rd_texture(RID p_texture, bool p_srgb = false);
	static uint64_t texture_get_native_handle(RID p_texture, bool p_srgb = false);

	/* SHADER API */

	static RID shader_create();
	static RID shader_create_from_code(const String& p_code, const String& p_path_hint = String());

	static void shader_set_code(RID p_shader, const String& p_code);
	static void shader_set_path_hint(RID p_shader, const String& p_path);
	static String shader_get_code(RID p_shader);

	static void shader_set_default_texture_parameter(
		RID p_shader, const String& p_name, RID p_texture, int p_index = 0);
	static RID shader_get_default_texture_parameter(
		RID p_shader, const String& p_name, int p_index = 0);

	static RenderingServerTypes::ShaderNativeSourceCode shader_get_native_source_code(RID p_shader);

	/* COMMON MATERIAL API */

	static RID material_create();
	static RID material_create_from_shader(RID p_next_pass, int p_render_priority, RID p_shader);

	static void material_set_shader(RID p_shader_material, RID p_shader);
	static void material_set_render_priority(RID p_material, int priority);
	static void material_set_next_pass(RID p_material, RID p_next_material);
	static void material_set_use_debanding(bool p_enable);

	/* MESH API */

	static RID mesh_create_from_surfaces(
		const Vector<RenderingServerTypes::SurfaceData>& p_surfaces, int p_blend_shape_count = 0);
	static RID mesh_create();

	static void mesh_set_blend_shape_count(RID p_mesh, int p_blend_shape_count);

	static uint32_t mesh_surface_get_format_offset(
		uint32_t p_format, int p_vertex_len, int p_array_index);
	static uint32_t mesh_surface_get_format_vertex_stride(uint32_t p_format, int p_vertex_len);
	static uint32_t mesh_surface_get_format_normal_tangent_stride(
		uint32_t p_format, int p_vertex_len);
	static uint32_t mesh_surface_get_format_attribute_stride(uint32_t p_format, int p_vertex_len);
	static uint32_t mesh_surface_get_format_skin_stride(uint32_t p_format, int p_vertex_len);
	static uint32_t mesh_surface_get_format_index_stride(uint32_t p_format, int p_vertex_len);

	static void mesh_surface_make_offsets_from_format(uint64_t p_format, int p_vertex_len,
		int p_index_len, uint32_t* r_offsets, uint32_t& r_vertex_element_size,
		uint32_t& r_normal_element_size, uint32_t& r_attrib_element_size,
		uint32_t& r_skin_element_size);

	static void mesh_add_surface(RID p_mesh, const RenderingServerTypes::SurfaceData& p_surface);

	static int mesh_get_blend_shape_count(RID p_mesh);

	static void mesh_set_blend_shape_mode(RID p_mesh, RSE::BlendShapeMode p_mode);
	static RSE::BlendShapeMode mesh_get_blend_shape_mode(RID p_mesh);

	static void mesh_surface_update_vertex_region(
		RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data);
	static void mesh_surface_update_attribute_region(
		RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data);
	static void mesh_surface_update_skin_region(
		RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data);
	static void mesh_surface_update_index_region(
		RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data);

	static void mesh_surface_set_material(RID p_mesh, int p_surface, RID p_material);
	static RID mesh_surface_get_material(RID p_mesh, int p_surface);

	static RenderingServerTypes::SurfaceData mesh_get_surface(RID p_mesh, int p_surface);

	static int mesh_get_surface_count(RID p_mesh);

	static void mesh_set_custom_aabb(RID p_mesh, const AABB& p_aabb);
	static AABB mesh_get_custom_aabb(RID p_mesh);

	static void mesh_set_path(RID p_mesh, const String& p_path);
	static String mesh_get_path(RID p_mesh);

	static void mesh_set_shadow_mesh(RID p_mesh, RID p_shadow_mesh);

	static void mesh_surface_remove(RID p_mesh, int p_surface);
	static void mesh_clear(RID p_mesh);

	static RID mesh_surface_get_vertex_buffer_rd_rid(RID p_mesh, int p_surface);
	static RID mesh_surface_get_attribute_buffer_rd_rid(RID p_mesh, int p_surface);
	static RID mesh_surface_get_skin_buffer_rd_rid(RID p_mesh, int p_surface);
	static RID mesh_surface_get_index_buffer_rd_rid(RID p_mesh, int p_surface);

	static void mesh_debug_usage(List<RenderingServerTypes::MeshInfo>* r_info);

	/* MULTIMESH API */

	static RID multimesh_create();

	static void multimesh_allocate_data(RID p_multimesh, int p_instances,
		RSE::MultimeshTransformFormat p_transform_format, bool p_use_colors = false,
		bool p_use_custom_data = false, bool p_use_indirect = false);
	static int multimesh_get_instance_count(RID p_multimesh);

	static void multimesh_set_mesh(RID p_multimesh, RID p_mesh);
	static void multimesh_instance_set_transform(
		RID p_multimesh, int p_index, const Transform3D& p_transform);
	static void multimesh_instance_set_transform_2d(
		RID p_multimesh, int p_index, const Transform2D& p_transform);
	static void multimesh_instance_set_color(RID p_multimesh, int p_index, const Color& p_color);
	static void multimesh_instance_set_custom_data(
		RID p_multimesh, int p_index, const Color& p_color);

	static RID multimesh_get_mesh(RID p_multimesh);
	static AABB multimesh_get_aabb(RID p_multimesh);

	static void multimesh_set_custom_aabb(RID p_mesh, const AABB& p_aabb);
	static AABB multimesh_get_custom_aabb(RID p_mesh);

	static Transform3D multimesh_instance_get_transform(RID p_multimesh, int p_index);
	static Transform2D multimesh_instance_get_transform_2d(RID p_multimesh, int p_index);
	static Color multimesh_instance_get_color(RID p_multimesh, int p_index);
	static Color multimesh_instance_get_custom_data(RID p_multimesh, int p_index);

	static void multimesh_set_buffer(RID p_multimesh, const Vector<float>& p_buffer);
	static RID multimesh_get_command_buffer_rd_rid(RID p_multimesh);
	static RID multimesh_get_buffer_rd_rid(RID p_multimesh);
	static Vector<float> multimesh_get_buffer(RID p_multimesh);

	static void multimesh_set_buffer_interpolated(
		RID p_multimesh, const Vector<float>& p_buffer_curr, const Vector<float>& p_buffer_prev);
	static void multimesh_set_physics_interpolated(RID p_multimesh, bool p_interpolated);
	static void multimesh_set_physics_interpolation_quality(
		RID p_multimesh, RSE::MultimeshPhysicsInterpolationQuality p_quality);
	static void multimesh_instance_reset_physics_interpolation(RID p_multimesh, int p_index);
	static void multimesh_instances_reset_physics_interpolation(RID p_multimesh);

	static void multimesh_set_visible_instances(RID p_multimesh, int p_visible);
	static int multimesh_get_visible_instances(RID p_multimesh);

	/* SKELETON API */

	static RID skeleton_create();
	static void skeleton_allocate_data(RID p_skeleton, int p_bones, bool p_2d_skeleton = false);
	static int skeleton_get_bone_count(RID p_skeleton);
	static void skeleton_bone_set_transform(
		RID p_skeleton, int p_bone, const Transform3D& p_transform);
	static Transform3D skeleton_bone_get_transform(RID p_skeleton, int p_bone);
	static void skeleton_bone_set_transform_2d(
		RID p_skeleton, int p_bone, const Transform2D& p_transform);
	static Transform2D skeleton_bone_get_transform_2d(RID p_skeleton, int p_bone);
	static void skeleton_set_base_transform_2d(RID p_skeleton, const Transform2D& p_base_transform);

	/* LIGHT API */

	static RID directional_light_create();
	static RID omni_light_create();
	static RID spot_light_create();
	static RID area_light_create();

	static void light_set_color(RID p_light, const Color& p_color);
	static void light_set_param(RID p_light, RSE::LightParam p_param, float p_value);
	static void light_set_shadow(RID p_light, bool p_enabled);
	static void light_set_projector(RID p_light, RID p_texture);
	static void light_set_negative(RID p_light, bool p_enable);
	static void light_set_cull_mask(RID p_light, uint32_t p_mask);
	static void light_set_distance_fade(
		RID p_light, bool p_enabled, float p_begin, float p_shadow, float p_length);
	static void light_set_reverse_cull_face_mode(RID p_light, bool p_enabled);
	static void light_set_shadow_caster_mask(RID p_light, uint32_t p_caster_mask);

	static void light_set_bake_mode(RID p_light, RSE::LightBakeMode p_bake_mode);
	static void light_set_max_sdfgi_cascade(RID p_light, uint32_t p_cascade);

	static void light_omni_set_shadow_mode(RID p_light, RSE::LightOmniShadowMode p_mode);

	static void light_directional_set_shadow_mode(
		RID p_light, RSE::LightDirectionalShadowMode p_mode);
	static void light_directional_set_blend_splits(RID p_light, bool p_enable);
	static void light_directional_set_sky_mode(RID p_light, RSE::LightDirectionalSkyMode p_mode);

	static void light_area_set_size(RID p_light, const Vector2& p_size);
	static void light_area_set_normalize_energy(RID p_light, bool p_enabled);
	static void light_area_set_texture(RID p_light, RID p_texture);

	static RID shadow_atlas_create();
	static void shadow_atlas_set_size(RID p_atlas, int p_size, bool p_use_16_bits = true);
	static void shadow_atlas_set_quadrant_subdivision(
		RID p_atlas, int p_quadrant, int p_subdivision);

	static void directional_shadow_atlas_set_size(int p_size, bool p_16_bits = true);

	static void positional_soft_shadow_filter_set_quality(RSE::ShadowQuality p_quality);
	static void directional_soft_shadow_filter_set_quality(RSE::ShadowQuality p_quality);

	static void light_projectors_set_filter(RSE::LightProjectorFilter p_filter);

	/* REFLECTION PROBE API */

	static RID reflection_probe_create();

	static void reflection_probe_set_update_mode(
		RID p_probe, RSE::ReflectionProbeUpdateMode p_mode);
	static void reflection_probe_set_intensity(RID p_probe, float p_intensity);
	static void reflection_probe_set_blend_distance(RID p_probe, float p_blend_distance);

	static void reflection_probe_set_ambient_mode(
		RID p_probe, RSE::ReflectionProbeAmbientMode p_mode);
	static void reflection_probe_set_ambient_color(RID p_probe, const Color& p_color);
	static void reflection_probe_set_ambient_energy(RID p_probe, float p_energy);
	static void reflection_probe_set_max_distance(RID p_probe, float p_distance);
	static void reflection_probe_set_size(RID p_probe, const Vector3& p_size);
	static void reflection_probe_set_origin_offset(RID p_probe, const Vector3& p_offset);
	static void reflection_probe_set_as_interior(RID p_probe, bool p_enable);
	static void reflection_probe_set_enable_box_projection(RID p_probe, bool p_enable);
	static void reflection_probe_set_enable_shadows(RID p_probe, bool p_enable);
	static void reflection_probe_set_cull_mask(RID p_probe, uint32_t p_layers);
	static void reflection_probe_set_reflection_mask(RID p_probe, uint32_t p_layers);
	static void reflection_probe_set_resolution(RID p_probe, int p_resolution);
	static void reflection_probe_set_mesh_lod_threshold(RID p_probe, float p_pixels);

	/* DECAL API */

	static RID decal_create();
	static void decal_set_size(RID p_decal, const Vector3& p_size);
	static void decal_set_texture(RID p_decal, RSE::DecalTexture p_type, RID p_texture);
	static void decal_set_emission_energy(RID p_decal, float p_energy);
	static void decal_set_albedo_mix(RID p_decal, float p_mix);
	static void decal_set_modulate(RID p_decal, const Color& p_modulate);
	static void decal_set_cull_mask(RID p_decal, uint32_t p_layers);
	static void decal_set_distance_fade(RID p_decal, bool p_enabled, float p_begin, float p_length);
	static void decal_set_fade(RID p_decal, float p_above, float p_below);
	static void decal_set_normal_fade(RID p_decal, float p_fade);

	static void decals_set_filter(RSE::DecalFilter p_quality);

	/* VOXEL GI API */

	static RID voxel_gi_create();

	static void voxel_gi_allocate_data(RID p_voxel_gi, const Transform3D& p_to_cell_xform,
		const AABB& p_aabb, const Vector3i& p_octree_size, const Vector<uint8_t>& p_octree_cells,
		const Vector<uint8_t>& p_data_cells, const Vector<uint8_t>& p_distance_field,
		const Vector<int>& p_level_counts);

	static AABB voxel_gi_get_bounds(RID p_voxel_gi);
	static Vector3i voxel_gi_get_octree_size(RID p_voxel_gi);
	static Vector<uint8_t> voxel_gi_get_octree_cells(RID p_voxel_gi);
	static Vector<uint8_t> voxel_gi_get_data_cells(RID p_voxel_gi);
	static Vector<uint8_t> voxel_gi_get_distance_field(RID p_voxel_gi);
	static Vector<int> voxel_gi_get_level_counts(RID p_voxel_gi);
	static Transform3D voxel_gi_get_to_cell_xform(RID p_voxel_gi);

	static void voxel_gi_set_dynamic_range(RID p_voxel_gi, float p_range);
	static void voxel_gi_set_propagation(RID p_voxel_gi, float p_range);
	static void voxel_gi_set_energy(RID p_voxel_gi, float p_energy);
	static void voxel_gi_set_baked_exposure_normalization(RID p_voxel_gi, float p_baked_exposure);
	static void voxel_gi_set_bias(RID p_voxel_gi, float p_bias);
	static void voxel_gi_set_normal_bias(RID p_voxel_gi, float p_range);
	static void voxel_gi_set_interior(RID p_voxel_gi, bool p_enable);
	static void voxel_gi_set_use_two_bounces(RID p_voxel_gi, bool p_enable);

	static void voxel_gi_set_quality(RSE::VoxelGIQuality p_quality);

	static void sdfgi_reset();

	/* LIGHTMAP API */

	static RID lightmap_create();

	static void lightmap_set_textures(RID p_lightmap, RID p_light, bool p_uses_spherical_haromics);
	static void lightmap_set_probe_bounds(RID p_lightmap, const AABB& p_bounds);
	static void lightmap_set_probe_interior(RID p_lightmap, bool p_interior);
	static void lightmap_set_probe_capture_data(RID p_lightmap, const PackedVector3Array& p_points,
		const PackedColorArray& p_point_sh, const PackedInt32Array& p_tetrahedra,
		const PackedInt32Array& p_bsp_tree);
	static void lightmap_set_baked_exposure_normalization(RID p_lightmap, float p_exposure);
	static PackedVector3Array lightmap_get_probe_capture_points(RID p_lightmap);
	static PackedColorArray lightmap_get_probe_capture_sh(RID p_lightmap);
	static PackedInt32Array lightmap_get_probe_capture_tetrahedra(RID p_lightmap);
	static PackedInt32Array lightmap_get_probe_capture_bsp_tree(RID p_lightmap);

	static void lightmap_set_probe_capture_update_speed(float p_speed);
	static void lightmaps_set_bicubic_filter(bool p_enable);

	static void lightmap_set_shadowmask_textures(RID p_lightmap, RID p_shadow);
	static RSE::ShadowmaskMode lightmap_get_shadowmask_mode(RID p_lightmap);
	static void lightmap_set_shadowmask_mode(RID p_lightmap, RSE::ShadowmaskMode p_mode);

	/* PARTICLES API */

	static RID particles_create();
	static void particles_set_mode(RID p_particles, RSE::ParticlesMode p_mode);

	static void particles_set_emitting(RID p_particles, bool p_enable);
	static bool particles_get_emitting(RID p_particles);
	static void particles_set_amount(RID p_particles, int p_amount);
	static void particles_set_amount_ratio(RID p_particles, float p_amount_ratio);
	static void particles_set_lifetime(RID p_particles, double p_lifetime);
	static void particles_set_one_shot(RID p_particles, bool p_one_shot);
	static void particles_set_pre_process_time(RID p_particles, double p_time);
	static void particles_request_process_time(RID p_particles, real_t p_request_process_time,
		real_t p_request_process_time_residual = 0.0);
	static void particles_set_explosiveness_ratio(RID p_particles, float p_ratio);
	static void particles_set_randomness_ratio(RID p_particles, float p_ratio);
	static void particles_set_custom_aabb(RID p_particles, const AABB& p_aabb);
	static void particles_set_speed_scale(RID p_particles, double p_scale);
	static void particles_set_use_local_coordinates(RID p_particles, bool p_enable);
	static void particles_set_process_material(RID p_particles, RID p_material);
	static void particles_set_fixed_fps(RID p_particles, int p_fps);
	static void particles_set_interpolate(RID p_particles, bool p_enable);
	static void particles_set_fractional_delta(RID p_particles, bool p_enable);
	static void particles_set_collision_base_size(RID p_particles, float p_size);
	static void particles_set_seed(RID p_particles, uint32_t p_seed);

	static void particles_set_transform_align(
		RID p_particles, RSE::ParticlesTransformAlign p_transform_align);
	static void particles_set_transform_align_channel_filter(
		RID p_particles, RSE::ParticlesTransformAlignCustomSrc p_transform_align_channel_filter);
	static void particles_set_transform_align_axis(
		RID p_particles, RSE::ParticlesTransformAlignAxis p_rotation_axis);

	static void particles_set_trails(RID p_particles, bool p_enable, float p_length_sec);
	static void particles_set_trail_bind_poses(
		RID p_particles, const Vector<Transform3D>& p_bind_poses);

	static bool particles_is_inactive(RID p_particles);
	static void particles_request_process(RID p_particles);
	static void particles_restart(RID p_particles);

	static void particles_set_subemitter(RID p_particles, RID p_subemitter_particles);

	static void particles_emit(RID p_particles, const Transform3D& p_transform,
		const Vector3& p_velocity, const Color& p_color, const Color& p_custom,
		uint32_t p_emit_flags);

	static void particles_set_draw_order(RID p_particles, RSE::ParticlesDrawOrder p_order);

	static void particles_set_draw_passes(RID p_particles, int p_count);
	static void particles_set_draw_pass_mesh(RID p_particles, int p_pass, RID p_mesh);

	static AABB particles_get_current_aabb(RID p_particles);

	static void particles_set_emission_transform(RID p_particles, const Transform3D& p_transform);
	static void particles_set_emitter_velocity(RID p_particles, const Vector3& p_velocity);
	static void particles_set_interp_to_end(RID p_particles, float p_interp);

	/* PARTICLES COLLISION API */

	static RID particles_collision_create();

	static void particles_collision_set_collision_type(
		RID p_particles_collision, RSE::ParticlesCollisionType p_type);
	static void particles_collision_set_cull_mask(RID p_particles_collision, uint32_t p_cull_mask);
	static void particles_collision_set_sphere_radius(RID p_particles_collision, real_t p_radius);
	static void particles_collision_set_box_extents(
		RID p_particles_collision, const Vector3& p_extents);
	static void particles_collision_set_attractor_strength(
		RID p_particles_collision, real_t p_strength);
	static void particles_collision_set_attractor_directionality(
		RID p_particles_collision, real_t p_directionality);
	static void particles_collision_set_attractor_attenuation(
		RID p_particles_collision, real_t p_curve);
	static void particles_collision_set_field_texture(RID p_particles_collision, RID p_texture);

	static void particles_collision_height_field_update(RID p_particles_collision);

	static void particles_collision_set_height_field_resolution(
		RID p_particles_collision, RSE::ParticlesCollisionHeightfieldResolution p_resolution);
	static void particles_collision_set_height_field_mask(
		RID p_particles_collision, uint32_t p_heightfield_mask);

	/* FOG VOLUME API */

	static RID fog_volume_create();

	static void fog_volume_set_shape(RID p_fog_volume, RSE::FogVolumeShape p_shape);
	static void fog_volume_set_size(RID p_fog_volume, const Vector3& p_size);
	static void fog_volume_set_material(RID p_fog_volume, RID p_material);

	/* VISIBILITY NOTIFIER API */

	static RID visibility_notifier_create();
	static void visibility_notifier_set_aabb(RID p_notifier, const AABB& p_aabb);

	/* OCCLUDER API */

	static RID occluder_create();
	static void occluder_set_mesh(
		RID p_occluder, const PackedVector3Array& p_vertices, const PackedInt32Array& p_indices);

	/* CAMERA API */

	static RID camera_create();
	static void camera_set_perspective(
		RID p_camera, float p_fovy_degrees, float p_z_near, float p_z_far);
	static void camera_set_orthogonal(RID p_camera, float p_size, float p_z_near, float p_z_far);
	static void camera_set_frustum(
		RID p_camera, float p_size, Vector2 p_offset, float p_z_near, float p_z_far);
	static void camera_set_transform(RID p_camera, const Transform3D& p_transform);
	static void camera_set_cull_mask(RID p_camera, uint32_t p_layers);
	static void camera_set_environment(RID p_camera, RID p_env);
	static void camera_set_camera_attributes(RID p_camera, RID p_camera_attributes);
	static void camera_set_compositor(RID p_camera, RID p_compositor);
	static void camera_set_use_vertical_aspect(RID p_camera, bool p_enable);

	/* VIEWPORT API */

	static RID viewport_create();

#ifndef XR_DISABLED
	static void viewport_set_use_xr(RID p_viewport, bool p_use_xr);
#endif // !XR_DISABLED

	static void viewport_set_size(RID p_viewport, int p_width, int p_height, int p_view_count = 1);
	static void viewport_set_active(RID p_viewport, bool p_active);
	static void viewport_set_parent_viewport(RID p_viewport, RID p_parent_viewport);
	static void viewport_set_canvas_cull_mask(RID p_viewport, uint32_t p_canvas_cull_mask);

	static void viewport_attach_to_screen(RID p_viewport, const Rect2& p_rect = Rect2(),
		DisplayServerEnums::WindowID p_screen = DisplayServerEnums::MAIN_WINDOW_ID);
	static void viewport_set_render_direct_to_screen(RID p_viewport, bool p_enable);

	static void viewport_set_scaling_3d_mode(
		RID p_viewport, RSE::ViewportScaling3DMode p_scaling_3d_mode);
	static void viewport_set_scaling_3d_scale(RID p_viewport, float p_scaling_3d_scale);
	static void viewport_set_fsr_sharpness(RID p_viewport, float p_fsr_sharpness);
	static void viewport_set_texture_mipmap_bias(RID p_viewport, float p_texture_mipmap_bias);
	static void viewport_set_anisotropic_filtering_level(
		RID p_viewport, RSE::ViewportAnisotropicFiltering p_anisotropic_filtering_level);

	static void viewport_set_update_mode(RID p_viewport, RSE::ViewportUpdateMode p_mode);
	static RSE::ViewportUpdateMode viewport_get_update_mode(RID p_viewport);

	static void viewport_set_clear_mode(RID p_viewport, RSE::ViewportClearMode p_clear_mode);

	static RID viewport_get_render_target(RID p_viewport);
	static RID viewport_get_texture(RID p_viewport);

	static void viewport_set_environment_mode(RID p_viewport, RSE::ViewportEnvironmentMode p_mode);
	static void viewport_set_disable_3d(RID p_viewport, bool p_disable);
	static void viewport_set_disable_2d(RID p_viewport, bool p_disable);

	static void viewport_attach_camera(RID p_viewport, RID p_camera);
	static void viewport_set_scenario(RID p_viewport, RID p_scenario);
	static void viewport_attach_canvas(RID p_viewport, RID p_canvas);
	static void viewport_remove_canvas(RID p_viewport, RID p_canvas);
	static void viewport_set_canvas_transform(
		RID p_viewport, RID p_canvas, const Transform2D& p_offset);
	static void viewport_set_transparent_background(RID p_viewport, bool p_enabled);
	static void viewport_set_use_hdr_2d(RID p_viewport, bool p_use_hdr);
	static bool viewport_is_using_hdr_2d(RID p_viewport);
	static void viewport_set_snap_2d_transforms_to_pixel(RID p_viewport, bool p_enabled);
	static void viewport_set_snap_2d_vertices_to_pixel(RID p_viewport, bool p_enabled);

	static void viewport_set_default_canvas_item_texture_filter(
		RID p_viewport, RSE::CanvasItemTextureFilter p_filter);
	static void viewport_set_default_canvas_item_texture_repeat(
		RID p_viewport, RSE::CanvasItemTextureRepeat p_repeat);

	static void viewport_set_global_canvas_transform(
		RID p_viewport, const Transform2D& p_transform);
	static void viewport_set_canvas_stacking(
		RID p_viewport, RID p_canvas, int p_layer, int p_sublayer);

	static void viewport_set_sdf_oversize_and_scale(
		RID p_viewport, RSE::ViewportSDFOversize p_oversize, RSE::ViewportSDFScale p_scale);

	static void viewport_set_positional_shadow_atlas_size(
		RID p_viewport, int p_size, bool p_16_bits = true);
	static void viewport_set_positional_shadow_atlas_quadrant_subdivision(
		RID p_viewport, int p_quadrant, int p_subdiv);

	static void viewport_set_msaa_3d(RID p_viewport, RSE::ViewportMSAA p_msaa);
	static void viewport_set_msaa_2d(RID p_viewport, RSE::ViewportMSAA p_msaa);

	static void viewport_set_screen_space_aa(RID p_viewport, RSE::ViewportScreenSpaceAA p_mode);

	static void viewport_set_use_taa(RID p_viewport, bool p_use_taa);
	static void viewport_set_use_debanding(RID p_viewport, bool p_use_debanding);
	static void viewport_set_force_motion_vectors(RID p_viewport, bool p_force_motion_vectors);

	static void viewport_set_mesh_lod_threshold(RID p_viewport, float p_pixels);

	static void viewport_set_use_occlusion_culling(RID p_viewport, bool p_use_occlusion_culling);
	static void viewport_set_occlusion_rays_per_thread(int p_rays_per_thread);

	static void viewport_set_occlusion_culling_build_quality(
		RSE::ViewportOcclusionCullingBuildQuality p_quality);

	static int viewport_get_render_info(
		RID p_viewport, RSE::ViewportRenderInfoType p_type, RSE::ViewportRenderInfo p_info);

	static void viewport_set_debug_draw(RID p_viewport, RSE::ViewportDebugDraw p_draw);

	static void viewport_set_measure_render_time(RID p_viewport, bool p_enable);
	static double viewport_get_measured_render_time_cpu(RID p_viewport);
	static double viewport_get_measured_render_time_gpu(RID p_viewport);

	static RID viewport_find_from_screen_attachment(
		DisplayServerEnums::WindowID p_id = DisplayServerEnums::MAIN_WINDOW_ID);

	static void viewport_set_vrs_mode(RID p_viewport, RSE::ViewportVRSMode p_mode);
	static void viewport_set_vrs_update_mode(RID p_viewport, RSE::ViewportVRSUpdateMode p_mode);
	static void viewport_set_vrs_texture(RID p_viewport, RID p_texture);

	/* SKY API */

	static RID sky_create();
	static void sky_set_radiance_size(RID p_sky, int p_radiance_size);
	static void sky_set_mode(RID p_sky, RSE::SkyMode p_mode);
	static void sky_set_material(RID p_sky, RID p_material);
	static Ref<Image> sky_bake_panorama(
		RID p_sky, float p_energy, bool p_bake_irradiance, const Size2i& p_size);

	/* COMPOSITOR EFFECTS API */

	static RID compositor_effect_create();
	static void compositor_effect_set_enabled(RID p_effect, bool p_enabled);
	static void compositor_effect_set_flag(
		RID p_effect, RSE::CompositorEffectFlags p_flag, bool p_set);

	/* COMPOSITOR API */

	static RID compositor_create();

	/* ENVIRONMENT API */

	static RID environment_create();

	static void environment_set_background(RID p_env, RSE::EnvironmentBG p_bg);
	static void environment_set_sky(RID p_env, RID p_sky);
	static void environment_set_sky_custom_fov(RID p_env, float p_scale);
	static void environment_set_sky_orientation(RID p_env, const Basis& p_orientation);
	static void environment_set_bg_color(RID p_env, const Color& p_color);
	static void environment_set_bg_energy(RID p_env, float p_multiplier, float p_exposure_value);
	static void environment_set_canvas_max_layer(RID p_env, int p_max_layer);
	static void environment_set_ambient_light(RID p_env, const Color& p_color,
		RSE::EnvironmentAmbientSource p_ambient = RSE::ENV_AMBIENT_SOURCE_BG, float p_energy = 1.0,
		float p_sky_contribution = 0.0,
		RSE::EnvironmentReflectionSource p_reflection_source = RSE::ENV_REFLECTION_SOURCE_BG);
	static void environment_set_camera_feed_id(RID p_env, int p_camera_feed_id);

	static void environment_set_glow(RID p_env, bool p_enable, Vector<float> p_levels,
		float p_intensity, float p_strength, float p_mix, float p_bloom_threshold,
		RSE::EnvironmentGlowBlendMode p_blend_mode, float p_hdr_bleed_threshold,
		float p_hdr_bleed_scale, float p_hdr_luminance_cap, float p_glow_map_strength,
		RID p_glow_map);

	static void environment_glow_set_use_bicubic_upscale(bool p_enable);

	static void environment_set_tonemap(
		RID p_env, RSE::EnvironmentToneMapper p_tone_mapper, float p_exposure, float p_white);
	static void environment_set_tonemap_agx_contrast(RID p_env, float p_agx_contrast);
	static void environment_set_adjustment(RID p_env, bool p_enable, float p_brightness,
		float p_contrast, float p_saturation, bool p_use_1d_color_correction,
		RID p_color_correction);

	static void environment_set_ssr(RID p_env, bool p_enable, int p_max_steps, float p_fade_in,
		float p_fade_out, float p_depth_tolerance);

	static void environment_set_ssr_half_size(bool p_half_size);

	static void environment_set_ssr_roughness_quality(
		RSE::EnvironmentSSRRoughnessQuality p_quality);

	static void environment_set_ssao(RID p_env, bool p_enable, float p_radius, float p_intensity,
		float p_power, float p_detail, float p_horizon, float p_sharpness, float p_light_affect,
		float p_ao_channel_affect);

	static void environment_set_ssao_quality(RSE::EnvironmentSSAOQuality p_quality,
		bool p_half_size, float p_adaptive_target, int p_blur_passes, float p_fadeout_from,
		float p_fadeout_to);

	static void environment_set_ssil(RID p_env, bool p_enable, float p_radius, float p_intensity,
		float p_sharpness, float p_normal_rejection);

	static void environment_set_ssil_quality(RSE::EnvironmentSSILQuality p_quality,
		bool p_half_size, float p_adaptive_target, int p_blur_passes, float p_fadeout_from,
		float p_fadeout_to);

	static void environment_set_sdfgi(RID p_env, bool p_enable, int p_cascades,
		float p_min_cell_size, RSE::EnvironmentSDFGIYScale p_y_scale, bool p_use_occlusion,
		float p_bounce_feedback, bool p_read_sky, float p_energy, float p_normal_bias,
		float p_probe_bias);

	static void environment_set_sdfgi_ray_count(RSE::EnvironmentSDFGIRayCount p_ray_count);

	static void environment_set_sdfgi_frames_to_converge(
		RSE::EnvironmentSDFGIFramesToConverge p_frames);

	static void environment_set_sdfgi_frames_to_update_light(
		RSE::EnvironmentSDFGIFramesToUpdateLight p_update);

	static void environment_set_fog(RID p_env, bool p_enable, const Color& p_light_color,
		float p_light_energy, float p_sun_scatter, float p_density, float p_height,
		float p_height_density, float p_aerial_perspective, float p_sky_affect,
		RSE::EnvironmentFogMode p_mode = RSE::EnvironmentFogMode::ENV_FOG_MODE_EXPONENTIAL);
	static void environment_set_fog_depth(RID p_env, float p_curve, float p_begin, float p_end);

	static void environment_set_volumetric_fog(RID p_env, bool p_enable, float p_density,
		const Color& p_albedo, const Color& p_emission, float p_emission_energy, float p_anisotropy,
		float p_length, float p_detail_spread, float p_gi_inject, bool p_temporal_reprojection,
		float p_temporal_reprojection_amount, float p_ambient_inject, float p_sky_affect);
	static void environment_set_volumetric_fog_volume_size(int p_size, int p_depth);
	static void environment_set_volumetric_fog_filter_active(bool p_enable);

	static Ref<Image> environment_bake_panorama(
		RID p_env, bool p_bake_irradiance, const Size2i& p_size);

	static void screen_space_roughness_limiter_set_active(
		bool p_enable, float p_amount, float p_limit);

	static void sub_surface_scattering_set_quality(RSE::SubSurfaceScatteringQuality p_quality);
	static void sub_surface_scattering_set_scale(float p_scale, float p_depth_scale);

	/* CAMERA ATTRIBUTES API */

	static RID camera_attributes_create();

	static void camera_attributes_set_dof_blur_quality(
		RSE::DOFBlurQuality p_quality, bool p_use_jitter);

	static void camera_attributes_set_dof_blur_bokeh_shape(RSE::DOFBokehShape p_shape);

	static void camera_attributes_set_dof_blur(RID p_camera_attributes, bool p_far_enable,
		float p_far_distance, float p_far_transition, bool p_near_enable, float p_near_distance,
		float p_near_transition, float p_amount);
	static void camera_attributes_set_exposure(
		RID p_camera_attributes, float p_multiplier, float p_exposure_normalization);
	static void camera_attributes_set_auto_exposure(RID p_camera_attributes, bool p_enable,
		float p_min_sensitivity, float p_max_sensitivity, float p_speed, float p_scale);

	/* SCENARIO API */

	static RID scenario_create();

	static void scenario_set_environment(RID p_scenario, RID p_environment);
	static void scenario_set_fallback_environment(RID p_scenario, RID p_environment);
	static void scenario_set_camera_attributes(RID p_scenario, RID p_camera_attributes);
	static void scenario_set_compositor(RID p_scenario, RID p_compositor);

	/* INSTANCING API */

	static RID instance_create2(RID p_base, RID p_scenario);
	static RID instance_create();

	static void instance_set_base(RID p_instance, RID p_base);
	static void instance_set_scenario(RID p_instance, RID p_scenario);
	static void instance_set_layer_mask(RID p_instance, uint32_t p_mask);
	static void instance_set_pivot_data(
		RID p_instance, float p_sorting_offset, bool p_use_aabb_center);
	static void instance_set_transform(RID p_instance, const Transform3D& p_transform);
	static void instance_set_blend_shape_weight(RID p_instance, int p_shape, float p_weight);
	static void instance_set_surface_override_material(
		RID p_instance, int p_surface, RID p_material);
	static void instance_set_visible(RID p_instance, bool p_visible);

	static void instance_teleport(RID p_instance);
	static void instance_set_custom_aabb(RID p_instance, AABB p_aabb);
	static void instance_attach_skeleton(RID p_instance, RID p_skeleton);
	static void instance_set_extra_visibility_margin(RID p_instance, real_t p_margin);
	static void instance_set_visibility_parent(RID p_instance, RID p_parent_instance);
	static void instance_set_ignore_culling(RID p_instance, bool p_enabled);

	static PackedInt64Array _instances_cull_aabb_bind(const AABB& p_aabb, RID p_scenario = RID());
	static PackedInt64Array _instances_cull_ray_bind(
		const Vector3& p_from, const Vector3& p_to, RID p_scenario = RID());

	static void instance_geometry_set_flag(
		RID p_instance, RSE::InstanceFlags p_flags, bool p_enabled);
	static void instance_geometry_set_cast_shadows_setting(
		RID p_instance, RSE::ShadowCastingSetting p_shadow_casting_setting);
	static void instance_geometry_set_material_override(RID p_instance, RID p_material);
	static void instance_geometry_set_material_overlay(RID p_instance, RID p_material);
	static void instance_geometry_set_visibility_range(RID p_instance, float p_min, float p_max,
		float p_min_margin, float p_max_margin, RSE::VisibilityRangeFadeMode p_fade_mode);
	static void instance_geometry_set_lightmap(
		RID p_instance, RID p_lightmap, const Rect2& p_lightmap_uv_scale, int p_lightmap_slice);
	static void instance_geometry_set_lod_bias(RID p_instance, float p_lod_bias);
	static void instance_geometry_set_transparency(RID p_instance, float p_transparency);

	/* CANVAS API (2D) */

	static RID canvas_create();
	static void canvas_set_item_mirroring(RID p_canvas, RID p_item, const Point2& p_mirroring);
	static void canvas_set_item_repeat(RID p_item, const Point2& p_repeat_size, int p_repeat_times);
	static void canvas_set_modulate(RID p_canvas, const Color& p_color);
	static void canvas_set_parent(RID p_canvas, RID p_parent, float p_scale);
	static void canvas_set_disable_scale(bool p_disable);

	/* CANVAS TEXTURE API */

	static RID canvas_texture_create();
	static void canvas_texture_set_channel(
		RID p_canvas_texture, RSE::CanvasTextureChannel p_channel, RID p_texture);
	static void canvas_texture_set_shading_parameters(
		RID p_canvas_texture, const Color& p_base_color, float p_shininess);

	static void canvas_texture_set_texture_filter(
		RID p_canvas_texture, RSE::CanvasItemTextureFilter p_filter);
	static void canvas_texture_set_texture_repeat(
		RID p_canvas_texture, RSE::CanvasItemTextureRepeat p_repeat);

	/* CANVAS ITEM API */

	static RID canvas_item_create();
	static void canvas_item_set_parent(RID p_item, RID p_parent);

	static void canvas_item_set_default_texture_filter(
		RID p_item, RSE::CanvasItemTextureFilter p_filter);
	static void canvas_item_set_default_texture_repeat(
		RID p_item, RSE::CanvasItemTextureRepeat p_repeat);

	static void canvas_item_set_visible(RID p_item, bool p_visible);
	static void canvas_item_set_light_mask(RID p_item, int p_mask);

	static void canvas_item_set_update_when_visible(RID p_item, bool p_update);

	static void canvas_item_set_transform(RID p_item, const Transform2D& p_transform);
	static void canvas_item_set_clip(RID p_item, bool p_clip);
	static void canvas_item_set_distance_field_mode(RID p_item, bool p_enable);
	static void canvas_item_set_custom_rect(
		RID p_item, bool p_custom_rect, const Rect2& p_rect = Rect2());
	static void canvas_item_set_modulate(RID p_item, const Color& p_color);
	static void canvas_item_set_self_modulate(RID p_item, const Color& p_color);
	static void canvas_item_set_visibility_layer(RID p_item, uint32_t p_visibility_layer);

	static void canvas_item_set_draw_behind_parent(RID p_item, bool p_enable);
	static void canvas_item_set_use_identity_transform(RID p_item, bool p_enabled);

	static void canvas_item_add_line(RID p_item, const Point2& p_from, const Point2& p_to,
		const Color& p_color, float p_width = -1.0, bool p_antialiased = false);
	static void canvas_item_add_polyline(RID p_item, const Vector<Point2>& p_points,
		const Vector<Color>& p_colors, float p_width = -1.0, bool p_antialiased = false);
	static void canvas_item_add_multiline(RID p_item, const Vector<Point2>& p_points,
		const Vector<Color>& p_colors, float p_width = -1.0, bool p_antialiased = false);
	static void canvas_item_add_rect(
		RID p_item, const Rect2& p_rect, const Color& p_color, bool p_antialiased = false);
	static void canvas_item_add_ellipse(RID p_item, const Point2& p_pos, float p_major,
		float p_minor, const Color& p_color, bool p_antialiased = false);
	static void canvas_item_add_circle(RID p_item, const Point2& p_pos, float p_radius,
		const Color& p_color, bool p_antialiased = false);
	static void canvas_item_add_texture_rect(RID p_item, const Rect2& p_rect, RID p_texture,
		bool p_tile = false, const Color& p_modulate = Color(1, 1, 1), bool p_transpose = false);
	static void canvas_item_add_texture_rect_region(RID p_item, const Rect2& p_rect, RID p_texture,
		const Rect2& p_src_rect, const Color& p_modulate = Color(1, 1, 1), bool p_transpose = false,
		bool p_clip_uv = false);
	static void canvas_item_add_msdf_texture_rect_region(RID p_item, const Rect2& p_rect,
		RID p_texture, const Rect2& p_src_rect, const Color& p_modulate = Color(1, 1, 1),
		int p_outline_size = 0, float p_px_range = 1.0, float p_scale = 1.0);
	static void canvas_item_add_lcd_texture_rect_region(RID p_item, const Rect2& p_rect,
		RID p_texture, const Rect2& p_src_rect, const Color& p_modulate = Color(1, 1, 1));
	static void canvas_item_add_nine_patch(RID p_item, const Rect2& p_rect, const Rect2& p_source,
		RID p_texture, const Vector2& p_topleft, const Vector2& p_bottomright,
		RSE::NinePatchAxisMode p_x_axis_mode = RSE::NINE_PATCH_STRETCH,
		RSE::NinePatchAxisMode p_y_axis_mode = RSE::NINE_PATCH_STRETCH, bool p_draw_center = true,
		const Color& p_modulate = Color(1, 1, 1));
	static void canvas_item_add_primitive(RID p_item, const Vector<Point2>& p_points,
		const Vector<Color>& p_colors, const Vector<Point2>& p_uvs, RID p_texture);
	static void canvas_item_add_polygon(RID p_item, const Vector<Point2>& p_points,
		const Vector<Color>& p_colors, const Vector<Point2>& p_uvs = Vector<Point2>(),
		RID p_texture = RID());
	static void canvas_item_add_triangle_array(RID p_item, const Vector<int>& p_indices,
		const Vector<Point2>& p_points, const Vector<Color>& p_colors,
		const Vector<Point2>& p_uvs = Vector<Point2>(), const Vector<int>& p_bones = Vector<int>(),
		const Vector<float>& p_weights = Vector<float>(), RID p_texture = RID(), int p_count = -1);
	static void canvas_item_add_mesh(RID p_item, const RID& p_mesh,
		const Transform2D& p_transform = Transform2D(), const Color& p_modulate = Color(1, 1, 1),
		RID p_texture = RID());
	static void canvas_item_add_multimesh(RID p_item, RID p_mesh, RID p_texture = RID());
	static void canvas_item_add_particles(RID p_item, RID p_particles, RID p_texture);
	static void canvas_item_add_set_transform(RID p_item, const Transform2D& p_transform);
	static void canvas_item_add_clip_ignore(RID p_item, bool p_ignore);
	static void canvas_item_add_animation_slice(RID p_item, double p_animation_length,
		double p_slice_begin, double p_slice_end, double p_offset);

	static void canvas_item_set_sort_children_by_y(RID p_item, bool p_enable);
	static void canvas_item_set_z_index(RID p_item, int p_z);
	static void canvas_item_set_z_as_relative_to_parent(RID p_item, bool p_enable);
	static void canvas_item_set_copy_to_backbuffer(RID p_item, bool p_enable, const Rect2& p_rect);

	static void canvas_item_attach_skeleton(RID p_item, RID p_skeleton);

	static void canvas_item_clear(RID p_item);
	static void canvas_item_set_draw_index(RID p_item, int p_index);

	static void canvas_item_set_material(RID p_item, RID p_material);
	static void canvas_item_set_use_parent_material(RID p_item, bool p_enable);

	static void canvas_item_set_canvas_group_mode(RID p_item, RSE::CanvasGroupMode p_mode,
		float p_clear_margin = 5.0, bool p_fit_empty = false, float p_fit_margin = 0.0,
		bool p_blur_mipmaps = false);

	static void canvas_item_set_debug_redraw(bool p_enabled);
	static bool canvas_item_get_debug_redraw();

	static void canvas_item_set_interpolated(RID p_item, bool p_interpolated);
	static void canvas_item_reset_physics_interpolation(RID p_item);
	static void canvas_item_transform_physics_interpolation(
		RID p_item, const Transform2D& p_transform);

	/* CANVAS LIGHT */

	static RID canvas_light_create();
	static void canvas_light_set_mode(RID p_light, RSE::CanvasLightMode p_mode);
	static void canvas_light_attach_to_canvas(RID p_light, RID p_canvas);
	static void canvas_light_set_enabled(RID p_light, bool p_enabled);
	static void canvas_light_set_transform(RID p_light, const Transform2D& p_transform);
	static void canvas_light_set_color(RID p_light, const Color& p_color);
	static void canvas_light_set_height(RID p_light, float p_height);
	static void canvas_light_set_energy(RID p_light, float p_energy);
	static void canvas_light_set_z_range(RID p_light, int p_min_z, int p_max_z);
	static void canvas_light_set_layer_range(RID p_light, int p_min_layer, int p_max_layer);
	static void canvas_light_set_item_cull_mask(RID p_light, int p_mask);
	static void canvas_light_set_item_shadow_cull_mask(RID p_light, int p_mask);

	static void canvas_light_set_directional_distance(RID p_light, float p_distance);

	static void canvas_light_set_texture_scale(RID p_light, float p_scale);
	static void canvas_light_set_texture(RID p_light, RID p_texture);
	static void canvas_light_set_texture_offset(RID p_light, const Vector2& p_offset);

	static void canvas_light_set_blend_mode(RID p_light, RSE::CanvasLightBlendMode p_mode);

	static void canvas_light_set_shadow_enabled(RID p_light, bool p_enabled);
	static void canvas_light_set_shadow_filter(RID p_light, RSE::CanvasLightShadowFilter p_filter);
	static void canvas_light_set_shadow_color(RID p_light, const Color& p_color);
	static void canvas_light_set_shadow_smooth(RID p_light, float p_smooth);

	static void canvas_light_set_interpolated(RID p_light, bool p_interpolated);
	static void canvas_light_reset_physics_interpolation(RID p_light);
	static void canvas_light_transform_physics_interpolation(
		RID p_light, const Transform2D& p_transform);

	/* CANVAS LIGHT OCCLUDER API */

	static RID canvas_light_occluder_create();
	static void canvas_light_occluder_attach_to_canvas(RID p_occluder, RID p_canvas);
	static void canvas_light_occluder_set_enabled(RID p_occluder, bool p_enabled);
	static void canvas_light_occluder_set_polygon(RID p_occluder, RID p_polygon);
	static void canvas_light_occluder_set_as_sdf_collision(RID p_occluder, bool p_enable);
	static void canvas_light_occluder_set_transform(RID p_occluder, const Transform2D& p_xform);
	static void canvas_light_occluder_set_light_mask(RID p_occluder, int p_mask);

	static void canvas_light_occluder_set_interpolated(RID p_occluder, bool p_interpolated);
	static void canvas_light_occluder_reset_physics_interpolation(RID p_occluder);
	static void canvas_light_occluder_transform_physics_interpolation(
		RID p_occluder, const Transform2D& p_transform);

	/* CANVAS OCCLUDER POLYGON API */

	static RID canvas_occluder_polygon_create();
	static void canvas_occluder_polygon_set_shape(
		RID p_occluder_polygon, const Vector<Vector2>& p_shape, bool p_closed);

	static void canvas_occluder_polygon_set_cull_mode(
		RID p_occluder_polygon, RSE::CanvasOccluderPolygonCullMode p_mode);

	static void canvas_set_shadow_texture_size(int p_size);

	static Rect2 debug_canvas_item_get_rect(RID p_item);
	static Rect2 _debug_canvas_item_get_rect(RID p_item);

	/* GLOBAL SHADER PARAMETERS API */

	static void global_shader_parameter_remove(const String& p_name);
	static Vector<String> global_shader_parameter_get_list();

	static RSE::GlobalShaderParameterType global_shader_parameter_get_type(
		const String& p_name);

	static void global_shader_parameters_load_settings(bool p_load_textures);
	static void global_shader_parameters_clear();

	static int global_shader_uniform_type_get_shader_datatype(
		RSE::GlobalShaderParameterType p_type);

	/* FREE */

	static void free_rid(RID p_rid);

	/* INTERPOLATION */

	static void set_physics_interpolation_enabled(bool p_enabled);

	/* EVENT QUEUING */

	static void draw(bool p_swap_buffers = true, double p_frame_step = 0.0);
	static void sync();
	static bool has_changed();
	static void tick();
	static void pre_draw(bool p_will_draw);

	/* STATUS INFORMATION */

	static uint64_t get_rendering_info(RSE::RenderingInfo p_info);
	static String get_video_adapter_name();
	static String get_video_adapter_vendor();
	static RenderingDeviceEnums::DeviceType get_video_adapter_type();
	static String get_video_adapter_api_version();

	static void set_frame_profiling_enabled(bool p_enable);
	static Vector<RenderingServerTypes::FrameProfileArea> get_frame_profile();
	static uint64_t get_frame_profile_frame();

	static double get_frame_setup_time_cpu();

	static void gi_set_use_half_resolution(bool p_enable);

	/* TESTING */

	static RID get_test_cube();
	static RID get_test_texture();
	static RID get_white_texture();

	static void sdfgi_set_debug_probe_select(const Vector3& p_position, const Vector3& p_dir);

	static RID make_sphere_mesh(int p_lats, int p_lons, real_t p_radius);

	static void mesh_add_surface_from_mesh_data(
		RID p_mesh, const Geometry3D::MeshData& p_mesh_data);
	static void mesh_add_surface_from_planes(RID p_mesh, const Vector<Plane>& p_planes);

	/* BACKGROUND */

	static void set_boot_image_with_stretch(const Ref<Image>& p_image, const Color& p_color,
		RSE::SplashStretchMode p_stretch_mode, bool p_use_filter = true);

	static Color get_default_clear_color();
	static void set_default_clear_color(const Color& p_color);

	/* MISC */

	static bool has_os_feature(const String& p_feature);

	static void set_debug_generate_wireframes(bool p_generate);

	static void call_set_vsync_mode(
		DisplayServerEnums::VSyncMode p_mode, DisplayServerEnums::WindowID p_window);

	static bool is_low_end();

	static void set_print_gpu_profile(bool p_enable);

	static Size2i get_maximum_viewport_size();

	static bool is_render_loop_enabled();
	static void set_render_loop_enabled(bool p_enabled);

	static bool is_on_render_thread();

	static String get_current_rendering_driver_name();
	static String get_current_rendering_method();

#ifdef TOOLS_ENABLED
	static void get_argument_options(
		const String& p_function, int p_idx, List<String>* r_options);

	typedef void (*SurfaceUpgradeCallback)();
	static void set_surface_upgrade_callback(SurfaceUpgradeCallback p_callback);
	static void set_warn_on_surface_upgrade(bool p_warn);
#endif

	static void redraw_request()
	{
		if (data) {
			data->backend.changes++;
		}
	}

	/* LIFECYCLE DISALLOWANCE */

	Renderer() = delete;
	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;
	~Renderer() = delete;
};


