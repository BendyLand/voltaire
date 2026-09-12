/**************************************************************************/
/*  image_compress_basisu.cpp                                             */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
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

#include "core/config/project_settings.h"
#include "core/io/image.h"
#include "core/os/os.h"
#include "core/string/print_string.h"
#include "image_compress_basisu.h"
#include "servers/rendering/rendering_server.h"

VLTR_GCC_WARNING_PUSH
VLTR_GCC_WARNING_IGNORE("-Wenum-conversion")
VLTR_GCC_WARNING_IGNORE("-Wshadow")
VLTR_GCC_WARNING_IGNORE("-Wunused-value")

#include <transcoder/basisu_transcoder.h>
#ifdef TOOLS_ENABLED
#include <encoder/basisu_comp.h>

static Mutex init_mutex;
static bool initialized = false;
#endif

VLTR_GCC_WARNING_POP

void basis_universal_init() { basist::basisu_transcoder_init(); }

#ifdef TOOLS_ENABLED
template <typename T>
inline void _basisu_pad_mipmap(const uint8_t* p_image_mip_data, Vector<uint8_t>& r_mip_data_padded,
	int p_next_width, int p_next_height, int p_width, int p_height, int64_t p_size)
{
	// Source mip's data interpreted as 32-bit RGBA blocks to help with copying pixel data.
	const T* mip_src_data = reinterpret_cast<const T*>(p_image_mip_data);

	// Reserve space in the padded buffer.
	r_mip_data_padded.resize(p_next_width * p_next_height * sizeof(T));
	T* data_padded_ptr = reinterpret_cast<T*>(r_mip_data_padded.ptrw());

	// Pad mipmap to the nearest block by smearing.
	int x = 0, y = 0;
	for (y = 0; y < p_height; y++) {
		for (x = 0; x < p_width; x++) {
			data_padded_ptr[p_next_width * y + x] = mip_src_data[p_width * y + x];
		}

		// First, smear in x.
		for (; x < p_next_width; x++) {
			data_padded_ptr[p_next_width * y + x] = data_padded_ptr[p_next_width * y + x - 1];
		}
	}

	// Then, smear in y.
	for (; y < p_next_height; y++) {
		for (x = 0; x < p_next_width; x++) {
			data_padded_ptr[p_next_width * y + x] =
				data_padded_ptr[p_next_width * y + x - p_next_width];
		}
	}
}

#endif // TOOLS_ENABLED

Ref<Image> basis_universal_unpacker_ptr(const uint8_t* p_data, int p_size)
{
	uint64_t start_time = OS::get_singleton()->get_ticks_msec();

	Ref<Image> image;
	ERR_FAIL_NULL_V_MSG(p_data, image, "Cannot unpack invalid BasisUniversal data.");

	const uint8_t* src_ptr = p_data;
	int src_size = p_size;

	basist::transcoder_texture_format basisu_format =
		basist::transcoder_texture_format::cTFTotalTextureFormats;
	Image::Format image_format = Image::FORMAT_MAX;

	// Get supported compression formats.
	bool bptc_supported = RS::get_singleton()->has_os_feature("bptc");
	bool astc_supported = RS::get_singleton()->has_os_feature("astc");
	bool rgtc_supported = RS::get_singleton()->has_os_feature("rgtc");
	bool s3tc_supported = RS::get_singleton()->has_os_feature("s3tc");
	bool etc2_supported = RS::get_singleton()->has_os_feature("etc2");
	bool astc_hdr_supported = RS::get_singleton()->has_os_feature("astc_hdr");

	bool needs_ra_rg_swap = false;
	bool needs_rg_trim = false;

	uint32_t decompress_format = *(uint32_t*)(src_ptr);
	bool is_ktx2 = decompress_format & BASIS_DECOMPRESS_FLAG_KTX2;
	decompress_format &= ~BASIS_DECOMPRESS_FLAG_KTX2;

	switch (decompress_format) {
	case BASIS_DECOMPRESS_R: {
		if (etc2_supported) {
			basisu_format = basist::transcoder_texture_format::cTFETC2_EAC_R11;
			image_format = Image::FORMAT_ETC2_R11;
		}
		else if (rgtc_supported) {
			basisu_format = basist::transcoder_texture_format::cTFBC4_R;
			image_format = Image::FORMAT_RGTC_R;
		}
		else if (s3tc_supported) {
			basisu_format = basist::transcoder_texture_format::cTFBC1;
			image_format = Image::FORMAT_DXT1;
		}
		else {
			// No supported VRAM compression formats, decompress.
			basisu_format = basist::transcoder_texture_format::cTFRGBA32;
			image_format = Image::FORMAT_RGBA8;
			needs_rg_trim = true;
		}

	} break;
	case BASIS_DECOMPRESS_RG:
	case BASIS_DECOMPRESS_RG_AS_RA: {
		if (etc2_supported) {
			basisu_format = basist::transcoder_texture_format::cTFETC2_EAC_RG11;
			image_format = Image::FORMAT_ETC2_RG11;
		}
		else if (rgtc_supported) {
			basisu_format = basist::transcoder_texture_format::cTFBC5_RG;
			image_format = Image::FORMAT_RGTC_RG;
		}
		else if (s3tc_supported) {
			basisu_format = basist::transcoder_texture_format::cTFBC3;
			image_format = Image::FORMAT_DXT5_RA_AS_RG;
		}
		else {
			// No supported VRAM compression formats, decompress.
			basisu_format = basist::transcoder_texture_format::cTFRGBA32;
			image_format = Image::FORMAT_RGBA8;
			needs_ra_rg_swap = true;
			needs_rg_trim = true;
		}

	} break;
	case BASIS_DECOMPRESS_RGB: {
		if (astc_supported) {
			basisu_format = basist::transcoder_texture_format::cTFASTC_4x4_RGBA;
			image_format = Image::FORMAT_ASTC_4x4;
		}
		else if (bptc_supported) {
			basisu_format = basist::transcoder_texture_format::cTFBC7_M6_OPAQUE_ONLY;
			image_format = Image::FORMAT_BPTC_RGBA;
		}
		else if (etc2_supported) {
			basisu_format = basist::transcoder_texture_format::cTFETC1;
			image_format = Image::FORMAT_ETC2_RGB8;
		}
		else if (s3tc_supported) {
			basisu_format = basist::transcoder_texture_format::cTFBC1;
			image_format = Image::FORMAT_DXT1;
		}
		else {
			// No supported VRAM compression formats, decompress.
			basisu_format = basist::transcoder_texture_format::cTFRGBA32;
			image_format = Image::FORMAT_RGBA8;
		}

	} break;
	case BASIS_DECOMPRESS_RGBA: {
		if (astc_supported) {
			basisu_format = basist::transcoder_texture_format::cTFASTC_4x4_RGBA;
			image_format = Image::FORMAT_ASTC_4x4;
		}
		else if (bptc_supported) {
			basisu_format = basist::transcoder_texture_format::cTFBC7_M5;
			image_format = Image::FORMAT_BPTC_RGBA;
		}
		else if (etc2_supported) {
			basisu_format = basist::transcoder_texture_format::cTFETC2;
			image_format = Image::FORMAT_ETC2_RGBA8;
		}
		else if (s3tc_supported) {
			basisu_format = basist::transcoder_texture_format::cTFBC3;
			image_format = Image::FORMAT_DXT5;
		}
		else {
			// No supported VRAM compression formats, decompress.
			basisu_format = basist::transcoder_texture_format::cTFRGBA32;
			image_format = Image::FORMAT_RGBA8;
		}

	} break;
	case BASIS_DECOMPRESS_HDR_RGB: {
		if (astc_hdr_supported) {
			basisu_format = basist::transcoder_texture_format::cTFASTC_HDR_4x4_RGBA;
			image_format = Image::FORMAT_ASTC_4x4_HDR;
		}
		else if (bptc_supported) {
			basisu_format = basist::transcoder_texture_format::cTFBC6H;
			image_format = Image::FORMAT_BPTC_RGBFU;
		}
		else {
			// No supported VRAM compression formats, decompress.
			basisu_format = basist::transcoder_texture_format::cTFRGB_9E5;
			image_format = Image::FORMAT_RGBE9995;
		}

	} break;
	default: {
		ERR_FAIL_V(image);
	} break;
	}

	src_ptr += 4;
	src_size -= 4;

	if (is_ktx2) {
		basist::ktx2_transcoder transcoder;
		ERR_FAIL_COND_V(!transcoder.init(src_ptr, src_size), image);

		transcoder.start_transcoding();

		// Create the buffer for transcoded/decompressed data.
		Vector<uint8_t> out_data;
		out_data.resize(Image::get_image_data_size(transcoder.get_width(), transcoder.get_height(),
			image_format, transcoder.get_levels() > 1));

		uint8_t* dst = out_data.ptrw();
		memset(dst, 0, out_data.size());

		for (uint32_t i = 0; i < transcoder.get_levels(); i++) {
			basist::ktx2_image_level_info basisu_level;
			transcoder.get_image_level_info(basisu_level, i, 0, 0);

			uint32_t mip_block_or_pixel_count =
				Image::is_format_compressed(image_format)
					? basisu_level.m_total_blocks
					: basisu_level.m_orig_width * basisu_level.m_orig_height;
			int64_t ofs = Image::get_image_mipmap_offset(
				transcoder.get_width(), transcoder.get_height(), image_format, i);

			bool result = transcoder.transcode_image_level(
				i, 0, 0, dst + ofs, mip_block_or_pixel_count, basisu_format);

			if (!result) {
				__print_line(vformat("BasisUniversal cannot unpack level %d.", i));
				break;
			}
		}

		image = Image::create_from_data(transcoder.get_width(), transcoder.get_height(),
			transcoder.get_levels() > 1, image_format, out_data);
	}
	else {
		basist::basisu_transcoder transcoder;
		ERR_FAIL_COND_V(!transcoder.validate_header(src_ptr, src_size), image);

		transcoder.start_transcoding(src_ptr, src_size);

		basist::basisu_image_info basisu_info;
		transcoder.get_image_info(src_ptr, src_size, basisu_info, 0);

		// Create the buffer for transcoded/decompressed data.
		Vector<uint8_t> out_data;
		out_data.resize(Image::get_image_data_size(basisu_info.m_width, basisu_info.m_height,
			image_format, basisu_info.m_total_levels > 1));

		uint8_t* dst = out_data.ptrw();
		memset(dst, 0, out_data.size());

		for (uint32_t i = 0; i < basisu_info.m_total_levels; i++) {
			basist::basisu_image_level_info basisu_level;
			transcoder.get_image_level_info(src_ptr, src_size, basisu_level, 0, i);

			uint32_t mip_block_or_pixel_count =
				Image::is_format_compressed(image_format)
					? basisu_level.m_total_blocks
					: basisu_level.m_orig_width * basisu_level.m_orig_height;
			int64_t ofs = Image::get_image_mipmap_offset(
				basisu_info.m_width, basisu_info.m_height, image_format, i);

			bool result = transcoder.transcode_image_level(
				src_ptr, src_size, 0, i, dst + ofs, mip_block_or_pixel_count, basisu_format);

			if (!result) {
				__print_line(vformat("BasisUniversal cannot unpack level %d.", i));
				break;
			}
		}

		image = Image::create_from_data(basisu_info.m_width, basisu_info.m_height,
			basisu_info.m_total_levels > 1, image_format, out_data);
	}

	if (needs_ra_rg_swap) {
		// Swap uncompressed RA-as-RG texture's color channels.
		image->convert_ra_rgba8_to_rg();
	}

	if (needs_rg_trim) {
		// Remove unnecessary color channels from uncompressed textures.
		if (decompress_format == BASIS_DECOMPRESS_R) {
			image->convert(Image::FORMAT_R8);
		}
		else if (decompress_format == BASIS_DECOMPRESS_RG ||
				   decompress_format == BASIS_DECOMPRESS_RG_AS_RA) {
			image->convert(Image::FORMAT_RG8);
		}
	}

	print_verbose(vformat("BasisU: Transcoding a %dx%d image with %d mipmaps into %s took %d ms.",
		image->get_width(), image->get_height(), image->get_mipmap_count(),
		Image::get_format_name(image_format), OS::get_singleton()->get_ticks_msec() - start_time));

	return image;
}

Ref<Image> basis_universal_unpacker(const Vector<uint8_t>& p_buffer)
{
	return basis_universal_unpacker_ptr(p_buffer.ptr(), p_buffer.size());
}


