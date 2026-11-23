#include "modules/modules_enabled.gen.h"

#if defined(MODULE_ASTCENC_ENABLED)
// --- Inlined from modules/astcenc/image_compress_astcenc.h ---
#include "core/io/image.h"

#ifdef TOOLS_ENABLED
void _compress_astc(Image *r_img, Image::UsedChannels p_channels, Image::CompressProfile p_profile);
#endif

void _decompress_astc(Image *r_img);

// --- End modules/astcenc/image_compress_astcenc.h ---
#include "modules/register_module_types.h"

#include "core/os/os.h"
#include "core/string/print_string.h"

#include <astcenc.h>

#ifdef TOOLS_ENABLED
Image::CompressProfile _pick_profile_from_channels(Image::UsedChannels p_channels) {
	switch (p_channels) {
		case Image::USED_CHANNELS_L:
		case Image::USED_CHANNELS_R:
			return Image::COMPRESS_PROFILE_MAX_COMPRESSION;
		case Image::USED_CHANNELS_LA:
		case Image::USED_CHANNELS_RG:
			return Image::COMPRESS_PROFILE_COMPRESSED;
		case Image::USED_CHANNELS_RGB:
		case Image::USED_CHANNELS_RGBA:
		default:
			return Image::COMPRESS_PROFILE_MAX_QUALITY;
	}
}

void _compress_astc(Image *r_img, Image::UsedChannels p_channels, Image::CompressProfile p_profile) {
	const uint64_t start_time = OS::get_ticks_msec();

	if (r_img->is_compressed()) {
		return; // Do not compress, already compressed.
	}

	const Image::Format src_format = r_img->get_format();
	const bool is_hdr = src_format >= Image::FORMAT_RF && src_format <= Image::FORMAT_RGBE9995;

	if (src_format >= Image::FORMAT_RH && src_format <= Image::FORMAT_RGBAH) {
		r_img->convert(Image::FORMAT_RGBAH);
	} else if (src_format >= Image::FORMAT_RF && src_format <= Image::FORMAT_RGBE9995) {
		r_img->convert(Image::FORMAT_RGBAF);
	} else {
		r_img->convert(Image::FORMAT_RGBA8);
	}

	// Determine encoder output format from our enum.
	const astcenc_profile profile = is_hdr ? ASTCENC_PRF_HDR : ASTCENC_PRF_LDR;

	Image::Format target_format = Image::FORMAT_MAX;
	unsigned int block_x = 1, block_y = 1;
	if (p_profile == Image::COMPRESS_PROFILE_AUTOMATIC) {
		// Automatic profile will automatically pick one of the other profiles based on the amount of channels currently in use.
		p_profile = _pick_profile_from_channels(p_channels);
	}

	switch (p_profile) {
		case Image::COMPRESS_PROFILE_MAX_QUALITY:
			target_format = is_hdr ? Image::FORMAT_ASTC_4x4_HDR : Image::FORMAT_ASTC_4x4;
			block_x = block_y = 4;
			break;
		case Image::COMPRESS_PROFILE_COMPRESSED:
			target_format = is_hdr ? Image::FORMAT_ASTC_6x6_HDR : Image::FORMAT_ASTC_6x6;
			block_x = block_y = 6;
			break;
		case Image::COMPRESS_PROFILE_MAX_COMPRESSION:
			target_format = is_hdr ? Image::FORMAT_ASTC_8x8_HDR : Image::FORMAT_ASTC_8x8;
			block_x = block_y = 8;
			break;
		default:
			break;
	}

	// Compress image data and (if required) mipmaps.
	const bool has_mipmaps = r_img->has_mipmaps();
	int width = r_img->get_width();
	int height = r_img->get_height();
	int required_width = (width % block_x) != 0 ? width + (block_x - (width % block_x)) : width;
	int required_height = (height % block_y) != 0 ? height + (block_y - (height % block_y)) : height;

	if (width != required_width || height != required_height) {
		// Resize texture to fit block size.
		r_img->resize(required_width, required_height);
		width = required_width;
		height = required_height;
	}

	print_verbose(vformat("astcenc: Encoding image size %dx%d to format %s%s.", width, height, Image::get_format_name(target_format), has_mipmaps ? ", with mipmaps" : ""));

	// Initialize astcenc.
	const int64_t dest_size = Image::get_image_data_size(width, height, target_format, has_mipmaps);
	Vector<uint8_t> dest_data;
	dest_data.resize(dest_size);
	uint8_t *dest_write = dest_data.ptrw();

	astcenc_config config;
	config.block_x = block_x;
	config.block_y = block_y;
	config.profile = profile;

	const float quality = ASTCENC_PRE_MEDIUM;
	astcenc_error status = astcenc_config_init(profile, block_x, block_y, 1, quality, 0, &config);
	ERR_FAIL_COND_MSG(status != ASTCENC_SUCCESS,
			vformat("astcenc: Configuration initialization failed: %s.", astcenc_get_error_string(status)));

	// Context allocation.
	astcenc_context *context;
	const unsigned int thread_count = 1; // Godot compresses multiple images each on a thread, which is more efficient for large amount of images imported.
	status = astcenc_context_alloc(&config, thread_count, &context);
	ERR_FAIL_COND_MSG(status != ASTCENC_SUCCESS,
			vformat("astcenc: Context allocation failed: %s.", astcenc_get_error_string(status)));

	const int mip_count = has_mipmaps ? Image::get_image_required_mipmaps(width, height, target_format) : 0;
	const uint8_t *src_data = r_img->ptr();

	for (int i = 0; i < mip_count + 1; i++) {
		int src_mip_w, src_mip_h;
		const int64_t src_ofs = Image::get_image_mipmap_offset_and_dimensions(width, height, r_img->get_format(), i, src_mip_w, src_mip_h);
		const uint8_t *mip_data = &src_data[src_ofs];

		const int64_t dst_ofs = Image::get_image_mipmap_offset(width, height, target_format, i);
		uint8_t *dest_mip_write = &dest_write[dst_ofs];

		// Ensure that mip offset is a multiple of 8 (etcpak expects uint64_t pointer).
		if (unlikely(dst_ofs % 8 != 0)) {
			astcenc_context_free(context);
			ERR_FAIL_MSG("astcenc: Mip offset is not a multiple of 8.");
		}

		// Compress image.
		astcenc_image image;
		image.dim_x = src_mip_w;
		image.dim_y = src_mip_h;
		image.dim_z = 1;

		if (r_img->get_format() == Image::FORMAT_RGBA8) {
			image.data_type = ASTCENC_TYPE_U8;
		} else if (r_img->get_format() == Image::FORMAT_RGBAH) {
			image.data_type = ASTCENC_TYPE_F16;
		} else {
			image.data_type = ASTCENC_TYPE_F32;
		}

		image.data = (void **)(&mip_data);

		// Compute the number of ASTC blocks in each dimension.
		unsigned int block_count_x = (src_mip_w + block_x - 1) / block_x;
		unsigned int block_count_y = (src_mip_h + block_y - 1) / block_y;
		size_t comp_len = block_count_x * block_count_y * 16;

		const astcenc_swizzle swizzle = {
			ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A
		};

		status = astcenc_compress_image(context, &image, &swizzle, dest_mip_write, comp_len, 0);
		ERR_BREAK_MSG(status != ASTCENC_SUCCESS,
				vformat("astcenc: ASTC image compression failed: %s.", astcenc_get_error_string(status)));

		astcenc_compress_reset(context);
	}

	astcenc_context_free(context);

	// Replace original image with compressed one.
	r_img->set_data(width, height, has_mipmaps, target_format, dest_data);

	print_verbose(vformat("astcenc: Encoding took %d ms.", OS::get_ticks_msec() - start_time));
}
#endif // TOOLS_ENABLED

void _decompress_astc(Image *r_img) {
	const uint64_t start_time = OS::get_ticks_msec();

	// Determine decompression parameters from image format.
	const Image::Format src_format = r_img->get_format();

	bool is_hdr = false;
	unsigned int block_x = 0;
	unsigned int block_y = 0;

	switch (src_format) {
		case Image::FORMAT_ASTC_4x4: {
			block_x = 4;
			block_y = 4;
			is_hdr = false;
		} break;
		case Image::FORMAT_ASTC_4x4_HDR: {
			block_x = 4;
			block_y = 4;
			is_hdr = true;
		} break;
		case Image::FORMAT_ASTC_6x6: {
			block_x = 6;
			block_y = 6;
			is_hdr = false;
		} break;
		case Image::FORMAT_ASTC_6x6_HDR: {
			block_x = 6;
			block_y = 6;
			is_hdr = true;
		} break;
		case Image::FORMAT_ASTC_8x8: {
			block_x = 8;
			block_y = 8;
			is_hdr = false;
		} break;
		case Image::FORMAT_ASTC_8x8_HDR: {
			block_x = 8;
			block_y = 8;
			is_hdr = true;
		} break;
		default: {
			ERR_FAIL_MSG(vformat("astcenc: Cannot decompress Image with a non-ASTC format: %s.", Image::get_format_name(src_format)));
		} break;
	}

	// Initialize astcenc.
	const astcenc_profile profile = is_hdr ? ASTCENC_PRF_HDR : ASTCENC_PRF_LDR;

	astcenc_config config;
	const float quality = ASTCENC_PRE_MEDIUM;
	const uint32_t flags = ASTCENC_FLG_DECOMPRESS_ONLY;

	astcenc_error status = astcenc_config_init(profile, block_x, block_y, 1, quality, flags, &config);
	ERR_FAIL_COND_MSG(status != ASTCENC_SUCCESS,
			vformat("astcenc: Configuration initialization failed: %s.", astcenc_get_error_string(status)));

	// Context allocation.
	astcenc_context *context = nullptr;
	const unsigned int thread_count = 1;

	status = astcenc_context_alloc(&config, thread_count, &context);
	ERR_FAIL_COND_MSG(status != ASTCENC_SUCCESS,
			vformat("astcenc: Context allocation failed: %s.", astcenc_get_error_string(status)));

	const Image::Format target_format = is_hdr ? Image::FORMAT_RGBAH : Image::FORMAT_RGBA8;

	const bool has_mipmaps = r_img->has_mipmaps();
	int width = r_img->get_width();
	int height = r_img->get_height();

	const int64_t dest_size = Image::get_image_data_size(width, height, target_format, has_mipmaps);
	Vector<uint8_t> dest_data;
	dest_data.resize(dest_size);
	uint8_t *dest_write = dest_data.ptrw();

	// Decompress image.
	const int mip_count = has_mipmaps ? Image::get_image_required_mipmaps(width, height, target_format) : 0;
	const uint8_t *src_data = r_img->ptr();

	for (int i = 0; i < mip_count + 1; i++) {
		const int64_t src_ofs = Image::get_image_mipmap_offset(width, height, src_format, i);
		const uint8_t *mip_data = &src_data[src_ofs];

		int64_t src_size;
		if (i == mip_count) {
			src_size = r_img->get_data_size() - src_ofs;
		} else {
			src_size = Image::get_image_mipmap_offset(width, height, src_format, i + 1) - src_ofs;
		}

		int dst_mip_w, dst_mip_h;
		const int64_t dst_ofs = Image::get_image_mipmap_offset_and_dimensions(width, height, target_format, i, dst_mip_w, dst_mip_h);

		// Ensure that mip offset is a multiple of 8 (etcpak expects uint64_t pointer).
		ERR_FAIL_COND(dst_ofs % 8 != 0);
		uint8_t *dest_mip_write = &dest_write[dst_ofs];

		astcenc_image image;
		image.dim_x = dst_mip_w;
		image.dim_y = dst_mip_h;
		image.dim_z = 1;
		image.data_type = is_hdr ? ASTCENC_TYPE_F16 : ASTCENC_TYPE_U8;

		image.data = (void **)(&dest_mip_write);

		const astcenc_swizzle swizzle = {
			ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A
		};

		status = astcenc_decompress_image(context, mip_data, src_size, &image, &swizzle, 0);
		ERR_BREAK_MSG(status != ASTCENC_SUCCESS, vformat("astcenc: ASTC decompression failed: %s.", astcenc_get_error_string(status)));
		ERR_BREAK_MSG(image.dim_z > 1, "astcenc: ASTC decompression failed because this is a 3D texture, which is not supported.");

		astcenc_compress_reset(context);
	}

	astcenc_context_free(context);

	// Replace original image with compressed one.
	r_img->set_data(width, height, has_mipmaps, target_format, dest_data);

	print_verbose(vformat("astcenc: Decompression took %d ms.", OS::get_ticks_msec() - start_time));
}

void initialize_astcenc_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

#ifdef TOOLS_ENABLED
	Image::_image_compress_astc_func = _compress_astc;
#endif

	Image::_image_decompress_astc = _decompress_astc;
}

void uninitialize_astcenc_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}
#endif // defined(MODULE_ASTCENC_ENABLED)


// --- Merged from modules/basis_universal/image_compress_basisu.cpp ---
#if defined(MODULE_BASIS_UNIVERSAL_ENABLED)
#include "core/config/project_settings.h"
#include "modules/register_module_types.h"
#include "core/io/image.h"
#include "core/os/os.h"
#include "core/string/print_string.h"
// --- Inlined from modules/basis_universal/image_compress_basisu.h ---
#include "core/io/image.h"

enum BasisDecompressFormat
{
	BASIS_DECOMPRESS_RG,
	BASIS_DECOMPRESS_RGB,
	BASIS_DECOMPRESS_RGBA,
	BASIS_DECOMPRESS_RG_AS_RA,
	BASIS_DECOMPRESS_R,
	BASIS_DECOMPRESS_HDR_RGB,
	BASIS_DECOMPRESS_MAX
};

constexpr uint32_t BASIS_DECOMPRESS_FLAG_KTX2 = 1 << 31;

void basis_universal_init();

#ifdef TOOLS_ENABLED
struct BasisRGBAF
{
	uint32_t r;
	uint32_t g;
	uint32_t b;
	uint32_t a;
};
#endif

Ref<Image> basis_universal_unpacker_ptr(const uint8_t* p_data, int p_size);
Ref<Image> basis_universal_unpacker(const Vector<uint8_t>& p_buffer);



// --- End modules/basis_universal/image_compress_basisu.h ---
#include "servers/rendering/renderer.h"

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
	uint64_t start_time = OS::get_ticks_msec();

	Ref<Image> image;
	ERR_FAIL_NULL_V_MSG(p_data, image, "Cannot unpack invalid BasisUniversal data.");

	const uint8_t* src_ptr = p_data;
	int src_size = p_size;

	basist::transcoder_texture_format basisu_format =
		basist::transcoder_texture_format::cTFTotalTextureFormats;
	Image::Format image_format = Image::FORMAT_MAX;

	// Get supported compression formats.
	bool bptc_supported = Renderer::has_os_feature("bptc");
	bool astc_supported = Renderer::has_os_feature("astc");
	bool rgtc_supported = Renderer::has_os_feature("rgtc");
	bool s3tc_supported = Renderer::has_os_feature("s3tc");
	bool etc2_supported = Renderer::has_os_feature("etc2");
	bool astc_hdr_supported = Renderer::has_os_feature("astc_hdr");

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
		Image::get_format_name(image_format), OS::get_ticks_msec() - start_time));

	return image;
}

Ref<Image> basis_universal_unpacker(const Vector<uint8_t>& p_buffer)
{
	return basis_universal_unpacker_ptr(p_buffer.ptr(), p_buffer.size());
}

void initialize_basis_universal_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	basis_universal_init();

	Image::basis_universal_unpacker = basis_universal_unpacker;
	Image::basis_universal_unpacker_ptr = basis_universal_unpacker_ptr;
}

void uninitialize_basis_universal_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

#ifdef TOOLS_ENABLED
	Image::basis_universal_packer = nullptr;
#endif

	Image::basis_universal_unpacker = nullptr;
	Image::basis_universal_unpacker_ptr = nullptr;
}
#endif // defined(MODULE_BASIS_UNIVERSAL_ENABLED)


// --- Merged from modules/bcdec/image_decompress_bcdec.cpp ---
#if defined(MODULE_BCDEC_ENABLED)
// --- Inlined from modules/bcdec/image_decompress_bcdec.h ---
#include "core/io/image.h"

enum BCdecFormat {
	BCdec_BC1,
	BCdec_BC2,
	BCdec_BC3,
	BCdec_BC4,
	BCdec_BC5,
	BCdec_BC6S,
	BCdec_BC6U,
	BCdec_BC7,
};

void image_decompress_bcdec(Image *p_image);

// --- End modules/bcdec/image_decompress_bcdec.h ---
#include "modules/register_module_types.h"

#include "core/os/os.h"
#include "core/string/print_string.h"

#define BCDEC_IMPLEMENTATION
#include <thirdparty/misc/bcdec.h>

inline void bcdec_bc6h_half_s(const void *compressedBlock, void *decompressedBlock, int destinationPitch) {
	bcdec_bc6h_half(compressedBlock, decompressedBlock, destinationPitch, true);
}

inline void bcdec_bc6h_half_u(const void *compressedBlock, void *decompressedBlock, int destinationPitch) {
	bcdec_bc6h_half(compressedBlock, decompressedBlock, destinationPitch, false);
}

template <void (*decompress_func)(const void *, void *, int), int block_size, int pixel_size, int component_size>
static inline void _bcdec_safe_bcdec_decompress_mipmap(int width, int height, const uint8_t *src, uint8_t *dst) {
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
			decompress_func(&src[src_pos], &dst[dst_pos], image_pitch / component_size);
			src_pos += block_size;
			dst_pos += block_pitch;
		}

		// Decompress the block on the right.
		if (width_diff > 0) {
			decompress_func(&src[src_pos], temp_buf, block_pitch / component_size);

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
			decompress_func(&src[src_pos], temp_buf, block_pitch / component_size);

			// Copy the data from the temporary buffer to the output.
			for (int i = 0; i < height_diff; i++) {
				memcpy(&dst[dst_pos + i * image_pitch], &temp_buf[i * block_pitch], block_pitch);
			}

			src_pos += block_size;
			dst_pos += block_pitch;
		}

		// Decompress the block in the lower-right corner.
		if (width_diff > 0) {
			decompress_func(&src[src_pos], temp_buf, block_pitch / component_size);

			// Copy the data from the temporary buffer to the output.
			for (int i = 0; i < height_diff; i++) {
				memcpy(&dst[dst_pos + i * image_pitch], &temp_buf[i * block_pitch], odd_pitch);
			}

			src_pos += block_size;
			dst_pos += odd_pitch;
		}
	}
}

template <void (*decompress_func)(const void *, void *, int), int block_size, int pixel_size, int component_size>
static inline void _bcdec_decompress_mipmap(int width, int height, const uint8_t *src, uint8_t *dst) {
	size_t src_pos = 0;
	size_t dst_pos = 0;

	// The size of a single block in bytes.
	const int block_pitch = 4 * pixel_size;
	// The pitch of the image in bytes.
	const int image_pitch = width * pixel_size;

	for (int y = 0; y < height; y += 4) {
		for (int x = 0; x < width; x += 4) {
			decompress_func(&src[src_pos], &dst[dst_pos], image_pitch / component_size);
			src_pos += block_size;
			dst_pos += block_pitch;
		}

		// Skip to the next row of blocks, the current one has already been filled.
		dst_pos += 3 * image_pitch;
	}
}

static void bcdec_decompress_image(BCdecFormat format, const void *src, void *dst, const uint64_t width, const uint64_t height) {
	const uint8_t *src_blocks = reinterpret_cast<const uint8_t *>(src);
	uint8_t *dec_blocks = reinterpret_cast<uint8_t *>(dst);

	const uint64_t aligned_width = (width + 3) & ~0x03;
	const uint64_t aligned_height = (height + 3) & ~0x03;

	if (width != aligned_width || height != aligned_height) {
		// Decompress the mipmap in a 'safe' way, which involves starting from the top left.
		// For each block row, decompress all of the 'full' blocks, then the misaligned one (on the x axis).
		// Then, decompress the final misaligned block row at the bottom.
		// Finally, decompress the misaligned block at the bottom right.
		switch (format) {
			case BCdec_BC1: {
				_bcdec_safe_bcdec_decompress_mipmap<bcdec_bc1, BCDEC_BC1_BLOCK_SIZE, 4, 1>(width, height, src_blocks, dec_blocks);
			} break;
			case BCdec_BC2: {
				_bcdec_safe_bcdec_decompress_mipmap<bcdec_bc2, BCDEC_BC2_BLOCK_SIZE, 4, 1>(width, height, src_blocks, dec_blocks);
			} break;
			case BCdec_BC3: {
				_bcdec_safe_bcdec_decompress_mipmap<bcdec_bc3, BCDEC_BC3_BLOCK_SIZE, 4, 1>(width, height, src_blocks, dec_blocks);
			} break;
			case BCdec_BC4: {
				_bcdec_safe_bcdec_decompress_mipmap<bcdec_bc4, BCDEC_BC4_BLOCK_SIZE, 1, 1>(width, height, src_blocks, dec_blocks);
			} break;
			case BCdec_BC5: {
				_bcdec_safe_bcdec_decompress_mipmap<bcdec_bc5, BCDEC_BC5_BLOCK_SIZE, 2, 1>(width, height, src_blocks, dec_blocks);
			} break;
			case BCdec_BC6U: {
				_bcdec_safe_bcdec_decompress_mipmap<bcdec_bc6h_half_u, BCDEC_BC6H_BLOCK_SIZE, 6, 2>(width, height, src_blocks, dec_blocks);
			} break;
			case BCdec_BC6S: {
				_bcdec_safe_bcdec_decompress_mipmap<bcdec_bc6h_half_s, BCDEC_BC6H_BLOCK_SIZE, 6, 2>(width, height, src_blocks, dec_blocks);
			} break;
			case BCdec_BC7: {
				_bcdec_safe_bcdec_decompress_mipmap<bcdec_bc7, BCDEC_BC7_BLOCK_SIZE, 4, 1>(width, height, src_blocks, dec_blocks);
			} break;
		}
	} else {
		// Just decompress as usual, as fast as possible.
		switch (format) {
			case BCdec_BC1: {
				_bcdec_decompress_mipmap<bcdec_bc1, BCDEC_BC1_BLOCK_SIZE, 4, 1>(width, height, src_blocks, dec_blocks);
			} break;
			case BCdec_BC2: {
				_bcdec_decompress_mipmap<bcdec_bc2, BCDEC_BC2_BLOCK_SIZE, 4, 1>(width, height, src_blocks, dec_blocks);
			} break;
			case BCdec_BC3: {
				_bcdec_decompress_mipmap<bcdec_bc3, BCDEC_BC3_BLOCK_SIZE, 4, 1>(width, height, src_blocks, dec_blocks);
			} break;
			case BCdec_BC4: {
				_bcdec_decompress_mipmap<bcdec_bc4, BCDEC_BC4_BLOCK_SIZE, 1, 1>(width, height, src_blocks, dec_blocks);
			} break;
			case BCdec_BC5: {
				_bcdec_decompress_mipmap<bcdec_bc5, BCDEC_BC5_BLOCK_SIZE, 2, 1>(width, height, src_blocks, dec_blocks);
			} break;
			case BCdec_BC6U: {
				_bcdec_decompress_mipmap<bcdec_bc6h_half_u, BCDEC_BC6H_BLOCK_SIZE, 6, 2>(width, height, src_blocks, dec_blocks);
			} break;
			case BCdec_BC6S: {
				_bcdec_decompress_mipmap<bcdec_bc6h_half_s, BCDEC_BC6H_BLOCK_SIZE, 6, 2>(width, height, src_blocks, dec_blocks);
			} break;
			case BCdec_BC7: {
				_bcdec_decompress_mipmap<bcdec_bc7, BCDEC_BC7_BLOCK_SIZE, 4, 1>(width, height, src_blocks, dec_blocks);
			} break;
		}
	}
}

void image_decompress_bcdec(Image *p_image) {
	uint64_t start_time = OS::get_ticks_msec();

	int width = p_image->get_width();
	int height = p_image->get_height();

	Image::Format source_format = p_image->get_format();
	Image::Format target_format = Image::FORMAT_MAX;

	BCdecFormat bcdec_format = BCdec_BC1;

	switch (source_format) {
		case Image::FORMAT_DXT1:
			bcdec_format = BCdec_BC1;
			target_format = Image::FORMAT_RGBA8;
			break;

		case Image::FORMAT_DXT3:
			bcdec_format = BCdec_BC2;
			target_format = Image::FORMAT_RGBA8;
			break;

		case Image::FORMAT_DXT5:
		case Image::FORMAT_DXT5_RA_AS_RG:
			bcdec_format = BCdec_BC3;
			target_format = Image::FORMAT_RGBA8;
			break;

		case Image::FORMAT_RGTC_R:
			bcdec_format = BCdec_BC4;
			target_format = Image::FORMAT_R8;
			break;

		case Image::FORMAT_RGTC_RG:
			bcdec_format = BCdec_BC5;
			target_format = Image::FORMAT_RG8;
			break;

		case Image::FORMAT_BPTC_RGBFU:
			bcdec_format = BCdec_BC6U;
			target_format = Image::FORMAT_RGBH;
			break;

		case Image::FORMAT_BPTC_RGBF:
			bcdec_format = BCdec_BC6S;
			target_format = Image::FORMAT_RGBH;
			break;

		case Image::FORMAT_BPTC_RGBA:
			bcdec_format = BCdec_BC7;
			target_format = Image::FORMAT_RGBA8;
			break;

		default:
			ERR_FAIL_MSG("bcdec: Can't decompress unknown format: " + Image::get_format_name(source_format) + ".");
			break;
	}

	int mm_count = p_image->get_mipmap_count();
	int64_t target_size = Image::get_image_data_size(width, height, target_format, p_image->has_mipmaps());

	// Decompressed data.
	Vector<uint8_t> data;
	data.resize(target_size);
	uint8_t *wb = data.ptrw();

	// Source data.
	const uint8_t *rb = p_image->get_data().ptr();

	// Decompress mipmaps.
	for (int i = 0; i <= mm_count; i++) {
		int mipmap_w = 0, mipmap_h = 0;
		int64_t src_ofs = Image::get_image_mipmap_offset(width, height, source_format, i);
		int64_t dst_ofs = Image::get_image_mipmap_offset_and_dimensions(width, height, target_format, i, mipmap_w, mipmap_h);
		bcdec_decompress_image(bcdec_format, rb + src_ofs, wb + dst_ofs, mipmap_w, mipmap_h);
	}

	p_image->set_data(width, height, p_image->has_mipmaps(), target_format, data);

	// Swap channels if the format is using a channel swizzle.
	if (source_format == Image::FORMAT_DXT5_RA_AS_RG) {
		p_image->convert_ra_rgba8_to_rg();
	}

	print_verbose(vformat("bcdec: Decompression of a %dx%d %s image with %d mipmaps took %d ms.",
			p_image->get_width(), p_image->get_height(), Image::get_format_name(source_format), p_image->get_mipmap_count(), OS::get_ticks_msec() - start_time));
}

void initialize_bcdec_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	Image::_image_decompress_bc = image_decompress_bcdec;
	Image::_image_decompress_bptc = image_decompress_bcdec;
}

void uninitialize_bcdec_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}
#endif // defined(MODULE_BCDEC_ENABLED)


// --- Merged from modules/etcpak/image_compress_etcpak.cpp ---
#if defined(MODULE_ETCPAK_ENABLED)
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
#include "modules/register_module_types.h"

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
#endif // defined(MODULE_ETCPAK_ENABLED)
