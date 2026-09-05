/**************************************************************************/
/*  environment.cpp                                                       */
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

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/os/os.h"
#include "environment.h"
#include "scene/resources/gradient_texture.h"
#include "scene/resources/sky.h"
#include "servers/rendering/rendering_server.h"

RID Environment::get_rid() const { return environment; }

// Background

void Environment::set_background(BGMode p_bg)
{
	bg_mode = p_bg;
	RS::get_singleton()->environment_set_background(environment, RSE::EnvironmentBG(p_bg));
	this->obj->notify_property_list_changed();
	if (bg_mode != BG_SKY) {
		set_fog_aerial_perspective(0.0);
	}
}

Environment::BGMode Environment::get_background() const { return bg_mode; }

void Environment::set_sky(const Ref<Sky>& p_sky)
{
	bg_sky = p_sky;
	RID sb_rid;
	if (bg_sky.is_valid()) {
		sb_rid = bg_sky->get_rid();
	}
	RS::get_singleton()->environment_set_sky(environment, sb_rid);
}

Ref<Sky> Environment::get_sky() const { return bg_sky; }

void Environment::set_sky_custom_fov(float p_scale)
{
	bg_sky_custom_fov = p_scale;
	RS::get_singleton()->environment_set_sky_custom_fov(environment, p_scale);
}

float Environment::get_sky_custom_fov() const { return bg_sky_custom_fov; }

void Environment::set_sky_rotation(const Vector3& p_rotation)
{
	bg_sky_rotation = p_rotation;
	RS::get_singleton()->environment_set_sky_orientation(
		environment, Basis::from_euler(p_rotation));
}

Vector3 Environment::get_sky_rotation() const { return bg_sky_rotation; }

void Environment::set_bg_color(const Color& p_color)
{
	bg_color = p_color;
	RS::get_singleton()->environment_set_bg_color(environment, p_color);
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
		RS::get_singleton()->environment_set_bg_energy(
			environment, bg_energy_multiplier, bg_intensity);
	}
	else {
		RS::get_singleton()->environment_set_bg_energy(environment, bg_energy_multiplier, 1.0);
	}
}

void Environment::set_canvas_max_layer(int p_max_layer)
{
	bg_canvas_max_layer = p_max_layer;
	RS::get_singleton()->environment_set_canvas_max_layer(environment, p_max_layer);
}

int Environment::get_canvas_max_layer() const { return bg_canvas_max_layer; }

void Environment::set_camera_feed_id(int p_id)
{
	bg_camera_feed_id = p_id;
	RS::get_singleton()->environment_set_camera_feed_id(environment, bg_camera_feed_id);
}

int Environment::get_camera_feed_id() const { return bg_camera_feed_id; }

// Ambient light

void Environment::set_ambient_light_color(const Color& p_color)
{
	ambient_color = p_color;
	_update_ambient_light();
}

Color Environment::get_ambient_light_color() const { return ambient_color; }

void Environment::set_ambient_source(AmbientSource p_source)
{
	ambient_source = p_source;
	_update_ambient_light();
	this->obj->notify_property_list_changed();
}

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

void Environment::set_reflection_source(ReflectionSource p_source)
{
	reflection_source = p_source;
	_update_ambient_light();
	this->obj->notify_property_list_changed();
}

Environment::ReflectionSource Environment::get_reflection_source() const
{
	return reflection_source;
}

void Environment::_update_ambient_light()
{
	RS::get_singleton()->environment_set_ambient_light(environment, ambient_color,
		RSE::EnvironmentAmbientSource(ambient_source), ambient_energy, ambient_sky_contribution,
		RSE::EnvironmentReflectionSource(reflection_source));
}

// Tonemap

void Environment::set_tonemapper(ToneMapper p_tone_mapper)
{
	tone_mapper = p_tone_mapper;
	_update_tonemap();
	this->obj->notify_property_list_changed();
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
	RS::get_singleton()->environment_set_tonemap_agx_contrast(environment, p_agx_contrast);
}

float Environment::get_tonemap_agx_contrast() const { return tonemap_agx_contrast; }

void Environment::_update_tonemap()
{
	RS::get_singleton()->environment_set_tonemap(environment,
		RSE::EnvironmentToneMapper(tone_mapper), tonemap_exposure,
		tone_mapper == TONE_MAPPER_AGX ? tonemap_agx_white : tonemap_white);
}

// SSR

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
	RS::get_singleton()->environment_set_ssr(
		environment, ssr_enabled, ssr_max_steps, ssr_fade_in, ssr_fade_out, ssr_depth_tolerance);
}

// SSAO

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
	RS::get_singleton()->environment_set_ssao(environment, ssao_enabled, ssao_radius,
		ssao_intensity, ssao_power, ssao_detail, ssao_horizon, ssao_sharpness,
		ssao_direct_light_affect, ssao_ao_channel_affect);
}

// SSIL

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
	RS::get_singleton()->environment_set_ssil(environment, ssil_enabled, ssil_radius,
		ssil_intensity, ssil_sharpness, ssil_normal_rejection);
}

// SDFGI

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
	RS::get_singleton()->environment_set_sdfgi(environment, sdfgi_enabled, sdfgi_cascades,
		sdfgi_min_cell_size, RSE::EnvironmentSDFGIYScale(sdfgi_y_scale), sdfgi_use_occlusion,
		sdfgi_bounce_feedback, sdfgi_read_sky_light, sdfgi_energy, sdfgi_normal_bias,
		sdfgi_probe_bias);
}

// Glow

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

void Environment::set_glow_blend_mode(GlowBlendMode p_mode)
{
	glow_blend_mode = p_mode;
	_update_glow();
	this->obj->notify_property_list_changed();
}

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

	RS::get_singleton()->environment_set_glow(environment, glow_enabled, normalized_levels,
		glow_intensity, glow_strength, glow_mix, glow_bloom,
		RSE::EnvironmentGlowBlendMode(glow_blend_mode), glow_hdr_bleed_threshold,
		glow_hdr_bleed_scale, glow_hdr_luminance_cap, _glow_map_strength, glow_map_rid);
}

// Fog

void Environment::set_fog_enabled(bool p_enabled)
{
	fog_enabled = p_enabled;
	_update_fog();
}

bool Environment::is_fog_enabled() const { return fog_enabled; }

void Environment::set_fog_mode(FogMode p_mode)
{
	if (fog_mode != p_mode && p_mode == FogMode::FOG_MODE_EXPONENTIAL) {
		set_fog_density(0.01);
	}
	else {
		set_fog_density(1.0);
	}
	fog_mode = p_mode;
	_update_fog();
	this->obj->notify_property_list_changed();
}

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
	RS::get_singleton()->environment_set_fog(environment, fog_enabled, fog_light_color,
		fog_light_energy, fog_sun_scatter, fog_density, fog_height, fog_height_density,
		fog_aerial_perspective, fog_sky_affect, RSE::EnvironmentFogMode(fog_mode));
}

// Depth Fog

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
	RS::get_singleton()->environment_set_fog_depth(
		environment, fog_depth_curve, fog_depth_begin, fog_depth_end);
}

// Volumetric Fog

void Environment::_update_volumetric_fog()
{
	RS::get_singleton()->environment_set_volumetric_fog(environment, volumetric_fog_enabled,
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

// Adjustment

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

void Environment::set_adjustment_color_correction(Ref<Texture> p_color_correction)
{
	adjustment_color_correction = p_color_correction;
	Ref<GradientTexture1D> grad_tex = p_color_correction;
	if (grad_tex.is_valid()) {
		grad_tex->connect_changed(callable_mp(this, &Environment::_update_adjustment));
	}
	Ref<Texture2D> adjustment_texture_2d = adjustment_color_correction;
	if (adjustment_texture_2d.is_valid()) {
		use_1d_color_correction = true;
	}
	else {
		use_1d_color_correction = false;
	}
	_update_adjustment();
}

Ref<Texture> Environment::get_adjustment_color_correction() const
{
	return adjustment_color_correction;
}

void Environment::_update_adjustment()
{
	RID color_correction =
		adjustment_color_correction.is_valid() ? adjustment_color_correction->get_rid() : RID();

	RS::get_singleton()->environment_set_adjustment(environment, adjustment_enabled,
		adjustment_brightness, adjustment_contrast, adjustment_saturation, use_1d_color_correction,
		color_correction);
}

// Private methods, constructor and destructor

void Environment::_validate_property(PropertyInfo& p_property) const
{
	if (!Engine::get_singleton()->is_editor_hint()) {
		return;
	}
	if (p_property.name == "sky" || p_property.name == "sky_custom_fov" ||
		p_property.name == "sky_rotation" || p_property.name == "ambient_light_sky_contribution") {
		if (bg_mode != BG_SKY && ambient_source != AMBIENT_SOURCE_SKY &&
			reflection_source != REFLECTION_SOURCE_SKY) {
			p_property.usage = PROPERTY_USAGE_NO_EDITOR;
		}
		return;
	}

	if (p_property.name == "fog_depth_curve" || p_property.name == "fog_depth_begin" ||
		p_property.name == "fog_depth_end") {
		if (fog_mode == FOG_MODE_EXPONENTIAL) {
			p_property.usage = PROPERTY_USAGE_NO_EDITOR;
		}
		return;
	}

	if (p_property.name == "ambient_light_color" || p_property.name == "ambient_light_energy") {
		if (ambient_source == AMBIENT_SOURCE_DISABLED) {
			p_property.usage = PROPERTY_USAGE_NO_EDITOR;
		}
		return;
	}

	if (p_property.name == "ambient_light_sky_contribution") {
		if (ambient_source == AMBIENT_SOURCE_DISABLED || ambient_source == AMBIENT_SOURCE_COLOR) {
			p_property.usage = PROPERTY_USAGE_NO_EDITOR;
		}
		return;
	}

	if (p_property.name == "fog_aerial_perspective") {
		if (bg_mode != BG_SKY) {
			p_property.usage = PROPERTY_USAGE_NO_EDITOR;
		}
		return;
	}

	if (p_property.name == "tonemap_white") {
		if (tone_mapper == TONE_MAPPER_LINEAR || tone_mapper == TONE_MAPPER_AGX) {
			p_property.usage = PROPERTY_USAGE_NO_EDITOR;
		}
		return;
	}

	if (p_property.name == "tonemap_agx_white") {
		if (tone_mapper != TONE_MAPPER_AGX) {
			p_property.usage = PROPERTY_USAGE_NO_EDITOR;
		}
		return;
	}

	if (p_property.name == "tonemap_agx_contrast") {
		if (tone_mapper != TONE_MAPPER_AGX) {
			p_property.usage = PROPERTY_USAGE_NO_EDITOR;
		}
		return;
	}

	if (p_property.name == "glow_intensity") {
		if (glow_blend_mode == GLOW_BLEND_MODE_MIX &&
			OS::get_singleton()->get_current_rendering_method() != "gl_compatibility") {
			p_property.usage = PROPERTY_USAGE_NO_EDITOR;
		}
		return;
	}

	if (OS::get_singleton()->get_current_rendering_method() == "gl_compatibility") {
		// Hide glow properties we do not support in GL Compatibility.
		if (p_property.name.begins_with("glow_levels") || p_property.name == "glow_normalized" ||
			p_property.name == "glow_strength" || p_property.name == "glow_mix" ||
			p_property.name == "glow_blend_mode" || p_property.name == "glow_map_strength" ||
			p_property.name == "glow_map") {
			p_property.usage = PROPERTY_USAGE_NO_EDITOR;
			return;
		}
	}
	else {
		if (p_property.name == "glow_mix" && glow_blend_mode != GLOW_BLEND_MODE_MIX) {
			p_property.usage = PROPERTY_USAGE_NO_EDITOR;
			return;
		}
	}

	if (OS::get_singleton()->get_current_rendering_method() != "forward_plus") {
		// Hide SSAO properties that only work in Forward+.
		if (p_property.name.begins_with("ssao_")) {
			if ((p_property.name != "ssao_enabled") && (p_property.name != "ssao_radius") &&
				(p_property.name != "ssao_intensity")) {
				p_property.usage = PROPERTY_USAGE_NO_EDITOR;
			}
			return;
		}
	}

	if (p_property.name == "background_color") {
		if (bg_mode != BG_COLOR && ambient_source != AMBIENT_SOURCE_COLOR) {
			p_property.usage = PROPERTY_USAGE_NO_EDITOR;
		}
		return;
	}

	if (p_property.name == "background_canvas_max_layer") {
		if (bg_mode != BG_CANVAS) {
			p_property.usage = PROPERTY_USAGE_NO_EDITOR;
		}
		return;
	}

	if (p_property.name == "background_camera_feed_id") {
		if (bg_mode != BG_CAMERA_FEED) {
			p_property.usage = PROPERTY_USAGE_NO_EDITOR;
		}
		return;
	}

	if (p_property.name == "background_intensity" &&
		!GLOBAL_GET_CACHED(bool, "rendering/lights_and_shadows/use_physical_light_units")) {
		p_property.usage = PROPERTY_USAGE_NO_EDITOR;
	}
}

#ifndef DISABLE_DEPRECATED
// Kept for compatibility from 3.x to 4.0.
bool Environment::_set(const StringName& p_name, const Variant& p_value)
{
	if (p_name == "background_sky") {
		set_sky(p_value);
		return true;
	}
	else if (p_name == "background_sky_custom_fov") {
		set_sky_custom_fov(p_value);
		return true;
	}
	else if (p_name == "background_sky_orientation") {
		Vector3 euler = p_value.operator Basis().get_euler();
		set_sky_rotation(euler);
		return true;
	}
	else {
		return false;
	}
}
#endif

void Environment::_bind_methods() {}

Environment::Environment()
{
	environment = RS::get_singleton()->environment_create();

	set_camera_feed_id(bg_camera_feed_id);

	glow_levels.resize(7);
	glow_levels.write[0] = 0.0;
	glow_levels.write[1] = 0.8;
	glow_levels.write[2] = 0.4;
	glow_levels.write[3] = 0.1;
	glow_levels.write[4] = 0.0;
	glow_levels.write[5] = 0.0;
	glow_levels.write[6] = 0.0;

	_update_ambient_light();
	_update_tonemap();
	_update_ssr();
	_update_ssao();
	_update_ssil();
	_update_sdfgi();
	_update_glow();
	_update_fog();
	_update_adjustment();
	_update_volumetric_fog();
	_update_bg_energy();
	this->obj->notify_property_list_changed();
}

Environment::~Environment()
{
	ERR_FAIL_NULL(RenderingServer::get_singleton());
	RS::get_singleton()->free_rid(environment);
}


