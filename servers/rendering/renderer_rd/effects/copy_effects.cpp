/**************************************************************************/
/*  copy_effects.cpp                                                      */
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

#include <thirdparty/misc/cubemap_coeffs.h>
#include "copy_effects.h"
#include "core/config/project_settings.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/renderer_rd/uniform_set_cache_rd.h"

using namespace RendererRD;

void CopyEffects::initialize(uint32_t p_raster_effects)
{
	if (data) {
		return;
	}
	data = memnew(Data);
	data->raster_effects = p_raster_effects;
}

void CopyEffects::finalize()
{
	if (!data) {
		return;
	}

	for (int i = 0; i < BLUR_MODE_MAX; i++) {
		data->blur_raster.pipelines[i].clear();
	}
	if (data->blur_raster.shader_version.is_valid()) {
		data->blur_raster.shader.version_free(data->blur_raster.shader_version);
	}
	if (data->blur_raster.glow_sampler.is_valid()) {
		RD::free_rid(data->blur_raster.glow_sampler);
	}

	for (int i = 0; i < COPY_MODE_MAX; i++) {
		data->copy.pipelines[i].free();
	}
	if (data->copy.shader_version.is_valid()) {
		data->copy.shader.version_free(data->copy.shader_version);
	}

	for (int i = 0; i < COPY_TO_FB_MAX; i++) {
		data->copy_to_fb.pipelines[i].clear();
	}
	if (data->copy_to_fb.shader_version.is_valid()) {
		data->copy_to_fb.shader.version_free(data->copy_to_fb.shader_version);
	}

	data->cube_to_dp.pipeline.clear();
	if (data->cube_to_dp.shader_version.is_valid()) {
		data->cube_to_dp.shader.version_free(data->cube_to_dp.shader_version);
	}

	data->cube_to_octmap.pipeline.clear();
	if (data->cube_to_octmap.shader_version.is_valid()) {
		data->cube_to_octmap.shader.version_free(data->cube_to_octmap.shader_version);
	}

	if (data->octmap_downsampler.shader_version.is_valid()) {
		if (data->raster_effects & RASTER_EFFECT_OCTMAP) {
			data->octmap_downsampler.raster_pipeline.clear();
			data->octmap_downsampler.raster_shader.version_free(
				data->octmap_downsampler.shader_version);
		}
		else {
			for (int i = 0; i < DOWNSAMPLER_MODE_COMPUTE_MAX; i++) {
				data->octmap_downsampler.compute_pipelines[i].free();
			}
			data->octmap_downsampler.compute_shader.version_free(
				data->octmap_downsampler.shader_version);
		}
	}

	if (data->filter.shader_version.is_valid()) {
		if (data->raster_effects & RASTER_EFFECT_OCTMAP) {
			for (int i = 0; i < FILTER_MODE_RASTER_MAX; i++) {
				data->filter.raster_pipelines[i].clear();
			}
			data->filter.raster_shader.version_free(data->filter.shader_version);
		}
		else {
			for (int i = 0; i < FILTER_MODE_COMPUTE_MAX; i++) {
				data->filter.compute_pipelines[i].free();
			}
			data->filter.compute_shader.version_free(data->filter.shader_version);
		}
	}
	if (data->filter.coefficient_buffer.is_valid()) {
		RD::free_rid(data->filter.coefficient_buffer);
	}

	if (data->roughness.shader_version.is_valid()) {
		if (data->raster_effects & RASTER_EFFECT_OCTMAP) {
			data->roughness.raster_pipeline.clear();
			data->roughness.raster_shader.version_free(data->roughness.shader_version);
		}
		else {
			for (int i = 0; i < ROUGHNESS_MODE_MAX; i++) {
				data->roughness.compute_pipelines[i].free();
			}
			data->roughness.compute_shader.version_free(data->roughness.shader_version);
		}
	}

	for (int i = 0; i < SPECULAR_MERGE_MAX; i++) {
		data->specular_merge.pipelines[i].clear();
	}
	if (data->specular_merge.shader_version.is_valid()) {
		data->specular_merge.shader.version_free(data->specular_merge.shader_version);
	}

	memdelete(data);
	data = nullptr;
}

void CopyEffects::copy_to_rect(RID p_source_rd_texture, RID p_dest_texture, const Rect2i& p_rect,
	bool p_flip_y, bool p_force_luminance, bool p_all_source, bool p_8_bit_dst, bool p_alpha_to_one,
	bool p_sanitize_inf_nan)
{
}

void CopyEffects::copy_octmap_to_panorama(RID p_source_octmap, RID p_dest_panorama,
	const Size2i& p_panorama_size, float p_lod, bool p_is_array,
	const Size2& p_source_octmap_border_size)
{
}

void CopyEffects::copy_depth_to_rect(RID p_source_rd_texture, RID p_dest_framebuffer,
	const Rect2i& p_rect, bool p_flip_y)
{
}

void CopyEffects::copy_depth_to_rect_and_linearize(RID p_source_rd_texture, RID p_dest_texture,
	const Rect2i& p_rect, bool p_flip_y, float p_z_near, float p_z_far)
{
}

void CopyEffects::copy_to_fb_rect(RID p_source_rd_texture, RID p_dest_framebuffer,
	const Rect2i& p_rect, bool p_flip_y, bool p_force_luminance, bool p_alpha_to_zero, bool p_srgb,
	RID p_secondary, bool p_multiview, bool alpha_to_one, bool p_linear, bool p_normal,
	const Rect2& p_src_rect, float p_linear_luminance_multiplier, bool p_bilinear_filtering)
{
}

void CopyEffects::copy_to_atlas_fb(RID p_source_rd_texture, RID p_dest_framebuffer,
	const Rect2& p_uv_rect, RD::DrawListID p_draw_list, bool p_flip_y, bool p_panorama)
{
}

void CopyEffects::copy_to_drawlist(RD::DrawListID p_draw_list, RD::FramebufferFormatID p_fb_format,
	RID p_source_rd_texture, bool p_linear, float p_linear_luminance_multiplier)
{
}

void CopyEffects::copy_raster(RID p_source_texture, RID p_dest_framebuffer) {}

static constexpr int _compute_dispatch_size(bool p_use_array)
{
	constexpr int SIZE = 320;
	constexpr int GROUP = 64;
	constexpr int LEVELS = 6; // One less than Sky::REAL_TIME_ROUGHNESS_LAYERS.
	int size = 0;
	if (p_use_array) {
		size = SIZE * SIZE * LEVELS;
	}
	else {
		int dim = SIZE;
		for (int i = 0; i < LEVELS && dim >= 2; i++) {
			size += dim * dim;
			dim >>= 1;
		}
	}

	return (size + GROUP - 1) / GROUP;
}

void CopyEffects::gaussian_blur(RID p_source_rd_texture, RID p_texture, const Rect2i& p_region,
	const Size2i& p_size, bool p_8bit_dst)
{
}

void CopyEffects::gaussian_blur_raster(
	RID p_source_rd_texture, RID p_dest_texture, const Rect2i& p_region, const Size2i& p_size)
{
}

void CopyEffects::gaussian_glow(RID p_source_rd_texture, RID p_back_texture, const Size2i& p_size,
	float p_strength, bool p_first_pass, float p_luminance_cap, float p_exposure, float p_bloom,
	float p_hdr_bleed_threshold, float p_hdr_bleed_scale, RID p_auto_exposure,
	float p_auto_exposure_scale)
{
}

void CopyEffects::gaussian_glow_downsample_raster(RID p_source_rd_texture, RID p_dest_texture,
	float p_luminance_multiplier, const Size2i& p_size, float p_strength, bool p_first_pass,
	float p_luminance_cap, float p_exposure, float p_bloom, float p_hdr_bleed_threshold,
	float p_hdr_bleed_scale)
{
}

void CopyEffects::gaussian_glow_upsample_raster(RID p_source_rd_texture, RID p_dest_texture,
	RID p_blend_texture, float p_luminance_multiplier, const Size2i& p_source_size,
	const Size2i& p_dest_size, float p_level, float p_base_strength, bool p_use_debanding)
{
}

void CopyEffects::make_mipmap(
	RID p_source_rd_texture, RID p_dest_texture, const Size2i& p_size)
{
}

void CopyEffects::make_mipmap_raster(
	RID p_source_rd_texture, RID p_dest_texture, const Size2i& p_size)
{
}

void CopyEffects::set_color(
	RID p_dest_texture, const Color& p_color, const Rect2i& p_region, bool p_8bit_dst)
{
}

void CopyEffects::set_color_raster(RID p_dest_texture, const Color& p_color, const Rect2i& p_region)
{
}

void CopyEffects::copy_cubemap_to_dp(RID p_source_rd_texture, RID p_dst_framebuffer,
	const Rect2& p_rect, const Vector2& p_dst_size, float p_z_near, float p_z_far, bool p_dp_flip)
{
}

void CopyEffects::copy_cubemap_to_octmap(
	RID p_source_rd_texture, RID p_dst_framebuffer, float p_border_size)
{
}

void CopyEffects::octmap_downsample(
	RID p_source_octmap, RID p_dest_octmap, const Size2i& p_size, float p_border_size)
{
}

void CopyEffects::octmap_downsample_raster(
	RID p_source_octmap, RID p_dest_framebuffer, const Size2i& p_size, float p_border_size)
{
}

void CopyEffects::octmap_filter(
	RID p_source_octmap, const Vector<RID>& p_dest_octmap, bool p_use_array, float p_border_size)
{
}

void CopyEffects::octmap_filter_raster(
	RID p_source_octmap, RID p_dest_framebuffer, uint32_t p_mip_level, float p_border_size)
{
}

void CopyEffects::octmap_roughness(RID p_source_rd_texture, RID p_dest_texture,
	uint32_t p_sample_count, float p_roughness, uint32_t p_source_size, uint32_t p_dest_size,
	float p_border_size)
{
}

void CopyEffects::octmap_roughness_raster(RID p_source_rd_texture, RID p_dest_framebuffer,
	uint32_t p_sample_count, float p_roughness, uint32_t p_source_size, uint32_t p_dest_size,
	float p_border_size)
{
}

void CopyEffects::merge_specular(RID p_dest_framebuffer, RID p_specular, RID p_base,
	RID p_reflection, uint32_t p_view_count)
{
}


