/**************************************************************************/
/*  light_3d.cpp                                                          */
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
#include "light_3d.h"
#include "scene/main/scene_tree.h"
#include "servers/rendering/rendering_server.h"

real_t Light3D::get_param(Param p_param) const
{
	ERR_FAIL_INDEX_V(p_param, PARAM_MAX, 0);
	return param[p_param];
}

bool Light3D::has_shadow() const { return shadow; }

void Light3D::set_negative(bool p_enable)
{
	negative = p_enable;
	RS::light_set_negative(light, p_enable);
}

bool Light3D::is_negative() const { return negative; }

bool Light3D::is_distance_fade_enabled() const { return distance_fade_enabled; }

void Light3D::set_distance_fade_begin(real_t p_distance)
{
	distance_fade_begin = p_distance;
	RS::light_set_distance_fade(light, distance_fade_enabled, distance_fade_begin,
		distance_fade_shadow, distance_fade_length);
}

real_t Light3D::get_distance_fade_begin() const { return distance_fade_begin; }

void Light3D::set_distance_fade_shadow(real_t p_distance)
{
	distance_fade_shadow = p_distance;
	RS::light_set_distance_fade(light, distance_fade_enabled, distance_fade_begin,
		distance_fade_shadow, distance_fade_length);
}

real_t Light3D::get_distance_fade_shadow() const { return distance_fade_shadow; }

void Light3D::set_distance_fade_length(real_t p_length)
{
	distance_fade_length = p_length;
	RS::light_set_distance_fade(light, distance_fade_enabled, distance_fade_begin,
		distance_fade_shadow, distance_fade_length);
}

real_t Light3D::get_distance_fade_length() const { return distance_fade_length; }

void Light3D::set_cull_mask(uint32_t p_cull_mask)
{
	cull_mask = p_cull_mask;
	RS::light_set_cull_mask(light, p_cull_mask);
}

uint32_t Light3D::get_cull_mask() const { return cull_mask; }

Color Light3D::get_color() const { return color; }

void Light3D::set_shadow_reverse_cull_face(bool p_enable)
{
	reverse_cull = p_enable;
	RS::light_set_reverse_cull_face_mode(light, reverse_cull);
}

bool Light3D::get_shadow_reverse_cull_face() const { return reverse_cull; }

void Light3D::set_shadow_caster_mask(uint32_t p_caster_mask)
{
	shadow_caster_mask = p_caster_mask;
	RS::light_set_shadow_caster_mask(light, shadow_caster_mask);
}

uint32_t Light3D::get_shadow_caster_mask() const { return shadow_caster_mask; }

AABB Light3D::get_aabb() const { return AABB(); }

PackedStringArray Light3D::get_configuration_warnings() const
{
	PackedStringArray warnings = VisualInstance3D::get_configuration_warnings();

	if (!get_scale().is_equal_approx(Vector3(1, 1, 1))) {
		warnings.push_back(RTR("A light's scale does not affect the visual size of the light."));
	}

	return warnings;
}

void Light3D::set_bake_mode(BakeMode p_mode)
{
	bake_mode = p_mode;
	RS::light_set_bake_mode(light, RSE::LightBakeMode(p_mode));
}

Light3D::BakeMode Light3D::get_bake_mode() const { return bake_mode; }

Ref<Texture2D> Light3D::get_projector() const { return projector; }

void Light3D::owner_changed_notify()
{
	// For cases where owner changes _after_ entering tree (as example, editor editing).
	_update_visibility();
}

// Temperature expressed in Kelvins. Valid range 1000 - 15000
// First converts to CIE 1960 then to sRGB
// As explained in the Filament documentation:
// https://google.github.io/filament/Filament.md.html#lighting/directlighting/lightsparameterization
Color _color_from_temperature(float p_temperature)
{
	float T2 = p_temperature * p_temperature;
	float u = (0.860117757f + 1.54118254e-4f * p_temperature + 1.28641212e-7f * T2) /
			  (1.0f + 8.42420235e-4f * p_temperature + 7.08145163e-7f * T2);
	float v = (0.317398726f + 4.22806245e-5f * p_temperature + 4.20481691e-8f * T2) /
			  (1.0f - 2.89741816e-5f * p_temperature + 1.61456053e-7f * T2);

	// Convert to xyY space.
	float d = 1.0f / (2.0f * u - 8.0f * v + 4.0f);
	float x = 3.0f * u * d;
	float y = 2.0f * v * d;

	// Convert to XYZ space
	const float a = 1.0 / MAX(y, 1e-5f);
	Vector3 xyz = Vector3(x * a, 1.0, (1.0f - x - y) * a);

	// Convert from XYZ to sRGB(linear)
	Vector3 linear = Vector3(3.2404542f * xyz.x - 1.5371385f * xyz.y - 0.4985314f * xyz.z,
		-0.9692660f * xyz.x + 1.8760108f * xyz.y + 0.0415560f * xyz.z,
		0.0556434f * xyz.x - 0.2040259f * xyz.y + 1.0572252f * xyz.z);
	linear /= MAX(1e-5f, linear[linear.max_axis_index()]);
	// Normalize, clamp, and convert to sRGB.
	return Color(linear.x, linear.y, linear.z).clamp().linear_to_srgb();
}

Color Light3D::get_correlated_color() const { return correlated_color; }

float Light3D::get_temperature() const { return temperature; }

void Light3D::_update_visibility()
{
	if (!is_inside_tree()) {
		return;
	}

	bool editor_ok = true;

#ifdef TOOLS_ENABLED
	if (editor_only) {
		if (!Engine::get_singleton()->is_editor_hint()) {
			editor_ok = false;
		}
		else {
			editor_ok = (get_tree()->get_edited_scene_root() &&
						 (this == get_tree()->get_edited_scene_root() ||
							 get_owner() == get_tree()->get_edited_scene_root()));
		}
	}
#else
	if (editor_only) {
		editor_ok = false;
	}
#endif

	RS::instance_set_visible(get_instance(), is_visible_in_tree() && editor_ok);
}

void Light3D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_VISIBILITY_CHANGED:
	case NOTIFICATION_ENTER_TREE: {
		_update_visibility();
	} break;
	}
}

void Light3D::set_editor_only(bool p_editor_only)
{
	editor_only = p_editor_only;
	_update_visibility();
}

bool Light3D::is_editor_only() const { return editor_only; }

Light3D::Light3D()
{
	ERR_PRINT("Light3D should not be instantiated directly; use the DirectionalLight3D, "
			  "OmniLight3D or SpotLight3D subtypes instead.");
}

Light3D::~Light3D()
{
	ERR_FAIL_NULL(RenderingServer::data);
	RS::instance_set_base(get_instance(), RID());

	if (light.is_valid()) {
		RenderingServer::free_rid(light);
	}
}

DirectionalLight3D::ShadowMode DirectionalLight3D::get_shadow_mode() const { return shadow_mode; }

void DirectionalLight3D::set_blend_splits(bool p_enable)
{
	blend_splits = p_enable;
	RS::light_directional_set_blend_splits(light, p_enable);
}

bool DirectionalLight3D::is_blend_splits_enabled() const { return blend_splits; }

void DirectionalLight3D::set_sky_mode(SkyMode p_mode)
{
	sky_mode = p_mode;
	RS::light_directional_set_sky_mode(
		light, RSE::LightDirectionalSkyMode(p_mode));
}

DirectionalLight3D::SkyMode DirectionalLight3D::get_sky_mode() const { return sky_mode; }

void OmniLight3D::set_shadow_mode(ShadowMode p_mode)
{
	shadow_mode = p_mode;
	RS::light_omni_set_shadow_mode(light, RSE::LightOmniShadowMode(p_mode));
}

OmniLight3D::ShadowMode OmniLight3D::get_shadow_mode() const { return shadow_mode; }

PackedStringArray OmniLight3D::get_configuration_warnings() const
{
	PackedStringArray warnings = Light3D::get_configuration_warnings();

	if (!has_shadow() && get_projector().is_valid()) {
		warnings.push_back(RTR("Projector texture only works with shadows active."));
	}

	if (get_projector().is_valid() &&
		(OS::get_singleton()->get_current_rendering_method() == "gl_compatibility" ||
			OS::get_singleton()->get_current_rendering_method() == "dummy")) {
		warnings.push_back(RTR("Projector textures are not supported when using the Compatibility "
							   "renderer yet. Support will be added in a future release."));
	}

	return warnings;
}



OmniLight3D::OmniLight3D() : Light3D(RSE::LIGHT_OMNI) { set_shadow_mode(SHADOW_CUBE); }

PackedStringArray SpotLight3D::get_configuration_warnings() const
{
	PackedStringArray warnings = Light3D::get_configuration_warnings();

	if (has_shadow() && get_param(PARAM_SPOT_ANGLE) >= 90.0) {
		warnings.push_back(
			RTR("A SpotLight3D with an angle wider than 90 degrees cannot cast shadows."));
	}

	if (!has_shadow() && get_projector().is_valid()) {
		warnings.push_back(RTR("Projector texture only works with shadows active."));
	}

	if (get_projector().is_valid() &&
		(OS::get_singleton()->get_current_rendering_method() == "gl_compatibility" ||
			OS::get_singleton()->get_current_rendering_method() == "dummy")) {
		warnings.push_back(RTR("Projector textures are not supported when using the Compatibility "
							   "renderer yet. Support will be added in a future release."));
	}

	return warnings;
}

Ref<Texture2D> AreaLight3D::get_area_texture() const { return area_texture; }

Vector2 AreaLight3D::get_area_size() const { return area_size; }

void AreaLight3D::set_area_normalize_energy(bool p_enabled)
{
	area_normalize_energy = p_enabled;
	RS::light_area_set_normalize_energy(light, p_enabled);
}

bool AreaLight3D::is_area_normalizing_energy() const { return area_normalize_energy; }

PackedStringArray AreaLight3D::get_configuration_warnings() const
{
	PackedStringArray warnings = Light3D::get_configuration_warnings();

	if (get_projector().is_valid()) {
		warnings.push_back(RTR("Projector texture is not supported for area lights. Use the "
							   "area_texture field instead."));
	}
	if (get_area_texture().is_valid() &&
		OS::get_singleton()->get_current_rendering_method() == "gl_compatibility") {
		warnings.push_back(RTR("Rendering textured area lights is not implemented in the "
							   "Compatibility rendering mode."));
	}

	if (has_shadow() && OS::get_singleton()->get_current_rendering_method() == "gl_compatibility") {
		warnings.push_back(
			RTR("Rendering area light shadows does not work in the Compatibility rendering mode."));
	}

	return warnings;
}

AreaLight3D::~AreaLight3D()
{
	// has to run, because light RID needs to be freed before area_texture RID.
	// Since area_texture is a member of AreaLight3D, it would be destructed before the
	// deconstructor of Light3D would be called, leading to errors.
	ERR_FAIL_NULL(RenderingServer::data);
	if (light.is_valid()) {
		RenderingServer::free_rid(light);
	}
}


