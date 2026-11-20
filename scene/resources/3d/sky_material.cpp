/**************************************************************************/
/*  sky_material.cpp                                                      */
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

#include "sky_material.h"

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/version.h"
#include "servers/rendering/renderer.h"

Mutex SkyMaterial::shader_mutex;
RID SkyMaterial::procedural_shader_cache[4];
RID SkyMaterial::panorama_shader_cache[2];
RID SkyMaterial::physical_shader_cache[4];
RID SkyMaterial::fog_shader;

void SkyMaterial::set_mode(Mode p_mode)
{
	if (mode == p_mode) {
		return;
	}
	mode = p_mode;
	if (shader_set) {
		shader_set = false;
		Renderer::material_set_shader(_get_material(), get_shader_rid());
		shader_set = true;
	}
}

// Procedural sky

void SkyMaterial::set_sky_top_color(const Color& p_sky_top) { sky_top_color = p_sky_top; }

Color SkyMaterial::get_sky_top_color() const { return sky_top_color; }

void SkyMaterial::set_sky_horizon_color(const Color& p_sky_horizon)
{
	sky_horizon_color = p_sky_horizon;
}

Color SkyMaterial::get_sky_horizon_color() const { return sky_horizon_color; }

void SkyMaterial::set_sky_curve(float p_curve) { sky_curve = p_curve; }

float SkyMaterial::get_sky_curve() const { return sky_curve; }

void SkyMaterial::set_sky_energy_multiplier(float p_multiplier)
{
	sky_energy_multiplier = p_multiplier;
}

float SkyMaterial::get_sky_energy_multiplier() const { return sky_energy_multiplier; }

void SkyMaterial::set_sky_cover(const Ref<Texture2D>& p_sky_cover)
{
	sky_cover = p_sky_cover;
	if (mode == MODE_PROCEDURAL) {
		_update_procedural_shader(use_debanding, sky_cover.is_valid());
		if (shader_set) {
			Renderer::material_set_shader(_get_material(), _get_procedural_shader_cache());
		}
	}
}

Ref<Texture2D> SkyMaterial::get_sky_cover() const { return sky_cover; }

void SkyMaterial::set_sky_cover_modulate(const Color& p_sky_cover_modulate)
{
	sky_cover_modulate = p_sky_cover_modulate;
}

Color SkyMaterial::get_sky_cover_modulate() const { return sky_cover_modulate; }

void SkyMaterial::set_ground_bottom_color(const Color& p_ground_bottom)
{
	ground_bottom_color = p_ground_bottom;
}

Color SkyMaterial::get_ground_bottom_color() const { return ground_bottom_color; }

void SkyMaterial::set_ground_horizon_color(const Color& p_ground_horizon)
{
	ground_horizon_color = p_ground_horizon;
}

Color SkyMaterial::get_ground_horizon_color() const { return ground_horizon_color; }

void SkyMaterial::set_ground_curve(float p_curve) { ground_curve = p_curve; }

float SkyMaterial::get_ground_curve() const { return ground_curve; }

void SkyMaterial::set_ground_energy_multiplier(float p_energy)
{
	ground_energy_multiplier = p_energy;
}

float SkyMaterial::get_ground_energy_multiplier() const { return ground_energy_multiplier; }

void SkyMaterial::set_sun_angle_max(float p_angle) { sun_angle_max = p_angle; }

float SkyMaterial::get_sun_angle_max() const { return sun_angle_max; }

void SkyMaterial::set_sun_curve(float p_curve) { sun_curve = p_curve; }

float SkyMaterial::get_sun_curve() const { return sun_curve; }

// Shared sky

void SkyMaterial::set_use_debanding(bool p_use_debanding)
{
	use_debanding = p_use_debanding;
	if (mode == MODE_PROCEDURAL) {
		_update_procedural_shader(use_debanding, sky_cover.is_valid());
		if (shader_set) {
			Renderer::material_set_shader(_get_material(), _get_procedural_shader_cache());
		}
	}
	else if (mode == MODE_PHYSICAL) {
		_update_physical_shader(use_debanding, night_sky.is_valid());
		if (shader_set) {
			Renderer::material_set_shader(_get_material(), _get_physical_shader_cache());
		}
	}
}

bool SkyMaterial::get_use_debanding() const { return use_debanding; }

void SkyMaterial::set_energy_multiplier(float p_multiplier) { energy_multiplier = p_multiplier; }

float SkyMaterial::get_energy_multiplier() const { return energy_multiplier; }

void SkyMaterial::set_exposure_value(float p_exposure)
{
	set_energy_multiplier(Math::pow(2.0f, p_exposure));
}

float SkyMaterial::get_exposure_value() const
{
	return Math::log(get_energy_multiplier()) / Math::log(2.0f);
}

// Panorama sky

void SkyMaterial::set_panorama(const Ref<Texture2D>& p_panorama) { panorama = p_panorama; }

Ref<Texture2D> SkyMaterial::get_panorama() const { return panorama; }

void SkyMaterial::set_filtering_enabled(bool p_enabled)
{
	filter = p_enabled;
	if (mode == MODE_PANORAMA) {
		_update_panorama_shader(filter);
		if (shader_set) {
			Renderer::material_set_shader(_get_material(), panorama_shader_cache[int(filter)]);
		}
	}
}

bool SkyMaterial::is_filtering_enabled() const { return filter; }

// Physical sky

void SkyMaterial::set_rayleigh_coefficient(float p_rayleigh) { rayleigh = p_rayleigh; }

float SkyMaterial::get_rayleigh_coefficient() const { return rayleigh; }

void SkyMaterial::set_rayleigh_color(Color p_rayleigh_color) { rayleigh_color = p_rayleigh_color; }

Color SkyMaterial::get_rayleigh_color() const { return rayleigh_color; }

void SkyMaterial::set_turbidity(float p_turbidity) { turbidity = p_turbidity; }

float SkyMaterial::get_turbidity() const { return turbidity; }

void SkyMaterial::set_mie_coefficient(float p_mie) { mie = p_mie; }

float SkyMaterial::get_mie_coefficient() const { return mie; }

void SkyMaterial::set_mie_eccentricity(float p_eccentricity) { mie_eccentricity = p_eccentricity; }

float SkyMaterial::get_mie_eccentricity() const { return mie_eccentricity; }

void SkyMaterial::set_mie_color(Color p_mie_color) { mie_color = p_mie_color; }

Color SkyMaterial::get_mie_color() const { return mie_color; }

void SkyMaterial::set_sun_disk_scale(float p_sun_disk_scale) { sun_disk_scale = p_sun_disk_scale; }

float SkyMaterial::get_sun_disk_scale() const { return sun_disk_scale; }

void SkyMaterial::set_ground_color(Color p_ground_color) { ground_color = p_ground_color; }

Color SkyMaterial::get_ground_color() const { return ground_color; }

void SkyMaterial::set_night_sky(const Ref<Texture2D>& p_night_sky)
{
	night_sky = p_night_sky;
	if (mode == MODE_PHYSICAL) {
		_update_physical_shader(use_debanding, night_sky.is_valid());
		if (shader_set) {
			Renderer::material_set_shader(_get_material(), _get_physical_shader_cache());
		}
	}
}

Ref<Texture2D> SkyMaterial::get_night_sky() const { return night_sky; }

// Fog

void SkyMaterial::set_density(float p_density) { density = p_density; }

float SkyMaterial::get_density() const { return density; }

void SkyMaterial::set_albedo(Color p_color) { albedo = p_color; }

Color SkyMaterial::get_albedo() const { return albedo; }

void SkyMaterial::set_emission(Color p_color) { emission = p_color; }

Color SkyMaterial::get_emission() const { return emission; }

void SkyMaterial::set_height_falloff(float p_falloff) { height_falloff = p_falloff; }

float SkyMaterial::get_height_falloff() const { return height_falloff; }

void SkyMaterial::set_edge_fade(float p_edge_fade) { edge_fade = p_edge_fade; }

float SkyMaterial::get_edge_fade() const { return edge_fade; }

void SkyMaterial::set_density_texture(const Ref<Texture3D>& p_texture)
{
	density_texture = p_texture;
}

Ref<Texture3D> SkyMaterial::get_density_texture() const { return density_texture; }

// Shader management

Shader::Mode SkyMaterial::get_shader_mode() const
{
	return mode == MODE_FOG ? Shader::MODE_FOG : Shader::MODE_SKY;
}

RID SkyMaterial::_get_procedural_shader_cache() const
{
	return procedural_shader_cache[int(use_debanding) + (sky_cover.is_valid() ? 2 : 0)];
}

RID SkyMaterial::_get_physical_shader_cache() const
{
	return physical_shader_cache[int(use_debanding) + (night_sky.is_valid() ? 2 : 0)];
}

RID SkyMaterial::get_shader_rid() const
{
	switch (mode) {
	case MODE_PROCEDURAL:
		_update_procedural_shader(use_debanding, sky_cover.is_valid());
		return _get_procedural_shader_cache();
	case MODE_PANORAMA:
		_update_panorama_shader(filter);
		return panorama_shader_cache[int(filter)];
	case MODE_PHYSICAL:
		_update_physical_shader(use_debanding, night_sky.is_valid());
		return _get_physical_shader_cache();
	case MODE_FOG:
		_update_fog_shader();
		return fog_shader;
	}
	return RID();
}

RID SkyMaterial::get_rid() const
{
	RID shader_rid = get_shader_rid();
	if (!shader_set) {
		Renderer::material_set_shader(_get_material(), shader_rid);
		shader_set = true;
	}
	return _get_material();
}

void SkyMaterial::cleanup_shader()
{
	for (int i = 0; i < 4; i++) {
		if (procedural_shader_cache[i].is_valid()) {
			Renderer::free_rid(procedural_shader_cache[i]);
			procedural_shader_cache[i] = RID();
		}
	}
	for (int i = 0; i < 2; i++) {
		if (panorama_shader_cache[i].is_valid()) {
			Renderer::free_rid(panorama_shader_cache[i]);
			panorama_shader_cache[i] = RID();
		}
	}
	for (int i = 0; i < 4; i++) {
		if (physical_shader_cache[i].is_valid()) {
			Renderer::free_rid(physical_shader_cache[i]);
			physical_shader_cache[i] = RID();
		}
	}
	if (fog_shader.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::free_rid(fog_shader);
		fog_shader = RID();
	}
}

void SkyMaterial::_update_procedural_shader(bool p_use_debanding, bool p_use_sky_cover)
{
	MutexLock shader_lock(shader_mutex);
	int index = int(p_use_debanding) + int(p_use_sky_cover) * 2;
	if (procedural_shader_cache[index].is_null()) {
		procedural_shader_cache[index] = Renderer::shader_create();

		// Add a comment to describe the shader origin (useful when converting to ShaderMaterial).
		Renderer::shader_set_code(procedural_shader_cache[index],
			vformat(R"(
// NOTE: Shader automatically converted from )" VLTR_VERSION_NAME " " VLTR_VERSION_FULL_CONFIG
					R"('s SkyMaterial (Procedural).

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

void SkyMaterial::_update_panorama_shader(bool p_filter)
{
	MutexLock shader_lock(shader_mutex);
	int index = int(p_filter);
	if (panorama_shader_cache[index].is_null()) {
		panorama_shader_cache[index] = Renderer::shader_create();

		// Add a comment to describe the shader origin (useful when converting to ShaderMaterial).
		Renderer::shader_set_code(
			panorama_shader_cache[index], vformat(R"(
// NOTE: Shader automatically converted from )" VLTR_VERSION_NAME " " VLTR_VERSION_FULL_CONFIG
												  R"('s SkyMaterial (Panorama).

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

void SkyMaterial::_update_physical_shader(bool p_use_debanding, bool p_use_night_sky)
{
	MutexLock shader_lock(shader_mutex);
	int index = int(p_use_debanding) + int(p_use_night_sky) * 2;
	if (physical_shader_cache[index].is_null()) {
		physical_shader_cache[index] = Renderer::shader_create();

		// Add a comment to describe the shader origin (useful when converting to ShaderMaterial).
		Renderer::shader_set_code(physical_shader_cache[index],
			vformat(R"(
// NOTE: Shader automatically converted from )" VLTR_VERSION_NAME " " VLTR_VERSION_FULL_CONFIG
					R"('s SkyMaterial (Physical).

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

void SkyMaterial::_update_fog_shader()
{
	MutexLock shader_lock(shader_mutex);
	if (fog_shader.is_null()) {
		fog_shader = Renderer::shader_create();

		// Add a comment to describe the shader origin (useful when converting to ShaderMaterial).
		Renderer::shader_set_code(fog_shader, R"(
// NOTE: Shader automatically converted from )" VLTR_VERSION_NAME " " VLTR_VERSION_FULL_CONFIG
											  R"('s SkyMaterial (Fog).

shader_type fog;

uniform float density : hint_range(0, 1, 0.0001) = 1.0;
uniform vec4 albedo : source_color = vec4(1.0);
uniform vec4 emission : source_color = vec4(0, 0, 0, 1);
uniform float height_falloff = 0.0;
uniform float edge_fade = 0.1;
uniform sampler3D density_texture: hint_default_white;


void fog() {
    DENSITY = density * clamp(exp2(-height_falloff * (WORLD_POSITION.y - OBJECT_POSITION.y)), 0.0, 1.0);
    DENSITY *= texture(density_texture, UVW).r;
    DENSITY *= pow(clamp(-2.0 * SDF / min(min(SIZE.x, SIZE.y), SIZE.z), 0.0, 1.0), edge_fade);
    ALBEDO = albedo.rgb;
    EMISSION = emission.rgb;
}
)");
	}
}

SkyMaterial::SkyMaterial(Mode p_mode) : mode(p_mode)
{
	_set_material(Renderer::material_create());

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

	set_rayleigh_coefficient(2.0);
	set_rayleigh_color(Color(0.3, 0.405, 0.6));
	set_mie_coefficient(0.005);
	set_mie_eccentricity(0.8);
	set_mie_color(Color(0.69, 0.729, 0.812));
	set_turbidity(10.0);
	set_sun_disk_scale(1.0);
	set_ground_color(Color(0.1, 0.07, 0.034));

	set_density(1.0);
	set_albedo(Color(1, 1, 1, 1));
	set_emission(Color(0, 0, 0, 1));
	set_height_falloff(0.0);
	set_edge_fade(0.1);

	set_energy_multiplier(1.0);
	if (mode == MODE_PROCEDURAL || mode == MODE_PHYSICAL) {
		set_use_debanding(true);
	}
}

SkyMaterial::~SkyMaterial()
{
	if (mode == MODE_FOG) {
		Renderer::material_set_shader(_get_material(), RID());
	}
}
