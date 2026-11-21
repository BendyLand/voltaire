/**************************************************************************/
/*  rendering_device_commons.cpp                                          */
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

#include "servers/rendering/rendering_device_commons.h"
#include "modules/modules_enabled.gen.h" // For glslang.
#include "servers/rendering/rendering_device_binds.h"
#ifdef MODULE_GLSLANG_ENABLED
#include "modules/glslang/shader_compile.h"
#endif

#include "servers/rendering/shader_include_db.h"
#include "servers/rendering/rendering_device_driver.h"
#include "servers/rendering/rendering_context_driver.h"
#include <thirdparty/spirv-reflect/spirv_reflect.h>
#include "core/io/compression.h"
#include "servers/rendering/rendering_shader_container.h"
#include "servers/rendering/renderer_rd/shader_rd.h"

// --- Merged from servers/rendering/rendering_device_commons.cpp ---

/*****************/
/**** GENERIC ****/
/*****************/

const char* const RenderingDeviceCommons::FORMAT_NAMES[DATA_FORMAT_MAX] = {
	"R4G4_Unorm_Pack8",
	"R4G4B4A4_Unorm_Pack16",
	"B4G4R4A4_Unorm_Pack16",
	"R5G6B5_Unorm_Pack16",
	"B5G6R5_Unorm_Pack16",
	"R5G5B5A1_Unorm_Pack16",
	"B5G5R5A1_Unorm_Pack16",
	"A1R5G5B5_Unorm_Pack16",
	"R8_Unorm",
	"R8_Snorm",
	"R8_Uscaled",
	"R8_Sscaled",
	"R8_Uint",
	"R8_Sint",
	"R8_Srgb",
	"R8G8_Unorm",
	"R8G8_Snorm",
	"R8G8_Uscaled",
	"R8G8_Sscaled",
	"R8G8_Uint",
	"R8G8_Sint",
	"R8G8_Srgb",
	"R8G8B8_Unorm",
	"R8G8B8_Snorm",
	"R8G8B8_Uscaled",
	"R8G8B8_Sscaled",
	"R8G8B8_Uint",
	"R8G8B8_Sint",
	"R8G8B8_Srgb",
	"B8G8R8_Unorm",
	"B8G8R8_Snorm",
	"B8G8R8_Uscaled",
	"B8G8R8_Sscaled",
	"B8G8R8_Uint",
	"B8G8R8_Sint",
	"B8G8R8_Srgb",
	"R8G8B8A8_Unorm",
	"R8G8B8A8_Snorm",
	"R8G8B8A8_Uscaled",
	"R8G8B8A8_Sscaled",
	"R8G8B8A8_Uint",
	"R8G8B8A8_Sint",
	"R8G8B8A8_Srgb",
	"B8G8R8A8_Unorm",
	"B8G8R8A8_Snorm",
	"B8G8R8A8_Uscaled",
	"B8G8R8A8_Sscaled",
	"B8G8R8A8_Uint",
	"B8G8R8A8_Sint",
	"B8G8R8A8_Srgb",
	"A8B8G8R8_Unorm_Pack32",
	"A8B8G8R8_Snorm_Pack32",
	"A8B8G8R8_Uscaled_Pack32",
	"A8B8G8R8_Sscaled_Pack32",
	"A8B8G8R8_Uint_Pack32",
	"A8B8G8R8_Sint_Pack32",
	"A8B8G8R8_Srgb_Pack32",
	"A2R10G10B10_Unorm_Pack32",
	"A2R10G10B10_Snorm_Pack32",
	"A2R10G10B10_Uscaled_Pack32",
	"A2R10G10B10_Sscaled_Pack32",
	"A2R10G10B10_Uint_Pack32",
	"A2R10G10B10_Sint_Pack32",
	"A2B10G10R10_Unorm_Pack32",
	"A2B10G10R10_Snorm_Pack32",
	"A2B10G10R10_Uscaled_Pack32",
	"A2B10G10R10_Sscaled_Pack32",
	"A2B10G10R10_Uint_Pack32",
	"A2B10G10R10_Sint_Pack32",
	"R16_Unorm",
	"R16_Snorm",
	"R16_Uscaled",
	"R16_Sscaled",
	"R16_Uint",
	"R16_Sint",
	"R16_Sfloat",
	"R16G16_Unorm",
	"R16G16_Snorm",
	"R16G16_Uscaled",
	"R16G16_Sscaled",
	"R16G16_Uint",
	"R16G16_Sint",
	"R16G16_Sfloat",
	"R16G16B16_Unorm",
	"R16G16B16_Snorm",
	"R16G16B16_Uscaled",
	"R16G16B16_Sscaled",
	"R16G16B16_Uint",
	"R16G16B16_Sint",
	"R16G16B16_Sfloat",
	"R16G16B16A16_Unorm",
	"R16G16B16A16_Snorm",
	"R16G16B16A16_Uscaled",
	"R16G16B16A16_Sscaled",
	"R16G16B16A16_Uint",
	"R16G16B16A16_Sint",
	"R16G16B16A16_Sfloat",
	"R32_Uint",
	"R32_Sint",
	"R32_Sfloat",
	"R32G32_Uint",
	"R32G32_Sint",
	"R32G32_Sfloat",
	"R32G32B32_Uint",
	"R32G32B32_Sint",
	"R32G32B32_Sfloat",
	"R32G32B32A32_Uint",
	"R32G32B32A32_Sint",
	"R32G32B32A32_Sfloat",
	"R64_Uint",
	"R64_Sint",
	"R64_Sfloat",
	"R64G64_Uint",
	"R64G64_Sint",
	"R64G64_Sfloat",
	"R64G64B64_Uint",
	"R64G64B64_Sint",
	"R64G64B64_Sfloat",
	"R64G64B64A64_Uint",
	"R64G64B64A64_Sint",
	"R64G64B64A64_Sfloat",
	"B10G11R11_Ufloat_Pack32",
	"E5B9G9R9_Ufloat_Pack32",
	"D16_Unorm",
	"X8_D24_Unorm_Pack32",
	"D32_Sfloat",
	"S8_Uint",
	"D16_Unorm_S8_Uint",
	"D24_Unorm_S8_Uint",
	"D32_Sfloat_S8_Uint",
	"Bc1_Rgb_Unorm_Block",
	"Bc1_Rgb_Srgb_Block",
	"Bc1_Rgba_Unorm_Block",
	"Bc1_Rgba_Srgb_Block",
	"Bc2_Unorm_Block",
	"Bc2_Srgb_Block",
	"Bc3_Unorm_Block",
	"Bc3_Srgb_Block",
	"Bc4_Unorm_Block",
	"Bc4_Snorm_Block",
	"Bc5_Unorm_Block",
	"Bc5_Snorm_Block",
	"Bc6H_Ufloat_Block",
	"Bc6H_Sfloat_Block",
	"Bc7_Unorm_Block",
	"Bc7_Srgb_Block",
	"Etc2_R8G8B8_Unorm_Block",
	"Etc2_R8G8B8_Srgb_Block",
	"Etc2_R8G8B8A1_Unorm_Block",
	"Etc2_R8G8B8A1_Srgb_Block",
	"Etc2_R8G8B8A8_Unorm_Block",
	"Etc2_R8G8B8A8_Srgb_Block",
	"Eac_R11_Unorm_Block",
	"Eac_R11_Snorm_Block",
	"Eac_R11G11_Unorm_Block",
	"Eac_R11G11_Snorm_Block",
	"Astc_4X4_Unorm_Block",
	"Astc_4X4_Srgb_Block",
	"Astc_5X4_Unorm_Block",
	"Astc_5X4_Srgb_Block",
	"Astc_5X5_Unorm_Block",
	"Astc_5X5_Srgb_Block",
	"Astc_6X5_Unorm_Block",
	"Astc_6X5_Srgb_Block",
	"Astc_6X6_Unorm_Block",
	"Astc_6X6_Srgb_Block",
	"Astc_8X5_Unorm_Block",
	"Astc_8X5_Srgb_Block",
	"Astc_8X6_Unorm_Block",
	"Astc_8X6_Srgb_Block",
	"Astc_8X8_Unorm_Block",
	"Astc_8X8_Srgb_Block",
	"Astc_10X5_Unorm_Block",
	"Astc_10X5_Srgb_Block",
	"Astc_10X6_Unorm_Block",
	"Astc_10X6_Srgb_Block",
	"Astc_10X8_Unorm_Block",
	"Astc_10X8_Srgb_Block",
	"Astc_10X10_Unorm_Block",
	"Astc_10X10_Srgb_Block",
	"Astc_12X10_Unorm_Block",
	"Astc_12X10_Srgb_Block",
	"Astc_12X12_Unorm_Block",
	"Astc_12X12_Srgb_Block",
	"G8B8G8R8_422_Unorm",
	"B8G8R8G8_422_Unorm",
	"G8_B8_R8_3Plane_420_Unorm",
	"G8_B8R8_2Plane_420_Unorm",
	"G8_B8_R8_3Plane_422_Unorm",
	"G8_B8R8_2Plane_422_Unorm",
	"G8_B8_R8_3Plane_444_Unorm",
	"R10X6_Unorm_Pack16",
	"R10X6G10X6_Unorm_2Pack16",
	"R10X6G10X6B10X6A10X6_Unorm_4Pack16",
	"G10X6B10X6G10X6R10X6_422_Unorm_4Pack16",
	"B10X6G10X6R10X6G10X6_422_Unorm_4Pack16",
	"G10X6_B10X6_R10X6_3Plane_420_Unorm_3Pack16",
	"G10X6_B10X6R10X6_2Plane_420_Unorm_3Pack16",
	"G10X6_B10X6_R10X6_3Plane_422_Unorm_3Pack16",
	"G10X6_B10X6R10X6_2Plane_422_Unorm_3Pack16",
	"G10X6_B10X6_R10X6_3Plane_444_Unorm_3Pack16",
	"R12X4_Unorm_Pack16",
	"R12X4G12X4_Unorm_2Pack16",
	"R12X4G12X4B12X4A12X4_Unorm_4Pack16",
	"G12X4B12X4G12X4R12X4_422_Unorm_4Pack16",
	"B12X4G12X4R12X4G12X4_422_Unorm_4Pack16",
	"G12X4_B12X4_R12X4_3Plane_420_Unorm_3Pack16",
	"G12X4_B12X4R12X4_2Plane_420_Unorm_3Pack16",
	"G12X4_B12X4_R12X4_3Plane_422_Unorm_3Pack16",
	"G12X4_B12X4R12X4_2Plane_422_Unorm_3Pack16",
	"G12X4_B12X4_R12X4_3Plane_444_Unorm_3Pack16",
	"G16B16G16R16_422_Unorm",
	"B16G16R16G16_422_Unorm",
	"G16_B16_R16_3Plane_420_Unorm",
	"G16_B16R16_2Plane_420_Unorm",
	"G16_B16_R16_3Plane_422_Unorm",
	"G16_B16R16_2Plane_422_Unorm",
	"G16_B16_R16_3Plane_444_Unorm",
	"Astc_4X4_Sfloat_Block",
	"Astc_5X4_Sfloat_Block",
	"Astc_5X5_Sfloat_Block",
	"Astc_6X5_Sfloat_Block",
	"Astc_6X6_Sfloat_Block",
	"Astc_8X5_Sfloat_Block",
	"Astc_8X6_Sfloat_Block",
	"Astc_8X8_Sfloat_Block",
	"Astc_10X5_Sfloat_Block",
	"Astc_10X6_Sfloat_Block",
	"Astc_10X8_Sfloat_Block",
	"Astc_10X10_Sfloat_Block",
	"Astc_12X10_Sfloat_Block",
	"Astc_12X12_Sfloat_Block",
};

/*****************/
/**** TEXTURE ****/
/*****************/

const char* const RenderingDeviceCommons::TEXTURE_TYPE_NAMES[TEXTURE_TYPE_MAX] = {
	"1D",
	"2D",
	"3D",
	"Cube",
	"1DArray",
	"2DArray",
	"CubeArray",
};

const uint32_t RenderingDeviceCommons::TEXTURE_SAMPLES_COUNT[TEXTURE_SAMPLES_MAX] = {
	1, 2, 4, 8, 16, 32, 64};

uint32_t RenderingDeviceCommons::get_image_format_pixel_size(DataFormat p_format)
{
	switch (p_format) {
	case DATA_FORMAT_R4G4_UNORM_PACK8:
		return 1;
	case DATA_FORMAT_R4G4B4A4_UNORM_PACK16:
	case DATA_FORMAT_B4G4R4A4_UNORM_PACK16:
	case DATA_FORMAT_R5G6B5_UNORM_PACK16:
	case DATA_FORMAT_B5G6R5_UNORM_PACK16:
	case DATA_FORMAT_R5G5B5A1_UNORM_PACK16:
	case DATA_FORMAT_B5G5R5A1_UNORM_PACK16:
	case DATA_FORMAT_A1R5G5B5_UNORM_PACK16:
		return 2;
	case DATA_FORMAT_R8_UNORM:
	case DATA_FORMAT_R8_SNORM:
	case DATA_FORMAT_R8_USCALED:
	case DATA_FORMAT_R8_SSCALED:
	case DATA_FORMAT_R8_UINT:
	case DATA_FORMAT_R8_SINT:
	case DATA_FORMAT_R8_SRGB:
		return 1;
	case DATA_FORMAT_R8G8_UNORM:
	case DATA_FORMAT_R8G8_SNORM:
	case DATA_FORMAT_R8G8_USCALED:
	case DATA_FORMAT_R8G8_SSCALED:
	case DATA_FORMAT_R8G8_UINT:
	case DATA_FORMAT_R8G8_SINT:
	case DATA_FORMAT_R8G8_SRGB:
		return 2;
	case DATA_FORMAT_R8G8B8_UNORM:
	case DATA_FORMAT_R8G8B8_SNORM:
	case DATA_FORMAT_R8G8B8_USCALED:
	case DATA_FORMAT_R8G8B8_SSCALED:
	case DATA_FORMAT_R8G8B8_UINT:
	case DATA_FORMAT_R8G8B8_SINT:
	case DATA_FORMAT_R8G8B8_SRGB:
	case DATA_FORMAT_B8G8R8_UNORM:
	case DATA_FORMAT_B8G8R8_SNORM:
	case DATA_FORMAT_B8G8R8_USCALED:
	case DATA_FORMAT_B8G8R8_SSCALED:
	case DATA_FORMAT_B8G8R8_UINT:
	case DATA_FORMAT_B8G8R8_SINT:
	case DATA_FORMAT_B8G8R8_SRGB:
		return 3;
	case DATA_FORMAT_R8G8B8A8_UNORM:
	case DATA_FORMAT_R8G8B8A8_SNORM:
	case DATA_FORMAT_R8G8B8A8_USCALED:
	case DATA_FORMAT_R8G8B8A8_SSCALED:
	case DATA_FORMAT_R8G8B8A8_UINT:
	case DATA_FORMAT_R8G8B8A8_SINT:
	case DATA_FORMAT_R8G8B8A8_SRGB:
	case DATA_FORMAT_B8G8R8A8_UNORM:
	case DATA_FORMAT_B8G8R8A8_SNORM:
	case DATA_FORMAT_B8G8R8A8_USCALED:
	case DATA_FORMAT_B8G8R8A8_SSCALED:
	case DATA_FORMAT_B8G8R8A8_UINT:
	case DATA_FORMAT_B8G8R8A8_SINT:
	case DATA_FORMAT_B8G8R8A8_SRGB:
		return 4;
	case DATA_FORMAT_A8B8G8R8_UNORM_PACK32:
	case DATA_FORMAT_A8B8G8R8_SNORM_PACK32:
	case DATA_FORMAT_A8B8G8R8_USCALED_PACK32:
	case DATA_FORMAT_A8B8G8R8_SSCALED_PACK32:
	case DATA_FORMAT_A8B8G8R8_UINT_PACK32:
	case DATA_FORMAT_A8B8G8R8_SINT_PACK32:
	case DATA_FORMAT_A8B8G8R8_SRGB_PACK32:
	case DATA_FORMAT_A2R10G10B10_UNORM_PACK32:
	case DATA_FORMAT_A2R10G10B10_SNORM_PACK32:
	case DATA_FORMAT_A2R10G10B10_USCALED_PACK32:
	case DATA_FORMAT_A2R10G10B10_SSCALED_PACK32:
	case DATA_FORMAT_A2R10G10B10_UINT_PACK32:
	case DATA_FORMAT_A2R10G10B10_SINT_PACK32:
	case DATA_FORMAT_A2B10G10R10_UNORM_PACK32:
	case DATA_FORMAT_A2B10G10R10_SNORM_PACK32:
	case DATA_FORMAT_A2B10G10R10_USCALED_PACK32:
	case DATA_FORMAT_A2B10G10R10_SSCALED_PACK32:
	case DATA_FORMAT_A2B10G10R10_UINT_PACK32:
	case DATA_FORMAT_A2B10G10R10_SINT_PACK32:
		return 4;
	case DATA_FORMAT_R16_UNORM:
	case DATA_FORMAT_R16_SNORM:
	case DATA_FORMAT_R16_USCALED:
	case DATA_FORMAT_R16_SSCALED:
	case DATA_FORMAT_R16_UINT:
	case DATA_FORMAT_R16_SINT:
	case DATA_FORMAT_R16_SFLOAT:
		return 2;
	case DATA_FORMAT_R16G16_UNORM:
	case DATA_FORMAT_R16G16_SNORM:
	case DATA_FORMAT_R16G16_USCALED:
	case DATA_FORMAT_R16G16_SSCALED:
	case DATA_FORMAT_R16G16_UINT:
	case DATA_FORMAT_R16G16_SINT:
	case DATA_FORMAT_R16G16_SFLOAT:
		return 4;
	case DATA_FORMAT_R16G16B16_UNORM:
	case DATA_FORMAT_R16G16B16_SNORM:
	case DATA_FORMAT_R16G16B16_USCALED:
	case DATA_FORMAT_R16G16B16_SSCALED:
	case DATA_FORMAT_R16G16B16_UINT:
	case DATA_FORMAT_R16G16B16_SINT:
	case DATA_FORMAT_R16G16B16_SFLOAT:
		return 6;
	case DATA_FORMAT_R16G16B16A16_UNORM:
	case DATA_FORMAT_R16G16B16A16_SNORM:
	case DATA_FORMAT_R16G16B16A16_USCALED:
	case DATA_FORMAT_R16G16B16A16_SSCALED:
	case DATA_FORMAT_R16G16B16A16_UINT:
	case DATA_FORMAT_R16G16B16A16_SINT:
	case DATA_FORMAT_R16G16B16A16_SFLOAT:
		return 8;
	case DATA_FORMAT_R32_UINT:
	case DATA_FORMAT_R32_SINT:
	case DATA_FORMAT_R32_SFLOAT:
		return 4;
	case DATA_FORMAT_R32G32_UINT:
	case DATA_FORMAT_R32G32_SINT:
	case DATA_FORMAT_R32G32_SFLOAT:
		return 8;
	case DATA_FORMAT_R32G32B32_UINT:
	case DATA_FORMAT_R32G32B32_SINT:
	case DATA_FORMAT_R32G32B32_SFLOAT:
		return 12;
	case DATA_FORMAT_R32G32B32A32_UINT:
	case DATA_FORMAT_R32G32B32A32_SINT:
	case DATA_FORMAT_R32G32B32A32_SFLOAT:
		return 16;
	case DATA_FORMAT_R64_UINT:
	case DATA_FORMAT_R64_SINT:
	case DATA_FORMAT_R64_SFLOAT:
		return 8;
	case DATA_FORMAT_R64G64_UINT:
	case DATA_FORMAT_R64G64_SINT:
	case DATA_FORMAT_R64G64_SFLOAT:
		return 16;
	case DATA_FORMAT_R64G64B64_UINT:
	case DATA_FORMAT_R64G64B64_SINT:
	case DATA_FORMAT_R64G64B64_SFLOAT:
		return 24;
	case DATA_FORMAT_R64G64B64A64_UINT:
	case DATA_FORMAT_R64G64B64A64_SINT:
	case DATA_FORMAT_R64G64B64A64_SFLOAT:
		return 32;
	case DATA_FORMAT_B10G11R11_UFLOAT_PACK32:
	case DATA_FORMAT_E5B9G9R9_UFLOAT_PACK32:
		return 4;
	case DATA_FORMAT_D16_UNORM:
		return 2;
	case DATA_FORMAT_X8_D24_UNORM_PACK32:
		return 4;
	case DATA_FORMAT_D32_SFLOAT:
		return 4;
	case DATA_FORMAT_S8_UINT:
		return 1;
	case DATA_FORMAT_D16_UNORM_S8_UINT:
		return 4;
	case DATA_FORMAT_D24_UNORM_S8_UINT:
		return 4;
	case DATA_FORMAT_D32_SFLOAT_S8_UINT:
		return 5; // ?
	case DATA_FORMAT_BC1_RGB_UNORM_BLOCK:
	case DATA_FORMAT_BC1_RGB_SRGB_BLOCK:
	case DATA_FORMAT_BC1_RGBA_UNORM_BLOCK:
	case DATA_FORMAT_BC1_RGBA_SRGB_BLOCK:
	case DATA_FORMAT_BC2_UNORM_BLOCK:
	case DATA_FORMAT_BC2_SRGB_BLOCK:
	case DATA_FORMAT_BC3_UNORM_BLOCK:
	case DATA_FORMAT_BC3_SRGB_BLOCK:
	case DATA_FORMAT_BC4_UNORM_BLOCK:
	case DATA_FORMAT_BC4_SNORM_BLOCK:
	case DATA_FORMAT_BC5_UNORM_BLOCK:
	case DATA_FORMAT_BC5_SNORM_BLOCK:
	case DATA_FORMAT_BC6H_UFLOAT_BLOCK:
	case DATA_FORMAT_BC6H_SFLOAT_BLOCK:
	case DATA_FORMAT_BC7_UNORM_BLOCK:
	case DATA_FORMAT_BC7_SRGB_BLOCK:
		return 1;
	case DATA_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
	case DATA_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
	case DATA_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
	case DATA_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
	case DATA_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
	case DATA_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
		return 1;
	case DATA_FORMAT_EAC_R11_UNORM_BLOCK:
	case DATA_FORMAT_EAC_R11_SNORM_BLOCK:
	case DATA_FORMAT_EAC_R11G11_UNORM_BLOCK:
	case DATA_FORMAT_EAC_R11G11_SNORM_BLOCK:
		return 1;
	case DATA_FORMAT_ASTC_4x4_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_4x4_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_5x4_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_5x4_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_5x5_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_5x5_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_6x5_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_6x5_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_6x6_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_6x6_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_8x5_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_8x5_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_8x6_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_8x6_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_8x8_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_8x8_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_10x5_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_10x5_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_10x6_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_10x6_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_10x8_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_10x8_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_10x10_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_10x10_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_12x10_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_12x10_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_12x12_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_12x12_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_4x4_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_5x4_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_5x5_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_6x5_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_6x6_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_8x5_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_8x6_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_8x8_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_10x5_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_10x6_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_10x8_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_10x10_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_12x10_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_12x12_SFLOAT_BLOCK:
		return 1;
	case DATA_FORMAT_G8B8G8R8_422_UNORM:
	case DATA_FORMAT_B8G8R8G8_422_UNORM:
		return 4;
	case DATA_FORMAT_G8_B8_R8_3PLANE_420_UNORM:
	case DATA_FORMAT_G8_B8R8_2PLANE_420_UNORM:
	case DATA_FORMAT_G8_B8_R8_3PLANE_422_UNORM:
	case DATA_FORMAT_G8_B8R8_2PLANE_422_UNORM:
	case DATA_FORMAT_G8_B8_R8_3PLANE_444_UNORM:
		return 4;
	case DATA_FORMAT_R10X6_UNORM_PACK16:
	case DATA_FORMAT_R10X6G10X6_UNORM_2PACK16:
	case DATA_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16:
	case DATA_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16:
	case DATA_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16:
	case DATA_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
	case DATA_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
	case DATA_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
	case DATA_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
	case DATA_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
	case DATA_FORMAT_R12X4_UNORM_PACK16:
	case DATA_FORMAT_R12X4G12X4_UNORM_2PACK16:
	case DATA_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16:
	case DATA_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16:
	case DATA_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16:
	case DATA_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
	case DATA_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
	case DATA_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
	case DATA_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
	case DATA_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
		return 2;
	case DATA_FORMAT_G16B16G16R16_422_UNORM:
	case DATA_FORMAT_B16G16R16G16_422_UNORM:
	case DATA_FORMAT_G16_B16_R16_3PLANE_420_UNORM:
	case DATA_FORMAT_G16_B16R16_2PLANE_420_UNORM:
	case DATA_FORMAT_G16_B16_R16_3PLANE_422_UNORM:
	case DATA_FORMAT_G16_B16R16_2PLANE_422_UNORM:
	case DATA_FORMAT_G16_B16_R16_3PLANE_444_UNORM:
		return 8;
	default: {
		ERR_PRINT("Format not handled, bug");
	}
	}

	return 1;
}

// https://www.khronos.org/registry/DataFormat/specs/1.1/dataformat.1.1.pdf
void RenderingDeviceCommons::get_compressed_image_format_block_dimensions(
	DataFormat p_format, uint32_t& r_w, uint32_t& r_h)
{
	switch (p_format) {
	case DATA_FORMAT_BC1_RGB_UNORM_BLOCK:
	case DATA_FORMAT_BC1_RGB_SRGB_BLOCK:
	case DATA_FORMAT_BC1_RGBA_UNORM_BLOCK:
	case DATA_FORMAT_BC1_RGBA_SRGB_BLOCK:
	case DATA_FORMAT_BC2_UNORM_BLOCK:
	case DATA_FORMAT_BC2_SRGB_BLOCK:
	case DATA_FORMAT_BC3_UNORM_BLOCK:
	case DATA_FORMAT_BC3_SRGB_BLOCK:
	case DATA_FORMAT_BC4_UNORM_BLOCK:
	case DATA_FORMAT_BC4_SNORM_BLOCK:
	case DATA_FORMAT_BC5_UNORM_BLOCK:
	case DATA_FORMAT_BC5_SNORM_BLOCK:
	case DATA_FORMAT_BC6H_UFLOAT_BLOCK:
	case DATA_FORMAT_BC6H_SFLOAT_BLOCK:
	case DATA_FORMAT_BC7_UNORM_BLOCK:
	case DATA_FORMAT_BC7_SRGB_BLOCK:
	case DATA_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
	case DATA_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
	case DATA_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
	case DATA_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
	case DATA_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
	case DATA_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
	case DATA_FORMAT_EAC_R11_UNORM_BLOCK:
	case DATA_FORMAT_EAC_R11_SNORM_BLOCK:
	case DATA_FORMAT_EAC_R11G11_UNORM_BLOCK:
	case DATA_FORMAT_EAC_R11G11_SNORM_BLOCK:
	case DATA_FORMAT_ASTC_4x4_UNORM_BLOCK: // Again, not sure about astc.
	case DATA_FORMAT_ASTC_4x4_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_4x4_SFLOAT_BLOCK: {
		r_w = 4;
		r_h = 4;
	} break;
	case DATA_FORMAT_ASTC_6x6_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_6x6_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_6x6_SFLOAT_BLOCK: {
		r_w = 6;
		r_h = 6;
	} break;
	case DATA_FORMAT_ASTC_5x4_UNORM_BLOCK: // Unsupported
	case DATA_FORMAT_ASTC_5x4_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_5x4_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_5x5_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_5x5_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_5x5_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_6x5_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_6x5_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_6x5_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_8x5_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_8x5_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_8x5_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_8x6_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_8x6_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_8x6_SFLOAT_BLOCK: {
		r_w = 4;
		r_h = 4;
	} break;
	case DATA_FORMAT_ASTC_8x8_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_8x8_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_8x8_SFLOAT_BLOCK: {
		r_w = 8;
		r_h = 8;
	} break;
	case DATA_FORMAT_ASTC_10x5_UNORM_BLOCK: // Unsupported
	case DATA_FORMAT_ASTC_10x5_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_10x5_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_10x6_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_10x6_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_10x6_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_10x8_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_10x8_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_10x8_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_10x10_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_10x10_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_10x10_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_12x10_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_12x10_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_12x10_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_12x12_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_12x12_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_12x12_SFLOAT_BLOCK:
		r_w = 4;
		r_h = 4;
		return;
	default: {
		r_w = 1;
		r_h = 1;
	}
	}
}

uint32_t RenderingDeviceCommons::get_compressed_image_format_block_byte_size(DataFormat p_format)
{
	switch (p_format) {
	case DATA_FORMAT_BC1_RGB_UNORM_BLOCK:
	case DATA_FORMAT_BC1_RGB_SRGB_BLOCK:
	case DATA_FORMAT_BC1_RGBA_UNORM_BLOCK:
	case DATA_FORMAT_BC1_RGBA_SRGB_BLOCK:
		return 8;
	case DATA_FORMAT_BC2_UNORM_BLOCK:
	case DATA_FORMAT_BC2_SRGB_BLOCK:
		return 16;
	case DATA_FORMAT_BC3_UNORM_BLOCK:
	case DATA_FORMAT_BC3_SRGB_BLOCK:
		return 16;
	case DATA_FORMAT_BC4_UNORM_BLOCK:
	case DATA_FORMAT_BC4_SNORM_BLOCK:
		return 8;
	case DATA_FORMAT_BC5_UNORM_BLOCK:
	case DATA_FORMAT_BC5_SNORM_BLOCK:
		return 16;
	case DATA_FORMAT_BC6H_UFLOAT_BLOCK:
	case DATA_FORMAT_BC6H_SFLOAT_BLOCK:
		return 16;
	case DATA_FORMAT_BC7_UNORM_BLOCK:
	case DATA_FORMAT_BC7_SRGB_BLOCK:
		return 16;
	case DATA_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
	case DATA_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
		return 8;
	case DATA_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
	case DATA_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
		return 8;
	case DATA_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
	case DATA_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
		return 16;
	case DATA_FORMAT_EAC_R11_UNORM_BLOCK:
	case DATA_FORMAT_EAC_R11_SNORM_BLOCK:
		return 8;
	case DATA_FORMAT_EAC_R11G11_UNORM_BLOCK:
	case DATA_FORMAT_EAC_R11G11_SNORM_BLOCK:
		return 16;
	case DATA_FORMAT_ASTC_4x4_UNORM_BLOCK: // Again, not sure about astc.
	case DATA_FORMAT_ASTC_4x4_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_4x4_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_5x4_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_5x4_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_5x4_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_5x5_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_5x5_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_5x5_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_6x5_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_6x5_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_6x5_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_6x6_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_6x6_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_6x6_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_8x5_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_8x5_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_8x5_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_8x6_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_8x6_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_8x6_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_8x8_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_8x8_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_8x8_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_10x5_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_10x5_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_10x5_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_10x6_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_10x6_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_10x6_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_10x8_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_10x8_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_10x8_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_10x10_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_10x10_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_10x10_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_12x10_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_12x10_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_12x10_SFLOAT_BLOCK:
	case DATA_FORMAT_ASTC_12x12_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_12x12_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_12x12_SFLOAT_BLOCK:
		return 16;
	default: {
	}
	}
	return 1;
}

uint32_t RenderingDeviceCommons::get_compressed_image_format_pixels_shifted(
	DataFormat p_format, uint32_t p_pixels)
{
	switch (p_format) {
	case DATA_FORMAT_BC1_RGB_UNORM_BLOCK: // These formats are half byte size, so rshift is 1.
	case DATA_FORMAT_BC1_RGB_SRGB_BLOCK:
	case DATA_FORMAT_BC1_RGBA_UNORM_BLOCK:
	case DATA_FORMAT_BC1_RGBA_SRGB_BLOCK:
	case DATA_FORMAT_BC4_UNORM_BLOCK:
	case DATA_FORMAT_BC4_SNORM_BLOCK:
	case DATA_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
	case DATA_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
	case DATA_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
	case DATA_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
	case DATA_FORMAT_EAC_R11_UNORM_BLOCK:
	case DATA_FORMAT_EAC_R11_SNORM_BLOCK:
		return (p_pixels >> 1);
	case DATA_FORMAT_ASTC_6x6_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_6x6_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_6x6_SFLOAT_BLOCK:
		return (p_pixels * 4) / 9;
	case DATA_FORMAT_ASTC_8x8_SRGB_BLOCK:
	case DATA_FORMAT_ASTC_8x8_UNORM_BLOCK:
	case DATA_FORMAT_ASTC_8x8_SFLOAT_BLOCK:
		return (p_pixels >> 2);
	default:
		return p_pixels;
	}
}

uint32_t RenderingDeviceCommons::get_image_format_required_size(DataFormat p_format,
	uint32_t p_width, uint32_t p_height, uint32_t p_depth, uint32_t p_mipmaps, uint32_t* r_blockw,
	uint32_t* r_blockh, uint32_t* r_depth)
{
	ERR_FAIL_COND_V(p_mipmaps == 0, 0);
	uint32_t w = p_width;
	uint32_t h = p_height;
	uint32_t d = p_depth;

	uint32_t size = 0;

	uint32_t pixel_size = get_image_format_pixel_size(p_format);
	uint32_t blockw = 0;
	uint32_t blockh = 0;
	get_compressed_image_format_block_dimensions(p_format, blockw, blockh);

	for (uint32_t i = 0; i < p_mipmaps; i++) {
		uint32_t bw = STEPIFY(w, blockw);
		uint32_t bh = STEPIFY(h, blockh);

		uint32_t s = bw * bh;

		s *= pixel_size;
		s = get_compressed_image_format_pixels_shifted(p_format, s);
		size += s * d;
		if (r_blockw) {
			*r_blockw = bw;
		}
		if (r_blockh) {
			*r_blockh = bh;
		}
		if (r_depth) {
			*r_depth = d;
		}
		w = MAX(blockw, w >> 1);
		h = MAX(blockh, h >> 1);
		d = MAX(1u, d >> 1);
	}

	return size;
}

uint32_t RenderingDeviceCommons::get_image_required_mipmaps(
	uint32_t p_width, uint32_t p_height, uint32_t p_depth)
{
	// Formats and block size don't really matter here since they can all go down to 1px (even if
	// block is larger).
	uint32_t w = p_width;
	uint32_t h = p_height;
	uint32_t d = p_depth;

	uint32_t mipmaps = 1;

	while (true) {
		if (w == 1 && h == 1 && d == 1) {
			break;
		}

		w = MAX(1u, w >> 1);
		h = MAX(1u, h >> 1);
		d = MAX(1u, d >> 1);

		mipmaps++;
	}

	return mipmaps;
}

bool RenderingDeviceCommons::format_has_depth(DataFormat p_format)
{
	switch (p_format) {
	case DATA_FORMAT_D16_UNORM:
	case DATA_FORMAT_X8_D24_UNORM_PACK32:
	case DATA_FORMAT_D32_SFLOAT:
	case DATA_FORMAT_D16_UNORM_S8_UINT:
	case DATA_FORMAT_D24_UNORM_S8_UINT:
	case DATA_FORMAT_D32_SFLOAT_S8_UINT:
		return true;
	default: {
	}
	}
	return false;
}

bool RenderingDeviceCommons::format_has_stencil(DataFormat p_format)
{
	switch (p_format) {
	case DATA_FORMAT_S8_UINT:
	case DATA_FORMAT_D16_UNORM_S8_UINT:
	case DATA_FORMAT_D24_UNORM_S8_UINT:
	case DATA_FORMAT_D32_SFLOAT_S8_UINT: {
		return true;
	}
	default: {
	}
	}
	return false;
}

uint32_t RenderingDeviceCommons::format_get_plane_count(DataFormat p_format)
{
	uint32_t planes = 1;
	switch (p_format) {
	case DATA_FORMAT_D16_UNORM_S8_UINT:
	case DATA_FORMAT_D24_UNORM_S8_UINT:
	case DATA_FORMAT_D32_SFLOAT_S8_UINT: {
		planes = 2;
		break;
	}
	default: {
	}
	}
	DEV_ASSERT(planes <= MAX_IMAGE_FORMAT_PLANES);
	return planes;
}

/*****************/
/**** SAMPLER ****/
/*****************/

const Color RenderingDeviceCommons::SAMPLER_BORDER_COLOR_VALUE[SAMPLER_BORDER_COLOR_MAX] = {
	Color(0, 0, 0, 0),
	Color(0, 0, 0, 0),
	Color(0, 0, 0, 1),
	Color(0, 0, 0, 1),
	Color(1, 1, 1, 1),
	Color(1, 1, 1, 1),
};

/**********************/
/**** VERTEX ARRAY ****/
/**********************/

uint32_t RenderingDeviceCommons::get_format_vertex_size(DataFormat p_format)
{
	switch (p_format) {
	case DATA_FORMAT_R8_UNORM:
	case DATA_FORMAT_R8_SNORM:
	case DATA_FORMAT_R8_UINT:
	case DATA_FORMAT_R8_SINT:
	case DATA_FORMAT_R8G8_UNORM:
	case DATA_FORMAT_R8G8_SNORM:
	case DATA_FORMAT_R8G8_UINT:
	case DATA_FORMAT_R8G8_SINT:
	case DATA_FORMAT_R8G8B8_UNORM:
	case DATA_FORMAT_R8G8B8_SNORM:
	case DATA_FORMAT_R8G8B8_UINT:
	case DATA_FORMAT_R8G8B8_SINT:
	case DATA_FORMAT_B8G8R8_UNORM:
	case DATA_FORMAT_B8G8R8_SNORM:
	case DATA_FORMAT_B8G8R8_UINT:
	case DATA_FORMAT_B8G8R8_SINT:
	case DATA_FORMAT_R8G8B8A8_UNORM:
	case DATA_FORMAT_R8G8B8A8_SNORM:
	case DATA_FORMAT_R8G8B8A8_UINT:
	case DATA_FORMAT_R8G8B8A8_SINT:
	case DATA_FORMAT_B8G8R8A8_UNORM:
	case DATA_FORMAT_B8G8R8A8_SNORM:
	case DATA_FORMAT_B8G8R8A8_UINT:
	case DATA_FORMAT_B8G8R8A8_SINT:
	case DATA_FORMAT_A2B10G10R10_UNORM_PACK32:
		return 4;
	case DATA_FORMAT_R16_UNORM:
	case DATA_FORMAT_R16_SNORM:
	case DATA_FORMAT_R16_UINT:
	case DATA_FORMAT_R16_SINT:
	case DATA_FORMAT_R16_SFLOAT:
		return 4;
	case DATA_FORMAT_R16G16_UNORM:
	case DATA_FORMAT_R16G16_SNORM:
	case DATA_FORMAT_R16G16_UINT:
	case DATA_FORMAT_R16G16_SINT:
	case DATA_FORMAT_R16G16_SFLOAT:
		return 4;
	case DATA_FORMAT_R16G16B16_UNORM:
	case DATA_FORMAT_R16G16B16_SNORM:
	case DATA_FORMAT_R16G16B16_UINT:
	case DATA_FORMAT_R16G16B16_SINT:
	case DATA_FORMAT_R16G16B16_SFLOAT:
		return 8;
	case DATA_FORMAT_R16G16B16A16_UNORM:
	case DATA_FORMAT_R16G16B16A16_SNORM:
	case DATA_FORMAT_R16G16B16A16_UINT:
	case DATA_FORMAT_R16G16B16A16_SINT:
	case DATA_FORMAT_R16G16B16A16_SFLOAT:
		return 8;
	case DATA_FORMAT_R32_UINT:
	case DATA_FORMAT_R32_SINT:
	case DATA_FORMAT_R32_SFLOAT:
		return 4;
	case DATA_FORMAT_R32G32_UINT:
	case DATA_FORMAT_R32G32_SINT:
	case DATA_FORMAT_R32G32_SFLOAT:
		return 8;
	case DATA_FORMAT_R32G32B32_UINT:
	case DATA_FORMAT_R32G32B32_SINT:
	case DATA_FORMAT_R32G32B32_SFLOAT:
		return 12;
	case DATA_FORMAT_R32G32B32A32_UINT:
	case DATA_FORMAT_R32G32B32A32_SINT:
	case DATA_FORMAT_R32G32B32A32_SFLOAT:
		return 16;
	case DATA_FORMAT_R64_UINT:
	case DATA_FORMAT_R64_SINT:
	case DATA_FORMAT_R64_SFLOAT:
		return 8;
	case DATA_FORMAT_R64G64_UINT:
	case DATA_FORMAT_R64G64_SINT:
	case DATA_FORMAT_R64G64_SFLOAT:
		return 16;
	case DATA_FORMAT_R64G64B64_UINT:
	case DATA_FORMAT_R64G64B64_SINT:
	case DATA_FORMAT_R64G64B64_SFLOAT:
		return 24;
	case DATA_FORMAT_R64G64B64A64_UINT:
	case DATA_FORMAT_R64G64B64A64_SINT:
	case DATA_FORMAT_R64G64B64A64_SFLOAT:
		return 32;
	default:
		return 0;
	}
}

/****************/
/**** SHADER ****/
/****************/

const char* RenderingDeviceCommons::SHADER_STAGE_NAMES[SHADER_STAGE_MAX] = {
	"Vertex",
	"Fragment",
	"TesselationControl",
	"TesselationEvaluation",
	"Compute",
};

// --- Merged from servers/rendering/rendering_device_binds.cpp ---

Error RDShaderFile::parse_versions_from_text(const String& p_text, const String p_defines,
	OpenIncludeFunction p_include_func, void* p_include_func_userdata)
{
	Vector<String> lines = p_text.split("\n");

	bool reading_versions = false;
	bool stage_found[RDC::SHADER_STAGE_MAX] = {};
	RDC::ShaderStage stage = RDC::SHADER_STAGE_MAX;
	static const char* stage_str[RDC::SHADER_STAGE_MAX] = {
		"vertex",
		"fragment",
		"tesselation_control",
		"tesselation_evaluation",
		"compute",
		"raygen",
		"any_hit",
		"closest_hit",
		"miss",
		"intersection",
	};
	String stage_code[RDC::SHADER_STAGE_MAX];
	int stages_found = 0;
	HashMap<String, String> version_texts;

	versions.clear();
	base_error = "";

	for (int lidx = 0; lidx < lines.size(); lidx++) {
		String line = lines[lidx];

		{
			String ls = line.strip_edges();
			if (ls.begins_with("#[") && ls.ends_with("]")) {
				String section = ls.substr(2, ls.length() - 3).strip_edges();
				if (section == "versions") {
					if (stages_found) {
						base_error =
							"Invalid shader file, #[versions] must be the first section found.";
						break;
					}
					reading_versions = true;
				}
				else {
					for (int i = 0; i < RDC::SHADER_STAGE_MAX; i++) {
						if (section == stage_str[i]) {
							if (stage_found[i]) {
								base_error = "Invalid shader file, stage appears twice: " + section;
								break;
							}

							stage_found[i] = true;
							stages_found++;

							stage = RDC::ShaderStage(i);
							reading_versions = false;
							break;
						}
					}

					if (!base_error.is_empty()) {
						break;
					}
				}

				continue;
			}
		}

		if (stage == RDC::SHADER_STAGE_MAX && !line.strip_edges().is_empty()) {
			line = line.strip_edges();
			if (line.begins_with("//") || line.begins_with("/*")) {
				continue; // assuming comment (single line)
			}
		}

		if (reading_versions) {
			String l = line.strip_edges();
			if (!l.is_empty()) {
				if (!l.contains_char('=')) {
					base_error = "Missing `=` in '" + l +
								 "'. Version syntax is `version = \"<defines with C escaping>\";`.";
					break;
				}
				if (!l.contains_char(';')) {
					// We don't require a semicolon per se, but it's needed for clang-format to
					// handle things properly.
					base_error = "Missing `;` in '" + l +
								 "'. Version syntax is `version = \"<defines with C escaping>\";`.";
					break;
				}
				Vector<String> slices = l.get_slicec(';', 0).split("=");
				String version = slices[0].strip_edges();
				if (!version.is_valid_ascii_identifier()) {
					base_error =
						"Version names must be valid identifiers, found '" + version + "' instead.";
					break;
				}
				String define = slices[1].strip_edges();
				if (!define.begins_with("\"") || !define.ends_with("\"")) {
					base_error =
						"Version text must be quoted using \"\", instead found '" + define + "'.";
					break;
				}
				define = "\n" + define.substr(1, define.length() - 2).c_unescape() +
						 "\n"; // Add newline before and after just in case.

				version_texts[version] = define + "\n" + p_defines;
			}
		}
		else {
			if (stage == RDC::SHADER_STAGE_MAX && !line.strip_edges().is_empty()) {
				base_error = "Text was found that does not belong to a valid section: " + line;
				break;
			}

			if (stage != RDC::SHADER_STAGE_MAX) {
				if (line.strip_edges().begins_with("#include")) {
					if (p_include_func) {
						// process include
						String include = line.replace("#include", "").strip_edges();
						if (!include.begins_with("\"") || !include.ends_with("\"")) {
							base_error = "Malformed #include syntax, expected #include \"<path>\", "
										 "found instead: " +
										 include;
							break;
						}
						include = include.substr(1, include.length() - 2).strip_edges();

						String include_code = ShaderIncludeDB::get_built_in_include_file(include);
						if (!include_code.is_empty()) {
							stage_code[stage] += "\n" + include_code + "\n";
						}
						else {
							String include_text = p_include_func(include, p_include_func_userdata);
							if (!include_text.is_empty()) {
								stage_code[stage] += "\n" + include_text + "\n";
							}
							else {
								base_error = "#include failed for file '" + include + "'.";
							}
						}
					}
					else {
						base_error = "#include used, but no include function provided.";
					}
				}
				else {
					stage_code[stage] += line + "\n";
				}
			}
		}
	}

	if (base_error.is_empty()) {
		if (stage_found[RDC::SHADER_STAGE_COMPUTE] && stages_found > 1) {
			ERR_FAIL_V_MSG(ERR_PARSE_ERROR,
				"When writing compute shaders, [compute] mustbe the only stage present.");
		}

		if (version_texts.is_empty()) {
			version_texts[""] = ""; // make sure a default version exists
		}

		bool errors_found = false;

		/* STEP 2, Compile the versions, add to shader file */

		for (const KeyValue<String, String>& E : version_texts) {
			Ref<RDShaderSPIRV> bytecode;
			bytecode.instantiate();

			for (int i = 0; i < RDC::SHADER_STAGE_MAX; i++) {
				String code = stage_code[i];
				if (code.is_empty()) {
					continue;
				}
				code = code.replace("VERSION_DEFINES", E.value);
				String error;
#ifdef MODULE_GLSLANG_ENABLED
				Vector<uint8_t> spirv = compile_glslang_shader(RDC::ShaderStage(i),
					ShaderIncludeDB::parse_include_files(code),
					RDC::SHADER_LANGUAGE_VULKAN_VERSION_1_1, RDC::SHADER_SPIRV_VERSION_1_4, &error);
				bytecode->set_stage_bytecode(RDC::ShaderStage(i), spirv);
#else
				error = "Shader compilation is not supported because glslang was not enabled.";
#endif
				if (!error.is_empty()) {
					error += String() + "\n\nStage '" + stage_str[i] + "' source code: \n\n";
					Vector<String> sclines = code.split("\n");
					for (int j = 0; j < sclines.size(); j++)
 {
						error += itos(j + 1) + "\t\t" + sclines[j] + "\n";
					}
					errors_found = true;
				}
				bytecode->set_stage_compile_error(RDC::ShaderStage(i), error);
			}

			set_bytecode(bytecode, E.key);
		}

		return errors_found ? ERR_PARSE_ERROR : OK;
	}
	else {
		return ERR_PARSE_ERROR;
	}
}

// --- Merged from servers/rendering/rendering_device_driver.cpp ---

/**************/
/**** MISC ****/
/**************/

uint64_t RenderingDeviceDriver::api_trait_get(ApiTrait p_trait) {
	// Sensible canonical defaults.
	switch (p_trait) {
		case API_TRAIT_HONORS_PIPELINE_BARRIERS:
			return 1;
		case API_TRAIT_SHADER_CHANGE_INVALIDATION:
			return SHADER_CHANGE_INVALIDATION_ALL_BOUND_UNIFORM_SETS;
		case API_TRAIT_TEXTURE_TRANSFER_ALIGNMENT:
			return 1;
		case API_TRAIT_TEXTURE_DATA_ROW_PITCH_STEP:
			return 1;
		case API_TRAIT_SECONDARY_VIEWPORT_SCISSOR:
			return 1;
		case API_TRAIT_CLEARS_WITH_COPY_ENGINE:
			return true;
		case API_TRAIT_USE_GENERAL_IN_COPY_QUEUES:
			return false;
		case API_TRAIT_BUFFERS_REQUIRE_TRANSITIONS:
			return false;
		case API_TRAIT_TEXTURE_OUTPUTS_REQUIRE_CLEARS:
			return false;
		default:
			ERR_FAIL_V(0);
	}
}

RenderingDeviceDriver::~RenderingDeviceDriver() {}

// --- Merged from servers/rendering/rendering_context_driver.cpp ---

RenderingContextDriver::~RenderingContextDriver() {}

RenderingContextDriver::SurfaceID RenderingContextDriver::surface_get_from_window(
	DisplayServerEnums::WindowID p_window) const
{
	// for (const KeyValue<DisplayServerEnums::WindowID, SurfaceID>& E : window_surface_map) {
	// 	printf("[MAP ENTRY] key=%d, surface=%llu\n", (int)E.key, (unsigned long long)E.value);
	// }

	HashMap<DisplayServerEnums::WindowID, SurfaceID>::ConstIterator it =
		window_surface_map.find(p_window);
	if (it != window_surface_map.end()) {
		// printf("[MAP HIT] found surface=%llu for window=%d\n", (unsigned long long)it->value,
		// 	(int)p_window);
		return it->value;
	}
	else {
		return SurfaceID();
	}
}

Error RenderingContextDriver::window_create(
	DisplayServerEnums::WindowID p_window, const void* p_platform_data)
{
	SurfaceID surface = surface_create(p_platform_data);
	if (surface != 0) {
		window_surface_map[p_window] = surface;
		return OK;
	}
	else {
		return ERR_CANT_CREATE;
	}
}

void RenderingContextDriver::window_set_size(
	DisplayServerEnums::WindowID p_window, uint32_t p_width, uint32_t p_height)
{
	SurfaceID surface = surface_get_from_window(p_window);
	if (surface) {
		surface_set_size(surface, p_width, p_height);
	}
}

void RenderingContextDriver::window_set_vsync_mode(
	DisplayServerEnums::WindowID p_window, DisplayServerEnums::VSyncMode p_vsync_mode)
{
	SurfaceID surface = surface_get_from_window(p_window);
	if (surface) {
		surface_set_vsync_mode(surface, p_vsync_mode);
	}
}

DisplayServerEnums::VSyncMode RenderingContextDriver::window_get_vsync_mode(
	DisplayServerEnums::WindowID p_window) const
{
	SurfaceID surface = surface_get_from_window(p_window);
	if (surface) {
		return surface_get_vsync_mode(surface);
	}
	else {
		return DisplayServerEnums::VSYNC_DISABLED;
	}
}

void RenderingContextDriver::window_set_hdr_output_enabled(
	DisplayServerEnums::WindowID p_window, bool p_enabled)
{
	SurfaceID surface = surface_get_from_window(p_window);
	if (surface) {
		surface_set_hdr_output_enabled(surface, p_enabled);
	}
}

bool RenderingContextDriver::window_get_hdr_output_enabled(
	DisplayServerEnums::WindowID p_window) const
{
	SurfaceID surface = surface_get_from_window(p_window);
	if (surface) {
		return surface_get_hdr_output_enabled(surface);
	}
	else {
		return false;
	}
}

void RenderingContextDriver::window_set_hdr_output_reference_luminance(
	DisplayServerEnums::WindowID p_window, float p_reference_luminance)
{
	SurfaceID surface = surface_get_from_window(p_window);
	if (surface) {
		surface_set_hdr_output_reference_luminance(surface, p_reference_luminance);
	}
}

float RenderingContextDriver::window_get_hdr_output_reference_luminance(
	DisplayServerEnums::WindowID p_window) const
{
	SurfaceID surface = surface_get_from_window(p_window);
	if (surface) {
		return surface_get_hdr_output_reference_luminance(surface);
	}
	else {
		return 0.0f;
	}
}

void RenderingContextDriver::window_set_hdr_output_max_luminance(
	DisplayServerEnums::WindowID p_window, float p_max_luminance)
{
	SurfaceID surface = surface_get_from_window(p_window);
	if (surface) {
		surface_set_hdr_output_max_luminance(surface, p_max_luminance);
	}
}

float RenderingContextDriver::window_get_hdr_output_max_luminance(
	DisplayServerEnums::WindowID p_window) const
{
	SurfaceID surface = surface_get_from_window(p_window);
	if (surface) {
		return surface_get_hdr_output_max_luminance(surface);
	}
	else {
		return 0.0f;
	}
}

void RenderingContextDriver::window_set_hdr_output_linear_luminance_scale(
	DisplayServerEnums::WindowID p_window, float p_linear_luminance_scale)
{
	SurfaceID surface = surface_get_from_window(p_window);
	if (surface) {
		surface_set_hdr_output_linear_luminance_scale(surface, p_linear_luminance_scale);
	}
}

float RenderingContextDriver::window_get_hdr_output_linear_luminance_scale(
	DisplayServerEnums::WindowID p_window) const
{
	SurfaceID surface = surface_get_from_window(p_window);
	if (surface) {
		return surface_get_hdr_output_linear_luminance_scale(surface);
	}
	else {
		return 0.0f;
	}
}

float RenderingContextDriver::window_get_output_max_linear_value(
	DisplayServerEnums::WindowID p_window) const
{
	SurfaceID surface = surface_get_from_window(p_window);
	if (surface) {
		if (surface_get_hdr_output_enabled(surface)) {
			return surface_get_hdr_output_max_value(surface);
		}
	}
	return 1.0f; // SDR
}

void RenderingContextDriver::window_destroy(DisplayServerEnums::WindowID p_window)
{
	SurfaceID surface = surface_get_from_window(p_window);
	if (surface) {
		surface_destroy(surface);
	}

	window_surface_map.erase(p_window);
}

String RenderingContextDriver::get_driver_and_device_memory_report() const
{
	String report;

	const uint32_t num_tracked_obj_types = static_cast<uint32_t>(get_tracked_object_type_count());

	report += "=== Driver Memory Report ===";

	report += "\nLaunch with --extra-gpu-memory-tracking and build with "
			  "DEBUG_ENABLED for this functionality to work.";
	report += "\nDevice memory may be unavailable if the API does not support it"
			  "(e.g. VK_EXT_device_memory_report is unsupported).";
	report += "\n";

	report += "\nTotal Driver Memory:";
	report += String::num_real(double(get_driver_total_memory()) / (1024.0 * 1024.0));
	report += " MB";
	report += "\nTotal Driver Num Allocations: ";
	report += String::num_uint64(get_driver_allocation_count());

	report += "\nTotal Device Memory:";
	report += String::num_real(double(get_device_total_memory()) / (1024.0 * 1024.0));
	report += " MB";
	report += "\nTotal Device Num Allocations: ";
	report += String::num_uint64(get_device_allocation_count());

	report += "\n\nMemory use by object type (CSV format):";
	report += "\n\nCategory; Driver memory in MB; Driver Allocation Count; "
			  "Device memory in MB; Device Allocation Count";

	for (uint32_t i = 0u; i < num_tracked_obj_types; ++i) {
		report += "\n";
		report += get_tracked_object_name(i);
		report += ";";
		report += String::num_real(double(get_driver_memory_by_object_type(i)) / (1024.0 * 1024.0));
		report += ";";
		report += String::num_uint64(get_driver_allocs_by_object_type(i));
		report += ";";
		report += String::num_real(double(get_device_memory_by_object_type(i)) / (1024.0 * 1024.0));
		report += ";";
		report += String::num_uint64(get_device_allocs_by_object_type(i));
	}

	return report;
}

const char* RenderingContextDriver::get_tracked_object_name(uint32_t p_type_index) const
{
	return "Tracking Unsupported by API";
}

uint64_t RenderingContextDriver::get_tracked_object_type_count() const { return 0; }

uint64_t RenderingContextDriver::get_driver_total_memory() const { return 0; }

uint64_t RenderingContextDriver::get_driver_allocation_count() const { return 0; }

uint64_t RenderingContextDriver::get_driver_memory_by_object_type(uint32_t) const { return 0; }

uint64_t RenderingContextDriver::get_driver_allocs_by_object_type(uint32_t) const { return 0; }

uint64_t RenderingContextDriver::get_device_total_memory() const { return 0; }

uint64_t RenderingContextDriver::get_device_allocation_count() const { return 0; }

uint64_t RenderingContextDriver::get_device_memory_by_object_type(uint32_t) const { return 0; }

uint64_t RenderingContextDriver::get_device_allocs_by_object_type(uint32_t) const { return 0; }

// --- Merged from servers/rendering/rendering_shader_container.cpp ---

static inline uint32_t aligned_to(uint32_t p_size, uint32_t p_alignment)
{
	if (p_size % p_alignment) {
		return p_size + (p_alignment - (p_size % p_alignment));
	}
	else {
		return p_size;
	}
}

template <class T>
const T& RenderingShaderContainer::ReflectSymbol<T>::get_spv_reflect(RDC::ShaderStage p_stage) const
{
	const T* info = _spv_reflect[get_index_for_stage(p_stage)];
	DEV_ASSERT(info != nullptr); // Caller is expected to specify valid shader stages
	return *info;
}

RenderingShaderContainer::ReflectShaderStage::ReflectShaderStage()
{
	_module = memnew(SpvReflectShaderModule);
	memset(_module, 0, sizeof(SpvReflectShaderModule));
}

RenderingShaderContainer::ReflectShaderStage::~ReflectShaderStage()
{
	spvReflectDestroyShaderModule(_module);
	memdelete(_module);
	_module = nullptr;
}

const SpvReflectShaderModule& RenderingShaderContainer::ReflectShaderStage::module() const
{
	return *_module;
}

const Span<uint32_t> RenderingShaderContainer::ReflectShaderStage::spirv() const
{
	return _spirv_data.span().reinterpret<uint32_t>();
}

uint32_t RenderingShaderContainer::_from_bytes_header_extra_data(const uint8_t* p_bytes)
{
	return 0;
}

uint32_t RenderingShaderContainer::_from_bytes_reflection_extra_data(const uint8_t* p_bytes)
{
	return 0;
}

uint32_t RenderingShaderContainer::_from_bytes_reflection_binding_uniform_extra_data_start(
	const uint8_t* p_bytes)
{
	return 0;
}

uint32_t RenderingShaderContainer::_from_bytes_reflection_binding_uniform_extra_data(
	const uint8_t* p_bytes, uint32_t p_index)
{
	return 0;
}

uint32_t RenderingShaderContainer::_from_bytes_reflection_specialization_extra_data_start(
	const uint8_t* p_bytes)
{
	return 0;
}

uint32_t RenderingShaderContainer::_from_bytes_reflection_specialization_extra_data(
	const uint8_t* p_bytes, uint32_t p_index)
{
	return 0;
}

uint32_t RenderingShaderContainer::_from_bytes_shader_extra_data_start(const uint8_t* p_bytes)
{
	return 0;
}

uint32_t RenderingShaderContainer::_from_bytes_shader_extra_data(
	const uint8_t* p_bytes, uint32_t p_index)
{
	return 0;
}

uint32_t RenderingShaderContainer::_from_bytes_footer_extra_data(const uint8_t* p_bytes)
{
	return 0;
}

uint32_t RenderingShaderContainer::_to_bytes_header_extra_data(uint8_t*) const { return 0; }

uint32_t RenderingShaderContainer::_to_bytes_reflection_extra_data(uint8_t*) const { return 0; }

uint32_t RenderingShaderContainer::_to_bytes_reflection_binding_uniform_extra_data(
	uint8_t*, uint32_t) const
{
	return 0;
}

uint32_t RenderingShaderContainer::_to_bytes_reflection_specialization_extra_data(
	uint8_t*, uint32_t) const
{
	return 0;
}

uint32_t RenderingShaderContainer::_to_bytes_shader_extra_data(uint8_t*, uint32_t) const
{
	return 0;
}

uint32_t RenderingShaderContainer::_to_bytes_footer_extra_data(uint8_t*) const { return 0; }

void RenderingShaderContainer::_set_from_shader_reflection_post(const ReflectShader& p_shader)
{
	// Do nothing.
}

static RenderingDeviceCommons::TextureType _spv_image_dim_to_texture_type(
	SpvDim p_dim, bool p_arrayed)
{
	switch (p_dim) {
	case SpvDim1D:
		return p_arrayed ? RenderingDeviceCommons::TEXTURE_TYPE_1D_ARRAY
						 : RenderingDeviceCommons::TEXTURE_TYPE_1D;
	case SpvDim2D:
		return p_arrayed ? RenderingDeviceCommons::TEXTURE_TYPE_2D_ARRAY
						 : RenderingDeviceCommons::TEXTURE_TYPE_2D;
	case SpvDim3D:
		return RenderingDeviceCommons::TEXTURE_TYPE_3D;
	case SpvDimCube:
		return p_arrayed ? RenderingDeviceCommons::TEXTURE_TYPE_CUBE_ARRAY
						 : RenderingDeviceCommons::TEXTURE_TYPE_CUBE;
	default:
		return RenderingDeviceCommons::TEXTURE_TYPE_MAX;
	}
}

static RenderingDeviceCommons::DataFormat _spv_image_format_to_data_format(
	const SpvImageFormat p_format)
{
	using RDC = RenderingDeviceCommons;
	switch (p_format) {
	case SpvImageFormatUnknown:
		return RDC::DATA_FORMAT_MAX;
	case SpvImageFormatRgba32f:
		return RDC::DATA_FORMAT_R32G32B32A32_SFLOAT;
	case SpvImageFormatRgba16f:
		return RDC::DATA_FORMAT_R16G16B16A16_SFLOAT;
	case SpvImageFormatR32f:
		return RDC::DATA_FORMAT_R32_SFLOAT;
	case SpvImageFormatRgba8:
		return RDC::DATA_FORMAT_R8G8B8A8_UNORM;
	case SpvImageFormatRgba8Snorm:
		return RDC::DATA_FORMAT_R8G8B8A8_SNORM;
	case SpvImageFormatRg32f:
		return RDC::DATA_FORMAT_R32G32_SFLOAT;
	case SpvImageFormatRg16f:
		return RDC::DATA_FORMAT_R16G16_SFLOAT;
	case SpvImageFormatR11fG11fB10f:
		return RDC::DATA_FORMAT_B10G11R11_UFLOAT_PACK32;
	case SpvImageFormatR16f:
		return RDC::DATA_FORMAT_R16_SFLOAT;
	case SpvImageFormatRgba16:
		return RDC::DATA_FORMAT_R16G16B16A16_UNORM;
	case SpvImageFormatRgb10A2:
		return RDC::DATA_FORMAT_A2B10G10R10_UNORM_PACK32;
	case SpvImageFormatRg16:
		return RDC::DATA_FORMAT_R16G16_UNORM;
	case SpvImageFormatRg8:
		return RDC::DATA_FORMAT_R8G8_UNORM;
	case SpvImageFormatR16:
		return RDC::DATA_FORMAT_R16_UNORM;
	case SpvImageFormatR8:
		return RDC::DATA_FORMAT_R8_UNORM;
	case SpvImageFormatRgba16Snorm:
		return RDC::DATA_FORMAT_R16G16B16A16_SNORM;
	case SpvImageFormatRg16Snorm:
		return RDC::DATA_FORMAT_R16G16_SNORM;
	case SpvImageFormatRg8Snorm:
		return RDC::DATA_FORMAT_R8G8_SNORM;
	case SpvImageFormatR16Snorm:
		return RDC::DATA_FORMAT_R16_SNORM;
	case SpvImageFormatR8Snorm:
		return RDC::DATA_FORMAT_R8_SNORM;
	case SpvImageFormatRgba32i:
		return RDC::DATA_FORMAT_R32G32B32A32_SINT;
	case SpvImageFormatRgba16i:
		return RDC::DATA_FORMAT_R16G16B16A16_SINT;
	case SpvImageFormatRgba8i:
		return RDC::DATA_FORMAT_R8G8B8A8_SINT;
	case SpvImageFormatR32i:
		return RDC::DATA_FORMAT_R32_SINT;
	case SpvImageFormatRg32i:
		return RDC::DATA_FORMAT_R32G32_SINT;
	case SpvImageFormatRg16i:
		return RDC::DATA_FORMAT_R16G16_SINT;
	case SpvImageFormatRg8i:
		return RDC::DATA_FORMAT_R8G8_SINT;
	case SpvImageFormatR16i:
		return RDC::DATA_FORMAT_R16_SINT;
	case SpvImageFormatR8i:
		return RDC::DATA_FORMAT_R8_SINT;
	case SpvImageFormatRgba32ui:
		return RDC::DATA_FORMAT_R32G32B32A32_UINT;
	case SpvImageFormatRgba16ui:
		return RDC::DATA_FORMAT_R16G16B16A16_UINT;
	case SpvImageFormatRgba8ui:
		return RDC::DATA_FORMAT_R8G8B8A8_UINT;
	case SpvImageFormatR32ui:
		return RDC::DATA_FORMAT_R32_UINT;
	case SpvImageFormatRgb10a2ui:
		return RDC::DATA_FORMAT_A2B10G10R10_UINT_PACK32;
	case SpvImageFormatRg32ui:
		return RDC::DATA_FORMAT_R32G32_UINT;
	case SpvImageFormatRg16ui:
		return RDC::DATA_FORMAT_R16G16_UINT;
	case SpvImageFormatRg8ui:
		return RDC::DATA_FORMAT_R8G8_UINT;
	case SpvImageFormatR16ui:
		return RDC::DATA_FORMAT_R16_UINT;
	case SpvImageFormatR8ui:
		return RDC::DATA_FORMAT_R8_UINT;
	case SpvImageFormatR64ui:
		return RDC::DATA_FORMAT_R64_UINT;
	case SpvImageFormatR64i:
		return RDC::DATA_FORMAT_R64_SINT;
	case SpvImageFormatMax:
		return RDC::DATA_FORMAT_MAX;
	}
	return RDC::DATA_FORMAT_MAX;
}

Error RenderingShaderContainer::reflect_spirv(
	const String& p_shader_name, Span<RDC::ShaderStageSPIRVData> p_spirv, ReflectShader& r_shader)
{
	ReflectShader& reflection = r_shader;

	shader_name = p_shader_name.utf8();

	const uint32_t spirv_size = p_spirv.size() + 0;

	LocalVector<ReflectShaderStage>& r_refl = r_shader.shader_stages;
	r_refl.resize(spirv_size);

	bool pipeline_type_detected = false;
	for (uint32_t i = 0; i < spirv_size; i++) {
		RDC::ShaderStage stage = p_spirv[i].shader_stage;
		RDC::ShaderStage stage_flag = (RDC::ShaderStage)(1 << stage);
		r_refl[i].shader_stage = stage;
		r_refl[i]._spirv_data = p_spirv[i].spirv;

		RDC::PipelineType pipeline_type = {};
		switch (stage) {
		case RDC::SHADER_STAGE_VERTEX:
		case RDC::SHADER_STAGE_FRAGMENT:
		case RDC::SHADER_STAGE_TESSELATION_CONTROL:
		case RDC::SHADER_STAGE_TESSELATION_EVALUATION:
			pipeline_type = RDC::PIPELINE_TYPE_RASTERIZATION;
			break;
		case RDC::SHADER_STAGE_COMPUTE:
			pipeline_type = RDC::PIPELINE_TYPE_COMPUTE;
			break;
		case RDC::SHADER_STAGE_RAYGEN:
		case RDC::SHADER_STAGE_ANY_HIT:
		case RDC::SHADER_STAGE_CLOSEST_HIT:
		case RDC::SHADER_STAGE_MISS:
		case RDC::SHADER_STAGE_INTERSECTION:
			pipeline_type = RDC::PIPELINE_TYPE_RAYTRACING;
			break;
		default:
			DEV_ASSERT(false && "Unknown shader stage.");
		}

		if (pipeline_type_detected) {
			ERR_FAIL_COND_V_MSG(r_shader.pipeline_type != pipeline_type, FAILED,
				"Shader stages of different pipeline types cannot be mixed in the same shader "
				"container.");
		}
		else {
			r_shader.pipeline_type = pipeline_type;
			pipeline_type_detected = true;
		}

		const Vector<uint64_t>& dynamic_buffers = p_spirv[i].dynamic_buffers;

		if (stage == RDC::SHADER_STAGE_COMPUTE) {
			ERR_FAIL_COND_V_MSG(spirv_size != 1, FAILED,
				"Compute shaders can only receive one stage, dedicated to compute.");
		}
		reflection.stages_bits |= (1 << stage);

		// We make all raytracing stages visible to make our lives easier when creating raytracing
		// pipelines. This makes no practical difference in current graphics drivers, since Vulkan
		// is the outlier.
		uint32_t uniform_stage_flags;
		if (pipeline_type == RDC::PIPELINE_TYPE_RAYTRACING) {
			uniform_stage_flags = RDC::SHADER_STAGE_RAYGEN | RDC::SHADER_STAGE_ANY_HIT |
								  RDC::SHADER_STAGE_CLOSEST_HIT | RDC::SHADER_STAGE_MISS |
								  RDC::SHADER_STAGE_INTERSECTION;
		}
		else {
			uniform_stage_flags = stage_flag;
		}

		{
			SpvReflectShaderModule& module = *r_refl.ptr()[i]._module;
			const uint8_t* spirv = p_spirv[i].spirv.ptr();
			SpvReflectResult result = spvReflectCreateShaderModule2(
				SPV_REFLECT_MODULE_FLAG_NO_COPY, p_spirv[i].spirv.size(), spirv, &module);
			ERR_FAIL_COND_V_MSG(result != SPV_REFLECT_RESULT_SUCCESS, FAILED,
				"Reflection of SPIR-V shader stage '" +
					String(RDC::SHADER_STAGE_NAMES[p_spirv[i].shader_stage]) +
					"' failed parsing shader.");

			for (uint32_t j = 0; j < module.capability_count; j++) {
				if (module.capabilities[j].value == SpvCapabilityMultiView) {
					reflection.has_multiview = true;
					break;
				}
			}

			if (reflection.is_compute()) {
				reflection.compute_local_size[0] = module.entry_points->local_size.x;
				reflection.compute_local_size[1] = module.entry_points->local_size.y;
				reflection.compute_local_size[2] = module.entry_points->local_size.z;
			}
			uint32_t binding_count = 0;
			result = spvReflectEnumerateDescriptorBindings(&module, &binding_count, nullptr);
			ERR_FAIL_COND_V_MSG(result != SPV_REFLECT_RESULT_SUCCESS, FAILED,
				"Reflection of SPIR-V shader stage '" +
					String(RDC::SHADER_STAGE_NAMES[p_spirv[i].shader_stage]) +
					"' failed enumerating descriptor bindings.");

			if (binding_count > 0) {
				// Parse bindings.

				Vector<SpvReflectDescriptorBinding*> bindings;
				bindings.resize(binding_count);
				result =
					spvReflectEnumerateDescriptorBindings(&module, &binding_count, bindings.ptrw());

				ERR_FAIL_COND_V_MSG(result != SPV_REFLECT_RESULT_SUCCESS, FAILED,
					"Reflection of SPIR-V shader stage '" +
						String(RDC::SHADER_STAGE_NAMES[p_spirv[i].shader_stage]) +
						"' failed getting descriptor bindings.");

				for (uint32_t j = 0; j < binding_count; j++) {
					const SpvReflectDescriptorBinding& binding = *bindings[j];

					ReflectUniform uniform;
					uniform.set_spv_reflect(stage, &binding);

					bool need_array_dimensions = false;
					bool need_block_size = false;
					bool may_be_writable = false;
					bool is_image = false;

					switch (binding.descriptor_type) {
					case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER: {
						uniform.type = RDC::UNIFORM_TYPE_SAMPLER;
						need_array_dimensions = true;
					} break;
					case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: {
						uniform.type = RDC::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE;
						need_array_dimensions = true;
						is_image = true;
					} break;
					case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE: {
						uniform.type = RDC::UNIFORM_TYPE_TEXTURE;
						need_array_dimensions = true;
						is_image = true;
					} break;
					case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE: {
						uniform.type = RDC::UNIFORM_TYPE_IMAGE;
						need_array_dimensions = true;
						may_be_writable = true;
						is_image = true;
					} break;
					case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER: {
						uniform.type = RDC::UNIFORM_TYPE_TEXTURE_BUFFER;
						need_array_dimensions = true;
						is_image = true;
					} break;
					case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: {
						uniform.type = RDC::UNIFORM_TYPE_IMAGE_BUFFER;
						need_array_dimensions = true;
						may_be_writable = true;
						is_image = true;
					} break;
					case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER: {
						const uint64_t key =
							ShaderRD::DynamicBuffer::encode(binding.set, binding.binding);
						if (dynamic_buffers.has(key)) {
							uniform.type = RDC::UNIFORM_TYPE_UNIFORM_BUFFER_DYNAMIC;
							reflection.has_dynamic_buffers = true;
						}
						else {
							uniform.type = RDC::UNIFORM_TYPE_UNIFORM_BUFFER;
						}
						need_block_size = true;
					} break;
					case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER: {
						const uint64_t key =
							ShaderRD::DynamicBuffer::encode(binding.set, binding.binding);
						if (dynamic_buffers.has(key)) {
							uniform.type = RDC::UNIFORM_TYPE_STORAGE_BUFFER_DYNAMIC;
							reflection.has_dynamic_buffers = true;
						}
						else {
							uniform.type = RDC::UNIFORM_TYPE_STORAGE_BUFFER;
						}
						need_block_size = true;
						may_be_writable = true;
					} break;
					case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: {
						ERR_PRINT("Dynamic uniform buffer not supported.");
						continue;
					} break;
					case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: {
						ERR_PRINT("Dynamic storage buffer not supported.");
						continue;
					} break;
					case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: {
						uniform.type = RDC::UNIFORM_TYPE_INPUT_ATTACHMENT;
						need_array_dimensions = true;
						is_image = true;
					} break;
					case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR: {
						uniform.type = RDC::UNIFORM_TYPE_ACCELERATION_STRUCTURE;
					} break;
					}

					if (need_array_dimensions) {
						uniform.length = 1;
						for (uint32_t k = 0; k < binding.array.dims_count; k++) {
							uniform.length *= binding.array.dims[k];
						}
					}
					else if (need_block_size) {
						uniform.length = binding.block.size;
					}
					else {
						uniform.length = 0;
					}

					if (may_be_writable) {
						if (binding.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE) {
							uniform.writable =
								!(binding.decoration_flags & SPV_REFLECT_DECORATION_NON_WRITABLE);
						}
						else {
							uniform.writable =
								!(binding.decoration_flags & SPV_REFLECT_DECORATION_NON_WRITABLE) &&
								!(binding.block.decoration_flags &
									SPV_REFLECT_DECORATION_NON_WRITABLE);
						}
					}
					else {
						uniform.writable = false;
					}

					// Gather the texture type and format for runtime validation when creating
					// uniform sets. We cannot enforce the type for input attachments since they do
					// not indicate whether the texture is 2D or 2D array for multiview.
					if (is_image &&
						binding.descriptor_type != SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT) {
						uniform.texture_type = _spv_image_dim_to_texture_type(
							binding.image.dim, binding.image.arrayed);
						uniform.texture_format =
							_spv_image_format_to_data_format(binding.image.image_format);

						ERR_FAIL_COND_V_MSG(uniform.texture_type == RDC::TEXTURE_TYPE_MAX, FAILED,
							"On shader stage '" + String(RDC::SHADER_STAGE_NAMES[stage]) +
								"', uniform '" + binding.name +
								"' does not have a valid texture type.");
					}

					uniform.binding = binding.binding;
					uint32_t set = binding.set;

					ERR_FAIL_COND_V_MSG(set >= RDC::MAX_UNIFORM_SETS, FAILED,
						"On shader stage '" + String(RDC::SHADER_STAGE_NAMES[stage]) +
							"', uniform '" + binding.name + "' uses a set (" + itos(set) +
							") index larger than what is supported (" +
							itos(RDC::MAX_UNIFORM_SETS) + ").");

					if (set < (uint32_t)reflection.uniform_sets.size()) {
						// Check if this already exists.
						bool exists = false;
						for (uint32_t k = 0; k < reflection.uniform_sets[set].size(); k++) {
							if (reflection.uniform_sets[set][k].binding == uniform.binding) {
								// Already exists, verify that it's the same type.
								ERR_FAIL_COND_V_MSG(
									reflection.uniform_sets[set][k].type != uniform.type, FAILED,
									"On shader stage '" + String(RDC::SHADER_STAGE_NAMES[stage]) +
										"', uniform '" + binding.name +
										"' trying to reuse location for set=" + itos(set) +
										", binding=" + itos(uniform.binding) +
										" with different uniform type.");

								// Also, verify that it's the same size.
								ERR_FAIL_COND_V_MSG(
									reflection.uniform_sets[set][k].length != uniform.length,
									FAILED,
									"On shader stage '" + String(RDC::SHADER_STAGE_NAMES[stage]) +
										"', uniform '" + binding.name +
										"' trying to reuse location for set=" + itos(set) +
										", binding=" + itos(uniform.binding) +
										" with different uniform size.");

								// Also, verify that it has the same writability.
								ERR_FAIL_COND_V_MSG(
									reflection.uniform_sets[set][k].writable != uniform.writable,
									FAILED,
									"On shader stage '" + String(RDC::SHADER_STAGE_NAMES[stage]) +
										"', uniform '" + binding.name +
										"' trying to reuse location for set=" + itos(set) +
										", binding=" + itos(uniform.binding) +
										" with different writability.");

								// Just append stage mask and return.
								reflection.uniform_sets[set][k].stages |= uniform_stage_flags;
								exists = true;
								break;
							}
						}

						if (exists) {
							continue; // Merged.
						}
					}

					uniform.stages |= uniform_stage_flags;

					if (set >= (uint32_t)reflection.uniform_sets.size()) {
						reflection.uniform_sets.resize(set + 1);
					}

					reflection.uniform_sets[set].push_back(uniform);
				}
			}

			{
				// Specialization constants.

				uint32_t sc_count = 0;
				result = spvReflectEnumerateSpecializationConstants(&module, &sc_count, nullptr);
				ERR_FAIL_COND_V_MSG(result != SPV_REFLECT_RESULT_SUCCESS, FAILED,
					"Reflection of SPIR-V shader stage '" +
						String(RDC::SHADER_STAGE_NAMES[p_spirv[i].shader_stage]) +
						"' failed enumerating specialization constants.");

				if (sc_count) {
					Vector<SpvReflectSpecializationConstant*> spec_constants;
					spec_constants.resize(sc_count);

					result = spvReflectEnumerateSpecializationConstants(
						&module, &sc_count, spec_constants.ptrw());
					ERR_FAIL_COND_V_MSG(result != SPV_REFLECT_RESULT_SUCCESS, FAILED,
						"Reflection of SPIR-V shader stage '" +
							String(RDC::SHADER_STAGE_NAMES[p_spirv[i].shader_stage]) +
							"' failed obtaining specialization constants.");

					for (uint32_t j = 0; j < sc_count; j++) {
						int32_t existing = -1;
						ReflectSpecializationConstant sconst;
						SpvReflectSpecializationConstant* spc = spec_constants[j];
						sconst.set_spv_reflect(stage, spc);

						if (spc->default_value_size != 4) {
							ERR_FAIL_V_MSG(FAILED,
								vformat("Reflection of SPIR-V shader stage '%s' failed because the "
										"specialization constant #%d's default value is not 4 "
										"bytes long (%d) and is currently not supported.",
									RDC::SHADER_STAGE_NAMES[p_spirv[i].shader_stage],
									spc->constant_id, spc->default_value_size));
						}

						sconst.constant_id = spc->constant_id;
						sconst.int_value = 0; // Clear previous value JIC.

						switch (spc->type_description->op) {
						case SpvOpTypeBool:
							sconst.type = RDC::PIPELINE_SPECIALIZATION_CONSTANT_TYPE_BOOL;
							sconst.bool_value = *(uint32_t*)(spc->default_value);
							break;
						case SpvOpTypeInt:
							sconst.type = RDC::PIPELINE_SPECIALIZATION_CONSTANT_TYPE_INT;
							sconst.int_value = *(uint32_t*)(spc->default_value);
							break;
						case SpvOpTypeFloat:
							sconst.type = RDC::PIPELINE_SPECIALIZATION_CONSTANT_TYPE_FLOAT;
							sconst.float_value = *(float*)(spc->default_value);
							break;
						default:
							ERR_FAIL_V_MSG(
								FAILED, vformat("Reflection of SPIR-V shader stage '%s' failed "
												"because the specialization constant #%d does not "
												"use a known operation (%d).",
											RDC::SHADER_STAGE_NAMES[p_spirv[i].shader_stage],
											spc->constant_id, spc->type_description->op));
							break;
						}

						sconst.stages |= (1 << stage);

						for (uint32_t k = 0; k < reflection.specialization_constants.size(); k++) {
							if (reflection.specialization_constants[k].constant_id ==
								sconst.constant_id) {
								ERR_FAIL_COND_V_MSG(
									reflection.specialization_constants[k].type != sconst.type,
									FAILED,
									"More than one specialization constant used for id (" +
										itos(sconst.constant_id) + "), but their types differ.");
								ERR_FAIL_COND_V_MSG(
									reflection.specialization_constants[k].int_value !=
										sconst.int_value,
									FAILED,
									"More than one specialization constant used for id (" +
										itos(sconst.constant_id) +
										"), but their default values differ.");
								existing = k;
								break;
							}
						}

						if (existing >= 0) {
							reflection.specialization_constants[existing].stages |= (1 << stage);
						}
						else {
							reflection.specialization_constants.push_back(sconst);
						}
					}

					reflection.specialization_constants.sort();
				}
			}

			if (stage == RDC::SHADER_STAGE_VERTEX || stage == RDC::SHADER_STAGE_FRAGMENT) {
				uint32_t iv_count = 0;
				result = spvReflectEnumerateInputVariables(&module, &iv_count, nullptr);
				ERR_FAIL_COND_V_MSG(result != SPV_REFLECT_RESULT_SUCCESS, FAILED,
					"Reflection of SPIR-V shader stage '" +
						String(RDC::SHADER_STAGE_NAMES[p_spirv[i].shader_stage]) +
						"' failed enumerating input variables.");

				if (iv_count) {
					Vector<SpvReflectInterfaceVariable*> input_vars;
					input_vars.resize(iv_count);

					result =
						spvReflectEnumerateInputVariables(&module, &iv_count, input_vars.ptrw());
					ERR_FAIL_COND_V_MSG(result != SPV_REFLECT_RESULT_SUCCESS, FAILED,
						"Reflection of SPIR-V shader stage '" +
							String(RDC::SHADER_STAGE_NAMES[p_spirv[i].shader_stage]) +
							"' failed obtaining input variables.");

					for (const SpvReflectInterfaceVariable* v : input_vars) {
						if (!v) {
							continue;
						}
						if (stage == RDC::SHADER_STAGE_VERTEX) {
							if (v->decoration_flags == 0) { // Regular input.
								reflection.vertex_input_mask |= (((uint64_t)1) << v->location);
							}
						}
						if (v->built_in == SpvBuiltInViewIndex) {
							reflection.has_multiview = true;
						}
					}
				}
			}

			if (stage == RDC::SHADER_STAGE_FRAGMENT) {
				uint32_t ov_count = 0;
				result = spvReflectEnumerateOutputVariables(&module, &ov_count, nullptr);
				ERR_FAIL_COND_V_MSG(result != SPV_REFLECT_RESULT_SUCCESS, FAILED,
					"Reflection of SPIR-V shader stage '" +
						String(RDC::SHADER_STAGE_NAMES[p_spirv[i].shader_stage]) +
						"' failed enumerating output variables.");

				if (ov_count) {
					Vector<SpvReflectInterfaceVariable*> output_vars;
					output_vars.resize(ov_count);

					result =
						spvReflectEnumerateOutputVariables(&module, &ov_count, output_vars.ptrw());
					ERR_FAIL_COND_V_MSG(result != SPV_REFLECT_RESULT_SUCCESS, FAILED,
						"Reflection of SPIR-V shader stage '" +
							String(RDC::SHADER_STAGE_NAMES[p_spirv[i].shader_stage]) +
							"' failed obtaining output variables.");

					for (const SpvReflectInterfaceVariable* refvar : output_vars) {
						if (!refvar) {
							continue;
						}
						if (refvar->built_in != SpvBuiltInFragDepth) {
							reflection.fragment_output_mask |= 1 << refvar->location;
						}
					}
				}
			}

			uint32_t pc_count = 0;
			result = spvReflectEnumeratePushConstantBlocks(&module, &pc_count, nullptr);
			ERR_FAIL_COND_V_MSG(result != SPV_REFLECT_RESULT_SUCCESS, FAILED,
				"Reflection of SPIR-V shader stage '" +
					String(RDC::SHADER_STAGE_NAMES[p_spirv[i].shader_stage]) +
					"' failed enumerating push constants.");

			if (pc_count) {
				ERR_FAIL_COND_V_MSG(pc_count > 1, FAILED,
					"Reflection of SPIR-V shader stage '" +
						String(RDC::SHADER_STAGE_NAMES[p_spirv[i].shader_stage]) +
						"': Only one push constant is supported, which should be the same across "
						"shader stages.");

				Vector<SpvReflectBlockVariable*> pconstants;
				pconstants.resize(pc_count);
				result =
					spvReflectEnumeratePushConstantBlocks(&module, &pc_count, pconstants.ptrw());
				ERR_FAIL_COND_V_MSG(result != SPV_REFLECT_RESULT_SUCCESS, FAILED,
					"Reflection of SPIR-V shader stage '" +
						String(RDC::SHADER_STAGE_NAMES[p_spirv[i].shader_stage]) +
						"' failed obtaining push constants.");
#if 0
				if (pconstants[0] == nullptr) {
					Ref<FileAccess> f = FileAccess::open("res://popo.spv", FileAccess::WRITE);
					f->store_buffer((const uint8_t *)&SpirV[0], SpirV.size() * sizeof(uint32_t));
				}
#endif

				ERR_FAIL_COND_V_MSG(reflection.push_constant_size &&
										reflection.push_constant_size != pconstants[0]->size,
					FAILED,
					"Reflection of SPIR-V shader stage '" +
						String(RDC::SHADER_STAGE_NAMES[p_spirv[i].shader_stage]) +
						"': Push constant block must be the same across shader stages.");

				reflection.push_constant_size = pconstants[0]->size;
				reflection.push_constant_stages |= uniform_stage_flags;

				// print_line("Stage: " + String(RDC::SHADER_STAGE_NAMES[stage]) + " push constant
				// of size=" + itos(push_constant.push_constant_size));
			}
		}
	}

	// Sort all uniform_sets by binding.
	for (uint32_t i = 0; i < reflection.uniform_sets.size(); i++) {
		reflection.uniform_sets[i].sort();
	}

	set_from_shader_reflection(reflection);

	return OK;
}

void RenderingShaderContainer::set_from_shader_reflection(const ReflectShader& p_reflection)
{
	reflection_binding_set_uniforms_count.clear();
	reflection_binding_set_uniforms_data.clear();
	reflection_specialization_data.clear();
	reflection_shader_stages.clear();

	reflection_data.vertex_input_mask = p_reflection.vertex_input_mask;
	reflection_data.fragment_output_mask = p_reflection.fragment_output_mask;
	reflection_data.specialization_constants_count = p_reflection.specialization_constants.size();
	reflection_data.pipeline_type = p_reflection.pipeline_type;
	reflection_data.has_multiview = p_reflection.has_multiview;
	reflection_data.has_dynamic_buffers = p_reflection.has_dynamic_buffers;
	reflection_data.compute_local_size[0] = p_reflection.compute_local_size[0];
	reflection_data.compute_local_size[1] = p_reflection.compute_local_size[1];
	reflection_data.compute_local_size[2] = p_reflection.compute_local_size[2];
	reflection_data.set_count = p_reflection.uniform_sets.size();
	reflection_data.push_constant_size = p_reflection.push_constant_size;
	reflection_data.push_constant_stages_mask = uint32_t(p_reflection.push_constant_stages);
	reflection_data.shader_name_len = shader_name.length();

	ReflectionBindingData binding_data;
	for (const ReflectDescriptorSet& uniform_set : p_reflection.uniform_sets) {
		for (const ReflectUniform& uniform : uniform_set) {
			binding_data.type = uint32_t(uniform.type);
			binding_data.binding = uniform.binding;
			binding_data.stages = uint32_t(uniform.stages);
			binding_data.length = uniform.length;
			binding_data.writable = uint32_t(uniform.writable);
			binding_data.texture_type = uniform.texture_type;
			binding_data.texture_format = uniform.texture_format;
			reflection_binding_set_uniforms_data.push_back(binding_data);
		}

		reflection_binding_set_uniforms_count.push_back(uniform_set.size());
	}

	ReflectionSpecializationData specialization_data;
	for (const ReflectSpecializationConstant& spec : p_reflection.specialization_constants) {
		specialization_data.type = uint32_t(spec.type);
		specialization_data.constant_id = spec.constant_id;
		specialization_data.int_value = spec.int_value;
		specialization_data.stage_flags = uint32_t(spec.stages);
		reflection_specialization_data.push_back(specialization_data);
	}

	for (uint32_t i = 0; i < RDC::SHADER_STAGE_MAX; i++) {
		if ((p_reflection.stages_bits & (1U << i)) != 0) {
			reflection_shader_stages.push_back(RDC::ShaderStage(i));
		}
	}

	reflection_data.stage_count = reflection_shader_stages.size();

	_set_from_shader_reflection_post(p_reflection);
}

bool RenderingShaderContainer::set_code_from_spirv(
	const String& p_shader_name, Span<RDC::ShaderStageSPIRVData> p_spirv)
{
	ReflectShader shader;
	ERR_FAIL_COND_V(reflect_spirv(p_shader_name, p_spirv, shader) != OK, false);
	return _set_code_from_spirv(shader);
}

RenderingDeviceCommons::ShaderReflection RenderingShaderContainer::get_shader_reflection() const
{
	RDC::ShaderReflection shader_refl;
	shader_refl.push_constant_size = reflection_data.push_constant_size;
	shader_refl.push_constant_stages = reflection_data.push_constant_stages_mask;
	shader_refl.vertex_input_mask = reflection_data.vertex_input_mask;
	shader_refl.fragment_output_mask = reflection_data.fragment_output_mask;
	shader_refl.pipeline_type = reflection_data.pipeline_type;
	shader_refl.has_multiview = reflection_data.has_multiview;
	shader_refl.has_dynamic_buffers = reflection_data.has_dynamic_buffers;
	shader_refl.compute_local_size[0] = reflection_data.compute_local_size[0];
	shader_refl.compute_local_size[1] = reflection_data.compute_local_size[1];
	shader_refl.compute_local_size[2] = reflection_data.compute_local_size[2];
	shader_refl.uniform_sets.resize(reflection_data.set_count);
	shader_refl.specialization_constants.resize(reflection_data.specialization_constants_count);
	shader_refl.stages_vector.resize(reflection_data.stage_count);

	DEV_ASSERT(
		reflection_binding_set_uniforms_count.size() == reflection_data.set_count &&
		"The amount of elements in the reflection and the shader container can't be different.");
	uint32_t uniform_index = 0;
	for (uint32_t i = 0; i < reflection_data.set_count; i++) {
		Vector<RDC::ShaderUniform>& uniform_set = shader_refl.uniform_sets.ptrw()[i];
		uint32_t uniforms_count = reflection_binding_set_uniforms_count[i];
		uniform_set.resize(uniforms_count);
		for (uint32_t j = 0; j < uniforms_count; j++) {
			const ReflectionBindingData& binding =
				reflection_binding_set_uniforms_data[uniform_index++];
			RDC::ShaderUniform& uniform = uniform_set.ptrw()[j];
			uniform.type = RDC::UniformType(binding.type);
			uniform.writable = binding.writable;
			uniform.length = binding.length;
			uniform.binding = binding.binding;
			uniform.stages = binding.stages;
			uniform.texture_type = binding.texture_type;
			uniform.texture_format = binding.texture_format;
		}
	}

	shader_refl.specialization_constants.resize(reflection_data.specialization_constants_count);
	for (uint32_t i = 0; i < reflection_data.specialization_constants_count; i++) {
		const ReflectionSpecializationData& spec = reflection_specialization_data[i];
		RDC::ShaderSpecializationConstant& sc = shader_refl.specialization_constants.ptrw()[i];
		sc.type = RDC::PipelineSpecializationConstantType(spec.type);
		sc.constant_id = spec.constant_id;
		sc.int_value = spec.int_value;
		sc.stages = spec.stage_flags;
	}

	shader_refl.stages_vector.resize(reflection_data.stage_count);
	for (uint32_t i = 0; i < reflection_data.stage_count; i++) {
		shader_refl.stages_vector.set(i, reflection_shader_stages[i]);
		shader_refl.stages_bits |= (1U << reflection_shader_stages[i]);
	}

	return shader_refl;
}

bool RenderingShaderContainer::from_bytes(const PackedByteArray& p_bytes)
{
	const uint64_t alignment = sizeof(uint32_t);
	const uint8_t* bytes_ptr = p_bytes.ptr();
	uint64_t bytes_offset = 0;

	// Read container header.
	ERR_FAIL_COND_V_MSG(int64_t(bytes_offset + sizeof(ContainerHeader)) > p_bytes.size(), false,
		"Not enough bytes for a container header in shader container.");
	const ContainerHeader& container_header = *(const ContainerHeader*)(&bytes_ptr[bytes_offset]);
	bytes_offset += sizeof(ContainerHeader);
	bytes_offset += _from_bytes_header_extra_data(&bytes_ptr[bytes_offset]);

	ERR_FAIL_COND_V_MSG(container_header.magic_number != CONTAINER_MAGIC_NUMBER, false,
		"Incorrect magic number in shader container.");
	ERR_FAIL_COND_V_MSG(container_header.version > CONTAINER_VERSION, false,
		"Unsupported version in shader container.");
	ERR_FAIL_COND_V_MSG(
		container_header.format != _format(), false, "Incorrect format in shader container.");
	ERR_FAIL_COND_V_MSG(container_header.format_version > _format_version(), false,
		"Unsupported format version in shader container.");

	// Adjust shaders to the size indicated by the container header.
	shaders.resize(container_header.shader_count);

	// Read reflection data.
	ERR_FAIL_COND_V_MSG(int64_t(bytes_offset + sizeof(ReflectionData)) > p_bytes.size(), false,
		"Not enough bytes for reflection data in shader container.");
	reflection_data = *(const ReflectionData*)(&bytes_ptr[bytes_offset]);
	bytes_offset += sizeof(ReflectionData);
	bytes_offset += _from_bytes_reflection_extra_data(&bytes_ptr[bytes_offset]);

	// Read shader name.
	ERR_FAIL_COND_V_MSG(int64_t(bytes_offset + reflection_data.shader_name_len) > p_bytes.size(),
		false, "Not enough bytes for shader name in shader container.");
	if (reflection_data.shader_name_len > 0) {
		String shader_name_str;
		shader_name_str.append_utf8(
			(const char*)(&bytes_ptr[bytes_offset]), reflection_data.shader_name_len);
		shader_name = shader_name_str.utf8();
		bytes_offset = aligned_to(bytes_offset + reflection_data.shader_name_len, alignment);
	}
	else {
		shader_name = CharString();
	}

	reflection_binding_set_uniforms_count.resize(reflection_data.set_count);
	reflection_binding_set_uniforms_data.clear();

	uint32_t uniform_index = 0;
	for (uint32_t i = 0; i < reflection_data.set_count; i++) {
		ERR_FAIL_COND_V_MSG(int64_t(bytes_offset + sizeof(uint32_t)) > p_bytes.size(), false,
			"Not enough bytes for uniform set count in shader container.");
		uint32_t uniforms_count = *(uint32_t*)(&bytes_ptr[bytes_offset]);
		reflection_binding_set_uniforms_count.ptrw()[i] = uniforms_count;
		bytes_offset += sizeof(uint32_t);

		reflection_binding_set_uniforms_data.resize(
			reflection_binding_set_uniforms_data.size() + uniforms_count);
		bytes_offset +=
			_from_bytes_reflection_binding_uniform_extra_data_start(&bytes_ptr[bytes_offset]);

		for (uint32_t j = 0; j < uniforms_count; j++) {
			ERR_FAIL_COND_V_MSG(
				int64_t(bytes_offset + sizeof(ReflectionBindingData)) > p_bytes.size(), false,
				"Not enough bytes for uniform in shader container.");
			memcpy(&reflection_binding_set_uniforms_data.ptrw()[uniform_index],
				&bytes_ptr[bytes_offset], sizeof(ReflectionBindingData));
			bytes_offset += sizeof(ReflectionBindingData);
			bytes_offset += _from_bytes_reflection_binding_uniform_extra_data(
				&bytes_ptr[bytes_offset], uniform_index);
			uniform_index++;
		}
	}

	reflection_specialization_data.resize(reflection_data.specialization_constants_count);
	bytes_offset +=
		_from_bytes_reflection_specialization_extra_data_start(&bytes_ptr[bytes_offset]);

	for (uint32_t i = 0; i < reflection_data.specialization_constants_count; i++) {
		ERR_FAIL_COND_V_MSG(
			int64_t(bytes_offset + sizeof(ReflectionSpecializationData)) > p_bytes.size(), false,
			"Not enough bytes for specialization in shader container.");
		memcpy(&reflection_specialization_data.ptrw()[i], &bytes_ptr[bytes_offset],
			sizeof(ReflectionSpecializationData));
		bytes_offset += sizeof(ReflectionSpecializationData);
		bytes_offset +=
			_from_bytes_reflection_specialization_extra_data(&bytes_ptr[bytes_offset], i);
	}

	const uint32_t stage_count = reflection_data.stage_count;
	if (stage_count > 0) {
		ERR_FAIL_COND_V_MSG(
			int64_t(bytes_offset + stage_count * sizeof(RDC::ShaderStage)) > p_bytes.size(), false,
			"Not enough bytes for stages in shader container.");
		reflection_shader_stages.resize(stage_count);
		bytes_offset += _from_bytes_shader_extra_data_start(&bytes_ptr[bytes_offset]);
		memcpy(reflection_shader_stages.ptrw(), &bytes_ptr[bytes_offset],
			stage_count * sizeof(RDC::ShaderStage));
		bytes_offset += stage_count * sizeof(RDC::ShaderStage);
	}

	// Read shaders.
	for (int64_t i = 0; i < shaders.size(); i++) {
		ERR_FAIL_COND_V_MSG(int64_t(bytes_offset + sizeof(ShaderHeader)) > p_bytes.size(), false,
			"Not enough bytes for shader header in shader container.");
		const ShaderHeader& header = *(const ShaderHeader*)(&bytes_ptr[bytes_offset]);
		bytes_offset += sizeof(ShaderHeader);

		ERR_FAIL_COND_V_MSG(int64_t(bytes_offset + header.code_compressed_size) > p_bytes.size(),
			false, "Not enough bytes for a shader in shader container.");
		Shader& shader = shaders.ptrw()[i];
		shader.shader_stage = RDC::ShaderStage(header.shader_stage);
		shader.code_compression_flags = header.code_compression_flags;
		shader.code_decompressed_size = header.code_decompressed_size;
		shader.code_compressed_bytes.resize(header.code_compressed_size);
		memcpy(shader.code_compressed_bytes.ptrw(), &bytes_ptr[bytes_offset],
			header.code_compressed_size);
		bytes_offset = aligned_to(bytes_offset + header.code_compressed_size, alignment);
		bytes_offset += _from_bytes_shader_extra_data(&bytes_ptr[bytes_offset], i);
	}

	bytes_offset += _from_bytes_footer_extra_data(&bytes_ptr[bytes_offset]);

	ERR_FAIL_COND_V_MSG(bytes_offset != (uint64_t)p_bytes.size(), false,
		"Amount of bytes in the container does not match the amount of bytes read.");
	return true;
}

PackedByteArray RenderingShaderContainer::to_bytes() const
{
	// Compute the exact size the container will require for writing everything out.
	const uint64_t alignment = sizeof(uint32_t);
	uint64_t total_size = 0;
	total_size += sizeof(ContainerHeader) + _to_bytes_header_extra_data(nullptr);
	total_size += sizeof(ReflectionData) + _to_bytes_reflection_extra_data(nullptr);
	total_size += aligned_to(reflection_data.shader_name_len, alignment);
	total_size += reflection_binding_set_uniforms_count.size() * sizeof(uint32_t);
	total_size += reflection_binding_set_uniforms_data.size() * sizeof(ReflectionBindingData);
	total_size += reflection_specialization_data.size() * sizeof(ReflectionSpecializationData);
	total_size += reflection_shader_stages.size() * sizeof(RDC::ShaderStage);

	for (uint32_t i = 0; i < reflection_binding_set_uniforms_data.size(); i++) {
		total_size += _to_bytes_reflection_binding_uniform_extra_data(nullptr, i);
	}

	for (uint32_t i = 0; i < reflection_specialization_data.size(); i++) {
		total_size += _to_bytes_reflection_specialization_extra_data(nullptr, i);
	}

	for (uint32_t i = 0; i < shaders.size(); i++) {
		total_size += sizeof(ShaderHeader);
		total_size += shaders[i].code_compressed_bytes.size();
		total_size = aligned_to(total_size, alignment);
		total_size += _to_bytes_shader_extra_data(nullptr, i);
	}

	total_size += _to_bytes_footer_extra_data(nullptr);

	// Create the array that will hold all of the data.
	PackedByteArray bytes;
	bytes.resize_initialized(total_size);

	// Write out the data to the array.
	uint64_t bytes_offset = 0;
	uint8_t* bytes_ptr = bytes.ptrw();
	ContainerHeader& container_header = *(ContainerHeader*)(&bytes_ptr[bytes_offset]);
	container_header.magic_number = CONTAINER_MAGIC_NUMBER;
	container_header.version = CONTAINER_VERSION;
	container_header.format = _format();
	container_header.format_version = _format_version();
	container_header.shader_count = shaders.size();
	bytes_offset += sizeof(ContainerHeader);
	bytes_offset += _to_bytes_header_extra_data(&bytes_ptr[bytes_offset]);

	memcpy(&bytes_ptr[bytes_offset], &reflection_data, sizeof(ReflectionData));
	bytes_offset += sizeof(ReflectionData);
	bytes_offset += _to_bytes_reflection_extra_data(&bytes_ptr[bytes_offset]);

	if (shader_name.size() > 0) {
		memcpy(&bytes_ptr[bytes_offset], shader_name.ptr(), reflection_data.shader_name_len);
		bytes_offset = aligned_to(bytes_offset + reflection_data.shader_name_len, alignment);
	}

	uint32_t uniform_index = 0;
	for (uint32_t uniform_count : reflection_binding_set_uniforms_count) {
		memcpy(&bytes_ptr[bytes_offset], &uniform_count, sizeof(uniform_count));
		bytes_offset += sizeof(uint32_t);

		for (uint32_t i = 0; i < uniform_count; i++) {
			memcpy(&bytes_ptr[bytes_offset], &reflection_binding_set_uniforms_data[uniform_index],
				sizeof(ReflectionBindingData));
			bytes_offset += sizeof(ReflectionBindingData);
			bytes_offset += _to_bytes_reflection_binding_uniform_extra_data(
				&bytes_ptr[bytes_offset], uniform_index);
			uniform_index++;
		}
	}

	for (uint32_t i = 0; i < reflection_specialization_data.size(); i++) {
		memcpy(&bytes_ptr[bytes_offset], &reflection_specialization_data.ptr()[i],
			sizeof(ReflectionSpecializationData));
		bytes_offset += sizeof(ReflectionSpecializationData);
		bytes_offset += _to_bytes_reflection_specialization_extra_data(&bytes_ptr[bytes_offset], i);
	}

	if (!reflection_shader_stages.is_empty()) {
		uint32_t stage_count = reflection_shader_stages.size();
		memcpy(&bytes_ptr[bytes_offset], reflection_shader_stages.ptr(),
			stage_count * sizeof(RDC::ShaderStage));
		bytes_offset += stage_count * sizeof(RDC::ShaderStage);
	}

	for (uint32_t i = 0; i < shaders.size(); i++) {
		const Shader& shader = shaders[i];
		ShaderHeader& header = *(ShaderHeader*)(&bytes.ptr()[bytes_offset]);
		header.shader_stage = shader.shader_stage;
		header.code_compressed_size = uint32_t(shader.code_compressed_bytes.size());
		header.code_compression_flags = shader.code_compression_flags;
		header.code_decompressed_size = shader.code_decompressed_size;
		bytes_offset += sizeof(ShaderHeader);
		memcpy(&bytes.ptrw()[bytes_offset], shader.code_compressed_bytes.ptr(),
			shader.code_compressed_bytes.size());
		bytes_offset = aligned_to(bytes_offset + shader.code_compressed_bytes.size(), alignment);
		bytes_offset += _to_bytes_shader_extra_data(&bytes_ptr[bytes_offset], i);
	}

	bytes_offset += _to_bytes_footer_extra_data(&bytes_ptr[bytes_offset]);

	ERR_FAIL_COND_V_MSG(bytes_offset != total_size, PackedByteArray(),
		"Amount of bytes written does not match the amount of bytes reserved for the container.");
	return bytes;
}

bool RenderingShaderContainer::compress_code(const uint8_t* p_decompressed_bytes,
	uint32_t p_decompressed_size, uint8_t* p_compressed_bytes, uint32_t* r_compressed_size,
	uint32_t* r_compressed_flags) const
{
	DEV_ASSERT(p_decompressed_bytes != nullptr);
	DEV_ASSERT(p_decompressed_size > 0);
	DEV_ASSERT(p_compressed_bytes != nullptr);
	DEV_ASSERT(r_compressed_size != nullptr);
	DEV_ASSERT(r_compressed_flags != nullptr);

	*r_compressed_flags = 0;

	PackedByteArray zstd_bytes;
	const int64_t zstd_max_bytes =
		Compression::get_max_compressed_buffer_size(p_decompressed_size, Compression::MODE_ZSTD);
	zstd_bytes.resize(zstd_max_bytes);

	const int64_t zstd_size = Compression::compress(
		zstd_bytes.ptrw(), p_decompressed_bytes, p_decompressed_size, Compression::MODE_ZSTD);
	if (zstd_size > 0 && (uint32_t)(zstd_size) < p_decompressed_size) {
		// Only choose Zstd if it results in actual compression.
		memcpy(p_compressed_bytes, zstd_bytes.ptr(), zstd_size);
		*r_compressed_size = zstd_size;
		*r_compressed_flags |= COMPRESSION_FLAG_ZSTD;
	}
	else {
		// Just copy the input to the output directly.
		memcpy(p_compressed_bytes, p_decompressed_bytes, p_decompressed_size);
		*r_compressed_size = p_decompressed_size;
	}

	return true;
}

bool RenderingShaderContainer::decompress_code(const uint8_t* p_compressed_bytes,
	uint32_t p_compressed_size, uint32_t p_compressed_flags, uint8_t* p_decompressed_bytes,
	uint32_t p_decompressed_size) const
{
	DEV_ASSERT(p_compressed_bytes != nullptr);
	DEV_ASSERT(p_compressed_size > 0);
	DEV_ASSERT(p_decompressed_bytes != nullptr);
	DEV_ASSERT(p_decompressed_size > 0);

	bool uses_zstd = p_compressed_flags & COMPRESSION_FLAG_ZSTD;
	if (uses_zstd) {
		if (!Compression::decompress(p_decompressed_bytes, p_decompressed_size, p_compressed_bytes,
				p_compressed_size, Compression::MODE_ZSTD)) {
			ERR_FAIL_V_MSG(false, "Malformed zstd input for decompressing shader code.");
		}
	}
	else {
		memcpy(
			p_decompressed_bytes, p_compressed_bytes, MIN(p_compressed_size, p_decompressed_size));
	}

	return true;
}

RenderingShaderContainer::RenderingShaderContainer() {}

RenderingShaderContainer::~RenderingShaderContainer() {}

template <typename T>
void RenderingShaderContainer::ReflectSymbol<T>::set_spv_reflect(
	RDC::ShaderStage p_stage, const T* p_spv)
{
}
