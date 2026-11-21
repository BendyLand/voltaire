/**************************************************************************/
/*  upscale_effects.h                                                     */
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
#include "servers/rendering/renderer_rd/pipeline_deferred_rd.h"
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
