/**************************************************************************/
/*  rendering_device_driver_metal.cpp                                     */
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

/**************************************************************************/
/*                                                                        */
/* Portions of this code were derived from MoltenVK.                      */
/*                                                                        */
/* Copyright (c) 2015-2023 The Brenwill Workshop Ltd.                     */
/* (http://www.brenwill.com)                                              */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License");        */
/* you may not use this file except in compliance with the License.       */
/* You may obtain a copy of the License at                                */
/*                                                                        */
/*     http://www.apache.org/licenses/LICENSE-2.0                         */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
/**************************************************************************/

#include "drivers/metal/rendering_device_driver_metal3.h"

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/marshalls.h"
#include "core/os/os.h"
#include "core/string/ustring.h"
#include "core/templates/hash_map.h"
#include "drivers/apple/foundation_helpers.h"
#include "drivers/metal/rendering_device_driver_metal3.h"
#include "drivers/metal/rendering_context_driver_metal.h"
#include "drivers/metal/rendering_device_driver_metal3.h"

#include <Metal/Metal.hpp>
#include <os/log.h>
#include <os/signpost.h>

#include <algorithm>

#ifndef MTLGPUAddress
typedef uint64_t MTLGPUAddress;
#endif

#pragma mark - Logging

extern os_log_t LOG_DRIVER;
// Used for dynamic tracing.
extern os_log_t LOG_INTERVALS;

/*****************/
/**** GENERIC ****/
/*****************/

// RDD::CompareOperator == VkCompareOp.
static_assert(ENUM_MEMBERS_EQUAL(RDD::COMPARE_OP_NEVER, MTL::CompareFunctionNever));
static_assert(ENUM_MEMBERS_EQUAL(RDD::COMPARE_OP_LESS, MTL::CompareFunctionLess));
static_assert(ENUM_MEMBERS_EQUAL(RDD::COMPARE_OP_EQUAL, MTL::CompareFunctionEqual));
static_assert(ENUM_MEMBERS_EQUAL(RDD::COMPARE_OP_LESS_OR_EQUAL, MTL::CompareFunctionLessEqual));
static_assert(ENUM_MEMBERS_EQUAL(RDD::COMPARE_OP_GREATER, MTL::CompareFunctionGreater));
static_assert(ENUM_MEMBERS_EQUAL(RDD::COMPARE_OP_NOT_EQUAL, MTL::CompareFunctionNotEqual));
static_assert(ENUM_MEMBERS_EQUAL(RDD::COMPARE_OP_GREATER_OR_EQUAL, MTL::CompareFunctionGreaterEqual));
static_assert(ENUM_MEMBERS_EQUAL(RDD::COMPARE_OP_ALWAYS, MTL::CompareFunctionAlways));

/*****************/
/**** BUFFERS ****/
/*****************/

RDD::BufferID RenderingDeviceDriverMetal::buffer_create(uint64_t p_size, uint32_t p_usage, MemoryAllocationType p_allocation_type, uint64_t p_frames_drawn) {
	const uint64_t original_size = p_size;
	if (p_usage.has_flag(BUFFER_USAGE_DYNAMIC_PERSISTENT_BIT)) {
		p_size = round_up_to_alignment(p_size, 16u) * _frame_count;
	}

	MTL::ResourceOptions options = 0;
	switch (p_allocation_type) {
		case MEMORY_ALLOCATION_TYPE_CPU:
			options = base_hazard_tracking | MTL::ResourceStorageModeShared;
			break;
		case MEMORY_ALLOCATION_TYPE_GPU:
			if (p_usage.has_flag(BUFFER_USAGE_DYNAMIC_PERSISTENT_BIT)) {
				options = MTL::ResourceHazardTrackingModeUntracked | MTL::ResourceStorageModeShared | MTL::ResourceCPUCacheModeWriteCombined;
			} else {
				options = base_hazard_tracking | MTL::ResourceStorageModePrivate;
			}
			break;
	}

	MTL::Buffer *obj = device->newBuffer(p_size, options);
	ERR_FAIL_NULL_V_MSG(obj, BufferID(), "Can't create buffer of size: " + itos(p_size));

	BufferInfo *buf_info;
	if (p_usage.has_flag(BUFFER_USAGE_DYNAMIC_PERSISTENT_BIT)) {
		MetalBufferDynamicInfo *dyn_buffer = memnew(MetalBufferDynamicInfo);
		buf_info = dyn_buffer;
#ifdef DEBUG_ENABLED
		dyn_buffer->last_frame_mapped = p_frames_drawn - 1ul;
#endif
		dyn_buffer->set_frame_index(0u);
		dyn_buffer->size_bytes = round_up_to_alignment(original_size, 16u);
	} else {
		buf_info = memnew(BufferInfo);
	}
	buf_info->metal_buffer = NS::TransferPtr(obj);

	_track_resource(buf_info->metal_buffer.get());

	return BufferID(buf_info);
}

bool RenderingDeviceDriverMetal::buffer_set_texel_format(BufferID p_buffer, DataFormat p_format) {
	// Nothing to do.
	return true;
}

void RenderingDeviceDriverMetal::buffer_free(BufferID p_buffer) {
	BufferInfo *buf_info = (BufferInfo *)p_buffer.id;

	_untrack_resource(buf_info->metal_buffer.get());

	if (buf_info->is_dynamic()) {
		memdelete((MetalBufferDynamicInfo *)buf_info);
	} else {
		memdelete(buf_info);
	}
}

uint64_t RenderingDeviceDriverMetal::buffer_get_allocation_size(BufferID p_buffer) {
	const BufferInfo *buf_info = (const BufferInfo *)p_buffer.id;
	return buf_info->metal_buffer.get()->allocatedSize();
}

uint8_t *RenderingDeviceDriverMetal::buffer_map(BufferID p_buffer) {
	const BufferInfo *buf_info = (const BufferInfo *)p_buffer.id;
	ERR_FAIL_COND_V_MSG(buf_info->metal_buffer.get()->storageMode() != MTL::StorageModeShared, nullptr, "Unable to map private buffers");
	return (uint8_t *)buf_info->metal_buffer.get()->contents();
}

void RenderingDeviceDriverMetal::buffer_unmap(BufferID p_buffer) {
	// Nothing to do.
}

uint8_t *RenderingDeviceDriverMetal::buffer_persistent_map_advance(BufferID p_buffer, uint64_t p_frames_drawn) {
	MetalBufferDynamicInfo *buf_info = (MetalBufferDynamicInfo *)p_buffer.id;
	ERR_FAIL_COND_V_MSG(!buf_info->is_dynamic(), nullptr, "Buffer must have BUFFER_USAGE_DYNAMIC_PERSISTENT_BIT. Use buffer_map() instead.");
#ifdef DEBUG_ENABLED
	ERR_FAIL_COND_V_MSG(buf_info->last_frame_mapped == p_frames_drawn, nullptr, "Buffers with BUFFER_USAGE_DYNAMIC_PERSISTENT_BIT must only be mapped once per frame. Otherwise there could be race conditions with the GPU. Amalgamate all data uploading into one map(), use an extra buffer or remove the bit.");
	buf_info->last_frame_mapped = p_frames_drawn;
#endif
	return (uint8_t *)buf_info->metal_buffer.get()->contents() + buf_info->next_frame_index(_frame_count) * buf_info->size_bytes;
}

uint64_t RenderingDeviceDriverMetal::buffer_get_dynamic_offsets(Span<BufferID> p_buffers) {
	uint64_t mask = 0u;
	uint64_t shift = 0u;

	for (const BufferID &buf : p_buffers) {
		const BufferInfo *buf_info = (const BufferInfo *)buf.id;
		if (!buf_info->is_dynamic()) {
			continue;
		}
		mask |= buf_info->frame_index() << shift;
		// We can encode the frame index in 2 bits since frame_count won't be > 4.
		shift += 2UL;
	}

	return mask;
}

uint64_t RenderingDeviceDriverMetal::buffer_get_device_address(BufferID p_buffer) {
	if (__builtin_available(iOS 16.0, macOS 13.0, *)) {
		const BufferInfo *buf_info = (const BufferInfo *)p_buffer.id;
		return buf_info->metal_buffer.get()->gpuAddress();
	} else {
#if DEV_ENABLED
		WARN_PRINT_ONCE("buffer_get_device_address is not supported on this OS version.");
#endif
		return 0;
	}
}

#pragma mark - Texture

#pragma mark - Format Conversions

static const MTL::TextureType TEXTURE_TYPE[RDD::TEXTURE_TYPE_MAX] = {
	MTL::TextureType1D,
	MTL::TextureType2D,
	MTL::TextureType3D,
	MTL::TextureTypeCube,
	MTL::TextureType1DArray,
	MTL::TextureType2DArray,
	MTL::TextureTypeCubeArray,
};

bool RenderingDeviceDriverMetal::is_valid_linear(const TextureFormat &p_format) const {
	MTLFormatType ft = pixel_formats->getFormatType(p_format.format);

	return p_format.texture_type == TEXTURE_TYPE_2D // Linear textures must be 2D textures.
			&& ft != MTLFormatType::DepthStencil && ft != MTLFormatType::Compressed // Linear textures must not be depth/stencil or compressed formats.)
			&& p_format.mipmaps == 1 // Linear textures must have 1 mipmap level.
			&& p_format.array_layers == 1 // Linear textures must have 1 array layer.
			&& p_format.samples == TEXTURE_SAMPLES_1; // Linear textures must have 1 sample.
}

RDD::TextureID RenderingDeviceDriverMetal::texture_create(const TextureFormat &p_format, const TextureView &p_view) {
	NS::SharedPtr<MTL::TextureDescriptor> desc = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
	desc->setTextureType(TEXTURE_TYPE[p_format.texture_type]);

	PixelFormats &formats = *pixel_formats;
	desc->setPixelFormat((MTL::PixelFormat)formats.getMTLPixelFormat(p_format.format));
	MTLFmtCaps format_caps = formats.getCapabilities(desc->pixelFormat());

	desc->setWidth(p_format.width);
	desc->setHeight(p_format.height);
	desc->setDepth(p_format.depth);
	desc->setMipmapLevelCount(p_format.mipmaps);

	if (p_format.texture_type == TEXTURE_TYPE_1D_ARRAY ||
			p_format.texture_type == TEXTURE_TYPE_2D_ARRAY) {
		desc->setArrayLength(p_format.array_layers);
	} else if (p_format.texture_type == TEXTURE_TYPE_CUBE_ARRAY) {
		desc->setArrayLength(p_format.array_layers / 6);
	}

	// TODO(sgc): Evaluate lossy texture support (perhaps as a project option?)
	//  https://developer.apple.com/videos/play/tech-talks/10876?time=459
	// desc->setCompressionType(MTL::TextureCompressionTypeLossy);

	if (p_format.samples > TEXTURE_SAMPLES_1) {
		SampleCount supported = (*device_properties).find_nearest_supported_sample_count(p_format.samples);

		if (supported > SampleCount1) {
			bool ok = p_format.texture_type == TEXTURE_TYPE_2D || p_format.texture_type == TEXTURE_TYPE_2D_ARRAY;
			if (ok) {
				switch (p_format.texture_type) {
					case TEXTURE_TYPE_2D:
						desc->setTextureType(MTL::TextureType2DMultisample);
						break;
					case TEXTURE_TYPE_2D_ARRAY:
						desc->setTextureType(MTL::TextureType2DMultisampleArray);
						break;
					default:
						break;
				}
				desc->setSampleCount((NS::UInteger)supported);
				if (p_format.mipmaps > 1) {
					// For a buffer-backed or multi-sample texture, the value must be 1.
					WARN_PRINT("mipmaps == 1 for multi-sample textures");
					desc->setMipmapLevelCount(1);
				}
			} else {
				WARN_PRINT("Unsupported multi-sample texture type; disabling multi-sample");
			}
		}
	}

	static const MTL::TextureSwizzle COMPONENT_SWIZZLE[TEXTURE_SWIZZLE_MAX] = {
		static_cast<MTL::TextureSwizzle>(255), // IDENTITY
		MTL::TextureSwizzleZero,
		MTL::TextureSwizzleOne,
		MTL::TextureSwizzleRed,
		MTL::TextureSwizzleGreen,
		MTL::TextureSwizzleBlue,
		MTL::TextureSwizzleAlpha,
	};

	MTL::TextureSwizzleChannels swizzle = MTL::TextureSwizzleChannels::Make(
			p_view.swizzle_r != TEXTURE_SWIZZLE_IDENTITY ? COMPONENT_SWIZZLE[p_view.swizzle_r] : MTL::TextureSwizzleRed,
			p_view.swizzle_g != TEXTURE_SWIZZLE_IDENTITY ? COMPONENT_SWIZZLE[p_view.swizzle_g] : MTL::TextureSwizzleGreen,
			p_view.swizzle_b != TEXTURE_SWIZZLE_IDENTITY ? COMPONENT_SWIZZLE[p_view.swizzle_b] : MTL::TextureSwizzleBlue,
			p_view.swizzle_a != TEXTURE_SWIZZLE_IDENTITY ? COMPONENT_SWIZZLE[p_view.swizzle_a] : MTL::TextureSwizzleAlpha);

	// Represents a swizzle operation that is a no-op.
	static MTL::TextureSwizzleChannels IDENTITY_SWIZZLE = MTL::TextureSwizzleChannels::Default();

	bool no_swizzle = memcmp(&IDENTITY_SWIZZLE, &swizzle, sizeof(MTL::TextureSwizzleChannels)) == 0;
	if (!no_swizzle) {
		desc->setSwizzle(swizzle);
	}

	// Usage.

	MTL::ResourceOptions options = 0;
	bool is_linear = false;
#if defined(VISIONOS_ENABLED)
	const bool supports_memoryless = true;
#else
	GODOT_CLANG_WARNING_PUSH_AND_IGNORE("-Wdeprecated-declarations")
	const bool supports_memoryless = (*device_properties).features.highestFamily >= MTL::GPUFamilyApple2 && (*device_properties).features.highestFamily < MTL::GPUFamilyMac1;
	GODOT_CLANG_WARNING_POP
#endif
	if (supports_memoryless && p_format.usage_bits & TEXTURE_USAGE_TRANSIENT_BIT) {
		options = base_hazard_tracking | MTL::ResourceStorageModeMemoryless;
		desc->setStorageMode(MTL::StorageModeMemoryless);
	} else {
		options = base_hazard_tracking | MTL::ResourceCPUCacheModeDefaultCache;
		if (p_format.usage_bits & TEXTURE_USAGE_CPU_READ_BIT) {
			options |= MTL::ResourceStorageModeShared;
			// The user has indicated they want to read from the texture on the CPU,
			// so we'll see if we can use a linear format.
			// A linear format is a texture that is backed by a buffer,
			// which allows for CPU access to the texture data via a pointer.
			is_linear = is_valid_linear(p_format);
		} else {
			options |= MTL::ResourceStorageModePrivate;
		}
	}
	desc->setResourceOptions(options);

	MTL::TextureUsage usage = desc->usage();
	if (p_format.usage_bits & TEXTURE_USAGE_SAMPLING_BIT) {
		usage |= MTL::TextureUsageShaderRead;
	}

	if (p_format.usage_bits & TEXTURE_USAGE_STORAGE_BIT) {
		usage |= MTL::TextureUsageShaderWrite;
	}

	bool can_be_attachment = flags::any(format_caps, (kMTLFmtCapsColorAtt | kMTLFmtCapsDSAtt));

	if (flags::any(p_format.usage_bits, TEXTURE_USAGE_COLOR_ATTACHMENT_BIT | TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) &&
			can_be_attachment) {
		usage |= MTL::TextureUsageRenderTarget;
	}

	if (p_format.usage_bits & TEXTURE_USAGE_INPUT_ATTACHMENT_BIT) {
		usage |= MTL::TextureUsageShaderRead;
	}

	if (p_format.usage_bits & TEXTURE_USAGE_STORAGE_ATOMIC_BIT) {
		ERR_FAIL_COND_V_MSG((format_caps & kMTLFmtCapsAtomic) == 0, RDD::TextureID(), "Atomic operations on this texture format are not supported.");
		ERR_FAIL_COND_V_MSG(!device_properties->features.supports_native_image_atomics, RDD::TextureID(), "Atomic operations on textures are not supported on this OS version. Check SUPPORTS_IMAGE_ATOMIC_32_BIT.");
		// If supports_native_image_atomics is true, this condition should always succeed, as it is set the same.
		if (__builtin_available(macOS 14.0, iOS 17.0, tvOS 17.0, *)) {
			usage |= MTL::TextureUsageShaderAtomic;
		}
	}

	if (p_format.usage_bits & TEXTURE_USAGE_VRS_ATTACHMENT_BIT) {
		ERR_FAIL_V_MSG(RDD::TextureID(), "unsupported: TEXTURE_USAGE_VRS_ATTACHMENT_BIT");
	}

	if (flags::any(p_format.usage_bits, TEXTURE_USAGE_CAN_UPDATE_BIT | TEXTURE_USAGE_CAN_COPY_TO_BIT) &&
			can_be_attachment && no_swizzle) {
		// Per MoltenVK, can be cleared as a render attachment.
		usage |= MTL::TextureUsageRenderTarget;
	}
	if (p_format.usage_bits & TEXTURE_USAGE_CAN_COPY_FROM_BIT) {
		// Covered by blits.
	}

	// Create texture views with a different component layout.
	if (!p_format.shareable_formats.is_empty()) {
		usage |= MTL::TextureUsagePixelFormatView;
	}

	desc->setUsage(usage);

	// Allocate memory.

	MTL::Texture *obj = nullptr;
	if (is_linear) {
		// Linear textures are restricted to 2D textures, a single mipmap level and a single array layer.
		MTL::PixelFormat pixel_format = desc->pixelFormat();
		size_t row_alignment = get_texel_buffer_alignment_for_format(p_format.format);
		size_t bytes_per_row = formats.getBytesPerRow(pixel_format, p_format.width);
		bytes_per_row = round_up_to_alignment(bytes_per_row, row_alignment);
		size_t bytes_per_layer = formats.getBytesPerLayer(pixel_format, bytes_per_row, p_format.height);
		size_t byte_count = bytes_per_layer * p_format.depth * p_format.array_layers;

		MTL::Buffer *buf = device->newBuffer(byte_count, options);
		obj = buf->newTexture(desc.get(), 0, bytes_per_row);
		buf->release();

		_track_resource(buf);
	} else {
		obj = device->newTexture(desc.get());
	}
	ERR_FAIL_NULL_V_MSG(obj, TextureID(), "Unable to create texture.");

	_track_resource(obj);

	return TextureID(reinterpret_cast<uint64_t>(obj));
}

RDD::TextureID RenderingDeviceDriverMetal::texture_create_from_extension(uint64_t p_native_texture, TextureType p_type, DataFormat p_format, uint32_t p_array_layers, bool p_depth_stencil, uint32_t p_mipmaps) {
	MTL::Texture *res = reinterpret_cast<MTL::Texture *>(p_native_texture);

	// If the requested format is different, we need to create a view.
	MTL::PixelFormat format = (MTL::PixelFormat)pixel_formats->getMTLPixelFormat(p_format);
	if (res->pixelFormat() != format) {
		MTL::TextureSwizzleChannels swizzle = MTL::TextureSwizzleChannels::Default();
		res = res->newTextureView(format, res->textureType(), NS::Range::Make(0, res->mipmapLevelCount()), NS::Range::Make(0, p_array_layers), swizzle);
		ERR_FAIL_NULL_V_MSG(res, TextureID(), "Unable to create texture view.");
	}

	_track_resource(res);

	return TextureID(reinterpret_cast<uint64_t>(res));
}

RDD::TextureID RenderingDeviceDriverMetal::texture_create_shared(TextureID p_original_texture, const TextureView &p_view) {
	MTL::Texture *src_texture = reinterpret_cast<MTL::Texture *>(p_original_texture.id);

	NS::UInteger slices = src_texture->arrayLength();
	if (src_texture->textureType() == MTL::TextureTypeCube) {
		// Metal expects Cube textures to have a slice count of 6.
		slices = 6;
	} else if (src_texture->textureType() == MTL::TextureTypeCubeArray) {
		// Metal expects Cube Array textures to have 6 slices per layer.
		slices *= 6;
	}

#if DEV_ENABLED
	if (src_texture->sampleCount() > 1) {
		// TODO(sgc): is it ok to create a shared texture from a multi-sample texture?
		WARN_PRINT("Is it safe to create a shared texture from multi-sample texture?");
	}
#endif

	MTL::PixelFormat format = (MTL::PixelFormat)pixel_formats->getMTLPixelFormat(p_view.format);

	static const MTL::TextureSwizzle component_swizzle[TEXTURE_SWIZZLE_MAX] = {
		static_cast<MTL::TextureSwizzle>(255), // IDENTITY
		MTL::TextureSwizzleZero,
		MTL::TextureSwizzleOne,
		MTL::TextureSwizzleRed,
		MTL::TextureSwizzleGreen,
		MTL::TextureSwizzleBlue,
		MTL::TextureSwizzleAlpha,
	};

#define SWIZZLE(C, CHAN) (p_view.swizzle_##C != TEXTURE_SWIZZLE_IDENTITY ? component_swizzle[p_view.swizzle_##C] : MTL::TextureSwizzle##CHAN)
	MTL::TextureSwizzleChannels swizzle = MTL::TextureSwizzleChannels::Make(SWIZZLE(r, Red), SWIZZLE(g, Green), SWIZZLE(b, Blue), SWIZZLE(a, Alpha));
#undef SWIZZLE
	MTL::Texture *obj = src_texture->newTextureView(format, src_texture->textureType(), NS::Range::Make(0, src_texture->mipmapLevelCount()), NS::Range::Make(0, slices), swizzle);
	ERR_FAIL_NULL_V_MSG(obj, TextureID(), "Unable to create shared texture");
	_track_resource(obj);
	return TextureID(reinterpret_cast<uint64_t>(obj));
}

RDD::TextureID RenderingDeviceDriverMetal::texture_create_shared_from_slice(TextureID p_original_texture, const TextureView &p_view, TextureSliceType p_slice_type, uint32_t p_layer, uint32_t p_layers, uint32_t p_mipmap, uint32_t p_mipmaps) {
	MTL::Texture *src_texture = reinterpret_cast<MTL::Texture *>(p_original_texture.id);

	static const MTL::TextureType VIEW_TYPES[] = {
		MTL::TextureType1D, // MTLTextureType1D
		MTL::TextureType1D, // MTLTextureType1DArray
		MTL::TextureType2D, // MTLTextureType2D
		MTL::TextureType2D, // MTLTextureType2DArray
		MTL::TextureType2D, // MTLTextureType2DMultisample
		MTL::TextureType2D, // MTLTextureTypeCube
		MTL::TextureType2D, // MTLTextureTypeCubeArray
		MTL::TextureType2D, // MTLTextureType3D
		MTL::TextureType2D, // MTLTextureType2DMultisampleArray
	};

	MTL::TextureType textureType = VIEW_TYPES[src_texture->textureType()];
	switch (p_slice_type) {
		case TEXTURE_SLICE_2D: {
			textureType = MTL::TextureType2D;
		} break;
		case TEXTURE_SLICE_3D: {
			textureType = MTL::TextureType3D;
		} break;
		case TEXTURE_SLICE_CUBEMAP: {
			textureType = MTL::TextureTypeCube;
		} break;
		case TEXTURE_SLICE_2D_ARRAY: {
			textureType = MTL::TextureType2DArray;
		} break;
		case TEXTURE_SLICE_MAX: {
			ERR_FAIL_V_MSG(TextureID(), "Invalid texture slice type");
		} break;
	}

	MTL::PixelFormat format = (MTL::PixelFormat)pixel_formats->getMTLPixelFormat(p_view.format);

	static const MTL::TextureSwizzle component_swizzle[TEXTURE_SWIZZLE_MAX] = {
		static_cast<MTL::TextureSwizzle>(255), // IDENTITY
		MTL::TextureSwizzleZero,
		MTL::TextureSwizzleOne,
		MTL::TextureSwizzleRed,
		MTL::TextureSwizzleGreen,
		MTL::TextureSwizzleBlue,
		MTL::TextureSwizzleAlpha,
	};

#define SWIZZLE(C, CHAN) (p_view.swizzle_##C != TEXTURE_SWIZZLE_IDENTITY ? component_swizzle[p_view.swizzle_##C] : MTL::TextureSwizzle##CHAN)
	MTL::TextureSwizzleChannels swizzle = MTL::TextureSwizzleChannels::Make(SWIZZLE(r, Red), SWIZZLE(g, Green), SWIZZLE(b, Blue), SWIZZLE(a, Alpha));
#undef SWIZZLE
	MTL::Texture *obj = src_texture->newTextureView(format, textureType, NS::Range::Make(p_mipmap, p_mipmaps), NS::Range::Make(p_layer, p_layers), swizzle);
	ERR_FAIL_NULL_V_MSG(obj, TextureID(), "Unable to create shared texture");
	_track_resource(obj);
	return TextureID(reinterpret_cast<uint64_t>(obj));
}

void RenderingDeviceDriverMetal::texture_free(TextureID p_texture) {
	MTL::Texture *obj = reinterpret_cast<MTL::Texture *>(p_texture.id);
	_untrack_resource(obj);
	obj->release();
}

uint64_t RenderingDeviceDriverMetal::texture_get_allocation_size(TextureID p_texture) {
	MTL::Texture *obj = reinterpret_cast<MTL::Texture *>(p_texture.id);
	return NS::Object::sendMessageSafe<NS::UInteger>(obj, _MTL_PRIVATE_SEL(allocatedSize));
}

void RenderingDeviceDriverMetal::texture_get_copyable_layout(TextureID p_texture, const TextureSubresource &p_subresource, TextureCopyableLayout *r_layout) {
	MTL::Texture *obj = reinterpret_cast<MTL::Texture *>(p_texture.id);

	PixelFormats &pf = *pixel_formats;
	DataFormat format = pf.getDataFormat(obj->pixelFormat());

	uint32_t w = MAX(1u, obj->width() >> p_subresource.mipmap);
	uint32_t h = MAX(1u, obj->height() >> p_subresource.mipmap);
	uint32_t d = MAX(1u, obj->depth() >> p_subresource.mipmap);

	uint32_t bw = 0, bh = 0;
	get_compressed_image_format_block_dimensions(format, bw, bh);

	uint32_t sbw = 0, sbh = 0;
	*r_layout = {};
	r_layout->size = get_image_format_required_size(format, w, h, d, 1, &sbw, &sbh);
	r_layout->row_pitch = r_layout->size / ((sbh / bh) * d);
}

Vector<uint8_t> RenderingDeviceDriverMetal::texture_get_data(TextureID p_texture, uint32_t p_layer) {
	MTL::Texture *obj = reinterpret_cast<MTL::Texture *>(p_texture.id);
	ERR_FAIL_COND_V_MSG(obj->storageMode() != MTL::StorageModeShared, Vector<uint8_t>(), "Texture must be created with TEXTURE_USAGE_CPU_READ_BIT set.");

	MTL::Buffer *buf = obj->buffer();
	if (buf) {
		ERR_FAIL_COND_V_MSG(p_layer > 0, Vector<uint8_t>(), "A linear texture has a single layer.");
		ERR_FAIL_COND_V_MSG(obj->mipmapLevelCount() > 1, Vector<uint8_t>(), "A linear texture has a single mipmap level.");
		Vector<uint8_t> image_data;
		image_data.resize_uninitialized(buf->length());
		memcpy(image_data.ptrw(), buf->contents(), buf->length());
		return image_data;
	}

	DataFormat tex_format = pixel_formats->getDataFormat(obj->pixelFormat());
	uint32_t tex_w = obj->width();
	uint32_t tex_h = obj->height();
	uint32_t tex_d = obj->depth();
	uint32_t tex_mipmaps = obj->mipmapLevelCount();

	// Must iteratively copy the texture data to a buffer.

	uint32_t tight_mip_size = get_image_format_required_size(tex_format, tex_w, tex_h, tex_d, tex_mipmaps);

	Vector<uint8_t> image_data;
	image_data.resize(tight_mip_size);

	uint32_t pixel_size = get_image_format_pixel_size(tex_format);
	uint32_t blockw = 0, blockh = 0;
	get_compressed_image_format_block_dimensions(tex_format, blockw, blockh);

	uint8_t *dest_ptr = image_data.ptrw();

	for (uint32_t mm_i = 0; mm_i < tex_mipmaps; mm_i++) {
		uint32_t bw = STEPIFY(tex_w, blockw);
		uint32_t bh = STEPIFY(tex_h, blockh);

		uint32_t bytes_per_row = get_compressed_image_format_pixels_shifted(tex_format, bw * pixel_size);
		uint32_t bytes_per_img = bytes_per_row * bh;
		uint32_t mip_size = bytes_per_img * tex_d;

		obj->getBytes(dest_ptr, bytes_per_row, bytes_per_img, MTL::Region(0, 0, 0, bw, bh, tex_d), mm_i, p_layer);

		dest_ptr += mip_size;

		// Next mipmap level.
		tex_w = MAX(blockw, tex_w >> 1);
		tex_h = MAX(blockh, tex_h >> 1);
		tex_d = MAX(1u, tex_d >> 1);
	}

	// Ensure that the destination pointer is at the end of the image data.
	DEV_ASSERT(dest_ptr - image_data.ptr() == image_data.size());

	return image_data;
}

uint32_t RenderingDeviceDriverMetal::texture_get_usages_supported_by_format(DataFormat p_format, bool p_cpu_readable) {
	PixelFormats &pf = *pixel_formats;
	if (pf.getMTLPixelFormat(p_format) == MTL::PixelFormatInvalid) {
		return 0;
	}

	MTLFmtCaps caps = pf.getCapabilities(p_format);

	// Everything supported by default makes an all-or-nothing check easier for the caller.
	uint32_t supported = INT64_MAX;
	supported.clear_flag(TEXTURE_USAGE_VRS_ATTACHMENT_BIT); // No VRS support for Metal.

	if (!flags::any(caps, kMTLFmtCapsColorAtt)) {
		supported.clear_flag(TEXTURE_USAGE_COLOR_ATTACHMENT_BIT);
	}
	if (!flags::any(caps, kMTLFmtCapsDSAtt)) {
		supported.clear_flag(TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
	}
	if (!flags::any(caps, kMTLFmtCapsRead)) {
		supported.clear_flag(TEXTURE_USAGE_SAMPLING_BIT);
	}
	if (!flags::any(caps, kMTLFmtCapsAtomic)) {
		supported.clear_flag(TEXTURE_USAGE_STORAGE_ATOMIC_BIT);
	}

	return supported;
}

bool RenderingDeviceDriverMetal::texture_can_make_shared_with_format(TextureID p_texture, DataFormat p_format, bool &r_raw_reinterpretation) {
	r_raw_reinterpretation = false;
	return true;
}

#pragma mark - Sampler

static const MTL::CompareFunction COMPARE_OPERATORS[RDD::COMPARE_OP_MAX] = {
	MTL::CompareFunctionNever,
	MTL::CompareFunctionLess,
	MTL::CompareFunctionEqual,
	MTL::CompareFunctionLessEqual,
	MTL::CompareFunctionGreater,
	MTL::CompareFunctionNotEqual,
	MTL::CompareFunctionGreaterEqual,
	MTL::CompareFunctionAlways,
};

static const MTL::StencilOperation STENCIL_OPERATIONS[RDD::STENCIL_OP_MAX] = {
	MTL::StencilOperationKeep,
	MTL::StencilOperationZero,
	MTL::StencilOperationReplace,
	MTL::StencilOperationIncrementClamp,
	MTL::StencilOperationDecrementClamp,
	MTL::StencilOperationInvert,
	MTL::StencilOperationIncrementWrap,
	MTL::StencilOperationDecrementWrap,
};

static const MTL::BlendFactor BLEND_FACTORS[RDD::BLEND_FACTOR_MAX] = {
	MTL::BlendFactorZero,
	MTL::BlendFactorOne,
	MTL::BlendFactorSourceColor,
	MTL::BlendFactorOneMinusSourceColor,
	MTL::BlendFactorDestinationColor,
	MTL::BlendFactorOneMinusDestinationColor,
	MTL::BlendFactorSourceAlpha,
	MTL::BlendFactorOneMinusSourceAlpha,
	MTL::BlendFactorDestinationAlpha,
	MTL::BlendFactorOneMinusDestinationAlpha,
	MTL::BlendFactorBlendColor,
	MTL::BlendFactorOneMinusBlendColor,
	MTL::BlendFactorBlendAlpha,
	MTL::BlendFactorOneMinusBlendAlpha,
	MTL::BlendFactorSourceAlphaSaturated,
	MTL::BlendFactorSource1Color,
	MTL::BlendFactorOneMinusSource1Color,
	MTL::BlendFactorSource1Alpha,
	MTL::BlendFactorOneMinusSource1Alpha,
};
static const MTL::BlendOperation BLEND_OPERATIONS[RDD::BLEND_OP_MAX] = {
	MTL::BlendOperationAdd,
	MTL::BlendOperationSubtract,
	MTL::BlendOperationReverseSubtract,
	MTL::BlendOperationMin,
	MTL::BlendOperationMax,
};

static const MTL::SamplerAddressMode ADDRESS_MODES[RDD::SAMPLER_REPEAT_MODE_MAX] = {
	MTL::SamplerAddressModeRepeat,
	MTL::SamplerAddressModeMirrorRepeat,
	MTL::SamplerAddressModeClampToEdge,
	MTL::SamplerAddressModeClampToBorderColor,
	MTL::SamplerAddressModeMirrorClampToEdge,
};

static const MTL::SamplerBorderColor SAMPLER_BORDER_COLORS[RDD::SAMPLER_BORDER_COLOR_MAX] = {
	MTL::SamplerBorderColorTransparentBlack,
	MTL::SamplerBorderColorTransparentBlack,
	MTL::SamplerBorderColorOpaqueBlack,
	MTL::SamplerBorderColorOpaqueBlack,
	MTL::SamplerBorderColorOpaqueWhite,
	MTL::SamplerBorderColorOpaqueWhite,
};

RDD::SamplerID RenderingDeviceDriverMetal::sampler_create(const SamplerState &p_state) {
	NS::SharedPtr<MTL::SamplerDescriptor> desc = NS::TransferPtr(MTL::SamplerDescriptor::alloc()->init());
	desc->setSupportArgumentBuffers(true);

	desc->setMagFilter(p_state.mag_filter == SAMPLER_FILTER_LINEAR ? MTL::SamplerMinMagFilterLinear : MTL::SamplerMinMagFilterNearest);
	desc->setMinFilter(p_state.min_filter == SAMPLER_FILTER_LINEAR ? MTL::SamplerMinMagFilterLinear : MTL::SamplerMinMagFilterNearest);
	desc->setMipFilter(p_state.mip_filter == SAMPLER_FILTER_LINEAR ? MTL::SamplerMipFilterLinear : MTL::SamplerMipFilterNearest);

	MTL::SamplerAddressMode address_u = ADDRESS_MODES[p_state.repeat_u];
	MTL::SamplerAddressMode address_v = ADDRESS_MODES[p_state.repeat_v];
	MTL::SamplerAddressMode address_w = ADDRESS_MODES[p_state.repeat_w];

	if (!device_properties->features.supports_border_color) {
		// Default to clamp to edge if border color is not supported.
		if (address_u == MTL::SamplerAddressModeClampToBorderColor) {
			address_u = MTL::SamplerAddressModeClampToEdge;
		}
		if (address_v == MTL::SamplerAddressModeClampToBorderColor) {
			address_v = MTL::SamplerAddressModeClampToEdge;
		}
		if (address_w == MTL::SamplerAddressModeClampToBorderColor) {
			address_w = MTL::SamplerAddressModeClampToEdge;
		}
	}

	desc->setSAddressMode(address_u);
	desc->setTAddressMode(address_v);
	desc->setRAddressMode(address_w);

	if (p_state.use_anisotropy) {
		desc->setMaxAnisotropy(p_state.anisotropy_max);
	}

	desc->setCompareFunction(COMPARE_OPERATORS[p_state.compare_op]);

	desc->setLodMinClamp(p_state.min_lod);
	desc->setLodMaxClamp(p_state.max_lod);

	if (device_properties->features.supports_border_color) {
		desc->setBorderColor(SAMPLER_BORDER_COLORS[p_state.border_color]);
	}

	desc->setNormalizedCoordinates(!p_state.unnormalized_uvw);

#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 260000 || __IPHONE_OS_VERSION_MAX_ALLOWED >= 260000 || __TV_OS_VERSION_MAX_ALLOWED >= 260000 || __VISION_OS_VERSION_MAX_ALLOWED >= 260000
	if (p_state.lod_bias != 0.0) {
		if (__builtin_available(macOS 26.0, iOS 26.0, tvOS 26.0, visionOS 26.0, *)) {
			desc->setLodBias(p_state.lod_bias);
		}
	}
#endif

	MTL::SamplerState *obj = device->newSamplerState(desc.get());
	ERR_FAIL_NULL_V_MSG(obj, SamplerID(), "newSamplerState failed");
	return SamplerID(reinterpret_cast<uint64_t>(obj));
}

void RenderingDeviceDriverMetal::sampler_free(SamplerID p_sampler) {
	MTL::SamplerState *obj = reinterpret_cast<MTL::SamplerState *>(p_sampler.id);
	obj->release();
}

bool RenderingDeviceDriverMetal::sampler_is_format_supported_for_filter(DataFormat p_format, SamplerFilter p_filter) {
	switch (p_filter) {
		case SAMPLER_FILTER_NEAREST:
			return true;
		case SAMPLER_FILTER_LINEAR: {
			MTLFmtCaps caps = pixel_formats->getCapabilities(p_format);
			return flags::any(caps, kMTLFmtCapsFilter);
		}
	}
}

#pragma mark - Vertex Array

RDD::VertexFormatID RenderingDeviceDriverMetal::vertex_format_create(Span<VertexAttribute> p_vertex_attribs, const VertexAttributeBindingsMap &p_vertex_bindings) {
	MTL::VertexDescriptor *desc = MTL::VertexDescriptor::vertexDescriptor();

	for (const VertexAttributeBindingsMap::KV &kv : p_vertex_bindings) {
		uint32_t idx = get_metal_buffer_index_for_vertex_attribute_binding(kv.key);
		MTL::VertexBufferLayoutDescriptor *ld = desc->layouts()->object(idx);
		if (kv.value.stride != 0) {
			ld->setStepFunction(kv.value.frequency == VERTEX_FREQUENCY_VERTEX ? MTL::VertexStepFunctionPerVertex : MTL::VertexStepFunctionPerInstance);
			ld->setStepRate(1);
			ld->setStride(kv.value.stride);
		} else {
			ld->setStepFunction(MTL::VertexStepFunctionConstant);
			ld->setStepRate(0);
			ld->setStride(0);
		}
		DEV_ASSERT(ld->stride() == desc->layouts()->object(idx)->stride());
	}

	for (const VertexAttribute &vf : p_vertex_attribs) {
		MTL::VertexAttributeDescriptor *attr = desc->attributes()->object(vf.location);
		attr->setFormat((MTL::VertexFormat)pixel_formats->getMTLVertexFormat(vf.format));
		attr->setOffset(vf.offset);
		uint32_t idx = get_metal_buffer_index_for_vertex_attribute_binding(vf.binding);
		attr->setBufferIndex(idx);
		if (vf.stride == 0) {
			// Constant attribute, so we must determine the stride to satisfy Metal API.
			uint32_t stride = desc->layouts()->object(idx)->stride();
			desc->layouts()->object(idx)->setStride(std::max(stride, vf.offset + pixel_formats->getBytesPerBlock(vf.format)));
		}
	}

	desc->retain();
	return VertexFormatID(reinterpret_cast<uint64_t>(desc));
}

void RenderingDeviceDriverMetal::vertex_format_free(VertexFormatID p_vertex_format) {
	MTL::VertexDescriptor *obj = reinterpret_cast<MTL::VertexDescriptor *>(p_vertex_format.id);
	obj->release();
}

#pragma mark - Barriers

void RenderingDeviceDriverMetal::command_pipeline_barrier(
		CommandBufferID p_cmd_buffer,
		uint32_t p_src_stages,
		uint32_t p_dst_stages,
		VectorView<MemoryAccessBarrier> p_memory_barriers,
		VectorView<BufferBarrier> p_buffer_barriers,
		VectorView<TextureBarrier> p_texture_barriers,
		VectorView<AccelerationStructureBarrier> p_acceleration_structure_barriers) {
	MDCommandBufferBase *obj = (MDCommandBufferBase *)(p_cmd_buffer.id);
	obj->pipeline_barrier(p_src_stages, p_dst_stages, p_memory_barriers, p_buffer_barriers, p_texture_barriers, p_acceleration_structure_barriers);
}

#pragma mark - Queues

RDD::CommandQueueFamilyID RenderingDeviceDriverMetal::command_queue_family_get(uint32_t p_cmd_queue_family_bits, RenderingContextDriver::SurfaceID p_surface) {
	if (p_cmd_queue_family_bits.has_flag(COMMAND_QUEUE_FAMILY_GRAPHICS_BIT) || (p_surface != 0)) {
		return CommandQueueFamilyID(COMMAND_QUEUE_FAMILY_GRAPHICS_BIT);
	} else if (p_cmd_queue_family_bits.has_flag(COMMAND_QUEUE_FAMILY_COMPUTE_BIT)) {
		return CommandQueueFamilyID(COMMAND_QUEUE_FAMILY_COMPUTE_BIT);
	} else if (p_cmd_queue_family_bits.has_flag(COMMAND_QUEUE_FAMILY_TRANSFER_BIT)) {
		return CommandQueueFamilyID(COMMAND_QUEUE_FAMILY_TRANSFER_BIT);
	} else {
		return CommandQueueFamilyID();
	}
}

#pragma mark - Command Buffers

bool RenderingDeviceDriverMetal::command_buffer_begin(CommandBufferID p_cmd_buffer) {
	MDCommandBufferBase *obj = (MDCommandBufferBase *)(p_cmd_buffer.id);
	obj->begin();
	return true;
}

bool RenderingDeviceDriverMetal::command_buffer_begin_secondary(CommandBufferID p_cmd_buffer, RenderPassID p_render_pass, uint32_t p_subpass, FramebufferID p_framebuffer) {
	ERR_FAIL_V_MSG(false, "not implemented");
}

void RenderingDeviceDriverMetal::command_buffer_end(CommandBufferID p_cmd_buffer) {
	MDCommandBufferBase *obj = (MDCommandBufferBase *)(p_cmd_buffer.id);
	obj->end();
}

void RenderingDeviceDriverMetal::command_buffer_execute_secondary(CommandBufferID p_cmd_buffer, VectorView<CommandBufferID> p_secondary_cmd_buffers) {
	ERR_FAIL_MSG("not implemented");
}

#pragma mark - Swap Chain

void RenderingDeviceDriverMetal::_swap_chain_release(SwapChain *p_swap_chain) {
	_swap_chain_release_buffers(p_swap_chain);

	if (p_swap_chain->render_pass.id != 0) {
		render_pass_free(p_swap_chain->render_pass);
		p_swap_chain->render_pass = RenderPassID();
	}
}

void RenderingDeviceDriverMetal::_swap_chain_release_buffers(SwapChain *p_swap_chain) {
}

RDD::SwapChainID RenderingDeviceDriverMetal::swap_chain_create(RenderingContextDriver::SurfaceID p_surface) {
	const RenderingContextDriverMetal::Surface *surface = (RenderingContextDriverMetal::Surface *)(p_surface);
	if (use_barriers) {
		GODOT_CLANG_WARNING_PUSH_AND_IGNORE("-Wunguarded-availability")
		add_residency_set_to_main_queue(surface->get_residency_set());
		GODOT_CLANG_WARNING_POP
	}

	SwapChain *swap_chain = memnew(SwapChain);
	swap_chain->surface = p_surface;
	return SwapChainID(swap_chain);
}

RDD::RenderPassID RenderingDeviceDriverMetal::_swap_chain_create_render_pass(RDD::DataFormat p_format) {
	// Create the render pass that will be used to draw to the swap chain's framebuffers.
	RDD::Attachment attachment;
	attachment.format = p_format;
	attachment.samples = RDD::TEXTURE_SAMPLES_1;
	attachment.load_op = RDD::ATTACHMENT_LOAD_OP_CLEAR;
	attachment.store_op = RDD::ATTACHMENT_STORE_OP_STORE;

	RDD::Subpass subpass;
	RDD::AttachmentReference color_ref;
	color_ref.attachment = 0;
	color_ref.aspect.set_flag(RDD::TEXTURE_ASPECT_COLOR_BIT);
	subpass.color_references.push_back(color_ref);

	return render_pass_create(attachment, subpass, {}, 1, RDD::AttachmentReference());
}

Error RenderingDeviceDriverMetal::swap_chain_resize(CommandQueueID p_cmd_queue, SwapChainID p_swap_chain, uint32_t p_desired_framebuffer_count) {
	DEV_ASSERT(p_cmd_queue.id != 0);
	DEV_ASSERT(p_swap_chain.id != 0);

	SwapChain *swap_chain = (SwapChain *)(p_swap_chain.id);
	RenderingContextDriverMetal::Surface *surface = (RenderingContextDriverMetal::Surface *)(swap_chain->surface);

	DataFormat new_data_format = DATA_FORMAT_MAX;
	ColorSpace new_color_space = COLOR_SPACE_MAX;
	Error err = surface->resize(p_desired_framebuffer_count, new_data_format, new_color_space);
	if (err != OK) {
		return err;
	}

	if (new_data_format != swap_chain->data_format) {
		_swap_chain_release(swap_chain);
		swap_chain->render_pass = _swap_chain_create_render_pass(new_data_format);
		if (!swap_chain->render_pass) {
			return ERR_INVALID_DATA;
		}
	}

	swap_chain->data_format = new_data_format;
	swap_chain->color_space = new_color_space;

	// Once everything's been created correctly, indicate the surface no longer needs to be resized.
	context_driver->surface_set_needs_resize(swap_chain->surface, false);

	return OK;
}

RDD::FramebufferID RenderingDeviceDriverMetal::swap_chain_acquire_framebuffer(CommandQueueID p_cmd_queue, SwapChainID p_swap_chain, bool &r_resize_required) {
	DEV_ASSERT(p_cmd_queue.id != 0);
	DEV_ASSERT(p_swap_chain.id != 0);

	SwapChain *swap_chain = (SwapChain *)(p_swap_chain.id);
	if (context_driver->surface_get_needs_resize(swap_chain->surface)) {
		r_resize_required = true;
		return FramebufferID();
	}

	RenderingContextDriverMetal::Surface *metal_surface = (RenderingContextDriverMetal::Surface *)(swap_chain->surface);
	return metal_surface->acquire_next_frame_buffer();
}

RDD::RenderPassID RenderingDeviceDriverMetal::swap_chain_get_render_pass(SwapChainID p_swap_chain) {
	const SwapChain *swap_chain = (const SwapChain *)(p_swap_chain.id);
	return swap_chain->render_pass;
}

RDD::DataFormat RenderingDeviceDriverMetal::swap_chain_get_format(SwapChainID p_swap_chain) {
	const SwapChain *swap_chain = (const SwapChain *)(p_swap_chain.id);
	return swap_chain->data_format;
}

RDD::ColorSpace RenderingDeviceDriverMetal::swap_chain_get_color_space(SwapChainID p_swap_chain) {
	const SwapChain *swap_chain = (const SwapChain *)(p_swap_chain.id);
	return swap_chain->color_space;
}

bool RenderingDeviceDriverMetal::swap_chain_get_hdr_output_supported(SwapChainID p_swap_chain) {
	// Our minimum supported version of metal requires support for EDR.
	return true;
}

void RenderingDeviceDriverMetal::swap_chain_set_max_fps(SwapChainID p_swap_chain, int p_max_fps) {
	SwapChain *swap_chain = (SwapChain *)(p_swap_chain.id);
	RenderingContextDriverMetal::Surface *metal_surface = (RenderingContextDriverMetal::Surface *)(swap_chain->surface);
	metal_surface->set_max_fps(p_max_fps);
}

void RenderingDeviceDriverMetal::swap_chain_free(SwapChainID p_swap_chain) {
	SwapChain *swap_chain = (SwapChain *)(p_swap_chain.id);
	if (use_barriers) {
		GODOT_CLANG_WARNING_PUSH_AND_IGNORE("-Wunguarded-availability")
		RenderingContextDriverMetal::Surface *surface = (RenderingContextDriverMetal::Surface *)(swap_chain->surface);
		remove_residency_set_to_main_queue(surface->get_residency_set());
		GODOT_CLANG_WARNING_POP
	}
	_swap_chain_release(swap_chain);
	memdelete(swap_chain);
}

#pragma mark - Frame buffer

RDD::FramebufferID RenderingDeviceDriverMetal::framebuffer_create(RenderPassID p_render_pass, VectorView<TextureID> p_attachments, uint32_t p_width, uint32_t p_height) {
	MDRenderPass *pass = (MDRenderPass *)(p_render_pass.id);

	Vector<MTL::Texture *> textures;
	textures.resize(p_attachments.size());

	for (uint32_t i = 0; i < p_attachments.size(); i += 1) {
		const MDAttachment &a = pass->attachments[i];
		MTL::Texture *tex = reinterpret_cast<MTL::Texture *>(p_attachments[i].id);
		if (tex == nullptr) {
#if DEV_ENABLED
			WARN_PRINT("Invalid texture for attachment " + itos(i));
#endif
		}
		if (a.samples > 1) {
			if (tex->sampleCount() != a.samples) {
#if DEV_ENABLED
				WARN_PRINT("Mismatched sample count for attachment " + itos(i) + "; expected " + itos(a.samples) + ", got " + itos(tex->sampleCount()));
#endif
			}
		}
		textures.write[i] = tex;
	}

	MDFrameBuffer *fb = memnew(MDFrameBuffer(textures, Size2i(p_width, p_height)));
	return FramebufferID(fb);
}

void RenderingDeviceDriverMetal::framebuffer_free(FramebufferID p_framebuffer) {
	MDFrameBuffer *obj = (MDFrameBuffer *)(p_framebuffer.id);
	memdelete(obj);
}

#pragma mark - Shader

void RenderingDeviceDriverMetal::shader_cache_free_entry(ShaderCacheEntry *p_entry) {
	MutexLock lock(_shader_cache_lock);
	// Only remove the map slot if it still refers to this exact entry. A concurrent
	// creation for the same hash may have replaced it, in which case that newer entry
	// (and its library) must be left untouched.
	if (ShaderCacheEntry **pentry = _shader_cache.getptr(p_entry->key); pentry != nullptr && *pentry == p_entry) {
		_shader_cache.erase(p_entry->key);
	}
	memdelete(p_entry);
}

std::optional<std::shared_ptr<MDLibrary>> RenderingDeviceDriverMetal::shader_cache_get_library(const SHA256Digest &key) {
	MutexLock lock(_shader_cache_lock);

	if (ShaderCacheEntry **p = _shader_cache.getptr(key); p != nullptr) {
		if (std::shared_ptr<MDLibrary> lib = (*p)->library.lock()) {
			return lib;
		}
		// Library was released; remove stale cache entry and recreate.
		_shader_cache.erase(key);
	}

	return std::nullopt;
}

void RenderingDeviceDriverMetal::shader_cache_set_entry(const SHA256Digest &key, ShaderCacheEntry *p_entry) {
	MutexLock lock(_shader_cache_lock);
	_shader_cache[key] = p_entry;
}

template <typename T, typename U>
struct is_layout_compatible
		: std::bool_constant<
				  sizeof(T) == sizeof(U) &&
				  alignof(T) == alignof(U) &&
				  std::is_trivially_copyable_v<T> &&
				  std::is_trivially_copyable_v<U>> {};
static_assert(is_layout_compatible<UniformInfo::Indexes, RenderingShaderContainerMetal::UniformData::Indexes>::value, "UniformInfo::Indexes layout does not match RenderingShaderContainerMetal::UniformData::Indexes layout");

API_AVAILABLE(macos(11.0), ios(14.0), tvos(14.0))
static void update_uniform_info(const RenderingShaderContainerMetal::UniformData &p_data, UniformInfo &r_ui) {
	r_ui.active_stages = p_data.active_stages;
	r_ui.dataType = static_cast<MTL::DataType>(p_data.data_type);
	memcpy(&r_ui.slot, &p_data.slot, sizeof(UniformInfo::Indexes));
	memcpy(&r_ui.arg_buffer, &p_data.arg_buffer, sizeof(UniformInfo::Indexes));
	r_ui.access = static_cast<MTL::BindingAccess>(p_data.access);
	r_ui.usage = static_cast<MTL::ResourceUsage>(p_data.usage);
	r_ui.textureType = static_cast<MTL::TextureType>(p_data.texture_type);
	r_ui.imageFormat = p_data.image_format;
	r_ui.arrayLength = p_data.array_length;
	r_ui.isMultisampled = p_data.is_multisampled;
}

RDD::ShaderID RenderingDeviceDriverMetal::shader_create_from_container(const Ref<RenderingShaderContainer> &p_shader_container, const Vector<ImmutableSampler> &p_immutable_samplers) {
	Ref<RenderingShaderContainerMetal> shader_container = p_shader_container;
	using RSCM = RenderingShaderContainerMetal;

	if (shader_container->is_invalid()) {
		WARN_PRINT("Metal shader container is invalid and will be recompiled.");
		return RDD::ShaderID();
	}

	CharString shader_name = shader_container->shader_name;
	RSCM::HeaderData &mtl_reflection_data = shader_container->mtl_reflection_data;
	Vector<RenderingShaderContainer::Shader> &shaders = shader_container->shaders;
	Vector<RSCM::StageData> &mtl_shaders = shader_container->mtl_shaders;

	// We need to regenerate the shader if the cache is moved to an incompatible device or argument buffer support differs.
	ERR_FAIL_COND_V_MSG(!device_properties->features.argument_buffers_supported() && mtl_reflection_data.uses_argument_buffers(),
			RDD::ShaderID(),
			"Shader was compiled with argument buffers enabled, but this device does not support them");

	ERR_FAIL_COND_V_MSG(device_properties->features.msl_max_version < mtl_reflection_data.msl_version,
			RDD::ShaderID(),
			"Shader was compiled for a newer version of Metal");

	MTL::GPUFamily compiled_gpu_family = static_cast<MTL::GPUFamily>(mtl_reflection_data.profile.gpu);
	ERR_FAIL_COND_V_MSG(device_properties->features.highestFamily < compiled_gpu_family,
			RDD::ShaderID(),
			"Shader was generated for a newer Apple GPU");

	NS::SharedPtr<MTL::CompileOptions> options = NS::TransferPtr(MTL::CompileOptions::alloc()->init());
	uint32_t major = mtl_reflection_data.msl_version / 10000;
	uint32_t minor = (mtl_reflection_data.msl_version / 100) % 100;
	options->setLanguageVersion(MTL::LanguageVersion((major << 0x10) + minor));
	if (__builtin_available(macOS 15.0, iOS 18.0, tvOS 18.0, visionOS 2.0, *)) {
		options->setEnableLogging(mtl_reflection_data.needs_debug_logging());
	}

	HashMap<RDD::ShaderStage, std::shared_ptr<MDLibrary>> libraries;

	PipelineType pipeline_type = PIPELINE_TYPE_RASTERIZATION;
	Vector<uint8_t> decompressed_code;
	for (uint32_t shader_index = 0; shader_index < shaders.size(); shader_index++) {
		const RenderingShaderContainer::Shader &shader = shaders[shader_index];
		const RSCM::StageData &shader_data = mtl_shaders[shader_index];

		if (shader.shader_stage == RDD::ShaderStage::SHADER_STAGE_COMPUTE) {
			pipeline_type = PIPELINE_TYPE_COMPUTE;
		}
		if (std::optional<std::shared_ptr<MDLibrary>> lib = shader_cache_get_library(shader_data.hash); lib) {
			libraries[shader.shader_stage] = std::move(*lib);
			continue;
		}

		if (shader.code_decompressed_size > 0) {
			decompressed_code.resize(shader.code_decompressed_size);
			bool decompressed = shader_container->decompress_code(shader.code_compressed_bytes.ptr(), shader.code_compressed_bytes.size(), shader.code_compression_flags, decompressed_code.ptrw(), decompressed_code.size());
			ERR_FAIL_COND_V_MSG(!decompressed, RDD::ShaderID(), vformat("Failed to decompress code on shader stage %s.", String(RDD::SHADER_STAGE_NAMES[shader.shader_stage])));
		} else {
			decompressed_code = shader.code_compressed_bytes;
		}

		ShaderCacheEntry *cd = memnew(ShaderCacheEntry(*this, shader_data.hash));
		cd->name = shader_name;
		cd->stage = shader.shader_stage;

		NS::SharedPtr<NS::String> source = NS::TransferPtr(NS::String::alloc()->init((void *)decompressed_code.ptr(), shader_data.source_size, NS::UTF8StringEncoding));

		std::shared_ptr<MDLibrary> library;
		if (shader_data.library_size > 0) {
			ERR_FAIL_COND_V_MSG(mtl_reflection_data.os_min_version > device_properties->os_version,
					RDD::ShaderID(),
					"Metal shader binary was generated for a newer target OS");
			dispatch_data_t binary = dispatch_data_create(decompressed_code.ptr() + shader_data.source_size, shader_data.library_size, dispatch_get_main_queue(), DISPATCH_DATA_DESTRUCTOR_DEFAULT);
			library = MDLibrary::create(cd, device,
#if DEV_ENABLED
					source.get(),
#endif
					binary);
		} else {
			options->setPreserveInvariance(shader_data.is_position_invariant);
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 150000 || __IPHONE_OS_VERSION_MIN_REQUIRED >= 180000 || __TV_OS_VERSION_MIN_REQUIRED >= 180000 || defined(VISIONOS_ENABLED)
			options->setMathMode(MTL::MathModeFast);
#else
			options->setFastMathEnabled(true);
#endif
			library = MDLibrary::create(cd, device, source.get(), options.get(), _shader_load_strategy);
		}

		shader_cache_set_entry(shader_data.hash, cd);
		libraries[shader.shader_stage] = library;
	}

	ShaderReflection refl = shader_container->get_shader_reflection();
	RSCM::MetalShaderReflection mtl_refl = shader_container->get_metal_shader_reflection();

	Vector<UniformSet> uniform_sets;
	uint32_t uniform_sets_count = mtl_refl.uniform_sets.size();
	uniform_sets.resize(uniform_sets_count);

	DynamicOffsetLayout dynamic_offset_layout;
	uint8_t dynamic_offset = 0;

	// Create sets.
	for (uint32_t i = 0; i < uniform_sets_count; i++) {
		UniformSet &set = uniform_sets.write[i];
		const Vector<ShaderUniform> &refl_set = refl.uniform_sets.ptr()[i];
		const Vector<RSCM::UniformData> &mtl_set = mtl_refl.uniform_sets.ptr()[i];
		uint32_t set_size = mtl_set.size();
		set.uniforms.resize(set_size);

		uint8_t dynamic_count = 0;

		LocalVector<UniformInfo>::Iterator iter = set.uniforms.begin();
		for (uint32_t j = 0; j < set_size; j++) {
			const ShaderUniform &uniform = refl_set.ptr()[j];
			const RSCM::UniformData &bind = mtl_set.ptr()[j];

			switch (uniform.type) {
				case UNIFORM_TYPE_STORAGE_BUFFER_DYNAMIC:
				case UNIFORM_TYPE_UNIFORM_BUFFER_DYNAMIC: {
					set.dynamic_uniforms.push_back(j);
					dynamic_count++;
				} break;
				default: {
				} break;
			}

			UniformInfo &ui = *iter;
			++iter;
			update_uniform_info(bind, ui);
			ui.binding = uniform.binding;

			if (ui.arg_buffer.texture == UINT32_MAX && ui.arg_buffer.buffer == UINT32_MAX && ui.arg_buffer.sampler == UINT32_MAX) {
				// No bindings.
				continue;
			}
#define VAL(x) (x == UINT32_MAX ? 0 : x)
			uint32_t max = std::max({ VAL(ui.arg_buffer.texture), VAL(ui.arg_buffer.buffer), VAL(ui.arg_buffer.sampler) });
			max += ui.arrayLength > 0 ? ui.arrayLength - 1 : 0;
			set.buffer_size = std::max(set.buffer_size, (max + 1) * (uint32_t)sizeof(uint64_t));
#undef VAL
		}

		if (dynamic_count > 0) {
			dynamic_offset_layout.set_offset_count(i, dynamic_offset, dynamic_count);
			dynamic_offset += dynamic_count;
		}
	}

	MDShader *shader = nullptr;
	if (pipeline_type == PIPELINE_TYPE_COMPUTE) {
		MDComputeShader *cs = new MDComputeShader(
				shader_name,
				uniform_sets,
				mtl_reflection_data.uses_argument_buffers(),
				libraries[RDD::ShaderStage::SHADER_STAGE_COMPUTE]);

		cs->local = MTL::Size(refl.compute_local_size[0], refl.compute_local_size[1], refl.compute_local_size[2]);
		shader = cs;
	} else {
		MDRenderShader *rs = new MDRenderShader(
				shader_name,
				uniform_sets,
				mtl_reflection_data.needs_view_mask_buffer(),
				mtl_reflection_data.uses_argument_buffers(),
				libraries[RDD::ShaderStage::SHADER_STAGE_VERTEX],
				libraries[RDD::ShaderStage::SHADER_STAGE_FRAGMENT]);
		shader = rs;
	}

	shader->push_constants.stages = refl.push_constant_stages;
	shader->push_constants.size = refl.push_constant_size;
	shader->push_constants.binding = mtl_reflection_data.push_constant_binding;
	shader->dynamic_offset_layout = dynamic_offset_layout;

	return RDD::ShaderID(shader);
}

void RenderingDeviceDriverMetal::shader_free(ShaderID p_shader) {
	MDShader *obj = (MDShader *)p_shader.id;
	delete obj;
}

void RenderingDeviceDriverMetal::shader_destroy_modules(ShaderID p_shader) {
	// TODO.
}

/*********************/
/**** UNIFORM SET ****/
/*********************/

RDD::UniformSetID RenderingDeviceDriverMetal::uniform_set_create(VectorView<BoundUniform> p_uniforms, ShaderID p_shader, uint32_t p_set_index, int p_linear_pool_index) {
	//p_linear_pool_index = -1; // TODO:? Linear pools not implemented or not supported by API backend.

	MDShader *shader = (MDShader *)(p_shader.id);
	ERR_FAIL_INDEX_V_MSG(p_set_index, shader->sets.size(), UniformSetID(), "Set index out of range");
	const UniformSet &shader_set = shader->sets.get(p_set_index);
	MDUniformSet *set = memnew(MDUniformSet);
	// Determine if there are any dynamic uniforms in this set.
	bool is_dynamic = !shader_set.dynamic_uniforms.is_empty();

	Vector<uint8_t> arg_buffer_data;

	if (device_properties->features.argument_buffers_supported()) {
		arg_buffer_data.resize(shader_set.buffer_size);

		// If argument buffers are enabled, we have already verified availability, so we can skip the runtime check.
		GODOT_CLANG_WARNING_PUSH_AND_IGNORE("-Wunguarded-availability-new")
		uint64_t *ptr = (uint64_t *)arg_buffer_data.ptrw();

		HashMap<MTL::Resource *, StageResourceUsage, HashMapHasherDefault> bound_resources;
		auto add_usage = [&bound_resources](MTL::Resource *res, uint32_t stage, MTL::ResourceUsage usage) {
			StageResourceUsage *sru = bound_resources.getptr(res);
			if (sru == nullptr) {
				sru = &bound_resources.insert(res, ResourceUnused)->value;
			}
			if (stage.has_flag(RDD::SHADER_STAGE_VERTEX_BIT)) {
				*sru |= stage_resource_usage(RDD::SHADER_STAGE_VERTEX, usage);
			}
			if (stage.has_flag(RDD::SHADER_STAGE_FRAGMENT_BIT)) {
				*sru |= stage_resource_usage(RDD::SHADER_STAGE_FRAGMENT, usage);
			}
			if (stage.has_flag(RDD::SHADER_STAGE_COMPUTE_BIT)) {
				*sru |= stage_resource_usage(RDD::SHADER_STAGE_COMPUTE, usage);
			}
		};
#define ADD_USAGE(res, stage, usage) \
	if (!use_barriers) { \
		add_usage(res, stage, usage); \
	}

		// Ensure the argument buffer exists for this set as some shader pipelines may
		// have been generated with argument buffers enabled.
		for (uint32_t i = 0; i < p_uniforms.size(); i += 1) {
			const BoundUniform &uniform = p_uniforms[i];
			const UniformInfo &ui = shader_set.uniforms[i];
			const UniformInfo::Indexes &idx = ui.arg_buffer;

			switch (uniform.type) {
				case UNIFORM_TYPE_SAMPLER: {
					size_t count = uniform.ids.size();
					for (size_t j = 0; j < count; j += 1) {
						MTL::SamplerState *sampler = reinterpret_cast<MTL::SamplerState *>(uniform.ids[j].id);
						*(MTL::ResourceID *)(ptr + idx.sampler + j) = sampler->gpuResourceID();
					}
				} break;
				case UNIFORM_TYPE_SAMPLER_WITH_TEXTURE: {
					uint32_t count = uniform.ids.size() / 2;
					for (uint32_t j = 0; j < count; j += 1) {
						MTL::SamplerState *sampler = reinterpret_cast<MTL::SamplerState *>(uniform.ids[j * 2 + 0].id);
						MTL::Texture *texture = reinterpret_cast<MTL::Texture *>(uniform.ids[j * 2 + 1].id);
						*(MTL::ResourceID *)(ptr + idx.texture + j) = texture->gpuResourceID();
						*(MTL::ResourceID *)(ptr + idx.sampler + j) = sampler->gpuResourceID();

						ADD_USAGE(texture, ui.active_stages, ui.usage);
					}
				} break;
				case UNIFORM_TYPE_TEXTURE: {
					size_t count = uniform.ids.size();
					for (size_t j = 0; j < count; j += 1) {
						MTL::Texture *texture = reinterpret_cast<MTL::Texture *>(uniform.ids[j].id);
						*(MTL::ResourceID *)(ptr + idx.texture + j) = texture->gpuResourceID();

						ADD_USAGE(texture, ui.active_stages, ui.usage);
					}
				} break;
				case UNIFORM_TYPE_IMAGE: {
					size_t count = uniform.ids.size();
					for (size_t j = 0; j < count; j += 1) {
						MTL::Texture *texture = reinterpret_cast<MTL::Texture *>(uniform.ids[j].id);
						*(MTL::ResourceID *)(ptr + idx.texture + j) = texture->gpuResourceID();
						ADD_USAGE(texture, ui.active_stages, ui.usage);

						if (idx.buffer != UINT32_MAX) {
							// Emulated atomic image access.
							MTL::Texture *parent = texture->parentTexture();
							MTL::Buffer *buffer = (parent ? parent : texture)->buffer();
							*(MTLGPUAddress *)(ptr + idx.buffer + j) = buffer->gpuAddress();

							ADD_USAGE(buffer, ui.active_stages, ui.usage);
						}
					}
				} break;
				case UNIFORM_TYPE_TEXTURE_BUFFER: {
					ERR_PRINT("not implemented: UNIFORM_TYPE_TEXTURE_BUFFER");
				} break;
				case UNIFORM_TYPE_SAMPLER_WITH_TEXTURE_BUFFER: {
					ERR_PRINT("not implemented: UNIFORM_TYPE_SAMPLER_WITH_TEXTURE_BUFFER");
				} break;
				case UNIFORM_TYPE_IMAGE_BUFFER: {
					CRASH_NOW_MSG("not implemented: UNIFORM_TYPE_IMAGE_BUFFER");
				} break;
				case UNIFORM_TYPE_STORAGE_BUFFER:
				case UNIFORM_TYPE_UNIFORM_BUFFER: {
					const BufferInfo *buffer = (const BufferInfo *)uniform.ids[0].id;
					*(MTLGPUAddress *)(ptr + idx.buffer) = buffer->metal_buffer.get()->gpuAddress();

					ADD_USAGE(buffer->metal_buffer.get(), ui.active_stages, ui.usage);
				} break;
				case UNIFORM_TYPE_INPUT_ATTACHMENT: {
					size_t count = uniform.ids.size();
					for (size_t j = 0; j < count; j += 1) {
						MTL::Texture *texture = reinterpret_cast<MTL::Texture *>(uniform.ids[j].id);
						*(MTL::ResourceID *)(ptr + idx.texture + j) = texture->gpuResourceID();

						ADD_USAGE(texture, ui.active_stages, ui.usage);
					}
				} break;
				case UNIFORM_TYPE_UNIFORM_BUFFER_DYNAMIC:
				case UNIFORM_TYPE_STORAGE_BUFFER_DYNAMIC: {
					// Encode the base GPU address (frame 0); it will be updated at bind time.
					const MetalBufferDynamicInfo *buffer = (const MetalBufferDynamicInfo *)uniform.ids[0].id;
					*(MTLGPUAddress *)(ptr + idx.buffer) = buffer->metal_buffer.get()->gpuAddress();

					ADD_USAGE(buffer->metal_buffer.get(), ui.active_stages, ui.usage);
				} break;
				default: {
					DEV_ASSERT(false);
				}
			}
		}

#undef ADD_USAGE

		if (!use_barriers) {
			for (const KeyValue<MTL::Resource *, StageResourceUsage> &keyval : bound_resources) {
				ResourceVector *resources = set->usage_to_resources.getptr(keyval.value);
				if (resources == nullptr) {
					resources = &set->usage_to_resources.insert(keyval.value, ResourceVector())->value;
				}
				int64_t pos = resources->span().bisect(keyval.key, true);
				if (pos == resources->size() || (*resources)[pos] != keyval.key) {
					resources->insert(pos, keyval.key);
				}
			}
		}

		if (!is_dynamic) {
			set->arg_buffer = NS::TransferPtr(device->newBuffer(shader_set.buffer_size, base_hazard_tracking | MTL::ResourceStorageModePrivate));
#if DEV_ENABLED
			char label[64];
			snprintf(label, sizeof(label), "Uniform Set %u", p_set_index);
			set->arg_buffer->setLabel(NS::String::string(label, NS::UTF8StringEncoding));
#endif
			_track_resource(set->arg_buffer.get());
			_copy_queue_copy_to_buffer(arg_buffer_data, set->arg_buffer.get());
		} else {
			// Store the arg buffer data for dynamic uniform sets.
			// It will be copied and updated at bind time.
			set->arg_buffer_data = arg_buffer_data;
		}

		GODOT_CLANG_WARNING_POP
	}
	Vector<BoundUniform> bound_uniforms;
	bound_uniforms.resize(p_uniforms.size());
	for (uint32_t i = 0; i < p_uniforms.size(); i += 1) {
		bound_uniforms.write[i] = p_uniforms[i];
	}
	set->uniforms = bound_uniforms;

	return UniformSetID(set);
}

void RenderingDeviceDriverMetal::uniform_set_free(UniformSetID p_uniform_set) {
	MDUniformSet *obj = (MDUniformSet *)p_uniform_set.id;
	if (obj->arg_buffer) {
		_untrack_resource(obj->arg_buffer.get());
	}
	memdelete(obj);
}

uint32_t RenderingDeviceDriverMetal::uniform_sets_get_dynamic_offsets(VectorView<UniformSetID> p_uniform_sets, ShaderID p_shader, uint32_t p_first_set_index, uint32_t p_set_count) const {
	const MDShader *shader = (const MDShader *)p_shader.id;
	const DynamicOffsetLayout layout = shader->dynamic_offset_layout;

	if (layout.is_empty()) {
		return 0u;
	}

	uint32_t mask = 0u;

	for (uint32_t i = 0; i < p_set_count; i++) {
		const uint32_t index = p_first_set_index + i;
		uint32_t shift = layout.get_offset_index_shift(index);
		const uint32_t count = layout.get_count(index);
		DEV_ASSERT(shader->sets[index].dynamic_uniforms.size() == count);
		if (count == 0) {
			continue;
		}

		const MDUniformSet *usi = (const MDUniformSet *)p_uniform_sets[i].id;
		for (uint32_t uniform_index : shader->sets[index].dynamic_uniforms) {
			const RDD::BoundUniform &uniform = usi->uniforms[uniform_index];
			DEV_ASSERT(uniform.is_dynamic());
			const MetalBufferDynamicInfo *buf_info = (const MetalBufferDynamicInfo *)uniform.ids[0].id;
			mask |= buf_info->frame_index() << shift;
			shift += 4u;
		}
	}

	return mask;
}

void RenderingDeviceDriverMetal::command_uniform_set_prepare_for_use(CommandBufferID p_cmd_buffer, UniformSetID p_uniform_set, ShaderID p_shader, uint32_t p_set_index) {
}

#pragma mark - Transfer

void RenderingDeviceDriverMetal::command_clear_buffer(CommandBufferID p_cmd_buffer, BufferID p_buffer, uint64_t p_offset, uint64_t p_size) {
	MDCommandBufferBase *cmd = (MDCommandBufferBase *)(p_cmd_buffer.id);
	cmd->clear_buffer(p_buffer, p_offset, p_size);
}

void RenderingDeviceDriverMetal::command_copy_buffer(CommandBufferID p_cmd_buffer, BufferID p_src_buffer, BufferID p_dst_buffer, VectorView<BufferCopyRegion> p_regions) {
	MDCommandBufferBase *cmd = (MDCommandBufferBase *)(p_cmd_buffer.id);
	cmd->copy_buffer(p_src_buffer, p_dst_buffer, p_regions);
}

void RenderingDeviceDriverMetal::command_copy_texture(CommandBufferID p_cmd_buffer, TextureID p_src_texture, TextureLayout p_src_texture_layout, TextureID p_dst_texture, TextureLayout p_dst_texture_layout, VectorView<TextureCopyRegion> p_regions) {
	MDCommandBufferBase *cmd = (MDCommandBufferBase *)(p_cmd_buffer.id);
	cmd->copy_texture(p_src_texture, p_dst_texture, p_regions);
}

void RenderingDeviceDriverMetal::command_resolve_texture(CommandBufferID p_cmd_buffer, TextureID p_src_texture, TextureLayout p_src_texture_layout, uint32_t p_src_layer, uint32_t p_src_mipmap, TextureID p_dst_texture, TextureLayout p_dst_texture_layout, uint32_t p_dst_layer, uint32_t p_dst_mipmap) {
	MDCommandBufferBase *cb = (MDCommandBufferBase *)(p_cmd_buffer.id);
	cb->resolve_texture(p_src_texture, p_src_texture_layout, p_src_layer, p_src_mipmap, p_dst_texture, p_dst_texture_layout, p_dst_layer, p_dst_mipmap);
}

void RenderingDeviceDriverMetal::command_clear_color_texture(CommandBufferID p_cmd_buffer, TextureID p_texture, TextureLayout p_texture_layout, const Color &p_color, const TextureSubresourceRange &p_subresources) {
	MDCommandBufferBase *cb = (MDCommandBufferBase *)(p_cmd_buffer.id);
	cb->clear_color_texture(p_texture, p_texture_layout, p_color, p_subresources);
}

void RenderingDeviceDriverMetal::command_clear_depth_stencil_texture(CommandBufferID p_cmd_buffer, TextureID p_texture, TextureLayout p_texture_layout, float p_depth, uint8_t p_stencil, const TextureSubresourceRange &p_subresources) {
	MDCommandBufferBase *cb = (MDCommandBufferBase *)(p_cmd_buffer.id);
	cb->clear_depth_stencil_texture(p_texture, p_texture_layout, p_depth, p_stencil, p_subresources);
}

void RenderingDeviceDriverMetal::command_copy_buffer_to_texture(CommandBufferID p_cmd_buffer, BufferID p_src_buffer, TextureID p_dst_texture, TextureLayout p_dst_texture_layout, VectorView<BufferTextureCopyRegion> p_regions) {
	MDCommandBufferBase *cmd = (MDCommandBufferBase *)(p_cmd_buffer.id);
	cmd->copy_buffer_to_texture(p_src_buffer, p_dst_texture, p_regions);
}

void RenderingDeviceDriverMetal::command_copy_texture_to_buffer(CommandBufferID p_cmd_buffer, TextureID p_src_texture, TextureLayout p_src_texture_layout, BufferID p_dst_buffer, VectorView<BufferTextureCopyRegion> p_regions) {
	MDCommandBufferBase *cmd = (MDCommandBufferBase *)(p_cmd_buffer.id);
	cmd->copy_texture_to_buffer(p_src_texture, p_dst_buffer, p_regions);
}

#pragma mark - Pipeline

void RenderingDeviceDriverMetal::pipeline_free(PipelineID p_pipeline_id) {
	MDPipeline *obj = (MDPipeline *)(p_pipeline_id.id);
	delete obj;
}

// ----- BINDING -----

void RenderingDeviceDriverMetal::command_bind_push_constants(CommandBufferID p_cmd_buffer, ShaderID p_shader, uint32_t p_dst_first_index, VectorView<uint32_t> p_data) {
	MDCommandBufferBase *cb = (MDCommandBufferBase *)(p_cmd_buffer.id);
	cb->encode_push_constant_data(p_shader, p_data);
}

// ----- CACHE -----

String RenderingDeviceDriverMetal::_pipeline_get_cache_path() const {
	String path = OS::get_user_data_dir() + "/metal/pipelines";
	path += "." + context_device.name.validate_filename().replace_char(' ', '_').to_lower();
	if (Engine::is_editor_hint()) {
		path += ".editor";
	}
	path += ".cache";

	return path;
}

bool RenderingDeviceDriverMetal::pipeline_cache_create(const Vector<uint8_t> &p_data) {
	return false;
	// TODO: Convert to metal-cpp when pipeline caching is re-enabled
	// CharString path = _pipeline_get_cache_path().utf8();
	// NS::SharedPtr<MTL::BinaryArchiveDescriptor> desc = NS::TransferPtr(MTL::BinaryArchiveDescriptor::alloc()->init());
	// NS::Error *error = nullptr;
	// archive = NS::TransferPtr(device->newBinaryArchive(desc.get(), &error));
	// return true;
}

void RenderingDeviceDriverMetal::pipeline_cache_free() {
	archive = nullptr;
}

size_t RenderingDeviceDriverMetal::pipeline_cache_query_size() {
	return archive_count * 1024;
}

Vector<uint8_t> RenderingDeviceDriverMetal::pipeline_cache_serialize() {
	if (!archive) {
		return Vector<uint8_t>();
	}

	// TODO: Convert to metal-cpp when pipeline caching is re-enabled
	// CharString path = _pipeline_get_cache_path().utf8();
	// NS::URL *target = NS::URL::fileURLWithPath(NS::String::string(path.get_data(), NS::UTF8StringEncoding));
	// NS::Error *error = nullptr;
	// if (archive->serializeToURL(target, &error)) {
	//     return Vector<uint8_t>();
	// } else {
	//     print_line(error->localizedDescription()->utf8String());
	//     return Vector<uint8_t>();
	// }
	return Vector<uint8_t>();
}

#pragma mark - Rendering

// ----- SUBPASS -----

RDD::RenderPassID RenderingDeviceDriverMetal::render_pass_create(VectorView<Attachment> p_attachments, VectorView<Subpass> p_subpasses, VectorView<SubpassDependency> p_subpass_dependencies, uint32_t p_view_count, AttachmentReference p_fragment_density_map_attachment) {
	PixelFormats &pf = *pixel_formats;

	size_t subpass_count = p_subpasses.size();

	Vector<MDSubpass> subpasses;
	subpasses.resize(subpass_count);
	for (uint32_t i = 0; i < subpass_count; i++) {
		MDSubpass &subpass = subpasses.write[i];
		subpass.subpass_index = i;
		subpass.view_count = p_view_count;
		subpass.input_references = p_subpasses[i].input_references;
		subpass.color_references = p_subpasses[i].color_references;
		subpass.depth_stencil_reference = p_subpasses[i].depth_stencil_reference;
		subpass.resolve_references = p_subpasses[i].resolve_references;
		subpass.depth_resolve_reference = p_subpasses[i].depth_resolve_reference;
	}

	static const MTL::LoadAction LOAD_ACTIONS[] = {
		MTL::LoadActionLoad, // ATTACHMENT_LOAD_OP_LOAD
		MTL::LoadActionClear, // ATTACHMENT_LOAD_OP_CLEAR
		MTL::LoadActionDontCare, // ATTACHMENT_LOAD_OP_DONT_CARE
	};

	static const MTL::StoreAction STORE_ACTIONS[] = {
		MTL::StoreActionStore, // ATTACHMENT_STORE_OP_STORE
		MTL::StoreActionDontCare, // ATTACHMENT_STORE_OP_DONT_CARE
	};

	Vector<MDAttachment> attachments;
	attachments.resize(p_attachments.size());

	for (uint32_t i = 0; i < p_attachments.size(); i++) {
		const Attachment &a = p_attachments[i];
		MDAttachment &mda = attachments.write[i];
		MTL::PixelFormat format = pf.getMTLPixelFormat(a.format);
		mda.format = format;
		if (a.samples > TEXTURE_SAMPLES_1) {
			mda.samples = (*device_properties).find_nearest_supported_sample_count(a.samples);
		}
		mda.loadAction = LOAD_ACTIONS[a.load_op];
		mda.storeAction = STORE_ACTIONS[a.store_op];
		bool is_depth = pf.isDepthFormat(format);
		if (is_depth) {
			mda.type |= MDAttachmentType::Depth;
		}
		bool is_stencil = pf.isStencilFormat(format);
		if (is_stencil) {
			mda.type |= MDAttachmentType::Stencil;
			mda.stencilLoadAction = LOAD_ACTIONS[a.stencil_load_op];
			mda.stencilStoreAction = STORE_ACTIONS[a.stencil_store_op];
		}
		if (!is_depth && !is_stencil) {
			mda.type |= MDAttachmentType::Color;
		}
	}
	MDRenderPass *obj = memnew(MDRenderPass(attachments, subpasses));
	return RenderPassID(obj);
}

void RenderingDeviceDriverMetal::render_pass_free(RenderPassID p_render_pass) {
	MDRenderPass *obj = (MDRenderPass *)(p_render_pass.id);
	memdelete(obj);
}

// ----- COMMANDS -----

void RenderingDeviceDriverMetal::command_begin_render_pass(CommandBufferID p_cmd_buffer, RenderPassID p_render_pass, FramebufferID p_framebuffer, CommandBufferType p_cmd_buffer_type, const Rect2i &p_rect, VectorView<RenderPassClearValue> p_clear_values) {
	MDCommandBufferBase *cb = (MDCommandBufferBase *)(p_cmd_buffer.id);
	cb->render_begin_pass(p_render_pass, p_framebuffer, p_cmd_buffer_type, p_rect, p_clear_values);
}

void RenderingDeviceDriverMetal::command_end_render_pass(CommandBufferID p_cmd_buffer) {
	MDCommandBufferBase *cb = (MDCommandBufferBase *)(p_cmd_buffer.id);
	cb->render_end_pass();
}

void RenderingDeviceDriverMetal::command_next_render_subpass(CommandBufferID p_cmd_buffer, CommandBufferType p_cmd_buffer_type) {
	MDCommandBufferBase *cb = (MDCommandBufferBase *)(p_cmd_buffer.id);
	cb->render_next_subpass();
}

void RenderingDeviceDriverMetal::command_render_set_viewport(CommandBufferID p_cmd_buffer, VectorView<Rect2i> p_viewports) {
	MDCommandBufferBase *cb = (MDCommandBufferBase *)(p_cmd_buffer.id);
	cb->render_set_viewport(p_viewports);
}

void RenderingDeviceDriverMetal::command_render_set_scissor(CommandBufferID p_cmd_buffer, VectorView<Rect2i> p_scissors) {
	MDCommandBufferBase *cb = (MDCommandBufferBase *)(p_cmd_buffer.id);
	cb->render_set_scissor(p_scissors);
}

void RenderingDeviceDriverMetal::command_render_clear_attachments(CommandBufferID p_cmd_buffer, VectorView<AttachmentClear> p_attachment_clears, VectorView<Rect2i> p_rects) {
	MDCommandBufferBase *cb = (MDCommandBufferBase *)(p_cmd_buffer.id);
	cb->render_clear_attachments(p_attachment_clears, p_rects);
}

void RenderingDeviceDriverMetal::command_bind_render_pipeline(CommandBufferID p_cmd_buffer, PipelineID p_pipeline) {
	MDCommandBufferBase *cb = (MDCommandBufferBase *)(p_cmd_buffer.id);
	cb->bind_pipeline(p_pipeline);
}

void RenderingDeviceDriverMetal::command_bind_render_uniform_sets(CommandBufferID p_cmd_buffer, VectorView<UniformSetID> p_uniform_sets, ShaderID p_shader, uint32_t p_first_set_index, uint32_t p_set_count, uint32_t p_dynamic_offsets) {
	MDCommandBufferBase *cb = (MDCommandBufferBase *)(p_cmd_buffer.id);
	cb->render_bind_uniform_sets(p_uniform_sets, p_shader, p_first_set_index, p_set_count, p_dynamic_offsets);
}

void RenderingDeviceDriverMetal::command_render_draw(CommandBufferID p_cmd_buffer, uint32_t p_vertex_count, uint32_t p_instance_count, uint32_t p_base_vertex, uint32_t p_first_instance) {
	MDCommandBufferBase *cb = (MDCommandBufferBase *)(p_cmd_buffer.id);
	cb->render_draw(p_vertex_count, p_instance_count, p_base_vertex, p_first_instance);
}

void RenderingDeviceDriverMetal::command_render_draw_indexed(CommandBufferID p_cmd_buffer, uint32_t p_index_count, uint32_t p_instance_count, uint32_t p_first_index, int32_t p_vertex_offset, uint32_t p_first_instance) {
	MDCommandBufferBase *cb = (MDCommandBufferBase *)(p_cmd_buffer.id);
	cb->render_draw_indexed(p_index_count, p_instance_count, p_first_index, p_vertex_offset, p_first_instance);
}

void RenderingDeviceDriverMetal::command_render_draw_indexed_indirect(CommandBufferID p_cmd_buffer, BufferID p_indirect_buffer, uint64_t p_offset, uint32_t p_draw_count, uint32_t p_stride) {
	MDCommandBufferBase *cb = (MDCommandBufferBase *)(p_cmd_buffer.id);
	cb->render_draw_indexed_indirect(p_indirect_buffer, p_offset, p_draw_count, p_stride);
}

void RenderingDeviceDriverMetal::command_render_draw_indexed_indirect_count(CommandBufferID p_cmd_buffer, BufferID p_indirect_buffer, uint64_t p_offset, BufferID p_count_buffer, uint64_t p_count_buffer_offset, uint32_t p_max_draw_count, uint32_t p_stride) {
	MDCommandBufferBase *cb = (MDCommandBufferBase *)(p_cmd_buffer.id);
	cb->render_draw_indexed_indirect_count(p_indirect_buffer, p_offset, p_count_buffer, p_count_buffer_offset, p_max_draw_count, p_stride);
}

void RenderingDeviceDriverMetal::command_render_draw_indirect(CommandBufferID p_cmd_buffer, BufferID p_indirect_buffer, uint64_t p_offset, uint32_t p_draw_count, uint32_t p_stride) {
	MDCommandBufferBase *cb = (MDCommandBufferBase *)(p_cmd_buffer.id);
	cb->render_draw_indirect(p_indirect_buffer, p_offset, p_draw_count, p_stride);
}

void RenderingDeviceDriverMetal::command_render_draw_indirect_count(CommandBufferID p_cmd_buffer, BufferID p_indirect_buffer, uint64_t p_offset, BufferID p_count_buffer, uint64_t p_count_buffer_offset, uint32_t p_max_draw_count, uint32_t p_stride) {
	MDCommandBufferBase *cb = (MDCommandBufferBase *)(p_cmd_buffer.id);
	cb->render_draw_indirect_count(p_indirect_buffer, p_offset, p_count_buffer, p_count_buffer_offset, p_max_draw_count, p_stride);
}

void RenderingDeviceDriverMetal::command_render_bind_vertex_buffers(CommandBufferID p_cmd_buffer, uint32_t p_binding_count, const BufferID *p_buffers, const uint64_t *p_offsets, uint64_t p_dynamic_offsets) {
	MDCommandBufferBase *cb = (MDCommandBufferBase *)(p_cmd_buffer.id);
	cb->render_bind_vertex_buffers(p_binding_count, p_buffers, p_offsets, p_dynamic_offsets);
}

void RenderingDeviceDriverMetal::command_render_bind_index_buffer(CommandBufferID p_cmd_buffer, BufferID p_buffer, IndexBufferFormat p_format, uint64_t p_offset) {
	MDCommandBufferBase *cb = (MDCommandBufferBase *)(p_cmd_buffer.id);
	cb->render_bind_index_buffer(p_buffer, p_format, p_offset);
}

void RenderingDeviceDriverMetal::command_render_set_blend_constants(CommandBufferID p_cmd_buffer, const Color &p_constants) {
	MDCommandBufferBase *cb = (MDCommandBufferBase *)(p_cmd_buffer.id);
	cb->render_set_blend_constants(p_constants);
}

void RenderingDeviceDriverMetal::command_render_set_line_width(CommandBufferID p_cmd_buffer, float p_width) {
	if (!Math::is_equal_approx(p_width, 1.0f)) {
		ERR_FAIL_MSG("Setting line widths other than 1.0 is not supported by the Metal rendering driver.");
	}
}

// ----- PIPELINE -----

RenderingDeviceDriverMetal::Result<NS::SharedPtr<MTL::Function>> RenderingDeviceDriverMetal::_create_function(MDLibrary *p_library, NS::String *p_name, VectorView<PipelineSpecializationConstant> &p_specialization_constants) {
	MTL::Library *library = p_library->get_library();
	if (!library) {
		ERR_FAIL_V_MSG(ERR_CANT_CREATE, "Failed to compile Metal library");
	}

	MTL::Function *function = library->newFunction(p_name);
	ERR_FAIL_NULL_V_MSG(function, ERR_CANT_CREATE, "No function named main0");

	NS::Dictionary *constants_dict = function->functionConstantsDictionary();
	if (constants_dict->count() == 0) {
		return NS::TransferPtr(function);
	}

	LocalVector<MTL::FunctionConstant *> constants;
	NS::Enumerator<NS::String> *keys = constants_dict->keyEnumerator<NS::String>();
	while (NS::String *key = keys->nextObject()) {
		constants.push_back(constants_dict->object<MTL::FunctionConstant>(key));
	}

	// Check if already sorted by index.
	bool is_sorted = true;
	for (NS::UInteger i = 1; i < constants.size(); i++) {
		MTL::FunctionConstant *prev = constants[i - 1];
		MTL::FunctionConstant *curr = constants[i];
		if (prev->index() > curr->index()) {
			is_sorted = false;
			break;
		}
	}

	if (!is_sorted) {
		struct Comparator {
			bool operator()(const MTL::FunctionConstant *p, const MTL::FunctionConstant *q) const {
				return p->index() < q->index();
			}
		};

		constants.sort_custom<Comparator>();
	}

	// Build a sorted list of specialization constants by constant_id.
	uint32_t *indexes = (uint32_t *)alloca(p_specialization_constants.size() * sizeof(uint32_t));
	for (uint32_t i = 0; i < p_specialization_constants.size(); i++) {
		indexes[i] = i;
	}
	std::sort(indexes, &indexes[p_specialization_constants.size()], [&](int a, int b) {
		return p_specialization_constants[a].constant_id < p_specialization_constants[b].constant_id;
	});

	NS::SharedPtr<MTL::FunctionConstantValues> constantValues = NS::TransferPtr(MTL::FunctionConstantValues::alloc()->init());

	// Merge the sorted constants from the function with the sorted user constants.
	NS::UInteger i = 0;
	uint32_t j = 0;
	while (i < constants.size() && j < p_specialization_constants.size()) {
		MTL::FunctionConstant *curr = (MTL::FunctionConstant *)constants[i];
		const PipelineSpecializationConstant &sc = p_specialization_constants[indexes[j]];
		if (curr->index() == sc.constant_id) {
			switch (curr->type()) {
				case MTL::DataTypeBool:
				case MTL::DataTypeFloat:
				case MTL::DataTypeInt:
				case MTL::DataTypeUInt: {
					constantValues->setConstantValue(&sc.int_value, curr->type(), sc.constant_id);
				} break;
				default:
					ERR_FAIL_V_MSG(NS::TransferPtr(function), "Invalid specialization constant type");
			}
			i++;
			j++;
		} else if (curr->index() < sc.constant_id) {
			i++;
		} else {
			j++;
		}
	}

	// Handle R32UI_ALIGNMENT_CONSTANT_ID if present.
	if (i < constants.size()) {
		MTL::FunctionConstant *curr = constants[i];
		if (curr->index() == R32UI_ALIGNMENT_CONSTANT_ID) {
			uint32_t alignment = 16; // TODO(sgc): is this always correct?
			constantValues->setConstantValue(&alignment, curr->type(), curr->index());
			i++;
		}
	}

	NS::Error *err = nullptr;
	function->release();
	function = library->newFunction(p_name, constantValues.get(), &err);
	ERR_FAIL_NULL_V_MSG(function, ERR_CANT_CREATE, String("specialized function failed: ") + (err ? err->localizedDescription()->utf8String() : "unknown error"));

	return NS::TransferPtr(function);
}

// RDD::PolygonCullMode == MTL::CullMode.
static_assert(ENUM_MEMBERS_EQUAL(RDD::POLYGON_CULL_DISABLED, MTL::CullModeNone));
static_assert(ENUM_MEMBERS_EQUAL(RDD::POLYGON_CULL_FRONT, MTL::CullModeFront));
static_assert(ENUM_MEMBERS_EQUAL(RDD::POLYGON_CULL_BACK, MTL::CullModeBack));

// RDD::StencilOperation == MTL::StencilOperation.
static_assert(ENUM_MEMBERS_EQUAL(RDD::STENCIL_OP_KEEP, MTL::StencilOperationKeep));
static_assert(ENUM_MEMBERS_EQUAL(RDD::STENCIL_OP_ZERO, MTL::StencilOperationZero));
static_assert(ENUM_MEMBERS_EQUAL(RDD::STENCIL_OP_REPLACE, MTL::StencilOperationReplace));
static_assert(ENUM_MEMBERS_EQUAL(RDD::STENCIL_OP_INCREMENT_AND_CLAMP, MTL::StencilOperationIncrementClamp));
static_assert(ENUM_MEMBERS_EQUAL(RDD::STENCIL_OP_DECREMENT_AND_CLAMP, MTL::StencilOperationDecrementClamp));
static_assert(ENUM_MEMBERS_EQUAL(RDD::STENCIL_OP_INVERT, MTL::StencilOperationInvert));
static_assert(ENUM_MEMBERS_EQUAL(RDD::STENCIL_OP_INCREMENT_AND_WRAP, MTL::StencilOperationIncrementWrap));
static_assert(ENUM_MEMBERS_EQUAL(RDD::STENCIL_OP_DECREMENT_AND_WRAP, MTL::StencilOperationDecrementWrap));

// RDD::BlendOperation == MTL::BlendOperation.
static_assert(ENUM_MEMBERS_EQUAL(RDD::BLEND_OP_ADD, MTL::BlendOperationAdd));
static_assert(ENUM_MEMBERS_EQUAL(RDD::BLEND_OP_SUBTRACT, MTL::BlendOperationSubtract));
static_assert(ENUM_MEMBERS_EQUAL(RDD::BLEND_OP_REVERSE_SUBTRACT, MTL::BlendOperationReverseSubtract));
static_assert(ENUM_MEMBERS_EQUAL(RDD::BLEND_OP_MINIMUM, MTL::BlendOperationMin));
static_assert(ENUM_MEMBERS_EQUAL(RDD::BLEND_OP_MAXIMUM, MTL::BlendOperationMax));

RDD::PipelineID RenderingDeviceDriverMetal::render_pipeline_create(
		ShaderID p_shader,
		VertexFormatID p_vertex_format,
		RenderPrimitive p_render_primitive,
		PipelineRasterizationState p_rasterization_state,
		PipelineMultisampleState p_multisample_state,
		PipelineDepthStencilState p_depth_stencil_state,
		PipelineColorBlendState p_blend_state,
		VectorView<int32_t> p_color_attachments,
		uint32_t p_dynamic_state,
		RenderPassID p_render_pass,
		uint32_t p_render_subpass,
		VectorView<PipelineSpecializationConstant> p_specialization_constants) {
	MDRenderShader *shader = (MDRenderShader *)(p_shader.id);
	MTL::VertexDescriptor *vert_desc = reinterpret_cast<MTL::VertexDescriptor *>(p_vertex_format.id);
	MDRenderPass *pass = (MDRenderPass *)(p_render_pass.id);

	os_signpost_id_t reflect_id = os_signpost_id_make_with_pointer(LOG_INTERVALS, shader);
	os_signpost_interval_begin(LOG_INTERVALS, reflect_id, "render_pipeline_create", "shader_name=%{public}s", shader->name.get_data());
	DEFER([=]() {
		os_signpost_interval_end(LOG_INTERVALS, reflect_id, "render_pipeline_create");
	});

	os_signpost_event_emit(LOG_DRIVER, OS_SIGNPOST_ID_EXCLUSIVE, "create_pipeline");

	NS::SharedPtr<MTL::RenderPipelineDescriptor> desc = NS::TransferPtr(MTL::RenderPipelineDescriptor::alloc()->init());

	{
		const MDSubpass &subpass = pass->subpasses[p_render_subpass];
		for (uint32_t i = 0; i < subpass.color_references.size(); i++) {
			uint32_t attachment = subpass.color_references[i].attachment;
			if (attachment != AttachmentReference::UNUSED) {
				const MDAttachment &a = pass->attachments[attachment];
				desc->colorAttachments()->object(i)->setPixelFormat(a.format);
			}
		}

		if (subpass.depth_stencil_reference.attachment != AttachmentReference::UNUSED) {
			uint32_t attachment = subpass.depth_stencil_reference.attachment;
			const MDAttachment &a = pass->attachments[attachment];

			if (a.type & MDAttachmentType::Depth) {
				desc->setDepthAttachmentPixelFormat(a.format);
			}

			if (a.type & MDAttachmentType::Stencil) {
				desc->setStencilAttachmentPixelFormat(a.format);
			}
		}
	}

	desc->setVertexDescriptor(vert_desc);
	desc->setLabel(conv::to_nsstring(shader->name));

	if (shader->uses_argument_buffers) {
		// Set mutability of argument buffers.
		for (uint32_t i = 0; i < shader->sets.size(); i++) {
			const UniformSet &set = shader->sets[i];
			const MTL::Mutability mutability = set.dynamic_uniforms.is_empty() ? MTL::MutabilityImmutable : MTL::MutabilityMutable;
			desc->vertexBuffers()->object(i)->setMutability(mutability);
			desc->fragmentBuffers()->object(i)->setMutability(mutability);
		}
	}

	// Input assembly & tessellation.

	MDRenderPipeline *pipeline = new MDRenderPipeline();

	switch (p_render_primitive) {
		case RENDER_PRIMITIVE_POINTS:
			desc->setInputPrimitiveTopology(MTL::PrimitiveTopologyClassPoint);
			break;
		case RENDER_PRIMITIVE_LINES:
		case RENDER_PRIMITIVE_LINES_WITH_ADJACENCY:
		case RENDER_PRIMITIVE_LINESTRIPS_WITH_ADJACENCY:
		case RENDER_PRIMITIVE_LINESTRIPS:
			desc->setInputPrimitiveTopology(MTL::PrimitiveTopologyClassLine);
			break;
		case RENDER_PRIMITIVE_TRIANGLES:
		case RENDER_PRIMITIVE_TRIANGLE_STRIPS:
		case RENDER_PRIMITIVE_TRIANGLES_WITH_ADJACENCY:
		case RENDER_PRIMITIVE_TRIANGLE_STRIPS_WITH_AJACENCY:
		case RENDER_PRIMITIVE_TRIANGLE_STRIPS_WITH_RESTART_INDEX:
			desc->setInputPrimitiveTopology(MTL::PrimitiveTopologyClassTriangle);
			break;
		case RENDER_PRIMITIVE_TESSELATION_PATCH:
			desc->setMaxTessellationFactor(p_rasterization_state.patch_control_points);
			desc->setTessellationPartitionMode(MTL::TessellationPartitionModeInteger);
			ERR_FAIL_V_MSG(PipelineID(), "tessellation not implemented");
			break;
		case RENDER_PRIMITIVE_MAX:
		default:
			desc->setInputPrimitiveTopology(MTL::PrimitiveTopologyClassUnspecified);
			break;
	}

	switch (p_render_primitive) {
		case RENDER_PRIMITIVE_POINTS:
			pipeline->raster_state.render_primitive = MTL::PrimitiveTypePoint;
			break;
		case RENDER_PRIMITIVE_LINES:
		case RENDER_PRIMITIVE_LINES_WITH_ADJACENCY:
			pipeline->raster_state.render_primitive = MTL::PrimitiveTypeLine;
			break;
		case RENDER_PRIMITIVE_LINESTRIPS:
		case RENDER_PRIMITIVE_LINESTRIPS_WITH_ADJACENCY:
			pipeline->raster_state.render_primitive = MTL::PrimitiveTypeLineStrip;
			break;
		case RENDER_PRIMITIVE_TRIANGLES:
		case RENDER_PRIMITIVE_TRIANGLES_WITH_ADJACENCY:
			pipeline->raster_state.render_primitive = MTL::PrimitiveTypeTriangle;
			break;
		case RENDER_PRIMITIVE_TRIANGLE_STRIPS:
		case RENDER_PRIMITIVE_TRIANGLE_STRIPS_WITH_AJACENCY:
		case RENDER_PRIMITIVE_TRIANGLE_STRIPS_WITH_RESTART_INDEX:
			pipeline->raster_state.render_primitive = MTL::PrimitiveTypeTriangleStrip;
			break;
		default:
			break;
	}

	// Rasterization.
	desc->setRasterizationEnabled(!p_rasterization_state.discard_primitives);
	pipeline->raster_state.clip_mode = p_rasterization_state.enable_depth_clamp ? MTL::DepthClipModeClamp : MTL::DepthClipModeClip;
	pipeline->raster_state.fill_mode = p_rasterization_state.wireframe ? MTL::TriangleFillModeLines : MTL::TriangleFillModeFill;

	static const MTL::CullMode CULL_MODE[3] = {
		MTL::CullModeNone,
		MTL::CullModeFront,
		MTL::CullModeBack,
	};
	pipeline->raster_state.cull_mode = CULL_MODE[p_rasterization_state.cull_mode];
	pipeline->raster_state.winding = (p_rasterization_state.front_face == POLYGON_FRONT_FACE_CLOCKWISE) ? MTL::WindingClockwise : MTL::WindingCounterClockwise;
	pipeline->raster_state.depth_bias.enabled = p_rasterization_state.depth_bias_enabled;
	pipeline->raster_state.depth_bias.depth_bias = p_rasterization_state.depth_bias_constant_factor;
	pipeline->raster_state.depth_bias.slope_scale = p_rasterization_state.depth_bias_slope_factor;
	pipeline->raster_state.depth_bias.clamp = p_rasterization_state.depth_bias_clamp;
	// In Metal there is no line width.
	if (!Math::is_equal_approx(p_rasterization_state.line_width, 1.0f)) {
		WARN_PRINT("unsupported: line width");
	}

	// Multisample.
	if (p_multisample_state.enable_sample_shading) {
		WARN_PRINT("unsupported: multi-sample shading");
	}

	if (p_multisample_state.sample_count > TEXTURE_SAMPLES_1) {
		pipeline->sample_count = (*device_properties).find_nearest_supported_sample_count(p_multisample_state.sample_count);
	}
	desc->setRasterSampleCount(static_cast<NS::UInteger>(pipeline->sample_count));
	desc->setAlphaToCoverageEnabled(p_multisample_state.enable_alpha_to_coverage);
	desc->setAlphaToOneEnabled(p_multisample_state.enable_alpha_to_one);

	// Depth buffer.
	bool depth_enabled = p_depth_stencil_state.enable_depth_test && desc->depthAttachmentPixelFormat() != MTL::PixelFormatInvalid;
	bool stencil_enabled = p_depth_stencil_state.enable_stencil && desc->stencilAttachmentPixelFormat() != MTL::PixelFormatInvalid;

	if (depth_enabled || stencil_enabled) {
		NS::SharedPtr<MTL::DepthStencilDescriptor> ds_desc = NS::TransferPtr(MTL::DepthStencilDescriptor::alloc()->init());

		pipeline->raster_state.depth_test.enabled = depth_enabled;
		ds_desc->setDepthWriteEnabled(p_depth_stencil_state.enable_depth_write);
		ds_desc->setDepthCompareFunction(COMPARE_OPERATORS[p_depth_stencil_state.depth_compare_operator]);
		if (p_depth_stencil_state.enable_depth_range) {
			WARN_PRINT("unsupported: depth range");
		}

		if (stencil_enabled) {
			pipeline->raster_state.stencil.enabled = true;
			pipeline->raster_state.stencil.front_reference = p_depth_stencil_state.front_op.reference;
			pipeline->raster_state.stencil.back_reference = p_depth_stencil_state.back_op.reference;

			{
				// Front.
				NS::SharedPtr<MTL::StencilDescriptor> sd = NS::TransferPtr(MTL::StencilDescriptor::alloc()->init());
				sd->setStencilFailureOperation(STENCIL_OPERATIONS[p_depth_stencil_state.front_op.fail]);
				sd->setDepthStencilPassOperation(STENCIL_OPERATIONS[p_depth_stencil_state.front_op.pass]);
				sd->setDepthFailureOperation(STENCIL_OPERATIONS[p_depth_stencil_state.front_op.depth_fail]);
				sd->setStencilCompareFunction(COMPARE_OPERATORS[p_depth_stencil_state.front_op.compare]);
				sd->setReadMask(p_depth_stencil_state.front_op.compare_mask);
				sd->setWriteMask(p_depth_stencil_state.front_op.write_mask);
				ds_desc->setFrontFaceStencil(sd.get());
			}
			{
				// Back.
				NS::SharedPtr<MTL::StencilDescriptor> sd = NS::TransferPtr(MTL::StencilDescriptor::alloc()->init());
				sd->setStencilFailureOperation(STENCIL_OPERATIONS[p_depth_stencil_state.back_op.fail]);
				sd->setDepthStencilPassOperation(STENCIL_OPERATIONS[p_depth_stencil_state.back_op.pass]);
				sd->setDepthFailureOperation(STENCIL_OPERATIONS[p_depth_stencil_state.back_op.depth_fail]);
				sd->setStencilCompareFunction(COMPARE_OPERATORS[p_depth_stencil_state.back_op.compare]);
				sd->setReadMask(p_depth_stencil_state.back_op.compare_mask);
				sd->setWriteMask(p_depth_stencil_state.back_op.write_mask);
				ds_desc->setBackFaceStencil(sd.get());
			}
		}

		pipeline->depth_stencil = NS::TransferPtr(device->newDepthStencilState(ds_desc.get()));
		ERR_FAIL_COND_V_MSG(!pipeline->depth_stencil, PipelineID(), "Failed to create depth stencil state");
	} else {
		// TODO(sgc): FB13671991 raised as Apple docs state calling setDepthStencilState:nil is valid, but currently generates an exception
		pipeline->depth_stencil = NS::RetainPtr(get_resource_cache().get_depth_stencil_state(false, false));
	}

	// Blend state.
	{
		for (uint32_t i = 0; i < p_color_attachments.size(); i++) {
			if (p_color_attachments[i] == ATTACHMENT_UNUSED) {
				continue;
			}

			const PipelineColorBlendState::Attachment &bs = p_blend_state.attachments[i];

			MTL::RenderPipelineColorAttachmentDescriptor *ca_desc = desc->colorAttachments()->object(p_color_attachments[i]);
			ca_desc->setBlendingEnabled(bs.enable_blend);

			ca_desc->setSourceRGBBlendFactor(BLEND_FACTORS[bs.src_color_blend_factor]);
			ca_desc->setDestinationRGBBlendFactor(BLEND_FACTORS[bs.dst_color_blend_factor]);
			ca_desc->setRgbBlendOperation(BLEND_OPERATIONS[bs.color_blend_op]);

			ca_desc->setSourceAlphaBlendFactor(BLEND_FACTORS[bs.src_alpha_blend_factor]);
			ca_desc->setDestinationAlphaBlendFactor(BLEND_FACTORS[bs.dst_alpha_blend_factor]);
			ca_desc->setAlphaBlendOperation(BLEND_OPERATIONS[bs.alpha_blend_op]);

			MTL::ColorWriteMask writeMask = MTL::ColorWriteMaskNone;
			if (bs.write_r) {
				writeMask |= MTL::ColorWriteMaskRed;
			}
			if (bs.write_g) {
				writeMask |= MTL::ColorWriteMaskGreen;
			}
			if (bs.write_b) {
				writeMask |= MTL::ColorWriteMaskBlue;
			}
			if (bs.write_a) {
				writeMask |= MTL::ColorWriteMaskAlpha;
			}
			ca_desc->setWriteMask(writeMask);
		}

		pipeline->raster_state.blend.r = p_blend_state.blend_constant.r;
		pipeline->raster_state.blend.g = p_blend_state.blend_constant.g;
		pipeline->raster_state.blend.b = p_blend_state.blend_constant.b;
		pipeline->raster_state.blend.a = p_blend_state.blend_constant.a;
	}

	// Dynamic state.

	if (p_dynamic_state.has_flag(DYNAMIC_STATE_DEPTH_BIAS)) {
		pipeline->raster_state.depth_bias.enabled = true;
	}

	if (p_dynamic_state.has_flag(DYNAMIC_STATE_BLEND_CONSTANTS)) {
		pipeline->raster_state.blend.enabled = true;
	}

	if (p_dynamic_state.has_flag(DYNAMIC_STATE_DEPTH_BOUNDS)) {
		// TODO(sgc): ??
	}

	if (p_dynamic_state.has_flag(DYNAMIC_STATE_STENCIL_COMPARE_MASK)) {
		// TODO(sgc): ??
	}

	if (p_dynamic_state.has_flag(DYNAMIC_STATE_STENCIL_WRITE_MASK)) {
		// TODO(sgc): ??
	}

	if (p_dynamic_state.has_flag(DYNAMIC_STATE_STENCIL_REFERENCE)) {
		pipeline->raster_state.stencil.enabled = true;
	}

	if (shader->vert) {
		Result<NS::SharedPtr<MTL::Function>> function_or_err = _create_function(shader->vert.get(), MTLSTR("main0"), p_specialization_constants);
		ERR_FAIL_COND_V(std::holds_alternative<Error>(function_or_err), PipelineID());
		desc->setVertexFunction(std::get<NS::SharedPtr<MTL::Function>>(function_or_err).get());
	}

	if (shader->frag) {
		Result<NS::SharedPtr<MTL::Function>> function_or_err = _create_function(shader->frag.get(), MTLSTR("main0"), p_specialization_constants);
		ERR_FAIL_COND_V(std::holds_alternative<Error>(function_or_err), PipelineID());
		desc->setFragmentFunction(std::get<NS::SharedPtr<MTL::Function>>(function_or_err).get());
	}

	MTL::PipelineOption options = MTL::PipelineOptionNone;
	MTL::BinaryArchive *arc = archive.get();
	if (arc) {
		NS::SharedPtr<NS::Array> archives = NS::TransferPtr(NS::Array::array(reinterpret_cast<NS::Object *const *>(&arc), 1)->retain());
		desc->setBinaryArchives(archives.get());
		if (archive_fail_on_miss) {
			options |= MTL::PipelineOptionFailOnBinaryArchiveMiss;
		}
	}

	NS::Error *error = nullptr;
	pipeline->state = NS::TransferPtr(device->newRenderPipelineState(desc.get(), options, nullptr, &error));
	pipeline->shader = shader;

	ERR_FAIL_COND_V_MSG(error != nullptr, PipelineID(), String("error creating pipeline: ") + error->localizedDescription()->utf8String());
	ERR_FAIL_COND_V_MSG(!pipeline->state, PipelineID(), "Failed to create render pipeline state");

	if (arc) {
		if (arc->addRenderPipelineFunctions(desc.get(), &error)) {
			archive_count += 1;
		} else {
			print_error(error->localizedDescription()->utf8String());
		}
	}

	return PipelineID(pipeline);
}

#pragma mark - Compute

// ----- COMMANDS -----

void RenderingDeviceDriverMetal::command_bind_compute_pipeline(CommandBufferID p_cmd_buffer, PipelineID p_pipeline) {
	MDCommandBufferBase *cb = (MDCommandBufferBase *)(p_cmd_buffer.id);
	cb->bind_pipeline(p_pipeline);
}

void RenderingDeviceDriverMetal::command_bind_compute_uniform_sets(CommandBufferID p_cmd_buffer, VectorView<UniformSetID> p_uniform_sets, ShaderID p_shader, uint32_t p_first_set_index, uint32_t p_set_count, uint32_t p_dynamic_offsets) {
	MDCommandBufferBase *cb = (MDCommandBufferBase *)(p_cmd_buffer.id);
	cb->compute_bind_uniform_sets(p_uniform_sets, p_shader, p_first_set_index, p_set_count, p_dynamic_offsets);
}

void RenderingDeviceDriverMetal::command_compute_dispatch(CommandBufferID p_cmd_buffer, uint32_t p_x_groups, uint32_t p_y_groups, uint32_t p_z_groups) {
	MDCommandBufferBase *cb = (MDCommandBufferBase *)(p_cmd_buffer.id);
	cb->compute_dispatch(p_x_groups, p_y_groups, p_z_groups);
}

void RenderingDeviceDriverMetal::command_compute_dispatch_indirect(CommandBufferID p_cmd_buffer, BufferID p_indirect_buffer, uint64_t p_offset) {
	MDCommandBufferBase *cb = (MDCommandBufferBase *)(p_cmd_buffer.id);
	cb->compute_dispatch_indirect(p_indirect_buffer, p_offset);
}

// ----- PIPELINE -----

RDD::PipelineID RenderingDeviceDriverMetal::compute_pipeline_create(ShaderID p_shader, VectorView<PipelineSpecializationConstant> p_specialization_constants) {
	MDComputeShader *shader = (MDComputeShader *)(p_shader.id);

	os_signpost_id_t reflect_id = os_signpost_id_make_with_pointer(LOG_INTERVALS, shader);
	os_signpost_interval_begin(LOG_INTERVALS, reflect_id, "compute_pipeline_create", "shader_name=%{public}s", shader->name.get_data());
	DEFER([=]() {
		os_signpost_interval_end(LOG_INTERVALS, reflect_id, "compute_pipeline_create");
	});

	os_signpost_event_emit(LOG_DRIVER, OS_SIGNPOST_ID_EXCLUSIVE, "create_pipeline");

	Result<NS::SharedPtr<MTL::Function>> function_or_err = _create_function(shader->kernel.get(), MTLSTR("main0"), p_specialization_constants);
	ERR_FAIL_COND_V(std::holds_alternative<Error>(function_or_err), PipelineID());
	NS::SharedPtr<MTL::Function> function = std::get<NS::SharedPtr<MTL::Function>>(function_or_err);

	NS::SharedPtr<MTL::ComputePipelineDescriptor> desc = NS::TransferPtr(MTL::ComputePipelineDescriptor::alloc()->init());
	desc->setComputeFunction(function.get());
	desc->setLabel(conv::to_nsstring(shader->name));

	if (shader->uses_argument_buffers) {
		// Set mutability of argument buffers.
		for (uint32_t i = 0; i < shader->sets.size(); i++) {
			const UniformSet &set = shader->sets[i];
			const MTL::Mutability mutability = set.dynamic_uniforms.is_empty() ? MTL::MutabilityImmutable : MTL::MutabilityMutable;
			desc->buffers()->object(i)->setMutability(mutability);
		}
	}

	MTL::PipelineOption options = MTL::PipelineOptionNone;
	MTL::BinaryArchive *arc = archive.get();
	if (arc) {
		NS::SharedPtr<NS::Array> archives = NS::TransferPtr(NS::Array::array(reinterpret_cast<NS::Object *const *>(&arc), 1)->retain());
		desc->setBinaryArchives(archives.get());
		if (archive_fail_on_miss) {
			options |= MTL::PipelineOptionFailOnBinaryArchiveMiss;
		}
	}

	NS::Error *error = nullptr;
	NS::SharedPtr<MTL::ComputePipelineState> state = NS::TransferPtr(device->newComputePipelineState(desc.get(), options, nullptr, &error));
	ERR_FAIL_COND_V_MSG(error != nullptr, PipelineID(), String("error creating pipeline: ") + error->localizedDescription()->utf8String());
	ERR_FAIL_COND_V_MSG(!state, PipelineID(), "Failed to create compute pipeline state");

	MDComputePipeline *pipeline = new MDComputePipeline(state);
	pipeline->compute_state.local = shader->local;
	pipeline->shader = shader;

	if (arc) {
		if (arc->addComputePipelineFunctions(desc.get(), &error)) {
			archive_count += 1;
		} else {
			print_error(error->localizedDescription()->utf8String());
		}
	}

	return PipelineID(pipeline);
}

#pragma mark - Raytracing

// ----- ACCELERATION STRUCTURE -----

RDD::AccelerationStructureID RenderingDeviceDriverMetal::blas_create(VectorView<AccelerationStructureGeometry> p_geometries, uint32_t p_flags) {
	ERR_FAIL_V_MSG(AccelerationStructureID(), "Ray tracing is not currently supported by the Metal driver.");
}

RDD::AccelerationStructureID RenderingDeviceDriverMetal::tlas_create(uint32_t p_max_instance_count, uint32_t p_flags) {
	ERR_FAIL_V_MSG(AccelerationStructureID(), "Ray tracing is not currently supported by the Metal driver.");
}

void RenderingDeviceDriverMetal::acceleration_structure_instance_write(uint8_t *r_driver_instance, const AccelerationStructureInstance &p_instance) {
	ERR_FAIL_MSG("Ray tracing is not currently supported by the Metal driver.");
}

void RenderingDeviceDriverMetal::acceleration_structure_free(RDD::AccelerationStructureID p_acceleration_structure) {
	ERR_FAIL_MSG("Ray tracing is not currently supported by the Metal driver.");
}

uint32_t RenderingDeviceDriverMetal::acceleration_structure_get_scratch_size_bytes(AccelerationStructureID p_acceleration_structure) {
	ERR_FAIL_V_MSG(0, "Ray tracing is not currently supported by the Metal driver.");
}

// ----- PIPELINE -----

RDD::RaytracingPipelineID RenderingDeviceDriverMetal::raytracing_pipeline_create(VectorView<PipelineShader> p_shaders, VectorView<uint32_t> p_raygen_shader_indices, VectorView<uint32_t> p_miss_shader_indices, VectorView<HitGroup> p_hit_groups, uint32_t p_max_trace_recursion_depth, ShaderID p_layout_defining_shader) {
	ERR_FAIL_V_MSG(RaytracingPipelineID(), "Ray tracing is not currently supported by the Metal driver.");
}

void RenderingDeviceDriverMetal::raytracing_pipeline_free(RDD::RaytracingPipelineID p_pipeline) {
	ERR_FAIL_MSG("Ray tracing is not currently supported by the Metal driver.");
}

bool RenderingDeviceDriverMetal::raytracing_pipeline_get_shader_group_handles(RaytracingPipelineID p_pipeline, uint32_t p_group_index_offset, VectorView<uint32_t> p_group_indices, uint8_t *r_data, uint32_t p_data_stride_bytes) {
	ERR_FAIL_V_MSG(false, "Ray tracing is not currently supported by the Metal driver.");
}

// ----- COMMANDS -----

void RenderingDeviceDriverMetal::command_build_blas(CommandBufferID p_cmd_buffer, AccelerationStructureID p_acceleration_structure, BufferID p_scratch_buffer) {
	ERR_FAIL_MSG("Ray tracing is not currently supported by the Metal driver.");
}

void RenderingDeviceDriverMetal::command_build_tlas(CommandBufferID p_cmd_buffer, AccelerationStructureID p_acceleration_structure, BufferID p_scratch_buffer, BufferID p_instance_buffer, uint32_t p_instance_offset, uint32_t p_instance_count) {
	ERR_FAIL_MSG("Ray tracing is not currently supported by the Metal driver.");
}

void RenderingDeviceDriverMetal::command_bind_raytracing_pipeline(CommandBufferID p_cmd_buffer, RaytracingPipelineID p_pipeline) {
	ERR_FAIL_MSG("Ray tracing is not currently supported by the Metal driver.");
}

void RenderingDeviceDriverMetal::command_bind_raytracing_uniform_set(CommandBufferID p_cmd_buffer, UniformSetID p_uniform_set, ShaderID p_shader, uint32_t p_set_index) {
	ERR_FAIL_MSG("Ray tracing is not currently supported by the Metal driver.");
}

void RenderingDeviceDriverMetal::command_trace_rays(CommandBufferID p_cmd_buffer, const ShaderBindingTable &p_raygen_sbt, const ShaderBindingTable &p_miss_sbt, const ShaderBindingTable &p_hit_sbt, uint32_t p_width, uint32_t p_height, uint32_t p_depth) {
	ERR_FAIL_MSG("Ray tracing is not currently supported by the Metal driver.");
}

#pragma mark - Queries

// ----- TIMESTAMP -----

RDD::QueryPoolID RenderingDeviceDriverMetal::timestamp_query_pool_create(uint32_t p_query_count) {
	return QueryPoolID(1);
}

void RenderingDeviceDriverMetal::timestamp_query_pool_free(QueryPoolID p_pool_id) {
}

void RenderingDeviceDriverMetal::timestamp_query_pool_get_results(QueryPoolID p_pool_id, uint32_t p_query_count, uint64_t *r_results) {
	// Metal doesn't support timestamp queries, so we just clear the buffer.
	bzero(r_results, p_query_count * sizeof(uint64_t));
}

uint64_t RenderingDeviceDriverMetal::timestamp_query_result_to_time(uint64_t p_result) {
	return p_result;
}

void RenderingDeviceDriverMetal::command_timestamp_query_pool_reset(CommandBufferID p_cmd_buffer, QueryPoolID p_pool_id, uint32_t p_query_count) {
}

void RenderingDeviceDriverMetal::command_timestamp_write(CommandBufferID p_cmd_buffer, QueryPoolID p_pool_id, uint32_t p_index) {
}

#pragma mark - Labels

void RenderingDeviceDriverMetal::command_begin_label(CommandBufferID p_cmd_buffer, const char *p_label_name, const Color &p_color) {
	MDCommandBufferBase *cb = (MDCommandBufferBase *)(p_cmd_buffer.id);
	cb->begin_label(p_label_name, p_color);
}

void RenderingDeviceDriverMetal::command_end_label(CommandBufferID p_cmd_buffer) {
	MDCommandBufferBase *cb = (MDCommandBufferBase *)(p_cmd_buffer.id);
	cb->end_label();
}

#pragma mark - Debug

void RenderingDeviceDriverMetal::command_insert_breadcrumb(CommandBufferID p_cmd_buffer, uint32_t p_data) {
	// TODO: Implement.
}

#pragma mark - Submission

void RenderingDeviceDriverMetal::begin_segment(uint32_t p_frame_index, uint32_t p_frames_drawn) {
	_frame_index = p_frame_index;
	_frames_drawn = p_frames_drawn;
}

void RenderingDeviceDriverMetal::end_segment() {
	MutexLock lock(copy_queue_mutex);
	_copy_queue_flush();
}

#pragma mark - Misc

void RenderingDeviceDriverMetal::set_object_name(ObjectType p_type, ID p_driver_id, const String &p_name) {
	NS::String *label = conv::to_nsstring(p_name);

	switch (p_type) {
		case OBJECT_TYPE_TEXTURE: {
			MTL::Texture *tex = reinterpret_cast<MTL::Texture *>(p_driver_id.id);
			tex->setLabel(label);
		} break;
		case OBJECT_TYPE_SAMPLER: {
			// Can't set label after creation.
		} break;
		case OBJECT_TYPE_BUFFER: {
			const BufferInfo *buf_info = (const BufferInfo *)p_driver_id.id;
			buf_info->metal_buffer.get()->setLabel(label);
		} break;
		case OBJECT_TYPE_SHADER: {
			MDShader *shader = (MDShader *)(p_driver_id.id);
			if (MDRenderShader *rs = dynamic_cast<MDRenderShader *>(shader); rs != nullptr) {
				rs->vert->set_label(label);
				rs->frag->set_label(label);
			} else if (MDComputeShader *cs = dynamic_cast<MDComputeShader *>(shader); cs != nullptr) {
				cs->kernel->set_label(label);
			} else {
				DEV_ASSERT(false);
			}
		} break;
		case OBJECT_TYPE_UNIFORM_SET: {
			MDUniformSet *set = (MDUniformSet *)(p_driver_id.id);
			set->arg_buffer->setLabel(label);
		} break;
		case OBJECT_TYPE_PIPELINE: {
			// Can't set label after creation.
		} break;
		default: {
			DEV_ASSERT(false);
		}
	}
}

uint64_t RenderingDeviceDriverMetal::get_resource_native_handle(DriverResource p_type, ID p_driver_id) {
	switch (p_type) {
		case DRIVER_RESOURCE_LOGICAL_DEVICE: {
			return (uint64_t)(uintptr_t)device;
		}
		case DRIVER_RESOURCE_PHYSICAL_DEVICE: {
			return 0;
		}
		case DRIVER_RESOURCE_TOPMOST_OBJECT: {
			return 0;
		}
		case DRIVER_RESOURCE_COMMAND_QUEUE: {
			return (uint64_t)(uintptr_t)get_command_queue();
		}
		case DRIVER_RESOURCE_QUEUE_FAMILY: {
			return 0;
		}
		case DRIVER_RESOURCE_TEXTURE: {
			return p_driver_id.id;
		}
		case DRIVER_RESOURCE_TEXTURE_VIEW: {
			return p_driver_id.id;
		}
		case DRIVER_RESOURCE_TEXTURE_DATA_FORMAT: {
			return 0;
		}
		case DRIVER_RESOURCE_SAMPLER: {
			return p_driver_id.id;
		}
		case DRIVER_RESOURCE_UNIFORM_SET: {
			return 0;
		}
		case DRIVER_RESOURCE_BUFFER: {
			return p_driver_id.id;
		}
		case DRIVER_RESOURCE_COMPUTE_PIPELINE: {
			MDComputePipeline *pipeline = (MDComputePipeline *)(p_driver_id.id);
			return (uint64_t)(uintptr_t)pipeline->state.get();
		}
		case DRIVER_RESOURCE_RENDER_PIPELINE: {
			MDRenderPipeline *pipeline = (MDRenderPipeline *)(p_driver_id.id);
			return (uint64_t)(uintptr_t)pipeline->state.get();
		}
		default: {
			return 0;
		}
	}
}

void RenderingDeviceDriverMetal::_copy_queue_copy_to_buffer(Span<uint8_t> p_src_data, MTL::Buffer *p_dst_buffer, uint64_t p_dst_offset) {
	MutexLock lock(copy_queue_mutex);
	if (_copy_queue_buffer_available() < p_src_data.size()) {
		_copy_queue_flush();
	}

	MTL::BlitCommandEncoder *blit_encoder = _copy_queue_blit_encoder();

	memcpy(_copy_queue_buffer_ptr(), p_src_data.ptr(), p_src_data.size());

	copy_queue_rs.get()->addAllocation(p_dst_buffer);
	blit_encoder->copyFromBuffer(copy_queue_buffer.get(), copy_queue_buffer_offset, p_dst_buffer, p_dst_offset, p_src_data.size());

	_copy_queue_buffer_consume(p_src_data.size());
}

void RenderingDeviceDriverMetal::_copy_queue_flush() {
	if (!copy_queue_blit_encoder) {
		return;
	}

	copy_queue_rs.get()->addAllocation(copy_queue_buffer.get());
	copy_queue_rs.get()->commit();

	copy_queue_blit_encoder.get()->endEncoding();
	copy_queue_blit_encoder.reset();
	copy_queue_command_buffer.get()->commit();
	copy_queue_command_buffer.get()->waitUntilCompleted();
	copy_queue_command_buffer.reset();
	copy_queue_buffer_offset = 0;
	copy_queue_rs.get()->removeAllAllocations();
}

Error RenderingDeviceDriverMetal::_copy_queue_initialize() {
	DEV_ASSERT(!copy_queue);

	copy_queue = NS::TransferPtr(device->newCommandQueue());
	copy_queue.get()->setLabel(MTLSTR("Copy Command Queue"));
	ERR_FAIL_COND_V(!copy_queue, ERR_CANT_CREATE);

	// Reserve 64 KiB for copy commands. If the buffer fills, it will be flushed automatically.
	copy_queue_buffer = NS::TransferPtr(device->newBuffer(64 * 1024, MTL::ResourceStorageModeShared | MTL::ResourceHazardTrackingModeUntracked));
	copy_queue_buffer.get()->setLabel(MTLSTR("Copy Command Scratch Buffer"));

	if (__builtin_available(macOS 15.0, iOS 18.0, tvOS 18.0, visionOS 1.0, *)) {
		if (device_properties->features.supports_residency_sets) {
			MTL::ResidencySetDescriptor *rs_desc = MTL::ResidencySetDescriptor::alloc()->init();
			rs_desc->setInitialCapacity(2);
			rs_desc->setLabel(MTLSTR("Copy Queue Residency Set"));
			NS::Error *error = nullptr;
			copy_queue_rs = NS::TransferPtr(device->newResidencySet(rs_desc, &error));
			rs_desc->release();
			copy_queue.get()->addResidencySet(copy_queue_rs.get());
		}
	}

	return OK;
}

uint64_t RenderingDeviceDriverMetal::get_total_memory_used() {
	return device->currentAllocatedSize();
}

uint64_t RenderingDeviceDriverMetal::get_lazily_memory_used() {
	return 0; // TODO: Track this (grep for memoryless in Godot's Metal backend).
}

uint64_t RenderingDeviceDriverMetal::limit_get(Limit p_limit) {
	const MetalDeviceProperties &props = (*device_properties);
	const MetalLimits &limits = props.limits;
	uint64_t safe_unbounded = ((uint64_t)1 << 30);
#if defined(DEV_ENABLED)
#define UNKNOWN(NAME) \
	case NAME: \
		WARN_PRINT_ONCE("Returning maximum value for unknown limit " #NAME "."); \
		return safe_unbounded;
#else
#define UNKNOWN(NAME) \
	case NAME: \
		return safe_unbounded
#endif

	// clang-format off
	switch (p_limit) {
		case LIMIT_MAX_BOUND_UNIFORM_SETS:
			return limits.maxBoundDescriptorSets;
		case LIMIT_MAX_FRAMEBUFFER_COLOR_ATTACHMENTS:
			return limits.maxColorAttachments;
		case LIMIT_MAX_TEXTURES_PER_UNIFORM_SET:
			return limits.maxTexturesPerArgumentBuffer;
		case LIMIT_MAX_SAMPLERS_PER_UNIFORM_SET:
			return limits.maxSamplersPerArgumentBuffer;
		case LIMIT_MAX_STORAGE_BUFFERS_PER_UNIFORM_SET:
			return limits.maxBuffersPerArgumentBuffer;
		case LIMIT_MAX_STORAGE_IMAGES_PER_UNIFORM_SET:
			return limits.maxTexturesPerArgumentBuffer;
		case LIMIT_MAX_UNIFORM_BUFFERS_PER_UNIFORM_SET:
			return limits.maxBuffersPerArgumentBuffer;
		case LIMIT_MAX_DRAW_INDEXED_INDEX:
			return limits.maxDrawIndexedIndexValue;
		case LIMIT_MAX_FRAMEBUFFER_HEIGHT:
			return limits.maxFramebufferHeight;
		case LIMIT_MAX_FRAMEBUFFER_WIDTH:
			return limits.maxFramebufferWidth;
		case LIMIT_MAX_TEXTURE_ARRAY_LAYERS:
			return limits.maxImageArrayLayers;
		case LIMIT_MAX_TEXTURE_SIZE_1D:
			return limits.maxImageDimension1D;
		case LIMIT_MAX_TEXTURE_SIZE_2D:
			return limits.maxImageDimension2D;
		case LIMIT_MAX_TEXTURE_SIZE_3D:
			return limits.maxImageDimension3D;
		case LIMIT_MAX_TEXTURE_SIZE_CUBE:
			return limits.maxImageDimensionCube;
		case LIMIT_MAX_TEXTURES_PER_SHADER_STAGE:
			return limits.maxTexturesPerArgumentBuffer;
		case LIMIT_MAX_SAMPLERS_PER_SHADER_STAGE:
			return limits.maxSamplersPerArgumentBuffer;
		case LIMIT_MAX_STORAGE_BUFFERS_PER_SHADER_STAGE:
			return limits.maxBuffersPerArgumentBuffer;
		case LIMIT_MAX_STORAGE_IMAGES_PER_SHADER_STAGE:
			return limits.maxTexturesPerArgumentBuffer;
		case LIMIT_MAX_UNIFORM_BUFFERS_PER_SHADER_STAGE:
			return limits.maxBuffersPerArgumentBuffer;
		case LIMIT_MAX_PUSH_CONSTANT_SIZE:
			return limits.maxBufferLength;
		case LIMIT_MAX_UNIFORM_BUFFER_SIZE:
			return limits.maxBufferLength;
		case LIMIT_MAX_VERTEX_INPUT_ATTRIBUTE_OFFSET:
			return limits.maxVertexDescriptorLayoutStride;
		case LIMIT_MAX_VERTEX_INPUT_ATTRIBUTES:
			return limits.maxVertexInputAttributes;
		case LIMIT_MAX_VERTEX_INPUT_BINDINGS:
			return limits.maxVertexInputBindings;
		case LIMIT_MAX_VERTEX_INPUT_BINDING_STRIDE:
			return limits.maxVertexInputBindingStride;
		case LIMIT_MIN_UNIFORM_BUFFER_OFFSET_ALIGNMENT:
			return limits.minUniformBufferOffsetAlignment;
		case LIMIT_MAX_COMPUTE_WORKGROUP_COUNT_X:
			return limits.maxComputeWorkGroupCount.width;
		case LIMIT_MAX_COMPUTE_WORKGROUP_COUNT_Y:
			return limits.maxComputeWorkGroupCount.height;
		case LIMIT_MAX_COMPUTE_WORKGROUP_COUNT_Z:
			return limits.maxComputeWorkGroupCount.depth;
		case LIMIT_MAX_COMPUTE_WORKGROUP_INVOCATIONS:
			return std::max({ limits.maxThreadsPerThreadGroup.width, limits.maxThreadsPerThreadGroup.height, limits.maxThreadsPerThreadGroup.depth });
		case LIMIT_MAX_COMPUTE_WORKGROUP_SIZE_X:
			return limits.maxThreadsPerThreadGroup.width;
		case LIMIT_MAX_COMPUTE_WORKGROUP_SIZE_Y:
			return limits.maxThreadsPerThreadGroup.height;
		case LIMIT_MAX_COMPUTE_WORKGROUP_SIZE_Z:
			return limits.maxThreadsPerThreadGroup.depth;
		case LIMIT_MAX_COMPUTE_SHARED_MEMORY_SIZE:
			return limits.maxThreadGroupMemoryAllocation;
		case LIMIT_MAX_VIEWPORT_DIMENSIONS_X:
			return limits.maxViewportDimensionX;
		case LIMIT_MAX_VIEWPORT_DIMENSIONS_Y:
			return limits.maxViewportDimensionY;
		case LIMIT_SUBGROUP_SIZE:
			// MoltenVK sets the subgroupSize to the same as the maxSubgroupSize.
			return limits.maxSubgroupSize;
		case LIMIT_SUBGROUP_MIN_SIZE:
			return limits.minSubgroupSize;
		case LIMIT_SUBGROUP_MAX_SIZE:
			return limits.maxSubgroupSize;
		case LIMIT_SUBGROUP_IN_SHADERS:
			return (uint64_t)limits.subgroupSupportedShaderStages;
		case LIMIT_SUBGROUP_OPERATIONS:
			return (uint64_t)limits.subgroupSupportedOperations;
		case LIMIT_METALFX_TEMPORAL_SCALER_MIN_SCALE:
			return (uint64_t)((1.0 / limits.temporalScalerInputContentMaxScale) * 1000'000);
		case LIMIT_METALFX_TEMPORAL_SCALER_MAX_SCALE:
			return (uint64_t)((1.0 / limits.temporalScalerInputContentMinScale) * 1000'000);
		case LIMIT_MAX_SHADER_VARYINGS:
			return limits.maxShaderVaryings;
		default: {
#ifdef DEV_ENABLED
			WARN_PRINT("Returning maximum value for unknown limit " + itos(p_limit) + ".");
#endif
			return safe_unbounded;
		}
	}
	// clang-format on
	return 0;
}

uint64_t RenderingDeviceDriverMetal::api_trait_get(ApiTrait p_trait) {
	switch (p_trait) {
		case API_TRAIT_HONORS_PIPELINE_BARRIERS:
			return use_barriers;
		case API_TRAIT_CLEARS_WITH_COPY_ENGINE:
			return false;
		default:
			return RenderingDeviceDriver::api_trait_get(p_trait);
	}
}

bool RenderingDeviceDriverMetal::has_feature(Features p_feature) {
	switch (p_feature) {
		case SUPPORTS_HALF_FLOAT:
			return true;
		case SUPPORTS_FRAGMENT_SHADER_WITH_ONLY_SIDE_EFFECTS:
			return true;
		case SUPPORTS_BUFFER_DEVICE_ADDRESS:
			return device_properties->features.supports_gpu_address;
		case SUPPORTS_METALFX_SPATIAL:
			return device_properties->features.metal_fx_spatial;
		case SUPPORTS_METALFX_TEMPORAL:
			return device_properties->features.metal_fx_temporal;
		case SUPPORTS_HDR_OUTPUT:
			return true;
		case SUPPORTS_IMAGE_ATOMIC_32_BIT:
			return device_properties->features.supports_native_image_atomics;
		case SUPPORTS_VULKAN_MEMORY_MODEL:
			return true;
		case SUPPORTS_POINT_SIZE:
			return true;
		case SUPPORTS_FRAMEBUFFER_DEPTH_RESOLVE:
			return device_properties->features.supports_msaa_depth_resolve;
		default:
			return false;
	}
}

const RDD::MultiviewCapabilities &RenderingDeviceDriverMetal::get_multiview_capabilities() {
	return multiview_capabilities;
}

const RDD::FragmentShadingRateCapabilities &RenderingDeviceDriverMetal::get_fragment_shading_rate_capabilities() {
	return fsr_capabilities;
}

const RDD::FragmentDensityMapCapabilities &RenderingDeviceDriverMetal::get_fragment_density_map_capabilities() {
	return fdm_capabilities;
}

String RenderingDeviceDriverMetal::get_api_version() const {
	return vformat("%d.%d", capabilities.version_major, capabilities.version_minor);
}

String RenderingDeviceDriverMetal::get_pipeline_cache_uuid() const {
	return pipeline_cache_id;
}

const RDD::Capabilities &RenderingDeviceDriverMetal::get_capabilities() const {
	return capabilities;
}

bool RenderingDeviceDriverMetal::is_composite_alpha_supported(CommandQueueID p_queue) const {
	// The CAMetalLayer.opaque property is configured according to this global setting.
	return OS::is_layered_allowed();
}

size_t RenderingDeviceDriverMetal::get_texel_buffer_alignment_for_format(RDD::DataFormat p_format) const {
	return device->minimumLinearTextureAlignmentForPixelFormat(pixel_formats->getMTLPixelFormat(p_format));
}

size_t RenderingDeviceDriverMetal::get_texel_buffer_alignment_for_format(MTL::PixelFormat p_format) const {
	return device->minimumLinearTextureAlignmentForPixelFormat(p_format);
}

/******************/

RenderingDeviceDriverMetal::RenderingDeviceDriverMetal(RenderingContextDriverMetal *p_context_driver) :
		context_driver(p_context_driver) {
	DEV_ASSERT(p_context_driver != nullptr);
	if (String res = OS::get_environment("GODOT_MTL_ARCHIVE_FAIL_ON_MISS"); res == "1") {
		archive_fail_on_miss = true;
	}

#if TARGET_OS_OSX
	if (String res = OS::get_environment("GODOT_MTL_SHADER_LOAD_STRATEGY"); res == U"lazy") {
		_shader_load_strategy = ShaderLoadStrategy::LAZY;
	}
#else
	// Always use the lazy strategy on other OSs like iOS, tvOS, or visionOS.
	_shader_load_strategy = ShaderLoadStrategy::LAZY;
#endif
}

RenderingDeviceDriverMetal::~RenderingDeviceDriverMetal() {
	for (KeyValue<SHA256Digest, ShaderCacheEntry *> &kv : _shader_cache) {
		memdelete(kv.value);
	}

	memdelete(shader_container_format);
	memdelete(pixel_formats);
	memdelete(device_properties);
}

#pragma mark - Initialization

Error RenderingDeviceDriverMetal::_create_device() {
	device = context_driver->get_metal_device();

	device_scope = NS::TransferPtr(MTL::CaptureManager::sharedCaptureManager()->newCaptureScope(device));
	device_scope->setLabel(MTLSTR("Godot Frame"));
	device_scope->beginScope(); // Allow Xcode to capture the first frame, if desired.

	return OK;
}

void RenderingDeviceDriverMetal::_track_resource(MTL::Resource *p_resource) {
	if (use_barriers) {
		_residency_add.push_back(p_resource);
	}
}

void RenderingDeviceDriverMetal::_untrack_resource(MTL::Resource *p_resource) {
	if (use_barriers) {
		_residency_del.push_back(p_resource);
	}
}

void RenderingDeviceDriverMetal::_check_capabilities() {
	capabilities.device_family = DEVICE_METAL;
	parse_msl_version(device_properties->features.msl_target_version, capabilities.version_major, capabilities.version_minor);
}

API_AVAILABLE(macos(11.0), ios(14.0), tvos(14.0))
static MetalDeviceProfile device_profile_from_properties(MetalDeviceProperties *p_device_properties) {
	using DP = MetalDeviceProfile;
	NS::OperatingSystemVersion os_version = NS::ProcessInfo::processInfo()->operatingSystemVersion();
	MetalDeviceProfile res;
	res.min_os_version = MinOsVersion(os_version.majorVersion, os_version.minorVersion, os_version.patchVersion);
#if TARGET_OS_OSX
	res.platform = DP::Platform::macOS;
#elif TARGET_OS_IPHONE
	res.platform = DP::Platform::iOS;
#elif TARGET_OS_VISION
	res.platform = DP::Platform::visionOS;
#else
#error "Unsupported Apple platform"
#endif
	res.features = {
		.msl_version = p_device_properties->features.msl_target_version,
		.use_argument_buffers = p_device_properties->features.argument_buffers_enabled(),
		.simdPermute = p_device_properties->features.simdPermute,
	};

	// highestFamily will only be set to an Apple GPU family
	switch (p_device_properties->features.highestFamily) {
		case MTL::GPUFamilyApple1:
			res.gpu = DP::GPU::Apple1;
			break;
		case MTL::GPUFamilyApple2:
			res.gpu = DP::GPU::Apple2;
			break;
		case MTL::GPUFamilyApple3:
			res.gpu = DP::GPU::Apple3;
			break;
		case MTL::GPUFamilyApple4:
			res.gpu = DP::GPU::Apple4;
			break;
		case MTL::GPUFamilyApple5:
			res.gpu = DP::GPU::Apple5;
			break;
		case MTL::GPUFamilyApple6:
			res.gpu = DP::GPU::Apple6;
			break;
		case MTL::GPUFamilyApple7:
			res.gpu = DP::GPU::Apple7;
			break;
		case MTL::GPUFamilyApple8:
			res.gpu = DP::GPU::Apple8;
			break;
		case MTL::GPUFamilyApple9:
			res.gpu = DP::GPU::Apple9;
			break;
		default: {
			// Programming error if the default case is hit.
			CRASH_NOW_MSG("Unsupported GPU family");
		} break;
	}

	return res;
}

Error RenderingDeviceDriverMetal::_initialize(uint32_t p_device_index, uint32_t p_frame_count) {
	context_device = context_driver->device_get(p_device_index);
	Error err = _create_device();
	ERR_FAIL_COND_V(err, ERR_CANT_CREATE);

	device_properties = memnew(MetalDeviceProperties(device));
	pixel_formats = memnew(PixelFormats(device, device_properties->features));
	device_profile = device_profile_from_properties(device_properties);
	resource_cache = std::make_unique<MDResourceCache>(device, *pixel_formats, device_properties->limits.maxPerStageBufferCount);
	shader_container_format = memnew(RenderingShaderContainerFormatMetal(&device_profile));

	_check_capabilities();

	err = _copy_queue_initialize();
	ERR_FAIL_COND_V(err, ERR_CANT_CREATE);

	_frame_count = p_frame_count;

	// Set the pipeline cache ID based on the Metal version.
	pipeline_cache_id = "metal-driver-" + get_api_version();

	if (device_properties->features.layeredRendering) {
		multiview_capabilities.is_supported = true;
		multiview_capabilities.max_view_count = device_properties->limits.maxViewports;
		// NOTE: I'm not sure what the limit is as I don't see it referenced anywhere
		multiview_capabilities.max_instance_count = UINT32_MAX;

		print_verbose("- Metal multiview supported:");
		print_verbose("  max view count: " + itos(multiview_capabilities.max_view_count));
		print_verbose("  max instances: " + itos(multiview_capabilities.max_instance_count));
	} else {
		print_verbose("- Metal multiview not supported");
	}

	// The Metal renderer requires Apple4 family. This is 2017 era A11 chips and newer.
	if (device_properties->features.highestFamily < MTL::GPUFamilyApple4) {
		String error_string = vformat("Your Apple GPU does not support the following features, which are required to use Metal-based renderers in Godot:\n\n");
		if (!device_properties->features.imageCubeArray) {
			error_string += "- No support for image cube arrays.\n";
		}

#if defined(APPLE_EMBEDDED_ENABLED)
		// Apple Embedded platforms exports currently don't exit themselves when this method returns `ERR_CANT_CREATE`.
		OS::alert(error_string + "\nClick OK to exit (black screen will be visible).");
#else
		OS::alert(error_string + "\nClick OK to exit.");
#endif

		return ERR_CANT_CREATE;
	}

	return OK;
}

const RenderingShaderContainerFormat &RenderingDeviceDriverMetal::get_shader_container_format() const {
	return *shader_container_format;
}


// --- Merged from drivers/metal/metal_device_profile.cpp ---
#include "drivers/metal/rendering_device_driver_metal3.h"


Mutex MetalDeviceProfile::profiles_lock;
HashMap<MetalDeviceProfile::ProfileKey, MetalDeviceProfile> MetalDeviceProfile::profiles;

const MetalDeviceProfile *MetalDeviceProfile::get_profile(Platform p_platform, GPU p_gpu, MinOsVersion p_min_os_version) {
	DEV_ASSERT(p_platform == Platform::macOS || p_platform == Platform::iOS || p_platform == Platform::visionOS);

	MutexLock lock(profiles_lock);

	ProfileKey key(p_min_os_version, p_platform, p_gpu);
	if (MetalDeviceProfile *profile = profiles.getptr(key)) {
		return profile;
	}

	MetalDeviceProfile res;
	res.platform = p_platform;
	res.gpu = p_gpu;
	res.min_os_version = p_min_os_version;

	switch (p_platform) {
		case Platform::macOS: {
			if (p_min_os_version >= os_version::MACOS_26_0) {
				res.features.msl_version = MSL_VERSION_40;
			} else if (p_min_os_version >= os_version::MACOS_15_0) {
				res.features.msl_version = MSL_VERSION_32;
			} else if (p_min_os_version >= os_version::MACOS_14_0) {
				res.features.msl_version = MSL_VERSION_31;
			} else if (p_min_os_version >= os_version::MACOS_13_0) {
				res.features.msl_version = MSL_VERSION_30;
			} else if (p_min_os_version >= os_version::MACOS_12_0) {
				res.features.msl_version = MSL_VERSION_24;
			} else {
				res.features.msl_version = MSL_VERSION_23;
			}
			res.features.use_argument_buffers = p_min_os_version >= os_version::MACOS_13_0;
			res.features.simdPermute = true;
		} break;

		case Platform::iOS: {
			if (p_min_os_version >= os_version::IOS_26_0) {
				res.features.msl_version = MSL_VERSION_40;
			} else if (p_min_os_version >= os_version::IOS_18_0) {
				res.features.msl_version = MSL_VERSION_32;
			} else if (p_min_os_version >= os_version::IOS_17_0) {
				res.features.msl_version = MSL_VERSION_31;
			} else if (p_min_os_version >= os_version::IOS_16_0) {
				res.features.msl_version = MSL_VERSION_30;
			} else if (p_min_os_version >= os_version::IOS_15_0) {
				res.features.msl_version = MSL_VERSION_24;
			} else {
				res.features.msl_version = MSL_VERSION_23;
			}

			switch (p_gpu) {
				case GPU::Apple1:
				case GPU::Apple2:
				case GPU::Apple3:
				case GPU::Apple4:
				case GPU::Apple5: {
					res.features.simdPermute = false;
					res.features.use_argument_buffers = false;
				} break;
				case GPU::Apple6:
				case GPU::Apple7:
				case GPU::Apple8:
				case GPU::Apple9: {
					res.features.use_argument_buffers = p_min_os_version >= os_version::IOS_16_0;
					res.features.simdPermute = true;
				} break;
			}
		} break;

		case Platform::visionOS: {
			if (p_min_os_version >= os_version::VISIONOS_26_0) {
				res.features.msl_version = MSL_VERSION_40;
			} else if (p_min_os_version >= os_version::VISIONOS_02_4) {
				res.features.msl_version = MSL_VERSION_32;
			} else {
				ERR_FAIL_V_MSG(nullptr, "visionOS 2.4 is the minimum supported version for visionOS.");
			}

			switch (p_gpu) {
				case GPU::Apple8:
				case GPU::Apple9: {
					res.features.use_argument_buffers = true;
					res.features.simdPermute = true;
				} break;
				default: {
					CRASH_NOW_MSG("visionOS hardware has a minimum Apple8 GPU.");
				}
			}
		} break;
	}

	return &profiles.insert(key, res)->value;
}


// --- Merged from drivers/metal/metal_device_properties.cpp ---
#include "drivers/metal/rendering_device_driver_metal3.h"

#include "core/os/os.h"
#include "drivers/metal/rendering_device_driver_metal3.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"

#include <MetalFX/MetalFX.hpp>
#include <spirv_cross.hpp>
#include <spirv_msl.hpp>
#include <unistd.h>

// Common scaling multipliers.
#define KIBI (1024)
#define MEBI (KIBI * KIBI)

#if (TARGET_OS_OSX && __MAC_OS_X_VERSION_MAX_ALLOWED < 140000) || (TARGET_OS_IPHONE && __IPHONE_OS_VERSION_MAX_ALLOWED < 170000)
constexpr MTL::GPUFamily GPUFamilyApple9 = static_cast<MTL::GPUFamily>(1009);
#else
constexpr MTL::GPUFamily GPUFamilyApple9 = MTL::GPUFamilyApple9;
#endif

API_AVAILABLE(macos(11.0), ios(14.0), tvos(14.0), visionos(1.0))
MTL::GPUFamily &operator--(MTL::GPUFamily &p_family) {
	p_family = static_cast<MTL::GPUFamily>(static_cast<int>(p_family) - 1);
	if (p_family < MTL::GPUFamilyApple1) {
		p_family = GPUFamilyApple9;
	}

	return p_family;
}

void MetalDeviceProperties::init_features(MTL::Device *p_device) {
	features = {};

	MTL::CompileOptions *opts = MTL::CompileOptions::alloc()->init();
	MTL::LanguageVersion lang_version = opts->languageVersion();
	features.msl_max_version = make_msl_version((static_cast<uint32_t>(lang_version) >> 0x10) & 0xff, (static_cast<uint32_t>(lang_version) >> 0x00) & 0xff);
	features.msl_target_version = features.msl_max_version;
	opts->release();
	if (String version = OS::get_environment("GODOT_MTL_TARGET_VERSION"); !version.is_empty()) {
		if (version != "max") {
			Vector<String> parts = version.split(".", true, 2);
			if (parts.size() == 2) {
				uint32_t major = parts[0].to_int();
				uint32_t minor = parts[1].to_int();
				uint32_t msl_version = make_msl_version(major, minor);
				if (msl_version < MSL_VERSION_23 || msl_version > MSL_VERSION_40) {
					WARN_PRINT(vformat("GODOT_MTL_TARGET_VERSION: invalid MSL version '%d.%d'", major, minor));
				} else {
					print_line(vformat("Override: Targeting Metal version %d.%d", major, minor));
					features.msl_target_version = msl_version;
				}
			} else {
				WARN_PRINT("GODOT_MTL_TARGET_VERSION: invalid version string format. Expected major.minor or 'max'.");
			}
		}
	}

	features.highestFamily = MTL::GPUFamilyApple1;
	for (MTL::GPUFamily family = GPUFamilyApple9; family >= MTL::GPUFamilyApple1; --family) {
		if (p_device->supportsFamily(family)) {
			features.highestFamily = family;
			break;
		}
	}

	if (__builtin_available(macOS 11, iOS 16.4, tvOS 16.4, *)) {
		features.supportsBCTextureCompression = p_device->supportsBCTextureCompression();
	} else {
		features.supportsBCTextureCompression = false;
	}

#if TARGET_OS_OSX
	features.supportsDepth24Stencil8 = p_device->isDepth24Stencil8PixelFormatSupported();
#endif

	if (__builtin_available(macOS 11.0, iOS 14.0, tvOS 14.0, *)) {
		features.supports32BitFloatFiltering = p_device->supports32BitFloatFiltering();
		features.supports32BitMSAA = p_device->supports32BitMSAA();
	}

	if (__builtin_available(macOS 13.0, iOS 16.0, tvOS 16.0, *)) {
		features.supports_gpu_address = true;
	}

	features.hostMemoryPageSize = sysconf(_SC_PAGESIZE);

	for (SampleCount sc = SampleCount1; sc <= SampleCount64; sc <<= 1) {
		if (p_device->supportsTextureSampleCount(sc)) {
			features.supportedSampleCounts |= sc;
		}
	}

	features.layeredRendering = p_device->supportsFamily(MTL::GPUFamilyApple5);
	features.multisampleLayeredRendering = p_device->supportsFamily(MTL::GPUFamilyApple7);
	features.tessellationShader = p_device->supportsFamily(MTL::GPUFamilyApple3);
	features.imageCubeArray = p_device->supportsFamily(MTL::GPUFamilyApple3);
	features.quadPermute = p_device->supportsFamily(MTL::GPUFamilyApple4);
	features.simdPermute = p_device->supportsFamily(MTL::GPUFamilyApple6);
	features.simdReduction = p_device->supportsFamily(MTL::GPUFamilyApple7);
	features.supports_border_color = p_device->supportsFamily(MTL::GPUFamilyApple7);
	features.argument_buffers_tier = p_device->argumentBuffersSupport();
	features.supports_image_atomic_32_bit = p_device->supportsFamily(MTL::GPUFamilyApple6);
	features.supports_image_atomic_64_bit = p_device->supportsFamily(GPUFamilyApple9) || (p_device->supportsFamily(MTL::GPUFamilyApple8) && p_device->supportsFamily(MTL::GPUFamilyMac2));
	features.supports_msaa_depth_resolve = p_device->supportsFamily(MTL::GPUFamilyMetal3) || p_device->supportsFamily(MTL::GPUFamilyApple3) || p_device->supportsFamily(MTL::GPUFamilyMac2);

	if (features.msl_target_version >= MSL_VERSION_31) {
		// Native atomics are only supported on 3.1 and above.
		if (__builtin_available(macOS 14.0, iOS 17.0, tvOS 17.0, visionOS 1.0, *)) {
			features.supports_native_image_atomics = true;
		}
	}

	if (OS::get_environment("GODOT_MTL_DISABLE_IMAGE_ATOMICS") == "1") {
		features.supports_native_image_atomics = false;
	}

	if (__builtin_available(macOS 15.0, iOS 18.0, tvOS 18.0, visionOS 2.0, *)) {
		features.supports_residency_sets = p_device->supportsFamily(MTL::GPUFamilyApple6);
	} else {
		features.supports_residency_sets = false;
	}

	if (__builtin_available(macOS 13.0, iOS 16.0, tvOS 16.0, *)) {
		features.needs_arg_encoders = !(p_device->supportsFamily(MTL::GPUFamilyMetal3) && features.argument_buffers_tier == MTL::ArgumentBuffersTier2);
	}

	if (String v = OS::get_environment("GODOT_MTL_DISABLE_ARGUMENT_BUFFERS"); v == "1") {
		features.use_argument_buffers = false;
	}

	if (__builtin_available(macOS 13.0, iOS 16.0, tvOS 16.0, *)) {
		features.metal_fx_spatial = MTLFX::SpatialScalerDescriptor::supportsDevice(p_device);
#ifdef METAL_MFXTEMPORAL_ENABLED
		features.metal_fx_temporal = MTLFX::TemporalScalerDescriptor::supportsDevice(p_device);
#else
		features.metal_fx_temporal = false;
#endif
	}
}

void MetalDeviceProperties::init_limits(MTL::Device *p_device) {
	using std::max;
	using std::min;

	// FST: https://developer.apple.com/metal/Metal-Feature-Set-Tables.pdf

	// FST: Maximum number of layers per 1D texture array, 2D texture array, or 3D texture.
	limits.maxImageArrayLayers = 2048;
	if (p_device->supportsFamily(MTL::GPUFamilyApple3)) {
		// FST: Maximum 2D texture width and height.
		limits.maxFramebufferWidth = 16384;
		limits.maxFramebufferHeight = 16384;
		limits.maxViewportDimensionX = 16384;
		limits.maxViewportDimensionY = 16384;
		// FST: Maximum 1D texture width.
		limits.maxImageDimension1D = 16384;
		// FST: Maximum 2D texture width and height.
		limits.maxImageDimension2D = 16384;
		// FST: Maximum cube map texture width and height.
		limits.maxImageDimensionCube = 16384;
	} else {
		// FST: Maximum 2D texture width and height.
		limits.maxFramebufferWidth = 8192;
		limits.maxFramebufferHeight = 8192;
		limits.maxViewportDimensionX = 8192;
		limits.maxViewportDimensionY = 8192;
		// FST: Maximum 1D texture width.
		limits.maxImageDimension1D = 8192;
		// FST: Maximum 2D texture width and height.
		limits.maxImageDimension2D = 8192;
		// FST: Maximum cube map texture width and height.
		limits.maxImageDimensionCube = 8192;
	}
	// FST: Maximum 3D texture width, height, and depth.
	limits.maxImageDimension3D = 2048;

	limits.maxThreadsPerThreadGroup = p_device->maxThreadsPerThreadgroup();
	// No effective limits.
	limits.maxComputeWorkGroupCount = { std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint32_t>::max() };
	// https://github.com/KhronosGroup/MoltenVK/blob/568cc3acc0e2299931fdaecaaa1fc3ec5b4af281/MoltenVK/MoltenVK/GPUObjects/MVKDevice.h#L85
	limits.maxBoundDescriptorSets = SPIRV_CROSS_NAMESPACE::kMaxArgumentBuffers;
	// FST: Maximum number of color render targets per render pass descriptor.
	limits.maxColorAttachments = 8;

	// Maximum number of textures the device can access, per stage, from an argument buffer.
	if (p_device->supportsFamily(MTL::GPUFamilyApple6)) {
		limits.maxTexturesPerArgumentBuffer = 1'000'000;
	} else if (p_device->supportsFamily(MTL::GPUFamilyApple4)) {
		limits.maxTexturesPerArgumentBuffer = 96;
	} else {
		limits.maxTexturesPerArgumentBuffer = 31;
	}

	// Maximum number of samplers the device can access, per stage, from an argument buffer.
	if (p_device->supportsFamily(MTL::GPUFamilyApple6)) {
		limits.maxSamplersPerArgumentBuffer = 1024;
	} else {
		limits.maxSamplersPerArgumentBuffer = 16;
	}

	// Maximum number of buffers the device can access, per stage, from an argument buffer.
	if (p_device->supportsFamily(MTL::GPUFamilyApple6)) {
		limits.maxBuffersPerArgumentBuffer = std::numeric_limits<uint64_t>::max();
	} else if (p_device->supportsFamily(MTL::GPUFamilyApple4)) {
		limits.maxBuffersPerArgumentBuffer = 96;
	} else {
		limits.maxBuffersPerArgumentBuffer = 31;
	}

	limits.minSubgroupSize = limits.maxSubgroupSize = 1;
	// These values were taken from MoltenVK.
	if (features.simdPermute) {
		limits.minSubgroupSize = 4;
		limits.maxSubgroupSize = 32;
	} else if (features.quadPermute) {
		limits.minSubgroupSize = limits.maxSubgroupSize = 4;
	}

	limits.subgroupSupportedShaderStages.set_flag(RDD::ShaderStage::SHADER_STAGE_COMPUTE_BIT);
	if (features.tessellationShader) {
		limits.subgroupSupportedShaderStages.set_flag(RDD::ShaderStage::SHADER_STAGE_TESSELATION_CONTROL_BIT);
	}
	limits.subgroupSupportedShaderStages.set_flag(RDD::ShaderStage::SHADER_STAGE_FRAGMENT_BIT);

	limits.subgroupSupportedOperations.set_flag(RDD::SubgroupOperations::SUBGROUP_BASIC_BIT);
	if (features.simdPermute || features.quadPermute) {
		limits.subgroupSupportedOperations.set_flag(RDD::SubgroupOperations::SUBGROUP_VOTE_BIT);
		limits.subgroupSupportedOperations.set_flag(RDD::SubgroupOperations::SUBGROUP_BALLOT_BIT);
		limits.subgroupSupportedOperations.set_flag(RDD::SubgroupOperations::SUBGROUP_SHUFFLE_BIT);
		limits.subgroupSupportedOperations.set_flag(RDD::SubgroupOperations::SUBGROUP_SHUFFLE_RELATIVE_BIT);
	}

	if (features.simdReduction) {
		limits.subgroupSupportedOperations.set_flag(RDD::SubgroupOperations::SUBGROUP_ARITHMETIC_BIT);
	}

	if (features.quadPermute) {
		limits.subgroupSupportedOperations.set_flag(RDD::SubgroupOperations::SUBGROUP_QUAD_BIT);
	}

	limits.maxBufferLength = p_device->maxBufferLength();

	// FST: Maximum size of vertex descriptor layout stride.
	limits.maxVertexDescriptorLayoutStride = std::numeric_limits<uint64_t>::max();

	// Maximum number of viewports.
	if (p_device->supportsFamily(MTL::GPUFamilyApple5)) {
		limits.maxViewports = 16;
	} else {
		limits.maxViewports = 1;
	}

	limits.maxPerStageBufferCount = 31;
	limits.maxPerStageSamplerCount = 16;
	if (p_device->supportsFamily(MTL::GPUFamilyApple6)) {
		limits.maxPerStageTextureCount = 128;
	} else if (p_device->supportsFamily(MTL::GPUFamilyApple4)) {
		limits.maxPerStageTextureCount = 96;
	} else {
		limits.maxPerStageTextureCount = 31;
	}

	limits.maxVertexInputAttributes = 31;
	limits.maxVertexInputBindings = 31;
	limits.maxVertexInputBindingStride = (2 * KIBI);
	limits.maxShaderVaryings = 31; // Accurate on Apple4 and above. See: https://developer.apple.com/metal/Metal-Feature-Set-Tables.pdf

	if (p_device->supportsFamily(MTL::GPUFamilyApple4)) {
		limits.maxThreadGroupMemoryAllocation = 32768;
	} else if (p_device->supportsFamily(MTL::GPUFamilyApple3)) {
		limits.maxThreadGroupMemoryAllocation = 16384;
	} else {
		limits.maxThreadGroupMemoryAllocation = 16352;
	}

#if TARGET_OS_IOS && !TARGET_OS_MACCATALYST
	limits.minUniformBufferOffsetAlignment = 64;
#endif

#if TARGET_OS_OSX
	// This is Apple Silicon specific.
	limits.minUniformBufferOffsetAlignment = 16;
#endif

	limits.maxDrawIndexedIndexValue = std::numeric_limits<uint32_t>::max() - 1;

#ifdef METAL_MFXTEMPORAL_ENABLED
	if (__builtin_available(macOS 14.0, iOS 17.0, tvOS 17.0, *)) {
		limits.temporalScalerInputContentMinScale = MTLFX::TemporalScalerDescriptor::supportedInputContentMinScale(p_device);
		limits.temporalScalerInputContentMaxScale = MTLFX::TemporalScalerDescriptor::supportedInputContentMaxScale(p_device);
	} else {
		// Defaults taken from macOS 14+
		limits.temporalScalerInputContentMinScale = 1.0;
		limits.temporalScalerInputContentMaxScale = 3.0;
	}
#else
	// Defaults taken from macOS 14+
	limits.temporalScalerInputContentMinScale = 1.0;
	limits.temporalScalerInputContentMaxScale = 3.0;
#endif
}

void MetalDeviceProperties::init_os_props() {
	NS::OperatingSystemVersion ver = NS::ProcessInfo::processInfo()->operatingSystemVersion();
	os_version = (uint32_t)ver.majorVersion * 10000 + (uint32_t)ver.minorVersion * 100 + (uint32_t)ver.patchVersion;
}

MetalDeviceProperties::MetalDeviceProperties(MTL::Device *p_device) {
	init_features(p_device);
	init_limits(p_device);
	init_os_props();
}

MetalDeviceProperties::~MetalDeviceProperties() {
}

SampleCount MetalDeviceProperties::find_nearest_supported_sample_count(RenderingDeviceCommons::TextureSamples p_samples) const {
	SampleCount supported = features.supportedSampleCounts;
	if (supported & sample_count[p_samples]) {
		return sample_count[p_samples];
	}

	SampleCount requested_sample_count = sample_count[p_samples];
	// Find the nearest supported sample count.
	while (requested_sample_count > SampleCount1) {
		if (supported & requested_sample_count) {
			return requested_sample_count;
		}
		requested_sample_count = (SampleCount)(requested_sample_count >> 1);
	}

	return SampleCount1;
}

// region static members

const SampleCount MetalDeviceProperties::sample_count[RenderingDeviceCommons::TextureSamples::TEXTURE_SAMPLES_MAX] = {
	SampleCount1,
	SampleCount2,
	SampleCount4,
	SampleCount8,
	SampleCount16,
	SampleCount32,
	SampleCount64,
};

// endregion


// --- Merged from drivers/metal/metal_objects_shared.cpp ---
#include "drivers/metal/rendering_device_driver_metal3.h"


#include <os/signpost.h>
#include <simd/simd.h>

#include <shared_mutex>
#include <string>

#pragma mark - Resource Factory

NS::SharedPtr<MTL::Function> MDResourceFactory::new_func(NS::String *p_source, NS::String *p_name, NS::Error **p_error) {
	NS::SharedPtr<NS::AutoreleasePool> pool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());
	NS::SharedPtr<MTL::CompileOptions> options = NS::TransferPtr(MTL::CompileOptions::alloc()->init());
	NS::Error *err = nullptr;
	NS::SharedPtr<MTL::Library> mtlLib = NS::TransferPtr(device->newLibrary(p_source, options.get(), &err));
	if (err) {
		if (p_error != nullptr) {
			*p_error = err;
		}
	}
	return NS::TransferPtr(mtlLib->newFunction(p_name));
}

NS::SharedPtr<MTL::Function> MDResourceFactory::new_clear_vert_func(ClearAttKey &p_key) {
	NS::SharedPtr<NS::AutoreleasePool> pool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());
	char msl[1024];
	snprintf(msl, sizeof(msl), R"(
#include <metal_stdlib>
using namespace metal;

typedef struct {
    float4 a_position [[attribute(0)]];
} AttributesPos;

typedef struct {
    float4 colors[9];
} ClearColorsIn;

typedef struct {
    float4 v_position [[position]];
    uint layer%s;
} VaryingsPos;

vertex VaryingsPos vertClear(AttributesPos attributes [[stage_in]], constant ClearColorsIn& ccIn [[buffer(0)]]) {
    VaryingsPos varyings;
    varyings.v_position = float4(attributes.a_position.x, -attributes.a_position.y, ccIn.colors[%d].r, 1.0);
    varyings.layer = uint(attributes.a_position.w);
    return varyings;
}
)",
			p_key.is_layered_rendering_enabled() ? " [[render_target_array_index]]" : "", ClearAttKey::DEPTH_INDEX);

	return new_func(NS::String::string(msl, NS::UTF8StringEncoding), MTLSTR("vertClear"), nullptr);
}

NS::SharedPtr<MTL::Function> MDResourceFactory::new_clear_frag_func(ClearAttKey &p_key) {
	NS::SharedPtr<NS::AutoreleasePool> pool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());
	std::string msl;
	msl.reserve(2048);

	msl += R"(
#include <metal_stdlib>
using namespace metal;

typedef struct {
    float4 v_position [[position]];
} VaryingsPos;

typedef struct {
    float4 colors[9];
} ClearColorsIn;

typedef struct {
)";

	char line[128];
	for (uint32_t caIdx = 0; caIdx < ClearAttKey::COLOR_COUNT; caIdx++) {
		if (p_key.is_enabled(caIdx)) {
			const char *typeStr = get_format_type_string((MTL::PixelFormat)p_key.pixel_formats[caIdx]);
			snprintf(line, sizeof(line), "    %s4 color%u [[color(%u)]];\n", typeStr, caIdx, caIdx);
			msl += line;
		}
	}
	msl += R"(} ClearColorsOut;

fragment ClearColorsOut fragClear(VaryingsPos varyings [[stage_in]], constant ClearColorsIn& ccIn [[buffer(0)]]) {

    ClearColorsOut ccOut;
)";
	for (uint32_t caIdx = 0; caIdx < ClearAttKey::COLOR_COUNT; caIdx++) {
		if (p_key.is_enabled(caIdx)) {
			const char *typeStr = get_format_type_string((MTL::PixelFormat)p_key.pixel_formats[caIdx]);
			snprintf(line, sizeof(line), "    ccOut.color%u = %s4(ccIn.colors[%u]);\n", caIdx, typeStr, caIdx);
			msl += line;
		}
	}
	msl += R"(    return ccOut;
})";

	return new_func(NS::String::string(msl.c_str(), NS::UTF8StringEncoding), MTLSTR("fragClear"), nullptr);
}

const char *MDResourceFactory::get_format_type_string(MTL::PixelFormat p_fmt) const {
	switch (pixel_formats.getFormatType(p_fmt)) {
		case MTLFormatType::ColorInt8:
		case MTLFormatType::ColorInt16:
			return "short";
		case MTLFormatType::ColorUInt8:
		case MTLFormatType::ColorUInt16:
			return "ushort";
		case MTLFormatType::ColorInt32:
			return "int";
		case MTLFormatType::ColorUInt32:
			return "uint";
		case MTLFormatType::ColorHalf:
			return "half";
		case MTLFormatType::ColorFloat:
		case MTLFormatType::DepthStencil:
		case MTLFormatType::Compressed:
			return "float";
		case MTLFormatType::None:
		default:
			return "unexpected_MTLPixelFormatInvalid";
	}
}

NS::SharedPtr<MTL::DepthStencilState> MDResourceFactory::new_depth_stencil_state(bool p_use_depth, bool p_use_stencil) {
	NS::SharedPtr<MTL::DepthStencilDescriptor> dsDesc = NS::TransferPtr(MTL::DepthStencilDescriptor::alloc()->init());
	dsDesc->setDepthCompareFunction(MTL::CompareFunctionAlways);
	dsDesc->setDepthWriteEnabled(p_use_depth);

	if (p_use_stencil) {
		NS::SharedPtr<MTL::StencilDescriptor> sDesc = NS::TransferPtr(MTL::StencilDescriptor::alloc()->init());
		sDesc->setStencilCompareFunction(MTL::CompareFunctionAlways);
		sDesc->setStencilFailureOperation(MTL::StencilOperationReplace);
		sDesc->setDepthFailureOperation(MTL::StencilOperationReplace);
		sDesc->setDepthStencilPassOperation(MTL::StencilOperationReplace);

		dsDesc->setFrontFaceStencil(sDesc.get());
		dsDesc->setBackFaceStencil(sDesc.get());
	} else {
		dsDesc->setFrontFaceStencil(nullptr);
		dsDesc->setBackFaceStencil(nullptr);
	}

	return NS::TransferPtr(device->newDepthStencilState(dsDesc.get()));
}

NS::SharedPtr<MTL::RenderPipelineState> MDResourceFactory::new_clear_pipeline_state(ClearAttKey &p_key, NS::Error **p_error) {
	NS::SharedPtr<MTL::Function> vtxFunc = new_clear_vert_func(p_key);
	NS::SharedPtr<MTL::Function> fragFunc = new_clear_frag_func(p_key);
	NS::SharedPtr<MTL::RenderPipelineDescriptor> plDesc = NS::TransferPtr(MTL::RenderPipelineDescriptor::alloc()->init());
	plDesc->setLabel(MTLSTR("ClearRenderAttachments"));
	plDesc->setVertexFunction(vtxFunc.get());
	plDesc->setFragmentFunction(fragFunc.get());
	plDesc->setRasterSampleCount(p_key.sample_count);
	plDesc->setInputPrimitiveTopology(MTL::PrimitiveTopologyClassTriangle);

	for (uint32_t caIdx = 0; caIdx < ClearAttKey::COLOR_COUNT; caIdx++) {
		MTL::RenderPipelineColorAttachmentDescriptor *colorDesc = plDesc->colorAttachments()->object(caIdx);
		colorDesc->setPixelFormat((MTL::PixelFormat)p_key.pixel_formats[caIdx]);
		colorDesc->setWriteMask(p_key.is_enabled(caIdx) ? MTL::ColorWriteMaskAll : MTL::ColorWriteMaskNone);
	}

	MTL::PixelFormat mtlDepthFormat = (MTL::PixelFormat)p_key.depth_format();
	if (pixel_formats.isDepthFormat(mtlDepthFormat)) {
		plDesc->setDepthAttachmentPixelFormat(mtlDepthFormat);
	}

	MTL::PixelFormat mtlStencilFormat = (MTL::PixelFormat)p_key.stencil_format();
	if (pixel_formats.isStencilFormat(mtlStencilFormat)) {
		plDesc->setStencilAttachmentPixelFormat(mtlStencilFormat);
	}

	MTL::VertexDescriptor *vtxDesc = plDesc->vertexDescriptor();

	// Vertex attribute descriptors.
	NS::UInteger vtxBuffIdx = get_vertex_buffer_index(VERT_CONTENT_BUFFER_INDEX);
	NS::UInteger vtxStride = 0;

	// Vertex location.
	MTL::VertexAttributeDescriptor *vaDesc = vtxDesc->attributes()->object(0);
	vaDesc->setFormat(MTL::VertexFormatFloat4);
	vaDesc->setBufferIndex(vtxBuffIdx);
	vaDesc->setOffset(vtxStride);
	vtxStride += sizeof(simd::float4);

	// Vertex attribute buffer.
	MTL::VertexBufferLayoutDescriptor *vbDesc = vtxDesc->layouts()->object(vtxBuffIdx);
	vbDesc->setStepFunction(MTL::VertexStepFunctionPerVertex);
	vbDesc->setStepRate(1);
	vbDesc->setStride(vtxStride);

	NS::Error *err = nullptr;
	NS::SharedPtr<MTL::RenderPipelineState> state = NS::TransferPtr(device->newRenderPipelineState(plDesc.get(), &err));
	if (p_error != nullptr) {
		*p_error = err;
	}
	return state;
}

NS::SharedPtr<MTL::RenderPipelineState> MDResourceFactory::new_empty_draw_pipeline_state(ClearAttKey &p_key, NS::Error **p_error) {
	DEV_ASSERT(!p_key.is_layered_rendering_enabled());
	DEV_ASSERT(p_key.is_enabled(0));
	DEV_ASSERT(!p_key.is_depth_enabled());
	DEV_ASSERT(!p_key.is_stencil_enabled());

	NS::SharedPtr<NS::AutoreleasePool> pool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());
	static const char *msl = R"(#include <metal_stdlib>
using namespace metal;

struct FullscreenNoopOut {
    float4 position [[position]];
};

vertex FullscreenNoopOut fullscreenNoopVert(uint vid [[vertex_id]]) {
    float2 positions[3] = { float2(-1.0, -1.0), float2(3.0, -1.0), float2(-1.0, 3.0) };
    float2 pos = positions[vid];

    FullscreenNoopOut out;
    out.position = float4(pos, 0.0, 1.0);
    return out;
}

fragment void fullscreenNoopFrag(float4 gl_FragCoord [[position]]) {
}
)";

	NS::Error *err = nullptr;
	NS::SharedPtr<MTL::CompileOptions> options = NS::TransferPtr(MTL::CompileOptions::alloc()->init());
	NS::SharedPtr<MTL::Library> mtlLib = NS::TransferPtr(device->newLibrary(NS::String::string(msl, NS::UTF8StringEncoding), options.get(), &err));
	if (err && p_error != nullptr) {
		*p_error = err;
	}

	if (mtlLib.get() == nullptr) {
		return {};
	}

	NS::SharedPtr<MTL::Function> vtxFunc = NS::TransferPtr(mtlLib->newFunction(MTLSTR("fullscreenNoopVert")));
	NS::SharedPtr<MTL::Function> fragFunc = NS::TransferPtr(mtlLib->newFunction(MTLSTR("fullscreenNoopFrag")));

	NS::SharedPtr<MTL::RenderPipelineDescriptor> plDesc = NS::TransferPtr(MTL::RenderPipelineDescriptor::alloc()->init());
	plDesc->setLabel(MTLSTR("EmptyDrawFullscreenTriangle"));
	plDesc->setVertexFunction(vtxFunc.get());
	plDesc->setFragmentFunction(fragFunc.get());
	plDesc->setRasterSampleCount(p_key.sample_count ? p_key.sample_count : 1);
	plDesc->setInputPrimitiveTopology(MTL::PrimitiveTopologyClassTriangle);

	MTL::RenderPipelineColorAttachmentDescriptor *colorDesc = plDesc->colorAttachments()->object(0);
	colorDesc->setPixelFormat((MTL::PixelFormat)p_key.pixel_formats[0]);
	colorDesc->setWriteMask(MTL::ColorWriteMaskNone);

	err = nullptr;
	NS::SharedPtr<MTL::RenderPipelineState> state = NS::TransferPtr(device->newRenderPipelineState(plDesc.get(), &err));
	if (p_error != nullptr && err != nullptr) {
		*p_error = err;
	}
	return state;
}

#pragma mark - Resource Cache

MTL::RenderPipelineState *MDResourceCache::get_clear_render_pipeline_state(ClearAttKey &p_key, NS::Error **p_error) {
	HashMap::ConstIterator it = clear_states.find(p_key);
	if (it != clear_states.end()) {
		return it->value.get();
	}

	NS::SharedPtr<MTL::RenderPipelineState> state = resource_factory->new_clear_pipeline_state(p_key, p_error);
	MTL::RenderPipelineState *result = state.get();
	clear_states[p_key] = std::move(state);
	return result;
}

MTL::RenderPipelineState *MDResourceCache::get_empty_draw_pipeline_state(ClearAttKey &p_key, NS::Error **p_error) {
	HashMap::ConstIterator it = empty_draw_states.find(p_key);
	if (it != empty_draw_states.end()) {
		return it->value.get();
	}

	NS::SharedPtr<MTL::RenderPipelineState> state = resource_factory->new_empty_draw_pipeline_state(p_key, p_error);
	MTL::RenderPipelineState *result = state.get();
	empty_draw_states[p_key] = std::move(state);
	return result;
}

MTL::DepthStencilState *MDResourceCache::get_depth_stencil_state(bool p_use_depth, bool p_use_stencil) {
	if (p_use_depth && p_use_stencil) {
		if (!clear_depth_stencil_state.all) {
			clear_depth_stencil_state.all = resource_factory->new_depth_stencil_state(true, true);
		}
		return clear_depth_stencil_state.all.get();
	} else if (p_use_depth) {
		if (!clear_depth_stencil_state.depth_only) {
			clear_depth_stencil_state.depth_only = resource_factory->new_depth_stencil_state(true, false);
		}
		return clear_depth_stencil_state.depth_only.get();
	} else if (p_use_stencil) {
		if (!clear_depth_stencil_state.stencil_only) {
			clear_depth_stencil_state.stencil_only = resource_factory->new_depth_stencil_state(false, true);
		}
		return clear_depth_stencil_state.stencil_only.get();
	} else {
		if (!clear_depth_stencil_state.none) {
			clear_depth_stencil_state.none = resource_factory->new_depth_stencil_state(false, false);
		}
		return clear_depth_stencil_state.none.get();
	}
}

#pragma mark - Render Pass Types

MTLFmtCaps MDSubpass::getRequiredFmtCapsForAttachmentAt(uint32_t p_index) const {
	MTLFmtCaps caps = kMTLFmtCapsNone;

	for (const RDD::AttachmentReference &ar : input_references) {
		if (ar.attachment == p_index) {
			flags::set(caps, kMTLFmtCapsRead);
			break;
		}
	}

	for (const RDD::AttachmentReference &ar : color_references) {
		if (ar.attachment == p_index) {
			flags::set(caps, kMTLFmtCapsColorAtt);
			break;
		}
	}

	for (const RDD::AttachmentReference &ar : resolve_references) {
		if (ar.attachment == p_index) {
			flags::set(caps, kMTLFmtCapsResolve);
			break;
		}
	}

	if (depth_stencil_reference.attachment == p_index) {
		flags::set(caps, kMTLFmtCapsDSAtt);
	}

	return caps;
}

void MDAttachment::linkToSubpass(const MDRenderPass &p_pass) {
	firstUseSubpassIndex = UINT32_MAX;
	lastUseSubpassIndex = 0;

	for (const MDSubpass &subpass : p_pass.subpasses) {
		MTLFmtCaps reqCaps = subpass.getRequiredFmtCapsForAttachmentAt(index);
		if (reqCaps) {
			firstUseSubpassIndex = MIN(subpass.subpass_index, firstUseSubpassIndex);
			lastUseSubpassIndex = MAX(subpass.subpass_index, lastUseSubpassIndex);
		}
	}
}

MTL::StoreAction MDAttachment::getMTLStoreAction(const MDSubpass &p_subpass,
		bool p_is_rendering_entire_area,
		bool p_has_resolve,
		bool p_can_resolve,
		bool p_is_stencil) const {
	if (!p_is_rendering_entire_area || !isLastUseOf(p_subpass)) {
		return p_has_resolve && p_can_resolve ? MTL::StoreActionStoreAndMultisampleResolve : MTL::StoreActionStore;
	}

	switch (p_is_stencil ? stencilStoreAction : storeAction) {
		case MTL::StoreActionStore:
			return p_has_resolve && p_can_resolve ? MTL::StoreActionStoreAndMultisampleResolve : MTL::StoreActionStore;
		case MTL::StoreActionDontCare:
			return p_has_resolve ? (p_can_resolve ? MTL::StoreActionMultisampleResolve : MTL::StoreActionStore) : MTL::StoreActionDontCare;

		default:
			return MTL::StoreActionStore;
	}
}

bool MDAttachment::shouldClear(const MDSubpass &p_subpass, bool p_is_stencil) const {
	// If the subpass is not the first subpass to use this attachment, don't clear this attachment.
	if (p_subpass.subpass_index != firstUseSubpassIndex) {
		return false;
	}
	return (p_is_stencil ? stencilLoadAction : loadAction) == MTL::LoadActionClear;
}

MDRenderPass::MDRenderPass(Vector<MDAttachment> &p_attachments, Vector<MDSubpass> &p_subpasses) :
		attachments(p_attachments), subpasses(p_subpasses) {
	for (MDAttachment &att : attachments) {
		att.linkToSubpass(*this);
	}
}

#pragma mark - Command Buffer Base

void MDCommandBufferBase::retain_resource(CFTypeRef p_resource) {
	CFRetain(p_resource);
	_retained_resources.push_back(p_resource);
}

void MDCommandBufferBase::release_resources() {
	for (CFTypeRef r : _retained_resources) {
		CFRelease(r);
	}
	_retained_resources.clear();
}

void MDCommandBufferBase::render_set_viewport(VectorView<Rect2i> p_viewports) {
	RenderStateBase &state = get_render_state_base();
	state.viewports.resize(p_viewports.size());
	for (uint32_t i = 0; i < p_viewports.size(); i += 1) {
		const Rect2i &vp = p_viewports[i];
		state.viewports[i] = {
			.originX = static_cast<double>(vp.position.x),
			.originY = static_cast<double>(vp.position.y),
			.width = static_cast<double>(vp.size.width),
			.height = static_cast<double>(vp.size.height),
			.znear = 0.0,
			.zfar = 1.0,
		};
	}
	state.dirty.set_flag(RenderStateBase::DIRTY_VIEWPORT);
}

void MDCommandBufferBase::render_set_scissor(VectorView<Rect2i> p_scissors) {
	RenderStateBase &state = get_render_state_base();
	state.scissors.resize(p_scissors.size());
	for (uint32_t i = 0; i < p_scissors.size(); i += 1) {
		const Rect2i &vp = p_scissors[i];
		state.scissors[i] = {
			.x = static_cast<NS::UInteger>(vp.position.x),
			.y = static_cast<NS::UInteger>(vp.position.y),
			.width = static_cast<NS::UInteger>(vp.size.width),
			.height = static_cast<NS::UInteger>(vp.size.height),
		};
	}
	state.dirty.set_flag(RenderStateBase::DIRTY_SCISSOR);
}

void MDCommandBufferBase::render_set_blend_constants(const Color &p_constants) {
	DEV_ASSERT(type == MDCommandBufferStateType::Render);
	RenderStateBase &state = get_render_state_base();
	if (state.blend_constants != p_constants) {
		state.blend_constants = p_constants;
		state.dirty.set_flag(RenderStateBase::DIRTY_BLEND);
	}
}

void MDCommandBufferBase::_populate_vertices(simd::float4 *p_vertices, Size2i p_fb_size, VectorView<Rect2i> p_rects) {
	uint32_t idx = 0;
	for (uint32_t i = 0; i < p_rects.size(); i++) {
		const Rect2i &rect = p_rects[i];
		idx = _populate_vertices(p_vertices, idx, rect, p_fb_size);
	}
}

uint32_t MDCommandBufferBase::_populate_vertices(simd::float4 *p_vertices, uint32_t p_index, const Rect2i &p_rect, Size2i p_fb_size) {
	// Determine the positions of the four edges of the
	// clear rectangle as a fraction of the attachment size.
	float leftPos = (float)(p_rect.position.x) / (float)p_fb_size.width;
	float rightPos = (float)(p_rect.size.width) / (float)p_fb_size.width + leftPos;
	float bottomPos = (float)(p_rect.position.y) / (float)p_fb_size.height;
	float topPos = (float)(p_rect.size.height) / (float)p_fb_size.height + bottomPos;

	// Transform to clip-space coordinates, which are bounded by (-1.0 < p < 1.0) in clip-space.
	leftPos = (leftPos * 2.0f) - 1.0f;
	rightPos = (rightPos * 2.0f) - 1.0f;
	bottomPos = (bottomPos * 2.0f) - 1.0f;
	topPos = (topPos * 2.0f) - 1.0f;

	simd::float4 vtx;

	uint32_t idx = p_index;
	uint32_t endLayer = get_current_view_count();

	for (uint32_t layer = 0; layer < endLayer; layer++) {
		vtx.z = 0.0;
		vtx.w = (float)layer;

		// Top left vertex - First triangle.
		vtx.y = topPos;
		vtx.x = leftPos;
		p_vertices[idx++] = vtx;

		// Bottom left vertex.
		vtx.y = bottomPos;
		vtx.x = leftPos;
		p_vertices[idx++] = vtx;

		// Bottom right vertex.
		vtx.y = bottomPos;
		vtx.x = rightPos;
		p_vertices[idx++] = vtx;

		// Bottom right vertex - Second triangle.
		p_vertices[idx++] = vtx;

		// Top right vertex.
		vtx.y = topPos;
		vtx.x = rightPos;
		p_vertices[idx++] = vtx;

		// Top left vertex.
		vtx.y = topPos;
		vtx.x = leftPos;
		p_vertices[idx++] = vtx;
	}

	return idx;
}

void MDCommandBufferBase::_end_render_pass() {
	const MDFrameBuffer &fb_info = *get_frame_buffer();
	const MDSubpass &subpass = get_current_subpass();

	PixelFormats &pf = device_driver->get_pixel_formats();

	for (uint32_t i = 0; i < subpass.resolve_references.size(); i++) {
		uint32_t color_index = subpass.color_references[i].attachment;
		uint32_t resolve_index = subpass.resolve_references[i].attachment;
		DEV_ASSERT((color_index == RDD::AttachmentReference::UNUSED) == (resolve_index == RDD::AttachmentReference::UNUSED));
		if (color_index == RDD::AttachmentReference::UNUSED || !fb_info.has_texture(color_index)) {
			continue;
		}

		MTL::Texture *resolve_tex = fb_info.get_texture(resolve_index);

		CRASH_COND_MSG(!flags::all(pf.getCapabilities(resolve_tex->pixelFormat()), kMTLFmtCapsResolve), "not implemented: unresolvable texture types");
		// see: https://github.com/KhronosGroup/MoltenVK/blob/d20d13fe2735adb845636a81522df1b9d89c0fba/MoltenVK/MoltenVK/GPUObjects/MVKRenderPass.mm#L407
	}

	end_render_encoding();
}

void MDCommandBufferBase::_render_clear_render_area() {
	const MDRenderPass &pass = *get_render_pass();
	const MDSubpass &subpass = get_current_subpass();
	LocalVector<RDD::RenderPassClearValue> &clear_values = get_clear_values();

	uint32_t ds_index = subpass.depth_stencil_reference.attachment;
	bool clear_depth = (ds_index != RDD::AttachmentReference::UNUSED && pass.attachments[ds_index].shouldClear(subpass, false));
	bool clear_stencil = (ds_index != RDD::AttachmentReference::UNUSED && pass.attachments[ds_index].shouldClear(subpass, true));

	uint32_t color_count = subpass.color_references.size();
	uint32_t clears_size = color_count + (clear_depth || clear_stencil ? 1 : 0);
	if (clears_size == 0) {
		return;
	}

	RDD::AttachmentClear *clears = ALLOCA_ARRAY(RDD::AttachmentClear, clears_size);
	uint32_t clears_count = 0;

	for (uint32_t i = 0; i < color_count; i++) {
		uint32_t idx = subpass.color_references[i].attachment;
		if (idx != RDD::AttachmentReference::UNUSED && pass.attachments[idx].shouldClear(subpass, false)) {
			clears[clears_count++] = { .aspect = RDD::TEXTURE_ASPECT_COLOR_BIT, .color_attachment = idx, .value = clear_values[idx] };
		}
	}

	if (clear_depth || clear_stencil) {
		const MDAttachment &attachment = pass.attachments[ds_index];
		uint32_t bits = {};
		if (clear_depth && attachment.type & MDAttachmentType::Depth) {
			bits.set_flag(RDD::TEXTURE_ASPECT_DEPTH_BIT);
		}
		if (clear_stencil && attachment.type & MDAttachmentType::Stencil) {
			bits.set_flag(RDD::TEXTURE_ASPECT_STENCIL_BIT);
		}

		clears[clears_count++] = { .aspect = bits, .color_attachment = ds_index, .value = clear_values[ds_index] };
	}

	if (clears_count == 0) {
		return;
	}

	render_clear_attachments(VectorView(clears, clears_count), { get_render_area() });
}

void MDCommandBufferBase::encode_push_constant_data(RDD::ShaderID p_shader, VectorView<uint32_t> p_data) {
	switch (type) {
		case MDCommandBufferStateType::Render:
		case MDCommandBufferStateType::Compute: {
			MDShader *shader = (MDShader *)(p_shader.id);
			if (shader->push_constants.binding == UINT32_MAX) {
				return;
			}
			push_constant_binding = shader->push_constants.binding;
			const void *ptr = p_data.ptr();
			push_constant_data_len = p_data.size() * sizeof(uint32_t);
			DEV_ASSERT(push_constant_data_len <= sizeof(push_constant_data));
			memcpy(push_constant_data, ptr, push_constant_data_len);
			if (push_constant_data_len > 0) {
				mark_push_constants_dirty();
			}
		} break;
		case MDCommandBufferStateType::Blit:
		case MDCommandBufferStateType::None:
			return;
	}
}

#pragma mark - Metal Library

static const char *SHADER_STAGE_NAMES[] = {
	"vert", // RDC::SHADER_STAGE_VERTEX
	"frag", // RDC::SHADER_STAGE_FRAGMENT
	"tess_ctrl", // RDC::SHADER_STAGE_TESSELATION_CONTROL
	"tess_eval", // RDC::SHADER_STAGE_TESSELATION_EVALUATION
	"comp", // RDC::SHADER_STAGE_COMPUTE
};

void ShaderCacheEntry::notify_free() {
	owner.shader_cache_free_entry(this);
}

#pragma mark - MDLibrary

MDLibrary::MDLibrary(ShaderCacheEntry *p_entry
#ifdef DEV_ENABLED
		,
		NS::String *p_source
#endif
		) :
		_entry(p_entry) {
#ifdef DEV_ENABLED
	_original_source = NS::RetainPtr(p_source);
#endif
}

MDLibrary::~MDLibrary() {
	_entry->notify_free();
}

void MDLibrary::set_label(NS::String *p_label) {
}

#pragma mark - MDLazyLibrary

/// Loads the MTLLibrary when the library is first accessed.
class MDLazyLibrary final : public MDLibrary {
	NS::SharedPtr<MTL::Library> _library;
	NS::Error *_error = nullptr;
	std::shared_mutex _mu;
	bool _loaded = false;
	MTL::Device *_device = nullptr;
	NS::SharedPtr<NS::String> _source;
	NS::SharedPtr<MTL::CompileOptions> _options;

	void _load();

public:
	MDLazyLibrary(ShaderCacheEntry *p_entry,
			MTL::Device *p_device,
			NS::String *p_source,
			MTL::CompileOptions *p_options);

	MTL::Library *get_library() override;
	NS::Error *get_error() override;
};

MDLazyLibrary::MDLazyLibrary(ShaderCacheEntry *p_entry,
		MTL::Device *p_device,
		NS::String *p_source,
		MTL::CompileOptions *p_options) :
		MDLibrary(p_entry
#ifdef DEV_ENABLED
				,
				p_source
#endif
				),
		_device(p_device),
		_source(NS::RetainPtr(p_source)),
		_options(NS::RetainPtr(p_options)) {
}

void MDLazyLibrary::_load() {
	{
		std::shared_lock<std::shared_mutex> lock(_mu);
		if (_loaded) {
			return;
		}
	}

	std::unique_lock<std::shared_mutex> lock(_mu);
	if (_loaded) {
		return;
	}

	os_signpost_id_t compile_id = (os_signpost_id_t)(uintptr_t)this;
	os_signpost_interval_begin(LOG_INTERVALS, compile_id, "shader_compile",
			"shader_name=%{public}s stage=%{public}s hash=%X",
			_entry->name.get_data(), SHADER_STAGE_NAMES[_entry->stage], _entry->key.short_sha());
	NS::Error *error = nullptr;
	_library = NS::TransferPtr(_device->newLibrary(_source.get(), _options.get(), &error));
	os_signpost_interval_end(LOG_INTERVALS, compile_id, "shader_compile");
	_error = error;
	_device = nullptr;
	_source.reset();
	_options.reset();
	_loaded = true;
}

MTL::Library *MDLazyLibrary::get_library() {
	_load();
	return _library.get();
}

NS::Error *MDLazyLibrary::get_error() {
	_load();
	return _error;
}

#pragma mark - MDImmediateLibrary

/// Loads the MTLLibrary immediately on initialization, using Metal's async compilation API.
class MDImmediateLibrary final : public MDLibrary {
	NS::SharedPtr<MTL::Library> _library;
	NS::Error *_error = nullptr;
	std::mutex _cv_mutex;
	std::condition_variable _cv;
	bool _complete = false;

	void _check_and_wait() {
		std::unique_lock<std::mutex> lock(_cv_mutex);
		_cv.wait(lock, [this] { return _complete; });
	}

public:
	MDImmediateLibrary(ShaderCacheEntry *p_entry,
			MTL::Device *p_device,
			NS::String *p_source,
			MTL::CompileOptions *p_options);
	~MDImmediateLibrary() override;

	MTL::Library *get_library() override;
	NS::Error *get_error() override;
};

MDImmediateLibrary::MDImmediateLibrary(ShaderCacheEntry *p_entry,
		MTL::Device *p_device,
		NS::String *p_source,
		MTL::CompileOptions *p_options) :
		MDLibrary(p_entry
#ifdef DEV_ENABLED
				,
				p_source
#endif
		) {
	os_signpost_id_t compile_id = (os_signpost_id_t)(uintptr_t)this;
	os_signpost_interval_begin(LOG_INTERVALS, compile_id, "shader_compile",
			"shader_name=%{public}s stage=%{public}s hash=%X",
			p_entry->name.get_data(), SHADER_STAGE_NAMES[p_entry->stage], p_entry->key.short_sha());

	// Use Metal's async compilation API with std::function callback.
	p_device->newLibrary(p_source, p_options, [this, compile_id, p_entry](MTL::Library *library, NS::Error *error) {
		os_signpost_interval_end(LOG_INTERVALS, compile_id, "shader_compile");
		if (library) {
			_library = NS::RetainPtr(library);
		}
		_error = error;
		if (error) {
			ERR_PRINT(vformat(U"Error compiling shader %s: %s", p_entry->name.get_data(), error->localizedDescription()->utf8String()));
		}

		std::lock_guard<std::mutex> lock(_cv_mutex);
		_complete = true;
		_cv.notify_all();
	});
}

MDImmediateLibrary::~MDImmediateLibrary() {
	// Wait for the async compilation callback to complete before destroying to avoid segfault.
	_check_and_wait();
}

MTL::Library *MDImmediateLibrary::get_library() {
	_check_and_wait();
	return _library.get();
}

NS::Error *MDImmediateLibrary::get_error() {
	_check_and_wait();
	return _error;
}

#pragma mark - MDBinaryLibrary

/// Loads the MTLLibrary from pre-compiled binary data.
class MDBinaryLibrary final : public MDLibrary {
	NS::SharedPtr<MTL::Library> _library;
	NS::Error *_error = nullptr;

public:
	MDBinaryLibrary(ShaderCacheEntry *p_entry,
			MTL::Device *p_device,
#ifdef DEV_ENABLED
			NS::String *p_source,
#endif
			dispatch_data_t p_data);

	MTL::Library *get_library() override;
	NS::Error *get_error() override;
};

MDBinaryLibrary::MDBinaryLibrary(ShaderCacheEntry *p_entry,
		MTL::Device *p_device,
#ifdef DEV_ENABLED
		NS::String *p_source,
#endif
		dispatch_data_t p_data) :
		MDLibrary(p_entry
#ifdef DEV_ENABLED
				,
				p_source
#endif
		) {
	NS::Error *error = nullptr;
	_library = NS::TransferPtr(p_device->newLibrary(p_data, &error));
	if (error != nullptr) {
		_error = error;
		ERR_PRINT(vformat("Unable to load shader library: %s", error->localizedDescription()->utf8String()));
	}
}

MTL::Library *MDBinaryLibrary::get_library() {
	return _library.get();
}

NS::Error *MDBinaryLibrary::get_error() {
	return _error;
}

#pragma mark - MDLibrary Factory Methods

std::shared_ptr<MDLibrary> MDLibrary::create(ShaderCacheEntry *p_entry,
		MTL::Device *p_device,
		NS::String *p_source,
		MTL::CompileOptions *p_options,
		ShaderLoadStrategy p_strategy) {
	std::shared_ptr<MDLibrary> lib;
	switch (p_strategy) {
		case ShaderLoadStrategy::IMMEDIATE:
			[[fallthrough]];
		default:
			lib = std::make_shared<MDImmediateLibrary>(p_entry, p_device, p_source, p_options);
			break;
		case ShaderLoadStrategy::LAZY:
			lib = std::make_shared<MDLazyLibrary>(p_entry, p_device, p_source, p_options);
			break;
	}
	p_entry->library = lib;
	return lib;
}

std::shared_ptr<MDLibrary> MDLibrary::create(ShaderCacheEntry *p_entry,
		MTL::Device *p_device,
#ifdef DEV_ENABLED
		NS::String *p_source,
#endif
		dispatch_data_t p_data) {
	std::shared_ptr<MDLibrary> lib = std::make_shared<MDBinaryLibrary>(p_entry, p_device,
#ifdef DEV_ENABLED
			p_source,
#endif
			p_data);
	p_entry->library = lib;
	return lib;
}


// --- Merged from drivers/metal/pixel_formats.cpp ---
#include "drivers/metal/rendering_device_driver_metal3.h"


#if TARGET_OS_IPHONE || TARGET_OS_TV
#if !(__IPHONE_OS_VERSION_MAX_ALLOWED >= 160400) // iOS/tvOS 16.4
#define PixelFormatBC1_RGBA PixelFormatInvalid
#define PixelFormatBC1_RGBA_sRGB PixelFormatInvalid
#define PixelFormatBC2_RGBA PixelFormatInvalid
#define PixelFormatBC2_RGBA_sRGB PixelFormatInvalid
#define PixelFormatBC3_RGBA PixelFormatInvalid
#define PixelFormatBC3_RGBA_sRGB PixelFormatInvalid
#define PixelFormatBC4_RUnorm PixelFormatInvalid
#define PixelFormatBC4_RSnorm PixelFormatInvalid
#define PixelFormatBC5_RGUnorm PixelFormatInvalid
#define PixelFormatBC5_RGSnorm PixelFormatInvalid
#define PixelFormatBC6H_RGBUfloat PixelFormatInvalid
#define PixelFormatBC6H_RGBFloat PixelFormatInvalid
#define PixelFormatBC7_RGBAUnorm PixelFormatInvalid
#define PixelFormatBC7_RGBAUnorm_sRGB PixelFormatInvalid
#endif

#define PixelFormatDepth16Unorm_Stencil8 PixelFormatDepth32Float_Stencil8
#define PixelFormatDepth24Unorm_Stencil8 PixelFormatInvalid
#define PixelFormatX24_Stencil8 PixelFormatInvalid
#endif

#if TARGET_OS_TV
#define PixelFormatASTC_4x4_HDR PixelFormatInvalid
#define PixelFormatASTC_5x4_HDR PixelFormatInvalid
#define PixelFormatASTC_5x5_HDR PixelFormatInvalid
#define PixelFormatASTC_6x5_HDR PixelFormatInvalid
#define PixelFormatASTC_6x6_HDR PixelFormatInvalid
#define PixelFormatASTC_8x5_HDR PixelFormatInvalid
#define PixelFormatASTC_8x6_HDR PixelFormatInvalid
#define PixelFormatASTC_8x8_HDR PixelFormatInvalid
#define PixelFormatASTC_10x5_HDR PixelFormatInvalid
#define PixelFormatASTC_10x6_HDR PixelFormatInvalid
#define PixelFormatASTC_10x8_HDR PixelFormatInvalid
#define PixelFormatASTC_10x10_HDR PixelFormatInvalid
#define PixelFormatASTC_12x10_HDR PixelFormatInvalid
#define PixelFormatASTC_12x12_HDR PixelFormatInvalid
#endif

#if !((__MAC_OS_X_VERSION_MAX_ALLOWED >= 140000) || (__IPHONE_OS_VERSION_MAX_ALLOWED >= 170000)) // Xcode 15
#define VertexFormatFloatRG11B10 VertexFormatInvalid
#define VertexFormatFloatRGB9E5 VertexFormatInvalid
#endif

template <typename T>
void clear(T *p_val, size_t p_count = 1) {
	memset(p_val, 0, sizeof(T) * p_count);
}

#pragma mark -
#pragma mark PixelFormats

bool PixelFormats::isSupported(DataFormat p_format) {
	return getDataFormatDesc(p_format).isSupported();
}

bool PixelFormats::isSupportedOrSubstitutable(DataFormat p_format) {
	return getDataFormatDesc(p_format).isSupportedOrSubstitutable();
}

bool PixelFormats::isPVRTCFormat(MTL::PixelFormat p_format) {
#if defined(VISIONOS_ENABLED)
	return false;
#else
	// Deprecated in SDK 26.0
	GODOT_CLANG_WARNING_PUSH_AND_IGNORE("-Wdeprecated-declarations")
	switch (p_format) {
		case MTL::PixelFormatPVRTC_RGBA_2BPP:
		case MTL::PixelFormatPVRTC_RGBA_2BPP_sRGB:
		case MTL::PixelFormatPVRTC_RGBA_4BPP:
		case MTL::PixelFormatPVRTC_RGBA_4BPP_sRGB:
		case MTL::PixelFormatPVRTC_RGB_2BPP:
		case MTL::PixelFormatPVRTC_RGB_2BPP_sRGB:
		case MTL::PixelFormatPVRTC_RGB_4BPP:
		case MTL::PixelFormatPVRTC_RGB_4BPP_sRGB:
			return true;
		default:
			return false;
	}
	GODOT_CLANG_WARNING_POP
#endif
}

MTLFormatType PixelFormats::getFormatType(DataFormat p_format) {
	return getDataFormatDesc(p_format).formatType;
}

MTLFormatType PixelFormats::getFormatType(MTL::PixelFormat p_format) {
	return getDataFormatDesc(p_format).formatType;
}

MTL::PixelFormat PixelFormats::getMTLPixelFormat(DataFormat p_format) {
	DataFormatDesc &dfDesc = getDataFormatDesc(p_format);
	MTL::PixelFormat mtlPixFmt = dfDesc.mtlPixelFormat;

	// If the MTL::PixelFormat is not supported but DataFormat is valid,
	// attempt to substitute a different format.
	if (mtlPixFmt == MTL::PixelFormatInvalid && p_format != RDC::DATA_FORMAT_MAX && dfDesc.chromaSubsamplingPlaneCount <= 1) {
		mtlPixFmt = dfDesc.mtlPixelFormatSubstitute;
	}

	return mtlPixFmt;
}

RDC::DataFormat PixelFormats::getDataFormat(MTL::PixelFormat p_format) {
	return getMTLPixelFormatDesc(p_format).dataFormat;
}

uint32_t PixelFormats::getBytesPerBlock(DataFormat p_format) {
	return getDataFormatDesc(p_format).bytesPerBlock;
}

uint32_t PixelFormats::getBytesPerBlock(MTL::PixelFormat p_format) {
	return getDataFormatDesc(p_format).bytesPerBlock;
}

uint8_t PixelFormats::getChromaSubsamplingPlaneCount(DataFormat p_format) {
	return getDataFormatDesc(p_format).chromaSubsamplingPlaneCount;
}

uint8_t PixelFormats::getChromaSubsamplingComponentBits(DataFormat p_format) {
	return getDataFormatDesc(p_format).chromaSubsamplingComponentBits;
}

float PixelFormats::getBytesPerTexel(DataFormat p_format) {
	return getDataFormatDesc(p_format).bytesPerTexel();
}

float PixelFormats::getBytesPerTexel(MTL::PixelFormat p_format) {
	return getDataFormatDesc(p_format).bytesPerTexel();
}

size_t PixelFormats::getBytesPerRow(DataFormat p_format, uint32_t p_texels_per_row) {
	DataFormatDesc &dfDesc = getDataFormatDesc(p_format);
	return Math::division_round_up(p_texels_per_row, dfDesc.blockTexelSize.width) * dfDesc.bytesPerBlock;
}

size_t PixelFormats::getBytesPerRow(MTL::PixelFormat p_format, uint32_t p_texels_per_row) {
	DataFormatDesc &dfDesc = getDataFormatDesc(p_format);
	return Math::division_round_up(p_texels_per_row, dfDesc.blockTexelSize.width) * dfDesc.bytesPerBlock;
}

size_t PixelFormats::getBytesPerLayer(DataFormat p_format, size_t p_bytes_per_row, uint32_t p_texel_rows_per_layer) {
	return Math::division_round_up(p_texel_rows_per_layer, getDataFormatDesc(p_format).blockTexelSize.height) * p_bytes_per_row;
}

size_t PixelFormats::getBytesPerLayer(MTL::PixelFormat p_format, size_t p_bytes_per_row, uint32_t p_texel_rows_per_layer) {
	return Math::division_round_up(p_texel_rows_per_layer, getDataFormatDesc(p_format).blockTexelSize.height) * p_bytes_per_row;
}

bool PixelFormats::needsSwizzle(DataFormat p_format) {
	return getDataFormatDesc(p_format).needsSwizzle();
}

MTLFmtCaps PixelFormats::getCapabilities(DataFormat p_format, bool p_extended) {
	return getCapabilities(getDataFormatDesc(p_format).mtlPixelFormat, p_extended);
}

MTLFmtCaps PixelFormats::getCapabilities(MTL::PixelFormat p_format, bool p_extended) {
	MTLFormatDesc &mtlDesc = getMTLPixelFormatDesc(p_format);
	MTLFmtCaps caps = mtlDesc.mtlFmtCaps;
	if (!p_extended || mtlDesc.mtlViewClass == MTLViewClass::None) {
		return caps;
	}
	// Now get caps of all formats in the view class.
	for (MTLFormatDesc &otherDesc : _mtl_vertex_format_descs) {
		if (otherDesc.mtlViewClass == mtlDesc.mtlViewClass) {
			caps |= otherDesc.mtlFmtCaps;
		}
	}
	return caps;
}

MTL::VertexFormat PixelFormats::getMTLVertexFormat(DataFormat p_format) {
	DataFormatDesc &dfDesc = getDataFormatDesc(p_format);
	MTL::VertexFormat format = dfDesc.mtlVertexFormat;

	if (format == MTL::VertexFormatInvalid) {
		String errMsg;
		errMsg += "DataFormat ";
		errMsg += dfDesc.name;
		errMsg += " is not supported for vertex buffers on this device.";

		if (dfDesc.vertexIsSupportedOrSubstitutable()) {
			format = dfDesc.mtlVertexFormatSubstitute;

			DataFormatDesc &dfDescSubs = getDataFormatDesc(getMTLVertexFormatDesc(format).dataFormat);
			errMsg += " Using DataFormat ";
			errMsg += dfDescSubs.name;
			errMsg += " instead.";
		}
		WARN_PRINT(errMsg);
	}

	return format;
}

DataFormatDesc &PixelFormats::getDataFormatDesc(DataFormat p_format) {
	return _data_format_descs[p_format];
}

DataFormatDesc &PixelFormats::getDataFormatDesc(MTL::PixelFormat p_format) {
	return getDataFormatDesc(getMTLPixelFormatDesc(p_format).dataFormat);
}

// Return a reference to the Metal format descriptor corresponding to the MTL::PixelFormat.
MTLFormatDesc &PixelFormats::getMTLPixelFormatDesc(MTL::PixelFormat p_format) {
	return _mtl_pixel_format_descs[p_format];
}

// Return a reference to the Metal format descriptor corresponding to the MTL::VertexFormat.
MTLFormatDesc &PixelFormats::getMTLVertexFormatDesc(MTL::VertexFormat p_format) {
	return _mtl_vertex_format_descs[p_format];
}

PixelFormats::PixelFormats(MTL::Device *p_device, const MetalFeatures &p_feat) :
		device(p_device->retain()) {
	initMTLPixelFormatCapabilities();
	initMTLVertexFormatCapabilities(p_feat);
	modifyMTLFormatCapabilities(p_feat);

	initDataFormatCapabilities();
	buildDFFormatMaps();
}

PixelFormats::~PixelFormats() {
	device->release();
}

#define addDataFormatDescFull(DATA_FMT, MTL_FMT, MTL_FMT_ALT, MTL_VTX_FMT, MTL_VTX_FMT_ALT, CSPC, CSCB, BLK_W, BLK_H, BLK_BYTE_CNT, MVK_FMT_TYPE, SWIZ_R, SWIZ_G, SWIZ_B, SWIZ_A) \
	dfFmt = RDC::DATA_FORMAT_##DATA_FMT; \
	_data_format_descs[dfFmt] = { dfFmt, MTL::PixelFormat##MTL_FMT, MTL::PixelFormat##MTL_FMT_ALT, MTL::VertexFormat##MTL_VTX_FMT, MTL::VertexFormat##MTL_VTX_FMT_ALT, \
		CSPC, CSCB, { BLK_W, BLK_H }, BLK_BYTE_CNT, MTLFormatType::MVK_FMT_TYPE, \
		{ RDC::TEXTURE_SWIZZLE_##SWIZ_R, RDC::TEXTURE_SWIZZLE_##SWIZ_G, RDC::TEXTURE_SWIZZLE_##SWIZ_B, RDC::TEXTURE_SWIZZLE_##SWIZ_A }, \
		"DATA_FORMAT_" #DATA_FMT, false }

#define addDataFormatDesc(VK_FMT, MTL_FMT, MTL_FMT_ALT, MTL_VTX_FMT, MTL_VTX_FMT_ALT, BLK_W, BLK_H, BLK_BYTE_CNT, MVK_FMT_TYPE) \
	addDataFormatDescFull(VK_FMT, MTL_FMT, MTL_FMT_ALT, MTL_VTX_FMT, MTL_VTX_FMT_ALT, 0, 0, BLK_W, BLK_H, BLK_BYTE_CNT, MVK_FMT_TYPE, IDENTITY, IDENTITY, IDENTITY, IDENTITY)

#define addDataFormatDescSwizzled(VK_FMT, MTL_FMT, MTL_FMT_ALT, MTL_VTX_FMT, MTL_VTX_FMT_ALT, BLK_W, BLK_H, BLK_BYTE_CNT, MVK_FMT_TYPE, SWIZ_R, SWIZ_G, SWIZ_B, SWIZ_A) \
	addDataFormatDescFull(VK_FMT, MTL_FMT, MTL_FMT_ALT, MTL_VTX_FMT, MTL_VTX_FMT_ALT, 0, 0, BLK_W, BLK_H, BLK_BYTE_CNT, MVK_FMT_TYPE, SWIZ_R, SWIZ_G, SWIZ_B, SWIZ_A)

#define addDfFormatDescChromaSubsampling(DATA_FMT, MTL_FMT, CSPC, CSCB, BLK_W, BLK_H, BLK_BYTE_CNT) \
	addDataFormatDescFull(DATA_FMT, MTL_FMT, Invalid, Invalid, Invalid, CSPC, CSCB, BLK_W, BLK_H, BLK_BYTE_CNT, ColorFloat, IDENTITY, IDENTITY, IDENTITY, IDENTITY)

void PixelFormats::initDataFormatCapabilities() {
	_data_format_descs.reserve(RDC::DATA_FORMAT_MAX + 1); // reserve enough space to avoid reallocs
	DataFormat dfFmt;

	addDataFormatDesc(R4G4_UNORM_PACK8, Invalid, Invalid, Invalid, Invalid, 1, 1, 1, ColorFloat);
	addDataFormatDesc(R4G4_UNORM_PACK8, Invalid, Invalid, Invalid, Invalid, 1, 1, 1, ColorFloat);
	addDataFormatDesc(R4G4B4A4_UNORM_PACK16, ABGR4Unorm, Invalid, Invalid, Invalid, 1, 1, 2, ColorFloat);
	addDataFormatDescSwizzled(B4G4R4A4_UNORM_PACK16, Invalid, Invalid, Invalid, Invalid, 1, 1, 2, ColorFloat, B, G, R, A);

	addDataFormatDesc(R5G6B5_UNORM_PACK16, B5G6R5Unorm, Invalid, Invalid, Invalid, 1, 1, 2, ColorFloat);
	addDataFormatDescSwizzled(B5G6R5_UNORM_PACK16, B5G6R5Unorm, Invalid, Invalid, Invalid, 1, 1, 2, ColorFloat, B, G, R, A);
	addDataFormatDesc(R5G5B5A1_UNORM_PACK16, A1BGR5Unorm, Invalid, Invalid, Invalid, 1, 1, 2, ColorFloat);
	addDataFormatDescSwizzled(B5G5R5A1_UNORM_PACK16, A1BGR5Unorm, Invalid, Invalid, Invalid, 1, 1, 2, ColorFloat, B, G, R, A);
	addDataFormatDesc(A1R5G5B5_UNORM_PACK16, BGR5A1Unorm, Invalid, Invalid, Invalid, 1, 1, 2, ColorFloat);

	addDataFormatDesc(R8_UNORM, R8Unorm, Invalid, UCharNormalized, UChar2Normalized, 1, 1, 1, ColorFloat);
	addDataFormatDesc(R8_SNORM, R8Snorm, Invalid, CharNormalized, Char2Normalized, 1, 1, 1, ColorFloat);
	addDataFormatDesc(R8_USCALED, Invalid, Invalid, UChar, UChar2, 1, 1, 1, ColorFloat);
	addDataFormatDesc(R8_SSCALED, Invalid, Invalid, Char, Char2, 1, 1, 1, ColorFloat);
	addDataFormatDesc(R8_UINT, R8Uint, Invalid, UChar, UChar2, 1, 1, 1, ColorUInt8);
	addDataFormatDesc(R8_SINT, R8Sint, Invalid, Char, Char2, 1, 1, 1, ColorInt8);
	addDataFormatDesc(R8_SRGB, R8Unorm_sRGB, Invalid, UCharNormalized, UChar2Normalized, 1, 1, 1, ColorFloat);

	addDataFormatDesc(R8G8_UNORM, RG8Unorm, Invalid, UChar2Normalized, Invalid, 1, 1, 2, ColorFloat);
	addDataFormatDesc(R8G8_SNORM, RG8Snorm, Invalid, Char2Normalized, Invalid, 1, 1, 2, ColorFloat);
	addDataFormatDesc(R8G8_USCALED, Invalid, Invalid, UChar2, Invalid, 1, 1, 2, ColorFloat);
	addDataFormatDesc(R8G8_SSCALED, Invalid, Invalid, Char2, Invalid, 1, 1, 2, ColorFloat);
	addDataFormatDesc(R8G8_UINT, RG8Uint, Invalid, UChar2, Invalid, 1, 1, 2, ColorUInt8);
	addDataFormatDesc(R8G8_SINT, RG8Sint, Invalid, Char2, Invalid, 1, 1, 2, ColorInt8);
	addDataFormatDesc(R8G8_SRGB, RG8Unorm_sRGB, Invalid, UChar2Normalized, Invalid, 1, 1, 2, ColorFloat);

	addDataFormatDesc(R8G8B8_UNORM, Invalid, Invalid, UChar3Normalized, Invalid, 1, 1, 3, ColorFloat);
	addDataFormatDesc(R8G8B8_SNORM, Invalid, Invalid, Char3Normalized, Invalid, 1, 1, 3, ColorFloat);
	addDataFormatDesc(R8G8B8_USCALED, Invalid, Invalid, UChar3, Invalid, 1, 1, 3, ColorFloat);
	addDataFormatDesc(R8G8B8_SSCALED, Invalid, Invalid, Char3, Invalid, 1, 1, 3, ColorFloat);
	addDataFormatDesc(R8G8B8_UINT, Invalid, Invalid, UChar3, Invalid, 1, 1, 3, ColorUInt8);
	addDataFormatDesc(R8G8B8_SINT, Invalid, Invalid, Char3, Invalid, 1, 1, 3, ColorInt8);
	addDataFormatDesc(R8G8B8_SRGB, Invalid, Invalid, UChar3Normalized, Invalid, 1, 1, 3, ColorFloat);

	addDataFormatDesc(B8G8R8_UNORM, Invalid, Invalid, Invalid, Invalid, 1, 1, 3, ColorFloat);
	addDataFormatDesc(B8G8R8_SNORM, Invalid, Invalid, Invalid, Invalid, 1, 1, 3, ColorFloat);
	addDataFormatDesc(B8G8R8_USCALED, Invalid, Invalid, Invalid, Invalid, 1, 1, 3, ColorFloat);
	addDataFormatDesc(B8G8R8_SSCALED, Invalid, Invalid, Invalid, Invalid, 1, 1, 3, ColorFloat);
	addDataFormatDesc(B8G8R8_UINT, Invalid, Invalid, Invalid, Invalid, 1, 1, 3, ColorUInt8);
	addDataFormatDesc(B8G8R8_SINT, Invalid, Invalid, Invalid, Invalid, 1, 1, 3, ColorInt8);
	addDataFormatDesc(B8G8R8_SRGB, Invalid, Invalid, Invalid, Invalid, 1, 1, 3, ColorFloat);

	addDataFormatDesc(R8G8B8A8_UNORM, RGBA8Unorm, Invalid, UChar4Normalized, Invalid, 1, 1, 4, ColorFloat);
	addDataFormatDesc(R8G8B8A8_SNORM, RGBA8Snorm, Invalid, Char4Normalized, Invalid, 1, 1, 4, ColorFloat);
	addDataFormatDesc(R8G8B8A8_USCALED, Invalid, Invalid, UChar4, Invalid, 1, 1, 4, ColorFloat);
	addDataFormatDesc(R8G8B8A8_SSCALED, Invalid, Invalid, Char4, Invalid, 1, 1, 4, ColorFloat);
	addDataFormatDesc(R8G8B8A8_UINT, RGBA8Uint, Invalid, UChar4, Invalid, 1, 1, 4, ColorUInt8);
	addDataFormatDesc(R8G8B8A8_SINT, RGBA8Sint, Invalid, Char4, Invalid, 1, 1, 4, ColorInt8);
	addDataFormatDesc(R8G8B8A8_SRGB, RGBA8Unorm_sRGB, Invalid, UChar4Normalized, Invalid, 1, 1, 4, ColorFloat);

	addDataFormatDesc(B8G8R8A8_UNORM, BGRA8Unorm, Invalid, UChar4Normalized_BGRA, Invalid, 1, 1, 4, ColorFloat);
	addDataFormatDescSwizzled(B8G8R8A8_SNORM, RGBA8Snorm, Invalid, Invalid, Invalid, 1, 1, 4, ColorFloat, B, G, R, A);
	addDataFormatDesc(B8G8R8A8_USCALED, Invalid, Invalid, Invalid, Invalid, 1, 1, 4, ColorFloat);
	addDataFormatDesc(B8G8R8A8_SSCALED, Invalid, Invalid, Invalid, Invalid, 1, 1, 4, ColorFloat);
	addDataFormatDescSwizzled(B8G8R8A8_UINT, RGBA8Uint, Invalid, Invalid, Invalid, 1, 1, 4, ColorUInt8, B, G, R, A);
	addDataFormatDescSwizzled(B8G8R8A8_SINT, RGBA8Sint, Invalid, Invalid, Invalid, 1, 1, 4, ColorInt8, B, G, R, A);
	addDataFormatDesc(B8G8R8A8_SRGB, BGRA8Unorm_sRGB, Invalid, Invalid, Invalid, 1, 1, 4, ColorFloat);

	addDataFormatDesc(A8B8G8R8_UNORM_PACK32, RGBA8Unorm, Invalid, UChar4Normalized, Invalid, 1, 1, 4, ColorFloat);
	addDataFormatDesc(A8B8G8R8_SNORM_PACK32, RGBA8Snorm, Invalid, Char4Normalized, Invalid, 1, 1, 4, ColorFloat);
	addDataFormatDesc(A8B8G8R8_USCALED_PACK32, Invalid, Invalid, UChar4, Invalid, 1, 1, 4, ColorFloat);
	addDataFormatDesc(A8B8G8R8_SSCALED_PACK32, Invalid, Invalid, Char4, Invalid, 1, 1, 4, ColorFloat);
	addDataFormatDesc(A8B8G8R8_UINT_PACK32, RGBA8Uint, Invalid, UChar4, Invalid, 1, 1, 4, ColorUInt8);
	addDataFormatDesc(A8B8G8R8_SINT_PACK32, RGBA8Sint, Invalid, Char4, Invalid, 1, 1, 4, ColorInt8);
	addDataFormatDesc(A8B8G8R8_SRGB_PACK32, RGBA8Unorm_sRGB, Invalid, UChar4Normalized, Invalid, 1, 1, 4, ColorFloat);

	addDataFormatDesc(A2R10G10B10_UNORM_PACK32, BGR10A2Unorm, Invalid, Invalid, Invalid, 1, 1, 4, ColorFloat);
	addDataFormatDesc(A2R10G10B10_SNORM_PACK32, Invalid, Invalid, Invalid, Invalid, 1, 1, 4, ColorFloat);
	addDataFormatDesc(A2R10G10B10_USCALED_PACK32, Invalid, Invalid, Invalid, Invalid, 1, 1, 4, ColorFloat);
	addDataFormatDesc(A2R10G10B10_SSCALED_PACK32, Invalid, Invalid, Invalid, Invalid, 1, 1, 4, ColorFloat);
	addDataFormatDesc(A2R10G10B10_UINT_PACK32, Invalid, Invalid, Invalid, Invalid, 1, 1, 4, ColorUInt16);
	addDataFormatDesc(A2R10G10B10_SINT_PACK32, Invalid, Invalid, Invalid, Invalid, 1, 1, 4, ColorInt16);

	addDataFormatDesc(A2B10G10R10_UNORM_PACK32, RGB10A2Unorm, Invalid, UInt1010102Normalized, Invalid, 1, 1, 4, ColorFloat);
	addDataFormatDesc(A2B10G10R10_SNORM_PACK32, Invalid, Invalid, Int1010102Normalized, Invalid, 1, 1, 4, ColorFloat);
	addDataFormatDesc(A2B10G10R10_USCALED_PACK32, Invalid, Invalid, Invalid, Invalid, 1, 1, 4, ColorFloat);
	addDataFormatDesc(A2B10G10R10_SSCALED_PACK32, Invalid, Invalid, Invalid, Invalid, 1, 1, 4, ColorFloat);
	addDataFormatDesc(A2B10G10R10_UINT_PACK32, RGB10A2Uint, Invalid, Invalid, Invalid, 1, 1, 4, ColorUInt16);
	addDataFormatDesc(A2B10G10R10_SINT_PACK32, Invalid, Invalid, Invalid, Invalid, 1, 1, 4, ColorInt16);

	addDataFormatDesc(R16_UNORM, R16Unorm, Invalid, UShortNormalized, UShort2Normalized, 1, 1, 2, ColorFloat);
	addDataFormatDesc(R16_SNORM, R16Snorm, Invalid, ShortNormalized, Short2Normalized, 1, 1, 2, ColorFloat);
	addDataFormatDesc(R16_USCALED, Invalid, Invalid, UShort, UShort2, 1, 1, 2, ColorFloat);
	addDataFormatDesc(R16_SSCALED, Invalid, Invalid, Short, Short2, 1, 1, 2, ColorFloat);
	addDataFormatDesc(R16_UINT, R16Uint, Invalid, UShort, UShort2, 1, 1, 2, ColorUInt16);
	addDataFormatDesc(R16_SINT, R16Sint, Invalid, Short, Short2, 1, 1, 2, ColorInt16);
	addDataFormatDesc(R16_SFLOAT, R16Float, Invalid, Half, Half2, 1, 1, 2, ColorFloat);

	addDataFormatDesc(R16G16_UNORM, RG16Unorm, Invalid, UShort2Normalized, Invalid, 1, 1, 4, ColorFloat);
	addDataFormatDesc(R16G16_SNORM, RG16Snorm, Invalid, Short2Normalized, Invalid, 1, 1, 4, ColorFloat);
	addDataFormatDesc(R16G16_USCALED, Invalid, Invalid, UShort2, Invalid, 1, 1, 4, ColorFloat);
	addDataFormatDesc(R16G16_SSCALED, Invalid, Invalid, Short2, Invalid, 1, 1, 4, ColorFloat);
	addDataFormatDesc(R16G16_UINT, RG16Uint, Invalid, UShort2, Invalid, 1, 1, 4, ColorUInt16);
	addDataFormatDesc(R16G16_SINT, RG16Sint, Invalid, Short2, Invalid, 1, 1, 4, ColorInt16);
	addDataFormatDesc(R16G16_SFLOAT, RG16Float, Invalid, Half2, Invalid, 1, 1, 4, ColorFloat);

	addDataFormatDesc(R16G16B16_UNORM, Invalid, Invalid, UShort3Normalized, Invalid, 1, 1, 6, ColorFloat);
	addDataFormatDesc(R16G16B16_SNORM, Invalid, Invalid, Short3Normalized, Invalid, 1, 1, 6, ColorFloat);
	addDataFormatDesc(R16G16B16_USCALED, Invalid, Invalid, UShort3, Invalid, 1, 1, 6, ColorFloat);
	addDataFormatDesc(R16G16B16_SSCALED, Invalid, Invalid, Short3, Invalid, 1, 1, 6, ColorFloat);
	addDataFormatDesc(R16G16B16_UINT, Invalid, Invalid, UShort3, Invalid, 1, 1, 6, ColorUInt16);
	addDataFormatDesc(R16G16B16_SINT, Invalid, Invalid, Short3, Invalid, 1, 1, 6, ColorInt16);
	addDataFormatDesc(R16G16B16_SFLOAT, Invalid, Invalid, Half3, Invalid, 1, 1, 6, ColorFloat);

	addDataFormatDesc(R16G16B16A16_UNORM, RGBA16Unorm, Invalid, UShort4Normalized, Invalid, 1, 1, 8, ColorFloat);
	addDataFormatDesc(R16G16B16A16_SNORM, RGBA16Snorm, Invalid, Short4Normalized, Invalid, 1, 1, 8, ColorFloat);
	addDataFormatDesc(R16G16B16A16_USCALED, Invalid, Invalid, UShort4, Invalid, 1, 1, 8, ColorFloat);
	addDataFormatDesc(R16G16B16A16_SSCALED, Invalid, Invalid, Short4, Invalid, 1, 1, 8, ColorFloat);
	addDataFormatDesc(R16G16B16A16_UINT, RGBA16Uint, Invalid, UShort4, Invalid, 1, 1, 8, ColorUInt16);
	addDataFormatDesc(R16G16B16A16_SINT, RGBA16Sint, Invalid, Short4, Invalid, 1, 1, 8, ColorInt16);
	addDataFormatDesc(R16G16B16A16_SFLOAT, RGBA16Float, Invalid, Half4, Invalid, 1, 1, 8, ColorFloat);

	addDataFormatDesc(R32_UINT, R32Uint, Invalid, UInt, Invalid, 1, 1, 4, ColorUInt32);
	addDataFormatDesc(R32_SINT, R32Sint, Invalid, Int, Invalid, 1, 1, 4, ColorInt32);
	addDataFormatDesc(R32_SFLOAT, R32Float, Invalid, Float, Invalid, 1, 1, 4, ColorFloat);

	addDataFormatDesc(R32G32_UINT, RG32Uint, Invalid, UInt2, Invalid, 1, 1, 8, ColorUInt32);
	addDataFormatDesc(R32G32_SINT, RG32Sint, Invalid, Int2, Invalid, 1, 1, 8, ColorInt32);
	addDataFormatDesc(R32G32_SFLOAT, RG32Float, Invalid, Float2, Invalid, 1, 1, 8, ColorFloat);

	addDataFormatDesc(R32G32B32_UINT, Invalid, Invalid, UInt3, Invalid, 1, 1, 12, ColorUInt32);
	addDataFormatDesc(R32G32B32_SINT, Invalid, Invalid, Int3, Invalid, 1, 1, 12, ColorInt32);
	addDataFormatDesc(R32G32B32_SFLOAT, Invalid, Invalid, Float3, Invalid, 1, 1, 12, ColorFloat);

	addDataFormatDesc(R32G32B32A32_UINT, RGBA32Uint, Invalid, UInt4, Invalid, 1, 1, 16, ColorUInt32);
	addDataFormatDesc(R32G32B32A32_SINT, RGBA32Sint, Invalid, Int4, Invalid, 1, 1, 16, ColorInt32);
	addDataFormatDesc(R32G32B32A32_SFLOAT, RGBA32Float, Invalid, Float4, Invalid, 1, 1, 16, ColorFloat);

	addDataFormatDesc(R64_UINT, Invalid, Invalid, Invalid, Invalid, 1, 1, 8, ColorFloat);
	addDataFormatDesc(R64_SINT, Invalid, Invalid, Invalid, Invalid, 1, 1, 8, ColorFloat);
	addDataFormatDesc(R64_SFLOAT, Invalid, Invalid, Invalid, Invalid, 1, 1, 8, ColorFloat);

	addDataFormatDesc(R64G64_UINT, Invalid, Invalid, Invalid, Invalid, 1, 1, 16, ColorFloat);
	addDataFormatDesc(R64G64_SINT, Invalid, Invalid, Invalid, Invalid, 1, 1, 16, ColorFloat);
	addDataFormatDesc(R64G64_SFLOAT, Invalid, Invalid, Invalid, Invalid, 1, 1, 16, ColorFloat);

	addDataFormatDesc(R64G64B64_UINT, Invalid, Invalid, Invalid, Invalid, 1, 1, 24, ColorFloat);
	addDataFormatDesc(R64G64B64_SINT, Invalid, Invalid, Invalid, Invalid, 1, 1, 24, ColorFloat);
	addDataFormatDesc(R64G64B64_SFLOAT, Invalid, Invalid, Invalid, Invalid, 1, 1, 24, ColorFloat);

	addDataFormatDesc(R64G64B64A64_UINT, Invalid, Invalid, Invalid, Invalid, 1, 1, 32, ColorFloat);
	addDataFormatDesc(R64G64B64A64_SINT, Invalid, Invalid, Invalid, Invalid, 1, 1, 32, ColorFloat);
	addDataFormatDesc(R64G64B64A64_SFLOAT, Invalid, Invalid, Invalid, Invalid, 1, 1, 32, ColorFloat);

	addDataFormatDesc(B10G11R11_UFLOAT_PACK32, RG11B10Float, Invalid, Invalid, Invalid, 1, 1, 4, ColorFloat);
	addDataFormatDesc(E5B9G9R9_UFLOAT_PACK32, RGB9E5Float, Invalid, Invalid, Invalid, 1, 1, 4, ColorFloat);

	addDataFormatDesc(D32_SFLOAT, Depth32Float, Invalid, Invalid, Invalid, 1, 1, 4, DepthStencil);
	addDataFormatDesc(D32_SFLOAT_S8_UINT, Depth32Float_Stencil8, Invalid, Invalid, Invalid, 1, 1, 5, DepthStencil);

	addDataFormatDesc(S8_UINT, Stencil8, Invalid, Invalid, Invalid, 1, 1, 1, DepthStencil);

	addDataFormatDesc(D16_UNORM, Depth16Unorm, Depth32Float, Invalid, Invalid, 1, 1, 2, DepthStencil);
	addDataFormatDesc(D16_UNORM_S8_UINT, Invalid, Invalid, Invalid, Invalid, 1, 1, 3, DepthStencil);
	addDataFormatDesc(D24_UNORM_S8_UINT, Depth24Unorm_Stencil8, Depth32Float_Stencil8, Invalid, Invalid, 1, 1, 4, DepthStencil);

	addDataFormatDesc(X8_D24_UNORM_PACK32, Invalid, Depth24Unorm_Stencil8, Invalid, Invalid, 1, 1, 4, DepthStencil);

	GODOT_CLANG_WARNING_PUSH_AND_IGNORE("-Wunguarded-availability")

	addDataFormatDesc(BC1_RGB_UNORM_BLOCK, BC1_RGBA, Invalid, Invalid, Invalid, 4, 4, 8, Compressed);
	addDataFormatDesc(BC1_RGB_SRGB_BLOCK, BC1_RGBA_sRGB, Invalid, Invalid, Invalid, 4, 4, 8, Compressed);
	addDataFormatDesc(BC1_RGBA_UNORM_BLOCK, BC1_RGBA, Invalid, Invalid, Invalid, 4, 4, 8, Compressed);
	addDataFormatDesc(BC1_RGBA_SRGB_BLOCK, BC1_RGBA_sRGB, Invalid, Invalid, Invalid, 4, 4, 8, Compressed);

	addDataFormatDesc(BC2_UNORM_BLOCK, BC2_RGBA, Invalid, Invalid, Invalid, 4, 4, 16, Compressed);
	addDataFormatDesc(BC2_SRGB_BLOCK, BC2_RGBA_sRGB, Invalid, Invalid, Invalid, 4, 4, 16, Compressed);

	addDataFormatDesc(BC3_UNORM_BLOCK, BC3_RGBA, Invalid, Invalid, Invalid, 4, 4, 16, Compressed);
	addDataFormatDesc(BC3_SRGB_BLOCK, BC3_RGBA_sRGB, Invalid, Invalid, Invalid, 4, 4, 16, Compressed);

	addDataFormatDesc(BC4_UNORM_BLOCK, BC4_RUnorm, Invalid, Invalid, Invalid, 4, 4, 8, Compressed);
	addDataFormatDesc(BC4_SNORM_BLOCK, BC4_RSnorm, Invalid, Invalid, Invalid, 4, 4, 8, Compressed);

	addDataFormatDesc(BC5_UNORM_BLOCK, BC5_RGUnorm, Invalid, Invalid, Invalid, 4, 4, 16, Compressed);
	addDataFormatDesc(BC5_SNORM_BLOCK, BC5_RGSnorm, Invalid, Invalid, Invalid, 4, 4, 16, Compressed);

	addDataFormatDesc(BC6H_UFLOAT_BLOCK, BC6H_RGBUfloat, Invalid, Invalid, Invalid, 4, 4, 16, Compressed);
	addDataFormatDesc(BC6H_SFLOAT_BLOCK, BC6H_RGBFloat, Invalid, Invalid, Invalid, 4, 4, 16, Compressed);

	addDataFormatDesc(BC7_UNORM_BLOCK, BC7_RGBAUnorm, Invalid, Invalid, Invalid, 4, 4, 16, Compressed);
	addDataFormatDesc(BC7_SRGB_BLOCK, BC7_RGBAUnorm_sRGB, Invalid, Invalid, Invalid, 4, 4, 16, Compressed);

	GODOT_CLANG_WARNING_POP

	addDataFormatDesc(ETC2_R8G8B8_UNORM_BLOCK, ETC2_RGB8, Invalid, Invalid, Invalid, 4, 4, 8, Compressed);
	addDataFormatDesc(ETC2_R8G8B8_SRGB_BLOCK, ETC2_RGB8_sRGB, Invalid, Invalid, Invalid, 4, 4, 8, Compressed);
	addDataFormatDesc(ETC2_R8G8B8A1_UNORM_BLOCK, ETC2_RGB8A1, Invalid, Invalid, Invalid, 4, 4, 8, Compressed);
	addDataFormatDesc(ETC2_R8G8B8A1_SRGB_BLOCK, ETC2_RGB8A1_sRGB, Invalid, Invalid, Invalid, 4, 4, 8, Compressed);

	addDataFormatDesc(ETC2_R8G8B8A8_UNORM_BLOCK, EAC_RGBA8, Invalid, Invalid, Invalid, 4, 4, 16, Compressed);
	addDataFormatDesc(ETC2_R8G8B8A8_SRGB_BLOCK, EAC_RGBA8_sRGB, Invalid, Invalid, Invalid, 4, 4, 16, Compressed);

	addDataFormatDesc(EAC_R11_UNORM_BLOCK, EAC_R11Unorm, Invalid, Invalid, Invalid, 4, 4, 8, Compressed);
	addDataFormatDesc(EAC_R11_SNORM_BLOCK, EAC_R11Snorm, Invalid, Invalid, Invalid, 4, 4, 8, Compressed);

	addDataFormatDesc(EAC_R11G11_UNORM_BLOCK, EAC_RG11Unorm, Invalid, Invalid, Invalid, 4, 4, 16, Compressed);
	addDataFormatDesc(EAC_R11G11_SNORM_BLOCK, EAC_RG11Snorm, Invalid, Invalid, Invalid, 4, 4, 16, Compressed);

	addDataFormatDesc(ASTC_4x4_UNORM_BLOCK, ASTC_4x4_LDR, Invalid, Invalid, Invalid, 4, 4, 16, Compressed);
	addDataFormatDesc(ASTC_4x4_SFLOAT_BLOCK, ASTC_4x4_HDR, Invalid, Invalid, Invalid, 4, 4, 16, Compressed);
	addDataFormatDesc(ASTC_4x4_SRGB_BLOCK, ASTC_4x4_sRGB, Invalid, Invalid, Invalid, 4, 4, 16, Compressed);
	addDataFormatDesc(ASTC_5x4_UNORM_BLOCK, ASTC_5x4_LDR, Invalid, Invalid, Invalid, 5, 4, 16, Compressed);
	addDataFormatDesc(ASTC_5x4_SFLOAT_BLOCK, ASTC_5x4_HDR, Invalid, Invalid, Invalid, 5, 4, 16, Compressed);
	addDataFormatDesc(ASTC_5x4_SRGB_BLOCK, ASTC_5x4_sRGB, Invalid, Invalid, Invalid, 5, 4, 16, Compressed);
	addDataFormatDesc(ASTC_5x5_UNORM_BLOCK, ASTC_5x5_LDR, Invalid, Invalid, Invalid, 5, 5, 16, Compressed);
	addDataFormatDesc(ASTC_5x5_SFLOAT_BLOCK, ASTC_5x5_HDR, Invalid, Invalid, Invalid, 5, 5, 16, Compressed);
	addDataFormatDesc(ASTC_5x5_SRGB_BLOCK, ASTC_5x5_sRGB, Invalid, Invalid, Invalid, 5, 5, 16, Compressed);
	addDataFormatDesc(ASTC_6x5_UNORM_BLOCK, ASTC_6x5_LDR, Invalid, Invalid, Invalid, 6, 5, 16, Compressed);
	addDataFormatDesc(ASTC_6x5_SFLOAT_BLOCK, ASTC_6x5_HDR, Invalid, Invalid, Invalid, 6, 5, 16, Compressed);
	addDataFormatDesc(ASTC_6x5_SRGB_BLOCK, ASTC_6x5_sRGB, Invalid, Invalid, Invalid, 6, 5, 16, Compressed);
	addDataFormatDesc(ASTC_6x6_UNORM_BLOCK, ASTC_6x6_LDR, Invalid, Invalid, Invalid, 6, 6, 16, Compressed);
	addDataFormatDesc(ASTC_6x6_SFLOAT_BLOCK, ASTC_6x6_HDR, Invalid, Invalid, Invalid, 6, 6, 16, Compressed);
	addDataFormatDesc(ASTC_6x6_SRGB_BLOCK, ASTC_6x6_sRGB, Invalid, Invalid, Invalid, 6, 6, 16, Compressed);
	addDataFormatDesc(ASTC_8x5_UNORM_BLOCK, ASTC_8x5_LDR, Invalid, Invalid, Invalid, 8, 5, 16, Compressed);
	addDataFormatDesc(ASTC_8x5_SFLOAT_BLOCK, ASTC_8x5_HDR, Invalid, Invalid, Invalid, 8, 5, 16, Compressed);
	addDataFormatDesc(ASTC_8x5_SRGB_BLOCK, ASTC_8x5_sRGB, Invalid, Invalid, Invalid, 8, 5, 16, Compressed);
	addDataFormatDesc(ASTC_8x6_UNORM_BLOCK, ASTC_8x6_LDR, Invalid, Invalid, Invalid, 8, 6, 16, Compressed);
	addDataFormatDesc(ASTC_8x6_SFLOAT_BLOCK, ASTC_8x6_HDR, Invalid, Invalid, Invalid, 8, 6, 16, Compressed);
	addDataFormatDesc(ASTC_8x6_SRGB_BLOCK, ASTC_8x6_sRGB, Invalid, Invalid, Invalid, 8, 6, 16, Compressed);
	addDataFormatDesc(ASTC_8x8_UNORM_BLOCK, ASTC_8x8_LDR, Invalid, Invalid, Invalid, 8, 8, 16, Compressed);
	addDataFormatDesc(ASTC_8x8_SFLOAT_BLOCK, ASTC_8x8_HDR, Invalid, Invalid, Invalid, 8, 8, 16, Compressed);
	addDataFormatDesc(ASTC_8x8_SRGB_BLOCK, ASTC_8x8_sRGB, Invalid, Invalid, Invalid, 8, 8, 16, Compressed);
	addDataFormatDesc(ASTC_10x5_UNORM_BLOCK, ASTC_10x5_LDR, Invalid, Invalid, Invalid, 10, 5, 16, Compressed);
	addDataFormatDesc(ASTC_10x5_SFLOAT_BLOCK, ASTC_10x5_HDR, Invalid, Invalid, Invalid, 10, 5, 16, Compressed);
	addDataFormatDesc(ASTC_10x5_SRGB_BLOCK, ASTC_10x5_sRGB, Invalid, Invalid, Invalid, 10, 5, 16, Compressed);
	addDataFormatDesc(ASTC_10x6_UNORM_BLOCK, ASTC_10x6_LDR, Invalid, Invalid, Invalid, 10, 6, 16, Compressed);
	addDataFormatDesc(ASTC_10x6_SFLOAT_BLOCK, ASTC_10x6_HDR, Invalid, Invalid, Invalid, 10, 6, 16, Compressed);
	addDataFormatDesc(ASTC_10x6_SRGB_BLOCK, ASTC_10x6_sRGB, Invalid, Invalid, Invalid, 10, 6, 16, Compressed);
	addDataFormatDesc(ASTC_10x8_UNORM_BLOCK, ASTC_10x8_LDR, Invalid, Invalid, Invalid, 10, 8, 16, Compressed);
	addDataFormatDesc(ASTC_10x8_SFLOAT_BLOCK, ASTC_10x8_HDR, Invalid, Invalid, Invalid, 10, 8, 16, Compressed);
	addDataFormatDesc(ASTC_10x8_SRGB_BLOCK, ASTC_10x8_sRGB, Invalid, Invalid, Invalid, 10, 8, 16, Compressed);
	addDataFormatDesc(ASTC_10x10_UNORM_BLOCK, ASTC_10x10_LDR, Invalid, Invalid, Invalid, 10, 10, 16, Compressed);
	addDataFormatDesc(ASTC_10x10_SFLOAT_BLOCK, ASTC_10x10_HDR, Invalid, Invalid, Invalid, 10, 10, 16, Compressed);
	addDataFormatDesc(ASTC_10x10_SRGB_BLOCK, ASTC_10x10_sRGB, Invalid, Invalid, Invalid, 10, 10, 16, Compressed);
	addDataFormatDesc(ASTC_12x10_UNORM_BLOCK, ASTC_12x10_LDR, Invalid, Invalid, Invalid, 12, 10, 16, Compressed);
	addDataFormatDesc(ASTC_12x10_SFLOAT_BLOCK, ASTC_12x10_HDR, Invalid, Invalid, Invalid, 12, 10, 16, Compressed);
	addDataFormatDesc(ASTC_12x10_SRGB_BLOCK, ASTC_12x10_sRGB, Invalid, Invalid, Invalid, 12, 10, 16, Compressed);
	addDataFormatDesc(ASTC_12x12_UNORM_BLOCK, ASTC_12x12_LDR, Invalid, Invalid, Invalid, 12, 12, 16, Compressed);
	addDataFormatDesc(ASTC_12x12_SFLOAT_BLOCK, ASTC_12x12_HDR, Invalid, Invalid, Invalid, 12, 12, 16, Compressed);
	addDataFormatDesc(ASTC_12x12_SRGB_BLOCK, ASTC_12x12_sRGB, Invalid, Invalid, Invalid, 12, 12, 16, Compressed);

	addDfFormatDescChromaSubsampling(G8B8G8R8_422_UNORM, GBGR422, 1, 8, 2, 1, 4);
	addDfFormatDescChromaSubsampling(B8G8R8G8_422_UNORM, BGRG422, 1, 8, 2, 1, 4);
	addDfFormatDescChromaSubsampling(G8_B8_R8_3PLANE_420_UNORM, Invalid, 3, 8, 2, 2, 6);
	addDfFormatDescChromaSubsampling(G8_B8R8_2PLANE_420_UNORM, Invalid, 2, 8, 2, 2, 6);
	addDfFormatDescChromaSubsampling(G8_B8_R8_3PLANE_422_UNORM, Invalid, 3, 8, 2, 1, 4);
	addDfFormatDescChromaSubsampling(G8_B8R8_2PLANE_422_UNORM, Invalid, 2, 8, 2, 1, 4);
	addDfFormatDescChromaSubsampling(G8_B8_R8_3PLANE_444_UNORM, Invalid, 3, 8, 1, 1, 3);
	addDfFormatDescChromaSubsampling(R10X6_UNORM_PACK16, R16Unorm, 0, 10, 1, 1, 2);
	addDfFormatDescChromaSubsampling(R10X6G10X6_UNORM_2PACK16, RG16Unorm, 0, 10, 1, 1, 4);
	addDfFormatDescChromaSubsampling(R10X6G10X6B10X6A10X6_UNORM_4PACK16, RGBA16Unorm, 0, 10, 1, 1, 8);
	addDfFormatDescChromaSubsampling(G10X6B10X6G10X6R10X6_422_UNORM_4PACK16, Invalid, 1, 10, 2, 1, 8);
	addDfFormatDescChromaSubsampling(B10X6G10X6R10X6G10X6_422_UNORM_4PACK16, Invalid, 1, 10, 2, 1, 8);
	addDfFormatDescChromaSubsampling(G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16, Invalid, 3, 10, 2, 2, 12);
	addDfFormatDescChromaSubsampling(G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16, Invalid, 2, 10, 2, 2, 12);
	addDfFormatDescChromaSubsampling(G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16, Invalid, 3, 10, 2, 1, 8);
	addDfFormatDescChromaSubsampling(G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16, Invalid, 2, 10, 2, 1, 8);
	addDfFormatDescChromaSubsampling(G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16, Invalid, 3, 10, 1, 1, 6);
	addDfFormatDescChromaSubsampling(R12X4_UNORM_PACK16, R16Unorm, 0, 12, 1, 1, 2);
	addDfFormatDescChromaSubsampling(R12X4G12X4_UNORM_2PACK16, RG16Unorm, 0, 12, 1, 1, 4);
	addDfFormatDescChromaSubsampling(R12X4G12X4B12X4A12X4_UNORM_4PACK16, RGBA16Unorm, 0, 12, 1, 1, 8);
	addDfFormatDescChromaSubsampling(G12X4B12X4G12X4R12X4_422_UNORM_4PACK16, Invalid, 1, 12, 2, 1, 8);
	addDfFormatDescChromaSubsampling(B12X4G12X4R12X4G12X4_422_UNORM_4PACK16, Invalid, 1, 12, 2, 1, 8);
	addDfFormatDescChromaSubsampling(G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16, Invalid, 3, 12, 2, 2, 12);
	addDfFormatDescChromaSubsampling(G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16, Invalid, 2, 12, 2, 2, 12);
	addDfFormatDescChromaSubsampling(G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16, Invalid, 3, 12, 2, 1, 8);
	addDfFormatDescChromaSubsampling(G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16, Invalid, 2, 12, 2, 1, 8);
	addDfFormatDescChromaSubsampling(G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16, Invalid, 3, 12, 1, 1, 6);
	addDfFormatDescChromaSubsampling(G16B16G16R16_422_UNORM, Invalid, 1, 16, 2, 1, 8);
	addDfFormatDescChromaSubsampling(B16G16R16G16_422_UNORM, Invalid, 1, 16, 2, 1, 8);
	addDfFormatDescChromaSubsampling(G16_B16_R16_3PLANE_420_UNORM, Invalid, 3, 16, 2, 2, 12);
	addDfFormatDescChromaSubsampling(G16_B16R16_2PLANE_420_UNORM, Invalid, 2, 16, 2, 2, 12);
	addDfFormatDescChromaSubsampling(G16_B16_R16_3PLANE_422_UNORM, Invalid, 3, 16, 2, 1, 8);
	addDfFormatDescChromaSubsampling(G16_B16R16_2PLANE_422_UNORM, Invalid, 2, 16, 2, 1, 8);
	addDfFormatDescChromaSubsampling(G16_B16_R16_3PLANE_444_UNORM, Invalid, 3, 16, 1, 1, 6);
}

void PixelFormats::addMTLPixelFormatDescImpl(MTL::PixelFormat p_pix_fmt, MTL::PixelFormat p_pix_fmt_linear,
		MTLViewClass p_view_class, MTLFmtCaps p_fmt_caps, const char *p_name) {
	_mtl_pixel_format_descs[p_pix_fmt] = { .mtlPixelFormat = p_pix_fmt, .dataFormat = DataFormat::DATA_FORMAT_MAX, .mtlFmtCaps = p_fmt_caps, .mtlViewClass = p_view_class, .mtlPixelFormatLinear = p_pix_fmt_linear, .name = p_name };
}

#define addMTLPixelFormatDescFull(mtlFmt, mtlFmtLinear, viewClass, appleGPUCaps) \
	addMTLPixelFormatDescImpl(MTL::PixelFormat##mtlFmt, MTL::PixelFormat##mtlFmtLinear, MTLViewClass::viewClass, \
			appleGPUCaps, "MTL::PixelFormat" #mtlFmt)

#define addMTLPixelFormatDesc(mtlFmt, viewClass, appleGPUCaps) \
	addMTLPixelFormatDescFull(mtlFmt, mtlFmt, viewClass, kMTLFmtCaps##appleGPUCaps)

#define addMTLPixelFormatDescSRGB(mtlFmt, viewClass, appleGPUCaps, mtlFmtLinear) \
	/* Cannot write to sRGB textures in the simulator */ \
	if (TARGET_OS_SIMULATOR) { \
		MTLFmtCaps appleFmtCaps = kMTLFmtCaps##appleGPUCaps; \
		flags::clear(appleFmtCaps, kMTLFmtCapsWrite); \
		addMTLPixelFormatDescFull(mtlFmt, mtlFmtLinear, viewClass, appleFmtCaps); \
	} else { \
		addMTLPixelFormatDescFull(mtlFmt, mtlFmtLinear, viewClass, kMTLFmtCaps##appleGPUCaps); \
	}

void PixelFormats::initMTLPixelFormatCapabilities() {
	_mtl_pixel_format_descs.reserve(1024);

	// MTL::PixelFormatInvalid must come first. Use addMTLPixelFormatDescImpl to avoid guard code.
	addMTLPixelFormatDescImpl(MTL::PixelFormatInvalid, MTL::PixelFormatInvalid, MTLViewClass::None, kMTLFmtCapsNone, "MTL::PixelFormatInvalid");

	// Ordinary 8-bit pixel formats.
	addMTLPixelFormatDesc(A8Unorm, Color8, All);
	addMTLPixelFormatDesc(R8Unorm, Color8, All);
	addMTLPixelFormatDescSRGB(R8Unorm_sRGB, Color8, All, R8Unorm);
	addMTLPixelFormatDesc(R8Snorm, Color8, All);
	addMTLPixelFormatDesc(R8Uint, Color8, RWCM);
	addMTLPixelFormatDesc(R8Sint, Color8, RWCM);

	// Ordinary 16-bit pixel formats
	addMTLPixelFormatDesc(R16Unorm, Color16, RFWCMB);
	addMTLPixelFormatDesc(R16Snorm, Color16, RFWCMB);
	addMTLPixelFormatDesc(R16Uint, Color16, RWCM);
	addMTLPixelFormatDesc(R16Sint, Color16, RWCM);
	addMTLPixelFormatDesc(R16Float, Color16, All);

	addMTLPixelFormatDesc(RG8Unorm, Color16, All);
	addMTLPixelFormatDescSRGB(RG8Unorm_sRGB, Color16, All, RG8Unorm);
	addMTLPixelFormatDesc(RG8Snorm, Color16, All);
	addMTLPixelFormatDesc(RG8Uint, Color16, RWCM);
	addMTLPixelFormatDesc(RG8Sint, Color16, RWCM);

	// Packed 16-bit pixel formats
	addMTLPixelFormatDesc(B5G6R5Unorm, Color16, RFCMRB);
	addMTLPixelFormatDesc(A1BGR5Unorm, Color16, RFCMRB);
	addMTLPixelFormatDesc(ABGR4Unorm, Color16, RFCMRB);
	addMTLPixelFormatDesc(BGR5A1Unorm, Color16, RFCMRB);

	// Ordinary 32-bit pixel formats
	addMTLPixelFormatDesc(R32Uint, Color32, RWC);
	addMTLPixelFormatDesc(R32Sint, Color32, RWC);
	addMTLPixelFormatDesc(R32Float, Color32, All);

	addMTLPixelFormatDesc(RG16Unorm, Color32, RFWCMB);
	addMTLPixelFormatDesc(RG16Snorm, Color32, RFWCMB);
	addMTLPixelFormatDesc(RG16Uint, Color32, RWCM);
	addMTLPixelFormatDesc(RG16Sint, Color32, RWCM);
	addMTLPixelFormatDesc(RG16Float, Color32, All);

	addMTLPixelFormatDesc(RGBA8Unorm, Color32, All);
	addMTLPixelFormatDescSRGB(RGBA8Unorm_sRGB, Color32, All, RGBA8Unorm);
	addMTLPixelFormatDesc(RGBA8Snorm, Color32, All);
	addMTLPixelFormatDesc(RGBA8Uint, Color32, RWCM);
	addMTLPixelFormatDesc(RGBA8Sint, Color32, RWCM);

	addMTLPixelFormatDesc(BGRA8Unorm, Color32, All);
	addMTLPixelFormatDescSRGB(BGRA8Unorm_sRGB, Color32, All, BGRA8Unorm);

	// Packed 32-bit pixel formats
	addMTLPixelFormatDesc(RGB10A2Unorm, Color32, All);
	addMTLPixelFormatDesc(BGR10A2Unorm, Color32, All);
	addMTLPixelFormatDesc(RGB10A2Uint, Color32, RWCM);
	addMTLPixelFormatDesc(RG11B10Float, Color32, All);
	addMTLPixelFormatDesc(RGB9E5Float, Color32, All);

	// Ordinary 64-bit pixel formats
	addMTLPixelFormatDesc(RG32Uint, Color64, RWCM);
	addMTLPixelFormatDesc(RG32Sint, Color64, RWCM);
	addMTLPixelFormatDesc(RG32Float, Color64, All);

	addMTLPixelFormatDesc(RGBA16Unorm, Color64, RFWCMB);
	addMTLPixelFormatDesc(RGBA16Snorm, Color64, RFWCMB);
	addMTLPixelFormatDesc(RGBA16Uint, Color64, RWCM);
	addMTLPixelFormatDesc(RGBA16Sint, Color64, RWCM);
	addMTLPixelFormatDesc(RGBA16Float, Color64, All);

	// Ordinary 128-bit pixel formats
	addMTLPixelFormatDesc(RGBA32Uint, Color128, RWC);
	addMTLPixelFormatDesc(RGBA32Sint, Color128, RWC);
	addMTLPixelFormatDesc(RGBA32Float, Color128, All);

#if !defined(VISIONOS_ENABLED)
	GODOT_CLANG_WARNING_PUSH_AND_IGNORE("-Wdeprecated-declarations")
	// Compressed pixel formats
	addMTLPixelFormatDesc(PVRTC_RGBA_2BPP, PVRTC_RGBA_2BPP, RF);
	addMTLPixelFormatDescSRGB(PVRTC_RGBA_2BPP_sRGB, PVRTC_RGBA_2BPP, RF, PVRTC_RGBA_2BPP);
	addMTLPixelFormatDesc(PVRTC_RGBA_4BPP, PVRTC_RGBA_4BPP, RF);
	addMTLPixelFormatDescSRGB(PVRTC_RGBA_4BPP_sRGB, PVRTC_RGBA_4BPP, RF, PVRTC_RGBA_4BPP);
	GODOT_CLANG_WARNING_POP
#endif

	addMTLPixelFormatDesc(ETC2_RGB8, ETC2_RGB8, RF);
	addMTLPixelFormatDescSRGB(ETC2_RGB8_sRGB, ETC2_RGB8, RF, ETC2_RGB8);
	addMTLPixelFormatDesc(ETC2_RGB8A1, ETC2_RGB8A1, RF);
	addMTLPixelFormatDescSRGB(ETC2_RGB8A1_sRGB, ETC2_RGB8A1, RF, ETC2_RGB8A1);
	addMTLPixelFormatDesc(EAC_RGBA8, EAC_RGBA8, RF);
	addMTLPixelFormatDescSRGB(EAC_RGBA8_sRGB, EAC_RGBA8, RF, EAC_RGBA8);
	addMTLPixelFormatDesc(EAC_R11Unorm, EAC_R11, RF);
	addMTLPixelFormatDesc(EAC_R11Snorm, EAC_R11, RF);
	addMTLPixelFormatDesc(EAC_RG11Unorm, EAC_RG11, RF);
	addMTLPixelFormatDesc(EAC_RG11Snorm, EAC_RG11, RF);

	addMTLPixelFormatDesc(ASTC_4x4_LDR, ASTC_4x4, RF);
	addMTLPixelFormatDescSRGB(ASTC_4x4_sRGB, ASTC_4x4, RF, ASTC_4x4_LDR);
	addMTLPixelFormatDesc(ASTC_4x4_HDR, ASTC_4x4, RF);
	addMTLPixelFormatDesc(ASTC_5x4_LDR, ASTC_5x4, RF);
	addMTLPixelFormatDescSRGB(ASTC_5x4_sRGB, ASTC_5x4, RF, ASTC_5x4_LDR);
	addMTLPixelFormatDesc(ASTC_5x4_HDR, ASTC_5x4, RF);
	addMTLPixelFormatDesc(ASTC_5x5_LDR, ASTC_5x5, RF);
	addMTLPixelFormatDescSRGB(ASTC_5x5_sRGB, ASTC_5x5, RF, ASTC_5x5_LDR);
	addMTLPixelFormatDesc(ASTC_5x5_HDR, ASTC_5x5, RF);
	addMTLPixelFormatDesc(ASTC_6x5_LDR, ASTC_6x5, RF);
	addMTLPixelFormatDescSRGB(ASTC_6x5_sRGB, ASTC_6x5, RF, ASTC_6x5_LDR);
	addMTLPixelFormatDesc(ASTC_6x5_HDR, ASTC_6x5, RF);
	addMTLPixelFormatDesc(ASTC_6x6_LDR, ASTC_6x6, RF);
	addMTLPixelFormatDescSRGB(ASTC_6x6_sRGB, ASTC_6x6, RF, ASTC_6x6_LDR);
	addMTLPixelFormatDesc(ASTC_6x6_HDR, ASTC_6x6, RF);
	addMTLPixelFormatDesc(ASTC_8x5_LDR, ASTC_8x5, RF);
	addMTLPixelFormatDescSRGB(ASTC_8x5_sRGB, ASTC_8x5, RF, ASTC_8x5_LDR);
	addMTLPixelFormatDesc(ASTC_8x5_HDR, ASTC_8x5, RF);
	addMTLPixelFormatDesc(ASTC_8x6_LDR, ASTC_8x6, RF);
	addMTLPixelFormatDescSRGB(ASTC_8x6_sRGB, ASTC_8x6, RF, ASTC_8x6_LDR);
	addMTLPixelFormatDesc(ASTC_8x6_HDR, ASTC_8x6, RF);
	addMTLPixelFormatDesc(ASTC_8x8_LDR, ASTC_8x8, RF);
	addMTLPixelFormatDescSRGB(ASTC_8x8_sRGB, ASTC_8x8, RF, ASTC_8x8_LDR);
	addMTLPixelFormatDesc(ASTC_8x8_HDR, ASTC_8x8, RF);
	addMTLPixelFormatDesc(ASTC_10x5_LDR, ASTC_10x5, RF);
	addMTLPixelFormatDescSRGB(ASTC_10x5_sRGB, ASTC_10x5, RF, ASTC_10x5_LDR);
	addMTLPixelFormatDesc(ASTC_10x5_HDR, ASTC_10x5, RF);
	addMTLPixelFormatDesc(ASTC_10x6_LDR, ASTC_10x6, RF);
	addMTLPixelFormatDescSRGB(ASTC_10x6_sRGB, ASTC_10x6, RF, ASTC_10x6_LDR);
	addMTLPixelFormatDesc(ASTC_10x6_HDR, ASTC_10x6, RF);
	addMTLPixelFormatDesc(ASTC_10x8_LDR, ASTC_10x8, RF);
	addMTLPixelFormatDescSRGB(ASTC_10x8_sRGB, ASTC_10x8, RF, ASTC_10x8_LDR);
	addMTLPixelFormatDesc(ASTC_10x8_HDR, ASTC_10x8, RF);
	addMTLPixelFormatDesc(ASTC_10x10_LDR, ASTC_10x10, RF);
	addMTLPixelFormatDescSRGB(ASTC_10x10_sRGB, ASTC_10x10, RF, ASTC_10x10_LDR);
	addMTLPixelFormatDesc(ASTC_10x10_HDR, ASTC_10x10, RF);
	addMTLPixelFormatDesc(ASTC_12x10_LDR, ASTC_12x10, RF);
	addMTLPixelFormatDescSRGB(ASTC_12x10_sRGB, ASTC_12x10, RF, ASTC_12x10_LDR);
	addMTLPixelFormatDesc(ASTC_12x10_HDR, ASTC_12x10, RF);
	addMTLPixelFormatDesc(ASTC_12x12_LDR, ASTC_12x12, RF);
	addMTLPixelFormatDescSRGB(ASTC_12x12_sRGB, ASTC_12x12, RF, ASTC_12x12_LDR);
	addMTLPixelFormatDesc(ASTC_12x12_HDR, ASTC_12x12, RF);

	GODOT_CLANG_WARNING_PUSH_AND_IGNORE("-Wunguarded-availability")

	addMTLPixelFormatDesc(BC1_RGBA, BC1_RGBA, RF);
	addMTLPixelFormatDescSRGB(BC1_RGBA_sRGB, BC1_RGBA, RF, BC1_RGBA);
	addMTLPixelFormatDesc(BC2_RGBA, BC2_RGBA, RF);
	addMTLPixelFormatDescSRGB(BC2_RGBA_sRGB, BC2_RGBA, RF, BC2_RGBA);
	addMTLPixelFormatDesc(BC3_RGBA, BC3_RGBA, RF);
	addMTLPixelFormatDescSRGB(BC3_RGBA_sRGB, BC3_RGBA, RF, BC3_RGBA);
	addMTLPixelFormatDesc(BC4_RUnorm, BC4_R, RF);
	addMTLPixelFormatDesc(BC4_RSnorm, BC4_R, RF);
	addMTLPixelFormatDesc(BC5_RGUnorm, BC5_RG, RF);
	addMTLPixelFormatDesc(BC5_RGSnorm, BC5_RG, RF);
	addMTLPixelFormatDesc(BC6H_RGBUfloat, BC6H_RGB, RF);
	addMTLPixelFormatDesc(BC6H_RGBFloat, BC6H_RGB, RF);
	addMTLPixelFormatDesc(BC7_RGBAUnorm, BC7_RGBA, RF);
	addMTLPixelFormatDescSRGB(BC7_RGBAUnorm_sRGB, BC7_RGBA, RF, BC7_RGBAUnorm);

	GODOT_CLANG_WARNING_POP

	// YUV pixel formats
	addMTLPixelFormatDesc(GBGR422, None, RF);
	addMTLPixelFormatDesc(BGRG422, None, RF);

	// Extended range and wide color pixel formats
	addMTLPixelFormatDesc(BGRA10_XR, BGRA10_XR, All);
	addMTLPixelFormatDescSRGB(BGRA10_XR_sRGB, BGRA10_XR, All, BGRA10_XR);
	addMTLPixelFormatDesc(BGR10_XR, BGR10_XR, All);
	addMTLPixelFormatDescSRGB(BGR10_XR_sRGB, BGR10_XR, All, BGR10_XR);

	// Depth and stencil pixel formats
	addMTLPixelFormatDesc(Depth16Unorm, None, DRFMR);
	addMTLPixelFormatDesc(Depth32Float, None, DRMR);
	addMTLPixelFormatDesc(Stencil8, None, DRM);
	addMTLPixelFormatDesc(Depth24Unorm_Stencil8, Depth24_Stencil8, None);
	addMTLPixelFormatDesc(Depth32Float_Stencil8, Depth32_Stencil8, DRMR);
	addMTLPixelFormatDesc(X24_Stencil8, Depth24_Stencil8, None);
	addMTLPixelFormatDesc(X32_Stencil8, Depth32_Stencil8, DRM);
}

// If necessary, resize vector with empty elements.
void PixelFormats::addMTLVertexFormatDescImpl(MTL::VertexFormat mtlVtxFmt, MTLFmtCaps vtxCap, const char *name) {
	if (mtlVtxFmt >= _mtl_vertex_format_descs.size()) {
		_mtl_vertex_format_descs.resize(mtlVtxFmt + 1);
	}
	_mtl_vertex_format_descs[mtlVtxFmt] = { .mtlVertexFormat = mtlVtxFmt, .dataFormat = RDC::DATA_FORMAT_MAX, .mtlFmtCaps = vtxCap, .mtlViewClass = MTLViewClass::None, .mtlPixelFormatLinear = MTL::PixelFormatInvalid, .name = name };
}

// Check mtlVtx exists on platform, to avoid overwriting the MTL::VertexFormatInvalid entry.
#define addMTLVertexFormatDesc(mtlVtx) \
	if (MTL::VertexFormat##mtlVtx) { \
		addMTLVertexFormatDescImpl(MTL::VertexFormat##mtlVtx, kMTLFmtCapsVertex, "MTL::VertexFormat" #mtlVtx); \
	}

void PixelFormats::initMTLVertexFormatCapabilities(const MetalFeatures &p_feat) {
	_mtl_vertex_format_descs.resize(MTL::VertexFormatHalf + 3);
	// MTL::VertexFormatInvalid must come first. Use addMTLVertexFormatDescImpl to avoid guard code.
	addMTLVertexFormatDescImpl(MTL::VertexFormatInvalid, kMTLFmtCapsNone, "MTL::VertexFormatInvalid");

	addMTLVertexFormatDesc(UChar2Normalized);
	addMTLVertexFormatDesc(Char2Normalized);
	addMTLVertexFormatDesc(UChar2);
	addMTLVertexFormatDesc(Char2);

	addMTLVertexFormatDesc(UChar3Normalized);
	addMTLVertexFormatDesc(Char3Normalized);
	addMTLVertexFormatDesc(UChar3);
	addMTLVertexFormatDesc(Char3);

	addMTLVertexFormatDesc(UChar4Normalized);
	addMTLVertexFormatDesc(Char4Normalized);
	addMTLVertexFormatDesc(UChar4);
	addMTLVertexFormatDesc(Char4);

	addMTLVertexFormatDesc(UInt1010102Normalized);
	addMTLVertexFormatDesc(Int1010102Normalized);

	addMTLVertexFormatDesc(UShort2Normalized);
	addMTLVertexFormatDesc(Short2Normalized);
	addMTLVertexFormatDesc(UShort2);
	addMTLVertexFormatDesc(Short2);
	addMTLVertexFormatDesc(Half2);

	addMTLVertexFormatDesc(UShort3Normalized);
	addMTLVertexFormatDesc(Short3Normalized);
	addMTLVertexFormatDesc(UShort3);
	addMTLVertexFormatDesc(Short3);
	addMTLVertexFormatDesc(Half3);

	addMTLVertexFormatDesc(UShort4Normalized);
	addMTLVertexFormatDesc(Short4Normalized);
	addMTLVertexFormatDesc(UShort4);
	addMTLVertexFormatDesc(Short4);
	addMTLVertexFormatDesc(Half4);

	addMTLVertexFormatDesc(UInt);
	addMTLVertexFormatDesc(Int);
	addMTLVertexFormatDesc(Float);

	addMTLVertexFormatDesc(UInt2);
	addMTLVertexFormatDesc(Int2);
	addMTLVertexFormatDesc(Float2);

	addMTLVertexFormatDesc(UInt3);
	addMTLVertexFormatDesc(Int3);
	addMTLVertexFormatDesc(Float3);

	addMTLVertexFormatDesc(UInt4);
	addMTLVertexFormatDesc(Int4);
	addMTLVertexFormatDesc(Float4);

	addMTLVertexFormatDesc(UCharNormalized);
	addMTLVertexFormatDesc(CharNormalized);
	addMTLVertexFormatDesc(UChar);
	addMTLVertexFormatDesc(Char);

	addMTLVertexFormatDesc(UShortNormalized);
	addMTLVertexFormatDesc(ShortNormalized);
	addMTLVertexFormatDesc(UShort);
	addMTLVertexFormatDesc(Short);
	addMTLVertexFormatDesc(Half);

	addMTLVertexFormatDesc(UChar4Normalized_BGRA);

	if (__builtin_available(macos 14.0, ios 17.0, tvos 17.0, *)) {
		if (p_feat.highestFamily >= MTL::GPUFamilyApple5) {
			addMTLVertexFormatDesc(FloatRG11B10);
			addMTLVertexFormatDesc(FloatRGB9E5);
		}
	}
}

// Return a reference to the format capabilities, so the caller can manipulate them.
// Check mtlPixFmt exists on platform, to avoid overwriting the MTL::PixelFormatInvalid entry.
// When returning the dummy, reset it on each access because it can be written to by caller.
MTLFmtCaps &PixelFormats::getMTLPixelFormatCapsIf(MTL::PixelFormat mtlPixFmt, bool cond) {
	static MTLFmtCaps dummyFmtCaps;
	if (mtlPixFmt && cond) {
		return getMTLPixelFormatDesc(mtlPixFmt).mtlFmtCaps;
	} else {
		dummyFmtCaps = kMTLFmtCapsNone;
		return dummyFmtCaps;
	}
}

#define setMTLPixFmtCapsIf(cond, mtlFmt, caps) getMTLPixelFormatCapsIf(MTL::PixelFormat##mtlFmt, cond) = kMTLFmtCaps##caps;
#define setMTLPixFmtCapsIfGPU(gpuFam, mtlFmt, caps) setMTLPixFmtCapsIf(gpuCaps.supports##gpuFam, mtlFmt, caps)

#define enableMTLPixFmtCapsIf(cond, mtlFmt, caps) flags::set(getMTLPixelFormatCapsIf(MTL::PixelFormat##mtlFmt, cond), kMTLFmtCaps##caps);
#define enableMTLPixFmtCapsIfGPU(gpuFam, mtlFmt, caps) enableMTLPixFmtCapsIf(p_feat.highestFamily >= MTL::GPUFamily##gpuFam, mtlFmt, caps)

#define disableMTLPixFmtCapsIf(cond, mtlFmt, caps) flags::clear(getMTLPixelFormatCapsIf(MTL::PixelFormat##mtlFmt, cond), kMTLFmtCaps##caps);

// Modifies the format capability tables based on the capabilities of the specific MTLDevice.
void PixelFormats::modifyMTLFormatCapabilities(const MetalFeatures &p_feat) {
	bool noVulkanSupport = false; // Indicated supported in Metal but not Vulkan or SPIR-V.
	bool notMac = !p_feat.supportsMac;
	bool iosOnly1 = notMac && p_feat.highestFamily < MTL::GPUFamilyApple2;
	bool iosOnly2 = notMac && p_feat.highestFamily < MTL::GPUFamilyApple3;
	bool iosOnly6 = notMac && p_feat.highestFamily < MTL::GPUFamilyApple7;
	bool iosOnly8 = notMac && p_feat.highestFamily < MTL::GPUFamilyApple9;

	setMTLPixFmtCapsIf(iosOnly2, A8Unorm, RF);
	setMTLPixFmtCapsIf(iosOnly1, R8Unorm_sRGB, RFCMRB);
	setMTLPixFmtCapsIf(iosOnly1, R8Snorm, RFWCMB);

	setMTLPixFmtCapsIf(iosOnly1, RG8Unorm_sRGB, RFCMRB);
	setMTLPixFmtCapsIf(iosOnly1, RG8Snorm, RFWCMB);

	enableMTLPixFmtCapsIfGPU(Apple6, R32Uint, Atomic);
	enableMTLPixFmtCapsIfGPU(Apple6, R32Sint, Atomic);

	setMTLPixFmtCapsIf(iosOnly8, R32Float, RWCMB);

	setMTLPixFmtCapsIf(iosOnly1, RGBA8Unorm_sRGB, RFCMRB);
	setMTLPixFmtCapsIf(iosOnly1, RGBA8Snorm, RFWCMB);
	setMTLPixFmtCapsIf(iosOnly1, BGRA8Unorm_sRGB, RFCMRB);

	setMTLPixFmtCapsIf(iosOnly2, RGB10A2Unorm, RFCMRB);
	setMTLPixFmtCapsIf(iosOnly2, RGB10A2Uint, RCM);
	setMTLPixFmtCapsIf(iosOnly2, RG11B10Float, RFCMRB);
	setMTLPixFmtCapsIf(iosOnly2, RGB9E5Float, RFCMRB);

	// Blending is actually supported for RGB9E5Float, but format channels cannot
	// be individually write-enabled during blending on macOS. Disabling blending
	// on macOS is the least-intrusive way to handle this in a Vulkan-friendly way.
	disableMTLPixFmtCapsIf(p_feat.supportsMac, RGB9E5Float, Blend);

	// RGB9E5Float cannot be used as a render target on the simulator.
	disableMTLPixFmtCapsIf(TARGET_OS_SIMULATOR, RGB9E5Float, ColorAtt);

	setMTLPixFmtCapsIf(iosOnly6, RG32Uint, RWC);
	setMTLPixFmtCapsIf(iosOnly6, RG32Sint, RWC);

	// Metal supports reading both R&G into as one 64-bit atomic operation, but Vulkan and SPIR-V do not.
	// Including this here so we remember to update this if support is added to Vulkan in the future.
	bool atomic64 = noVulkanSupport && (p_feat.highestFamily >= MTL::GPUFamilyApple9 || (p_feat.highestFamily >= MTL::GPUFamilyApple8 && p_feat.supportsMac));
	enableMTLPixFmtCapsIf(atomic64, RG32Uint, Atomic);
	enableMTLPixFmtCapsIf(atomic64, RG32Sint, Atomic);

	setMTLPixFmtCapsIf(iosOnly8, RG32Float, RWCMB);
	setMTLPixFmtCapsIf(iosOnly6, RG32Float, RWCB);

	setMTLPixFmtCapsIf(iosOnly8, RGBA32Float, RWCM);
	setMTLPixFmtCapsIf(iosOnly6, RGBA32Float, RWC);

	bool msaa32 = p_feat.supports32BitMSAA;
	enableMTLPixFmtCapsIf(msaa32, R32Uint, MSAA);
	enableMTLPixFmtCapsIf(msaa32, R32Sint, MSAA);
	enableMTLPixFmtCapsIf(msaa32, R32Float, Resolve);
	enableMTLPixFmtCapsIf(msaa32, RG32Uint, MSAA);
	enableMTLPixFmtCapsIf(msaa32, RG32Sint, MSAA);
	enableMTLPixFmtCapsIf(msaa32, RG32Float, Resolve);
	enableMTLPixFmtCapsIf(msaa32, RGBA32Uint, MSAA);
	enableMTLPixFmtCapsIf(msaa32, RGBA32Sint, MSAA);
	enableMTLPixFmtCapsIf(msaa32, RGBA32Float, Resolve);

	bool floatFB = p_feat.supports32BitFloatFiltering;
	enableMTLPixFmtCapsIf(floatFB, R32Float, Filter);
	enableMTLPixFmtCapsIf(floatFB, RG32Float, Filter);
	enableMTLPixFmtCapsIf(floatFB, RGBA32Float, Filter);
	enableMTLPixFmtCapsIf(floatFB, RGBA32Float, Blend); // Undocumented by confirmed through testing.

	bool noHDR_ASTC = p_feat.highestFamily < MTL::GPUFamilyApple6;
	setMTLPixFmtCapsIf(noHDR_ASTC, ASTC_4x4_HDR, None);
	setMTLPixFmtCapsIf(noHDR_ASTC, ASTC_5x4_HDR, None);
	setMTLPixFmtCapsIf(noHDR_ASTC, ASTC_5x5_HDR, None);
	setMTLPixFmtCapsIf(noHDR_ASTC, ASTC_6x5_HDR, None);
	setMTLPixFmtCapsIf(noHDR_ASTC, ASTC_6x6_HDR, None);
	setMTLPixFmtCapsIf(noHDR_ASTC, ASTC_8x5_HDR, None);
	setMTLPixFmtCapsIf(noHDR_ASTC, ASTC_8x6_HDR, None);
	setMTLPixFmtCapsIf(noHDR_ASTC, ASTC_8x8_HDR, None);
	setMTLPixFmtCapsIf(noHDR_ASTC, ASTC_10x5_HDR, None);
	setMTLPixFmtCapsIf(noHDR_ASTC, ASTC_10x6_HDR, None);
	setMTLPixFmtCapsIf(noHDR_ASTC, ASTC_10x8_HDR, None);
	setMTLPixFmtCapsIf(noHDR_ASTC, ASTC_10x10_HDR, None);
	setMTLPixFmtCapsIf(noHDR_ASTC, ASTC_12x10_HDR, None);
	setMTLPixFmtCapsIf(noHDR_ASTC, ASTC_12x12_HDR, None);

	GODOT_CLANG_WARNING_PUSH_AND_IGNORE("-Wunguarded-availability")

	bool noBC = !p_feat.supportsBCTextureCompression;
	setMTLPixFmtCapsIf(noBC, BC1_RGBA, None);
	setMTLPixFmtCapsIf(noBC, BC1_RGBA_sRGB, None);
	setMTLPixFmtCapsIf(noBC, BC2_RGBA, None);
	setMTLPixFmtCapsIf(noBC, BC2_RGBA_sRGB, None);
	setMTLPixFmtCapsIf(noBC, BC3_RGBA, None);
	setMTLPixFmtCapsIf(noBC, BC3_RGBA_sRGB, None);
	setMTLPixFmtCapsIf(noBC, BC4_RUnorm, None);
	setMTLPixFmtCapsIf(noBC, BC4_RSnorm, None);
	setMTLPixFmtCapsIf(noBC, BC5_RGUnorm, None);
	setMTLPixFmtCapsIf(noBC, BC5_RGSnorm, None);
	setMTLPixFmtCapsIf(noBC, BC6H_RGBUfloat, None);
	setMTLPixFmtCapsIf(noBC, BC6H_RGBFloat, None);
	setMTLPixFmtCapsIf(noBC, BC7_RGBAUnorm, None);
	setMTLPixFmtCapsIf(noBC, BC7_RGBAUnorm_sRGB, None);

	GODOT_CLANG_WARNING_POP

	setMTLPixFmtCapsIf(iosOnly2, BGRA10_XR, None);
	setMTLPixFmtCapsIf(iosOnly2, BGRA10_XR_sRGB, None);
	setMTLPixFmtCapsIf(iosOnly2, BGR10_XR, None);
	setMTLPixFmtCapsIf(iosOnly2, BGR10_XR_sRGB, None);

	setMTLPixFmtCapsIf(iosOnly2, Depth16Unorm, DRFM);
	setMTLPixFmtCapsIf(iosOnly2, Depth32Float, DRM);

	setMTLPixFmtCapsIf(!p_feat.supportsDepth24Stencil8, Depth24Unorm_Stencil8, None);
	setMTLPixFmtCapsIf(iosOnly2, Depth32Float_Stencil8, DRM);
}

// Populates the DataFormat lookup maps and connects Godot and Metal pixel formats to one-another.
void PixelFormats::buildDFFormatMaps() {
	for (DataFormatDesc &dfDesc : _data_format_descs) {
		// Populate the back reference from the Metal formats to the Godot format.
		// Validate the corresponding Metal formats for the platform, and clear them
		// in the Godot format if not supported.
		if (dfDesc.mtlPixelFormat) {
			MTLFormatDesc &mtlDesc = getMTLPixelFormatDesc(dfDesc.mtlPixelFormat);
			if (mtlDesc.dataFormat == RDC::DATA_FORMAT_MAX) {
				mtlDesc.dataFormat = dfDesc.dataFormat;
			}
			if (!mtlDesc.isSupported()) {
				dfDesc.mtlPixelFormat = MTL::PixelFormatInvalid;
			}
		}
		if (dfDesc.mtlPixelFormatSubstitute) {
			MTLFormatDesc &mtlDesc = getMTLPixelFormatDesc(dfDesc.mtlPixelFormatSubstitute);
			if (!mtlDesc.isSupported()) {
				dfDesc.mtlPixelFormatSubstitute = MTL::PixelFormatInvalid;
			}
		}
		if (dfDesc.mtlVertexFormat) {
			MTLFormatDesc &mtlDesc = getMTLVertexFormatDesc(dfDesc.mtlVertexFormat);
			if (mtlDesc.dataFormat == RDC::DATA_FORMAT_MAX) {
				mtlDesc.dataFormat = dfDesc.dataFormat;
			}
			if (!mtlDesc.isSupported()) {
				dfDesc.mtlVertexFormat = MTL::VertexFormatInvalid;
			}
		}
		if (dfDesc.mtlVertexFormatSubstitute) {
			MTLFormatDesc &mtlDesc = getMTLVertexFormatDesc(dfDesc.mtlVertexFormatSubstitute);
			if (!mtlDesc.isSupported()) {
				dfDesc.mtlVertexFormatSubstitute = MTL::VertexFormatInvalid;
			}
		}
	}
}


// --- Merged from drivers/metal/rendering_context_driver_metal.cpp ---
#include "drivers/metal/rendering_context_driver_metal.h"

#include "core/os/os.h"
#include "core/templates/sort_array.h"
#include "drivers/metal/rendering_device_driver_metal3.h"

#include <objc/message.h>
#include <os/log.h>
#include <os/signpost.h>

#pragma mark - Logging

os_log_t LOG_DRIVER;
// Used for dynamic tracing.
os_log_t LOG_INTERVALS;

__attribute__((constructor)) static void InitializeLogging(void) {
	LOG_DRIVER = os_log_create("org.godotengine.godot.metal", OS_LOG_CATEGORY_POINTS_OF_INTEREST);
	LOG_INTERVALS = os_log_create("org.godotengine.godot.metal", "events");
}

RenderingContextDriverMetal::RenderingContextDriverMetal() {
}

RenderingContextDriverMetal::~RenderingContextDriverMetal() {
}

Error RenderingContextDriverMetal::initialize() {
	if (OS::get_environment("MTL_CAPTURE_ENABLED") == "1" || OS::get_environment("MTLCAPTURE_DESTINATION_DEVELOPER_TOOLS_ENABLE") == "1") {
		capture_available = true;
	}

	metal_device = MTL::CreateSystemDefaultDevice();
#if TARGET_OS_OSX
	if (__builtin_available(macOS 13.3, *)) {
		metal_device->setShouldMaximizeConcurrentCompilation(true);
	}
#endif
	device.type = DEVICE_TYPE_INTEGRATED_GPU;
	device.vendor = Vendor::VENDOR_APPLE;

	MetalDeviceProperties props(metal_device);
	int version = (int)props.features.highestFamily - (int)MTL::GPUFamilyApple1 + 1;
	device.name = vformat("%s (Apple%d)", metal_device->name()->utf8String(), version);

	return OK;
}

const RenderingContextDriver::Device &RenderingContextDriverMetal::device_get(uint32_t p_device_index) const {
	DEV_ASSERT(p_device_index < 1);
	return device;
}

uint32_t RenderingContextDriverMetal::device_get_count() const {
	return 1;
}

RenderingDeviceDriver *RenderingContextDriverMetal::driver_create() {
	return memnew(MTL3::RenderingDeviceDriverMetal(this));
}

void RenderingContextDriverMetal::driver_free(RenderingDeviceDriver *p_driver) {
	memdelete(p_driver);
}

class API_AVAILABLE(macos(11.0), ios(14.0), tvos(14.0)) SurfaceLayer : public RenderingContextDriverMetal::Surface {
	CA::MetalLayer *layer = nullptr;
	LocalVector<MDFrameBuffer> frame_buffers;
	LocalVector<MTL::Drawable *> drawables;
	uint32_t rear = -1;
	uint32_t front = 0;
	uint32_t count = 0;

public:
	SurfaceLayer(CA::MetalLayer *p_layer, MTL::Device *p_device) :
			Surface(p_device), layer(p_layer) {
		layer->setAllowsNextDrawableTimeout(true);
		layer->setFramebufferOnly(true);
		layer->setOpaque(!OS::is_layered_allowed());
		layer->setPixelFormat(get_pixel_format());
		layer->setDevice(p_device);
	}

	~SurfaceLayer() override {
		layer = nullptr;
	}

	Error resize(uint32_t p_desired_framebuffer_count, RDD::DataFormat &r_format, RDD::ColorSpace &r_color_space) override final {
		if (width == 0 || height == 0) {
			// Very likely the window is minimized, don't create a swap chain.
			return ERR_SKIP;
		}

		CGSize drawableSize = CGSizeMake(width, height);
		CGSize current = layer->drawableSize();
		if (!CGSizeEqualToSize(current, drawableSize)) {
			layer->setDrawableSize(drawableSize);
		}

		// Metal supports a maximum of 3 drawables.
		p_desired_framebuffer_count = MIN(3U, p_desired_framebuffer_count);
		layer->setMaximumDrawableCount(p_desired_framebuffer_count);

#if TARGET_OS_OSX
		// Display sync is only supported on macOS.
		switch (vsync_mode) {
			case DisplayServerEnums::VSYNC_MAILBOX:
			case DisplayServerEnums::VSYNC_ADAPTIVE:
			case DisplayServerEnums::VSYNC_ENABLED:
				layer->setDisplaySyncEnabled(true);
				break;
			case DisplayServerEnums::VSYNC_DISABLED:
				layer->setDisplaySyncEnabled(false);
				break;
		}
#endif
		drawables.resize(p_desired_framebuffer_count);
		frame_buffers.resize(p_desired_framebuffer_count);
		for (uint32_t i = 0; i < p_desired_framebuffer_count; i++) {
			// Reserve space for the drawable texture.
			frame_buffers[i].set_texture_count(1);
		}

		if (hdr_output) {
			layer->setWantsExtendedDynamicRangeContent(true);
			CGColorSpaceRef color_space = CGColorSpaceCreateWithName(kCGColorSpaceExtendedLinearSRGB);
			layer->setColorspace(color_space);
			CGColorSpaceRelease(color_space);
			layer->setPixelFormat(MTL::PixelFormatRGBA16Float);

			r_color_space = RDD::COLOR_SPACE_REC709_LINEAR;
			r_format = RDD::DATA_FORMAT_R16G16B16A16_SFLOAT;
			pixel_format = MTL::PixelFormatRGBA16Float;
		} else {
			layer->setWantsExtendedDynamicRangeContent(false);
			CGColorSpaceRef color_space = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
			layer->setColorspace(color_space);
			CGColorSpaceRelease(color_space);
			layer->setPixelFormat(MTL::PixelFormatBGRA8Unorm);

			r_color_space = RDD::COLOR_SPACE_REC709_NONLINEAR_SRGB;
			r_format = RDD::DATA_FORMAT_B8G8R8A8_UNORM;
			pixel_format = MTL::PixelFormatBGRA8Unorm;
		}

		return OK;
	}

	RDD::FramebufferID acquire_next_frame_buffer() override final {
		if (count == frame_buffers.size()) {
			return RDD::FramebufferID();
		}

		rear = (rear + 1) % frame_buffers.size();
		count++;

		MDFrameBuffer &frame_buffer = frame_buffers[rear];
		frame_buffer.size = Size2i(width, height);

		CA::MetalDrawable *drawable = layer->nextDrawable();
		ERR_FAIL_NULL_V_MSG(drawable, RDD::FramebufferID(), "no drawable available");
		drawables[rear] = drawable;
		frame_buffer.set_texture(0, drawable->texture());

		return RDD::FramebufferID(&frame_buffer);
	}

	void present(MTL3::MDCommandBuffer *p_cmd_buffer) override final {
		if (count == 0) {
			return;
		}

		// Release texture and drawable.
		frame_buffers[front].unset_texture(0);
		MTL::Drawable *drawable = drawables[front];
		drawables[front] = nullptr;

		count--;
		front = (front + 1) % frame_buffers.size();

		if (vsync_mode != DisplayServerEnums::VSYNC_DISABLED) {
			p_cmd_buffer->get_command_buffer()->presentDrawableAfterMinimumDuration(drawable, present_minimum_duration);
		} else {
			p_cmd_buffer->get_command_buffer()->presentDrawable(drawable);
		}
	}

	MTL::Drawable *next_drawable() override final {
		if (count == 0) {
			return nullptr;
		}

		// Release texture and drawable.
		frame_buffers[front].unset_texture(0);
		MTL::Drawable *drawable = drawables[front];
		drawables[front] = nullptr;

		count--;
		front = (front + 1) % frame_buffers.size();

		return drawable;
	}

	API_AVAILABLE(macos(26.0), ios(26.0))
	MTL::ResidencySet *get_residency_set() const override final {
		return layer->residencySet();
	}
};

class API_AVAILABLE(macos(11.0), ios(14.0), tvos(14.0)) SurfaceOffscreen : public RenderingContextDriverMetal::Surface {
	int frame_buffer_size = 3;
	MDFrameBuffer *frame_buffers;
	LocalVector<MTL::Texture *> textures;
	LocalVector<MTL::Drawable *> drawables;

	int32_t rear = -1;
	std::atomic_int count;
	uint64_t target_time = 0;
	CA::MetalLayer *layer;

public:
	SurfaceOffscreen(CA::MetalLayer *p_layer, MTL::Device *p_device) :
			Surface(p_device), layer(p_layer) {
		layer->setAllowsNextDrawableTimeout(true);
		layer->setFramebufferOnly(true);
		layer->setOpaque(!OS::is_layered_allowed());
		layer->setPixelFormat(get_pixel_format());
		layer->setDevice(p_device);
#if TARGET_OS_OSX
		layer->setDisplaySyncEnabled(false);
#endif
		target_time = OS::get_ticks_usec();

		textures.resize(frame_buffer_size);
		drawables.resize(frame_buffer_size);

		frame_buffers = memnew_arr(MDFrameBuffer, frame_buffer_size);
		for (int i = 0; i < frame_buffer_size; i++) {
			frame_buffers[i].set_texture_count(1);
		}
	}

	~SurfaceOffscreen() override {
		memdelete_arr(frame_buffers);
		for (MTL::Texture *texture : textures) {
			if (texture) {
				texture->release();
			}
		}
	}

	Error resize(uint32_t p_desired_framebuffer_count, RDD::DataFormat &r_format, RDD::ColorSpace &r_color_space) override final {
		if (width == 0 || height == 0) {
			// Very likely the window is minimized, don't create a swap chain.
			return ERR_SKIP;
		}

		CGSize drawableSize = CGSizeMake(width, height);
		CGSize current = layer->drawableSize();
		if (!CGSizeEqualToSize(current, drawableSize)) {
			layer->setDrawableSize(drawableSize);
		}

		if (hdr_output) {
			layer->setWantsExtendedDynamicRangeContent(true);
			CGColorSpaceRef color_space = CGColorSpaceCreateWithName(kCGColorSpaceExtendedLinearSRGB);
			layer->setColorspace(color_space);
			CGColorSpaceRelease(color_space);
			layer->setPixelFormat(MTL::PixelFormatRGBA16Float);

			r_color_space = RDD::COLOR_SPACE_REC709_LINEAR;
			r_format = RDD::DATA_FORMAT_R16G16B16A16_SFLOAT;
			pixel_format = MTL::PixelFormatRGBA16Float;
		} else {
			layer->setWantsExtendedDynamicRangeContent(false);
			CGColorSpaceRef color_space = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
			layer->setColorspace(color_space);
			CGColorSpaceRelease(color_space);
			layer->setPixelFormat(MTL::PixelFormatBGRA8Unorm);

			r_color_space = RDD::COLOR_SPACE_REC709_NONLINEAR_SRGB;
			r_format = RDD::DATA_FORMAT_B8G8R8A8_UNORM;
			pixel_format = MTL::PixelFormatBGRA8Unorm;
		}

		return OK;
	}

	RDD::FramebufferID acquire_next_frame_buffer() override final {
		if (count.load(std::memory_order_relaxed) == 3) {
			// Wait for a frame to be presented.
			return RDD::FramebufferID();
		}

		rear = (rear + 1) % 3;
		count.fetch_add(1, std::memory_order_relaxed);

		MDFrameBuffer &frame_buffer = frame_buffers[rear];

		MTL::Texture *texture = textures[rear];
		if (texture == nullptr || texture->width() != width || texture->height() != height || texture->pixelFormat() != pixel_format) {
			MTL::TextureDescriptor *texture_descriptor = MTL::TextureDescriptor::texture2DDescriptor(get_pixel_format(), width, height, false);
			texture_descriptor->setUsage(MTL::TextureUsageRenderTarget);
			texture_descriptor->setHazardTrackingMode(MTL::HazardTrackingModeUntracked);
			texture_descriptor->setStorageMode(MTL::StorageModePrivate);
			if (texture) {
				texture->release();
			}
			texture = device->newTexture(texture_descriptor);
			textures[rear] = texture;
		}

		frame_buffer.size = Size2i(width, height);
		uint64_t now = OS::get_ticks_usec();
		if (now >= target_time) {
			target_time = now + 1'000'000; // 1 second into the future.
			CA::MetalDrawable *drawable = layer->nextDrawable();
			ERR_FAIL_NULL_V_MSG(drawable, RDD::FramebufferID(), "no drawable available");
			drawables[rear] = drawable;
			frame_buffer.set_texture(0, drawable->texture());
		} else {
			frame_buffer.set_texture(0, texture);
		}

		return RDD::FramebufferID(&frame_buffers[rear]);
	}

	void present(MTL3::MDCommandBuffer *p_cmd_buffer) override final {
		MDFrameBuffer *frame_buffer = &frame_buffers[rear];

		if (drawables[rear] != nullptr) {
			p_cmd_buffer->get_command_buffer()->presentDrawable(drawables[rear]);
			drawables[rear] = nullptr;
		}

		p_cmd_buffer->get_command_buffer()->addScheduledHandler([frame_buffer, this](MTL::CommandBuffer *) {
			frame_buffer->unset_texture(0);
			count.fetch_add(-1, std::memory_order_relaxed);
		});
	}

	MTL::Drawable *next_drawable() override final {
		if (count == 0) {
			return nullptr;
		}

		MDFrameBuffer *frame_buffer = &frame_buffers[rear];

		MTL::Drawable *next = drawables[rear];
		drawables[rear] = nullptr;

		frame_buffer->unset_texture(0);
		count--;

		return next;
	}

	API_AVAILABLE(macos(26.0), ios(26.0))
	MTL::ResidencySet *get_residency_set() const override final {
		return layer->residencySet();
	}
};

RenderingContextDriver::SurfaceID RenderingContextDriverMetal::surface_create(const void *p_platform_data) {
	const WindowPlatformData *wpd = (const WindowPlatformData *)(p_platform_data);
	Surface *surface;
	if (String v = OS::get_environment("GODOT_MTL_OFF_SCREEN"); v == U"1") {
		surface = memnew(SurfaceOffscreen(wpd->layer, metal_device));
	} else {
		surface = memnew(SurfaceLayer(wpd->layer, metal_device));
	}

	return SurfaceID(surface);
}

void RenderingContextDriverMetal::surface_set_size(SurfaceID p_surface, uint32_t p_width, uint32_t p_height) {
	Surface *surface = (Surface *)(p_surface);
	if (surface->width == p_width && surface->height == p_height) {
		return;
	}
	surface->width = p_width;
	surface->height = p_height;
	surface->needs_resize = true;
}

void RenderingContextDriverMetal::surface_set_vsync_mode(SurfaceID p_surface, DisplayServerEnums::VSyncMode p_vsync_mode) {
	Surface *surface = (Surface *)(p_surface);
	if (surface->vsync_mode == p_vsync_mode) {
		return;
	}
	surface->vsync_mode = p_vsync_mode;
	surface->needs_resize = true;
}

DisplayServerEnums::VSyncMode RenderingContextDriverMetal::surface_get_vsync_mode(SurfaceID p_surface) const {
	Surface *surface = (Surface *)(p_surface);
	return surface->vsync_mode;
}

void RenderingContextDriverMetal::surface_set_hdr_output_enabled(SurfaceID p_surface, bool p_enabled) {
	Surface *surface = (Surface *)(p_surface);
	surface->set_hdr_output_enabled(p_enabled);
}

bool RenderingContextDriverMetal::surface_get_hdr_output_enabled(SurfaceID p_surface) const {
	Surface *surface = (Surface *)(p_surface);
	return surface->is_hdr_output_enabled();
}

void RenderingContextDriverMetal::surface_set_hdr_output_reference_luminance(SurfaceID p_surface, float p_reference_luminance) {
	Surface *surface = (Surface *)(p_surface);
	surface->hdr_reference_luminance = p_reference_luminance;
}

float RenderingContextDriverMetal::surface_get_hdr_output_reference_luminance(SurfaceID p_surface) const {
	const Surface *surface = (Surface *)(p_surface);
	return surface->hdr_reference_luminance;
}

void RenderingContextDriverMetal::surface_set_hdr_output_max_luminance(SurfaceID p_surface, float p_max_luminance) {
	Surface *surface = (Surface *)(p_surface);
	surface->hdr_max_luminance = p_max_luminance;
}

float RenderingContextDriverMetal::surface_get_hdr_output_max_luminance(SurfaceID p_surface) const {
	const Surface *surface = (Surface *)(p_surface);
	return surface->hdr_max_luminance;
}

void RenderingContextDriverMetal::surface_set_hdr_output_linear_luminance_scale(SurfaceID p_surface, float p_linear_luminance_scale) {
	Surface *surface = (Surface *)(p_surface);
	surface->hdr_linear_luminance_scale = p_linear_luminance_scale;
}

float RenderingContextDriverMetal::surface_get_hdr_output_linear_luminance_scale(SurfaceID p_surface) const {
	Surface *surface = (Surface *)(p_surface);
	return surface->hdr_linear_luminance_scale;
}

float RenderingContextDriverMetal::surface_get_hdr_output_max_value(SurfaceID p_surface) const {
	Surface *surface = (Surface *)(p_surface);
	return MAX(surface->hdr_max_luminance / MAX(surface->hdr_reference_luminance, 1.0f), 1.0f);
}

uint32_t RenderingContextDriverMetal::surface_get_width(SurfaceID p_surface) const {
	Surface *surface = (Surface *)(p_surface);
	return surface->width;
}

uint32_t RenderingContextDriverMetal::surface_get_height(SurfaceID p_surface) const {
	Surface *surface = (Surface *)(p_surface);
	return surface->height;
}

void RenderingContextDriverMetal::surface_set_needs_resize(SurfaceID p_surface, bool p_needs_resize) {
	Surface *surface = (Surface *)(p_surface);
	surface->needs_resize = p_needs_resize;
}

bool RenderingContextDriverMetal::surface_get_needs_resize(SurfaceID p_surface) const {
	Surface *surface = (Surface *)(p_surface);
	return surface->needs_resize;
}

void RenderingContextDriverMetal::surface_destroy(SurfaceID p_surface) {
	Surface *surface = (Surface *)(p_surface);
	memdelete(surface);
}


// --- Merged from drivers/metal/rendering_shader_container_metal.cpp ---
#include "drivers/metal/rendering_device_driver_metal3.h"

#include "core/io/file_access.h"
#include "core/io/marshalls.h"
#include "core/os/os.h"
#include "core/templates/span.h"
#include "drivers/metal/rendering_device_driver_metal3.h"

#include <thirdparty/spirv-reflect/spirv_reflect.h>

#include <Metal/Metal.hpp>
#include <spirv.hpp>
#include <spirv_msl.hpp>
#include <spirv_parser.hpp>

void RenderingShaderContainerMetal::_initialize_toolchain_properties() {
	if (compiler_props.is_valid()) {
		return;
	}

	String sdk;
	switch (device_profile->platform) {
		case MetalDeviceProfile::Platform::macOS:
			sdk = "macosx";
			break;
		case MetalDeviceProfile::Platform::iOS:
			sdk = "iphoneos";
			break;
		case MetalDeviceProfile::Platform::visionOS:
			sdk = "xros";
			break;
	}

	Vector<String> parts{ "echo", R"("")", "|", "/usr/bin/xcrun", "-sdk", sdk, "metal", "-E", "-dM", "-x", "metal" };

	switch (device_profile->platform) {
		case MetalDeviceProfile::Platform::macOS: {
			parts.push_back("-mtargetos=macos" + device_profile->min_os_version.to_compiler_os_version());
			break;
		}
		case MetalDeviceProfile::Platform::iOS: {
			parts.push_back("-mtargetos=ios" + device_profile->min_os_version.to_compiler_os_version());
			break;
		}
		case MetalDeviceProfile::Platform::visionOS: {
			parts.push_back("-mtargetos=xros" + device_profile->min_os_version.to_compiler_os_version());
			break;
		}
	}

	parts.append_array({ "-", "|", "grep", "-E", R"(\"__METAL_VERSION__|__ENVIRONMENT_OS\")" });

	List<String> args = { "-c", String(" ").join(parts) };

	String r_pipe;
	int exit_code;
	Error err = OS::execute("sh", args, &r_pipe, &exit_code, true);
	ERR_FAIL_COND_MSG(err != OK, "Failed to determine Metal toolchain properties");

	// Parse the lines, which are in the form:
	//
	// #define VARNAME VALUE
	Vector<String> lines = r_pipe.split("\n", false);
	for (String &line : lines) {
		Vector<String> name_val = line.trim_prefix("#define ").split(" ");
		if (name_val.size() != 2) {
			continue;
		}
		if (name_val[0] == "__ENVIRONMENT_OS_VERSION_MIN_REQUIRED__") {
			compiler_props.os_version_min_required = MinOsVersion((uint32_t)name_val[1].to_int());
		} else if (name_val[0] == "__METAL_VERSION__") {
			uint32_t ver = (uint32_t)name_val[1].to_int();
			uint32_t maj = ver / 100;
			uint32_t min = (ver % 100) / 10;
			compiler_props.metal_version = make_msl_version(maj, min);
		}

		if (compiler_props.is_valid()) {
			break;
		}
	}
}

Error RenderingShaderContainerMetal::compile_metal_source(const char *p_source, const StageData &p_stage_data, Vector<uint8_t> &r_binary_data) {
	String name(shader_name.ptr());
	if (name.contains_char(':')) {
		name = name.replace_char(':', '_');
	}
	Error r_error;
	Ref<FileAccess> source_file = FileAccess::create_temp(FileAccess::ModeFlags::READ_WRITE,
			name + "_" + itos(p_stage_data.hash.short_sha()),
			"metal", false, &r_error);
	ERR_FAIL_COND_V_MSG(r_error != OK, r_error, "Unable to create temporary source file.");
	if (!source_file->store_buffer((const uint8_t *)p_source, strlen(p_source))) {
		ERR_FAIL_V_MSG(ERR_CANT_CREATE, "Unable to write temporary source file");
	}
	source_file->flush();
	Ref<FileAccess> result_file = FileAccess::create_temp(FileAccess::ModeFlags::READ_WRITE,
			name + "_" + itos(p_stage_data.hash.short_sha()),
			"metallib", false, &r_error);

	ERR_FAIL_COND_V_MSG(r_error != OK, r_error, "Unable to create temporary target file");

	String sdk;
	switch (device_profile->platform) {
		case MetalDeviceProfile::Platform::macOS:
			sdk = "macosx";
			break;
		case MetalDeviceProfile::Platform::iOS:
			sdk = "iphoneos";
			break;
		case MetalDeviceProfile::Platform::visionOS:
			sdk = "xros";
			break;
	}

	// Build the .metallib binary.
	{
		List<String> args{
			"-sdk",
			sdk,
			"metal",
			"-O3",
			"-Wno-unused-variable",
			"-Wno-uninitialized",
			"-Wno-unused-function",
		};

		// Compile metal shaders for the minimum supported target instead of the host machine.
		switch (device_profile->platform) {
			case MetalDeviceProfile::Platform::macOS: {
				args.push_back("-mtargetos=macos" + device_profile->min_os_version.to_compiler_os_version());
				break;
			}
			case MetalDeviceProfile::Platform::iOS: {
				args.push_back("-mtargetos=ios" + device_profile->min_os_version.to_compiler_os_version());
				break;
			}
			case MetalDeviceProfile::Platform::visionOS: {
				args.push_back("-mtargetos=xros" + device_profile->min_os_version.to_compiler_os_version());
				break;
			}
		}

		if (p_stage_data.is_position_invariant) {
			args.push_back("-fpreserve-invariance");
		}
		args.push_back("-fmetal-math-mode=fast");
		args.push_back(source_file->get_path_absolute());
		args.push_back("-o");
		args.push_back(result_file->get_path_absolute());
		String r_pipe;
		int exit_code;
		Error err = OS::execute("/usr/bin/xcrun", args, &r_pipe, &exit_code, true);
		if (!r_pipe.is_empty()) {
			print_line(r_pipe);
		}
		if (err != OK) {
			ERR_PRINT(vformat("Metal compiler returned error code: %d", err));
		}

		if (exit_code != 0) {
			ERR_PRINT(vformat("Metal compiler exited with error code: %d", exit_code));
		}
		int len = result_file->get_length();
		ERR_FAIL_COND_V_MSG(len == 0, ERR_CANT_CREATE, "Metal compiler created empty library");
	}

	// Strip the source from the binary. AIR versions before 2.4 (MSL 2.4) don't support
	// companion MetalLib, so metal-dsymutil copies verbatim and emits a warning:
	//   "architecture air64_v23 does not support a companion MetalLib; copying verbatim"
	if (device_profile->features.msl_version >= MSL_VERSION_24) {
		List<String> args{ "-sdk", sdk, "metal-dsymutil", "--remove-source", result_file->get_path_absolute() };
		String r_pipe;
		int exit_code;
		Error err = OS::execute("/usr/bin/xcrun", args, &r_pipe, &exit_code, true);
		if (!r_pipe.is_empty()) {
			print_line(r_pipe);
		}
		if (err != OK) {
			ERR_PRINT(vformat("metal-dsymutil tool returned error code: %d", err));
		}

		if (exit_code != 0) {
			ERR_PRINT(vformat("metal-dsymutil Compiler exited with error code: %d", exit_code));
		}
		int len = result_file->get_length();
		ERR_FAIL_COND_V_MSG(len == 0, ERR_CANT_CREATE, "metal-dsymutil tool created empty library");
	}

	r_binary_data = result_file->get_buffer(result_file->get_length());

	return OK;
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunguarded-availability"

static spv::ExecutionModel SHADER_STAGE_REMAP[RDD::SHADER_STAGE_MAX] = {
	spv::ExecutionModelVertex, // RDD::SHADER_STAGE_VERTEX
	spv::ExecutionModelFragment, // RDD::SHADER_STAGE_FRAGMENT
	spv::ExecutionModelTessellationControl, // RDD::SHADER_STAGE_TESSELATION_CONTROL
	spv::ExecutionModelTessellationEvaluation, // RDD::SHADER_STAGE_TESSELATION_EVALUATION
	spv::ExecutionModelGLCompute, // RDD::SHADER_STAGE_COMPUTE
};

spv::ExecutionModel get_stage(uint32_t p_stages_mask, RDD::ShaderStage p_stage) {
	if (p_stages_mask & (1 << p_stage)) {
		return SHADER_STAGE_REMAP[p_stage];
	}
	return spv::ExecutionModel::ExecutionModelMax;
}

spv::ExecutionModel map_stage(RDD::ShaderStage p_stage) {
	return SHADER_STAGE_REMAP[p_stage];
}

MetalDeviceProfile::MinimumRequirements RenderingShaderContainerMetal::inspect_spirv(const ReflectShader &p_shader) {
	// Scan SPIR-V for OpImageTexelPointer to detect image atomic usage and determine
	// the minimum GPU family and MSL version required.
	// 32-bit atomics (R32ui/R32i) require Apple6+ and MSL 3.1+.
	// 64-bit atomics (Rg32ui/Rg32i) require Apple8+ and MSL 3.1+.

	MetalDeviceProfile::MinimumRequirements reqs;

	for (const ReflectShaderStage &stage : p_shader.shader_stages) {
		Span<uint32_t> spirv = stage.spirv();

		// SPIR-V header is 5 words (magic, version, generator, bound, schema).
		HashSet<uint32_t> atomic_image_ids;
		const uint32_t *words = spirv.ptr() + 5;
		while (words < spirv.end()) {
			const uint32_t instruction = *words;
			const uint16_t word_count = instruction >> 16;
			spv::Op opcode = static_cast<spv::Op>(instruction & 0xFFFF);
			if (opcode == spv::OpImageTexelPointer) {
				atomic_image_ids.insert(words[3]);
			}
			words += word_count;
		}

		if (atomic_image_ids.is_empty()) {
			continue;
		}

		// Look up image formats via spv-reflect bindings.
		const SpvReflectShaderModule &module = stage.module();
		uint32_t binding_count = 0;
		spvReflectEnumerateDescriptorBindings(&module, &binding_count, nullptr);

		LocalVector<SpvReflectDescriptorBinding *> bindings_heap;
		constexpr uint32_t MAX_STACK_BINDINGS = 256;

		SpvReflectDescriptorBinding **bindings;
		if (binding_count > MAX_STACK_BINDINGS) {
			bindings_heap.resize(binding_count);
			bindings = bindings_heap.ptr();
		} else {
			bindings = ALLOCA_ARRAY(SpvReflectDescriptorBinding *, binding_count);
		}
		spvReflectEnumerateDescriptorBindings(&module, &binding_count, bindings);

		for (uint32_t i = 0; i < binding_count; i++) {
			const SpvReflectDescriptorBinding &binding = *bindings[i];
			if (!atomic_image_ids.has(binding.spirv_id)) {
				continue;
			}

			switch (binding.image.image_format) {
				case SpvImageFormatR32ui:
				case SpvImageFormatR32i:
					if (reqs.gpu < MetalDeviceProfile::GPU::Apple6) {
						reqs.gpu = MetalDeviceProfile::GPU::Apple6;
					}
					reqs.msl_version = MAX(reqs.msl_version, MSL_VERSION_31);
					break;
				case SpvImageFormatRg32ui:
				case SpvImageFormatRg32i:
					if (reqs.gpu < MetalDeviceProfile::GPU::Apple8) {
						reqs.gpu = MetalDeviceProfile::GPU::Apple8;
					}
					reqs.msl_version = MAX(reqs.msl_version, MSL_VERSION_31);
					break;
				default:
					break;
			}
		}
	}

	return reqs;
}

bool RenderingShaderContainerMetal::_set_code_from_spirv(const ReflectShader &p_shader) {
	using namespace spirv_cross;
	using spirv_cross::CompilerMSL;
	using spirv_cross::Resource;

	const LocalVector<ReflectShaderStage> &p_spirv = p_shader.shader_stages;

	if (export_mode) {
		_initialize_toolchain_properties();

		// When baking shaders for export, check if the SPIR-V requires capabilities
		// that the target profile can't support natively. SPIRV-Cross would emulate
		// image atomics with auxiliary buffer bindings incompatible with Godot's binding
		// layout. Return an empty baked shader so the runtime recompiles for the actual device.
		MetalDeviceProfile::MinimumRequirements reqs = inspect_spirv(p_shader);
		MetalDeviceProfile::MinimumRequirements target = device_profile->get_minimum_requirements();
		if (reqs > target) {
			uint32_t req_maj, req_min, tgt_maj, tgt_min;
			parse_msl_version(reqs.msl_version, req_maj, req_min);
			parse_msl_version(target.msl_version, tgt_maj, tgt_min);
			WARN_PRINT(vformat("Shader '%s' requires Apple%d / MSL %d.%d but target is Apple%d / MSL %d.%d. Shader will be compiled at runtime on the device.",
					String(shader_name.ptr()),
					static_cast<uint32_t>(reqs.gpu) - 1000, req_maj, req_min,
					static_cast<uint32_t>(target.gpu) - 1000, tgt_maj, tgt_min));
			mtl_reflection_data.mark_invalid();
			return true;
		}
	}

	// initialize Metal-specific reflection data
	shaders.resize(p_spirv.size());
	mtl_shaders.resize(p_spirv.size());
	mtl_reflection_binding_set_uniforms_data.resize(reflection_binding_set_uniforms_data.size());

	mtl_reflection_data.set_needs_view_mask_buffer(reflection_data.has_multiview);
	mtl_reflection_data.profile = *device_profile;

	CompilerMSL::Options msl_options{};

	// Determine Metal language version.
	uint32_t msl_version = 0;
	{
		if (export_mode && compiler_props.is_valid()) {
			// Use the properties determined by the toolchain and minimum OS version.
			msl_version = compiler_props.metal_version;
			mtl_reflection_data.os_min_version = compiler_props.os_version_min_required;
		} else {
			msl_version = device_profile->features.msl_version;
			mtl_reflection_data.os_min_version = MinOsVersion();
		}
		uint32_t msl_ver_maj = 0;
		uint32_t msl_ver_min = 0;
		parse_msl_version(msl_version, msl_ver_maj, msl_ver_min);
		msl_options.set_msl_version(msl_ver_maj, msl_ver_min);
		mtl_reflection_data.msl_version = msl_version;
	}

	msl_options.platform = device_profile->platform == MetalDeviceProfile::Platform::macOS ? CompilerMSL::Options::macOS : CompilerMSL::Options::iOS;

	if (device_profile->platform == MetalDeviceProfile::Platform::iOS) {
		msl_options.ios_use_simdgroup_functions = device_profile->features.simdPermute;
		msl_options.ios_support_base_vertex_instance = true;
	}

	if (device_profile->features.use_argument_buffers) {
		msl_options.argument_buffers_tier = CompilerMSL::Options::ArgumentBuffersTier::Tier2;
		msl_options.argument_buffers = true;
		mtl_reflection_data.set_uses_argument_buffers(true);
	} else {
		msl_options.argument_buffers_tier = CompilerMSL::Options::ArgumentBuffersTier::Tier1;
		// Tier 1 argument buffers don't support writable textures, so we disable them completely.
		msl_options.argument_buffers = false;
		mtl_reflection_data.set_uses_argument_buffers(false);
	}
	msl_options.force_active_argument_buffer_resources = true;
	msl_options.pad_argument_buffer_resources = true;
	msl_options.texture_buffer_native = true; // Enable texture buffer support.
	msl_options.use_framebuffer_fetch_subpasses = false;
	msl_options.pad_fragment_output_components = true;
	msl_options.r32ui_alignment_constant_id = R32UI_ALIGNMENT_CONSTANT_ID;
	msl_options.agx_manual_cube_grad_fixup = true;
	if (reflection_data.has_multiview) {
		msl_options.multiview = true;
		msl_options.multiview_layered_rendering = true;
		msl_options.view_mask_buffer_index = VIEW_MASK_BUFFER_INDEX;
	}
	if (msl_version >= MSL_VERSION_32) {
		// All 3.2+ versions support device coherence, so we can disable texture fences.
		msl_options.readwrite_texture_fences = false;
	}

	CompilerGLSL::Options options{};
	options.vertex.flip_vert_y = true;
#if DEV_ENABLED
	options.emit_line_directives = true;
#endif

	// Assign MSL bindings for all the descriptor sets.
	typedef std::pair<MSLResourceBinding, uint32_t> MSLBindingInfo;
	LocalVector<MSLBindingInfo> spirv_bindings;
	MSLResourceBinding push_constant_resource_binding;
	{
		enum IndexType {
			Texture,
			Buffer,
			Sampler,
			Max,
		};

		uint32_t dset_count = p_shader.uniform_sets.size();
		uint32_t size = reflection_binding_set_uniforms_data.size();
		spirv_bindings.resize(size);

		uint32_t indices[IndexType::Max] = { 0 };
		auto next_index = [&indices](IndexType p_t, uint32_t p_stride) -> uint32_t {
			uint32_t v = indices[p_t];
			indices[p_t] += p_stride;
			return v;
		};

		uint32_t idx_dset = 0;
		MSLBindingInfo *iter = spirv_bindings.ptr();
		UniformData *found = mtl_reflection_binding_set_uniforms_data.ptrw();
		UniformData::IndexType shader_index_type = msl_options.argument_buffers ? UniformData::IndexType::ARG : UniformData::IndexType::SLOT;

		for (const ReflectDescriptorSet &dset : p_shader.uniform_sets) {
			// Reset the index count for each descriptor set, as this is an index in to the argument table.
			uint32_t next_arg_buffer_index = 0;
			auto next_arg_index = [&next_arg_buffer_index](uint32_t p_stride) -> uint32_t {
				uint32_t v = next_arg_buffer_index;
				next_arg_buffer_index += p_stride;
				return v;
			};

			for (const ReflectUniform &uniform : dset) {
				const SpvReflectDescriptorBinding &binding = uniform.get_spv_reflect();

				found->active_stages = uniform.stages;

				RDC::UniformType type = RDC::UniformType(uniform.type);
				uint32_t binding_stride = 1; // If this is an array, stride will be the length of the array.
				if (uniform.length > 1) {
					switch (type) {
						case RDC::UNIFORM_TYPE_UNIFORM_BUFFER_DYNAMIC:
						case RDC::UNIFORM_TYPE_STORAGE_BUFFER_DYNAMIC:
						case RDC::UNIFORM_TYPE_UNIFORM_BUFFER:
						case RDC::UNIFORM_TYPE_STORAGE_BUFFER:
							// Buffers's length is its size, in bytes, so there is no stride.
							break;
						default: {
							binding_stride = uniform.length;
							found->array_length = uniform.length;
						} break;
					}
				}

				// Determine access type.
				switch (binding.descriptor_type) {
					case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE: {
						if (!(binding.decoration_flags & SPV_REFLECT_DECORATION_NON_WRITABLE)) {
							if (!(binding.decoration_flags & SPV_REFLECT_DECORATION_NON_READABLE)) {
								found->access = MTL::BindingAccessReadWrite;
							} else {
								found->access = MTL::BindingAccessWriteOnly;
							}
						}
					} break;
					case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
					case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER: {
						if (!(binding.decoration_flags & SPV_REFLECT_DECORATION_NON_WRITABLE) && !(binding.block.decoration_flags & SPV_REFLECT_DECORATION_NON_WRITABLE)) {
							if (!(binding.decoration_flags & SPV_REFLECT_DECORATION_NON_READABLE) && !(binding.block.decoration_flags & SPV_REFLECT_DECORATION_NON_READABLE)) {
								found->access = MTL::BindingAccessReadWrite;
							} else {
								found->access = MTL::BindingAccessWriteOnly;
							}
						}
					} break;
					default:
						break;
				}

				switch (found->access) {
					case MTL::BindingAccessReadOnly:
						found->usage = MTL::ResourceUsageRead;
						break;
					case MTL::BindingAccessWriteOnly:
						found->usage = MTL::ResourceUsageWrite;
						break;
					case MTL::BindingAccessReadWrite:
						found->usage = MTL::ResourceUsageRead | MTL::ResourceUsageWrite;
						break;
				}

				iter->second = uniform.stages;
				MSLResourceBinding &rb = iter->first;
				rb.desc_set = idx_dset;
				rb.binding = uniform.binding;
				rb.count = binding_stride;

				switch (type) {
					case RDC::UNIFORM_TYPE_SAMPLER: {
						found->data_type = MTL::DataTypeSampler;
						found->get_indexes(UniformData::IndexType::SLOT).sampler = next_index(Sampler, binding_stride);
						found->get_indexes(UniformData::IndexType::ARG).sampler = next_arg_index(binding_stride);

						rb.basetype = SPIRType::BaseType::Sampler;

					} break;
					case RDC::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE:
					case RDC::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE_BUFFER: {
						found->data_type = MTL::DataTypeTexture;
						found->get_indexes(UniformData::IndexType::SLOT).texture = next_index(Texture, binding_stride);
						found->get_indexes(UniformData::IndexType::SLOT).sampler = next_index(Sampler, binding_stride);
						found->get_indexes(UniformData::IndexType::ARG).texture = next_arg_index(binding_stride);
						found->get_indexes(UniformData::IndexType::ARG).sampler = next_arg_index(binding_stride);
						rb.basetype = SPIRType::BaseType::SampledImage;
					} break;
					case RDC::UNIFORM_TYPE_TEXTURE:
					case RDC::UNIFORM_TYPE_IMAGE:
					case RDC::UNIFORM_TYPE_TEXTURE_BUFFER: {
						found->data_type = MTL::DataTypeTexture;
						found->get_indexes(UniformData::IndexType::SLOT).texture = next_index(Texture, binding_stride);
						found->get_indexes(UniformData::IndexType::ARG).texture = next_arg_index(binding_stride);
						rb.basetype = SPIRType::BaseType::Image;
					} break;
					case RDC::UNIFORM_TYPE_IMAGE_BUFFER:
						CRASH_NOW_MSG("Unimplemented!"); // TODO.
						break;
					case RDC::UNIFORM_TYPE_UNIFORM_BUFFER_DYNAMIC:
					case RDC::UNIFORM_TYPE_STORAGE_BUFFER_DYNAMIC:
					case RDC::UNIFORM_TYPE_UNIFORM_BUFFER:
					case RDC::UNIFORM_TYPE_STORAGE_BUFFER: {
						found->data_type = MTL::DataTypePointer;
						found->get_indexes(UniformData::IndexType::SLOT).buffer = next_index(Buffer, binding_stride);
						found->get_indexes(UniformData::IndexType::ARG).buffer = next_arg_index(binding_stride);
						rb.basetype = SPIRType::BaseType::Void;
					} break;
					case RDC::UNIFORM_TYPE_INPUT_ATTACHMENT: {
						found->data_type = MTL::DataTypeTexture;
						found->get_indexes(UniformData::IndexType::SLOT).texture = next_index(Texture, binding_stride);
						found->get_indexes(UniformData::IndexType::ARG).texture = next_arg_index(binding_stride);
						rb.basetype = SPIRType::BaseType::Image;
					} break;
					case RDC::UNIFORM_TYPE_MAX:
					default:
						CRASH_NOW_MSG("Unreachable");
				}

				// Specify the MSL resource bindings based on how the binding mode used by the shader.
				rb.msl_buffer = found->get_indexes(shader_index_type).buffer;
				rb.msl_texture = found->get_indexes(shader_index_type).texture;
				rb.msl_sampler = found->get_indexes(shader_index_type).sampler;

				if (found->data_type == MTL::DataTypeTexture) {
					const SpvReflectImageTraits &image = uniform.get_spv_reflect().image;

					switch (image.dim) {
						case SpvDim1D: {
							if (image.arrayed) {
								found->texture_type = MTL::TextureType1DArray;
							} else {
								found->texture_type = MTL::TextureType1D;
							}
						} break;
						case SpvDimSubpassData:
						case SpvDim2D: {
							if (image.arrayed && image.ms) {
								found->texture_type = MTL::TextureType2DMultisampleArray;
							} else if (image.arrayed) {
								found->texture_type = MTL::TextureType2DArray;
							} else if (image.ms) {
								found->texture_type = MTL::TextureType2DMultisample;
							} else {
								found->texture_type = MTL::TextureType2D;
							}
						} break;
						case SpvDim3D: {
							found->texture_type = MTL::TextureType3D;
						} break;
						case SpvDimCube: {
							if (image.arrayed) {
								found->texture_type = MTL::TextureTypeCubeArray;
							} else {
								found->texture_type = MTL::TextureTypeCube;
							}
						} break;
						case SpvDimRect: {
							// Ignored.
						} break;
						case SpvDimBuffer: {
							found->texture_type = MTL::TextureTypeTextureBuffer;
							// If this is used with atomics, we need to use a read-write texture.
							// 	scan_atomic_accesses();
							// 	if (atomic_spirv_ids.find(uniform.spirv_id) != atomic_spirv_ids.end()) {
							// 		rb.access = MTLBindingAccessReadWrite;
							// 		found->access = MTLBindingAccessReadWrite;
							// 	} else {
							// 		rb.access = MTLBindingAccessReadOnly;
							// 		found->access = MTLBindingAccessReadOnly;
							// 	}
						} break;
						case SpvDimTileImageDataEXT: {
							// Godot does not use this extension.
							// See: https://registry.khronos.org/vulkan/specs/latest/man/html/VK_EXT_shader_tile_image.html
						} break;
						case SpvDimMax: {
							// Add all enumerations to silence the compiler warning
							// and generate future warnings, should a new one be added.
						} break;
					}
				}

				iter++;
				found++;
			}
			idx_dset++;
		}

		if (reflection_data.push_constant_size > 0) {
			push_constant_resource_binding.desc_set = ResourceBindingPushConstantDescriptorSet;
			push_constant_resource_binding.basetype = SPIRType::BaseType::Void;
			if (msl_options.argument_buffers) {
				push_constant_resource_binding.msl_buffer = dset_count;
			} else {
				push_constant_resource_binding.msl_buffer = next_index(Buffer, 1);
			}
			mtl_reflection_data.push_constant_binding = push_constant_resource_binding.msl_buffer;
		}
	}

	for (uint32_t i = 0; i < p_spirv.size(); i++) {
		StageData &stage_data = mtl_shaders.write[i];
		const ReflectShaderStage &v = p_spirv[i];
		RDC::ShaderStage stage = v.shader_stage;
		Span<uint32_t> spirv = v.spirv();
		Parser parser(spirv.ptr(), spirv.size());
		try {
			parser.parse();
		} catch (CompilerError &e) {
			ERR_FAIL_V_MSG(false, "Failed to parse IR at stage " + String(RDC::SHADER_STAGE_NAMES[stage]) + ": " + e.what());
		}

		CompilerMSL compiler(std::move(parser.get_parsed_ir()));
		compiler.set_msl_options(msl_options);
		compiler.set_common_options(options);

		spv::ExecutionModel execution_model = map_stage(stage);
		for (uint32_t jj = 0; jj < spirv_bindings.size(); jj++) {
			MSLResourceBinding &rb = spirv_bindings.ptr()[jj].first;
			rb.stage = execution_model;
			compiler.add_msl_resource_binding(rb);
		}

		if (push_constant_resource_binding.desc_set == ResourceBindingPushConstantDescriptorSet) {
			push_constant_resource_binding.stage = execution_model;
			compiler.add_msl_resource_binding(push_constant_resource_binding);
		}

		std::unordered_set<VariableID> active = compiler.get_active_interface_variables();
		ShaderResources resources = compiler.get_shader_resources();

		std::string source;
		try {
			source = compiler.compile();
		} catch (CompilerError &e) {
			ERR_FAIL_V_MSG(false, "Failed to compile stage " + String(RDC::SHADER_STAGE_NAMES[stage]) + ": " + e.what());
		}

		ERR_FAIL_COND_V_MSG(compiler.get_entry_points_and_stages().size() != 1, false, "Expected a single entry point and stage.");

		SmallVector<EntryPoint> entry_pts_stages = compiler.get_entry_points_and_stages();
		EntryPoint &entry_point_stage = entry_pts_stages.front();
		SPIREntryPoint &entry_point = compiler.get_entry_point(entry_point_stage.name, entry_point_stage.execution_model);

		for (auto ext : compiler.get_declared_extensions()) {
			if (ext == "SPV_KHR_non_semantic_info" || ext == "SPV_KHR_printf") {
				mtl_reflection_data.set_needs_debug_logging(true);
				break;
			}
		}

		if (!resources.stage_inputs.empty()) {
			for (const Resource &res : resources.stage_inputs) {
				uint32_t binding = compiler.get_automatic_msl_resource_binding(res.id);
				if (binding != (uint32_t)-1) {
					stage_data.vertex_input_binding_mask |= 1 << binding;
				}
			}
		}

		stage_data.is_position_invariant = compiler.is_position_invariant();
		stage_data.supports_fast_math = !entry_point.flags.get(spv::ExecutionModeSignedZeroInfNanPreserve);
		stage_data.hash = SHA256Digest(source.c_str(), source.length());
		stage_data.source_size = source.length();
		::Vector<uint8_t> binary_data;
		binary_data.resize(stage_data.source_size);
		memcpy(binary_data.ptrw(), source.c_str(), stage_data.source_size);

		if (export_mode) {
			if (compiler_props.is_valid()) {
				// Try to compile the Metal source code.
				::Vector<uint8_t> library_data;
				Error compile_err = compile_metal_source(source.c_str(), stage_data, library_data);
				if (compile_err == OK) {
					// If we successfully compiled to a `.metallib`, there are greater restrictions on target platforms,
					// so we must update the properties.
					stage_data.library_size = library_data.size();
					binary_data.resize(stage_data.source_size + stage_data.library_size);
					memcpy(binary_data.ptrw() + stage_data.source_size, library_data.ptr(), stage_data.library_size);
				}
			} else {
				WARN_PRINT_ONCE("Metal shader baking limited to SPIR-V: Unable to determine toolchain properties to compile .metallib");
			}
		}

		uint32_t binary_data_size = binary_data.size();
		Shader &shader = shaders.write[i];
		shader.shader_stage = stage;
		shader.code_decompressed_size = binary_data_size;
		shader.code_compressed_bytes.resize(binary_data_size);

		uint32_t compressed_size = 0;
		bool compressed = compress_code(binary_data.ptr(), binary_data_size, shader.code_compressed_bytes.ptrw(), &compressed_size, &shader.code_compression_flags);
		ERR_FAIL_COND_V_MSG(!compressed, false, vformat("Failed to compress native code to native for SPIR-V #%d.", i));

		shader.code_compressed_bytes.resize(compressed_size);
	}

	return true;
}

#pragma clang diagnostic pop

uint32_t RenderingShaderContainerMetal::_to_bytes_reflection_extra_data(uint8_t *p_bytes) const {
	if (p_bytes != nullptr) {
		*(HeaderData *)p_bytes = mtl_reflection_data;
	}
	return sizeof(HeaderData);
}

uint32_t RenderingShaderContainerMetal::_to_bytes_reflection_binding_uniform_extra_data(uint8_t *p_bytes, uint32_t p_index) const {
	if (is_invalid()) {
		return 0;
	}

	if (p_bytes != nullptr) {
		*(UniformData *)p_bytes = mtl_reflection_binding_set_uniforms_data[p_index];
	}
	return sizeof(UniformData);
}

uint32_t RenderingShaderContainerMetal::_to_bytes_shader_extra_data(uint8_t *p_bytes, uint32_t p_index) const {
	if (is_invalid()) {
		return 0;
	}

	if (p_bytes != nullptr) {
		*(StageData *)p_bytes = mtl_shaders[p_index];
	}
	return sizeof(StageData);
}

uint32_t RenderingShaderContainerMetal::_from_bytes_reflection_extra_data(const uint8_t *p_bytes) {
	mtl_reflection_data = *(HeaderData *)p_bytes;
	return sizeof(HeaderData);
}

uint32_t RenderingShaderContainerMetal::_from_bytes_reflection_binding_uniform_extra_data_start(const uint8_t *p_bytes) {
	mtl_reflection_binding_set_uniforms_data.resize(reflection_binding_set_uniforms_data.size());
	return 0;
}

uint32_t RenderingShaderContainerMetal::_from_bytes_reflection_binding_uniform_extra_data(const uint8_t *p_bytes, uint32_t p_index) {
	mtl_reflection_binding_set_uniforms_data.ptrw()[p_index] = *(UniformData *)p_bytes;
	return sizeof(UniformData);
}

uint32_t RenderingShaderContainerMetal::_from_bytes_shader_extra_data_start(const uint8_t *p_bytes) {
	mtl_shaders.resize(shaders.size());
	return 0;
}

uint32_t RenderingShaderContainerMetal::_from_bytes_shader_extra_data(const uint8_t *p_bytes, uint32_t p_index) {
	mtl_shaders.ptrw()[p_index] = *(StageData *)p_bytes;
	return sizeof(StageData);
}

RenderingShaderContainerMetal::MetalShaderReflection RenderingShaderContainerMetal::get_metal_shader_reflection() const {
	MetalShaderReflection res;

	uint32_t uniform_set_count = reflection_binding_set_uniforms_count.size();
	uint32_t start = 0;
	res.uniform_sets.resize(uniform_set_count);
	for (uint32_t i = 0; i < uniform_set_count; i++) {
		Vector<UniformData> &set = res.uniform_sets.ptrw()[i];
		uint32_t count = reflection_binding_set_uniforms_count.get(i);
		set.resize(count);
		memcpy(set.ptrw(), &mtl_reflection_binding_set_uniforms_data.ptr()[start], count * sizeof(UniformData));
		start += count;
	}

	return res;
}

uint32_t RenderingShaderContainerMetal::_format() const {
	return 0x42424242;
}

uint32_t RenderingShaderContainerMetal::_format_version() const {
	return FORMAT_VERSION;
}

Ref<RenderingShaderContainer> RenderingShaderContainerFormatMetal::create_container() const {
	Ref<RenderingShaderContainerMetal> result;
	result.instantiate();
	result->set_export_mode(export_mode);
	result->set_device_profile(device_profile);
	return result;
}

RenderingDeviceCommons::ShaderLanguageVersion RenderingShaderContainerFormatMetal::get_shader_language_version() const {
	return SHADER_LANGUAGE_VULKAN_VERSION_1_1;
}

RenderingDeviceCommons::ShaderSpirvVersion RenderingShaderContainerFormatMetal::get_shader_spirv_version() const {
	return SHADER_SPIRV_VERSION_1_6;
}

RenderingShaderContainerFormatMetal::RenderingShaderContainerFormatMetal(const MetalDeviceProfile *p_device_profile, bool p_export) :
		export_mode(p_export), device_profile(p_device_profile) {
}

String MinOsVersion::to_compiler_os_version() const {
	if (version == UINT32_MAX) {
		return "";
	}

	uint32_t major = version / 10000;
	uint32_t minor = (version % 10000) / 100;
	return vformat("%d.%d", major, minor);
}

MinOsVersion::MinOsVersion(const String &p_version) {
	int pos = p_version.find_char('.');
	if (pos > 0) {
		version = (uint32_t)(p_version.substr(0, pos).to_int() * 10000 +
				p_version.substr(pos + 1).to_int() * 100);
	} else {
		version = (uint32_t)(p_version.to_int() * 10000);
	}

	if (version == 0) {
		version = UINT32_MAX;
	}
}
