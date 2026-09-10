/**************************************************************************/
/*  sky.cpp                                                               */
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

#include "core/config/project_settings.h"
#include "core/math/math_defs.h"
#include "servers/rendering/renderer_rd/effects/copy_effects.h"
#include "servers/rendering/renderer_rd/framebuffer_cache_rd.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/renderer_scene_render_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/render_data_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/render_scene_buffers_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/texture_storage.h"
#include "servers/rendering/renderer_rd/uniform_set_cache_rd.h"
#include "servers/rendering/rendering_server_default.h"
#include "servers/rendering/rendering_server_globals.h"
#include "sky.h"

using namespace RendererRD;

#define RB_SCOPE_SKY SNAME("sky_buffers")
#define RB_HALF_TEXTURE SNAME("half_texture")
#define RB_QUARTER_TEXTURE SNAME("quarter_texture")

bool SkyRD::SkyShaderData::is_animated() const { return false; }

bool SkyRD::SkyShaderData::casts_shadows() const { return false; }

RenderingServerTypes::ShaderNativeSourceCode SkyRD::SkyShaderData::get_native_source_code() const
{
	RendererSceneRenderRD* scene_singleton =
		static_cast<RendererSceneRenderRD*>(RendererSceneRenderRD::singleton);

	return scene_singleton->sky.sky_shader.shader.version_get_native_source_code(version);
}

Pair<ShaderRD*, RID> SkyRD::SkyShaderData::get_native_shader_and_version() const
{
	RendererSceneRenderRD* scene_singleton =
		static_cast<RendererSceneRenderRD*>(RendererSceneRenderRD::singleton);
	return {&scene_singleton->sky.sky_shader.shader, version};
}

SkyRD::SkyShaderData::~SkyShaderData()
{
	RendererSceneRenderRD* scene_singleton =
		static_cast<RendererSceneRenderRD*>(RendererSceneRenderRD::singleton);
	ERR_FAIL_NULL(scene_singleton);
	// pipeline variants will clear themselves if shader is gone
	if (version.is_valid()) {
		scene_singleton->sky.sky_shader.shader.version_free(version);
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

	RenderingDevice::FramebufferFormatID fb_format =
		RD::get_singleton()->framebuffer_get_format(p_fb);

	RD::DrawListID draw_list = p_list;

	RD::get_singleton()->draw_list_bind_render_pipeline(
		draw_list, p_pipeline->get_render_pipeline(RD::INVALID_ID, fb_format, false,
					   RD::get_singleton()->draw_list_get_current_pass()));

	// Update uniform sets.
	{
		RD::get_singleton()->draw_list_bind_uniform_set(
			draw_list, sky_scene_state.uniform_set, SKY_SET_UNIFORMS);
		if (p_uniform_set.is_valid() &&
			RD::get_singleton()->uniform_set_is_valid(
				p_uniform_set)) { // Material may not have a uniform set.
			RD::get_singleton()->draw_list_bind_uniform_set(
				draw_list, p_uniform_set, SKY_SET_MATERIAL);
		}
		RD::get_singleton()->draw_list_bind_uniform_set(draw_list, p_texture_set, SKY_SET_TEXTURES);
		// Fog uniform set can be invalidated before drawing, so validate at draw time
		if (sky_scene_state.fog_uniform_set.is_valid() &&
			RD::get_singleton()->uniform_set_is_valid(sky_scene_state.fog_uniform_set)) {
			RD::get_singleton()->draw_list_bind_uniform_set(
				draw_list, sky_scene_state.fog_uniform_set, SKY_SET_FOG);
		}
		else {
			RD::get_singleton()->draw_list_bind_uniform_set(
				draw_list, sky_scene_state.default_fog_uniform_set, SKY_SET_FOG);
		}
	}

	RD::get_singleton()->draw_list_set_push_constant(
		draw_list, &sky_push_constant, sizeof(SkyPushConstant));

	RD::get_singleton()->draw_list_draw(draw_list, false, 1u, 3u);
}

void SkyRD::ReflectionData::clear_reflection_data()
{
	layers.clear();
	radiance_base_octmap = RID();
	if (downsampled_radiance_octmap.is_valid()) {
		RD::get_singleton()->free_rid(downsampled_radiance_octmap);
	}
	downsampled_radiance_octmap = RID();
	downsampled_layer.mipmaps.clear();
	coefficient_buffer = RID();
}

void SkyRD::Sky::free_radiance()
{
	if (radiance.is_valid()) {
		RD::get_singleton()->free_rid(radiance);
		radiance = RID();
	}
	if (radiance_first_layer_slice.is_valid()) {
		if (RD::get_singleton()->texture_is_valid(radiance_first_layer_slice)) {
			RD::get_singleton()->free_rid(radiance_first_layer_slice);
		}
		radiance_first_layer_slice = RID();
	}
}

void SkyRD::Sky::free()
{
	free_radiance();
	reflection.clear_reflection_data();

	if (uniform_buffer.is_valid()) {
		RD::get_singleton()->free_rid(uniform_buffer);
		uniform_buffer = RID();
	}

	if (material.is_valid()) {
		material = RID();
	}
}

RID SkyRD::Sky::get_textures(SkyTextureSetVersion p_version, RID p_default_shader_rd,
	bool p_is_multiview, Ref<RenderSceneBuffersRD> p_render_buffers)
{
	RendererRD::TextureStorage* texture_storage = RendererRD::TextureStorage::get_singleton();

	thread_local LocalVector<RD::Uniform> uniforms;
	uniforms.clear();

	{
		RD::Uniform u;
		u.uniform_type = RD::UNIFORM_TYPE_TEXTURE;
		u.binding = 0;
		if (radiance.is_valid() && p_version <= SKY_TEXTURE_SET_QUARTER_RES) {
			u.append_id(
				radiance_first_layer_slice.is_valid() ? radiance_first_layer_slice : radiance);
		}
		else {
			u.append_id(texture_storage->texture_rd_get_default(
				RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_BLACK));
		}
		uniforms.push_back(u);
	}
	{
		RD::Uniform u;
		u.uniform_type = RD::UNIFORM_TYPE_TEXTURE;
		u.binding = 1; // half res
		if (p_version >= SKY_TEXTURE_SET_OCTMAP) {
			if (reflection.layers.size() && reflection.layers[0].mipmaps.size() >= 2 &&
				reflection.layers[0].mipmaps[1].view.is_valid() &&
				p_version != SKY_TEXTURE_SET_OCTMAP_HALF_RES) {
				u.append_id(reflection.layers[0].mipmaps[1].view);
			}
			else {
				u.append_id(texture_storage->texture_rd_get_default(
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
				u.append_id(texture_storage->texture_rd_get_default(
					p_is_multiview ? RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_2D_ARRAY_WHITE
								   : RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_WHITE));
			}
		}
		uniforms.push_back(u);
	}
	{
		RD::Uniform u;
		u.uniform_type = RD::UNIFORM_TYPE_TEXTURE;
		u.binding = 2; // quarter res
		if (p_version >= SKY_TEXTURE_SET_OCTMAP) {
			if (reflection.layers.size() && reflection.layers[0].mipmaps.size() >= 3 &&
				reflection.layers[0].mipmaps[2].view.is_valid() &&
				p_version != SKY_TEXTURE_SET_OCTMAP_QUARTER_RES) {
				u.append_id(reflection.layers[0].mipmaps[2].view);
			}
			else {
				u.append_id(texture_storage->texture_rd_get_default(
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
				u.append_id(texture_storage->texture_rd_get_default(
					p_is_multiview ? RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_2D_ARRAY_WHITE
								   : RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_WHITE));
			}
		}
		uniforms.push_back(u);
	}

	return UniformSetCacheRD::get_singleton()->get_cache_vec(
		p_default_shader_rd, SKY_SET_TEXTURES, uniforms);
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
		RendererRD::CopyEffects* copy_effects = RendererRD::CopyEffects::get_singleton();

		RD::TextureFormat tf;
		tf.format = RD::DATA_FORMAT_R32G32B32A32_SFLOAT; // Could be RGBA16
		tf.width = p_size.width;
		tf.height = p_size.height;
		tf.usage_bits = RD::TEXTURE_USAGE_STORAGE_BIT | RD::TEXTURE_USAGE_CAN_COPY_FROM_BIT;

		RID rad_tex = RD::get_singleton()->texture_create(tf, RD::TextureView());
		copy_effects->copy_octmap_to_panorama(radiance, rad_tex, p_size, p_roughness_layers,
			reflection.layers.size() > 1, Size2(uv_border_size, 1.0f - uv_border_size * 2.0));
		Vector<uint8_t> data = RD::get_singleton()->texture_get_data(rad_tex, 0);
		RD::get_singleton()->free_rid(rad_tex);

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

////////////////////////////////////////////////////////////////////////////////
// SkyRD

RendererRD::MaterialStorage::ShaderData* SkyRD::_create_sky_shader_func()
{
	SkyShaderData* shader_data = memnew(SkyShaderData);
	return shader_data;
}

RendererRD::MaterialStorage::ShaderData* SkyRD::_create_sky_shader_funcs()
{
	// !BAS! Why isn't _create_sky_shader_func not just static too?
	return static_cast<RendererSceneRenderRD*>(RendererSceneRenderRD::singleton)
		->sky._create_sky_shader_func();
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
	// !BAS! same here, we could just make _create_sky_material_func static?
	return static_cast<RendererSceneRenderRD*>(RendererSceneRenderRD::singleton)
		->sky._create_sky_material_func(static_cast<SkyShaderData*>(p_shader));
}

RID SkyRD::SkySceneState::get_fog_only_texture_uniform_set(
	RID p_default_shader_rd, bool p_is_multiview)
{
	RID& uniform_set_rid =
		p_is_multiview ? fog_only_texture_multiview_uniform_set : fog_only_texture_uniform_set;

	if (uniform_set_rid.is_null()) {
		RendererRD::TextureStorage* texture_storage = RendererRD::TextureStorage::get_singleton();

		Vector<RD::Uniform> uniforms;
		{
			RD::Uniform u;
			u.uniform_type = RD::UNIFORM_TYPE_TEXTURE;
			u.binding = 0;
			u.append_id(texture_storage->texture_rd_get_default(
				RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_BLACK));
			uniforms.push_back(u);
		}
		{
			RD::Uniform u;
			u.uniform_type = RD::UNIFORM_TYPE_TEXTURE;
			u.binding = 1;
			u.append_id(texture_storage->texture_rd_get_default(
				p_is_multiview ? RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_2D_ARRAY_WHITE
							   : RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_WHITE));
			uniforms.push_back(u);
		}
		{
			RD::Uniform u;
			u.uniform_type = RD::UNIFORM_TYPE_TEXTURE;
			u.binding = 2;
			u.append_id(texture_storage->texture_rd_get_default(
				p_is_multiview ? RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_2D_ARRAY_WHITE
							   : RendererRD::TextureStorage::DEFAULT_RD_TEXTURE_WHITE));
			uniforms.push_back(u);
		}

		uniform_set_rid = RD::get_singleton()->uniform_set_create(
			uniforms, p_default_shader_rd, SKY_SET_TEXTURES);
	}

	return uniform_set_rid;
}

void SkyRD::set_texture_format(RD::DataFormat p_texture_format)
{
	texture_format = p_texture_format;
}

SkyRD::~SkyRD()
{
	// cleanup anything created in init...
	RendererRD::MaterialStorage* material_storage = RendererRD::MaterialStorage::get_singleton();

	SkyMaterialData* md = static_cast<SkyMaterialData*>(material_storage->material_get_data(
		sky_shader.default_material, RendererRD::MaterialStorage::SHADER_TYPE_SKY));
	sky_shader.shader.version_free(md->shader_data->version);
	RD::get_singleton()->free_rid(sky_scene_state.directional_light_buffer);
	RD::get_singleton()->free_rid(sky_scene_state.uniform_buffer);
	memdelete_arr(sky_scene_state.directional_lights);
	memdelete_arr(sky_scene_state.last_frame_directional_lights);
	material_storage->shader_free(sky_shader.default_shader);
	material_storage->material_free(sky_shader.default_material);
	material_storage->shader_free(sky_scene_state.fog_shader);
	material_storage->material_free(sky_scene_state.fog_material);

	if (RD::get_singleton()->uniform_set_is_valid(sky_scene_state.uniform_set)) {
		RD::get_singleton()->free_rid(sky_scene_state.uniform_set);
	}

	if (RD::get_singleton()->uniform_set_is_valid(sky_scene_state.default_fog_uniform_set)) {
		RD::get_singleton()->free_rid(sky_scene_state.default_fog_uniform_set);
	}

	if (sky_scene_state.fog_only_texture_uniform_set.is_valid() &&
		RD::get_singleton()->uniform_set_is_valid(sky_scene_state.fog_only_texture_uniform_set)) {
		RD::get_singleton()->free_rid(sky_scene_state.fog_only_texture_uniform_set);
	}

	if (sky_scene_state.fog_only_texture_multiview_uniform_set.is_valid() &&
		RD::get_singleton()->uniform_set_is_valid(
			sky_scene_state.fog_only_texture_multiview_uniform_set)) {
		RD::get_singleton()->free_rid(sky_scene_state.fog_only_texture_multiview_uniform_set);
	}
}

void SkyRD::setup_sky(const RenderDataRD* p_render_data, const Size2i p_screen_size)
{
	RendererRD::LightStorage* light_storage = RendererRD::LightStorage::get_singleton();
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

	Sky* sky = get_sky(
		RendererSceneRenderRD::get_singleton()->environment_get_sky(p_render_data->environment));
	if (sky) {
		// Save our screen size; our buffers will already have been cleared.
		sky->screen_size.x = p_screen_size.x < 4 ? 4 : p_screen_size.x;
		sky->screen_size.y = p_screen_size.y < 4 ? 4 : p_screen_size.y;

		RSE::SkyMode sky_mode = sky->mode;

		if (sky_mode == RSE::SKY_MODE_AUTOMATIC) {
			bool sun_scatter_enabled =
				RendererSceneRenderRD::get_singleton()->environment_get_fog_enabled(
					p_render_data->environment) &&
				RendererSceneRenderRD::get_singleton()->environment_get_fog_sun_scatter(
					p_render_data->environment) > 0.001;

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
			RenderingServerDefault::redraw_request();
		}

		if (RendererSceneRenderRD::get_singleton()->environment_get_fog_aerial_perspective(
				p_render_data->environment) != sky->prev_fog_aerial_perspective) {
			sky->prev_fog_aerial_perspective =
				RendererSceneRenderRD::get_singleton()->environment_get_fog_aerial_perspective(
					p_render_data->environment);
			sky->reflection.dirty = true;
			RenderingServerDefault::redraw_request();
		}

		if (RendererSceneRenderRD::get_singleton()->environment_get_fog_light_color(
				p_render_data->environment) != sky->prev_fog_light_color) {
			sky->prev_fog_light_color =
				RendererSceneRenderRD::get_singleton()->environment_get_fog_light_color(
					p_render_data->environment);
			sky->reflection.dirty = true;
			RenderingServerDefault::redraw_request();
		}

		if (RendererSceneRenderRD::get_singleton()->environment_get_fog_sun_scatter(
				p_render_data->environment) != sky->prev_fog_sun_scatter) {
			sky->prev_fog_sun_scatter =
				RendererSceneRenderRD::get_singleton()->environment_get_fog_sun_scatter(
					p_render_data->environment);
			sky->reflection.dirty = true;
			RenderingServerDefault::redraw_request();
		}

		if (RendererSceneRenderRD::get_singleton()->environment_get_fog_enabled(
				p_render_data->environment) != sky->prev_fog_enabled) {
			sky->prev_fog_enabled =
				RendererSceneRenderRD::get_singleton()->environment_get_fog_enabled(
					p_render_data->environment);
			sky->reflection.dirty = true;
			RenderingServerDefault::redraw_request();
		}

		if (RendererSceneRenderRD::get_singleton()->environment_get_fog_density(
				p_render_data->environment) != sky->prev_fog_density) {
			sky->prev_fog_density =
				RendererSceneRenderRD::get_singleton()->environment_get_fog_density(
					p_render_data->environment);
			sky->reflection.dirty = true;
			RenderingServerDefault::redraw_request();
		}

		if (RendererSceneRenderRD::get_singleton()->environment_get_fog_sky_affect(
				p_render_data->environment) != sky->prev_fog_sky_affect) {
			sky->prev_fog_sky_affect =
				RendererSceneRenderRD::get_singleton()->environment_get_fog_sky_affect(
					p_render_data->environment);
			sky->reflection.dirty = true;
			RenderingServerDefault::redraw_request();
		}

		if (RendererSceneRenderRD::get_singleton()->environment_get_fog_light_energy(
				p_render_data->environment) != sky->prev_fog_light_energy) {
			sky->prev_fog_light_energy =
				RendererSceneRenderRD::get_singleton()->environment_get_fog_light_energy(
					p_render_data->environment);
			sky->reflection.dirty = true;
			RenderingServerDefault::redraw_request();
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
		RendererSceneRenderRD::get_singleton()->environment_get_fog_enabled(
			p_render_data->environment) &&
		RendererSceneRenderRD::get_singleton()->environment_get_fog_sun_scatter(
			p_render_data->environment) > 0.001;
	sky_scene_state.ubo.directional_light_count = 0;
	if (shader_data->uses_light || sun_scatter_enabled) {
		const PagedArray<RID>& lights = *p_render_data->lights;
		// Run through the list of lights in the scene and pick out the Directional Lights.
		// This can't be done in RenderSceneRenderRD::_setup lights because that needs to be called
		// after the depth prepass, but this runs before the depth prepass.
		for (int i = 0; i < (int)lights.size(); i++) {
			if (!light_storage->owns_light_instance(lights[i])) {
				continue;
			}
			RID base = light_storage->light_instance_get_base_light(lights[i]);

			ERR_CONTINUE(base.is_null());

			RSE::LightType type = light_storage->light_get_type(base);
			if (type == RSE::LIGHT_DIRECTIONAL &&
				light_storage->light_directional_get_sky_mode(base) !=
					RSE::LIGHT_DIRECTIONAL_SKY_MODE_LIGHT_ONLY) {
				SkyDirectionalLightData& sky_light_data =
					sky_scene_state.directional_lights[sky_scene_state.ubo.directional_light_count];
				Transform3D light_transform =
					light_storage->light_instance_get_base_transform(lights[i]);
				Vector3 world_direction =
					light_transform.basis.xform(Vector3(0, 0, 1)).normalized();

				sky_light_data.direction[0] = world_direction.x;
				sky_light_data.direction[1] = world_direction.y;
				sky_light_data.direction[2] = world_direction.z;

				float sign = light_storage->light_is_negative(base) ? -1 : 1;
				sky_light_data.energy =
					sign * light_storage->light_get_param(base, RSE::LIGHT_PARAM_ENERGY);

				if (RendererSceneRenderRD::get_singleton()->is_using_physical_light_units()) {
					sky_light_data.energy *=
						light_storage->light_get_param(base, RSE::LIGHT_PARAM_INTENSITY);
				}

				if (p_render_data->camera_attributes.is_valid()) {
					sky_light_data.energy *=
						RSG::camera_attributes->camera_attributes_get_exposure_normalization_factor(
							p_render_data->camera_attributes);
				}

				Color linear_col = light_storage->light_get_color(base).srgb_to_linear();
				sky_light_data.color[0] = linear_col.r;
				sky_light_data.color[1] = linear_col.g;
				sky_light_data.color[2] = linear_col.b;

				sky_light_data.enabled = true;

				float angular_diameter =
					light_storage->light_get_param(base, RSE::LIGHT_PARAM_SIZE);
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

	float custom_fov = RendererSceneRenderRD::get_singleton()->environment_get_sky_custom_fov(
		p_render_data->environment);

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
		RendererSceneRenderRD::get_singleton()->environment_get_fog_enabled(
			p_render_data->environment);
	sky_scene_state.ubo.fog_density =
		RendererSceneRenderRD::get_singleton()->environment_get_fog_density(
			p_render_data->environment);
	sky_scene_state.ubo.fog_aerial_perspective =
		RendererSceneRenderRD::get_singleton()->environment_get_fog_aerial_perspective(
			p_render_data->environment);
	Color fog_color = RendererSceneRenderRD::get_singleton()
						  ->environment_get_fog_light_color(p_render_data->environment)
						  .srgb_to_linear();
	float fog_energy = RendererSceneRenderRD::get_singleton()->environment_get_fog_light_energy(
		p_render_data->environment);
	sky_scene_state.ubo.fog_light_color[0] = fog_color.r * fog_energy;
	sky_scene_state.ubo.fog_light_color[1] = fog_color.g * fog_energy;
	sky_scene_state.ubo.fog_light_color[2] = fog_color.b * fog_energy;
	sky_scene_state.ubo.fog_sun_scatter =
		RendererSceneRenderRD::get_singleton()->environment_get_fog_sun_scatter(
			p_render_data->environment);

	sky_scene_state.ubo.fog_sky_affect =
		RendererSceneRenderRD::get_singleton()->environment_get_fog_sky_affect(
			p_render_data->environment);
	sky_scene_state.ubo.volumetric_fog_sky_affect =
		RendererSceneRenderRD::get_singleton()->environment_get_volumetric_fog_sky_affect(
			p_render_data->environment);
	sky_scene_state.ubo.fog_use_legacy_blending =
		RendererSceneRenderRD::get_singleton()->fog_use_legacy_blending_get();
}

void SkyRD::draw_sky(RD::DrawListID p_draw_list, Ref<RenderSceneBuffersRD> p_render_buffers,
	RID p_env, RID p_fb, double p_time, float p_luminance_multiplier, float p_brightness_multiplier)
{
	ERR_FAIL_COND(p_render_buffers.is_null());
	ERR_FAIL_COND(p_env.is_null());

	Sky* sky = get_sky(RendererSceneRenderRD::get_singleton()->environment_get_sky(p_env));

	SkyMaterialData* material_data = _get_sky_material_data(p_env);
	ERR_FAIL_NULL(material_data);

	SkyShaderData* shader_data = material_data->shader_data;
	ERR_FAIL_NULL(shader_data);

	material_data->set_as_used();

	Basis sky_transform =
		RendererSceneRenderRD::get_singleton()->environment_get_sky_orientation(p_env);
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
	bool use_raster_effect = (RendererRD::CopyEffects::get_singleton()->get_raster_effects() &
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
				RD::TextureFormat tf;
				tf.array_layers = layers;
				tf.format = texture_format;
				tf.texture_type = RD::TEXTURE_TYPE_2D_ARRAY;
				tf.mipmaps = mipmaps;
				tf.width = w;
				tf.height = h;
				tf.usage_bits =
					RD::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT | RD::TEXTURE_USAGE_SAMPLING_BIT;
				if (!use_raster_effect) {
					tf.usage_bits |= RD::TEXTURE_USAGE_STORAGE_BIT;
				}

				sky->radiance = RD::get_singleton()->texture_create(tf, RD::TextureView());

				// Create view into the first layer slice for user shaders.
				sky->radiance_first_layer_slice =
					RD::get_singleton()->texture_create_shared_from_slice(
						RD::TextureView(), sky->radiance, 0, 0, mipmaps, RD::TEXTURE_SLICE_2D, 1);

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
				RD::TextureFormat tf;
				tf.format = texture_format;
				tf.mipmaps = MIN(mipmaps, layers);
				tf.width = w;
				tf.height = h;
				tf.usage_bits =
					RD::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT | RD::TEXTURE_USAGE_SAMPLING_BIT;
				if (!use_raster_effect) {
					tf.usage_bits |= RD::TEXTURE_USAGE_STORAGE_BIT;
				}

				sky->radiance = RD::get_singleton()->texture_create(tf, RD::TextureView());

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

	RendererRD::MaterialStorage* material_storage = RendererRD::MaterialStorage::get_singleton();
	Sky* sky = get_sky(RendererSceneRenderRD::get_singleton()->environment_get_sky(p_env));
	RSE::EnvironmentBG background =
		RendererSceneRenderRD::get_singleton()->environment_get_background(p_env);

	SkyMaterialData* material_data = nullptr;
	RID sky_material;

	if (background == RSE::ENV_BG_CLEAR_COLOR || background == RSE::ENV_BG_COLOR) {
		sky_material = sky_scene_state.fog_material;
		material_data = static_cast<SkyMaterialData*>(material_storage->material_get_data(
			sky_material, RendererRD::MaterialStorage::SHADER_TYPE_SKY));
	}
	else if (sky) {
		sky_material =
			sky_get_material(RendererSceneRenderRD::get_singleton()->environment_get_sky(p_env));

		if (sky_material.is_valid()) {
			material_data = static_cast<SkyMaterialData*>(material_storage->material_get_data(
				sky_material, RendererRD::MaterialStorage::SHADER_TYPE_SKY));
			if (!material_data || !material_data->shader_data->valid) {
				material_data = nullptr;
			}
		}
	}

	if (!material_data) {
		sky_material = sky_shader.default_material;
		material_data = static_cast<SkyMaterialData*>(material_storage->material_get_data(
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


