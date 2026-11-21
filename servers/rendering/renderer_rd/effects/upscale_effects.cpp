/**************************************************************************/
/*  upscale_effects.cpp                                                   */
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

#include "upscale_effects.h"
#include "servers/rendering/renderer_rd/effects/copy_effects.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/render_scene_buffers_rd.h"
#include "servers/rendering/renderer_rd/uniform_set_cache_rd.h"
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
