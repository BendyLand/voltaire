/**************************************************************************/
/*  renderer_scene_render_rd.h                                            */
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
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

#include "servers/rendering/renderer_compositor.h"
#include "servers/rendering/renderer_rd/effects/bokeh_dof.h"
#include "servers/rendering/renderer_rd/effects/copy_effects.h"
#include "servers/rendering/renderer_rd/effects/debug_effects.h"
#include "servers/rendering/renderer_rd/effects/fsr.h"
#include "servers/rendering/renderer_rd/effects/luminance.h"
#include "servers/rendering/renderer_rd/effects/resolve.h"
#include "servers/rendering/renderer_rd/effects/smaa.h"
#include "servers/rendering/renderer_rd/effects/tone_mapper.h"
#include "servers/rendering/renderer_rd/effects/vrs.h"
#include "servers/rendering/renderer_rd/environment/gi.h"
#include "servers/rendering/renderer_rd/environment/sky.h"
#include "servers/rendering/renderer_rd/storage_rd/light_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/render_data_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/render_scene_buffers_rd.h"
#include "servers/rendering/renderer_scene_render.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/rendering_server_types.h"
#include "servers/rendering/rendering_shader_library.h"

#ifdef METAL_ENABLED
#include "servers/rendering/renderer_rd/effects/metal_fx.h"
#endif

class RendererSceneRenderRD final : public RenderingShaderLibrary
{
	static inline BinaryMutex _thread_safe_mutex;

	friend class RendererRD::SkyRD;
	friend class RendererRD::GI;

public:
	using CameraData = RendererSceneRender::CameraData;
	using RenderShadowData = RendererSceneRender::RenderShadowData;
	using RenderSDFGIData = RendererSceneRender::RenderSDFGIData;
	using RenderSDFGIUpdateData = RendererSceneRender::RenderSDFGIUpdateData;

	struct Data
	{
		RendererRD::ForwardIDStorage* forward_id_storage = nullptr;
		RendererRD::BokehDOF* bokeh_dof = nullptr;
		RendererRD::CopyEffects* copy_effects = nullptr;
		RendererRD::DebugEffects* debug_effects = nullptr;
		RendererRD::Luminance* luminance = nullptr;
		RendererRD::SMAA* smaa = nullptr;
		RendererRD::ToneMapper* tone_mapper = nullptr;
		RendererRD::FSR* fsr = nullptr;
		RendererRD::VRS* vrs = nullptr;
		RendererRD::Resolve* resolve_effects = nullptr;
#ifdef METAL_ENABLED
		RendererRD::MFXSpatialEffect* mfx_spatial = nullptr;
#endif
		double time = 0.0;
		double time_step = 0.0;

		/* ENVIRONMENT */
		bool glow_bicubic_upscale = false;
		bool use_physical_light_units = false;

		// Needed for single argument calls (material and uv2)
		PagedArrayPool<RenderGeometryInstance*> cull_argument_pool;
		PagedArray<RenderGeometryInstance*> cull_argument;

		RendererRD::SkyRD sky;
		RendererRD::GI gi;

		RSE::ViewportDebugDraw debug_draw = RSE::VIEWPORT_DEBUG_DRAW_DISABLED;

		/* Shadow atlas */
		RSE::ShadowQuality shadows_quality = RSE::SHADOW_QUALITY_MAX;
		RSE::ShadowQuality directional_shadow_quality = RSE::SHADOW_QUALITY_MAX;
		float shadows_quality_radius = 1.0f;
		float directional_shadow_quality_radius = 1.0f;

		float* directional_penumbra_shadow_kernel = nullptr;
		float* directional_soft_shadow_kernel = nullptr;
		float* penumbra_shadow_kernel = nullptr;
		float* soft_shadow_kernel = nullptr;
		bool lightmap_filter_bicubic = false;
		int directional_penumbra_shadow_samples = 0;
		int directional_soft_shadow_samples = 0;
		int penumbra_shadow_samples = 0;
		int soft_shadow_samples = 0;
		RSE::DecalFilter decals_filter = RSE::DECAL_FILTER_LINEAR_MIPMAPS;
		RSE::LightProjectorFilter light_projectors_filter =
			RSE::LIGHT_PROJECTOR_FILTER_LINEAR_MIPMAPS;
		bool material_use_debanding = false;

		/* GI */
		bool screen_space_roughness_limiter = false;
		float screen_space_roughness_limiter_amount = 0.25f;
		float screen_space_roughness_limiter_limit = 0.18f;

		/* Light data */
		uint64_t scene_pass = 0;
		uint32_t max_cluster_elements = 512;

		/* Fog */
		bool fog_use_legacy_blending = false;

		/* Volumetric Fog */
		uint32_t volumetric_fog_size = 128;
		uint32_t volumetric_fog_depth = 128;
		bool volumetric_fog_filter_active = true;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();

	static bool is_initialized() { return data != nullptr; }

public:
	static void setup_render_buffer_data(Ref<RenderSceneBuffersRD> p_render_buffers);

	static void _render_scene(RenderDataRD* p_render_data, const Color& p_default_color);
	static void _render_buffers_debug_draw(const RenderDataRD* p_render_data);

	static void _render_material(const Transform3D& p_cam_transform,
		const Projection& p_cam_projection, bool p_cam_orthogonal,
		const PagedArray<RenderGeometryInstance*>& p_instances, RID p_framebuffer,
		const Rect2i& p_region, float p_exposure_normalization);
	static void _render_uv2(const PagedArray<RenderGeometryInstance*>& p_instances,
		RID p_framebuffer, const Rect2i& p_region);
	static void _render_sdfgi(Ref<RenderSceneBuffersRD> p_render_buffers, const Vector3i& p_from,
		const Vector3i& p_size, const AABB& p_bounds,
		const PagedArray<RenderGeometryInstance*>& p_instances, const RID& p_albedo_texture,
		const RID& p_emission_texture, const RID& p_emission_aniso_texture,
		const RID& p_geom_facing_texture, float p_exposure_normalization);
	static void _render_particle_collider_heightfield(RID p_fb, const Transform3D& p_cam_transform,
		const Projection& p_cam_projection, const PagedArray<RenderGeometryInstance*>& p_instances);


	static RID _render_buffers_get_normal_texture(Ref<RenderSceneBuffersRD> p_render_buffers);
	static RID _render_buffers_get_velocity_texture(Ref<RenderSceneBuffersRD> p_render_buffers);

	static RendererRD::ForwardIDStorage* create_forward_id_storage()
	{
		return memnew(RendererRD::ForwardIDStorage);
	}

	static void _post_prepass_render(RenderDataRD* p_render_data, bool p_use_gi);

	static bool _has_compositor_effect(
		RSE::CompositorEffectCallbackType p_callback_type, const RenderDataRD* p_render_data);
	static void _render_buffers_ensure_screen_texture(const RenderDataRD* p_render_data);
	static void _render_buffers_copy_screen_texture(const RenderDataRD* p_render_data);
	static void _render_buffers_ensure_depth_texture(const RenderDataRD* p_render_data);
	static void _render_buffers_copy_depth_texture(
		const RenderDataRD* p_render_data, bool p_use_msaa = false);
	static void _render_buffers_post_process_and_tonemap(
		const RenderDataRD* p_render_data, bool p_use_msaa = false);
	static void _post_process_subpass(
		RID p_source_texture, RID p_framebuffer, const RenderDataRD* p_render_data);
	static void _disable_clear_request(const RenderDataRD* p_render_data);

	static void _update_shader_quality_settings() {}

	static bool _debug_draw_can_use_effects(RSE::ViewportDebugDraw p_debug_draw);

	static void _debug_sdfgi_probes(Ref<RenderSceneBuffersRD> p_render_buffers, RID p_framebuffer,
		uint32_t p_view_count, const Projection* p_camera_with_transforms);
	static void _process_compositor_effects(
		RSE::CompositorEffectCallbackType p_callback_type, const RenderDataRD* p_render_data);
	static bool _needs_post_prepass_render(RenderDataRD* p_render_data, bool p_use_gi);
	static void _update_vrs(Ref<RenderSceneBuffersRD> p_render_buffers);
	static bool _compositor_effects_has_flag(const RenderDataRD* p_render_data,
		RSE::CompositorEffectFlags p_flag,
		RSE::CompositorEffectCallbackType p_callback_type =
			RSE::COMPOSITOR_EFFECT_CALLBACK_TYPE_ANY);

	/* LIGHTING */

	static void setup_added_reflection_probe(
		const Transform3D& p_transform, const Vector3& p_half_size)
	{
	}

	static void setup_added_light(const RSE::LightType p_type, const Transform3D& p_transform,
		float p_radius, float p_spot_aperture, const Vector2& p_area_size)
	{
	}

	static void setup_added_decal(const Transform3D& p_transform, const Vector3& p_half_size) {}

	/* GI */

	static RendererRD::GI* get_gi() { return &data->gi; }

	/* SKY */

	static RendererRD::SkyRD* get_sky() { return &data->sky; }

	/* SKY API */

	static RID sky_allocate();
	static void sky_initialize(RID p_rid);

	static void sky_set_radiance_size(RID p_sky, int p_radiance_size);
	static void sky_set_mode(RID p_sky, RSE::SkyMode p_mode);
	static void sky_set_material(RID p_sky, RID p_material);
	static Ref<Image> sky_bake_panorama(
		RID p_sky, float p_energy, bool p_bake_irradiance, const Size2i& p_size);

	/* ENVIRONMENT API */

	static void environment_glow_set_use_bicubic_upscale(bool p_enable);

	static void environment_set_volumetric_fog_volume_size(int p_size, int p_depth);
	static void environment_set_volumetric_fog_filter_active(bool p_enable);

	static void environment_set_sdfgi_ray_count(RSE::EnvironmentSDFGIRayCount p_ray_count);
	static void environment_set_sdfgi_frames_to_converge(
		RSE::EnvironmentSDFGIFramesToConverge p_frames);
	static void environment_set_sdfgi_frames_to_update_light(
		RSE::EnvironmentSDFGIFramesToUpdateLight p_update);

	static Ref<Image> environment_bake_panorama(
		RID p_env, bool p_bake_irradiance, const Size2i& p_size);

	static _FORCE_INLINE_ bool is_using_physical_light_units()
	{
		return data->use_physical_light_units;
	}

	/* REFLECTION PROBE */

	static RID reflection_probe_create_framebuffer(RID p_color, RID p_depth);

	/* FOG VOLUMES */

	static uint32_t get_volumetric_fog_size() { return data->volumetric_fog_size; }

	static uint32_t get_volumetric_fog_depth() { return data->volumetric_fog_depth; }

	static bool get_volumetric_fog_filter_active() { return data->volumetric_fog_filter_active; }

	static RID fog_volume_instance_create(RID p_fog_volume);
	static void fog_volume_instance_set_transform(
		RID p_fog_volume_instance, const Transform3D& p_transform);
	static void fog_volume_instance_set_active(RID p_fog_volume_instance, bool p_active);
	static RID fog_volume_instance_get_volume(RID p_fog_volume_instance);
	static Vector3 fog_volume_instance_get_position(RID p_fog_volume_instance);

	/* GI LIGHT PROBES */

	static RID voxel_gi_instance_create(RID p_base);
	static void voxel_gi_instance_set_transform_to_data(RID p_probe, const Transform3D& p_xform);
	static bool voxel_gi_needs_update(RID p_probe);
	static void voxel_gi_update(RID p_probe, bool p_update_light_instances,
		const Vector<RID>& p_light_instances,
		const PagedArray<RenderGeometryInstance*>& p_dynamic_objects);

	static void voxel_gi_set_quality(RSE::VoxelGIQuality p_quality)
	{
		data->gi.voxel_gi_quality = p_quality;
	}

	/* RENDER BUFFERS */
	static RD::DataFormat _render_buffers_get_preferred_color_format();
	static bool _render_buffers_can_be_storage();
	static Ref<RenderSceneBuffers> render_buffers_create();
	static void gi_set_use_half_resolution(bool p_enable);

	static RID render_buffers_get_default_voxel_gi_buffer();

	static void base_uniforms_changed();

	static void render_scene(const Ref<RenderSceneBuffers>& p_render_buffers,
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
		RenderingServerTypes::RenderInfo* r_render_info = nullptr);

	static void render_material(const Transform3D& p_cam_transform,
		const Projection& p_cam_projection, bool p_cam_orthogonal,
		const PagedArray<RenderGeometryInstance*>& p_instances, RID p_framebuffer,
		const Rect2i& p_region);

	static void render_particle_collider_heightfield(RID p_collider, const Transform3D& p_transform,
		const PagedArray<RenderGeometryInstance*>& p_instances);

	static void set_scene_pass(uint64_t p_pass) { data->scene_pass = p_pass; }

	static _FORCE_INLINE_ uint64_t get_scene_pass() { return data->scene_pass; }

	static void screen_space_roughness_limiter_set_active(
		bool p_enable, float p_amount, float p_limit);
	static bool screen_space_roughness_limiter_is_active();
	static float screen_space_roughness_limiter_get_amount();
	static float screen_space_roughness_limiter_get_limit();

	static void positional_soft_shadow_filter_set_quality(RSE::ShadowQuality p_quality);
	static void directional_soft_shadow_filter_set_quality(RSE::ShadowQuality p_quality);

	static void decals_set_filter(RSE::DecalFilter p_filter);
	static void light_projectors_set_filter(RSE::LightProjectorFilter p_filter);
	static void lightmaps_set_bicubic_filter(bool p_enable);
	static void material_set_use_debanding(bool p_enable);

	_FORCE_INLINE_ static RSE::ShadowQuality shadows_quality_get() { return data->shadows_quality; }

	_FORCE_INLINE_ static RSE::ShadowQuality directional_shadow_quality_get()
	{
		return data->directional_shadow_quality;
	}

	static _FORCE_INLINE_ float shadows_quality_radius_get()
	{
		if (!data->shadows_quality_radius) {
			data->shadows_quality_radius = 1.0f;
		}
		return data->shadows_quality_radius;
	}

	static _FORCE_INLINE_ float directional_shadow_quality_radius_get()
	{
		if (!data->directional_shadow_quality_radius) {
			data->directional_shadow_quality_radius = 1.0f;
		}
		return data->directional_shadow_quality_radius;
	}

	_FORCE_INLINE_ static float* directional_penumbra_shadow_kernel_get()
	{
		return data->directional_penumbra_shadow_kernel;
	}

	_FORCE_INLINE_ static float* directional_soft_shadow_kernel_get()
	{
		return data->directional_soft_shadow_kernel;
	}

	_FORCE_INLINE_ static float* penumbra_shadow_kernel_get()
	{
		return data->penumbra_shadow_kernel;
	}

	_FORCE_INLINE_ static float* soft_shadow_kernel_get() { return data->soft_shadow_kernel; }

	_FORCE_INLINE_ static int directional_penumbra_shadow_samples_get()
	{
		return data->directional_penumbra_shadow_samples;
	}

	_FORCE_INLINE_ static bool
 lightmap_filter_bicubic_get()
	{
		return data->lightmap_filter_bicubic;
	}

	_FORCE_INLINE_ static int directional_soft_shadow_samples_get()
	{
		return data->directional_soft_shadow_samples;
	}

	_FORCE_INLINE_ static int penumbra_shadow_samples_get()
	{
		return data->penumbra_shadow_samples;
	}

	_FORCE_INLINE_ static int soft_shadow_samples_get() { return data->soft_shadow_samples; }

	_FORCE_INLINE_ static RSE::LightProjectorFilter light_projectors_get_filter()
	{
		return data->light_projectors_filter;
	}

	_FORCE_INLINE_ static RSE::DecalFilter decals_get_filter() { return data->decals_filter; }

	_FORCE_INLINE_ static bool material_use_debanding_get() { return data->material_use_debanding; }

	static _FORCE_INLINE_ bool fog_use_legacy_blending_get()
	{
		return data->fog_use_legacy_blending;
	}

	static int get_roughness_layers();
	static bool is_using_radiance_octmap_array();

	static bool free(RID p_rid);

	static void update();

	static void set_debug_draw_mode(RSE::ViewportDebugDraw p_debug_draw);

	_FORCE_INLINE_ static RSE::ViewportDebugDraw get_debug_draw_mode() { return data->debug_draw; }

	static void set_time(double p_time, double p_step);

	static void sdfgi_set_debug_probe_select(const Vector3& p_position, const Vector3& p_dir);

	static bool is_vrs_supported();
	static bool is_dynamic_gi_supported();
	static bool is_volumetric_supported();
	static uint32_t get_max_elements();

	static void init();

	RendererSceneRenderRD() = delete;
	RendererSceneRenderRD(const RendererSceneRenderRD&) = delete;
	RendererSceneRenderRD& operator=(const RendererSceneRenderRD&) = delete;
	~RendererSceneRenderRD() = delete;
};


