/**************************************************************************/
/*  render_scene_buffers_rd.h                                             */
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

#pragma once

#include "core/templates/hash_map.h"
// --- Inlined/Folded from servers/rendering/renderer_rd/effects/copy_effects.h ---
#include "servers/rendering/renderer_rd/rd_caches.h"
#include "servers/rendering/renderer_rd/shaders/effects/blur_raster.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/copy.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/copy_to_fb.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/cube_to_dp.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/cube_to_octmap.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/motion_vectors_store.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/octmap_downsampler.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/octmap_downsampler_raster.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/octmap_filter.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/octmap_filter_raster.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/octmap_roughness.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/octmap_roughness_raster.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/resolve.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/resolve_raster.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/roughness_limiter.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/sort.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/specular_merge.glsl.gen.h"

class RenderSceneBuffersRD;

namespace RendererRD
{

class CopyEffects final
{
public:
	enum RasterEffects
	{
		RASTER_EFFECT_COPY = 1 << 0,
		RASTER_EFFECT_GAUSSIAN_BLUR = 1 << 1,
		RASTER_EFFECT_OCTMAP = 1 << 2,
	};

	CopyEffects() = delete;
	~CopyEffects() = delete;
	CopyEffects(const CopyEffects&) = delete;
	CopyEffects& operator=(const CopyEffects&) = delete;

private:
	// Blur raster shader

	enum BlurRasterMode
	{
		BLUR_MIPMAP,

		BLUR_MODE_GAUSSIAN_BLUR,
		BLUR_MODE_GAUSSIAN_GLOW_GATHER,
		BLUR_MODE_GAUSSIAN_GLOW_DOWNSAMPLE,
		BLUR_MODE_GAUSSIAN_GLOW_UPSAMPLE,
		BLUR_MODE_COPY,

		BLUR_MODE_SET_COLOR,

		BLUR_MODE_MAX
	};

	enum
	{
		BLUR_FLAG_USE_ORTHOGONAL_PROJECTION = (1 << 1),
	};

	struct BlurRasterPushConstant
	{
		float dest_pixel_size[2];
		float source_pixel_size[2];

		float pad[2];
		uint32_t flags;
		float level;

		// glow
		float glow_strength;
		float glow_bloom;
		float glow_hdr_threshold;
		float glow_hdr_scale;

		float glow_exposure;
		float glow_white;
		float glow_luminance_cap;
		float luminance_multiplier;
	};

	struct BlurRaster
	{
		BlurRasterPushConstant push_constant;
		ShaderRD shader{BlurRasterShaderRD()};
		RID shader_version;
		PipelineCacheRD pipelines[BLUR_MODE_MAX];
		RID glow_sampler;
	};

	// Copy shader

	enum CopyMode
	{
		COPY_MODE_GAUSSIAN_COPY,
		COPY_MODE_GAUSSIAN_COPY_8BIT,
		COPY_MODE_GAUSSIAN_GLOW,
		COPY_MODE_GAUSSIAN_GLOW_AUTO_EXPOSURE,
		COPY_MODE_SIMPLY_COPY,
		COPY_MODE_SIMPLY_COPY_8BIT,
		COPY_MODE_SIMPLY_COPY_DEPTH,
		COPY_MODE_SET_COLOR,
		COPY_MODE_SET_COLOR_8BIT,
		COPY_MODE_MIPMAP,
		COPY_MODE_LINEARIZE_DEPTH,
		COPY_MODE_OCTMAP_TO_PANORAMA,
		COPY_MODE_OCTMAP_ARRAY_TO_PANORAMA,
		COPY_MODE_MAX,

	};

	enum
	{
		COPY_FLAG_HORIZONTAL = (1 << 0),
		COPY_FLAG_USE_COPY_SECTION = (1 << 1),
		COPY_FLAG_USE_ORTHOGONAL_PROJECTION = (1 << 2),
		COPY_FLAG_DOF_NEAR_FIRST_TAP = (1 << 3),
		COPY_FLAG_GLOW_FIRST_PASS = (1 << 4),
		COPY_FLAG_FLIP_Y = (1 << 5),
		COPY_FLAG_FORCE_LUMINANCE = (1 << 6),
		COPY_FLAG_ALL_SOURCE = (1 << 7),
		COPY_FLAG_ALPHA_TO_ONE = (1 << 8),
		COPY_FLAG_SANITIZE_INF_NAN = (1 << 9),
	};

	struct CopyPushConstant
	{
		int32_t section[4];
		int32_t target[2];
		uint32_t flags;
		float luminance_multiplier;
		// Glow.
		float glow_strength;
		float glow_bloom;
		float glow_hdr_threshold;
		float glow_hdr_scale;

		float glow_exposure;
		float glow_white;
		float glow_luminance_cap;
		float glow_auto_exposure_scale;
		// DOF.
		float camera_z_far;
		float camera_z_near;
		// Octmap.
		float octmap_border_size[2];
		// SET color
		float set_color[4];
	};

	struct Copy
	{
		CopyPushConstant push_constant;
		ShaderRD shader{CopyShaderRD()};
		RID shader_version;
		PipelineDeferredRD pipelines[COPY_MODE_MAX];
	};

	// Copy to FB shader

	enum CopyToFBMode
	{
		COPY_TO_FB_COPY,
		COPY_TO_FB_COPY_PANORAMA_TO_DP,
		COPY_TO_FB_COPY2,
		COPY_TO_FB_SET_COLOR,

		// These variants are disabled unless XR shaders are enabled.
		// They should be listed last.
		COPY_TO_FB_MULTIVIEW,
		COPY_TO_FB_MULTIVIEW_WITH_DEPTH,

		COPY_TO_FB_MAX,
	};

	enum CopyToFBFlags
	{
		COPY_TO_FB_FLAG_FLIP_Y = (1 << 0),
		COPY_TO_FB_FLAG_USE_SECTION = (1 << 1),
		COPY_TO_FB_FLAG_FORCE_LUMINANCE = (1 << 2),
		COPY_TO_FB_FLAG_ALPHA_TO_ZERO = (1 << 3),
		COPY_TO_FB_FLAG_SRGB = (1 << 4),
		COPY_TO_FB_FLAG_ALPHA_TO_ONE = (1 << 5),
		COPY_TO_FB_FLAG_LINEAR = (1 << 6),
		COPY_TO_FB_FLAG_NORMAL = (1 << 7),
		COPY_TO_FB_FLAG_USE_SRC_SECTION = (1 << 8),
	};

	struct CopyToFbPushConstant
	{
		float section[4];
		float pixel_size[2];
		float luminance_multiplier;
		uint32_t flags;

		float set_color[4];
	};

	struct CopyToFb
	{
		CopyToFbPushConstant push_constant;
		ShaderRD shader{CopyToFbShaderRD()};
		RID shader_version;
		PipelineCacheRD pipelines[COPY_TO_FB_MAX];
	};

	// Copy to DP

	struct CopyToDPPushConstant
	{
		float z_far;
		float z_near;
		float texel_size[2];
	};

	struct CopyToDP
	{
		ShaderRD shader{CubeToDpShaderRD()};
		RID shader_version;
		PipelineCacheRD pipeline;
	};

	// Copy to Octmap

	struct CopyToOctmapPushConstant
	{
		float border_size;
		uint32_t pad[3];
	};

	struct CopyToOctmap
	{
		CopyToOctmapPushConstant push_constant;
		ShaderRD shader{CubeToOctmapShaderRD()};
		RID shader_version;
		PipelineCacheRD pipeline;
	};

	// Octmap effects

	struct OctmapDownsamplerPushConstant
	{
		float border_size;
		uint32_t size;
		uint32_t pad[2];
	};

	enum OctmapDownsamplerMode
	{
		DOWNSAMPLER_MODE_FLAG_RGB10_A2 = (1 << 0),

		DOWNSAMPLER_MODE_COMPUTE_MAX = (DOWNSAMPLER_MODE_FLAG_RGB10_A2 + 1),
		DOWNSAMPLER_MODE_RASTER_MAX = 1,
	};

	struct OctmapDownsampler
	{
		OctmapDownsamplerPushConstant push_constant;
		ShaderRD compute_shader{OctmapDownsamplerShaderRD()};
		ShaderRD raster_shader{OctmapDownsamplerRasterShaderRD()};
		RID shader_version;
		PipelineDeferredRD compute_pipelines[DOWNSAMPLER_MODE_COMPUTE_MAX];
		PipelineCacheRD raster_pipeline;
	};

	enum OctmapFilterMode
	{
		FILTER_MODE_FLAG_HIGH_QUALITY = (1 << 0),
		FILTER_MODE_FLAG_ARRAY = (1 << 1),
		FILTER_MODE_FLAG_RGB10_A2 = (1 << 2),

		FILTER_MODE_COMPUTE_MAX =
			((FILTER_MODE_FLAG_HIGH_QUALITY | FILTER_MODE_FLAG_ARRAY | FILTER_MODE_FLAG_RGB10_A2) +
				1),
		FILTER_MODE_RASTER_MAX = (FILTER_MODE_FLAG_HIGH_QUALITY + 1),
	};

	struct OctmapFilterPushConstant
	{
		float border_size[2];
		uint32_t pad1;
		uint32_t pad2;
	};

	struct OctmapFilterRasterPushConstant
	{
		float border_size[2];
		uint32_t mip_level;
		uint32_t pad;
	};

	struct OctmapFilter
	{
		ShaderRD compute_shader{OctmapFilterShaderRD()};
		ShaderRD raster_shader{OctmapFilterRasterShaderRD()};
		RID shader_version;
		PipelineDeferredRD compute_pipelines[FILTER_MODE_COMPUTE_MAX];
		PipelineCacheRD raster_pipelines[FILTER_MODE_RASTER_MAX];

		RID uniform_set;
		RID coefficient_buffer;
		bool use_high_quality = false;
	};

	enum OctmapRoughnessMode
	{
		ROUGHNESS_MODE_RGBA16F,
		ROUGHNESS_MODE_RGB10_A2,
		ROUGHNESS_MODE_MAX,
	};

	struct OctmapRoughnessPushConstant
	{
		uint32_t sample_count;
		float roughness;
		uint32_t source_size;
		uint32_t dest_size;

		float border_size[2];
		uint32_t use_direct_write;
		uint32_t pad;
	};

	struct OctmapRoughness
	{
		OctmapRoughnessPushConstant push_constant;
		ShaderRD compute_shader{OctmapRoughnessShaderRD()};
		ShaderRD raster_shader{OctmapRoughnessRasterShaderRD()};
		RID shader_version;
		PipelineDeferredRD compute_pipelines[ROUGHNESS_MODE_MAX];
		PipelineCacheRD raster_pipeline;
	};

	// Merge specular

	enum SpecularMergeMode
	{
		SPECULAR_MERGE_ADD,
		SPECULAR_MERGE_SSR,
		SPECULAR_MERGE_ADDITIVE_ADD,
		SPECULAR_MERGE_ADDITIVE_SSR,

		SPECULAR_MERGE_ADD_MULTIVIEW,
		SPECULAR_MERGE_SSR_MULTIVIEW,
		SPECULAR_MERGE_ADDITIVE_ADD_MULTIVIEW,
		SPECULAR_MERGE_ADDITIVE_SSR_MULTIVIEW,

		SPECULAR_MERGE_MAX
	};

	/* Specular merge must be done using raster, rather than compute
	 * because it must continue the existing color buffer
	 */

	struct SpecularMerge
	{
		ShaderRD shader{SpecularMergeShaderRD()};
		RID shader_version;
		PipelineCacheRD pipelines[SPECULAR_MERGE_MAX];
	};

	struct Data
	{
		uint32_t raster_effects = 0;
		BlurRaster blur_raster;
		Copy copy;
		CopyToFb copy_to_fb;
		CopyToDP cube_to_dp;
		CopyToOctmap cube_to_octmap;
		OctmapDownsampler octmap_downsampler;
		OctmapFilter filter;
		OctmapRoughness roughness;
		SpecularMerge specular_merge;
	};

	static inline Data* data = nullptr;

public:
	static void initialize(uint32_t p_raster_effects = 0);
	static void finalize();
	static bool is_initialized() { return data != nullptr; }

	static uint32_t get_raster_effects() { return data ? data->raster_effects : 0; }

	static void copy_to_rect(RID p_source_rd_texture, RID p_dest_texture, const Rect2i& p_rect,
		bool p_flip_y = false, bool p_force_luminance = false, bool p_all_source = false,
		bool p_8_bit_dst = false, bool p_alpha_to_one = false, bool p_sanitize_inf_nan = false);
	static void copy_octmap_to_panorama(RID p_source_octmap, RID p_dest_panorama,
		const Size2i& p_panorama_size, float p_lod, bool p_is_array,
		const Size2& p_source_octmap_border_size);
	static void copy_depth_to_rect(RID p_source_rd_texture, RID p_dest_framebuffer,
		const Rect2i& p_rect, bool p_flip_y = false);
	static void copy_depth_to_rect_and_linearize(RID p_source_rd_texture, RID p_dest_texture,
		const Rect2i& p_rect, bool p_flip_y, float p_z_near, float p_z_far);
	static void copy_to_fb_rect(RID p_source_rd_texture, RID p_dest_framebuffer,
		const Rect2i& p_rect, bool p_flip_y = false, bool p_force_luminance = false,
		bool p_alpha_to_zero = false, bool p_srgb = false, RID p_secondary = RID(),
		bool p_multiview = false, bool alpha_to_one = false, bool p_linear = false,
		bool p_normal = false, const Rect2& p_src_rect = Rect2(),
		float p_linear_luminance_multiplier = 1.0, bool p_bilinear_filtering = true);
	static void copy_to_atlas_fb(RID p_source_rd_texture, RID p_dest_framebuffer,
		const Rect2& p_uv_rect, RD::DrawListID p_draw_list, bool p_flip_y = false,
		bool p_panorama = false);
	static void copy_to_drawlist(RD::DrawListID p_draw_list, RD::FramebufferFormatID p_fb_format,
		RID p_source_rd_texture, bool p_linear = false, float p_linear_luminance_multiplier = 1.0);
	static void copy_raster(RID p_source_texture, RID p_dest_framebuffer);

	static void gaussian_blur(RID p_source_rd_texture, RID p_texture, const Rect2i& p_region,
		const Size2i& p_size, bool p_8bit_dst = false);
	static void gaussian_blur_raster(
		RID p_source_rd_texture, RID p_dest_texture, const Rect2i& p_region, const Size2i& p_size);
	static void gaussian_glow(RID p_source_rd_texture, RID p_back_texture, const Size2i& p_size,
		float p_strength = 1.0, bool p_first_pass = false, float p_luminance_cap = 16.0,
		float p_exposure = 1.0, float p_bloom = 0.0, float p_hdr_bleed_threshold = 1.0,
		float p_hdr_bleed_scale = 1.0, RID p_auto_exposure = RID(),
		float p_auto_exposure_scale = 1.0);
	static void gaussian_glow_downsample_raster(RID p_source_rd_texture, RID p_dest_texture,
		float p_luminance_multiplier, const Size2i& p_size, float p_strength = 1.0,
		bool p_first_pass = false, float p_luminance_cap = 16.0, float p_exposure = 1.0,
		float p_bloom = 0.0, float p_hdr_bleed_threshold = 1.0, float p_hdr_bleed_scale = 1.0);
	static void gaussian_glow_upsample_raster(RID p_source_rd_texture, RID p_dest_texture,
		RID p_blend_texture, float p_luminance_multiplier, const Size2i& p_source_size,
		const Size2i& p_dest_size, float p_level, float p_base_strength, bool p_use_debanding);

	static void make_mipmap(RID p_source_rd_texture, RID p_dest_texture, const Size2i& p_size);
	static void make_mipmap_raster(
		RID p_source_rd_texture, RID p_dest_texture, const Size2i& p_size);

	static void set_color(
		RID p_dest_texture, const Color& p_color, const Rect2i& p_region, bool p_8bit_dst = false);
	static void set_color_raster(RID p_dest_texture, const Color& p_color, const Rect2i& p_region);

	static void copy_cubemap_to_dp(RID p_source_rd_texture, RID p_dst_framebuffer,
		const Rect2& p_rect, const Vector2& p_dst_size, float p_z_near, float p_z_far,
		bool p_dp_flip);
	static void copy_cubemap_to_octmap(
		RID p_source_rd_texture, RID p_dst_framebuffer, float p_border_size);
	static void octmap_downsample(
		RID p_source_octmap, RID p_dest_octmap, const Size2i& p_size, float p_border_size);
	static void octmap_downsample_raster(
		RID p_source_octmap, RID p_dest_framebuffer, const Size2i& p_size, float p_border_size);
	static void octmap_filter(RID p_source_octmap, const Vector<RID>& p_dest_octmap,
		bool p_use_array, float p_border_size);
	static void octmap_filter_raster(
		RID p_source_octmap, RID p_dest_framebuffer, uint32_t p_mip_level, float p_border_size);
	static void octmap_roughness(RID p_source_rd_texture, RID p_dest_texture,
		uint32_t p_sample_count, float p_roughness, uint32_t p_source_size, uint32_t p_dest_size,
		float p_border_size);
	static void octmap_roughness_raster(RID p_source_rd_texture, RID p_dest_framebuffer,
		uint32_t p_sample_count, float p_roughness, uint32_t p_source_size, uint32_t p_dest_size,
		float p_border_size);

	static void merge_specular(RID p_dest_framebuffer, RID p_specular, RID p_base,
		RID p_reflection, uint32_t p_view_count);
};

class Resolve final
{
private:
	bool prefer_raster_effects;

	struct ResolvePushConstant
	{
		int32_t screen_size[2];
		int32_t samples;
		uint32_t pad;
	};

	enum ResolveMode
	{
		RESOLVE_MODE_GI,
		RESOLVE_MODE_GI_VOXEL_GI,
		RESOLVE_MODE_DEPTH,
		RESOLVE_MODE_MAX
	};

	struct ResolveShader
	{
		ResolvePushConstant push_constant;
		ShaderRD shader{ResolveShaderRD()};
		RID shader_version;
		RID pipelines[RESOLVE_MODE_MAX]; // 3 quality levels
	} resolve;

	struct ResolveRasterShader
	{
		ResolvePushConstant push_constant;
		ShaderRD shader{ResolveRasterShaderRD()};
		RID shader_version;
		PipelineCacheRD pipeline;
	} resolve_raster;

public:
	Resolve(bool p_prefer_raster_effects);
	~Resolve();

	void resolve_gi(RID p_source_depth, RID p_source_normal_roughness, RID p_source_voxel_gi,
		RID p_dest_depth, RID p_dest_normal_roughness, RID p_dest_voxel_gi, Vector2i p_screen_size,
		int p_samples);
	void resolve_depth(RID p_source_depth, RID p_dest_depth, Vector2i p_screen_size, int p_samples);
	void resolve_depth_raster(RID p_source_rd_texture, RID p_dest_framebuffer, int p_samples);
};

// Note, this logic is unused at the time of writing. It should be re-incorporated into the renderer
// at some point.
class RoughnessLimiter final
{
private:
	struct RoughnessLimiterPushConstant
	{
		int32_t screen_size[2];
		float curve;
		uint32_t pad;
	};

	RoughnessLimiterPushConstant push_constant;
	ShaderRD shader{RoughnessLimiterShaderRD()};
	RID shader_version;
	RID pipeline;

public:
	RoughnessLimiter();
	~RoughnessLimiter();

	void roughness_limit(RID p_source_normal, RID p_roughness, const Size2i& p_size, float p_curve);
};

class SortEffects final
{
private:
	enum SortMode
	{
		SORT_MODE_BLOCK,
		SORT_MODE_STEP,
		SORT_MODE_INNER,
		SORT_MODE_MAX
	};

	struct PushConstant
	{
		uint32_t total_elements;
		uint32_t pad[3];
		int32_t job_params[4];
	};

	ShaderRD shader{SortShaderRD()};
	RID shader_version;
	RID pipelines[SORT_MODE_MAX];

public:
	SortEffects();
	~SortEffects();

	void sort_buffer(RID p_uniform_set, int p_size);
};

class MotionVectorsStore final
{
	struct PushConstant
	{
		float reprojection_matrix[16];
		float resolution[2];
		uint32_t pad[2];
	};

	ShaderRD motion_shader{MotionVectorsStoreShaderRD()};
	RID shader_version;
	RID pipeline;

public:
	MotionVectorsStore();
	~MotionVectorsStore();

	void process(Ref<RenderSceneBuffersRD> p_render_buffers, const Projection& p_current_projection,
		const Transform3D& p_current_transform, const Projection& p_previous_projection,
		const Transform3D& p_previous_transform);
};

} // namespace RendererRD

// --- Folded from servers/rendering/renderer_rd/effects/post_effects.h ---
#include "core/types.h"
#include "servers/rendering/renderer_rd/rd_caches.h"
#include "servers/rendering/renderer_rd/shaders/effects/bokeh_dof.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/bokeh_dof_raster.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/luminance_reduce.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/luminance_reduce_raster.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/motion_vectors.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/shadow_frustum.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/smaa_blending.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/smaa_edge_detection.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/smaa_weight_calculation.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/tonemap.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/tonemap_mobile.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/vrs.glsl.gen.h"

#define RB_LUMINANCE_BUFFERS SNAME("luminance_buffers")

#define RB_SCOPE_SMAA SNAME("rb_smaa")

#ifndef RB_EDGES
#define RB_EDGES SNAME("edges")
#endif
#define RB_BLEND SNAME("blend")
#define RB_STENCIL SNAME("stencil")

class RenderSceneBuffersRD;

namespace RendererRD
{

class BokehDOF final
{
private:
	bool prefer_raster_effects;

	struct BokehPushConstant
	{
		uint32_t size[2];
		float z_far;
		float z_near;

		uint32_t orthogonal;
		float blur_size;
		float blur_scale;
		uint32_t steps;

		uint32_t blur_near_active;
		float blur_near_begin;
		float blur_near_end;
		uint32_t blur_far_active;

		float blur_far_begin;
		float blur_far_end;
		uint32_t second_pass;
		uint32_t half_size;

		uint32_t use_jitter;
		float jitter_seed;
		uint32_t use_physical_near;
		uint32_t use_physical_far;

		float blur_size_near;
		float blur_size_far;
		uint32_t pad[2];
	};

	enum BokehMode
	{
		BOKEH_GEN_BLUR_SIZE,
		BOKEH_GEN_BOKEH_BOX,
		BOKEH_GEN_BOKEH_BOX_NOWEIGHT,
		BOKEH_GEN_BOKEH_HEXAGONAL,
		BOKEH_GEN_BOKEH_HEXAGONAL_NOWEIGHT,
		BOKEH_GEN_BOKEH_CIRCULAR,
		BOKEH_COMPOSITE,
		BOKEH_MAX
	};

	struct Bokeh
	{
		BokehPushConstant push_constant;
		ShaderRD compute_shader{BokehDofShaderRD()};
		ShaderRD raster_shader{BokehDofRasterShaderRD()};
		RID shader_version;
		PipelineDeferredRD compute_pipelines[BOKEH_MAX];
		PipelineCacheRD raster_pipelines[BOKEH_MAX];
	} bokeh;

public:
	struct BokehBuffers
	{
		// bokeh buffers

		// textures
		Size2i base_texture_size;
		RID base_texture;
		RID depth_texture;
		RID secondary_texture;
		RID half_texture[2];

		// raster only
		RID base_fb;
		RID secondary_fb; // with weights
		RID half_fb[2];	  // with weights
		RID base_weight_fb;
		RID weight_texture[4];
	};

	BokehDOF(bool p_prefer_raster_effects);
	~BokehDOF();

	void bokeh_dof_compute(const BokehBuffers& p_buffers, RID p_camera_attributes,
		float p_cam_znear, float p_cam_zfar, bool p_cam_orthogonal);
	void bokeh_dof_raster(const BokehBuffers& p_buffers, RID p_camera_attributes, float p_cam_znear,
		float p_cam_zfar, bool p_cam_orthogonal);
};

class ToneMapper final
{
private:
	bool using_mobile_version = false;

	enum TonemapMode
	{
		TONEMAP_MODE_NORMAL,
		TONEMAP_MODE_BICUBIC_GLOW_FILTER,
		TONEMAP_MODE_1D_LUT,
		TONEMAP_MODE_BICUBIC_GLOW_FILTER_1D_LUT,

		TONEMAP_MODE_NORMAL_MULTIVIEW,
		TONEMAP_MODE_BICUBIC_GLOW_FILTER_MULTIVIEW,
		TONEMAP_MODE_1D_LUT_MULTIVIEW,
		TONEMAP_MODE_BICUBIC_GLOW_FILTER_1D_LUT_MULTIVIEW,

		TONEMAP_MODE_MAX
	};

	enum TonemapModeMobile
	{
		TONEMAP_MOBILE_MODE_NORMAL,
		TONEMAP_MOBILE_MODE_1D_LUT,
		TONEMAP_MOBILE_MODE_SUBPASS,
		TONEMAP_MOBILE_MODE_SUBPASS_1D_LUT,

		TONEMAP_MOBILE_MODE_NORMAL_MULTIVIEW,
		TONEMAP_MOBILE_MODE_1D_LUT_MULTIVIEW,
		TONEMAP_MOBILE_MODE_SUBPASS_MULTIVIEW,
		TONEMAP_MOBILE_MODE_SUBPASS_1D_LUT_MULTIVIEW,

		TONEMAP_MOBILE_MODE_MAX
	};

	enum Flags
	{
		TONEMAP_FLAG_USE_BCS = (1 << 0),
		TONEMAP_FLAG_USE_GLOW = (1 << 1),
		TONEMAP_FLAG_USE_AUTO_EXPOSURE = (1 << 2),
		TONEMAP_FLAG_USE_COLOR_CORRECTION = (1 << 3),
		TONEMAP_FLAG_USE_FXAA = (1 << 4),
		TONEMAP_FLAG_USE_8_BIT_DEBANDING = (1 << 5),
		TONEMAP_FLAG_CONVERT_TO_SRGB = (1 << 6),
	};

	enum FlagsMobile
	{
		TONEMAP_MOBILE_FLAG_USE_BCS = (1 << 0),
		TONEMAP_MOBILE_FLAG_USE_GLOW = (1 << 1),
		TONEMAP_MOBILE_FLAG_USE_GLOW_MAP = (1 << 2),
		TONEMAP_MOBILE_FLAG_USE_COLOR_CORRECTION = (1 << 3),
		TONEMAP_MOBILE_FLAG_USE_FXAA = (1 << 4),
		TONEMAP_MOBILE_FLAG_USE_8_BIT_DEBANDING = (1 << 5),
		TONEMAP_MOBILE_FLAG_USE_10_BIT_DEBANDING = (1 << 6),
		TONEMAP_MOBILE_FLAG_CONVERT_TO_SRGB = (1 << 7),

		TONEMAP_MOBILE_FLAG_TONEMAPPER_LINEAR = (1 << 8),
		TONEMAP_MOBILE_FLAG_TONEMAPPER_REINHARD = (1 << 9),
		TONEMAP_MOBILE_FLAG_TONEMAPPER_FILMIC = (1 << 10),
		TONEMAP_MOBILE_FLAG_TONEMAPPER_ACES = (1 << 11),
		TONEMAP_MOBILE_FLAG_TONEMAPPER_AGX = (1 << 12),

		TONEMAP_MOBILE_FLAG_GLOW_MODE_ADD = (1 << 13),
		TONEMAP_MOBILE_FLAG_GLOW_MODE_SCREEN = (1 << 14),
		TONEMAP_MOBILE_FLAG_GLOW_MODE_SOFTLIGHT = (1 << 15),
		TONEMAP_MOBILE_FLAG_GLOW_MODE_REPLACE = (1 << 16),
		TONEMAP_MOBILE_FLAG_GLOW_MODE_MIX = (1 << 17),
		TONEMAP_MOBILE_ADRENO_BUG = (1 << 18), // Needs to be last so we force the pipeline cache to
											   // specify specializations for all variants.
	};

	struct TonemapPushConstant
	{
		float bcs[3];	// 12 - 12
		uint32_t flags; //  4 - 16

		float pixel_size[2];	//  8 - 24
		uint32_t tonemapper;	//  4 - 28
		float output_max_value; //  4 - 32

		uint32_t glow_texture_size[2]; //  8 - 40
		float glow_intensity;		   //  4 - 44
		float glow_map_strength;	   //  4 - 48

		uint32_t glow_mode;	  //  4 - 52
		float glow_levels[7]; // 28 - 80

		float exposure;				//  4 - 84
		float white;				//  4 - 88
		float auto_exposure_scale;	//  4 - 92
		float luminance_multiplier; //  4 - 96

		float tonemapper_params[4]; //  16 - 112
	};

	struct TonemapPushConstantMobile
	{
		float bcs[3];				// 12 - 12
		float luminance_multiplier; //  4 - 16

		float src_pixel_size[2];  //  8 - 24
		float dest_pixel_size[2]; //  8 - 32

		float glow_intensity;	 //  4 - 36
		float glow_map_strength; //  4 - 40
		float exposure;			 //  4 - 44
		float white;			 //  4 - 48

		float tonemapper_params[4]; //  16 - 64
		float output_max_value;		//  4 - 68
		float pad[3];				//  12 - 80
	};

	/* tonemap actually writes to a framebuffer, which is
	 * better to do using the raster pipeline rather than
	 * compute, as that framebuffer might be in different formats
	 */
	struct Tonemap
	{
		TonemapPushConstant push_constant;
		ShaderRD shader{TonemapShaderRD()};
		RID shader_version;
		PipelineCacheRD pipelines[TONEMAP_MODE_MAX];
	} tonemap;

	struct TonemapMobile
	{
		TonemapPushConstantMobile push_constant;
		ShaderRD shader{TonemapMobileShaderRD()};
		RID shader_version;
		PipelineCacheRD pipelines[TONEMAP_MOBILE_MODE_MAX];
	} tonemap_mobile;

public:
	ToneMapper(bool p_use_mobile_version);
	~ToneMapper();

	struct TonemapSettings
	{
		bool use_glow = false;
		RSE::EnvironmentGlowBlendMode glow_mode = RSE::ENV_GLOW_BLEND_MODE_SCREEN;
		float glow_intensity = 0.3;
		float glow_map_strength = 0.0f;
		float glow_levels[7] = {1.0, 0.8, 0.4, 0.1, 0.0, 0.0, 0.0};
		Vector2i glow_texture_size;
		bool glow_use_bicubic_upscale = false;
		RID glow_texture;
		RID glow_map;

		RSE::EnvironmentToneMapper tonemap_mode = RSE::ENV_TONE_MAPPER_LINEAR;
		float tonemapper_params[4] = {0.0, 0.0, 0.0, 0.0};
		float exposure = 1.0;
		float white = 1.0;
		float max_value = 1.0;

		bool use_auto_exposure = false;
		float auto_exposure_scale = 0.5;
		RID exposure_texture;
		float luminance_multiplier = 1.0;

		bool use_bcs = false;
		float brightness = 1.0;
		float contrast = 1.0;
		float saturation = 1.0;

		bool use_color_correction = false;
		bool use_1d_color_correction = false;
		RID color_correction_texture;

		bool use_fxaa = false;

		enum DebandingMode
		{
			DEBANDING_MODE_DISABLED,
			DEBANDING_MODE_8_BIT,
			DEBANDING_MODE_10_BIT,
		};

		DebandingMode debanding_mode = DEBANDING_MODE_DISABLED;
		Vector2i texture_size;
		Vector2i dest_texture_size;
		uint32_t view_count = 1;
		bool bilinear_filtering = true;

		bool convert_to_srgb = false;
	};

	void tonemapper(RID p_source_color, RID p_dst_framebuffer, const TonemapSettings& p_settings);
	void tonemapper_mobile(
		RID p_source_color, RID p_dst_framebuffer, const TonemapSettings& p_settings);
	void tonemapper_subpass(RD::DrawListID p_subpass_draw_list, RID p_source_color,
		RD::FramebufferFormatID p_dst_format_id, const TonemapSettings& p_settings);
};

class Luminance final
{
private:
	bool prefer_raster_effects;

	enum LuminanceReduceMode
	{
		LUMINANCE_REDUCE_READ,
		LUMINANCE_REDUCE,
		LUMINANCE_REDUCE_WRITE,
		LUMINANCE_REDUCE_MAX
	};

	struct LuminanceReducePushConstant
	{
		int32_t source_size[2];
		float max_luminance;
		float min_luminance;
		float exposure_adjust;
		float pad[3];
	};

	struct LuminanceReduce
	{
		ShaderRD shader{LuminanceReduceShaderRD()};
		RID shader_version;
		RID pipelines[LUMINANCE_REDUCE_MAX];
	} luminance_reduce;

	enum LuminanceReduceRasterMode
	{
		LUMINANCE_REDUCE_FRAGMENT_FIRST,
		LUMINANCE_REDUCE_FRAGMENT,
		LUMINANCE_REDUCE_FRAGMENT_FINAL,
		LUMINANCE_REDUCE_FRAGMENT_MAX
	};

	struct LuminanceReduceRasterPushConstant
	{
		int32_t source_size[2];
		int32_t dest_size[2];
		float exposure_adjust;
		float min_luminance;
		float max_luminance;
		uint32_t pad1;
	};

	struct LuminanceReduceFragment
	{
		ShaderRD shader{LuminanceReduceRasterShaderRD()};
		RID shader_version;
		PipelineCacheRD pipelines[LUMINANCE_REDUCE_FRAGMENT_MAX];
	} luminance_reduce_raster;

public:
	class LuminanceBuffers final : public RefCounted
	{
	private:
		bool prefer_raster_effects;

	public:
		Vector<RID> reduce;
		RID current;

		void configure(RenderSceneBuffersRD* p_render_buffers);
		void free_data();

		void set_prefer_raster_effects(bool p_prefer_raster_effects);
	};

	Ref<LuminanceBuffers> get_luminance_buffers(Ref<RenderSceneBuffersRD> p_render_buffers);
	RID get_current_luminance_buffer(Ref<RenderSceneBuffersRD> p_render_buffers);
	void luminance_reduction(RID p_source_texture, const Size2i p_source_size,
		Ref<LuminanceBuffers> p_luminance_buffers, float p_min_luminance, float p_max_luminance,
		float p_adjust, bool p_set = false);

	Luminance(bool p_prefer_raster_effects);
	~Luminance();
};

class SMAA final
{
private:
	enum SMAAMode
	{
		SMAA_EDGE_DETECTION_COLOR,
		SMAA_WEIGHT_FULL,
		SMAA_BLENDING,
		SMAA_MAX,
	};

	struct SMAAEdgePushConstant
	{
		float inv_size[2];
		float threshold;
		float pad;
	};

	struct SMAAWeightPushConstant
	{
		float inv_size[2];
		uint32_t size[2];

		float subsample_indices[4];
	};

	struct SMAABlendPushConstant
	{
		float inv_size[2];
		uint32_t use_debanding;
		float pad;
	};

	enum SMAABlendFlags
	{
		SMAA_BLEND_FLAG_USE_8_BIT_DEBANDING = (1 << 0),
		SMAA_BLEND_FLAG_USE_10_BIT_DEBANDING = (1 << 1),
	};

	struct SMAAEffect
	{
		SMAAEdgePushConstant edge_push_constant;
		ShaderRD edge_shader{SmaaEdgeDetectionShaderRD()};
		RID edge_shader_version;

		SMAAWeightPushConstant weight_push_constant;
		ShaderRD weight_shader{SmaaWeightCalculationShaderRD()};
		RID weight_shader_version;

		SMAABlendPushConstant blend_push_constant;
		ShaderRD blend_shader{SmaaBlendingShaderRD()};
		RID blend_shader_version;

		RID search_tex;
		RID area_tex;

		RDC::DataFormat stencil_format;

		PipelineCacheRD pipelines[SMAA_MAX];
	} smaa;

	float edge_detection_threshold;

public:
	SMAA();
	~SMAA();

	void allocate_render_targets(Ref<RenderSceneBuffersRD> p_render_buffers);
	void process(Ref<RenderSceneBuffersRD> p_render_buffers, RID p_source_color,
		RID p_dst_framebuffer, bool p_use_debanding);
};

class DebugEffects final
{
private:
	struct
	{
		RD::VertexFormatID vertex_format;
		RID vertex_buffer;
		RID vertex_array;

		RID index_buffer;
		RID index_array;

		RID lines_buffer;
		RID lines_array;
	} frustum;

	struct ShadowFrustumPushConstant
	{
		float mvp[16];
		float color[4];
	};

	enum ShadowFrustumPipelines
	{
		SFP_TRANSPARENT,
		SFP_WIREFRAME,
		SFP_MAX
	};

	struct
	{
		ShaderRD shader{ShadowFrustumShaderRD()};
		RID shader_version;
		PipelineCacheRD pipelines[SFP_MAX];
	} shadow_frustum;

	struct MotionVectorsPushConstant
	{
		float reprojection_matrix[16];
		float resolution[2];
		uint32_t force_derive_from_depth;
		uint32_t pad;
	};

	struct
	{
		ShaderRD shader{MotionVectorsShaderRD()};
		RID shader_version;
		PipelineCacheRD pipeline;
		MotionVectorsPushConstant push_constant;
	} motion_vectors;

	void _create_frustum_arrays();

public:
	DebugEffects();
	~DebugEffects();

	void draw_shadow_frustum(RID p_light, const Projection& p_cam_projection,
		const Transform3D& p_cam_transform, RID p_dest_fb, const Rect2 p_rect);
	void draw_motion_vectors(RID p_velocity, RID p_depth, RID p_dest_fb,
		const Projection& p_current_projection, const Transform3D& p_current_transform,
		const Projection& p_previous_projection, const Transform3D& p_previous_transform,
		Size2i p_resolution);
};

class VRS final
{
private:
	enum VRSMode
	{
		VRS_DEFAULT,
		VRS_MULTIVIEW,
		VRS_RG,
		VRS_RG_MULTIVIEW,
		VRS_MAX,
	};

	struct VRSPushConstant
	{
		float max_texel_factor; // 4x8, 8x4 and 8x8 are only available on some GPUs.
		float res1;
		float res2;
		float res3;
	};

	struct VRSShader
	{
		// VRSPushConstant push_constant;
		ShaderRD shader{VrsShaderRD()};
		RID shader_version;
		PipelineCacheRD pipelines[VRS_MAX];
	} vrs_shader;

public:
	VRS();
	~VRS();

	void copy_vrs(RID p_source_rd_texture, RID p_dest_framebuffer, bool p_multiview = false);

	Size2i get_vrs_texture_size(const Size2i p_base_size) const;
	void update_vrs_texture(RID p_vrs_fb, RID p_render_target);
};

} // namespace RendererRD

// --- End servers/rendering/renderer_rd/effects/post_effects.h ---

// --- Folded from servers/rendering/renderer_rd/effects/ss_effects.h ---
#include "servers/rendering/renderer_rd/rd_caches.h"
#include "servers/rendering/renderer_rd/shaders/effects/screen_space_reflection.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/screen_space_reflection_downsample.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/screen_space_reflection_filter.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/screen_space_reflection_hiz.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/screen_space_reflection_resolve.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/ss_effects_downsample.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/ssao.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/ssao_blur.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/ssao_importance_map.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/ssao_interleave.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/ssil.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/ssil_blur.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/ssil_importance_map.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/ssil_interleave.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/subsurface_scattering.glsl.gen.h"

#define RB_SCOPE_SSLF SNAME("rb_sslf")
#define RB_SCOPE_SSDS SNAME("rb_ssds")
#define RB_SCOPE_SSIL SNAME("rb_ssil")
#define RB_SCOPE_SSAO SNAME("rb_ssao")
#define RB_SCOPE_SSR SNAME("rb_ssr")

#define RB_LINEAR_DEPTH SNAME("linear_depth")
#define RB_FINAL SNAME("final")
#define RB_LAST_FRAME SNAME("last_frame")
#define RB_DEINTERLEAVED SNAME("deinterleaved")
#define RB_DEINTERLEAVED_PONG SNAME("deinterleaved_pong")
#ifndef RB_EDGES
#define RB_EDGES SNAME("edges")
#endif
#define RB_IMPORTANCE_MAP SNAME("importance_map")
#define RB_IMPORTANCE_PONG SNAME("importance_pong")

#define RB_NORMAL_ROUGHNESS SNAME("normal_roughness")
#define RB_HIZ SNAME("hiz")
#define RB_SSR SNAME("ssr")
#define RB_MIP_LEVEL SNAME("mip_level")

class RenderSceneBuffersRD;

namespace RendererRD
{

class SSEffects final
{
public:
	SSEffects() = delete;
	~SSEffects() = delete;
	SSEffects(const SSEffects&) = delete;
	SSEffects& operator=(const SSEffects&) = delete;

	static void initialize();
	static void finalize();
	static bool is_initialized() { return data != nullptr; }

	/* Last Frame */

	static void allocate_last_frame_buffer(
		Ref<RenderSceneBuffersRD> p_render_buffers, bool p_use_ssil, bool p_use_ssr);
	static void copy_internal_texture_to_last_frame(Ref<RenderSceneBuffersRD> p_render_buffers);

	/* SS Downsampler */

	static void downsample_depth(
		Ref<RenderSceneBuffersRD> p_render_buffers, uint32_t p_view, const Projection& p_projection);

	/* SSIL */
	static void ssil_set_quality(RSE::EnvironmentSSILQuality p_quality, bool p_half_size,
		float p_adaptive_target, int p_blur_passes, float p_fadeout_from, float p_fadeout_to);

	struct SSILRenderBuffers
	{
		bool half_size = false;
		int buffer_width;
		int buffer_height;
		int half_buffer_width;
		int half_buffer_height;
	};

	struct SSILSettings
	{
		float radius = 1.0;
		float intensity = 2.0;
		float sharpness = 0.98;
		float normal_rejection = 1.0;

		Size2i full_screen_size;
	};

	static void ssil_allocate_buffers(Ref<RenderSceneBuffersRD> p_render_buffers,
		SSILRenderBuffers& p_ssil_buffers, const SSILSettings& p_settings);
	static void screen_space_indirect_lighting(Ref<RenderSceneBuffersRD> p_render_buffers,
		SSILRenderBuffers& p_ssil_buffers, uint32_t p_view, RID p_normal_buffer,
		const Projection& p_projection, const Projection& p_last_projection,
		const SSILSettings& p_settings);

	/* SSAO */
	static void ssao_set_quality(RSE::EnvironmentSSAOQuality p_quality, bool p_half_size,
		float p_adaptive_target, int p_blur_passes, float p_fadeout_from, float p_fadeout_to);

	struct SSAORenderBuffers
	{
		bool half_size = false;
		int buffer_width;
		int buffer_height;
		int half_buffer_width;
		int half_buffer_height;
	};

	struct SSAOSettings
	{
		float radius = 1.0;
		float intensity = 2.0;
		float power = 1.5;
		float detail = 0.5;
		float horizon = 0.06;
		float sharpness = 0.98;

		Size2i full_screen_size;
	};

	static void ssao_allocate_buffers(Ref<RenderSceneBuffersRD> p_render_buffers,
		SSAORenderBuffers& p_ssao_buffers, const SSAOSettings& p_settings);
	static void generate_ssao(Ref<RenderSceneBuffersRD> p_render_buffers,
		SSAORenderBuffers& p_ssao_buffers, uint32_t p_view, RID p_normal_buffer,
		const Projection& p_projection, const SSAOSettings& p_settings);

	/* Screen Space Reflection */
	static void ssr_set_half_size(bool p_half_size);

	struct SSRRenderBuffers
	{
		Size2i size;
		uint32_t mipmaps = 1;
		bool half_size = false;
	};

	static void ssr_allocate_buffers(Ref<RenderSceneBuffersRD> p_render_buffers,
		SSRRenderBuffers& p_ssr_buffers, const RDC::DataFormat p_color_format);
	static void screen_space_reflection(Ref<RenderSceneBuffersRD> p_render_buffers,
		SSRRenderBuffers& p_ssr_buffers, const RID* p_normal_roughness_slices, int p_max_steps,
		float p_fade_in, float p_fade_out, float p_tolerance, const Projection* p_projections,
		const Projection* p_reprojections, const Vector3* p_eye_offsets);

	/* subsurface scattering */
	static void sss_set_quality(RSE::SubSurfaceScatteringQuality p_quality);
	static RSE::SubSurfaceScatteringQuality sss_get_quality();
	static void sss_set_scale(float p_scale, float p_depth_scale);

	static void sub_surface_scattering(Ref<RenderSceneBuffersRD> p_render_buffers, RID p_diffuse,
		RID p_depth, const Projection& p_camera, const Size2i& p_screen_size);

private:
	/* SS Downsampler */

	struct SSEffectsDownsamplePushConstant
	{
		float pixel_size[2];
		float z_far;
		float z_near;
		uint32_t orthogonal;
		float radius_sq;
		uint32_t pad[2];
	};

	enum SSEffectsMode
	{
		SS_EFFECTS_DOWNSAMPLE,
		SS_EFFECTS_DOWNSAMPLE_HALF_RES,
		SS_EFFECTS_DOWNSAMPLE_MIPMAP,
		SS_EFFECTS_DOWNSAMPLE_MIPMAP_HALF_RES,
		SS_EFFECTS_DOWNSAMPLE_HALF,
		SS_EFFECTS_DOWNSAMPLE_HALF_RES_HALF,
		SS_EFFECTS_DOWNSAMPLE_FULL_MIPS,
		SS_EFFECTS_MAX
	};

	struct SSEffectsGatherConstants
	{
		float rotation_matrices[80]; // 5 vec4s * 4
	};

	struct SSEffectsShader
	{
		SSEffectsDownsamplePushConstant downsample_push_constant;
		ShaderRD downsample_shader{SsEffectsDownsampleShaderRD()};
		RID downsample_shader_version;
		bool used_half_size_last_frame = false;
		bool used_mips_last_frame = false;
		bool used_full_mips_last_frame = false;

		RID gather_constants_buffer;

		RID mirror_sampler;

		PipelineDeferredRD pipelines[SS_EFFECTS_MAX];
	};

	/* SSIL */

	enum SSILMode
	{
		SSIL_GATHER,
		SSIL_GATHER_BASE,
		SSIL_GATHER_ADAPTIVE,
		SSIL_GENERATE_IMPORTANCE_MAP,
		SSIL_PROCESS_IMPORTANCE_MAPA,
		SSIL_PROCESS_IMPORTANCE_MAPB,
		SSIL_BLUR_PASS,
		SSIL_BLUR_PASS_SMART,
		SSIL_BLUR_PASS_WIDE,
		SSIL_INTERLEAVE,
		SSIL_INTERLEAVE_SMART,
		SSIL_INTERLEAVE_HALF,
		SSIL_MAX
	};

	struct SSILGatherPushConstant
	{
		int32_t screen_size[2];
		int pass;
		int quality;

		float half_screen_pixel_size[2];
		float half_screen_pixel_size_x025[2];

		float NDC_to_view_mul[2];
		float NDC_to_view_add[2];

		float pad2[2];
		float z_near;
		float z_far;

		float radius;
		float intensity;
		int size_multiplier;
		int pad;

		float fade_out_mul;
		float fade_out_add;
		float normal_rejection_amount;
		float inv_radius_near_limit;

		uint32_t is_orthogonal;
		float neg_inv_radius;
		float load_counter_avg_div;
		float adaptive_sample_limit;

		int32_t pass_coord_offset[2];
		float pass_uv_offset[2];
	};

	struct SSILImportanceMapPushConstant
	{
		float half_screen_pixel_size[2];
		float intensity;
		float pad;
	};

	struct SSILBlurPushConstant
	{
		float edge_sharpness;
		float pad;
		float half_screen_pixel_size[2];
	};

	struct SSILInterleavePushConstant
	{
		float inv_sharpness;
		uint32_t size_modifier;
		float pixel_size[2];
	};

	struct SSILProjectionUniforms
	{
		float inv_last_frame_projection_matrix[16];
	};

	struct SSIL
	{
		SSILGatherPushConstant gather_push_constant;
		ShaderRD gather_shader{SsilShaderRD()};
		RID gather_shader_version;
		RID projection_uniform_buffer;

		SSILImportanceMapPushConstant importance_map_push_constant;
		ShaderRD importance_map_shader{SsilImportanceMapShaderRD()};
		RID importance_map_shader_version;
		RID importance_map_load_counter;
		RID counter_uniform_set;

		SSILBlurPushConstant blur_push_constant;
		ShaderRD blur_shader{SsilBlurShaderRD()};
		RID blur_shader_version;

		SSILInterleavePushConstant interleave_push_constant;
		ShaderRD interleave_shader{SsilInterleaveShaderRD()};
		RID interleave_shader_version;

		PipelineDeferredRD pipelines[SSIL_MAX];
	};

	static void gather_ssil(RD::ComputeListID p_compute_list, const RID* p_ssil_slices,
		const RID* p_edges_slices, const SSILSettings& p_settings, bool p_adaptive_base_pass,
		RID p_gather_uniform_set, RID p_importance_map_uniform_set, RID p_projection_uniform_set);

	/* SSAO */

	enum SSAOMode
	{
		SSAO_GATHER,
		SSAO_GATHER_BASE,
		SSAO_GATHER_ADAPTIVE,
		SSAO_GENERATE_IMPORTANCE_MAP,
		SSAO_PROCESS_IMPORTANCE_MAPA,
		SSAO_PROCESS_IMPORTANCE_MAPB,
		SSAO_BLUR_PASS,
		SSAO_BLUR_PASS_SMART,
		SSAO_BLUR_PASS_WIDE,
		SSAO_INTERLEAVE,
		SSAO_INTERLEAVE_SMART,
		SSAO_INTERLEAVE_HALF,
		SSAO_MAX
	};

	struct SSAOGatherPushConstant
	{
		int32_t screen_size[2];
		int pass;
		int quality;

		float half_screen_pixel_size[2];
		int size_multiplier;
		float detail_intensity;

		float NDC_to_view_mul[2];
		float NDC_to_view_add[2];

		float pad[2];
		float half_screen_pixel_size_x025[2];

		float radius;
		float intensity;
		float shadow_power;
		float shadow_clamp;

		float fade_out_mul;
		float fade_out_add;
		float horizon_angle_threshold;
		float inv_radius_near_limit;

		uint32_t is_orthogonal;
		float neg_inv_radius;
		float load_counter_avg_div;
		float adaptive_sample_limit;

		int32_t pass_coord_offset[2];
		float pass_uv_offset[2];
	};

	struct SSAOImportanceMapPushConstant
	{
		float half_screen_pixel_size[2];
		float intensity;
		float power;
	};

	struct SSAOBlurPushConstant
	{
		float edge_sharpness;
		float pad;
		float half_screen_pixel_size[2];
	};

	struct SSAOInterleavePushConstant
	{
		float inv_sharpness;
		uint32_t size_modifier;
		float pixel_size[2];
	};

	struct SSAO
	{
		SSAOGatherPushConstant gather_push_constant;
		ShaderRD gather_shader{SsaoShaderRD()};
		RID gather_shader_version;

		SSAOImportanceMapPushConstant importance_map_push_constant;
		ShaderRD importance_map_shader{SsaoImportanceMapShaderRD()};
		RID importance_map_shader_version;
		RID importance_map_load_counter;
		RID counter_uniform_set;

		SSAOBlurPushConstant blur_push_constant;
		ShaderRD blur_shader{SsaoBlurShaderRD()};
		RID blur_shader_version;

		SSAOInterleavePushConstant interleave_push_constant;
		ShaderRD interleave_shader{SsaoInterleaveShaderRD()};
		RID interleave_shader_version;

		PipelineDeferredRD pipelines[SSAO_MAX];
	};

	static void gather_ssao(RD::ComputeListID p_compute_list, const RID* p_ao_slices,
		const SSAOSettings& p_settings, bool p_adaptive_base_pass, RID p_gather_uniform_set,
		RID p_importance_map_uniform_set);

	/* Screen Space Reflection */

	enum ScreenSpaceReflectionDownsampleMode
	{
		SCREEN_SPACE_REFLECTION_DOWNSAMPLE_DEFAULT,
		SCREEN_SPACE_REFLECTION_DOWNSAMPLE_ODD_WIDTH,
		SCREEN_SPACE_REFLECTION_DOWNSAMPLE_ODD_HEIGHT,
		SCREEN_SPACE_REFLECTION_DOWNSAMPLE_ODD_WIDTH_AND_HEIGHT,
		SCREEN_SPACE_REFLECTION_DOWNSAMPLE_MAX
	};

	struct ScreenSpaceReflectionDownsamplePushConstant
	{
		int32_t screen_size[2];
		int32_t pad[2];
	};

	enum ScreenSpaceReflectionHizMode
	{
		SCREEN_SPACE_REFLECTION_HIZ_DEFAULT,
		SCREEN_SPACE_REFLECTION_HIZ_ODD_WIDTH,
		SCREEN_SPACE_REFLECTION_HIZ_ODD_HEIGHT,
		SCREEN_SPACE_REFLECTION_HIZ_ODD_WIDTH_AND_HEIGHT,
		SCREEN_SPACE_REFLECTION_HIZ_MAX
	};

	struct ScreenSpaceReflectionHizPushConstant
	{
		int32_t screen_size[2];
		int32_t pad[2];
	};

	struct ScreenSpaceReflectionSceneData
	{
		float projection[2][16];
		float inv_projection[2][16];
		float reprojection[2][16];
		float eye_offset[2][4];
	};

	struct ScreenSpaceReflectionPushConstant
	{
		int32_t screen_size[2];
		int32_t mipmaps;
		int32_t num_steps;
		float distance_fade;
		float curve_fade_in;
		float depth_tolerance;
		int32_t orthogonal;
		uint32_t view_index;
		int32_t pad[3];
	};

	struct ScreenSpaceReflectionFilterPushConstant
	{
		int32_t screen_size[2];
		uint32_t mip_level;
		int32_t pad;
	};

	struct ScreenSpaceReflectionResolvePushConstant
	{
		int32_t screen_size[2];
		int32_t pad[2];
	};

	struct ScreenSpaceReflection
	{
		ShaderRD downsample_shader{ScreenSpaceReflectionDownsampleShaderRD()};
		RID downsample_shader_version;
		PipelineDeferredRD downsample_pipelines[SCREEN_SPACE_REFLECTION_DOWNSAMPLE_MAX];

		ShaderRD hiz_shader{ScreenSpaceReflectionHizShaderRD()};
		RID hiz_shader_version;
		PipelineDeferredRD hiz_pipelines[SCREEN_SPACE_REFLECTION_HIZ_MAX];

		ShaderRD ssr_shader{ScreenSpaceReflectionShaderRD()};
		RID ssr_shader_version;
		PipelineDeferredRD ssr_pipeline;
		RID ubo;

		ShaderRD filter_shader{ScreenSpaceReflectionFilterShaderRD()};
		RID filter_shader_version;
		PipelineDeferredRD filter_pipeline;

		ShaderRD resolve_shader{ScreenSpaceReflectionResolveShaderRD()};
		RID resolve_shader_version;
		PipelineDeferredRD resolve_pipeline;
	};

	/* Subsurface scattering */

	enum SSSMode
	{
		SUBSURFACE_SCATTERING_MODE_LOW_QUALITY,
		SUBSURFACE_SCATTERING_MODE_MEDIUM_QUALITY,
		SUBSURFACE_SCATTERING_MODE_HIGH_QUALITY,
		SUBSURFACE_SCATTERING_MODE_MAX
	};

	struct SubSurfaceScatteringPushConstant
	{
		int32_t screen_size[2];
		float camera_z_far;
		float camera_z_near;

		uint32_t vertical;
		uint32_t orthogonal;
		float unit_size;
		float scale;

		float depth_scale;
		uint32_t pad[3];
	};

	struct SubSurfaceScattering
	{
		SubSurfaceScatteringPushConstant push_constant;
		ShaderRD shader{SubsurfaceScatteringShaderRD()};
		RID shader_version;
		PipelineDeferredRD pipelines[SUBSURFACE_SCATTERING_MODE_MAX];
	};

	struct Data
	{
		RSE::EnvironmentSSAOQuality ssao_quality = RSE::ENV_SSAO_QUALITY_MEDIUM;
		bool ssao_half_size = false;
		float ssao_adaptive_target = 0.5f;
		int ssao_blur_passes = 2;
		float ssao_fadeout_from = 50.0f;
		float ssao_fadeout_to = 300.0f;

		RSE::EnvironmentSSILQuality ssil_quality = RSE::ENV_SSIL_QUALITY_MEDIUM;
		bool ssil_half_size = false;
		float ssil_adaptive_target = 0.5f;
		int ssil_blur_passes = 4;
		float ssil_fadeout_from = 50.0f;
		float ssil_fadeout_to = 300.0f;

		bool ssr_half_size = false;

		RSE::SubSurfaceScatteringQuality sss_quality = RSE::SUB_SURFACE_SCATTERING_QUALITY_MEDIUM;
		float sss_scale = 0.05f;
		float sss_depth_scale = 0.01f;

		SSEffectsShader ss_effects;
		SSIL ssil;
		SSAO ssao;
		ScreenSpaceReflection ssr;
		SubSurfaceScattering sss;
	};

	static inline Data* data = nullptr;
};

} // namespace RendererRD

// --- End servers/rendering/renderer_rd/effects/ss_effects.h ---
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/rendering_device_binds.h"
#include "servers/rendering/storage/render_scene_buffers.h"

// --- Inlined/Folded from servers/rendering/renderer_rd/effects/upscale_effects.h ---
#include "core/types.h"
#include "servers/rendering/renderer_rd/rd_caches.h"
#include "servers/rendering/renderer_rd/shaders/effects/fsr2/fsr2_accumulate_pass.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/fsr2/fsr2_autogen_reactive_pass.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/fsr2/fsr2_compute_luminance_pyramid_pass.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/fsr2/fsr2_depth_clip_pass.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/fsr2/fsr2_lock_pass.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/fsr2/fsr2_rcas_pass.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/fsr2/fsr2_reconstruct_previous_depth_pass.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/fsr2/fsr2_tcr_autogen_pass.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/fsr_upscale.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/taa_resolve.glsl.gen.h"

#if defined(METAL_ENABLED) && !defined(VISIONOS_ENABLED)
#define METAL_MFXTEMPORAL_ENABLED
#endif

#ifdef METAL_ENABLED
#include "core/templates/paged_allocator.h"
#include "servers/rendering/renderer_scene_render.h"
#include "servers/rendering/rendering_device_driver.h"

namespace MTLFX
{
class SpatialScalerBase;
class TemporalScalerBase;
} // namespace MTLFX
#endif

// This flag doesn't actually control anything GCC specific in FSR2. It determines
// if symbols should be exported, which is not required for Godot.
#ifndef FFX_GCC
#define FFX_GCC
#endif

#include <thirdparty/amd-fsr2/ffx_fsr2.h>

#define FSR2_MAX_QUEUED_FRAMES (4)
#define FSR2_MAX_UNIFORM_BUFFERS (4)
#define FSR2_MAX_BUFFERED_DESCRIPTORS (FFX_FSR2_PASS_COUNT * FSR2_MAX_QUEUED_FRAMES)
#define FSR2_UBO_RING_BUFFER_SIZE (FSR2_MAX_BUFFERED_DESCRIPTORS * FSR2_MAX_UNIFORM_BUFFERS)

class RenderSceneBuffersRD;

class SpatialUpscaler
{
public:
	virtual const Span<char> get_label() const = 0;
	virtual void ensure_context(Ref<RenderSceneBuffersRD> p_render_buffers) = 0;
	virtual void process(Ref<RenderSceneBuffersRD> p_render_buffers, RID p_source_rd_texture,
		RID p_destination_texture) = 0;

	SpatialUpscaler() = default;
	virtual ~SpatialUpscaler() = default;
};

namespace RendererRD
{

class FSR final : public SpatialUpscaler
{
public:
	FSR();
	~FSR();

	virtual const Span<char> get_label() const override { return "FSR 1.0 Upscale"; }

	virtual void ensure_context(Ref<RenderSceneBuffersRD> p_render_buffers) override;

	virtual void process(Ref<RenderSceneBuffersRD> p_render_buffers, RID p_source_rd_texture,
		RID p_destination_texture) override;

private:
	enum FSRShaderVariant
	{
		FSR_SHADER_VARIANT_NORMAL,
		FSR_SHADER_VARIANT_FALLBACK,
	};

	enum FSRUpscalePass
	{
		FSR_UPSCALE_PASS_EASU = 0,
		FSR_UPSCALE_PASS_RCAS = 1
	};

	struct FSRUpscalePushConstant
	{
		float resolution_width;
		float resolution_height;
		float upscaled_width;
		float upscaled_height;
		float sharpness;
		int pass;
		int pad[2];
	};

	ShaderRD fsr_shader{FsrUpscaleShaderRD()};
	RID shader_version;
	PipelineDeferredRD pipeline;
};

class FSR2Context final
{
public:
	enum ResourceID : uint32_t
	{
		RESOURCE_ID_DYNAMIC = 0xFFFFFFFF
	};

	struct Resources
	{
		LocalVector<RID> rids;
		LocalVector<LocalVector<RID>> mip_slice_rids;
		LocalVector<uint32_t> ids;
		LocalVector<FfxResourceDescription> descriptions;
		LocalVector<uint32_t> dynamic_list;
		LocalVector<uint32_t> free_list;

		uint32_t add(
			RID p_rid, bool p_dynamic, uint32_t p_id, FfxResourceDescription p_description)
		{
			uint32_t ret_index;
			if (free_list.is_empty()) {
				ret_index = rids.size();
				uint32_t new_size = ret_index + 1;
				rids.resize(new_size);
				mip_slice_rids.resize(new_size);
				ids.resize(new_size);
				descriptions.resize(new_size);
			}
			else {
				uint32_t end_index = free_list.size() - 1;
				ret_index = free_list[end_index];
				free_list.resize(end_index);
			}

			rids[ret_index] = p_rid;
			mip_slice_rids[ret_index].clear();
			ids[ret_index] = p_id;
			descriptions[ret_index] = p_description;

			if (p_dynamic) {
				dynamic_list.push_back(ret_index);
			}

			return ret_index;
		}

		void remove(uint32_t p_index)
		{
			DEV_ASSERT(p_index < rids.size());
			free_list.push_back(p_index);
			rids[p_index] = RID();
			mip_slice_rids[p_index].clear();
			ids[p_index] = 0;
			descriptions[p_index] = {};
			dynamic_list.erase(p_index);
		}

		uint32_t size() const { return rids.size(); }
	};

	struct Scratch
	{
		Resources resources;
		LocalVector<FfxGpuJobDescription> gpu_jobs;
		RID ubo_ring_buffer[FSR2_UBO_RING_BUFFER_SIZE];
		uint32_t ubo_ring_buffer_index = 0;
		FfxDevice device = nullptr;
	};

	Scratch scratch;
	FfxFsr2Context fsr_context;
	FfxFsr2ContextDescription fsr_desc;

	~FSR2Context();
};

class FSR2Effect final
{
public:
	struct RootSignature
	{
		// Proxy structure to store the shader required by RD that uses the terminology used by the
		// FSR2 API.
		RID shader_rid;
	};

	struct Pipeline
	{
		RID pipeline_rid;
	};

	struct Pass
	{
		ShaderRD* shader;
		RID shader_version;
		RootSignature root_signature;
		uint32_t shader_variant = 0;
		Pipeline pipeline;
		Vector<FfxResourceBinding> sampled_bindings;
		Vector<FfxResourceBinding> storage_bindings;
		Vector<FfxResourceBinding> uniform_bindings;
	};

	struct Device
	{
		Pass passes[FFX_FSR2_PASS_COUNT];
		FfxDeviceCapabilities capabilities;
		RID point_clamp_sampler;
		RID linear_clamp_sampler;
	};

	struct Parameters
	{
		FSR2Context* context;
		Size2i internal_size;
		RID color;
		RID depth;
		RID velocity;
		RID reactive;
		RID exposure;
		RID output;
		float z_near = 0.0f;
		float z_far = 0.0f;
		float fovy = 0.0f;
		Vector2 jitter;
		float delta_time = 0.0f;
		float sharpness = 0.0f;
		bool reset_accumulation = false;
		Projection reprojection;
	};

	FSR2Effect();
	~FSR2Effect();
	FSR2Context* create_context(Size2i p_internal_size, Size2i p_target_size);
	void upscale(const Parameters& p_params);

private:
	struct
	{
		ShaderRD depth_clip{Fsr2DepthClipPassShaderRD()};
		ShaderRD reconstruct_previous_depth{Fsr2ReconstructPreviousDepthPassShaderRD()};
		ShaderRD lock{Fsr2LockPassShaderRD()};
		ShaderRD accumulate{Fsr2AccumulatePassShaderRD()};
		ShaderRD accumulate_sharpen{Fsr2AccumulatePassShaderRD()};
		ShaderRD rcas{Fsr2RcasPassShaderRD()};
		ShaderRD compute_luminance_pyramid{Fsr2ComputeLuminancePyramidPassShaderRD()};
		ShaderRD autogen_reactive{Fsr2AutogenReactivePassShaderRD()};
		ShaderRD tcr_autogen{Fsr2TcrAutogenPassShaderRD()};
	} shaders;

	Device device;
};

class TAA final
{
public:
	TAA();
	~TAA();

	void process(Ref<RenderSceneBuffersRD> p_render_buffers, RDC::DataFormat p_format,
		float p_z_near, float p_z_far);

private:
	struct TAAResolvePushConstant
	{
		float resolution_width;
		float resolution_height;
		float disocclusion_threshold;
		float variance_dynamic;
	};

	ShaderRD taa_shader{TaaResolveShaderRD()};
	RID shader_version;
	RID pipeline;

	void resolve(RID p_frame, RID p_temp, RID p_depth, RID p_velocity, RID p_prev_velocity,
		RID p_history, Size2 p_resolution, float p_z_near, float p_z_far);
};

#ifdef METAL_ENABLED

struct MFXSpatialContext
{
	MTLFX::SpatialScalerBase* scaler = nullptr;
	MFXSpatialContext() = default;
	~MFXSpatialContext();
};

class MFXSpatialEffect final : public SpatialUpscaler
{
	struct CallbackArgs
	{
		MFXSpatialEffect* owner = nullptr;
		MTLFX::SpatialScalerBase* scaler = nullptr;
		RDD::TextureID src;
		RDD::TextureID dst;

		CallbackArgs(MFXSpatialEffect* p_owner, RDD::TextureID p_src, RDD::TextureID p_dst,
			const MFXSpatialContext& p_ctx)
			: owner(p_owner), scaler(p_ctx.scaler), src(p_src), dst(p_dst)
		{
		}

		static void free(CallbackArgs** p_args)
		{
			(*p_args)->owner->args_allocator.free(*p_args);
			*p_args = nullptr;
		}
	};

	PagedAllocator<CallbackArgs, true, 16> args_allocator;
	static void callback(
		RDD* p_driver, RDD::CommandBufferID p_command_buffer, CallbackArgs* p_userdata);

public:
	virtual const Span<char> get_label() const override { return "MetalFX Spatial Upscale"; }
	virtual void ensure_context(Ref<RenderSceneBuffersRD> p_render_buffers) override;
	virtual void process(Ref<RenderSceneBuffersRD> p_render_buffers, RID p_src, RID p_dst) override;

	struct CreateParams
	{
		Vector2i input_size;
		Vector2i output_size;
		RDD::DataFormat input_format;
		RDD::DataFormat output_format;
	};

	MFXSpatialContext* create_context(CreateParams p_params) const;

	MFXSpatialEffect();
	~MFXSpatialEffect();
};

#ifdef METAL_MFXTEMPORAL_ENABLED

struct MFXTemporalContext
{
	MTLFX::TemporalScalerBase* scaler = nullptr;
	MFXTemporalContext() = default;
	~MFXTemporalContext();
};

class MFXTemporalEffect final
{
	struct CallbackArgs
	{
		MFXTemporalEffect* owner = nullptr;
		MTLFX::TemporalScalerBase* scaler = nullptr;
		RDD::TextureID src;
		RDD::TextureID depth;
		RDD::TextureID motion;
		RDD::TextureID exposure;
		Vector2 jitter_offset;
		RDD::TextureID dst;
		bool reset = false;

		CallbackArgs(MFXTemporalEffect* p_owner, RDD::TextureID p_src, RDD::TextureID p_depth,
			RDD::TextureID p_motion, RDD::TextureID p_exposure, Vector2 p_jitter_offset,
			RDD::TextureID p_dst, const MFXTemporalContext& p_ctx, bool p_reset)
			: owner(p_owner), scaler(p_ctx.scaler), src(p_src), depth(p_depth), motion(p_motion),
			  exposure(p_exposure), jitter_offset(p_jitter_offset), dst(p_dst), reset(p_reset)
		{
		}

		static void free(CallbackArgs** p_args)
		{
			(*p_args)->owner->args_allocator.free(*p_args);
			*p_args = nullptr;
		}
	};

	PagedAllocator<CallbackArgs, true, 16> args_allocator;

	static void callback(
		RDD* p_driver, RDD::CommandBufferID p_command_buffer, CallbackArgs* p_userdata);

public:
	MFXTemporalEffect();
	~MFXTemporalEffect();

	struct CreateParams
	{
		Vector2i input_size;
		Vector2i output_size;
		RDD::DataFormat input_format;
		RDD::DataFormat depth_format;
		RDD::DataFormat motion_format;
		RDD::DataFormat reactive_format;
		RDD::DataFormat output_format;
		Vector2 motion_vector_scale;
	};

	MFXTemporalContext* create_context(CreateParams p_params) const;

	struct Params
	{
		RID src;
		RID depth;
		RID motion;
		RID exposure;
		RID dst;
		Vector2 jitter_offset;
		bool reset = false;
	};

	void process(MFXTemporalContext* p_ctx, Params p_params);
};

#endif // METAL_MFXTEMPORAL_ENABLED

#endif // METAL_ENABLED

} // namespace RendererRD

#define RB_SCOPE_BUFFERS SNAME("render_buffers")
#define RB_SCOPE_VRS SNAME("VRS")

#define RB_TEXTURE SNAME("texture")
#define RB_TEX_COLOR SNAME("color")
#define RB_TEX_COLOR_MSAA SNAME("color_msaa")
#define RB_TEX_COLOR_UPSCALED SNAME("color_upscaled")
#define RB_TEX_COLOR_SUBSAMPLED SNAME("color_subsampled")
#define RB_TEX_COLOR_MSAA_SUBSAMPLED SNAME("color_msaa_subsampled")
#define RB_TEX_DEPTH SNAME("depth")
#define RB_TEX_DEPTH_MSAA SNAME("depth_msaa")
#define RB_TEX_DEPTH_SUBSAMPLED SNAME("depth_subsampled")
#define RB_TEX_DEPTH_MSAA_SUBSAMPLED SNAME("depth_msaa_subsampled")
#define RB_TEX_VELOCITY SNAME("velocity")
#define RB_TEX_VELOCITY_MSAA SNAME("velocity_msaa")

#define RB_TEX_BLUR_0 SNAME("blur_0")
#define RB_TEX_BLUR_1 SNAME("blur_1")

#define RB_TEX_BACK_COLOR SNAME("back_color")
#define RB_TEX_BACK_DEPTH SNAME("back_depth")

class RenderSceneBuffersRD : public RenderSceneBuffers
{
private:
	bool can_be_storage = true;
	bool force_hdr = false;
	uint32_t max_cluster_elements = 512;
	RDC::DataFormat preferred_data_format = RDC::DATA_FORMAT_MAX;
	RendererRD::VRS* vrs = nullptr;
	uint64_t auto_exposure_version = 1;
	RSE::ViewportVRSMode vrs_mode = RSE::VIEWPORT_VRS_DISABLED;

	// Our render target represents our final destination that we display on screen.
	RID render_target;
	Size2i target_size = Size2i(0, 0);
	uint32_t view_count = 1;

	// The internal size of the textures we render 3D to in case we render at a lower resolution and
	// upscale
	Size2i internal_size = Size2i(0, 0);
	RSE::ViewportScaling3DMode scaling_3d_mode = RSE::VIEWPORT_SCALING_3D_MODE_OFF;
	float fsr_sharpness = 0.2f;
	float texture_mipmap_bias = 0.0f;
	RSE::ViewportAnisotropicFiltering anisotropic_filtering_level = RSE::VIEWPORT_ANISOTROPY_4X;

#ifdef METAL_ENABLED
	RendererRD::MFXSpatialContext* mfx_spatial_context = nullptr;
#endif

	// Aliasing settings
	RSE::ViewportMSAA msaa_3d = RSE::VIEWPORT_MSAA_DISABLED;
	RSE::ViewportScreenSpaceAA screen_space_aa = RSE::VIEWPORT_SCREEN_SPACE_AA_DISABLED;
	bool use_taa = false;
	bool use_debanding = false;
	RDC::TextureSamples texture_samples = RDC::TEXTURE_SAMPLES_1;

	// Named Textures

	struct NTKey
	{
		String context;
		String buffer_name;

		bool operator==(const NTKey& p_val) const
		{
			return (context == p_val.context) && (buffer_name == p_val.buffer_name);
		}

		static uint32_t hash(const NTKey& p_val)
		{
			uint32_t h = p_val.context.hash();
			h = hash_murmur3_one_32(p_val.buffer_name.hash(), h);
			return hash_fmix32(h);
		}

		NTKey() {}

		NTKey(const String& p_context, const String& p_texture_name)
		{
			context = p_context;
			buffer_name = p_texture_name;
		}
	};

	struct NTSliceKey
	{
		uint32_t layer;
		uint32_t layers;
		uint32_t mipmap;
		uint32_t mipmaps;
		RD::TextureView texture_view;

		bool operator==(const NTSliceKey& p_val) const
		{
			return (layer == p_val.layer) && (layers == p_val.layers) && (mipmap == p_val.mipmap) &&
				   (mipmaps == p_val.mipmaps) && (texture_view == p_val.texture_view);
		}

		static uint32_t hash(const NTSliceKey& p_val)
		{
			uint32_t h = hash_murmur3_one_32(p_val.layer);
			h = hash_murmur3_one_32(p_val.layers, h);
			h = hash_murmur3_one_32(p_val.mipmap, h);
			h = hash_murmur3_one_32(p_val.mipmaps, h);
			h = hash_murmur3_one_32(p_val.texture_view.format_override, h);
			h = hash_murmur3_one_32(p_val.texture_view.swizzle_r, h);
			h = hash_murmur3_one_32(p_val.texture_view.swizzle_g, h);
			h = hash_murmur3_one_32(p_val.texture_view.swizzle_b, h);
			h = hash_murmur3_one_32(p_val.texture_view.swizzle_a, h);
			return hash_fmix32(h);
		}

		NTSliceKey() {}

		NTSliceKey(uint32_t p_layer, uint32_t p_layers, uint32_t p_mipmap, uint32_t p_mipmaps,
			RD::TextureView p_texture_view)
		{
			layer = p_layer;
			layers = p_layers;
			mipmap = p_mipmap;
			mipmaps = p_mipmaps;
			texture_view = p_texture_view;
		}
	};

	struct NamedTexture
	{
		// Cache the data used to create our texture
		RDC::TextureFormat format;
		bool is_unique; // If marked as unique, we return it into our pool

		// Our texture objects, slices are lazy (i.e. only created when requested).
		RID texture;
		mutable HashMap<NTSliceKey, RID, NTSliceKey> slices;
		Vector<Size2i> sizes;
	};

	mutable HashMap<NTKey, NamedTexture, NTKey> named_textures;
	void update_sizes(NamedTexture& p_named_texture);
	void free_named_texture(NamedTexture& p_named_texture);

	struct CustomDataEntry
	{
		Ref<RefCounted> data;
		void (*configure_fn)(RefCounted*, RenderSceneBuffersRD*) = nullptr;
		void (*free_fn)(RefCounted*) = nullptr;
	};

	// Data buffers
	mutable HashMap<String, CustomDataEntry> data_buffers;

	// Samplers.
	RendererRD::Samplers samplers;

	void update_samplers();

protected:

public:
	RenderSceneBuffersRD();
	virtual ~RenderSceneBuffersRD();

	// info from our renderer
	void set_can_be_storage(const bool p_can_be_storage) { can_be_storage = p_can_be_storage; }

	bool get_can_be_storage() const { return can_be_storage; }

	void set_max_cluster_elements(const uint32_t p_max_elements)
	{
		max_cluster_elements = p_max_elements;
	}

	uint32_t get_max_cluster_elements() { return max_cluster_elements; }

	void set_preferred_data_format(const RDC::DataFormat p_preferred_data_format)
	{
		preferred_data_format = p_preferred_data_format;
	}

	RDC::DataFormat get_preferred_data_format() const { return preferred_data_format; }

	RDC::DataFormat get_base_data_format() const
	{
		return force_hdr ? RDC::DATA_FORMAT_R16G16B16A16_SFLOAT : preferred_data_format;
	}

	float get_luminance_multiplier() const;

	void set_vrs(RendererRD::VRS* p_vrs) { vrs = p_vrs; }

	RSE::ViewportVRSMode get_vrs_mode() { return vrs_mode; }

	void cleanup();
	virtual void configure(const RenderSceneBuffersConfiguration* p_config) override;
	void configure_for_reflections(const Size2i p_reflection_size);
	virtual void set_fsr_sharpness(float p_fsr_sharpness) override;
	virtual void set_texture_mipmap_bias(float p_texture_mipmap_bias) override;
	virtual void set_anisotropic_filtering_level(
		RSE::ViewportAnisotropicFiltering p_anisotropic_filtering_level) override;
	virtual void set_use_debanding(bool p_use_debanding) override;

#ifdef METAL_ENABLED
	void ensure_mfx(RendererRD::MFXSpatialEffect* p_effect);

	_FORCE_INLINE_ RendererRD::MFXSpatialContext* get_mfx_spatial_context() const
	{
		return mfx_spatial_context;
	}
#endif

	// Named Textures

	bool has_texture(const String& p_context, const String& p_texture_name) const;
	RID create_texture(const String& p_context, const String& p_texture_name,
		const RDC::DataFormat p_data_format, const uint32_t p_usage_bits,
		const RDC::TextureSamples p_texture_samples = RDC::TEXTURE_SAMPLES_1,
		const Size2i p_size = Size2i(0, 0), const uint32_t p_layers = 0,
		const uint32_t p_mipmaps = 1, bool p_unique = true, bool p_discardable = false);
	RID create_texture_from_format(const String& p_context, const String& p_texture_name,
		const RDC::TextureFormat& p_texture_format, RD::TextureView p_view = RD::TextureView(),
		bool p_unique = true);
	RID create_texture_view(const String& p_context, const String& p_texture_name,
		const String& p_view_name, RD::TextureView p_view = RD::TextureView());
	RID get_texture(const String& p_context, const String& p_texture_name) const;
	const RDC::TextureFormat get_texture_format(
		const String& p_context, const String& p_texture_name) const;
	RID get_texture_slice(const String& p_context, const String& p_texture_name,
		const uint32_t p_layer, const uint32_t p_mipmap, const uint32_t p_layers = 1,
		const uint32_t p_mipmaps = 1);
	RID get_texture_slice_view(const String& p_context, const String& p_texture_name,
		const uint32_t p_layer, const uint32_t p_mipmap, const uint32_t p_layers = 1,
		const uint32_t p_mipmaps = 1, RD::TextureView p_view = RD::TextureView());
	Size2i get_texture_slice_size(
		const String& p_context, const String& p_texture_name, const uint32_t p_mipmap);

	void clear_context(const String& p_context);

	// Allocate shared buffers
	void allocate_blur_textures();

	// Custom data
	bool has_custom_data(const String& p_name) const;

	template <typename T>
	void set_custom_data(const String& p_name, const Ref<T>& p_data)
	{
		static_assert(std::is_base_of_v<RefCounted, T>, "Custom data must inherit RefCounted.");
		if (p_data.is_valid()) {
			data_buffers[p_name] = {
				p_data,
				+[](RefCounted* p_obj, RenderSceneBuffersRD* p_rb) {
					static_cast<T*>(p_obj)->configure(p_rb);
				},
				+[](RefCounted* p_obj) { static_cast<T*>(p_obj)->free_data(); }};
		}
		else if (data_buffers.has(p_name)) {
			data_buffers.erase(p_name);
		}
	}

	Ref<RefCounted> get_custom_data(const String& p_name) const;

	// Getters

	_FORCE_INLINE_ RID get_render_target() const { return render_target; }

	_FORCE_INLINE_ uint32_t get_view_count() const { return view_count; }

	_FORCE_INLINE_ Size2i get_internal_size() const { return internal_size; }

	_FORCE_INLINE_ Size2i get_target_size() const { return target_size; }

	_FORCE_INLINE_ RSE::ViewportScaling3DMode get_scaling_3d_mode() const
	{
		return scaling_3d_mode;
	}

	_FORCE_INLINE_ float get_fsr_sharpness() const { return fsr_sharpness; }

	_FORCE_INLINE_ RSE::ViewportMSAA get_msaa_3d() const { return msaa_3d; }

	_FORCE_INLINE_ RDC::TextureSamples get_texture_samples() const { return texture_samples; }

	_FORCE_INLINE_ RSE::ViewportScreenSpaceAA get_screen_space_aa() const
	{
		return screen_space_aa;
	}

	_FORCE_INLINE_ bool get_use_taa() const { return use_taa; }

	_FORCE_INLINE_ bool get_use_debanding() const { return use_debanding; }

	uint64_t get_auto_exposure_version() const { return auto_exposure_version; }

	void set_auto_exposure_version(const uint64_t p_auto_exposure_version)
	{
		auto_exposure_version = p_auto_exposure_version;
	}

	// For our internal textures we provide some easy access methods.

	_FORCE_INLINE_ bool has_internal_texture() const
	{
		return has_texture(RB_SCOPE_BUFFERS, RB_TEX_COLOR);
	}

	_FORCE_INLINE_ RID get_internal_texture() const
	{
		return get_texture(RB_SCOPE_BUFFERS, RB_TEX_COLOR);
	}

	_FORCE_INLINE_ RID get_internal_texture(const uint32_t p_layer)
	{
		return get_texture_slice(RB_SCOPE_BUFFERS, RB_TEX_COLOR, p_layer, 0);
	}

	_FORCE_INLINE_ RID get_internal_texture_reactive(const uint32_t p_layer)
	{
		RD::TextureView alpha_only_view;
		alpha_only_view.swizzle_r = RDC::TEXTURE_SWIZZLE_A;
		alpha_only_view.swizzle_g = RDC::TEXTURE_SWIZZLE_A;
		alpha_only_view.swizzle_b = RDC::TEXTURE_SWIZZLE_A;
		alpha_only_view.swizzle_a = RDC::TEXTURE_SWIZZLE_A;
		return get_texture_slice_view(
			RB_SCOPE_BUFFERS, RB_TEX_COLOR, p_layer, 0, 1, 1, alpha_only_view);
	}

	_FORCE_INLINE_ RID get_color_msaa() const
	{
		return get_texture(RB_SCOPE_BUFFERS, RB_TEX_COLOR_MSAA);
	}

	_FORCE_INLINE_ RID get_color_msaa(uint32_t p_layer)
	{
		return get_texture_slice(RB_SCOPE_BUFFERS, RB_TEX_COLOR_MSAA, p_layer, 0);
	}

	bool has_depth_texture();
	RID get_depth_texture();
	RID get_depth_texture(const uint32_t p_layer);

	RID get_depth_msaa() const { return get_texture(RB_SCOPE_BUFFERS, RB_TEX_DEPTH_MSAA); }

	RID get_depth_msaa(uint32_t p_layer)
	{
		return get_texture_slice(RB_SCOPE_BUFFERS, RB_TEX_DEPTH_MSAA, p_layer, 0);
	}

	RID get_color_subsampled();
	RID get_color_msaa_subsampled();
	RID get_depth_subsampled();
	RID get_depth_msaa_subsampled();

	// back buffer (color)
	RID get_back_buffer_texture() const
	{
		// Prefer returning the dedicated backbuffer color texture if it was created. Return the
		// reused blur texture otherwise.
		if (has_texture(RB_SCOPE_BUFFERS, RB_TEX_BACK_COLOR)) {
			return get_texture(RB_SCOPE_BUFFERS, RB_TEX_BACK_COLOR);
		}
		else if (has_texture(RB_SCOPE_BUFFERS, RB_TEX_BLUR_0)) {
			return get_texture(RB_SCOPE_BUFFERS, RB_TEX_BLUR_0);
		}
		else {
			return RID();
		}
	}

	// Upscaled.
	void ensure_upscaled();

	_FORCE_INLINE_ bool has_upscaled_texture() const
	{
		return has_texture(RB_SCOPE_BUFFERS, RB_TEX_COLOR_UPSCALED);
	}

	_FORCE_INLINE_ RID get_upscaled_texture() const
	{
		return get_texture(RB_SCOPE_BUFFERS, RB_TEX_COLOR_UPSCALED);
	}

	_FORCE_INLINE_ RID get_upscaled_texture(const uint32_t p_layer)
	{
		return get_texture_slice(RB_SCOPE_BUFFERS, RB_TEX_COLOR_UPSCALED, p_layer, 0);
	}

	// Velocity, currently only used by TAA (Clustered) but we'll be using this in other places soon
	// too.

	void ensure_velocity();
	bool has_velocity_buffer(bool p_has_msaa);
	RID get_velocity_buffer(bool p_get_msaa);
	RID get_velocity_buffer(bool p_get_msaa, uint32_t p_layer);

	RID get_velocity_depth_buffer();

	// Samplers adjusted with the mipmap bias that is best fit for the configuration of these render
	// buffers.

	_FORCE_INLINE_ RendererRD::Samplers get_samplers() const { return samplers; }

	_FORCE_INLINE_ static RDC::TextureSamples msaa_to_samples(RSE::ViewportMSAA p_msaa)
	{
		switch (p_msaa) {
		case RSE::VIEWPORT_MSAA_DISABLED:
			return RDC::TEXTURE_SAMPLES_1;
		case RSE::VIEWPORT_MSAA_2X:
			return RDC::TEXTURE_SAMPLES_2;
		case RSE::VIEWPORT_MSAA_4X:
			return RDC::TEXTURE_SAMPLES_4;
		case RSE::VIEWPORT_MSAA_8X:
			return RDC::TEXTURE_SAMPLES_8;
		default:
			DEV_ASSERT(false && "Unknown MSAA option.");
			return RDC::TEXTURE_SAMPLES_1;
		}
	}

	static uint32_t get_color_usage_bits(bool p_resolve, bool p_msaa, bool p_storage);
	static RDC::DataFormat get_depth_format(bool p_resolve, bool p_msaa, bool p_storage);
	static uint32_t get_depth_usage_bits(bool p_resolve, bool p_msaa, bool p_storage);
	static RDC::DataFormat get_velocity_format();
	static uint32_t get_velocity_usage_bits(bool p_resolve, bool p_msaa, bool p_storage);
	static RDC::DataFormat get_vrs_format();
	static uint32_t get_vrs_usage_bits();

private:
	RID _create_texture_from_format(const String& p_context, const String& p_texture_name,
		const Ref<RDTextureFormat>& p_texture_format,
		const Ref<RDTextureView>& p_view = Ref<RDTextureView>(), bool p_unique = true);
	RID _create_texture_view(const String& p_context, const String& p_texture_name,
		const String& p_view_name, const Ref<RDTextureView> p_view = Ref<RDTextureView>());
	Ref<RDTextureFormat> _get_texture_format(
		const String& p_context, const String& p_texture_name) const;
	RID _get_texture_slice_view(const String& p_context, const String& p_texture_name,
		const uint32_t p_layer, const uint32_t p_mipmap, const uint32_t p_layers = 1,
		const uint32_t p_mipmaps = 1, const Ref<RDTextureView> p_view = Ref<RDTextureView>());

	// For color and depth as exposed to extensions:
	// - we need separately named functions to access the layer,
	// - we don't output an error for missing buffers but just return an empty RID.
	RID _get_color_texture(bool p_msaa = false)
	{
		if (p_msaa) {
			if (has_texture(RB_SCOPE_BUFFERS, RB_TEX_COLOR_MSAA)) {
				return get_texture(RB_SCOPE_BUFFERS, RB_TEX_COLOR_MSAA);
			}
			else {
				return RID();
			}
		}
		else if (has_internal_texture()) {
			return get_internal_texture();
		}
		else {
			return RID();
		}
	}

	RID _get_color_layer(const uint32_t p_layer, bool p_msaa = false)
	{
		if (p_msaa) {
			if (has_texture(RB_SCOPE_BUFFERS, RB_TEX_COLOR_MSAA)) {
				return get_texture_slice(RB_SCOPE_BUFFERS, RB_TEX_COLOR_MSAA, p_layer, 0);
			}
			else {
				return RID();
			}
		}
		else if (has_internal_texture()) {
			return get_internal_texture(p_layer);
		}
		else {
			return RID();
		}
	}

	RID _get_depth_texture(bool p_msaa = false)
	{
		if (p_msaa) {
			if (has_texture(RB_SCOPE_BUFFERS, RB_TEX_DEPTH_MSAA)) {
				return get_texture(RB_SCOPE_BUFFERS, RB_TEX_DEPTH_MSAA);
			}
			else {
				return RID();
			}
		}
		else if (has_depth_texture()) {
			return get_depth_texture();
		}
		else {
			return RID();
		}
	}

	RID _get_depth_layer(const uint32_t p_layer, bool p_msaa = false)
	{
		if (p_msaa) {
			if (has_texture(RB_SCOPE_BUFFERS, RB_TEX_DEPTH_MSAA)) {
				return get_texture_slice(RB_SCOPE_BUFFERS, RB_TEX_DEPTH_MSAA, p_layer, 0);
			}
			else {
				return RID();
			}
		}
		else if (has_depth_texture()) {
			return get_depth_texture(p_layer);
		}
		else {
			return RID();
		}
	}

	RID _get_velocity_texture(bool p_msaa = false)
	{
		if (has_velocity_buffer(p_msaa)) {
			return get_velocity_buffer(p_msaa);
		}
		else {
			return RID();
		}
	}

	RID _get_velocity_layer(const uint32_t p_layer, bool p_msaa = false)
	{
		if (has_velocity_buffer(p_msaa)) {
			return get_velocity_buffer(p_msaa, p_layer);
		}
		else {
			return RID();
		}
	}

public:
	struct WeightBuffers
	{
		RID weight;
		RID fb; // FB with both texture and weight writing into one level lower
	};

	// 2 full size, 2 half size
	WeightBuffers weight_buffers[4]; // Only used in raster
};


