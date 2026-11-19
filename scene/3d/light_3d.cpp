/**************************************************************************/
/*  light_3d.cpp                                                          */
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

#include "light_3d.h"

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/os/os.h"
#include "scene/main/scene_tree.h"
#include "servers/rendering/renderer.h"

void Light3D::_create_light_rid()
{
	if (!Renderer::data) {
		return;
	}
	switch (type) {
	case RSE::LIGHT_DIRECTIONAL:
		light = Renderer::directional_light_create();
		break;
	case RSE::LIGHT_OMNI:
		light = Renderer::omni_light_create();
		Renderer::light_omni_set_shadow_mode(light, RSE::LightOmniShadowMode(omni_shadow_mode));
		break;
	case RSE::LIGHT_SPOT:
		light = Renderer::spot_light_create();
		break;
	case RSE::LIGHT_AREA:
		light = Renderer::area_light_create();
		break;
	}
	set_base(light);
}

void Light3D::_free_light_rid()
{
	if (light.is_valid() && Renderer::data) {
		set_base(RID());
		Renderer::free_rid(light);
		light = RID();
	}
}

void Light3D::set_light_type(RSE::LightType p_type)
{
	if (type == p_type) {
		return;
	}
	_free_light_rid();
	type = p_type;
	_create_light_rid();
}

void Light3D::set_param(Param p_param, real_t p_value)
{
	ERR_FAIL_INDEX(p_param, PARAM_MAX);
	param[p_param] = p_value;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_param(light, RSE::LightParam(p_param), p_value);
	}
}

real_t Light3D::get_param(Param p_param) const
{
	ERR_FAIL_INDEX_V(p_param, PARAM_MAX, 0);
	return param[p_param];
}

void Light3D::set_shadow(bool p_enable)
{
	shadow = p_enable;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_shadow(light, p_enable);
	}
}

bool Light3D::has_shadow() const { return shadow; }

void Light3D::set_negative(bool p_enable)
{
	negative = p_enable;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_negative(light, p_enable);
	}
}

bool Light3D::is_negative() const { return negative; }

void Light3D::set_enable_distance_fade(bool p_enable)
{
	distance_fade_enabled = p_enable;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_distance_fade(light, distance_fade_enabled, distance_fade_begin,
			distance_fade_shadow, distance_fade_length);
	}
}

bool Light3D::is_distance_fade_enabled() const { return distance_fade_enabled; }

void Light3D::set_distance_fade_begin(real_t p_distance)
{
	distance_fade_begin = p_distance;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_distance_fade(light, distance_fade_enabled, distance_fade_begin,
			distance_fade_shadow, distance_fade_length);
	}
}

real_t Light3D::get_distance_fade_begin() const { return distance_fade_begin; }

void Light3D::set_distance_fade_shadow(real_t p_distance)
{
	distance_fade_shadow = p_distance;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_distance_fade(light, distance_fade_enabled, distance_fade_begin,
			distance_fade_shadow, distance_fade_length);
	}
}

real_t Light3D::get_distance_fade_shadow() const { return distance_fade_shadow; }

void Light3D::set_distance_fade_length(real_t p_length)
{
	distance_fade_length = p_length;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_distance_fade(light, distance_fade_enabled, distance_fade_begin,
			distance_fade_shadow, distance_fade_length);
	}
}

real_t Light3D::get_distance_fade_length() const { return distance_fade_length; }

void Light3D::set_cull_mask(uint32_t p_cull_mask)
{
	cull_mask = p_cull_mask;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_cull_mask(light, p_cull_mask);
	}
}

uint32_t Light3D::get_cull_mask() const { return cull_mask; }

void Light3D::set_color(const Color& p_color)
{
	color = p_color;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_color(light, color * correlated_color);
	}
}

Color Light3D::get_color() const { return color; }

void Light3D::set_shadow_reverse_cull_face(bool p_enable)
{
	reverse_cull = p_enable;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_reverse_cull_face_mode(light, reverse_cull);
	}
}

bool Light3D::get_shadow_reverse_cull_face() const { return reverse_cull; }

void Light3D::set_shadow_caster_mask(uint32_t p_caster_mask)
{
	shadow_caster_mask = p_caster_mask;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_shadow_caster_mask(light, shadow_caster_mask);
	}
}

uint32_t Light3D::get_shadow_caster_mask() const { return shadow_caster_mask; }

AABB Light3D::get_aabb() const { return AABB(); }

void Light3D::set_bake_mode(BakeMode p_mode)
{
	bake_mode = p_mode;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_bake_mode(light, RSE::LightBakeMode(p_mode));
	}
}

Light3D::BakeMode Light3D::get_bake_mode() const { return bake_mode; }

void Light3D::set_projector(const Ref<Texture2D>& p_texture)
{
	projector = p_texture;
	if (light.is_valid() && Renderer::data) {
		Renderer::light_set_projector(light, projector.is_valid() ? projector->get_rid() : RID());
	}
}

Ref<Texture2D> Light3D::get_projector() const { return projector; }

void Light3D::owner_changed_notify()
{
	_update_visibility();
}

Color _color_from_temperature(float p_temperature)
{
	float T2 = p_temperature * p_temperature;
	float u = (0.860117757f + 1.54118254e-4f * p_temperature + 1.28641212e-7f * T2) /
			  (1.0f + 8.42420235e-4f * p_temperature + 7.08145163e-7f * T2);
	float v = (0.317398726f + 4.22806245e-5f * p_temperature + 4.20481691e-8f * T2) /
			  (1.0f - 2.89741816e-5f * p_temperature + 1.61456053e-7f * T2);

	float d = 1.0f / (2.0f * u - 8.0f * v + 4.0f);
	float x = 3.0f * u * d;
	float y = 2.0f * v * d;

	const float a = 1.0 / MAX(y, 1e-5f);
	Vector3 xyz = Vector3(x * a, 1.0, (1.0f - x - y) * a);

	Vector3 linear = Vector3(3.2404542f * xyz.x - 1.5371385f * xyz.y - 0.4985314f * xyz.z,
		-0.9692660f * xyz.x + 1.8760108f * xyz.y + 0.0415560f * xyz.z,
		0.0556434f * xyz.x - 0.2040259f * xyz.y + 1.0572252f * xyz.z);
	linear /= MAX(1e-5f, linear[linear.max_axis_index()]);
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
		if (!Engine::is_editor_hint()) {
			editor_ok = false;
		}
		else {
			editor_ok = (SceneTree::get_edited_scene_root() &&
						 (this == SceneTree::get_edited_scene_root() ||
							 get_owner() == SceneTree::get_edited_scene_root()));
		}
	}
#else
	if (editor_only) {
		editor_ok = false;
	}
#endif

	if (get_instance().is_valid() && Renderer::data) {
		Renderer::instance_set_visible(get_instance(), is_visible_in_tree() && editor_ok);
	}
}

void Light3D::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

void Light3D::_notification(int p_what)
{
	render_instance.notification(this, p_what);

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

void Light3D::set_directional_shadow_mode(DirectionalShadowMode p_mode)
{
	directional_shadow_mode = p_mode;
	if (type == RSE::LIGHT_DIRECTIONAL && light.is_valid() && Renderer::data) {
		Renderer::light_directional_set_shadow_mode(
			light, RSE::LightDirectionalShadowMode(p_mode));
	}
}

void Light3D::set_blend_splits(bool p_enable)
{
	blend_splits = p_enable;
	if (type == RSE::LIGHT_DIRECTIONAL && light.is_valid() && Renderer::data) {
		Renderer::light_directional_set_blend_splits(light, p_enable);
	}
}

void Light3D::set_sky_mode(SkyMode p_mode)
{
	sky_mode = p_mode;
	if (type == RSE::LIGHT_DIRECTIONAL && light.is_valid() && Renderer::data) {
		Renderer::light_directional_set_sky_mode(light, RSE::LightDirectionalSkyMode(p_mode));
	}
}

void Light3D::set_omni_shadow_mode(OmniShadowMode p_mode)
{
	omni_shadow_mode = p_mode;
	if (type == RSE::LIGHT_OMNI && light.is_valid() && Renderer::data) {
		Renderer::light_omni_set_shadow_mode(light, RSE::LightOmniShadowMode(p_mode));
	}
}

void Light3D::set_area_size(const Vector2& p_size)
{
	area_size = p_size;
	if (type == RSE::LIGHT_AREA && light.is_valid() && Renderer::data) {
		Renderer::light_area_set_size(light, area_size);
	}
}

void Light3D::set_area_texture(const Ref<Texture2D>& p_texture)
{
	area_texture = p_texture;
	if (type == RSE::LIGHT_AREA && light.is_valid() && Renderer::data) {
		Renderer::light_area_set_texture(
			light, area_texture.is_valid() ? area_texture->get_rid() : RID());
	}
}

void Light3D::set_area_normalize_energy(bool p_enabled)
{
	area_normalize_energy = p_enabled;
	if (type == RSE::LIGHT_AREA && light.is_valid() && Renderer::data) {
		Renderer::light_area_set_normalize_energy(light, p_enabled);
	}
}

PackedStringArray Light3D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();

	if (!get_scale().is_equal_approx(Vector3(1, 1, 1))) {
		warnings.push_back(RTR("A light's scale does not affect the visual size of the light."));
	}

	switch (type) {
	case RSE::LIGHT_OMNI: {
		if (!has_shadow() && get_projector().is_valid()) {
			warnings.push_back(RTR("Projector texture only works with shadows active."));
		}
		if (get_projector().is_valid() &&
			(OS::get_current_rendering_method() == "gl_compatibility" ||
				OS::get_current_rendering_method() == "dummy")) {
			warnings.push_back(
				RTR("Projector textures are not supported when using the Compatibility renderer."));
		}
	} break;

	case RSE::LIGHT_SPOT: {
		if (has_shadow() && get_param(PARAM_SPOT_ANGLE) >= 90.0) {
			warnings.push_back(
				RTR("A SpotLight3D with an angle wider than 90 degrees cannot cast shadows."));
		}
		if (!has_shadow() && get_projector().is_valid()) {
			warnings.push_back(RTR("Projector texture only works with shadows active."));
		}
		if (get_projector().is_valid() &&
			(OS::get_current_rendering_method() == "gl_compatibility" ||
				OS::get_current_rendering_method() == "dummy")) {
			warnings.push_back(
				RTR("Projector textures are not supported when using the Compatibility renderer."));
		}
	} break;

	case RSE::LIGHT_AREA: {
		if (get_projector().is_valid()) {
			warnings.push_back(RTR("Projector texture is not supported for area lights. Use the "
								   "area_texture field instead."));
		}
		if (area_texture.is_valid() && OS::get_current_rendering_method() == "gl_compatibility") {
			warnings.push_back(RTR("Rendering textured area lights is not implemented in the "
								   "Compatibility rendering mode."));
		}
		if (has_shadow() && OS::get_current_rendering_method() == "gl_compatibility") {
			warnings.push_back(RTR(
				"Rendering area light shadows does not work in the Compatibility rendering mode."));
		}
	} break;

	default:
		break;
	}

	return warnings;
}

Light3D::Light3D(RSE::LightType p_type) : type(p_type)
{
	_create_light_rid();
}

Light3D::~Light3D()
{
	_free_light_rid();
}
