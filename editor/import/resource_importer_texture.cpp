/**************************************************************************/
/*  resource_importer_texture.cpp                                         */
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
#include "core/io/config_file.h"
#include "core/io/image_loader.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "editor/themes/editor_theme_manager.h"
#include "resource_importer_texture.h"
#include "scene/resources/compressed_texture.h"

void ResourceImporterTexture::_texture_reimport_roughness(const Ref<CompressedTexture2D>& p_tex,
	const String& p_normal_path, RSE::TextureDetectRoughnessChannel p_channel)
{
	ERR_FAIL_COND(p_tex.is_null());

	MutexLock lock(singleton->mutex);
	StringName path = p_tex->get_path();

	if (!singleton->make_flags.has(path)) {
		singleton->make_flags[path] = MakeInfo();
	}

	singleton->make_flags[path].flags |= MAKE_ROUGHNESS_FLAG;
	singleton->make_flags[path].channel_for_roughness = p_channel;
	singleton->make_flags[path].normal_path_for_roughness = p_normal_path;
}

void ResourceImporterTexture::_texture_reimport_3d(const Ref<CompressedTexture2D>& p_tex)
{
	ERR_FAIL_COND(p_tex.is_null());

	MutexLock lock(singleton->mutex);
	StringName path = p_tex->get_path();

	if (!singleton->make_flags.has(path)) {
		singleton->make_flags[path] = MakeInfo();
	}

	singleton->make_flags[path].flags |= MAKE_3D_FLAG;
}

void ResourceImporterTexture::_texture_reimport_normal(const Ref<CompressedTexture2D>& p_tex)
{
	ERR_FAIL_COND(p_tex.is_null());

	MutexLock lock(singleton->mutex);
	StringName path = p_tex->get_path();

	if (!singleton->make_flags.has(path)) {
		singleton->make_flags[path] = MakeInfo();
	}

	singleton->make_flags[path].flags |= MAKE_NORMAL_FLAG;
}

String ResourceImporterTexture::get_importer_name() const { return "texture"; }

String ResourceImporterTexture::get_visible_name() const { return "Texture2D"; }

void ResourceImporterTexture::get_recognized_extensions(List<String>* p_extensions) const
{
	ImageLoader::get_recognized_extensions(p_extensions);
}

String ResourceImporterTexture::get_save_extension() const { return "ctex"; }

String ResourceImporterTexture::get_resource_type() const { return "CompressedTexture2D"; }

int ResourceImporterTexture::get_preset_count() const { return 3; }

String ResourceImporterTexture::get_preset_name(int p_idx) const
{
	static const char* preset_names[] = {
		TTRC("2D/3D (Auto-Detect)"),
		TTRC("2D"),
		TTRC("3D"),
	};

	return TTRGET(preset_names[p_idx]);
}

void ResourceImporterTexture::_save_ctex(const Ref<Image>& p_image, const String& p_to_path,
	CompressMode p_compress_mode, float p_lossy_quality,
	const Image::BasisUniversalPackerParams& p_basisu_params,
	Image::CompressMode p_vram_compression, Image::CompressProfile p_vram_compression_profile,
	bool p_mipmaps, bool p_streamable, bool p_detect_3d, bool p_detect_roughness,
	bool p_detect_normal, bool p_force_normal, bool p_srgb_friendly,
	bool p_force_po2_for_compressed, uint32_t p_limit_mipmap, const Ref<Image>& p_normal,
	Image::RoughnessChannel p_roughness_channel)
{
	Ref<FileAccess> f = FileAccess::open(p_to_path, FileAccess::WRITE);
	ERR_FAIL_COND(f.is_null());

	// Godot Streamable Texture 2D.
	f->store_8('G');
	f->store_8('S');
	f->store_8('T');
	f->store_8('2');

	// Current format version.
	f->store_32(CompressedTexture2D::FORMAT_VERSION);

	// Texture may be resized later, so original size must be saved first.
	f->store_32(p_image->get_width());
	f->store_32(p_image->get_height());

	uint32_t flags = 0;
	if (p_streamable) {
		flags |= CompressedTexture2D::FORMAT_BIT_STREAM;
	}
	if (p_mipmaps) {
		flags |= CompressedTexture2D::FORMAT_BIT_HAS_MIPMAPS;
	}
	if (p_detect_3d) {
		flags |= CompressedTexture2D::FORMAT_BIT_DETECT_3D;
	}
	if (p_detect_roughness) {
		flags |= CompressedTexture2D::FORMAT_BIT_DETECT_ROUGNESS;
	}
	if (p_detect_normal) {
		flags |= CompressedTexture2D::FORMAT_BIT_DETECT_NORMAL;
	}

	f->store_32(flags);
	f->store_32(p_limit_mipmap);

	// Reserved.
	f->store_32(0);
	f->store_32(0);
	f->store_32(0);

	if ((p_compress_mode == COMPRESS_LOSSLESS || p_compress_mode == COMPRESS_LOSSY) &&
		p_image->get_format() >= Image::FORMAT_RF) {
		p_compress_mode = COMPRESS_VRAM_UNCOMPRESSED; // these can't go as lossy
	}

	Ref<Image> image = p_image->duplicate();

	if (p_mipmaps) {
		if (p_force_po2_for_compressed && (p_compress_mode == COMPRESS_BASIS_UNIVERSAL ||
											  p_compress_mode == COMPRESS_VRAM_COMPRESSED)) {
			image->resize_to_po2();
		}

		if (!image->has_mipmaps() || p_force_normal) {
			image->generate_mipmaps(p_force_normal);
		}

	}
	else {
		image->clear_mipmaps();
	}

	// Generate roughness mipmaps from normal texture.
	if (image->has_mipmaps() && p_normal.is_valid()) {
		image->generate_mipmap_roughness(p_roughness_channel, p_normal);
	}

	// Optimization: Only check for color channels when compressing as BasisU or VRAM.
	Image::UsedChannels used_channels = Image::USED_CHANNELS_RGBA;

	if (p_compress_mode == COMPRESS_BASIS_UNIVERSAL ||
		p_compress_mode == COMPRESS_VRAM_COMPRESSED) {
		Image::CompressSource comp_source = Image::COMPRESS_SOURCE_GENERIC;
		if (p_force_normal) {
			comp_source = Image::COMPRESS_SOURCE_NORMAL;
		}
		else if (p_srgb_friendly) {
			comp_source = Image::COMPRESS_SOURCE_SRGB;
		}

		used_channels = image->detect_used_channels(comp_source);
	}

	save_to_ctex_format(f, image, p_compress_mode, used_channels, p_vram_compression,
		p_vram_compression_profile, p_lossy_quality, p_basisu_params, Image::BPTC_DETECT);
}

void ResourceImporterTexture::_remap_channels(Ref<Image>& r_image, ChannelRemap p_options[4])
{
	ERR_FAIL_COND(r_image->is_compressed());

	// Currently HDR inverted remapping is not allowed.
	bool attempted_hdr_inverted = false;
	if (r_image->get_format() >= Image::FORMAT_RF &&
		r_image->get_format() <= Image::FORMAT_RGBE9995) {
		// Formats which can hold HDR data cannot be inverted the same way as unsigned normalized
		// ones (1.0 - channel).
		for (int i = 0; i < 4; i++) {
			switch (p_options[i]) {
			case REMAP_INV_R:
				attempted_hdr_inverted = true;
				p_options[i] = REMAP_R;
				break;
			case REMAP_INV_G:
				attempted_hdr_inverted = true;
				p_options[i] = REMAP_G;
				break;
			case REMAP_INV_B:
				attempted_hdr_inverted = true;
				p_options[i] = REMAP_B;
				break;
			case REMAP_INV_A:
				attempted_hdr_inverted = true;
				p_options[i] = REMAP_A;
				break;
			default:
				break;
			}
		}
	}

	if (attempted_hdr_inverted) {
		WARN_PRINT("Attempted to use an inverted channel remap on an HDR image. The remap has been "
				   "changed to its uninverted equivalent.");
	}

	// Optimization: Set the remap from 'unused' to either 0 or 1 to avoid repeated checks in the
	// conversion loop.
	for (int i = 0; i < 4; i++) {
		if (p_options[i] == REMAP_UNUSED) {
			p_options[i] = i == 3 ? REMAP_1 : REMAP_0;
		}
	}

	// Expand the image's channel count in the event that the current set of channels doesn't allow
	// for the desired remap.
	const Image::Format original_format = r_image->get_format();
	const uint32_t channel_mask = Image::get_format_component_mask(original_format);

	// Whether a channel is supported by the format itself.
	const bool has_channel_r = channel_mask & 0x1;
	const bool has_channel_g = channel_mask & 0x2;
	const bool has_channel_b = channel_mask & 0x4;
	const bool has_channel_a = channel_mask & 0x8;

	// Whether a certain channel needs to be remapped.
	const bool remap_r =
		p_options[0] != REMAP_R ? !(!has_channel_r && p_options[0] == REMAP_0) : false;
	const bool remap_g =
		p_options[1] != REMAP_G ? !(!has_channel_g && p_options[1] == REMAP_0) : false;
	const bool remap_b =
		p_options[2] != REMAP_B ? !(!has_channel_b && p_options[2] == REMAP_0) : false;
	const bool remap_a =
		p_options[3] != REMAP_A ? !(!has_channel_a && p_options[3] == REMAP_1) : false;

	if (!(remap_r || remap_g || remap_b || remap_a)) {
		// Default color map, do nothing.
		return;
	}

	// Whether a certain channel set is needed, either from the source or the remap.
	const bool needs_rg = remap_g || has_channel_g;
	const bool needs_rgb = remap_b || has_channel_b;
	const bool needs_rgba = remap_a || has_channel_a;

	bool could_not_expand = false;
	switch (original_format) {
	case Image::FORMAT_R8:
	case Image::FORMAT_RG8:
	case Image::FORMAT_RGB8: {
		// Convert to either RGBA8, RGB8 or RG8.
		if (needs_rgba) {
			r_image->convert(Image::FORMAT_RGBA8);
		}
		else if (needs_rgb) {
			r_image->convert(Image::FORMAT_RGB8);
		}
		else if (needs_rg) {
			r_image->convert(Image::FORMAT_RG8);
		}
	} break;
	case Image::FORMAT_RH:
	case Image::FORMAT_RGH:
	case Image::FORMAT_RGBH: {
		// Convert to either RGBAH, RGBH or RGH.
		if (needs_rgba) {
			r_image->convert(Image::FORMAT_RGBAH);
		}
		else if (needs_rgb) {
			r_image->convert(Image::FORMAT_RGBH);
		}
		else if (needs_rg) {
			r_image->convert(Image::FORMAT_RGH);
		}
	} break;
	case Image::FORMAT_RF:
	case Image::FORMAT_RGF:
	case Image::FORMAT_RGBF: {
		// Convert to either RGBAF, RGBF or RGF.
		if (needs_rgba) {
			r_image->convert(Image::FORMAT_RGBAF);
		}
		else if (needs_rgb) {
			r_image->convert(Image::FORMAT_RGBF);
		}
		else if (needs_rg) {
			r_image->convert(Image::FORMAT_RGF);
		}
	} break;
	case Image::FORMAT_L8: {
		const bool uniform_rgb = (p_options[0] == p_options[1] && p_options[1] == p_options[2]) ||
								 !(remap_r || remap_g || remap_b);
		if (uniform_rgb) {
			// Uniform RGB.
			if (needs_rgba) {
				r_image->convert(Image::FORMAT_LA8);
			}
		}
		else {
			// Non-uniform RGB.
			if (needs_rgba) {
				r_image->convert(Image::FORMAT_RGBA8);
			}
			else {
				r_image->convert(Image::FORMAT_RGB8);
			}
			could_not_expand = true;
		}
	} break;
	case Image::FORMAT_LA8: {
		const bool uniform_rgb = (p_options[0] == p_options[1] && p_options[1] == p_options[2]) ||
								 !(remap_r || remap_g || remap_b);
		if (!uniform_rgb) {
			// Non-uniform RGB.
			r_image->convert(Image::FORMAT_RGBA8);
			could_not_expand = true;
		}
	} break;
	case Image::FORMAT_RGB565: {
		if (needs_rgba) {
			// RGB565 doesn't have an alpha expansion, convert to RGBA8.
			r_image->convert(Image::FORMAT_RGBA8);
			could_not_expand = true;
		}
	} break;
	case Image::FORMAT_RGBE9995: {
		if (needs_rgba) {
			// RGB9995 doesn't have an alpha expansion, convert to RGBAH.
			r_image->convert(Image::FORMAT_RGBAH);
			could_not_expand = true;
		}
	} break;

	default: {
	} break;
	}

	if (could_not_expand) {
		WARN_PRINT(vformat("Unable to expand image format %s's channels (the target format does "
						   "not exist), converting to %s as a fallback.",
			Image::get_format_name(original_format),
			Image::get_format_name(r_image->get_format())));
	}

	// Remap the channels.
	for (int x = 0; x < r_image->get_width(); x++) {
		for (int y = 0; y < r_image->get_height(); y++) {
			Color src = r_image->get_pixel(x, y);
			Color dst;

			for (int i = 0; i < 4; i++) {
				switch (p_options[i]) {
				case REMAP_R:
					dst[i] = src.r;
					break;
				case REMAP_G:
					dst[i] = src.g;
					break;
				case REMAP_B:
					dst[i] = src.b;
					break;
				case REMAP_A:
					dst[i] = src.a;
					break;

				case REMAP_INV_R:
					dst[i] = 1.0f - src.r;
					break;
				case REMAP_INV_G:
					dst[i] = 1.0f - src.g;
					break;
				case REMAP_INV_B:
					dst[i] = 1.0f - src.b;
					break;
				case REMAP_INV_A:
					dst[i] = 1.0f - src.a;
					break;

				case REMAP_0:
					dst[i] = 0.0f;
					break;
				case REMAP_1:
					dst[i] = 1.0f;
					break;

				default:
					break;
				}
			}

			r_image->set_pixel(x, y, dst);
		}
	}
}

void ResourceImporterTexture::_invert_y_channel(Ref<Image>& r_image)
{
	// Inverting the green channel can be used to flip a normal map's direction.
	// There's no standard when it comes to normal map Y direction, so this is
	// sometimes needed when using a normal map exported from another program.
	// See <http://wiki.polycount.com/wiki/Normal_Map_Technical_Details#Common_Swizzle_Coordinates>.
	const int height = r_image->get_height();
	const int width = r_image->get_width();

	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			const Color color = r_image->get_pixel(i, j);
			r_image->set_pixel(i, j, Color(color.r, 1 - color.g, color.b, color.a));
		}
	}
}

void ResourceImporterTexture::_clamp_hdr_exposure(Ref<Image>& r_image)
{
	// Clamp HDR exposure following Filament's tonemapping formula.
	// This can be used to reduce fireflies in environment maps or reduce the influence
	// of the sun from an HDRI panorama on environment lighting (when a DirectionalLight3D is used
	// instead).
	const int height = r_image->get_height();
	const int width = r_image->get_width();

	// These values are chosen arbitrarily and seem to produce good results with 4,096 samples.
	const float linear = 4096.0;
	const float compressed = 16384.0;

	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			const Color color = r_image->get_pixel(i, j);
			const float luma = color.get_luminance();

			Color clamped_color;
			if (luma <= linear) {
				clamped_color = color;
			}
			else {
				clamped_color = (color / luma) * ((linear * linear - compressed * luma) /
													 (2 * linear - compressed - luma));
			}

			r_image->set_pixel(i, j, clamped_color);
		}
	}
}

const char* ResourceImporterTexture::compression_formats[] = {"s3tc_bptc", "etc2_astc", nullptr};

ResourceImporterTexture* ResourceImporterTexture::singleton = nullptr;

ResourceImporterTexture::ResourceImporterTexture(bool p_singleton)
{
	// This should only be set through the EditorNode.
	if (p_singleton) {
		singleton = this;
	}

	CompressedTexture2D::request_3d_callback = _texture_reimport_3d;
	CompressedTexture2D::request_roughness_callback = _texture_reimport_roughness;
	CompressedTexture2D::request_normal_callback = _texture_reimport_normal;
}

ResourceImporterTexture::~ResourceImporterTexture()
{
	if (singleton == this) {
		singleton = nullptr;
	}
}


