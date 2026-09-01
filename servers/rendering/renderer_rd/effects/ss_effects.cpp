/**************************************************************************/
/*  ss_effects.cpp                                                        */
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
#include "servers/rendering/renderer_rd/effects/copy_effects.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/render_scene_buffers_rd.h"
#include "servers/rendering/renderer_rd/uniform_set_cache_rd.h"
#include "ss_effects.h"

using namespace RendererRD;

SSEffects* SSEffects::singleton = nullptr;

static _FORCE_INLINE_ void store_camera(const Projection& p_mtx, float* p_array)
{
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			p_array[i * 4 + j] = p_mtx.columns[i][j];
		}
	}
}

void SSEffects::allocate_last_frame_buffer(
	Ref<RenderSceneBuffersRD> p_render_buffers, bool p_use_ssil, bool p_use_ssr)
{
	Size2i last_frame_size = p_render_buffers->get_internal_size();
	uint32_t mipmaps = 1;
	uint32_t view_count = p_render_buffers->get_view_count();

	if (!p_use_ssil && p_use_ssr && ssr_half_size) {
		last_frame_size /= 2;
	}

	if (p_use_ssil) {
		mipmaps = 6;
	}

	bool should_create = true;
	bool has_texture = p_render_buffers->has_texture(RB_SCOPE_SSLF, RB_LAST_FRAME);

	if (has_texture) {
		RID last_frame_texture = p_render_buffers->get_texture(RB_SCOPE_SSLF, RB_LAST_FRAME);
		RD::TextureFormat texture_format =
			RD::get_singleton()->texture_get_format(last_frame_texture);
		should_create = texture_format.width != (uint32_t)last_frame_size.width ||
						texture_format.height != (uint32_t)last_frame_size.height ||
						texture_format.mipmaps != mipmaps ||
						texture_format.array_layers != view_count;
	}

	if (should_create) {
		if (has_texture) {
			p_render_buffers->clear_context(RB_SCOPE_SSLF);
		}

		RID last_frame_texture = p_render_buffers->create_texture(RB_SCOPE_SSLF, RB_LAST_FRAME,
			RD::DATA_FORMAT_R16G16B16A16_SFLOAT,
			RD::TEXTURE_USAGE_SAMPLING_BIT | RD::TEXTURE_USAGE_STORAGE_BIT |
				RD::TEXTURE_USAGE_CAN_COPY_TO_BIT,
			RD::TEXTURE_SAMPLES_1, last_frame_size, view_count, mipmaps);
		RD::get_singleton()->texture_clear(
			last_frame_texture, Color(0, 0, 0, 0), 0, mipmaps, 0, view_count);
	}
}

void SSEffects::copy_internal_texture_to_last_frame(
	Ref<RenderSceneBuffersRD> p_render_buffers, CopyEffects& p_copy_effects)
{
	uint32_t mipmaps = p_render_buffers->get_texture_format(RB_SCOPE_SSLF, RB_LAST_FRAME).mipmaps;
	for (uint32_t v = 0; v < p_render_buffers->get_view_count(); v++) {
		for (uint32_t m = 0; m < mipmaps; m++) {
			RID source;
			if (m == 0) {
				source = p_render_buffers->get_internal_texture(v);
			}
			else {
				source =
					p_render_buffers->get_texture_slice(RB_SCOPE_SSLF, RB_LAST_FRAME, v, m - 1);
			}

			RID dest = p_render_buffers->get_texture_slice(RB_SCOPE_SSLF, RB_LAST_FRAME, v, m);

			Size2i source_size = RD::get_singleton()->texture_size(source);
			Size2i dest_size = RD::get_singleton()->texture_size(dest);

			if (m == 0 && source_size == dest_size) {
				p_copy_effects.copy_to_rect(source, dest, Rect2i(Vector2i(), source_size), false,
					false, false, false, false, true);
			}
			else {
				p_copy_effects.make_mipmap(source, dest, dest_size);
			}
		}
	}
}

SSEffects::~SSEffects()
{
	{
		// Cleanup SS Reflections
		for (int i = 0; i < SCREEN_SPACE_REFLECTION_DOWNSAMPLE_MAX; i++) {
			ssr.downsample_pipelines[i].free();
		}
		for (int i = 0; i < SCREEN_SPACE_REFLECTION_HIZ_MAX; i++) {
			ssr.hiz_pipelines[i].free();
		}
		ssr.ssr_pipeline.free();
		ssr.filter_pipeline.free();
		ssr.resolve_pipeline.free();

		ssr.downsample_shader.version_free(ssr.downsample_shader_version);
		ssr.hiz_shader.version_free(ssr.hiz_shader_version);
		ssr.ssr_shader.version_free(ssr.ssr_shader_version);
		ssr.filter_shader.version_free(ssr.filter_shader_version);
		ssr.resolve_shader.version_free(ssr.resolve_shader_version);

		if (ssr.ubo.is_valid()) {
			RD::get_singleton()->free_rid(ssr.ubo);
		}
	}

	{
		// Cleanup SS downsampler
		for (int i = 0; i < SS_EFFECTS_MAX; i++) {
			ss_effects.pipelines[i].free();
		}

		ss_effects.downsample_shader.version_free(ss_effects.downsample_shader_version);

		RD::get_singleton()->free_rid(ss_effects.mirror_sampler);
		RD::get_singleton()->free_rid(ss_effects.gather_constants_buffer);
	}

	{
		// Cleanup SSIL
		for (int i = 0; i < SSIL_MAX; i++) {
			ssil.pipelines[i].free();
		}

		ssil.blur_shader.version_free(ssil.blur_shader_version);
		ssil.gather_shader.version_free(ssil.gather_shader_version);
		ssil.interleave_shader.version_free(ssil.interleave_shader_version);
		ssil.importance_map_shader.version_free(ssil.importance_map_shader_version);

		RD::get_singleton()->free_rid(ssil.importance_map_load_counter);
		RD::get_singleton()->free_rid(ssil.projection_uniform_buffer);
	}

	{
		// Cleanup SSAO
		for (int i = 0; i < SSAO_MAX; i++) {
			ssao.pipelines[i].free();
		}

		ssao.blur_shader.version_free(ssao.blur_shader_version);
		ssao.gather_shader.version_free(ssao.gather_shader_version);
		ssao.interleave_shader.version_free(ssao.interleave_shader_version);
		ssao.importance_map_shader.version_free(ssao.importance_map_shader_version);

		RD::get_singleton()->free_rid(ssao.importance_map_load_counter);
	}

	{
		// Cleanup Subsurface scattering
		for (int i = 0; i < SUBSURFACE_SCATTERING_MODE_MAX; i++) {
			sss.pipelines[i].free();
		}

		sss.shader.version_free(sss.shader_version);
	}

	singleton = nullptr;
}

/* SSIL */

void SSEffects::ssil_set_quality(RSE::EnvironmentSSILQuality p_quality, bool p_half_size,
	float p_adaptive_target, int p_blur_passes, float p_fadeout_from, float p_fadeout_to)
{
	ssil_quality = p_quality;
	ssil_half_size = p_half_size;
	ssil_adaptive_target = p_adaptive_target;
	ssil_blur_passes = p_blur_passes;
	ssil_fadeout_from = p_fadeout_from;
	ssil_fadeout_to = p_fadeout_to;
}

void SSEffects::ssil_allocate_buffers(Ref<RenderSceneBuffersRD> p_render_buffers,
	SSILRenderBuffers& p_ssil_buffers, const SSILSettings& p_settings)
{
	if (p_ssil_buffers.half_size != ssil_half_size) {
		p_render_buffers->clear_context(RB_SCOPE_SSIL);
	}

	p_ssil_buffers.half_size = ssil_half_size;
	if (p_ssil_buffers.half_size) {
		p_ssil_buffers.buffer_width = (p_settings.full_screen_size.x + 3) / 4;
		p_ssil_buffers.buffer_height = (p_settings.full_screen_size.y + 3) / 4;
		p_ssil_buffers.half_buffer_width = (p_settings.full_screen_size.x + 7) / 8;
		p_ssil_buffers.half_buffer_height = (p_settings.full_screen_size.y + 7) / 8;
	}
	else {
		p_ssil_buffers.buffer_width = (p_settings.full_screen_size.x + 1) / 2;
		p_ssil_buffers.buffer_height = (p_settings.full_screen_size.y + 1) / 2;
		p_ssil_buffers.half_buffer_width = (p_settings.full_screen_size.x + 3) / 4;
		p_ssil_buffers.half_buffer_height = (p_settings.full_screen_size.y + 3) / 4;
	}

	uint32_t view_count = p_render_buffers->get_view_count();
	Size2i full_size = Size2i(p_ssil_buffers.buffer_width, p_ssil_buffers.buffer_height);
	Size2i half_size = Size2i(p_ssil_buffers.half_buffer_width, p_ssil_buffers.half_buffer_height);

	// We create our intermediate and final results as render buffers.
	// These are automatically cached and cleaned up when our viewport resizes
	// or when our viewport gets destroyed.

	if (!p_render_buffers->has_texture(
			RB_SCOPE_SSIL, RB_FINAL)) { // We don't strictly have to check if it exists but we only
										// want to clear it when we create it...
		RID final = p_render_buffers->create_texture(RB_SCOPE_SSIL, RB_FINAL,
			RD::DATA_FORMAT_R16G16B16A16_SFLOAT,
			RD::TEXTURE_USAGE_SAMPLING_BIT | RD::TEXTURE_USAGE_STORAGE_BIT |
				RD::TEXTURE_USAGE_CAN_COPY_TO_BIT);
		RD::get_singleton()->texture_clear(final, Color(0, 0, 0, 0), 0, 1, 0, view_count);
	}

	// As we're not clearing these, and render buffers will return the cached texture if it already
	// exists, we don't first check has_texture here

	p_render_buffers->create_texture(RB_SCOPE_SSIL, RB_DEINTERLEAVED,
		RD::DATA_FORMAT_R16G16B16A16_SFLOAT,
		RD::TEXTURE_USAGE_SAMPLING_BIT | RD::TEXTURE_USAGE_STORAGE_BIT, RD::TEXTURE_SAMPLES_1,
		full_size, 4 * view_count);
	p_render_buffers->create_texture(RB_SCOPE_SSIL, RB_DEINTERLEAVED_PONG,
		RD::DATA_FORMAT_R16G16B16A16_SFLOAT,
		RD::TEXTURE_USAGE_SAMPLING_BIT | RD::TEXTURE_USAGE_STORAGE_BIT, RD::TEXTURE_SAMPLES_1,
		full_size, 4 * view_count);
	p_render_buffers->create_texture(RB_SCOPE_SSIL, RB_EDGES, RD::DATA_FORMAT_R8_UNORM,
		RD::TEXTURE_USAGE_SAMPLING_BIT | RD::TEXTURE_USAGE_STORAGE_BIT, RD::TEXTURE_SAMPLES_1,
		full_size, 4 * view_count);
	p_render_buffers->create_texture(RB_SCOPE_SSIL, RB_IMPORTANCE_MAP, RD::DATA_FORMAT_R8_UNORM,
		RD::TEXTURE_USAGE_SAMPLING_BIT | RD::TEXTURE_USAGE_STORAGE_BIT, RD::TEXTURE_SAMPLES_1,
		half_size);
	p_render_buffers->create_texture(RB_SCOPE_SSIL, RB_IMPORTANCE_PONG, RD::DATA_FORMAT_R8_UNORM,
		RD::TEXTURE_USAGE_SAMPLING_BIT | RD::TEXTURE_USAGE_STORAGE_BIT, RD::TEXTURE_SAMPLES_1,
		half_size);
}

void SSEffects::ssao_set_quality(RSE::EnvironmentSSAOQuality p_quality, bool p_half_size,
	float p_adaptive_target, int p_blur_passes, float p_fadeout_from, float p_fadeout_to)
{
	ssao_quality = p_quality;
	ssao_half_size = p_half_size;
	ssao_adaptive_target = p_adaptive_target;
	ssao_blur_passes = p_blur_passes;
	ssao_fadeout_from = p_fadeout_from;
	ssao_fadeout_to = p_fadeout_to;
}

void SSEffects::ssao_allocate_buffers(Ref<RenderSceneBuffersRD> p_render_buffers,
	SSAORenderBuffers& p_ssao_buffers, const SSAOSettings& p_settings)
{
	if (p_ssao_buffers.half_size != ssao_half_size) {
		p_render_buffers->clear_context(RB_SCOPE_SSAO);
	}

	p_ssao_buffers.half_size = ssao_half_size;
	if (ssao_half_size) {
		p_ssao_buffers.buffer_width = (p_settings.full_screen_size.x + 3) / 4;
		p_ssao_buffers.buffer_height = (p_settings.full_screen_size.y + 3) / 4;
		p_ssao_buffers.half_buffer_width = (p_settings.full_screen_size.x + 7) / 8;
		p_ssao_buffers.half_buffer_height = (p_settings.full_screen_size.y + 7) / 8;
	}
	else {
		p_ssao_buffers.buffer_width = (p_settings.full_screen_size.x + 1) / 2;
		p_ssao_buffers.buffer_height = (p_settings.full_screen_size.y + 1) / 2;
		p_ssao_buffers.half_buffer_width = (p_settings.full_screen_size.x + 3) / 4;
		p_ssao_buffers.half_buffer_height = (p_settings.full_screen_size.y + 3) / 4;
	}

	uint32_t view_count = p_render_buffers->get_view_count();
	Size2i full_size = Size2i(p_ssao_buffers.buffer_width, p_ssao_buffers.buffer_height);
	Size2i half_size = Size2i(p_ssao_buffers.half_buffer_width, p_ssao_buffers.half_buffer_height);

	// As we're not clearing these, and render buffers will return the cached texture if it already
	// exists, we don't first check has_texture here

	p_render_buffers->create_texture(RB_SCOPE_SSAO, RB_DEINTERLEAVED, RD::DATA_FORMAT_R8G8_UNORM,
		RD::TEXTURE_USAGE_SAMPLING_BIT | RD::TEXTURE_USAGE_STORAGE_BIT, RD::TEXTURE_SAMPLES_1,
		full_size, 4 * view_count);
	p_render_buffers->create_texture(RB_SCOPE_SSAO, RB_DEINTERLEAVED_PONG,
		RD::DATA_FORMAT_R8G8_UNORM, RD::TEXTURE_USAGE_SAMPLING_BIT | RD::TEXTURE_USAGE_STORAGE_BIT,
		RD::TEXTURE_SAMPLES_1, full_size, 4 * view_count);
	p_render_buffers->create_texture(RB_SCOPE_SSAO, RB_IMPORTANCE_MAP, RD::DATA_FORMAT_R8_UNORM,
		RD::TEXTURE_USAGE_SAMPLING_BIT | RD::TEXTURE_USAGE_STORAGE_BIT, RD::TEXTURE_SAMPLES_1,
		half_size);
	p_render_buffers->create_texture(RB_SCOPE_SSAO, RB_IMPORTANCE_PONG, RD::DATA_FORMAT_R8_UNORM,
		RD::TEXTURE_USAGE_SAMPLING_BIT | RD::TEXTURE_USAGE_STORAGE_BIT, RD::TEXTURE_SAMPLES_1,
		half_size);
	p_render_buffers->create_texture(RB_SCOPE_SSAO, RB_FINAL, RD::DATA_FORMAT_R8_UNORM,
		RD::TEXTURE_USAGE_SAMPLING_BIT | RD::TEXTURE_USAGE_STORAGE_BIT, RD::TEXTURE_SAMPLES_1);
}

void SSEffects::ssr_set_half_size(bool p_half_size) { ssr_half_size = p_half_size; }

void SSEffects::ssr_allocate_buffers(Ref<RenderSceneBuffersRD> p_render_buffers,
	SSRRenderBuffers& p_ssr_buffers, const RD::DataFormat p_color_format)
{
	if (p_ssr_buffers.half_size != ssr_half_size) {
		p_render_buffers->clear_context(RB_SCOPE_SSR);
	}

	Vector2i internal_size = p_render_buffers->get_internal_size();
	p_ssr_buffers.size = ssr_half_size ? (internal_size / 2) : internal_size;

	uint32_t cur_width = p_ssr_buffers.size.width;
	uint32_t cur_height = p_ssr_buffers.size.height;
	p_ssr_buffers.mipmaps = 1;

	while (cur_width > 1 && cur_height > 1) {
		if (cur_width > 1) {
			cur_width /= 2;
		}
		if (cur_height > 1) {
			cur_height /= 2;
		}
		++p_ssr_buffers.mipmaps;
	}

	p_ssr_buffers.half_size = ssr_half_size;

	uint32_t view_count = p_render_buffers->get_view_count();

	if (ssr_half_size) {
		p_render_buffers->create_texture(RB_SCOPE_SSR, RB_NORMAL_ROUGHNESS,
			RD::DATA_FORMAT_R8G8B8A8_UNORM,
			RD::TEXTURE_USAGE_SAMPLING_BIT | RD::TEXTURE_USAGE_STORAGE_BIT, RD::TEXTURE_SAMPLES_1,
			p_ssr_buffers.size, view_count);
	}

	p_render_buffers->create_texture(RB_SCOPE_SSR, RB_HIZ, RD::DATA_FORMAT_R32_SFLOAT,
		RD::TEXTURE_USAGE_SAMPLING_BIT | RD::TEXTURE_USAGE_STORAGE_BIT, RD::TEXTURE_SAMPLES_1,
		p_ssr_buffers.size, view_count, p_ssr_buffers.mipmaps);
	p_render_buffers->create_texture(RB_SCOPE_SSR, RB_SSR, p_color_format,
		RD::TEXTURE_USAGE_SAMPLING_BIT | RD::TEXTURE_USAGE_STORAGE_BIT, RD::TEXTURE_SAMPLES_1,
		p_ssr_buffers.size, view_count, p_ssr_buffers.mipmaps);
	p_render_buffers->create_texture(RB_SCOPE_SSR, RB_MIP_LEVEL, RD::DATA_FORMAT_R8_UNORM,
		RD::TEXTURE_USAGE_SAMPLING_BIT | RD::TEXTURE_USAGE_STORAGE_BIT, RD::TEXTURE_SAMPLES_1,
		p_ssr_buffers.size, view_count);

	if (ssr_half_size) {
		p_render_buffers->create_texture(RB_SCOPE_SSR, RB_FINAL, p_color_format,
			RD::TEXTURE_USAGE_SAMPLING_BIT | RD::TEXTURE_USAGE_STORAGE_BIT, RD::TEXTURE_SAMPLES_1,
			internal_size, view_count);
	}
}

void SSEffects::sss_set_quality(RSE::SubSurfaceScatteringQuality p_quality)
{
	sss_quality = p_quality;
}

RSE::SubSurfaceScatteringQuality SSEffects::sss_get_quality() const { return sss_quality; }

void SSEffects::sss_set_scale(float p_scale, float p_depth_scale)
{
	sss_scale = p_scale;
	sss_depth_scale = p_depth_scale;
}


