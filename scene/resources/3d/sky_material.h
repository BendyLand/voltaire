/**************************************************************************/
/*  sky_material.h                                                        */
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

#include "core/templates/rid.h"
#include "scene/resources/material.h"
#include "scene/resources/texture.h"

class SkyMaterial final : public Material
{
public:
	enum Mode
	{
		MODE_PROCEDURAL,
		MODE_PANORAMA,
		MODE_PHYSICAL,
		MODE_FOG,
	};

private:
	Mode mode = MODE_PROCEDURAL;

	// Procedural sky properties
	Color sky_top_color = Color(0.385, 0.454, 0.55);
	Color sky_horizon_color = Color(0.6463, 0.6558, 0.6708);
	float sky_curve = 0.15f;
	float sky_energy_multiplier = 1.0f;
	Ref<Texture2D> sky_cover;
	Color sky_cover_modulate = Color(1, 1, 1);

	Color ground_bottom_color = Color(0.2, 0.169, 0.133);
	Color ground_horizon_color = Color(0.6463, 0.6558, 0.6708);
	float ground_curve = 0.02f;
	float ground_energy_multiplier = 1.0f;

	float sun_angle_max = 30.0f;
	float sun_curve = 0.15f;

	// Shared sky properties
	bool use_debanding = true;
	float energy_multiplier = 1.0f;

	// Panorama sky properties
	Ref<Texture2D> panorama;
	bool filter = true;

	// Physical sky properties
	float rayleigh = 2.0f;
	Color rayleigh_color = Color(0.3, 0.405, 0.6);
	float mie = 0.005f;
	float mie_eccentricity = 0.8f;
	Color mie_color = Color(0.69, 0.729, 0.812);
	float turbidity = 10.0f;
	float sun_disk_scale = 1.0f;
	Color ground_color = Color(0.1, 0.07, 0.034);
	Ref<Texture2D> night_sky;

	// Fog properties
	float density = 1.0f;
	Color albedo = Color(1, 1, 1, 1);
	Color emission = Color(0, 0, 0, 1);
	float height_falloff = 0.0f;
	float edge_fade = 0.1f;
	Ref<Texture3D> density_texture;

	static Mutex shader_mutex;
	static RID procedural_shader_cache[4];
	static RID panorama_shader_cache[2];
	static RID physical_shader_cache[4];
	static RID fog_shader;

	static void _update_procedural_shader(bool p_use_debanding, bool p_use_sky_cover);
	static void _update_panorama_shader(bool p_filter);
	static void _update_physical_shader(bool p_use_debanding, bool p_use_night_sky);
	static void _update_fog_shader();

	RID _get_procedural_shader_cache() const;
	RID _get_physical_shader_cache() const;

	mutable bool shader_set = false;

public:
	void set_mode(Mode p_mode);
	Mode get_mode() const { return mode; }

	// Procedural sky
	void set_sky_top_color(const Color& p_sky_top);
	Color get_sky_top_color() const;

	void set_sky_horizon_color(const Color& p_sky_horizon);
	Color get_sky_horizon_color() const;

	void set_sky_curve(float p_curve);
	float get_sky_curve() const;

	void set_sky_energy_multiplier(float p_multiplier);
	float get_sky_energy_multiplier() const;

	void set_sky_cover(const Ref<Texture2D>& p_sky_cover);
	Ref<Texture2D> get_sky_cover() const;

	void set_sky_cover_modulate(const Color& p_sky_cover_modulate);
	Color get_sky_cover_modulate() const;

	void set_ground_bottom_color(const Color& p_ground_bottom);
	Color get_ground_bottom_color() const;

	void set_ground_horizon_color(const Color& p_ground_horizon);
	Color get_ground_horizon_color() const;

	void set_ground_curve(float p_curve);
	float get_ground_curve() const;

	void set_ground_energy_multiplier(float p_energy);
	float get_ground_energy_multiplier() const;

	void set_sun_angle_max(float p_angle);
	float get_sun_angle_max() const;

	void set_sun_curve(float p_curve);
	float get_sun_curve() const;

	// Shared sky
	void set_use_debanding(bool p_use_debanding);
	bool get_use_debanding() const;

	void set_energy_multiplier(float p_multiplier);
	float get_energy_multiplier() const;

	void set_exposure_value(float p_exposure);
	float get_exposure_value() const;

	// Panorama sky
	void set_panorama(const Ref<Texture2D>& p_panorama);
	Ref<Texture2D> get_panorama() const;

	void set_filtering_enabled(bool p_enabled);
	bool is_filtering_enabled() const;

	// Physical sky
	void set_rayleigh_coefficient(float p_rayleigh);
	float get_rayleigh_coefficient() const;

	void set_rayleigh_color(Color p_rayleigh_color);
	Color get_rayleigh_color() const;

	void set_turbidity(float p_turbidity);
	float get_turbidity() const;

	void set_mie_coefficient(float p_mie);
	float get_mie_coefficient() const;

	void set_mie_eccentricity(float p_eccentricity);
	float get_mie_eccentricity() const;

	void set_mie_color(Color p_mie_color);
	Color get_mie_color() const;

	void set_sun_disk_scale(float p_sun_disk_scale);
	float get_sun_disk_scale() const;

	void set_ground_color(Color p_ground_color);
	Color get_ground_color() const;

	void set_night_sky(const Ref<Texture2D>& p_night_sky);
	Ref<Texture2D> get_night_sky() const;

	// Fog
	void set_density(float p_density);
	float get_density() const;

	void set_albedo(Color p_color);
	Color get_albedo() const;

	void set_emission(Color p_color);
	Color get_emission() const;

	void set_height_falloff(float p_falloff);
	float get_height_falloff() const;

	void set_edge_fade(float p_edge_fade);
	float get_edge_fade() const;

	void set_density_texture(const Ref<Texture3D>& p_texture);
	Ref<Texture3D> get_density_texture() const;

	virtual Shader::Mode get_shader_mode() const override;
	virtual RID get_shader_rid() const override;
	virtual RID get_rid() const override;

	static void cleanup_shader();

	SkyMaterial(Mode p_mode = MODE_PROCEDURAL);
	virtual ~SkyMaterial();
};
