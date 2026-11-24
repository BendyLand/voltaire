/**************************************************************************/
/*  rendering_device.h                                                    */
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

#include "core/os/condition_variable.h"
#include "core/os/os.h"
#include "core/templates/local_vector.h"
#include "core/templates/hash_map.h"
#include "core/templates/rb_set.h"
#include "core/types.h"
#include "scene/resources/texture.h"
#include "servers/display/display_server.h"
// --- Folded from servers/rendering/rendering_device_commons.h ---
#include "core/types.h"
#include "core/typedefs.h"

#define STEPIFY(m_number, m_alignment)                                         \
    ((((m_number) + ((m_alignment)-1)) / (m_alignment)) * (m_alignment))

template <typename T> class VectorView
{
    const T* _ptr = nullptr;
    uint32_t _size = 0;

public:
    const T& operator[](uint32_t p_index) const
    {
        DEV_ASSERT(p_index < _size);
        return _ptr[p_index];
    }

    _ALWAYS_INLINE_ const T* ptr() const { return _ptr; }

    _ALWAYS_INLINE_ uint32_t size() const { return _size; }

    VectorView() = default;

    VectorView(const T& p_ptr)
        : _ptr(&p_ptr), _size(1)
    {
    }

    VectorView(const T* p_ptr, uint32_t p_size) : _ptr(p_ptr), _size(p_size) {}

    VectorView(const Vector<T>& p_lv) : _ptr(p_lv.ptr()), _size(p_lv.size()) {}

    VectorView(const LocalVector<T>& p_lv) : _ptr(p_lv.ptr()), _size(p_lv.size()) {}
};

class RenderingDeviceCommons final
{
public:
    static const bool command_pool_reset_enabled = true;

    /*****************/
    /**** GENERIC ****/
    /*****************/

    static const int INVALID_ID = -1;

    enum DataFormat
    {
        DATA_FORMAT_R4G4_UNORM_PACK8,
        DATA_FORMAT_R4G4B4A4_UNORM_PACK16,
        DATA_FORMAT_B4G4R4A4_UNORM_PACK16,
        DATA_FORMAT_R5G6B5_UNORM_PACK16,
        DATA_FORMAT_B5G6R5_UNORM_PACK16,
        DATA_FORMAT_R5G5B5A1_UNORM_PACK16,
        DATA_FORMAT_B5G5R5A1_UNORM_PACK16,
        DATA_FORMAT_A1R5G5B5_UNORM_PACK16,
        DATA_FORMAT_R8_UNORM,
        DATA_FORMAT_R8_SNORM,
        DATA_FORMAT_R8_USCALED,
        DATA_FORMAT_R8_SSCALED,
        DATA_FORMAT_R8_UINT,
        DATA_FORMAT_R8_SINT,
        DATA_FORMAT_R8_SRGB,
        DATA_FORMAT_R8G8_UNORM,
        DATA_FORMAT_R8G8_SNORM,
        DATA_FORMAT_R8G8_USCALED,
        DATA_FORMAT_R8G8_SSCALED,
        DATA_FORMAT_R8G8_UINT,
        DATA_FORMAT_R8G8_SINT,
        DATA_FORMAT_R8G8_SRGB,
        DATA_FORMAT_R8G8B8_UNORM,
        DATA_FORMAT_R8G8B8_SNORM,
        DATA_FORMAT_R8G8B8_USCALED,
        DATA_FORMAT_R8G8B8_SSCALED,
        DATA_FORMAT_R8G8B8_UINT,
        DATA_FORMAT_R8G8B8_SINT,
        DATA_FORMAT_R8G8B8_SRGB,
        DATA_FORMAT_B8G8R8_UNORM,
        DATA_FORMAT_B8G8R8_SNORM,
        DATA_FORMAT_B8G8R8_USCALED,
        DATA_FORMAT_B8G8R8_SSCALED,
        DATA_FORMAT_B8G8R8_UINT,
        DATA_FORMAT_B8G8R8_SINT,
        DATA_FORMAT_B8G8R8_SRGB,
        DATA_FORMAT_R8G8B8A8_UNORM,
        DATA_FORMAT_R8G8B8A8_SNORM,
        DATA_FORMAT_R8G8B8A8_USCALED,
        DATA_FORMAT_R8G8B8A8_SSCALED,
        DATA_FORMAT_R8G8B8A8_UINT,
        DATA_FORMAT_R8G8B8A8_SINT,
        DATA_FORMAT_R8G8B8A8_SRGB,
        DATA_FORMAT_B8G8R8A8_UNORM,
        DATA_FORMAT_B8G8R8A8_SNORM,
        DATA_FORMAT_B8G8R8A8_USCALED,
        DATA_FORMAT_B8G8R8A8_SSCALED,
        DATA_FORMAT_B8G8R8A8_UINT,
        DATA_FORMAT_B8G8R8A8_SINT,
        DATA_FORMAT_B8G8R8A8_SRGB,
        DATA_FORMAT_A8B8G8R8_UNORM_PACK32,
        DATA_FORMAT_A8B8G8R8_SNORM_PACK32,
        DATA_FORMAT_A8B8G8R8_USCALED_PACK32,
        DATA_FORMAT_A8B8G8R8_SSCALED_PACK32,
        DATA_FORMAT_A8B8G8R8_UINT_PACK32,
        DATA_FORMAT_A8B8G8R8_SINT_PACK32,
        DATA_FORMAT_A8B8G8R8_SRGB_PACK32,
        DATA_FORMAT_A2R10G10B10_UNORM_PACK32,
        DATA_FORMAT_A2R10G10B10_SNORM_PACK32,
        DATA_FORMAT_A2R10G10B10_USCALED_PACK32,
        DATA_FORMAT_A2R10G10B10_SSCALED_PACK32,
        DATA_FORMAT_A2R10G10B10_UINT_PACK32,
        DATA_FORMAT_A2R10G10B10_SINT_PACK32,
        DATA_FORMAT_A2B10G10R10_UNORM_PACK32,
        DATA_FORMAT_A2B10G10R10_SNORM_PACK32,
        DATA_FORMAT_A2B10G10R10_USCALED_PACK32,
        DATA_FORMAT_A2B10G10R10_SSCALED_PACK32,
        DATA_FORMAT_A2B10G10R10_UINT_PACK32,
        DATA_FORMAT_A2B10G10R10_SINT_PACK32,
        DATA_FORMAT_R16_UNORM,
        DATA_FORMAT_R16_SNORM,
        DATA_FORMAT_R16_USCALED,
        DATA_FORMAT_R16_SSCALED,
        DATA_FORMAT_R16_UINT,
        DATA_FORMAT_R16_SINT,
        DATA_FORMAT_R16_SFLOAT,
        DATA_FORMAT_R16G16_UNORM,
        DATA_FORMAT_R16G16_SNORM,
        DATA_FORMAT_R16G16_USCALED,
        DATA_FORMAT_R16G16_SSCALED,
        DATA_FORMAT_R16G16_UINT,
        DATA_FORMAT_R16G16_SINT,
        DATA_FORMAT_R16G16_SFLOAT,
        DATA_FORMAT_R16G16B16_UNORM,
        DATA_FORMAT_R16G16B16_SNORM,
        DATA_FORMAT_R16G16B16_USCALED,
        DATA_FORMAT_R16G16B16_SSCALED,
        DATA_FORMAT_R16G16B16_UINT,
        DATA_FORMAT_R16G16B16_SINT,
        DATA_FORMAT_R16G16B16_SFLOAT,
        DATA_FORMAT_R16G16B16A16_UNORM,
        DATA_FORMAT_R16G16B16A16_SNORM,
        DATA_FORMAT_R16G16B16A16_USCALED,
        DATA_FORMAT_R16G16B16A16_SSCALED,
        DATA_FORMAT_R16G16B16A16_UINT,
        DATA_FORMAT_R16G16B16A16_SINT,
        DATA_FORMAT_R16G16B16A16_SFLOAT,
        DATA_FORMAT_R32_UINT,
        DATA_FORMAT_R32_SINT,
        DATA_FORMAT_R32_SFLOAT,
        DATA_FORMAT_R32G32_UINT,
        DATA_FORMAT_R32G32_SINT,
        DATA_FORMAT_R32G32_SFLOAT,
        DATA_FORMAT_R32G32B32_UINT,
        DATA_FORMAT_R32G32B32_SINT,
        DATA_FORMAT_R32G32B32_SFLOAT,
        DATA_FORMAT_R32G32B32A32_UINT,
        DATA_FORMAT_R32G32B32A32_SINT,
        DATA_FORMAT_R32G32B32A32_SFLOAT,
        DATA_FORMAT_R64_UINT,
        DATA_FORMAT_R64_SINT,
        DATA_FORMAT_R64_SFLOAT,
        DATA_FORMAT_R64G64_UINT,
        DATA_FORMAT_R64G64_SINT,
        DATA_FORMAT_R64G64_SFLOAT,
        DATA_FORMAT_R64G64B64_UINT,
        DATA_FORMAT_R64G64B64_SINT,
        DATA_FORMAT_R64G64B64_SFLOAT,
        DATA_FORMAT_R64G64B64A64_UINT,
        DATA_FORMAT_R64G64B64A64_SINT,
        DATA_FORMAT_R64G64B64A64_SFLOAT,
        DATA_FORMAT_B10G11R11_UFLOAT_PACK32,
        DATA_FORMAT_E5B9G9R9_UFLOAT_PACK32,
        DATA_FORMAT_D16_UNORM,
        DATA_FORMAT_X8_D24_UNORM_PACK32,
        DATA_FORMAT_D32_SFLOAT,
        DATA_FORMAT_S8_UINT,
        DATA_FORMAT_D16_UNORM_S8_UINT,
        DATA_FORMAT_D24_UNORM_S8_UINT,
        DATA_FORMAT_D32_SFLOAT_S8_UINT,
        DATA_FORMAT_BC1_RGB_UNORM_BLOCK,
        DATA_FORMAT_BC1_RGB_SRGB_BLOCK,
        DATA_FORMAT_BC1_RGBA_UNORM_BLOCK,
        DATA_FORMAT_BC1_RGBA_SRGB_BLOCK,
        DATA_FORMAT_BC2_UNORM_BLOCK,
        DATA_FORMAT_BC2_SRGB_BLOCK,
        DATA_FORMAT_BC3_UNORM_BLOCK,
        DATA_FORMAT_BC3_SRGB_BLOCK,
        DATA_FORMAT_BC4_UNORM_BLOCK,
        DATA_FORMAT_BC4_SNORM_BLOCK,
        DATA_FORMAT_BC5_UNORM_BLOCK,
        DATA_FORMAT_BC5_SNORM_BLOCK,
        DATA_FORMAT_BC6H_UFLOAT_BLOCK,
        DATA_FORMAT_BC6H_SFLOAT_BLOCK,
        DATA_FORMAT_BC7_UNORM_BLOCK,
        DATA_FORMAT_BC7_SRGB_BLOCK,
        DATA_FORMAT_ETC2_R8G8B8_UNORM_BLOCK,
        DATA_FORMAT_ETC2_R8G8B8_SRGB_BLOCK,
        DATA_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK,
        DATA_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK,
        DATA_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK,
        DATA_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK,
        DATA_FORMAT_EAC_R11_UNORM_BLOCK,
        DATA_FORMAT_EAC_R11_SNORM_BLOCK,
        DATA_FORMAT_EAC_R11G11_UNORM_BLOCK,
        DATA_FORMAT_EAC_R11G11_SNORM_BLOCK,
        DATA_FORMAT_ASTC_4x4_UNORM_BLOCK,
        DATA_FORMAT_ASTC_4x4_SRGB_BLOCK,
        DATA_FORMAT_ASTC_5x4_UNORM_BLOCK,
        DATA_FORMAT_ASTC_5x4_SRGB_BLOCK,
        DATA_FORMAT_ASTC_5x5_UNORM_BLOCK,
        DATA_FORMAT_ASTC_5x5_SRGB_BLOCK,
        DATA_FORMAT_ASTC_6x5_UNORM_BLOCK,
        DATA_FORMAT_ASTC_6x5_SRGB_BLOCK,
        DATA_FORMAT_ASTC_6x6_UNORM_BLOCK,
        DATA_FORMAT_ASTC_6x6_SRGB_BLOCK,
        DATA_FORMAT_ASTC_8x5_UNORM_BLOCK,
        DATA_FORMAT_ASTC_8x5_SRGB_BLOCK,
        DATA_FORMAT_ASTC_8x6_UNORM_BLOCK,
        DATA_FORMAT_ASTC_8x6_SRGB_BLOCK,
        DATA_FORMAT_ASTC_8x8_UNORM_BLOCK,
        DATA_FORMAT_ASTC_8x8_SRGB_BLOCK,
        DATA_FORMAT_ASTC_10x5_UNORM_BLOCK,
        DATA_FORMAT_ASTC_10x5_SRGB_BLOCK,
        DATA_FORMAT_ASTC_10x6_UNORM_BLOCK,
        DATA_FORMAT_ASTC_10x6_SRGB_BLOCK,
        DATA_FORMAT_ASTC_10x8_UNORM_BLOCK,
        DATA_FORMAT_ASTC_10x8_SRGB_BLOCK,
        DATA_FORMAT_ASTC_10x10_UNORM_BLOCK,
        DATA_FORMAT_ASTC_10x10_SRGB_BLOCK,
        DATA_FORMAT_ASTC_12x10_UNORM_BLOCK,
        DATA_FORMAT_ASTC_12x10_SRGB_BLOCK,
        DATA_FORMAT_ASTC_12x12_UNORM_BLOCK,
        DATA_FORMAT_ASTC_12x12_SRGB_BLOCK,
        DATA_FORMAT_G8B8G8R8_422_UNORM,
        DATA_FORMAT_B8G8R8G8_422_UNORM,
        DATA_FORMAT_G8_B8_R8_3PLANE_420_UNORM,
        DATA_FORMAT_G8_B8R8_2PLANE_420_UNORM,
        DATA_FORMAT_G8_B8_R8_3PLANE_422_UNORM,
        DATA_FORMAT_G8_B8R8_2PLANE_422_UNORM,
        DATA_FORMAT_G8_B8_R8_3PLANE_444_UNORM,
        DATA_FORMAT_R10X6_UNORM_PACK16,
        DATA_FORMAT_R10X6G10X6_UNORM_2PACK16,
        DATA_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16,
        DATA_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16,
        DATA_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16,
        DATA_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16,
        DATA_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16,
        DATA_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16,
        DATA_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16,
        DATA_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16,
        DATA_FORMAT_R12X4_UNORM_PACK16,
        DATA_FORMAT_R12X4G12X4_UNORM_2PACK16,
        DATA_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16,
        DATA_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16,
        DATA_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16,
        DATA_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16,
        DATA_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16,
        DATA_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16,
        DATA_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16,
        DATA_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16,
        DATA_FORMAT_G16B16G16R16_422_UNORM,
        DATA_FORMAT_B16G16R16G16_422_UNORM,
        DATA_FORMAT_G16_B16_R16_3PLANE_420_UNORM,
        DATA_FORMAT_G16_B16R16_2PLANE_420_UNORM,
        DATA_FORMAT_G16_B16_R16_3PLANE_422_UNORM,
        DATA_FORMAT_G16_B16R16_2PLANE_422_UNORM,
        DATA_FORMAT_G16_B16_R16_3PLANE_444_UNORM,
        DATA_FORMAT_ASTC_4x4_SFLOAT_BLOCK,
        DATA_FORMAT_ASTC_5x4_SFLOAT_BLOCK,
        DATA_FORMAT_ASTC_5x5_SFLOAT_BLOCK,
        DATA_FORMAT_ASTC_6x5_SFLOAT_BLOCK,
        DATA_FORMAT_ASTC_6x6_SFLOAT_BLOCK,
        DATA_FORMAT_ASTC_8x5_SFLOAT_BLOCK,
        DATA_FORMAT_ASTC_8x6_SFLOAT_BLOCK,
        DATA_FORMAT_ASTC_8x8_SFLOAT_BLOCK,
        DATA_FORMAT_ASTC_10x5_SFLOAT_BLOCK,
        DATA_FORMAT_ASTC_10x6_SFLOAT_BLOCK,
        DATA_FORMAT_ASTC_10x8_SFLOAT_BLOCK,
        DATA_FORMAT_ASTC_10x10_SFLOAT_BLOCK,
        DATA_FORMAT_ASTC_12x10_SFLOAT_BLOCK,
        DATA_FORMAT_ASTC_12x12_SFLOAT_BLOCK,
        DATA_FORMAT_MAX,
    };

    enum ColorSpace
    {
        COLOR_SPACE_REC709_LINEAR,
        COLOR_SPACE_REC709_NONLINEAR_SRGB,
        COLOR_SPACE_MAX,
    };

    enum BreadcrumbMarker
    {
        NONE = 0,
        REFLECTION_PROBES = 1u << 16u,
        SKY_PASS = 2u << 16u,
        LIGHTMAPPER_PASS = 3u << 16u,
        SHADOW_PASS_DIRECTIONAL = 4u << 16u,
        SHADOW_PASS_CUBE = 5u << 16u,
        OPAQUE_PASS = 6u << 16u,
        ALPHA_PASS = 7u << 16u,
        TRANSPARENT_PASS = 8u << 16u,
        POST_PROCESSING_PASS = 9u << 16u,
        BLIT_PASS = 10u << 16u,
        UI_PASS = 11u << 16u,
        DEBUG_PASS = 12u << 16u,
    };

    enum CompareOperator
    {
        COMPARE_OP_NEVER,
        COMPARE_OP_LESS,
        COMPARE_OP_EQUAL,
        COMPARE_OP_LESS_OR_EQUAL,
        COMPARE_OP_GREATER,
        COMPARE_OP_NOT_EQUAL,
        COMPARE_OP_GREATER_OR_EQUAL,
        COMPARE_OP_ALWAYS,
        COMPARE_OP_MAX
    };

    /*****************/
    /**** TEXTURE ****/
    /*****************/

    enum TextureType
    {
        TEXTURE_TYPE_1D,
        TEXTURE_TYPE_2D,
        TEXTURE_TYPE_3D,
        TEXTURE_TYPE_CUBE,
        TEXTURE_TYPE_1D_ARRAY,
        TEXTURE_TYPE_2D_ARRAY,
        TEXTURE_TYPE_CUBE_ARRAY,
        TEXTURE_TYPE_MAX,
    };

    enum TextureSamples
    {
        TEXTURE_SAMPLES_1,
        TEXTURE_SAMPLES_2,
        TEXTURE_SAMPLES_4,
        TEXTURE_SAMPLES_8,
        TEXTURE_SAMPLES_16,
        TEXTURE_SAMPLES_32,
        TEXTURE_SAMPLES_64,
        TEXTURE_SAMPLES_MAX,
    };

    enum TextureUsageBits
    {
        TEXTURE_USAGE_SAMPLING_BIT = (1 << 0),
        TEXTURE_USAGE_COLOR_ATTACHMENT_BIT = (1 << 1),
        TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT = (1 << 2),
        TEXTURE_USAGE_STORAGE_BIT = (1 << 3),
        TEXTURE_USAGE_STORAGE_ATOMIC_BIT = (1 << 4),
        TEXTURE_USAGE_CPU_READ_BIT = (1 << 5),
        TEXTURE_USAGE_CAN_UPDATE_BIT = (1 << 6),
        TEXTURE_USAGE_CAN_COPY_FROM_BIT = (1 << 7),
        TEXTURE_USAGE_CAN_COPY_TO_BIT = (1 << 8),
        TEXTURE_USAGE_INPUT_ATTACHMENT_BIT = (1 << 9),
        TEXTURE_USAGE_VRS_ATTACHMENT_BIT = (1 << 10),
        TEXTURE_USAGE_TRANSIENT_BIT = (1 << 11),
        TEXTURE_USAGE_DEPTH_RESOLVE_ATTACHMENT_BIT = (1 << 12),
        TEXTURE_USAGE_MAX_BIT = TEXTURE_USAGE_DEPTH_RESOLVE_ATTACHMENT_BIT,
    };

    struct TextureFormat
    {
        DataFormat format = DATA_FORMAT_R8_UNORM;
        uint32_t width = 1;
        uint32_t height = 1;
        uint32_t depth = 1;
        uint32_t array_layers = 1;
        uint32_t mipmaps = 1;
        TextureType texture_type = TEXTURE_TYPE_2D;
        TextureSamples samples = TEXTURE_SAMPLES_1;
        uint32_t usage_bits = 0;
        Vector<DataFormat> shareable_formats;
        bool is_resolve_buffer = false;
        bool is_discardable = false;
        bool is_subsampled = false;

        bool operator==(const TextureFormat& b) const
        {
            if (format != b.format) {
                return false;
            }
            else if (width != b.width) {
                return false;
            }
            else if (height != b.height) {
                return false;
            }
            else if (depth != b.depth) {
                return false;
            }
            else if (array_layers != b.array_layers) {
                return false;
            }
            else if (mipmaps != b.mipmaps) {
                return false;
            }
            else if (texture_type != b.texture_type) {
                return false;
            }
            else if (samples != b.samples) {
                return false;
            }
            else if (usage_bits != b.usage_bits) {
                return false;
            }
            else if (shareable_formats != b.shareable_formats) {
                return false;
            }
            else if (is_resolve_buffer != b.is_resolve_buffer) {
                return false;
            }
            else if (is_discardable != b.is_discardable) {
                return false;
            }
            else {
                return true;
            }
        }
    };

    enum TextureSwizzle
    {
        TEXTURE_SWIZZLE_IDENTITY,
        TEXTURE_SWIZZLE_ZERO,
        TEXTURE_SWIZZLE_ONE,
        TEXTURE_SWIZZLE_R,
        TEXTURE_SWIZZLE_G,
        TEXTURE_SWIZZLE_B,
        TEXTURE_SWIZZLE_A,
        TEXTURE_SWIZZLE_MAX
    };

    enum TextureSliceType
    {
        TEXTURE_SLICE_2D,
        TEXTURE_SLICE_CUBEMAP,
        TEXTURE_SLICE_3D,
        TEXTURE_SLICE_2D_ARRAY,
        TEXTURE_SLICE_MAX
    };

    /*****************/
    /**** SAMPLER ****/
    /*****************/

    enum SamplerFilter
    {
        SAMPLER_FILTER_NEAREST,
        SAMPLER_FILTER_LINEAR,
    };

    enum SamplerRepeatMode
    {
        SAMPLER_REPEAT_MODE_REPEAT,
        SAMPLER_REPEAT_MODE_MIRRORED_REPEAT,
        SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE,
        SAMPLER_REPEAT_MODE_CLAMP_TO_BORDER,
        SAMPLER_REPEAT_MODE_MIRROR_CLAMP_TO_EDGE,
        SAMPLER_REPEAT_MODE_MAX
    };

    enum SamplerBorderColor
    {
        SAMPLER_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
        SAMPLER_BORDER_COLOR_INT_TRANSPARENT_BLACK,
        SAMPLER_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
        SAMPLER_BORDER_COLOR_INT_OPAQUE_BLACK,
        SAMPLER_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
        SAMPLER_BORDER_COLOR_INT_OPAQUE_WHITE,
        SAMPLER_BORDER_COLOR_MAX
    };

    struct SamplerState
    {
        SamplerFilter mag_filter = SAMPLER_FILTER_NEAREST;
        SamplerFilter min_filter = SAMPLER_FILTER_NEAREST;
        SamplerFilter mip_filter = SAMPLER_FILTER_NEAREST;
        SamplerRepeatMode repeat_u = SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE;
        SamplerRepeatMode repeat_v = SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE;
        SamplerRepeatMode repeat_w = SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE;
        float lod_bias = 0.0f;
        bool use_anisotropy = false;
        float anisotropy_max = 1.0f;
        bool enable_compare = false;
        CompareOperator compare_op = COMPARE_OP_ALWAYS;
        float min_lod = 0.0f;
        float max_lod = 1e20;
        SamplerBorderColor border_color = SAMPLER_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        bool unnormalized_uvw = false;
    };

    /**********************/
    /**** VERTEX ARRAY ****/
    /**********************/

    enum IndexBufferFormat
    {
        INDEX_BUFFER_FORMAT_UINT16,
        INDEX_BUFFER_FORMAT_UINT32,
    };

    enum VertexFrequency
    {
        VERTEX_FREQUENCY_VERTEX,
        VERTEX_FREQUENCY_INSTANCE,
    };

    struct VertexAttribute
    {
        uint32_t binding = UINT32_MAX;
        uint32_t location = 0;
        uint32_t offset = 0;
        DataFormat format = DATA_FORMAT_MAX;
        uint32_t stride = 0;
        VertexFrequency frequency = VERTEX_FREQUENCY_VERTEX;
    };

    struct VertexAttributeBinding
    {
        uint32_t stride = 0;
        VertexFrequency frequency = VERTEX_FREQUENCY_VERTEX;

        VertexAttributeBinding() = default;

        VertexAttributeBinding(uint32_t p_stride, VertexFrequency p_frequency)
            : stride(p_stride), frequency(p_frequency)
        {
        }
    };

    typedef HashMap<uint32_t, VertexAttributeBinding> VertexAttributeBindingsMap;

    /*********************/
    /**** FRAMEBUFFER ****/
    /*********************/

    static const int32_t ATTACHMENT_UNUSED = -1;

    /****************/
    /**** SHADER ****/
    /****************/

    enum ShaderStage
    {
        SHADER_STAGE_VERTEX,
        SHADER_STAGE_FRAGMENT,
        SHADER_STAGE_TESSELATION_CONTROL,
        SHADER_STAGE_TESSELATION_EVALUATION,
        SHADER_STAGE_COMPUTE,
        SHADER_STAGE_RAYGEN,
        SHADER_STAGE_ANY_HIT,
        SHADER_STAGE_CLOSEST_HIT,
        SHADER_STAGE_MISS,
        SHADER_STAGE_INTERSECTION,
        SHADER_STAGE_MAX,
        SHADER_STAGE_VERTEX_BIT = (1 << SHADER_STAGE_VERTEX),
        SHADER_STAGE_FRAGMENT_BIT = (1 << SHADER_STAGE_FRAGMENT),
        SHADER_STAGE_TESSELATION_CONTROL_BIT = (1 << SHADER_STAGE_TESSELATION_CONTROL),
        SHADER_STAGE_TESSELATION_EVALUATION_BIT = (1 << SHADER_STAGE_TESSELATION_EVALUATION),
        SHADER_STAGE_COMPUTE_BIT = (1 << SHADER_STAGE_COMPUTE),
        SHADER_STAGE_RAYGEN_BIT = (1 << SHADER_STAGE_RAYGEN),
        SHADER_STAGE_ANY_HIT_BIT = (1 << SHADER_STAGE_ANY_HIT),
        SHADER_STAGE_CLOSEST_HIT_BIT = (1 << SHADER_STAGE_CLOSEST_HIT),
        SHADER_STAGE_MISS_BIT = (1 << SHADER_STAGE_MISS),
        SHADER_STAGE_INTERSECTION_BIT = (1 << SHADER_STAGE_INTERSECTION),
    };

    enum ShaderLanguage
    {
        SHADER_LANGUAGE_GLSL,
        SHADER_LANGUAGE_HLSL,
    };

    enum ShaderLanguageVersion
    {
        SHADER_LANGUAGE_VULKAN_VERSION_1_0 = (1 << 22),
        SHADER_LANGUAGE_VULKAN_VERSION_1_1 = (1 << 22) | (1 << 12),
        SHADER_LANGUAGE_VULKAN_VERSION_1_2 = (1 << 22) | (2 << 12),
        SHADER_LANGUAGE_VULKAN_VERSION_1_3 = (1 << 22) | (3 << 12),
        SHADER_LANGUAGE_VULKAN_VERSION_1_4 = (1 << 22) | (4 << 12),
        SHADER_LANGUAGE_OPENGL_VERSION_4_5_0 = 450,
    };

    enum ShaderSpirvVersion
    {
        SHADER_SPIRV_VERSION_1_0 = (1 << 16),
        SHADER_SPIRV_VERSION_1_1 = (1 << 16) | (1 << 8),
        SHADER_SPIRV_VERSION_1_2 = (1 << 16) | (2 << 8),
        SHADER_SPIRV_VERSION_1_3 = (1 << 16) | (3 << 8),
        SHADER_SPIRV_VERSION_1_4 = (1 << 16) | (4 << 8),
        SHADER_SPIRV_VERSION_1_5 = (1 << 16) | (5 << 8),
        SHADER_SPIRV_VERSION_1_6 = (1 << 16) | (6 << 8),
    };

    struct ShaderStageSPIRVData
    {
        ShaderStage shader_stage = SHADER_STAGE_MAX;
        Vector<uint8_t> spirv;
        Vector<uint64_t> dynamic_buffers;
    };

    /*********************/
    /**** UNIFORM SET ****/
    /*********************/

    static const uint32_t MAX_UNIFORM_SETS = 16;

    enum UniformType
    {
        UNIFORM_TYPE_SAMPLER,
        UNIFORM_TYPE_SAMPLER_WITH_TEXTURE,
        UNIFORM_TYPE_TEXTURE,
        UNIFORM_TYPE_IMAGE,
        UNIFORM_TYPE_TEXTURE_BUFFER,
        UNIFORM_TYPE_SAMPLER_WITH_TEXTURE_BUFFER,
        UNIFORM_TYPE_IMAGE_BUFFER,
        UNIFORM_TYPE_UNIFORM_BUFFER,
        UNIFORM_TYPE_STORAGE_BUFFER,
        UNIFORM_TYPE_INPUT_ATTACHMENT,
        UNIFORM_TYPE_UNIFORM_BUFFER_DYNAMIC,
        UNIFORM_TYPE_STORAGE_BUFFER_DYNAMIC,
        UNIFORM_TYPE_ACCELERATION_STRUCTURE,
        UNIFORM_TYPE_MAX
    };

    /******************/
    /**** PIPELINE ****/
    /******************/

    enum PipelineSpecializationConstantType
    {
        PIPELINE_SPECIALIZATION_CONSTANT_TYPE_BOOL,
        PIPELINE_SPECIALIZATION_CONSTANT_TYPE_INT,
        PIPELINE_SPECIALIZATION_CONSTANT_TYPE_FLOAT,
    };

    struct PipelineSpecializationConstant
    {
        PipelineSpecializationConstantType type = {};
        uint32_t constant_id = 0xffffffff;

        union
        {
            uint32_t int_value = 0;
            float float_value;
            bool bool_value;
        };

        bool operator==(const PipelineSpecializationConstant& p_rhs) const
        {
            if (type != p_rhs.type) {
                return false;
            }
            if (constant_id != p_rhs.constant_id) {
                return false;
            }
            if (int_value != p_rhs.int_value) {
                return false;
            }
            return true;
        }

        bool operator!=(const PipelineSpecializationConstant& p_rhs) const
        {
            return !(*this == p_rhs);
        }
    };

    /*******************/
    /**** RENDERING ****/
    /*******************/

    enum PipelineType : uint32_t
    {
        PIPELINE_TYPE_RASTERIZATION,
        PIPELINE_TYPE_COMPUTE,
        PIPELINE_TYPE_RAYTRACING,
    };

    enum RenderPrimitive
    {
        RENDER_PRIMITIVE_POINTS,
        RENDER_PRIMITIVE_LINES,
        RENDER_PRIMITIVE_LINES_WITH_ADJACENCY,
        RENDER_PRIMITIVE_LINESTRIPS,
        RENDER_PRIMITIVE_LINESTRIPS_WITH_ADJACENCY,
        RENDER_PRIMITIVE_TRIANGLES,
        RENDER_PRIMITIVE_TRIANGLES_WITH_ADJACENCY,
        RENDER_PRIMITIVE_TRIANGLE_STRIPS,
        RENDER_PRIMITIVE_TRIANGLE_STRIPS_WITH_ADJACENCY,
        RENDER_PRIMITIVE_TRIANGLE_STRIPS_WITH_RESTART_INDEX,
        RENDER_PRIMITIVE_TESSELLATION_PATCH,
        RENDER_PRIMITIVE_MAX
    };

    enum PolygonCullMode
    {
        POLYGON_CULL_DISABLED,
        POLYGON_CULL_FRONT,
        POLYGON_CULL_BACK,
        POLYGON_CULL_MAX
    };

    enum PolygonFrontFace
    {
        POLYGON_FRONT_FACE_CLOCKWISE,
        POLYGON_FRONT_FACE_COUNTER_CLOCKWISE,
    };

    enum StencilOperation
    {
        STENCIL_OP_KEEP,
        STENCIL_OP_ZERO,
        STENCIL_OP_REPLACE,
        STENCIL_OP_INCREMENT_AND_CLAMP,
        STENCIL_OP_DECREMENT_AND_CLAMP,
        STENCIL_OP_INVERT,
        STENCIL_OP_INCREMENT_AND_WRAP,
        STENCIL_OP_DECREMENT_AND_WRAP,
        STENCIL_OP_MAX
    };

    enum LogicOperation
    {
        LOGIC_OP_CLEAR,
        LOGIC_OP_AND,
        LOGIC_OP_AND_REVERSE,
        LOGIC_OP_COPY,
        LOGIC_OP_AND_INVERTED,
        LOGIC_OP_NO_OP,
        LOGIC_OP_XOR,
        LOGIC_OP_OR,
        LOGIC_OP_NOR,
        LOGIC_OP_EQUIVALENT,
        LOGIC_OP_INVERT,
        LOGIC_OP_OR_REVERSE,
        LOGIC_OP_COPY_INVERTED,
        LOGIC_OP_OR_INVERTED,
        LOGIC_OP_NAND,
        LOGIC_OP_SET,
        LOGIC_OP_MAX
    };

    enum BlendFactor
    {
        BLEND_FACTOR_ZERO,
        BLEND_FACTOR_ONE,
        BLEND_FACTOR_SRC_COLOR,
        BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
        BLEND_FACTOR_DST_COLOR,
        BLEND_FACTOR_ONE_MINUS_DST_COLOR,
        BLEND_FACTOR_SRC_ALPHA,
        BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        BLEND_FACTOR_DST_ALPHA,
        BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
        BLEND_FACTOR_CONSTANT_COLOR,
        BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,
        BLEND_FACTOR_CONSTANT_ALPHA,
        BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA,
        BLEND_FACTOR_SRC_ALPHA_SATURATE,
        BLEND_FACTOR_SRC1_COLOR,
        BLEND_FACTOR_ONE_MINUS_SRC1_COLOR,
        BLEND_FACTOR_SRC1_ALPHA,
        BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA,
        BLEND_FACTOR_MAX
    };

    enum BlendOperation
    {
        BLEND_OP_ADD,
        BLEND_OP_SUBTRACT,
        BLEND_OP_REVERSE_SUBTRACT,
        BLEND_OP_MINIMUM,
        BLEND_OP_MAXIMUM,
        BLEND_OP_MAX
    };

    struct PipelineRasterizationState
    {
        bool enable_depth_clamp = false;
        bool discard_primitives = false;
        bool wireframe = false;
        PolygonCullMode cull_mode = POLYGON_CULL_DISABLED;
        PolygonFrontFace front_face = POLYGON_FRONT_FACE_CLOCKWISE;
        bool depth_bias_enabled = false;
        float depth_bias_constant_factor = 0.0f;
        float depth_bias_clamp = 0.0f;
        float depth_bias_slope_factor = 0.0f;
        float line_width = 1.0f;
        uint32_t patch_control_points = 1;
    };

    struct PipelineMultisampleState
    {
        TextureSamples sample_count = TEXTURE_SAMPLES_1;
        bool enable_sample_shading = false;
        float min_sample_shading = 0.0f;
        Vector<uint32_t> sample_mask;
        bool enable_alpha_to_coverage = false;
        bool enable_alpha_to_one = false;
    };

    struct PipelineDepthStencilState
    {
        bool enable_depth_test = false;
        bool enable_depth_write = false;
        CompareOperator depth_compare_operator = COMPARE_OP_ALWAYS;
        bool enable_depth_range = false;
        float depth_range_min = 0;
        float depth_range_max = 0;
        bool enable_stencil = false;

        struct StencilOperationState
        {
            StencilOperation fail = STENCIL_OP_ZERO;
            StencilOperation pass = STENCIL_OP_ZERO;
            StencilOperation depth_fail = STENCIL_OP_ZERO;
            CompareOperator compare = COMPARE_OP_ALWAYS;
            uint32_t compare_mask = 0;
            uint32_t write_mask = 0;
            uint32_t reference = 0;
        };

        StencilOperationState front_op;
        StencilOperationState back_op;
    };

    struct PipelineColorBlendState
    {
        bool enable_logic_op = false;
        LogicOperation logic_op = LOGIC_OP_CLEAR;

        struct Attachment
        {
            bool enable_blend = false;
            BlendFactor src_color_blend_factor = BLEND_FACTOR_ZERO;
            BlendFactor dst_color_blend_factor = BLEND_FACTOR_ZERO;
            BlendOperation color_blend_op = BLEND_OP_ADD;
            BlendFactor src_alpha_blend_factor = BLEND_FACTOR_ZERO;
            BlendFactor dst_alpha_blend_factor = BLEND_FACTOR_ZERO;
            BlendOperation alpha_blend_op = BLEND_OP_ADD;
            bool write_r = true;
            bool write_g = true;
            bool write_b = true;
            bool write_a = true;
        };

        static PipelineColorBlendState create_disabled(int p_attachments = 1)
        {
            PipelineColorBlendState bs;
            for (int i = 0; i < p_attachments; i++) {
                bs.attachments.push_back(Attachment());
            }
            return bs;
        }

        static PipelineColorBlendState create_blend(int p_attachments = 1)
        {
            PipelineColorBlendState bs;
            for (int i = 0; i < p_attachments; i++) {
                Attachment ba;
                ba.enable_blend = true;
                ba.src_color_blend_factor = BLEND_FACTOR_SRC_ALPHA;
                ba.dst_color_blend_factor = BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                ba.src_alpha_blend_factor = BLEND_FACTOR_SRC_ALPHA;
                ba.dst_alpha_blend_factor = BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

                bs.attachments.push_back(ba);
            }
            return bs;
        }

        Vector<Attachment> attachments;
        Color blend_constant;
    };

    enum PipelineDynamicStateFlags
    {
        DYNAMIC_STATE_LINE_WIDTH = (1 << 0),
        DYNAMIC_STATE_DEPTH_BIAS = (1 << 1),
        DYNAMIC_STATE_BLEND_CONSTANTS = (1 << 2),
        DYNAMIC_STATE_DEPTH_BOUNDS = (1 << 3),
        DYNAMIC_STATE_STENCIL_COMPARE_MASK = (1 << 4),
        DYNAMIC_STATE_STENCIL_WRITE_MASK = (1 << 5),
        DYNAMIC_STATE_STENCIL_REFERENCE = (1 << 6),
    };

    /********************/
    /**** RAYTRACING ****/
    /********************/

    enum AccelerationStructureType
    {
        ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL,
        ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL,
    };

    enum AccelerationStructureFlagBits
    {
        ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT = (1 << 0),
        ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT = (1 << 1),
        ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT = (1 << 2),
        ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT = (1 << 3),
        ACCELERATION_STRUCTURE_LOW_MEMORY_BIT = (1 << 4),
    };

    enum AccelerationStructureGeometryFlagBits
    {
        ACCELERATION_STRUCTURE_GEOMETRY_OPAQUE_BIT = (1 << 0),
        ACCELERATION_STRUCTURE_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT = (1 << 1),
    };

    enum AccelerationStructureInstanceFlagBits
    {
        ACCELERATION_STRUCTURE_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT = (1 << 0),
        ACCELERATION_STRUCTURE_INSTANCE_TRIANGLE_FLIP_FACING_BIT = (1 << 1),
        ACCELERATION_STRUCTURE_INSTANCE_FORCE_OPAQUE_BIT = (1 << 2),
        ACCELERATION_STRUCTURE_INSTANCE_FORCE_NO_OPAQUE_BIT = (1 << 3),
    };

    /**************/
    /**** MISC ****/
    /**************/

    enum DriverResource
    {
        DRIVER_RESOURCE_LOGICAL_DEVICE,
        DRIVER_RESOURCE_PHYSICAL_DEVICE,
        DRIVER_RESOURCE_TOPMOST_OBJECT,
        DRIVER_RESOURCE_COMMAND_QUEUE,
        DRIVER_RESOURCE_QUEUE_FAMILY,
        DRIVER_RESOURCE_TEXTURE,
        DRIVER_RESOURCE_TEXTURE_VIEW,
        DRIVER_RESOURCE_TEXTURE_DATA_FORMAT,
        DRIVER_RESOURCE_SAMPLER,
        DRIVER_RESOURCE_UNIFORM_SET,
        DRIVER_RESOURCE_BUFFER,
        DRIVER_RESOURCE_COMPUTE_PIPELINE,
        DRIVER_RESOURCE_RENDER_PIPELINE,
#ifndef DISABLE_DEPRECATED
        DRIVER_RESOURCE_VULKAN_DEVICE = DRIVER_RESOURCE_LOGICAL_DEVICE,
        DRIVER_RESOURCE_VULKAN_PHYSICAL_DEVICE = DRIVER_RESOURCE_PHYSICAL_DEVICE,
        DRIVER_RESOURCE_VULKAN_INSTANCE = DRIVER_RESOURCE_TOPMOST_OBJECT,
        DRIVER_RESOURCE_VULKAN_QUEUE = DRIVER_RESOURCE_COMMAND_QUEUE,
        DRIVER_RESOURCE_VULKAN_QUEUE_FAMILY_INDEX = DRIVER_RESOURCE_QUEUE_FAMILY,
        DRIVER_RESOURCE_VULKAN_IMAGE = DRIVER_RESOURCE_TEXTURE,
        DRIVER_RESOURCE_VULKAN_IMAGE_VIEW = DRIVER_RESOURCE_TEXTURE_VIEW,
        DRIVER_RESOURCE_VULKAN_IMAGE_NATIVE_TEXTURE_FORMAT = DRIVER_RESOURCE_TEXTURE_DATA_FORMAT,
        DRIVER_RESOURCE_VULKAN_SAMPLER = DRIVER_RESOURCE_SAMPLER,
        DRIVER_RESOURCE_VULKAN_DESCRIPTOR_SET = DRIVER_RESOURCE_UNIFORM_SET,
        DRIVER_RESOURCE_VULKAN_BUFFER = DRIVER_RESOURCE_BUFFER,
        DRIVER_RESOURCE_VULKAN_COMPUTE_PIPELINE = DRIVER_RESOURCE_COMPUTE_PIPELINE,
        DRIVER_RESOURCE_VULKAN_RENDER_PIPELINE = DRIVER_RESOURCE_RENDER_PIPELINE,
#endif
    };

    enum Limit
    {
        LIMIT_MAX_BOUND_UNIFORM_SETS,
        LIMIT_MAX_FRAMEBUFFER_COLOR_ATTACHMENTS,
        LIMIT_MAX_TEXTURES_PER_UNIFORM_SET,
        LIMIT_MAX_SAMPLERS_PER_UNIFORM_SET,
        LIMIT_MAX_STORAGE_BUFFERS_PER_UNIFORM_SET,
        LIMIT_MAX_STORAGE_IMAGES_PER_UNIFORM_SET,
        LIMIT_MAX_UNIFORM_BUFFERS_PER_UNIFORM_SET,
        LIMIT_MAX_DRAW_INDEXED_INDEX,
        LIMIT_MAX_FRAMEBUFFER_HEIGHT,
        LIMIT_MAX_FRAMEBUFFER_WIDTH,
        LIMIT_MAX_TEXTURE_ARRAY_LAYERS,
        LIMIT_MAX_TEXTURE_SIZE_1D,
        LIMIT_MAX_TEXTURE_SIZE_2D,
        LIMIT_MAX_TEXTURE_SIZE_3D,
        LIMIT_MAX_TEXTURE_SIZE_CUBE,
        LIMIT_MAX_TEXTURES_PER_SHADER_STAGE,
        LIMIT_MAX_SAMPLERS_PER_SHADER_STAGE,
        LIMIT_MAX_STORAGE_BUFFERS_PER_SHADER_STAGE,
        LIMIT_MAX_STORAGE_IMAGES_PER_SHADER_STAGE,
        LIMIT_MAX_UNIFORM_BUFFERS_PER_SHADER_STAGE,
        LIMIT_MAX_PUSH_CONSTANT_SIZE,
        LIMIT_MAX_UNIFORM_BUFFER_SIZE,
        LIMIT_MAX_VERTEX_INPUT_ATTRIBUTE_OFFSET,
        LIMIT_MAX_VERTEX_INPUT_ATTRIBUTES,
        LIMIT_MAX_VERTEX_INPUT_BINDINGS,
        LIMIT_MAX_VERTEX_INPUT_BINDING_STRIDE,
        LIMIT_MIN_UNIFORM_BUFFER_OFFSET_ALIGNMENT,
        LIMIT_MAX_COMPUTE_SHARED_MEMORY_SIZE,
        LIMIT_MAX_COMPUTE_WORKGROUP_COUNT_X,
        LIMIT_MAX_COMPUTE_WORKGROUP_COUNT_Y,
        LIMIT_MAX_COMPUTE_WORKGROUP_COUNT_Z,
        LIMIT_MAX_COMPUTE_WORKGROUP_INVOCATIONS,
        LIMIT_MAX_COMPUTE_WORKGROUP_SIZE_X,
        LIMIT_MAX_COMPUTE_WORKGROUP_SIZE_Y,
        LIMIT_MAX_COMPUTE_WORKGROUP_SIZE_Z,
        LIMIT_MAX_VIEWPORT_DIMENSIONS_X,
        LIMIT_MAX_VIEWPORT_DIMENSIONS_Y,
        LIMIT_SUBGROUP_SIZE,
        LIMIT_SUBGROUP_MIN_SIZE,
        LIMIT_SUBGROUP_MAX_SIZE,
        LIMIT_SUBGROUP_IN_SHADERS,
        LIMIT_SUBGROUP_OPERATIONS,
        LIMIT_METALFX_TEMPORAL_SCALER_MIN_SCALE = 46,
        LIMIT_METALFX_TEMPORAL_SCALER_MAX_SCALE,
        LIMIT_MAX_SHADER_VARYINGS,
    };

    enum Features
    {
        SUPPORTS_MULTIVIEW,
        SUPPORTS_HALF_FLOAT,
        SUPPORTS_ATTACHMENT_VRS,
        SUPPORTS_METALFX_SPATIAL,
        SUPPORTS_METALFX_TEMPORAL,
        SUPPORTS_FRAGMENT_SHADER_WITH_ONLY_SIDE_EFFECTS,
        SUPPORTS_BUFFER_DEVICE_ADDRESS,
        SUPPORTS_IMAGE_ATOMIC_32_BIT,
        SUPPORTS_VULKAN_MEMORY_MODEL,
        SUPPORTS_FRAMEBUFFER_DEPTH_RESOLVE,
        SUPPORTS_POINT_SIZE,
        SUPPORTS_RAY_QUERY,
        SUPPORTS_RAYTRACING_PIPELINE,
        SUPPORTS_HDR_OUTPUT,
    };

    enum SubgroupOperations
    {
        SUBGROUP_BASIC_BIT = 1,
        SUBGROUP_VOTE_BIT = 2,
        SUBGROUP_ARITHMETIC_BIT = 4,
        SUBGROUP_BALLOT_BIT = 8,
        SUBGROUP_SHUFFLE_BIT = 16,
        SUBGROUP_SHUFFLE_RELATIVE_BIT = 32,
        SUBGROUP_CLUSTERED_BIT = 64,
        SUBGROUP_QUAD_BIT = 128,
    };

    struct DriverWorkarounds
    {
        bool avoid_compute_after_draw = false;
        bool dont_print_on_render_pipeline_creation_failure = false;
        bool disable_ubershaders = false;
    };

    /**********************************/
    /**** FORMAT UTILITIES & DATA  ****/
    /**********************************/

    static const char* const FORMAT_NAMES[DATA_FORMAT_MAX];
    static const char* const TEXTURE_TYPE_NAMES[TEXTURE_TYPE_MAX];
    static const uint32_t MAX_IMAGE_FORMAT_PLANES = 2;
    static const uint32_t TEXTURE_SAMPLES_COUNT[TEXTURE_SAMPLES_MAX];
    static void get_compressed_image_format_block_dimensions(
        DataFormat p_format, uint32_t& r_w, uint32_t& r_h);
    static uint32_t get_compressed_image_format_block_byte_size(DataFormat p_format);
    static uint32_t get_compressed_image_format_pixels_shifted(
        DataFormat p_format, uint32_t p_pixels);
    static uint32_t get_image_format_required_size(DataFormat p_format, uint32_t p_width,
        uint32_t p_height, uint32_t p_depth, uint32_t p_mipmaps, uint32_t* r_blockw = nullptr,
        uint32_t* r_blockh = nullptr, uint32_t* r_depth = nullptr);
    static uint32_t get_image_required_mipmaps(
        uint32_t p_width, uint32_t p_height, uint32_t p_depth);
    static bool format_has_depth(DataFormat p_format);
    static bool format_has_stencil(DataFormat p_format);
    static uint32_t format_get_plane_count(DataFormat p_format);
    static const Color SAMPLER_BORDER_COLOR_VALUE[SAMPLER_BORDER_COLOR_MAX];
    static uint32_t get_format_vertex_size(DataFormat p_format);
    static uint32_t get_image_format_pixel_size(DataFormat p_format);
    static const char* SHADER_STAGE_NAMES[SHADER_STAGE_MAX];

    struct ShaderUniform
    {
        UniformType type = UniformType::UNIFORM_TYPE_MAX;
        bool writable = false;
        uint32_t binding = 0;
        uint32_t stages = 0;
        uint32_t length = 0;
        TextureType texture_type = TEXTURE_TYPE_MAX;
        DataFormat texture_format = DATA_FORMAT_MAX;

        bool operator!=(const ShaderUniform& p_other) const
        {
            return binding != p_other.binding || type != p_other.type ||
                   writable != p_other.writable || stages != p_other.stages ||
                   length != p_other.length || texture_type != p_other.texture_type ||
                   texture_format != p_other.texture_format;
        }

        bool operator<(const ShaderUniform& p_other) const
        {
            if (binding != p_other.binding) {
                return binding < p_other.binding;
            }
            if (type != p_other.type) {
                return type < p_other.type;
            }
            if (writable != p_other.writable) {
                return writable < p_other.writable;
            }
            if (stages != p_other.stages) {
                return stages < p_other.stages;
            }
            if (length != p_other.length) {
                return length < p_other.length;
            }
            if (texture_type != p_other.texture_type) {
                return texture_type < p_other.texture_type;
            }
            if (texture_format != p_other.texture_format) {
                return texture_format < p_other.texture_format;
            }
            return false;
        }
    };

    struct ShaderSpecializationConstant
    {
        PipelineSpecializationConstantType type = {};
        uint32_t constant_id = 0xffffffff;

        union
        {
            uint32_t int_value = 0;
            float float_value;
            bool bool_value;
        };

        uint32_t stages = 0;

        operator PipelineSpecializationConstant() const
        {
            PipelineSpecializationConstant psc;
            psc.type = type;
            psc.constant_id = constant_id;
            psc.int_value = int_value;
            return psc;
        }

        bool operator<(const ShaderSpecializationConstant& p_other) const
        {
            return constant_id < p_other.constant_id;
        }
    };

    struct ShaderReflection
    {
        uint64_t vertex_input_mask = 0;
        uint32_t fragment_output_mask = 0;
        PipelineType pipeline_type = PIPELINE_TYPE_RASTERIZATION;
        bool has_multiview = false;
        bool has_dynamic_buffers = false;
        uint32_t compute_local_size[3] = {};
        uint32_t push_constant_size = 0;

        Vector<Vector<ShaderUniform>> uniform_sets;
        Vector<ShaderSpecializationConstant> specialization_constants;
        Vector<ShaderStage> stages_vector;
        uint32_t stages_bits = 0;
        uint32_t push_constant_stages = 0;
    };

    RenderingDeviceCommons() = delete;
    RenderingDeviceCommons(const RenderingDeviceCommons&) = delete;
    ~RenderingDeviceCommons() = delete;
};

using RDC = RenderingDeviceCommons;

// --- Folded from servers/rendering/rendering_device_enums.h ---
namespace RenderingDeviceEnums
{

// This enum matches VkPhysicalDeviceType (except for `DEVICE_TYPE_MAX`).
// Unlike VkPhysicalDeviceType, DeviceType is exposed to the scripting API.
enum DeviceType
{
	DEVICE_TYPE_OTHER,
	DEVICE_TYPE_INTEGRATED_GPU,
	DEVICE_TYPE_DISCRETE_GPU,
	DEVICE_TYPE_VIRTUAL_GPU,
	DEVICE_TYPE_CPU,
	DEVICE_TYPE_MAX
};

} // namespace RenderingDeviceEnums

// --- Folded from modules/glslang/register_types.h ---
// --- Folded from modules/glslang/shader_compile.h ---


Vector<uint8_t> compile_glslang_shader(RenderingDeviceCommons::ShaderStage p_stage, const String &p_source_code, RenderingDeviceCommons::ShaderLanguageVersion p_language_version, RenderingDeviceCommons::ShaderSpirvVersion p_spirv_version, String *r_error);
// --- End modules/glslang/register_types.h ---
// --- End servers/rendering/rendering_device_commons.h ---
// --- Folded from servers/rendering/rendering_device_driver.h ---
// ***********************************************************************************
// RenderingDeviceDriver - Design principles
// -----------------------------------------
// - Very little validation is done, and normally only in dev or debug builds.
// - Error reporting is generally simple: returning an id of 0 or a false boolean.
// - Certain enums/constants/structs follow Vulkan values/layout. That makes things easier for
// RDDVulkan (it asserts compatibility).
// - We allocate as little as possible in functions expected to be quick (a counterexample is
// loading/saving shaders) and use alloca() whenever suitable.
// - We try to back opaque ids with the native ones or memory addresses.
// - When using bookkeeping structures because the actual API id of a resource is not enough, we use
// a PagedAllocator.
// - Every struct has default initializers.
// - Using VectorView to take array-like arguments. Vector<uint8_t> is an exception (an indiom for
// "BLOB").
// - If a driver needs some higher-level information (the kind of info RenderingDevice keeps), it
// shall store a copy of what it needs.
//   There's no backwards communication from the driver to query data from RenderingDevice.
// ***********************************************************************************

#include "core/types.h"
// --- Folded from servers/rendering/rendering_context_driver.h ---
#include <cstdint>
#include "core/string/ustring.h"
#include "core/templates/hash_map.h"
#include "servers/display/display_server.h"

class RenderingDeviceDriver;

class RenderingContextDriver
{
public:
	typedef uint64_t SurfaceID;

private:
	HashMap<DisplayServerEnums::WindowID, SurfaceID> window_surface_map;

public:
	SurfaceID surface_get_from_window(DisplayServerEnums::WindowID p_window) const;
	Error window_create(DisplayServerEnums::WindowID p_window, const void* p_platform_data);
	void window_set_size(
		DisplayServerEnums::WindowID p_window, uint32_t p_width, uint32_t p_height);
	void window_set_vsync_mode(
		DisplayServerEnums::WindowID p_window, DisplayServerEnums::VSyncMode p_vsync_mode);
	DisplayServerEnums::VSyncMode window_get_vsync_mode(
		DisplayServerEnums::WindowID p_window) const;
	void window_set_hdr_output_enabled(DisplayServerEnums::WindowID p_window, bool p_enabled);
	bool window_get_hdr_output_enabled(DisplayServerEnums::WindowID p_window) const;
	void window_set_hdr_output_reference_luminance(
		DisplayServerEnums::WindowID p_window, float p_reference_luminance);
	float window_get_hdr_output_reference_luminance(DisplayServerEnums::WindowID p_window) const;
	void window_set_hdr_output_max_luminance(
		DisplayServerEnums::WindowID p_window, float p_max_luminance);
	float window_get_hdr_output_max_luminance(DisplayServerEnums::WindowID p_window) const;
	void window_set_hdr_output_linear_luminance_scale(
		DisplayServerEnums::WindowID p_window, float p_linear_luminance_scale);
	float window_get_hdr_output_linear_luminance_scale(DisplayServerEnums::WindowID p_window) const;
	float window_get_output_max_linear_value(DisplayServerEnums::WindowID p_window) const;
	void window_destroy(DisplayServerEnums::WindowID p_window);

public:
	// Not an enum as these values are matched against values returned by
	// the various drivers, which report them in uint32_t. Casting to an
	// enum value is dangerous in this case as we don't actually know what
	// range the driver is reporting a value in.
	struct Vendor
	{
		constexpr static uint32_t VENDOR_UNKNOWN = 0x0;
		constexpr static uint32_t VENDOR_AMD = 0x1002;
		constexpr static uint32_t VENDOR_IMGTEC = 0x1010;
		constexpr static uint32_t VENDOR_APPLE = 0x106B;
		constexpr static uint32_t VENDOR_NVIDIA = 0x10DE;
		constexpr static uint32_t VENDOR_ARM = 0x13B5;
		constexpr static uint32_t VENDOR_MICROSOFT = 0x1414;
		constexpr static uint32_t VENDOR_QUALCOMM = 0x5143;
		constexpr static uint32_t VENDOR_INTEL = 0x8086;
	};

	enum DeviceType
	{
		DEVICE_TYPE_OTHER = 0x0,
		DEVICE_TYPE_INTEGRATED_GPU = 0x1,
		DEVICE_TYPE_DISCRETE_GPU = 0x2,
		DEVICE_TYPE_VIRTUAL_GPU = 0x3,
		DEVICE_TYPE_CPU = 0x4,
		DEVICE_TYPE_MAX = 0x5
	};

	struct Device
	{
		String name = "Unknown";
		uint32_t vendor = Vendor::VENDOR_UNKNOWN;
		DeviceType type = DEVICE_TYPE_OTHER;
	};

	virtual ~RenderingContextDriver();
	virtual Error initialize() = 0;
	virtual const Device& device_get(uint32_t p_device_index) const = 0;
	virtual uint32_t device_get_count() const = 0;
	virtual bool device_supports_present(uint32_t p_device_index, SurfaceID p_surface) const = 0;
	virtual RenderingDeviceDriver* driver_create() = 0;
	virtual void driver_free(RenderingDeviceDriver* p_driver) = 0;
	virtual SurfaceID surface_create(const void* p_platform_data) = 0;
	virtual void surface_set_size(SurfaceID p_surface, uint32_t p_width, uint32_t p_height) = 0;
	virtual void surface_set_vsync_mode(
		SurfaceID p_surface, DisplayServerEnums::VSyncMode p_vsync_mode) = 0;
	virtual DisplayServerEnums::VSyncMode surface_get_vsync_mode(SurfaceID p_surface) const = 0;
	virtual void surface_set_hdr_output_enabled(SurfaceID p_surface, bool p_enabled) = 0;
	virtual bool surface_get_hdr_output_enabled(SurfaceID p_surface) const = 0;
	virtual void surface_set_hdr_output_reference_luminance(
		SurfaceID p_surface, float p_reference_luminance) = 0;
	virtual float surface_get_hdr_output_reference_luminance(SurfaceID p_surface) const = 0;
	virtual void surface_set_hdr_output_max_luminance(
		SurfaceID p_surface, float p_max_luminance) = 0;
	virtual float surface_get_hdr_output_max_luminance(SurfaceID p_surface) const = 0;
	virtual void surface_set_hdr_output_linear_luminance_scale(
		SurfaceID p_surface, float p_linear_luminance_scale) = 0;
	virtual float surface_get_hdr_output_linear_luminance_scale(SurfaceID p_surface) const = 0;
	virtual float surface_get_hdr_output_max_value(SurfaceID p_surface) const = 0;
	virtual uint32_t surface_get_width(SurfaceID p_surface) const = 0;
	virtual uint32_t surface_get_height(SurfaceID p_surface) const = 0;
	virtual void surface_set_needs_resize(SurfaceID p_surface, bool p_needs_resize) = 0;
	virtual bool surface_get_needs_resize(SurfaceID p_surface) const = 0;
	virtual void surface_destroy(SurfaceID p_surface) = 0;
	virtual bool is_debug_utils_enabled() const = 0;

	String get_driver_and_device_memory_report() const;

	virtual const char* get_tracked_object_name(uint32_t p_type_index) const;
	virtual uint64_t get_tracked_object_type_count() const;

	virtual uint64_t get_driver_total_memory() const;
	virtual uint64_t get_driver_allocation_count() const;
	virtual uint64_t get_driver_memory_by_object_type(uint32_t p_type) const;
	virtual uint64_t get_driver_allocs_by_object_type(uint32_t p_type) const;

	virtual uint64_t get_device_total_memory() const;
	virtual uint64_t get_device_allocation_count() const;
	virtual uint64_t get_device_memory_by_object_type(uint32_t p_type) const;
	virtual uint64_t get_device_allocs_by_object_type(uint32_t p_type) const;
};
// --- End servers/rendering/rendering_context_driver.h ---


class RenderingShaderContainer;
class RenderingShaderContainerFormat;

// These utilities help drivers avoid allocations.
#define ALLOCA(m_size) ((m_size != 0) ? alloca(m_size) : nullptr)
#define ALLOCA_ARRAY(m_type, m_count) ((m_type*)ALLOCA(sizeof(m_type) * (m_count)))
#define ALLOCA_SINGLE(m_type) ALLOCA_ARRAY(m_type, 1)

// This helps forwarding certain arrays to the API with confidence.
#define ARRAYS_COMPATIBLE(m_type_a, m_type_b)                                                      \
	(sizeof(m_type_a) == sizeof(m_type_b) && alignof(m_type_a) == alignof(m_type_b))
// This is used when you also need to ensure structured types are compatible field-by-field.
// TODO: The fieldwise check is unimplemented, but still this one is useful, as a strong annotation
// about the needs.
#define ARRAYS_COMPATIBLE_FIELDWISE(m_type_a, m_type_b) ARRAYS_COMPATIBLE(m_type_a, m_type_b)
// Another utility, to make it easy to compare members of different enums, which is not fine with
// some compilers.
#define ENUM_MEMBERS_EQUAL(m_a, m_b) ((int64_t)m_a == (int64_t)m_b)

// This helps using a single paged allocator for many resource types.
template <typename... RESOURCE_TYPES> struct VersatileResourceTemplate
{
	static constexpr size_t RESOURCE_SIZES[] = {sizeof(RESOURCE_TYPES)...};
	static constexpr size_t MAX_RESOURCE_SIZE = Span(RESOURCE_SIZES).max();
	uint8_t data[MAX_RESOURCE_SIZE];

	template <typename T>
	static T* allocate(PagedAllocator<VersatileResourceTemplate, true>& p_allocator)
	{
		T* obj = (T*)p_allocator.alloc();
		memnew_placement(obj, T);
		return obj;
	}

	template <typename T>
	static void free(PagedAllocator<VersatileResourceTemplate, true>& p_allocator, T* p_object)
	{
		p_object->~T();
		p_allocator.free((VersatileResourceTemplate*)p_object);
	}
};

class RenderingDeviceDriver
{
public:
	struct ID
	{
		uint64_t id = 0;
		_ALWAYS_INLINE_ ID() = default;

		_ALWAYS_INLINE_ ID(uint64_t p_id) : id(p_id) {}
	};

#define DEFINE_ID(m_name)                                                                          \
	struct m_name##ID : public ID                                                                  \
	{                                                                                              \
		_ALWAYS_INLINE_ explicit operator bool() const                                             \
		{                                                                                          \
			return id != 0;                                                                        \
		}                                                                                          \
		_ALWAYS_INLINE_ m_name##ID& operator=(m_name##ID p_other)                                  \
		{                                                                                          \
			id = p_other.id;                                                                       \
			return *this;                                                                          \
		}                                                                                          \
		_ALWAYS_INLINE_ bool operator<(const m_name##ID& p_other) const                            \
		{                                                                                          \
			return id < p_other.id;                                                                \
		}                                                                                          \
		_ALWAYS_INLINE_ bool operator==(const m_name##ID& p_other) const                           \
		{                                                                                          \
			return id == p_other.id;                                                               \
		}                                                                                          \
		_ALWAYS_INLINE_ bool operator!=(const m_name##ID& p_other) const                           \
		{                                                                                          \
			return id != p_other.id;                                                               \
		}                                                                                          \
		_ALWAYS_INLINE_ m_name##ID(const m_name##ID& p_other) : ID(p_other.id)                     \
		{                                                                                          \
		}                                                                                          \
		_ALWAYS_INLINE_ explicit m_name##ID(uint64_t p_int) : ID(p_int)                            \
		{                                                                                          \
		}                                                                                          \
		_ALWAYS_INLINE_ explicit m_name##ID(void* p_ptr) : ID((uint64_t)p_ptr)                     \
		{                                                                                          \
		}                                                                                          \
		_ALWAYS_INLINE_ m_name##ID() = default;                                                    \
	};

	// Id types declared before anything else to prevent cyclic dependencies between the different
	// concerns.
	DEFINE_ID(Buffer);
	DEFINE_ID(Texture);
	DEFINE_ID(Sampler);
	DEFINE_ID(VertexFormat);
	DEFINE_ID(CommandQueue);
	DEFINE_ID(CommandQueueFamily);
	DEFINE_ID(CommandPool);
	DEFINE_ID(CommandBuffer);
	DEFINE_ID(SwapChain);
	DEFINE_ID(Framebuffer);
	DEFINE_ID(Shader);
	DEFINE_ID(UniformSet);
	DEFINE_ID(Pipeline);
	DEFINE_ID(RenderPass);
	DEFINE_ID(QueryPool);
	DEFINE_ID(Fence);
	DEFINE_ID(Semaphore);
	DEFINE_ID(AccelerationStructure);
	DEFINE_ID(RaytracingPipeline);

public:
	virtual Error initialize(uint32_t p_device_index, uint32_t p_frame_count) = 0;

	enum MemoryAllocationType
	{
		MEMORY_ALLOCATION_TYPE_CPU, // For images, CPU allocation also means linear, GPU is tiling
									// optimal.
		MEMORY_ALLOCATION_TYPE_GPU,
	};

	enum BufferUsageBits
	{
		BUFFER_USAGE_TRANSFER_FROM_BIT = (1 << 0),
		BUFFER_USAGE_TRANSFER_TO_BIT = (1 << 1),
		BUFFER_USAGE_TEXEL_BIT = (1 << 2),
		BUFFER_USAGE_UNIFORM_BIT = (1 << 4),
		BUFFER_USAGE_STORAGE_BIT = (1 << 5),
		BUFFER_USAGE_INDEX_BIT = (1 << 6),
		BUFFER_USAGE_VERTEX_BIT = (1 << 7),
		BUFFER_USAGE_INDIRECT_BIT = (1 << 8),
		BUFFER_USAGE_SHADER_BINDING_TABLE_BIT = (1 << 10),
		BUFFER_USAGE_DEVICE_ADDRESS_BIT = (1 << 17),
		BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT = (1 << 19),
		BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT = (1 << 20),
		// There are no Vulkan-equivalent. Try to use unused/unclaimed bits.
		BUFFER_USAGE_DYNAMIC_PERSISTENT_BIT = (1 << 31),
	};

	enum
	{
		BUFFER_WHOLE_SIZE = ~0ULL
	};

	/** Allocates a new GPU buffer. Must be destroyed with buffer_free().
	 * @param p_size The size in bytes of the buffer.
	 * @param p_usage Usage flags.
	 * @param p_allocation_type See MemoryAllocationType.
	 * @param p_frames_drawn Used for debug checks when BUFFER_USAGE_DYNAMIC_PERSISTENT_BIT is set.
	 * @return the buffer.
	 */
	virtual BufferID buffer_create(uint64_t p_size, uint32_t p_usage,
		MemoryAllocationType p_allocation_type, uint64_t p_frames_drawn) = 0;
	// Only for a buffer with BUFFER_USAGE_TEXEL_BIT.
	virtual bool buffer_set_texel_format(BufferID p_buffer, RDC::DataFormat p_format) = 0;
	virtual void buffer_free(BufferID p_buffer) = 0;
	virtual uint64_t buffer_get_allocation_size(BufferID p_buffer) = 0;
	virtual uint8_t* buffer_map(BufferID p_buffer) = 0;
	virtual void buffer_unmap(BufferID p_buffer) = 0;
	virtual uint8_t* buffer_persistent_map_advance(BufferID p_buffer, uint64_t p_frames_drawn) = 0;
	virtual uint64_t buffer_get_dynamic_offsets(Span<BufferID> p_buffers) = 0;

	virtual void buffer_flush(BufferID p_buffer) {}

	// Only for a buffer with BUFFER_USAGE_DEVICE_ADDRESS_BIT.
	virtual uint64_t buffer_get_device_address(BufferID p_buffer) = 0;

	struct TextureView
	{
		RDC::DataFormat format = RDC::DATA_FORMAT_MAX;
		RDC::TextureSwizzle swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		RDC::TextureSwizzle swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		RDC::TextureSwizzle swizzle_b = RDC::TEXTURE_SWIZZLE_B;
		RDC::TextureSwizzle swizzle_a = RDC::TEXTURE_SWIZZLE_A;
	};

	enum TextureLayout
	{
		TEXTURE_LAYOUT_UNDEFINED,
		TEXTURE_LAYOUT_GENERAL,
		TEXTURE_LAYOUT_STORAGE_OPTIMAL,
		TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
		TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		TEXTURE_LAYOUT_COPY_SRC_OPTIMAL,
		TEXTURE_LAYOUT_COPY_DST_OPTIMAL,
		TEXTURE_LAYOUT_RESOLVE_SRC_OPTIMAL,
		TEXTURE_LAYOUT_RESOLVE_DST_OPTIMAL,
		TEXTURE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL,
		TEXTURE_LAYOUT_FRAGMENT_DENSITY_MAP_ATTACHMENT_OPTIMAL,
		TEXTURE_LAYOUT_MAX
	};

	enum TextureAspect
	{
		TEXTURE_ASPECT_COLOR = 0,
		TEXTURE_ASPECT_DEPTH = 1,
		TEXTURE_ASPECT_STENCIL = 2,
		TEXTURE_ASPECT_MAX
	};

	enum TextureUsageMethod
	{
		TEXTURE_USAGE_VRS_FRAGMENT_SHADING_RATE_BIT = RDC::TEXTURE_USAGE_MAX_BIT << 1,
		TEXTURE_USAGE_VRS_FRAGMENT_DENSITY_MAP_BIT = RDC::TEXTURE_USAGE_MAX_BIT << 2,
	};

	enum TextureAspectBits
	{
		TEXTURE_ASPECT_COLOR_BIT = (1 << TEXTURE_ASPECT_COLOR),
		TEXTURE_ASPECT_DEPTH_BIT = (1 << TEXTURE_ASPECT_DEPTH),
		TEXTURE_ASPECT_STENCIL_BIT = (1 << TEXTURE_ASPECT_STENCIL),
	};

	struct TextureSubresource
	{
		TextureAspect aspect = TEXTURE_ASPECT_COLOR;
		uint32_t layer = 0;
		uint32_t mipmap = 0;
	};

	struct TextureSubresourceLayers
	{
		uint32_t aspect = 0;
		uint32_t mipmap = 0;
		uint32_t base_layer = 0;
		uint32_t layer_count = 0;
	};

	struct TextureSubresourceRange
	{
		uint32_t aspect = 0;
		uint32_t base_mipmap = 0;
		uint32_t mipmap_count = 0;
		uint32_t base_layer = 0;
		uint32_t layer_count = 0;
	};

	struct TextureCopyableLayout
	{
		uint64_t size = 0;
		uint64_t row_pitch = 0;
	};

	virtual TextureID texture_create(const RDC::TextureFormat& p_format, const TextureView& p_view) = 0;
	virtual TextureID texture_create_from_extension(uint64_t p_native_texture, RDC::TextureType p_type,
		RDC::DataFormat p_format, uint32_t p_array_layers, bool p_depth_stencil, uint32_t p_mipmaps) = 0;
	// texture_create_shared_*() can only use original, non-view textures as original.
	// RenderingDevice is responsible for ensuring that.
	virtual TextureID texture_create_shared(
		TextureID p_original_texture, const TextureView& p_view) = 0;
	virtual TextureID texture_create_shared_from_slice(TextureID p_original_texture,
		const TextureView& p_view, RDC::TextureSliceType p_slice_type, uint32_t p_layer,
		uint32_t p_layers, uint32_t p_mipmap, uint32_t p_mipmaps) = 0;
	virtual void texture_free(TextureID p_texture) = 0;
	virtual uint64_t texture_get_allocation_size(TextureID p_texture) = 0;
	// Returns a texture layout for buffer <-> texture copies. If you are copying multiple texture
	// subresources to/from the same buffer, you are responsible for correctly aligning the start
	// offset for every buffer region. See API_TRAIT_TEXTURE_TRANSFER_ALIGNMENT.
	virtual void texture_get_copyable_layout(TextureID p_texture,
		const TextureSubresource& p_subresource, TextureCopyableLayout* r_layout) = 0;
	// Returns the data of a texture layer for a CPU texture that was created with
	// TEXTURE_USAGE_CPU_READ_BIT.
	virtual Vector<uint8_t> texture_get_data(TextureID p_texture, uint32_t p_layer) = 0;
	virtual uint32_t texture_get_usages_supported_by_format(
		RDC::DataFormat p_format, bool p_cpu_readable) = 0;
	virtual bool texture_can_make_shared_with_format(
		TextureID p_texture, RDC::DataFormat p_format, bool& r_raw_reinterpretation) = 0;

	virtual SamplerID sampler_create(const RDC::SamplerState& p_state) = 0;
	virtual void sampler_free(SamplerID p_sampler) = 0;
	virtual bool sampler_is_format_supported_for_filter(
		RDC::DataFormat p_format, RDC::SamplerFilter p_filter) = 0;

	virtual VertexFormatID vertex_format_create(Span<RDC::VertexAttribute> p_vertex_attribs,
		const RDC::VertexAttributeBindingsMap& p_vertex_bindings) = 0;
	virtual void vertex_format_free(VertexFormatID p_vertex_format) = 0;

	enum PipelineStageBits
	{
		PIPELINE_STAGE_TOP_OF_PIPE_BIT = (1 << 0),
		PIPELINE_STAGE_DRAW_INDIRECT_BIT = (1 << 1),
		PIPELINE_STAGE_VERTEX_INPUT_BIT = (1 << 2),
		PIPELINE_STAGE_VERTEX_SHADER_BIT = (1 << 3),
		PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT = (1 << 4),
		PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT = (1 << 5),
		PIPELINE_STAGE_GEOMETRY_SHADER_BIT = (1 << 6),
		PIPELINE_STAGE_FRAGMENT_SHADER_BIT = (1 << 7),
		PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT = (1 << 8),
		PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT = (1 << 9),
		PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT = (1 << 10),
		PIPELINE_STAGE_COMPUTE_SHADER_BIT = (1 << 11),
		PIPELINE_STAGE_COPY_BIT = (1 << 12),
		PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT = (1 << 13),
		PIPELINE_STAGE_RESOLVE_BIT = (1 << 14),
		PIPELINE_STAGE_ALL_GRAPHICS_BIT = (1 << 15),
		PIPELINE_STAGE_ALL_COMMANDS_BIT = (1 << 16),
		PIPELINE_STAGE_CLEAR_STORAGE_BIT = (1 << 17),
		PIPELINE_STAGE_RAY_TRACING_SHADER_BIT = (1 << 21),
		PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT = (1 << 22),
		PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT = (1 << 23),
		PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT = (1 << 25),
	};

	enum BarrierAccessBits
	{
		BARRIER_ACCESS_INDIRECT_COMMAND_READ_BIT = (1 << 0),
		BARRIER_ACCESS_INDEX_READ_BIT = (1 << 1),
		BARRIER_ACCESS_VERTEX_ATTRIBUTE_READ_BIT = (1 << 2),
		BARRIER_ACCESS_UNIFORM_READ_BIT = (1 << 3),
		BARRIER_ACCESS_INPUT_ATTACHMENT_READ_BIT = (1 << 4),
		BARRIER_ACCESS_SHADER_READ_BIT = (1 << 5),
		BARRIER_ACCESS_SHADER_WRITE_BIT = (1 << 6),
		BARRIER_ACCESS_COLOR_ATTACHMENT_READ_BIT = (1 << 7),
		BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT = (1 << 8),
		BARRIER_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT = (1 << 9),
		BARRIER_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT = (1 << 10),
		BARRIER_ACCESS_COPY_READ_BIT = (1 << 11),
		BARRIER_ACCESS_COPY_WRITE_BIT = (1 << 12),
		BARRIER_ACCESS_HOST_READ_BIT = (1 << 13),
		BARRIER_ACCESS_HOST_WRITE_BIT = (1 << 14),
		BARRIER_ACCESS_MEMORY_READ_BIT = (1 << 15),
		BARRIER_ACCESS_MEMORY_WRITE_BIT = (1 << 16),
		BARRIER_ACCESS_ACCELERATION_STRUCTURE_READ_BIT = (1 << 21),
		BARRIER_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT = (1 << 22),
		BARRIER_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT = (1 << 23),
		BARRIER_ACCESS_FRAGMENT_DENSITY_MAP_ATTACHMENT_READ_BIT = (1 << 24),
		BARRIER_ACCESS_RESOLVE_READ_BIT = (1 << 25),
		BARRIER_ACCESS_RESOLVE_WRITE_BIT = (1 << 26),
		BARRIER_ACCESS_STORAGE_CLEAR_BIT = (1 << 27),
	};

	// https://github.com/godotengine/godot/pull/110360 - "MemoryBarrier" conflicts with Windows
	// header defines
	struct MemoryAccessBarrier
	{
		uint32_t src_access = 0;
		uint32_t dst_access = 0;
	};

	struct BufferBarrier
	{
		BufferID buffer;
		uint32_t src_access = 0;
		uint32_t dst_access = 0;
		uint64_t offset = 0;
		uint64_t size = 0;
	};

	struct TextureBarrier
	{
		TextureID texture;
		uint32_t src_access = 0;
		uint32_t dst_access = 0;
		TextureLayout prev_layout = TEXTURE_LAYOUT_UNDEFINED;
		TextureLayout next_layout = TEXTURE_LAYOUT_UNDEFINED;
		TextureSubresourceRange subresources;
	};

	struct AccelerationStructureBarrier
	{
		AccelerationStructureID acceleration_structure;
		uint32_t src_access;
		uint32_t dst_access;
		uint64_t offset = 0;
		uint64_t size = 0;
	};

	virtual void command_pipeline_barrier(CommandBufferID p_cmd_buffer, uint32_t p_src_stages,
		uint32_t p_dst_stages, VectorView<MemoryAccessBarrier> p_memory_barriers,
		VectorView<BufferBarrier> p_buffer_barriers, VectorView<TextureBarrier> p_texture_barriers,
		VectorView<AccelerationStructureBarrier> p_acceleration_structure_barriers)
	{
	}

	virtual FenceID fence_create() = 0;
	virtual Error fence_wait(FenceID p_fence) = 0;
	virtual void fence_free(FenceID p_fence) = 0;

	virtual SemaphoreID semaphore_create() = 0;
	virtual void semaphore_free(SemaphoreID p_semaphore) = 0;

	enum CommandQueueFamilyBits
	{
		COMMAND_QUEUE_FAMILY_GRAPHICS_BIT = 0x1,
		COMMAND_QUEUE_FAMILY_COMPUTE_BIT = 0x2,
		COMMAND_QUEUE_FAMILY_TRANSFER_BIT = 0x4
	};

	// The requested command queue family must support all specified bits or it'll fail to return a
	// valid family otherwise. If a valid surface is specified, the queue must support presenting to
	// it. It is valid to specify no bits and a valid surface: in this case, the dedicated
	// presentation queue family will be the preferred option.
	virtual CommandQueueFamilyID command_queue_family_get(
		uint32_t p_cmd_queue_family_bits, RenderingContextDriver::SurfaceID p_surface = 0) = 0;

	virtual CommandQueueID command_queue_create(
		CommandQueueFamilyID p_cmd_queue_family, bool p_identify_as_main_queue = false) = 0;
	virtual Error command_queue_execute_and_present(CommandQueueID p_cmd_queue,
		VectorView<SemaphoreID> p_wait_semaphores, VectorView<CommandBufferID> p_cmd_buffers,
		VectorView<SemaphoreID> p_cmd_semaphores, FenceID p_cmd_fence,
		VectorView<SwapChainID> p_swap_chains) = 0;
	virtual void command_queue_free(CommandQueueID p_cmd_queue) = 0;

	enum CommandBufferType
	{
		COMMAND_BUFFER_TYPE_PRIMARY,
		COMMAND_BUFFER_TYPE_SECONDARY,
	};

	virtual CommandPoolID command_pool_create(
		CommandQueueFamilyID p_cmd_queue_family, CommandBufferType p_cmd_buffer_type) = 0;
	virtual bool command_pool_reset(CommandPoolID p_cmd_pool) = 0;
	virtual void command_pool_free(CommandPoolID p_cmd_pool) = 0;

	virtual CommandBufferID command_buffer_create(CommandPoolID p_cmd_pool) = 0;
	virtual bool command_buffer_begin(CommandBufferID p_cmd_buffer) = 0;
	virtual bool command_buffer_begin_secondary(CommandBufferID p_cmd_buffer,
		RenderPassID p_render_pass, uint32_t p_subpass, FramebufferID p_framebuffer) = 0;
	virtual void command_buffer_end(CommandBufferID p_cmd_buffer) = 0;
	virtual void command_buffer_execute_secondary(
		CommandBufferID p_cmd_buffer, VectorView<CommandBufferID> p_secondary_cmd_buffers) = 0;

	// The swap chain won't be valid for use until it is resized at least once.
	virtual SwapChainID swap_chain_create(RenderingContextDriver::SurfaceID p_surface) = 0;

	// The swap chain must not be in use when a resize is requested. Wait until all rendering
	// associated to the swap chain is finished before resizing it.
	virtual Error swap_chain_resize(CommandQueueID p_cmd_queue, SwapChainID p_swap_chain,
		uint32_t p_desired_framebuffer_count) = 0;

	// Acquire the framebuffer that can be used for drawing. This must be called only once every
	// time a new frame will be rendered.
	virtual FramebufferID swap_chain_acquire_framebuffer(
		CommandQueueID p_cmd_queue, SwapChainID p_swap_chain, bool& r_resize_required) = 0;

	// Retrieve the render pass that can be used to draw on the swap chain's framebuffers.
	virtual RenderPassID swap_chain_get_render_pass(SwapChainID p_swap_chain) = 0;

	// Retrieve the rotation in degrees to apply as a pre-transform. Usually 0 on PC. May be 0, 90,
	// 180 & 270 on Android.
	virtual int swap_chain_get_pre_rotation_degrees(SwapChainID p_swap_chain) { return 0; }

	// Retrieve the format used by the swap chain's framebuffers.
	virtual RDC::DataFormat swap_chain_get_format(SwapChainID p_swap_chain) = 0;

	// Retrieve the color space used by the swap chain's framebuffers.
	virtual RDC::ColorSpace swap_chain_get_color_space(SwapChainID p_swap_chain) = 0;

	// Retrieve whether the swapchain supports our preferred HDR formats.
	virtual bool swap_chain_get_hdr_output_supported(SwapChainID p_swap_chain) = 0;

	// Tells the swapchain the max_fps so it can use the proper frame pacing.
	// Android uses this with Swappy library. Some implementations or platforms may ignore this
	// hint.
	virtual void swap_chain_set_max_fps(SwapChainID p_swap_chain, int p_max_fps) {}

	// Wait until all rendering associated to the swap chain is finished before deleting it.
	virtual void swap_chain_free(SwapChainID p_swap_chain) = 0;

	virtual FramebufferID framebuffer_create(RenderPassID p_render_pass,
		VectorView<TextureID> p_attachments, uint32_t p_width, uint32_t p_height) = 0;
	virtual void framebuffer_free(FramebufferID p_framebuffer) = 0;

	struct ImmutableSampler
	{
		RDC::UniformType type = RDC::UNIFORM_TYPE_MAX;
		uint32_t binding = 0xffffffff; // Binding index as specified in shader.
		LocalVector<ID> ids;
	};

	// Creates a Pipeline State Object (PSO) out of the shader and all the input data it needs.
	// Immutable samplers can be embedded when creating the pipeline layout on the condition they
	// remain valid and unchanged, so they don't need to be specified when creating uniform sets PSO
	// resource for binding.
	virtual ShaderID shader_create_from_container(
		const Ref<RenderingShaderContainer>& p_shader_container,
		const Vector<ImmutableSampler>& p_immutable_samplers) = 0;

	// Only meaningful if API_TRAIT_SHADER_CHANGE_INVALIDATION is
	// SHADER_CHANGE_INVALIDATION_ALL_OR_NONE_ACCORDING_TO_LAYOUT_HASH.
	virtual uint32_t shader_get_layout_hash(ShaderID p_shader) { return 0; }

	virtual void shader_free(ShaderID p_shader) = 0;
	virtual void shader_destroy_modules(ShaderID p_shader) = 0;

public:
	struct BoundUniform
	{
		RDC::UniformType type = RDC::UNIFORM_TYPE_MAX;
		uint32_t binding = 0xffffffff; // Binding index as specified in shader.
		LocalVector<ID> ids;
		// Flag to indicate  that this is an immutable sampler so it is skipped when creating
		// uniform sets, as it would be set previously when creating the pipeline layout.
		bool immutable_sampler = false;

		_FORCE_INLINE_ bool is_dynamic() const
		{
			return type == RDC::UNIFORM_TYPE_STORAGE_BUFFER_DYNAMIC ||
				   type == RDC::UNIFORM_TYPE_UNIFORM_BUFFER_DYNAMIC;
		}
	};

	virtual UniformSetID uniform_set_create(VectorView<BoundUniform> p_uniforms, ShaderID p_shader,
		uint32_t p_set_index, int p_linear_pool_index) = 0;

	virtual void linear_uniform_set_pools_reset(int p_linear_pool_index) {}

	virtual void uniform_set_free(UniformSetID p_uniform_set) = 0;

	virtual bool uniform_sets_have_linear_pools() const { return false; }

	virtual uint32_t uniform_sets_get_dynamic_offsets(VectorView<UniformSetID> p_uniform_sets,
		ShaderID p_shader, uint32_t p_first_set_index, uint32_t p_set_count) const = 0;

	virtual void command_uniform_set_prepare_for_use(CommandBufferID p_cmd_buffer,
		UniformSetID p_uniform_set, ShaderID p_shader, uint32_t p_set_index) = 0;

	struct BufferCopyRegion
	{
		uint64_t src_offset = 0;
		uint64_t dst_offset = 0;
		uint64_t size = 0;
	};

	struct TextureCopyRegion
	{
		TextureSubresourceLayers src_subresources;
		Vector3i src_offset;
		TextureSubresourceLayers dst_subresources;
		Vector3i dst_offset;
		Vector3i size;
	};

	struct BufferTextureCopyRegion
	{
		uint64_t buffer_offset = 0;
		uint64_t row_pitch = 0;
		TextureSubresource texture_subresource;
		Vector3i texture_offset;
		Vector3i texture_region_size;
	};

	virtual void command_clear_buffer(
		CommandBufferID p_cmd_buffer, BufferID p_buffer, uint64_t p_offset, uint64_t p_size) = 0;
	virtual void command_copy_buffer(CommandBufferID p_cmd_buffer, BufferID p_src_buffer,
		BufferID p_dst_buffer, VectorView<BufferCopyRegion> p_regions) = 0;

	virtual void command_copy_texture(CommandBufferID p_cmd_buffer, TextureID p_src_texture,
		TextureLayout p_src_texture_layout, TextureID p_dst_texture,
		TextureLayout p_dst_texture_layout, VectorView<TextureCopyRegion> p_regions) = 0;
	virtual void command_resolve_texture(CommandBufferID p_cmd_buffer, TextureID p_src_texture,
		TextureLayout p_src_texture_layout, uint32_t p_src_layer, uint32_t p_src_mipmap,
		TextureID p_dst_texture, TextureLayout p_dst_texture_layout, uint32_t p_dst_layer,
		uint32_t p_dst_mipmap) = 0;
	virtual void command_clear_color_texture(CommandBufferID p_cmd_buffer, TextureID p_texture,
		TextureLayout p_texture_layout, const Color& p_color,
		const TextureSubresourceRange& p_subresources) = 0;
	virtual void command_clear_depth_stencil_texture(CommandBufferID p_cmd_buffer,
		TextureID p_texture, TextureLayout p_texture_layout, float p_depth, uint8_t p_stencil,
		const TextureSubresourceRange& p_subresources) = 0;

	virtual void command_copy_buffer_to_texture(CommandBufferID p_cmd_buffer, BufferID p_src_buffer,
		TextureID p_dst_texture, TextureLayout p_dst_texture_layout,
		VectorView<BufferTextureCopyRegion> p_regions) = 0;
	virtual void command_copy_texture_to_buffer(CommandBufferID p_cmd_buffer,
		TextureID p_src_texture, TextureLayout p_src_texture_layout, BufferID p_dst_buffer,
		VectorView<BufferTextureCopyRegion> p_regions) = 0;

	virtual void pipeline_free(PipelineID p_pipeline) = 0;

	virtual void command_bind_push_constants(CommandBufferID p_cmd_buffer, ShaderID p_shader,
		uint32_t p_first_index, VectorView<uint32_t> p_data) = 0;

	virtual bool pipeline_cache_create(const Vector<uint8_t>& p_data) = 0;
	virtual void pipeline_cache_free() = 0;
	virtual size_t pipeline_cache_query_size() = 0;
	virtual Vector<uint8_t> pipeline_cache_serialize() = 0;

	enum AttachmentLoadOp
	{
		ATTACHMENT_LOAD_OP_LOAD = 0,
		ATTACHMENT_LOAD_OP_CLEAR = 1,
		ATTACHMENT_LOAD_OP_DONT_CARE = 2,
	};

	enum AttachmentStoreOp
	{
		ATTACHMENT_STORE_OP_STORE = 0,
		ATTACHMENT_STORE_OP_DONT_CARE = 1,
	};

	struct Attachment
	{
		RDC::DataFormat format = RDC::DATA_FORMAT_MAX;
		RDC::TextureSamples samples = RDC::TEXTURE_SAMPLES_MAX;
		AttachmentLoadOp load_op = ATTACHMENT_LOAD_OP_DONT_CARE;
		AttachmentStoreOp store_op = ATTACHMENT_STORE_OP_DONT_CARE;
		AttachmentLoadOp stencil_load_op = ATTACHMENT_LOAD_OP_DONT_CARE;
		AttachmentStoreOp stencil_store_op = ATTACHMENT_STORE_OP_DONT_CARE;
		TextureLayout initial_layout = TEXTURE_LAYOUT_UNDEFINED;
		TextureLayout final_layout = TEXTURE_LAYOUT_UNDEFINED;
	};

	struct AttachmentReference
	{
		static constexpr uint32_t UNUSED = 0xffffffff;
		uint32_t attachment = UNUSED;
		TextureLayout layout = TEXTURE_LAYOUT_UNDEFINED;
		uint32_t aspect = 0;
	};

	struct Subpass
	{
		LocalVector<AttachmentReference> input_references;
		LocalVector<AttachmentReference> color_references;
		AttachmentReference depth_stencil_reference;
		AttachmentReference depth_resolve_reference;
		LocalVector<AttachmentReference> resolve_references;
		LocalVector<uint32_t> preserve_attachments;
		AttachmentReference fragment_shading_rate_reference;
		Size2i fragment_shading_rate_texel_size;
	};

	struct SubpassDependency
	{
		uint32_t src_subpass = 0xffffffff;
		uint32_t dst_subpass = 0xffffffff;
		uint32_t src_stages = 0;
		uint32_t dst_stages = 0;
		uint32_t src_access = 0;
		uint32_t dst_access = 0;
	};

	virtual RenderPassID render_pass_create(VectorView<Attachment> p_attachments,
		VectorView<Subpass> p_subpasses, VectorView<SubpassDependency> p_subpass_dependencies,
		uint32_t p_view_count, AttachmentReference p_fragment_density_map_attachment) = 0;
	virtual void render_pass_free(RenderPassID p_render_pass) = 0;

	union RenderPassClearValue
	{
		Color color = {};

		struct
		{
			float depth;
			uint32_t stencil;
		};

		RenderPassClearValue() {}
	};

	struct AttachmentClear
	{
		uint32_t aspect = 0;
		uint32_t color_attachment = 0xffffffff;
		RenderPassClearValue value;
	};

	virtual void command_begin_render_pass(CommandBufferID p_cmd_buffer, RenderPassID p_render_pass,
		FramebufferID p_framebuffer, CommandBufferType p_cmd_buffer_type, const Rect2i& p_rect,
		VectorView<RenderPassClearValue> p_clear_values) = 0;
	virtual void command_end_render_pass(CommandBufferID p_cmd_buffer) = 0;
	virtual void command_next_render_subpass(
		CommandBufferID p_cmd_buffer, CommandBufferType p_cmd_buffer_type) = 0;
	virtual void command_render_set_viewport(
		CommandBufferID p_cmd_buffer, VectorView<Rect2i> p_viewports) = 0;
	virtual void command_render_set_scissor(
		CommandBufferID p_cmd_buffer, VectorView<Rect2i> p_scissors) = 0;
	virtual void command_render_clear_attachments(CommandBufferID p_cmd_buffer,
		VectorView<AttachmentClear> p_attachment_clears, VectorView<Rect2i> p_rects) = 0;

	// Binding.
	virtual void command_bind_render_pipeline(
		CommandBufferID p_cmd_buffer, PipelineID p_pipeline) = 0;
	virtual void command_bind_render_uniform_sets(CommandBufferID p_cmd_buffer,
		VectorView<UniformSetID> p_uniform_sets, ShaderID p_shader, uint32_t p_first_set_index,
		uint32_t p_set_count, uint32_t p_dynamic_offsets) = 0;

	// Drawing.
	virtual void command_render_draw(CommandBufferID p_cmd_buffer, uint32_t p_vertex_count,
		uint32_t p_instance_count, uint32_t p_base_vertex, uint32_t p_first_instance) = 0;
	virtual void command_render_draw_indexed(CommandBufferID p_cmd_buffer, uint32_t p_index_count,
		uint32_t p_instance_count, uint32_t p_first_index, int32_t p_vertex_offset,
		uint32_t p_first_instance) = 0;
	virtual void command_render_draw_indexed_indirect(CommandBufferID p_cmd_buffer,
		BufferID p_indirect_buffer, uint64_t p_offset, uint32_t p_draw_count,
		uint32_t p_stride) = 0;
	virtual void command_render_draw_indexed_indirect_count(CommandBufferID p_cmd_buffer,
		BufferID p_indirect_buffer, uint64_t p_offset, BufferID p_count_buffer,
		uint64_t p_count_buffer_offset, uint32_t p_max_draw_count, uint32_t p_stride) = 0;
	virtual void command_render_draw_indirect(CommandBufferID p_cmd_buffer,
		BufferID p_indirect_buffer, uint64_t p_offset, uint32_t p_draw_count,
		uint32_t p_stride) = 0;
	virtual void command_render_draw_indirect_count(CommandBufferID p_cmd_buffer,
		BufferID p_indirect_buffer, uint64_t p_offset, BufferID p_count_buffer,
		uint64_t p_count_buffer_offset, uint32_t p_max_draw_count, uint32_t p_stride) = 0;

	// Buffer binding.
	virtual void command_render_bind_vertex_buffers(CommandBufferID p_cmd_buffer,
		uint32_t p_binding_count, const BufferID* p_buffers, const uint64_t* p_offsets,
		uint64_t p_dynamic_offsets) = 0;
	virtual void command_render_bind_index_buffer(CommandBufferID p_cmd_buffer, BufferID p_buffer,
		RDC::IndexBufferFormat p_format, uint64_t p_offset) = 0;

	// Dynamic state.
	virtual void command_render_set_blend_constants(
		CommandBufferID p_cmd_buffer, const Color& p_constants) = 0;
	virtual void command_render_set_line_width(CommandBufferID p_cmd_buffer, float p_width) = 0;

	virtual PipelineID render_pipeline_create(ShaderID p_shader, VertexFormatID p_vertex_format,
		RDC::RenderPrimitive p_render_primitive, RDC::PipelineRasterizationState p_rasterization_state,
		RDC::PipelineMultisampleState p_multisample_state,
		RDC::PipelineDepthStencilState p_depth_stencil_state, RDC::PipelineColorBlendState p_blend_state,
		VectorView<int32_t> p_color_attachments, uint32_t p_dynamic_state,
		RenderPassID p_render_pass, uint32_t p_render_subpass,
		VectorView<RDC::PipelineSpecializationConstant> p_specialization_constants) = 0;

	// Binding.
	virtual void command_bind_compute_pipeline(
		CommandBufferID p_cmd_buffer, PipelineID p_pipeline) = 0;
	virtual void command_bind_compute_uniform_sets(CommandBufferID p_cmd_buffer,
		VectorView<UniformSetID> p_uniform_sets, ShaderID p_shader, uint32_t p_first_set_index,
		uint32_t p_set_count, uint32_t p_dynamic_offsets) = 0;

	// Dispatching.
	virtual void command_compute_dispatch(CommandBufferID p_cmd_buffer, uint32_t p_x_groups,
		uint32_t p_y_groups, uint32_t p_z_groups) = 0;
	virtual void command_compute_dispatch_indirect(
		CommandBufferID p_cmd_buffer, BufferID p_indirect_buffer, uint64_t p_offset) = 0;

	virtual PipelineID compute_pipeline_create(ShaderID p_shader,
		VectorView<RDC::PipelineSpecializationConstant> p_specialization_constants) = 0;

	struct AccelerationStructureGeometry
	{
		uint32_t flags = 0;
		BufferID vertex_buffer;
		uint32_t vertex_offset = 0;
		uint32_t vertex_stride = 0;
		uint32_t vertex_count = 0;
		RDC::DataFormat vertex_format = RDC::DATA_FORMAT_MAX;
		BufferID index_buffer;
		uint32_t index_offset = 0;
		uint32_t index_count = 0;
		RDC::IndexBufferFormat index_format = {};
	};

	virtual AccelerationStructureID blas_create(
		VectorView<AccelerationStructureGeometry> p_geometries, uint32_t p_flags) = 0;

	struct AccelerationStructureInstance
	{
		Transform3D transform;
		uint32_t id = 0;
		uint8_t mask = 0;
		uint32_t hit_sbt_offset = 0;
		uint32_t flags = 0;
		AccelerationStructureID blas;
	};

	virtual AccelerationStructureID tlas_create(
		uint32_t p_max_instance_count, uint32_t p_flags) = 0;
	virtual void acceleration_structure_instance_write(
		uint8_t* r_driver_instance, const AccelerationStructureInstance& p_instance) = 0;

	virtual void acceleration_structure_free(AccelerationStructureID p_acceleration_structure) = 0;
	virtual uint32_t acceleration_structure_get_scratch_size_bytes(
		AccelerationStructureID p_acceleration_structure) = 0;

	struct PipelineShader
	{
		ShaderID shader;
		VectorView<RDC::PipelineSpecializationConstant> specialization_constants;
		RDC::ShaderStage shader_stage = {};
	};

	struct HitGroup
	{
		uint32_t closest_hit_shader_index = UINT32_MAX;
		uint32_t any_hit_shader_index = UINT32_MAX;
		uint32_t intersection_shader_index = UINT32_MAX;
	};

	virtual RaytracingPipelineID raytracing_pipeline_create(VectorView<PipelineShader> p_shaders,
		VectorView<uint32_t> p_raygen_shader_indices, VectorView<uint32_t> p_miss_shader_indices,
		VectorView<HitGroup> p_hit_groups, uint32_t p_max_trace_recursion_depth,
		ShaderID p_layout_defining_shader) = 0;
	virtual void raytracing_pipeline_free(RaytracingPipelineID p_pipeline) = 0;

	virtual bool raytracing_pipeline_get_shader_group_handles(RaytracingPipelineID p_pipeline,
		uint32_t p_group_index_offset, VectorView<uint32_t> p_group_indices, uint8_t* r_data,
		uint32_t p_data_stride_bytes) = 0;

	virtual void command_build_blas(CommandBufferID p_cmd_buffer,
		AccelerationStructureID p_acceleration_structure, BufferID p_scratch_buffer) = 0;
	virtual void command_build_tlas(CommandBufferID p_cmd_buffer,
		AccelerationStructureID p_acceleration_structure, BufferID p_scratch_buffer,
		BufferID p_instance_buffer, uint32_t p_instance_offset, uint32_t p_instance_count) = 0;
	virtual void command_bind_raytracing_pipeline(
		CommandBufferID p_cmd_buffer, RaytracingPipelineID p_pipeline) = 0;
	virtual void command_bind_raytracing_uniform_set(CommandBufferID p_cmd_buffer,
		UniformSetID p_uniform_set, ShaderID p_shader, uint32_t p_set_index) = 0;

	struct ShaderBindingTable
	{
		BufferID buffer;
		uint32_t offset = 0;
		uint32_t stride = 0;
		uint32_t size = 0;
	};

	virtual void command_trace_rays(CommandBufferID p_cmd_buffer,
		const ShaderBindingTable& p_raygen_sbt, const ShaderBindingTable& p_miss_sbt,
		const ShaderBindingTable& p_hit_sbt, uint32_t p_width, uint32_t p_height,
		uint32_t p_depth) = 0;

	typedef void (*DriverCallback)(
		RenderingDeviceDriver* p_driver, CommandBufferID p_command_buffer, void* p_userdata);

	// Basic.
	virtual QueryPoolID timestamp_query_pool_create(uint32_t p_query_count) = 0;
	virtual void timestamp_query_pool_free(QueryPoolID p_pool_id) = 0;
	virtual void timestamp_query_pool_get_results(
		QueryPoolID p_pool_id, uint32_t p_query_count, uint64_t* r_results) = 0;
	virtual uint64_t timestamp_query_result_to_time(uint64_t p_result) = 0;

	// Commands.
	virtual void command_timestamp_query_pool_reset(
		CommandBufferID p_cmd_buffer, QueryPoolID p_pool_id, uint32_t p_query_count) = 0;
	virtual void command_timestamp_write(
		CommandBufferID p_cmd_buffer, QueryPoolID p_pool_id, uint32_t p_index) = 0;

	virtual void command_begin_label(
		CommandBufferID p_cmd_buffer, const char* p_label_name, const Color& p_color) = 0;
	virtual void command_end_label(CommandBufferID p_cmd_buffer) = 0;

	virtual void command_insert_breadcrumb(CommandBufferID p_cmd_buffer, uint32_t p_data) = 0;

	virtual void begin_segment(uint32_t p_frame_index, uint32_t p_frames_drawn) = 0;
	virtual void end_segment() = 0;

	enum ObjectType
	{
		OBJECT_TYPE_TEXTURE,
		OBJECT_TYPE_SAMPLER,
		OBJECT_TYPE_BUFFER,
		OBJECT_TYPE_SHADER,
		OBJECT_TYPE_UNIFORM_SET,
		OBJECT_TYPE_PIPELINE,
		OBJECT_TYPE_ACCELERATION_STRUCTURE,
		OBJECT_TYPE_RAYTRACING_PIPELINE,
	};

	struct MultiviewCapabilities
	{
		bool is_supported = false;
		bool geometry_shader_is_supported = false;
		bool tessellation_shader_is_supported = false;
		uint32_t max_view_count = 0;
		uint32_t max_instance_count = 0;
	};

	struct FragmentShadingRateCapabilities
	{
		Size2i min_texel_size;
		Size2i max_texel_size;
		Size2i max_fragment_size;
		bool pipeline_supported = false;
		bool primitive_supported = false;
		bool attachment_supported = false;
	};

	struct FragmentDensityMapCapabilities
	{
		Size2i min_texel_size;
		Size2i max_texel_size;
		Size2i offset_granularity;
		bool attachment_supported = false;
		bool dynamic_attachment_supported = false;
		bool non_subsampled_images_supported = false;
		bool invocations_supported = false;
		bool offset_supported = false;
	};

	enum ApiTrait
	{
		API_TRAIT_HONORS_PIPELINE_BARRIERS,
		API_TRAIT_SHADER_CHANGE_INVALIDATION,
		API_TRAIT_TEXTURE_TRANSFER_ALIGNMENT,
		API_TRAIT_TEXTURE_DATA_ROW_PITCH_STEP,
		API_TRAIT_SECONDARY_VIEWPORT_SCISSOR,
		API_TRAIT_CLEARS_WITH_COPY_ENGINE,
		API_TRAIT_USE_GENERAL_IN_COPY_QUEUES,
		API_TRAIT_BUFFERS_REQUIRE_TRANSITIONS,
		API_TRAIT_TEXTURE_OUTPUTS_REQUIRE_CLEARS,
		API_TRAIT_ACCELERATION_STRUCTURE_INSTANCE_SIZE,
		API_TRAIT_SHADER_GROUP_HANDLE_SIZE,
		API_TRAIT_SHADER_GROUP_BASE_ALIGNMENT,
		API_TRAIT_SHADER_GROUP_HANDLE_ALIGNMENT,
	};

	enum ShaderChangeInvalidation
	{
		SHADER_CHANGE_INVALIDATION_ALL_BOUND_UNIFORM_SETS,
		// What Vulkan does.
		SHADER_CHANGE_INVALIDATION_INCOMPATIBLE_SETS_PLUS_CASCADE,
		// What D3D12 does.
		SHADER_CHANGE_INVALIDATION_ALL_OR_NONE_ACCORDING_TO_LAYOUT_HASH,
	};

	enum DeviceFamily
	{
		DEVICE_UNKNOWN,
		DEVICE_OPENGL,
		DEVICE_VULKAN,
		DEVICE_DIRECTX,
		DEVICE_METAL,
	};

	struct Capabilities
	{
		DeviceFamily device_family = DEVICE_UNKNOWN;
		uint32_t version_major = 1;
		uint32_t version_minor = 0;
	};

	virtual void set_object_name(ObjectType p_type, ID p_driver_id, const String& p_name) = 0;
	virtual uint64_t get_resource_native_handle(RDC::DriverResource p_type, ID p_driver_id) = 0;
	virtual uint64_t get_total_memory_used() = 0;
	virtual uint64_t get_lazily_memory_used() = 0;
	virtual uint64_t limit_get(RDC::Limit p_limit) = 0;
	virtual uint64_t api_trait_get(ApiTrait p_trait);
	virtual bool has_feature(RDC::Features p_feature) = 0;
	virtual const MultiviewCapabilities& get_multiview_capabilities() = 0;
	virtual const FragmentShadingRateCapabilities& get_fragment_shading_rate_capabilities() = 0;
	virtual const FragmentDensityMapCapabilities& get_fragment_density_map_capabilities() = 0;
	virtual String get_api_name() const = 0;
	virtual String get_api_version() const = 0;
	virtual String get_pipeline_cache_uuid() const = 0;
	virtual const Capabilities& get_capabilities() const = 0;
	virtual const RenderingShaderContainerFormat& get_shader_container_format() const = 0;

	virtual bool is_composite_alpha_supported(CommandQueueID p_queue) const { return false; }

	virtual RDC::DriverWorkarounds get_driver_workarounds() const { return RDC::DriverWorkarounds(); }

	virtual ~RenderingDeviceDriver();
};

using RDD = RenderingDeviceDriver;

// --- Folded from servers/rendering/rendering_shader_container.h ---
#include "core/types.h"


struct SpvReflectShaderModule;
struct SpvReflectDescriptorBinding;
struct SpvReflectSpecializationConstant;

class RenderingShaderContainer : public RefCounted
{
public:
	static const uint32_t CONTAINER_MAGIC_NUMBER = 0x43535247;
	static const uint32_t CONTAINER_VERSION = 2;

protected:
	using RDC = RenderingDeviceCommons;

	struct ContainerHeader
	{
		uint32_t magic_number = 0;
		uint32_t version = 0;
		uint32_t format = 0;
		uint32_t format_version = 0;
		uint32_t shader_count = 0;
	};

	struct ReflectionData
	{
		uint64_t vertex_input_mask = 0;
		uint32_t fragment_output_mask = 0;
		uint32_t specialization_constants_count = 0;
		RDC::PipelineType pipeline_type = RDC::PIPELINE_TYPE_RASTERIZATION;
		uint32_t has_multiview = 0;
		uint32_t has_dynamic_buffers = 0;
		uint32_t compute_local_size[3] = {};
		uint32_t set_count = 0;
		uint32_t push_constant_size = 0;
		uint32_t push_constant_stages_mask = 0;
		uint32_t stage_count = 0;
		uint32_t shader_name_len = 0;
	};

	struct ReflectionBindingData
	{
		uint32_t type = 0;
		uint32_t binding = 0;
		uint32_t stages = 0;
		uint32_t length =
			0; // Size of arrays (in total elements), or UBOs (in bytes * total elements).
		uint32_t writable = 0;
		RDC::TextureType texture_type = RDC::TEXTURE_TYPE_MAX;
		RDC::DataFormat texture_format = RDC::DATA_FORMAT_MAX;

		bool operator<(const ReflectionBindingData& p_other) const
		{
			return binding < p_other.binding;
		}
	};

	struct ReflectionSpecializationData
	{
		uint32_t type = 0;
		uint32_t constant_id = 0;
		uint32_t int_value = 0;
		uint32_t stage_flags = 0;
	};

	struct ShaderHeader
	{
		uint32_t shader_stage = 0;
		uint32_t code_compressed_size = 0;
		uint32_t code_compression_flags = 0;
		uint32_t code_decompressed_size = 0;
	};

	ReflectionData reflection_data;
	Vector<uint32_t> reflection_binding_set_uniforms_count;
	Vector<ReflectionBindingData> reflection_binding_set_uniforms_data;
	Vector<ReflectionSpecializationData> reflection_specialization_data;
	Vector<RDC::ShaderStage> reflection_shader_stages;

	virtual uint32_t _format() const = 0;
	virtual uint32_t _format_version() const = 0;

	// These methods will always be called with a valid pointer.
	virtual uint32_t _from_bytes_header_extra_data(const uint8_t* p_bytes);
	virtual uint32_t _from_bytes_reflection_extra_data(const uint8_t* p_bytes);
	virtual uint32_t _from_bytes_reflection_binding_uniform_extra_data_start(
		const uint8_t* p_bytes);
	virtual uint32_t _from_bytes_reflection_binding_uniform_extra_data(
		const uint8_t* p_bytes, uint32_t p_index);
	virtual uint32_t _from_bytes_reflection_specialization_extra_data_start(const uint8_t* p_bytes);
	virtual uint32_t _from_bytes_reflection_specialization_extra_data(
		const uint8_t* p_bytes, uint32_t p_index);
	virtual uint32_t _from_bytes_shader_extra_data_start(const uint8_t* p_bytes);
	virtual uint32_t _from_bytes_shader_extra_data(const uint8_t* p_bytes, uint32_t p_index);
	virtual uint32_t _from_bytes_footer_extra_data(const uint8_t* p_bytes);

	// These methods will be called with a nullptr to retrieve the size of the data.
	virtual uint32_t _to_bytes_header_extra_data(uint8_t* p_bytes) const;
	virtual uint32_t _to_bytes_reflection_extra_data(uint8_t* p_bytes) const;
	virtual uint32_t _to_bytes_reflection_binding_uniform_extra_data(
		uint8_t* p_bytes, uint32_t p_index) const;
	virtual uint32_t _to_bytes_reflection_specialization_extra_data(
		uint8_t* p_bytes, uint32_t p_index) const;
	virtual uint32_t _to_bytes_shader_extra_data(uint8_t* p_bytes, uint32_t p_index) const;
	virtual uint32_t _to_bytes_footer_extra_data(uint8_t* p_bytes) const;

	template <class T> struct ReflectSymbol
	{
		static constexpr uint32_t STAGE_INDEX[RDC::SHADER_STAGE_MAX] = {
			0, // SHADER_STAGE_VERTEX
			1, // SHADER_STAGE_FRAGMENT
			0, // SHADER_STAGE_TESSELATION_CONTROL
			1, // SHADER_STAGE_TESSELATION_EVALUATION
			0, // SHADER_STAGE_COMPUTE
		};

		uint32_t stages = {};

	private:
		const T* _spv_reflect[2] = {nullptr};

	public:
		_FORCE_INLINE_ constexpr uint32_t get_index_for_stage(RDC::ShaderStage p_stage) const
		{
			DEV_ASSERT(stages.has_flag((1 << p_stage)));
			return STAGE_INDEX[p_stage];
		}

		const T& get_spv_reflect(RDC::ShaderStage p_stage) const;

		/*! Returns the first valid stage if multiple stages are set.
		 *
		 * Crashes if no stages are set.
		 */
		const T& get_spv_reflect() const
		{
			for (const T* d : _spv_reflect) {
				if (d != nullptr) {
					return *d;
				}
			}
			CRASH_NOW_MSG("No stages set in ReflectSymbol");
		}

		void set_spv_reflect(RDC::ShaderStage p_stage, const T* p_spv);
	};

	struct ReflectUniform : ReflectSymbol<SpvReflectDescriptorBinding>
	{
		RDC::UniformType type = RDC::UniformType::UNIFORM_TYPE_MAX;
		uint32_t binding = 0;
		uint32_t length =
			0; // Size of arrays (in total elements), or ubos (in bytes * total elements).
		bool writable = false;
		RDC::TextureType texture_type = RDC::TEXTURE_TYPE_MAX;
		RDC::DataFormat texture_format = RDC::DATA_FORMAT_MAX;

		bool operator<(const ReflectUniform& p_other) const
		{
			if (binding != p_other.binding) {
				return binding < p_other.binding;
			}
			if (type != p_other.type) {
				return type < p_other.type;
			}
			if (writable != p_other.writable) {
				return writable < p_other.writable;
			}
			if (stages != p_other.stages) {
				return stages < p_other.stages;
			}
			if (length != p_other.length) {
				return length < p_other.length;
			}
			return false;
		}
	};

	struct ReflectSpecializationConstant : ReflectSymbol<SpvReflectSpecializationConstant>
	{
		RDC::PipelineSpecializationConstantType type = {};
		uint32_t constant_id = 0xffffffff;

		union
		{
			uint32_t int_value = 0;
			float float_value;
			bool bool_value;
		};

		bool operator<(const ReflectSpecializationConstant& p_other) const
		{
			return constant_id < p_other.constant_id;
		}
	};

	class ReflectShaderStage
	{
		friend class RenderingShaderContainer;

		Vector<uint8_t> _spirv_data;
		SpvReflectShaderModule* _module = nullptr;

	public:
		RDC::ShaderStage shader_stage = RDC::SHADER_STAGE_MAX;
		const SpvReflectShaderModule& module() const;
		const Span<uint32_t> spirv() const;

		const Vector<uint8_t> spirv_data() const { return _spirv_data; }

		ReflectShaderStage();
		~ReflectShaderStage();
	};

	typedef LocalVector<ReflectUniform> ReflectDescriptorSet;

	struct ReflectShader
	{
		uint64_t vertex_input_mask = 0;
		uint32_t fragment_output_mask = 0;
		uint32_t compute_local_size[3] = {};
		uint32_t push_constant_size = 0;
		bool has_multiview = false;
		bool has_dynamic_buffers = false;
		RDC::PipelineType pipeline_type = RDC::PIPELINE_TYPE_RASTERIZATION;

		LocalVector<ReflectShaderStage> shader_stages;
		LocalVector<ReflectDescriptorSet> uniform_sets;
		LocalVector<ReflectSymbol<SpvReflectDescriptorBinding>> reflect_uniforms;
		LocalVector<ReflectSpecializationConstant> specialization_constants;
		LocalVector<ReflectSymbol<SpvReflectSpecializationConstant>>
			reflect_specialization_constants;
		LocalVector<RDC::ShaderStage> stages_vector;
		uint32_t stages_bits = {};
		uint32_t push_constant_stages = {};

		_FORCE_INLINE_ bool is_compute() const
		{
			return (stages_bits & (RDC::SHADER_STAGE_COMPUTE_BIT)) != 0;
		}

		/*! Returns the uniform at the specified global index.
		 *
		 * This is a flattened view of all uniform sets.
		 */
		ReflectUniform& uniform_at(uint32_t p_index)
		{
			for (LocalVector<ReflectUniform>& set : uniform_sets) {
				if (p_index < set.size()) {
					return set[p_index];
				}
				p_index -= set.size();
			}
			CRASH_NOW_MSG(
				vformat("Uniform index %d out of range (total %d)", p_index, uniform_count()));
		}

		uint32_t uniform_count() const
		{
			uint32_t count = 0;
			for (const LocalVector<ReflectUniform>& set : uniform_sets) {
				count += set.size();
			}
			return count;
		}
	};

	// This method will be called when set_from_shader_reflection() is finished. Used to update
	// internal structures to match the reflection if necessary.
	virtual void _set_from_shader_reflection_post(const ReflectShader& p_shader);

	// This method will be called when set_code_from_spirv() is called.
	virtual bool _set_code_from_spirv(const ReflectShader& p_shader) = 0;

	void set_from_shader_reflection(const ReflectShader& p_reflection);
	Error reflect_spirv(const String& p_shader_name, Span<RDC::ShaderStageSPIRVData> p_spirv,
		ReflectShader& r_shader);

public:
	enum CompressionFlags
	{
		COMPRESSION_FLAG_ZSTD = 0x1,
	};

	struct Shader
	{
		RDC::ShaderStage shader_stage = RDC::SHADER_STAGE_MAX;
		PackedByteArray code_compressed_bytes;
		uint32_t code_compression_flags = 0;
		uint32_t code_decompressed_size = 0;
	};

	CharString shader_name;
	Vector<Shader> shaders;

	bool set_code_from_spirv(const String& p_shader_name, Span<RDC::ShaderStageSPIRVData> p_spirv);
	RDC::ShaderReflection get_shader_reflection() const;
	bool from_bytes(const PackedByteArray& p_bytes);
	PackedByteArray to_bytes() const;
	bool compress_code(const uint8_t* p_decompressed_bytes, uint32_t p_decompressed_size,
		uint8_t* p_compressed_bytes, uint32_t* r_compressed_size,
		uint32_t* r_compressed_flags) const;
	bool decompress_code(const uint8_t* p_compressed_bytes, uint32_t p_compressed_size,
		uint32_t p_compressed_flags, uint8_t* p_decompressed_bytes,
		uint32_t p_decompressed_size) const;
	RenderingShaderContainer();
	virtual ~RenderingShaderContainer();
};

class RenderingShaderContainerFormat
{
public:
	virtual Ref<RenderingShaderContainer> create_container() const = 0;
	virtual RDC::ShaderLanguageVersion get_shader_language_version() const = 0;
	virtual RDC::ShaderSpirvVersion get_shader_spirv_version() const = 0;
};
// --- End servers/rendering/rendering_device_driver.h ---

// --- Folded from servers/rendering/rendering_device_graph.h ---




#ifdef DEV_ENABLED
#include "core/templates/hash_map.h"
#endif

// Buffer barriers have not shown any significant improvement or shown to be
// even detrimental to performance. However, there are currently some known
// cases where using them can solve problems that using singular memory
// barriers does not, probably due to driver issues (see comment on PR #84976
// https://github.com/godotengine/godot/pull/84976#issuecomment-1878566830).

#define USE_BUFFER_BARRIERS 1

class RenderingDeviceGraph
{
public:
	struct RaytracingListInstruction
	{
		enum Type
		{
			TYPE_NONE,
			TYPE_BIND_PIPELINE,
			TYPE_BIND_UNIFORM_SET,
			TYPE_SET_PUSH_CONSTANT,
			TYPE_TRACE_RAYS,
			TYPE_UNIFORM_SET_PREPARE_FOR_USE,
		};

		Type type = TYPE_NONE;
	};

	struct ComputeListInstruction
	{
		enum Type
		{
			TYPE_NONE,
			TYPE_BIND_PIPELINE,
			TYPE_BIND_UNIFORM_SETS,
			TYPE_DISPATCH,
			TYPE_DISPATCH_INDIRECT,
			TYPE_SET_PUSH_CONSTANT,
			TYPE_UNIFORM_SET_PREPARE_FOR_USE
		};

		Type type = TYPE_NONE;
	};

	struct DrawListInstruction
	{
		enum Type
		{
			TYPE_NONE,
			TYPE_BIND_INDEX_BUFFER,
			TYPE_BIND_PIPELINE,
			TYPE_BIND_UNIFORM_SETS,
			TYPE_BIND_VERTEX_BUFFERS,
			TYPE_CLEAR_ATTACHMENTS,
			TYPE_DRAW,
			TYPE_DRAW_INDEXED,
			TYPE_DRAW_INDIRECT,
			TYPE_DRAW_INDEXED_INDIRECT,
			TYPE_EXECUTE_COMMANDS,
			TYPE_NEXT_SUBPASS,
			TYPE_SET_BLEND_CONSTANTS,
			TYPE_SET_LINE_WIDTH,
			TYPE_SET_PUSH_CONSTANT,
			TYPE_SET_SCISSOR,
			TYPE_SET_VIEWPORT,
			TYPE_UNIFORM_SET_PREPARE_FOR_USE
		};

		Type type = TYPE_NONE;
	};

	struct RecordedCommand
	{
		enum Type
		{
			TYPE_NONE,
			TYPE_BOTTOM_LEVEL_ACCELERATION_STRUCTURE_BUILD,
			TYPE_TOP_LEVEL_ACCELERATION_STRUCTURE_BUILD,
			TYPE_BUFFER_CLEAR,
			TYPE_BUFFER_COPY,
			TYPE_BUFFER_GET_DATA,
			TYPE_BUFFER_UPDATE,
			TYPE_COMPUTE_LIST,
			TYPE_RAYTRACING_LIST,
			TYPE_DRAW_LIST,
			TYPE_TEXTURE_CLEAR_COLOR,
			TYPE_TEXTURE_CLEAR_DEPTH_STENCIL,
			TYPE_TEXTURE_COPY,
			TYPE_TEXTURE_GET_DATA,
			TYPE_TEXTURE_RESOLVE,
			TYPE_TEXTURE_UPDATE,
			TYPE_CAPTURE_TIMESTAMP,
			TYPE_DRIVER_CALLBACK,
			TYPE_MAX
		};

		Type type = TYPE_NONE;
		int32_t adjacent_command_list_index = -1;
		RDD::MemoryAccessBarrier memory_barrier;
		int32_t normalization_barrier_index = -1;
		int normalization_barrier_count = 0;
		int32_t transition_barrier_index = -1;
		int32_t transition_barrier_count = 0;
#if USE_BUFFER_BARRIERS
		int32_t buffer_barrier_index = -1;
		int32_t buffer_barrier_count = 0;
#endif
		int32_t acceleration_structure_barrier_index = -1;
		int32_t acceleration_structure_barrier_count = 0;
		int32_t label_index = -1;
		uint32_t previous_stages = 0;
		uint32_t next_stages = 0;
		uint32_t self_stages = 0;
	};

	struct RecordedBufferCopy
	{
		RDD::BufferID source;
		RDD::BufferCopyRegion region;
	};

	struct RecordedBufferToTextureCopy
	{
		RDD::BufferID from_buffer;
		RDD::BufferTextureCopyRegion region;
	};

	enum ResourceUsage
	{
		RESOURCE_USAGE_NONE,
		RESOURCE_USAGE_COPY_FROM,
		RESOURCE_USAGE_COPY_TO,
		RESOURCE_USAGE_RESOLVE_FROM,
		RESOURCE_USAGE_RESOLVE_TO,
		RESOURCE_USAGE_UNIFORM_BUFFER_READ,
		RESOURCE_USAGE_INDIRECT_BUFFER_READ,
		RESOURCE_USAGE_TEXTURE_BUFFER_READ,
		RESOURCE_USAGE_TEXTURE_BUFFER_READ_WRITE,
		RESOURCE_USAGE_STORAGE_BUFFER_READ,
		RESOURCE_USAGE_STORAGE_BUFFER_READ_WRITE,
		RESOURCE_USAGE_VERTEX_BUFFER_READ,
		RESOURCE_USAGE_INDEX_BUFFER_READ,
		RESOURCE_USAGE_TEXTURE_SAMPLE,
		RESOURCE_USAGE_STORAGE_IMAGE_READ,
		RESOURCE_USAGE_STORAGE_IMAGE_READ_WRITE,
		RESOURCE_USAGE_ATTACHMENT_COLOR_READ_WRITE,
		RESOURCE_USAGE_ATTACHMENT_DEPTH_STENCIL_READ_WRITE,
		RESOURCE_USAGE_ATTACHMENT_FRAGMENT_SHADING_RATE_READ,
		RESOURCE_USAGE_ATTACHMENT_FRAGMENT_DENSITY_MAP_READ,
		RESOURCE_USAGE_GENERAL,
		RESOURCE_USAGE_ACCELERATION_STRUCTURE_READ,
		RESOURCE_USAGE_ACCELERATION_STRUCTURE_READ_WRITE,
		RESOURCE_USAGE_MAX
	};

	struct ResourceTracker
	{
		uint32_t reference_count = 0;
		int64_t command_frame = -1;
		int32_t command_index = -1;
		uint32_t usage_index = UINT32_MAX;
		uint32_t previous_frame_stages = 0;
		uint32_t current_frame_stages = 0;
		int32_t read_full_command_list_index = -1;
		int32_t read_slice_command_list_index = -1;
		int32_t write_command_or_list_index = -1;
		int32_t draw_list_index = -1;
		ResourceUsage draw_list_usage = RESOURCE_USAGE_NONE;
		int32_t compute_list_index = -1;
		int32_t raytracing_list_index = -1;
		ResourceUsage compute_list_usage = RESOURCE_USAGE_NONE;
		ResourceUsage raytracing_list_usage = RESOURCE_USAGE_NONE;
		ResourceUsage usage = RESOURCE_USAGE_NONE;
		uint32_t usage_access = 0;
		RDD::BufferID buffer_driver_id;
		RDD::TextureID texture_driver_id;
		RDD::AccelerationStructureID acceleration_structure_driver_id;
		RDD::TextureSubresourceRange texture_subresources;
		Size2i texture_size;
		uint32_t texture_usage = 0;
		int32_t texture_slice_command_index = -1;
		ResourceTracker* parent = nullptr;
		ResourceTracker* dirty_shared_list = nullptr;
		ResourceTracker* next_shared = nullptr;
		Rect2i texture_slice_or_dirty_rect;
		bool in_parent_dirty_list = false;
		bool write_command_list_enabled = false;
		bool is_discardable = false;

		_FORCE_INLINE_ void reset_if_outdated(int64_t new_command_frame)
		{
			if (new_command_frame != command_frame) {
				command_frame = new_command_frame;
				command_index = -1;
				usage_index = UINT32_MAX;
				previous_frame_stages = current_frame_stages;
				current_frame_stages = 0;
				read_full_command_list_index = -1;
				read_slice_command_list_index = -1;
				write_command_or_list_index = -1;
				draw_list_index = -1;
				compute_list_index = -1;
				raytracing_list_index = -1;
				texture_slice_command_index = -1;
				write_command_list_enabled = false;
			}
		}
	};

	typedef RDD::RenderPassID (*RenderPassCreationFunction)(RenderingDeviceDriver* p_driver,
		VectorView<RDD::AttachmentLoadOp> p_load_ops,
		VectorView<RDD::AttachmentStoreOp> p_store_ops, void* p_user_data);

	struct FramebufferStorage
	{
		RDD::FramebufferID framebuffer;
		RDD::RenderPassID render_pass;
	};

	struct FramebufferCache
	{
		uint32_t width = 0;
		uint32_t height = 0;
		LocalVector<RDD::TextureID> textures;
		LocalVector<ResourceTracker*> trackers;
		HashMap<uint64_t, FramebufferStorage> storage_map;
		void* render_pass_creation_user_data = nullptr;
	};

	struct CommandBufferPool
	{
		// Provided by RenderingDevice.
		RDD::CommandPoolID pool;

		// Created internally by RenderingDeviceGraph.
		LocalVector<RDD::CommandBufferID> buffers;
		LocalVector<RDD::SemaphoreID> semaphores;
		uint32_t buffers_used = 0;
	};

	struct WorkaroundsState
	{
		bool draw_list_found = false;
	};

	enum AttachmentOperation
	{
		// Loads or ignores if the attachment is discardable.
		ATTACHMENT_OPERATION_DEFAULT,
		// Clear the attachment to a value.
		ATTACHMENT_OPERATION_CLEAR,
		// Ignore any contents from the attachment.
		ATTACHMENT_OPERATION_IGNORE,
	};

private:
	struct InstructionList
	{
		LocalVector<uint8_t> data;
		LocalVector<ResourceTracker*> command_trackers;
		LocalVector<ResourceUsage> command_tracker_usages;
		uint32_t stages = 0;
		int32_t index = 0;

		void clear()
		{
			data.clear();
			command_trackers.clear();
			command_tracker_usages.clear();
			stages = 0;
		}
	};

	struct ComputeInstructionList
	{
		InstructionList list;
#if defined(DEBUG_ENABLED) || defined(DEV_ENABLED)
		uint32_t breadcrumb;
#endif
	};

	struct RaytracingInstructionList
	{
		InstructionList list;
	};

	struct DrawInstructionList
	{
		InstructionList list;
		FramebufferCache* framebuffer_cache = nullptr;
		RDD::RenderPassID render_pass;
		RDD::FramebufferID framebuffer;
		Rect2i region;
		LocalVector<AttachmentOperation> attachment_operations;
		LocalVector<RDD::RenderPassClearValue> attachment_clear_values;

#if defined(DEBUG_ENABLED) || defined(DEV_ENABLED)
		uint32_t breadcrumb;
#endif
		bool split_cmd_buffer = false;
	};

	struct RecordedCommandSort
	{
		uint32_t level = 0;
		uint32_t priority = 0;
		int32_t index = -1;

		RecordedCommandSort() = default;

		bool operator<(const RecordedCommandSort& p_other) const
		{
			if (level < p_other.level) {
				return true;
			}
			else if (level > p_other.level) {
				return false;
			}

			if (priority < p_other.priority) {
				return true;
			}
			else if (priority > p_other.priority) {
				return false;
			}

			return index < p_other.index;
		}
	};

	struct RecordedCommandListNode
	{
		int32_t command_index = -1;
		int32_t next_list_index = -1;
	};

	struct RecordedSliceListNode
	{
		int32_t command_index = -1;
		int32_t next_list_index = -1;
		Rect2i subresources;
		bool partial_coverage = false;
	};

	struct RecordedBottomLevelAccelerationStructureBuildCommand
	{
		RecordedCommand header;
		RDD::AccelerationStructureID acceleration_structure;
		RDD::BufferID scratch_buffer;
	};

	struct RecordedTopLevelAccelerationStructureBuildCommand
	{
		RecordedCommand header;
		RDD::AccelerationStructureID acceleration_structure;
		RDD::BufferID scratch_buffer;
		RDD::BufferID instance_buffer;
		uint32_t instance_offset = 0;
		uint32_t instance_count = 0;
	};

	struct RecordedBufferClearCommand
	{
		RecordedCommand header;
		RDD::BufferID buffer;
		uint32_t offset = 0;
		uint32_t size = 0;
	};

	struct RecordedBufferCopyCommand
	{
		RecordedCommand header;
		RDD::BufferID source;
		RDD::BufferID destination;
		RDD::BufferCopyRegion region;
	};

	struct RecordedBufferGetDataCommand
	{
		RecordedCommand header;
		RDD::BufferID source;
		RDD::BufferID destination;
		RDD::BufferCopyRegion region;
	};

	struct RecordedBufferUpdateCommand
	{
		RecordedCommand header;
		RDD::BufferID destination;
		uint32_t buffer_copies_count = 0;

		_FORCE_INLINE_ RecordedBufferCopy* buffer_copies()
		{
			return reinterpret_cast<RecordedBufferCopy*>(&this[1]);
		}

		_FORCE_INLINE_ const RecordedBufferCopy* buffer_copies() const
		{
			return reinterpret_cast<const RecordedBufferCopy*>(&this[1]);
		}
	};

	struct RecordedDriverCallbackCommand
	{
		RecordedCommand header;
		RDD::DriverCallback callback;
		void* userdata = nullptr;
	};

	struct RecordedRaytracingListCommand
	{
		RecordedCommand header;
		uint32_t instruction_data_size = 0;

		_FORCE_INLINE_ uint8_t* instruction_data() { return reinterpret_cast<uint8_t*>(&this[1]); }

		_FORCE_INLINE_ const uint8_t* instruction_data() const
		{
			return reinterpret_cast<const uint8_t*>(&this[1]);
		}
	};

	struct RecordedComputeListCommand
	{
		RecordedCommand header;
		uint32_t instruction_data_size = 0;
		uint32_t breadcrumb = 0;

		_FORCE_INLINE_ uint8_t* instruction_data() { return reinterpret_cast<uint8_t*>(&this[1]); }

		_FORCE_INLINE_ const uint8_t* instruction_data() const
		{
			return reinterpret_cast<const uint8_t*>(&this[1]);
		}
	};

	struct RecordedDrawListCommand
	{
		RecordedCommand header;
		FramebufferCache* framebuffer_cache = nullptr;
		RDD::FramebufferID framebuffer;
		RDD::RenderPassID render_pass;
		uint32_t instruction_data_size = 0;
		RDD::CommandBufferType command_buffer_type;
		Rect2i region;
		uint32_t clear_values_count = 0;
		uint32_t trackers_count = 0;

#if defined(DEBUG_ENABLED) || defined(DEV_ENABLED)
		uint32_t breadcrumb = 0;
#endif
		bool split_cmd_buffer = false;

		_FORCE_INLINE_ RDD::RenderPassClearValue* clear_values()
		{
			return reinterpret_cast<RDD::RenderPassClearValue*>(&this[1]);
		}

		_FORCE_INLINE_ const RDD::RenderPassClearValue* clear_values() const
		{
			return reinterpret_cast<const RDD::RenderPassClearValue*>(&this[1]);
		}

		_FORCE_INLINE_ ResourceTracker** trackers()
		{
			return reinterpret_cast<ResourceTracker**>(&clear_values()[clear_values_count]);
		}

		_FORCE_INLINE_ ResourceTracker* const* trackers() const
		{
			return reinterpret_cast<ResourceTracker* const*>(&clear_values()[clear_values_count]);
		}

		_FORCE_INLINE_ RDD::AttachmentLoadOp* load_ops()
		{
			return reinterpret_cast<RDD::AttachmentLoadOp*>(&trackers()[trackers_count]);
		}

		_FORCE_INLINE_ const RDD::AttachmentLoadOp* load_ops() const
		{
			return reinterpret_cast<const RDD::AttachmentLoadOp*>(&trackers()[trackers_count]);
		}

		_FORCE_INLINE_ RDD::AttachmentStoreOp* store_ops()
		{
			return reinterpret_cast<RDD::AttachmentStoreOp*>(&load_ops()[trackers_count]);
		}

		_FORCE_INLINE_ const RDD::AttachmentStoreOp* store_ops() const
		{
			return reinterpret_cast<const RDD::AttachmentStoreOp*>(&load_ops()[trackers_count]);
		}

		_FORCE_INLINE_ uint8_t* instruction_data()
		{
			return reinterpret_cast<uint8_t*>(&store_ops()[trackers_count]);
		}

		_FORCE_INLINE_ const uint8_t* instruction_data() const
		{
			return reinterpret_cast<const uint8_t*>(&store_ops()[trackers_count]);
		}
	};

	struct RecordedTextureClearColorCommand
	{
		RecordedCommand header;
		RDD::TextureID texture;
		RDD::TextureSubresourceRange range;
		Color color;
	};

	struct RecordedTextureClearDepthStencilCommand
	{
		RecordedCommand header;
		RDD::TextureID texture;
		RDD::TextureSubresourceRange range;
		float depth;
		uint8_t stencil;
	};

	struct RecordedTextureCopyCommand
	{
		RecordedCommand header;
		RDD::TextureID from_texture;
		RDD::TextureID to_texture;
		uint32_t texture_copy_regions_count = 0;

		_FORCE_INLINE_ RDD::TextureCopyRegion* texture_copy_regions()
		{
			return reinterpret_cast<RDD::TextureCopyRegion*>(&this[1]);
		}

		_FORCE_INLINE_ const RDD::TextureCopyRegion* texture_copy_regions() const
		{
			return reinterpret_cast<const RDD::TextureCopyRegion*>(&this[1]);
		}
	};

	struct RecordedTextureGetDataCommand
	{
		RecordedCommand header;
		RDD::TextureID from_texture;
		RDD::BufferID to_buffer;
		uint32_t buffer_texture_copy_regions_count = 0;

		_FORCE_INLINE_ RDD::BufferTextureCopyRegion* buffer_texture_copy_regions()
		{
			return reinterpret_cast<RDD::BufferTextureCopyRegion*>(&this[1]);
		}

		_FORCE_INLINE_ const RDD::BufferTextureCopyRegion* buffer_texture_copy_regions() const
		{
			return reinterpret_cast<const RDD::BufferTextureCopyRegion*>(&this[1]);
		}
	};

	struct RecordedTextureResolveCommand
	{
		RecordedCommand header;
		RDD::TextureID from_texture;
		RDD::TextureID to_texture;
		uint32_t src_layer = 0;
		uint32_t src_mipmap = 0;
		uint32_t dst_layer = 0;
		uint32_t dst_mipmap = 0;
	};

	struct RecordedTextureUpdateCommand
	{
		RecordedCommand header;
		RDD::TextureID to_texture;
		uint32_t buffer_to_texture_copies_count = 0;

		_FORCE_INLINE_ RecordedBufferToTextureCopy* buffer_to_texture_copies()
		{
			return reinterpret_cast<RecordedBufferToTextureCopy*>(&this[1]);
		}

		_FORCE_INLINE_ const RecordedBufferToTextureCopy* buffer_to_texture_copies() const
		{
			return reinterpret_cast<const RecordedBufferToTextureCopy*>(&this[1]);
		}
	};

	struct RecordedCaptureTimestampCommand
	{
		RecordedCommand header;
		RDD::QueryPoolID pool;
		uint32_t index = 0;
	};

	struct DrawListBindIndexBufferInstruction
	{
		DrawListInstruction header;
		RDD::BufferID buffer;
		RenderingDeviceCommons::IndexBufferFormat format;
		uint32_t offset = 0;
	};

	struct DrawListBindPipelineInstruction
	{
		DrawListInstruction header;
		RDD::PipelineID pipeline;
	};

	struct DrawListBindUniformSetsInstruction
	{
		DrawListInstruction header;
		RDD::ShaderID shader;
		uint32_t first_set_index = 0;
		uint32_t set_count = 0;
		uint32_t dynamic_offsets_mask = 0u;

		_FORCE_INLINE_ RDD::UniformSetID* uniform_set_ids()
		{
			return reinterpret_cast<RDD::UniformSetID*>(&this[1]);
		}

		_FORCE_INLINE_ const RDD::UniformSetID* uniform_set_ids() const
		{
			return reinterpret_cast<const RDD::UniformSetID*>(&this[1]);
		}
	};

	struct DrawListBindVertexBuffersInstruction
	{
		DrawListInstruction header;
		uint32_t vertex_buffers_count = 0;
		uint64_t dynamic_offsets_mask = 0;

		_FORCE_INLINE_ RDD::BufferID* vertex_buffers()
		{
			return reinterpret_cast<RDD::BufferID*>(&this[1]);
		}

		_FORCE_INLINE_ const RDD::BufferID* vertex_buffers() const
		{
			return reinterpret_cast<const RDD::BufferID*>(&this[1]);
		}

		_FORCE_INLINE_ uint64_t* vertex_buffer_offsets()
		{
			return reinterpret_cast<uint64_t*>(&vertex_buffers()[vertex_buffers_count]);
		}

		_FORCE_INLINE_ const uint64_t* vertex_buffer_offsets() const
		{
			return reinterpret_cast<const uint64_t*>(&vertex_buffers()[vertex_buffers_count]);
		}
	};

	struct DrawListClearAttachmentsInstruction
	{
		DrawListInstruction header;
		uint32_t attachments_clear_count = 0;
		uint32_t attachments_clear_rect_count = 0;

		_FORCE_INLINE_ RDD::AttachmentClear* attachments_clear()
		{
			return reinterpret_cast<RDD::AttachmentClear*>(&this[1]);
		}

		_FORCE_INLINE_ const RDD::AttachmentClear* attachments_clear() const
		{
			return reinterpret_cast<const RDD::AttachmentClear*>(&this[1]);
		}

		_FORCE_INLINE_ Rect2i* attachments_clear_rect()
		{
			return reinterpret_cast<Rect2i*>(&attachments_clear()[attachments_clear_count]);
		}

		_FORCE_INLINE_ const Rect2i* attachments_clear_rect() const
		{
			return reinterpret_cast<const Rect2i*>(&attachments_clear()[attachments_clear_count]);
		}
	};

	struct DrawListDrawInstruction
	{
		DrawListInstruction header;
		uint32_t vertex_count = 0;
		uint32_t instance_count = 0;
	};

	struct DrawListDrawIndexedInstruction
	{
		DrawListInstruction header;
		uint32_t index_count = 0;
		uint32_t instance_count = 0;
		uint32_t first_index = 0;
	};

	struct DrawListDrawIndirectInstruction
	{
		DrawListInstruction header;
		RDD::BufferID buffer;
		uint32_t offset = 0;
		uint32_t draw_count = 0;
		uint32_t stride = 0;
	};

	struct DrawListDrawIndexedIndirectInstruction
	{
		DrawListInstruction header;
		RDD::BufferID buffer;
		uint32_t offset = 0;
		uint32_t draw_count = 0;
		uint32_t stride = 0;
	};

	struct DrawListEndRenderPassInstruction
	{
		DrawListInstruction header;
	};

	struct DrawListExecuteCommandsInstruction
	{
		DrawListInstruction header;
		RDD::CommandBufferID command_buffer;
	};

	struct DrawListSetPushConstantInstruction
	{
		DrawListInstruction header;
		uint32_t size = 0;
		RDD::ShaderID shader;

		_FORCE_INLINE_ uint8_t* data() { return reinterpret_cast<uint8_t*>(&this[1]); }

		_FORCE_INLINE_ const uint8_t* data() const
		{
			return reinterpret_cast<const uint8_t*>(&this[1]);
		}
	};

	struct DrawListNextSubpassInstruction
	{
		DrawListInstruction header;
		RDD::CommandBufferType command_buffer_type;
	};

	struct DrawListSetBlendConstantsInstruction
	{
		DrawListInstruction header;
		Color color;
	};

	struct DrawListSetLineWidthInstruction
	{
		DrawListInstruction header;
		float width;
	};

	struct DrawListSetScissorInstruction
	{
		DrawListInstruction header;
		Rect2i rect;
	};

	struct DrawListSetViewportInstruction
	{
		DrawListInstruction header;
		Rect2i rect;
	};

	struct DrawListUniformSetPrepareForUseInstruction
	{
		DrawListInstruction header;
		RDD::UniformSetID uniform_set;
		RDD::ShaderID shader;
		uint32_t set_index = 0;
	};

	struct RaytracingListBuildAccelerationStructureInstruction
	{
		RaytracingListInstruction header;
		RDD::AccelerationStructureID acceleration_structure;
		RDC::AccelerationStructureType acceleration_structure_type;
	};

	struct RaytracingListBindPipelineInstruction
	{
		RaytracingListInstruction header;
		RDD::RaytracingPipelineID pipeline;
	};

	struct RaytracingListBindUniformSetInstruction
	{
		RaytracingListInstruction header;
		RDD::UniformSetID uniform_set;
		RDD::ShaderID shader;
		uint32_t set_index = 0;
	};

	struct RaytracingListSetPushConstantInstruction
	{
		RaytracingListInstruction header;
		uint32_t size = 0;
		RDD::ShaderID shader;

		_FORCE_INLINE_ uint8_t* data() { return reinterpret_cast<uint8_t*>(&this[1]); }

		_FORCE_INLINE_ const uint8_t* data() const
		{
			return reinterpret_cast<const uint8_t*>(&this[1]);
		}
	};

	struct RaytracingListTraceRaysInstruction
	{
		RaytracingListInstruction header;
		RDD::ShaderBindingTable raygen_sbt;
		RDD::ShaderBindingTable miss_sbt;
		RDD::ShaderBindingTable hit_sbt;
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t depth = 0;
	};

	struct RaytracingListUniformSetPrepareForUseInstruction
	{
		RaytracingListInstruction header;
		RDD::UniformSetID uniform_set;
		RDD::ShaderID shader;
		uint32_t set_index = 0;
	};

	struct ComputeListBindPipelineInstruction
	{
		ComputeListInstruction header;
		RDD::PipelineID pipeline;
	};

	struct ComputeListBindUniformSetsInstruction
	{
		ComputeListInstruction header;
		RDD::ShaderID shader;
		uint32_t first_set_index = 0;
		uint32_t set_count = 0;
		uint32_t dynamic_offsets_mask = 0u;

		_FORCE_INLINE_ RDD::UniformSetID* uniform_set_ids()
		{
			return reinterpret_cast<RDD::UniformSetID*>(&this[1]);
		}

		_FORCE_INLINE_ const RDD::UniformSetID* uniform_set_ids() const
		{
			return reinterpret_cast<const RDD::UniformSetID*>(&this[1]);
		}
	};

	struct ComputeListDispatchInstruction
	{
		ComputeListInstruction header;
		uint32_t x_groups = 0;
		uint32_t y_groups = 0;
		uint32_t z_groups = 0;
	};

	struct ComputeListDispatchIndirectInstruction
	{
		ComputeListInstruction header;
		RDD::BufferID buffer;
		uint32_t offset = 0;
	};

	struct ComputeListSetPushConstantInstruction
	{
		ComputeListInstruction header;
		uint32_t size = 0;
		RDD::ShaderID shader;

		_FORCE_INLINE_ uint8_t* data() { return reinterpret_cast<uint8_t*>(&this[1]); }

		_FORCE_INLINE_ const uint8_t* data() const
		{
			return reinterpret_cast<const uint8_t*>(&this[1]);
		}
	};

	struct ComputeListUniformSetPrepareForUseInstruction
	{
		ComputeListInstruction header;
		RDD::UniformSetID uniform_set;
		RDD::ShaderID shader;
		uint32_t set_index = 0;
	};

	struct BarrierGroup
	{
		uint32_t src_stages = 0;
		uint32_t dst_stages = 0;
		RDD::MemoryAccessBarrier memory_barrier;
		LocalVector<RDD::TextureBarrier> normalization_barriers;
		LocalVector<RDD::TextureBarrier> transition_barriers;
#if USE_BUFFER_BARRIERS
		LocalVector<RDD::BufferBarrier> buffer_barriers;
#endif
		LocalVector<RDD::AccelerationStructureBarrier> acceleration_structure_barriers;

		void clear()
		{
			src_stages = 0;
			dst_stages = 0;
			memory_barrier.src_access = 0;
			memory_barrier.dst_access = 0;
			normalization_barriers.clear();
			transition_barriers.clear();
#if USE_BUFFER_BARRIERS
			buffer_barriers.clear();
#endif
			acceleration_structure_barriers.clear();
		}
	};

	struct SecondaryCommandBuffer
	{
		LocalVector<uint8_t> instruction_data;
		RDD::CommandBufferID command_buffer;
		RDD::CommandPoolID command_pool;
		RDD::RenderPassID render_pass;
		RDD::FramebufferID framebuffer;
	};

	struct Frame
	{
		TightLocalVector<SecondaryCommandBuffer> secondary_command_buffers;
		uint32_t secondary_command_buffers_used = 0;
	};

	RDD* driver = nullptr;
	RDC::DriverWorkarounds driver_workarounds;
	RenderPassCreationFunction render_pass_creation_function = nullptr;
	int64_t tracking_frame = 0;
	LocalVector<uint8_t> command_data;
	LocalVector<uint32_t> command_data_offsets;
	LocalVector<RDD::TextureBarrier> command_normalization_barriers;
	LocalVector<RDD::TextureBarrier> command_transition_barriers;
	LocalVector<RDD::BufferBarrier> command_buffer_barriers;
	LocalVector<RDD::AccelerationStructureBarrier> command_acceleration_structure_barriers;
	LocalVector<char> command_label_chars;
	LocalVector<Color> command_label_colors;
	LocalVector<uint32_t> command_label_offsets;
	int32_t command_label_index = -1;
	DrawInstructionList draw_instruction_list;
	ComputeInstructionList compute_instruction_list;
	RaytracingInstructionList raytracing_instruction_list;
	uint32_t command_count = 0;
	uint32_t command_label_count = 0;
	LocalVector<RecordedCommandListNode> command_list_nodes;
	LocalVector<RecordedSliceListNode> read_slice_list_nodes;
	LocalVector<RecordedSliceListNode> write_slice_list_nodes;
	int32_t command_timestamp_index = -1;
	int32_t command_synchronization_index = -1;
	bool command_synchronization_pending = false;
	BarrierGroup barrier_group;
	bool driver_honors_barriers : 1;
	bool driver_clears_with_copy_engine : 1;
	bool driver_buffers_require_transitions : 1;
	WorkaroundsState workarounds_state;
	TightLocalVector<Frame> frames;
	uint32_t frame = 0;

#ifdef DEV_ENABLED
	RBMap<ResourceTracker*, uint32_t> write_dependency_counters;
#endif

	static String _usage_to_string(ResourceUsage p_usage);
	static bool _is_write_usage(ResourceUsage p_usage);
	static RDD::TextureLayout _usage_to_image_layout(ResourceUsage p_usage);
	static RDD::BarrierAccessBits _usage_to_access_bits(ResourceUsage p_usage);
	bool _check_command_intersection(ResourceTracker* p_resource_tracker,
		int32_t p_previous_command_index, int32_t p_command_index) const;
	bool _check_command_partial_coverage(
		ResourceTracker* p_resource_tracker, int32_t p_command_index) const;
	int32_t _add_to_command_list(int32_t p_command_index, int32_t p_list_index);
	void _add_adjacent_command(
		int32_t p_previous_command_index, int32_t p_command_index, RecordedCommand* r_command);
	int32_t _add_to_slice_read_list(
		int32_t p_command_index, Rect2i p_subresources, int32_t p_list_index);
	int32_t _add_to_write_list(int32_t p_command_index, Rect2i p_subresources, int32_t p_list_index,
		bool p_partial_coverage);
	RecordedCommand* _allocate_command(uint32_t p_command_size, int32_t& r_command_index);
	DrawListInstruction* _allocate_draw_list_instruction(uint32_t p_instruction_size);
	ComputeListInstruction* _allocate_compute_list_instruction(uint32_t p_instruction_size);
	void _check_discardable_attachment_dependency(ResourceTracker* p_resource_tracker,
		int32_t p_previous_command_index, int32_t p_command_index);
	RaytracingListInstruction* _allocate_raytracing_list_instruction(uint32_t p_instruction_size);
	void _add_command_to_graph(ResourceTracker** p_resource_trackers,
		ResourceUsage* p_resource_usages, uint32_t p_resource_count, int32_t p_command_index,
		RecordedCommand* r_command);
	void _add_texture_barrier_to_command(RDD::TextureID p_texture_id,
		uint32_t p_src_access,
		uint32_t p_dst_access, ResourceUsage p_prev_usage,
		ResourceUsage p_next_usage, RDD::TextureSubresourceRange p_subresources,
		LocalVector<RDD::TextureBarrier>& r_barrier_vector, int32_t& r_barrier_index,
		int32_t& r_barrier_count);
#if USE_BUFFER_BARRIERS
	void _add_buffer_barrier_to_command(RDD::BufferID p_buffer_id,
		uint32_t p_src_access,
		uint32_t p_dst_access, int32_t& r_barrier_index,
		int32_t& r_barrier_count);
#endif
	void _add_acceleration_structure_barrier_to_command(
		RDD::AccelerationStructureID p_acceleration_structure_id,
		uint32_t p_src_access,
		uint32_t p_dst_access,
		LocalVector<RDD::AccelerationStructureBarrier>& r_barrier_vector, int32_t& r_barrier_index,
		int32_t& r_barrier_count);
	void _run_compute_list_command(RDD::CommandBufferID p_command_buffer,
		const uint8_t* p_instruction_data, uint32_t p_instruction_data_size);
	void _get_draw_list_render_pass_and_framebuffer(
		const RecordedDrawListCommand* p_draw_list_command, RDD::RenderPassID& r_render_pass,
		RDD::FramebufferID& r_framebuffer);
	void _run_raytracing_list_command(RDD::CommandBufferID p_command_buffer,
		const uint8_t* p_instruction_data, uint32_t p_instruction_data_size);
	void _run_draw_list_command(RDD::CommandBufferID p_command_buffer,
		const uint8_t* p_instruction_data, uint32_t p_instruction_data_size);
	void _add_draw_list_begin(FramebufferCache* p_framebuffer_cache,
		RDD::RenderPassID p_render_pass, RDD::FramebufferID p_framebuffer, Rect2i p_region,
		VectorView<AttachmentOperation> p_attachment_operations,
		VectorView<RDD::RenderPassClearValue> p_attachment_clear_values,
		uint32_t p_stages, uint32_t p_breadcrumb, bool p_split_cmd_buffer);
	void _run_secondary_command_buffer_task(const SecondaryCommandBuffer* p_secondary);
	void _wait_for_secondary_command_buffer_tasks();
	void _run_render_commands(int32_t p_level, const RecordedCommandSort* p_sorted_commands,
		uint32_t p_sorted_commands_count, RDD::CommandBufferID& r_command_buffer,
		CommandBufferPool& r_command_buffer_pool, int32_t& r_current_label_index,
		int32_t& r_current_label_level);
	void _run_label_command_change(RDD::CommandBufferID p_command_buffer, int32_t p_new_label_index,
		int32_t p_new_level, bool p_ignore_previous_value, bool p_use_label_for_empty,
		const RecordedCommandSort* p_sorted_commands, uint32_t p_sorted_commands_count,
		int32_t& r_current_label_index, int32_t& r_current_label_level);
	void _boost_priority_for_render_commands(RecordedCommandSort* p_sorted_commands,
		uint32_t p_sorted_commands_count, uint32_t& r_boosted_priority);
	void _group_barriers_for_render_commands(RDD::CommandBufferID p_command_buffer,
		const RecordedCommandSort* p_sorted_commands, uint32_t p_sorted_commands_count,
		bool p_full_memory_barrier);
	void _print_render_commands(
		const RecordedCommandSort* p_sorted_commands, uint32_t p_sorted_commands_count);
	void _print_draw_list(const uint8_t* p_instruction_data, uint32_t p_instruction_data_size);
	void _print_compute_list(const uint8_t* p_instruction_data, uint32_t p_instruction_data_size);
	void _print_raytracing_list(
		const uint8_t* p_instruction_data, uint32_t p_instruction_data_size);

public:
	RenderingDeviceGraph();
	~RenderingDeviceGraph();
	void initialize(RDD* p_driver, RenderPassCreationFunction p_render_pass_creation_function,
		uint32_t p_frame_count, RDD::CommandQueueFamilyID p_secondary_command_queue_family,
		uint32_t p_secondary_command_buffers_per_frame);
	void finalize();
	void begin();
	void add_blas_build(RDD::AccelerationStructureID p_blas, RDD::BufferID p_scratch_buffer,
		ResourceTracker* p_dst_tracker, VectorView<ResourceTracker*> p_src_trackers);
	void add_tlas_build(RDD::AccelerationStructureID p_tlas, RDD::BufferID p_scratch_buffer,
		RDD::BufferID p_instance_buffer, uint32_t p_instance_offset, uint32_t p_instance_count,
		ResourceTracker* p_dst_tracker, VectorView<ResourceTracker*> p_src_trackers);
	void add_buffer_clear(
		RDD::BufferID p_dst, ResourceTracker* p_dst_tracker, uint32_t p_offset, uint32_t p_size);
	void add_buffer_copy(RDD::BufferID p_src, ResourceTracker* p_src_tracker, RDD::BufferID p_dst,
		ResourceTracker* p_dst_tracker, RDD::BufferCopyRegion p_region);
	void add_buffer_get_data(RDD::BufferID p_src, ResourceTracker* p_src_tracker,
		RDD::BufferID p_dst, RDD::BufferCopyRegion p_region);
	void add_buffer_update(RDD::BufferID p_dst, ResourceTracker* p_dst_tracker,
		VectorView<RecordedBufferCopy> p_buffer_copies);
	void add_driver_callback(RDD::DriverCallback p_callback, void* p_userdata,
		VectorView<ResourceTracker*> p_trackers, VectorView<ResourceUsage> p_usages);
	void add_raytracing_list_begin();
	void add_raytracing_list_bind_pipeline(RDD::RaytracingPipelineID p_pipeline);
	void add_raytracing_list_bind_uniform_set(
		RDD::ShaderID p_shader, RDD::UniformSetID p_uniform_set, uint32_t set_index);
	void add_raytracing_list_set_push_constant(
		RDD::ShaderID p_shader, const void* p_data, uint32_t p_data_size);
	void add_raytracing_list_trace_rays(const RDD::ShaderBindingTable& p_raygen_sbt,
		const RDD::ShaderBindingTable& p_miss_sbt, const RDD::ShaderBindingTable& p_hit_sbt,
		uint32_t p_width, uint32_t p_height, uint32_t p_depth);
	void add_raytracing_list_uniform_set_prepare_for_use(
		RDD::ShaderID p_shader, RDD::UniformSetID p_uniform_set, uint32_t set_index);
	void add_raytracing_list_usage(ResourceTracker* p_tracker, ResourceUsage p_usage);
	void add_raytracing_list_usages(
		VectorView<ResourceTracker*> p_trackers, VectorView<ResourceUsage> p_usages);
	void add_raytracing_list_end();
	void add_compute_list_begin(RDC::BreadcrumbMarker p_phase = RDC::BreadcrumbMarker::NONE,
		uint32_t p_breadcrumb_data = 0);
	void add_compute_list_bind_pipeline(RDD::PipelineID p_pipeline);
	void add_compute_list_bind_uniform_set(
		RDD::ShaderID p_shader, RDD::UniformSetID p_uniform_set, uint32_t set_index);
	void add_compute_list_bind_uniform_sets(RDD::ShaderID p_shader,
		VectorView<RDD::UniformSetID> p_uniform_set, uint32_t p_first_set_index,
		uint32_t p_set_count);
	void add_compute_list_dispatch(uint32_t p_x_groups, uint32_t p_y_groups, uint32_t p_z_groups);
	void add_compute_list_dispatch_indirect(RDD::BufferID p_buffer, uint32_t p_offset);
	void add_compute_list_set_push_constant(
		RDD::ShaderID p_shader, const void* p_data, uint32_t p_data_size);
	void add_compute_list_uniform_set_prepare_for_use(
		RDD::ShaderID p_shader, RDD::UniformSetID p_uniform_set, uint32_t set_index);
	void add_compute_list_usage(ResourceTracker* p_tracker, ResourceUsage p_usage);
	void add_compute_list_usages(
		VectorView<ResourceTracker*> p_trackers, VectorView<ResourceUsage> p_usages);
	void add_compute_list_end();
	void add_draw_list_begin(FramebufferCache* p_framebuffer_cache, Rect2i p_region,
		VectorView<AttachmentOperation> p_attachment_operations,
		VectorView<RDD::RenderPassClearValue> p_attachment_clear_values,
		uint32_t p_stages, uint32_t p_breadcrumb = 0,
		bool p_split_cmd_buffer = false);
	void add_draw_list_begin(RDD::RenderPassID p_render_pass, RDD::FramebufferID p_framebuffer,
		Rect2i p_region, VectorView<AttachmentOperation> p_attachment_operations,
		VectorView<RDD::RenderPassClearValue> p_attachment_clear_values,
		uint32_t p_stages, uint32_t p_breadcrumb = 0,
		bool p_split_cmd_buffer = false);
	void add_draw_list_bind_index_buffer(RDD::BufferID p_buffer,
        RenderingDeviceCommons::IndexBufferFormat p_format, uint32_t p_offset);
	void add_draw_list_bind_pipeline(
		RDD::PipelineID p_pipeline, uint32_t p_pipeline_stage_bits);
	void add_draw_list_bind_uniform_set(
		RDD::ShaderID p_shader, RDD::UniformSetID p_uniform_set, uint32_t set_index);
	void add_draw_list_bind_uniform_sets(RDD::ShaderID p_shader,
		VectorView<RDD::UniformSetID> p_uniform_set, uint32_t p_first_index, uint32_t p_set_count);
	void add_draw_list_bind_vertex_buffers(
		Span<RDD::BufferID> p_vertex_buffers, Span<uint64_t> p_vertex_buffer_offsets);
	void add_draw_list_clear_attachments(VectorView<RDD::AttachmentClear> p_attachments_clear,
		VectorView<Rect2i> p_attachments_clear_rect);
	void add_draw_list_draw(uint32_t p_vertex_count, uint32_t p_instance_count);
	void add_draw_list_draw_indexed(
		uint32_t p_index_count, uint32_t p_instance_count, uint32_t p_first_index);
	void add_draw_list_draw_indirect(
		RDD::BufferID p_buffer, uint32_t p_offset, uint32_t p_draw_count, uint32_t p_stride);
	void add_draw_list_draw_indexed_indirect(
		RDD::BufferID p_buffer, uint32_t p_offset, uint32_t p_draw_count, uint32_t p_stride);
	void add_draw_list_execute_commands(RDD::CommandBufferID p_command_buffer);
	void add_draw_list_next_subpass(RDD::CommandBufferType p_command_buffer_type);
	void add_draw_list_set_blend_constants(const Color& p_color);
	void add_draw_list_set_line_width(float p_width);
	void add_draw_list_set_push_constant(
		RDD::ShaderID p_shader, const void* p_data, uint32_t p_data_size);
	void add_draw_list_set_scissor(Rect2i p_rect);
	void add_draw_list_set_viewport(Rect2i p_rect);
	void add_draw_list_uniform_set_prepare_for_use(
		RDD::ShaderID p_shader, RDD::UniformSetID p_uniform_set, uint32_t set_index);
	void add_draw_list_usage(ResourceTracker* p_tracker, ResourceUsage p_usage);
	void add_draw_list_usages(
		VectorView<ResourceTracker*> p_trackers, VectorView<ResourceUsage> p_usages);
	void add_draw_list_end();
	void add_texture_clear_color(RDD::TextureID p_dst, ResourceTracker* p_dst_tracker,
		const Color& p_color, const RDD::TextureSubresourceRange& p_range);
	void add_texture_clear_depth_stencil(RDD::TextureID p_dst, ResourceTracker* p_dst_tracker,
		float p_depth, uint8_t p_stencil, const RDD::TextureSubresourceRange& p_range);
	void add_texture_copy(RDD::TextureID p_src, ResourceTracker* p_src_tracker,
		RDD::TextureID p_dst, ResourceTracker* p_dst_tracker,
		VectorView<RDD::TextureCopyRegion> p_texture_copy_regions);
	void add_texture_get_data(RDD::TextureID p_src, ResourceTracker* p_src_tracker,
		RDD::BufferID p_dst, VectorView<RDD::BufferTextureCopyRegion> p_buffer_texture_copy_regions,
		ResourceTracker* p_dst_tracker = nullptr);
	void add_texture_resolve(RDD::TextureID p_src, ResourceTracker* p_src_tracker,
		RDD::TextureID p_dst, ResourceTracker* p_dst_tracker, uint32_t p_src_layer,
		uint32_t p_src_mipmap, uint32_t p_dst_layer, uint32_t p_dst_mipmap);
	void add_texture_update(RDD::TextureID p_dst, ResourceTracker* p_dst_tracker,
		VectorView<RecordedBufferToTextureCopy> p_buffer_copies,
		VectorView<ResourceTracker*> p_buffer_trackers = VectorView<ResourceTracker*>());
	void add_capture_timestamp(RDD::QueryPoolID p_query_pool, uint32_t p_index);
	void add_synchronization();
	void begin_label(const Span<char>& p_label_name, const Color& p_color);
	void end_label();
	void end(bool p_reorder_commands, bool p_full_barriers, RDD::CommandBufferID& r_command_buffer,
		CommandBufferPool& r_command_buffer_pool);
	static ResourceTracker* resource_tracker_create();
	static void resource_tracker_free(ResourceTracker* p_tracker);
	static FramebufferCache* framebuffer_cache_create();
	static void framebuffer_cache_free(RDD* p_driver, FramebufferCache* p_cache);
};

using RDG = RenderingDeviceGraph;



// --- End servers/rendering/rendering_device_graph.h ---

class RDTextureFormat;
class RDTextureView;
class RDAttachmentFormat;
class RDSamplerState;
class RDVertexAttribute;
class RDShaderSource;
class RDShaderSPIRV;
class RDUniform;
class RDPipelineRasterizationState;
class RDPipelineMultisampleState;
class RDPipelineDepthStencilState;
class RDPipelineColorBlendState;
class RDFramebufferPass;
class RDPipelineSpecializationConstant;
class RDAccelerationStructureGeometry;
class RDAccelerationStructureInstance;
class RDPipelineShader;
class RDHitGroup;

class RenderingDevice final
{
	static inline Mutex _thread_safe_mutex;

public:
	using DrawListID = int64_t;
	using ComputeListID = int64_t;
	using RaytracingListID = int64_t;
	using FramebufferFormatID = int64_t;
	using VertexFormatID = int64_t;
	using HitShaderBindingTableRange = int64_t;
	using InvalidationCallback = void (*)(void*);

	enum
	{
		INVALID_FORMAT_ID = -1
	};

	enum IDType
	{
		ID_TYPE_FRAMEBUFFER_FORMAT,
		ID_TYPE_VERTEX_FORMAT,
		ID_TYPE_DRAW_LIST,
		ID_TYPE_COMPUTE_LIST = 4,
		ID_TYPE_RAYTRACING_LIST = 5,
		ID_TYPE_MAX,
		ID_BASE_SHIFT = 58,
		ID_MASK = (ID_BASE_SHIFT - 1),
	};

	enum BufferCreationBits
	{
		BUFFER_CREATION_DEVICE_ADDRESS_BIT = (1 << 0),
		BUFFER_CREATION_AS_STORAGE_BIT = (1 << 1),
		BUFFER_CREATION_DYNAMIC_PERSISTENT_BIT = (1 << 2),
		BUFFER_CREATION_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT = (1 << 3),
	};

	enum StorageBufferUsage
	{
		STORAGE_BUFFER_USAGE_DISPATCH_INDIRECT = (1 << 0),
	};

	enum CallbackResourceType
	{
		CALLBACK_RESOURCE_TYPE_TEXTURE,
		CALLBACK_RESOURCE_TYPE_BUFFER,
	};

	enum CallbackResourceUsage
	{
		CALLBACK_RESOURCE_USAGE_NONE,
		CALLBACK_RESOURCE_USAGE_COPY_FROM,
		CALLBACK_RESOURCE_USAGE_COPY_TO,
		CALLBACK_RESOURCE_USAGE_RESOLVE_FROM,
		CALLBACK_RESOURCE_USAGE_RESOLVE_TO,
		CALLBACK_RESOURCE_USAGE_UNIFORM_BUFFER_READ,
		CALLBACK_RESOURCE_USAGE_INDIRECT_BUFFER_READ,
		CALLBACK_RESOURCE_USAGE_TEXTURE_BUFFER_READ,
		CALLBACK_RESOURCE_USAGE_TEXTURE_BUFFER_READ_WRITE,
		CALLBACK_RESOURCE_USAGE_STORAGE_BUFFER_READ,
		CALLBACK_RESOURCE_USAGE_STORAGE_BUFFER_READ_WRITE,
		CALLBACK_RESOURCE_USAGE_VERTEX_BUFFER_READ,
		CALLBACK_RESOURCE_USAGE_INDEX_BUFFER_READ,
		CALLBACK_RESOURCE_USAGE_TEXTURE_SAMPLE,
		CALLBACK_RESOURCE_USAGE_STORAGE_IMAGE_READ,
		CALLBACK_RESOURCE_USAGE_STORAGE_IMAGE_READ_WRITE,
		CALLBACK_RESOURCE_USAGE_ATTACHMENT_COLOR_READ_WRITE,
		CALLBACK_RESOURCE_USAGE_ATTACHMENT_DEPTH_STENCIL_READ_WRITE,
		CALLBACK_RESOURCE_USAGE_ATTACHMENT_FRAGMENT_SHADING_RATE_READ,
		CALLBACK_RESOURCE_USAGE_ATTACHMENT_FRAGMENT_DENSITY_MAP_READ,
		CALLBACK_RESOURCE_USAGE_GENERAL,
		CALLBACK_RESOURCE_USAGE_ACCELERATION_STRUCTURE_READ,
		CALLBACK_RESOURCE_USAGE_ACCELERATION_STRUCTURE_READ_WRITE,
		CALLBACK_RESOURCE_USAGE_MAX
	};

	enum VRSMethod
	{
		VRS_METHOD_NONE,
		VRS_METHOD_FRAGMENT_SHADING_RATE,
		VRS_METHOD_FRAGMENT_DENSITY_MAP,
	};

	enum DrawFlags
	{
		DRAW_DEFAULT_ALL = 0,
		DRAW_CLEAR_COLOR_0 = (1 << 0),
		DRAW_CLEAR_COLOR_1 = (1 << 1),
		DRAW_CLEAR_COLOR_2 = (1 << 2),
		DRAW_CLEAR_COLOR_3 = (1 << 3),
		DRAW_CLEAR_COLOR_4 = (1 << 4),
		DRAW_CLEAR_COLOR_5 = (1 << 5),
		DRAW_CLEAR_COLOR_6 = (1 << 6),
		DRAW_CLEAR_COLOR_7 = (1 << 7),
		DRAW_CLEAR_COLOR_MASK = 0xFF,
		DRAW_CLEAR_COLOR_ALL = DRAW_CLEAR_COLOR_MASK,
		DRAW_IGNORE_COLOR_0 = (1 << 8),
		DRAW_IGNORE_COLOR_1 = (1 << 9),
		DRAW_IGNORE_COLOR_2 = (1 << 10),
		DRAW_IGNORE_COLOR_3 = (1 << 11),
		DRAW_IGNORE_COLOR_4 = (1 << 12),
		DRAW_IGNORE_COLOR_5 = (1 << 13),
		DRAW_IGNORE_COLOR_6 = (1 << 14),
		DRAW_IGNORE_COLOR_7 = (1 << 15),
		DRAW_IGNORE_COLOR_MASK = 0xFF00,
		DRAW_IGNORE_COLOR_ALL = DRAW_IGNORE_COLOR_MASK,
		DRAW_CLEAR_DEPTH = (1 << 16),
		DRAW_IGNORE_DEPTH = (1 << 17),
		DRAW_CLEAR_STENCIL = (1 << 18),
		DRAW_IGNORE_STENCIL = (1 << 19),
		DRAW_CLEAR_ALL = DRAW_CLEAR_COLOR_ALL | DRAW_CLEAR_DEPTH | DRAW_CLEAR_STENCIL,
		DRAW_IGNORE_ALL = DRAW_IGNORE_COLOR_ALL | DRAW_IGNORE_DEPTH | DRAW_IGNORE_STENCIL
	};

	enum MemoryType
	{
		MEMORY_TEXTURES,
		MEMORY_BUFFERS,
		MEMORY_TOTAL
	};

	struct CallbackResource
	{
		RID rid;
		CallbackResourceType type = CALLBACK_RESOURCE_TYPE_TEXTURE;
		CallbackResourceUsage usage = CALLBACK_RESOURCE_USAGE_NONE;
	};

	struct TextureView
	{
		RDC::DataFormat format_override = RDC::DATA_FORMAT_MAX;
		RDC::TextureSwizzle swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		RDC::TextureSwizzle swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		RDC::TextureSwizzle swizzle_b = RDC::TEXTURE_SWIZZLE_B;
		RDC::TextureSwizzle swizzle_a = RDC::TEXTURE_SWIZZLE_A;

		bool operator==(const TextureView& p_other) const
		{
			if (format_override != p_other.format_override) {
				return false;
			}
			else if (swizzle_r != p_other.swizzle_r) {
				return false;
			}
			else if (swizzle_g != p_other.swizzle_g) {
				return false;
			}
			else if (swizzle_b != p_other.swizzle_b) {
				return false;
			}
			else if (swizzle_a != p_other.swizzle_a) {
				return false;
			}
			else {
				return true;
			}
		}
	};

	struct AttachmentFormat
	{
		enum : uint32_t
		{
			UNUSED_ATTACHMENT = 0xFFFFFFFF
		};

		RDC::DataFormat format;
		RDC::TextureSamples samples;
		uint32_t usage_flags;

		AttachmentFormat()
		{
			format = RDC::DATA_FORMAT_R8G8B8A8_UNORM;
			samples = RDC::TEXTURE_SAMPLES_1;
			usage_flags = 0;
		}
	};

	struct FramebufferPass
	{
		Vector<int32_t> color_attachments;
		Vector<int32_t> input_attachments;
		Vector<int32_t> resolve_attachments;
		Vector<int32_t> preserve_attachments;
		int32_t depth_attachment = RDC::ATTACHMENT_UNUSED;
		int32_t depth_resolve_attachment = RDC::ATTACHMENT_UNUSED;
	};

	struct Uniform
	{
		RDC::UniformType uniform_type = RDC::UNIFORM_TYPE_IMAGE;
		uint32_t binding = 0;
		bool immutable_sampler = false;

	private:
		RID id;
		Vector<RID> ids;

	public:
		_FORCE_INLINE_ uint32_t get_id_count() const { return (id.is_valid() ? 1 : ids.size()); }

		_FORCE_INLINE_ RID get_id(uint32_t p_idx) const
		{
			if (id.is_valid()) {
				ERR_FAIL_COND_V(p_idx != 0, RID());
				return id;
			}
			else {
				return ids[p_idx];
			}
		}

		_FORCE_INLINE_ void set_id(uint32_t p_idx, RID p_id)
		{
			if (id.is_valid()) {
				ERR_FAIL_COND(p_idx != 0);
				id = p_id;
			}
			else {
				ids.write[p_idx] = p_id;
			}
		}

		_FORCE_INLINE_ void append_id(RID p_id)
		{
			if (ids.is_empty()) {
				if (id == RID()) {
					id = p_id;
				}
				else {
					ids.push_back(id);
					ids.push_back(p_id);
					id = RID();
				}
			}
			else {
				ids.push_back(p_id);
			}
		}

		_FORCE_INLINE_ void clear_ids()
		{
			id = RID();
			ids.clear();
		}

		_FORCE_INLINE_ Uniform(RDC::UniformType p_type, int p_binding, RID p_id)
		{
			uniform_type = p_type;
			binding = p_binding;
			id = p_id;
		}

		_FORCE_INLINE_ Uniform(RDC::UniformType p_type, int p_binding, const Vector<RID>& p_ids)
		{
			uniform_type = p_type;
			binding = p_binding;
			ids = p_ids;
		}

		_FORCE_INLINE_ Uniform() = default;
	};

	static void _texture_ensure_shareable_format(
		RID p_texture, const RDC::DataFormat& p_shareable_format);

	using PipelineImmutableSampler = Uniform;

	struct PipelineShader
	{
		RID shader;
		Vector<RDC::PipelineSpecializationConstant> specialization_constants;

		bool is_valid() const { return shader.is_valid(); }
	};

	struct HitGroup
	{
		PipelineShader closest_hit_shader;
		PipelineShader any_hit_shader;
		PipelineShader intersection_shader;
	};

	struct AccelerationStructureGeometry
	{
		uint32_t flags = 0;
		RID vertex_buffer;
		uint32_t vertex_offset = 0;
		uint32_t vertex_stride = 0;
		uint32_t vertex_count = 0;
		RDC::DataFormat vertex_format = RDC::DATA_FORMAT_MAX;
		RID index_buffer;
		uint32_t index_offset = 0;
		uint32_t index_count = 0;
	};

	struct AccelerationStructureInstance
	{
		Transform3D transform;
		uint32_t id = 0;
		uint8_t mask = 0xFF;
		HitShaderBindingTableRange hit_sbt_range = 0;
		uint32_t flags = 0;
		RID blas;
	};

public:
	// --- Lifecycle & System Management ---
	static void make_current();

	static RenderingDeviceDriver* get_device_driver() { return data->driver; }

	static RenderingContextDriver* get_context_driver() { return data->context; }

	static const RDD::Capabilities& get_device_capabilities()
	{
		return data->driver->get_capabilities();
	}

	static void free_rid(RID p_rid);
	static void _free_internal(RID p_id);
	static void _execute_frame(bool p_present);
	template <typename T> static void _free_rids(T& p_owner, const char* p_type);

	static void swap_buffers(bool p_present);
	static void sync();
	static void _set_max_fps(int p_max_fps);
	static uint32_t get_frame_delay();

	static uint64_t get_frames_drawn() { return data->frames_drawn; }

	static bool has_pending_resources_for_processing()
	{
		return data->frames_pending_resources_for_processing != 0u;
	}

	static bool has_feature(const RDC::Features p_feature);

	// --- Buffer Operations ---
	static RID index_buffer_create(uint32_t p_index_count, RDC::IndexBufferFormat p_format,
        Span<uint8_t> p_data = {}, bool p_use_restart_indices = false);
	static RID vertex_buffer_create(
		uint32_t p_size_bytes, Span<uint8_t> p_data = {}, uint32_t p_creation_bits = 0);
	static VertexFormatID vertex_format_create(
		const Vector<RDC::VertexAttribute>& p_vertex_descriptions);
	static RID vertex_array_create(uint32_t p_vertex_count, VertexFormatID p_vertex_format,
		const Vector<RID>& p_src_buffers, const Vector<uint64_t>& p_offsets = Vector<uint64_t>());
	static RID index_array_create(
		RID p_index_buffer, uint32_t p_index_offset, uint32_t p_index_count);

	static RID uniform_buffer_create(
		uint32_t p_size_bytes, Span<uint8_t> p_data = {}, uint32_t p_creation_bits = 0);
	static RID storage_buffer_create(
		uint32_t p_size_bytes, Span<uint8_t> p_data = {}, uint32_t p_creation_bits = 0);
	static RID texture_buffer_create(
		uint32_t p_size_elements, RDC::DataFormat p_format, Span<uint8_t> p_data = {});

	static Error buffer_copy(RID p_src_buffer, RID p_dst_buffer, uint32_t p_src_offset,
		uint32_t p_dst_offset, uint32_t p_size);
	static Error buffer_update(RID p_buffer, uint32_t p_offset, uint32_t p_size,
		const void* p_data, bool p_skip_check = false);
	static Error buffer_clear(RID p_buffer, uint32_t p_offset, uint32_t p_size);
	static Vector<uint8_t> buffer_get_data(
		RID p_buffer, uint32_t p_offset = 0, uint32_t p_size = 0);
	static Error buffer_get_data_async(RID p_buffer, uint32_t p_offset = 0, uint32_t p_size = 0);
	static uint64_t buffer_get_device_address(RID p_buffer);
	static uint8_t* buffer_persistent_map_advance(RID p_buffer);
	static void buffer_flush(RID p_buffer);

	// --- Texture Operations ---
	static RID texture_create(
		const RDC::TextureFormat& p_format, const RDD::TextureView& p_view, const Vector<Vector<uint8_t>>& p_data = {});
	static RID texture_create_shared(const TextureView& p_view, RID p_with_texture);
	static RID texture_create_from_extension(RDC::TextureType p_type, RDC::DataFormat p_format,
		RDC::TextureSamples p_samples, uint32_t p_usage, uint64_t p_image, uint32_t p_width,
		uint32_t p_height, uint32_t p_depth, uint32_t p_layers, uint32_t p_mipmaps = 1);
	static RID texture_create_shared_from_slice(const TextureView& p_view, RID p_with_texture,
		uint32_t p_layer, uint32_t p_mipmap, uint32_t p_mipmaps = 1,
		RDC::TextureSliceType p_slice_type = RDC::TEXTURE_SLICE_2D, uint32_t p_layers = 0);
	static Error texture_update(RID p_texture, uint32_t p_layer, const Vector<uint8_t>& p_data);
	static Vector<uint8_t> texture_get_data(RID p_texture, uint32_t p_layer);
	static Error texture_get_data_async(RID p_texture, uint32_t p_layer);

	static bool texture_is_format_supported_for_usage(RDC::DataFormat p_format, uint32_t p_usage);
	static bool texture_is_shared(RID p_texture);
	static bool texture_is_valid(RID p_texture);
	static RDC::TextureFormat texture_get_format(RID p_texture);
	static Size2i texture_size(RID p_texture);

	static Error texture_copy(RID p_from_texture, RID p_to_texture, const Vector3& p_from,
		const Vector3& p_to, const Vector3& p_size, uint32_t p_src_mipmap, uint32_t p_dst_mipmap,
		uint32_t p_src_layer, uint32_t p_dst_layer);
	static Error texture_clear(RID p_texture, const Color& p_color, uint32_t p_base_mipmap,
		uint32_t p_mipmaps, uint32_t p_base_layer, uint32_t p_layers);
	static Error texture_resolve_multisample(RID p_from_texture, RID p_to_texture);

	static void texture_set_discardable(RID p_texture, bool p_discardable);
	static bool texture_is_discardable(RID p_texture);

	// --- VRS & Framebuffers ---
	static VRSMethod vrs_get_method();
	static RDC::DataFormat vrs_get_format();
	static Size2i vrs_get_texel_size();

	static FramebufferFormatID framebuffer_format_create(const Vector<AttachmentFormat>& p_format,
		uint32_t p_view_count = 1, int32_t p_vrs_attachment = -1);
	static FramebufferFormatID framebuffer_format_create_multipass(
		const Vector<AttachmentFormat>& p_format, const Vector<FramebufferPass>& p_passes,
		uint32_t p_view_count = 1, int32_t p_vrs_attachment = -1);
	static FramebufferFormatID framebuffer_format_create_empty(
		RDC::TextureSamples p_samples = RDC::TEXTURE_SAMPLES_1);
	static RDC::TextureSamples framebuffer_format_get_texture_samples(
		FramebufferFormatID p_format, uint32_t p_pass = 0);

	static RID framebuffer_create(const Vector<RID>& p_texture_attachments,
		FramebufferFormatID p_format_check = RDC::INVALID_ID, uint32_t p_view_count = 1);
	static RID framebuffer_create_multipass(const Vector<RID>& p_texture_attachments,
		const Vector<FramebufferPass>& p_passes, FramebufferFormatID p_format_check = RDC::INVALID_ID,
		uint32_t p_view_count = 1);
	static RID framebuffer_create_empty(const Size2i& p_size,
		RDC::TextureSamples p_samples = RDC::TEXTURE_SAMPLES_1,
		FramebufferFormatID p_format_check = RDC::INVALID_ID);
	static bool framebuffer_is_valid(RID p_framebuffer);
	static void framebuffer_set_invalidation_callback(
		RID p_framebuffer, InvalidationCallback p_callback, void* p_userdata);
	static FramebufferFormatID framebuffer_get_format(RID p_framebuffer);
	static Size2 framebuffer_get_size(RID p_framebuffer);

	// --- Samplers & Shaders ---
	static RID sampler_create(const RDC::SamplerState& p_state);
	static bool sampler_is_format_supported_for_filter(
		RDC::DataFormat p_format, RDC::SamplerFilter p_sampler_filter);

	static Vector<uint8_t> shader_compile_spirv_from_source(RDC::ShaderStage p_stage,
		const String& p_source_code, RDC::ShaderLanguage p_language = RDC::SHADER_LANGUAGE_GLSL,
		String* r_error = nullptr, bool p_allow_cache = true);
	static Vector<uint8_t> shader_compile_binary_from_spirv(
		const Vector<RDC::ShaderStageSPIRVData>& p_spirv, const String& p_shader_name = "");
	static RID shader_create_from_bytecode(
		const Vector<uint8_t>& p_shader_binary, RID p_placeholder = RID());
	static RID shader_create_placeholder();
	static void shader_destroy_modules(RID p_shader);
	static uint64_t shader_get_vertex_input_attribute_mask(RID p_shader);

	static RID uniform_set_create(const VectorView<Uniform>& p_uniforms, RID p_shader,
		uint32_t p_shader_set, bool p_linear_pool = false);
	static bool uniform_set_is_valid(RID p_uniform_set);
	static void uniform_set_set_invalidation_callback(
		RID p_uniform_set, InvalidationCallback p_callback, void* p_userdata);
	static bool uniform_sets_have_linear_pools();

	// --- Pipelines ---
	static RID render_pipeline_create(RID p_shader, FramebufferFormatID p_framebuffer_format,
		VertexFormatID p_vertex_format, RDC::RenderPrimitive p_render_primitive,
		const RDC::PipelineRasterizationState& p_rasterization_state,
		const RDC::PipelineMultisampleState& p_multisample_state,
		const RDC::PipelineDepthStencilState& p_depth_stencil_state,
		const RDC::PipelineColorBlendState& p_blend_state, uint32_t p_dynamic_state_flags = 0,
		uint32_t p_for_render_pass = 0,
		const Vector<RDC::PipelineSpecializationConstant>& p_specialization_constants =
			Vector<RDC::PipelineSpecializationConstant>());
	static bool render_pipeline_is_valid(RID p_pipeline);

	static RID compute_pipeline_create(
		RID p_shader, const Vector<RDC::PipelineSpecializationConstant>& p_specialization_constants =
						  Vector<RDC::PipelineSpecializationConstant>());
	static bool compute_pipeline_is_valid(RID p_pipeline);

	static RID raytracing_pipeline_create(Span<PipelineShader> p_raygen_shaders,
		Span<PipelineShader> p_miss_shaders, Span<HitGroup> p_hit_groups,
		uint32_t p_max_trace_recursion_depth);
	static bool raytracing_pipeline_is_valid(RID p_pipeline);

	// --- Raytracing & Acceleration Structures ---
	static RID blas_create(Span<AccelerationStructureGeometry> p_geometries, uint32_t p_flags);
	static RID tlas_create(uint32_t p_max_instance_count, uint32_t p_flags);
	static Error blas_build(RID p_blas);
	static Error tlas_build(RID p_tlas, Span<AccelerationStructureInstance> p_instances);

	static RID hit_sbt_create(RID p_raytracing_pipeline, uint32_t p_initial_hit_group_capacity);
	static Error hit_sbt_set_pipeline(RID p_hit_sbt, RID p_raytracing_pipeline);
	static HitShaderBindingTableRange hit_sbt_range_alloc(
		RID p_hit_sbt, uint32_t p_hit_group_count);
	static Error hit_sbt_range_free(RID p_hit_sbt, HitShaderBindingTableRange p_range);
	static Error hit_sbt_range_update(RID p_hit_sbt, HitShaderBindingTableRange p_range,
		uint32_t p_hit_group_offset, Span<uint32_t> p_hit_group_indices);

	// --- Command Lists ---
	static DrawListID draw_list_begin_for_screen(
		DisplayServerEnums::WindowID p_screen = 0, const Color& p_clear_color = Color());
	static DrawListID draw_list_begin(RID p_framebuffer, uint32_t p_draw_flags = DRAW_DEFAULT_ALL,
		const Vector<Color>& p_clear_color_values = Vector<Color>(),
		float p_clear_depth_value = 1.0f, uint32_t p_clear_stencil_value = 0,
		const Rect2& p_region = Rect2(), uint32_t p_breadcrumb = 0);
	static void draw_list_set_blend_constants(DrawListID p_list, const Color& p_color);
	static void draw_list_bind_render_pipeline(DrawListID p_list, RID p_render_pipeline);
	static void draw_list_bind_uniform_set(DrawListID p_list, RID p_uniform_set, uint32_t p_index);
	static void draw_list_bind_vertex_array(DrawListID p_list, RID p_vertex_array);
	static void draw_list_bind_vertex_buffers_format(DrawListID p_list,
		VertexFormatID p_vertex_format, uint32_t p_vertex_count,
		const Span<RID>& p_vertex_buffers,
		const Span<uint64_t>& p_offsets = Vector<uint64_t>());
	static void draw_list_bind_index_array(DrawListID p_list, RID p_index_array);
	static void draw_list_set_line_width(DrawListID p_list, float p_width);
	static void draw_list_set_push_constant(
		DrawListID p_list, const void* p_data, uint32_t p_data_size);
	static void draw_list_draw(DrawListID p_list, bool p_use_indices, uint32_t p_instances = 1,
		uint32_t p_procedural_vertices = 0);
	static void draw_list_draw_indirect(DrawListID p_list, bool p_use_indices, RID p_buffer,
		uint32_t p_offset = 0, uint32_t p_draw_count = 1, uint32_t p_stride = 0);
	static void draw_list_set_viewport(DrawListID p_list, const Rect2& p_rect);
	static void draw_list_enable_scissor(DrawListID p_list, const Rect2& p_rect);
	static void draw_list_disable_scissor(DrawListID p_list);
	static uint32_t draw_list_get_current_pass();
	static DrawListID draw_list_switch_to_next_pass();
	static void draw_list_end();

	static ComputeListID compute_list_begin();
	static void compute_list_bind_compute_pipeline(ComputeListID p_list, RID p_compute_pipeline);
	static void compute_list_bind_uniform_set(
		ComputeListID p_list, RID p_uniform_set, uint32_t p_index);
	static void compute_list_set_push_constant(
		ComputeListID p_list, const void* p_data, uint32_t p_data_size);
	static void compute_list_dispatch(
		ComputeListID p_list, uint32_t p_x_groups, uint32_t p_y_groups, uint32_t p_z_groups);
	static void compute_list_dispatch_threads(
		ComputeListID p_list, uint32_t p_x_threads, uint32_t p_y_threads, uint32_t p_z_threads);
	static void compute_list_dispatch_indirect(
		ComputeListID p_list, RID p_buffer, uint32_t p_offset);
	static void compute_list_add_barrier(ComputeListID p_list);
	static void compute_list_end();

	static RaytracingListID raytracing_list_begin();
	static void raytracing_list_bind_raytracing_pipeline(
		RaytracingListID p_list, RID p_raytracing_pipeline);
	static void raytracing_list_bind_uniform_set(
		RaytracingListID p_list, RID p_uniform_set, uint32_t p_index);
	static void raytracing_list_set_push_constant(
		RaytracingListID p_list, const void* p_data, uint32_t p_data_size);
	static void raytracing_list_trace_rays(RaytracingListID p_list, uint32_t p_raygen_shader_index,
		RID p_hit_sbt, uint32_t p_width, uint32_t p_height, uint32_t p_depth);
	static void raytracing_list_end();

	// --- Screen Presentation ---
	static Error screen_create(
		DisplayServerEnums::WindowID p_screen = DisplayServerEnums::MAIN_WINDOW_ID);
	static Error screen_prepare_for_drawing(
		DisplayServerEnums::WindowID p_screen = DisplayServerEnums::MAIN_WINDOW_ID);
	static int screen_get_width(
		DisplayServerEnums::WindowID p_screen = DisplayServerEnums::MAIN_WINDOW_ID);
	static int screen_get_height(
		DisplayServerEnums::WindowID p_screen = DisplayServerEnums::MAIN_WINDOW_ID);
	static int screen_get_pre_rotation_degrees(
		DisplayServerEnums::WindowID p_screen = DisplayServerEnums::MAIN_WINDOW_ID);
	static FramebufferFormatID screen_get_framebuffer_format(
		DisplayServerEnums::WindowID p_screen = DisplayServerEnums::MAIN_WINDOW_ID);
	static RDC::ColorSpace screen_get_color_space(
		DisplayServerEnums::WindowID p_screen = DisplayServerEnums::MAIN_WINDOW_ID);
	static bool screen_get_hdr_output_supported(
		DisplayServerEnums::WindowID p_screen = DisplayServerEnums::MAIN_WINDOW_ID);
	static Error screen_free(
		DisplayServerEnums::WindowID p_screen = DisplayServerEnums::MAIN_WINDOW_ID);
	static bool is_composite_alpha_supported();

	// --- Diagnostics & Profiling ---
	static Error driver_callback_add(
		RDD::DriverCallback p_callback, void* p_userdata, VectorView<CallbackResource> p_resources);
	static void set_resource_name(RID p_id, const String& p_name);
	static void _draw_command_begin_label(
		String p_label_name, const Color& p_color = Color(1, 1, 1, 1));
	static void draw_command_begin_label(
		const Span<char> p_label_name, const Color& p_color = Color(1, 1, 1, 1));
	static void draw_command_end_label();

	static void capture_timestamp(const String& p_name);
	static uint32_t get_captured_timestamps_count();
	static uint64_t get_captured_timestamps_frame();
	static uint64_t get_captured_timestamp_gpu_time(uint32_t p_index);
	static uint64_t get_captured_timestamp_cpu_time(uint32_t p_index);
	static String get_captured_timestamp_name(uint32_t p_index);

	static uint64_t limit_get(RDC::Limit p_limit);
	static uint64_t get_memory_usage(MemoryType p_type);
	static String get_perf_report();
	static String get_device_vendor_name();
	static String get_device_name();
	static RenderingDeviceEnums::DeviceType get_device_type();
	static String get_device_api_name();
	static String get_device_api_version();
	static String get_device_pipeline_cache_uuid();
	static RDC::DriverWorkarounds get_driver_workarounds();
	static uint64_t get_driver_resource(
		RDC::DriverResource p_resource, RID p_rid = RID(), uint64_t p_index = 0);
	static String get_driver_and_device_memory_report();
	static String get_tracked_object_name(uint32_t p_type_index);
	static uint64_t get_tracked_object_type_count();
	static uint64_t get_driver_total_memory();
	static uint64_t get_driver_allocation_count();
	static uint64_t get_driver_memory_by_object_type(uint32_t p_type);
	static uint64_t get_driver_allocs_by_object_type(uint32_t p_type);
	static uint64_t get_device_total_memory();
	static uint64_t get_device_allocation_count();
	static uint64_t get_device_memory_by_object_type(uint32_t p_type);
	static uint64_t get_device_allocs_by_object_type(uint32_t p_type);

private:
	static void _add_dependency(RID p_id, RID p_depends_on);
	static void _remove_dependency(RID p_id, RID p_depends_on);
	static void _free_dependencies(RID p_id);

	enum StagingRequiredAction
	{
		STAGING_REQUIRED_ACTION_NONE,
		STAGING_REQUIRED_ACTION_FLUSH_AND_STALL_ALL,
		STAGING_REQUIRED_ACTION_STALL_PREVIOUS,
	};

	struct StagingBufferBlock
	{
		RDD::BufferID driver_id;
		uint64_t frame_used = 0;
		uint32_t fill_amount = 0;
		uint8_t* data_ptr = nullptr;
	};

	struct StagingBuffers
	{
		Vector<StagingBufferBlock> blocks;
		int current = 0;
		uint32_t block_size = 0;
		uint64_t max_size = 0;
		uint32_t usage_bits = 0;
		bool used = false;
	};

	static Error _staging_buffer_allocate(StagingBuffers& p_staging_buffers, uint32_t p_amount,
		uint32_t p_required_align, uint32_t& r_alloc_offset, uint32_t& r_alloc_size,
		StagingRequiredAction& r_required_action, bool p_can_segment = true);
	static void _staging_buffer_execute_required_action(
		StagingBuffers& p_staging_buffers, StagingRequiredAction p_required_action);
	static Error _insert_staging_block(StagingBuffers& p_staging_buffers);

	struct Buffer
	{
		RDD::BufferID driver_id;
		uint32_t size = 0;
		uint32_t usage = 0;
		RDG::ResourceTracker* draw_tracker = nullptr;
		int32_t transfer_worker_index = -1;
		uint64_t transfer_worker_operation = 0;
	};

	static Buffer* _get_buffer_from_owner(RID p_buffer);
	static Error _buffer_initialize(
		Buffer* p_buffer, Span<uint8_t> p_data, uint32_t p_required_align = 32);
	static Error _buffer_update(
		Buffer* p_buffer, RID p_buffer_id, uint32_t p_offset, uint32_t p_size, const void* p_data);

	static void update_perf_report();

	struct BufferGetDataRequest
	{
		uint32_t frame_local_index = 0;
		uint32_t frame_local_count = 0;
		uint32_t size = 0;
	};

	struct Texture
	{
		struct SharedFallback
		{
			uint32_t revision = 1;
			RDD::TextureID texture;
			RDG::ResourceTracker* texture_tracker = nullptr;
			RDD::BufferID buffer;
			RDG::ResourceTracker* buffer_tracker = nullptr;
			bool raw_reinterpretation = false;
		};

		RDD::TextureID driver_id;
		RDC::TextureType type = RDC::TEXTURE_TYPE_MAX;
		RDC::DataFormat format = RDC::DATA_FORMAT_MAX;
		RDC::TextureSamples samples = RDC::TEXTURE_SAMPLES_MAX;
		RDC::TextureSliceType slice_type = RDC::TEXTURE_SLICE_MAX;
		Rect2i slice_rect;
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t depth = 0;
		uint32_t layers = 0;
		uint32_t mipmaps = 0;
		uint32_t usage_flags = 0;
		uint32_t base_mipmap = 0;
		uint32_t base_layer = 0;

		Vector<RDC::DataFormat> allowed_shared_formats;
		bool is_resolve_buffer = false;
		bool is_discardable = false;
		bool is_subsampled = false;
		bool has_initial_data = false;
		bool pending_clear = false;

		uint32_t read_aspect_flags = 0;
		uint32_t barrier_aspect_flags = 0;
		bool bound = false;
		RID owner;

		RDG::ResourceTracker* draw_tracker = nullptr;
		HashMap<Rect2i, RDG::ResourceTracker*>* slice_trackers = nullptr;
		SharedFallback* shared_fallback = nullptr;
		int32_t transfer_worker_index = -1;
		uint64_t transfer_worker_operation = 0;

		RDD::TextureSubresourceRange barrier_range() const
		{
			RDD::TextureSubresourceRange r;
			r.aspect = barrier_aspect_flags;
			r.base_mipmap = base_mipmap;
			r.mipmap_count = mipmaps;
			r.base_layer = base_layer;
			r.layer_count = layers;
			return r;
		}

		RDC::TextureFormat texture_format() const
		{
			RDC::TextureFormat tf;
			tf.format = format;
			tf.width = width;
			tf.height = height;
			tf.depth = depth;
			tf.array_layers = layers;
			tf.mipmaps = mipmaps;
			tf.texture_type = type;
			tf.samples = samples;
			tf.usage_bits = usage_flags;
			tf.shareable_formats = allowed_shared_formats;
			tf.is_resolve_buffer = is_resolve_buffer;
			tf.is_discardable = is_discardable;
			return tf;
		}
	};

	static uint32_t _texture_layer_count(Texture* p_texture);
	static uint32_t _texture_alignment(Texture* p_texture);
	static Error _texture_initialize(RID p_texture, uint32_t p_layer, const Vector<uint8_t>& p_data,
		RDD::TextureLayout p_dst_layout, bool p_immediate_flush);
	static void _texture_check_shared_fallback(Texture* p_texture);
	static void _texture_update_shared_fallback(
		RID p_texture_rid, Texture* p_texture, bool p_for_writing);
	static void _texture_free_shared_fallback(Texture* p_texture);
	static void _texture_copy_shared(RID p_src_texture_rid, Texture* p_src_texture,
		RID p_dst_texture_rid, Texture* p_dst_texture);
	static void _texture_create_reinterpret_buffer(Texture* p_texture);
	static void _texture_clear_color(RID p_texture_rid, Texture* p_texture, const Color& p_color,
		uint32_t p_base_mipmap, uint32_t p_mipmaps, uint32_t p_base_layer, uint32_t p_layers);
	static uint32_t _texture_vrs_method_to_usage_bits();

	struct TextureGetDataRequest
	{
		uint32_t frame_local_index = 0;
		uint32_t frame_local_count = 0;
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t depth = 0;
		uint32_t mipmaps = 0;
		RDC::DataFormat format = RDC::DATA_FORMAT_MAX;
	};

	static RDG::ResourceUsage _vrs_usage_from_method(VRSMethod p_method);
	static RDD::PipelineStageBits _vrs_stages_from_method(VRSMethod p_method);
	static RDD::TextureLayout _vrs_layout_from_method(VRSMethod p_method);
	static void _vrs_detect_method();

	struct FramebufferFormatKey
	{
		Vector<AttachmentFormat> attachments;
		Vector<FramebufferPass> passes;
		uint32_t view_count = 1;
		VRSMethod vrs_method = VRS_METHOD_NONE;
		int32_t vrs_attachment = RDC::ATTACHMENT_UNUSED;
		Size2i vrs_texel_size;

		bool operator<(const FramebufferFormatKey& p_key) const
		{
			if (vrs_texel_size != p_key.vrs_texel_size) {
				return vrs_texel_size < p_key.vrs_texel_size;
			}
			if (vrs_attachment != p_key.vrs_attachment) {
				return vrs_attachment < p_key.vrs_attachment;
			}
			if (vrs_method != p_key.vrs_method) {
				return vrs_method < p_key.vrs_method;
			}
			if (view_count != p_key.view_count) {
				return view_count < p_key.view_count;
			}
			uint32_t pass_size = passes.size();
			uint32_t key_pass_size = p_key.passes.size();
			if (pass_size != key_pass_size) {
				return pass_size < key_pass_size;
			}
			const FramebufferPass* pass_ptr = passes.ptr();
			const FramebufferPass* key_pass_ptr = p_key.passes.ptr();

			for (uint32_t i = 0; i < pass_size; i++) {
				uint32_t attachment_size = pass_ptr[i].color_attachments.size();
				uint32_t key_attachment_size = key_pass_ptr[i].color_attachments.size();
				if (attachment_size != key_attachment_size) {
					return attachment_size < key_attachment_size;
				}
				const int32_t* pass_attachment_ptr = pass_ptr[i].color_attachments.ptr();
				const int32_t* key_pass_attachment_ptr = key_pass_ptr[i].color_attachments.ptr();
				for (uint32_t j = 0; j < attachment_size; j++) {
					if (pass_attachment_ptr[j] != key_pass_attachment_ptr[j]) {
						return pass_attachment_ptr[j] < key_pass_attachment_ptr[j];
					}
				}

				attachment_size = pass_ptr[i].input_attachments.size();
				key_attachment_size = key_pass_ptr[i].input_attachments.size();
				if (attachment_size != key_attachment_size) {
					return attachment_size < key_attachment_size;
				}
				pass_attachment_ptr = pass_ptr[i].input_attachments.ptr();
				key_pass_attachment_ptr = key_pass_ptr[i].input_attachments.ptr();
				for (uint32_t j = 0; j < attachment_size; j++) {
					if (pass_attachment_ptr[j] != key_pass_attachment_ptr[j]) {
						return pass_attachment_ptr[j] < key_pass_attachment_ptr[j];
					}
				}

				attachment_size = pass_ptr[i].resolve_attachments.size();
				key_attachment_size = key_pass_ptr[i].resolve_attachments.size();
				if (attachment_size != key_attachment_size) {
					return attachment_size < key_attachment_size;
				}
				pass_attachment_ptr = pass_ptr[i].resolve_attachments.ptr();
				key_pass_attachment_ptr = key_pass_ptr[i].resolve_attachments.ptr();
				for (uint32_t j = 0; j < attachment_size; j++) {
					if (pass_attachment_ptr[j] != key_pass_attachment_ptr[j]) {
						return pass_attachment_ptr[j] < key_pass_attachment_ptr[j];
					}
				}

				attachment_size = pass_ptr[i].preserve_attachments.size();
				key_attachment_size = key_pass_ptr[i].preserve_attachments.size();
				if (attachment_size != key_attachment_size) {
					return attachment_size < key_attachment_size;
				}
				pass_attachment_ptr = pass_ptr[i].preserve_attachments.ptr();
				key_pass_attachment_ptr = key_pass_ptr[i].preserve_attachments.ptr();
				for (uint32_t j = 0; j < attachment_size; j++) {
					if (pass_attachment_ptr[j] != key_pass_attachment_ptr[j]) {
						return pass_attachment_ptr[j] < key_pass_attachment_ptr[j];
					}
				}

				if (pass_ptr[i].depth_attachment != key_pass_ptr[i].depth_attachment) {
					return pass_ptr[i].depth_attachment < key_pass_ptr[i].depth_attachment;
				}
			}

			int as = attachments.size();
			int bs = p_key.attachments.size();
			if (as != bs) {
				return as < bs;
			}

			const AttachmentFormat* af_a = attachments.ptr();
			const AttachmentFormat* af_b = p_key.attachments.ptr();
			for (int i = 0; i < as; i++) {
				const AttachmentFormat& a = af_a[i];
				const AttachmentFormat& b = af_b[i];
				if (a.format != b.format) {
					return a.format < b.format;
				}
				if (a.samples != b.samples) {
					return a.samples < b.samples;
				}
				if (a.usage_flags != b.usage_flags) {
					return a.usage_flags < b.usage_flags;
				}
			}

			return false;
		}
	};

	static RDD::RenderPassID _render_pass_create(RenderingDeviceDriver* p_driver,
		const Vector<AttachmentFormat>& p_attachments, const Vector<FramebufferPass>& p_passes,
		VectorView<RDD::AttachmentLoadOp> p_load_ops,
		VectorView<RDD::AttachmentStoreOp> p_store_ops, uint32_t p_view_count = 1,
		VRSMethod p_vrs_method = VRS_METHOD_NONE, int32_t p_vrs_attachment = -1,
		Size2i p_vrs_texel_size = Size2i(), Vector<RDC::TextureSamples>* r_samples = nullptr);
	static RDD::RenderPassID _render_pass_create_from_graph(RenderingDeviceDriver* p_driver,
		VectorView<RDD::AttachmentLoadOp> p_load_ops,
		VectorView<RDD::AttachmentStoreOp> p_store_ops, void* p_user_data);

	struct FramebufferFormat
	{
		const RBMap<FramebufferFormatKey, FramebufferFormatID>::Element* E;
		RDD::RenderPassID render_pass;
		Vector<RDC::TextureSamples> pass_samples;
		uint32_t view_count = 1;
	};

	struct Framebuffer
	{
		FramebufferFormatID format_id;
		uint32_t storage_mask = 0;
		Vector<RID> texture_ids;
		InvalidationCallback invalidated_callback = nullptr;
		void* invalidated_callback_userdata = nullptr;
		RDG::FramebufferCache* framebuffer_cache = nullptr;
		Size2 size;
		uint32_t view_count;
	};

	struct VertexDescriptionKey
	{
		Vector<RDC::VertexAttribute> vertex_formats;

		bool operator==(const VertexDescriptionKey& p_key) const
		{
			int vdc = vertex_formats.size();
			int vdck = p_key.vertex_formats.size();
			if (vdc != vdck) {
				return false;
			}
			const RDC::VertexAttribute* a_ptr = vertex_formats.ptr();
			const RDC::VertexAttribute* b_ptr = p_key.vertex_formats.ptr();
			for (int i = 0; i < vdc; i++) {
				const RDC::VertexAttribute& a = a_ptr[i];
				const RDC::VertexAttribute& b = b_ptr[i];
				if (a.location != b.location || a.offset != b.offset || a.format != b.format ||
					a.stride != b.stride || a.frequency != b.frequency) {
					return false;
				}
			}
			return true;
		}

		uint32_t hash() const
		{
			int vdc = vertex_formats.size();
			uint32_t h = hash_murmur3_one_32(vdc);
			const RDC::VertexAttribute* ptr = vertex_formats.ptr();
			for (int i = 0; i < vdc; i++) {
				const RDC::VertexAttribute& vd = ptr[i];
				h = hash_murmur3_one_32(vd.location, h);
				h = hash_murmur3_one_32(vd.offset, h);
				h = hash_murmur3_one_32(vd.format, h);
				h = hash_murmur3_one_32(vd.stride, h);
				h = hash_murmur3_one_32(vd.frequency, h);
			}
			return hash_fmix32(h);
		}
	};

	struct VertexDescriptionHash
	{
		static _FORCE_INLINE_ uint32_t hash(const VertexDescriptionKey& p_key)
		{
			return p_key.hash();
		}
	};

	struct VertexDescriptionCache
	{
		Vector<RDC::VertexAttribute> vertex_formats;
		RDC::VertexAttributeBindingsMap bindings;
		RDD::VertexFormatID driver_id;
	};

	struct VertexArray
	{
		RID buffer;
		VertexFormatID description;
		int vertex_count = 0;
		uint32_t max_instances_allowed = 0;
		Vector<RDD::BufferID> buffers;
		Vector<RDG::ResourceTracker*> draw_trackers;
		Vector<uint64_t> offsets;
		Vector<int32_t> transfer_worker_indices;
		Vector<uint64_t> transfer_worker_operations;
		HashSet<RID> untracked_buffers;
	};

	struct IndexBuffer : public Buffer
	{
		uint32_t max_index = 0;
		uint32_t index_count = 0;
		RDC::IndexBufferFormat format = RDC::INDEX_BUFFER_FORMAT_UINT16;
		bool supports_restart_indices = false;
	};

	struct IndexArray
	{
		uint32_t max_index = 0;
		RDD::BufferID driver_id;
		RDG::ResourceTracker* draw_tracker = nullptr;
		uint32_t offset = 0;
		uint32_t indices = 0;
		RDC::IndexBufferFormat format = RDC::INDEX_BUFFER_FORMAT_UINT16;
		bool supports_restart_indices = false;
		int32_t transfer_worker_index = -1;
		uint64_t transfer_worker_operation = 0;
	};

	static uint32_t _creation_to_usage_bits(uint32_t p_creation_bits);

	struct UniformSetFormat
	{
		Vector<RDC::ShaderUniform> uniforms;

		_FORCE_INLINE_ bool operator<(const UniformSetFormat& p_other) const
		{
			if (uniforms.size() != p_other.uniforms.size()) {
				return uniforms.size() < p_other.uniforms.size();
			}
			for (int i = 0; i < uniforms.size(); i++) {
				if (uniforms[i] < p_other.uniforms[i]) {
					return true;
				}
				else if (p_other.uniforms[i] < uniforms[i]) {
					return false;
				}
			}
			return false;
		}
	};

	struct Shader : public RDC::ShaderReflection
	{
		String name;
		RDD::ShaderID driver_id;
		uint32_t layout_hash = 0;
		uint32_t stage_bits = 0;
		Vector<uint32_t> set_formats;
	};

	static String _shader_uniform_debug(RID p_shader, int p_set = -1);

	static const uint32_t MAX_UNIFORM_SETS = 16;
	static const uint32_t MAX_PUSH_CONSTANT_SIZE = 128;

	struct UniformSet
	{
		uint32_t format = 0;
		RID shader_id;
		uint32_t shader_set = 0;
		RDD::UniformSetID driver_id;

		struct AttachableTexture
		{
			uint32_t bind = 0;
			RID texture;
		};

		struct SharedTexture
		{
			uint32_t writing = 0;
			RID texture;
		};

		LocalVector<AttachableTexture> attachable_textures;
		Vector<RDG::ResourceTracker*> draw_trackers;
		Vector<RDG::ResourceUsage> draw_trackers_usage;
		HashMap<RID, RDG::ResourceUsage> untracked_usage;
		LocalVector<SharedTexture> shared_textures_to_update;
		LocalVector<RID> pending_clear_textures;
		Vector<RID> acceleration_structures;
		InvalidationCallback invalidated_callback = nullptr;
		void* invalidated_callback_userdata = nullptr;
	};

	static void _uniform_set_update_shared(UniformSet* p_uniform_set);

	struct RenderPipeline
	{
#ifdef DEBUG_ENABLED
		struct Validation
		{
			FramebufferFormatID framebuffer_format;
			uint32_t render_pass = 0;
			uint32_t dynamic_state = 0;
			VertexFormatID vertex_format;
			bool uses_restart_indices = false;
			uint32_t primitive_minimum = 0;
			uint32_t primitive_divisor = 0;
		} validation;
#endif
		RID shader;
		RDD::ShaderID shader_driver_id;
		uint32_t shader_layout_hash = 0;
		Vector<uint32_t> set_formats;
		RDD::PipelineID driver_id;
		uint32_t stage_bits = 0;
		uint32_t push_constant_size = 0;
	};

	static Vector<uint8_t> _load_pipeline_cache();
	static void _save_pipeline_cache(void* p_data);

	struct ComputePipeline
	{
		RID shader;
		RDD::ShaderID shader_driver_id;
		uint32_t shader_layout_hash = 0;
		Vector<uint32_t> set_formats;
		RDD::PipelineID driver_id;
		uint32_t push_constant_size = 0;
		uint32_t local_group_size[3] = {0, 0, 0};
	};

	struct RaytracingPipeline
	{
		RID layout_defining_shader;
		RDD::ShaderID layout_defining_shader_driver_id;
		uint32_t layout_defining_shader_layout_hash = 0;
		Vector<uint32_t> set_formats;
		RDD::RaytracingPipelineID driver_id;
		uint32_t push_constant_size = 0;
		Buffer sbt_buffer;
		uint32_t raygen_shader_count = 0;
		uint32_t miss_shader_count = 0;
		uint32_t hit_group_count = 0;
	};

	static Error _raytracing_pipeline_create_sbt_buffer(
		RDD::RaytracingPipelineID p_raytracing_pipeline, uint32_t p_raygen_shader_count,
		uint32_t p_miss_shader_count, Buffer& r_sbt_buffer);

	static uint32_t _get_swap_chain_desired_count();

	struct AccelerationStructure
	{
		RDD::AccelerationStructureID driver_id;
		RDC::AccelerationStructureType type = {};
		RDG::ResourceTracker* draw_tracker = nullptr;
		HashSet<RID> acceleration_structure_dependencies;
		bool invalidated = true;
		RDD::BufferID scratch_buffer;

		Vector<RDG::ResourceTracker*> draw_trackers;
		HashSet<RID> untracked_buffers;

		uint32_t max_instance_count = 0;

		struct InstanceBuffer
		{
			RDD::BufferID driver_id;
			uint64_t frame_used = 0;
			uint32_t used_size = 0;
			uint8_t* data_ptr = nullptr;
		};

		LocalVector<InstanceBuffer> instance_buffers;
	};

	static Error _acceleration_structure_scratch_buffer_create(
		AccelerationStructure* p_acceleration_structure);
	static void _blas_remove_tlas_dependencies(AccelerationStructure* p_blas, RID p_blas_id);
	static void _tlas_remove_blas_dependencies(AccelerationStructure* p_tlas, RID p_tlas_id);

	struct HitShaderBindingTable : Buffer
	{
		RID raytracing_pipeline_id;
		RDD::RaytracingPipelineID raytracing_pipeline;
		uint32_t index_offset = 0;
		uint32_t raytracing_pipeline_hit_group_count = 0;
		Vector<uint32_t> hit_group_indices;
		uint32_t used_hit_group_count = 0;
		uint32_t first_dirty_index = UINT32_MAX;
		uint32_t last_dirty_index = 0;

		using FreeList = RBMap<uint32_t, RBSet<uint32_t>>;
		using ReverseFreeList = RBMap<uint32_t, uint32_t>;
		FreeList free_list;
		ReverseFreeList reverse_free_list;
	};

	static RDD::BufferID _hit_sbt_buffer_create(uint32_t p_buffer_size);
	static void _hit_sbt_add_dirty_range(
		HitShaderBindingTable* p_hit_sbt, uint32_t p_offset, uint32_t p_count);

	struct DrawList
	{
		Rect2i viewport;
		bool active = false;

		struct SetState
		{
			uint32_t pipeline_expected_format = 0;
			uint32_t uniform_set_format = 0;
			RDD::UniformSetID uniform_set_driver_id;
			RID uniform_set;
			bool bound = false;
		};

		struct State
		{
			SetState sets[MAX_UNIFORM_SETS];
			uint32_t set_count = 0;
			RID pipeline;
			RID pipeline_shader;
			RDD::ShaderID pipeline_shader_driver_id;
			uint32_t pipeline_shader_layout_hash = 0;
			uint32_t pipeline_push_constant_size = 0;
			RID vertex_array;
			RID index_array;
			uint32_t draw_count = 0;
		} state;

#ifdef DEBUG_ENABLED
		struct Validation
		{
			uint32_t dynamic_state = 0;
			VertexFormatID vertex_format = RDC::INVALID_ID;
			uint32_t vertex_array_size = 0;
			uint32_t vertex_max_instances_allowed = 0xFFFFFFFF;
			bool index_buffer_uses_restart_indices = false;
			uint32_t index_array_count = 0;
			uint32_t index_array_max_index = 0;
			Vector<uint32_t> set_formats;
			Vector<bool> set_bound;
			Vector<RID> set_rids;
			bool pipeline_active = false;
			uint32_t pipeline_dynamic_state = 0;
			VertexFormatID pipeline_vertex_format = RDC::INVALID_ID;
			RID pipeline_shader;
			bool pipeline_uses_restart_indices = false;
			uint32_t pipeline_primitive_divisor = 0;
			uint32_t pipeline_primitive_minimum = 0;
			uint32_t pipeline_push_constant_size = 0;
			bool pipeline_push_constant_supplied = false;
		} validation;
#else
		struct Validation
		{
			uint32_t vertex_array_size = 0;
			uint32_t index_array_count = 0;
		} validation;
#endif
	};

	static void _draw_list_start(const Rect2i& p_viewport);
	static void _draw_list_end(Rect2i* r_last_viewport = nullptr);

	struct RaytracingList
	{
		bool active = false;

		struct SetState
		{
			uint32_t pipeline_expected_format = 0;
			uint32_t uniform_set_format = 0;
			RDD::UniformSetID uniform_set_driver_id;
			RID uniform_set;
			bool bound = false;
		};

		struct State
		{
			SetState sets[MAX_UNIFORM_SETS];
			uint32_t set_count = 0;
			RID pipeline;
			RDD::RaytracingPipelineID pipeline_driver_id;
			RID layout_defining_shader;
			RDD::ShaderID layout_defining_shader_driver_id;
			uint32_t layout_defining_shader_layout_hash = 0;
			uint8_t push_constant_data[MAX_PUSH_CONSTANT_SIZE] = {};
			uint32_t push_constant_size = 0;
			RDD::BufferID sbt_buffer;
			uint32_t raygen_shader_count = 0;
			uint32_t miss_shader_count = 0;
			uint32_t trace_count = 0;
		} state;

#ifdef DEBUG_ENABLED
		struct Validation
		{
			bool active = true;
			Vector<uint32_t> set_formats;
			Vector<bool> set_bound;
			Vector<RID> set_rids;
			bool pipeline_active = false;
			RID pipeline_shader;
			uint32_t invalid_set_from = 0;
			uint32_t pipeline_push_constant_size = 0;
			bool pipeline_push_constant_supplied = false;
		} validation;
#endif
	};

	struct ComputeList
	{
		bool active = false;

		struct SetState
		{
			uint32_t pipeline_expected_format = 0;
			uint32_t uniform_set_format = 0;
			RDD::UniformSetID uniform_set_driver_id;
			RID uniform_set;
			bool bound = false;
		};

		struct State
		{
			SetState sets[MAX_UNIFORM_SETS];
			uint32_t set_count = 0;
			RID pipeline;
			RID pipeline_shader;
			RDD::ShaderID pipeline_shader_driver_id;
			uint32_t pipeline_shader_layout_hash = 0;
			uint32_t local_group_size[3] = {0, 0, 0};
			uint8_t push_constant_data[MAX_PUSH_CONSTANT_SIZE] = {};
			uint32_t push_constant_size = 0;
			uint32_t dispatch_count = 0;
		} state;

#ifdef DEBUG_ENABLED
		struct Validation
		{
			Vector<uint32_t> set_formats;
			Vector<bool> set_bound;
			Vector<RID> set_rids;
			bool pipeline_active = false;
			RID pipeline_shader;
			uint32_t invalid_set_from = 0;
			uint32_t pipeline_push_constant_size = 0;
			bool pipeline_push_constant_supplied = false;
		} validation;
#endif
	};

	struct TransferWorker
	{
		uint32_t index = 0;
		RDD::BufferID staging_buffer;
		uint32_t max_transfer_size = 0;
		uint32_t staging_buffer_size_in_use = 0;
		uint32_t staging_buffer_size_allocated = 0;
		RDD::CommandBufferID command_buffer;
		RDD::CommandPoolID command_pool;
		RDD::FenceID command_fence;
		LocalVector<RDD::TextureBarrier> texture_barriers;
		bool recording = false;
		bool submitted = false;
		BinaryMutex thread_mutex;
		uint64_t operations_processed = 0;
		uint64_t operations_submitted = 0;
		uint64_t operations_counter = 0;
		BinaryMutex operations_mutex;
	};

	static TransferWorker* _acquire_transfer_worker(
		uint32_t p_transfer_size, uint32_t p_required_align, uint32_t& r_staging_offset);
	static void _release_transfer_worker(TransferWorker* p_transfer_worker);
	static void _end_transfer_worker(TransferWorker* p_transfer_worker);
	static void _submit_transfer_worker(TransferWorker* p_transfer_worker,
		VectorView<RDD::SemaphoreID> p_signal_semaphores = VectorView<RDD::SemaphoreID>());
	static void _wait_for_transfer_worker(TransferWorker* p_transfer_worker);
	static void _flush_barriers_for_transfer_worker(TransferWorker* p_transfer_worker);
	static void _check_transfer_worker_operation(
		uint32_t p_transfer_worker_index, uint64_t p_transfer_worker_operation);
	static void _check_transfer_worker_buffer(Buffer* p_buffer);
	static void _check_transfer_worker_texture(Texture* p_texture);
	static void _check_transfer_worker_vertex_array(VertexArray* p_vertex_array);
	static void _check_transfer_worker_index_array(IndexArray* p_index_array);
	static void _submit_transfer_workers(
		RDD::CommandBufferID p_draw_command_buffer = RDD::CommandBufferID());
	static void _submit_transfer_barriers(RDD::CommandBufferID p_draw_command_buffer);
	static void _wait_for_transfer_workers();
	static void _free_transfer_workers();

	static bool _texture_make_mutable(Texture* p_texture, RID p_texture_id);
	static bool _buffer_make_mutable(Buffer* p_buffer, RID p_buffer_id);
	static bool _vertex_array_make_mutable(
		VertexArray* p_vertex_array, RID p_resource_id, RDG::ResourceTracker* p_resource_tracker);
	static bool _index_array_make_mutable(
		IndexArray* p_index_array, RDG::ResourceTracker* p_resource_tracker);
	static bool _uniform_set_make_mutable(
		UniformSet* p_uniform_set, RID p_resource_id, RDG::ResourceTracker* p_resource_tracker);
	static bool _acceleration_structure_make_mutable(
		AccelerationStructure* p_acceleration_structure, RID p_resource_id,
		RDG::ResourceTracker* p_resource_tracker);
	static bool _dependency_make_mutable(
		RID p_id, RID p_resource_id, RDG::ResourceTracker* p_resource_tracker);
	static bool _dependencies_make_mutable_recursive(
		RID p_id, RDG::ResourceTracker* p_resource_tracker);
	static bool _dependencies_make_mutable(RID p_id, RDG::ResourceTracker* p_resource_tracker);

	struct Frame
	{
		List<Buffer> buffers_to_dispose_of;
		List<Texture> textures_to_dispose_of;
		List<Framebuffer> framebuffers_to_dispose_of;
		List<RDD::SamplerID> samplers_to_dispose_of;
		List<Shader> shaders_to_dispose_of;
		List<UniformSet> uniform_sets_to_dispose_of;
		List<RenderPipeline> render_pipelines_to_dispose_of;
		List<ComputePipeline> compute_pipelines_to_dispose_of;
		List<AccelerationStructure> acceleration_structures_to_dispose_of;
		List<RaytracingPipeline> raytracing_pipelines_to_dispose_of;

		LocalVector<RDD::BufferID> download_buffer_staging_buffers;
		LocalVector<RDD::BufferCopyRegion> download_buffer_copy_regions;
		LocalVector<BufferGetDataRequest> download_buffer_get_data_requests;

		LocalVector<RDD::BufferID> download_texture_staging_buffers;
		LocalVector<RDD::BufferTextureCopyRegion> download_buffer_texture_copy_regions;
		LocalVector<uint32_t> download_texture_mipmap_offsets;
		LocalVector<TextureGetDataRequest> download_texture_get_data_requests;

		RDD::CommandPoolID command_pool;
		RDD::CommandBufferID command_buffer;
		RDD::SemaphoreID semaphore;
		RDD::FenceID fence;
		bool fence_signaled = false;

		LocalVector<RDD::SemaphoreID> semaphores_to_wait_on;
		LocalVector<RDD::SwapChainID> swap_chains_to_present;
		TightLocalVector<RDD::SemaphoreID> transfer_worker_semaphores;
		RDG::CommandBufferPool command_buffer_pool;

		struct Timestamp
		{
			String description;
			uint64_t value = 0;
		};

		RDD::QueryPoolID timestamp_pool;
		TightLocalVector<String> timestamp_names;
		TightLocalVector<uint64_t> timestamp_cpu_values;
		uint32_t timestamp_count = 0;
		TightLocalVector<String> timestamp_result_names;
		TightLocalVector<uint64_t> timestamp_cpu_result_values;
		TightLocalVector<uint64_t> timestamp_result_values;
		uint32_t timestamp_result_count = 0;
		uint64_t index = 0;
	};

	static void _free_pending_resources(int p_frame);
	static void _stall_for_previous_frames();
	static void _flush_and_stall_for_all_frames();

	static void execute_chained_cmds(bool p_present_swap_chain,
		RenderingDeviceDriver::FenceID p_draw_fence,
		RenderingDeviceDriver::SemaphoreID p_dst_draw_semaphore_to_signal);

private:
	struct Data
	{
		Thread::ID render_thread_id;

		RenderingContextDriver* context = nullptr;
		RenderingDeviceDriver* driver = nullptr;
		RenderingContextDriver::Device device;

		bool local_device_processing = false;
		bool is_main_instance = false;

		HashMap<RID, HashSet<RID>> dependency_map;
		HashMap<RID, HashSet<RID>> reverse_dependency_map;

		StagingBuffers upload_staging_buffers;
		StagingBuffers download_staging_buffers;

		bool descriptor_set_batching = true;
		bool split_swapchain_into_its_own_cmd_buffer = false;
		uint32_t gpu_copy_count = 0;
		uint32_t direct_copy_count = 0;
		uint32_t copy_bytes_count = 0;
		uint32_t prev_gpu_copy_count = 0;
		uint32_t prev_copy_bytes_count = 0;

		RID_Owner<Buffer, true> uniform_buffer_owner;
		RID_Owner<Buffer, true> storage_buffer_owner;
		RID_Owner<Buffer, true> texture_buffer_owner;

		RID_Owner<Texture, true> texture_owner;
		uint32_t texture_upload_region_size_px = 0;
		uint32_t texture_download_region_size_px = 0;

		VRSMethod vrs_method = VRS_METHOD_NONE;
		RDC::DataFormat vrs_format = RDC::DATA_FORMAT_MAX;
		Size2i vrs_texel_size;

		RBMap<FramebufferFormatKey, FramebufferFormatID> framebuffer_format_cache;
		HashMap<FramebufferFormatID, FramebufferFormat> framebuffer_formats;

		RID_Owner<Framebuffer, true> framebuffer_owner;
		RID_Owner<RDD::SamplerID, true> sampler_owner;
		RID_Owner<Buffer, true> vertex_buffer_owner;

		HashMap<VertexDescriptionKey, VertexFormatID, VertexDescriptionHash> vertex_format_cache;
		HashMap<VertexFormatID, VertexDescriptionCache> vertex_formats;

		RID_Owner<VertexArray, true> vertex_array_owner;
		RID_Owner<IndexBuffer, true> index_buffer_owner;
		RID_Owner<IndexArray, true> index_array_owner;

		RBMap<UniformSetFormat, uint32_t> uniform_set_format_cache;
		RID_Owner<Shader, true> shader_owner;

		RID_Owner<UniformSet, true> uniform_set_owner;
		RID_Owner<RenderPipeline, true> render_pipeline_owner;

		bool pipeline_cache_enabled = false;
		size_t pipeline_cache_size = 0;
		String pipeline_cache_file_path;

		RID_Owner<ComputePipeline, true> compute_pipeline_owner;
		RID_Owner<RaytracingPipeline, true> raytracing_pipeline_owner;

		HashMap<DisplayServerEnums::WindowID, RDD::SwapChainID> screen_swap_chains;
		HashMap<DisplayServerEnums::WindowID, RDD::FramebufferID> screen_framebuffers;

		RID_Owner<AccelerationStructure, true> acceleration_structure_owner;
		RID_Owner<HitShaderBindingTable, true> hit_sbt_owner;

		DrawList draw_list;
		uint32_t draw_list_subpass_count = 0;
#ifdef DEBUG_ENABLED
		FramebufferFormatID draw_list_framebuffer_format = RDC::INVALID_ID;
#endif
		uint32_t draw_list_current_subpass = 0;
		LocalVector<RID> draw_list_bound_textures;

		RaytracingList raytracing_list;
		RaytracingList::State raytracing_list_barrier_state;

		ComputeList compute_list;
		ComputeList::State compute_list_barrier_state;

		TightLocalVector<TransferWorker*> transfer_worker_pool;
		uint32_t transfer_worker_pool_size = 0;
		uint32_t transfer_worker_pool_max_size = 1;
		TightLocalVector<uint64_t> transfer_worker_operation_used_by_draw;
		LocalVector<uint32_t> transfer_worker_pool_available_list;
		LocalVector<RDD::TextureBarrier> transfer_worker_pool_texture_barriers;
		BinaryMutex transfer_worker_pool_mutex;
		BinaryMutex transfer_worker_pool_texture_barriers_mutex;
		ConditionVariable transfer_worker_pool_condition;

		RenderingDeviceGraph draw_graph;
		RDD::CommandQueueFamilyID main_queue_family;
		RDD::CommandQueueFamilyID transfer_queue_family;
		RDD::CommandQueueFamilyID present_queue_family;
		RDD::CommandQueueID main_queue;
		RDD::CommandQueueID transfer_queue;
		RDD::CommandQueueID present_queue;

		uint32_t max_timestamp_query_elements = 0;
		int frame = 0;
		TightLocalVector<Frame> frames;
		uint64_t frames_drawn = 0;
		uint32_t frames_pending_resources_for_processing = 0u;

		SafeNumeric<uint64_t> texture_memory;
		SafeNumeric<uint64_t> buffer_memory;

#ifdef DEV_ENABLED
		HashMap<RID, String> resource_names;
#endif
	};

public:
	static inline Data* data = nullptr;
	static void _texture_check_pending_clear(RID p_texture_rid, Texture* p_texture);
	static RID texture_create_shared_from_slice(const TextureView& p_view, RID p_with_texture,
	    uint32_t p_layer, uint32_t p_mipmap, uint32_t p_mipmaps,
	    RDC::TextureSliceType p_slice_type);
	static void _texture_clear_depth_stencil(RID p_texture_rid, Texture* p_texture,
		float p_depth, uint8_t p_stencil, uint32_t p_base_mipmap, uint32_t p_mipmaps,
		uint32_t p_base_layer, uint32_t p_layers);

	static RID texture_create(const RDC::TextureFormat& p_format, const TextureView& p_view,
		const Vector<Vector<uint8_t>>& p_data = {});

	static Error initialize(
		RenderingContextDriver* p_context, RenderingContextDriver::Device p_device);
	static void finalize();

	static bool is_initialized() { return data != nullptr; }

	RenderingDevice() = delete;
	RenderingDevice(const RenderingDevice&) = delete;
	RenderingDevice& operator=(const RenderingDevice&) = delete;
	~RenderingDevice() = delete;
};

using RD = RenderingDevice;

// --- Folded from core/utils.h ---
#include "core/typedefs.h"

namespace Utils
{
	_ALWAYS_INLINE_ constexpr uint32_t set_flag(uint32_t p_value, uint32_t p_flag) { return p_value | p_flag; }
	_ALWAYS_INLINE_ constexpr bool has_flag(uint32_t p_value, uint32_t p_flag) { return (p_value & p_flag) != 0; }
	_ALWAYS_INLINE_ constexpr uint32_t clear_flag(uint32_t p_value, uint32_t p_flag) { return p_value &= ~p_flag; }
}; // namespace Utils

// --- Folded from servers/rendering/rendering_device_binds.h ---
#include "core/io/resource.h"

#define RD_SETGET(m_type, m_member)                                                                \
	void set_##m_member(m_type p_##m_member)                                                       \
	{                                                                                              \
		base.m_member = p_##m_member;                                                              \
	}                                                                                              \
	m_type get_##m_member() const                                                                  \
	{                                                                                              \
		return base.m_member;                                                                      \
	}

#define RD_SETGET_SUB(m_type, m_sub, m_member)                                                     \
	void set_##m_sub##_##m_member(m_type p_##m_member)                                             \
	{                                                                                              \
		base.m_sub.m_member = p_##m_member;                                                        \
	}                                                                                              \
	m_type get_##m_sub##_##m_member() const                                                        \
	{                                                                                              \
		return base.m_sub.m_member;                                                                \
	}

class RDTextureFormat : public RefCounted
{
	friend class RenderingDevice;
	friend class RenderSceneBuffersRD;

	RDC::TextureFormat base;

public:
	RD_SETGET(RDC::DataFormat, format)
	RD_SETGET(uint32_t, width)
	RD_SETGET(uint32_t, height)
	RD_SETGET(uint32_t, depth)
	RD_SETGET(uint32_t, array_layers)
	RD_SETGET(uint32_t, mipmaps)
	RD_SETGET(RDC::TextureType, texture_type)
	RD_SETGET(RDC::TextureSamples, samples)
	RD_SETGET(uint32_t, usage_bits)
	RD_SETGET(bool, is_resolve_buffer)
	RD_SETGET(bool, is_discardable)

	void add_shareable_format(RDC::DataFormat p_format)
	{
		base.shareable_formats.push_back(p_format);
	}

	void remove_shareable_format(RDC::DataFormat p_format)
	{
		base.shareable_formats.erase(p_format);
	}

protected:
};

class RDTextureView : public RefCounted
{
	friend class RenderingDevice;
	friend class RenderSceneBuffersRD;

	RD::TextureView base;

public:
	RD_SETGET(RDC::DataFormat, format_override)
	RD_SETGET(RDC::TextureSwizzle, swizzle_r)
	RD_SETGET(RDC::TextureSwizzle, swizzle_g)
	RD_SETGET(RDC::TextureSwizzle, swizzle_b)
	RD_SETGET(RDC::TextureSwizzle, swizzle_a)
protected:
};

class RDAttachmentFormat : public RefCounted
{
	friend class RenderingDevice;

	RD::AttachmentFormat base;

public:
	RD_SETGET(RDC::DataFormat, format)
	RD_SETGET(RDC::TextureSamples, samples)
	RD_SETGET(uint32_t, usage_flags)
protected:
};

class RDFramebufferPass : public RefCounted
{
	friend class RenderingDevice;
	friend class FramebufferCacheRD;

	RD::FramebufferPass base;

public:
	RD_SETGET(PackedInt32Array, color_attachments)
	RD_SETGET(PackedInt32Array, input_attachments)
	RD_SETGET(PackedInt32Array, resolve_attachments)
	RD_SETGET(PackedInt32Array, preserve_attachments)
	RD_SETGET(int32_t, depth_attachment)
protected:
	enum
	{
		ATTACHMENT_UNUSED = -1
	};

};

class RDSamplerState : public RefCounted
{
	friend class RenderingDevice;

	RDC::SamplerState base;

public:
	RD_SETGET(RDC::SamplerFilter, mag_filter)
	RD_SETGET(RDC::SamplerFilter, min_filter)
	RD_SETGET(RDC::SamplerFilter, mip_filter)
	RD_SETGET(RDC::SamplerRepeatMode, repeat_u)
	RD_SETGET(RDC::SamplerRepeatMode, repeat_v)
	RD_SETGET(RDC::SamplerRepeatMode, repeat_w)
	RD_SETGET(float, lod_bias)
	RD_SETGET(bool, use_anisotropy)
	RD_SETGET(float, anisotropy_max)
	RD_SETGET(bool, enable_compare)
	RD_SETGET(RDC::CompareOperator, compare_op)
	RD_SETGET(float, min_lod)
	RD_SETGET(float, max_lod)
	RD_SETGET(RDC::SamplerBorderColor, border_color)
	RD_SETGET(bool, unnormalized_uvw)

protected:
};

class RDVertexAttribute : public RefCounted
{
	friend class RenderingDevice;
	RDC::VertexAttribute base;

public:
	RD_SETGET(uint32_t, binding)
	RD_SETGET(uint32_t, location)
	RD_SETGET(uint32_t, offset)
	RD_SETGET(RDC::DataFormat, format)
	RD_SETGET(uint32_t, stride)
	RD_SETGET(RDC::VertexFrequency, frequency)

protected:
};

class RDShaderSource : public RefCounted
{
	String source[RDC::SHADER_STAGE_MAX];
	RDC::ShaderLanguage language = RDC::SHADER_LANGUAGE_GLSL;

public:
	void set_stage_source(RDC::ShaderStage p_stage, const String& p_source)
	{
		ERR_FAIL_INDEX(p_stage, RDC::SHADER_STAGE_MAX);
		source[p_stage] = p_source;
	}

	String get_stage_source(RDC::ShaderStage p_stage) const
	{
		ERR_FAIL_INDEX_V(p_stage, RDC::SHADER_STAGE_MAX, String());
		return source[p_stage];
	}

	void set_language(RDC::ShaderLanguage p_language) { language = p_language; }

	RDC::ShaderLanguage get_language() const { return language; }

protected:
};

class RDShaderSPIRV : public Resource
{
	Vector<uint8_t> bytecode[RDC::SHADER_STAGE_MAX];
	String compile_error[RDC::SHADER_STAGE_MAX];

public:
	void set_stage_bytecode(RDC::ShaderStage p_stage, const Vector<uint8_t>& p_bytecode)
	{
		ERR_FAIL_INDEX(p_stage, RDC::SHADER_STAGE_MAX);
		bytecode[p_stage] = p_bytecode;
	}

	Vector<uint8_t> get_stage_bytecode(RDC::ShaderStage p_stage) const
	{
		ERR_FAIL_INDEX_V(p_stage, RDC::SHADER_STAGE_MAX, Vector<uint8_t>());
		return bytecode[p_stage];
	}

	Vector<RDC::ShaderStageSPIRVData> get_stages() const
	{
		Vector<RDC::ShaderStageSPIRVData> stages;
		for (int i = 0; i < RDC::SHADER_STAGE_MAX; i++) {
			if (bytecode[i].size()) {
				RDC::ShaderStageSPIRVData stage;
				stage.shader_stage = RDC::ShaderStage(i);
				stage.spirv = bytecode[i];
				stages.push_back(stage);
			}
		}
		return stages;
	}

	void set_stage_compile_error(RDC::ShaderStage p_stage, const String& p_compile_error)
	{
		ERR_FAIL_INDEX(p_stage, RDC::SHADER_STAGE_MAX);
		compile_error[p_stage] = p_compile_error;
	}

	String get_stage_compile_error(RDC::ShaderStage p_stage) const
	{
		ERR_FAIL_INDEX_V(p_stage, RDC::SHADER_STAGE_MAX, String());
		return compile_error[p_stage];
	}

protected:
};

class RDShaderFile : public Resource
{
	HashMap<String, Ref<RDShaderSPIRV>> versions;
	String base_error;

public:
	void set_bytecode(
		const Ref<RDShaderSPIRV>& p_bytecode, const String& p_version = String())
	{
		ERR_FAIL_COND(p_bytecode.is_null());
		versions[p_version] = p_bytecode;
		emit_changed();
	}

	Ref<RDShaderSPIRV> get_spirv(const String& p_version = String()) const
	{
		ERR_FAIL_COND_V(!versions.has(p_version), Ref<RDShaderSPIRV>());
		return versions[p_version];
	}

	Vector<RDC::ShaderStageSPIRVData> get_spirv_stages(
		const String& p_version = String()) const
	{
		ERR_FAIL_COND_V(!versions.has(p_version), Vector<RDC::ShaderStageSPIRVData>());
		return versions[p_version]->get_stages();
	}

	void set_base_error(const String& p_error)
	{
		base_error = p_error;
		emit_changed();
	}

	String get_base_error() const { return base_error; }

	void print_errors(const String& p_file)
	{
		if (!base_error.is_empty()) {
			ERR_PRINT("Error parsing shader '" + p_file + "':\n\n" + base_error);
		}
		else {
			for (KeyValue<String, Ref<RDShaderSPIRV>>& E : versions) {
				for (int i = 0; i < RDC::SHADER_STAGE_MAX; i++) {
					String error = E.value->get_stage_compile_error(RDC::ShaderStage(i));
					if (!error.is_empty()) {
						static const char* stage_str[RDC::SHADER_STAGE_MAX] = {"vertex", "fragment",
							"tesselation_control", "tesselation_evaluation", "compute"};

						print_error("Error parsing shader '" + p_file + "', version '" +
									String(E.key) + "', stage '" + stage_str[i] + "':\n\n" + error);
					}
				}
			}
		}
	}

	typedef String (*OpenIncludeFunction)(const String&, void* userdata);
	Error parse_versions_from_text(const String& p_text, const String p_defines = String(),
		OpenIncludeFunction p_include_func = nullptr, void* p_include_func_userdata = nullptr);
};

class RDUniform : public RefCounted
{
	friend class RenderingDevice;
	friend class UniformSetCacheRD;
	RD::Uniform base;

public:
	RD_SETGET(RDC::UniformType, uniform_type)
	RD_SETGET(int32_t, binding)

	void add_id(const RID& p_id) { base.append_id(p_id); }

	void clear_ids() { base.clear_ids(); }
};

class RDPipelineSpecializationConstant : public RefCounted
{
	friend class RenderingDevice;

	uint32_t constant_id = 0;

public:
	void set_constant_id(uint32_t p_id) { constant_id = p_id; }

	uint32_t get_constant_id() const { return constant_id; }
};

class RDPipelineRasterizationState : public RefCounted
{
	friend class RenderingDevice;

	RDC::PipelineRasterizationState base;

public:
	RD_SETGET(bool, enable_depth_clamp)
	RD_SETGET(bool, discard_primitives)
	RD_SETGET(bool, wireframe)
	RD_SETGET(RDC::PolygonCullMode, cull_mode)
	RD_SETGET(RDC::PolygonFrontFace, front_face)
	RD_SETGET(bool, depth_bias_enabled)
	RD_SETGET(float, depth_bias_constant_factor)
	RD_SETGET(float, depth_bias_clamp)
	RD_SETGET(float, depth_bias_slope_factor)
	RD_SETGET(float, line_width)
	RD_SETGET(uint32_t, patch_control_points)
};

class RDPipelineMultisampleState : public RefCounted
{
	friend class RenderingDevice;

	RDC::PipelineMultisampleState base;

public:
	RD_SETGET(RDC::TextureSamples, sample_count)
	RD_SETGET(bool, enable_sample_shading)
	RD_SETGET(float, min_sample_shading)
	RD_SETGET(bool, enable_alpha_to_coverage)
	RD_SETGET(bool, enable_alpha_to_one)

protected:
};

class RDPipelineDepthStencilState : public RefCounted
{
	friend class RenderingDevice;

	RDC::PipelineDepthStencilState base;

public:
	RD_SETGET(bool, enable_depth_test)
	RD_SETGET(bool, enable_depth_write)
	RD_SETGET(RDC::CompareOperator, depth_compare_operator)
	RD_SETGET(bool, enable_depth_range)
	RD_SETGET(float, depth_range_min)
	RD_SETGET(float, depth_range_max)
	RD_SETGET(bool, enable_stencil)

	RD_SETGET_SUB(RDC::StencilOperation, front_op, fail)
	RD_SETGET_SUB(RDC::StencilOperation, front_op, pass)
	RD_SETGET_SUB(RDC::StencilOperation, front_op, depth_fail)
	RD_SETGET_SUB(RDC::CompareOperator, front_op, compare)
	RD_SETGET_SUB(uint32_t, front_op, compare_mask)
	RD_SETGET_SUB(uint32_t, front_op, write_mask)
	RD_SETGET_SUB(uint32_t, front_op, reference)

	RD_SETGET_SUB(RDC::StencilOperation, back_op, fail)
	RD_SETGET_SUB(RDC::StencilOperation, back_op, pass)
	RD_SETGET_SUB(RDC::StencilOperation, back_op, depth_fail)
	RD_SETGET_SUB(RDC::CompareOperator, back_op, compare)
	RD_SETGET_SUB(uint32_t, back_op, compare_mask)
	RD_SETGET_SUB(uint32_t, back_op, write_mask)
	RD_SETGET_SUB(uint32_t, back_op, reference)

protected:
};

class RDPipelineColorBlendStateAttachment : public RefCounted
{
	friend class RenderingDevice;
	RDC::PipelineColorBlendState::Attachment base;

public:
	RD_SETGET(bool, enable_blend)
	RD_SETGET(RDC::BlendFactor, src_color_blend_factor)
	RD_SETGET(RDC::BlendFactor, dst_color_blend_factor)
	RD_SETGET(RDC::BlendOperation, color_blend_op)
	RD_SETGET(RDC::BlendFactor, src_alpha_blend_factor)
	RD_SETGET(RDC::BlendFactor, dst_alpha_blend_factor)
	RD_SETGET(RDC::BlendOperation, alpha_blend_op)
	RD_SETGET(bool, write_r)
	RD_SETGET(bool, write_g)
	RD_SETGET(bool, write_b)
	RD_SETGET(bool, write_a)

	void set_as_mix()
	{
		base = RDC::PipelineColorBlendState::Attachment();
		base.enable_blend = true;
		base.src_color_blend_factor = RDC::BLEND_FACTOR_SRC_ALPHA;
		base.dst_color_blend_factor = RDC::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		base.src_alpha_blend_factor = RDC::BLEND_FACTOR_SRC_ALPHA;
		base.dst_alpha_blend_factor = RDC::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	}

protected:
};

class RDPipelineColorBlendState : public RefCounted
{
	friend class RenderingDevice;
	RDC::PipelineColorBlendState base;

public:
	RD_SETGET(bool, enable_logic_op)
	RD_SETGET(RDC::LogicOperation, logic_op)
	RD_SETGET(Color, blend_constant)

protected:
};

class RDAccelerationStructureGeometry : public RefCounted
{
	friend class RenderingDevice;
	RD::AccelerationStructureGeometry base;

public:
	RD_SETGET(uint32_t, flags)
	RD_SETGET(RID, vertex_buffer)
	RD_SETGET(uint32_t, vertex_offset)
	RD_SETGET(uint32_t, vertex_stride)
	RD_SETGET(uint32_t, vertex_count)
	RD_SETGET(RDC::DataFormat, vertex_format)
	RD_SETGET(RID, index_buffer)
	RD_SETGET(uint32_t, index_offset)
	RD_SETGET(uint32_t, index_count)

protected:
};

class RDAccelerationStructureInstance : public RefCounted
{
	friend class RenderingDevice;
	RD::AccelerationStructureInstance base;

public:
	RD_SETGET(Transform3D, transform)
	RD_SETGET(uint32_t, id)
	RD_SETGET(uint8_t, mask)
	RD_SETGET(RD::HitShaderBindingTableRange, hit_sbt_range)
	RD_SETGET(uint32_t, flags)
	RD_SETGET(RID, blas)

protected:
};

class RDPipelineShader : public RefCounted
{
	friend class RenderingDevice;
	RD::PipelineShader base;

public:
	RD_SETGET(RID, shader)
};

class RDHitGroup : public RefCounted
{
	friend class RenderingDevice;

	Ref<RDPipelineShader> closest_hit_shader;
	Ref<RDPipelineShader> any_hit_shader;
	Ref<RDPipelineShader> intersection_shader;

public:
	void set_closest_hit_shader(const Ref<RDPipelineShader>& p_closest_hit_shader)
	{
		closest_hit_shader = p_closest_hit_shader;
	}

	Ref<RDPipelineShader> get_closest_hit_shader() const { return closest_hit_shader; }

	void set_any_hit_shader(const Ref<RDPipelineShader>& p_any_hit_shader)
	{
		any_hit_shader = p_any_hit_shader;
	}

	Ref<RDPipelineShader> get_any_hit_shader() const { return any_hit_shader; }

	void set_intersection_shader(const Ref<RDPipelineShader>& p_intersection_shader)
	{
		intersection_shader = p_intersection_shader;
	}

	Ref<RDPipelineShader> get_intersection_shader() const { return intersection_shader; }
};

// --- Folded from servers/rendering/rendering_server_types.h ---
#include "core/io/image.h"
#include "core/types.h"
#include "core/math/vector2.h"
#include "core/types.h"
#include "core/string/ustring.h"
#include "core/templates/hashfuncs.h"
#include "core/templates/vector.h"
#include "scene/resources/texture.h"

#include <cstdint>

template <typename T>
class Vector;

namespace RenderingServerTypes {

/* TEXTURE API */

typedef void (*TextureDetectCallback)(void *);
typedef void (*TextureDetectRoughnessCallback)(void *, const String &, RSE::TextureDetectRoughnessChannel);

struct TextureInfo {
	RID texture;
	uint32_t width;
	uint32_t height;
	uint32_t depth;
	Image::Format format;
	int64_t bytes;
	String path;
	RSE::TextureType type;
};

/* SHADER API */

struct ShaderNativeSourceCode {
	struct Version {
		struct Stage {
			String name;
			String code;
		};
		Vector<Stage> stages;
	};
	Vector<Version> versions;
};

/* MESH API */

struct SurfaceData {
	RSE::PrimitiveType primitive = RSE::PRIMITIVE_MAX;

	uint64_t format = RSE::ARRAY_FLAG_FORMAT_CURRENT_VERSION;
	Vector<uint8_t> vertex_data; // Vertex, Normal, Tangent (change with skinning, blendshape).
	Vector<uint8_t> attribute_data; // Color, UV, UV2, Custom0-3.
	Vector<uint8_t> skin_data; // Bone index, Bone weight.
	uint32_t vertex_count = 0;
	Vector<uint8_t> index_data;
	uint32_t index_count = 0;

	AABB aabb;
	struct LOD {
		float edge_length = 0.0f;
		Vector<uint8_t> index_data;
	};
	Vector<LOD> lods;
	Vector<AABB> bone_aabbs;

	// Transforms used in runtime bone AABBs compute.
	// Since bone AABBs is saved in Mesh space, but bones is in Skeleton space.
	Transform3D mesh_to_skeleton_xform;

	Vector<uint8_t> blend_shape_data;

	Vector4 uv_scale;

	RID material;
};

struct MeshInfo {
	RID mesh;
	String path;
	uint32_t vertex_buffer_size = 0;
	uint32_t attribute_buffer_size = 0;
	uint32_t skin_buffer_size = 0;
	uint32_t index_buffer_size = 0;
	uint32_t blend_shape_buffer_size = 0;
	uint32_t lod_index_buffers_size = 0;
	uint64_t vertex_count = 0;
};

/* STATUS INFORMATION */

struct FrameProfileArea {
	String name;
	double gpu_msec;
	double cpu_msec;
};

/* COMPOSITOR */

struct BlitToScreen {
	RID render_target;
	Rect2 src_rect = Rect2(0.0, 0.0, 1.0, 1.0);
	Rect2i dst_rect;

	struct {
		bool use_layer = false;
		uint32_t layer = 0;
	} multi_view;

	struct {
		//lens distorted parameters for VR
		bool apply = false;
		Vector2 eye_center;
		float k1 = 0.0;
		float k2 = 0.0;

		float upscale = 1.0;
		float aspect_ratio = 1.0;
	} lens_distortion;
};

/* BACKGROUND */

// Helper for RSE::SplashStretchMode, put here for convenience.
inline Rect2 get_splash_stretched_screen_rect(const Size2 &p_image_size, const Size2 &p_window_size, RSE::SplashStretchMode p_stretch_mode) {
	Size2 imgsize = p_image_size;
	Rect2 screenrect;
	switch (p_stretch_mode) {
		case RSE::SPLASH_STRETCH_MODE_DISABLED: {
			screenrect.size = imgsize;
			screenrect.position = ((p_window_size - screenrect.size) / 2.0).floor();
		} break;
		case RSE::SPLASH_STRETCH_MODE_KEEP: {
			if (p_window_size.width > p_window_size.height) {
				// Scale horizontally.
				screenrect.size.y = p_window_size.height;
				screenrect.size.x = imgsize.width * p_window_size.height / imgsize.height;
				screenrect.position.x = (p_window_size.width - screenrect.size.x) / 2;
			} else {
				// Scale vertically.
				screenrect.size.x = p_window_size.width;
				screenrect.size.y = imgsize.height * p_window_size.width / imgsize.width;
				screenrect.position.y = (p_window_size.height - screenrect.size.y) / 2;
			}
		} break;
		case RSE::SPLASH_STRETCH_MODE_KEEP_WIDTH: {
			// Scale vertically.
			screenrect.size.x = p_window_size.width;
			screenrect.size.y = imgsize.height * p_window_size.width / imgsize.width;
			screenrect.position.y = (p_window_size.height - screenrect.size.y) / 2;
		} break;
		case RSE::SPLASH_STRETCH_MODE_KEEP_HEIGHT: {
			// Scale horizontally.
			screenrect.size.y = p_window_size.height;
			screenrect.size.x = imgsize.width * p_window_size.height / imgsize.height;
			screenrect.position.x = (p_window_size.width - screenrect.size.x) / 2;
		} break;
		case RSE::SPLASH_STRETCH_MODE_COVER: {
			double window_aspect = (double)p_window_size.width / p_window_size.height;
			double img_aspect = imgsize.width / imgsize.height;

			if (window_aspect > img_aspect) {
				// Scale vertically.
				screenrect.size.x = p_window_size.width;
				screenrect.size.y = imgsize.height * p_window_size.width / imgsize.width;
				screenrect.position.y = (p_window_size.height - screenrect.size.y) / 2;
			} else {
				// Scale horizontally.
				screenrect.size.y = p_window_size.height;
				screenrect.size.x = imgsize.width * p_window_size.height / imgsize.height;
				screenrect.position.x = (p_window_size.width - screenrect.size.x) / 2;
			}
		} break;
		case RSE::SPLASH_STRETCH_MODE_IGNORE: {
			screenrect.size.x = p_window_size.width;
			screenrect.size.y = p_window_size.height;
		} break;
	}
	return screenrect;
}

/* RENDERING METHOD */

struct RenderInfo {
	int info[RSE::VIEWPORT_RENDER_INFO_TYPE_MAX][RSE::VIEWPORT_RENDER_INFO_MAX] = {};
};

} // namespace RenderingServerTypes
// --- End servers/rendering/rendering_server_types.h ---
