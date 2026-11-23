#include "modules/modules_enabled.gen.h"

#if defined(MODULE_SVG_ENABLED)
#include "core/io/resource_loader.h"
#include "main/main.h"

#include "core/os/os.h"

#include <thorvg.h>

HashMap<Color, Color> ImageLoaderSVG::forced_color_map = HashMap<Color, Color>();

void ImageLoaderSVG::set_forced_color_map(const HashMap<Color, Color> &p_color_map) {
	forced_color_map = p_color_map;
}

void ImageLoaderSVG::_replace_color_property(const HashMap<Color, Color> &p_color_map, const String &p_prefix, String &r_string) {
	// Replace colors in the SVG based on what is passed in `p_color_map`.
	// Used to change the colors of editor icons based on the used theme.
	// The strings being replaced are typically of the form:
	//   fill="#5abbef"
	// But can also be 3-letter codes, include alpha, be "none" or a named color
	// string ("blue"). So we convert to Godot Color to compare with `p_color_map`.

	const int prefix_len = p_prefix.length();
	int pos = r_string.find(p_prefix);
	while (pos != -1) {
		pos += prefix_len; // Skip prefix.
		int end_pos = r_string.find_char('"', pos);
		ERR_FAIL_COND_MSG(end_pos == -1, vformat("Malformed SVG string after property \"%s\".", p_prefix));
		const String color_code = r_string.substr(pos, end_pos - pos);
		if (color_code != "none" && !color_code.begins_with("url(")) {
			const Color color = Color(color_code); // Handles both HTML codes and named colors.
			if (p_color_map.has(color)) {
				r_string = r_string.left(pos) + "#" + p_color_map[color].to_html(false) + r_string.substr(end_pos);
			}
		}
		// Search for other occurrences.
		pos = r_string.find(p_prefix, pos);
	}
}

Ref<Image> ImageLoaderSVG::load_mem_svg(const uint8_t *p_svg, int p_size, float p_scale) {
	Ref<Image> img;
	img.instantiate();

	Error err = create_image_from_utf8_buffer(img, p_svg, p_size, p_scale, false);
	ERR_FAIL_COND_V_MSG(err != OK, Ref<Image>(), vformat("ImageLoaderSVG: Failed to create SVG from buffer, error code %d.", err));

	return img;
}

Error ImageLoaderSVG::create_image_from_utf8_buffer(Ref<Image> p_image, const uint8_t *p_buffer, int p_buffer_size, float p_scale, bool p_upsample) {
	ERR_FAIL_COND_V_MSG(Math::is_zero_approx(p_scale), ERR_INVALID_PARAMETER, "ImageLoaderSVG: Can't load SVG with a scale of 0.");

	tvg::Picture *picture = tvg::Picture::gen();

	tvg::Result result = picture->load((const char *)p_buffer, p_buffer_size, "svg", nullptr, true);
	if (result != tvg::Result::Success) {
		tvg::Paint::rel(picture);
		return ERR_INVALID_DATA;
	}
	float fw, fh;
	picture->size(&fw, &fh);

	uint32_t width = MAX(1, std::round(fw * p_scale));
	uint32_t height = MAX(1, std::round(fh * p_scale));

	const uint32_t max_dimension = 16384;
	if (width > max_dimension || height > max_dimension) {
		WARN_PRINT(vformat(
				String::utf8("ImageLoaderSVG: Target canvas dimensions %d×%d (with scale %.2f) exceed the max supported dimensions %d×%d. The target canvas will be scaled down."),
				width, height, p_scale, max_dimension, max_dimension));
		width = MIN(width, max_dimension);
		height = MIN(height, max_dimension);
	}

	picture->size(width, height);

	uint32_t threads = OS::get_processor_count();
	tvg::Initializer::init(threads);
	std::unique_ptr<tvg::SwCanvas> sw_canvas(tvg::SwCanvas::gen());
	Vector<uint8_t> buffer;
	buffer.resize(sizeof(uint32_t) * width * height);

	tvg::Result res = sw_canvas->target((uint32_t *)buffer.ptrw(), width, width, height, tvg::ColorSpace::ABGR8888S);
	if (res != tvg::Result::Success) {
		tvg::Paint::rel(picture);
		ERR_FAIL_V_MSG(FAILED, vformat("ImageLoaderSVG: Couldn't set target on ThorVG canvas, error code %d.", res));
	}

	res = sw_canvas->add(picture);
	if (res != tvg::Result::Success) {
		ERR_FAIL_V_MSG(FAILED, vformat("ImageLoaderSVG: Couldn't insert ThorVG picture on canvas, error code %d.", res));
	}

	res = sw_canvas->draw(true);
	if (res != tvg::Result::Success) {
		ERR_FAIL_V_MSG(FAILED, vformat("ImageLoaderSVG: Couldn't draw ThorVG pictures on canvas, error code %d.", res));
	}

	res = sw_canvas->sync();
	if (res != tvg::Result::Success) {
		ERR_FAIL_V_MSG(FAILED, vformat("ImageLoaderSVG: Couldn't sync ThorVG canvas, error code %d.", res));
	}

	p_image->set_data(width, height, false, Image::FORMAT_RGBA8, buffer);

	return OK;
}

Error ImageLoaderSVG::create_image_from_utf8_buffer(Ref<Image> p_image, const PackedByteArray &p_buffer, float p_scale, bool p_upsample) {
	return create_image_from_utf8_buffer(p_image, p_buffer.ptr(), p_buffer.size(), p_scale, p_upsample);
}

Error ImageLoaderSVG::create_image_from_string(Ref<Image> p_image, String p_string, float p_scale, bool p_upsample, const HashMap<Color, Color> &p_color_map) {
	if (p_color_map.size()) {
		_replace_color_property(p_color_map, "stop-color=\"", p_string);
		_replace_color_property(p_color_map, "fill=\"", p_string);
		_replace_color_property(p_color_map, "stroke=\"", p_string);
	}

	PackedByteArray bytes = p_string.to_utf8_buffer();

	return create_image_from_utf8_buffer(p_image, bytes, p_scale, p_upsample);
}

void ImageLoaderSVG::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("svg");
}

Error ImageLoaderSVG::load_image(Ref<Image> p_image, Ref<FileAccess> p_fileaccess, uint32_t p_flags, float p_scale) {
	const uint64_t len = p_fileaccess->get_length() - p_fileaccess->get_position();
	Vector<uint8_t> buffer;
	buffer.resize(len);
	p_fileaccess->get_buffer(buffer.ptrw(), buffer.size());

	String svg;
	Error err = svg.append_utf8((const char *)buffer.ptr(), buffer.size());
	if (err != OK) {
		return err;
	}

	if (p_flags & FLAG_CONVERT_COLORS) {
		err = create_image_from_string(p_image, svg, p_scale, false, forced_color_map);
	} else {
		err = create_image_from_string(p_image, svg, p_scale, false, HashMap<Color, Color>());
	}

	if (err != OK) {
		return err;
	} else if (p_image->is_empty()) {
		return ERR_INVALID_DATA;
	}

	if (p_flags & FLAG_FORCE_LINEAR) {
		p_image->srgb_to_linear();
	}
	return OK;
}

ImageLoaderSVG::ImageLoaderSVG() {
	Image::_svg_scalable_mem_loader_func = load_mem_svg;
}

#ifdef THREADS_ENABLED
#define TVG_THREADS 1
#else
#define TVG_THREADS 0
#endif

static Ref<ImageLoaderSVG> image_loader_svg;

void initialize_svg_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	if (tvg::Initializer::init(TVG_THREADS) != tvg::Result::Success) {
		return;
	}

	image_loader_svg.instantiate();
	ImageLoader::add_image_format_loader(image_loader_svg);
}

void uninitialize_svg_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	if (image_loader_svg.is_null()) {
		// It failed to initialize so it was not added.
		return;
	}

	ImageLoader::remove_image_format_loader(image_loader_svg);
	image_loader_svg.unref();
	tvg::Initializer::term();
}
#endif // defined(MODULE_SVG_ENABLED)


// --- Merged from modules/bmp/image_loader_bmp.cpp ---
#if defined(MODULE_BMP_ENABLED)
// --- Inlined from modules/bmp/image_loader_bmp.h ---
#include "core/io/resource_loader.h"

class ImageLoaderBMP : public ImageFormatLoader {
protected:
	static const unsigned BITMAP_SIGNATURE = 0x4d42;

	static const unsigned BITMAP_FILE_HEADER_SIZE = 14; // bmp_file_header_s
	static const unsigned BITMAP_INFO_HEADER_MIN_SIZE = 40; // bmp_info_header_s

	enum bmp_compression_s {
		BI_RGB = 0x00,
		BI_RLE8 = 0x01, // compressed
		BI_RLE4 = 0x02, // compressed
		BI_BITFIELDS = 0x03,
		BI_JPEG = 0x04,
		BI_PNG = 0x05,
		BI_ALPHABITFIELDS = 0x06,
		BI_CMYK = 0x0b,
		BI_CMYKRLE8 = 0x0c, // compressed
		BI_CMYKRLE4 = 0x0d // compressed
	};

	struct bmp_header_s {
		struct bmp_file_header_s {
			uint16_t bmp_signature = 0;
			uint32_t bmp_file_size = 0;
			uint32_t bmp_file_padding = 0;
			uint32_t bmp_file_offset = 0;
		} bmp_file_header;

		struct bmp_info_header_s {
			uint32_t bmp_header_size = 0;
			uint32_t bmp_width = 0;
			uint32_t bmp_height = 0;
			uint16_t bmp_planes = 0;
			uint16_t bmp_bit_count = 0;
			uint32_t bmp_compression = 0;
			uint32_t bmp_size_image = 0;
			uint32_t bmp_pixels_per_meter_x = 0;
			uint32_t bmp_pixels_per_meter_y = 0;
			uint32_t bmp_colors_used = 0;
			uint32_t bmp_important_colors = 0;
		} bmp_info_header;

		struct bmp_bitfield_s {
			uint16_t alpha_mask = 0x8000;
			uint16_t red_mask = 0x7C00;
			uint16_t green_mask = 0x03E0;
			uint16_t blue_mask = 0x001F;
			uint16_t alpha_mask_width = 1u;
			uint16_t red_mask_width = 5u;
			uint16_t green_mask_width = 5u;
			uint16_t blue_mask_width = 5u;
			uint8_t alpha_offset = 15u; // Used for bit shifting.
			uint8_t red_offset = 10u; // Used for bit shifting.
			uint8_t green_offset = 5u; // Used for bit shifting.
			//uint8_t blue_offset = 0u; // Always LSB aligned no shifting needed.
			//uint8_t alpha_max = 1u; // Always boolean or on, so no scaling needed.
			uint8_t red_max = 32u; // Used for color space scaling.
			uint8_t green_max = 32u; // Used for color space scaling.
			uint8_t blue_max = 32u; // Used for color space scaling.
		} bmp_bitfield;
	};

	static Error bmp_convert_to_image(Ref<Image> p_image,
			const uint8_t *p_buffer,
			const uint8_t *p_color_buffer,
			const uint32_t color_table_size,
			const bmp_header_s &p_header);

public:
	virtual Error load_image(Ref<Image> p_image, Ref<FileAccess> f, uint32_t p_flags, float p_scale);
	virtual void get_recognized_extensions(List<String> *p_extensions) const;
	ImageLoaderBMP();
};

// --- End modules/bmp/image_loader_bmp.h ---
#include "main/main.h"

#include "core/io/file_access_wrappers.h"

static uint8_t get_mask_width(uint16_t mask) {
	// Returns number of ones in the binary value of the parameter: mask.
	// Uses a Simple pop_count.
	uint8_t c = 0u;
	for (; mask != 0u; mask &= mask - 1u) {
		c++;
	}
	return c;
}

Error ImageLoaderBMP::bmp_convert_to_image(Ref<Image> p_image,
		const uint8_t *p_buffer,
		const uint8_t *p_color_buffer,
		const uint32_t color_table_size,
		const bmp_header_s &p_header) {
	Error err = OK;

	if (p_buffer == nullptr) {
		err = FAILED;
	}

	if (err == OK) {
		size_t index = 0;
		size_t width = (size_t)p_header.bmp_info_header.bmp_width;
		size_t height = (size_t)p_header.bmp_info_header.bmp_height;
		size_t bits_per_pixel = (size_t)p_header.bmp_info_header.bmp_bit_count;

		// Image data (might be indexed)
		Vector<uint8_t> data;
		int data_len = 0;

		if (bits_per_pixel <= 8) { // indexed
			data_len = width * height;
		} else { // color
			data_len = width * height * 4;
		}
		ERR_FAIL_COND_V_MSG(data_len == 0, ERR_BUG, "Couldn't parse the BMP image data.");
		err = data.resize(data_len);

		uint8_t *data_w = data.ptrw();
		uint8_t *write_buffer = data_w;

		const uint32_t width_bytes = (width * bits_per_pixel + 7) / 8;
		const uint32_t line_width = (width_bytes + 3) & ~3; // Padded to 4 bytes.

		const uint8_t *line = p_buffer + (line_width * (height - 1));
		const uint8_t *end_buffer = p_buffer + p_header.bmp_file_header.bmp_file_size - p_header.bmp_file_header.bmp_file_offset;
		ERR_FAIL_COND_V(line + line_width > end_buffer, ERR_FILE_CORRUPT);

		for (uint64_t i = 0; i < height; i++) {
			const uint8_t *line_ptr = line;

			for (unsigned int j = 0; j < width; j++) {
				switch (bits_per_pixel) {
					case 1: {
						write_buffer[index] = (line[(j * bits_per_pixel) / 8] >> (8 - bits_per_pixel * (1 + j % 8))) & 0x01;

						index++;
					} break;
					case 2: {
						write_buffer[index] = (line[(j * bits_per_pixel) / 8] >> (8 - bits_per_pixel * (1 + j % 4))) & 0x03;

						index++;
					} break;
					case 4: {
						write_buffer[index] = (line[(j * bits_per_pixel) / 8] >> (8 - bits_per_pixel * (1 + j % 2))) & 0x0f;

						index++;
					} break;
					case 8: {
						uint8_t color_index = *line_ptr;

						write_buffer[index] = color_index;

						index += 1;
						line_ptr += 1;
					} break;
					case 16: {
						uint16_t rgb = (static_cast<uint16_t>(line_ptr[1]) << 8) | line_ptr[0];
						// A1R5G5B5/X1R5G5B5 => uint16_t
						// [A/X]1R5G2 | G3B5 => uint8_t | uint8_t
						uint8_t ba = (rgb & p_header.bmp_bitfield.alpha_mask) >> p_header.bmp_bitfield.alpha_offset; // Alpha 0b 1000 ...
						uint8_t b0 = (rgb & p_header.bmp_bitfield.red_mask) >> p_header.bmp_bitfield.red_offset; // Red   0b 0111 1100 ...
						uint8_t b1 = (rgb & p_header.bmp_bitfield.green_mask) >> p_header.bmp_bitfield.green_offset; // Green 0b 0000 0011 1110 ...
						uint8_t b2 = (rgb & p_header.bmp_bitfield.blue_mask); // >> p_header.bmp_bitfield.blue_offset; // Blue  0b  ... 0001 1111

						// Next we apply some color scaling going from a variable value space to a 256 value space.
						// This may be simplified some but left as is for legibility.
						// float scaled_value = unscaled_value * byte_max_value / color_channel_maximum_value + rounding_offset;
						float f0 = b0 * 255.0f / static_cast<float>(p_header.bmp_bitfield.red_max) + 0.5f;
						float f1 = b1 * 255.0f / static_cast<float>(p_header.bmp_bitfield.green_max) + 0.5f;
						float f2 = b2 * 255.0f / static_cast<float>(p_header.bmp_bitfield.blue_max) + 0.5f;
						write_buffer[index + 0] = static_cast<uint8_t>(f0); // R
						write_buffer[index + 1] = static_cast<uint8_t>(f1); // G
						write_buffer[index + 2] = static_cast<uint8_t>(f2); // B

						if (p_header.bmp_bitfield.alpha_mask_width > 0) {
							write_buffer[index + 3] = ba * 0xFF; // Alpha value(Always true or false so no scaling)
						} else {
							write_buffer[index + 3] = 0xFF; // No Alpha channel, Show everything.
						}

						index += 4;
						line_ptr += 2;
					} break;
					case 24: {
						write_buffer[index + 2] = line_ptr[0];
						write_buffer[index + 1] = line_ptr[1];
						write_buffer[index + 0] = line_ptr[2];
						write_buffer[index + 3] = 0xff;

						index += 4;
						line_ptr += 3;
					} break;
					case 32: {
						write_buffer[index + 2] = line_ptr[0];
						write_buffer[index + 1] = line_ptr[1];
						write_buffer[index + 0] = line_ptr[2];
						write_buffer[index + 3] = line_ptr[3];

						index += 4;
						line_ptr += 4;
					} break;
				}
			}
			line -= line_width;
		}

		if (p_color_buffer == nullptr || color_table_size == 0) { // regular pixels

			p_image->set_data(width, height, false, Image::FORMAT_RGBA8, data);

		} else { // data is in indexed format, extend it

			// Palette data
			Vector<uint8_t> palette_data;
			palette_data.resize(color_table_size * 4);

			uint8_t *palette_data_w = palette_data.ptrw();
			uint8_t *pal = palette_data_w;

			const uint8_t *cb = p_color_buffer;

			for (unsigned int i = 0; i < color_table_size; ++i) {
				pal[i * 4 + 0] = cb[2];
				pal[i * 4 + 1] = cb[1];
				pal[i * 4 + 2] = cb[0];
				pal[i * 4 + 3] = 0xff;

				cb += 4;
			}
			// Extend palette to image
			Vector<uint8_t> extended_data;
			extended_data.resize(data.size() * 4);

			uint8_t *ex_w = extended_data.ptrw();
			uint8_t *dest = ex_w;

			const int num_pixels = width * height;

			for (int i = 0; i < num_pixels; i++) {
				dest[0] = pal[write_buffer[i] * 4 + 0];
				dest[1] = pal[write_buffer[i] * 4 + 1];
				dest[2] = pal[write_buffer[i] * 4 + 2];
				dest[3] = pal[write_buffer[i] * 4 + 3];

				dest += 4;
			}
			p_image->set_data(width, height, false, Image::FORMAT_RGBA8, extended_data);
		}
	}
	return err;
}

Error ImageLoaderBMP::load_image(Ref<Image> p_image, Ref<FileAccess> f, uint32_t p_flags, float p_scale) {
	bmp_header_s bmp_header;
	Error err = ERR_INVALID_DATA;

	// A valid bmp file should always at least have a
	// file header and a minimal info header
	if (f->get_length() > BITMAP_FILE_HEADER_SIZE + BITMAP_INFO_HEADER_MIN_SIZE) {
		// File Header
		bmp_header.bmp_file_header.bmp_signature = f->get_16();
		if (bmp_header.bmp_file_header.bmp_signature == BITMAP_SIGNATURE) {
			bmp_header.bmp_file_header.bmp_file_size = f->get_32();
			bmp_header.bmp_file_header.bmp_file_padding = f->get_32();
			bmp_header.bmp_file_header.bmp_file_offset = f->get_32();

			// Info Header
			bmp_header.bmp_info_header.bmp_header_size = f->get_32();
			ERR_FAIL_COND_V_MSG(bmp_header.bmp_info_header.bmp_header_size < BITMAP_INFO_HEADER_MIN_SIZE, ERR_FILE_CORRUPT,
					vformat("Couldn't parse the BMP info header. The file is likely corrupt: %s", f->get_path()));

			bmp_header.bmp_info_header.bmp_width = f->get_32();
			bmp_header.bmp_info_header.bmp_height = f->get_32();

			bmp_header.bmp_info_header.bmp_planes = f->get_16();
			ERR_FAIL_COND_V_MSG(bmp_header.bmp_info_header.bmp_planes != 1, ERR_FILE_CORRUPT,
					vformat("Couldn't parse the BMP planes. The file is likely corrupt: %s", f->get_path()));

			bmp_header.bmp_info_header.bmp_bit_count = f->get_16();
			bmp_header.bmp_info_header.bmp_compression = f->get_32();
			bmp_header.bmp_info_header.bmp_size_image = f->get_32();
			bmp_header.bmp_info_header.bmp_pixels_per_meter_x = f->get_32();
			bmp_header.bmp_info_header.bmp_pixels_per_meter_y = f->get_32();
			bmp_header.bmp_info_header.bmp_colors_used = f->get_32();
			bmp_header.bmp_info_header.bmp_important_colors = f->get_32();

			switch (bmp_header.bmp_info_header.bmp_compression) {
				case BI_BITFIELDS: {
					bmp_header.bmp_bitfield.red_mask = f->get_32();
					bmp_header.bmp_bitfield.green_mask = f->get_32();
					bmp_header.bmp_bitfield.blue_mask = f->get_32();
					bmp_header.bmp_bitfield.alpha_mask = f->get_32();

					bmp_header.bmp_bitfield.red_mask_width = get_mask_width(bmp_header.bmp_bitfield.red_mask);
					bmp_header.bmp_bitfield.green_mask_width = get_mask_width(bmp_header.bmp_bitfield.green_mask);
					bmp_header.bmp_bitfield.blue_mask_width = get_mask_width(bmp_header.bmp_bitfield.blue_mask);
					bmp_header.bmp_bitfield.alpha_mask_width = get_mask_width(bmp_header.bmp_bitfield.alpha_mask);

					bmp_header.bmp_bitfield.alpha_offset = bmp_header.bmp_bitfield.red_mask_width + bmp_header.bmp_bitfield.green_mask_width + bmp_header.bmp_bitfield.blue_mask_width;
					bmp_header.bmp_bitfield.red_offset = bmp_header.bmp_bitfield.green_mask_width + bmp_header.bmp_bitfield.blue_mask_width;
					bmp_header.bmp_bitfield.green_offset = bmp_header.bmp_bitfield.blue_mask_width;

					bmp_header.bmp_bitfield.red_max = (1 << bmp_header.bmp_bitfield.red_mask_width) - 1;
					bmp_header.bmp_bitfield.green_max = (1 << bmp_header.bmp_bitfield.green_mask_width) - 1;
					bmp_header.bmp_bitfield.blue_max = (1 << bmp_header.bmp_bitfield.blue_mask_width) - 1;
				} break;
				case BI_RLE8:
				case BI_RLE4:
				case BI_CMYKRLE8:
				case BI_CMYKRLE4: {
					// Stop parsing.
					ERR_FAIL_V_MSG(ERR_UNAVAILABLE,
							vformat("RLE compressed BMP files are not yet supported: %s", f->get_path()));
				} break;
			}
			// Don't rely on sizeof(bmp_file_header) as structure padding
			// adds 2 bytes offset leading to misaligned color table reading
			uint32_t ct_offset = BITMAP_FILE_HEADER_SIZE + bmp_header.bmp_info_header.bmp_header_size;
			f->seek(ct_offset);

			uint32_t color_table_size = 0;

			// bmp_colors_used may report 0 despite having a color table
			// for 4 and 1 bit images, so don't rely on this information
			if (bmp_header.bmp_info_header.bmp_bit_count <= 8) {
				// Support 256 colors max
				color_table_size = 1 << bmp_header.bmp_info_header.bmp_bit_count;
				ERR_FAIL_COND_V_MSG(color_table_size == 0, ERR_BUG,
						vformat("Couldn't parse the BMP color table: %s", f->get_path()));
			}

			Vector<uint8_t> bmp_color_table;
			// Color table is usually 4 bytes per color -> [B][G][R][0]
			bmp_color_table.resize(color_table_size * 4);
			uint8_t *bmp_color_table_w = bmp_color_table.ptrw();
			f->get_buffer(bmp_color_table_w, color_table_size * 4);

			f->seek(bmp_header.bmp_file_header.bmp_file_offset);

			uint32_t bmp_buffer_size = (bmp_header.bmp_file_header.bmp_file_size - bmp_header.bmp_file_header.bmp_file_offset);

			Vector<uint8_t> bmp_buffer;
			err = bmp_buffer.resize(bmp_buffer_size);
			if (err == OK) {
				uint8_t *bmp_buffer_w = bmp_buffer.ptrw();
				f->get_buffer(bmp_buffer_w, bmp_buffer_size);

				const uint8_t *bmp_buffer_r = bmp_buffer.ptr();
				const uint8_t *bmp_color_table_r = bmp_color_table.ptr();
				err = bmp_convert_to_image(p_image, bmp_buffer_r,
						bmp_color_table_r, color_table_size, bmp_header);
			}
		}
	}
	return err;
}

void ImageLoaderBMP::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("bmp");
}

static Ref<Image> _bmp_mem_loader_func(const uint8_t *p_bmp, int p_size) {
	Ref<FileAccessMemory> memfile;
	memfile.instantiate();
	Error open_memfile_error = memfile->open_custom(p_bmp, p_size);
	ERR_FAIL_COND_V_MSG(open_memfile_error, Ref<Image>(), "Could not create memfile for BMP image buffer.");

	Ref<Image> img;
	img.instantiate();
	Error load_error = ImageLoaderBMP().load_image(img, memfile, false, 1.0f);
	ERR_FAIL_COND_V_MSG(load_error, Ref<Image>(), "Failed to load BMP image.");
	return img;
}

ImageLoaderBMP::ImageLoaderBMP() {
	Image::_bmp_mem_loader_func = _bmp_mem_loader_func;
}

static Ref<ImageLoaderBMP> image_loader_bmp;

void initialize_bmp_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	image_loader_bmp.instantiate();
	ImageLoader::add_image_format_loader(image_loader_bmp);
}

void uninitialize_bmp_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	ImageLoader::remove_image_format_loader(image_loader_bmp);
	image_loader_bmp.unref();
}
#endif // defined(MODULE_BMP_ENABLED)


// --- Merged from modules/dds/texture_loader_dds.cpp ---
#if defined(MODULE_DDS_ENABLED)
#include "core/io/file_access.h"
#include "core/io/file_access_wrappers.h"
#include "core/io/stream_peer.h"
#include "main/main.h"
#include "scene/resources/texture.h"
#include "scene/resources/image_texture.h"

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
#endif // defined(MODULE_DDS_ENABLED)


// --- Merged from modules/hdr/image_loader_hdr.cpp ---
#if defined(MODULE_HDR_ENABLED)
// --- Inlined from modules/hdr/image_loader_hdr.h ---
#include "core/io/resource_loader.h"

class ImageLoaderHDR : public ImageFormatLoader {
public:
	virtual Error load_image(Ref<Image> p_image, Ref<FileAccess> f, uint32_t p_flags, float p_scale);
	virtual void get_recognized_extensions(List<String> *p_extensions) const;

	ImageLoaderHDR();
};

// --- End modules/hdr/image_loader_hdr.h ---
#include "main/main.h"

Error ImageLoaderHDR::load_image(Ref<Image> p_image, Ref<FileAccess> f, uint32_t p_flags, float p_scale) {
	String header = f->get_token();

	ERR_FAIL_COND_V_MSG(header != "#?RADIANCE" && header != "#?RGBE", ERR_FILE_UNRECOGNIZED, "Unsupported header information in HDR: " + header + ".");

	while (true) {
		String line = f->get_line();
		ERR_FAIL_COND_V(f->eof_reached(), ERR_FILE_UNRECOGNIZED);
		if (line.is_empty()) { // empty line indicates end of header
			break;
		}
		if (line.begins_with("FORMAT=")) { // leave option to implement other commands
			ERR_FAIL_COND_V_MSG(line != "FORMAT=32-bit_rle_rgbe", ERR_FILE_UNRECOGNIZED, "Only 32-bit_rle_rgbe is supported for HDR files.");
		} else if (!line.begins_with("#")) { // not comment
			WARN_PRINT("Ignoring unsupported header information in HDR: " + line + ".");
		}
	}

	String token = f->get_token();

	ERR_FAIL_COND_V(token != "-Y", ERR_FILE_CORRUPT);

	int height = f->get_token().to_int();

	token = f->get_token();

	ERR_FAIL_COND_V(token != "+X", ERR_FILE_CORRUPT);

	int width = f->get_line().to_int();

	Vector<uint8_t> imgdata;

	imgdata.resize(height * width * (int)sizeof(uint32_t));

	{
		uint8_t *ptr = imgdata.ptrw();

		Vector<uint8_t> temp_read_data;
		temp_read_data.resize(128);
		uint8_t *temp_read_ptr = temp_read_data.ptrw();

		if (width < 8 || width >= 32768) {
			// Read flat data

			f->get_buffer(ptr, (uint64_t)width * height * 4);
		} else {
			// Read RLE-encoded data

			for (int j = 0; j < height; ++j) {
				int c1 = f->get_8();
				int c2 = f->get_8();
				int len = f->get_8();
				if (c1 != 2 || c2 != 2 || (len & 0x80)) {
					// not run-length encoded, so we have to actually use THIS data as a decoded
					// pixel (note this can't be a valid pixel--one of RGB must be >= 128)

					ptr[(j * width) * 4 + 0] = uint8_t(c1);
					ptr[(j * width) * 4 + 1] = uint8_t(c2);
					ptr[(j * width) * 4 + 2] = uint8_t(len);
					ptr[(j * width) * 4 + 3] = f->get_8();

					f->get_buffer(&ptr[(j * width + 1) * 4], (width - 1) * 4);
					continue;
				}
				len <<= 8;
				len |= f->get_8();

				ERR_FAIL_COND_V_MSG(len != width, ERR_FILE_CORRUPT, "Invalid decoded scanline length, corrupt HDR.");

				for (int k = 0; k < 4; ++k) {
					int i = 0;
					while (i < width) {
						int count = f->get_8();
						if (count > 128) {
							// Run
							int value = f->get_8();
							count -= 128;
							for (int z = 0; z < count; ++z) {
								ptr[(j * width + i++) * 4 + k] = uint8_t(value);
							}
						} else {
							// Dump
							f->get_buffer(temp_read_ptr, count);
							for (int z = 0; z < count; ++z) {
								ptr[(j * width + i++) * 4 + k] = temp_read_ptr[z];
							}
						}
					}
				}
			}
		}

		const bool force_linear = p_flags & FLAG_FORCE_LINEAR;

		//convert
		for (int i = 0; i < width * height; i++) {
			int e = ptr[3] - 128;

			if (force_linear || (e < -15 || e > 15)) {
				float exp = std::pow(2.0f, e);
				Color c(ptr[0] * exp / 255.0, ptr[1] * exp / 255.0, ptr[2] * exp / 255.0);

				if (force_linear) {
					c = c.srgb_to_linear();
				}

				*(uint32_t *)ptr = c.to_rgbe9995();
			} else {
				// https://github.com/george-steel/rgbe-rs/blob/e7cc33b7f42b4eb3272c166dac75385e48687c92/src/types.rs#L123-L129
				uint32_t e5 = (uint32_t)(e + 15);
				*(uint32_t *)ptr = ((e5 << 27) | ((uint32_t)ptr[2] << 19) | ((uint32_t)ptr[1] << 10) | ((uint32_t)ptr[0] << 1));
			}

			ptr += 4;
		}
	}

	p_image->set_data(width, height, false, Image::FORMAT_RGBE9995, imgdata);

	return OK;
}

void ImageLoaderHDR::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("hdr");
}

ImageLoaderHDR::ImageLoaderHDR() {
}

static Ref<ImageLoaderHDR> image_loader_hdr;

void initialize_hdr_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	image_loader_hdr.instantiate();
	ImageLoader::add_image_format_loader(image_loader_hdr);
}

void uninitialize_hdr_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	ImageLoader::remove_image_format_loader(image_loader_hdr);
	image_loader_hdr.unref();
}
#endif // defined(MODULE_HDR_ENABLED)


// --- Merged from modules/jpg/image_loader_libjpeg_turbo.cpp ---
#if defined(MODULE_JPG_ENABLED)
#include <turbojpeg.h>
#include "core/config/project_settings.h"
#include "core/io/file_access.h"
#include "main/main.h"
// --- Inlined from modules/jpg/image_loader_libjpeg_turbo.h ---
#include "core/io/resource_loader.h"

class ImageLoaderLibJPEGTurbo final : public ImageFormatLoader {
public:
	virtual Error load_image(Ref<Image> p_image, Ref<FileAccess> f, uint32_t p_flags, float p_scale);
	virtual void get_recognized_extensions(List<String> *p_extensions) const;
	ImageLoaderLibJPEGTurbo() = default;
};

#include "main/main.h"

class MovieWriterMJPEG final : public MovieWriter
{
	uint32_t mix_rate = 48000;
	AudioServer::SpeakerMode speaker_mode = AudioServer::SPEAKER_MODE_STEREO;
	uint32_t audio_bit_depth = 16;
	String base_path;
	uint32_t frame_count = 0;
	uint32_t fps = 0;
	float quality = 0.75;

	uint32_t audio_block_size = 0;

	Vector<uint32_t> jpg_frame_sizes;

	uint64_t total_frames_ofs = 0;
	uint64_t total_frames_ofs2 = 0;
	uint64_t total_frames_ofs3 = 0;
	uint64_t total_audio_frames_ofs4 = 0;
	uint64_t movi_data_ofs = 0;

	Ref<FileAccess> f;

protected:
	virtual uint32_t get_audio_mix_rate() const override;
	virtual AudioServer::SpeakerMode get_audio_speaker_mode() const override;
	virtual void get_supported_extensions(List<String>* r_extensions) const override;

	virtual Error write_begin(
		const Size2i& p_movie_size, uint32_t p_fps, const String& p_base_path) override;
	virtual Error write_frame(const Ref<Image>& p_image, const int32_t* p_audio_data) override;
	virtual void write_end() override;

	virtual bool handles_file(const String& p_path) const override;

public:
	MovieWriterMJPEG();
};

// --- End modules/jpg/image_loader_libjpeg_turbo.h ---

Error jpeg_turbo_load_image_from_buffer(Image* p_image, const uint8_t* p_buffer, int p_buffer_len)
{
	tjhandle tj_instance = tj3Init(TJINIT_DECOMPRESS);
	if (tj_instance == nullptr) {
		return FAILED;
	}

	if (tj3DecompressHeader(tj_instance, p_buffer, p_buffer_len) < 0) {
		tj3Destroy(tj_instance);
		return ERR_FILE_CORRUPT;
	}

	const unsigned int width = tj3Get(tj_instance, TJPARAM_JPEGWIDTH);
	const unsigned int height = tj3Get(tj_instance, TJPARAM_JPEGHEIGHT);
	const TJCS colorspace = (TJCS)tj3Get(tj_instance, TJPARAM_COLORSPACE);

	if (tj3Get(tj_instance, TJPARAM_PRECISION) > 8) {
		// TODO: Proceed anyway and convert to rgb8?
		ERR_PRINT("JPEGTurbo: JPEG files with precision > 8 are not currently supported.");
		tj3Destroy(tj_instance);
		return ERR_UNAVAILABLE;
	}

	TJPF tj_pixel_format;
	Image::Format gd_pixel_format;
	if (colorspace == TJCS_GRAY) {
		tj_pixel_format = TJPF_GRAY;
		gd_pixel_format = Image::FORMAT_L8;
	}
	else if (colorspace == TJCS_CMYK || colorspace == TJCS_YCCK) {
		ERR_PRINT(
			"JPEGTurbo: JPEG files with CMYK or YCCK colorspaces are not currently supported.");
		tj3Destroy(tj_instance);
		return ERR_UNAVAILABLE;
	}
	else {
		// Other color spaces should be RGB.
		tj_pixel_format = TJPF_RGB;
		gd_pixel_format = Image::FORMAT_RGB8;
	}

	Vector<uint8_t> data;
	data.resize(width * height * tjPixelSize[tj_pixel_format]);

	if (tj3Decompress8(tj_instance, p_buffer, p_buffer_len, data.ptrw(), 0, tj_pixel_format) < 0) {
		if (tj3GetErrorCode(tj_instance) == TJERR_FATAL) {
			tj3Destroy(tj_instance);
			return ERR_FILE_CORRUPT;
		}
		else {
			WARN_PRINT(String::utf8(tj3GetErrorStr(tj_instance)));
		}
	}

	tj3Destroy(tj_instance);
	p_image->set_data(width, height, false, gd_pixel_format, data);
	return OK;
}

Error ImageLoaderLibJPEGTurbo::load_image(
	Ref<Image> p_image, Ref<FileAccess> f, uint32_t p_flags, float p_scale)
{
	Vector<uint8_t> src_image;
	uint64_t src_image_len = f->get_length();
	ERR_FAIL_COND_V(src_image_len == 0, ERR_FILE_CORRUPT);
	src_image.resize(src_image_len);

	uint8_t* w = src_image.ptrw();

	f->get_buffer(&w[0], src_image_len);

	Error err = jpeg_turbo_load_image_from_buffer(p_image.ptr(), w, src_image_len);

	return err;
}

void ImageLoaderLibJPEGTurbo::get_recognized_extensions(List<String>* p_extensions) const
{
	p_extensions->push_back("jpg");
	p_extensions->push_back("jpeg");
}

static Ref<Image> _jpeg_turbo_mem_loader_func(const uint8_t* p_data, int p_size)
{
	Ref<Image> img;
	img.instantiate();
	Error err = jpeg_turbo_load_image_from_buffer(img.ptr(), p_data, p_size);
	ERR_FAIL_COND_V(err, Ref<Image>());
	return img;
}

uint32_t MovieWriterMJPEG::get_audio_mix_rate() const { return mix_rate; }

AudioServer::SpeakerMode MovieWriterMJPEG::get_audio_speaker_mode() const { return speaker_mode; }

bool MovieWriterMJPEG::handles_file(const String& p_path) const
{
	return p_path.has_extension("avi");
}

void MovieWriterMJPEG::get_supported_extensions(List<String>* r_extensions) const
{
	r_extensions->push_back("avi");
}

Error MovieWriterMJPEG::write_begin(
	const Size2i& p_movie_size, uint32_t p_fps, const String& p_base_path)
{
	// Quick & Dirty MJPEG Code based on -
	// https://docs.microsoft.com/en-us/windows/win32/directshow/avi-riff-file-reference

	base_path = p_base_path.get_basename();
	if (base_path.is_relative_path()) {
		base_path = "res://" + base_path;
	}

	base_path += ".avi";

	f = FileAccess::open(base_path, FileAccess::WRITE_READ);

	fps = p_fps;

	ERR_FAIL_COND_V(f.is_null(), ERR_CANT_OPEN);

	f->store_buffer((const uint8_t*)"RIFF", 4);
	f->store_32(0); // Total length (update later)
	f->store_buffer((const uint8_t*)"AVI ", 4);
	f->store_buffer((const uint8_t*)"LIST", 4);
	f->store_32(300); // 4 + 4 + 4 + 56 + 4 + 4 + 132 + 4 + 4 + 84
	f->store_buffer((const uint8_t*)"hdrl", 4);
	f->store_buffer((const uint8_t*)"avih", 4);
	f->store_32(56);

	f->store_32(1000000 / p_fps); // Microsecs per frame.
	f->store_32(7000);			  // Max bytes per second
	f->store_32(0);				  // Padding Granularity
	f->store_32(16);
	total_frames_ofs = f->get_position();
	f->store_32(0);					  // Total frames (update later)
	f->store_32(0);					  // Initial frames
	f->store_32(1);					  // Streams
	f->store_32(0);					  // Suggested buffer size
	f->store_32(p_movie_size.width);  // Movie Width
	f->store_32(p_movie_size.height); // Movie Height
	for (uint32_t i = 0; i < 4; i++) {
		f->store_32(0); // Reserved.
	}
	f->store_buffer((const uint8_t*)"LIST", 4);
	f->store_32(132); // 4 + 4 + 4 + 48 + 4 + 4 + 40 + 4 + 4 + 16
	f->store_buffer((const uint8_t*)"strl", 4);
	f->store_buffer((const uint8_t*)"strh", 4);
	f->store_32(48);
	f->store_buffer((const uint8_t*)"vids", 4);
	f->store_buffer((const uint8_t*)"MJPG", 4);
	f->store_32(0);		// Flags
	f->store_16(0);		// Priority
	f->store_16(0);		// Language
	f->store_32(0);		// Initial Frames
	f->store_32(1);		// Scale
	f->store_32(p_fps); // FPS
	f->store_32(0);		// Start
	total_frames_ofs2 = f->get_position();
	f->store_32(0); // Number of frames (to be updated later)
	f->store_32(0); // Suggested Buffer Size
	f->store_32(0); // Quality
	f->store_32(0); // Sample Size

	f->store_buffer((const uint8_t*)"strf", 4);
	f->store_32(40); // Size.
	f->store_32(40); // Size.

	f->store_32(p_movie_size.width);			// Width
	f->store_32(p_movie_size.height);			// Width
	f->store_16(1);								// Planes
	f->store_16(24);							// Bitcount
	f->store_buffer((const uint8_t*)"MJPG", 4); // Compression

	f->store_32(
		((p_movie_size.width * 24 / 8 + 3) & 0xFFFFFFFC) * p_movie_size.height); // SizeImage
	f->store_32(0);																 // XPelsXMeter
	f->store_32(0);																 // YPelsXMeter
	f->store_32(0);																 // ClrUsed
	f->store_32(0);																 // ClrImportant

	f->store_buffer((const uint8_t*)"LIST", 4);
	f->store_32(16);

	f->store_buffer((const uint8_t*)"odml", 4);
	f->store_buffer((const uint8_t*)"dmlh", 4);
	f->store_32(4); // sizes

	total_frames_ofs3 = f->get_position();
	f->store_32(0); // Number of frames (to be updated later)

	// Audio //

	const uint32_t bit_depth = audio_bit_depth;
	uint32_t channels = 2;
	switch (speaker_mode) {
	case AudioServer::SPEAKER_MODE_STEREO:
		channels = 2;
		break;
	case AudioServer::SPEAKER_SURROUND_31:
		channels = 4;
		break;
	case AudioServer::SPEAKER_SURROUND_51:
		channels = 6;
		break;
	case AudioServer::SPEAKER_SURROUND_71:
		channels = 8;
		break;
	}
	uint32_t blockalign = bit_depth / 8 * channels;

	f->store_buffer((const uint8_t*)"LIST", 4);
	f->store_32(84); // 4 + 4 + 4 + 48 + 4 + 4 + 16
	f->store_buffer((const uint8_t*)"strl", 4);
	f->store_buffer((const uint8_t*)"strh", 4);
	f->store_32(48);
	f->store_buffer((const uint8_t*)"auds", 4);
	f->store_32(0);						// Handler
	f->store_32(0);						// Flags
	f->store_16(0);						// Priority
	f->store_16(0);						// Language
	f->store_32(0);						// Initial Frames
	f->store_32(blockalign);			// Scale
	f->store_32(mix_rate * blockalign); // mix rate
	f->store_32(0);						// Start
	total_audio_frames_ofs4 = f->get_position();
	f->store_32(0);			 // Number of frames (to be updated later)
	f->store_32(12288);		 // Suggested Buffer Size
	f->store_32(0xFFFFFFFF); // Quality
	f->store_32(blockalign); // Block Align to 32 bits

	audio_block_size = (mix_rate / fps) * blockalign;

	f->store_buffer((const uint8_t*)"strf", 4);
	f->store_32(16); // Standard format, no extra fields
	f->store_16(1);	 // Compression code, standard PCM
	f->store_16(channels);
	f->store_32(mix_rate);				// Samples (frames) / Sec
	f->store_32(mix_rate * blockalign); // Bytes / sec
	f->store_16(blockalign);			// Bytes / sec
	f->store_16(bit_depth);				// Bytes / sec

	f->store_buffer((const uint8_t*)"LIST", 4);
	movi_data_ofs = f->get_position();
	f->store_32(0); // Number of frames (to be updated later)
	f->store_buffer((const uint8_t*)"movi", 4);

	return OK;
}

Error MovieWriterMJPEG::write_frame(const Ref<Image>& p_image, const int32_t* p_audio_data)
{
	ERR_FAIL_COND_V(f.is_null(), ERR_UNCONFIGURED);

	Vector<uint8_t> jpg_buffer = p_image->save_jpg_to_buffer(quality);
	uint32_t s = jpg_buffer.size();

	f->store_buffer((const uint8_t*)"00db", 4); // Stream 0, Video
	f->store_32(jpg_buffer.size());				// sizes
	f->store_buffer(jpg_buffer.ptr(), jpg_buffer.size());
	if (jpg_buffer.size() & 1) {
		f->store_8(0);
		s++;
	}
	jpg_frame_sizes.push_back(s);

	f->store_buffer((const uint8_t*)"01wb", 4); // Stream 1, Audio.
	f->store_32(audio_block_size);
	if (audio_bit_depth == 16) {
		// Convert from 32bit to 16bit.
		Vector<int16_t> audio_buffer_16;
		int num_samples = audio_block_size / 2;
		audio_buffer_16.resize(num_samples);
		for (int i = 0; i < num_samples; ++i) {
			audio_buffer_16.write[i] = (int16_t)(p_audio_data[i] >> 16);
		}
		f->store_buffer((const uint8_t*)audio_buffer_16.ptr(), audio_block_size);
	}
	else {
		f->store_buffer((const uint8_t*)p_audio_data, audio_block_size);
	}

	frame_count++;

	return OK;
}

void MovieWriterMJPEG::write_end()
{
	if (f.is_valid()) {
		// Finalize the file (frame indices)
		f->store_buffer((const uint8_t*)"idx1", 4);
		f->store_32(8 * 4 * frame_count);
		uint32_t ofs = 4;
		uint32_t all_data_size = 0;
		for (uint32_t i = 0; i < frame_count; i++) {
			f->store_buffer((const uint8_t*)"00db", 4);
			f->store_32(16); // AVI_KEYFRAME
			f->store_32(ofs);
			f->store_32(jpg_frame_sizes[i]);

			ofs += jpg_frame_sizes[i] + 8;

			f->store_buffer((const uint8_t*)"01wb", 4);
			f->store_32(16); // AVI_KEYFRAME
			f->store_32(ofs);
			f->store_32(audio_block_size);

			ofs += audio_block_size + 8;
			all_data_size += jpg_frame_sizes[i] + audio_block_size;
		}

		uint32_t file_size = f->get_position();
		f->seek(4);
		f->store_32(file_size - 78);
		f->seek(total_frames_ofs);
		f->store_32(frame_count);
		f->seek(total_frames_ofs2);
		f->store_32(frame_count);
		f->seek(total_frames_ofs3);
		f->store_32(frame_count);
		f->seek(total_audio_frames_ofs4);
		f->store_32(frame_count * mix_rate / fps);
		f->seek(movi_data_ofs);
		f->store_32(all_data_size + 4 + 16 * frame_count);

		f.unref();
	}
}



MovieWriterMJPEG::MovieWriterMJPEG() {}

static Ref<ImageLoaderLibJPEGTurbo> image_loader_libjpeg_turbo;
static MovieWriterMJPEG *writer_mjpeg = nullptr;

void initialize_jpg_module(ModuleInitializationLevel p_level) {
	switch (p_level) {
		case MODULE_INITIALIZATION_LEVEL_SERVERS: {
			writer_mjpeg = memnew(MovieWriterMJPEG);
			MovieWriter::add_writer(writer_mjpeg);
		} break;

		case MODULE_INITIALIZATION_LEVEL_SCENE: {
			image_loader_libjpeg_turbo.instantiate();
			ImageLoader::add_image_format_loader(image_loader_libjpeg_turbo);
		} break;

		default:
			break;
	}
}

void uninitialize_jpg_module(ModuleInitializationLevel p_level) {
	switch (p_level) {
		case MODULE_INITIALIZATION_LEVEL_SCENE: {
			ImageLoader::remove_image_format_loader(image_loader_libjpeg_turbo);
			image_loader_libjpeg_turbo.unref();
		} break;

		case MODULE_INITIALIZATION_LEVEL_SERVERS: {
			memdelete(writer_mjpeg);
		} break;

		default:
			break;
	}
}
#endif // defined(MODULE_JPG_ENABLED)


// --- Merged from modules/ktx/texture_loader_ktx.cpp ---
#if defined(MODULE_KTX_ENABLED)
#include <ktx.h>
#include "main/main.h"
#include "core/io/resource_loader.h"
#include <vk_format.h>
#include "core/io/file_access.h"
#include "core/io/file_access_wrappers.h"
#include "scene/resources/image_texture.h"
#include "servers/rendering/renderer.h"
// --- Inlined from modules/ktx/texture_loader_ktx.h ---
#include "core/io/resource_loader.h"

class ResourceFormatKTX : public ResourceFormatLoader
{
public:
	virtual Ref<Resource> load(const String& p_path, const String& p_original_path = "",
		Error* r_error = nullptr, bool p_use_sub_threads = false, float* r_progress = nullptr,
		CacheMode p_cache_mode = CACHE_MODE_REUSE) override;
	virtual void get_recognized_extensions(List<String>* p_extensions) const override;
	virtual bool handles_type(const String& p_type) const override;
	virtual String get_resource_type(const String& p_path) const override;

	virtual ~ResourceFormatKTX() {}

	ResourceFormatKTX();
};



// --- End modules/ktx/texture_loader_ktx.h ---

KTX_error_code ktx_read(ktxStream* stream, void* dst, const ktx_size_t count)
{
	Ref<FileAccess>* f = reinterpret_cast<Ref<FileAccess>*>(stream->data.custom_ptr.address);
	(*f)->get_buffer(reinterpret_cast<uint8_t*>(dst), count);
	return KTX_SUCCESS;
}

KTX_error_code ktx_skip(ktxStream* stream, const ktx_size_t count)
{
	Ref<FileAccess>* f = reinterpret_cast<Ref<FileAccess>*>(stream->data.custom_ptr.address);
	for (ktx_size_t i = 0; i < count; ++i) {
		(*f)->get_8();
	}
	return KTX_SUCCESS;
}

KTX_error_code ktx_write(
	ktxStream* stream, const void* src, const ktx_size_t size, const ktx_size_t count)
{
	Ref<FileAccess>* f = reinterpret_cast<Ref<FileAccess>*>(stream->data.custom_ptr.address);
	(*f)->store_buffer(reinterpret_cast<const uint8_t*>(src), size * count);
	return KTX_SUCCESS;
}

KTX_error_code ktx_getpos(ktxStream* stream, ktx_off_t* const offset)
{
	Ref<FileAccess>* f = reinterpret_cast<Ref<FileAccess>*>(stream->data.custom_ptr.address);
	*offset = (*f)->get_position();
	return KTX_SUCCESS;
}

KTX_error_code ktx_setpos(ktxStream* stream, const ktx_off_t offset)
{
	Ref<FileAccess>* f = reinterpret_cast<Ref<FileAccess>*>(stream->data.custom_ptr.address);
	(*f)->seek(offset);
	return KTX_SUCCESS;
}

KTX_error_code ktx_getsize(ktxStream* stream, ktx_size_t* const size)
{
	Ref<FileAccess>* f = reinterpret_cast<Ref<FileAccess>*>(stream->data.custom_ptr.address);
	*size = (*f)->get_length();
	return KTX_SUCCESS;
}

void ktx_destruct(ktxStream* stream) { (void)stream; }

static Ref<Image> load_from_file_access(Ref<FileAccess> f, Error* r_error)
{
	ktxStream ktx_stream;
	ktx_stream.read = ktx_read;
	ktx_stream.skip = ktx_skip;
	ktx_stream.write = ktx_write;
	ktx_stream.getpos = ktx_getpos;
	ktx_stream.setpos = ktx_setpos;
	ktx_stream.getsize = ktx_getsize;
	ktx_stream.destruct = ktx_destruct;
	ktx_stream.type = eStreamTypeCustom;
	ktx_stream.data.custom_ptr.address = &f;
	ktx_stream.data.custom_ptr.allocatorAddress = nullptr;
	ktx_stream.data.custom_ptr.size = 0;
	ktx_stream.readpos = 0;
	ktx_stream.closeOnDestruct = false;
	ktxTexture* ktx_texture;
	KTX_error_code result = ktxTexture_CreateFromStream(
		&ktx_stream, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktx_texture);
	if (result != KTX_SUCCESS) {
		ERR_FAIL_V_MSG(Ref<Resource>(), "Invalid or unsupported KTX texture file.");
	}

	if (ktx_texture->numDimensions != 2) {
		ktxTexture_Destroy(ktx_texture);
		ERR_FAIL_V_MSG(Ref<Resource>(), "Unsupported non-2D KTX texture file.");
	}

	if (ktx_texture->isCubemap || ktx_texture->numFaces != 1) {
		ktxTexture_Destroy(ktx_texture);
		ERR_FAIL_V_MSG(Ref<Resource>(), "Unsupported cube map KTX texture file.");
	}

	if (ktx_texture->isArray) {
		ktxTexture_Destroy(ktx_texture);
		ERR_FAIL_V_MSG(Ref<Resource>(), "Unsupported array KTX texture file.");
	}

	uint32_t width = ktx_texture->baseWidth;
	uint32_t height = ktx_texture->baseHeight;
	uint32_t mipmaps = ktx_texture->numLevels;
	Image::Format format;
	bool srgb = false;

	switch (ktx_texture->classId) {
	case ktxTexture1_c:
		switch (((ktxTexture1*)ktx_texture)->glInternalformat) {
		case GL_LUMINANCE:
			format = Image::FORMAT_L8;
			break;
		case GL_LUMINANCE_ALPHA:
			format = Image::FORMAT_LA8;
			break;
		case GL_SRGB8:
			format = Image::FORMAT_RGB8;
			srgb = true;
			break;
		case GL_SRGB8_ALPHA8:
			format = Image::FORMAT_RGBA8;
			srgb = true;
			break;
		case GL_R8:
		case GL_R8UI:
			format = Image::FORMAT_R8;
			break;
		case GL_RG8:
			format = Image::FORMAT_RG8;
			break;
		case GL_RGB8:
			format = Image::FORMAT_RGB8;
			break;
		case GL_RGBA8:
			format = Image::FORMAT_RGBA8;
			break;
		case GL_RGBA4:
			format = Image::FORMAT_RGBA4444;
			break;
		case GL_RGB565:
			format = Image::FORMAT_RGB565;
			break;
		case GL_R32F:
			format = Image::FORMAT_RF;
			break;
		case GL_RG32F:
			format = Image::FORMAT_RGF;
			break;
		case GL_RGB32F:
			format = Image::FORMAT_RGBF;
			break;
		case GL_RGBA32F:
			format = Image::FORMAT_RGBAF;
			break;
		case GL_R16F:
			format = Image::FORMAT_RH;
			break;
		case GL_RG16F:
			format = Image::FORMAT_RGH;
			break;
		case GL_RGB16F:
			format = Image::FORMAT_RGBH;
			break;
		case GL_RGBA16F:
			format = Image::FORMAT_RGBAH;
			break;
		case GL_RGB9_E5:
			format = Image::FORMAT_RGBE9995;
			break;
		case GL_R16:
			format = Image::FORMAT_R16;
			break;
		case GL_RG16:
			format = Image::FORMAT_RG16;
			break;
		case GL_RGB16:
			format = Image::FORMAT_RGB16;
			break;
		case GL_RGBA16:
			format = Image::FORMAT_RGBA16;
			break;
		case GL_R16UI:
			format = Image::FORMAT_R16I;
			break;
		case GL_RG16UI:
			format = Image::FORMAT_RG16I;
			break;
		case GL_RGB16UI:
			format = Image::FORMAT_RGB16I;
			break;
		case GL_RGBA16UI:
			format = Image::FORMAT_RGBA16I;
			break;
		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
			format = Image::FORMAT_DXT1;
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
			format = Image::FORMAT_DXT3;
			break;
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			format = Image::FORMAT_DXT5;
			break;
		case GL_COMPRESSED_RED_RGTC1:
			format = Image::FORMAT_RGTC_R;
			break;
		case GL_COMPRESSED_RG_RGTC2:
			format = Image::FORMAT_RGTC_RG;
			break;
		case GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT:
			format = Image::FORMAT_BPTC_RGBFU;
			break;
		case GL_COMPRESSED_RGBA_BPTC_UNORM:
			format = Image::FORMAT_BPTC_RGBA;
			break;
		case GL_ETC1_RGB8_OES:
			format = Image::FORMAT_ETC;
			break;
		case GL_COMPRESSED_R11_EAC:
			format = Image::FORMAT_ETC2_R11;
			break;
		// Decompression is not supported for this format.
		/*case GL_COMPRESSED_SIGNED_R11_EAC:
			format = Image::FORMAT_ETC2_R11S;
			break;*/
		case GL_COMPRESSED_RG11_EAC:
			format = Image::FORMAT_ETC2_RG11;
			break;
		// Decompression is not supported for this format.
		/*case GL_COMPRESSED_SIGNED_RG11_EAC:
			format = Image::FORMAT_ETC2_RG11S;
			break;*/
		case GL_COMPRESSED_RGB8_ETC2:
			format = Image::FORMAT_ETC2_RGB8;
			break;
		case GL_COMPRESSED_RGBA8_ETC2_EAC:
			format = Image::FORMAT_ETC2_RGBA8;
			break;
		// Decompression is not supported for this format.
		/*case GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2:
			format = Image::FORMAT_ETC2_RGB8A1;
			break;*/
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR:
			format = Image::FORMAT_ASTC_4x4;
			break;
		case GL_COMPRESSED_RGBA_ASTC_4x4_KHR:
			format = Image::FORMAT_ASTC_4x4_HDR;
			break;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR:
			format = Image::FORMAT_ASTC_6x6;
			break;
		case GL_COMPRESSED_RGBA_ASTC_6x6_KHR:
			format = Image::FORMAT_ASTC_6x6_HDR;
			break;
		case GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR:
			format = Image::FORMAT_ASTC_8x8;
			break;
		case GL_COMPRESSED_RGBA_ASTC_8x8_KHR:
			format = Image::FORMAT_ASTC_8x8_HDR;
			break;
		default:
			ktxTexture_Destroy(ktx_texture);
			ERR_FAIL_V_MSG(Ref<Resource>(),
				"Unsupported format " + itos(((ktxTexture1*)ktx_texture)->glInternalformat) +
					" of KTX1 texture file.");
		}
		break;
	case ktxTexture2_c: {
		ktxTexture2* ktx_texture2 = reinterpret_cast<ktxTexture2*>(ktx_texture);
		if (ktx_texture2->vkFormat == VK_FORMAT_UNDEFINED) {
			if (!ktxTexture2_NeedsTranscoding(ktx_texture2)) {
				ktxTexture_Destroy(ktx_texture);
				ERR_FAIL_V_MSG(
					Ref<Resource>(), "Invalid VK_FORMAT_UNDEFINED of KTX2 texture file.");
			}
			ktx_transcode_fmt_e ktxfmt;
			switch (ktxTexture2_GetNumComponents(ktx_texture2)) {
			case 1: {
				if (ktxTexture2_GetOETF_e(ktx_texture2) ==
					KHR_DF_TRANSFER_SRGB) { // TODO srgb native support
					ktxfmt = KTX_TTF_RGBA32;
				}
				else if (Renderer::has_os_feature("rgtc")) {
					ktxfmt = KTX_TTF_BC4_R;
				}
				else {
					ktxfmt = KTX_TTF_RGBA32;
				}
				break;
			}
			case 2: {
				if (ktxTexture2_GetOETF_e(ktx_texture2) ==
					KHR_DF_TRANSFER_SRGB) { // TODO srgb native support
					ktxfmt = KTX_TTF_RGBA32;
				}
				else if (Renderer::has_os_feature("rgtc")) {
					ktxfmt = KTX_TTF_BC5_RG;
				}
				else {
					ktxfmt = KTX_TTF_RGBA32;
				}
				break;
			}
			case 3: {
				if (ktxTexture2_GetOETF_e(ktx_texture2) ==
					KHR_DF_TRANSFER_SRGB) { // TODO: srgb native support
					ktxfmt = KTX_TTF_RGBA32;
				}
				else if (Renderer::has_os_feature("bptc")) {
					ktxfmt = KTX_TTF_BC7_RGBA;
				}
				else if (Renderer::has_os_feature("s3tc")) {
					ktxfmt = KTX_TTF_BC1_RGB;
				}
				else if (Renderer::has_os_feature("etc2")) {
					ktxfmt = KTX_TTF_ETC1_RGB;
				}
				else {
					ktxfmt = KTX_TTF_RGBA32;
				}
				break;
			}
			case 4: {
				if (ktxTexture2_GetOETF_e(ktx_texture2) ==
					KHR_DF_TRANSFER_SRGB) { // TODO srgb native support
					ktxfmt = KTX_TTF_RGBA32;
				}
				else if (Renderer::has_os_feature("astc")) {
					ktxfmt = KTX_TTF_ASTC_4x4_RGBA;
				}
				else if (Renderer::has_os_feature("bptc")) {
					ktxfmt = KTX_TTF_BC7_RGBA;
				}
				else if (Renderer::has_os_feature("s3tc")) {
					ktxfmt = KTX_TTF_BC3_RGBA;
				}
				else if (Renderer::has_os_feature("etc2")) {
					ktxfmt = KTX_TTF_ETC2_RGBA;
				}
				else {
					ktxfmt = KTX_TTF_RGBA32;
				}
				break;
			}
			default: {
				ktxTexture_Destroy(ktx_texture);
				ERR_FAIL_V_MSG(Ref<Resource>(), "Invalid components of KTX2 texture file.");
			}
			}
			result = ktxTexture2_TranscodeBasis(ktx_texture2, ktxfmt, 0);
			if (result != KTX_SUCCESS) {
				ktxTexture_Destroy(ktx_texture);
				ERR_FAIL_V_MSG(Ref<Resource>(), "Failed to transcode KTX2 texture file.");
			}
		}
		switch (ktx_texture2->vkFormat) {
		case VK_FORMAT_R8_UNORM:
			format = Image::FORMAT_L8;
			break;
		case VK_FORMAT_R8G8_UNORM:
			format = Image::FORMAT_LA8;
			break;
		case VK_FORMAT_R8G8B8_SRGB:
			format = Image::FORMAT_RGB8;
			srgb = true;
			break;
		case VK_FORMAT_R8G8B8A8_SRGB:
			format = Image::FORMAT_RGBA8;
			srgb = true;
			break;
		case VK_FORMAT_R8_UINT:
			format = Image::FORMAT_R8;
			break;
		case VK_FORMAT_R8G8_UINT:
			format = Image::FORMAT_RG8;
			break;
		case VK_FORMAT_R8G8B8_UINT:
			format = Image::FORMAT_RGB8;
			break;
		case VK_FORMAT_R8G8B8A8_UINT:
			format = Image::FORMAT_RGBA8;
			break;
		case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
			format = Image::FORMAT_RGBA4444;
			break;
		case VK_FORMAT_R5G6B5_UNORM_PACK16:
			format = Image::FORMAT_RGB565;
			break;
		case VK_FORMAT_R32_SFLOAT:
			format = Image::FORMAT_RF;
			break;
		case VK_FORMAT_R32G32_SFLOAT:
			format = Image::FORMAT_RGF;
			break;
		case VK_FORMAT_R32G32B32_SFLOAT:
			format = Image::FORMAT_RGBF;
			break;
		case VK_FORMAT_R32G32B32A32_SFLOAT:
			format = Image::FORMAT_RGBAF;
			break;
		case VK_FORMAT_R16_SFLOAT:
			format = Image::FORMAT_RH;
			break;
		case VK_FORMAT_R16G16_SFLOAT:
			format = Image::FORMAT_RGH;
			break;
		case VK_FORMAT_R16G16B16_SFLOAT:
			format = Image::FORMAT_RGBH;
			break;
		case VK_FORMAT_R16G16B16A16_SFLOAT:
			format = Image::FORMAT_RGBAH;
			break;
		case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
			format = Image::FORMAT_RGBE9995;
			break;
		case VK_FORMAT_R16_UNORM:
			format = Image::FORMAT_R16;
			break;
		case VK_FORMAT_R16G16_UNORM:
			format = Image::FORMAT_RG16;
			break;
		case VK_FORMAT_R16G16B16_UNORM:
			format = Image::FORMAT_RGB16;
			break;
		case VK_FORMAT_R16G16B16A16_UNORM:
			format = Image::FORMAT_RGBA16;
			break;
		case VK_FORMAT_R16_UINT:
			format = Image::FORMAT_R16I;
			break;
		case VK_FORMAT_R16G16_UINT:
			format = Image::FORMAT_RG16I;
			break;
		case VK_FORMAT_R16G16B16_UINT:
			format = Image::FORMAT_RGB16I;
			break;
		case VK_FORMAT_R16G16B16A16_UINT:
			format = Image::FORMAT_RGBA16I;
			break;
		case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
		case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
			format = Image::FORMAT_DXT1;
			break;
		case VK_FORMAT_BC2_UNORM_BLOCK:
			format = Image::FORMAT_DXT3;
			break;
		case VK_FORMAT_BC3_UNORM_BLOCK:
			format = Image::FORMAT_DXT5;
			break;
		case VK_FORMAT_BC4_UNORM_BLOCK:
			format = Image::FORMAT_RGTC_R;
			break;
		case VK_FORMAT_BC5_UNORM_BLOCK:
			format = Image::FORMAT_RGTC_RG;
			break;
		case VK_FORMAT_BC6H_UFLOAT_BLOCK:
			format = Image::FORMAT_BPTC_RGBFU;
			break;
		case VK_FORMAT_BC6H_SFLOAT_BLOCK:
			format = Image::FORMAT_BPTC_RGBF;
			break;
		case VK_FORMAT_BC7_UNORM_BLOCK:
			format = Image::FORMAT_BPTC_RGBA;
			break;
		case VK_FORMAT_EAC_R11_UNORM_BLOCK:
			format = Image::FORMAT_ETC2_R11;
			break;
		// Decompression is not supported for this format.
		/*case VK_FORMAT_EAC_R11_SNORM_BLOCK:
			format = Image::FORMAT_ETC2_R11S;
			break;*/
		case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
			format = Image::FORMAT_ETC2_RG11;
			break;
		// Decompression is not supported for this format.
		/*case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
			format = Image::FORMAT_ETC2_RG11S;
			break;*/
		case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
			format = Image::FORMAT_ETC2_RGB8;
			break;
		case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
			format = Image::FORMAT_ETC2_RGBA8;
			break;
		// Decompression is not supported for this format.
		/*case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
			format = Image::FORMAT_ETC2_RGB8A1;
			break;*/
		case VK_FORMAT_ASTC_4x4_SRGB_BLOCK:
			format = Image::FORMAT_ASTC_4x4;
			break;
		case VK_FORMAT_ASTC_4x4_UNORM_BLOCK:
			format = Image::FORMAT_ASTC_4x4_HDR;
			break;
		case VK_FORMAT_ASTC_6x6_SRGB_BLOCK:
			format = Image::FORMAT_ASTC_6x6;
			break;
		case VK_FORMAT_ASTC_6x6_UNORM_BLOCK:
			format = Image::FORMAT_ASTC_6x6_HDR;
			break;
		case VK_FORMAT_ASTC_8x8_SRGB_BLOCK:
			format = Image::FORMAT_ASTC_8x8;
			break;
		case VK_FORMAT_ASTC_8x8_UNORM_BLOCK:
			format = Image::FORMAT_ASTC_8x8_HDR;
			break;
		default:
			ktxTexture_Destroy(ktx_texture);
			ERR_FAIL_V_MSG(Ref<Resource>(), "Unsupported format " +
												itos(((ktxTexture2*)ktx_texture)->vkFormat) +
												" of KTX2 texture file.");
			break;
		}
		break;
	}
	default:
		ktxTexture_Destroy(ktx_texture);
		ERR_FAIL_V_MSG(Ref<Resource>(), "Unsupported version KTX texture file.");
		break;
	}

	Vector<uint8_t> src_data;

	// KTX use 4-bytes padding, don't use mipmaps if padding is effective
	// TODO: unpad dynamically
	int pixel_size = Image::get_format_pixel_size(format);
	int block = Image::get_format_block_size(format);
	int minw, minh;
	Image::get_format_min_pixel_size(format, minw, minh);
	int w = width;
	int h = height;
	for (uint32_t i = 0; i < mipmaps; ++i) {
		ktx_size_t mip_size = ktxTexture_GetImageSize(ktx_texture, i);
		size_t bw = w % block != 0 ? w + (block - w % block) : w;
		size_t bh = h % block != 0 ? h + (block - h % block) : h;
		size_t s = bw * bh;
		s *= pixel_size;
		s = Image::get_format_pixels_shifted(format, s);
		if (mip_size != static_cast<ktx_size_t>(s)) {
			if (!i) {
				ktxTexture_Destroy(ktx_texture);
				ERR_FAIL_V_MSG(Ref<Resource>(), "Unsupported padded KTX texture file.");
			}
			mipmaps = 1;
			break;
		}
		w = MAX(minw, w >> 1);
		h = MAX(minh, h >> 1);
	}

	for (uint32_t i = 0; i < mipmaps; ++i) {
		ktx_size_t mip_size = ktxTexture_GetImageSize(ktx_texture, i);
		ktx_size_t offset;
		if (ktxTexture_GetImageOffset(ktx_texture, i, 0, 0, &offset) != KTX_SUCCESS) {
			ktxTexture_Destroy(ktx_texture);
			ERR_FAIL_V_MSG(Ref<Resource>(), "Invalid KTX texture file.");
		}
		int prev_size = src_data.size();
		src_data.resize(prev_size + mip_size);
		memcpy(src_data.ptrw() + prev_size, ktxTexture_GetData(ktx_texture) + offset, mip_size);
	}

	Ref<Image> img = memnew(Image(width, height, mipmaps - 1, format, src_data));
	if (srgb) {
		img->srgb_to_linear();
	}

	if (r_error) {
		*r_error = OK;
	}

	ktxTexture_Destroy(ktx_texture);
	return img;
}

Ref<Resource> ResourceFormatKTX::load(const String& p_path, const String& p_original_path,
	Error* r_error, bool p_use_sub_threads, float* r_progress, CacheMode p_cache_mode)
{
	if (r_error) {
		*r_error = ERR_CANT_OPEN;
	}

	Error err;
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ, &err);
	if (f.is_null()) {
		return Ref<Resource>();
	}

	Ref<FileAccess> fref(f);
	if (r_error) {
		*r_error = ERR_FILE_CORRUPT;
	}

	ERR_FAIL_COND_V_MSG(
		err != OK, Ref<Resource>(), "Unable to open KTX texture file '" + p_path + "'.");
	Ref<Image> img = load_from_file_access(f, r_error);
	Ref<ImageTexture> texture = ImageTexture::create_from_image(img);
	return texture;
}

static Ref<Image> _ktx_mem_loader_func(const uint8_t* p_ktx, int p_size)
{
	Ref<FileAccessMemory> f;
	f.instantiate();
	f->open_custom(p_ktx, p_size);
	Error err;
	Ref<Image> img = load_from_file_access(f, &err);
	ERR_FAIL_COND_V(err, Ref<Image>());
	return img;
}

void ResourceFormatKTX::get_recognized_extensions(List<String>* p_extensions) const
{
	p_extensions->push_back("ktx");
	p_extensions->push_back("ktx2");
}

bool ResourceFormatKTX::handles_type(const String& p_type) const { return true; }

String ResourceFormatKTX::get_resource_type(const String& p_path) const
{
	if (p_path.has_extension("ktx") || p_path.has_extension("ktx2")) {
		return "ImageTexture";
	}
	return "";
}

ResourceFormatKTX::ResourceFormatKTX() { Image::_ktx_mem_loader_func = _ktx_mem_loader_func; }

static Ref<ResourceFormatKTX> resource_loader_ktx;

void initialize_ktx_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	resource_loader_ktx.instantiate();
	ResourceLoader::add_resource_format_loader(resource_loader_ktx);
}

void uninitialize_ktx_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	ResourceLoader::remove_resource_format_loader(resource_loader_ktx);
	resource_loader_ktx.unref();
}
#endif // defined(MODULE_KTX_ENABLED)


// --- Merged from modules/tga/image_loader_tga.cpp ---
#if defined(MODULE_TGA_ENABLED)
// --- Inlined from modules/tga/image_loader_tga.h ---
#include "core/io/resource_loader.h"

#define TGA_IMAGE_DESCRIPTOR_ALPHA_MASK 0xf

class ImageLoaderTGA : public ImageFormatLoader {
	enum tga_type_e {
		TGA_TYPE_NO_DATA = 0,
		TGA_TYPE_INDEXED = 1,
		TGA_TYPE_RGB = 2,
		TGA_TYPE_MONOCHROME = 3,
		TGA_TYPE_RLE_INDEXED = 9,
		TGA_TYPE_RLE_RGB = 10,
		TGA_TYPE_RLE_MONOCHROME = 11
	};

	enum tga_origin_e {
		TGA_ORIGIN_BOTTOM_LEFT = 0x00,
		TGA_ORIGIN_BOTTOM_RIGHT = 0x01,
		TGA_ORIGIN_TOP_LEFT = 0x02,
		TGA_ORIGIN_TOP_RIGHT = 0x03,
		TGA_ORIGIN_SHIFT = 0x04,
		TGA_ORIGIN_MASK = 0x30
	};

	struct tga_header_s {
		uint8_t id_length = 0;
		uint8_t color_map_type = 0;
		tga_type_e image_type;

		uint16_t first_color_entry = 0;
		uint16_t color_map_length = 0;
		uint8_t color_map_depth = 0;

		uint16_t x_origin = 0;
		uint16_t y_origin = 0;
		uint16_t image_width = 0;
		uint16_t image_height = 0;
		uint8_t pixel_depth = 0;
		uint8_t image_descriptor = 0;
	};
	static Error decode_tga_rle(const uint8_t *p_compressed_buffer, size_t p_pixel_size, uint8_t *p_uncompressed_buffer, size_t p_output_size, size_t p_input_size);
	static Error convert_to_image(Ref<Image> p_image, const uint8_t *p_buffer, const tga_header_s &p_header, const uint8_t *p_palette, const bool p_is_monochrome, size_t p_input_size);

public:
	virtual Error load_image(Ref<Image> p_image, Ref<FileAccess> f, uint32_t p_flags, float p_scale);
	virtual void get_recognized_extensions(List<String> *p_extensions) const;
	ImageLoaderTGA();
};

// --- End modules/tga/image_loader_tga.h ---
#include "main/main.h"

#include "core/error/error_macros.h"
#include "core/io/file_access_wrappers.h"
#include "core/io/image.h"

Error ImageLoaderTGA::decode_tga_rle(const uint8_t *p_compressed_buffer, size_t p_pixel_size, uint8_t *p_uncompressed_buffer, size_t p_output_size, size_t p_input_size) {
	Error error;

	Vector<uint8_t> pixels;
	error = pixels.resize(p_pixel_size);
	if (error != OK) {
		return error;
	}

	uint8_t *pixels_w = pixels.ptrw();

	size_t compressed_pos = 0;
	size_t output_pos = 0;
	size_t c = 0;
	size_t count = 0;

	while (output_pos < p_output_size) {
		c = p_compressed_buffer[compressed_pos];
		compressed_pos += 1;
		count = (c & 0x7f) + 1;

		if (output_pos + count * p_pixel_size > p_output_size) {
			return ERR_PARSE_ERROR;
		}

		if (c & 0x80) {
			if (compressed_pos + p_pixel_size > p_input_size) {
				return ERR_PARSE_ERROR;
			}
			for (size_t i = 0; i < p_pixel_size; i++) {
				pixels_w[i] = p_compressed_buffer[compressed_pos];
				compressed_pos += 1;
			}
			for (size_t i = 0; i < count; i++) {
				for (size_t j = 0; j < p_pixel_size; j++) {
					p_uncompressed_buffer[output_pos + j] = pixels_w[j];
				}
				output_pos += p_pixel_size;
			}
		} else {
			if (compressed_pos + count * p_pixel_size > p_input_size) {
				return ERR_PARSE_ERROR;
			}
			count *= p_pixel_size;
			for (size_t i = 0; i < count; i++) {
				p_uncompressed_buffer[output_pos] = p_compressed_buffer[compressed_pos];
				compressed_pos += 1;
				output_pos += 1;
			}
		}
	}
	return OK;
}

Error ImageLoaderTGA::convert_to_image(Ref<Image> p_image, const uint8_t *p_buffer, const tga_header_s &p_header, const uint8_t *p_palette, const bool p_is_monochrome, size_t p_input_size) {
#define TGA_PUT_PIXEL(r, g, b, a) \
	int image_data_ofs = ((y * width) + x); \
	image_data_w[image_data_ofs * 4 + 0] = r; \
	image_data_w[image_data_ofs * 4 + 1] = g; \
	image_data_w[image_data_ofs * 4 + 2] = b; \
	image_data_w[image_data_ofs * 4 + 3] = a;

	uint32_t width = p_header.image_width;
	uint32_t height = p_header.image_height;
	tga_origin_e origin = static_cast<tga_origin_e>((p_header.image_descriptor & TGA_ORIGIN_MASK) >> TGA_ORIGIN_SHIFT);
	uint8_t alpha_bits = p_header.image_descriptor & TGA_IMAGE_DESCRIPTOR_ALPHA_MASK;
	uint32_t x_start;
	int32_t x_step;
	uint32_t x_end;
	uint32_t y_start;
	int32_t y_step;
	uint32_t y_end;

	if (origin == TGA_ORIGIN_TOP_LEFT || origin == TGA_ORIGIN_TOP_RIGHT) {
		y_start = 0;
		y_step = 1;
		y_end = height;
	} else {
		y_start = height - 1;
		y_step = -1;
		y_end = -1;
	}

	if (origin == TGA_ORIGIN_TOP_LEFT || origin == TGA_ORIGIN_BOTTOM_LEFT) {
		x_start = 0;
		x_step = 1;
		x_end = width;
	} else {
		x_start = width - 1;
		x_step = -1;
		x_end = -1;
	}

	Vector<uint8_t> image_data;
	image_data.resize(width * height * sizeof(uint32_t));
	uint8_t *image_data_w = image_data.ptrw();

	size_t i = 0;
	uint32_t x = x_start;
	uint32_t y = y_start;

	if (p_header.pixel_depth == 8) {
		if (p_is_monochrome) {
			while (y != y_end) {
				while (x != x_end) {
					if (i >= p_input_size) {
						return ERR_PARSE_ERROR;
					}
					uint8_t shade = p_buffer[i];

					TGA_PUT_PIXEL(shade, shade, shade, 0xff)

					x += x_step;
					i += 1;
				}
				x = x_start;
				y += y_step;
			}
		} else {
			while (y != y_end) {
				while (x != x_end) {
					if (i >= p_input_size) {
						return ERR_PARSE_ERROR;
					}
					uint8_t index = p_buffer[i];
					uint8_t r = 0x00;
					uint8_t g = 0x00;
					uint8_t b = 0x00;
					uint8_t a = 0xff;

					if (p_header.color_map_depth == 24) {
						// Due to low-high byte order, the color table must be
						// read in the same order as image data (little endian)
						r = (p_palette[(index * 3) + 2]);
						g = (p_palette[(index * 3) + 1]);
						b = (p_palette[(index * 3) + 0]);
					} else {
						return ERR_INVALID_DATA;
					}

					TGA_PUT_PIXEL(r, g, b, a)

					x += x_step;
					i += 1;
				}
				x = x_start;
				y += y_step;
			}
		}
	} else if (p_header.pixel_depth == 16) {
		while (y != y_end) {
			while (x != x_end) {
				if (i + 1 >= p_input_size) {
					return ERR_PARSE_ERROR;
				}

				// Always stored as RGBA5551
				uint8_t r = (p_buffer[i + 1] & 0x7c) << 1;
				uint8_t g = ((p_buffer[i + 1] & 0x03) << 6) | ((p_buffer[i + 0] & 0xe0) >> 2);
				uint8_t b = (p_buffer[i + 0] & 0x1f) << 3;
				uint8_t a = (p_buffer[i + 1] & 0x80) ? 0xff : 0;

				TGA_PUT_PIXEL(r, g, b, alpha_bits ? a : 0xff);

				x += x_step;
				i += 2;
			}
			x = x_start;
			y += y_step;
		}
	} else if (p_header.pixel_depth == 24) {
		while (y != y_end) {
			while (x != x_end) {
				if (i + 2 >= p_input_size) {
					return ERR_PARSE_ERROR;
				}

				uint8_t r = p_buffer[i + 2];
				uint8_t g = p_buffer[i + 1];
				uint8_t b = p_buffer[i + 0];

				TGA_PUT_PIXEL(r, g, b, 0xff)

				x += x_step;
				i += 3;
			}
			x = x_start;
			y += y_step;
		}
	} else if (p_header.pixel_depth == 32) {
		while (y != y_end) {
			while (x != x_end) {
				if (i + 3 >= p_input_size) {
					return ERR_PARSE_ERROR;
				}

				uint8_t a = p_buffer[i + 3];
				uint8_t r = p_buffer[i + 2];
				uint8_t g = p_buffer[i + 1];
				uint8_t b = p_buffer[i + 0];

				TGA_PUT_PIXEL(r, g, b, a)

				x += x_step;
				i += 4;
			}
			x = x_start;
			y += y_step;
		}
	}

	p_image->initialize_data(width, height, false, Image::FORMAT_RGBA8, image_data);

	return OK;
}

Error ImageLoaderTGA::load_image(Ref<Image> p_image, Ref<FileAccess> f, uint32_t p_flags, float p_scale) {
	Vector<uint8_t> src_image;
	uint64_t src_image_len = f->get_length();
	ERR_FAIL_COND_V(src_image_len == 0, ERR_FILE_CORRUPT);
	ERR_FAIL_COND_V(src_image_len < (int64_t)sizeof(tga_header_s), ERR_FILE_CORRUPT);
	src_image.resize(src_image_len);

	Error err = OK;

	tga_header_s tga_header;
	tga_header.id_length = f->get_8();
	tga_header.color_map_type = f->get_8();
	tga_header.image_type = static_cast<tga_type_e>(f->get_8());

	tga_header.first_color_entry = f->get_16();
	tga_header.color_map_length = f->get_16();
	tga_header.color_map_depth = f->get_8();

	tga_header.x_origin = f->get_16();
	tga_header.y_origin = f->get_16();
	tga_header.image_width = f->get_16();
	tga_header.image_height = f->get_16();
	tga_header.pixel_depth = f->get_8();
	tga_header.image_descriptor = f->get_8();

	bool is_encoded = (tga_header.image_type == TGA_TYPE_RLE_INDEXED || tga_header.image_type == TGA_TYPE_RLE_RGB || tga_header.image_type == TGA_TYPE_RLE_MONOCHROME);
	bool has_color_map = (tga_header.image_type == TGA_TYPE_RLE_INDEXED || tga_header.image_type == TGA_TYPE_INDEXED);
	bool is_monochrome = (tga_header.image_type == TGA_TYPE_RLE_MONOCHROME || tga_header.image_type == TGA_TYPE_MONOCHROME);

	if (tga_header.image_type == TGA_TYPE_NO_DATA) {
		err = FAILED;
	}

	uint64_t color_map_size;
	if (has_color_map) {
		if (tga_header.color_map_length > 256 || (tga_header.color_map_depth != 24) || tga_header.color_map_type != 1) {
			err = FAILED;
		}
		color_map_size = tga_header.color_map_length * (tga_header.color_map_depth >> 3);
	} else {
		if (tga_header.color_map_type) {
			err = FAILED;
		}
		color_map_size = 0;
	}

	if ((src_image_len - f->get_position()) < (tga_header.id_length + color_map_size)) {
		err = FAILED; // TGA data appears to be truncated (fewer bytes than expected).
	}

	if (tga_header.image_width <= 0 || tga_header.image_height <= 0) {
		err = FAILED;
	}

	if (!(tga_header.pixel_depth == 8 || tga_header.pixel_depth == 16 || tga_header.pixel_depth == 24 || tga_header.pixel_depth == 32)) {
		err = FAILED;
	}

	if (err == OK) {
		f->seek(f->get_position() + tga_header.id_length);

		Vector<uint8_t> palette;

		if (has_color_map) {
			err = palette.resize(color_map_size);
			if (err == OK) {
				uint8_t *palette_w = palette.ptrw();
				f->get_buffer(&palette_w[0], color_map_size);
			} else {
				return OK;
			}
		}

		uint8_t *src_image_w = src_image.ptrw();
		f->get_buffer(&src_image_w[0], src_image_len - f->get_position());

		const uint8_t *src_image_r = src_image.ptr();

		const size_t pixel_size = tga_header.pixel_depth >> 3;
		size_t buffer_size = (tga_header.image_width * tga_header.image_height) * pixel_size;

		Vector<uint8_t> uncompressed_buffer;
		uncompressed_buffer.resize(buffer_size);
		uint8_t *uncompressed_buffer_w = uncompressed_buffer.ptrw();
		const uint8_t *uncompressed_buffer_r;

		const uint8_t *buffer = nullptr;

		if (is_encoded) {
			err = decode_tga_rle(src_image_r, pixel_size, uncompressed_buffer_w, buffer_size, src_image_len);

			if (err == OK) {
				uncompressed_buffer_r = uncompressed_buffer.ptr();
				buffer = uncompressed_buffer_r;
			}
		} else {
			buffer = src_image_r;
			buffer_size = src_image_len;
		};

		if (err == OK) {
			const uint8_t *palette_r = palette.ptr();
			err = convert_to_image(p_image, buffer, tga_header, palette_r, is_monochrome, buffer_size);
		}
	}

	return err;
}

void ImageLoaderTGA::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("tga");
}

static Ref<Image> _tga_mem_loader_func(const uint8_t *p_tga, int p_size) {
	Ref<FileAccessMemory> memfile;
	memfile.instantiate();
	Error open_memfile_error = memfile->open_custom(p_tga, p_size);
	ERR_FAIL_COND_V_MSG(open_memfile_error, Ref<Image>(), "Could not create memfile for TGA image buffer.");

	Ref<Image> img;
	img.instantiate();
	Error load_error = ImageLoaderTGA().load_image(img, memfile, false, 1.0f);
	ERR_FAIL_COND_V_MSG(load_error, Ref<Image>(), "Failed to load TGA image.");
	return img;
}

ImageLoaderTGA::ImageLoaderTGA() {
	Image::_tga_mem_loader_func = _tga_mem_loader_func;
}

static Ref<ImageLoaderTGA> image_loader_tga;

void initialize_tga_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	image_loader_tga.instantiate();
	ImageLoader::add_image_format_loader(image_loader_tga);
}

void uninitialize_tga_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	ImageLoader::remove_image_format_loader(image_loader_tga);
	image_loader_tga.unref();
}
#endif // defined(MODULE_TGA_ENABLED)


// --- Merged from modules/tinyexr/image_loader_tinyexr.cpp ---
#if defined(MODULE_TINYEXR_ENABLED)
// --- Inlined from modules/tinyexr/image_loader_tinyexr.h ---
#include "core/io/resource_loader.h"

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
#include "main/main.h"
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
#endif // defined(MODULE_TINYEXR_ENABLED)


// --- Merged from modules/webp/image_loader_webp.cpp ---
#if defined(MODULE_WEBP_ENABLED)
#include <webp/decode.h>
#include "core/config/project_settings.h"
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "main/main.h"
#include "scene/resources/image_texture.h"
#include <webp/encode.h>
// --- Inlined from modules/webp/image_loader_webp.h ---
#include "core/io/resource_loader.h"

class ImageLoaderWebP final : public ImageFormatLoader {
public:
	virtual Error load_image(Ref<Image> p_image, Ref<FileAccess> f, uint32_t p_flags, float p_scale);
	virtual void get_recognized_extensions(List<String> *p_extensions) const;
	ImageLoaderWebP();
};

namespace WebPCommon {
// Given an image, pack this data into a WebP file.
Vector<uint8_t> _webp_lossy_pack(const Ref<Image> &p_image, float p_quality);
Vector<uint8_t> _webp_lossless_pack(const Ref<Image> &p_image);
// Helper function for those above.
Vector<uint8_t> _webp_packer(const Ref<Image> &p_image, float p_quality, bool p_lossless);
// Given a WebP file, unpack it into an image.
Ref<Image> _webp_unpack(const Vector<uint8_t> &p_buffer);
Error webp_load_image_from_buffer(Image *p_image, const uint8_t *p_buffer, int p_buffer_len);
} //namespace WebPCommon

#include "core/io/resource_loader.h"

class ResourceSaverWebP final : public ResourceFormatSaver
{
public:
	static Error save_image(const String& p_path, const Ref<Image>& p_img,
		const bool p_lossy = false, const float p_quality = 0.75f);
	static Vector<uint8_t> save_image_to_buffer(
		const Ref<Image>& p_img, const bool p_lossy = false, const float p_quality = 0.75f);

	virtual Error save(
		const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags = 0) override;
	virtual bool recognize(const Ref<Resource>& p_resource) const override;
	virtual void get_recognized_extensions(
		const Ref<Resource>& p_resource, List<String>* p_extensions) const override;

	ResourceSaverWebP();
};

// --- End modules/webp/image_loader_webp.h ---

static Ref<Image> _webp_mem_loader_func(const uint8_t* p_webp_data, int p_size)
{
	Ref<Image> img;
	img.instantiate();
	Error err = WebPCommon::webp_load_image_from_buffer(img.ptr(), p_webp_data, p_size);
	ERR_FAIL_COND_V(err, Ref<Image>());
	return img;
}

Error ImageLoaderWebP::load_image(
	Ref<Image> p_image, Ref<FileAccess> f, uint32_t p_flags, float p_scale)
{
	Vector<uint8_t> src_image;
	uint64_t src_image_len = f->get_length();
	ERR_FAIL_COND_V(src_image_len == 0, ERR_FILE_CORRUPT);
	src_image.resize(src_image_len);

	uint8_t* w = src_image.ptrw();

	f->get_buffer(&w[0], src_image_len);

	Error err = WebPCommon::webp_load_image_from_buffer(p_image.ptr(), w, src_image_len);

	return err;
}

void ImageLoaderWebP::get_recognized_extensions(List<String>* p_extensions) const
{
	p_extensions->push_back("webp");
}

ImageLoaderWebP::ImageLoaderWebP()
{
	Image::_webp_mem_loader_func = _webp_mem_loader_func;
	Image::webp_lossy_packer = WebPCommon::_webp_lossy_pack;
	Image::webp_lossless_packer = WebPCommon::_webp_lossless_pack;
	Image::webp_unpacker = WebPCommon::_webp_unpack;
}

Vector<uint8_t> WebPCommon::_webp_lossless_pack(const Ref<Image>& p_image)
{
	return Vector<uint8_t>();
}

Vector<uint8_t> WebPCommon::_webp_packer(
	const Ref<Image>& p_image, float p_quality, bool p_lossless)
{
	return Vector<uint8_t>();
}

namespace WebPCommon
{
Vector<uint8_t> _webp_lossy_pack(const Ref<Image>& p_image, float p_quality)
{
	ERR_FAIL_COND_V(p_image.is_null() || p_image->is_empty(), Vector<uint8_t>());

	return _webp_packer(p_image, CLAMP(p_quality * 100.0f, 0.0f, 100.0f), false);
}

Ref<Image> _webp_unpack(const Vector<uint8_t>& p_buffer)
{
	int size = p_buffer.size();
	ERR_FAIL_COND_V(size <= 0, Ref<Image>());
	const uint8_t* r = p_buffer.ptr();

	// A WebP file uses a RIFF header, which starts with "RIFF____WEBP".
	ERR_FAIL_COND_V(r[0] != 'R' || r[1] != 'I' || r[2] != 'F' || r[3] != 'F' || r[8] != 'W' ||
						r[9] != 'E' || r[10] != 'B' || r[11] != 'P',
		Ref<Image>());
	WebPBitstreamFeatures features;
	if (WebPGetFeatures(r, size, &features) != VP8_STATUS_OK) {
		ERR_FAIL_V_MSG(Ref<Image>(), "Error unpacking WebP image.");
	}

	Vector<uint8_t> dst_image;
	int datasize = features.width * features.height * (features.has_alpha ? 4 : 3);
	dst_image.resize(datasize);

	uint8_t* dst_w = dst_image.ptrw();

	bool errdec = false;
	if (features.has_alpha) {
		errdec = WebPDecodeRGBAInto(r, size, dst_w, datasize, 4 * features.width) == nullptr;
	}
	else {
		errdec = WebPDecodeRGBInto(r, size, dst_w, datasize, 3 * features.width) == nullptr;
	}

	ERR_FAIL_COND_V_MSG(errdec, Ref<Image>(), "Failed decoding WebP image.");

	Ref<Image> img = memnew(Image(features.width, features.height, false,
		features.has_alpha ? Image::FORMAT_RGBA8 : Image::FORMAT_RGB8, dst_image));
	return img;
}

Error webp_load_image_from_buffer(Image* p_image, const uint8_t* p_buffer, int p_buffer_len)
{
	ERR_FAIL_NULL_V(p_image, ERR_INVALID_PARAMETER);

	WebPBitstreamFeatures features;
	if (WebPGetFeatures(p_buffer, p_buffer_len, &features) != VP8_STATUS_OK) {
		ERR_FAIL_V(ERR_FILE_CORRUPT);
	}

	Vector<uint8_t> dst_image;
	int datasize = features.width * features.height * (features.has_alpha ? 4 : 3);
	dst_image.resize(datasize);
	uint8_t* dst_w = dst_image.ptrw();

	bool errdec = false;
	if (features.has_alpha) {
		errdec = WebPDecodeRGBAInto(p_buffer, p_buffer_len, dst_w, datasize, 4 * features.width) ==
				 nullptr;
	}
	else {
		errdec = WebPDecodeRGBInto(p_buffer, p_buffer_len, dst_w, datasize, 3 * features.width) ==
				 nullptr;
	}

	ERR_FAIL_COND_V_MSG(errdec, ERR_FILE_CORRUPT, "Failed decoding WebP image.");

	p_image->set_data(features.width, features.height, false,
		features.has_alpha ? Image::FORMAT_RGBA8 : Image::FORMAT_RGB8, dst_image);

	return OK;
}
} // namespace WebPCommon

Error ResourceSaverWebP::save(
	const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags)
{
	Ref<ImageTexture> texture = p_resource;

	ERR_FAIL_COND_V_MSG(
		texture.is_null(), ERR_INVALID_PARAMETER, "Can't save invalid texture as WebP.");
	ERR_FAIL_COND_V_MSG(
		!texture->get_width(), ERR_INVALID_PARAMETER, "Can't save empty texture as WebP.");

	Ref<Image> img = texture->get_image();

	Error err = save_image(p_path, img);

	return err;
}

Error ResourceSaverWebP::save_image(
	const String& p_path, const Ref<Image>& p_img, const bool p_lossy, const float p_quality)
{
	Vector<uint8_t> buffer = save_image_to_buffer(p_img, p_lossy, p_quality);
	Error err;
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE, &err);
	ERR_FAIL_COND_V_MSG(err, err, vformat("Can't save WebP at path: '%s'.", p_path));

	const uint8_t* reader = buffer.ptr();

	file->store_buffer(reader, buffer.size());
	if (file->get_error() != OK && file->get_error() != ERR_FILE_EOF) {
		return ERR_CANT_CREATE;
	}

	return OK;
}

Vector<uint8_t> ResourceSaverWebP::save_image_to_buffer(
	const Ref<Image>& p_img, const bool p_lossy, const float p_quality)
{
	Vector<uint8_t> buffer;
	if (p_lossy) {
		buffer = WebPCommon::_webp_lossy_pack(p_img, p_quality);
	}
	else {
		buffer = WebPCommon::_webp_lossless_pack(p_img);
	}
	return buffer;
}

ResourceSaverWebP::ResourceSaverWebP()
{
	Image::save_webp_func = &save_image;
	Image::save_webp_buffer_func = &save_image_to_buffer;
}

bool ResourceSaverWebP::recognize(const Ref<Resource>& p_resource) const { return false; }

void ResourceSaverWebP::get_recognized_extensions(
	const Ref<Resource>& p_resource, List<String>* p_extensions) const
{
}

static Ref<ImageLoaderWebP> image_loader_webp;
static Ref<ResourceSaverWebP> resource_saver_webp;

void initialize_webp_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	image_loader_webp.instantiate();
	ImageLoader::add_image_format_loader(image_loader_webp);

	resource_saver_webp.instantiate();
	ResourceSaver::add_resource_format_saver(resource_saver_webp);
}

void uninitialize_webp_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	ImageLoader::remove_image_format_loader(image_loader_webp);
	image_loader_webp.unref();

	ResourceSaver::remove_resource_format_saver(resource_saver_webp);
	resource_saver_webp.unref();
}
#endif // defined(MODULE_WEBP_ENABLED)

// === Merged from modules/astcenc/image_compress_astcenc.cpp ===
#include "modules/modules_enabled.gen.h"

#if defined(MODULE_ASTCENC_ENABLED)
// --- Inlined from modules/astcenc/image_compress_astcenc.h ---
#include "core/io/image.h"

#ifdef TOOLS_ENABLED
void _compress_astc(Image *r_img, Image::UsedChannels p_channels, Image::CompressProfile p_profile);
#endif

void _decompress_astc(Image *r_img);

// --- End modules/astcenc/image_compress_astcenc.h ---
#include "main/main.h"

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
#include "main/main.h"
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
#include "main/main.h"

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
#include "main/main.h"

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

// === Merged from modules/noise/fastnoise_lite.cpp ===
#include "modules/modules_enabled.gen.h"

#if defined(MODULE_NOISE_ENABLED)
#include <cfloat> // FLT_MAX
#include "main/main.h"
#include "servers/rendering/renderer.h"
#include "core/config/engine.h"
#include "scene/resources/image_texture.h"

_FastNoiseLite::FractalType FastNoiseLite::_convert_domain_warp_fractal_type_enum(
	DomainWarpFractalType p_domain_warp_fractal_type)
{
	_FastNoiseLite::FractalType type;
	switch (p_domain_warp_fractal_type) {
	case DOMAIN_WARP_FRACTAL_NONE:
		type = _FastNoiseLite::FractalType_None;
		break;
	case DOMAIN_WARP_FRACTAL_PROGRESSIVE:
		type = _FastNoiseLite::FractalType_DomainWarpProgressive;
		break;
	case DOMAIN_WARP_FRACTAL_INDEPENDENT:
		type = _FastNoiseLite::FractalType_DomainWarpIndependent;
		break;
	default:
		type = _FastNoiseLite::FractalType_None;
	}
	return type;
}

FastNoiseLite::FastNoiseLite()
{
	_noise.SetNoiseType((_FastNoiseLite::NoiseType)noise_type);
	_noise.SetSeed(seed);
	_noise.SetFrequency(frequency);

	_noise.SetFractalType((_FastNoiseLite::FractalType)fractal_type);
	_noise.SetFractalOctaves(fractal_octaves);
	_noise.SetFractalLacunarity(fractal_lacunarity);
	_noise.SetFractalGain(fractal_gain);
	_noise.SetFractalWeightedStrength(fractal_weighted_strength);
	_noise.SetFractalPingPongStrength(fractal_ping_pong_strength);

	_noise.SetCellularDistanceFunction(
		(_FastNoiseLite::CellularDistanceFunction)cellular_distance_function);
	_noise.SetCellularReturnType((_FastNoiseLite::CellularReturnType)cellular_return_type);
	_noise.SetCellularJitter(cellular_jitter);

	_domain_warp_noise.SetDomainWarpType((_FastNoiseLite::DomainWarpType)domain_warp_type);
	_domain_warp_noise.SetSeed(seed);
	_domain_warp_noise.SetDomainWarpAmp(domain_warp_amplitude);
	_domain_warp_noise.SetFrequency(domain_warp_frequency);
	_domain_warp_noise.SetFractalType(
		_convert_domain_warp_fractal_type_enum(domain_warp_fractal_type));
	_domain_warp_noise.SetFractalOctaves(domain_warp_fractal_octaves);
	_domain_warp_noise.SetFractalLacunarity(domain_warp_fractal_lacunarity);
	_domain_warp_noise.SetFractalGain(domain_warp_fractal_gain);
}

FastNoiseLite::~FastNoiseLite() {}

// General settings.

void FastNoiseLite::set_noise_type(NoiseType p_noise_type)
{
	noise_type = p_noise_type;
	_noise.SetNoiseType((_FastNoiseLite::NoiseType)p_noise_type);
	emit_changed();
}

FastNoiseLite::NoiseType FastNoiseLite::get_noise_type() const { return noise_type; }

void FastNoiseLite::set_seed(int p_seed)
{
	seed = p_seed;
	_noise.SetSeed(p_seed);
	_domain_warp_noise.SetSeed(p_seed);
	emit_changed();
}

int FastNoiseLite::get_seed() const { return seed; }

void FastNoiseLite::set_frequency(real_t p_freq)
{
	frequency = p_freq;
	_noise.SetFrequency(p_freq);
	emit_changed();
}

real_t FastNoiseLite::get_frequency() const { return frequency; }

void FastNoiseLite::set_offset(Vector3 p_offset)
{
	offset = p_offset;
	emit_changed();
}

Vector3 FastNoiseLite::get_offset() const { return offset; }

// Fractal.

void FastNoiseLite::set_fractal_type(FractalType p_type)
{
	fractal_type = p_type;
	_noise.SetFractalType((_FastNoiseLite::FractalType)p_type);
	emit_changed();
}

FastNoiseLite::FractalType FastNoiseLite::get_fractal_type() const { return fractal_type; }

void FastNoiseLite::set_fractal_octaves(int p_octaves)
{
	fractal_octaves = p_octaves;
	_noise.SetFractalOctaves(p_octaves);
	emit_changed();
}

int FastNoiseLite::get_fractal_octaves() const { return fractal_octaves; }

void FastNoiseLite::set_fractal_lacunarity(real_t p_lacunarity)
{
	fractal_lacunarity = p_lacunarity;
	_noise.SetFractalLacunarity(p_lacunarity);
	emit_changed();
}

real_t FastNoiseLite::get_fractal_lacunarity() const { return fractal_lacunarity; }

void FastNoiseLite::set_fractal_gain(real_t p_gain)
{
	fractal_gain = p_gain;
	_noise.SetFractalGain(p_gain);
	emit_changed();
}

real_t FastNoiseLite::get_fractal_gain() const { return fractal_gain; }

void FastNoiseLite::set_fractal_weighted_strength(real_t p_weighted_strength)
{
	fractal_weighted_strength = p_weighted_strength;
	_noise.SetFractalWeightedStrength(p_weighted_strength);
	emit_changed();
}

real_t FastNoiseLite::get_fractal_weighted_strength() const { return fractal_weighted_strength; }

void FastNoiseLite::set_fractal_ping_pong_strength(real_t p_ping_pong_strength)
{
	fractal_ping_pong_strength = p_ping_pong_strength;
	_noise.SetFractalPingPongStrength(p_ping_pong_strength);
	emit_changed();
}

real_t FastNoiseLite::get_fractal_ping_pong_strength() const { return fractal_ping_pong_strength; }

// Cellular.

void FastNoiseLite::set_cellular_distance_function(CellularDistanceFunction p_func)
{
	cellular_distance_function = p_func;
	_noise.SetCellularDistanceFunction((_FastNoiseLite::CellularDistanceFunction)p_func);
	emit_changed();
}

FastNoiseLite::CellularDistanceFunction FastNoiseLite::get_cellular_distance_function() const
{
	return cellular_distance_function;
}

void FastNoiseLite::set_cellular_jitter(real_t p_jitter)
{
	cellular_jitter = p_jitter;
	_noise.SetCellularJitter(p_jitter);
	emit_changed();
}

real_t FastNoiseLite::get_cellular_jitter() const { return cellular_jitter; }

void FastNoiseLite::set_cellular_return_type(CellularReturnType p_ret)
{
	cellular_return_type = p_ret;
	_noise.SetCellularReturnType((_FastNoiseLite::CellularReturnType)p_ret);
	emit_changed();
}

FastNoiseLite::CellularReturnType FastNoiseLite::get_cellular_return_type() const
{
	return cellular_return_type;
}

// Domain warp specific.

void FastNoiseLite::set_domain_warp_enabled(bool p_enabled)
{
	if (domain_warp_enabled != p_enabled) {
		domain_warp_enabled = p_enabled;
		emit_changed();
	}
}

bool FastNoiseLite::is_domain_warp_enabled() const { return domain_warp_enabled; }

void FastNoiseLite::set_domain_warp_type(DomainWarpType p_domain_warp_type)
{
	domain_warp_type = p_domain_warp_type;
	_domain_warp_noise.SetDomainWarpType((_FastNoiseLite::DomainWarpType)p_domain_warp_type);
	emit_changed();
}

FastNoiseLite::DomainWarpType FastNoiseLite::get_domain_warp_type() const
{
	return domain_warp_type;
}

void FastNoiseLite::set_domain_warp_amplitude(real_t p_amplitude)
{
	domain_warp_amplitude = p_amplitude;
	_domain_warp_noise.SetDomainWarpAmp(p_amplitude);
	emit_changed();
}

real_t FastNoiseLite::get_domain_warp_amplitude() const { return domain_warp_amplitude; }

void FastNoiseLite::set_domain_warp_frequency(real_t p_frequency)
{
	domain_warp_frequency = p_frequency;
	_domain_warp_noise.SetFrequency(p_frequency);
	emit_changed();
}

real_t FastNoiseLite::get_domain_warp_frequency() const { return domain_warp_frequency; }

void FastNoiseLite::set_domain_warp_fractal_type(DomainWarpFractalType p_domain_warp_fractal_type)
{
	domain_warp_fractal_type = p_domain_warp_fractal_type;

	_domain_warp_noise.SetFractalType(
		_convert_domain_warp_fractal_type_enum(p_domain_warp_fractal_type));
	emit_changed();
}

FastNoiseLite::DomainWarpFractalType FastNoiseLite::get_domain_warp_fractal_type() const
{
	return domain_warp_fractal_type;
}

void FastNoiseLite::set_domain_warp_fractal_octaves(int p_octaves)
{
	domain_warp_fractal_octaves = p_octaves;
	_domain_warp_noise.SetFractalOctaves(p_octaves);
	emit_changed();
}

int FastNoiseLite::get_domain_warp_fractal_octaves() const { return domain_warp_fractal_octaves; }

void FastNoiseLite::set_domain_warp_fractal_lacunarity(real_t p_lacunarity)
{
	domain_warp_fractal_lacunarity = p_lacunarity;
	_domain_warp_noise.SetFractalLacunarity(p_lacunarity);
	emit_changed();
}

real_t FastNoiseLite::get_domain_warp_fractal_lacunarity() const
{
	return domain_warp_fractal_lacunarity;
}

void FastNoiseLite::set_domain_warp_fractal_gain(real_t p_gain)
{
	domain_warp_fractal_gain = p_gain;
	_domain_warp_noise.SetFractalGain(p_gain);
	emit_changed();
}

real_t FastNoiseLite::get_domain_warp_fractal_gain() const { return domain_warp_fractal_gain; }

// Noise evaluation & image generation functions.

real_t FastNoiseLite::get_noise_1d(real_t p_x) const
{
	p_x += offset.x;
	if (domain_warp_enabled) {
		// Needed since DomainWarp expects a reference.
		real_t y_dummy = 0;
		_domain_warp_noise.DomainWarp(p_x, y_dummy);
	}
	return get_noise_2d(p_x, 0.0);
}

real_t FastNoiseLite::get_noise_2dv(Vector2 p_v) const { return get_noise_2d(p_v.x, p_v.y); }

real_t FastNoiseLite::get_noise_2d(real_t p_x, real_t p_y) const
{
	p_x += offset.x;
	p_y += offset.y;
	if (domain_warp_enabled) {
		_domain_warp_noise.DomainWarp(p_x, p_y);
	}
	return _noise.GetNoise(p_x, p_y);
}

real_t FastNoiseLite::get_noise_3dv(Vector3 p_v) const { return get_noise_3d(p_v.x, p_v.y, p_v.z); }

real_t FastNoiseLite::get_noise_3d(real_t p_x, real_t p_y, real_t p_z) const
{
	p_x += offset.x;
	p_y += offset.y;
	p_z += offset.z;
	if (domain_warp_enabled) {
		_domain_warp_noise.DomainWarp(p_x, p_y, p_z);
	}
	return _noise.GetNoise(p_x, p_y, p_z);
}

// Template specialization for faster grayscale blending.
template <> uint8_t FastNoiseLite::_alpha_blend<uint8_t>(uint8_t p_bg, uint8_t p_fg, int p_alpha) const
{
	uint16_t alpha = p_alpha + 1;
	uint16_t inv_alpha = 256 - p_alpha;

	return (uint8_t)((alpha * p_fg + inv_alpha * p_bg) >> 8);
}

Vector<Ref<Image>> FastNoiseLite::_get_seamless_image(int p_width, int p_height, int p_depth,
	bool p_invert, bool p_in_3d_space, real_t p_blend_skirt, bool p_normalize) const
{
	ERR_FAIL_COND_V(p_width <= 0 || p_height <= 0 || p_depth <= 0, Vector<Ref<Image>>());

	int skirt_width = MAX(1, p_width * p_blend_skirt);
	int skirt_height = MAX(1, p_height * p_blend_skirt);
	int skirt_depth = MAX(1, p_depth * p_blend_skirt);
	int src_width = p_width + skirt_width;
	int src_height = p_height + skirt_height;
	int src_depth = p_depth + skirt_depth;

	Vector<Ref<Image>> src =
		_get_image(src_width, src_height, src_depth, p_invert, p_in_3d_space, p_normalize);
	bool grayscale = (src[0]->get_format() == Image::FORMAT_L8);

	if (grayscale) {
		return _generate_seamless_image<uint8_t>(
			src, p_width, p_height, p_depth, p_invert, p_blend_skirt);
	}
	else {
		return _generate_seamless_image<uint32_t>(
			src, p_width, p_height, p_depth, p_invert, p_blend_skirt);
	}
}

Ref<Image> FastNoiseLite::get_seamless_image(int p_width, int p_height, bool p_invert,
	bool p_in_3d_space, real_t p_blend_skirt, bool p_normalize) const
{
	Vector<Ref<Image>> images = _get_seamless_image(
		p_width, p_height, 1, p_invert, p_in_3d_space, p_blend_skirt, p_normalize);
	if (images.is_empty()) {
		return Ref<Image>();
	}
	return images[0];
}

Vector<Ref<Image>> FastNoiseLite::_get_image(int p_width, int p_height, int p_depth, bool p_invert,
	bool p_in_3d_space, bool p_normalize) const
{
	ERR_FAIL_COND_V(p_width <= 0 || p_height <= 0 || p_depth <= 0, Vector<Ref<Image>>());

	Vector<Ref<Image>> images;
	images.resize(p_depth);

	if (p_normalize) {
		// Get all values and identify min/max values.
		LocalVector<real_t> values;
		values.resize(p_width * p_height * p_depth);

		real_t min_val = FLT_MAX;
		real_t max_val = -FLT_MAX;
		int idx = 0;
		for (int d = 0; d < p_depth; d++) {
			for (int y = 0; y < p_height; y++) {
				for (int x = 0; x < p_width; x++) {
					values[idx] = p_in_3d_space ? get_noise_3d(x, y, d) : get_noise_2d(x, y);
					if (values[idx] > max_val) {
						max_val = values[idx];
					}
					if (values[idx] < min_val) {
						min_val = values[idx];
					}
					idx++;
				}
			}
		}
		idx = 0;
		// Normalize values and write to texture.
		for (int d = 0; d < p_depth; d++) {
			Vector<uint8_t> data;
			data.resize(p_width * p_height);

			uint8_t* wd8 = data.ptrw();
			uint8_t ivalue;

			for (int y = 0; y < p_height; y++) {
				for (int x = 0; x < p_width; x++) {
					if (max_val == min_val) {
						ivalue = 0;
					}
					else {
						ivalue = static_cast<uint8_t>(
							CLAMP((values[idx] - min_val) / (max_val - min_val) * 255.f, 0, 255));
					}

					if (p_invert) {
						ivalue = 255 - ivalue;
					}

					wd8[x + y * p_width] = ivalue;
					idx++;
				}
			}
			Ref<Image> img = memnew(Image(p_width, p_height, false, Image::FORMAT_L8, data));
			images.write[d] = img;
		}
	}
	else {
		// Without normalization, the expected range of the noise function is [-1, 1].
		for (int d = 0; d < p_depth; d++) {
			Vector<uint8_t> data;
			data.resize(p_width * p_height);

			uint8_t* wd8 = data.ptrw();

			uint8_t ivalue;
			int idx = 0;
			for (int y = 0; y < p_height; y++) {
				for (int x = 0; x < p_width; x++) {
					float value = (p_in_3d_space ? get_noise_3d(x, y, d) : get_noise_2d(x, y));
					ivalue = static_cast<uint8_t>(CLAMP(value * 127.5f + 127.5f, 0.0f, 255.0f));
					wd8[idx] = p_invert ? (255 - ivalue) : ivalue;
					idx++;
				}
			}

			Ref<Image> img = memnew(Image(p_width, p_height, false, Image::FORMAT_L8, data));
			images.write[d] = img;
		}
	}

	return images;
}

Ref<Image> FastNoiseLite::get_image(
	int p_width, int p_height, bool p_invert, bool p_in_3d_space, bool p_normalize) const
{
	Vector<Ref<Image>> images =
		_get_image(p_width, p_height, 1, p_invert, p_in_3d_space, p_normalize);
	if (images.is_empty()) {
		return Ref<Image>();
	}
	return images[0];
}

void FastNoiseLite::_changed() { emit_changed(); }

NoiseTexture2D::NoiseTexture2D()
{
	noise = Ref<FastNoiseLite>();

	_queue_update();
}

void NoiseTexture2D::_set_texture_image(const Ref<Image>& p_image)
{
	image = p_image;
	if (image.is_valid()) {
		if (texture.is_valid()) {
			RID new_texture = Renderer::texture_2d_create(p_image);
			Renderer::texture_replace(texture, new_texture);
		}
		else {
			texture = Renderer::texture_2d_create(p_image);
		}
		Renderer::texture_set_path(texture, get_path());
	}
	emit_changed();
}

Ref<Image> NoiseTexture2D::_generate_texture()
{
	// Prevent memdelete due to unref() on other thread.
	Ref<FastNoiseLite> ref_noise = noise;

	if (ref_noise.is_null()) {
		return Ref<Image>();
	}

	Ref<Image> new_image;

	if (seamless) {
		new_image = ref_noise->get_seamless_image(
			size.x, size.y, invert, in_3d_space, seamless_blend_skirt, normalize);
	}
	else {
		new_image = ref_noise->get_image(size.x, size.y, invert, in_3d_space, normalize);
	}
	if (color_ramp.is_valid()) {
		new_image = _modulate_with_gradient(new_image, color_ramp);
	}
	if (as_normal_map) {
		new_image->bump_map_to_normal_map(bump_strength);
	}
	if (generate_mipmaps) {
		new_image->generate_mipmaps();
	}

	return new_image;
}

Ref<Image> NoiseTexture2D::_modulate_with_gradient(Ref<Image> p_image, Ref<Gradient> p_gradient)
{
	int width = p_image->get_width();
	int height = p_image->get_height();

	Ref<Image> new_image = Image::create_empty(width, height, false, Image::FORMAT_RGBA8);

	for (int row = 0; row < height; row++) {
		for (int col = 0; col < width; col++) {
			Color pixel_color = p_image->get_pixel(col, row);
			Color ramp_color = p_gradient->get_color_at_offset(pixel_color.get_luminance());
			new_image->set_pixel(col, row, ramp_color);
		}
	}

	return new_image;
}

void NoiseTexture2D::set_noise(Ref<FastNoiseLite> p_noise)
{
	if (noise == p_noise) {
		return;
	}
	noise = p_noise;
	_queue_update();
}

Ref<FastNoiseLite> NoiseTexture2D::get_noise() { return noise; }

void NoiseTexture2D::set_width(int p_width)
{
	ERR_FAIL_COND(p_width <= 0);
	if (p_width == size.x) {
		return;
	}
	size.x = p_width;
	_queue_update();
}

void NoiseTexture2D::set_height(int p_height)
{
	ERR_FAIL_COND(p_height <= 0);
	if (p_height == size.y) {
		return;
	}
	size.y = p_height;
	_queue_update();
}

void NoiseTexture2D::set_invert(bool p_invert)
{
	if (p_invert == invert) {
		return;
	}
	invert = p_invert;
	_queue_update();
}

bool NoiseTexture2D::get_invert() const { return invert; }

void NoiseTexture2D::set_in_3d_space(bool p_enable)
{
	if (p_enable == in_3d_space) {
		return;
	}
	in_3d_space = p_enable;
	_queue_update();
}

bool NoiseTexture2D::is_in_3d_space() const { return in_3d_space; }

void NoiseTexture2D::set_generate_mipmaps(bool p_enable)
{
	if (p_enable == generate_mipmaps) {
		return;
	}
	generate_mipmaps = p_enable;
	_queue_update();
}

bool NoiseTexture2D::is_generating_mipmaps() const { return generate_mipmaps; }

void NoiseTexture2D::set_seamless(bool p_seamless)
{
	if (p_seamless == seamless) {
		return;
	}
	seamless = p_seamless;
	_queue_update();
}

bool NoiseTexture2D::get_seamless() { return seamless; }

void NoiseTexture2D::set_seamless_blend_skirt(real_t p_blend_skirt)
{
	ERR_FAIL_COND(p_blend_skirt < 0 || p_blend_skirt > 1);

	if (p_blend_skirt == seamless_blend_skirt) {
		return;
	}
	seamless_blend_skirt = p_blend_skirt;
	_queue_update();
}

real_t NoiseTexture2D::get_seamless_blend_skirt() { return seamless_blend_skirt; }

void NoiseTexture2D::set_as_normal_map(bool p_as_normal_map)
{
	if (p_as_normal_map == as_normal_map) {
		return;
	}
	as_normal_map = p_as_normal_map;
	_queue_update();
}

bool NoiseTexture2D::is_normal_map() { return as_normal_map; }

void NoiseTexture2D::set_bump_strength(float p_bump_strength)
{
	if (p_bump_strength == bump_strength) {
		return;
	}
	bump_strength = p_bump_strength;
	if (as_normal_map) {
		_queue_update();
	}
}

float NoiseTexture2D::get_bump_strength() { return bump_strength; }

void NoiseTexture2D::set_normalize(bool p_normalize)
{
	if (normalize == p_normalize) {
		return;
	}
	normalize = p_normalize;
	_queue_update();
}

bool NoiseTexture2D::is_normalized() const { return normalize; }

void NoiseTexture2D::set_color_ramp(const Ref<Gradient>& p_gradient)
{
	if (color_ramp == p_gradient) {
		return;
	}
	color_ramp = p_gradient;
	_queue_update();
}

Ref<Gradient> NoiseTexture2D::get_color_ramp() const { return color_ramp; }

int NoiseTexture2D::get_width() const { return size.x; }

int NoiseTexture2D::get_height() const { return size.y; }

RID NoiseTexture2D::get_rid() const
{
	if (!texture.is_valid()) {
		texture = Renderer::texture_2d_placeholder_create();
	}

	return texture;
}

Ref<Image> NoiseTexture2D::get_image() const { return image; }

void NoiseTexture2D::_queue_update() {}

NoiseTexture3D::NoiseTexture3D()
{
	noise = Ref<FastNoiseLite>();

	_queue_update();
}

Ref<Image> NoiseTexture3D::_modulate_with_gradient(Ref<Image> p_image, Ref<Gradient> p_gradient)
{
	int w = p_image->get_width();
	int h = p_image->get_height();

	Ref<Image> new_image = Image::create_empty(w, h, false, Image::FORMAT_RGBA8);

	for (int row = 0; row < h; row++) {
		for (int col = 0; col < w; col++) {
			Color pixel_color = p_image->get_pixel(col, row);
			Color ramp_color = p_gradient->get_color_at_offset(pixel_color.get_luminance());
			new_image->set_pixel(col, row, ramp_color);
		}
	}

	return new_image;
}

void NoiseTexture3D::set_noise(Ref<FastNoiseLite> p_noise)
{
	if (noise == p_noise) {
		return;
	}
	noise = p_noise;
	_queue_update();
}

Ref<FastNoiseLite> NoiseTexture3D::get_noise() { return noise; }

void NoiseTexture3D::set_width(int p_width)
{
	ERR_FAIL_COND(p_width <= 0);
	if (p_width == width) {
		return;
	}
	width = p_width;
	_queue_update();
}

void NoiseTexture3D::set_height(int p_height)
{
	ERR_FAIL_COND(p_height <= 0);
	if (p_height == height) {
		return;
	}
	height = p_height;
	_queue_update();
}

void NoiseTexture3D::set_depth(int p_depth)
{
	ERR_FAIL_COND(p_depth <= 0);
	if (p_depth == depth) {
		return;
	}
	depth = p_depth;
	_queue_update();
}

void NoiseTexture3D::set_invert(bool p_invert)
{
	if (p_invert == invert) {
		return;
	}
	invert = p_invert;
	_queue_update();
}

bool NoiseTexture3D::get_invert() const { return invert; }

void NoiseTexture3D::set_seamless(bool p_seamless)
{
	if (p_seamless == seamless) {
		return;
	}
	seamless = p_seamless;
	_queue_update();
}

bool NoiseTexture3D::get_seamless() { return seamless; }

void NoiseTexture3D::set_seamless_blend_skirt(real_t p_blend_skirt)
{
	ERR_FAIL_COND(p_blend_skirt < 0.05 || p_blend_skirt > 1);

	if (p_blend_skirt == seamless_blend_skirt) {
		return;
	}
	seamless_blend_skirt = p_blend_skirt;
	_queue_update();
}

real_t NoiseTexture3D::get_seamless_blend_skirt() { return seamless_blend_skirt; }

void NoiseTexture3D::set_normalize(bool p_normalize)
{
	if (normalize == p_normalize) {
		return;
	}
	normalize = p_normalize;
	_queue_update();
}

bool NoiseTexture3D::is_normalized() const { return normalize; }

void NoiseTexture3D::set_color_ramp(const Ref<Gradient>& p_gradient)
{
	if (color_ramp == p_gradient) {
		return;
	}
	color_ramp = p_gradient;
	_queue_update();
}

Ref<Gradient> NoiseTexture3D::get_color_ramp() const { return color_ramp; }

int NoiseTexture3D::get_width() const { return width; }

int NoiseTexture3D::get_height() const { return height; }

int NoiseTexture3D::get_depth() const { return depth; }

bool NoiseTexture3D::has_mipmaps() const { return false; }

RID NoiseTexture3D::get_rid() const
{
	if (!texture.is_valid()) {
		texture = Renderer::texture_3d_placeholder_create();
	}

	return texture;
}

Vector<Ref<Image>> NoiseTexture3D::get_data() const
{
	ERR_FAIL_COND_V(!texture.is_valid(), Vector<Ref<Image>>());
	return Renderer::texture_3d_get(texture);
}

Image::Format NoiseTexture3D::get_format() const { return format; }

void NoiseTexture3D::_queue_update() {}

void initialize_noise_module(ModuleInitializationLevel p_level) {}

void uninitialize_noise_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}
#endif // defined(MODULE_NOISE_ENABLED)


// --- Merged from modules/regex/regex.cpp ---
#if defined(MODULE_REGEX_ENABLED)
#include "core/os/memory.h"
#include "main/main.h"
#include "core/string/translation_server.h"

extern "C"
{
#include <pcre2.h>
}

static void* _regex_malloc(PCRE2_SIZE size, void* user) { return memalloc(size); }

static void _regex_free(void* ptr, void* user)
{
	if (ptr) {
		memfree(ptr);
	}
}

String RegExMatch::get_subject() const { return subject; }

int RegExMatch::get_group_count() const
{
	if (data.is_empty()) {
		return 0;
	}
	return data.size() - 1;
}

PackedStringArray RegExMatch::get_strings() const
{
	PackedStringArray result;

	int size = data.size();

	for (int i = 0; i < size; i++) {
		int start = data[i].start;

		if (start == -1) {
			result.append(String());
			continue;
		}

		int length = data[i].end - start;

		result.append(subject.substr(start, length));
	}

	return result;
}

void RegEx::_pattern_info(uint32_t what, void* where) const
{
	pcre2_pattern_info_32((pcre2_code_32*)code, what, where);
}

Ref<RegEx> RegEx::create_from_string(const String& p_pattern, bool p_show_error)
{
	Ref<RegEx> ret;
	ret.instantiate();
	ret->compile(p_pattern, p_show_error);
	return ret;
}

void RegEx::clear()
{
	if (code) {
		pcre2_code_free_32((pcre2_code_32*)code);
		code = nullptr;
	}
}

Error RegEx::compile(const String& p_pattern, bool p_show_error)
{
	pattern = p_pattern;
	clear();

	int err;
	PCRE2_SIZE offset;
	uint32_t flags = PCRE2_DUPNAMES;

	pcre2_general_context_32* gctx = (pcre2_general_context_32*)general_ctx;
	pcre2_compile_context_32* cctx = pcre2_compile_context_create_32(gctx);
	PCRE2_SPTR32 p = (PCRE2_SPTR32)pattern.get_data();

	code = pcre2_compile_32(p, pattern.length(), flags, &err, &offset, cctx);

	pcre2_compile_context_free_32(cctx);

	if (!code) {
		if (p_show_error) {
			PCRE2_UCHAR32 buf[256];
			pcre2_get_error_message_32(err, buf, 256);
			String message = String::num_int64(offset) + ": " + String((const char32_t*)buf);
			ERR_PRINT(message);
		}
		return FAILED;
	}
	return OK;
}

Ref<RegExMatch> RegEx::search(const String& p_subject, int p_offset, int p_end) const
{
	ERR_FAIL_COND_V(!is_valid(), nullptr);
	ERR_FAIL_COND_V_MSG(p_offset < 0, nullptr, "RegEx search offset must be >= 0");

	Ref<RegExMatch> result = memnew(RegExMatch);

	int length = p_subject.length();
	if (p_end >= 0 && p_end < length) {
		length = p_end;
	}

	pcre2_code_32* c = (pcre2_code_32*)code;
	pcre2_general_context_32* gctx = (pcre2_general_context_32*)general_ctx;
	pcre2_match_context_32* mctx = pcre2_match_context_create_32(gctx);
	PCRE2_SPTR32 s = (PCRE2_SPTR32)p_subject.get_data();

	pcre2_match_data_32* match = pcre2_match_data_create_from_pattern_32(c, gctx);

	int res = pcre2_match_32(c, s, length, p_offset, 0, match, mctx);

	if (res < 0) {
		pcre2_match_data_free_32(match);
		pcre2_match_context_free_32(mctx);

		return nullptr;
	}

	uint32_t size = pcre2_get_ovector_count_32(match);
	PCRE2_SIZE* ovector = pcre2_get_ovector_pointer_32(match);

	result->data.resize(size);

	for (uint32_t i = 0; i < size; i++) {
		result->data.write[i].start = ovector[i * 2];
		result->data.write[i].end = ovector[i * 2 + 1];
	}

	pcre2_match_data_free_32(match);
	pcre2_match_context_free_32(mctx);

	result->subject = p_subject;

	uint32_t count;
	const char32_t* table;
	uint32_t entry_size;

	_pattern_info(PCRE2_INFO_NAMECOUNT, &count);
	_pattern_info(PCRE2_INFO_NAMETABLE, &table);
	_pattern_info(PCRE2_INFO_NAMEENTRYSIZE, &entry_size);

	for (uint32_t i = 0; i < count; i++) {
		char32_t id = table[i * entry_size];
		if (result->data[id].start == -1) {
			continue;
		}
		String name = &table[i * entry_size + 1];
		if (result->names.has(name)) {
			continue;
		}

		result->names.insert(name, id);
	}

	return result;
}

int RegEx::_sub(const String& p_subject, const String& p_replacement, int p_offset, int p_end,
	uint32_t p_flags, String& r_output) const
{
	// `safety_zone` is the number of chars we allocate in addition to the number of chars expected
	// in order to guard against the PCRE API writing one additional `\0` at the end. PCRE's API
	// docs are unclear on whether PCRE understands outlength in `pcre2_substitute(`) as counting an
	// implicit additional terminating char or not. Always allocating one char more than telling
	// PCRE has us on the safe side.
	const int safety_zone = 1;

	PCRE2_SIZE olength =
		p_subject.length() + 1; // Space for output string and one terminating `\0` character.
	Vector<char32_t> output;
	output.resize(olength + safety_zone);

	PCRE2_SIZE length = p_subject.length();
	if (p_end >= 0 && (uint32_t)p_end < length) {
		length = p_end;
	}

	pcre2_code_32* c = (pcre2_code_32*)code;
	pcre2_general_context_32* gctx = (pcre2_general_context_32*)general_ctx;
	pcre2_match_context_32* mctx = pcre2_match_context_create_32(gctx);
	PCRE2_SPTR32 s = (PCRE2_SPTR32)p_subject.get_data();
	PCRE2_SPTR32 r = (PCRE2_SPTR32)p_replacement.get_data();
	PCRE2_UCHAR32* o = (PCRE2_UCHAR32*)output.ptrw();

	pcre2_match_data_32* match = pcre2_match_data_create_from_pattern_32(c, gctx);

	int res = pcre2_substitute_32(
		c, s, length, p_offset, p_flags, match, mctx, r, p_replacement.length(), o, &olength);

	if (res == PCRE2_ERROR_NOMEMORY) {
		output.resize(olength + safety_zone);
		o = (PCRE2_UCHAR32*)output.ptrw();
		res = pcre2_substitute_32(
			c, s, length, p_offset, p_flags, match, mctx, r, p_replacement.length(), o, &olength);
	}

	pcre2_match_data_free_32(match);
	pcre2_match_context_free_32(mctx);

	if (res >= 0) {
		r_output = String::utf32(Span(output.ptr(), olength)) + p_subject.substr(length);
	}

	return res;
}

String RegEx::sub(
	const String& p_subject, const String& p_replacement, bool p_all, int p_offset, int p_end) const
{
	ERR_FAIL_COND_V(!is_valid(), String());
	ERR_FAIL_COND_V_MSG(p_offset < 0, String(), "RegEx sub offset must be >= 0");

	uint32_t flags = PCRE2_SUBSTITUTE_OVERFLOW_LENGTH | PCRE2_SUBSTITUTE_UNSET_EMPTY;
	if (p_all) {
		flags |= PCRE2_SUBSTITUTE_GLOBAL;
	}

	String output;
	const int res = _sub(p_subject, p_replacement,
p_offset, p_end, flags, output);

	if (res < 0) {
		PCRE2_UCHAR32 buf[256];
		pcre2_get_error_message_32(res, buf, 256);
		String message = "PCRE2 Error: " + String((const char32_t*)buf);
		ERR_PRINT(message);

		if (res == PCRE2_ERROR_NOSUBSTRING) {
			flags |= PCRE2_SUBSTITUTE_UNKNOWN_UNSET;
			_sub(p_subject, p_replacement, p_offset, p_end, flags, output);
		}
	}

	return output;
}

bool RegEx::is_valid() const { return (code != nullptr); }

String RegEx::get_pattern() const { return pattern; }

int RegEx::get_group_count() const
{
	ERR_FAIL_COND_V(!is_valid(), 0);

	uint32_t count;

	_pattern_info(PCRE2_INFO_CAPTURECOUNT, &count);

	return count;
}

PackedStringArray RegEx::get_names() const
{
	PackedStringArray result;

	ERR_FAIL_COND_V(!is_valid(), result);

	uint32_t count;
	const char32_t* table;
	uint32_t entry_size;

	_pattern_info(PCRE2_INFO_NAMECOUNT, &count);
	_pattern_info(PCRE2_INFO_NAMETABLE, &table);
	_pattern_info(PCRE2_INFO_NAMEENTRYSIZE, &entry_size);

	for (uint32_t i = 0; i < count; i++) {
		String name = &table[i * entry_size + 1];
		if (!result.has(name)) {
			result.append(name);
		}
	}

	return result;
}

RegEx::RegEx()
{
	general_ctx = pcre2_general_context_create_32(&_regex_malloc, &_regex_free, nullptr);
}

RegEx::RegEx(const String& p_pattern)
{
	general_ctx = pcre2_general_context_create_32(&_regex_malloc, &_regex_free, nullptr);
	compile(p_pattern);
}

RegEx::~RegEx()
{
	if (code) {
		pcre2_code_free_32((pcre2_code_32*)code);
	}
	pcre2_general_context_free_32((pcre2_general_context_32*)general_ctx);
}

#ifndef DISABLE_DEPRECATED


Ref<RegEx> RegEx::_create_from_string_bind_compat_95212(const String& p_pattern)
{
	return create_from_string(p_pattern, true);
}

Error RegEx::_compile_bind_compat_95212(const String& p_pattern)
{
	return compile(p_pattern, true);
}

void RegEx::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED

void initialize_regex_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}

void uninitialize_regex_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}
#endif // defined(MODULE_REGEX_ENABLED)


// --- Merged from modules/zip/zip_reader.cpp ---
#if defined(MODULE_ZIP_ENABLED)
#include "core/io/file_access_wrappers.h"
#include "main/main.h"

#include "core/error/error_macros.h"
#include "core/io/file_access.h"
#include "core/os/time.h"

// ZIPPacker

Error ZIPPacker::open(const String& p_path, ZipAppend p_append)
{
	if (fa.is_valid()) {
		close();
	}

	zlib_filefunc_def io = zipio_create_io(&fa);
	zf = zipOpen2(p_path.utf8().get_data(), p_append, nullptr, &io);
	return zf != nullptr ? OK : FAILED;
}

Error ZIPPacker::close()
{
	ERR_FAIL_COND_V_MSG(fa.is_null(), FAILED, "ZIPPacker cannot be closed because it is not open.");

	Error err = zipClose(zf, nullptr) == ZIP_OK ? OK : FAILED;
	if (err == OK) {
		DEV_ASSERT(fa.is_null());
		directories.clear();
		zf = nullptr;
	}

	return err;
}

void ZIPPacker::set_compression_level(int p_compression_level)
{
	ERR_FAIL_COND_MSG(
		p_compression_level < Z_DEFAULT_COMPRESSION || p_compression_level > Z_BEST_COMPRESSION,
		"Invalid compression level.");
	compression_level = p_compression_level;
}

int ZIPPacker::get_compression_level() const { return compression_level; }

Error ZIPPacker::write_file(const Vector<uint8_t>& p_data)
{
	ERR_FAIL_COND_V_MSG(fa.is_null(), FAILED, "ZIPPacker must be opened before use.");

	return zipWriteInFileInZip(zf, p_data.ptr(), p_data.size()) == ZIP_OK ? OK : FAILED;
}

Error ZIPPacker::close_file()
{
	ERR_FAIL_COND_V_MSG(fa.is_null(), FAILED, "ZIPPacker must be opened before use.");

	return zipCloseFileInZip(zf) == ZIP_OK ? OK : FAILED;
}

ZIPPacker::ZIPPacker() {}

ZIPPacker::~ZIPPacker()
{
	if (fa.is_valid()) {
		close();
	}
}

Error ZIPPacker::start_file(
	const String& p_path, uint32_t p_permissions, uint64_t p_modified_time)
{
	return OK;
}

// ZIPReader

Error ZIPReader::open(const String& p_path)
{
	if (fa.is_valid()) {
		close();
	}

	zlib_filefunc_def io = zipio_create_io(&fa);
	uzf = unzOpen2(p_path.utf8().get_data(), &io);
	return uzf != nullptr ? OK : FAILED;
}

Error ZIPReader::close()
{
	ERR_FAIL_COND_V_MSG(fa.is_null(), FAILED, "ZIPReader cannot be closed because it is not open.");

	Error err = unzClose(uzf) == UNZ_OK ? OK : FAILED;
	if (err == OK) {
		DEV_ASSERT(fa.is_null());
		uzf = nullptr;
	}

	return err;
}

PackedStringArray ZIPReader::get_files()
{
	ERR_FAIL_COND_V_MSG(fa.is_null(), PackedStringArray(), "ZIPReader must be opened before use.");

	unz_global_info gi;
	int err = unzGetGlobalInfo(uzf, &gi);
	ERR_FAIL_COND_V(err != UNZ_OK, PackedStringArray());
	if (gi.number_entry == 0) {
		return PackedStringArray();
	}

	err = unzGoToFirstFile(uzf);
	ERR_FAIL_COND_V(err != UNZ_OK, PackedStringArray());

	List<String> s;
	do {
		unz_file_info64 file_info;
		String filepath;

		err = godot_unzip_get_current_file_info(uzf, file_info, filepath);
		if (err == UNZ_OK) {
			s.push_back(filepath);
		}
	} while (unzGoToNextFile(uzf) == UNZ_OK);

	PackedStringArray arr;
	arr.resize(s.size());
	int idx = 0;
	for (const List<String>::Element* E = s.front(); E; E = E->next()) {
		arr.set(idx++, E->get());
	}
	return arr;
}

PackedByteArray ZIPReader::read_file(const String& p_path, bool p_case_sensitive)
{
	ERR_FAIL_COND_V_MSG(fa.is_null(), PackedByteArray(), "ZIPReader must be opened before use.");

	int err = UNZ_OK;

	// Locate and open the file.
	err = godot_unzip_locate_file(uzf, p_path, p_case_sensitive);
	ERR_FAIL_COND_V_MSG(
		err != UNZ_OK, PackedByteArray(), "File does not exist in zip archive: " + p_path);
	err = unzOpenCurrentFile(uzf);
	ERR_FAIL_COND_V_MSG(
		err != UNZ_OK, PackedByteArray(), "Could not open file within zip archive.");

	// Read the file info.
	unz_file_info info;
	err = unzGetCurrentFileInfo(uzf, &info, nullptr, 0, nullptr, 0, nullptr, 0);
	ERR_FAIL_COND_V_MSG(
		err != UNZ_OK, PackedByteArray(), "Unable to read file information from zip archive.");
	ERR_FAIL_COND_V_MSG(info.uncompressed_size > INT_MAX, PackedByteArray(),
		"File contents too large to read from zip archive (>2 GB).");

	// Read the file data.
	PackedByteArray data;
	data.resize(info.uncompressed_size);
	uint8_t* buffer = data.ptrw();
	int to_read = data.size();
	while (to_read > 0) {
		int bytes_read = unzReadCurrentFile(uzf, buffer, to_read);
		ERR_FAIL_COND_V_MSG(
			bytes_read < 0, PackedByteArray(), "IO/zlib error reading file from zip archive.");
		ERR_FAIL_COND_V_MSG(bytes_read == UNZ_EOF && to_read != 0, PackedByteArray(),
			"Incomplete file read from zip archive.");
		DEV_ASSERT(bytes_read <= to_read);
		buffer += bytes_read;
		to_read -= bytes_read;
	}

	// Verify the data and return.
	err = unzCloseCurrentFile(uzf);
	ERR_FAIL_COND_V_MSG(
		err != UNZ_OK, PackedByteArray(), "CRC error reading file from zip archive.");
	return data;
}

bool ZIPReader::file_exists(const String& p_path, bool p_case_sensitive)
{
	ERR_FAIL_COND_V_MSG(fa.is_null(), false, "ZIPReader must be opened before use.");

	int cs = p_case_sensitive ? 1 : 2;
	if (unzLocateFile(uzf, p_path.utf8().get_data(), cs) != UNZ_OK) {
		return false;
	}
	if (unzOpenCurrentFile(uzf) != UNZ_OK) {
		return false;
	}

	unzCloseCurrentFile(uzf);
	return true;
}

int ZIPReader::get_compression_level(const String& p_path, bool p_case_sensitive)
{
	ERR_FAIL_COND_V_MSG(fa.is_null(), -1, "ZIPReader must be opened before use.");

	int cs = p_case_sensitive ? 1 : 2;
	if (unzLocateFile(uzf, p_path.utf8().get_data(), cs) != UNZ_OK) {
		return -1;
	}

	int method;
	int level;
	if (unzOpenCurrentFile2(uzf, &method, &level, 1) != UNZ_OK) {
		return -1;
	}

	unzCloseCurrentFile(uzf);

	return level;
}

ZIPReader::ZIPReader() {}

ZIPReader::~ZIPReader()
{
	if (fa.is_valid()) {
		close();
	}
}

// Module registration

void initialize_zip_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}

void uninitialize_zip_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}
#endif // defined(MODULE_ZIP_ENABLED)

// === Merged from modules/text_server_adv/text_server_adv.cpp ===
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

#include "core/config/project_settings.h"
#include "core/io/config_file.h"
#include "main/main.h"
#include "core/error/error_macros.h"
#include "core/io/file_access.h"
#include "core/types.h"
#include "core/os/os.h"
#include "core/string/translation_server.h"
#include "modules/modules_enabled.gen.h" // For freetype, msdfgen, svg.
#include "scene/resources/image_texture.h"
// --- Inlined from modules/text_server_adv/text_server_adv.h ---
/*************************************************************************/
/* ICU/HarfBuzz/Graphite backed Text Server implementation with BiDi,    */
/* shaping and advanced font features support.                           */
/*************************************************************************/

#include "core/os/os.h"
#include "core/templates/hash_map.h"
#include "core/templates/rid_owner.h"
#include "core/templates/safe_refcount.h"
#include "modules/modules_enabled.gen.h" // For freetype, msdfgen, svg.
#include "scene/resources/image_texture.h"
#include "servers/text/text_server.h"

// Thirdparty headers.

VLTR_GCC_WARNING_PUSH_AND_IGNORE("-Wshadow")
#if defined(__EMSCRIPTEN__) || (defined(__MINGW32__) && __clang_major__ >= 21)
GODOT_CLANG_WARNING_PUSH_AND_IGNORE("-Wunnecessary-virtual-specifier")
#endif

#ifdef MODULE_MSDFGEN_ENABLED
#include "thirdparty/msdfgen/core/edge-selectors.h"
#include "thirdparty/msdfgen/msdfgen.h"
#endif

#include <unicode/ubidi.h>
#include <unicode/ubrk.h>
#include <unicode/uchar.h>
#include <unicode/uclean.h>
#include <unicode/udata.h>
#include <unicode/uiter.h>
#include <unicode/uloc.h>
#include <unicode/unorm2.h>
#include <unicode/uscript.h>
#include <unicode/uspoof.h>
#include <unicode/ustring.h>
#include <unicode/utypes.h>

VLTR_GCC_WARNING_POP
#if defined(__EMSCRIPTEN__) || (defined(__MINGW32__) && __clang_major__ >= 21)
GODOT_CLANG_WARNING_POP
#endif

#ifdef MODULE_FREETYPE_ENABLED
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H
#include FT_STROKER_H
#include FT_ADVANCES_H
#include FT_MULTIPLE_MASTERS_H
#include FT_BBOX_H
#include FT_SIZES_H
#include FT_MODULE_H
#include FT_CONFIG_OPTIONS_H
#if !defined(FT_CONFIG_OPTION_USE_BROTLI) && !defined(_MSC_VER)
#warning FreeType is configured without Brotli support, built-in fonts will not be available.
#endif
#include <hb-ft.h>
#include <hb-ot.h>
#endif

#include <hb-icu.h>
#include <hb.h>
#if HB_VERSION_ATLEAST(13, 0, 0)
#include <hb-raster.h>
#endif

class ScriptIterator final {
	static const int PAREN_STACK_DEPTH = 128;
	static const int EMOJI_STACK_DEPTH = 32;

public:
	struct ScriptRange {
		int start = 0;
		int end = 0;
		hb_script_t script = HB_SCRIPT_COMMON;
	};
	Vector<ScriptRange> script_ranges;

private:
	inline static bool same_script(int32_t p_script_one, int32_t p_script_two);
	inline static bool is_emoji(UChar32 p_c, UChar32 p_next);

public:
	ScriptIterator(const String &p_string, int p_start, int p_length);
};

class TextServerAdvanced final : public TextServer
{
	_THREAD_SAFE_CLASS_

	struct FeatureInfo
	{
		String name;
		bool hidden = false;
	};

	HashMap<String, int32_t> feature_sets;
	HashMap<int32_t, FeatureInfo> feature_sets_inv;

	enum LineBreakStrictness
	{
		LB_AUTO,
		LB_LOOSE,
		LB_NORMAL,
		LB_STRICT,
	};

	SafeNumeric<TextServer::FontLCDSubpixelLayout> lcd_subpixel_layout{
		TextServer::FontLCDSubpixelLayout::FONT_LCD_SUBPIXEL_LAYOUT_NONE};
	LineBreakStrictness lb_strictness = LB_AUTO;
	void _update_settings();

	void _insert_feature_sets();

	// ICU support data.

	static bool icu_data_loaded;
	static PackedByteArray icu_data;
	mutable USet* allowed = nullptr;
	mutable USpoofChecker* sc_spoof = nullptr;
	mutable USpoofChecker* sc_conf = nullptr;

	mutable HashMap<String, UBreakIterator*> line_break_iterators_per_language;

	UBreakIterator* _create_line_break_iterator_for_locale(
		const String& p_language, UErrorCode* r_err) const;

	// Font cache data.

#ifdef MODULE_FREETYPE_ENABLED
	mutable FT_Library ft_library = nullptr;
#endif

	const int rect_range = 1;

	struct FontTexturePosition
	{
		int32_t index = -1;
		int32_t x = 0;
		int32_t y = 0;

		FontTexturePosition() {}

		FontTexturePosition(int32_t p_id, int32_t p_x, int32_t p_y) : index(p_id), x(p_x), y(p_y) {}
	};

	struct Shelf
	{
		int32_t x = 0;
		int32_t y = 0;
		int32_t w = 0;
		int32_t h = 0;

		FontTexturePosition alloc_shelf(int32_t p_id, int32_t p_w, int32_t p_h)
		{
			if (p_w > w || p_h > h) {
				return FontTexturePosition(-1, 0, 0);
			}
			int32_t xx = x;
			x += p_w;
			w -= p_w;
			return FontTexturePosition(p_id, xx, y);
		}

		Shelf() {}

		Shelf(int32_t p_x, int32_t p_y, int32_t p_w, int32_t p_h) : x(p_x), y(p_y), w(p_w), h(p_h)
		{
		}
	};

	struct ShelfPackTexture
	{
		int32_t texture_w = 1024;
		int32_t texture_h = 1024;

		Ref<Image> image;
		Ref<ImageTexture> texture;
		bool dirty = true;

		List<Shelf> shelves;

		FontTexturePosition pack_rect(int32_t p_id, int32_t p_h, int32_t p_w)
		{
			int32_t y = 0;
			int32_t waste = 0;
			Shelf* best_shelf = nullptr;
			int32_t best_waste = std::numeric_limits<std::int32_t>::max();

			for (Shelf& E : shelves) {
				y += E.h;
				if (p_w > E.w) {
					continue;
				}
				if (p_h == E.h) {
					return E.alloc_shelf(p_id, p_w, p_h);
				}
				if (p_h > E.h) {
					continue;
				}
				if (p_h < E.h) {
					waste = (E.h - p_h) * p_w;
					if (waste < best_waste) {
						best_waste = waste;
						best_shelf = &E;
					}
				}
			}
			if (best_shelf) {
				return best_shelf->alloc_shelf(p_id, p_w, p_h);
			}
			if (p_h <= (texture_h - y) && p_w <= texture_w) {
				List<Shelf>::Element* E = shelves.push_back(Shelf(0, y, texture_w, p_h));
				return E->get().alloc_shelf(p_id, p_w, p_h);
			}
			return FontTexturePosition(-1, 0, 0);
		}

		ShelfPackTexture() {}

		ShelfPackTexture(int32_t p_w, int32_t p_h) : texture_w(p_w), texture_h(p_h) {}
	};

	struct FontGlyph
	{
		bool found = false;
		int texture_idx = -1;
		Rect2 rect;
		Rect2 uv_rect;
		Vector2 advance;
		bool fix_edge = false;
	};

	struct FontAdvanced;

	struct FontForSizeAdvanced
	{
		double ascent = 0.0;
		double descent = 0.0;
		double underline_position = 0.0;
		double underline_thickness = 0.0;
		double scale = 1.0;

		FontAdvanced* owner = nullptr;
		uint32_t viewport_oversampling = 0;

		Vector2i size;

		Vector<ShelfPackTexture> textures;
		HashMap<int64_t, int64_t> inv_glyph_map;
		HashMap<int32_t, FontGlyph> glyph_map;
		HashMap<Vector2i, Vector2> kerning_map;
		hb_font_t* hb_handle = nullptr;
#if HB_VERSION_ATLEAST(13, 0, 0)
		hb_face_t* hb_face = nullptr;
		bool color_paint = false;
#endif

#ifdef MODULE_FREETYPE_ENABLED
		FT_Size fsize = nullptr;
#endif

		~FontForSizeAdvanced()
		{
			if (hb_handle != nullptr) {
				hb_font_destroy(hb_handle);
			}
#ifdef MODULE_FREETYPE_ENABLED
			if (fsize != nullptr) {
				FT_Done_Size(fsize);
			}
#endif
		}
	};

	struct OversamplingLevel
	{
		HashSet<FontForSizeAdvanced*> fonts;
		int32_t refcount = 1;
	};

	mutable HashMap<uint32_t, OversamplingLevel> oversampling_levels;

	struct FontAdvancedLinkedVariation
	{
		RID base_font;
		int extra_spacing[4] = {0, 0, 0, 0};
		double baseline_offset = 0.0;
	};

	struct FontAdvanced
	{
		Mutex mutex;

		TextServer::FontAntialiasing antialiasing = TextServer::FONT_ANTIALIASING_GRAY;
		bool disable_embedded_bitmaps = true;
		bool mipmaps = false;
		bool msdf = false;
		int msdf_range = 14;
		FixedSizeScaleMode fixed_size_scale_mode = FIXED_SIZE_SCALE_DISABLE;
		int msdf_source_size = 48;
		int fixed_size = 0;
		bool allow_system_fallback = true;
		bool force_autohinter = false;
		bool modulate_color_glyphs = false;
		TextServer::Hinting hinting = TextServer::HINTING_LIGHT;
		TextServer::SubpixelPositioning subpixel_positioning =
			TextServer::SUBPIXEL_POSITIONING_AUTO;
		bool keep_rounding_remainders = true;
		double oversampling_override = 0.0;
		double embolden = 0.0;
		Transform2D transform;

		uint32_t style_flags = 0;
		String font_name;
		String style_name;
		int weight = 400;
		int stretch = 100;
		int extra_spacing[4] = {0, 0, 0, 0};
		double baseline_offset = 0.0;

		HashMap<Vector2i, FontForSizeAdvanced*> cache;

#if HB_VERSION_ATLEAST(13, 0, 0)
		Vector<String> palette_names;
		Vector<PackedColorArray> palette_colors;
		PackedColorArray palette_custom_colors;
		Vector<hb_color_t> palette_custom_colors_hb;
		unsigned int palette_index = 0;

		hb_raster_paint_t* hb_rdr = nullptr;
		hb_raster_draw_t* hb_mono = nullptr;
#endif

		bool face_init = false;
		HashSet<uint32_t> supported_scripts;

		// Language/script support override.
		HashMap<String, bool> language_support_overrides;
		HashMap<String, bool> script_support_overrides;

		PackedByteArray data;
		const uint8_t* data_ptr = nullptr;
		size_t data_size;
		int face_index = 0;

#ifdef MODULE_FREETYPE_ENABLED
		FT_Face face = nullptr;
		FT_StreamRec stream;
#endif

		~FontAdvanced()
		{
			for (const KeyValue<Vector2i, FontForSizeAdvanced*>& E : cache) {
				memdelete(E.value);
			}
			cache.clear();
#if HB_VERSION_ATLEAST(13, 0, 0)
			if (hb_rdr != nullptr) {
				hb_raster_paint_destroy(hb_rdr);
			}
			if (hb_mono != nullptr) {
				hb_raster_draw_destroy(hb_mono);
			}
#endif
#ifdef MODULE_FREETYPE_ENABLED
			if (face != nullptr) {
				FT_Done_Face(face);
			}
#endif
		}
	};

	_FORCE_INLINE_ FontTexturePosition find_texture_pos_for_glyph(FontForSizeAdvanced* p_data,
		int p_color_size, Image::Format p_image_format, int p_width, int p_height,
		bool p_msdf) const;
#ifdef MODULE_FREETYPE_ENABLED
	_FORCE_INLINE_ FontGlyph rasterize_bitmap(FontForSizeAdvanced* p_data, int p_rect_margin,
		FT_Bitmap p_bitmap, int p_yofs, int p_xofs, const Vector2& p_advance, bool p_bgra) const;
#if HB_VERSION_ATLEAST(13, 0, 0)
	_FORCE_INLINE_ FontGlyph rasterize_hb_bitmap(FontForSizeAdvanced* p_data, int p_rect_margin,
		hb_raster_image_t* p_image, const hb_raster_extents_t& p_ext, const Vector2& p_advance,
		bool p_bgra) const;
#endif
#endif
	bool _ensure_glyph(FontAdvanced* p_font_data, const Vector2i& p_size, int32_t p_glyph,
		FontGlyph& r_glyph, uint32_t p_oversampling = 0) const;
	bool _ensure_cache_for_size(FontAdvanced* p_font_data, const Vector2i& p_size,
		FontForSizeAdvanced*& r_cache_for_size, bool p_silent = false,
		uint32_t p_oversampling = 0) const;
	_FORCE_INLINE_ bool _font_validate(const RID& p_font_rid) const;
	static void _generateMTSDF_threaded(void* p_td, uint32_t p_y);

	_FORCE_INLINE_ Vector2i _get_size(const FontAdvanced* p_font_data, int p_size) const
	{
		if (p_font_data->msdf) {
			return Vector2i(p_font_data->msdf_source_size * 64, 0);
		}
		else if (p_font_data->fixed_size > 0) {
			return Vector2i(p_font_data->fixed_size * 64, 0);
		}
		else {
			return Vector2i(p_size * 64, 0);
		}
	}

	_FORCE_INLINE_ Vector2i _get_size_outline(
		const FontAdvanced* p_font_data, const Vector2i& p_size) const
	{
		if (p_font_data->msdf) {
			return Vector2i(p_font_data->msdf_source_size * 64, 0);
		}
		else if (p_font_data->fixed_size > 0) {
			return Vector2i(p_font_data->fixed_size * 64, MIN(p_size.y, 1));
		}
		else {
			return Vector2i(p_size.x * 64, p_size.y);
		}
	}

	_FORCE_INLINE_ double _get_extra_advance(RID p_font_rid, int p_font_size) const;
	_FORCE_INLINE_ bool _get_tag_hidden(int64_t p_tag) const;

	_FORCE_INLINE_ int _font_get_weight_by_name(const String& p_sty_name) const
	{
		String sty_name = p_sty_name.remove_chars(" -");
		if (sty_name.contains("thin") || sty_name.contains("hairline")) {
			return 100;
		}
		else if (sty_name.contains("extralight") || sty_name.contains("ultralight")) {
			return 200;
		}
		else if (sty_name.contains("light")) {
			return 300;
		}
		else if (sty_name.contains("semilight")) {
			return 350;
		}
		else if (sty_name.contains("regular")) {
			return 400;
		}
		else if (sty_name.contains("medium")) {
			return 500;
		}
		else if (sty_name.contains("semibold") || sty_name.contains("demibold")) {
			return 600;
		}
		else if (sty_name.contains("bold")) {
			return 700;
		}
		else if (sty_name.contains("extrabold") || sty_name.contains("ultrabold")) {
			return 800;
		}
		else if (sty_name.contains("black") || sty_name.contains("heavy")) {
			return 900;
		}
		else if (sty_name.contains("extrablack") || sty_name.contains("ultrablack")) {
			return 950;
		}
		return 400;
	}

	_FORCE_INLINE_ int _font_get_stretch_by_name(const String& p_sty_name) const
	{
		String sty_name = p_sty_name.remove_chars(" -");
		if (sty_name.contains("ultracondensed")) {
			return 50;
		}
		else if (sty_name.contains("extracondensed")) {
			return 63;
		}
		else if (sty_name.contains("condensed")) {
			return 75;
		}
		else if (sty_name.contains("semicondensed")) {
			return 87;
		}
		else if (sty_name.contains("semiexpanded")) {
			return 113;
		}
		else if (sty_name.contains("expanded")) {
			return 125;
		}
		else if (sty_name.contains("extraexpanded")) {
			return 150;
		}
		else if (sty_name.contains("ultraexpanded")) {
			return 200;
		}
		return 100;
	}

	_FORCE_INLINE_ bool _is_ital_style(const String& p_sty_name) const
	{
		return p_sty_name.contains("italic") || p_sty_name.contains("oblique");
	}

	// Shaped text cache data.
	struct TrimData
	{
		int trim_pos = -1;
		int ellipsis_pos = -1;
		Vector<Glyph> ellipsis_glyph_buf;
	};

	struct TextRun
	{
		Vector2i range;
		Vector2i gl_range;
		RID font_rid;
		int font_size = 0;
		bool rtl = false;
		int64_t span_index = -1;
	};

	struct ShapedTextDataAdvanced
	{
		Mutex mutex;

		/* Source data */
		RID parent; // Substring parent ShapedTextData.

		int start = 0; // Substring start offset in the parent string.
		int end = 0;   // Substring end offset in the parent string.

		String text;
		String custom_punct;
		TextServer::Direction direction = DIRECTION_LTR; // Desired text direction.
		TextServer::Orientation orientation = ORIENTATION_HORIZONTAL;

		struct Span
		{
			int start = -1;
			int end = -1;

			int font_size = 0;

			String language;
		};

		Vector<Span> spans;
		int first_span = 0; // First span in the parent ShapedTextData.
		int last_span = 0;

		Vector<TextRun> runs;
		bool runs_dirty = true;

		struct EmbeddedObject
		{
			int start = -1;
			int end = -1;
			InlineAlignment inline_align = INLINE_ALIGNMENT_CENTER;
			Rect2 rect;
			double baseline = 0;
		};

		/* Shaped data */
		TextServer::Direction para_direction = DIRECTION_LTR; // Detected text direction.
		int base_para_direction = UBIDI_DEFAULT_LTR;
		SafeFlag valid{false};			// String is shaped.
		bool line_breaks_valid = false; // Line and word break flags are populated (and virtual zero
										// width spaces inserted).
		bool justification_ops_valid = false; // Virtual elongation glyphs are added to the string.
		bool sort_valid = false;
		bool text_trimmed = false;

		bool preserve_invalid = true;  // Draw hex code box instead of missing characters.
		bool preserve_control = false; // Draw control characters.

		double ascent = 0.0;  // Ascent for horizontal layout, 1/2 of width for vertical.
		double descent = 0.0; // Descent for horizontal layout, 1/2 of width for vertical.
		double width = 0.0;	  // Width for horizontal layout, height for vertical.
		double width_trimmed = 0.0;
		int extra_spacing[4] = {0, 0, 0, 0};

		double upos = 0.0;
		double uthk = 0.0;

		char32_t el_char = 0x2026;
		TrimData overrun_trim_data;
		bool fit_width_minimum_reached = false;

		LocalVector<Glyph> glyphs;
		LocalVector<Glyph> glyphs_logical;

		/* Intermediate data */
		Char16String utf16;
		Vector<UBiDi*> bidi_iter;
		Vector<Vector3i> bidi_override;
		ScriptIterator* script_iter = nullptr;
		hb_buffer_t* hb_buffer = nullptr;

		HashMap<int, bool> jstops;
		HashMap<int, bool> breaks;
		PackedInt32Array chars;
		int break_inserts = 0;
		bool break_ops_valid = false;
		bool js_ops_valid = false;
		bool chars_valid = false;

		~ShapedTextDataAdvanced()
		{
			for (int i = 0; i < bidi_iter.size(); i++) {
				if (bidi_iter[i]) {
					ubidi_close(bidi_iter[i]);
				}
			}
			memdelete(script_iter);
			if (hb_buffer) {
				hb_buffer_destroy(hb_buffer);
			}
		}
	};

	// Common data.

	mutable RID_PtrOwner<FontAdvancedLinkedVariation> font_var_owner;
	mutable RID_PtrOwner<FontAdvanced> font_owner;
	mutable RID_PtrOwner<ShapedTextDataAdvanced> shaped_owner{65536, 1048576};

	_FORCE_INLINE_ FontAdvanced* _get_font_data(const RID& p_font_rid) const
	{
		RID rid = p_font_rid;
		FontAdvancedLinkedVariation* fdv = font_var_owner.get_or_null(rid);
		if (unlikely(fdv)) {
			rid = fdv->base_font;
		}
		return font_owner.get_or_null(rid);
	}

	struct SystemFontKey
	{
		String font_name;
		TextServer::FontAntialiasing antialiasing = TextServer::FONT_ANTIALIASING_GRAY;
		bool disable_embedded_bitmaps = true;
		bool italic = false;
		bool mipmaps = false;
		bool msdf = false;
		bool force_autohinter = false;
		int weight = 400;
		int stretch = 100;
		int msdf_range = 14;
		int msdf_source_size = 48;
		int fixed_size = 0;
		TextServer::Hinting hinting = TextServer::HINTING_LIGHT;
		TextServer::SubpixelPositioning subpixel_positioning =
			TextServer::SUBPIXEL_POSITIONING_AUTO;
		bool keep_rounding_remainders = true;
		double embolden = 0.0;
		Transform2D transform;
		int extra_spacing[4] = {0, 0, 0, 0};
		double baseline_offset = 0.0;

		bool operator==(const SystemFontKey& p_b) const
		{
			return (font_name == p_b.font_name) && (antialiasing == p_b.antialiasing) &&
				   (italic == p_b.italic) &&
				   (disable_embedded_bitmaps == p_b.disable_embedded_bitmaps) &&
				   (mipmaps == p_b.mipmaps) && (msdf == p_b.msdf) &&
				   (force_autohinter == p_b.force_autohinter) && (weight == p_b.weight) &&
				   (stretch == p_b.stretch) && (msdf_range == p_b.msdf_range) &&
				   (msdf_source_size == p_b.msdf_source_size) && (fixed_size == p_b.fixed_size) &&
				   (hinting == p_b.hinting) && (subpixel_positioning == p_b.subpixel_positioning) &&
				   (keep_rounding_remainders == p_b.keep_rounding_remainders) &&
				   (embolden == p_b.embolden) && (transform == p_b.transform) &&
				   (extra_spacing[SPACING_TOP] == p_b.extra_spacing[SPACING_TOP]) &&
				   (extra_spacing[SPACING_BOTTOM] == p_b.extra_spacing[SPACING_BOTTOM]) &&
				   (extra_spacing[SPACING_SPACE] == p_b.extra_spacing[SPACING_SPACE]) &&
				   (extra_spacing[SPACING_GLYPH] == p_b.extra_spacing[SPACING_GLYPH]) &&
				   (baseline_offset == p_b.baseline_offset);
		}

		SystemFontKey(const String& p_font_name, bool p_italic, int p_weight, int p_stretch,
			RID p_font, const TextServerAdvanced* p_fb)
		{
			font_name = p_font_name;
			italic = p_italic;
			weight = p_weight;
			stretch = p_stretch;
		}
	};

	struct SystemFontCacheRec
	{
		RID rid;
		int index = 0;
	};

	struct SystemFontCache
	{
		Vector<SystemFontCacheRec> var;
		int max_var = 0;
	};

	struct SystemFontKeyHasher
	{
		_FORCE_INLINE_ static uint32_t hash(const SystemFontKey& p_a)
		{
			uint32_t hash = p_a.font_name.hash();
			hash = hash_murmur3_one_32(p_a.weight, hash);
			hash = hash_murmur3_one_32(p_a.stretch, hash);
			hash = hash_murmur3_one_32(p_a.msdf_range, hash);
			hash = hash_murmur3_one_32(p_a.msdf_source_size, hash);
			hash = hash_murmur3_one_32(p_a.fixed_size, hash);
			hash = hash_murmur3_one_double(p_a.embolden, hash);
			hash = hash_murmur3_one_real(p_a.transform[0].x, hash);
			hash = hash_murmur3_one_real(p_a.transform[0].y, hash);
			hash = hash_murmur3_one_real(p_a.transform[1].x, hash);
			hash = hash_murmur3_one_real(p_a.transform[1].y, hash);
			hash = hash_murmur3_one_32(p_a.extra_spacing[SPACING_TOP], hash);
			hash = hash_murmur3_one_32(p_a.extra_spacing[SPACING_BOTTOM], hash);
			hash = hash_murmur3_one_32(p_a.extra_spacing[SPACING_SPACE], hash);
			hash = hash_murmur3_one_32(p_a.extra_spacing[SPACING_GLYPH], hash);
			hash = hash_murmur3_one_double(p_a.baseline_offset, hash);
			return hash_fmix32(hash_murmur3_one_32(
				((int)p_a.mipmaps) | ((int)p_a.msdf << 1) | ((int)p_a.italic << 2) |
					((int)p_a.force_autohinter << 3) | ((int)p_a.hinting << 4) |
					((int)p_a.subpixel_positioning << 8) | ((int)p_a.antialiasing << 12) |
					((int)p_a.disable_embedded_bitmaps << 14) |
					((int)p_a.keep_rounding_remainders << 15),
				hash));
		}
	};

	mutable HashMap<SystemFontKey, SystemFontCache, SystemFontKeyHasher> system_fonts;
	mutable HashMap<String, PackedByteArray> system_font_data;

	void _update_chars(ShapedTextDataAdvanced* p_sd) const;
	void _generate_runs(ShapedTextDataAdvanced* p_sd) const;
	void _realign(ShapedTextDataAdvanced* p_sd) const;
	int64_t _convert_pos(const String& p_utf32, const Char16String& p_utf16, int64_t p_pos) const;
	int64_t _convert_pos(const ShapedTextDataAdvanced* p_sd, int64_t p_pos) const;
	int64_t _convert_pos_inv(const ShapedTextDataAdvanced* p_sd, int64_t p_pos) const;
	bool _shape_substr(ShapedTextDataAdvanced* p_new_sd, const ShapedTextDataAdvanced* p_sd,
		int64_t p_start, int64_t p_length) const;

	struct FontPriorityList
	{
		friend class TextServerAdvanced;

		const int PRIORITY_SKIP = 100; // Font already used.
		const int PRIORITY_MAX = 2;
		int current_priority = 0;
		uint32_t current_index = 0;
		uint32_t font_count = 0;
		String language;
		String script_code;
		bool color = false;
		LocalVector<Pair<RID, int>> unprocessed_fonts;
		LocalVector<RID> fonts;
		const TextServerAdvanced* text_server;

		_FORCE_INLINE_ uint32_t size() const { return font_count; }

		RID operator[](uint32_t p_index)
		{
			if (p_index < fonts.size()) {
				return fonts[p_index];
			}
			while (current_priority < PRIORITY_MAX || current_index < font_count) {
				if (current_index >= font_count) {
					current_priority++;
					current_index = 0;
				}
				const RID& font = unprocessed_fonts[current_index].first;
				int& priority = unprocessed_fonts[current_index].second;
				if (priority < 0) {
					priority = 0;
				}
				if (priority == current_priority) {
					unprocessed_fonts[current_index].second = PRIORITY_SKIP;
					fonts.push_back(font);
					if (p_index < fonts.size()) {
						return fonts[p_index];
					}
				}
				current_index++;
			}
			return RID();
		}
	};

	void _shape_run(ShapedTextDataAdvanced* p_sd, int64_t p_start, int64_t p_end,
		const String& p_language, hb_script_t p_script, hb_direction_t p_direction,
		FontPriorityList& p_fonts, int64_t p_span, int64_t p_fb_index, int64_t p_prev_start,
		int64_t p_prev_end, RID p_prev_font);
	Glyph _shape_single_glyph(ShapedTextDataAdvanced* p_sd, char32_t p_char, hb_script_t p_script,
		hb_direction_t p_direction, const RID& p_font, int64_t p_font_size);
	_FORCE_INLINE_ RID _find_sys_font_for_text(const RID& p_fdef, const String& p_script_code,
		const String& p_language, const String& p_text);

	String os_locale;

	Mutex ft_mutex;

	// HarfBuzz bitmap font interface.

	static hb_font_funcs_t* funcs;

	struct bmp_font_t
	{
		TextServerAdvanced::FontForSizeAdvanced* face = nullptr;
		bool unref = false; /* Whether to destroy bm_face when done. */
	};

	static bmp_font_t* _bmp_font_create(
		TextServerAdvanced::FontForSizeAdvanced* p_face, bool p_unref);
	static void _bmp_font_destroy(void* p_data);
	static hb_bool_t _bmp_get_nominal_glyph(hb_font_t* p_font, void* p_font_data,
		hb_codepoint_t p_unicode, hb_codepoint_t* r_glyph, void* p_user_data);
	static hb_position_t _bmp_get_glyph_h_advance(
		hb_font_t* p_font, void* p_font_data, hb_codepoint_t p_glyph, void* p_user_data);
	static hb_position_t _bmp_get_glyph_v_advance(
		hb_font_t* p_font, void* p_font_data, hb_codepoint_t p_glyph, void* p_user_data);
	static hb_position_t _bmp_get_glyph_h_kerning(hb_font_t* p_font, void* p_font_data,
		hb_codepoint_t p_left_glyph, hb_codepoint_t p_right_glyph, void* p_user_data);
	static hb_bool_t _bmp_get_glyph_v_origin(hb_font_t* p_font, void* p_font_data,
		hb_codepoint_t p_glyph, hb_position_t* r_x, hb_position_t* r_y, void* p_user_data);
	static hb_bool_t _bmp_get_glyph_extents(hb_font_t* p_font, void* p_font_data,
		hb_codepoint_t p_glyph, hb_glyph_extents_t* r_extents, void* p_user_data);
	static hb_bool_t _bmp_get_font_h_extents(
		hb_font_t* p_font, void* p_font_data, hb_font_extents_t* r_metrics, void* p_user_data);
	static void _bmp_create_font_funcs();
	static void _bmp_free_font_funcs();
	static void _bmp_font_set_funcs(
		hb_font_t* p_font, TextServerAdvanced::FontForSizeAdvanced* p_face, bool p_unref);
	static hb_font_t* _bmp_font_create(
		TextServerAdvanced::FontForSizeAdvanced* p_face, hb_destroy_func_t p_destroy);

	hb_font_t* _font_get_hb_handle(const RID& p_font, int64_t p_font_size, bool& r_is_color) const;
	bool _font_is_color(const RID& p_font) const;

	struct GlyphCompare
	{ // For line breaking reordering.

		_FORCE_INLINE_ bool operator()(const Glyph& l, const Glyph& r) const
		{
			if (l.start == r.start) {
				if (l.count == r.count) {
					return (l.flags & TextServer::GRAPHEME_IS_VIRTUAL) <
						   (r.flags & TextServer::GRAPHEME_IS_VIRTUAL);
				}
				return l.count > r.count; // Sort first glyph with count & flags, order of the rest
										  // are irrelevant.
			}
			else {
				return l.start < r.start;
			}
		}
	};

protected:
	void full_copy(ShapedTextDataAdvanced* p_shaped);
	void invalidate(ShapedTextDataAdvanced* p_shaped, bool p_text = false);

public:
	TextServerAdvanced() = default;
	bool _has_feature(Feature p_feature) const;
	String _get_name() const;
	String _get_short_name() const;
	int64_t _get_features() const;
	void _free_rid(const RID& p_rid);
	bool _has(const RID& p_rid);
	bool _load_support_data(const String& p_filename);
	String _get_support_data_filename() const;
	String _get_support_data_info() const;
	bool _save_support_data(const String& p_filename) const;
	PackedByteArray _get_support_data() const;
	bool _is_locale_using_support_data(const String& p_locale) const;
	bool _is_locale_right_to_left(const String& p_locale) const;
	int64_t _name_to_tag(const String& p_name) const;
	String _tag_to_name(int64_t p_tag) const;

#ifdef MODULE_MSDFGEN_ENABLED

	struct MSContext
	{
		msdfgen::Point2 position;
		msdfgen::Shape* shape = nullptr;
		msdfgen::Contour* contour = nullptr;
	};

	class DistancePixelConversion
	{
		double invRange;

	public:
		_FORCE_INLINE_ explicit DistancePixelConversion(double range) : invRange(1 / range) {}

		_FORCE_INLINE_ void operator()(
			float* pixels, const msdfgen::MultiAndTrueDistance& distance) const
		{
			pixels[0] = float(invRange * distance.r + .5);
			pixels[1] = float(invRange * distance.g + .5);
			pixels[2] = float(invRange * distance.b + .5);
			pixels[3] = float(invRange * distance.a + .5);
		}
	};

	struct MSDFThreadData
	{
		msdfgen::Bitmap<float, 4>* output;
		msdfgen::Shape* shape;
		msdfgen::Projection* projection;
		DistancePixelConversion* distancePixelConversion;
	};

	static msdfgen::Point2 ft_point2(const FT_Vector& vector);
	static int ft_move_to(const FT_Vector* to, void* user);
	static int ft_line_to(const FT_Vector* to, void* user);
	static int ft_conic_to(const FT_Vector* control, const FT_Vector* to, void* user);
	static int ft_cubic_to(
		const FT_Vector* control1, const FT_Vector* control2, const FT_Vector* to, void* user);

#endif

	/*************************************************************************/
	/* Font Cache                                                            */
	/*************************************************************************/

	void _reference_oversampling_level(double p_oversampling);
	void _unreference_oversampling_level(double p_oversampling);
	RID _create_font();
	RID _create_font_linked_variation(const RID& p_font_rid);
	void _font_set_data(const RID& p_font_rid, const PackedByteArray& p_data);
	void _font_set_data_ptr(const RID& p_font_rid, const uint8_t* p_data_ptr, int64_t p_data_size);
	void _font_set_face_index(const RID& p_font_rid, int64_t p_face_index);
	int64_t _font_get_face_index(const RID& p_font_rid) const;
	int64_t _font_get_face_count(const RID& p_font_rid) const;
	void _font_set_style(const RID& p_font_rid, uint32_t p_style);
	uint32_t _font_get_style(const RID& p_font_rid) const;
	void _font_set_style_name(const RID& p_font_rid, const String& p_name);
	String _font_get_style_name(const RID& p_font_rid) const;
	void _font_set_weight(const RID& p_font_rid, int64_t p_weight);
	int64_t _font_get_weight(const RID& p_font_rid) const;
	void _font_set_stretch(const RID& p_font_rid, int64_t p_stretch);
	int64_t _font_get_stretch(const RID& p_font_rid) const;
	void _font_set_name(const RID& p_font_rid, const String& p_name);
	void _font_set_antialiasing(const RID& p_font_rid, TextServer::FontAntialiasing p_antialiasing);
	TextServer::FontAntialiasing _font_get_antialiasing(const RID& p_font_rid) const;
	void _font_set_disable_embedded_bitmaps(const RID& p_font_rid, bool p_disable_embedded_bitmaps);
	bool _font_get_disable_embedded_bitmaps(const RID& p_font_rid) const;
	void _font_set_generate_mipmaps(const RID& p_font_rid, bool p_generate_mipmaps);
	bool _font_get_generate_mipmaps(const RID& p_font_rid) const;
	void _font_set_multichannel_signed_distance_field(const RID& p_font_rid, bool p_msdf);
	bool _font_is_multichannel_signed_distance_field(const RID& p_font_rid) const;
	void _font_set_msdf_pixel_range(const RID& p_font_rid, int64_t p_msdf_pixel_range);
	int64_t _font_get_msdf_pixel_range(const RID& p_font_rid) const;
	void _font_set_msdf_size(const RID& p_font_rid, int64_t p_msdf_size);
	int64_t _font_get_msdf_size(const RID& p_font_rid) const;
	void _font_set_fixed_size(const RID& p_font_rid, int64_t p_fixed_size);
	int64_t _font_get_fixed_size(const RID& p_font_rid) const;
	void _font_set_fixed_size_scale_mode(
		const RID& p_font_rid, TextServer::FixedSizeScaleMode p_fixed_size_scale_mode);
	TextServer::FixedSizeScaleMode _font_get_fixed_size_scale_mode(const RID& p_font_rid) const;
	void _font_set_allow_system_fallback(const RID& p_font_rid, bool p_allow_system_fallback);
	bool _font_is_allow_system_fallback(const RID& p_font_rid) const;
	void _font_set_force_autohinter(const RID& p_font_rid, bool p_force_autohinter);
	bool _font_is_force_autohinter(const RID& p_font_rid) const;
	void _font_set_modulate_color_glyphs(const RID& p_font_rid, bool p_modulate);
	bool _font_is_modulate_color_glyphs(const RID& p_font_rid) const;
	int64_t _font_get_palette_count(const RID& p_font_rid) const;
	String _font_get_palette_name(const RID& p_font_rid, int64_t p_index) const;
	Vector<Color> _font_get_palette_colors(const RID& p_font_rid, int64_t p_index) const;
	void _font_set_palette_custom_colors(const RID& p_font_rid, const Vector<Color>& p_colors);
	Vector<Color> _font_get_palette_custom_colors(const RID& p_font_rid) const;
	int64_t _font_get_used_palette(const RID& p_font_rid) const;
	void _font_set_used_palette(const RID& p_font_rid, int64_t p_index);
	void _font_set_hinting(const RID& p_font_rid, TextServer::Hinting p_hinting);
	TextServer::Hinting _font_get_hinting(const RID& p_font_rid) const;
	void _font_set_subpixel_positioning(
		const RID& p_font_rid, TextServer::SubpixelPositioning p_subpixel);
	TextServer::SubpixelPositioning _font_get_subpixel_positioning(const RID& p_font_rid) const;
	void _font_set_keep_rounding_remainders(const RID& p_font_rid, bool p_keep_rounding_remainders);
	bool _font_get_keep_rounding_remainders(const RID& p_font_rid) const;
	void _font_set_embolden(const RID& p_font_rid, double p_strength);
	double _font_get_embolden(const RID& p_font_rid) const;
	void _font_set_spacing(const RID& p_font_rid, SpacingType p_spacing, int64_t p_value);
	int64_t _font_get_spacing(const RID& p_font_rid, SpacingType p_spacing) const;
	void _font_set_baseline_offset(const RID& p_font_rid, double p_baseline_offset);
	double _font_get_baseline_offset(const RID& p_font_rid) const;
	void font_set_transform(const RID& p_font_rid, const Transform2D& p_transform) override;
	Transform2D _font_get_transform(const RID& p_font_rid) const;
	double _font_get_oversampling(const RID& p_font_rid) const;
	void _font_set_oversampling(const RID& p_font_rid, double p_oversampling);
	TypedArray<Vector2i> _font_get_size_cache_list(const RID& p_font_rid) const;
	void _font_clear_size_cache(const RID& p_font_rid);
	void _font_remove_size_cache(const RID& p_font_rid, const Vector2i& p_size);
	void _font_set_ascent(const RID& p_font_rid, int64_t p_size, double p_ascent);
	double _font_get_ascent(const RID& p_font_rid, int64_t p_size) const;
	void _font_set_descent(const RID& p_font_rid, int64_t p_size, double p_descent);
	double _font_get_descent(const RID& p_font_rid, int64_t p_size) const;
	void _font_set_underline_position(
		const RID& p_font_rid, int64_t p_size, double p_underline_position);
	double _font_get_underline_position(const RID& p_font_rid, int64_t p_size) const;
	void _font_set_underline_thickness(
		const RID& p_font_rid, int64_t p_size, double p_underline_thickness);
	double _font_get_underline_thickness(const RID& p_font_rid, int64_t p_size) const;
	void _font_set_scale(const RID& p_font_rid, int64_t p_size, double p_scale);
	double _font_get_scale(const RID& p_font_rid, int64_t p_size) const;
	int64_t _font_get_texture_count(const RID& p_font_rid, const Vector2i& p_size) const;
	void _font_clear_textures(const RID& p_font_rid, const Vector2i& p_size);
	void _font_remove_texture(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_texture_index);
	void _font_set_texture_image(const RID& p_font_rid, const Vector2i& p_size,
		int64_t p_texture_index, const Ref<Image>& p_image);
	Ref<Image> _font_get_texture_image(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_texture_index) const;
	void _font_set_texture_offsets(const RID& p_font_rid, const Vector2i& p_size,
		int64_t p_texture_index, const PackedInt32Array& p_offsets);
	PackedInt32Array _font_get_texture_offsets(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_texture_index) const;
	PackedInt32Array _font_get_glyph_list(const RID& p_font_rid, const Vector2i& p_size) const;
	void _font_clear_glyphs(const RID& p_font_rid, const Vector2i& p_size);
	void _font_remove_glyph(const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph);
	Vector2 _font_get_glyph_advance(const RID& p_font_rid, int64_t p_size, int64_t p_glyph) const;
	void _font_set_glyph_advance(
		const RID& p_font_rid, int64_t p_size, int64_t p_glyph, const Vector2& p_advance);
	Vector2 _font_get_glyph_offset(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const;
	void _font_set_glyph_offset(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph, const Vector2& p_offset);
	Vector2 _font_get_glyph_size(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const;
	void _font_set_glyph_size(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph, const Vector2& p_gl_size);
	Rect2 _font_get_glyph_uv_rect(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const;
	void _font_set_glyph_uv_rect(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph, const Rect2& p_uv_rect);
	int64_t _font_get_glyph_texture_idx(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const;
	void _font_set_glyph_texture_idx(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph, int64_t p_texture_idx);
	RID _font_get_glyph_texture_rid(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const;
	Size2 _font_get_glyph_texture_size(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const;
	TypedArray<Vector2i> _font_get_kerning_list(const RID& p_font_rid, int64_t p_size) const;
	void _font_clear_kerning_map(const RID& p_font_rid, int64_t p_size);
	void _font_remove_kerning(const RID& p_font_rid, int64_t p_size, const Vector2i& p_glyph_pair);
	void _font_set_kerning(const RID& p_font_rid, int64_t p_size, const Vector2i& p_glyph_pair,
		const Vector2& p_kerning);
	Vector2 _font_get_kerning(
		const RID& p_font_rid, int64_t p_size, const Vector2i& p_glyph_pair) const;
	int64_t _font_get_glyph_index(
		const RID& p_font_rid, int64_t p_size, int64_t p_char, int64_t p_variation_selector) const;
	int64_t _font_get_char_from_glyph_index(
		const RID& p_font_rid, int64_t p_size, int64_t p_glyph_index) const;
	bool _font_has_char(const RID& p_font_rid, int64_t p_char) const;
	String _font_get_supported_chars(const RID& p_font_rid) const;
	PackedInt32Array _font_get_supported_glyphs(const RID& p_font_rid) const;
	void _font_render_range(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_start, int64_t p_end);
	void _font_render_glyph(const RID& p_font_rid, const Vector2i& p_size, int64_t p_index);
	void _font_draw_glyph(const RID& p_font_rid, const RID& p_canvas, int64_t p_size,
		const Vector2& p_pos, int64_t p_index, const Color& p_color, float p_oversampling) const;
	void _font_draw_glyph_outline(const RID& p_font_rid, const RID& p_canvas, int64_t p_size,
		int64_t p_outline_size, const Vector2& p_pos, int64_t p_index, const Color& p_color,
		float p_oversampling) const;
	bool _font_is_language_supported(const RID& p_font_rid, const String& p_language) const;
	void _font_set_language_support_override(
		const RID& p_font_rid, const String& p_language, bool p_supported);
	bool _font_get_language_support_override(const RID& p_font_rid, const String& p_language);
	void _font_remove_language_support_override(const RID& p_font_rid, const String& p_language);
	PackedStringArray _font_get_language_support_overrides(const RID& p_font_rid);
	bool _font_is_script_supported(const RID& p_font_rid, const String& p_script) const;
	void _font_set_script_support_override(
		const RID& p_font_rid, const String& p_script, bool p_supported);
	bool _font_get_script_support_override(const RID& p_font_rid, const String& p_script);
	void _font_remove_script_support_override(const RID& p_font_rid, const String& p_script);
	PackedStringArray _font_get_script_support_overrides(const RID& p_font_rid);
	RID _create_shaped_text(
		TextServer::Direction p_direction, TextServer::Orientation p_orientation);
	void _shaped_text_clear(const RID& p_shaped);
	RID _shaped_text_duplicate(const RID& p_shaped);
	void _shaped_text_set_direction(const RID& p_shaped, TextServer::Direction p_direction);
	TextServer::Direction _shaped_text_get_direction(const RID& p_shaped) const;
	TextServer::Direction _shaped_text_get_inferred_direction(const RID& p_shaped) const;
	void _shaped_text_set_custom_punctuation(const RID& p_shaped, const String& p_punct);
	String _shaped_text_get_custom_punctuation(const RID& p_shaped) const;
	void _shaped_text_set_custom_ellipsis(const RID& p_shaped, int64_t p_char);
	int64_t _shaped_text_get_custom_ellipsis(const RID& p_shaped) const;
	void _shaped_text_set_orientation(const RID& p_shaped, TextServer::Orientation p_orientation);
	void _shaped_text_set_preserve_invalid(const RID& p_shaped, bool p_enabled);
	bool _shaped_text_get_preserve_invalid(const RID& p_shaped) const;
	void _shaped_text_set_preserve_control(const RID& p_shaped, bool p_enabled);
	bool _shaped_text_get_preserve_control(const RID& p_shaped) const;
	void _shaped_text_set_spacing(const RID& p_shaped, SpacingType p_spacing, int64_t p_value);
	int64_t _shaped_text_get_spacing(const RID& p_shaped, SpacingType p_spacing) const;
	TextServer::Orientation _shaped_text_get_orientation(const RID& p_shaped) const;
	int64_t _shaped_get_span_count(const RID& p_shaped) const;
	String _shaped_get_span_text(const RID& p_shaped, int64_t p_index) const;
	int64_t _shaped_get_run_count(const RID& p_shaped) const;
	String _shaped_get_run_text(const RID& p_shaped, int64_t p_index) const;
	Vector2i _shaped_get_run_range(const RID& p_shaped, int64_t p_index) const;
	Vector2i _shaped_get_run_glyph_range(const RID& p_shaped, int64_t p_index) const;
	RID _shaped_get_run_font_rid(const RID& p_shaped, int64_t p_index) const;
	int _shaped_get_run_font_size(const RID& p_shaped, int64_t p_index) const;
	String _shaped_get_run_language(const RID& p_shaped, int64_t p_index) const;
	TextServer::Direction _shaped_get_run_direction(const RID& p_shaped, int64_t p_index) const;
	String _shaped_get_text(const RID& p_shaped) const;
	RID _shaped_text_substr(const RID& p_shaped, int64_t p_start, int64_t p_length) const;
	RID _shaped_text_get_parent(const RID& p_shaped) const;
	double _shaped_text_fit_to_width(const RID& p_shaped, double p_width, uint32_t p_jst_flags);
	double _shaped_text_tab_align(const RID& p_shaped, const PackedFloat32Array& p_tab_stops);
	void _shaped_text_overrun_trim_to_width(
		const RID& p_shaped_line, double p_width, uint32_t p_trim_flags);
	int64_t _shaped_text_get_trim_pos(const RID& p_shaped) const;
	int64_t _shaped_text_get_ellipsis_pos(const RID& p_shaped) const;
	const Glyph* _shaped_text_get_ellipsis_glyphs(const RID& p_shaped) const;
	int64_t _shaped_text_get_ellipsis_glyph_count(const RID& p_shaped) const;
	PackedInt32Array _shaped_text_get_character_breaks(const RID& p_shaped) const;
	bool _shaped_text_update_breaks(const RID& p_shaped);
	_FORCE_INLINE_ int64_t _generate_kashida_justification_opportunities(
		const String& p_data, int64_t p_start, int64_t p_end);
	bool _shaped_text_update_justification_ops(const RID& p_shaped);
	bool _shaped_text_shape(const RID& p_shaped);
	bool _shaped_text_is_ready(const RID& p_shaped) const;
	const Glyph* _shaped_text_get_glyphs(const RID& p_shaped) const;
	int64_t _shaped_text_get_glyph_count(const RID& p_shaped) const;
	const Glyph* _shaped_text_sort_logical(const RID& p_shaped);
	Vector2i _shaped_text_get_range(const RID& p_shaped) const;
	Size2 _shaped_text_get_size(const RID& p_shaped) const;
	double _shaped_text_get_ascent(const RID& p_shaped) const;
	double _shaped_text_get_descent(const RID& p_shaped) const;
	double _shaped_text_get_width(const RID& p_shaped) const;
	double _shaped_text_get_underline_position(const RID& p_shaped) const;
	double _shaped_text_get_underline_thickness(const RID& p_shaped) const;
	int64_t _is_confusable(const String& p_string, const PackedStringArray& p_dict) const;
	bool _spoof_check(const String& p_string) const;
	String _strip_diacritics(const String& p_string) const;
	String _string_to_upper(const String& p_string, const String& p_language) const;
	String _string_to_lower(const String& p_string, const String& p_language) const;
	String _string_to_title(const String& p_string, const String& p_language) const;
	PackedInt32Array _string_get_word_breaks(
		const String& p_string, const String& p_language, int64_t p_chars_per_line) const;
	PackedInt32Array _string_get_character_breaks(
		const String& p_string, const String& p_language) const;
	bool _is_valid_identifier(const String& p_string) const;
	bool _is_valid_letter(uint64_t p_unicode) const;
	void _font_clear_system_fallback_cache();
	String _font_get_name(const RID& p_font_rid) const;
	void _cleanup();

	// Virtual TextServer overrides forwarding to TextServerAdvanced implementations
	bool has_feature(Feature p_feature) const override { return _has_feature(p_feature); }
	String get_name() const override { return _get_name(); }
	String get_short_name() const override { return _get_short_name(); }
	int64_t get_features() const override { return _get_features(); }
	void free_rid(const RID& p_rid) override { _free_rid(p_rid); }
	bool has(const RID& p_rid) override { return _has(p_rid); }
	bool load_support_data(const String& p_filename) override
	{
		return _load_support_data(p_filename);
	}
	String get_support_data_filename() const override { return _get_support_data_filename(); }
	String get_support_data_info() const override { return _get_support_data_info(); }
	bool save_support_data(const String& p_filename) const override
	{
		return _save_support_data(p_filename);
	}
	PackedByteArray get_support_data() const override { return _get_support_data(); }
	bool is_locale_using_support_data(const String& p_locale) const override
	{
		return _is_locale_using_support_data(p_locale);
	}
	bool is_locale_right_to_left(const String& p_locale) const override
	{
		return _is_locale_right_to_left(p_locale);
	}
	int64_t name_to_tag(const String& p_name) const override { return _name_to_tag(p_name); }
	String tag_to_name(int64_t p_tag) const override { return _tag_to_name(p_tag); }

	void reference_oversampling_level(double p_oversampling) override
	{
		_reference_oversampling_level(p_oversampling);
	}
	void unreference_oversampling_level(double p_oversampling) override
	{
		_unreference_oversampling_level(p_oversampling);
	}

	RID create_font() override { return _create_font(); }
	RID create_font_linked_variation(const RID& p_font_rid) override
	{
		return _create_font_linked_variation(p_font_rid);
	}
	void font_set_data(const RID& p_font_rid, const PackedByteArray& p_data) override
	{
		_font_set_data(p_font_rid, p_data);
	}
	void font_set_data_ptr(
		const RID& p_font_rid, const uint8_t* p_data_ptr, int64_t p_data_size) override
	{
		_font_set_data_ptr(p_font_rid, p_data_ptr, p_data_size);
	}
	void font_set_face_index(const RID& p_font_rid, int64_t p_index) override
	{
		_font_set_face_index(p_font_rid, p_index);
	}
	int64_t font_get_face_index(const RID& p_font_rid) const override
	{
		return _font_get_face_index(p_font_rid);
	}
	int64_t font_get_face_count(const RID& p_font_rid) const override
	{
		return _font_get_face_count(p_font_rid);
	}
	void font_set_style(const RID& p_font_rid, uint32_t p_style) override
	{
		_font_set_style(p_font_rid, p_style);
	}
	uint32_t font_get_style(const RID& p_font_rid) const override
	{
		return _font_get_style(p_font_rid);
	}
	void font_set_name(const RID& p_font_rid, const String& p_name) override
	{
		_font_set_name(p_font_rid, p_name);
	}
	String font_get_name(const RID& p_font_rid) const override
	{
		return _font_get_name(p_font_rid);
	}
	void font_set_style_name(const RID& p_font_rid, const String& p_name) override
	{
		_font_set_style_name(p_font_rid, p_name);
	}
	String font_get_style_name(const RID& p_font_rid) const override
	{
		return _font_get_style_name(p_font_rid);
	}
	void font_set_weight(const RID& p_font_rid, int64_t p_weight) override
	{
		_font_set_weight(p_font_rid, p_weight);
	}
	int64_t font_get_weight(const RID& p_font_rid) const override
	{
		return _font_get_weight(p_font_rid);
	}
	void font_set_stretch(const RID& p_font_rid, int64_t p_stretch) override
	{
		_font_set_stretch(p_font_rid, p_stretch);
	}
	int64_t font_get_stretch(const RID& p_font_rid) const override
	{
		return _font_get_stretch(p_font_rid);
	}
	void font_set_antialiasing(const RID& p_font_rid, FontAntialiasing p_antialiasing) override
	{
		_font_set_antialiasing(p_font_rid, p_antialiasing);
	}
	FontAntialiasing font_get_antialiasing(const RID& p_font_rid) const override
	{
		return _font_get_antialiasing(p_font_rid);
	}
	void font_set_disable_embedded_bitmaps(
		const RID& p_font_rid, bool p_disable_embedded_bitmaps) override
	{
		_font_set_disable_embedded_bitmaps(p_font_rid, p_disable_embedded_bitmaps);
	}
	bool font_get_disable_embedded_bitmaps(const RID& p_font_rid) const override
	{
		return _font_get_disable_embedded_bitmaps(p_font_rid);
	}
	void font_set_generate_mipmaps(const RID& p_font_rid, bool p_generate_mipmaps) override
	{
		_font_set_generate_mipmaps(p_font_rid, p_generate_mipmaps);
	}
	bool font_get_generate_mipmaps(const RID& p_font_rid) const override
	{
		return _font_get_generate_mipmaps(p_font_rid);
	}
	void font_set_multichannel_signed_distance_field(const RID& p_font_rid, bool p_msdf) override
	{
		_font_set_multichannel_signed_distance_field(p_font_rid, p_msdf);
	}
	bool font_is_multichannel_signed_distance_field(const RID& p_font_rid) const override
	{
		return _font_is_multichannel_signed_distance_field(p_font_rid);
	}
	void font_set_msdf_pixel_range(const RID& p_font_rid, int64_t p_msdf_pixel_range) override
	{
		_font_set_msdf_pixel_range(p_font_rid, p_msdf_pixel_range);
	}
	int64_t font_get_msdf_pixel_range(const RID& p_font_rid) const override
	{
		return _font_get_msdf_pixel_range(p_font_rid);
	}
	void font_set_msdf_size(const RID& p_font_rid, int64_t p_msdf_size) override
	{
		_font_set_msdf_size(p_font_rid, p_msdf_size);
	}
	int64_t font_get_msdf_size(const RID& p_font_rid) const override
	{
		return _font_get_msdf_size(p_font_rid);
	}
	void font_set_fixed_size(const RID& p_font_rid, int64_t p_fixed_size) override
	{
		_font_set_fixed_size(p_font_rid, p_fixed_size);
	}
	int64_t font_get_fixed_size(const RID& p_font_rid) const override
	{
		return _font_get_fixed_size(p_font_rid);
	}
	void font_set_fixed_size_scale_mode(
		const RID& p_font_rid, FixedSizeScaleMode p_fixed_size_scale) override
	{
		_font_set_fixed_size_scale_mode(p_font_rid, p_fixed_size_scale);
	}
	FixedSizeScaleMode font_get_fixed_size_scale_mode(const RID& p_font_rid) const override
	{
		return _font_get_fixed_size_scale_mode(p_font_rid);
	}
	void font_set_allow_system_fallback(
		const RID& p_font_rid, bool p_allow_system_fallback) override
	{
		_font_set_allow_system_fallback(p_font_rid, p_allow_system_fallback);
	}
	bool font_is_allow_system_fallback(const RID& p_font_rid) const override
	{
		return _font_is_allow_system_fallback(p_font_rid);
	}
	void font_clear_system_fallback_cache() override { _font_clear_system_fallback_cache(); }
	void font_set_force_autohinter(const RID& p_font_rid, bool p_force_autohinter) override
	{
		_font_set_force_autohinter(p_font_rid, p_force_autohinter);
	}
	bool font_is_force_autohinter(const RID& p_font_rid) const override
	{
		return _font_is_force_autohinter(p_font_rid);
	}
	void font_set_modulate_color_glyphs(const RID& p_font_rid, bool p_modulate) override
	{
		_font_set_modulate_color_glyphs(p_font_rid, p_modulate);
	}
	bool font_is_modulate_color_glyphs(const RID& p_font_rid) const override
	{
		return _font_is_modulate_color_glyphs(p_font_rid);
	}
	void font_set_hinting(const RID& p_font_rid, Hinting p_hinting) override
	{
		_font_set_hinting(p_font_rid, p_hinting);
	}
	Hinting font_get_hinting(const RID& p_font_rid) const override
	{
		return _font_get_hinting(p_font_rid);
	}
	void font_set_subpixel_positioning(
		const RID& p_font_rid, SubpixelPositioning p_subpixel) override
	{
		_font_set_subpixel_positioning(p_font_rid, p_subpixel);
	}
	SubpixelPositioning font_get_subpixel_positioning(const RID& p_font_rid) const override
	{
		return _font_get_subpixel_positioning(p_font_rid);
	}
	void font_set_keep_rounding_remainders(
		const RID& p_font_rid, bool p_keep_rounding_remainders) override
	{
		_font_set_keep_rounding_remainders(p_font_rid, p_keep_rounding_remainders);
	}
	bool font_get_keep_rounding_remainders(const RID& p_font_rid) const override
	{
		return _font_get_keep_rounding_remainders(p_font_rid);
	}
	void font_set_embolden(const RID& p_font_rid, double p_strength) override
	{
		_font_set_embolden(p_font_rid, p_strength);
	}
	double font_get_embolden(const RID& p_font_rid) const override
	{
		return _font_get_embolden(p_font_rid);
	}
	void font_set_spacing(const RID& p_font_rid, SpacingType p_spacing, int64_t p_value) override
	{
		_font_set_spacing(p_font_rid, p_spacing, p_value);
	}
	int64_t font_get_spacing(const RID& p_font_rid, SpacingType p_spacing) const override
	{
		return _font_get_spacing(p_font_rid, p_spacing);
	}
	void font_set_baseline_offset(const RID& p_font_rid, double p_baseline_offset) override
	{
		_font_set_baseline_offset(p_font_rid, p_baseline_offset);
	}
	double font_get_baseline_offset(const RID& p_font_rid) const override
	{
		return _font_get_baseline_offset(p_font_rid);
	}
	Transform2D font_get_transform(const RID& p_font_rid) const override
	{
		return _font_get_transform(p_font_rid);
	}
	void font_set_oversampling(const RID& p_font_rid, double p_oversampling) override
	{
		_font_set_oversampling(p_font_rid, p_oversampling);
	}
	double font_get_oversampling(const RID& p_font_rid) const override
	{
		return _font_get_oversampling(p_font_rid);
	}
	void font_clear_size_cache(const RID& p_font_rid) override
	{
		_font_clear_size_cache(p_font_rid);
	}
	void font_remove_size_cache(const RID& p_font_rid, const Vector2i& p_size) override
	{
		_font_remove_size_cache(p_font_rid, p_size);
	}
	void font_set_ascent(const RID& p_font_rid, int64_t p_size, double p_ascent) override
	{
		_font_set_ascent(p_font_rid, p_size, p_ascent);
	}
	double font_get_ascent(const RID& p_font_rid, int64_t p_size) const override
	{
		return _font_get_ascent(p_font_rid, p_size);
	}
	void font_set_descent(const RID& p_font_rid, int64_t p_size, double p_descent) override
	{
		_font_set_descent(p_font_rid, p_size, p_descent);
	}
	double font_get_descent(const RID& p_font_rid, int64_t p_size) const override
	{
		return _font_get_descent(p_font_rid, p_size);
	}
	void font_set_underline_position(
		const RID& p_font_rid, int64_t p_size, double p_underline_position) override
	{
		_font_set_underline_position(p_font_rid, p_size, p_underline_position);
	}
	double font_get_underline_position(const RID& p_font_rid, int64_t p_size) const override
	{
		return _font_get_underline_position(p_font_rid, p_size);
	}
	void font_set_underline_thickness(
		const RID& p_font_rid, int64_t p_size, double p_underline_thickness) override
	{
		_font_set_underline_thickness(p_font_rid, p_size, p_underline_thickness);
	}
	double font_get_underline_thickness(const RID& p_font_rid, int64_t p_size) const override
	{
		return _font_get_underline_thickness(p_font_rid, p_size);
	}
	void font_set_scale(const RID& p_font_rid, int64_t p_size, double p_scale) override
	{
		_font_set_scale(p_font_rid, p_size, p_scale);
	}
	double font_get_scale(const RID& p_font_rid, int64_t p_size) const override
	{
		return _font_get_scale(p_font_rid, p_size);
	}
	int64_t font_get_texture_count(const RID& p_font_rid, const Vector2i& p_size) const override
	{
		return _font_get_texture_count(p_font_rid, p_size);
	}
	void font_clear_textures(const RID& p_font_rid, const Vector2i& p_size) override
	{
		_font_clear_textures(p_font_rid, p_size);
	}
	void font_remove_texture(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_texture_index) override
	{
		_font_remove_texture(p_font_rid, p_size, p_texture_index);
	}
	void font_set_texture_image(const RID& p_font_rid, const Vector2i& p_size,
		int64_t p_texture_index, const Ref<Image>& p_image) override
	{
		_font_set_texture_image(p_font_rid, p_size, p_texture_index, p_image);
	}
	Ref<Image> font_get_texture_image(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_texture_index) const override
	{
		return _font_get_texture_image(p_font_rid, p_size, p_texture_index);
	}
	Vector2 font_get_glyph_advance(
		const RID& p_font_rid, int64_t p_size, int64_t p_glyph) const override
	{
		return _font_get_glyph_advance(p_font_rid, p_size, p_glyph);
	}
	void font_set_glyph_advance(const RID& p_font_rid, int64_t p_size, int64_t p_glyph,
		const Vector2& p_advance) override
	{
		_font_set_glyph_advance(p_font_rid, p_size, p_glyph, p_advance);
	}
	Vector2 font_get_glyph_offset(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const override
	{
		return _font_get_glyph_offset(p_font_rid, p_size, p_glyph);
	}
	void font_set_glyph_offset(const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph,
		const Vector2& p_offset) override
	{
		_font_set_glyph_offset(p_font_rid, p_size, p_glyph, p_offset);
	}
	Vector2 font_get_glyph_size(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const override
	{
		return _font_get_glyph_size(p_font_rid, p_size, p_glyph);
	}
	void font_set_glyph_size(const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph,
		const Vector2& p_gl_size) override
	{
		_font_set_glyph_size(p_font_rid, p_size, p_glyph, p_gl_size);
	}
	Rect2 font_get_glyph_uv_rect(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const override
	{
		return _font_get_glyph_uv_rect(p_font_rid, p_size, p_glyph);
	}
	void font_set_glyph_uv_rect(const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph,
		const Rect2& p_uv_rect) override
	{
		_font_set_glyph_uv_rect(p_font_rid, p_size, p_glyph, p_uv_rect);
	}
	int64_t font_get_glyph_texture_idx(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const override
	{
		return _font_get_glyph_texture_idx(p_font_rid, p_size, p_glyph);
	}
	void font_set_glyph_texture_idx(const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph,
		int64_t p_texture_idx) override
	{
		_font_set_glyph_texture_idx(p_font_rid, p_size, p_glyph, p_texture_idx);
	}
	RID font_get_glyph_texture_rid(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const override
	{
		return _font_get_glyph_texture_rid(p_font_rid, p_size, p_glyph);
	}
	Size2 font_get_glyph_texture_size(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const override
	{
		return _font_get_glyph_texture_size(p_font_rid, p_size, p_glyph);
	}
	void font_clear_kerning_map(const RID& p_font_rid, int64_t p_size) override
	{
		_font_clear_kerning_map(p_font_rid, p_size);
	}
	void font_remove_kerning(
		const RID& p_font_rid, int64_t p_size, const Vector2i& p_glyph_pair) override
	{
		_font_remove_kerning(p_font_rid, p_size, p_glyph_pair);
	}
	void font_set_kerning(const RID& p_font_rid, int64_t p_size, const Vector2i& p_glyph_pair,
		const Vector2& p_kerning) override
	{
		_font_set_kerning(p_font_rid, p_size, p_glyph_pair, p_kerning);
	}
	Vector2 font_get_kerning(
		const RID& p_font_rid, int64_t p_size, const Vector2i& p_glyph_pair) const override
	{
		return _font_get_kerning(p_font_rid, p_size, p_glyph_pair);
	}
	int64_t font_get_glyph_index(const RID& p_font_rid, int64_t p_size, int64_t p_char,
		int64_t p_variation_selector) const override
	{
		return _font_get_glyph_index(p_font_rid, p_size, p_char, p_variation_selector);
	}
	int64_t font_get_char_from_glyph_index(
		const RID& p_font_rid, int64_t p_size, int64_t p_glyph_index) const override
	{
		return _font_get_char_from_glyph_index(p_font_rid, p_size, p_glyph_index);
	}
	bool font_has_char(const RID& p_font_rid, int64_t p_char) const override
	{
		return _font_has_char(p_font_rid, p_char);
	}
	String font_get_supported_chars(const RID& p_font_rid) const override
	{
		return _font_get_supported_chars(p_font_rid);
	}
	PackedInt32Array font_get_supported_glyphs(const RID& p_font_rid) const override
	{
		return _font_get_supported_glyphs(p_font_rid);
	}
	void font_render_range(
		const RID& p_font, const Vector2i& p_size, int64_t p_start, int64_t p_end) override
	{
		_font_render_range(p_font, p_size, p_start, p_end);
	}
	void font_render_glyph(
		const RID& p_font_rid, const Vector2i& p_size, int64_t p_index) override
	{
		_font_render_glyph(p_font_rid, p_size, p_index);
	}
	void font_draw_glyph(const RID& p_font, const RID& p_canvas, int64_t p_size,
		const Vector2& p_pos, int64_t p_index, const Color& p_color = Color(1, 1, 1),
		float p_oversampling = 0.0) const override
	{
		_font_draw_glyph(p_font, p_canvas, p_size, p_pos, p_index, p_color, p_oversampling);
	}
	void font_draw_glyph_outline(const RID& p_font, const RID& p_canvas, int64_t p_size,
		int64_t p_outline_size, const Vector2& p_pos, int64_t p_index,
		const Color& p_color = Color(1, 1, 1), float p_oversampling = 0.0) const override
	{
		_font_draw_glyph_outline(
			p_font, p_canvas, p_size, p_outline_size, p_pos, p_index, p_color, p_oversampling);
	}
	bool font_is_language_supported(
		const RID& p_font_rid, const String& p_language) const override
	{
		return _font_is_language_supported(p_font_rid, p_language);
	}
	bool font_is_script_supported(const RID& p_font_rid, const String& p_script) const override
	{
		return _font_is_script_supported(p_font_rid, p_script);
	}
	void cleanup() override { _cleanup(); }

	~TextServerAdvanced();
};

#ifdef MODULE_SVG_ENABLED
#ifdef MODULE_FREETYPE_ENABLED

#include <freetype/freetype.h>
#include <freetype/otsvg.h>
#include <ft2build.h>
#include <thorvg.h>

struct GL_State {
	bool ready = false;
	float x = 0;
	float y = 0;
	float w = 0;
	float h = 0;
	CharString xml_code;
	tvg::Matrix m;
};

struct TVG_NodeCache {
	uint64_t document_offset;
	uint64_t body_offset;
};

struct TVG_DocumentCache {
	String xml_body;
	double embox_x;
	double embox_y;
	HashMap<int64_t, Vector<TVG_NodeCache>> node_caches;
};

struct TVG_State {
	Mutex mutex;
	HashMap<uint32_t, GL_State> glyph_map;
	HashMap<FT_Byte *, TVG_DocumentCache> document_map;
};

FT_Error tvg_svg_in_ot_init(FT_Pointer *p_state);
void tvg_svg_in_ot_free(FT_Pointer *p_state);
FT_Error tvg_svg_in_ot_preset_slot(FT_GlyphSlot p_slot, FT_Bool p_cache, FT_Pointer *p_state);
FT_Error tvg_svg_in_ot_render(FT_GlyphSlot p_slot, FT_Pointer *p_state);

SVG_RendererHooks *get_tvg_svg_in_ot_hooks();

#endif // MODULE_FREETYPE_ENABLED
#endif // MODULE_SVG_ENABLED

// --- End modules/text_server_adv/text_server_adv.h ---

// Built-in ICU data.

#ifdef ICU_STATIC_DATA
#include <icudata.gen.h>
#endif

// Thirdparty headers.

#ifdef MODULE_MSDFGEN_ENABLED
VLTR_GCC_WARNING_PUSH_AND_IGNORE("-Wshadow")
VLTR_MSVC_WARNING_PUSH_AND_IGNORE(4458) // "Declaration of 'identifier' hides class member".

#include <core/EdgeHolder.h>
#include <core/ShapeDistanceFinder.h>
#include <core/contour-combiners.h>
#include <core/edge-selectors.h>
#include <msdfgen.h>

VLTR_GCC_WARNING_POP
VLTR_MSVC_WARNING_POP
#endif

#ifdef MODULE_SVG_ENABLED
#ifdef MODULE_FREETYPE_ENABLED
#endif
#endif

/*************************************************************************/
/*  bmp_font_t HarfBuzz Bitmap font interface                            */
/*************************************************************************/

hb_font_funcs_t* TextServerAdvanced::funcs = nullptr;

TextServerAdvanced::bmp_font_t* TextServerAdvanced::_bmp_font_create(
	TextServerAdvanced::FontForSizeAdvanced* p_face, bool p_unref)
{
	bmp_font_t* bm_font = memnew(bmp_font_t);

	if (!bm_font) {
		return nullptr;
	}

	bm_font->face = p_face;
	bm_font->unref = p_unref;

	return bm_font;
}

void TextServerAdvanced::_bmp_font_destroy(void* p_data)
{
	bmp_font_t* bm_font = static_cast<bmp_font_t*>(p_data);
	memdelete(bm_font);
}

hb_bool_t TextServerAdvanced::_bmp_get_nominal_glyph(hb_font_t* p_font, void* p_font_data,
	hb_codepoint_t p_unicode, hb_codepoint_t* r_glyph, void* p_user_data)
{
	const bmp_font_t* bm_font = static_cast<const bmp_font_t*>(p_font_data);

	if (!bm_font->face) {
		return false;
	}

	if (!bm_font->face->glyph_map.has(p_unicode)) {
		if (bm_font->face->glyph_map.has(0xf000u + p_unicode)) {
			*r_glyph = 0xf000u + p_unicode;
			return true;
		}
		else {
			return false;
		}
	}

	*r_glyph = p_unicode;
	return true;
}

hb_position_t TextServerAdvanced::_bmp_get_glyph_h_advance(
	hb_font_t* p_font, void* p_font_data, hb_codepoint_t p_glyph, void* p_user_data)
{
	const bmp_font_t* bm_font = static_cast<const bmp_font_t*>(p_font_data);

	if (!bm_font->face) {
		return 0;
	}

	HashMap<int32_t, FontGlyph>::Iterator E = bm_font->face->glyph_map.find(p_glyph);
	if (!E) {
		return 0;
	}

	return E->value.advance.x * 64;
}

hb_position_t TextServerAdvanced::_bmp_get_glyph_v_advance(
	hb_font_t* p_font, void* p_font_data, hb_codepoint_t p_glyph, void* p_user_data)
{
	const bmp_font_t* bm_font = static_cast<const bmp_font_t*>(p_font_data);

	if (!bm_font->face) {
		return 0;
	}

	HashMap<int32_t, FontGlyph>::Iterator E = bm_font->face->glyph_map.find(p_glyph);
	if (!E) {
		return 0;
	}

	return -E->value.advance.y * 64;
}

hb_position_t TextServerAdvanced::_bmp_get_glyph_h_kerning(hb_font_t* p_font, void* p_font_data,
	hb_codepoint_t p_left_glyph, hb_codepoint_t p_right_glyph, void* p_user_data)
{
	const bmp_font_t* bm_font = static_cast<const bmp_font_t*>(p_font_data);

	if (!bm_font->face) {
		return 0;
	}

	if (!bm_font->face->kerning_map.has(Vector2i(p_left_glyph, p_right_glyph))) {
		return 0;
	}

	return bm_font->face->kerning_map[Vector2i(p_left_glyph, p_right_glyph)].x * 64;
}

hb_bool_t TextServerAdvanced::_bmp_get_glyph_v_origin(hb_font_t* p_font, void* p_font_data,
	hb_codepoint_t p_glyph, hb_position_t* r_x, hb_position_t* r_y, void* p_user_data)
{
	const bmp_font_t* bm_font = static_cast<const bmp_font_t*>(p_font_data);

	if (!bm_font->face) {
		return false;
	}

	HashMap<int32_t, FontGlyph>::Iterator E = bm_font->face->glyph_map.find(p_glyph);
	if (!E) {
		return false;
	}

	*r_x = E->value.advance.x * 32;
	*r_y = -bm_font->face->ascent * 64;

	return true;
}

hb_bool_t TextServerAdvanced::_bmp_get_glyph_extents(hb_font_t* p_font, void* p_font_data,
	hb_codepoint_t p_glyph, hb_glyph_extents_t* r_extents, void* p_user_data)
{
	const bmp_font_t* bm_font = static_cast<const bmp_font_t*>(p_font_data);

	if (!bm_font->face) {
		return false;
	}

	HashMap<int32_t, FontGlyph>::Iterator E = bm_font->face->glyph_map.find(p_glyph);
	if (!E) {
		return false;
	}

	r_extents->x_bearing = 0;
	r_extents->y_bearing = 0;
	r_extents->width = E->value.rect.size.x * 64;
	r_extents->height = E->value.rect.size.y * 64;

	return true;
}

hb_bool_t TextServerAdvanced::_bmp_get_font_h_extents(
	hb_font_t* p_font, void* p_font_data, hb_font_extents_t* r_metrics, void* p_user_data)
{
	const bmp_font_t* bm_font = static_cast<const bmp_font_t*>(p_font_data);

	if (!bm_font->face) {
		return false;
	}

	r_metrics->ascender = bm_font->face->ascent;
	r_metrics->descender = bm_font->face->descent;
	r_metrics->line_gap = 0;

	return true;
}

void TextServerAdvanced::_bmp_create_font_funcs()
{
	if (funcs == nullptr) {
		funcs = hb_font_funcs_create();

		hb_font_funcs_set_font_h_extents_func(funcs, _bmp_get_font_h_extents, nullptr, nullptr);
		hb_font_funcs_set_nominal_glyph_func(funcs, _bmp_get_nominal_glyph, nullptr, nullptr);
		hb_font_funcs_set_glyph_h_advance_func(funcs, _bmp_get_glyph_h_advance, nullptr, nullptr);
		hb_font_funcs_set_glyph_v_advance_func(funcs, _bmp_get_glyph_v_advance, nullptr, nullptr);
		hb_font_funcs_set_glyph_v_origin_func(funcs, _bmp_get_glyph_v_origin, nullptr, nullptr);
		hb_font_funcs_set_glyph_h_kerning_func(funcs, _bmp_get_glyph_h_kerning, nullptr, nullptr);
		hb_font_funcs_set_glyph_extents_func(funcs, _bmp_get_glyph_extents, nullptr, nullptr);

		hb_font_funcs_make_immutable(funcs);
	}
}

void TextServerAdvanced::_bmp_free_font_funcs()
{
	if (funcs != nullptr) {
		hb_font_funcs_destroy(funcs);
		funcs = nullptr;
	}
}

void TextServerAdvanced::_bmp_font_set_funcs(
	hb_font_t* p_font, TextServerAdvanced::FontForSizeAdvanced* p_face, bool p_unref)
{
	hb_font_set_funcs(p_font, funcs, _bmp_font_create(p_face, p_unref), _bmp_font_destroy);
}

hb_font_t* TextServerAdvanced::_bmp_font_create(
	TextServerAdvanced::FontForSizeAdvanced* p_face, hb_destroy_func_t p_destroy)
{
	hb_font_t* font;
	hb_face_t* face = hb_face_create(nullptr, 0);

	font = hb_font_create(face);
	hb_face_destroy(face);
	_bmp_font_set_funcs(font, p_face, false);
	return font;
}

/*************************************************************************/
/*  Character properties.                                                */
/*************************************************************************/

_FORCE_INLINE_ bool is_ain(char32_t p_chr)
{
	return u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_AIN;
}

_FORCE_INLINE_ bool is_alef(char32_t p_chr)
{
	return u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_ALEF;
}

_FORCE_INLINE_ bool is_beh(char32_t p_chr)
{
	int32_t prop = u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP);
	return (prop == U_JG_BEH) || (prop == U_JG_NOON) || (prop == U_JG_AFRICAN_NOON) ||
		   (prop == U_JG_NYA) || (prop == U_JG_YEH) || (prop == U_JG_FARSI_YEH);
}

_FORCE_INLINE_ bool is_dal(char32_t p_chr)
{
	return u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_DAL;
}

_FORCE_INLINE_ bool is_feh(char32_t p_chr)
{
	return (u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_FEH) ||
		   (u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_AFRICAN_FEH);
}

_FORCE_INLINE_ bool is_gaf(char32_t p_chr)
{
	return u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_GAF;
}

_FORCE_INLINE_ bool is_heh(char32_t p_chr)
{
	return u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_HEH;
}

_FORCE_INLINE_ bool is_kaf(char32_t p_chr)
{
	return u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_KAF;
}

_FORCE_INLINE_ bool is_lam(char32_t p_chr)
{
	return u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_LAM;
}

_FORCE_INLINE_ bool is_qaf(char32_t p_chr)
{
	return (u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_QAF) ||
		   (u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_AFRICAN_QAF);
}

_FORCE_INLINE_ bool is_reh(char32_t p_chr)
{
	return u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_REH;
}

_FORCE_INLINE_ bool is_seen_sad(char32_t p_chr)
{
	return (u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_SAD) ||
		   (u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_SEEN);
}

_FORCE_INLINE_ bool is_tah(char32_t p_chr)
{
	return u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_TAH;
}

_FORCE_INLINE_ bool is_teh_marbuta(char32_t p_chr)
{
	return u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_TEH_MARBUTA;
}

_FORCE_INLINE_ bool is_yeh(char32_t p_chr)
{
	int32_t prop = u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP);
	return (prop == U_JG_YEH) || (prop == U_JG_FARSI_YEH) || (prop == U_JG_YEH_BARREE) ||
		   (prop == U_JG_BURUSHASKI_YEH_BARREE) || (prop == U_JG_YEH_WITH_TAIL);
}

_FORCE_INLINE_ bool is_waw(char32_t p_chr)
{
	return u_getIntPropertyValue(p_chr, UCHAR_JOINING_GROUP) == U_JG_WAW;
}

_FORCE_INLINE_ bool is_transparent(char32_t p_chr)
{
	return u_getIntPropertyValue(p_chr, UCHAR_JOINING_TYPE) == U_JT_TRANSPARENT;
}

_FORCE_INLINE_ bool is_ligature(char32_t p_chr, char32_t p_nchr)
{
	return (is_lam(p_chr) && is_alef(p_nchr));
}

_FORCE_INLINE_ bool is_connected_to_prev(char32_t p_chr, char32_t p_pchr)
{
	int32_t prop = u_getIntPropertyValue(p_pchr, UCHAR_JOINING_TYPE);
	return (prop != U_JT_RIGHT_JOINING) && (prop != U_JT_NON_JOINING) ? !is_ligature(p_pchr, p_chr)
																	  : false;
}

/*************************************************************************/

bool TextServerAdvanced::icu_data_loaded = false;
PackedByteArray TextServerAdvanced::icu_data;

bool TextServerAdvanced::_has_feature(Feature p_feature) const
{
	switch (p_feature) {
	case FEATURE_SIMPLE_LAYOUT:
	case FEATURE_BIDI_LAYOUT:
	case FEATURE_VERTICAL_LAYOUT:
	case FEATURE_SHAPING:
	case FEATURE_KASHIDA_JUSTIFICATION:
	case FEATURE_BREAK_ITERATORS:
	case FEATURE_FONT_BITMAP:
#ifdef MODULE_FREETYPE_ENABLED
	case FEATURE_FONT_DYNAMIC:
#endif
#ifdef MODULE_MSDFGEN_ENABLED
	case FEATURE_FONT_MSDF:
#endif
	case FEATURE_FONT_VARIABLE:
	case FEATURE_CONTEXT_SENSITIVE_CASE_CONVERSION:
	case FEATURE_USE_SUPPORT_DATA:
	case FEATURE_UNICODE_IDENTIFIERS:
	case FEATURE_UNICODE_SECURITY:
		return true;
	default: {
	}
	}
	return false;
}

String TextServerAdvanced::_get_name() const { return "ICU / HarfBuzz / Graphite (Built-in)"; }

String TextServerAdvanced::_get_short_name() const { return "advanced"; }

int64_t TextServerAdvanced::_get_features() const
{
	int64_t interface_features =
		FEATURE_SIMPLE_LAYOUT | FEATURE_BIDI_LAYOUT | FEATURE_VERTICAL_LAYOUT | FEATURE_SHAPING |
		FEATURE_KASHIDA_JUSTIFICATION | FEATURE_BREAK_ITERATORS | FEATURE_FONT_BITMAP |
		FEATURE_FONT_VARIABLE | FEATURE_CONTEXT_SENSITIVE_CASE_CONVERSION |
		FEATURE_USE_SUPPORT_DATA;
#ifdef MODULE_FREETYPE_ENABLED
	interface_features |= FEATURE_FONT_DYNAMIC;
#endif
#ifdef MODULE_MSDFGEN_ENABLED
	interface_features |= FEATURE_FONT_MSDF;
#endif

	return interface_features;
}

void TextServerAdvanced::_free_rid(const RID& p_rid)
{
	_THREAD_SAFE_METHOD_
	if (font_owner.owns(p_rid)) {
		MutexLock ftlock(ft_mutex);

		FontAdvanced* fd = font_owner.get_or_null(p_rid);
		for (const KeyValue<Vector2i, FontForSizeAdvanced*>& ffsd : fd->cache) {
			OversamplingLevel* ol = oversampling_levels.getptr(ffsd.value->viewport_oversampling);
			if (ol != nullptr) {
				ol->fonts.erase(ffsd.value);
			}
		}
		{
			MutexLock lock(fd->mutex);
			font_owner.free(p_rid);
		}
		memdelete(fd);
	}
	else if (font_var_owner.owns(p_rid)) {
		MutexLock ftlock(ft_mutex);

		FontAdvancedLinkedVariation* fdv = font_var_owner.get_or_null(p_rid);
		{
			font_var_owner.free(p_rid);
		}
		memdelete(fdv);
	}
	else if (shaped_owner.owns(p_rid)) {
		ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_rid);
		{
			MutexLock lock(sd->mutex);
			shaped_owner.free(p_rid);
		}
		memdelete(sd);
	}
}

bool TextServerAdvanced::_has(const RID& p_rid)
{
	_THREAD_SAFE_METHOD_
	return font_owner.owns(p_rid) || font_var_owner.owns(p_rid) || shaped_owner.owns(p_rid);
}

bool TextServerAdvanced::_load_support_data(const String& p_filename)
{
	_THREAD_SAFE_METHOD_

#if defined(ICU_STATIC_DATA) || !defined(HAVE_ICU_BUILTIN)
	if (!icu_data_loaded) {
		UErrorCode err = U_ZERO_ERROR;
		u_init(&err); // Do not check for errors, since we only load part of the data.
		icu_data_loaded = true;
	}
#else
	if (!icu_data_loaded) {
		UErrorCode err = U_ZERO_ERROR;
		String filename = (p_filename.is_empty()) ? String("res://icudt_godot.dat") : p_filename;
		if (FileAccess::exists(filename)) {
			Ref<FileAccess> f = FileAccess::open(filename, FileAccess::READ);
			if (f.is_null()) {
				return false;
			}
			uint64_t len = f->get_length();
			icu_data = f->get_buffer(len);

			udata_setCommonData(icu_data.ptr(), &err);
			if (U_FAILURE(err)) {
				ERR_FAIL_V_MSG(false, u_errorName(err));
			}

			err = U_ZERO_ERROR;
			icu_data_loaded = true;
		}

		u_init(&err);
		if (U_FAILURE(err)) {
			ERR_FAIL_V_MSG(false, u_errorName(err));
		}
	}
#endif
	return true;
}

String TextServerAdvanced::_get_support_data_filename() const { return String("icudt_godot.dat"); }

String TextServerAdvanced::_get_support_data_info() const
{
	return String("ICU break iteration data (\"icudt_godot.dat\").");
}

bool TextServerAdvanced::_save_support_data(const String& p_filename) const
{
	_THREAD_SAFE_METHOD_
#ifdef ICU_STATIC_DATA

	// Store data to the res file if it's available.

	Ref<FileAccess> f = FileAccess::open(p_filename, FileAccess::WRITE);
	if (f.is_null()) {
		return false;
	}

	PackedByteArray icu_data_static;
	icu_data_static.resize(U_ICUDATA_SIZE);
	memcpy(icu_data_static.ptrw(), U_ICUDATA_ENTRY_POINT, U_ICUDATA_SIZE);
	f->store_buffer(icu_data_static);

	return true;
#else
	return false;
#endif
}

PackedByteArray TextServerAdvanced::_get_support_data() const
{
	_THREAD_SAFE_METHOD_
#ifdef ICU_STATIC_DATA

	PackedByteArray icu_data_static;
	icu_data_static.resize(U_ICUDATA_SIZE);
	memcpy(icu_data_static.ptrw(), U_ICUDATA_ENTRY_POINT, U_ICUDATA_SIZE);

	return icu_data_static;
#else
	return icu_data;
#endif
}

bool TextServerAdvanced::_is_locale_using_support_data(const String& p_locale) const
{
	String l = p_locale.get_slicec('_', 0);
	if ((l == "my") || (l == "zh") || (l == "ja") || (l == "ko") || (l == "km") || (l == "lo") ||
		(l == "th")) {
		return true;
	}
	else {
		return false;
	}
}

bool TextServerAdvanced::_is_locale_right_to_left(const String& p_locale) const
{
	String l = p_locale.get_slicec('_', 0);
	if ((l == "ar") || (l == "dv") || (l == "he") || (l == "fa") || (l == "ff") || (l == "ku") ||
		(l == "ur")) {
		return true;
	}
	else {
		return false;
	}
}

int64_t TextServerAdvanced::_name_to_tag(const String& p_name) const
{
	if (feature_sets.has(p_name)) {
		return feature_sets[p_name];
	}

	// No readable name, use tag string.
	return hb_tag_from_string(p_name.replace("custom_", "").ascii().get_data(), -1);
}

bool TextServerAdvanced::_get_tag_hidden(int64_t p_tag) const
{
	if (feature_sets_inv.has(p_tag)) {
		return feature_sets_inv[p_tag].hidden;
	}
	return false;
}

String TextServerAdvanced::_tag_to_name(int64_t p_tag) const
{
	if (feature_sets_inv.has(p_tag)) {
		return feature_sets_inv[p_tag].name;
	}

	// No readable name, use tag string.
	char name[5];
	memset(name, 0, 5);
	hb_tag_to_string(p_tag, name);
	return String("custom_") + String(name);
}

/*************************************************************************/
/* Font Glyph Rendering                                                  */
/*************************************************************************/

_FORCE_INLINE_ TextServerAdvanced::FontTexturePosition
TextServerAdvanced::find_texture_pos_for_glyph(FontForSizeAdvanced* p_data, int p_color_size,
	Image::Format p_image_format, int p_width, int p_height, bool p_msdf) const
{
	FontTexturePosition ret;

	int mw = p_width;
	int mh = p_height;

	ShelfPackTexture* ct = p_data->textures.ptrw();
	for (int32_t i = 0; i < p_data->textures.size(); i++) {
		if (ct[i].image.is_null()) {
			continue;
		}
		if (p_image_format != ct[i].image->get_format()) {
			continue;
		}
		if (mw > ct[i].texture_w || mh > ct[i].texture_h) { // Too big for this texture.
			continue;
		}

		ret = ct[i].pack_rect(i, mh, mw);
		if (ret.index != -1) {
			break;
		}
	}

	if (ret.index == -1) {
		// Could not find texture to fit, create one.
		int texsize = MAX(p_data->size.x * 0.125, 256);

		texsize = Math::next_power_of_2((uint32_t)texsize);
		if (p_msdf) {
			texsize = MIN(texsize, 2048);
		}
		else {
			texsize = MIN(texsize, 1024);
		}
		if (mw > texsize) { // Special case, adapt to it?
			texsize = Math::next_power_of_2((uint32_t)mw);
		}
		if (mh > texsize) { // Special case, adapt to it?
			texsize = Math::next_power_of_2((uint32_t)mh);
		}

		ShelfPackTexture tex = ShelfPackTexture(texsize, texsize);
		tex.image = Image::create_empty(texsize, texsize, false, p_image_format);
		{
			// Zero texture.
			uint8_t* w = tex.image->ptrw();
			ERR_FAIL_COND_V(texsize * texsize * p_color_size > tex.image->get_data_size(), ret);
			// Initialize the texture to all-white pixels to prevent artifacts when the
			// font is displayed at a non-default scale with filtering enabled.
			if (p_color_size == 2) {
				for (int i = 0; i < texsize * texsize * p_color_size;
					 i += 2) { // FORMAT_LA8, BW font.
					w[i + 0] = 255;
					w[i + 1] = 0;
				}
			}
			else if (p_color_size == 4) {
				for (int i = 0; i < texsize * texsize * p_color_size;
					 i += 4) { // FORMAT_RGBA8, Color font, Multichannel(+True) SDF.
					if (p_msdf) {
						w[i + 0] = 0;
						w[i + 1] = 0;
						w[i + 2] = 0;
					}
					else {
						w[i + 0] = 255;
						w[i + 1] = 255;
						w[i + 2] = 255;
					}
					w[i + 3] = 0;
				}
			}
			else {
				ERR_FAIL_V(ret);
			}
		}
		p_data->textures.push_back(tex);

		int32_t idx = p_data->textures.size() - 1;
		ret = p_data->textures.write[idx].pack_rect(idx, mh, mw);
	}

	return ret;
}

#ifdef MODULE_MSDFGEN_ENABLED

struct MSContext
{
	msdfgen::Point2 position;
	msdfgen::Shape* shape = nullptr;
	msdfgen::Contour* contour = nullptr;
};

class DistancePixelConversion
{
	double invRange;

public:
	_FORCE_INLINE_ explicit DistancePixelConversion(double range) : invRange(1 / range) {}

	_FORCE_INLINE_ void operator()(
		float* pixels, const msdfgen::MultiAndTrueDistance& distance) const
	{
		pixels[0] = float(invRange * distance.r + .5);
		pixels[1] = float(invRange * distance.g + .5);
		pixels[2] = float(invRange * distance.b + .5);
		pixels[3] = float(invRange * distance.a + .5);
	}
};

struct MSDFThreadData
{
	msdfgen::Bitmap<float, 4>* output;
	msdfgen::Shape* shape;
	msdfgen::Projection* projection;
	DistancePixelConversion* distancePixelConversion;
};

// static msdfgen::Point2 ft_point2(const FT_Vector &vector) {
// 	return msdfgen::Point2(vector.x / 60.0f, vector.y / 60.0f);
// }

// static int ft_move_to(const FT_Vector *to, void *user) {
// 	MSContext *context = static_cast<MSContext *>(user);
// 	if (!(context->contour && context->contour->edges.empty())) {
// 		context->contour = &context->shape->addContour();
// 	}
// 	context->position = ft_point2(*to);
// 	return 0;
// }

// static int ft_line_to(const FT_Vector *to, void *user) {
// 	MSContext *context = static_cast<MSContext *>(user);
// 	msdfgen::Point2 endpoint = ft_point2(*to);
// 	if (endpoint != context->position) {
// 		context->contour->addEdge(new msdfgen::LinearSegment(context->position, endpoint));
// 		context->position = endpoint;
// 	}
// 	return 0;
// }

// static int ft_conic_to(const FT_Vector *control, const FT_Vector *to, void *user) {
// 	MSContext *context = static_cast<MSContext *>(user);
// 	context->contour->addEdge(new msdfgen::QuadraticSegment(context->position, ft_point2(*control),
// ft_point2(*to))); 	context->position = ft_point2(*to); 	return 0;
// }

// static int ft_cubic_to(const FT_Vector *control1, const FT_Vector *control2, const FT_Vector *to,
// void *user) { 	MSContext *context = static_cast<MSContext *>(user);
// context->contour->addEdge(new msdfgen::CubicSegment(context->position, ft_point2(*control1),
// ft_point2(*control2), ft_point2(*to))); 	context->position = ft_point2(*to); 	return 0;
// }

void TextServerAdvanced::_generateMTSDF_threaded(void* p_td, uint32_t p_y)
{
	MSDFThreadData* td = static_cast<MSDFThreadData*>(p_td);

	msdfgen::ShapeDistanceFinder<
		msdfgen::OverlappingContourCombiner<msdfgen::MultiAndTrueDistanceSelector>>
		distanceFinder(*td->shape);
	int row = td->shape->inverseYAxis ? td->output->height() - p_y - 1 : p_y;
	for (int col = 0; col < td->output->width(); ++col) {
		int x = (p_y % 2) ? td->output->width() - col - 1 : col;
		msdfgen::Point2 p = td->projection->unproject(msdfgen::Point2(x + .5, p_y + .5));
		msdfgen::MultiAndTrueDistance distance = distanceFinder.distance(p);
		td->distancePixelConversion->operator()(td->output->operator()(x, row), distance);
	}
}

#endif

#ifdef MODULE_FREETYPE_ENABLED
#if HB_VERSION_ATLEAST(13, 0, 0)
_FORCE_INLINE_ TextServerAdvanced::FontGlyph TextServerAdvanced::rasterize_hb_bitmap(
	FontForSizeAdvanced* p_data, int p_rect_margin, hb_raster_image_t* p_image,
	const hb_raster_extents_t& p_ext, const Vector2& p_advance, bool p_bgra) const
{
	FontGlyph chr;
	chr.advance = p_advance * p_data->scale;
	chr.found = true;

	int w = p_ext.width;
	int h = p_ext.height;

	if (w == 0 || h == 0 || p_image == nullptr) {
		chr.texture_idx = -1;
		chr.uv_rect = Rect2();
		chr.rect = Rect2();
		return chr;
	}

	int color_size = p_bgra ? 4 : 2;

	int mw = w + p_rect_margin * 4;
	int mh = h + p_rect_margin * 4;

	ERR_FAIL_COND_V(mw > 4096, FontGlyph());
	ERR_FAIL_COND_V(mh > 4096, FontGlyph());

	Image::Format require_format = color_size == 4 ? Image::FORMAT_RGBA8 : Image::FORMAT_LA8;

	FontTexturePosition tex_pos =
		find_texture_pos_for_glyph(p_data, color_size, require_format, mw, mh, false);
	ERR_FAIL_COND_V(tex_pos.index < 0, FontGlyph());

	const uint8_t* img_src = hb_raster_image_get_buffer(p_image);

	// Fit character in char texture.
	ShelfPackTexture& tex = p_data->textures.write[tex_pos.index];

	{
		uint8_t* wr = tex.image->ptrw();

		for (int i = 0; i < h; i++) {
			for (int j = 0; j < w; j++) {
				int ofs = ((i + tex_pos.y + p_rect_margin * 2) * tex.texture_w + j + tex_pos.x +
							  p_rect_margin * 2) *
						  color_size;
				ERR_FAIL_COND_V(ofs >= tex.image->get_data_size(), FontGlyph());
				if (p_bgra) {
					int ofs_color = i * p_ext.stride + (j << 2);
					wr[ofs + 2] = img_src[ofs_color + 0];
					wr[ofs + 1] = img_src[ofs_color + 1];
					wr[ofs + 0] = img_src[ofs_color + 2];
					wr[ofs + 3] = img_src[ofs_color + 3];
				}
				else {
					wr[ofs + 0] = 255; // grayscale as 1
					wr[ofs + 1] = img_src[i * p_ext.stride + j];
				}
			}
		}
	}

	tex.dirty = true;

	chr.texture_idx = tex_pos.index;

	chr.uv_rect = Rect2(tex_pos.x + p_rect_margin, tex_pos.y + p_rect_margin, w + p_rect_margin * 2,
		h + p_rect_margin * 2);
	chr.rect.position =
		Vector2(p_ext.x_origin - p_rect_margin, p_ext.y_origin - p_rect_margin) * p_data->scale;
	chr.rect.size = chr.uv_rect.size * p_data->scale;
	return chr;
}
#endif

_FORCE_INLINE_ TextServerAdvanced::FontGlyph TextServerAdvanced::rasterize_bitmap(
	FontForSizeAdvanced* p_data, int p_rect_margin, FT_Bitmap p_bitmap, int p_yofs, int p_xofs,
	const Vector2& p_advance, bool p_bgra) const
{
	FontGlyph chr;
	chr.advance = p_advance * p_data->scale;
	chr.found = true;

	int w = p_bitmap.width;
	int h = p_bitmap.rows;

	if (w == 0 || h == 0 || p_bitmap.buffer == nullptr) {
		chr.texture_idx = -1;
		chr.uv_rect = Rect2();
		chr.rect = Rect2();
		return chr;
	}

	int color_size = 2;

	switch (p_bitmap.pixel_mode) {
	case FT_PIXEL_MODE_MONO:
	case FT_PIXEL_MODE_GRAY: {
		color_size = 2;
	} break;
	case FT_PIXEL_MODE_BGRA: {
		color_size = 4;
	} break;
	case FT_PIXEL_MODE_LCD: {
		color_size = 4;
		w /= 3;
	} break;
	case FT_PIXEL_MODE_LCD_V: {
		color_size = 4;
		h /= 3;
	} break;
	}

	int mw = w + p_rect_margin * 4;
	int mh = h + p_rect_margin * 4;

	ERR_FAIL_COND_V(mw > 4096, FontGlyph());
	ERR_FAIL_COND_V(mh > 4096, FontGlyph());

	Image::Format require_format = color_size == 4 ? Image::FORMAT_RGBA8 : Image::FORMAT_LA8;

	FontTexturePosition tex_pos =
		find_texture_pos_for_glyph(p_data, color_size, require_format, mw, mh, false);
	ERR_FAIL_COND_V(tex_pos.index < 0, FontGlyph());

	// Fit character in char texture.
	ShelfPackTexture& tex = p_data->textures.write[tex_pos.index];

	{
		uint8_t* wr = tex.image->ptrw();

		for (int i = 0; i < h; i++) {
			for (int j = 0; j < w; j++) {
				int ofs = ((i + tex_pos.y + p_rect_margin * 2) * tex.texture_w + j + tex_pos.x +
							  p_rect_margin * 2) *
						  color_size;
				ERR_FAIL_COND_V(ofs >= tex.image->get_data_size(), FontGlyph());
				switch (p_bitmap.pixel_mode) {
				case FT_PIXEL_MODE_MONO: {
					int byte = i * p_bitmap.pitch + (j >> 3);
					int bit = 1 << (7 - (j % 8));
					wr[ofs + 0] = 255; // grayscale as 1
					wr[ofs + 1] = (p_bitmap.buffer[byte] & bit) ? 255 : 0;
				} break;
				case FT_PIXEL_MODE_GRAY:
					wr[ofs + 0] = 255; // grayscale as 1
					wr[ofs + 1] = p_bitmap.buffer[i * p_bitmap.pitch + j];
					break;
				case FT_PIXEL_MODE_BGRA: {
					int ofs_color = i * p_bitmap.pitch + (j << 2);
					wr[ofs + 2] = p_bitmap.buffer[ofs_color + 0];
					wr[ofs + 1] = p_bitmap.buffer[ofs_color + 1];
					wr[ofs + 0] = p_bitmap.buffer[ofs_color + 2];
					wr[ofs + 3] = p_bitmap.buffer[ofs_color + 3];
				} break;
				case FT_PIXEL_MODE_LCD: {
					int ofs_color = i * p_bitmap.pitch + (j * 3);
					if (p_bgra) {
						wr[ofs + 0] = p_bitmap.buffer[ofs_color + 2];
						wr[ofs + 1] = p_bitmap.buffer[ofs_color + 1];
						wr[ofs + 2] = p_bitmap.buffer[ofs_color + 0];
						wr[ofs + 3] = 255;
					}
					else {
						wr[ofs + 0] = p_bitmap.buffer[ofs_color + 0];
						wr[ofs + 1] = p_bitmap.buffer[ofs_color + 1];
						wr[ofs + 2] = p_bitmap.buffer[ofs_color + 2];
						wr[ofs + 3] = 255;
					}
				} break;
				case FT_PIXEL_MODE_LCD_V: {
					int ofs_color = i * p_bitmap.pitch * 3 + j;
					if (p_bgra) {
						wr[ofs + 0] = p_bitmap.buffer[ofs_color + p_bitmap.pitch * 2];
						wr[ofs + 1] = p_bitmap.buffer[ofs_color + p_bitmap.pitch];
						wr[ofs + 2] = p_bitmap.buffer[ofs_color + 0];
						wr[ofs + 3] = 255;
					}
					else {
						wr[ofs + 0] = p_bitmap.buffer[ofs_color + 0];
						wr[ofs + 1] = p_bitmap.buffer[ofs_color + p_bitmap.pitch];
						wr[ofs + 2] = p_bitmap.buffer[ofs_color + p_bitmap.pitch * 2];
						wr[ofs + 3] = 255;
					}
				} break;
				default:
					ERR_FAIL_V_MSG(FontGlyph(), "Font uses unsupported pixel format: " +
													String::num_int64(p_bitmap.pixel_mode) + ".");
					break;
				}
			}
		}
	}

	tex.dirty = true;

	chr.texture_idx = tex_pos.index;

	chr.uv_rect = Rect2(tex_pos.x + p_rect_margin, tex_pos.y + p_rect_margin, w + p_rect_margin * 2,
		h + p_rect_margin * 2);
	chr.rect.position = Vector2(p_xofs - p_rect_margin, -p_yofs - p_rect_margin) * p_data->scale;
	chr.rect.size = chr.uv_rect.size * p_data->scale;
	return chr;
}
#endif

/*************************************************************************/
/* Font Cache                                                            */
/*************************************************************************/

bool TextServerAdvanced::_ensure_glyph(FontAdvanced* p_font_data, const Vector2i& p_size,
	int32_t p_glyph, FontGlyph& r_glyph, uint32_t p_oversampling) const
{
	FontForSizeAdvanced* fd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(p_font_data, p_size, fd, false, p_oversampling), false);

	int32_t glyph_index = p_glyph & 0xffffff; // Remove subpixel shifts.

	HashMap<int32_t, FontGlyph>::Iterator E = fd->glyph_map.find(p_glyph);
	if (E) {
		bool tx_valid = true;
		if (E->value.texture_idx >= 0) {
			if (E->value.texture_idx < fd->textures.size()) {
				tx_valid = fd->textures[E->value.texture_idx].image.is_valid();
			}
			else {
				tx_valid = false;
			}
		}
		if (tx_valid) {
			r_glyph = E->value;
			return E->value.found;
#ifdef DEBUG_ENABLED
		}
		else {
			WARN_PRINT(vformat("Invalid texture cache for glyph %x in font %s, glyph will be "
							   "re-rendered. Re-import this font to regenerate textures.",
				glyph_index, p_font_data->font_name));
#endif
		}
	}

	if (glyph_index == 0) { // Non graphical or invalid glyph, do not render.
		E = fd->glyph_map.insert(p_glyph, FontGlyph());
		r_glyph = E->value;
		return true;
	}

#ifdef MODULE_FREETYPE_ENABLED
	FontGlyph gl;
	if (p_font_data->face) {
		FT_Int32 flags = FT_LOAD_DEFAULT;

		bool outline = p_size.y > 0;
		switch (p_font_data->hinting) {
		case TextServer::HINTING_NONE:
			flags |= FT_LOAD_NO_HINTING;
			break;
		case TextServer::HINTING_LIGHT:
			flags |= FT_LOAD_TARGET_LIGHT;
			break;
		default:
			flags |= FT_LOAD_TARGET_NORMAL;
			break;
		}
		if (p_font_data->force_autohinter) {
			flags |= FT_LOAD_FORCE_AUTOHINT;
		}
		if (outline ||
			(p_font_data->disable_embedded_bitmaps && !FT_HAS_COLOR(p_font_data->face))) {
			flags |= FT_LOAD_NO_BITMAP;
		}
		else if (FT_HAS_COLOR(p_font_data->face)) {
			flags |= FT_LOAD_COLOR;
		}

		FT_Fixed v, h;
		FT_Get_Advance(p_font_data->face, glyph_index, flags, &h);
		FT_Get_Advance(p_font_data->face, glyph_index, flags | FT_LOAD_VERTICAL_LAYOUT, &v);

		int error = FT_Load_Glyph(p_font_data->face, glyph_index, flags);
		if (error) {
			E = fd->glyph_map.insert(p_glyph, FontGlyph());
			r_glyph = E->value;
			return false;
		}

		if (!p_font_data->msdf) {
			if ((p_font_data->subpixel_positioning == SUBPIXEL_POSITIONING_ONE_QUARTER) ||
				(p_font_data->subpixel_positioning == SUBPIXEL_POSITIONING_AUTO &&
					p_size.x <= SUBPIXEL_POSITIONING_ONE_QUARTER_MAX_SIZE * 64)) {
				FT_Pos xshift = (int)((p_glyph >> 27) & 3) << 4;
				FT_Outline_Translate(&p_font_data->face->glyph->outline, xshift, 0);
			}
			else if ((p_font_data->subpixel_positioning == SUBPIXEL_POSITIONING_ONE_HALF) ||
					   (p_font_data->subpixel_positioning == SUBPIXEL_POSITIONING_AUTO &&
						   p_size.x <= SUBPIXEL_POSITIONING_ONE_HALF_MAX_SIZE * 64)) {
				FT_Pos xshift = (int)((p_glyph >> 27) & 3) << 5;
				FT_Outline_Translate(&p_font_data->face->glyph->outline, xshift, 0);
			}
		}

		if (p_font_data->embolden != 0.f) {
			FT_Pos strength =
				p_font_data->embolden * p_size.x / 16; // 26.6 fractional units (1 / 64).
			FT_Outline_Embolden(&p_font_data->face->glyph->outline, strength);
		}

		if (p_font_data->transform != Transform2D()) {
			FT_Matrix mat = {FT_Fixed(p_font_data->transform[0][0] * 65536),
				FT_Fixed(p_font_data->transform[0][1] * 65536),
				FT_Fixed(p_font_data->transform[1][0] * 65536),
				FT_Fixed(
					p_font_data->transform[1][1] * 65536)}; // 16.16 fractional units (1 / 65536).
			FT_Outline_Transform(&p_font_data->face->glyph->outline, &mat);
		}

		FT_Render_Mode aa_mode = FT_RENDER_MODE_NORMAL;
		bool bgra = false;
		switch (p_font_data->antialiasing) {
		case FONT_ANTIALIASING_NONE: {
			aa_mode = FT_RENDER_MODE_MONO;
		} break;
		case FONT_ANTIALIASING_GRAY: {
			aa_mode = FT_RENDER_MODE_NORMAL;
		} break;
		case FONT_ANTIALIASING_LCD: {
			int aa_layout = (int)((p_glyph >> 24) & 7);
			switch (aa_layout) {
			case FONT_LCD_SUBPIXEL_LAYOUT_HRGB: {
				aa_mode = FT_RENDER_MODE_LCD;
				bgra = false;
			} break;
			case FONT_LCD_SUBPIXEL_LAYOUT_HBGR: {
				aa_mode = FT_RENDER_MODE_LCD;
				bgra = true;
			} break;
			case FONT_LCD_SUBPIXEL_LAYOUT_VRGB: {
				aa_mode = FT_RENDER_MODE_LCD_V;
				bgra = false;
			} break;
			case FONT_LCD_SUBPIXEL_LAYOUT_VBGR: {
				aa_mode = FT_RENDER_MODE_LCD_V;
				bgra = true;
			} break;
			default: {
				aa_mode = FT_RENDER_MODE_NORMAL;
			} break;
			}
		} break;
		}

		FT_GlyphSlot slot = p_font_data->face->glyph;
		bool fix_edge =
			(slot->format == FT_GLYPH_FORMAT_SVG); // Need to check before FT_Render_Glyph as it
												   // will change format to bitmap.
#if HB_VERSION_ATLEAST(13, 0, 0)
		bool from_bitmap = (slot->format == FT_GLYPH_FORMAT_BITMAP);
#endif
		if (!outline) {
			if (p_font_data->msdf) {
#ifndef MODULE_MSDFGEN_ENABLED
				fd->glyph_map[p_glyph] = FontGlyph();
				ERR_FAIL_V_MSG(false, "Compiled without MSDFGEN support!");
#endif
			}
			else {
#if HB_VERSION_ATLEAST(13, 0, 0)
				if (p_font_data->hb_rdr && p_font_data->hb_mono &&
					p_font_data->antialiasing == FONT_ANTIALIASING_GRAY && !from_bitmap) {
					bool is_rasterized = false;
					float xshift = 0.0;
					if ((p_font_data->subpixel_positioning == SUBPIXEL_POSITIONING_ONE_QUARTER) ||
						(p_font_data->subpixel_positioning == SUBPIXEL_POSITIONING_AUTO &&
							p_size.x <= SUBPIXEL_POSITIONING_ONE_QUARTER_MAX_SIZE * 64)) {
						xshift = float((int)((p_glyph >> 27) & 3) << 4);
					}
					else if ((p_font_data->subpixel_positioning ==
								   SUBPIXEL_POSITIONING_ONE_HALF) ||
							   (p_font_data->subpixel_positioning == SUBPIXEL_POSITIONING_AUTO &&
								   p_size.x <= SUBPIXEL_POSITIONING_ONE_HALF_MAX_SIZE * 64)) {
						xshift = float((int)((p_glyph >> 27) & 3) << 5);
					}
					xshift += (p_font_data->embolden * double(p_size.x) / 64.0);
					if (fd->color_paint) {
						hb_raster_paint_reset(p_font_data->hb_rdr);
						hb_raster_paint_set_scale_factor(p_font_data->hb_rdr, 64.0, 64.0);
						if (Math::is_equal_approx(p_font_data->transform[0][0], (real_t)1.f) &&
							Math::is_equal_approx(p_font_data->transform[1][0], (real_t)0.f) &&
							Math::is_equal_approx(p_font_data->transform[1][1], (real_t)1.f)) {
							hb_raster_paint_set_transform(
								p_font_data->hb_rdr, 1.f, 0.f, 0.f, -1.f, xshift, 0.f);
						}
						else {
							Transform2D tr = p_font_data->transform * Transform2D::FLIP_Y;
							hb_raster_paint_set_transform(p_font_data->hb_rdr, tr[0][0], tr[1][0],
								tr[0][1], tr[1][1], xshift, 0.f);
						}
						hb_raster_paint_clear_custom_palette_colors(p_font_data->hb_rdr);
						if (!p_font_data->palette_custom_colors_hb.is_empty()) {
							for (int col = 0; col < p_font_data->palette_custom_colors_hb.size();
								 col++) {
								if (p_font_data->palette_custom_colors_hb[col] != 0) {
									hb_raster_paint_set_custom_palette_color(p_font_data->hb_rdr,
										col, p_font_data->palette_custom_colors_hb[col]);
								}
							}
						}
#if HB_VERSION_ATLEAST(14, 2, 0)
						hb_raster_paint_set_palette(
							p_font_data->hb_rdr, p_font_data->palette_index);
						bool ok = hb_raster_paint_glyph_or_fail(
							p_font_data->hb_rdr, fd->hb_handle, (hb_codepoint_t)glyph_index);
#else
						bool ok = hb_raster_paint_glyph(p_font_data->hb_rdr, fd->hb_handle,
							(hb_codepoint_t)glyph_index, 0, 0, p_font_data->palette_index,
							(hb_color_t)0xFFFFFFFF);
#endif
						if (ok) {
							is_rasterized = true;
							fix_edge = false;
							hb_raster_image_t* img = hb_raster_paint_render(p_font_data->hb_rdr);
							hb_raster_extents_t ext = {0, 0, 0, 0, 0};
							if (img) {
								hb_raster_image_get_extents(img, &ext);
								gl = rasterize_hb_bitmap(fd, rect_range, img, ext,
									Vector2((h + (1 << 9)) >> 10, (v + (1 << 9)) >> 10) / 64.0,
									true);
								hb_raster_paint_recycle_image(p_font_data->hb_rdr, img);
							}
							else {
								gl = rasterize_hb_bitmap(fd, rect_range, nullptr, ext,
									Vector2((h + (1 << 9)) >> 10, (v + (1 << 9)) >> 10) / 64.0,
									true);
							}
						}
					}
					if (!is_rasterized && !fix_edge &&
						p_font_data->hinting == TextServer::HINTING_NONE) {
						hb_raster_draw_reset(p_font_data->hb_mono);
						hb_raster_draw_set_scale_factor(p_font_data->hb_mono, 64.0, 64.0);
						if (Math::is_equal_approx(p_font_data->transform[0][0], (real_t)1.f) &&
							Math::is_equal_approx(p_font_data->transform[1][0], (real_t)0.f) &&
							Math::is_equal_approx(p_font_data->transform[1][1], (real_t)1.f)) {
							hb_raster_draw_set_transform(
								p_font_data->hb_mono, 1.f, 0.f, 0.f, -1.f, xshift, 0.f);
						}
						else {
							Transform2D tr = p_font_data->transform * Transform2D::FLIP_Y;
							hb_raster_draw_set_transform(p_font_data->hb_mono, tr[0][0], tr[1][0],
								tr[0][1], tr[1][1], xshift, 0.f);
						}
#if HB_VERSION_ATLEAST(14, 2, 0)
						hb_raster_draw_glyph(
							p_font_data->hb_mono, fd->hb_handle, (hb_codepoint_t)glyph_index);
#else
						hb_raster_draw_glyph(
							p_font_data->hb_mono, fd->hb_handle, (hb_codepoint_t)glyph_index, 0, 0);
#endif
						hb_raster_image_t* img = hb_raster_draw_render(p_font_data->hb_mono);
						hb_raster_extents_t ext = {0, 0, 0, 0, 0};
						if (img) {
							is_rasterized = true;
							hb_raster_image_get_extents(img, &ext);
							gl = rasterize_hb_bitmap(fd, rect_range, img, ext,
								Vector2((h + (1 << 9)) >> 10, (v + (1 << 9)) >> 10) / 64.0, false);
							hb_raster_draw_recycle_image(p_font_data->hb_mono, img);
						}
						else {
							gl = rasterize_hb_bitmap(fd, rect_range, nullptr, ext,
								Vector2((h + (1 << 9)) >> 10, (v + (1 << 9)) >> 10) / 64.0, false);
						}
					}
					if (!is_rasterized) {
						error = FT_Render_Glyph(slot, aa_mode);
						if (!error) {
							gl = rasterize_bitmap(fd, rect_range, slot->bitmap, slot->bitmap_top,
								slot->bitmap_left,
								Vector2((h + (1 << 9)) >> 10, (v + (1 << 9)) >> 10) / 64.0, bgra);
						}
					}
				}
				else {
#else
				{
#endif
					error = FT_Render_Glyph(slot, aa_mode);
					if (!error) {
						gl = rasterize_bitmap(fd, rect_range, slot->bitmap, slot->bitmap_top,
							slot->bitmap_left,
							Vector2((h + (1 << 9)) >> 10, (v + (1 << 9)) >> 10) / 64.0, bgra);
					}
				}
			}
		}
		else {
			FT_Stroker stroker;
			if (FT_Stroker_New(ft_library, &stroker) != 0) {
				fd->glyph_map[p_glyph] = FontGlyph();
				ERR_FAIL_V_MSG(false, "FreeType: Failed to load glyph stroker.");
			}

			FT_Stroker_Set(stroker, (int)(fd->size.y * 16.0), FT_STROKER_LINECAP_BUTT,
				FT_STROKER_LINEJOIN_ROUND, 0);
			FT_Glyph glyph;
			FT_BitmapGlyph glyph_bitmap;

			if (FT_Get_Glyph(p_font_data->face->glyph, &glyph) != 0) {
				goto cleanup_stroker;
			}
			if (FT_Glyph_Stroke(&glyph, stroker, 1) != 0) {
				goto cleanup_glyph;
			}
			if (FT_Glyph_To_Bitmap(&glyph, aa_mode, nullptr, 1) != 0) {
				goto cleanup_glyph;
			}
			glyph_bitmap = (FT_BitmapGlyph)glyph;
			gl = rasterize_bitmap(fd, rect_range, glyph_bitmap->bitmap, glyph_bitmap->top,
				glyph_bitmap->left, Vector2(), bgra);

		cleanup_glyph:
			FT_Done_Glyph(glyph);
		cleanup_stroker:
			FT_Stroker_Done(stroker);
		}
		gl.fix_edge = fix_edge;
		E = fd->glyph_map.insert(p_glyph, gl);
		r_glyph = E->value;
		return gl.found;
	}
#endif
	E = fd->glyph_map.insert(p_glyph, FontGlyph());
	r_glyph = E->value;
	return false;
}

void TextServerAdvanced::_reference_oversampling_level(double p_oversampling)
{
	uint32_t oversampling = CLAMP(p_oversampling, 0.1, 100.0) * 64;
	if (oversampling == 64) {
		return;
	}
	OversamplingLevel* ol = oversampling_levels.getptr(oversampling);
	if (ol) {
		ol->refcount++;
	}
	else {
		OversamplingLevel new_ol;
		oversampling_levels.insert(oversampling, new_ol);
	}
}

void TextServerAdvanced::_unreference_oversampling_level(double p_oversampling)
{
	uint32_t oversampling = CLAMP(p_oversampling, 0.1, 100.0) * 64;
	if (oversampling == 64) {
		return;
	}
	OversamplingLevel* ol = oversampling_levels.getptr(oversampling);
	if (ol) {
		ol->refcount--;
		if (ol->refcount == 0) {
			for (FontForSizeAdvanced* fd : ol->fonts) {
				fd->owner->cache.erase(fd->size);
				memdelete(fd);
			}
			ol->fonts.clear();
			oversampling_levels.erase(oversampling);
		}
	}
}

_FORCE_INLINE_ bool TextServerAdvanced::_font_validate(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, false);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, 16);
	FontForSizeAdvanced* ffsd = nullptr;
	return _ensure_cache_for_size(fd, size, ffsd, true);
}

bool TextServerAdvanced::_font_is_color(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, false);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, 16);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), false);
#ifdef MODULE_FREETYPE_ENABLED
	return fd->face && FT_HAS_COLOR(fd->face);
#else
	return false;
#endif
}

hb_font_t* TextServerAdvanced::_font_get_hb_handle(
	const RID& p_font_rid, int64_t p_size, bool& r_is_color) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, nullptr);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), nullptr);
#ifdef MODULE_FREETYPE_ENABLED
	r_is_color = fd->face && FT_HAS_COLOR(fd->face);
#else
	r_is_color = false;
#endif

	return ffsd->hb_handle;
}

RID TextServerAdvanced::_create_font()
{
	_THREAD_SAFE_METHOD_

	FontAdvanced* fd = memnew(FontAdvanced);

	return font_owner.make_rid(fd);
}

RID TextServerAdvanced::_create_font_linked_variation(const RID& p_font_rid)
{
	_THREAD_SAFE_METHOD_

	RID rid = p_font_rid;
	FontAdvancedLinkedVariation* fdv = font_var_owner.get_or_null(rid);
	if (unlikely(fdv)) {
		rid = fdv->base_font;
	}
	ERR_FAIL_COND_V(!font_owner.owns(rid), RID());

	FontAdvancedLinkedVariation* new_fdv = memnew(FontAdvancedLinkedVariation);
	new_fdv->base_font = rid;

	return font_var_owner.make_rid(new_fdv);
}

void TextServerAdvanced::_font_set_data(const RID& p_font_rid, const PackedByteArray& p_data)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	fd->data = p_data;
	fd->data_ptr = fd->data.ptr();
	fd->data_size = fd->data.size();
}

void TextServerAdvanced::_font_set_data_ptr(
	const RID& p_font_rid, const uint8_t* p_data_ptr, int64_t p_data_size)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	fd->data.resize(0);
	fd->data_ptr = p_data_ptr;
	fd->data_size = p_data_size;
}

void TextServerAdvanced::_font_set_face_index(const RID& p_font_rid, int64_t p_face_index)
{
	ERR_FAIL_COND(p_face_index < 0);
	ERR_FAIL_COND(p_face_index >= 0x7FFF);

	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	if (fd->face_index != p_face_index) {
		fd->face_index = p_face_index;
	}
}

int64_t TextServerAdvanced::_font_get_face_index(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, 0);

	MutexLock lock(fd->mutex);
	return fd->face_index;
}

int64_t TextServerAdvanced::_font_get_face_count(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, 0);

	MutexLock lock(fd->mutex);
	int face_count = 0;

	if (fd->data_ptr && (fd->data_size > 0)) {
		// Init dynamic font.
#ifdef MODULE_FREETYPE_ENABLED
		int error = 0;
		if (!ft_library) {
			error = FT_Init_FreeType(&ft_library);
			ERR_FAIL_COND_V_MSG(error != 0, false,
				"FreeType: Error initializing library: '" + String(FT_Error_String(error)) + "'.");
#ifdef MODULE_SVG_ENABLED
			FT_Property_Set(ft_library, "ot-svg", "svg-hooks", get_tvg_svg_in_ot_hooks());
#endif
		}

		FT_StreamRec stream;
		memset(&stream, 0, sizeof(FT_StreamRec));
		stream.base = (unsigned char*)fd->data_ptr;
		stream.size = fd->data_size;
		stream.pos = 0;

		FT_Open_Args fargs;
		memset(&fargs, 0, sizeof(FT_Open_Args));
		fargs.memory_base = (unsigned char*)fd->data_ptr;
		fargs.memory_size = fd->data_size;
		fargs.flags = FT_OPEN_MEMORY;
		fargs.stream = &stream;

		MutexLock ftlock(ft_mutex);

		FT_Face tmp_face = nullptr;
		error = FT_Open_Face(ft_library, &fargs, -1, &tmp_face);
		if (error == 0) {
			face_count = tmp_face->num_faces;
			FT_Done_Face(tmp_face);
		}
#endif
	}

	return face_count;
}

void TextServerAdvanced::_font_set_style(const RID& p_font_rid, uint32_t p_style)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, 16);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));
	fd->style_flags = p_style;
}

uint32_t TextServerAdvanced::_font_get_style(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, 0);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, 16);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), 0);
	return fd->style_flags;
}

void TextServerAdvanced::_font_set_style_name(const RID& p_font_rid, const String& p_name)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, 16);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));
	fd->style_name = p_name;
}

String TextServerAdvanced::_font_get_style_name(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, String());

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, 16);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), String());
	return fd->style_name;
}

void TextServerAdvanced::_font_set_weight(const RID& p_font_rid, int64_t p_weight)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, 16);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));
	fd->weight = CLAMP(p_weight, 100, 999);
}

int64_t TextServerAdvanced::_font_get_weight(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, 400);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, 16);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), 400);
	return fd->weight;
}

void TextServerAdvanced::_font_set_stretch(const RID& p_font_rid, int64_t p_stretch)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, 16);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));
	fd->stretch = CLAMP(p_stretch, 50, 200);
}

int64_t TextServerAdvanced::_font_get_stretch(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, 100);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, 16);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), 100);
	return fd->stretch;
}

void TextServerAdvanced::_font_set_name(const RID& p_font_rid, const String& p_name)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, 16);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));
	fd->font_name = p_name;
}

String TextServerAdvanced::_font_get_name(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, String());

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, 16);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), String());
	return fd->font_name;
}

void TextServerAdvanced::_font_set_antialiasing(
	const RID& p_font_rid, TextServer::FontAntialiasing p_antialiasing)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	if (fd->antialiasing != p_antialiasing) {
		fd->antialiasing = p_antialiasing;
	}
}

TextServer::FontAntialiasing TextServerAdvanced::_font_get_antialiasing(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, TextServer::FONT_ANTIALIASING_NONE);

	MutexLock lock(fd->mutex);
	return fd->antialiasing;
}

void TextServerAdvanced::_font_set_disable_embedded_bitmaps(
	const RID& p_font_rid, bool p_disable_embedded_bitmaps)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	if (fd->disable_embedded_bitmaps != p_disable_embedded_bitmaps) {
		fd->disable_embedded_bitmaps = p_disable_embedded_bitmaps;
	}
}

bool TextServerAdvanced::_font_get_disable_embedded_bitmaps(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, false);

	MutexLock lock(fd->mutex);
	return fd->disable_embedded_bitmaps;
}

void TextServerAdvanced::_font_set_generate_mipmaps(const RID& p_font_rid, bool p_generate_mipmaps)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	if (fd->mipmaps != p_generate_mipmaps) {
		for (KeyValue<Vector2i, FontForSizeAdvanced*>& E : fd->cache) {
			for (int i = 0; i < E.value->textures.size(); i++) {
				E.value->textures.write[i].dirty = true;
				E.value->textures.write[i].texture = Ref<ImageTexture>();
			}
		}
		fd->mipmaps = p_generate_mipmaps;
	}
}

bool TextServerAdvanced::_font_get_generate_mipmaps(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, false);

	MutexLock lock(fd->mutex);
	return fd->mipmaps;
}

void TextServerAdvanced::_font_set_multichannel_signed_distance_field(
	const RID& p_font_rid, bool p_msdf)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	if (fd->msdf != p_msdf) {
		fd->msdf = p_msdf;
	}
}

bool TextServerAdvanced::_font_is_multichannel_signed_distance_field(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, false);

	MutexLock lock(fd->mutex);
	return fd->msdf;
}

void TextServerAdvanced::_font_set_msdf_pixel_range(
	const RID& p_font_rid, int64_t p_msdf_pixel_range)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	if (fd->msdf_range != p_msdf_pixel_range) {
		fd->msdf_range = p_msdf_pixel_range;
	}
}

int64_t TextServerAdvanced::_font_get_msdf_pixel_range(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, false);

	MutexLock lock(fd->mutex);
	return fd->msdf_range;
}

void TextServerAdvanced::_font_set_msdf_size(const RID& p_font_rid, int64_t p_msdf_size)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	if (fd->msdf_source_size != p_msdf_size) {
		fd->msdf_source_size = p_msdf_size;
	}
}

int64_t TextServerAdvanced::_font_get_msdf_size(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, 0);

	MutexLock lock(fd->mutex);
	return fd->msdf_source_size;
}

void TextServerAdvanced::_font_set_fixed_size(const RID& p_font_rid, int64_t p_fixed_size)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	fd->fixed_size = p_fixed_size;
}

int64_t TextServerAdvanced::_font_get_fixed_size(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, 0);

	MutexLock lock(fd->mutex);
	return fd->fixed_size;
}

void TextServerAdvanced::_font_set_fixed_size_scale_mode(
	const RID& p_font_rid, TextServer::FixedSizeScaleMode p_fixed_size_scale_mode)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	fd->fixed_size_scale_mode = p_fixed_size_scale_mode;
}

TextServer::FixedSizeScaleMode TextServerAdvanced::_font_get_fixed_size_scale_mode(
	const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, FIXED_SIZE_SCALE_DISABLE);

	MutexLock lock(fd->mutex);
	return fd->fixed_size_scale_mode;
}

void TextServerAdvanced::_font_set_allow_system_fallback(
	const RID& p_font_rid, bool p_allow_system_fallback)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	fd->allow_system_fallback = p_allow_system_fallback;
}

bool TextServerAdvanced::_font_is_allow_system_fallback(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, false);

	MutexLock lock(fd->mutex);
	return fd->allow_system_fallback;
}

void TextServerAdvanced::_font_set_force_autohinter(const RID& p_font_rid, bool p_force_autohinter)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	if (fd->force_autohinter != p_force_autohinter) {
		fd->force_autohinter = p_force_autohinter;
	}
}

bool TextServerAdvanced::_font_is_force_autohinter(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, false);

	MutexLock lock(fd->mutex);
	return fd->force_autohinter;
}

void TextServerAdvanced::_font_set_modulate_color_glyphs(const RID& p_font_rid, bool p_modulate)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	if (fd->modulate_color_glyphs != p_modulate) {
		fd->modulate_color_glyphs = p_modulate;
	}
}

bool TextServerAdvanced::_font_is_modulate_color_glyphs(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, false);

	MutexLock lock(fd->mutex);
	return fd->modulate_color_glyphs;
}

int64_t TextServerAdvanced::_font_get_palette_count(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, 0);

#if HB_VERSION_ATLEAST(13, 0, 0)
	return fd->palette_names.size();
#else
	return 0;
#endif
}

String TextServerAdvanced::_font_get_palette_name(const RID& p_font_rid, int64_t p_index) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, String());

#if HB_VERSION_ATLEAST(13, 0, 0)
	ERR_FAIL_INDEX_V(p_index, fd->palette_names.size(), String());
	return fd->palette_names[p_index];
#else
	return String();
#endif
}

Vector<Color> TextServerAdvanced::_font_get_palette_colors(
	const RID& p_font_rid, int64_t p_index) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, Vector<Color>());

#if HB_VERSION_ATLEAST(13, 0, 0)
	ERR_FAIL_INDEX_V(p_index, fd->palette_names.size(), Vector<Color>());
	return fd->palette_colors[p_index];
#else
	return Vector<Color>();
#endif
}

void TextServerAdvanced::_font_set_palette_custom_colors(
	const RID& p_font_rid, const Vector<Color>& p_colors)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

#if HB_VERSION_ATLEAST(13, 0, 0)
	if (fd->palette_custom_colors != p_colors) {
		fd->palette_custom_colors = p_colors;
		fd->palette_custom_colors_hb.resize_uninitialized(p_colors.size());
		for (int col = 0; col < fd->palette_custom_colors.size(); col++) {
			const Color& c = fd->palette_custom_colors[col];
			fd->palette_custom_colors_hb.write[col] =
				HB_COLOR(c.get_b8(), c.get_g8(), c.get_r8(), c.get_a8());
		}
	}
#endif
}

Vector<Color> TextServerAdvanced::_font_get_palette_custom_colors(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, Vector<Color>());

#if HB_VERSION_ATLEAST(13, 0, 0)
	return fd->palette_custom_colors;
#else
	return Vector<Color>();
#endif
}

int64_t TextServerAdvanced::_font_get_used_palette(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, 0);
#if HB_VERSION_ATLEAST(13, 0, 0)
	return fd->palette_index;
#else
	return 0;
#endif
}

void TextServerAdvanced::_font_set_used_palette(const RID& p_font_rid, int64_t p_index)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);
#if HB_VERSION_ATLEAST(13, 0, 0)
	if (fd->palette_index != p_index) {
		fd->palette_index = p_index;
	}
#endif
}

void TextServerAdvanced::_font_set_hinting(const RID& p_font_rid, TextServer::Hinting p_hinting)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	if (fd->hinting != p_hinting) {
		fd->hinting = p_hinting;
	}
}

TextServer::Hinting TextServerAdvanced::_font_get_hinting(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, HINTING_NONE);

	MutexLock lock(fd->mutex);
	return fd->hinting;
}

void TextServerAdvanced::_font_set_subpixel_positioning(
	const RID& p_font_rid, TextServer::SubpixelPositioning p_subpixel)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	fd->subpixel_positioning = p_subpixel;
}

TextServer::SubpixelPositioning TextServerAdvanced::_font_get_subpixel_positioning(
	const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, SUBPIXEL_POSITIONING_DISABLED);

	MutexLock lock(fd->mutex);
	return fd->subpixel_positioning;
}

void TextServerAdvanced::_font_set_keep_rounding_remainders(
	const RID& p_font_rid, bool p_keep_rounding_remainders)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	fd->keep_rounding_remainders = p_keep_rounding_remainders;
}

bool TextServerAdvanced::_font_get_keep_rounding_remainders(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, false);

	MutexLock lock(fd->mutex);
	return fd->keep_rounding_remainders;
}

void TextServerAdvanced::_font_set_embolden(const RID& p_font_rid, double p_strength)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	if (fd->embolden != p_strength) {
		fd->embolden = p_strength;
	}
}

double TextServerAdvanced::_font_get_embolden(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, 0.0);

	MutexLock lock(fd->mutex);
	return fd->embolden;
}

void TextServerAdvanced::_font_set_spacing(
	const RID& p_font_rid, SpacingType p_spacing, int64_t p_value)
{
	ERR_FAIL_INDEX((int)p_spacing, 4);
	FontAdvancedLinkedVariation* fdv = font_var_owner.get_or_null(p_font_rid);
	if (fdv) {
		if (fdv->extra_spacing[p_spacing] != p_value) {
			fdv->extra_spacing[p_spacing] = p_value;
		}
	}
	else {
		FontAdvanced* fd = font_owner.get_or_null(p_font_rid);
		ERR_FAIL_NULL(fd);

		MutexLock lock(fd->mutex);
		if (fd->extra_spacing[p_spacing] != p_value) {
			fd->extra_spacing[p_spacing] = p_value;
		}
	}
}

int64_t TextServerAdvanced::_font_get_spacing(const RID& p_font_rid, SpacingType p_spacing) const
{
	ERR_FAIL_INDEX_V((int)p_spacing, 4, 0);
	FontAdvancedLinkedVariation* fdv = font_var_owner.get_or_null(p_font_rid);
	if (fdv) {
		return fdv->extra_spacing[p_spacing];
	}
	else {
		FontAdvanced* fd = font_owner.get_or_null(p_font_rid);
		ERR_FAIL_NULL_V(fd, 0);

		MutexLock lock(fd->mutex);
		return fd->extra_spacing[p_spacing];
	}
}

void TextServerAdvanced::_font_set_baseline_offset(const RID& p_font_rid, double p_baseline_offset)
{
	FontAdvancedLinkedVariation* fdv = font_var_owner.get_or_null(p_font_rid);
	if (fdv) {
		if (fdv->baseline_offset != p_baseline_offset) {
			fdv->baseline_offset = p_baseline_offset;
		}
	}
	else {
		FontAdvanced* fd = font_owner.get_or_null(p_font_rid);
		ERR_FAIL_NULL(fd);

		MutexLock lock(fd->mutex);
		if (fd->baseline_offset != p_baseline_offset) {
			fd->baseline_offset = p_baseline_offset;
		}
	}
}

double TextServerAdvanced::_font_get_baseline_offset(const RID& p_font_rid) const
{
	FontAdvancedLinkedVariation* fdv = font_var_owner.get_or_null(p_font_rid);
	if (fdv) {
		return fdv->baseline_offset;
	}
	else {
		FontAdvanced* fd = font_owner.get_or_null(p_font_rid);
		ERR_FAIL_NULL_V(fd, 0.0);

		MutexLock lock(fd->mutex);
		return fd->baseline_offset;
	}
}

void TextServerAdvanced::font_set_transform(const RID& p_font_rid, const Transform2D& p_transform)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	if (fd->transform != p_transform) {
		fd->transform = p_transform;
	}
}

Transform2D TextServerAdvanced::_font_get_transform(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, Transform2D());

	MutexLock lock(fd->mutex);
	return fd->transform;
}

double TextServerAdvanced::_font_get_oversampling(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, -1.0);

	MutexLock lock(fd->mutex);
	return fd->oversampling_override;
}

void TextServerAdvanced::_font_set_oversampling(const RID& p_font_rid, double p_oversampling)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	if (fd->oversampling_override != p_oversampling) {
		fd->oversampling_override = p_oversampling;
	}
}

void TextServerAdvanced::_font_clear_size_cache(const RID& p_font_rid)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	MutexLock ftlock(ft_mutex);
	for (const KeyValue<Vector2i, FontForSizeAdvanced*>& E : fd->cache) {
		if (E.value->viewport_oversampling != 0) {
			OversamplingLevel* ol = oversampling_levels.getptr(E.value->viewport_oversampling);
			if (ol) {
				ol->fonts.erase(E.value);
			}
		}
		memdelete(E.value);
	}
	fd->cache.clear();
}

void TextServerAdvanced::_font_remove_size_cache(const RID& p_font_rid, const Vector2i& p_size)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	MutexLock ftlock(ft_mutex);
	Vector2i size = Vector2i(p_size.x * 64, p_size.y);
	if (fd->cache.has(size)) {
		if (fd->cache[size]->viewport_oversampling != 0) {
			OversamplingLevel* ol =
				oversampling_levels.getptr(fd->cache[size]->viewport_oversampling);
			if (ol) {
				ol->fonts.erase(fd->cache[size]);
			}
		}
		memdelete(fd->cache[size]);
		fd->cache.erase(size);
	}
}

void TextServerAdvanced::_font_set_ascent(const RID& p_font_rid, int64_t p_size, double p_ascent)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));
	ffsd->ascent = p_ascent;
}

double TextServerAdvanced::_font_get_ascent(const RID& p_font_rid, int64_t p_size) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, 0.0);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), 0.0);

	if (fd->msdf) {
		return ffsd->ascent * (double)p_size / (double)fd->msdf_source_size;
	}
	else if (fd->fixed_size > 0 && fd->fixed_size_scale_mode != FIXED_SIZE_SCALE_DISABLE &&
			   size.x != p_size * 64) {
		if (fd->fixed_size_scale_mode == FIXED_SIZE_SCALE_ENABLED) {
			return ffsd->ascent * (double)p_size / (double)fd->fixed_size;
		}
		else {
			return ffsd->ascent * Math::round((double)p_size / (double)fd->fixed_size);
		}
	}
	else {
		return ffsd->ascent;
	}
}

void TextServerAdvanced::_font_set_descent(const RID& p_font_rid, int64_t p_size, double p_descent)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	Vector2i size = _get_size(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));
	ffsd->descent = p_descent;
}

double TextServerAdvanced::_font_get_descent(const RID& p_font_rid, int64_t p_size) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, 0.0);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), 0.0);

	if (fd->msdf) {
		return ffsd->descent * (double)p_size / (double)fd->msdf_source_size;
	}
	else if (fd->fixed_size > 0 && fd->fixed_size_scale_mode != FIXED_SIZE_SCALE_DISABLE &&
			   size.x != p_size * 64) {
		if (fd->fixed_size_scale_mode == FIXED_SIZE_SCALE_ENABLED) {
			return ffsd->descent * (double)p_size / (double)fd->fixed_size;
		}
		else {
			return ffsd->descent * Math::round((double)p_size / (double)fd->fixed_size);
		}
	}
	else {
		return ffsd->descent;
	}
}

void TextServerAdvanced::_font_set_underline_position(
	const RID& p_font_rid, int64_t p_size, double p_underline_position)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));
	ffsd->underline_position = p_underline_position;
}

double TextServerAdvanced::_font_get_underline_position(const RID& p_font_rid, int64_t p_size) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, 0.0);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), 0.0);

	if (fd->msdf) {
		return ffsd->underline_position * (double)p_size / (double)fd->msdf_source_size;
	}
	else if (fd->fixed_size > 0 && fd->fixed_size_scale_mode != FIXED_SIZE_SCALE_DISABLE &&
			   size.x != p_size * 64) {
		if (fd->fixed_size_scale_mode == FIXED_SIZE_SCALE_ENABLED) {
			return ffsd->underline_position * (double)p_size / (double)fd->fixed_size;
		}
		else {
			return ffsd->underline_position * Math::round((double)p_size / (double)fd->fixed_size);
		}
	}
	else {
		return ffsd->underline_position;
	}
}

void TextServerAdvanced::_font_set_underline_thickness(
	const RID& p_font_rid, int64_t p_size, double p_underline_thickness)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));
	ffsd->underline_thickness = p_underline_thickness;
}

double TextServerAdvanced::_font_get_underline_thickness(
	const RID& p_font_rid, int64_t p_size) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, 0.0);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), 0.0);

	if (fd->msdf) {
		return ffsd->underline_thickness * (double)p_size / (double)fd->msdf_source_size;
	}
	else if (fd->fixed_size > 0 && fd->fixed_size_scale_mode != FIXED_SIZE_SCALE_DISABLE &&
			   size.x != p_size * 64) {
		if (fd->fixed_size_scale_mode == FIXED_SIZE_SCALE_ENABLED) {
			return ffsd->underline_thickness * (double)p_size / (double)fd->fixed_size;
		}
		else {
			return ffsd->underline_thickness * Math::round((double)p_size / (double)fd->fixed_size);
		}
	}
	else {
		return ffsd->underline_thickness;
	}
}

void TextServerAdvanced::_font_set_scale(const RID& p_font_rid, int64_t p_size, double p_scale)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));

#ifdef MODULE_FREETYPE_ENABLED
	if (fd->face) {
		return; // Do not override scale for dynamic fonts, it's calculated automatically.
	}
#endif
	ffsd->scale = p_scale;
}

double TextServerAdvanced::_font_get_scale(const RID& p_font_rid, int64_t p_size) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, 0.0);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), 0.0);

	if (fd->msdf) {
		return ffsd->scale * (double)p_size / (double)fd->msdf_source_size;
	}
	else if (fd->fixed_size > 0 && fd->fixed_size_scale_mode != FIXED_SIZE_SCALE_DISABLE &&
			   size.x != p_size * 64) {
		if (fd->fixed_size_scale_mode == FIXED_SIZE_SCALE_ENABLED) {
			return ffsd->scale * (double)p_size / (double)fd->fixed_size;
		}
		else {
			return ffsd->scale * Math::round((double)p_size / (double)fd->fixed_size);
		}
	}
	else {
		return ffsd->scale;
	}
}

int64_t TextServerAdvanced::_font_get_texture_count(
	const RID& p_font_rid, const Vector2i& p_size) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, 0);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), 0);

	return ffsd->textures.size();
}

void TextServerAdvanced::_font_clear_textures(const RID& p_font_rid, const Vector2i& p_size)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);
	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));
	ffsd->textures.clear();
}

void TextServerAdvanced::_font_remove_texture(
	const RID& p_font_rid, const Vector2i& p_size, int64_t p_texture_index)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));
	ERR_FAIL_INDEX(p_texture_index, ffsd->textures.size());

	ffsd->textures.remove_at(p_texture_index);
}

void TextServerAdvanced::_font_set_texture_image(const RID& p_font_rid, const Vector2i& p_size,
	int64_t p_texture_index, const Ref<Image>& p_image)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);
	ERR_FAIL_COND(p_image.is_null());

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));
	ERR_FAIL_COND(p_texture_index < 0);
	if (p_texture_index >= ffsd->textures.size()) {
		ffsd->textures.resize(p_texture_index + 1);
	}

	ShelfPackTexture& tex = ffsd->textures.write[p_texture_index];

	tex.image = p_image;
	tex.texture_w = p_image->get_width();
	tex.texture_h = p_image->get_height();

	Ref<Image> img = p_image;
	tex.texture = ImageTexture::create_from_image(img);
	tex.dirty = false;
}

Ref<Image> TextServerAdvanced::_font_get_texture_image(
	const RID& p_font_rid, const Vector2i& p_size, int64_t p_texture_index) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, Ref<Image>());

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), Ref<Image>());
	ERR_FAIL_INDEX_V(p_texture_index, ffsd->textures.size(), Ref<Image>());

	const ShelfPackTexture& tex = ffsd->textures[p_texture_index];
	return tex.image;
}

void TextServerAdvanced::_font_set_texture_offsets(const RID& p_font_rid, const Vector2i& p_size,
	int64_t p_texture_index, const PackedInt32Array& p_offsets)
{
	ERR_FAIL_COND(p_offsets.size() % 4 != 0);
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));
	ERR_FAIL_COND(p_texture_index < 0);
	if (p_texture_index >= ffsd->textures.size()) {
		ffsd->textures.resize(p_texture_index + 1);
	}

	ShelfPackTexture& tex = ffsd->textures.write[p_texture_index];
	tex.shelves.clear();
	for (int32_t i = 0; i < p_offsets.size(); i += 4) {
		tex.shelves.push_back(
			Shelf(p_offsets[i], p_offsets[i + 1], p_offsets[i + 2], p_offsets[i + 3]));
	}
}

PackedInt32Array TextServerAdvanced::_font_get_texture_offsets(
	const RID& p_font_rid, const Vector2i& p_size, int64_t p_texture_index) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, PackedInt32Array());

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), PackedInt32Array());
	ERR_FAIL_INDEX_V(p_texture_index, ffsd->textures.size(), PackedInt32Array());

	const ShelfPackTexture& tex = ffsd->textures[p_texture_index];
	PackedInt32Array ret;
	ret.resize(tex.shelves.size() * 4);

	int32_t* wr = ret.ptrw();
	int32_t i = 0;
	for (const Shelf& E : tex.shelves) {
		wr[i * 4] = E.x;
		wr[i * 4 + 1] = E.y;
		wr[i * 4 + 2] = E.w;
		wr[i * 4 + 3] = E.h;
		i++;
	}
	return ret;
}

PackedInt32Array TextServerAdvanced::_font_get_glyph_list(
	const RID& p_font_rid, const Vector2i& p_size) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, PackedInt32Array());

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), PackedInt32Array());

	PackedInt32Array ret;
	const HashMap<int32_t, FontGlyph>& gl = ffsd->glyph_map;
	for (const KeyValue<int32_t, FontGlyph>& E : gl) {
		ret.push_back(E.key);
	}
	return ret;
}

void TextServerAdvanced::_font_clear_glyphs(const RID& p_font_rid, const Vector2i& p_size)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));

	ffsd->glyph_map.clear();
}

void TextServerAdvanced::_font_remove_glyph(
	const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));

	ffsd->glyph_map.erase(p_glyph);
}

double TextServerAdvanced::_get_extra_advance(RID p_font_rid, int p_font_size) const
{
	const FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, 0.0);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, p_font_size);

	if (fd->embolden != 0.0) {
		return fd->embolden * double(size.x) / 4096.0;
	}
	else {
		return 0.0;
	}
}

Vector2 TextServerAdvanced::_font_get_glyph_advance(
	const RID& p_font_rid, int64_t p_size, int64_t p_glyph) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, Vector2());

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), Vector2());

	int mod = 0;
	if (fd->antialiasing == FONT_ANTIALIASING_LCD) {
		TextServer::FontLCDSubpixelLayout layout = lcd_subpixel_layout.get();
		if (layout != FONT_LCD_SUBPIXEL_LAYOUT_NONE) {
			mod = (layout << 24);
		}
	}

	FontGlyph fgl;
	if (!_ensure_glyph(fd, size, p_glyph | mod, fgl)) {
		return Vector2(); // Invalid or non graphicl glyph, do not display errors.
	}

	Vector2 ea;
	if (fd->embolden != 0.0) {
		ea.x = fd->embolden * double(size.x) / 4096.0;
	}

	double scale = _font_get_scale(p_font_rid, p_size);
	if (fd->msdf) {
		return (fgl.advance + ea) * (double)p_size / (double)fd->msdf_source_size;
	}
	else if (fd->fixed_size > 0 && fd->fixed_size_scale_mode != FIXED_SIZE_SCALE_DISABLE &&
			   size.x != p_size * 64) {
		if (fd->fixed_size_scale_mode == FIXED_SIZE_SCALE_ENABLED) {
			return (fgl.advance + ea) * (double)p_size / (double)fd->fixed_size;
		}
		else {
			return (fgl.advance + ea) * Math::round((double)p_size / (double)fd->fixed_size);
		}
	}
	else if ((scale == 1.0) && ((fd->subpixel_positioning == SUBPIXEL_POSITIONING_DISABLED) ||
									 (fd->subpixel_positioning == SUBPIXEL_POSITIONING_AUTO &&
										 size.x > SUBPIXEL_POSITIONING_ONE_HALF_MAX_SIZE * 64))) {
		return (fgl.advance + ea).round();
	}
	else {
		return fgl.advance + ea;
	}
}

void TextServerAdvanced::_font_set_glyph_advance(
	const RID& p_font_rid, int64_t p_size, int64_t p_glyph, const Vector2& p_advance)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));

	FontGlyph& fgl = ffsd->glyph_map[p_glyph];

	fgl.advance = p_advance;
	fgl.found = true;
}

Vector2 TextServerAdvanced::_font_get_glyph_offset(
	const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, Vector2());

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), Vector2());

	int mod = 0;
	if (fd->antialiasing == FONT_ANTIALIASING_LCD) {
		TextServer::FontLCDSubpixelLayout layout = lcd_subpixel_layout.get();
		if (layout != FONT_LCD_SUBPIXEL_LAYOUT_NONE) {
			mod = (layout << 24);
		}
	}

	FontGlyph fgl;
	if (!_ensure_glyph(fd, size, p_glyph | mod, fgl)) {
		return Vector2(); // Invalid or non graphicl glyph, do not display errors.
	}

	if (fd->msdf) {
		return fgl.rect.position * (double)p_size.x / (double)fd->msdf_source_size;
	}
	else if (fd->fixed_size > 0 && fd->fixed_size_scale_mode != FIXED_SIZE_SCALE_DISABLE &&
			   size.x != p_size.x * 64) {
		if (fd->fixed_size_scale_mode == FIXED_SIZE_SCALE_ENABLED) {
			return fgl.rect.position * (double)p_size.x / (double)fd->fixed_size;
		}
		else {
			return fgl.rect.position * Math::round((double)p_size.x / (double)fd->fixed_size);
		}
	}
	else {
		return fgl.rect.position;
	}
}

void TextServerAdvanced::_font_set_glyph_offset(
	const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph, const Vector2& p_offset)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));

	FontGlyph& fgl = ffsd->glyph_map[p_glyph];

	fgl.rect.position = p_offset;
	fgl.found = true;
}

Vector2 TextServerAdvanced::_font_get_glyph_size(
	const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, Vector2());

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), Vector2());

	int mod = 0;
	if (fd->antialiasing == FONT_ANTIALIASING_LCD) {
		TextServer::FontLCDSubpixelLayout layout = lcd_subpixel_layout.get();
		if (layout != FONT_LCD_SUBPIXEL_LAYOUT_NONE) {
			mod = (layout << 24);
		}
	}

	FontGlyph fgl;
	if (!_ensure_glyph(fd, size, p_glyph | mod, fgl)) {
		return Vector2(); // Invalid or non graphicl glyph, do not display errors.
	}

	if (fd->msdf) {
		return fgl.rect.size * (double)p_size.x / (double)fd->msdf_source_size;
	}
	else if (fd->fixed_size > 0 && fd->fixed_size_scale_mode != FIXED_SIZE_SCALE_DISABLE &&
			   size.x != p_size.x * 64) {
		if (fd->fixed_size_scale_mode == FIXED_SIZE_SCALE_ENABLED) {
			return fgl.rect.size * (double)p_size.x / (double)fd->fixed_size;
		}
		else {
			return fgl.rect.size * Math::round((double)p_size.x / (double)fd->fixed_size);
		}
	}
	else {
		return fgl.rect.size;
	}
}

void TextServerAdvanced::_font_set_glyph_size(
	const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph, const Vector2& p_gl_size)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));

	FontGlyph& fgl = ffsd->glyph_map[p_glyph];

	fgl.rect.size = p_gl_size;
	fgl.found = true;
}

Rect2 TextServerAdvanced::_font_get_glyph_uv_rect(
	const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, Rect2());

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), Rect2());

	int mod = 0;
	if (fd->antialiasing == FONT_ANTIALIASING_LCD) {
		TextServer::FontLCDSubpixelLayout layout = lcd_subpixel_layout.get();
		if (layout != FONT_LCD_SUBPIXEL_LAYOUT_NONE) {
			mod = (layout << 24);
		}
	}

	FontGlyph fgl;
	if (!_ensure_glyph(fd, size, p_glyph | mod, fgl)) {
		return Rect2(); // Invalid or non graphicl glyph, do not display errors.
	}

	return fgl.uv_rect;
}

void TextServerAdvanced::_font_set_glyph_uv_rect(
	const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph, const Rect2& p_uv_rect)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));

	FontGlyph& fgl = ffsd->glyph_map[p_glyph];

	fgl.uv_rect = p_uv_rect;
	fgl.found = true;
}

int64_t TextServerAdvanced::_font_get_glyph_texture_idx(
	const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, -1);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), -1);

	int mod = 0;
	if (fd->antialiasing == FONT_ANTIALIASING_LCD) {
		TextServer::FontLCDSubpixelLayout layout = lcd_subpixel_layout.get();
		if (layout != FONT_LCD_SUBPIXEL_LAYOUT_NONE) {
			mod = (layout << 24);
		}
	}

	FontGlyph fgl;
	if (!_ensure_glyph(fd, size, p_glyph | mod, fgl)) {
		return -1; // Invalid or non graphicl glyph, do not display errors.
	}

	return fgl.texture_idx;
}

void TextServerAdvanced::_font_set_glyph_texture_idx(
	const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph, int64_t p_texture_idx)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));

	FontGlyph& fgl = ffsd->glyph_map[p_glyph];

	fgl.texture_idx = p_texture_idx;
	fgl.found = true;
}

RID TextServerAdvanced::_font_get_glyph_texture_rid(
	const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, RID());

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), RID());

	int mod = 0;
	if (fd->antialiasing == FONT_ANTIALIASING_LCD) {
		TextServer::FontLCDSubpixelLayout layout = lcd_subpixel_layout.get();
		if (layout != FONT_LCD_SUBPIXEL_LAYOUT_NONE) {
			mod = (layout << 24);
		}
	}

	FontGlyph fgl;
	if (!_ensure_glyph(fd, size, p_glyph | mod, fgl)) {
		return RID(); // Invalid or non graphicl glyph, do not display errors.
	}

	ERR_FAIL_COND_V(fgl.texture_idx < -1 || fgl.texture_idx >= ffsd->textures.size(), RID());

	if (fgl.texture_idx != -1) {
		if (ffsd->textures[fgl.texture_idx].dirty) {
			ShelfPackTexture& tex = ffsd->textures.write[fgl.texture_idx];
			Ref<Image> img = tex.image;
			if (fgl.fix_edge) {
				// Same as the "fix alpha border" process option when importing SVGs
				img->fix_alpha_edges();
			}
			if (tex.texture.is_null()) {
				tex.texture = ImageTexture::create_from_image(img);
			}
			else {
				tex.texture->update(img);
			}
			tex.dirty = false;
		}
		return ffsd->textures[fgl.texture_idx].texture->get_rid();
	}

	return RID();
}

Size2 TextServerAdvanced::_font_get_glyph_texture_size(
	const RID& p_font_rid, const Vector2i& p_size, int64_t p_glyph) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, Size2());

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), Size2());

	int mod = 0;
	if (fd->antialiasing == FONT_ANTIALIASING_LCD) {
		TextServer::FontLCDSubpixelLayout layout = lcd_subpixel_layout.get();
		if (layout != FONT_LCD_SUBPIXEL_LAYOUT_NONE) {
			mod = (layout << 24);
		}
	}

	FontGlyph fgl;
	if (!_ensure_glyph(fd, size, p_glyph | mod, fgl)) {
		return Size2(); // Invalid or non graphicl glyph, do not display errors.
	}

	ERR_FAIL_COND_V(fgl.texture_idx < -1 || fgl.texture_idx >= ffsd->textures.size(), Size2());

	if (fgl.texture_idx != -1) {
		if (ffsd->textures[fgl.texture_idx].dirty) {
			ShelfPackTexture& tex = ffsd->textures.write[fgl.texture_idx];
			Ref<Image> img = tex.image;
			if (fgl.fix_edge) {
				// Same as the "fix alpha border" process option when importing SVGs
				img->fix_alpha_edges();
			}
			if (tex.texture.is_null()) {
				tex.texture = ImageTexture::create_from_image(img);
			}
			else {
				tex.texture->update(img);
			}
			tex.dirty = false;
		}
		return ffsd->textures[fgl.texture_idx].texture->get_size();
	}

	return Size2();
}

void TextServerAdvanced::_font_clear_kerning_map(const RID& p_font_rid, int64_t p_size)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));
	ffsd->kerning_map.clear();
}

void TextServerAdvanced::_font_remove_kerning(
	const RID& p_font_rid, int64_t p_size, const Vector2i& p_glyph_pair)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));
	ffsd->kerning_map.erase(p_glyph_pair);
}

void TextServerAdvanced::_font_set_kerning(
	const RID& p_font_rid, int64_t p_size, const Vector2i& p_glyph_pair, const Vector2& p_kerning)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));
	ffsd->kerning_map[p_glyph_pair] = p_kerning;
}

Vector2 TextServerAdvanced::_font_get_kerning(
	const RID& p_font_rid, int64_t p_size, const Vector2i& p_glyph_pair) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, Vector2());

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, p_size);

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), Vector2());

	const HashMap<Vector2i, Vector2>& kern = ffsd->kerning_map;

	if (kern.has(p_glyph_pair)) {
		if (fd->msdf) {
			return kern[p_glyph_pair] * (double)p_size / (double)fd->msdf_source_size;
		}
		else if (fd->fixed_size > 0 && fd->fixed_size_scale_mode != FIXED_SIZE_SCALE_DISABLE &&
				   size.x != p_size * 64) {
			if (fd->fixed_size_scale_mode == FIXED_SIZE_SCALE_ENABLED) {
				return kern[p_glyph_pair] * (double)p_size / (double)fd->fixed_size;
			}
			else {
				return kern[p_glyph_pair] * Math::round((double)p_size / (double)fd->fixed_size);
			}
		}
		else {
			return kern[p_glyph_pair];
		}
	}
	else {
#ifdef MODULE_FREETYPE_ENABLED
		if (fd->face) {
			FT_Vector delta;
			FT_Get_Kerning(fd->face, p_glyph_pair.x, p_glyph_pair.y, FT_KERNING_DEFAULT, &delta);
			if (fd->msdf) {
				return Vector2(delta.x, delta.y) * (double)p_size / (double)fd->msdf_source_size;
			}
			else if (fd->fixed_size > 0 &&
					   fd->fixed_size_scale_mode != FIXED_SIZE_SCALE_DISABLE &&
					   size.x != p_size * 64) {
				if (fd->fixed_size_scale_mode == FIXED_SIZE_SCALE_ENABLED) {
					return Vector2(delta.x, delta.y) * (double)p_size / (double)fd->fixed_size;
				}
				else {
					return Vector2(delta.x, delta.y) *
						   Math::round((double)p_size / (double)fd->fixed_size);
				}
			}
			else {
				return Vector2(delta.x, delta.y);
			}
		}
#endif
	}
	return Vector2();
}

int64_t TextServerAdvanced::_font_get_glyph_index(
	const RID& p_font_rid, int64_t p_size, int64_t p_char, int64_t p_variation_selector) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, 0);
	ERR_FAIL_COND_V_MSG((p_char >= 0xd800 && p_char <= 0xdfff) || (p_char > 0x10ffff), 0,
		"Unicode parsing error: Invalid unicode codepoint " + String::num_int64(p_char, 16) + ".");
	ERR_FAIL_COND_V_MSG((p_variation_selector >= 0xd800 && p_variation_selector <= 0xdfff) ||
							(p_variation_selector > 0x10ffff),
		0,
		"Unicode parsing error: Invalid unicode codepoint " +
			String::num_int64(p_variation_selector, 16) + ".");

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, p_size);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), 0);

#ifdef MODULE_FREETYPE_ENABLED
	if (fd->face) {
		if (p_variation_selector) {
			return FT_Face_GetCharVariantIndex(fd->face, p_char, p_variation_selector);
		}
		else {
			return FT_Get_Char_Index(fd->face, p_char);
		}
	}
	else {
		return (int64_t)p_char;
	}
#else
	return (int64_t)p_char;
#endif
}

int64_t TextServerAdvanced::_font_get_char_from_glyph_index(
	const RID& p_font_rid, int64_t p_size, int64_t p_glyph_index) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, 0);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size(fd, p_size);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), 0);

#ifdef MODULE_FREETYPE_ENABLED
	if (ffsd->inv_glyph_map.is_empty()) {
		FT_Face face = fd->face;
		FT_UInt gindex;
		FT_ULong charcode = FT_Get_First_Char(face, &gindex);
		while (gindex != 0) {
			if (charcode != 0) {
				ffsd->inv_glyph_map[gindex] = charcode;
			}
			charcode = FT_Get_Next_Char(face, charcode, &gindex);
		}
	}

	if (ffsd->inv_glyph_map.has(p_glyph_index)) {
		return ffsd->inv_glyph_map[p_glyph_index];
	}
	else {
		return 0;
	}
#else
	return p_glyph_index;
#endif
}

bool TextServerAdvanced::_font_has_char(const RID& p_font_rid, int64_t p_char) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_COND_V_MSG((p_char >= 0xd800 && p_char <= 0xdfff) || (p_char > 0x10ffff), false,
		"Unicode parsing error: Invalid unicode codepoint " + String::num_int64(p_char, 16) + ".");
	if (!fd) {
		return false;
	}

	MutexLock lock(fd->mutex);
	FontForSizeAdvanced* ffsd = nullptr;
	if (fd->cache.is_empty()) {
		ERR_FAIL_COND_V(
			!_ensure_cache_for_size(
				fd, fd->msdf ? Vector2i(fd->msdf_source_size * 64, 0) : Vector2i(16 * 64, 0), ffsd),
			false);
	}
	else {
		ffsd = fd->cache.begin()->value;
	}

#ifdef MODULE_FREETYPE_ENABLED
	if (fd->face) {
		return FT_Get_Char_Index(fd->face, p_char) != 0;
	}
#endif
	return ffsd->glyph_map.has((int32_t)p_char);
}

String TextServerAdvanced::_font_get_supported_chars(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, String());

	MutexLock lock(fd->mutex);
	FontForSizeAdvanced* ffsd = nullptr;
	if (fd->cache.is_empty()) {
		ERR_FAIL_COND_V(
			!_ensure_cache_for_size(
				fd, fd->msdf ? Vector2i(fd->msdf_source_size * 64, 0) : Vector2i(16 * 64, 0), ffsd),
			String());
	}
	else {
		ffsd = fd->cache.begin()->value;
	}

	String chars;
#ifdef MODULE_FREETYPE_ENABLED
	if (fd->face) {
		FT_UInt gindex;
		FT_ULong charcode = FT_Get_First_Char(fd->face, &gindex);
		while (gindex != 0) {
			if (charcode != 0) {
				chars = chars + String::chr(charcode);
			}
			charcode = FT_Get_Next_Char(fd->face, charcode, &gindex);
		}
		return chars;
	}
#endif
	const HashMap<int32_t, FontGlyph>& gl = ffsd->glyph_map;
	for (const KeyValue<int32_t, FontGlyph>& E : gl) {
		chars = chars + String::chr(E.key);
	}
	return chars;
}

PackedInt32Array TextServerAdvanced::_font_get_supported_glyphs(const RID& p_font_rid) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, PackedInt32Array());

	MutexLock lock(fd->mutex);
	FontForSizeAdvanced* at_size = nullptr;
	if (fd->cache.is_empty()) {
		ERR_FAIL_COND_V(
			!_ensure_cache_for_size(fd,
				fd->msdf ? Vector2i(fd->msdf_source_size * 64, 0) : Vector2i(16 * 64, 0), at_size),
			PackedInt32Array());
	}
	else {
		at_size = fd->cache.begin()->value;
	}

	PackedInt32Array glyphs;
#ifdef MODULE_FREETYPE_ENABLED
	if (fd->face) {
		FT_UInt gindex;
		FT_ULong charcode = FT_Get_First_Char(fd->face, &gindex);
		while (gindex != 0) {
			glyphs.push_back(gindex);
			charcode = FT_Get_Next_Char(fd->face, charcode, &gindex);
		}
		return glyphs;
	}
#endif
	if (at_size) {
		const HashMap<int32_t, FontGlyph>& gl = at_size->glyph_map;
		for (const KeyValue<int32_t, FontGlyph>& E : gl) {
			glyphs.push_back(E.key);
		}
	}
	return glyphs;
}

void TextServerAdvanced::_font_render_range(
	const RID& p_font_rid, const Vector2i& p_size, int64_t p_start, int64_t p_end)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);
	ERR_FAIL_COND_MSG((p_start >= 0xd800 && p_start <= 0xdfff) || (p_start > 0x10ffff),
		"Unicode parsing error: Invalid unicode codepoint " + String::num_int64(p_start, 16) + ".");
	ERR_FAIL_COND_MSG((p_end >= 0xd800 && p_end <= 0xdfff) || (p_end > 0x10ffff),
		"Unicode parsing error: Invalid unicode codepoint " + String::num_int64(p_end, 16) + ".");

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));
	for (int64_t i = p_start; i <= p_end; i++) {
#ifdef MODULE_FREETYPE_ENABLED
		int32_t idx = FT_Get_Char_Index(fd->face, i);
		if (fd->face) {
			FontGlyph fgl;
			if (fd->msdf) {
				_ensure_glyph(fd, size, (int32_t)idx, fgl);
			}
			else {
				for (int aa = 0; aa < ((fd->antialiasing == FONT_ANTIALIASING_LCD)
											  ? FONT_LCD_SUBPIXEL_LAYOUT_MAX
											  : 1);
					 aa++) {
					if ((fd->subpixel_positioning == SUBPIXEL_POSITIONING_ONE_QUARTER) ||
						(fd->subpixel_positioning == SUBPIXEL_POSITIONING_AUTO &&
							size.x <= SUBPIXEL_POSITIONING_ONE_QUARTER_MAX_SIZE * 64)) {
						_ensure_glyph(fd, size, (int32_t)idx | (0 << 27) | (aa << 24), fgl);
						_ensure_glyph(fd, size, (int32_t)idx | (1 << 27) | (aa << 24), fgl);
						_ensure_glyph(fd, size, (int32_t)idx | (2 << 27) | (aa << 24), fgl);
						_ensure_glyph(fd, size, (int32_t)idx | (3 << 27) | (aa << 24), fgl);
					}
					else if ((fd->subpixel_positioning == SUBPIXEL_POSITIONING_ONE_HALF) ||
							   (fd->subpixel_positioning == SUBPIXEL_POSITIONING_AUTO &&
								   size.x <= SUBPIXEL_POSITIONING_ONE_HALF_MAX_SIZE * 64)) {
						_ensure_glyph(fd, size, (int32_t)idx | (1 << 27) | (aa << 24), fgl);
						_ensure_glyph(fd, size, (int32_t)idx | (0 << 27) | (aa << 24), fgl);
					}
					else {
						_ensure_glyph(fd, size, (int32_t)idx | (aa << 24), fgl);
					}
				}
			}
		}
#endif
	}
}

void TextServerAdvanced::_font_render_glyph(
	const RID& p_font_rid, const Vector2i& p_size, int64_t p_index)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	Vector2i size = _get_size_outline(fd, p_size);
	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(fd, size, ffsd));
#ifdef MODULE_FREETYPE_ENABLED
	int32_t idx = p_index & 0xffffff; // Remove subpixel shifts.
	if (fd->face) {
		FontGlyph fgl;
		if (fd->msdf) {
			_ensure_glyph(fd, size, (int32_t)idx, fgl);
		}
		else {
			for (int aa = 0;
				 aa <
				 ((fd->antialiasing == FONT_ANTIALIASING_LCD) ? FONT_LCD_SUBPIXEL_LAYOUT_MAX : 1);
				 aa++) {
				if ((fd->subpixel_positioning == SUBPIXEL_POSITIONING_ONE_QUARTER) ||
					(fd->subpixel_positioning == SUBPIXEL_POSITIONING_AUTO &&
						size.x <= SUBPIXEL_POSITIONING_ONE_QUARTER_MAX_SIZE * 64)) {
					_ensure_glyph(fd, size, (int32_t)idx | (0 << 27) | (aa << 24), fgl);
					_ensure_glyph(fd, size, (int32_t)idx | (1 << 27) | (aa << 24), fgl);
					_ensure_glyph(fd, size, (int32_t)idx | (2 << 27) | (aa << 24), fgl);
					_ensure_glyph(fd, size, (int32_t)idx | (3 << 27) | (aa << 24), fgl);
				}
				else if ((fd->subpixel_positioning == SUBPIXEL_POSITIONING_ONE_HALF) ||
						   (fd->subpixel_positioning == SUBPIXEL_POSITIONING_AUTO &&
							   size.x <= SUBPIXEL_POSITIONING_ONE_HALF_MAX_SIZE * 64)) {
					_ensure_glyph(fd, size, (int32_t)idx | (1 << 27) | (aa << 24), fgl);
					_ensure_glyph(fd, size, (int32_t)idx | (0 << 27) | (aa << 24), fgl);
				}
				else {
					_ensure_glyph(fd, size, (int32_t)idx | (aa << 24), fgl);
				}
			}
		}
	}
#endif
}

void TextServerAdvanced::_font_draw_glyph(const RID& p_font_rid, const RID& p_canvas,
	int64_t p_size, const Vector2& p_pos, int64_t p_index, const Color& p_color,
	float p_oversampling) const
{
	if (p_index == 0) {
		return; // Non visual character, skip.
	}
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);

	// Oversampling.
	bool viewport_oversampling = false;
	float oversampling_factor = p_oversampling;
	if (p_oversampling <= 0.0) {
		if (fd->oversampling_override > 0.0) {
			oversampling_factor = fd->oversampling_override;
		}
		else if (vp_oversampling > 0.0) {
			oversampling_factor = vp_oversampling;
			viewport_oversampling = true;
		}
		else {
			oversampling_factor = 1.0;
		}
	}
	bool skip_oversampling = fd->msdf || fd->fixed_size > 0;
	if (skip_oversampling) {
		oversampling_factor = 1.0;
	}
	else {
		uint64_t oversampling_level = CLAMP(oversampling_factor, 0.1, 100.0) * 64;
		oversampling_factor = double(oversampling_level) / 64.0;
	}

	Vector2i size;
	if (skip_oversampling) {
		size = _get_size(fd, p_size);
	}
	else {
		size = Vector2i(p_size * 64 * oversampling_factor, 0);
	}

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(
		fd, size, ffsd, false, viewport_oversampling ? 64 * oversampling_factor : 0));

	int32_t index = p_index & 0xffffff; // Remove subpixel shifts.
	bool lcd_aa = false;

#ifdef MODULE_FREETYPE_ENABLED
	if (!fd->msdf && fd->face) {
		// LCD layout, bits 24, 25, 26
		if (fd->antialiasing == FONT_ANTIALIASING_LCD) {
			TextServer::FontLCDSubpixelLayout layout = lcd_subpixel_layout.get();
			if (layout != FONT_LCD_SUBPIXEL_LAYOUT_NONE) {
				lcd_aa = true;
				index = index | (layout << 24);
			}
		}
		// Subpixel X-shift, bits 27, 28
		if ((fd->subpixel_positioning == SUBPIXEL_POSITIONING_ONE_QUARTER) ||
			(fd->subpixel_positioning == SUBPIXEL_POSITIONING_AUTO &&
				size.x <= SUBPIXEL_POSITIONING_ONE_QUARTER_MAX_SIZE * 64)) {
			int xshift =
				(int)(Math::floor(4 * (p_pos.x + 0.125)) - 4 * Math::floor(p_pos.x + 0.125));
			index = index | (xshift << 27);
		}
		else if ((fd->subpixel_positioning == SUBPIXEL_POSITIONING_ONE_HALF) ||
				   (fd->subpixel_positioning == SUBPIXEL_POSITIONING_AUTO &&
					   size.x <= SUBPIXEL_POSITIONING_ONE_HALF_MAX_SIZE * 64)) {
			int xshift = (int)(Math::floor(2 * (p_pos.x + 0.25)) - 2 * Math::floor(p_pos.x + 0.25));
			index = index | (xshift << 27);
		}
	}
#endif

	FontGlyph fgl;
	if (!_ensure_glyph(
			fd, size, index, fgl, viewport_oversampling ? 64 * oversampling_factor : 0)) {
		return; // Invalid or non-graphical glyph, do not display errors, nothing to draw.
	}

	if (fgl.found) {
		ERR_FAIL_COND(fgl.texture_idx < -1 || fgl.texture_idx >= ffsd->textures.size());

		if (fgl.texture_idx != -1) {
			Color modulate = p_color;
#ifdef MODULE_FREETYPE_ENABLED
			if (!fd->modulate_color_glyphs && fd->face &&
				ffsd->textures[fgl.texture_idx].image.is_valid() &&
				(ffsd->textures[fgl.texture_idx].image->get_format() == Image::FORMAT_RGBA8) &&
				!lcd_aa && !fd->msdf) {
				modulate.r = modulate.g = modulate.b = 1.0;
			}
#endif
			if (ffsd->textures[fgl.texture_idx].dirty) {
				ShelfPackTexture& tex = ffsd->textures.write[fgl.texture_idx];
				Ref<Image> img = tex.image;
				if (fgl.fix_edge) {
					// Same as the "fix alpha border" process option when importing SVGs
					img->fix_alpha_edges();
				}
				if (tex.texture.is_null()) {
					tex.texture = ImageTexture::create_from_image(img);
				}
				else {
					tex.texture->update(img);
				}
				tex.dirty = false;
			}
			if (fd->msdf) {
				Point2 cpos = p_pos;
				cpos += fgl.rect.position * (double)p_size / (double)fd->msdf_source_size;
				Size2 csize = fgl.rect.size * (double)p_size / (double)fd->msdf_source_size;
				ffsd->textures[fgl.texture_idx].texture->draw_msdf_rect_region(p_canvas,
					Rect2(cpos, csize), fgl.uv_rect, modulate, 0, fd->msdf_range,
					(double)p_size / (double)fd->msdf_source_size);
			}
			else {
				Point2 cpos = p_pos;
				double scale = _font_get_scale(p_font_rid, p_size) / oversampling_factor;
				if ((fd->subpixel_positioning == SUBPIXEL_POSITIONING_ONE_QUARTER) ||
					(fd->subpixel_positioning == SUBPIXEL_POSITIONING_AUTO &&
						size.x <= SUBPIXEL_POSITIONING_ONE_QUARTER_MAX_SIZE * 64)) {
					cpos.x = cpos.x + 0.125;
				}
				else if ((fd->subpixel_positioning == SUBPIXEL_POSITIONING_ONE_HALF) ||
						   (fd->subpixel_positioning == SUBPIXEL_POSITIONING_AUTO &&
							   size.x <= SUBPIXEL_POSITIONING_ONE_HALF_MAX_SIZE * 64)) {
					cpos.x = cpos.x + 0.25;
				}
				if (scale == 1.0) {
					cpos.y = Math::floor(cpos.y);
					cpos.x = Math::floor(cpos.x);
				}
				Vector2 gpos = fgl.rect.position;
				Size2 csize = fgl.rect.size;
				if (fd->fixed_size > 0 && fd->fixed_size_scale_mode != FIXED_SIZE_SCALE_DISABLE) {
					if (size.x != p_size * 64) {
						if (fd->fixed_size_scale_mode == FIXED_SIZE_SCALE_ENABLED) {
							double gl_scale = (double)p_size / (double)fd->fixed_size;
							gpos *= gl_scale;
							csize *= gl_scale;
						}
						else {
							double gl_scale = Math::round((double)p_size / (double)fd->fixed_size);
							gpos *= gl_scale;
							csize *= gl_scale;
						}
					}
				}
				else {
					gpos /= oversampling_factor;
					csize /= oversampling_factor;
				}
				cpos += gpos;
				if (lcd_aa) {
					ffsd->textures[fgl.texture_idx].texture->draw_lcd_rect_region(
						p_canvas, Rect2(cpos, csize), fgl.uv_rect, modulate);
				}
				else {
					ffsd->textures[fgl.texture_idx].texture->draw_rect_region(
						p_canvas, Rect2(cpos, csize), fgl.uv_rect, modulate, false, false);
				}
			}
		}
	}
}

void TextServerAdvanced::_font_draw_glyph_outline(const RID& p_font_rid, const RID& p_canvas,
	int64_t p_size, int64_t p_outline_size, const Vector2& p_pos, int64_t p_index,
	const Color& p_color, float p_oversampling) const
{
	if (p_index == 0) {
		return; // Non visual character, skip.
	}
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);

	// Oversampling.
	bool viewport_oversampling = false;
	float oversampling_factor = p_oversampling;
	if (p_oversampling <= 0.0) {
		if (fd->oversampling_override > 0.0) {
			oversampling_factor = fd->oversampling_override;
		}
		else if (vp_oversampling > 0.0) {
			oversampling_factor = vp_oversampling;
			viewport_oversampling = true;
		}
		else {
			oversampling_factor = 1.0;
		}
	}
	bool skip_oversampling = fd->msdf || fd->fixed_size > 0;
	if (skip_oversampling) {
		oversampling_factor = 1.0;
	}
	else {
		uint64_t oversampling_level = CLAMP(oversampling_factor, 0.1, 100.0) * 64;
		oversampling_factor = double(oversampling_level) / 64.0;
	}

	Vector2i size;
	if (skip_oversampling) {
		size = _get_size_outline(fd, Vector2i(p_size, p_outline_size));
	}
	else {
		size = Vector2i(p_size * 64 * oversampling_factor, p_outline_size * oversampling_factor);
	}

	FontForSizeAdvanced* ffsd = nullptr;
	ERR_FAIL_COND(!_ensure_cache_for_size(
		fd, size, ffsd, false, viewport_oversampling ? 64 * oversampling_factor : 0));

	int32_t index = p_index & 0xffffff; // Remove subpixel shifts.
	bool lcd_aa = false;

#ifdef MODULE_FREETYPE_ENABLED
	if (!fd->msdf && fd->face) {
		// LCD layout, bits 24, 25, 26
		if (fd->antialiasing == FONT_ANTIALIASING_LCD) {
			TextServer::FontLCDSubpixelLayout layout = lcd_subpixel_layout.get();
			if (layout != FONT_LCD_SUBPIXEL_LAYOUT_NONE) {
				lcd_aa = true;
				index = index | (layout << 24);
			}
		}
		// Subpixel X-shift, bits 27, 28
		if ((fd->subpixel_positioning == SUBPIXEL_POSITIONING_ONE_QUARTER) ||
			(fd->subpixel_positioning == SUBPIXEL_POSITIONING_AUTO &&
				size.x <= SUBPIXEL_POSITIONING_ONE_QUARTER_MAX_SIZE * 64)) {
			int xshift =
				(int)(Math::floor(4 * (p_pos.x + 0.125)) - 4 * Math::floor(p_pos.x + 0.125));
			index = index | (xshift << 27);
		}
		else if ((fd->subpixel_positioning == SUBPIXEL_POSITIONING_ONE_HALF) ||
				   (fd->subpixel_positioning == SUBPIXEL_POSITIONING_AUTO &&
					   size.x <= SUBPIXEL_POSITIONING_ONE_HALF_MAX_SIZE * 64)) {
			int xshift = (int)(Math::floor(2 * (p_pos.x + 0.25)) - 2 * Math::floor(p_pos.x + 0.25));
			index = index | (xshift << 27);
		}
	}
#endif

	FontGlyph fgl;
	if (!_ensure_glyph(
			fd, size, index, fgl, viewport_oversampling ? 64 * oversampling_factor : 0)) {
		return; // Invalid or non-graphical glyph, do not display errors, nothing to draw.
	}

	if (fgl.found) {
		ERR_FAIL_COND(fgl.texture_idx < -1 || fgl.texture_idx >= ffsd->textures.size());

		if (fgl.texture_idx != -1) {
			Color modulate = p_color;
#ifdef MODULE_FREETYPE_ENABLED
			if (fd->face && fd->cache[size]->textures[fgl.texture_idx].image.is_valid() &&
				(ffsd->textures[fgl.texture_idx].image->get_format() == Image::FORMAT_RGBA8) &&
				!lcd_aa && !fd->msdf) {
				modulate.r = modulate.g = modulate.b = 1.0;
			}
#endif
			if (ffsd->textures[fgl.texture_idx].dirty) {
				ShelfPackTexture& tex = ffsd->textures.write[fgl.texture_idx];
				Ref<Image> img = tex.image;
				if (tex.texture.is_null()) {
					tex.texture = ImageTexture::create_from_image(img);
				}
				else {
					tex.texture->update(img);
				}
				tex.dirty = false;
			}
			if (fd->msdf) {
				Point2 cpos = p_pos;
				cpos += fgl.rect.position * (double)p_size / (double)fd->msdf_source_size;
				Size2 csize = fgl.rect.size * (double)p_size / (double)fd->msdf_source_size;
				ffsd->textures[fgl.texture_idx].texture->draw_msdf_rect_region(p_canvas,
					Rect2(cpos, csize), fgl.uv_rect, modulate, p_outline_size, fd->msdf_range,
					(double)p_size / (double)fd->msdf_source_size);
			}
			else {
				Point2 cpos = p_pos;
				double scale = _font_get_scale(p_font_rid, p_size) / oversampling_factor;
				if ((fd->subpixel_positioning == SUBPIXEL_POSITIONING_ONE_QUARTER) ||
					(fd->subpixel_positioning == SUBPIXEL_POSITIONING_AUTO &&
						size.x <= SUBPIXEL_POSITIONING_ONE_QUARTER_MAX_SIZE * 64)) {
					cpos.x = cpos.x + 0.125;
				}
				else if ((fd->subpixel_positioning == SUBPIXEL_POSITIONING_ONE_HALF) ||
						   (fd->subpixel_positioning == SUBPIXEL_POSITIONING_AUTO &&
							   size.x <= SUBPIXEL_POSITIONING_ONE_HALF_MAX_SIZE * 64)) {
					cpos.x = cpos.x + 0.25;
				}
				if (scale == 1.0) {
					cpos.y = Math::floor(cpos.y);
					cpos.x = Math::floor(cpos.x);
				}
				Vector2 gpos = fgl.rect.position;
				Size2 csize = fgl.rect.size;
				if (fd->fixed_size > 0 && fd->fixed_size_scale_mode != FIXED_SIZE_SCALE_DISABLE) {
					if (size.x != p_size * 64) {
						if (fd->fixed_size_scale_mode == FIXED_SIZE_SCALE_ENABLED) {
							double gl_scale = (double)p_size / (double)fd->fixed_size;
							gpos *= gl_scale;
							csize *= gl_scale;
						}
						else {
							double gl_scale = Math::round((double)p_size / (double)fd->fixed_size);
							gpos *= gl_scale;
							csize *= gl_scale;
						}
					}
				}
				else {
					gpos /= oversampling_factor;
					csize /= oversampling_factor;
				}
				cpos += gpos;
				if (lcd_aa) {
					ffsd->textures[fgl.texture_idx].texture->draw_lcd_rect_region(
						p_canvas, Rect2(cpos, csize), fgl.uv_rect, modulate);
				}
				else {
					ffsd->textures[fgl.texture_idx].texture->draw_rect_region(
						p_canvas, Rect2(cpos, csize), fgl.uv_rect, modulate, false, false);
				}
			}
		}
	}
}

bool TextServerAdvanced::_font_is_language_supported(
	const RID& p_font_rid, const String& p_language) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, false);

	MutexLock lock(fd->mutex);
	if (fd->language_support_overrides.has(p_language)) {
		return fd->language_support_overrides[p_language];
	}
	else {
		if (fd->language_support_overrides.has("*")) {
			return fd->language_support_overrides["*"];
		}
		return true;
	}
}

void TextServerAdvanced::_font_set_language_support_override(
	const RID& p_font_rid, const String& p_language, bool p_supported)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	fd->language_support_overrides[p_language] = p_supported;
}

bool TextServerAdvanced::_font_get_language_support_override(
	const RID& p_font_rid, const String& p_language)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, false);

	MutexLock lock(fd->mutex);
	return fd->language_support_overrides[p_language];
}

void TextServerAdvanced::_font_remove_language_support_override(
	const RID& p_font_rid, const String& p_language)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	fd->language_support_overrides.erase(p_language);
}

PackedStringArray TextServerAdvanced::_font_get_language_support_overrides(const RID& p_font_rid)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, PackedStringArray());

	MutexLock lock(fd->mutex);
	PackedStringArray out;
	for (const KeyValue<String, bool>& E : fd->language_support_overrides) {
		out.push_back(E.key);
	}
	return out;
}

bool TextServerAdvanced::_font_is_script_supported(
	const RID& p_font_rid, const String& p_script) const
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, false);

	MutexLock lock(fd->mutex);
	if (fd->script_support_overrides.has(p_script)) {
		return fd->script_support_overrides[p_script];
	}
	else {
		if (fd->script_support_overrides.has("*")) {
			return fd->script_support_overrides["*"];
		}
		Vector2i size = _get_size(fd, 16);
		FontForSizeAdvanced* ffsd = nullptr;
		ERR_FAIL_COND_V(!_ensure_cache_for_size(fd, size, ffsd), false);
		char ascii_script[] = {' ', ' ', ' ', ' '};
		for (int i = 0; i < MIN(4, p_script.size()); i++) {
			if (p_script[i] <= 0x7f) {
				ascii_script[i] = p_script[i];
			}
		}
		return fd->supported_scripts.has(hb_tag_from_string(ascii_script, -1));
	}
}

void TextServerAdvanced::_font_set_script_support_override(
	const RID& p_font_rid, const String& p_script, bool p_supported)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	fd->script_support_overrides[p_script] = p_supported;
}

bool TextServerAdvanced::_font_get_script_support_override(
	const RID& p_font_rid, const String& p_script)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, false);

	MutexLock lock(fd->mutex);
	return fd->script_support_overrides[p_script];
}

void TextServerAdvanced::_font_remove_script_support_override(
	const RID& p_font_rid, const String& p_script)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL(fd);

	MutexLock lock(fd->mutex);
	fd->script_support_overrides.erase(p_script);
}

PackedStringArray TextServerAdvanced::_font_get_script_support_overrides(const RID& p_font_rid)
{
	FontAdvanced* fd = _get_font_data(p_font_rid);
	ERR_FAIL_NULL_V(fd, PackedStringArray());

	MutexLock lock(fd->mutex);
	PackedStringArray out;
	for (const KeyValue<String, bool>& E : fd->script_support_overrides) {
		out.push_back(E.key);
	}
	return out;
}

/*************************************************************************/
/* Shaped text buffer interface                                          */
/*************************************************************************/

int64_t TextServerAdvanced::_convert_pos(
	const String& p_utf32, const Char16String& p_utf16, int64_t p_pos) const
{
	int64_t limit = p_pos;
	if (p_utf32.length() != p_utf16.length()) {
		const UChar* data = p_utf16.get_data();
		for (int i = 0; i < p_pos; i++) {
			if (U16_IS_LEAD(data[i])) {
				limit--;
			}
		}
	}
	return limit;
}

int64_t TextServerAdvanced::_convert_pos(const ShapedTextDataAdvanced* p_sd, int64_t p_pos) const
{
	int64_t limit = p_pos;
	if (p_sd->text.length() != p_sd->utf16.length()) {
		const UChar* data = p_sd->utf16.get_data();
		for (int i = 0; i < p_pos; i++) {
			if (U16_IS_LEAD(data[i])) {
				limit--;
			}
		}
	}
	return limit;
}

int64_t TextServerAdvanced::_convert_pos_inv(
	const ShapedTextDataAdvanced* p_sd, int64_t p_pos) const
{
	int64_t limit = p_pos;
	if (p_sd->text.length() != p_sd->utf16.length()) {
		for (int i = 0; i < p_pos; i++) {
			if (p_sd->text[i] > 0xffff) {
				limit++;
			}
		}
	}
	return limit;
}

void TextServerAdvanced::invalidate(
	TextServerAdvanced::ShapedTextDataAdvanced* p_shaped, bool p_text)
{
	p_shaped->valid.clear();
	p_shaped->sort_valid = false;
	p_shaped->line_breaks_valid = false;
	p_shaped->justification_ops_valid = false;
	p_shaped->text_trimmed = false;
	p_shaped->ascent = 0.0;
	p_shaped->descent = 0.0;
	p_shaped->width = 0.0;
	p_shaped->upos = 0.0;
	p_shaped->uthk = 0.0;
	p_shaped->glyphs.clear();
	p_shaped->glyphs_logical.clear();
	p_shaped->runs.clear();
	p_shaped->runs_dirty = true;
	p_shaped->overrun_trim_data = TrimData();
	p_shaped->utf16 = Char16String();
	for (int i = 0; i < p_shaped->bidi_iter.size(); i++) {
		ubidi_close(p_shaped->bidi_iter[i]);
	}
	p_shaped->bidi_iter.clear();

	if (p_text) {
		if (p_shaped->script_iter != nullptr) {
			memdelete(p_shaped->script_iter);
			p_shaped->script_iter = nullptr;
		}
		p_shaped->break_ops_valid = false;
		p_shaped->chars_valid = false;
		p_shaped->js_ops_valid = false;
	}
}

RID TextServerAdvanced::_create_shaped_text(
	TextServer::Direction p_direction, TextServer::Orientation p_orientation)
{
	_THREAD_SAFE_METHOD_
	ERR_FAIL_COND_V_MSG(p_direction == DIRECTION_INHERITED, RID(), "Invalid text direction.");

	ShapedTextDataAdvanced* sd = memnew(ShapedTextDataAdvanced);
	sd->hb_buffer = hb_buffer_create();
	sd->direction = p_direction;
	sd->orientation = p_orientation;
	return shaped_owner.make_rid(sd);
}

void TextServerAdvanced::_shaped_text_set_direction(
	const RID& p_shaped, TextServer::Direction p_direction)
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_COND_MSG(p_direction == DIRECTION_INHERITED, "Invalid text direction.");
	ERR_FAIL_NULL(sd);

	MutexLock lock(sd->mutex);
	if (sd->direction != p_direction) {
		if (sd->parent != RID()) {
			full_copy(sd);
		}
		sd->direction = p_direction;
		invalidate(sd, false);
	}
}

TextServer::Direction TextServerAdvanced::_shaped_text_get_direction(const RID& p_shaped) const
{
	const ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, TextServer::DIRECTION_LTR);

	MutexLock lock(sd->mutex);
	return sd->direction;
}

TextServer::Direction TextServerAdvanced::_shaped_text_get_inferred_direction(
	const RID& p_shaped) const
{
	const ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, TextServer::DIRECTION_LTR);

	MutexLock lock(sd->mutex);
	return sd->para_direction;
}

void TextServerAdvanced::_shaped_text_set_custom_punctuation(
	const RID& p_shaped, const String& p_punct)
{
	_THREAD_SAFE_METHOD_
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL(sd);

	if (sd->custom_punct != p_punct) {
		if (sd->parent != RID()) {
			full_copy(sd);
		}
		sd->custom_punct = p_punct;
		invalidate(sd, false);
	}
}

String TextServerAdvanced::_shaped_text_get_custom_punctuation(const RID& p_shaped) const
{
	_THREAD_SAFE_METHOD_
	const ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, String());
	return sd->custom_punct;
}

void TextServerAdvanced::_shaped_text_set_custom_ellipsis(const RID& p_shaped, int64_t p_char)
{
	_THREAD_SAFE_METHOD_
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL(sd);
	sd->el_char = p_char;
}

int64_t TextServerAdvanced::_shaped_text_get_custom_ellipsis(const RID& p_shaped) const
{
	_THREAD_SAFE_METHOD_
	const ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, 0);
	return sd->el_char;
}

void TextServerAdvanced::_shaped_text_set_orientation(
	const RID& p_shaped, TextServer::Orientation p_orientation)
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL(sd);

	MutexLock lock(sd->mutex);
	if (sd->orientation != p_orientation) {
		if (sd->parent != RID()) {
			full_copy(sd);
		}
		sd->orientation = p_orientation;
		invalidate(sd, false);
	}
}

void TextServerAdvanced::_shaped_text_set_preserve_invalid(const RID& p_shaped, bool p_enabled)
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL(sd);

	MutexLock lock(sd->mutex);
	ERR_FAIL_COND(sd->parent != RID());
	if (sd->preserve_invalid != p_enabled) {
		sd->preserve_invalid = p_enabled;
		invalidate(sd, false);
	}
}

bool TextServerAdvanced::_shaped_text_get_preserve_invalid(const RID& p_shaped) const
{
	const ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, false);

	MutexLock lock(sd->mutex);
	return sd->preserve_invalid;
}

void TextServerAdvanced::_shaped_text_set_preserve_control(const RID& p_shaped, bool p_enabled)
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL(sd);

	MutexLock lock(sd->mutex);
	if (sd->preserve_control != p_enabled) {
		if (sd->parent != RID()) {
			full_copy(sd);
		}
		sd->preserve_control = p_enabled;
		invalidate(sd, false);
	}
}

bool TextServerAdvanced::_shaped_text_get_preserve_control(const RID& p_shaped) const
{
	const ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, false);

	MutexLock lock(sd->mutex);
	return sd->preserve_control;
}

void TextServerAdvanced::_shaped_text_set_spacing(
	const RID& p_shaped, SpacingType p_spacing, int64_t p_value)
{
	ERR_FAIL_INDEX((int)p_spacing, 4);
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL(sd);

	MutexLock lock(sd->mutex);
	if (sd->extra_spacing[p_spacing] != p_value) {
		if (sd->parent != RID()) {
			full_copy(sd);
		}
		sd->extra_spacing[p_spacing] = p_value;
		invalidate(sd, false);
	}
}

int64_t TextServerAdvanced::_shaped_text_get_spacing(
	const RID& p_shaped, SpacingType p_spacing) const
{
	ERR_FAIL_INDEX_V((int)p_spacing, 4, 0);

	const ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, 0);

	MutexLock lock(sd->mutex);
	return sd->extra_spacing[p_spacing];
}

TextServer::Orientation TextServerAdvanced::_shaped_text_get_orientation(const RID& p_shaped) const
{
	const ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, TextServer::ORIENTATION_HORIZONTAL);

	MutexLock lock(sd->mutex);
	return sd->orientation;
}

int64_t TextServerAdvanced::_shaped_get_span_count(const RID& p_shaped) const
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, 0);

	if (sd->parent != RID()) {
		return sd->last_span - sd->first_span + 1;
	}
	else {
		return sd->spans.size();
	}
}

String TextServerAdvanced::_shaped_get_span_text(const RID& p_shaped, int64_t p_index) const
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, String());
	ShapedTextDataAdvanced* span_sd = sd;
	if (sd->parent.is_valid()) {
		span_sd = shaped_owner.get_or_null(sd->parent);
		ERR_FAIL_NULL_V(span_sd, String());
	}
	ERR_FAIL_INDEX_V(p_index, span_sd->spans.size(), String());
	return span_sd->text.substr(
		span_sd->spans[p_index].start, span_sd->spans[p_index].end - span_sd->spans[p_index].start);
}

void TextServerAdvanced::_generate_runs(ShapedTextDataAdvanced* p_sd) const
{
	ERR_FAIL_NULL(p_sd);
	p_sd->runs.clear();

	ShapedTextDataAdvanced* span_sd = p_sd;
	if (p_sd->parent.is_valid()) {
		span_sd = shaped_owner.get_or_null(p_sd->parent);
		ERR_FAIL_NULL(span_sd);
	}

	int sd_size = p_sd->glyphs.size();
	const Glyph* sd_gl = p_sd->glyphs.ptr();

	int span_count = span_sd->spans.size();
	int span = -1;
	int span_start = -1;
	int span_end = -1;

	TextRun run;
	for (int i = 0; i < sd_size; i += sd_gl[i].count) {
		const Glyph& gl = sd_gl[i];
		if (gl.start < 0 || gl.end < 0) {
			continue;
		}
		if (gl.start < span_start || gl.start >= span_end) {
			span = -1;
			span_start = -1;
			span_end = -1;
			for (int j = 0; j < span_count; j++) {
				if (gl.start >= span_sd->spans[j].start && gl.end <= span_sd->spans[j].end) {
					span = j;
					span_start = span_sd->spans[j].start;
					span_end = span_sd->spans[j].end;
					break;
				}
			}
		}
		if (run.font_rid != gl.font_rid || run.font_size != gl.font_size ||
			run.span_index != span || run.rtl != bool(gl.flags & GRAPHEME_IS_RTL)) {
			if (run.span_index >= 0) {
				p_sd->runs.push_back(run);
			}
			run.range = Vector2i(gl.start, gl.end);
			run.gl_range = Vector2i(i, i);
			run.font_rid = gl.font_rid;
			run.font_size = gl.font_size;
			run.rtl = bool(gl.flags & GRAPHEME_IS_RTL);
			run.span_index = span;
		}
		run.range.x = MIN(run.range.x, gl.start);
		run.range.y = MAX(run.range.y, gl.end);
		run.gl_range.x = MIN(run.gl_range.x, i);
		run.gl_range.y = MAX(run.gl_range.y, i);
	}
	if (run.span_index >= 0) {
		p_sd->runs.push_back(run);
	}
	p_sd->runs_dirty = false;
}

int64_t TextServerAdvanced::_shaped_get_run_count(const RID& p_shaped) const
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, 0);
	MutexLock lock(sd->mutex);
	if (!sd->valid.is_set()) {
		const_cast<TextServerAdvanced*>(this)->_shaped_text_shape(p_shaped);
	}
	if (sd->runs_dirty) {
		_generate_runs(sd);
	}
	return sd->runs.size();
}

String TextServerAdvanced::_shaped_get_run_text(const RID& p_shaped, int64_t p_index) const
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, String());
	MutexLock lock(sd->mutex);
	if (!sd->valid.is_set()) {
		const_cast<TextServerAdvanced*>(this)->_shaped_text_shape(p_shaped);
	}
	if (sd->runs_dirty) {
		_generate_runs(sd);
	}
	ERR_FAIL_INDEX_V(p_index, sd->runs.size(), String());
	return sd->text.substr(sd->runs[p_index].range.x - sd->start,
		sd->runs[p_index].range.y - sd->runs[p_index].range.x);
}

Vector2i TextServerAdvanced::_shaped_get_run_range(const RID& p_shaped, int64_t p_index) const
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, Vector2i());
	MutexLock lock(sd->mutex);
	if (!sd->valid.is_set()) {
		const_cast<TextServerAdvanced*>(this)->_shaped_text_shape(p_shaped);
	}
	if (sd->runs_dirty) {
		_generate_runs(sd);
	}
	ERR_FAIL_INDEX_V(p_index, sd->runs.size(), Vector2i());
	return sd->runs[p_index].range;
}

Vector2i TextServerAdvanced::_shaped_get_run_glyph_range(const RID& p_shaped, int64_t p_index) const
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, Vector2i());
	MutexLock lock(sd->mutex);
	if (!sd->valid.is_set()) {
		const_cast<TextServerAdvanced*>(this)->_shaped_text_shape(p_shaped);
	}
	if (sd->runs_dirty) {
		_generate_runs(sd);
	}
	ERR_FAIL_INDEX_V(p_index, sd->runs.size(), Vector2i());
	return sd->runs[p_index].gl_range;
}

RID TextServerAdvanced::_shaped_get_run_font_rid(const RID& p_shaped, int64_t p_index) const
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, RID());
	MutexLock lock(sd->mutex);
	if (!sd->valid.is_set()) {
		const_cast<TextServerAdvanced*>(this)->_shaped_text_shape(p_shaped);
	}
	if (sd->runs_dirty) {
		_generate_runs(sd);
	}
	ERR_FAIL_INDEX_V(p_index, sd->runs.size(), RID());
	return sd->runs[p_index].font_rid;
}

int TextServerAdvanced::_shaped_get_run_font_size(const RID& p_shaped, int64_t p_index) const
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, 0);
	MutexLock lock(sd->mutex);
	if (!sd->valid.is_set()) {
		const_cast<TextServerAdvanced*>(this)->_shaped_text_shape(p_shaped);
	}
	if (sd->runs_dirty) {
		_generate_runs(sd);
	}
	ERR_FAIL_INDEX_V(p_index, sd->runs.size(), 0);
	return sd->runs[p_index].font_size;
}

String TextServerAdvanced::_shaped_get_run_language(const RID& p_shaped, int64_t p_index) const
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, String());
	MutexLock lock(sd->mutex);
	if (!sd->valid.is_set()) {
		const_cast<TextServerAdvanced*>(this)->_shaped_text_shape(p_shaped);
	}
	if (sd->runs_dirty) {
		_generate_runs(sd);
	}
	ERR_FAIL_INDEX_V(p_index, sd->runs.size(), String());

	int span_idx = sd->runs[p_index].span_index;
	ShapedTextDataAdvanced* span_sd = sd;
	if (sd->parent.is_valid()) {
		span_sd = shaped_owner.get_or_null(sd->parent);
		ERR_FAIL_NULL_V(span_sd, String());
	}
	ERR_FAIL_INDEX_V(span_idx, span_sd->spans.size(), String());
	return span_sd->spans[span_idx].language;
}

TextServer::Direction TextServerAdvanced::_shaped_get_run_direction(
	const RID& p_shaped, int64_t p_index) const
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, TextServer::DIRECTION_LTR);
	MutexLock lock(sd->mutex);
	if (!sd->valid.is_set()) {
		const_cast<TextServerAdvanced*>(this)->_shaped_text_shape(p_shaped);
	}
	if (sd->runs_dirty) {
		_generate_runs(sd);
	}
	ERR_FAIL_INDEX_V(p_index, sd->runs.size(), TextServer::DIRECTION_LTR);
	return sd->runs[p_index].rtl ? TextServer::DIRECTION_RTL : TextServer::DIRECTION_LTR;
}

String TextServerAdvanced::_shaped_get_text(const RID& p_shaped) const
{
	const ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, String());

	return sd->text;
}

RID TextServerAdvanced::_shaped_text_substr(
	const RID& p_shaped, int64_t p_start, int64_t p_length) const
{
	_THREAD_SAFE_METHOD_
	const ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, RID());

	MutexLock lock(sd->mutex);
	if (sd->parent != RID()) {
		return _shaped_text_substr(sd->parent, p_start, p_length);
	}
	if (!sd->valid.is_set()) {
		const_cast<TextServerAdvanced*>(this)->_shaped_text_shape(p_shaped);
	}
	ERR_FAIL_COND_V(p_start < 0 || p_length < 0, RID());
	ERR_FAIL_COND_V(sd->start > p_start || sd->end < p_start, RID());
	ERR_FAIL_COND_V(sd->end < p_start + p_length, RID());

	ShapedTextDataAdvanced* new_sd = memnew(ShapedTextDataAdvanced);
	new_sd->parent = p_shaped;
	new_sd->start = p_start;
	new_sd->end = p_start + p_length;
	new_sd->orientation = sd->orientation;
	new_sd->direction = sd->direction;
	new_sd->custom_punct = sd->custom_punct;
	new_sd->para_direction = sd->para_direction;
	new_sd->base_para_direction = sd->base_para_direction;
	for (int i = 0; i < TextServer::SPACING_MAX; i++) {
		new_sd->extra_spacing[i] = sd->extra_spacing[i];
	}

	if (!_shape_substr(new_sd, sd, p_start, p_length)) {
		memdelete(new_sd);
		return RID();
	}
	return shaped_owner.make_rid(new_sd);
}

RID TextServerAdvanced::_shaped_text_get_parent(const RID& p_shaped) const
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, RID());

	MutexLock lock(sd->mutex);
	return sd->parent;
}

bool TextServerAdvanced::_shaped_text_shape(const RID& p_shaped) { return true; }

double TextServerAdvanced::_shaped_text_tab_align(
	const RID& p_shaped, const PackedFloat32Array& p_tab_stops)
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, 0.0);

	MutexLock lock(sd->mutex);
	if (!sd->valid.is_set()) {
		_shaped_text_shape(p_shaped);
	}
	if (!sd->line_breaks_valid) {
		_shaped_text_update_breaks(p_shaped);
	}

	for (int i = 0; i < p_tab_stops.size(); i++) {
		if (p_tab_stops[i] <= 0) {
			return 0.0;
		}
	}

	int tab_index = 0;
	double off = 0.0;

	int start, end, delta;
	if (sd->para_direction == DIRECTION_LTR) {
		start = 0;
		end = sd->glyphs.size();
		delta = +1;
	}
	else {
		start = sd->glyphs.size() - 1;
		end = -1;
		delta = -1;
	}

	Glyph* gl = sd->glyphs.ptr();

	for (int i = start; i != end; i += delta) {
		if ((gl[i].flags & GRAPHEME_IS_TAB) == GRAPHEME_IS_TAB) {
			double tab_off = 0.0;
			while (tab_off <= off) {
				tab_off += p_tab_stops[tab_index];
				tab_index++;
				if (tab_index >= p_tab_stops.size()) {
					tab_index = 0;
				}
			}
			double old_adv = gl[i].advance;
			gl[i].advance = tab_off - off;
			sd->width += gl[i].advance - old_adv;
			off = 0;
			continue;
		}
		off += gl[i].advance * gl[i].repeat;
	}

	return 0.0;
}

int64_t TextServerAdvanced::_shaped_text_get_trim_pos(const RID& p_shaped) const
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V_MSG(sd, -1, "ShapedTextDataAdvanced invalid.");

	MutexLock lock(sd->mutex);
	return sd->overrun_trim_data.trim_pos;
}

int64_t TextServerAdvanced::_shaped_text_get_ellipsis_pos(const RID& p_shaped) const
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V_MSG(sd, -1, "ShapedTextDataAdvanced invalid.");

	MutexLock lock(sd->mutex);
	return sd->overrun_trim_data.ellipsis_pos;
}

const Glyph* TextServerAdvanced::_shaped_text_get_ellipsis_glyphs(const RID& p_shaped) const
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V_MSG(sd, nullptr, "ShapedTextDataAdvanced invalid.");

	MutexLock lock(sd->mutex);
	return sd->overrun_trim_data.ellipsis_glyph_buf.ptr();
}

int64_t TextServerAdvanced::_shaped_text_get_ellipsis_glyph_count(const RID& p_shaped) const
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V_MSG(sd, 0, "ShapedTextDataAdvanced invalid.");

	MutexLock lock(sd->mutex);
	return sd->overrun_trim_data.ellipsis_glyph_buf.size();
}

void TextServerAdvanced::_update_chars(ShapedTextDataAdvanced* p_sd) const
{
	if (!p_sd->chars_valid) {
		p_sd->chars.clear();

		const UChar* data = p_sd->utf16.get_data();
		UErrorCode err = U_ZERO_ERROR;
		int prev = -1;
		int i = 0;

		Vector<ShapedTextDataAdvanced::Span>& spans = p_sd->spans;
		if (p_sd->parent != RID()) {
			ShapedTextDataAdvanced* parent_sd = shaped_owner.get_or_null(p_sd->parent);
			ERR_FAIL_COND(!parent_sd->valid.is_set());
			spans = parent_sd->spans;
		}

		int span_size = spans.size();
		while (i < span_size) {
			if (spans[i].start > p_sd->end) {
				break;
			}
			if (spans[i].end < p_sd->start) {
				i++;
				continue;
			}

			int r_start = MAX(0, spans[i].start - p_sd->start);
			String language = spans[i].language;
			while (i + 1 < span_size && language == spans[i + 1].language) {
				i++;
			}
			int r_end = MIN(spans[i].end - p_sd->start, p_sd->text.length());
			UBreakIterator* bi = ubrk_open(UBRK_CHARACTER,
				(language.is_empty())
					? TranslationServer::get_tool_locale().ascii().get_data()
					: language.ascii().get_data(),
				data + _convert_pos_inv(p_sd, r_start), _convert_pos_inv(p_sd, r_end - r_start),
				&err);
			if (U_SUCCESS(err)) {
				while (ubrk_next(bi) != UBRK_DONE) {
					int pos = _convert_pos(p_sd, ubrk_current(bi)) + r_start + p_sd->start;
					if (prev != pos) {
						p_sd->chars.push_back(pos);
					}
					prev = pos;
				}
				ubrk_close(bi);
			}
			else {
				for (int j = r_start; j < r_end; j++) {
					if (prev != j) {
						p_sd->chars.push_back(j + 1 + p_sd->start);
					}
					prev = j;
				}
			}
			i++;
		}
		p_sd->chars_valid = true;
	}
}

PackedInt32Array TextServerAdvanced::_shaped_text_get_character_breaks(const RID& p_shaped) const
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, PackedInt32Array());

	MutexLock lock(sd->mutex);
	if (!sd->valid.is_set()) {
		const_cast<TextServerAdvanced*>(this)->_shaped_text_shape(p_shaped);
	}

	_update_chars(sd);

	return sd->chars;
}

_FORCE_INLINE_ int64_t TextServerAdvanced::_generate_kashida_justification_opportunities(
	const String& p_data, int64_t p_start, int64_t p_end)
{
	int64_t kashida_pos = -1;
	int8_t priority = 100;
	int64_t i = p_start;

	char32_t pc = 0;

	while ((p_end > p_start) && is_transparent(p_data[p_end - 1])) {
		p_end--;
	}

	while (i < p_end) {
		uint32_t c = p_data[i];

		if (c == 0x0640) {
			kashida_pos = i;
			priority = 0;
		}
		if (priority >= 1 && i < p_end - 1) {
			if (is_seen_sad(c) && (p_data[i + 1] != 0x200c)) {
				kashida_pos = i;
				priority = 1;
			}
		}
		if (priority >= 2 && i > p_start) {
			if (is_teh_marbuta(c) || is_dal(c) || (is_heh(c) && i == p_end - 1)) {
				if (is_connected_to_prev(c, pc)) {
					kashida_pos = i - 1;
					priority = 2;
				}
			}
		}
		if (priority >= 3 && i > p_start) {
			if (is_alef(c) ||
				((is_lam(c) || is_tah(c) || is_kaf(c) || is_gaf(c)) && i == p_end - 1)) {
				if (is_connected_to_prev(c, pc)) {
					kashida_pos = i - 1;
					priority = 3;
				}
			}
		}
		if (priority >= 4 && i > p_start && i < p_end - 1) {
			if (is_beh(c)) {
				if (is_reh(p_data[i + 1]) || is_yeh(p_data[i + 1])) {
					if (is_connected_to_prev(c, pc)) {
						kashida_pos = i - 1;
						priority = 4;
					}
				}
			}
		}
		if (priority >= 5 && i > p_start) {
			if (is_waw(c) || ((is_ain(c) || is_qaf(c) || is_feh(c)) && i == p_end - 1)) {
				if (is_connected_to_prev(c, pc)) {
					kashida_pos = i - 1;
					priority = 5;
				}
			}
		}
		if (priority >= 6 && i > p_start) {
			if (is_reh(c)) {
				if (is_connected_to_prev(c, pc)) {
					kashida_pos = i - 1;
					priority = 6;
				}
			}
		}
		if (!is_transparent(c)) {
			pc = c;
		}
		i++;
	}

	return kashida_pos;
}

bool TextServerAdvanced::_shaped_text_update_justification_ops(const RID& p_shaped)
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, false);

	MutexLock lock(sd->mutex);
	if (!sd->valid.is_set()) {
		_shaped_text_shape(p_shaped);
	}
	if (!sd->line_breaks_valid) {
		_shaped_text_update_breaks(p_shaped);
	}

	if (sd->justification_ops_valid) {
		return true; // Nothing to do.
	}

	const UChar* data = sd->utf16.get_data();
	int data_size = sd->utf16.length();

	if (!sd->js_ops_valid) {
		sd->jstops.clear();

		// Use ICU word iterator and custom kashida detection.
		UErrorCode err = U_ZERO_ERROR;
		UBreakIterator* bi = ubrk_open(UBRK_WORD, "", data, data_size, &err);
		if (U_FAILURE(err)) {
			// No data - use fallback.
			int limit = 0;
			for (int i = 0; i < sd->text.length(); i++) {
				if (is_whitespace(sd->text[i])) {
					int ks = _generate_kashida_justification_opportunities(sd->text, limit, i) +
							 sd->start;
					if (ks != -1) {
						sd->jstops[ks] = true;
					}
					limit = i + 1;
				}
			}
			int ks =
				_generate_kashida_justification_opportunities(sd->text, limit, sd->text.length()) +
				sd->start;
			if (ks != -1) {
				sd->jstops[ks] = true;
			}
		}
		else {
			int limit = 0;
			while (ubrk_next(bi) != UBRK_DONE) {
				if (ubrk_getRuleStatus(bi) != UBRK_WORD_NONE) {
					int i = _convert_pos(sd, ubrk_current(bi));
					sd->jstops[i + sd->start] = false;
					int ks = _generate_kashida_justification_opportunities(sd->text, limit, i);
					if (ks != -1) {
						sd->jstops[ks + sd->start] = true;
					}
					limit = i;
				}
			}
			ubrk_close(bi);
		}

		sd->js_ops_valid = true;
	}

	sd->sort_valid = false;
	sd->glyphs_logical.clear();

	Glyph* sd_glyphs = sd->glyphs.ptr();
	int sd_size = sd->glyphs.size();
	if (!sd->jstops.is_empty()) {
		for (int i = 0; i < sd_size; i++) {
			if (sd_glyphs[i].count > 0) {
				char32_t c = sd->text[sd_glyphs[i].start - sd->start];
				if (c == 0x0640 && sd_glyphs[i].start == sd_glyphs[i].end - 1) {
					sd_glyphs[i].flags |= GRAPHEME_IS_ELONGATION;
				}
				if (sd->jstops.has(sd_glyphs[i].start)) {
					if (c == 0xfffc || c == 0x00ad) {
						continue;
					}
					if (sd->jstops[sd_glyphs[i].start]) {
						if (c != 0x0640) {
							if (sd_glyphs[i].font_rid != RID()) {
								Glyph gl = _shape_single_glyph(sd, 0x0640, HB_SCRIPT_ARABIC,
									HB_DIRECTION_RTL, sd->glyphs[i].font_rid,
									sd->glyphs[i].font_size);
								if ((sd_glyphs[i].flags & GRAPHEME_IS_VALID) == GRAPHEME_IS_VALID) {
#if HB_VERSION_ATLEAST(5, 1, 0)
									if ((i > 0) && ((sd_glyphs[i - 1].flags &
														GRAPHEME_IS_SAFE_TO_INSERT_TATWEEL) !=
													   GRAPHEME_IS_SAFE_TO_INSERT_TATWEEL)) {
										continue;
									}
#endif
									gl.start = sd_glyphs[i].start;
									gl.end = sd_glyphs[i].end;
									gl.repeat = 0;
									gl.count = 1;
									if (sd->orientation == ORIENTATION_HORIZONTAL) {
										gl.y_off = sd_glyphs[i].y_off;
									}
									else {
										gl.x_off = sd_glyphs[i].x_off;
									}
									gl.flags |= GRAPHEME_IS_ELONGATION | GRAPHEME_IS_VIRTUAL;
									sd->glyphs.insert(i, gl);
									i++;

									// Update write pointer and size.
									sd_size = sd->glyphs.size();
									sd_glyphs = sd->glyphs.ptr();
									continue;
								}
							}
						}
					}
					else if ((sd_glyphs[i].flags & GRAPHEME_IS_SPACE) != GRAPHEME_IS_SPACE &&
							   (sd_glyphs[i].flags & GRAPHEME_IS_PUNCTUATION) !=
								   GRAPHEME_IS_PUNCTUATION) {
						int count = sd_glyphs[i].count;
						// Do not add extra spaces at the end of the line.
						if (sd_glyphs[i].end == sd->end) {
							continue;
						}
						// Do not add extra space after existing space.
						if (sd_glyphs[i].flags & GRAPHEME_IS_RTL) {
							if ((i + count < sd_size - 1) &&
								((sd_glyphs[i + count].flags &
									 (GRAPHEME_IS_SPACE | GRAPHEME_IS_BREAK_SOFT)) ==
									(GRAPHEME_IS_SPACE | GRAPHEME_IS_BREAK_SOFT))) {
								continue;
							}
						}
						else {
							if ((i > 0) && ((sd_glyphs[i - 1].flags &
												(GRAPHEME_IS_SPACE | GRAPHEME_IS_BREAK_SOFT)) ==
											   (GRAPHEME_IS_SPACE | GRAPHEME_IS_BREAK_SOFT))) {
								continue;
							}
						}
						// Inject virtual space for alignment.
						Glyph gl;
						gl.span_index = sd_glyphs[i].span_index;
						gl.start = sd_glyphs[i].start;
						gl.end = sd_glyphs[i].end;
						gl.count = 1;
						gl.font_rid = sd_glyphs[i].font_rid;
						gl.font_size = sd_glyphs[i].font_size;
						gl.flags = GRAPHEME_IS_SPACE | GRAPHEME_IS_VIRTUAL;
						if (sd_glyphs[i].flags & GRAPHEME_IS_RTL) {
							gl.flags |= GRAPHEME_IS_RTL;
							sd->glyphs.insert(i, gl); // Insert before.
						}
						else {
							sd->glyphs.insert(i + count, gl); // Insert after.
						}
						i += count;

						// Update write pointer and size.
						sd_size = sd->glyphs.size();
						sd_glyphs = sd->glyphs.ptr();
						continue;
					}
				}
			}
		}
	}

	sd->justification_ops_valid = true;
	return sd->justification_ops_valid;
}

Glyph TextServerAdvanced::_shape_single_glyph(ShapedTextDataAdvanced* p_sd, char32_t p_char,
	hb_script_t p_script, hb_direction_t p_direction, const RID& p_font, int64_t p_font_size)
{
	bool color = false;
	hb_font_t* hb_font = _font_get_hb_handle(p_font, p_font_size, color);
	double scale = _font_get_scale(p_font, p_font_size);
	bool subpos = (scale != 1.0) ||
				  (_font_get_subpixel_positioning(p_font) == SUBPIXEL_POSITIONING_ONE_HALF) ||
				  (_font_get_subpixel_positioning(p_font) == SUBPIXEL_POSITIONING_ONE_QUARTER) ||
				  (_font_get_subpixel_positioning(p_font) == SUBPIXEL_POSITIONING_AUTO &&
					  p_font_size <= SUBPIXEL_POSITIONING_ONE_HALF_MAX_SIZE);
	ERR_FAIL_NULL_V(hb_font, Glyph());

	hb_buffer_clear_contents(p_sd->hb_buffer);
	hb_buffer_set_direction(p_sd->hb_buffer, p_direction);
	hb_buffer_set_flags(p_sd->hb_buffer, (hb_buffer_flags_t)(HB_BUFFER_FLAG_DEFAULT));
	hb_buffer_set_script(
		p_sd->hb_buffer, (p_script == HB_TAG('Z', 's', 'y', 'e')) ? HB_SCRIPT_COMMON : p_script);
	hb_buffer_add_utf32(p_sd->hb_buffer, (const uint32_t*)&p_char, 1, 0, 1);

	hb_shape(hb_font, p_sd->hb_buffer, nullptr, 0);

	unsigned int glyph_count = 0;
	hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(p_sd->hb_buffer, &glyph_count);
	hb_glyph_position_t* glyph_pos = hb_buffer_get_glyph_positions(p_sd->hb_buffer, &glyph_count);

	// Process glyphs.
	Glyph gl;

	if (p_direction == HB_DIRECTION_RTL || p_direction == HB_DIRECTION_BTT) {
		gl.flags |= TextServer::GRAPHEME_IS_RTL;
	}

	gl.font_rid = p_font;
	gl.font_size = p_font_size;

	if (glyph_count > 0) {
		if (p_sd->orientation == ORIENTATION_HORIZONTAL) {
			if (subpos) {
				gl.advance = (double)glyph_pos[0].x_advance / (64.0 / scale) +
							 _get_extra_advance(p_font, p_font_size);
			}
			else {
				gl.advance = Math::round((double)glyph_pos[0].x_advance / (64.0 / scale) +
										 _get_extra_advance(p_font, p_font_size));
			}
		}
		else {
			gl.advance = -Math::round((double)glyph_pos[0].y_advance / (64.0 / scale));
		}
		gl.count = 1;

		gl.index = glyph_info[0].codepoint;
		if (subpos) {
			gl.x_off = (double)glyph_pos[0].x_offset / (64.0 / scale);
		}
		else {
			gl.x_off = Math::round((double)glyph_pos[0].x_offset / (64.0 / scale));
		}
		gl.y_off = -Math::round((double)glyph_pos[0].y_offset / (64.0 / scale));
		if (p_sd->orientation == ORIENTATION_HORIZONTAL) {
			gl.y_off += _font_get_baseline_offset(gl.font_rid) *
						(double)(_font_get_ascent(gl.font_rid, gl.font_size) +
								 _font_get_descent(gl.font_rid, gl.font_size));
		}
		else {
			gl.x_off += _font_get_baseline_offset(gl.font_rid) *
						(double)(_font_get_ascent(gl.font_rid, gl.font_size) +
								 _font_get_descent(gl.font_rid, gl.font_size));
		}

		if ((glyph_info[0].codepoint != 0) || !u_isgraph(p_char)) {
			gl.flags |= GRAPHEME_IS_VALID;
		}
	}
	return gl;
}

UBreakIterator* TextServerAdvanced::_create_line_break_iterator_for_locale(
	const String& p_language, UErrorCode* r_err) const
{
	// Creating UBreakIterator (ubrk_open) is surprisingly costly.
	// However, cloning (ubrk_clone) is cheaper, so we keep around blueprints to accelerate creating
	// new ones.

	String language =
		p_language.is_empty() ? TranslationServer::get_tool_locale() : p_language;
	if (!language.contains("@")) {
		if (lb_strictness == LB_LOOSE) {
			language += "@lb=loose";
		}
		else if (lb_strictness == LB_NORMAL) {
			language += "@lb=normal";
		}
		else if (lb_strictness == LB_STRICT) {
			language += "@lb=strict";
		}
	}

	_THREAD_SAFE_METHOD_
	const HashMap<String, UBreakIterator*>::Iterator key_value =
		line_break_iterators_per_language.find(language);
	if (key_value) {
		return ubrk_clone(key_value->value, r_err);
	}
	UBreakIterator* bi = ubrk_open(UBRK_LINE, language.ascii().get_data(), nullptr, 0, r_err);
	if (U_FAILURE(*r_err) || !bi) {
		return nullptr;
	}
	line_break_iterators_per_language.insert(language, bi);
	return ubrk_clone(bi, r_err);
}

bool TextServerAdvanced::_shaped_text_is_ready(const RID& p_shaped) const
{
	const ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, false);

	// Atomic read is safe and faster.
	return sd->valid.is_set();
}

const Glyph* TextServerAdvanced::_shaped_text_get_glyphs(const RID& p_shaped) const
{
	const ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, nullptr);

	MutexLock lock(sd->mutex);
	if (!sd->valid.is_set()) {
		const_cast<TextServerAdvanced*>(this)->_shaped_text_shape(p_shaped);
	}
	return sd->glyphs.ptr();
}

int64_t TextServerAdvanced::_shaped_text_get_glyph_count(const RID& p_shaped) const
{
	const ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, 0);

	MutexLock lock(sd->mutex);
	if (!sd->valid.is_set()) {
		const_cast<TextServerAdvanced*>(this)->_shaped_text_shape(p_shaped);
	}
	return sd->glyphs.size();
}

const Glyph* TextServerAdvanced::_shaped_text_sort_logical(const RID& p_shaped)
{
	ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, nullptr);

	MutexLock lock(sd->mutex);
	if (!sd->valid.is_set()) {
		_shaped_text_shape(p_shaped);
	}

	if (!sd->sort_valid) {
		sd->glyphs_logical = sd->glyphs;
		sd->glyphs_logical.sort_custom<GlyphCompare>();
		sd->sort_valid = true;
	}

	return sd->glyphs_logical.ptr();
}

Vector2i TextServerAdvanced::_shaped_text_get_range(const RID& p_shaped) const
{
	const ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, Vector2i());

	MutexLock lock(sd->mutex);
	return Vector2(sd->start, sd->end);
}

Size2 TextServerAdvanced::_shaped_text_get_size(const RID& p_shaped) const
{
	const ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, Size2());

	MutexLock lock(sd->mutex);
	if (!sd->valid.is_set()) {
		const_cast<TextServerAdvanced*>(this)->_shaped_text_shape(p_shaped);
	}
	if (sd->orientation == TextServer::ORIENTATION_HORIZONTAL) {
		return Size2((sd->text_trimmed ? sd->width_trimmed : sd->width),
			sd->ascent + sd->descent + sd->extra_spacing[SPACING_TOP] +
				sd->extra_spacing[SPACING_BOTTOM])
			.ceil();
	}
	else {
		return Size2(sd->ascent + sd->descent + sd->extra_spacing[SPACING_TOP] +
						 sd->extra_spacing[SPACING_BOTTOM],
			(sd->text_trimmed ? sd->width_trimmed : sd->width))
			.ceil();
	}
}

double TextServerAdvanced::_shaped_text_get_ascent(const RID& p_shaped) const
{
	const ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, 0.0);

	MutexLock lock(sd->mutex);
	if (!sd->valid.is_set()) {
		const_cast<TextServerAdvanced*>(this)->_shaped_text_shape(p_shaped);
	}
	return sd->ascent + sd->extra_spacing[SPACING_TOP];
}

double TextServerAdvanced::_shaped_text_get_descent(const RID& p_shaped) const
{
	const ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, 0.0);

	MutexLock lock(sd->mutex);
	if (!sd->valid.is_set()) {
		const_cast<TextServerAdvanced*>(this)->_shaped_text_shape(p_shaped);
	}
	return sd->descent + sd->extra_spacing[SPACING_BOTTOM];
}

double TextServerAdvanced::_shaped_text_get_width(const RID& p_shaped) const
{
	const ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, 0.0);

	MutexLock lock(sd->mutex);
	if (!sd->valid.is_set()) {
		const_cast<TextServerAdvanced*>(this)->_shaped_text_shape(p_shaped);
	}
	return Math::ceil(sd->text_trimmed ? sd->width_trimmed : sd->width);
}

double TextServerAdvanced::_shaped_text_get_underline_position(const RID& p_shaped) const
{
	const ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, 0.0);

	MutexLock lock(sd->mutex);
	if (!sd->valid.is_set()) {
		const_cast<TextServerAdvanced*>(this)->_shaped_text_shape(p_shaped);
	}

	return sd->upos;
}

double TextServerAdvanced::_shaped_text_get_underline_thickness(const RID& p_shaped) const
{
	const ShapedTextDataAdvanced* sd = shaped_owner.get_or_null(p_shaped);
	ERR_FAIL_NULL_V(sd, 0.0);

	MutexLock lock(sd->mutex);
	if (!sd->valid.is_set()) {
		const_cast<TextServerAdvanced*>(this)->_shaped_text_shape(p_shaped);
	}

	return sd->uthk;
}

int64_t TextServerAdvanced::_is_confusable(
	const String& p_string, const PackedStringArray& p_dict) const
{
#ifndef ICU_STATIC_DATA
	if (!icu_data_loaded) {
		return -1;
	}
#endif
	UErrorCode status = U_ZERO_ERROR;
	int64_t match_index = -1;

	Char16String utf16 = p_string.utf16();
	Vector<UChar*> skeletons;
	skeletons.resize(p_dict.size());

	if (sc_conf == nullptr) {
		sc_conf = uspoof_open(&status);
		uspoof_setChecks(sc_conf, USPOOF_CONFUSABLE, &status);
	}
	for (int i = 0; i < p_dict.size(); i++) {
		Char16String word = p_dict[i].utf16();
		int32_t len = uspoof_getSkeleton(sc_conf, 0, word.get_data(), -1, nullptr, 0, &status);
		skeletons.write[i] = (UChar*)memalloc(++len * sizeof(UChar));
		status = U_ZERO_ERROR;
		uspoof_getSkeleton(sc_conf, 0, word.get_data(), -1, skeletons.write[i], len, &status);
	}

	int32_t len = uspoof_getSkeleton(sc_conf, 0, utf16.get_data(), -1, nullptr, 0, &status);
	UChar* skel = (UChar*)memalloc(++len * sizeof(UChar));
	status = U_ZERO_ERROR;
	uspoof_getSkeleton(sc_conf, 0, utf16.get_data(), -1, skel, len, &status);
	for (int i = 0; i < skeletons.size(); i++) {
		if (u_strcmp(skel, skeletons[i]) == 0) {
			match_index = i;
			break;
		}
	}
	memfree(skel);

	for (int i = 0; i < skeletons.size(); i++) {
		memfree(skeletons.write[i]);
	}

	ERR_FAIL_COND_V_MSG(U_FAILURE(status), -1, u_errorName(status));

	return match_index;
}

bool TextServerAdvanced::_spoof_check(const String& p_string) const
{
#ifndef ICU_STATIC_DATA
	if (!icu_data_loaded) {
		return false;
	}
#endif
	UErrorCode status = U_ZERO_ERROR;
	Char16String utf16 = p_string.utf16();

	if (allowed == nullptr) {
		allowed = uset_openEmpty();
		uset_addAll(allowed, uspoof_getRecommendedSet(&status));
		uset_addAll(allowed, uspoof_getInclusionSet(&status));
	}
	if (sc_spoof == nullptr) {
		sc_spoof = uspoof_open(&status);
		uspoof_setAllowedChars(sc_spoof, allowed, &status);
		uspoof_setRestrictionLevel(sc_spoof, USPOOF_MODERATELY_RESTRICTIVE);
	}

	int32_t bitmask = uspoof_check(sc_spoof, utf16.get_data(), -1, nullptr, &status);
	ERR_FAIL_COND_V_MSG(U_FAILURE(status), false, u_errorName(status));

	return (bitmask != 0);
}

String TextServerAdvanced::_strip_diacritics(const String& p_string) const
{
#ifndef ICU_STATIC_DATA
	if (!icu_data_loaded) {
		return TextServer::strip_diacritics(p_string);
	}
#endif
	UErrorCode err = U_ZERO_ERROR;

	// Get NFKD normalizer singleton.
	const UNormalizer2* unorm = unorm2_getNFKDInstance(&err);
	ERR_FAIL_COND_V_MSG(U_FAILURE(err), TextServer::strip_diacritics(p_string), u_errorName(err));

	// Convert to UTF-16.
	Char16String utf16 = p_string.utf16();

	// Normalize.
	Vector<char16_t> normalized;
	err = U_ZERO_ERROR;
	int32_t len = unorm2_normalize(unorm, utf16.get_data(), -1, nullptr, 0, &err);
	ERR_FAIL_COND_V_MSG(
		err != U_BUFFER_OVERFLOW_ERROR, TextServer::strip_diacritics(p_string), u_errorName(err));
	normalized.resize(len);
	err = U_ZERO_ERROR;
	unorm2_normalize(unorm, utf16.get_data(), -1, normalized.ptrw(), len, &err);
	ERR_FAIL_COND_V_MSG(U_FAILURE(err), TextServer::strip_diacritics(p_string), u_errorName(err));

	// Convert back to UTF-32.
	String normalized_string = String::utf16(normalized.ptr(), len);

	// Strip combining characters.
	String result;
	for (int i = 0; i < normalized_string.length(); i++) {
		if (u_getCombiningClass(normalized_string[i]) == 0) {
			result = result + normalized_string[i];
		}
	}
	return result;
}

String TextServerAdvanced::_string_to_upper(const String& p_string, const String& p_language) const
{
#ifndef ICU_STATIC_DATA
	if (!icu_data_loaded) {
		return p_string.to_upper();
	}
#endif

	if (p_string.is_empty()) {
		return p_string;
	}
	const String lang = (p_language.is_empty())
							? TranslationServer::get_tool_locale()
							: p_language;

	// Convert to UTF-16.
	Char16String utf16 = p_string.utf16();

	Vector<char16_t> upper;
	UErrorCode err = U_ZERO_ERROR;
	int32_t len = u_strToUpper(nullptr, 0, utf16.get_data(), -1, lang.ascii().get_data(), &err);
	ERR_FAIL_COND_V_MSG(err != U_BUFFER_OVERFLOW_ERROR, p_string, u_errorName(err));
	upper.resize(len);
	err = U_ZERO_ERROR;
	u_strToUpper(upper.ptrw(), len, utf16.get_data(), -1, lang.ascii().get_data(), &err);
	ERR_FAIL_COND_V_MSG(U_FAILURE(err), p_string, u_errorName(err));

	// Convert back to UTF-32.
	return String::utf16(upper.ptr(), len);
}

String TextServerAdvanced::_string_to_lower(const String& p_string, const String& p_language) const
{
#ifndef ICU_STATIC_DATA
	if (!icu_data_loaded) {
		return p_string.to_lower();
	}
#endif

	if (p_string.is_empty()) {
		return p_string;
	}
	const String lang = (p_language.is_empty())
							? TranslationServer::get_tool_locale()
							: p_language;
	// Convert to UTF-16.
	Char16String utf16 = p_string.utf16();

	Vector<char16_t> lower;
	UErrorCode err = U_ZERO_ERROR;
	int32_t len = u_strToLower(nullptr, 0, utf16.get_data(), -1, lang.ascii().get_data(), &err);
	ERR_FAIL_COND_V_MSG(err != U_BUFFER_OVERFLOW_ERROR, p_string, u_errorName(err));
	lower.resize(len);
	err = U_ZERO_ERROR;
	u_strToLower(lower.ptrw(), len, utf16.get_data(), -1, lang.ascii().get_data(), &err);
	ERR_FAIL_COND_V_MSG(U_FAILURE(err), p_string, u_errorName(err));

	// Convert back to UTF-32.
	return String::utf16(lower.ptr(), len);
}

String TextServerAdvanced::_string_to_title(const String& p_string, const String& p_language) const
{
#ifndef ICU_STATIC_DATA
	if (!icu_data_loaded) {
		return p_string.capitalize();
	}
#endif

	if (p_string.is_empty()) {
		return p_string;
	}
	const String lang = (p_language.is_empty())
							? TranslationServer::get_tool_locale()
							: p_language;

	// Convert to UTF-16.
	Char16String utf16 = p_string.utf16();

	Vector<char16_t> upper;
	UErrorCode err = U_ZERO_ERROR;
	int32_t len =
		u_strToTitle(nullptr, 0, utf16.get_data(), -1, nullptr, lang.ascii().get_data(), &err);
	ERR_FAIL_COND_V_MSG(err != U_BUFFER_OVERFLOW_ERROR, p_string, u_errorName(err));
	upper.resize(len);
	err = U_ZERO_ERROR;
	u_strToTitle(upper.ptrw(), len, utf16.get_data(), -1, nullptr, lang.ascii().get_data(), &err);
	ERR_FAIL_COND_V_MSG(U_FAILURE(err), p_string, u_errorName(err));

	// Convert back to UTF-32.
	return String::utf16(upper.ptr(), len);
}

PackedInt32Array TextServerAdvanced::_string_get_word_breaks(
	const String& p_string, const String& p_language, int64_t p_chars_per_line) const
{
	const String lang = (p_language.is_empty())
							? TranslationServer::get_tool_locale()
							: p_language;
	// Convert to UTF-16.
	Char16String utf16 = p_string.utf16();

	HashSet<int> breaks;
	UErrorCode err = U_ZERO_ERROR;
	UBreakIterator* bi = ubrk_open(
		UBRK_WORD, lang.ascii().get_data(), (const UChar*)utf16.get_data(), utf16.length(), &err);
	if (U_SUCCESS(err)) {
		while (ubrk_next(bi) != UBRK_DONE) {
			int pos = _convert_pos(p_string, utf16, ubrk_current(bi));
			if (pos != p_string.length() - 1) {
				breaks.insert(pos);
			}
		}
		ubrk_close(bi);
	}

	PackedInt32Array ret;

	if (p_chars_per_line > 0) {
		int line_start = 0;
		int last_break = -1;
		int line_length = 0;

		for (int i = 0; i < p_string.length(); i++) {
			const char32_t c = p_string[i];

			bool is_lb = is_linebreak(c);
			bool is_ws = is_whitespace(c);
			bool is_p =
				(u_ispunct(c) && c != 0x005F) || is_underscore(c) || c == '\t' || c == 0xfffc;

			if (is_lb) {
				if (line_length > 0) {
					ret.push_back(line_start);
					ret.push_back(i);
				}
				line_start = i;
				line_length = 0;
				last_break = -1;
				continue;
			}
			else if (breaks.has(i) || is_ws || is_p) {
				last_break = i;
			}

			if (line_length == p_chars_per_line) {
				if (last_break != -1) {
					int last_break_w_spaces = last_break;
					while (last_break > line_start && is_whitespace(p_string[last_break - 1])) {
						last_break--;
					}
					if (line_start != last_break) {
						ret.push_back(line_start);
						ret.push_back(last_break);
					}
					while (last_break_w_spaces < p_string.length() &&
						   is_whitespace(p_string[last_break_w_spaces])) {
						last_break_w_spaces++;
					}
					line_start = last_break_w_spaces;
					if (last_break_w_spaces < i) {
						line_length = i - last_break_w_spaces;
					}
					else {
						i = last_break_w_spaces;
						line_length = 0;
					}
				}
				else {
					ret.push_back(line_start);
					ret.push_back(i);
					line_start = i;
					line_length = 0;
				}
				last_break = -1;
			}
			line_length++;
		}
		if (line_length > 0) {
			ret.push_back(line_start);
			ret.push_back(p_string.length());
		}
	}
	else {
		int word_start = 0; // -1 if no word encountered. Leading spaces are part of a word.
		int word_length = 0;

		for (int i = 0; i < p_string.length(); i++) {
			const char32_t c = p_string[i];

			bool is_lb = is_linebreak(c);
			bool is_ws = is_whitespace(c);
			bool is_p =
				(u_ispunct(c) && c != 0x005F) || is_underscore(c) || c == '\t' || c == 0xfffc;

			if (word_start == -1) {
				if (!is_lb && !is_ws && !is_p) {
					word_start = i;
				}
				continue;
			}

			if (is_lb) {
				if (word_start != -1 && word_length > 0) {
					ret.push_back(word_start);
					ret.push_back(i);
				}
				word_start = -1;
				word_length = 0;
			}
			else if (breaks.has(i) || is_ws || is_p) {
				if (word_start != -1 && word_length > 0) {
					ret.push_back(word_start);
					ret.push_back(i);
				}
				if (is_ws || is_p) {
					word_start = -1;
				}
				else {
					word_start = i;
				}
				word_length = 0;
			}

			word_length++;
		}
		if (word_start != -1 && word_length > 0) {
			ret.push_back(word_start);
			ret.push_back(p_string.length());
		}
	}

	return ret;
}

PackedInt32Array TextServerAdvanced::_string_get_character_breaks(
	const String& p_string, const String& p_language) const
{
	const String lang = (p_language.is_empty())
							? TranslationServer::get_tool_locale()
							: p_language;
	// Convert to UTF-16.
	Char16String utf16 = p_string.utf16();

	PackedInt32Array ret;

	UErrorCode err = U_ZERO_ERROR;
	UBreakIterator* bi = ubrk_open(UBRK_CHARACTER, lang.ascii().get_data(),
		(const UChar*)utf16.get_data(), utf16.length(), &err);
	if (U_SUCCESS(err)) {
		while (ubrk_next(bi) != UBRK_DONE) {
			int pos = _convert_pos(p_string, utf16, ubrk_current(bi));
			ret.push_back(pos);
		}
		ubrk_close(bi);
	}
	else {
		return TextServer::string_get_character_breaks(p_string, p_language);
	}

	return ret;
}

bool TextServerAdvanced::_is_valid_identifier(const String& p_string) const
{
#ifndef ICU_STATIC_DATA
	if (!icu_data_loaded) {
		WARN_PRINT_ONCE(
			"ICU data is not loaded, Unicode security and spoofing detection disabled.");
		return TextServer::is_valid_identifier(p_string);
	}
#endif

	enum UAX31SequenceStatus
	{
		SEQ_NOT_STARTED,
		SEQ_STARTED,
		SEQ_STARTED_VIR,
		SEQ_NEAR_END,
	};

	const char32_t* str = p_string.ptr();
	int len = p_string.length();

	if (len == 0) {
		return false; // Empty string.
	}

	UErrorCode err = U_ZERO_ERROR;
	Char16String utf16 = p_string.utf16();
	const UNormalizer2* norm_c = unorm2_getNFCInstance(&err);
	if (U_FAILURE(err)) {
		return false; // Failed to load normalizer.
	}
	bool isnurom = unorm2_isNormalized(norm_c, utf16.get_data(), utf16.length(), &err);
	if (U_FAILURE(err) || !isnurom) {
		return false; // Do not conform to Normalization Form C.
	}

	UAX31SequenceStatus A1_sequence_status = SEQ_NOT_STARTED;
	UScriptCode A1_scr = USCRIPT_INHERITED;
	UAX31SequenceStatus A2_sequence_status = SEQ_NOT_STARTED;
	UScriptCode A2_scr = USCRIPT_INHERITED;
	UAX31SequenceStatus B_sequence_status = SEQ_NOT_STARTED;
	UScriptCode B_scr = USCRIPT_INHERITED;

	for (int i = 0; i < len; i++) {
		err = U_ZERO_ERROR;
		UScriptCode scr = uscript_getScript(str[i], &err);
		if (U_FAILURE(err)) {
			return false; // Invalid script.
		}
		if (uscript_getUsage(scr) != USCRIPT_USAGE_RECOMMENDED) {
			return false; // Not a recommended script.
		}
		uint8_t cat = u_charType(str[i]);
		int32_t jt = u_getIntPropertyValue(str[i], UCHAR_JOINING_TYPE);

		// UAX #31 section 2.3 subsections A1, A2 and B, check ZWNJ and ZWJ usage.
		switch (A1_sequence_status) {
		case SEQ_NEAR_END: {
			if ((A1_scr > USCRIPT_INHERITED) && (scr > USCRIPT_INHERITED) && (scr != A1_scr)) {
				return false; // Mixed script.
			}
			if (jt == U_JT_RIGHT_JOINING || jt == U_JT_DUAL_JOINING) {
				A1_sequence_status = SEQ_NOT_STARTED; // Valid end of sequence, reset.
			}
			else if (jt != U_JT_TRANSPARENT) {
				return false; // Invalid end of sequence.
			}
		} break;
		case SEQ_STARTED: {
			if ((A1_scr > USCRIPT_INHERITED) && (scr > USCRIPT_INHERITED) && (scr != A1_scr)) {
				A1_sequence_status = SEQ_NOT_STARTED; // Reset.
			}
			else {
				if (jt != U_JT_TRANSPARENT) {
					if (str[i] == 0x200C /*ZWNJ*/) {
						A1_sequence_status = SEQ_NEAR_END;
						continue;
					}
					else {
						A1_sequence_status = SEQ_NOT_STARTED; // Reset.
					}
				}
			}
		} break;
		default:
			break;
		}
		if (A1_sequence_status == SEQ_NOT_STARTED) {
			if (jt == U_JT_LEFT_JOINING || jt == U_JT_DUAL_JOINING) {
				A1_sequence_status = SEQ_STARTED;
				A1_scr = scr;
			}
		};

		switch (A2_sequence_status) {
		case SEQ_NEAR_END: {
			if ((A2_scr > USCRIPT_INHERITED) && (scr > USCRIPT_INHERITED) && (scr != A2_scr)) {
				return false; // Mixed script.
			}
			if (cat == U_UPPERCASE_LETTER || cat == U_LOWERCASE_LETTER ||
				cat == U_TITLECASE_LETTER || cat == U_MODIFIER_LETTER || cat == U_OTHER_LETTER) {
				A2_sequence_status = SEQ_NOT_STARTED; // Valid end of sequence, reset.
			}
			else if (cat != U_MODIFIER_LETTER || u_getCombiningClass(str[i]) == 0) {
				return false; // Invalid end of sequence.
			}
		} break;
		case SEQ_STARTED_VIR: {
			if ((A2_scr > USCRIPT_INHERITED) && (scr > USCRIPT_INHERITED) && (scr != A2_scr)) {
				A2_sequence_status = SEQ_NOT_STARTED; // Reset.
			}
			else {
				if (str[i] == 0x200C /*ZWNJ*/) {
					A2_sequence_status = SEQ_NEAR_END;
					continue;
				}
				else if (cat != U_MODIFIER_LETTER || u_getCombiningClass(str[i]) == 0) {
					A2_sequence_status = SEQ_NOT_STARTED; // Reset.
				}
			}
		} break;
		case SEQ_STARTED: {
			if ((A2_scr > USCRIPT_INHERITED) && (scr > USCRIPT_INHERITED) && (scr != A2_scr)) {
				A2_sequence_status = SEQ_NOT_STARTED; // Reset.
			}
			else {
				if (u_getCombiningClass(str[i]) == 9 /*Virama Combining Class*/) {
					A2_sequence_status = SEQ_STARTED_VIR;
				}
				else if (cat != U_MODIFIER_LETTER) {
					A2_sequence_status = SEQ_NOT_STARTED; // Reset.
				}
			}
		} break;
		default:
			break;
		}
		if (A2_sequence_status == SEQ_NOT_STARTED) {
			if (cat == U_UPPERCASE_LETTER || cat == U_LOWERCASE_LETTER ||
				cat == U_TITLECASE_LETTER || cat == U_MODIFIER_LETTER || cat == U_OTHER_LETTER) {
				A2_sequence_status = SEQ_STARTED;
				A2_scr = scr;
			}
		}

		switch (B_sequence_status) {
		case SEQ_NEAR_END: {
			if ((B_scr > USCRIPT_INHERITED) && (scr > USCRIPT_INHERITED) && (scr != B_scr)) {
				return false; // Mixed script.
			}
			if (u_getIntPropertyValue(str[i], UCHAR_INDIC_SYLLABIC_CATEGORY) !=
				U_INSC_VOWEL_DEPENDENT) {
				B_sequence_status = SEQ_NOT_STARTED; // Valid end of sequence, reset.
			}
			else {
				return false; // Invalid end of sequence.
			}
		} break;
		case SEQ_STARTED_VIR: {
			if ((B_scr > USCRIPT_INHERITED) && (scr > USCRIPT_INHERITED) && (scr != B_scr)) {
				B_sequence_status = SEQ_NOT_STARTED; // Reset.
			}
			else {
				if (str[i] == 0x200D /*ZWJ*/) {
					B_sequence_status = SEQ_NEAR_END;
					continue;
				}
				else if (cat != U_MODIFIER_LETTER || u_getCombiningClass(str[i]) == 0) {
					B_sequence_status = SEQ_NOT_STARTED; // Reset.
				}
			}
		} break;
		case SEQ_STARTED: {
			if ((B_scr > USCRIPT_INHERITED) && (scr > USCRIPT_INHERITED) && (scr != B_scr)) {
				B_sequence_status = SEQ_NOT_STARTED; // Reset.
			}
			else {
				if (u_getCombiningClass(str[i]) == 9 /*Virama Combining Class*/) {
					B_sequence_status = SEQ_STARTED_VIR;
				}
				else if (cat != U_MODIFIER_LETTER) {
					B_sequence_status = SEQ_NOT_STARTED; // Reset.
				}
			}
		} break;
		default:
			break;
		}
		if (B_sequence_status == SEQ_NOT_STARTED) {
			if (cat == U_UPPERCASE_LETTER || cat == U_LOWERCASE_LETTER ||
				cat == U_TITLECASE_LETTER || cat == U_MODIFIER_LETTER || cat == U_OTHER_LETTER) {
				B_sequence_status = SEQ_STARTED;
				B_scr = scr;
			}
		}

		if (u_hasBinaryProperty(str[i], UCHAR_PATTERN_SYNTAX) ||
			u_hasBinaryProperty(str[i], UCHAR_PATTERN_WHITE_SPACE) ||
			u_hasBinaryProperty(str[i], UCHAR_NONCHARACTER_CODE_POINT)) {
			return false; // Not a XID_Start or XID_Continue character.
		}
		if (i == 0) {
			if (!(cat == U_LOWERCASE_LETTER || cat == U_UPPERCASE_LETTER ||
					cat == U_TITLECASE_LETTER || cat == U_OTHER_LETTER ||
					cat == U_MODIFIER_LETTER || cat == U_LETTER_NUMBER || str[0] == 0x2118 ||
					str[0] == 0x212E || str[0] == 0x309B || str[0] == 0x309C || str[0] == 0x005F)) {
				return false; // Not a XID_Start character.
			}
		}
		else {
			if (!(cat == U_LOWERCASE_LETTER || cat == U_UPPERCASE_LETTER ||
					cat == U_TITLECASE_LETTER || cat == U_OTHER_LETTER ||
					cat == U_MODIFIER_LETTER || cat == U_LETTER_NUMBER ||
					cat == U_NON_SPACING_MARK || cat == U_COMBINING_SPACING_MARK ||
					cat == U_DECIMAL_DIGIT_NUMBER || cat == U_CONNECTOR_PUNCTUATION ||
					str[i] == 0x2118 || str[i] == 0x212E || str[i] == 0x309B || str[i] == 0x309C ||
					str[i] == 0x1369 || str[i] == 0x1371 || str[i] == 0x00B7 || str[i] == 0x0387 ||
					str[i] == 0x19DA || str[i] == 0x0E33 || str[i] == 0x0EB3 || str[i] == 0xFF9E ||
					str[i] == 0xFF9F)) {
				return false; // Not a XID_Continue character.
			}
		}
	}
	return true;
}

bool TextServerAdvanced::_is_valid_letter(uint64_t p_unicode) const
{
#ifndef ICU_STATIC_DATA
	if (!icu_data_loaded) {
		return TextServer::is_valid_letter(p_unicode);
	}
#endif

	return u_isalpha(p_unicode);
}

void TextServerAdvanced::_font_clear_system_fallback_cache()
{
	_THREAD_SAFE_METHOD_
	for (const KeyValue<SystemFontKey, SystemFontCache>& E : system_fonts) {
		const Vector<SystemFontCacheRec>& sysf_cache = E.value.var;
		for (const SystemFontCacheRec& F : sysf_cache) {
			_free_rid(F.rid);
		}
	}
	system_fonts.clear();
	system_font_data.clear();
}

void TextServerAdvanced::_cleanup() { font_clear_system_fallback_cache(); }

TextServerAdvanced::~TextServerAdvanced()
{
	_bmp_free_font_funcs();
#ifdef MODULE_FREETYPE_ENABLED
	if (ft_library != nullptr) {
		FT_Done_FreeType(ft_library);
	}
#endif
	if (sc_spoof != nullptr) {
		uspoof_close(sc_spoof);
		sc_spoof = nullptr;
	}
	if (sc_conf != nullptr) {
		uspoof_close(sc_conf);
		sc_conf = nullptr;
	}
	if (allowed != nullptr) {
		uset_close(allowed);
		allowed = nullptr;
	}
	for (const KeyValue<String, UBreakIterator*>& bi : line_break_iterators_per_language) {
		ubrk_close(bi.value);
	}

	std::atexit(u_cleanup);
}

int TextServerAdvanced::ft_move_to(const FT_Vector* to, void* user) { return 0; }

int TextServerAdvanced::ft_line_to(const FT_Vector* to, void* user) { return 0; }

int TextServerAdvanced::ft_conic_to(const FT_Vector* control, const FT_Vector* to, void* user)
{
	return 0;
}

int TextServerAdvanced::ft_cubic_to(
	const FT_Vector* control1, const FT_Vector* control2, const FT_Vector* to, void* user)
{
	return 0;
}

bool TextServerAdvanced::_ensure_cache_for_size(FontAdvanced* p_font_data, const Vector2i& p_size,
	FontForSizeAdvanced*& r_cache_for_size, bool p_silent, uint32_t p_oversampling) const
{
	ERR_FAIL_NULL_V(p_font_data, false);
	ERR_FAIL_COND_V(p_size.x <= 0, false);

	HashMap<Vector2i, FontForSizeAdvanced*>::Iterator E = p_font_data->cache.find(p_size);
	if (E) {
#ifdef MODULE_FREETYPE_ENABLED
		if (E->value->fsize != nullptr) {
			FT_Activate_Size(E->value->fsize);
		}
#endif
		r_cache_for_size = E->value;
		if (p_oversampling == 0 && E->value->viewport_oversampling != 0) {
			OversamplingLevel* ol = oversampling_levels.getptr(E->value->viewport_oversampling);
			if (ol) {
				ol->fonts.erase(E->value);
			}
		}
		return true;
	}

	FontForSizeAdvanced* fd = memnew(FontForSizeAdvanced);
	fd->size = p_size;
	if (p_font_data->data_ptr && (p_font_data->data_size > 0)) {
#ifdef MODULE_FREETYPE_ENABLED
		int error = 0;
		{
			MutexLock ftlock(ft_mutex);
			if (!ft_library) {
				error = FT_Init_FreeType(&ft_library);
				if (error != 0) {
					memdelete(fd);
					if (p_silent) {
						return false;
					}
					ERR_FAIL_V_MSG(false,
						"FreeType: Error initializing library: '" + String(FT_Error_String(error)) +
							"'.");
				}
#ifdef MODULE_SVG_ENABLED
				FT_Property_Set(ft_library, "ot-svg", "svg-hooks", get_tvg_svg_in_ot_hooks());
#endif
			}

			if (p_font_data->face == nullptr) {
				memset(&p_font_data->stream, 0, sizeof(FT_StreamRec));
				p_font_data->stream.base = (unsigned char*)p_font_data->data_ptr;
				p_font_data->stream.size = p_font_data->data_size;
				p_font_data->stream.pos = 0;

				FT_Open_Args fargs;
				memset(&fargs, 0, sizeof(FT_Open_Args));
				fargs.memory_base = (unsigned char*)p_font_data->data_ptr;
				fargs.memory_size = p_font_data->data_size;
				fargs.flags = FT_OPEN_MEMORY;
				fargs.stream = &p_font_data->stream;

				error = FT_Open_Face(
					ft_library, &fargs, p_font_data->face_index, &p_font_data->face);
				if (error) {
					if (p_font_data->face) {
						FT_Done_Face(p_font_data->face);
						p_font_data->face = nullptr;
					}
					memdelete(fd);
					if (p_silent) {
						return false;
					}
					ERR_FAIL_V_MSG(false,
						"FreeType: Error loading font: '" + String(FT_Error_String(error)) +
							"' (face_index=" + String::num_int64(p_font_data->face_index) + ").");
				}
			}

			FT_New_Size(p_font_data->face, &fd->fsize);
			FT_Activate_Size(fd->fsize);
		}

		double sz = double(fd->size.x) / 64.0;
		if (p_font_data->msdf) {
			sz = p_font_data->msdf_source_size;
		}

		if (FT_HAS_COLOR(p_font_data->face) && p_font_data->face->num_fixed_sizes > 0) {
			int best_match = 0;
			int diff = Math::abs(sz - ((int64_t)p_font_data->face->available_sizes[0].width));
			fd->scale = sz / p_font_data->face->available_sizes[0].width;
			for (int i = 1; i < p_font_data->face->num_fixed_sizes; i++) {
				int ndiff = Math::abs(sz - ((int64_t)p_font_data->face->available_sizes[i].width));
				if (ndiff < diff) {
					best_match = i;
					diff = ndiff;
					fd->scale = sz / p_font_data->face->available_sizes[i].width;
				}
			}
			FT_Select_Size(p_font_data->face, best_match);
		}
		else {
			FT_Size_RequestRec req;
			req.type = FT_SIZE_REQUEST_TYPE_NOMINAL;
			req.width = MIN(2048.0, sz) * 64.0;
			req.height = MIN(2048.0, sz) * 64.0;
			req.horiResolution = 0;
			req.vertResolution = 0;

			FT_Request_Size(p_font_data->face, &req);
			if (p_font_data->face->size->metrics.y_ppem != 0) {
				fd->scale = sz / (double)p_font_data->face->size->metrics.y_ppem;
			}
		}

		fd->hb_handle = hb_ft_font_create(p_font_data->face, nullptr);

		fd->ascent = (p_font_data->face->size->metrics.ascender / 64.0) * fd->scale;
		fd->descent = (-p_font_data->face->size->metrics.descender / 64.0) * fd->scale;
		fd->underline_position =
			(-FT_MulFix(p_font_data->face->underline_position,
				 p_font_data->face->size->metrics.y_scale) /
				64.0) *
			fd->scale;
		fd->underline_thickness =
			(FT_MulFix(p_font_data->face->underline_thickness,
				 p_font_data->face->size->metrics.y_scale) /
				64.0) *
			fd->scale;

#if HB_VERSION_ATLEAST(3, 3, 0)
		hb_font_set_synthetic_slant(fd->hb_handle, p_font_data->transform[0][1]);
#endif

		if (!p_font_data->face_init) {
			if (p_font_data->font_name.is_empty() && p_font_data->face->family_name != nullptr) {
				p_font_data->font_name = String::utf8((const char*)p_font_data->face->family_name);
			}
			if (p_font_data->face->style_name != nullptr) {
				p_font_data->style_name = String::utf8((const char*)p_font_data->face->style_name);
			}
			p_font_data->weight = _font_get_weight_by_name(p_font_data->style_name.to_lower());
			p_font_data->stretch = _font_get_stretch_by_name(p_font_data->style_name.to_lower());
			p_font_data->style_flags = 0;
			if ((p_font_data->face->style_flags & FT_STYLE_FLAG_BOLD) ||
				p_font_data->weight >= 700) {
				p_font_data->style_flags |= FONT_BOLD;
			}
			if ((p_font_data->face->style_flags & FT_STYLE_FLAG_ITALIC) ||
				_is_ital_style(p_font_data->style_name.to_lower())) {
				p_font_data->style_flags |= FONT_ITALIC;
			}
			if (p_font_data->face->face_flags & FT_FACE_FLAG_FIXED_WIDTH) {
				p_font_data->style_flags |= FONT_FIXED_WIDTH;
			}
			p_font_data->face_init = true;
		}
#else
		memdelete(fd);
		if (p_silent) {
			return false;
		}
		ERR_FAIL_V_MSG(false,
			"FreeType: Can't load dynamic font, engine is compiled without FreeType support!");
#endif
	}
	else {
		fd->hb_handle = _bmp_font_create(fd, nullptr);
	}

	fd->owner = p_font_data;
	p_font_data->cache.insert(p_size, fd);
	r_cache_for_size = fd;
	if (p_oversampling != 0) {
		OversamplingLevel* ol = oversampling_levels.getptr(p_oversampling);
		if (ol) {
			fd->viewport_oversampling = p_oversampling;
			ol->fonts.insert(fd);
		}
	}
	return true;
}

void TextServerAdvanced::full_copy(ShapedTextDataAdvanced* p_shaped) {}

bool TextServerAdvanced::_shaped_text_update_breaks(const RID& p_shaped) { return true; }

bool TextServerAdvanced::_shape_substr(ShapedTextDataAdvanced* p_new_sd,
	const ShapedTextDataAdvanced* p_sd, int64_t p_start, int64_t p_length) const
{
	return true;
}

// This implementation is derived from ICU: icu4c/source/extra/scrptrun/scrptrun.cpp

inline constexpr UChar32 ZERO_WIDTH_JOINER = 0x200d;
inline constexpr UChar32 VARIATION_SELECTOR_15 = 0xfe0e;
inline constexpr UChar32 VARIATION_SELECTOR_16 = 0xfe0f;
inline constexpr UChar32 COMBINING_ENCLOSING_KEYCAP = 0x20e3;

inline bool ScriptIterator::same_script(int32_t p_script_one, int32_t p_script_two) {
	return p_script_one <= USCRIPT_INHERITED || p_script_two <= USCRIPT_INHERITED || p_script_one == p_script_two;
}

inline bool ScriptIterator::is_emoji(UChar32 p_c, UChar32 p_next) {
	if (p_next == VARIATION_SELECTOR_15 && (u_hasBinaryProperty(p_c, UCHAR_EMOJI) || u_hasBinaryProperty(p_c, UCHAR_EXTENDED_PICTOGRAPHIC))) {
		return false;
	} else if (p_next == VARIATION_SELECTOR_16 && (u_hasBinaryProperty(p_c, UCHAR_EMOJI) || u_hasBinaryProperty(p_c, UCHAR_EXTENDED_PICTOGRAPHIC))) {
		return true;
	} else {
		return u_hasBinaryProperty(p_c, UCHAR_EMOJI_PRESENTATION) || u_hasBinaryProperty(p_c, UCHAR_EMOJI_MODIFIER) || u_hasBinaryProperty(p_c, UCHAR_REGIONAL_INDICATOR) || (u_hasBinaryProperty(p_c, UCHAR_EMOJI) && u_hasBinaryProperty(p_next, UCHAR_EMOJI_MODIFIER));
	}
}

ScriptIterator::ScriptIterator(const String &p_string, int p_start, int p_length) {
	struct ParenStackEntry {
		int pair_index;
		UScriptCode script_code;
	};

	struct EmojiSubrunEntry {
		int start;
		int end;
	};

	if (p_start >= p_length) {
		p_start = p_length - 1;
	}

	if (p_start < 0) {
		p_start = 0;
	}

	int paren_size = PAREN_STACK_DEPTH;
	ParenStackEntry starter_paren_stack[PAREN_STACK_DEPTH];
	ParenStackEntry *paren_stack = starter_paren_stack;

	int emoji_size = EMOJI_STACK_DEPTH;
	EmojiSubrunEntry starter_emoji_stack[EMOJI_STACK_DEPTH];
	EmojiSubrunEntry *emoji_stack = starter_emoji_stack;

	int script_start;
	int script_end = p_start;
	UScriptCode script_code;
	int paren_sp = -1;
	int start_sp = paren_sp;
	UErrorCode err = U_ZERO_ERROR;
	const char32_t *str = p_string.ptr();

	do {
		script_code = USCRIPT_COMMON;
		int emoji_sp = -1;
		bool emoji_run = false;
		for (script_start = script_end; script_end < p_length; script_end++) {
			UChar32 ch = str[script_end];
			UChar32 n = (script_end + 1 < p_length) ? str[script_end + 1] : 0;
			if (is_emoji(ch, n)) {
				if (!emoji_run) {
					emoji_run = true;
					emoji_sp++;
					if (unlikely(emoji_sp >= emoji_size)) {
						emoji_size += EMOJI_STACK_DEPTH;
						if (emoji_stack == starter_emoji_stack) {
							emoji_stack = static_cast<EmojiSubrunEntry *>(memalloc(emoji_size * sizeof(EmojiSubrunEntry)));
						} else {
							emoji_stack = static_cast<EmojiSubrunEntry *>(memrealloc(emoji_stack, emoji_size * sizeof(EmojiSubrunEntry)));
						}
					}
					emoji_stack[emoji_sp].start = script_end;
					emoji_stack[emoji_sp].end = script_end;
				}
			} else if (emoji_run && ch != ZERO_WIDTH_JOINER && ch != VARIATION_SELECTOR_16 && ch != COMBINING_ENCLOSING_KEYCAP && !(u_hasBinaryProperty(ch, UCHAR_EXTENDED_PICTOGRAPHIC) && n != VARIATION_SELECTOR_15)) {
				emoji_run = false;
				emoji_stack[emoji_sp].end = script_end;
			}

			UScriptCode sc = uscript_getScript(ch, &err);
			if (U_FAILURE(err)) {
				if (paren_stack != starter_paren_stack) {
					memfree(paren_stack);
				}
				ERR_FAIL_MSG(u_errorName(err));
			}

			if (u_getIntPropertyValue(ch, UCHAR_BIDI_PAIRED_BRACKET_TYPE) != U_BPT_NONE) {
				if (u_getIntPropertyValue(ch, UCHAR_BIDI_PAIRED_BRACKET_TYPE) == U_BPT_OPEN) {
					// If it's an open character, push it onto the stack.
					paren_sp++;
					if (unlikely(paren_sp >= paren_size)) {
						// If the stack is full, allocate more space to handle deeply nested parentheses. This is unlikely to happen with any real text.
						paren_size += PAREN_STACK_DEPTH;
						if (paren_stack == starter_paren_stack) {
							paren_stack = static_cast<ParenStackEntry *>(memalloc(paren_size * sizeof(ParenStackEntry)));
						} else {
							paren_stack = static_cast<ParenStackEntry *>(memrealloc(paren_stack, paren_size * sizeof(ParenStackEntry)));
						}
					}
					paren_stack[paren_sp].pair_index = ch;
					paren_stack[paren_sp].script_code = script_code;
				} else if (paren_sp >= 0) {
					// If it's a close character, find the matching open on the stack, and use that script code. Any non-matching open characters above it on the stack will be popped.
					UChar32 paired_ch = u_getBidiPairedBracket(ch);
					while (paren_sp >= 0 && paren_stack[paren_sp].pair_index != paired_ch) {
						paren_sp -= 1;
					}
					if (paren_sp < start_sp) {
						start_sp = paren_sp;
					}
					if (paren_sp >= 0) {
						sc = paren_stack[paren_sp].script_code;
					}
				}
			}

			if (same_script(script_code, sc)) {
				if (script_code <= USCRIPT_INHERITED && sc > USCRIPT_INHERITED) {
					script_code = sc;
					// Now that we have a final script code, fix any open characters we pushed before we knew the script code.
					while (start_sp < paren_sp) {
						paren_stack[++start_sp].script_code = script_code;
					}
				}
				if ((u_getIntPropertyValue(ch, UCHAR_BIDI_PAIRED_BRACKET_TYPE) == U_BPT_CLOSE) && paren_sp >= 0) {
					// If this character is a close paired character pop the matching open character from the stack.
					paren_sp -= 1;
					if (start_sp >= 0) {
						start_sp -= 1;
					}
				}
			} else {
				break;
			}
		}
		if (emoji_run) {
			emoji_stack[emoji_sp].end = script_end;
		}

		for (int sub = 0; sub <= emoji_sp; sub++) {
			if (emoji_stack[sub].start > script_start) {
				ScriptRange rng;
				rng.script = hb_icu_script_to_script(script_code);
				rng.start = script_start;
				rng.end = emoji_stack[sub].start;
				script_ranges.push_back(rng);
			}
			ScriptRange rng;
			rng.script = (hb_script_t)HB_TAG('Z', 's', 'y', 'e');
			rng.start = emoji_stack[sub].start;
			rng.end = emoji_stack[sub].end;
			script_ranges.push_back(rng);

			script_start = emoji_stack[sub].end;
		}
		if (script_start != script_end) {
			ScriptRange rng;
			rng.script = hb_icu_script_to_script(script_code);
			rng.start = script_start;
			rng.end = script_end;
			script_ranges.push_back(rng);
		}

		if (emoji_stack != starter_emoji_stack) {
			memfree(emoji_stack);
		}
	} while (script_end < p_length);

	if (paren_stack != starter_paren_stack) {
		memfree(paren_stack);
	}
}

#ifdef MODULE_SVG_ENABLED
#ifdef MODULE_FREETYPE_ENABLED

#include <freetype/otsvg.h>
#include <ft2build.h>

FT_Error tvg_svg_in_ot_init(FT_Pointer *p_state) {
	*p_state = memnew(TVG_State);

	return FT_Err_Ok;
}

void tvg_svg_in_ot_free(FT_Pointer *p_state) {
	TVG_State *state = *reinterpret_cast<TVG_State **>(p_state);
	memdelete(state);
}

static void construct_xml(Ref<XMLParser> &parser, double &r_embox_x, double &r_embox_y, String *p_xml, int64_t &r_tag_count) {
	if (parser->get_node_type() == XMLParser::NODE_ELEMENT) {
		*p_xml += vformat("<%s", parser->get_node_name());
		bool is_svg_tag = parser->get_node_name() == "svg";
		for (int i = 0; i < parser->get_attribute_count(); i++) {
			String aname = parser->get_attribute_name(i);
			String value = parser->get_attribute_value(i);
			if (is_svg_tag && aname == "viewBox") {
				PackedStringArray vb = value.split(" ");
				if (vb.size() == 4) {
					r_embox_x = vb[2].to_float();
					r_embox_y = vb[3].to_float();
				}
			} else if (is_svg_tag && aname == "width") {
				r_embox_x = value.to_float();
			} else if (is_svg_tag && aname == "height") {
				r_embox_y = value.to_float();
			} else {
				*p_xml += vformat(" %s=\"%s\"", aname, value);
			}
		}

		if (parser->is_empty()) {
			*p_xml += "/>";
		} else {
			*p_xml += ">";
			if (r_tag_count >= 0) {
				r_tag_count++;
			}
		}
	} else if (parser->get_node_type() == XMLParser::NODE_TEXT) {
		*p_xml += parser->get_node_data();
	} else if (parser->get_node_type() == XMLParser::NODE_ELEMENT_END) {
		*p_xml += vformat("</%s>", parser->get_node_name());
		if (r_tag_count > 0) {
			r_tag_count--;
		}
	}
}

FT_Error tvg_svg_in_ot_preset_slot(FT_GlyphSlot p_slot, FT_Bool p_cache, FT_Pointer *p_state) {
	TVG_State *state = *reinterpret_cast<TVG_State **>(p_state);
	if (!state) {
		ERR_FAIL_V_MSG(FT_Err_Invalid_SVG_Document, "SVG in OT state not initialized.");
	}
	MutexLock lock(state->mutex);

	FT_SVG_Document document = (FT_SVG_Document)p_slot->other;
	FT_Size_Metrics metrics = document->metrics;

	GL_State &gl_state = state->glyph_map[p_slot->glyph_index];
	if (!gl_state.ready) {
		Ref<XMLParser> parser;
		parser.instantiate();
		parser->_open_buffer((const uint8_t *)document->svg_document, document->svg_document_length);

		String xml_body;

		double embox_x = document->units_per_EM;
		double embox_y = document->units_per_EM;

		TVG_DocumentCache &cache = state->document_map[document->svg_document];

		if (!cache.xml_body.is_empty()) {
			// If we have a cached document, that means we have already parsed it.
			// All node cache should be available.

			xml_body = cache.xml_body;
			embox_x = cache.embox_x;
			embox_y = cache.embox_y;

			ERR_FAIL_COND_V(!cache.node_caches.has(p_slot->glyph_index), FT_Err_Invalid_SVG_Document);
			Vector<TVG_NodeCache> &ncs = cache.node_caches[p_slot->glyph_index];

			uint64_t offset = 0;
			for (TVG_NodeCache &nc : ncs) {
				// Seek will call read() internally.
				if (parser->seek(nc.document_offset) == OK) {
					int64_t tag_count = 0;
					String xml_node;

					// We only parse the glyph node.
					do {
						construct_xml(parser, embox_x, embox_y, &xml_node, tag_count);
					} while (tag_count != 0 && parser->read() == OK);

					xml_body = xml_body.insert(nc.body_offset + offset, xml_node);
					offset += xml_node.length();
				}
			}
		} else {
			String xml_node;
			String xml_body_temp;

			String *p_xml = &xml_body_temp;
			int64_t tag_count = -1;

			bool is_in_defs = false;
			while (parser->read() == OK) {
				if (parser->get_node_type() == XMLParser::NODE_ELEMENT && parser->get_node_name().to_lower() == "defs") {
					is_in_defs = true;
				} else if (parser->get_node_type() == XMLParser::NODE_ELEMENT_END && parser->get_node_name().to_lower() == "defs") {
					is_in_defs = false;
				}
				if (!is_in_defs && parser->has_attribute("id")) {
					const String &gl_name = parser->get_named_attribute_value("id");
					if (gl_name.begins_with("glyph")) {
						int dot_pos = gl_name.find_char('.');
						int64_t gl_idx = gl_name.substr(5, (dot_pos > 0) ? dot_pos - 5 : -1).to_int();

						TVG_NodeCache node_cache = TVG_NodeCache();
						node_cache.document_offset = parser->get_node_offset(),
						node_cache.body_offset = (uint64_t)cache.xml_body.length();
						cache.node_caches[gl_idx].push_back(node_cache);

						if (p_slot->glyph_index != gl_idx) {
							parser->skip_section();
							continue;
						}
						tag_count = 0;
						xml_node = "";
						p_xml = &xml_node;
					}
				}

				xml_body_temp = "";
				construct_xml(parser, embox_x, embox_y, p_xml, tag_count);

				if (xml_body_temp.length() > 0) {
					xml_body += xml_body_temp;
					cache.xml_body += xml_body_temp;
					continue;
				}

				if (tag_count == 0) {
					p_xml = &xml_body_temp;
					tag_count = -1;
					xml_body += xml_node;
				}
			}

			cache.embox_x = embox_x;
			cache.embox_y = embox_y;
		}

		tvg::Picture *picture = tvg::Picture::gen();
		gl_state.xml_code = xml_body.utf8();

		tvg::Result result = picture->load(gl_state.xml_code.get_data(), gl_state.xml_code.length(), "svg+xml", nullptr, false);
		if (result != tvg::Result::Success) {
			tvg::Paint::rel(picture);
			ERR_FAIL_V_MSG(FT_Err_Invalid_SVG_Document, "Failed to load SVG document (glyph metrics).");
		}

		float svg_width, svg_height;
		picture->size(&svg_width, &svg_height);
		double aspect_x = (svg_width > svg_height) ? svg_width / svg_height : 1.0;
		double aspect_y = (svg_width < svg_height) ? svg_height / svg_width : 1.0;

		result = picture->size(embox_x * aspect_x, embox_y * aspect_y);
		if (result != tvg::Result::Success) {
			tvg::Paint::rel(picture);
			ERR_FAIL_V_MSG(FT_Err_Invalid_SVG_Document, "Failed to resize SVG document.");
		}

		double scale = double(embox_x * aspect_x) / double(svg_width);
		double yoff = double(document->metrics.ascender + document->metrics.descender) / double(document->metrics.ascender);

		double x_svg_to_out = (double)metrics.x_ppem / embox_x / scale;
		double y_svg_to_out = (double)metrics.y_ppem / embox_y / scale;

		gl_state.m.e11 = (double)document->transform.xx / (1 << 16);
		gl_state.m.e12 = -(double)document->transform.xy / (1 << 16);
		gl_state.m.e21 = -(double)document->transform.yx / (1 << 16);
		gl_state.m.e22 = (double)document->transform.yy / (1 << 16);
		gl_state.m.e13 = (double)document->delta.x / 64 * embox_x / metrics.x_ppem * scale;
		gl_state.m.e23 = -(double)document->delta.y / 64 * embox_y / metrics.y_ppem * scale;
		gl_state.m.e31 = 0;
		gl_state.m.e32 = 0;
		gl_state.m.e33 = 1;

		result = picture->size(embox_x * aspect_x * x_svg_to_out, embox_y * aspect_y * y_svg_to_out);
		if (result != tvg::Result::Success) {
			tvg::Paint::rel(picture);
			ERR_FAIL_V_MSG(FT_Err_Invalid_SVG_Document, "Failed to resize SVG document.");
		}

		result = picture->transform(gl_state.m);
		if (result != tvg::Result::Success) {
			tvg::Paint::rel(picture);
			ERR_FAIL_V_MSG(FT_Err_Invalid_SVG_Document, "Failed to apply transform to SVG document.");
		}

		picture->size(&gl_state.w, &gl_state.h);

		gl_state.x = (double(p_slot->metrics.horiAdvance) / 64.0 - gl_state.w) / 2.0;
		gl_state.y = -Math::ceil(gl_state.h * yoff);

		tvg::Paint::rel(picture);

		gl_state.ready = true;
	}

	p_slot->bitmap_left = (FT_Int)gl_state.x;
	p_slot->bitmap_top = (FT_Int)-gl_state.y;

	double tmpd = Math::ceil(gl_state.h);
	p_slot->bitmap.rows = (unsigned int)tmpd;
	tmpd = Math::ceil(gl_state.w);
	p_slot->bitmap.width = (unsigned int)tmpd;
	p_slot->bitmap.pitch = (int)p_slot->bitmap.width * 4;

	p_slot->bitmap.pixel_mode = FT_PIXEL_MODE_BGRA;

	float metrics_width = (float)gl_state.w;
	float metrics_height = (float)gl_state.h;

	float horiBearingX = (float)gl_state.x;
	float horiBearingY = (float)-gl_state.y;

	float vertBearingX = p_slot->metrics.horiBearingX / 64.0f - p_slot->metrics.horiAdvance / 64.0f / 2;
	float vertBearingY = (p_slot->metrics.vertAdvance / 64.0f - p_slot->metrics.height / 64.0f) / 2;

	float tmpf = Math::round(metrics_width * 64);
	p_slot->metrics.width = (FT_Pos)tmpf;
	tmpf = Math::round(metrics_height * 64);
	p_slot->metrics.height = (FT_Pos)tmpf;

	p_slot->metrics.horiBearingX = (FT_Pos)(horiBearingX * 64);
	p_slot->metrics.horiBearingY = (FT_Pos)(horiBearingY * 64);
	p_slot->metrics.vertBearingX = (FT_Pos)(vertBearingX * 64);
	p_slot->metrics.vertBearingY = (FT_Pos)(vertBearingY * 64);

	if (p_slot->metrics.vertAdvance == 0) {
		p_slot->metrics.vertAdvance = (FT_Pos)(metrics_height * 1.2f * 64);
	}

	return FT_Err_Ok;
}

FT_Error tvg_svg_in_ot_render(FT_GlyphSlot p_slot, FT_Pointer *p_state) {
	TVG_State *state = *reinterpret_cast<TVG_State **>(p_state);
	if (!state) {
		ERR_FAIL_V_MSG(FT_Err_Invalid_SVG_Document, "SVG in OT state not initialized.");
	}
	MutexLock lock(state->mutex);

	if (!state->glyph_map.has(p_slot->glyph_index)) {
		ERR_FAIL_V_MSG(FT_Err_Invalid_SVG_Document, "SVG glyph not loaded.");
	}

	GL_State &gl_state = state->glyph_map[p_slot->glyph_index];
	ERR_FAIL_COND_V_MSG(!gl_state.ready, FT_Err_Invalid_SVG_Document, "SVG glyph not ready.");

	tvg::Picture *picture = tvg::Picture::gen();
	tvg::Result res = picture->load(gl_state.xml_code.get_data(), gl_state.xml_code.length(), "svg+xml", nullptr, false);
	if (res != tvg::Result::Success) {
		tvg::Paint::rel(picture);
		ERR_FAIL_V_MSG(FT_Err_Invalid_SVG_Document, "Failed to load SVG document (glyph rendering).");
	}
	res = picture->size(gl_state.w, gl_state.h);
	if (res != tvg::Result::Success) {
		tvg::Paint::rel(picture);
		ERR_FAIL_V_MSG(FT_Err_Invalid_SVG_Document, "Failed to resize SVG document.");
	}
	res = picture->transform(gl_state.m);
	if (res != tvg::Result::Success) {
		tvg::Paint::rel(picture);
		ERR_FAIL_V_MSG(FT_Err_Invalid_SVG_Document, "Failed to apply transform to SVG document.");
	}

	std::unique_ptr<tvg::SwCanvas> sw_canvas(tvg::SwCanvas::gen());
	res = sw_canvas->target((uint32_t *)p_slot->bitmap.buffer, (int)p_slot->bitmap.width, (int)p_slot->bitmap.width, (int)p_slot->bitmap.rows, tvg::ColorSpace::ARGB8888S);
	if (res != tvg::Result::Success) {
		tvg::Paint::rel(picture);
		ERR_FAIL_V_MSG(FT_Err_Invalid_Outline, "Failed to create SVG canvas.");
	}
	res = sw_canvas->add(picture);
	if (res != tvg::Result::Success) {
		ERR_FAIL_V_MSG(FT_Err_Invalid_Outline, "Failed to set SVG canvas source.");
	}
	res = sw_canvas->draw();
	if (res != tvg::Result::Success) {
		ERR_FAIL_V_MSG(FT_Err_Invalid_Outline, "Failed to draw to SVG canvas.");
	}
	res = sw_canvas->sync();
	if (res != tvg::Result::Success) {
		ERR_FAIL_V_MSG(FT_Err_Invalid_Outline, "Failed to sync SVG canvas.");
	}

	state->glyph_map.erase(p_slot->glyph_index);

	p_slot->bitmap.pixel_mode = FT_PIXEL_MODE_BGRA;
	p_slot->bitmap.num_grays = 256;
	p_slot->format = FT_GLYPH_FORMAT_BITMAP;

	return FT_Err_Ok;
}

SVG_RendererHooks tvg_svg_in_ot_hooks = {
	(SVG_Lib_Init_Func)tvg_svg_in_ot_init,
	(SVG_Lib_Free_Func)tvg_svg_in_ot_free,
	(SVG_Lib_Render_Func)tvg_svg_in_ot_render,
	(SVG_Lib_Preset_Slot_Func)tvg_svg_in_ot_preset_slot,
};

SVG_RendererHooks *get_tvg_svg_in_ot_hooks() {
	return &tvg_svg_in_ot_hooks;
}

#endif // MODULE_FREETYPE_ENABLED
#endif // MODULE_SVG_ENABLED

void initialize_text_server_adv_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SERVERS) {
		return;
	}
	if (TextServerManager::is_initialized()) {
		Ref<TextServerAdvanced> ts;
		ts.instantiate();
		TextServerManager::add_interface(ts);
	}
}

void uninitialize_text_server_adv_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SERVERS) {
		return;
	}
}
