/**************************************************************************/
/*  light_storage.h                                                       */
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

#pragma once

#include "core/math/aabb.h"
#include "core/math/color.h"
#include "core/math/projection.h"
#include "core/math/transform_3d.h"
#include "core/math/vector2.h"
#include "core/math/vector3.h"
#include "core/templates/rid.h"
#include "servers/rendering/rendering_server_enums.h"
#include "servers/rendering/storage/render_scene_buffers.h"

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


