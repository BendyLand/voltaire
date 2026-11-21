/**************************************************************************/
/*  post_effects.cpp                                                      */
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

#include "post_effects.h"
#include "core/config/project_settings.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/renderer_rd/effects/copy_effects.h"
#include "servers/rendering/renderer_rd/effects/smaa_area_tex.gen.h"
#include "servers/rendering/renderer_rd/effects/smaa_search_tex.gen.h"
#include "servers/rendering/renderer_rd/framebuffer_cache_rd.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/light_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/render_scene_buffers_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/texture_storage.h"
#include "servers/rendering/renderer_rd/uniform_set_cache_rd.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/storage/camera_attributes_storage.h"

#ifndef XR_DISABLED
#include "servers/xr/xr_interface.h"
#include "servers/xr/xr_server.h"
#endif // XR_DISABLED

using namespace RendererRD;

/* BokehDOF */

BokehDOF::~BokehDOF()
{
	for (int i = 0; i < BOKEH_MAX; i++) {
		bokeh.compute_pipelines[i].free();
	}

	if (prefer_raster_effects) {
		bokeh.raster_shader.version_free(bokeh.shader_version);
	}
	else {
		bokeh.compute_shader.version_free(bokeh.shader_version);
	}
}

void RendererRD::BokehDOF::bokeh_dof_compute(const RendererRD::BokehDOF::BokehBuffers& p_buffers,
	RID p_dest_framebuffer, float p_bokeh_size, float p_blur_size, bool p_use_jitter)
{
}

BokehDOF::BokehDOF(bool p_prefer_raster_effects)
{
	prefer_raster_effects = p_prefer_raster_effects;

	Vector<String> modes;
	modes.push_back("\n#define MODE_GEN_BLUR_SIZE\n");
	modes.push_back("\n#define MODE_BOKEH_BOX\n");
	modes.push_back("\n#define MODE_BOKEH_BOX\n#define MODE_NO_WEIGHTS\n");
	modes.push_back("\n#define MODE_BOKEH_HEXAGONAL\n");
	modes.push_back("\n#define MODE_BOKEH_HEXAGONAL\n#define MODE_NO_WEIGHTS\n");
	modes.push_back("\n#define MODE_BOKEH_CIRCULAR\n");
	modes.push_back("\n#define MODE_COMPOSITE\n");

	if (prefer_raster_effects) {
		bokeh.raster_shader.initialize(modes);
		bokeh.shader_version = bokeh.raster_shader.version_create();
	}
	else {
		bokeh.compute_shader.initialize(modes);
		bokeh.shader_version = bokeh.compute_shader.version_create();
	}
}

/* ToneMapper */

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

/* Luminance */

Luminance::~Luminance()
{
	if (prefer_raster_effects) {
		luminance_reduce_raster.shader.version_free(luminance_reduce_raster.shader_version);
	}
	else {
		luminance_reduce.shader.version_free(luminance_reduce.shader_version);
	}
}

void Luminance::LuminanceBuffers::set_prefer_raster_effects(bool p_prefer_raster_effects)
{
	prefer_raster_effects = p_prefer_raster_effects;
}

void Luminance::LuminanceBuffers::configure(RenderSceneBuffersRD* p_render_buffers)
{
	Size2i internal_size = p_render_buffers->get_internal_size();
	int w = internal_size.x;
	int h = internal_size.y;

	while (true) {
		w = MAX(w / 8, 1);
		h = MAX(h / 8, 1);

		RDC::TextureFormat tf;
		tf.format = RDC::DATA_FORMAT_R32_SFLOAT;
		tf.width = w;
		tf.height = h;

		bool final = w == 1 && h == 1;

		if (prefer_raster_effects) {
			tf.usage_bits =
				RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT | RDC::TEXTURE_USAGE_SAMPLING_BIT;
		}
		else {
			tf.usage_bits = RDC::TEXTURE_USAGE_STORAGE_BIT;
		}

		if (final) {
			tf.usage_bits |= RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_CAN_COPY_TO_BIT;
		}

		RID texture = RD::texture_create(tf, RDD::TextureView());
		reduce.push_back(texture);

		if (final) {
			current = RD::texture_create(tf, RDD::TextureView());
			RD::texture_clear(current, Color(0.0, 0.0, 0.0), 0u, 1u, 0u, 1u);
			break;
		}
	}
}

void Luminance::LuminanceBuffers::free_data()
{
	for (int i = 0; i < reduce.size(); i++) {
		RD::free_rid(reduce[i]);
	}
	reduce.clear();

	if (current.is_valid()) {
		RD::free_rid(current);
		current = RID();
	}
}

Ref<Luminance::LuminanceBuffers> Luminance::get_luminance_buffers(
	Ref<RenderSceneBuffersRD> p_render_buffers)
{
	if (p_render_buffers->has_custom_data(RB_LUMINANCE_BUFFERS)) {
		return p_render_buffers->get_custom_data(RB_LUMINANCE_BUFFERS);
	}

	Ref<LuminanceBuffers> buffers;
	buffers.instantiate();
	buffers->set_prefer_raster_effects(prefer_raster_effects);
	buffers->configure(p_render_buffers.ptr());

	p_render_buffers->set_custom_data(RB_LUMINANCE_BUFFERS, buffers);

	return buffers;
}

RID Luminance::get_current_luminance_buffer(Ref<RenderSceneBuffersRD> p_render_buffers)
{
	if (p_render_buffers->has_custom_data(RB_LUMINANCE_BUFFERS)) {
		Ref<LuminanceBuffers> buffers = p_render_buffers->get_custom_data(RB_LUMINANCE_BUFFERS);
		return buffers->current;
	}

	return RID();
}

void RendererRD::Luminance::luminance_reduction(RID p_source_texture, Vector2i p_size,
	Ref<RendererRD::Luminance::LuminanceBuffers> p_buffers, float p_min_luminance,
	float p_max_luminance, float p_frame_time, bool p_immediate)
{
}

Luminance::Luminance(bool p_prefer_raster_effects)
{
	prefer_raster_effects = p_prefer_raster_effects;
	if (prefer_raster_effects) {
		Vector<String> modes;
		modes.push_back("\n#define READ_FIRST\n");
		modes.push_back("\n");
		modes.push_back("\n#define WRITE_FINAL\n");

		luminance_reduce_raster.shader.initialize(modes);
		luminance_reduce_raster.shader_version = luminance_reduce_raster.shader.version_create();
	}
	else {
		Vector<String> modes;
		modes.push_back("\n#define READ_FIRST\n");
		modes.push_back("\n");
		modes.push_back("\n#define WRITE_FINAL\n");

		luminance_reduce.shader.initialize(modes);
		luminance_reduce.shader_version = luminance_reduce.shader.version_create();

		for (int i = 0; i < LUMINANCE_REDUCE_MAX; i++) {
			luminance_reduce.pipelines[i] = RD::compute_pipeline_create(
				luminance_reduce.shader.version_get_shader(luminance_reduce.shader_version, i));
		}
	}
}

/* SMAA */

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

SMAA::SMAA()
{
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

/* DebugEffects */

void DebugEffects::_create_frustum_arrays()
{
	if (frustum.vertex_buffer.is_null()) {
		// Create vertex buffer, but don't put data in it yet
		frustum.vertex_buffer = RD::vertex_buffer_create(8 * sizeof(float) * 3, Vector<uint8_t>());

		Vector<RDC::VertexAttribute> attributes;
		Vector<RID> buffers;
		RDC::VertexAttribute vd;

		vd.location = 0;
		vd.stride = sizeof(float) * 3;
		vd.format = RDC::DATA_FORMAT_R32G32B32_SFLOAT;

		attributes.push_back(vd);
		buffers.push_back(frustum.vertex_buffer);

		frustum.vertex_format = RD::vertex_format_create(attributes);
		frustum.vertex_array = RD::vertex_array_create(8, frustum.vertex_format, buffers);
	}

	if (frustum.index_buffer.is_null()) {
		uint16_t indices[6 * 2 * 3] = {
			// Far
			0, 1, 2, // FLT, FLB, FRT
			1, 3, 2, // FLB, FRB, FRT
			// Near
			4, 6, 5, // NLT, NRT, NLB
			6, 7, 5, // NRT, NRB, NLB
			// Left
			0, 4, 1, // FLT, NLT, FLB
			4, 5, 1, // NLT, NLB, FLB
			// Right
			6, 2, 7, // NRT, FRT, NRB
			2, 3, 7, // FRT, FRB, NRB
			// Top
			0, 2, 4, // FLT, FRT, NLT
			2, 6, 4, // FRT, NRT, NLT
			// Bottom
			5, 7, 1, // NLB, NRB, FLB,
			7, 3, 1	 // NRB, FRB, FLB
		};

		// Create our index_array
		PackedByteArray data;
		data.resize(6 * 2 * 3 * 2);
		{
			uint8_t* w = data.ptrw();
			uint16_t* p16 = (uint16_t*)w;
			for (int i = 0; i < 6 * 2 * 3; i++) {
				*p16 = indices[i];
				p16++;
			}
		}
		frustum.index_buffer =
			RD::index_buffer_create(6 * 2 * 3, RDC::INDEX_BUFFER_FORMAT_UINT16, data);
		frustum.index_array = RD::index_array_create(frustum.index_buffer, 0, 6 * 2 * 3);
	}

	if (frustum.lines_buffer.is_null()) {
		uint16_t indices[12 * 2] = {
			0, 1, // FLT - FLB
			1, 3, // FLB - FRB
			3, 2, // FRB - FRT
			2, 0, // FRT - FLT

			4, 6, // NLT - NRT
			6, 7, // NRT - NRB
			7, 5, // NRB - NLB
			5, 4, // NLB - NLT

			0, 4, // FLT - NLT
			1, 5, // FLB - NLB
			2, 6, // FRT - NRT
			3, 7  // FRB - NRB
		};

		// Create our lines_array
		PackedByteArray data;
		data.resize(12 * 2 * 2);
		{
			uint8_t* w = data.ptrw();
			uint16_t* p16 = (uint16_t*)w;
			for (int i = 0; i < 12 * 2; i++) {
				*p16 = indices[i];
				p16++;
			}
		}
		frustum.lines_buffer =
			RD::index_buffer_create(12 * 2, RDC::INDEX_BUFFER_FORMAT_UINT16, data);
		frustum.lines_array = RD::index_array_create(frustum.lines_buffer, 0, 12 * 2);
	}
}

DebugEffects::DebugEffects()
{
	// 1. Initialize frustum geometry buffers
	_create_frustum_arrays();

	// 2. Shadow frustum shader & version
	Vector<String> shadow_frustum_modes;
	shadow_frustum_modes.push_back("\n#define MODE_TRANSPARENT\n");
	shadow_frustum_modes.push_back("\n#define MODE_WIREFRAME\n");

	shadow_frustum.shader.initialize(shadow_frustum_modes);
	shadow_frustum.shader_version = shadow_frustum.shader.version_create();

	// 3. Motion vectors shader & version
	Vector<String> motion_vectors_modes;
	motion_vectors_modes.push_back("\n");

	motion_vectors.shader.initialize(motion_vectors_modes);
	motion_vectors.shader_version = motion_vectors.shader.version_create();
}

DebugEffects::~DebugEffects()
{
	shadow_frustum.shader.version_free(shadow_frustum.shader_version);

	// Destroy vertex buffer and array.
	if (frustum.vertex_buffer.is_valid()) {
		RD::free_rid(frustum.vertex_buffer); // Array gets freed as dependency.
	}

	// Destroy index buffer and array,
	if (frustum.index_buffer.is_valid()) {
		RD::free_rid(frustum.index_buffer); // Array gets freed as dependency.
	}

	// Destroy lines buffer and array.
	if (frustum.lines_buffer.is_valid()) {
		RD::free_rid(frustum.lines_buffer); // Array gets freed as dependency.
	}

	motion_vectors.shader.version_free(motion_vectors.shader_version);
}

void RendererRD::DebugEffects::draw_shadow_frustum(RID p_dest_framebuffer,
	const Projection& p_projection, const Transform3D& p_transform, RID p_depth, Rect2 p_rect)
{
}

/* VRS */

VRS::~VRS() { vrs_shader.shader.version_free(vrs_shader.shader_version); }

Size2i VRS::get_vrs_texture_size(const Size2i p_base_size) const
{
	Size2i vrs_texel_size = RD::vrs_get_texel_size();
	return Size2i((p_base_size.x + vrs_texel_size.x - 1) / vrs_texel_size.x,
		(p_base_size.y + vrs_texel_size.y - 1) / vrs_texel_size.y);
}

void VRS::update_vrs_texture(RID p_vrs_fb, RID p_render_target)
{
	RSE::ViewportVRSMode vrs_mode =
		RendererRD::TextureStorage::render_target_get_vrs_mode(p_render_target);
	RSE::ViewportVRSUpdateMode vrs_update_mode =
		RendererRD::TextureStorage::render_target_get_vrs_update_mode(p_render_target);

	if (vrs_mode != RSE::VIEWPORT_VRS_DISABLED &&
		vrs_update_mode != RSE::VIEWPORT_VRS_UPDATE_DISABLED) {
		RD::draw_command_begin_label("VRS Setup");

		if (vrs_mode == RSE::VIEWPORT_VRS_TEXTURE) {
			RID vrs_texture =
				RendererRD::TextureStorage::render_target_get_vrs_texture(p_render_target);
			if (vrs_texture.is_valid()) {
				RID rd_texture = RendererRD::TextureStorage::texture_get_rd_texture(vrs_texture);
				int layers = RendererRD::TextureStorage::texture_get_layers(vrs_texture);
				if (rd_texture.is_valid()) {
					// Copy into our density buffer
					copy_vrs(rd_texture, p_vrs_fb, layers > 1);
				}
			}
#ifndef XR_DISABLED
		}
		else if (vrs_mode == RSE::VIEWPORT_VRS_XR) {
			Ref<XRInterface> interface = XRServer::get_primary_interface();
			if (interface.is_valid() &&
				interface->get_vrs_texture_format() == XRInterface::XR_VRS_TEXTURE_FORMAT_UNIFIED) {
				RID vrs_texture = interface->get_vrs_texture();
				if (vrs_texture.is_valid()) {
					RID rd_texture =
						RendererRD::TextureStorage::texture_get_rd_texture(vrs_texture);
					int layers = RendererRD::TextureStorage::texture_get_layers(vrs_texture);

					if (rd_texture.is_valid()) {
						// Copy into our density buffer
						copy_vrs(rd_texture, p_vrs_fb, layers > 1);
					}
				}
			}
#endif // XR_DISABLED
		}

		if (vrs_update_mode == RSE::VIEWPORT_VRS_UPDATE_ONCE) {
			RendererRD::TextureStorage::render_target_set_vrs_update_mode(
				p_render_target, RSE::VIEWPORT_VRS_UPDATE_DISABLED);
		}

		RD::draw_command_end_label();
	}
}

void RendererRD::VRS::copy_vrs(RID p_source, RID p_dest, bool p_is_mobile) {}

VRS::VRS()
{
	Vector<String> modes;
	modes.push_back("\n");
	vrs_shader.shader.initialize(modes);
	vrs_shader.shader_version = vrs_shader.shader.version_create();
}
