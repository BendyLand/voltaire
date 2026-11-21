/**************************************************************************/
/*  texture_loader_dds.cpp                                                */
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

#include "core/io/file_access.h"
#include "core/io/file_access_wrappers.h"
#include "core/io/stream_peer.h"
#include "register_types.h"
#include "scene/resources/texture.h"
#include "scene/resources/image_texture.h"
#include "texture_loader_dds.h"

#define PF_FOURCC(m_s) ((uint32_t)(((m_s)[3] << 24U) | ((m_s)[2] << 16U) | ((m_s)[1] << 8U) | ((m_s)[0])))

// Reference: https://docs.microsoft.com/en-us/windows/win32/direct3ddds/dds-header

enum {
	DDS_MAGIC = 0x20534444,
	DDS_HEADER_SIZE = 124,
	DDS_PIXELFORMAT_SIZE = 32,

	DDSD_PITCH = 0x00000008,
	DDSD_LINEARSIZE = 0x00080000,
	DDSD_MIPMAPCOUNT = 0x00020000,
	DDSD_CAPS = 0x1,
	DDSD_HEIGHT = 0x2,
	DDSD_WIDTH = 0x4,
	DDSD_PIXELFORMAT = 0x1000,
	DDPF_ALPHAPIXELS = 0x00000001,
	DDPF_ALPHAONLY = 0x00000002,
	DDPF_FOURCC = 0x00000004,
	DDPF_RGB = 0x00000040,
	DDPF_RG_SNORM = 0x00080000,

	DDSC2_CUBEMAP = 0x200,
	DDSC2_VOLUME = 0x200000,

	DX10D_1D = 2,
	DX10D_2D = 3,
	DX10D_3D = 4,
};

enum DDSFourCC {
	DDFCC_DXT1 = PF_FOURCC("DXT1"),
	DDFCC_DXT2 = PF_FOURCC("DXT2"),
	DDFCC_DXT3 = PF_FOURCC("DXT3"),
	DDFCC_DXT4 = PF_FOURCC("DXT4"),
	DDFCC_DXT5 = PF_FOURCC("DXT5"),
	DDFCC_ATI1 = PF_FOURCC("ATI1"),
	DDFCC_BC4U = PF_FOURCC("BC4U"),
	DDFCC_ATI2 = PF_FOURCC("ATI2"),
	DDFCC_BC5U = PF_FOURCC("BC5U"),
	DDFCC_A2XY = PF_FOURCC("A2XY"),
	DDFCC_DX10 = PF_FOURCC("DX10"),
	DDFCC_RGBA16 = 36,
	DDFCC_R16F = 111,
	DDFCC_RG16F = 112,
	DDFCC_RGBA16F = 113,
	DDFCC_R32F = 114,
	DDFCC_RG32F = 115,
	DDFCC_RGBA32F = 116,
};

// Reference: https://learn.microsoft.com/en-us/windows/win32/api/dxgiformat/ne-dxgiformat-dxgi_format
enum DXGIFormat {
	DXGI_R32G32B32A32_FLOAT = 2,
	DXGI_R32G32B32_FLOAT = 6,
	DXGI_R16G16B16A16_FLOAT = 10,
	DXGI_R16G16B16A16_UNORM = 11,
	DXGI_R16G16B16A16_UINT = 12,
	DXGI_R32G32_FLOAT = 16,
	DXGI_R10G10B10A2_UNORM = 24,
	DXGI_R8G8B8A8_UNORM = 28,
	DXGI_R8G8B8A8_UNORM_SRGB = 29,
	DXGI_R16G16_FLOAT = 34,
	DXGI_R16G16_UNORM = 35,
	DXGI_R16G16_UINT = 36,
	DXGI_R32_FLOAT = 41,
	DXGI_R8G8_UNORM = 49,
	DXGI_R16_FLOAT = 54,
	DXGI_R16_UNORM = 56,
	DXGI_R16_UINT = 57,
	DXGI_R8_UNORM = 61,
	DXGI_A8_UNORM = 65,
	DXGI_R9G9B9E5 = 67,
	DXGI_BC1_UNORM = 71,
	DXGI_BC1_UNORM_SRGB = 72,
	DXGI_BC2_UNORM = 74,
	DXGI_BC2_UNORM_SRGB = 75,
	DXGI_BC3_UNORM = 77,
	DXGI_BC3_UNORM_SRGB = 78,
	DXGI_BC4_UNORM = 80,
	DXGI_BC5_UNORM = 83,
	DXGI_B5G6R5_UNORM = 85,
	DXGI_B5G5R5A1_UNORM = 86,
	DXGI_B8G8R8A8_UNORM = 87,
	DXGI_BC6H_UF16 = 95,
	DXGI_BC6H_SF16 = 96,
	DXGI_BC7_UNORM = 98,
	DXGI_BC7_UNORM_SRGB = 99,
	DXGI_B4G4R4A4_UNORM = 115,
};

// The legacy bitmasked format names here represent the actual data layout in the files,
// while their official names are flipped (e.g. RGBA8 layout is officially called ABGR8).
enum DDSFormat {
	DDS_DXT1,
	DDS_DXT3,
	DDS_DXT5,
	DDS_ATI1,
	DDS_ATI2,
	DDS_BC6U,
	DDS_BC6S,
	DDS_BC7,
	DDS_R16,
	DDS_RG16,
	DDS_RGBA16,
	DDS_R16I,
	DDS_RG16I,
	DDS_RGBA16I,
	DDS_R16F,
	DDS_RG16F,
	DDS_RGBA16F,
	DDS_R32F,
	DDS_RG32F,
	DDS_RGB32F,
	DDS_RGBA32F,
	DDS_RGB9E5,
	DDS_RGB8,
	DDS_RGBA8,
	DDS_RGBX8,
	DDS_BGR8,
	DDS_BGRA8,
	DDS_BGRX8,
	DDS_BGR5A1,
	DDS_BGR565,
	DDS_B2GR3,
	DDS_B2GR3A8,
	DDS_BGR10A2,
	DDS_RGB10A2,
	DDS_BGRA4,
	DDS_LUMINANCE,
	DDS_LUMINANCE_ALPHA,
	DDS_LUMINANCE_ALPHA_4,
	DDS_RG8,
	DDS_R8,
	DDS_MAX
};

enum DDSType {
	DDST_2D = 1,
	DDST_CUBEMAP,
	DDST_3D,

	DDST_TYPE_MASK = 0x7F,
	DDST_ARRAY = 0x80,
};

struct DDSFormatInfo {
	const char *name = nullptr;
	bool compressed = false;
	uint32_t divisor = 0;
	uint32_t block_size = 0;
	Image::Format format = Image::Format::FORMAT_BPTC_RGBA;
};

static const DDSFormatInfo dds_format_info[DDS_MAX] = {
	{ "DXT1/BC1", true, 4, 8, Image::FORMAT_DXT1 },
	{ "DXT2/DXT3/BC2", true, 4, 16, Image::FORMAT_DXT3 },
	{ "DXT4/DXT5/BC3", true, 4, 16, Image::FORMAT_DXT5 },
	{ "ATI1/BC4", true, 4, 8, Image::FORMAT_RGTC_R },
	{ "ATI2/A2XY/BC5", true, 4, 16, Image::FORMAT_RGTC_RG },
	{ "BC6UF", true, 4, 16, Image::FORMAT_BPTC_RGBFU },
	{ "BC6SF", true, 4, 16, Image::FORMAT_BPTC_RGBF },
	{ "BC7", true, 4, 16, Image::FORMAT_BPTC_RGBA },
	{ "R16", false, 1, 2, Image::FORMAT_R16 },
	{ "RG16", false, 1, 4, Image::FORMAT_RG16 },
	{ "RGBA16", false, 1, 8, Image::FORMAT_RGBA16 },
	{ "R16I", false, 1, 2, Image::FORMAT_R16I },
	{ "RG16I", false, 1, 4, Image::FORMAT_RG16I },
	{ "RGBA16I", false, 1, 8, Image::FORMAT_RGBA16I },
	{ "R16F", false, 1, 2, Image::FORMAT_RH },
	{ "RG16F", false, 1, 4, Image::FORMAT_RGH },
	{ "RGBA16F", false, 1, 8, Image::FORMAT_RGBAH },
	{ "R32F", false, 1, 4, Image::FORMAT_RF },
	{ "RG32F", false, 1, 8, Image::FORMAT_RGF },
	{ "RGB32F", false, 1, 12, Image::FORMAT_RGBF },
	{ "RGBA32F", false, 1, 16, Image::FORMAT_RGBAF },
	{ "RGB9E5", false, 1, 4, Image::FORMAT_RGBE9995 },
	{ "RGB8", false, 1, 3, Image::FORMAT_RGB8 },
	{ "RGBA8", false, 1, 4, Image::FORMAT_RGBA8 },
	{ "RGBX8", false, 1, 4, Image::FORMAT_RGB8 },
	{ "BGR8", false, 1, 3, Image::FORMAT_RGB8 },
	{ "BGRA8", false, 1, 4, Image::FORMAT_RGBA8 },
	{ "BGRX8", false, 1, 4, Image::FORMAT_RGB8 },
	{ "BGR5A1", false, 1, 2, Image::FORMAT_RGBA8 },
	{ "BGR565", false, 1, 2, Image::FORMAT_RGB565 },
	{ "B2GR3", false, 1, 1, Image::FORMAT_RGB8 },
	{ "B2GR3A8", false, 1, 2, Image::FORMAT_RGBA8 },
	{ "BGR10A2", false, 1, 4, Image::FORMAT_RGBA8 },
	{ "RGB10A2", false, 1, 4, Image::FORMAT_RGBA8 },
	{ "BGRA4", false, 1, 2, Image::FORMAT_RGBA4444 },
	{ "GRAYSCALE", false, 1, 1, Image::FORMAT_L8 },
	{ "GRAYSCALE_ALPHA", false, 1, 2, Image::FORMAT_LA8 },
	{ "GRAYSCALE_ALPHA_4", false, 1, 1, Image::FORMAT_LA8 },
	{ "RG8", false, 1, 2, Image::FORMAT_RG8 },
	{ "R8", false, 1, 1, Image::FORMAT_R8 },
};

DDSFormat _dxgi_to_dds_format(uint32_t p_dxgi_format)
{
	switch (p_dxgi_format) {
	case DXGI_R32G32B32A32_FLOAT: {
		return DDS_RGBA32F;
	}
	case DXGI_R32G32B32_FLOAT: {
		return DDS_RGB32F;
	}
	case DXGI_R16G16B16A16_FLOAT: {
		return DDS_RGBA16F;
	}
	case DXGI_R16G16B16A16_UNORM: {
		return DDS_RGBA16;
	}
	case DXGI_R16G16B16A16_UINT: {
		return DDS_RGBA16I;
	}
	case DXGI_R32G32_FLOAT: {
		return DDS_RG32F;
	}
	case DXGI_R10G10B10A2_UNORM: {
		return DDS_RGB10A2;
	}
	case DXGI_R8G8B8A8_UNORM:
	case DXGI_R8G8B8A8_UNORM_SRGB: {
		return DDS_RGBA8;
	}
	case DXGI_R16G16_FLOAT: {
		return DDS_RG16F;
	}
	case DXGI_R16G16_UNORM: {
		return DDS_RG16;
	}
	case DXGI_R16G16_UINT: {
		return DDS_RG16I;
	}
	case DXGI_R32_FLOAT: {
		return DDS_R32F;
	}
	case DXGI_R8_UNORM: {
		return DDS_R8;
	}
	case DXGI_A8_UNORM: {
		return DDS_LUMINANCE;
	}
	case DXGI_R16_FLOAT: {
		return DDS_R16F;
	}
	case DXGI_R16_UNORM: {
		return DDS_R16;
	}
	case DXGI_R16_UINT: {
		return DDS_R16I;
	}
	case DXGI_R8G8_UNORM: {
		return DDS_RG8;
	}
	case DXGI_R9G9B9E5: {
		return DDS_RGB9E5;
	}
	case DXGI_BC1_UNORM:
	case DXGI_BC1_UNORM_SRGB: {
		return DDS_DXT1;
	}
	case DXGI_BC2_UNORM:
	case DXGI_BC2_UNORM_SRGB: {
		return DDS_DXT3;
	}
	case DXGI_BC3_UNORM:
	case DXGI_BC3_UNORM_SRGB: {
		return DDS_DXT5;
	}
	case DXGI_BC4_UNORM: {
		return DDS_ATI1;
	}
	case DXGI_BC5_UNORM: {
		return DDS_ATI2;
	}
	case DXGI_B5G6R5_UNORM: {
		return DDS_BGR565;
	}
	case DXGI_B5G5R5A1_UNORM: {
		return DDS_BGR5A1;
	}
	case DXGI_B8G8R8A8_UNORM: {
		return DDS_BGRA8;
	}
	case DXGI_BC6H_UF16: {
		return DDS_BC6U;
	}
	case DXGI_BC6H_SF16: {
		return DDS_BC6S;
	}
	case DXGI_BC7_UNORM:
	case DXGI_BC7_UNORM_SRGB: {
		return DDS_BC7;
	}
	case DXGI_B4G4R4A4_UNORM: {
		return DDS_BGRA4;
	}

	default: {
		return DDS_MAX;
	}
	}
}

static Ref<Image> _dds_load_layer(Ref<FileAccess> p_file, DDSFormat p_dds_format, uint32_t p_width,
	uint32_t p_height, uint32_t p_mipmaps, uint32_t p_pitch, uint32_t p_flags,
	Vector<uint8_t>& r_src_data)
{
	const DDSFormatInfo& info = dds_format_info[p_dds_format];

	uint32_t w = p_width;
	uint32_t h = p_height;

	if (info.compressed) {
		// BC compressed.
		w += w % info.divisor;
		h += h % info.divisor;
		if (w != p_width) {
			WARN_PRINT(vformat("%s: DDS width '%d' is not divisible by %d. This is not allowed as "
							   "per the DDS specification, attempting to load anyway.",
				p_file->get_path(), p_width, info.divisor));
		}
		if (h != p_height) {
			WARN_PRINT(vformat("%s: DDS height '%d' is not divisible by %d. This is not allowed as "
							   "per the DDS specification, attempting to load anyway.",
				p_file->get_path(), p_height, info.divisor));
		}

		uint32_t size = MAX(1u, (w + 3) / 4) * MAX(1u, (h + 3) / 4) * info.block_size;

		if (p_flags & DDSD_LINEARSIZE) {
			ERR_FAIL_COND_V_MSG(size != p_pitch, Ref<Resource>(),
				"DDS header flags specify that a linear size of the top-level image is present, "
				"but the specified size does not match the expected value.");
		}
		else {
			ERR_FAIL_COND_V_MSG(p_pitch != 0, Ref<Resource>(),
				"DDS header flags specify that no linear size will given for the top-level image, "
				"but a non-zero linear size value is present in the header.");
		}

		for (uint32_t i = 1; i < p_mipmaps; i++) {
			w = MAX(1u, w >> 1);
			h = MAX(1u, h >> 1);

			uint32_t bsize = MAX(1u, (w + 3) / 4) * MAX(1u, (h + 3) / 4) * info.block_size;
			size += bsize;
		}

		r_src_data.resize(size);
		uint8_t* wb = r_src_data.ptrw();
		p_file->get_buffer(wb, size);

	}
	else {
		// Generic uncompressed.
		uint32_t size = p_width * p_height * info.block_size;

		for (uint32_t i = 1; i < p_mipmaps; i++) {
			w = MAX(1u, w >> 1);
			h = MAX(1u, h >> 1);
			size += w * h * info.block_size;
		}

		// Calculate the space these formats will take up after decoding.
		switch (p_dds_format) {
		case DDS_BGR5A1:
		case DDS_B2GR3A8:
		case DDS_LUMINANCE_ALPHA_4:
			size = size * 2;
			break;

		case DDS_B2GR3:
			size = size * 3;
			break;

		default:
			break;
		}

		r_src_data.resize(size);
		uint8_t* wb = r_src_data.ptrw();
		p_file->get_buffer(wb, size);

		switch (p_dds_format) {
		case DDS_BGR5A1: {
			// To RGBA8.
			int colcount = size / 4;

			for (int i = colcount - 1; i >= 0; i--) {
				int src_ofs = i * 2;
				int dst_ofs = i * 4;

				uint8_t a = wb[src_ofs + 1] & 0x80;
				uint8_t b = wb[src_ofs] & 0x1F;
				uint8_t g = (wb[src_ofs] >> 5) | ((wb[src_ofs + 1] & 0x3) << 3);
				uint8_t r = (wb[src_ofs + 1] >> 2) & 0x1F;

				wb[dst_ofs + 0] = r << 3;
				wb[dst_ofs + 1] = g << 3;
				wb[dst_ofs + 2] = b << 3;
				wb[dst_ofs + 3] = a ? 255 : 0;
			}

		} break;
		case DDS_BGRA4: {
			// To RGBA4.
			for (uint32_t i = 0; i < size; i += 2) {
				uint8_t ar = wb[i + 0];
				uint8_t gb = wb[i + 1];

				wb[i + 0] = ((ar & 0x0F) << 4) | ((gb & 0xF0) >> 4);
				wb[i + 1] = ((ar & 0xF0) >> 4) | ((gb & 0x0F) << 4);
			}

		} break;
		case DDS_B2GR3: {
			// To RGB8.
			int colcount = size / 3;

			for (int i = colcount - 1; i >= 0; i--) {
				int src_ofs = i;
				int dst_ofs = i * 3;

				uint8_t b = (wb[src_ofs] & 0x3) << 6;
				uint8_t g = (wb[src_ofs] & 0x1C) << 3;
				uint8_t r = (wb[src_ofs] & 0xE0);

				wb[dst_ofs] = r;
				wb[dst_ofs + 1] = g;
				wb[dst_ofs + 2] = b;
			}

		} break;
		case DDS_B2GR3A8: {
			// To RGBA8.
			int colcount = size / 4;

			for (int i = colcount - 1; i >= 0; i--) {
				int src_ofs = i * 2;
				int dst_ofs = i * 4;

				uint8_t b = (wb[src_ofs] & 0x3) << 6;
				uint8_t g = (wb[src_ofs] & 0x1C) << 3;
				uint8_t r = (wb[src_ofs] & 0xE0);
				uint8_t a = wb[src_ofs + 1];

				wb[dst_ofs] = r;
				wb[dst_ofs + 1] = g;
				wb[dst_ofs + 2] = b;
				wb[dst_ofs + 3] = a;
			}

		} break;
		case DDS_RGB10A2: {
			// To RGBA8.
			int colcount = size / 4;

			for (int i = 0; i < colcount; i++) {
				int ofs = i * 4;

				uint32_t w32 = uint32_t(wb[ofs + 0]) | (uint32_t(wb[ofs + 1]) << 8) |
							   (uint32_t(wb[ofs + 2]) << 16) | (uint32_t(wb[ofs + 3]) << 24);

				// This method follows the 'standard' way of decoding 10-bit dds files,
				// which means the ones created with DirectXTex will be loaded incorrectly.
				uint8_t a = (w32 & 0xc0000000) >> 24;
				uint8_t r = (w32 & 0x3ff) >> 2;
				uint8_t g = (w32 & 0xffc00) >> 12;
				uint8_t b = (w32 & 0x3ff00000) >> 22;

				wb[ofs + 0] = r;
				wb[ofs + 1] = g;
				wb[ofs + 2] = b;
				wb[ofs + 3] = a == 0xc0 ? 255 : a; // 0xc0 should be opaque.
			}

		} break;
		case DDS_BGR10A2: {
			// To RGBA8.
			int colcount = size / 4;

			for (int i = 0; i < colcount; i++) {
				int ofs = i * 4;

				uint32_t w32 = uint32_t(wb[ofs + 0]) | (uint32_t(wb[ofs + 1]) << 8) |
							   (uint32_t(wb[ofs + 2]) << 16) | (uint32_t(wb[ofs + 3]) << 24);

				// This method follows the 'standard' way of decoding 10-bit dds files,
				// which means the ones created with DirectXTex will be loaded incorrectly.
				uint8_t a = (w32 & 0xc0000000) >> 24;
				uint8_t r = (w32 & 0x3ff00000) >> 22;
				uint8_t g = (w32 & 0xffc00) >> 12;
				uint8_t b = (w32 & 0x3ff) >> 2;

				wb[ofs + 0] = r;
				wb[ofs + 1] = g;
				wb[ofs + 2] = b;
				wb[ofs + 3] = a == 0xc0 ? 255 : a; // 0xc0 should be opaque.
			}

		} break;

		// Channel-swapped.
		case DDS_BGRA8: {
			// To RGBA8.
			int colcount = size / 4;

			for (int i = 0; i < colcount; i++) {
				SWAP(wb[i * 4 + 0], wb[i * 4 + 2]);
			}

		} break;
		case DDS_BGR8: {
			// To RGB8.
			int colcount = size / 3;

			for (int i = 0; i < colcount; i++) {
				SWAP(wb[i * 3 + 0], wb[i * 3 + 2]);
			}

		} break;

		case DDS_RGBX8: {
			// To RGB8.
			int colcount = size / 4;

			for (int i = 0; i < colcount; i++) {
				int src_ofs = i * 4;
				int dst_ofs = i * 3;

				wb[dst_ofs + 0] = wb[src_ofs + 0];
				wb[dst_ofs + 1] = wb[src_ofs + 1];
				wb[dst_ofs + 2] = wb[src_ofs + 2];
			}

			r_src_data.resize(size * 3 / 4);

		} break;
		case DDS_BGRX8: {
			// To RGB8.
			int colcount = size / 4;

			for (int i = 0; i < colcount; i++) {
				int src_ofs = i * 4;
				int dst_ofs = i * 3;

				wb[dst_ofs + 0] = wb[src_ofs + 2];
				wb[dst_ofs + 1] = wb[src_ofs + 1];
				wb[dst_ofs + 2] = wb[src_ofs + 0];
			}

			r_src_data.resize(size * 3 / 4);

		} break;

		// Grayscale.
		case DDS_LUMINANCE_ALPHA_4: {
			// To LA8.
			int colcount = size / 2;

			for (int i = colcount - 1; i >= 0; i--) {
				int src_ofs = i;
				int dst_ofs = i * 2;

				uint8_t l = wb[src_ofs] & 0x0F;
				uint8_t a = wb[src_ofs] & 0xF0;

				wb[dst_ofs] = (l << 4) | l;
				wb[dst_ofs + 1] = a | (a >> 4);
			}

		} break;

		default: {
		}
		}
	}

	return memnew(Image(p_width, p_height, p_mipmaps > 1, info.format, r_src_data));
}

static Vector<Ref<Image>> _dds_load_images(Ref<FileAccess> p_f, DDSFormat p_dds_format,
	uint32_t p_width, uint32_t p_height, uint32_t p_mipmaps, uint32_t p_pitch, uint32_t p_flags,
	uint32_t p_layer_count)
{
	Vector<uint8_t> src_data;
	Vector<Ref<Image>> images;
	images.resize(p_layer_count);

	for (uint32_t i = 0; i < p_layer_count; i++) {
		images.write[i] = _dds_load_layer(
			p_f, p_dds_format, p_width, p_height, p_mipmaps, p_pitch, p_flags, src_data);
		ERR_FAIL_COND_V(images.write[i].is_null(), Vector<Ref<Image>>());
	}

	return images;
}

static Ref<Resource> _dds_create_texture(const Vector<Ref<Image>>& p_images, uint32_t p_dds_type,
	uint32_t p_width, uint32_t p_height, uint32_t p_layer_count, uint32_t p_mipmaps, Error* r_error)
{
	ERR_FAIL_COND_V(p_images.is_empty(), Ref<Resource>());

	if ((p_dds_type & DDST_TYPE_MASK) == DDST_2D) {
		if (p_dds_type & DDST_ARRAY) {
			Ref<ImageTextureLayered> texture =
				memnew(ImageTextureLayered(TextureLayered::LAYERED_TYPE_2D_ARRAY));
			texture->create_from_images(p_images);

			if (r_error) {
				*r_error = OK;
			}

			return texture;

		}
		else {
			if (r_error) {
				*r_error = OK;
			}

			return ImageTexture::create_from_image(p_images[0]);
		}

	}
	else if ((p_dds_type & DDST_TYPE_MASK) == DDST_CUBEMAP) {
		ERR_FAIL_COND_V(p_layer_count % 6 != 0, Ref<Resource>());

		if (p_dds_type & DDST_ARRAY) {
			Ref<ImageTextureLayered> texture =
				memnew(ImageTextureLayered(TextureLayered::LAYERED_TYPE_CUBEMAP_ARRAY));
			texture->create_from_images(p_images);

			if (r_error) {
				*r_error = OK;
			}

			return texture;

		}
		else {
			Ref<ImageTextureLayered> texture =
				memnew(ImageTextureLayered(TextureLayered::LAYERED_TYPE_CUBEMAP));
			texture->create_from_images(p_images);

			if (r_error) {
				*r_error = OK;
			}

			return texture;
		}

	}
	else if ((p_dds_type & DDST_TYPE_MASK) == DDST_3D) {
		Ref<ImageTexture3D> texture;
		texture.instantiate();
		texture->create(
			p_images[0]->get_format(), p_width, p_height, p_layer_count, p_mipmaps > 1, p_images);

		if (r_error) {
			*r_error = OK;
		}

		return texture;
	}

	return Ref<Resource>();
}

static Ref<Resource> _dds_create_texture_from_images(const Vector<Ref<Image>>& p_images,
	DDSFormat p_dds_format, uint32_t p_width, uint32_t p_height, uint32_t p_mipmaps,
	uint32_t p_pitch, uint32_t p_flags, uint32_t p_layer_count, uint32_t p_dds_type, Error* r_error)
{
	return _dds_create_texture(
		p_images, p_dds_type, p_width, p_height, p_layer_count, p_mipmaps, r_error);
}

static Vector<Ref<Image>> _dds_load_images_from_buffer(Ref<FileAccess> p_f, DDSFormat& r_dds_format,
	uint32_t& r_width, uint32_t& r_height, uint32_t& r_mipmaps, uint32_t& r_pitch,
	uint32_t& r_flags, uint32_t& r_layer_count, uint32_t& r_dds_type, const String& p_path = "")
{
	ERR_FAIL_COND_V_MSG(p_f.is_null(), Vector<Ref<Image>>(), vformat("Empty DDS texture file."));
	ERR_FAIL_COND_V_MSG(
		!p_f->get_length(), Vector<Ref<Image>>(), vformat("Empty DDS texture file."));

	uint32_t magic = p_f->get_32();
	uint32_t hsize = p_f->get_32();
	r_flags = p_f->get_32();
	r_height = p_f->get_32();
	r_width = p_f->get_32();
	r_pitch = p_f->get_32();
	uint32_t depth = p_f->get_32();
	r_mipmaps = p_f->get_32();

	// Skip reserved.
	for (int i = 0; i < 11; i++) {
		p_f->get_32();
	}

	// Validate.
	// We don't check DDSD_CAPS or DDSD_PIXELFORMAT, as they're mandatory when writing,
	// but non-mandatory when reading (as some writers don't set them).
	if (magic != DDS_MAGIC || hsize != 124) {
		ERR_FAIL_V_MSG(
			Vector<Ref<Image>>(), vformat("Invalid or unsupported DDS texture file '%s'.", p_path));
	}

	/* uint32_t format_size = */ p_f->get_32();
	uint32_t format_flags = p_f->get_32();
	uint32_t format_fourcc = p_f->get_32();
	uint32_t format_rgb_bits = p_f->get_32();
	uint32_t format_red_mask = p_f->get_32();
	uint32_t format_green_mask = p_f->get_32();
	uint32_t format_blue_mask = p_f->get_32();
	uint32_t format_alpha_mask = p_f->get_32();

	/* uint32_t caps_1 = */ p_f->get_32();
	uint32_t caps_2 = p_f->get_32();
	/* uint32_t caps_3 = */ p_f->get_32();
	/* uint32_t caps_4 = */ p_f->get_32();

	// Skip reserved.
	p_f->get_32();

	if (p_f->get_position() < 128) {
		p_f->seek(128);
	}

	r_layer_count = 1;
	r_dds_type = DDST_2D;

	if (caps_2 & DDSC2_CUBEMAP) {
		r_dds_type = DDST_CUBEMAP;
		r_layer_count *= 6;

	}
	else if (caps_2 & DDSC2_VOLUME) {
		r_dds_type = DDST_3D;
		r_layer_count = depth;
	}

	r_dds_format = DDS_MAX;

	if (format_flags & DDPF_FOURCC) {
		// FourCC formats.
		switch (format_fourcc) {
		case DDFCC_DXT1: {
			r_dds_format = DDS_DXT1;
		} break;
		case DDFCC_DXT2:
		case DDFCC_DXT3: {
			r_dds_format = DDS_DXT3;
		} break;
		case DDFCC_DXT4:
		case DDFCC_DXT5: {
			r_dds_format = DDS_DXT5;
		} break;
		case DDFCC_ATI1:
		case DDFCC_BC4U: {
			r_dds_format = DDS_ATI1;
		} break;
		case DDFCC_ATI2:
		case DDFCC_BC5U:
		case DDFCC_A2XY: {
			r_dds_format = DDS_ATI2;
		} break;
		case DDFCC_RGBA16: {
			r_dds_format = DDS_RGBA16;
		} break;
		case DDFCC_R16F: {
			r_dds_format = DDS_R16F;
		} break;
		case DDFCC_RG16F: {
			r_dds_format = DDS_RG16F;
		} break;
		case DDFCC_RGBA16F: {
			r_dds_format = DDS_RGBA16F;
		} break;
		case DDFCC_R32F: {
			r_dds_format = DDS_R32F;
		} break;
		case DDFCC_RG32F: {
			r_dds_format = DDS_RG32F;
		} break;
		case DDFCC_RGBA32F: {
			r_dds_format = DDS_RGBA32F;
		} break;
		case DDFCC_DX10: {
			uint32_t dxgi_format = p_f->get_32();
			uint32_t dimension = p_f->get_32();
			/* uint32_t misc_flags_1 = */ p_f->get_32();
			uint32_t array_size = p_f->get_32();
			/* uint32_t misc_flags_2 = */ p_f->get_32();

			if (dimension == DX10D_3D) {
				r_dds_type = DDST_3D;
				r_layer_count = depth;
			}

			if (array_size > 1) {
				r_layer_count *= array_size;
				r_dds_type |= DDST_ARRAY;
			}

			r_dds_format = _dxgi_to_dds_format(dxgi_format);
		} break;

		default: {
			ERR_FAIL_V_MSG(Vector<Ref<Image>>(),
				vformat("Unrecognized or unsupported FourCC in DDS '%s'.", p_path));
		}
		}

	}
	else if (format_flags & DDPF_RGB) {
		// Channel-bitmasked formats.
		if (format_flags & DDPF_ALPHAPIXELS) {
			// With alpha.
			if (format_rgb_bits == 32 && format_red_mask == 0xff0000 &&
				format_green_mask == 0xff00 && format_blue_mask == 0xff &&
				format_alpha_mask == 0xff000000) {
				r_dds_format = DDS_BGRA8;
			}
			else if (format_rgb_bits == 32 && format_red_mask == 0xff &&
					   format_green_mask == 0xff00 && format_blue_mask == 0xff0000 &&
					   format_alpha_mask == 0xff000000) {
				r_dds_format = DDS_RGBA8;
			}
			else if (format_rgb_bits == 16 && format_red_mask == 0x00007c00 &&
					   format_green_mask == 0x000003e0 && format_blue_mask == 0x0000001f &&
					   format_alpha_mask == 0x00008000) {
				r_dds_format = DDS_BGR5A1;
			}
			else if (format_rgb_bits == 32 && format_red_mask == 0x3ff00000 &&
					   format_green_mask == 0xffc00 && format_blue_mask == 0x3ff &&
					   format_alpha_mask == 0xc0000000) {
				r_dds_format = DDS_BGR10A2;
			}
			else if (format_rgb_bits == 32 && format_red_mask == 0x3ff &&
					   format_green_mask == 0xffc00 && format_blue_mask == 0x3ff00000 &&
					   format_alpha_mask == 0xc0000000) {
				r_dds_format = DDS_RGB10A2;
			}
			else if (format_rgb_bits == 16 && format_red_mask == 0xf00 &&
					   format_green_mask == 0xf0 && format_blue_mask == 0xf &&
					   format_alpha_mask == 0xf000) {
				r_dds_format = DDS_BGRA4;
			}
			else if (format_rgb_bits == 16 && format_red_mask == 0xe0 &&
					   format_green_mask == 0x1c && format_blue_mask == 0x3 &&
					   format_alpha_mask == 0xff00) {
				r_dds_format = DDS_B2GR3A8;
			}

		}
		else {
			// Without alpha.
			if (format_rgb_bits == 24 && format_red_mask == 0xff0000 &&
				format_green_mask == 0xff00 && format_blue_mask == 0xff) {
				r_dds_format = DDS_BGR8;
			}
			else if (format_rgb_bits == 24 && format_red_mask == 0xff &&
					   format_green_mask == 0xff00 && format_blue_mask == 0xff0000) {
				r_dds_format = DDS_RGB8;
			}
			else if (format_rgb_bits == 16 && format_red_mask == 0x0000f800 &&
					   format_green_mask == 0x000007e0 && format_blue_mask == 0x0000001f) {
				r_dds_format = DDS_BGR565;
			}
			else if (format_rgb_bits == 8 && format_red_mask == 0xe0 &&
					   format_green_mask == 0x1c && format_blue_mask == 0x3) {
				r_dds_format = DDS_B2GR3;
			}
			else if (format_rgb_bits == 32 && format_red_mask == 0xff0000 &&
					   format_green_mask == 0xff00 && format_blue_mask == 0xff) {
				r_dds_format = DDS_BGRX8;
			}
			else if (format_rgb_bits == 32 && format_red_mask == 0xff &&
					   format_green_mask == 0xff00 && format_blue_mask == 0xff0000) {
				r_dds_format = DDS_RGBX8;
			}
			else if (format_rgb_bits == 8 && format_red_mask == 0xff && format_green_mask == 0 &&
					   format_blue_mask == 0) {
				r_dds_format = DDS_R8;
			}
			else if (format_rgb_bits == 16 && format_red_mask == 0xff &&
					   format_green_mask == 0xff00 && format_blue_mask == 0) {
				r_dds_format = DDS_RG8;
			}
		}

		if (format_rgb_bits == 32 && format_red_mask == 0xffff && format_green_mask == 0xffff0000) {
			r_dds_format = DDS_RG16;
		}

	}
	else {
		// Other formats.
		if (format_flags & DDPF_ALPHAONLY && format_rgb_bits == 8 && format_alpha_mask == 0xff) {
			// Alpha only.
			r_dds_format = DDS_LUMINANCE;
		}
	}

	// Depending on the writer, luminance formats may or may not have the DDPF_RGB or DDPF_LUMINANCE
	// flags defined, so we check for these formats after everything else failed.
	if (r_dds_format == DDS_MAX) {
		if (format_flags & DDPF_ALPHAPIXELS) {
			// With alpha.
			if (format_rgb_bits == 16 && format_red_mask == 0xff && format_alpha_mask == 0xff00) {
				r_dds_format = DDS_LUMINANCE_ALPHA;
			}
			else if (format_rgb_bits == 8 && format_red_mask == 0xf &&
					   format_alpha_mask == 0xf0) {
				r_dds_format = DDS_LUMINANCE_ALPHA_4;
			}

		}
		else {
			// Without alpha.
			if (format_rgb_bits == 8 && format_red_mask == 0xff) {
				r_dds_format = DDS_LUMINANCE;
			}
			else if (format_rgb_bits == 16 && format_red_mask == 0xffff) {
				r_dds_format = DDS_R16;
			}
		}
	}

	// No format detected, error.
	if (r_dds_format == DDS_MAX) {
		ERR_FAIL_V_MSG(Vector<Ref<Image>>(),
			vformat("Unrecognized or unsupported color layout in DDS '%s'.", p_path));
	}

	if (!(r_flags & DDSD_MIPMAPCOUNT)) {
		r_mipmaps = 1;
	}

	return _dds_load_images(
		p_f, r_dds_format, r_width, r_height, r_mipmaps, r_pitch, r_flags, r_layer_count);
}

static Ref<Resource> _dds_load_from_buffer(
	Ref<FileAccess> p_f, Error* r_error, const String& p_path = "")
{
	if (r_error) {
		*r_error = ERR_FILE_CORRUPT;
	}

	DDSFormat dds_format;
	uint32_t width = 0, height = 0, mipmaps = 0, pitch = 0, flags = 0, layer_count = 0,
			 dds_type = 0;

	Vector<Ref<Image>> images = _dds_load_images_from_buffer(
		p_f, dds_format, width, height, mipmaps, pitch, flags, layer_count, dds_type, p_path);
	return _dds_create_texture_from_images(
		images, dds_format, width, height, mipmaps, pitch, flags, layer_count, dds_type, r_error);
}

static Ref<Resource> _dds_load_from_file(const String& p_path, Error* r_error)
{
	if (r_error) {
		*r_error = ERR_CANT_OPEN;
	}

	Error err;
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ, &err);
	if (f.is_null()) {
		return Ref<Resource>();
	}

	return _dds_load_from_buffer(f, r_error, p_path);
}

Ref<Resource> ResourceFormatDDS::load(const String& p_path, const String& p_original_path,
	Error* r_error, bool p_use_sub_threads, float* r_progress, CacheMode p_cache_mode)
{
	return _dds_load_from_file(p_path, r_error);
}

void ResourceFormatDDS::get_recognized_extensions(List<String>* p_extensions) const
{
	p_extensions->push_back("dds");
}

bool ResourceFormatDDS::handles_type(const String& p_type) const { return true; }

String ResourceFormatDDS::get_resource_type(const String& p_path) const
{
	if (p_path.has_extension("dds")) {
		return "Texture";
	}
	return "";
}

Ref<Image> load_mem_dds(const uint8_t* p_dds, int p_size)
{
	ERR_FAIL_NULL_V(p_dds, Ref<Image>());
	ERR_FAIL_COND_V(!p_size, Ref<Image>());
	Ref<FileAccessMemory> memfile;
	memfile.instantiate();
	Error open_memfile_error = memfile->open_custom(p_dds, p_size);
	ERR_FAIL_COND_V_MSG(
		open_memfile_error, Ref<Image>(), "Could not create memfile for DDS image buffer.");

	DDSFormat dds_format;
	uint32_t width, height, mipmaps, pitch, flags, layer_count, dds_type;

	Vector<Ref<Image>> images = _dds_load_images_from_buffer(
		memfile, dds_format, width, height, mipmaps, pitch, flags, layer_count, dds_type);
	ERR_FAIL_COND_V_MSG(images.is_empty(), Ref<Image>(), "Failed to load DDS image.");

	return images[0];
}

ResourceFormatDDS::ResourceFormatDDS() { Image::_dds_mem_loader_func = load_mem_dds; }

Error save_dds(const String &p_path, const Ref<Image> &p_img) {
	Vector<uint8_t> buffer = save_dds_buffer(p_img);

	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE);
	if (file.is_null()) {
		return ERR_CANT_CREATE;
	}

	file->store_buffer(buffer.ptr(), buffer.size());

	return OK;
}

enum DDSFormatType {
	DDFT_BITMASK,
	DDFT_FOURCC,
	DDFT_DXGI,
};

DDSFormatType _dds_format_get_type(DDSFormat p_format) {
	switch (p_format) {
		case DDS_DXT1:
		case DDS_DXT3:
		case DDS_DXT5:
		case DDS_ATI1:
		case DDS_ATI2:
		case DDS_R16F:
		case DDS_RG16F:
		case DDS_RGBA16F:
		case DDS_R32F:
		case DDS_RG32F:
		case DDS_RGBA32F:
		case DDS_RGBA16:
			return DDFT_FOURCC;

		case DDS_BC6S:
		case DDS_BC6U:
		case DDS_BC7:
		case DDS_RGB9E5:
		case DDS_RGB32F:
		case DDS_R16I:
		case DDS_RG16I:
		case DDS_RGBA16I:
			return DDFT_DXGI;

		default:
			return DDFT_BITMASK;
	}
}

DDSFormat _image_format_to_dds_format(Image::Format p_image_format) {
	switch (p_image_format) {
		case Image::FORMAT_RGBAF: {
			return DDS_RGBA32F;
		}
		case Image::FORMAT_RGBF: {
			return DDS_RGB32F;
		}
		case Image::FORMAT_RGBAH: {
			return DDS_RGBA16F;
		}
		case Image::FORMAT_RGF: {
			return DDS_RG32F;
		}
		case Image::FORMAT_RGBA8: {
			return DDS_RGBA8;
		}
		case Image::FORMAT_RGH: {
			return DDS_RG16F;
		}
		case Image::FORMAT_RF: {
			return DDS_R32F;
		}
		case Image::FORMAT_L8:
		case Image::FORMAT_R8: {
			return DDS_LUMINANCE;
		}
		case Image::FORMAT_RH: {
			return DDS_R16F;
		}
		case Image::FORMAT_LA8:
		case Image::FORMAT_RG8: {
			return DDS_LUMINANCE_ALPHA;
		}
		case Image::FORMAT_RGBA4444: {
			return DDS_BGRA4;
		}
		case Image::FORMAT_RGB565: {
			return DDS_BGR565;
		}
		case Image::FORMAT_RGBE9995: {
			return DDS_RGB9E5;
		}
		case Image::FORMAT_DXT1: {
			return DDS_DXT1;
		}
		case Image::FORMAT_DXT3: {
			return DDS_DXT3;
		}
		case Image::FORMAT_DXT5: {
			return DDS_DXT5;
		}
		case Image::FORMAT_RGTC_R: {
			return DDS_ATI1;
		}
		case Image::FORMAT_RGTC_RG: {
			return DDS_ATI2;
		}
		case Image::FORMAT_RGB8: {
			return DDS_RGB8;
		}
		case Image::FORMAT_BPTC_RGBFU: {
			return DDS_BC6U;
		}
		case Image::FORMAT_BPTC_RGBF: {
			return DDS_BC6S;
		}
		case Image::FORMAT_BPTC_RGBA: {
			return DDS_BC7;
		}
		case Image::FORMAT_R16: {
			return DDS_R16;
		}
		case Image::FORMAT_RG16: {
			return DDS_RG16;
		}
		case Image::FORMAT_RGBA16: {
			return DDS_RGBA16;
		}
		case Image::FORMAT_R16I: {
			return DDS_R16I;
		}
		case Image::FORMAT_RG16I: {
			return DDS_RG16I;
		}
		case Image::FORMAT_RGBA16I: {
			return DDS_RGBA16I;
		}
		default: {
			return DDS_MAX;
		}
	}
}

uint32_t _image_format_to_fourcc_format(Image::Format p_format) {
	switch (p_format) {
		case Image::FORMAT_DXT1:
			return DDFCC_DXT1;
		case Image::FORMAT_DXT3:
			return DDFCC_DXT3;
		case Image::FORMAT_DXT5:
			return DDFCC_DXT5;
		case Image::FORMAT_RGTC_R:
			return DDFCC_ATI1;
		case Image::FORMAT_RGTC_RG:
			return DDFCC_ATI2;
		case Image::FORMAT_RF:
			return DDFCC_R32F;
		case Image::FORMAT_RGF:
			return DDFCC_RG32F;
		case Image::FORMAT_RGBAF:
			return DDFCC_RGBA32F;
		case Image::FORMAT_RH:
			return DDFCC_R16F;
		case Image::FORMAT_RGH:
			return DDFCC_RG16F;
		case Image::FORMAT_RGBAH:
			return DDFCC_RGBA16F;
		case Image::FORMAT_RGBA16:
			return DDFCC_RGBA16;

		default:
			return 0;
	}
}

uint32_t _image_format_to_dxgi_format(Image::Format p_format) {
	switch (p_format) {
		case Image::FORMAT_DXT1:
			return DXGI_BC1_UNORM;
		case Image::FORMAT_DXT3:
			return DXGI_BC2_UNORM;
		case Image::FORMAT_DXT5:
			return DXGI_BC3_UNORM;
		case Image::FORMAT_RGTC_R:
			return DXGI_BC4_UNORM;
		case Image::FORMAT_RGTC_RG:
			return DXGI_BC5_UNORM;
		case Image::FORMAT_BPTC_RGBFU:
			return DXGI_BC6H_UF16;
		case Image::FORMAT_BPTC_RGBF:
			return DXGI_BC6H_SF16;
		case Image::FORMAT_BPTC_RGBA:
			return DXGI_BC7_UNORM;
		case Image::FORMAT_RF:
			return DXGI_R32_FLOAT;
		case Image::FORMAT_RGF:
			return DXGI_R32G32_FLOAT;
		case Image::FORMAT_RGBF:
			return DXGI_R32G32B32_FLOAT;
		case Image::FORMAT_RGBAF:
			return DXGI_R32G32B32A32_FLOAT;
		case Image::FORMAT_RH:
			return DXGI_R16_FLOAT;
		case Image::FORMAT_RGH:
			return DXGI_R16G16_FLOAT;
		case Image::FORMAT_RGBAH:
			return DXGI_R16G16B16A16_FLOAT;
		case Image::FORMAT_RGBE9995:
			return DXGI_R9G9B9E5;
		case Image::FORMAT_R16:
			return DXGI_R16_UNORM;
		case Image::FORMAT_RG16:
			return DXGI_R16G16_UNORM;
		case Image::FORMAT_RGBA16:
			return DXGI_R16G16B16A16_UNORM;
		case Image::FORMAT_R16I:
			return DXGI_R16_UINT;
		case Image::FORMAT_RG16I:
			return DXGI_R16G16_UINT;
		case Image::FORMAT_RGBA16I:
			return DXGI_R16G16B16A16_UINT;

		default:
			return 0;
	}
}

void _get_dds_pixel_bitmask(Image::Format p_format, uint32_t &r_bit_count, uint32_t &r_red_mask, uint32_t &r_green_mask, uint32_t &r_blue_mask, uint32_t &r_alpha_mask) {
	switch (p_format) {
		case Image::FORMAT_R8:
		case Image::FORMAT_L8: {
			r_bit_count = 8;
			r_red_mask = 0xff;
			r_green_mask = 0;
			r_blue_mask = 0;
			r_alpha_mask = 0;
		} break;
		case Image::FORMAT_RG8:
		case Image::FORMAT_LA8: {
			r_bit_count = 16;
			r_red_mask = 0xff;
			r_green_mask = 0;
			r_blue_mask = 0;
			r_alpha_mask = 0xff00;
		} break;
		case Image::FORMAT_RGB8: {
			// BGR8
			r_bit_count = 24;
			r_red_mask = 0xff0000;
			r_green_mask = 0xff00;
			r_blue_mask = 0xff;
			r_alpha_mask = 0;
		} break;
		case Image::FORMAT_RGBA8: {
			r_bit_count = 32;
			r_red_mask = 0xff;
			r_green_mask = 0xff00;
			r_blue_mask = 0xff0000;
			r_alpha_mask = 0xff000000;
		} break;
		case Image::FORMAT_RGBA4444: {
			// BGRA4444
			r_bit_count = 16;
			r_red_mask = 0xf00;
			r_green_mask = 0xf0;
			r_blue_mask = 0xf;
			r_alpha_mask = 0xf000;
		} break;
		case Image::FORMAT_RGB565: {
			// BGR565
			r_bit_count = 16;
			r_red_mask = 0xf800;
			r_green_mask = 0x7e0;
			r_blue_mask = 0x1f;
			r_alpha_mask = 0;
		} break;
		case Image::FORMAT_R16: {
			r_bit_count = 16;
			r_red_mask = 0xffff;
			r_green_mask = 0;
			r_blue_mask = 0;
			r_alpha_mask = 0;
		} break;
		case Image::FORMAT_RG16: {
			r_bit_count = 32;
			r_red_mask = 0xffff;
			r_green_mask = 0xffff0000;
			r_blue_mask = 0;
			r_alpha_mask = 0;
		} break;

		default: {
			r_bit_count = 0;
			r_red_mask = 0;
			r_green_mask = 0;
			r_blue_mask = 0;
			r_alpha_mask = 0;
		} break;
	}
}

Vector<uint8_t> save_dds_buffer(const Ref<Image> &p_img) {
	Ref<StreamPeerBuffer> stream_buffer;
	stream_buffer.instantiate();

	Ref<Image> image = p_img;

	stream_buffer->put_u32(DDS_MAGIC);
	stream_buffer->put_u32(DDS_HEADER_SIZE);

	uint32_t flags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_PITCH | DDSD_LINEARSIZE;

	if (image->has_mipmaps()) {
		flags |= DDSD_MIPMAPCOUNT;
	}

	stream_buffer->put_u32(flags);

	uint32_t height = image->get_height();
	stream_buffer->put_u32(height);

	uint32_t width = image->get_width();
	stream_buffer->put_u32(width);

	DDSFormat dds_format = _image_format_to_dds_format(image->get_format());
	ERR_FAIL_COND_V_MSG(dds_format == DDS_MAX, Vector<uint8_t>(), vformat("DDS: Cannot save image with format %s.", Image::get_format_name(image->get_format())));

	const DDSFormatInfo &info = dds_format_info[dds_format];

	uint32_t depth = 1; // Default depth for 2D textures

	uint32_t pitch;
	if (info.compressed) {
		pitch = ((MAX(info.divisor, width) + info.divisor - 1) / info.divisor) * ((MAX(info.divisor, height) + info.divisor - 1) / info.divisor) * info.block_size;
	} else {
		pitch = width * info.block_size;
	}

	stream_buffer->put_u32(pitch);
	stream_buffer->put_u32(depth);

	uint32_t mipmaps = image->get_mipmap_count() + 1;
	stream_buffer->put_u32(mipmaps);

	uint32_t reserved = 0;
	for (int i = 0; i < 11; i++) {
		stream_buffer->put_u32(reserved);
	}

	stream_buffer->put_u32(DDS_PIXELFORMAT_SIZE);

	uint32_t pf_flags = 0;

	DDSFormatType format_type = _dds_format_get_type(dds_format);

	if (format_type == DDFT_BITMASK) {
		pf_flags = DDPF_RGB;

		if (image->get_format() == Image::FORMAT_LA8 || image->get_format() == Image::FORMAT_RG8 || image->get_format() == Image::FORMAT_RGBA8 || image->get_format() == Image::FORMAT_RGBA4444) {
			pf_flags |= DDPF_ALPHAPIXELS;
		}
	} else {
		pf_flags = DDPF_FOURCC;
	}

	stream_buffer->put_u32(pf_flags);

	bool needs_pixeldata_swap = false;

	if (format_type == DDFT_BITMASK) {
		// Uncompressed bitmasked.
		stream_buffer->put_u32(0); // FourCC

		uint32_t bit_count, r_mask, g_mask, b_mask, a_mask;
		_get_dds_pixel_bitmask(image->get_format(), bit_count, r_mask, g_mask, b_mask, a_mask);

		stream_buffer->put_u32(bit_count);
		stream_buffer->put_u32(r_mask);
		stream_buffer->put_u32(g_mask);
		stream_buffer->put_u32(b_mask);
		stream_buffer->put_u32(a_mask);

		if (image->get_format() == Image::FORMAT_RGBA4444 || image->get_format() == Image::FORMAT_RGB8) {
			needs_pixeldata_swap = true;
		}
	} else if (format_type == DDFT_FOURCC) {
		// FourCC.
		uint32_t fourcc = _image_format_to_fourcc_format(image->get_format());
		stream_buffer->put_u32(fourcc);

		stream_buffer->put_u32(0); // Bit count
		stream_buffer->put_u32(0); // R Bitmask
		stream_buffer->put_u32(0); // G Bitmask
		stream_buffer->put_u32(0); // B Bitmask
		stream_buffer->put_u32(0); // A Bitmask
	} else {
		// DXGI format and DX10 header.
		stream_buffer->put_u32(DDFCC_DX10);

		stream_buffer->put_u32(0); // Bit count
		stream_buffer->put_u32(0); // R Bitmask
		stream_buffer->put_u32(0); // G Bitmask
		stream_buffer->put_u32(0); // B Bitmask
		stream_buffer->put_u32(0); // A Bitmask
	}

	uint32_t caps1 = info.compressed ? DDSD_LINEARSIZE : DDSD_PITCH;
	stream_buffer->put_u32(caps1);

	stream_buffer->put_u32(0); // Caps2
	stream_buffer->put_u32(0); // Caps3
	stream_buffer->put_u32(0); // Caps4
	stream_buffer->put_u32(0); // Reserved 2

	if (format_type == DDFT_DXGI) {
		// DX10 header.
		uint32_t dxgi_format = _image_format_to_dxgi_format(image->get_format());
		stream_buffer->put_u32(dxgi_format);
		stream_buffer->put_u32(DX10D_2D);
		stream_buffer->put_u32(0); // Misc flags 1
		stream_buffer->put_u32(1); // Array size
		stream_buffer->put_u32(0); // Misc flags 2
	}

	for (uint32_t mip_i = 0; mip_i < mipmaps; mip_i++) {
		uint32_t mip_width = MAX(1u, width >> mip_i);
		uint32_t mip_height = MAX(1u, height >> mip_i);

		uint32_t expected_size = 0;
		if (info.compressed) {
			uint32_t blocks_x = (mip_width + info.divisor - 1) / info.divisor;
			uint32_t blocks_y = (mip_height + info.divisor - 1) / info.divisor;
			expected_size = blocks_x * blocks_y * info.block_size;
		} else {
			expected_size = mip_width * mip_height * info.block_size;
		}

		if (needs_pixeldata_swap) {
			// The image's channels need to be swapped.
			Ref<Image> mip_image = image->get_image_from_mipmap(mip_i);
			Vector<uint8_t> data = mip_image->get_data();

			ERR_FAIL_COND_V_MSG(data.size() != expected_size, Vector<uint8_t>(),
					"Image data size mismatch for mipmap level " + itos(mip_i) +
							". Expected size: " + itos(expected_size) + ", actual size: " + itos(data.size()) + ".");

			if (mip_image->get_format() == Image::FORMAT_RGBA4444) {
				// RGBA4 to BGRA4
				const int64_t data_size = data.size();
				uint8_t *wb = data.ptrw();

				for (int64_t data_i = 0; data_i < data_size; data_i += 2) {
					uint8_t ar = wb[data_i + 0];
					uint8_t gb = wb[data_i + 1];

					wb[data_i + 1] = ((ar & 0x0F) << 4) | ((gb & 0xF0) >> 4);
					wb[data_i + 0] = ((ar & 0xF0) >> 4) | ((gb & 0x0F) << 4);
				}
			} else if (mip_image->get_format() == Image::FORMAT_RGB8) {
				// RGB8 to BGR8
				const int64_t data_size = data.size();
				uint8_t *wb = data.ptrw();

				for (int64_t data_i = 0; data_i < data_size; data_i += 3) {
					SWAP(wb[data_i], wb[data_i + 2]);
				}
			}

			stream_buffer->put_data(data.ptr(), data.size());
		} else {
			int64_t ofs, size;

			image->get_mipmap_offset_and_size(mip_i, ofs, size);

			ERR_FAIL_COND_V_MSG(size != expected_size, Vector<uint8_t>(),
					"Image data size mismatch for mipmap level " + itos(mip_i) +
							". Expected size: " + itos(expected_size) + ", actual size: " + itos(size) + ".");

			stream_buffer->put_data(image->ptr() + ofs, size);
		}
	}

	return stream_buffer->get_data_array();
}

static Ref<ResourceFormatDDS> resource_loader_dds;

void initialize_dds_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	Image::save_dds_func = save_dds;
	Image::save_dds_buffer_func = save_dds_buffer;

	resource_loader_dds.instantiate();
	ResourceLoader::add_resource_format_loader(resource_loader_dds);
}

void uninitialize_dds_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	ResourceLoader::remove_resource_format_loader(resource_loader_dds);
	resource_loader_dds.unref();

	Image::save_dds_func = nullptr;
	Image::save_dds_buffer_func = nullptr;
}
