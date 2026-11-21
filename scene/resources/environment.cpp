/**************************************************************************/
/*  environment.cpp                                                       */
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

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/os/os.h"
#include "environment.h"
#include "scene/resources/gradient_texture.h"
#include "scene/resources/sky.h"
#include "servers/rendering/renderer.h"

RID Environment::get_rid() const { return environment; }

Environment::BGMode Environment::get_background() const { return bg_mode; }

void Environment::set_sky(const Ref<Sky>& p_sky)
{
	bg_sky = p_sky;
	RID sb_rid;
	if (bg_sky.is_valid()) {
		sb_rid = bg_sky->get_rid();
	}
	Renderer::environment_set_sky(environment, sb_rid);
}

Ref<Sky> Environment::get_sky() const { return bg_sky; }

void Environment::set_sky_custom_fov(float p_scale)
{
	bg_sky_custom_fov = p_scale;
	Renderer::environment_set_sky_custom_fov(environment, p_scale);
}

float Environment::get_sky_custom_fov() const { return bg_sky_custom_fov; }

void Environment::set_sky_rotation(const Vector3& p_rotation)
{
	bg_sky_rotation = p_rotation;
	Renderer::environment_set_sky_orientation(
		environment, Basis::from_euler(p_rotation));
}

Vector3 Environment::get_sky_rotation() const { return bg_sky_rotation; }

void Environment::set_bg_color(const Color& p_color)
{
	bg_color = p_color;
	Renderer::environment_set_bg_color(environment, p_color);
}

Color Environment::get_bg_color() const { return bg_color; }

void Environment::set_bg_energy_multiplier(float p_multiplier)
{
	bg_energy_multiplier = p_multiplier;
	_update_bg_energy();
}

float Environment::get_bg_energy_multiplier() const { return bg_energy_multiplier; }

void Environment::set_bg_intensity(float p_exposure_value)
{
	bg_intensity = p_exposure_value;
	_update_bg_energy();
}

float Environment::get_bg_intensity() const { return bg_intensity; }

void Environment::_update_bg_energy()
{
	if (GLOBAL_GET_CACHED(bool, "rendering/lights_and_shadows/use_physical_light_units")) {
		Renderer::environment_set_bg_energy(
			environment, bg_energy_multiplier, bg_intensity);
	}
	else {
		Renderer::environment_set_bg_energy(environment, bg_energy_multiplier, 1.0);
	}
}

void Environment::set_canvas_max_layer(int p_max_layer)
{
	bg_canvas_max_layer = p_max_layer;
	Renderer::environment_set_canvas_max_layer(environment, p_max_layer);
}

int Environment::get_canvas_max_layer() const { return bg_canvas_max_layer; }

void Environment::set_camera_feed_id(int p_id)
{
	bg_camera_feed_id = p_id;
	Renderer::environment_set_camera_feed_id(environment, bg_camera_feed_id);
}

int Environment::get_camera_feed_id() const { return bg_camera_feed_id; }


void Environment::set_ambient_light_color(const Color& p_color)
{
	ambient_color = p_color;
	_update_ambient_light();
}

Color Environment::get_ambient_light_color() const { return ambient_color; }

Environment::AmbientSource Environment::get_ambient_source() const { return ambient_source; }

void Environment::set_ambient_light_energy(float p_energy)
{
	ambient_energy = p_energy;
	_update_ambient_light();
}

float Environment::get_ambient_light_energy() const { return ambient_energy; }

void Environment::set_ambient_light_sky_contribution(float p_ratio)
{
	// Sky contribution values outside the [0.0; 1.0] range don't make sense and
	// can result in negative colors.
	ambient_sky_contribution = CLAMP(p_ratio, 0.0, 1.0);
	_update_ambient_light();
}

float Environment::get_ambient_light_sky_contribution() const { return ambient_sky_contribution; }

Environment::ReflectionSource Environment::get_reflection_source() const
{
	return reflection_source;
}

void Environment::_update_ambient_light()
{
	Renderer::environment_set_ambient_light(environment, ambient_color,
		RSE::EnvironmentAmbientSource(ambient_source), ambient_energy, ambient_sky_contribution,
		RSE::EnvironmentReflectionSource(reflection_source));
}

Environment::ToneMapper Environment::get_tonemapper() const { return tone_mapper; }

void Environment::set_tonemap_exposure(float p_exposure)
{
	tonemap_exposure = p_exposure;
	_update_tonemap();
}

float Environment::get_tonemap_exposure() const { return tonemap_exposure; }

void Environment::set_tonemap_white(float p_white)
{
	tonemap_white = p_white;
	_update_tonemap();
}

float Environment::get_tonemap_white() const { return tonemap_white; }

void Environment::set_tonemap_agx_white(float p_white)
{
	tonemap_agx_white = p_white;
	_update_tonemap();
}

float Environment::get_tonemap_agx_white() const { return tonemap_agx_white; }

void Environment::set_tonemap_agx_contrast(float p_agx_contrast)
{
	tonemap_agx_contrast = p_agx_contrast;
	Renderer::environment_set_tonemap_agx_contrast(environment, p_agx_contrast);
}

float Environment::get_tonemap_agx_contrast() const { return tonemap_agx_contrast; }

void Environment::_update_tonemap()
{
	Renderer::environment_set_tonemap(environment,
		RSE::EnvironmentToneMapper(tone_mapper), tonemap_exposure,
		tone_mapper == TONE_MAPPER_AGX ? tonemap_agx_white : tonemap_white);
}


void Environment::set_ssr_enabled(bool p_enabled)
{
	ssr_enabled = p_enabled;
	_update_ssr();
}

bool Environment::is_ssr_enabled() const { return ssr_enabled; }

void Environment::set_ssr_max_steps(int p_steps)
{
	ssr_max_steps = p_steps;
	_update_ssr();
}

int Environment::get_ssr_max_steps() const { return ssr_max_steps; }

void Environment::set_ssr_fade_in(float p_fade_in)
{
	ssr_fade_in = MAX(p_fade_in, 0.0f);
	_update_ssr();
}

float Environment::get_ssr_fade_in() const { return ssr_fade_in; }

void Environment::set_ssr_fade_out(float p_fade_out)
{
	ssr_fade_out = MAX(p_fade_out, 0.0f);
	_update_ssr();
}

float Environment::get_ssr_fade_out() const { return ssr_fade_out; }

void Environment::set_ssr_depth_tolerance(float p_depth_tolerance)
{
	ssr_depth_tolerance = p_depth_tolerance;
	_update_ssr();
}

float Environment::get_ssr_depth_tolerance() const { return ssr_depth_tolerance; }

void Environment::_update_ssr()
{
	Renderer::environment_set_ssr(
		environment, ssr_enabled, ssr_max_steps, ssr_fade_in, ssr_fade_out, ssr_depth_tolerance);
}


void Environment::set_ssao_enabled(bool p_enabled)
{
	ssao_enabled = p_enabled;
	_update_ssao();
}

bool Environment::is_ssao_enabled() const { return ssao_enabled; }

void Environment::set_ssao_radius(float p_radius)
{
	ssao_radius = p_radius;
	_update_ssao();
}

float Environment::get_ssao_radius() const { return ssao_radius; }

void Environment::set_ssao_intensity(float p_intensity)
{
	ssao_intensity = p_intensity;
	_update_ssao();
}

float Environment::get_ssao_intensity() const { return ssao_intensity; }

void Environment::set_ssao_power(float p_power)
{
	ssao_power = p_power;
	_update_ssao();
}

float Environment::get_ssao_power() const { return ssao_power; }

void Environment::set_ssao_detail(float p_detail)
{
	ssao_detail = p_detail;
	_update_ssao();
}

float Environment::get_ssao_detail() const { return ssao_detail; }

void Environment::set_ssao_horizon(float p_horizon)
{
	ssao_horizon = p_horizon;
	_update_ssao();
}

float Environment::get_ssao_horizon() const { return ssao_horizon; }

void Environment::set_ssao_sharpness(float p_sharpness)
{
	ssao_sharpness = p_sharpness;
	_update_ssao();
}

float Environment::get_ssao_sharpness() const { return ssao_sharpness; }

void Environment::set_ssao_direct_light_affect(float p_direct_light_affect)
{
	ssao_direct_light_affect = p_direct_light_affect;
	_update_ssao();
}

float Environment::get_ssao_direct_light_affect() const { return ssao_direct_light_affect; }

void Environment::set_ssao_ao_channel_affect(float p_ao_channel_affect)
{
	ssao_ao_channel_affect = p_ao_channel_affect;
	_update_ssao();
}

float Environment::get_ssao_ao_channel_affect() const { return ssao_ao_channel_affect; }

void Environment::_update_ssao()
{
	Renderer::environment_set_ssao(environment, ssao_enabled, ssao_radius,
		ssao_intensity, ssao_power, ssao_detail, ssao_horizon, ssao_sharpness,
		ssao_direct_light_affect, ssao_ao_channel_affect);
}


void Environment::set_ssil_enabled(bool p_enabled)
{
	ssil_enabled = p_enabled;
	_update_ssil();
}

bool Environment::is_ssil_enabled() const { return ssil_enabled; }

void Environment::set_ssil_radius(float p_radius)
{
	ssil_radius = p_radius;
	_update_ssil();
}

float Environment::get_ssil_radius() const { return ssil_radius; }

void Environment::set_ssil_intensity(float p_intensity)
{
	ssil_intensity = p_intensity;
	_update_ssil();
}

float Environment::get_ssil_intensity() const { return ssil_intensity; }

void Environment::set_ssil_sharpness(float p_sharpness)
{
	ssil_sharpness = p_sharpness;
	_update_ssil();
}

float Environment::get_ssil_sharpness() const { return ssil_sharpness; }

void Environment::set_ssil_normal_rejection(float p_normal_rejection)
{
	ssil_normal_rejection = p_normal_rejection;
	_update_ssil();
}

float Environment::get_ssil_normal_rejection() const { return ssil_normal_rejection; }

void Environment::_update_ssil()
{
	Renderer::environment_set_ssil(environment, ssil_enabled, ssil_radius,
		ssil_intensity, ssil_sharpness, ssil_normal_rejection);
}


void Environment::set_sdfgi_enabled(bool p_enabled)
{
	sdfgi_enabled = p_enabled;
	_update_sdfgi();
}

bool Environment::is_sdfgi_enabled() const { return sdfgi_enabled; }

void Environment::set_sdfgi_cascades(int p_cascades)
{
	ERR_FAIL_COND_MSG(p_cascades < 1 || p_cascades > 8,
		"Invalid number of SDFGI cascades (must be between 1 and 8).");
	sdfgi_cascades = p_cascades;
	_update_sdfgi();
}

int Environment::get_sdfgi_cascades() const { return sdfgi_cascades; }

void Environment::set_sdfgi_min_cell_size(float p_size)
{
	sdfgi_min_cell_size = p_size;
	_update_sdfgi();
}

float Environment::get_sdfgi_min_cell_size() const { return sdfgi_min_cell_size; }

void Environment::set_sdfgi_max_distance(float p_distance)
{
	p_distance /= 64.0;
	for (int i = 0; i < sdfgi_cascades; i++) {
		p_distance *= 0.5; // halve for each cascade
	}
	sdfgi_min_cell_size = p_distance;
	_update_sdfgi();
}

float Environment::get_sdfgi_max_distance() const
{
	float md = sdfgi_min_cell_size;
	md *= 64.0;
	for (int i = 0; i < sdfgi_cascades; i++) {
		md *= 2.0;
	}
	return md;
}

void Environment::set_sdfgi_cascade0_distance(float p_distance)
{
	sdfgi_min_cell_size = p_distance / 64.0;
	_update_sdfgi();
}

float Environment::get_sdfgi_cascade0_distance() const { return sdfgi_min_cell_size * 64.0; }

void Environment::set_sdfgi_y_scale(SDFGIYScale p_y_scale)
{
	sdfgi_y_scale = p_y_scale;
	_update_sdfgi();
}

Environment::SDFGIYScale Environment::get_sdfgi_y_scale() const { return sdfgi_y_scale; }

void Environment::set_sdfgi_use_occlusion(bool p_enabled)
{
	sdfgi_use_occlusion = p_enabled;
	_update_sdfgi();
}

bool Environment::is_sdfgi_using_occlusion() const { return sdfgi_use_occlusion; }

void Environment::set_sdfgi_bounce_feedback(float p_amount)
{
	sdfgi_bounce_feedback = p_amount;
	_update_sdfgi();
}

float Environment::get_sdfgi_bounce_feedback() const { return sdfgi_bounce_feedback; }

void Environment::set_sdfgi_read_sky_light(bool p_enabled)
{
	sdfgi_read_sky_light = p_enabled;
	_update_sdfgi();
}

bool Environment::is_sdfgi_reading_sky_light() const { return sdfgi_read_sky_light; }

void Environment::set_sdfgi_energy(float p_energy)
{
	sdfgi_energy = p_energy;
	_update_sdfgi();
}

float Environment::get_sdfgi_energy() const { return sdfgi_energy; }

void Environment::set_sdfgi_normal_bias(float p_bias)
{
	sdfgi_normal_bias = p_bias;
	_update_sdfgi();
}

float Environment::get_sdfgi_normal_bias() const { return sdfgi_normal_bias; }

void Environment::set_sdfgi_probe_bias(float p_bias)
{
	sdfgi_probe_bias = p_bias;
	_update_sdfgi();
}

float Environment::get_sdfgi_probe_bias() const { return sdfgi_probe_bias; }

void Environment::_update_sdfgi()
{
	Renderer::environment_set_sdfgi(environment, sdfgi_enabled, sdfgi_cascades,
		sdfgi_min_cell_size, RSE::EnvironmentSDFGIYScale(sdfgi_y_scale), sdfgi_use_occlusion,
		sdfgi_bounce_feedback, sdfgi_read_sky_light, sdfgi_energy, sdfgi_normal_bias,
		sdfgi_probe_bias);
}


void Environment::set_glow_enabled(bool p_enabled)
{
	glow_enabled = p_enabled;
	_update_glow();
}

bool Environment::is_glow_enabled() const { return glow_enabled; }

void Environment::set_glow_level(int p_level, float p_intensity)
{
	ERR_FAIL_INDEX(p_level, RSE::MAX_GLOW_LEVELS);

	glow_levels.write[p_level] = p_intensity;

	_update_glow();
}

float Environment::get_glow_level(int p_level) const
{
	ERR_FAIL_INDEX_V(p_level, RSE::MAX_GLOW_LEVELS, 0.0);

	return glow_levels[p_level];
}

void Environment::set_glow_normalized(bool p_normalized)
{
	glow_normalize_levels = p_normalized;

	_update_glow();
}

bool Environment::is_glow_normalized() const { return glow_normalize_levels; }

void Environment::set_glow_intensity(float p_intensity)
{
	glow_intensity = p_intensity;
	_update_glow();
}

float Environment::get_glow_intensity() const { return glow_intensity; }

void Environment::set_glow_strength(float p_strength)
{
	glow_strength = p_strength;
	_update_glow();
}

float Environment::get_glow_strength() const { return glow_strength; }

void Environment::set_glow_mix(float p_mix)
{
	glow_mix = p_mix;
	_update_glow();
}

float Environment::get_glow_mix() const { return glow_mix; }

void Environment::set_glow_bloom(float p_threshold)
{
	glow_bloom = p_threshold;
	_update_glow();
}

float Environment::get_glow_bloom() const { return glow_bloom; }

Environment::GlowBlendMode Environment::get_glow_blend_mode() const { return glow_blend_mode; }

void Environment::set_glow_hdr_bleed_threshold(float p_threshold)
{
	glow_hdr_bleed_threshold = p_threshold;
	_update_glow();
}

float Environment::get_glow_hdr_bleed_threshold() const { return glow_hdr_bleed_threshold; }

void Environment::set_glow_hdr_bleed_scale(float p_scale)
{
	glow_hdr_bleed_scale = p_scale;
	_update_glow();
}

float Environment::get_glow_hdr_bleed_scale() const { return glow_hdr_bleed_scale; }

void Environment::set_glow_hdr_luminance_cap(float p_amount)
{
	glow_hdr_luminance_cap = p_amount;
	_update_glow();
}

float Environment::get_glow_hdr_luminance_cap() const { return glow_hdr_luminance_cap; }

void Environment::set_glow_map_strength(float p_strength)
{
	glow_map_strength = p_strength;
	_update_glow();
}

float Environment::get_glow_map_strength() const { return glow_map_strength; }

void Environment::set_glow_map(Ref<Texture> p_glow_map)
{
	glow_map = p_glow_map;
	_update_glow();
}

Ref<Texture> Environment::get_glow_map() const { return glow_map; }

void Environment::_update_glow()
{
	Vector<float> normalized_levels;
	if (glow_normalize_levels) {
		normalized_levels.resize(7);
		float size = 0.0;
		for (int i = 0; i < glow_levels.size(); i++) {
			size += glow_levels[i];
		}
		for (int i = 0; i < glow_levels.size(); i++) {
			normalized_levels.write[i] = glow_levels[i] / size;
		}
	}
	else {
		normalized_levels = glow_levels;
	}

	float _glow_map_strength = 0.0f;
	RID glow_map_rid;
	if (glow_map.is_valid()) {
		glow_map_rid = glow_map->get_rid();
		_glow_map_strength = glow_map_strength;
	}
	else {
		glow_map_rid = RID();
	}

	Renderer::environment_set_glow(environment, glow_enabled, normalized_levels,
		glow_intensity, glow_strength, glow_mix, glow_bloom,
		RSE::EnvironmentGlowBlendMode(glow_blend_mode), glow_hdr_bleed_threshold,
		glow_hdr_bleed_scale, glow_hdr_luminance_cap, _glow_map_strength, glow_map_rid);
}


void Environment::set_fog_enabled(bool p_enabled)
{
	fog_enabled = p_enabled;
	_update_fog();
}

bool Environment::is_fog_enabled() const { return fog_enabled; }

Environment::FogMode Environment::get_fog_mode() const { return fog_mode; }

void Environment::set_fog_light_color(const Color& p_light_color)
{
	fog_light_color = p_light_color;
	_update_fog();
}

Color Environment::get_fog_light_color() const { return fog_light_color; }

void Environment::set_fog_light_energy(float p_amount)
{
	fog_light_energy = p_amount;
	_update_fog();
}

float Environment::get_fog_light_energy() const { return fog_light_energy; }

void Environment::set_fog_sun_scatter(float p_amount)
{
	fog_sun_scatter = p_amount;
	_update_fog();
}

float Environment::get_fog_sun_scatter() const { return fog_sun_scatter; }

void Environment::set_fog_density(float p_amount)
{
	fog_density = p_amount;
	_update_fog();
}

float Environment::get_fog_density() const { return fog_density; }

void Environment::set_fog_height(float p_amount)
{
	fog_height = p_amount;
	_update_fog();
}

float Environment::get_fog_height() const { return fog_height; }

void Environment::set_fog_height_density(float p_amount)
{
	fog_height_density = p_amount;
	_update_fog();
}

float Environment::get_fog_height_density() const { return fog_height_density; }

void Environment::set_fog_aerial_perspective(float p_aerial_perspective)
{
	fog_aerial_perspective = p_aerial_perspective;
	_update_fog();
}

float Environment::get_fog_aerial_perspective() const { return fog_aerial_perspective; }

void Environment::set_fog_sky_affect(float p_sky_affect)
{
	fog_sky_affect = p_sky_affect;
	_update_fog();
}

float Environment::get_fog_sky_affect() const { return fog_sky_affect; }

void Environment::_update_fog()
{
	Renderer::environment_set_fog(environment, fog_enabled, fog_light_color,
		fog_light_energy, fog_sun_scatter, fog_density, fog_height, fog_height_density,
		fog_aerial_perspective, fog_sky_affect, RSE::EnvironmentFogMode(fog_mode));
}


void Environment::set_fog_depth_curve(float p_curve)
{
	fog_depth_curve = p_curve;
	_update_fog_depth();
}

float Environment::get_fog_depth_curve() const { return fog_depth_curve; }

void Environment::set_fog_depth_begin(float p_begin)
{
	fog_depth_begin = p_begin;
	if (fog_depth_begin > fog_depth_end) {
		set_fog_depth_end(fog_depth_begin);
	}
	_update_fog_depth();
}

float Environment::get_fog_depth_begin() const { return fog_depth_begin; }

void Environment::set_fog_depth_end(float p_end)
{
	fog_depth_end = p_end;
	if (fog_depth_end < fog_depth_begin) {
		set_fog_depth_begin(fog_depth_end);
	}
	_update_fog_depth();
}

float Environment::get_fog_depth_end() const { return fog_depth_end; }

void Environment::_update_fog_depth()
{
	Renderer::environment_set_fog_depth(
		environment, fog_depth_curve, fog_depth_begin, fog_depth_end);
}


void Environment::_update_volumetric_fog()
{
	Renderer::environment_set_volumetric_fog(environment, volumetric_fog_enabled,
		volumetric_fog_density, volumetric_fog_albedo, volumetric_fog_emission,
		volumetric_fog_emission_energy, volumetric_fog_anisotropy, volumetric_fog_length,
		volumetric_fog_detail_spread, volumetric_fog_gi_inject, volumetric_fog_temporal_reproject,
		volumetric_fog_temporal_reproject_amount, volumetric_fog_ambient_inject,
		volumetric_fog_sky_affect);
}

void Environment::set_volumetric_fog_enabled(bool p_enable)
{
	volumetric_fog_enabled = p_enable;
	_update_volumetric_fog();
}

bool Environment::is_volumetric_fog_enabled() const { return volumetric_fog_enabled; }

void Environment::set_volumetric_fog_density(float p_density)
{
	volumetric_fog_density = p_density;
	_update_volumetric_fog();
}

float Environment::get_volumetric_fog_density() const { return volumetric_fog_density; }

void Environment::set_volumetric_fog_albedo(Color p_color)
{
	volumetric_fog_albedo = p_color;
	_update_volumetric_fog();
}

Color Environment::get_volumetric_fog_albedo() const { return volumetric_fog_albedo; }

void Environment::set_volumetric_fog_emission(Color p_color)
{
	volumetric_fog_emission = p_color;
	_update_volumetric_fog();
}

Color Environment::get_volumetric_fog_emission() const { return volumetric_fog_emission; }

void Environment::set_volumetric_fog_emission_energy(float p_begin)
{
	volumetric_fog_emission_energy = p_begin;
	_update_volumetric_fog();
}

float Environment::get_volumetric_fog_emission_energy() const
{
	return volumetric_fog_emission_energy;
}

void Environment::set_volumetric_fog_anisotropy(float p_anisotropy)
{
	volumetric_fog_anisotropy = p_anisotropy;
	_update_volumetric_fog();
}

float Environment::get_volumetric_fog_anisotropy() const { return volumetric_fog_anisotropy; }

void Environment::set_volumetric_fog_length(float p_length)
{
	volumetric_fog_length = p_length;
	_update_volumetric_fog();
}

float Environment::get_volumetric_fog_length() const { return volumetric_fog_length; }

void Environment::set_volumetric_fog_detail_spread(float p_detail_spread)
{
	p_detail_spread = CLAMP(p_detail_spread, 0.5, 6.0);
	volumetric_fog_detail_spread = p_detail_spread;
	_update_volumetric_fog();
}

float Environment::get_volumetric_fog_detail_spread() const { return volumetric_fog_detail_spread; }

void Environment::set_volumetric_fog_gi_inject(float p_gi_inject)
{
	volumetric_fog_gi_inject = p_gi_inject;
	_update_volumetric_fog();
}

float Environment::get_volumetric_fog_gi_inject() const { return volumetric_fog_gi_inject; }

void Environment::set_volumetric_fog_ambient_inject(float p_ambient_inject)
{
	volumetric_fog_ambient_inject = p_ambient_inject;
	_update_volumetric_fog();
}

float Environment::get_volumetric_fog_ambient_inject() const
{
	return volumetric_fog_ambient_inject;
}

void Environment::set_volumetric_fog_sky_affect(float p_sky_affect)
{
	volumetric_fog_sky_affect = p_sky_affect;
	_update_volumetric_fog();
}

float Environment::get_volumetric_fog_sky_affect() const { return volumetric_fog_sky_affect; }

void Environment::set_volumetric_fog_temporal_reprojection_enabled(bool p_enable)
{
	volumetric_fog_temporal_reproject = p_enable;
	_update_volumetric_fog();
}

bool Environment::is_volumetric_fog_temporal_reprojection_enabled() const
{
	return volumetric_fog_temporal_reproject;
}

void Environment::set_volumetric_fog_temporal_reprojection_amount(float p_amount)
{
	volumetric_fog_temporal_reproject_amount = p_amount;
	_update_volumetric_fog();
}

float Environment::get_volumetric_fog_temporal_reprojection_amount() const
{
	return volumetric_fog_temporal_reproject_amount;
}


void Environment::set_adjustment_enabled(bool p_enabled)
{
	adjustment_enabled = p_enabled;
	_update_adjustment();
}

bool Environment::is_adjustment_enabled() const { return adjustment_enabled; }

void Environment::set_adjustment_brightness(float p_brightness)
{
	adjustment_brightness = p_brightness;
	_update_adjustment();
}

float Environment::get_adjustment_brightness() const { return adjustment_brightness; }

void Environment::set_adjustment_contrast(float p_contrast)
{
	adjustment_contrast = p_contrast;
	_update_adjustment();
}

float Environment::get_adjustment_contrast() const { return adjustment_contrast; }

void Environment::set_adjustment_saturation(float p_saturation)
{
	adjustment_saturation = p_saturation;
	_update_adjustment();
}

float Environment::get_adjustment_saturation() const { return adjustment_saturation; }

Ref<Texture> Environment::get_adjustment_color_correction() const
{
	return adjustment_color_correction;
}

void Environment::_update_adjustment()
{
	RID color_correction =
		adjustment_color_correction.is_valid() ? adjustment_color_correction->get_rid() : RID();

	Renderer::environment_set_adjustment(environment, adjustment_enabled,
		adjustment_brightness, adjustment_contrast, adjustment_saturation, use_1d_color_correction,
		color_correction);
}

Environment::~Environment()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(environment);
}



void Environment::set_tonemapper(Environment::ToneMapper) {}

// --- camera_attributes.cpp ---

#include "camera_attributes.h"
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/os/os.h"
#include "servers/rendering/renderer.h"

void CameraAttributes::set_exposure_multiplier(float p_multiplier)
{
	exposure_multiplier = p_multiplier;
	_update_exposure();
	emit_changed();
}

float CameraAttributes::get_exposure_multiplier() const { return exposure_multiplier; }

void CameraAttributes::set_exposure_sensitivity(float p_sensitivity)
{
	exposure_sensitivity = p_sensitivity;
	_update_exposure();
	emit_changed();
}

float CameraAttributes::get_exposure_sensitivity() const { return exposure_sensitivity; }

void CameraAttributes::_update_exposure()
{
	float exposure_normalization = 1.0;
	// Ignore physical properties if not using physical light units.
	if (GLOBAL_GET_CACHED(bool, "rendering/lights_and_shadows/use_physical_light_units")) {
		exposure_normalization = calculate_exposure_normalization();
	}

	Renderer::camera_attributes_set_exposure(
		camera_attributes, exposure_multiplier, exposure_normalization);
}

bool CameraAttributes::is_auto_exposure_enabled() const { return auto_exposure_enabled; }

void CameraAttributes::set_auto_exposure_speed(float p_auto_exposure_speed)
{
	auto_exposure_speed = p_auto_exposure_speed;
	_update_auto_exposure();
}

float CameraAttributes::get_auto_exposure_speed() const { return auto_exposure_speed; }

void CameraAttributes::set_auto_exposure_scale(float p_auto_exposure_scale)
{
	auto_exposure_scale = p_auto_exposure_scale;
	_update_auto_exposure();
}

float CameraAttributes::get_auto_exposure_scale() const { return auto_exposure_scale; }

RID CameraAttributes::get_rid() const { return camera_attributes; }


CameraAttributes::CameraAttributes()
{
	camera_attributes = Renderer::camera_attributes_create();
}

CameraAttributes::~CameraAttributes()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(camera_attributes);
}

//////////////////////////////////////////////////////
/* CameraAttributesPractical */

bool CameraAttributesPractical::is_dof_blur_far_enabled() const { return dof_blur_far_enabled; }

void CameraAttributesPractical::set_dof_blur_far_distance(float p_distance)
{
	dof_blur_far_distance = p_distance;
	_update_dof_blur();
}

float CameraAttributesPractical::get_dof_blur_far_distance() const { return dof_blur_far_distance; }

void CameraAttributesPractical::set_dof_blur_far_transition(float p_distance)
{
	dof_blur_far_transition = p_distance;
	_update_dof_blur();
}

float CameraAttributesPractical::get_dof_blur_far_transition() const
{
	return dof_blur_far_transition;
}

bool CameraAttributesPractical::is_dof_blur_near_enabled() const { return dof_blur_near_enabled; }

void CameraAttributesPractical::set_dof_blur_near_distance(float p_distance)
{
	dof_blur_near_distance = p_distance;
	_update_dof_blur();
}

float CameraAttributesPractical::get_dof_blur_near_distance() const
{
	return dof_blur_near_distance;
}

void CameraAttributesPractical::set_dof_blur_near_transition(float p_distance)
{
	dof_blur_near_transition = p_distance;
	_update_dof_blur();
}

float CameraAttributesPractical::get_dof_blur_near_transition() const
{
	return dof_blur_near_transition;
}

void CameraAttributesPractical::set_dof_blur_amount(float p_amount)
{
	dof_blur_amount = p_amount;
	_update_dof_blur();
}

float CameraAttributesPractical::get_dof_blur_amount() const { return dof_blur_amount; }

void CameraAttributesPractical::_update_dof_blur()
{
	Renderer::camera_attributes_set_dof_blur(get_rid(), dof_blur_far_enabled,
		dof_blur_far_distance, dof_blur_far_transition, dof_blur_near_enabled,
		dof_blur_near_distance, dof_blur_near_transition, dof_blur_amount);
}

float CameraAttributesPractical::calculate_exposure_normalization() const
{
	return exposure_sensitivity / 3072007.0; // Matches exposure normalization for default
											 // CameraAttributesPhysical at ISO 100.
}

void CameraAttributesPractical::set_auto_exposure_min_sensitivity(float p_min)
{
	auto_exposure_min = p_min;
	_update_auto_exposure();
}

float CameraAttributesPractical::get_auto_exposure_min_sensitivity() const
{
	return auto_exposure_min;
}

void CameraAttributesPractical::set_auto_exposure_max_sensitivity(float p_max)
{
	auto_exposure_max = p_max;
	_update_auto_exposure();
}

float CameraAttributesPractical::get_auto_exposure_max_sensitivity() const
{
	return auto_exposure_max;
}

void CameraAttributesPractical::_update_auto_exposure()
{
	Renderer::camera_attributes_set_auto_exposure(get_rid(), auto_exposure_enabled,
		auto_exposure_min *
			((12.5 / 100.0) / exposure_sensitivity), // Convert from Sensitivity to Luminance
		auto_exposure_max *
			((12.5 / 100.0) / exposure_sensitivity), // Convert from Sensitivity to Luminance
		auto_exposure_speed, auto_exposure_scale);
	emit_changed();
}

CameraAttributesPractical::~CameraAttributesPractical() {}

//////////////////////////////////////////////////////
/* CameraAttributesPhysical */

void CameraAttributesPhysical::set_aperture(float p_aperture)
{
	exposure_aperture = p_aperture;
	_update_exposure();
	_update_frustum();
}

float CameraAttributesPhysical::get_aperture() const { return exposure_aperture; }

void CameraAttributesPhysical::set_shutter_speed(float p_shutter_speed)
{
	exposure_shutter_speed = p_shutter_speed;
	_update_exposure();
}

float CameraAttributesPhysical::get_shutter_speed() const { return exposure_shutter_speed; }

void CameraAttributesPhysical::set_focal_length(float p_focal_length)
{
	frustum_focal_length = p_focal_length;
	_update_frustum();
	emit_changed();
}

float CameraAttributesPhysical::get_focal_length() const { return frustum_focal_length; }

void CameraAttributesPhysical::set_focus_distance(float p_focus_distance)
{
	frustum_focus_distance = p_focus_distance;
	_update_frustum();
}

float CameraAttributesPhysical::get_focus_distance() const { return frustum_focus_distance; }

void CameraAttributesPhysical::set_near(real_t p_near)
{
	frustum_near = p_near;
	_update_frustum();
	emit_changed();
}

real_t CameraAttributesPhysical::get_near() const { return frustum_near; }

void CameraAttributesPhysical::set_far(real_t p_far)
{
	frustum_far = p_far;
	_update_frustum();
	emit_changed();
}

real_t CameraAttributesPhysical::get_far() const { return frustum_far; }

real_t CameraAttributesPhysical::get_fov() const { return frustum_fov; }

void CameraAttributesPhysical::_update_frustum()
{
	// https://en.wikipedia.org/wiki/Circle_of_confusion#Circle_of_confusion_diameter_limit_based_on_d/1500
	Vector2i sensor_size =
		Vector2i(36, 24); // Matches high-end DSLR, could be made variable if there is demand.
	float CoC = sensor_size.length() / 1500.0;

	frustum_fov = Math::rad_to_deg(2 * std::atan(sensor_size.height / (2 * frustum_focal_length)));

	// Based on https://en.wikipedia.org/wiki/Depth_of_field.
	float u = MAX(frustum_focus_distance * 1000.0,
		frustum_focal_length +
			1.0); // Focus distance expressed in mm and clamped to at least 1 mm away from lens.
	float hyperfocal_length =
		frustum_focal_length +
		((frustum_focal_length * frustum_focal_length) / (exposure_aperture * CoC));

	// This computes the start and end of the depth of field. Anything between these two points has
	// a Circle of Confusino so small that it is not picked up by the camera sensors. To be properly
	// physically-based, we would run the DoF shader at all depths. To be efficient, we are only
	// running it where the CoC will be visible, this introduces some value shifts in the near field
	// that we have to compensate for below.
	float depth_near =
		((hyperfocal_length * u) / (hyperfocal_length + (u - frustum_focal_length))) /
		1000.0; // In meters.
	float depth_far = ((hyperfocal_length * u) / (hyperfocal_length - (u - frustum_focal_length))) /
					  1000.0; // In meters.
	float scale = (frustum_focal_length / (u - frustum_focal_length)) *
				  (frustum_focal_length / exposure_aperture);

	bool use_far = (depth_far < frustum_far) && (depth_far > 0.0);
	bool use_near = depth_near > frustum_near;
#ifdef DEBUG_ENABLED
	if (OS::get_current_rendering_method() == "gl_compatibility") {
		// Force disable DoF in editor builds to suppress warnings.
		use_far = false;
		use_near = false;
	}
#endif
	Renderer::camera_attributes_set_dof_blur(get_rid(), use_far,
		u / 1000.0, // Focus distance clampd to focal length expressed in meters.
		-1.0,		// Negative to tell Bokeh effect to use physically-based scaling.
		use_near, u / 1000.0, -1.0,
		scale / 5.0); // Arbitrary scaling to get close to how much blur there should be.
}

float CameraAttributesPhysical::calculate_exposure_normalization() const
{
	const float e = (exposure_aperture * exposure_aperture) * exposure_shutter_speed *
					(100.0 / exposure_sensitivity);
	return 1.0 / (e * 1.2);
}

void CameraAttributesPhysical::set_auto_exposure_min_exposure_value(float p_min)
{
	auto_exposure_min = p_min;
	_update_auto_exposure();
}

float CameraAttributesPhysical::get_auto_exposure_min_exposure_value() const
{
	return auto_exposure_min;
}

void CameraAttributesPhysical::set_auto_exposure_max_exposure_value(float p_max)
{
	auto_exposure_max = p_max;
	_update_auto_exposure();
}

float CameraAttributesPhysical::get_auto_exposure_max_exposure_value() const
{
	return auto_exposure_max;
}

void CameraAttributesPhysical::_update_auto_exposure()
{
	Renderer::camera_attributes_set_auto_exposure(get_rid(), auto_exposure_enabled,
		std::pow(2.0, auto_exposure_min) *
			(12.5 / exposure_sensitivity), // Convert from EV100 to Luminance
		std::pow(2.0, auto_exposure_max) *
			(12.5 / exposure_sensitivity), // Convert from EV100 to Luminance
		auto_exposure_speed, auto_exposure_scale);
	emit_changed();
}

CameraAttributesPhysical::~CameraAttributesPhysical() {}

// --- compositor.cpp ---

#include "compositor.h"
#include "servers/rendering/renderer.h"

void CompositorEffect::set_enabled(bool p_enabled)
{
	enabled = p_enabled;
	if (rid.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::compositor_effect_set_enabled(rid, enabled);
	}
}

bool CompositorEffect::get_enabled() const { return enabled; }

CompositorEffect::EffectCallbackType CompositorEffect::get_effect_callback_type() const
{
	return effect_callback_type;
}

void CompositorEffect::set_access_resolved_color(bool p_enabled)
{
	access_resolved_color = p_enabled;
	if (rid.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::compositor_effect_set_flag(rid,
			RSE::CompositorEffectFlags::COMPOSITOR_EFFECT_FLAG_ACCESS_RESOLVED_COLOR,
			access_resolved_color);
	}
}

bool CompositorEffect::get_access_resolved_color() const { return access_resolved_color; }

void CompositorEffect::set_access_resolved_depth(bool p_enabled)
{
	access_resolved_depth = p_enabled;
	if (rid.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::compositor_effect_set_flag(rid,
			RSE::CompositorEffectFlags::COMPOSITOR_EFFECT_FLAG_ACCESS_RESOLVED_DEPTH,
			access_resolved_depth);
	}
}

bool CompositorEffect::get_access_resolved_depth() const { return access_resolved_depth; }

void CompositorEffect::set_needs_motion_vectors(bool p_enabled)
{
	needs_motion_vectors = p_enabled;
	if (rid.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::compositor_effect_set_flag(rid,
			RSE::CompositorEffectFlags::COMPOSITOR_EFFECT_FLAG_NEEDS_MOTION_VECTORS,
			needs_motion_vectors);
	}
}

bool CompositorEffect::get_needs_motion_vectors() const { return needs_motion_vectors; }

void CompositorEffect::set_needs_normal_roughness(bool p_enabled)
{
	needs_normal_roughness = p_enabled;
	if (rid.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::compositor_effect_set_flag(rid,
			RSE::CompositorEffectFlags::COMPOSITOR_EFFECT_FLAG_NEEDS_ROUGHNESS,
			needs_normal_roughness);
	}
}

bool CompositorEffect::get_needs_normal_roughness() const { return needs_normal_roughness; }

void CompositorEffect::set_needs_separate_specular(bool p_enabled)
{
	needs_separate_specular = p_enabled;
	if (rid.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::compositor_effect_set_flag(rid,
			RSE::CompositorEffectFlags::COMPOSITOR_EFFECT_FLAG_NEEDS_SEPARATE_SPECULAR,
			needs_separate_specular);
	}
}

bool CompositorEffect::get_needs_separate_specular() const { return needs_separate_specular; }

CompositorEffect::~CompositorEffect()
{
	if (Renderer::data != nullptr && rid.is_valid()) {
		Renderer::free_rid(rid);
	}
}

/* Compositor */


Compositor::Compositor()
{
	if (Renderer::data != nullptr) {
		compositor = Renderer::compositor_create();
	}
}

Compositor::~Compositor()
{
	if (Renderer::data != nullptr && compositor.is_valid()) {
		Renderer::free_rid(compositor);
	}
}

void CompositorEffect::_call_render_callback(
	int p_effect_callback_type, const RenderDataRD* p_render_data)
{
}

// --- sky.cpp ---

#include "servers/rendering/renderer.h"
#include "sky.h"

void Sky::set_radiance_size(RadianceSize p_size)
{
	ERR_FAIL_INDEX(p_size, RADIANCE_SIZE_MAX);

	radiance_size = p_size;
	static const int size[RADIANCE_SIZE_MAX] = {32, 64, 128, 256, 512, 1024, 2048};
	Renderer::sky_set_radiance_size(sky, size[radiance_size]);
}

Sky::RadianceSize Sky::get_radiance_size() const { return radiance_size; }

void Sky::set_process_mode(ProcessMode p_mode)
{
	mode = p_mode;
	Renderer::sky_set_mode(sky, RSE::SkyMode(mode));
}

Sky::ProcessMode Sky::get_process_mode() const { return mode; }

void Sky::set_material(const Ref<Material>& p_material)
{
	sky_material = p_material;
	RID material_rid;
	if (sky_material.is_valid()) {
		material_rid = sky_material->get_rid();
	}
	Renderer::sky_set_material(sky, material_rid);
}

Ref<Material> Sky::get_material() const { return sky_material; }

RID Sky::get_rid() const { return sky; }


Sky::Sky() { sky = Renderer::sky_create(); }

Sky::~Sky()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(sky);
}

// --- world_2d.cpp ---

#include "core/config/project_settings.h"
#include "scene/2d/visible_on_screen_notifier_2d.h"
#include "servers/rendering/renderer.h"
#include "world_2d.h"

#ifndef NAVIGATION_2D_DISABLED
#include "servers/navigation_2d/navigation_server_2d.h"
#endif // NAVIGATION_2D_DISABLED

RID World2D::get_canvas() const { return canvas; }

#ifndef PHYSICS_2D_DISABLED
PhysicsDirectSpaceState2D* World2D::get_direct_space_state()
{
	return PhysicsServer2D::space_get_direct_state(get_space());
}
#endif // PHYSICS_2D_DISABLED

void World2D::register_viewport(Viewport* p_viewport) { viewports.insert(p_viewport); }

void World2D::remove_viewport(Viewport* p_viewport) { viewports.erase(p_viewport); }

World2D::World2D() { canvas = Renderer::canvas_create(); }

World2D::~World2D()
{
	ERR_FAIL_NULL(Renderer::data);
	Renderer::free_rid(canvas);

#ifndef NAVIGATION_2D_DISABLED
	ERR_FAIL_NULL(NavigationServer2D::data);
	if (navigation_map.is_valid()) {
		NavigationServer2D::free_rid(navigation_map);
	}
#endif // NAVIGATION_2D_DISABLED

#ifndef PHYSICS_2D_DISABLED
	if (space.is_valid()) {
		ERR_FAIL_COND(!PhysicsServer2D::is_initialized());
		PhysicsServer2D::free_rid(space);
	}
#endif // PHYSICS_2D_DISABLED
}

#ifndef NAVIGATION_2D_DISABLED
RID World2D::get_navigation_map() const
{
	return RID();
}
#endif // NAVIGATION_2D_DISABLED

RID World2D::get_space() const { return RID(); }

// --- navigation_mesh.cpp ---

#include "navigation_mesh.h"

#include "servers/navigation_3d/navigation_server_3d.h"

NavigationMesh::NavigationMesh() :
		cell_size(NavigationDefaults3D::NAV_MESH_CELL_SIZE),
		cell_height(NavigationDefaults3D::NAV_MESH_CELL_HEIGHT) {
}

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

void NavigationMesh::set_source_group_name(const String& p_group_name)
{
	source_group_name = p_group_name;
}

String NavigationMesh::get_source_group_name() const { return source_group_name; }

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

void NavigationMesh::set_vertices(const Vector<Vector3>& p_vertices) {}
