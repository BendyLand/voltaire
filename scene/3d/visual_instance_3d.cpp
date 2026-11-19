/**************************************************************************/
/*  visual_instance_3d.cpp                                                */
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

#include "visual_instance_3d.h"

STATIC_ASSERT_INCOMPLETE_TYPE(class, Renderer);

#include "core/math/triangle_mesh.h"
#include "core/os/os.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/material.h"
#include "servers/rendering/renderer.h"

void RenderInstance3D::update_visibility(Node3D* p_owner)
{
	if (!p_owner || !p_owner->is_inside_tree()) {
		return;
	}

	bool already_visible = p_owner->_is_vi_visible();
	bool visible = p_owner->is_visible_in_tree();
	p_owner->_set_vi_visible(visible);

	if (visible && !already_visible) {
		if (!p_owner->_is_using_identity_transform()) {
			Transform3D gt = p_owner->get_global_transform();
			Renderer::instance_set_transform(instance, gt);
		}
	}

	Renderer::instance_set_visible(instance, visible);
}

void RenderInstance3D::set_instance_use_identity_transform(Node3D* p_owner, bool p_enable)
{
	if (!p_owner) {
		return;
	}
	p_owner->_set_use_identity_transform(p_enable);

	if (p_owner->is_inside_tree()) {
		if (p_enable) {
			Renderer::instance_set_transform(instance, Transform3D());
		}
		else {
			Renderer::instance_set_transform(instance, p_owner->get_global_transform());
		}
	}
}

void RenderInstance3D::fti_update_servers_xform(Node3D* p_owner)
{
	if (p_owner && !p_owner->_is_using_identity_transform()) {
		Renderer::instance_set_transform(
			instance, p_owner->_get_cached_global_transform_interpolated());
	}
}

void RenderInstance3D::notification(Node3D* p_owner, int p_what)
{
	if (!p_owner) {
		return;
	}

	switch (p_what) {
	case Node3D::NOTIFICATION_ENTER_WORLD: {
		ERR_FAIL_COND(p_owner->get_world_3d().is_null());
		if (!instance.is_valid() && Renderer::data) {
			instance = Renderer::instance_create();
			if (base.is_valid()) {
				Renderer::instance_set_base(instance, base);
			}
			Renderer::instance_set_layer_mask(instance, layers);
		}
		Renderer::instance_set_scenario(instance, p_owner->get_world_3d()->get_scenario());
		update_visibility(p_owner);
	} break;

	case Node3D::NOTIFICATION_TRANSFORM_CHANGED: {
		if (p_owner->_is_vi_visible() &&
			!(p_owner->is_inside_tree() && SceneTree::is_physics_interpolation_enabled()) &&
			!p_owner->_is_using_identity_transform()) {
			Renderer::instance_set_transform(instance, p_owner->get_global_transform());
		}
	} break;

	case Node::NOTIFICATION_RESET_PHYSICS_INTERPOLATION: {
		if (p_owner->_is_vi_visible() && p_owner->is_inside_tree()) {
			Renderer::instance_teleport(instance);
		}
	} break;

	case Node3D::NOTIFICATION_EXIT_WORLD: {
		Renderer::instance_set_scenario(instance, RID());
		Renderer::instance_attach_skeleton(instance, RID());
		p_owner->_set_vi_visible(false);
	} break;

	case Node3D::NOTIFICATION_VISIBILITY_CHANGED: {
		update_visibility(p_owner);
	} break;
	}
}

RID RenderInstance3D::get_instance() const { return instance; }

void RenderInstance3D::set_layer_mask(uint32_t p_mask)
{
	layers = p_mask;
	if (instance.is_valid()) {
		Renderer::instance_set_layer_mask(instance, p_mask);
	}
}

uint32_t RenderInstance3D::get_layer_mask() const { return layers; }

void RenderInstance3D::set_layer_mask_value(int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Render layer number must be between 1 and 20 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 20, "Render layer number must be between 1 and 20 inclusive.");
	uint32_t mask = get_layer_mask();
	if (p_value) {
		mask |= 1 << (p_layer_number - 1);
	}
	else {
		mask &= ~(1 << (p_layer_number - 1));
	}
	set_layer_mask(mask);
}

bool RenderInstance3D::get_layer_mask_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Render layer number must be between 1 and 20 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 20, false, "Render layer number must be between 1 and 20 inclusive.");
	return layers & (1 << (p_layer_number - 1));
}

void RenderInstance3D::set_sorting_offset(float p_offset)
{
	sorting_offset = p_offset;
	if (instance.is_valid()) {
		Renderer::instance_set_pivot_data(instance, sorting_offset, sorting_use_aabb_center);
	}
}

float RenderInstance3D::get_sorting_offset() const { return sorting_offset; }

void RenderInstance3D::set_sorting_use_aabb_center(bool p_enabled)
{
	sorting_use_aabb_center = p_enabled;
	if (instance.is_valid()) {
		Renderer::instance_set_pivot_data(instance, sorting_offset, sorting_use_aabb_center);
	}
}

bool RenderInstance3D::is_sorting_use_aabb_center() const { return sorting_use_aabb_center; }

void RenderInstance3D::set_base(const RID& p_base)
{
	base = p_base;
	if (!instance.is_valid() && base.is_valid() && Renderer::data) {
		instance = Renderer::instance_create();
		Renderer::instance_set_layer_mask(instance, layers);
	}
	if (instance.is_valid() && Renderer::data) {
		Renderer::instance_set_base(instance, p_base);
	}
}

RID RenderInstance3D::get_base() const { return base; }

void RenderInstance3D::set_material_override(const Ref<Material>& p_material)
{
	material_override = p_material;
	if (instance.is_valid() && Renderer::data) {
		Renderer::instance_geometry_set_material_override(
			instance, p_material.is_valid() ? p_material->get_rid() : RID());
	}
}

Ref<Material> RenderInstance3D::get_material_override() const { return material_override; }

void RenderInstance3D::set_material_overlay(const Ref<Material>& p_material)
{
	material_overlay = p_material;
	if (instance.is_valid() && Renderer::data) {
		Renderer::instance_geometry_set_material_overlay(
			instance, p_material.is_valid() ? p_material->get_rid() : RID());
	}
}

Ref<Material> RenderInstance3D::get_material_overlay() const { return material_overlay; }

float RenderInstance3D::get_transparency() const { return transparency; }

float RenderInstance3D::get_visibility_range_begin() const { return visibility_range_begin; }

float RenderInstance3D::get_visibility_range_end() const { return visibility_range_end; }

float RenderInstance3D::get_visibility_range_begin_margin() const
{
	return visibility_range_begin_margin;
}

float RenderInstance3D::get_visibility_range_end_margin() const
{
	return visibility_range_end_margin;
}

RenderInstance3D::VisibilityRangeFadeMode RenderInstance3D::get_visibility_range_fade_mode() const
{
	return visibility_range_fade_mode;
}

const String* RenderInstance3D::_instance_uniform_get_remap(const String& p_name) const
{
	String* r = instance_shader_parameter_property_remap.getptr(p_name);
	if (!r) {
		String s = p_name;
#ifndef DISABLE_DEPRECATED
		if (s.begins_with("shader_uniforms/")) {
			s = s.replace("shader_uniforms/", "instance_shader_parameters/");
		}
#endif // DISABLE_DEPRECATED
		if (s.begins_with("instance_shader_parameters/")) {
			String pname = String(s);
			String name = s.replace("instance_shader_parameters/", "");
			instance_shader_parameter_property_remap[pname] = name;
			return instance_shader_parameter_property_remap.getptr(pname);
		}
		return nullptr;
	}

	return r;
}

void RenderInstance3D::set_cast_shadows_setting(ShadowCastingSetting p_shadow_casting_setting)
{
	shadow_casting_setting = p_shadow_casting_setting;
	if (instance.is_valid() && Renderer::data) {
		Renderer::instance_geometry_set_cast_shadows_setting(
			instance, (RSE::ShadowCastingSetting)p_shadow_casting_setting);
	}
}

RenderInstance3D::ShadowCastingSetting RenderInstance3D::get_cast_shadows_setting() const
{
	return shadow_casting_setting;
}

void RenderInstance3D::set_extra_cull_margin(float p_margin)
{
	ERR_FAIL_COND(p_margin < 0);
	extra_cull_margin = p_margin;
	if (instance.is_valid() && Renderer::data) {
		Renderer::instance_set_extra_visibility_margin(instance, extra_cull_margin);
	}
}

float RenderInstance3D::get_extra_cull_margin() const { return extra_cull_margin; }

void RenderInstance3D::set_lod_bias(float p_bias)
{
	ERR_FAIL_COND(p_bias < 0.0);
	lod_bias = p_bias;
	if (instance.is_valid() && Renderer::data) {
		Renderer::instance_geometry_set_lod_bias(instance, lod_bias);
	}
}

float RenderInstance3D::get_lod_bias() const { return lod_bias; }

AABB RenderInstance3D::get_custom_aabb() const { return custom_aabb; }

void RenderInstance3D::set_lightmap_texel_scale(float p_scale) { lightmap_texel_scale = p_scale; }

float RenderInstance3D::get_lightmap_texel_scale() const { return lightmap_texel_scale; }

void RenderInstance3D::set_gi_mode(GIMode p_mode)
{
	gi_mode = p_mode;
	if (!instance.is_valid() || !Renderer::data) {
		return;
	}

	switch (p_mode) {
	case GI_MODE_DISABLED: {
		Renderer::instance_geometry_set_flag(instance, RSE::INSTANCE_FLAG_USE_BAKED_LIGHT, false);
		Renderer::instance_geometry_set_flag(instance, RSE::INSTANCE_FLAG_USE_DYNAMIC_GI, false);
	} break;
	case GI_MODE_STATIC: {
		Renderer::instance_geometry_set_flag(instance, RSE::INSTANCE_FLAG_USE_BAKED_LIGHT, true);
		Renderer::instance_geometry_set_flag(instance, RSE::INSTANCE_FLAG_USE_DYNAMIC_GI, false);
	} break;
	case GI_MODE_DYNAMIC: {
		Renderer::instance_geometry_set_flag(instance, RSE::INSTANCE_FLAG_USE_BAKED_LIGHT, false);
		Renderer::instance_geometry_set_flag(instance, RSE::INSTANCE_FLAG_USE_DYNAMIC_GI, true);
	} break;
	}
}

RenderInstance3D::GIMode RenderInstance3D::get_gi_mode() const { return gi_mode; }

void RenderInstance3D::set_ignore_occlusion_culling(bool p_enabled)
{
	ignore_occlusion_culling = p_enabled;
	if (instance.is_valid() && Renderer::data) {
		Renderer::instance_geometry_set_flag(
			instance, RSE::INSTANCE_FLAG_IGNORE_OCCLUSION_CULLING, ignore_occlusion_culling);
	}
}

bool RenderInstance3D::is_ignoring_occlusion_culling() const { return ignore_occlusion_culling; }

void RenderInstance3D::append_geometry_warnings(PackedStringArray& r_warnings) const
{
	if (!Math::is_zero_approx(visibility_range_end) &&
		visibility_range_end <= visibility_range_begin) {
		r_warnings.push_back(
			RTR("The visibility range's End distance is set to a non-zero value, but is lower than "
				"the Begin distance.\nTo resolve this, set the End distance to 0 or to a value "
				"greater than the Begin distance."));
	}

	if ((visibility_range_fade_mode == VISIBILITY_RANGE_FADE_SELF ||
			visibility_range_fade_mode == VISIBILITY_RANGE_FADE_DEPENDENCIES) &&
		!Math::is_zero_approx(visibility_range_begin) &&
		Math::is_zero_approx(visibility_range_begin_margin)) {
		r_warnings.push_back(RTR("Configured to fade in smoothly over distance, but the fade "
								 "transition distance is set to 0.\nTo resolve this, increase "
								 "Visibility Range Begin Margin above 0."));
	}

	if ((visibility_range_fade_mode == VISIBILITY_RANGE_FADE_SELF ||
			visibility_range_fade_mode == VISIBILITY_RANGE_FADE_DEPENDENCIES) &&
		!Math::is_zero_approx(visibility_range_end) &&
		Math::is_zero_approx(visibility_range_end_margin)) {
		r_warnings.push_back(RTR("Configured to fade out smoothly over distance, but the fade "
								 "transition distance is set to 0.\nTo resolve this, increase "
								 "Visibility Range End Margin above 0."));
	}

	if (!Math::is_zero_approx(transparency) &&
		OS::get_current_rendering_method() != "forward_plus") {
		r_warnings.push_back(
			RTR("Instance transparency is only available when using the Forward+ renderer."));
	}

	if ((visibility_range_fade_mode == VISIBILITY_RANGE_FADE_SELF ||
			visibility_range_fade_mode == VISIBILITY_RANGE_FADE_DEPENDENCIES) &&
		OS::get_current_rendering_method() != "forward_plus") {
		r_warnings.push_back(RTR("Visibility range transparency fade is only available when using "
								 "the Forward+ renderer."));
	}
}

RenderInstance3D::~RenderInstance3D()
{
	if (material_overlay.is_valid()) {
		set_material_overlay(Ref<Material>());
	}
	if (material_override.is_valid()) {
		set_material_override(Ref<Material>());
	}
	if (instance.is_valid() && Renderer::data) {
		Renderer::free_rid(instance);
	}
}
