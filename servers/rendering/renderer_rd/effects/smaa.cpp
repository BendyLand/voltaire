/**************************************************************************/
/*  smaa.cpp                                                              */
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
#include "servers/rendering/renderer_rd/effects/smaa_area_tex.gen.h"
#include "servers/rendering/renderer_rd/effects/smaa_search_tex.gen.h"
#include "servers/rendering/renderer_rd/framebuffer_cache_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/render_scene_buffers_rd.h"
#include "servers/rendering/renderer_rd/uniform_set_cache_rd.h"
#include "servers/rendering/rendering_device.h"
#include "smaa.h"

using namespace RendererRD;

SMAA::~SMAA()
{
	RD::free_rid(smaa.search_tex);
	RD::free_rid(smaa.area_tex);

	smaa.edge_shader.version_free(smaa.edge_shader_version);
	smaa.weight_shader.version_free(smaa.weight_shader_version);
	smaa.blend_shader.version_free(smaa.blend_shader_version);
}

void SMAA::allocate_render_targets(Ref<RenderSceneBuffersRD> p_render_buffers)
{
	RSE::ViewportScaling3DType scaling_type =
		RSE::scaling_3d_mode_type(p_render_buffers->get_scaling_3d_mode());
	bool use_upscaled_texture = p_render_buffers->has_upscaled_texture() &&
								scaling_type == RSE::VIEWPORT_SCALING_3D_TYPE_TEMPORAL;
	Size2i full_size = use_upscaled_texture ? p_render_buffers->get_target_size()
											: p_render_buffers->get_internal_size();

	// As we're not clearing these, and render buffers will return the cached texture if it already
	// exists, we don't first check has_texture here.

	p_render_buffers->create_texture(RB_SCOPE_SMAA, RB_EDGES, RDC::DATA_FORMAT_R8G8_UNORM,
		RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT,
		RDC::TEXTURE_SAMPLES_1, full_size, 1, 1, true, true);
	p_render_buffers->create_texture(RB_SCOPE_SMAA, RB_BLEND, RDC::DATA_FORMAT_R8G8B8A8_UNORM,
		RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT,
		RDC::TEXTURE_SAMPLES_1, full_size, 1, 1, true, true);
	p_render_buffers->create_texture(RB_SCOPE_SMAA, RB_STENCIL, smaa.stencil_format,
		RDC::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, RDC::TEXTURE_SAMPLES_1, full_size, 1, 1,
		true, true);
}

void RendererRD::SMAA::process(Ref<RenderSceneBuffersRD> p_render_buffers, RID p_dest_framebuffer,
	RID p_source_color, bool p_use_srgb)
{
}


SMAA::SMAA() {
	edge_detection_threshold = 0.1f;

	// 1. Shaders initialization
	Vector<String> edge_modes;
	edge_modes.push_back("\n#define COLOR_EDGE_DETECTION\n");
	edge_modes.push_back("\n#define LUMA_EDGE_DETECTION\n");

	smaa.edge_shader.initialize(edge_modes);
	smaa.edge_shader_version = smaa.edge_shader.version_create();

	Vector<String> empty_modes;
	empty_modes.push_back("\n");

	smaa.weight_shader.initialize(empty_modes);
	smaa.weight_shader_version = smaa.weight_shader.version_create();

	Vector<String> blend_modes;
	blend_modes.push_back("\n");
	blend_modes.push_back("\n#define USE_8_BIT_DEBANDING\n");
	blend_modes.push_back("\n#define USE_10_BIT_DEBANDING\n");

	smaa.blend_shader.initialize(blend_modes);
	smaa.blend_shader_version = smaa.blend_shader.version_create();

	// 2. Load precomputed search texture from embedded PNG
	{
		Vector<uint8_t> png_data;
		png_data.resize(sizeof(search_tex_png));
		memcpy(png_data.ptrw(), search_tex_png, sizeof(search_tex_png));

		Ref<Image> img = memnew(Image);
		img->load_png_from_buffer(png_data);

		RDC::TextureFormat tf;
		tf.format = RDC::DATA_FORMAT_R8_UNORM;
		tf.width = img->get_width();
		tf.height = img->get_height();
		tf.usage_bits = RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_CAN_UPDATE_BIT;

		Vector<Vector<uint8_t>> data_slices;
		data_slices.push_back(img->get_data());

		smaa.search_tex = RD::texture_create(tf, RDD::TextureView(), data_slices);
	}

	// 3. Load precomputed area texture from embedded PNG
	{
		Vector<uint8_t> png_data;
		png_data.resize(sizeof(area_tex_png));
		memcpy(png_data.ptrw(), area_tex_png, sizeof(area_tex_png));

		Ref<Image> img = memnew(Image);
		img->load_png_from_buffer(png_data);

		RDC::TextureFormat tf;
		tf.format = RDC::DATA_FORMAT_R8G8_UNORM;
		tf.width = img->get_width();
		tf.height = img->get_height();
		tf.usage_bits = RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_CAN_UPDATE_BIT;

		Vector<Vector<uint8_t>> data_slices;
		data_slices.push_back(img->get_data());

		smaa.area_tex = RD::texture_create(tf, RDD::TextureView(), data_slices);
	}
}


