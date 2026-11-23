#include "modules/modules_enabled.gen.h"

#if defined(MODULE_SVG_ENABLED)
#include "core/io/resource_loader.h"
#include "modules/register_module_types.h"

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
#include "modules/register_module_types.h"

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
#include "modules/register_module_types.h"
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
#include "modules/register_module_types.h"

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
#include "modules/register_module_types.h"
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
#include "modules/register_module_types.h"
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
#include "modules/register_module_types.h"

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
#include "modules/register_module_types.h"
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
#include "core/io/resource_saver.h"
#include "modules/register_module_types.h"
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

#include "core/io/resource_saver.h"

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

// === Merged from modules/noise/fastnoise_lite.cpp ===
#include "modules/modules_enabled.gen.h"

#if defined(MODULE_NOISE_ENABLED)
#include <cfloat> // FLT_MAX
#include "modules/register_module_types.h"
#include "servers/rendering/renderer.h"
#include "core/config/engine.h"
#include "modules/noise/fastnoise_lite.h"

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
#include "modules/register_module_types.h"
#include "modules/regex/regex.h"

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
#include "modules/register_module_types.h"

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
