/**************************************************************************/
/*  image_loader_png.cpp                                                  */
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

#include "image_loader_png.h"

#include "drivers/png/png_driver_common.h"

Error ImageLoaderPNG::load_image(Ref<Image> p_image, Ref<FileAccess> f, uint32_t p_flags, float p_scale) {
	const uint64_t buffer_size = f->get_length();
	Vector<uint8_t> file_buffer;
	Error err = file_buffer.resize(buffer_size);
	if (err) {
		return err;
	}
	{
		uint8_t *writer = file_buffer.ptrw();
		f->get_buffer(writer, buffer_size);
	}
	const uint8_t *reader = file_buffer.ptr();
	return PNGDriverCommon::png_to_image(reader, buffer_size, p_flags & FLAG_FORCE_LINEAR, p_image);
}

void ImageLoaderPNG::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("png");
}

Ref<Image> ImageLoaderPNG::load_mem_png(const uint8_t *p_png, int p_size) {
	Ref<Image> img;
	img.instantiate();

	// the value of p_force_linear does not matter since it only applies to 16 bit
	Error err = PNGDriverCommon::png_to_image(p_png, p_size, false, img);
	ERR_FAIL_COND_V(err, Ref<Image>());

	return img;
}

Ref<Image> ImageLoaderPNG::unpack_mem_png(const uint8_t *p_png, int p_size) {
	ERR_FAIL_COND_V(p_size < 4, Ref<Image>());
	ERR_FAIL_COND_V(p_png[0] != 'P' || p_png[1] != 'N' || p_png[2] != 'G' || p_png[3] != ' ', Ref<Image>());
	return load_mem_png(&p_png[4], p_size - 4);
}

Ref<Image> ImageLoaderPNG::lossless_unpack_png(const Vector<uint8_t> &p_data) {
	return unpack_mem_png(p_data.ptr(), p_data.size());
}

Vector<uint8_t> ImageLoaderPNG::lossless_pack_png(const Ref<Image> &p_image) {
	Vector<uint8_t> out_buffer;

	// add Godot's own "PNG " prefix
	if (out_buffer.resize(4) != OK) {
		ERR_FAIL_V(Vector<uint8_t>());
	}

	// scope for writer lifetime
	{
		// must be closed before call to image_to_png
		uint8_t *writer = out_buffer.ptrw();
		memcpy(writer, "PNG ", 4);
	}

	Error err = PNGDriverCommon::image_to_png(p_image, out_buffer);
	if (err) {
		ERR_FAIL_V(Vector<uint8_t>());
	}

	return out_buffer;
}

ImageLoaderPNG::ImageLoaderPNG() {
	Image::_png_mem_loader_func = load_mem_png;
	Image::_png_mem_unpacker_func = unpack_mem_png;
	Image::png_unpacker = lossless_unpack_png;
	Image::png_packer = lossless_pack_png;
}

// ============================================================================
// Merged from drivers/png/png_driver_common.cpp
// ============================================================================

#include "png_driver_common.h"

#ifdef TOOLS_ENABLED
#include "core/config/engine.h"
#endif

#include <png.h>

namespace PNGDriverCommon
{

// Print any warnings.
// On error, set explain and return true.
// Call should be wrapped in ERR_FAIL_COND
static bool check_error(const png_image& image)
{
	const png_uint_32 failed = PNG_IMAGE_FAILED(image);
	if (failed & PNG_IMAGE_ERROR) {
		return true;
	}
	else if (failed) {
#ifdef TOOLS_ENABLED
		// suppress this warning, to avoid log spam when opening assetlib
		const static char* const noisy = "iCCP: known incorrect sRGB profile";
		if (Engine::is_editor_hint() && !strcmp(image.message, noisy)) {
			return false;
		}
#endif
		WARN_PRINT(image.message);
	}
	return false;
}

Error png_to_image(const uint8_t* p_source, size_t p_size, bool p_force_linear, Ref<Image> p_image)
{
	png_image png_img;
	memset(&png_img, 0, sizeof(png_img));
	png_img.version = PNG_IMAGE_VERSION;

	// fetch image properties
	int success = png_image_begin_read_from_memory(&png_img, p_source, p_size);
	ERR_FAIL_COND_V_MSG(check_error(png_img), ERR_FILE_CORRUPT, png_img.message);
	ERR_FAIL_COND_V(!success, ERR_FILE_CORRUPT);

	// flags to be masked out of input format to give target format
	const png_uint_32 format_mask = ~(
		// convert component order to RGBA
		PNG_FORMAT_FLAG_BGR |
		PNG_FORMAT_FLAG_AFIRST
		// convert 16 bit components to 8 bit
		| PNG_FORMAT_FLAG_LINEAR
		// convert indexed image to direct color
		| PNG_FORMAT_FLAG_COLORMAP);

	png_img.format &= format_mask;

	Image::Format dest_format;
	switch (png_img.format) {
	case PNG_FORMAT_GRAY:
		dest_format = Image::FORMAT_L8;
		break;
	case PNG_FORMAT_GA:
		dest_format = Image::FORMAT_LA8;
		break;
	case PNG_FORMAT_RGB:
		dest_format = Image::FORMAT_RGB8;
		break;
	case PNG_FORMAT_RGBA:
		dest_format = Image::FORMAT_RGBA8;
		break;
	default:
		png_image_free(&png_img); // only required when we return before finish_read
		ERR_PRINT("Unsupported png format.");
		return ERR_UNAVAILABLE;
	}

	if (!p_force_linear) {
		// assume 16 bit pngs without sRGB or gAMA chunks are in sRGB format
		png_img.flags |= PNG_IMAGE_FLAG_16BIT_sRGB;
	}

	const png_uint_32 stride = PNG_IMAGE_ROW_STRIDE(png_img);
	Vector<uint8_t> buffer;
	Error err = buffer.resize(PNG_IMAGE_BUFFER_SIZE(png_img, stride));
	if (err) {
		png_image_free(&png_img); // only required when we return before finish_read
		return err;
	}
	uint8_t* writer = buffer.ptrw();

	// read image data to buffer and release libpng resources
	success = png_image_finish_read(&png_img, nullptr, writer, stride, nullptr);
	ERR_FAIL_COND_V_MSG(check_error(png_img), ERR_FILE_CORRUPT, png_img.message);
	ERR_FAIL_COND_V(!success, ERR_FILE_CORRUPT);

	// print_line("png width: "+itos(png_img.width)+" height: "+itos(png_img.height));
	p_image->set_data(png_img.width, png_img.height, false, dest_format, buffer);

	return OK;
}

} // namespace PNGDriverCommon



// ============================================================================
// Merged from drivers/png/resource_saver_png.cpp
// ============================================================================

#include "core/io/file_access.h"
#include "core/io/image.h"
#include "drivers/png/png_driver_common.h"
#include "drivers/png/image_loader_png.h"
#include "scene/resources/image_texture.h"

Error ResourceSaverPNG::save(
	const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags)
{
	Ref<ImageTexture> texture = p_resource;

	ERR_FAIL_COND_V_MSG(
		texture.is_null(), ERR_INVALID_PARAMETER, "Can't save invalid texture as PNG.");
	ERR_FAIL_COND_V_MSG(
		!texture->get_width(), ERR_INVALID_PARAMETER, "Can't save empty texture as PNG.");

	Ref<Image> img = texture->get_image();

	Error err = save_image(p_path, img);

	return err;
}

Error ResourceSaverPNG::save_image(const String& p_path, const Ref<Image>& p_img)
{
	Vector<uint8_t> buffer;
	Error err = PNGDriverCommon::image_to_png(p_img, buffer);
	ERR_FAIL_COND_V_MSG(err, err, "Can't convert image to PNG.");
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::WRITE, &err);
	ERR_FAIL_COND_V_MSG(err, err, vformat("Can't save PNG at path: '%s'.", p_path));

	const uint8_t* reader = buffer.ptr();

	file->store_buffer(reader, buffer.size());
	if (file->get_error() != OK && file->get_error() != ERR_FILE_EOF) {
		return ERR_CANT_CREATE;
	}

	return OK;
}

Vector<uint8_t> ResourceSaverPNG::save_image_to_buffer(const Ref<Image>& p_img)
{
	Vector<uint8_t> buffer;
	Error err = PNGDriverCommon::image_to_png(p_img, buffer);
	ERR_FAIL_COND_V_MSG(err, Vector<uint8_t>(), "Can't convert image to PNG.");
	return buffer;
}

ResourceSaverPNG::ResourceSaverPNG()
{
	Image::save_png_func = &save_image;
	Image::save_png_buffer_func = &save_image_to_buffer;
}

Error PNGDriverCommon::image_to_png(Ref<Image> const&, Vector<unsigned char>&) { return OK; }

bool ResourceSaverPNG::recognize(const Ref<Resource>& p_resource) const { return false; }

void ResourceSaverPNG::get_recognized_extensions(
	const Ref<Resource>& p_resource, List<String>* p_extensions) const
{
}


