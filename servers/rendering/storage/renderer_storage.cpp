/**************************************************************************/
/*  renderer_storage.cpp                                                  */
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

#include "servers/rendering/storage/camera_attributes_storage.h"

#ifdef DEBUG_ENABLED
#include "core/os/os.h"
#endif
#include "servers/rendering/storage/compositor_storage.h"
#include "servers/rendering/storage/environment_storage.h"

#ifdef DEBUG_ENABLED
#include "core/os/os.h"
#endif
#include "core/config/engine.h"
#include "core/math/transform_interpolator.h"
#include "servers/rendering/storage/mesh_storage.h"
#include "servers/rendering/storage/render_scene_buffers.h"
#include "servers/rendering/renderer.h" // IWYU pragma: keep // Needed to bind RSE enums.
#include "servers/rendering/storage/utilities.h"

// --- Merged from servers/rendering/storage/camera_attributes_storage.cpp ---

void RendererCameraAttributes::initialize()
{
	if (_initialized) {
		return;
	}
	dof_blur_quality = RSE::DOF_BLUR_QUALITY_MEDIUM;
	dof_blur_bokeh_shape = RSE::DOF_BOKEH_HEXAGON;
	dof_blur_use_jitter = false;
	auto_exposure_counter = 2;
	_initialized = true;
}

void RendererCameraAttributes::finalize()
{
	if (!_initialized) {
		return;
	}
	_initialized = false;
}

RID RendererCameraAttributes::camera_attributes_allocate()
{
	return camera_attributes_owner.allocate_rid();
}

void RendererCameraAttributes::camera_attributes_initialize(RID p_rid)
{
	camera_attributes_owner.initialize_rid(p_rid, CameraAttributes());
}

void RendererCameraAttributes::camera_attributes_free(RID p_rid)
{
	camera_attributes_owner.free(p_rid);
}

void RendererCameraAttributes::camera_attributes_set_dof_blur_quality(
	RSE::DOFBlurQuality p_quality, bool p_use_jitter)
{
	dof_blur_quality = p_quality;
	dof_blur_use_jitter = p_use_jitter;
}

void RendererCameraAttributes::camera_attributes_set_dof_blur_bokeh_shape(RSE::DOFBokehShape p_shape)
{
	dof_blur_bokeh_shape = p_shape;
}

void RendererCameraAttributes::camera_attributes_set_dof_blur(RID p_camera_attributes,
	bool p_far_enable, float p_far_distance, float p_far_transition, bool p_near_enable,
	float p_near_distance, float p_near_transition, float p_amount)
{
	CameraAttributes* cam_attributes = camera_attributes_owner.get_or_null(p_camera_attributes);
	ERR_FAIL_NULL(cam_attributes);
#ifdef DEBUG_ENABLED
	if (OS::get_current_rendering_method() == "gl_compatibility" &&
		(p_far_enable || p_near_enable)) {
		WARN_PRINT_ONCE_ED(
			"Depth of field blur is only available when using the Forward+ or Mobile renderer.");
	}
#endif
	cam_attributes->dof_blur_far_enabled = p_far_enable;
	cam_attributes->dof_blur_far_distance = p_far_distance;
	cam_attributes->dof_blur_far_transition = p_far_transition;

	cam_attributes->dof_blur_near_enabled = p_near_enable;
	cam_attributes->dof_blur_near_distance = p_near_distance;
	cam_attributes->dof_blur_near_transition = p_near_transition;

	cam_attributes->dof_blur_amount = p_amount;
}

bool RendererCameraAttributes::camera_attributes_get_dof_far_enabled(RID p_camera_attributes)
{
	CameraAttributes* cam_attributes = camera_attributes_owner.get_or_null(p_camera_attributes);
	ERR_FAIL_NULL_V(cam_attributes, false);
	return cam_attributes->dof_blur_far_enabled;
}

float RendererCameraAttributes::camera_attributes_get_dof_far_distance(RID p_camera_attributes)
{
	CameraAttributes* cam_attributes = camera_attributes_owner.get_or_null(p_camera_attributes);
	ERR_FAIL_NULL_V(cam_attributes, 0.0);
	return cam_attributes->dof_blur_far_distance;
}

float RendererCameraAttributes::camera_attributes_get_dof_far_transition(RID p_camera_attributes)
{
	CameraAttributes* cam_attributes = camera_attributes_owner.get_or_null(p_camera_attributes);
	ERR_FAIL_NULL_V(cam_attributes, 0.0);
	return cam_attributes->dof_blur_far_transition;
}

bool RendererCameraAttributes::camera_attributes_get_dof_near_enabled(RID p_camera_attributes)
{
	CameraAttributes* cam_attributes = camera_attributes_owner.get_or_null(p_camera_attributes);
	ERR_FAIL_NULL_V(cam_attributes, false);
	return cam_attributes->dof_blur_near_enabled;
}

float RendererCameraAttributes::camera_attributes_get_dof_near_distance(RID p_camera_attributes)
{
	CameraAttributes* cam_attributes = camera_attributes_owner.get_or_null(p_camera_attributes);
	ERR_FAIL_NULL_V(cam_attributes, 0.0);
	return cam_attributes->dof_blur_near_distance;
}

float RendererCameraAttributes::camera_attributes_get_dof_near_transition(RID p_camera_attributes)
{
	CameraAttributes* cam_attributes = camera_attributes_owner.get_or_null(p_camera_attributes);
	ERR_FAIL_NULL_V(cam_attributes, 0.0);
	return cam_attributes->dof_blur_near_transition;
}

float RendererCameraAttributes::camera_attributes_get_dof_blur_amount(RID p_camera_attributes)
{
	CameraAttributes* cam_attributes = camera_attributes_owner.get_or_null(p_camera_attributes);
	ERR_FAIL_NULL_V(cam_attributes, 0.0);
	return cam_attributes->dof_blur_amount;
}

void RendererCameraAttributes::camera_attributes_set_exposure(
	RID p_camera_attributes, float p_multiplier, float p_exposure_normalization)
{
	CameraAttributes* cam_attributes = camera_attributes_owner.get_or_null(p_camera_attributes);
	ERR_FAIL_NULL(cam_attributes);
	cam_attributes->exposure_multiplier = p_multiplier;
	cam_attributes->exposure_normalization = p_exposure_normalization;
}

float RendererCameraAttributes::camera_attributes_get_exposure_normalization_factor(
	RID p_camera_attributes)
{
	CameraAttributes* cam_attributes = camera_attributes_owner.get_or_null(p_camera_attributes);
	ERR_FAIL_NULL_V(cam_attributes, 1.0);

	return cam_attributes->exposure_multiplier * cam_attributes->exposure_normalization;
}

void RendererCameraAttributes::camera_attributes_set_auto_exposure(RID p_camera_attributes,
	bool p_enable, float p_min_sensitivity, float p_max_sensitivity, float p_speed, float p_scale)
{
	CameraAttributes* cam_attributes = camera_attributes_owner.get_or_null(p_camera_attributes);
	ERR_FAIL_NULL(cam_attributes);
	if (!cam_attributes->use_auto_exposure && p_enable) {
		cam_attributes->auto_exposure_version = ++auto_exposure_counter;
	}
#ifdef DEBUG_ENABLED
	if ((OS::get_current_rendering_method() == "gl_compatibility" ||
			OS::get_current_rendering_method() == "mobile") &&
		p_enable) {
		WARN_PRINT_ONCE_ED("Auto-exposure is only available when using the Forward+ renderer.");
	}
#endif
	cam_attributes->use_auto_exposure = p_enable;
	cam_attributes->auto_exposure_min_sensitivity = p_min_sensitivity;
	cam_attributes->auto_exposure_max_sensitivity = p_max_sensitivity;
	cam_attributes->auto_exposure_adjust_speed = p_speed;
	cam_attributes->auto_exposure_scale = p_scale;
}

float RendererCameraAttributes::camera_attributes_get_auto_exposure_min_sensitivity(
	RID p_camera_attributes)
{
	CameraAttributes* cam_attributes = camera_attributes_owner.get_or_null(p_camera_attributes);
	ERR_FAIL_NULL_V(cam_attributes, 0.0);
	return cam_attributes->auto_exposure_min_sensitivity;
}

float RendererCameraAttributes::camera_attributes_get_auto_exposure_max_sensitivity(
	RID p_camera_attributes)
{
	CameraAttributes* cam_attributes = camera_attributes_owner.get_or_null(p_camera_attributes);
	ERR_FAIL_NULL_V(cam_attributes, 0.0);
	return cam_attributes->auto_exposure_max_sensitivity;
}

float RendererCameraAttributes::camera_attributes_get_auto_exposure_adjust_speed(
	RID p_camera_attributes)
{
	CameraAttributes* cam_attributes = camera_attributes_owner.get_or_null(p_camera_attributes);
	ERR_FAIL_NULL_V(cam_attributes, 0.0);
	return cam_attributes->auto_exposure_adjust_speed;
}

float RendererCameraAttributes::camera_attributes_get_auto_exposure_scale(RID p_camera_attributes)
{
	CameraAttributes* cam_attributes = camera_attributes_owner.get_or_null(p_camera_attributes);
	ERR_FAIL_NULL_V(cam_attributes, 0.0);
	return cam_attributes->auto_exposure_scale;
}

uint64_t RendererCameraAttributes::camera_attributes_get_auto_exposure_version(
	RID p_camera_attributes)
{
	CameraAttributes* cam_attributes = camera_attributes_owner.get_or_null(p_camera_attributes);
	ERR_FAIL_NULL_V(cam_attributes, 0);
	return cam_attributes->auto_exposure_version;
}

// --- Merged from servers/rendering/storage/compositor_storage.cpp ---

void RendererCompositorStorage::initialize()
{
	if (_initialized) {
		return;
	}
	num_compositor_effects_with_motion_vectors = 0;
	_initialized = true;
}

void RendererCompositorStorage::finalize()
{
	if (!_initialized) {
		return;
	}
	_initialized = false;
}

RID RendererCompositorStorage::compositor_effect_allocate()
{
	return compositor_effects_owner.allocate_rid();
}

void RendererCompositorStorage::compositor_effect_initialize(RID p_rid)
{
	compositor_effects_owner.initialize_rid(p_rid, CompositorEffect());
}

bool RendererCompositorStorage::compositor_effect_get_enabled(RID p_effect)
{
	CompositorEffect* effect = compositor_effects_owner.get_or_null(p_effect);
	ERR_FAIL_NULL_V(effect, false);

	return effect->is_enabled;
}

RSE::CompositorEffectCallbackType RendererCompositorStorage::compositor_effect_get_callback_type(
	RID p_effect)
{
	CompositorEffect* effect = compositor_effects_owner.get_or_null(p_effect);
	ERR_FAIL_NULL_V(effect, RSE::COMPOSITOR_EFFECT_CALLBACK_TYPE_MAX);

	return effect->callback_type;
}

RID RendererCompositorStorage::compositor_allocate() { return compositor_owner.allocate_rid(); }

void RendererCompositorStorage::compositor_initialize(RID p_rid)
{
	compositor_owner.initialize_rid(p_rid, Compositor());
}

void RendererCompositorStorage::compositor_free(RID p_rid) { compositor_owner.free(p_rid); }

void RendererCompositorStorage::compositor_set_compositor_effects(
	RID p_compositor, const Vector<RID>& p_effects)
{
	Compositor* compositor = compositor_owner.get_or_null(p_compositor);
	ERR_FAIL_NULL(compositor);

	compositor->compositor_effects.clear();
	for (const RID& effect : p_effects) {
		if (is_compositor_effect(effect)) {
			compositor->compositor_effects.push_back(effect);
		}
	}
}

Vector<RID> RendererCompositorStorage::compositor_get_compositor_effects(
	RID p_compositor, RSE::CompositorEffectCallbackType p_callback_type, bool p_enabled_only)
{
	Compositor* compositor = compositor_owner.get_or_null(p_compositor);
	ERR_FAIL_NULL_V(compositor, Vector<RID>());

	if (p_enabled_only || p_callback_type != RSE::COMPOSITOR_EFFECT_CALLBACK_TYPE_ANY) {
		Vector<RID> effects;

		for (RID rid : compositor->compositor_effects) {
			if ((!p_enabled_only || compositor_effect_get_enabled(rid)) &&
				(p_callback_type == RSE::COMPOSITOR_EFFECT_CALLBACK_TYPE_ANY ||
					compositor_effect_get_callback_type(rid) == p_callback_type)) {
				effects.push_back(rid);
			}
		}

		return effects;
	}
	else {
		return compositor->compositor_effects;
	}
}

void RendererCompositorStorage::compositor_effect_set_flag(
	RID p_effect, RenderingServerEnums::CompositorEffectFlags p_flag, bool p_set)
{
}

void RendererCompositorStorage::compositor_effect_set_enabled(RID p_effect, bool p_enabled) {}

bool RendererCompositorStorage::compositor_effect_get_flag(
	RID p_effect, RenderingServerEnums::CompositorEffectFlags p_flag)
{
	return false;
}

void RendererCompositorStorage::compositor_effect_free(RID p_effect) {}

// --- Merged from servers/rendering/storage/environment_storage.cpp ---

// Storage

void RendererEnvironmentStorage::initialize()
{
	if (_initialized) {
		return;
	}
	_initialized = true;
}

void RendererEnvironmentStorage::finalize()
{
	if (!_initialized) {
		return;
	}
	_initialized = false;
}

// Environment

RID RendererEnvironmentStorage::environment_allocate() {
	return environment_owner.allocate_rid();
}

void RendererEnvironmentStorage::environment_initialize(RID p_rid) {
	environment_owner.initialize_rid(p_rid, Environment());
}

void RendererEnvironmentStorage::environment_free(RID p_rid) {
	environment_owner.free(p_rid);
}

// Background

void RendererEnvironmentStorage::environment_set_background(RID p_env, RSE::EnvironmentBG p_bg) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL(env);
	env->background = p_bg;
}

void RendererEnvironmentStorage::environment_set_sky(RID p_env, RID p_sky) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL(env);
	env->sky = p_sky;
}

void RendererEnvironmentStorage::environment_set_sky_custom_fov(RID p_env, float p_scale) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL(env);
	env->sky_custom_fov = p_scale;
}

void RendererEnvironmentStorage::environment_set_sky_orientation(RID p_env, const Basis &p_orientation) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL(env);
	env->sky_orientation = p_orientation;
}

void RendererEnvironmentStorage::environment_set_bg_color(RID p_env, const Color &p_color) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL(env);
	env->bg_color = p_color;
}

void RendererEnvironmentStorage::environment_set_bg_energy(RID p_env, float p_multiplier, float p_intensity) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL(env);
	env->bg_energy_multiplier = p_multiplier;
	env->bg_intensity = p_intensity;
}

void RendererEnvironmentStorage::environment_set_canvas_max_layer(RID p_env, int p_max_layer) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL(env);
	env->canvas_max_layer = p_max_layer;
}

void RendererEnvironmentStorage::environment_set_ambient_light(RID p_env, const Color &p_color, RSE::EnvironmentAmbientSource p_ambient, float p_energy, float p_sky_contribution, RSE::EnvironmentReflectionSource p_reflection_source) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL(env);
	env->ambient_light = p_color;
	env->ambient_source = p_ambient;
	env->ambient_light_energy = p_energy;
	env->ambient_sky_contribution = p_sky_contribution;
	env->reflection_source = p_reflection_source;
}

RSE::EnvironmentBG RendererEnvironmentStorage::environment_get_background(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, RSE::ENV_BG_CLEAR_COLOR);
	return env->background;
}

RID RendererEnvironmentStorage::environment_get_sky(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, RID());
	return env->sky;
}

float RendererEnvironmentStorage::environment_get_sky_custom_fov(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.0);
	return env->sky_custom_fov;
}

Basis RendererEnvironmentStorage::environment_get_sky_orientation(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, Basis());
	return env->sky_orientation;
}

Color RendererEnvironmentStorage::environment_get_bg_color(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, Color());
	return env->bg_color;
}

float RendererEnvironmentStorage::environment_get_bg_energy_multiplier(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 1.0);
	return env->bg_energy_multiplier;
}

float RendererEnvironmentStorage::environment_get_bg_intensity(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 1.0);
	return env->bg_intensity;
}

int RendererEnvironmentStorage::environment_get_canvas_max_layer(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0);
	return env->canvas_max_layer;
}

RSE::EnvironmentAmbientSource RendererEnvironmentStorage::environment_get_ambient_source(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, RSE::ENV_AMBIENT_SOURCE_BG);
	return env->ambient_source;
}

Color RendererEnvironmentStorage::environment_get_ambient_light(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, Color());
	return env->ambient_light;
}

float RendererEnvironmentStorage::environment_get_ambient_light_energy(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 1.0);
	return env->ambient_light_energy;
}

float RendererEnvironmentStorage::environment_get_ambient_sky_contribution(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 1.0);
	return env->ambient_sky_contribution;
}

RSE::EnvironmentReflectionSource RendererEnvironmentStorage::environment_get_reflection_source(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, RSE::ENV_REFLECTION_SOURCE_BG);
	return env->reflection_source;
}

void RendererEnvironmentStorage::environment_set_camera_feed_id(RID p_env, int p_camera_feed_id) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL(env);
	env->camera_feed_id = p_camera_feed_id;
}

int RendererEnvironmentStorage::environment_get_camera_feed_id(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, -1);
	return env->camera_feed_id;
}

// Tonemap

void RendererEnvironmentStorage::environment_set_tonemap(RID p_env, RSE::EnvironmentToneMapper p_tone_mapper, float p_exposure, float p_white) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL(env);
	env->exposure = p_exposure;
	env->tone_mapper = p_tone_mapper;
	env->white = p_white;
}

RSE::EnvironmentToneMapper RendererEnvironmentStorage::environment_get_tone_mapper(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, RSE::ENV_TONE_MAPPER_LINEAR);
	return env->tone_mapper;
}

float RendererEnvironmentStorage::environment_get_exposure(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 1.0);
	return env->exposure;
}

float RendererEnvironmentStorage::environment_get_white(RID p_env, bool p_limit_agx_white, float p_output_max_value) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 1.0);

	// Glow with screen blend mode does not work when white < 1.0, so make sure
	// it is at least 1.0 for all tonemappers:
	if (env->tone_mapper == RSE::ENV_TONE_MAPPER_LINEAR) {
		return p_output_max_value;
	} else if (env->tone_mapper == RSE::ENV_TONE_MAPPER_FILMIC || env->tone_mapper == RSE::ENV_TONE_MAPPER_ACES) {
		// Filmic and ACES only support SDR; their white is stable regardless
		// of output_max_value.
		return MAX(1.0, env->white);
	} else if (env->tone_mapper == RSE::ENV_TONE_MAPPER_AGX) {
		// AgX works best with a high white. 2.0 is the minimum required for
		// good behavior with Mobile rendering method.
		if (p_limit_agx_white) {
			return 2.0;
		} else {
			float agx_white = MAX(2.0, env->white);
			// Instead of constraining by matching the output_max_value, constrain
			// by multiplying to ensure the desired non-uniform scaling behavior
			// is maintained in the shoulder.
			return agx_white * p_output_max_value;
		}
	} else { // Reinhard
		// The Reinhard tonemapper is not designed to have a white parameter
		// that is less than the output max value. This is especially important
		// in the variable Extended Dynamic Range (EDR) paradigm where the
		// output max value may change to be greater or less than the white
		// parameter, depending on the available dynamic range.
		return MAX(p_output_max_value, env->white);
	}
}

void RendererEnvironmentStorage::environment_set_tonemap_agx_contrast(RID p_env, float p_agx_contrast) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL(env);
	env->tonemap_agx_contrast = p_agx_contrast;
}

float RendererEnvironmentStorage::environment_get_tonemap_agx_contrast(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 1.0);
	return env->tonemap_agx_contrast;
}

RendererEnvironmentStorage::TonemapParameters RendererEnvironmentStorage::environment_get_tonemap_parameters(RID p_env, bool p_limit_agx_white, float p_output_max_value) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, TonemapParameters());

	float white = environment_get_white(p_env, p_limit_agx_white, p_output_max_value);
	TonemapParameters tonemap_parameters = TonemapParameters();

	if (env->tone_mapper == RSE::ENV_TONE_MAPPER_LINEAR) {
		// Linear has no tonemapping parameters
	} else if (env->tone_mapper == RSE::ENV_TONE_MAPPER_REINHARD) {
		tonemap_parameters.white_squared = (white * white) / p_output_max_value;
	} else if (env->tone_mapper == RSE::ENV_TONE_MAPPER_FILMIC) {
		// These constants must match those in the shader code.
		// exposure_bias: Input scale (color *= bias, white *= bias) to make the brightness consistent with other tonemappers
		// also useful to scale the input to the range that the tonemapper is designed for (some require very high input values).
		// Has no effect on the curve's general shape or visual properties.
		const float exposure_bias = 2.0f;
		const float A = 0.22f * exposure_bias * exposure_bias; // bias baked into constants for performance
		const float B = 0.30f * exposure_bias;
		const float C = 0.10f;
		const float D = 0.20f;
		const float E = 0.01f;
		const float F = 0.30f;

		tonemap_parameters.white_tonemapped = ((white * (A * white + C * B) + D * E) / (white * (A * white + B) + D * F)) - E / F;
	} else if (env->tone_mapper == RSE::ENV_TONE_MAPPER_ACES) {
		// These constants must match those in the shader code.
		const float exposure_bias = 1.8f;
		const float A = 0.0245786f;
		const float B = 0.000090537f;
		const float C = 0.983729f;
		const float D = 0.432951f;
		const float E = 0.238081f;

		white *= exposure_bias;
		float white_tonemapped = (white * (white + A) - B) / (white * (C * white + D) + E);
		tonemap_parameters.white_tonemapped = white_tonemapped;
	} else if (env->tone_mapper == RSE::ENV_TONE_MAPPER_AGX) {
		// Calculate allenwp tonemapping curve parameters on the CPU to improve shader performance.
		// Source and details: https://allenwp.com/blog/2025/05/29/allenwp-tonemapping-curve/

		// These constants must match the those in the shader code.
		// 18% "middle gray" is perceptually 50% of the brightness of reference white.
		const float awp_crossover_point = 0.18;
		// When output_max_value and/or awp_crossover_point are no longer constant, awp_shoulder_max can
		// be calculated on the CPU and passed in as tonemap_parameters.tonemap_e.
		const float awp_shoulder_max = p_output_max_value - awp_crossover_point;

		float awp_high_clip = white;

		// awp_toe_a is a solution generated by Mathematica that ensures intersection at awp_crossover_point.
		float awp_toe_a = ((1.0 / awp_crossover_point) - 1.0) * pow(awp_crossover_point, env->tonemap_agx_contrast);
		// Slope formula is simply the derivative of the toe function with an input of awp_crossover_point.
		float awp_slope_denom = pow(awp_crossover_point, env->tonemap_agx_contrast) + awp_toe_a;
		float awp_slope = (env->tonemap_agx_contrast * pow(awp_crossover_point, env->tonemap_agx_contrast - 1.0) * awp_toe_a) / (awp_slope_denom * awp_slope_denom);

		float awp_w = awp_high_clip - awp_crossover_point;
		awp_w = awp_w * awp_w;
		awp_w = awp_w / awp_shoulder_max;
		awp_w = awp_w * awp_slope;

		tonemap_parameters.awp_contrast = env->tonemap_agx_contrast;
		tonemap_parameters.awp_toe_a = awp_toe_a;
		tonemap_parameters.awp_slope = awp_slope;
		tonemap_parameters.awp_w = awp_w;
	}

	return tonemap_parameters;
}

// Fog

void RendererEnvironmentStorage::environment_set_fog(RID p_env, bool p_enable, const Color &p_light_color, float p_light_energy, float p_sun_scatter, float p_density, float p_height, float p_height_density, float p_fog_aerial_perspective, float p_sky_affect, RSE::EnvironmentFogMode p_mode) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL(env);
	env->fog_enabled = p_enable;
	env->fog_mode = p_mode;
	env->fog_light_color = p_light_color;
	env->fog_light_energy = p_light_energy;
	env->fog_sun_scatter = p_sun_scatter;
	env->fog_density = p_density;
	env->fog_height = p_height;
	env->fog_height_density = p_height_density;
	env->fog_aerial_perspective = p_fog_aerial_perspective;
	env->fog_sky_affect = p_sky_affect;
}

bool RendererEnvironmentStorage::environment_get_fog_enabled(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, false);
	return env->fog_enabled;
}

RSE::EnvironmentFogMode RendererEnvironmentStorage::environment_get_fog_mode(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, RSE::ENV_FOG_MODE_EXPONENTIAL);
	return env->fog_mode;
}

Color RendererEnvironmentStorage::environment_get_fog_light_color(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, Color(0.5, 0.6, 0.7));
	return env->fog_light_color;
}

float RendererEnvironmentStorage::environment_get_fog_light_energy(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 1.0);
	return env->fog_light_energy;
}

float RendererEnvironmentStorage::environment_get_fog_sun_scatter(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.0);
	return env->fog_sun_scatter;
}

float RendererEnvironmentStorage::environment_get_fog_density(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.001);
	return env->fog_density;
}

float RendererEnvironmentStorage::environment_get_fog_height(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.0);
	return env->fog_height;
}

float RendererEnvironmentStorage::environment_get_fog_height_density(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.0);
	return env->fog_height_density;
}

float RendererEnvironmentStorage::environment_get_fog_aerial_perspective(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.0);
	return env->fog_aerial_perspective;
}

float RendererEnvironmentStorage::environment_get_fog_sky_affect(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.0);
	return env->fog_sky_affect;
}

// Depth Fog

void RendererEnvironmentStorage::environment_set_fog_depth(RID p_env, float p_curve, float p_begin, float p_end) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL(env);
	env->fog_depth_curve = p_curve;
	env->fog_depth_begin = p_begin;
	env->fog_depth_end = p_end;
}

float RendererEnvironmentStorage::environment_get_fog_depth_curve(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.0);
	return env->fog_depth_curve;
}

float RendererEnvironmentStorage::environment_get_fog_depth_begin(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.0);
	return env->fog_depth_begin;
}

float RendererEnvironmentStorage::environment_get_fog_depth_end(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.0);
	return env->fog_depth_end;
}

// Volumetric Fog

void RendererEnvironmentStorage::environment_set_volumetric_fog(RID p_env, bool p_enable, float p_density, const Color &p_albedo, const Color &p_emission, float p_emission_energy, float p_anisotropy, float p_length, float p_detail_spread, float p_gi_inject, bool p_temporal_reprojection, float p_temporal_reprojection_amount, float p_ambient_inject, float p_sky_affect) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL(env);
#ifdef DEBUG_ENABLED
	if (OS::get_current_rendering_method() != "forward_plus" && p_enable) {
		WARN_PRINT_ONCE_ED("Volumetric fog is only available when using the Forward+ renderer.");
	}
#endif
	env->volumetric_fog_enabled = p_enable;
	env->volumetric_fog_density = p_density;
	env->volumetric_fog_scattering = p_albedo;
	env->volumetric_fog_emission = p_emission;
	env->volumetric_fog_emission_energy = p_emission_energy;
	env->volumetric_fog_anisotropy = p_anisotropy;
	env->volumetric_fog_length = p_length;
	env->volumetric_fog_detail_spread = p_detail_spread;
	env->volumetric_fog_gi_inject = p_gi_inject;
	env->volumetric_fog_temporal_reprojection = p_temporal_reprojection;
	env->volumetric_fog_temporal_reprojection_amount = p_temporal_reprojection_amount;
	env->volumetric_fog_ambient_inject = p_ambient_inject;
	env->volumetric_fog_sky_affect = p_sky_affect;
}

bool RendererEnvironmentStorage::environment_get_volumetric_fog_enabled(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, false);
	return env->volumetric_fog_enabled;
}

float RendererEnvironmentStorage::environment_get_volumetric_fog_density(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.01);
	return env->volumetric_fog_density;
}

Color RendererEnvironmentStorage::environment_get_volumetric_fog_scattering(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, Color(1, 1, 1));
	return env->volumetric_fog_scattering;
}

Color RendererEnvironmentStorage::environment_get_volumetric_fog_emission(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, Color(0, 0, 0));
	return env->volumetric_fog_emission;
}

float RendererEnvironmentStorage::environment_get_volumetric_fog_emission_energy(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.0);
	return env->volumetric_fog_emission_energy;
}

float RendererEnvironmentStorage::environment_get_volumetric_fog_anisotropy(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.2);
	return env->volumetric_fog_anisotropy;
}

float RendererEnvironmentStorage::environment_get_volumetric_fog_length(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 64.0);
	return env->volumetric_fog_length;
}

float RendererEnvironmentStorage::environment_get_volumetric_fog_detail_spread(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 2.0);
	return env->volumetric_fog_detail_spread;
}

float RendererEnvironmentStorage::environment_get_volumetric_fog_gi_inject(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.0);
	return env->volumetric_fog_gi_inject;
}

float RendererEnvironmentStorage::environment_get_volumetric_fog_sky_affect(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.0);
	return env->volumetric_fog_sky_affect;
}

bool RendererEnvironmentStorage::environment_get_volumetric_fog_temporal_reprojection(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, true);
	return env->volumetric_fog_temporal_reprojection;
}

float RendererEnvironmentStorage::environment_get_volumetric_fog_temporal_reprojection_amount(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.9);
	return env->volumetric_fog_temporal_reprojection_amount;
}

float RendererEnvironmentStorage::environment_get_volumetric_fog_ambient_inject(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.0);
	return env->volumetric_fog_ambient_inject;
}

// GLOW

void RendererEnvironmentStorage::environment_set_glow(RID p_env, bool p_enable, Vector<float> p_levels, float p_intensity, float p_strength, float p_mix, float p_bloom_threshold, RSE::EnvironmentGlowBlendMode p_blend_mode, float p_hdr_bleed_threshold, float p_hdr_bleed_scale, float p_hdr_luminance_cap, float p_glow_map_strength, RID p_glow_map) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL(env);
	ERR_FAIL_COND_MSG(p_levels.size() != 7, "Size of array of glow levels must be 7");
	env->glow_enabled = p_enable;
	env->glow_levels = p_levels;
	env->glow_intensity = p_intensity;
	env->glow_strength = p_strength;
	env->glow_mix = p_mix;
	env->glow_bloom = p_bloom_threshold;
	env->glow_blend_mode = p_blend_mode;
	env->glow_hdr_bleed_threshold = p_hdr_bleed_threshold;
	env->glow_hdr_bleed_scale = p_hdr_bleed_scale;
	env->glow_hdr_luminance_cap = p_hdr_luminance_cap;
	env->glow_map_strength = p_glow_map_strength;
	env->glow_map = p_glow_map;
}

bool RendererEnvironmentStorage::environment_get_glow_enabled(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, false);
	return env->glow_enabled;
}

Vector<float> RendererEnvironmentStorage::environment_get_glow_levels(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, Vector<float>());
	return env->glow_levels;
}

float RendererEnvironmentStorage::environment_get_glow_intensity(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.3);
	return env->glow_intensity;
}

float RendererEnvironmentStorage::environment_get_glow_strength(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 1.0);
	return env->glow_strength;
}

float RendererEnvironmentStorage::environment_get_glow_bloom(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.0);
	return env->glow_bloom;
}

float RendererEnvironmentStorage::environment_get_glow_mix(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.01);
	return env->glow_mix;
}

RSE::EnvironmentGlowBlendMode RendererEnvironmentStorage::environment_get_glow_blend_mode(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, RSE::ENV_GLOW_BLEND_MODE_SCREEN);
	return env->glow_blend_mode;
}

float RendererEnvironmentStorage::environment_get_glow_hdr_bleed_threshold(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 1.0);
	return env->glow_hdr_bleed_threshold;
}

float RendererEnvironmentStorage::environment_get_glow_hdr_luminance_cap(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 12.0);
	return env->glow_hdr_luminance_cap;
}

float RendererEnvironmentStorage::environment_get_glow_hdr_bleed_scale(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 2.0);
	return env->glow_hdr_bleed_scale;
}

float RendererEnvironmentStorage::environment_get_glow_map_strength(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.0);
	return env->glow_map_strength;
}

RID RendererEnvironmentStorage::environment_get_glow_map(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, RID());
	return env->glow_map;
}

// SSR

void RendererEnvironmentStorage::environment_set_ssr(RID p_env, bool p_enable, int p_max_steps, float p_fade_int, float p_fade_out, float p_depth_tolerance) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL(env);
#ifdef DEBUG_ENABLED
	if (OS::get_current_rendering_method() != "forward_plus" && p_enable) {
		WARN_PRINT_ONCE_ED("Screen-space reflections (SSR) are only available when using the Forward+ renderer.");
	}
#endif
	env->ssr_enabled = p_enable;
	env->ssr_max_steps = p_max_steps;
	env->ssr_fade_in = p_fade_int;
	env->ssr_fade_out = p_fade_out;
	env->ssr_depth_tolerance = p_depth_tolerance;
}

bool RendererEnvironmentStorage::environment_get_ssr_enabled(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, false);
	return env->ssr_enabled;
}

int RendererEnvironmentStorage::environment_get_ssr_max_steps(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 64);
	return env->ssr_max_steps;
}

float RendererEnvironmentStorage::environment_get_ssr_fade_in(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.15);
	return env->ssr_fade_in;
}

float RendererEnvironmentStorage::environment_get_ssr_fade_out(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 2.0);
	return env->ssr_fade_out;
}

float RendererEnvironmentStorage::environment_get_ssr_depth_tolerance(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.5);
	return env->ssr_depth_tolerance;
}

// SSAO

void RendererEnvironmentStorage::environment_set_ssao(RID p_env, bool p_enable, float p_radius, float p_intensity, float p_power, float p_detail, float p_horizon, float p_sharpness, float p_light_affect, float p_ao_channel_affect) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL(env);
#ifdef DEBUG_ENABLED
	if (OS::get_current_rendering_method() == "mobile" && p_enable) {
		WARN_PRINT_ONCE_ED("Screen-space ambient occlusion (SSAO) is only available when using the Forward+ or Compatibility renderers.");
	}
#endif
	env->ssao_enabled = p_enable;
	env->ssao_radius = p_radius;
	env->ssao_intensity = p_intensity;
	env->ssao_power = p_power;
	env->ssao_detail = p_detail;
	env->ssao_horizon = p_horizon;
	env->ssao_sharpness = p_sharpness;
	env->ssao_direct_light_affect = p_light_affect;
	env->ssao_ao_channel_affect = p_ao_channel_affect;
}

bool RendererEnvironmentStorage::environment_get_ssao_enabled(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, false);
	return env->ssao_enabled;
}

float RendererEnvironmentStorage::environment_get_ssao_radius(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 1.0);
	return env->ssao_radius;
}

float RendererEnvironmentStorage::environment_get_ssao_intensity(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 2.0);
	return env->ssao_intensity;
}

float RendererEnvironmentStorage::environment_get_ssao_power(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 1.5);
	return env->ssao_power;
}

float RendererEnvironmentStorage::environment_get_ssao_detail(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.5);
	return env->ssao_detail;
}

float RendererEnvironmentStorage::environment_get_ssao_horizon(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.06);
	return env->ssao_horizon;
}

float RendererEnvironmentStorage::environment_get_ssao_sharpness(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.98);
	return env->ssao_sharpness;
}

float RendererEnvironmentStorage::environment_get_ssao_direct_light_affect(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.0);
	return env->ssao_direct_light_affect;
}

float RendererEnvironmentStorage::environment_get_ssao_ao_channel_affect(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.0);
	return env->ssao_ao_channel_affect;
}

// SSIL

void RendererEnvironmentStorage::environment_set_ssil(RID p_env, bool p_enable, float p_radius, float p_intensity, float p_sharpness, float p_normal_rejection) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL(env);
#ifdef DEBUG_ENABLED
	if (OS::get_current_rendering_method() != "forward_plus" && p_enable) {
		WARN_PRINT_ONCE_ED("Screen-space indirect lighting (SSIL) is only available when using the Forward+ renderer.");
	}
#endif
	env->ssil_enabled = p_enable;
	env->ssil_radius = p_radius;
	env->ssil_intensity = p_intensity;
	env->ssil_sharpness = p_sharpness;
	env->ssil_normal_rejection = p_normal_rejection;
}

bool RendererEnvironmentStorage::environment_get_ssil_enabled(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, false);
	return env->ssil_enabled;
}

float RendererEnvironmentStorage::environment_get_ssil_radius(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 5.0);
	return env->ssil_radius;
}

float RendererEnvironmentStorage::environment_get_ssil_intensity(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 1.0);
	return env->ssil_intensity;
}

float RendererEnvironmentStorage::environment_get_ssil_sharpness(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.98);
	return env->ssil_sharpness;
}

float RendererEnvironmentStorage::environment_get_ssil_normal_rejection(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 1.0);
	return env->ssil_normal_rejection;
}

// SDFGI

void RendererEnvironmentStorage::environment_set_sdfgi(RID p_env, bool p_enable, int p_cascades, float p_min_cell_size, RSE::EnvironmentSDFGIYScale p_y_scale, bool p_use_occlusion, float p_bounce_feedback, bool p_read_sky, float p_energy, float p_normal_bias, float p_probe_bias) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL(env);
#ifdef DEBUG_ENABLED
	if (OS::get_current_rendering_method() != "forward_plus" && p_enable) {
		WARN_PRINT_ONCE_ED("SDFGI is only available when using the Forward+ renderer.");
	}
#endif
	env->sdfgi_enabled = p_enable;
	env->sdfgi_cascades = p_cascades;
	env->sdfgi_min_cell_size = p_min_cell_size;
	env->sdfgi_use_occlusion = p_use_occlusion;
	env->sdfgi_bounce_feedback = p_bounce_feedback;
	env->sdfgi_read_sky_light = p_read_sky;
	env->sdfgi_energy = p_energy;
	env->sdfgi_normal_bias = p_normal_bias;
	env->sdfgi_probe_bias = p_probe_bias;
	env->sdfgi_y_scale = p_y_scale;
}

bool RendererEnvironmentStorage::environment_get_sdfgi_enabled(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, false);
	return env->sdfgi_enabled;
}

int RendererEnvironmentStorage::environment_get_sdfgi_cascades(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 4);
	return env->sdfgi_cascades;
}

float RendererEnvironmentStorage::environment_get_sdfgi_min_cell_size(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.2);
	return env->sdfgi_min_cell_size;
}

bool RendererEnvironmentStorage::environment_get_sdfgi_use_occlusion(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, false);
	return env->sdfgi_use_occlusion;
}

float RendererEnvironmentStorage::environment_get_sdfgi_bounce_feedback(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 0.5);
	return env->sdfgi_bounce_feedback;
}

bool RendererEnvironmentStorage::environment_get_sdfgi_read_sky_light(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, true);
	return env->sdfgi_read_sky_light;
}

float RendererEnvironmentStorage::environment_get_sdfgi_energy(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 1.0);
	return env->sdfgi_energy;
}

float RendererEnvironmentStorage::environment_get_sdfgi_normal_bias(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 1.1);
	return env->sdfgi_normal_bias;
}

float RendererEnvironmentStorage::environment_get_sdfgi_probe_bias(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 1.1);
	return env->sdfgi_probe_bias;
}

RSE::EnvironmentSDFGIYScale RendererEnvironmentStorage::environment_get_sdfgi_y_scale(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, RSE::ENV_SDFGI_Y_SCALE_75_PERCENT);
	return env->sdfgi_y_scale;
}

// Adjustments

void RendererEnvironmentStorage::environment_set_adjustment(RID p_env, bool p_enable, float p_brightness, float p_contrast, float p_saturation, bool p_use_1d_color_correction, RID p_color_correction) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL(env);

	env->adjustments_enabled = p_enable;
	// Scale brightness via the nonlinear sRGB transfer function to provide a
	// somewhat perceptually uniform brightness adjustment.
	env->adjustments_brightness = p_brightness < 0.04045f ? p_brightness * (1.0f / 12.92f) : Math::pow(float((p_brightness + 0.055f) * (1.0f / (1.055f))), 2.4f);
	env->adjustments_contrast = p_contrast;
	env->adjustments_saturation = p_saturation;
	env->use_1d_color_correction = p_use_1d_color_correction;
	env->color_correction = p_color_correction;
}

bool RendererEnvironmentStorage::environment_get_adjustments_enabled(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, false);
	return env->adjustments_enabled;
}

float RendererEnvironmentStorage::environment_get_adjustments_brightness(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 1.0);
	return env->adjustments_brightness;
}

float RendererEnvironmentStorage::environment_get_adjustments_contrast(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 1.0);
	return env->adjustments_contrast;
}

float RendererEnvironmentStorage::environment_get_adjustments_saturation(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, 1.0);
	return env->adjustments_saturation;
}

bool RendererEnvironmentStorage::environment_get_use_1d_color_correction(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, false);
	return env->use_1d_color_correction;
}

RID RendererEnvironmentStorage::environment_get_color_correction(RID p_env) {
	Environment *env = environment_owner.get_or_null(p_env);
	ERR_FAIL_NULL_V(env, RID());
	return env->color_correction;
}

// --- Merged from servers/rendering/storage/mesh_storage.cpp ---

void RendererMeshStorage::multimesh_allocate_data(RID p_multimesh, int p_instances,
	RSE::MultimeshTransformFormat p_transform_format, bool p_use_colors, bool p_use_custom_data,
	bool p_use_indirect)
{
	MultiMeshInterpolator* mmi = _multimesh_get_interpolator(p_multimesh);
	if (mmi) {
		mmi->_transform_format = p_transform_format;
		mmi->_use_colors = p_use_colors;
		mmi->_use_custom_data = p_use_custom_data;
		mmi->_num_instances = p_instances;

		mmi->_vf_size_xform = p_transform_format == RSE::MULTIMESH_TRANSFORM_2D ? 8 : 12;
		mmi->_vf_size_color = p_use_colors ? 4 : 0;
		mmi->_vf_size_data = p_use_custom_data ? 4 : 0;

		mmi->_stride = mmi->_vf_size_xform + mmi->_vf_size_color + mmi->_vf_size_data;

		int size_in_floats = p_instances * mmi->_stride;
		mmi->_data_curr.resize_initialized(size_in_floats);
		mmi->_data_prev.resize_initialized(size_in_floats);
		mmi->_data_interpolated.resize_initialized(size_in_floats);
	}

	_multimesh_allocate_data(p_multimesh, p_instances, p_transform_format, p_use_colors,
		p_use_custom_data, p_use_indirect);
}

int RendererMeshStorage::multimesh_get_instance_count(RID p_multimesh)
{
	return _multimesh_get_instance_count(p_multimesh);
}

void RendererMeshStorage::multimesh_set_mesh(RID p_multimesh, RID p_mesh)
{
	_multimesh_set_mesh(p_multimesh, p_mesh);
}

void RendererMeshStorage::multimesh_instance_set_transform(
	RID p_multimesh, int p_index, const Transform3D& p_transform)
{
	MultiMeshInterpolator* mmi = _multimesh_get_interpolator(p_multimesh);
	if (mmi && mmi->interpolated) {
		ERR_FAIL_COND(p_index >= mmi->_num_instances);
		ERR_FAIL_COND(mmi->_vf_size_xform != 12);

		int start = p_index * mmi->_stride;
		float* ptr = mmi->_data_curr.ptrw();
		ptr += start;

		const Transform3D& t = p_transform;
		ptr[0] = t.basis.rows[0][0];
		ptr[1] = t.basis.rows[0][1];
		ptr[2] = t.basis.rows[0][2];
		ptr[3] = t.origin.x;
		ptr[4] = t.basis.rows[1][0];
		ptr[5] = t.basis.rows[1][1];
		ptr[6] = t.basis.rows[1][2];
		ptr[7] = t.origin.y;
		ptr[8] = t.basis.rows[2][0];
		ptr[9] = t.basis.rows[2][1];
		ptr[10] = t.basis.rows[2][2];
		ptr[11] = t.origin.z;

		_multimesh_add_to_interpolation_lists(p_multimesh, *mmi);

		return;
	}

	_multimesh_instance_set_transform(p_multimesh, p_index, p_transform);
}

void RendererMeshStorage::multimesh_instance_set_transform_2d(
	RID p_multimesh, int p_index, const Transform2D& p_transform)
{
	MultiMeshInterpolator* mmi = _multimesh_get_interpolator(p_multimesh);
	if (mmi && mmi->interpolated) {
		ERR_FAIL_COND(p_index >= mmi->_num_instances);
		ERR_FAIL_COND(mmi->_vf_size_xform != 8);

		int start = p_index * mmi->_stride;
		float* ptr = mmi->_data_curr.ptrw();
		ptr += start;

		const Transform2D& t = p_transform;

		ptr[0] = t.columns[0][0];
		ptr[1] = t.columns[1][0];
		ptr[2] = 0;
		ptr[3] = t.columns[2][0];
		ptr[4] = t.columns[0][1];
		ptr[5] = t.columns[1][1];
		ptr[6] = 0;
		ptr[7] = t.columns[2][1];

		_multimesh_add_to_interpolation_lists(p_multimesh, *mmi);

		return;
	}

	_multimesh_instance_set_transform_2d(p_multimesh, p_index, p_transform);
}

void RendererMeshStorage::multimesh_instance_set_color(
	RID p_multimesh, int p_index, const Color& p_color)
{
	MultiMeshInterpolator* mmi = _multimesh_get_interpolator(p_multimesh);
	if (mmi && mmi->interpolated) {
		ERR_FAIL_COND(p_index >= mmi->_num_instances);
		ERR_FAIL_COND(mmi->_vf_size_color == 0);

		int start = (p_index * mmi->_stride) + mmi->_vf_size_xform;
		float* ptr = mmi->_data_curr.ptrw();
		ptr += start;

		if (mmi->_vf_size_color == 4) {
			for (int n = 0; n < 4; n++) {
				ptr[n] = p_color[n];
			}
		}
		else {
#ifdef DEV_ENABLED
			// The options are currently 4 or zero, but just in case this changes in future...
			ERR_FAIL_COND(mmi->_vf_size_color != 0);
#endif
		}
		_multimesh_add_to_interpolation_lists(p_multimesh, *mmi);
		return;
	}

	_multimesh_instance_set_color(p_multimesh, p_index, p_color);
}

void RendererMeshStorage::multimesh_instance_set_custom_data(
	RID p_multimesh, int p_index, const Color& p_color)
{
	MultiMeshInterpolator* mmi = _multimesh_get_interpolator(p_multimesh);
	if (mmi && mmi->interpolated) {
		ERR_FAIL_COND(p_index >= mmi->_num_instances);
		ERR_FAIL_COND(mmi->_vf_size_data == 0);

		int start = (p_index * mmi->_stride) + mmi->_vf_size_xform + mmi->_vf_size_color;
		float* ptr = mmi->_data_curr.ptrw();
		ptr += start;

		if (mmi->_vf_size_data == 4) {
			for (int n = 0; n < 4; n++) {
				ptr[n] = p_color[n];
			}
		}
		else {
#ifdef DEV_ENABLED
			// The options are currently 4 or zero, but just in case this changes in future...
			ERR_FAIL_COND(mmi->_vf_size_data != 0);
#endif
		}
		_multimesh_add_to_interpolation_lists(p_multimesh, *mmi);
		return;
	}

	_multimesh_instance_set_custom_data(p_multimesh, p_index, p_color);
}

void RendererMeshStorage::multimesh_set_custom_aabb(RID p_multimesh, const AABB& p_aabb)
{
	_multimesh_set_custom_aabb(p_multimesh, p_aabb);
}

AABB RendererMeshStorage::multimesh_get_custom_aabb(RID p_multimesh)
{
	return _multimesh_get_custom_aabb(p_multimesh);
}

RID RendererMeshStorage::multimesh_get_mesh(RID p_multimesh)
{
	return _multimesh_get_mesh(p_multimesh);
}

Transform3D RendererMeshStorage::multimesh_instance_get_transform(
	RID p_multimesh, int p_index)
{
	return _multimesh_instance_get_transform(p_multimesh, p_index);
}

Transform2D RendererMeshStorage::multimesh_instance_get_transform_2d(
	RID p_multimesh, int p_index)
{
	return _multimesh_instance_get_transform_2d(p_multimesh, p_index);
}

Color RendererMeshStorage::multimesh_instance_get_color(RID p_multimesh, int p_index)
{
	return _multimesh_instance_get_color(p_multimesh, p_index);
}

Color RendererMeshStorage::multimesh_instance_get_custom_data(RID p_multimesh, int p_index)
{
	return _multimesh_instance_get_custom_data(p_multimesh, p_index);
}

void RendererMeshStorage::multimesh_set_buffer(RID p_multimesh, const Vector<float>& p_buffer)
{
	MultiMeshInterpolator* mmi = _multimesh_get_interpolator(p_multimesh);
	if (mmi && mmi->interpolated) {
		ERR_FAIL_COND_MSG(p_buffer.size() != mmi->_data_curr.size(),
			"Buffer should have " + itos(mmi->_data_curr.size()) + " elements, got " +
				itos(p_buffer.size()) + " instead.");

		mmi->_data_curr = p_buffer;
		_multimesh_add_to_interpolation_lists(p_multimesh, *mmi);

		return;
	}

	_multimesh_set_buffer(p_multimesh, p_buffer);
}

RID RendererMeshStorage::multimesh_get_command_buffer_rd_rid(RID p_multimesh)
{
	return _multimesh_get_command_buffer_rd_rid(p_multimesh);
}

RID RendererMeshStorage::multimesh_get_buffer_rd_rid(RID p_multimesh)
{
	return _multimesh_get_buffer_rd_rid(p_multimesh);
}

Vector<float> RendererMeshStorage::multimesh_get_buffer(RID p_multimesh)
{
	return _multimesh_get_buffer(p_multimesh);
}

void RendererMeshStorage::multimesh_set_buffer_interpolated(
	RID p_multimesh, const Vector<float>& p_buffer, const Vector<float>& p_buffer_prev)
{
	MultiMeshInterpolator* mmi = _multimesh_get_interpolator(p_multimesh);
	if (mmi) {
		ERR_FAIL_COND_MSG(p_buffer.size() != mmi->_data_curr.size(),
			"Buffer for current frame should have " + itos(mmi->_data_curr.size()) +
				" elements, got " + itos(p_buffer.size()) + " instead.");
		ERR_FAIL_COND_MSG(p_buffer_prev.size() != mmi->_data_prev.size(),
			"Buffer for previous frame should have " + itos(mmi->_data_prev.size()) +
				" elements, got " + itos(p_buffer_prev.size()) + " instead.");

		// We are assuming that mmi->interpolated is the case. (Can possibly assert this?)
		// Even if this flag hasn't been set - just calling this function suggests interpolation is
		// desired.
		mmi->_data_prev = p_buffer_prev;
		mmi->_data_curr = p_buffer;
		_multimesh_add_to_interpolation_lists(p_multimesh, *mmi);
	}
}

void RendererMeshStorage::multimesh_set_physics_interpolated(RID p_multimesh, bool p_interpolated)
{
	MultiMeshInterpolator* mmi = _multimesh_get_interpolator(p_multimesh);
	if (mmi) {
		if (p_interpolated == mmi->interpolated) {
			return;
		}

		mmi->interpolated = p_interpolated;

		// If we are turning on physics interpolation, as a convenience,
		// we want to get the current buffer data from the backend,
		// and reset all the instances.
		if (p_interpolated) {
			mmi->_data_curr = _multimesh_get_buffer(p_multimesh);
			mmi->_data_prev = mmi->_data_curr;
			mmi->_data_interpolated = mmi->_data_curr;
		}
	}
}

void RendererMeshStorage::multimesh_set_physics_interpolation_quality(
	RID p_multimesh, RSE::MultimeshPhysicsInterpolationQuality p_quality)
{
	ERR_FAIL_COND((p_quality < 0) || (p_quality > 1));
	MultiMeshInterpolator* mmi = _multimesh_get_interpolator(p_multimesh);
	if (mmi) {
		mmi->quality = (int)p_quality;
	}
}

void RendererMeshStorage::multimesh_instance_reset_physics_interpolation(
	RID p_multimesh, int p_index)
{
	MultiMeshInterpolator* mmi = _multimesh_get_interpolator(p_multimesh);
	if (mmi) {
		ERR_FAIL_INDEX(p_index, mmi->_num_instances);

		float* w = mmi->_data_prev.ptrw();
		const float* r = mmi->_data_curr.ptr();
		int start = p_index * mmi->_stride;

		for (int n = 0; n < mmi->_stride; n++) {
			w[start + n] = r[start + n];
		}
	}
}

void RendererMeshStorage::multimesh_instances_reset_physics_interpolation(RID p_multimesh)
{
	MultiMeshInterpolator* mmi = _multimesh_get_interpolator(p_multimesh);
	if (mmi && mmi->_data_curr.size()) {
		// We don't want to invoke COW here, so copy the data directly.
		ERR_FAIL_COND(mmi->_data_prev.size() != mmi->_data_curr.size());
		float* w = mmi->_data_prev.ptrw();
		const float* r = mmi->_data_curr.ptr();
		memcpy(w, r, sizeof(float) * mmi->_data_curr.size());
	}
}

void RendererMeshStorage::multimesh_set_visible_instances(RID p_multimesh, int p_visible)
{
	return _multimesh_set_visible_instances(p_multimesh, p_visible);
}

int RendererMeshStorage::multimesh_get_visible_instances(RID p_multimesh)
{
	return _multimesh_get_visible_instances(p_multimesh);
}

AABB RendererMeshStorage::multimesh_get_aabb(RID p_multimesh)
{
	return _multimesh_get_aabb(p_multimesh);
}

void RendererMeshStorage::_multimesh_add_to_interpolation_lists(
	RID p_multimesh, MultiMeshInterpolator& r_mmi)
{
	if (!r_mmi.on_interpolate_update_list) {
		r_mmi.on_interpolate_update_list = true;
		_interpolation_data.multimesh_interpolate_update_list.push_back(p_multimesh);
	}

	if (!r_mmi.on_transform_update_list) {
		r_mmi.on_transform_update_list = true;
		_interpolation_data.multimesh_transform_update_list_curr->push_back(p_multimesh);
	}
}

void InterpolationData::notify_free_multimesh(RID p_rid)
{
	// If the instance was on any of the lists, remove.
	multimesh_interpolate_update_list.erase_multiple_unordered(p_rid);
	multimesh_transform_update_lists[0].erase_multiple_unordered(p_rid);
	multimesh_transform_update_lists[1].erase_multiple_unordered(p_rid);
}

void RendererMeshStorage::update_interpolation_tick(bool p_process)
{
	// Detect any that were on the previous transform list that are no longer active,
	// we should remove them from the interpolate list.

	for (unsigned int n = 0; n < _interpolation_data.multimesh_transform_update_list_prev->size();
		 n++) {
		const RID& rid = (*_interpolation_data.multimesh_transform_update_list_prev)[n];

		bool active = true;

		// No longer active? (Either the instance deleted or no longer being transformed.)

		MultiMeshInterpolator* mmi = _multimesh_get_interpolator(rid);
		if (mmi && !mmi->on_transform_update_list) {
			active = false;
			mmi->on_interpolate_update_list = false;

			// Make sure the most recent transform is set...
			mmi->_data_interpolated =
				mmi->_data_curr; // TODO: Copy data rather than use Packed = function?

			// ... and that both prev and current are the same, just in case of any interpolations.
			mmi->_data_prev = mmi->_data_curr;

			// Update the actual stable buffer to the backend.
			_multimesh_set_buffer(rid, mmi->_data_interpolated);
		}

		if (!mmi) {
			active = false;
		}

		if (!active) {
			_interpolation_data.multimesh_interpolate_update_list.erase(rid);
		}
	}

	if (p_process) {
		for (unsigned int i = 0;
			 i < _interpolation_data.multimesh_transform_update_list_curr->size(); i++) {
			const RID& rid = (*_interpolation_data.multimesh_transform_update_list_curr)[i];

			MultiMeshInterpolator* mmi = _multimesh_get_interpolator(rid);
			if (mmi) {
				// Reset for next tick.
				mmi->on_transform_update_list = false;
				mmi->_data_prev = mmi->_data_curr;
			}
		}
	}

	// If any have left the transform list, remove from the interpolate list.

	// We maintain a mirror list for the transform updates, so we can detect when an instance
	// is no longer being transformed, and remove it from the interpolate list.
	SWAP(_interpolation_data.multimesh_transform_update_list_curr,
		_interpolation_data.multimesh_transform_update_list_prev);

	// Prepare for the next iteration.
	_interpolation_data.multimesh_transform_update_list_curr->clear();
}

void RendererMeshStorage::update_interpolation_frame(bool p_process)
{
	if (p_process) {
		// Only need 32 bits for interpolation, don't use real_t.
		float f = Engine::get_physics_interpolation_fraction();

		for (unsigned int c = 0; c < _interpolation_data.multimesh_interpolate_update_list.size();
			 c++) {
			const RID& rid = _interpolation_data.multimesh_interpolate_update_list[c];

			// We could use the TransformInterpolator here to slerp transforms, but that might be
			// too expensive, so just using a Basis lerp for now.
			MultiMeshInterpolator* mmi = _multimesh_get_interpolator(rid);
			if (mmi) {
				// Make sure arrays are the correct size.
				DEV_ASSERT(mmi->_data_prev.size() == mmi->_data_curr.size());

				if (mmi->_data_interpolated.size() < mmi->_data_curr.size()) {
					mmi->_data_interpolated.resize(mmi->_data_curr.size());
				}
				DEV_ASSERT(mmi->_data_interpolated.size() >= mmi->_data_curr.size());

				DEV_ASSERT((mmi->_data_curr.size() % mmi->_stride) == 0);
				int num = mmi->_data_curr.size() / mmi->_stride;

				const float* pf_prev = mmi->_data_prev.ptr();
				const float* pf_curr = mmi->_data_curr.ptr();
				float* pf_int = mmi->_data_interpolated.ptrw();

				bool use_lerp = mmi->quality == 0;

				// Temporary transform (needed for swizzling).
				Transform3D tp, tc, tr; // (transform prev, curr and result)

				// Test for cache friendliness versus doing branchless.
				for (int n = 0; n < num; n++) {
					// Transform.
					if (use_lerp) {
						for (int i = 0; i < mmi->_vf_size_xform; i++) {
							pf_int[i] = Math::lerp(pf_prev[i], pf_curr[i], f);
						}
					}
					else {
						// Silly swizzling, this will slow things down.
						// No idea why it is using this format...
						// ... maybe due to the shader.
						tp.basis.rows[0][0] = pf_prev[0];
						tp.basis.rows[0][1] = pf_prev[1];
						tp.basis.rows[0][2] = pf_prev[2];
						tp.basis.rows[1][0] = pf_prev[4];
						tp.basis.rows[1][1] = pf_prev[5];
						tp.basis.rows[1][2] = pf_prev[6];
						tp.basis.rows[2][0] = pf_prev[8];
						tp.basis.rows[2][1] = pf_prev[9];
						tp.basis.rows[2][2] = pf_prev[10];
						tp.origin.x = pf_prev[3];
						tp.origin.y = pf_prev[7];
						tp.origin.z = pf_prev[11];

						tc.basis.rows[0][0] = pf_curr[0];
						tc.basis.rows[0][1] = pf_curr[1];
						tc.basis.rows[0][2] = pf_curr[2];
						tc.basis.rows[1][0] = pf_curr[4];
						tc.basis.rows[1][1] = pf_curr[5];
						tc.basis.rows[1][2] = pf_curr[6];
						tc.basis.rows[2][0] = pf_curr[8];
						tc.basis.rows[2][1] = pf_curr[9];
						tc.basis.rows[2][2] = pf_curr[10];
						tc.origin.x = pf_curr[3];
						tc.origin.y = pf_curr[7];
						tc.origin.z = pf_curr[11];

						TransformInterpolator::interpolate_transform_3d(tp, tc, tr, f);

						pf_int[0] = tr.basis.rows[0][0];
						pf_int[1] = tr.basis.rows[0][1];
						pf_int[2] = tr.basis.rows[0][2];
						pf_int[4] = tr.basis.rows[1][0];
						pf_int[5] = tr.basis.rows[1][1];
						pf_int[6] = tr.basis.rows[1][2];
						pf_int[8] = tr.basis.rows[2][0];
						pf_int[9] = tr.basis.rows[2][1];
						pf_int[10] = tr.basis.rows[2][2];
						pf_int[3] = tr.origin.x;
						pf_int[7] = tr.origin.y;
						pf_int[11] = tr.origin.z;
					}

					pf_prev += mmi->_vf_size_xform;
					pf_curr += mmi->_vf_size_xform;
					pf_int += mmi->_vf_size_xform;

					// Color.
					if (mmi->_vf_size_color == 4) {
						for (int i = 0; i < 4; i++) {
							pf_int[i] = Math::lerp(pf_prev[i], pf_curr[i], f);
						}

						pf_prev += 4;
						pf_curr += 4;
						pf_int += 4;
					}

					// Custom data.
					if (mmi->_vf_size_data == 4) {
						for (int i = 0; i < 4; i++) {
							pf_int[i] = Math::lerp(pf_prev[i], pf_curr[i], f);
						}

						pf_prev += 4;
						pf_curr += 4;
						pf_int += 4;
					}
				}

				_multimesh_set_buffer(rid, mmi->_data_interpolated);

				// TODO: Make sure AABBs are constantly up to date through the interpolation?
				// NYI.
			}
		}
	}
}

// --- Merged from servers/rendering/storage/render_scene_buffers.cpp ---



// --- Merged from servers/rendering/storage/utilities.cpp ---

void Dependency::changed_notify(DependencyChangedNotification p_notification) {
	for (const KeyValue<DependencyTracker *, uint32_t> &E : instances) {
		if (E.key->changed_callback) {
			E.key->changed_callback(p_notification, E.key);
		}
	}
}

void Dependency::deleted_notify(const RID &p_rid) {
	for (const KeyValue<DependencyTracker *, uint32_t> &E : instances) {
		if (E.key->deleted_callback) {
			E.key->deleted_callback(p_rid, E.key);
		}
	}
	for (const KeyValue<DependencyTracker *, uint32_t> &E : instances) {
		E.key->dependencies.erase(this);
	}
	instances.clear();
}

Dependency::~Dependency() {
#ifdef DEBUG_ENABLED
	if (instances.size()) {
		WARN_PRINT("Leaked instance dependency: Bug - did not call instance_notify_deleted when freeing.");
		for (const KeyValue<DependencyTracker *, uint32_t> &E : instances) {
			E.key->dependencies.erase(this);
		}
	}
#endif
}
