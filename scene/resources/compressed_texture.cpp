/**************************************************************************/
/*  compressed_texture.cpp                                                */
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

#include "compressed_texture.h"

#include "core/config/project_settings.h"
#include "core/io/file_access.h"
#include "core/io/marshalls.h"
#include "core/io/resource_loader.h"
#include "scene/resources/bit_map.h"
#include "servers/rendering/renderer.h"

static const char* compression_mode_names[7] = {
	"Lossless", "Lossy", "Basis Universal", "S3TC", "ETC2", "BPTC", "ASTC"};

static CompressedTexture2D::CompressionMode get_expected_compression_mode(Image::Format format)
{
	if ((format >= Image::FORMAT_DXT1 && format <= Image::FORMAT_RGTC_RG) ||
		format == Image::FORMAT_DXT5_RA_AS_RG) {
		return CompressedTexture2D::COMPRESSION_MODE_S3TC;
	}
	else if (format >= Image::FORMAT_ETC && format <= Image::FORMAT_ETC2_RA_AS_RG) {
		return CompressedTexture2D::COMPRESSION_MODE_ETC2;
	}
	else if (format >= Image::FORMAT_BPTC_RGBA && format <= Image::FORMAT_BPTC_RGBFU) {
		return CompressedTexture2D::COMPRESSION_MODE_BPTC;
	}
	else if ((format >= Image::FORMAT_ASTC_4x4 && format <= Image::FORMAT_ASTC_8x8_HDR) ||
			 (format >= Image::FORMAT_ASTC_6x6 && format <= Image::FORMAT_ASTC_6x6_HDR)) {
		return CompressedTexture2D::COMPRESSION_MODE_ASTC;
	}
	ERR_FAIL_V(CompressedTexture2D::COMPRESSION_MODE_LOSSLESS);
}

Error CompressedTexture2D::_load_data(const String& p_path, int& r_width, int& r_height,
	Ref<Image>& image, bool& r_request_3d, bool& r_request_normal, bool& r_request_roughness,
	int& mipmap_limit, int p_size_limit)
{
	alpha_cache.unref();

	ERR_FAIL_COND_V(image.is_null(), ERR_INVALID_PARAMETER);

	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(f.is_null(), ERR_CANT_OPEN, vformat("Unable to open file: %s.", p_path));

	uint8_t header[4];
	f->get_buffer(header, 4);
	if (header[0] != 'G' || header[1] != 'S' || header[2] != 'T' || header[3] != '2') {
		ERR_FAIL_V_MSG(ERR_FILE_CORRUPT, "Compressed texture file is corrupt (Bad header).");
	}

	uint32_t version = f->get_32();

	if (version > FORMAT_VERSION) {
		ERR_FAIL_V_MSG(ERR_FILE_CORRUPT, "Compressed texture file is too new.");
	}
	r_width = f->get_32();
	r_height = f->get_32();
	uint32_t df = f->get_32(); // data format

	// skip reserved
	mipmap_limit = int(f->get_32());
	// reserved
	f->get_32();
	f->get_32();
	f->get_32();

#ifdef TOOLS_ENABLED
	r_request_3d = request_3d_callback && df & FORMAT_BIT_DETECT_3D;
	r_request_roughness = request_roughness_callback && df & FORMAT_BIT_DETECT_ROUGNESS;
	r_request_normal = request_normal_callback && df & FORMAT_BIT_DETECT_NORMAL;
#else
	r_request_3d = false;
	r_request_roughness = false;
	r_request_normal = false;
#endif
	if (!(df & FORMAT_BIT_STREAM)) {
		p_size_limit = 0;
	}

	image = load_image_from_file(f, p_size_limit);

	if (image.is_null() || image->is_empty()) {
		return ERR_CANT_OPEN;
	}

	return OK;
}

void CompressedTexture2D::set_path(const String& p_path, bool p_take_over)
{
	if (texture.is_valid()) {
		Renderer::texture_set_path(texture, p_path);
	}

	Resource::set_path(p_path, p_take_over);
}

void CompressedTexture2D::_requested_3d(void* p_ud)
{
	CompressedTexture2D* ct = (CompressedTexture2D*)p_ud;
	Ref<CompressedTexture2D> ctex(ct);
	ERR_FAIL_NULL(request_3d_callback);
	request_3d_callback(ctex);
}

void CompressedTexture2D::_requested_roughness(
	void* p_ud, const String& p_normal_path, RSE::TextureDetectRoughnessChannel p_roughness_channel)
{
	CompressedTexture2D* ct = (CompressedTexture2D*)p_ud;
	Ref<CompressedTexture2D> ctex(ct);
	ERR_FAIL_NULL(request_roughness_callback);
	request_roughness_callback(ctex, p_normal_path, p_roughness_channel);
}

void CompressedTexture2D::_requested_normal(void* p_ud)
{
	CompressedTexture2D* ct = (CompressedTexture2D*)p_ud;
	Ref<CompressedTexture2D> ctex(ct);
	ERR_FAIL_NULL(request_normal_callback);
	request_normal_callback(ctex);
}

CompressedTexture2D::TextureFormatRequestCallback CompressedTexture2D::request_3d_callback =
	nullptr;
CompressedTexture2D::TextureFormatRoughnessRequestCallback
	CompressedTexture2D::request_roughness_callback = nullptr;
CompressedTexture2D::TextureFormatRequestCallback CompressedTexture2D::request_normal_callback =
	nullptr;

bool CompressedTexture2D::keep_all_compressed_buffers = false;

void CompressedTexture2D::_set_data(const Vector<uint8_t>& p_data)
{
	if (p_data.is_empty()) {
		return; // nothing to do
	}

	const uint8_t* data = p_data.ptr();
	uint32_t data_size = p_data.size();
	ERR_FAIL_COND(data_size < 20);
	is_portable = true;
	compression_mode = CompressionMode(decode_uint16(data));
	PortableDataFormat data_format = PortableDataFormat(decode_uint16(data + 2));
	format = Image::Format(decode_uint32(data + 4));
	uint32_t mipmap_count = decode_uint32(data + 8);
	w = int(decode_uint32(data + 12));
	h = int(decode_uint32(data + 16));
	mipmaps = mipmap_count > 1;

	data += 20;
	data_size -= 20;

	Ref<Image> image;

	switch (compression_mode) {
	case COMPRESSION_MODE_LOSSLESS:
	case COMPRESSION_MODE_LOSSY: {
		ImageMemLoadFunc loader_func;
		if (data_format == PORTABLE_DATA_FORMAT_UNDEFINED) {
			loader_func = nullptr;
		}
		else if (data_format == PORTABLE_DATA_FORMAT_PNG) {
			loader_func = Image::_png_mem_unpacker_func;
		}
		else if (data_format == PORTABLE_DATA_FORMAT_WEBP) {
			loader_func = Image::_webp_mem_loader_func;
		}
		else {
			ERR_FAIL();
		}
		Vector<uint8_t> image_data;

		ERR_FAIL_COND(data_size < 4);
		for (uint32_t i = 0; i < mipmap_count; i++) {
			uint32_t mipsize = decode_uint32(data);
			data += 4;
			data_size -= 4;
			ERR_FAIL_COND(mipsize > data_size);
			Ref<Image> img = loader_func == nullptr ? memnew(Image(data, data_size))
													: Ref<Image>(loader_func(data, data_size));
			ERR_FAIL_COND(img->is_empty());
			if (img->get_format() != format) { // May happen due to webp/png in the tiny mipmaps.
				img->convert(format);
			}
			image_data.append_array(img->get_data());

			data += mipsize;
			data_size -= mipsize;
		}

		image.instantiate(w, h, mipmaps, format, image_data);
	} break;
	case COMPRESSION_MODE_BASIS_UNIVERSAL: {
		ERR_FAIL_NULL(Image::basis_universal_unpacker_ptr);
		image = Image::basis_universal_unpacker_ptr(data, data_size);
		format = image->get_format();
	} break;
	case COMPRESSION_MODE_S3TC:
	case COMPRESSION_MODE_ETC2:
	case COMPRESSION_MODE_BPTC:
	case COMPRESSION_MODE_ASTC: {
		image.instantiate(w, h, mipmaps, format, p_data.slice(20));
	} break;
	}
	ERR_FAIL_COND(image.is_null());

	if (texture.is_null()) {
		texture = Renderer::texture_2d_create(image);
	}
	else {
		RID new_texture = Renderer::texture_2d_create(image);
		Renderer::texture_replace(texture, new_texture);
	}

	image_stored = true;
	size_override = Size2(w, h);
	Renderer::texture_set_size_override(texture, size_override.width, size_override.height);
	alpha_cache.unref();

	if (keep_all_compressed_buffers || keep_compressed_buffer) {
		compressed_buffer = p_data;
	}
	else {
		compressed_buffer.clear();
	}
}

Vector<uint8_t> CompressedTexture2D::_get_data() const { return compressed_buffer; }

CompressedTexture2D::CompressionMode CompressedTexture2D::get_compression_mode() const
{
	return compression_mode;
}

void CompressedTexture2D::create_from_image(const Ref<Image>& p_image,
	CompressionMode p_compression_mode, bool p_normal_map, float p_lossy_quality)
{
	ERR_FAIL_COND(p_image.is_null() || p_image->is_empty());

	Vector<uint8_t> buffer;

	buffer.resize(20);
	encode_uint16(p_compression_mode, buffer.ptrw());
	encode_uint16(PORTABLE_DATA_FORMAT_UNDEFINED, buffer.ptrw() + 2);
	encode_uint32(p_image->get_format(), buffer.ptrw() + 4);
	encode_uint32(p_image->get_mipmap_count() + 1, buffer.ptrw() + 8);
	encode_uint32(p_image->get_width(), buffer.ptrw() + 12);
	encode_uint32(p_image->get_height(), buffer.ptrw() + 16);

	switch (p_compression_mode) {
	case COMPRESSION_MODE_LOSSLESS:
	case COMPRESSION_MODE_LOSSY: {
		bool lossless_force_png =
			GLOBAL_GET_CACHED(bool, "rendering/textures/lossless_compression/force_png") ||
			!Image::_webp_mem_loader_func; // WebP module disabled.
		bool use_webp = !lossless_force_png && p_image->get_width() <= 16383 &&
						p_image->get_height() <= 16383; // WebP has a size limit.
		for (int i = 0; i < p_image->get_mipmap_count() + 1; i++) {
			Vector<uint8_t> data;
			if (p_compression_mode == COMPRESSION_MODE_LOSSY) {
				data = Image::webp_lossy_packer(
					i ? p_image->get_image_from_mipmap(i) : p_image, p_lossy_quality);
				encode_uint16(PORTABLE_DATA_FORMAT_WEBP, buffer.ptrw() + 2);
			}
			else {
				if (use_webp) {
					data = Image::webp_lossless_packer(
						i ? p_image->get_image_from_mipmap(i) : p_image);
					encode_uint16(PORTABLE_DATA_FORMAT_WEBP, buffer.ptrw() + 2);
				}
				else {
					data = Image::png_packer(i ? p_image->get_image_from_mipmap(i) : p_image);
					encode_uint16(PORTABLE_DATA_FORMAT_PNG, buffer.ptrw() + 2);
				}
			}
			int data_len = data.size();
			buffer.resize(buffer.size() + 4);
			encode_uint32(data_len, buffer.ptrw() + buffer.size() - 4);
			buffer.append_array(data);
		}
	} break;
	case COMPRESSION_MODE_BASIS_UNIVERSAL: {
#ifdef TOOLS_ENABLED
		ERR_FAIL_COND(p_image->is_compressed());
		encode_uint16(PORTABLE_DATA_FORMAT_BASIS_UNIVERSAL, buffer.ptrw() + 2);
		Image::UsedChannels uc = p_image->detect_used_channels(
			p_normal_map ? Image::COMPRESS_SOURCE_NORMAL : Image::COMPRESS_SOURCE_GENERIC);
		Vector<uint8_t> budata = Image::basis_universal_packer(p_image, uc, basisu_params);
		buffer.append_array(budata);
#else
		ERR_FAIL_MSG("Basis Universal compression can only run in editor build.");
#endif
	} break;
	case COMPRESSION_MODE_S3TC:
	case COMPRESSION_MODE_ETC2:
	case COMPRESSION_MODE_BPTC:
	case COMPRESSION_MODE_ASTC: {
		encode_uint16(PORTABLE_DATA_FORMAT_IMAGE, buffer.ptrw() + 2);
		Ref<Image> copy = p_image;
		if (p_image->is_compressed()) {
			CompressionMode expected_compression_mode =
				get_expected_compression_mode(p_image->get_format());
			ERR_FAIL_COND_MSG(expected_compression_mode != p_compression_mode,
				vformat("Mismatched compression mode for image format %s, expected %s, got %s.",
					Image::get_format_name(p_image->get_format()),
					compression_mode_names[expected_compression_mode],
					compression_mode_names[p_compression_mode]));
		}
		encode_uint32(copy->get_format(), buffer.ptrw() + 4);
		buffer.append_array(copy->get_data());
	} break;
	}

	_set_data(buffer);
}

void CompressedTexture2D::update(const Ref<Image>& p_image)
{
	ERR_FAIL_COND_MSG(p_image.is_null(), "Invalid image");
	ERR_FAIL_COND_MSG(texture.is_null(), "Texture is not initialized.");
	ERR_FAIL_COND_MSG(p_image->get_width() != w || p_image->get_height() != h,
		"The new image dimensions must match the texture size.");
	ERR_FAIL_COND_MSG(p_image->get_format() != format,
		"The new image format must match the texture's image format.");
	ERR_FAIL_COND_MSG(mipmaps != p_image->has_mipmaps(),
		"The new image mipmaps configuration must match the texture's image mipmaps configuration");

	Renderer::texture_2d_update(texture, p_image);
	alpha_cache.unref();
	image_stored = true;
	emit_changed();
}

void CompressedTexture2D::set_size_override(const Size2& p_size)
{
	size_override = p_size;
	if (p_size.width > 0) {
		w = int(p_size.width);
	}
	if (p_size.height > 0) {
		h = int(p_size.height);
	}
	Renderer::texture_set_size_override(texture, size_override.width, size_override.height);
}

Size2 CompressedTexture2D::get_size_override() const { return size_override; }

void CompressedTexture2D::set_keep_all_compressed_buffers(bool p_keep)
{
	keep_all_compressed_buffers = p_keep;
}

bool CompressedTexture2D::is_keeping_all_compressed_buffers()
{
	return keep_all_compressed_buffers;
}

void CompressedTexture2D::set_keep_compressed_buffer(bool p_keep)
{
	keep_compressed_buffer = p_keep;
	if (!p_keep) {
		compressed_buffer.clear();
	}
}

bool CompressedTexture2D::is_keeping_compressed_buffer() const
{
	return keep_compressed_buffer;
}

void CompressedTexture2D::set_basisu_compressor_params(
	int p_uastc_level, float p_rdo_quality_loss)
{
	basisu_params.uastc_level = p_uastc_level;
	basisu_params.rdo_quality_loss = p_rdo_quality_loss;
}

Image::Format CompressedTexture2D::get_format() const { return format; }

String CompressedTexture2D::get_load_path() const { return path_to_file; }

int CompressedTexture2D::get_width() const { return w; }

int CompressedTexture2D::get_height() const { return h; }

RID CompressedTexture2D::get_rid() const
{
	if (!texture.is_valid()) {
		texture = Renderer::texture_2d_placeholder_create();
	}
	return texture;
}

void CompressedTexture2D::draw(
	RID p_canvas_item, const Point2& p_pos, const Color& p_modulate, bool p_transpose) const
{
	if ((w | h) == 0) {
		return;
	}
	Renderer::canvas_item_add_texture_rect(
		p_canvas_item, Rect2(p_pos, Size2(w, h)), texture, false, p_modulate, p_transpose);
}

void CompressedTexture2D::draw_rect(RID p_canvas_item, const Rect2& p_rect, bool p_tile,
	const Color& p_modulate, bool p_transpose) const
{
	if ((w | h) == 0) {
		return;
	}
	Renderer::canvas_item_add_texture_rect(
		p_canvas_item, p_rect, texture, p_tile, p_modulate, p_transpose);
}

void CompressedTexture2D::draw_rect_region(RID p_canvas_item, const Rect2& p_rect,
	const Rect2& p_src_rect, const Color& p_modulate, bool p_transpose, bool p_clip_uv) const
{
	if ((w | h) == 0) {
		return;
	}
	Renderer::canvas_item_add_texture_rect_region(
		p_canvas_item, p_rect, texture, p_src_rect, p_modulate, p_transpose, p_clip_uv);
}

bool CompressedTexture2D::is_pixel_opaque(int p_x, int p_y) const
{
	if (alpha_cache.is_valid()) {
		int aw = int(alpha_cache->get_size().width);
		int ah = int(alpha_cache->get_size().height);
		if (aw == 0 || ah == 0 || w == 0 || h == 0) {
			return true;
		}

		int x = p_x * aw / w;
		int y = p_y * ah / h;

		x = CLAMP(x, 0, aw - 1);
		y = CLAMP(y, 0, ah - 1);

		return alpha_cache->get_bit(x, y);
	}

	return true;
}

bool CompressedTexture2D::has_alpha() const
{
	if (is_portable) {
		return (format == Image::FORMAT_LA8 || format == Image::FORMAT_RGBA8);
	}
	return false;
}

Ref<Image> CompressedTexture2D::get_image() const
{
	if (is_portable && !image_stored) {
		return Ref<Image>();
	}
	if (texture.is_valid()) {
		return Renderer::texture_2d_get(texture);
	}
	else {
		return Ref<Image>();
	}
}

void CompressedTexture2D::reload_from_file()
{
	String path = get_path();
	if (!path.is_resource_file()) {
		return;
	}

	path = ResourceLoader::path_remap(path);   // remap for translation
	path = ResourceLoader::import_remap(path); // remap for import
	if (!path.is_resource_file()) {
		return;
	}

	load(path);
}

Error CompressedTexture2D::load(const String& p_path)
{
	int lw, lh;
	Ref<Image> image;
	image.instantiate();

	bool request_3d = false;
	bool request_normal = false;
	bool request_roughness = false;
	int mipmap_limit = 0;

	Error err = _load_data(p_path, lw, lh, image, request_3d, request_normal, request_roughness,
		mipmap_limit);
	if (err != OK) {
		return err;
	}

	path_to_file = p_path;

	if (texture.is_valid()) {
		RID new_texture = Renderer::texture_2d_create(image);
		Renderer::texture_replace(texture, new_texture);
	}
	else {
		texture = Renderer::texture_2d_create(image);
	}
	if (lw || lh) {
		Renderer::texture_set_size_override(texture, lw, lh);
	}

	w = lw;
	h = lh;
	format = image->get_format();
	mipmaps = image->has_mipmaps();
	is_portable = false;
	image_stored = true;
	alpha_cache.unref();

	if (get_path().is_empty()) {
		Renderer::texture_set_path(texture, p_path);
	}
	else {
		Renderer::texture_set_path(texture, get_path());
	}

	emit_changed();
	return OK;
}

Ref<Image> CompressedTexture2D::load_image_from_file(Ref<FileAccess> f, int p_size_limit)
{
	uint32_t data_format = f->get_32();
	uint32_t w = f->get_16();
	uint32_t h = f->get_16();
	uint32_t mipmaps = f->get_32();
	Image::Format format = Image::Format(f->get_32());

	if (data_format == DATA_FORMAT_PNG || data_format == DATA_FORMAT_WEBP) {
		// look for a PNG or WebP file inside

		int sw = w;
		int sh = h;

		// mipmaps need to be read independently, they will be later combined
		Vector<Ref<Image>> mipmap_images;
		uint64_t total_size = 0;

		for (uint32_t i = 0; i < mipmaps + 1; i++) {
			uint32_t size = f->get_32();

			if (p_size_limit > 0 && i < (mipmaps - 1) && (sw > p_size_limit || sh > p_size_limit)) {
				// can't load this due to size limit
				sw = MAX(sw >> 1, 1);
				sh = MAX(sh >> 1, 1);
				f->seek(f->get_position() + size);
				continue;
			}

			Vector<uint8_t> pv;
			pv.resize(size);
			{
				uint8_t* wr = pv.ptrw();
				f->get_buffer(wr, size);
			}

			Ref<Image> img;
			if (data_format == DATA_FORMAT_PNG && Image::png_unpacker) {
				img = Image::png_unpacker(pv);
			}
			else if (data_format == DATA_FORMAT_WEBP && Image::webp_unpacker) {
				img = Image::webp_unpacker(pv);
			}

			if (img.is_null() || img->is_empty()) {
				ERR_FAIL_COND_V(img.is_null() || img->is_empty(), Ref<Image>());
			}
			// If the image is compressed and its format doesn't match the desired format, return an
			// empty reference. This is done to avoid recompressing the image on load.
			ERR_FAIL_COND_V(img->is_compressed() && format != img->get_format(), Ref<Image>());

			// The format will actually be the format of the header,
			// as it may have changed on compression.
			if (format != img->get_format()) {
				// Convert the image to the desired format.
				// Note: We are not decompressing the image here, just changing its format.
				// It's important that all images in the texture array share the same format for
				// correct rendering.
				img->convert(format);
			}

			total_size += img->get_data().size();

			mipmap_images.push_back(img);

			sw = MAX(sw >> 1, 1);
			sh = MAX(sh >> 1, 1);
		}

		Ref<Image> image;
		image.instantiate();

		if (mipmap_images.size() == 1) {
			// only one image (which will most likely be the case anyway for this format)
			image = mipmap_images[0];
			return image;
		}
		else {
			// rarer use case, but needs to be supported
			Vector<uint8_t> img_data;
			img_data.resize(total_size);

			{
				uint8_t* wr = img_data.ptrw();

				int ofs = 0;
				for (int i = 0; i < mipmap_images.size(); i++) {
					Vector<uint8_t> id = mipmap_images[i]->get_data();
					int len = id.size();
					const uint8_t* r = id.ptr();
					memcpy(&wr[ofs], r, len);
					ofs += len;
				}
			}

			image->set_data(w, h, true, mipmap_images[0]->get_format(), img_data);
			return image;
		}
	}
	else if (data_format == DATA_FORMAT_BASIS_UNIVERSAL) {
		int sw = w;
		int sh = h;
		uint32_t size = f->get_32();
		if (p_size_limit > 0 && (sw > p_size_limit || sh > p_size_limit)) {
			// can't load this due to size limit
			sw = MAX(sw >> 1, 1);
			sh = MAX(sh >> 1, 1);
			f->seek(f->get_position() + size);
			return Ref<Image>();
		}
		Vector<uint8_t> pv;
		pv.resize(size);
		{
			uint8_t* wr = pv.ptrw();
			f->get_buffer(wr, size);
		}
		Ref<Image> img;
		img = Image::basis_universal_unpacker(pv);
		if (img.is_null() || img->is_empty()) {
			ERR_FAIL_COND_V(img.is_null() || img->is_empty(), Ref<Image>());
		}
		format = img->get_format();
		sw = MAX(sw >> 1, 1);
		sh = MAX(sh >> 1, 1);
		return img;
	}
	else if (data_format == DATA_FORMAT_IMAGE) {
		int size = Image::get_image_data_size(w, h, format, mipmaps ? true : false);

		for (uint32_t i = 0; i < mipmaps + 1; i++) {
			int tw, th;
			int ofs = Image::get_image_mipmap_offset_and_dimensions(w, h, format, i, tw, th);

			if (p_size_limit > 0 && i < mipmaps && (p_size_limit > tw || p_size_limit > th)) {
				if (ofs) {
					f->seek(f->get_position() + ofs);
				}
				continue; // oops, size limit enforced, go to next
			}

			Vector<uint8_t> data;
			data.resize(size - ofs);

			{
				uint8_t* wr = data.ptrw();
				f->get_buffer(wr, data.size());
			}

			Ref<Image> image =
				Image::create_from_data(tw, th, mipmaps - i ? true : false, format, data);

			return image;
		}
	}

	return Ref<Image>();
}

CompressedTexture2D::~CompressedTexture2D()
{
	if (texture.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::free_rid(texture);
	}
}

void CompressedTexture3D::set_path(const String& p_path, bool p_take_over)
{
	if (texture.is_valid()) {
		Renderer::texture_set_path(texture, p_path);
	}

	Resource::set_path(p_path, p_take_over);
}

Image::Format CompressedTexture3D::get_format() const { return format; }

Error CompressedTexture3D::_load_data(const String& p_path, Vector<Ref<Image>>& r_data,
	Image::Format& r_format, int& r_width, int& r_height, int& r_depth, bool& r_mipmaps)
{
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(f.is_null(), ERR_CANT_OPEN, vformat("Unable to open file: %s.", p_path));

	uint8_t header[4];
	f->get_buffer(header, 4);
	ERR_FAIL_COND_V(header[0] != 'G' || header[1] != 'S' || header[2] != 'T' || header[3] != 'L',
		ERR_FILE_UNRECOGNIZED);

	// stored as compressed textures (used for lossless and lossy compression)
	uint32_t version = f->get_32();

	if (version > FORMAT_VERSION) {
		ERR_FAIL_V_MSG(ERR_FILE_CORRUPT, "Compressed texture file is too new.");
	}

	r_depth = f->get_32(); // depth
	f->get_32();		   // ignored (mode)
	f->get_32();		   // ignored (data format)

	f->get_32(); // ignored
	int mipmap_count = f->get_32();
	f->get_32(); // ignored
	f->get_32(); // ignored

	r_mipmaps = mipmap_count != 0;

	r_data.clear();

	for (int i = 0; i < (r_depth + mipmap_count); i++) {
		Ref<Image> image = CompressedTexture2D::load_image_from_file(f, 0);
		ERR_FAIL_COND_V(image.is_null() || image->is_empty(), ERR_CANT_OPEN);
		if (i == 0) {
			r_format = image->get_format();
			r_width = image->get_width();
			r_height = image->get_height();
		}
		r_data.push_back(image);
	}

	return OK;
}

Error CompressedTexture3D::load(const String& p_path)
{
	Vector<Ref<Image>> data;
	Image::Format r_format;
	int r_width, r_height, r_depth;
	bool r_mipmaps;

	Error err = _load_data(p_path, data, r_format, r_width, r_height, r_depth, r_mipmaps);
	if (err != OK) {
		return err;
	}

	path_to_file = p_path;

	RID new_tex = Renderer::texture_3d_create(r_format, r_width, r_height, r_depth, r_mipmaps, data);
	ERR_FAIL_COND_V(new_tex.is_null(), ERR_CANT_CREATE);

	if (texture.is_valid()) {
		Renderer::texture_replace(texture, new_tex);
	}
	else {
		texture = new_tex;
	}

	format = r_format;
	w = r_width;
	h = r_height;
	d = r_depth;
	mipmaps = r_mipmaps;

	if (get_path().is_empty()) {
		Renderer::texture_set_path(texture, p_path);
	}
	else {
		Renderer::texture_set_path(texture, get_path());
	}

	emit_changed();
	return OK;
}

String CompressedTexture3D::get_load_path() const { return path_to_file; }

int CompressedTexture3D::get_width() const { return w; }

int CompressedTexture3D::get_height() const { return h; }

int CompressedTexture3D::get_depth() const { return d; }

bool CompressedTexture3D::has_mipmaps() const { return mipmaps; }

RID CompressedTexture3D::get_rid() const
{
	if (!texture.is_valid()) {
		texture = Renderer::texture_3d_placeholder_create();
	}
	return texture;
}

Vector<Ref<Image>> CompressedTexture3D::get_data() const
{
	if (texture.is_valid()) {
		return Renderer::texture_3d_get(texture);
	}
	else {
		return Vector<Ref<Image>>();
	}
}

void CompressedTexture3D::reload_from_file()
{
	String path = get_path();
	if (!path.is_resource_file()) {
		return;
	}

	path = ResourceLoader::path_remap(path);   // remap for translation
	path = ResourceLoader::import_remap(path); // remap for import
	if (!path.is_resource_file()) {
		return;
	}

	load(path);
}

CompressedTexture3D::~CompressedTexture3D()
{
	if (texture.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::free_rid(texture);
	}
}

void CompressedTextureLayered::set_path(const String& p_path, bool p_take_over)
{
	if (texture.is_valid()) {
		Renderer::texture_set_path(texture, p_path);
	}

	Resource::set_path(p_path, p_take_over);
}

Image::Format CompressedTextureLayered::get_format() const { return format; }

Error CompressedTextureLayered::_load_data(
	const String& p_path, Vector<Ref<Image>>& images, int& mipmap_limit, int p_size_limit)
{
	ERR_FAIL_COND_V(images.size() != 0, ERR_INVALID_PARAMETER);

	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V_MSG(f.is_null(), ERR_CANT_OPEN, vformat("Unable to open file: %s.", p_path));

	uint8_t header[4];
	f->get_buffer(header, 4);
	if (header[0] != 'G' || header[1] != 'S' || header[2] != 'T' || header[3] != 'L') {
		ERR_FAIL_V_MSG(
			ERR_FILE_CORRUPT, "Compressed texture layered file is corrupt (Bad header).");
	}

	uint32_t version = f->get_32();

	if (version > FORMAT_VERSION) {
		ERR_FAIL_V_MSG(ERR_FILE_CORRUPT, "Compressed texture file is too new.");
	}

	uint32_t layer_count = f->get_32(); // layer count
	uint32_t type = f->get_32();		// layer type
	ERR_FAIL_COND_V((int)type != layered_type, ERR_INVALID_DATA);

	uint32_t df = f->get_32(); // data format
	mipmap_limit = int(f->get_32());
	// reserved
	f->get_32();
	f->get_32();
	f->get_32();

	if (!(df & FORMAT_BIT_STREAM)) {
		p_size_limit = 0;
	}

	images.resize(layer_count);

	for (uint32_t i = 0; i < layer_count; i++) {
		Ref<Image> image = CompressedTexture2D::load_image_from_file(f, p_size_limit);
		ERR_FAIL_COND_V(image.is_null() || image->is_empty(), ERR_CANT_OPEN);
		images.write[i] = image;
	}

	return OK;
}

Error CompressedTextureLayered::load(const String& p_path)
{
	Vector<Ref<Image>> images;
	int mipmap_limit = 0;

	Error err = _load_data(p_path, images, mipmap_limit);
	if (err != OK) {
		return err;
	}

	path_to_file = p_path;

	RID new_tex =
		Renderer::texture_2d_layered_create(images, RSE::TextureLayeredType(layered_type));
	ERR_FAIL_COND_V(new_tex.is_null(), ERR_CANT_CREATE);

	if (texture.is_valid()) {
		Renderer::texture_replace(texture, new_tex);
	}
	else {
		texture = new_tex;
	}

	w = images[0]->get_width();
	h = images[0]->get_height();
	mipmaps = images[0]->has_mipmaps();
	format = images[0]->get_format();
	layers = images.size();

	if (get_path().is_empty()) {
		Renderer::texture_set_path(texture, p_path);
	}
	else {
		Renderer::texture_set_path(texture, get_path());
	}

	emit_changed();
	return OK;
}

String CompressedTextureLayered::get_load_path() const { return path_to_file; }

int CompressedTextureLayered::get_width() const { return w; }

int CompressedTextureLayered::get_height() const { return h; }

int CompressedTextureLayered::get_layers() const { return layers; }

bool CompressedTextureLayered::has_mipmaps() const { return mipmaps; }

TextureLayered::LayeredType CompressedTextureLayered::get_layered_type() const
{
	return layered_type;
}

RID CompressedTextureLayered::get_rid() const
{
	if (!texture.is_valid()) {
		texture = Renderer::texture_2d_layered_placeholder_create(
			RSE::TextureLayeredType(layered_type));
	}
	return texture;
}

Ref<Image> CompressedTextureLayered::get_layer_data(int p_layer) const
{
	if (texture.is_valid()) {
		ERR_FAIL_INDEX_V(p_layer, get_layers(), Ref<Image>());
		return Renderer::texture_2d_layer_get(texture, p_layer);
	}
	else {
		return Ref<Image>();
	}
}

void CompressedTextureLayered::reload_from_file()
{
	String path = get_path();
	if (!path.is_resource_file()) {
		return;
	}
	path = ResourceLoader::path_remap(path);   // remap for translation
	path = ResourceLoader::import_remap(path); // remap for import
	if (!path.is_resource_file()) {
		return;
	}
	load(path);
}

CompressedTextureLayered::CompressedTextureLayered(LayeredType p_type) { layered_type = p_type; }

CompressedTextureLayered::~CompressedTextureLayered()
{
	if (texture.is_valid()) {
		ERR_FAIL_NULL(Renderer::data);
		Renderer::free_rid(texture);
	}
}

// --- compressed_texture_resource_format.cpp ---

#include "compressed_texture_resource_format.h"

#include "scene/resources/compressed_texture.h"

Ref<Resource> ResourceFormatLoaderCompressedTexture::load(const String& p_path,
	const String& p_original_path, Error* r_error, bool p_use_sub_threads, float* r_progress,
	CacheMode p_cache_mode)
{
	if (p_path.has_extension("ctex")) {
		Ref<CompressedTexture2D> st;
		st.instantiate();
		Error err = st->load(p_path);
		if (r_error) {
			*r_error = err;
		}
		if (err != OK) {
			return Ref<Resource>();
		}
		return st;
	}

	if (p_path.has_extension("ctexarray") || p_path.has_extension("ccube") ||
		p_path.has_extension("ccubearray")) {
		Ref<CompressedTextureLayered> ct;
		if (p_path.has_extension("ctexarray")) {
			ct = memnew(CompressedTextureLayered(TextureLayered::LAYERED_TYPE_2D_ARRAY));
		}
		else if (p_path.has_extension("ccube")) {
			ct = memnew(CompressedTextureLayered(TextureLayered::LAYERED_TYPE_CUBEMAP));
		}
		else {
			ct = memnew(CompressedTextureLayered(TextureLayered::LAYERED_TYPE_CUBEMAP_ARRAY));
		}
		Error err = ct->load(p_path);
		if (r_error) {
			*r_error = err;
		}
		if (err != OK) {
			return Ref<Resource>();
		}
		return ct;
	}

	if (p_path.has_extension("ctex3d")) {
		Ref<CompressedTexture3D> st;
		st.instantiate();
		Error err = st->load(p_path);
		if (r_error) {
			*r_error = err;
		}
		if (err != OK) {
			return Ref<Resource>();
		}
		return st;
	}

	if (r_error) {
		*r_error = ERR_FILE_UNRECOGNIZED;
	}
	return Ref<Resource>();
}

void ResourceFormatLoaderCompressedTexture::get_recognized_extensions(
	List<String>* p_extensions) const
{
	p_extensions->push_back("ctex");
	p_extensions->push_back("ctexarray");
	p_extensions->push_back("ccube");
	p_extensions->push_back("ccubearray");
	p_extensions->push_back("ctex3d");
}

bool ResourceFormatLoaderCompressedTexture::handles_type(const String& p_type) const
{
	return p_type == "CompressedTexture2D" || p_type == "CompressedTextureLayered" ||
		   p_type == "CompressedTexture3D";
}

String ResourceFormatLoaderCompressedTexture::get_resource_type(const String& p_path) const
{
	if (p_path.has_extension("ctex")) {
		return "CompressedTexture2D";
	}
	if (p_path.has_extension("ctexarray") || p_path.has_extension("ccube") ||
		p_path.has_extension("ccubearray")) {
		return "CompressedTextureLayered";
	}
	if (p_path.has_extension("ctex3d")) {
		return "CompressedTexture3D";
	}
	return "";
}
