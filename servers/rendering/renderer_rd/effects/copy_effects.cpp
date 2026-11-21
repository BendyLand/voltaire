/**************************************************************************/
/*  copy_effects.cpp                                                      */
/**************************************************************************/
/*                         This file is part of:                          */
/*                            VOLTAIRE ENGINE                             */
/*                https://github.com/BendyLand/Voltaire                   */
/**************************************************************************/
/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
/* Copyright (c) 2014-2026 Juan Linietsky, Ariel Manzur.                  */
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
#include "servers/rendering/renderer_rd/storage_rd/render_scene_buffers_rd.h"
#include "servers/rendering/renderer_rd/rd_caches.h"
#include "servers/rendering/rendering_device.h"

using namespace RendererRD;

/* CopyEffects */

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

void CopyEffects::make_mipmap(RID p_source_rd_texture, RID p_dest_texture, const Size2i& p_size) {}

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

/* Resolve */

Resolve::~Resolve()
{
	if (prefer_raster_effects) {
		resolve_raster.shader.version_free(resolve_raster.shader_version);
	}
	else {
		resolve.shader.version_free(resolve.shader_version);
	}
}

void Resolve::resolve_depth(
	RID p_source_depth, RID p_dest_depth, Vector2i p_screen_size, int p_samples)
{
}

void RendererRD::Resolve::resolve_gi(RID p_source_color, RID p_source_depth, RID p_dest_color,
	RID p_dest_depth, RID p_velocity, RID p_normal, Vector2i p_size, int p_samples)
{
}

Resolve::Resolve(bool p_prefer_raster_effects)
{
	prefer_raster_effects = p_prefer_raster_effects;

	if (prefer_raster_effects) {
		Vector<String> modes;
		modes.push_back("\n");

		resolve_raster.shader.initialize(modes);
		resolve_raster.shader_version = resolve_raster.shader.version_create();
	}
	else {
		Vector<String> modes;
		modes.push_back("\n#define MODE_GI\n");
		modes.push_back("\n#define MODE_GI\n#define MODE_VOXEL_GI\n");
		modes.push_back("\n#define MODE_DEPTH\n");

		resolve.shader.initialize(modes);
		resolve.shader_version = resolve.shader.version_create();
	}
}

/* RoughnessLimiter */

RoughnessLimiter::~RoughnessLimiter() { shader.version_free(shader_version); }

/* SortEffects */

SortEffects::~SortEffects() { shader.version_free(shader_version); }

/* MotionVectorsStore */

MotionVectorsStore::MotionVectorsStore()
{
	Vector<String> modes;
	modes.push_back("");

	motion_shader.initialize(modes);
	shader_version = motion_shader.version_create();
}

MotionVectorsStore::~MotionVectorsStore() { motion_shader.version_free(shader_version); }

// ============================================================================
// Merged from servers/rendering/renderer_rd/effects/post_effects.cpp
// ============================================================================

#include "servers/rendering/renderer_rd/effects/copy_effects.h"
#include "core/config/project_settings.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/renderer_rd/effects/copy_effects.h"
#include "servers/rendering/renderer_rd/effects/smaa_area_tex.gen.h"
#include "servers/rendering/renderer_rd/effects/smaa_search_tex.gen.h"
#include "servers/rendering/renderer_rd/rd_caches.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/light_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/render_scene_buffers_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/texture_storage.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/renderer.h"

#ifndef XR_DISABLED
#include "servers/xr/xr_interface.h"
#include "servers/xr/xr_server.h"
#endif // XR_DISABLED

using namespace RendererRD;

/* BokehDOF */

BokehDOF::~BokehDOF()
{
	for (int i = 0; i < BOKEH_MAX; i++) {
		bokeh.compute_pipelines[i].free();
	}

	if (prefer_raster_effects) {
		bokeh.raster_shader.version_free(bokeh.shader_version);
	}
	else {
		bokeh.compute_shader.version_free(bokeh.shader_version);
	}
}

void RendererRD::BokehDOF::bokeh_dof_compute(const RendererRD::BokehDOF::BokehBuffers& p_buffers,
	RID p_dest_framebuffer, float p_bokeh_size, float p_blur_size, bool p_use_jitter)
{
}

BokehDOF::BokehDOF(bool p_prefer_raster_effects)
{
	prefer_raster_effects = p_prefer_raster_effects;

	Vector<String> modes;
	modes.push_back("\n#define MODE_GEN_BLUR_SIZE\n");
	modes.push_back("\n#define MODE_BOKEH_BOX\n");
	modes.push_back("\n#define MODE_BOKEH_BOX\n#define MODE_NO_WEIGHTS\n");
	modes.push_back("\n#define MODE_BOKEH_HEXAGONAL\n");
	modes.push_back("\n#define MODE_BOKEH_HEXAGONAL\n#define MODE_NO_WEIGHTS\n");
	modes.push_back("\n#define MODE_BOKEH_CIRCULAR\n");
	modes.push_back("\n#define MODE_COMPOSITE\n");

	if (prefer_raster_effects) {
		bokeh.raster_shader.initialize(modes);
		bokeh.shader_version = bokeh.raster_shader.version_create();
	}
	else {
		bokeh.compute_shader.initialize(modes);
		bokeh.shader_version = bokeh.compute_shader.version_create();
	}
}

/* ToneMapper */

ToneMapper::~ToneMapper()
{
	if (using_mobile_version) {
		tonemap_mobile.shader.version_free(tonemap_mobile.shader_version);
	}
	else {
		tonemap.shader.version_free(tonemap.shader_version);
	}
}

void RendererRD::ToneMapper::tonemapper(
	RID p_source_color, RID p_dst_framebuffer, const TonemapSettings& p_settings)
{
}

void RendererRD::ToneMapper::tonemapper_subpass(long p_subpass, RID p_framebuffer,
	long p_render_pass, const RendererRD::ToneMapper::TonemapSettings& p_settings)
{
}

ToneMapper::ToneMapper(bool p_use_mobile_version)
{
	using_mobile_version = p_use_mobile_version;

	if (using_mobile_version) {
		Vector<String> modes;
		modes.push_back("\n");
		modes.push_back("\n#define USE_1D_LUT\n");
		modes.push_back("\n#define SUBPASS\n");
		modes.push_back("\n#define SUBPASS\n#define USE_1D_LUT\n");
		modes.push_back("\n#define USE_MULTIVIEW\n");
		modes.push_back("\n#define USE_1D_LUT\n#define USE_MULTIVIEW\n");
		modes.push_back("\n#define SUBPASS\n#define USE_MULTIVIEW\n");
		modes.push_back("\n#define SUBPASS\n#define USE_1D_LUT\n#define USE_MULTIVIEW\n");

		tonemap_mobile.shader.initialize(modes);
		tonemap_mobile.shader_version = tonemap_mobile.shader.version_create();
	}
	else {
		Vector<String> modes;
		modes.push_back("\n");
		modes.push_back("\n#define GLOW_BICUBIC_FILTER\n");
		modes.push_back("\n#define USE_1D_LUT\n");
		modes.push_back("\n#define GLOW_BICUBIC_FILTER\n#define USE_1D_LUT\n");
		modes.push_back("\n#define USE_MULTIVIEW\n");
		modes.push_back("\n#define GLOW_BICUBIC_FILTER\n#define USE_MULTIVIEW\n");
		modes.push_back("\n#define USE_1D_LUT\n#define USE_MULTIVIEW\n");
		modes.push_back(
			"\n#define GLOW_BICUBIC_FILTER\n#define USE_1D_LUT\n#define USE_MULTIVIEW\n");

		tonemap.shader.initialize(modes);
		tonemap.shader_version = tonemap.shader.version_create();
	}
}

/* Luminance */

Luminance::~Luminance()
{
	if (prefer_raster_effects) {
		luminance_reduce_raster.shader.version_free(luminance_reduce_raster.shader_version);
	}
	else {
		luminance_reduce.shader.version_free(luminance_reduce.shader_version);
	}
}

void Luminance::LuminanceBuffers::set_prefer_raster_effects(bool p_prefer_raster_effects)
{
	prefer_raster_effects = p_prefer_raster_effects;
}

void Luminance::LuminanceBuffers::configure(RenderSceneBuffersRD* p_render_buffers)
{
	Size2i internal_size = p_render_buffers->get_internal_size();
	int w = internal_size.x;
	int h = internal_size.y;

	while (true) {
		w = MAX(w / 8, 1);
		h = MAX(h / 8, 1);

		RDC::TextureFormat tf;
		tf.format = RDC::DATA_FORMAT_R32_SFLOAT;
		tf.width = w;
		tf.height = h;

		bool final = w == 1 && h == 1;

		if (prefer_raster_effects) {
			tf.usage_bits =
				RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT | RDC::TEXTURE_USAGE_SAMPLING_BIT;
		}
		else {
			tf.usage_bits = RDC::TEXTURE_USAGE_STORAGE_BIT;
		}

		if (final) {
			tf.usage_bits |= RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_CAN_COPY_TO_BIT;
		}

		RID texture = RD::texture_create(tf, RDD::TextureView());
		reduce.push_back(texture);

		if (final) {
			current = RD::texture_create(tf, RDD::TextureView());
			RD::texture_clear(current, Color(0.0, 0.0, 0.0), 0u, 1u, 0u, 1u);
			break;
		}
	}
}

void Luminance::LuminanceBuffers::free_data()
{
	for (int i = 0; i < reduce.size(); i++) {
		RD::free_rid(reduce[i]);
	}
	reduce.clear();

	if (current.is_valid()) {
		RD::free_rid(current);
		current = RID();
	}
}

Ref<Luminance::LuminanceBuffers> Luminance::get_luminance_buffers(
	Ref<RenderSceneBuffersRD> p_render_buffers)
{
	if (p_render_buffers->has_custom_data(RB_LUMINANCE_BUFFERS)) {
		return p_render_buffers->get_custom_data(RB_LUMINANCE_BUFFERS);
	}

	Ref<LuminanceBuffers> buffers;
	buffers.instantiate();
	buffers->set_prefer_raster_effects(prefer_raster_effects);
	buffers->configure(p_render_buffers.ptr());

	p_render_buffers->set_custom_data(RB_LUMINANCE_BUFFERS, buffers);

	return buffers;
}

RID Luminance::get_current_luminance_buffer(Ref<RenderSceneBuffersRD> p_render_buffers)
{
	if (p_render_buffers->has_custom_data(RB_LUMINANCE_BUFFERS)) {
		Ref<LuminanceBuffers> buffers = p_render_buffers->get_custom_data(RB_LUMINANCE_BUFFERS);
		return buffers->current;
	}

	return RID();
}

void RendererRD::Luminance::luminance_reduction(RID p_source_texture, Vector2i p_size,
	Ref<RendererRD::Luminance::LuminanceBuffers> p_buffers, float p_min_luminance,
	float p_max_luminance, float p_frame_time, bool p_immediate)
{
}

Luminance::Luminance(bool p_prefer_raster_effects)
{
	prefer_raster_effects = p_prefer_raster_effects;
	if (prefer_raster_effects) {
		Vector<String> modes;
		modes.push_back("\n#define READ_FIRST\n");
		modes.push_back("\n");
		modes.push_back("\n#define WRITE_FINAL\n");

		luminance_reduce_raster.shader.initialize(modes);
		luminance_reduce_raster.shader_version = luminance_reduce_raster.shader.version_create();
	}
	else {
		Vector<String> modes;
		modes.push_back("\n#define READ_FIRST\n");
		modes.push_back("\n");
		modes.push_back("\n#define WRITE_FINAL\n");

		luminance_reduce.shader.initialize(modes);
		luminance_reduce.shader_version = luminance_reduce.shader.version_create();

		for (int i = 0; i < LUMINANCE_REDUCE_MAX; i++) {
			luminance_reduce.pipelines[i] = RD::compute_pipeline_create(
				luminance_reduce.shader.version_get_shader(luminance_reduce.shader_version, i));
		}
	}
}

/* SMAA */

SMAA::~SMAA()
{
	RD::free_rid(smaa.search_tex);
	RD::free_rid(smaa.area_tex);

	smaa.edge_shader.version_free(smaa.edge_shader_version);
	smaa.weight_shader.version_free(smaa.weight_shader_version);
	smaa.blend_shader.version_free(smaa.blend_shader_version);
}

void SMAA::allocate_render_targets(Ref<RenderSceneBuffersRD> p_render_buffers)
{
	RSE::ViewportScaling3DType scaling_type =
		RSE::scaling_3d_mode_type(p_render_buffers->get_scaling_3d_mode());
	bool use_upscaled_texture = p_render_buffers->has_upscaled_texture() &&
								scaling_type == RSE::VIEWPORT_SCALING_3D_TYPE_TEMPORAL;
	Size2i full_size = use_upscaled_texture ? p_render_buffers->get_target_size()
											: p_render_buffers->get_internal_size();

	// As we're not clearing these, and render buffers will return the cached texture if it already
	// exists, we don't first check has_texture here.

	p_render_buffers->create_texture(RB_SCOPE_SMAA, RB_EDGES, RDC::DATA_FORMAT_R8G8_UNORM,
		RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT,
		RDC::TEXTURE_SAMPLES_1, full_size, 1, 1, true, true);
	p_render_buffers->create_texture(RB_SCOPE_SMAA, RB_BLEND, RDC::DATA_FORMAT_R8G8B8A8_UNORM,
		RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT,
		RDC::TEXTURE_SAMPLES_1, full_size, 1, 1, true, true);
	p_render_buffers->create_texture(RB_SCOPE_SMAA, RB_STENCIL, smaa.stencil_format,
		RDC::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, RDC::TEXTURE_SAMPLES_1, full_size, 1, 1,
		true, true);
}

void RendererRD::SMAA::process(Ref<RenderSceneBuffersRD> p_render_buffers, RID p_dest_framebuffer,
	RID p_source_color, bool p_use_srgb)
{
}

SMAA::SMAA()
{
	edge_detection_threshold = 0.1f;

	// 1. Shaders initialization
	Vector<String> edge_modes;
	edge_modes.push_back("\n#define COLOR_EDGE_DETECTION\n");
	edge_modes.push_back("\n#define LUMA_EDGE_DETECTION\n");

	smaa.edge_shader.initialize(edge_modes);
	smaa.edge_shader_version = smaa.edge_shader.version_create();

	Vector<String> empty_modes;
	empty_modes.push_back("\n");

	smaa.weight_shader.initialize(empty_modes);
	smaa.weight_shader_version = smaa.weight_shader.version_create();

	Vector<String> blend_modes;
	blend_modes.push_back("\n");
	blend_modes.push_back("\n#define USE_8_BIT_DEBANDING\n");
	blend_modes.push_back("\n#define USE_10_BIT_DEBANDING\n");

	smaa.blend_shader.initialize(blend_modes);
	smaa.blend_shader_version = smaa.blend_shader.version_create();

	// 2. Load precomputed search texture from embedded PNG
	{
		Vector<uint8_t> png_data;
		png_data.resize(sizeof(search_tex_png));
		memcpy(png_data.ptrw(), search_tex_png, sizeof(search_tex_png));

		Ref<Image> img = memnew(Image);
		img->load_png_from_buffer(png_data);

		RDC::TextureFormat tf;
		tf.format = RDC::DATA_FORMAT_R8_UNORM;
		tf.width = img->get_width();
		tf.height = img->get_height();
		tf.usage_bits = RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_CAN_UPDATE_BIT;

		Vector<Vector<uint8_t>> data_slices;
		data_slices.push_back(img->get_data());

		smaa.search_tex = RD::texture_create(tf, RDD::TextureView(), data_slices);
	}

	// 3. Load precomputed area texture from embedded PNG
	{
		Vector<uint8_t> png_data;
		png_data.resize(sizeof(area_tex_png));
		memcpy(png_data.ptrw(), area_tex_png, sizeof(area_tex_png));

		Ref<Image> img = memnew(Image);
		img->load_png_from_buffer(png_data);

		RDC::TextureFormat tf;
		tf.format = RDC::DATA_FORMAT_R8G8_UNORM;
		tf.width = img->get_width();
		tf.height = img->get_height();
		tf.usage_bits = RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_CAN_UPDATE_BIT;

		Vector<Vector<uint8_t>> data_slices;
		data_slices.push_back(img->get_data());

		smaa.area_tex = RD::texture_create(tf, RDD::TextureView(), data_slices);
	}
}

/* DebugEffects */

void DebugEffects::_create_frustum_arrays()
{
	if (frustum.vertex_buffer.is_null()) {
		// Create vertex buffer, but don't put data in it yet
		frustum.vertex_buffer = RD::vertex_buffer_create(8 * sizeof(float) * 3, Vector<uint8_t>());

		Vector<RDC::VertexAttribute> attributes;
		Vector<RID> buffers;
		RDC::VertexAttribute vd;

		vd.location = 0;
		vd.stride = sizeof(float) * 3;
		vd.format = RDC::DATA_FORMAT_R32G32B32_SFLOAT;

		attributes.push_back(vd);
		buffers.push_back(frustum.vertex_buffer);

		frustum.vertex_format = RD::vertex_format_create(attributes);
		frustum.vertex_array = RD::vertex_array_create(8, frustum.vertex_format, buffers);
	}

	if (frustum.index_buffer.is_null()) {
		uint16_t indices[6 * 2 * 3] = {
			// Far
			0, 1, 2, // FLT, FLB, FRT
			1, 3, 2, // FLB, FRB, FRT
			// Near
			4, 6, 5, // NLT, NRT, NLB
			6, 7, 5, // NRT, NRB, NLB
			// Left
			0, 4, 1, // FLT, NLT, FLB
			4, 5, 1, // NLT, NLB, FLB
			// Right
			6, 2, 7, // NRT, FRT, NRB
			2, 3, 7, // FRT, FRB, NRB
			// Top
			0, 2, 4, // FLT, FRT, NLT
			2, 6, 4, // FRT, NRT, NLT
			// Bottom
			5, 7, 1, // NLB, NRB, FLB,
			7, 3, 1	 // NRB, FRB, FLB
		};

		// Create our index_array
		PackedByteArray data;
		data.resize(6 * 2 * 3 * 2);
		{
			uint8_t* w = data.ptrw();
			uint16_t* p16 = (uint16_t*)w;
			for (int i = 0; i < 6 * 2 * 3; i++) {
				*p16 = indices[i];
				p16++;
			}
		}
		frustum.index_buffer =
			RD::index_buffer_create(6 * 2 * 3, RDC::INDEX_BUFFER_FORMAT_UINT16, data);
		frustum.index_array = RD::index_array_create(frustum.index_buffer, 0, 6 * 2 * 3);
	}

	if (frustum.lines_buffer.is_null()) {
		uint16_t indices[12 * 2] = {
			0, 1, // FLT - FLB
			1, 3, // FLB - FRB
			3, 2, // FRB - FRT
			2, 0, // FRT - FLT

			4, 6, // NLT - NRT
			6, 7, // NRT - NRB
			7, 5, // NRB - NLB
			5, 4, // NLB - NLT

			0, 4, // FLT - NLT
			1, 5, // FLB - NLB
			2, 6, // FRT - NRT
			3, 7  // FRB - NRB
		};

		// Create our lines_array
		PackedByteArray data;
		data.resize(12 * 2 * 2);
		{
			uint8_t* w = data.ptrw();
			uint16_t* p16 = (uint16_t*)w;
			for (int i = 0; i < 12 * 2; i++) {
				*p16 = indices[i];
				p16++;
			}
		}
		frustum.lines_buffer =
			RD::index_buffer_create(12 * 2, RDC::INDEX_BUFFER_FORMAT_UINT16, data);
		frustum.lines_array = RD::index_array_create(frustum.lines_buffer, 0, 12 * 2);
	}
}

DebugEffects::DebugEffects()
{
	// 1. Initialize frustum geometry buffers
	_create_frustum_arrays();

	// 2. Shadow frustum shader & version
	Vector<String> shadow_frustum_modes;
	shadow_frustum_modes.push_back("\n#define MODE_TRANSPARENT\n");
	shadow_frustum_modes.push_back("\n#define MODE_WIREFRAME\n");

	shadow_frustum.shader.initialize(shadow_frustum_modes);
	shadow_frustum.shader_version = shadow_frustum.shader.version_create();

	// 3. Motion vectors shader & version
	Vector<String> motion_vectors_modes;
	motion_vectors_modes.push_back("\n");

	motion_vectors.shader.initialize(motion_vectors_modes);
	motion_vectors.shader_version = motion_vectors.shader.version_create();
}

DebugEffects::~DebugEffects()
{
	shadow_frustum.shader.version_free(shadow_frustum.shader_version);

	// Destroy vertex buffer and array.
	if (frustum.vertex_buffer.is_valid()) {
		RD::free_rid(frustum.vertex_buffer); // Array gets freed as dependency.
	}

	// Destroy index buffer and array,
	if (frustum.index_buffer.is_valid()) {
		RD::free_rid(frustum.index_buffer); // Array gets freed as dependency.
	}

	// Destroy lines buffer and array.
	if (frustum.lines_buffer.is_valid()) {
		RD::free_rid(frustum.lines_buffer); // Array gets freed as dependency.
	}

	motion_vectors.shader.version_free(motion_vectors.shader_version);
}

void RendererRD::DebugEffects::draw_shadow_frustum(RID p_dest_framebuffer,
	const Projection& p_projection, const Transform3D& p_transform, RID p_depth, Rect2 p_rect)
{
}

/* VRS */

VRS::~VRS() { vrs_shader.shader.version_free(vrs_shader.shader_version); }

Size2i VRS::get_vrs_texture_size(const Size2i p_base_size) const
{
	Size2i vrs_texel_size = RD::vrs_get_texel_size();
	return Size2i((p_base_size.x + vrs_texel_size.x - 1) / vrs_texel_size.x,
		(p_base_size.y + vrs_texel_size.y - 1) / vrs_texel_size.y);
}

void VRS::update_vrs_texture(RID p_vrs_fb, RID p_render_target)
{
	RSE::ViewportVRSMode vrs_mode =
		RendererRD::TextureStorage::render_target_get_vrs_mode(p_render_target);
	RSE::ViewportVRSUpdateMode vrs_update_mode =
		RendererRD::TextureStorage::render_target_get_vrs_update_mode(p_render_target);

	if (vrs_mode != RSE::VIEWPORT_VRS_DISABLED &&
		vrs_update_mode != RSE::VIEWPORT_VRS_UPDATE_DISABLED) {
		RD::draw_command_begin_label("VRS Setup");

		if (vrs_mode == RSE::VIEWPORT_VRS_TEXTURE) {
			RID vrs_texture =
				RendererRD::TextureStorage::render_target_get_vrs_texture(p_render_target);
			if (vrs_texture.is_valid()) {
				RID rd_texture = RendererRD::TextureStorage::texture_get_rd_texture(vrs_texture);
				int layers = RendererRD::TextureStorage::texture_get_layers(vrs_texture);
				if (rd_texture.is_valid()) {
					// Copy into our density buffer
					copy_vrs(rd_texture, p_vrs_fb, layers > 1);
				}
			}
#ifndef XR_DISABLED
		}
		else if (vrs_mode == RSE::VIEWPORT_VRS_XR) {
			Ref<XRInterface> interface = XRServer::get_primary_interface();
			if (interface.is_valid() &&
				interface->get_vrs_texture_format() == XRInterface::XR_VRS_TEXTURE_FORMAT_UNIFIED) {
				RID vrs_texture = interface->get_vrs_texture();
				if (vrs_texture.is_valid()) {
					RID rd_texture =
						RendererRD::TextureStorage::texture_get_rd_texture(vrs_texture);
					int layers = RendererRD::TextureStorage::texture_get_layers(vrs_texture);

					if (rd_texture.is_valid()) {
						// Copy into our density buffer
						copy_vrs(rd_texture, p_vrs_fb, layers > 1);
					}
				}
			}
#endif // XR_DISABLED
		}

		if (vrs_update_mode == RSE::VIEWPORT_VRS_UPDATE_ONCE) {
			RendererRD::TextureStorage::render_target_set_vrs_update_mode(
				p_render_target, RSE::VIEWPORT_VRS_UPDATE_DISABLED);
		}

		RD::draw_command_end_label();
	}
}

void RendererRD::VRS::copy_vrs(RID p_source, RID p_dest, bool p_is_mobile) {}

VRS::VRS()
{
	Vector<String> modes;
	modes.push_back("\n");
	vrs_shader.shader.initialize(modes);
	vrs_shader.shader_version = vrs_shader.shader.version_create();
}

// ============================================================================
// Merged from servers/rendering/renderer_rd/effects/ss_effects.cpp
// ============================================================================

#include "core/config/project_settings.h"
#include "servers/rendering/renderer_rd/effects/copy_effects.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/render_scene_buffers_rd.h"
#include "servers/rendering/renderer_rd/rd_caches.h"
#include "servers/rendering/renderer_rd/effects/copy_effects.h"

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



// ============================================================================
// Merged from servers/rendering/renderer_rd/effects/upscale_effects.cpp
// ============================================================================

#include "upscale_effects.h"
#include "servers/rendering/renderer_rd/effects/copy_effects.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/render_scene_buffers_rd.h"
#include "servers/rendering/renderer_rd/rd_caches.h"
#include "servers/rendering/rendering_device.h"

#ifdef METAL_ENABLED
#include "drivers/metal/pixel_formats.h"
#include "drivers/metal/rendering_device_driver_metal3.h"

#include <MetalFX/MetalFX.hpp>
#endif

#ifndef _MSC_VER
#include <cwchar>
#define wcscpy_s wcscpy
#endif

using namespace RendererRD;

/* FSR */

FSR::~FSR()
{
	pipeline.free();
	fsr_shader.version_free(shader_version);
}

void RendererRD::FSR::ensure_context(Ref<RenderSceneBuffersRD> p_render_buffers) {}

void RendererRD::FSR::process(
	Ref<RenderSceneBuffersRD> p_buffers, RID p_color_image, RID p_dst_image)
{
}

FSR::FSR()
{
	Vector<String> modes;
	modes.push_back("\n");
	modes.push_back("\n#define SAMPLE_SLOW_FALLBACK\n");

	fsr_shader.initialize(modes);
	shader_version = fsr_shader.version_create();
}

/* FSR2 */

static RDC::TextureType ffx_resource_type_to_rd_texture_type(FfxResourceType p_type)
{
	switch (p_type) {
	case FFX_RESOURCE_TYPE_TEXTURE1D:
		return RDC::TEXTURE_TYPE_1D;
	case FFX_RESOURCE_TYPE_TEXTURE2D:
		return RDC::TEXTURE_TYPE_2D;
	case FFX_RESOURCE_TYPE_TEXTURE3D:
		return RDC::TEXTURE_TYPE_3D;
	default:
		return RDC::TEXTURE_TYPE_MAX;
	}
}

static FfxResourceType rd_texture_type_to_ffx_resource_type(RDC::TextureType p_type)
{
	switch (p_type) {
	case RDC::TEXTURE_TYPE_1D:
		return FFX_RESOURCE_TYPE_TEXTURE1D;
	case RDC::TEXTURE_TYPE_2D:
		return FFX_RESOURCE_TYPE_TEXTURE2D;
	case RDC::TEXTURE_TYPE_3D:
		return FFX_RESOURCE_TYPE_TEXTURE3D;
	default:
		return FFX_RESOURCE_TYPE_BUFFER;
	}
}

static RDC::DataFormat ffx_surface_format_to_rd_format(FfxSurfaceFormat p_format)
{
	switch (p_format) {
	case FFX_SURFACE_FORMAT_R32G32B32A32_TYPELESS:
		return RDC::DATA_FORMAT_R32G32B32A32_SFLOAT;
	case FFX_SURFACE_FORMAT_R32G32B32A32_FLOAT:
		return RDC::DATA_FORMAT_R32G32B32A32_SFLOAT;
	case FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT:
		return RDC::DATA_FORMAT_R16G16B16A16_SFLOAT;
	case FFX_SURFACE_FORMAT_R16G16B16A16_UNORM:
		return RDC::DATA_FORMAT_R16G16B16A16_UNORM;
	case FFX_SURFACE_FORMAT_R32G32_FLOAT:
		return RDC::DATA_FORMAT_R32G32_SFLOAT;
	case FFX_SURFACE_FORMAT_R32_UINT:
		return RDC::DATA_FORMAT_R32_UINT;
	case FFX_SURFACE_FORMAT_R8G8B8A8_TYPELESS:
		return RDC::DATA_FORMAT_R8G8B8A8_UNORM;
	case FFX_SURFACE_FORMAT_R8G8B8A8_UNORM:
		return RDC::DATA_FORMAT_R8G8B8A8_UNORM;
	case FFX_SURFACE_FORMAT_R11G11B10_FLOAT:
		return RDC::DATA_FORMAT_B10G11R11_UFLOAT_PACK32;
	case FFX_SURFACE_FORMAT_R16G16_FLOAT:
		return RDC::DATA_FORMAT_R16G16_SFLOAT;
	case FFX_SURFACE_FORMAT_R16G16_UINT:
		return RDC::DATA_FORMAT_R16G16_UINT;
	case FFX_SURFACE_FORMAT_R16_FLOAT:
		return RDC::DATA_FORMAT_R16_SFLOAT;
	case FFX_SURFACE_FORMAT_R16_UINT:
		return RDC::DATA_FORMAT_R16_UINT;
	case FFX_SURFACE_FORMAT_R16_UNORM:
		return RDC::DATA_FORMAT_R16_UNORM;
	case FFX_SURFACE_FORMAT_R16_SNORM:
		return RDC::DATA_FORMAT_R16_SNORM;
	case FFX_SURFACE_FORMAT_R8_UNORM:
		return RDC::DATA_FORMAT_R8_UNORM;
	case FFX_SURFACE_FORMAT_R8_UINT:
		return RDC::DATA_FORMAT_R8_UINT;
	case FFX_SURFACE_FORMAT_R8G8_UNORM:
		return RDC::DATA_FORMAT_R8G8_UNORM;
	case FFX_SURFACE_FORMAT_R32_FLOAT:
		return RDC::DATA_FORMAT_R32_SFLOAT;
	default:
		return RDC::DATA_FORMAT_MAX;
	}
}

static FfxSurfaceFormat rd_format_to_ffx_surface_format(RDC::DataFormat p_format)
{
	switch (p_format) {
	case RDC::DATA_FORMAT_R32G32B32A32_SFLOAT:
		return FFX_SURFACE_FORMAT_R32G32B32A32_FLOAT;
	case RDC::DATA_FORMAT_R16G16B16A16_SFLOAT:
		return FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT;
	case RDC::DATA_FORMAT_R16G16B16A16_UNORM:
		return FFX_SURFACE_FORMAT_R16G16B16A16_UNORM;
	case RDC::DATA_FORMAT_R32G32_SFLOAT:
		return FFX_SURFACE_FORMAT_R32G32_FLOAT;
	case RDC::DATA_FORMAT_R32_UINT:
		return FFX_SURFACE_FORMAT_R32_UINT;
	case RDC::DATA_FORMAT_R8G8B8A8_UNORM:
		return FFX_SURFACE_FORMAT_R8G8B8A8_UNORM;
	case RDC::DATA_FORMAT_B10G11R11_UFLOAT_PACK32:
		return FFX_SURFACE_FORMAT_R11G11B10_FLOAT;
	case RDC::DATA_FORMAT_R16G16_SFLOAT:
		return FFX_SURFACE_FORMAT_R16G16_FLOAT;
	case RDC::DATA_FORMAT_R16G16_UINT:
		return FFX_SURFACE_FORMAT_R16G16_UINT;
	case RDC::DATA_FORMAT_R16_SFLOAT:
		return FFX_SURFACE_FORMAT_R16_FLOAT;
	case RDC::DATA_FORMAT_R16_UINT:
		return FFX_SURFACE_FORMAT_R16_UINT;
	case RDC::DATA_FORMAT_R16_UNORM:
		return FFX_SURFACE_FORMAT_R16_UNORM;
	case RDC::DATA_FORMAT_R16_SNORM:
		return FFX_SURFACE_FORMAT_R16_SNORM;
	case RDC::DATA_FORMAT_R8_UNORM:
		return FFX_SURFACE_FORMAT_R8_UNORM;
	case RDC::DATA_FORMAT_R8_UINT:
		return FFX_SURFACE_FORMAT_R8_UINT;
	case RDC::DATA_FORMAT_R8G8_UNORM:
		return FFX_SURFACE_FORMAT_R8G8_UNORM;
	case RDC::DATA_FORMAT_R32_SFLOAT:
		return FFX_SURFACE_FORMAT_R32_FLOAT;
	default:
		return FFX_SURFACE_FORMAT_UNKNOWN;
	}
}

static uint32_t ffx_usage_to_rd_usage_flags(uint32_t p_flags)
{
	uint32_t ret = RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_CAN_UPDATE_BIT;

	if (p_flags & FFX_RESOURCE_USAGE_RENDERTARGET) {
		ret |= RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT;
	}

	if (p_flags & FFX_RESOURCE_USAGE_UAV) {
		ret |= RDC::TEXTURE_USAGE_STORAGE_BIT;
		ret |= RDC::TEXTURE_USAGE_CAN_COPY_FROM_BIT;
		ret |= RDC::TEXTURE_USAGE_CAN_COPY_TO_BIT;
	}

	return ret;
}

static FfxErrorCode get_device_capabilities_rd(FfxFsr2Interface* p_backend_interface,
	FfxDeviceCapabilities* p_out_device_capabilities, FfxDevice p_device)
{
	FSR2Effect::Device& effect_device = *reinterpret_cast<FSR2Effect::Device*>(p_device);

	*p_out_device_capabilities = effect_device.capabilities;

	return FFX_OK;
}

static FfxErrorCode destroy_backend_context_rd(FfxFsr2Interface* p_backend_interface)
{
	FSR2Context::Scratch& scratch =
		*reinterpret_cast<FSR2Context::Scratch*>(p_backend_interface->scratchBuffer);

	for (uint32_t i = 0; i < FSR2_UBO_RING_BUFFER_SIZE; i++) {
		RD::free_rid(scratch.ubo_ring_buffer[i]);
	}

	return FFX_OK;
}

static FfxErrorCode create_resource_rd(FfxFsr2Interface* p_backend_interface,
	const FfxCreateResourceDescription* p_create_resource_description,
	FfxResourceInternal* p_out_resource)
{
	// FSR2's base implementation won't issue a call to create a heap type that isn't just default
	// on its own, so we can safely ignore it as RD does not expose this concept.
	ERR_FAIL_COND_V(p_create_resource_description->heapType != FFX_HEAP_TYPE_DEFAULT,
		FFX_ERROR_INVALID_ARGUMENT);

	FSR2Context::Scratch& scratch =
		*reinterpret_cast<FSR2Context::Scratch*>(p_backend_interface->scratchBuffer);
	FfxResourceDescription res_desc = p_create_resource_description->resourceDescription;

	// FSR2's base implementation never requests buffer creation.
	ERR_FAIL_COND_V(res_desc.type != FFX_RESOURCE_TYPE_TEXTURE1D &&
						res_desc.type != FFX_RESOURCE_TYPE_TEXTURE2D &&
						res_desc.type != FFX_RESOURCE_TYPE_TEXTURE3D,
		FFX_ERROR_INVALID_ARGUMENT);

	if (res_desc.mipCount == 0) {
		// Mipmap count must be derived from the resource's dimensions.
		res_desc.mipCount = uint32_t(
			1 + std::floor(std::log2(MAX(MAX(res_desc.width, res_desc.height), res_desc.depth))));
	}

	Vector<PackedByteArray> initial_data;
	if (p_create_resource_description->initDataSize) {
		PackedByteArray byte_array;
		byte_array.resize(p_create_resource_description->initDataSize);
		memcpy(byte_array.ptrw(), p_create_resource_description->initData,
			p_create_resource_description->initDataSize);
		initial_data.push_back(byte_array);
	}

	RDC::TextureFormat texture_format;
	texture_format.texture_type = ffx_resource_type_to_rd_texture_type(res_desc.type);
	texture_format.format = ffx_surface_format_to_rd_format(res_desc.format);
	texture_format.usage_bits = ffx_usage_to_rd_usage_flags(p_create_resource_description->usage);
	texture_format.width = res_desc.width;
	texture_format.height = res_desc.height;
	texture_format.depth = res_desc.depth;
	texture_format.mipmaps = res_desc.mipCount;
	texture_format.is_discardable = true;

	RID texture = RD::texture_create(texture_format, RDD::TextureView(), initial_data);
	ERR_FAIL_COND_V(texture.is_null(), FFX_ERROR_BACKEND_API_ERROR);

	RD::set_resource_name(texture, String(p_create_resource_description->name));

	// Add the resource to the storage and use the internal index to reference it.
	p_out_resource->internalIndex =
		scratch.resources.add(texture, false, p_create_resource_description->id, res_desc);

	return FFX_OK;
}

static FfxErrorCode register_resource_rd(FfxFsr2Interface* p_backend_interface,
	const FfxResource* p_in_resource, FfxResourceInternal* p_out_resource)
{
	if (p_in_resource->resource == nullptr) {
		// Null resource case.
		p_out_resource->internalIndex = -1;
		return FFX_OK;
	}

	FSR2Context::Scratch& scratch =
		*reinterpret_cast<FSR2Context::Scratch*>(p_backend_interface->scratchBuffer);
	const RID& rid = *reinterpret_cast<const RID*>(p_in_resource->resource);
	ERR_FAIL_COND_V(rid.is_null(), FFX_ERROR_INVALID_ARGUMENT);

	// Add the resource to the storage and use the internal index to reference it.
	p_out_resource->internalIndex = scratch.resources.add(
		rid, true, FSR2Context::RESOURCE_ID_DYNAMIC, p_in_resource->description);

	return FFX_OK;
}

static FfxErrorCode unregister_resources_rd(FfxFsr2Interface* p_backend_interface)
{
	FSR2Context::Scratch& scratch =
		*reinterpret_cast<FSR2Context::Scratch*>(p_backend_interface->scratchBuffer);
	LocalVector<uint32_t> dynamic_list_copy(scratch.resources.dynamic_list);
	for (uint32_t i : dynamic_list_copy) {
		scratch.resources.remove(i);
	}

	return FFX_OK;
}

static FfxResourceDescription get_resource_description_rd(
	FfxFsr2Interface* p_backend_interface, FfxResourceInternal p_resource)
{
	if (p_resource.internalIndex != -1) {
		FSR2Context::Scratch& scratch =
			*reinterpret_cast<FSR2Context::Scratch*>(p_backend_interface->scratchBuffer);
		return scratch.resources.descriptions[p_resource.internalIndex];
	}
	else {
		return {};
	}
}

static FfxErrorCode destroy_resource_rd(
	FfxFsr2Interface* p_backend_interface, FfxResourceInternal p_resource)
{
	if (p_resource.internalIndex != -1) {
		FSR2Context::Scratch& scratch =
			*reinterpret_cast<FSR2Context::Scratch*>(p_backend_interface->scratchBuffer);
		if (scratch.resources.rids[p_resource.internalIndex].is_valid()) {
			RD::free_rid(scratch.resources.rids[p_resource.internalIndex]);
			scratch.resources.remove(p_resource.internalIndex);
		}
	}

	return FFX_OK;
}

static FfxErrorCode destroy_pipeline_rd(
	FfxFsr2Interface* p_backend_interface, FfxPipelineState* p_pipeline)
{
	// We don't want to destroy pipelines when the FSR2 API deems it necessary as it'll do so
	// whenever the context is destroyed.

	return FFX_OK;
}

static FfxErrorCode schedule_gpu_job_rd(
	FfxFsr2Interface* p_backend_interface, const FfxGpuJobDescription* p_job)
{
	ERR_FAIL_NULL_V(p_backend_interface, FFX_ERROR_INVALID_ARGUMENT);
	ERR_FAIL_NULL_V(p_job, FFX_ERROR_INVALID_ARGUMENT);

	FSR2Context::Scratch& scratch =
		*reinterpret_cast<FSR2Context::Scratch*>(p_backend_interface->scratchBuffer);
	scratch.gpu_jobs.push_back(*p_job);

	return FFX_OK;
}

static FfxErrorCode execute_gpu_job_clear_float_rd(
	FSR2Context::Scratch& p_scratch, const FfxClearFloatJobDescription& p_job)
{
	RID resource = p_scratch.resources.rids[p_job.target.internalIndex];
	FfxResourceDescription& desc = p_scratch.resources.descriptions[p_job.target.internalIndex];

	ERR_FAIL_COND_V(desc.type == FFX_RESOURCE_TYPE_BUFFER, FFX_ERROR_INVALID_ARGUMENT);

	Color color(p_job.color[0], p_job.color[1], p_job.color[2], p_job.color[3]);
	RD::texture_clear(resource, color, 0, desc.mipCount, 0, 1);

	return FFX_OK;
}

static FfxErrorCode execute_gpu_job_copy_rd(
	FSR2Context::Scratch& p_scratch, const FfxCopyJobDescription& p_job)
{
	RID src = p_scratch.resources.rids[p_job.src.internalIndex];
	RID dst = p_scratch.resources.rids[p_job.dst.internalIndex];
	FfxResourceDescription& src_desc = p_scratch.resources.descriptions[p_job.src.internalIndex];
	FfxResourceDescription& dst_desc = p_scratch.resources.descriptions[p_job.dst.internalIndex];

	ERR_FAIL_COND_V(src_desc.type == FFX_RESOURCE_TYPE_BUFFER, FFX_ERROR_INVALID_ARGUMENT);
	ERR_FAIL_COND_V(dst_desc.type == FFX_RESOURCE_TYPE_BUFFER, FFX_ERROR_INVALID_ARGUMENT);

	for (uint32_t mip_level = 0; mip_level < src_desc.mipCount; mip_level++) {
		RD::texture_copy(src, dst, Vector3(0, 0, 0), Vector3(0, 0, 0),
			Vector3(src_desc.width, src_desc.height, src_desc.depth), mip_level, mip_level, 0, 0);
	}

	return FFX_OK;
}

static FfxErrorCode execute_gpu_jobs_rd(
	FfxFsr2Interface* p_backend_interface, FfxCommandList p_command_list)
{
	ERR_FAIL_NULL_V(p_backend_interface, FFX_ERROR_INVALID_ARGUMENT);

	FSR2Context::Scratch& scratch =
		*reinterpret_cast<FSR2Context::Scratch*>(p_backend_interface->scratchBuffer);
	FfxErrorCode error_code = FFX_OK;
	for (const FfxGpuJobDescription& job : scratch.gpu_jobs) {
		switch (job.jobType) {
		case FFX_GPU_JOB_CLEAR_FLOAT: {
			error_code = execute_gpu_job_clear_float_rd(scratch, job.clearJobDescriptor);
		} break;
		case FFX_GPU_JOB_COPY: {
			error_code = execute_gpu_job_copy_rd(scratch, job.copyJobDescriptor);
		} break;
		default: {
			error_code = FFX_ERROR_INVALID_ARGUMENT;
		} break;
		}
		if (error_code != FFX_OK) {
			scratch.gpu_jobs.clear();
			return error_code;
		}
	}
	scratch.gpu_jobs.clear();
	return FFX_OK;
}

static FfxResource get_resource_rd(RID* p_rid, const wchar_t* p_name)
{
	FfxResource res = {};
	if (p_rid->is_null()) {
		return res;
	}

	wcscpy_s(res.name, p_name);

	RDC::TextureFormat texture_format = RD::texture_get_format(*p_rid);
	res.description.type = rd_texture_type_to_ffx_resource_type(texture_format.texture_type);
	res.description.format = rd_format_to_ffx_surface_format(texture_format.format);
	res.description.width = texture_format.width;
	res.description.height = texture_format.height;
	res.description.depth = texture_format.depth;
	res.description.mipCount = texture_format.mipmaps;
	res.description.flags = FFX_RESOURCE_FLAGS_NONE;
	res.resource = reinterpret_cast<void*>(p_rid);
	res.isDepth = texture_format.usage_bits & RDC::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	return res;
}

FSR2Context::~FSR2Context() { ffxFsr2ContextDestroy(&fsr_context); }

FSR2Effect::FSR2Effect()
{
	FfxDeviceCapabilities& capabilities = device.capabilities;
	capabilities.minimumSupportedShaderModel = FFX_SHADER_MODEL_5_1;
	capabilities.waveLaneCountMin = 32;
	capabilities.waveLaneCountMax = 32;
	capabilities.fp16Supported = RD::has_feature(RDC::Features::SUPPORTS_HALF_FLOAT);
	capabilities.raytracingSupported = false;

	String general_defines = "\n#define FFX_GPU\n"
							 "\n#define FFX_GLSL 1\n"
							 "\n#define FFX_FSR2_OPTION_LOW_RESOLUTION_MOTION_VECTORS 1\n"
							 "\n#define FFX_FSR2_OPTION_HDR_COLOR_INPUT 1\n"
							 "\n#define FFX_FSR2_OPTION_INVERTED_DEPTH 1\n"
							 "\n#define FFX_FSR2_OPTION_GODOT_REACTIVE_MASK_CLAMP 1\n"
							 "\n#define FFX_FSR2_OPTION_GODOT_DERIVE_INVALID_MOTION_VECTORS 1\n";

	Vector<String> modes_single;
	modes_single.push_back("");

	Vector<String> modes_with_fp16;
	modes_with_fp16.push_back("");
	modes_with_fp16.push_back("\n#define FFX_HALF 1\n");

	{
		Pass& pass = device.passes[FFX_FSR2_PASS_DEPTH_CLIP];
		pass.shader = &shaders.depth_clip;
		pass.shader->initialize(modes_with_fp16, general_defines);
		pass.shader_version = pass.shader->version_create();
		pass.shader_variant = capabilities.fp16Supported ? 1 : 0;

		pass.sampled_bindings = {
			FfxResourceBinding{0, 0, L"r_reconstructed_previous_nearest_depth"},
			FfxResourceBinding{1, 0, L"r_dilated_motion_vectors"},
			FfxResourceBinding{2, 0, L"r_dilatedDepth"},
			FfxResourceBinding{3, 0, L"r_reactive_mask"},
			FfxResourceBinding{4, 0, L"r_transparency_and_composition_mask"},
			FfxResourceBinding{6, 0, L"r_previous_dilated_motion_vectors"},
			FfxResourceBinding{7, 0, L"r_input_motion_vectors"},
			FfxResourceBinding{8, 0, L"r_input_color_jittered"},
			FfxResourceBinding{9, 0, L"r_input_depth"},
			FfxResourceBinding{10, 0, L"r_input_exposure"}};

		pass.storage_bindings = {// FSR2_BIND_UAV_DEPTH_CLIP (11) does not point to anything.
			FfxResourceBinding{12, 0, L"rw_dilated_reactive_masks"},
			FfxResourceBinding{13, 0, L"rw_prepared_input_color"}};

		pass.uniform_bindings = {FfxResourceBinding{14, 0, L"cbFSR2"}};
	}

	{
		Pass& pass = device.passes[FFX_FSR2_PASS_RECONSTRUCT_PREVIOUS_DEPTH];
		pass.shader = &shaders.reconstruct_previous_depth;
		pass.shader->initialize(modes_with_fp16, general_defines);
		pass.shader_version = pass.shader->version_create();
		pass.shader_variant = capabilities.fp16Supported ? 1 : 0;

		pass.sampled_bindings = {FfxResourceBinding{0, 0, L"r_input_motion_vectors"},
			FfxResourceBinding{1, 0, L"r_input_depth"},
			FfxResourceBinding{2, 0, L"r_input_color_jittered"},
			FfxResourceBinding{3, 0, L"r_input_exposure"},
			FfxResourceBinding{4, 0, L"r_luma_history"}};

		pass.storage_bindings = {
			FfxResourceBinding{5, 0, L"rw_reconstructed_previous_nearest_depth"},
			FfxResourceBinding{6, 0, L"rw_dilated_motion_vectors"},
			FfxResourceBinding{7, 0, L"rw_dilatedDepth"},
			FfxResourceBinding{8, 0, L"rw_prepared_input_color"},
			FfxResourceBinding{9, 0, L"rw_luma_history"},
			// FSR2_BIND_UAV_LUMA_INSTABILITY (10) does not point to anything.
			FfxResourceBinding{11, 0, L"rw_lock_input_luma"}};

		pass.uniform_bindings = {FfxResourceBinding{12, 0, L"cbFSR2"}};
	}

	{
		Pass& pass = device.passes[FFX_FSR2_PASS_LOCK];
		pass.shader = &shaders.lock;
		pass.shader->initialize(modes_with_fp16, general_defines);
		pass.shader_version = pass.shader->version_create();
		pass.shader_variant = capabilities.fp16Supported ? 1 : 0;

		pass.sampled_bindings = {FfxResourceBinding{0, 0, L"r_lock_input_luma"}};

		pass.storage_bindings = {FfxResourceBinding{1, 0, L"rw_new_locks"},
			FfxResourceBinding{2, 0, L"rw_reconstructed_previous_nearest_depth"}};

		pass.uniform_bindings = {FfxResourceBinding{3, 0, L"cbFSR2"}};
	}

	{
		Vector<String> accumulate_modes_with_fp16;
		accumulate_modes_with_fp16.push_back("\n");
		accumulate_modes_with_fp16.push_back("\n#define FFX_FSR2_OPTION_APPLY_SHARPENING 1\n");
		accumulate_modes_with_fp16.push_back("\n#define FFX_HALF 1\n");
		accumulate_modes_with_fp16.push_back(
			"\n#define FFX_HALF 1\n#define FFX_FSR2_OPTION_APPLY_SHARPENING 1\n");

		// Workaround: Disable FP16 path for the accumulate pass on NVIDIA due to reduced occupancy
		// and high VRAM throughput.
		const bool fp16_path_supported = RD::get_device_vendor_name() != "NVIDIA";
		Pass& pass = device.passes[FFX_FSR2_PASS_ACCUMULATE];
		pass.shader = &shaders.accumulate;
		pass.shader->initialize(accumulate_modes_with_fp16, general_defines);
		pass.shader_version = pass.shader->version_create();
		pass.shader_variant = capabilities.fp16Supported && fp16_path_supported ? 2 : 0;

		pass.sampled_bindings = {FfxResourceBinding{0, 0, L"r_input_exposure"},
			FfxResourceBinding{1, 0, L"r_dilated_reactive_masks"},
			FfxResourceBinding{2, 0, L"r_input_motion_vectors"},
			FfxResourceBinding{3, 0, L"r_internal_upscaled_color"},
			FfxResourceBinding{4, 0, L"r_lock_status"}, FfxResourceBinding{5, 0, L"r_input_depth"},
			FfxResourceBinding{6, 0, L"r_prepared_input_color"},
			// FSR2_BIND_SRV_LUMA_INSTABILITY(7) does not point to anything.
			FfxResourceBinding{8, 0, L"r_lanczos_lut"},
			FfxResourceBinding{9, 0, L"r_upsample_maximum_bias_lut"},
			FfxResourceBinding{10, 0, L"r_imgMips"}, FfxResourceBinding{11, 0, L"r_auto_exposure"},
			FfxResourceBinding{12, 0, L"r_luma_history"}};

		pass.storage_bindings = {FfxResourceBinding{13, 0, L"rw_internal_upscaled_color"},
			FfxResourceBinding{14, 0, L"rw_lock_status"},
			FfxResourceBinding{15, 0, L"rw_upscaled_output"},
			FfxResourceBinding{16, 0, L"rw_new_locks"},
			FfxResourceBinding{17, 0, L"rw_luma_history"}};

		pass.uniform_bindings = {FfxResourceBinding{18, 0, L"cbFSR2"}};

		// Sharpen pass is a clone of the accumulate pass with the sharpening variant.
		Pass& sharpen_pass = device.passes[FFX_FSR2_PASS_ACCUMULATE_SHARPEN];
		sharpen_pass = pass;
		sharpen_pass.shader_variant = pass.shader_variant + 1;
	}

	{
		Pass& pass = device.passes[FFX_FSR2_PASS_RCAS];
		pass.shader = &shaders.rcas;
		pass.shader->initialize(modes_single, general_defines);
		pass.shader_version = pass.shader->version_create();

		pass.sampled_bindings = {FfxResourceBinding{0, 0, L"r_input_exposure"},
			FfxResourceBinding{1, 0, L"r_rcas_input"}};

		pass.storage_bindings = {FfxResourceBinding{2, 0, L"rw_upscaled_output"}};

		pass.uniform_bindings = {
			FfxResourceBinding{3, 0, L"cbFSR2"}, FfxResourceBinding{4, 0, L"cbRCAS"}};
	}

	{
		Pass& pass = device.passes[FFX_FSR2_PASS_COMPUTE_LUMINANCE_PYRAMID];
		pass.shader = &shaders.compute_luminance_pyramid;
		pass.shader->initialize(modes_single, general_defines);
		pass.shader_version = pass.shader->version_create();

		pass.sampled_bindings = {FfxResourceBinding{0, 0, L"r_input_color_jittered"}};

		pass.storage_bindings = {FfxResourceBinding{1, 0, L"rw_spd_global_atomic"},
			FfxResourceBinding{2, 0, L"rw_img_mip_shading_change"},
			FfxResourceBinding{3, 0, L"rw_img_mip_5"},
			FfxResourceBinding{4, 0, L"rw_auto_exposure"}};

		pass.uniform_bindings = {
			FfxResourceBinding{5, 0, L"cbFSR2"}, FfxResourceBinding{6, 0, L"cbSPD"}};
	}

	{
		Pass& pass = device.passes[FFX_FSR2_PASS_GENERATE_REACTIVE];
		pass.shader = &shaders.autogen_reactive;
		pass.shader->initialize(modes_with_fp16, general_defines);
		pass.shader_version = pass.shader->version_create();
		pass.shader_variant = capabilities.fp16Supported ? 1 : 0;

		pass.sampled_bindings = {FfxResourceBinding{0, 0, L"r_input_opaque_only"},
			FfxResourceBinding{1, 0, L"r_input_color_jittered"}};

		pass.storage_bindings = {FfxResourceBinding{2, 0, L"rw_output_autoreactive"}};

		pass.uniform_bindings = {
			FfxResourceBinding{3, 0, L"cbGenerateReactive"}, FfxResourceBinding{4, 0, L"cbFSR2"}};
	}

	{
		Pass& pass = device.passes[FFX_FSR2_PASS_TCR_AUTOGENERATE];
		pass.shader = &shaders.tcr_autogen;
		pass.shader->initialize(modes_with_fp16, general_defines);
		pass.shader_version = pass.shader->version_create();
		pass.shader_variant = capabilities.fp16Supported ? 1 : 0;

		pass.sampled_bindings = {FfxResourceBinding{0, 0, L"r_input_opaque_only"},
			FfxResourceBinding{1, 0, L"r_input_color_jittered"},
			FfxResourceBinding{2, 0, L"r_input_motion_vectors"},
			FfxResourceBinding{3, 0, L"r_input_prev_color_pre_alpha"},
			FfxResourceBinding{4, 0, L"r_input_prev_color_post_alpha"},
			FfxResourceBinding{5, 0, L"r_reactive_mask"},
			FfxResourceBinding{6, 0, L"r_transparency_and_composition_mask"},
			FfxResourceBinding{13, 0, L"r_input_depth"}};

		pass.storage_bindings = {FfxResourceBinding{7, 0, L"rw_output_autoreactive"},
			FfxResourceBinding{8, 0, L"rw_output_autocomposition"},
			FfxResourceBinding{9, 0, L"rw_output_prev_color_pre_alpha"},
			FfxResourceBinding{10, 0, L"rw_output_prev_color_post_alpha"}};

		pass.uniform_bindings = {
			FfxResourceBinding{11, 0, L"cbFSR2"}, FfxResourceBinding{12, 0, L"cbGenerateReactive"}};
	}

	RDC::SamplerState state;
	state.mag_filter = RDC::SAMPLER_FILTER_NEAREST;
	state.min_filter = RDC::SAMPLER_FILTER_NEAREST;
	state.repeat_u = RDC::SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE;
	state.repeat_v = RDC::SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE;
	state.repeat_w = RDC::SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE;
	state.min_lod = -1000.0f;
	state.max_lod = 1000.0f;
	state.anisotropy_max = 1.0;
	device.point_clamp_sampler = RD::sampler_create(state);
	ERR_FAIL_COND(device.point_clamp_sampler.is_null());

	state.mag_filter = RDC::SAMPLER_FILTER_LINEAR;
	state.min_filter = RDC::SAMPLER_FILTER_LINEAR;
	device.linear_clamp_sampler = RD::sampler_create(state);
	ERR_FAIL_COND(device.linear_clamp_sampler.is_null());
}

FSR2Effect::~FSR2Effect()
{
	RD::free_rid(device.point_clamp_sampler);
	RD::free_rid(device.linear_clamp_sampler);

	for (uint32_t i = 0; i < FFX_FSR2_PASS_COUNT; i++) {
		device.passes[i].shader->version_free(device.passes[i].shader_version);
	}
}

FSR2Context* FSR2Effect::create_context(Size2i p_internal_size, Size2i p_target_size)
{
	FSR2Context* context = memnew(RendererRD::FSR2Context);
	context->fsr_desc.flags = FFX_FSR2_ENABLE_HIGH_DYNAMIC_RANGE | FFX_FSR2_ENABLE_DEPTH_INVERTED;
	context->fsr_desc.maxRenderSize.width = p_internal_size.x;
	context->fsr_desc.maxRenderSize.height = p_internal_size.y;
	context->fsr_desc.displaySize.width = p_target_size.x;
	context->fsr_desc.displaySize.height = p_target_size.y;
	context->fsr_desc.device = &device;

	FfxFsr2Interface& functions = context->fsr_desc.callbacks;
	functions.fpGetDeviceCapabilities = get_device_capabilities_rd;
	functions.fpDestroyBackendContext = destroy_backend_context_rd;
	functions.fpCreateResource = create_resource_rd;
	functions.fpRegisterResource = register_resource_rd;
	functions.fpUnregisterResources = unregister_resources_rd;
	functions.fpGetResourceDescription = get_resource_description_rd;
	functions.fpDestroyResource = destroy_resource_rd;
	functions.fpDestroyPipeline = destroy_pipeline_rd;
	functions.fpScheduleGpuJob = schedule_gpu_job_rd;
	functions.fpExecuteGpuJobs = execute_gpu_jobs_rd;
	functions.scratchBuffer = &context->scratch;
	functions.scratchBufferSize = sizeof(context->scratch);

	FfxErrorCode result = ffxFsr2ContextCreate(&context->fsr_context, &context->fsr_desc);
	if (result == FFX_OK) {
		return context;
	}
	else {
		memdelete(context);
		return nullptr;
	}
}

void FSR2Effect::upscale(const Parameters& p_params)
{
	// TODO: Transparency & Composition mask is not implemented.
	FfxFsr2DispatchDescription dispatch_desc = {};
	RID color = p_params.color;
	RID depth = p_params.depth;
	RID velocity = p_params.velocity;
	RID reactive = p_params.reactive;
	RID exposure = p_params.exposure;
	RID output = p_params.output;
	dispatch_desc.commandList = nullptr;
	dispatch_desc.color = get_resource_rd(&color, L"color");
	dispatch_desc.depth = get_resource_rd(&depth, L"depth");
	dispatch_desc.motionVectors = get_resource_rd(&velocity, L"velocity");
	dispatch_desc.reactive = get_resource_rd(&reactive, L"reactive");
	dispatch_desc.exposure = get_resource_rd(&exposure, L"exposure");
	dispatch_desc.transparencyAndComposition = {};
	dispatch_desc.output = get_resource_rd(&output, L"output");
	dispatch_desc.colorOpaqueOnly = {};
	dispatch_desc.jitterOffset.x = p_params.jitter.x;
	dispatch_desc.jitterOffset.y = p_params.jitter.y;
	dispatch_desc.motionVectorScale.x = float(p_params.internal_size.width);
	dispatch_desc.motionVectorScale.y = float(p_params.internal_size.height);
	dispatch_desc.reset = p_params.reset_accumulation;
	dispatch_desc.renderSize.width = p_params.internal_size.width;
	dispatch_desc.renderSize.height = p_params.internal_size.height;
	dispatch_desc.enableSharpening = (p_params.sharpness > 1e-6f);
	dispatch_desc.sharpness = p_params.sharpness;
	dispatch_desc.frameTimeDelta = p_params.delta_time;
	dispatch_desc.preExposure = 1.0f;
	dispatch_desc.cameraNear = p_params.z_near;
	dispatch_desc.cameraFar = p_params.z_far;
	dispatch_desc.cameraFovAngleVertical = p_params.fovy;
	dispatch_desc.viewSpaceToMetersFactor = 1.0f;
	dispatch_desc.enableAutoReactive = false;
	dispatch_desc.autoTcThreshold = 1.0f;
	dispatch_desc.autoTcScale = 1.0f;
	dispatch_desc.autoReactiveScale = 1.0f;
	dispatch_desc.autoReactiveMax = 1.0f;

	RendererRD::MaterialStorage::store_camera(
		p_params.reprojection, dispatch_desc.reprojectionMatrix);

	FfxErrorCode result = ffxFsr2ContextDispatch(&p_params.context->fsr_context, &dispatch_desc);
	ERR_FAIL_COND(result != FFX_OK);
}

/* TAA */

TAA::~TAA() { taa_shader.version_free(shader_version); }

void TAA::process(Ref<RenderSceneBuffersRD> p_render_buffers, RDC::DataFormat p_format,
	float p_z_near, float p_z_far)
{
	ERR_FAIL_COND(!CopyEffects::is_initialized());

	uint32_t view_count = p_render_buffers->get_view_count();
	Size2i internal_size = p_render_buffers->get_internal_size();
	Size2i target_size = p_render_buffers->get_target_size();

	bool just_allocated = false;
	if (!p_render_buffers->has_texture(SNAME("taa"), SNAME("history"))) {
		uint32_t usage_bits = RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT;

		p_render_buffers->create_texture(SNAME("taa"), SNAME("history"), p_format, usage_bits);
		p_render_buffers->create_texture(SNAME("taa"), SNAME("temp"), p_format, usage_bits);

		p_render_buffers->create_texture(
			SNAME("taa"), SNAME("prev_velocity"), RDC::DATA_FORMAT_R16G16_SFLOAT, usage_bits);

		just_allocated = true;
	}

	RD::draw_command_begin_label("TAA");

	for (uint32_t v = 0; v < view_count; v++) {
		// Get our (cached) slices
		RID internal_texture = p_render_buffers->get_internal_texture(v);
		RID velocity_buffer = p_render_buffers->get_velocity_buffer(false, v);
		RID taa_history = p_render_buffers->get_texture_slice(SNAME("taa"), SNAME("history"), v, 0);
		RID taa_prev_velocity =
			p_render_buffers->get_texture_slice(SNAME("taa"), SNAME("prev_velocity"), v, 0);

		if (!just_allocated) {
			RID depth_texture = p_render_buffers->get_depth_texture(v);
			RID taa_temp = p_render_buffers->get_texture_slice(SNAME("taa"), SNAME("temp"), v, 0);
			resolve(internal_texture, taa_temp, depth_texture, velocity_buffer, taa_prev_velocity,
				taa_history, Size2(internal_size.x, internal_size.y), p_z_near, p_z_far);
			CopyEffects::copy_to_rect(
				taa_temp, internal_texture, Rect2(0, 0, internal_size.x, internal_size.y));
		}

		CopyEffects::copy_to_rect(
			internal_texture, taa_history, Rect2(0, 0, internal_size.x, internal_size.y));
		CopyEffects::copy_to_rect(
			velocity_buffer, taa_prev_velocity, Rect2(0, 0, target_size.x, target_size.y));
	}

	RD::draw_command_end_label();
}

void RendererRD::TAA::resolve(RID p_dest_framebuffer, RID p_current_color, RID p_previous_color,
	RID p_velocity, RID p_depth, RID p_feedback, Vector2 p_jitter, float p_feedback_weight,
	float p_scale)
{
}

#ifdef METAL_ENABLED

/* MetalFX Spatial Scaler */

MFXSpatialContext::~MFXSpatialContext()
{
	if (scaler) {
		scaler->release();
	}
}

MFXSpatialEffect::MFXSpatialEffect() {}

MFXSpatialEffect::~MFXSpatialEffect() {}

void MFXSpatialEffect::callback(
	RDD* p_driver, RDD::CommandBufferID p_command_buffer, CallbackArgs* p_userdata)
{
	MDCommandBufferBase* obj = (MDCommandBufferBase*)(p_command_buffer.id);
	obj->end();

	MTL::Texture* src_texture = reinterpret_cast<MTL::Texture*>(p_userdata->src.id);
	MTL::Texture* dst_texture = reinterpret_cast<MTL::Texture*>(p_userdata->dst.id);

	MTLFX::SpatialScalerBase* scaler = p_userdata->scaler;
	scaler->setColorTexture(src_texture);
	scaler->setOutputTexture(dst_texture);
	MTLFX::SpatialScaler* s = static_cast<MTLFX::SpatialScaler*>(scaler);
	MTL3::MDCommandBuffer* cmd = (MTL3::MDCommandBuffer*)(p_command_buffer.id);
	s->encodeToCommandBuffer(cmd->get_command_buffer());
	obj->retain_resource(scaler);

	CallbackArgs::free(&p_userdata);
}

void MFXSpatialEffect::ensure_context(Ref<RenderSceneBuffersRD> p_render_buffers)
{
	p_render_buffers->ensure_mfx(this);
}

void MFXSpatialEffect::process(Ref<RenderSceneBuffersRD> p_render_buffers, RID p_src, RID p_dst)
{
	MFXSpatialContext* ctx = p_render_buffers->get_mfx_spatial_context();
	DEV_ASSERT(ctx); // this should have been done by the caller via ensure_context

	CallbackArgs* userdata = args_allocator.alloc(this,
		RDD::TextureID(RD::get_driver_resource(RDC::DRIVER_RESOURCE_TEXTURE, p_src)),
		RDD::TextureID(RD::get_driver_resource(RDC::DRIVER_RESOURCE_TEXTURE, p_dst)), *ctx);
	RD::CallbackResource res[2] = {
		{.rid = p_src, .usage = RD::CALLBACK_RESOURCE_USAGE_TEXTURE_SAMPLE},
		{.rid = p_dst, .usage = RD::CALLBACK_RESOURCE_USAGE_STORAGE_IMAGE_READ_WRITE}};
	RD::driver_callback_add((RDD::DriverCallback)MFXSpatialEffect::callback, userdata,
		VectorView<RD::CallbackResource>(res, 2));
}

MFXSpatialContext* MFXSpatialEffect::create_context(CreateParams p_params) const
{
	DEV_ASSERT(RD::has_feature(RD::SUPPORTS_METALFX_SPATIAL));

	RenderingDeviceDriverMetal* rdd = (RenderingDeviceDriverMetal*)RD::get_device_driver();
	PixelFormats& pf = rdd->get_pixel_formats();
	MTL::Device* dev = rdd->get_device();

	NS::SharedPtr<MTLFX::SpatialScalerDescriptor> desc =
		NS::TransferPtr(MTLFX::SpatialScalerDescriptor::alloc()->init());
	desc->setInputWidth((NS::UInteger)p_params.input_size.width);
	desc->setInputHeight((NS::UInteger)p_params.input_size.height);

	desc->setOutputWidth((NS::UInteger)p_params.output_size.width);
	desc->setOutputHeight((NS::UInteger)p_params.output_size.height);

	desc->setColorTextureFormat((MTL::PixelFormat)pf.getMTLPixelFormat(p_params.input_format));
	desc->setOutputTextureFormat((MTL::PixelFormat)pf.getMTLPixelFormat(p_params.output_format));
	desc->setColorProcessingMode(MTLFX::SpatialScalerColorProcessingModeLinear);

	MFXSpatialContext* context = memnew(MFXSpatialContext);
	context->scaler = desc->newSpatialScaler(dev);

	return context;
}

#ifdef METAL_MFXTEMPORAL_ENABLED

/* MetalFX Temporal Scaler */

MFXTemporalContext::~MFXTemporalContext()
{
	if (scaler) {
		scaler->release();
	}
}

MFXTemporalEffect::MFXTemporalEffect() {}
MFXTemporalEffect::~MFXTemporalEffect() {}

MFXTemporalContext* MFXTemporalEffect::create_context(CreateParams p_params) const
{
	DEV_ASSERT(RD::has_feature(RD::SUPPORTS_METALFX_TEMPORAL));

	RenderingDeviceDriverMetal* rdd = (RenderingDeviceDriverMetal*)RD::get_device_driver();
	PixelFormats& pf = rdd->get_pixel_formats();
	MTL::Device* dev = rdd->get_device();

	NS::SharedPtr<MTLFX::TemporalScalerDescriptor> desc =
		NS::TransferPtr(MTLFX::TemporalScalerDescriptor::alloc()->init());
	desc->setInputWidth((NS::UInteger)p_params.input_size.width);
	desc->setInputHeight((NS::UInteger)p_params.input_size.height);

	desc->setOutputWidth((NS::UInteger)p_params.output_size.width);
	desc->setOutputHeight((NS::UInteger)p_params.output_size.height);

	desc->setColorTextureFormat((MTL::PixelFormat)pf.getMTLPixelFormat(p_params.input_format));
	desc->setDepthTextureFormat((MTL::PixelFormat)pf.getMTLPixelFormat(p_params.depth_format));
	desc->setMotionTextureFormat((MTL::PixelFormat)pf.getMTLPixelFormat(p_params.motion_format));
	desc->setAutoExposureEnabled(false);

	desc->setOutputTextureFormat((MTL::PixelFormat)pf.getMTLPixelFormat(p_params.output_format));

	MFXTemporalContext* context = memnew(MFXTemporalContext);
	context->scaler = desc->newTemporalScaler(dev);
	context->scaler->setMotionVectorScaleX(p_params.motion_vector_scale.x);
	context->scaler->setMotionVectorScaleY(p_params.motion_vector_scale.y);
	context->scaler->setDepthReversed(true);

	return context;
}

void MFXTemporalEffect::process(
	RendererRD::MFXTemporalContext* p_ctx, RendererRD::MFXTemporalEffect::Params p_params)
{
	CallbackArgs* userdata = args_allocator.alloc(this,
		RDD::TextureID(RD::get_driver_resource(RDC::DRIVER_RESOURCE_TEXTURE, p_params.src)),
		RDD::TextureID(RD::get_driver_resource(RDC::DRIVER_RESOURCE_TEXTURE, p_params.depth)),
		RDD::TextureID(RD::get_driver_resource(RDC::DRIVER_RESOURCE_TEXTURE, p_params.motion)),
		p_params.exposure.is_valid()
			? RDD::TextureID(
				  RD::get_driver_resource(RDC::DRIVER_RESOURCE_TEXTURE, p_params.exposure))
			: RDD::TextureID(),
		p_params.jitter_offset,
		RDD::TextureID(RD::get_driver_resource(RDC::DRIVER_RESOURCE_TEXTURE, p_params.dst)), *p_ctx,
		p_params.reset);
	RD::CallbackResource res[3] = {
		{.rid = p_params.src, .usage = RD::CALLBACK_RESOURCE_USAGE_TEXTURE_SAMPLE},
		{.rid = p_params.depth, .usage = RD::CALLBACK_RESOURCE_USAGE_TEXTURE_SAMPLE},
		{.rid = p_params.dst, .usage = RD::CALLBACK_RESOURCE_USAGE_STORAGE_IMAGE_READ_WRITE},
	};
	RD::driver_callback_add((RDD::DriverCallback)MFXTemporalEffect::callback, userdata,
		VectorView<RD::CallbackResource>(res, 3));
}

void MFXTemporalEffect::callback(
	RDD* p_driver, RDD::CommandBufferID p_command_buffer, CallbackArgs* p_userdata)
{
	MDCommandBufferBase* obj = (MDCommandBufferBase*)(p_command_buffer.id);
	obj->end();

	MTL::Texture* src_texture = reinterpret_cast<MTL::Texture*>(p_userdata->src.id);
	MTL::Texture* depth = reinterpret_cast<MTL::Texture*>(p_userdata->depth.id);
	MTL::Texture* motion = reinterpret_cast<MTL::Texture*>(p_userdata->motion.id);
	MTL::Texture* exposure = reinterpret_cast<MTL::Texture*>(p_userdata->exposure.id);

	MTL::Texture* dst_texture = reinterpret_cast<MTL::Texture*>(p_userdata->dst.id);

	MTLFX::TemporalScalerBase* scaler = p_userdata->scaler;
	scaler->setReset(p_userdata->reset);
	scaler->setColorTexture(src_texture);
	scaler->setDepthTexture(depth);
	scaler->setMotionTexture(motion);
	scaler->setExposureTexture(exposure);
	scaler->setJitterOffsetX(p_userdata->jitter_offset.x);
	scaler->setJitterOffsetY(p_userdata->jitter_offset.y);
	scaler->setOutputTexture(dst_texture);
	MTLFX::TemporalScaler* s = static_cast<MTLFX::TemporalScaler*>(scaler);
	MTL3::MDCommandBuffer* cmd = (MTL3::MDCommandBuffer*)(p_command_buffer.id);
	s->encodeToCommandBuffer(cmd->get_command_buffer());
	obj->retain_resource(scaler);

	CallbackArgs::free(&p_userdata);
}

#endif // METAL_MFXTEMPORAL_ENABLED

#endif // METAL_ENABLED
