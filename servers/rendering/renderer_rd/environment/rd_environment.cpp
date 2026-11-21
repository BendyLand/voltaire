/**************************************************************************/
/*  rd_environment.cpp                                                    */
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

#include "servers/rendering/renderer_rd/environment/rd_environment.h"
#include "servers/rendering/renderer_rd/rd_caches.h"
#include "core/config/project_settings.h"
#include "core/math/math_defs.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/renderer_rd/effects/copy_effects.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/renderer_scene_render_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/render_data_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/render_scene_buffers_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/texture_storage.h"
#include "core/math/geometry_3d.h"

// --- Merged from servers/rendering/renderer_rd/environment/sky.cpp ---

using namespace RendererRD;

#define RB_SCOPE_SKY SNAME("sky_buffers")
#define RB_HALF_TEXTURE SNAME("half_texture")
#define RB_QUARTER_TEXTURE SNAME("quarter_texture")

bool SkyRD::SkyShaderData::is_animated() const { return false; }

bool SkyRD::SkyShaderData::casts_shadows() const { return false; }

RenderingServerTypes::ShaderNativeSourceCode SkyRD::SkyShaderData::get_native_source_code() const
{
	return RendererSceneRenderRD::data->sky.sky_shader.shader.version_get_native_source_code(
		version);
}

Pair<ShaderRD*, RID> SkyRD::SkyShaderData::get_native_shader_and_version() const
{
	return {&RendererSceneRenderRD::data->sky.sky_shader.shader, version};
}

SkyRD::SkyShaderData::~SkyShaderData()
{
	ERR_FAIL_NULL(RendererSceneRenderRD::data);
	// pipeline variants will clear themselves if shader is gone
	if (version.is_valid()) {
		RendererSceneRenderRD::data->sky.sky_shader.shader.version_free(version);
	}
}

SkyRD::SkyMaterialData::~SkyMaterialData() { free_parameters_uniform_set(uniform_set); }

static _FORCE_INLINE_ void store_transform_3x3(const Basis& p_basis, float* p_array)
{
	p_array[0] = p_basis.rows[0][0];
	p_array[1] = p_basis.rows[1][0];
	p_array[2] = p_basis.rows[2][0];
	p_array[3] = 0;
	p_array[4] = p_basis.rows[0][1];
	p_array[5] = p_basis.rows[1][1];
	p_array[6] = p_basis.rows[2][1];
	p_array[7] = 0;
	p_array[8] = p_basis.rows[0][2];
	p_array[9] = p_basis.rows[1][2];
	p_array[10] = p_basis.rows[2][2];
	p_array[11] = 0;
}

void SkyRD::_render_sky(RD::DrawListID p_list, float p_time, RID p_fb, PipelineCacheRD* p_pipeline,
	RID p_uniform_set, RID p_texture_set, const Projection& p_projection,
	const Basis& p_orientation, const Vector3& p_position, float p_luminance_multiplier,
	float p_brightness_multiplier, float p_border_size)
{
	SkyPushConstant sky_push_constant;

	memset(&sky_push_constant, 0, sizeof(SkyPushConstant));

	// We only need key components of our projection matrix
	sky_push_constant.projection[0] = p_projection.columns[2][0];
	sky_push_constant.projection[1] = p_projection.columns[0][0];
	sky_push_constant.projection[2] = p_projection.columns[2][1];
	sky_push_constant.projection[3] = p_projection.columns[1][1];

	sky_push_constant.position[0] = p_position.x;
	sky_push_constant.position[1] = p_position.y;
	sky_push_constant.position[2] = p_position.z;
	sky_push_constant.time = p_time;
	sky_push_constant.border_size[0] = p_border_size;
	sky_push_constant.border_size[1] = 1.0f - p_border_size * 2.0;
	sky_push_constant.luminance_multiplier = p_luminance_multiplier;
	sky_push_constant.brightness_multiplier = p_brightness_multiplier;
	store_transform_3x3(p_orientation, sky_push_constant.orientation);

	RenderingDevice::FramebufferFormatID fb_format = RD::framebuffer_get_format(p_fb);

	RD::DrawListID draw_list = p_list;

	RD::draw_list_bind_render_pipeline(
		draw_list, p_pipeline->get_render_pipeline(
					   RDC::INVALID_ID, fb_format, false, RD::draw_list_get_current_pass()));

	// Update uniform sets.
	{
		RD::draw_list_bind_uniform_set(draw_list, sky_scene_state.uniform_set, SKY_SET_UNIFORMS);
		if (p_uniform_set.is_valid() &&
			RD::uniform_set_is_valid(p_uniform_set)) { // Material may not have a uniform set.
			RD::draw_list_bind_uniform_set(draw_list, p_uniform_set, SKY_SET_MATERIAL);
		}
		RD::draw_list_bind_uniform_set(draw_list, p_texture_set, SKY_SET_TEXTURES);
		// Fog uniform set can be invalidated before drawing, so validate at draw time
		if (sky_scene_state.fog_uniform_set.is_valid() &&
			RD::uniform_set_is_valid(sky_scene_state.fog_uniform_set)) {
			RD::draw_list_bind_uniform_set(draw_list, sky_scene_state.fog_uniform_set, SKY_SET_FOG);
		}
		else {
			RD::draw_list_bind_uniform_set(
				draw_list, sky_scene_state.default_fog_uniform_set, SKY_SET_FOG);
		}
	}

	RD::draw_list_set_push_constant(draw_list, &sky_push_constant, sizeof(SkyPushConstant));

	RD::draw_list_draw(draw_list, false, 1u, 3u);
}

void SkyRD::ReflectionData::clear_reflection_data()
{
	layers.clear();
	radiance_base_octmap = RID();
	if (downsampled_radiance_octmap.is_valid()) {
		RD::free_rid(downsampled_radiance_octmap);
	}
	downsampled_radiance_octmap = RID();
	downsampled_layer.mipmaps.clear();
	coefficient_buffer = RID();
}

void SkyRD::Sky::free_radiance()
{
	if (radiance.is_valid()) {
		RD::free_rid(radiance);
		radiance = RID();
	}
	if (radiance_first_layer_slice.is_valid()) {
		if (RD::texture_is_valid(radiance_first_layer_slice)) {
			RD::free_rid(radiance_first_layer_slice);
		}
		radiance_first_layer_slice = RID();
	}
}

void SkyRD::Sky::free()
{
	free_radiance();
	reflection.clear_reflection_data();

	if (uniform_buffer.is_valid()) {
		RD::free_rid(uniform_buffer);
		uniform_buffer = RID();
	}

	if (material.is_valid()) {
		material = RID();
	}
}

RID SkyRD::Sky::get_textures(SkyTextureSetVersion p_version, RID p_default_shader_rd,
	bool p_is_multiview, Ref<RenderSceneBuffersRD> p_render_buffers)
{
	thread_local LocalVector<RD::Uniform> uniforms;
	uniforms.clear();

	{
		RD::Uniform u;
		u.uniform_type = RDC::UNIFORM_TYPE_TEXTURE;
		u.binding = 0;
		if (radiance.is_valid() && p_version <= SKY_TEXTURE_SET_QUARTER_RES) {
			u.append_id(
				radiance_first_layer_slice.is_valid() ? radiance_first_layer_slice : radiance);
		}
		else {
			u.append_id(RendererRD::TextureStorage::texture_rd_get_default(
				RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_BLACK));
		}
		uniforms.push_back(u);
	}
	{
		RD::Uniform u;
		u.uniform_type = RDC::UNIFORM_TYPE_TEXTURE;
		u.binding = 1; // half res
		if (p_version >= SKY_TEXTURE_SET_OCTMAP) {
			if (reflection.layers.size() && reflection.layers[0].mipmaps.size() >= 2 &&
				reflection.layers[0].mipmaps[1].view.is_valid() &&
				p_version != SKY_TEXTURE_SET_OCTMAP_HALF_RES) {
				u.append_id(reflection.layers[0].mipmaps[1].view);
			}
			else {
				u.append_id(RendererRD::TextureStorage::texture_rd_get_default(
					RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_BLACK));
			}
		}
		else {
			RID half_texture = p_render_buffers->has_texture(RB_SCOPE_SKY, RB_HALF_TEXTURE)
								   ? p_render_buffers->get_texture(RB_SCOPE_SKY, RB_HALF_TEXTURE)
								   : RID();
			if (half_texture.is_valid() && p_version != SKY_TEXTURE_SET_HALF_RES) {
				u.append_id(half_texture);
			}
			else {
				u.append_id(RendererRD::TextureStorage::texture_rd_get_default(
					p_is_multiview ? RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_2D_ARRAY_WHITE
								   : RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_WHITE));
			}
		}
		uniforms.push_back(u);
	}
	{
		RD::Uniform u;
		u.uniform_type = RDC::UNIFORM_TYPE_TEXTURE;
		u.binding = 2; // quarter res
		if (p_version >= SKY_TEXTURE_SET_OCTMAP) {
			if (reflection.layers.size() && reflection.layers[0].mipmaps.size() >= 3 &&
				reflection.layers[0].mipmaps[2].view.is_valid() &&
				p_version != SKY_TEXTURE_SET_OCTMAP_QUARTER_RES) {
				u.append_id(reflection.layers[0].mipmaps[2].view);
			}
			else {
				u.append_id(RendererRD::TextureStorage::texture_rd_get_default(
					RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_BLACK));
			}
		}
		else {
			RID quarter_texture =
				p_render_buffers->has_texture(RB_SCOPE_SKY, RB_QUARTER_TEXTURE)
					? p_render_buffers->get_texture(RB_SCOPE_SKY, RB_QUARTER_TEXTURE)
					: RID();
			if (quarter_texture.is_valid() && p_version != SKY_TEXTURE_SET_QUARTER_RES) {
				u.append_id(quarter_texture);
			}
			else {
				u.append_id(RendererRD::TextureStorage::texture_rd_get_default(
					p_is_multiview ? RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_2D_ARRAY_WHITE
								   : RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_WHITE));
			}
		}
		uniforms.push_back(u);
	}

	return UniformSetCacheRD::get_cache_vec(p_default_shader_rd, SKY_SET_TEXTURES, uniforms);
}

bool SkyRD::Sky::set_radiance_size(int p_radiance_size)
{
	ERR_FAIL_COND_V(p_radiance_size < 32 || p_radiance_size > 2048, false);
	if (radiance_size == p_radiance_size) {
		return false;
	}
	radiance_size = p_radiance_size;

	if (mode == RSE::SKY_MODE_REALTIME && radiance_size != REAL_TIME_SIZE) {
		WARN_PRINT(vformat("Realtime Skies can only use a radiance size of %d. Radiance size will "
						   "be set to %d internally.",
			REAL_TIME_SIZE, REAL_TIME_SIZE));
		radiance_size = REAL_TIME_SIZE;
	}

	free_radiance();
	reflection.clear_reflection_data();

	return true;
}

int SkyRD::Sky::get_radiance_size() const { return radiance_size; }

bool SkyRD::Sky::set_mode(RSE::SkyMode p_mode)
{
	if (mode == p_mode) {
		return false;
	}

	mode = p_mode;

	if (mode == RSE::SKY_MODE_REALTIME && radiance_size != REAL_TIME_SIZE) {
		WARN_PRINT(vformat("Realtime Skies can only use a radiance size of %d. Radiance size will "
						   "be set to %d internally.",
			REAL_TIME_SIZE, REAL_TIME_SIZE));
		set_radiance_size(REAL_TIME_SIZE);
	}

	free_radiance();
	reflection.clear_reflection_data();

	return true;
}

bool SkyRD::Sky::set_material(RID p_material)
{
	if (material == p_material) {
		return false;
	}

	material = p_material;
	return true;
}

Ref<Image> SkyRD::Sky::bake_panorama(float p_energy, int p_roughness_layers, const Size2i& p_size)
{
	if (radiance.is_valid()) {
		ERR_FAIL_COND_V(!RendererRD::CopyEffects::is_initialized(), Ref<Image>());

		RDC::TextureFormat tf;
		tf.format = RDC::DATA_FORMAT_R32G32B32A32_SFLOAT; // Could be RGBA16
		tf.width = p_size.width;
		tf.height = p_size.height;
		tf.usage_bits = RDC::TEXTURE_USAGE_STORAGE_BIT | RDC::TEXTURE_USAGE_CAN_COPY_FROM_BIT;

		RID rad_tex = RD::texture_create(tf, RDD::TextureView());
		RendererRD::CopyEffects::copy_octmap_to_panorama(radiance, rad_tex, p_size,
			p_roughness_layers, reflection.layers.size() > 1,
			Size2(uv_border_size, 1.0f - uv_border_size * 2.0));
		Vector<uint8_t> data = RD::texture_get_data(rad_tex, 0);
		RD::free_rid(rad_tex);

		Ref<Image> img =
			Image::create_from_data(p_size.width, p_size.height, false, Image::FORMAT_RGBAF, data);
		for (int i = 0; i < p_size.width; i++) {
			for (int j = 0; j < p_size.height; j++) {
				Color c = img->get_pixel(i, j);
				c.r *= p_energy;
				c.g *= p_energy;
				c.b *= p_energy;
				img->set_pixel(i, j, c);
			}
		}
		return img;
	}

	return Ref<Image>();
}

RendererRD::MaterialStorage::ShaderData* SkyRD::_create_sky_shader_func()
{
	SkyShaderData* shader_data = memnew(SkyShaderData);
	return shader_data;
}

RendererRD::MaterialStorage::ShaderData* SkyRD::_create_sky_shader_funcs()
{
	return RendererSceneRenderRD::data->sky._create_sky_shader_func();
}

RendererRD::MaterialStorage::MaterialData* SkyRD::_create_sky_material_func(SkyShaderData* p_shader)
{
	SkyMaterialData* material_data = memnew(SkyMaterialData);
	material_data->shader_data = p_shader;
	// update will happen later anyway so do nothing.
	return material_data;
}

RendererRD::MaterialStorage::MaterialData* SkyRD::_create_sky_material_funcs(
	RendererRD::MaterialStorage::ShaderData* p_shader)
{
	return RendererSceneRenderRD::data->sky._create_sky_material_func(
		static_cast<SkyShaderData*>(p_shader));
}

RID SkyRD::SkySceneState::get_fog_only_texture_uniform_set(
	RID p_default_shader_rd, bool p_is_multiview)
{
	RID& uniform_set_rid =
		p_is_multiview ? fog_only_texture_multiview_uniform_set : fog_only_texture_uniform_set;

	if (uniform_set_rid.is_null()) {
		Vector<RD::Uniform> uniforms;
		{
			RD::Uniform u;
			u.uniform_type = RDC::UNIFORM_TYPE_TEXTURE;
			u.binding = 0;
			u.append_id(RendererRD::TextureStorage::texture_rd_get_default(
				RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_BLACK));
			uniforms.push_back(u);
		}
		{
			RD::Uniform u;
			u.uniform_type = RDC::UNIFORM_TYPE_TEXTURE;
			u.binding = 1;
			u.append_id(RendererRD::TextureStorage::texture_rd_get_default(
				p_is_multiview ? RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_2D_ARRAY_WHITE
							   : RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_WHITE));
			uniforms.push_back(u);
		}
		{
			RD::Uniform u;
			u.uniform_type = RDC::UNIFORM_TYPE_TEXTURE;
			u.binding = 2;
			u.append_id(RendererRD::TextureStorage::texture_rd_get_default(
				p_is_multiview ? RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_2D_ARRAY_WHITE
							   : RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_WHITE));
			uniforms.push_back(u);
		}

		uniform_set_rid = RD::uniform_set_create(uniforms, p_default_shader_rd, SKY_SET_TEXTURES);
	}

	return uniform_set_rid;
}

void SkyRD::set_texture_format(RDC::DataFormat p_texture_format)
{
	texture_format = p_texture_format;
}

SkyRD::~SkyRD()
{
	// cleanup anything created in init...
	if (sky_shader.default_material.is_valid()) {
		SkyMaterialData* md = static_cast<SkyMaterialData*>(MaterialStorage::material_get_data(
			sky_shader.default_material, RendererRD::MaterialStorage::SHADER_TYPE_SKY));
		if (md && md->shader_data && md->shader_data->version.is_valid()) {
			sky_shader.shader.version_free(md->shader_data->version);
			md->shader_data->version = RID();
		}
		MaterialStorage::material_free(sky_shader.default_material);
		sky_shader.default_material = RID();
	}
	if (sky_shader.default_shader.is_valid()) {
		MaterialStorage::shader_free(sky_shader.default_shader);
		sky_shader.default_shader = RID();
	}
	if (sky_scene_state.directional_light_buffer.is_valid()) {
		RD::free_rid(sky_scene_state.directional_light_buffer);
	}
	if (sky_scene_state.uniform_buffer.is_valid()) {
		RD::free_rid(sky_scene_state.uniform_buffer);
	}
	if (sky_scene_state.directional_lights) {
		memdelete_arr(sky_scene_state.directional_lights);
	}
	if (sky_scene_state.last_frame_directional_lights) {
		memdelete_arr(sky_scene_state.last_frame_directional_lights);
	}
	if (sky_scene_state.fog_material.is_valid()) {
		MaterialStorage::material_free(sky_scene_state.fog_material);
	}
	if (sky_scene_state.fog_shader.is_valid()) {
		MaterialStorage::shader_free(sky_scene_state.fog_shader);
	}

	if (RD::uniform_set_is_valid(sky_scene_state.uniform_set)) {
		RD::free_rid(sky_scene_state.uniform_set);
	}

	if (RD::uniform_set_is_valid(sky_scene_state.default_fog_uniform_set)) {
		RD::free_rid(sky_scene_state.default_fog_uniform_set);
	}

	if (sky_scene_state.fog_only_texture_uniform_set.is_valid() &&
		RD::uniform_set_is_valid(sky_scene_state.fog_only_texture_uniform_set)) {
		RD::free_rid(sky_scene_state.fog_only_texture_uniform_set);
	}

	if (sky_scene_state.fog_only_texture_multiview_uniform_set.is_valid() &&
		RD::uniform_set_is_valid(sky_scene_state.fog_only_texture_multiview_uniform_set)) {
		RD::free_rid(sky_scene_state.fog_only_texture_multiview_uniform_set);
	}
}

void SkyRD::setup_sky(const RenderDataRD* p_render_data, const Size2i p_screen_size)
{
	ERR_FAIL_COND(p_render_data->environment.is_null());

	ERR_FAIL_COND(p_render_data->render_buffers.is_null());

	// make sure we support our view count
	ERR_FAIL_COND(p_render_data->scene_data->view_count == 0);
	ERR_FAIL_COND(p_render_data->scene_data->view_count > RendererSceneRender::MAX_RENDER_VIEWS);

	SkyMaterialData* material_data = _get_sky_material_data(p_render_data->environment);
	ERR_FAIL_NULL(material_data);

	SkyShaderData* shader_data = material_data->shader_data;
	ERR_FAIL_NULL(shader_data);

	material_data->set_as_used();

	Sky* sky = get_sky(RendererSceneRender::environment_get_sky(p_render_data->environment));
	if (sky) {
		// Save our screen size; our buffers will already have been cleared.
		sky->screen_size.x = p_screen_size.x < 4 ? 4 : p_screen_size.x;
		sky->screen_size.y = p_screen_size.y < 4 ? 4 : p_screen_size.y;

		RSE::SkyMode sky_mode = sky->mode;

		if (sky_mode == RSE::SKY_MODE_AUTOMATIC) {
			bool sun_scatter_enabled =
				RendererSceneRender::environment_get_fog_enabled(p_render_data->environment) &&
				RendererSceneRender::environment_get_fog_sun_scatter(p_render_data->environment) >
					0.001;

			if ((shader_data->uses_time || shader_data->uses_position) &&
				sky->radiance_size == Sky::REAL_TIME_SIZE) {
				sky_mode = RSE::SKY_MODE_REALTIME;
			}
			else if (shader_data->uses_light || sun_scatter_enabled ||
					   shader_data->ubo_size > 0) {
				sky_mode = RSE::SKY_MODE_INCREMENTAL;
			}
			else {
				sky_mode = RSE::SKY_MODE_QUALITY;
			}

			if (sky_mode != sky->internal_mode) {
				sky->internal_mode = sky_mode;

				sky->free_radiance();
				sky->reflection.clear_reflection_data();
			}
		}
		else {
			sky->internal_mode = sky_mode;
		}

		// Trigger updating radiance buffers.
		if (sky->radiance.is_null()) {
			invalidate_sky(sky);
			update_dirty_skys();
		}

		if (shader_data->uses_time && p_render_data->scene_data->time - sky->prev_time > 0.00001) {
			sky->prev_time = p_render_data->scene_data->time;
			sky->reflection.dirty = true;
			Renderer::redraw_request();
		}

		if (RendererSceneRender::environment_get_fog_aerial_perspective(
				p_render_data->environment) != sky->prev_fog_aerial_perspective) {
			sky->prev_fog_aerial_perspective =
				RendererSceneRender::environment_get_fog_aerial_perspective(
					p_render_data->environment);
			sky->reflection.dirty = true;
			Renderer::redraw_request();
		}

		if (RendererSceneRender::environment_get_fog_light_color(p_render_data->environment) !=
			sky->prev_fog_light_color) {
			sky->prev_fog_light_color =
				RendererSceneRender::environment_get_fog_light_color(p_render_data->environment);
			sky->reflection.dirty = true;
			Renderer::redraw_request();
		}

		if (RendererSceneRender::environment_get_fog_sun_scatter(p_render_data->environment) !=
			sky->prev_fog_sun_scatter) {
			sky->prev_fog_sun_scatter =
				RendererSceneRender::environment_get_fog_sun_scatter(p_render_data->environment);
			sky->reflection.dirty = true;
			Renderer::redraw_request();
		}

		if (RendererSceneRender::environment_get_fog_enabled(p_render_data->environment) !=
			sky->prev_fog_enabled) {
			sky->prev_fog_enabled =
				RendererSceneRender::environment_get_fog_enabled(p_render_data->environment);
			sky->reflection.dirty = true;
			Renderer::redraw_request();
		}

		if (RendererSceneRender::environment_get_fog_density(p_render_data->environment) !=
			sky->prev_fog_density) {
			sky->prev_fog_density =
				RendererSceneRender::environment_get_fog_density(p_render_data->environment);
			sky->reflection.dirty = true;
			Renderer::redraw_request();
		}

		if (RendererSceneRender::environment_get_fog_sky_affect(p_render_data->environment) !=
			sky->prev_fog_sky_affect) {
			sky->prev_fog_sky_affect =
				RendererSceneRender::environment_get_fog_sky_affect(p_render_data->environment);
			sky->reflection.dirty = true;
			Renderer::redraw_request();
		}

		if (RendererSceneRender::environment_get_fog_light_energy(p_render_data->environment) !=
			sky->prev_fog_light_energy) {
			sky->prev_fog_light_energy =
				RendererSceneRender::environment_get_fog_light_energy(p_render_data->environment);
			sky->reflection.dirty = true;
			Renderer::redraw_request();
		}

		if (material_data != sky->prev_material_data) {
			sky->prev_material_data = material_data;
			sky->reflection.dirty = true;
		}

		if (material_data->uniform_set_updated) {
			material_data->uniform_set_updated = false;
			sky->reflection.dirty = true;
		}

		if (!p_render_data->scene_data->cam_transform.origin.is_equal_approx(sky->prev_position) &&
			shader_data->uses_position) {
			sky->prev_position = p_render_data->scene_data->cam_transform.origin;
			sky->reflection.dirty = true;
		}
	}

	bool sun_scatter_enabled =
		RendererSceneRender::environment_get_fog_enabled(p_render_data->environment) &&
		RendererSceneRender::environment_get_fog_sun_scatter(p_render_data->environment) > 0.001;
	sky_scene_state.ubo.directional_light_count = 0;
	if (shader_data->uses_light || sun_scatter_enabled) {
		const PagedArray<RID>& lights = *p_render_data->lights;
		// Run through the list of lights in the scene and pick out the Directional Lights.
		// This can't be done in RenderSceneRenderRD::_setup lights because that needs to be called
		// after the depth prepass, but this runs before the depth prepass.
		for (int i = 0; i < (int)lights.size(); i++) {
			if (!RendererRD::LightStorage::owns_light_instance(lights[i])) {
				continue;
			}
			RID base = RendererRD::LightStorage::light_instance_get_base_light(lights[i]);

			ERR_CONTINUE(base.is_null());

			RSE::LightType type = RendererRD::LightStorage::light_get_type(base);
			if (type == RSE::LIGHT_DIRECTIONAL &&
				RendererRD::LightStorage::light_directional_get_sky_mode(base) !=
					RSE::LIGHT_DIRECTIONAL_SKY_MODE_LIGHT_ONLY) {
				SkyDirectionalLightData& sky_light_data =
					sky_scene_state.directional_lights[sky_scene_state.ubo.directional_light_count];
				Transform3D light_transform =
					RendererRD::LightStorage::light_instance_get_base_transform(lights[i]);
				Vector3 world_direction =
					light_transform.basis.xform(Vector3(0, 0, 1)).normalized();

				sky_light_data.direction[0] = world_direction.x;
				sky_light_data.direction[1] = world_direction.y;
				sky_light_data.direction[2] = world_direction.z;

				float sign = RendererRD::LightStorage::light_is_negative(base) ? -1 : 1;
				sky_light_data.energy =
					sign * RendererRD::LightStorage::light_get_param(base, RSE::LIGHT_PARAM_ENERGY);

				if (RendererSceneRenderRD::is_using_physical_light_units()) {
					sky_light_data.energy *=
						RendererRD::LightStorage::light_get_param(base, RSE::LIGHT_PARAM_INTENSITY);
				}

				if (p_render_data->camera_attributes.is_valid()) {
					sky_light_data.energy *=
						RendererCameraAttributes::camera_attributes_get_exposure_normalization_factor(
							p_render_data->camera_attributes);
				}

				Color linear_col = RendererRD::LightStorage::light_get_color(base).srgb_to_linear();
				sky_light_data.color[0] = linear_col.r;
				sky_light_data.color[1] = linear_col.g;
				sky_light_data.color[2] = linear_col.b;

				sky_light_data.enabled = true;

				float angular_diameter =
					RendererRD::LightStorage::light_get_param(base, RSE::LIGHT_PARAM_SIZE);
				sky_light_data.size = Math::deg_to_rad(angular_diameter);
				sky_scene_state.ubo.directional_light_count++;
				if (sky_scene_state.ubo.directional_light_count >=
					sky_scene_state.max_directional_lights) {
					break;
				}
			}
		}
		// Check whether the directional_light_buffer changes.
		bool light_data_dirty = false;

		// Light buffer is dirty if we have fewer or more lights.
		// If we have fewer lights, make sure that old lights are disabled.
		if (sky_scene_state.ubo.directional_light_count !=
			sky_scene_state.last_frame_directional_light_count) {
			light_data_dirty = true;
			for (uint32_t i = sky_scene_state.ubo.directional_light_count;
				 i < sky_scene_state.max_directional_lights; i++) {
				sky_scene_state.directional_lights[i] = {};
				sky_scene_state.directional_lights[i].enabled = false;
				sky_scene_state.last_frame_directional_lights[i] = {};
				sky_scene_state.last_frame_directional_lights[i].enabled = false;
			}
		}

		if (!light_data_dirty) {
			for (uint32_t i = 0; i < sky_scene_state.ubo.directional_light_count; i++) {
				if (sky_scene_state.directional_lights[i].direction[0] !=
						sky_scene_state.last_frame_directional_lights[i].direction[0] ||
					sky_scene_state.directional_lights[i].direction[1] !=
						sky_scene_state.last_frame_directional_lights[i].direction[1] ||
					sky_scene_state.directional_lights[i].direction[2] !=
						sky_scene_state.last_frame_directional_lights[i].direction[2] ||
					sky_scene_state.directional_lights[i].energy !=
						sky_scene_state.last_frame_directional_lights[i].energy ||
					sky_scene_state.directional_lights[i].color[0] !=
						sky_scene_state.last_frame_directional_lights[i].color[0] ||
					sky_scene_state.directional_lights[i].color[1] !=
						sky_scene_state.last_frame_directional_lights[i].color[1] ||
					sky_scene_state.directional_lights[i].color[2] !=
						sky_scene_state.last_frame_directional_lights[i].color[2] ||
					sky_scene_state.directional_lights[i].enabled !=
						sky_scene_state.last_frame_directional_lights[i].enabled ||
					sky_scene_state.directional_lights[i].size !=
						sky_scene_state.last_frame_directional_lights[i].size) {
					light_data_dirty = true;
					break;
				}
			}
		}

		if (light_data_dirty) {
			SkyDirectionalLightData* temp = sky_scene_state.last_frame_directional_lights;
			sky_scene_state.last_frame_directional_lights = sky_scene_state.directional_lights;
			sky_scene_state.directional_lights = temp;
			sky_scene_state.last_frame_directional_light_count =
				sky_scene_state.ubo.directional_light_count;
			if (sky) {
				sky->reflection.dirty = true;
			}
		}
	}

	// Setup fog variables.
	sky_scene_state.ubo.volumetric_fog_enabled = false;
	if (p_render_data->render_buffers->has_custom_data(RB_SCOPE_FOG)) {
		Ref<RendererRD::Fog::VolumetricFog> fog =
			p_render_data->render_buffers->get_custom_data(RB_SCOPE_FOG);
		sky_scene_state.ubo.volumetric_fog_enabled = true;

		float fog_end = fog->length;
		if (fog_end > 0.0) {
			sky_scene_state.ubo.volumetric_fog_inv_length = 1.0 / fog_end;
		}
		else {
			sky_scene_state.ubo.volumetric_fog_inv_length = 1.0;
		}

		float fog_detail_spread = fog->spread; // Reverse lookup.
		if (fog_detail_spread > 0.0) {
			sky_scene_state.ubo.volumetric_fog_detail_spread = 1.0 / fog_detail_spread;
		}
		else {
			sky_scene_state.ubo.volumetric_fog_detail_spread = 1.0;
		}

		sky_scene_state.fog_uniform_set = fog->sky_uniform_set;
	}

	sky_scene_state.view_count = p_render_data->scene_data->view_count;
	sky_scene_state.cam_transform = p_render_data->scene_data->cam_transform;

	Projection correction;
	correction.set_depth_correction(p_render_data->scene_data->flip_y, true);
	correction.add_jitter_offset(p_render_data->scene_data->taa_jitter);

	Projection projection = p_render_data->scene_data->cam_projection;

	float custom_fov =
		RendererSceneRender::environment_get_sky_custom_fov(p_render_data->environment);

	if (custom_fov && sky_scene_state.view_count == 1) {
		// With custom fov we don't support stereo...
		float near_plane = projection.get_z_near();
		float far_plane = projection.get_z_far();
		float aspect = projection.get_aspect();

		projection.set_perspective(custom_fov, aspect, near_plane, far_plane);
	}

	sky_scene_state.cam_projection = correction * projection;

	// Our info in our UBO is only used if we're rendering stereo.
	for (uint32_t i = 0; i < p_render_data->scene_data->view_count; i++) {
		Projection view_inv_projection =
			(correction * p_render_data->scene_data->view_projection[i]).inverse();
		if (p_render_data->scene_data->view_count > 1) {
			// Reprojection is used when we need to have things in combined space.
			RendererRD::MaterialStorage::store_camera(
				sky_scene_state.cam_projection * view_inv_projection,
				sky_scene_state.ubo.combined_reprojection[i]);
		}
		else {
			// This is unused so just reset to identity.
			Projection ident;
			RendererRD::MaterialStorage::store_camera(
				ident, sky_scene_state.ubo.combined_reprojection[i]);
		}

		RendererRD::MaterialStorage::store_camera(
			view_inv_projection, sky_scene_state.ubo.view_inv_projections[i]);
		sky_scene_state.ubo.view_eye_offsets[i][0] =
			p_render_data->scene_data->view_eye_offset[i].x;
		sky_scene_state.ubo.view_eye_offsets[i][1] =
			p_render_data->scene_data->view_eye_offset[i].y;
		sky_scene_state.ubo.view_eye_offsets[i][2] =
			p_render_data->scene_data->view_eye_offset[i].z;
		sky_scene_state.ubo.view_eye_offsets[i][3] = 0.0;
	}

	sky_scene_state.ubo.z_far = p_render_data->scene_data->view_projection[0]
									.get_z_far(); // Should be the same for all projection.
	sky_scene_state.ubo.fog_enabled =
		RendererSceneRender::environment_get_fog_enabled(p_render_data->environment);
	sky_scene_state.ubo.fog_density =
		RendererSceneRender::environment_get_fog_density(p_render_data->environment);
	sky_scene_state.ubo.fog_aerial_perspective =
		RendererSceneRender::environment_get_fog_aerial_perspective(p_render_data->environment);
	Color fog_color =
		RendererSceneRender::environment_get_fog_light_color(p_render_data->environment)
			.srgb_to_linear();
	float fog_energy =
		RendererSceneRender::environment_get_fog_light_energy(p_render_data->environment);
	sky_scene_state.ubo.fog_light_color[0] = fog_color.r * fog_energy;
	sky_scene_state.ubo.fog_light_color[1] = fog_color.g * fog_energy;
	sky_scene_state.ubo.fog_light_color[2] = fog_color.b * fog_energy;
	sky_scene_state.ubo.fog_sun_scatter =
		RendererSceneRender::environment_get_fog_sun_scatter(p_render_data->environment);

	sky_scene_state.ubo.fog_sky_affect =
		RendererSceneRender::environment_get_fog_sky_affect(p_render_data->environment);
	sky_scene_state.ubo.volumetric_fog_sky_affect =
		RendererSceneRender::environment_get_volumetric_fog_sky_affect(p_render_data->environment);
	sky_scene_state.ubo.fog_use_legacy_blending =
		RendererSceneRenderRD::fog_use_legacy_blending_get();
}

void SkyRD::draw_sky(RD::DrawListID p_draw_list, Ref<RenderSceneBuffersRD> p_render_buffers,
	RID p_env, RID p_fb, double p_time, float p_luminance_multiplier, float p_brightness_multiplier)
{
	ERR_FAIL_COND(p_render_buffers.is_null());
	ERR_FAIL_COND(p_env.is_null());

	Sky* sky = get_sky(RendererSceneRender::environment_get_sky(p_env));

	SkyMaterialData* material_data = _get_sky_material_data(p_env);
	ERR_FAIL_NULL(material_data);

	SkyShaderData* shader_data = material_data->shader_data;
	ERR_FAIL_NULL(shader_data);

	material_data->set_as_used();

	Basis sky_transform = RendererSceneRender::environment_get_sky_orientation(p_env);
	sky_transform.invert();

	// Camera
	Projection projection = sky_scene_state.cam_projection;

	sky_transform = sky_transform * sky_scene_state.cam_transform.basis;

	bool is_multiview = sky_scene_state.view_count > 1;

	PipelineCacheRD* pipeline =
		&shader_data
			 ->pipelines[is_multiview ? SKY_VERSION_BACKGROUND_MULTIVIEW : SKY_VERSION_BACKGROUND];

	RID default_shader_rd = sky_shader.get_default_shader_rd(is_multiview);

	RID texture_uniform_set;
	float border_size = 0.0;
	if (sky) {
		texture_uniform_set = sky->get_textures(
			SKY_TEXTURE_SET_BACKGROUND, default_shader_rd, is_multiview, p_render_buffers);
		border_size = sky->uv_border_size;
	}
	else {
		texture_uniform_set =
			sky_scene_state.get_fog_only_texture_uniform_set(default_shader_rd, is_multiview);
	}

	_render_sky(p_draw_list, p_time, p_fb, pipeline, material_data->uniform_set,
		texture_uniform_set, projection, sky_transform, sky_scene_state.cam_transform.origin,
		p_luminance_multiplier, p_brightness_multiplier, border_size);
}

void SkyRD::invalidate_sky(Sky* p_sky)
{
	if (!p_sky->dirty) {
		p_sky->dirty = true;
		p_sky->dirty_list = dirty_sky_list;
		dirty_sky_list = p_sky;
	}
}

void SkyRD::update_dirty_skys()
{
	bool use_raster_effect = (RendererRD::CopyEffects::get_raster_effects() &
								 RendererRD::CopyEffects::RASTER_EFFECT_OCTMAP) != 0;
	Sky* sky = dirty_sky_list;

	while (sky) {
		// update sky configuration if texture is missing

		// TODO See if we can move this into `update_radiance_buffers` and remove our dirty_sky
		// logic. As this is basically a duplicate of the logic in reflection probes we could move
		// this logic into RenderSceneBuffersRD and use that from both places.
		if (sky->radiance.is_null()) {
			int mipmaps = Image::get_image_required_mipmaps(
							  sky->radiance_size, sky->radiance_size, Image::FORMAT_RGBAH) +
						  1;

			int layers = roughness_layers;
			bool use_realtime =
				sky->mode == RSE::SKY_MODE_REALTIME || sky->internal_mode == RSE::SKY_MODE_REALTIME;
			if (use_realtime) {
				layers = Sky::REAL_TIME_ROUGHNESS_LAYERS;
			}

			if (sky_use_octmap_array) {
				mipmaps -= 2; //  reduce the number of mipmaps to keep the border size reasonable.
				// Double size to approximate texel density of cubemaps + add border for proper
				// filtering/mipmapping.
				uint32_t padding_pixels = (1 << (mipmaps - 1));
				uint32_t w = sky->radiance_size * 2 + padding_pixels * 2;
				uint32_t h = w;
				sky->uv_border_size = float(padding_pixels) / float(w);

				// Array (higher quality, more memory).
				RDC::TextureFormat tf;
				tf.array_layers = layers;
				tf.format = texture_format;
				tf.texture_type = RDC::TEXTURE_TYPE_2D_ARRAY;
				tf.mipmaps = mipmaps;
				tf.width = w;
				tf.height = h;
				tf.usage_bits =
					RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT | RDC::TEXTURE_USAGE_SAMPLING_BIT;
				if (!use_raster_effect) {
					tf.usage_bits |= RDC::TEXTURE_USAGE_STORAGE_BIT;
				}

				sky->radiance = RD::texture_create(tf, RDD::TextureView());

				// Create view into the first layer slice for user shaders.
				sky->radiance_first_layer_slice = RD::texture_create_shared_from_slice(
					RD::TextureView(), sky->radiance, 0, 0, mipmaps, RDC::TEXTURE_SLICE_2D, 1);

				sky->reflection.update_reflection_data(w, mipmaps, true, sky->radiance, 0,
					use_realtime, roughness_layers, texture_format, sky->uv_border_size);
			}
			else {
				// Double size to approximate texel density of cubemaps + add border for proper
				// filtering/mipmapping.
				uint32_t padding_pixels = (1 << (MIN(mipmaps, layers) - 1));
				uint32_t w = sky->radiance_size * 2 + padding_pixels * 2;
				uint32_t h = w;
				sky->uv_border_size = float(padding_pixels) / float(w);

				// Single texture (lower quality, less memory).
				RDC::TextureFormat tf;
				tf.format = texture_format;
				tf.mipmaps = MIN(mipmaps, layers);
				tf.width = w;
				tf.height = h;
				tf.usage_bits =
					RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT | RDC::TEXTURE_USAGE_SAMPLING_BIT;
				if (!use_raster_effect) {
					tf.usage_bits |= RDC::TEXTURE_USAGE_STORAGE_BIT;
				}

				sky->radiance = RD::texture_create(tf, RDD::TextureView());

				DEV_ASSERT(sky->radiance_first_layer_slice.is_null());

				sky->reflection.update_reflection_data(w, MIN(mipmaps, layers), false,
					sky->radiance, 0, use_realtime, roughness_layers, texture_format,
					sky->uv_border_size);
			}
		}

		sky->reflection.dirty = true;
		sky->processing_layer = 0;

		Sky* next = sky->dirty_list;
		sky->dirty_list = nullptr;
		sky->dirty = false;
		sky = next;
	}

	dirty_sky_list = nullptr;
}

SkyRD::SkyMaterialData* SkyRD::_get_sky_material_data(RID p_env)
{
	ERR_FAIL_COND_V(p_env.is_null(), nullptr);

	Sky* sky = get_sky(RendererSceneRender::environment_get_sky(p_env));
	RSE::EnvironmentBG background = RendererSceneRender::environment_get_background(p_env);

	SkyMaterialData* material_data = nullptr;
	RID sky_material;

	if (background == RSE::ENV_BG_CLEAR_COLOR || background == RSE::ENV_BG_COLOR) {
		sky_material = sky_scene_state.fog_material;
		material_data = static_cast<SkyMaterialData*>(MaterialStorage::material_get_data(
			sky_material, RendererRD::MaterialStorage::SHADER_TYPE_SKY));
	}
	else if (sky) {
		sky_material = sky_get_material(RendererSceneRender::environment_get_sky(p_env));

		if (sky_material.is_valid()) {
			material_data = static_cast<SkyMaterialData*>(MaterialStorage::material_get_data(
				sky_material, RendererRD::MaterialStorage::SHADER_TYPE_SKY));
			if (!material_data || !material_data->shader_data->valid) {
				material_data = nullptr;
			}
		}
	}

	if (!material_data) {
		sky_material = sky_shader.default_material;
		material_data = static_cast<SkyMaterialData*>(MaterialStorage::material_get_data(
			sky_material, RendererRD::MaterialStorage::SHADER_TYPE_SKY));
	}

	return material_data;
}

RID SkyRD::sky_get_material(RID p_sky) const
{
	Sky* sky = get_sky(p_sky);
	ERR_FAIL_NULL_V(sky, RID());

	return sky->material;
}

float SkyRD::sky_get_baked_exposure(RID p_sky) const
{
	Sky* sky = get_sky(p_sky);
	ERR_FAIL_NULL_V(sky, 1.0);

	return sky->baked_exposure;
}

RID SkyRD::allocate_sky_rid() { return sky_owner.allocate_rid(); }

void SkyRD::initialize_sky_rid(RID p_rid) { sky_owner.initialize_rid(p_rid, Sky()); }

SkyRD::Sky* SkyRD::get_sky(RID p_sky) const { return sky_owner.get_or_null(p_sky); }

void SkyRD::free_sky(RID p_sky)
{
	Sky* sky = get_sky(p_sky);
	ERR_FAIL_NULL(sky);

	sky->free();
	sky_owner.free(p_sky);
}

void SkyRD::sky_set_radiance_size(RID p_sky, int p_radiance_size)
{
	Sky* sky = get_sky(p_sky);
	ERR_FAIL_NULL(sky);

	if (sky->set_radiance_size(p_radiance_size)) {
		invalidate_sky(sky);
	}
}

int SkyRD::sky_get_radiance_size(RID p_sky) const
{
	Sky* sky = get_sky(p_sky);
	ERR_FAIL_NULL_V(sky, 0);

	return sky->get_radiance_size();
}

void SkyRD::sky_set_mode(RID p_sky, RSE::SkyMode p_mode)
{
	Sky* sky = get_sky(p_sky);
	ERR_FAIL_NULL(sky);

	if (sky->set_mode(p_mode)) {
		invalidate_sky(sky);
	}
}

void SkyRD::sky_set_material(RID p_sky, RID p_material)
{
	Sky* sky = get_sky(p_sky);
	ERR_FAIL_NULL(sky);

	if (sky->set_material(p_material)) {
		invalidate_sky(sky);
	}
}

Ref<Image> SkyRD::sky_bake_panorama(
	RID p_sky, float p_energy, bool p_bake_irradiance, const Size2i& p_size)
{
	Sky* sky = get_sky(p_sky);
	ERR_FAIL_NULL_V(sky, Ref<Image>());

	update_dirty_skys();

	return sky->bake_panorama(p_energy, p_bake_irradiance ? roughness_layers : 0, p_size);
}

RID SkyRD::sky_get_radiance_texture_rd(RID p_sky) const
{
	Sky* sky = get_sky(p_sky);
	ERR_FAIL_NULL_V(sky, RID());

	return sky->radiance;
}

float SkyRD::sky_get_uv_border_size(RID p_sky)
{
	Sky* sky = get_sky(p_sky);
	ERR_FAIL_NULL_V(sky, 1.0);

	return sky->uv_border_size;
}

void RendererRD::SkyRD::ReflectionData::update_reflection_data(int p_size, int p_mipmaps,
	bool p_use_float, RID p_radiance_texture, int p_roughness_layers, bool p_use_fast_filter,
	int p_sample_count, RenderingDeviceCommons::DataFormat p_format, float p_intensity)
{
}

void RendererRD::SkyRD::ReflectionData::create_reflection_importance_sample(
	bool p_use_float, int p_sample_count, unsigned int p_layers)
{
}

void RendererRD::SkyRD::ReflectionData::create_reflection_fast_filter(bool p_use_float) {}

void RendererRD::SkyRD::update_res_buffers(Ref<RenderSceneBuffersRD> p_render_buffers, RID p_sky,
	double p_time, float p_step, float p_luminance_multiplier)
{
}

void RendererRD::SkyRD::update_radiance_buffers(Ref<RenderSceneBuffersRD> p_render_buffers,
	RID p_sky, const Vector3& p_cam_pos, double p_time, float p_step, float p_luminance_multiplier)
{
}

RID RendererRD::SkyRD::SkyShader::get_default_shader_rd(bool p_wireframe) { return RID(); }

RendererRD::SkyRD::SkyRD() {}

void RendererRD::SkyRD::SkyShaderData::set_code(const String& p_code) {}

void SkyRD::init()
{
	ShaderCompiler::DefaultIdentifierActions actions;

	actions.renames["COLOR"] = "color";
	actions.renames["ALPHA"] = "alpha";
	actions.renames["EYEDIR"] = "eye_dir";
	actions.renames["POSITION"] = "position";
	actions.renames["TIME"] = "time";
	actions.renames["PI"] = "3.14159265358979323846";
	actions.renames["TAU"] = "6.28318530717958647692";
	actions.renames["E"] = "2.71828182845904523536";
	actions.renames["HALF_RES_COLOR"] = "half_res_color";
	actions.renames["QUARTER_RES_COLOR"] = "quarter_res_color";
	actions.renames["RADIANCE"] = "radiance";
	actions.renames["FOG"] = "custom_fog";
	actions.renames["SKY_COORDS"] = "sky_coords";
	actions.renames["SCREEN_UV"] = "screen_uv";

	actions.renames["LIGHT0_DIRECTION"] = "light_directional_data[0].direction";
	actions.renames["LIGHT0_COLOR"] = "light_directional_data[0].color";
	actions.renames["LIGHT0_ENERGY"] = "light_directional_data[0].energy";
	actions.renames["LIGHT0_SIZE"] = "light_directional_data[0].size";
	actions.renames["LIGHT0_ENABLED"] = "light_directional_data[0].enabled";

	actions.renames["LIGHT1_DIRECTION"] = "light_directional_data[1].direction";
	actions.renames["LIGHT1_COLOR"] = "light_directional_data[1].color";
	actions.renames["LIGHT1_ENERGY"] = "light_directional_data[1].energy";
	actions.renames["LIGHT1_SIZE"] = "light_directional_data[1].size";
	actions.renames["LIGHT1_ENABLED"] = "light_directional_data[1].enabled";

	actions.renames["LIGHT2_DIRECTION"] = "light_directional_data[2].direction";
	actions.renames["LIGHT2_COLOR"] = "light_directional_data[2].color";
	actions.renames["LIGHT2_ENERGY"] = "light_directional_data[2].energy";
	actions.renames["LIGHT2_SIZE"] = "light_directional_data[2].size";
	actions.renames["LIGHT2_ENABLED"] = "light_directional_data[2].enabled";

	actions.renames["LIGHT3_DIRECTION"] = "light_directional_data[3].direction";
	actions.renames["LIGHT3_COLOR"] = "light_directional_data[3].color";
	actions.renames["LIGHT3_ENERGY"] = "light_directional_data[3].energy";
	actions.renames["LIGHT3_SIZE"] = "light_directional_data[3].size";
	actions.renames["LIGHT3_ENABLED"] = "light_directional_data[3].enabled";

	actions.render_mode_defines["use_half_res"] = "#define USE_HALF_RES\n";
	actions.render_mode_defines["use_quarter_res"] = "#define USE_QUARTER_RES\n";
	actions.render_mode_defines["disable_fog"] = "#define DISABLE_FOG\n";
	actions.render_mode_defines["use_debanding"] = "#define USE_DEBANDING\n";

	sky_shader.compiler.initialize(actions);

	MaterialStorage::shader_set_data_request_function(
		MaterialStorage::SHADER_TYPE_SKY, _create_sky_shader_funcs);
	MaterialStorage::material_set_data_request_function(
		MaterialStorage::SHADER_TYPE_SKY, _create_sky_material_funcs);

	sky_shader.default_shader = MaterialStorage::shader_allocate();
	MaterialStorage::shader_initialize(sky_shader.default_shader);
	MaterialStorage::shader_set_code(sky_shader.default_shader,
		R"(
shader_type sky;

void sky() {
	COLOR = vec3(0.0);
}
)");

	sky_shader.default_material = MaterialStorage::material_allocate();
	MaterialStorage::material_initialize(sky_shader.default_material);
	MaterialStorage::material_set_shader(
		sky_shader.default_material, sky_shader.default_shader);
}
#undef RB_SCOPE_SKY
#undef RB_HALF_TEXTURE
#undef RB_QUARTER_TEXTURE

// --- Merged from servers/rendering/renderer_rd/environment/gi.cpp ---

using namespace RendererRD;

const Vector3i GI::SDFGI::Cascade::DIRTY_ALL = Vector3i(0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF);

RID GI::voxel_gi_allocate() { return voxel_gi_owner.allocate_rid(); }

void GI::voxel_gi_free(RID p_voxel_gi)
{
	voxel_gi_allocate_data(p_voxel_gi, Transform3D(), AABB(), Vector3i(), Vector<uint8_t>(),
		Vector<uint8_t>(), Vector<uint8_t>(), Vector<int>()); // deallocate
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	voxel_gi->dependency.deleted_notify(p_voxel_gi);
	voxel_gi_owner.free(p_voxel_gi);
}

void GI::voxel_gi_initialize(RID p_voxel_gi)
{
	voxel_gi_owner.initialize_rid(p_voxel_gi, VoxelGI());
}

void GI::voxel_gi_allocate_data(RID p_voxel_gi, const Transform3D& p_to_cell_xform,
	const AABB& p_aabb, const Vector3i& p_octree_size, const Vector<uint8_t>& p_octree_cells,
	const Vector<uint8_t>& p_data_cells, const Vector<uint8_t>& p_distance_field,
	const Vector<int>& p_level_counts)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL(voxel_gi);

	if (voxel_gi->octree_buffer.is_valid()) {
		RD::free_rid(voxel_gi->octree_buffer);
		RD::free_rid(voxel_gi->data_buffer);
		if (voxel_gi->sdf_texture.is_valid()) {
			RD::free_rid(voxel_gi->sdf_texture);
		}

		voxel_gi->sdf_texture = RID();
		voxel_gi->octree_buffer = RID();
		voxel_gi->data_buffer = RID();
		voxel_gi->octree_buffer_size = 0;
		voxel_gi->data_buffer_size = 0;
		voxel_gi->cell_count = 0;
	}

	voxel_gi->to_cell_xform = p_to_cell_xform;
	voxel_gi->bounds = p_aabb;
	voxel_gi->octree_size = p_octree_size;
	voxel_gi->level_counts = p_level_counts;

	if (p_octree_cells.size()) {
		ERR_FAIL_COND(p_octree_cells.size() % 32 != 0); // cells size must be a multiple of 32

		uint32_t cell_count = p_octree_cells.size() / 32;

		ERR_FAIL_COND(p_data_cells.size() != (int)cell_count * 16); // see that data size matches

		voxel_gi->cell_count = cell_count;
		voxel_gi->octree_buffer = RD::storage_buffer_create(p_octree_cells.size(), p_octree_cells);
		voxel_gi->octree_buffer_size = p_octree_cells.size();
		voxel_gi->data_buffer = RD::storage_buffer_create(p_data_cells.size(), p_data_cells);
		voxel_gi->data_buffer_size = p_data_cells.size();

		if (p_distance_field.size()) {
			RDC::TextureFormat tf;
			tf.format = RDC::DATA_FORMAT_R8_UNORM;
			tf.width = voxel_gi->octree_size.x;
			tf.height = voxel_gi->octree_size.y;
			tf.depth = voxel_gi->octree_size.z;
			tf.texture_type = RDC::TEXTURE_TYPE_3D;
			tf.usage_bits = RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_CAN_UPDATE_BIT |
							RDC::TEXTURE_USAGE_CAN_COPY_FROM_BIT;
			Vector<Vector<uint8_t>> s;
			s.push_back(p_distance_field);
			voxel_gi->sdf_texture = RD::texture_create(tf, RDD::TextureView(), s);
			RD::set_resource_name(voxel_gi->sdf_texture, "VoxelGI SDF Texture");
		}
#if 0
			{
				RD::TextureFormat tf;
				tf.format = RD::DATA_FORMAT_R8_UNORM;
				tf.width = voxel_gi->octree_size.x;
				tf.height = voxel_gi->octree_size.y;
				tf.depth = voxel_gi->octree_size.z;
				tf.type = RD::TEXTURE_TYPE_3D;
				tf.usage_bits = RD::TEXTURE_USAGE_SAMPLING_BIT | RD::TEXTURE_USAGE_STORAGE_BIT | RD::TEXTURE_USAGE_CAN_COPY_TO_BIT;
				tf.shareable_formats.push_back(RD::DATA_FORMAT_R8_UNORM);
				tf.shareable_formats.push_back(RD::DATA_FORMAT_R8_UINT);
				voxel_gi->sdf_texture = RD::texture_create(tf, RD::TextureView());
				RD::set_resource_name(voxel_gi->sdf_texture, "VoxelGI SDF Texture");
			}
			RID shared_tex;
			{
				RD::TextureView tv;
				tv.format_override = RD::DATA_FORMAT_R8_UINT;
				shared_tex = RD::texture_create_shared(tv, voxel_gi->sdf_texture);
			}
			//update SDF texture
			Vector<RD::Uniform> uniforms;
			{
				RD::Uniform u;
				u.uniform_type = RD::UNIFORM_TYPE_STORAGE_BUFFER;
				u.binding = 1;
				u.append_id(voxel_gi->octree_buffer);
				uniforms.push_back(u);
			}
			{
				RD::Uniform u;
				u.uniform_type = RD::UNIFORM_TYPE_STORAGE_BUFFER;
				u.binding = 2;
				u.append_id(voxel_gi->data_buffer);
				uniforms.push_back(u);
			}
			{
				RD::Uniform u;
				u.uniform_type = RD::UNIFORM_TYPE_IMAGE;
				u.binding = 3;
				u.append_id(shared_tex);
				uniforms.push_back(u);
			}

			RID uniform_set = RD::uniform_set_create(uniforms, voxel_gi_sdf_shader_version_shader, 0);

			{
				uint32_t push_constant[4] = { 0, 0, 0, 0 };

				for (int i = 0; i < voxel_gi->level_counts.size() - 1; i++) {
					push_constant[0] += voxel_gi->level_counts[i];
				}
				push_constant[1] = push_constant[0] + voxel_gi->level_counts[voxel_gi->level_counts.size() - 1];

				print_line("offset: " + itos(push_constant[0]));
				print_line("size: " + itos(push_constant[1]));
				//create SDF
				RD::ComputeListID compute_list = RD::compute_list_begin();
				RD::compute_list_bind_compute_pipeline(compute_list, voxel_gi_sdf_shader_pipeline);
				RD::compute_list_bind_uniform_set(compute_list, uniform_set, 0);
				RD::compute_list_set_push_constant(compute_list, push_constant, sizeof(uint32_t) * 4);
				RD::compute_list_dispatch(compute_list, voxel_gi->octree_size.x / 4, voxel_gi->octree_size.y / 4, voxel_gi->octree_size.z / 4);
				RD::compute_list_end();
			}

			RD::free(uniform_set);
			RD::free(shared_tex);
		}
#endif
	}

	voxel_gi->version++;
	voxel_gi->data_version++;

	voxel_gi->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_AABB);
}

AABB GI::voxel_gi_get_bounds(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, AABB());

	return voxel_gi->bounds;
}

Vector3i GI::voxel_gi_get_octree_size(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, Vector3i());
	return voxel_gi->octree_size;
}

Vector<uint8_t> GI::voxel_gi_get_octree_cells(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, Vector<uint8_t>());

	if (voxel_gi->octree_buffer.is_valid()) {
		return RD::buffer_get_data(voxel_gi->octree_buffer);
	}
	return Vector<uint8_t>();
}

Vector<uint8_t> GI::voxel_gi_get_data_cells(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, Vector<uint8_t>());

	if (voxel_gi->data_buffer.is_valid()) {
		return RD::buffer_get_data(voxel_gi->data_buffer);
	}
	return Vector<uint8_t>();
}

Vector<uint8_t> GI::voxel_gi_get_distance_field(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, Vector<uint8_t>());

	if (voxel_gi->data_buffer.is_valid()) {
		return RD::texture_get_data(voxel_gi->sdf_texture, 0);
	}
	return Vector<uint8_t>();
}

Vector<int> GI::voxel_gi_get_level_counts(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, Vector<int>());

	return voxel_gi->level_counts;
}

Transform3D GI::voxel_gi_get_to_cell_xform(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, Transform3D());

	return voxel_gi->to_cell_xform;
}

void GI::voxel_gi_set_dynamic_range(RID p_voxel_gi, float p_range)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->dynamic_range = p_range;
	voxel_gi->version++;
}

float GI::voxel_gi_get_dynamic_range(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, 0);

	return voxel_gi->dynamic_range;
}

void GI::voxel_gi_set_propagation(RID p_voxel_gi, float p_range)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->propagation = p_range;
	voxel_gi->version++;
}

float GI::voxel_gi_get_propagation(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, 0);
	return voxel_gi->propagation;
}

void GI::voxel_gi_set_energy(RID p_voxel_gi, float p_energy)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->energy = p_energy;
}

float GI::voxel_gi_get_energy(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, 0);
	return voxel_gi->energy;
}

void GI::voxel_gi_set_baked_exposure_normalization(RID p_voxel_gi, float p_baked_exposure)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->baked_exposure = p_baked_exposure;
}

float GI::voxel_gi_get_baked_exposure_normalization(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, 0);
	return voxel_gi->baked_exposure;
}

void GI::voxel_gi_set_bias(RID p_voxel_gi, float p_bias)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->bias = p_bias;
}

float GI::voxel_gi_get_bias(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, 0);
	return voxel_gi->bias;
}

void GI::voxel_gi_set_normal_bias(RID p_voxel_gi, float p_normal_bias)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->normal_bias = p_normal_bias;
}

float GI::voxel_gi_get_normal_bias(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, 0);
	return voxel_gi->normal_bias;
}

void GI::voxel_gi_set_interior(RID p_voxel_gi, bool p_enable)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->interior = p_enable;
}

void GI::voxel_gi_set_use_two_bounces(RID p_voxel_gi, bool p_enable)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->use_two_bounces = p_enable;
	voxel_gi->version++;
}

bool GI::voxel_gi_is_using_two_bounces(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, false);
	return voxel_gi->use_two_bounces;
}

bool GI::voxel_gi_is_interior(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, false);
	return voxel_gi->interior;
}

uint32_t GI::voxel_gi_get_version(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, 0);
	return voxel_gi->version;
}

uint32_t GI::voxel_gi_get_data_version(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, 0);
	return voxel_gi->data_version;
}

RID GI::voxel_gi_get_octree_buffer(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, RID());
	return voxel_gi->octree_buffer;
}

RID GI::voxel_gi_get_data_buffer(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, RID());
	return voxel_gi->data_buffer;
}

RID GI::voxel_gi_get_sdf_texture(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, RID());

	return voxel_gi->sdf_texture;
}

Dependency* GI::voxel_gi_get_dependency(RID p_voxel_gi)
{
	VoxelGI* voxel_gi = voxel_gi_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL_V(voxel_gi, nullptr);

	return &voxel_gi->dependency;
}

void GI::sdfgi_reset() { sdfgi_current_version++; }

static RID create_clear_texture(const RDC::TextureFormat& p_format, const String& p_name)
{
	RID texture = RD::texture_create(p_format, RDD::TextureView());
	ERR_FAIL_COND_V_MSG(texture.is_null(), RID(), String("Cannot create texture: ") + p_name);

	RD::set_resource_name(texture, p_name);
	RD::texture_clear(texture, Color(0, 0, 0, 0), 0, p_format.mipmaps, 0, p_format.array_layers);

	return texture;
}

void GI::SDFGI::free_data()
{
	// we don't free things here, we handle SDFGI differently at the moment destructing the object
	// when it needs to change.
}

GI::SDFGI::~SDFGI()
{
	for (const SDFGI::Cascade& c : cascades) {
		RD::free_rid(c.light_data);
		RD::free_rid(c.light_aniso_0_tex);
		RD::free_rid(c.light_aniso_1_tex);
		RD::free_rid(c.sdf_tex);
		RD::free_rid(c.solid_cell_dispatch_buffer_storage);
		RD::free_rid(c.solid_cell_dispatch_buffer_call);
		RD::free_rid(c.solid_cell_buffer);
		RD::free_rid(c.lightprobe_history_tex);
		RD::free_rid(c.lightprobe_average_tex);
		RD::free_rid(c.lights_buffer);
	}

	RD::free_rid(render_albedo);
	RD::free_rid(render_emission);
	RD::free_rid(render_emission_aniso);

	RD::free_rid(render_sdf[0]);
	RD::free_rid(render_sdf[1]);

	RD::free_rid(render_sdf_half[0]);
	RD::free_rid(render_sdf_half[1]);

	for (int i = 0; i < 8; i++) {
		RD::free_rid(render_occlusion[i]);
	}

	RD::free_rid(render_geom_facing);

	RD::free_rid(lightprobe_data);
	RD::free_rid(lightprobe_history_scroll);
	RD::free_rid(lightprobe_average_scroll);
	RD::free_rid(occlusion_data);
	RD::free_rid(ambient_texture);

	RD::free_rid(cascades_ubo);

	for (uint32_t v = 0; v < RendererSceneRender::MAX_RENDER_VIEWS; v++) {
		if (RD::uniform_set_is_valid(debug_uniform_set[v])) {
			RD::free_rid(debug_uniform_set[v]);
		}
		debug_uniform_set[v] = RID();
	}

	if (RD::uniform_set_is_valid(debug_probes_uniform_set)) {
		RD::free_rid(debug_probes_uniform_set);
	}
	debug_probes_uniform_set = RID();

	if (debug_probes_scene_data_ubo.is_valid()) {
		RD::free_rid(debug_probes_scene_data_ubo);
		debug_probes_scene_data_ubo = RID();
	}
}

void GI::SDFGI::update(RID p_env, const Vector3& p_world_position)
{
	bounce_feedback = RendererSceneRender::environment_get_sdfgi_bounce_feedback(p_env);
	energy = RendererSceneRender::environment_get_sdfgi_energy(p_env);
	normal_bias = RendererSceneRender::environment_get_sdfgi_normal_bias(p_env);
	probe_bias = RendererSceneRender::environment_get_sdfgi_probe_bias(p_env);
	reads_sky = RendererSceneRender::environment_get_sdfgi_read_sky_light(p_env);

	int32_t drag_margin = (cascade_size / SDFGI::PROBE_DIVISOR) / 2;

	for (SDFGI::Cascade& cascade : cascades) {
		cascade.dirty_regions = Vector3i();

		Vector3 probe_half_size =
			Vector3(1, 1, 1) * cascade.cell_size * float(cascade_size / SDFGI::PROBE_DIVISOR) * 0.5;
		probe_half_size = Vector3(0, 0, 0);

		Vector3 world_position = p_world_position;
		world_position.y *= y_mult;
		Vector3i pos_in_cascade = Vector3i((world_position + probe_half_size) / cascade.cell_size);

		for (int j = 0; j < 3; j++) {
			if (pos_in_cascade[j] < cascade.position[j]) {
				while (pos_in_cascade[j] < (cascade.position[j] - drag_margin)) {
					cascade.position[j] -= drag_margin * 2;
					cascade.dirty_regions[j] += drag_margin * 2;
				}
			}
			else if (pos_in_cascade[j] > cascade.position[j]) {
				while (pos_in_cascade[j] > (cascade.position[j] + drag_margin)) {
					cascade.position[j] += drag_margin * 2;
					cascade.dirty_regions[j] -= drag_margin * 2;
				}
			}

			if (cascade.dirty_regions[j] == 0) {
				continue; // not dirty
			}
			else if (uint32_t(Math::abs(cascade.dirty_regions[j])) >= cascade_size) {
				// moved too much, just redraw everything (make all dirty)
				cascade.dirty_regions = SDFGI::Cascade::DIRTY_ALL;
				break;
			}
		}

		if (cascade.dirty_regions != Vector3i() &&
			cascade.dirty_regions != SDFGI::Cascade::DIRTY_ALL) {
			// see how much the total dirty volume represents from the total volume
			uint32_t total_volume = cascade_size * cascade_size * cascade_size;
			uint32_t safe_volume = 1;
			for (int j = 0; j < 3; j++) {
				safe_volume *= cascade_size - Math::abs(cascade.dirty_regions[j]);
			}
			uint32_t dirty_volume = total_volume - safe_volume;
			if (dirty_volume > (safe_volume / 2)) {
				// more than half the volume is dirty, make all dirty so its only rendered once
				cascade.dirty_regions = SDFGI::Cascade::DIRTY_ALL;
			}
		}
	}
}

int GI::SDFGI::get_pending_region_data(
	int p_region, Vector3i& r_local_offset, Vector3i& r_local_size, AABB& r_bounds) const
{
	int dirty_count = 0;
	for (uint32_t i = 0; i < cascades.size(); i++) {
		const SDFGI::Cascade& c = cascades[i];

		if (c.dirty_regions == SDFGI::Cascade::DIRTY_ALL) {
			if (dirty_count == p_region) {
				r_local_offset = Vector3i();
				r_local_size = Vector3i(1, 1, 1) * cascade_size;

				r_bounds.position =
					Vector3((Vector3i(1, 1, 1) * -int32_t(cascade_size >> 1) + c.position)) *
					c.cell_size * Vector3(1, 1.0 / y_mult, 1);
				r_bounds.size = Vector3(r_local_size) * c.cell_size * Vector3(1, 1.0 / y_mult, 1);
				return i;
			}
			dirty_count++;
		}
		else {
			for (int j = 0; j < 3; j++) {
				if (c.dirty_regions[j] != 0) {
					if (dirty_count == p_region) {
						Vector3i from = Vector3i(0, 0, 0);
						Vector3i to = Vector3i(1, 1, 1) * cascade_size;

						if (c.dirty_regions[j] > 0) {
							// fill from the beginning
							to[j] = c.dirty_regions[j];
						}
						else {
							// fill from the end
							from[j] = to[j] + c.dirty_regions[j];
						}

						for (int k = 0; k < j; k++) {
							// "chip" away previous regions to avoid re-voxelizing the same thing
							if (c.dirty_regions[k] > 0) {
								from[k] += c.dirty_regions[k];
							}
							else if (c.dirty_regions[k] < 0) {
								to[k] += c.dirty_regions[k];
							}
						}

						r_local_offset = from;
						r_local_size = to - from;

						r_bounds.position =
							Vector3(from + Vector3i(1, 1, 1) * -int32_t(cascade_size >> 1) +
									c.position) *
							c.cell_size * Vector3(1, 1.0 / y_mult, 1);
						r_bounds.size =
							Vector3(r_local_size) * c.cell_size * Vector3(1, 1.0 / y_mult, 1);

						return i;
					}

					dirty_count++;
				}
			}
		}
	}
	return -1;
}

void GI::SDFGI::update_cascades()
{
	// update cascades
	SDFGI::Cascade::UBO cascade_data[SDFGI::MAX_CASCADES];
	int32_t probe_divisor = cascade_size / SDFGI::PROBE_DIVISOR;

	for (uint32_t i = 0; i < cascades.size(); i++) {
		Vector3 pos =
			Vector3((Vector3i(1, 1, 1) * -int32_t(cascade_size >> 1) + cascades[i].position)) *
			cascades[i].cell_size;

		cascade_data[i].offset[0] = pos.x;
		cascade_data[i].offset[1] = pos.y;
		cascade_data[i].offset[2] = pos.z;
		cascade_data[i].to_cell = 1.0 / cascades[i].cell_size;
		cascade_data[i].probe_offset[0] = cascades[i].position.x / probe_divisor;
		cascade_data[i].probe_offset[1] = cascades[i].position.y / probe_divisor;
		cascade_data[i].probe_offset[2] = cascades[i].position.z / probe_divisor;
		cascade_data[i].pad = 0;
	}
}

void GI::SDFGI::pre_process_gi(const Transform3D& p_transform, RenderDataRD* p_render_data)
{
	if (p_render_data->sdfgi_update_data == nullptr) {
		return;
	}

	/* Update general SDFGI Buffer */

	SDFGIData sdfgi_data;

	sdfgi_data.grid_size[0] = cascade_size;
	sdfgi_data.grid_size[1] = cascade_size;
	sdfgi_data.grid_size[2] = cascade_size;

	sdfgi_data.max_cascades = cascades.size();
	sdfgi_data.probe_axis_size = probe_axis_count;
	sdfgi_data.cascade_probe_size[0] =
		sdfgi_data.probe_axis_size - 1; // float version for performance
	sdfgi_data.cascade_probe_size[1] = sdfgi_data.probe_axis_size - 1;
	sdfgi_data.cascade_probe_size[2] = sdfgi_data.probe_axis_size - 1;

	float csize = cascade_size;
	sdfgi_data.probe_to_uvw = 1.0 / float(sdfgi_data.cascade_probe_size[0]);
	sdfgi_data.use_occlusion = uses_occlusion;
	// sdfgi_data.energy = energy;

	sdfgi_data.y_mult = y_mult;

	float cascade_voxel_size = (csize / sdfgi_data.cascade_probe_size[0]);
	float occlusion_clamp = (cascade_voxel_size - 0.5) / cascade_voxel_size;
	sdfgi_data.occlusion_clamp[0] = occlusion_clamp;
	sdfgi_data.occlusion_clamp[1] = occlusion_clamp;
	sdfgi_data.occlusion_clamp[2] = occlusion_clamp;
	sdfgi_data.normal_bias = (normal_bias / csize) * sdfgi_data.cascade_probe_size[0];

	// vec2 tex_pixel_size = 1.0 / vec2(ivec2( (OCT_SIZE+2) * params.probe_axis_size *
	// params.probe_axis_size, (OCT_SIZE+2) * params.probe_axis_size ) ); vec3 probe_uv_offset =
	// (ivec3(OCT_SIZE+2,OCT_SIZE+2,(OCT_SIZE+2) * params.probe_axis_size)) * tex_pixel_size.xyx;

	uint32_t oct_size = SDFGI::LIGHTPROBE_OCT_SIZE;

	sdfgi_data.lightprobe_tex_pixel_size[0] =
		1.0 / ((oct_size + 2) * sdfgi_data.probe_axis_size * sdfgi_data.probe_axis_size);
	sdfgi_data.lightprobe_tex_pixel_size[1] = 1.0 / ((oct_size + 2) * sdfgi_data.probe_axis_size);
	sdfgi_data.lightprobe_tex_pixel_size[2] = 1.0;

	sdfgi_data.energy = energy;

	sdfgi_data.lightprobe_uv_offset[0] =
		float(oct_size + 2) * sdfgi_data.lightprobe_tex_pixel_size[0];
	sdfgi_data.lightprobe_uv_offset[1] =
		float(oct_size + 2) * sdfgi_data.lightprobe_tex_pixel_size[1];
	sdfgi_data.lightprobe_uv_offset[2] = float((oct_size + 2) * sdfgi_data.probe_axis_size) *
										 sdfgi_data.lightprobe_tex_pixel_size[0];

	sdfgi_data.occlusion_renormalize[0] = 0.5;
	sdfgi_data.occlusion_renormalize[1] = 1.0;
	sdfgi_data.occlusion_renormalize[2] = 1.0 / float(sdfgi_data.max_cascades);

	int32_t probe_divisor = cascade_size / SDFGI::PROBE_DIVISOR;

	for (uint32_t i = 0; i < sdfgi_data.max_cascades; i++) {
		SDFGIData::ProbeCascadeData& c = sdfgi_data.cascades[i];
		Vector3 pos =
			Vector3((Vector3i(1, 1, 1) * -int32_t(cascade_size >> 1) + cascades[i].position)) *
			cascades[i].cell_size;
		Vector3 cam_origin = p_transform.origin;
		cam_origin.y *= y_mult;
		pos -= cam_origin; // make pos local to camera, to reduce numerical error
		c.position[0] = pos.x;
		c.position[1] = pos.y;
		c.position[2] = pos.z;
		c.to_probe =
			1.0 / (float(cascade_size) * cascades[i].cell_size / float(probe_axis_count - 1));

		Vector3i probe_ofs = cascades[i].position / probe_divisor;
		c.probe_world_offset[0] = probe_ofs.x;
		c.probe_world_offset[1] = probe_ofs.y;
		c.probe_world_offset[2] = probe_ofs.z;

		c.to_cell = 1.0 / cascades[i].cell_size;
		c.exposure_normalization = 1.0;
		if (p_render_data->camera_attributes.is_valid()) {
			float exposure_normalization =
				RendererCameraAttributes::camera_attributes_get_exposure_normalization_factor(
					p_render_data->camera_attributes);
			c.exposure_normalization =
				exposure_normalization / cascades[i].baked_exposure_normalization;
		}
	}

	/* Update dynamic lights in SDFGI cascades */

	for (uint32_t i = 0; i < cascades.size(); i++) {
		SDFGI::Cascade& cascade = cascades[i];

		SDFGIShader::Light lights[SDFGI::MAX_DYNAMIC_LIGHTS];
		uint32_t idx = 0;
		for (uint32_t j = 0;
			 j < (uint32_t)p_render_data->sdfgi_update_data->directional_lights->size(); j++) {
			if (idx == SDFGI::MAX_DYNAMIC_LIGHTS) {
				break;
			}

			RID light_instance = p_render_data->sdfgi_update_data->directional_lights->get(j);
			ERR_CONTINUE(!RendererRD::LightStorage::owns_light_instance(light_instance));

			RID light = RendererRD::LightStorage::light_instance_get_base_light(light_instance);
			Transform3D light_transform =
				RendererRD::LightStorage::light_instance_get_base_transform(light_instance);

			if (RendererLightStorage::light_directional_get_sky_mode(light) ==
				RSE::LIGHT_DIRECTIONAL_SKY_MODE_SKY_ONLY) {
				continue;
			}

			Vector3 dir = -light_transform.basis.get_column(Vector3::AXIS_Z);
			dir.y *= y_mult;
			dir.normalize();
			lights[idx].direction[0] = dir.x;
			lights[idx].direction[1] = dir.y;
			lights[idx].direction[2] = dir.z;
			Color color = RendererLightStorage::light_get_color(light);
			color = color.srgb_to_linear();
			lights[idx].color[0] = color.r;
			lights[idx].color[1] = color.g;
			lights[idx].color[2] = color.b;
			lights[idx].type = RSE::LIGHT_DIRECTIONAL;
			lights[idx].energy =
				RendererLightStorage::light_get_param(light, RSE::LIGHT_PARAM_ENERGY) *
				RendererLightStorage::light_get_param(light, RSE::LIGHT_PARAM_INDIRECT_ENERGY);
			if (RendererSceneRenderRD::is_using_physical_light_units()) {
				lights[idx].energy *=
					RendererLightStorage::light_get_param(light, RSE::LIGHT_PARAM_INTENSITY);
			}

			if (p_render_data->camera_attributes.is_valid()) {
				lights[idx].energy *=
					RendererCameraAttributes::camera_attributes_get_exposure_normalization_factor(
						p_render_data->camera_attributes);
			}

			lights[idx].has_shadow = RendererLightStorage::light_has_shadow(light);

			idx++;
		}

		AABB cascade_aabb;
		cascade_aabb.position =
			Vector3((Vector3i(1, 1, 1) * -int32_t(cascade_size >> 1) + cascade.position)) *
			cascade.cell_size;
		cascade_aabb.size = Vector3(1, 1, 1) * cascade_size * cascade.cell_size;

		for (uint32_t j = 0; j < p_render_data->sdfgi_update_data->positional_light_count; j++) {
			if (idx == SDFGI::MAX_DYNAMIC_LIGHTS) {
				break;
			}

			RID light_instance = p_render_data->sdfgi_update_data->positional_light_instances[j];
			ERR_CONTINUE(!RendererRD::LightStorage::owns_light_instance(light_instance));

			RID light = RendererRD::LightStorage::light_instance_get_base_light(light_instance);
			AABB light_aabb =
				RendererRD::LightStorage::light_instance_get_base_aabb(light_instance);
			Transform3D light_transform =
				RendererRD::LightStorage::light_instance_get_base_transform(light_instance);

			uint32_t max_sdfgi_cascade = RendererLightStorage::light_get_max_sdfgi_cascade(light);
			if (i > max_sdfgi_cascade) {
				continue;
			}

			if (!cascade_aabb.intersects(light_aabb)) {
				continue;
			}

			Vector3 dir = -light_transform.basis.get_column(Vector3::AXIS_Z);
			Vector2 area_size = RendererLightStorage::light_area_get_size(light);
			// faster to not do this here
			// dir.y *= y_mult;
			// dir.normalize();
			lights[idx].direction[0] = dir.x;
			lights[idx].direction[1] = dir.y;
			lights[idx].direction[2] = dir.z;
			Vector3 pos = light_transform.origin;
			pos.y *= y_mult;
			lights[idx].position[0] = pos.x;
			lights[idx].position[1] = pos.y;
			lights[idx].position[2] = pos.z;
			Color color = RendererLightStorage::light_get_color(light);
			color = color.srgb_to_linear();
			lights[idx].color[0] = color.r;
			lights[idx].color[1] = color.g;
			lights[idx].color[2] = color.b;
			lights[idx].type = RendererLightStorage::light_get_type(light);

			lights[idx].energy =
				RendererLightStorage::light_get_param(light, RSE::LIGHT_PARAM_ENERGY) *
				RendererLightStorage::light_get_param(light, RSE::LIGHT_PARAM_INDIRECT_ENERGY);
			if (RendererSceneRenderRD::is_using_physical_light_units()) {
				lights[idx].energy *=
					RendererLightStorage::light_get_param(light, RSE::LIGHT_PARAM_INTENSITY);

				// Convert from Luminous Power to Luminous Intensity
				if (lights[idx].type == RSE::LIGHT_OMNI) {
					lights[idx].energy *= 1.0 / (Math::PI * 4.0);
				}
				else if (lights[idx].type == RSE::LIGHT_SPOT) {
					// Spot Lights are not physically accurate, Luminous Intensity should change in
					// relation to the cone angle. We make this assumption to keep them easy to
					// control.
					lights[idx].energy *= 1.0 / Math::PI;
				}
			}

			if (p_render_data->camera_attributes.is_valid()) {
				lights[idx].energy *=
					RendererCameraAttributes::camera_attributes_get_exposure_normalization_factor(
						p_render_data->camera_attributes);
			}

			lights[idx].has_shadow = RendererLightStorage::light_has_shadow(light);
			lights[idx].attenuation =
				RendererLightStorage::light_get_param(light, RSE::LIGHT_PARAM_ATTENUATION);
			lights[idx].radius =
				RendererLightStorage::light_get_param(light, RSE::LIGHT_PARAM_RANGE);
			lights[idx].cos_spot_angle = Math::cos(Math::deg_to_rad(
				RendererLightStorage::light_get_param(light, RSE::LIGHT_PARAM_SPOT_ANGLE)));
			lights[idx].inv_spot_attenuation = 1.0f / RendererLightStorage::light_get_param(
														  light, RSE::LIGHT_PARAM_SPOT_ATTENUATION);

			if (lights[idx].type == RSE::LIGHT_AREA) {
				Vector3 area_vec_a = light_transform.basis.get_column(0).normalized() * area_size.x;
				Vector3 area_vec_b = light_transform.basis.get_column(1).normalized() * area_size.y;
				Rect2 proj_rect = RendererRD::TextureStorage::area_light_atlas_get_texture_rect(
					RendererLightStorage::light_area_get_texture(light));
				lights[idx].area_width[0] = area_vec_a.x;
				lights[idx].area_width[1] = area_vec_a.y;
				lights[idx].area_width[2] = area_vec_a.z;
				lights[idx].area_height[0] = area_vec_b.x;
				lights[idx].area_height[1] = area_vec_b.y;
				lights[idx].area_height[2] = area_vec_b.z;
				lights[idx].area_projector_rect[0] = proj_rect.position.x;
				lights[idx].area_projector_rect[1] = proj_rect.position.y;
				lights[idx].area_projector_rect[2] = proj_rect.size.x;
				lights[idx].area_projector_rect[3] = proj_rect.size.y;

				Size2i texture_size =
					proj_rect.size * RendererRD::TextureStorage::area_light_atlas_get_size();
				lights[idx].cos_spot_angle =
					MIN(Math::floor(Math::log2(MAX(MIN(texture_size.x, texture_size.y), 1.0f))),
						RendererRD::TextureStorage::area_light_atlas_get_mipmaps()) -
					1.0f; // max mipmaps
				lights[idx].inv_spot_attenuation =
					1.0f / (lights[idx].radius + area_size.length() / 2.0f); // center range

				if (RendererLightStorage::light_area_get_normalize_energy(light)) {
					// normalization to make larger lights output same amount of light as smaller
					// lights with same energy
					float surface_area = area_size.x * area_size.y;
					lights[idx].energy /= surface_area;
				}
			}

			idx++;
		}

		cascade_dynamic_light_count[i] = idx;
	}
}

void GI::VoxelGIInstance::free_resources()
{
	if (texture.is_valid()) {
		RD::free_rid(texture);
		RD::free_rid(write_buffer);

		texture = RID();
		write_buffer = RID();
		mipmaps.clear();
	}

	for (int i = 0; i < dynamic_maps.size(); i++) {
		RD::free_rid(dynamic_maps[i].texture);
		RD::free_rid(dynamic_maps[i].depth);

		// these only exist on the first level...
		if (dynamic_maps[i].fb_depth.is_valid()) {
			RD::free_rid(dynamic_maps[i].fb_depth);
		}
		if (dynamic_maps[i].albedo.is_valid()) {
			RD::free_rid(dynamic_maps[i].albedo);
		}
		if (dynamic_maps[i].normal.is_valid()) {
			RD::free_rid(dynamic_maps[i].normal);
		}
		if (dynamic_maps[i].orm.is_valid()) {
			RD::free_rid(dynamic_maps[i].orm);
		}
	}
	dynamic_maps.clear();
}

void GI::VoxelGIInstance::debug(RD::DrawListID p_draw_list, RID p_framebuffer,
	const Projection& p_camera_with_transform, bool p_lighting, bool p_emission, float p_alpha)
{
	if (mipmaps.is_empty()) {
		return;
	}

	Projection cam_transform = (p_camera_with_transform * Projection(transform)) *
							   Projection(GI::voxel_gi_get_to_cell_xform(probe).affine_inverse());

	int level = 0;
	Vector3i octree_size = GI::voxel_gi_get_octree_size(probe);

	VoxelGIDebugPushConstant push_constant;
	push_constant.alpha = p_alpha;
	push_constant.dynamic_range = GI::voxel_gi_get_dynamic_range(probe);
	push_constant.cell_offset = mipmaps[level].cell_offset;
	push_constant.level = level;

	push_constant.bounds[0] = octree_size.x >> level;
	push_constant.bounds[1] = octree_size.y >> level;
	push_constant.bounds[2] = octree_size.z >> level;
	push_constant.pad = 0;

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			push_constant.projection[i * 4 + j] = cam_transform.columns[i][j];
		}
	}

	if (GI::voxel_gi_debug_uniform_set.is_valid()) {
		RD::free_rid(GI::voxel_gi_debug_uniform_set);
	}
	Vector<RD::Uniform> uniforms;
	{
		RD::Uniform u;
		u.uniform_type = RDC::UNIFORM_TYPE_STORAGE_BUFFER;
		u.binding = 1;
		u.append_id(GI::voxel_gi_get_data_buffer(probe));
		uniforms.push_back(u);
	}
	{
		RD::Uniform u;
		u.uniform_type = RDC::UNIFORM_TYPE_TEXTURE;
		u.binding = 2;
		u.append_id(texture);
		uniforms.push_back(u);
	}
	{
		RD::Uniform u;
		u.uniform_type = RDC::UNIFORM_TYPE_SAMPLER;
		u.binding = 3;
		u.append_id(MaterialStorage::sampler_rd_get_default(
			RSE::CANVAS_ITEM_TEXTURE_FILTER_NEAREST, RSE::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED));
		uniforms.push_back(u);
	}

	int cell_count;
	if (!p_emission && p_lighting && has_dynamic_object_data) {
		cell_count = push_constant.bounds[0] * push_constant.bounds[1] * push_constant.bounds[2];
	}
	else {
		cell_count = mipmaps[level].cell_count;
	}

	voxel_gi_debug_uniform_set =
		RD::uniform_set_create(uniforms, GI::voxel_gi_debug_shader_version_shaders[0], 0);

	int voxel_gi_debug_pipeline = VOXEL_GI_DEBUG_COLOR;
	if (p_emission) {
		voxel_gi_debug_pipeline = VOXEL_GI_DEBUG_EMISSION;
	}
	else if (p_lighting) {
		voxel_gi_debug_pipeline =
			has_dynamic_object_data ? VOXEL_GI_DEBUG_LIGHT_FULL : VOXEL_GI_DEBUG_LIGHT;
	}
	RD::draw_list_bind_render_pipeline(p_draw_list,
		GI::voxel_gi_debug_shader_version_pipelines[voxel_gi_debug_pipeline].get_render_pipeline(
			RDC::INVALID_ID, RD::framebuffer_get_format(p_framebuffer)));
	RD::draw_list_bind_uniform_set(p_draw_list, GI::voxel_gi_debug_uniform_set, 0);
	RD::draw_list_set_push_constant(p_draw_list, &push_constant, sizeof(VoxelGIDebugPushConstant));
	RD::draw_list_draw(p_draw_list, false, cell_count, 36);
}

void GI::finalize()
{
	if (!_initialized) {
		return;
	}
	_initialized = false;

	for (int v = 0; v < SHADER_SPECIALIZATION_VARIATIONS; v++) {
		for (int i = 0; i < MODE_MAX; i++) {
			pipelines[v][i].free();
		}
	}

	sdfgi_shader.debug_pipeline.free();

	for (int i = 0; i < SDFGIShader::DIRECT_LIGHT_MODE_MAX; i++) {
		sdfgi_shader.direct_light_pipeline[i].free();
	}

	for (int i = 0; i < SDFGIShader::INTEGRATE_MODE_MAX; i++) {
		sdfgi_shader.integrate_pipeline[i].free();
	}

	for (int i = 0; i < SDFGIShader::PRE_PROCESS_MAX; i++) {
		sdfgi_shader.preprocess_pipeline[i].free();
	}

	for (int i = 0; i < VOXEL_GI_SHADER_VERSION_MAX; i++) {
		voxel_gi_lighting_shader_version_pipelines[i].free();
	}

	if (shader_version.is_valid()) {
		shader.version_free(shader_version);
		shader_version = RID();
	}
	if (voxel_gi_lighting_shader_version.is_valid()) {
		voxel_gi_shader.version_free(voxel_gi_lighting_shader_version);
		voxel_gi_lighting_shader_version = RID();
	}
	if (voxel_gi_debug_shader_version.is_valid()) {
		voxel_gi_debug_shader.version_free(voxel_gi_debug_shader_version);
		voxel_gi_debug_shader_version = RID();
	}
	if (sdfgi_shader.preprocess_shader.is_valid()) {
		sdfgi_shader.preprocess.version_free(sdfgi_shader.preprocess_shader);
		sdfgi_shader.preprocess_shader = RID();
	}
	if (sdfgi_shader.direct_light_shader.is_valid()) {
		sdfgi_shader.direct_light.version_free(sdfgi_shader.direct_light_shader);
		sdfgi_shader.direct_light_shader = RID();
	}
	if (sdfgi_shader.integrate_shader.is_valid()) {
		sdfgi_shader.integrate.version_free(sdfgi_shader.integrate_shader);
		sdfgi_shader.integrate_shader = RID();
	}
	if (sdfgi_shader.debug_shader_version.is_valid()) {
		sdfgi_shader.debug.version_free(sdfgi_shader.debug_shader_version);
		sdfgi_shader.debug_shader_version = RID();
	}
	if (sdfgi_shader.debug_probes_shader_version.is_valid()) {
		sdfgi_shader.debug_probes.version_free(sdfgi_shader.debug_probes_shader_version);
		sdfgi_shader.debug_probes_shader_version = RID();
	}
}

void GI::free()
{
	if (default_voxel_gi_buffer.is_valid()) {
		RD::free_rid(default_voxel_gi_buffer);
	}
	if (voxel_gi_lights_uniform.is_valid()) {
		RD::free_rid(voxel_gi_lights_uniform);
	}
	if (sdfgi_ubo.is_valid()) {
		RD::free_rid(sdfgi_ubo);
	}

	if (voxel_gi_lights) {
		memdelete_arr(voxel_gi_lights);
	}
}

Ref<GI::SDFGI> GI::create_sdfgi(
	RID p_env, const Vector3& p_world_position, uint32_t p_requested_history_size)
{
	Ref<SDFGI> sdfgi;
	sdfgi.instantiate();

	sdfgi->create(p_env, p_world_position, p_requested_history_size);

	return sdfgi;
}

void GI::setup_voxel_gi_instances(RenderDataRD* p_render_data,
	Ref<RenderSceneBuffersRD> p_render_buffers, const Transform3D& p_transform,
	const PagedArray<RID>& p_voxel_gi_instances, uint32_t& r_voxel_gi_instances_used)
{
	ERR_FAIL_COND(p_render_buffers.is_null());

	r_voxel_gi_instances_used = 0;

	Ref<RenderBuffersGI> rbgi = p_render_buffers->get_custom_data(RB_SCOPE_GI);
	ERR_FAIL_COND(rbgi.is_null());

	RID voxel_gi_buffer = rbgi->get_voxel_gi_buffer();
	VoxelGIData voxel_gi_data[MAX_VOXEL_GI_INSTANCES];

	bool voxel_gi_instances_changed = false;

	Transform3D to_camera;
	to_camera.origin = p_transform.origin; // only translation, make local

	for (int i = 0; i < MAX_VOXEL_GI_INSTANCES; i++) {
		RID texture;
		if (i < (int)p_voxel_gi_instances.size()) {
			VoxelGIInstance* gipi = voxel_gi_instance_owner.get_or_null(p_voxel_gi_instances[i]);

			if (gipi) {
				texture = gipi->texture;
				VoxelGIData& gipd = voxel_gi_data[i];

				RID base_probe = gipi->probe;

				Transform3D to_cell = voxel_gi_get_to_cell_xform(gipi->probe) *
									  gipi->transform.affine_inverse() * to_camera;

				gipd.xform[0] = to_cell.basis.rows[0][0];
				gipd.xform[1] = to_cell.basis.rows[1][0];
				gipd.xform[2] = to_cell.basis.rows[2][0];
				gipd.xform[3] = 0;
				gipd.xform[4] = to_cell.basis.rows[0][1];
				gipd.xform[5] = to_cell.basis.rows[1][1];
				gipd.xform[6] = to_cell.basis.rows[2][1];
				gipd.xform[7] = 0;
				gipd.xform[8] = to_cell.basis.rows[0][2];
				gipd.xform[9] = to_cell.basis.rows[1][2];
				gipd.xform[10] = to_cell.basis.rows[2][2];
				gipd.xform[11] = 0;
				gipd.xform[12] = to_cell.origin.x;
				gipd.xform[13] = to_cell.origin.y;
				gipd.xform[14] = to_cell.origin.z;
				gipd.xform[15] = 1;

				Vector3 bounds = voxel_gi_get_octree_size(base_probe);

				gipd.bounds[0] = bounds.x;
				gipd.bounds[1] = bounds.y;
				gipd.bounds[2] = bounds.z;

				gipd.dynamic_range =
					voxel_gi_get_dynamic_range(base_probe) * voxel_gi_get_energy(base_probe);
				gipd.bias = voxel_gi_get_bias(base_probe);
				gipd.normal_bias = voxel_gi_get_normal_bias(base_probe);
				gipd.blend_ambient = !voxel_gi_is_interior(base_probe);
				gipd.mipmaps = gipi->mipmaps.size();
				gipd.exposure_normalization = 1.0;
				if (p_render_data->camera_attributes.is_valid()) {
					float exposure_normalization =
						RendererCameraAttributes::camera_attributes_get_exposure_normalization_factor(
							p_render_data->camera_attributes);
					gipd.exposure_normalization =
						exposure_normalization /
						voxel_gi_get_baked_exposure_normalization(base_probe);
				}
			}

			r_voxel_gi_instances_used++;
		}

		if (texture == RID()) {
			texture = RendererRD::TextureStorage::texture_rd_get_default(
				RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_3D_WHITE);
		}

		if (texture != rbgi->voxel_gi_textures[i]) {
			voxel_gi_instances_changed = true;
			rbgi->voxel_gi_textures[i] = texture;
		}
	}

	if (voxel_gi_instances_changed) {
		for (uint32_t v = 0; v < RendererSceneRender::MAX_RENDER_VIEWS; v++) {
			if (RD::uniform_set_is_valid(rbgi->uniform_set[v])) {
				RD::free_rid(rbgi->uniform_set[v]);
			}
			rbgi->uniform_set[v] = RID();
		}

		if (p_render_buffers->has_custom_data(RB_SCOPE_FOG)) {
			// VoxelGI instances have changed, so we need to update volumetric fog.
			Ref<RendererRD::Fog::VolumetricFog> fog =
				p_render_buffers->get_custom_data(RB_SCOPE_FOG);
			fog->sync_gi_dependent_sets_validity(true);
		}
	}

	if (p_voxel_gi_instances.size() > 0) {
		RD::draw_command_begin_label("VoxelGIs Setup");
		RD::draw_command_end_label();
	}
}

void GI::RenderBuffersGI::free_data()
{
	for (uint32_t v = 0; v < RendererSceneRender::MAX_RENDER_VIEWS; v++) {
		if (RD::uniform_set_is_valid(uniform_set[v])) {
			RD::free_rid(uniform_set[v]);
		}
		uniform_set[v] = RID();
	}

	if (scene_data_ubo.is_valid()) {
		RD::free_rid(scene_data_ubo);
		scene_data_ubo = RID();
	}

	if (voxel_gi_buffer.is_valid()) {
		RD::free_rid(voxel_gi_buffer);
		voxel_gi_buffer = RID();
	}
}

RID GI::voxel_gi_instance_create(RID p_base)
{
	VoxelGIInstance voxel_gi;
	voxel_gi.probe = p_base;
	RID rid = voxel_gi_instance_owner.make_rid(voxel_gi);
	return rid;
}

void GI::voxel_gi_instance_free(RID p_rid)
{
	GI::VoxelGIInstance* voxel_gi = voxel_gi_instance_owner.get_or_null(p_rid);
	voxel_gi->free_resources();
	voxel_gi_instance_owner.free(p_rid);
}

void GI::voxel_gi_instance_set_transform_to_data(RID p_probe, const Transform3D& p_xform)
{
	VoxelGIInstance* voxel_gi = voxel_gi_instance_owner.get_or_null(p_probe);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->transform = p_xform;
}

bool GI::voxel_gi_needs_update(RID p_probe)
{
	VoxelGIInstance* voxel_gi = voxel_gi_instance_owner.get_or_null(p_probe);
	ERR_FAIL_NULL_V(voxel_gi, false);

	return voxel_gi->last_probe_version != voxel_gi_get_version(voxel_gi->probe);
}

void GI::voxel_gi_update(RID p_probe, bool p_update_light_instances,
	const Vector<RID>& p_light_instances,
	const PagedArray<RenderGeometryInstance*>& p_dynamic_objects)
{
	VoxelGIInstance* voxel_gi = voxel_gi_instance_owner.get_or_null(p_probe);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->update(p_update_light_instances, p_light_instances, p_dynamic_objects);
}

void GI::debug_voxel_gi(RID p_voxel_gi, RD::DrawListID p_draw_list, RID p_framebuffer,
	const Projection& p_camera_with_transform, bool p_lighting, bool p_emission, float p_alpha)
{
	VoxelGIInstance* voxel_gi = voxel_gi_instance_owner.get_or_null(p_voxel_gi);
	ERR_FAIL_NULL(voxel_gi);

	voxel_gi->debug(
		p_draw_list, p_framebuffer, p_camera_with_transform, p_lighting, p_emission, p_alpha);
}

void GI::enable_vrs_shader_group() { shader.enable_group(GROUP_VRS); }

RID RendererRD::GI::RenderBuffersGI::get_voxel_gi_buffer() { return RID(); }

void RendererRD::GI::VoxelGIInstance::update(bool p_update_light_instances,
	const Vector<RID>& p_light_instances,
	const PagedArray<RenderGeometryInstance*>& p_dynamic_objects)
{
}

void RendererRD::GI::process_gi(Ref<RenderSceneBuffersRD> p_render_buffers,
	const RID* p_color_buffers, RID p_dest_buffer, RID p_environment, unsigned int p_view_count,
	const Projection* p_projections, const Vector3* p_eye_offsets,
	const Transform3D& p_cam_transform, const PagedArray<RID>& p_position_buffers)
{
}

void RendererRD::GI::initialize(RendererRD::SkyRD* p_sky)
{
	if (_initialized) {
		return;
	}

	// 1. Main GI gather shader
	{
		Vector<String> modes;
		modes.push_back("\n#define MODE_VOXEL_GI\n");
		modes.push_back("\n#define MODE_VOXEL_GI\n#define MODE_VOXEL_GI_WITHOUT_SAMPLER\n");
		modes.push_back("\n#define MODE_SDFGI\n");
		modes.push_back("\n#define MODE_VOXEL_GI\n#define MODE_SDFGI\n");
		modes.push_back(
			"\n#define MODE_VOXEL_GI\n#define MODE_VOXEL_GI_WITHOUT_SAMPLER\n#define MODE_SDFGI\n");

		shader.initialize(modes);
		shader_version = shader.version_create();
	}

	// 2. Voxel GI lighting & debug shaders
	{
		Vector<String> modes;
		modes.push_back("\n#define MODE_COMPUTE_LIGHT\n");
		modes.push_back("\n#define MODE_COMPUTE_SECOND_BOUNCE\n");
		modes.push_back("\n#define MODE_COMPUTE_MIPMAP\n");
		modes.push_back("\n#define MODE_WRITE_TEXTURE\n");
		modes.push_back("\n#define MODE_DYNAMIC_OBJECT_LIGHTING\n");
		modes.push_back("\n#define MODE_DYNAMIC_SHRINK_WRITE\n");
		modes.push_back("\n#define MODE_DYNAMIC_SHRINK_PLOT\n");
		modes.push_back("\n#define MODE_DYNAMIC_SHRINK_WRITE_PLOT\n");

		voxel_gi_shader.initialize(modes);
		voxel_gi_lighting_shader_version = voxel_gi_shader.version_create();

		Vector<String> debug_modes;
		debug_modes.push_back("\n#define MODE_DEBUG_COLOR\n");
		debug_modes.push_back("\n#define MODE_DEBUG_LIGHT\n");
		debug_modes.push_back("\n#define MODE_DEBUG_EMISSION\n");
		debug_modes.push_back("\n#define MODE_DEBUG_LIGHT_FULL\n");

		voxel_gi_debug_shader.initialize(debug_modes);
		voxel_gi_debug_shader_version = voxel_gi_debug_shader.version_create();
	}

	// 3. SDFGI compute pipeline stages
	{
		Vector<String> preprocess_modes;
		preprocess_modes.push_back("\n#define MODE_SCROLL\n");
		preprocess_modes.push_back("\n#define MODE_SCROLL_OCCLUSION\n");
		preprocess_modes.push_back("\n#define MODE_JUMP_FLOOD_INITIALIZE\n");
		preprocess_modes.push_back("\n#define MODE_JUMP_FLOOD_INITIALIZE_HALF\n");
		preprocess_modes.push_back("\n#define MODE_JUMP_FLOOD\n");
		preprocess_modes.push_back("\n#define MODE_JUMP_FLOOD_OPTIMIZED\n");
		preprocess_modes.push_back("\n#define MODE_JUMP_FLOOD_UPSCALE\n");
		preprocess_modes.push_back("\n#define MODE_OCCLUSION\n");
		preprocess_modes.push_back("\n#define MODE_STORE\n");

		sdfgi_shader.preprocess.initialize(preprocess_modes);
		sdfgi_shader.preprocess_shader = sdfgi_shader.preprocess.version_create();

		Vector<String> direct_light_modes;
		direct_light_modes.push_back("\n#define MODE_STATIC\n");
		direct_light_modes.push_back("\n#define MODE_DYNAMIC\n");

		sdfgi_shader.direct_light.initialize(direct_light_modes);
		sdfgi_shader.direct_light_shader = sdfgi_shader.direct_light.version_create();

		Vector<String> integrate_modes;
		integrate_modes.push_back("\n#define MODE_PROCESS\n");
		integrate_modes.push_back("\n#define MODE_STORE\n");
		integrate_modes.push_back("\n#define MODE_SCROLL\n");
		integrate_modes.push_back("\n#define MODE_SCROLL_STORE\n");

		sdfgi_shader.integrate.initialize(integrate_modes);
		sdfgi_shader.integrate_shader = sdfgi_shader.integrate.version_create();

		Vector<String> debug_modes;
		debug_modes.push_back("\n");

		sdfgi_shader.debug.initialize(debug_modes);
		sdfgi_shader.debug_shader_version = sdfgi_shader.debug.version_create();

		Vector<String> debug_probes_modes;
		debug_probes_modes.push_back("\n#define MODE_PROBES\n");
		debug_probes_modes.push_back("\n#define MODE_PROBES\n#define MODE_MULTIVIEW\n");
		debug_probes_modes.push_back("\n#define MODE_VISIBILITY\n");
		debug_probes_modes.push_back("\n#define MODE_VISIBILITY\n#define MODE_MULTIVIEW\n");

		sdfgi_shader.debug_probes.initialize(debug_probes_modes);
		sdfgi_shader.debug_probes_shader_version = sdfgi_shader.debug_probes.version_create();
	}

	_initialized = true;
}

// --- Merged from servers/rendering/renderer_rd/environment/fog.cpp ---

using namespace RendererRD;


void Fog::initialize() {}

void Fog::finalize() {}

int Fog::_get_fog_shader_group()
{
	bool use_32_bit_atomics = RD::has_feature(RDC::SUPPORTS_IMAGE_ATOMIC_32_BIT);
	bool use_vulkan_memory_model = RD::has_feature(RDC::SUPPORTS_VULKAN_MEMORY_MODEL);
	if (use_vulkan_memory_model) {
		return use_32_bit_atomics
				   ? VolumetricFogShader::SHADER_GROUP_VULKAN_MEMORY_MODEL
				   : VolumetricFogShader::SHADER_GROUP_VULKAN_MEMORY_MODEL_NO_ATOMICS;
	}
	else {
		return use_32_bit_atomics ? VolumetricFogShader::SHADER_GROUP_BASE
								  : VolumetricFogShader::SHADER_GROUP_NO_ATOMICS;
	}
}

int Fog::_get_fog_variant()
{
	bool use_32_bit_atomics = RD::has_feature(RDC::SUPPORTS_IMAGE_ATOMIC_32_BIT);
	bool use_vulkan_memory_model = RD::has_feature(RDC::SUPPORTS_VULKAN_MEMORY_MODEL);
	return (use_vulkan_memory_model ? 2 : 0) + (use_32_bit_atomics ? 0 : 1);
}

int Fog::_get_fog_process_variant(int p_idx)
{
	bool use_32_bit_atomics = RD::has_feature(RDC::SUPPORTS_IMAGE_ATOMIC_32_BIT);
	bool use_vulkan_memory_model = RD::has_feature(RDC::SUPPORTS_VULKAN_MEMORY_MODEL);
	return (use_vulkan_memory_model ? (VolumetricFogShader::VOLUMETRIC_FOG_PROCESS_SHADER_MAX * 2)
									: 0) +
		   (use_32_bit_atomics ? 0 : VolumetricFogShader::VOLUMETRIC_FOG_PROCESS_SHADER_MAX) +
		   p_idx;
}

/* FOG VOLUMES */

RID Fog::fog_volume_allocate() { return fog_volume_owner.allocate_rid(); }

void Fog::fog_volume_initialize(RID p_rid) { fog_volume_owner.initialize_rid(p_rid, FogVolume()); }

void Fog::fog_volume_free(RID p_rid)
{
	FogVolume* fog_volume = fog_volume_owner.get_or_null(p_rid);
	fog_volume->dependency.deleted_notify(p_rid);
	fog_volume_owner.free(p_rid);
}

Dependency* Fog::fog_volume_get_dependency(RID p_fog_volume)
{
	FogVolume* fog_volume = fog_volume_owner.get_or_null(p_fog_volume);
	ERR_FAIL_NULL_V(fog_volume, nullptr);

	return &fog_volume->dependency;
}

void Fog::fog_volume_set_shape(RID p_fog_volume, RSE::FogVolumeShape p_shape)
{
	FogVolume* fog_volume = fog_volume_owner.get_or_null(p_fog_volume);
	ERR_FAIL_NULL(fog_volume);

	if (p_shape == fog_volume->shape) {
		return;
	}

	fog_volume->shape = p_shape;
	fog_volume->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_AABB);
}

void Fog::fog_volume_set_size(RID p_fog_volume, const Vector3& p_size)
{
	FogVolume* fog_volume = fog_volume_owner.get_or_null(p_fog_volume);
	ERR_FAIL_NULL(fog_volume);

	fog_volume->size = p_size;
	fog_volume->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_AABB);
}

void Fog::fog_volume_set_material(RID p_fog_volume, RID p_material)
{
	FogVolume* fog_volume = fog_volume_owner.get_or_null(p_fog_volume);
	ERR_FAIL_NULL(fog_volume);
	fog_volume->material = p_material;
}

RID Fog::fog_volume_get_material(RID p_fog_volume)
{
	FogVolume* fog_volume = fog_volume_owner.get_or_null(p_fog_volume);
	ERR_FAIL_NULL_V(fog_volume, RID());

	return fog_volume->material;
}

RSE::FogVolumeShape Fog::fog_volume_get_shape(RID p_fog_volume)
{
	FogVolume* fog_volume = fog_volume_owner.get_or_null(p_fog_volume);
	ERR_FAIL_NULL_V(fog_volume, RSE::FOG_VOLUME_SHAPE_BOX);

	return fog_volume->shape;
}

AABB Fog::fog_volume_get_aabb(RID p_fog_volume)
{
	FogVolume* fog_volume = fog_volume_owner.get_or_null(p_fog_volume);
	ERR_FAIL_NULL_V(fog_volume, AABB());

	switch (fog_volume->shape) {
	case RSE::FOG_VOLUME_SHAPE_ELLIPSOID:
	case RSE::FOG_VOLUME_SHAPE_CONE:
	case RSE::FOG_VOLUME_SHAPE_CYLINDER:
	case RSE::FOG_VOLUME_SHAPE_BOX: {
		AABB aabb;
		aabb.position = -fog_volume->size / 2;
		aabb.size = fog_volume->size;
		return aabb;
	}
	default: {
		// Need some size otherwise will get culled
		return AABB(Vector3(-1, -1, -1), Vector3(2, 2, 2));
	}
	}
}

Vector3 Fog::fog_volume_get_size(RID p_fog_volume)
{
	const FogVolume* fog_volume = fog_volume_owner.get_or_null(p_fog_volume);
	ERR_FAIL_NULL_V(fog_volume, Vector3());
	return fog_volume->size;
}

Fog::FogMaterialData::~FogMaterialData() { free_parameters_uniform_set(uniform_set); }

RendererRD::MaterialStorage::ShaderData* Fog::_create_fog_shader_func()
{
	FogShaderData* shader_data = memnew(FogShaderData);
	return shader_data;
}

RendererRD::MaterialStorage::ShaderData* Fog::_create_fog_shader_funcs()
{
	return Fog::_create_fog_shader_func();
}

RendererRD::MaterialStorage::MaterialData* Fog::_create_fog_material_func(FogShaderData* p_shader)
{
	FogMaterialData* material_data = memnew(FogMaterialData);
	material_data->shader_data = p_shader;
	// update will happen later anyway so do nothing.
	return material_data;
}

RendererRD::MaterialStorage::MaterialData* Fog::_create_fog_material_funcs(
	RendererRD::MaterialStorage::ShaderData* p_shader)
{
	return Fog::_create_fog_material_func(static_cast<FogShaderData*>(p_shader));
}

RID Fog::fog_volume_instance_create(RID p_fog_volume)
{
	FogVolumeInstance fvi;
	fvi.volume = p_fog_volume;
	return fog_volume_instance_owner.make_rid(fvi);
}

void Fog::fog_instance_free(RID p_rid) { fog_volume_instance_owner.free(p_rid); }

void Fog::free_fog_shader()
{
	for (int i = 0; i < VolumetricFogShader::VOLUMETRIC_FOG_PROCESS_SHADER_MAX; i++) {
		volumetric_fog.process_pipelines[i].free();
	}
	if (volumetric_fog.process_shader_version.is_valid()) {
		volumetric_fog.process_shader.version_free(volumetric_fog.process_shader_version);
	}
	if (volumetric_fog.volume_ubo.is_valid()) {
		RD::free_rid(volumetric_fog.volume_ubo);
	}
	if (volumetric_fog.params_ubo.is_valid()) {
		RD::free_rid(volumetric_fog.params_ubo);
	}
	if (volumetric_fog.default_shader.is_valid()) {
		MaterialStorage::shader_free(volumetric_fog.default_shader);
	}
	if (volumetric_fog.default_material.is_valid()) {
		MaterialStorage::material_free(volumetric_fog.default_material);
	}
}

void Fog::FogShaderData::set_code(const String& p_code)
{
	// compile

	code = p_code;
	valid = false;
	ubo_size = 0;
	uniforms.clear();

	if (code.is_empty()) {
		return; // just invalid, but no error
	}

	ShaderCompiler::GeneratedCode gen_code;
	ShaderCompiler::IdentifierActions actions;
	actions.entry_point_stages["fog"] = ShaderCompiler::STAGE_COMPUTE;

	uses_time = false;

	actions.usage_flag_pointers["TIME"] = &uses_time;

	actions.uniforms = &uniforms;

	Error err = RendererRD::Fog::volumetric_fog.compiler.compile(
		RSE::SHADER_FOG, code, &actions, path, gen_code);
	ERR_FAIL_COND_MSG(err != OK, "Fog shader compilation failed.");

	if (version.is_null()) {
		version = RendererRD::Fog::volumetric_fog.shader.version_create();
	}
	else {
		pipeline.free();
	}

	RendererRD::Fog::volumetric_fog.shader.version_set_compute_code(version, gen_code.code,
		gen_code.uniforms, gen_code.stage_globals[ShaderCompiler::STAGE_COMPUTE], gen_code.defines);
	ERR_FAIL_COND(!RendererRD::Fog::volumetric_fog.shader.version_is_valid(version));

	ubo_size = gen_code.uniform_total_size;
	ubo_offsets = gen_code.uniform_offsets;
	texture_uniforms = gen_code.texture_uniforms;

	valid = true;
}

bool Fog::FogShaderData::is_animated() const { return false; }

bool Fog::FogShaderData::casts_shadows() const { return false; }

RenderingServerTypes::ShaderNativeSourceCode Fog::FogShaderData::get_native_source_code() const
{
	return RendererRD::Fog::volumetric_fog.shader.version_get_native_source_code(version);
}

Pair<ShaderRD*, RID> Fog::FogShaderData::get_native_shader_and_version() const
{
	return {&RendererRD::Fog::volumetric_fog.shader, version};
}

Fog::FogShaderData::~FogShaderData()
{
	pipeline.free();

	if (version.is_valid()) {
		RendererRD::Fog::volumetric_fog.shader.version_free(version);
	}
}

bool Fog::VolumetricFog::sync_gi_dependent_sets_validity(bool p_ensure_freed)
{
	bool null = gi_dependent_sets.process_uniform_set_density.is_null();
	bool valid = !null && RD::uniform_set_is_valid(
							  gi_dependent_sets.process_uniform_set_density);

#ifdef DEV_ENABLED
	// It's all-or-nothing, or something else has changed that requires dev attention.
	DEV_ASSERT(null == gi_dependent_sets.process_uniform_set.is_null());
	DEV_ASSERT(null == gi_dependent_sets.process_uniform_set2.is_null());
	DEV_ASSERT(
		valid == RD::uniform_set_is_valid(gi_dependent_sets.process_uniform_set));
	DEV_ASSERT(
		valid == RD::uniform_set_is_valid(gi_dependent_sets.process_uniform_set2));
#endif

	if (valid) {
		if (p_ensure_freed) {
			RD::free_rid(gi_dependent_sets.process_uniform_set_density);
			RD::free_rid(gi_dependent_sets.process_uniform_set);
			RD::free_rid(gi_dependent_sets.process_uniform_set2);
			valid = false;
		}
	}

	if (!valid && !null) {
		gi_dependent_sets = {};
	}

	return valid;
}

void Fog::VolumetricFog::init(const Vector3i& fog_size, RID p_sky_shader)
{
	width = fog_size.x;
	height = fog_size.y;
	depth = fog_size.z;
	atomic_type = RD::has_feature(RDC::SUPPORTS_IMAGE_ATOMIC_32_BIT)
					  ? RDC::UNIFORM_TYPE_IMAGE
					  : RDC::UNIFORM_TYPE_STORAGE_BUFFER;

	RDC::TextureFormat tf;
	tf.format = RDC::DATA_FORMAT_R16G16B16A16_SFLOAT;
	tf.width = fog_size.x;
	tf.height = fog_size.y;
	tf.depth = fog_size.z;
	tf.texture_type = RDC::TEXTURE_TYPE_3D;
	tf.usage_bits = RDC::TEXTURE_USAGE_STORAGE_BIT | RDC::TEXTURE_USAGE_CAN_COPY_FROM_BIT;

	light_density_map = RD::texture_create(tf, RDD::TextureView());
	RD::set_resource_name(light_density_map, "Fog light-density map");

	tf.usage_bits = RDC::TEXTURE_USAGE_STORAGE_BIT | RDC::TEXTURE_USAGE_SAMPLING_BIT |
					RDC::TEXTURE_USAGE_CAN_COPY_TO_BIT;

	prev_light_density_map = RD::texture_create(tf, RDD::TextureView());
	RD::set_resource_name(
		prev_light_density_map, "Fog previous light-density map");
	RD::texture_clear(prev_light_density_map, Color(0, 0, 0, 0), 0, 1, 0, 1);

	tf.usage_bits = RDC::TEXTURE_USAGE_STORAGE_BIT | RDC::TEXTURE_USAGE_SAMPLING_BIT;

	fog_map = RD::texture_create(tf, RDD::TextureView());
	RD::set_resource_name(fog_map, "Fog map");

	if (atomic_type == RDC::UNIFORM_TYPE_STORAGE_BUFFER) {
		Vector<uint8_t> dm;
		dm.resize_initialized(fog_size.x * fog_size.y * fog_size.z * 4);

		density_map = RD::storage_buffer_create(dm.size(), dm);
		RD::set_resource_name(density_map, "Fog density map");
		light_map = RD::storage_buffer_create(dm.size(), dm);
		RD::set_resource_name(light_map, "Fog light map");
		emissive_map = RD::storage_buffer_create(dm.size(), dm);
		RD::set_resource_name(emissive_map, "Fog emissive map");
	}
	else {
		tf.format = RDC::DATA_FORMAT_R32_UINT;
		tf.usage_bits = RDC::TEXTURE_USAGE_STORAGE_BIT | RDC::TEXTURE_USAGE_CAN_COPY_TO_BIT |
						RDC::TEXTURE_USAGE_STORAGE_ATOMIC_BIT;
		density_map = RD::texture_create(tf, RDD::TextureView());
		RD::set_resource_name(density_map, "Fog density map");
		RD::texture_clear(density_map, Color(0, 0, 0, 0), 0, 1, 0, 1);
		light_map = RD::texture_create(tf, RDD::TextureView());
		RD::set_resource_name(light_map, "Fog light map");
		RD::texture_clear(light_map, Color(0, 0, 0, 0), 0, 1, 0, 1);
		emissive_map = RD::texture_create(tf, RDD::TextureView());
		RD::set_resource_name(emissive_map, "Fog emissive map");
		RD::texture_clear(emissive_map, Color(0, 0, 0, 0), 0, 1, 0, 1);
	}

	Vector<RD::Uniform> uniforms;
	{
		RD::Uniform u;
		u.binding = 0;
		u.uniform_type = RDC::UNIFORM_TYPE_TEXTURE;
		u.append_id(fog_map);
		uniforms.push_back(u);
	}

	sky_uniform_set = RD::uniform_set_create(
		uniforms, p_sky_shader, RendererRD::SkyRD::SKY_SET_FOG);
}

Fog::VolumetricFog::~VolumetricFog()
{
	RD::free_rid(prev_light_density_map);
	RD::free_rid(light_density_map);
	RD::free_rid(fog_map);
	RD::free_rid(density_map);
	RD::free_rid(light_map);
	RD::free_rid(emissive_map);

	if (fog_uniform_set.is_valid() && RD::uniform_set_is_valid(fog_uniform_set)) {
		RD::free_rid(fog_uniform_set);
	}
	if (copy_uniform_set.is_valid() &&
		RD::uniform_set_is_valid(copy_uniform_set)) {
		RD::free_rid(copy_uniform_set);
	}

	sync_gi_dependent_sets_validity(true);

	if (sdfgi_uniform_set.is_valid() &&
		RD::uniform_set_is_valid(sdfgi_uniform_set)) {
		RD::free_rid(sdfgi_uniform_set);
	}
	if (sky_uniform_set.is_valid() && RD::uniform_set_is_valid(sky_uniform_set)) {
		RD::free_rid(sky_uniform_set);
	}
}

Vector3i Fog::_point_get_position_in_froxel_volume(const Vector3& p_point, float fog_end,
	const Vector2& fog_near_size, const Vector2& fog_far_size, float volumetric_fog_detail_spread,
	const Vector3& fog_size, const Transform3D& p_cam_transform)
{
	Vector3 view_position = p_cam_transform.affine_inverse().xform(p_point);
	view_position.z = MIN(view_position.z, -0.01); // Clamp to the front of camera
	Vector3 fog_position = Vector3(0, 0, 0);

	view_position.y = -view_position.y;
	fog_position.z = -view_position.z / fog_end;
	fog_position.x =
		(view_position.x /
			(2 * (fog_near_size.x * (1.0 - fog_position.z) + fog_far_size.x * fog_position.z))) +
		0.5;
	fog_position.y =
		(view_position.y /
			(2 * (fog_near_size.y * (1.0 - fog_position.z) + fog_far_size.y * fog_position.z))) +
		0.5;
	fog_position.z = Math::pow(float(fog_position.z), float(1.0 / volumetric_fog_detail_spread));
	fog_position = fog_position * fog_size - Vector3(0.5, 0.5, 0.5);

	fog_position = fog_position.clamp(Vector3(), fog_size);

	return Vector3i(fog_position);
}
