/**************************************************************************/
/*  sky_material.cpp                                                      */
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
#include "core/version.h"
#include "scene/resources/texture.h"
#include "servers/rendering/rendering_server.h"
#include "sky_material.h"

Mutex ProceduralSkyMaterial::shader_mutex;
RID ProceduralSkyMaterial::shader_cache[4];

Color ProceduralSkyMaterial::get_sky_top_color() const { return sky_top_color; }

Color ProceduralSkyMaterial::get_sky_horizon_color() const { return sky_horizon_color; }

float ProceduralSkyMaterial::get_sky_curve() const { return sky_curve; }

float ProceduralSkyMaterial::get_sky_energy_multiplier() const { return sky_energy_multiplier; }

Ref<Texture2D> ProceduralSkyMaterial::get_sky_cover() const { return sky_cover; }

Color ProceduralSkyMaterial::get_sky_cover_modulate() const { return sky_cover_modulate; }

Color ProceduralSkyMaterial::get_ground_bottom_color() const { return ground_bottom_color; }

Color ProceduralSkyMaterial::get_ground_horizon_color() const { return ground_horizon_color; }

float ProceduralSkyMaterial::get_ground_curve() const { return ground_curve; }

float ProceduralSkyMaterial::get_ground_energy_multiplier() const
{
	return ground_energy_multiplier;
}

float ProceduralSkyMaterial::get_sun_angle_max() const { return sun_angle_max; }

float ProceduralSkyMaterial::get_sun_curve() const { return sun_curve; }

void ProceduralSkyMaterial::set_use_debanding(bool p_use_debanding)
{
	use_debanding = p_use_debanding;
	_update_shader(use_debanding, sky_cover.is_valid());
	// Only set if shader already compiled
	if (shader_set) {
		RS::get_singleton()->material_set_shader(_get_material(), get_shader_cache());
	}
}

bool ProceduralSkyMaterial::get_use_debanding() const { return use_debanding; }

float ProceduralSkyMaterial::get_energy_multiplier() const { return global_energy_multiplier; }

Shader::Mode ProceduralSkyMaterial::get_shader_mode() const { return Shader::MODE_SKY; }

// Internal function to grab the current shader RID.
// Must only be called if the shader is initialized.
RID ProceduralSkyMaterial::get_shader_cache() const
{
	return shader_cache[int(use_debanding) + (sky_cover.is_valid() ? 2 : 0)];
}

RID ProceduralSkyMaterial::get_rid() const
{
	_update_shader(use_debanding, sky_cover.is_valid());
	if (!shader_set) {
		RS::get_singleton()->material_set_shader(_get_material(), get_shader_cache());
		shader_set = true;
	}
	return _get_material();
}

RID ProceduralSkyMaterial::get_shader_rid() const
{
	_update_shader(use_debanding, sky_cover.is_valid());
	return get_shader_cache();
}

void ProceduralSkyMaterial::cleanup_shader()
{
	for (int i = 0; i < 4; i++) {
		if (shader_cache[i].is_valid()) {
			RS::get_singleton()->free_rid(shader_cache[i]);
		}
	}
}

void ProceduralSkyMaterial::_update_shader(bool p_use_debanding, bool p_use_sky_cover)
{
	MutexLock shader_lock(shader_mutex);
	int index = int(p_use_debanding) + int(p_use_sky_cover) * 2;
	if (shader_cache[index].is_null()) {
		shader_cache[index] = RS::get_singleton()->shader_create();

		// Add a comment to describe the shader origin (useful when converting to ShaderMaterial).
		RS::get_singleton()->shader_set_code(shader_cache[index],
			vformat(R"(
// NOTE: Shader automatically converted from )" VLTR_VERSION_NAME " " VLTR_VERSION_FULL_CONFIG
					R"('s ProceduralSkyMaterial.

shader_type sky;
%s

uniform vec4 sky_top_color : source_color = vec4(0.385, 0.454, 0.55, 1.0);
uniform vec4 sky_horizon_color : source_color = vec4(0.646, 0.656, 0.67, 1.0);
uniform float inv_sky_curve : hint_range(1, 100) = 4.0;
uniform vec4 ground_bottom_color : source_color = vec4(0.2, 0.169, 0.133, 1.0);
uniform vec4 ground_horizon_color : source_color = vec4(0.646, 0.656, 0.67, 1.0);
uniform float inv_ground_curve : hint_range(1, 100) = 30.0;
uniform float sun_angle_max = 0.877;
uniform float inv_sun_curve : hint_range(1, 100) = 22.78;
uniform float exposure : hint_range(0, 128) = 1.0;

uniform sampler2D sky_cover : filter_linear, source_color, hint_default_black;
uniform vec4 sky_cover_modulate : source_color = vec4(1.0, 1.0, 1.0, 1.0);

void sky() {
	float v_angle = clamp(EYEDIR.y, -1.0, 1.0);
	vec3 sky = mix(sky_top_color.rgb, sky_horizon_color.rgb, clamp(pow(1.0 - v_angle, inv_sky_curve), 0.0, 1.0));

	if (LIGHT0_ENABLED) {
		float sun_angle = dot(LIGHT0_DIRECTION, EYEDIR);
		float sun_size = cos(LIGHT0_SIZE);
		if (sun_angle > sun_size) {
			sky = LIGHT0_COLOR * LIGHT0_ENERGY;
		}
		else if (sun_angle > sun_angle_max) {
			float c2 = (sun_size - sun_angle) / (sun_size - sun_angle_max);
			sky = mix(sky, LIGHT0_COLOR * LIGHT0_ENERGY, clamp(pow(1.0 - c2, inv_sun_curve), 0.0, 1.0));
		}
	}

	if (LIGHT1_ENABLED) {
		float sun_angle = dot(LIGHT1_DIRECTION, EYEDIR);
		float sun_size = cos(LIGHT1_SIZE);
		if (sun_angle > sun_size) {
			sky = LIGHT1_COLOR * LIGHT1_ENERGY;
		}
		else if (sun_angle > sun_angle_max) {
			float c2 = (sun_size - sun_angle) / (sun_size - sun_angle_max);
			sky = mix(sky, LIGHT1_COLOR * LIGHT1_ENERGY, clamp(pow(1.0 - c2, inv_sun_curve), 0.0, 1.0));
		}
	}

	if (LIGHT2_ENABLED) {
		float sun_angle = dot(LIGHT2_DIRECTION, EYEDIR);
		float sun_size = cos(LIGHT2_SIZE);
		if (sun_angle > sun_size) {
			sky = LIGHT2_COLOR * LIGHT2_ENERGY;
		}
		else if (sun_angle > sun_angle_max) {
			float c2 = (sun_size - sun_angle) / (sun_size - sun_angle_max);
			sky = mix(sky, LIGHT2_COLOR * LIGHT2_ENERGY, clamp(pow(1.0 - c2, inv_sun_curve), 0.0, 1.0));
		}
	}

	if (LIGHT3_ENABLED) {
		float sun_angle = dot(LIGHT3_DIRECTION, EYEDIR);
		float sun_size = cos(LIGHT3_SIZE);
		if (sun_angle > sun_size) {
			sky = LIGHT3_COLOR * LIGHT3_ENERGY;
		}
		else if (sun_angle > sun_angle_max) {
			float c2 = (sun_size - sun_angle) / (sun_size - sun_angle_max);
			sky = mix(sky, LIGHT3_COLOR * LIGHT3_ENERGY, clamp(pow(1.0 - c2, inv_sun_curve), 0.0, 1.0));
		}
	}

	%s
	%s
	vec3 ground = mix(ground_bottom_color.rgb, ground_horizon_color.rgb, clamp(pow(1.0 + v_angle, inv_ground_curve), 0.0, 1.0));

	COLOR = mix(ground, sky, step(0.0, EYEDIR.y)) * exposure;
}
)",
				p_use_debanding ? "render_mode use_debanding;" : "",
				p_use_sky_cover ? "vec4 sky_cover_texture = texture(sky_cover, SKY_COORDS);" : "",
				p_use_sky_cover ? "sky += (sky_cover_texture.rgb * sky_cover_modulate.rgb) * "
								  "sky_cover_texture.a * sky_cover_modulate.a;"
								: ""));
	}
}

ProceduralSkyMaterial::ProceduralSkyMaterial()
{
	_set_material(RS::get_singleton()->material_create());
	set_sky_top_color(Color(0.385, 0.454, 0.55));
	set_sky_horizon_color(Color(0.6463, 0.6558, 0.6708));
	set_sky_curve(0.15);
	set_sky_energy_multiplier(1.0);
	set_sky_cover_modulate(Color(1, 1, 1));

	set_ground_bottom_color(Color(0.2, 0.169, 0.133));
	set_ground_horizon_color(Color(0.6463, 0.6558, 0.6708));
	set_ground_curve(0.02);
	set_ground_energy_multiplier(1.0);

	set_sun_angle_max(30.0);
	set_sun_curve(0.15);
	set_use_debanding(true);
	set_energy_multiplier(1.0);
}

ProceduralSkyMaterial::~ProceduralSkyMaterial() {}

Ref<Texture2D> PanoramaSkyMaterial::get_panorama() const { return panorama; }

bool PanoramaSkyMaterial::is_filtering_enabled() const { return filter; }

float PanoramaSkyMaterial::get_energy_multiplier() const { return energy_multiplier; }

Shader::Mode PanoramaSkyMaterial::get_shader_mode() const { return Shader::MODE_SKY; }

RID PanoramaSkyMaterial::get_rid() const
{
	_update_shader(filter);
	if (!shader_set) {
		RS::get_singleton()->material_set_shader(_get_material(), shader_cache[int(filter)]);
		shader_set = true;
	}
	return _get_material();
}

RID PanoramaSkyMaterial::get_shader_rid() const
{
	_update_shader(filter);
	return shader_cache[int(filter)];
}

Mutex PanoramaSkyMaterial::shader_mutex;
RID PanoramaSkyMaterial::shader_cache[2];

void PanoramaSkyMaterial::cleanup_shader()
{
	for (int i = 0; i < 2; i++) {
		if (shader_cache[i].is_valid()) {
			RS::get_singleton()->free_rid(shader_cache[i]);
		}
	}
}

void PanoramaSkyMaterial::_update_shader(bool p_filter)
{
	MutexLock shader_lock(shader_mutex);
	int index = int(p_filter);
	if (shader_cache[index].is_null()) {
		shader_cache[index] = RS::get_singleton()->shader_create();

		// Add a comment to describe the shader origin (useful when converting to ShaderMaterial).
		RS::get_singleton()->shader_set_code(
			shader_cache[index], vformat(R"(
// NOTE: Shader automatically converted from )" VLTR_VERSION_NAME " " VLTR_VERSION_FULL_CONFIG
										 R"('s PanoramaSkyMaterial.

shader_type sky;

uniform sampler2D source_panorama : %s, source_color, hint_default_black;
uniform float exposure : hint_range(0, 128) = 1.0;

void sky() {
	COLOR = texture(source_panorama, SKY_COORDS).rgb * exposure;
}
)",
									 p_filter ? "filter_linear" : "filter_nearest"));
	}
}

PanoramaSkyMaterial::PanoramaSkyMaterial()
{
	_set_material(RS::get_singleton()->material_create());
	set_energy_multiplier(1.0);
}

PanoramaSkyMaterial::~PanoramaSkyMaterial() {}

float PhysicalSkyMaterial::get_rayleigh_coefficient() const { return rayleigh; }

Color PhysicalSkyMaterial::get_rayleigh_color() const { return rayleigh_color; }

float PhysicalSkyMaterial::get_mie_coefficient() const { return mie; }

float PhysicalSkyMaterial::get_mie_eccentricity() const { return mie_eccentricity; }

Color PhysicalSkyMaterial::get_mie_color() const { return mie_color; }

float PhysicalSkyMaterial::get_turbidity() const { return turbidity; }

float PhysicalSkyMaterial::get_sun_disk_scale() const { return sun_disk_scale; }

Color PhysicalSkyMaterial::get_ground_color() const { return ground_color; }

float PhysicalSkyMaterial::get_energy_multiplier() const { return energy_multiplier; }

void PhysicalSkyMaterial::set_use_debanding(bool p_use_debanding)
{
	use_debanding = p_use_debanding;
	_update_shader(use_debanding, night_sky.is_valid());
	// Only set if shader already compiled
	if (shader_set) {
		RS::get_singleton()->material_set_shader(_get_material(), get_shader_cache());
	}
}

bool PhysicalSkyMaterial::get_use_debanding() const { return use_debanding; }

Ref<Texture2D> PhysicalSkyMaterial::get_night_sky() const { return night_sky; }

Shader::Mode PhysicalSkyMaterial::get_shader_mode() const { return Shader::MODE_SKY; }

// Internal function to grab the current shader RID.
// Must only be called if the shader is initialized.
RID PhysicalSkyMaterial::get_shader_cache() const
{
	return shader_cache[int(use_debanding) + (night_sky.is_valid() ? 2 : 0)];
}

RID PhysicalSkyMaterial::get_rid() const
{
	_update_shader(use_debanding, night_sky.is_valid());
	if (!shader_set) {
		RS::get_singleton()->material_set_shader(_get_material(), get_shader_cache());
		shader_set = true;
	}
	return _get_material();
}

RID PhysicalSkyMaterial::get_shader_rid() const
{
	_update_shader(use_debanding, night_sky.is_valid());
	return get_shader_cache();
}

Mutex PhysicalSkyMaterial::shader_mutex;
RID PhysicalSkyMaterial::shader_cache[4];

void PhysicalSkyMaterial::cleanup_shader()
{
	for (int i = 0; i < 4; i++) {
		if (shader_cache[i].is_valid()) {
			RS::get_singleton()->free_rid(shader_cache[i]);
		}
	}
}

void PhysicalSkyMaterial::_update_shader(bool p_use_debanding, bool p_use_night_sky)
{
	MutexLock shader_lock(shader_mutex);
	int index = int(p_use_debanding) + int(p_use_night_sky) * 2;
	if (shader_cache[index].is_null()) {
		shader_cache[index] = RS::get_singleton()->shader_create();

		// Add a comment to describe the shader origin (useful when converting to ShaderMaterial).
		RS::get_singleton()->shader_set_code(shader_cache[index],
			vformat(R"(
// NOTE: Shader automatically converted from )" VLTR_VERSION_NAME " " VLTR_VERSION_FULL_CONFIG
					R"('s PhysicalSkyMaterial.

shader_type sky;
%s

uniform float rayleigh : hint_range(0, 64) = 2.0;
uniform vec4 rayleigh_color : source_color = vec4(0.3, 0.405, 0.6, 1.0);
uniform float mie : hint_range(0, 1) = 0.005;
uniform float mie_eccentricity : hint_range(-1, 1) = 0.8;
uniform vec4 mie_color : source_color = vec4(0.69, 0.729, 0.812, 1.0);

uniform float turbidity : hint_range(0, 1000) = 10.0;
uniform float sun_disk_scale : hint_range(0, 360) = 1.0;
uniform vec4 ground_color : source_color = vec4(0.1, 0.07, 0.034, 1.0);
uniform float exposure : hint_range(0, 128) = 1.0;

uniform sampler2D night_sky : filter_linear, source_color, hint_default_black;

const vec3 UP = vec3( 0.0, 1.0, 0.0 );

// Optical length at zenith for molecules.
const float rayleigh_zenith_size = 8.4e3;
const float mie_zenith_size = 1.25e3;

float henyey_greenstein(float cos_theta, float g) {
	const float k = 0.0795774715459;
	return k * (1.0 - g * g) / (pow(1.0 + g * g - 2.0 * g * cos_theta, 1.5));
}

void sky() {
	if (LIGHT0_ENABLED) {
		float zenith_angle = clamp( dot(UP, normalize(LIGHT0_DIRECTION)), -1.0, 1.0 );
		float sun_energy = max(0.0, 0.757 * zenith_angle) * LIGHT0_ENERGY;
		float sun_fade = 1.0 - clamp(1.0 - exp(LIGHT0_DIRECTION.y), 0.0, 1.0);

		// Rayleigh coefficients.
		float rayleigh_coefficient = rayleigh - ( 1.0 * ( 1.0 - sun_fade ) );
		vec3 rayleigh_beta = rayleigh_coefficient * rayleigh_color.rgb * 0.0001;
		// mie coefficients from Preetham
		vec3 mie_beta = turbidity * mie * mie_color.rgb * 0.000434;

		// Optical length.
		float zenith = max(0.0, dot(UP, EYEDIR));
		float optical_mass = 1.0 / (zenith + 0.15 * pow(3.885 + 54.5 * zenith, -1.253));
		float rayleigh_scatter = rayleigh_zenith_size * optical_mass;
		float mie_scatter = mie_zenith_size * optical_mass;

		// Light extinction based on thickness of atmosphere.
		vec3 extinction = exp(-(rayleigh_beta * rayleigh_scatter + mie_beta * mie_scatter));

		// In scattering.
		float cos_theta = dot(EYEDIR, normalize(LIGHT0_DIRECTION));

		float rayleigh_phase = (3.0 / (16.0 * PI)) * (1.0 + pow(cos_theta * 0.5 + 0.5, 2.0));
		vec3 betaRTheta = rayleigh_beta * rayleigh_phase;

		float mie_phase = henyey_greenstein(cos_theta, mie_eccentricity);
		vec3 betaMTheta = mie_beta * mie_phase;

		vec3 Lin = pow(sun_energy * ((betaRTheta + betaMTheta) / (rayleigh_beta + mie_beta)) * (1.0 - extinction), vec3(1.5));
		// Hack from https://github.com/mrdoob/three.js/blob/master/examples/jsm/objects/Sky.js
		Lin *= mix(vec3(1.0), pow(sun_energy * ((betaRTheta + betaMTheta) / (rayleigh_beta + mie_beta)) * extinction, vec3(0.5)), clamp(pow(1.0 - zenith_angle, 5.0), 0.0, 1.0));

		// Hack in the ground color.
		Lin  *= mix(ground_color.rgb, vec3(1.0), smoothstep(-0.1, 0.1, dot(UP, EYEDIR)));

		// Solar disk and out-scattering.
		float sunAngularDiameterCos = cos(LIGHT0_SIZE * sun_disk_scale);
		float sunAngularDiameterCos2 = cos(LIGHT0_SIZE * sun_disk_scale * 0.5);
		float sundisk = smoothstep(sunAngularDiameterCos, sunAngularDiameterCos2, cos_theta);
		vec3 L0 = (sun_energy * extinction) * sundisk * LIGHT0_COLOR;
		%s

		vec3 color = Lin + L0;
		COLOR = pow(color, vec3(1.0 / (1.2 + (1.2 * sun_fade))));
		COLOR *= exposure;
	}
	else {
		// There is no sun, so display night_sky and nothing else.
		%s
		COLOR *= exposure;
	}
}
)",
				p_use_debanding ? "render_mode use_debanding;" : "",
				p_use_night_sky ? "L0 += texture(night_sky, SKY_COORDS).xyz * extinction;" : "",
				p_use_night_sky ? "COLOR = texture(night_sky, SKY_COORDS).xyz;" : ""));
	}
}

PhysicalSkyMaterial::PhysicalSkyMaterial()
{
	_set_material(RS::get_singleton()->material_create());
	set_rayleigh_coefficient(2.0);
	set_rayleigh_color(Color(0.3, 0.405, 0.6));
	set_mie_coefficient(0.005);
	set_mie_eccentricity(0.8);
	set_mie_color(Color(0.69, 0.729, 0.812));
	set_turbidity(10.0);
	set_sun_disk_scale(1.0);
	set_ground_color(Color(0.1, 0.07, 0.034));
	set_energy_multiplier(1.0);
	set_use_debanding(true);
}

PhysicalSkyMaterial::~PhysicalSkyMaterial() {}


