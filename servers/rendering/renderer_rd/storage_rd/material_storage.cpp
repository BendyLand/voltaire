/**************************************************************************/
/*  material_storage.cpp                                                  */
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

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/resource_loader.h"
#include "core/math/projection.h"
#include "core/templates/local_vector.h"
#include "material_storage.h"
#include "servers/rendering/renderer_rd/forward_clustered/scene_shader_forward_clustered.h"
#include "servers/rendering/renderer_rd/forward_mobile/scene_shader_forward_mobile.h"
#include "servers/rendering/renderer_rd/storage_rd/texture_storage.h"
#include "servers/rendering/storage/variant_converters.h"

using namespace RendererRD;

_FORCE_INLINE_ static void _fill_std140_ubo_value(ShaderLanguage::DataType type,
	const Vector<ShaderLanguage::Scalar>& value, uint8_t* data, bool p_use_linear_color)
{
	switch (type) {
	case ShaderLanguage::TYPE_BOOL: {
		uint32_t* gui = (uint32_t*)data;
		gui[0] = value[0].boolean ? 1 : 0;
	} break;
	case ShaderLanguage::TYPE_BVEC2: {
		uint32_t* gui = (uint32_t*)data;
		gui[0] = value[0].boolean ? 1 : 0;
		gui[1] = value[1].boolean ? 1 : 0;

	} break;
	case ShaderLanguage::TYPE_BVEC3: {
		uint32_t* gui = (uint32_t*)data;
		gui[0] = value[0].boolean ? 1 : 0;
		gui[1] = value[1].boolean ? 1 : 0;
		gui[2] = value[2].boolean ? 1 : 0;

	} break;
	case ShaderLanguage::TYPE_BVEC4: {
		uint32_t* gui = (uint32_t*)data;
		gui[0] = value[0].boolean ? 1 : 0;
		gui[1] = value[1].boolean ? 1 : 0;
		gui[2] = value[2].boolean ? 1 : 0;
		gui[3] = value[3].boolean ? 1 : 0;

	} break;
	case ShaderLanguage::TYPE_INT: {
		int32_t* gui = (int32_t*)data;
		gui[0] = value[0].sint;

	} break;
	case ShaderLanguage::TYPE_IVEC2: {
		int32_t* gui = (int32_t*)data;

		for (int i = 0; i < 2; i++) {
			gui[i] = value[i].sint;
		}

	} break;
	case ShaderLanguage::TYPE_IVEC3: {
		int32_t* gui = (int32_t*)data;

		for (int i = 0; i < 3; i++) {
			gui[i] = value[i].sint;
		}

	} break;
	case ShaderLanguage::TYPE_IVEC4: {
		int32_t* gui = (int32_t*)data;

		for (int i = 0; i < 4; i++) {
			gui[i] = value[i].sint;
		}

	} break;
	case ShaderLanguage::TYPE_UINT: {
		uint32_t* gui = (uint32_t*)data;
		gui[0] = value[0].uint;

	} break;
	case ShaderLanguage::TYPE_UVEC2: {
		int32_t* gui = (int32_t*)data;

		for (int i = 0; i < 2; i++) {
			gui[i] = value[i].uint;
		}
	} break;
	case ShaderLanguage::TYPE_UVEC3: {
		int32_t* gui = (int32_t*)data;

		for (int i = 0; i < 3; i++) {
			gui[i] = value[i].uint;
		}

	} break;
	case ShaderLanguage::TYPE_UVEC4: {
		int32_t* gui = (int32_t*)data;

		for (int i = 0; i < 4; i++) {
			gui[i] = value[i].uint;
		}
	} break;
	case ShaderLanguage::TYPE_FLOAT: {
		float* gui = reinterpret_cast<float*>(data);
		gui[0] = value[0].real;

	} break;
	case ShaderLanguage::TYPE_VEC2: {
		float* gui = reinterpret_cast<float*>(data);

		for (int i = 0; i < 2; i++) {
			gui[i] = value[i].real;
		}

	} break;
	case ShaderLanguage::TYPE_VEC3: {
		Color c = Color(value[0].real, value[1].real, value[2].real);
		if (p_use_linear_color) {
			c = c.srgb_to_linear();
		}

		float* gui = reinterpret_cast<float*>(data);

		for (int i = 0; i < 3; i++) {
			gui[i] = c[i];
		}

	} break;
	case ShaderLanguage::TYPE_VEC4: {
		Color c = Color(value[0].real, value[1].real, value[2].real, value[3].real);
		if (p_use_linear_color) {
			c = c.srgb_to_linear();
		}

		float* gui = reinterpret_cast<float*>(data);

		for (int i = 0; i < 4; i++) {
			gui[i] = c[i];
		}
	} break;
	case ShaderLanguage::TYPE_MAT2: {
		float* gui = reinterpret_cast<float*>(data);

		// in std140 members of mat2 are treated as vec4s
		gui[0] = value[0].real;
		gui[1] = value[1].real;
		gui[2] = 0;
		gui[3] = 0;
		gui[4] = value[2].real;
		gui[5] = value[3].real;
		gui[6] = 0;
		gui[7] = 0;
	} break;
	case ShaderLanguage::TYPE_MAT3: {
		float* gui = reinterpret_cast<float*>(data);

		gui[0] = value[0].real;
		gui[1] = value[1].real;
		gui[2] = value[2].real;
		gui[3] = 0;
		gui[4] = value[3].real;
		gui[5] = value[4].real;
		gui[6] = value[5].real;
		gui[7] = 0;
		gui[8] = value[6].real;
		gui[9] = value[7].real;
		gui[10] = value[8].real;
		gui[11] = 0;
	} break;
	case ShaderLanguage::TYPE_MAT4: {
		float* gui = reinterpret_cast<float*>(data);

		for (int i = 0; i < 16; i++) {
			gui[i] = value[i].real;
		}
	} break;
	default: {
	}
	}
}

_FORCE_INLINE_ static void _fill_std140_ubo_empty(
	ShaderLanguage::DataType type, int p_array_size, uint8_t* data)
{
	if (p_array_size <= 0) {
		p_array_size = 1;
	}

	switch (type) {
	case ShaderLanguage::TYPE_BOOL:
	case ShaderLanguage::TYPE_INT:
	case ShaderLanguage::TYPE_UINT:
	case ShaderLanguage::TYPE_FLOAT: {
		memset(data, 0, 4 * p_array_size);
	} break;
	case ShaderLanguage::TYPE_BVEC2:
	case ShaderLanguage::TYPE_IVEC2:
	case ShaderLanguage::TYPE_UVEC2:
	case ShaderLanguage::TYPE_VEC2: {
		memset(data, 0, 8 * p_array_size);
	} break;
	case ShaderLanguage::TYPE_BVEC3:
	case ShaderLanguage::TYPE_IVEC3:
	case ShaderLanguage::TYPE_UVEC3:
	case ShaderLanguage::TYPE_VEC3: {
		memset(data, 0, 12 * p_array_size);
	} break;
	case ShaderLanguage::TYPE_BVEC4:
	case ShaderLanguage::TYPE_IVEC4:
	case ShaderLanguage::TYPE_UVEC4:
	case ShaderLanguage::TYPE_VEC4: {
		memset(data, 0, 16 * p_array_size);
	} break;
	case ShaderLanguage::TYPE_MAT2: {
		memset(data, 0, 32 * p_array_size);
	} break;
	case ShaderLanguage::TYPE_MAT3: {
		memset(data, 0, 48 * p_array_size);
	} break;
	case ShaderLanguage::TYPE_MAT4: {
		memset(data, 0, 64 * p_array_size);
	} break;

	default: {
	}
	}
}

///////////////////////////////////////////////////////////////////////////
// MaterialStorage::ShaderData

void MaterialStorage::ShaderData::set_path_hint(const String& p_hint) { path = p_hint; }

void MaterialStorage::ShaderData::set_default_texture_parameter(
	const StringName& p_name, RID p_texture, int p_index)
{
	if (!p_texture.is_valid()) {
		if (default_texture_params.has(p_name) && default_texture_params[p_name].has(p_index)) {
			default_texture_params[p_name].erase(p_index);

			if (default_texture_params[p_name].is_empty()) {
				default_texture_params.erase(p_name);
			}
		}
	}
	else {
		if (!default_texture_params.has(p_name)) {
			default_texture_params[p_name] = HashMap<int, RID>();
		}
		default_texture_params[p_name][p_index] = p_texture;
	}
}

bool MaterialStorage::ShaderData::is_parameter_texture(const StringName& p_param) const
{
	if (!uniforms.has(p_param)) {
		return false;
	}

	return uniforms[p_param].is_texture();
}

RD::PipelineColorBlendState::Attachment MaterialStorage::ShaderData::blend_mode_to_blend_attachment(
	BlendMode p_mode)
{
	RD::PipelineColorBlendState::Attachment attachment;

	switch (p_mode) {
	case BLEND_MODE_MIX: {
		attachment.enable_blend = true;
		attachment.alpha_blend_op = RD::BLEND_OP_ADD;
		attachment.color_blend_op = RD::BLEND_OP_ADD;
		attachment.src_color_blend_factor = RD::BLEND_FACTOR_SRC_ALPHA;
		attachment.dst_color_blend_factor = RD::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		attachment.src_alpha_blend_factor = RD::BLEND_FACTOR_ONE;
		attachment.dst_alpha_blend_factor = RD::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	} break;
	case BLEND_MODE_ADD: {
		attachment.enable_blend = true;
		attachment.alpha_blend_op = RD::BLEND_OP_ADD;
		attachment.color_blend_op = RD::BLEND_OP_ADD;
		attachment.src_color_blend_factor = RD::BLEND_FACTOR_SRC_ALPHA;
		attachment.dst_color_blend_factor = RD::BLEND_FACTOR_ONE;
		attachment.src_alpha_blend_factor = RD::BLEND_FACTOR_SRC_ALPHA;
		attachment.dst_alpha_blend_factor = RD::BLEND_FACTOR_ONE;
	} break;
	case BLEND_MODE_SUB: {
		attachment.enable_blend = true;
		attachment.alpha_blend_op = RD::BLEND_OP_REVERSE_SUBTRACT;
		attachment.color_blend_op = RD::BLEND_OP_REVERSE_SUBTRACT;
		attachment.src_color_blend_factor = RD::BLEND_FACTOR_SRC_ALPHA;
		attachment.dst_color_blend_factor = RD::BLEND_FACTOR_ONE;
		attachment.src_alpha_blend_factor = RD::BLEND_FACTOR_SRC_ALPHA;
		attachment.dst_alpha_blend_factor = RD::BLEND_FACTOR_ONE;
	} break;
	case BLEND_MODE_MUL: {
		attachment.enable_blend = true;
		attachment.alpha_blend_op = RD::BLEND_OP_ADD;
		attachment.color_blend_op = RD::BLEND_OP_ADD;
		attachment.src_color_blend_factor = RD::BLEND_FACTOR_DST_COLOR;
		attachment.dst_color_blend_factor = RD::BLEND_FACTOR_ZERO;
		attachment.src_alpha_blend_factor = RD::BLEND_FACTOR_DST_ALPHA;
		attachment.dst_alpha_blend_factor = RD::BLEND_FACTOR_ZERO;
	} break;
	case BLEND_MODE_ALPHA_TO_COVERAGE: {
		attachment.enable_blend = true;
		attachment.alpha_blend_op = RD::BLEND_OP_ADD;
		attachment.color_blend_op = RD::BLEND_OP_ADD;
		attachment.src_color_blend_factor = RD::BLEND_FACTOR_SRC_ALPHA;
		attachment.dst_color_blend_factor = RD::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		attachment.src_alpha_blend_factor = RD::BLEND_FACTOR_ONE;
		attachment.dst_alpha_blend_factor = RD::BLEND_FACTOR_ZERO;
	} break;
	case BLEND_MODE_PREMULTIPLIED_ALPHA: {
		attachment.enable_blend = true;
		attachment.alpha_blend_op = RD::BLEND_OP_ADD;
		attachment.color_blend_op = RD::BLEND_OP_ADD;
		attachment.src_color_blend_factor = RD::BLEND_FACTOR_ONE;
		attachment.dst_color_blend_factor = RD::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		attachment.src_alpha_blend_factor = RD::BLEND_FACTOR_ONE;
		attachment.dst_alpha_blend_factor = RD::BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	} break;
	case BLEND_MODE_DISABLED:
	default: {
		// Use default attachment values.
	} break;
	}

	return attachment;
}

bool MaterialStorage::ShaderData::blend_mode_uses_blend_alpha(BlendMode p_mode)
{
	switch (p_mode) {
	case BLEND_MODE_MIX:
		return false;
	case BLEND_MODE_ADD:
		return true;
	case BLEND_MODE_SUB:
		return true;
	case BLEND_MODE_MUL:
		return true;
	case BLEND_MODE_ALPHA_TO_COVERAGE:
		return false;
	case BLEND_MODE_PREMULTIPLIED_ALPHA:
		return true;
	case BLEND_MODE_DISABLED:
	default:
		return false;
	}
}

MaterialStorage::MaterialData::~MaterialData()
{
	MaterialStorage* material_storage = MaterialStorage::get_singleton();

	if (global_buffer_E) {
		// unregister global buffers
		material_storage->global_shader_uniforms.materials_using_buffer.erase(global_buffer_E);
	}

	if (global_texture_E) {
		// unregister global textures

		for (const KeyValue<StringName, uint64_t>& E : used_global_textures) {
			GlobalShaderUniforms::Variable* v =
				material_storage->global_shader_uniforms.variables.getptr(E.key);
			if (v) {
				v->texture_materials.erase(self);
			}
		}
		// unregister material from those using global textures
		material_storage->global_shader_uniforms.materials_using_texture.erase(global_texture_E);
	}

	for (int i = 0; i < 2; i++) {
		if (uniform_buffer[i].is_valid()) {
			RD::get_singleton()->free_rid(uniform_buffer[i]);
		}
	}
}

RID MaterialStorage::MaterialData::get_default_texture_id(
	ShaderLanguage::DataType p_type, ShaderLanguage::ShaderNode::Uniform::Hint p_hint)
{
	TextureStorage* texture_storage = TextureStorage::get_singleton();
	RID rd_texture;

	switch (p_type) {
	case ShaderLanguage::TYPE_ISAMPLER2D:
	case ShaderLanguage::TYPE_USAMPLER2D:
	case ShaderLanguage::TYPE_SAMPLER2D: {
		switch (p_hint) {
		case ShaderLanguage::ShaderNode::Uniform::HINT_DEFAULT_BLACK: {
			rd_texture =
				texture_storage->texture_rd_get_default(TextureStorage::DEFAULT_RD_TEXTURE_BLACK);
		} break;
		case ShaderLanguage::ShaderNode::Uniform::HINT_DEFAULT_TRANSPARENT: {
			rd_texture = texture_storage->texture_rd_get_default(
				TextureStorage::DEFAULT_RD_TEXTURE_TRANSPARENT);
		} break;
		case ShaderLanguage::ShaderNode::Uniform::HINT_ANISOTROPY: {
			rd_texture =
				texture_storage->texture_rd_get_default(TextureStorage::DEFAULT_RD_TEXTURE_ANISO);
		} break;
		case ShaderLanguage::ShaderNode::Uniform::HINT_NORMAL: {
			rd_texture =
				texture_storage->texture_rd_get_default(TextureStorage::DEFAULT_RD_TEXTURE_NORMAL);
		} break;
		case ShaderLanguage::ShaderNode::Uniform::HINT_ROUGHNESS_NORMAL: {
			rd_texture =
				texture_storage->texture_rd_get_default(TextureStorage::DEFAULT_RD_TEXTURE_NORMAL);
		} break;
		default: {
			rd_texture =
				texture_storage->texture_rd_get_default(TextureStorage::DEFAULT_RD_TEXTURE_WHITE);
		} break;
		}
	} break;

	case ShaderLanguage::TYPE_SAMPLERCUBE: {
		switch (p_hint) {
		case ShaderLanguage::ShaderNode::Uniform::HINT_DEFAULT_BLACK: {
			rd_texture = texture_storage->texture_rd_get_default(
				TextureStorage::DEFAULT_RD_TEXTURE_CUBEMAP_BLACK);
		} break;
		case ShaderLanguage::ShaderNode::Uniform::HINT_DEFAULT_TRANSPARENT: {
			rd_texture = texture_storage->texture_rd_get_default(
				TextureStorage::DEFAULT_RD_TEXTURE_CUBEMAP_TRANSPARENT);
		} break;
		default: {
			rd_texture = texture_storage->texture_rd_get_default(
				TextureStorage::DEFAULT_RD_TEXTURE_CUBEMAP_WHITE);
		} break;
		}
	} break;
	case ShaderLanguage::TYPE_SAMPLERCUBEARRAY: {
		switch (p_hint) {
		case ShaderLanguage::ShaderNode::Uniform::HINT_DEFAULT_WHITE: {
			rd_texture = texture_storage->texture_rd_get_default(
				TextureStorage::DEFAULT_RD_TEXTURE_CUBEMAP_ARRAY_WHITE);
		} break;
		case ShaderLanguage::ShaderNode::Uniform::HINT_DEFAULT_TRANSPARENT: {
			rd_texture = texture_storage->texture_rd_get_default(
				TextureStorage::DEFAULT_RD_TEXTURE_CUBEMAP_ARRAY_TRANSPARENT);
		} break;
		default: { // previously this only had the black texture available. Keeping black as the
				   // default to minimize breaking anything.
			rd_texture = texture_storage->texture_rd_get_default(
				TextureStorage::DEFAULT_RD_TEXTURE_CUBEMAP_ARRAY_BLACK);
		} break;
		}
	} break;

	case ShaderLanguage::TYPE_ISAMPLER3D:
	case ShaderLanguage::TYPE_USAMPLER3D:
	case ShaderLanguage::TYPE_SAMPLER3D: {
		switch (p_hint) {
		case ShaderLanguage::ShaderNode::Uniform::HINT_DEFAULT_BLACK: {
			rd_texture = texture_storage->texture_rd_get_default(
				TextureStorage::DEFAULT_RD_TEXTURE_3D_BLACK);
		} break;
		case ShaderLanguage::ShaderNode::Uniform::HINT_DEFAULT_TRANSPARENT: {
			rd_texture = texture_storage->texture_rd_get_default(
				TextureStorage::DEFAULT_RD_TEXTURE_3D_TRANSPARENT);
		} break;
		default: {
			rd_texture = texture_storage->texture_rd_get_default(
				TextureStorage::DEFAULT_RD_TEXTURE_3D_WHITE);
		} break;
		}
	} break;

	case ShaderLanguage::TYPE_ISAMPLER2DARRAY:
	case ShaderLanguage::TYPE_USAMPLER2DARRAY:
	case ShaderLanguage::TYPE_SAMPLER2DARRAY: {
		switch (p_hint) {
		case ShaderLanguage::ShaderNode::Uniform::HINT_DEFAULT_BLACK: {
			rd_texture = texture_storage->texture_rd_get_default(
				TextureStorage::DEFAULT_RD_TEXTURE_2D_ARRAY_BLACK);
		} break;
		case ShaderLanguage::ShaderNode::Uniform::HINT_DEFAULT_TRANSPARENT: {
			rd_texture = texture_storage->texture_rd_get_default(
				TextureStorage::DEFAULT_RD_TEXTURE_2D_ARRAY_TRANSPARENT);
		} break;
		default: {
			rd_texture = texture_storage->texture_rd_get_default(
				TextureStorage::DEFAULT_RD_TEXTURE_2D_ARRAY_WHITE);
		} break;
		}
	} break;

	default: {
	}
	}

	return rd_texture;
}

void MaterialStorage::MaterialData::free_parameters_uniform_set(RID p_uniform_set)
{
	if (p_uniform_set.is_valid() && RD::get_singleton()->uniform_set_is_valid(p_uniform_set)) {
		RD::get_singleton()->uniform_set_set_invalidation_callback(p_uniform_set, nullptr, nullptr);
		RD::get_singleton()->free_rid(p_uniform_set);
	}
}

void MaterialStorage::MaterialData::set_as_used()
{
	for (int i = 0; i < render_target_cache.size(); i++) {
		render_target_cache[i]->was_used = true;
	}
}

/* TextureBlit SHADER */

bool MaterialStorage::TexBlitShaderData::is_animated() const { return false; }

bool MaterialStorage::TexBlitShaderData::casts_shadows() const { return false; }

RenderingServerTypes::ShaderNativeSourceCode
MaterialStorage::TexBlitShaderData::get_native_source_code() const
{
	return TextureStorage::get_singleton()->tex_blit_shader.shader.version_get_native_source_code(
		version);
}

Pair<ShaderRD*, RID> MaterialStorage::TexBlitShaderData::get_native_shader_and_version() const
{
	return {&TextureStorage::get_singleton()->tex_blit_shader.shader, version};
}

MaterialStorage::TexBlitShaderData::TexBlitShaderData() { valid = false; }

MaterialStorage::TexBlitShaderData::~TexBlitShaderData()
{
	if (version.is_valid()) {
		TextureStorage::get_singleton()->tex_blit_shader.shader.version_free(version);
	}
}

MaterialStorage::ShaderData* MaterialStorage::_create_tex_blit_shader_func()
{
	MaterialStorage::TexBlitShaderData* shader_data = memnew(MaterialStorage::TexBlitShaderData);
	return shader_data;
}

MaterialStorage::TexBlitMaterialData::~TexBlitMaterialData()
{
	free_parameters_uniform_set(uniform_set);
}

MaterialStorage::MaterialData* MaterialStorage::_create_tex_blit_material_func(
	MaterialStorage::ShaderData* p_shader)
{
	MaterialStorage::TexBlitMaterialData* material_data = memnew(TexBlitMaterialData);
	material_data->shader_data = static_cast<TexBlitShaderData*>(p_shader);
	// update will happen later anyway so do nothing.
	return material_data;
}

///////////////////////////////////////////////////////////////////////////
// MaterialStorage::Samplers

template void MaterialStorage::Samplers::append_uniforms(
	LocalVector<RD::Uniform>& p_uniforms, int p_first_index) const;

template void MaterialStorage::Samplers::append_uniforms(
	Vector<RD::Uniform>& p_uniforms, int p_first_index) const;

template <typename Collection>
void MaterialStorage::Samplers::append_uniforms(Collection& p_uniforms, int p_first_index) const
{
	// Binding ids are aligned with samplers_inc.glsl.
	p_uniforms.push_back(RD::Uniform(RD::UNIFORM_TYPE_SAMPLER, p_first_index + 0,
		rids[RSE::CANVAS_ITEM_TEXTURE_FILTER_NEAREST][RSE::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED]));
	p_uniforms.push_back(RD::Uniform(RD::UNIFORM_TYPE_SAMPLER, p_first_index + 1,
		rids[RSE::CANVAS_ITEM_TEXTURE_FILTER_LINEAR][RSE::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED]));
	p_uniforms.push_back(RD::Uniform(RD::UNIFORM_TYPE_SAMPLER, p_first_index + 2,
		rids[RSE::CANVAS_ITEM_TEXTURE_FILTER_NEAREST_WITH_MIPMAPS]
			[RSE::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED]));
	p_uniforms.push_back(RD::Uniform(RD::UNIFORM_TYPE_SAMPLER, p_first_index + 3,
		rids[RSE::CANVAS_ITEM_TEXTURE_FILTER_LINEAR_WITH_MIPMAPS]
			[RSE::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED]));
	p_uniforms.push_back(RD::Uniform(RD::UNIFORM_TYPE_SAMPLER, p_first_index + 4,
		rids[RSE::CANVAS_ITEM_TEXTURE_FILTER_NEAREST_WITH_MIPMAPS_ANISOTROPIC]
			[RSE::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED]));
	p_uniforms.push_back(RD::Uniform(RD::UNIFORM_TYPE_SAMPLER, p_first_index + 5,
		rids[RSE::CANVAS_ITEM_TEXTURE_FILTER_LINEAR_WITH_MIPMAPS_ANISOTROPIC]
			[RSE::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED]));
	p_uniforms.push_back(RD::Uniform(RD::UNIFORM_TYPE_SAMPLER, p_first_index + 6,
		rids[RSE::CANVAS_ITEM_TEXTURE_FILTER_NEAREST][RSE::CANVAS_ITEM_TEXTURE_REPEAT_ENABLED]));
	p_uniforms.push_back(RD::Uniform(RD::UNIFORM_TYPE_SAMPLER, p_first_index + 7,
		rids[RSE::CANVAS_ITEM_TEXTURE_FILTER_LINEAR][RSE::CANVAS_ITEM_TEXTURE_REPEAT_ENABLED]));
	p_uniforms.push_back(RD::Uniform(RD::UNIFORM_TYPE_SAMPLER, p_first_index + 8,
		rids[RSE::CANVAS_ITEM_TEXTURE_FILTER_NEAREST_WITH_MIPMAPS]
			[RSE::CANVAS_ITEM_TEXTURE_REPEAT_ENABLED]));
	p_uniforms.push_back(RD::Uniform(RD::UNIFORM_TYPE_SAMPLER, p_first_index + 9,
		rids[RSE::CANVAS_ITEM_TEXTURE_FILTER_LINEAR_WITH_MIPMAPS]
			[RSE::CANVAS_ITEM_TEXTURE_REPEAT_ENABLED]));
	p_uniforms.push_back(RD::Uniform(RD::UNIFORM_TYPE_SAMPLER, p_first_index + 10,
		rids[RSE::CANVAS_ITEM_TEXTURE_FILTER_NEAREST_WITH_MIPMAPS_ANISOTROPIC]
			[RSE::CANVAS_ITEM_TEXTURE_REPEAT_ENABLED]));
	p_uniforms.push_back(RD::Uniform(RD::UNIFORM_TYPE_SAMPLER, p_first_index + 11,
		rids[RSE::CANVAS_ITEM_TEXTURE_FILTER_LINEAR_WITH_MIPMAPS_ANISOTROPIC]
			[RSE::CANVAS_ITEM_TEXTURE_REPEAT_ENABLED]));
}

bool MaterialStorage::Samplers::is_valid() const { return rids[1][1].is_valid(); }

bool MaterialStorage::Samplers::is_null() const { return rids[1][1].is_null(); }

///////////////////////////////////////////////////////////////////////////
// MaterialStorage

MaterialStorage* MaterialStorage::singleton = nullptr;

MaterialStorage* MaterialStorage::get_singleton() { return singleton; }

MaterialStorage::~MaterialStorage()
{
	memdelete_arr(global_shader_uniforms.buffer_values);
	memdelete_arr(global_shader_uniforms.buffer_usage);
	memdelete_arr(global_shader_uniforms.buffer_dirty_regions);
	RD::get_singleton()->free_rid(global_shader_uniforms.buffer);

	// buffers

	RD::get_singleton()->free_rid(quad_index_buffer); // array gets freed as dependency

	// def samplers
	samplers_rd_free(default_samplers);

	singleton = nullptr;
}

bool MaterialStorage::free(RID p_rid)
{
	if (owns_shader(p_rid)) {
		shader_free(p_rid);
		return true;
	}
	else if (owns_material(p_rid)) {
		material_free(p_rid);
		return true;
	}

	return false;
}

/* GLOBAL SHADER UNIFORM API */

int32_t MaterialStorage::_global_shader_uniform_allocate(uint32_t p_elements)
{
	int32_t idx = 0;
	while (idx + p_elements <= global_shader_uniforms.buffer_size) {
		if (global_shader_uniforms.buffer_usage[idx].elements == 0) {
			bool valid = true;
			for (uint32_t i = 1; i < p_elements; i++) {
				if (global_shader_uniforms.buffer_usage[idx + i].elements > 0) {
					valid = false;
					idx += i + global_shader_uniforms.buffer_usage[idx + i].elements;
					break;
				}
			}

			if (!valid) {
				continue; // if not valid, idx is in new position
			}

			return idx;
		}
		else {
			idx += global_shader_uniforms.buffer_usage[idx].elements;
		}
	}

	return -1;
}

void MaterialStorage::_global_shader_uniform_mark_buffer_dirty(int32_t p_index, int32_t p_elements)
{
	int32_t prev_chunk = -1;

	for (int32_t i = 0; i < p_elements; i++) {
		int32_t chunk = (p_index + i) / GlobalShaderUniforms::BUFFER_DIRTY_REGION_SIZE;
		if (chunk != prev_chunk) {
			if (!global_shader_uniforms.buffer_dirty_regions[chunk]) {
				global_shader_uniforms.buffer_dirty_regions[chunk] = true;
				global_shader_uniforms.buffer_dirty_region_count++;
			}
		}

		prev_chunk = chunk;
	}
}

void MaterialStorage::global_shader_parameter_remove(const StringName& p_name)
{
	if (!global_shader_uniforms.variables.has(p_name)) {
		return;
	}
	const GlobalShaderUniforms::Variable& gv = global_shader_uniforms.variables[p_name];

	if (gv.buffer_index >= 0) {
		global_shader_uniforms.buffer_usage[gv.buffer_index].elements = 0;
		global_shader_uniforms.must_update_buffer_materials = true;
	}
	else {
		global_shader_uniforms.must_update_texture_materials = true;
	}

	global_shader_uniforms.variables.erase(p_name);
}

Vector<StringName> MaterialStorage::global_shader_parameter_get_list() const
{
	if (!Engine::get_singleton()->is_editor_hint()) {
		ERR_FAIL_V_MSG(Vector<StringName>(), "This function should never be used outside the "
											 "editor, it can severely damage performance.");
	}

	Vector<StringName> names;
	for (const KeyValue<StringName, GlobalShaderUniforms::Variable>& E :
		global_shader_uniforms.variables) {
		names.push_back(E.key);
	}
	names.sort_custom<StringName::AlphCompare>();
	return names;
}

RSE::GlobalShaderParameterType MaterialStorage::global_shader_parameter_get_type_internal(
	const StringName& p_name) const
{
	if (!global_shader_uniforms.variables.has(p_name)) {
		return RSE::GLOBAL_VAR_TYPE_MAX;
	}

	return global_shader_uniforms.variables[p_name].type;
}

RSE::GlobalShaderParameterType MaterialStorage::global_shader_parameter_get_type(
	const StringName& p_name) const
{
	if (!Engine::get_singleton()->is_editor_hint()) {
		ERR_FAIL_V_MSG(RSE::GLOBAL_VAR_TYPE_MAX, "This function should never be used outside the "
												 "editor, it can severely damage performance.");
	}

	return global_shader_parameter_get_type_internal(p_name);
}

void MaterialStorage::global_shader_parameters_clear()
{
	global_shader_uniforms.variables.clear(); // not right but for now enough
}

RID MaterialStorage::global_shader_uniforms_get_storage_buffer() const
{
	return global_shader_uniforms.buffer;
}

int32_t MaterialStorage::global_shader_parameters_instance_allocate(RID p_instance)
{
	ERR_FAIL_COND_V(global_shader_uniforms.instance_buffer_pos.has(p_instance), -1);
	int32_t pos = _global_shader_uniform_allocate(ShaderLanguage::MAX_INSTANCE_UNIFORM_INDICES);
	global_shader_uniforms.instance_buffer_pos[p_instance] = pos; // save anyway
	ERR_FAIL_COND_V_MSG(pos < 0, -1,
		"Too many instances using shader instance variables. Increase buffer size in Project "
		"Settings.");
	global_shader_uniforms.buffer_usage[pos].elements =
		ShaderLanguage::MAX_INSTANCE_UNIFORM_INDICES;
	return pos;
}

void MaterialStorage::global_shader_parameters_instance_free(RID p_instance)
{
	ERR_FAIL_COND(!global_shader_uniforms.instance_buffer_pos.has(p_instance));
	int32_t pos = global_shader_uniforms.instance_buffer_pos[p_instance];
	if (pos >= 0) {
		global_shader_uniforms.buffer_usage[pos].elements = 0;
	}
	global_shader_uniforms.instance_buffer_pos.erase(p_instance);
}

void MaterialStorage::_update_global_shader_uniforms()
{
	MaterialStorage* material_storage = MaterialStorage::get_singleton();
	if (global_shader_uniforms.buffer_dirty_region_count > 0) {
		uint32_t total_regions = 1 + (global_shader_uniforms.buffer_size /
										 GlobalShaderUniforms::BUFFER_DIRTY_REGION_SIZE);
		if (total_regions / global_shader_uniforms.buffer_dirty_region_count <= 4) {
			// 25% of regions dirty, just update all buffer
			RD::get_singleton()->buffer_update(global_shader_uniforms.buffer, 0,
				sizeof(GlobalShaderUniforms::Value) * global_shader_uniforms.buffer_size,
				global_shader_uniforms.buffer_values);
			memset(global_shader_uniforms.buffer_dirty_regions, 0, sizeof(bool) * total_regions);
		}
		else {
			uint32_t region_byte_size = sizeof(GlobalShaderUniforms::Value) *
										GlobalShaderUniforms::BUFFER_DIRTY_REGION_SIZE;

			for (uint32_t i = 0; i < total_regions; i++) {
				if (global_shader_uniforms.buffer_dirty_regions[i]) {
					RD::get_singleton()->buffer_update(global_shader_uniforms.buffer,
						i * region_byte_size, region_byte_size,
						&global_shader_uniforms
							 .buffer_values[i * GlobalShaderUniforms::BUFFER_DIRTY_REGION_SIZE]);

					global_shader_uniforms.buffer_dirty_regions[i] = false;
				}
			}
		}

		global_shader_uniforms.buffer_dirty_region_count = 0;
	}

	if (global_shader_uniforms.must_update_buffer_materials) {
		// only happens in the case of a buffer variable added or removed,
		// so not often.
		for (const RID& E : global_shader_uniforms.materials_using_buffer) {
			Material* material = material_storage->get_material(E);
			ERR_CONTINUE(!material); // wtf

			material_storage->_material_queue_update(material, true, false);
		}

		global_shader_uniforms.must_update_buffer_materials = false;
	}

	if (global_shader_uniforms.must_update_texture_materials) {
		// only happens in the case of a buffer variable added or removed,
		// so not often.
		for (const RID& E : global_shader_uniforms.materials_using_texture) {
			Material* material = material_storage->get_material(E);
			ERR_CONTINUE(!material); // wtf

			material_storage->_material_queue_update(material, false, true);
		}

		global_shader_uniforms.must_update_texture_materials = false;
	}
}

/* SHADER API */

RID MaterialStorage::shader_allocate() { return shader_owner.allocate_rid(); }

void MaterialStorage::shader_initialize(RID p_rid, bool p_embedded)
{
	Shader shader;
	shader.mutex = memnew(Mutex);
	shader.data = nullptr;
	shader.type = SHADER_TYPE_MAX;
	shader.embedded = p_embedded;

	shader_owner.initialize_rid(p_rid, shader);

	if (p_embedded) {
		// Add to the global embedded set.
		MutexLock lock(embedded_set_mutex);
		embedded_set.insert(p_rid);
	}
}

void MaterialStorage::shader_free(RID p_rid)
{
	Shader* shader = shader_owner.get_or_null(p_rid);
	ERR_FAIL_NULL(shader);

	{
		MutexLock lock(*shader->mutex);

		// make material unreference this
		while (shader->owners.size()) {
			material_set_shader((*shader->owners.begin())->self, RID());
		}

		// clear data if exists
		memdelete(shader->data);
	}

	if (shader->embedded) {
		// Remove from the global embedded set.
		MutexLock lock(embedded_set_mutex);
		embedded_set.erase(p_rid);
	}

	memdelete(shader->mutex);

	shader_owner.free(p_rid);
}

void MaterialStorage::shader_set_code(RID p_shader, const String& p_code)
{
	Shader* shader = shader_owner.get_or_null(p_shader);
	ERR_FAIL_NULL(shader);

	MutexLock lock(*shader->mutex);

	shader->code = p_code;
	String mode_string = ShaderLanguage::get_shader_type(p_code);

	ShaderType new_type;
	if (mode_string == "canvas_item") {
		new_type = SHADER_TYPE_2D;
	}
	else if (mode_string == "particles") {
		new_type = SHADER_TYPE_PARTICLES;
	}
	else if (mode_string == "spatial") {
		new_type = SHADER_TYPE_3D;
	}
	else if (mode_string == "sky") {
		new_type = SHADER_TYPE_SKY;
	}
	else if (mode_string == "fog") {
		new_type = SHADER_TYPE_FOG;
	}
	else if (mode_string == "texture_blit") {
		new_type = SHADER_TYPE_TEXTURE_BLIT;
	}
	else {
		new_type = SHADER_TYPE_MAX;
	}

	if (new_type != shader->type) {
		if (shader->data) {
			memdelete(shader->data);
			shader->data = nullptr;
		}

		for (Material* E : shader->owners) {
			Material* material = E;
			material->shader_type = new_type;
			if (material->data) {
				memdelete(material->data);
				material->data = nullptr;
			}
		}

		shader->type = new_type;

		if (new_type < SHADER_TYPE_MAX && shader_data_request_func[new_type]) {
			shader->data = shader_data_request_func[new_type]();
		}
		else {
			shader->type = SHADER_TYPE_MAX; // invalid
		}

		for (Material* E : shader->owners) {
			Material* material = E;
			if (shader->data) {
				material->data = material_get_data_request_function(new_type)(shader->data);
				material->data->self = material->self;
				material->data->set_next_pass(material->next_pass);
				material->data->set_render_priority(material->priority);
			}
			material->shader_type = new_type;
		}

		if (shader->data) {
			for (const KeyValue<StringName, HashMap<int, RID>>& E :
				shader->default_texture_parameter) {
				for (const KeyValue<int, RID>& E2 : E.value) {
					shader->data->set_default_texture_parameter(E.key, E2.value, E2.key);
				}
			}
		}
	}

	if (shader->data) {
		shader->data->set_path_hint(shader->path_hint);
		shader->data->set_code(p_code);
	}

	for (Material* E : shader->owners) {
		Material* material = E;
		material->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MATERIAL);
		_material_queue_update(material, true, true);
	}
}

void MaterialStorage::shader_set_path_hint(RID p_shader, const String& p_path)
{
	Shader* shader = shader_owner.get_or_null(p_shader);
	ERR_FAIL_NULL(shader);

	shader->path_hint = p_path;
	if (shader->data) {
		shader->data->set_path_hint(p_path);
	}
}

String MaterialStorage::shader_get_code(RID p_shader) const
{
	Shader* shader = shader_owner.get_or_null(p_shader);
	ERR_FAIL_NULL_V(shader, String());
	return shader->code;
}

void MaterialStorage::shader_set_default_texture_parameter(
	RID p_shader, const StringName& p_name, RID p_texture, int p_index)
{
	Shader* shader = shader_owner.get_or_null(p_shader);
	ERR_FAIL_NULL(shader);

	if (p_texture.is_valid() && TextureStorage::get_singleton()->owns_texture(p_texture)) {
		if (!shader->default_texture_parameter.has(p_name)) {
			shader->default_texture_parameter[p_name] = HashMap<int, RID>();
		}
		shader->default_texture_parameter[p_name][p_index] = p_texture;
	}
	else {
		if (shader->default_texture_parameter.has(p_name) &&
			shader->default_texture_parameter[p_name].has(p_index)) {
			shader->default_texture_parameter[p_name].erase(p_index);

			if (shader->default_texture_parameter[p_name].is_empty()) {
				shader->default_texture_parameter.erase(p_name);
			}
		}
	}
	if (shader->data) {
		shader->data->set_default_texture_parameter(p_name, p_texture, p_index);
	}

	{
		MutexLock lock(*shader->mutex);

		for (Material* E : shader->owners) {
			Material* material = E;
			_material_queue_update(material, false, true);
		}
	}
}

RID MaterialStorage::shader_get_default_texture_parameter(
	RID p_shader, const StringName& p_name, int p_index) const
{
	Shader* shader = shader_owner.get_or_null(p_shader);
	ERR_FAIL_NULL_V(shader, RID());
	if (shader->default_texture_parameter.has(p_name) &&
		shader->default_texture_parameter[p_name].has(p_index)) {
		return shader->default_texture_parameter[p_name][p_index];
	}

	return RID();
}

void MaterialStorage::shader_set_data_request_function(
	ShaderType p_shader_type, ShaderDataRequestFunction p_function)
{
	ERR_FAIL_INDEX(p_shader_type, SHADER_TYPE_MAX);
	shader_data_request_func[p_shader_type] = p_function;
}

MaterialStorage::ShaderData* MaterialStorage::shader_get_data(RID p_shader) const
{
	Shader* shader = shader_owner.get_or_null(p_shader);
	ERR_FAIL_NULL_V(shader, nullptr);
	return shader->data;
}

RenderingServerTypes::ShaderNativeSourceCode MaterialStorage::shader_get_native_source_code(
	RID p_shader) const
{
	Shader* shader = shader_owner.get_or_null(p_shader);
	ERR_FAIL_NULL_V(shader, RenderingServerTypes::ShaderNativeSourceCode());
	if (shader->data) {
		return shader->data->get_native_source_code();
	}
	return RenderingServerTypes::ShaderNativeSourceCode();
}

void MaterialStorage::shader_embedded_set_lock() { embedded_set_mutex.lock(); }

const HashSet<RID>& MaterialStorage::shader_embedded_set_get() const { return embedded_set; }

void MaterialStorage::shader_embedded_set_unlock() { embedded_set_mutex.unlock(); }

/* MATERIAL API */

void MaterialStorage::_material_uniform_set_erased(void* p_material)
{
	RID rid = *(RID*)p_material;
	Material* material = MaterialStorage::get_singleton()->get_material(rid);
	if (material) {
		if (material->data) {
			// Uniform set may be gone because a dependency was erased. This happens
			// if a texture is deleted, so re-create it.
			MaterialStorage::get_singleton()->_material_queue_update(material, false, true);
		}
		material->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MATERIAL);
	}
}

void MaterialStorage::_material_queue_update(Material* material, bool p_uniform, bool p_texture)
{
	MutexLock lock(material_update_list_mutex);
	material->uniform_dirty = material->uniform_dirty || p_uniform;
	material->texture_dirty = material->texture_dirty || p_texture;

	if (material->update_element.in_list()) {
		return;
	}

	material_update_list.add(&material->update_element);
}

RID MaterialStorage::material_allocate() { return material_owner.allocate_rid(); }

void MaterialStorage::material_initialize(RID p_rid)
{
	material_owner.initialize_rid(p_rid);
	Material* material = material_owner.get_or_null(p_rid);
	material->self = p_rid;
}

void MaterialStorage::material_set_shader(RID p_material, RID p_shader)
{
	Material* material = material_owner.get_or_null(p_material);
	ERR_FAIL_NULL(material);

	if (material->data) {
		memdelete(material->data);
		material->data = nullptr;
	}

	if (material->shader) {
		{
			MutexLock lock(*material->shader->mutex);
			material->shader->owners.erase(material);
		}

		material->shader = nullptr;
		material->shader_type = SHADER_TYPE_MAX;
	}

	if (p_shader.is_null()) {
		material->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MATERIAL);
		material->shader_id = 0;
		return;
	}

	Shader* shader = get_shader(p_shader);
	ERR_FAIL_NULL(shader);

	MutexLock lock(*shader->mutex);

	material->shader = shader;
	material->shader_type = shader->type;
	material->shader_id = p_shader.get_local_index();
	shader->owners.insert(material);

	if (shader->type == SHADER_TYPE_MAX) {
		return;
	}

	ERR_FAIL_NULL(shader->data);

	material->data = material_data_request_func[shader->type](shader->data);
	material->data->self = p_material;
	material->data->set_next_pass(material->next_pass);
	material->data->set_render_priority(material->priority);
	// updating happens later
	material->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MATERIAL);
	_material_queue_update(material, true, true);
}

MaterialStorage::ShaderData* MaterialStorage::material_get_shader_data(RID p_material)
{
	const MaterialStorage::Material* material =
		MaterialStorage::get_singleton()->get_material(p_material);
	if (material && material->shader && material->shader->data) {
		return material->shader->data;
	}

	return nullptr;
}

void MaterialStorage::material_set_next_pass(RID p_material, RID p_next_material)
{
	Material* material = material_owner.get_or_null(p_material);
	ERR_FAIL_NULL(material);

	if (material->next_pass == p_next_material) {
		return;
	}

	material->next_pass = p_next_material;
	if (material->data) {
		material->data->set_next_pass(p_next_material);
	}

	material->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MATERIAL);
}

void MaterialStorage::material_set_render_priority(RID p_material, int priority)
{
	Material* material = material_owner.get_or_null(p_material);
	ERR_FAIL_NULL(material);
	material->priority = priority;
	if (material->data) {
		material->data->set_render_priority(priority);
	}
	material->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MATERIAL);
}

bool MaterialStorage::material_is_animated(RID p_material)
{
	Material* material = material_owner.get_or_null(p_material);
	ERR_FAIL_NULL_V(material, false);
	if (material->shader && material->shader->data) {
		if (material->shader->data->is_animated()) {
			return true;
		}
		else if (material->next_pass.is_valid()) {
			return material_is_animated(material->next_pass);
		}
	}
	return false; // by default nothing is animated
}

bool MaterialStorage::material_casts_shadows(RID p_material)
{
	Material* material = material_owner.get_or_null(p_material);
	ERR_FAIL_NULL_V(material, true);
	if (material->shader && material->shader->data) {
		if (material->shader->data->casts_shadows()) {
			return true;
		}
		else if (material->next_pass.is_valid()) {
			return material_casts_shadows(material->next_pass);
		}
	}
	return true; // by default everything casts shadows
}

RSE::CullMode RendererRD::MaterialStorage::material_get_cull_mode(RID p_material) const
{
	Material* material = material_owner.get_or_null(p_material);
	ERR_FAIL_NULL_V(material, RSE::CULL_MODE_DISABLED);
	ERR_FAIL_NULL_V(material->shader, RSE::CULL_MODE_DISABLED);
	if (material->shader->type == ShaderType::SHADER_TYPE_3D && material->shader->data) {
		RendererSceneRenderImplementation::SceneShaderForwardClustered::ShaderData* sd_clustered =
			dynamic_cast<
				RendererSceneRenderImplementation::SceneShaderForwardClustered::ShaderData*>(
				material->shader->data);
		if (sd_clustered) {
			return (RSE::CullMode)sd_clustered->cull_mode;
		}

		RendererSceneRenderImplementation::SceneShaderForwardMobile::ShaderData* sd_mobile =
			dynamic_cast<RendererSceneRenderImplementation::SceneShaderForwardMobile::ShaderData*>(
				material->shader->data);
		if (sd_mobile) {
			return (RSE::CullMode)sd_mobile->cull_mode;
		}
	}
	return RSE::CULL_MODE_DISABLED;
}

void MaterialStorage::material_get_instance_shader_parameters(
	RID p_material, List<InstanceShaderParam>* r_parameters)
{
	Material* material = material_owner.get_or_null(p_material);
	ERR_FAIL_NULL(material);
	if (material->shader && material->shader->data) {
		material->shader->data->get_instance_param_list(r_parameters);

		if (material->next_pass.is_valid()) {
			material_get_instance_shader_parameters(material->next_pass, r_parameters);
		}
	}
}

void MaterialStorage::material_update_dependency(RID p_material, DependencyTracker* p_instance)
{
	Material* material = material_owner.get_or_null(p_material);
	ERR_FAIL_NULL(material);
	p_instance->update_dependency(&material->dependency);
	if (material->next_pass.is_valid()) {
		material_update_dependency(material->next_pass, p_instance);
	}
}

MaterialStorage::Samplers MaterialStorage::samplers_rd_allocate(
	float p_mipmap_bias, RSE::ViewportAnisotropicFiltering anisotropic_filtering_level) const
{
	Samplers samplers;
	samplers.mipmap_bias = p_mipmap_bias;
	samplers.anisotropic_filtering_level = (int)anisotropic_filtering_level;
	samplers.use_nearest_mipmap_filter =
		GLOBAL_GET_CACHED(bool, "rendering/textures/default_filters/use_nearest_mipmap_filter");

	RD::SamplerFilter mip_filter =
		samplers.use_nearest_mipmap_filter ? RD::SAMPLER_FILTER_NEAREST : RD::SAMPLER_FILTER_LINEAR;
	float anisotropy_max = float(1 << samplers.anisotropic_filtering_level);

	for (int i = 1; i < RSE::CANVAS_ITEM_TEXTURE_FILTER_MAX; i++) {
		for (int j = 1; j < RSE::CANVAS_ITEM_TEXTURE_REPEAT_MAX; j++) {
			RD::SamplerState sampler_state;
			switch (i) {
			case RSE::CANVAS_ITEM_TEXTURE_FILTER_NEAREST: {
				sampler_state.mag_filter = RD::SAMPLER_FILTER_NEAREST;
				sampler_state.min_filter = RD::SAMPLER_FILTER_NEAREST;
				sampler_state.max_lod = 0;
			} break;
			case RSE::CANVAS_ITEM_TEXTURE_FILTER_LINEAR: {
				sampler_state.mag_filter = RD::SAMPLER_FILTER_LINEAR;
				sampler_state.min_filter = RD::SAMPLER_FILTER_LINEAR;
				sampler_state.max_lod = 0;
			} break;
			case RSE::CANVAS_ITEM_TEXTURE_FILTER_NEAREST_WITH_MIPMAPS: {
				sampler_state.mag_filter = RD::SAMPLER_FILTER_NEAREST;
				sampler_state.min_filter = RD::SAMPLER_FILTER_NEAREST;
				sampler_state.mip_filter = mip_filter;
				sampler_state.lod_bias = samplers.mipmap_bias;
			} break;
			case RSE::CANVAS_ITEM_TEXTURE_FILTER_LINEAR_WITH_MIPMAPS: {
				sampler_state.mag_filter = RD::SAMPLER_FILTER_LINEAR;
				sampler_state.min_filter = RD::SAMPLER_FILTER_LINEAR;
				sampler_state.mip_filter = mip_filter;
				sampler_state.lod_bias = samplers.mipmap_bias;

			} break;
			case RSE::CANVAS_ITEM_TEXTURE_FILTER_NEAREST_WITH_MIPMAPS_ANISOTROPIC: {
				sampler_state.mag_filter = RD::SAMPLER_FILTER_NEAREST;
				sampler_state.min_filter = RD::SAMPLER_FILTER_NEAREST;
				sampler_state.mip_filter = mip_filter;
				sampler_state.lod_bias = samplers.mipmap_bias;
				sampler_state.use_anisotropy = true;
				sampler_state.anisotropy_max = anisotropy_max;
			} break;
			case RSE::CANVAS_ITEM_TEXTURE_FILTER_LINEAR_WITH_MIPMAPS_ANISOTROPIC: {
				sampler_state.mag_filter = RD::SAMPLER_FILTER_LINEAR;
				sampler_state.min_filter = RD::SAMPLER_FILTER_LINEAR;
				sampler_state.mip_filter = mip_filter;
				sampler_state.lod_bias = samplers.mipmap_bias;
				sampler_state.use_anisotropy = true;
				sampler_state.anisotropy_max = anisotropy_max;

			} break;
			default: {
			}
			}
			switch (j) {
			case RSE::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED: {
				sampler_state.repeat_u = RD::SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE;
				sampler_state.repeat_v = RD::SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE;
				sampler_state.repeat_w = RD::SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE;

			} break;
			case RSE::CANVAS_ITEM_TEXTURE_REPEAT_ENABLED: {
				sampler_state.repeat_u = RD::SAMPLER_REPEAT_MODE_REPEAT;
				sampler_state.repeat_v = RD::SAMPLER_REPEAT_MODE_REPEAT;
				sampler_state.repeat_w = RD::SAMPLER_REPEAT_MODE_REPEAT;
			} break;
			case RSE::CANVAS_ITEM_TEXTURE_REPEAT_MIRROR: {
				sampler_state.repeat_u = RD::SAMPLER_REPEAT_MODE_MIRRORED_REPEAT;
				sampler_state.repeat_v = RD::SAMPLER_REPEAT_MODE_MIRRORED_REPEAT;
				sampler_state.repeat_w = RD::SAMPLER_REPEAT_MODE_MIRRORED_REPEAT;
			} break;
			default: {
			}
			}

			samplers.rids[i][j] = RD::get_singleton()->sampler_create(sampler_state);
		}
	}

	return samplers;
}

void MaterialStorage::samplers_rd_free(Samplers& p_samplers) const
{
	for (int i = 1; i < RSE::CANVAS_ITEM_TEXTURE_FILTER_MAX; i++) {
		for (int j = 1; j < RSE::CANVAS_ITEM_TEXTURE_REPEAT_MAX; j++) {
			if (p_samplers.rids[i][j].is_valid()) {
				RD::get_singleton()->free_rid(p_samplers.rids[i][j]);
				p_samplers.rids[i][j] = RID();
			}
		}
	}
}

void MaterialStorage::material_set_data_request_function(
	ShaderType p_shader_type, MaterialStorage::MaterialDataRequestFunction p_function)
{
	ERR_FAIL_INDEX(p_shader_type, SHADER_TYPE_MAX);
	material_data_request_func[p_shader_type] = p_function;
}

MaterialStorage::MaterialDataRequestFunction MaterialStorage::material_get_data_request_function(
	ShaderType p_shader_type)
{
	ERR_FAIL_INDEX_V(p_shader_type, SHADER_TYPE_MAX, nullptr);
	return material_data_request_func[p_shader_type];
}


