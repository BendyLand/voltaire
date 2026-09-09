/**************************************************************************/
/*  image_compress_betsy.cpp                                              */
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

#include "alpha_stitch.glsl.gen.h"
#include "bc1.glsl.gen.h"
#include "bc4.glsl.gen.h"
#include "bc6h.glsl.gen.h"
#include "betsy_bc1.h"
#include "core/config/project_settings.h"
#include "core/os/os.h"
#include "image_compress_betsy.h"
#include "rgb_to_rgba.glsl.gen.h"
#include "servers/display/display_server.h"
#include "servers/rendering/rendering_context_driver.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/rendering_device_binds.h"
#include "servers/rendering/rendering_server.h"

#if defined(VULKAN_ENABLED)
#include "drivers/vulkan/rendering_context_driver_vulkan.h"
#endif
#if defined(METAL_ENABLED)
#include "drivers/metal/rendering_context_driver_metal.h"
#endif

static Mutex betsy_mutex;
static BetsyCompressor* betsy = nullptr;

static const BetsyShaderType FORMAT_TO_TYPE[BETSY_FORMAT_MAX] = {
	BETSY_SHADER_BC1_STANDARD,
	BETSY_SHADER_BC1_DITHER,
	BETSY_SHADER_BC1_STANDARD,
	BETSY_SHADER_BC4_SIGNED,
	BETSY_SHADER_BC4_UNSIGNED,
	BETSY_SHADER_BC4_SIGNED,
	BETSY_SHADER_BC4_UNSIGNED,
	BETSY_SHADER_BC6_SIGNED,
	BETSY_SHADER_BC6_UNSIGNED,
};

static const RD::DataFormat BETSY_TO_RD_FORMAT[BETSY_FORMAT_MAX] = {
	RD::DATA_FORMAT_R32G32_UINT,
	RD::DATA_FORMAT_R32G32_UINT,
	RD::DATA_FORMAT_R32G32_UINT,
	RD::DATA_FORMAT_R32G32_UINT,
	RD::DATA_FORMAT_R32G32_UINT,
	RD::DATA_FORMAT_R32G32_UINT,
	RD::DATA_FORMAT_R32G32_UINT,
	RD::DATA_FORMAT_R32G32B32A32_UINT,
	RD::DATA_FORMAT_R32G32B32A32_UINT,
};

static const Image::Format BETSY_TO_IMAGE_FORMAT[BETSY_FORMAT_MAX] = {
	Image::FORMAT_DXT1,
	Image::FORMAT_DXT1,
	Image::FORMAT_DXT5,
	Image::FORMAT_RGTC_R,
	Image::FORMAT_RGTC_R,
	Image::FORMAT_RGTC_RG,
	Image::FORMAT_RGTC_RG,
	Image::FORMAT_BPTC_RGBF,
	Image::FORMAT_BPTC_RGBFU,
};

void BetsyCompressor::_thread_exit()
{
	exit = true;

	if (compress_rd != nullptr) {
		if (dxt1_encoding_table_buffer.is_valid()) {
			compress_rd->free_rid(dxt1_encoding_table_buffer);
		}

		compress_rd->free_rid(src_sampler);

		// Clear the shader cache, pipelines will be unreferenced automatically.
		for (int i = 0; i < BETSY_SHADER_MAX; i++) {
			if (cached_shaders[i].compiled.is_valid()) {
				compress_rd->free_rid(cached_shaders[i].compiled);
			}
		}

		// Free the RD (and RCD if necessary).
		memdelete(compress_rd);
		compress_rd = nullptr;
		if (compress_rcd != nullptr) {
			memdelete(compress_rcd);
			compress_rcd = nullptr;
		}
	}
}

// Helper functions.

static int get_next_multiple(int n, int m) { return n + (m - (n % m)); }

static Error get_src_texture_format(Image* r_img, RD::DataFormat& r_format, bool& r_is_rgb)
{
	r_is_rgb = false;

	switch (r_img->get_format()) {
	case Image::FORMAT_L8:
		r_img->convert(Image::FORMAT_RGBA8);
		r_format = RD::DATA_FORMAT_R8G8B8A8_UNORM;
		break;

	case Image::FORMAT_LA8:
		r_img->convert(Image::FORMAT_RGBA8);
		r_format = RD::DATA_FORMAT_R8G8B8A8_UNORM;
		break;

	case Image::FORMAT_R8:
		r_format = RD::DATA_FORMAT_R8_UNORM;
		break;

	case Image::FORMAT_RG8:
		r_format = RD::DATA_FORMAT_R8G8_UNORM;
		break;

	case Image::FORMAT_RGB8:
		r_is_rgb = true;
		r_format = RD::DATA_FORMAT_R8G8B8A8_UNORM;
		break;

	case Image::FORMAT_RGBA8:
		r_format = RD::DATA_FORMAT_R8G8B8A8_UNORM;
		break;

	case Image::FORMAT_RH:
		r_format = RD::DATA_FORMAT_R16_SFLOAT;
		break;

	case Image::FORMAT_RGH:
		r_format = RD::DATA_FORMAT_R16G16_SFLOAT;
		break;

	case Image::FORMAT_RGBH:
		r_is_rgb = true;
		r_format = RD::DATA_FORMAT_R16G16B16A16_SFLOAT;
		break;

	case Image::FORMAT_RGBAH:
		r_format = RD::DATA_FORMAT_R16G16B16A16_SFLOAT;
		break;

	case Image::FORMAT_RF:
		r_format = RD::DATA_FORMAT_R32_SFLOAT;
		break;

	case Image::FORMAT_RGF:
		r_format = RD::DATA_FORMAT_R32G32_SFLOAT;
		break;

	case Image::FORMAT_RGBF:
		r_is_rgb = true;
		r_format = RD::DATA_FORMAT_R32G32B32A32_SFLOAT;
		break;

	case Image::FORMAT_RGBAF:
		r_format = RD::DATA_FORMAT_R32G32B32A32_SFLOAT;
		break;

	case Image::FORMAT_RGBE9995:
		r_format = RD::DATA_FORMAT_E5B9G9R9_UFLOAT_PACK32;
		break;

	case Image::FORMAT_R16:
		r_format = RD::DATA_FORMAT_R16_UNORM;
		break;

	case Image::FORMAT_RG16:
		r_format = RD::DATA_FORMAT_R16G16_UNORM;
		break;

	case Image::FORMAT_RGB16:
		r_is_rgb = true;
		r_format = RD::DATA_FORMAT_R16G16B16A16_UNORM;
		break;

	case Image::FORMAT_RGBA16:
		r_format = RD::DATA_FORMAT_R16G16B16A16_UNORM;
		break;

	default: {
		return ERR_UNAVAILABLE;
	}
	}

	return OK;
}

void ensure_betsy_exists()
{
	betsy_mutex.lock();
	if (betsy == nullptr) {
		betsy = memnew(BetsyCompressor);
		betsy->init();
	}
	betsy_mutex.unlock();
}

Error _betsy_compress_bptc(
	Image* r_img, Image::UsedChannels p_channels, Image::BPTCFormat p_bptc_format)
{
	ensure_betsy_exists();
	Image::Format format = r_img->get_format();
	Error result = ERR_UNAVAILABLE;

	if (format >= Image::FORMAT_RF && format <= Image::FORMAT_RGBE9995) {
		if ((p_bptc_format == Image::BPTC_DETECT && r_img->detect_signed()) ||
			p_bptc_format == Image::BPTC_FORCE_SIGNED) {
			result = betsy->compress(BETSY_FORMAT_BC6_SIGNED, r_img);
		}
		else {
			result = betsy->compress(BETSY_FORMAT_BC6_UNSIGNED, r_img);
		}
	}

	return result;
}

Error _betsy_compress_s3tc(Image* r_img, Image::UsedChannels p_channels)
{
	ensure_betsy_exists();
	Error result = ERR_UNAVAILABLE;

	switch (p_channels) {
	case Image::USED_CHANNELS_RGB:
	case Image::USED_CHANNELS_L:
		result = betsy->compress(BETSY_FORMAT_BC1, r_img);
		break;

	case Image::USED_CHANNELS_RGBA:
	case Image::USED_CHANNELS_LA:
		result = betsy->compress(BETSY_FORMAT_BC3, r_img);
		break;

	case Image::USED_CHANNELS_R:
		result = betsy->compress(BETSY_FORMAT_BC4_UNSIGNED, r_img);
		break;

	case Image::USED_CHANNELS_RG:
		result = betsy->compress(BETSY_FORMAT_BC5_UNSIGNED, r_img);
		break;

	default:
		break;
	}

	return result;
}

void free_device()
{
	if (betsy != nullptr) {
		betsy->finish();
		memdelete(betsy);
	}
}


