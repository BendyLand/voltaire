/**************************************************************************/
/*  post_effects.h                                                        */
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

#include "core/types.h"
#include "servers/rendering/renderer_rd/pipeline_cache_rd.h"
#include "servers/rendering/renderer_rd/pipeline_deferred_rd.h"
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
