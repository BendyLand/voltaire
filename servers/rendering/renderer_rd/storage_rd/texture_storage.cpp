/**************************************************************************/
/*  texture_storage.cpp                                                   */
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

#include "core/config/engine.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/renderer_rd/effects/copy_effects.h"
#include "servers/rendering/renderer_rd/rd_caches.h"
#include "servers/rendering/renderer_rd/renderer_scene_render_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/rendering_device.h"
#include "servers/rendering/storage/texture_storage.h"
#include "texture_storage.h"

using namespace RendererRD;

void RendererRD::TextureStorage::CanvasTexture::clear_cache()
{
	info_cache[0] = CanvasTextureCache();
	info_cache[1] = CanvasTextureCache();
	if (invalidated_callback != nullptr) {
		invalidated_callback(false, invalidated_callback_userdata);
	}
}

RendererRD::TextureStorage::CanvasTexture::~CanvasTexture()
{
	if (invalidated_callback != nullptr) {
		invalidated_callback(true, invalidated_callback_userdata);
	}
}

void RendererRD::TextureStorage::Texture::cleanup()
{
	if (RD::texture_is_valid(rd_texture_srgb)) {
		// erase this first, as it's a dependency of the one below
		RD::free_rid(rd_texture_srgb);
	}
	if (RD::texture_is_valid(rd_texture)) {
		RD::free_rid(rd_texture);
	}
	memdelete(canvas_texture);
}

void RendererRD::TextureStorage::finalize()
{
	if (rt_sdf.shader_version.is_valid()) {
		rt_sdf.shader.version_free(rt_sdf.shader_version);
		rt_sdf.shader_version = RID();
	}

	free_decal_data();

	if (decal_atlas.textures.size()) {
		ERR_PRINT("Decal Atlas: " + itos(decal_atlas.textures.size()) +
				  " textures were not removed from the atlas.");
	}

	if (decal_atlas.texture.is_valid()) {
		RD::free_rid(decal_atlas.texture);
	}

	if (area_light_atlas.textures.size()) {
		ERR_PRINT("Area Light Atlas: " + itos(area_light_atlas.textures.size()) +
				  " textures were not removed from the atlas.");
	}

	if (area_light_atlas.texture.is_valid()) {
		RD::free_rid(area_light_atlas.texture);
	}

	// def textures
	for (int i = 0; i < DEFAULT_RD_TEXTURE_MAX; i++) {
		if (default_rd_textures[i].is_valid()) {
			RD::free_rid(default_rd_textures[i]);
		}
	}
}

void RendererRD::TextureStorage::_tex_blit_shader_initialize()
{
	{
		Vector<String> tex_blit_modes;
		tex_blit_modes.push_back("");											  // Only 1 Output
		tex_blit_modes.push_back("\n#define USE_OUTPUT1\n");					  // 2 Outputs
		tex_blit_modes.push_back("\n#define USE_OUTPUT1\n#define USE_OUTPUT2\n"); // 3 Outputs
		tex_blit_modes.push_back(
			"\n#define USE_OUTPUT1\n#define USE_OUTPUT2\n#define USE_OUTPUT3\n"); // 4 Outputs
		String global_defines;
		global_defines +=
			"\n#define SAMPLERS_BINDING_FIRST_INDEX " + itos(SAMPLERS_BINDING_FIRST_INDEX) + "\n";
		global_defines +=
			"#define MAX_GLOBAL_SHADER_UNIFORMS 256\n"; // TODO: this is arbitrary for now
		tex_blit_shader.shader.initialize(tex_blit_modes, global_defines);
	}
	MaterialStorage::shader_set_data_request_function(
		MaterialStorage::SHADER_TYPE_TEXTURE_BLIT, MaterialStorage::_create_tex_blit_shader_funcs);
	MaterialStorage::material_set_data_request_function(MaterialStorage::SHADER_TYPE_TEXTURE_BLIT,
		MaterialStorage::_create_tex_blit_material_funcs);

	{
		// Setup TextureBlit compiler
		ShaderCompiler::DefaultIdentifierActions actions;

		actions.renames["TIME"] = "data.time";
		actions.renames["PI"] = String::num(Math::PI);
		actions.renames["TAU"] = String::num(Math::TAU);
		actions.renames["E"] = String::num(Math::E);

		actions.renames["FRAGCOORD"] = "gl_FragCoord";

		actions.renames["UV"] = "uv";
		actions.renames["MODULATE"] = "data.modulate";

		actions.renames["COLOR0"] = "color0";
		actions.renames["COLOR1"] = "color1";
		actions.renames["COLOR2"] = "color2";
		actions.renames["COLOR3"] = "color3";

		actions.base_uniform_string = "material.";
		actions.base_texture_binding_index = 1;
		actions.texture_layout_set = 1;

		tex_blit_shader.compiler.initialize(actions);
	}

	{
		// default material and shader for Texture Blit shader
		tex_blit_shader.default_shader = MaterialStorage::shader_allocate();
		MaterialStorage::shader_initialize(tex_blit_shader.default_shader);
		MaterialStorage::shader_set_code(tex_blit_shader.default_shader, R"(
// Default Texture Blit shader.

shader_type texture_blit;
render_mode blend_mix;

uniform sampler2D source_texture0 : hint_blit_source0;
uniform sampler2D source_texture1 : hint_blit_source1;
uniform sampler2D source_texture2 : hint_blit_source2;
uniform sampler2D source_texture3 : hint_blit_source3;

void blit() {
	// Copies from each whole source texture to a rect on each output texture.
	COLOR0 = texture(source_texture0, UV) * MODULATE;
	COLOR1 = texture(source_texture1, UV) * MODULATE;
	COLOR2 = texture(source_texture2, UV) * MODULATE;
	COLOR3 = texture(source_texture3, UV) * MODULATE;
}
)");
		tex_blit_shader.default_material = MaterialStorage::material_allocate();
		MaterialStorage::material_initialize(tex_blit_shader.default_material);
		MaterialStorage::material_set_shader(
			tex_blit_shader.default_material, tex_blit_shader.default_shader);
	}

	tex_blit_shader.initialized = true;
}

void RendererRD::TextureStorage::_tex_blit_shader_free()
{
	if (tex_blit_shader.initialized) {
		print_verbose("Freeing Default Tex_Blit Shader");
		MaterialStorage::material_free(tex_blit_shader.default_material);
		MaterialStorage::shader_free(tex_blit_shader.default_shader);
	}
}

bool RendererRD::TextureStorage::free(RID p_rid)
{
	if (owns_texture(p_rid)) {
		texture_free(p_rid);
		return true;
	}
	else if (owns_canvas_texture(p_rid)) {
		canvas_texture_free(p_rid);
		return true;
	}
	else if (owns_decal(p_rid)) {
		decal_free(p_rid);
		return true;
	}
	else if (owns_decal_instance(p_rid)) {
		decal_instance_free(p_rid);
		return true;
	}
	else if (owns_render_target(p_rid)) {
		render_target_free(p_rid);
		return true;
	}

	return false;
}

RID RendererRD::TextureStorage::canvas_texture_allocate()
{
	return canvas_texture_owner.allocate_rid();
}

void RendererRD::TextureStorage::canvas_texture_initialize(RID p_rid)
{
	canvas_texture_owner.initialize_rid(p_rid);
}

void RendererRD::TextureStorage::canvas_texture_free(RID p_rid)
{
	canvas_texture_owner.free(p_rid);
}

void RendererRD::TextureStorage::canvas_texture_set_channel(
	RID p_canvas_texture, RSE::CanvasTextureChannel p_channel, RID p_texture)
{
	CanvasTexture* ct = canvas_texture_owner.get_or_null(p_canvas_texture);
	ERR_FAIL_NULL(ct);

	switch (p_channel) {
	case RSE::CANVAS_TEXTURE_CHANNEL_DIFFUSE: {
		ct->diffuse = p_texture;
	} break;
	case RSE::CANVAS_TEXTURE_CHANNEL_NORMAL: {
		ct->normal_map = p_texture;
	} break;
	case RSE::CANVAS_TEXTURE_CHANNEL_SPECULAR: {
		ct->specular = p_texture;
	} break;
	}
	ct->clear_cache();
}

void RendererRD::TextureStorage::canvas_texture_set_shading_parameters(
	RID p_canvas_texture, const Color& p_specular_color, float p_shininess)
{
	CanvasTexture* ct = canvas_texture_owner.get_or_null(p_canvas_texture);
	ERR_FAIL_NULL(ct);

	ct->specular_color.r = p_specular_color.r;
	ct->specular_color.g = p_specular_color.g;
	ct->specular_color.b = p_specular_color.b;
	ct->specular_color.a = p_shininess;
}

void RendererRD::TextureStorage::canvas_texture_set_texture_filter(
	RID p_canvas_texture, RSE::CanvasItemTextureFilter p_filter)
{
	CanvasTexture* ct = canvas_texture_owner.get_or_null(p_canvas_texture);
	ERR_FAIL_NULL(ct);

	ct->texture_filter = p_filter;
}

void RendererRD::TextureStorage::canvas_texture_set_texture_repeat(
	RID p_canvas_texture, RSE::CanvasItemTextureRepeat p_repeat)
{
	CanvasTexture* ct = canvas_texture_owner.get_or_null(p_canvas_texture);
	ERR_FAIL_NULL(ct);

	ct->texture_repeat = p_repeat;
}

RendererRD::TextureStorage::CanvasTextureInfo RendererRD::TextureStorage::canvas_texture_get_info(
	RID p_texture, RSE::CanvasItemTextureFilter p_base_filter,
	RSE::CanvasItemTextureRepeat p_base_repeat, bool p_use_srgb, bool p_texture_is_data)
{
	CanvasTexture* ct = nullptr;
	Texture* t = get_texture(p_texture);

	if (t) {
		// regular texture
		if (!t->canvas_texture) {
			t->canvas_texture = memnew(CanvasTexture);
			t->canvas_texture->diffuse = p_texture;
		}

		ct = t->canvas_texture;
		if (t->render_target) {
			t->render_target->was_used = true;
		}
	}
	else {
		ct = canvas_texture_owner.get_or_null(p_texture);
	}

	if (!ct) {
		return CanvasTextureInfo(); // invalid texture RID
	}

	RSE::CanvasItemTextureFilter filter =
		ct->texture_filter != RSE::CANVAS_ITEM_TEXTURE_FILTER_DEFAULT ? ct->texture_filter
																	  : p_base_filter;
	ERR_FAIL_COND_V(filter == RSE::CANVAS_ITEM_TEXTURE_FILTER_DEFAULT, CanvasTextureInfo());

	RSE::CanvasItemTextureRepeat repeat =
		ct->texture_repeat != RSE::CANVAS_ITEM_TEXTURE_REPEAT_DEFAULT ? ct->texture_repeat
																	  : p_base_repeat;
	ERR_FAIL_COND_V(repeat == RSE::CANVAS_ITEM_TEXTURE_REPEAT_DEFAULT, CanvasTextureInfo());

	CanvasTextureCache& ctc = ct->info_cache[int(p_use_srgb)];
	if (!RD::texture_is_valid(ctc.diffuse) || !RD::texture_is_valid(ctc.normal) ||
		!RD::texture_is_valid(ctc.specular)) {
		{ // diffuse
			t = get_texture(ct->diffuse);
			if (!t) {
				ctc.diffuse = texture_rd_get_default(DEFAULT_RD_TEXTURE_WHITE);
				ct->size_cache = Size2i(1, 1);
			}
			else {
				ctc.diffuse = t->rd_texture_srgb.is_valid() && p_use_srgb && !p_texture_is_data
								  ? t->rd_texture_srgb
								  : t->rd_texture;
				ct->size_cache = Size2i(t->width_2d, t->height_2d);
				if (t->render_target) {
					t->render_target->was_used = true;
				}
			}
		}
		{ // normal
			t = get_texture(ct->normal_map);
			if (!t) {
				ctc.normal = texture_rd_get_default(DEFAULT_RD_TEXTURE_NORMAL);
				ct->use_normal_cache = false;
			}
			else {
				ctc.normal = t->rd_texture;
				ct->use_normal_cache = true;
				if (t->render_target) {
					t->render_target->was_used = true;
				}
			}
		}
		{ // specular
			t = get_texture(ct->specular);
			if (!t) {
				ctc.specular = texture_rd_get_default(DEFAULT_RD_TEXTURE_WHITE);
				ct->use_specular_cache = false;
			}
			else {
				ctc.specular = t->rd_texture;
				ct->use_specular_cache = true;
				if (t->render_target) {
					t->render_target->was_used = true;
				}
			}
		}
	}

	CanvasTextureInfo res;
	res.diffuse = ctc.diffuse;
	res.normal = ctc.normal;
	res.specular = ctc.specular;
	res.sampler = MaterialStorage::sampler_rd_get_default(filter, repeat);
	res.size = ct->size_cache;
	res.specular_color = ct->specular_color;
	res.use_normal = ct->use_normal_cache;
	res.use_specular = ct->use_specular_cache;

	return res;
}

void RendererRD::TextureStorage::canvas_texture_set_invalidation_callback(
	RID p_canvas_texture, InvalidationCallback p_callback, void* p_userdata)
{
	CanvasTexture* ct = canvas_texture_owner.get_or_null(p_canvas_texture);
	if (!ct) {
		return;
	}

	ct->invalidated_callback = p_callback;
	ct->invalidated_callback_userdata = p_userdata;
}

RID RendererRD::TextureStorage::texture_allocate() { return texture_owner.allocate_rid(); }

void RendererRD::TextureStorage::texture_free(RID p_texture)
{
	Texture* t = texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL(t);
	ERR_FAIL_COND(t->is_render_target);

	t->cleanup();

	if (t->is_proxy && t->proxy_to.is_valid()) {
		Texture* proxy_to = texture_owner.get_or_null(t->proxy_to);
		if (proxy_to) {
			proxy_to->proxies.erase(p_texture);
		}
	}

	decal_atlas_remove_texture(p_texture);
	area_light_atlas_remove_texture(p_texture);

	for (int i = 0; i < t->proxies.size(); i++) {
		Texture* p = texture_owner.get_or_null(t->proxies[i]);
		ERR_CONTINUE(!p);
		p->proxy_to = RID();
		p->rd_texture = RID();
		p->rd_texture_srgb = RID();
	}

	texture_owner.free(p_texture);
}

void RendererRD::TextureStorage::texture_external_initialize(
	RID p_texture, int p_width, int p_height, uint64_t p_external_buffer)
{
}

void RendererRD::TextureStorage::texture_proxy_initialize(RID p_texture, RID p_base)
{
	Texture* tex = texture_owner.get_or_null(p_base);
	ERR_FAIL_NULL(tex);
	Texture proxy_tex = *tex;

	proxy_tex.rd_view.format_override = tex->rd_format;
	proxy_tex.rd_texture = RD::texture_create_shared(proxy_tex.rd_view, tex->rd_texture);
	if (proxy_tex.rd_texture_srgb.is_valid()) {
		proxy_tex.rd_view.format_override = tex->rd_format_srgb;
		proxy_tex.rd_texture_srgb = RD::texture_create_shared(proxy_tex.rd_view, tex->rd_texture);
	}
	proxy_tex.proxy_to = p_base;
	proxy_tex.is_render_target = false;
	proxy_tex.is_proxy = true;
	proxy_tex.proxies.clear();

	texture_owner.initialize_rid(p_texture, proxy_tex);

	tex->proxies.push_back(p_texture);
}

// Note: We make some big assumptions about format and usage. If developers need more control,
// they should use RD::texture_create_from_extension() instead.
RID RendererRD::TextureStorage::texture_create_from_native_handle(RSE::TextureType p_type,
	Image::Format p_format, uint64_t p_native_handle, int p_width, int p_height, int p_depth,
	int p_layers, RSE::TextureLayeredType p_layered_type)
{
	RDC::TextureType type;
	switch (p_type) {
	case RSE::TEXTURE_TYPE_2D:
		type = RDC::TEXTURE_TYPE_2D;
		break;

	case RSE::TEXTURE_TYPE_3D:
		type = RDC::TEXTURE_TYPE_3D;
		break;

	case RSE::TEXTURE_TYPE_LAYERED:
		if (p_layered_type == RSE::TEXTURE_LAYERED_2D_ARRAY) {
			type = RDC::TEXTURE_TYPE_2D_ARRAY;
		}
		else if (p_layered_type == RSE::TEXTURE_LAYERED_CUBEMAP) {
			type = RDC::TEXTURE_TYPE_CUBE;
		}
		else if (p_layered_type == RSE::TEXTURE_LAYERED_CUBEMAP_ARRAY) {
			type = RDC::TEXTURE_TYPE_CUBE_ARRAY;
		}
		else {
			// Arbitrary fallback.
			type = RDC::TEXTURE_TYPE_2D_ARRAY;
		}
		break;

	default:
		// Arbitrary fallback.
		type = RDC::TEXTURE_TYPE_2D;
	}

	// Only a rough conversion - see note above.
	RDC::DataFormat format;
	switch (p_format) {
	case Image::FORMAT_L8:
	case Image::FORMAT_R8:
		format = RDC::DATA_FORMAT_R8_UNORM;
		break;

	case Image::FORMAT_LA8:
	case Image::FORMAT_RG8:
		format = RDC::DATA_FORMAT_R8G8_UNORM;
		break;

	case Image::FORMAT_RGB8:
		format = RDC::DATA_FORMAT_R8G8B8_UNORM;
		break;

	case Image::FORMAT_RGBA8:
		format = RDC::DATA_FORMAT_R8G8B8A8_UNORM;
		break;

	case Image::FORMAT_RGBA4444:
		format = RDC::DATA_FORMAT_B4G4R4A4_UNORM_PACK16;
		break;

	case Image::FORMAT_RGB565:
		format = RDC::DATA_FORMAT_B5G6R5_UNORM_PACK16;
		break;

	case Image::FORMAT_RF:
		format = RDC::DATA_FORMAT_R32_SFLOAT;
		break;

	case Image::FORMAT_RGF:
		format = RDC::DATA_FORMAT_R32G32_SFLOAT;
		break;

	case Image::FORMAT_RGBF:
		format = RDC::DATA_FORMAT_R32G32B32_SFLOAT;
		break;

	case Image::FORMAT_RGBAF:
		format = RDC::DATA_FORMAT_R32G32B32_SFLOAT;
		break;

	case Image::FORMAT_RH:
		format = RDC::DATA_FORMAT_R16_SFLOAT;
		break;

	case Image::FORMAT_RGH:
		format = RDC::DATA_FORMAT_R16G16_SFLOAT;
		break;

	case Image::FORMAT_RGBH:
		format = RDC::DATA_FORMAT_R16G16B16_SFLOAT;
		break;

	case Image::FORMAT_RGBAH:
		format = RDC::DATA_FORMAT_R16G16B16A16_SFLOAT;
		break;

	case Image::FORMAT_RGBE9995:
		format = RDC::DATA_FORMAT_E5B9G9R9_UFLOAT_PACK32;
		break;

	case Image::FORMAT_DXT1:
		format = RDC::DATA_FORMAT_BC1_RGB_UNORM_BLOCK;
		break;

	case Image::FORMAT_DXT3:
		format = RDC::DATA_FORMAT_BC2_UNORM_BLOCK;
		break;

	case Image::FORMAT_DXT5:
		format = RDC::DATA_FORMAT_BC3_UNORM_BLOCK;
		break;

	case Image::FORMAT_RGTC_R:
		format = RDC::DATA_FORMAT_BC4_UNORM_BLOCK;
		break;

	case Image::FORMAT_RGTC_RG:
		format = RDC::DATA_FORMAT_BC5_UNORM_BLOCK;
		break;

	case Image::FORMAT_BPTC_RGBA:
		format = RDC::DATA_FORMAT_BC7_UNORM_BLOCK;
		break;

	case Image::FORMAT_BPTC_RGBF:
		format = RDC::DATA_FORMAT_BC6H_SFLOAT_BLOCK;
		break;

	case Image::FORMAT_BPTC_RGBFU:
		format = RDC::DATA_FORMAT_BC6H_UFLOAT_BLOCK;
		break;

	case Image::FORMAT_ETC:
		format = RDC::DATA_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
		break;

	case Image::FORMAT_ETC2_R11:
		format = RDC::DATA_FORMAT_EAC_R11_UNORM_BLOCK;
		break;

	case Image::FORMAT_ETC2_R11S:
		format = RDC::DATA_FORMAT_EAC_R11_SNORM_BLOCK;
		break;

	case Image::FORMAT_ETC2_RG11:
		format = RDC::DATA_FORMAT_EAC_R11G11_UNORM_BLOCK;
		break;

	case Image::FORMAT_ETC2_RG11S:
		format = RDC::DATA_FORMAT_EAC_R11G11_SNORM_BLOCK;
		break;

	case Image::FORMAT_ETC2_RGB8:
		format = RDC::DATA_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
		break;

	case Image::FORMAT_ETC2_RGBA8:
		format = RDC::DATA_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;
		break;

	case Image::FORMAT_ETC2_RGB8A1:
		format = RDC::DATA_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK;
		break;

	case Image::FORMAT_ETC2_RA_AS_RG:
		format = RDC::DATA_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;
		break;

	case Image::FORMAT_DXT5_RA_AS_RG:
		format = RDC::DATA_FORMAT_BC3_UNORM_BLOCK;
		break;

	case Image::FORMAT_ASTC_4x4:
		format = RDC::DATA_FORMAT_ASTC_4x4_UNORM_BLOCK;
		break;

	case Image::FORMAT_ASTC_4x4_HDR:
		format = RDC::DATA_FORMAT_ASTC_4x4_SFLOAT_BLOCK;
		break;

	case Image::FORMAT_ASTC_6x6:
		format = RDC::DATA_FORMAT_ASTC_6x6_UNORM_BLOCK;
		break;

	case Image::FORMAT_ASTC_6x6_HDR:
		format = RDC::DATA_FORMAT_ASTC_6x6_SFLOAT_BLOCK;
		break;

	case Image::FORMAT_ASTC_8x8:
		format = RDC::DATA_FORMAT_ASTC_8x8_UNORM_BLOCK;
		break;

	case Image::FORMAT_ASTC_8x8_HDR:
		format = RDC::DATA_FORMAT_ASTC_8x8_SFLOAT_BLOCK;
		break;

	default:
		// Arbitrary fallback.
		format = RDC::DATA_FORMAT_R8G8B8A8_UNORM;
	}

	// Assumed to be a color attachment - see note above.
	uint64_t usage_flags =
		RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT;

	RID rd_texture = RD::texture_create_from_extension(type, format, RDC::TEXTURE_SAMPLES_1,
		usage_flags, p_native_handle, p_width, p_height, p_depth, p_layers, 1);

	RID texture = texture_allocate();
	texture_rd_initialize(texture, rd_texture, p_layered_type);

	return texture;
}

void RendererRD::TextureStorage::texture_external_update(
	RID p_texture, int p_width, int p_height, uint64_t p_external_buffer)
{
}

void RendererRD::TextureStorage::texture_proxy_update(RID p_texture, RID p_proxy_to)
{
	Texture* tex = texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL(tex);
	ERR_FAIL_COND(!tex->is_proxy);
	Texture* proxy_to = texture_owner.get_or_null(p_proxy_to);
	ERR_FAIL_NULL(proxy_to);
	ERR_FAIL_COND(proxy_to->is_proxy);

	if (tex->proxy_to.is_valid()) {
		// unlink proxy
		if (RD::texture_is_valid(tex->rd_texture)) {
			RD::free_rid(tex->rd_texture);
			tex->rd_texture = RID();
		}
		if (RD::texture_is_valid(tex->rd_texture_srgb)) {
			RD::free_rid(tex->rd_texture_srgb);
			tex->rd_texture_srgb = RID();
		}
		Texture* prev_tex = texture_owner.get_or_null(tex->proxy_to);
		ERR_FAIL_NULL(prev_tex);
		prev_tex->proxies.erase(p_texture);
	}

	// Copy canvas_texture so it doesn't leak.
	CanvasTexture* canvas_texture = tex->canvas_texture;

	*tex = *proxy_to;

	tex->proxy_to = p_proxy_to;
	tex->is_render_target = false;
	tex->is_proxy = true;
	tex->proxies.clear();
	proxy_to->proxies.push_back(p_texture);
	tex->canvas_texture = canvas_texture;

	tex->rd_view.format_override = tex->rd_format;
	tex->rd_texture = RD::texture_create_shared(tex->rd_view, proxy_to->rd_texture);
	if (tex->rd_texture_srgb.is_valid()) {
		tex->rd_view.format_override = tex->rd_format_srgb;
		tex->rd_texture_srgb = RD::texture_create_shared(tex->rd_view, proxy_to->rd_texture);
	}
}

// these two APIs can be used together or in combination with the others.
void RendererRD::TextureStorage::texture_2d_placeholder_initialize(RID p_texture)
{
	texture_2d_initialize(p_texture, texture_2d_placeholder);
}

void RendererRD::TextureStorage::texture_2d_layered_placeholder_initialize(
	RID p_texture, RSE::TextureLayeredType p_layered_type)
{
	if (p_layered_type == RSE::TEXTURE_LAYERED_2D_ARRAY) {
		texture_2d_layered_initialize(p_texture, texture_2d_array_placeholder, p_layered_type);
	}
	else {
		texture_2d_layered_initialize(p_texture, cubemap_placeholder, p_layered_type);
	}
}

void RendererRD::TextureStorage::texture_3d_placeholder_initialize(RID p_texture)
{
	texture_3d_initialize(p_texture, Image::FORMAT_RGBA8, 4, 4, 4, false, texture_3d_placeholder);
}

Ref<Image> RendererRD::TextureStorage::texture_2d_get(RID p_texture)
{
	Texture* tex = texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL_V(tex, Ref<Image>());

#ifdef TOOLS_ENABLED
	if (tex->image_cache_2d.is_valid() && !tex->is_render_target) {
		return tex->image_cache_2d;
	}
#endif
	Vector<uint8_t> data = RD::texture_get_data(tex->rd_texture, 0);
	ERR_FAIL_COND_V(data.is_empty(), Ref<Image>());
	Ref<Image> image;

	// Expand RGB10_A2 into RGBAH.
	if (tex->rd_format == RDC::DATA_FORMAT_A2B10G10R10_UNORM_PACK32) {
		Vector<uint8_t> new_data;
		new_data.resize(data.size() * 2);
		uint16_t* ndp = (uint16_t*)new_data.ptr();

		uint32_t* ptr = (uint32_t*)data.ptr();
		uint32_t num_pixels = data.size() / 4;

		for (uint32_t ofs = 0; ofs < num_pixels; ofs++) {
			uint32_t px = ptr[ofs];
			uint32_t r = (px & 0x3FF);
			uint32_t g = ((px >> 10) & 0x3FF);
			uint32_t b = ((px >> 20) & 0x3FF);
			uint32_t a = ((px >> 30) & 0x3);

			ndp[ofs * 4 + 0] = Math::make_half_float(float(r) / 1023.0);
			ndp[ofs * 4 + 1] = Math::make_half_float(float(g) / 1023.0);
			ndp[ofs * 4 + 2] = Math::make_half_float(float(b) / 1023.0);
			ndp[ofs * 4 + 3] = Math::make_half_float(float(a) / 3.0);
		}
		image = Image::create_from_data(
			tex->width, tex->height, tex->mipmaps > 1, tex->validated_format, new_data);
	}
	else {
		image = Image::create_from_data(
			tex->width, tex->height, tex->mipmaps > 1, tex->validated_format, data);
	}

	if (image->is_empty()) {
		const String& path_str =
			tex->path.is_empty() ? "with no path" : vformat("with path '%s'", tex->path);
		ERR_FAIL_V_MSG(Ref<Image>(), vformat("Texture %s has no data.", path_str));
	}

	if (tex->format != tex->validated_format) {
		image->convert(tex->format);
	}

#ifdef TOOLS_ENABLED
	if (Engine::is_editor_hint() && !tex->is_render_target) {
		tex->image_cache_2d = image;
	}
#endif

	return image;
}

Ref<Image> RendererRD::TextureStorage::texture_2d_layer_get(RID p_texture, int p_layer)
{
	Texture* tex = texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL_V(tex, Ref<Image>());

	Vector<uint8_t> data = RD::texture_get_data(tex->rd_texture, p_layer);
	ERR_FAIL_COND_V(data.is_empty(), Ref<Image>());
	Ref<Image> image = Image::create_from_data(
		tex->width, tex->height, tex->mipmaps > 1, tex->validated_format, data);
	if (image->is_empty()) {
		const String& path_str =
			tex->path.is_empty() ? "with no path" : vformat("with path '%s'", tex->path);
		ERR_FAIL_V_MSG(Ref<Image>(), vformat("Texture %s has no data.", path_str));
	}
	if (tex->format != tex->validated_format) {
		image->convert(tex->format);
	}

	return image;
}

Vector<Ref<Image>> RendererRD::TextureStorage::texture_3d_get(RID p_texture)
{
	Texture* tex = texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL_V(tex, Vector<Ref<Image>>());
	ERR_FAIL_COND_V(tex->type != TextureStorage::TYPE_3D, Vector<Ref<Image>>());

	Vector<uint8_t> all_data = RD::texture_get_data(tex->rd_texture, 0);

	ERR_FAIL_COND_V(all_data.size() != (int)tex->buffer_size_3d, Vector<Ref<Image>>());

	Vector<Ref<Image>> ret;

	for (int i = 0; i < tex->buffer_slices_3d.size(); i++) {
		const Texture::BufferSlice3D& bs = tex->buffer_slices_3d[i];
		ERR_FAIL_COND_V(bs.offset >= (uint32_t)all_data.size(), Vector<Ref<Image>>());
		ERR_FAIL_COND_V(
			bs.offset + bs.buffer_size > (uint32_t)all_data.size(), Vector<Ref<Image>>());
		Vector<uint8_t> sub_region = all_data.slice(bs.offset, bs.offset + bs.buffer_size);

		Ref<Image> img = Image::create_from_data(
			bs.size.width, bs.size.height, false, tex->validated_format, sub_region);
		ERR_FAIL_COND_V(img->is_empty(), Vector<Ref<Image>>());
		if (img->is_empty()) {
			const String& path_str =
				tex->path.is_empty() ? "with no path" : vformat("with path '%s'", tex->path);
			ERR_FAIL_V_MSG(Vector<Ref<Image>>(), vformat("Texture %s has no data.", path_str));
		}
		if (tex->format != tex->validated_format) {
			img->convert(tex->format);
		}

		ret.push_back(img);
	}

	return ret;
}

RID RendererRD::TextureStorage::texture_drawable_get_default_material()
{
	// Return a material with a default Texture_Blit shader for DrawableTexture2D to use
	return tex_blit_shader.default_material;
}

void RendererRD::TextureStorage::texture_replace(RID p_texture, RID p_by_texture)
{
	Texture* tex = texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL(tex);
	ERR_FAIL_COND(tex->proxy_to.is_valid()); // can't replace proxy
	Texture* by_tex = texture_owner.get_or_null(p_by_texture);
	ERR_FAIL_NULL(by_tex);
	ERR_FAIL_COND(by_tex->proxy_to.is_valid()); // can't replace proxy

	if (tex == by_tex) {
		return;
	}

	if (tex->rd_texture_srgb.is_valid()) {
		RD::free_rid(tex->rd_texture_srgb);
	}
	RD::free_rid(tex->rd_texture);

	if (tex->canvas_texture) {
		memdelete(tex->canvas_texture);
		tex->canvas_texture = nullptr;
	}

	Vector<RID> proxies_to_update = tex->proxies;
	Vector<RID> proxies_to_redirect = by_tex->proxies;

	*tex = *by_tex;

	tex->proxies = proxies_to_update; // restore proxies, so they can be updated

	if (tex->canvas_texture) {
		tex->canvas_texture->diffuse = p_texture; // update
	}

	for (int i = 0; i < proxies_to_update.size(); i++) {
		texture_proxy_update(proxies_to_update[i], p_texture);
	}
	for (int i = 0; i < proxies_to_redirect.size(); i++) {
		texture_proxy_update(proxies_to_redirect[i], p_texture);
	}
	// delete last, so proxies can be updated
	texture_owner.free(p_by_texture);

	decal_atlas_mark_dirty_on_texture(p_texture);
	area_light_atlas_mark_dirty_on_texture(p_texture);
}

void RendererRD::TextureStorage::texture_set_size_override(RID p_texture, int p_width, int p_height)
{
	Texture* tex = texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL(tex);
	ERR_FAIL_COND(tex->type != TextureStorage::TYPE_2D);

	tex->width_2d = p_width;
	tex->height_2d = p_height;
}

void RendererRD::TextureStorage::texture_set_path(RID p_texture, const String& p_path)
{
	Texture* tex = texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL(tex);

	tex->path = p_path;
}

String RendererRD::TextureStorage::texture_get_path(RID p_texture)
{
	Texture* tex = texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL_V(tex, String());

	return tex->path;
}

Image::Format RendererRD::TextureStorage::texture_get_format(RID p_texture)
{
	Texture* tex = texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL_V(tex, Image::FORMAT_MAX);

	return tex->format;
}

void RendererRD::TextureStorage::texture_set_detect_3d_callback(
	RID p_texture, RenderingServerTypes::TextureDetectCallback p_callback, void* p_userdata)
{
	Texture* tex = texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL(tex);

	tex->detect_3d_callback_ud = p_userdata;
	tex->detect_3d_callback = p_callback;
}

void RendererRD::TextureStorage::texture_set_detect_normal_callback(
	RID p_texture, RenderingServerTypes::TextureDetectCallback p_callback, void* p_userdata)
{
	Texture* tex = texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL(tex);

	tex->detect_normal_callback_ud = p_userdata;
	tex->detect_normal_callback = p_callback;
}

void RendererRD::TextureStorage::texture_set_detect_roughness_callback(RID p_texture,
	RenderingServerTypes::TextureDetectRoughnessCallback p_callback, void* p_userdata)
{
	Texture* tex = texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL(tex);

	tex->detect_roughness_callback_ud = p_userdata;
	tex->detect_roughness_callback = p_callback;
}

void RendererRD::TextureStorage::texture_debug_usage(
	List<RenderingServerTypes::TextureInfo>* r_info)
{
	for (const RID& rid : texture_owner.get_owned_list()) {
		Texture* t = texture_owner.get_or_null(rid);
		if (!t) {
			continue;
		}
		RenderingServerTypes::TextureInfo tinfo;
		tinfo.path = t->path;
		tinfo.format = t->format;
		tinfo.width = t->width;
		tinfo.height = t->height;
		tinfo.bytes = Image::get_image_data_size(t->width, t->height, t->format, t->mipmaps > 1);
		tinfo.type = static_cast<RSE::TextureType>(t->type);

		switch (t->type) {
		case TextureType::TYPE_3D:
			tinfo.depth = t->depth;
			tinfo.bytes *= t->depth;
			break;

		case TextureType::TYPE_LAYERED:
			tinfo.depth = t->layers;
			tinfo.bytes *= t->layers;
			break;

		default:
			tinfo.depth = 0;
			break;
		}

		r_info->push_back(tinfo);
	}
}

void RendererRD::TextureStorage::texture_set_force_redraw_if_visible(RID p_texture, bool p_enable)
{
}

Size2 RendererRD::TextureStorage::texture_size_with_proxy(RID p_proxy)
{
	return texture_2d_get_size(p_proxy);
}

void RendererRD::TextureStorage::texture_rd_initialize(
	RID p_texture, const RID& p_rd_texture, const RSE::TextureLayeredType p_layer_type)
{
	ERR_FAIL_COND(!RD::texture_is_valid(p_rd_texture));

	// TODO : investigate if we can support this, will need to be able to obtain the order and
	// obtain the slice info
	ERR_FAIL_COND_MSG(RD::texture_is_shared(p_rd_texture),
		"Please create the texture object using the original texture");

	RDC::TextureFormat tf = RD::texture_get_format(p_rd_texture);
	ERR_FAIL_COND(!(tf.usage_bits & RDC::TEXTURE_USAGE_SAMPLING_BIT));

	TextureFromRDFormat imfmt;
	_texture_format_from_rd(tf.format, imfmt);
	ERR_FAIL_COND(imfmt.image_format == Image::FORMAT_MAX);

	Texture texture;

	switch (tf.texture_type) {
	case RDC::TEXTURE_TYPE_2D: {
		ERR_FAIL_COND(tf.array_layers != 1);
		texture.type = RendererRD::TextureStorage::TYPE_2D;
	} break;
	case RDC::TEXTURE_TYPE_2D_ARRAY:
	case RDC::TEXTURE_TYPE_CUBE:
	case RDC::TEXTURE_TYPE_CUBE_ARRAY: {
		// RenderingDevice doesn't distinguish between Array textures and Cube textures
		// this condition covers TextureArrays, TextureCube, and TextureCubeArray.
		ERR_FAIL_COND(tf.array_layers == 1);
		texture.type = RendererRD::TextureStorage::TYPE_LAYERED;
		texture.layered_type = p_layer_type;
	} break;
	case RDC::TEXTURE_TYPE_3D: {
		ERR_FAIL_COND(tf.array_layers != 1);
		texture.type = RendererRD::TextureStorage::TYPE_3D;
	} break;
	default: {
		ERR_FAIL_MSG("This RD texture can't be used as a render texture");
	} break;
	}

	texture.width = tf.width;
	texture.height = tf.height;
	texture.depth = tf.depth;
	texture.layers = tf.array_layers;
	texture.mipmaps = tf.mipmaps;
	texture.format = imfmt.image_format;
	texture.validated_format = texture.format; // ??

	RD::TextureView rd_view;
	rd_view.format_override = imfmt.rd_format == tf.format ? RDC::DATA_FORMAT_MAX : imfmt.rd_format;
	rd_view.swizzle_r = imfmt.swizzle_r;
	rd_view.swizzle_g = imfmt.swizzle_g;
	rd_view.swizzle_b = imfmt.swizzle_b;
	rd_view.swizzle_a = imfmt.swizzle_a;

	texture.rd_type = tf.texture_type;
	texture.rd_format = imfmt.rd_format;
	texture.rd_format_srgb = imfmt.rd_format_srgb;
	texture.rd_view = rd_view;

	if (imfmt.rd_format_srgb != RDC::DATA_FORMAT_MAX) {
		// Allow creating a shared sRGB texture even if users didn't explicitly add them.
		RD::_texture_ensure_shareable_format(p_rd_texture, texture.rd_format);
		RD::_texture_ensure_shareable_format(p_rd_texture, texture.rd_format_srgb);
	}

	// We create a shared texture here even if our view matches, so we don't obtain ownership.
	texture.rd_texture = RD::texture_create_shared(rd_view, p_rd_texture);
	if (imfmt.rd_format_srgb != RDC::DATA_FORMAT_MAX) {
		// The texture supports sRGB override, create it for 3D usage.
		rd_view.format_override =
			imfmt.rd_format_srgb == tf.format ? RDC::DATA_FORMAT_MAX : imfmt.rd_format_srgb;
		texture.rd_format_srgb = imfmt.rd_format_srgb;
		texture.rd_texture_srgb = RD::texture_create_shared(rd_view, p_rd_texture);
	}

	// TODO figure out what to do with slices

	texture.width_2d = texture.width;
	texture.height_2d = texture.height;
	texture.is_render_target = false;
	texture.is_proxy = false;

	texture_owner.initialize_rid(p_texture, texture);
}

RID RendererRD::TextureStorage::texture_get_rd_texture(RID p_texture, bool p_srgb)
{
	if (p_texture.is_null()) {
		return RID();
	}

	Texture* tex = texture_owner.get_or_null(p_texture);
	if (!tex) {
		return RID();
	}

	return (p_srgb && tex->rd_texture_srgb.is_valid()) ? tex->rd_texture_srgb : tex->rd_texture;
}

uint64_t RendererRD::TextureStorage::texture_get_native_handle(RID p_texture, bool p_srgb)
{
	Texture* tex = texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL_V(tex, 0);

	if (p_srgb && tex->rd_texture_srgb.is_valid()) {
		return RD::get_driver_resource(RDC::DRIVER_RESOURCE_TEXTURE, tex->rd_texture_srgb);
	}
	else {
		return RD::get_driver_resource(RDC::DRIVER_RESOURCE_TEXTURE, tex->rd_texture);
	}
}

void RendererRD::TextureStorage::_texture_format_from_rd(
	RDC::DataFormat p_rd_format, TextureFromRDFormat& r_format)
{
	switch (p_rd_format) {
	case RDC::DATA_FORMAT_R8_UNORM: {
		r_format.image_format = Image::FORMAT_L8;
		r_format.rd_format = RDC::DATA_FORMAT_R8_UNORM;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_ONE;
	} break; // luminance
	case RDC::DATA_FORMAT_R8G8_UNORM: {
		r_format.image_format = Image::FORMAT_LA8;
		r_format.rd_format = RDC::DATA_FORMAT_R8G8_UNORM;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_G;
	} break; // luminance-alpha
	/* already maps to L8/LA8
	case RD::DATA_FORMAT_R8_UNORM: {
		r_format.image_format = Image::FORMAT_R8;
		r_format.rd_format = RD::DATA_FORMAT_R8_UNORM;
		r_format.swizzle_r = RD::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RD::TEXTURE_SWIZZLE_ZERO;
		r_format.swizzle_b = RD::TEXTURE_SWIZZLE_ZERO;
		r_format.swizzle_a = RD::TEXTURE_SWIZZLE_ONE;
	} break;
	case RD::DATA_FORMAT_R8G8_UNORM: {
		r_format.image_format = Image::FORMAT_RG8;
		r_format.rd_format = RD::DATA_FORMAT_R8G8_UNORM;
		r_format.swizzle_r = RD::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RD::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RD::TEXTURE_SWIZZLE_ZERO;
		r_format.swizzle_a = RD::TEXTURE_SWIZZLE_ONE;
	} break;
	*/
	case RDC::DATA_FORMAT_R8G8B8_UNORM:
	case RDC::DATA_FORMAT_R8G8B8_SRGB: {
		r_format.image_format = Image::FORMAT_RGB8;
		r_format.rd_format = RDC::DATA_FORMAT_R8G8B8_UNORM;
		r_format.rd_format_srgb = RDC::DATA_FORMAT_R8G8B8_SRGB;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_B;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_ONE;

	} break;
	case RDC::DATA_FORMAT_R8G8B8A8_UNORM:
	case RDC::DATA_FORMAT_R8G8B8A8_SRGB: {
		r_format.image_format = Image::FORMAT_RGBA8;
		r_format.rd_format = RDC::DATA_FORMAT_R8G8B8A8_UNORM;
		r_format.rd_format_srgb = RDC::DATA_FORMAT_R8G8B8A8_SRGB;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_B;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_A;
	} break;
	case RDC::DATA_FORMAT_B8G8R8A8_UNORM:
	case RDC::DATA_FORMAT_B8G8R8A8_SRGB: {
		r_format.image_format = Image::FORMAT_RGBA8;
		r_format.rd_format = RDC::DATA_FORMAT_B8G8R8A8_UNORM;
		r_format.rd_format_srgb = RDC::DATA_FORMAT_B8G8R8A8_SRGB;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_B;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_A;
	} break;
	case RDC::DATA_FORMAT_B4G4R4A4_UNORM_PACK16: {
		r_format.image_format = Image::FORMAT_RGBA4444;
		r_format.rd_format = RDC::DATA_FORMAT_B4G4R4A4_UNORM_PACK16;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_B; // needs swizzle
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_A;
	} break;
	case RDC::DATA_FORMAT_B5G6R5_UNORM_PACK16: {
		r_format.image_format = Image::FORMAT_RGB565;
		r_format.rd_format = RDC::DATA_FORMAT_B5G6R5_UNORM_PACK16;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_B;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_A;
	} break;
	case RDC::DATA_FORMAT_R32_SFLOAT: {
		r_format.image_format = Image::FORMAT_RF;
		r_format.rd_format = RDC::DATA_FORMAT_R32_SFLOAT;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_ZERO;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_ZERO;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_ONE;
	} break; // float
	case RDC::DATA_FORMAT_R32G32_SFLOAT: {
		r_format.image_format = Image::FORMAT_RGF;
		r_format.rd_format = RDC::DATA_FORMAT_R32G32_SFLOAT;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_ZERO;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_ONE;
	} break;
	case RDC::DATA_FORMAT_R32G32B32_SFLOAT: {
		r_format.image_format = Image::FORMAT_RGBF;
		r_format.rd_format = RDC::DATA_FORMAT_R32G32B32_SFLOAT;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_B;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_ONE;
	} break;
	case RDC::DATA_FORMAT_R32G32B32A32_SFLOAT: {
		r_format.image_format = Image::FORMAT_RGBAF;
		r_format.rd_format = RDC::DATA_FORMAT_R32G32B32A32_SFLOAT;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_B;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_A;

	} break;
	case RDC::DATA_FORMAT_R16_SFLOAT: {
		r_format.image_format = Image::FORMAT_RH;
		r_format.rd_format = RDC::DATA_FORMAT_R16_SFLOAT;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_ZERO;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_ZERO;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_ONE;

	} break; // half float
	case RDC::DATA_FORMAT_R16G16_SFLOAT: {
		r_format.image_format = Image::FORMAT_RGH;
		r_format.rd_format = RDC::DATA_FORMAT_R16G16_SFLOAT;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_ZERO;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_ONE;

	} break;
	case RDC::DATA_FORMAT_R16G16B16_SFLOAT: {
		r_format.image_format = Image::FORMAT_RGBH;
		r_format.rd_format = RDC::DATA_FORMAT_R16G16B16_SFLOAT;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_B;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_ONE;
	} break;
	case RDC::DATA_FORMAT_R16G16B16A16_SFLOAT: {
		r_format.image_format = Image::FORMAT_RGBAH;
		r_format.rd_format = RDC::DATA_FORMAT_R16G16B16A16_SFLOAT;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_B;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_A;

	} break;
	case RDC::DATA_FORMAT_E5B9G9R9_UFLOAT_PACK32: {
		r_format.image_format = Image::FORMAT_RGBE9995;
		r_format.rd_format = RDC::DATA_FORMAT_E5B9G9R9_UFLOAT_PACK32;
		// TODO: Need to make a function in Image to swap bits for this.
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_IDENTITY;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_IDENTITY;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_IDENTITY;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_IDENTITY;
	} break;
	case RDC::DATA_FORMAT_BC1_RGB_UNORM_BLOCK:
	case RDC::DATA_FORMAT_BC1_RGB_SRGB_BLOCK: {
		r_format.image_format = Image::FORMAT_DXT1;
		r_format.rd_format = RDC::DATA_FORMAT_BC1_RGB_UNORM_BLOCK;
		r_format.rd_format_srgb = RDC::DATA_FORMAT_BC1_RGB_SRGB_BLOCK;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_B;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_ONE;

	} break; // s3tc bc1
	case RDC::DATA_FORMAT_BC2_UNORM_BLOCK:
	case RDC::DATA_FORMAT_BC2_SRGB_BLOCK: {
		r_format.image_format = Image::FORMAT_DXT3;
		r_format.rd_format = RDC::DATA_FORMAT_BC2_UNORM_BLOCK;
		r_format.rd_format_srgb = RDC::DATA_FORMAT_BC2_SRGB_BLOCK;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_B;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_A;

	} break; // bc2
	case RDC::DATA_FORMAT_BC3_UNORM_BLOCK:
	case RDC::DATA_FORMAT_BC3_SRGB_BLOCK: {
		r_format.image_format = Image::FORMAT_DXT5;
		r_format.rd_format = RDC::DATA_FORMAT_BC3_UNORM_BLOCK;
		r_format.rd_format_srgb = RDC::DATA_FORMAT_BC3_SRGB_BLOCK;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_B;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_A;
	} break; // bc3
	case RDC::DATA_FORMAT_BC4_UNORM_BLOCK: {
		r_format.image_format = Image::FORMAT_RGTC_R;
		r_format.rd_format = RDC::DATA_FORMAT_BC4_UNORM_BLOCK;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_ZERO;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_ZERO;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_ONE;

	} break;
	case RDC::DATA_FORMAT_BC5_UNORM_BLOCK: {
		r_format.image_format = Image::FORMAT_RGTC_RG;
		r_format.rd_format = RDC::DATA_FORMAT_BC5_UNORM_BLOCK;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_ZERO;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_ONE;

	} break;
	case RDC::DATA_FORMAT_BC7_UNORM_BLOCK:
	case RDC::DATA_FORMAT_BC7_SRGB_BLOCK: {
		r_format.image_format = Image::FORMAT_BPTC_RGBA;
		r_format.rd_format = RDC::DATA_FORMAT_BC7_UNORM_BLOCK;
		r_format.rd_format_srgb = RDC::DATA_FORMAT_BC7_SRGB_BLOCK;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_B;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_A;

	} break; // btpc bc7
	case RDC::DATA_FORMAT_BC6H_SFLOAT_BLOCK: {
		r_format.image_format = Image::FORMAT_BPTC_RGBF;
		r_format.rd_format = RDC::DATA_FORMAT_BC6H_SFLOAT_BLOCK;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_B;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_ONE;
	} break; // float bc6h
	case RDC::DATA_FORMAT_BC6H_UFLOAT_BLOCK: {
		r_format.image_format = Image::FORMAT_BPTC_RGBFU;
		r_format.rd_format = RDC::DATA_FORMAT_BC6H_UFLOAT_BLOCK;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_B;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_ONE;
	} break; // unsigned float bc6hu
	case RDC::DATA_FORMAT_EAC_R11_UNORM_BLOCK: {
		r_format.image_format = Image::FORMAT_ETC2_R11;
		r_format.rd_format = RDC::DATA_FORMAT_EAC_R11_UNORM_BLOCK;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_ZERO;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_ZERO;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_ONE;

	} break; // etc2
	case RDC::DATA_FORMAT_EAC_R11_SNORM_BLOCK: {
		r_format.image_format = Image::FORMAT_ETC2_R11S;
		r_format.rd_format = RDC::DATA_FORMAT_EAC_R11_SNORM_BLOCK;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_ZERO;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_ZERO;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_ONE;
	} break; // signed: {} break; NOT srgb.
	case RDC::DATA_FORMAT_EAC_R11G11_UNORM_BLOCK: {
		r_format.image_format = Image::FORMAT_ETC2_RG11;
		r_format.rd_format = RDC::DATA_FORMAT_EAC_R11G11_UNORM_BLOCK;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_ZERO;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_ONE;
	} break;
	case RDC::DATA_FORMAT_EAC_R11G11_SNORM_BLOCK: {
		r_format.image_format = Image::FORMAT_ETC2_RG11S;
		r_format.rd_format = RDC::DATA_FORMAT_EAC_R11G11_SNORM_BLOCK;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_ZERO;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_ONE;
	} break;
	case RDC::DATA_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
	case RDC::DATA_FORMAT_ETC2_R8G8B8_SRGB_BLOCK: {
		r_format.image_format = Image::FORMAT_ETC2_RGB8;
		r_format.rd_format = RDC::DATA_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
		r_format.rd_format_srgb = RDC::DATA_FORMAT_ETC2_R8G8B8_SRGB_BLOCK;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_B;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_ONE;

	} break;
	case RDC::DATA_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
	case RDC::DATA_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK: {
		r_format.image_format = Image::FORMAT_ETC2_RGBA8;
		r_format.rd_format = RDC::DATA_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;
		r_format.rd_format_srgb = RDC::DATA_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_B;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_A;
	} break;
	case RDC::DATA_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
	case RDC::DATA_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK: {
		r_format.image_format = Image::FORMAT_ETC2_RGB8A1;
		r_format.rd_format = RDC::DATA_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK;
		r_format.rd_format_srgb = RDC::DATA_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_B;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_A;
	} break;
	/* already maps to FORMAT_ETC2_RGBA8
	case RD::DATA_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
	case RD::DATA_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK: {
		r_format.image_format = Image::FORMAT_ETC2_RA_AS_RG;
		r_format.rd_format = RD::DATA_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;
		r_format.rd_format_srgb = RD::DATA_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK;
		r_format.swizzle_r = RD::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RD::TEXTURE_SWIZZLE_A;
		r_format.swizzle_b = RD::TEXTURE_SWIZZLE_ZERO;
		r_format.swizzle_a = RD::TEXTURE_SWIZZLE_ONE;
	} break;*/
	/* already maps to FORMAT_DXT5
	case RD::DATA_FORMAT_BC3_UNORM_BLOCK:
	case RD::DATA_FORMAT_BC3_SRGB_BLOCK: {
		r_format.image_format = Image::FORMAT_DXT5_RA_AS_RG;
		r_format.rd_format = RD::DATA_FORMAT_BC3_UNORM_BLOCK;
		r_format.rd_format_srgb = RD::DATA_FORMAT_BC3_SRGB_BLOCK;
		r_format.swizzle_r = RD::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RD::TEXTURE_SWIZZLE_A;
		r_format.swizzle_b = RD::TEXTURE_SWIZZLE_ZERO;
		r_format.swizzle_a = RD::TEXTURE_SWIZZLE_ONE;
	} break;
	*/
	case RDC::DATA_FORMAT_ASTC_4x4_UNORM_BLOCK: {
		// Q: Do we do as we do below, just create the sRGB variant?
		r_format.image_format = Image::FORMAT_ASTC_4x4;
		r_format.rd_format = RDC::DATA_FORMAT_ASTC_4x4_UNORM_BLOCK;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_B;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_A;
	} break;
	case RDC::DATA_FORMAT_ASTC_4x4_SRGB_BLOCK: {
		r_format.image_format = Image::FORMAT_ASTC_4x4;
		r_format.rd_format = RDC::DATA_FORMAT_ASTC_4x4_UNORM_BLOCK;
		r_format.rd_format_srgb = RDC::DATA_FORMAT_ASTC_4x4_SRGB_BLOCK;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_B;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_A;

	} break;
	case RDC::DATA_FORMAT_ASTC_4x4_SFLOAT_BLOCK: {
		r_format.image_format = Image::FORMAT_ASTC_4x4_HDR;
		r_format.rd_format = RDC::DATA_FORMAT_ASTC_4x4_SFLOAT_BLOCK;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_B;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_A;
	} break; // astc 4x4
	case RDC::DATA_FORMAT_ASTC_6x6_UNORM_BLOCK: {
		r_format.image_format = Image::FORMAT_ASTC_6x6;
		r_format.rd_format = RDC::DATA_FORMAT_ASTC_6x6_UNORM_BLOCK;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_B;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_A;
	} break;
	case RDC::DATA_FORMAT_ASTC_6x6_SRGB_BLOCK: {
		r_format.image_format = Image::FORMAT_ASTC_6x6;
		r_format.rd_format = RDC::DATA_FORMAT_ASTC_6x6_UNORM_BLOCK;
		r_format.rd_format_srgb = RDC::DATA_FORMAT_ASTC_6x6_SRGB_BLOCK;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_B;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_A;
	} break;
	case RDC::DATA_FORMAT_ASTC_6x6_SFLOAT_BLOCK: {
		r_format.image_format = Image::FORMAT_ASTC_6x6_HDR;
		r_format.rd_format = RDC::DATA_FORMAT_ASTC_6x6_SFLOAT_BLOCK;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_B;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_A;
	} break; // astc 8x8
	case RDC::DATA_FORMAT_ASTC_8x8_UNORM_BLOCK: {
		// Q: Do we do as we do below, just create the sRGB variant?
		r_format.image_format = Image::FORMAT_ASTC_8x8;
		r_format.rd_format = RDC::DATA_FORMAT_ASTC_8x8_UNORM_BLOCK;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_B;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_A;
	} break;
	case RDC::DATA_FORMAT_ASTC_8x8_SRGB_BLOCK: {
		r_format.image_format = Image::FORMAT_ASTC_8x8;
		r_format.rd_format = RDC::DATA_FORMAT_ASTC_8x8_UNORM_BLOCK;
		r_format.rd_format_srgb = RDC::DATA_FORMAT_ASTC_8x8_SRGB_BLOCK;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_B;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_A;
	} break;
	case RDC::DATA_FORMAT_ASTC_8x8_SFLOAT_BLOCK: {
		r_format.image_format = Image::FORMAT_ASTC_8x8_HDR;
		r_format.rd_format = RDC::DATA_FORMAT_ASTC_8x8_SFLOAT_BLOCK;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_B;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_A;
	} break; // astc 8x8
	case RDC::DATA_FORMAT_D16_UNORM: {
		r_format.image_format = Image::FORMAT_R16;
		r_format.rd_format = RDC::DATA_FORMAT_D16_UNORM;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_ZERO;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_ZERO;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_ONE;
	} break;
	case RDC::DATA_FORMAT_D32_SFLOAT: {
		r_format.image_format = Image::FORMAT_RF;
		r_format.rd_format = RDC::DATA_FORMAT_D32_SFLOAT;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_ZERO;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_ZERO;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_ONE;
	} break;
	case RDC::DATA_FORMAT_R16_UNORM: {
		r_format.image_format = Image::FORMAT_R16;
		r_format.rd_format = RDC::DATA_FORMAT_R16_UNORM;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_ZERO;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_ZERO;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_ONE;
	} break; // unorm16
	case RDC::DATA_FORMAT_R16G16_UNORM: {
		r_format.image_format = Image::FORMAT_RG16;
		r_format.rd_format = RDC::DATA_FORMAT_R16G16_UNORM;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_ZERO;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_ONE;
	} break;
	case RDC::DATA_FORMAT_R16G16B16_UNORM: {
		r_format.image_format = Image::FORMAT_RGB16;
		r_format.rd_format = RDC::DATA_FORMAT_R16G16B16_UNORM;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_B;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_ONE;
	} break;
	case RDC::DATA_FORMAT_R16G16B16A16_UNORM: {
		r_format.image_format = Image::FORMAT_RGBA16;
		r_format.rd_format = RDC::DATA_FORMAT_R16G16B16A16_UNORM;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_B;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_A;
	} break;
	case RDC::DATA_FORMAT_R16_UINT: {
		r_format.image_format = Image::FORMAT_R16I;
		r_format.rd_format = RDC::DATA_FORMAT_R16_UINT;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_ZERO;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_ZERO;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_ONE;
	} break; // uint16
	case RDC::DATA_FORMAT_R16G16_UINT: {
		r_format.image_format = Image::FORMAT_RG16I;
		r_format.rd_format = RDC::DATA_FORMAT_R16G16_UINT;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_ZERO;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_ONE;
	} break;
	case RDC::DATA_FORMAT_R16G16B16_UINT: {
		r_format.image_format = Image::FORMAT_RGB16I;
		r_format.rd_format = RDC::DATA_FORMAT_R16G16B16_UINT;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_B;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_ONE;
	} break;
	case RDC::DATA_FORMAT_R16G16B16A16_UINT: {
		r_format.image_format = Image::FORMAT_RGBA16I;
		r_format.rd_format = RDC::DATA_FORMAT_R16G16B16A16_UINT;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_B;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_A;
	} break;

	default: {
		ERR_FAIL_MSG("Unsupported image format");
	}
	}
}

RID RendererRD::TextureStorage::area_light_atlas_get_texture() { return area_light_atlas.texture; }

void RendererRD::TextureStorage::area_light_atlas_mark_dirty_on_texture(RID p_texture)
{
	if (area_light_atlas.textures.has(p_texture)) {
		// belongs to area light atlas..

		area_light_atlas.dirty = true; // mark it dirty since it was most likely modified
	}
}

void RendererRD::TextureStorage::area_light_atlas_remove_texture(RID p_texture)
{
	if (area_light_atlas.textures.has(p_texture)) {
		area_light_atlas.textures.erase(p_texture);
	}
}

void RendererRD::TextureStorage::update_area_light_atlas()
{
	ERR_FAIL_COND(!CopyEffects::is_initialized());

	if (!area_light_atlas.dirty) {
		return; // nothing to do
	}

	area_light_atlas.dirty = false;

	if (area_light_atlas.texture.is_valid()) {
		RD::free_rid(area_light_atlas.texture);
		area_light_atlas.texture = RID();
		area_light_atlas.texture_mipmaps.clear();
	}

	int border = 1 << (area_light_atlas.mipmaps - 1);

	if (area_light_atlas.textures.size()) {
		// generate atlas
		Vector<AreaLightAtlas::SortItem> itemsv;
		itemsv.resize(area_light_atlas.textures.size());
		uint32_t base_size = 1;

		int idx = 0;

		for (const KeyValue<RID, AreaLightAtlas::Texture>& E : area_light_atlas.textures) {
			AreaLightAtlas::SortItem& si = itemsv.write[idx];

			Texture* src_tex = get_texture(E.key);
			Vector2i b_size = Vector2i(Math::ceil(float(src_tex->width) / border),
				Math::ceil(float(src_tex->height) / border));
			si.size.width = b_size.width + 1;
			si.size.height = b_size.height + 1;

			si.pixel_size =
				b_size * border; // components are either small powers of 2 or N * border
			if (src_tex->width < border) {
				si.pixel_size.width = Math::nearest_power_of_2_templated(src_tex->width);
			}
			if (src_tex->height < border) {
				si.pixel_size.height = Math::nearest_power_of_2_templated(src_tex->height);
			}

			if (base_size < (uint32_t)si.size.width) {
				base_size = Math::nearest_power_of_2_templated(si.size.width);
			}

			si.texture = E.key;
			idx++;
		}

		// sort items by size
		itemsv.sort();

		// attempt to create atlas
		int item_count = itemsv.size();
		AreaLightAtlas::SortItem* items = itemsv.ptrw();

		int atlas_height = 0;

		while (true) {
			Vector<int> v_offsetsv;
			v_offsetsv.resize(base_size);

			int* v_offsets = v_offsetsv.ptrw();
			memset(v_offsets, 0, sizeof(int) * base_size);

			int max_height = 0;

			for (int i = 0; i < item_count; i++) {
				// best fit
				AreaLightAtlas::SortItem& si = items[i];
				int best_idx = -1;
				int best_height = 0x7FFFFFFF;
				for (uint32_t j = 0; j <= base_size - si.size.width; j++) {
					int height = 0;
					for (int k = 0; k < si.size.width; k++) {
						int h = v_offsets[k + j];
						if (h > height) {
							height = h;
							if (height > best_height) {
								break; // already bad
							}
						}
					}

					if (height < best_height) {
						best_height = height;
						best_idx = j;
					}
				}

				// update
				for (int k = 0; k < si.size.width; k++) {
					v_offsets[k + best_idx] = best_height + si.size.height;
				}

				si.pos.x = best_idx;
				si.pos.y = best_height;

				if (si.pos.y + si.size.height > max_height) {
					max_height = si.pos.y + si.size.height;
				}
			}

			if ((uint32_t)max_height <= base_size * 2) {
				atlas_height = max_height;
				break; // good ratio, break;
			}

			base_size *= 2;
		}

		area_light_atlas.size.width = base_size * border;
		area_light_atlas.size.height = Math::nearest_power_of_2_templated(atlas_height * border);

		for (int i = 0; i < item_count; i++) {
			AreaLightAtlas::Texture* t = area_light_atlas.textures.getptr(items[i].texture);
			t->uv_rect.position = items[i].pos * border;
			t->uv_rect.position /= Size2(area_light_atlas.size);
			t->uv_rect.size = items[i].pixel_size;
			t->uv_rect.size /= Size2(area_light_atlas.size);
		}
	}
	else {
		// use border as size, so it at least has enough mipmaps
		area_light_atlas.size.width = border;
		area_light_atlas.size.height = border;
	}

	// blit textures

	RDC::TextureFormat tformat;
	tformat.format = RDC::DATA_FORMAT_R8G8B8A8_UNORM;
	tformat.width = area_light_atlas.size.width;
	tformat.height = area_light_atlas.size.height;
	tformat.usage_bits = RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT |
						 RDC::TEXTURE_USAGE_CAN_COPY_TO_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT;
	tformat.texture_type = RDC::TEXTURE_TYPE_2D;
	tformat.mipmaps = area_light_atlas.mipmaps;
	tformat.shareable_formats.push_back(RDC::DATA_FORMAT_R8G8B8A8_UNORM);

	area_light_atlas.texture = RD::texture_create(tformat, RDD::TextureView());
	RD::texture_clear(
		area_light_atlas.texture, Color(0, 0, 0, 0), 0, area_light_atlas.mipmaps, 0, 1);

	{
		// create the framebuffer

		Size2i s = area_light_atlas.size;

		for (int i = 0; i < area_light_atlas.mipmaps; i++) {
			AreaLightAtlas::MipMap mm;
			mm.texture = RD::texture_create_shared_from_slice(
				RD::TextureView(), area_light_atlas.texture, 0, i);
			Vector<RID> fb;
			fb.push_back(mm.texture);
			mm.fb = RD::framebuffer_create(fb);
			mm.size = s;
			area_light_atlas.texture_mipmaps.push_back(mm);

			s = Vector2i(s.width >> 1, s.height >> 1).maxi(1);
		}
	}

	Color clear_color(0, 0, 0, 0);
	if (area_light_atlas.textures.is_empty()) {
		for (int i = 0; i < area_light_atlas.texture_mipmaps.size(); i++) {
			const AreaLightAtlas::MipMap& mm = area_light_atlas.texture_mipmaps[i];
			RD::texture_clear(mm.texture, clear_color, 0, 1, 0, 1);
		}
	}
}

void RendererRD::TextureStorage::texture_add_to_area_light_atlas(RID p_texture)
{
	if (!area_light_atlas.textures.has(p_texture)) {
		AreaLightAtlas::Texture t;
		t.users = 1;
		area_light_atlas.textures[p_texture] = t;
		area_light_atlas.dirty = true;
	}
	else {
		AreaLightAtlas::Texture* t = area_light_atlas.textures.getptr(p_texture);
		t->users++;
	}
}

void RendererRD::TextureStorage::texture_remove_from_area_light_atlas(RID p_texture)
{
	AreaLightAtlas::Texture* t = area_light_atlas.textures.getptr(p_texture);
	ERR_FAIL_NULL(t);
	t->users--;
	if (t->users == 0) {
		area_light_atlas.textures.erase(p_texture);
	}
}

RID RendererRD::TextureStorage::decal_atlas_get_texture() { return decal_atlas.texture; }

RID RendererRD::TextureStorage::decal_atlas_get_texture_srgb() { return decal_atlas.texture_srgb; }

RID RendererRD::TextureStorage::decal_allocate() { return decal_owner.allocate_rid(); }

void RendererRD::TextureStorage::decal_initialize(RID p_decal)
{
	decal_owner.initialize_rid(p_decal, Decal());
}

void RendererRD::TextureStorage::decal_free(RID p_rid)
{
	Decal* decal = decal_owner.get_or_null(p_rid);
	for (int i = 0; i < RSE::DECAL_TEXTURE_MAX; i++) {
		if (decal->textures[i].is_valid() && owns_texture(decal->textures[i])) {
			texture_remove_from_decal_atlas(decal->textures[i]);
		}
	}
	decal->dependency.deleted_notify(p_rid);
	decal_owner.free(p_rid);
}

void RendererRD::TextureStorage::decal_set_size(RID p_decal, const Vector3& p_size)
{
	Decal* decal = decal_owner.get_or_null(p_decal);
	ERR_FAIL_NULL(decal);
	decal->size = p_size;
	decal->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_AABB);
}

void RendererRD::TextureStorage::decal_set_texture(
	RID p_decal, RSE::DecalTexture p_type, RID p_texture)
{
	Decal* decal = decal_owner.get_or_null(p_decal);
	ERR_FAIL_NULL(decal);
	ERR_FAIL_INDEX(p_type, RSE::DECAL_TEXTURE_MAX);

	if (decal->textures[p_type] == p_texture) {
		return;
	}

	ERR_FAIL_COND(p_texture.is_valid() && !owns_texture(p_texture));

	if (decal->textures[p_type].is_valid() && owns_texture(decal->textures[p_type])) {
		texture_remove_from_decal_atlas(decal->textures[p_type]);
	}

	decal->textures[p_type] = p_texture;

	if (decal->textures[p_type].is_valid()) {
		texture_add_to_decal_atlas(decal->textures[p_type]);
	}

	decal->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_DECAL);
}

void RendererRD::TextureStorage::decal_set_emission_energy(RID p_decal, float p_energy)
{
	Decal* decal = decal_owner.get_or_null(p_decal);
	ERR_FAIL_NULL(decal);
	decal->emission_energy = p_energy;
}

void RendererRD::TextureStorage::decal_set_albedo_mix(RID p_decal, float p_mix)
{
	Decal* decal = decal_owner.get_or_null(p_decal);
	ERR_FAIL_NULL(decal);
	decal->albedo_mix = p_mix;
}

void RendererRD::TextureStorage::decal_set_modulate(RID p_decal, const Color& p_modulate)
{
	Decal* decal = decal_owner.get_or_null(p_decal);
	ERR_FAIL_NULL(decal);
	decal->modulate = p_modulate;
}

void RendererRD::TextureStorage::decal_set_cull_mask(RID p_decal, uint32_t p_layers)
{
	Decal* decal = decal_owner.get_or_null(p_decal);
	ERR_FAIL_NULL(decal);
	decal->cull_mask = p_layers;
	decal->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_CULL_MASK);
}

void RendererRD::TextureStorage::decal_set_distance_fade(
	RID p_decal, bool p_enabled, float p_begin, float p_length)
{
	Decal* decal = decal_owner.get_or_null(p_decal);
	ERR_FAIL_NULL(decal);
	decal->distance_fade = p_enabled;
	decal->distance_fade_begin = p_begin;
	decal->distance_fade_length = p_length;
}

void RendererRD::TextureStorage::decal_set_fade(RID p_decal, float p_above, float p_below)
{
	Decal* decal = decal_owner.get_or_null(p_decal);
	ERR_FAIL_NULL(decal);
	decal->upper_fade = p_above;
	decal->lower_fade = p_below;
}

void RendererRD::TextureStorage::decal_set_normal_fade(RID p_decal, float p_fade)
{
	Decal* decal = decal_owner.get_or_null(p_decal);
	ERR_FAIL_NULL(decal);
	decal->normal_fade = p_fade;
}

void RendererRD::TextureStorage::decal_atlas_mark_dirty_on_texture(RID p_texture)
{
	if (decal_atlas.textures.has(p_texture)) {
		// belongs to decal atlas..

		decal_atlas.dirty = true; // mark it dirty since it was most likely modified
	}
}

void RendererRD::TextureStorage::decal_atlas_remove_texture(RID p_texture)
{
	if (decal_atlas.
textures.has(p_texture)) {
		decal_atlas.textures.erase(p_texture);
		// there is not much a point of making it dirty, just let it be.
	}
}

AABB RendererRD::TextureStorage::decal_get_aabb(RID p_decal)
{
	Decal* decal = decal_owner.get_or_null(p_decal);
	ERR_FAIL_NULL_V(decal, AABB());

	return AABB(-decal->size / 2, decal->size);
}

uint32_t RendererRD::TextureStorage::decal_get_cull_mask(RID p_decal)
{
	Decal* decal = decal_owner.get_or_null(p_decal);
	ERR_FAIL_NULL_V(decal, 0);

	return decal->cull_mask;
}

Dependency* RendererRD::TextureStorage::decal_get_dependency(RID p_decal)
{
	Decal* decal = decal_owner.get_or_null(p_decal);
	ERR_FAIL_NULL_V(decal, nullptr);

	return &decal->dependency;
}

void RendererRD::TextureStorage::update_decal_atlas()
{
	ERR_FAIL_COND(!CopyEffects::is_initialized());

	if (!decal_atlas.dirty) {
		return; // nothing to do
	}

	decal_atlas.dirty = false;

	if (decal_atlas.texture.is_valid()) {
		RD::free_rid(decal_atlas.texture);
		decal_atlas.texture = RID();
		decal_atlas.texture_srgb = RID();
		decal_atlas.texture_mipmaps.clear();
	}

	int border = 1 << decal_atlas.mipmaps;

	if (decal_atlas.textures.size()) {
		// generate atlas
		Vector<DecalAtlas::SortItem> itemsv;
		itemsv.resize(decal_atlas.textures.size());
		uint32_t base_size = 8;

		int idx = 0;

		for (const KeyValue<RID, DecalAtlas::Texture>& E : decal_atlas.textures) {
			DecalAtlas::SortItem& si = itemsv.write[idx];

			Texture* src_tex = get_texture(E.key);

			si.size.width = (src_tex->width / border) + 1;
			si.size.height = (src_tex->height / border) + 1;
			si.pixel_size = Size2i(src_tex->width, src_tex->height);

			if (base_size < (uint32_t)si.size.width) {
				base_size = Math::nearest_power_of_2_templated(si.size.width);
			}

			si.texture = E.key;
			idx++;
		}

		// sort items by size
		itemsv.sort();

		// attempt to create atlas
		int item_count = itemsv.size();
		DecalAtlas::SortItem* items = itemsv.ptrw();

		int atlas_height = 0;

		while (true) {
			Vector<int> v_offsetsv;
			v_offsetsv.resize(base_size);

			int* v_offsets = v_offsetsv.ptrw();
			memset(v_offsets, 0, sizeof(int) * base_size);

			// Take border into account for minimum height.
			int max_height = 2;

			for (int i = 0; i < item_count; i++) {
				// best fit
				DecalAtlas::SortItem& si = items[i];
				int best_idx = -1;			  // ideal x position
				int best_height = 0x7FFFFFFF; // ideal y position
				for (uint32_t j = 0; j <= base_size - si.size.width; j++) {
					int height = 0;
					for (int k = 0; k < si.size.width; k++) {
						int h = v_offsets[k + j];
						if (h > height) {
							height = h;
							if (height > best_height) {
								break; // already bad
							}
						}
					}

					if (height < best_height) {
						best_height = height;
						best_idx = j;
					}
				}

				// update
				for (int k = 0; k < si.size.width; k++) {
					v_offsets[k + best_idx] = best_height + si.size.height;
				}

				si.pos.x = best_idx;
				si.pos.y = best_height;

				if (si.pos.y + si.size.height > max_height) {
					max_height = si.pos.y + si.size.height;
				}
			}

			if ((uint32_t)max_height <= base_size * 2) {
				atlas_height = max_height;
				break; // good ratio, break;
			}

			base_size *= 2;
		}

		decal_atlas.size.width = base_size * border;
		decal_atlas.size.height = Math::nearest_power_of_2_templated(atlas_height * border);

		for (int i = 0; i < item_count; i++) {
			DecalAtlas::Texture* t = decal_atlas.textures.getptr(items[i].texture);
			t->uv_rect.position = items[i].pos * border + Vector2i(border / 2, border / 2);
			t->uv_rect.size = items[i].pixel_size;

			t->uv_rect.position /= Size2(decal_atlas.size);
			t->uv_rect.size /= Size2(decal_atlas.size);
		}
	}
	else {
		// use border as size, so it at least has enough mipmaps
		decal_atlas.size.width = border;
		decal_atlas.size.height = border;
	}

	// blit textures

	RDC::TextureFormat tformat;
	tformat.format = RDC::DATA_FORMAT_R8G8B8A8_UNORM;
	tformat.width = decal_atlas.size.width;
	tformat.height = decal_atlas.size.height;
	tformat.usage_bits = RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT |
						 RDC::TEXTURE_USAGE_CAN_COPY_TO_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT;
	tformat.texture_type = RDC::TEXTURE_TYPE_2D;
	tformat.mipmaps = decal_atlas.mipmaps;
	tformat.shareable_formats.push_back(RDC::DATA_FORMAT_R8G8B8A8_UNORM);
	tformat.shareable_formats.push_back(RDC::DATA_FORMAT_R8G8B8A8_SRGB);

	decal_atlas.texture = RD::texture_create(tformat, RDD::TextureView());
	RD::texture_clear(decal_atlas.texture, Color(0, 0, 0, 0), 0, decal_atlas.mipmaps, 0, 1);

	{
		// create the framebuffer

		Size2i s = decal_atlas.size;

		for (int i = 0; i < decal_atlas.mipmaps; i++) {
			DecalAtlas::MipMap mm;
			mm.texture =
				RD::texture_create_shared_from_slice(RD::TextureView(), decal_atlas.texture, 0, i);
			Vector<RID> fb;
			fb.push_back(mm.texture);
			mm.fb = RD::framebuffer_create(fb);
			mm.size = s;
			decal_atlas.texture_mipmaps.push_back(mm);

			s = Vector2i(s.width >> 1, s.height >> 1).maxi(1);
		}
		{
			// create the SRGB variant
			RD::TextureView rd_view;
			rd_view.format_override = RDC::DATA_FORMAT_R8G8B8A8_SRGB;
			decal_atlas.texture_srgb = RD::texture_create_shared(rd_view, decal_atlas.texture);
		}
	}
}

void RendererRD::TextureStorage::texture_add_to_decal_atlas(RID p_texture, bool p_panorama_to_dp)
{
	if (!decal_atlas.textures.has(p_texture)) {
		DecalAtlas::Texture t;
		t.users = 1;
		t.panorama_to_dp_users = p_panorama_to_dp ? 1 : 0;
		decal_atlas.textures[p_texture] = t;
		decal_atlas.dirty = true;
	}
	else {
		DecalAtlas::Texture* t = decal_atlas.textures.getptr(p_texture);
		t->users++;
		if (p_panorama_to_dp) {
			t->panorama_to_dp_users++;
		}
	}
}

void RendererRD::TextureStorage::texture_remove_from_decal_atlas(
	RID p_texture, bool p_panorama_to_dp)
{
	DecalAtlas::Texture* t = decal_atlas.textures.getptr(p_texture);
	ERR_FAIL_NULL(t);
	t->users--;
	if (p_panorama_to_dp) {
		ERR_FAIL_COND(t->panorama_to_dp_users == 0);
		t->panorama_to_dp_users--;
	}
	if (t->users == 0) {
		decal_atlas.textures.erase(p_texture);
		// do not mark it dirty, there is no need to since it remains working
	}
}

RID RendererRD::TextureStorage::decal_instance_create(RID p_decal)
{
	DecalInstance di;
	di.decal = p_decal;
	di.forward_id = ForwardIDStorage::allocate_forward_id(FORWARD_ID_TYPE_DECAL);
	return decal_instance_owner.make_rid(di);
}

void RendererRD::TextureStorage::decal_instance_free(RID p_decal_instance)
{
	DecalInstance* di = decal_instance_owner.get_or_null(p_decal_instance);
	ForwardIDStorage::free_forward_id(FORWARD_ID_TYPE_DECAL, di->forward_id);
	decal_instance_owner.free(p_decal_instance);
}

void RendererRD::TextureStorage::decal_instance_set_transform(
	RID p_decal_instance, const Transform3D& p_transform)
{
	DecalInstance* di = decal_instance_owner.get_or_null(p_decal_instance);
	ERR_FAIL_NULL(di);
	di->transform = p_transform;
}

void RendererRD::TextureStorage::decal_instance_set_sorting_offset(
	RID p_decal_instance, float p_sorting_offset)
{
	DecalInstance* di = decal_instance_owner.get_or_null(p_decal_instance);
	ERR_FAIL_NULL(di);
	di->sorting_offset = p_sorting_offset;
}

void RendererRD::TextureStorage::free_decal_data()
{
	if (decal_buffer.is_valid()) {
		RD::free_rid(decal_buffer);
		decal_buffer = RID();
	}

	if (decals != nullptr) {
		memdelete_arr(decals);
		decals = nullptr;
	}

	if (decal_sort != nullptr) {
		memdelete_arr(decal_sort);
		decal_sort = nullptr;
	}
}

void RendererRD::TextureStorage::set_max_decals(const uint32_t p_max_decals)
{
	max_decals = p_max_decals;
	uint32_t decal_buffer_size = max_decals * sizeof(DecalData);
	decals = memnew_arr(DecalData, max_decals);
	decal_sort = memnew_arr(DecalInstanceSort, max_decals);
	decal_buffer = RD::storage_buffer_create(decal_buffer_size);
}

void RendererRD::TextureStorage::update_decal_buffer(
	const PagedArray<RID>& p_decals, const Transform3D& p_camera_xform)
{
	Transform3D uv_xform;
	uv_xform.basis.scale(Vector3(2.0, 1.0, 2.0));
	uv_xform.origin = Vector3(-1.0, 0.0, -1.0);

	uint32_t decals_size = p_decals.size();

	decal_count = 0;

	for (uint32_t i = 0; i < decals_size; i++) {
		if (decal_count == max_decals) {
			break;
		}

		DecalInstance* decal_instance = decal_instance_owner.get_or_null(p_decals[i]);
		if (!decal_instance) {
			continue;
		}
		Decal* decal = decal_owner.get_or_null(decal_instance->decal);

		Transform3D xform = decal_instance->transform;

		real_t distance = p_camera_xform.origin.distance_to(xform.origin);

		if (decal->distance_fade) {
			float fade_begin = decal->distance_fade_begin;
			float fade_length = decal->distance_fade_length;

			if (distance > fade_begin) {
				if (distance > fade_begin + fade_length) {
					continue; // do not use this decal, its invisible
				}
			}
		}

		decal_sort[decal_count].decal_instance = decal_instance;
		decal_sort[decal_count].decal = decal;
		decal_sort[decal_count].depth = distance - decal_instance->sorting_offset;
		decal_count++;
	}

	if (decal_count > 0) {
		SortArray<DecalInstanceSort> sort_array;
		sort_array.sort(decal_sort, decal_count);
	}

	bool using_forward_ids = ForwardIDStorage::uses_forward_ids();
	for (uint32_t i = 0; i < decal_count; i++) {
		DecalInstance* decal_instance = decal_sort[i].decal_instance;
		Decal* decal = decal_sort[i].decal;

		if (using_forward_ids) {
			ForwardIDStorage::map_forward_id(FORWARD_ID_TYPE_DECAL, decal_instance->forward_id, i,
				RendererCompositor::get_frame_number());
		}

		decal_instance->cull_mask = decal->cull_mask;

		float fade = 1.0;

		if (decal->distance_fade) {
			const real_t distance = decal_sort[i].depth + decal_instance->sorting_offset;
			const float fade_begin = decal->distance_fade_begin;
			const float fade_length = decal->distance_fade_length;

			if (distance > fade_begin) {
				// Use `smoothstep()` to make opacity changes more gradual and less noticeable to
				// the player.
				fade =
					Math::smoothstep(0.0f, 1.0f, 1.0f - float(distance - fade_begin) / fade_length);
			}
		}

		DecalData& dd = decals[i];

		Vector3 decal_extents = decal->size / 2;

		Transform3D scale_xform;
		scale_xform.basis.scale(decal_extents);

		Transform3D xform = decal_instance->transform;

		Transform3D camera_inverse_xform = p_camera_xform.affine_inverse();

		Transform3D to_decal_xform =
			(camera_inverse_xform * xform * scale_xform * uv_xform).affine_inverse();
		MaterialStorage::store_transform(to_decal_xform, dd.xform);

		Vector3 normal = xform.basis.get_column(Vector3::AXIS_Y).normalized();
		normal = camera_inverse_xform.basis.xform(normal); // camera is normalized, so fine

		dd.normal[0] = normal.x;
		dd.normal[1] = normal.y;
		dd.normal[2] = normal.z;
		dd.normal_fade = decal->normal_fade;

		RID albedo_tex = decal->textures[RSE::DECAL_TEXTURE_ALBEDO];
		RID emission_tex = decal->textures[RSE::DECAL_TEXTURE_EMISSION];
		if (albedo_tex.is_valid()) {
			Rect2 rect = decal_atlas_get_texture_rect(albedo_tex);
			dd.albedo_rect[0] = rect.position.x;
			dd.albedo_rect[1] = rect.position.y;
			dd.albedo_rect[2] = rect.size.x;
			dd.albedo_rect[3] = rect.size.y;
		}
		else {
			if (!emission_tex.is_valid()) {
				continue; // no albedo, no emission, no decal.
			}
			dd.albedo_rect[0] = 0;
			dd.albedo_rect[1] = 0;
			dd.albedo_rect[2] = 0;
			dd.albedo_rect[3] = 0;
		}

		RID normal_tex = decal->textures[RSE::DECAL_TEXTURE_NORMAL];

		if (normal_tex.is_valid()) {
			Rect2 rect = decal_atlas_get_texture_rect(normal_tex);
			dd.normal_rect[0] = rect.position.x;
			dd.normal_rect[1] = rect.position.y;
			dd.normal_rect[2] = rect.size.x;
			dd.normal_rect[3] = rect.size.y;

			Basis normal_xform = camera_inverse_xform.basis * xform.basis.orthonormalized();
			MaterialStorage::store_basis_3x4(normal_xform, dd.normal_xform);
		}
		else {
			dd.normal_rect[0] = 0;
			dd.normal_rect[1] = 0;
			dd.normal_rect[2] = 0;
			dd.normal_rect[3] = 0;
		}

		RID orm_tex = decal->textures[RSE::DECAL_TEXTURE_ORM];
		if (orm_tex.is_valid()) {
			Rect2 rect = decal_atlas_get_texture_rect(orm_tex);
			dd.orm_rect[0] = rect.position.x;
			dd.orm_rect[1] = rect.position.y;
			dd.orm_rect[2] = rect.size.x;
			dd.orm_rect[3] = rect.size.y;
		}
		else {
			dd.orm_rect[0] = 0;
			dd.orm_rect[1] = 0;
			dd.orm_rect[2] = 0;
			dd.orm_rect[3] = 0;
		}

		if (emission_tex.is_valid()) {
			Rect2 rect = decal_atlas_get_texture_rect(emission_tex);
			dd.emission_rect[0] = rect.position.x;
			dd.emission_rect[1] = rect.position.y;
			dd.emission_rect[2] = rect.size.x;
			dd.emission_rect[3] = rect.size.y;
		}
		else {
			dd.emission_rect[0] = 0;
			dd.emission_rect[1] = 0;
			dd.emission_rect[2] = 0;
			dd.emission_rect[3] = 0;
		}

		Color modulate = decal->modulate.srgb_to_linear();
		dd.modulate[0] = modulate.r;
		dd.modulate[1] = modulate.g;
		dd.modulate[2] = modulate.b;
		dd.modulate[3] = modulate.a * fade;
		dd.emission_energy = decal->emission_energy * fade;
		dd.albedo_mix = decal->albedo_mix;
		dd.mask = decal->cull_mask;
		dd.upper_fade = decal->upper_fade;
		dd.lower_fade = decal->lower_fade;

		// hook for subclass to do further processing.
		RendererSceneRenderRD::setup_added_decal(xform, decal_extents);
	}
}

RID RendererRD::TextureStorage::RenderTarget::get_framebuffer()
{
	// We can't resolve into our overridden buffer as it won't be marked as a resolve buffer.
	// This is only applicable when OpenXR is used and 2D rendering is skipped.

	if (msaa != RSE::VIEWPORT_MSAA_DISABLED && overridden.color.is_null()) {
		// Render into our MSAA buffer and resolve into our color buffer.
		return FramebufferCacheRD::get_cache_multiview(view_count, color_multisample, color);
	}
	else {
		// Note that if we're using an overridden color buffer, we're likely cycling through a
		// texture chain. this is where our framebuffer cache comes in clutch..

		return FramebufferCacheRD::get_cache_multiview(
			view_count, overridden.color.is_valid() ? overridden.color : color);
	}
}

void RendererRD::TextureStorage::_clear_render_target(RenderTarget* rt)
{
	// clear overrides, we assume these are freed by the object that created them
	rt->overridden.color = RID();
	rt->overridden.depth = RID();
	rt->overridden.velocity = RID();
	rt->overridden.cached_slices.clear(); // these are automatically freed when their parent
										  // textures are freed so just clear

	// free in reverse dependency order
	if (rt->framebuffer_uniform_set.is_valid()) {
		rt->framebuffer_uniform_set = RID(); // chain deleted
	}

	if (rt->color.is_valid()) {
		RD::free_rid(rt->color);
	}
	rt->color_slices.clear(); // these are automatically freed.

	if (rt->color_multisample.is_valid()) {
		RD::free_rid(rt->color_multisample);
	}

	if (rt->backbuffer.is_valid()) {
		RD::free_rid(rt->backbuffer);
		rt->backbuffer = RID();
		rt->backbuffer_mipmaps.clear();
		rt->backbuffer_uniform_set = RID(); // chain deleted
	}

	_render_target_clear_sdf(rt);

	rt->color = RID();
	rt->color_multisample = RID();
	if (rt->texture.is_valid()) {
		Texture* tex = get_texture(rt->texture);
		tex->render_target = nullptr;
	}
}

void RendererRD::TextureStorage::_update_render_target(RenderTarget* rt)
{
	if (rt->overridden.color.is_valid()) {
		return;
	}

	if (rt->texture.is_null()) {
		rt->texture = texture_allocate();

		Texture tex;
		tex.type = TYPE_2D;
		tex.is_render_target = true;
		tex.render_target = rt;
		tex.path = "Render Target (Internal)";

		texture_owner.initialize_rid(rt->texture, tex);
	}

	_clear_render_target(rt);

	if (rt->size.width == 0 || rt->size.height == 0) {
		return;
	}

	rt->color_format = render_target_get_color_format(rt->use_hdr, false);
	rt->color_format_srgb = render_target_get_color_format(rt->use_hdr, true);

	if (rt->use_hdr) {
		rt->image_format = rt->is_transparent ? Image::FORMAT_RGBAH : Image::FORMAT_RGBH;
	}
	else {
		rt->image_format = rt->is_transparent ? Image::FORMAT_RGBA8 : Image::FORMAT_RGB8;
	}

	RDC::TextureFormat rd_color_attachment_format;
	RDD::TextureView rd_view;
	{ // attempt register
		rd_color_attachment_format.format = rt->color_format;
		rd_color_attachment_format.width = rt->size.width;
		rd_color_attachment_format.height = rt->size.height;
		rd_color_attachment_format.depth = 1;
		rd_color_attachment_format.array_layers =
			rt->view_count; // for stereo we create two (or more) layers, need to see if we can make
							// fallback work like this too if we don't have multiview
		rd_color_attachment_format.mipmaps = 1;
		if (rd_color_attachment_format.array_layers >
			1) { // why are we not using rt->texture_type ??
			rd_color_attachment_format.texture_type = RDC::TEXTURE_TYPE_2D_ARRAY;
		}
		else {
			rd_color_attachment_format.texture_type = RDC::TEXTURE_TYPE_2D;
		}
		rd_color_attachment_format.samples = RDC::TEXTURE_SAMPLES_1;
		rd_color_attachment_format.usage_bits = render_target_get_color_usage_bits(false);
		rd_color_attachment_format.shareable_formats.push_back(rt->color_format);
		rd_color_attachment_format.shareable_formats.push_back(rt->color_format_srgb);
		if (rt->msaa != RSE::VIEWPORT_MSAA_DISABLED) {
			rd_color_attachment_format.is_resolve_buffer = true;
		}
	}

	// TODO see if we can lazy create this once we actually use it as we may not need to create this
	// if we have an overridden color buffer...
	rt->color = RD::texture_create(rd_color_attachment_format, rd_view);
	ERR_FAIL_COND(rt->color.is_null());

	if (rt->msaa != RSE::VIEWPORT_MSAA_DISABLED) {
		// Use the texture format of the color attachment for the multisample color attachment.
		RDC::TextureFormat rd_color_multisample_format = rd_color_attachment_format;
		const RDC::TextureSamples texture_samples[RSE::VIEWPORT_MSAA_MAX] = {
			RDC::TEXTURE_SAMPLES_1,
			RDC::TEXTURE_SAMPLES_2,
			RDC::TEXTURE_SAMPLES_4,
			RDC::TEXTURE_SAMPLES_8,
		};
		rd_color_multisample_format.samples = texture_samples[rt->msaa];
		rd_color_multisample_format.usage_bits = render_target_get_color_usage_bits(true);
		RDD::TextureView rd_view_multisample;
		rd_color_multisample_format.is_resolve_buffer = false;
		rd_color_multisample_format.is_discardable = true;
		rt->color_multisample =
			RD::texture_create(rd_color_multisample_format, rd_view_multisample);
		ERR_FAIL_COND(rt->color_multisample.is_null());
	}

	{ // update texture

		Texture* tex = get_texture(rt->texture);

		// free existing textures
		if (RD::texture_is_valid(tex->rd_texture)) {
			RD::free_rid(tex->rd_texture);
		}
		if (RD::texture_is_valid(tex->rd_texture_srgb)) {
			RD::free_rid(tex->rd_texture_srgb);
		}

		tex->rd_texture = RID();
		tex->rd_texture_srgb = RID();
		tex->render_target = rt;

		// create shared textures to the color buffer,
		// so transparent can be supported
		RD::TextureView view;
		view.format_override = rt->color_format;
		if (!rt->is_transparent) {
			view.swizzle_a = RDC::TEXTURE_SWIZZLE_ONE;
		}
		tex->rd_texture = RD::texture_create_shared(view, rt->color);
		if (rt->color_format_srgb != RDC::DATA_FORMAT_MAX) {
			view.format_override = rt->color_format_srgb;
			tex->rd_texture_srgb = RD::texture_create_shared(view, rt->color);
		}
		tex->rd_view = view;
		tex->width = rt->size.width;
		tex->height = rt->size.height;
		tex->width_2d = rt->size.width;
		tex->height_2d = rt->size.height;
		tex->rd_format = rt->color_format;
		tex->rd_format_srgb = rt->color_format_srgb;
		tex->format = rt->image_format;
		tex->validated_format = rt->use_hdr ? Image::FORMAT_RGBAH : Image::FORMAT_RGBA8;
		Vector<RID> proxies = tex->proxies; // make a copy, since update may change it
		for (int i = 0; i < proxies.size(); i++) {
			texture_proxy_update(proxies[i], rt->texture);
		}
	}
}

void RendererRD::TextureStorage::_create_render_target_backbuffer(RenderTarget* rt)
{
	ERR_FAIL_COND(rt->backbuffer.is_valid());

	uint32_t mipmaps_required =
		Image::get_image_required_mipmaps(rt->size.width, rt->size.height, Image::FORMAT_RGBA8);
	RDC::TextureFormat tf;
	tf.format = rt->color_format;
	tf.width = rt->size.width;
	tf.height = rt->size.height;
	tf.texture_type = RDC::TEXTURE_TYPE_2D;
	tf.usage_bits = RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT |
					RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_CAN_COPY_TO_BIT;
	tf.mipmaps = mipmaps_required;

	rt->backbuffer = RD::texture_create(tf, RDD::TextureView());
	RD::set_resource_name(rt->backbuffer, "Render Target Back Buffer");
	rt->backbuffer_mipmap0 =
		RD::texture_create_shared_from_slice(RD::TextureView(), rt->backbuffer, 0, 0);
	RD::set_resource_name(rt->backbuffer_mipmap0, "Back Buffer slice mipmap 0");

	{
		Vector<RID> fb_tex;
		fb_tex.push_back(rt->backbuffer_mipmap0);
		rt->backbuffer_fb = RD::framebuffer_create(fb_tex);
	}

	if (rt->framebuffer_uniform_set.is_valid() &&
		RD::uniform_set_is_valid(rt->framebuffer_uniform_set)) {
		// the new one will require the backbuffer.
		RD::free_rid(rt->framebuffer_uniform_set);
		rt->framebuffer_uniform_set = RID();
	}
	// create mipmaps
	for (uint32_t i = 1; i < mipmaps_required; i++) {
		RID mipmap = RD::texture_create_shared_from_slice(RD::TextureView(), rt->backbuffer, 0, i);
		RD::set_resource_name(mipmap, "Back Buffer slice mip: " + itos(i));

		rt->backbuffer_mipmaps.push_back(mipmap);
	}
}

RID RendererRD::TextureStorage::render_target_create()
{
	RenderTarget render_target;

	render_target.was_used = false;
	render_target.clear_requested = false;

	_update_render_target(&render_target);
	return render_target_owner.make_rid(render_target);
}

void RendererRD::TextureStorage::render_target_free(RID p_rid)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_rid);

	_clear_render_target(rt);

	if (rt->texture.is_valid()) {
		Texture* tex = get_texture(rt->texture);
		tex->is_render_target = false;
		texture_free(rt->texture);
	}

	render_target_owner.free(p_rid);
}

void RendererRD::TextureStorage::render_target_set_position(RID p_render_target, int p_x, int p_y)
{
	// unused for this render target
}

Point2i RendererRD::TextureStorage::render_target_get_position(RID p_render_target)
{
	// unused for this render target
	return Point2i();
}

void RendererRD::TextureStorage::render_target_set_size(
	RID p_render_target, int p_width, int p_height, uint32_t p_view_count)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);
	if (rt->size.x != p_width || rt->size.y != p_height || rt->view_count != p_view_count) {
		rt->size.x = p_width;
		rt->size.y = p_height;
		rt->view_count = p_view_count;
		_update_render_target(rt);
	}
}

Size2i RendererRD::TextureStorage::render_target_get_size(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, Size2i());

	return rt->size;
}

RID RendererRD::TextureStorage::render_target_get_texture(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, RID());

	return rt->texture;
}

void RendererRD::TextureStorage::render_target_set_override(RID p_render_target,
	RID p_color_texture, RID p_depth_texture, RID p_velocity_texture, RID p_velocity_depth_texture)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);

	rt->overridden.color = p_color_texture;
	rt->overridden.depth = p_depth_texture;
	rt->overridden.velocity = p_velocity_texture;
	rt->overridden.velocity_depth = p_velocity_depth_texture;
}

RID RendererRD::TextureStorage::render_target_get_override_color(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, RID());

	return rt->overridden.color;
}

RID RendererRD::TextureStorage::render_target_get_override_depth(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, RID());

	return rt->overridden.depth;
}

RID RendererRD::TextureStorage::render_target_get_override_depth_slice(
	RID p_render_target, const uint32_t p_layer)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, RID());

	if (rt->overridden.depth.is_null()) {
		return RID();
	}
	else if (rt->view_count == 1) {
		return rt->overridden.depth;
	}
	else {
		RenderTarget::RTOverridden::SliceKey key(rt->overridden.depth, p_layer);

		if (!rt->overridden.cached_slices.has(key)) {
			rt->overridden.cached_slices[key] = RD::texture_create_shared_from_slice(
				RD::TextureView(), rt->overridden.depth, p_layer, 0);
		}

		return rt->overridden.cached_slices[key];
	}
}

RID RendererRD::TextureStorage::render_target_get_override_velocity(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, RID());

	return rt->overridden.velocity;
}

RID RendererRD::TextureStorage::render_target_get_override_velocity_slice(
	RID p_render_target, const uint32_t p_layer)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, RID());

	if (rt->overridden.velocity.is_null()) {
		return RID();
	}
	else if (rt->view_count == 1) {
		return rt->overridden.velocity;
	}
	else {
		RenderTarget::RTOverridden::SliceKey key(rt->overridden.velocity, p_layer);

		if (!rt->overridden.cached_slices.has(key)) {
			rt->overridden.cached_slices[key] = RD::texture_create_shared_from_slice(
				RD::TextureView(), rt->overridden.velocity, p_layer, 0);
		}

		return rt->overridden.cached_slices[key];
	}
}

RID RendererRD::TextureStorage::render_target_get_override_velocity_depth(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, RID());

	return rt->overridden.velocity_depth;
}

void RendererRD::TextureStorage::render_target_set_render_region(
	RID p_render_target, const Rect2i& p_render_region)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);

	rt->render_region = p_render_region;
}

Rect2i RendererRD::TextureStorage::render_target_get_render_region(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, Rect2i());

	return rt->render_region;
}

void RendererRD::TextureStorage::render_target_set_subsampled_enabled(
	RID p_render_target, bool p_enabled)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);

	rt->subsampled_enabled = p_enabled;
}

bool RendererRD::TextureStorage::render_target_is_subsampled_enabled(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, false);

	return rt->subsampled_enabled;
}

void RendererRD::TextureStorage::render_target_set_subsampled_allowed(
	RID p_render_target, bool p_allowed)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);

	rt->subsampled_allowed = p_allowed;
}

bool RendererRD::TextureStorage::render_target_is_subsampled_allowed(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, false);

	return rt->subsampled_allowed;
}

void RendererRD::TextureStorage::render_target_set_transparent(
	RID p_render_target, bool p_is_transparent)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);
	rt->is_transparent = p_is_transparent;
	_update_render_target(rt);
}

bool RendererRD::TextureStorage::render_target_get_transparent(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, false);

	return rt->is_transparent;
}

void RendererRD::TextureStorage::render_target_set_direct_to_screen(
	RID p_render_target, bool p_value)
{
}

bool RendererRD::TextureStorage::render_target_get_direct_to_screen(RID p_render_target)
{
	return false;
}

bool RendererRD::TextureStorage::render_target_was_used(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, false);
	return rt->was_used;
}

void RendererRD::TextureStorage::render_target_set_as_unused(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);
	rt->was_used = false;
}

void RendererRD::TextureStorage::render_target_set_msaa(
	RID p_render_target, RSE::ViewportMSAA p_msaa)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);
	if (p_msaa == rt->msaa) {
		return;
	}

	rt->msaa = p_msaa;
	_update_render_target(rt);
}

RSE::ViewportMSAA RendererRD::TextureStorage::render_target_get_msaa(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, RSE::VIEWPORT_MSAA_DISABLED);

	return rt->msaa;
}

void RendererRD::TextureStorage::render_target_set_msaa_needs_resolve(
	RID p_render_target, bool p_needs_resolve)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);

	rt->msaa_needs_resolve = p_needs_resolve;
}

bool RendererRD::TextureStorage::render_target_get_msaa_needs_resolve(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, false);

	return rt->msaa_needs_resolve;
}

void RendererRD::TextureStorage::render_target_do_msaa_resolve(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);
	if (!rt->msaa_needs_resolve) {
		return;
	}
	RD::draw_list_end();
	rt->msaa_needs_resolve = false;
}

void RendererRD::TextureStorage::render_target_set_use_hdr(RID p_render_target, bool p_use_hdr)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);

	if (p_use_hdr == rt->use_hdr) {
		return;
	}

	rt->use_hdr = p_use_hdr;
	_update_render_target(rt);
}

bool RendererRD::TextureStorage::render_target_is_using_hdr(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, false);

	return rt->use_hdr;
}

void RendererRD::TextureStorage::render_target_set_use_debanding(
	RID p_render_target, bool p_use_debanding)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);

	rt->use_debanding = p_use_debanding;
}

bool RendererRD::TextureStorage::render_target_is_using_debanding(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, false);

	return rt->use_debanding;
}

RID RendererRD::TextureStorage::render_target_get_rd_framebuffer(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, RID());

	return rt->get_framebuffer();
}

RID RendererRD::TextureStorage::render_target_get_rd_texture(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, RID());

	if (rt->overridden.color.is_valid()) {
		return rt->overridden.color;
	}
	else {
		return rt->color;
	}
}

RID RendererRD::TextureStorage::render_target_get_rd_texture_slice(
	RID p_render_target, uint32_t p_layer)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, RID());

	if (rt->view_count == 1) {
		return rt->color;
	}
	else {
		ERR_FAIL_UNSIGNED_INDEX_V(p_layer, rt->view_count, RID());
		if (rt->color_slices.is_empty()) {
			for (uint32_t v = 0; v < rt->view_count; v++) {
				RID slice =
					RD::texture_create_shared_from_slice(RD::TextureView(), rt->color, v, 0);
				rt->color_slices.push_back(slice);
			}
		}
		return rt->color_slices[p_layer];
	}
}

RID RendererRD::TextureStorage::render_target_get_rd_texture_msaa(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, RID());

	return rt->color_multisample;
}

RID RendererRD::TextureStorage::render_target_get_rd_backbuffer(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, RID());
	return rt->backbuffer;
}

RID RendererRD::TextureStorage::render_target_get_rd_backbuffer_framebuffer(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, RID());

	if (!rt->backbuffer.is_valid()) {
		_create_render_target_backbuffer(rt);
	}

	return rt->backbuffer_fb;
}

void RendererRD::TextureStorage::render_target_request_clear(
	RID p_render_target, const Color& p_clear_color)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);
	rt->clear_requested = true;
	rt->clear_color = p_clear_color;
}

bool RendererRD::TextureStorage::render_target_is_clear_requested(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, false);
	return rt->clear_requested;
}

Color RendererRD::TextureStorage::render_target_get_clear_request_color(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, Color());
	return rt->clear_color;
}

void RendererRD::TextureStorage::render_target_disable_clear_request(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);
	rt->clear_requested = false;
}

void RendererRD::TextureStorage::render_target_do_clear_request(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);
	if (!rt->clear_requested) {
		return;
	}
	Vector<Color> clear_colors;
	clear_colors.push_back(rt->use_hdr ? rt->clear_color.srgb_to_linear() : rt->clear_color);
	const Vector<Color> clear_colors_const = clear_colors;

	// Begin a draw list on the render target's framebuffer
	if (rt->rd_framebuffer.is_valid()) {
        RD::DrawListID draw_list = RD::draw_list_begin(
            rt->rd_framebuffer, RenderingDevice::DRAW_CLEAR_COLOR_0, clear_colors);
        if (draw_list != RDC::INVALID_ID) {
            RD::draw_list_end();
        }
    }

	rt->clear_requested = false;
	rt->msaa_needs_resolve = false;
}

void RendererRD::TextureStorage::render_target_set_sdf_size_and_scale(
	RID p_render_target, RSE::ViewportSDFOversize p_size, RSE::ViewportSDFScale p_scale)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);
	if (rt->sdf_oversize == p_size && rt->sdf_scale == p_scale) {
		return;
	}

	rt->sdf_oversize = p_size;
	rt->sdf_scale = p_scale;

	_render_target_clear_sdf(rt);
}

Rect2i RendererRD::TextureStorage::_render_target_get_sdf_rect(const RenderTarget* rt)
{
	Size2i margin;
	int scale;
	switch (rt->sdf_oversize) {
	case RSE::VIEWPORT_SDF_OVERSIZE_100_PERCENT: {
		scale = 100;
	} break;
	case RSE::VIEWPORT_SDF_OVERSIZE_120_PERCENT: {
		scale = 120;
	} break;
	case RSE::VIEWPORT_SDF_OVERSIZE_150_PERCENT: {
		scale = 150;
	} break;
	case RSE::VIEWPORT_SDF_OVERSIZE_200_PERCENT: {
		scale = 200;
	} break;
	default: {
		ERR_PRINT("Invalid viewport SDF oversize, defaulting to 100%.");
		scale = 100;
	} break;
	}

	margin = (rt->size * scale / 100) - rt->size;

	Rect2i r(Vector2i(), rt->size);
	r.position -= margin;
	r.size += margin * 2;

	return r;
}

Rect2i RendererRD::TextureStorage::render_target_get_sdf_rect(RID p_render_target)
{
	const RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, Rect2i());

	return _render_target_get_sdf_rect(rt);
}

void RendererRD::TextureStorage::render_target_mark_sdf_enabled(RID p_render_target, bool p_enabled)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);

	rt->sdf_enabled = p_enabled;
}

bool RendererRD::TextureStorage::render_target_is_sdf_enabled(RID p_render_target)
{
	const RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, false);

	return rt->sdf_enabled;
}

RID RendererRD::TextureStorage::render_target_get_sdf_texture(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, RID());
	if (rt->sdf_buffer_read.is_null()) {
		// no texture, create a dummy one for the 2D uniform set
		RDC::TextureFormat tformat;
		tformat.format = RDC::DATA_FORMAT_R8G8B8A8_UNORM;
		tformat.width = 4;
		tformat.height = 4;
		tformat.usage_bits = RDC::TEXTURE_USAGE_SAMPLING_BIT;
		tformat.texture_type = RDC::TEXTURE_TYPE_2D;

		Vector<uint8_t> pv;
		pv.resize(16 * 4);
		memset(pv.ptrw(), 0, 16 * 4);
		Vector<Vector<uint8_t>> vpv;
		vpv.push_back(pv);
		rt->sdf_buffer_read = RD::texture_create(tformat, RDD::TextureView(), vpv);
	}

	return rt->sdf_buffer_read;
}

void RendererRD::TextureStorage::_render_target_clear_sdf(RenderTarget* rt)
{
	if (rt->sdf_buffer_read.is_valid()) {
		RD::free_rid(rt->sdf_buffer_read);
		rt->sdf_buffer_read = RID();
	}
	if (rt->sdf_buffer_write_fb.is_valid()) {
		RD::free_rid(rt->sdf_buffer_write);
		RD::free_rid(rt->sdf_buffer_process[0]);
		RD::free_rid(rt->sdf_buffer_process[1]);
		rt->sdf_buffer_write = RID();
		rt->sdf_buffer_write_fb = RID();
		rt->sdf_buffer_process[0] = RID();
		rt->sdf_buffer_process[1] = RID();
		rt->sdf_buffer_process_uniform_sets[0] = RID();
		rt->sdf_buffer_process_uniform_sets[1] = RID();
	}
}

RID RendererRD::TextureStorage::render_target_get_sdf_framebuffer(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, RID());

	if (rt->sdf_buffer_write_fb.is_null()) {
		_render_target_allocate_sdf(rt);
	}

	return rt->sdf_buffer_write_fb;
}

void RendererRD::TextureStorage::render_target_sdf_process(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);
	ERR_FAIL_COND(rt->sdf_buffer_write_fb.is_null());

	RenderTargetSDF::PushConstant push_constant;

	Rect2i r = _render_target_get_sdf_rect(rt);

	push_constant.size[0] = r.size.width;
	push_constant.size[1] = r.size.height;
	push_constant.stride = 0;
	push_constant.shift = 0;
	push_constant.base_size[0] = r.size.width;
	push_constant.base_size[1] = r.size.height;

	bool shrink = false;

	switch (rt->sdf_scale) {
	case RSE::VIEWPORT_SDF_SCALE_50_PERCENT: {
		push_constant.size[0] >>= 1;
		push_constant.size[1] >>= 1;
		push_constant.shift = 1;
		shrink = true;
	} break;
	case RSE::VIEWPORT_SDF_SCALE_25_PERCENT: {
		push_constant.size[0] >>= 2;
		push_constant.size[1] >>= 2;
		push_constant.shift = 2;
		shrink = true;
	} break;
	default: {
	};
	}

	RD::ComputeListID compute_list = RD::compute_list_begin();

	RD::compute_list_bind_uniform_set(
		compute_list, rt->sdf_buffer_process_uniform_sets[1], 0); // fill [0]
	RD::compute_list_set_push_constant(
		compute_list, &push_constant, sizeof(RenderTargetSDF::PushConstant));

	RD::compute_list_dispatch_threads(
		compute_list, push_constant.size[0], push_constant.size[1], 1);

	int stride =
		Math::nearest_power_of_2_templated(MAX(push_constant.size[0], push_constant.size[1]) / 2);

	RD::compute_list_add_barrier(compute_list);
	bool swap = false;

	// jumpflood
	while (stride > 0) {
		RD::compute_list_bind_uniform_set(
			compute_list, rt->sdf_buffer_process_uniform_sets[swap ? 1 : 0], 0);
		push_constant.stride = stride;
		RD::compute_list_set_push_constant(
			compute_list, &push_constant, sizeof(RenderTargetSDF::PushConstant));
		RD::compute_list_dispatch_threads(
			compute_list, push_constant.size[0], push_constant.size[1], 1);
		stride /= 2;
		swap = !swap;
		RD::compute_list_add_barrier(compute_list);
	}

	RD::compute_list_bind_uniform_set(
		compute_list, rt->sdf_buffer_process_uniform_sets[swap ? 1 : 0], 0);
	RD::compute_list_set_push_constant(
		compute_list, &push_constant, sizeof(RenderTargetSDF::PushConstant));
	RD::compute_list_dispatch_threads(
		compute_list, push_constant.size[0], push_constant.size[1], 1);

	RD::compute_list_end();
}

void RendererRD::TextureStorage::render_target_copy_to_back_buffer(
	RID p_render_target, const Rect2i& p_region, bool p_gen_mipmaps)
{
	ERR_FAIL_COND(!CopyEffects::is_initialized());

	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);
	if (!rt->backbuffer.is_valid()) {
		_create_render_target_backbuffer(rt);
	}

	Rect2i region;
	if (p_region == Rect2i()) {
		region.size = rt->size;
	}
	else {
		region = Rect2i(Size2i(), rt->size).intersection(p_region);
		if (region.size == Size2i()) {
			return; // nothing to do
		}
	}

	// TODO figure out stereo support here

	if (RendererSceneRenderRD::_render_buffers_can_be_storage()) {
		CopyEffects::copy_to_rect(
			rt->color, rt->backbuffer_mipmap0, region, false, false, false, !rt->use_hdr, true);
	}
	else {
		Rect2 src_rect = Rect2(region);
		src_rect.position /= Size2(rt->size);
		src_rect.size /= Size2(rt->size);
		CopyEffects::copy_to_fb_rect(rt->color, rt->backbuffer_fb, region, false, false, false,
			false, RID(), false, true, false, false, src_rect);
	}

	if (!p_gen_mipmaps) {
		return;
	}
	RD::draw_command_begin_label("Gaussian Blur Mipmaps");
	// then mipmap blur
	RID prev_texture = rt->color; // use color, not backbuffer, as bb has mipmaps.

	Size2i texture_size = rt->size;

	for (int i = 0; i < rt->backbuffer_mipmaps.size(); i++) {
		region.position.x >>= 1;
		region.position.y >>= 1;
		region.size = Size2i(region.size.x >> 1, region.size.y >> 1).maxi(1);
		texture_size = Size2i(texture_size.x >> 1, texture_size.y >> 1).maxi(1);

		RID mipmap = rt->backbuffer_mipmaps[i];
		if (RendererSceneRenderRD::_render_buffers_can_be_storage()) {
			CopyEffects::gaussian_blur(prev_texture, mipmap, region, texture_size, !rt->use_hdr);
		}
		else {
			CopyEffects::gaussian_blur_raster(prev_texture, mipmap, region, texture_size);
		}
		prev_texture = mipmap;
	}
	RD::draw_command_end_label();
}

void RendererRD::TextureStorage::render_target_clear_back_buffer(
	RID p_render_target, const Rect2i& p_region, const Color& p_color)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);

	ERR_FAIL_COND(!CopyEffects::is_initialized());

	if (!rt->backbuffer.is_valid()) {
		_create_render_target_backbuffer(rt);
	}

	Rect2i region;
	if (p_region == Rect2i()) {
		region.size = rt->size;
	}
	else {
		region = Rect2i(Size2i(), rt->size).intersection(p_region);
		if (region.size == Size2i()) {
			return; // nothing to do
		}
	}

	// Single texture copy for backbuffer.
	if (RendererSceneRenderRD::_render_buffers_can_be_storage()) {
		CopyEffects::set_color(rt->backbuffer_mipmap0, p_color, region, !rt->use_hdr);
	}
	else {
		CopyEffects::set_color_raster(rt->backbuffer_mipmap0, p_color, region);
	}
}

void RendererRD::TextureStorage::render_target_gen_back_buffer_mipmaps(
	RID p_render_target, const Rect2i& p_region)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);

	ERR_FAIL_COND(!CopyEffects::is_initialized());

	if (!rt->backbuffer.is_valid()) {
		_create_render_target_backbuffer(rt);
	}

	Rect2i region;
	if (p_region == Rect2i()) {
		region.size = rt->size;
	}
	else {
		region = Rect2i(Size2i(), rt->size).intersection(p_region);
		if (region.size == Size2i()) {
			return; // nothing to do
		}
	}
	RD::draw_command_begin_label("Gaussian Blur Mipmaps Pass 2");
	// then mipmap blur
	RID prev_texture = rt->backbuffer_mipmap0;
	Size2i texture_size = rt->size;

	for (int i = 0; i < rt->backbuffer_mipmaps.size(); i++) {
		region.position.x >>= 1;
		region.position.y >>= 1;
		region.size = Size2i(region.size.x >> 1, region.size.y >> 1).maxi(1);
		texture_size = Size2i(texture_size.x >> 1, texture_size.y >> 1).maxi(1);

		RID mipmap = rt->backbuffer_mipmaps[i];

		if (RendererSceneRenderRD::_render_buffers_can_be_storage()) {
			CopyEffects::gaussian_blur(prev_texture, mipmap, region, texture_size, !rt->use_hdr);
		}
		else {
			CopyEffects::gaussian_blur_raster(prev_texture, mipmap, region, texture_size);
		}
		prev_texture = mipmap;
	}
	RD::draw_command_end_label();
}

RID RendererRD::TextureStorage::render_target_get_framebuffer_uniform_set(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, RID());
	return rt->framebuffer_uniform_set;
}

RID RendererRD::TextureStorage::render_target_get_backbuffer_uniform_set(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, RID());
	return rt->backbuffer_uniform_set;
}

void RendererRD::TextureStorage::render_target_set_framebuffer_uniform_set(
	RID p_render_target, RID p_uniform_set)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);
	rt->framebuffer_uniform_set = p_uniform_set;
}

void RendererRD::TextureStorage::render_target_set_backbuffer_uniform_set(
	RID p_render_target, RID p_uniform_set)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);
	rt->backbuffer_uniform_set = p_uniform_set;
}

void RendererRD::TextureStorage::render_target_set_vrs_mode(
	RID p_render_target, RSE::ViewportVRSMode p_mode)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);

	rt->vrs_mode = p_mode;
}

RSE::ViewportVRSMode RendererRD::TextureStorage::render_target_get_vrs_mode(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, RSE::VIEWPORT_VRS_DISABLED);

	return rt->vrs_mode;
}

void RendererRD::TextureStorage::render_target_set_vrs_update_mode(
	RID p_render_target, RSE::ViewportVRSUpdateMode p_mode)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);

	rt->vrs_update_mode = p_mode;
}

RSE::ViewportVRSUpdateMode RendererRD::TextureStorage::render_target_get_vrs_update_mode(
	RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, RSE::VIEWPORT_VRS_UPDATE_DISABLED);

	return rt->vrs_update_mode;
}

void RendererRD::TextureStorage::render_target_set_vrs_texture(RID p_render_target, RID p_texture)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL(rt);

	rt->vrs_texture = p_texture;
}

RID RendererRD::TextureStorage::render_target_get_vrs_texture(RID p_render_target)
{
	RenderTarget* rt = render_target_owner.get_or_null(p_render_target);
	ERR_FAIL_NULL_V(rt, RID());

	return rt->vrs_texture;
}

RDC::DataFormat RendererRD::TextureStorage::render_target_get_color_format(
	bool p_use_hdr, bool p_srgb)
{
	if (p_use_hdr) {
		return RDC::DATA_FORMAT_R16G16B16A16_SFLOAT;
	}
	else {
		return p_srgb ? RDC::DATA_FORMAT_R8G8B8A8_SRGB : RDC::DATA_FORMAT_R8G8B8A8_UNORM;
	}
}

uint32_t RendererRD::TextureStorage::render_target_get_color_usage_bits(bool p_msaa)
{
	if (p_msaa) {
		return RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT;
	}
	else {
		// FIXME: Storage bit should only be requested when FSR is required.
		return RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT |
			   RDC::TEXTURE_USAGE_CAN_COPY_FROM_BIT | RDC::TEXTURE_USAGE_STORAGE_BIT;
	}
}

void RendererRD::TextureStorage::_render_target_allocate_sdf(
	RendererRD::TextureStorage::RenderTarget* p_render_target)
{
}

void RendererRD::TextureStorage::texture_drawable_generate_mipmaps(RID p_texture) {}

RendererRD::TextureStorage::TextureType RendererRD::TextureStorage::texture_get_type(RID p_texture)
{
	Texture* tex = texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL_V(tex, TYPE_2D);
	return tex->type;
}

int RendererRD::TextureStorage::texture_get_layers(RID p_texture)
{
	Texture* tex = texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL_V(tex, 0);
	return tex->layers;
}

Size2i RendererRD::TextureStorage::texture_2d_get_size(RID p_texture)
{
	Texture* tex = texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL_V(tex, Size2i());
	return Size2i(tex->width, tex->height);
}

uint32_t RendererRD::TextureStorage::decal_instance_get_forward_id(RID p_decal_instance)
{
	DecalInstance* di = decal_instance_owner.get_or_null(p_decal_instance);
	ERR_FAIL_NULL_V(di, 0);
	return di->forward_id;
}

struct LocalTextureToRDFormat
{
	RDC::DataFormat format = RDC::DATA_FORMAT_MAX;
	RDC::DataFormat format_srgb = RDC::DATA_FORMAT_MAX;
	RDC::TextureSwizzle swizzle_r = RDC::TEXTURE_SWIZZLE_R;
	RDC::TextureSwizzle swizzle_g = RDC::TEXTURE_SWIZZLE_G;
	RDC::TextureSwizzle swizzle_b = RDC::TEXTURE_SWIZZLE_B;
	RDC::TextureSwizzle swizzle_a = RDC::TEXTURE_SWIZZLE_A;
};

static Ref<Image> _map_image_to_rd_format(
	const Ref<Image>& p_image, LocalTextureToRDFormat& r_format)
{
	Ref<Image> image = p_image;
	switch (p_image->get_format()) {
	case Image::FORMAT_L8: {
		r_format.format = RDC::DATA_FORMAT_R8_UNORM;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_ONE;
	} break;
	case Image::FORMAT_LA8: {
		r_format.format = RDC::DATA_FORMAT_R8G8_UNORM;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_G;
	} break;
	case Image::FORMAT_R8: {
		r_format.format = RDC::DATA_FORMAT_R8_UNORM;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_ZERO;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_ZERO;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_ONE;
	} break;
	case Image::FORMAT_RG8: {
		r_format.format = RDC::DATA_FORMAT_R8G8_UNORM;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_ZERO;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_ONE;
	} break;
	case Image::FORMAT_RGB8: {
		image = Ref<Image>(memnew(Image(p_image->get_width(), p_image->get_height(),
			p_image->has_mipmaps(), p_image->get_format(), p_image->get_data())));
		image->convert(Image::FORMAT_RGBA8);
		r_format.format = RDC::DATA_FORMAT_R8G8B8A8_UNORM;
		r_format.format_srgb = RDC::DATA_FORMAT_R8G8B8A8_SRGB;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_B;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_ONE;
	} break;
	case Image::FORMAT_RGBA8: {
		r_format.format = RDC::DATA_FORMAT_R8G8B8A8_UNORM;
		r_format.format_srgb = RDC::DATA_FORMAT_R8G8B8A8_SRGB;
	} break;
	case Image::FORMAT_RGBA4444: {
		r_format.format = RDC::DATA_FORMAT_B4G4R4A4_UNORM_PACK16;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_B;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_A;
	} break;
	case Image::FORMAT_RGB565: {
		r_format.format = RDC::DATA_FORMAT_B5G6R5_UNORM_PACK16;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_B;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_A;
	} break;
	case Image::FORMAT_RF: {
		r_format.format = RDC::DATA_FORMAT_R32_SFLOAT;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_ZERO;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_ZERO;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_ONE;
	} break;
	case Image::FORMAT_RGF: {
		r_format.format = RDC::DATA_FORMAT_R32G32_SFLOAT;
		r_format.swizzle_r = RDC::TEXTURE_SWIZZLE_R;
		r_format.swizzle_g = RDC::TEXTURE_SWIZZLE_G;
		r_format.swizzle_b = RDC::TEXTURE_SWIZZLE_ZERO;
		r_format.swizzle_a = RDC::TEXTURE_SWIZZLE_ONE;
	} break;
	case Image::FORMAT_RGBAF: {
		r_format.format = RDC::DATA_FORMAT_R32G32B32A32_SFLOAT;
	} break;
	case Image::FORMAT_RGBAH: {
		r_format.format = RDC::DATA_FORMAT_R16G16B16A16_SFLOAT;
	} break;
	default: {
		image = Ref<Image>(memnew(Image(p_image->get_width(), p_image->get_height(),
			p_image->has_mipmaps(), p_image->get_format(), p_image->get_data())));
		image->convert(Image::FORMAT_RGBA8);
		r_format.format = RDC::DATA_FORMAT_R8G8B8A8_UNORM;
		r_format.format_srgb = RDC::DATA_FORMAT_R8G8B8A8_SRGB;
	} break;
	}
	return image;
}

void RendererRD::TextureStorage::texture_2d_initialize(RID p_texture, const Ref<Image>& p_image)
{
	ERR_FAIL_COND(p_image.is_null() || p_image->is_empty());

	LocalTextureToRDFormat ret_format;
	Ref<Image> image = _map_image_to_rd_format(p_image, ret_format);

	Texture texture;
	texture.type = TYPE_2D;
	texture.width = p_image->get_width();
	texture.height = p_image->get_height();
	texture.width_2d = texture.width;
	texture.height_2d = texture.height;
	texture.depth = 1;
	texture.layers = 1;
	texture.mipmaps = image->get_mipmap_count() + 1;
	texture.format = p_image->get_format();
	texture.validated_format = image->get_format();
	texture.image = p_image;
	texture.rd_type = RDC::TEXTURE_TYPE_2D;
	texture.rd_format = ret_format.format;
	texture.rd_format_srgb = ret_format.format_srgb;

	RDC::TextureFormat tf;
	tf.format = texture.rd_format;
	tf.width = texture.width;
	tf.height = texture.height;
	tf.depth = 1;
	tf.array_layers = 1;
	tf.mipmaps = texture.mipmaps;
	tf.texture_type = RDC::TEXTURE_TYPE_2D;
	tf.samples = RDC::TEXTURE_SAMPLES_1;
	tf.usage_bits = RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_CAN_UPDATE_BIT |
					RDC::TEXTURE_USAGE_CAN_COPY_FROM_BIT;
	if (texture.rd_format_srgb != RDC::DATA_FORMAT_MAX) {
		tf.shareable_formats.push_back(texture.rd_format);
		tf.shareable_formats.push_back(texture.rd_format_srgb);
	}

	RD::TextureView rd_view;
	rd_view.swizzle_r = ret_format.swizzle_r;
	rd_view.swizzle_g = ret_format.swizzle_g;
	rd_view.swizzle_b = ret_format.swizzle_b;
	rd_view.swizzle_a = ret_format.swizzle_a;

	Vector<Vector<uint8_t>> data_slices;
	data_slices.push_back(image->get_data());

	texture.rd_texture = RD::texture_create(tf, rd_view, data_slices);
	ERR_FAIL_COND(texture.rd_texture.is_null());

	if (texture.rd_format_srgb != RDC::DATA_FORMAT_MAX) {
		RD::TextureView srgb_view = rd_view;
		srgb_view.format_override = texture.rd_format_srgb;
		texture.rd_texture_srgb = RD::texture_create_shared(srgb_view, texture.rd_texture);
		if (texture.rd_texture_srgb.is_null()) {
			RD::free_rid(texture.rd_texture);
			ERR_FAIL_COND(texture.rd_texture_srgb.is_null());
		}
	}

	texture.is_render_target = false;
	texture.rd_view = rd_view;
	texture.is_proxy = false;

	texture_owner.initialize_rid(p_texture, texture);
}

void RendererRD::TextureStorage::texture_2d_layered_initialize(
	RID p_texture, const Vector<Ref<Image>>& p_layers, RSE::TextureLayeredType p_layered_type)
{
	ERR_FAIL_COND(p_layers.is_empty());
	Texture texture;
	texture.type = TYPE_LAYERED;
	texture.layered_type = p_layered_type;
	texture.width = p_layers[0]->get_width();
	texture.height = p_layers[0]->get_height();
	texture.layers = p_layers.size();
	texture_owner.initialize_rid(p_texture, texture);
}

void RendererRD::TextureStorage::texture_3d_initialize(RID p_texture, Image::Format p_format,
	int p_width, int p_height, int p_depth, bool p_mipmaps, const Vector<Ref<Image>>& p_data)
{
	Texture texture;
	texture.type = TYPE_3D;
	texture.width = p_width;
	texture.height = p_height;
	texture.depth = p_depth;
	texture.format = p_format;
	texture_owner.initialize_rid(p_texture, texture);
}

void RendererRD::TextureStorage::initialize()
{
	{ // create default textures
		RDC::TextureFormat tformat;
		tformat.format = RDC::DATA_FORMAT_R8G8B8A8_UNORM;
		tformat.width = 4;
		tformat.height = 4;
		tformat.usage_bits = RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_CAN_UPDATE_BIT;
		tformat.texture_type = RDC::TEXTURE_TYPE_2D;

		Vector<uint8_t> pv;
		pv.resize(16 * 4);
		for (int i = 0; i < 16; i++) {
			// Opaque white.
			pv.set(i * 4 + 0, 255);
			pv.set(i * 4 + 1, 255);
			pv.set(i * 4 + 2, 255);
			pv.set(i * 4 + 3, 255);
		}

		{
			Vector<Vector<uint8_t>> vpv;
			vpv.push_back(pv);
			default_rd_textures[DEFAULT_RD_TEXTURE_WHITE] =
				RD::texture_create(tformat, RDD::TextureView(), vpv);
		}

		for (int i = 0; i < 16; i++) {
			// Opaque black.
			pv.set(i * 4 + 0, 0);
			pv.set(i * 4 + 1, 0);
			pv.set(i * 4 + 2, 0);
			pv.set(i * 4 + 3, 255);
		}

		{
			Vector<Vector<uint8_t>> vpv;
			vpv.push_back(pv);
			default_rd_textures[DEFAULT_RD_TEXTURE_BLACK] =
				RD::texture_create(tformat, RDD::TextureView(), vpv);
		}

		for (int i = 0; i < 16; i++) {
			// Transparent black.
			pv.set(i * 4 + 0, 0);
			pv.set(i * 4 + 1, 0);
			pv.set(i * 4 + 2, 0);
			pv.set(i * 4 + 3, 0);
		}

		{
			Vector<Vector<uint8_t>> vpv;
			vpv.push_back(pv);
			default_rd_textures[DEFAULT_RD_TEXTURE_TRANSPARENT] =
				RD::texture_create(tformat, RDD::TextureView(), vpv);
		}

		for (int i = 0; i < 16; i++) {
			// Opaque normal map "flat" color.
			pv.set(i * 4 + 0, 128);
			pv.set(i * 4 + 1, 128);
			pv.set(i * 4 + 2, 255);
			pv.set(i * 4 + 3, 255);
		}

		{
			Vector<Vector<uint8_t>> vpv;
			vpv.push_back(pv);
			default_rd_textures[DEFAULT_RD_TEXTURE_NORMAL] =
				RD::texture_create(tformat, RDD::TextureView(), vpv);
		}

		for (int i = 0; i < 16; i++) {
			// Opaque flowmap "flat" color.
			pv.set(i * 4 + 0, 255);
			pv.set(i * 4 + 1, 128);
			pv.set(i * 4 + 2, 255);
			pv.set(i * 4 + 3, 255);
		}

		{
			Vector<Vector<uint8_t>> vpv;
			vpv.push_back(pv);
			default_rd_textures[DEFAULT_RD_TEXTURE_ANISO] =
				RD::texture_create(tformat, RDD::TextureView(), vpv);
		}

		{
			RDC::TextureFormat tf;
			tf.format = RDC::DATA_FORMAT_D16_UNORM;
			tf.width = 4;
			tf.height = 4;
			tf.usage_bits = RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_CAN_UPDATE_BIT |
							RDC::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
			tf.texture_type = RDC::TEXTURE_TYPE_2D;

			Vector<uint8_t> sv;
			sv.resize(16 * 2);
			uint16_t* ptr = (uint16_t*)sv.ptrw();
			for (int i = 0; i < 16; i++) {
				ptr[i] = Math::make_half_float(1.0f);
			}

			default_rd_textures[DEFAULT_RD_TEXTURE_DEPTH] =
				RD::texture_create(tf, RDD::TextureView());
			RD::texture_update(default_rd_textures[DEFAULT_RD_TEXTURE_DEPTH], 0, sv);
		}
	}

	{ // create default black cubemap array
		RDC::TextureFormat tformat;
		tformat.format = RDC::DATA_FORMAT_R8G8B8A8_UNORM;
		tformat.width = 4;
		tformat.height = 4;
		tformat.array_layers = 6;
		tformat.usage_bits = RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_CAN_UPDATE_BIT;
		tformat.texture_type = RDC::TEXTURE_TYPE_CUBE_ARRAY;

		Vector<uint8_t> pv;
		pv.resize(16 * 4);
		for (int i = 0; i < 16; i++) {
			pv.set(i * 4 + 0, 0);
			pv.set(i * 4 + 1, 0);
			pv.set(i * 4 + 2, 0);
			pv.set(i * 4 + 3, 255);
		}

		{
			Vector<Vector<uint8_t>> vpv;
			for (int i = 0; i < 6; i++) {
				vpv.push_back(pv);
			}
			default_rd_textures[DEFAULT_RD_TEXTURE_CUBEMAP_ARRAY_BLACK] =
				RD::texture_create(tformat, RDD::TextureView(), vpv);
		}
	}

	{ // create default white cubemap array
		RDC::TextureFormat tformat;
		tformat.format = RDC::DATA_FORMAT_R8G8B8A8_UNORM;
		tformat.width = 4;
		tformat.height = 4;
		tformat.array_layers = 6;
		tformat.usage_bits = RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_CAN_UPDATE_BIT;
		tformat.texture_type = RDC::TEXTURE_TYPE_CUBE_ARRAY;

		Vector<uint8_t> pv;
		pv.resize(16 * 4);
		for (int i = 0; i < 16; i++) {
			pv.set(i * 4 + 0, 255);
			pv.set(i * 4 + 1, 255);
			pv.set(i * 4 + 2, 255);
			pv.set(i * 4 + 3, 255);
		}

		{
			Vector<Vector<uint8_t>> vpv;
			for (int i = 0; i < 6; i++) {
				vpv.push_back(pv);
			}
			default_rd_textures[DEFAULT_RD_TEXTURE_CUBEMAP_ARRAY_WHITE] =
				RD::texture_create(tformat, RDD::TextureView(), vpv);
		}
	}

	{ // create default transparent cubemap array
		RDC::TextureFormat tformat;
		tformat.format = RDC::DATA_FORMAT_R8G8B8A8_UNORM;
		tformat.width = 4;
		tformat.height = 4;
		tformat.array_layers = 6;
		tformat.usage_bits = RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_CAN_UPDATE_BIT;
		tformat.texture_type = RDC::TEXTURE_TYPE_CUBE_ARRAY;

		Vector<uint8_t> pv;
		pv.resize(16 * 4);
		for (int i = 0; i < 16; i++) {
			pv.set(i * 4 + 0, 0);
			pv.set(i * 4 + 1, 0);
			pv.set(i * 4 + 2, 0);
			pv.set(i * 4 + 3, 0);
		}

		{
			Vector<Vector<uint8_t>> vpv;
			for (int i = 0; i < 6; i++) {
				vpv.push_back(pv);
			}
			default_rd_textures[DEFAULT_RD_TEXTURE_CUBEMAP_ARRAY_TRANSPARENT] =
				RD::texture_create(tformat, RDD::TextureView(), vpv);
		}
	}

	{ // create default black cubemap
		RDC::TextureFormat tformat;
		tformat.format = RDC::DATA_FORMAT_R8G8B8A8_UNORM;
		tformat.width = 4;
		tformat.height = 4;
		tformat.array_layers = 6;
		tformat.usage_bits = RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_CAN_UPDATE_BIT;
		tformat.texture_type = RDC::TEXTURE_TYPE_CUBE;

		Vector<uint8_t> pv;
		pv.resize(16 * 4);
		for (int i = 0; i < 16; i++) {
			pv.set(i * 4 + 0, 0);
			pv.set(i * 4 + 1, 0);
			pv.set(i * 4 + 2, 0);
			pv.set(i * 4 + 3, 255);
		}

		{
			Vector<Vector<uint8_t>> vpv;
			for (int i = 0; i < 6; i++) {
				vpv.push_back(pv);
			}
			default_rd_textures[DEFAULT_RD_TEXTURE_CUBEMAP_BLACK] =
				RD::texture_create(tformat, RDD::TextureView(), vpv);
		}
	}

	{ // create default white cubemap
		RDC::TextureFormat tformat;
		tformat.format = RDC::DATA_FORMAT_R8G8B8A8_UNORM;
		tformat.width = 4;
		tformat.height = 4;
		tformat.array_layers = 6;
		tformat.usage_bits = RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_CAN_UPDATE_BIT;
		tformat.texture_type = RDC::TEXTURE_TYPE_CUBE;

		Vector<uint8_t> pv;
		pv.resize(16 * 4);
		for (int i = 0; i < 16; i++) {
			pv.set(i * 4 + 0, 255);
			pv.set(i * 4 + 1, 255);
			pv.set(i * 4 + 2, 255);
			pv.set(i * 4 + 3, 255);
		}

		{
			Vector<Vector<uint8_t>> vpv;
			for (int i = 0; i < 6; i++) {
				vpv.push_back(pv);
			}
			default_rd_textures[DEFAULT_RD_TEXTURE_CUBEMAP_WHITE] =
				RD::texture_create(tformat, RDD::TextureView(), vpv);
		}
	}

	{ // create default transparent cubemap
		RDC::TextureFormat tformat;
		tformat.format = RDC::DATA_FORMAT_R8G8B8A8_UNORM;
		tformat.width = 4;
		tformat.height = 4;
		tformat.array_layers = 6;
		tformat.usage_bits = RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_CAN_UPDATE_BIT;
		tformat.texture_type = RDC::TEXTURE_TYPE_CUBE;

		Vector<uint8_t> pv;
		pv.resize(16 * 4);
		for (int i = 0; i < 16; i++) {
			pv.set(i * 4 + 0, 0);
			pv.set(i * 4 + 1, 0);
			pv.set(i * 4 + 2, 0);
			pv.set(i * 4 + 3, 0);
		}

		{
			Vector<Vector<uint8_t>> vpv;
			for (int i = 0; i < 6; i++) {
				vpv.push_back(pv);
			}
			default_rd_textures[DEFAULT_RD_TEXTURE_CUBEMAP_TRANSPARENT] =
				RD::texture_create(tformat, RDD::TextureView(), vpv);
		}
	}

	{ // create default 3D
		RDC::TextureFormat tformat;
		tformat.format = RDC::DATA_FORMAT_R8G8B8A8_UNORM;
		tformat.width = 4;
		tformat.height = 4;
		tformat.depth = 4;
		tformat.usage_bits = RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_CAN_UPDATE_BIT;
		tformat.texture_type = RDC::TEXTURE_TYPE_3D;

		Vector<uint8_t> pv;
		pv.resize(64 * 4);
		for (int i = 0; i < 64; i++) {
			pv.set(i * 4 + 0, 0);
			pv.set(i * 4 + 1, 0);
			pv.set(i * 4 + 2, 0);
			pv.set(i * 4 + 3, 255);
		}

		{
			Vector<Vector<uint8_t>> vpv;
			vpv.push_back(pv);
			default_rd_textures[DEFAULT_RD_TEXTURE_3D_BLACK] =
				RD::texture_create(tformat, RDD::TextureView(), vpv);
		}
		for (int i = 0; i < 64; i++) {
			pv.set(i * 4 + 0, 0);
			pv.set(i * 4 + 1, 0);
			pv.set(i * 4 + 2, 0);
			pv.set(i * 4 + 3, 0);
		}

		{
			Vector<Vector<uint8_t>> vpv;
			vpv.push_back(pv);
			default_rd_textures[DEFAULT_RD_TEXTURE_3D_TRANSPARENT] =
				RD::texture_create(tformat, RDD::TextureView(), vpv);
		}
		for (int i = 0; i < 64; i++) {
			pv.set(i * 4 + 0, 255);
			pv.set(i * 4 + 1, 255);
			pv.set(i * 4 + 2, 255);
			pv.set(i * 4 + 3, 255);
		}

		{
			Vector<Vector<uint8_t>> vpv;
			vpv.push_back(pv);
			default_rd_textures[DEFAULT_RD_TEXTURE_3D_WHITE] =
				RD::texture_create(tformat, RDD::TextureView(), vpv);
		}
	}

	{ // Creates a default 2D texture array filled with a white color.
		RDC::TextureFormat tformat;
		tformat.format = RDC::DATA_FORMAT_R8G8B8A8_UNORM;
		tformat.width = 4;
		tformat.height = 4;
		tformat.array_layers = 1;
		tformat.usage_bits = RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_CAN_UPDATE_BIT;
		tformat.texture_type = RDC::TEXTURE_TYPE_2D_ARRAY;

		Vector<uint8_t> pv;
		pv.resize(16 * 4);
		for (int i = 0; i < 16; i++) {
			pv.set(i * 4 + 0, 255);
			pv.set(i * 4 + 1, 255);
			pv.set(i * 4 + 2, 255);
			pv.set(i * 4 + 3, 255);
		}

		{
			Vector<Vector<uint8_t>> vpv;
			vpv.push_back(pv);
			default_rd_textures[DEFAULT_RD_TEXTURE_2D_ARRAY_WHITE] =
				RD::texture_create(tformat, RDD::TextureView(), vpv);
		}
	}

	{ // Creates a default 2D texture array filled with a black color.
		RDC::TextureFormat tformat;
		tformat.format = RDC::DATA_FORMAT_R8G8B8A8_UNORM;
		tformat.width = 4;
		tformat.height = 4;
		tformat.array_layers = 1;
		tformat.usage_bits = RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_CAN_UPDATE_BIT;
		tformat.texture_type = RDC::TEXTURE_TYPE_2D_ARRAY;

		Vector<uint8_t> pv;
		pv.resize(16 * 4);
		for (int i = 0; i < 16; i++) {
			pv.set(i * 4 + 0, 0);
			pv.set(i * 4 + 1, 0);
			pv.set(i * 4 + 2, 0);
			pv.set(i * 4 + 3, 255);
		}

		{
			Vector<Vector<uint8_t>> vpv;
			vpv.push_back(pv);
			default_rd_textures[DEFAULT_RD_TEXTURE_2D_ARRAY_BLACK] =
				RD::texture_create(tformat, RDD::TextureView(), vpv);
		}
	}

	{ // Creates a default 2D texture array filled with a transparent color.
		RDC::TextureFormat tformat;
		tformat.format = RDC::DATA_FORMAT_R8G8B8A8_UNORM;
		tformat.width = 4;
		tformat.height = 4;
		tformat.array_layers = 1;
		tformat.usage_bits = RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_CAN_UPDATE_BIT;
		tformat.texture_type = RDC::TEXTURE_TYPE_2D_ARRAY;

		Vector<uint8_t> pv;
		pv.resize(16 * 4);
		for (int i = 0; i < 16; i++) {
			pv.set(i * 4 + 0, 0);
			pv.set(i * 4 + 1, 0);
			pv.set(i * 4 + 2, 0);
			pv.set(i * 4 + 3, 0);
		}

		{
			Vector<Vector<uint8_t>> vpv;
			vpv.push_back(pv);
			default_rd_textures[DEFAULT_RD_TEXTURE_2D_ARRAY_TRANSPARENT] =
				RD::texture_create(tformat, RDD::TextureView(), vpv);
		}
	}

	{ // create default array normal
		RDC::TextureFormat tformat;
		tformat.format = RDC::DATA_FORMAT_R8G8B8A8_UNORM;
		tformat.width = 4;
		tformat.height = 4;
		tformat.array_layers = 1;
		tformat.usage_bits = RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_CAN_UPDATE_BIT;
		tformat.texture_type = RDC::TEXTURE_TYPE_2D_ARRAY;

		Vector<uint8_t> pv;
		pv.resize(16 * 4);
		for (int i = 0; i < 16; i++) {
			pv.set(i * 4 + 0, 128);
			pv.set(i * 4 + 1, 128);
			pv.set(i * 4 + 2, 255);
			pv.set(i * 4 + 3, 255);
		}

		{
			Vector<Vector<uint8_t>> vpv;
			vpv.push_back(pv);
			default_rd_textures[DEFAULT_RD_TEXTURE_2D_ARRAY_NORMAL] =
				RD::texture_create(tformat, RDD::TextureView(), vpv);
		}
	}

	{ // create default array depth
		RDC::TextureFormat tformat;
		tformat.format = RDC::DATA_FORMAT_D16_UNORM;
		tformat.width = 4;
		tformat.height = 4;
		tformat.array_layers = 1;
		tformat.usage_bits = RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_CAN_UPDATE_BIT |
							 RDC::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		tformat.texture_type = RDC::TEXTURE_TYPE_2D_ARRAY;

		Vector<uint8_t> sv;
		sv.resize(16 * 2);
		uint16_t* ptr = (uint16_t*)sv.ptrw();
		for (int i = 0; i < 16; i++) {
			ptr[i] = Math::make_half_float(1.0f);
		}

		default_rd_textures[DEFAULT_RD_TEXTURE_2D_ARRAY_DEPTH] =
			RD::texture_create(tformat, RDD::TextureView());
		RD::texture_update(default_rd_textures[DEFAULT_RD_TEXTURE_2D_ARRAY_DEPTH], 0, sv);
	}

	{ // default decal atlas texture
		RDC::TextureFormat tformat;
		tformat.format = RDC::DATA_FORMAT_R8G8B8A8_UNORM;
		tformat.width = 4;
		tformat.height = 4;
		tformat.usage_bits = RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_CAN_UPDATE_BIT;
		tformat.texture_type = RDC::TEXTURE_TYPE_2D;

		Vector<uint8_t> pv;
		pv.resize(16 * 4);
		for (int i = 0; i < 16; i++) {
			pv.set(i * 4 + 0, 0);
			pv.set(i * 4 + 1, 0);
			pv.set(i * 4 + 2, 0);
			pv.set(i * 4 + 3, 255);
		}

		Vector<Vector<uint8_t>> vpv;
		vpv.push_back(pv);
		decal_atlas.texture = RD::texture_create(tformat, RDD::TextureView(), vpv);
		decal_atlas.texture_srgb = decal_atlas.texture;
	}

	{ // default area light atlas texture
		RDC::TextureFormat tformat;
		tformat.format = RDC::DATA_FORMAT_R8G8B8A8_UNORM;
		tformat.width = 4;
		tformat.height = 4;
		tformat.usage_bits = RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_CAN_UPDATE_BIT;
		tformat.texture_type = RDC::TEXTURE_TYPE_2D;

		Vector<uint8_t> pv;
		pv.resize(16 * 4);
		for (int i = 0; i < 16; i++) {
			pv.set(i * 4 + 0, 0);
			pv.set(i * 4 + 1, 0);
			pv.set(i * 4 + 2, 0);
			pv.set(i * 4 + 3, 255);
		}

		Vector<Vector<uint8_t>> vpv;
		vpv.push_back(pv);
		area_light_atlas.texture = RD::texture_create(tformat, RDD::TextureView(), vpv);
	}

	{
		Vector<String> sdf_modes;
		sdf_modes.push_back("\n#define MODE_LOAD\n");
		sdf_modes.push_back("\n#define MODE_LOAD_SHRINK\n");
		sdf_modes.push_back("\n#define MODE_PROCESS\n");
		sdf_modes.push_back("\n#define MODE_PROCESS_OPTIMIZED\n");
		sdf_modes.push_back("\n#define MODE_STORE\n");
		sdf_modes.push_back("\n#define MODE_STORE_SHRINK\n");

		rt_sdf.shader.initialize(sdf_modes);
		rt_sdf.shader_version = rt_sdf.shader.version_create();
	}

	constexpr int placeholder_size = 4;
	texture_2d_placeholder =
		Image::create_empty(placeholder_size, placeholder_size, false, Image::FORMAT_RGBA8);
	for (int i = 0; i < placeholder_size * placeholder_size; i++) {
		const int x = i % placeholder_size;
		const int y = i / placeholder_size;
		texture_2d_placeholder->set_pixel(
			x, y, (x + y) % 2 == 0 ? Color(1, 0, 1) : Color(0, 0, 0));
	}

	texture_2d_array_placeholder.push_back(texture_2d_placeholder);

	for (int i = 0; i < 6; i++) {
		cubemap_placeholder.push_back(texture_2d_placeholder);
	}

	Ref<Image> texture_2d_placeholder_rotated;
	texture_2d_placeholder_rotated.instantiate();
	texture_2d_placeholder_rotated->copy_internals_from(texture_2d_placeholder);
	texture_2d_placeholder_rotated->rotate_90(CLOCKWISE);
	for (int i = 0; i < 4; i++) {
		texture_3d_placeholder.push_back(
			i % 2 == 0 ? texture_2d_placeholder : texture_2d_placeholder_rotated);
	}
}

void RendererRD::TextureStorage::texture_2d_update(
	RID p_texture, const Ref<Image>& p_image, int p_layer)
{
	ERR_FAIL_COND(p_image.is_null() || p_image->is_empty());

	Texture* tex = texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL(tex);
	ERR_FAIL_COND(tex->is_render_target);
	ERR_FAIL_COND(p_image->get_width() != tex->width || p_image->get_height() != tex->height);
	ERR_FAIL_COND(p_image->get_format() != tex->format);

	if (tex->type == TYPE_LAYERED) {
		ERR_FAIL_INDEX(p_layer, tex->layers);
	}

	tex->image_cache_2d.unref();
	tex->image = p_image;

	LocalTextureToRDFormat f;
	Ref<Image> validated = _map_image_to_rd_format(p_image, f);
	RD::texture_update(tex->rd_texture, p_layer, validated->get_data());
}

void RendererRD::TextureStorage::texture_3d_update(RID p_texture, const Vector<Ref<Image>>& p_data)
{
}

void RendererRD::TextureStorage::texture_drawable_initialize(RID p_texture, int p_width,
	int p_height, RenderingServerEnums::TextureDrawableFormat p_format, const Color& p_color,
	bool p_depth)
{
	Texture* texture = texture_owner.get_or_null(p_texture);
	ERR_FAIL_NULL(texture);

	if (texture->rd_texture.is_valid()) {
		free(texture->rd_texture);
		texture->rd_texture = RID();
	}

	RDC::TextureFormat rd_format;
	rd_format.width = p_width;
	rd_format.height = p_height;
	rd_format.depth = 1;
	rd_format.array_layers = 1;
	rd_format.mipmaps = 1;
	rd_format.texture_type = RDC::TEXTURE_TYPE_2D;
	rd_format.samples = RDC::TEXTURE_SAMPLES_1;

	RDD::TextureView rd_view;

	if (p_depth) {
		rd_format.format = RDC::DATA_FORMAT_D32_SFLOAT;
		rd_format.usage_bits = RDC::TEXTURE_USAGE_SAMPLING_BIT |
							   RDC::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
							   RDC::TEXTURE_USAGE_CAN_COPY_FROM_BIT;
	}
	else {
		switch (p_format) {
		case RenderingServerEnums::TEXTURE_DRAWABLE_FORMAT_RGBA8: {
			rd_format.format = RDC::DATA_FORMAT_R8G8B8A8_UNORM;
		} break;
		case RenderingServerEnums::TEXTURE_DRAWABLE_FORMAT_RGBA8_SRGB: {
			rd_format.format = RDC::DATA_FORMAT_R8G8B8A8_SRGB;
		} break;
		case RenderingServerEnums::TEXTURE_DRAWABLE_FORMAT_RGBAH: {
			rd_format.format = RDC::DATA_FORMAT_R16G16B16A16_SFLOAT;
		} break;
		case RenderingServerEnums::TEXTURE_DRAWABLE_FORMAT_RGBAF: {
			rd_format.format = RDC::DATA_FORMAT_R32G32B32A32_SFLOAT;
		} break;
		default: {
			rd_format.format = RDC::DATA_FORMAT_R8G8B8A8_UNORM;
		} break;
		}

		rd_format.usage_bits =
			RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT |
			RDC::TEXTURE_USAGE_STORAGE_BIT | RDC::TEXTURE_USAGE_CAN_COPY_FROM_BIT |
			RDC::TEXTURE_USAGE_CAN_COPY_TO_BIT | RDC::TEXTURE_USAGE_CAN_UPDATE_BIT;
	}

	Vector<Vector<uint8_t>> initial_data;

	if (!p_depth &&
		(p_color.r != 0.0f || p_color.g != 0.0f || p_color.b != 0.0f || p_color.a != 0.0f)) {
		Vector<uint8_t> buffer;
		buffer.resize(p_width * p_height * 4);
		uint8_t* w = buffer.ptrw();
		uint8_t r = uint8_t(CLAMP(p_color.r * 255.0f, 0.0f, 255.0f));
		uint8_t g = uint8_t(CLAMP(p_color.g * 255.0f, 0.0f, 255.0f));
		uint8_t b = uint8_t(CLAMP(p_color.b * 255.0f, 0.0f, 255.0f));
		uint8_t a = uint8_t(CLAMP(p_color.a * 255.0f, 0.0f, 255.0f));

		for (int i = 0; i < p_width * p_height; i++) {
			w[i * 4 + 0] = r;
			w[i * 4 + 1] = g;
			w[i * 4 + 2] = b;
			w[i * 4 + 3] = a
;
		}
		initial_data.push_back(buffer);
	}

	texture->rd_texture = RD::texture_create(rd_format, rd_view, initial_data);
	ERR_FAIL_COND(texture->rd_texture.is_null());

	texture->width = p_width;
	texture->height = p_height;
	texture->depth = 1;
	texture->layers = 1;
	texture->mipmaps = 1;
	texture->type = TextureType::TYPE_2D;
	texture->rd_type = rd_format.texture_type;
	texture->rd_format = rd_format.format;
	texture->is_render_target = true;
}

void RendererRD::TextureStorage::texture_set_format(RID p_texture, Image::Format p_format) {}

// ============================================================================
// Merged from servers/rendering/renderer_rd/storage_rd/light_storage.cpp
// ============================================================================

#include "core/config/project_settings.h"
#include "core/math/geometry_3d.h"
#include "core/os/os.h"
#include "light_storage.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/renderer_rd/renderer_scene_render_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/texture_storage.h"

using namespace RendererRD;

bool RendererRD::LightStorage::free(RID p_rid)
{
	if (owns_reflection_probe(p_rid)) {
		reflection_probe_free(p_rid);
		return true;
	}
	else if (owns_reflection_atlas(p_rid)) {
		reflection_atlas_free(p_rid);
		return true;
	}
	else if (owns_reflection_probe_instance(p_rid)) {
		reflection_probe_instance_free(p_rid);
		return true;
	}
	else if (owns_light(p_rid)) {
		light_free(p_rid);
		return true;
	}
	else if (owns_light_instance(p_rid)) {
		light_instance_free(p_rid);
		return true;
	}
	else if (owns_lightmap(p_rid)) {
		lightmap_free(p_rid);
		return true;
	}
	else if (owns_lightmap_instance(p_rid)) {
		lightmap_instance_free(p_rid);
		return true;
	}
	else if (owns_shadow_atlas(p_rid)) {
		shadow_atlas_free(p_rid);
		return true;
	}

	return false;
}

ForwardIDType RendererRD::LightStorage::_light_type_to_forward_id_type(RSE::LightType p_type)
{
	switch (p_type) {
	case RSE::LIGHT_OMNI: {
		return FORWARD_ID_TYPE_OMNI_LIGHT;
	} break;
	case RSE::LIGHT_SPOT: {
		return FORWARD_ID_TYPE_SPOT_LIGHT;
	} break;
	case RSE::LIGHT_AREA: {
		return FORWARD_ID_TYPE_AREA_LIGHT;
	} break;
	default: {
		CRASH_NOW_MSG("Supplied LightType has no equivalent forward type.");
	} break;
	}
	return FORWARD_ID_TYPE_OMNI_LIGHT; // unreachable
}

void RendererRD::LightStorage::_light_initialize(RID p_light, RSE::LightType p_type)
{
	Light light;
	light.type = p_type;

	light.param[RSE::LIGHT_PARAM_ENERGY] = 1.0;
	light.param[RSE::LIGHT_PARAM_INDIRECT_ENERGY] = 1.0;
	light.param[RSE::LIGHT_PARAM_VOLUMETRIC_FOG_ENERGY] = 1.0;
	light.param[RSE::LIGHT_PARAM_SPECULAR] = 0.5;
	light.param[RSE::LIGHT_PARAM_RANGE] = 1.0;
	light.param[RSE::LIGHT_PARAM_SIZE] = 0.0;
	light.param[RSE::LIGHT_PARAM_ATTENUATION] = 1.0;
	light.param[RSE::LIGHT_PARAM_SPOT_ANGLE] = 45;
	light.param[RSE::LIGHT_PARAM_SPOT_ATTENUATION] = 1.0;
	light.param[RSE::LIGHT_PARAM_SHADOW_MAX_DISTANCE] = 0;
	light.param[RSE::LIGHT_PARAM_SHADOW_SPLIT_1_OFFSET] = 0.1;
	light.param[RSE::LIGHT_PARAM_SHADOW_SPLIT_2_OFFSET] = 0.3;
	light.param[RSE::LIGHT_PARAM_SHADOW_SPLIT_3_OFFSET] = 0.6;
	light.param[RSE::LIGHT_PARAM_SHADOW_FADE_START] = 0.8;
	light.param[RSE::LIGHT_PARAM_SHADOW_NORMAL_BIAS] = 1.0;
	light.param[RSE::LIGHT_PARAM_SHADOW_BIAS] = 0.02;
	light.param[RSE::LIGHT_PARAM_SHADOW_OPACITY] = 1.0;
	light.param[RSE::LIGHT_PARAM_SHADOW_BLUR] = 0;
	light.param[RSE::LIGHT_PARAM_SHADOW_PANCAKE_SIZE] = 20.0;
	light.param[RSE::LIGHT_PARAM_TRANSMITTANCE_BIAS] = 0.05;
	light.param[RSE::LIGHT_PARAM_INTENSITY] = p_type == RSE::LIGHT_DIRECTIONAL ? 100000.0 : 1000.0;

	light_owner.initialize_rid(p_light, light);
}

RID RendererRD::LightStorage::directional_light_allocate() { return light_owner.allocate_rid(); }

void RendererRD::LightStorage::directional_light_initialize(RID p_light)
{
	_light_initialize(p_light, RSE::LIGHT_DIRECTIONAL);
}

RID RendererRD::LightStorage::omni_light_allocate() { return light_owner.allocate_rid(); }

void RendererRD::LightStorage::omni_light_initialize(RID p_light)
{
	_light_initialize(p_light, RSE::LIGHT_OMNI);
}

RID RendererRD::LightStorage::spot_light_allocate() { return light_owner.allocate_rid(); }

void RendererRD::LightStorage::spot_light_initialize(RID p_light)
{
	_light_initialize(p_light, RSE::LIGHT_SPOT);
}

RID RendererRD::LightStorage::area_light_allocate() { return light_owner.allocate_rid(); }

void RendererRD::LightStorage::area_light_initialize(RID p_rid)
{
	_light_initialize(p_rid, RSE::LIGHT_AREA);
}

void RendererRD::LightStorage::light_free(RID p_rid)
{
	light_set_projector(p_rid, RID());	  // clear projector
	light_area_set_texture(p_rid, RID()); // clear area texture

	// delete the texture
	Light* light = light_owner.get_or_null(p_rid);
	light->dependency.deleted_notify(p_rid);
	light_owner.free(p_rid);
}

void RendererRD::LightStorage::light_set_color(RID p_light, const Color& p_color)
{
	Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(light);

	light->color = p_color;
}

void RendererRD::LightStorage::light_set_param(RID p_light, RSE::LightParam p_param, float p_value)
{
	Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(light);
	ERR_FAIL_INDEX(p_param, RSE::LIGHT_PARAM_MAX);

	if (light->param[p_param] == p_value) {
		return;
	}

	switch (p_param) {
	case RSE::LIGHT_PARAM_RANGE:
	case RSE::LIGHT_PARAM_SPOT_ANGLE:
	case RSE::LIGHT_PARAM_SHADOW_MAX_DISTANCE:
	case RSE::LIGHT_PARAM_SHADOW_SPLIT_1_OFFSET:
	case RSE::LIGHT_PARAM_SHADOW_SPLIT_2_OFFSET:
	case RSE::LIGHT_PARAM_SHADOW_SPLIT_3_OFFSET:
	case RSE::LIGHT_PARAM_SHADOW_NORMAL_BIAS:
	case RSE::LIGHT_PARAM_SHADOW_PANCAKE_SIZE:
	case RSE::LIGHT_PARAM_SHADOW_BIAS: {
		light->version++;
		light->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_LIGHT);
	} break;
	case RSE::LIGHT_PARAM_SIZE: {
		if ((light->param[p_param] > CMP_EPSILON) != (p_value > CMP_EPSILON)) {
			// changing from no size to size and the opposite
			light->dependency.changed_notify(
				Dependency::DEPENDENCY_CHANGED_LIGHT_SOFT_SHADOW_AND_PROJECTOR);
		}
	} break;
	default: {
	}
	}

	light->param[p_param] = p_value;
}

void RendererRD::LightStorage::light_set_shadow(RID p_light, bool p_enabled)
{
	Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(light);
	light->shadow = p_enabled;

	light->version++;
	light->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_LIGHT);
}

void RendererRD::LightStorage::light_set_projector(RID p_light, RID p_texture)
{
	Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(light);

	if (light->projector == p_texture) {
		return;
	}

	ERR_FAIL_COND(p_texture.is_valid() && !RendererRD::TextureStorage::owns_texture(p_texture));

	if (light->type != RSE::LIGHT_DIRECTIONAL && light->projector.is_valid()) {
		RendererRD::TextureStorage::texture_remove_from_decal_atlas(
			light->projector, light->type == RSE::LIGHT_OMNI);
	}

	light->projector = p_texture;

	if (light->type != RSE::LIGHT_DIRECTIONAL) {
		if (light->projector.is_valid()) {
			RendererRD::TextureStorage::texture_add_to_decal_atlas(
				light->projector, light->type == RSE::LIGHT_OMNI);
		}
		light->dependency.changed_notify(
			Dependency::DEPENDENCY_CHANGED_LIGHT_SOFT_SHADOW_AND_PROJECTOR);
	}
}

void RendererRD::LightStorage::light_set_negative(RID p_light, bool p_enable)
{
	Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(light);

	light->negative = p_enable;
}

void RendererRD::LightStorage::light_set_cull_mask(RID p_light, uint32_t p_mask)
{
	Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(light);

	light->cull_mask = p_mask;

	light->version++;
	light->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_CULL_MASK);
}

void RendererRD::LightStorage::light_set_distance_fade(
	RID p_light, bool p_enabled, float p_begin, float p_shadow, float p_length)
{
	Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(light);

	light->distance_fade = p_enabled;
	light->distance_fade_begin = p_begin;
	light->distance_fade_shadow = p_shadow;
	light->distance_fade_length = p_length;
}

void RendererRD::LightStorage::light_set_reverse_cull_face_mode(RID p_light, bool p_enabled)
{
	Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(light);

	light->reverse_cull = p_enabled;

	light->version++;
	light->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_LIGHT);
}

void RendererRD::LightStorage::light_set_shadow_caster_mask(RID p_light, uint32_t p_caster_mask)
{
	Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(light);

	light->shadow_caster_mask = p_caster_mask;

	light->version++;
	light->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_LIGHT);
}

uint32_t RendererRD::LightStorage::light_get_shadow_caster_mask(RID p_light)
{
	Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL_V(light, 0);

	return light->shadow_caster_mask;
}

void RendererRD::LightStorage::light_set_bake_mode(RID p_light, RSE::LightBakeMode p_bake_mode)
{
	Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(light);

	light->bake_mode = p_bake_mode;

	light->version++;
	light->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_LIGHT);
}

void RendererRD::LightStorage::light_set_max_sdfgi_cascade(RID p_light, uint32_t p_cascade)
{
	Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(light);

	light->max_sdfgi_cascade = p_cascade;

	light->version++;
	light->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_LIGHT);
}

void RendererRD::LightStorage::light_omni_set_shadow_mode(
	RID p_light, RSE::LightOmniShadowMode p_mode)
{
	Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(light);

	light->omni_shadow_mode = p_mode;

	light->version++;
	light->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_LIGHT);

	if (p_mode == RSE::LIGHT_OMNI_SHADOW_DUAL_PARABOLOID) {
		shadow_dual_paraboloid_used = true;
	}
	else if (p_mode == RSE::LIGHT_OMNI_SHADOW_CUBE) {
		shadow_cubemaps_used = true;
	}
}

RSE::LightOmniShadowMode RendererRD::LightStorage::light_omni_get_shadow_mode(RID p_light)
{
	const Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL_V(light, RSE::LIGHT_OMNI_SHADOW_CUBE);

	return light->omni_shadow_mode;
}

void RendererRD::LightStorage::light_directional_set_shadow_mode(
	RID p_light, RSE::LightDirectionalShadowMode p_mode)
{
	Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(light);

	light->directional_shadow_mode = p_mode;
	light->version++;
	light->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_LIGHT);
}

void RendererRD::LightStorage::light_directional_set_blend_splits(RID p_light, bool p_enable)
{
	Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(light);

	light->directional_blend_splits = p_enable;
	light->version++;
	light->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_LIGHT);
}

bool RendererRD::LightStorage::light_directional_get_blend_splits(RID p_light)
{
	const Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL_V(light, false);

	return light->directional_blend_splits;
}

void RendererRD::LightStorage::light_directional_set_sky_mode(
	RID p_light, RSE::LightDirectionalSkyMode p_mode)
{
	Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(light);

	light->directional_sky_mode = p_mode;
}

RSE::LightDirectionalSkyMode RendererRD::LightStorage::light_directional_get_sky_mode(RID p_light)
{
	const Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL_V(light, RSE::LIGHT_DIRECTIONAL_SKY_MODE_LIGHT_AND_SKY);

	return light->directional_sky_mode;
}

RSE::LightDirectionalShadowMode RendererRD::LightStorage::light_directional_get_shadow_mode(
	RID p_light)
{
	const Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL_V(light, RSE::LIGHT_DIRECTIONAL_SHADOW_ORTHOGONAL);

	return light->directional_shadow_mode;
}

void RendererRD::LightStorage::light_area_set_size(RID p_light, const Vector2& p_size)
{
	Light* light = light_owner.get_or_null(p_light);
	light->area_size = p_size.maxf(0.0f);
	// The range in which objects are illuminated change, so the z-range of the shadow map needs to
	// adjust accordingly.
	light->version++;
	light->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_LIGHT);
}

Vector2 RendererRD::LightStorage::light_area_get_size(RID p_light)
{
	const Light* light = light_owner.get_or_null(p_light);
	return light->area_size;
}

void RendererRD::LightStorage::light_area_set_normalize_energy(RID p_light, bool p_enabled)
{
	Light* light = light_owner.get_or_null(p_light);
	light->area_normalize_energy = p_enabled;
}

bool RendererRD::LightStorage::light_area_get_normalize_energy(RID p_light)
{
	const Light* light = light_owner.get_or_null(p_light);
	return light->area_normalize_energy;
}

void RendererRD::LightStorage::light_area_set_texture(RID p_light, RID p_texture)
{
	Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL(light);

	if (light->area_texture == p_texture) {
		return;
	}

	ERR_FAIL_COND(p_texture.is_valid() && !RendererRD::TextureStorage::owns_texture(p_texture));

	if (light->area_texture.is_valid()) {
		RendererRD::TextureStorage::texture_remove_from_area_light_atlas(light->area_texture);
	}

	light->area_texture = p_texture;

	if (light->area_texture.is_valid()) {
		RendererRD::TextureStorage::texture_add_to_area_light_atlas(light->area_texture);
	}
	light->dependency.changed_notify(
		Dependency::DEPENDENCY_CHANGED_LIGHT_SOFT_SHADOW_AND_PROJECTOR);
}

RID RendererRD::LightStorage::light_area_get_texture(RID p_light)
{
	const Light* light = light_owner.get_or_null(p_light);
	return light->area_texture;
}

uint32_t RendererRD::LightStorage::light_get_max_sdfgi_cascade(RID p_light)
{
	const Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL_V(light, 0);

	return light->max_sdfgi_cascade;
}

RSE::LightBakeMode RendererRD::LightStorage::light_get_bake_mode(RID p_light)
{
	const Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL_V(light, RSE::LIGHT_BAKE_DISABLED);

	return light->bake_mode;
}

uint64_t RendererRD::LightStorage::light_get_version(RID p_light)
{
	const Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL_V(light, 0);

	return light->version;
}

uint32_t RendererRD::LightStorage::light_get_cull_mask(RID p_light)
{
	const Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL_V(light, 0);

	return light->cull_mask;
}

AABB RendererRD::LightStorage::light_get_aabb(RID p_light)
{
	const Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL_V(light, AABB());

	switch (light->type) {
	case RSE::LIGHT_SPOT: {
		float len = light->param[RSE::LIGHT_PARAM_RANGE];
		float angle = Math::deg_to_rad(light->param[RSE::LIGHT_PARAM_SPOT_ANGLE]);

		if (angle > Math::PI * 0.5) {
			// Light casts backwards as well.
			return AABB(Vector3(-1, -1, -1) * len, Vector3(2, 2, 2) * len);
		}

		float size = Math::sin(angle) * len;
		return AABB(Vector3(-size, -size, -len), Vector3(size * 2, size * 2, len));
	};
	case RSE::LIGHT_OMNI: {
		float r = light->param[RSE::LIGHT_PARAM_RANGE];
		return AABB(-Vector3(r, r, r), Vector3(r, r, r) * 2);
	};
	case RSE::LIGHT_AREA: {
		float len = light->param[RSE::LIGHT_PARAM_RANGE];
		float width = light->area_size.x / 2.0 + len;
		float height = light->area_size.y / 2.0 + len;
		return AABB(-Vector3(width, height, 0), Vector3(width * 2, height * 2, -len));
	};
	case RSE::LIGHT_DIRECTIONAL: {
		return AABB();
	};
	}

	ERR_FAIL_V(AABB());
}

Dependency* RendererRD::LightStorage::light_get_dependency(RID p_light)
{
	Light* light = light_owner.get_or_null(p_light);
	ERR_FAIL_NULL_V(light, nullptr);

	return &light->dependency;
}

RID RendererRD::LightStorage::light_instance_create(RID p_light)
{
	RID li = light_instance_owner.make_rid(LightInstance());

	LightInstance* light_instance = light_instance_owner.get_or_null(li);

	light_instance->self = li;
	light_instance->light = p_light;
	light_instance->light_type = light_get_type(p_light);
	if (light_instance->light_type != RSE::LIGHT_DIRECTIONAL) {
		ForwardIDType forward_id_type = _light_type_to_forward_id_type(light_instance->light_type);
		light_instance->forward_id = ForwardIDStorage::allocate_forward_id(forward_id_type);
	}

	return li;
}

void RendererRD::LightStorage::light_instance_free(RID p_light)
{
	LightInstance* light_instance = light_instance_owner.get_or_null(p_light);

	// remove from shadow atlases..
	for (const RID& E : light_instance->shadow_atlases) {
		ShadowAtlas* shadow_atlas = shadow_atlas_owner.get_or_null(E);
		ERR_CONTINUE(!shadow_atlas->shadow_owners.has(p_light));
		uint32_t key = shadow_atlas->shadow_owners[p_light];
		uint32_t q = (key >> QUADRANT_SHIFT) & 0x3;
		uint32_t s = key & SHADOW_INDEX_MASK;

		shadow_atlas->quadrants[q].shadows.write[s].owner = RID();

		if (key & OMNI_LIGHT_FLAG) {
			// Omni lights use two atlas spots, make sure to clear the other as well
			shadow_atlas->quadrants[q].shadows.write[s + 1].owner = RID();
		}

		shadow_atlas->shadow_owners.erase(p_light);
	}

	if (light_instance->light_type != RSE::LIGHT_DIRECTIONAL) {
		ForwardIDType forward_id_type = _light_type_to_forward_id_type(light_instance->light_type);
		ForwardIDStorage::free_forward_id(forward_id_type, light_instance->forward_id);
	}
	light_instance_owner.free(p_light);
}

void RendererRD::LightStorage::light_instance_set_transform(
	RID p_light_instance, const Transform3D& p_transform)
{
	LightInstance* light_instance = light_instance_owner.get_or_null(p_light_instance);
	ERR_FAIL_NULL(light_instance);

	light_instance->transform = p_transform;
}

void RendererRD::LightStorage::light_instance_set_aabb(RID p_light_instance, const AABB& p_aabb)
{
	LightInstance* light_instance = light_instance_owner.get_or_null(p_light_instance);
	ERR_FAIL_NULL(light_instance);

	light_instance->aabb = p_aabb;
}

void RendererRD::LightStorage::light_instance_set_shadow_transform(RID p_light_instance,
	const Projection& p_projection, const Transform3D& p_transform, float p_far, float p_split,
	int p_pass, float p_shadow_texel_size, float p_bias_scale, float p_range_begin,
	const Vector2& p_uv_scale)
{
	LightInstance* light_instance = light_instance_owner.get_or_null(p_light_instance);
	ERR_FAIL_NULL(light_instance);

	ERR_FAIL_INDEX(p_pass, 6);

	light_instance->shadow_transform[p_pass].camera = p_projection;
	light_instance->shadow_transform[p_pass].transform = p_transform;
	light_instance->shadow_transform[p_pass].farplane = p_far;
	light_instance->shadow_transform[p_pass].split = p_split;
	light_instance->shadow_transform[p_pass].bias_scale = p_bias_scale;
	light_instance->shadow_transform[p_pass].range_begin = p_range_begin;
	light_instance->shadow_transform[p_pass].shadow_texel_size = p_shadow_texel_size;
	light_instance->shadow_transform[p_pass].uv_scale = p_uv_scale;
}

void RendererRD::LightStorage::light_instance_mark_visible(RID p_light_instance)
{
	LightInstance* light_instance = light_instance_owner.get_or_null(p_light_instance);
	ERR_FAIL_NULL(light_instance);

	light_instance->last_scene_pass = RendererSceneRenderRD::get_scene_pass();
}

void RendererRD::LightStorage::free_light_data()
{
	if (directional_light_buffer.is_valid()) {
		RD::free_rid(directional_light_buffer);
		directional_light_buffer = RID();
	}

	if (omni_light_buffer.is_valid()) {
		RD::free_rid(omni_light_buffer);
		omni_light_buffer = RID();
	}

	if (spot_light_buffer.is_valid()) {
		RD::free_rid(spot_light_buffer);
		spot_light_buffer = RID();
	}

	if (area_light_buffer.is_valid()) {
		RD::free_rid(area_light_buffer);
		area_light_buffer = RID();
	}

	if (directional_lights != nullptr) {
		memdelete_arr(directional_lights);
		directional_lights = nullptr;
	}

	if (omni_lights != nullptr) {
		memdelete_arr(omni_lights);
		omni_lights = nullptr;
	}

	if (spot_lights != nullptr) {
		memdelete_arr(spot_lights);
		spot_lights = nullptr;
	}

	if (area_lights != nullptr) {
		memdelete_arr(area_lights);
		area_lights = nullptr;
	}

	if (omni_light_sort != nullptr) {
		memdelete_arr(omni_light_sort);
		omni_light_sort = nullptr;
	}

	if (spot_light_sort != nullptr) {
		memdelete_arr(spot_light_sort);
		spot_light_sort = nullptr;
	}

	if (area_light_sort != nullptr) {
		memdelete_arr(area_light_sort);
		area_light_sort = nullptr;
	}
}

void RendererRD::LightStorage::update_light_buffers(RenderDataRD* p_render_data,
	const PagedArray<RID>& p_lights, const Transform3D& p_camera_transform, RID p_shadow_atlas,
	bool p_using_shadows, uint32_t& r_directional_light_count, uint32_t& r_positional_light_count,
	bool& r_directional_light_soft_shadows)
{
	Transform3D inverse_transform = p_camera_transform.affine_inverse();

	r_directional_light_count = 0;
	r_positional_light_count = 0;

	omni_light_count = 0;
	spot_light_count = 0;
	area_light_count = 0;

	r_directional_light_soft_shadows = false;

	for (int i = 0; i < (int)p_lights.size(); i++) {
		LightInstance* light_instance = light_instance_owner.get_or_null(p_lights[i]);
		if (!light_instance) {
			continue;
		}
		Light* light = light_owner.get_or_null(light_instance->light);

		ERR_CONTINUE(light == nullptr);

		switch (light->type) {
		case RSE::LIGHT_DIRECTIONAL: {
			if (r_directional_light_count >= max_directional_lights ||
				light->directional_sky_mode == RSE::LIGHT_DIRECTIONAL_SKY_MODE_SKY_ONLY) {
				continue;
			}

			DirectionalLightData& light_data = directional_lights[r_directional_light_count];

			Transform3D light_transform = light_instance->transform;

			Vector3 direction =
				inverse_transform.basis.xform(light_transform.basis.xform(Vector3(0, 0, 1)))
					.normalized();

			light_data.direction[0] = direction.x;
			light_data.direction[1] = direction.y;
			light_data.direction[2] = direction.z;

			float sign = light->negative ? -1 : 1;

			light_data.energy = sign * light->param[RSE::LIGHT_PARAM_ENERGY];

			if (RendererSceneRenderRD::is_using_physical_light_units()) {
				light_data.energy *= light->param[RSE::LIGHT_PARAM_INTENSITY];
			}
			else {
				light_data.energy *= Math::PI;
			}

			if (p_render_data->camera_attributes.is_valid()) {
				light_data.energy *=
					RendererCameraAttributes::camera_attributes_get_exposure_normalization_factor(
						p_render_data->camera_attributes);
			}

			Color linear_col = light->color.srgb_to_linear();
			light_data.color[0] = linear_col.r;
			light_data.color[1] = linear_col.g;
			light_data.color[2] = linear_col.b;

			light_data.specular = light->param[RSE::LIGHT_PARAM_SPECULAR];
			light_data.volumetric_fog_energy = light->param[RSE::LIGHT_PARAM_VOLUMETRIC_FOG_ENERGY];
			light_data.mask = light->cull_mask;

			float size = light->param[RSE::LIGHT_PARAM_SIZE];

			light_data.size = 1.0 - Math::cos(Math::deg_to_rad(size)); // angle to cosine offset

			light_data.shadow_opacity = (p_using_shadows && light->shadow)
											? light->param[RSE::LIGHT_PARAM_SHADOW_OPACITY]
											: 0.0;

			float angular_diameter = light->param[RSE::LIGHT_PARAM_SIZE];
			if (angular_diameter > 0.0) {
				// I know tan(0) is 0, but let's not risk it with numerical precision.
				// technically this will keep expanding until reaching the sun, but all we care
				// is expand until we reach the radius of the near plane (there can't be more
				// occluders than that)
				angular_diameter = Math::tan(Math::deg_to_rad(angular_diameter));
				if (light->shadow && light->param[RSE::LIGHT_PARAM_SHADOW_BLUR] > 0.0) {
					// Only enable PCSS-like soft shadows if blurring is enabled.
					// Otherwise, performance would decrease with no visual difference.
					r_directional_light_soft_shadows = true;
				}
			}
			else {
				angular_diameter = 0.0;
			}

			light_data.bake_mode = light->bake_mode;

			if (light_data.shadow_opacity > 0.001) {
				RSE::LightDirectionalShadowMode smode = light->directional_shadow_mode;

				light_data.soft_shadow_scale = light->param[RSE::LIGHT_PARAM_SHADOW_BLUR];
				light_data.softshadow_angle = angular_diameter;

				if (angular_diameter <= 0.0) {
					light_data.soft_shadow_scale *= RendererSceneRenderRD::
						directional_shadow_quality_radius_get(); // Only use quality radius
																 // for PCF
				}

				int limit =
					smode == RSE::LIGHT_DIRECTIONAL_SHADOW_ORTHOGONAL
						? 0
						: (smode == RSE::LIGHT_DIRECTIONAL_SHADOW_PARALLEL_2_SPLITS ? 1 : 3);
				light_data.blend_splits = (smode != RSE::LIGHT_DIRECTIONAL_SHADOW_ORTHOGONAL) &&
										  light->directional_blend_splits;
				for (int j = 0; j < 4; j++) {
					Rect2 atlas_rect = light_instance->shadow_transform[j].atlas_rect;
					Projection correction;
					correction.set_depth_correction(false, true, false);
					Projection matrix = correction * light_instance->shadow_transform[j].camera;
					float split = light_instance->shadow_transform[MIN(limit, j)].split;

					Projection bias;
					bias.set_light_bias();
					Projection rectm;
					rectm.set_light_atlas_rect(atlas_rect);

					Transform3D modelview =
						(inverse_transform * light_instance->shadow_transform[j].transform)
							.inverse();

					Projection shadow_mtx = rectm * bias * matrix * modelview;
					light_data.shadow_split_offsets[j] = split;
					float bias_scale = light_instance->shadow_transform[j].bias_scale *
									   light_data.soft_shadow_scale;
					light_data.shadow_bias[j] =
						light->param[RSE::LIGHT_PARAM_SHADOW_BIAS] / 100.0 * bias_scale;
					light_data.shadow_normal_bias[j] =
						light->param[RSE::LIGHT_PARAM_SHADOW_NORMAL_BIAS] *
						light_instance->shadow_transform[j].shadow_texel_size;
					light_data.shadow_transmittance_bias[j] =
						light->param[RSE::LIGHT_PARAM_TRANSMITTANCE_BIAS] / 100.0 * bias_scale;
					light_data.shadow_z_range[j] = light_instance->shadow_transform[j].farplane;
					light_data.shadow_range_begin[j] =
						light_instance->shadow_transform[j].range_begin;
					RendererRD::MaterialStorage::store_camera(
						shadow_mtx, light_data.shadow_matrices[j]);

					Vector2 uv_scale = light_instance->shadow_transform[j].uv_scale;
					uv_scale *= atlas_rect.size; // adapt to atlas size
					switch (j) {
					case 0: {
						light_data.uv_scale1[0] = uv_scale.x;
						light_data.uv_scale1[1] = uv_scale.y;
					} break;
					case 1: {
						light_data.uv_scale2[0] = uv_scale.x;
						light_data.uv_scale2[1] = uv_scale.y;
					} break;
					case 2: {
						light_data.uv_scale3[0] = uv_scale.x;
						light_data.uv_scale3[1] = uv_scale.y;
					} break;
					case 3: {
						light_data.uv_scale4[0] = uv_scale.x;
						light_data.uv_scale4[1] = uv_scale.y;
					} break;
					}
				}

				float fade_start = light->param[RSE::LIGHT_PARAM_SHADOW_FADE_START];
				light_data.fade_from = -light_data.shadow_split_offsets[3] *
									   MIN(fade_start, 0.999); // using 1.0 would break smoothstep
				light_data.fade_to = -light_data.shadow_split_offsets[3];
			}

			r_directional_light_count++;
		} break;
		case RSE::LIGHT_OMNI: {
			if (omni_light_count >= max_lights) {
				continue;
			}

			Transform3D light_transform = light_instance->transform;
			const real_t distance = p_camera_transform.origin.distance_to(light_transform.origin);

			if (light->distance_fade) {
				const float fade_begin = light->distance_fade_begin;
				const float fade_length = light->distance_fade_length;

				if (distance > fade_begin) {
					if (distance > fade_begin + fade_length) {
						// Out of range, don't draw this light to improve performance.
						continue;
					}
				}
			}

			omni_light_sort[omni_light_count].light_instance = light_instance;
			omni_light_sort[omni_light_count].light = light;
			omni_light_sort[omni_light_count].depth = distance;
			omni_light_count++;
		} break;
		case RSE::LIGHT_SPOT: {
			if (spot_light_count >= max_lights) {
				continue;
			}

			Transform3D light_transform = light_instance->transform;
			const real_t distance = p_camera_transform.origin.distance_to(light_transform.origin);

			if (light->distance_fade) {
				const float fade_begin = light->distance_fade_begin;
				const float fade_length = light->distance_fade_length;

				if (distance > fade_begin) {
					if (distance > fade_begin + fade_length) {
						// Out of range, don't draw this light to improve performance.
						continue;
					}
				}
			}

			spot_light_sort[spot_light_count].light_instance = light_instance;
			spot_light_sort[spot_light_count].light = light;
			spot_light_sort[spot_light_count].depth = distance;
			spot_light_count++;
		} break;
		case RSE::LIGHT_AREA: {
			if (area_light_count >= max_lights) {
				continue;
			}

			Transform3D light_transform = light_instance->transform;
			const real_t distance = p_camera_transform.origin.distance_to(light_transform.origin);

			if (light->distance_fade) {
				const float fade_begin = light->distance_fade_begin;
				const float fade_length = light->distance_fade_length;

				if (distance > fade_begin) {
					if (distance > fade_begin + fade_length) {
						// Out of range, don't draw this light, to improve performance.
						continue;
					}
				}
			}

			area_light_sort[area_light_count].light_instance = light_instance;
			area_light_sort[area_light_count].light = light;
			area_light_sort[area_light_count].depth = distance;
			area_light_count++;
		}
		}

		light_instance->last_pass = RendererCompositor::get_frame_number();
	}

	if (omni_light_count) {
		SortArray<LightInstanceDepthSort> sorter;
		sorter.sort(omni_light_sort, omni_light_count);
	}

	if (spot_light_count) {
		SortArray<LightInstanceDepthSort> sorter;
		sorter.sort(spot_light_sort, spot_light_count);
	}

	if (area_light_count) {
		SortArray<LightInstanceDepthSort> sorter;
		sorter.sort(area_light_sort, area_light_count);
	}

	bool using_forward_ids = ForwardIDStorage::uses_forward_ids();

	for (uint32_t i = 0; i < (omni_light_count + spot_light_count + area_light_count); i++) {
		uint32_t index;
		LightData* light_data_ptr;
		RSE::LightType type;
		LightInstance* light_instance;
		Light* light;
		real_t distance;

		if (i >= omni_light_count + spot_light_count) {
			index = i - (omni_light_count + spot_light_count);
			light_data_ptr = &area_lights[index];
			type = RSE::LIGHT_AREA;
			light_instance = area_light_sort[index].light_instance;
			light = area_light_sort[index].light;
			distance = area_light_sort[index].depth;
		}
		else if (i >= omni_light_count) {
			index = i - (omni_light_count);
			light_data_ptr = &spot_lights[index];
			type = RSE::LIGHT_SPOT;
			light_instance = spot_light_sort[index].light_instance;
			light = spot_light_sort[index].light;
			distance = spot_light_sort[index].depth;
		}
		else {
			index = i;
			light_data_ptr = &omni_lights[index];
			type = RSE::LIGHT_OMNI;
			light_instance = omni_light_sort[index].light_instance;
			light = omni_light_sort[index].light;
			distance = omni_light_sort[index].depth;
		}

		if (using_forward_ids) {
			ForwardIDType forward_id_type =
				_light_type_to_forward_id_type(light_instance->light_type);
			ForwardIDStorage::map_forward_id(
				forward_id_type, light_instance->forward_id, index, light_instance->last_pass);
		}

		LightData& light_data = *light_data_ptr;
		Transform3D light_transform = light_instance->transform;

		float sign = light->negative ? -1 : 1;
		Color linear_col = light->color.srgb_to_linear();

		light_data.attenuation = light->param[RSE::LIGHT_PARAM_ATTENUATION];

		// Reuse fade begin, fade length and distance for shadow LOD determination later.
		float fade_begin = 0.0;
		float fade_shadow = 0.0;
		float fade_length = 0.0;

		float fade = 1.0;
		float shadow_opacity_fade = 1.0;
		if (light->distance_fade) {
			fade_begin = light->distance_fade_begin;
			fade_shadow = light->distance_fade_shadow;
			fade_length = light->distance_fade_length;

			// Use `smoothstep()` to make opacity changes more gradual and less noticeable to the
			// player.
			if (distance > fade_begin) {
				fade =
					Math::smoothstep(0.0f, 1.0f, 1.0f - float(distance - fade_begin) / fade_length);
			}

			if (distance > fade_shadow) {
				shadow_opacity_fade = Math::smoothstep(
					0.0f, 1.0f, 1.0f - float(distance - fade_shadow) / fade_length);
			}
		}

		float energy = sign * light->param[RSE::LIGHT_PARAM_ENERGY] * fade;

		if (RendererSceneRenderRD::is_using_physical_light_units()) {
			energy *= light->param[RSE::LIGHT_PARAM_INTENSITY];

			// Convert from Luminous Power to Luminous Intensity
			if (type == RSE::LIGHT_OMNI) {
				energy *= 1.0 / (Math::PI * 4.0);
			}
			else if (type == RSE::LIGHT_AREA) {
				energy *= 1.0 / (Math::PI * 2.0);
			}
			else {
				// Spot Lights are not physically accurate, Luminous Intensity should change in
				// relation to the cone angle. We make this assumption to keep them easy to control.
				energy *= 1.0 / Math::PI;
			}
		}
		else {
			energy *= Math::PI;
		}

		if (p_render_data->camera_attributes.is_valid()) {
			energy *= RendererCameraAttributes::camera_attributes_get_exposure_normalization_factor(
				p_render_data->camera_attributes);
		}

		light_data.color[0] = linear_col.r * energy;
		light_data.color[1] = linear_col.g * energy;
		light_data.color[2] = linear_col.b * energy;
		light_data.specular_amount = light->param[RSE::LIGHT_PARAM_SPECULAR] * 2.0;
		light_data.volumetric_fog_energy = light->param[RSE::LIGHT_PARAM_VOLUMETRIC_FOG_ENERGY];
		light_data.bake_mode = light->bake_mode;

		float radius = MAX(0.001, light->param[RSE::LIGHT_PARAM_RANGE]);
		light_data.inv_radius = 1.0 / radius;
		Vector2 area_size = light->area_size;
		Vector3 pos = inverse_transform.xform(light_transform.origin);
		light_data.position[0] = pos.x;
		light_data.position[1] = pos.y;
		light_data.position[2] = pos.z;

		Vector3 direction =
			inverse_transform.basis.xform(light_transform.basis.xform(Vector3(0, 0, -1)))
				.normalized();

		light_data.direction[0] = direction.x;
		light_data.direction[1] = direction.y;
		light_data.direction[2] = direction.z;

		float size = light->param[RSE::LIGHT_PARAM_SIZE];

		light_data.size = size;

		light_data.inv_spot_attenuation = 1.0f / light->param[RSE::LIGHT_PARAM_SPOT_ATTENUATION];
		float spot_angle = light->param[RSE::LIGHT_PARAM_SPOT_ANGLE];
		light_data.cos_spot_angle = Math::cos(Math::deg_to_rad(spot_angle));
		if (type == RSE::LIGHT_AREA) {
			Vector3 area_vec_a =
				inverse_transform.basis.xform(light_transform.basis.xform(Vector3(1, 0, 0)))
					.normalized() *
				area_size.x;
			Vector3 area_vec_b =
				inverse_transform.basis.xform(light_transform.basis.xform(Vector3(0, 1, 0)))
					.normalized() *
				area_size.y;

			light_data.area_width[0] = area_vec_a.x;
			light_data.area_width[1] = area_vec_a.y;
			light_data.area_width[2] = area_vec_a.z;

			light_data.area_height[0] = area_vec_b.x;
			light_data.area_height[1] = area_vec_b.y;
			light_data.area_height[2] = area_vec_b.z;
			light_data.inv_spot_attenuation =
				1.0 / (radius + area_size.length() / 2.0); // center range

			if (light->area_normalize_energy) {
				// normalization to make larger lights output same amount of light as smaller lights
				// with same energy
				float surface_area = area_size.x * area_size.y;
				light_data.color[0] /= surface_area;
				light_data.color[1] /= surface_area;
				light_data.color[2] /= surface_area;
			}
		}
		light_data.mask = light->cull_mask;

		light_data.atlas_rect[0] = 0;
		light_data.atlas_rect[1] = 0;
		light_data.atlas_rect[2] = 0;
		light_data.atlas_rect[3] = 0;

		RID projector = light->projector;

		if (projector.is_valid() && type != RSE::LIGHT_AREA) {
			Rect2 rect = RendererRD::TextureStorage::decal_atlas_get_texture_rect(projector);

			if (type == RSE::LIGHT_SPOT) {
				light_data.projector_rect[0] = rect.position.x;
				light_data.projector_rect[1] =
					rect.position.y + rect.size.height; // flip because shadow is flipped
				light_data.projector_rect[2] = rect.size.width;
				light_data.projector_rect[3] = -rect.size.height;
			}
			else {
				light_data.projector_rect[0] = rect.position.x;
				light_data.projector_rect[1] = rect.position.y;
				light_data.projector_rect[2] = rect.size.width;
				light_data.projector_rect[3] =
					rect.size.height * 0.5; // used by dp, so needs to be half
			}
		}
		else {
			light_data.projector_rect[0] = 0;
			light_data.projector_rect[1] = 0;
			light_data.projector_rect[2] = 0;
			light_data.projector_rect[3] = 0;
		}

		RID area_texture = light->area_texture;
		if (area_texture.is_valid() && type == RSE::LIGHT_AREA) {
			Rect2 rect =
				RendererRD::TextureStorage::area_light_atlas_get_texture_rect(area_texture);
			light_data.projector_rect[0] = rect.position.x;
			light_data.projector_rect[1] = rect.position.y;
			light_data.projector_rect[2] = rect.size.width;
			light_data.projector_rect[3] = rect.size.height;
			Size2i texture_size =
				(rect.size * RendererRD::TextureStorage::area_light_atlas_get_size()).ceil();
			light_data.cos_spot_angle =
				MIN(Math::floor(Math::log2(MAX(MIN(texture_size.x, texture_size.y), 1.0f))),
					RendererRD::TextureStorage::area_light_atlas_get_mipmaps()) -
				1.0f; // max mipmaps
		}

		const bool needs_shadow =
			p_using_shadows && owns_shadow_atlas(p_shadow_atlas) &&
			shadow_atlas_owns_light_instance(p_shadow_atlas, light_instance->self) && light->shadow;

		bool in_shadow_range = true;
		if (needs_shadow && light->distance_fade) {
			if (distance > light->distance_fade_shadow + light->distance_fade_length) {
				// Out of range, don't draw shadows to improve performance.
				in_shadow_range = false;
			}
		}

		if (needs_shadow && in_shadow_range) {
			// fill in the shadow information

			light_data.shadow_opacity =
				light->param[RSE::LIGHT_PARAM_SHADOW_OPACITY] * shadow_opacity_fade;

			float shadow_texel_size =
				light_instance_get_shadow_texel_size(light_instance->self, p_shadow_atlas);
			light_data.shadow_normal_bias =
				light->param[RSE::LIGHT_PARAM_SHADOW_NORMAL_BIAS] * shadow_texel_size * 10.0;

			if (type == RSE::LIGHT_SPOT) {
				light_data.shadow_bias = light->param[RSE::LIGHT_PARAM_SHADOW_BIAS] / 100.0;
			}
			else { // omni or area
				light_data.shadow_bias = light->param[RSE::LIGHT_PARAM_SHADOW_BIAS];
			}

			light_data.transmittance_bias = light->param[RSE::LIGHT_PARAM_TRANSMITTANCE_BIAS];

			Vector2i omni_offset;
			Rect2 rect = light_instance_get_shadow_atlas_rect(
				light_instance->self, p_shadow_atlas, omni_offset);

			light_data.atlas_rect[0] = rect.position.x;
			light_data.atlas_rect[1] = rect.position.y;
			light_data.atlas_rect[2] = rect.size.width;
			light_data.atlas_rect[3] = rect.size.height;

			light_data.soft_shadow_scale = light->param[RSE::LIGHT_PARAM_SHADOW_BLUR];

			if (type == RSE::LIGHT_OMNI) {
				Transform3D proj = (inverse_transform * light_transform).inverse();

				RendererRD::MaterialStorage::store_transform(proj, light_data.shadow_matrix);

				if (size > 0.0 && light_data.soft_shadow_scale > 0.0) {
					// Only enable PCSS-like soft shadows if blurring is enabled.
					// Otherwise, performance would decrease with no visual difference.
					light_data.soft_shadow_size = size;
				}
				else {
					light_data.soft_shadow_size = 0.0;
					light_data.soft_shadow_scale *=
						RendererSceneRenderRD::shadows_quality_radius_get(); // Only use quality
																			 // radius for PCF
				}

				light_data.direction[0] = omni_offset.x * float(rect.size.width);
				light_data.direction[1] = omni_offset.y * float(rect.size.height);
			}
			else if (type == RSE::LIGHT_AREA) {
				Transform3D proj = (inverse_transform * light_transform).inverse();

				RendererRD::MaterialStorage::store_transform(proj, light_data.shadow_matrix);

				if (light_data.size > 0.0 && light_data.soft_shadow_scale > 0.0) {
					// Only enable PCSS-like soft shadows if blurring is enabled.
					// Otherwise, performance would decrease with no visual difference.
					light_data.soft_shadow_size = light_data.size;
				}
				else {
					light_data.soft_shadow_size = 0.0;
					light_data.soft_shadow_scale *=
						RendererSceneRenderRD::shadows_quality_radius_get(); // Only use quality
																			 // radius for PCF
				}
			}
			else if (type == RSE::LIGHT_SPOT) {
				Transform3D modelview = (inverse_transform * light_transform).inverse();
				Projection bias;
				bias.set_light_bias();

				Projection correction;
				correction.set_depth_correction(false, true, false);
				Projection cm = correction * light_instance->shadow_transform[0].camera;
				Projection shadow_mtx = bias * cm * modelview;
				RendererRD::MaterialStorage::store_camera(shadow_mtx, light_data.shadow_matrix);

				if (size > 0.0 && light_data.soft_shadow_scale > 0.0) {
					// Only enable PCSS-like soft shadows if blurring is enabled.
					// Otherwise, performance would decrease with no visual difference.
					float half_np = cm.get_z_near() * Math::tan(Math::deg_to_rad(spot_angle));
					light_data.soft_shadow_size =
						(size * 0.5 / radius) / (half_np / cm.get_z_near()) * rect.size.width;
				}
				else {
					light_data.soft_shadow_size = 0.0;
					light_data.soft_shadow_scale *=
						RendererSceneRenderRD::shadows_quality_radius_get(); // Only use quality
																			 // radius for PCF
				}
				light_data.shadow_bias *= light_data.soft_shadow_scale;
			}
		}
		else {
			light_data.shadow_opacity = 0.0;
		}

		light_instance->cull_mask = light->cull_mask;

		// hook for subclass to do further processing.
		RendererSceneRenderRD::setup_added_light(
			type, light_transform, radius, spot_angle, area_size);

		r_positional_light_count++;
	}
}

RID RendererRD::LightStorage::reflection_probe_allocate()
{
	return reflection_probe_owner.allocate_rid();
}

void RendererRD::LightStorage::reflection_probe_initialize(RID p_reflection_probe)
{
	reflection_probe_owner.initialize_rid(p_reflection_probe, ReflectionProbe());
}

void RendererRD::LightStorage::reflection_probe_free(RID p_rid)
{
	ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_rid);
	reflection_probe->dependency.deleted_notify(p_rid);
	reflection_probe_owner.free(p_rid);
}

void RendererRD::LightStorage::reflection_probe_set_update_mode(
	RID p_probe, RSE::ReflectionProbeUpdateMode p_mode)
{
	ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL(reflection_probe);

	reflection_probe->update_mode = p_mode;
	reflection_probe->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_REFLECTION_PROBE);
}

void RendererRD::LightStorage::reflection_probe_set_intensity(RID p_probe, float p_intensity)
{
	ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL(reflection_probe);

	reflection_probe->intensity = p_intensity;
}

void RendererRD::LightStorage::reflection_probe_set_blend_distance(
	RID p_probe, float p_blend_distance)
{
	ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL(reflection_probe);

	reflection_probe->blend_distance = p_blend_distance;
}

void RendererRD::LightStorage::reflection_probe_set_ambient_mode(
	RID p_probe, RSE::ReflectionProbeAmbientMode p_mode)
{
	ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL(reflection_probe);

	reflection_probe->ambient_mode = p_mode;
}

void RendererRD::LightStorage::reflection_probe_set_ambient_color(RID p_probe, const Color& p_color)
{
	ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL(reflection_probe);

	reflection_probe->ambient_color = p_color;
}

void RendererRD::LightStorage::reflection_probe_set_ambient_energy(RID p_probe, float p_energy)
{
	ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL(reflection_probe);

	reflection_probe->ambient_color_energy = p_energy;
}

void RendererRD::LightStorage::reflection_probe_set_max_distance(RID p_probe, float p_distance)
{
	ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL(reflection_probe);

	reflection_probe->max_distance = p_distance;

	reflection_probe->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_REFLECTION_PROBE);
}

void RendererRD::LightStorage::reflection_probe_set_size(RID p_probe, const Vector3& p_size)
{
	ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL(reflection_probe);

	if (reflection_probe->size == p_size) {
		return;
	}
	reflection_probe->size = p_size;
	reflection_probe->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_REFLECTION_PROBE);
}

void RendererRD::LightStorage::reflection_probe_set_origin_offset(
	RID p_probe, const Vector3& p_offset)
{
	ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL(reflection_probe);

	reflection_probe->origin_offset = p_offset;
	reflection_probe->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_REFLECTION_PROBE);
}

void RendererRD::LightStorage::reflection_probe_set_as_interior(RID p_probe, bool p_enable)
{
	ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL(reflection_probe);

	reflection_probe->interior = p_enable;
	reflection_probe->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_REFLECTION_PROBE);
}

void RendererRD::LightStorage::reflection_probe_set_enable_box_projection(
	RID p_probe, bool p_enable)
{
	ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL(reflection_probe);

	reflection_probe->box_projection = p_enable;
}

void RendererRD::LightStorage::reflection_probe_set_enable_shadows(RID p_probe, bool p_enable)
{
	ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL(reflection_probe);

	reflection_probe->enable_shadows = p_enable;
	reflection_probe->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_REFLECTION_PROBE);
}

void RendererRD::LightStorage::reflection_probe_set_cull_mask(RID p_probe, uint32_t p_layers)
{
	ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL(reflection_probe);

	reflection_probe->cull_mask = p_layers;
	reflection_probe->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_REFLECTION_PROBE);
}

void RendererRD::LightStorage::reflection_probe_set_reflection_mask(RID p_probe, uint32_t p_layers)
{
	ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL(reflection_probe);

	reflection_probe->reflection_mask = p_layers;
	reflection_probe->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_REFLECTION_PROBE);
}

void RendererRD::LightStorage::reflection_probe_set_resolution(RID p_probe, int p_resolution)
{
	WARN_PRINT_ONCE("reflection_probe_set_resolution is not available in Godot 4. ReflectionProbe "
					"size is configured in the project settings with the "
					"rendering/reflections/reflection_atlas/reflection_size setting.");
}

void RendererRD::LightStorage::reflection_probe_set_mesh_lod_threshold(RID p_probe, float p_ratio)
{
	ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL(reflection_probe);

	reflection_probe->mesh_lod_threshold = p_ratio;

	reflection_probe->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_REFLECTION_PROBE);
}

void RendererRD::LightStorage::reflection_probe_set_baked_exposure(RID p_probe, float p_exposure)
{
	ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL(reflection_probe);

	reflection_probe->baked_exposure = p_exposure;
}

AABB RendererRD::LightStorage::reflection_probe_get_aabb(RID p_probe)
{
	const ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL_V(reflection_probe, AABB());

	AABB aabb;
	aabb.position = -reflection_probe->size / 2;
	aabb.size = reflection_probe->size;

	return aabb;
}

RSE::ReflectionProbeUpdateMode RendererRD::LightStorage::reflection_probe_get_update_mode(
	RID p_probe)
{
	const ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL_V(reflection_probe, RSE::REFLECTION_PROBE_UPDATE_ALWAYS);

	return reflection_probe->update_mode;
}

uint32_t RendererRD::LightStorage::reflection_probe_get_cull_mask(RID p_probe)
{
	const ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL_V(reflection_probe, 0);

	return reflection_probe->cull_mask;
}

uint32_t RendererRD::LightStorage::reflection_probe_get_reflection_mask(RID p_probe)
{
	const ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL_V(reflection_probe, 0);

	return reflection_probe->reflection_mask;
}

Vector3 RendererRD::LightStorage::reflection_probe_get_size(RID p_probe)
{
	const ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL_V(reflection_probe, Vector3());

	return reflection_probe->size;
}

Vector3 RendererRD::LightStorage::reflection_probe_get_origin_offset(RID p_probe)
{
	const ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL_V(reflection_probe, Vector3());

	return reflection_probe->origin_offset;
}

bool RendererRD::LightStorage::reflection_probe_renders_shadows(RID p_probe)
{
	const ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL_V(reflection_probe, false);

	return reflection_probe->enable_shadows;
}

float RendererRD::LightStorage::reflection_probe_get_origin_max_distance(RID p_probe)
{
	const ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL_V(reflection_probe, 0);

	return reflection_probe->max_distance;
}

float RendererRD::LightStorage::reflection_probe_get_mesh_lod_threshold(RID p_probe)
{
	const ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL_V(reflection_probe, 0);

	return reflection_probe->mesh_lod_threshold;
}

float RendererRD::LightStorage::reflection_probe_get_baked_exposure(RID p_probe)
{
	const ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL_V(reflection_probe, 1.0);

	return reflection_probe->baked_exposure;
}

float RendererRD::LightStorage::reflection_probe_get_intensity(RID p_probe)
{
	const ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL_V(reflection_probe, 0);

	return reflection_probe->intensity;
}

float RendererRD::LightStorage::reflection_probe_get_blend_distance(RID p_probe)
{
	const ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL_V(reflection_probe, 0);

	return reflection_probe->blend_distance;
}

bool RendererRD::LightStorage::reflection_probe_is_interior(RID p_probe)
{
	const ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL_V(reflection_probe, false);

	return reflection_probe->interior;
}

bool RendererRD::LightStorage::reflection_probe_is_box_projection(RID p_probe)
{
	const ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL_V(reflection_probe, false);

	return reflection_probe->box_projection;
}

RSE::ReflectionProbeAmbientMode RendererRD::LightStorage::reflection_probe_get_ambient_mode(
	RID p_probe)
{
	const ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL_V(reflection_probe, RSE::REFLECTION_PROBE_AMBIENT_DISABLED);
	return reflection_probe->ambient_mode;
}

Color RendererRD::LightStorage::reflection_probe_get_ambient_color(RID p_probe)
{
	const ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL_V(reflection_probe, Color());

	return reflection_probe->ambient_color;
}

float RendererRD::LightStorage::reflection_probe_get_ambient_color_energy(RID p_probe)
{
	const ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL_V(reflection_probe, 0);

	return reflection_probe->ambient_color_energy;
}

Dependency* RendererRD::LightStorage::reflection_probe_get_dependency(RID p_probe)
{
	ReflectionProbe* reflection_probe = reflection_probe_owner.get_or_null(p_probe);
	ERR_FAIL_NULL_V(reflection_probe, nullptr);

	return &reflection_probe->dependency;
}

void RendererRD::LightStorage::reflection_atlas_free(RID p_ref_atlas)
{
	reflection_atlas_set_size(p_ref_atlas, 0, 0);
	ReflectionAtlas* ra = reflection_atlas_owner.get_or_null(p_ref_atlas);
	memdelete(ra->cluster_builder);
	reflection_atlas_owner.free(p_ref_atlas);
}

void RendererRD::LightStorage::_reflection_atlas_clear(ReflectionAtlas* p_reflection_atlas)
{
	RD::free_rid(p_reflection_atlas->reflection);
	p_reflection_atlas->reflection = RID();

	RD::free_rid(p_reflection_atlas->depth_fb);
	p_reflection_atlas->depth_fb = RID();

	RD::free_rid(p_reflection_atlas->depth_buffer);
	p_reflection_atlas->depth_buffer = RID();

	for (int i = 0; i < p_reflection_atlas->reflections.size(); i++) {
		p_reflection_atlas->reflections.write[i].data.clear_reflection_data();
		if (p_reflection_atlas->reflections[i].owner.is_null()) {
			continue;
		}

		reflection_probe_release_atlas_index(p_reflection_atlas->reflections[i].owner);
	}

	p_reflection_atlas->reflections.clear();
}

void RendererRD::LightStorage::reflection_atlas_set_size(
	RID p_ref_atlas, int p_reflection_size, int p_reflection_count)
{
	ReflectionAtlas* ra = reflection_atlas_owner.get_or_null(p_ref_atlas);
	ERR_FAIL_NULL(ra);

	if (ra->size == p_reflection_size && ra->count == p_reflection_count) {
		return; // no changes
	}

	if (ra->cluster_builder) {
		// only if we're using our cluster
		ra->cluster_builder->setup(
			Size2i(ra->size, ra->size), max_cluster_elements, RID(), RID(), RID());
	}

	ra->size = p_reflection_size;
	ra->count = p_reflection_count;

	if (ra->reflection.is_valid()) {
		_reflection_atlas_clear(ra);
	}

	if (ra->render_buffers.is_valid()) {
		ra->render_buffers->cleanup();
	}
}

int RendererRD::LightStorage::reflection_atlas_get_size(RID p_ref_atlas)
{
	ReflectionAtlas* ra = reflection_atlas_owner.get_or_null(p_ref_atlas);
	ERR_FAIL_NULL_V(ra, 0);

	return ra->size;
}

RID RendererRD::LightStorage::reflection_probe_instance_create(RID p_probe)
{
	ReflectionProbeInstance rpi;
	rpi.probe = p_probe;
	rpi.forward_id = ForwardIDStorage::allocate_forward_id(FORWARD_ID_TYPE_REFLECTION_PROBE);

	return reflection_probe_instance_owner.make_rid(rpi);
}

void RendererRD::LightStorage::reflection_probe_instance_free(RID p_instance)
{
	ReflectionProbeInstance* rpi = reflection_probe_instance_owner.get_or_null(p_instance);
	ForwardIDStorage::free_forward_id(FORWARD_ID_TYPE_REFLECTION_PROBE, rpi->forward_id);
	reflection_probe_release_atlas_index(p_instance);
	reflection_probe_instance_owner.free(p_instance);
}

void RendererRD::LightStorage::reflection_probe_instance_set_transform(
	RID p_instance, const Transform3D& p_transform)
{
	ReflectionProbeInstance* rpi = reflection_probe_instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL(rpi);

	rpi->transform = p_transform;
	rpi->dirty = true;
}

bool RendererRD::LightStorage::reflection_probe_has_atlas_index(RID p_instance)
{
	ReflectionProbeInstance* rpi = reflection_probe_instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL_V(rpi, false);

	if (rpi->atlas.is_null()) {
		return false;
	}

	return rpi->atlas_index >= 0;
}

void RendererRD::LightStorage::reflection_probe_release_atlas_index(RID p_instance)
{
	ReflectionProbeInstance* rpi = reflection_probe_instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL(rpi);

	if (rpi->atlas.is_null()) {
		return; // nothing to release
	}
	ReflectionAtlas* atlas = reflection_atlas_owner.get_or_null(rpi->atlas);
	ERR_FAIL_NULL(atlas);
	ERR_FAIL_INDEX(rpi->atlas_index, atlas->reflections.size());
	atlas->reflections.write[rpi->atlas_index].owner = RID();

	// TODO investigate if this is enough? shouldn't we be freeing our textures and framebuffers?

	if (rpi->rendering) {
		// We were cancelled mid rendering, trigger refresh.
		rpi->rendering = false;
		rpi->dirty = true;
		rpi->processing_layer = 1;
	}

	rpi->atlas_index = -1;
	rpi->atlas = RID();
}

bool RendererRD::LightStorage::reflection_probe_instance_needs_redraw(RID p_instance)
{
	ReflectionProbeInstance* rpi = reflection_probe_instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL_V(rpi, false);

	if (rpi->rendering) {
		return false;
	}

	if (rpi->dirty) {
		return true;
	}

	if (RendererRD::LightStorage::reflection_probe_get_update_mode(rpi->probe) ==
		RSE::REFLECTION_PROBE_UPDATE_ALWAYS) {
		return true;
	}

	return rpi->atlas_index == -1;
}

bool RendererRD::LightStorage::reflection_probe_instance_has_reflection(RID p_instance)
{
	ReflectionProbeInstance* rpi = reflection_probe_instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL_V(rpi, false);

	return rpi->atlas.is_valid();
}

bool RendererRD::LightStorage::reflection_probe_instance_begin_render(
	RID p_instance, RID p_reflection_atlas)
{
	ReflectionAtlas* atlas = reflection_atlas_owner.get_or_null(p_reflection_atlas);

	ERR_FAIL_NULL_V(atlas, false);

	ERR_FAIL_COND_V_MSG(
		atlas->size < 2, false, "Attempted to render to a reflection atlas of invalid resolution.");
	ERR_FAIL_COND_V_MSG(
		atlas->count < 1, false, "Attempted to render to a reflection atlas of size < 1.");

	ReflectionProbeInstance* rpi = reflection_probe_instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL_V(rpi, false);

	if (atlas->render_buffers.is_null()) {
		atlas->render_buffers.instantiate();
	}

	RD::draw_command_begin_label("Reflection Probe Render");

	const bool update_always = RendererRD::LightStorage::reflection_probe_get_update_mode(
								   rpi->probe) == RSE::REFLECTION_PROBE_UPDATE_ALWAYS;
	if (update_always && atlas->reflection.is_valid() && atlas->size != 256) {
		WARN_PRINT("ReflectionProbes set to UPDATE_ALWAYS must have an atlas size of 256. Please "
				   "update the atlas size in the ProjectSettings.");
		reflection_atlas_set_size(p_reflection_atlas, 256, atlas->count);
	}

	// Reflection atlas only allows recreating the texture at a lower resolution when going from not
	// real time to real time, but not the other way around. This can lead to situations where the
	// reflection atlas will be stuck on a lower resolution even if no real-time probes are present.
	// This is intentional behavior until a future solution can accommodate for both quality levels
	// being used simultaneously.
	const int required_real_time_mipmaps = 7;
	const bool switched_to_real_time =
		!atlas->update_always && update_always && atlas->reflection.is_valid();
	const bool real_time_mipmaps_different =
		update_always && atlas->reflection.is_valid() &&
		atlas->reflections[0].data.layers[0].mipmaps.size() != required_real_time_mipmaps;
	if (switched_to_real_time || real_time_mipmaps_different) {
		_reflection_atlas_clear(atlas);
	}

	if (atlas->reflection.is_null()) {
		ERR_FAIL_COND_V_MSG(
			!RendererRD::CopyEffects::is_initialized(), false, "Effects haven't been initialized");

		int mipmaps = MIN(RendererSceneRenderRD::get_sky()->roughness_layers,
			Image::get_image_required_mipmaps(atlas->size, atlas->size, Image::FORMAT_RGBAH) + 1);
		mipmaps = update_always ? required_real_time_mipmaps : mipmaps;

		// Double size to approximate texel density of cubemaps + add border for proper
		// filtering/mipmapping.
		uint32_t padding_pixels = (1 << (mipmaps - 1));
		atlas->reflection_texture_size = atlas->size * 2 + padding_pixels * 2;
		atlas->uv_border_size = float(padding_pixels) / float(atlas->reflection_texture_size);

		bool use_storage =
			!((CopyEffects::get_raster_effects() & CopyEffects::RASTER_EFFECT_OCTMAP) != 0);
		{
			RDC::TextureFormat tf;
			tf.array_layers = atlas->count;
			tf.format = get_reflection_probe_color_format();
			tf.texture_type = RDC::TEXTURE_TYPE_2D_ARRAY;
			tf.mipmaps = mipmaps;
			tf.width = atlas->reflection_texture_size;
			tf.height = atlas->reflection_texture_size;
			tf.usage_bits = get_reflection_probe_color_usage_bits(use_storage);
			atlas->reflection = RD::texture_create(tf, RDD::TextureView());
		}
		{
			RDC::TextureFormat tf;
			tf.array_layers = 6;
			tf.format = get_reflection_probe_color_format();
			tf.texture_type = RDC::TEXTURE_TYPE_CUBE;
			tf.width = atlas->size;
			tf.height = atlas->size;
			tf.usage_bits = get_reflection_probe_color_usage_bits(use_storage);
			atlas->color_buffer = RD::texture_create(tf, RDD::TextureView());
		}
		{
			RDC::TextureFormat tf;
			tf.format = get_reflection_probe_depth_format();
			tf.width = atlas->size;
			tf.height = atlas->size;
			tf.usage_bits = get_reflection_probe_depth_usage_bits();
			atlas->depth_buffer = RD::texture_create(tf, RDD::TextureView());
		}
		atlas->reflections.resize(atlas->count);
		for (int i = 0; i < atlas->count; i++) {
			atlas->reflections.write[i].data.update_reflection_data(atlas->reflection_texture_size,
				mipmaps, false, atlas->reflection, i, update_always,
				RendererSceneRenderRD::get_sky()->roughness_layers,
				RendererSceneRenderRD::_render_buffers_get_preferred_color_format(),
				atlas->uv_border_size);
		}

		for (int i = 0; i < 6; i++) {
			atlas->color_views[i] =
				RD::texture_create_shared_from_slice(RD::TextureView(), atlas->color_buffer, i, 0);
			atlas->color_fbs[i] = RendererSceneRenderRD::reflection_probe_create_framebuffer(
				atlas->color_views[i], atlas->depth_buffer);
		}

		Vector<RID> fbs;
		fbs.push_back(atlas->depth_buffer);
		atlas->depth_fb = RD::framebuffer_create(fbs);

		atlas->render_buffers->configure_for_reflections(Size2i(atlas->size, atlas->size));
		atlas->update_always = update_always;
	}

	if (rpi->atlas_index == -1) {
		for (int i = 0; i < atlas->reflections.size(); i++) {
			if (atlas->reflections[i].owner.is_null()) {
				rpi->atlas_index = i;
				break;
			}
		}
		// find the one used last
		if (rpi->atlas_index == -1) {
			// everything is in use, find the one least used via LRU
			uint64_t pass_min = 0;

			for (int i = 0; i < atlas->reflections.size(); i++) {
				ReflectionProbeInstance* rpi2 =

					reflection_probe_instance_owner.get_or_null(atlas->reflections[i].owner);
				if (rpi2->last_pass < pass_min) {
					pass_min = rpi2->last_pass;
					rpi->atlas_index = i;
				}
			}
		}
	}

	if (rpi->atlas_index != -1) { // should we fail if this is still -1 ?
		atlas->reflections.write[rpi->atlas_index].owner = p_instance;
	}

	rpi->atlas = p_reflection_atlas;
	rpi->rendering = true;
	rpi->dirty = false;
	rpi->processing_layer = 1;

	RD::draw_command_end_label();

	return true;
}

bool RendererRD::LightStorage::reflection_probe_instance_end_render(
	RID p_instance, RID p_reflection_atlas)
{
	ERR_FAIL_COND_V_MSG(
		!RendererRD::CopyEffects::is_initialized(), false, "Effects haven't been initialized");

	ReflectionAtlas* atlas = reflection_atlas_owner.get_or_null(p_reflection_atlas);
	ERR_FAIL_NULL_V(atlas, false);

	ReflectionProbeInstance* rpi = reflection_probe_instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL_V(rpi, false);

	RD::draw_command_begin_label("Convert reflection probe to octahedral");
	CopyEffects::copy_cubemap_to_octmap(atlas->color_buffer,
		atlas->reflections.write[rpi->atlas_index].data.layers[0].mipmaps[0].framebuffer,
		atlas->uv_border_size);
	RD::draw_command_end_label();

	return true;
}

Ref<RenderSceneBuffers> RendererRD::LightStorage::reflection_probe_atlas_get_render_buffers(
	RID p_reflection_atlas)
{
	ReflectionAtlas* atlas = reflection_atlas_owner.get_or_null(p_reflection_atlas);
	ERR_FAIL_NULL_V(atlas, Ref<RenderSceneBuffersRD>());

	return atlas->render_buffers;
}

bool RendererRD::LightStorage::reflection_probe_instance_postprocess_step(RID p_instance)
{
	ReflectionProbeInstance* rpi = reflection_probe_instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL_V(rpi, false);
	ERR_FAIL_COND_V(!rpi->rendering, false);

	ReflectionAtlas* atlas = reflection_atlas_owner.get_or_null(rpi->atlas);
	if (!atlas || rpi->atlas_index == -1) {
		// Does not belong to an atlas anymore, cancel (was removed from atlas or atlas changed
		// while rendering).
		rpi->rendering = false;
		return false;
	}

	if (RendererRD::LightStorage::reflection_probe_get_update_mode(rpi->probe) ==
		RSE::REFLECTION_PROBE_UPDATE_ALWAYS) {
		// Using real time reflections, all roughness is done in one step
		atlas->reflections.write[rpi->atlas_index].data.create_reflection_fast_filter(false);
		rpi->rendering = false;
		rpi->processing_layer = 1;
		return true;
	}

	atlas->reflections.write[rpi->atlas_index].data.create_reflection_importance_sample(
		false, rpi->processing_layer, RendererSceneRenderRD::get_sky()->sky_ggx_samples_quality);

	rpi->processing_layer++;
	if (rpi->processing_layer ==
		(int)atlas->reflections[rpi->atlas_index].data.layers[0].mipmaps.size()) {
		rpi->rendering = false;
		rpi->processing_layer = 1;
		return true;
	}

	return false;
}

uint32_t RendererRD::LightStorage::reflection_probe_instance_get_resolution(RID p_instance)
{
	ReflectionProbeInstance* rpi = reflection_probe_instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL_V(rpi, 0);

	ReflectionAtlas* atlas = reflection_atlas_owner.get_or_null(rpi->atlas);
	ERR_FAIL_NULL_V(atlas, 0);
	return atlas->size;
}

RID RendererRD::LightStorage::reflection_probe_instance_get_framebuffer(RID p_instance, int p_index)
{
	ReflectionProbeInstance* rpi = reflection_probe_instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL_V(rpi, RID());
	ERR_FAIL_INDEX_V(p_index, 6, RID());

	ReflectionAtlas* atlas = reflection_atlas_owner.get_or_null(rpi->atlas);
	ERR_FAIL_NULL_V(atlas, RID());
	ERR_FAIL_COND_V_MSG(rpi->atlas_index < 0, RID(),
		"Reflection probe atlas index invalid. Maximum amount of reflection probes in use (" +
			itos(atlas->count) +
			") may have been exceeded, reflections will not display properly. Consider increasing "
			"Rendering > Reflections > Reflection Atlas > Reflection Count in the Project "
			"Settings.");

	return atlas->color_fbs[p_index];
}

RID RendererRD::LightStorage::reflection_probe_instance_get_depth_framebuffer(
	RID p_instance, int p_index)
{
	ReflectionProbeInstance* rpi = reflection_probe_instance_owner.get_or_null(p_instance);
	ERR_FAIL_NULL_V(rpi, RID());
	ERR_FAIL_INDEX_V(p_index, 6, RID());

	ReflectionAtlas* atlas = reflection_atlas_owner.get_or_null(rpi->atlas);
	ERR_FAIL_NULL_V(atlas, RID());
	return atlas->depth_fb;
}

ClusterBuilderRD* RendererRD::LightStorage::reflection_probe_instance_get_cluster_builder(
	RID p_instance, ClusterBuilderSharedDataRD* p_cluster_builder_shared)
{
	ReflectionProbeInstance* rpi = reflection_probe_instance_owner.get_or_null(p_instance);
	ReflectionAtlas* ra = reflection_atlas_owner.get_or_null(rpi->atlas);
	if (!ra) {
		ERR_PRINT("reflection probe has no reflection atlas! Bug?");
		return nullptr;
	}
	else {
		if (ra->cluster_builder == nullptr) {
			ra->cluster_builder = memnew(ClusterBuilderRD);
			ra->cluster_builder->set_shared(p_cluster_builder_shared);
			ra->cluster_builder->setup(
				Size2i(ra->size, ra->size), get_max_cluster_elements(), RID(), RID(), RID());
		}
		return ra->cluster_builder;
	}
}

void RendererRD::LightStorage::free_reflection_data()
{
	if (reflection_buffer.is_valid()) {
		RD::free_rid(reflection_buffer);
		reflection_buffer = RID();
	}

	if (reflections != nullptr) {
		memdelete_arr(reflections);
		reflections = nullptr;
	}

	if (reflection_sort != nullptr) {
		memdelete_arr(reflection_sort);
		reflection_sort = nullptr;
	}
}

void RendererRD::LightStorage::set_max_reflection_probes(const uint32_t p_max_reflection_probes)
{
	max_reflections = p_max_reflection_probes;
	reflections = memnew_arr(ReflectionData, max_reflections);
	reflection_sort = memnew_arr(ReflectionProbeInstanceSort, max_reflections);
	reflection_buffer = RD::storage_buffer_create(sizeof(ReflectionData) * max_reflections);
}

void RendererRD::LightStorage::update_reflection_probe_buffer(RenderDataRD* p_render_data,
	const PagedArray<RID>& p_reflections, const Transform3D& p_camera_inverse_transform,
	RID p_environment)
{
	reflection_count = 0;

	for (uint32_t i = 0; i < (uint32_t)p_reflections.size(); i++) {
		if (reflection_count == max_reflections) {
			break;
		}

		ReflectionProbeInstance* rpi =
			reflection_probe_instance_owner.get_or_null(p_reflections[i]);
		if (!rpi) {
			continue;
		}
		ReflectionProbe* probe = reflection_probe_owner.get_or_null(rpi->probe);
		Vector3 extents = probe->size / 2;
		float probe_size = extents.length();

		reflection_sort[reflection_count].probe_instance = rpi;
		reflection_sort[reflection_count].size = -probe_size;
		reflection_count++;
	}

	if (reflection_count > 0) {
		SortArray<ReflectionProbeInstanceSort> sort_array;
		sort_array.sort(reflection_sort, reflection_count);
	}

	bool using_forward_ids = ForwardIDStorage::uses_forward_ids();
	for (uint32_t i = 0; i < reflection_count; i++) {
		ReflectionProbeInstance* rpi = reflection_sort[i].probe_instance;

		rpi->last_pass = RendererCompositor::get_frame_number();

		if (using_forward_ids) {
			ForwardIDStorage::map_forward_id(
				FORWARD_ID_TYPE_REFLECTION_PROBE, rpi->forward_id, i, rpi->last_pass);
		}

		ReflectionProbe* probe = reflection_probe_owner.get_or_null(rpi->probe);

		ReflectionData& reflection_ubo = reflections[i];

		Vector3 extents = probe->size / 2;

		rpi->cull_mask = probe->reflection_mask;

		reflection_ubo.box_extents[0] = extents.x;
		reflection_ubo.box_extents[1] = extents.y;
		reflection_ubo.box_extents[2] = extents.z;
		reflection_ubo.index = rpi->atlas_index;

		Vector3 origin_offset = probe->origin_offset;

		reflection_ubo.box_offset[0] = origin_offset.x;
		reflection_ubo.box_offset[1] = origin_offset.y;
		reflection_ubo.box_offset[2] = origin_offset.z;
		reflection_ubo.mask = probe->reflection_mask;

		reflection_ubo.intensity = probe->intensity;
		reflection_ubo.blend_distance = probe->blend_distance;
		reflection_ubo.ambient_mode = probe->ambient_mode;

		reflection_ubo.exterior = !probe->interior;
		reflection_ubo.box_project = probe->box_projection;
		reflection_ubo.exposure_normalization = 1.0;

		if (p_render_data->camera_attributes.is_valid()) {
			float exposure =
				RendererCameraAttributes::camera_attributes_get_exposure_normalization_factor(
					p_render_data->camera_attributes);
			reflection_ubo.exposure_normalization = exposure / probe->baked_exposure;
		}

		Color ambient_linear = probe->ambient_color.srgb_to_linear();
		float interior_ambient_energy = probe->ambient_color_energy;
		reflection_ubo.ambient[0] = ambient_linear.r * interior_ambient_energy;
		reflection_ubo.ambient[1] = ambient_linear.g * interior_ambient_energy;
		reflection_ubo.ambient[2] = ambient_linear.b * interior_ambient_energy;

		Transform3D transform = rpi->transform;
		Transform3D proj = (p_camera_inverse_transform * transform).inverse();
		MaterialStorage::store_transform(proj, reflection_ubo.local_matrix);

		// hook for subclass to do further processing.
		RendererSceneRenderRD::setup_added_reflection_probe(transform, extents);
	}
}

RDC::DataFormat RendererRD::LightStorage::get_reflection_probe_color_format()
{
	return RendererSceneRenderRD::_render_buffers_get_preferred_color_format();
}

uint32_t RendererRD::LightStorage::get_reflection_probe_color_usage_bits(bool p_storage)
{
	return RDC::TEXTURE_USAGE_COLOR_ATTACHMENT_BIT | RDC::TEXTURE_USAGE_SAMPLING_BIT |
		   (p_storage ? RDC::TEXTURE_USAGE_STORAGE_BIT : 0);
}

RDC::DataFormat RendererRD::LightStorage::get_reflection_probe_depth_format()
{
	return RD::texture_is_format_supported_for_usage(
			   RDC::DATA_FORMAT_D32_SFLOAT, RDC::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
			   ? RDC::DATA_FORMAT_D32_SFLOAT
			   : RDC::DATA_FORMAT_X8_D24_UNORM_PACK32;
}

uint32_t RendererRD::LightStorage::get_reflection_probe_depth_usage_bits()
{
	return RDC::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | RDC::TEXTURE_USAGE_SAMPLING_BIT;
}

RID RendererRD::LightStorage::lightmap_allocate() { return lightmap_owner.allocate_rid(); }

void RendererRD::LightStorage::lightmap_initialize(RID p_lightmap)
{
	lightmap_owner.initialize_rid(p_lightmap, Lightmap());
}

void RendererRD::LightStorage::lightmap_free(RID p_rid)
{
	lightmap_set_textures(p_rid, RID(), false);
	Lightmap* lightmap = lightmap_owner.get_or_null(p_rid);
	lightmap->dependency.deleted_notify(p_rid);
	lightmap_owner.free(p_rid);
}

void RendererRD::LightStorage::lightmap_set_textures(
	RID p_lightmap, RID p_light, bool p_uses_spherical_haromics)
{
	Lightmap* lm = lightmap_owner.get_or_null(p_lightmap);
	ERR_FAIL_NULL(lm);

	lightmap_array_version++;

	// erase lightmap users
	if (lm->light_texture.is_valid()) {
		TextureStorage::Texture* t = RendererRD::TextureStorage::get_texture(lm->light_texture);
		if (t) {
			t->lightmap_users.erase(p_lightmap);
		}
	}

	TextureStorage::Texture* t = RendererRD::TextureStorage::get_texture(p_light);
	lm->light_texture = p_light;
	lm->uses_spherical_harmonics = p_uses_spherical_haromics;

	RID default_2d_array = RendererRD::TextureStorage::texture_rd_get_default(
		TextureStorage::DEFAULT_RD_TEXTURE_2D_ARRAY_WHITE);
	if (!t) {
		if (using_lightmap_array) {
			if (lm->array_index >= 0) {
				lightmap_textures.write[lm->array_index] = default_2d_array;
				lm->array_index = -1;
			}
		}

		return;
	}

	t->lightmap_users.insert(p_lightmap);
	lm->light_texture_size = Vector2i(t->width, t->height);

	if (using_lightmap_array) {
		if (lm->array_index < 0) {
			// not in array, try to put in array
			for (int i = 0; i < lightmap_textures.size(); i++) {
				if (lightmap_textures[i] == default_2d_array) {
					lm->array_index = i;
					break;
				}
			}
		}
		ERR_FAIL_COND_MSG(lm->array_index < 0,
			"Maximum amount of lightmaps in use (" + itos(lightmap_textures.size()) +
				") has been exceeded, lightmap will not display properly.");

		lightmap_textures.write[lm->array_index] = t->rd_texture;
	}
}

void RendererRD::LightStorage::lightmap_set_probe_bounds(RID p_lightmap, const AABB& p_bounds)
{
	Lightmap* lm = lightmap_owner.get_or_null(p_lightmap);
	ERR_FAIL_NULL(lm);
	lm->bounds = p_bounds;
}

void RendererRD::LightStorage::lightmap_set_probe_interior(RID p_lightmap, bool p_interior)
{
	Lightmap* lm = lightmap_owner.get_or_null(p_lightmap);
	ERR_FAIL_NULL(lm);
	lm->interior = p_interior;
}

void RendererRD::LightStorage::lightmap_set_probe_capture_data(RID p_lightmap,
	const PackedVector3Array& p_points, const PackedColorArray& p_point_sh,
	const PackedInt32Array& p_tetrahedra, const PackedInt32Array& p_bsp_tree)
{
	Lightmap* lm = lightmap_owner.get_or_null(p_lightmap);
	ERR_FAIL_NULL(lm);

	if (p_points.size()) {
		ERR_FAIL_COND(p_points.size() * 9 != p_point_sh.size());
		ERR_FAIL_COND((p_tetrahedra.size() % 4) != 0);
		ERR_FAIL_COND((p_bsp_tree.size() % 6) != 0);
	}

	lm->points = p_points;
	lm->bsp_tree = p_bsp_tree;
	lm->point_sh = p_point_sh;
	lm->tetrahedra = p_tetrahedra;
}

void RendererRD::LightStorage::lightmap_set_baked_exposure_normalization(
	RID p_lightmap, float p_exposure)
{
	Lightmap* lm = lightmap_owner.get_or_null(p_lightmap);
	ERR_FAIL_NULL(lm);

	lm->baked_exposure = p_exposure;
}

PackedVector3Array RendererRD::LightStorage::lightmap_get_probe_capture_points(RID p_lightmap)
{
	Lightmap* lm = lightmap_owner.get_or_null(p_lightmap);
	ERR_FAIL_NULL_V(lm, PackedVector3Array());

	return lm->points;
}

PackedColorArray RendererRD::LightStorage::lightmap_get_probe_capture_sh(RID p_lightmap)
{
	Lightmap* lm = lightmap_owner.get_or_null(p_lightmap);
	ERR_FAIL_NULL_V(lm, PackedColorArray());
	return lm->point_sh;
}

PackedInt32Array RendererRD::LightStorage::lightmap_get_probe_capture_tetrahedra(RID p_lightmap)
{
	Lightmap* lm = lightmap_owner.get_or_null(p_lightmap);
	ERR_FAIL_NULL_V(lm, PackedInt32Array());
	return lm->tetrahedra;
}

PackedInt32Array RendererRD::LightStorage::lightmap_get_probe_capture_bsp_tree(RID p_lightmap)
{
	Lightmap* lm = lightmap_owner.get_or_null(p_lightmap);
	ERR_FAIL_NULL_V(lm, PackedInt32Array());
	return lm->bsp_tree;
}

void RendererRD::LightStorage::lightmap_set_probe_capture_update_speed(float p_speed)
{
	lightmap_probe_capture_update_speed = p_speed;
}

Dependency* RendererRD::LightStorage::lightmap_get_dependency(RID p_lightmap)
{
	Lightmap* lm = lightmap_owner.get_or_null(p_lightmap);
	ERR_FAIL_NULL_V(lm, nullptr);

	return &lm->dependency;
}

void RendererRD::LightStorage::lightmap_tap_sh_light(
	RID p_lightmap, const Vector3& p_point, Color* r_sh)
{
	Lightmap* lm = lightmap_owner.get_or_null(p_lightmap);
	ERR_FAIL_NULL(lm);

	for (int i = 0; i < 9; i++) {
		r_sh[i] = Color(0, 0, 0, 0);
	}

	if (!lm->points.size() || !lm->bsp_tree.size() || !lm->tetrahedra.size()) {
		return;
	}

	static_assert(sizeof(Lightmap::BSP) == 24);

	const Lightmap::BSP* bsp = (const Lightmap::BSP*)lm->bsp_tree.ptr();
	int32_t node = 0;
	while (node >= 0) {
		if (Plane(bsp[node].plane[0], bsp[node].plane[1], bsp[node].plane[2], bsp[node].plane[3])
				.is_point_over(p_point)) {
#ifdef DEBUG_ENABLED
			ERR_FAIL_COND(bsp[node].over >= 0 && bsp[node].over < node);
#endif

			node = bsp[node].over;
		}
		else {
#ifdef DEBUG_ENABLED
			ERR_FAIL_COND(bsp[node].under >= 0 && bsp[node].under < node);
#endif
			node = bsp[node].under;
		}
	}

	if (node == Lightmap::BSP::EMPTY_LEAF) {
		return; // nothing could be done
	}

	node = Math::abs(node) - 1;

	uint32_t* tetrahedron = (uint32_t*)&lm->tetrahedra[node * 4];
	Vector3 points[4] = {lm->points[tetrahedron[0]], lm->points[tetrahedron[1]],
		lm->points[tetrahedron[2]], lm->points[tetrahedron[3]]};
	const Color* sh_colors[4]{&lm->point_sh[tetrahedron[0] * 9], &lm->point_sh[tetrahedron[1] * 9],
		&lm->point_sh[tetrahedron[2] * 9], &lm->point_sh[tetrahedron[3] * 9]};
	Color barycentric = Geometry3D::tetrahedron_get_barycentric_coords(
		points[0], points[1], points[2], points[3], p_point);

	for (int i = 0; i < 4; i++) {
		float c = CLAMP(barycentric[i], 0.0, 1.0);
		for (int j = 0; j < 9; j++) {
			r_sh[j] += sh_colors[i][j] * c;
		}
	}
}

bool RendererRD::LightStorage::lightmap_is_interior(RID p_lightmap)
{
	const Lightmap* lm = lightmap_owner.get_or_null(p_lightmap);
	ERR_FAIL_NULL_V(lm, false);
	return lm->interior;
}

AABB RendererRD::LightStorage::lightmap_get_aabb(RID p_lightmap)
{
	const Lightmap* lm = lightmap_owner.get_or_null(p_lightmap);
	ERR_FAIL_NULL_V(lm, AABB());
	return lm->bounds;
}

void RendererRD::LightStorage::lightmap_set_shadowmask_textures(RID p_lightmap, RID p_shadow)
{
	Lightmap* lm = lightmap_owner.get_or_null(p_lightmap);
	ERR_FAIL_NULL(lm);

	// Erase lightmap users from shadow texture.
	if (lm->shadow_texture.is_valid()) {
		TextureStorage::Texture* t = RendererRD::TextureStorage::get_texture(lm->shadow_texture);
		if (t) {
			t->lightmap_users.erase(p_lightmap);
		}
	}

	TextureStorage::Texture* t = RendererRD::TextureStorage::get_texture(p_shadow);
	lm->shadow_texture = p_shadow;

	RID default_2d_array = RendererRD::TextureStorage::texture_rd_get_default(
		TextureStorage::DEFAULT_RD_TEXTURE_2D_ARRAY_WHITE);
	if (!t) {
		if (lm->array_index >= 0) {
			shadowmask_textures.write[lm->array_index] = default_2d_array;
			lm->array_index = -1;
		}

		return;
	}

	t->lightmap_users.insert(p_lightmap);

	if (lm->array_index < 0) {
		// Not in array, try to put in array.
		for (int i = 0; i < shadowmask_textures.size(); i++) {
			if (shadowmask_textures[i] == default_2d_array) {
				lm->array_index = i;
				break;
			}
		}
	}

	ERR_FAIL_COND_MSG(
		lm->array_index < 0, vformat("Maximum amount of shadowmasks in use (%d) has been exceeded, "
									 "shadowmask will not display properly.",
								 shadowmask_textures.size()));

	shadowmask_textures.write[lm->array_index] = t->rd_texture;
}

RSE::ShadowmaskMode RendererRD::LightStorage::lightmap_get_shadowmask_mode(RID p_lightmap)
{
	Lightmap* lm = lightmap_owner.get_or_null(p_lightmap);
	ERR_FAIL_NULL_V(lm, RSE::SHADOWMASK_MODE_NONE);

	return lm->shadowmask_mode;
}

void RendererRD::LightStorage::lightmap_set_shadowmask_mode(
	RID p_lightmap, RSE::ShadowmaskMode p_mode)
{
	Lightmap* lm = lightmap_owner.get_or_null(p_lightmap);
	ERR_FAIL_NULL(lm);

	lm->shadowmask_mode = p_mode;
}

RID RendererRD::LightStorage::lightmap_instance_create(RID p_lightmap)
{
	LightmapInstance li;
	li.lightmap = p_lightmap;
	return lightmap_instance_owner.make_rid(li);
}

void RendererRD::LightStorage::lightmap_instance_free(RID p_lightmap)
{
	lightmap_instance_owner.free(p_lightmap);
}

void RendererRD::LightStorage::lightmap_instance_set_transform(
	RID p_lightmap, const Transform3D& p_transform)
{
	LightmapInstance* li = lightmap_instance_owner.get_or_null(p_lightmap);
	ERR_FAIL_NULL(li);
	li->transform = p_transform;
}

RID RendererRD::LightStorage::shadow_atlas_create()
{
	return shadow_atlas_owner.make_rid(ShadowAtlas());
}

void RendererRD::LightStorage::shadow_atlas_free(RID p_atlas)
{
	shadow_atlas_set_size(p_atlas, 0);
	shadow_atlas_owner.free(p_atlas);
}

void RendererRD::LightStorage::_update_shadow_atlas(ShadowAtlas* shadow_atlas)
{
	if (shadow_atlas->size > 0 && shadow_atlas->depth.is_null()) {
		RDC::TextureFormat tf;
		tf.format = get_shadow_atlas_depth_format(shadow_atlas->use_16_bits);
		tf.width = shadow_atlas->size;
		tf.height = shadow_atlas->size;
		tf.usage_bits = get_shadow_atlas_depth_usage_bits();

		shadow_atlas->depth = RD::texture_create(tf, RDD::TextureView());
		Vector<RID> fb_tex;
		fb_tex.push_back(shadow_atlas->depth);
		shadow_atlas->fb = RD::framebuffer_create(fb_tex);
	}
}

void RendererRD::LightStorage::shadow_atlas_set_size(RID p_atlas, int p_size, bool p_16_bits)
{
	ShadowAtlas* shadow_atlas = shadow_atlas_owner.get_or_null(p_atlas);
	ERR_FAIL_NULL(shadow_atlas);
	ERR_FAIL_COND(p_size < 0);
	p_size = Math::next_power_of_2((uint32_t)p_size);

	if (p_size == shadow_atlas->size && p_16_bits == shadow_atlas->use_16_bits) {
		return;
	}

	// erasing atlas
	if (shadow_atlas->depth.is_valid()) {
		RD::free_rid(shadow_atlas->depth);
		shadow_atlas->depth = RID();
	}
	for (int i = 0; i < 4; i++) {
		// clear subdivisions
		shadow_atlas->quadrants[i].shadows.clear();
		shadow_atlas->quadrants[i].shadows.resize(int64_t(
			shadow_atlas->quadrants[i].subdivision * shadow_atlas->quadrants[i].subdivision));
	}

	// erase shadow atlas reference from lights
	for (const KeyValue<RID, uint32_t>& E : shadow_atlas->shadow_owners) {
		LightInstance* li = light_instance_owner.get_or_null(E.key);
		ERR_CONTINUE(!li);
		li->shadow_atlases.erase(p_atlas);
	}

	// clear owners
	shadow_atlas->shadow_owners.clear();

	shadow_atlas->size = p_size;
	shadow_atlas->use_16_bits = p_16_bits;
}

void RendererRD::LightStorage::shadow_atlas_set_quadrant_subdivision(
	RID p_atlas, int p_quadrant, int p_subdivision)
{
	ShadowAtlas* shadow_atlas = shadow_atlas_owner.get_or_null(p_atlas);
	ERR_FAIL_NULL(shadow_atlas);
	ERR_FAIL_INDEX(p_quadrant, 4);
	ERR_FAIL_INDEX(p_subdivision, 16384);

	uint32_t subdiv = Math::next_power_of_2((uint32_t)p_subdivision);
	if (subdiv & 0xaaaaaaaa) { // sqrt(subdiv) must be integer
		subdiv <<= 1;
	}

	subdiv = int(Math::sqrt((float)subdiv));

	// obtain the number that will be x*x

	if (shadow_atlas->quadrants[p_quadrant].subdivision == subdiv) {
		return;
	}

	// erase all data from quadrant
	for (int i = 0; i < shadow_atlas->quadrants[p_quadrant].shadows.size(); i++) {
		if (shadow_atlas->quadrants[p_quadrant].shadows[i].owner.is_valid()) {
			shadow_atlas->shadow_owners.erase(shadow_atlas->quadrants[p_quadrant].shadows[i].owner);
			LightInstance* li = light_instance_owner.get_or_null(
				shadow_atlas->quadrants[p_quadrant].shadows[i].owner);
			ERR_CONTINUE(!li);
			li->shadow_atlases.erase(p_atlas);
		}
	}

	shadow_atlas->quadrants[p_quadrant].shadows.clear();
	shadow_atlas->quadrants[p_quadrant].shadows.resize(subdiv * subdiv);
	shadow_atlas->quadrants[p_quadrant].subdivision = subdiv;

	// cache the smallest subdiv (for faster allocation in light update)

	shadow_atlas->smallest_subdiv = 1 << 30;

	for (int i = 0; i < 4; i++) {
		if (shadow_atlas->quadrants[i].subdivision) {
			shadow_atlas->smallest_subdiv =
				MIN(shadow_atlas->smallest_subdiv, shadow_atlas->quadrants[i].subdivision);
		}
	}

	if (shadow_atlas->smallest_subdiv == 1 << 30) {
		shadow_atlas->smallest_subdiv = 0;
	}

	// resort the size orders, simple bublesort for 4 elements..

	int swaps = 0;
	do {
		swaps = 0;

		for (int i = 0; i < 3; i++) {
			if (shadow_atlas->quadrants[shadow_atlas->size_order[i]].subdivision <
				shadow_atlas->quadrants[shadow_atlas->size_order[i + 1]].subdivision) {
				SWAP(shadow_atlas->size_order[i], shadow_atlas->size_order[i + 1]);
				swaps++;
			}
		}
	} while (swaps > 0);
}

bool RendererRD::LightStorage::_shadow_atlas_find_shadow(ShadowAtlas* shadow_atlas,
	int* p_in_quadrants, int p_quadrant_count, int p_current_subdiv, uint64_t p_tick,
	int& r_quadrant, int& r_shadow)
{
	for (int i = p_quadrant_count - 1; i >= 0; i--) {
		int qidx = p_in_quadrants[i];

		if (shadow_atlas->quadrants[qidx].subdivision == (uint32_t)p_current_subdiv) {
			return false;
		}

		// look for an empty space
		int sc = shadow_atlas->quadrants[qidx].shadows.size();
		const ShadowAtlas::Quadrant::Shadow* sarr = shadow_atlas->quadrants[qidx].shadows.ptr();

		int found_free_idx = -1; // found a free one
		int found_used_idx = -1; // found existing one, must steal it
		uint64_t min_pass =
			0; // pass of the existing one, try to use the least recently used one (LRU fashion)

		for (int j = 0; j < sc; j++) {
			if (!sarr[j].owner.is_valid()) {
				found_free_idx = j;
				break;
			}

			LightInstance* sli = light_instance_owner.get_or_null(sarr[j].owner);
			ERR_CONTINUE(!sli);

			if (sli->last_scene_pass != RendererSceneRenderRD::get_scene_pass()) {
				// was just allocated, don't kill it so soon, wait a bit..
				if (p_tick - sarr[j].alloc_tick < shadow_atlas_realloc_tolerance_msec) {
					continue;
				}

				if (found_used_idx == -1 || sli->last_scene_pass < min_pass) {
					found_used_idx = j;
					min_pass = sli->last_scene_pass;
				}
			}
		}

		if (found_free_idx == -1 && found_used_idx == -1) {
			continue; // nothing found
		}

		if (found_free_idx == -1 && found_used_idx != -1) {
			found_free_idx = found_used_idx;
		}

		r_quadrant = qidx;
		r_shadow = found_free_idx;

		return true;
	}

	return false;
}

bool RendererRD::LightStorage::_shadow_atlas_find_omni_shadows(ShadowAtlas* shadow_atlas,
	int* p_in_quadrants, int p_quadrant_count, int p_current_subdiv, uint64_t p_tick,
	int& r_quadrant, int& r_shadow)
{
	for (int i = p_quadrant_count - 1; i >= 0; i--) {
		int qidx = p_in_quadrants[i];

		if (shadow_atlas->quadrants[qidx].subdivision == (uint32_t)p_current_subdiv) {
			return false;
		}

		// look for an empty space
		int sc = shadow_atlas->quadrants[qidx].shadows.size();
		const ShadowAtlas::Quadrant::Shadow* sarr = shadow_atlas->quadrants[qidx].shadows.ptr();

		int found_idx = -1;
		uint64_t min_pass =
			0; // sum of currently selected spots, try to get the least recently used pair

		for (int j = 0; j < sc - 1; j++) {
			uint64_t pass = 0;

			if (sarr[j].owner.is_valid()) {
				LightInstance* sli = light_instance_owner.get_or_null(sarr[j].owner);
				ERR_CONTINUE(!sli);

				if (sli->last_scene_pass == RendererSceneRenderRD::get_scene_pass()) {
					continue;
				}

				// was just allocated, don't kill it so soon, wait a bit..
				if (p_tick - sarr[j].alloc_tick < shadow_atlas_realloc_tolerance_msec) {
					continue;
				}
				pass += sli->last_scene_pass;
			}

			if (sarr[j + 1].owner.is_valid()) {
				LightInstance* sli = light_instance_owner.get_or_null(sarr[j + 1].owner);
				ERR_CONTINUE(!sli);

				if (sli->last_scene_pass == RendererSceneRenderRD::get_scene_pass()) {
					continue;
				}

				// was just allocated, don't kill it so soon, wait a bit..
				if (p_tick - sarr[j + 1].alloc_tick < shadow_atlas_realloc_tolerance_msec) {
					continue;
				}
				pass += sli->last_scene_pass;
			}

			if (found_idx == -1 || pass < min_pass) {
				found_idx = j;
				min_pass = pass;

				// we found two empty spots, no need to check the rest
				if (pass == 0) {
					break;
				}
			}
		}

		if (found_idx == -1) {
			continue; // nothing found
		}

		r_quadrant = qidx;
		r_shadow = found_idx;

		return true;
	}

	return false;
}

bool RendererRD::LightStorage::shadow_atlas_update_light(
	RID p_atlas, RID p_light_instance, float p_coverage, uint64_t p_light_version)
{
	ShadowAtlas* shadow_atlas = shadow_atlas_owner.get_or_null(p_atlas);
	ERR_FAIL_NULL_V(shadow_atlas, false);

	LightInstance* li = light_instance_owner.get_or_null(p_light_instance);
	ERR_FAIL_NULL_V(li, false);

	if (shadow_atlas->size == 0 || shadow_atlas->smallest_subdiv == 0) {
		return false;
	}

	uint32_t quad_size = shadow_atlas->size >> 1;
	int desired_fit = MIN(quad_size / shadow_atlas->smallest_subdiv,
		Math::next_power_of_2(uint32_t(quad_size * p_coverage)));

	int valid_quadrants[4];
	int valid_quadrant_count = 0;
	int best_size = -1;	  // best size found
	int best_subdiv = -1; // subdiv for the best size

	// find the quadrants this fits into, and the best possible size it can fit into
	for (int i = 0; i < 4; i++) {
		int q = shadow_atlas->size_order[i];
		int sd = shadow_atlas->quadrants[q].subdivision;
		if (sd == 0) {
			continue; // unused
		}

		int max_fit = quad_size / sd;

		if (best_size != -1 && max_fit > best_size) {
			break; // too large
		}

		valid_quadrants[valid_quadrant_count++] = q;
		best_subdiv = sd;

		if (max_fit >= desired_fit) {
			best_size = max_fit;
		}
	}

	ERR_FAIL_COND_V(valid_quadrant_count == 0, false);

	uint64_t tick = OS::get_ticks_msec();

	uint32_t old_key = SHADOW_INVALID;
	uint32_t old_quadrant = SHADOW_INVALID;
	uint32_t old_shadow = SHADOW_INVALID;
	int old_subdivision = -1;

	bool should_realloc = false;
	bool should_redraw = false;

	if (shadow_atlas->shadow_owners.has(p_light_instance)) {
		old_key = shadow_atlas->shadow_owners[p_light_instance];
		old_quadrant = (old_key >> QUADRANT_SHIFT) & 0x3;
		old_shadow = old_key & SHADOW_INDEX_MASK;

		should_realloc =
			shadow_atlas->quadrants[old_quadrant].subdivision != (uint32_t)best_subdiv &&
			(tick - shadow_atlas->quadrants[old_quadrant].shadows[old_shadow].alloc_tick >
				shadow_atlas_realloc_tolerance_msec);
		should_redraw =
			shadow_atlas->quadrants[old_quadrant].shadows[old_shadow].version != p_light_version;

		if (!should_realloc) {
			shadow_atlas->quadrants[old_quadrant].shadows.write[old_shadow].version =
				p_light_version;
			// already existing, see if it should redraw or it's just OK
			return should_redraw;
		}

		old_subdivision = shadow_atlas->quadrants[old_quadrant].subdivision;
	}

	bool is_omni = li->light_type == RSE::LIGHT_OMNI;
	bool found_shadow = false;
	int new_quadrant = -1;
	int new_shadow = -1;

	if (is_omni) {
		found_shadow = _shadow_atlas_find_omni_shadows(shadow_atlas, valid_quadrants,
			valid_quadrant_count, old_subdivision, tick, new_quadrant, new_shadow);
	}
	else {
		found_shadow = _shadow_atlas_find_shadow(shadow_atlas, valid_quadrants,
			valid_quadrant_count, old_subdivision, tick, new_quadrant, new_shadow);
	}

	if (found_shadow) {
		if (old_quadrant != SHADOW_INVALID) {
			shadow_atlas->quadrants[old_quadrant].shadows.write[old_shadow].version = 0;
			shadow_atlas->quadrants[old_quadrant].shadows.write[old_shadow].owner = RID();

			if (old_key & OMNI_LIGHT_FLAG) {
				shadow_atlas->quadrants[old_quadrant].shadows.write[old_shadow + 1].version = 0;
				shadow_atlas->quadrants[old_quadrant].shadows.write[old_shadow + 1].owner = RID();
			}
		}

		uint32_t new_key = new_quadrant << QUADRANT_SHIFT;
		new_key |= new_shadow;

		ShadowAtlas::Quadrant::Shadow* sh =
			&shadow_atlas->quadrants[new_quadrant].shadows.write[new_shadow];
		_shadow_atlas_invalidate_shadow(sh, p_atlas, shadow_atlas, new_quadrant, new_shadow);

		sh->owner = p_light_instance;
		sh->alloc_tick = tick;
		sh->version = p_light_version;

		if (is_omni) {
			new_key |= OMNI_LIGHT_FLAG;

			int new_omni_shadow = new_shadow + 1;
			ShadowAtlas::Quadrant::Shadow* extra_sh =
				&shadow_atlas->quadrants[new_quadrant].shadows.write[new_omni_shadow];
			_shadow_atlas_invalidate_shadow(
				extra_sh, p_atlas, shadow_atlas, new_quadrant, new_omni_shadow);

			extra_sh->owner = p_light_instance;
			extra_sh->alloc_tick = tick;
			extra_sh->version = p_light_version;
		}

		li->shadow_atlases.insert(p_atlas);

		// update it in map
		shadow_atlas->shadow_owners[p_light_instance] = new_key;
		// make it dirty, as it should redraw anyway
		return true;
	}

	return should_redraw;
}

void RendererRD::LightStorage::_shadow_atlas_invalidate_shadow(
	ShadowAtlas::Quadrant::Shadow* p_shadow, RID p_atlas, ShadowAtlas* p_shadow_atlas,
	uint32_t p_quadrant, uint32_t p_shadow_idx)
{
	if (p_shadow->owner.is_valid()) {
		LightInstance* sli = light_instance_owner.get_or_null(p_shadow->owner);
		uint32_t old_key = p_shadow_atlas->shadow_owners[p_shadow->owner];

		if (old_key & OMNI_LIGHT_FLAG) {
			uint32_t s = old_key & SHADOW_INDEX_MASK;
			uint32_t omni_shadow_idx = p_shadow_idx + (s == (uint32_t)p_shadow_idx ? 1 : -1);
			ShadowAtlas::Quadrant::Shadow* omni_shadow =
				&p_shadow_atlas->quadrants[p_quadrant].shadows.write[omni_shadow_idx];
			omni_shadow->version = 0;
			omni_shadow->owner = RID();
		}

		p_shadow_atlas->shadow_owners.erase(p_shadow->owner);
		p_shadow->version = 0;
		p_shadow->owner = RID();
		sli->shadow_atlases.erase(p_atlas);
	}
}

void RendererRD::LightStorage::shadow_atlas_update(RID p_atlas)
{
	ShadowAtlas* shadow_atlas = shadow_atlas_owner.get_or_null(p_atlas);
	ERR_FAIL_NULL(shadow_atlas);

	_update_shadow_atlas(shadow_atlas);
}

RDC::DataFormat RendererRD::LightStorage::get_shadow_atlas_depth_format(bool p_16_bits)
{
	return p_16_bits ? RDC::DATA_FORMAT_D16_UNORM : RDC::DATA_FORMAT_D32_SFLOAT;
}

uint32_t RendererRD::LightStorage::get_shadow_atlas_depth_usage_bits()
{
	return RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
}

void RendererRD::LightStorage::update_directional_shadow_atlas()
{
	if (directional_shadow.depth.is_null() && directional_shadow.size > 0) {
		RDC::TextureFormat tf;
		tf.format = get_shadow_atlas_depth_format(directional_shadow.use_16_bits);
		tf.width = directional_shadow.size;
		tf.height = directional_shadow.size;
		tf.usage_bits = get_shadow_atlas_depth_usage_bits();

		directional_shadow.depth = RD::texture_create(tf, RDD::TextureView());
		Vector<RID> fb_tex;
		fb_tex.push_back(directional_shadow.depth);
		directional_shadow.fb = RD::framebuffer_create(fb_tex);
	}
}

void RendererRD::LightStorage::directional_shadow_atlas_set_size(int p_size, bool p_16_bits)
{
	p_size = Math::nearest_power_of_2_templated(p_size);

	if (directional_shadow.size == p_size && directional_shadow.use_16_bits == p_16_bits) {
		return;
	}

	directional_shadow.size = p_size;
	directional_shadow.use_16_bits = p_16_bits;

	if (directional_shadow.depth.is_valid()) {
		RD::free_rid(directional_shadow.depth);
		directional_shadow.depth = RID();
		RendererSceneRenderRD::base_uniforms_changed();
	}
}

void RendererRD::LightStorage::set_directional_shadow_count(int p_count)
{
	directional_shadow.light_count = p_count;
	directional_shadow.current_light = 0;
}

static Rect2i _get_directional_shadow_rect(int p_size, int p_shadow_count, int p_shadow_index)
{
	int split_h = 1;
	int split_v = 1;

	while (split_h * split_v < p_shadow_count) {
		if (split_h == split_v) {
			split_h <<= 1;
		}
		else {
			split_v <<= 1;
		}
	}

	Rect2i rect(0, 0, p_size, p_size);
	rect.size.width /= split_h;
	rect.size.height /= split_v;

	rect.position.x = rect.size.width * (p_shadow_index % split_h);
	rect.position.y = rect.size.height * (p_shadow_index / split_h);

	return rect;
}

Rect2i RendererRD::LightStorage::get_directional_shadow_rect()
{
	return _get_directional_shadow_rect(
		directional_shadow.size, directional_shadow.light_count, directional_shadow.current_light);
}

int RendererRD::LightStorage::get_directional_light_shadow_size(RID p_light_instance)
{
	ERR_FAIL_COND_V(directional_shadow.light_count == 0, 0);

	Rect2i r =
		_get_directional_shadow_rect(directional_shadow.size, directional_shadow.light_count, 0);

	LightInstance* light_instance = light_instance_owner.get_or_null(p_light_instance);
	ERR_FAIL_NULL_V(light_instance, 0);

	switch (light_directional_get_shadow_mode(light_instance->light)) {
	case RSE::LIGHT_DIRECTIONAL_SHADOW_ORTHOGONAL:
		break; // none
	case RSE::LIGHT_DIRECTIONAL_SHADOW_PARALLEL_2_SPLITS:
		r.size.height /= 2;
		break;
	case RSE::LIGHT_DIRECTIONAL_SHADOW_PARALLEL_4_SPLITS:
		r.size /= 2;
		break;
	}

	return MAX(r.size.width, r.size.height);
}

RendererRD::LightStorage::ShadowCubemap* RendererRD::LightStorage::_get_shadow_cubemap(int p_size)
{
	if (!shadow_cubemaps.has(p_size)) {
		ShadowCubemap sc;
		{
			RDC::TextureFormat tf;
			tf.format = get_cubemap_depth_format();
			tf.width = p_size;

			tf.height = p_size;
			tf.texture_type = RDC::TEXTURE_TYPE_CUBE;
			tf.array_layers = 6;
			tf.usage_bits = get_cubemap_depth_usage_bits();
			sc.cubemap = RD::texture_create(tf, RDD::TextureView());
		}

		for (int i = 0; i < 6; i++) {
			RID side_texture =
				RD::texture_create_shared_from_slice(RD::TextureView(), sc.cubemap, i, 0);
			Vector<RID> fbtex;
			fbtex.push_back(side_texture);
			sc.side_fb[i] = RD::framebuffer_create(fbtex);
		}

		shadow_cubemaps[p_size] = sc;
	}

	return &shadow_cubemaps[p_size];
}

RID RendererRD::LightStorage::get_cubemap(int p_size)
{
	ShadowCubemap* cubemap = _get_shadow_cubemap(p_size);

	return cubemap->cubemap;
}

RID RendererRD::LightStorage::get_cubemap_fb(int p_size, int p_pass)
{
	ShadowCubemap* cubemap = _get_shadow_cubemap(p_size);

	return cubemap->side_fb[p_pass];
}

RDC::DataFormat RendererRD::LightStorage::get_cubemap_depth_format()
{
	return RD::texture_is_format_supported_for_usage(
			   RDC::DATA_FORMAT_D32_SFLOAT, RDC::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
			   ? RDC::DATA_FORMAT_D32_SFLOAT
			   : RDC::DATA_FORMAT_X8_D24_UNORM_PACK32;
}

uint32_t RendererRD::LightStorage::get_cubemap_depth_usage_bits()
{
	return RDC::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | RDC::TEXTURE_USAGE_SAMPLING_BIT;
}

bool RendererRD::LightStorage::get_shadow_cubemaps_used() { return shadow_cubemaps_used; }

bool RendererRD::LightStorage::get_shadow_dual_paraboloid_used()
{
	return shadow_dual_paraboloid_used;
}

RID RendererRD::LightStorage::reflection_atlas_create() { return RID(); }

void RendererRD::LightStorage::finalize()
{
	free_light_data();
	free_reflection_data();

	if (directional_shadow.depth.is_valid()) {
		RendererRD::LightStorage::free(directional_shadow.depth);
		directional_shadow.depth = RID();
	}

	for (KeyValue<int, ShadowCubemap>& E : shadow_cubemaps) {
		if (E.value.cubemap.is_valid()) {
			RendererRD::LightStorage::free(E.value.cubemap);
		}
	}
	shadow_cubemaps.clear();
}

bool RendererRD::LightStorage::has_directional_shadows(unsigned int p_index) { return false; }



// ============================================================================
// Merged from servers/rendering/renderer_rd/storage_rd/mesh_storage.cpp
// ============================================================================

#include "mesh_storage.h"
#include "servers/rendering/renderer_viewport.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/rendering_server_types.h"
#include "servers/rendering/storage/mesh_storage.h"

using namespace RendererRD;

bool MeshStorage::free(RID p_rid)
{
	if (owns_mesh(p_rid)) {
		mesh_free(p_rid);
		return true;
	}
	else if (owns_mesh_instance(p_rid)) {
		mesh_instance_free(p_rid);
		return true;
	}
	else if (owns_multimesh(p_rid)) {
		_multimesh_free(p_rid);
		return true;
	}
	else if (owns_skeleton(p_rid)) {
		skeleton_free(p_rid);
		return true;
	}

	return false;
}

RID MeshStorage::mesh_allocate() { return mesh_owner.allocate_rid(); }

void MeshStorage::mesh_initialize(RID p_rid) { mesh_owner.initialize_rid(p_rid, Mesh()); }

void MeshStorage::mesh_free(RID p_rid)
{
	mesh_clear(p_rid);
	mesh_set_shadow_mesh(p_rid, RID());
	Mesh* mesh = mesh_owner.get_or_null(p_rid);
	ERR_FAIL_NULL(mesh);

	mesh->dependency.deleted_notify(p_rid);
	if (mesh->instances.size()) {
		ERR_PRINT("deleting mesh with active instances");
	}
	if (mesh->shadow_owners.size()) {
		for (Mesh* E : mesh->shadow_owners) {
			Mesh* shadow_owner = E;
			shadow_owner->shadow_mesh = RID();
			shadow_owner->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MESH);
		}
	}
	mesh_owner.free(p_rid);
}

void MeshStorage::mesh_set_blend_shape_count(RID p_mesh, int p_blend_shape_count)
{
	ERR_FAIL_COND(p_blend_shape_count < 0);

	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL(mesh);

	ERR_FAIL_COND(mesh->surface_count > 0); // surfaces already exist

	mesh->blend_shape_count = p_blend_shape_count;
}

void MeshStorage::mesh_add_surface(RID p_mesh, const RenderingServerTypes::SurfaceData& p_surface)
{
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL(mesh);

	ERR_FAIL_COND(mesh->surface_count == RSE::MAX_MESH_SURFACES);

#ifdef DEBUG_ENABLED
	// do a validation, to catch errors first
	{
		uint32_t stride = 0;
		uint32_t attrib_stride = 0;
		uint32_t skin_stride = 0;

		for (int i = 0; i < RSE::ARRAY_WEIGHTS; i++) {
			if ((p_surface.format & (1ULL << i))) {
				switch (i) {
				case RSE::ARRAY_VERTEX: {
					if ((p_surface.format & RSE::ARRAY_FLAG_USE_2D_VERTICES) ||
						(p_surface.format & RSE::ARRAY_FLAG_COMPRESS_ATTRIBUTES)) {
						stride += sizeof(float) * 2;
					}
					else {
						stride += sizeof(float) * 3;
					}

				} break;
				case RSE::ARRAY_NORMAL: {
					stride += sizeof(uint16_t) * 2;

				} break;
				case RSE::ARRAY_TANGENT: {
					if (!(p_surface.format & RSE::ARRAY_FLAG_COMPRESS_ATTRIBUTES)) {
						stride += sizeof(uint16_t) * 2;
					}
				} break;
				case RSE::ARRAY_COLOR: {
					attrib_stride += sizeof(uint32_t);
				} break;
				case RSE::ARRAY_TEX_UV: {
					if (p_surface.format & RSE::ARRAY_FLAG_COMPRESS_ATTRIBUTES) {
						attrib_stride += sizeof(uint16_t) * 2;
					}
					else {
						attrib_stride += sizeof(float) * 2;
					}

				} break;
				case RSE::ARRAY_TEX_UV2: {
					if (p_surface.format & RSE::ARRAY_FLAG_COMPRESS_ATTRIBUTES) {
						attrib_stride += sizeof(uint16_t) * 2;
					}
					else {
						attrib_stride += sizeof(float) * 2;
					}

				} break;
				case RSE::ARRAY_CUSTOM0:
				case RSE::ARRAY_CUSTOM1:
				case RSE::ARRAY_CUSTOM2:
				case RSE::ARRAY_CUSTOM3: {
					int idx = i - RSE::ARRAY_CUSTOM0;
					const uint32_t fmt_shift[RSE::ARRAY_CUSTOM_COUNT] = {
						RSE::ARRAY_FORMAT_CUSTOM0_SHIFT, RSE::ARRAY_FORMAT_CUSTOM1_SHIFT,
						RSE::ARRAY_FORMAT_CUSTOM2_SHIFT, RSE::ARRAY_FORMAT_CUSTOM3_SHIFT};
					uint32_t fmt =
						(p_surface.format >> fmt_shift[idx]) & RSE::ARRAY_FORMAT_CUSTOM_MASK;
					const uint32_t fmtsize[RSE::ARRAY_CUSTOM_MAX] = {4, 4, 4, 8, 4, 8, 12, 16};
					attrib_stride += fmtsize[fmt];

				} break;
				case RSE::ARRAY_WEIGHTS:
				case RSE::ARRAY_BONES: {
					// uses a separate array
					bool use_8 = p_surface.format & RSE::ARRAY_FLAG_USE_8_BONE_WEIGHTS;
					skin_stride += sizeof(int16_t) * (use_8 ? 16 : 8);
				} break;
				}
			}
		}

		int expected_size = stride * p_surface.vertex_count;
		ERR_FAIL_COND_MSG(expected_size != p_surface.vertex_data.size(),
			"Size of vertex data provided (" + itos(p_surface.vertex_data.size()) +
				") does not match expected (" + itos(expected_size) + ")");

		int bs_expected_size = expected_size * mesh->blend_shape_count;

		ERR_FAIL_COND_MSG(bs_expected_size != p_surface.blend_shape_data.size(),
			"Size of blend shape data provided (" + itos(p_surface.blend_shape_data.size()) +
				") does not match expected (" + itos(bs_expected_size) + ")");

		int expected_attrib_size = attrib_stride * p_surface.vertex_count;
		ERR_FAIL_COND_MSG(expected_attrib_size != p_surface.attribute_data.size(),
			"Size of attribute data provided (" + itos(p_surface.attribute_data.size()) +
				") does not match expected (" + itos(expected_attrib_size) + ")");

		if ((p_surface.format & RSE::ARRAY_FORMAT_WEIGHTS) &&
			(p_surface.format & RSE::ARRAY_FORMAT_BONES)) {
			expected_size = skin_stride * p_surface.vertex_count;
			ERR_FAIL_COND_MSG(expected_size != p_surface.skin_data.size(),
				"Size of skin data provided (" + itos(p_surface.skin_data.size()) +
					") does not match expected (" + itos(expected_size) + ")");
		}
	}

#endif

	uint64_t surface_version = p_surface.format & (uint64_t(RSE::ARRAY_FLAG_FORMAT_VERSION_MASK)
													  << RSE::ARRAY_FLAG_FORMAT_VERSION_SHIFT);
	RenderingServerTypes::SurfaceData new_surface = p_surface;
#ifdef DISABLE_DEPRECATED

	ERR_FAIL_COND_MSG(surface_version != RSE::ARRAY_FLAG_FORMAT_CURRENT_VERSION,
		"Surface version provided (" +
			itos(int(surface_version >> RSE::ARRAY_FLAG_FORMAT_VERSION_SHIFT)) +
			") does not match current version (" +
			itos(RSE::ARRAY_FLAG_FORMAT_CURRENT_VERSION >> RSE::ARRAY_FLAG_FORMAT_VERSION_SHIFT) +
			")");

#else

	if (surface_version != uint64_t(RSE::ARRAY_FLAG_FORMAT_CURRENT_VERSION)) {
		Renderer::fix_surface_compatibility(new_surface);
		surface_version = new_surface.format & (RSE::ARRAY_FLAG_FORMAT_VERSION_MASK
												   << RSE::ARRAY_FLAG_FORMAT_VERSION_SHIFT);
		ERR_FAIL_COND_MSG(surface_version != RSE::ARRAY_FLAG_FORMAT_CURRENT_VERSION,
			vformat("Surface version provided (%d) does not match current version (%d).",
				(surface_version >> RSE::ARRAY_FLAG_FORMAT_VERSION_SHIFT) &
					RSE::ARRAY_FLAG_FORMAT_VERSION_MASK,
				(RSE::ARRAY_FLAG_FORMAT_CURRENT_VERSION >> RSE::ARRAY_FLAG_FORMAT_VERSION_SHIFT) &
					RSE::ARRAY_FLAG_FORMAT_VERSION_MASK));
	}
#endif

	Mesh::Surface* s = memnew(Mesh::Surface);

	s->format = new_surface.format;
	s->primitive = new_surface.primitive;

	const bool use_as_storage = (new_surface.skin_data.size() || mesh->blend_shape_count > 0);
	const bool requested_storage_buffer = (new_surface.format & RSE::ARRAY_FLAG_USE_STORAGE_BUFFER);
	const uint32_t as_storage_flag =
		(use_as_storage || requested_storage_buffer) ? RD::BUFFER_CREATION_AS_STORAGE_BIT : 0;
	const uint32_t requested_storage_flag =
		requested_storage_buffer ? RD::BUFFER_CREATION_AS_STORAGE_BIT : 0;

	if (new_surface.vertex_data.size()) {
		// If we have an uncompressed surface that contains normals, but not tangents, we need to
		// differentiate the array from a compressed array in the shader. To do so, we allow the
		// normal to read 4 components out of the buffer But only give it 2 components per normal.
		// So essentially, each vertex reads the next normal in normal.zw. This allows us to avoid
		// adding a shader permutation, and avoid passing dummy tangents. Since the stride is kept
		// small this should still be a net win for bandwidth. If we do this, then the last normal
		// will read past the end of the array. So we need to pad the array with dummy data.
		if (!(new_surface.format & RSE::ARRAY_FLAG_COMPRESS_ATTRIBUTES) &&
			(new_surface.format & RSE::ARRAY_FORMAT_NORMAL) &&
			!(new_surface.format & RSE::ARRAY_FORMAT_TANGENT)) {
			// Unfortunately, we need to copy the buffer, which is fine as doing a resize triggers a
			// CoW anyway.
			Vector<uint8_t> new_vertex_data;
			new_vertex_data.resize_initialized(
				new_surface.vertex_data.size() + sizeof(uint16_t) * 2);
			memcpy(new_vertex_data.ptrw(), new_surface.vertex_data.ptr(),
				new_surface.vertex_data.size());
			s->vertex_buffer = RD::vertex_buffer_create(
				new_vertex_data.size(), new_vertex_data, as_storage_flag);
			s->vertex_buffer_size = new_vertex_data.size();
		}
		else {
			s->vertex_buffer = RD::vertex_buffer_create(
				new_surface.vertex_data.size(), new_surface.vertex_data, as_storage_flag);
			s->vertex_buffer_size = new_surface.vertex_data.size();
		}
	}

	if (new_surface.attribute_data.size()) {
		s->attribute_buffer = RD::vertex_buffer_create(
			new_surface.attribute_data.size(), new_surface.attribute_data, requested_storage_flag);
		s->attribute_buffer_size = new_surface.attribute_data.size();
	}
	if (new_surface.skin_data.size()) {
		s->skin_buffer = RD::vertex_buffer_create(
			new_surface.skin_data.size(), new_surface.skin_data, as_storage_flag);
		s->skin_buffer_size = new_surface.skin_data.size();
	}

	s->vertex_count = new_surface.vertex_count;

	if (new_surface.format & RSE::ARRAY_FORMAT_BONES) {
		mesh->has_bone_weights = true;
	}

	if (new_surface.index_count) {
		bool is_index_16 = new_surface.vertex_count <= 65536 && new_surface.vertex_count > 0;

		s->index_buffer_size = new_surface.index_data.size();
		s->index_count = new_surface.index_count;
		s->index_array =
			RD::index_array_create(s->index_buffer, 0, s->index_count);
		if (new_surface.lods.size()) {
			s->lods = memnew_arr(Mesh::Surface::LOD, new_surface.lods.size());
			s->lod_count = new_surface.lods.size();

			for (int i = 0; i < new_surface.lods.size(); i++) {
				uint32_t indices = new_surface.lods[i].index_data.size() / (is_index_16 ? 2 : 4);
				s->lods[i].index_buffer_size = new_surface.lods[i].index_data.size();
				s->lods[i].index_array =
					RD::index_array_create(s->lods[i].index_buffer, 0, indices);
				s->lods[i].edge_length = new_surface.lods[i].edge_length;
				s->lods[i].index_count = indices;
			}
		}
	}

	ERR_FAIL_COND_MSG(!new_surface.index_count && !new_surface.vertex_count,
		"Meshes must contain a vertex array, an index array, or both");

	s->aabb = new_surface.aabb;
	s->bone_aabbs = new_surface.bone_aabbs; // only really useful for returning them.
	s->mesh_to_skeleton_xform = p_surface.mesh_to_skeleton_xform;

	s->uv_scale = new_surface.uv_scale;

	if (mesh->blend_shape_count > 0) {
		s->blend_shape_buffer = RD::storage_buffer_create(
			new_surface.blend_shape_data.size(), new_surface.blend_shape_data);
		s->blend_shape_buffer_size = new_surface.blend_shape_data.size();
	}

	if (use_as_storage) {
		Vector<RD::Uniform> uniforms;
		{
			RD::Uniform u;
			u.binding = 0;
			u.uniform_type = RDC::UNIFORM_TYPE_STORAGE_BUFFER;
			if (s->vertex_buffer.is_valid()) {
				u.append_id(s->vertex_buffer);
			}
			else {
				u.append_id(default_rd_storage_buffer);
			}
			uniforms.push_back(u);
		}
		{
			RD::Uniform u;
			u.binding = 1;
			u.uniform_type = RDC::UNIFORM_TYPE_STORAGE_BUFFER;
			if (s->skin_buffer.is_valid()) {
				u.append_id(s->skin_buffer);
			}
			else {
				u.append_id(default_rd_storage_buffer);
			}
			uniforms.push_back(u);
		}
		{
			RD::Uniform u;
			u.binding = 2;
			u.uniform_type = RDC::UNIFORM_TYPE_STORAGE_BUFFER;
			if (s->blend_shape_buffer.is_valid()) {
				u.append_id(s->blend_shape_buffer);
			}
			else {
				u.append_id(default_rd_storage_buffer);
			}
			uniforms.push_back(u);
		}

		s->uniform_set = RD::uniform_set_create(
			uniforms, skeleton_shader->version_shader[0], SkeletonShader::UNIFORM_SET_SURFACE);
	}

	if (mesh->surface_count == 0) {
		mesh->aabb = new_surface.aabb;
	}
	else {
		mesh->aabb.merge_with(new_surface.aabb);
	}
	mesh->skeleton_aabb_version = 0;

	s->material = new_surface.material;

	mesh->surfaces = (Mesh::Surface**)memrealloc(
		mesh->surfaces, sizeof(Mesh::Surface*) * (mesh->surface_count + 1));
	mesh->surfaces[mesh->surface_count] = s;
	mesh->surface_count++;

	for (MeshInstance* mi : mesh->instances) {
		_mesh_instance_add_surface(mi, mesh, mesh->surface_count - 1);
	}

	mesh->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MESH);

	for (Mesh* E : mesh->shadow_owners) {
		Mesh* shadow_owner = E;
		shadow_owner->shadow_mesh = RID();
		shadow_owner->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MESH);
	}

	mesh->material_cache.clear();
}

void MeshStorage::_mesh_surface_clear(Mesh* p_mesh, int p_surface)
{
	Mesh::Surface& s = *p_mesh->surfaces[p_surface];

	if (s.vertex_buffer.is_valid()) {
		RD::free_rid(
			s.vertex_buffer); // Clears arrays as dependency automatically, including all versions.
	}
	if (s.attribute_buffer.is_valid()) {
		RD::free_rid(s.attribute_buffer);
	}
	if (s.skin_buffer.is_valid()) {
		RD::free_rid(s.skin_buffer);
	}
	if (s.versions) {
		memfree(s.versions); // reallocs, so free with memfree.
	}

	if (s.index_buffer.is_valid()) {
		RD::free_rid(s.index_buffer);
	}

	if (s.lod_count) {
		for (uint32_t j = 0; j < s.lod_count; j++) {
			RD::free_rid(s.lods[j].index_buffer);
		}
		memdelete_arr(s.lods);
	}

	if (s.blend_shape_buffer.is_valid()) {
		RD::free_rid(s.blend_shape_buffer);
	}

	memdelete(p_mesh->surfaces[p_surface]);
}

int MeshStorage::mesh_get_blend_shape_count(RID p_mesh)
{
	const Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL_V(mesh, -1);
	return mesh->blend_shape_count;
}

void MeshStorage::mesh_set_blend_shape_mode(RID p_mesh, RSE::BlendShapeMode p_mode)
{
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL(mesh);
	ERR_FAIL_INDEX((int)p_mode, 2);

	mesh->blend_shape_mode = p_mode;
}

RSE::BlendShapeMode MeshStorage::mesh_get_blend_shape_mode(RID p_mesh)
{
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL_V(mesh, RSE::BLEND_SHAPE_MODE_NORMALIZED);
	return mesh->blend_shape_mode;
}

RID MeshStorage::mesh_surface_get_vertex_buffer_rd_rid(RID p_mesh, int p_surface)
{
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL_V(mesh, RID());
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_surface, mesh->surface_count, RID());
	return mesh->surfaces[p_surface]->vertex_buffer;
}

RID MeshStorage::mesh_surface_get_attribute_buffer_rd_rid(RID p_mesh, int p_surface)
{
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL_V(mesh, RID());
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_surface, mesh->surface_count, RID());
	return mesh->surfaces[p_surface]->attribute_buffer;
}

RID MeshStorage::mesh_surface_get_skin_buffer_rd_rid(RID p_mesh, int p_surface)
{
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL_V(mesh, RID());
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_surface, mesh->surface_count, RID());
	return mesh->surfaces[p_surface]->skin_buffer;
}

RID MeshStorage::mesh_surface_get_index_buffer_rd_rid(RID p_mesh, int p_surface)
{
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL_V(mesh, RID());
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_surface, mesh->surface_count, RID());
	return mesh->surfaces[p_surface]->index_buffer;
}

void MeshStorage::mesh_surface_set_material(RID p_mesh, int p_surface, RID p_material)
{
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL(mesh);
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_surface, mesh->surface_count);
	mesh->surfaces[p_surface]->material = p_material;

	mesh->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MATERIAL);
	mesh->material_cache.clear();
}

RID MeshStorage::mesh_surface_get_material(RID p_mesh, int p_surface)
{
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL_V(mesh, RID());
	ERR_FAIL_UNSIGNED_INDEX_V((uint32_t)p_surface, mesh->surface_count, RID());

	return mesh->surfaces[p_surface]->material;
}

RenderingServerTypes::SurfaceData MeshStorage::mesh_get_surface(RID p_mesh, int p_surface)
{
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL_V(mesh, RenderingServerTypes::SurfaceData());
	ERR_FAIL_UNSIGNED_INDEX_V(
		(uint32_t)p_surface, mesh->surface_count, RenderingServerTypes::SurfaceData());

	Mesh::Surface& s = *mesh->surfaces[p_surface];

	RenderingServerTypes::SurfaceData sd;
	sd.format = s.format;
	if (s.vertex_buffer.is_valid()) {
		sd.vertex_data = RD::buffer_get_data(s.vertex_buffer);
		// When using an uncompressed buffer with normals, but without tangents, we have to trim the
		// padding.
		if (!(s.format & RSE::ARRAY_FLAG_COMPRESS_ATTRIBUTES) &&
			(s.format & RSE::ARRAY_FORMAT_NORMAL) && !(s.format & RSE::ARRAY_FORMAT_TANGENT)) {
			sd.vertex_data.resize(sd.vertex_data.size() - sizeof(uint16_t) * 2);
		}
	}
	if (s.attribute_buffer.is_valid()) {
		sd.attribute_data = RD::buffer_get_data(s.attribute_buffer);
	}
	if (s.skin_buffer.is_valid()) {
		sd.skin_data = RD::buffer_get_data(s.skin_buffer);
	}
	sd.vertex_count = s.vertex_count;
	sd.index_count = s.index_count;
	sd.primitive = s.primitive;

	if (sd.index_count) {
		sd.index_data = RD::buffer_get_data(s.index_buffer);
	}
	sd.aabb = s.aabb;
	sd.uv_scale = s.uv_scale;
	for (uint32_t i = 0; i < s.lod_count; i++) {
		RenderingServerTypes::SurfaceData::LOD lod;
		lod.edge_length = s.lods[i].edge_length;
		lod.index_data = RD::buffer_get_data(s.lods[i].index_buffer);
		sd.lods.push_back(lod);
	}

	sd.bone_aabbs = s.bone_aabbs;
	sd.mesh_to_skeleton_xform = s.mesh_to_skeleton_xform;

	if (s.blend_shape_buffer.is_valid()) {
		sd.blend_shape_data = RD::buffer_get_data(s.blend_shape_buffer);
	}

	return sd;
}

int MeshStorage::mesh_get_surface_count(RID p_mesh)
{
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL_V(mesh, 0);
	return mesh->surface_count;
}

void MeshStorage::mesh_set_custom_aabb(RID p_mesh, const AABB& p_aabb)
{
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL(mesh);
	mesh->custom_aabb = p_aabb;

	mesh->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_AABB);
}

AABB MeshStorage::mesh_get_custom_aabb(RID p_mesh)
{
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL_V(mesh, AABB());
	return mesh->custom_aabb;
}

AABB MeshStorage::mesh_get_aabb(RID p_mesh, RID p_skeleton)
{
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL_V(mesh, AABB());

	if (mesh->custom_aabb != AABB()) {
		return mesh->custom_aabb;
	}

	Skeleton* skeleton = skeleton_owner.get_or_null(p_skeleton);

	// A mesh can be shared by multiple skeletons and we need to avoid using the AABB from a
	// different skeleton.
	if (!skeleton || skeleton->size == 0 ||
		(mesh->skeleton_aabb_version == skeleton->version &&
			mesh->skeleton_aabb_rid == p_skeleton)) {
		return mesh->aabb;
	}

	AABB aabb;

	for (uint32_t i = 0; i < mesh->surface_count; i++) {
		AABB laabb;
		const Mesh::Surface& surface = *mesh->surfaces[i];
		if ((surface.format & RSE::ARRAY_FORMAT_BONES) && surface.bone_aabbs.size()) {
			int bs = surface.bone_aabbs.size();
			const AABB* skbones = surface.bone_aabbs.ptr();

			int sbs = skeleton->size;
			ERR_CONTINUE(bs > sbs);
			const float* baseptr = skeleton->data.ptr();

			bool found_bone_aabb = false;

			if (skeleton->use_2d) {
				for (int j = 0; j < bs; j++) {
					if (skbones[j].size == Vector3(-1, -1, -1)) {
						continue; // bone is unused
					}

					const float* dataptr = baseptr + j * 8;

					Transform3D mtx;

					mtx.basis.rows[0][0] = dataptr[0];
					mtx.basis.rows[0][1] = dataptr[1];
					mtx.origin.x = dataptr[3];

					mtx.basis.rows[1][0] = dataptr[4];
					mtx.basis.rows[1][1] = dataptr[5];
					mtx.origin.y = dataptr[7];

					// Transform bounds to skeleton's space before applying animation data.
					AABB baabb = surface.mesh_to_skeleton_xform.xform(skbones[j]);
					baabb = mtx.xform(baabb);

					if (!found_bone_aabb) {
						laabb = baabb;
						found_bone_aabb = true;
					}
					else {
						laabb.merge_with(baabb);
					}
				}
			}
			else {
				for (int j = 0; j < bs; j++) {
					if (skbones[j].size == Vector3(-1, -1, -1)) {
						continue; // bone is unused
					}

					const float* dataptr = baseptr + j * 12;

					Transform3D mtx;

					mtx.basis.rows[0][0] = dataptr[0];
					mtx.basis.rows[0][1] = dataptr[1];
					mtx.basis.rows[0][2] = dataptr[2];
					mtx.origin.x = dataptr[3];
					mtx.basis.rows[1][0] = dataptr[4];
					mtx.basis.rows[1][1] = dataptr[5];
					mtx.basis.rows[1][2] = dataptr[6];
					mtx.origin.y = dataptr[7];
					mtx.basis.rows[2][0] = dataptr[8];
					mtx.basis.rows[2][1] = dataptr[9];
					mtx.basis.rows[2][2] = dataptr[10];
					mtx.origin.z = dataptr[11];

					// Transform bounds to skeleton's space before applying animation data.
					AABB baabb = surface.mesh_to_skeleton_xform.xform(skbones[j]);
					baabb = mtx.xform(baabb);

					if (!found_bone_aabb) {
						laabb = baabb;
						found_bone_aabb = true;
					}
					else {
						laabb.merge_with(baabb);
					}
				}
			}

			if (found_bone_aabb) {
				// Transform skeleton bounds back to mesh's space if any animated AABB applied.
				laabb = surface.mesh_to_skeleton_xform.affine_inverse().xform(laabb);
			}

			if (laabb.size == Vector3()) {
				laabb = surface.aabb;
			}
		}
		else {
			laabb = surface.aabb;
		}

		if (i == 0) {
			aabb = laabb;
		}
		else {
			aabb.merge_with(laabb);
		}
	}

	mesh->aabb = aabb;

	mesh->skeleton_aabb_version = skeleton->version;
	mesh->skeleton_aabb_rid = p_skeleton;
	return aabb;
}

void MeshStorage::mesh_set_path(RID p_mesh, const String& p_path)
{
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL(mesh);

	mesh->path = p_path;
}

String MeshStorage::mesh_get_path(RID p_mesh)
{
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL_V(mesh, String());

	return mesh->path;
}

void MeshStorage::mesh_set_shadow_mesh(RID p_mesh, RID p_shadow_mesh)
{
	ERR_FAIL_COND_MSG(p_mesh == p_shadow_mesh, "Cannot set a mesh as its own shadow mesh.");
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL(mesh);

	Mesh* shadow_mesh = mesh_owner.get_or_null(mesh->shadow_mesh);
	if (shadow_mesh) {
		shadow_mesh->shadow_owners.erase(mesh);
	}
	mesh->shadow_mesh = p_shadow_mesh;

	shadow_mesh = mesh_owner.get_or_null(mesh->shadow_mesh);

	if (shadow_mesh) {
		shadow_mesh->shadow_owners.insert(mesh);
	}

	mesh->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MESH);
}

void MeshStorage::mesh_clear(RID p_mesh)
{
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL(mesh);

	// Clear instance data before mesh data.
	for (MeshInstance* mi : mesh->instances) {
		_mesh_instance_clear(mi);
	}

	for (uint32_t i = 0; i < mesh->surface_count; i++) {
		_mesh_surface_clear(mesh, i);
	}
	if (mesh->surfaces) {
		memfree(mesh->surfaces);
	}

	mesh->surfaces = nullptr;
	mesh->surface_count = 0;
	mesh->material_cache.clear();
	mesh->has_bone_weights = false;
	mesh->aabb = AABB();
	mesh->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MESH);

	for (Mesh* E : mesh->shadow_owners) {
		Mesh* shadow_owner = E;
		shadow_owner->shadow_mesh = RID();
		shadow_owner->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MESH);
	}
}

void MeshStorage::mesh_surface_remove(RID p_mesh, int p_surface)
{
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL(mesh);
	ERR_FAIL_UNSIGNED_INDEX((uint32_t)p_surface, mesh->surface_count);

	// Clear instance data before mesh data.
	for (MeshInstance* mi : mesh->instances) {
		_mesh_instance_remove_surface(mi, p_surface);
	}

	_mesh_surface_clear(mesh, p_surface);

	if ((uint32_t)p_surface < mesh->surface_count - 1) {
		memmove(mesh->surfaces + p_surface, mesh->surfaces + p_surface + 1,
			sizeof(Mesh::Surface*) * (mesh->surface_count - (p_surface + 1)));
	}
	mesh->surfaces = (Mesh::Surface**)memrealloc(
		mesh->surfaces, sizeof(Mesh::Surface*) * (mesh->surface_count - 1));
	--mesh->surface_count;

	mesh->material_cache.clear();

	mesh->skeleton_aabb_version = 0;

	if (mesh->has_bone_weights) {
		mesh->has_bone_weights = false;
		for (uint32_t i = 0; i < mesh->surface_count; i++) {
			if (mesh->surfaces[i]->format & RSE::ARRAY_FORMAT_BONES) {
				mesh->has_bone_weights = true;
				break;
			}
		}
	}

	if (mesh->surface_count == 0) {
		mesh->aabb = AABB();
	}
	else {
		mesh->aabb = mesh->surfaces[0]->aabb;
		for (uint32_t i = 1; i < mesh->surface_count; i++) {
			mesh->aabb.merge_with(mesh->surfaces[i]->aabb);
		}
	}

	mesh->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MESH);

	for (Mesh* E : mesh->shadow_owners) {
		Mesh* shadow_owner = E;
		shadow_owner->shadow_mesh = RID();
		shadow_owner->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MESH);
	}
}

void MeshStorage::mesh_debug_usage(List<RenderingServerTypes::MeshInfo>* r_info)
{
	for (const RID& mesh_rid : mesh_owner.get_owned_list()) {
		Mesh* mesh = mesh_owner.get_or_null(mesh_rid);
		if (!mesh) {
			continue;
		}
		RenderingServerTypes::MeshInfo mesh_info;
		mesh_info.mesh = mesh_rid;
		mesh_info.path = mesh->path;

		for (uint32_t surface_index = 0; surface_index < mesh->surface_count; surface_index++) {
			MeshStorage::Mesh::Surface* surface = mesh->surfaces[surface_index];

			mesh_info.vertex_buffer_size += surface->vertex_buffer_size;
			mesh_info.attribute_buffer_size += surface->attribute_buffer_size;
			mesh_info.skin_buffer_size += surface->skin_buffer_size;
			mesh_info.index_buffer_size += surface->index_buffer_size;
			mesh_info.blend_shape_buffer_size += surface->blend_shape_buffer_size;
			mesh_info.vertex_count += surface->vertex_count;

			for (uint32_t lod_index = 0; lod_index < surface->lod_count; lod_index++) {
				mesh_info.lod_index_buffers_size += surface->lods[lod_index].index_buffer_size;
			}
		}

		r_info->push_back(mesh_info);
	}
}

bool MeshStorage::mesh_needs_instance(RID p_mesh, bool p_has_skeleton)
{
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL_V(mesh, false);

	return mesh->blend_shape_count > 0 || (mesh->has_bone_weights && p_has_skeleton);
}

Dependency* MeshStorage::mesh_get_dependency(RID p_mesh)
{
	Mesh* mesh = mesh_owner.get_or_null(p_mesh);
	ERR_FAIL_NULL_V(mesh, nullptr);

	return &mesh->dependency;
}

RID MeshStorage::mesh_instance_create(RID p_base)
{
	Mesh* mesh = mesh_owner.get_or_null(p_base);
	ERR_FAIL_NULL_V(mesh, RID());

	RID rid = mesh_instance_owner.make_rid();
	MeshInstance* mi = mesh_instance_owner.get_or_null(rid);

	mi->mesh = mesh;

	for (uint32_t i = 0; i < mesh->surface_count; i++) {
		_mesh_instance_add_surface(mi, mesh, i);
	}

	mi->I = mesh->instances.push_back(mi);

	mi->dirty = true;

	return rid;
}

void MeshStorage::mesh_instance_free(RID p_rid)
{
	MeshInstance* mi = mesh_instance_owner.get_or_null(p_rid);
	_mesh_instance_clear(mi);
	mi->mesh->instances.erase(mi->I);
	mi->I = nullptr;

	mesh_instance_owner.free(p_rid);
}

void MeshStorage::mesh_instance_set_skeleton(RID p_mesh_instance, RID p_skeleton)
{
	MeshInstance* mi = mesh_instance_owner.get_or_null(p_mesh_instance);
	if (mi->skeleton == p_skeleton) {
		return;
	}
	mi->skeleton = p_skeleton;
	mi->skeleton_version = 0;
	mi->dirty = true;
}

void MeshStorage::mesh_instance_set_blend_shape_weight(
	RID p_mesh_instance, int p_shape, float p_weight)
{
	MeshInstance* mi = mesh_instance_owner.get_or_null(p_mesh_instance);
	ERR_FAIL_NULL(mi);
	ERR_FAIL_INDEX(p_shape, (int)mi->blend_weights.size());
	mi->blend_weights[p_shape] = p_weight;
	mi->weights_dirty = true;
	// will be eventually updated
}

void MeshStorage::_mesh_instance_clear(MeshInstance* mi)
{
	while (mi->surfaces.size()) {
		_mesh_instance_remove_surface(mi, mi->surfaces.size() - 1);
	}
	mi->dirty = false;
}

void MeshStorage::_mesh_instance_add_surface(MeshInstance* mi, Mesh* mesh, uint32_t p_surface)
{
	if (mesh->blend_shape_count > 0 && mi->blend_weights_buffer.is_null()) {
		mi->blend_weights.resize(mesh->blend_shape_count);
		for (float& weight : mi->blend_weights) {
			weight = 0;
		}
		mi->blend_weights_buffer =
			RD::storage_buffer_create(sizeof(float) * mi->blend_weights.size(),
				mi->blend_weights.span().reinterpret<uint8_t>());
		mi->weights_dirty = true;
	}

	MeshInstance::Surface s;
	if ((mesh->blend_shape_count > 0 ||
			(mesh->surfaces[p_surface]->format & RSE::ARRAY_FORMAT_BONES)) &&
		mesh->surfaces[p_surface]->vertex_buffer_size > 0) {
		_mesh_instance_add_surface_buffer(mi, mesh, &s, p_surface, 0);
	}

	mi->surfaces.push_back(s);
	mi->dirty = true;
}

void MeshStorage::_mesh_instance_add_surface_buffer(MeshInstance* mi, Mesh* mesh,
	MeshInstance::Surface* s, uint32_t p_surface, uint32_t p_buffer_index)
{
	s->vertex_buffer[p_buffer_index] =
		RD::vertex_buffer_create(mesh->surfaces[p_surface]->vertex_buffer_size,
			Vector<uint8_t>(), RD::BUFFER_CREATION_AS_STORAGE_BIT);

	Vector<RD::Uniform> uniforms;
	{
		RD::Uniform u;
		u.binding = 1;
		u.uniform_type = RDC::UNIFORM_TYPE_STORAGE_BUFFER;
		u.append_id(s->vertex_buffer[p_buffer_index]);
		uniforms.push_back(u);
	}
	{
		RD::Uniform u;
		u.binding = 2;
		u.uniform_type = RDC::UNIFORM_TYPE_STORAGE_BUFFER;
		if (mi->blend_weights_buffer.is_valid()) {
			u.append_id(mi->blend_weights_buffer);
		}
		else {
			u.append_id(default_rd_storage_buffer);
		}
		uniforms.push_back(u);
	}
	s->uniform_set[p_buffer_index] = RD::uniform_set_create(
		uniforms, skeleton_shader->version_shader[0], SkeletonShader::UNIFORM_SET_INSTANCE);
}

void MeshStorage::_mesh_instance_remove_surface(MeshInstance* mi, int p_surface)
{
	MeshInstance::Surface& surface = mi->surfaces[p_surface];

	if (surface.versions) {
		for (uint32_t j = 0; j < surface.version_count; j++) {
			RD::free_rid(surface.versions[j].vertex_array);
		}
		memfree(surface.versions);
	}
	for (uint32_t i = 0; i < 2; i++) {
		if (surface.vertex_buffer[i].is_valid()) {
			RD::free_rid(surface.vertex_buffer[i]);
		}
	}

	mi->surfaces.remove_at(p_surface);

	if (mi->surfaces.is_empty()) {
		if (mi->blend_weights_buffer.is_valid()) {
			RD::free_rid(mi->blend_weights_buffer);
			mi->blend_weights_buffer = RID();
		}

		mi->blend_weights.clear();
		mi->weights_dirty = false;
		mi->skeleton_version = 0;
	}
	mi->dirty = true;
}

void MeshStorage::mesh_instance_check_for_update(RID p_mesh_instance)
{
	MeshInstance* mi = mesh_instance_owner.get_or_null(p_mesh_instance);

	bool needs_update = mi->dirty;

	if (mi->weights_dirty && !mi->weight_update_list.in_list()) {
		dirty_mesh_instance_weights.add(&mi->weight_update_list);
		needs_update = true;
	}

	if (mi->array_update_list.in_list()) {
		return;
	}

	if (!needs_update && mi->skeleton.is_valid()) {
		Skeleton* sk = skeleton_owner.get_or_null(mi->skeleton);
		if (sk && sk->version != mi->skeleton_version) {
			needs_update = true;
		}
	}

	if (needs_update) {
		dirty_mesh_instance_arrays.add(&mi->array_update_list);
	}
}

void MeshStorage::mesh_instance_set_canvas_item_transform(
	RID p_mesh_instance, const Transform2D& p_transform)
{
	MeshInstance* mi = mesh_instance_owner.get_or_null(p_mesh_instance);
	mi->canvas_item_transform_2d = p_transform;
}

RD::VertexFormatID MeshStorage::_mesh_surface_generate_vertex_format(uint64_t p_surface_format,
	uint64_t p_input_mask, bool p_instanced_surface, bool p_input_motion_vectors,
	bool p_point_size_emulated, uint32_t& r_position_stride)
{
	Vector<RDC::VertexAttribute> attributes;
	uint32_t normal_tangent_stride = 0;
	uint32_t attribute_stride = 0;
	uint32_t skin_stride = 0;

	r_position_stride = 0;

	for (int i = 0; i < RSE::ARRAY_INDEX; i++) {
		RDC::VertexAttribute vd;
		vd.location = i;

		if (!(p_surface_format & (1ULL << i))) {
			vd.stride = 0;
			switch (i) {
			case RSE::ARRAY_VERTEX:
			case RSE::ARRAY_NORMAL:
				vd.format = RDC::DATA_FORMAT_R32G32B32_SFLOAT;
				break;
			case RSE::ARRAY_TEX_UV:
			case RSE::ARRAY_TEX_UV2:
				vd.format = RDC::DATA_FORMAT_R32G32_SFLOAT;
				break;
			case RSE::ARRAY_BONES:
				vd.format = RDC::DATA_FORMAT_R32G32B32A32_UINT;
				break;
			case RSE::ARRAY_TANGENT:
			case RSE::ARRAY_COLOR:
			case RSE::ARRAY_CUSTOM0:
			case RSE::ARRAY_CUSTOM1:
			case RSE::ARRAY_CUSTOM2:
			case RSE::ARRAY_CUSTOM3:
			case RSE::ARRAY_WEIGHTS:
				vd.format = RDC::DATA_FORMAT_R32G32B32A32_SFLOAT;
				break;
			default:
				DEV_ASSERT(false && "Unknown vertex format element.");
				break;
			}
		}
		else {
			// Mark that it needs a stride set (default uses 0).
			vd.stride = 1;

			switch (i) {
			case RSE::ARRAY_VERTEX: {
				vd.offset = r_position_stride;

				if (p_surface_format & RSE::ARRAY_FLAG_USE_2D_VERTICES) {
					vd.format = RDC::DATA_FORMAT_R32G32_SFLOAT;
					r_position_stride = sizeof(float) * 2;
				}
				else {
					if (!p_instanced_surface &&
						(p_surface_format & RSE::ARRAY_FLAG_COMPRESS_ATTRIBUTES)) {
						vd.format = RDC::DATA_FORMAT_R16G16B16A16_UNORM;
						r_position_stride = sizeof(uint16_t) * 4;
					}
					else {
						vd.format = RDC::DATA_FORMAT_R32G32B32_SFLOAT;
						r_position_stride = sizeof(float) * 3;
					}
				}

			} break;
			case RSE::ARRAY_NORMAL: {
				vd.offset = 0;

				if (!p_instanced_surface &&
					(p_surface_format & RSE::ARRAY_FLAG_COMPRESS_ATTRIBUTES)) {
					vd.format = RDC::DATA_FORMAT_R16G16_UNORM;
					normal_tangent_stride += sizeof(uint16_t) * 2;
				}
				else {
					vd.format = RDC::DATA_FORMAT_R16G16B16A16_UNORM;
					// A small trick here: if we are uncompressed and we have normals, but no
					// tangents. We need the shader to think there are 4 components to
					// "axis_tangent_attrib". So we give a size of 4, but a stride based on only
					// having 2 elements.
					if (!(p_surface_format & RSE::ARRAY_FORMAT_TANGENT)) {
						normal_tangent_stride += sizeof(uint16_t) * 2;
					}
					else {
						normal_tangent_stride += sizeof(uint16_t) * 4;
					}
				}
			} break;
			case RSE::ARRAY_TANGENT: {
				vd.stride = 0;
				vd.format = RDC::DATA_FORMAT_R32G32B32A32_SFLOAT;
			} break;
			case RSE::ARRAY_COLOR: {
				vd.offset = attribute_stride;

				vd.format = RDC::DATA_FORMAT_R8G8B8A8_UNORM;
				attribute_stride += sizeof(int8_t) * 4;
			} break;
			case RSE::ARRAY_TEX_UV: {
				vd.offset = attribute_stride;
				if (p_surface_format & RSE::ARRAY_FLAG_COMPRESS_ATTRIBUTES) {
					vd.format = RDC::DATA_FORMAT_R16G16_UNORM;
					attribute_stride += sizeof(uint16_t) * 2;
				}
				else {
					vd.format = RDC::DATA_FORMAT_R32G32_SFLOAT;
					attribute_stride += sizeof(float) * 2;
				}

			} break;
			case RSE::ARRAY_TEX_UV2: {
				vd.offset = attribute_stride;
				if (p_surface_format & RSE::ARRAY_FLAG_COMPRESS_ATTRIBUTES) {
					vd.format = RDC::DATA_FORMAT_R16G16_UNORM;
					attribute_stride += sizeof(uint16_t) * 2;
				}
				else {
					vd.format = RDC::DATA_FORMAT_R32G32_SFLOAT;
					attribute_stride += sizeof(float) * 2;
				}
			} break;
			case RSE::ARRAY_CUSTOM0:
			case RSE::ARRAY_CUSTOM1:
			case RSE::ARRAY_CUSTOM2:
			case RSE::ARRAY_CUSTOM3: {
				vd.offset = attribute_stride;

				int idx = i - RSE::ARRAY_CUSTOM0;
				const uint32_t fmt_shift[RSE::ARRAY_CUSTOM_COUNT] = {
					RSE::ARRAY_FORMAT_CUSTOM0_SHIFT, RSE::ARRAY_FORMAT_CUSTOM1_SHIFT,
					RSE::ARRAY_FORMAT_CUSTOM2_SHIFT, RSE::ARRAY_FORMAT_CUSTOM3_SHIFT};
				uint32_t fmt = (p_surface_format >> fmt_shift[idx]) & RSE::ARRAY_FORMAT_CUSTOM_MASK;
				const uint32_t fmtsize[RSE::ARRAY_CUSTOM_MAX] = {4, 4, 4, 8, 4, 8, 12, 16};
				const RDC::DataFormat fmtrd[RSE::ARRAY_CUSTOM_MAX] = {RDC::DATA_FORMAT_R8G8B8A8_UNORM,
					RDC::DATA_FORMAT_R8G8B8A8_SNORM, RDC::DATA_FORMAT_R16G16_SFLOAT,
					RDC::DATA_FORMAT_R16G16B16A16_SFLOAT, RDC::DATA_FORMAT_R32_SFLOAT,
					RDC::DATA_FORMAT_R32G32_SFLOAT, RDC::DATA_FORMAT_R32G32B32_SFLOAT,
					RDC::DATA_FORMAT_R32G32B32A32_SFLOAT};
				vd.format = fmtrd[fmt];
				attribute_stride += fmtsize[fmt];
			} break;
			case RSE::ARRAY_BONES: {
				vd.offset = skin_stride;

				vd.format = RDC::DATA_FORMAT_R16G16B16A16_UINT;
				skin_stride += sizeof(int16_t) * 4;
			} break;
			case RSE::ARRAY_WEIGHTS: {
				vd.offset = skin_stride;

				vd.format = RDC::DATA_FORMAT_R16G16B16A16_UNORM;
				skin_stride += sizeof(int16_t) * 4;
			} break;
			}

			if (p_point_size_emulated) {
				vd.frequency = RDC::VERTEX_FREQUENCY_INSTANCE;
			}
		}

		if (!(p_input_mask & (1ULL << i))) {
			continue; // Shader does not need this, skip it (but computing stride was important
					  // anyway)
		}

		attributes.push_back(vd);

		if (p_input_motion_vectors) {
			// Since the previous vertex, normal and tangent can't be part of the vertex format but
			// they are required when motion vectors are enabled, we opt to push a copy of the
			// vertex attribute with a different location.
			switch (i) {
			case RSE::ARRAY_VERTEX: {
				vd.location = ATTRIBUTE_LOCATION_PREV_VERTEX;
			} break;
			case RSE::ARRAY_NORMAL: {
				vd.location = ATTRIBUTE_LOCATION_PREV_NORMAL;
			} break;
			case RSE::ARRAY_TANGENT: {
				vd.location = ATTRIBUTE_LOCATION_PREV_TANGENT;
			} break;
			}

			if (int(vd.location) != i) {
				attributes.push_back(vd);
			}
		}
	}

	// Update final stride.
	for (int i = 0; i < attributes.size(); i++) {
		if (attributes[i].stride == 0) {
			// Default location.
			continue;
		}

		int loc = attributes[i].location;
		if (loc == RSE::ARRAY_VERTEX || loc == ATTRIBUTE_LOCATION_PREV_VERTEX) {
			attributes.write[i].stride = r_position_stride;
		}
		else if ((loc < RSE::ARRAY_COLOR) || ((loc >= ATTRIBUTE_LOCATION_PREV_NORMAL) &&
												   (loc <= ATTRIBUTE_LOCATION_PREV_TANGENT))) {
			attributes.write[i].stride = normal_tangent_stride;
		}
		else if (loc < RSE::ARRAY_BONES) {
			attributes.write[i].stride = attribute_stride;
		}
		else {
			attributes.write[i].stride = skin_stride;
		}
	}

	return RD::vertex_format_create(attributes);
}

void MeshStorage::_mesh_surface_generate_version_for_input_mask(Mesh::Surface::Version& v,
	Mesh::Surface* s, uint64_t p_input_mask, bool p_input_motion_vectors,
	bool p_point_size_emulated, MeshInstance::Surface* mis, uint32_t p_current_buffer,
	uint32_t p_previous_buffer)
{
	uint32_t position_stride = 0;
	v.vertex_format = _mesh_surface_generate_vertex_format(s->format, p_input_mask, mis != nullptr,
		p_input_motion_vectors, p_point_size_emulated, position_stride);

	Vector<RID> buffers;
	Vector<uint64_t> offsets;
	RID buffer;
	uint64_t offset = 0;
	for (int i = 0; i < RSE::ARRAY_INDEX; i++) {
		offset = 0;

		if (!(s->format & (1ULL << i))) {
			// Not supplied by surface, use default buffers.
			buffer = mesh_default_rd_buffers[i];
		}
		else {
			// Supplied by surface, use buffer.
			switch (i) {
			case RSE::ARRAY_VERTEX:
			case RSE::ARRAY_NORMAL:
				offset = i == RSE::ARRAY_NORMAL ? position_stride * s->vertex_count : 0;
				buffer = mis != nullptr ? mis->vertex_buffer[p_current_buffer] : s->vertex_buffer;
				break;
			case RSE::ARRAY_TANGENT:
				buffer = mesh_default_rd_buffers[i];
				break;
			case RSE::ARRAY_COLOR:
			case RSE::ARRAY_TEX_UV:
			case RSE::ARRAY_TEX_UV2:
			case RSE::ARRAY_CUSTOM0:
			case RSE::ARRAY_CUSTOM1:
			case RSE::ARRAY_CUSTOM2:
			case RSE::ARRAY_CUSTOM3:
				buffer = s->attribute_buffer;
				break;
			case RSE::ARRAY_BONES:
			case RSE::ARRAY_WEIGHTS:
				buffer = s->skin_buffer;
				break;
			}
		}

		if (!(p_input_mask & (1ULL << i))) {
			continue; // Shader does not need this, skip it (but computing stride was important
					  // anyway)
		}

		buffers.push_back(buffer);
		offsets.push_back(offset);

		if (p_input_motion_vectors) {
			// Push the buffer for motion vector inputs.
			if (i == RSE::ARRAY_VERTEX || i == RSE::ARRAY_NORMAL || i == RSE::ARRAY_TANGENT) {
				if (mis && buffer != mesh_default_rd_buffers[i]) {
					buffers.push_back(mis->vertex_buffer[p_previous_buffer]);
				}
				else {
					buffers.push_back(buffer);
				}

				offsets.push_back(offset);
			}
		}
	}

	v.input_mask = p_input_mask;
	v.current_buffer = p_current_buffer;
	v.previous_buffer = p_previous_buffer;
	v.input_motion_vectors = p_input_motion_vectors;
	v.point_size_emulated = p_point_size_emulated;
	v.vertex_array = RD::vertex_array_create(
		p_point_size_emulated ? 0 : s->vertex_count, v.vertex_format, buffers, offsets);
}

RID MeshStorage::_multimesh_allocate() { return multimesh_owner.allocate_rid(); }

void MeshStorage::_multimesh_initialize(RID p_rid)
{
	multimesh_owner.initialize_rid(p_rid, MultiMesh());
}

void MeshStorage::_multimesh_free(RID p_rid)
{
	// Remove from interpolator.
	RendererMeshStorage::_interpolation_data.notify_free_multimesh(p_rid);
	_update_dirty_multimeshes();
	_multimesh_allocate_data(p_rid, 0, RSE::MULTIMESH_TRANSFORM_2D);
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_rid);
	multimesh->dependency.deleted_notify(p_rid);
	multimesh_owner.free(p_rid);
}

void MeshStorage::_multimesh_allocate_data(RID p_multimesh, int p_instances,
	RSE::MultimeshTransformFormat p_transform_format, bool p_use_colors, bool p_use_custom_data,
	bool p_use_indirect)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL(multimesh);

	if (multimesh->instances == p_instances && multimesh->xform_format == p_transform_format &&
		multimesh->uses_colors == p_use_colors &&
		multimesh->uses_custom_data == p_use_custom_data) {
		return;
	}

	if (multimesh->buffer.is_valid()) {
		RD::free_rid(multimesh->buffer);
		multimesh->buffer = RID();
		multimesh->uniform_set_2d = RID(); // cleared by dependency
		multimesh->uniform_set_3d = RID(); // cleared by dependency
	}

	if (multimesh->data_cache_dirty_regions) {
		memdelete_arr(multimesh->data_cache_dirty_regions);
		multimesh->data_cache_dirty_regions = nullptr;
		multimesh->data_cache_dirty_region_count = 0;
	}

	if (multimesh->previous_data_cache_dirty_regions) {
		memdelete_arr(multimesh->previous_data_cache_dirty_regions);
		multimesh->previous_data_cache_dirty_regions = nullptr;
		multimesh->previous_data_cache_dirty_region_count = 0;
	}

	multimesh->instances = p_instances;
	multimesh->xform_format = p_transform_format;
	multimesh->uses_colors = p_use_colors;
	multimesh->color_offset_cache = p_transform_format == RSE::MULTIMESH_TRANSFORM_2D ? 8 : 12;
	multimesh->uses_custom_data = p_use_custom_data;
	multimesh->custom_data_offset_cache = multimesh->color_offset_cache + (p_use_colors ? 4 : 0);
	multimesh->stride_cache = multimesh->custom_data_offset_cache + (p_use_custom_data ? 4 : 0);
	multimesh->buffer_set = false;

	multimesh->indirect = p_use_indirect;
	multimesh->command_buffer = RID();

	// print_line("allocate, elements: " + itos(p_instances) + " 2D: " + itos(p_transform_format ==
	// RSE::MULTIMESH_TRANSFORM_2D) + " colors " + itos(multimesh->uses_colors) + " data " +
	// itos(multimesh->uses_custom_data) + " stride " + itos(multimesh->stride_cache) + " total size
	// " + itos(multimesh->stride_cache * multimesh->instances));
	multimesh->data_cache = Vector<float>();
	multimesh->aabb = AABB();
	multimesh->aabb_dirty = false;
	multimesh->visible_instances = MIN(multimesh->visible_instances, multimesh->instances);
	multimesh->motion_vectors_current_offset = 0;
	multimesh->motion_vectors_previous_offset = 0;
	multimesh->motion_vectors_last_change = -1;
	multimesh->motion_vectors_enabled = false;

	if (multimesh->instances) {
		uint32_t buffer_size = multimesh->instances * multimesh->stride_cache * sizeof(float);
		LocalVector<uint8_t> zeros;
		zeros.resize_initialized(buffer_size);
		multimesh->buffer = RD::storage_buffer_create(buffer_size, zeros.span());
	}

	multimesh->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MULTIMESH);
}

void MeshStorage::_multimesh_get_motion_vectors_offsets(
	RID p_multimesh, uint32_t& r_current_offset, uint32_t& r_prev_offset)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL(multimesh);
	r_current_offset = multimesh->motion_vectors_current_offset;
	if (!_multimesh_uses_motion_vectors(multimesh)) {
		multimesh->motion_vectors_previous_offset = multimesh->motion_vectors_current_offset;
	}
	r_prev_offset = multimesh->motion_vectors_previous_offset;
}

bool MeshStorage::_multimesh_uses_motion_vectors_offsets(RID p_multimesh)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL_V(multimesh, false);
	return _multimesh_uses_motion_vectors(multimesh);
}

int MeshStorage::_multimesh_get_instance_count(RID p_multimesh)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL_V(multimesh, 0);
	return multimesh->instances;
}

void MeshStorage::_multimesh_set_mesh(RID p_multimesh, RID p_mesh)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL(multimesh);
	if (multimesh->mesh == p_mesh) {
		return;
	}
	multimesh->mesh = p_mesh;

	if (multimesh->indirect) {
		Mesh* mesh = mesh_owner.get_or_null(p_mesh);
		ERR_FAIL_NULL(mesh);
		if (mesh->surface_count > 0) {
			if (multimesh->command_buffer.is_valid()) {
				RD::free_rid(multimesh->command_buffer);
			}

			Vector<uint8_t> newVector;
			newVector.resize_initialized(
				sizeof(uint32_t) * INDIRECT_MULTIMESH_COMMAND_STRIDE * mesh->surface_count);

			for (uint32_t i = 0; i < mesh->surface_count; i++) {
				uint32_t count = mesh_surface_get_vertices_drawn_count(mesh->surfaces[i]);
				newVector.set(i * sizeof(uint32_t) * INDIRECT_MULTIMESH_COMMAND_STRIDE,
					static_cast<uint8_t>(count));
				newVector.set(i * sizeof(uint32_t) * INDIRECT_MULTIMESH_COMMAND_STRIDE + 1,
					static_cast<uint8_t>(count >> 8));
				newVector.set(i * sizeof(uint32_t) * INDIRECT_MULTIMESH_COMMAND_STRIDE + 2,
					static_cast<uint8_t>(count >> 16));
				newVector.set(i * sizeof(uint32_t) * INDIRECT_MULTIMESH_COMMAND_STRIDE + 3,
					static_cast<uint8_t>(count >> 24));
			}

			RID newBuffer = RD::storage_buffer_create(
				sizeof(uint32_t) * INDIRECT_MULTIMESH_COMMAND_STRIDE * mesh->surface_count,
				newVector, RD::STORAGE_BUFFER_USAGE_DISPATCH_INDIRECT);
			multimesh->command_buffer = newBuffer;
		}
	}

	if (multimesh->instances == 0) {
		return;
	}

	if (multimesh->data_cache.size()) {
		// we have a data cache, just mark it dirt
		_multimesh_mark_all_dirty(multimesh, false, true);
	}
	else if (multimesh->instances) {
		// need to re-create AABB unfortunately, calling this has a penalty
		if (multimesh->buffer_set) {
			Vector<uint8_t> buffer = RD::buffer_get_data(multimesh->buffer);
			const uint8_t* r = buffer.ptr() + multimesh->motion_vectors_current_offset *
												  multimesh->stride_cache * sizeof(float);
			const float* data = reinterpret_cast<const float*>(r);
			_multimesh_re_create_aabb(multimesh, data, multimesh->instances);
		}
	}

	multimesh->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MESH);
}

#define MULTIMESH_DIRTY_REGION_SIZE 512

void MeshStorage::_multimesh_make_local(MultiMesh* multimesh)
{
	if (multimesh->data_cache.size() > 0) {
		return; // already local
	}

	// this means that the user wants to load/save individual elements,
	// for this, the data must reside on CPU, so just copy it there.
	uint32_t buffer_size = multimesh->instances * multimesh->stride_cache;
	if (multimesh->motion_vectors_enabled) {
		buffer_size *= 2;
	}
	multimesh->data_cache.resize(buffer_size);
	{
		float* w = multimesh->data_cache.ptrw();

		if (multimesh->buffer_set) {
			Vector<uint8_t> buffer = RD::buffer_get_data(multimesh->buffer);
			{
				const uint8_t* r = buffer.ptr();
				memcpy(w, r, buffer.size());
			}
		}
		else {
			memset(w, 0, buffer_size * sizeof(float));
		}
	}
	uint32_t data_cache_dirty_region_count =
		Math::division_round_up(multimesh->instances, MULTIMESH_DIRTY_REGION_SIZE);
	multimesh->data_cache_dirty_regions = memnew_arr(bool, data_cache_dirty_region_count);
	memset(multimesh->data_cache_dirty_regions, 0, data_cache_dirty_region_count * sizeof(bool));
	multimesh->data_cache_dirty_region_count = 0;

	multimesh->previous_data_cache_dirty_regions = memnew_arr(bool, data_cache_dirty_region_count);
	memset(multimesh->previous_data_cache_dirty_regions, 0,
		data_cache_dirty_region_count * sizeof(bool));
	multimesh->previous_data_cache_dirty_region_count = 0;
}

void MeshStorage::_multimesh_update_motion_vectors_data_cache(MultiMesh* multimesh)
{
	ERR_FAIL_COND(multimesh->data_cache.is_empty());

	if (!multimesh->motion_vectors_enabled) {
		return;
	}

	uint32_t frame = RendererCompositor::get_frame_number();
	if (multimesh->motion_vectors_last_change != frame) {
		multimesh->motion_vectors_previous_offset = multimesh->motion_vectors_current_offset;
		multimesh->motion_vectors_current_offset =
			multimesh->instances - multimesh->motion_vectors_current_offset;
		multimesh->motion_vectors_last_change = frame;

		if (multimesh->previous_data_cache_dirty_region_count > 0) {
			uint8_t* data = (uint8_t*)multimesh->data_cache.ptrw();
			uint32_t current_ofs =
				multimesh->motion_vectors_current_offset * multimesh->stride_cache * sizeof(float);
			uint32_t previous_ofs =
				multimesh->motion_vectors_previous_offset * multimesh->stride_cache * sizeof(float);
			uint32_t visible_instances = multimesh->visible_instances >= 0
											 ? multimesh->visible_instances
											 : multimesh->instances;
			uint32_t visible_region_count = visible_instances == 0
												? 0
												: Math::division_round_up(visible_instances,
													  (uint32_t)MULTIMESH_DIRTY_REGION_SIZE);
			uint32_t region_size =
				multimesh->stride_cache * MULTIMESH_DIRTY_REGION_SIZE * sizeof(float);
			uint32_t size =
				multimesh->stride_cache * (uint32_t)multimesh->instances * (uint32_t)sizeof(float);
			for (uint32_t i = 0; i < visible_region_count; i++) {
				if (multimesh->previous_data_cache_dirty_regions[i]) {
					uint32_t offset = i * region_size;
					memcpy(data + current_ofs + offset, data + previous_ofs + offset,
						MIN(region_size, size - offset));
				}
			}
		}
	}
}

bool MeshStorage::_multimesh_uses_motion_vectors(MultiMesh* multimesh)
{
	return (RendererCompositor::get_frame_number() - multimesh->motion_vectors_last_change) < 2;
}

void MeshStorage::_multimesh_mark_dirty(MultiMesh* multimesh, int p_index, bool p_aabb)
{
	uint32_t region_index = p_index / MULTIMESH_DIRTY_REGION_SIZE;
#ifdef DEBUG_ENABLED
	uint32_t data_cache_dirty_region_count =
		Math::division_round_up(multimesh->instances, MULTIMESH_DIRTY_REGION_SIZE);
	ERR_FAIL_UNSIGNED_INDEX(region_index, data_cache_dirty_region_count); // bug
#endif
	if (!multimesh->data_cache_dirty_regions[region_index]) {
		multimesh->data_cache_dirty_regions[region_index] = true;
		multimesh->data_cache_dirty_region_count++;
	}

	if (p_aabb) {
		multimesh->aabb_dirty = true;
	}

	if (!multimesh->dirty) {
		multimesh->dirty_list = multimesh_dirty_list;
		multimesh_dirty_list = multimesh;
		multimesh->dirty = true;
	}
}

void MeshStorage::_multimesh_mark_all_dirty(MultiMesh* multimesh, bool p_data, bool p_aabb)
{
	if (p_data) {
		uint32_t data_cache_dirty_region_count =
			Math::division_round_up(multimesh->instances, MULTIMESH_DIRTY_REGION_SIZE);

		for (uint32_t i = 0; i < data_cache_dirty_region_count; i++) {
			if (!multimesh->data_cache_dirty_regions[i]) {
				multimesh->data_cache_dirty_regions[i] = true;
				multimesh->data_cache_dirty_region_count++;
			}
		}
	}

	if (p_aabb) {
		multimesh->aabb_dirty = true;
	}

	if (!multimesh->dirty) {
		multimesh->dirty_list = multimesh_dirty_list;
		multimesh_dirty_list = multimesh;
		multimesh->dirty = true;
	}
}

void MeshStorage::_multimesh_re_create_aabb(
	MultiMesh* multimesh, const float* p_data, int p_instances)
{
	ERR_FAIL_COND(multimesh->mesh.is_null());
	if (multimesh->custom_aabb != AABB()) {
		return;
	}
	AABB aabb;
	AABB mesh_aabb = mesh_get_aabb(multimesh->mesh);
	for (int i = 0; i < p_instances; i++) {
		const float* data = p_data + multimesh->stride_cache * i;
		Transform3D t;

		if (multimesh->xform_format == RSE::MULTIMESH_TRANSFORM_3D) {
			t.basis.rows[0][0] = data[0];
			t.basis.rows[0][1] = data[1];
			t.basis.rows[0][2] = data[2];
			t.origin.x = data[3];
			t.basis.rows[1][0] = data[4];
			t.basis.rows[1][1] = data[5];
			t.basis.rows[1][2] = data[6];
			t.origin.y = data[7];
			t.basis.rows[2][0] = data[8];
			t.basis.rows[2][1] = data[9];
			t.basis.rows[2][2] = data[10];
			t.origin.z = data[11];

		}
		else {
			t.basis.rows[0][0] = data[0];
			t.basis.rows[0][1] = data[1];
			t.origin.x = data[3];

			t.basis.rows[1][0] = data[4];
			t.basis.rows[1][1] = data[5];
			t.origin.y = data[7];
		}

		if (i == 0) {
			aabb = t.xform(mesh_aabb);
		}
		else {
			aabb.merge_with(t.xform(mesh_aabb));
		}
	}

	multimesh->aabb = aabb;
}

void MeshStorage::_multimesh_instance_set_transform(
	RID p_multimesh, int p_index, const Transform3D& p_transform)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL(multimesh);
	ERR_FAIL_INDEX(p_index, multimesh->instances);
	ERR_FAIL_COND(multimesh->xform_format != RSE::MULTIMESH_TRANSFORM_3D);

	_multimesh_make_local(multimesh);

	bool uses_motion_vectors = (Renderer::viewport->get_num_viewports_with_motion_vectors() > 0) ||
							   (RendererCompositorStorage::get_num_compositor_effects_with_motion_vectors() > 0);

	_multimesh_update_motion_vectors_data_cache(multimesh);

	{
		float* w = multimesh->data_cache.ptrw();

		float* dataptr =
			w + (multimesh->motion_vectors_current_offset + p_index) * multimesh->stride_cache;

		dataptr[0] = p_transform.basis.rows[0][0];
		dataptr[1] = p_transform.basis.rows[0][1];
		dataptr[2] = p_transform.basis.rows[0][2];
		dataptr[3] = p_transform.origin.x;
		dataptr[4] = p_transform.basis.rows[1][0];
		dataptr[5] = p_transform.basis.rows[1][1];
		dataptr[6] = p_transform.basis.rows[1][2];
		dataptr[7] = p_transform.origin.y;
		dataptr[8] = p_transform.basis.rows[2][0];
		dataptr[9] = p_transform.basis.rows[2][1];
		dataptr[10] = p_transform.basis.rows[2][2];
		dataptr[11] = p_transform.origin.z;
	}

	_multimesh_mark_dirty(multimesh, p_index, true);
}

void MeshStorage::_multimesh_instance_set_transform_2d(
	RID p_multimesh, int p_index, const Transform2D& p_transform)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL(multimesh);
	ERR_FAIL_INDEX(p_index, multimesh->instances);
	ERR_FAIL_COND(multimesh->xform_format != RSE::MULTIMESH_TRANSFORM_2D);

	_multimesh_make_local(multimesh);
	_multimesh_update_motion_vectors_data_cache(multimesh);

	{
		float* w = multimesh->data_cache.ptrw();

		float* dataptr =
			w + (multimesh->motion_vectors_current_offset + p_index) * multimesh->stride_cache;

		dataptr[0] = p_transform.columns[0][0];
		dataptr[1] = p_transform.columns[1][0];
		dataptr[2] = 0;
		dataptr[3] = p_transform.columns[2][0];
		dataptr[4] = p_transform.columns[0][1];
		dataptr[5] = p_transform.columns[1][1];
		dataptr[6] = 0;
		dataptr[7] = p_transform.columns[2][1];
	}

	_multimesh_mark_dirty(multimesh, p_index, true);
}

void MeshStorage::_multimesh_instance_set_color(RID p_multimesh, int p_index, const Color& p_color)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL(multimesh);
	ERR_FAIL_INDEX(p_index, multimesh->instances);
	ERR_FAIL_COND(!multimesh->uses_colors);

	_multimesh_make_local(multimesh);
	_multimesh_update_motion_vectors_data_cache(multimesh);

	{
		float* w = multimesh->data_cache.ptrw();

		float* dataptr =
			w + (multimesh->motion_vectors_current_offset + p_index) * multimesh->stride_cache +
			multimesh->color_offset_cache;

		dataptr[0] = p_color.r;
		dataptr[1] = p_color.g;
		dataptr[2] = p_color.b;
		dataptr[3] = p_color.a;
	}

	_multimesh_mark_dirty(multimesh, p_index, false);
}

void MeshStorage::_multimesh_instance_set_custom_data(
	RID p_multimesh, int p_index, const Color& p_color)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL(multimesh);
	ERR_FAIL_INDEX(p_index, multimesh->instances);
	ERR_FAIL_COND(!multimesh->uses_custom_data);

	_multimesh_make_local(multimesh);
	_multimesh_update_motion_vectors_data_cache(multimesh);

	{
		float* w = multimesh->data_cache.ptrw();

		float* dataptr =
			w + (multimesh->motion_vectors_current_offset + p_index) * multimesh->stride_cache +
			multimesh->custom_data_offset_cache;

		dataptr[0] = p_color.r;
		dataptr[1] = p_color.g;
		dataptr[2] = p_color.b;
		dataptr[3] = p_color.a;
	}

	_multimesh_mark_dirty(multimesh, p_index, false);
}

RID MeshStorage::_multimesh_get_mesh(RID p_multimesh)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL_V(multimesh, RID());

	return multimesh->mesh;
}

Dependency* MeshStorage::multimesh_get_dependency(RID p_multimesh)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL_V(multimesh, nullptr);

	return &multimesh->dependency;
}

Transform3D MeshStorage::_multimesh_instance_get_transform(RID p_multimesh, int p_index)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL_V(multimesh, Transform3D());
	ERR_FAIL_INDEX_V(p_index, multimesh->instances, Transform3D());
	ERR_FAIL_COND_V(multimesh->xform_format != RSE::MULTIMESH_TRANSFORM_3D, Transform3D());

	_multimesh_make_local(multimesh);

	Transform3D t;
	{
		const float* r = multimesh->data_cache.ptr();

		const float* dataptr =
			r + (multimesh->motion_vectors_current_offset + p_index) * multimesh->stride_cache;

		t.basis.rows[0][0] = dataptr[0];
		t.basis.rows[0][1] = dataptr[1];
		t.basis.rows[0][2] = dataptr[2];
		t.origin.x = dataptr[3];
		t.basis.rows[1][0] = dataptr[4];
		t.basis.rows[1][1] = dataptr[5];
		t.basis.rows[1][2] = dataptr[6];
		t.origin.y = dataptr[7];
		t.basis.rows[2][0] = dataptr[8];
		t.basis.rows[2][1] = dataptr[9];
		t.basis.rows[2][2] = dataptr[10];
		t.origin.z = dataptr[11];
	}

	return t;
}

Transform2D MeshStorage::_multimesh_instance_get_transform_2d(RID p_multimesh, int p_index)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL_V(multimesh, Transform2D());
	ERR_FAIL_INDEX_V(p_index, multimesh->instances, Transform2D());
	ERR_FAIL_COND_V(multimesh->xform_format != RSE::MULTIMESH_TRANSFORM_2D, Transform2D());

	_multimesh_make_local(multimesh);

	Transform2D t;
	{
		const float* r = multimesh->data_cache.ptr();

		const float* dataptr =
			r + (multimesh->motion_vectors_current_offset + p_index) * multimesh->stride_cache;

		t.columns[0][0] = dataptr[0];
		t.columns[1][0] = dataptr[1];
		t.columns[2][0] = dataptr[3];
		t.columns[0][1] = dataptr[4];
		t.columns[1][1] = dataptr[5];
		t.columns[2][1] = dataptr[7];
	}

	return t;
}

Color MeshStorage::_multimesh_instance_get_color(RID p_multimesh, int p_index)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL_V(multimesh, Color());
	ERR_FAIL_INDEX_V(p_index, multimesh->instances, Color());
	ERR_FAIL_COND_V(!multimesh->uses_colors, Color());

	_multimesh_make_local(multimesh);

	Color c;
	{
		const float* r = multimesh->data_cache.ptr();

		const float* dataptr =
			r + (multimesh->motion_vectors_current_offset + p_index) * multimesh->stride_cache +
			multimesh->color_offset_cache;

		c.r = dataptr[0];
		c.g = dataptr[1];
		c.b = dataptr[2];
		c.a = dataptr[3];
	}

	return c;
}

Color MeshStorage::_multimesh_instance_get_custom_data(RID p_multimesh, int p_index)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL_V(multimesh, Color());
	ERR_FAIL_INDEX_V(p_index, multimesh->instances, Color());
	ERR_FAIL_COND_V(!multimesh->uses_custom_data, Color());

	_multimesh_make_local(multimesh);

	Color c;
	{
		const float* r = multimesh->data_cache.ptr();

		const float* dataptr =
			r + (multimesh->motion_vectors_current_offset + p_index) * multimesh->stride_cache +
			multimesh->custom_data_offset_cache;

		c.r = dataptr[0];
		c.g = dataptr[1];
		c.b = dataptr[2];
		c.a = dataptr[3];
	}

	return c;
}

RID MeshStorage::_multimesh_get_command_buffer_rd_rid(RID p_multimesh)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL_V(multimesh, RID());
	return multimesh->command_buffer;
}

RID MeshStorage::_multimesh_get_buffer_rd_rid(RID p_multimesh)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL_V(multimesh, RID());
	return multimesh->buffer;
}

Vector<float> MeshStorage::_multimesh_get_buffer(RID p_multimesh)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL_V(multimesh, Vector<float>());
	if (multimesh->buffer.is_null()) {
		return Vector<float>();
	}
	else {
		Vector<float> ret;
		ret.resize(multimesh->instances * multimesh->stride_cache);
		float* w = ret.ptrw();

		if (multimesh->data_cache.size()) {
			const uint8_t* r =
				(uint8_t*)multimesh->data_cache.ptr() +
				multimesh->motion_vectors_current_offset * multimesh->stride_cache * sizeof(float);
			memcpy(w, r, ret.size() * sizeof(float));
		}
		else {
			Vector<uint8_t> buffer = RD::buffer_get_data(multimesh->buffer);
			const uint8_t* r = buffer.ptr() + multimesh->motion_vectors_current_offset *
												  multimesh->stride_cache * sizeof(float);
			memcpy(w, r, ret.size() * sizeof(float));
		}
		return ret;
	}
}

int MeshStorage::_multimesh_get_visible_instances(RID p_multimesh)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL_V(multimesh, 0);
	return multimesh->visible_instances;
}

void MeshStorage::_multimesh_set_custom_aabb(RID p_multimesh, const AABB& p_aabb)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL(multimesh);
	multimesh->custom_aabb = p_aabb;
	multimesh->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_AABB);
}

AABB MeshStorage::_multimesh_get_custom_aabb(RID p_multimesh)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL_V(multimesh, AABB());
	return multimesh->custom_aabb;
}

AABB MeshStorage::_multimesh_get_aabb(RID p_multimesh)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL_V(multimesh, AABB());
	if (multimesh->custom_aabb != AABB()) {
		return multimesh->custom_aabb;
	}

	if (multimesh->aabb_dirty) {
		_update_dirty_multimeshes();
	}
	return multimesh->aabb;
}

RendererMeshStorage::MultiMeshInterpolator* MeshStorage::_multimesh_get_interpolator(RID p_multimesh)
{
	MultiMesh* multimesh = multimesh_owner.get_or_null(p_multimesh);
	ERR_FAIL_NULL_V_MSG(multimesh, nullptr, "Multimesh not found: " + itos(p_multimesh.get_id()));

	return &multimesh->interpolator;
}

RID MeshStorage::skeleton_allocate() { return skeleton_owner.allocate_rid(); }

void MeshStorage::skeleton_initialize(RID p_rid)
{
	skeleton_owner.initialize_rid(p_rid, Skeleton());
}

void MeshStorage::skeleton_free(RID p_rid)
{
	_update_dirty_skeletons();
	skeleton_allocate_data(p_rid, 0);
	Skeleton* skeleton = skeleton_owner.get_or_null(p_rid);
	skeleton->dependency.deleted_notify(p_rid);
	skeleton_owner.free(p_rid);
}

void MeshStorage::_skeleton_make_dirty(Skeleton* skeleton)
{
	if (!skeleton->dirty) {
		skeleton->dirty = true;
		skeleton->dirty_list = skeleton_dirty_list;
		skeleton_dirty_list = skeleton;
	}
}

void MeshStorage::skeleton_allocate_data(RID p_skeleton, int p_bones, bool p_2d_skeleton)
{
	Skeleton* skeleton = skeleton_owner.get_or_null(p_skeleton);
	ERR_FAIL_NULL(skeleton);
	ERR_FAIL_COND(p_bones < 0);

	if (skeleton->size == p_bones && skeleton->use_2d == p_2d_skeleton) {
		return;
	}

	skeleton->size = p_bones;
	skeleton->use_2d = p_2d_skeleton;
	skeleton->uniform_set_3d = RID();

	if (skeleton->buffer.is_valid()) {
		RD::free_rid(skeleton->buffer);
		skeleton->buffer = RID();
		skeleton->data.clear();
		skeleton->uniform_set_mi = RID();
	}

	if (skeleton->size) {
		skeleton->data.resize(skeleton->size * (skeleton->use_2d ? 8 : 12));
		skeleton->buffer =
			RD::storage_buffer_create(skeleton->data.size() * sizeof(float));
		memset(skeleton->data.ptr(), 0, skeleton->data.size() * sizeof(float));

		_skeleton_make_dirty(skeleton);

		{
			Vector<RD::Uniform> uniforms;
			{
				RD::Uniform u;
				u.binding = 0;
				u.uniform_type = RDC::UNIFORM_TYPE_STORAGE_BUFFER;
				u.append_id(skeleton->buffer);
				uniforms.push_back(u);
			}
			skeleton->uniform_set_mi = RD::uniform_set_create(
				uniforms, skeleton_shader->version_shader[0], SkeletonShader::UNIFORM_SET_SKELETON);
		}
	}

	skeleton->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_SKELETON_DATA);
}

int MeshStorage::skeleton_get_bone_count(RID p_skeleton)
{
	Skeleton* skeleton = skeleton_owner.get_or_null(p_skeleton);
	ERR_FAIL_NULL_V(skeleton, 0);

	return skeleton->size;
}

void MeshStorage::skeleton_bone_set_transform(
	RID p_skeleton, int p_bone, const Transform3D& p_transform)
{
	Skeleton* skeleton = skeleton_owner.get_or_null(p_skeleton);

	ERR_FAIL_NULL(skeleton);
	ERR_FAIL_INDEX(p_bone, skeleton->size);
	ERR_FAIL_COND(skeleton->use_2d);

	float* dataptr = skeleton->data.ptr() + p_bone * 12;

	dataptr[0] = p_transform.basis.rows[0][0];
	dataptr[1] = p_transform.basis.rows[0][1];
	dataptr[2] = p_transform.basis.rows[0][2];
	dataptr[3] = p_transform.origin.x;
	dataptr[4] = p_transform.basis.rows[1][0];
	dataptr[5] = p_transform.basis.rows[1][1];
	dataptr[6] = p_transform.basis.rows[1][2];
	dataptr[7] = p_transform.origin.y;
	dataptr[8] = p_transform.basis.rows[2][0];
	dataptr[9] = p_transform.basis.rows[2][1];
	dataptr[10] = p_transform.basis.rows[2][2];
	dataptr[11] = p_transform.origin.z;

	_skeleton_make_dirty(skeleton);
}

Transform3D MeshStorage::skeleton_bone_get_transform(RID p_skeleton, int p_bone)
{
	Skeleton* skeleton = skeleton_owner.get_or_null(p_skeleton);

	ERR_FAIL_NULL_V(skeleton, Transform3D());
	ERR_FAIL_INDEX_V(p_bone, skeleton->size, Transform3D());
	ERR_FAIL_COND_V(skeleton->use_2d, Transform3D());

	const float* dataptr = skeleton->data.ptr() + p_bone * 12;

	Transform3D t;

	t.basis.rows[0][0] = dataptr[0];
	t.basis.rows[0][1] = dataptr[1];
	t.basis.rows[0][2] = dataptr[2];
	t.origin.x = dataptr[3];
	t.basis.rows[1][0] = dataptr[4];
	t.basis.rows[1][1] = dataptr[5];
	t.basis.rows[1][2] = dataptr[6];
	t.origin.y = dataptr[7];
	t.basis.rows[2][0] = dataptr[8];
	t.basis.rows[2][1] = dataptr[9];
	t.basis.rows[2][2] = dataptr[10];
	t.origin.z = dataptr[11];

	return t;
}

void MeshStorage::skeleton_bone_set_transform_2d(
	RID p_skeleton, int p_bone, const Transform2D& p_transform)

{
	Skeleton* skeleton = skeleton_owner.get_or_null(p_skeleton);

	ERR_FAIL_NULL(skeleton);
	ERR_FAIL_INDEX(p_bone, skeleton->size);
	ERR_FAIL_COND(!skeleton->use_2d);
	float* dataptr = skeleton->data.ptr() + p_bone * 8;

	dataptr[0] = p_transform.columns[0][0];
	dataptr[1] = p_transform.columns[1][0];
	dataptr[2] = 0;
	dataptr[3] = p_transform.columns[2][0];
	dataptr[4] = p_transform.columns[0][1];
	dataptr[5] = p_transform.columns[1][1];
	dataptr[6] = 0;
	dataptr[7] = p_transform.columns[2][1];

	_skeleton_make_dirty(skeleton);
}

Transform2D MeshStorage::skeleton_bone_get_transform_2d(RID p_skeleton, int p_bone)
{
	Skeleton* skeleton = skeleton_owner.get_or_null(p_skeleton);

	ERR_FAIL_NULL_V(skeleton, Transform2D());
	ERR_FAIL_INDEX_V(p_bone, skeleton->size, Transform2D());
	ERR_FAIL_COND_V(!skeleton->use_2d, Transform2D());

	const float* dataptr = skeleton->data.ptr() + p_bone * 8;

	Transform2D t;
	t.columns[0][0] = dataptr[0];
	t.columns[1][0] = dataptr[1];
	t.columns[2][0] = dataptr[3];
	t.columns[0][1] = dataptr[4];
	t.columns[1][1] = dataptr[5];
	t.columns[2][1] = dataptr[7];

	return t;
}

void MeshStorage::skeleton_set_base_transform_2d(
	RID p_skeleton, const Transform2D& p_base_transform)
{
	Skeleton* skeleton = skeleton_owner.get_or_null(p_skeleton);

	ERR_FAIL_NULL(skeleton);
	ERR_FAIL_COND(!skeleton->use_2d);

	skeleton->base_transform_2d = p_base_transform;
}

void MeshStorage::skeleton_update_dependency(RID p_skeleton, DependencyTracker* p_instance)
{
	Skeleton* skeleton = skeleton_owner.get_or_null(p_skeleton);
	ERR_FAIL_NULL(skeleton);

	p_instance->update_dependency(&skeleton->dependency);
}

void RendererRD::MeshStorage::_update_dirty_multimeshes() {}

void RendererRD::MeshStorage::_update_dirty_skeletons() {}

void RendererRD::MeshStorage::update_mesh_instances() {}

void RendererRD::MeshStorage::_multimesh_set_visible_instances(RID p_multimesh, int p_visible) {}

void RendererRD::MeshStorage::_multimesh_set_buffer(RID p_multimesh, const Vector<float>& p_buffer)
{
}

void RendererRD::MeshStorage::mesh_surface_update_vertex_region(
	RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data)
{
}

void RendererRD::MeshStorage::mesh_surface_update_skin_region(
	RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data)
{
}

void RendererRD::MeshStorage::mesh_surface_update_index_region(
	RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data)
{
}

void RendererRD::MeshStorage::mesh_surface_update_attribute_region(
	RID p_mesh, int p_surface, int p_offset, const Vector<uint8_t>& p_data)
{
}

void RendererRD::MeshStorage::initialize()
{
	default_rd_storage_buffer = RD::storage_buffer_create(sizeof(uint32_t) * 4);

	{
		Vector<uint8_t> buffer;
		{
			buffer.resize(sizeof(float) * 3);
			{
				uint8_t* w = buffer.ptrw();
				float* fptr = reinterpret_cast<float*>(w);
				fptr[0] = 0.0f;
				fptr[1] = 0.0f;
				fptr[2] = 0.0f;
			}
			mesh_default_rd_buffers[DEFAULT_RD_BUFFER_VERTEX] =
				RD::vertex_buffer_create(buffer.size(), buffer);
		}

		{
			buffer.resize(sizeof(float) * 3);
			{
				uint8_t* w = buffer.ptrw();
				float* fptr = reinterpret_cast<float*>(w);
				fptr[0] = 1.0f;
				fptr[1] = 0.0f;
				fptr[2] = 0.0f;
			}
			mesh_default_rd_buffers[DEFAULT_RD_BUFFER_NORMAL] =
				RD::vertex_buffer_create(buffer.size(), buffer);
		}

		{
			buffer.resize(sizeof(float) * 4);
			{
				uint8_t* w = buffer.ptrw();
				float* fptr = reinterpret_cast<float*>(w);
				fptr[0] = 1.0f;
				fptr[1] = 0.0f;
				fptr[2] = 0.0f;
				fptr[3] = 0.0f;
			}
			mesh_default_rd_buffers[DEFAULT_RD_BUFFER_TANGENT] =
				RD::vertex_buffer_create(buffer.size(), buffer);
		}

		{
			buffer.resize(sizeof(float) * 4);
			{
				uint8_t* w = buffer.ptrw();
				float* fptr = reinterpret_cast<float*>(w);
				fptr[0] = 1.0f;
				fptr[1] = 1.0f;
				fptr[2] = 1.0f;
				fptr[3] = 1.0f;
			}
			mesh_default_rd_buffers[DEFAULT_RD_BUFFER_COLOR] =
				RD::vertex_buffer_create(buffer.size(), buffer);
		}

		{
			buffer.resize(sizeof(float) * 2);
			{
				uint8_t* w = buffer.ptrw();
				float* fptr = reinterpret_cast<float*>(w);
				fptr[0] = 0.0f;
				fptr[1] = 0.0f;
			}
			mesh_default_rd_buffers[DEFAULT_RD_BUFFER_TEX_UV] =
				RD::vertex_buffer_create(buffer.size(), buffer);
		}

		{
			buffer.resize(sizeof(float) * 2);
			{
				uint8_t* w = buffer.ptrw();
				float* fptr = reinterpret_cast<float*>(w);
				fptr[0] = 0.0f;
				fptr[1] = 0.0f;
			}
			mesh_default_rd_buffers[DEFAULT_RD_BUFFER_TEX_UV2] =
				RD::vertex_buffer_create(buffer.size(), buffer);
		}

		for (int i = 0; i < RSE::ARRAY_CUSTOM_COUNT; i++) {
			buffer.resize(sizeof(float) * 4);
			{
				uint8_t* w = buffer.ptrw();
				float* fptr = reinterpret_cast<float*>(w);
				fptr[0] = 0.0f;
				fptr[1] = 0.0f;
				fptr[2] = 0.0f;
				fptr[3] = 0.0f;
			}
			mesh_default_rd_buffers[DEFAULT_RD_BUFFER_CUSTOM0 + i] =
				RD::vertex_buffer_create(buffer.size(), buffer);
		}

		{
			buffer.resize(sizeof(uint32_t) * 4);
			{
				uint8_t* w = buffer.ptrw();
				uint32_t* fptr = reinterpret_cast<uint32_t*>(w);
				fptr[0] = 0;
				fptr[1] = 0;
				fptr[2] = 0;
				fptr[3] = 0;
			}
			mesh_default_rd_buffers[DEFAULT_RD_BUFFER_BONES] =
				RD::vertex_buffer_create(buffer.size(), buffer);
		}

		{
			buffer.resize(sizeof(float) * 4);
			{
				uint8_t* w = buffer.ptrw();
				float* fptr = reinterpret_cast<float*>(w);
				fptr[0] = 0.0f;
				fptr[1] = 0.0f;
				fptr[2] = 0.0f;
				fptr[3] = 0.0f;
			}
			mesh_default_rd_buffers[DEFAULT_RD_BUFFER_WEIGHTS] =
				RD::vertex_buffer_create(buffer.size(), buffer);
		}
	}

	skeleton_shader = memnew(SkeletonShader);
	{
		Vector<String> skeleton_modes;
		skeleton_modes.push_back("\n#define MODE_2D\n");
		skeleton_modes.push_back("");

		skeleton_shader->shader.initialize(skeleton_modes);
		skeleton_shader->version = skeleton_shader->shader.version_create();
		for (int i = 0; i < SkeletonShader::SHADER_MODE_MAX; i++) {
			skeleton_shader->version_shader[i] =
				skeleton_shader->shader.version_get_shader(skeleton_shader->version, i);
			skeleton_shader->pipeline[i] =
				RD::compute_pipeline_create(skeleton_shader->version_shader[i]);
		}

		{
			Vector<RD::Uniform> uniforms;
			{
				RD::Uniform u;
				u.binding = 0;
				u.uniform_type = RDC::UNIFORM_TYPE_STORAGE_BUFFER;
				u.append_id(default_rd_storage_buffer);
				uniforms.push_back(u);
			}
			skeleton_shader->default_skeleton_uniform_set = RD::uniform_set_create(
				uniforms, skeleton_shader->version_shader[0], SkeletonShader::UNIFORM_SET_SKELETON);
		}
	}
}

void RendererRD::MeshStorage::finalize()
{
	for (int i = 0; i < DEFAULT_RD_BUFFER_MAX; i++) {
		if (mesh_default_rd_buffers[i].is_valid()) {
			RD::free_rid(mesh_default_rd_buffers[i]);
			mesh_default_rd_buffers[i] = RID();
		}
	}

	if (skeleton_shader) {
		if (skeleton_shader->version.is_valid()) {
			skeleton_shader->shader.version_free(skeleton_shader->version);
			skeleton_shader->version = RID();
		}
		memdelete(skeleton_shader);
		skeleton_shader = nullptr;
	}

	if (default_rd_storage_buffer.is_valid()) {
		RD::free_rid(default_rd_storage_buffer);
		default_rd_storage_buffer = RID();
	}
}

// ============================================================================
// Merged from servers/rendering/renderer_rd/storage_rd/particles_storage.cpp
// ============================================================================

#include "particles_storage.h"
#include "servers/rendering/renderer_rd/effects/copy_effects.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/texture_storage.h"
#include "servers/rendering/renderer_viewport.h"
#include "servers/rendering/renderer.h"

using namespace RendererRD;

void ParticlesStorage::finalize()
{
	for (int i = 0; i <= ParticlesShader::MAX_USERDATAS; i++) {
		for (int j = 0; j < ParticlesShader::COPY_MODE_MAX; j++) {
			particles_shader.copy_pipelines[i][j].free();
		}
	}

	if (particles_shader.copy_shader_version.is_valid()) {
		particles_shader.copy_shader.version_free(particles_shader.copy_shader_version);
		particles_shader.copy_shader_version = RID();
	}

	if (particles_shader.default_material.is_valid()) {
		MaterialStorage::material_free(particles_shader.default_material);
		particles_shader.default_material = RID();
	}
	if (particles_shader.default_shader.is_valid()) {
		MaterialStorage::shader_free(particles_shader.default_shader);
		particles_shader.default_shader = RID();
	}

	if (sort_effects) {
		memdelete(sort_effects);
		sort_effects = nullptr;
	}
}

bool ParticlesStorage::free(RID p_rid)
{
	if (owns_particles(p_rid)) {
		particles_free(p_rid);
		return true;
	}
	else if (owns_particles_collision(p_rid)) {
		particles_collision_free(p_rid);
		return true;
	}
	else if (owns_particles_collision_instance(p_rid)) {
		particles_collision_instance_free(p_rid);
		return true;
	}

	return false;
}

/* PARTICLES */

RID ParticlesStorage::particles_allocate() { return particles_owner.allocate_rid(); }

void ParticlesStorage::particles_initialize(RID p_rid) { particles_owner.initialize_rid(p_rid); }

void ParticlesStorage::particles_free(RID p_rid)
{
	Particles* particles = particles_owner.get_or_null(p_rid);

	particles->dependency.deleted_notify(p_rid);
	particles->update_list.remove_from_list();

	_particles_free_data(particles);
	particles_owner.free(p_rid);
}

void ParticlesStorage::particles_set_mode(RID p_particles, RSE::ParticlesMode p_mode)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);
	if (particles->mode == p_mode) {
		return;
	}

	_particles_free_data(particles);

	particles->mode = p_mode;
}

void ParticlesStorage::particles_set_emitting(RID p_particles, bool p_emitting)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	particles->emitting = p_emitting;
}

bool ParticlesStorage::particles_get_emitting(RID p_particles)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL_V(particles, false);

	return particles->emitting;
}

void ParticlesStorage::_particles_free_data(Particles* particles)
{
	if (particles->particle_buffer.is_valid()) {
		RD::free_rid(particles->particle_buffer);
		particles->particle_buffer = RID();
		RD::free_rid(particles->particle_instance_buffer);
		particles->particle_instance_buffer = RID();
	}

	particles->userdata_count = 0;

	if (particles->frame_params_buffer.is_valid()) {
		RD::free_rid(particles->frame_params_buffer);
		particles->frame_params_buffer = RID();
	}
	particles->particles_transforms_buffer_uniform_set = RID();

	if (RD::uniform_set_is_valid(particles->trail_bind_pose_uniform_set)) {
		RD::free_rid(particles->trail_bind_pose_uniform_set);
	}
	particles->trail_bind_pose_uniform_set = RID();

	if (particles->trail_bind_pose_buffer.is_valid()) {
		RD::free_rid(particles->trail_bind_pose_buffer);
		particles->trail_bind_pose_buffer = RID();
	}
	if (RD::uniform_set_is_valid(particles->collision_textures_uniform_set)) {
		RD::free_rid(particles->collision_textures_uniform_set);
	}
	particles->collision_textures_uniform_set = RID();

	if (particles->particles_sort_buffer.is_valid()) {
		RD::free_rid(particles->particles_sort_buffer);
		particles->particles_sort_buffer = RID();
		particles->particles_sort_uniform_set = RID();
	}

	if (particles->emission_buffer != nullptr) {
		particles->emission_buffer = nullptr;
		particles->emission_buffer_data.clear();
		RD::free_rid(particles->emission_storage_buffer);
		particles->emission_storage_buffer = RID();
	}

	if (particles->unused_emission_storage_buffer.is_valid()) {
		RD::free_rid(particles->unused_emission_storage_buffer);
		particles->unused_emission_storage_buffer = RID();
	}

	if (particles->unused_trail_storage_buffer.is_valid()) {
		RD::free_rid(particles->unused_trail_storage_buffer);
		particles->unused_trail_storage_buffer = RID();
	}

	if (RD::uniform_set_is_valid(particles->particles_material_uniform_set)) {
		// will need to be re-created
		RD::free_rid(particles->particles_material_uniform_set);
	}
	particles->particles_material_uniform_set = RID();
}

void ParticlesStorage::particles_set_amount(RID p_particles, int p_amount)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	if (particles->amount == p_amount) {
		return;
	}

	_particles_free_data(particles);

	particles->amount = p_amount;

	particles->prev_ticks = 0;
	particles->phase = 0;
	particles->prev_phase = 0;
	particles->clear = true;

	particles->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_PARTICLES);
}

void ParticlesStorage::particles_set_amount_ratio(RID p_particles, float p_amount_ratio)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	particles->amount_ratio = p_amount_ratio;
}

void ParticlesStorage::particles_set_lifetime(RID p_particles, double p_lifetime)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);
	particles->lifetime = p_lifetime;
}

void ParticlesStorage::particles_set_one_shot(RID p_particles, bool p_one_shot)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);
	particles->one_shot = p_one_shot;
}

void ParticlesStorage::particles_set_pre_process_time(RID p_particles, double p_time)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);
	particles->pre_process_time = p_time;
}

void ParticlesStorage::particles_request_process_time(
	RID p_particles, real_t p_request_process_time, real_t p_request_process_time_residual)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);
	particles->request_process_time = p_request_process_time;
	particles->request_process_time_residual = p_request_process_time_residual;
}

void ParticlesStorage::particles_set_explosiveness_ratio(RID p_particles, real_t p_ratio)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);
	particles->explosiveness = p_ratio;
}

void ParticlesStorage::particles_set_randomness_ratio(RID p_particles, real_t p_ratio)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);
	particles->randomness = p_ratio;
}

void ParticlesStorage::particles_set_custom_aabb(RID p_particles, const AABB& p_aabb)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);
	particles->custom_aabb = p_aabb;
	particles->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_AABB);
}

void ParticlesStorage::particles_set_speed_scale(RID p_particles, double p_scale)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	particles->speed_scale = p_scale;
}

void ParticlesStorage::particles_set_use_local_coordinates(RID p_particles, bool p_enable)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	particles->use_local_coords = p_enable;
	particles->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_PARTICLES);
}

void ParticlesStorage::particles_set_fixed_fps(RID p_particles, int p_fps)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	particles->fixed_fps = p_fps;

	_particles_free_data(particles);

	particles->prev_ticks = 0;
	particles->phase = 0;
	particles->prev_phase = 0;
	particles->clear = true;

	particles->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_PARTICLES);
}

void ParticlesStorage::particles_set_interpolate(RID p_particles, bool p_enable)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	particles->interpolate = p_enable;
}

void ParticlesStorage::particles_set_fractional_delta(RID p_particles, bool p_enable)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	particles->fractional_delta = p_enable;
}

void ParticlesStorage::particles_set_trails(RID p_particles, bool p_enable, double p_length)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);
	ERR_FAIL_COND(p_length < 0.01 - CMP_EPSILON);
	p_length = MIN(10.0, p_length);

	particles->trails_enabled = p_enable;
	particles->trail_lifetime = p_length;

	_particles_free_data(particles);

	particles->prev_ticks = 0;
	particles->phase = 0;
	particles->prev_phase = 0;
	particles->clear = true;

	particles->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_PARTICLES);
}

void ParticlesStorage::particles_set_trail_bind_poses(
	RID p_particles, const Vector<Transform3D>& p_bind_poses)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);
	if (particles->trail_bind_pose_buffer.is_valid() &&
		particles->trail_bind_poses.size() != p_bind_poses.size()) {
		_particles_free_data(particles);

		particles->prev_ticks = 0;
		particles->phase = 0;
		particles->prev_phase = 0;
		particles->clear = true;
	}
	particles->trail_bind_poses = p_bind_poses;
	particles->trail_bind_poses_dirty = true;

	particles->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_PARTICLES);
}

void ParticlesStorage::particles_set_collision_base_size(RID p_particles, real_t p_size)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	particles->collision_base_size = p_size;
}

void ParticlesStorage::particles_set_transform_align(
	RID p_particles, RSE::ParticlesTransformAlign p_transform_align)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	particles->transform_align = p_transform_align;
}

void ParticlesStorage::particles_set_transform_align_channel_filter(
	RID p_particles, RSE::ParticlesTransformAlignCustomSrc p_transform_align_channel_filter)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	particles->transform_align_src = p_transform_align_channel_filter;
}

void ParticlesStorage::particles_set_transform_align_axis(
	RID p_particles, RSE::ParticlesTransformAlignAxis p_rotation_axis)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	particles->transform_align_axis = p_rotation_axis;
}

void ParticlesStorage::particles_set_process_material(RID p_particles, RID p_material)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	particles->process_material = p_material;
	particles->dependency.changed_notify(
		Dependency::DEPENDENCY_CHANGED_PARTICLES); // the instance buffer may have changed
}

RID ParticlesStorage::particles_get_process_material(RID p_particles)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL_V(particles, RID());

	return particles->process_material;
}

void ParticlesStorage::particles_set_draw_order(RID p_particles, RSE::ParticlesDrawOrder p_order)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	particles->draw_order = p_order;
}

void ParticlesStorage::particles_set_draw_passes(RID p_particles, int p_passes)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	particles->draw_passes.resize(p_passes);
}

void ParticlesStorage::particles_set_draw_pass_mesh(RID p_particles, int p_pass, RID p_mesh)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);
	ERR_FAIL_INDEX(p_pass, particles->draw_passes.size());
	particles->draw_passes.write[p_pass] = p_mesh;
}

void ParticlesStorage::particles_restart(RID p_particles)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	particles->restart_request = true;
}

void ParticlesStorage::particles_set_seed(RID p_particles, uint32_t p_seed)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);
	particles->random_seed = p_seed;
}

void ParticlesStorage::_particles_allocate_emission_buffer(Particles* particles)
{
	ERR_FAIL_COND(particles->emission_buffer != nullptr);

	particles->emission_buffer_data.resize(
		sizeof(ParticleEmissionBuffer::Data) * particles->amount + sizeof(uint32_t) * 4);
	memset(particles->emission_buffer_data.ptrw(), 0, particles->emission_buffer_data.size());
	particles->emission_buffer =
		reinterpret_cast<ParticleEmissionBuffer*>(particles->emission_buffer_data.ptrw());
	particles->emission_buffer->particle_max = particles->amount;

	particles->emission_storage_buffer = RD::storage_buffer_create(
		particles->emission_buffer_data.size(), particles->emission_buffer_data);

	if (RD::uniform_set_is_valid(particles->particles_material_uniform_set)) {
		// will need to be re-created
		RD::free_rid(particles->particles_material_uniform_set);
		particles->particles_material_uniform_set = RID();
	}
}

void ParticlesStorage::_particles_ensure_unused_emission_buffer(Particles* particles)
{
	if (particles->unused_emission_storage_buffer.is_null()) {
		// For rendering devices that do not support empty arrays (like C++),
		// we need to size the buffer with at least 1 element.
		particles->unused_emission_storage_buffer =
			RD::storage_buffer_create(sizeof(ParticleEmissionBuffer));
	}
}

void ParticlesStorage::_particles_ensure_unused_trail_buffer(Particles* particles)
{
	if (particles->unused_trail_storage_buffer.is_null()) {
		particles->unused_trail_storage_buffer =
			RD::storage_buffer_create(16 * sizeof(float)); // Size of mat4.
	}
}

void ParticlesStorage::particles_set_subemitter(RID p_particles, RID p_subemitter_particles)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);
	ERR_FAIL_COND(p_particles == p_subemitter_particles);

	particles->sub_emitter = p_subemitter_particles;

	if (RD::uniform_set_is_valid(particles->particles_material_uniform_set)) {
		RD::free_rid(particles->particles_material_uniform_set);
		particles->particles_material_uniform_set =
			RID(); // clear and force to re create sub emitting
	}
}

void ParticlesStorage::particles_emit(RID p_particles, const Transform3D& p_transform,
	const Vector3& p_velocity, const Color& p_color, const Color& p_custom, uint32_t p_emit_flags)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);
	ERR_FAIL_COND(particles->amount == 0);

	if (particles->emitting) {
		particles->clear = true;
		particles->emitting = false;
	}

	if (particles->emission_buffer == nullptr) {
		_particles_allocate_emission_buffer(particles);
	}

	particles->inactive = false;
	particles->inactive_time = 0;

	int32_t idx = particles->emission_buffer->particle_count;
	if (idx < particles->emission_buffer->particle_max) {
		RendererRD::MaterialStorage::store_transform(
			p_transform, particles->emission_buffer->data[idx].xform);

		particles->emission_buffer->data[idx].velocity[0] = p_velocity.x;
		particles->emission_buffer->data[idx].velocity[1] = p_velocity.y;
		particles->emission_buffer->data[idx].velocity[2] = p_velocity.z;

		particles->emission_buffer->data[idx].custom[0] = p_custom.r;
		particles->emission_buffer->data[idx].custom[1] = p_custom.g;
		particles->emission_buffer->data[idx].custom[2] = p_custom.b;
		particles->emission_buffer->data[idx].custom[3] = p_custom.a;

		particles->emission_buffer->data[idx].color[0] = p_color.r;
		particles->emission_buffer->data[idx].color[1] = p_color.g;
		particles->emission_buffer->data[idx].color[2] = p_color.b;
		particles->emission_buffer->data[idx].color[3] = p_color.a;

		particles->emission_buffer->data[idx].flags = p_emit_flags;
		particles->emission_buffer->particle_count++;
	}
}

void ParticlesStorage::particles_request_process(RID p_particles)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	if (!particles->dirty) {
		particles->dirty = true;

		if (!particles->update_list.in_list()) {
			particle_update_list.add(&particles->update_list);
		}
	}
}

AABB ParticlesStorage::particles_get_current_aabb(RID p_particles)
{
	const Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL_V(particles, AABB());

	int total_amount = particles->amount;
	if (particles->trails_enabled && particles->trail_bind_poses.size() > 1) {
		total_amount *= particles->trail_bind_poses.size();
	}

	uint32_t particle_data_size =
		sizeof(ParticleData) + sizeof(float) * 4 * particles->userdata_count;
	Vector<uint8_t> buffer = RD::buffer_get_data(particles->particle_buffer);
	ERR_FAIL_COND_V(buffer.size() != (int)(total_amount * particle_data_size), AABB());

	Transform3D inv = particles->emission_transform.affine_inverse();

	AABB aabb;
	if (buffer.size()) {
		bool first = true;

		const uint8_t* data_ptr = (const uint8_t*)buffer.ptr();

		for (int i = 0; i < total_amount; i++) {
			const ParticleData& particle_data =
				*(const ParticleData*)&data_ptr[particle_data_size * i];
			if (particle_data.active) {
				Vector3 pos = Vector3(
					particle_data.xform[12], particle_data.xform[13], particle_data.xform[14]);
				if (!particles->use_local_coords) {
					pos = inv.xform(pos);
				}
				if (first) {
					aabb.position = pos;
					first = false;
				}
				else {
					aabb.expand_to(pos);
				}
			}
		}
	}

	float longest_axis_size = 0;
	for (int i = 0; i < particles->draw_passes.size(); i++) {
		if (particles->draw_passes[i].is_valid()) {
			AABB maabb =
				MeshStorage::mesh_get_aabb(particles->draw_passes[i], RID());
			longest_axis_size = MAX(maabb.get_longest_axis_size(), longest_axis_size);
		}
	}

	aabb.grow_by(longest_axis_size);

	return aabb;
}

AABB ParticlesStorage::particles_get_aabb(RID p_particles)
{
	const Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL_V(particles, AABB());

	return particles->custom_aabb;
}

void ParticlesStorage::particles_set_emission_transform(
	RID p_particles, const Transform3D& p_transform)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	particles->emission_transform = p_transform;
}

void ParticlesStorage::particles_set_emitter_velocity(RID p_particles, const Vector3& p_velocity)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	particles->emitter_velocity = p_velocity;
}

void ParticlesStorage::particles_set_interp_to_end(RID p_particles, float p_interp)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);

	particles->interp_to_end = p_interp;
}

int ParticlesStorage::particles_get_draw_passes(RID p_particles)
{
	const Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL_V(particles, 0);

	return particles->draw_passes.size();
}

RID ParticlesStorage::particles_get_draw_pass_mesh(RID p_particles, int p_pass)
{
	const Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL_V(particles, RID());
	ERR_FAIL_INDEX_V(p_pass, particles->draw_passes.size(), RID());

	return particles->draw_passes[p_pass];
}

void ParticlesStorage::particles_update_dependency(RID p_particles, DependencyTracker* p_instance)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);
	p_instance->update_dependency(&particles->dependency);
}

void ParticlesStorage::particles_get_instance_buffer_motion_vectors_offsets(
	RID p_particles, uint32_t& r_current_offset, uint32_t& r_prev_offset)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);
	r_current_offset = particles->instance_motion_vectors_current_offset;
	r_prev_offset = particles->instance_motion_vectors_previous_offset;
}

void ParticlesStorage::particles_add_collision(RID p_particles, RID p_particles_collision_instance)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);
	particles->collisions.insert(p_particles_collision_instance);
}

void ParticlesStorage::particles_remove_collision(
	RID p_particles, RID p_particles_collision_instance)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);
	particles->collisions.erase(p_particles_collision_instance);
}

void ParticlesStorage::particles_set_canvas_sdf_collision(RID p_particles, bool p_enable,
	const Transform2D& p_xform, const Rect2& p_to_screen, RID p_texture)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL(particles);
	particles->has_sdf_collision = p_enable;
	particles->sdf_collision_transform = p_xform;
	particles->sdf_collision_to_screen = p_to_screen;
	particles->sdf_collision_texture = p_texture;
}

void ParticlesStorage::_particles_process(Particles* p_particles, double p_delta)
{
	if (p_particles->particles_material_uniform_set.is_null() ||
		!RD::uniform_set_is_valid(p_particles->particles_material_uniform_set)) {
		thread_local LocalVector<RD::Uniform> uniforms;
		uniforms.clear();

		{
			RD::Uniform u;
			u.uniform_type = RDC::UNIFORM_TYPE_STORAGE_BUFFER;
			u.binding = 0;
			u.append_id(p_particles->frame_params_buffer);
			uniforms.push_back(u);
		}
		{
			RD::Uniform u;
			u.uniform_type = RDC::UNIFORM_TYPE_STORAGE_BUFFER;
			u.binding = 1;
			u.append_id(p_particles->particle_buffer);
			uniforms.push_back(u);
		}

		{
			RD::Uniform u;
			u.uniform_type = RDC::UNIFORM_TYPE_STORAGE_BUFFER;
			u.binding = 2;
			if (p_particles->emission_storage_buffer.is_valid()) {
				u.append_id(p_particles->emission_storage_buffer);
			}
			else {
				_particles_ensure_unused_emission_buffer(p_particles);
				u.append_id(p_particles->unused_emission_storage_buffer);
			}
			uniforms.push_back(u);
		}
		{
			RD::Uniform u;
			u.uniform_type = RDC::UNIFORM_TYPE_STORAGE_BUFFER;
			u.binding = 3;
			Particles* sub_emitter = particles_owner.get_or_null(p_particles->sub_emitter);
			if (sub_emitter) {
				if (sub_emitter->emission_buffer ==
					nullptr) { // no emission buffer, allocate emission buffer
					_particles_allocate_emission_buffer(sub_emitter);
				}
				u.append_id(sub_emitter->emission_storage_buffer);
			}
			else {
				_particles_ensure_unused_emission_buffer(p_particles);
				u.append_id(p_particles->unused_emission_storage_buffer);
			}
			uniforms.push_back(u);
		}

		p_particles->particles_material_uniform_set =
			RD::uniform_set_create(uniforms, particles_shader.default_shader_rd, 1);
	}

	double new_phase =
		Math::fmod((double)(p_particles->phase + (p_delta / p_particles->lifetime)), 1.0);

	// move back history (if there is any)
	for (uint32_t i = p_particles->frame_history.size() - 1; i > 0; i--) {
		p_particles->frame_history[i] = p_particles->frame_history[i - 1];
	}
	// update current frame
	ParticlesFrameParams& frame_params = p_particles->frame_history[0];

	if (p_particles->clear) {
		p_particles->cycle_number = 0;
	}
	else if (new_phase < p_particles->phase) {
		if (p_particles->one_shot) {
			p_particles->emitting = false;
		}
		p_particles->cycle_number++;
	}

	frame_params.emitting = p_particles->emitting;
	frame_params.system_phase = new_phase;
	frame_params.prev_system_phase = p_particles->phase;

	p_particles->phase = new_phase;

	frame_params.time = RendererCompositorRD::get_total_time();
	frame_params.delta = p_delta;
	frame_params.random_seed = p_particles->random_seed;
	frame_params.explosiveness = p_particles->explosiveness;
	frame_params.randomness = p_particles->randomness;

	if (p_particles->use_local_coords) {
		RendererRD::MaterialStorage::store_transform(
			Transform3D(), frame_params.emission_transform);
	}
	else {
		RendererRD::MaterialStorage::store_transform(
			p_particles->emission_transform, frame_params.emission_transform);
	}

	frame_params.cycle = p_particles->cycle_number;
	frame_params.frame = p_particles->frame_counter++;
	frame_params.amount_ratio = p_particles->amount_ratio;
	frame_params.pad1 = 0;
	frame_params.pad2 = 0;
	frame_params.emitter_velocity[0] = p_particles->emitter_velocity.x;
	frame_params.emitter_velocity[1] = p_particles->emitter_velocity.y;
	frame_params.emitter_velocity[2] = p_particles->emitter_velocity.z;
	frame_params.interp_to_end = p_particles->interp_to_end;

	{ // collision and attractors

		frame_params.collider_count = 0;
		frame_params.attractor_count = 0;
		frame_params.particle_size = p_particles->collision_base_size;

		RID collision_3d_textures[ParticlesFrameParams::MAX_3D_TEXTURES];
		RID collision_heightmap_texture;

		Transform3D to_particles;
		if (p_particles->use_local_coords) {
			to_particles = p_particles->emission_transform.affine_inverse();
		}

		if (p_particles->has_sdf_collision &&
			RD::texture_is_valid(p_particles->sdf_collision_texture)) {
			// 2D collision

			Transform2D xform =
				p_particles
					->sdf_collision_transform; // will use dotproduct manually so invert beforehand

			if (!p_particles->use_local_coords) {
				Transform2D emission;
				emission.columns[0] = Vector2(p_particles->emission_transform.basis.get_column(0).x,
					p_particles->emission_transform.basis.get_column(0).y);
				emission.columns[1] = Vector2(p_particles->emission_transform.basis.get_column(1).x,
					p_particles->emission_transform.basis.get_column(1).y);
				emission.set_origin(Vector2(p_particles->emission_transform.origin.x,
					p_particles->emission_transform.origin.y));
				xform = xform * emission.affine_inverse();
			}

			Transform2D revert = xform.affine_inverse();
			frame_params.collider_count = 1;
			frame_params.colliders[0].transform[0] = xform.columns[0][0];
			frame_params.colliders[0].transform[1] = xform.columns[0][1];
			frame_params.colliders[0].transform[2] = 0;
			frame_params.colliders[0].transform[3] = xform.columns[2][0];

			frame_params.colliders[0].transform[4] = xform.columns[1][0];
			frame_params.colliders[0].transform[5] = xform.columns[1][1];
			frame_params.colliders[0].transform[6] = 0;
			frame_params.colliders[0].transform[7] = xform.columns[2][1];

			frame_params.colliders[0].transform[8] = revert.columns[0][0];
			frame_params.colliders[0].transform[9] = revert.columns[0][1];
			frame_params.colliders[0].transform[10] = 0;
			frame_params.colliders[0].transform[11] = revert.columns[2][0];

			frame_params.colliders[0].transform[12] = revert.columns[1][0];
			frame_params.colliders[0].transform[13] = revert.columns[1][1];
			frame_params.colliders[0].transform[14] = 0;
			frame_params.colliders[0].transform[15] = revert.columns[2][1];

			frame_params.colliders[0].extents[0] = p_particles->sdf_collision_to_screen.size.x;
			frame_params.colliders[0].extents[1] = p_particles->sdf_collision_to_screen.size.y;
			frame_params.colliders[0].extents[2] = p_particles->sdf_collision_to_screen.position.x;
			frame_params.colliders[0].scale = p_particles->sdf_collision_to_screen.position.y;
			frame_params.colliders[0].texture_index = 0;
			frame_params.colliders[0].type = ParticlesFrameParams::COLLISION_TYPE_2D_SDF;

			collision_heightmap_texture = p_particles->sdf_collision_texture;

			// replace in all other history frames where used because parameters are no longer valid
			// if screen moves
			for (ParticlesFrameParams& params : p_particles->frame_history) {
				if (params.collider_count > 0 &&
					params.colliders[0].type == ParticlesFrameParams::COLLISION_TYPE_2D_SDF) {
					params.colliders[0] = frame_params.colliders[0];
				}
			}
		}

		uint32_t collision_3d_textures_used = 0;
		for (const RID& E : p_particles->collisions) {
			ParticlesCollisionInstance* pci = particles_collision_instance_owner.get_or_null(E);
			if (!pci || !pci->active) {
				continue;
			}
			ParticlesCollision* pc = particles_collision_owner.get_or_null(pci->collision);
			ERR_CONTINUE(!pc);

			Transform3D to_collider = pci->transform;
			if (p_particles->use_local_coords) {
				to_collider = to_particles * to_collider;
			}
			Vector3 scale = to_collider.basis.get_scale();
			to_collider.basis.orthonormalize();

			if (pc->type <= RSE::PARTICLES_COLLISION_TYPE_VECTOR_FIELD_ATTRACT) {
				// attractor
				if (frame_params.attractor_count >= ParticlesFrameParams::MAX_ATTRACTORS) {
					continue;
				}

				ParticlesFrameParams::Attractor& attr =
					frame_params.attractors[frame_params.attractor_count];

				RendererRD::MaterialStorage::store_transform(to_collider, attr.transform);
				attr.strength = pc->attractor_strength;
				attr.attenuation = pc->attractor_attenuation;
				attr.directionality = pc->attractor_directionality;

				switch (pc->type) {
				case RSE::PARTICLES_COLLISION_TYPE_SPHERE_ATTRACT: {
					attr.type = ParticlesFrameParams::ATTRACTOR_TYPE_SPHERE;
					float radius = pc->radius;
					radius *= (scale.x + scale.y + scale.z) / 3.0;
					attr.extents[0] = radius;
					attr.extents[1] = radius;
					attr.extents[2] = radius;
				} break;
				case RSE::PARTICLES_COLLISION_TYPE_BOX_ATTRACT: {
					attr.type = ParticlesFrameParams::ATTRACTOR_TYPE_BOX;
					Vector3 extents = pc->extents * scale;
					attr.extents[0] = extents.x;
					attr.extents[1] = extents.y;
					attr.extents[2] = extents.z;
				} break;
				case RSE::PARTICLES_COLLISION_TYPE_VECTOR_FIELD_ATTRACT: {
					if (collision_3d_textures_used >= ParticlesFrameParams::MAX_3D_TEXTURES) {
						continue;
					}
					attr.type = ParticlesFrameParams::ATTRACTOR_TYPE_VECTOR_FIELD;
					Vector3 extents = pc->extents * scale;
					attr.extents[0] = extents.x;
					attr.extents[1] = extents.y;
					attr.extents[2] = extents.z;
					attr.texture_index = collision_3d_textures_used;

					collision_3d_textures[collision_3d_textures_used] = pc->field_texture;
					collision_3d_textures_used++;
				} break;
				default: {
				}
				}

				frame_params.attractor_count++;
			}
			else {
				// collider
				if (frame_params.collider_count >= ParticlesFrameParams::MAX_COLLIDERS) {
					continue;
				}

				ParticlesFrameParams::Collider& col =
					frame_params.colliders[frame_params.collider_count];

				RendererRD::MaterialStorage::store_transform(to_collider, col.transform);
				switch (pc->type) {
				case RSE::PARTICLES_COLLISION_TYPE_SPHERE_COLLIDE: {
					col.type = ParticlesFrameParams::COLLISION_TYPE_SPHERE;
					float radius = pc->radius;
					radius *= (scale.x + scale.y + scale.z) / 3.0;
					col.extents[0] = radius;
					col.extents[1] = radius;
					col.extents[2] = radius;
				} break;
				case RSE::PARTICLES_COLLISION_TYPE_BOX_COLLIDE: {
					col.type = ParticlesFrameParams::COLLISION_TYPE_BOX;
					Vector3 extents = pc->extents * scale;
					col.extents[0] = extents.x;
					col.extents[1] = extents.y;
					col.extents[2] = extents.z;
				} break;
				case RSE::PARTICLES_COLLISION_TYPE_SDF_COLLIDE: {
					if (collision_3d_textures_used >= ParticlesFrameParams::MAX_3D_TEXTURES) {
						continue;
					}
					col.type = ParticlesFrameParams::COLLISION_TYPE_SDF;
					Vector3 extents = pc->extents * scale;
					col.extents[0] = extents.x;
					col.extents[1] = extents.y;
					col.extents[2] = extents.z;
					col.texture_index = collision_3d_textures_used;
					col.scale = (scale.x + scale.y + scale.z) *
								0.333333333333; // non uniform scale non supported

					collision_3d_textures[collision_3d_textures_used] = pc->field_texture;
					collision_3d_textures_used++;
				} break;
				case RSE::PARTICLES_COLLISION_TYPE_HEIGHTFIELD_COLLIDE: {
					if (collision_heightmap_texture != RID()) { // already taken
						continue;
					}

					col.type = ParticlesFrameParams::COLLISION_TYPE_HEIGHT_FIELD;
					Vector3 extents = pc->extents * scale;
					col.extents[0] = extents.x;
					col.extents[1] = extents.y;
					col.extents[2] = extents.z;
					collision_heightmap_texture = pc->heightfield_texture;
				} break;
				default: {
				}
				}

				frame_params.collider_count++;
			}
		}

		bool different = false;
		if (collision_3d_textures_used == p_particles->collision_3d_textures_used) {
			for (int i = 0; i < ParticlesFrameParams::MAX_3D_TEXTURES; i++) {
				if (p_particles->collision_3d_textures[i] != collision_3d_textures[i]) {
					different = true;
					break;
				}
			}
		}

		if (collision_heightmap_texture != p_particles->collision_heightmap_texture) {
			different = true;
		}

		bool uniform_set_valid =
			RD::uniform_set_is_valid(p_particles->collision_textures_uniform_set);

		if (different || !uniform_set_valid) {
			if (uniform_set_valid) {
				RD::free_rid(p_particles->collision_textures_uniform_set);
			}

			thread_local LocalVector<RD::Uniform> uniforms;
			uniforms.clear();

			{
				RD::Uniform u;
				u.uniform_type = RDC::UNIFORM_TYPE_TEXTURE;
				u.binding = 0;
				for (uint32_t i = 0; i < ParticlesFrameParams::MAX_3D_TEXTURES; i++) {
					RID rd_tex;
					if (i < collision_3d_textures_used) {
						if (TextureStorage::texture_get_type(
								collision_3d_textures[i]) == TextureStorage::TYPE_3D) {
							rd_tex = TextureStorage::texture_get_rd_texture(
								collision_3d_textures[i]);
						}
					}

					if (rd_tex == RID()) {
						rd_tex = RendererRD::TextureStorage::texture_rd_get_default(
							TextureStorage::DEFAULT_RD_TEXTURE_3D_WHITE);
					}
					u.append_id(rd_tex);
				}
				uniforms.push_back(u);
			}
			{
				RD::Uniform u;
				u.uniform_type = RDC::UNIFORM_TYPE_TEXTURE;
				u.binding = 1;
				if (collision_heightmap_texture.is_valid()) {
					u.append_id(collision_heightmap_texture);
				}
				else {
					u.append_id(RendererRD::TextureStorage::texture_rd_get_default(
						TextureStorage::DEFAULT_RD_TEXTURE_BLACK));
				}
				uniforms.push_back(u);
			}
			p_particles->collision_textures_uniform_set =
				RD::uniform_set_create(uniforms, particles_shader.default_shader_rd, 2);
			p_particles->collision_heightmap_texture = collision_heightmap_texture;
		}
	}

	ParticlesShader::PushConstant push_constant;

	int process_amount = p_particles->amount;

	if (p_particles->trails_enabled && p_particles->trail_bind_poses.size() > 1) {
		process_amount *= p_particles->trail_bind_poses.size();
	}
	push_constant.clear = p_particles->clear;
	push_constant.total_particles = p_particles->amount;
	push_constant.lifetime = p_particles->lifetime;
	push_constant.trail_size = p_particles->trail_params.size();
	push_constant.use_fractional_delta = p_particles->fractional_delta;
	push_constant.sub_emitter_mode =
		!p_particles->emitting && p_particles->emission_buffer &&
		(p_particles->emission_buffer->particle_count > 0 || p_particles->force_sub_emit);
	push_constant.trail_pass = false;

	p_particles->force_sub_emit = false; // reset

	Particles* sub_emitter = particles_owner.get_or_null(p_particles->sub_emitter);

	if (sub_emitter && sub_emitter->emission_storage_buffer.is_valid()) {
		//	print_line("updating subemitter buffer");
		int32_t zero[4] = {0, sub_emitter->amount, 0, 0};
		push_constant.can_emit = true;

		if (sub_emitter->emitting) {
			sub_emitter->emitting = false;
			sub_emitter->clear = true; // will need to clear if it was emitting, sorry
		}
		// make sure the sub emitter processes particles too
		sub_emitter->inactive = false;
		sub_emitter->inactive_time = 0;

		sub_emitter->force_sub_emit = true;

	}
	else {
		push_constant.can_emit = false;
	}

	if (p_particles->emission_buffer && p_particles->emission_buffer->particle_count) {
		p_particles->emission_buffer->particle_count = 0;
	}

	p_particles->clear = false;

	if (p_particles->trail_params.size() > 1) {
		// fill the trail params
		for (uint32_t i = 0; i < p_particles->trail_params.size(); i++) {
			uint32_t src_idx =
				i * p_particles->frame_history.size() / p_particles->trail_params.size();
			if (p_particles->speed_scale <= 0.0) {
				// Stop trails.
				src_idx = 0;
			}
			p_particles->trail_params[i] = p_particles->frame_history[src_idx];
		}
	}
	else {
		p_particles->trail_params[0] = p_particles->frame_history[0];
	}

	ParticleProcessMaterialData* m =
		static_cast<ParticleProcessMaterialData*>(MaterialStorage::material_get_data(
			p_particles->process_material, MaterialStorage::SHADER_TYPE_PARTICLES));
	if (!m) {
		m = static_cast<ParticleProcessMaterialData*>(MaterialStorage::material_get_data(
			particles_shader.default_material, MaterialStorage::SHADER_TYPE_PARTICLES));
	}

	ERR_FAIL_NULL(m);

	p_particles->has_collision_cache = m->shader_data->uses_collision;

	// todo should maybe compute all particle systems together?
	RD::ComputeListID compute_list = RD::compute_list_begin();
	RD::compute_list_bind_uniform_set(
		compute_list, particles_shader.base_uniform_set, BASE_UNIFORM_SET);
	RD::compute_list_bind_uniform_set(
		compute_list, p_particles->particles_material_uniform_set, MATERIAL_UNIFORM_SET);
	RD::compute_list_bind_uniform_set(
		compute_list, p_particles->collision_textures_uniform_set, COLLISION_TEXTURTES_UNIFORM_SET);

	if (m->uniform_set.is_valid() && RD::uniform_set_is_valid(m->uniform_set)) {
		RD::compute_list_bind_uniform_set(compute_list, m->uniform_set, 3);
		m->set_as_used();
	}

	RD::compute_list_set_push_constant(
		compute_list, &push_constant, sizeof(ParticlesShader::PushConstant));

	if (p_particles->trails_enabled && p_particles->trail_bind_poses.size() > 1) {
		// trails requires two passes in order to catch particle starts
		RD::compute_list_dispatch_threads(
			compute_list, process_amount / p_particles->trail_bind_poses.size(), 1, 1);

		RD::compute_list_add_barrier(compute_list);

		push_constant.trail_pass = true;
		RD::compute_list_set_push_constant(
			compute_list, &push_constant, sizeof(ParticlesShader::PushConstant));
		RD::compute_list_dispatch_threads(compute_list, process_amount - p_particles->amount, 1, 1);
	}
	else {
		RD::compute_list_dispatch_threads(compute_list, process_amount, 1, 1);
	}

	RD::compute_list_end();
}

Dependency* ParticlesStorage::particles_get_dependency(RID p_particles)
{
	Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL_V(particles, nullptr);

	return &particles->dependency;
}

bool ParticlesStorage::particles_is_inactive(RID p_particles)
{
	const Particles* particles = particles_owner.get_or_null(p_particles);
	ERR_FAIL_NULL_V(particles, false);
	return !particles->emitting && particles->inactive;
}

/* Particles SHADER */

bool ParticlesStorage::ParticlesShaderData::is_animated() const { return false; }

bool ParticlesStorage::ParticlesShaderData::casts_shadows() const { return false; }

RenderingServerTypes::ShaderNativeSourceCode
ParticlesStorage::ParticlesShaderData::get_native_source_code() const
{
	return ParticlesStorage::particles_shader.shader.version_get_native_source_code(version);
}

Pair<ShaderRD*, RID> ParticlesStorage::ParticlesShaderData::get_native_shader_and_version() const
{
	return {&ParticlesStorage::particles_shader.shader, version};
}

ParticlesStorage::ParticlesShaderData::~ParticlesShaderData()
{
	pipeline.free();

	if (version.is_valid()) {
		ParticlesStorage::particles_shader.shader.version_free(version);
	}
}

RendererRD::MaterialStorage::ShaderData* ParticlesStorage::_create_particles_shader_func()
{
	ParticlesShaderData* shader_data = memnew(ParticlesShaderData);
	return shader_data;
}

ParticlesStorage::ParticleProcessMaterialData::~ParticleProcessMaterialData()
{
	free_parameters_uniform_set(uniform_set);
}

RendererRD::MaterialStorage::MaterialData* ParticlesStorage::_create_particles_material_func(
	ParticlesShaderData* p_shader)
{
	ParticleProcessMaterialData* material_data = memnew(ParticleProcessMaterialData);
	material_data->shader_data = p_shader;
	// update will happen later anyway so do nothing.
	return material_data;
}

/* PARTICLES COLLISION API */

RID ParticlesStorage::particles_collision_allocate()
{
	return particles_collision_owner.allocate_rid();
}

void ParticlesStorage::particles_collision_initialize(RID p_rid)
{
	particles_collision_owner.initialize_rid(p_rid, ParticlesCollision());
}

void ParticlesStorage::particles_collision_free(RID p_rid)
{
	ParticlesCollision* particles_collision = particles_collision_owner.get_or_null(p_rid);

	if (particles_collision->heightfield_texture.is_valid()) {
		RD::free_rid(particles_collision->heightfield_texture);
	}
	particles_collision->dependency.deleted_notify(p_rid);
	particles_collision_owner.free(p_rid);
}

RID ParticlesStorage::particles_collision_get_heightfield_framebuffer(
	RID p_particles_collision)
{
	ParticlesCollision* particles_collision =
		particles_collision_owner.get_or_null(p_particles_collision);
	ERR_FAIL_NULL_V(particles_collision, RID());
	ERR_FAIL_COND_V(
		particles_collision->type != RSE::PARTICLES_COLLISION_TYPE_HEIGHTFIELD_COLLIDE, RID());

	if (particles_collision->heightfield_texture == RID()) {
		// create
		const int resolutions[RSE::PARTICLES_COLLISION_HEIGHTFIELD_RESOLUTION_MAX] = {
			256, 512, 1024, 2048, 4096, 8192};
		Size2i size;
		if (particles_collision->extents.x > particles_collision->extents.z) {
			size.x = resolutions[particles_collision->heightfield_resolution];
			size.y =
				int32_t(particles_collision->extents.z / particles_collision->extents.x * size.x);
		}
		else {
			size.y = resolutions[particles_collision->heightfield_resolution];
			size.x =
				int32_t(particles_collision->extents.x / particles_collision->extents.z * size.y);
		}

		RDC::TextureFormat tf;
		tf.format = RDC::DATA_FORMAT_D32_SFLOAT;
		tf.width = size.x;
		tf.height = size.y;
		tf.texture_type = RDC::TEXTURE_TYPE_2D;
		tf.usage_bits =
			RDC::TEXTURE_USAGE_SAMPLING_BIT | RDC::TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

		particles_collision->heightfield_texture = RD::texture_create(tf, RDD::TextureView());

		Vector<RID> fb_tex;
		fb_tex.push_back(particles_collision->heightfield_texture);
		particles_collision->heightfield_fb = RD::framebuffer_create(fb_tex);
		particles_collision->heightfield_fb_size = size;
	}

	return particles_collision->heightfield_fb;
}

void ParticlesStorage::particles_collision_set_collision_type(
	RID p_particles_collision, RSE::ParticlesCollisionType p_type)
{
	ParticlesCollision* particles_collision =
		particles_collision_owner.get_or_null(p_particles_collision);
	ERR_FAIL_NULL(particles_collision);

	if (p_type == particles_collision->type) {
		return;
	}

	if (particles_collision->heightfield_texture.is_valid()) {
		RD::free_rid(particles_collision->heightfield_texture);
		particles_collision->heightfield_texture = RID();
	}
	particles_collision->type = p_type;
	particles_collision->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_AABB);
}

void ParticlesStorage::particles_collision_set_cull_mask(
	RID p_particles_collision, uint32_t p_cull_mask)
{
	ParticlesCollision* particles_collision =
		particles_collision_owner.get_or_null(p_particles_collision);
	ERR_FAIL_NULL(particles_collision);
	particles_collision->cull_mask = p_cull_mask;
	particles_collision->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_CULL_MASK);
}

uint32_t ParticlesStorage::particles_collision_get_cull_mask(RID p_particles_collision)
{
	ParticlesCollision* particles_collision =
		particles_collision_owner.get_or_null(p_particles_collision);
	ERR_FAIL_NULL_V(particles_collision, 0);
	return particles_collision->cull_mask;
}

uint32_t ParticlesStorage::particles_collision_get_height_field_mask(
	RID p_particles_collision)
{
	const ParticlesCollision* particles_collision =
		particles_collision_owner.get_or_null(p_particles_collision);
	ERR_FAIL_NULL_V(particles_collision, false);
	return particles_collision->heightfield_mask;
}

void ParticlesStorage::particles_collision_set_height_field_mask(
	RID p_particles_collision, uint32_t p_heightfield_mask)
{
	ParticlesCollision* particles_collision =
		particles_collision_owner.get_or_null(p_particles_collision);
	ERR_FAIL_NULL(particles_collision);
	particles_collision->heightfield_mask = p_heightfield_mask;
}

void ParticlesStorage::particles_collision_set_sphere_radius(
	RID p_particles_collision, real_t p_radius)
{
	ParticlesCollision* particles_collision =
		particles_collision_owner.get_or_null(p_particles_collision);
	ERR_FAIL_NULL(particles_collision);

	particles_collision->radius = p_radius;
	particles_collision->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_AABB);
}

void ParticlesStorage::particles_collision_set_box_extents(
	RID p_particles_collision, const Vector3& p_extents)
{
	ParticlesCollision* particles_collision =
		particles_collision_owner.get_or_null(p_particles_collision);
	ERR_FAIL_NULL(particles_collision);

	particles_collision->extents = p_extents;
	particles_collision->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_AABB);
}

void ParticlesStorage::particles_collision_set_attractor_strength(
	RID p_particles_collision, real_t p_strength)
{
	ParticlesCollision* particles_collision =
		particles_collision_owner.get_or_null(p_particles_collision);
	ERR_FAIL_NULL(particles_collision);

	particles_collision->attractor_strength = p_strength;
}

void ParticlesStorage::particles_collision_set_attractor_directionality(
	RID p_particles_collision, real_t p_directionality)
{
	ParticlesCollision* particles_collision =
		particles_collision_owner.get_or_null(p_particles_collision);
	ERR_FAIL_NULL(particles_collision);

	particles_collision->attractor_directionality = p_directionality;
}

void ParticlesStorage::particles_collision_set_attractor_attenuation(
	RID p_particles_collision, real_t p_curve)
{
	ParticlesCollision* particles_collision =
		particles_collision_owner.get_or_null(p_particles_collision);
	ERR_FAIL_NULL(particles_collision);

	particles_collision->attractor_attenuation = p_curve;
}

void ParticlesStorage::particles_collision_set_field_texture(
	RID p_particles_collision, RID p_texture)
{
	ParticlesCollision* particles_collision =
		particles_collision_owner.get_or_null(p_particles_collision);
	ERR_FAIL_NULL(particles_collision);

	particles_collision->field_texture = p_texture;
}

void ParticlesStorage::particles_collision_height_field_update(RID p_particles_collision)
{
	ParticlesCollision* particles_collision =
		particles_collision_owner.get_or_null(p_particles_collision);
	ERR_FAIL_NULL(particles_collision);
	particles_collision->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_AABB);
}

void ParticlesStorage::particles_collision_set_height_field_resolution(
	RID p_particles_collision, RSE::ParticlesCollisionHeightfieldResolution p_resolution)
{
	ParticlesCollision* particles_collision =
		particles_collision_owner.get_or_null(p_particles_collision);
	ERR_FAIL_NULL(particles_collision);
	ERR_FAIL_INDEX(p_resolution, RSE::PARTICLES_COLLISION_HEIGHTFIELD_RESOLUTION_MAX);

	if (particles_collision->heightfield_resolution == p_resolution) {
		return;
	}

	particles_collision->heightfield_resolution = p_resolution;

	if (particles_collision->heightfield_texture.is_valid()) {
		RD::free_rid(particles_collision->heightfield_texture);
		particles_collision->heightfield_texture = RID();
	}
}

AABB ParticlesStorage::particles_collision_get_aabb(RID p_particles_collision)
{
	ParticlesCollision* particles_collision =
		particles_collision_owner.get_or_null(p_particles_collision);
	ERR_FAIL_NULL_V(particles_collision, AABB());

	switch (particles_collision->type) {
	case RSE::PARTICLES_COLLISION_TYPE_SPHERE_ATTRACT:
	case RSE::PARTICLES_COLLISION_TYPE_SPHERE_COLLIDE: {
		AABB aabb;
		aabb.position = -Vector3(1, 1, 1) * particles_collision->radius;
		aabb.size = Vector3(2, 2, 2) * particles_collision->radius;
		return aabb;
	}
	default: {
		AABB aabb;
		aabb.position = -particles_collision->extents;
		aabb.size = particles_collision->extents * 2;
		return aabb;
	}
	}
}

Vector3 ParticlesStorage::particles_collision_get_extents(RID p_particles_collision)
{
	const ParticlesCollision* particles_collision =
		particles_collision_owner.get_or_null(p_particles_collision);
	ERR_FAIL_NULL_V(particles_collision, Vector3());
	return particles_collision->extents;
}

bool ParticlesStorage::particles_collision_is_heightfield(RID p_particles_collision)
{
	const ParticlesCollision* particles_collision =
		particles_collision_owner.get_or_null(p_particles_collision);
	ERR_FAIL_NULL_V(particles_collision, false);
	return particles_collision->type == RSE::PARTICLES_COLLISION_TYPE_HEIGHTFIELD_COLLIDE;
}

Dependency* ParticlesStorage::particles_collision_get_dependency(RID p_particles_collision)
{
	ParticlesCollision* pc = particles_collision_owner.get_or_null(p_particles_collision);
	ERR_FAIL_NULL_V(pc, nullptr);

	return &pc->dependency;
}

/* Particles collision instance */

RID ParticlesStorage::particles_collision_instance_create(RID p_collision)
{
	ParticlesCollisionInstance pci;
	pci.collision = p_collision;
	return particles_collision_instance_owner.make_rid(pci);
}

void ParticlesStorage::particles_collision_instance_free(RID p_rid)
{
	particles_collision_instance_owner.free(p_rid);
}

void ParticlesStorage::particles_collision_instance_set_transform(
	RID p_collision_instance, const Transform3D& p_transform)
{
	ParticlesCollisionInstance* pci =
		particles_collision_instance_owner.get_or_null(p_collision_instance);
	ERR_FAIL_NULL(pci);
	pci->transform = p_transform;
}

void ParticlesStorage::particles_collision_instance_set_active(
	RID p_collision_instance, bool p_active)
{
	ParticlesCollisionInstance* pci =
		particles_collision_instance_owner.get_or_null(p_collision_instance);
	ERR_FAIL_NULL(pci);
	pci->active = p_active;
}

void RendererRD::ParticlesStorage::_particles_update_buffers(
	RendererRD::ParticlesStorage::Particles* p_particles)
{
}

void RendererRD::ParticlesStorage::particles_set_view_axis(
	RID p_particles, const Vector3& p_axis, const Vector3& p_up_axis)
{
}

void RendererRD::ParticlesStorage::update_particles() {}

void RendererRD::ParticlesStorage::ParticlesShaderData::set_code(const String& p_code) {}



// ============================================================================
// Merged from servers/rendering/renderer_rd/storage_rd/render_scene_buffers_rd.cpp
// ============================================================================

#include "servers/rendering/renderer_rd/environment/rd_environment.h"
#include "servers/rendering/renderer_rd/storage_rd/render_scene_buffers_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/texture_storage.h"
#include "servers/rendering/rendering_device_binds.h"
#include "servers/rendering/renderer.h" // IWYU pragma: keep // Needed to bind RSE enums.
#include "servers/rendering/rendering_server_enums.h"
#include "servers/rendering/renderer_rd/renderer_scene_render_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/light_storage.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/renderer_rd/storage_rd/mesh_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/particles_storage.h"
#include "servers/rendering/renderer_rd/renderer_compositor_rd.h"

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
