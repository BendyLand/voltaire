/**************************************************************************/
/*  image_loader_tinyexr.cpp                                              */
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
// --- Inlined from modules/tinyexr/image_loader_tinyexr.h ---
#include "core/io/image_loader.h"

class ImageLoaderTinyEXR final : public ImageFormatLoader {
public:
	virtual Error load_image(Ref<Image> p_image, Ref<FileAccess> f, uint32_t p_flags, float p_scale);
	virtual void get_recognized_extensions(List<String> *p_extensions) const;
	ImageLoaderTinyEXR();
};

Error save_exr(const String &p_path, const Ref<Image> &p_img, bool p_grayscale, bool p_color_image, float p_max_value);
Vector<uint8_t> save_exr_buffer(const Ref<Image> &p_img, bool p_grayscale, bool p_color_image, float p_max_value);

// --- End modules/tinyexr/image_loader_tinyexr.h ---
#include "core/io/file_access.h"
#include "core/math/math_funcs.h"
#include "register_types.h"
#include <cstdlib>

#include "core/io/file_access_wrappers.h"

#include <zlib.h>
// zlib should come before including tinyexr.
#include <thirdparty/tinyexr/tinyexr.h>

Error ImageLoaderTinyEXR::load_image(Ref<Image> p_image, Ref<FileAccess> f, uint32_t p_flags, float p_scale) {
	Vector<uint8_t> src_image;
	uint64_t src_image_len = f->get_length();
	ERR_FAIL_COND_V(src_image_len == 0, ERR_FILE_CORRUPT);
	src_image.resize(src_image_len);

	uint8_t *w = src_image.ptrw();

	f->get_buffer(&w[0], src_image_len);

	// Re-implementation of tinyexr's LoadEXRFromMemory using Godot types to store the Image data
	// and Godot's error codes.
	// When debugging after updating the thirdparty library, check that we're still in sync with
	// their API usage in LoadEXRFromMemory.

	EXRVersion exr_version;
	EXRImage exr_image;
	EXRHeader exr_header;
	const char *err = nullptr;

	InitEXRHeader(&exr_header);

	int ret = ParseEXRVersionFromMemory(&exr_version, w, src_image_len);
	if (ret != TINYEXR_SUCCESS) {
		return ERR_FILE_CORRUPT;
	}

	ret = ParseEXRHeaderFromMemory(&exr_header, &exr_version, w, src_image_len, &err);
	if (ret != TINYEXR_SUCCESS) {
		if (err) {
			ERR_PRINT(String(err));
			FreeEXRErrorMessage(err);
		}
		return ERR_FILE_CORRUPT;
	}

	// Read HALF channel as FLOAT. (GH-13490)
	bool use_float16 = false;
	for (int i = 0; i < exr_header.num_channels; i++) {
		if (exr_header.pixel_types[i] == TINYEXR_PIXELTYPE_HALF) {
			use_float16 = true;
			exr_header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;
		}
	}

	InitEXRImage(&exr_image);
	ret = LoadEXRImageFromMemory(&exr_image, &exr_header, w, src_image_len, &err);
	if (ret != TINYEXR_SUCCESS) {
		if (err) {
			ERR_PRINT(String(err));
			FreeEXRErrorMessage(err);
		}
		return ERR_FILE_CORRUPT;
	}

	// RGBA
	int idxR = -1;
	int idxG = -1;
	int idxB = -1;
	int idxA = -1;
	for (int c = 0; c < exr_header.num_channels; c++) {
		if (strcmp(exr_header.channels[c].name, "R") == 0) {
			idxR = c;
		} else if (strcmp(exr_header.channels[c].name, "G") == 0) {
			idxG = c;
		} else if (strcmp(exr_header.channels[c].name, "B") == 0) {
			idxB = c;
		} else if (strcmp(exr_header.channels[c].name, "A") == 0) {
			idxA = c;
		} else if (strcmp(exr_header.channels[c].name, "Y") == 0) {
			idxR = c;
			idxG = c;
			idxB = c;
		}
	}

	// EXR image data loaded, now parse it into Godot-friendly image data

	Vector<uint8_t> imgdata;
	Image::Format format;
	int output_channels = 0;

	int channel_size = use_float16 ? 2 : 4;
	if (idxA != -1) {
		imgdata.resize(exr_image.width * exr_image.height * 4 * channel_size); //RGBA
		format = use_float16 ? Image::FORMAT_RGBAH : Image::FORMAT_RGBAF;
		output_channels = 4;
	} else if (idxB != -1) {
		ERR_FAIL_COND_V(idxG == -1, ERR_FILE_CORRUPT);
		ERR_FAIL_COND_V(idxR == -1, ERR_FILE_CORRUPT);
		imgdata.resize(exr_image.width * exr_image.height * 3 * channel_size); //RGB
		format = use_float16 ? Image::FORMAT_RGBH : Image::FORMAT_RGBF;
		output_channels = 3;
	} else if (idxG != -1) {
		ERR_FAIL_COND_V(idxR == -1, ERR_FILE_CORRUPT);
		imgdata.resize(exr_image.width * exr_image.height * 2 * channel_size); //RG
		format = use_float16 ? Image::FORMAT_RGH : Image::FORMAT_RGF;
		output_channels = 2;
	} else {
		ERR_FAIL_COND_V(idxR == -1, ERR_FILE_CORRUPT);
		imgdata.resize(exr_image.width * exr_image.height * 1 * channel_size); //R
		format = use_float16 ? Image::FORMAT_RH : Image::FORMAT_RF;
		output_channels = 1;
	}

	EXRTile single_image_tile;
	int num_tiles;
	int tile_width = 0;
	int tile_height = 0;

	const EXRTile *exr_tiles;

	if (!exr_header.tiled) {
		single_image_tile.images = exr_image.images;
		single_image_tile.width = exr_image.width;
		single_image_tile.height = exr_image.height;
		single_image_tile.level_x = exr_image.width;
		single_image_tile.level_y = exr_image.height;
		single_image_tile.offset_x = 0;
		single_image_tile.offset_y = 0;

		exr_tiles = &single_image_tile;
		num_tiles = 1;
		tile_width = exr_image.width;
		tile_height = exr_image.height;
	} else {
		tile_width = exr_header.tile_size_x;
		tile_height = exr_header.tile_size_y;
		num_tiles = exr_image.num_tiles;
		exr_tiles = exr_image.tiles;
	}

	//print_line("reading format: " + Image::get_format_name(format));
	{
		uint8_t *wd = imgdata.ptrw();
		uint16_t *iw16 = (uint16_t *)wd;
		float *iw32 = (float *)wd;

		// Assume `out_rgba` have enough memory allocated.
		for (int tile_index = 0; tile_index < num_tiles; tile_index++) {
			const EXRTile &tile = exr_tiles[tile_index];

			int tw = tile.width;
			int th = tile.height;

			const float *r_channel_start = reinterpret_cast<const float *>(tile.images[idxR]);
			const float *g_channel_start = nullptr;
			const float *b_channel_start = nullptr;
			const float *a_channel_start = nullptr;

			if (idxG != -1) {
				g_channel_start = reinterpret_cast<const float *>(tile.images[idxG]);
			}
			if (idxB != -1) {
				b_channel_start = reinterpret_cast<const float *>(tile.images[idxB]);
			}
			if (idxA != -1) {
				a_channel_start = reinterpret_cast<const float *>(tile.images[idxA]);
			}

			uint16_t *first_row_w16 = iw16 + (tile.offset_y * tile_height * exr_image.width + tile.offset_x * tile_width) * output_channels;
			float *first_row_w32 = iw32 + (tile.offset_y * tile_height * exr_image.width + tile.offset_x * tile_width) * output_channels;

			for (int y = 0; y < th; y++) {
				const float *r_channel = r_channel_start + y * tile_width;
				const float *g_channel = nullptr;
				const float *b_channel = nullptr;
				const float *a_channel = nullptr;
				if (g_channel_start) {
					g_channel = g_channel_start + y * tile_width;
				}
				if (b_channel_start) {
					b_channel = b_channel_start + y * tile_width;
				}
				if (a_channel_start) {
					a_channel = a_channel_start + y * tile_width;
				}

				if (use_float16) {
					uint16_t *row_w = first_row_w16 + (y * exr_image.width * output_channels);

					for (int x = 0; x < tw; x++) {
						Color color;
						color.r = *r_channel++;
						if (g_channel) {
							color.g = *g_channel++;
						}
						if (b_channel) {
							color.b = *b_channel++;
						}
						if (a_channel) {
							color.a = *a_channel++;
						}

						if (p_flags & FLAG_FORCE_LINEAR) {
							color = color.srgb_to_linear();
						}

						*row_w++ = Math::make_half_float(color.r);
						if (g_channel) {
							*row_w++ = Math::make_half_float(color.g);
						}
						if (b_channel) {
							*row_w++ = Math::make_half_float(color.b);
						}
						if (a_channel) {
							*row_w++ = Math::make_half_float(color.a);
						}
					}
				} else {
					float *row_w = first_row_w32 + (y * exr_image.width * output_channels);

					for (int x = 0; x < tw; x++) {
						Color color;
						color.r = *r_channel++;
						if (g_channel) {
							color.g = *g_channel++;
						}
						if (b_channel) {
							color.b = *b_channel++;
						}
						if (a_channel) {
							color.a = *a_channel++;
						}

						if (p_flags & FLAG_FORCE_LINEAR) {
							color = color.srgb_to_linear();
						}

						*row_w++ = color.r;
						if (g_channel) {
							*row_w++ = color.g;
						}
						if (b_channel) {
							*row_w++ = color.b;
						}
						if (a_channel) {
							*row_w++ = color.a;
						}
					}
				}
			}
		}
	}

	p_image->set_data(exr_image.width, exr_image.height, false, format, imgdata);

	FreeEXRHeader(&exr_header);
	FreeEXRImage(&exr_image);

	return OK;
}

void ImageLoaderTinyEXR::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("exr");
}

static Ref<Image> _tinyexr_mem_loader_func(const uint8_t *p_exr, int p_size) {
	Ref<FileAccessMemory> memfile;
	memfile.instantiate();
	Error open_memfile_error = memfile->open_custom(p_exr, p_size);
	ERR_FAIL_COND_V_MSG(open_memfile_error, Ref<Image>(), "Could not create memfile for EXR image buffer.");

	Ref<Image> img;
	img.instantiate();
	Error load_error = ImageLoaderTinyEXR().load_image(img, memfile, false, 1.0f);
	ERR_FAIL_COND_V_MSG(load_error, Ref<Image>(), "Failed to load EXR image.");
	return img;
}

ImageLoaderTinyEXR::ImageLoaderTinyEXR() {
	Image::_exr_mem_loader_func = _tinyexr_mem_loader_func;
}

// zlib should come before including tinyexr.


static bool is_supported_format(Image::Format p_format) {
	// This is checked before anything else.
	// Mostly uncompressed formats are considered.
	switch (p_format) {
		case Image::FORMAT_RF:
		case Image::FORMAT_RGF:
		case Image::FORMAT_RGBF:
		case Image::FORMAT_RGBAF:
		case Image::FORMAT_RH:
		case Image::FORMAT_RGH:
		case Image::FORMAT_RGBH:
		case Image::FORMAT_RGBAH:
		case Image::FORMAT_R8:
		case Image::FORMAT_RG8:
		case Image::FORMAT_RGB8:
		case Image::FORMAT_RGBA8:
			return true;
		default:
			return false;
	}
}

enum SrcPixelType {
	SRC_FLOAT,
	SRC_HALF,
	SRC_BYTE,
	SRC_UNSUPPORTED
};

static SrcPixelType get_source_pixel_type(Image::Format p_format) {
	switch (p_format) {
		case Image::FORMAT_RF:
		case Image::FORMAT_RGF:
		case Image::FORMAT_RGBF:
		case Image::FORMAT_RGBAF:
			return SRC_FLOAT;
		case Image::FORMAT_RH:
		case Image::FORMAT_RGH:
		case Image::FORMAT_RGBH:
		case Image::FORMAT_RGBAH:
			return SRC_HALF;
		case Image::FORMAT_R8:
		case Image::FORMAT_RG8:
		case Image::FORMAT_RGB8:
		case Image::FORMAT_RGBA8:
			return SRC_BYTE;
		default:
			return SRC_UNSUPPORTED;
	}
}

static int get_target_pixel_type(Image::Format p_format) {
	switch (p_format) {
		case Image::FORMAT_RF:
		case Image::FORMAT_RGF:
		case Image::FORMAT_RGBF:
		case Image::FORMAT_RGBAF:
			return TINYEXR_PIXELTYPE_FLOAT;
		case Image::FORMAT_RH:
		case Image::FORMAT_RGH:
		case Image::FORMAT_RGBH:
		case Image::FORMAT_RGBAH:
		// EXR doesn't support 8-bit channels so in that case we'll convert
		case Image::FORMAT_R8:
		case Image::FORMAT_RG8:
		case Image::FORMAT_RGB8:
		case Image::FORMAT_RGBA8:
			return TINYEXR_PIXELTYPE_HALF;
		default:
			return -1;
	}
}

static int get_pixel_type_size(int p_pixel_type) {
	switch (p_pixel_type) {
		case TINYEXR_PIXELTYPE_HALF:
			return 2;
		case TINYEXR_PIXELTYPE_FLOAT:
			return 4;
	}
	return -1;
}

static int get_channel_count(Image::Format p_format) {
	switch (p_format) {
		case Image::FORMAT_RF:
		case Image::FORMAT_RH:
		case Image::FORMAT_R8:
			return 1;
		case Image::FORMAT_RGF:
		case Image::FORMAT_RGH:
		case Image::FORMAT_RG8:
			return 2;
		case Image::FORMAT_RGBF:
		case Image::FORMAT_RGBH:
		case Image::FORMAT_RGB8:
			return 3;
		case Image::FORMAT_RGBAF:
		case Image::FORMAT_RGBAH:
		case Image::FORMAT_RGBA8:
			return 4;
		default:
			return -1;
	}
}

Vector<uint8_t> save_exr_buffer(const Ref<Image> &p_img, bool p_grayscale, bool p_color_image, float p_max_value) {
	Image::Format format = p_img->get_format();

	if (!is_supported_format(format)) {
		// Format not supported
		print_error("Image format not supported for saving as EXR. Consider saving as PNG.");

		return Vector<uint8_t>();
	}

	EXRHeader header;
	InitEXRHeader(&header);

	EXRImage image;
	InitEXRImage(&image);

	const int max_channels = 4;

	// Godot does not support more than 4 channels,
	// so we can preallocate header infos on the stack and use only the subset we need
	PackedByteArray channels[max_channels];
	unsigned char *channels_ptrs[max_channels];
	EXRChannelInfo channel_infos[max_channels];
	int pixel_types[max_channels];
	int requested_pixel_types[max_channels] = { -1 };

	// Gimp and Blender are a bit annoying so order of channels isn't straightforward.
	const int channel_mappings[4][4] = {
		{ 0 }, // R
		{ 1, 0 }, // GR
		{ 2, 1, 0 }, // BGR
		{ 3, 2, 1, 0 } // ABGR
	};

	int channel_count = get_channel_count(format);
	ERR_FAIL_COND_V(channel_count < 0, Vector<uint8_t>());
	ERR_FAIL_COND_V(p_grayscale && channel_count != 1, Vector<uint8_t>());

	int target_pixel_type = get_target_pixel_type(format);
	ERR_FAIL_COND_V(target_pixel_type < 0, Vector<uint8_t>());
	int target_pixel_type_size = get_pixel_type_size(target_pixel_type);
	ERR_FAIL_COND_V(target_pixel_type_size < 0, Vector<uint8_t>());
	SrcPixelType src_pixel_type = get_source_pixel_type(format);
	ERR_FAIL_COND_V(src_pixel_type == SRC_UNSUPPORTED, Vector<uint8_t>());
	const int pixel_count = p_img->get_width() * p_img->get_height();

	const int *channel_mapping = channel_mappings[channel_count - 1];

	{
		PackedByteArray src_data = p_img->get_data();
		const uint8_t *src_r = src_data.ptr();

		for (int channel_index = 0; channel_index < channel_count; ++channel_index) {
			// De-interleave channels

			PackedByteArray &dst = channels[channel_index];
			dst.resize(pixel_count * target_pixel_type_size);

			uint8_t *dst_w = dst.ptrw();

			if (src_pixel_type == SRC_FLOAT && target_pixel_type == TINYEXR_PIXELTYPE_FLOAT) {
				// Note: we don't save mipmaps
				CRASH_COND(src_data.size() < pixel_count * channel_count * target_pixel_type_size);

				const float *src_rp = (float *)src_r;
				float *dst_wp = (float *)dst_w;

				for (int i = 0; i < pixel_count; ++i) {
					float src_float = src_rp[channel_index + i * channel_count];
					if (p_max_value >= 0.f) {
						src_float = fmin(p_max_value, src_float);
					}
					if (p_color_image) {
						src_float = fmax(0.f, src_float);
					}
					dst_wp[i] = src_float;
				}

			} else if (src_pixel_type == SRC_HALF && target_pixel_type == TINYEXR_PIXELTYPE_HALF) {
				CRASH_COND(src_data.size() < pixel_count * channel_count * target_pixel_type_size);

				const uint16_t *src_rp = (uint16_t *)src_r;
				uint16_t *dst_wp = (uint16_t *)dst_w;

				for (int i = 0; i < pixel_count; ++i) {
					int src_rp_index = channel_index + i * channel_count;
					if (p_max_value >= 0.f || p_color_image) {
						float src_float = Math::halfptr_to_float(src_rp + src_rp_index);
						if (p_max_value >= 0.f) {
							src_float = fmin(p_max_value, src_float);
						}
						if (p_color_image) {
							src_float = fmax(0.f, src_float);
						}
						dst_wp[i] = Math::make_half_float(src_float);
					} else {
						dst_wp[i] = src_rp[src_rp_index];
					}
				}

			} else if (src_pixel_type == SRC_BYTE && target_pixel_type == TINYEXR_PIXELTYPE_HALF) {
				CRASH_COND(src_data.size() < pixel_count * channel_count);

				const uint8_t *src_rp = (uint8_t *)src_r;
				uint16_t *dst_wp = (uint16_t *)dst_w;

				for (int i = 0; i < pixel_count; ++i) {
					float src_float = src_rp[channel_index + i * channel_count] / 255.f;
					if (p_max_value >= 0.f) {
						src_float = fmin(p_max_value, src_float);
					}
					dst_wp[i] = Math::make_half_float(src_float);
				}

			} else {
				CRASH_NOW();
			}

			int remapped_index = channel_mapping[channel_index];

			channels_ptrs[remapped_index] = dst_w;

			// No conversion
			pixel_types[remapped_index] = target_pixel_type;
			requested_pixel_types[remapped_index] = target_pixel_type;

			// Write channel name
			if (p_grayscale) {
				channel_infos[remapped_index].name[0] = 'Y';
				channel_infos[remapped_index].name[1] = '\0';
			} else {
				const char *rgba = "RGBA";
				channel_infos[remapped_index].name[0] = rgba[channel_index];
				channel_infos[remapped_index].name[1] = '\0';
			}
		}
	}

	image.images = channels_ptrs;
	image.num_channels = channel_count;
	image.width = p_img->get_width();
	image.height = p_img->get_height();

	header.num_channels = image.num_channels;
	header.channels = channel_infos;
	header.pixel_types = pixel_types;
	header.requested_pixel_types = requested_pixel_types;
	header.compression_type = TINYEXR_COMPRESSIONTYPE_PIZ;

	unsigned char *mem = nullptr;
	const char *err = nullptr;

	size_t bytes = SaveEXRImageToMemory(&image, &header, &mem, &err);
	if (err && *err != OK) {
		return Vector<uint8_t>();
	}
	Vector<uint8_t> buffer;
	buffer.resize(bytes);
	memcpy(buffer.ptrw(), mem, bytes);
	free(mem);
	return buffer;
}

Error save_exr(const String &p_path, const Ref<Image> &p_img, bool p_grayscale, bool p_color_image, float p_max_value) {
	const Vector<uint8_t> buffer = save_exr_buffer(p_img, p_grayscale, p_color_image, p_max_value);
	if (buffer.is_empty()) {
		print_error(String("Saving EXR failed."));
		return ERR_FILE_CANT_WRITE;
	} else {
		Ref<FileAccess> ref = FileAccess::open(p_path, FileAccess::WRITE);
		ERR_FAIL_COND_V(ref.is_null(), ERR_FILE_CANT_WRITE);
		ref->store_buffer(buffer.ptr(), buffer.size());
	}

	return OK;
}

static Ref<ImageLoaderTinyEXR> image_loader_tinyexr;

void initialize_tinyexr_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	image_loader_tinyexr.instantiate();
	ImageLoader::add_image_format_loader(image_loader_tinyexr);

	Image::save_exr_func = save_exr;
	Image::save_exr_buffer_func = save_exr_buffer;
}

void uninitialize_tinyexr_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	ImageLoader::remove_image_format_loader(image_loader_tinyexr);
	image_loader_tinyexr.unref();

	Image::save_exr_func = nullptr;
}
