/**************************************************************************/
/*  renderer_scene_render.h                                               */
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

#include "core/math/projection.h"
#include "core/templates/paged_allocator.h"
// --- Folded from servers/rendering/renderer_geometry_instance.h ---
#include "core/math/color.h"
#include "core/math/rect2.h"
#include "core/math/transform_3d.h"
#include "core/templates/rid.h"
#include "servers/rendering/storage/utilities.h"

// Base implementation of RenderGeometryInstance shared by internal renderers.
class RenderGeometryInstance {
public:
	// setup
	uint32_t base_flags = 0;
	uint32_t flags_cache = 0;

	// used during rendering
	float depth = 0;

	RID mesh_instance;

	Transform3D transform;
	bool mirror = false;
	AABB transformed_aabb;
	bool non_uniform_scale = false;
	float lod_model_scale = 1.0;
	float lod_bias = 0.0;
	float sorting_offset = 0.0;
	bool use_aabb_center = true;

	uint32_t layer_mask = 1;

	bool fade_near = false;
	float fade_near_begin = 0;
	float fade_near_end = 0;
	bool fade_far = false;
	float fade_far_begin = 0;
	float fade_far_end = 0;

	float parent_fade_alpha = 1.0;
	float force_alpha = 1.0;

	int32_t shader_uniforms_offset = -1;

	struct Data {
		//data used less often goes into regular heap
		RID base;
		RSE::InstanceType base_type;

		RID skeleton;
		Vector<RID> surface_materials;
		RID material_override;
		RID material_overlay;
		AABB aabb;

		bool use_baked_light = false;
		bool use_dynamic_gi = false;
		bool cast_double_sided_shadows = false;
		bool dirty_dependencies = false;

		DependencyTracker dependency_tracker;
	};

	Data *data = nullptr;

	virtual ~RenderGeometryInstance() {}

	virtual void _mark_dirty() {}

	void set_skeleton(RID p_skeleton);
	void set_material_override(RID p_override);
	void set_material_overlay(RID p_overlay);
	void set_surface_materials(const Vector<RID> &p_materials);
	void set_mesh_instance(RID p_mesh_instance);
	virtual void set_transform(const Transform3D &p_transform, const AABB &p_aabb, const AABB &p_transformed_aabb);
	void set_pivot_data(float p_sorting_offset, bool p_use_aabb_center);
	void set_lod_bias(float p_lod_bias);
	void set_layer_mask(uint32_t p_layer_mask);
	void set_fade_range(bool p_enable_near, float p_near_begin, float p_near_end, bool p_enable_far, float p_far_begin, float p_far_end);
	void set_parent_fade_alpha(float p_alpha);
	void set_transparency(float p_transparency);
	void set_use_baked_light(bool p_enable);
	void set_use_dynamic_gi(bool p_enable);
	virtual void set_use_lightmap(RID p_lightmap_instance, const Rect2 &p_lightmap_uv_scale, int p_lightmap_slice_index) {}
	virtual void set_lightmap_capture(const Color *p_sh9) {}
	void set_instance_shader_uniforms_offset(int32_t p_offset);
	void set_cast_double_sided_shadows(bool p_enable);

	virtual void reset_motion_vectors();

	Transform3D get_transform();
	AABB get_aabb();

	virtual void clear_light_instances() {}
	virtual void pair_light_instance(const RID p_light_instance, RSE::LightType light_type, uint32_t placement_idx) {}
	virtual void pair_reflection_probe_instances(const RID *p_reflection_probe_instances, uint32_t p_reflection_probe_instance_count) {}
	virtual void pair_decal_instances(const RID *p_decal_instances, uint32_t p_decal_instance_count) {}
	virtual void pair_voxel_gi_instances(const RID *p_voxel_gi_instances, uint32_t p_voxel_gi_instance_count) {}

	virtual void set_softshadow_projector_pairing(bool p_softshadow, bool p_projector) {}
};

// --- End servers/rendering/renderer_geometry_instance.h ---
#include "servers/rendering/rendering_server_types.h"
// --- Folded from servers/rendering/storage/compositor_storage.h ---
#include "core/templates/rid_owner.h"
#include "servers/rendering/rendering_server_enums.h"

class RendererCompositorStorage final
{
public:
	RendererCompositorStorage() = delete;
	~RendererCompositorStorage() = delete;
	RendererCompositorStorage(const RendererCompositorStorage&) = delete;
	RendererCompositorStorage& operator=(const RendererCompositorStorage&) = delete;

private:
	static inline bool _initialized = false;
	static inline int num_compositor_effects_with_motion_vectors = 0;

	// Compositor effect
	struct CompositorEffect
	{
		bool is_enabled = true;
		RSE::CompositorEffectCallbackType callback_type;

		uint32_t flags = {};
	};

	static inline RID_Owner<CompositorEffect, true> compositor_effects_owner;

	// Compositor
	struct Compositor
	{
		// Compositor effects
		Vector<RID> compositor_effects;
	};

	static inline RID_Owner<Compositor, true> compositor_owner;

public:
	static void initialize();
	static void finalize();

	static bool is_initialized() { return _initialized; }

	static int get_num_compositor_effects_with_motion_vectors()
	{
		return num_compositor_effects_with_motion_vectors;
	}

	// Compositor effect
	static RID compositor_effect_allocate();
	static void compositor_effect_initialize(RID p_rid);
	static void compositor_effect_free(RID p_rid);

	static bool is_compositor_effect(RID p_effect)
	{
		return compositor_effects_owner.owns(p_effect);
	}

	static void compositor_effect_set_enabled(RID p_effect, bool p_enabled);
	static bool compositor_effect_get_enabled(RID p_effect);

	static RSE::CompositorEffectCallbackType compositor_effect_get_callback_type(RID p_effect);

	static void compositor_effect_set_flag(
		RID p_effect, RSE::CompositorEffectFlags p_flag, bool p_set);
	static bool compositor_effect_get_flag(RID p_effect, RSE::CompositorEffectFlags p_flag);

	// Compositor
	static RID compositor_allocate();
	static void compositor_initialize(RID p_rid);
	static void compositor_free(RID p_rid);

	static bool is_compositor(RID p_compositor) { return compositor_owner.owns(p_compositor); }

	static void compositor_set_compositor_effects(RID p_compositor, const Vector<RID>& p_effects);
	static Vector<RID> compositor_get_compositor_effects(RID p_compositor,
		RSE::CompositorEffectCallbackType p_callback_type =
			RSE::COMPOSITOR_EFFECT_CALLBACK_TYPE_ANY,
		bool p_enabled_only = true);
};

// --- End servers/rendering/storage/compositor_storage.h ---
// --- Folded from servers/rendering/storage/environment_storage.h ---
#include "core/templates/rid_owner.h"
#include "servers/rendering/rendering_server_enums.h"

class RendererEnvironmentStorage final
{
public:
	RendererEnvironmentStorage() = delete;
	~RendererEnvironmentStorage() = delete;
	RendererEnvironmentStorage(const RendererEnvironmentStorage&) = delete;
	RendererEnvironmentStorage& operator=(const RendererEnvironmentStorage&) = delete;

	union TonemapParameters
	{
		// Shader vec4:
		float tonemapper_params[4];

		// Reinhard:
		struct
		{
			float white_squared;
		};

		// Filmic and ACES:
		struct
		{
			float white_tonemapped;
		};

		// AgX:
		struct
		{
			float awp_contrast;
			float awp_toe_a;
			float awp_slope;
			float awp_w;
		};
	};

private:
	static inline bool _initialized = false;

	// Environment
	struct Environment
	{
		// Note, we capture and store all environment parameters received from Godot here.
		// Not all renderers support all effects and should just ignore the bits they don't support.

		// Background
		RSE::EnvironmentBG background = RSE::ENV_BG_CLEAR_COLOR;
		RID sky;
		float sky_custom_fov = 0.0;
		Basis sky_orientation;
		Color bg_color;
		float bg_energy_multiplier = 1.0;
		float bg_intensity = 1.0; // Measured in nits or candela/m^2. Default to 1.0 so this doesn't impact rendering when Physical Light Units disabled.
		int canvas_max_layer = 0;
		RSE::EnvironmentAmbientSource ambient_source = RSE::ENV_AMBIENT_SOURCE_BG;
		Color ambient_light;
		float ambient_light_energy = 1.0;
		float ambient_sky_contribution = 1.0;
		RSE::EnvironmentReflectionSource reflection_source = RSE::ENV_REFLECTION_SOURCE_BG;
		int camera_feed_id = 0;

		// Tonemap
		RSE::EnvironmentToneMapper tone_mapper;
		float exposure = 1.0;
		float white = 1.0;
		float tonemap_agx_contrast = 1.25; // Default to approximately Blender's AgX contrast
		float max_value = 1.0;

		// Fog
		bool fog_enabled = false;
		RSE::EnvironmentFogMode fog_mode = RSE::EnvironmentFogMode::ENV_FOG_MODE_EXPONENTIAL;
		Color fog_light_color = Color(0.518, 0.553, 0.608);
		float fog_light_energy = 1.0;
		float fog_sun_scatter = 0.0;
		float fog_density = 0.01;
		float fog_sky_affect = 1.0;
		float fog_height = 0.0;
		float fog_height_density = 0.0; //can be negative to invert effect
		float fog_aerial_perspective = 0.0;

		// Depth Fog
		float fog_depth_curve = 1.0;
		float fog_depth_begin = 10.0;
		float fog_depth_end = 100.0;

		// Volumetric Fog
		bool volumetric_fog_enabled = false;
		float volumetric_fog_density = 0.01;
		Color volumetric_fog_scattering = Color(1, 1, 1);
		Color volumetric_fog_emission = Color(0, 0, 0);
		float volumetric_fog_emission_energy = 0.0;
		float volumetric_fog_anisotropy = 0.2;
		float volumetric_fog_length = 64.0;
		float volumetric_fog_detail_spread = 2.0;
		float volumetric_fog_gi_inject = 1.0;
		float volumetric_fog_ambient_inject = 0.0;
		float volumetric_fog_sky_affect = 1.0;
		bool volumetric_fog_temporal_reprojection = true;
		float volumetric_fog_temporal_reprojection_amount = 0.9;

		// Glow
		bool glow_enabled = false;
		Vector<float> glow_levels;
		float glow_intensity = 0.3;
		float glow_strength = 1.0;
		float glow_bloom = 0.0;
		float glow_mix = 0.01;
		RSE::EnvironmentGlowBlendMode glow_blend_mode = RSE::ENV_GLOW_BLEND_MODE_SCREEN;
		float glow_hdr_bleed_threshold = 1.0;
		float glow_hdr_luminance_cap = 12.0;
		float glow_hdr_bleed_scale = 2.0;
		float glow_map_strength = 0.0f; // 1.0f in GLES3 ??
		RID glow_map;

		// SSR
		bool ssr_enabled = false;
		int ssr_max_steps = 64;
		float ssr_fade_in = 0.15;
		float ssr_fade_out = 2.0;
		float ssr_depth_tolerance = 0.5;

		// SSAO
		bool ssao_enabled = false;
		float ssao_radius = 1.0;
		float ssao_intensity = 2.0;
		float ssao_power = 1.5;
		float ssao_detail = 0.5;
		float ssao_horizon = 0.06;
		float ssao_sharpness = 0.98;
		float ssao_direct_light_affect = 0.0;
		float ssao_ao_channel_affect = 0.0;

		// SSIL
		bool ssil_enabled = false;
		float ssil_radius = 5.0;
		float ssil_intensity = 1.0;
		float ssil_sharpness = 0.98;
		float ssil_normal_rejection = 1.0;

		// SDFGI
		bool sdfgi_enabled = false;
		int sdfgi_cascades = 4;
		float sdfgi_min_cell_size = 0.2;
		bool sdfgi_use_occlusion = false;
		float sdfgi_bounce_feedback = 0.5;
		bool sdfgi_read_sky_light = true;
		float sdfgi_energy = 1.0;
		float sdfgi_normal_bias = 1.1;
		float sdfgi_probe_bias = 1.1;
		RSE::EnvironmentSDFGIYScale sdfgi_y_scale = RSE::ENV_SDFGI_Y_SCALE_75_PERCENT;

		// Adjustments
		bool adjustments_enabled = false;
		float adjustments_brightness = 1.0f;
		float adjustments_contrast = 1.0f;
		float adjustments_saturation = 1.0f;
		bool use_1d_color_correction = false;
		RID color_correction;
	};

	static inline RID_Owner<Environment, true> environment_owner;

public:
	static void initialize();
	static void finalize();

	static bool is_initialized() { return _initialized; }

	// Environment
	static RID environment_allocate();
	static void environment_initialize(RID p_rid);
	static void environment_free(RID p_rid);

	static bool is_environment(RID p_environment) { return environment_owner.owns(p_environment); }

	// Background
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
	static int environment_get_camera_feed_id(RID p_env);

	static RSE::EnvironmentBG environment_get_background(RID p_env);
	static RID environment_get_sky(RID p_env);
	static float environment_get_sky_custom_fov(RID p_env);
	static Basis environment_get_sky_orientation(RID p_env);
	static Color environment_get_bg_color(RID p_env);
	static float environment_get_bg_energy_multiplier(RID p_env);
	static float environment_get_bg_intensity(RID p_env);
	static int environment_get_canvas_max_layer(RID p_env);
	static RSE::EnvironmentAmbientSource environment_get_ambient_source(RID p_env);
	static Color environment_get_ambient_light(RID p_env);
	static float environment_get_ambient_light_energy(RID p_env);
	static float environment_get_ambient_sky_contribution(RID p_env);
	static RSE::EnvironmentReflectionSource environment_get_reflection_source(RID p_env);

	// Tonemap
	static void environment_set_tonemap(
		RID p_env, RSE::EnvironmentToneMapper p_tone_mapper, float p_exposure, float p_white);
	static RSE::EnvironmentToneMapper environment_get_tone_mapper(RID p_env);
	static float environment_get_exposure(RID p_env);
	static float environment_get_white(RID p_env, bool p_limit_agx_white, float p_output_max_value);
	static void environment_set_tonemap_agx_contrast(RID p_env, float p_agx_contrast);
	static float environment_get_tonemap_agx_contrast(RID p_env);
	static TonemapParameters environment_get_tonemap_parameters(
		RID p_env, bool p_limit_agx_white, float p_output_max_value);

	// Fog
	static void environment_set_fog(RID p_env, bool p_enable, const Color& p_light_color,
		float p_light_energy, float p_sun_scatter, float p_density, float p_height,
		float p_height_density, float p_aerial_perspective, float p_sky_affect,
		RSE::EnvironmentFogMode p_mode);
	static bool environment_get_fog_enabled(RID p_env);
	static RSE::EnvironmentFogMode environment_get_fog_mode(RID p_env);
	static Color environment_get_fog_light_color(RID p_env);
	static float environment_get_fog_light_energy(RID p_env);
	static float environment_get_fog_sun_scatter(RID p_env);
	static float environment_get_fog_density(RID p_env);
	static float environment_get_fog_sky_affect(RID p_env);
	static float environment_get_fog_height(RID p_env);
	static float environment_get_fog_height_density(RID p_env);
	static float environment_get_fog_aerial_perspective(RID p_env);

	// Depth Fog
	static void environment_set_fog_depth(RID p_env, float p_curve, float p_begin, float p_end);
	static float environment_get_fog_depth_curve(RID p_env);
	static float environment_get_fog_depth_begin(RID p_env);
	static float environment_get_fog_depth_end(RID p_env);

	// Volumetric Fog
	static void environment_set_volumetric_fog(RID p_env, bool p_enable, float p_density,
		const Color& p_albedo, const Color& p_emission, float p_emission_energy, float p_anisotropy,
		float p_length, float p_detail_spread, float p_gi_inject, bool p_temporal_reprojection,
		float p_temporal_reprojection_amount, float p_ambient_inject, float p_sky_affect);
	static bool environment_get_volumetric_fog_enabled(RID p_env);
	static float environment_get_volumetric_fog_density(RID p_env);
	static Color environment_get_volumetric_fog_scattering(RID p_env);
	static Color environment_get_volumetric_fog_emission(RID p_env);
	static float environment_get_volumetric_fog_emission_energy(RID p_env);
	static float environment_get_volumetric_fog_anisotropy(RID p_env);
	static float environment_get_volumetric_fog_length(RID p_env);
	static float environment_get_volumetric_fog_detail_spread(RID p_env);
	static float environment_get_volumetric_fog_gi_inject(RID p_env);
	static float environment_get_volumetric_fog_sky_affect(RID p_env);
	static bool environment_get_volumetric_fog_temporal_reprojection(RID p_env);
	static float environment_get_volumetric_fog_temporal_reprojection_amount(RID p_env);
	static float environment_get_volumetric_fog_ambient_inject(RID p_env);

	// GLOW
	static void environment_set_glow(RID p_env, bool p_enable, Vector<float> p_levels,
		float p_intensity, float p_strength, float p_mix, float p_bloom_threshold,
		RSE::EnvironmentGlowBlendMode p_blend_mode, float p_hdr_bleed_threshold,
		float p_hdr_bleed_scale, float p_hdr_luminance_cap, float p_glow_map_strength,
		RID p_glow_map);
	static bool environment_get_glow_enabled(RID p_env);
	static Vector<float> environment_get_glow_levels(RID p_env);
	static float environment_get_glow_intensity(RID p_env);
	static float environment_get_glow_strength(RID p_env);
	static float environment_get_glow_bloom(RID p_env);
	static float environment_get_glow_mix(RID p_env);
	static RSE::EnvironmentGlowBlendMode environment_get_glow_blend_mode(RID p_env);
	static float environment_get_glow_hdr_bleed_threshold(RID p_env);
	static float environment_get_glow_hdr_luminance_cap(RID p_env);
	static float environment_get_glow_hdr_bleed_scale(RID p_env);
	static float environment_get_glow_map_strength(RID p_env);
	static RID environment_get_glow_map(RID p_env);

	// SSR
	static void environment_set_ssr(RID p_env, bool p_enable, int p_max_steps, float p_fade_int,
		float p_fade_out, float p_depth_tolerance);
	static bool environment_get_ssr_enabled(RID p_env);
	static int environment_get_ssr_max_steps(RID p_env);
	static float environment_get_ssr_fade_in(RID p_env);
	static float environment_get_ssr_fade_out(RID p_env);
	static float environment_get_ssr_depth_tolerance(RID p_env);

	// SSAO
	static void environment_set_ssao(RID p_env, bool p_enable, float p_radius, float p_intensity,
		float p_power, float p_detail, float p_horizon, float p_sharpness, float p_light_affect,
		float p_ao_channel_affect);
	static bool environment_get_ssao_enabled(RID p_env);
	static float environment_get_ssao_radius(RID p_env);
	static float environment_get_ssao_intensity(RID p_env);
	static float environment_get_ssao_power(RID p_env);
	static float environment_get_ssao_detail(RID p_env);
	static float environment_get_ssao_horizon(RID p_env);
	static float environment_get_ssao_sharpness(RID p_env);
	static float environment_get_ssao_direct_light_affect(RID p_env);
	static float environment_get_ssao_ao_channel_affect(RID p_env);

	// SSIL
	static void environment_set_ssil(RID p_env, bool p_enable, float p_radius, float p_intensity,
		float p_sharpness, float p_normal_rejection);
	static bool environment_get_ssil_enabled(RID p_env);
	static float environment_get_ssil_radius(RID p_env);
	static float environment_get_ssil_intensity(RID p_env);
	static float environment_get_ssil_sharpness(RID p_env);
	static float environment_get_ssil_normal_rejection(RID p_env);

	// SDFGI
	static void environment_set_sdfgi(RID p_env, bool p_enable, int p_cascades,
		float p_min_cell_size, RSE::EnvironmentSDFGIYScale p_y_scale, bool p_use_occlusion,
		float p_bounce_feedback, bool p_read_sky, float p_energy, float p_normal_bias,
		float p_probe_bias);
	static bool environment_get_sdfgi_enabled(RID p_env);
	static int environment_get_sdfgi_cascades(RID p_env);
	static float environment_get_sdfgi_min_cell_size(RID p_env);
	static bool environment_get_sdfgi_use_occlusion(RID p_env);
	static float environment_get_sdfgi_bounce_feedback(RID p_env);
	static bool environment_get_sdfgi_read_sky_light(RID p_env);
	static float environment_get_sdfgi_energy(RID p_env);
	static float environment_get_sdfgi_normal_bias(RID p_env);
	static float environment_get_sdfgi_probe_bias(RID p_env);
	static RSE::EnvironmentSDFGIYScale environment_get_sdfgi_y_scale(RID p_env);

	// Adjustment
	static void environment_set_adjustment(RID p_env, bool p_enable, float p_brightness,
		float p_contrast, float p_saturation, bool p_use_1d_color_correction,
		RID p_color_correction);
	static bool environment_get_adjustments_enabled(RID p_env);
	static float environment_get_adjustments_brightness(RID p_env);
	static float environment_get_adjustments_contrast(RID p_env);
	static float environment_get_adjustments_saturation(RID p_env);
	static bool environment_get_use_1d_color_correction(RID p_env);
	static RID environment_get_color_correction(RID p_env);
};

// --- End servers/rendering/storage/environment_storage.h ---
// --- Inlined/Folded from servers/rendering/storage/render_scene_buffers.h ---
#include "core/types.h"
#include "servers/rendering/rendering_server_enums.h"

class RenderSceneBuffersConfiguration : public RefCounted
{
private:
	RID render_target;

	Size2i internal_size;
	Size2i target_size;
	uint32_t view_count = 1;

	RSE::ViewportScaling3DMode scaling_3d_mode = RSE::VIEWPORT_SCALING_3D_MODE_OFF;
	RSE::ViewportMSAA msaa_3d = RSE::VIEWPORT_MSAA_DISABLED;
	RSE::ViewportScreenSpaceAA screen_space_aa = RSE::VIEWPORT_SCREEN_SPACE_AA_DISABLED;
	RSE::ViewportAnisotropicFiltering anisotropic_filtering_level = RSE::VIEWPORT_ANISOTROPY_4X;

	float fsr_sharpness = 0.0;
	float texture_mipmap_bias = 0.0;
	bool use_taa = false;
	bool use_debanding = false;

protected:

public:
	RID get_render_target() const { return render_target; }

	void set_render_target(RID p_render_target) { render_target = p_render_target; }

	Size2i get_internal_size() const { return internal_size; }

	void set_internal_size(Size2i p_internal_size) { internal_size = p_internal_size; }

	Size2i get_target_size() const { return target_size; }

	void set_target_size(Size2i p_target_size) { target_size = p_target_size; }

	uint32_t get_view_count() const { return view_count; }

	void set_view_count(uint32_t p_view_count) { view_count = p_view_count; }

	RSE::ViewportScaling3DMode get_scaling_3d_mode() const { return scaling_3d_mode; }

	void set_scaling_3d_mode(RSE::ViewportScaling3DMode p_scaling_3d_mode)
	{
		scaling_3d_mode = p_scaling_3d_mode;
	}

	RSE::ViewportMSAA get_msaa_3d() const { return msaa_3d; }

	void set_msaa_3d(RSE::ViewportMSAA p_msaa_3d) { msaa_3d = p_msaa_3d; }

	RSE::ViewportScreenSpaceAA get_screen_space_aa() const { return screen_space_aa; }

	void set_screen_space_aa(RSE::ViewportScreenSpaceAA p_screen_space_aa)
	{
		screen_space_aa = p_screen_space_aa;
	}

	float get_fsr_sharpness() const { return fsr_sharpness; }

	void set_fsr_sharpness(float p_fsr_sharpness) { fsr_sharpness = p_fsr_sharpness; }

	float get_texture_mipmap_bias() const { return texture_mipmap_bias; }

	void set_texture_mipmap_bias(float p_texture_mipmap_bias)
	{
		texture_mipmap_bias = p_texture_mipmap_bias;
	}

	RSE::ViewportAnisotropicFiltering get_anisotropic_filtering_level() const
	{
		return anisotropic_filtering_level;
	}

	void set_anisotropic_filtering_level(
		RSE::ViewportAnisotropicFiltering p_anisotropic_filtering_level)
	{
		anisotropic_filtering_level = p_anisotropic_filtering_level;
	}

	bool get_use_taa() const { return use_taa; }

	void set_use_taa(bool p_use_taa) { use_taa = p_use_taa; }

	bool get_use_debanding() const { return use_debanding; }

	void set_use_debanding(bool p_use_debanding) { use_debanding = p_use_debanding; }

	RenderSceneBuffersConfiguration() {}

	virtual ~RenderSceneBuffersConfiguration() {}
};

class RenderSceneBuffers : public RefCounted
{
protected:

public:
	RenderSceneBuffers() {}

	virtual ~RenderSceneBuffers() {}

	virtual void configure(const RenderSceneBuffersConfiguration* p_config) = 0;

	// for those settings that are unlikely to require buffers to be recreated, we'll add setters
	virtual void set_fsr_sharpness(float p_fsr_sharpness) = 0;
	virtual void set_texture_mipmap_bias(float p_texture_mipmap_bias) = 0;
	virtual void set_anisotropic_filtering_level(
		RSE::ViewportAnisotropicFiltering p_anisotropic_filtering_level) = 0;
	virtual void set_use_debanding(bool p_use_debanding) = 0;
};

class RendererSceneRender
{
public:
	enum
	{
		MAX_DIRECTIONAL_LIGHTS = 8,
		MAX_DIRECTIONAL_LIGHT_CASCADES = 4,
		MAX_RENDER_VIEWS = 2
	};

	/* Geometry Instance */

	virtual RenderGeometryInstance* geometry_instance_create(RID p_base);
	virtual void geometry_instance_free(RenderGeometryInstance* p_geometry_instance);
	virtual uint32_t geometry_instance_get_pair_mask();

	/* Lights matched up to Geometry Instances */
	virtual uint32_t get_max_lights_total();
	virtual uint32_t get_max_lights_per_mesh();

	/* PIPELINES */

	virtual void mesh_generate_pipelines(RID p_mesh, bool p_background_compilation);
	virtual uint32_t get_pipeline_compilations(RSE::PipelineSource p_source);

	/* SDFGI UPDATE */

	virtual void sdfgi_update(const Ref<RenderSceneBuffers>& p_render_buffers, RID p_environment,
		const Vector3& p_world_position);
	virtual int sdfgi_get_pending_region_count(
		const Ref<RenderSceneBuffers>& p_render_buffers) const;
	virtual AABB sdfgi_get_pending_region_bounds(
		const Ref<RenderSceneBuffers>& p_render_buffers, int p_region) const;
	virtual uint32_t sdfgi_get_pending_region_cascade(
		const Ref<RenderSceneBuffers>& p_render_buffers, int p_region) const;

	/* SKY API */

	virtual RID sky_allocate();
	virtual void sky_initialize(RID p_rid);

	virtual void sky_set_radiance_size(RID p_sky, int p_radiance_size);
	virtual void sky_set_mode(RID p_sky, RSE::SkyMode p_samples);
	virtual void sky_set_material(RID p_sky, RID p_material);
	virtual Ref<Image> sky_bake_panorama(
		RID p_sky, float p_energy, bool p_bake_irradiance, const Size2i& p_size);

	/* COMPOSITOR EFFECT API */

	static RID compositor_effect_allocate();
	static void compositor_effect_initialize(RID p_rid);
	static void compositor_effect_free(RID p_rid);

	static bool is_compositor_effect(RID p_compositor);
	static void compositor_effect_set_enabled(RID p_compositor, bool p_enabled);
	static void compositor_effect_set_flag(
		RID p_compositor, RSE::CompositorEffectFlags p_flag, bool p_set);

	/* COMPOSITOR API */

	static RID compositor_allocate();
	static void compositor_initialize(RID p_rid);
	static void compositor_free(RID p_rid);

	static bool is_compositor(RID p_compositor);

	/* ENVIRONMENT API */

	static RID environment_allocate();
	static void environment_initialize(RID p_rid);
	static void environment_free(RID p_rid);

	static bool is_environment(RID p_env);

	// Background
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

	static RSE::EnvironmentBG environment_get_background(RID p_env);
	static RID environment_get_sky(RID p_env);
	static float environment_get_sky_custom_fov(RID p_env);
	static Basis environment_get_sky_orientation(RID p_env);
	static Color environment_get_bg_color(RID p_env);
	static float environment_get_bg_energy_multiplier(RID p_env);
	static float environment_get_bg_intensity(RID p_env);
	static int environment_get_canvas_max_layer(RID p_env);
	static RSE::EnvironmentAmbientSource environment_get_ambient_source(RID p_env);
	static Color environment_get_ambient_light(RID p_env);
	static float environment_get_ambient_light_energy(RID p_env);
	static float environment_get_ambient_sky_contribution(RID p_env);
	static RSE::EnvironmentReflectionSource environment_get_reflection_source(RID p_env);
	static int environment_get_camera_feed_id(RID p_env);

	// Tonemap
	static void environment_set_tonemap(
		RID p_env, RSE::EnvironmentToneMapper p_tone_mapper, float p_exposure, float p_white);
	static RSE::EnvironmentToneMapper environment_get_tone_mapper(RID p_env);
	static float environment_get_exposure(RID p_env);
	static float environment_get_white(RID p_env, bool p_limit_agx_white, float p_output_max_value);
	static void environment_set_tonemap_agx_contrast(RID p_env, float p_agx_contrast);
	static float environment_get_tonemap_agx_contrast(RID p_env);
	static RendererEnvironmentStorage::TonemapParameters environment_get_tonemap_parameters(
		RID p_env, bool p_limit_agx_white, float p_output_max_value);

	// Fog
	static void environment_set_fog(RID p_env, bool p_enable, const Color& p_light_color,
		float p_light_energy, float p_sun_scatter, float p_density, float p_height,
		float p_height_density, float p_aerial_perspective, float p_sky_affect,
		RSE::EnvironmentFogMode p_mode);
	static bool environment_get_fog_enabled(RID p_env);
	static RSE::EnvironmentFogMode environment_get_fog_mode(RID p_env);
	static Color environment_get_fog_light_color(RID p_env);
	static float environment_get_fog_light_energy(RID p_env);
	static float environment_get_fog_sun_scatter(RID p_env);
	static float environment_get_fog_density(RID p_env);
	static float environment_get_fog_sky_affect(RID p_env);
	static float environment_get_fog_height(RID p_env);
	static float environment_get_fog_height_density(RID p_env);
	static float environment_get_fog_aerial_perspective(RID p_env);

	// Depth Fog
	static void environment_set_fog_depth(RID p_env, float p_curve, float p_begin, float p_end);
	static float environment_get_fog_depth_curve(RID p_env);
	static float environment_get_fog_depth_begin(RID p_env);
	static float environment_get_fog_depth_end(RID p_env);

	// Volumetric Fog
	static void environment_set_volumetric_fog(RID p_env, bool p_enable, float p_density,
		const Color& p_albedo, const Color& p_emission, float p_emission_energy, float p_anisotropy,
		float p_length, float p_detail_spread, float p_gi_inject, bool p_temporal_reprojection,
		float p_temporal_reprojection_amount, float p_ambient_inject, float p_sky_affect);
	static bool environment_get_volumetric_fog_enabled(RID p_env);
	static float environment_get_volumetric_fog_density(RID p_env);
	static Color environment_get_volumetric_fog_scattering(RID p_env);
	static Color environment_get_volumetric_fog_emission(RID p_env);
	static float environment_get_volumetric_fog_emission_energy(RID p_env);
	static float environment_get_volumetric_fog_anisotropy(RID p_env);
	static float environment_get_volumetric_fog_length(RID p_env);
	static float environment_get_volumetric_fog_detail_spread(RID p_env);
	static float environment_get_volumetric_fog_gi_inject(RID p_env);
	static float environment_get_volumetric_fog_sky_affect(RID p_env);
	static bool environment_get_volumetric_fog_temporal_reprojection(RID p_env);
	static float environment_get_volumetric_fog_temporal_reprojection_amount(RID p_env);
	static float environment_get_volumetric_fog_ambient_inject(RID p_env);

	virtual void environment_set_volumetric_fog_volume_size(int p_size, int p_depth);
	virtual void environment_set_volumetric_fog_filter_active(bool p_enable);

	// GLOW
	static void environment_set_glow(RID p_env, bool p_enable, Vector<float> p_levels, float p_intensity,
		float p_strength, float p_mix, float p_bloom_threshold,
		RSE::EnvironmentGlowBlendMode p_blend_mode, float p_hdr_bleed_threshold,
		float p_hdr_bleed_scale, float p_hdr_luminance_cap, float p_glow_map_strength,
		RID p_glow_map);
	static bool environment_get_glow_enabled(RID p_env);
	static Vector<float> environment_get_glow_levels(RID p_env);
	static float environment_get_glow_intensity(RID p_env);
	static float environment_get_glow_strength(RID p_env);
	static float environment_get_glow_bloom(RID p_env);
	static float environment_get_glow_mix(RID p_env);
	static RSE::EnvironmentGlowBlendMode environment_get_glow_blend_mode(RID p_env);
	static float environment_get_glow_hdr_bleed_threshold(RID p_env);
	static float environment_get_glow_hdr_luminance_cap(RID p_env);
	static float environment_get_glow_hdr_bleed_scale(RID p_env);
	static float environment_get_glow_map_strength(RID p_env);
	static RID environment_get_glow_map(RID p_env);

	virtual void environment_glow_set_use_bicubic_upscale(bool p_enable);

	// SSR
	static void environment_set_ssr(RID p_env, bool p_enable, int p_max_steps, float p_fade_int,
		float p_fade_out, float p_depth_tolerance);
	static bool environment_get_ssr_enabled(RID p_env);
	static int environment_get_ssr_max_steps(RID p_env);
	static float environment_get_ssr_fade_in(RID p_env);
	static float environment_get_ssr_fade_out(RID p_env);
	static float environment_get_ssr_depth_tolerance(RID p_env);

	virtual void environment_set_ssr_half_size(bool p_half_size);
	virtual void environment_set_ssr_roughness_quality(
		RSE::EnvironmentSSRRoughnessQuality p_quality);

	// SSAO
	static void environment_set_ssao(RID p_env, bool p_enable, float p_radius, float p_intensity,
		float p_power, float p_detail, float p_horizon, float p_sharpness, float p_light_affect,
		float p_ao_channel_affect);
	static bool environment_get_ssao_enabled(RID p_env);
	static float environment_get_ssao_radius(RID p_env);
	static float environment_get_ssao_intensity(RID p_env);
	static float environment_get_ssao_power(RID p_env);
	static float environment_get_ssao_detail(RID p_env);
	static float environment_get_ssao_horizon(RID p_env);
	static float environment_get_ssao_sharpness(RID p_env);
	static float environment_get_ssao_direct_light_affect(RID p_env);
	static float environment_get_ssao_ao_channel_affect(RID p_env);

	virtual void environment_set_ssao_quality(RSE::EnvironmentSSAOQuality p_quality,
		bool p_half_size, float p_adaptive_target, int p_blur_passes, float p_fadeout_from,
		float p_fadeout_to);

	// SSIL
	static void environment_set_ssil(RID p_env, bool p_enable, float p_radius, float p_intensity,
		float p_sharpness, float p_normal_rejection);
	static bool environment_get_ssil_enabled(RID p_env);
	static float environment_get_ssil_radius(RID p_env);
	static float environment_get_ssil_intensity(RID p_env);
	static float environment_get_ssil_sharpness(RID p_env);
	static float environment_get_ssil_normal_rejection(RID p_env);

	virtual void environment_set_ssil_quality(RSE::EnvironmentSSILQuality p_quality,
		bool p_half_size, float p_adaptive_target, int p_blur_passes, float p_fadeout_from,
		float p_fadeout_to);

	// SDFGI
	static void environment_set_sdfgi(RID p_env, bool p_enable, int p_cascades, float p_min_cell_size,
		RSE::EnvironmentSDFGIYScale p_y_scale, bool p_use_occlusion, float p_bounce_feedback,
		bool p_read_sky, float p_energy, float p_normal_bias, float p_probe_bias);
	static bool environment_get_sdfgi_enabled(RID p_env);
	static int environment_get_sdfgi_cascades(RID p_env);
	static float environment_get_sdfgi_min_cell_size(RID p_env);
	static bool environment_get_sdfgi_use_occlusion(RID p_env);
	static float environment_get_sdfgi_bounce_feedback(RID p_env);
	static bool environment_get_sdfgi_read_sky_light(RID p_env);
	static float environment_get_sdfgi_energy(RID p_env);
	static float environment_get_sdfgi_normal_bias(RID p_env);
	static float environment_get_sdfgi_probe_bias(RID p_env);
	static RSE::EnvironmentSDFGIYScale environment_get_sdfgi_y_scale(RID p_env);

	virtual void environment_set_sdfgi_ray_count(RSE::EnvironmentSDFGIRayCount p_ray_count);
	virtual void environment_set_sdfgi_frames_to_converge(
		RSE::EnvironmentSDFGIFramesToConverge p_frames);
	virtual void environment_set_sdfgi_frames_to_update_light(
		RSE::EnvironmentSDFGIFramesToUpdateLight p_update);

	// Adjustment
	static void environment_set_adjustment(RID p_env, bool p_enable, float p_brightness, float p_contrast,
		float p_saturation, bool p_use_1d_color_correction, RID p_color_correction);
	static bool environment_get_adjustments_enabled(RID p_env);
	static float environment_get_adjustments_brightness(RID p_env);
	static float environment_get_adjustments_contrast(RID p_env);
	static float environment_get_adjustments_saturation(RID p_env);
	static bool environment_get_use_1d_color_correction(RID p_env);
	static RID environment_get_color_correction(RID p_env);

	virtual Ref<Image> environment_bake_panorama(
		RID p_env, bool p_bake_irradiance, const Size2i& p_size);

	virtual void positional_soft_shadow_filter_set_quality(RSE::ShadowQuality p_quality);
	virtual void directional_soft_shadow_filter_set_quality(RSE::ShadowQuality p_quality);

	virtual RID fog_volume_instance_create(RID p_fog_volume);
	virtual void fog_volume_instance_set_transform(
		RID p_fog_volume_instance, const Transform3D& p_transform);
	virtual void fog_volume_instance_set_active(RID p_fog_volume_instance, bool p_active);
	virtual RID fog_volume_instance_get_volume(RID p_fog_volume_instance) const;
	virtual Vector3 fog_volume_instance_get_position(RID p_fog_volume_instance) const;

	virtual RID voxel_gi_instance_create(RID p_voxel_gi);
	virtual void voxel_gi_instance_set_transform_to_data(
		RID p_probe, const Transform3D& p_xform);
	virtual bool voxel_gi_needs_update(RID p_probe) const;
	virtual void voxel_gi_update(RID p_probe, bool p_update_light_instances,
		const Vector<RID>& p_light_instances,
		const PagedArray<RenderGeometryInstance*>& p_dynamic_objects);

	virtual void voxel_gi_set_quality(RSE::VoxelGIQuality);

	struct RenderShadowData
	{
		RID light;
		int pass = 0;
		PagedArray<RenderGeometryInstance*> instances;
	};

	struct RenderSDFGIData
	{
		int region = 0;
		PagedArray<RenderGeometryInstance*> instances;
	};

	struct RenderSDFGIUpdateData
	{
		bool update_static = false;
		uint32_t static_cascade_count;
		uint32_t* static_cascade_indices = nullptr;
		PagedArray<RID>* static_positional_lights;

		const Vector<RID>* directional_lights;
		const RID* positional_light_instances;
		uint32_t positional_light_count;
	};

	struct CameraData
	{
		// flags
		uint32_t view_count;
		bool is_orthogonal;
		uint32_t visible_layers;
		bool vaspect;

		// Main/center projection
		Transform3D main_transform;
		Projection main_projection;

		Transform3D view_offset[RendererSceneRender::MAX_RENDER_VIEWS];
		Projection view_projection[RendererSceneRender::MAX_RENDER_VIEWS];
		Vector2 taa_jitter;
		float taa_frame_count = 0.0f;

		void set_camera(const Transform3D p_transform, const Projection p_projection,
			bool p_is_orthogonal, bool p_vaspect, const Vector2& p_taa_jitter = Vector2(),
			float p_taa_frame_count = 0.0f, uint32_t p_visible_layers = 0xFFFFFFFF);
		void set_multiview_camera(uint32_t p_view_count, const Transform3D* p_transforms,
			const Projection* p_projections, bool p_is_orthogonal, bool p_vaspect,
			uint32_t p_visible_layers = 0xFFFFFFFF);
	};

	virtual void render_scene(const Ref<RenderSceneBuffers>& p_render_buffers,
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

	virtual void render_material(const Transform3D& p_cam_transform,
		const Projection& p_cam_projection, bool p_cam_orthogonal,
		const PagedArray<RenderGeometryInstance*>& p_instances, RID p_framebuffer,
		const Rect2i& p_region);
	virtual void render_particle_collider_heightfield(RID p_collider,
		const Transform3D& p_transform, const PagedArray<RenderGeometryInstance*>& p_instances);

	virtual void set_scene_pass(uint64_t p_pass);
	virtual void set_time(double p_time, double p_step);
	virtual void set_debug_draw_mode(RSE::ViewportDebugDraw p_debug_draw);

	virtual Ref<RenderSceneBuffers> render_buffers_create();
	virtual void gi_set_use_half_resolution(bool p_enable);

	virtual void screen_space_roughness_limiter_set_active(
		bool p_enable, float p_amount, float p_limit);
	static bool screen_space_roughness_limiter_is_active();

	virtual void sub_surface_scattering_set_quality(RSE::SubSurfaceScatteringQuality p_quality);
	virtual void sub_surface_scattering_set_scale(float p_scale, float p_depth_scale);

	virtual bool free(RID p_rid);

	virtual void sdfgi_set_debug_probe_select(const Vector3& p_position, const Vector3& p_dir);

	virtual void decals_set_filter(RSE::DecalFilter p_filter);
	virtual void light_projectors_set_filter(RSE::LightProjectorFilter p_filter);
	virtual void lightmaps_set_bicubic_filter(bool p_enable);
	virtual void material_set_use_debanding(bool p_enable);

	virtual void update();

	virtual ~RendererSceneRender() {
	    printf("[LIFECYCLE 2] ~RendererSceneRender running on %p, vptr=%p\n", (void*)this, *(void**)this);
	    fflush(stdout);
	}
};

// --- Folded from servers/rendering/storage/variant_converters.h ---
#include <initializer_list>
#include <type_traits>
#include "core/error/error_macros.h"
#include "core/types.h"

template <typename T> struct VariantConverterStd140
{
	// Generic base template for all Vector2/3/4(i) classes.
	static constexpr int Elements = T::AXIS_COUNT;

	template <typename P> static void convert(const T& p_v, P* p_write, bool p_compact)
	{
		for (int i = 0; i < Elements; i++) {
			p_write[i] = p_v[i];
		}
	}
};

template <> struct VariantConverterStd140<float>
{
	static constexpr int Elements = 1;

	template <typename P> static void convert(float p_v, P* p_write, bool p_compact)
	{
		p_write[0] = p_v;
	}
};

template <> struct VariantConverterStd140<int32_t>
{
	static constexpr int Elements = 1;

	template <typename P> static void convert(int32_t p_v, P* p_write, bool p_compact)
	{
		p_write[0] = p_v;
	}
};

template <> struct VariantConverterStd140<uint32_t>
{
	static constexpr int Elements = 1;

	template <typename P> static void convert(uint32_t p_v, P* p_write, bool p_compact)
	{
		p_write[0] = p_v;
	}
};

template <> struct VariantConverterStd140<Basis>
{
	static constexpr int Elements = 9;

	template <typename P> static void convert(const Basis& p_v, P* p_write, bool p_compact)
	{
		// Basis can have compact 9 floats or std140 layout 12 floats.
		int i = 0;

		p_write[i++] = p_v.rows[0][0];
		p_write[i++] = p_v.rows[1][0];
		p_write[i++] = p_v.rows[2][0];
		if (!p_compact) {
			p_write[i++] = 0;
		}

		p_write[i++] = p_v.rows[0][1];
		p_write[i++] = p_v.rows[1][1];
		p_write[i++] = p_v.rows[2][1];
		if (!p_compact) {
			p_write[i++] = 0;
		}

		p_write[i++] = p_v.rows[0][2];
		p_write[i++] = p_v.rows[1][2];
		p_write[i++] = p_v.rows[2][2];
		if (!p_compact) {
			p_write[i++] = 0;
		}
	}
};

template <> struct VariantConverterStd140<Transform2D>
{
	static constexpr int Elements = 12;

	template <typename P> static void convert(const Transform2D& p_v, P* p_write, bool p_compact)
	{
		p_write[0] = p_v.columns[0][0];
		p_write[1] = p_v.columns[0][1];
		p_write[2] = 0;
		p_write[3] = 0;

		p_write[4] = p_v.columns[1][0];
		p_write[5] = p_v.columns[1][1];
		p_write[6] = 0;
		p_write[7] = 0;

		p_write[8] = p_v.columns[2][0];
		p_write[9] = p_v.columns[2][1];
		p_write[10] = 1;
		p_write[11] = 0;
	}
};

template <> struct VariantConverterStd140<Transform3D>
{
	static constexpr int Elements = 16;

	template <typename P> static void convert(const Transform3D& p_v, P* p_write, bool p_compact)
	{
		p_write[0] = p_v.basis.rows[0][0];
		p_write[1] = p_v.basis.rows[1][0];
		p_write[2] = p_v.basis.rows[2][0];
		p_write[3] = 0;

		p_write[4] = p_v.basis.rows[0][1];
		p_write[5] = p_v.basis.rows[1][1];
		p_write[6] = p_v.basis.rows[2][1];
		p_write[7] = 0;

		p_write[8] = p_v.basis.rows[0][2];
		p_write[9] = p_v.basis.rows[1][2];
		p_write[10] = p_v.basis.rows[2][2];
		p_write[11] = 0;

		p_write[12] = p_v.origin.x;
		p_write[13] = p_v.origin.y;
		p_write[14] = p_v.origin.z;
		p_write[15] = 1;
	}
};

template <> struct VariantConverterStd140<Projection>
{
	static constexpr int Elements = 16;

	template <typename P> static void convert(const Projection& p_v, P* p_write, bool p_compact)
	{
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				p_write[i * 4 + j] = p_v.columns[i][j];
			}
		}
	}
};

template <typename T, typename P> T construct_vector(const std::initializer_list<P>& values)
{
	T vector{};
	int index = 0;
	for (P v : values) {
		vector[index++] = v;
		if (index >= T::AXIS_COUNT) {
			break;
		}
	}
	return vector;
}

template <typename, typename = void> inline constexpr bool is_vector_type_v = false;

template <typename T>
inline constexpr bool is_vector_type_v<T, std::void_t<decltype(T::AXIS_COUNT)>> = true;

template <typename T, typename P>
void convert_item_std140(const T& p_item, P* p_write, bool p_compact = false)
{
	VariantConverterStd140<T>::template convert<P>(p_item, p_write, p_compact);
}

template <typename T, typename From, typename To>
void write_array_std140(const Vector<From>& p_values, To* p_write, int p_array_size, int p_stride)
{
	constexpr int elements = VariantConverterStd140<T>::Elements;
	const int src_count = p_values.size();
	const int dst_count = elements * p_array_size;
	const int stride_count = p_stride * p_array_size;
	const From* read = p_values.ptr();
	const T default_value{};

	memset(p_write, 0, sizeof(To) * stride_count);

	for (int i = 0, j = 0; i < dst_count; i += elements, j += p_stride) {
		if (i + elements - 1 < src_count) {
			// Only copy full items with all elements, no partial or missing data.
			for (int e = 0; e < elements; e++) {
				DEV_ASSERT(j + e < stride_count && i + e < src_count);
				p_write[j + e] = read[i + e];
			}
		}
		else {
			// If not enough source data was passed in, write default values.
			convert_item_std140(default_value, p_write + j);
		}
	}
}
