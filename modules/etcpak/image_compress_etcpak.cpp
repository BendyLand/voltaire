/**************************************************************************/
/*  image_compress_etcpak.cpp                                             */
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
// --- Inlined from modules/etcpak/image_compress_etcpak.h ---
#include "core/io/image.h"

#ifdef TOOLS_ENABLED

enum class EtcpakType {
	ETCPAK_TYPE_ETC1,
	ETCPAK_TYPE_ETC2,
	ETCPAK_TYPE_ETC2_ALPHA,
	ETCPAK_TYPE_ETC2_RA_AS_RG,
	ETCPAK_TYPE_ETC2_R,
	ETCPAK_TYPE_ETC2_RG,
	ETCPAK_TYPE_DXT1,
	ETCPAK_TYPE_DXT5,
	ETCPAK_TYPE_DXT5_RA_AS_RG,
	ETCPAK_TYPE_RGTC_R,
	ETCPAK_TYPE_RGTC_RG,
};

void _compress_etc1(Image *r_img);
void _compress_etc2(Image *r_img, Image::UsedChannels p_channels);
void _compress_bc(Image *r_img, Image::UsedChannels p_channels);

void _compress_etcpak(EtcpakType p_compress_type, Image *r_img);

#endif // TOOLS_ENABLED

enum EtcpakFormat {
	Etcpak_R,
	Etcpak_RG,
	Etcpak_RGB,
	Etcpak_RGBA,
};

void _decompress_etc(Image *p_image);

// --- End modules/etcpak/image_compress_etcpak.h ---
#include "register_types.h"

#ifdef TOOLS_ENABLED

#include "core/os/os.h"
#include "core/string/print_string.h"

#include <ProcessDxtc.hpp>
#include <ProcessRGB.hpp>

EtcpakType _determine_etc_type(Image::UsedChannels p_channels) {
	switch (p_channels) {
		case Image::USED_CHANNELS_L:
			return EtcpakType::ETCPAK_TYPE_ETC2;
		case Image::USED_CHANNELS_LA:
			return EtcpakType::ETCPAK_TYPE_ETC2_ALPHA;
		case Image::USED_CHANNELS_R:
			return EtcpakType::ETCPAK_TYPE_ETC2_R;
		case Image::USED_CHANNELS_RG:
			return EtcpakType::ETCPAK_TYPE_ETC2_RG;
		case Image::USED_CHANNELS_RGB:
			return EtcpakType::ETCPAK_TYPE_ETC2;
		case Image::USED_CHANNELS_RGBA:
			return EtcpakType::ETCPAK_TYPE_ETC2_ALPHA;

		default:
			return EtcpakType::ETCPAK_TYPE_ETC2_ALPHA;
	}
}

EtcpakType _determine_dxt_type(Image::UsedChannels p_channels) {
	switch (p_channels) {
		case Image::USED_CHANNELS_L:
			return EtcpakType::ETCPAK_TYPE_DXT1;
		case Image::USED_CHANNELS_LA:
			return EtcpakType::ETCPAK_TYPE_DXT5;
		case Image::USED_CHANNELS_R:
			return EtcpakType::ETCPAK_TYPE_RGTC_R;
		case Image::USED_CHANNELS_RG:
			return EtcpakType::ETCPAK_TYPE_RGTC_RG;
		case Image::USED_CHANNELS_RGB:
			return EtcpakType::ETCPAK_TYPE_DXT1;
		case Image::USED_CHANNELS_RGBA:
			return EtcpakType::ETCPAK_TYPE_DXT5;

		default:
			return EtcpakType::ETCPAK_TYPE_DXT5;
	}
}

void _compress_etc1(Image *r_img) {
	_compress_etcpak(EtcpakType::ETCPAK_TYPE_ETC1, r_img);
}

void _compress_etc2(Image *r_img, Image::UsedChannels p_channels) {
	_compress_etcpak(_determine_etc_type(p_channels), r_img);
}

void _compress_bc(Image *r_img, Image::UsedChannels p_channels) {
	_compress_etcpak(_determine_dxt_type(p_channels), r_img);
}

void _compress_etcpak(EtcpakType p_compress_type, Image *r_img) {
	uint64_t start_time = OS::get_ticks_msec();

	// The image is already compressed, return.
	if (r_img->is_compressed()) {
		return;
	}

	// Convert to RGBA8 for compression.
	r_img->convert(Image::FORMAT_RGBA8);

	// Determine output format based on Etcpak type.
	Image::Format target_format = Image::FORMAT_RGBA8;

	switch (p_compress_type) {
		case EtcpakType::ETCPAK_TYPE_ETC1:
			target_format = Image::FORMAT_ETC;
			break;

		case EtcpakType::ETCPAK_TYPE_ETC2:
			target_format = Image::FORMAT_ETC2_RGB8;
			break;

		case EtcpakType::ETCPAK_TYPE_ETC2_ALPHA:
			target_format = Image::FORMAT_ETC2_RGBA8;
			break;

		case EtcpakType::ETCPAK_TYPE_ETC2_R:
			target_format = Image::FORMAT_ETC2_R11;
			break;

		case EtcpakType::ETCPAK_TYPE_ETC2_RG:
			target_format = Image::FORMAT_ETC2_RG11;
			break;

		case EtcpakType::ETCPAK_TYPE_ETC2_RA_AS_RG:
			target_format = Image::FORMAT_ETC2_RA_AS_RG;
			r_img->convert_rg_to_ra_rgba8();
			break;

		case EtcpakType::ETCPAK_TYPE_DXT1:
			target_format = Image::FORMAT_DXT1;
			break;

		case EtcpakType::ETCPAK_TYPE_DXT5:
			target_format = Image::FORMAT_DXT5;
			break;

		case EtcpakType::ETCPAK_TYPE_RGTC_R:
			target_format = Image::FORMAT_RGTC_R;
			break;

		case EtcpakType::ETCPAK_TYPE_RGTC_RG:
			target_format = Image::FORMAT_RGTC_RG;
			break;

		case EtcpakType::ETCPAK_TYPE_DXT5_RA_AS_RG:
			target_format = Image::FORMAT_DXT5_RA_AS_RG;
			r_img->convert_rg_to_ra_rgba8();
			break;

		default:
			ERR_FAIL_MSG("Invalid or unsupported etcpak compression format, not ETC or DXT.");
			break;
	}

	// It's badly documented but ETCPAK seems to expect BGRA8 for ETC formats.
	if (p_compress_type < EtcpakType::ETCPAK_TYPE_DXT1) {
		r_img->convert_rgba8_to_bgra8();
	}

	// Compress image data and (if required) mipmaps.
	const bool has_mipmaps = r_img->has_mipmaps();
	int width = r_img->get_width();
	int height = r_img->get_height();

	/*
	The first mipmap level of a compressed texture must be a multiple of 4. Quote from D3D11.3 spec:

	BC format surfaces are always multiples of full blocks, each block representing 4x4 pixels.
	For mipmaps, the top level map is required to be a multiple of 4 size in all dimensions.
	The sizes for the lower level maps are computed as they are for all mipmapped surfaces,
	and thus may not be a multiple of 4, for example a top level map of 20 results in a second level
	map size of 10. For these cases, there is a differing 'physical' size and a 'virtual' size.
	The virtual size is that computed for each mip level without adjustment, which is 10 for the example.
	The physical size is the virtual size rounded up to the next multiple of 4, which is 12 for the example,
	and this represents the actual memory size. The sampling hardware will apply texture address
	processing based on the virtual size (using, for example, border color if specified for accesses
	beyond 10), and thus for the example case will not access the 11th and 12th row of the resource.
	So for mipmap chains when an axis becomes < 4 in size, only texels 'a','b','e','f'
	are used for a 2x2 map, and texel 'a' is used for 1x1. Note that this is similar to, but distinct from,
	the surface pitch, which can encompass additional padding beyond the physical surface size.
	*/

	if (width % 4 != 0 || height % 4 != 0) {
		width = width <= 2 ? width : (width + 3) & ~3;
		height = height <= 2 ? height : (height + 3) & ~3;
	}

	// Multiple-of-4 should be guaranteed by above.
	// However, power-of-two 3d textures will create Nx2 and Nx1 mipmap levels,
	// which are individually compressed Image objects that violate the above rule.
	// Hence, we allow Nx1 and Nx2 images through without forcing to multiple-of-4.

	// Create the buffer for compressed image data.
	Vector<uint8_t> dest_data;
	dest_data.resize(Image::get_image_data_size(width, height, target_format, has_mipmaps));
	uint8_t *dest_write = dest_data.ptrw();

	const uint8_t *src_read = r_img->get_data().ptr();

	const int mip_count = has_mipmaps ? Image::get_image_required_mipmaps(width, height, target_format) : 0;
	Vector<uint32_t> padded_src;

	for (int i = 0; i < mip_count + 1; i++) {
		// Get write mip metrics for target image.
		int dest_mip_w, dest_mip_h;
		int64_t dest_mip_ofs = Image::get_image_mipmap_offset_and_dimensions(width, height, target_format, i, dest_mip_w, dest_mip_h);

		// Ensure that mip offset is a multiple of 8 (etcpak expects uint64_t pointer).
		ERR_FAIL_COND(dest_mip_ofs % 8 != 0);
		uint64_t *dest_mip_write = reinterpret_cast<uint64_t *>(dest_write + dest_mip_ofs);

		// Block size.
		dest_mip_w = (dest_mip_w + 3) & ~3;
		dest_mip_h = (dest_mip_h + 3) & ~3;
		const uint32_t blocks = dest_mip_w * dest_mip_h / 16;

		// Get mip data from source image for reading.
		int64_t src_mip_ofs, src_mip_size;
		int src_mip_w, src_mip_h;

		r_img->get_mipmap_offset_size_and_dimensions(i, src_mip_ofs, src_mip_size, src_mip_w, src_mip_h);

		const uint32_t *src_mip_read = reinterpret_cast<const uint32_t *>(src_read + src_mip_ofs);

		// Pad textures to nearest block by smearing.
		if (dest_mip_w != src_mip_w || dest_mip_h != src_mip_h) {
			// Reserve the buffer for padded image data.
			padded_src.resize(dest_mip_w * dest_mip_h);
			uint32_t *ptrw = padded_src.ptrw();

			int x = 0, y = 0;
			for (y = 0; y < src_mip_h; y++) {
				for (x = 0; x < src_mip_w; x++) {
					ptrw[dest_mip_w * y + x] = src_mip_read[src_mip_w * y + x];
				}

				// First, smear in x.
				for (; x < dest_mip_w; x++) {
					ptrw[dest_mip_w * y + x] = ptrw[dest_mip_w * y + x - 1];
				}
			}

			// Then, smear in y.
			for (; y < dest_mip_h; y++) {
				for (x = 0; x < dest_mip_w; x++) {
					ptrw[dest_mip_w * y + x] = ptrw[dest_mip_w * y + x - dest_mip_w];
				}
			}

			// Override the src_mip_read pointer to our temporary Vector.
			src_mip_read = padded_src.ptr();
		}

		switch (p_compress_type) {
			case EtcpakType::ETCPAK_TYPE_ETC1:
				CompressEtc1RgbDither(src_mip_read, dest_mip_write, blocks, dest_mip_w);
				break;

			case EtcpakType::ETCPAK_TYPE_ETC2:
				CompressEtc2Rgb(src_mip_read, dest_mip_write, blocks, dest_mip_w, true);
				break;

			case EtcpakType::ETCPAK_TYPE_ETC2_ALPHA:
			case EtcpakType::ETCPAK_TYPE_ETC2_RA_AS_RG:
				CompressEtc2Rgba(src_mip_read, dest_mip_write, blocks, dest_mip_w, true);
				break;

			case EtcpakType::ETCPAK_TYPE_ETC2_R:
				CompressEacR(src_mip_read, dest_mip_write, blocks, dest_mip_w);
				break;

			case EtcpakType::ETCPAK_TYPE_ETC2_RG:
				CompressEacRg(src_mip_read, dest_mip_write, blocks, dest_mip_w);
				break;

			case EtcpakType::ETCPAK_TYPE_DXT1:
				CompressBc1Dither(src_mip_read, dest_mip_write, blocks, dest_mip_w);
				break;

			case EtcpakType::ETCPAK_TYPE_DXT5:
			case EtcpakType::ETCPAK_TYPE_DXT5_RA_AS_RG:
				CompressBc3(src_mip_read, dest_mip_write, blocks, dest_mip_w);
				break;

			case EtcpakType::ETCPAK_TYPE_RGTC_R:
				CompressBc4(src_mip_read, dest_mip_write, blocks, dest_mip_w);
				break;

			case EtcpakType::ETCPAK_TYPE_RGTC_RG:
				CompressBc5(src_mip_read, dest_mip_write, blocks, dest_mip_w);
				break;

			default:
				ERR_FAIL_MSG("etcpak: Invalid or unsupported compression format.");
				break;
		}
	}

	// Replace original image with compressed one.
	r_img->set_data(width, height, has_mipmaps, target_format, dest_data);

	print_verbose(vformat("etcpak: Encoding took %d ms.", OS::get_ticks_msec() - start_time));
}
#endif // TOOLS_ENABLED

#include "core/os/os.h"
#include "core/string/print_string.h"

#include <DecodeRGB.hpp>

#define ETCPAK_R_BLOCK_SIZE 8
#define ETCPAK_RG_BLOCK_SIZE 16
#define ETCPAK_RGB_BLOCK_SIZE 8
#define ETCPAK_RGBA_BLOCK_SIZE 16

template <void (*decompress_func)(const void *, void *, size_t), int block_size, int pixel_size>
static inline void _safe_decompress_mipmap(int width, int height, const uint8_t *src, uint8_t *dst) {
	// A stack-allocated output buffer large enough to contain an entire uncompressed block.
	uint8_t temp_buf[4 * 4 * pixel_size];

	// The amount of misaligned pixels on each axis.
	const int width_diff = width - (width & ~0x03);
	const int height_diff = height - (height & ~0x03);

	// The amount of uncompressed blocks on each axis.
	const int width_blocks = (width & ~0x03) / 4;
	const int height_blocks = (height & ~0x03) / 4;

	// The pitch of the image in bytes.
	const int image_pitch = width * pixel_size;
	// The pitch of a block in bytes.
	const int block_pitch = 4 * pixel_size;
	// The pitch of the last block in bytes.
	const int odd_pitch = width_diff * pixel_size;

	size_t src_pos = 0;
	size_t dst_pos = 0;

	// Decompress the blocks, starting from the top.
	for (int y = 0; y < height_blocks; y += 1) {
		// Decompress the blocks, starting from the left.
		for (int x = 0; x < width_blocks; x += 1) {
			decompress_func(&src[src_pos], &dst[dst_pos], width);
			src_pos += block_size;
			dst_pos += block_pitch;
		}

		// Decompress the block on the right.
		if (width_diff > 0) {
			decompress_func(&src[src_pos], temp_buf, 4);

			// Copy the data from the temporary buffer to the output.
			for (int i = 0; i < 4; i++) {
				memcpy(&dst[dst_pos + i * image_pitch], &temp_buf[i * block_pitch], odd_pitch);
			}

			src_pos += block_size;
			dst_pos += odd_pitch;
		}

		// Skip to the next row of blocks, the current one has already been filled.
		dst_pos += 3 * image_pitch;
	}

	// Decompress the blocks at the bottom of the image.
	if (height_diff > 0) {
		// Decompress the blocks at the bottom.
		for (int x = 0; x < width_blocks; x += 1) {
			decompress_func(&src[src_pos], temp_buf, 4);

			// Copy the data from the temporary buffer to the output.
			for (int i = 0; i < height_diff; i++) {
				memcpy(&dst[dst_pos + i * image_pitch], &temp_buf[i * block_pitch], block_pitch);
			}

			src_pos += block_size;
			dst_pos += block_pitch;
		}

		// Decompress the block in the lower-right corner.
		if (width_diff > 0) {
			decompress_func(&src[src_pos], temp_buf, 4);

			// Copy the data from the temporary buffer to the output.
			for (int i = 0; i < height_diff; i++) {
				memcpy(&dst[dst_pos + i * image_pitch], &temp_buf[i * block_pitch], odd_pitch);
			}

			src_pos += block_size;
			dst_pos += odd_pitch;
		}
	}
}

template <void (*decompress_func)(const void *, void *, size_t), int block_size, int pixel_size>
static inline void _decompress_mipmap(int width, int height, const uint8_t *src, uint8_t *dst) {
	size_t src_pos = 0;
	size_t dst_pos = 0;

	// The size of a single block in bytes.
	const int block_pitch = 4 * pixel_size;

	for (int y = 0; y < height; y += 4) {
		for (int x = 0; x < width; x += 4) {
			decompress_func(&src[src_pos], &dst[dst_pos], width);
			src_pos += block_size;
			dst_pos += block_pitch;
		}

		// Skip to the next row of blocks, the current one has already been filled.
		dst_pos += 3 * width * pixel_size;
	}
}

static void decompress_image(EtcpakFormat format, const void *src, void *dst, const uint64_t width, const uint64_t height) {
	const uint8_t *src_blocks = reinterpret_cast<const uint8_t *>(src);
	uint8_t *dec_blocks = reinterpret_cast<uint8_t *>(dst);

	const uint64_t aligned_width = (width + 3) & ~0x03;
	const uint64_t aligned_height = (height + 3) & ~0x03;

	if (width != aligned_width || height != aligned_height) {
		switch (format) {
			case Etcpak_R: {
				_safe_decompress_mipmap<DecodeRBlock, ETCPAK_R_BLOCK_SIZE, 4>(width, height, src_blocks, dec_blocks);
			} break;
			case Etcpak_RG: {
				_safe_decompress_mipmap<DecodeRGBlock, ETCPAK_RG_BLOCK_SIZE, 4>(width, height, src_blocks, dec_blocks);
			} break;
			case Etcpak_RGB: {
				_safe_decompress_mipmap<DecodeRGBBlock, ETCPAK_RGB_BLOCK_SIZE, 4>(width, height, src_blocks, dec_blocks);
			} break;
			case Etcpak_RGBA: {
				_safe_decompress_mipmap<DecodeRGBABlock, ETCPAK_RGBA_BLOCK_SIZE, 4>(width, height, src_blocks, dec_blocks);
			} break;
		}
	} else {
		switch (format) {
			case Etcpak_R: {
				_decompress_mipmap<DecodeRBlock, ETCPAK_R_BLOCK_SIZE, 4>(width, height, src_blocks, dec_blocks);
			} break;
			case Etcpak_RG: {
				_decompress_mipmap<DecodeRGBlock, ETCPAK_RG_BLOCK_SIZE, 4>(width, height, src_blocks, dec_blocks);
			} break;
			case Etcpak_RGB: {
				_decompress_mipmap<DecodeRGBBlock, ETCPAK_RGB_BLOCK_SIZE, 4>(width, height, src_blocks, dec_blocks);
			} break;
			case Etcpak_RGBA: {
				_decompress_mipmap<DecodeRGBABlock, ETCPAK_RGBA_BLOCK_SIZE, 4>(width, height, src_blocks, dec_blocks);
			} break;
		}
	}
}

void _decompress_etc(Image *p_image) {
	uint64_t start_time = OS::get_ticks_msec();

	int width = p_image->get_width();
	int height = p_image->get_height();

	Image::Format source_format = p_image->get_format();
	Image::Format target_format = Image::FORMAT_RGBA8;

	EtcpakFormat etcpak_format = Etcpak_R;

	switch (source_format) {
		case Image::FORMAT_ETC:
		case Image::FORMAT_ETC2_RGB8:
			etcpak_format = Etcpak_RGB;
			break;

		case Image::FORMAT_ETC2_RGBA8:
		case Image::FORMAT_ETC2_RA_AS_RG:
			etcpak_format = Etcpak_RGBA;
			break;

		case Image::FORMAT_ETC2_R11:
			etcpak_format = Etcpak_R;
			break;

		case Image::FORMAT_ETC2_RG11:
			etcpak_format = Etcpak_RG;
			break;

		default:
			ERR_FAIL_MSG(vformat("etcpak: Can't decompress image %s with an unknown format: %s.", p_image->get_path(), Image::get_format_name(source_format)));
			break;
	}

	int mm_count = p_image->get_mipmap_count();
	int64_t target_size = Image::get_image_data_size(width, height, target_format, p_image->has_mipmaps());

	// Decompressed data.
	Vector<uint8_t> data;
	data.resize(target_size);
	uint8_t *wb = data.ptrw();

	// Source data.
	const uint8_t *rb = p_image->ptr();

	// Decompress mipmaps.
	for (int i = 0; i <= mm_count; i++) {
		int mipmap_w = 0, mipmap_h = 0;
		int64_t src_ofs = Image::get_image_mipmap_offset(width, height, source_format, i);
		int64_t dst_ofs = Image::get_image_mipmap_offset_and_dimensions(width, height, target_format, i, mipmap_w, mipmap_h);
		decompress_image(etcpak_format, rb + src_ofs, wb + dst_ofs, mipmap_w, mipmap_h);
	}

	p_image->set_data(p_image->get_width(), p_image->get_height(), p_image->has_mipmaps(), target_format, data);

	// Swap channels if the format is using a channel swizzle.
	if (source_format == Image::FORMAT_ETC2_RA_AS_RG) {
		p_image->convert_ra_rgba8_to_rg();
	}

	print_verbose(vformat("etcpak: Decompression of %dx%d %s image %s with %d mipmaps took %d ms.",
			p_image->get_width(), p_image->get_height(), Image::get_format_name(source_format), p_image->get_path(), p_image->get_mipmap_count(), OS::get_ticks_msec() - start_time));
}

void initialize_etcpak_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

#ifdef TOOLS_ENABLED
	Image::_image_compress_etc1_func = _compress_etc1;
	Image::_image_compress_etc2_func = _compress_etc2;
	Image::_image_compress_bc_func = _compress_bc;
#endif

	Image::_image_decompress_etc1 = _decompress_etc;
	Image::_image_decompress_etc2 = _decompress_etc;
}

void uninitialize_etcpak_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}
