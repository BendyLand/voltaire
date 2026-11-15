/**************************************************************************/
/*  ss_effects.cpp                                                        */
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

#include "core/config/project_settings.h"
#include "servers/rendering/renderer_rd/effects/copy_effects.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/render_scene_buffers_rd.h"
#include "servers/rendering/renderer_rd/uniform_set_cache_rd.h"
#include "ss_effects.h"

using namespace RendererRD;

static _FORCE_INLINE_ void store_camera(const Projection& p_mtx, float* p_array)
{
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			p_array[i * 4 + j] = p_mtx.columns[i][j];
		}
	}
}

void SSEffects::initialize()
{
	if (data) {
		return;
	}
	data = memnew(Data);
}

void SSEffects::finalize()
{
	if (!data) {
		return;
	}

	{
		// Cleanup SS Reflections
		for (int i = 0; i < SCREEN_SPACE_REFLECTION_DOWNSAMPLE_MAX; i++) {
			data->ssr.downsample_pipelines[i].free();
		}
		for (int i = 0; i < SCREEN_SPACE_REFLECTION_HIZ_MAX; i++) {
			data->ssr.hiz_pipelines[i].free();
		}
		data->ssr.ssr_pipeline.free();
		data->ssr.filter_pipeline.free();
		data->ssr.resolve_pipeline.free();

		if (data->ssr.downsample_shader_version.is_valid()) {
			data->ssr.downsample_shader.version_free(data->ssr.downsample_shader_version);
		}
		if (data->ssr.hiz_shader_version.is_valid()) {
			data->ssr.hiz_shader.version_free(data->ssr.hiz_shader_version);
		}
		if (data->ssr.ssr_shader_version.is_valid()) {
			data->ssr.ssr_shader.version_free(data->ssr.ssr_shader_version);
		}
		if (data->ssr.filter_shader_version.is_valid()) {
			data->ssr.filter_shader.version_free(data->ssr.filter_shader_version);
		}
		if (data->ssr.resolve_shader_version.is_valid()) {
			data->ssr.resolve_shader.version_free(data->ssr.resolve_shader_version);
		}

		if (data->ssr.ubo.is_valid()) {
			RD::free_rid(data->ssr.ubo);
		}
	}

	{
		// Cleanup SS downsampler
		for (int i = 0; i < SS_EFFECTS_MAX; i++) {
			data->ss_effects.pipelines[i].free();
		}

		if (data->ss_effects.downsample_shader_version.is_valid()) {
			data->ss_effects.downsample_shader.version_free(
				data->ss_effects.downsample_shader_version);
		}

		if (data->ss_effects.mirror_sampler.is_valid()) {
			RD::free_rid(data->ss_effects.mirror_sampler);
		}
		if (data->ss_effects.gather_constants_buffer.is_valid()) {
			RD::free_rid(data->ss_effects.gather_constants_buffer);
		}
	}

	{
		// Cleanup SSIL
		for (int i = 0; i < SSIL_MAX; i++) {
			data->ssil.pipelines[i].free();
		}

		if (data->ssil.blur_shader_version.is_valid()) {
			data->ssil.blur_shader.version_free(data->ssil.blur_shader_version);
		}
		if (data->ssil.gather_shader_version.is_valid()) {
			data->ssil.gather_shader.version_free(data->ssil.gather_shader_version);
		}
		if (data->ssil.interleave_shader_version.is_valid()) {
			data->ssil.interleave_shader.version_free(data->ssil.interleave_shader_version);
		}
		if (data->ssil.importance_map_shader_version.is_valid()) {
			data->ssil.importance_map_shader.version_free(data->ssil.importance_map_shader_version);
		}

		if (data->ssil.importance_map_load_counter.is_valid()) {
			RD::free_rid(data->ssil.importance_map_load_counter);
		}
		if (data->ssil.projection_uniform_buffer.is_valid()) {
			RD::free_rid(data->ssil.projection_uniform_buffer);
		}
	}

	{
		// Cleanup SSAO
		for (int i = 0; i < SSAO_MAX; i++) {
			data->ssao.pipelines[i].free();
		}

		if (data->ssao.blur_shader_version.is_valid()) {
			data->ssao.blur_shader.version_free(data->ssao.blur_shader_version);
		}
		if (data->ssao.gather_shader_version.is_valid()) {
			data->ssao.gather_shader.version_free(data->ssao.gather_shader_version);
		}
		if (data->ssao.interleave_shader_version.is_valid()) {
			data->ssao.interleave_shader.version_free(data->ssao.interleave_shader_version);
		}
		if (data->ssao.importance_map_shader_version.is_valid()) {
			data->ssao.importance_map_shader.version_free(data->ssao.importance_map_shader_version);
		}

		if (data->ssao.importance_map_load_counter.is_valid()) {
			RD::free_rid(data->ssao.importance_map_load_counter);
		}
	}

	{
		// Cleanup Subsurface scattering
		for (int i = 0; i < SUBSURFACE_SCATTERING_MODE_MAX; i++) {
			data->sss.pipelines[i].free();
		}

		if (data->sss.shader_version.is_valid()) {
			data->sss.shader.version_free(data->sss.shader_version);
		}
	}

	memdelete(data);
	data = nullptr;
}

void SSEffects::allocate_last_frame_buffer(
	Ref<RenderSceneBuffersRD> p_render_buffers, bool p_use_ssil, bool p_use_ssr)
{
	ERR_FAIL_NULL(data);

	Size2i last_frame_size = p_render_buffers->get_internal_size();
	uint32_t mipmaps = 1;
	uint32_t view_count = p_render_buffers->get_view_count();

	if (!p_use_ssil && p_use_ssr && data->ssr_half_size) {
		last_frame_size /= 2;
	}

	if (p_use_ssil) {
		mipmaps = 6;
	}

	bool should_create = true;
	bool has_texture = p_render_buffers->has_texture(RB_SCOPE_SSLF, RB_LAST_FRAME);

	if (has_texture) {
		RID last_frame_texture = p_render_buffers->get_texture(RB_SCOPE_SSLF, RB_LAST_FRAME);
		RDC::TextureFormat texture_format = RD::texture_get_format(last_frame_texture);
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
			RDC::DATA_FORMAT_R16G16B16A16_SFLOAT,
			RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT |
				RDC::TEXTURE_USAGE_CAN_COPY_TO_BIT,
			RDC::TEXTURE_SAMPLES_1, last_frame_size, view_count, mipmaps);
		RD::texture_clear(last_frame_texture, Color(0, 0, 0, 0), 0, mipmaps, 0, view_count);
	}
}

void SSEffects::copy_internal_texture_to_last_frame(Ref<RenderSceneBuffersRD> p_render_buffers)
{
	ERR_FAIL_NULL(data);

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

			Size2i source_size = RD::texture_size(source);
			Size2i dest_size = RD::texture_size(dest);

			if (m == 0 && source_size == dest_size) {
				CopyEffects::copy_to_rect(source, dest, Rect2i(Vector2i(), source_size), false,
					false, false, false, false, true);
			}
			else {
				CopyEffects::make_mipmap(source, dest, dest_size);
			}
		}
	}
}

/* SSIL */

void SSEffects::ssil_set_quality(RSE::EnvironmentSSILQuality p_quality, bool p_half_size,
	float p_adaptive_target, int p_blur_passes, float p_fadeout_from, float p_fadeout_to)
{
	ERR_FAIL_NULL(data);

	data->ssil_quality = p_quality;
	data->ssil_half_size = p_half_size;
	data->ssil_adaptive_target = p_adaptive_target;
	data->ssil_blur_passes = p_blur_passes;
	data->ssil_fadeout_from = p_fadeout_from;
	data->ssil_fadeout_to = p_fadeout_to;
}

void SSEffects::ssil_allocate_buffers(Ref<RenderSceneBuffersRD> p_render_buffers,
	SSILRenderBuffers& p_ssil_buffers, const SSILSettings& p_settings)
{
	ERR_FAIL_NULL(data);

	if (p_ssil_buffers.half_size != data->ssil_half_size) {
		p_render_buffers->clear_context(RB_SCOPE_SSIL);
	}

	p_ssil_buffers.half_size = data->ssil_half_size;
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
			RDC::DATA_FORMAT_R16G16B16A16_SFLOAT,
			RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT |
				RDC::TEXTURE_USAGE_CAN_COPY_TO_BIT);
		RD::texture_clear(final, Color(0, 0, 0, 0), 0, 1, 0, view_count);
	}

	// As we're not clearing these, and render buffers will return the cached texture if it already
	// exists, we don't first check has_texture here

	p_render_buffers->create_texture(RB_SCOPE_SSIL, RB_DEINTERLEAVED,
		RDC::DATA_FORMAT_R16G16B16A16_SFLOAT,
		RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT, RDC::TEXTURE_SAMPLES_1,
		full_size, 4 * view_count);
	p_render_buffers->create_texture(RB_SCOPE_SSIL, RB_DEINTERLEAVED_PONG,
		RDC::DATA_FORMAT_R16G16B16A16_SFLOAT,
		RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT, RDC::TEXTURE_SAMPLES_1,
		full_size, 4 * view_count);
	p_render_buffers->create_texture(RB_SCOPE_SSIL, RB_EDGES, RDC::DATA_FORMAT_R8_UNORM,
		RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT, RDC::TEXTURE_SAMPLES_1,
		full_size, 4 * view_count);
	p_render_buffers->create_texture(RB_SCOPE_SSIL, RB_IMPORTANCE_MAP, RDC::DATA_FORMAT_R8_UNORM,
		RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT, RDC::TEXTURE_SAMPLES_1,
		half_size);
	p_render_buffers->create_texture(RB_SCOPE_SSIL, RB_IMPORTANCE_PONG, RDC::DATA_FORMAT_R8_UNORM,
		RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT, RDC::TEXTURE_SAMPLES_1,
		half_size);
}

void SSEffects::ssao_set_quality(RSE::EnvironmentSSAOQuality p_quality, bool p_half_size,
	float p_adaptive_target, int p_blur_passes, float p_fadeout_from, float p_fadeout_to)
{
	ERR_FAIL_NULL(data);

	data->ssao_quality = p_quality;
	data->ssao_half_size = p_half_size;
	data->ssao_adaptive_target = p_adaptive_target;
	data->ssao_blur_passes = p_blur_passes;
	data->ssao_fadeout_from = p_fadeout_from;
	data->ssao_fadeout_to = p_fadeout_to;
}

void SSEffects::ssao_allocate_buffers(Ref<RenderSceneBuffersRD> p_render_buffers,
	SSAORenderBuffers& p_ssao_buffers, const SSAOSettings& p_settings)
{
	ERR_FAIL_NULL(data);

	if (p_ssao_buffers.half_size != data->ssao_half_size) {
		p_render_buffers->clear_context(RB_SCOPE_SSAO);
	}

	p_ssao_buffers.half_size = data->ssao_half_size;
	if (data->ssao_half_size) {
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

	p_render_buffers->create_texture(RB_SCOPE_SSAO, RB_DEINTERLEAVED, RDC::DATA_FORMAT_R8G8_UNORM,
		RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT, RDC::TEXTURE_SAMPLES_1,
		full_size, 4 * view_count);
	p_render_buffers->create_texture(RB_SCOPE_SSAO, RB_DEINTERLEAVED_PONG,
		RDC::DATA_FORMAT_R8G8_UNORM, RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT,
		RDC::TEXTURE_SAMPLES_1, full_size, 4 * view_count);
	p_render_buffers->create_texture(RB_SCOPE_SSAO, RB_IMPORTANCE_MAP, RDC::DATA_FORMAT_R8_UNORM,
		RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT, RDC::TEXTURE_SAMPLES_1,
		half_size);
	p_render_buffers->create_texture(RB_SCOPE_SSAO, RB_IMPORTANCE_PONG, RDC::DATA_FORMAT_R8_UNORM,
		RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT, RDC::TEXTURE_SAMPLES_1,
		half_size);
	p_render_buffers->create_texture(RB_SCOPE_SSAO, RB_FINAL, RDC::DATA_FORMAT_R8_UNORM,
		RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT, RDC::TEXTURE_SAMPLES_1);
}

void SSEffects::ssr_set_half_size(bool p_half_size)
{
	ERR_FAIL_NULL(data);
	data->ssr_half_size = p_half_size;
}

void SSEffects::ssr_allocate_buffers(Ref<RenderSceneBuffersRD> p_render_buffers,
	SSRRenderBuffers& p_ssr_buffers, const RDC::DataFormat p_color_format)
{
	ERR_FAIL_NULL(data);

	if (p_ssr_buffers.half_size != data->ssr_half_size) {
		p_render_buffers->clear_context(RB_SCOPE_SSR);
	}

	Vector2i internal_size = p_render_buffers->get_internal_size();
	p_ssr_buffers.size = data->ssr_half_size ? (internal_size / 2) : internal_size;

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

	p_ssr_buffers.half_size = data->ssr_half_size;

	uint32_t view_count = p_render_buffers->get_view_count();

	if (data->ssr_half_size) {
		p_render_buffers->create_texture(RB_SCOPE_SSR, RB_NORMAL_ROUGHNESS,
			RDC::DATA_FORMAT_R8G8B8A8_UNORM,
			RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT, RDC::TEXTURE_SAMPLES_1,
			p_ssr_buffers.size, view_count);
	}

	p_render_buffers->create_texture(RB_SCOPE_SSR, RB_HIZ, RDC::DATA_FORMAT_R32_SFLOAT,
		RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT, RDC::TEXTURE_SAMPLES_1,
		p_ssr_buffers.size, view_count, p_ssr_buffers.mipmaps);
	p_render_buffers->create_texture(RB_SCOPE_SSR, RB_SSR, p_color_format,
		RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT, RDC::TEXTURE_SAMPLES_1,
		p_ssr_buffers.size, view_count, p_ssr_buffers.mipmaps);
	p_render_buffers->create_texture(RB_SCOPE_SSR, RB_MIP_LEVEL, RDC::DATA_FORMAT_R8_UNORM,
		RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT, RDC::TEXTURE_SAMPLES_1,
		p_ssr_buffers.size, view_count);

	if (data->ssr_half_size) {
		p_render_buffers->create_texture(RB_SCOPE_SSR, RB_FINAL, p_color_format,
			RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT, RDC::TEXTURE_SAMPLES_1,
			internal_size, view_count);
	}
}

void SSEffects::sss_set_quality(RSE::SubSurfaceScatteringQuality p_quality)
{
	ERR_FAIL_NULL(data);
	data->sss_quality = p_quality;
}

RSE::SubSurfaceScatteringQuality SSEffects::sss_get_quality()
{
	ERR_FAIL_NULL_V(data, RSE::SUB_SURFACE_SCATTERING_QUALITY_DISABLED);
	return data->sss_quality;
}

void SSEffects::sss_set_scale(float p_scale, float p_depth_scale)
{
	ERR_FAIL_NULL(data);
	data->sss_scale = p_scale;
	data->sss_depth_scale = p_depth_scale;
}

void SSEffects::sub_surface_scattering(Ref<RenderSceneBuffersRD> p_render_buffers, RID p_diffuse,
	RID p_depth, const Projection& p_camera, const Size2i& p_screen_size)
{
}

void SSEffects::screen_space_reflection(Ref<RenderSceneBuffersRD> p_render_buffers,
	SSRRenderBuffers& p_ssr_buffers, const RID* p_normal_roughness_slices, int p_max_steps,
	float p_fade_in, float p_fade_out, float p_tolerance, const Projection* p_projections,
	const Projection* p_reprojections, const Vector3* p_eye_offsets)
{
}

void SSEffects::screen_space_indirect_lighting(Ref<RenderSceneBuffersRD> p_render_buffers,
	SSILRenderBuffers& p_ssil_buffers, uint32_t p_view, RID p_normal_buffer,
	const Projection& p_projection, const Projection& p_last_projection,
	const SSILSettings& p_settings)
{
}

void SSEffects::generate_ssao(Ref<RenderSceneBuffersRD> p_render_buffers,
	SSAORenderBuffers& p_ssao_buffers, uint32_t p_view, RID p_normal_buffer,
	const Projection& p_projection, const SSAOSettings& p_settings)
{
}

void SSEffects::downsample_depth(
	Ref<RenderSceneBuffersRD> p_render_buffers, uint32_t p_view, const Projection& p_projection)
{
}

void SSEffects::gather_ssil(RD::ComputeListID p_compute_list, const RID* p_ssil_slices,
	const RID* p_edges_slices, const SSILSettings& p_settings, bool p_adaptive_base_pass,
	RID p_gather_uniform_set, RID p_importance_map_uniform_set, RID p_projection_uniform_set)
{
}

void SSEffects::gather_ssao(RD::ComputeListID p_compute_list, const RID* p_ao_slices,
	const SSAOSettings& p_settings, bool p_adaptive_base_pass, RID p_gather_uniform_set,
	RID p_importance_map_uniform_set)
{
}


