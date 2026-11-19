/**************************************************************************/
/*  light_3d.h                                                            */
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

#include "scene/3d/visual_instance_3d.h"

class Light3D : public Node3D
{
public:
	enum Param
	{
		PARAM_ENERGY = RSE::LIGHT_PARAM_ENERGY,
		PARAM_INDIRECT_ENERGY = RSE::LIGHT_PARAM_INDIRECT_ENERGY,
		PARAM_VOLUMETRIC_FOG_ENERGY = RSE::LIGHT_PARAM_VOLUMETRIC_FOG_ENERGY,
		PARAM_SPECULAR = RSE::LIGHT_PARAM_SPECULAR,
		PARAM_RANGE = RSE::LIGHT_PARAM_RANGE,
		PARAM_SIZE = RSE::LIGHT_PARAM_SIZE,
		PARAM_ATTENUATION = RSE::LIGHT_PARAM_ATTENUATION,
		PARAM_SPOT_ANGLE = RSE::LIGHT_PARAM_SPOT_ANGLE,
		PARAM_SPOT_ATTENUATION = RSE::LIGHT_PARAM_SPOT_ATTENUATION,
		PARAM_SHADOW_MAX_DISTANCE = RSE::LIGHT_PARAM_SHADOW_MAX_DISTANCE,
		PARAM_SHADOW_SPLIT_1_OFFSET = RSE::LIGHT_PARAM_SHADOW_SPLIT_1_OFFSET,
		PARAM_SHADOW_SPLIT_2_OFFSET = RSE::LIGHT_PARAM_SHADOW_SPLIT_2_OFFSET,
		PARAM_SHADOW_SPLIT_3_OFFSET = RSE::LIGHT_PARAM_SHADOW_SPLIT_3_OFFSET,
		PARAM_SHADOW_FADE_START = RSE::LIGHT_PARAM_SHADOW_FADE_START,
		PARAM_SHADOW_NORMAL_BIAS = RSE::LIGHT_PARAM_SHADOW_NORMAL_BIAS,
		PARAM_SHADOW_BIAS = RSE::LIGHT_PARAM_SHADOW_BIAS,
		PARAM_SHADOW_PANCAKE_SIZE = RSE::LIGHT_PARAM_SHADOW_PANCAKE_SIZE,
		PARAM_SHADOW_OPACITY = RSE::LIGHT_PARAM_SHADOW_OPACITY,
		PARAM_SHADOW_BLUR = RSE::LIGHT_PARAM_SHADOW_BLUR,
		PARAM_TRANSMITTANCE_BIAS = RSE::LIGHT_PARAM_TRANSMITTANCE_BIAS,
		PARAM_INTENSITY = RSE::LIGHT_PARAM_INTENSITY,
		PARAM_MAX = RSE::LIGHT_PARAM_MAX
	};

	enum BakeMode
	{
		BAKE_DISABLED,
		BAKE_STATIC,
		BAKE_DYNAMIC,
	};

	enum DirectionalShadowMode
	{
		DIRECTIONAL_SHADOW_ORTHOGONAL,
		DIRECTIONAL_SHADOW_PARALLEL_2_SPLITS,
		DIRECTIONAL_SHADOW_PARALLEL_4_SPLITS,
	};

	enum SkyMode
	{
		SKY_MODE_LIGHT_AND_SKY,
		SKY_MODE_LIGHT_ONLY,
		SKY_MODE_SKY_ONLY,
	};

	enum OmniShadowMode
	{
		OMNI_SHADOW_DUAL_PARABOLOID,
		OMNI_SHADOW_CUBE,
	};

	RenderInstance3D render_instance;

private:
	RSE::LightType type = RSE::LIGHT_DIRECTIONAL;
	RID light;

	Color color = Color(1, 1, 1);
	real_t param[PARAM_MAX] = {};
	bool shadow = false;
	bool negative = false;
	bool reverse_cull = false;
	uint32_t cull_mask = 0xFFFFFFFF;
	uint32_t shadow_caster_mask = 0xFFFFFFFF;
	bool distance_fade_enabled = false;
	real_t distance_fade_begin = 40.0;
	real_t distance_fade_shadow = 50.0;
	real_t distance_fade_length = 10.0;
	bool editor_only = false;
	BakeMode bake_mode = BAKE_DYNAMIC;
	Ref<Texture2D> projector;
	Color correlated_color = Color(1.0, 1.0, 1.0);
	float temperature = 6500.0;

	// Directional-specific state
	bool blend_splits = false;
	DirectionalShadowMode directional_shadow_mode = DIRECTIONAL_SHADOW_PARALLEL_4_SPLITS;
	SkyMode sky_mode = SKY_MODE_LIGHT_AND_SKY;

	// Omni-specific state
	OmniShadowMode omni_shadow_mode = OMNI_SHADOW_CUBE;

	// Area-specific state
	Vector2 area_size = Vector2(1.0, 1.0);
	Ref<Texture2D> area_texture;
	bool area_normalize_energy = true;

	void _create_light_rid();
	void _free_light_rid();
	void _update_visibility();

	virtual void owner_changed_notify() override;

protected:
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	void set_light_type(RSE::LightType p_type);
	RSE::LightType get_light_type() const { return type; }

	void set_editor_only(bool p_editor_only);
	bool is_editor_only() const;

	void set_param(Param p_param, real_t p_value);
	real_t get_param(Param p_param) const;

	void set_shadow(bool p_enable);
	bool has_shadow() const;

	void set_negative(bool p_enable);
	bool is_negative() const;

	void set_enable_distance_fade(bool p_enable);
	bool is_distance_fade_enabled() const;

	void set_distance_fade_begin(real_t p_distance);
	real_t get_distance_fade_begin() const;

	void set_distance_fade_shadow(real_t p_distance);
	real_t get_distance_fade_shadow() const;

	void set_distance_fade_length(real_t p_length);
	real_t get_distance_fade_length() const;

	void set_cull_mask(uint32_t p_cull_mask);
	uint32_t get_cull_mask() const;

	void set_color(const Color& p_color);
	Color get_color() const;

	void set_shadow_reverse_cull_face(bool p_enable);
	bool get_shadow_reverse_cull_face() const;

	void set_shadow_caster_mask(uint32_t p_caster_mask);
	uint32_t get_shadow_caster_mask() const;

	void set_bake_mode(BakeMode p_mode);
	BakeMode get_bake_mode() const;

	void set_projector(const Ref<Texture2D>& p_texture);
	Ref<Texture2D> get_projector() const;

	float get_temperature() const;
	Color get_correlated_color() const;

	// Directional light parameters
	void set_directional_shadow_mode(DirectionalShadowMode p_mode);
	DirectionalShadowMode get_directional_shadow_mode() const { return directional_shadow_mode; }

	void set_blend_splits(bool p_enable);
	bool is_blend_splits_enabled() const { return blend_splits; }

	void set_sky_mode(SkyMode p_mode);
	SkyMode get_sky_mode() const { return sky_mode; }

	// Omni light parameters
	void set_omni_shadow_mode(OmniShadowMode p_mode);
	OmniShadowMode get_omni_shadow_mode() const { return omni_shadow_mode; }

	// Area light parameters
	void set_area_size(const Vector2& p_size);
	Vector2 get_area_size() const { return area_size; }

	void set_area_texture(const Ref<Texture2D>& p_texture);
	Ref<Texture2D> get_area_texture() const { return area_texture; }

	void set_area_normalize_energy(bool p_enable);
	bool is_area_normalizing_energy() const { return area_normalize_energy; }

	AABB get_aabb() const;
	virtual PackedStringArray get_configuration_warnings() const override;

	Light3D(RSE::LightType p_type = RSE::LIGHT_DIRECTIONAL);
	~Light3D();
};
