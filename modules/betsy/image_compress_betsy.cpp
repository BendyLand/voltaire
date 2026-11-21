/**************************************************************************/
/*  image_compress_betsy.cpp                                              */
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

#include "alpha_stitch.glsl.gen.h"
#include "register_types.h"
#include "bc1.glsl.gen.h"
#include "bc4.glsl.gen.h"
#include "bc6h.glsl.gen.h"
// --- Inlined from modules/betsy/betsy_bc1.h ---
static unsigned char stb__OMatch5[256][2] = {
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 1 },
	{ 0, 1 },
	{ 1, 0 },
	{ 1, 0 },
	{ 1, 0 },
	{ 1, 1 },
	{ 1, 1 },
	{ 2, 0 },
	{ 2, 0 },
	{ 0, 4 },
	{ 2, 1 },
	{ 2, 1 },
	{ 2, 1 },
	{ 3, 0 },
	{ 3, 0 },
	{ 3, 0 },
	{ 3, 1 },
	{ 1, 5 },
	{ 3, 2 },
	{ 3, 2 },
	{ 4, 0 },
	{ 4, 0 },
	{ 4, 1 },
	{ 4, 1 },
	{ 4, 2 },
	{ 4, 2 },
	{ 4, 2 },
	{ 3, 5 },
	{ 5, 1 },
	{ 5, 1 },
	{ 5, 2 },
	{ 4, 4 },
	{ 5, 3 },
	{ 5, 3 },
	{ 5, 3 },
	{ 6, 2 },
	{ 6, 2 },
	{ 6, 2 },
	{ 6, 3 },
	{ 5, 5 },
	{ 6, 4 },
	{ 6, 4 },
	{ 4, 8 },
	{ 7, 3 },
	{ 7, 3 },
	{ 7, 3 },
	{ 7, 4 },
	{ 7, 4 },
	{ 7, 4 },
	{ 7, 5 },
	{ 5, 9 },
	{ 7, 6 },
	{ 7, 6 },
	{ 8, 4 },
	{ 8, 4 },
	{ 8, 5 },
	{ 8, 5 },
	{ 8, 6 },
	{ 8, 6 },
	{ 8, 6 },
	{ 7, 9 },
	{ 9, 5 },
	{ 9, 5 },
	{ 9, 6 },
	{ 8, 8 },
	{ 9, 7 },
	{ 9, 7 },
	{ 9, 7 },
	{ 10, 6 },
	{ 10, 6 },
	{ 10, 6 },
	{ 10, 7 },
	{ 9, 9 },
	{ 10, 8 },
	{ 10, 8 },
	{ 8, 12 },
	{ 11, 7 },
	{ 11, 7 },
	{ 11, 7 },
	{ 11, 8 },
	{ 11, 8 },
	{ 11, 8 },
	{ 11, 9 },
	{ 9, 13 },
	{ 11, 10 },
	{ 11, 10 },
	{ 12, 8 },
	{ 12, 8 },
	{ 12, 9 },
	{ 12, 9 },
	{ 12, 10 },
	{ 12, 10 },
	{ 12, 10 },
	{ 11, 13 },
	{ 13, 9 },
	{ 13, 9 },
	{ 13, 10 },
	{ 12, 12 },
	{ 13, 11 },
	{ 13, 11 },
	{ 13, 11 },
	{ 14, 10 },
	{ 14, 10 },
	{ 14, 10 },
	{ 14, 11 },
	{ 13, 13 },
	{ 14, 12 },
	{ 14, 12 },
	{ 12, 16 },
	{ 15, 11 },
	{ 15, 11 },
	{ 15, 11 },
	{ 15, 12 },
	{ 15, 12 },
	{ 15, 12 },
	{ 15, 13 },
	{ 13, 17 },
	{ 15, 14 },
	{ 15, 14 },
	{ 16, 12 },
	{ 16, 12 },
	{ 16, 13 },
	{ 16, 13 },
	{ 16, 14 },
	{ 16, 14 },
	{ 16, 14 },
	{ 15, 17 },
	{ 17, 13 },
	{ 17, 13 },
	{ 17, 14 },
	{ 16, 16 },
	{ 17, 15 },
	{ 17, 15 },
	{ 17, 15 },
	{ 18, 14 },
	{ 18, 14 },
	{ 18, 14 },
	{ 18, 15 },
	{ 17, 17 },
	{ 18, 16 },
	{ 18, 16 },
	{ 16, 20 },
	{ 19, 15 },
	{ 19, 15 },
	{ 19, 15 },
	{ 19, 16 },
	{ 19, 16 },
	{ 19, 16 },
	{ 19, 17 },
	{ 17, 21 },
	{ 19, 18 },
	{ 19, 18 },
	{ 20, 16 },
	{ 20, 16 },
	{ 20, 17 },
	{ 20, 17 },
	{ 20, 18 },
	{ 20, 18 },
	{ 20, 18 },
	{ 19, 21 },
	{ 21, 17 },
	{ 21, 17 },
	{ 21, 18 },
	{ 20, 20 },
	{ 21, 19 },
	{ 21, 19 },
	{ 21, 19 },
	{ 22, 18 },
	{ 22, 18 },
	{ 22, 18 },
	{ 22, 19 },
	{ 21, 21 },
	{ 22, 20 },
	{ 22, 20 },
	{ 20, 24 },
	{ 23, 19 },
	{ 23, 19 },
	{ 23, 19 },
	{ 23, 20 },
	{ 23, 20 },
	{ 23, 20 },
	{ 23, 21 },
	{ 21, 25 },
	{ 23, 22 },
	{ 23, 22 },
	{ 24, 20 },
	{ 24, 20 },
	{ 24, 21 },
	{ 24, 21 },
	{ 24, 22 },
	{ 24, 22 },
	{ 24, 22 },
	{ 23, 25 },
	{ 25, 21 },
	{ 25, 21 },
	{ 25, 22 },
	{ 24, 24 },
	{ 25, 23 },
	{ 25, 23 },
	{ 25, 23 },
	{ 26, 22 },
	{ 26, 22 },
	{ 26, 22 },
	{ 26, 23 },
	{ 25, 25 },
	{ 26, 24 },
	{ 26, 24 },
	{ 24, 28 },
	{ 27, 23 },
	{ 27, 23 },
	{ 27, 23 },
	{ 27, 24 },
	{ 27, 24 },
	{ 27, 24 },
	{ 27, 25 },
	{ 25, 29 },
	{ 27, 26 },
	{ 27, 26 },
	{ 28, 24 },
	{ 28, 24 },
	{ 28, 25 },
	{ 28, 25 },
	{ 28, 26 },
	{ 28, 26 },
	{ 28, 26 },
	{ 27, 29 },
	{ 29, 25 },
	{ 29, 25 },
	{ 29, 26 },
	{ 28, 28 },
	{ 29, 27 },
	{ 29, 27 },
	{ 29, 27 },
	{ 30, 26 },
	{ 30, 26 },
	{ 30, 26 },
	{ 30, 27 },
	{ 29, 29 },
	{ 30, 28 },
	{ 30, 28 },
	{ 30, 28 },
	{ 31, 27 },
	{ 31, 27 },
	{ 31, 27 },
	{ 31, 28 },
	{ 31, 28 },
	{ 31, 28 },
	{ 31, 29 },
	{ 31, 29 },
	{ 31, 30 },
	{ 31, 30 },
	{ 31, 30 },
	{ 31, 31 },
	{ 31, 31 },
};

static unsigned char stb__OMatch6[256][2] = {
	{ 0, 0 },
	{ 0, 1 },
	{ 1, 0 },
	{ 1, 0 },
	{ 1, 1 },
	{ 2, 0 },
	{ 2, 1 },
	{ 3, 0 },
	{ 3, 0 },
	{ 3, 1 },
	{ 4, 0 },
	{ 4, 0 },
	{ 4, 1 },
	{ 5, 0 },
	{ 5, 1 },
	{ 6, 0 },
	{ 6, 0 },
	{ 6, 1 },
	{ 7, 0 },
	{ 7, 0 },
	{ 7, 1 },
	{ 8, 0 },
	{ 8, 1 },
	{ 8, 1 },
	{ 8, 2 },
	{ 9, 1 },
	{ 9, 2 },
	{ 9, 2 },
	{ 9, 3 },
	{ 10, 2 },
	{ 10, 3 },
	{ 10, 3 },
	{ 10, 4 },
	{ 11, 3 },
	{ 11, 4 },
	{ 11, 4 },
	{ 11, 5 },
	{ 12, 4 },
	{ 12, 5 },
	{ 12, 5 },
	{ 12, 6 },
	{ 13, 5 },
	{ 13, 6 },
	{ 8, 16 },
	{ 13, 7 },
	{ 14, 6 },
	{ 14, 7 },
	{ 9, 17 },
	{ 14, 8 },
	{ 15, 7 },
	{ 15, 8 },
	{ 11, 16 },
	{ 15, 9 },
	{ 15, 10 },
	{ 16, 8 },
	{ 16, 9 },
	{ 16, 10 },
	{ 15, 13 },
	{ 17, 9 },
	{ 17, 10 },
	{ 17, 11 },
	{ 15, 16 },
	{ 18, 10 },
	{ 18, 11 },
	{ 18, 12 },
	{ 16, 16 },
	{ 19, 11 },
	{ 19, 12 },
	{ 19, 13 },
	{ 17, 17 },
	{ 20, 12 },
	{ 20, 13 },
	{ 20, 14 },
	{ 19, 16 },
	{ 21, 13 },
	{ 21, 14 },
	{ 21, 15 },
	{ 20, 17 },
	{ 22, 14 },
	{ 22, 15 },
	{ 25, 10 },
	{ 22, 16 },
	{ 23, 15 },
	{ 23, 16 },
	{ 26, 11 },
	{ 23, 17 },
	{ 24, 16 },
	{ 24, 17 },
	{ 27, 12 },
	{ 24, 18 },
	{ 25, 17 },
	{ 25, 18 },
	{ 28, 13 },
	{ 25, 19 },
	{ 26, 18 },
	{ 26, 19 },
	{ 29, 14 },
	{ 26, 20 },
	{ 27, 19 },
	{ 27, 20 },
	{ 30, 15 },
	{ 27, 21 },
	{ 28, 20 },
	{ 28, 21 },
	{ 28, 21 },
	{ 28, 22 },
	{ 29, 21 },
	{ 29, 22 },
	{ 24, 32 },
	{ 29, 23 },
	{ 30, 22 },
	{ 30, 23 },
	{ 25, 33 },
	{ 30, 24 },
	{ 31, 23 },
	{ 31, 24 },
	{ 27, 32 },
	{ 31, 25 },
	{ 31, 26 },
	{ 32, 24 },
	{ 32, 25 },
	{ 32, 26 },
	{ 31, 29 },
	{ 33, 25 },
	{ 33, 26 },
	{ 33, 27 },
	{ 31, 32 },
	{ 34, 26 },
	{ 34, 27 },
	{ 34, 28 },
	{ 32, 32 },
	{ 35, 27 },
	{ 35, 28 },
	{ 35, 29 },
	{ 33, 33 },
	{ 36, 28 },
	{ 36, 29 },
	{ 36, 30 },
	{ 35, 32 },
	{ 37, 29 },
	{ 37, 30 },
	{ 37, 31 },
	{ 36, 33 },
	{ 38, 30 },
	{ 38, 31 },
	{ 41, 26 },
	{ 38, 32 },
	{ 39, 31 },
	{ 39, 32 },
	{ 42, 27 },
	{ 39, 33 },
	{ 40, 32 },
	{ 40, 33 },
	{ 43, 28 },
	{ 40, 34 },
	{ 41, 33 },
	{ 41, 34 },
	{ 44, 29 },
	{ 41, 35 },
	{ 42, 34 },
	{ 42, 35 },
	{ 45, 30 },
	{ 42, 36 },
	{ 43, 35 },
	{ 43, 36 },
	{ 46, 31 },
	{ 43, 37 },
	{ 44, 36 },
	{ 44, 37 },
	{ 44, 37 },
	{ 44, 38 },
	{ 45, 37 },
	{ 45, 38 },
	{ 40, 48 },
	{ 45, 39 },
	{ 46, 38 },
	{ 46, 39 },
	{ 41, 49 },
	{ 46, 40 },
	{ 47, 39 },
	{ 47, 40 },
	{ 43, 48 },
	{ 47, 41 },
	{ 47, 42 },
	{ 48, 40 },
	{ 48, 41 },
	{ 48, 42 },
	{ 47, 45 },
	{ 49, 41 },
	{ 49, 42 },
	{ 49, 43 },
	{ 47, 48 },
	{ 50, 42 },
	{ 50, 43 },
	{ 50, 44 },
	{ 48, 48 },
	{ 51, 43 },
	{ 51, 44 },
	{ 51, 45 },
	{ 49, 49 },
	{ 52, 44 },
	{ 52, 45 },
	{ 52, 46 },
	{ 51, 48 },
	{ 53, 45 },
	{ 53, 46 },
	{ 53, 47 },
	{ 52, 49 },
	{ 54, 46 },
	{ 54, 47 },
	{ 57, 42 },
	{ 54, 48 },
	{ 55, 47 },
	{ 55, 48 },
	{ 58, 43 },
	{ 55, 49 },
	{ 56, 48 },
	{ 56, 49 },
	{ 59, 44 },
	{ 56, 50 },
	{ 57, 49 },
	{ 57, 50 },
	{ 60, 45 },
	{ 57, 51 },
	{ 58, 50 },
	{ 58, 51 },
	{ 61, 46 },
	{ 58, 52 },
	{ 59, 51 },
	{ 59, 52 },
	{ 62, 47 },
	{ 59, 53 },
	{ 60, 52 },
	{ 60, 53 },
	{ 60, 53 },
	{ 60, 54 },
	{ 61, 53 },
	{ 61, 54 },
	{ 61, 54 },
	{ 61, 55 },
	{ 62, 54 },
	{ 62, 55 },
	{ 62, 55 },
	{ 62, 56 },
	{ 63, 55 },
	{ 63, 56 },
	{ 63, 56 },
	{ 63, 57 },
	{ 63, 58 },
	{ 63, 59 },
	{ 63, 59 },
	{ 63, 60 },
	{ 63, 61 },
	{ 63, 62 },
	{ 63, 62 },
	{ 63, 63 },
};

// --- End modules/betsy/betsy_bc1.h ---
#include "core/config/project_settings.h"
#include "core/os/os.h"
// --- Inlined from modules/betsy/image_compress_betsy.h ---
#include "core/io/image.h"
#include "core/templates/command_queue_mt.h"

class RDShaderFile;
class RenderingDevice;
class RenderingContextDriver;

enum BetsyFormat
{
	BETSY_FORMAT_BC1,
	BETSY_FORMAT_BC1_DITHER,
	BETSY_FORMAT_BC3,
	BETSY_FORMAT_BC4_SIGNED,
	BETSY_FORMAT_BC4_UNSIGNED,
	BETSY_FORMAT_BC5_SIGNED,
	BETSY_FORMAT_BC5_UNSIGNED,
	BETSY_FORMAT_BC6_SIGNED,
	BETSY_FORMAT_BC6_UNSIGNED,
	BETSY_FORMAT_MAX,
};

enum BetsyShaderType
{
	BETSY_SHADER_BC1_STANDARD,
	BETSY_SHADER_BC1_DITHER,
	BETSY_SHADER_BC4_SIGNED,
	BETSY_SHADER_BC4_UNSIGNED,
	BETSY_SHADER_BC6_SIGNED,
	BETSY_SHADER_BC6_UNSIGNED,
	BETSY_SHADER_ALPHA_STITCH,
	BETSY_SHADER_RGB_TO_RGBA_FLOAT,
	BETSY_SHADER_RGB_TO_RGBA_HALF,
	BETSY_SHADER_RGB_TO_RGBA_UNORM8,
	BETSY_SHADER_RGB_TO_RGBA_UNORM16,
	BETSY_SHADER_MAX,
};

struct BC6PushConstant
{
	float sizeX;
	float sizeY;
	uint32_t padding[2] = {0};
};

struct BC1PushConstant
{
	uint32_t num_refines;
	uint32_t padding[3] = {0};
};

struct BC4PushConstant
{
	uint32_t channel_idx;
	uint32_t padding[3] = {0};
};

struct RGBToRGBAPushConstant
{
	uint32_t width;
	uint32_t height;
	uint32_t padding[2];
};

void free_device();

Error _betsy_compress_bptc(
	Image* r_img, Image::UsedChannels p_channels, Image::BPTCFormat p_bptc_format);
Error _betsy_compress_s3tc(Image* r_img, Image::UsedChannels p_channels);

class BetsyCompressor
{
	mutable CommandQueueMT command_queue;
	bool exit = false;

	struct BetsyShader
	{
		RID compiled;
		RID pipeline;
	};

	// Resources shared by all compression formats.
	RenderingContextDriver* compress_rcd = nullptr;
	BetsyShader cached_shaders[BETSY_SHADER_MAX];
	RID src_sampler;

	// Format-specific resources.
	RID dxt1_encoding_table_buffer;

	void _init();
	void _thread_loop();
	void _thread_exit();

	Error _get_shader(BetsyFormat p_format, const String& p_version, BetsyShader& r_shader);
	Error _compress(BetsyFormat p_format, Image* r_img);

public:
	void init();
	void finish();

	Error compress(BetsyFormat p_format, Image* r_img)
	{
		Error err;
		command_queue.push_and_ret(this, &BetsyCompressor::_compress, &err, p_format, r_img);
		return err;
	}
};



// --- End modules/betsy/image_compress_betsy.h ---
#include "rgb_to_rgba.glsl.gen.h"
#include "servers/display/display_server.h"
#include "servers/rendering/rendering_context_driver.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/rendering_device_binds.h"
#include "servers/rendering/renderer.h"

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

static const RDC::DataFormat BETSY_TO_RD_FORMAT[BETSY_FORMAT_MAX] = {
	RDC::DATA_FORMAT_R32G32_UINT,
	RDC::DATA_FORMAT_R32G32_UINT,
	RDC::DATA_FORMAT_R32G32_UINT,
	RDC::DATA_FORMAT_R32G32_UINT,
	RDC::DATA_FORMAT_R32G32_UINT,
	RDC::DATA_FORMAT_R32G32_UINT,
	RDC::DATA_FORMAT_R32G32_UINT,
	RDC::DATA_FORMAT_R32G32B32A32_UINT,
	RDC::DATA_FORMAT_R32G32B32A32_UINT,
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

void BetsyCompressor::init() {}

void BetsyCompressor::finish() {}

Error BetsyCompressor::_compress(BetsyFormat p_format, Image* r_img) { return OK; }

void BetsyCompressor::_thread_exit()
{
	exit = true;

	if (RD::data != nullptr) {
		if (dxt1_encoding_table_buffer.is_valid()) {
			RD::free_rid(dxt1_encoding_table_buffer);
		}

		RD::free_rid(src_sampler);

		// Clear the shader cache, pipelines will be unreferenced automatically.
		for (int i = 0; i < BETSY_SHADER_MAX; i++) {
			if (cached_shaders[i].compiled.is_valid()) {
				RD::free_rid(cached_shaders[i].compiled);
			}
		}

		// Free the RD (and RCD if necessary).
		memdelete(RD::data);
		RD::data = nullptr;
		if (compress_rcd != nullptr) {
			memdelete(compress_rcd);
			compress_rcd = nullptr;
		}
	}
}

// Helper functions.

static int get_next_multiple(int n, int m) { return n + (m - (n % m)); }

static Error get_src_texture_format(Image* r_img, RDC::DataFormat& r_format, bool& r_is_rgb)
{
	r_is_rgb = false;

	switch (r_img->get_format()) {
	case Image::FORMAT_L8:
		r_img->convert(Image::FORMAT_RGBA8);
		r_format = RDC::DATA_FORMAT_R8G8B8A8_UNORM;
		break;
	case Image::FORMAT_LA8:
		r_img->convert(Image::FORMAT_RGBA8);
		r_format = RDC::DATA_FORMAT_R8G8B8A8_UNORM;
		break;
	case Image::FORMAT_R8:
		r_format = RDC::DATA_FORMAT_R8_UNORM;
		break;
	case Image::FORMAT_RG8:
		r_format = RDC::DATA_FORMAT_R8G8_UNORM;
		break;
	case Image::FORMAT_RGB8:
		r_is_rgb = true;
		r_format = RDC::DATA_FORMAT_R8G8B8A8_UNORM;
		break;
	case Image::FORMAT_RGBA8:
		r_format = RDC::DATA_FORMAT_R8G8B8A8_UNORM;
		break;
	case Image::FORMAT_RH:
		r_format = RDC::DATA_FORMAT_R16_SFLOAT;
		break;
	case Image::FORMAT_RGH:
		r_format = RDC::DATA_FORMAT_R16G16_SFLOAT;
		break;
	case Image::FORMAT_RGBH:
		r_is_rgb = true;
		r_format = RDC::DATA_FORMAT_R16G16B16A16_SFLOAT;
		break;
	case Image::FORMAT_RGBAH:
		r_format = RDC::DATA_FORMAT_R16G16B16A16_SFLOAT;
		break;
	case Image::FORMAT_RF:
		r_format = RDC::DATA_FORMAT_R32_SFLOAT;
		break;
	case Image::FORMAT_RGF:
		r_format = RDC::DATA_FORMAT_R32G32_SFLOAT;
		break;
	case Image::FORMAT_RGBF:
		r_is_rgb = true;
		r_format = RDC::DATA_FORMAT_R32G32B32A32_SFLOAT;
		break;
	case Image::FORMAT_RGBAF:
		r_format = RDC::DATA_FORMAT_R32G32B32A32_SFLOAT;
		break;
	case Image::FORMAT_RGBE9995:
		r_format = RDC::DATA_FORMAT_E5B9G9R9_UFLOAT_PACK32;
		break;
	case Image::FORMAT_R16:
		r_format = RDC::DATA_FORMAT_R16_UNORM;
		break;
	case Image::FORMAT_RG16:
		r_format = RDC::DATA_FORMAT_R16G16_UNORM;
		break;
	case Image::FORMAT_RGB16:
		r_is_rgb = true;
		r_format = RDC::DATA_FORMAT_R16G16B16A16_UNORM;
		break;
	case Image::FORMAT_RGBA16:
		r_format = RDC::DATA_FORMAT_R16G16B16A16_UNORM;
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

void initialize_betsy_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	Image::_image_compress_bptc_rd_func = _betsy_compress_bptc;
	Image::_image_compress_bc_rd_func = _betsy_compress_s3tc;
}

void uninitialize_betsy_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	free_device();
}
