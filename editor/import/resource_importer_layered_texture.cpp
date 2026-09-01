/**************************************************************************/
/*  resource_importer_layered_texture.cpp                                 */
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
#include "core/error/error_macros.h"
#include "core/io/image_loader.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/import/resource_importer_texture.h"
#include "resource_importer_layered_texture.h"
#include "scene/resources/compressed_texture.h"

String ResourceImporterLayeredTexture::get_importer_name() const
{
	switch (mode) {
	case MODE_CUBEMAP: {
		return "cubemap_texture";
	} break;
	case MODE_2D_ARRAY: {
		return "2d_array_texture";
	} break;
	case MODE_CUBEMAP_ARRAY: {
		return "cubemap_array_texture";
	} break;
	case MODE_3D: {
		return "3d_texture";
	} break;
	}

	ERR_FAIL_V("");
}

String ResourceImporterLayeredTexture::get_visible_name() const
{
	switch (mode) {
	case MODE_CUBEMAP: {
		return "Cubemap";
	} break;
	case MODE_2D_ARRAY: {
		return "Texture2DArray";
	} break;
	case MODE_CUBEMAP_ARRAY: {
		return "CubemapArray";
	} break;
	case MODE_3D: {
		return "Texture3D";
	} break;
	}

	ERR_FAIL_V("");
}

void ResourceImporterLayeredTexture::get_recognized_extensions(List<String>* p_extensions) const
{
	ImageLoader::get_recognized_extensions(p_extensions);
}

String ResourceImporterLayeredTexture::get_save_extension() const
{
	switch (mode) {
	case MODE_CUBEMAP: {
		return "ccube";
	} break;
	case MODE_2D_ARRAY: {
		return "ctexarray";
	} break;
	case MODE_CUBEMAP_ARRAY: {
		return "ccubearray";
	} break;
	case MODE_3D: {
		return "ctex3d";
	} break;
	}

	ERR_FAIL_V(String());
}

String ResourceImporterLayeredTexture::get_resource_type() const
{
	switch (mode) {
	case MODE_CUBEMAP: {
		return "CompressedCubemap";
	} break;
	case MODE_2D_ARRAY: {
		return "CompressedTexture2DArray";
	} break;
	case MODE_CUBEMAP_ARRAY: {
		return "CompressedCubemapArray";
	} break;
	case MODE_3D: {
		return "CompressedTexture3D";
	} break;
	}
	ERR_FAIL_V(String());
}

int ResourceImporterLayeredTexture::get_preset_count() const { return 0; }

String ResourceImporterLayeredTexture::get_preset_name(int p_idx) const { return ""; }

void ResourceImporterLayeredTexture::_save_tex(Vector<Ref<Image>> p_images, const String& p_to_path,
	int p_compress_mode, float p_lossy, const Image::BasisUniversalPackerParams& p_basisu_params,
	Image::CompressMode p_vram_compression, Image::CompressProfile p_vram_compression_profile,
	Image::CompressSource p_csource, Image::UsedChannels used_channels, bool p_mipmaps,
	bool p_force_po2, Image::BPTCFormat p_bptc_format)
{
	Vector<Ref<Image>> mipmap_images; // for 3D

	if (mode == MODE_3D) {
		// 3D saves in its own way

		for (int i = 0; i < p_images.size(); i++) {
			if (p_images.write[i]->has_mipmaps()) {
				p_images.write[i]->clear_mipmaps();
			}

			if (p_force_po2) {
				p_images.write[i]->resize_to_po2();
			}
		}

		if (p_mipmaps) {
			Vector<Ref<Image>> parent_images = p_images;
			// create 3D mipmaps, this is horrible, though not used very often
			int w = p_images[0]->get_width();
			int h = p_images[0]->get_height();
			int d = p_images.size();

			while (w > 1 || h > 1 || d > 1) {
				Vector<Ref<Image>> mipmaps;
				int mm_w = MAX(1, w >> 1);
				int mm_h = MAX(1, h >> 1);
				int mm_d = MAX(1, d >> 1);

				for (int i = 0; i < mm_d; i++) {
					Ref<Image> mm =
						Image::create_empty(mm_w, mm_h, false, p_images[0]->get_format());
					Vector3 pos;
					pos.z = float(i) * float(d) / float(mm_d) + 0.5;
					for (int x = 0; x < mm_w; x++) {
						for (int y = 0; y < mm_h; y++) {
							pos.x = float(x) * float(w) / float(mm_w) + 0.5;
							pos.y = float(y) * float(h) / float(mm_h) + 0.5;

							Vector3i posi = Vector3i(pos);
							Vector3 fract = pos - Vector3(posi);
							Vector3i posi_n = posi;
							if (posi_n.x < w - 1) {
								posi_n.x++;
							}
							if (posi_n.y < h - 1) {
								posi_n.y++;
							}
							if (posi_n.z < d - 1) {
								posi_n.z++;
							}

							Color c000 = parent_images[posi.z]->get_pixel(posi.x, posi.y);
							Color c100 = parent_images[posi.z]->get_pixel(posi_n.x, posi.y);
							Color c010 = parent_images[posi.z]->get_pixel(posi.x, posi_n.y);
							Color c110 = parent_images[posi.z]->get_pixel(posi_n.x, posi_n.y);
							Color c001 = parent_images[posi_n.z]->get_pixel(posi.x, posi.y);
							Color c101 = parent_images[posi_n.z]->get_pixel(posi_n.x, posi.y);
							Color c011 = parent_images[posi_n.z]->get_pixel(posi.x, posi_n.y);
							Color c111 = parent_images[posi_n.z]->get_pixel(posi_n.x, posi_n.y);

							Color cx00 = c000.lerp(c100, fract.x);
							Color cx01 = c001.lerp(c101, fract.x);
							Color cx10 = c010.lerp(c110, fract.x);
							Color cx11 = c011.lerp(c111, fract.x);

							Color cy0 = cx00.lerp(cx10, fract.y);
							Color cy1 = cx01.lerp(cx11, fract.y);

							Color cz = cy0.lerp(cy1, fract.z);

							mm->set_pixel(x, y, cz);
						}
					}

					mipmaps.push_back(mm);
				}

				w = mm_w;
				h = mm_h;
				d = mm_d;

				mipmap_images.append_array(mipmaps);
				parent_images = mipmaps;
			}
		}
	}
	else {
		for (int i = 0; i < p_images.size(); i++) {
			if (p_force_po2) {
				p_images.write[i]->resize_to_po2();
			}

			if (p_mipmaps) {
				p_images.write[i]->generate_mipmaps(p_csource == Image::COMPRESS_SOURCE_NORMAL);
			}
			else {
				p_images.write[i]->clear_mipmaps();
			}
		}
	}

	Ref<FileAccess> f = FileAccess::open(p_to_path, FileAccess::WRITE);
	f->store_8('G');
	f->store_8('S');
	f->store_8('T');
	f->store_8('L');

	f->store_32(CompressedTextureLayered::FORMAT_VERSION);
	f->store_32(p_images.size()); // For 2d layers or 3d depth.
	f->store_32(mode);
	f->store_32(0);

	f->store_32(0);
	f->store_32(mipmap_images.size()); // Adjust the amount of mipmaps.
	f->store_32(0);
	f->store_32(0);

	if ((p_compress_mode == COMPRESS_LOSSLESS || p_compress_mode == COMPRESS_LOSSY) &&
		p_images[0]->get_format() >= Image::FORMAT_RF) {
		p_compress_mode = COMPRESS_VRAM_UNCOMPRESSED; // These can't go as lossy.
	}

	for (int i = 0; i < p_images.size(); i++) {
		ResourceImporterTexture::save_to_ctex_format(f, p_images[i],
			ResourceImporterTexture::CompressMode(p_compress_mode), used_channels,
			p_vram_compression, p_vram_compression_profile, p_lossy, p_basisu_params,
			p_bptc_format);
	}

	for (int i = 0; i < mipmap_images.size(); i++) {
		ResourceImporterTexture::save_to_ctex_format(f, mipmap_images[i],
			ResourceImporterTexture::CompressMode(p_compress_mode), used_channels,
			p_vram_compression, p_vram_compression_profile, p_lossy, p_basisu_params,
			p_bptc_format);
	}
}

const char* ResourceImporterLayeredTexture::compression_formats[] = {
	"s3tc_bptc", "etc2_astc", nullptr};

ResourceImporterLayeredTexture* ResourceImporterLayeredTexture::singleton = nullptr;

ResourceImporterLayeredTexture::ResourceImporterLayeredTexture(bool p_singleton)
{
	// This should only be set through the EditorNode.
	if (p_singleton) {
		singleton = this;
	}

	mode = MODE_CUBEMAP;
}

ResourceImporterLayeredTexture::~ResourceImporterLayeredTexture()
{
	if (singleton == this) {
		singleton = nullptr;
	}
}


