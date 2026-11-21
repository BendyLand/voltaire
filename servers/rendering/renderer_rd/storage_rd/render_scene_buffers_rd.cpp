/**************************************************************************/
/*  render_scene_buffers_rd.cpp                                           */
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

#include "servers/rendering/renderer_rd/environment/rd_environment.h"
#include "servers/rendering/renderer_rd/storage_rd/render_scene_buffers_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/texture_storage.h"
#include "servers/rendering/rendering_device_binds.h"
#include "servers/rendering/renderer.h" // IWYU pragma: keep // Needed to bind RSE enums.
#include "servers/rendering/rendering_server_enums.h"
#include "servers/rendering/renderer_rd/storage_rd/render_scene_data_rd.h"
#include "servers/rendering/renderer_rd/renderer_scene_render_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/light_storage.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/renderer_rd/storage_rd/mesh_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/particles_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/utilities.h"

// --- Merged from servers/rendering/renderer_rd/storage_rd/render_scene_buffers_rd.cpp ---

RenderSceneBuffersRD::RenderSceneBuffersRD() {}

RenderSceneBuffersRD::~RenderSceneBuffersRD()
{
	cleanup();

	data_buffers.clear();

	RendererRD::MaterialStorage::samplers_rd_free(samplers);
}

void RenderSceneBuffersRD::update_sizes(NamedTexture& p_named_texture)
{
	ERR_FAIL_COND(p_named_texture.texture.is_null());

	p_named_texture.sizes.resize(p_named_texture.format.mipmaps);

	Size2i mipmap_size = Size2i(p_named_texture.format.width, p_named_texture.format.height);
	for (uint32_t mipmap = 0; mipmap < p_named_texture.format.mipmaps; mipmap++) {
		p_named_texture.sizes.ptrw()[mipmap] = mipmap_size;

		mipmap_size = Size2i(mipmap_size.width >> 1, mipmap_size.height >> 1).maxi(1);
	}
}

void RenderSceneBuffersRD::free_named_texture(NamedTexture& p_named_texture)
{
	if (p_named_texture.texture.is_valid()) {
		RD::free_rid(p_named_texture.texture);
	}
	p_named_texture.texture = RID();
	p_named_texture.slices.clear(); // slices should be freed automatically as dependents...
}

void RenderSceneBuffersRD::update_samplers()
{
	float computed_mipmap_bias = texture_mipmap_bias;

	if (use_taa ||
		(RSE::scaling_3d_mode_type(scaling_3d_mode) == RSE::VIEWPORT_SCALING_3D_TYPE_TEMPORAL)) {
		// Use negative mipmap LOD bias when TAA or FSR2 is enabled to compensate for loss of
		// sharpness. This restores sharpness in still images to be roughly at the same level as
		// without TAA, but moving scenes will still be blurrier.
		computed_mipmap_bias -= 0.5;
	}

	if (screen_space_aa == RSE::VIEWPORT_SCREEN_SPACE_AA_FXAA) {
		// Use negative mipmap LOD bias when FXAA is enabled to compensate for loss of sharpness.
		// If both TAA and FXAA are enabled, combine their negative LOD biases together.
		computed_mipmap_bias -= 0.25;
	}

	RendererRD::MaterialStorage::samplers_rd_free(samplers);
	samplers =
		RendererRD::MaterialStorage::samplers_rd_allocate(computed_mipmap_bias, anisotropic_filtering_level);
}

void RenderSceneBuffersRD::cleanup()
{
	// Free our data buffers (but don't destroy them)
	for (KeyValue<String, CustomDataEntry>& E : data_buffers) {
		E.value.free_fn(E.value.data.ptr());
	}

	// Clear our named textures
	for (KeyValue<NTKey, NamedTexture>& E : named_textures) {
		free_named_texture(E.value);
	}
	named_textures.clear();

	// Clear weight_buffer / blur textures.
	for (WeightBuffers& weight_buffer : weight_buffers) {
		if (weight_buffer.weight.is_valid()) {
			RD::free_rid(weight_buffer.weight);
			weight_buffer.weight = RID();
		}
	}

#ifdef METAL_ENABLED
	if (mfx_spatial_context) {
		memdelete(mfx_spatial_context);
		mfx_spatial_context = nullptr;
	}
#endif
}

void RenderSceneBuffersRD::configure(const RenderSceneBuffersConfiguration* p_config)
{
	render_target = p_config->get_render_target();
	force_hdr = RendererRD::TextureStorage::render_target_is_using_hdr(render_target);

	target_size = p_config->get_target_size();
	internal_size = p_config->get_internal_size();
	view_count = p_config->get_view_count();

	scaling_3d_mode = p_config->get_scaling_3d_mode();
	msaa_3d = p_config->get_msaa_3d();
	screen_space_aa = p_config->get_screen_space_aa();

	fsr_sharpness = p_config->get_fsr_sharpness();
	texture_mipmap_bias = p_config->get_texture_mipmap_bias();
	anisotropic_filtering_level = p_config->get_anisotropic_filtering_level();
	use_taa = p_config->get_use_taa();
	use_debanding = p_config->get_use_debanding();

	ERR_FAIL_COND_MSG(view_count == 0, "Must have at least 1 view");

	vrs_mode = RendererRD::TextureStorage::render_target_get_vrs_mode(render_target);

	update_samplers();

	// cleanout any old buffers we had.
	cleanup();

	// Create our color buffer.
	const bool resolve_target = msaa_3d != RSE::VIEWPORT_MSAA_DISABLED;
	create_texture(RB_SCOPE_BUFFERS, RB_TEX_COLOR, get_base_data_format(),
		get_color_usage_bits(resolve_target, false, can_be_storage));

	// TODO: Detect when it is safe to use RD::TEXTURE_USAGE_TRANSIENT_BIT for RB_TEX_DEPTH,
	// RB_TEX_COLOR_MSAA and/or RB_TEX_DEPTH_MSAA. (it means we cannot sample from it, we cannot
	// copy from/to it) to save VRAM (and maybe performance too).

	// Create our depth buffer.
	create_texture(RB_SCOPE_BUFFERS, RB_TEX_DEPTH,
		get_depth_format(resolve_target, false, can_be_storage),
		get_depth_usage_bits(resolve_target, false, can_be_storage));

	// Create our MSAA buffers.
	if (msaa_3d == RSE::VIEWPORT_MSAA_DISABLED) {
		texture_samples = RDC::TEXTURE_SAMPLES_1;
	}
	else {
		texture_samples = msaa_to_samples(msaa_3d);
		create_texture(RB_SCOPE_BUFFERS, RB_TEX_COLOR_MSAA, get_base_data_format(),
			get_color_usage_bits(false, true, can_be_storage), texture_samples, Size2i(), 0, 1,
			true, true);
		create_texture(RB_SCOPE_BUFFERS, RB_TEX_DEPTH_MSAA,
			get_depth_format(false, true, can_be_storage),
			get_depth_usage_bits(false, true, can_be_storage), texture_samples, Size2i(), 0, 1,
			true, true);
	}

	// VRS (note, our vrs object will only be set if VRS is supported)
	RID vrs_texture;
	if (vrs && vrs_mode != RSE::VIEWPORT_VRS_DISABLED) {
		vrs_texture = create_texture(RB_SCOPE_VRS, RB_TEXTURE, get_vrs_format(),
			get_vrs_usage_bits(), RDC::TEXTURE_SAMPLES_1, vrs->get_vrs_texture_size(internal_size));
	}

	// (re-)configure any named buffers
	for (KeyValue<String, CustomDataEntry>& E : data_buffers) {
		E.value.configure_fn(E.value.data.ptr(), this);
	}
}

void RenderSceneBuffersRD::configure_for_reflections(const Size2i p_reflection_size)
{
	// For now our render buffers for reflections are only used for effects/environment
	// (Sky/Fog/Etc) Possibly at some point move our entire reflection atlas buffer management into
	// this class

	target_size = p_reflection_size;
	internal_size = p_reflection_size;
	render_target = RID();
	scaling_3d_mode = RSE::VIEWPORT_SCALING_3D_MODE_OFF;
	fsr_sharpness = 0.0;
	msaa_3d = RSE::VIEWPORT_MSAA_DISABLED;
	screen_space_aa = RSE::VIEWPORT_SCREEN_SPACE_AA_DISABLED;
	use_taa = false;
	use_debanding = false;
	view_count = 1;

	// cleanout any old buffers we had.
	cleanup();

	// (re-)configure any named buffers
	for (KeyValue<String, CustomDataEntry>& E : data_buffers) {
		E.value.configure_fn(E.value.data.ptr(), this);
	}
}

void RenderSceneBuffersRD::set_fsr_sharpness(float p_fsr_sharpness)
{
	fsr_sharpness = p_fsr_sharpness;
}

void RenderSceneBuffersRD::set_texture_mipmap_bias(float p_texture_mipmap_bias)
{
	texture_mipmap_bias = p_texture_mipmap_bias;

	update_samplers();
}

void RenderSceneBuffersRD::set_anisotropic_filtering_level(
	RSE::ViewportAnisotropicFiltering p_anisotropic_filtering_level)
{
	anisotropic_filtering_level = p_anisotropic_filtering_level;

	update_samplers();
}

void RenderSceneBuffersRD::set_use_debanding(bool p_use_debanding)
{
	use_debanding = p_use_debanding;
}

#ifdef METAL_ENABLED
void RenderSceneBuffersRD::ensure_mfx(RendererRD::MFXSpatialEffect* p_effect)
{
	if (mfx_spatial_context) {
		return;
	}

	ERR_FAIL_COND(!RendererRD::TextureStorage::is_initialized());
	ERR_FAIL_NULL(RD::data);

	// Determine the output format of the render target.
	RID dest = RendererRD::TextureStorage::render_target_get_rd_texture(render_target);
	RD::TextureFormat tf = RD::texture_get_format(dest);
	RD::DataFormat output_format = tf.format;

	RendererRD::MFXSpatialEffect::CreateParams params = {
		.input_size = internal_size,
		.output_size = target_size,
		.input_format = get_base_data_format(),
		.output_format = output_format,
	};

	mfx_spatial_context = p_effect->create_context(params);
}
#endif

bool RenderSceneBuffersRD::has_texture(
	const String& p_context, const String& p_texture_name) const
{
	NTKey key(p_context, p_texture_name);

	return named_textures.has(key);
}

RID RenderSceneBuffersRD::create_texture(const String& p_context,
	const String& p_texture_name, const RDC::DataFormat p_data_format,
	const uint32_t p_usage_bits, const RDC::TextureSamples p_texture_samples, const Size2i p_size,
	const uint32_t p_layers, const uint32_t p_mipmaps, bool p_unique, bool p_discardable)
{
	// Keep some useful data, we use default values when these are 0.
	Size2i size = p_size == Size2i(0, 0) ? internal_size : p_size;
	uint32_t layers = p_layers == 0 ? view_count : p_layers;
	uint32_t mipmaps = p_mipmaps == 0 ? 1 : p_mipmaps;

	// Create our texture
	RDC::TextureFormat tf;
	tf.format = p_data_format;
	if (layers > 1) {
		tf.texture_type = RDC::TEXTURE_TYPE_2D_ARRAY;
	}

	tf.width = size.x;
	tf.height = size.y;
	tf.depth = 1;
	tf.array_layers = layers;
	tf.mipmaps = mipmaps;
	tf.usage_bits = p_usage_bits;
	tf.samples = p_texture_samples;
	tf.is_discardable = p_discardable;

	return create_texture_from_format(p_context, p_texture_name, tf, RD::TextureView(), p_unique);
}

RID RenderSceneBuffersRD::_create_texture_from_format(const String& p_context,
	const String& p_texture_name, const Ref<RDTextureFormat>& p_texture_format,
	const Ref<RDTextureView>& p_view, bool p_unique)
{
	ERR_FAIL_COND_V(p_texture_format.is_null(), RID());

	RD::TextureView texture_view;
	if (p_view.is_valid()) { // only use when supplied, else default.
		texture_view = p_view->base;
	}

	return create_texture_from_format(
		p_context, p_texture_name, p_texture_format->base, texture_view, p_unique);
}

RID RenderSceneBuffersRD::_create_texture_view(const String& p_context,
	const String& p_texture_name, const String& p_view_name,
	const Ref<RDTextureView> p_view)
{
	RD::TextureView texture_view;
	if (p_view.is_valid()) { // only use when supplied, else default.
		texture_view = p_view->base;
	}

	return create_texture_view(p_context, p_texture_name, p_view_name, texture_view);
}

RID RenderSceneBuffersRD::get_texture(
	const String& p_context, const String& p_texture_name) const
{
	NTKey key(p_context, p_texture_name);

	ERR_FAIL_COND_V(!named_textures.has(key), RID());

	return named_textures[key].texture;
}

Ref<RDTextureFormat> RenderSceneBuffersRD::_get_texture_format(
	const String& p_context, const String& p_texture_name) const
{
	Ref<RDTextureFormat> tf;
	tf.instantiate();

	tf->base = get_texture_format(p_context, p_texture_name);

	return tf;
}

RID RenderSceneBuffersRD::_get_texture_slice_view(const String& p_context,
	const String& p_texture_name, const uint32_t p_layer, const uint32_t p_mipmap,
	const uint32_t p_layers, const uint32_t p_mipmaps, const Ref<RDTextureView> p_view)
{
	RD::TextureView texture_view;
	if (p_view.is_valid()) {
		texture_view = p_view->base;
	}

	return get_texture_slice_view(
		p_context, p_texture_name, p_layer, p_mipmap, p_layers, p_mipmaps, texture_view);
}

const RDC::TextureFormat RenderSceneBuffersRD::get_texture_format(
	const String& p_context, const String& p_texture_name) const
{
	NTKey key(p_context, p_texture_name);

	ERR_FAIL_COND_V(!named_textures.has(key), RDC::TextureFormat());

	return named_textures[key].format;
}

RID RenderSceneBuffersRD::get_texture_slice_view(const String& p_context,
	const String& p_texture_name, const uint32_t p_layer, const uint32_t p_mipmap,
	const uint32_t p_layers, const uint32_t p_mipmaps, RD::TextureView p_view)
{
	return RID();
}

RID RenderSceneBuffersRD::get_texture_slice(const String& p_context,
	const String& p_texture_name, const uint32_t p_layer, const uint32_t p_mipmap,
	const uint32_t p_layers, const uint32_t p_mipmaps)
{
	return get_texture_slice_view(
		p_context, p_texture_name, p_layer, p_mipmap, p_layers, p_mipmaps, RD::TextureView());
}

Size2i RenderSceneBuffersRD::get_texture_slice_size(
	const String& p_context, const String& p_texture_name, const uint32_t p_mipmap)
{
	NTKey key(p_context, p_texture_name);

	// check if this is a known texture
	ERR_FAIL_COND_V(!named_textures.has(key), Size2i());
	NamedTexture& named_texture = named_textures[key];
	ERR_FAIL_COND_V(named_texture.texture.is_null(), Size2i());

	// check if we're in bounds
	ERR_FAIL_UNSIGNED_INDEX_V(p_mipmap, named_texture.format.mipmaps, Size2i());

	// return our size
	return named_texture.sizes[p_mipmap];
}

void RenderSceneBuffersRD::clear_context(const String& p_context)
{
	Vector<NTKey> to_free; // free these

	// Find all entries for our context, we don't want to free them yet or our loop fails.
	for (KeyValue<NTKey, NamedTexture>& E : named_textures) {
		if (E.key.context == p_context) {
			to_free.push_back(E.key);
		}
	}

	// Now free these and remove them from our textures
	for (NTKey& key : to_free) {
		free_named_texture(named_textures[key]);
		named_textures.erase(key);
	}
}

void RenderSceneBuffersRD::allocate_blur_textures()
{
	if (has_texture(RB_SCOPE_BUFFERS, RB_TEX_BLUR_0)) {
		// already allocated...
		return;
	}

	Size2i blur_size = internal_size;
	if (RSE::scaling_3d_mode_type(scaling_3d_mode) == RSE::VIEWPORT_SCALING_3D_TYPE_TEMPORAL) {
		// The blur texture should be as big as the target size when using an upscaler.
		blur_size = target_size;
	}

	uint32_t mipmaps_required =
		Image::get_image_required_mipmaps(blur_size.x, blur_size.y, Image::FORMAT_RGBAH);

	uint32_t usage_bits = RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_CAN_COPY_TO_BIT;
	if (can_be_storage) {
		usage_bits += RDC::TEXTURE_USAGE_STORAGE_BIT;
	}
	else {
		usage_bits += RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT;
	}

	create_texture(RB_SCOPE_BUFFERS, RB_TEX_BLUR_0, get_base_data_format(), usage_bits,
		RDC::TEXTURE_SAMPLES_1, blur_size, view_count, mipmaps_required);
	create_texture(RB_SCOPE_BUFFERS, RB_TEX_BLUR_1, get_base_data_format(), usage_bits,
		RDC::TEXTURE_SAMPLES_1, Size2i(blur_size.x >> 1, blur_size.y >> 1), view_count,
		mipmaps_required - 1);

	// TODO redo this:
	if (!can_be_storage) {
		// create 4 weight textures, 2 full size, 2 half size

		RDC::TextureFormat tf;
		tf.format = RDC::DATA_FORMAT_R16_SFLOAT; // We could probably use DATA_FORMAT_R8_SNORM if we
												 // don't pre-multiply by blur_size but that depends
												 // on whether we can remove DEPTH_GAP
		tf.width = blur_size.x;
		tf.height = blur_size.y;
		tf.texture_type = RDC::TEXTURE_TYPE_2D;
		tf.array_layers = 1; // Our DOF effect handles one eye per turn
		tf.usage_bits = RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_CAN_COPY_TO_BIT |
						RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT;
		tf.mipmaps = 1;
		for (uint32_t i = 0; i < 4; i++) {
			// associated blur texture
			RID texture;
			if (i == 1) {
				texture = get_texture_slice(RB_SCOPE_BUFFERS, RB_TEX_BLUR_0, 0, 0);
			}
			else if (i == 2) {
				texture = get_texture_slice(RB_SCOPE_BUFFERS, RB_TEX_BLUR_1, 0, 0);
			}
			else if (i == 3) {
				texture = get_texture_slice(RB_SCOPE_BUFFERS, RB_TEX_BLUR_0, 0, 1);
			}

			// create weight texture
			weight_buffers[i].weight = RD::texture_create(tf, RDD::TextureView());

			// create frame buffer
			Vector<RID> fb;
			if (i != 0) {
				fb.push_back(texture);
			}
			fb.push_back(weight_buffers[i].weight);
			weight_buffers[i].fb = RD::framebuffer_create(fb);

			if (i == 1) {
				// next 2 are half size
				tf.width = MAX(1u, tf.width >> 1);
				tf.height = MAX(1u, tf.height >> 1);
			}
		}
	}
}

bool RenderSceneBuffersRD::has_custom_data(const String& p_name) const
{
	return data_buffers.has(p_name);
}

Ref<RefCounted> RenderSceneBuffersRD::get_custom_data(const String& p_name) const
{
	ERR_FAIL_COND_V(!data_buffers.has(p_name), Ref<RefCounted>());

	return data_buffers[p_name].data;
}

bool RenderSceneBuffersRD::has_depth_texture()
{
	if (render_target.is_null()) {
		// not applicable when there is no render target (likely this is for a reflection probe)
		return false;
	}

	RID depth = RendererRD::TextureStorage::render_target_get_override_depth(render_target);
	if (depth.is_valid()) {
		return true;
	}
	else {
		return has_texture(RB_SCOPE_BUFFERS, RB_TEX_DEPTH);
	}
}

RID RenderSceneBuffersRD::get_depth_texture()
{
	if (render_target.is_null()) {
		// not applicable when there is no render target (likely this is for a reflection probe)
		return RID();
	}

	RID depth = RendererRD::TextureStorage::render_target_get_override_depth(render_target);
	if (depth.is_valid()) {
		return depth;
	}
	else {
		return get_texture(RB_SCOPE_BUFFERS, RB_TEX_DEPTH);
	}
}

RID RenderSceneBuffersRD::get_depth_texture(const uint32_t p_layer)
{
	RID depth_slice =
		RendererRD::TextureStorage::render_target_get_override_depth_slice(render_target, p_layer);
	if (depth_slice.is_valid()) {
		return depth_slice;
	}
	else {
		return get_texture_slice(RB_SCOPE_BUFFERS, RB_TEX_DEPTH, p_layer, 0);
	}
}

RID RenderSceneBuffersRD::get_color_subsampled()
{
	const bool use_msaa = (msaa_3d != RSE::VIEWPORT_MSAA_DISABLED);

	RDC::TextureFormat tf;
	tf.format = get_base_data_format();
	if (view_count > 1) {
		tf.texture_type = RDC::TEXTURE_TYPE_2D_ARRAY;
	}
	tf.width = internal_size.x;
	tf.height = internal_size.y;
	tf.array_layers = view_count;
	tf.usage_bits = get_color_usage_bits(use_msaa, false, can_be_storage);
	tf.is_subsampled = true;

	return create_texture_from_format(RB_SCOPE_BUFFERS, RB_TEX_COLOR_SUBSAMPLED, tf);
}

RID RenderSceneBuffersRD::get_color_msaa_subsampled()
{
	ERR_FAIL_COND_V(msaa_3d == RSE::VIEWPORT_MSAA_DISABLED, RID());

	RDC::TextureFormat tf;
	tf.format = get_base_data_format();
	if (view_count > 1) {
		tf.texture_type = RDC::TEXTURE_TYPE_2D_ARRAY;
	}
	tf.width = internal_size.x;
	tf.height = internal_size.y;
	tf.array_layers = view_count;
	tf.samples = msaa_to_samples(msaa_3d);
	tf.usage_bits = get_color_usage_bits(false, true, can_be_storage);
	tf.is_discardable = true;
	tf.is_subsampled = true;

	return create_texture_from_format(RB_SCOPE_BUFFERS, RB_TEX_COLOR_MSAA_SUBSAMPLED, tf);
}

RID RenderSceneBuffersRD::get_depth_subsampled()
{
	const bool use_msaa = (msaa_3d != RSE::VIEWPORT_MSAA_DISABLED);

	RDC::TextureFormat tf;
	tf.format = get_depth_format(use_msaa, false, can_be_storage);
	if (view_count > 1) {
		tf.texture_type = RDC::TEXTURE_TYPE_2D_ARRAY;
	}
	tf.width = internal_size.x;
	tf.height = internal_size.y;
	tf.array_layers = view_count;
	tf.usage_bits = get_depth_usage_bits(use_msaa, false, can_be_storage);
	tf.is_subsampled = true;

	return create_texture_from_format(RB_SCOPE_BUFFERS, RB_TEX_DEPTH_SUBSAMPLED, tf);
}

RID RenderSceneBuffersRD::get_depth_msaa_subsampled()
{
	ERR_FAIL_COND_V(msaa_3d == RSE::VIEWPORT_MSAA_DISABLED, RID());

	RDC::TextureFormat tf;
	tf.format = get_depth_format(false, true, can_be_storage);
	if (view_count > 1) {
		tf.texture_type = RDC::TEXTURE_TYPE_2D_ARRAY;
	}
	tf.width = internal_size.x;
	tf.height = internal_size.y;
	tf.array_layers = view_count;
	tf.samples = msaa_to_samples(msaa_3d);
	tf.usage_bits = get_depth_usage_bits(false, true, can_be_storage);
	tf.is_discardable = true;
	tf.is_subsampled = true;

	return create_texture_from_format(RB_SCOPE_BUFFERS, RB_TEX_DEPTH_MSAA_SUBSAMPLED, tf);
}

void RenderSceneBuffersRD::ensure_upscaled()
{
	if (!has_upscaled_texture()) {
		uint32_t usage_bits = RDC::TEXTURE_USAGE_SAMPLING_BIT |
							  (can_be_storage ? RDC::TEXTURE_USAGE_STORAGE_BIT : 0) |
							  RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT;
		usage_bits |= RDC::TEXTURE_USAGE_INPUT_ATTACHMENT_BIT;
		create_texture(RB_SCOPE_BUFFERS, RB_TEX_COLOR_UPSCALED, get_base_data_format(), usage_bits,
			RDC::TEXTURE_SAMPLES_1, target_size);
	}
}

void RenderSceneBuffersRD::ensure_velocity()
{
	if (!has_texture(RB_SCOPE_BUFFERS, RB_TEX_VELOCITY)) {
		const bool msaa = msaa_3d != RSE::VIEWPORT_MSAA_DISABLED;
		create_texture(RB_SCOPE_BUFFERS, RB_TEX_VELOCITY, get_velocity_format(),
			get_velocity_usage_bits(msaa, false, can_be_storage));

		if (msaa) {
			create_texture(RB_SCOPE_BUFFERS, RB_TEX_VELOCITY_MSAA, get_velocity_format(),
				get_velocity_usage_bits(false, msaa, can_be_storage), texture_samples, Size2i(), 0,
				1, true, true);
		}
	}
}

bool RenderSceneBuffersRD::has_velocity_buffer(bool p_has_msaa)
{
	if (p_has_msaa) {
		return has_texture(RB_SCOPE_BUFFERS, RB_TEX_VELOCITY_MSAA);
	}
	else {
		RID velocity = RendererRD::TextureStorage::render_target_get_override_velocity(render_target);
		if (velocity.is_valid()) {
			return true;
		}
		else {
			return has_texture(RB_SCOPE_BUFFERS, RB_TEX_VELOCITY);
		}
	}
}

RID RenderSceneBuffersRD::get_velocity_buffer(bool p_get_msaa)
{
	if (p_get_msaa) {
		if (!has_texture(RB_SCOPE_BUFFERS, RB_TEX_VELOCITY_MSAA)) {
			return RID();
		}
		else {
			return get_texture(RB_SCOPE_BUFFERS, RB_TEX_VELOCITY_MSAA);
		}
	}
	else {
		RID velocity = RendererRD::TextureStorage::render_target_get_override_velocity(render_target);
		if (velocity.is_valid()) {
			return velocity;
		}
		else if (!has_texture(RB_SCOPE_BUFFERS, RB_TEX_VELOCITY)) {
			return RID();
		}
		else {
			return get_texture(RB_SCOPE_BUFFERS, RB_TEX_VELOCITY);
		}
	}
}

RID RenderSceneBuffersRD::get_velocity_buffer(bool p_get_msaa, uint32_t p_layer)
{
	if (p_get_msaa) {
		return get_texture_slice(RB_SCOPE_BUFFERS, RB_TEX_VELOCITY_MSAA, p_layer, 0);
	}
	else {
		RID velocity_slice =
			RendererRD::TextureStorage::render_target_get_override_velocity_slice(render_target, p_layer);
		if (velocity_slice.is_valid()) {
			return velocity_slice;
		}
		else {
			return get_texture_slice(RB_SCOPE_BUFFERS, RB_TEX_VELOCITY, p_layer, 0);
		}
	}
}

RID RenderSceneBuffersRD::get_velocity_depth_buffer()
{
	RID velocity_depth = RendererRD::TextureStorage::render_target_get_override_velocity_depth(render_target);
	return velocity_depth;
}

uint32_t RenderSceneBuffersRD::get_color_usage_bits(bool p_resolve, bool p_msaa, bool p_storage)
{
	DEV_ASSERT((!p_resolve && !p_msaa) || (p_resolve != p_msaa));

	uint32_t usage_bits = RDC::TEXTURE_USAGE_SAMPLING_BIT |
						  RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT |
						  RDC::TEXTURE_USAGE_INPUT_ATTACHMENT_BIT;
	if (p_msaa) {
		usage_bits |= RDC::TEXTURE_USAGE_CAN_COPY_FROM_BIT;
	}
	else if (p_resolve) {
		usage_bits |=
			RDC::TEXTURE_USAGE_CAN_COPY_TO_BIT | (p_storage ? RDC::TEXTURE_USAGE_STORAGE_BIT : 0);
	}
	else {
		usage_bits |= (p_storage ? RDC::TEXTURE_USAGE_STORAGE_BIT : 0);
	}

	return usage_bits;
}

RDC::DataFormat RenderSceneBuffersRD::get_depth_format(bool p_resolve, bool p_msaa, bool p_storage)
{
	if (p_resolve &&
		(p_storage || !RenderingDevice::has_feature(RDC::SUPPORTS_FRAMEBUFFER_DEPTH_RESOLVE))) {
		// Use R32 for resolve on Forward+ (p_storage == true), or if we don't support depth
		// resolve.
		return RDC::DATA_FORMAT_R32_SFLOAT;
	}
	else {
		const RenderingDeviceCommons::DataFormat preferred_formats[2] = {
			p_storage ? RDC::DATA_FORMAT_D32_SFLOAT_S8_UINT : RDC::DATA_FORMAT_D24_UNORM_S8_UINT,
			p_storage ? RDC::DATA_FORMAT_D24_UNORM_S8_UINT : RDC::DATA_FORMAT_D32_SFLOAT_S8_UINT};

		return RD::texture_is_format_supported_for_usage(
				   preferred_formats[0], get_depth_usage_bits(p_resolve, p_msaa, p_storage))
				   ? preferred_formats[0]
				   : preferred_formats[1];
	}
}

uint32_t RenderSceneBuffersRD::get_depth_usage_bits(bool p_resolve, bool p_msaa, bool p_storage)
{
	DEV_ASSERT((!p_resolve && !p_msaa) || (p_resolve != p_msaa));

	uint32_t usage_bits = RDC::TEXTURE_USAGE_SAMPLING_BIT;
	if (p_msaa) {
		usage_bits |=
			RDC::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | RDC::TEXTURE_USAGE_CAN_COPY_FROM_BIT;
	}
	else if (p_resolve) {
		usage_bits |= RDC::TEXTURE_USAGE_CAN_COPY_TO_BIT;
		if (p_storage) {
			usage_bits |= RDC::TEXTURE_USAGE_STORAGE_BIT;
		}
		else if (RenderingDevice::has_feature(RDC::SUPPORTS_FRAMEBUFFER_DEPTH_RESOLVE)) {
			// We're able to resolve depth in (sub)passes and we make use of this in our mobile
			// renderer.
			usage_bits |= RDC::TEXTURE_USAGE_DEPTH_RESOLVE_ATTACHMENT_BIT;
		}
	}
	else {
		usage_bits |= RDC::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	}

	return usage_bits;
}

RDC::DataFormat RenderSceneBuffersRD::get_velocity_format()
{
	return RDC::DATA_FORMAT_R16G16_SFLOAT;
}

uint32_t RenderSceneBuffersRD::get_velocity_usage_bits(bool p_resolve, bool p_msaa, bool p_storage)
{
	return get_color_usage_bits(p_resolve, p_msaa, p_storage);
}

RDC::DataFormat RenderSceneBuffersRD::get_vrs_format() { return RD::vrs_get_format(); }

uint32_t RenderSceneBuffersRD::get_vrs_usage_bits()
{
	return RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT | RDC::TEXTURE_USAGE_SAMPLING_BIT |
		   RDC::TEXTURE_USAGE_STORAGE_BIT | RDC::TEXTURE_USAGE_VRS_ATTACHMENT_BIT;
}

float RenderSceneBuffersRD::get_luminance_multiplier() const
{
	// On mobile renderer when not using HDR2D we need to scale HDR values by two
	// to fit 0-2 range color values into a UNORM buffer.
	return (force_hdr || can_be_storage) ? 1.0 : 2.0;
}

RID RenderSceneBuffersRD::create_texture_from_format(const String& p_context,
	const String& p_texture_name, const RDC::TextureFormat& p_texture_format,
	RD::TextureView p_view, bool p_unique)
{
	return RID();
}

RID RenderSceneBuffersRD::create_texture_view(
	String const&, String const&, String const&, RenderingDevice::TextureView)
{
}

// --- Merged from servers/rendering/renderer_rd/storage_rd/render_scene_data_rd.cpp ---

Transform3D RenderSceneDataRD::get_cam_transform() const { return cam_transform; }

Projection RenderSceneDataRD::get_cam_projection() const
{
	Projection correction;
	correction.set_depth_correction(flip_y);
	correction.add_jitter_offset(taa_jitter);

	return correction * cam_projection;
}

uint32_t RenderSceneDataRD::get_view_count() const { return view_count; }

Vector3 RenderSceneDataRD::get_view_eye_offset(uint32_t p_view) const
{
	ERR_FAIL_UNSIGNED_INDEX_V(p_view, view_count, Vector3());

	return view_eye_offset[p_view];
}

Projection RenderSceneDataRD::get_view_projection(uint32_t p_view) const
{
	ERR_FAIL_UNSIGNED_INDEX_V(p_view, view_count, Projection());

	Projection correction;
	correction.set_depth_correction(flip_y);
	correction.add_jitter_offset(taa_jitter);

	return correction * view_projection[p_view];
}

void RenderSceneDataRD::update_ubo(RID p_uniform_buffer, RSE::ViewportDebugDraw p_debug_mode,
	RID p_env, RID p_reflection_probe_instance, RID p_camera_attributes, bool p_pancake_shadows,
	const Size2i& p_screen_size, const Size2& p_viewport_size, const Color& p_default_bg_color,
	float p_luminance_multiplier, bool p_opaque_render_buffers, bool p_apply_alpha_multiplier)
{
	UBODATA ubo_data;
	memset(&ubo_data, 0, sizeof(UBODATA));

	// just for easy access..
	UBO& ubo = ubo_data.ubo;
	UBO& prev_ubo = ubo_data.prev_ubo;

	Projection correction;
	correction.set_depth_correction(flip_y);
	correction.add_jitter_offset(taa_jitter);
	Projection projection = correction * cam_projection;

	// store camera into ubo
	RendererRD::MaterialStorage::store_camera(projection, ubo.projection_matrix);
	RendererRD::MaterialStorage::store_camera(projection.inverse(), ubo.inv_projection_matrix);
	RendererRD::MaterialStorage::store_transform_transposed_3x4(cam_transform, ubo.inv_view_matrix);
	RendererRD::MaterialStorage::store_transform_transposed_3x4(
		cam_transform.affine_inverse(), ubo.view_matrix);

#ifdef REAL_T_IS_DOUBLE
	RendererRD::MaterialStorage::split_double(
		-cam_transform.origin.x, &ubo.inv_view_matrix[3], &ubo.inv_view_precision[0]);
	RendererRD::MaterialStorage::split_double(
		-cam_transform.origin.y, &ubo.inv_view_matrix[7], &ubo.inv_view_precision[1]);
	RendererRD::MaterialStorage::split_double(
		-cam_transform.origin.z, &ubo.inv_view_matrix[11], &ubo.inv_view_precision[2]);
#endif

	for (uint32_t v = 0; v < view_count; v++) {
		projection = correction * view_projection[v];
		RendererRD::MaterialStorage::store_camera(projection, ubo.projection_matrix_view[v]);
		RendererRD::MaterialStorage::store_camera(
			projection.inverse(), ubo.inv_projection_matrix_view[v]);

		ubo.eye_offset[v][0] = view_eye_offset[v].x;
		ubo.eye_offset[v][1] = view_eye_offset[v].y;
		ubo.eye_offset[v][2] = view_eye_offset[v].z;
		ubo.eye_offset[v][3] = 0.0;
	}

	RendererRD::MaterialStorage::store_transform(main_cam_transform, ubo.main_cam_inv_view_matrix);

	ubo.taa_jitter[0] = taa_jitter.x;
	ubo.taa_jitter[1] = taa_jitter.y;
	ubo.taa_frame_count = taa_frame_count;

	ubo.z_far = z_far;
	ubo.z_near = z_near;

	ubo.flags = 0;

	ubo.flags |= p_pancake_shadows ? SCENE_DATA_FLAGS_USE_PANCAKE_SHADOWS : 0;

	RendererRD::MaterialStorage::store_soft_shadow_kernel(
		RendererSceneRenderRD::directional_penumbra_shadow_kernel_get(),
		ubo.directional_penumbra_shadow_kernel);
	RendererRD::MaterialStorage::store_soft_shadow_kernel(
		RendererSceneRenderRD::directional_soft_shadow_kernel_get(),
		ubo.directional_soft_shadow_kernel);
	RendererRD::MaterialStorage::store_soft_shadow_kernel(
		RendererSceneRenderRD::penumbra_shadow_kernel_get(), ubo.penumbra_shadow_kernel);
	RendererRD::MaterialStorage::store_soft_shadow_kernel(
		RendererSceneRenderRD::soft_shadow_kernel_get(), ubo.soft_shadow_kernel);
	ubo.camera_visible_layers = camera_visible_layers;
	ubo.pass_alpha_multiplier = p_opaque_render_buffers && p_apply_alpha_multiplier ? 0.0f : 1.0f;

	ubo.viewport_size[0] = p_viewport_size.x;
	ubo.viewport_size[1] = p_viewport_size.y;

	Size2 screen_pixel_size = Vector2(1.0, 1.0) / Size2(p_screen_size);
	ubo.screen_pixel_size[0] = screen_pixel_size.x;
	ubo.screen_pixel_size[1] = screen_pixel_size.y;

	ubo.shadow_atlas_pixel_size[0] = shadow_atlas_pixel_size.x;
	ubo.shadow_atlas_pixel_size[1] = shadow_atlas_pixel_size.y;

	ubo.directional_shadow_pixel_size[0] = directional_shadow_pixel_size.x;
	ubo.directional_shadow_pixel_size[1] = directional_shadow_pixel_size.y;

	ubo.radiance_pixel_size = radiance_pixel_size;
	ubo.radiance_border_size = radiance_border_size;

	ubo.reflection_atlas_border_size[0] = reflection_atlas_border_size.x;
	ubo.reflection_atlas_border_size[1] = reflection_atlas_border_size.y;

	ubo.time = time;

	ubo.directional_light_count = directional_light_count;
	ubo.dual_paraboloid_side = dual_paraboloid_side;
	ubo.opaque_prepass_threshold = opaque_prepass_threshold;
	ubo.flags |= material_uv2_mode ? SCENE_DATA_FLAGS_USE_UV2_MATERIAL : 0;
	ubo.flags |= shadow_pass ? SCENE_DATA_FLAGS_IN_SHADOW_PASS : 0;

	if (p_debug_mode == RSE::VIEWPORT_DEBUG_DRAW_UNSHADED) {
		ubo.flags |= SCENE_DATA_FLAGS_USE_AMBIENT_LIGHT;
		ubo.ambient_light_color_energy[0] = 1;
		ubo.ambient_light_color_energy[1] = 1;
		ubo.ambient_light_color_energy[2] = 1;
		ubo.ambient_light_color_energy[3] = 1.0;
	}
	else if (p_env.is_valid()) {
		RSE::EnvironmentBG env_bg = RendererSceneRender::environment_get_background(p_env);
		RSE::EnvironmentAmbientSource ambient_src =
			RendererSceneRender::environment_get_ambient_source(p_env);

		float bg_energy_multiplier =
			RendererSceneRender::environment_get_bg_energy_multiplier(p_env);

		ubo.ambient_light_color_energy[3] = bg_energy_multiplier;

		ubo.ambient_color_sky_mix =
			RendererSceneRender::environment_get_ambient_sky_contribution(p_env);

		// ambient
		if (ambient_src == RSE::ENV_AMBIENT_SOURCE_BG &&
			(env_bg == RSE::ENV_BG_CLEAR_COLOR || env_bg == RSE::ENV_BG_COLOR)) {
			Color color = env_bg == RSE::ENV_BG_CLEAR_COLOR
							  ? p_default_bg_color
							  : RendererSceneRender::environment_get_bg_color(p_env);
			color = color.srgb_to_linear();

			ubo.ambient_light_color_energy[0] = color.r * bg_energy_multiplier;
			ubo.ambient_light_color_energy[1] = color.g * bg_energy_multiplier;
			ubo.ambient_light_color_energy[2] = color.b * bg_energy_multiplier;
			ubo.flags |= SCENE_DATA_FLAGS_USE_AMBIENT_LIGHT;
		}
		else {
			float energy = RendererSceneRender::environment_get_ambient_light_energy(p_env);
			Color color = RendererSceneRender::environment_get_ambient_light(p_env);
			color = color.srgb_to_linear();
			ubo.ambient_light_color_energy[0] = color.r * energy;
			ubo.ambient_light_color_energy[1] = color.g * energy;
			ubo.ambient_light_color_energy[2] = color.b * energy;

			bool use_ambient_cubemap =
				(ambient_src == RSE::ENV_AMBIENT_SOURCE_BG && env_bg == RSE::ENV_BG_SKY) ||
				ambient_src == RSE::ENV_AMBIENT_SOURCE_SKY;
			bool use_ambient_light =
				use_ambient_cubemap || ambient_src == RSE::ENV_AMBIENT_SOURCE_COLOR;
			ubo.flags |= use_ambient_cubemap ? SCENE_DATA_FLAGS_USE_AMBIENT_CUBEMAP : 0;
			ubo.flags |= use_ambient_light ? SCENE_DATA_FLAGS_USE_AMBIENT_LIGHT : 0;
		}

		// specular
		RSE::EnvironmentReflectionSource ref_src =
			RendererSceneRender::environment_get_reflection_source(p_env);
		if ((ref_src == RSE::ENV_REFLECTION_SOURCE_BG && env_bg == RSE::ENV_BG_SKY) ||
			ref_src == RSE::ENV_REFLECTION_SOURCE_SKY) {
			ubo.flags |= SCENE_DATA_FLAGS_USE_REFLECTION_CUBEMAP;
		}

		if ((ubo.flags & SCENE_DATA_FLAGS_USE_AMBIENT_CUBEMAP) ||
			(ubo.flags & SCENE_DATA_FLAGS_USE_REFLECTION_CUBEMAP)) {
			Basis sky_transform = RendererSceneRender::environment_get_sky_orientation(p_env);
			sky_transform = sky_transform.inverse() * cam_transform.basis;
			RendererRD::MaterialStorage::store_transform_3x3(
				sky_transform, ubo.radiance_inverse_xform);
		}

		ubo.flags |=
			RendererSceneRender::environment_get_fog_enabled(p_env) ? SCENE_DATA_FLAGS_USE_FOG : 0;
		ubo.fog_density = RendererSceneRender::environment_get_fog_density(p_env);
		ubo.fog_height = RendererSceneRender::environment_get_fog_height(p_env);
		ubo.fog_height_density = RendererSceneRender::environment_get_fog_height_density(p_env);
		ubo.fog_aerial_perspective =
			RendererSceneRender::environment_get_fog_aerial_perspective(p_env);

		ubo.fog_depth_curve = RendererSceneRender::environment_get_fog_depth_curve(p_env);
		ubo.fog_depth_end = RendererSceneRender::environment_get_fog_depth_end(p_env) > 0.0
								? RendererSceneRender::environment_get_fog_depth_end(p_env)
								: ubo.z_far;
		ubo.fog_depth_begin = MIN(
			RendererSceneRender::environment_get_fog_depth_begin(p_env), ubo.fog_depth_end - 0.001);

		Color fog_color =
			RendererSceneRender::environment_get_fog_light_color(p_env).srgb_to_linear();
		float fog_energy = RendererSceneRender::environment_get_fog_light_energy(p_env);

		ubo.fog_light_color[0] = fog_color.r * fog_energy;
		ubo.fog_light_color[1] = fog_color.g * fog_energy;
		ubo.fog_light_color[2] = fog_color.b * fog_energy;

		ubo.fog_sun_scatter = RendererSceneRender::environment_get_fog_sun_scatter(p_env);
	}
	else {
		if (!(p_reflection_probe_instance.is_valid() &&
				RendererRD::LightStorage::reflection_probe_is_interior(
					p_reflection_probe_instance))) {
			ubo.flags |= SCENE_DATA_FLAGS_USE_AMBIENT_LIGHT;
			Color clear_color = p_default_bg_color;
			clear_color = clear_color.srgb_to_linear();
			ubo.ambient_light_color_energy[0] = clear_color.r;
			ubo.ambient_light_color_energy[1] = clear_color.g;
			ubo.ambient_light_color_energy[2] = clear_color.b;
			ubo.ambient_light_color_energy[3] = 1.0;
		}
	}

	if (p_camera_attributes.is_valid()) {
		ubo.emissive_exposure_normalization =
			RendererCameraAttributes::camera_attributes_get_exposure_normalization_factor(
				p_camera_attributes);
		ubo.IBL_exposure_normalization = 1.0;
		if (p_env.is_valid()) {
			RID sky_rid = RendererSceneRender::environment_get_sky(p_env);
			if (sky_rid.is_valid()) {
				float current_exposure =
					RendererCameraAttributes::camera_attributes_get_exposure_normalization_factor(
						p_camera_attributes) *
					RendererSceneRender::environment_get_bg_intensity(p_env) /
					p_luminance_multiplier;
				ubo.IBL_exposure_normalization =
					current_exposure /
					MAX(0.001, RendererSceneRenderRD::get_sky()->sky_get_baked_exposure(sky_rid));
			}
		}
	}
	else if (emissive_exposure_normalization > 0.0) {
		// This branch is triggered when using render_material().
		// Emissive is set outside the function.
		ubo.emissive_exposure_normalization = emissive_exposure_normalization;
		// IBL isn't used don't set it.
	}
	else {
		ubo.emissive_exposure_normalization = 1.0;
		ubo.IBL_exposure_normalization = 1.0;
	}

	bool roughness_limiter_enabled =
		p_opaque_render_buffers && RendererSceneRender::screen_space_roughness_limiter_is_active();
	ubo.flags |= roughness_limiter_enabled ? SCENE_DATA_FLAGS_USE_ROUGHNESS_LIMITER : 0;
	ubo.roughness_limiter_amount = RendererSceneRenderRD::screen_space_roughness_limiter_get_amount();
	ubo.roughness_limiter_limit = RendererSceneRenderRD::screen_space_roughness_limiter_get_limit();

	if (calculate_motion_vectors) {
		// Q : Should we make a complete copy or should we define a separate UBO with just the
		// components we need?
		memcpy(&prev_ubo, &ubo, sizeof(UBO));

		Projection prev_correction;
		prev_correction.set_depth_correction(true);
		prev_correction.add_jitter_offset(prev_taa_jitter);
		Projection prev_projection = prev_correction * prev_cam_projection;

		// store camera into ubo
		RendererRD::MaterialStorage::store_camera(prev_projection, prev_ubo.projection_matrix);
		RendererRD::MaterialStorage::store_camera(
			prev_projection.inverse(), prev_ubo.inv_projection_matrix);
		RendererRD::MaterialStorage::store_transform_transposed_3x4(
			prev_cam_transform, prev_ubo.inv_view_matrix);
		RendererRD::MaterialStorage::store_transform_transposed_3x4(
			prev_cam_transform.affine_inverse(), prev_ubo.view_matrix);

#ifdef REAL_T_IS_DOUBLE
		RendererRD::MaterialStorage::split_double(-prev_cam_transform.origin.x,
			&prev_ubo.inv_view_matrix[3], &prev_ubo.inv_view_precision[0]);
		RendererRD::MaterialStorage::split_double(-prev_cam_transform.origin.y,
			&prev_ubo.inv_view_matrix[7], &prev_ubo.inv_view_precision[1]);
		RendererRD::MaterialStorage::split_double(-prev_cam_transform.origin.z,
			&prev_ubo.inv_view_matrix[11], &prev_ubo.inv_view_precision[2]);
#endif

		for (uint32_t v = 0; v < view_count; v++) {
			prev_projection = prev_correction * view_projection[v];
			RendererRD::MaterialStorage::store_camera(
				prev_projection, prev_ubo.projection_matrix_view[v]);
			RendererRD::MaterialStorage::store_camera(
				prev_projection.inverse(), prev_ubo.inv_projection_matrix_view[v]);
		}
		prev_ubo.taa_jitter[0] = prev_taa_jitter.x;
		prev_ubo.taa_jitter[1] = prev_taa_jitter.y;
		prev_ubo.time -= time_step;
	}
	uniform_buffer = p_uniform_buffer;
}

RID RenderSceneDataRD::get_uniform_buffer() const { return uniform_buffer; }

// --- Merged from servers/rendering/renderer_rd/storage_rd/utilities.cpp ---

using namespace RendererRD;

RSE::InstanceType Utilities::get_base_type(RID p_rid)
{
	if (RendererRD::MeshStorage::owns_mesh(p_rid)) {
		return RSE::INSTANCE_MESH;
	}
	if (RendererRD::MeshStorage::owns_multimesh(p_rid)) {
		return RSE::INSTANCE_MULTIMESH;
	}
	if (RendererRD::LightStorage::owns_reflection_probe(p_rid)) {
		return RSE::INSTANCE_REFLECTION_PROBE;
	}
	if (RendererRD::TextureStorage::owns_decal(p_rid)) {
		return RSE::INSTANCE_DECAL;
	}
	if (RendererRD::GI::owns_voxel_gi(p_rid)) {
		return RSE::INSTANCE_VOXEL_GI;
	}
	if (RendererRD::LightStorage::owns_light(p_rid)) {
		return RSE::INSTANCE_LIGHT;
	}
	if (RendererRD::LightStorage::owns_lightmap(p_rid)) {
		return RSE::INSTANCE_LIGHTMAP;
	}
	if (RendererRD::ParticlesStorage::owns_particles(p_rid)) {
		return RSE::INSTANCE_PARTICLES;
	}
	if (RendererRD::ParticlesStorage::owns_particles_collision(p_rid)) {
		return RSE::INSTANCE_PARTICLES_COLLISION;
	}
	if (RendererRD::Fog::owns_fog_volume(p_rid)) {
		return RSE::INSTANCE_FOG_VOLUME;
	}
	if (owns_visibility_notifier(p_rid)) {
		return RSE::INSTANCE_VISIBLITY_NOTIFIER;
	}

	return RSE::INSTANCE_NONE;
}

bool Utilities::free(RID p_rid)
{
	if (ParticlesStorage::is_initialized() && ParticlesStorage::owns_particles(p_rid)) {
		ParticlesStorage::free(p_rid);
		return true;
	}
	if (ParticlesStorage::is_initialized() && ParticlesStorage::owns_particles_collision(p_rid)) {
		ParticlesStorage::particles_collision_free(p_rid);
		return true;
	}
	if (ParticlesStorage::is_initialized() && ParticlesStorage::owns_particles_collision_instance(p_rid)) {
		ParticlesStorage::particles_collision_instance_free(p_rid);
		return true;
	}
	if (RendererRD::LightStorage::free(p_rid)) {
		return true;
	}
	else if (RendererRD::MaterialStorage::free(p_rid)) {
		return true;
	}
	else if (RendererRD::MeshStorage::free(p_rid)) {
		return true;
	}
	else if (RendererRD::ParticlesStorage::free(p_rid)) {
		return true;
	}
	else if (RendererRD::TextureStorage::free(p_rid)) {
		return true;
	}
	else if (RendererRD::GI::owns_voxel_gi(p_rid)) {
		RendererRD::GI::voxel_gi_free(p_rid);
		return true;
	}
	else if (RendererRD::Fog::owns_fog_volume(p_rid)) {
		RendererRD::Fog::fog_volume_free(p_rid);
		return true;
	}
	else if (owns_visibility_notifier(p_rid)) {
		visibility_notifier_free(p_rid);
		return true;
	}
	else {
		return false;
	}
}

void Utilities::base_update_dependency(RID p_base, DependencyTracker* p_instance)
{
	if (MeshStorage::owns_mesh(p_base)) {
		Dependency* dependency = MeshStorage::mesh_get_dependency(p_base);
		p_instance->update_dependency(dependency);
	}
	else if (MeshStorage::owns_multimesh(p_base)) {
		Dependency* dependency = MeshStorage::multimesh_get_dependency(p_base);
		p_instance->update_dependency(dependency);

		RID mesh = MeshStorage::_multimesh_get_mesh(p_base);
		if (mesh.is_valid()) {
			base_update_dependency(mesh, p_instance);
		}
	}
	else if (LightStorage::owns_reflection_probe(p_base)) {
		Dependency* dependency = LightStorage::reflection_probe_get_dependency(p_base);
		p_instance->update_dependency(dependency);
	}
	else if (TextureStorage::owns_decal(p_base)) {
		Dependency* dependency = TextureStorage::decal_get_dependency(p_base);
		p_instance->update_dependency(dependency);
	}
	else if (GI::owns_voxel_gi(p_base)) {
		Dependency* dependency = GI::voxel_gi_get_dependency(p_base);
		p_instance->update_dependency(dependency);
	}
	else if (LightStorage::owns_lightmap(p_base)) {
		Dependency* dependency = LightStorage::lightmap_get_dependency(p_base);
		p_instance->update_dependency(dependency);
	}
	else if (LightStorage::owns_light(p_base)) {
		Dependency* dependency = LightStorage::light_get_dependency(p_base);
		p_instance->update_dependency(dependency);
	}
	else if (ParticlesStorage::owns_particles(p_base)) {
		Dependency* dependency =
			ParticlesStorage::particles_get_dependency(p_base);
		p_instance->update_dependency(dependency);
	}
	else if (ParticlesStorage::owns_particles_collision(p_base)) {
		Dependency* dependency =
			ParticlesStorage::particles_collision_get_dependency(p_base);
		p_instance->update_dependency(dependency);
	}
	else if (Fog::owns_fog_volume(p_base)) {
		Dependency* dependency = Fog::fog_volume_get_dependency(p_base);
		p_instance->update_dependency(dependency);
	}
	else if (owns_visibility_notifier(p_base)) {
		VisibilityNotifier* vn = get_visibility_notifier(p_base);
		p_instance->update_dependency(&vn->dependency);
	}
}

RID Utilities::visibility_notifier_allocate() { return visibility_notifier_owner.allocate_rid(); }

void Utilities::visibility_notifier_initialize(RID p_notifier)
{
	visibility_notifier_owner.initialize_rid(p_notifier, VisibilityNotifier());
}

void Utilities::visibility_notifier_free(RID p_notifier)
{
	VisibilityNotifier* vn = visibility_notifier_owner.get_or_null(p_notifier);
	vn->dependency.deleted_notify(p_notifier);
	visibility_notifier_owner.free(p_notifier);
}

void Utilities::visibility_notifier_set_aabb(RID p_notifier, const AABB& p_aabb)
{
	VisibilityNotifier* vn = visibility_notifier_owner.get_or_null(p_notifier);
	ERR_FAIL_NULL(vn);
	vn->aabb = p_aabb;
	vn->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_AABB);
}

AABB Utilities::visibility_notifier_get_aabb(RID p_notifier)
{
	const VisibilityNotifier* vn = visibility_notifier_owner.get_or_null(p_notifier);
	ERR_FAIL_NULL_V(vn, AABB());
	return vn->aabb;
}

void Utilities::capture_timestamps_begin() { RD::capture_timestamp("Frame Begin"); }

void Utilities::capture_timestamp(const String& p_name) { RD::capture_timestamp(p_name); }

uint32_t Utilities::get_captured_timestamps_count() { return RD::get_captured_timestamps_count(); }

uint64_t Utilities::get_captured_timestamps_frame() { return RD::get_captured_timestamps_frame(); }

uint64_t Utilities::get_captured_timestamp_gpu_time(uint32_t p_index)
{
	return RD::get_captured_timestamp_gpu_time(p_index);
}

uint64_t Utilities::get_captured_timestamp_cpu_time(uint32_t p_index)
{
	return RD::get_captured_timestamp_cpu_time(p_index);
}

String Utilities::get_captured_timestamp_name(uint32_t p_index)
{
	return RD::get_captured_timestamp_name(p_index);
}

void Utilities::update_dirty_resources()
{
	MaterialStorage::_update_global_shader_uniforms(); // must do before materials, so it can queue
													   // them for update
	MaterialStorage::_update_queued_materials();
	MeshStorage::_update_dirty_multimeshes();
	MeshStorage::_update_dirty_skeletons();
	TextureStorage::update_decal_atlas();
	TextureStorage::update_area_light_atlas();
}

bool Utilities::has_os_feature(const String& p_feature)
{
	if (!RD::data) {
		return false;
	}

	if (p_feature == "rgtc" &&
		RD::texture_is_format_supported_for_usage(
			RDC::DATA_FORMAT_BC5_UNORM_BLOCK, RDC::TEXTURE_USAGE_SAMPLING_BIT)) {
		return true;
	}

#if !defined(ANDROID_ENABLED) && !defined(APPLE_EMBEDDED_ENABLED)
	// Some Android devices report support for S3TC but we don't expect that and don't export the
	// textures. This could be fixed but so few devices support it that it doesn't seem useful (and
	// makes bigger APKs). For good measure we do the same hack for iOS, just in case.
	if (p_feature == "s3tc" &&
		RD::texture_is_format_supported_for_usage(
			RDC::DATA_FORMAT_BC1_RGB_UNORM_BLOCK, RDC::TEXTURE_USAGE_SAMPLING_BIT)) {
		return true;
	}
#endif

	if (p_feature == "bptc" &&
		RD::texture_is_format_supported_for_usage(
			RDC::DATA_FORMAT_BC7_UNORM_BLOCK, RDC::TEXTURE_USAGE_SAMPLING_BIT)) {
		return true;
	}

	if (p_feature == "etc2" &&
		RD::texture_is_format_supported_for_usage(
			RDC::DATA_FORMAT_ETC2_R8G8B8_UNORM_BLOCK, RDC::TEXTURE_USAGE_SAMPLING_BIT)) {
		return true;
	}

	if (p_feature == "astc" &&
		RD::texture_is_format_supported_for_usage(
			RDC::DATA_FORMAT_ASTC_4x4_UNORM_BLOCK, RDC::TEXTURE_USAGE_SAMPLING_BIT)) {
		return true;
	}

	if (p_feature == "astc_hdr" &&
		RD::texture_is_format_supported_for_usage(
			RDC::DATA_FORMAT_ASTC_4x4_SFLOAT_BLOCK, RDC::TEXTURE_USAGE_SAMPLING_BIT)) {
		return true;
	}

	return false;
}

void Utilities::update_memory_info()
{
	texture_mem_cache = RenderingDevice::get_memory_usage(RenderingDevice::MEMORY_TEXTURES);
	buffer_mem_cache = RenderingDevice::get_memory_usage(RenderingDevice::MEMORY_BUFFERS);
	total_mem_cache = RenderingDevice::get_memory_usage(RenderingDevice::MEMORY_TOTAL);
}

uint64_t Utilities::get_rendering_info(RSE::RenderingInfo p_info)
{
	if (p_info == RSE::RENDERING_INFO_TEXTURE_MEM_USED) {
		return texture_mem_cache;
	}
	else if (p_info == RSE::RENDERING_INFO_BUFFER_MEM_USED) {
		return buffer_mem_cache;
	}
	else if (p_info == RSE::RENDERING_INFO_VIDEO_MEM_USED) {
		return total_mem_cache;
	}
	return 0;
}

String Utilities::get_video_adapter_name() { return RenderingDevice::get_device_name(); }

String Utilities::get_video_adapter_vendor() { return RenderingDevice::get_device_vendor_name(); }

RenderingDeviceEnums::DeviceType Utilities::get_video_adapter_type()
{
	return RenderingDevice::get_device_type();
}

String Utilities::get_video_adapter_api_version()
{
	return RenderingDevice::get_device_api_version();
}

Size2i Utilities::get_maximum_viewport_size()
{
	int max_x = RenderingDevice::limit_get(RDC::LIMIT_MAX_VIEWPORT_DIMENSIONS_X);
	int max_y = RenderingDevice::limit_get(RDC::LIMIT_MAX_VIEWPORT_DIMENSIONS_Y);
	return Size2i(max_x, max_y);
}

uint32_t Utilities::get_maximum_shader_varyings()
{
	return RenderingDevice::limit_get(RDC::LIMIT_MAX_SHADER_VARYINGS);
}

uint64_t Utilities::get_maximum_uniform_buffer_size()
{
	return RenderingDevice::limit_get(RDC::LIMIT_MAX_UNIFORM_BUFFER_SIZE);
}

void RendererRD::Utilities::visibility_notifier_call(
	RID p_notifier, bool p_enter_call, bool p_deferred)
{
}
