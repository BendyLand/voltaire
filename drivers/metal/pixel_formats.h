/**************************************************************************/
/*  pixel_formats.h                                                       */
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

#include "core/typedefs.h"

GODOT_CLANG_WARNING_PUSH_AND_IGNORE("-Wdeprecated-declarations")
// --- Folded from drivers/metal/inflection_map.h ---
#include "core/templates/hash_map.h"
#include "core/templates/local_vector.h"

#include <iterator>

/// An unordered map that splits elements between a fast-access vector of LinearCount consecutively
/// indexed elements, and a slower-access map holding sparse indexes larger than LinearCount.
///
/// \tparam KeyType is used to lookup values, and must be a type that is convertible to an unsigned integer.
/// \tparam ValueType must have an empty constructor (default or otherwise).
/// \tparam LinearCount
/// \tparam IndexType must be a type that is convertible to an unsigned integer (eg. uint8_t...uint64_t), and which is large enough to represent the number of values in this map.
template <typename KeyType, typename ValueType, size_t LinearCount, typename IndexType = uint16_t>
class InflectionMap {
public:
	using value_type = ValueType;
	class Iterator {
		InflectionMap *map;
		IndexType index;

	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type = ValueType;
		using pointer = value_type *;
		using reference = value_type &;

		Iterator() :
				map(nullptr), index(0) {}
		Iterator(InflectionMap &p_m, const IndexType p_i) :
				map(&p_m), index(p_i) {}

		Iterator &operator=(const Iterator &p_it) {
			map = p_it.map;
			index = p_it.index;
			return *this;
		}

		ValueType *operator->() { return &map->_values[index]; }
		ValueType &operator*() { return map->_values[index]; }
		operator ValueType *() { return &map->_values[index]; }

		bool operator==(const Iterator &p_it) const { return map == p_it.map && index == p_it.index; }
		bool operator!=(const Iterator &p_it) const { return map != p_it.map || index != p_it.index; }

		Iterator &operator++() {
			index++;
			return *this;
		}
		Iterator operator++(int) {
			Iterator t = *this;
			index++;
			return t;
		}

		bool is_valid() const { return index < map->_values.size(); }
	};

	const ValueType &operator[](const KeyType p_idx) const { return get_value(p_idx); }
	ValueType &operator[](const KeyType p_idx) { return get_value(p_idx); }

	Iterator begin() { return Iterator(*this, 0); }
	Iterator end() { return Iterator(*this, _values.size()); }

	bool is_empty() { return _values.is_empty(); }
	size_t size() { return _values.size(); }
	void reserve(size_t p_new_cap) { _values.reserve(p_new_cap); }

protected:
	static constexpr IndexType INVALID = std::numeric_limits<IndexType>::max();
	typedef struct IndexValue {
		IndexType value = INVALID;
	} IndexValue;

	// Returns a reference to the value at the index.
	// If the index has not been initialized, add an empty element at
	// the end of the values array, and set the index to its position.
	ValueType &get_value(KeyType p_idx) {
		IndexValue *val_idx = p_idx < LinearCount ? &_linear_indexes[p_idx] : _inflection_indexes.getptr(p_idx);
		if (val_idx == nullptr || val_idx->value == INVALID) {
			_values.push_back({});
			if (val_idx == nullptr) {
				val_idx = &_inflection_indexes.insert(p_idx, {})->value;
			}
			val_idx->value = _values.size() - 1;
		}
		return _values[val_idx->value];
	}

	TightLocalVector<ValueType> _values;
	HashMap<KeyType, IndexValue> _inflection_indexes;
	IndexValue _linear_indexes[LinearCount];
};

// --- End drivers/metal/inflection_map.h ---
// --- Folded from drivers/metal/metal_device_properties.h ---
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

#include "servers/rendering/rendering_device_driver.h"

#include <Metal/Metal.hpp>

#include <cstddef>

/** The buffer index to use for vertex content. */
const static uint32_t VERT_CONTENT_BUFFER_INDEX = 0;
const static uint32_t MAX_COLOR_ATTACHMENT_COUNT = 8;

enum SampleCount : NS::UInteger {
	SampleCount1 = (1UL << 0),
	SampleCount2 = (1UL << 1),
	SampleCount4 = (1UL << 2),
	SampleCount8 = (1UL << 3),
	SampleCount16 = (1UL << 4),
	SampleCount32 = (1UL << 5),
	SampleCount64 = (1UL << 6),
};

_FORCE_INLINE_ SampleCount operator|(SampleCount a, SampleCount b) {
	return static_cast<SampleCount>(static_cast<NS::UInteger>(a) | static_cast<NS::UInteger>(b));
}

_FORCE_INLINE_ SampleCount &operator|=(SampleCount &a, SampleCount b) {
	return a = a | b;
}

_FORCE_INLINE_ SampleCount operator<<(SampleCount a, int shift) {
	return static_cast<SampleCount>(static_cast<NS::UInteger>(a) << shift);
}

_FORCE_INLINE_ SampleCount &operator<<=(SampleCount &a, int shift) {
	return a = a << shift;
}

struct API_AVAILABLE(macos(11.0), ios(14.0), tvos(14.0)) MetalFeatures {
	/// Maximum version of the Metal Shading Language version available.
	uint32_t msl_max_version = 0;
	/*! @brief Target version of the Metal Shading Language used to translate shaders.
	 *
	 * This can be used to override the features used to generate shaders. Primarily
	 * for engine developers for testing.
	 */
	uint32_t msl_target_version = 0;
	MTL::GPUFamily highestFamily = MTL::GPUFamilyApple4;
	bool supportsBCTextureCompression = false;
	bool supportsDepth24Stencil8 = false;
	bool supports32BitFloatFiltering = false;
	bool supports32BitMSAA = false;
	bool supportsMac = TARGET_OS_OSX;
	SampleCount supportedSampleCounts = SampleCount1;
	long hostMemoryPageSize = 0;
	bool layeredRendering = false;
	bool multisampleLayeredRendering = false;
	bool quadPermute = false; /**< If true, quadgroup permutation functions (vote, ballot, shuffle) are supported in shaders. */
	bool simdPermute = false; /**< If true, SIMD-group permutation functions (vote, ballot, shuffle) are supported in shaders. */
	bool simdReduction = false; /**< If true, SIMD-group reduction functions (arithmetic) are supported in shaders. */
	bool tessellationShader = false; /**< If true, tessellation shaders are supported. */
	bool imageCubeArray = false; /**< If true, image cube arrays are supported. */
	MTL::ArgumentBuffersTier argument_buffers_tier = MTL::ArgumentBuffersTier1;
	bool needs_arg_encoders = true; /**< If true, argument encoders are required to encode arguments into an argument buffer. */
	bool use_argument_buffers = true; /**< If true, argument buffers are can be used instead of slot binding, if available. */
	bool metal_fx_spatial = false; /**< If true, Metal FX spatial functions are supported. */
	bool metal_fx_temporal = false; /**< If true, Metal FX temporal functions are supported. */
	bool supports_gpu_address = false; /**< If true, referencing a GPU address in a shader is supported. */
	bool supports_image_atomic_32_bit = false; /**< If true, 32-bit atomic operations on images are supported by the GPU. */
	bool supports_image_atomic_64_bit = false; /**< If true, 64-bit atomic operations on images are supported by the GPU. */
	bool supports_native_image_atomics = false; /**< If true, native image atomic operations are supported by the OS. */
	bool supports_border_color = false; /**< If true, sampler border color (clamp-to-border) is supported. Requires Apple7+. */
	bool supports_residency_sets = false; /**< If true, residency sets (MTLResidencySet) are supported by the OS. */
	bool supports_msaa_depth_resolve = false;

	/*!
	 * Check if argument buffers are fully supported, which requires tier 2 support and no need for argument encoders.
	 */
	_FORCE_INLINE_ bool argument_buffers_supported() const {
		return argument_buffers_tier == MTL::ArgumentBuffersTier2 && needs_arg_encoders == false;
	}

	/*!
	 * Check if argument buffers can be used, which requires that they are supported and that the user has enabled their use.
	 */
	_FORCE_INLINE_ bool argument_buffers_enabled() const {
		return use_argument_buffers && argument_buffers_supported();
	}
};

struct MetalLimits {
	uint64_t maxImageArrayLayers;
	uint64_t maxFramebufferHeight;
	uint64_t maxFramebufferWidth;
	uint64_t maxImageDimension1D;
	uint64_t maxImageDimension2D;
	uint64_t maxImageDimension3D;
	uint64_t maxImageDimensionCube;
	uint64_t maxViewportDimensionX;
	uint64_t maxViewportDimensionY;
	MTL::Size maxThreadsPerThreadGroup;
	MTL::Size maxComputeWorkGroupCount;
	uint64_t maxBoundDescriptorSets;
	uint64_t maxColorAttachments;
	uint64_t maxTexturesPerArgumentBuffer;
	uint64_t maxSamplersPerArgumentBuffer;
	uint64_t maxBuffersPerArgumentBuffer;
	uint64_t maxBufferLength;
	uint64_t minUniformBufferOffsetAlignment;
	uint64_t maxVertexDescriptorLayoutStride;
	uint16_t maxViewports;
	uint32_t maxPerStageBufferCount; /**< The total number of per-stage Metal buffers available for shader uniform content and attributes. */
	uint32_t maxPerStageTextureCount; /**< The total number of per-stage Metal textures available for shader uniform content. */
	uint32_t maxPerStageSamplerCount; /**< The total number of per-stage Metal samplers available for shader uniform content. */
	uint32_t maxVertexInputAttributes;
	uint32_t maxVertexInputBindings;
	uint32_t maxVertexInputBindingStride;
	uint32_t maxDrawIndexedIndexValue;
	uint32_t maxShaderVaryings;
	uint32_t maxThreadGroupMemoryAllocation;

	double temporalScalerInputContentMinScale;
	double temporalScalerInputContentMaxScale;

	uint32_t minSubgroupSize; /**< The minimum number of threads in a SIMD-group. */
	uint32_t maxSubgroupSize; /**< The maximum number of threads in a SIMD-group. */
	uint32_t subgroupSupportedShaderStages;
	uint32_t subgroupSupportedOperations; /**< The subgroup operations supported by the device. */
};

class API_AVAILABLE(macos(11.0), ios(14.0), tvos(14.0)) MetalDeviceProperties {
private:
	void init_features(MTL::Device *p_device);
	void init_limits(MTL::Device *p_device);
	void init_os_props();

public:
	MetalFeatures features;
	MetalLimits limits;

	// maj * 10000 + min * 100 + patch
	uint32_t os_version;

	SampleCount find_nearest_supported_sample_count(RDD::TextureSamples p_samples) const;

	MetalDeviceProperties(MTL::Device *p_device);
	~MetalDeviceProperties();

private:
	static const SampleCount sample_count[RDD::TextureSamples::TEXTURE_SAMPLES_MAX];
};

// --- End drivers/metal/metal_device_properties.h ---
#include "servers/rendering/rendering_device_commons.h"

#ifdef __OBJC__
#include <Metal/Metal.h>
#endif
#include <Metal/Metal.hpp>

#include <iterator>

#pragma mark -
#pragma mark Metal format capabilities

using RDC = RenderingDeviceCommons;

typedef enum : uint16_t {

	kMTLFmtCapsNone = 0,
	/*! The format can be used in a shader read operation. */
	kMTLFmtCapsRead = (1 << 0),
	/*! The format can be used in a shader filter operation during sampling. */
	kMTLFmtCapsFilter = (1 << 1),
	/*! The format can be used in a shader write operation. */
	kMTLFmtCapsWrite = (1 << 2),
	/*! The format can be used with atomic operations. */
	kMTLFmtCapsAtomic = (1 << 3),
	/*! The format can be used as a color attachment. */
	kMTLFmtCapsColorAtt = (1 << 4),
	/*! The format can be used as a depth-stencil attachment. */
	kMTLFmtCapsDSAtt = (1 << 5),
	/*! The format can be used with blend operations. */
	kMTLFmtCapsBlend = (1 << 6),
	/*! The format can be used as a destination for multisample antialias (MSAA) data. */
	kMTLFmtCapsMSAA = (1 << 7),
	/*! The format can be used as a resolve attachment. */
	kMTLFmtCapsResolve = (1 << 8),
	kMTLFmtCapsVertex = (1 << 9),

	kMTLFmtCapsRF = (kMTLFmtCapsRead | kMTLFmtCapsFilter),
	kMTLFmtCapsRC = (kMTLFmtCapsRead | kMTLFmtCapsColorAtt),
	kMTLFmtCapsRCB = (kMTLFmtCapsRC | kMTLFmtCapsBlend),
	kMTLFmtCapsRCM = (kMTLFmtCapsRC | kMTLFmtCapsMSAA),
	kMTLFmtCapsRCMB = (kMTLFmtCapsRCM | kMTLFmtCapsBlend),
	kMTLFmtCapsRWC = (kMTLFmtCapsRC | kMTLFmtCapsWrite),
	kMTLFmtCapsRWCB = (kMTLFmtCapsRWC | kMTLFmtCapsBlend),
	kMTLFmtCapsRWCM = (kMTLFmtCapsRWC | kMTLFmtCapsMSAA),
	kMTLFmtCapsRWCMB = (kMTLFmtCapsRWCM | kMTLFmtCapsBlend),
	kMTLFmtCapsRFCMRB = (kMTLFmtCapsRCMB | kMTLFmtCapsFilter | kMTLFmtCapsResolve),
	kMTLFmtCapsRFWCMB = (kMTLFmtCapsRWCMB | kMTLFmtCapsFilter),
	kMTLFmtCapsAll = (kMTLFmtCapsRFWCMB | kMTLFmtCapsResolve),

	kMTLFmtCapsDRM = (kMTLFmtCapsDSAtt | kMTLFmtCapsRead | kMTLFmtCapsMSAA),
	kMTLFmtCapsDRFM = (kMTLFmtCapsDRM | kMTLFmtCapsFilter),
	kMTLFmtCapsDRMR = (kMTLFmtCapsDRM | kMTLFmtCapsResolve),
	kMTLFmtCapsDRFMR = (kMTLFmtCapsDRFM | kMTLFmtCapsResolve),

	kMTLFmtCapsChromaSubsampling = kMTLFmtCapsRF,
	kMTLFmtCapsMultiPlanar = kMTLFmtCapsChromaSubsampling,
} MTLFmtCaps;

inline MTLFmtCaps operator|(MTLFmtCaps p_left, MTLFmtCaps p_right) {
	return static_cast<MTLFmtCaps>(static_cast<uint32_t>(p_left) | p_right);
}

inline MTLFmtCaps &operator|=(MTLFmtCaps &p_left, MTLFmtCaps p_right) {
	return (p_left = p_left | p_right);
}

#pragma mark -
#pragma mark Metal view classes

enum class MTLViewClass : uint8_t {
	None,
	Color8,
	Color16,
	Color32,
	Color64,
	Color128,
	PVRTC_RGB_2BPP,
	PVRTC_RGB_4BPP,
	PVRTC_RGBA_2BPP,
	PVRTC_RGBA_4BPP,
	EAC_R11,
	EAC_RG11,
	EAC_RGBA8,
	ETC2_RGB8,
	ETC2_RGB8A1,
	ASTC_4x4,
	ASTC_5x4,
	ASTC_5x5,
	ASTC_6x5,
	ASTC_6x6,
	ASTC_8x5,
	ASTC_8x6,
	ASTC_8x8,
	ASTC_10x5,
	ASTC_10x6,
	ASTC_10x8,
	ASTC_10x10,
	ASTC_12x10,
	ASTC_12x12,
	BC1_RGBA,
	BC2_RGBA,
	BC3_RGBA,
	BC4_R,
	BC5_RG,
	BC6H_RGB,
	BC7_RGBA,
	Depth24_Stencil8,
	Depth32_Stencil8,
	BGRA10_XR,
	BGR10_XR
};

#pragma mark -
#pragma mark Format descriptors

/** Enumerates the data type of a format. */
enum class MTLFormatType {
	None, /**< Format type is unknown. */
	ColorHalf, /**< A 16-bit floating point color. */
	ColorFloat, /**< A 32-bit floating point color. */
	ColorInt8, /**< A signed 8-bit integer color. */
	ColorUInt8, /**< An unsigned 8-bit integer color. */
	ColorInt16, /**< A signed 16-bit integer color. */
	ColorUInt16, /**< An unsigned 16-bit integer color. */
	ColorInt32, /**< A signed 32-bit integer color. */
	ColorUInt32, /**< An unsigned 32-bit integer color. */
	DepthStencil, /**< A depth and stencil value. */
	Compressed, /**< A block-compressed color. */
};

struct Extent2D {
	uint32_t width;
	uint32_t height;
};

struct ComponentMapping {
	RDC::TextureSwizzle r = RDC::TEXTURE_SWIZZLE_IDENTITY;
	RDC::TextureSwizzle g = RDC::TEXTURE_SWIZZLE_IDENTITY;
	RDC::TextureSwizzle b = RDC::TEXTURE_SWIZZLE_IDENTITY;
	RDC::TextureSwizzle a = RDC::TEXTURE_SWIZZLE_IDENTITY;
};

/** Describes the properties of a DataFormat, including the corresponding Metal pixel and vertex format. */
struct DataFormatDesc {
	RDC::DataFormat dataFormat;
	MTL::PixelFormat mtlPixelFormat;
	MTL::PixelFormat mtlPixelFormatSubstitute;
	MTL::VertexFormat mtlVertexFormat;
	MTL::VertexFormat mtlVertexFormatSubstitute;
	uint8_t chromaSubsamplingPlaneCount;
	uint8_t chromaSubsamplingComponentBits;
	Extent2D blockTexelSize;
	uint32_t bytesPerBlock;
	MTLFormatType formatType;
	ComponentMapping componentMapping;
	const char *name;
	bool hasReportedSubstitution;

	inline double bytesPerTexel() const { return (double)bytesPerBlock / (double)(blockTexelSize.width * blockTexelSize.height); }

	inline bool isSupported() const { return (mtlPixelFormat != MTL::PixelFormatInvalid || chromaSubsamplingPlaneCount > 1); }
	inline bool isSupportedOrSubstitutable() const { return isSupported() || (mtlPixelFormatSubstitute != MTL::PixelFormatInvalid); }

	inline bool vertexIsSupported() const { return (mtlVertexFormat != MTL::VertexFormatInvalid); }
	inline bool vertexIsSupportedOrSubstitutable() const { return vertexIsSupported() || (mtlVertexFormatSubstitute != MTL::VertexFormatInvalid); }

	bool needsSwizzle() const {
		return (componentMapping.r != RDC::TEXTURE_SWIZZLE_IDENTITY ||
				componentMapping.g != RDC::TEXTURE_SWIZZLE_IDENTITY ||
				componentMapping.b != RDC::TEXTURE_SWIZZLE_IDENTITY ||
				componentMapping.a != RDC::TEXTURE_SWIZZLE_IDENTITY);
	}
};

/** Describes the properties of a MTL::PixelFormat or MTL::VertexFormat. */
struct MTLFormatDesc {
	union {
		MTL::PixelFormat mtlPixelFormat;
		MTL::VertexFormat mtlVertexFormat;
	};
	RDC::DataFormat dataFormat = RDC::DATA_FORMAT_MAX;
	MTLFmtCaps mtlFmtCaps;
	MTLViewClass mtlViewClass;
	MTL::PixelFormat mtlPixelFormatLinear;
	const char *name = nullptr;

	inline bool isSupported() const { return (mtlPixelFormat != MTL::PixelFormatInvalid) && (mtlFmtCaps != kMTLFmtCapsNone); }
};

class API_AVAILABLE(macos(11.0), ios(14.0), tvos(14.0)) PixelFormats {
	using DataFormat = RDC::DataFormat;

public:
	/** Returns whether the DataFormat is supported by the GPU bound to this instance. */
	bool isSupported(DataFormat p_format);

	/** Returns whether the DataFormat is supported by this implementation, or can be substituted by one that is. */
	bool isSupportedOrSubstitutable(DataFormat p_format);

	/** Returns whether the specified Metal MTL::PixelFormat can be used as a depth format. */
	_FORCE_INLINE_ bool isDepthFormat(MTL::PixelFormat p_format) {
		switch (p_format) {
			case MTL::PixelFormatDepth32Float:
			case MTL::PixelFormatDepth16Unorm:
			case MTL::PixelFormatDepth32Float_Stencil8:
#if TARGET_OS_OSX
			case MTL::PixelFormatDepth24Unorm_Stencil8:
#endif
				return true;
			default:
				return false;
		}
	}

	/** Returns whether the specified Metal MTL::PixelFormat can be used as a stencil format. */
	_FORCE_INLINE_ bool isStencilFormat(MTL::PixelFormat p_format) {
		switch (p_format) {
			case MTL::PixelFormatStencil8:
#if TARGET_OS_OSX
			case MTL::PixelFormatDepth24Unorm_Stencil8:
			case MTL::PixelFormatX24_Stencil8:
#endif
			case MTL::PixelFormatDepth32Float_Stencil8:
			case MTL::PixelFormatX32_Stencil8:
				return true;
			default:
				return false;
		}
	}

	/** Returns whether the specified Metal MTL::PixelFormat is a PVRTC format. */
	bool isPVRTCFormat(MTL::PixelFormat p_format);

	/** Returns the format type corresponding to the specified Godot pixel format, */
	MTLFormatType getFormatType(DataFormat p_format);

	/** Returns the format type corresponding to the specified Metal MTL::PixelFormat, */
	MTLFormatType getFormatType(MTL::PixelFormat p_format);

	/**
	 * Returns the Metal MTL::PixelFormat corresponding to the specified Godot pixel
	 * or returns MTL::PixelFormatInvalid if no corresponding MTL::PixelFormat exists.
	 */
	MTL::PixelFormat getMTLPixelFormat(DataFormat p_format);

	/**
	 * Returns the DataFormat corresponding to the specified Metal MTL::PixelFormat,
	 * or returns DATA_FORMAT_MAX if no corresponding DataFormat exists.
	 */
	DataFormat getDataFormat(MTL::PixelFormat p_format);

	/**
	 * Returns the size, in bytes, of a texel block of the specified Godot pixel.
	 * For uncompressed formats, the returned value corresponds to the size in bytes of a single texel.
	 */
	uint32_t getBytesPerBlock(DataFormat p_format);

	/**
	 * Returns the size, in bytes, of a texel block of the specified Metal format.
	 * For uncompressed formats, the returned value corresponds to the size in bytes of a single texel.
	 */
	uint32_t getBytesPerBlock(MTL::PixelFormat p_format);

	/** Returns the number of planes of the specified chroma-subsampling (YCbCr) DataFormat */
	uint8_t getChromaSubsamplingPlaneCount(DataFormat p_format);

	/** Returns the number of bits per channel of the specified chroma-subsampling (YCbCr) DataFormat */
	uint8_t getChromaSubsamplingComponentBits(DataFormat p_format);

	/**
	 * Returns the size, in bytes, of a texel of the specified Godot format.
	 * The returned value may be fractional for certain compressed formats.
	 */
	float getBytesPerTexel(DataFormat p_format);

	/**
	 * Returns the size, in bytes, of a texel of the specified Metal format.
	 * The returned value may be fractional for certain compressed formats.
	 */
	float getBytesPerTexel(MTL::PixelFormat p_format);

	/**
	 * Returns the size, in bytes, of a row of texels of the specified Godot pixel format.
	 *
	 * For compressed formats, this takes into consideration the compression block size,
	 * and p_texels_per_row should specify the width in texels, not blocks. The result is rounded
	 * up if p_texels_per_row is not an integer multiple of the compression block width.
	 */
	size_t getBytesPerRow(DataFormat p_format, uint32_t p_texels_per_row);

	/**
	 * Returns the size, in bytes, of a row of texels of the specified Metal format.
	 *
	 * For compressed formats, this takes into consideration the compression block size,
	 * and texelsPerRow should specify the width in texels, not blocks. The result is rounded
	 * up if texelsPerRow is not an integer multiple of the compression block width.
	 */
	size_t getBytesPerRow(MTL::PixelFormat p_format, uint32_t p_texels_per_row);

	/**
	 * Returns the size, in bytes, of a texture layer of the specified Godot pixel format.
	 *
	 * For compressed formats, this takes into consideration the compression block size,
	 * and p_texel_rows_per_layer should specify the height in texels, not blocks. The result is
	 * rounded up if p_texel_rows_per_layer is not an integer multiple of the compression block height.
	 */
	size_t getBytesPerLayer(DataFormat p_format, size_t p_bytes_per_row, uint32_t p_texel_rows_per_layer);

	/**
	 * Returns the size, in bytes, of a texture layer of the specified Metal format.
	 * For compressed formats, this takes into consideration the compression block size,
	 * and p_texel_rows_per_layer should specify the height in texels, not blocks. The result is
	 * rounded up if p_texel_rows_per_layer is not an integer multiple of the compression block height.
	 */
	size_t getBytesPerLayer(MTL::PixelFormat p_format, size_t p_bytes_per_row, uint32_t p_texel_rows_per_layer);

	/** Returns whether or not the specified Godot format requires swizzling to use with Metal. */
	bool needsSwizzle(DataFormat p_format);

	/** Returns the Metal format capabilities supported by the specified Godot format, without substitution. */
	MTLFmtCaps getCapabilities(DataFormat p_format, bool p_extended = false);

	/** Returns the Metal format capabilities supported by the specified Metal format. */
	MTLFmtCaps getCapabilities(MTL::PixelFormat p_format, bool p_extended = false);

	/**
	 * Returns the Metal MTL::VertexFormat corresponding to the specified
	 * DataFormat as used as a vertex attribute format.
	 */
	MTL::VertexFormat getMTLVertexFormat(DataFormat p_format);

#pragma mark Construction

	explicit PixelFormats(MTL::Device *p_device, const MetalFeatures &p_feat);
	~PixelFormats();

protected:
	DataFormatDesc &getDataFormatDesc(DataFormat p_format);
	DataFormatDesc &getDataFormatDesc(MTL::PixelFormat p_format);
	MTLFormatDesc &getMTLPixelFormatDesc(MTL::PixelFormat p_format);
	MTLFmtCaps &getMTLPixelFormatCapsIf(MTL::PixelFormat mtlPixFmt, bool cond);
	MTLFormatDesc &getMTLVertexFormatDesc(MTL::VertexFormat p_format);

	void initDataFormatCapabilities();
	void initMTLPixelFormatCapabilities();
	void initMTLVertexFormatCapabilities(const MetalFeatures &p_feat);
	void modifyMTLFormatCapabilities(const MetalFeatures &p_feat);
	void buildDFFormatMaps();
	void addMTLPixelFormatDescImpl(MTL::PixelFormat p_pix_fmt, MTL::PixelFormat p_pix_fmt_linear,
			MTLViewClass p_view_class, MTLFmtCaps p_fmt_caps, const char *p_name);
	void addMTLVertexFormatDescImpl(MTL::VertexFormat p_vert_fmt, MTLFmtCaps p_vert_caps, const char *name);

	MTL::Device *device;
	InflectionMap<DataFormat, DataFormatDesc, RDC::DATA_FORMAT_MAX> _data_format_descs;
	InflectionMap<uint16_t, MTLFormatDesc, MTL::PixelFormatX32_Stencil8 + 2> _mtl_pixel_format_descs; // The actual last enum value is not available on iOS.
	TightLocalVector<MTLFormatDesc> _mtl_vertex_format_descs;
};

GODOT_CLANG_WARNING_POP
