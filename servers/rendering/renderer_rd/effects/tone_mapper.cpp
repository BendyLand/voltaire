/**************************************************************************/
/*  tone_mapper.cpp                                                       */
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

#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/renderer_rd/uniform_set_cache_rd.h"
#include "servers/rendering/rendering_device.h"
#include "tone_mapper.h"

using namespace RendererRD;

ToneMapper::~ToneMapper()
{
	if (using_mobile_version) {
		tonemap_mobile.shader.version_free(tonemap_mobile.shader_version);
	}
	else {
		tonemap.shader.version_free(tonemap.shader_version);
	}
}

void RendererRD::ToneMapper::tonemapper(
	RID p_source_color, RID p_dst_framebuffer, const TonemapSettings& p_settings)
{
}

void RendererRD::ToneMapper::tonemapper_subpass(long p_subpass, RID p_framebuffer,
	long p_render_pass, const RendererRD::ToneMapper::TonemapSettings& p_settings)
{
}

ToneMapper::ToneMapper(bool p_use_mobile_version)
{
	using_mobile_version = p_use_mobile_version;

	if (using_mobile_version) {
		Vector<String> modes;
		modes.push_back("\n");
		modes.push_back("\n#define USE_1D_LUT\n");
		modes.push_back("\n#define SUBPASS\n");
		modes.push_back("\n#define SUBPASS\n#define USE_1D_LUT\n");
		modes.push_back("\n#define USE_MULTIVIEW\n");
		modes.push_back("\n#define USE_1D_LUT\n#define USE_MULTIVIEW\n");
		modes.push_back("\n#define SUBPASS\n#define USE_MULTIVIEW\n");
		modes.push_back("\n#define SUBPASS\n#define USE_1D_LUT\n#define USE_MULTIVIEW\n");

		tonemap_mobile.shader.initialize(modes);
		tonemap_mobile.shader_version = tonemap_mobile.shader.version_create();
	}
	else {
		Vector<String> modes;
		modes.push_back("\n");
		modes.push_back("\n#define GLOW_BICUBIC_FILTER\n");
		modes.push_back("\n#define USE_1D_LUT\n");
		modes.push_back("\n#define GLOW_BICUBIC_FILTER\n#define USE_1D_LUT\n");
		modes.push_back("\n#define USE_MULTIVIEW\n");
		modes.push_back("\n#define GLOW_BICUBIC_FILTER\n#define USE_MULTIVIEW\n");
		modes.push_back("\n#define USE_1D_LUT\n#define USE_MULTIVIEW\n");
		modes.push_back(
			"\n#define GLOW_BICUBIC_FILTER\n#define USE_1D_LUT\n#define USE_MULTIVIEW\n");

		tonemap.shader.initialize(modes);
		tonemap.shader_version = tonemap.shader.version_create();
	}
}


