/**************************************************************************/
/*  material.cpp                                                          */
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
#include "core/error/error_macros.h"
#include "core/io/resource_loader.h"
#include "core/os/os.h"
#include "core/version.h"
#include "material.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/texture.h"
#include "servers/rendering/renderer.h"

void Material::set_next_pass(const Ref<Material>& p_pass)
{
	for (Ref<Material> pass_child = p_pass; pass_child.is_valid();
		 pass_child = pass_child->get_next_pass()) {
		ERR_FAIL_COND_MSG(pass_child == this,
			"Can't set as next_pass one of its parents to prevent crashes due to recursive loop.");
	}

	if (next_pass == p_pass) {
		return;
	}

	next_pass = p_pass;

	if (material.is_valid()) {
		RID next_pass_rid;
		if (next_pass.is_valid()) {
			next_pass_rid = next_pass->get_rid();
		}

		Renderer::material_set_next_pass(material, next_pass_rid);
	}
}

Ref<Material> Material::get_next_pass() const { return next_pass; }

void Material::set_render_priority(int p_priority)
{
	ERR_FAIL_COND(p_priority < RENDER_PRIORITY_MIN);
	ERR_FAIL_COND(p_priority > RENDER_PRIORITY_MAX);

	render_priority = p_priority;

	if (material.is_valid()) {
		Renderer::material_set_render_priority(material, p_priority);
	}
}

int Material::get_render_priority() const { return render_priority; }

RID Material::get_rid() const { return material; }

void Material::_mark_ready() { init_state = INIT_STATE_READY; }

Shader::Mode Material::get_shader_mode() const
{
	if (is_placeholder) {
		return Shader::MODE_CANVAS_ITEM;
	}
	return Shader::MODE_MAX;
}

Ref<Resource> Material::create_placeholder() const
{
	Ref<Material> placeholder;
	placeholder.instantiate();
	placeholder->is_placeholder = true;
	return placeholder;
}

Material::Material() { render_priority = 0; }

Material::~Material()
{
	if (material.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::free_rid(material);
	}
}

bool ShaderMaterial::_property_can_revert(const String& p_name) const
{
	if (shader.is_valid()) {
		if (remap_cache.has(p_name)) {
			return true;
		}
		const String sname = p_name;
		return sname == "render_priority" || sname == "next_pass";
	}
	return false;
}

Ref<Shader> ShaderMaterial::get_shader() const { return shader; }

bool ShaderMaterial::_can_do_next_pass() const
{
	return shader.is_valid() && shader->get_mode() == Shader::MODE_SPATIAL;
}

bool ShaderMaterial::_can_use_render_priority() const
{
	return shader.is_valid() && shader->get_mode() == Shader::MODE_SPATIAL;
}

Shader::Mode ShaderMaterial::get_shader_mode() const
{
	if (shader.is_valid()) {
		return shader->get_mode();
	}
	else {
		return Shader::MODE_SPATIAL;
	}
}

RID ShaderMaterial::get_rid() const
{
	_check_material_rid();
	return Material::get_rid();
}

RID ShaderMaterial::get_shader_rid() const
{
	if (shader.is_valid()) {
		return shader->get_rid();
	}
	else {
		return RID();
	}
}

ShaderMaterial::ShaderMaterial()
{
	// Material RID will be empty until it is required.
}

ShaderMaterial::~ShaderMaterial() {}

HashMap<StandardMaterial3D::MaterialKey, StandardMaterial3D::ShaderData, StandardMaterial3D::MaterialKey>
	StandardMaterial3D::shader_map;
Mutex StandardMaterial3D::shader_map_mutex;
StandardMaterial3D::ShaderNames* StandardMaterial3D::shader_names = nullptr;
Mutex StandardMaterial3D::material_mutex;
SelfList<StandardMaterial3D>::List StandardMaterial3D::dirty_materials;

void StandardMaterial3D::init_shaders()
{
	shader_names = memnew(ShaderNames);

	shader_names->albedo = "albedo";
	shader_names->specular = "specular";
	shader_names->roughness = "roughness";
	shader_names->metallic = "metallic";
	shader_names->emission = "emission";
	shader_names->emission_energy = "emission_energy";
	shader_names->normal_scale = "normal_scale";
	shader_names->rim = "rim";
	shader_names->rim_tint = "rim_tint";
	shader_names->clearcoat = "clearcoat";
	shader_names->clearcoat_roughness = "clearcoat_roughness";
	shader_names->anisotropy = "anisotropy_ratio";
	shader_names->heightmap_scale = "heightmap_scale";
	shader_names->subsurface_scattering_strength = "subsurface_scattering_strength";
	shader_names->backlight = "backlight";
	shader_names->refraction = "refraction";
	shader_names->point_size = "point_size";
	shader_names->uv1_scale = "uv1_scale";
	shader_names->uv1_offset = "uv1_offset";
	shader_names->uv2_scale = "uv2_scale";
	shader_names->uv2_offset = "uv2_offset";
	shader_names->uv1_blend_sharpness = "uv1_blend_sharpness";
	shader_names->uv2_blend_sharpness = "uv2_blend_sharpness";

	shader_names->particles_anim_h_frames = "particles_anim_h_frames";
	shader_names->particles_anim_v_frames = "particles_anim_v_frames";
	shader_names->particles_anim_loop = "particles_anim_loop";
	shader_names->heightmap_min_layers = "heightmap_min_layers";
	shader_names->heightmap_max_layers = "heightmap_max_layers";
	shader_names->heightmap_flip = "heightmap_flip";

	shader_names->grow = "grow";

	shader_names->ao_light_affect = "ao_light_affect";

	shader_names->proximity_fade_distance = "proximity_fade_distance";
	shader_names->distance_fade_min = "distance_fade_min";
	shader_names->distance_fade_max = "distance_fade_max";

	shader_names->msdf_pixel_range = "msdf_pixel_range";
	shader_names->msdf_outline_size = "msdf_outline_size";

	shader_names->metallic_texture_channel = "metallic_texture_channel";
	shader_names->ao_texture_channel = "ao_texture_channel";
	shader_names->clearcoat_texture_channel = "clearcoat_texture_channel";
	shader_names->rim_texture_channel = "rim_texture_channel";
	shader_names->heightmap_texture_channel = "heightmap_texture_channel";
	shader_names->refraction_texture_channel = "refraction_texture_channel";

	shader_names->transmittance_color = "transmittance_color";
	shader_names->transmittance_depth = "transmittance_depth";
	shader_names->transmittance_boost = "transmittance_boost";

	shader_names->texture_names[TEXTURE_ALBEDO] = "texture_albedo";
	shader_names->texture_names[TEXTURE_METALLIC] = "texture_metallic";
	shader_names->texture_names[TEXTURE_ROUGHNESS] = "texture_roughness";
	shader_names->texture_names[TEXTURE_EMISSION] = "texture_emission";
	shader_names->texture_names[TEXTURE_NORMAL] = "texture_normal";
	shader_names->texture_names[TEXTURE_BENT_NORMAL] = "texture_bent_normal";
	shader_names->texture_names[TEXTURE_RIM] = "texture_rim";
	shader_names->texture_names[TEXTURE_CLEARCOAT] = "texture_clearcoat";
	shader_names->texture_names[TEXTURE_FLOWMAP] = "texture_flowmap";
	shader_names->texture_names[TEXTURE_AMBIENT_OCCLUSION] = "texture_ambient_occlusion";
	shader_names->texture_names[TEXTURE_HEIGHTMAP] = "texture_heightmap";
	shader_names->texture_names[TEXTURE_SUBSURFACE_SCATTERING] = "texture_subsurface_scattering";
	shader_names->texture_names[TEXTURE_SUBSURFACE_TRANSMITTANCE] =
		"texture_subsurface_transmittance";
	shader_names->texture_names[TEXTURE_BACKLIGHT] = "texture_backlight";
	shader_names->texture_names[TEXTURE_REFRACTION] = "texture_refraction";
	shader_names->texture_names[TEXTURE_DETAIL_MASK] = "texture_detail_mask";
	shader_names->texture_names[TEXTURE_DETAIL_ALBEDO] = "texture_detail_albedo";
	shader_names->texture_names[TEXTURE_DETAIL_NORMAL] = "texture_detail_normal";
	shader_names->texture_names[TEXTURE_ORM] = "texture_orm";

	shader_names->alpha_scissor_threshold = "alpha_scissor_threshold";
	shader_names->alpha_hash_scale = "alpha_hash_scale";

	shader_names->alpha_antialiasing_edge = "alpha_antialiasing_edge";
	shader_names->albedo_texture_size = "albedo_texture_size";
	shader_names->z_clip_scale = "z_clip_scale";
	shader_names->fov_override = "fov_override";
}

HashMap<uint64_t, Ref<StandardMaterial3D>> StandardMaterial3D::materials_for_2d;

void StandardMaterial3D::finish_shaders()
{
	materials_for_2d.clear();

	dirty_materials.clear();

	memdelete(shader_names);
	shader_names = nullptr;
}

void StandardMaterial3D::_update_shader()
{
	if (!_is_initialized()) {
		_mark_ready();
	}

	MaterialKey mk = _compute_key();
	if (mk == current_key) {
		return; // no update required in the end
	}

	{
		MutexLock lock(shader_map_mutex);
		ShaderData* v = shader_map.getptr(current_key);
		if (v) {
			v->users--;
			if (v->users == 0) {
				// Deallocate shader which is no longer in use.
				shader_rid = RID();
				Renderer::free_rid(v->shader);
				shader_map.erase(current_key);
			}
		}

		current_key = mk;

		v = shader_map.getptr(mk);
		if (v) {
			shader_rid = v->shader;
			v->users++;

			if (_get_material().is_valid()) {
				Renderer::material_set_shader(_get_material(), shader_rid);
			}

			return;
		}
	}

	// From this point, it is possible that multiple threads requesting the same key will
	// race to create the shader. The winner, which is the one found in shader_map, will be
	// used. The losers will free their shader.

	String texfilter_str;
	// Force linear filtering for the heightmap texture, as the heightmap effect
	// looks broken with nearest-neighbor filtering (with and without Deep Parallax).
	String texfilter_height_str;
	switch (texture_filter) {
	case TEXTURE_FILTER_NEAREST:
		texfilter_str = "filter_nearest";
		texfilter_height_str = "filter_linear";
		break;
	case TEXTURE_FILTER_LINEAR:
		texfilter_str = "filter_linear";
		texfilter_height_str = "filter_linear";
		break;
	case TEXTURE_FILTER_NEAREST_WITH_MIPMAPS:
		texfilter_str = "filter_nearest_mipmap";
		texfilter_height_str = "filter_linear_mipmap";
		break;
	case TEXTURE_FILTER_LINEAR_WITH_MIPMAPS:
		texfilter_str = "filter_linear_mipmap";
		texfilter_height_str = "filter_linear_mipmap";
		break;
	case TEXTURE_FILTER_NEAREST_WITH_MIPMAPS_ANISOTROPIC:
		texfilter_str = "filter_nearest_mipmap_anisotropic";
		texfilter_height_str = "filter_linear_mipmap_anisotropic";
		break;
	case TEXTURE_FILTER_LINEAR_WITH_MIPMAPS_ANISOTROPIC:
		texfilter_str = "filter_linear_mipmap_anisotropic";
		texfilter_height_str = "filter_linear_mipmap_anisotropic";
		break;
	case TEXTURE_FILTER_MAX:
		break; // Internal value, skip.
	}

	if (flags[FLAG_USE_TEXTURE_REPEAT]) {
		texfilter_str += ", repeat_enable";
		texfilter_height_str += ", repeat_enable";
	}
	else {
		texfilter_str += ", repeat_disable";
		texfilter_height_str += ", repeat_disable";
	}

	// Add a comment to describe the shader origin (useful when converting to ShaderMaterial).
	String code = vformat("// NOTE: Shader automatically converted from " VLTR_VERSION_NAME
						  " " VLTR_VERSION_FULL_CONFIG "'s %s.\n\n",
		orm ? "StandardMaterial3D (ORM)" : "StandardMaterial3D");

	// Define shader type and render mode based on property values.
	code += "shader_type spatial;\nrender_mode ";
	switch (blend_mode) {
	case BLEND_MODE_MIX:
		code += "blend_mix";
		break;
	case BLEND_MODE_ADD:
		code += "blend_add";
		break;
	case BLEND_MODE_SUB:
		code += "blend_sub";
		break;
	case BLEND_MODE_MUL:
		code += "blend_mul";
		break;
	case BLEND_MODE_PREMULT_ALPHA:
		code += "blend_premul_alpha";
		break;
	case BLEND_MODE_MAX:
		break; // Internal value, skip.
	}

	DepthDrawMode ddm = depth_draw_mode;
	if (features[FEATURE_REFRACTION]) {
		ddm = DEPTH_DRAW_ALWAYS;
	}

	switch (ddm) {
	case DEPTH_DRAW_OPAQUE_ONLY:
		code += ", depth_draw_opaque";
		break;
	case DEPTH_DRAW_ALWAYS:
		code += ", depth_draw_always";
		break;
	case DEPTH_DRAW_DISABLED:
		code += ", depth_draw_never";
		break;
	case DEPTH_DRAW_MAX:
		break; // Internal value, skip.
	}

	switch (cull_mode) {
	case CULL_BACK:
		code += ", cull_back";
		break;
	case CULL_FRONT:
		code += ", cull_front";
		break;
	case CULL_DISABLED:
		code += ", cull_disabled";
		break;
	case CULL_MAX:
		break; // Internal value, skip.
	}
	switch (diffuse_mode) {
	case DIFFUSE_BURLEY:
		code += ", diffuse_burley";
		break;
	case DIFFUSE_LAMBERT:
		code += ", diffuse_lambert";
		break;
	case DIFFUSE_LAMBERT_WRAP:
		code += ", diffuse_lambert_wrap";
		break;
	case DIFFUSE_TOON:
		code += ", diffuse_toon";
		break;
	case DIFFUSE_MAX:
		break; // Internal value, skip.
	}
	switch (specular_mode) {
	case SPECULAR_SCHLICK_GGX:
		code += ", specular_schlick_ggx";
		break;
	case SPECULAR_TOON:
		code += ", specular_toon";
		break;
	case SPECULAR_DISABLED:
		code += ", specular_disabled";
		break;
	case SPECULAR_MAX:
		break; // Internal value, skip.
	}
	if (features[FEATURE_SUBSURFACE_SCATTERING] && flags[FLAG_SUBSURFACE_MODE_SKIN]) {
		code += ", sss_mode_skin";
	}

	if (shading_mode == SHADING_MODE_UNSHADED) {
		code += ", unshaded";
	}
	if (flags[FLAG_DISABLE_DEPTH_TEST]) {
		code += ", depth_test_disabled";
	}
	else {
		switch (depth_test) {
		case DEPTH_TEST_DEFAULT:
			// depth_test_default is the default behavior, no need to emit it here.
			break;
		case DEPTH_TEST_INVERTED:
			code += ", depth_test_inverted";
			break;
		case DEPTH_TEST_MAX:
			break; // Internal value, skip.
		}
	}
	if (flags[FLAG_PARTICLE_TRAILS_MODE]) {
		code += ", particle_trails";
	}
	if (shading_mode == SHADING_MODE_PER_VERTEX) {
		code += ", vertex_lighting";
	}
	if (flags[FLAG_DONT_RECEIVE_SHADOWS]) {
		code += ", shadows_disabled";
	}
	if (flags[FLAG_DISABLE_AMBIENT_LIGHT]) {
		code += ", ambient_light_disabled";
	}
	if (flags[FLAG_USE_SHADOW_TO_OPACITY]) {
		code += ", shadow_to_opacity";
	}
	if (flags[FLAG_DISABLE_FOG]) {
		code += ", fog_disabled";
	}
	if (flags[FLAG_DISABLE_SPECULAR_OCCLUSION]) {
		code += ", specular_occlusion_disabled";
	}
	if (transparency == TRANSPARENCY_ALPHA_DEPTH_PRE_PASS) {
		code += ", depth_prepass_alpha";
	}

	// Although it's technically possible to do alpha antialiasing without using alpha hash or alpha
	// scissor, it is restricted in the base material because it has no use, and abusing it with
	// regular Alpha blending can saturate the MSAA mask.
	if (transparency == TRANSPARENCY_ALPHA_HASH || transparency == TRANSPARENCY_ALPHA_SCISSOR) {
		// Alpha antialiasing is only useful with ALPHA_HASH or ALPHA_SCISSOR.
		if (alpha_antialiasing_mode == ALPHA_ANTIALIASING_ALPHA_TO_COVERAGE) {
			code += ", alpha_to_coverage";
		}
		else if (alpha_antialiasing_mode == ALPHA_ANTIALIASING_ALPHA_TO_COVERAGE_AND_TO_ONE) {
			code += ", alpha_to_coverage_and_one";
		}
	}

	code += ";\n";

	if (stencil_mode != STENCIL_MODE_DISABLED && stencil_flags != 0) {
		code += "stencil_mode ";

		if (stencil_flags & STENCIL_FLAG_READ) {
			code += "read";
		}

		if (stencil_flags & STENCIL_FLAG_WRITE) {
			if (stencil_flags & STENCIL_FLAG_READ) {
				code += ", ";
			}
			code += "write";
		}

		if (stencil_flags & STENCIL_FLAG_WRITE_DEPTH_FAIL) {
			if (stencil_flags & (STENCIL_FLAG_READ | STENCIL_FLAG_WRITE)) {
				code += ", ";
			}
			code += "write_depth_fail";
		}

		switch (stencil_compare) {
		case STENCIL_COMPARE_ALWAYS:
			code += ", compare_always";
			break;
		case STENCIL_COMPARE_LESS:
			code += ", compare_less";
			break;
		case STENCIL_COMPARE_EQUAL:
			code += ", compare_equal";
			break;
		case STENCIL_COMPARE_LESS_OR_EQUAL:
			code += ", compare_less_or_equal";
			break;
		case STENCIL_COMPARE_GREATER:
			code += ", compare_greater";
			break;
		case STENCIL_COMPARE_NOT_EQUAL:
			code += ", compare_not_equal";
			break;
		case STENCIL_COMPARE_GREATER_OR_EQUAL:
			code += ", compare_greater_or_equal";
			break;
		case STENCIL_COMPARE_MAX:
			break;
		}

		code += vformat(", %s;\n", stencil_reference);
	}

	// Generate list of uniforms.
	code += vformat(R"(
uniform vec4 albedo : source_color;
uniform sampler2D texture_albedo : source_color, %s;
)",
		texfilter_str);

	if (grow_enabled) {
		code += "uniform float grow : hint_range(-16.0, 16.0, 0.001);\n";
	}

	if (proximity_fade_enabled) {
		code += "uniform float proximity_fade_distance : hint_range(0.0, 4096.0, 0.01);\n";
	}
	if (distance_fade != DISTANCE_FADE_DISABLED) {
		code += R"(
uniform float distance_fade_min : hint_range(0.0, 4096.0, 0.01);
uniform float distance_fade_max : hint_range(0.0, 4096.0, 0.01);
)";
	}

	if (flags[FLAG_ALBEDO_TEXTURE_MSDF] && flags[FLAG_UV1_USE_TRIPLANAR]) {
		String msg = "MSDF is not supported on triplanar materials. Ignoring MSDF in favor of "
					 "triplanar mapping.";
		if (textures[TEXTURE_ALBEDO].is_valid()) {
			WARN_PRINT(vformat(
				"%s (albedo %s): " + msg, get_path(), textures[TEXTURE_ALBEDO]->get_path()));
		}
		else if (!get_path().is_empty()) {
			WARN_PRINT(vformat("%s: " + msg, get_path()));
		}
		else {
			WARN_PRINT(msg);
		}
	}

	if (flags[FLAG_ALBEDO_TEXTURE_MSDF] && !flags[FLAG_UV1_USE_TRIPLANAR]) {
		code += R"(
uniform float msdf_pixel_range : hint_range(1.0, 100.0, 1.0);
uniform float msdf_outline_size : hint_range(0.0, 250.0, 1.0);
)";
	}

	// Alpha scissor is only valid if there is no antialiasing edge.
	// Alpha hash is valid whenever, but not with alpha scissor.
	if (transparency == TRANSPARENCY_ALPHA_SCISSOR) {
		code += "uniform float alpha_scissor_threshold : hint_range(0.0, 1.0, 0.001);\n";
	}
	else if (transparency == TRANSPARENCY_ALPHA_HASH) {
		code += "uniform float alpha_hash_scale : hint_range(0.0, 2.0, 0.01);\n";
	}
	// If alpha antialiasing isn't off, add in the edge variable.
	if (alpha_antialiasing_mode != ALPHA_ANTIALIASING_OFF &&
		(transparency == TRANSPARENCY_ALPHA_SCISSOR || transparency == TRANSPARENCY_ALPHA_HASH)) {
		code += "uniform float alpha_antialiasing_edge : hint_range(0.0, 1.0, 0.01);\n";
	}

	code += "uniform ivec2 albedo_texture_size;\n";
	code += "uniform float point_size : hint_range(0.1, 128.0, 0.1);\n";

	if (!orm) {
		code += vformat(R"(
uniform float roughness : hint_range(0.0, 1.0);
uniform sampler2D texture_metallic : hint_default_white, %s;
uniform vec4 metallic_texture_channel;
)",
			texfilter_str);
		switch (roughness_texture_channel) {
		case TEXTURE_CHANNEL_RED: {
			code += vformat(
				"uniform sampler2D texture_roughness : hint_roughness_r, %s;\n", texfilter_str);
		} break;
		case TEXTURE_CHANNEL_GREEN: {
			code += vformat(
				"uniform sampler2D texture_roughness : hint_roughness_g, %s;\n", texfilter_str);
		} break;
		case TEXTURE_CHANNEL_BLUE: {
			code += vformat(
				"uniform sampler2D texture_roughness : hint_roughness_b, %s;\n", texfilter_str);
		} break;
		case TEXTURE_CHANNEL_ALPHA: {
			code += vformat(
				"uniform sampler2D texture_roughness : hint_roughness_a, %s;\n", texfilter_str);
		} break;
		case TEXTURE_CHANNEL_GRAYSCALE: {
			code += vformat(
				"uniform sampler2D texture_roughness : hint_roughness_gray, %s;\n", texfilter_str);
		} break;
		case TEXTURE_CHANNEL_MAX:
			break; // Internal value, skip.
		}

		code += R"(
uniform float specular : hint_range(0.0, 1.0, 0.01);
uniform float metallic : hint_range(0.0, 1.0, 0.01);
)";
	}
	else {
		code += "uniform sampler2D texture_orm : hint_roughness_g, " + texfilter_str + ";\n";
	}

	if (billboard_mode == BILLBOARD_PARTICLES) {
		code += R"(
uniform int particles_anim_h_frames : hint_range(1, 128);
uniform int particles_anim_v_frames : hint_range(1, 128);
uniform bool particles_anim_loop;
)";
	}

	if (features[FEATURE_EMISSION]) {
		code += vformat(R"(
uniform sampler2D texture_emission : source_color, hint_default_black, %s;
uniform vec4 emission : source_color;
uniform float emission_energy : hint_range(0.0, 100.0, 0.01);
)",
			texfilter_str);
	}

	if (features[FEATURE_REFRACTION]) {
		code += vformat(R"(
uniform sampler2D texture_refraction : %s;
uniform float refraction : hint_range(-1.0, 1.0, 0.001);
uniform vec4 refraction_texture_channel;
)",
			texfilter_str);
	}

	if (features[FEATURE_REFRACTION]) {
		code += "uniform sampler2D screen_texture : hint_screen_texture, repeat_disable, "
				"filter_linear_mipmap;\n";
	}

	if (features[FEATURE_REFRACTION] || proximity_fade_enabled) {
		code += "uniform sampler2D depth_texture : hint_depth_texture, repeat_disable, "
				"filter_nearest;\n";
	}

	if (features[FEATURE_NORMAL_MAPPING]) {
		code += vformat(R"(
uniform sampler2D texture_normal : hint_roughness_normal, %s;
uniform float normal_scale : hint_range(-16.0, 16.0);
)",
			texfilter_str);
	}
	if (features[FEATURE_BENT_NORMAL_MAPPING]) {
		code += vformat(R"(
uniform sampler2D texture_bent_normal : hint_roughness_normal, %s;
)",
			texfilter_str);
	}
	if (features[FEATURE_RIM]) {
		code += vformat(R"(
uniform float rim : hint_range(0.0, 1.0, 0.01);
uniform float rim_tint : hint_range(0.0, 1.0, 0.01);
uniform sampler2D texture_rim : hint_default_white, %s;
)",
			texfilter_str);
	}
	if (features[FEATURE_CLEARCOAT]) {
		code += vformat(R"(
uniform float clearcoat : hint_range(0.0, 1.0, 0.01);
uniform float clearcoat_roughness : hint_range(0.0, 1.0, 0.01);
uniform sampler2D texture_clearcoat : hint_default_white, %s;
)",
			texfilter_str);
	}
	if (features[FEATURE_ANISOTROPY]) {
		code += vformat(R"(
uniform float anisotropy_ratio : hint_range(0.0, 1.0, 0.01);
uniform sampler2D texture_flowmap : hint_anisotropy, %s;
)",
			texfilter_str);
	}
	if (features[FEATURE_AMBIENT_OCCLUSION]) {
		code += vformat(R"(
uniform sampler2D texture_ambient_occlusion : hint_default_white, %s;
uniform vec4 ao_texture_channel;
uniform float ao_light_affect : hint_range(0.0, 1.0, 0.01);
)",
			texfilter_str);
	}

	if (features[FEATURE_DETAIL]) {
		code += vformat(R"(
uniform sampler2D texture_detail_albedo : source_color, %s;
uniform sampler2D texture_detail_normal : hint_normal, %s;
uniform sampler2D texture_detail_mask : hint_default_white, %s;
)",
			texfilter_str, texfilter_str, texfilter_str);
	}

	if (features[FEATURE_SUBSURFACE_SCATTERING]) {
		code += vformat(R"(
uniform float subsurface_scattering_strength : hint_range(0.0, 1.0, 0.01);
uniform sampler2D texture_subsurface_scattering : hint_default_white, %s;
)",
			texfilter_str);
	}

	if (features[FEATURE_SUBSURFACE_TRANSMITTANCE]) {
		code += vformat(R"(
uniform vec4 transmittance_color : source_color;
uniform float transmittance_depth : hint_range(0.001, 8.0, 0.001);
uniform sampler2D texture_subsurface_transmittance : hint_default_white, %s;
uniform float transmittance_boost : hint_range(0.0, 1.0, 0.01);
)",
			texfilter_str);
	}

	if (features[FEATURE_BACKLIGHT]) {
		code += vformat(R"(
uniform vec4 backlight : source_color;
uniform sampler2D texture_backlight : hint_default_black, %s;
)",
			texfilter_str);
	}

	if (features[FEATURE_HEIGHT_MAPPING]) {
		code += vformat(R"(
uniform sampler2D texture_heightmap : hint_default_black, %s;
uniform float heightmap_scale : hint_range(-16.0, 16.0, 0.001);
uniform int heightmap_min_layers : hint_range(1, 64);
uniform int heightmap_max_layers : hint_range(1, 64);
uniform vec2 heightmap_flip;
)",
			texfilter_height_str);
	}
	if (flags[FLAG_UV1_USE_TRIPLANAR]) {
		code += "varying vec3 uv1_triplanar_pos;\n";
	}
	if (flags[FLAG_UV2_USE_TRIPLANAR]) {
		code += "varying vec3 uv2_triplanar_pos;\n";
	}
	if (flags[FLAG_UV1_USE_TRIPLANAR]) {
		code += R"(
uniform float uv1_blend_sharpness : hint_range(0.0, 150.0, 0.001);
varying vec3 uv1_power_normal;
)";
	}

	if (flags[FLAG_UV2_USE_TRIPLANAR]) {
		code += R"(uniform float uv2_blend_sharpness : hint_range(0.0, 150.0, 0.001);
varying vec3 uv2_power_normal;
)";
	}

	code += R"(
uniform vec3 uv1_scale;
uniform vec3 uv1_offset;
uniform vec3 uv2_scale;
uniform vec3 uv2_offset;
)";

	if (flags[FLAG_USE_Z_CLIP_SCALE]) {
		code += "uniform float z_clip_scale : hint_range(0.01, 1.0, 0.01);\n";
	}

	if (flags[FLAG_USE_FOV_OVERRIDE]) {
		code += "uniform float fov_override : hint_range(1.0, 179.0, 0.1);\n";
	}

	// Generate vertex shader.
	code += R"(
void vertex() {)";

	if (flags[FLAG_SRGB_VERTEX_COLOR]) {
		code += R"(
	// Vertex Color is sRGB: Enabled
	if (!OUTPUT_IS_SRGB) {
		COLOR.rgb = mix(
				pow((COLOR.rgb + vec3(0.055)) * (1.0 / (1.0 + 0.055)), vec3(2.4)),
				COLOR.rgb * (1.0 / 12.92),
				lessThan(COLOR.rgb, vec3(0.04045)));
	}
)";
	}
	if (flags[FLAG_USE_POINT_SIZE]) {
		code += R"(
	// Use Point Size: Enabled
	POINT_SIZE = point_size;
)";
	}

	if (shading_mode == SHADING_MODE_PER_VERTEX) {
		code += R"(
	// Shading Mode: Per Vertex
	ROUGHNESS = roughness;
)";
	}

	if (!flags[FLAG_UV1_USE_TRIPLANAR]) {
		code += R"(
	UV = UV * uv1_scale.xy + uv1_offset.xy;
)";
	}

	if (detail_uv == DETAIL_UV_2 && !flags[FLAG_UV2_USE_TRIPLANAR]) {
		// Don't add a newline if the UV assignment above is already performed,
		// so that UV1 and UV2 are closer to each other.
		if (flags[FLAG_UV1_USE_TRIPLANAR]) {
			code += "\n";
		}
		code += R"(	// Detail UV Layer: UV2
	UV2 = UV2 * uv2_scale.xy + uv2_offset.xy;
)";
	}

	switch (billboard_mode) {
	case BILLBOARD_DISABLED: {
	} break;
	case BILLBOARD_ENABLED: {
		// `MAIN_CAM_INV_VIEW_MATRIX` is inverse of the camera, even on shadow passes.
		// This ensures the billboard faces the camera when casting shadows.
		code += R"(
	// Billboard Mode: Enabled
	MODELVIEW_MATRIX = VIEW_MATRIX * mat4(
			MAIN_CAM_INV_VIEW_MATRIX[0],
			MAIN_CAM_INV_VIEW_MATRIX[1],
			MAIN_CAM_INV_VIEW_MATRIX[2],
			MODEL_MATRIX[3]);
)";
		if (flags[FLAG_BILLBOARD_KEEP_SCALE]) {
			code += R"(
	// Billboard Keep Scale: Enabled
	MODELVIEW_MATRIX = MODELVIEW_MATRIX * mat4(
			vec4(length(MODEL_MATRIX[0].xyz), 0.0, 0.0, 0.0),
			vec4(0.0, length(MODEL_MATRIX[1].xyz), 0.0, 0.0),
			vec4(0.0, 0.0, length(MODEL_MATRIX[2].xyz), 0.0),
			vec4(0.0, 0.0, 0.0, 1.0));
)";
		}
		code += "	MODELVIEW_NORMAL_MATRIX = mat3(MODELVIEW_MATRIX);\n";
	} break;
	case BILLBOARD_FIXED_Y: {
		// `MAIN_CAM_INV_VIEW_MATRIX` is inverse of the camera, even on shadow passes.
		// This ensures the billboard faces the camera when casting shadows.
		code += R"(
	// Billboard Mode: Y-Billboard
	MODELVIEW_MATRIX = VIEW_MATRIX * mat4(
			vec4(normalize(cross(vec3(0.0, 1.0, 0.0), MAIN_CAM_INV_VIEW_MATRIX[2].xyz)), 0.0),
			vec4(0.0, 1.0, 0.0, 0.0),
			vec4(normalize(cross(MAIN_CAM_INV_VIEW_MATRIX[0].xyz, vec3(0.0, 1.0, 0.0))), 0.0),
			MODEL_MATRIX[3]);
)";
		if (flags[FLAG_BILLBOARD_KEEP_SCALE]) {
			code += R"(
	// Billboard Keep Scale: Enabled
	MODELVIEW_MATRIX = MODELVIEW_MATRIX * mat4(
			vec4(length(MODEL_MATRIX[0].xyz), 0.0, 0.0, 0.0),
			vec4(0.0, length(MODEL_MATRIX[1].xyz), 0.0, 0.0),
			vec4(0.0, 0.0, length(MODEL_MATRIX[2].xyz), 0.0),
			vec4(0.0, 0.0, 0.0, 1.0));
)";
		}
		code += "	MODELVIEW_NORMAL_MATRIX = mat3(MODELVIEW_MATRIX);\n";
	} break;
	case BILLBOARD_PARTICLES: {
		// Make billboard and rotated by rotation.
		code += R"(
	// Billboard Mode: Particles
	mat4 mat_world = mat4(
			normalize(INV_VIEW_MATRIX[0]),
			normalize(INV_VIEW_MATRIX[1]),
			normalize(INV_VIEW_MATRIX[2]),
			MODEL_MATRIX[3]);
	mat_world = mat_world * mat4(
			vec4(cos(INSTANCE_CUSTOM.x), -sin(INSTANCE_CUSTOM.x), 0.0, 0.0),
			vec4(sin(INSTANCE_CUSTOM.x), cos(INSTANCE_CUSTOM.x), 0.0, 0.0),
			vec4(0.0, 0.0, 1.0, 0.0),
			vec4(0.0, 0.0, 0.0, 1.0));
)";
		// Set modelview.
		code += "	MODELVIEW_MATRIX = VIEW_MATRIX * mat_world;\n";
		if (flags[FLAG_BILLBOARD_KEEP_SCALE]) {
			code += R"(
	// Billboard Keep Scale: Enabled
	MODELVIEW_MATRIX = MODELVIEW_MATRIX * mat4(
			vec4(length(MODEL_MATRIX[0].xyz), 0.0, 0.0, 0.0),
			vec4(0.0, length(MODEL_MATRIX[1].xyz), 0.0, 0.0),
			vec4(0.0, 0.0, length(MODEL_MATRIX[2].xyz), 0.0),
			vec4(0.0, 0.0, 0.0, 1.0));
)";
		}
		// Set modelview normal and handle animation.
		code += R"(
	MODELVIEW_NORMAL_MATRIX = mat3(MODELVIEW_MATRIX);

	float h_frames = float(particles_anim_h_frames);
	float v_frames = float(particles_anim_v_frames);
	float particle_total_frames = float(particles_anim_h_frames * particles_anim_v_frames);
	float particle_frame = floor(INSTANCE_CUSTOM.z * float(particle_total_frames));
	if (!particles_anim_loop) {
		particle_frame = clamp(particle_frame, 0.0, particle_total_frames - 1.0);
	}
	else {
		particle_frame = mod(particle_frame, particle_total_frames);
	}
	UV /= vec2(h_frames, v_frames);
	UV += vec2(mod(particle_frame, h_frames) / h_frames, floor((particle_frame + 0.5) / h_frames) / v_frames);
)";
	} break;
	case BILLBOARD_MAX:
		break; // Internal value, skip.
	}

	if (flags[FLAG_FIXED_SIZE]) {
		code += R"(
	// Fixed Size: Enabled
	if (PROJECTION_MATRIX[3][3] != 0.0) {
		// Orthogonal matrix; try to do about the same with viewport size.
		float h = abs(1.0 / (2.0 * PROJECTION_MATRIX[1][1]));
		// Consistent with vertical FOV (Keep Height).
		float sc = (h * 2.0);
		MODELVIEW_MATRIX[0] *= sc;
		MODELVIEW_MATRIX[1] *= sc;
		MODELVIEW_MATRIX[2] *= sc;
	}
	else {
		// Scale by depth.
		float sc;
		if (IS_MULTIVIEW) {
			// Assuming stereo rendering.
			// Moving in the z-plane gives the illusion of the object growing/shrinking in size.
			// We need to take the full distance to camera to compensate.
			sc = length((MODELVIEW_MATRIX)[3].xyz);
		}
		else {
			sc = -(MODELVIEW_MATRIX)[3].z;
		}
		MODELVIEW_MATRIX[0] *= sc;
		MODELVIEW_MATRIX[1] *= sc;
		MODELVIEW_MATRIX[2] *= sc;
	}
)";
	}

	if (flags[FLAG_UV1_USE_TRIPLANAR] || flags[FLAG_UV2_USE_TRIPLANAR]) {
		// Generate tangent and binormal in world space.
		if (flags[FLAG_UV1_USE_WORLD_TRIPLANAR]) {
			code += R"(
	vec3 normal = MODEL_NORMAL_MATRIX * NORMAL;
)";
		}
		else {
			code += R"(
	vec3 normal = NORMAL;
)";
		}
		code += R"(
	TANGENT = vec3(0.0, 0.0, -1.0) * abs(normal.x);
	TANGENT += vec3(1.0, 0.0, 0.0) * abs(normal.y);
	TANGENT += vec3(1.0, 0.0, 0.0) * abs(normal.z);
)";
		if (flags[FLAG_UV1_USE_WORLD_TRIPLANAR]) {
			code += "	TANGENT = inverse(MODEL_NORMAL_MATRIX) * normalize(TANGENT);\n";
		}
		else {
			code += "	TANGENT = normalize(TANGENT);\n";
		}

		code += R"(
	BINORMAL = vec3(0.0, 1.0, 0.0) * abs(normal.x);
	BINORMAL += vec3(0.0, 0.0, -1.0) * abs(normal.y);
	BINORMAL += vec3(0.0, 1.0, 0.0) * abs(normal.z);
)";
		if (flags[FLAG_UV1_USE_WORLD_TRIPLANAR]) {
			code += "	BINORMAL = inverse(MODEL_NORMAL_MATRIX) * normalize(BINORMAL);\n";
		}
		else {
			code += "	BINORMAL = normalize(BINORMAL);\n";
		}
	}

	if (flags[FLAG_UV1_USE_TRIPLANAR]) {
		if (flags[FLAG_UV1_USE_WORLD_TRIPLANAR]) {
			code += R"(
	// UV1 Triplanar: Enabled (with World Triplanar)
	uv1_power_normal = pow(abs(normal), vec3(uv1_blend_sharpness));
	uv1_triplanar_pos = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz * uv1_scale + uv1_offset;
)";
		}
		else {
			code += R"(
	// UV1 Triplanar: Enabled
	uv1_power_normal = pow(abs(NORMAL), vec3(uv1_blend_sharpness));
	uv1_triplanar_pos = VERTEX * uv1_scale + uv1_offset;
)";
		}
		code += R"(	uv1_power_normal /= dot(uv1_power_normal, vec3(1.0));
	uv1_triplanar_pos *= vec3(1.0, -1.0, 1.0);
)";
	}

	if (flags[FLAG_UV2_USE_TRIPLANAR]) {
		if (flags[FLAG_UV2_USE_WORLD_TRIPLANAR]) {
			code += R"(
	// UV2 Triplanar: Enabled (with World Triplanar)
	uv2_power_normal = pow(abs(mat3(MODEL_MATRIX) * NORMAL), vec3(uv2_blend_sharpness));
	uv2_triplanar_pos = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz * uv2_scale + uv2_offset;
)";
		}
		else {
			code += R"(
	// UV2 Triplanar: Enabled
	uv2_power_normal = pow(abs(NORMAL), vec3(uv2_blend_sharpness));
	uv2_triplanar_pos = VERTEX * uv2_scale + uv2_offset;
)";
		}
		code += R"(	uv2_power_normal /= dot(uv2_power_normal, vec3(1.0));
	uv2_triplanar_pos *= vec3(1.0, -1.0, 1.0);
)";
	}

	if (grow_enabled) {
		code += R"(
	// Grow: Enabled
	VERTEX += NORMAL * grow;
)";
	}

	if (flags[FLAG_USE_Z_CLIP_SCALE]) {
		code += R"(
	Z_CLIP_SCALE = z_clip_scale;
)";
	}

	if (flags[FLAG_USE_FOV_OVERRIDE]) {
		code += R"(
	if (!IN_SHADOW_PASS) {
		float flip_y = sign(PROJECTION_MATRIX[1][1]);
		float aspect = PROJECTION_MATRIX[1][1] / PROJECTION_MATRIX[0][0];
		float f = flip_y / tan(fov_override * PI / 360.0);
		PROJECTION_MATRIX[0][0] = f / aspect;
		PROJECTION_MATRIX[1][1] = f;
	}
)";
	}

	// End of the vertex shader function.
	code += "}\n";

	if (flags[FLAG_ALBEDO_TEXTURE_MSDF] && !flags[FLAG_UV1_USE_TRIPLANAR]) {
		code += R"(
float msdf_median(float r, float g, float b) {
	return max(min(r, g), min(max(r, g), b));
}
)";
	}

	if (flags[FLAG_UV1_USE_TRIPLANAR] || flags[FLAG_UV2_USE_TRIPLANAR]) {
		code += R"(
vec4 triplanar_texture(sampler2D p_sampler, vec3 p_weights, vec3 p_triplanar_pos) {
	vec4 samp = vec4(0.0);
	samp += texture(p_sampler, p_triplanar_pos.xy) * p_weights.z;
	samp += texture(p_sampler, p_triplanar_pos.xz) * p_weights.y;
	samp += texture(p_sampler, p_triplanar_pos.zy * vec2(-1.0, 1.0)) * p_weights.x;
	return samp;
}
)";
	}

	// Generate fragment shader.
	code += R"(
void fragment() {)";

	if (!flags[FLAG_UV1_USE_TRIPLANAR]) {
		code += R"(
	vec2 base_uv = UV;
)";
	}

	if ((features[FEATURE_DETAIL] && detail_uv == DETAIL_UV_2) ||
		(features[FEATURE_AMBIENT_OCCLUSION] && flags[FLAG_AO_ON_UV2]) ||
		(features[FEATURE_EMISSION] && flags[FLAG_EMISSION_ON_UV2])) {
		// Don't add a newline if the UV assignment above is already performed,
		// so that UV1 and UV2 are closer to each other.
		if (flags[FLAG_UV1_USE_TRIPLANAR]) {
			code += "\n";
		}
		code += R"(	vec2 base_uv2 = UV2;
)";
	}

	if (features[FEATURE_HEIGHT_MAPPING] && flags[FLAG_UV1_USE_TRIPLANAR]) {
		// Display both resource name and albedo texture name.
		// Materials are often built-in to scenes, so displaying the resource name alone may not be
		// meaningful. On the other hand, albedo textures are almost always external to the scene.
		if (textures[TEXTURE_ALBEDO].is_valid()) {
			WARN_PRINT(vformat("%s (albedo %s): Height mapping is not supported on triplanar "
							   "materials. Ignoring height mapping in favor of triplanar mapping.",
				get_path(), textures[TEXTURE_ALBEDO]->get_path()));
		}
		else if (!get_path().is_empty()) {
			WARN_PRINT(vformat("%s: Height mapping is not supported on triplanar materials. "
							   "Ignoring height mapping in favor of triplanar mapping.",
				get_path()));
		}
		else {
			// Resource wasn't saved yet.
			WARN_PRINT("Height mapping is not supported on triplanar materials. Ignoring height "
					   "mapping in favor of triplanar mapping.");
		}
	}

	// Heightmapping isn't supported at the same time as triplanar mapping.
	if (features[FEATURE_HEIGHT_MAPPING] && !flags[FLAG_UV1_USE_TRIPLANAR]) {
		// Binormal is negative due to mikktspace. Flipping it "unflips" it.
		code += R"(
	{
		// Height: Enabled
		vec3 view_dir = normalize(normalize(-VERTEX + EYE_OFFSET) * mat3(TANGENT * heightmap_flip.x, -BINORMAL * heightmap_flip.y, NORMAL));
)";

		if (deep_parallax) {
			// Multiply the heightmap scale by 0.01 to improve heightmap scale usability.
			code += R"(
		// Height Deep Parallax: Enabled
		float num_layers = mix(float(heightmap_max_layers), float(heightmap_min_layers), abs(dot(vec3(0.0, 0.0, 1.0), view_dir)));
		float layer_depth = 1.0 / num_layers;
		float current_layer_depth = 0.0;
		vec2 p = view_dir.xy * heightmap_scale * 0.01;
		vec2 delta = p / num_layers;
		vec2 ofs = base_uv;
)";
			if (flags[FLAG_INVERT_HEIGHTMAP]) {
				code += "		float depth = texture(texture_heightmap, ofs).r;\n";
			}
			else {
				code += "		float depth = 1.0 - texture(texture_heightmap, ofs).r;\n";
			}
			code += R"(
		float current_depth = 0.0;
		while (current_depth < depth) {
			ofs -= delta;
)";
			if (flags[FLAG_INVERT_HEIGHTMAP]) {
				code += "			depth = texture(texture_heightmap, ofs).r;\n";
			}
			else {
				code += "			depth = 1.0 - texture(texture_heightmap, ofs).r;\n";
			}
			code += R"(
			current_depth += layer_depth;
		}

		vec2 prev_ofs = ofs + delta;
		float after_depth = depth - current_depth;
)";
			if (flags[FLAG_INVERT_HEIGHTMAP]) {
				code += "		float before_depth = texture(texture_heightmap, prev_ofs).r - "
						"current_depth + layer_depth;\n";
			}
			else {
				code += "		float before_depth = (1.0 - texture(texture_heightmap, "
						"prev_ofs).r) - current_depth + layer_depth;\n";
			}
			code += R"(
		float weight = after_depth / (after_depth - before_depth);
		ofs = mix(ofs, prev_ofs, weight);
)";

		}
		else {
			if (flags[FLAG_INVERT_HEIGHTMAP]) {
				code += "		float depth = texture(texture_heightmap, base_uv).r;\n";
			}
			else {
				code += "		float depth = 1.0 - texture(texture_heightmap, base_uv).r;\n";
			}
			// Use offset limiting to improve the appearance of non-deep parallax.
			// This reduces the impression of depth, but avoids visible warping in the distance.
			// Multiply the heightmap scale by 0.01 to improve heightmap scale usability.
			code += "		vec2 ofs = base_uv - view_dir.xy * depth * heightmap_scale * 0.01;\n";
		}

		code += "		base_uv = ofs;\n";
		if (features[FEATURE_DETAIL] && detail_uv == DETAIL_UV_2) {
			code += "		base_uv2 -= ofs;\n";
		}

		code += "	}\n";
	}

	if (flags[FLAG_USE_POINT_SIZE]) {
		code += R"(
	// Use Point Size: Enabled
	vec4 albedo_tex = texture(texture_albedo, POINT_COORD);
)";
	}
	else {
		if (flags[FLAG_UV1_USE_TRIPLANAR]) {
			code += R"(
	vec4 albedo_tex = triplanar_texture(texture_albedo, uv1_power_normal, uv1_triplanar_pos);
)";
		}
		else {
			code += R"(
	vec4 albedo_tex = texture(texture_albedo, base_uv);
)";
		}
	}

	if (flags[FLAG_ALBEDO_TEXTURE_MSDF] && !flags[FLAG_UV1_USE_TRIPLANAR]) {
		code += R"(
	{
		// Albedo Texture MSDF: Enabled
		if (!OUTPUT_IS_SRGB) {
			albedo_tex.rgb = mix(
					vec3(1.0 + 0.055) * pow(albedo_tex.rgb, vec3(1.0 / 2.4)) - vec3(0.055),
					vec3(12.92) * albedo_tex.rgb,
					lessThan(albedo_tex.rgb, vec3(0.0031308)));
		}
		vec2 msdf_size = vec2(msdf_pixel_range) / vec2(albedo_texture_size);
)";
		if (flags[FLAG_USE_POINT_SIZE]) {
			code += "		vec2 dest_size = vec2(1.0) / fwidth(POINT_COORD);\n";
		}
		else {
			code += "		vec2 dest_size = vec2(1.0) / fwidth(base_uv);\n";
		}
		code += R"(
		float px_size = max(0.5 * dot(msdf_size, dest_size), 1.0);
		float d = msdf_median(albedo_tex.r, albedo_tex.g, albedo_tex.b);
		if (msdf_outline_size > 0.0) {
			float cr = clamp(msdf_outline_size, 0.0, (msdf_pixel_range / 2.0) - 1.0) / msdf_pixel_range;
			d = min(d, albedo_tex.a);
			albedo_tex.a = clamp((d - 0.5 + cr) * px_size, 0.0, 1.0);
		}
		else {
			albedo_tex.a = clamp((d - 0.5) * px_size + 0.5, 0.0, 1.0);
		}
		albedo_tex.rgb = vec3(1.0);
	}
)";
	}
	else if (flags[FLAG_ALBEDO_TEXTURE_FORCE_SRGB]) {
		code += R"(
	// Albedo Texture Force sRGB: Enabled
	albedo_tex.rgb = mix(
			pow((albedo_tex.rgb + vec3(0.055)) * (1.0 / (1.0 + 0.055)), vec3(2.4)),
			albedo_tex.rgb.rgb * (1.0 / 12.92),
			lessThan(albedo_tex.rgb, vec3(0.04045)));
)";
	}

	if (flags[FLAG_ALBEDO_FROM_VERTEX_COLOR]) {
		code += R"(
	// Vertex Color Use as Albedo: Enabled
	albedo_tex *= COLOR;

)";
	}
	code += "	ALBEDO = albedo.rgb * albedo_tex.rgb;\n";

	if (!orm) {
		if (flags[FLAG_UV1_USE_TRIPLANAR]) {
			code += R"(
	float metallic_tex = dot(triplanar_texture(texture_metallic, uv1_power_normal, uv1_triplanar_pos), metallic_texture_channel);
)";
		}
		else {
			code += R"(
	float metallic_tex = dot(texture(texture_metallic, base_uv), metallic_texture_channel);
)";
		}

		code += R"(	METALLIC = metallic_tex * metallic;
	SPECULAR = specular;
)";
		switch (roughness_texture_channel) {
		case TEXTURE_CHANNEL_RED: {
			code += R"(
	vec4 roughness_texture_channel = vec4(1.0, 0.0, 0.0, 0.0);
)";
		} break;
		case TEXTURE_CHANNEL_GREEN: {
			code += R"(
	vec4 roughness_texture_channel = vec4(0.0, 1.0, 0.0, 0.0);
)";
		} break;
		case TEXTURE_CHANNEL_BLUE: {
			code += R"(
	vec4 roughness_texture_channel = vec4(0.0, 0.0, 1.0, 0.0);
)";
		} break;
		case TEXTURE_CHANNEL_ALPHA: {
			code += R"(
	vec4 roughness_texture_channel = vec4(0.0, 0.0, 0.0, 1.0);
)";
		} break;
		case TEXTURE_CHANNEL_GRAYSCALE: {
			code += R"(
	vec4 roughness_texture_channel = vec4(0.333333, 0.333333, 0.333333, 0.0);
)";
		} break;
		case TEXTURE_CHANNEL_MAX:
			break; // Internal value, skip.
		}

		if (flags[FLAG_UV1_USE_TRIPLANAR]) {
			code += "	float roughness_tex = dot(triplanar_texture(texture_roughness, "
					"uv1_power_normal, uv1_triplanar_pos), roughness_texture_channel);\n";
		}
		else {
			code += "	float roughness_tex = dot(texture(texture_roughness, base_uv), "
					"roughness_texture_channel);\n";
		}
		code += R"(	ROUGHNESS = roughness_tex * roughness;
)";
	}
	else {
		if (flags[FLAG_UV1_USE_TRIPLANAR]) {
			code += R"(
	vec4 orm_tex = triplanar_texture(texture_orm, uv1_power_normal, uv1_triplanar_pos);
)";
		}
		else {
			code += R"(
	vec4 orm_tex = texture(texture_orm, base_uv);
)";
		}

		code += R"(	ROUGHNESS = orm_tex.g;
 	METALLIC = orm_tex.b;
)";
	}

	if (features[FEATURE_NORMAL_MAPPING]) {
		code += R"(
	// Normal Map: Enabled
)";
		if (flags[FLAG_UV1_USE_TRIPLANAR]) {
			code += "	NORMAL_MAP = triplanar_texture(texture_normal, uv1_power_normal, "
					"uv1_triplanar_pos).rgb;\n";
		}
		else {
			code += "	NORMAL_MAP = texture(texture_normal, base_uv).rgb;\n";
		}
		code += "	NORMAL_MAP_DEPTH = normal_scale;\n";
	}

	if (features[FEATURE_BENT_NORMAL_MAPPING]) {
		code += R"(
	// Bent Normal Map: Enabled
)";
		if (flags[FLAG_UV1_USE_TRIPLANAR]) {
			code += "	BENT_NORMAL_MAP = triplanar_texture(texture_bent_normal, uv1_power_normal, "
					"uv1_triplanar_pos).rgb;\n";
		}
		else {
			code += "	BENT_NORMAL_MAP = texture(texture_bent_normal, base_uv).rgb;\n";
		}
	}

	if (features[FEATURE_EMISSION]) {
		code += R"(
	// Emission: Enabled
)";
		if (flags[FLAG_EMISSION_ON_UV2]) {
			if (flags[FLAG_UV2_USE_TRIPLANAR]) {
				code += "	vec3 emission_tex = triplanar_texture(texture_emission, "
						"uv2_power_normal, uv2_triplanar_pos).rgb;\n";
			}
			else {
				code += "	vec3 emission_tex = texture(texture_emission, base_uv2).rgb;\n";
			}
		}
		else {
			if (flags[FLAG_UV1_USE_TRIPLANAR]) {
				code += "	vec3 emission_tex = triplanar_texture(texture_emission, "
						"uv1_power_normal, uv1_triplanar_pos).rgb;\n";
			}
			else {
				code += "	vec3 emission_tex = texture(texture_emission, base_uv).rgb;\n";
			}
		}

		if (emission_op == EMISSION_OP_ADD) {
			code += R"(	// Emission Operator: Add
	EMISSION = (emission.rgb + emission_tex) * emission_energy;
)";
		}
		else {
			code += R"(	// Emission Operator: Multiply
	EMISSION = (emission.rgb * emission_tex) * emission_energy;
)";
		}
	}

	if (features[FEATURE_REFRACTION]) {
		if (features[FEATURE_NORMAL_MAPPING]) {
			code += R"(
	// Refraction: Enabled (with normal map texture)
	vec3 unpacked_normal = NORMAL_MAP;
	unpacked_normal.xy = unpacked_normal.xy * 2.0 - 1.0;
	unpacked_normal.z = sqrt(max(0.0, 1.0 - dot(unpacked_normal.xy, unpacked_normal.xy)));
	vec3 ref_normal = normalize(mix(
			NORMAL,
			TANGENT * unpacked_normal.x + BINORMAL * unpacked_normal.y + NORMAL * unpacked_normal.z,
			NORMAL_MAP_DEPTH));
)";
		}
		else {
			code += R"(
	// Refraction: Enabled
	vec3 ref_normal = NORMAL;
)";
		}
		if (flags[FLAG_UV1_USE_TRIPLANAR]) {
			code += "	vec2 ref_ofs = SCREEN_UV - ref_normal.xy * "
					"dot(triplanar_texture(texture_refraction, uv1_power_normal, "
					"uv1_triplanar_pos), refraction_texture_channel) * refraction;\n";
		}
		else {
			code += "	vec2 ref_ofs = SCREEN_UV - ref_normal.xy * dot(texture(texture_refraction, "
					"base_uv), refraction_texture_channel) * refraction;\n";
		}
		code += R"(
	float ref_amount = 1.0 - albedo.a * albedo_tex.a;

	float refraction_depth_tex = textureLod(depth_texture, ref_ofs, 0.0).r;
	vec4 ndc = OUTPUT_IS_SRGB ? vec4(vec3(SCREEN_UV, refraction_depth_tex) * 2.0 - 1.0, 1.0) : vec4(SCREEN_UV * 2.0 - 1.0, refraction_depth_tex, 1.0);
	vec4 refraction_view_pos = INV_PROJECTION_MATRIX * ndc;
	refraction_view_pos.xyz /= refraction_view_pos.w;

	// If the depth buffer is lower then the model's Z position, use the refracted UV, otherwise use the normal screen UV.
	// At low depth differences, decrease refraction intensity to avoid sudden discontinuities.
	EMISSION += textureLod(screen_texture, mix(SCREEN_UV, ref_ofs, smoothstep(0.0, 1.0, VERTEX.z - refraction_view_pos.z)), ROUGHNESS * 8.0).rgb * ref_amount * EXPOSURE;
	ALBEDO *= 1.0 - ref_amount;
	// Force transparency on the material (required for refraction).
	ALPHA = 1.0;
)";

	}
	else if (transparency != TRANSPARENCY_DISABLED || flags[FLAG_USE_SHADOW_TO_OPACITY] ||
			   (distance_fade == DISTANCE_FADE_PIXEL_ALPHA) || proximity_fade_enabled) {
		code += "	ALPHA *= albedo.a * albedo_tex.a;\n";
	}
	if (transparency == TRANSPARENCY_ALPHA_HASH) {
		code += "	ALPHA_HASH_SCALE = alpha_hash_scale;\n";
	}
	else if (transparency == TRANSPARENCY_ALPHA_SCISSOR) {
		code += "	ALPHA_SCISSOR_THRESHOLD = alpha_scissor_threshold;\n";
	}
	if (alpha_antialiasing_mode != ALPHA_ANTIALIASING_OFF &&
		(transparency == TRANSPARENCY_ALPHA_HASH || transparency == TRANSPARENCY_ALPHA_SCISSOR)) {
		code += "	ALPHA_ANTIALIASING_EDGE = alpha_antialiasing_edge;\n";
		code += "	ALPHA_TEXTURE_COORDINATE = UV * vec2(albedo_texture_size);\n";
	}

	if (proximity_fade_enabled) {
		code += R"(
	// Proximity Fade: Enabled
	float proximity_depth_tex = textureLod(depth_texture, SCREEN_UV, 0.0).r;
	vec4 ndc = OUTPUT_IS_SRGB ? vec4(vec3(SCREEN_UV, proximity_depth_tex) * 2.0 - 1.0, 1.0) : vec4(SCREEN_UV * 2.0 - 1.0, proximity_depth_tex, 1.0);
	vec4 proximity_view_pos = INV_PROJECTION_MATRIX * ndc;
	proximity_view_pos.xyz /= proximity_view_pos.w;
	ALPHA *= clamp(1.0 - smoothstep(proximity_view_pos.z + proximity_fade_distance, proximity_view_pos.z, VERTEX.z), 0.0, 1.0);
)";
	}

	if (distance_fade != DISTANCE_FADE_DISABLED) {
		// Use the slightly more expensive circular fade (distance to the object) instead of linear
		// (Z distance), so that the fade is always the same regardless of the camera angle.
		if ((distance_fade == DISTANCE_FADE_OBJECT_DITHER ||
				distance_fade == DISTANCE_FADE_PIXEL_DITHER)) {
			code += "\n	{";

			if (distance_fade == DISTANCE_FADE_OBJECT_DITHER) {
				code += R"(
		// Distance Fade: Object Dither
		float fade_distance = length((VIEW_MATRIX * MODEL_MATRIX[3]));
)";
			}
			else {
				code += R"(
		// Distance Fade: Pixel Dither
		float fade_distance = length(VERTEX);
)";
			}
			code += R"(
		// Use interleaved gradient noise, which is fast but still looks good.
		const vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
		float fade = clamp(smoothstep(distance_fade_min, distance_fade_max, fade_distance), 0.0, 1.0);
		// Use a hard cap to prevent a few stray pixels from remaining when past the fade-out distance.
		if (fade < 0.001 || fade < fract(magic.z * fract(dot(FRAGCOORD.xy, magic.xy)))) {
			discard;
		}
	}
)";
		}
		else {
			code += R"(
	// Distance Fade: Pixel Alpha
	ALPHA *= clamp(smoothstep(distance_fade_min, distance_fade_max, length(VERTEX)), 0.0, 1.0);
)";
		}
	}

	if (features[FEATURE_RIM]) {
		code += R"(
	// Rim: Enabled
)";
		if (flags[FLAG_UV1_USE_TRIPLANAR]) {
			code += "	vec2 rim_tex = triplanar_texture(texture_rim, uv1_power_normal, "
					"uv1_triplanar_pos).xy;\n";
		}
		else {
			code += "	vec2 rim_tex = texture(texture_rim, base_uv).xy;\n";
		}
		code += R"(	RIM = rim * rim_tex.x;
	RIM_TINT = rim_tint * rim_tex.y;
)";
	}

	if (features[FEATURE_CLEARCOAT]) {
		code += R"(
	// Clearcoat: Enabled
)";
		if (flags[FLAG_UV1_USE_TRIPLANAR]) {
			code += "	vec2 clearcoat_tex = triplanar_texture(texture_clearcoat, "
					"uv1_power_normal, uv1_triplanar_pos).xy;\n";
		}
		else {
			code += "	vec2 clearcoat_tex = texture(texture_clearcoat, base_uv).xy;\n";
		}
		code += R"(	CLEARCOAT = clearcoat * clearcoat_tex.x;
	CLEARCOAT_ROUGHNESS = clearcoat_roughness * clearcoat_tex.y;
)";
	}

	if (features[FEATURE_ANISOTROPY]) {
		code += R"(
	// Anisotropy: Enabled
)";
		if (flags[FLAG_UV1_USE_TRIPLANAR]) {
			code += "	vec3 anisotropy_tex = triplanar_texture(texture_flowmap, uv1_power_normal, "
					"uv1_triplanar_pos).rga;\n";
		}
		else {
			code += "	vec3 anisotropy_tex = texture(texture_flowmap, base_uv).rga;\n";
		}
		code += R"(	ANISOTROPY = anisotropy_ratio * anisotropy_tex.b;
	ANISOTROPY_FLOW = anisotropy_tex.rg * 2.0 - 1.0;
)";
	}

	if (features[FEATURE_AMBIENT_OCCLUSION]) {
		code += R"(
	// Ambient Occlusion: Enabled
)";
		if (!orm) {
			if (flags[FLAG_AO_ON_UV2]) {
				if (flags[FLAG_UV2_USE_TRIPLANAR]) {
					code += "	AO = dot(triplanar_texture(texture_ambient_occlusion, "
							"uv2_power_normal, uv2_triplanar_pos), ao_texture_channel);\n";
				}
				else {
					code += "	AO = dot(texture(texture_ambient_occlusion, base_uv2), "
							"ao_texture_channel);\n";
				}
			}
			else {
				if (flags[FLAG_UV1_USE_TRIPLANAR]) {
					code += "	AO = dot(triplanar_texture(texture_ambient_occlusion, "
							"uv1_power_normal, uv1_triplanar_pos), ao_texture_channel);\n";
				}
				else {
					code += "	AO = dot(texture(texture_ambient_occlusion, base_uv), "
							"ao_texture_channel);\n";
				}
			}
		}
		else {
			code += "	AO = orm_tex.r;\n";
		}

		code += "	AO_LIGHT_AFFECT = ao_light_affect;\n";
	}

	if (features[FEATURE_SUBSURFACE_SCATTERING]) {
		code += R"(
	// Subsurface Scattering: Enabled
)";
		if (flags[FLAG_UV1_USE_TRIPLANAR]) {
			code += "	float sss_tex = triplanar_texture(texture_subsurface_scattering, "
					"uv1_power_normal, uv1_triplanar_pos).r;\n";
		}
		else {
			code += "	float sss_tex = texture(texture_subsurface_scattering, base_uv).r;\n";
		}
		code += "	SSS_STRENGTH = subsurface_scattering_strength * sss_tex;\n";
	}

	if (features[FEATURE_SUBSURFACE_TRANSMITTANCE]) {
		code += R"(
	// Subsurface Scattering Transmittance: Enabled
)";
		if (flags[FLAG_UV1_USE_TRIPLANAR]) {
			code += "	vec4 trans_color_tex = triplanar_texture(texture_subsurface_transmittance, "
					"uv1_power_normal, uv1_triplanar_pos);\n";
		}
		else {
			code +=
				"	vec4 trans_color_tex = texture(texture_subsurface_transmittance, base_uv);\n";
		}
		code += "	SSS_TRANSMITTANCE_COLOR = transmittance_color * trans_color_tex;\n";

		code += R"(	SSS_TRANSMITTANCE_DEPTH = transmittance_depth;
	SSS_TRANSMITTANCE_BOOST = transmittance_boost;
)";
	}

	if (features[FEATURE_BACKLIGHT]) {
		code += R"(
	// Backlight: Enabled
)";
		if (flags[FLAG_UV1_USE_TRIPLANAR]) {
			code += "	vec3 backlight_tex = triplanar_texture(texture_backlight, "
					"uv1_power_normal, uv1_triplanar_pos).rgb;\n";
		}
		else {
			code += "	vec3 backlight_tex = texture(texture_backlight, base_uv).rgb;\n";
		}
		code += "	BACKLIGHT = (backlight.rgb + backlight_tex);\n";
	}

	if (features[FEATURE_DETAIL]) {
		code += R"(
	// Detail: Enabled
)";
		const bool triplanar = (flags[FLAG_UV1_USE_TRIPLANAR] && detail_uv == DETAIL_UV_1) ||
							   (flags[FLAG_UV2_USE_TRIPLANAR] && detail_uv == DETAIL_UV_2);
		if (triplanar) {
			const String tp_uv = detail_uv == DETAIL_UV_1 ? "uv1" : "uv2";
			code += vformat(
				R"(	vec4 detail_tex = triplanar_texture(texture_detail_albedo, %s_power_normal, %s_triplanar_pos);
	vec4 detail_norm_tex = triplanar_texture(texture_detail_normal, %s_power_normal, %s_triplanar_pos);
)",
				tp_uv, tp_uv, tp_uv, tp_uv);
		}
		else {
			const String det_uv = detail_uv == DETAIL_UV_1 ? "base_uv" : "base_uv2";
			code += vformat(R"(	vec4 detail_tex = texture(texture_detail_albedo, %s);
	vec4 detail_norm_tex = texture(texture_detail_normal, %s);
)",
				det_uv, det_uv);
		}

		if (flags[FLAG_UV1_USE_TRIPLANAR]) {
			code += "	vec4 detail_mask_tex = triplanar_texture(texture_detail_mask, "
					"uv1_power_normal, uv1_triplanar_pos);\n";
		}
		else {
			code += "	vec4 detail_mask_tex = texture(texture_detail_mask, base_uv);\n";
		}

		switch (detail_blend_mode) {
		case BLEND_MODE_MIX: {
			code += R"(
	// Detail Blend Mode: Mix
	vec3 detail = mix(ALBEDO.rgb, detail_tex.rgb, detail_tex.a);
)";
		} break;
		case BLEND_MODE_ADD: {
			code += R"(
	// Detail Blend Mode: Add
	vec3 detail = mix(ALBEDO.rgb, ALBEDO.rgb + detail_tex.rgb, detail_tex.a);
)";
		} break;
		case BLEND_MODE_SUB: {
			code += R"(
	// Detail Blend Mode: Subtract
	vec3 detail = mix(ALBEDO.rgb, ALBEDO.rgb - detail_tex.rgb, detail_tex.a);
)";
		} break;
		case BLEND_MODE_MUL: {
			code += R"(
	// Detail Blend Mode: Multiply
	vec3 detail = mix(ALBEDO.rgb, ALBEDO.rgb * detail_tex.rgb, detail_tex.a);
)";
		} break;
		case BLEND_MODE_PREMULT_ALPHA: {
			// This is unlikely to ever be used for detail textures, and in order for it to function
			// in the editor, another bit must be used in MaterialKey, but there are only 5 bits
			// left, so I'm going to leave this disabled unless it's actually requested.
			// code += "\tvec3 detail = (1.0-detail_tex.a)*ALBEDO.rgb+detail_tex.rgb;\n";
		} break;
		case BLEND_MODE_MAX:
			break; // Internal value, skip.
		}

		code += R"(	vec3 detail_norm = mix(NORMAL_MAP, detail_norm_tex.rgb, detail_tex.a);
	NORMAL_MAP = mix(NORMAL_MAP, detail_norm, detail_mask_tex.r);
	ALBEDO.rgb = mix(ALBEDO.rgb, detail, detail_mask_tex.r);)";
	}

	code += "}\n";

	// We must create the shader outside the shader_map_mutex to avoid potential deadlocks with
	// other tasks in the WorkerThreadPool simultaneously creating materials, which
	// may also hold the shared shader_map_mutex lock.
	RID new_shader = Renderer::shader_create_from_code(code);
	MutexLock lock(shader_map_mutex);
	ShaderData* v = shader_map.getptr(mk);
	if (unlikely(v)) {
		// We raced and managed to create the same key concurrently, so we'll free the shader we
		// just created, given we know it isn't used, and use the winner.
		Renderer::free_rid(new_shader);
	}
	else {
		ShaderData shader_data;
		shader_data.shader = new_shader;
		// ShaderData will be inserted with a users count of 0, but we
		// increment unconditionally outside this if block, whilst still under lock.
		v = &shader_map.insert(mk, shader_data)->value;
	}

	shader_rid = v->shader;
	v->users++;

	if (_get_material().is_valid()) {
		Renderer::material_set_shader(_get_material(), shader_rid);
	}
}

void StandardMaterial3D::flush_changes()
{
	SelfList<StandardMaterial3D>::List copy;
	{
		MutexLock lock(material_mutex);
		while (SelfList<StandardMaterial3D>* E = dirty_materials.first()) {
			dirty_materials.remove(E);
			copy.add(E);
		}
	}

	while (SelfList<StandardMaterial3D>* E = copy.first()) {
		E->self()->_update_shader();
		copy.remove(E);
	}
}

void StandardMaterial3D::_queue_shader_change()
{
	if (!_is_initialized()) {
		return;
	}

	MutexLock lock(material_mutex);

	if (!element.in_list()) {
		dirty_materials.add(&element);
	}
}

void StandardMaterial3D::set_albedo(const Color& p_albedo) {}

Color StandardMaterial3D::get_albedo() const { return albedo; }

float StandardMaterial3D::get_specular() const { return specular; }

float StandardMaterial3D::get_roughness() const { return roughness; }

float StandardMaterial3D::get_metallic() const { return metallic; }

Color StandardMaterial3D::get_emission() const { return emission; }

float StandardMaterial3D::get_emission_energy_multiplier() const { return emission_energy_multiplier; }

float StandardMaterial3D::get_emission_intensity() const { return emission_intensity; }

float StandardMaterial3D::get_normal_scale() const { return normal_scale; }

float StandardMaterial3D::get_rim() const { return rim; }

float StandardMaterial3D::get_rim_tint() const { return rim_tint; }

float StandardMaterial3D::get_ao_light_affect() const { return ao_light_affect; }

float StandardMaterial3D::get_clearcoat() const { return clearcoat; }

float StandardMaterial3D::get_clearcoat_roughness() const { return clearcoat_roughness; }

float StandardMaterial3D::get_anisotropy() const { return anisotropy; }

float StandardMaterial3D::get_heightmap_scale() const { return heightmap_scale; }

float StandardMaterial3D::get_subsurface_scattering_strength() const
{
	return subsurface_scattering_strength;
}

Color StandardMaterial3D::get_transmittance_color() const { return transmittance_color; }

float StandardMaterial3D::get_transmittance_depth() const { return transmittance_depth; }

float StandardMaterial3D::get_transmittance_boost() const { return transmittance_boost; }

Color StandardMaterial3D::get_backlight() const { return backlight; }

float StandardMaterial3D::get_refraction() const { return refraction; }

void StandardMaterial3D::set_detail_uv(DetailUV p_detail_uv)
{
	if (detail_uv == p_detail_uv) {
		return;
	}

	detail_uv = p_detail_uv;
	_queue_shader_change();
}

StandardMaterial3D::DetailUV StandardMaterial3D::get_detail_uv() const { return detail_uv; }

void StandardMaterial3D::set_blend_mode(BlendMode p_mode)
{
	if (blend_mode == p_mode) {
		return;
	}

	blend_mode = p_mode;
	_queue_shader_change();
}

StandardMaterial3D::BlendMode StandardMaterial3D::get_blend_mode() const { return blend_mode; }

void StandardMaterial3D::set_detail_blend_mode(BlendMode p_mode)
{
	detail_blend_mode = p_mode;
	_queue_shader_change();
}

StandardMaterial3D::BlendMode StandardMaterial3D::get_detail_blend_mode() const
{
	return detail_blend_mode;
}

void StandardMaterial3D::set_transparency(Transparency p_transparency) {}

StandardMaterial3D::Transparency StandardMaterial3D::get_transparency() const { return transparency; }

StandardMaterial3D::AlphaAntiAliasing StandardMaterial3D::get_alpha_antialiasing() const
{
	return alpha_antialiasing_mode;
}

void StandardMaterial3D::set_shading_mode(ShadingMode p_shading_mode) {}

StandardMaterial3D::ShadingMode StandardMaterial3D::get_shading_mode() const { return shading_mode; }

void StandardMaterial3D::set_depth_draw_mode(DepthDrawMode p_mode)
{
	if (depth_draw_mode == p_mode) {
		return;
	}

	depth_draw_mode = p_mode;
	_queue_shader_change();
}

StandardMaterial3D::DepthDrawMode StandardMaterial3D::get_depth_draw_mode() const
{
	return depth_draw_mode;
}

void StandardMaterial3D::set_depth_test(DepthTest p_func)
{
	if (depth_test == p_func) {
		return;
	}

	depth_test = p_func;
	_queue_shader_change();
}

StandardMaterial3D::DepthTest StandardMaterial3D::get_depth_test() const { return depth_test; }

void StandardMaterial3D::set_cull_mode(CullMode p_mode)
{
	if (cull_mode == p_mode) {
		return;
	}

	cull_mode = p_mode;
	_queue_shader_change();
}

StandardMaterial3D::CullMode StandardMaterial3D::get_cull_mode() const { return cull_mode; }

void StandardMaterial3D::set_diffuse_mode(DiffuseMode p_mode)
{
	if (diffuse_mode == p_mode) {
		return;
	}

	diffuse_mode = p_mode;
	_queue_shader_change();
}

StandardMaterial3D::DiffuseMode StandardMaterial3D::get_diffuse_mode() const { return diffuse_mode; }

void StandardMaterial3D::set_specular_mode(SpecularMode p_mode)
{
	if (specular_mode == p_mode) {
		return;
	}

	specular_mode = p_mode;
	_queue_shader_change();
}

StandardMaterial3D::SpecularMode StandardMaterial3D::get_specular_mode() const { return specular_mode; }

void StandardMaterial3D::set_flag(Flags p_flag, bool p_enabled) {}

bool StandardMaterial3D::get_flag(Flags p_flag) const
{
	ERR_FAIL_INDEX_V(p_flag, FLAG_MAX, false);
	return flags[p_flag];
}

void StandardMaterial3D::set_feature(Feature p_feature, bool p_enabled)
{
	ERR_FAIL_INDEX(p_feature, FEATURE_MAX);
	if (features[p_feature] == p_enabled) {
		return;
	}

	features[p_feature] = p_enabled;
	_queue_shader_change();
}

bool StandardMaterial3D::get_feature(Feature p_feature) const
{
	ERR_FAIL_INDEX_V(p_feature, FEATURE_MAX, false);
	return features[p_feature];
}

Ref<Texture2D> StandardMaterial3D::get_texture(TextureParam p_param) const
{
	ERR_FAIL_INDEX_V(p_param, TEXTURE_MAX, Ref<Texture2D>());
	return textures[p_param];
}

Ref<Texture2D> StandardMaterial3D::get_texture_by_name(const String& p_name) const
{
	for (int i = 0; i < (int)StandardMaterial3D::TEXTURE_MAX; i++) {
		TextureParam param = TextureParam(i);
		if (p_name == shader_names->texture_names[param]) {
			return textures[param];
		}
	}
	return Ref<Texture2D>();
}

void StandardMaterial3D::set_texture_filter(TextureFilter p_filter)
{
	texture_filter = p_filter;
	_queue_shader_change();
}

StandardMaterial3D::TextureFilter StandardMaterial3D::get_texture_filter() const { return texture_filter; }

void StandardMaterial3D::set_point_size(float p_point_size) {}

float StandardMaterial3D::get_point_size() const { return point_size; }

Vector3 StandardMaterial3D::get_uv1_scale() const { return uv1_scale; }

Vector3 StandardMaterial3D::get_uv1_offset() const { return uv1_offset; }

float StandardMaterial3D::get_uv1_triplanar_blend_sharpness() const { return uv1_triplanar_sharpness; }

Vector3 StandardMaterial3D::get_uv2_scale() const { return uv2_scale; }

Vector3 StandardMaterial3D::get_uv2_offset() const { return uv2_offset; }

float StandardMaterial3D::get_uv2_triplanar_blend_sharpness() const { return uv2_triplanar_sharpness; }

StandardMaterial3D::BillboardMode StandardMaterial3D::get_billboard_mode() const { return billboard_mode; }

int StandardMaterial3D::get_particles_anim_h_frames() const { return particles_anim_h_frames; }

int StandardMaterial3D::get_particles_anim_v_frames() const { return particles_anim_v_frames; }

bool StandardMaterial3D::get_particles_anim_loop() const { return particles_anim_loop; }

bool StandardMaterial3D::is_heightmap_deep_parallax_enabled() const { return deep_parallax; }

int StandardMaterial3D::get_heightmap_deep_parallax_min_layers() const
{
	return deep_parallax_min_layers;
}

int StandardMaterial3D::get_heightmap_deep_parallax_max_layers() const
{
	return deep_parallax_max_layers;
}

bool StandardMaterial3D::get_heightmap_deep_parallax_flip_tangent() const
{
	return heightmap_parallax_flip_tangent;
}

bool StandardMaterial3D::get_heightmap_deep_parallax_flip_binormal() const
{
	return heightmap_parallax_flip_binormal;
}

bool StandardMaterial3D::is_grow_enabled() const { return grow_enabled; }

float StandardMaterial3D::get_alpha_scissor_threshold() const { return alpha_scissor_threshold; }

float StandardMaterial3D::get_alpha_hash_scale() const { return alpha_hash_scale; }

float StandardMaterial3D::get_alpha_antialiasing_edge() const { return alpha_antialiasing_edge; }

float StandardMaterial3D::get_grow() const { return grow; }

static Vector4 _get_texture_mask(StandardMaterial3D::TextureChannel p_channel)
{
	static const Vector4 masks[5] = {
		Vector4(1, 0, 0, 0),
		Vector4(0, 1, 0, 0),
		Vector4(0, 0, 1, 0),
		Vector4(0, 0, 0, 1),
		Vector4(0.3333333, 0.3333333, 0.3333333, 0),
	};

	return masks[p_channel];
}

StandardMaterial3D::TextureChannel StandardMaterial3D::get_metallic_texture_channel() const
{
	return metallic_texture_channel;
}

void StandardMaterial3D::set_roughness_texture_channel(TextureChannel p_channel)
{
	ERR_FAIL_INDEX(p_channel, 5);
	roughness_texture_channel = p_channel;
	_queue_shader_change();
}

StandardMaterial3D::TextureChannel StandardMaterial3D::get_roughness_texture_channel() const
{
	return roughness_texture_channel;
}

StandardMaterial3D::TextureChannel StandardMaterial3D::get_ao_texture_channel() const
{
	return ao_texture_channel;
}

StandardMaterial3D::TextureChannel StandardMaterial3D::get_refraction_texture_channel() const
{
	return refraction_texture_channel;
}

float StandardMaterial3D::get_z_clip_scale() const { return z_clip_scale; }

float StandardMaterial3D::get_fov_override() const { return fov_override; }

Ref<Material> StandardMaterial3D::get_material_for_2d(bool p_shaded, Transparency p_transparency,
	bool p_double_sided, bool p_billboard, bool p_billboard_y, bool p_msdf, bool p_no_depth,
	bool p_fixed_size, TextureFilter p_filter, AlphaAntiAliasing p_alpha_antialiasing_mode,
	bool p_texture_repeat, RID* r_shader_rid)
{
	uint64_t key = 0;
	key |= ((int8_t)p_shaded & 0x01) << 0;
	key |= ((int8_t)p_transparency & 0x07) << 1; // Bits 1-3.
	key |= ((int8_t)p_double_sided & 0x01) << 4;
	key |= ((int8_t)p_billboard & 0x01) << 5;
	key |= ((int8_t)p_billboard_y & 0x01) << 6;
	key |= ((int8_t)p_msdf & 0x01) << 7;
	key |= ((int8_t)p_no_depth & 0x01) << 8;
	key |= ((int8_t)p_fixed_size & 0x01) << 9;
	key |= ((int8_t)p_filter & 0x07) << 10;					 // Bits 10-12.
	key |= ((int8_t)p_alpha_antialiasing_mode & 0x07) << 13; // Bits 13-15.
	key |= ((int8_t)p_texture_repeat & 0x01) << 16;

	if (materials_for_2d.has(key)) {
		if (r_shader_rid) {
			*r_shader_rid = materials_for_2d[key]->get_shader_rid();
		}
		return materials_for_2d[key];
	}

	Ref<StandardMaterial3D> material;
	material.instantiate();

	material->set_shading_mode(p_shaded ? SHADING_MODE_PER_PIXEL : SHADING_MODE_UNSHADED);
	material->set_transparency(p_transparency);
	material->set_cull_mode(p_double_sided ? CULL_DISABLED : CULL_BACK);
	material->set_alpha_antialiasing(p_alpha_antialiasing_mode);
	material->set_texture_filter(p_filter);
	if (p_billboard || p_billboard_y) {
		material->set_billboard_mode(p_billboard_y ? BILLBOARD_FIXED_Y : BILLBOARD_ENABLED);
	}

	materials_for_2d[key] = material;

	if (r_shader_rid) {
		*r_shader_rid = materials_for_2d[key]->get_shader_rid();
	}

	return materials_for_2d[key];
}

void StandardMaterial3D::set_on_top_of_alpha()
{
	set_transparency(TRANSPARENCY_DISABLED);
	set_render_priority(RENDER_PRIORITY_MAX);
}

bool StandardMaterial3D::is_proximity_fade_enabled() const { return proximity_fade_enabled; }

float StandardMaterial3D::get_proximity_fade_distance() const { return proximity_fade_distance; }

float StandardMaterial3D::get_msdf_pixel_range() const { return msdf_pixel_range; }

float StandardMaterial3D::get_msdf_outline_size() const { return msdf_outline_size; }

StandardMaterial3D::DistanceFadeMode StandardMaterial3D::get_distance_fade() const { return distance_fade; }

float StandardMaterial3D::get_distance_fade_max_distance() const { return distance_fade_max_distance; }

float StandardMaterial3D::get_distance_fade_min_distance() const { return distance_fade_min_distance; }

void StandardMaterial3D::set_emission_operator(EmissionOperator p_op)
{
	if (emission_op == p_op) {
		return;
	}
	emission_op = p_op;
	_queue_shader_change();
}

StandardMaterial3D::EmissionOperator StandardMaterial3D::get_emission_operator() const
{
	return emission_op;
}

RID StandardMaterial3D::get_rid() const
{
	const_cast<StandardMaterial3D*>(this)->_update_shader();
	const_cast<StandardMaterial3D*>(this)->_check_material_rid();
	return _get_material();
}

StandardMaterial3D::StencilMode StandardMaterial3D::get_stencil_mode() const { return stencil_mode; }

void StandardMaterial3D::set_stencil_flags(int p_stencil_flags)
{
	if (stencil_flags == p_stencil_flags) {
		return;
	}

	// If enabling read while already writing, switch to read only.
	if ((p_stencil_flags & STENCIL_FLAG_READ) &&
		(stencil_flags & (STENCIL_FLAG_WRITE | STENCIL_FLAG_WRITE_DEPTH_FAIL))) {
		p_stencil_flags = p_stencil_flags & STENCIL_FLAG_READ;
	}

	// If enabling write while already reading, switch to write or write_depth_fail.
	if ((p_stencil_flags & (STENCIL_FLAG_WRITE | STENCIL_FLAG_WRITE_DEPTH_FAIL)) &&
		(stencil_flags & STENCIL_FLAG_READ)) {
		p_stencil_flags = p_stencil_flags & (STENCIL_FLAG_WRITE | STENCIL_FLAG_WRITE_DEPTH_FAIL);
	}

	// If enabling read+write while already doing neither, only allow read.
	if ((p_stencil_flags & STENCIL_FLAG_READ) &&
		(p_stencil_flags & (STENCIL_FLAG_WRITE | STENCIL_FLAG_WRITE_DEPTH_FAIL))) {
		p_stencil_flags = p_stencil_flags & STENCIL_FLAG_READ;
	}

	stencil_flags = p_stencil_flags;
	_queue_shader_change();
}

int StandardMaterial3D::get_stencil_flags() const { return stencil_flags; }

void StandardMaterial3D::set_stencil_compare(StandardMaterial3D::StencilCompare p_op)
{
	if (stencil_compare == p_op) {
		return;
	}

	stencil_compare = p_op;
	_queue_shader_change();
}

StandardMaterial3D::StencilCompare StandardMaterial3D::get_stencil_compare() const
{
	return stencil_compare;
}

void StandardMaterial3D::set_stencil_reference(int p_reference)
{
	if (stencil_reference == p_reference) {
		return;
	}

	stencil_reference = p_reference;
	_queue_shader_change();

	Ref<StandardMaterial3D> stencil_next_pass = _get_stencil_next_pass();
	if (stencil_next_pass.is_valid()) {
		stencil_next_pass->set_stencil_reference(p_reference);
	}
}

int StandardMaterial3D::get_stencil_reference() const { return stencil_reference; }

void StandardMaterial3D::set_stencil_effect_color(const Color& p_color)
{
	if (stencil_effect_color == p_color) {
		return;
	}

	stencil_effect_color = p_color;

	Ref<StandardMaterial3D> stencil_next_pass = _get_stencil_next_pass();
}

Color StandardMaterial3D::get_stencil_effect_color() const { return stencil_effect_color; }

void StandardMaterial3D::set_stencil_effect_outline_thickness(float p_outline_thickness)
{
	if (stencil_effect_outline_thickness == p_outline_thickness) {
		return;
	}

	stencil_effect_outline_thickness = p_outline_thickness;

	Ref<StandardMaterial3D> stencil_next_pass = _get_stencil_next_pass();
	if (stencil_next_pass.is_valid()) {
		stencil_next_pass->set_grow(p_outline_thickness);
	}
}

float StandardMaterial3D::get_stencil_effect_outline_thickness() const
{
	return stencil_effect_outline_thickness;
}

RID StandardMaterial3D::get_shader_rid() const
{
	const_cast<StandardMaterial3D*>(this)->_update_shader();
	return shader_rid;
}

void StandardMaterial3D::set_orm_mode(bool p_orm)
{
	if (orm == p_orm) {
		return;
	}
	orm = p_orm;
	_queue_shader_change();
}

Shader::Mode StandardMaterial3D::get_shader_mode() const { return Shader::MODE_SPATIAL; }

StandardMaterial3D::StandardMaterial3D(bool p_orm) : element(this)
{
	orm = p_orm;
	// Initialize to the same values as the shader
	set_specular(0.5);
	set_roughness(1.0);
	set_metallic(0.0);
	set_emission(Color(0, 0, 0));
	set_emission_energy_multiplier(1.0);
	set_normal_scale(1);
	set_rim(1.0);
	set_rim_tint(0.5);
	set_clearcoat(1);
	set_clearcoat_roughness(0.5);
	set_anisotropy(0);
	set_heightmap_scale(5.0);
	set_subsurface_scattering_strength(0);
	set_backlight(Color(0, 0, 0));
	set_transmittance_color(Color(1, 1, 1, 1));
	set_transmittance_depth(0.1);
	set_transmittance_boost(0.0);
	set_refraction(0.05);
	set_point_size(1);
	set_uv1_offset(Vector3(0, 0, 0));
	set_uv1_scale(Vector3(1, 1, 1));
	set_uv1_triplanar_blend_sharpness(1);
	set_uv2_offset(Vector3(0, 0, 0));
	set_uv2_scale(Vector3(1, 1, 1));
	set_uv2_triplanar_blend_sharpness(1);
	set_billboard_mode(BILLBOARD_DISABLED);
	set_particles_anim_h_frames(1);
	set_particles_anim_v_frames(1);
	set_particles_anim_loop(false);

	set_transparency(TRANSPARENCY_DISABLED);
	set_alpha_antialiasing(ALPHA_ANTIALIASING_OFF);
	// Alpha scissor threshold of 0.5 matches the glTF specification and Label3D default.
	// <https://www.khronos.org/registry/glTF/specs/2.0/glTF-2.0.html#_material_alphacutoff>
	set_alpha_scissor_threshold(0.5);
	set_alpha_hash_scale(1.0);
	set_alpha_antialiasing_edge(0.3);

	set_proximity_fade_distance(1);
	set_distance_fade_min_distance(0);
	set_distance_fade_max_distance(10);

	set_ao_light_affect(0.0);

	set_metallic_texture_channel(TEXTURE_CHANNEL_RED);
	set_roughness_texture_channel(TEXTURE_CHANNEL_RED);
	set_ao_texture_channel(TEXTURE_CHANNEL_RED);
	set_refraction_texture_channel(TEXTURE_CHANNEL_RED);

	set_grow(0.0);

	set_msdf_pixel_range(4.0);
	set_msdf_outline_size(0.0);

	set_heightmap_deep_parallax_min_layers(8);
	set_heightmap_deep_parallax_max_layers(32);
	set_heightmap_deep_parallax_flip_tangent(false); // also sets binormal

	set_z_clip_scale(1.0);
	set_fov_override(75.0);

	set_stencil_mode(STENCIL_MODE_DISABLED);

	flags[FLAG_ALBEDO_TEXTURE_MSDF] = false;
	flags[FLAG_USE_TEXTURE_REPEAT] = true;

	current_key.invalid_key = 1;
}

StandardMaterial3D::~StandardMaterial3D()
{
	ERR_FAIL_NULL(Renderer::data);

	{
		MutexLock lock(shader_map_mutex);
		if (shader_map.has(current_key)) {
			shader_map[current_key].users--;
			if (shader_map[current_key].users == 0) {
				// Deallocate shader which is no longer in use.
				Renderer::free_rid(shader_map[current_key].shader);
				shader_map.erase(current_key);
			}
		}
	}

	if (_get_material().is_valid()) {
		Renderer::material_set_shader(_get_material(), RID());
	}
}

#ifndef DISABLE_DEPRECATED

bool Material::_can_do_next_pass() const { return false; }

bool Material::_can_use_render_priority() const { return false; }

RID Material::get_shader_rid() const { return RID(); }

#endif // DISABLE_DEPRECATED

void ShaderMaterial::set_shader(const Ref<Shader>& p_shader) {}

void StandardMaterial3D::set_texture(TextureParam p_param, const Ref<Texture2D>& p_texture) {}

void StandardMaterial3D::set_roughness(float) {}

void StandardMaterial3D::set_metallic(float) {}

void StandardMaterial3D::set_emission(Color const&) {}

void StandardMaterial3D::set_billboard_mode(StandardMaterial3D::BillboardMode) {}

Ref<StandardMaterial3D> StandardMaterial3D::_get_stencil_next_pass() const
{
	Ref<StandardMaterial3D> bm = memnew(StandardMaterial3D);
	return bm;
}

void StandardMaterial3D::set_specular(float) {}

void StandardMaterial3D::set_metallic_texture_channel(StandardMaterial3D::TextureChannel) {}

void StandardMaterial3D::set_grow(float) {}

void StandardMaterial3D::set_alpha_antialiasing(StandardMaterial3D::AlphaAntiAliasing) {}

void ShaderMaterial::_check_material_rid() const {}

void StandardMaterial3D::set_z_clip_scale(float) {}

void StandardMaterial3D::set_uv2_triplanar_blend_sharpness(float) {}

void StandardMaterial3D::set_uv2_scale(Vector3 const&) {}

void StandardMaterial3D::set_uv2_offset(Vector3 const&) {}


void StandardMaterial3D::set_uv1_triplanar_blend_sharpness(float) {}

void StandardMaterial3D::set_uv1_scale(Vector3 const&) {}

void StandardMaterial3D::set_uv1_offset(Vector3 const&) {}

void StandardMaterial3D::set_transmittance_depth(float) {}

void StandardMaterial3D::set_transmittance_color(Color const&) {}

void StandardMaterial3D::set_transmittance_boost(float) {}

void StandardMaterial3D::set_subsurface_scattering_strength(float) {}

void StandardMaterial3D::set_stencil_mode(StandardMaterial3D::StencilMode) {}

void StandardMaterial3D::set_rim_tint(float) {}

void StandardMaterial3D::set_rim(float) {}

void StandardMaterial3D::set_refraction_texture_channel(StandardMaterial3D::TextureChannel) {}

void StandardMaterial3D::set_refraction(float) {}

void StandardMaterial3D::set_proximity_fade_distance(float) {}

void StandardMaterial3D::set_particles_anim_v_frames(int) {}

void StandardMaterial3D::set_particles_anim_loop(bool) {}

void StandardMaterial3D::set_particles_anim_h_frames(int) {}

void StandardMaterial3D::set_normal_scale(float) {}

void StandardMaterial3D::set_msdf_pixel_range(float) {}

void StandardMaterial3D::set_msdf_outline_size(float) {}

void StandardMaterial3D::set_heightmap_scale(float) {}

void StandardMaterial3D::set_heightmap_deep_parallax_min_layers(int) {}

void StandardMaterial3D::set_heightmap_deep_parallax_max_layers(int) {}

void StandardMaterial3D::set_heightmap_deep_parallax_flip_tangent(bool) {}

void StandardMaterial3D::set_fov_override(float) {}

void StandardMaterial3D::set_emission_energy_multiplier(float) {}

void StandardMaterial3D::set_distance_fade_min_distance(float) {}

void StandardMaterial3D::set_distance_fade_max_distance(float) {}

void StandardMaterial3D::set_clearcoat_roughness(float) {}

void StandardMaterial3D::set_clearcoat(float) {}

void StandardMaterial3D::set_backlight(Color const&) {}

void StandardMaterial3D::set_ao_texture_channel(StandardMaterial3D::TextureChannel) {}

void StandardMaterial3D::set_ao_light_affect(float) {}

void StandardMaterial3D::set_anisotropy(float) {}

void StandardMaterial3D::set_alpha_scissor_threshold(float) {}

void StandardMaterial3D::set_alpha_hash_scale(float) {}

void StandardMaterial3D::set_alpha_antialiasing_edge(float) {}

void StandardMaterial3D::_check_material_rid() {}

// --- canvas_item_material.cpp ---

#include "scene/resources/material.h"
#include "core/version.h"
#include "servers/rendering/renderer.h"

Mutex CanvasItemMaterial::material_mutex;
SelfList<CanvasItemMaterial>::List CanvasItemMaterial::dirty_materials;
HashMap<CanvasItemMaterial::MaterialKey, CanvasItemMaterial::ShaderData,
	CanvasItemMaterial::MaterialKey>
	CanvasItemMaterial::shader_map;
CanvasItemMaterial::ShaderNames* CanvasItemMaterial::shader_names = nullptr;
Mutex CanvasItemMaterial::blit_shader_mutex;
RID CanvasItemMaterial::blit_shader_cache[6];

void CanvasItemMaterial::init_shaders()
{
	shader_names = memnew(ShaderNames);

	shader_names->particles_anim_h_frames = "particles_anim_h_frames";
	shader_names->particles_anim_v_frames = "particles_anim_v_frames";
	shader_names->particles_anim_loop = "particles_anim_loop";
}

void CanvasItemMaterial::cleanup_blit_shaders()
{
	for (int i = 0; i < 6; i++) {
		if (blit_shader_cache[i].is_valid()) {
			Renderer::free_rid(blit_shader_cache[i]);
			blit_shader_cache[i] = RID();
		}
	}
}

void CanvasItemMaterial::finish_shaders()
{
	cleanup_blit_shaders();

	dirty_materials.clear();

	memdelete(shader_names);
	shader_names = nullptr;
}

void CanvasItemMaterial::_update_blit_shader(BlendMode p_blend)
{
	ERR_FAIL_INDEX(int(p_blend), 6);
	MutexLock shader_lock(blit_shader_mutex);
	int index = int(p_blend);
	if (blit_shader_cache[index].is_null()) {
		blit_shader_cache[index] = Renderer::shader_create();
		String code = "// NOTE: Shader automatically converted from " VLTR_VERSION_NAME
					  " " VLTR_VERSION_FULL_CONFIG "'s CanvasItemMaterial (TextureBlit).\n\n";

		code += "shader_type texture_blit;\nrender_mode ";
		switch (p_blend) {
		case BLEND_MODE_MIX:
			code += "blend_mix";
			break;
		case BLEND_MODE_ADD:
			code += "blend_add";
			break;
		case BLEND_MODE_SUB:
			code += "blend_sub";
			break;
		case BLEND_MODE_MUL:
			code += "blend_mul";
			break;
		case BLEND_MODE_PREMULT_ALPHA:
			// texture_blit shaders support mix, add, sub, mul, disabled.
			code += "blend_mix";
			break;
		case BLEND_MODE_DISABLED:
			code += "blend_disabled";
			break;
		default:
			code += "blend_mix";
			break;
		}
		code += ";\n\n";

		code += "uniform sampler2D source_texture0 : hint_blit_source0;\n";
		code += "uniform sampler2D source_texture1 : hint_blit_source1;\n";
		code += "uniform sampler2D source_texture2 : hint_blit_source2;\n";
		code += "uniform sampler2D source_texture3 : hint_blit_source3;\n\n";

		code += "void blit() {\n";
		code += "	// Copies from each whole source texture to a rect on each output texture.\n";
		code += "	COLOR0 = texture(source_texture0, UV) * MODULATE;\n";
		code += "	COLOR1 = texture(source_texture1, UV) * MODULATE;\n";
		code += "	COLOR2 = texture(source_texture2, UV) * MODULATE;\n";
		code += "	COLOR3 = texture(source_texture3, UV) * MODULATE;\n}";
		Renderer::shader_set_code(blit_shader_cache[index], code);
	}
}

void CanvasItemMaterial::_update_shader()
{
	if (texture_blit) {
		return;
	}

	MaterialKey mk = _compute_key();
	if (mk.key == current_key.key) {
		return; // no update required in the end
	}

	if (shader_map.has(current_key)) {
		shader_map[current_key].users--;
		if (shader_map[current_key].users == 0) {
			// deallocate shader, as it's no longer in use
			Renderer::free_rid(shader_map[current_key].shader);
			shader_map.erase(current_key);
		}
	}

	current_key = mk;

	if (shader_map.has(mk)) {
		Renderer::material_set_shader(_get_material(), shader_map[mk].shader);
		shader_map[mk].users++;
		return;
	}

	// must create a shader!

	// Add a comment to describe the shader origin (useful when converting to ShaderMaterial).
	String code = "// NOTE: Shader automatically converted from " VLTR_VERSION_NAME
				  " " VLTR_VERSION_FULL_CONFIG "'s CanvasItemMaterial.\n\n";

	code += "shader_type canvas_item;\nrender_mode ";
	switch (blend_mode) {
	case BLEND_MODE_MIX:
		code += "blend_mix";
		break;
	case BLEND_MODE_ADD:
		code += "blend_add";
		break;
	case BLEND_MODE_SUB:
		code += "blend_sub";
		break;
	case BLEND_MODE_MUL:
		code += "blend_mul";
		break;
	case BLEND_MODE_PREMULT_ALPHA:
		code += "blend_premul_alpha";
		break;
	case BLEND_MODE_DISABLED:
		code += "blend_disabled";
		break;
	}

	switch (light_mode) {
	case LIGHT_MODE_NORMAL:
		break;
	case LIGHT_MODE_UNSHADED:
		code += ",unshaded";
		break;
	case LIGHT_MODE_LIGHT_ONLY:
		code += ",light_only";
		break;
	}

	code += ";\n";

	if (particles_animation) {
		code += "uniform int particles_anim_h_frames;\n";
		code += "uniform int particles_anim_v_frames;\n";
		code += "uniform bool particles_anim_loop;\n\n";

		code += "void vertex() {\n";
		code += "	float h_frames = float(particles_anim_h_frames);\n";
		code += "	float v_frames = float(particles_anim_v_frames);\n";
		code += "	VERTEX.xy /= vec2(h_frames, v_frames);\n";
		code += "	float particle_total_frames = float(particles_anim_h_frames * "
				"particles_anim_v_frames);\n";
		code +=
			"	float particle_frame = floor(INSTANCE_CUSTOM.z * float(particle_total_frames));\n";
		code += "	if (!particles_anim_loop) {\n";
		code +=
			"		particle_frame = clamp(particle_frame, 0.0, particle_total_frames - 1.0);\n";
		code += "	} else {\n";
		code += "		particle_frame = mod(particle_frame, particle_total_frames);\n";
		code += "	}";
		code += "	UV /= vec2(h_frames, v_frames);\n";
		code += "	UV += vec2(mod(particle_frame, h_frames) / h_frames, floor((particle_frame + "
				"0.5) / h_frames) / v_frames);\n";
		code += "}\n";
	}

	ShaderData shader_data;
	shader_data.shader = Renderer::shader_create();
	shader_data.users = 1;

	Renderer::shader_set_code(shader_data.shader, code);

	shader_map[mk] = shader_data;

	Renderer::material_set_shader(_get_material(), shader_data.shader);
}

void CanvasItemMaterial::flush_changes()
{
	MutexLock lock(material_mutex);

	while (dirty_materials.first()) {
		dirty_materials.first()->self()->_update_shader();
		dirty_materials.first()->remove_from_list();
	}
}

void CanvasItemMaterial::_queue_shader_change()
{
	if (texture_blit || !_is_initialized()) {
		return;
	}

	MutexLock lock(material_mutex);

	if (!element.in_list()) {
		dirty_materials.add(&element);
	}
}

void CanvasItemMaterial::set_texture_blit(bool p_enable)
{
	if (texture_blit == p_enable) {
		return;
	}

	if (!texture_blit) {
		MutexLock lock(material_mutex);
		if (element.in_list()) {
			element.remove_from_list();
		}
		if (shader_map.has(current_key)) {
			shader_map[current_key].users--;
			if (shader_map[current_key].users == 0) {
				Renderer::free_rid(shader_map[current_key].shader);
				shader_map.erase(current_key);
			}
		}
		current_key.key = 0;
		current_key.invalid_key = 1;
	}

	texture_blit = p_enable;
	blit_shader_set = false;

	if (texture_blit) {
		_update_blit_shader(blend_mode);
		Renderer::material_set_shader(_get_material(), blit_shader_cache[int(blend_mode)]);
		blit_shader_set = true;
	}
	else {
		Renderer::material_set_shader(_get_material(), RID());
		_queue_shader_change();
	}
}

void CanvasItemMaterial::set_blend_mode(BlendMode p_blend_mode)
{
	ERR_FAIL_INDEX(int(p_blend_mode), 6);
	blend_mode = p_blend_mode;
	if (texture_blit) {
		_update_blit_shader(blend_mode);
		if (blit_shader_set) {
			Renderer::material_set_shader(_get_material(), blit_shader_cache[int(blend_mode)]);
		}
	}
	else {
		_queue_shader_change();
	}
}

CanvasItemMaterial::BlendMode CanvasItemMaterial::get_blend_mode() const { return blend_mode; }

void CanvasItemMaterial::set_light_mode(LightMode p_light_mode)
{
	light_mode = p_light_mode;
	_queue_shader_change();
}

CanvasItemMaterial::LightMode CanvasItemMaterial::get_light_mode() const { return light_mode; }

void CanvasItemMaterial::set_particles_animation(bool p_particles_anim)
{
	particles_animation = p_particles_anim;
	_queue_shader_change();
}

bool CanvasItemMaterial::get_particles_animation() const { return particles_animation; }

void CanvasItemMaterial::set_particles_anim_h_frames(int p_frames)
{
	particles_anim_h_frames = p_frames;
}

int CanvasItemMaterial::get_particles_anim_h_frames() const { return particles_anim_h_frames; }

void CanvasItemMaterial::set_particles_anim_v_frames(int p_frames)
{
	particles_anim_v_frames = p_frames;
}

int CanvasItemMaterial::get_particles_anim_v_frames() const { return particles_anim_v_frames; }

void CanvasItemMaterial::set_particles_anim_loop(bool p_loop) { particles_anim_loop = p_loop; }

bool CanvasItemMaterial::get_particles_anim_loop() const { return particles_anim_loop; }

RID CanvasItemMaterial::get_rid() const
{
	if (texture_blit) {
		_update_blit_shader(blend_mode);
		if (!blit_shader_set) {
			Renderer::material_set_shader(_get_material(), blit_shader_cache[int(blend_mode)]);
			blit_shader_set = true;
		}
	}
	return _get_material();
}

RID CanvasItemMaterial::get_shader_rid() const
{
	if (texture_blit) {
		_update_blit_shader(blend_mode);
		return blit_shader_cache[int(blend_mode)];
	}
	ERR_FAIL_COND_V(!shader_map.has(current_key), RID());
	return shader_map[current_key].shader;
}

Shader::Mode CanvasItemMaterial::get_shader_mode() const
{
	return texture_blit ? Shader::MODE_TEXTURE_BLIT : Shader::MODE_CANVAS_ITEM;
}

CanvasItemMaterial::CanvasItemMaterial(bool p_texture_blit)
	: element(this), texture_blit(p_texture_blit)
{
	_set_material(Renderer::material_create());
	current_key.invalid_key = 1;
	set_particles_anim_h_frames(1);
	set_particles_anim_v_frames(1);
	set_particles_anim_loop(false);
	_mark_ready();
	if (texture_blit) {
		_update_blit_shader(blend_mode);
	}
	else {
		_queue_shader_change();
	}
}

CanvasItemMaterial::~CanvasItemMaterial()
{
	MutexLock lock(material_mutex);

	ERR_FAIL_NULL(Renderer::data);

	if (!texture_blit && shader_map.has(current_key)) {
		shader_map[current_key].users--;
		if (shader_map[current_key].users == 0) {
			// deallocate shader, as it's no longer in use
			Renderer::free_rid(shader_map[current_key].shader);
			shader_map.erase(current_key);
		}

		Renderer::material_set_shader(_get_material(), RID());
	}
}

// --- particle_process_material.cpp ---

#include "core/config/engine.h"
#include "core/version.h"
#include "scene/resources/material.h"
#include "servers/rendering/renderer.h"

Mutex ParticleProcessMaterial::dirty_materials_mutex;
SelfList<ParticleProcessMaterial>::List ParticleProcessMaterial::dirty_materials;
Mutex ParticleProcessMaterial::shader_map_mutex;
HashMap<ParticleProcessMaterial::MaterialKey, ParticleProcessMaterial::ShaderData,
	ParticleProcessMaterial::MaterialKey>
	ParticleProcessMaterial::shader_map;
RBSet<String> ParticleProcessMaterial::min_max_properties;
ParticleProcessMaterial::ShaderNames* ParticleProcessMaterial::shader_names = nullptr;

void ParticleProcessMaterial::init_shaders()
{
	shader_names = memnew(ShaderNames);

	shader_names->direction = "direction";
	shader_names->spread = "spread";
	shader_names->flatness = "flatness";
	shader_names->initial_linear_velocity_min = "initial_linear_velocity_min";
	shader_names->initial_angle_min = "initial_angle_min";
	shader_names->angular_velocity_min = "angular_velocity_min";
	shader_names->orbit_velocity_min = "orbit_velocity_min";
	shader_names->radial_velocity_min = "radial_velocity_min";
	shader_names->linear_accel_min = "linear_accel_min";
	shader_names->radial_accel_min = "radial_accel_min";
	shader_names->tangent_accel_min = "tangent_accel_min";
	shader_names->damping_min = "damping_min";
	shader_names->scale_min = "scale_min";
	shader_names->hue_variation_min = "hue_variation_min";
	shader_names->anim_speed_min = "anim_speed_min";
	shader_names->anim_offset_min = "anim_offset_min";
	shader_names->directional_velocity_min = "directional_velocity_min";
	shader_names->scale_over_velocity_min = "scale_over_velocity_min";
	shader_names->scale_3d_min = "scale_3d_min";
	shader_names->rotation_3d_min = "rotation_3d_min";

	shader_names->initial_linear_velocity_max = "initial_linear_velocity_max";
	shader_names->initial_angle_max = "initial_angle_max";
	shader_names->angular_velocity_max = "angular_velocity_max";
	shader_names->orbit_velocity_max = "orbit_velocity_max";
	shader_names->radial_velocity_max = "radial_velocity_max";
	shader_names->linear_accel_max = "linear_accel_max";
	shader_names->radial_accel_max = "radial_accel_max";
	shader_names->tangent_accel_max = "tangent_accel_max";
	shader_names->damping_max = "damping_max";
	shader_names->scale_max = "scale_max";
	shader_names->hue_variation_max = "hue_variation_max";
	shader_names->anim_speed_max = "anim_speed_max";
	shader_names->anim_offset_max = "anim_offset_max";
	shader_names->directional_velocity_max = "directional_velocity_max";
	shader_names->scale_over_velocity_max = "scale_over_velocity_max";
	shader_names->scale_3d_max = "scale_3d_max";
	shader_names->rotation_3d_max = "rotation_3d_max";

	shader_names->angle_texture = "angle_texture";
	shader_names->angular_velocity_texture = "angular_velocity_texture";
	shader_names->orbit_velocity_texture = "orbit_velocity_curve";
	shader_names->radial_velocity_texture = "radial_velocity_curve";
	shader_names->linear_accel_texture = "linear_accel_texture";
	shader_names->radial_accel_texture = "radial_accel_texture";
	shader_names->tangent_accel_texture = "tangent_accel_texture";
	shader_names->damping_texture = "damping_texture";
	shader_names->scale_texture = "scale_curve";
	shader_names->hue_variation_texture = "hue_rot_curve";
	shader_names->anim_speed_texture = "animation_speed_curve";
	shader_names->anim_offset_texture = "animation_offset_curve";
	shader_names->directional_velocity_texture = "directional_velocity_curve";
	shader_names->scale_over_velocity_texture = "scale_over_velocity_curve";

	shader_names->color = "color_value";
	shader_names->color_ramp = "color_ramp";
	shader_names->alpha_ramp = "alpha_curve";
	shader_names->emission_ramp = "emission_curve";
	shader_names->color_initial_ramp = "color_initial_ramp";
	shader_names->velocity_limit_curve = "velocity_limit_curve";
	shader_names->inherit_emitter_velocity_ratio = "inherit_emitter_velocity_ratio";
	shader_names->velocity_pivot = "velocity_pivot";
	shader_names->rotation_velocity_3d_max = "rotation_velocity_3d_max";
	shader_names->rotation_velocity_3d_min = "rotation_velocity_3d_min";
	shader_names->rotation_velocity_3d_curve = "rotation_velocity_3d_curve";

	shader_names->emission_sphere_radius = "emission_sphere_radius";
	shader_names->emission_box_extents = "emission_box_extents";
	shader_names->emission_texture_point_count = "emission_texture_point_count";
	shader_names->emission_texture_points = "emission_texture_points";
	shader_names->emission_texture_normal = "emission_texture_normal";
	shader_names->emission_texture_color = "emission_texture_color";
	shader_names->emission_ring_axis = "emission_ring_axis";
	shader_names->emission_ring_height = "emission_ring_height";
	shader_names->emission_ring_radius = "emission_ring_radius";
	shader_names->emission_ring_inner_radius = "emission_ring_inner_radius";
	shader_names->emission_ring_cone_angle = "emission_ring_cone_angle";
	shader_names->emission_shape_offset = "emission_shape_offset";
	shader_names->emission_shape_scale = "emission_shape_scale";

	shader_names->turbulence_enabled = "turbulence_enabled";
	shader_names->turbulence_noise_strength = "turbulence_noise_strength";
	shader_names->turbulence_noise_scale = "turbulence_noise_scale";
	shader_names->turbulence_noise_speed = "turbulence_noise_speed";
	shader_names->turbulence_noise_speed_random = "turbulence_noise_speed_random";
	shader_names->turbulence_influence_over_life = "turbulence_influence_over_life";
	shader_names->turbulence_influence_min = "turbulence_influence_min";
	shader_names->turbulence_influence_max = "turbulence_influence_max";
	shader_names->turbulence_initial_displacement_min = "turbulence_initial_displacement_min";
	shader_names->turbulence_initial_displacement_max = "turbulence_initial_displacement_max";

	shader_names->gravity = "gravity";

	shader_names->lifetime_randomness = "lifetime_randomness";

	shader_names->sub_emitter_frequency = "sub_emitter_frequency";
	shader_names->sub_emitter_amount_at_end = "sub_emitter_amount_at_end";
	shader_names->sub_emitter_amount_at_collision = "sub_emitter_amount_at_collision";
	shader_names->sub_emitter_amount_at_start = "sub_emitter_amount_at_start";
	shader_names->sub_emitter_keep_velocity = "sub_emitter_keep_velocity";

	shader_names->collision_friction = "collision_friction";
	shader_names->collision_bounce = "collision_bounce";
}

void ParticleProcessMaterial::finish_shaders()
{
	dirty_materials.clear();

	memdelete(shader_names);
	shader_names = nullptr;
}

void ParticleProcessMaterial::flush_changes()
{
	MutexLock lock(dirty_materials_mutex);

	while (dirty_materials.first()) {
		dirty_materials.first()->self()->_update_shader();
		dirty_materials.first()->remove_from_list();
	}
}

void ParticleProcessMaterial::_queue_shader_change()
{
	if (!_is_initialized()) {
		return;
	}

	MutexLock lock(dirty_materials_mutex);

	if (!element.in_list()) {
		dirty_materials.add(&element);
	}
}

bool ParticleProcessMaterial::has_min_max_property(const String& p_name)
{
	return min_max_properties.has(p_name);
}

Vector3 ParticleProcessMaterial::get_direction() const { return direction; }

float ParticleProcessMaterial::get_spread() const { return spread; }

float ParticleProcessMaterial::get_flatness() const { return flatness; }

Vector3 ParticleProcessMaterial::get_velocity_pivot() { return velocity_pivot; }

void ParticleProcessMaterial::set_param(Parameter p_param, const Vector2& p_value)
{
	set_param_min(p_param, p_value.x);
	set_param_max(p_param, p_value.y);
}

Vector2 ParticleProcessMaterial::get_param(Parameter p_param) const
{
	return Vector2(get_param_min(p_param), get_param_max(p_param));
}

float ParticleProcessMaterial::get_param_min(Parameter p_param) const
{
	ERR_FAIL_INDEX_V(p_param, PARAM_MAX, 0);

	return params_min[p_param];
}

float ParticleProcessMaterial::get_param_max(Parameter p_param) const
{
	ERR_FAIL_INDEX_V(p_param, PARAM_MAX, 0);

	return params_max[p_param];
}

static void _adjust_curve_range(const Ref<Texture2D>& p_texture, float p_min, float p_max)
{
	Ref<CurveTexture> curve_tex = p_texture;
	if (curve_tex.is_valid()) {
		curve_tex->ensure_default_setup(p_min, p_max);
	}
}

Ref<Texture2D> ParticleProcessMaterial::get_param_texture(Parameter p_param) const
{
	ERR_FAIL_INDEX_V(p_param, PARAM_MAX, Ref<Texture2D>());

	return tex_parameters[p_param];
}

Color ParticleProcessMaterial::get_color() const { return color; }

Ref<Texture2D> ParticleProcessMaterial::get_color_ramp() const { return color_ramp; }

Ref<Texture2D> ParticleProcessMaterial::get_color_initial_ramp() const
{
	return color_initial_ramp;
}

Ref<Texture2D> ParticleProcessMaterial::get_alpha_curve() const { return alpha_curve; }

Ref<Texture2D> ParticleProcessMaterial::get_emission_curve() const { return emission_curve; }

Ref<Texture2D> ParticleProcessMaterial::get_velocity_limit_curve() const
{
	return velocity_limit_curve;
}

bool ParticleProcessMaterial::get_particle_flag(ParticleFlags p_particle_flag) const
{
	ERR_FAIL_INDEX_V(p_particle_flag, PARTICLE_FLAG_MAX, false);
	return particle_flags[p_particle_flag];
}

ParticleProcessMaterial::EmissionShape ParticleProcessMaterial::get_emission_shape() const
{
	return emission_shape;
}

real_t ParticleProcessMaterial::get_emission_sphere_radius() const
{
	return emission_sphere_radius;
}

Vector3 ParticleProcessMaterial::get_emission_box_extents() const { return emission_box_extents; }

Ref<Texture2D> ParticleProcessMaterial::get_emission_point_texture() const
{
	return emission_point_texture;
}

Ref<Texture2D> ParticleProcessMaterial::get_emission_normal_texture() const
{
	return emission_normal_texture;
}

Ref<Texture2D> ParticleProcessMaterial::get_emission_color_texture() const
{
	return emission_color_texture;
}

int ParticleProcessMaterial::get_emission_point_count() const { return emission_point_count; }

Vector3 ParticleProcessMaterial::get_emission_ring_axis() const { return emission_ring_axis; }

real_t ParticleProcessMaterial::get_emission_ring_height() const { return emission_ring_height; }

real_t ParticleProcessMaterial::get_emission_ring_radius() const { return emission_ring_radius; }

real_t ParticleProcessMaterial::get_emission_ring_inner_radius() const
{
	return emission_ring_inner_radius;
}

real_t ParticleProcessMaterial::get_emission_ring_cone_angle() const
{
	return emission_ring_cone_angle;
}

Vector3 ParticleProcessMaterial::get_emission_shape_offset() const { return emission_shape_offset; }

Vector3 ParticleProcessMaterial::get_emission_shape_scale() const { return emission_shape_scale; }

double ParticleProcessMaterial::get_inherit_velocity_ratio()
{
	return inherit_emitter_velocity_ratio;
}

bool ParticleProcessMaterial::is_using_scale_3d() const { return use_scale_3d; }

Vector3 ParticleProcessMaterial::get_scale_3d_min() const { return scale_3d_min; }

Vector3 ParticleProcessMaterial::get_scale_3d_max() const { return scale_3d_max; }

bool ParticleProcessMaterial::is_using_rotation_3d() const { return use_rotation_3d; }

Vector3 ParticleProcessMaterial::get_rotation_3d_min() const { return rotation_3d_min; }

Vector3 ParticleProcessMaterial::get_rotation_3d_max() const { return rotation_3d_max; }

bool ParticleProcessMaterial::is_using_rotation_velocity_3d() const
{
	return using_rotation_velocity_3d;
}

Vector3 ParticleProcessMaterial::get_rotation_velocity_3d_min() const
{
	return rotation_velocity_3d_min;
}

Vector3 ParticleProcessMaterial::get_rotation_velocity_3d_max() const
{
	return rotation_velocity_3d_max;
}

Ref<Texture2D> ParticleProcessMaterial::get_rotation_velocity_3d_curve() const
{
	return rotation_velocity_3d_curve;
}

bool ParticleProcessMaterial::get_turbulence_enabled() const { return turbulence_enabled; }

float ParticleProcessMaterial::get_turbulence_noise_strength() const
{
	return turbulence_noise_strength;
}

float ParticleProcessMaterial::get_turbulence_noise_scale() const { return turbulence_noise_scale; }

float ParticleProcessMaterial::get_turbulence_noise_speed_random() const
{
	return turbulence_noise_speed_random;
}

Vector3 ParticleProcessMaterial::get_turbulence_noise_speed() const
{
	return turbulence_noise_speed;
}

Vector3 ParticleProcessMaterial::get_gravity() const { return gravity; }

double ParticleProcessMaterial::get_lifetime_randomness() const { return lifetime_randomness; }

RID ParticleProcessMaterial::get_rid() const
{
	const_cast<ParticleProcessMaterial*>(this)->_update_shader();
	return Material::get_rid();
}

RID ParticleProcessMaterial::get_shader_rid() const
{
	const_cast<ParticleProcessMaterial*>(this)->_update_shader();
	return shader_rid;
}

ParticleProcessMaterial::SubEmitterMode ParticleProcessMaterial::get_sub_emitter_mode() const
{
	return sub_emitter_mode;
}

double ParticleProcessMaterial::get_sub_emitter_frequency() const { return sub_emitter_frequency; }

int ParticleProcessMaterial::get_sub_emitter_amount_at_end() const
{
	return sub_emitter_amount_at_end;
}

int ParticleProcessMaterial::get_sub_emitter_amount_at_collision() const
{
	return sub_emitter_amount_at_collision;
}

int ParticleProcessMaterial::get_sub_emitter_amount_at_start() const
{
	return sub_emitter_amount_at_start;
}

bool ParticleProcessMaterial::get_sub_emitter_keep_velocity() const
{
	return sub_emitter_keep_velocity;
}

void ParticleProcessMaterial::set_attractor_interaction_enabled(bool p_enable)
{
	attractor_interaction_enabled = p_enable;
	_queue_shader_change();
}

bool ParticleProcessMaterial::is_attractor_interaction_enabled() const
{
	return attractor_interaction_enabled;
}

ParticleProcessMaterial::CollisionMode ParticleProcessMaterial::get_collision_mode() const
{
	return collision_mode;
}

void ParticleProcessMaterial::set_collision_use_scale(bool p_scale)
{
	collision_scale = p_scale;
	_queue_shader_change();
}

bool ParticleProcessMaterial::is_collision_using_scale() const { return collision_scale; }

float ParticleProcessMaterial::get_collision_friction() const { return collision_friction; }

float ParticleProcessMaterial::get_collision_bounce() const { return collision_bounce; }

Shader::Mode ParticleProcessMaterial::get_shader_mode() const { return Shader::MODE_PARTICLES; }

ParticleProcessMaterial::ParticleProcessMaterial() : element(this)
{
	_set_material(Renderer::material_create());

	set_direction(Vector3(1, 0, 0));
	set_spread(45);
	set_flatness(0);
	set_param_min(PARAM_INITIAL_LINEAR_VELOCITY, 0);
	set_param_min(PARAM_ANGULAR_VELOCITY, 0);
	set_param_min(PARAM_ORBIT_VELOCITY, 0);
	set_param_min(PARAM_LINEAR_ACCEL, 0);
	set_param_min(PARAM_RADIAL_ACCEL, 0);
	set_param_min(PARAM_TANGENTIAL_ACCEL, 0);
	set_param_min(PARAM_DAMPING, 0);
	set_param_min(PARAM_ANGLE, 0);
	set_param_min(PARAM_SCALE, 1);
	set_param_min(PARAM_HUE_VARIATION, 0);
	set_param_min(PARAM_ANIM_SPEED, 0);
	set_param_min(PARAM_ANIM_OFFSET, 0);
	set_param_max(PARAM_INITIAL_LINEAR_VELOCITY, 0);
	set_param_max(PARAM_ANGULAR_VELOCITY, 0);
	set_param_max(PARAM_ORBIT_VELOCITY, 0);
	set_param_max(PARAM_LINEAR_ACCEL, 0);
	set_param_max(PARAM_RADIAL_ACCEL, 0);
	set_param_max(PARAM_TANGENTIAL_ACCEL, 0);
	set_param_max(PARAM_DAMPING, 0);
	set_param_max(PARAM_ANGLE, 0);
	set_param_max(PARAM_SCALE, 1);
	set_param_max(PARAM_HUE_VARIATION, 0);
	set_param_max(PARAM_ANIM_SPEED, 0);
	set_param_max(PARAM_ANIM_OFFSET, 0);
	set_param_min(PARAM_DIRECTIONAL_VELOCITY, 1.0);
	set_param_max(PARAM_DIRECTIONAL_VELOCITY, 1.0);
	set_use_scale_3d(false);
	set_scale_3d_min(Vector3(1.0, 1.0, 1.0));
	set_scale_3d_max(Vector3(1.0, 1.0, 1.0));
	set_use_rotation_3d(false);
	set_rotation_3d_min(Vector3(0.0, 0.0, 0.0));
	set_rotation_3d_max(Vector3(0.0, 0.0, 0.0));
	set_emission_shape(EMISSION_SHAPE_POINT);
	set_emission_sphere_radius(1);
	set_emission_box_extents(Vector3(1, 1, 1));
	set_emission_ring_axis(Vector3(0, 0, 1.0));
	set_emission_ring_height(1);
	set_emission_ring_radius(1);
	set_emission_ring_inner_radius(0);
	set_emission_ring_cone_angle(90);
	set_emission_shape_offset(Vector3(0.0, 0.0, 0.0));
	set_emission_shape_scale(Vector3(1.0, 1.0, 1.0));

	set_turbulence_enabled(false);
	set_turbulence_noise_speed(Vector3(0.0, 0.0, 0.0));
	set_turbulence_noise_strength(1);
	set_turbulence_noise_scale(9);
	set_turbulence_noise_speed_random(0.2);
	set_param_min(PARAM_TURB_VEL_INFLUENCE, 0.1);
	set_param_max(PARAM_TURB_VEL_INFLUENCE, 0.1);
	set_param_min(PARAM_TURB_INIT_DISPLACEMENT, 0.0);
	set_param_max(PARAM_TURB_INIT_DISPLACEMENT, 0.0);

	set_gravity(Vector3(0, -9.8, 0));
	set_lifetime_randomness(0);

	set_sub_emitter_mode(SUB_EMITTER_DISABLED);
	set_sub_emitter_frequency(4);
	set_sub_emitter_amount_at_end(1);
	set_sub_emitter_amount_at_collision(1);
	set_sub_emitter_amount_at_start(1);
	set_sub_emitter_keep_velocity(false);

	set_attractor_interaction_enabled(true);
	set_collision_mode(COLLISION_DISABLED);
	set_collision_bounce(0.0);
	set_collision_friction(0.0);
	set_collision_use_scale(false);

	for (int i = 0; i < PARTICLE_FLAG_MAX; i++) {
		particle_flags[i] = false;
	}

	set_color(Color(1, 1, 1, 1));

	current_key.invalid_key = 1;
}

ParticleProcessMaterial::~ParticleProcessMaterial()
{
	ERR_FAIL_NULL(Renderer::data);
	MutexLock lock(shader_map_mutex);

	if (shader_map.has(current_key)) {
		shader_map[current_key].users--;
		if (shader_map[current_key].users == 0) {
			// deallocate shader, as it's no longer in use
			Renderer::free_rid(shader_map[current_key].shader);
			shader_map.erase(current_key);
		}

		Renderer::material_set_shader(_get_material(), RID());
	}
}

void ParticleProcessMaterial::set_param_min(Parameter p_param, float p_value) {}

void ParticleProcessMaterial::set_param_max(Parameter p_param, float p_value) {}



void ParticleProcessMaterial::_update_shader() {}

void ParticleProcessMaterial::set_use_scale_3d(bool) {}

void ParticleProcessMaterial::set_use_rotation_3d(bool) {}

void ParticleProcessMaterial::set_turbulence_noise_strength(float) {}

void ParticleProcessMaterial::set_turbulence_noise_speed(Vector3 const&) {}

void ParticleProcessMaterial::set_turbulence_noise_speed_random(float) {}

void ParticleProcessMaterial::set_turbulence_noise_scale(float) {}

void ParticleProcessMaterial::set_turbulence_enabled(bool) {}

void ParticleProcessMaterial::set_sub_emitter_mode(ParticleProcessMaterial::SubEmitterMode) {}

void ParticleProcessMaterial::set_sub_emitter_keep_velocity(bool) {}

void ParticleProcessMaterial::set_sub_emitter_frequency(double) {}

void ParticleProcessMaterial::set_sub_emitter_amount_at_start(int) {}

void ParticleProcessMaterial::set_sub_emitter_amount_at_end(int) {}

void ParticleProcessMaterial::set_sub_emitter_amount_at_collision(int) {}

void ParticleProcessMaterial::set_spread(float) {}

void ParticleProcessMaterial::set_scale_3d_min(Vector3 const&) {}

void ParticleProcessMaterial::set_scale_3d_max(Vector3 const&) {}

void ParticleProcessMaterial::set_rotation_3d_min(Vector3 const&) {}

void ParticleProcessMaterial::set_rotation_3d_max(Vector3 const&) {}

void ParticleProcessMaterial::set_lifetime_randomness(double) {}

void ParticleProcessMaterial::set_gravity(Vector3 const&) {}

void ParticleProcessMaterial::set_flatness(float) {}

void ParticleProcessMaterial::set_emission_sphere_radius(float) {}

void ParticleProcessMaterial::set_emission_shape_scale(Vector3 const&) {}

void ParticleProcessMaterial::set_emission_shape(ParticleProcessMaterial::EmissionShape) {}

void ParticleProcessMaterial::set_emission_shape_offset(Vector3 const&) {}

void ParticleProcessMaterial::set_emission_ring_radius(float) {}

void ParticleProcessMaterial::set_emission_ring_inner_radius(float) {}

void ParticleProcessMaterial::set_emission_ring_height(float) {}

void ParticleProcessMaterial::set_emission_ring_cone_angle(float) {}

void ParticleProcessMaterial::set_emission_ring_axis(Vector3) {}

void ParticleProcessMaterial::set_emission_box_extents(Vector3) {}

void ParticleProcessMaterial::set_direction(Vector3) {}

void ParticleProcessMaterial::set_color(Color const&) {}

void ParticleProcessMaterial::set_collision_mode(ParticleProcessMaterial::CollisionMode) {}

void ParticleProcessMaterial::set_collision_friction(float) {}

void ParticleProcessMaterial::set_collision_bounce(float) {}

// --- physics_material.cpp ---

#include "physics_material.h"

#if !defined(PHYSICS_2D_DISABLED) || !defined(PHYSICS_3D_DISABLED)



void PhysicsMaterial::set_friction(real_t p_val)
{
	friction = p_val;
	emit_changed();
}

void PhysicsMaterial::set_rough(bool p_val)
{
	rough = p_val;
	emit_changed();
}

void PhysicsMaterial::set_bounce(real_t p_val)
{
	bounce = p_val;
	emit_changed();
}

void PhysicsMaterial::set_absorbent(bool p_val)
{
	absorbent = p_val;
	emit_changed();
}
#endif // !defined(PHYSICS_2D_DISABLED) || !defined(PHYSICS_3D_DISABLED)

// ============================================================================
// Merged from scene/resources/shader.cpp
// ============================================================================

#include "core/config/engine.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/texture.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/shader_language.h"
#include "servers/rendering/shader_compiler.h"
#include "shader.h"

#ifdef TOOLS_ENABLED
#include "modules/modules_enabled.gen.h" // For regex.
#ifdef MODULE_REGEX_ENABLED
#include "modules/regex/regex.h"
#endif
#endif

Shader::Mode Shader::get_mode() const { return mode; }

void Shader::_check_shader_rid() const
{
	MutexLock lock(shader_rid_mutex);
	if (shader_rid.is_null() && !preprocessed_code.is_empty()) {
		shader_rid = Renderer::shader_create_from_code(
			preprocessed_code, get_path());
		preprocessed_code = String();
	}
}

void Shader::_dependency_changed()
{
	// Preprocess and compile the code again because a dependency has changed. It also calls
	// emit_changed() for us.
	_recompile();
}

void Shader::_recompile() { set_code(get_code()); }

void Shader::set_path(const String& p_path, bool p_take_over)
{
	Resource::set_path(p_path, p_take_over);

	if (shader_rid.is_valid()) {
		Renderer::shader_set_path_hint(shader_rid, p_path);
	}
}

void Shader::set_include_path(const String& p_path)
{
	// Used only if the shader does not have a resource path set,
	// for example during loading stage or when created by code.
	include_path = p_path;
}


void Shader::set_code(const String& p_code)
{
	code = p_code;
}

String Shader::get_code() const
{
	_update_shader();
	return code;
}

RID Shader::get_rid() const
{
	_update_shader();
	_check_shader_rid();

	return shader_rid;
}

void Shader::set_default_texture_parameter(
	const String& p_name, const Ref<Texture>& p_texture, int p_index)
{
	_check_shader_rid();

	if (p_texture.is_valid()) {
		if (!default_textures.has(p_name)) {
			default_textures[p_name] = HashMap<int, Ref<Texture>>();
		}
		default_textures[p_name][p_index] = p_texture;
		Renderer::shader_set_default_texture_parameter(
			shader_rid, p_name, p_texture->get_rid(), p_index);
	}
	else {
		if (default_textures.has(p_name) && default_textures[p_name].has(p_index)) {
			default_textures[p_name].erase(p_index);

			if (default_textures[p_name].is_empty()) {
				default_textures.erase(p_name);
			}
		}
		Renderer::shader_set_default_texture_parameter(
			shader_rid, p_name, RID(), p_index);
	}

	emit_changed();
}

Ref<Texture> Shader::get_default_texture_parameter(const String& p_name, int p_index) const
{
	if (default_textures.has(p_name) && default_textures[p_name].has(p_index)) {
		return default_textures[p_name][p_index];
	}
	return Ref<Texture>();
}

void Shader::get_default_texture_parameter_list(List<String>* r_textures) const
{
	for (const KeyValue<String, HashMap<int, Ref<Texture>>>& E : default_textures) {
		r_textures->push_back(E.key);
	}
}

bool Shader::is_text_shader() const { return true; }

void Shader::_update_shader() const
{
	// Base implementation does nothing.
}

Shader::Shader()
{
	// Shader RID will be empty until it is required.
}

Shader::~Shader()
{
	if (shader_rid.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::free_rid(shader_rid);
	}
}

// --- shader.compat.inc ---

#ifndef DISABLE_DEPRECATED
void Shader::_set_default_texture_parameter_bind_compat_95126(
	const String& p_name, const Ref<Texture2D>& p_texture, int p_index)
{
	set_default_texture_parameter(p_name, p_texture, p_index);
}

Ref<Texture2D> Shader::_get_default_texture_parameter_bind_compat_95126(
	const String& p_name, int p_index) const
{
	return get_default_texture_parameter(p_name, p_index);
}

void Shader::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED

// --- shader_include.cpp ---

#include "servers/rendering/shader_compiler.h"
#include "scene/resources/shader.h"

void ShaderInclude::_dependency_changed() { emit_changed(); }

String ShaderInclude::get_code() const { return code; }

void ShaderInclude::set_include_path(const String& p_path) { include_path = p_path; }

void set_code(const String& p_text) {}

void ShaderInclude::set_code(const String& p_code) {}

// --- shader_resource_format.cpp ---

#include "scene/resources/shader.h"

#include "core/io/file_access.h"
#include "scene/resources/shader.h"

Ref<Resource> ResourceFormatLoaderShader::load(const String& p_path, const String& p_original_path,
	Error* r_error, bool p_use_sub_threads, float* r_progress, CacheMode p_cache_mode)
{
	(void)p_original_path;
	(void)p_use_sub_threads;
	(void)r_progress;
	(void)p_cache_mode;

	if (r_error) {
		*r_error = ERR_FILE_CANT_OPEN;
	}

	const bool is_include = p_path.has_extension("gdshaderinc");

	Error error = OK;
	Vector<uint8_t> buffer = FileAccess::get_file_as_bytes(p_path, &error);
	ERR_FAIL_COND_V_MSG(error, nullptr,
		(is_include ? "Cannot load shader include: " : "Cannot load shader: ") + p_path);

	String str;
	if (buffer.size() > 0) {
		error = str.append_utf8((const char*)buffer.ptr(), buffer.size());
		ERR_FAIL_COND_V_MSG(error, nullptr,
			(is_include ? "Cannot parse shader include: " : "Cannot parse shader: ") + p_path);
	}

	if (is_include) {
		Ref<ShaderInclude> shader_inc;
		shader_inc.instantiate();

		shader_inc->set_include_path(p_path);
		shader_inc->set_code(str);

		if (r_error) {
			*r_error = OK;
		}

		return shader_inc;
	}

	Ref<Shader> shader;
	shader.instantiate();

	shader->set_include_path(p_path);
	shader->set_code(str);

	if (r_error) {
		*r_error = OK;
	}

	return shader;
}

void ResourceFormatLoaderShader::get_recognized_extensions(List<String>* p_extensions) const
{
	p_extensions->push_back("gdshader");
	p_extensions->push_back("shader");
	p_extensions->push_back("gdshaderinc");
}

void ResourceFormatLoaderShader::get_recognized_extensions_for_type(
	const String& p_type, List<String>* p_extensions) const
{
	if (p_type.is_empty()) {
		get_recognized_extensions(p_extensions);
		return;
	}
	if (p_type == "Shader") {
		p_extensions->push_back("gdshader");
		p_extensions->push_back("shader");
	}
	else if (p_type == "ShaderInclude") {
		p_extensions->push_back("gdshaderinc");
	}
}

bool ResourceFormatLoaderShader::handles_type(const String& p_type) const
{
	return (p_type == "Shader" || p_type == "ShaderInclude");
}

String ResourceFormatLoaderShader::get_resource_type(const String& p_path) const
{
	if (p_path.has_extension("gdshader") || p_path.has_extension("shader")) {
		return "Shader";
	}
	if (p_path.has_extension("gdshaderinc")) {
		return "ShaderInclude";
	}
	return "";
}

Error ResourceFormatSaverShader::save(
	const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags)
{
	(void)p_flags;

	Ref<Shader> shader = p_resource;
	Ref<ShaderInclude> shader_inc = p_resource;
	ERR_FAIL_COND_V(shader.is_null() && shader_inc.is_null(), ERR_INVALID_PARAMETER);

	String source = shader.is_valid() ? shader->get_code() : shader_inc->get_code();

	Error err;
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE, &err);

	ERR_FAIL_COND_V_MSG(err, err,
		(shader_inc.is_valid() ? "Cannot save shader include '" : "Cannot save shader '") + p_path +
			"'.");

	file->store_string(source);
	if (file->get_error() != OK && file->get_error() != ERR_FILE_EOF) {
		return ERR_CANT_CREATE;
	}

	return OK;
}

bool ResourceFormatSaverShader::recognize(const Ref<Resource>& p_resource) const
{
	if (p_resource.is_null()) {
		return false;
	}
	Ref<Shader> shader = p_resource;
	if (shader.is_valid()) {
		return shader->is_text_shader();
	}
	Ref<ShaderInclude> shader_inc = p_resource;
	return shader_inc.is_valid();
}

void ResourceFormatSaverShader::get_recognized_extensions(
	const Ref<Resource>& p_resource, List<String>* p_extensions) const
{
	if (p_resource.is_null()) {
		return;
	}
	Ref<Shader> shader = p_resource;
	if (shader.is_valid() && shader->is_text_shader()) {
		p_extensions->push_back("gdshader");
	}
	Ref<ShaderInclude> shader_inc = p_resource;
	if (shader_inc.is_valid()) {
		p_extensions->push_back("gdshaderinc");
	}
}

// ============================================================================
// Merged from scene/resources/environment.cpp
// ============================================================================

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/os/os.h"
#include "environment.h"
#include "scene/resources/gradient_texture.h"
#include "scene/resources/environment.h"
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

#include "scene/resources/material.h"
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

#include "scene/resources/environment.h"
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
#include "scene/resources/environment.h"

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
#include "scene/2d/camera_2d.h"
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
