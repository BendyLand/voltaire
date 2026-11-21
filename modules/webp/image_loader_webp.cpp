/**************************************************************************/
/*  image_loader_webp.cpp                                                 */
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

#include <webp/decode.h>
#include "core/config/project_settings.h"
#include "core/io/file_access.h"
#include "core/io/resource_saver.h"
#include "register_types.h"
#include "scene/resources/image_texture.h"
#include <webp/encode.h>
#include "image_loader_webp.h"

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
