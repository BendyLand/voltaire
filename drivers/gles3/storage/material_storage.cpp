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

#include "material_storage.h"

#ifdef GLES3_ENABLED

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/resource_loader.h"
#include "drivers/gles3/rasterizer_canvas_gles3.h"
#include "drivers/gles3/rasterizer_gles3.h"
#include "drivers/gles3/storage/config.h"
#include "drivers/gles3/storage/texture_storage.h"
#include "servers/rendering/rendering_server_types.h"
#include "servers/rendering/storage/variant_converters.h"

using namespace GLES3;

_FORCE_INLINE_ static void _fill_std140_ubo_value(
	ShaderLanguage::DataType type, const Vector<ShaderLanguage::Scalar>& value, uint8_t* data)
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
		float* gui = (float*)data;
		gui[0] = value[0].real;

	} break;
	case ShaderLanguage::TYPE_VEC2: {
		float* gui = (float*)data;

		for (int i = 0; i < 2; i++) {
			gui[i] = value[i].real;
		}

	} break;
	case ShaderLanguage::TYPE_VEC3: {
		float* gui = (float*)data;

		for (int i = 0; i < 3; i++) {
			gui[i] = value[i].real;
		}

	} break;
	case ShaderLanguage::TYPE_VEC4: {
		float* gui = (float*)data;

		for (int i = 0; i < 4; i++) {
			gui[i] = value[i].real;
		}
	} break;
	case ShaderLanguage::TYPE_MAT2: {
		float* gui = (float*)data;

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
		float* gui = (float*)data;

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
		float* gui = (float*)data;

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

void ShaderData::get_instance_param_list(
	List<InstanceShaderParam>* p_param_list) const
{
}

void ShaderData::set_path_hint(const String& p_hint) { path = p_hint; }

void ShaderData::set_default_texture_parameter(const StringName& p_name, RID p_texture, int p_index)
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

bool ShaderData::is_parameter_texture(const StringName& p_param) const
{
	if (!uniforms.has(p_param)) {
		return false;
	}

	return uniforms[p_param].is_texture();
}

// Look up table to translate ShaderLanguage::DataType to GL_TEXTURE_*
static const GLenum target_from_type[ShaderLanguage::TYPE_MAX] = {
	GL_TEXTURE_2D,			  // TYPE_VOID,
	GL_TEXTURE_2D,			  // TYPE_BOOL,
	GL_TEXTURE_2D,			  // TYPE_BVEC2,
	GL_TEXTURE_2D,			  // TYPE_BVEC3,
	GL_TEXTURE_2D,			  // TYPE_BVEC4,
	GL_TEXTURE_2D,			  // TYPE_INT,
	GL_TEXTURE_2D,			  // TYPE_IVEC2,
	GL_TEXTURE_2D,			  // TYPE_IVEC3,
	GL_TEXTURE_2D,			  // TYPE_IVEC4,
	GL_TEXTURE_2D,			  // TYPE_UINT,
	GL_TEXTURE_2D,			  // TYPE_UVEC2,
	GL_TEXTURE_2D,			  // TYPE_UVEC3,
	GL_TEXTURE_2D,			  // TYPE_UVEC4,
	GL_TEXTURE_2D,			  // TYPE_FLOAT,
	GL_TEXTURE_2D,			  // TYPE_VEC2,
	GL_TEXTURE_2D,			  // TYPE_VEC3,
	GL_TEXTURE_2D,			  // TYPE_VEC4,
	GL_TEXTURE_2D,			  // TYPE_MAT2,
	GL_TEXTURE_2D,			  // TYPE_MAT3,
	GL_TEXTURE_2D,			  // TYPE_MAT4,
	GL_TEXTURE_2D,			  // TYPE_SAMPLER2D,
	GL_TEXTURE_2D,			  // TYPE_ISAMPLER2D,
	GL_TEXTURE_2D,			  // TYPE_USAMPLER2D,
	GL_TEXTURE_2D_ARRAY,	  // TYPE_SAMPLER2DARRAY,
	GL_TEXTURE_2D_ARRAY,	  // TYPE_ISAMPLER2DARRAY,
	GL_TEXTURE_2D_ARRAY,	  // TYPE_USAMPLER2DARRAY,
	GL_TEXTURE_3D,			  // TYPE_SAMPLER3D,
	GL_TEXTURE_3D,			  // TYPE_ISAMPLER3D,
	GL_TEXTURE_3D,			  // TYPE_USAMPLER3D,
	GL_TEXTURE_CUBE_MAP,	  // TYPE_SAMPLERCUBE,
	GL_TEXTURE_CUBE_MAP,	  // TYPE_SAMPLERCUBEARRAY,
	_GL_TEXTURE_EXTERNAL_OES, // TYPE_SAMPLEREXT
	GL_TEXTURE_2D,			  // TYPE_STRUCT
};

static const RSE::CanvasItemTextureRepeat repeat_from_uniform[ShaderLanguage::REPEAT_DEFAULT + 1] =
	{
		RSE::CanvasItemTextureRepeat::
			CANVAS_ITEM_TEXTURE_REPEAT_DISABLED, // ShaderLanguage::TextureRepeat::REPEAT_DISABLE,
		RSE::CanvasItemTextureRepeat::
			CANVAS_ITEM_TEXTURE_REPEAT_ENABLED, // ShaderLanguage::TextureRepeat::REPEAT_ENABLE,
		RSE::CanvasItemTextureRepeat::
			CANVAS_ITEM_TEXTURE_REPEAT_ENABLED, // ShaderLanguage::TextureRepeat::REPEAT_DEFAULT,
};

static const RSE::CanvasItemTextureRepeat
	repeat_from_uniform_canvas[ShaderLanguage::REPEAT_DEFAULT + 1] = {
		RSE::CanvasItemTextureRepeat::
			CANVAS_ITEM_TEXTURE_REPEAT_DISABLED, // ShaderLanguage::TextureRepeat::REPEAT_DISABLE,
		RSE::CanvasItemTextureRepeat::
			CANVAS_ITEM_TEXTURE_REPEAT_ENABLED, // ShaderLanguage::TextureRepeat::REPEAT_ENABLE,
		RSE::CanvasItemTextureRepeat::
			CANVAS_ITEM_TEXTURE_REPEAT_DISABLED, // ShaderLanguage::TextureRepeat::REPEAT_DEFAULT,
};

static const RSE::CanvasItemTextureFilter filter_from_uniform[ShaderLanguage::FILTER_DEFAULT +
															  1] = {
	RSE::CanvasItemTextureFilter::
		CANVAS_ITEM_TEXTURE_FILTER_NEAREST, // ShaderLanguage::TextureFilter::FILTER_NEAREST,
	RSE::CanvasItemTextureFilter::
		CANVAS_ITEM_TEXTURE_FILTER_LINEAR, // ShaderLanguage::TextureFilter::FILTER_LINEAR,
	RSE::CanvasItemTextureFilter::
		CANVAS_ITEM_TEXTURE_FILTER_NEAREST_WITH_MIPMAPS, // ShaderLanguage::TextureFilter::FILTER_NEAREST_MIPMAP,
	RSE::CanvasItemTextureFilter::
		CANVAS_ITEM_TEXTURE_FILTER_LINEAR_WITH_MIPMAPS, // ShaderLanguage::TextureFilter::FILTER_LINEAR_MIPMAP,
	RSE::CanvasItemTextureFilter::
		CANVAS_ITEM_TEXTURE_FILTER_NEAREST_WITH_MIPMAPS_ANISOTROPIC, // ShaderLanguage::TextureFilter::FILTER_NEAREST_MIPMAP_ANISOTROPIC,
	RSE::CanvasItemTextureFilter::
		CANVAS_ITEM_TEXTURE_FILTER_LINEAR_WITH_MIPMAPS_ANISOTROPIC, // ShaderLanguage::TextureFilter::FILTER_LINEAR_MIPMAP_ANISOTROPIC,
	RSE::CanvasItemTextureFilter::
		CANVAS_ITEM_TEXTURE_FILTER_LINEAR_WITH_MIPMAPS, // ShaderLanguage::TextureFilter::FILTER_DEFAULT,
};

static const RSE::CanvasItemTextureFilter filter_from_uniform_canvas[ShaderLanguage::
																		 FILTER_DEFAULT +
																	 1] = {
	RSE::CanvasItemTextureFilter::
		CANVAS_ITEM_TEXTURE_FILTER_NEAREST, // ShaderLanguage::TextureFilter::FILTER_NEAREST,
	RSE::CanvasItemTextureFilter::
		CANVAS_ITEM_TEXTURE_FILTER_LINEAR, // ShaderLanguage::TextureFilter::FILTER_LINEAR,
	RSE::CanvasItemTextureFilter::
		CANVAS_ITEM_TEXTURE_FILTER_NEAREST_WITH_MIPMAPS, // ShaderLanguage::TextureFilter::FILTER_NEAREST_MIPMAP,
	RSE::CanvasItemTextureFilter::
		CANVAS_ITEM_TEXTURE_FILTER_LINEAR_WITH_MIPMAPS, // ShaderLanguage::TextureFilter::FILTER_LINEAR_MIPMAP,
	RSE::CanvasItemTextureFilter::
		CANVAS_ITEM_TEXTURE_FILTER_NEAREST_WITH_MIPMAPS_ANISOTROPIC, // ShaderLanguage::TextureFilter::FILTER_NEAREST_MIPMAP_ANISOTROPIC,
	RSE::CanvasItemTextureFilter::
		CANVAS_ITEM_TEXTURE_FILTER_LINEAR_WITH_MIPMAPS_ANISOTROPIC, // ShaderLanguage::TextureFilter::FILTER_LINEAR_MIPMAP_ANISOTROPIC,
	RSE::CanvasItemTextureFilter::
		CANVAS_ITEM_TEXTURE_FILTER_LINEAR, // ShaderLanguage::TextureFilter::FILTER_DEFAULT,
};

MaterialData::~MaterialData()
{
	if (global_buffer_E) {
		// unregister global buffers
		MaterialStorage::global_shader_uniforms.materials_using_buffer.erase(global_buffer_E);
	}

	if (global_texture_E) {
		// unregister global textures

		for (const KeyValue<StringName, uint64_t>& E : used_global_textures) {
			GlobalShaderUniforms::Variable* v =
				MaterialStorage::global_shader_uniforms.variables.getptr(E.key);
			if (v) {
				v->texture_materials.erase(self);
			}
		}
		// unregister material from those using global textures
		MaterialStorage::global_shader_uniforms.materials_using_texture.erase(global_texture_E);
	}

	if (uniform_buffer) {
		glDeleteBuffers(1, &uniform_buffer);
		uniform_buffer = 0;
	}
}

RID MaterialData::get_default_texture_id(
	ShaderLanguage::DataType p_type, ShaderLanguage::ShaderNode::Uniform::Hint p_hint)
{
	RID gl_texture;

	switch (p_type) {
	case ShaderLanguage::TYPE_ISAMPLER2D:
	case ShaderLanguage::TYPE_USAMPLER2D:
	case ShaderLanguage::TYPE_SAMPLER2D: {
		switch (p_hint) {
		case ShaderLanguage::ShaderNode::Uniform::HINT_DEFAULT_BLACK: {
			gl_texture = GLES3::TextureStorage::texture_gl_get_default(DEFAULT_GL_TEXTURE_BLACK);
		} break;
		case ShaderLanguage::ShaderNode::Uniform::HINT_DEFAULT_TRANSPARENT: {
			gl_texture = GLES3::TextureStorage::texture_gl_get_default(DEFAULT_GL_TEXTURE_TRANSPARENT);
		} break;
		case ShaderLanguage::ShaderNode::Uniform::HINT_ANISOTROPY: {
			gl_texture = GLES3::TextureStorage::texture_gl_get_default(DEFAULT_GL_TEXTURE_ANISO);
		} break;
		case ShaderLanguage::ShaderNode::Uniform::HINT_NORMAL: {
			gl_texture = GLES3::TextureStorage::texture_gl_get_default(DEFAULT_GL_TEXTURE_NORMAL);
		} break;
		case ShaderLanguage::ShaderNode::Uniform::HINT_ROUGHNESS_NORMAL: {
			gl_texture = GLES3::TextureStorage::texture_gl_get_default(DEFAULT_GL_TEXTURE_NORMAL);
		} break;
		default: {
			gl_texture = GLES3::TextureStorage::texture_gl_get_default(DEFAULT_GL_TEXTURE_WHITE);
		} break;
		}
	} break;

	case ShaderLanguage::TYPE_SAMPLERCUBE: {
		switch (p_hint) {
		case ShaderLanguage::ShaderNode::Uniform::HINT_DEFAULT_BLACK: {
			gl_texture = GLES3::TextureStorage::texture_gl_get_default(DEFAULT_GL_TEXTURE_CUBEMAP_BLACK);
		} break;
		case ShaderLanguage::ShaderNode::Uniform::HINT_DEFAULT_TRANSPARENT: {
			gl_texture =
				GLES3::TextureStorage::texture_gl_get_default(DEFAULT_GL_TEXTURE_CUBEMAP_TRANSPARENT);
		} break;
		default: {
			gl_texture = GLES3::TextureStorage::texture_gl_get_default(DEFAULT_GL_TEXTURE_CUBEMAP_WHITE);
		} break;
		}
	} break;
	case ShaderLanguage::TYPE_SAMPLERCUBEARRAY: {
		ERR_PRINT_ONCE("Type: SamplerCubeArray is not supported in the Compatibility renderer, "
					   "please use another type.");
	} break;
	case ShaderLanguage::TYPE_SAMPLEREXT: {
		gl_texture = GLES3::TextureStorage::texture_gl_get_default(DEFAULT_GL_TEXTURE_EXT);
	} break;

	case ShaderLanguage::TYPE_ISAMPLER3D:
	case ShaderLanguage::TYPE_USAMPLER3D:
	case ShaderLanguage::TYPE_SAMPLER3D: {
		switch (p_hint) {
		case ShaderLanguage::ShaderNode::Uniform::HINT_DEFAULT_BLACK: {
			gl_texture = GLES3::TextureStorage::texture_gl_get_default(DEFAULT_GL_TEXTURE_3D_BLACK);
		} break;
		case ShaderLanguage::ShaderNode::Uniform::HINT_DEFAULT_TRANSPARENT: {
			gl_texture = GLES3::TextureStorage::texture_gl_get_default(DEFAULT_GL_TEXTURE_3D_TRANSPARENT);
		} break;
		default: {
			gl_texture = GLES3::TextureStorage::texture_gl_get_default(DEFAULT_GL_TEXTURE_3D_WHITE);
		} break;
		}
	} break;

	case ShaderLanguage::TYPE_ISAMPLER2DARRAY:
	case ShaderLanguage::TYPE_USAMPLER2DARRAY:
	case ShaderLanguage::TYPE_SAMPLER2DARRAY: {
		switch (p_hint) {
		case ShaderLanguage::ShaderNode::Uniform::HINT_DEFAULT_BLACK: {
			gl_texture = GLES3::TextureStorage::texture_gl_get_default(DEFAULT_GL_TEXTURE_2D_ARRAY_BLACK);
		} break;
		case ShaderLanguage::ShaderNode::Uniform::HINT_DEFAULT_TRANSPARENT: {
			gl_texture =
				GLES3::TextureStorage::texture_gl_get_default(DEFAULT_GL_TEXTURE_2D_ARRAY_TRANSPARENT);
		} break;
		default: {
			gl_texture = GLES3::TextureStorage::texture_gl_get_default(DEFAULT_GL_TEXTURE_2D_ARRAY_WHITE);
		} break;
		}
	} break;

	default: {
	}
	}

	return gl_texture;
}

int32_t GLES3::MaterialStorage::_global_shader_uniform_allocate(uint32_t p_elements)
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

void GLES3::MaterialStorage::_global_shader_uniform_mark_buffer_dirty(int32_t p_index, int32_t p_elements)
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

void GLES3::MaterialStorage::global_shader_parameter_remove(const StringName& p_name)
{
	if (!global_shader_uniforms.variables.has(p_name)) {
		return;
	}
	GlobalShaderUniforms::Variable& gv = global_shader_uniforms.variables[p_name];

	if (gv.buffer_index >= 0) {
		global_shader_uniforms.buffer_usage[gv.buffer_index].elements = 0;
		global_shader_uniforms.must_update_buffer_materials = true;
	}
	else {
		global_shader_uniforms.must_update_texture_materials = true;
	}

	global_shader_uniforms.variables.erase(p_name);
}

Vector<StringName> GLES3::MaterialStorage::global_shader_parameter_get_list()
{
	if (!Engine::is_editor_hint() &&
		!Engine::is_project_manager_hint()) {
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

RSE::GlobalShaderParameterType GLES3::MaterialStorage::global_shader_parameter_get_type_internal(
	const StringName& p_name)
{
	if (!global_shader_uniforms.variables.has(p_name)) {
		return RSE::GLOBAL_VAR_TYPE_MAX;
	}

	return global_shader_uniforms.variables[p_name].type;
}

RSE::GlobalShaderParameterType GLES3::MaterialStorage::global_shader_parameter_get_type(
	const StringName& p_name)
{
	if (!Engine::is_editor_hint()) {
		ERR_FAIL_V_MSG(RSE::GLOBAL_VAR_TYPE_MAX, "This function should never be used outside the "
												 "editor, it can severely damage performance.");
	}

	return global_shader_parameter_get_type_internal(p_name);
}

void GLES3::MaterialStorage::global_shader_parameters_clear() { global_shader_uniforms.variables.clear(); }

GLuint GLES3::MaterialStorage::global_shader_parameters_get_uniform_buffer()
{
	return global_shader_uniforms.buffer;
}

int32_t GLES3::MaterialStorage::global_shader_parameters_instance_allocate(RID p_instance)
{
	ERR_FAIL_COND_V(global_shader_uniforms.instance_buffer_pos.has(p_instance), -1);
	int32_t pos = _global_shader_uniform_allocate(ShaderLanguage::MAX_INSTANCE_UNIFORM_INDICES);
	global_shader_uniforms.instance_buffer_pos[p_instance] = pos; // save anyway
	ERR_FAIL_COND_V_MSG(pos < 0, -1,
		vformat("Too many instances using shader instance variables. Consider increasing "
				"rendering/limits/global_shader_variables/buffer_size in the Project Settings. "
				"Maximum items supported by this hardware is: %d.",
			Config::get_singleton()->max_uniform_buffer_size /
				sizeof(GlobalShaderUniforms::Value)));
	global_shader_uniforms.buffer_usage[pos].elements =
		ShaderLanguage::MAX_INSTANCE_UNIFORM_INDICES;
	return pos;
}

void GLES3::MaterialStorage::global_shader_parameters_instance_free(RID p_instance)
{
	ERR_FAIL_COND(!global_shader_uniforms.instance_buffer_pos.has(p_instance));
	int32_t pos = global_shader_uniforms.instance_buffer_pos[p_instance];
	if (pos >= 0) {
		global_shader_uniforms.buffer_usage[pos].elements = 0;
	}
	global_shader_uniforms.instance_buffer_pos.erase(p_instance);
}

void GLES3::MaterialStorage::_update_global_shader_uniforms()
{
	if (global_shader_uniforms.buffer_dirty_region_count > 0) {
		uint32_t total_regions = 1 + (global_shader_uniforms.buffer_size /
										 GlobalShaderUniforms::BUFFER_DIRTY_REGION_SIZE);
		if (total_regions / global_shader_uniforms.buffer_dirty_region_count <= 4) {
			// 25% of regions dirty, just update all buffer
			glBindBuffer(GL_UNIFORM_BUFFER, global_shader_uniforms.buffer);
			glBufferData(GL_UNIFORM_BUFFER,
				sizeof(GlobalShaderUniforms::Value) * global_shader_uniforms.buffer_size,
				global_shader_uniforms.buffer_values, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);
			memset(global_shader_uniforms.buffer_dirty_regions, 0, sizeof(bool) * total_regions);
		}
		else {
			uint32_t region_byte_size = sizeof(GlobalShaderUniforms::Value) *
										GlobalShaderUniforms::BUFFER_DIRTY_REGION_SIZE;
			glBindBuffer(GL_UNIFORM_BUFFER, global_shader_uniforms.buffer);
			for (uint32_t i = 0; i < total_regions; i++) {
				if (global_shader_uniforms.buffer_dirty_regions[i]) {
					glBufferSubData(GL_UNIFORM_BUFFER, i * region_byte_size, region_byte_size,
						&global_shader_uniforms
							 .buffer_values[i * GlobalShaderUniforms::BUFFER_DIRTY_REGION_SIZE]);
					global_shader_uniforms.buffer_dirty_regions[i] = false;
				}
			}
			glBindBuffer(GL_UNIFORM_BUFFER, 0);
		}

		global_shader_uniforms.buffer_dirty_region_count = 0;
	}

	if (global_shader_uniforms.must_update_buffer_materials) {
		// only happens in the case of a buffer variable added or removed,
		// so not often.
		for (const RID& E : global_shader_uniforms.materials_using_buffer) {
			Material* material = MaterialStorage::get_material(E);
			ERR_CONTINUE(!material); // wtf

			MaterialStorage::_material_queue_update(material, true, false);
		}

		global_shader_uniforms.must_update_buffer_materials = false;
	}

	if (global_shader_uniforms.must_update_texture_materials) {
		// only happens in the case of a buffer variable added or removed,
		// so not often.
		for (const RID& E : global_shader_uniforms.materials_using_texture) {
			Material* material = MaterialStorage::get_material(E);
			ERR_CONTINUE(!material); // wtf

			MaterialStorage::_material_queue_update(material, false, true);
		}

		global_shader_uniforms.must_update_texture_materials = false;
	}
}

RID GLES3::MaterialStorage::shader_allocate() { return shader_owner.allocate_rid(); }

void GLES3::MaterialStorage::shader_initialize(RID p_rid, bool p_embedded)
{
	Shader shader;
	shader.data = nullptr;
	shader.mode = RSE::SHADER_MAX;

	shader_owner.initialize_rid(p_rid, shader);
}

void GLES3::MaterialStorage::shader_free(RID p_rid)
{
	GLES3::Shader* shader = shader_owner.get_or_null(p_rid);
	ERR_FAIL_NULL(shader);

	// make material unreference this
	while (shader->owners.size()) {
		material_set_shader((*shader->owners.begin())->self, RID());
	}

	// clear data if exists
	memdelete(shader->data);
	shader_owner.free(p_rid);
}

void GLES3::MaterialStorage::shader_set_code(RID p_shader, const String& p_code)
{
	GLES3::Shader* shader = shader_owner.get_or_null(p_shader);
	ERR_FAIL_NULL(shader);

	shader->code = p_code;

	String mode_string = ShaderLanguage::get_shader_type(p_code);

	RSE::ShaderMode new_mode;
	if (mode_string == "canvas_item") {
		new_mode = RSE::SHADER_CANVAS_ITEM;
	}
	else if (mode_string == "particles") {
		new_mode = RSE::SHADER_PARTICLES;
	}
	else if (mode_string == "spatial") {
		new_mode = RSE::SHADER_SPATIAL;
	}
	else if (mode_string == "sky") {
		new_mode = RSE::SHADER_SKY;
		/*
		}
		else if (mode_string == "fog") {
			new_mode = RSE::SHADER_FOG;
			*/
	}
	else if (mode_string == "texture_blit") {
		new_mode = RSE::SHADER_TEXTURE_BLIT;
	}
	else {
		new_mode = RSE::SHADER_MAX;
		ERR_PRINT("shader type " + mode_string + " not supported in OpenGL renderer");
	}

	if (new_mode != shader->mode) {
		if (shader->data) {
			memdelete(shader->data);
			shader->data = nullptr;
		}

		for (Material* E : shader->owners) {
			Material* material = E;
			material->shader_mode = new_mode;
			if (material->data) {
				memdelete(material->data);
				material->data = nullptr;
			}
		}

		shader->mode = new_mode;

		if (new_mode < RSE::SHADER_MAX && shader_data_request_func[new_mode]) {
			shader->data = shader_data_request_func[new_mode]();
		}
		else {
			shader->mode = RSE::SHADER_MAX; // invalid
		}

		for (Material* E : shader->owners) {
			Material* material = E;
			if (shader->data) {
				material->data = material_data_request_func[new_mode](shader->data);
				material->data->self = material->self;
				material->data->set_next_pass(material->next_pass);
				material->data->set_render_priority(material->priority);
			}
			material->shader_mode = new_mode;
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

void GLES3::MaterialStorage::shader_set_path_hint(RID p_shader, const String& p_path)
{
	GLES3::Shader* shader = shader_owner.get_or_null(p_shader);
	ERR_FAIL_NULL(shader);

	shader->path_hint = p_path;
	if (shader->data) {
		shader->data->set_path_hint(p_path);
	}
}

String GLES3::MaterialStorage::shader_get_code(RID p_shader)
{
	const GLES3::Shader* shader = shader_owner.get_or_null(p_shader);
	ERR_FAIL_NULL_V(shader, String());
	return shader->code;
}

void GLES3::MaterialStorage::shader_set_default_texture_parameter(
	RID p_shader, const StringName& p_name, RID p_texture, int p_index)
{
	GLES3::Shader* shader = shader_owner.get_or_null(p_shader);
	ERR_FAIL_NULL(shader);

	if (p_texture.is_valid() && TextureStorage::owns_texture(p_texture)) {
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
	for (Material* E : shader->owners) {
		Material* material = E;
		_material_queue_update(material, false, true);
	}
}

RID GLES3::MaterialStorage::shader_get_default_texture_parameter(
	RID p_shader, const StringName& p_name, int p_index)
{
	const GLES3::Shader* shader = shader_owner.get_or_null(p_shader);
	ERR_FAIL_NULL_V(shader, RID());
	if (shader->default_texture_parameter.has(p_name) &&
		shader->default_texture_parameter[p_name].has(p_index)) {
		return shader->default_texture_parameter[p_name][p_index];
	}

	return RID();
}

RenderingServerTypes::ShaderNativeSourceCode GLES3::MaterialStorage::shader_get_native_source_code(
	RID p_shader)
{
	Shader* shader = shader_owner.get_or_null(p_shader);
	ERR_FAIL_NULL_V(shader, RenderingServerTypes::ShaderNativeSourceCode());
	if (shader->data) {
		return shader->data->get_native_source_code();
	}
	return RenderingServerTypes::ShaderNativeSourceCode();
}

void GLES3::MaterialStorage::_material_queue_update(
	GLES3::Material* material, bool p_uniform, bool p_texture)
{
	material->uniform_dirty = material->uniform_dirty || p_uniform;
	material->texture_dirty = material->texture_dirty || p_texture;

	if (material->update_element.in_list()) {
		return;
	}

	material_update_list.add(&material->update_element);
}

void GLES3::MaterialStorage::_update_queued_materials()
{
	while (material_update_list.first()) {
		Material* material = material_update_list.first()->self();
		material->texture_dirty = false;
		material->uniform_dirty = false;

		material_update_list.remove(&material->update_element);
	}
}

RID GLES3::MaterialStorage::material_allocate() { return material_owner.allocate_rid(); }

void GLES3::MaterialStorage::material_initialize(RID p_rid)
{
	material_owner.initialize_rid(p_rid);
	Material* material = material_owner.get_or_null(p_rid);
	material->self = p_rid;
}

void GLES3::MaterialStorage::material_set_shader(RID p_material, RID p_shader)
{
	GLES3::Material* material = material_owner.get_or_null(p_material);
	ERR_FAIL_NULL(material);

	if (material->data) {
		memdelete(material->data);
		material->data = nullptr;
	}

	if (material->shader) {
		material->shader->owners.erase(material);
		material->shader = nullptr;
		material->shader_mode = RSE::SHADER_MAX;
	}

	if (p_shader.is_null()) {
		material->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MATERIAL);
		material->shader_id = 0;
		return;
	}

	Shader* shader = get_shader(p_shader);
	ERR_FAIL_NULL(shader);
	material->shader = shader;
	material->shader_mode = shader->mode;
	material->shader_id = p_shader.get_local_index();
	shader->owners.insert(material);

	if (shader->mode == RSE::SHADER_MAX) {
		return;
	}

	ERR_FAIL_NULL(shader->data);

	material->data = material_data_request_func[shader->mode](shader->data);
	material->data->self = p_material;
	material->data->set_next_pass(material->next_pass);
	material->data->set_render_priority(material->priority);
	// updating happens later
	material->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MATERIAL);
	_material_queue_update(material, true, true);
}

void GLES3::MaterialStorage::material_set_next_pass(RID p_material, RID p_next_material)
{
	GLES3::Material* material = material_owner.get_or_null(p_material);
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

void GLES3::MaterialStorage::material_set_render_priority(RID p_material, int priority)
{
	ERR_FAIL_COND(priority < RSE::MATERIAL_RENDER_PRIORITY_MIN);
	ERR_FAIL_COND(priority > RSE::MATERIAL_RENDER_PRIORITY_MAX);

	GLES3::Material* material = material_owner.get_or_null(p_material);
	ERR_FAIL_NULL(material);
	material->priority = priority;
	if (material->data) {
		material->data->set_render_priority(priority);
	}
	material->dependency.changed_notify(Dependency::DEPENDENCY_CHANGED_MATERIAL);
}

bool GLES3::MaterialStorage::material_is_animated(RID p_material)
{
	GLES3::Material* material = material_owner.get_or_null(p_material);
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

bool GLES3::MaterialStorage::material_casts_shadows(RID p_material)
{
	GLES3::Material* material = material_owner.get_or_null(p_material);
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

RSE::CullMode GLES3::MaterialStorage::material_get_cull_mode(RID p_material)
{
	const GLES3::Material* material = material_owner.get_or_null(p_material);
	ERR_FAIL_NULL_V(material, RSE::CULL_MODE_DISABLED);
	ERR_FAIL_NULL_V(material->shader, RSE::CULL_MODE_DISABLED);
	if (material->shader->data) {
		SceneShaderData* data = dynamic_cast<SceneShaderData*>(material->shader->data);
		if (data) {
			return (RSE::CullMode)data->cull_mode;
		}
	}
	return RSE::CULL_MODE_DISABLED;
}

void GLES3::MaterialStorage::material_get_instance_shader_parameters(
	RID p_material, List<InstanceShaderParam>* r_parameters)
{
	GLES3::Material* material = material_owner.get_or_null(p_material);
	ERR_FAIL_NULL(material);
	if (material->shader && material->shader->data) {
		material->shader->data->get_instance_param_list(r_parameters);

		if (material->next_pass.is_valid()) {
			material_get_instance_shader_parameters(material->next_pass, r_parameters);
		}
	}
}

void GLES3::MaterialStorage::material_update_dependency(RID p_material, DependencyTracker* p_instance)
{
	Material* material = material_owner.get_or_null(p_material);
	ERR_FAIL_NULL(material);
	p_instance->update_dependency(&material->dependency);
	if (material->next_pass.is_valid()) {
		material_update_dependency(material->next_pass, p_instance);
	}
}

LocalVector<ShaderGLES3::TextureUniformData> get_texture_uniform_data(
	const Vector<ShaderCompiler::GeneratedCode::Texture>& texture_uniforms)
{
	LocalVector<ShaderGLES3::TextureUniformData> texture_uniform_data;
	for (int i = 0; i < texture_uniforms.size(); i++) {
		int num_textures = texture_uniforms[i].array_size;
		if (num_textures == 0) {
			num_textures = 1;
		}
		texture_uniform_data.push_back({texture_uniforms[i].name, num_textures});
	}
	return texture_uniform_data;
}

void CanvasShaderData::set_code(const String& p_code)
{
	// Initialize and compile the shader.

	code = p_code;
	valid = false;
	ubo_size = 0;
	uniforms.clear();

	uses_screen_texture = false;
	uses_screen_texture_mipmaps = false;
	uses_sdf = false;
	uses_time = false;
	uses_custom0 = false;
	uses_custom1 = false;

	if (code.is_empty()) {
		return; // Just invalid, but no error.
	}

	ShaderCompiler::GeneratedCode gen_code;

	// Actual enum set further down after compilation.
	int blend_modei = BLEND_MODE_MIX;

	ShaderCompiler::IdentifierActions actions;
	actions.entry_point_stages["vertex"] = ShaderCompiler::STAGE_VERTEX;
	actions.entry_point_stages["fragment"] = ShaderCompiler::STAGE_FRAGMENT;
	actions.entry_point_stages["light"] = ShaderCompiler::STAGE_FRAGMENT;

	actions.render_mode_values["blend_add"] = Pair<int*, int>(&blend_modei, BLEND_MODE_ADD);
	actions.render_mode_values["blend_mix"] = Pair<int*, int>(&blend_modei, BLEND_MODE_MIX);
	actions.render_mode_values["blend_sub"] = Pair<int*, int>(&blend_modei, BLEND_MODE_SUB);
	actions.render_mode_values["blend_mul"] = Pair<int*, int>(&blend_modei, BLEND_MODE_MUL);
	actions.render_mode_values["blend_premul_alpha"] =
		Pair<int*, int>(&blend_modei, BLEND_MODE_PMALPHA);
	actions.render_mode_values["blend_disabled"] =
		Pair<int*, int>(&blend_modei, BLEND_MODE_DISABLED);

	actions.usage_flag_pointers["texture_sdf"] = &uses_sdf;
	actions.usage_flag_pointers["texture_sdf_normal"] = &uses_sdf;
	actions.usage_flag_pointers["TIME"] = &uses_time;
	actions.usage_flag_pointers["CUSTOM0"] = &uses_custom0;
	actions.usage_flag_pointers["CUSTOM1"] = &uses_custom1;

	actions.uniforms = &uniforms;
	Error err = MaterialStorage::shaders->compiler_canvas.compile(
		RSE::SHADER_CANVAS_ITEM, code, &actions, path, gen_code);
	ERR_FAIL_COND_MSG(err != OK, "Shader compilation failed.");

	if (version.is_null()) {
		version = MaterialStorage::shaders->canvas_shader.version_create();
	}

	blend_mode = BlendMode(blend_modei);
	uses_screen_texture = gen_code.uses_screen_texture;
	uses_screen_texture_mipmaps = gen_code.uses_screen_texture_mipmaps;

#if 0
	print_line("**compiling shader:");
	print_line("**defines:\n");
	for (int i = 0; i < gen_code.defines.size(); i++) {
		print_line(gen_code.defines[i]);
	}

	HashMap<String, String>::Iterator el = gen_code.code.begin();
	while (el) {
		print_line("\n**code " + el->key + ":\n" + el->value);
		++el;
	}

	print_line("\n**uniforms:\n" + gen_code.uniforms);
	print_line("\n**vertex_globals:\n" + gen_code.stage_globals[ShaderCompiler::STAGE_VERTEX]);
	print_line("\n**fragment_globals:\n" + gen_code.stage_globals[ShaderCompiler::STAGE_FRAGMENT]);
#endif

	LocalVector<ShaderGLES3::TextureUniformData> texture_uniform_data =
		get_texture_uniform_data(gen_code.texture_uniforms);

	MaterialStorage::shaders->canvas_shader.version_set_code(version, gen_code.code,
		gen_code.uniforms, gen_code.stage_globals[ShaderCompiler::STAGE_VERTEX],
		gen_code.stage_globals[ShaderCompiler::STAGE_FRAGMENT], gen_code.defines,
		texture_uniform_data);
	ERR_FAIL_COND(
		!MaterialStorage::shaders->canvas_shader.version_is_valid(version));

	vertex_input_mask =
		RSE::ARRAY_FORMAT_VERTEX | RSE::ARRAY_FORMAT_COLOR | RSE::ARRAY_FORMAT_TEX_UV;
	vertex_input_mask |= uses_custom0 << RSE::ARRAY_CUSTOM0;
	vertex_input_mask |= uses_custom1 << RSE::ARRAY_CUSTOM1;

	ubo_size = gen_code.uniform_total_size;
	ubo_offsets = gen_code.uniform_offsets;
	texture_uniforms = gen_code.texture_uniforms;

	valid = true;
}

bool CanvasShaderData::is_animated() const { return false; }

bool CanvasShaderData::casts_shadows() const { return false; }

RenderingServerTypes::ShaderNativeSourceCode CanvasShaderData::get_native_source_code() const
{
	return MaterialStorage::shaders->canvas_shader.version_get_native_source_code(
		version);
}

CanvasShaderData::CanvasShaderData()
{
	valid = false;
	uses_screen_texture = false;
	uses_sdf = false;
}

CanvasShaderData::~CanvasShaderData()
{
	if (version.is_valid()) {
		MaterialStorage::shaders->canvas_shader.version_free(version);
	}
}

GLES3::ShaderData* GLES3::_create_canvas_shader_func()
{
	CanvasShaderData* shader_data = memnew(CanvasShaderData);
	return shader_data;
}

static void bind_uniforms_generic(const Vector<RID>& p_textures,
	const Vector<ShaderCompiler::GeneratedCode::Texture>& p_texture_uniforms,
	int texture_offset = 0,
	const RSE::CanvasItemTextureFilter* filter_mapping = filter_from_uniform,
	const RSE::CanvasItemTextureRepeat* repeat_mapping = repeat_from_uniform)
{
	const RID* textures = p_textures.ptr();
	const ShaderCompiler::GeneratedCode::Texture* texture_uniforms = p_texture_uniforms.ptr();
	int texture_uniform_index = 0;
	int texture_uniform_count = 0;
	for (int ti = 0; ti < p_textures.size(); ti++) {
		ERR_FAIL_COND_MSG(texture_uniform_index >= p_texture_uniforms.size(),
			"texture_uniform_index out of bounds");
		GLES3::Texture* texture = GLES3::TextureStorage::get_texture(textures[ti]);
		const ShaderCompiler::GeneratedCode::Texture& texture_uniform =
			texture_uniforms[texture_uniform_index];
		if (texture) {
			glActiveTexture(GL_TEXTURE0 + texture_offset + ti);
			GLenum target = target_from_type[texture_uniform.type];
			if (target == _GL_TEXTURE_EXTERNAL_OES &&
				!GLES3::Config::get_singleton()->external_texture_supported) {
				target = GL_TEXTURE_2D;
			}
			glBindTexture(target, texture->tex_id);
			if (texture->render_target) {
				texture->render_target->used_in_frame = true;
			}

			texture->gl_set_filter(filter_mapping[int(texture_uniform.filter)]);
			texture->gl_set_repeat(repeat_mapping[int(texture_uniform.repeat)]);
		}
		texture_uniform_count++;
		if (texture_uniform_count >= texture_uniform.array_size) {
			texture_uniform_index++;
			texture_uniform_count = 0;
		}
	}
}

void CanvasMaterialData::bind_uniforms()
{
	// Bind Material Uniforms
	glBindBufferBase(
		GL_UNIFORM_BUFFER, RasterizerCanvasGLES3::MATERIAL_UNIFORM_LOCATION, uniform_buffer);

	bind_uniforms_generic(texture_cache, shader_data->texture_uniforms, 1,
		filter_from_uniform_canvas,
		repeat_from_uniform_canvas); // Start at GL_TEXTURE1 because texture slot 0 is used by the
									 // base texture
}

CanvasMaterialData::~CanvasMaterialData() {}

GLES3::MaterialData* GLES3::_create_canvas_material_func(ShaderData* p_shader)
{
	CanvasMaterialData* material_data = memnew(CanvasMaterialData);
	material_data->shader_data = static_cast<CanvasShaderData*>(p_shader);
	// update will happen later anyway so do nothing.
	return material_data;
}

void SkyShaderData::set_code(const String& p_code)
{
	// Initialize and compile the shader.

	code = p_code;
	valid = false;
	ubo_size = 0;
	uniforms.clear();

	uses_time = false;
	uses_position = false;
	uses_half_res = false;
	uses_quarter_res = false;
	uses_light = false;

	if (code.is_empty()) {
		return; // Just invalid, but no error.
	}

	ShaderCompiler::GeneratedCode gen_code;

	ShaderCompiler::IdentifierActions actions;
	actions.entry_point_stages["sky"
] = ShaderCompiler::STAGE_FRAGMENT;

	actions.render_mode_flags["use_half_res_pass"] = &uses_half_res;
	actions.render_mode_flags["use_quarter_res_pass"] = &uses_quarter_res;

	actions.usage_flag_pointers["TIME"] = &uses_time;
	actions.usage_flag_pointers["POSITION"] = &uses_position;
	actions.usage_flag_pointers["LIGHT0_ENABLED"] = &uses_light;
	actions.usage_flag_pointers["LIGHT0_ENERGY"] = &uses_light;
	actions.usage_flag_pointers["LIGHT0_DIRECTION"] = &uses_light;
	actions.usage_flag_pointers["LIGHT0_COLOR"] = &uses_light;
	actions.usage_flag_pointers["LIGHT0_SIZE"] = &uses_light;
	actions.usage_flag_pointers["LIGHT1_ENABLED"] = &uses_light;
	actions.usage_flag_pointers["LIGHT1_ENERGY"] = &uses_light;
	actions.usage_flag_pointers["LIGHT1_DIRECTION"] = &uses_light;
	actions.usage_flag_pointers["LIGHT1_COLOR"] = &uses_light;
	actions.usage_flag_pointers["LIGHT1_SIZE"] = &uses_light;
	actions.usage_flag_pointers["LIGHT2_ENABLED"] = &uses_light;
	actions.usage_flag_pointers["LIGHT2_ENERGY"] = &uses_light;
	actions.usage_flag_pointers["LIGHT2_DIRECTION"] = &uses_light;
	actions.usage_flag_pointers["LIGHT2_COLOR"] = &uses_light;
	actions.usage_flag_pointers["LIGHT2_SIZE"] = &uses_light;
	actions.usage_flag_pointers["LIGHT3_ENABLED"] = &uses_light;
	actions.usage_flag_pointers["LIGHT3_ENERGY"] = &uses_light;
	actions.usage_flag_pointers["LIGHT3_DIRECTION"] = &uses_light;
	actions.usage_flag_pointers["LIGHT3_COLOR"] = &uses_light;
	actions.usage_flag_pointers["LIGHT3_SIZE"] = &uses_light;

	actions.uniforms = &uniforms;

	Error err = MaterialStorage::shaders->compiler_sky.compile(
		RSE::SHADER_SKY, code, &actions, path, gen_code);
	ERR_FAIL_COND_MSG(err != OK, "Shader compilation failed.");

	if (version.is_null()) {
		version = MaterialStorage::shaders->sky_shader.version_create();
	}

#if 0
	print_line("**compiling shader:");
	print_line("**defines:\n");
	for (int i = 0; i < gen_code.defines.size(); i++) {
		print_line(gen_code.defines[i]);
	}

	HashMap<String, String>::Iterator el = gen_code.code.begin();
	while (el) {
		print_line("\n**code " + el->key + ":\n" + el->value);
		++el;
	}

	print_line("\n**uniforms:\n" + gen_code.uniforms);
	print_line("\n**vertex_globals:\n" + gen_code.stage_globals[ShaderCompiler::STAGE_VERTEX]);
	print_line("\n**fragment_globals:\n" + gen_code.stage_globals[ShaderCompiler::STAGE_FRAGMENT]);
#endif

	LocalVector<ShaderGLES3::TextureUniformData> texture_uniform_data =
		get_texture_uniform_data(gen_code.texture_uniforms);

	MaterialStorage::shaders->sky_shader.version_set_code(version, gen_code.code,
		gen_code.uniforms, gen_code.stage_globals[ShaderCompiler::STAGE_VERTEX],
		gen_code.stage_globals[ShaderCompiler::STAGE_FRAGMENT], gen_code.defines,
		texture_uniform_data);
	ERR_FAIL_COND(!MaterialStorage::shaders->sky_shader.version_is_valid(version));

	ubo_size = gen_code.uniform_total_size;
	ubo_offsets = gen_code.uniform_offsets;
	texture_uniforms = gen_code.texture_uniforms;

	valid = true;
}

bool SkyShaderData::is_animated() const { return false; }

bool SkyShaderData::casts_shadows() const { return false; }

RenderingServerTypes::ShaderNativeSourceCode SkyShaderData::get_native_source_code() const
{
	return MaterialStorage::shaders->sky_shader.version_get_native_source_code(
		version);
}

SkyShaderData::SkyShaderData() { valid = false; }

SkyShaderData::~SkyShaderData()
{
	if (version.is_valid()) {
		MaterialStorage::shaders->sky_shader.version_free(version);
	}
}

GLES3::ShaderData* GLES3::_create_sky_shader_func()
{
	SkyShaderData* shader_data = memnew(SkyShaderData);
	return shader_data;
}

SkyMaterialData::~SkyMaterialData() {}

GLES3::MaterialData* GLES3::_create_sky_material_func(ShaderData* p_shader)
{
	SkyMaterialData* material_data = memnew(SkyMaterialData);
	material_data->shader_data = static_cast<SkyShaderData*>(p_shader);
	// update will happen later anyway so do nothing.
	return material_data;
}

void SkyMaterialData::bind_uniforms()
{
	// Bind Material Uniforms
	glBindBufferBase(GL_UNIFORM_BUFFER, SKY_MATERIAL_UNIFORM_LOCATION, uniform_buffer);

	bind_uniforms_generic(texture_cache, shader_data->texture_uniforms);
}

void SceneShaderData::set_code(const String& p_code)
{
	// Initialize and compile the shader.

	code = p_code;
	valid = false;
	ubo_size = 0;
	uniforms.clear();

	uses_point_size = false;
	uses_alpha = false;
	uses_alpha = false;
	uses_blend_alpha = false;
	uses_depth_prepass_alpha = false;
	uses_discard = false;
	uses_roughness = false;
	uses_normal = false;
	uses_particle_trails = false;
	wireframe = false;

	unshaded = false;
	uses_vertex = false;
	uses_position = false;
	uses_sss = false;
	uses_transmittance = false;
	uses_screen_texture = false;
	uses_screen_texture_mipmaps = false;
	uses_depth_texture = false;
	uses_normal_texture = false;
	uses_bent_normal_texture = false;
	uses_time = false;
	uses_vertex_time = false;
	uses_fragment_time = false;
	writes_modelview_or_projection = false;
	uses_world_coordinates = false;
	uses_tangent = false;
	writes_tangent = false;
	uses_color = false;
	uses_uv = false;
	uses_uv2 = false;
	uses_custom0 = false;
	uses_custom1 = false;
	uses_custom2 = false;
	uses_custom3 = false;
	uses_bones = false;
	uses_weights = false;

	if (code.is_empty()) {
		return; // Just invalid, but no error.
	}

	ShaderCompiler::GeneratedCode gen_code;

	// Actual enums set further down after compilation.
	int blend_modei = BLEND_MODE_MIX;
	int depth_test_disabledi = 0;
	int depth_test_invertedi = 0;
	int alpha_antialiasing_modei = ALPHA_ANTIALIASING_OFF;
	int cull_modei = RSE::CULL_MODE_BACK;
	int depth_drawi = DEPTH_DRAW_OPAQUE;

	int stencil_readi = 0;
	int stencil_writei = 0;
	int stencil_write_depth_faili = 0;
	int stencil_comparei = STENCIL_COMPARE_ALWAYS;
	int stencil_referencei = -1;

	ShaderCompiler::IdentifierActions actions;
	actions.entry_point_stages["vertex"] = ShaderCompiler::STAGE_VERTEX;
	actions.entry_point_stages["fragment"] = ShaderCompiler::STAGE_FRAGMENT;
	actions.entry_point_stages["light"] = ShaderCompiler::STAGE_FRAGMENT;

	actions.render_mode_values["blend_add"] = Pair<int*, int>(&blend_modei, BLEND_MODE_ADD);
	actions.render_mode_values["blend_mix"] = Pair<int*, int>(&blend_modei, BLEND_MODE_MIX);
	actions.render_mode_values["blend_sub"] = Pair<int*, int>(&blend_modei, BLEND_MODE_SUB);
	actions.render_mode_values["blend_mul"] = Pair<int*, int>(&blend_modei, BLEND_MODE_MUL);
	actions.render_mode_values["blend_premul_alpha"] =
		Pair<int*, int>(&blend_modei, BLEND_MODE_PREMULT_ALPHA);

	actions.render_mode_values["alpha_to_coverage"] =
		Pair<int*, int>(&alpha_antialiasing_modei, ALPHA_ANTIALIASING_ALPHA_TO_COVERAGE);
	actions.render_mode_values["alpha_to_coverage_and_one"] =
		Pair<int*, int>(&alpha_antialiasing_modei, ALPHA_ANTIALIASING_ALPHA_TO_COVERAGE_AND_TO_ONE);

	actions.render_mode_values["depth_draw_never"] =
		Pair<int*, int>(&depth_drawi, DEPTH_DRAW_DISABLED);
	actions.render_mode_values["depth_draw_opaque"] =
		Pair<int*, int>(&depth_drawi, DEPTH_DRAW_OPAQUE);
	actions.render_mode_values["depth_draw_always"] =
		Pair<int*, int>(&depth_drawi, DEPTH_DRAW_ALWAYS);

	actions.render_mode_values["depth_test_disabled"] = Pair<int*, int>(&depth_test_disabledi, 1);
	actions.render_mode_values["depth_test_inverted"] = Pair<int*, int>(&depth_test_invertedi, 1);

	actions.render_mode_values["cull_disabled"] =
		Pair<int*, int>(&cull_modei, RSE::CULL_MODE_DISABLED);
	actions.render_mode_values["cull_front"] = Pair<int*, int>(&cull_modei, RSE::CULL_MODE_FRONT);
	actions.render_mode_values["cull_back"] = Pair<int*, int>(&cull_modei, RSE::CULL_MODE_BACK);

	actions.render_mode_flags["unshaded"] = &unshaded;
	actions.render_mode_flags["wireframe"] = &wireframe;
	actions.render_mode_flags["particle_trails"] = &uses_particle_trails;
	actions.render_mode_flags["world_vertex_coords"] = &uses_world_coordinates;

	actions.usage_flag_pointers["ALPHA"] = &uses_alpha;
	actions.usage_flag_pointers["ALPHA_SCISSOR_THRESHOLD"] = &uses_alpha;
	// Use alpha clip pipeline for alpha hash/dither.
	// This prevents sorting issues inherent to alpha blending and allows such materials to cast
	// shadows.
	actions.usage_flag_pointers["ALPHA_HASH_SCALE"] = &uses_alpha;
	actions.render_mode_flags["depth_prepass_alpha"] = &uses_depth_prepass_alpha;

	actions.usage_flag_pointers["SSS_STRENGTH"] = &uses_sss;
	actions.usage_flag_pointers["SSS_TRANSMITTANCE_DEPTH"] = &uses_transmittance;

	actions.usage_flag_pointers["DISCARD"] = &uses_discard;
	actions.usage_flag_pointers["TIME"] = &uses_time;
	actions.usage_flag_pointers["ROUGHNESS"] = &uses_roughness;
	actions.usage_flag_pointers["NORMAL"] = &uses_normal;
	actions.usage_flag_pointers["NORMAL_MAP"] = &uses_normal;
	actions.usage_flag_pointers["BENT_NORMAL_MAP"] = &uses_bent_normal_texture;

	actions.usage_flag_pointers["POINT_SIZE"] = &uses_point_size;
	actions.usage_flag_pointers["POINT_COORD"] = &uses_point_size;

	actions.write_flag_pointers["MODELVIEW_MATRIX"] = &writes_modelview_or_projection;
	actions.write_flag_pointers["PROJECTION_MATRIX"] = &writes_modelview_or_projection;
	actions.write_flag_pointers["VERTEX"] = &uses_vertex;
	actions.write_flag_pointers["POSITION"] = &uses_position;
	actions.write_flag_pointers["TANGENT"] = &writes_tangent;
	actions.write_flag_pointers["BINORMAL"] = &writes_tangent;

	actions.usage_flag_pointers["TANGENT"] = &uses_tangent;
	actions.usage_flag_pointers["BINORMAL"] = &uses_tangent;
	actions.usage_flag_pointers["ANISOTROPY"] = &uses_tangent;
	actions.usage_flag_pointers["ANISOTROPY_FLOW"] = &uses_tangent;
	actions.usage_flag_pointers["COLOR"] = &uses_color;
	actions.usage_flag_pointers["UV"] = &uses_uv;
	actions.usage_flag_pointers["UV2"] = &uses_uv2;
	actions.usage_flag_pointers["CUSTOM0"] = &uses_custom0;
	actions.usage_flag_pointers["CUSTOM1"] = &uses_custom1;
	actions.usage_flag_pointers["CUSTOM2"] = &uses_custom2;
	actions.usage_flag_pointers["CUSTOM3"] = &uses_custom3;
	actions.usage_flag_pointers["BONE_INDICES"] = &uses_bones;
	actions.usage_flag_pointers["BONE_WEIGHTS"] = &uses_weights;

	actions.stencil_mode_values["read"] = Pair<int*, int>(&stencil_readi, STENCIL_FLAG_READ);
	actions.stencil_mode_values["write"] = Pair<int*, int>(&stencil_writei, STENCIL_FLAG_WRITE);
	actions.stencil_mode_values["write_depth_fail"] =
		Pair<int*, int>(&stencil_write_depth_faili, STENCIL_FLAG_WRITE_DEPTH_FAIL);

	actions.stencil_mode_values["compare_less"] =
		Pair<int*, int>(&stencil_comparei, STENCIL_COMPARE_LESS);
	actions.stencil_mode_values["compare_equal"] =
		Pair<int*, int>(&stencil_comparei, STENCIL_COMPARE_EQUAL);
	actions.stencil_mode_values["compare_less_or_equal"] =
		Pair<int*, int>(&stencil_comparei, STENCIL_COMPARE_LESS_OR_EQUAL);
	actions.stencil_mode_values["compare_greater"] =
		Pair<int*, int>(&stencil_comparei, STENCIL_COMPARE_GREATER);
	actions.stencil_mode_values["compare_not_equal"] =
		Pair<int*, int>(&stencil_comparei, STENCIL_COMPARE_NOT_EQUAL);
	actions.stencil_mode_values["compare_greater_or_equal"] =
		Pair<int*, int>(&stencil_comparei, STENCIL_COMPARE_GREATER_OR_EQUAL);
	actions.stencil_mode_values["compare_always"] =
		Pair<int*, int>(&stencil_comparei, STENCIL_COMPARE_ALWAYS);

	actions.stencil_reference = &stencil_referencei;

	actions.uniforms = &uniforms;

	Error err = MaterialStorage::shaders->compiler_scene.compile(
		RSE::SHADER_SPATIAL, code, &actions, path, gen_code);
	ERR_FAIL_COND_MSG(err != OK, "Shader compilation failed.");

	if (version.is_null()) {
		version = MaterialStorage::shaders->scene_shader.version_create();
	}

	blend_mode = BlendMode(blend_modei);
	alpha_antialiasing_mode = AlphaAntiAliasing(alpha_antialiasing_modei);
	depth_draw = DepthDraw(depth_drawi);
	if (depth_test_disabledi) {
		depth_test = DEPTH_TEST_DISABLED;
	}
	else if (depth_test_invertedi) {
		depth_test = DEPTH_TEST_ENABLED_INVERTED;
	}
	else {
		depth_test = DEPTH_TEST_ENABLED;
	}
	cull_mode = RSE::CullMode(cull_modei);

	vertex_input_mask = RSE::ARRAY_FORMAT_VERTEX |
						RSE::ARRAY_FORMAT_NORMAL; // We can always read vertices and normals.
	vertex_input_mask |= uses_tangent << RSE::ARRAY_TANGENT;
	vertex_input_mask |= uses_color << RSE::ARRAY_COLOR;
	vertex_input_mask |= uses_uv << RSE::ARRAY_TEX_UV;
	vertex_input_mask |= uses_uv2 << RSE::ARRAY_TEX_UV2;
	vertex_input_mask |= uses_custom0 << RSE::ARRAY_CUSTOM0;
	vertex_input_mask |= uses_custom1 << RSE::ARRAY_CUSTOM1;
	vertex_input_mask |= uses_custom2 << RSE::ARRAY_CUSTOM2;
	vertex_input_mask |= uses_custom3 << RSE::ARRAY_CUSTOM3;
	vertex_input_mask |= uses_bones << RSE::ARRAY_BONES;
	vertex_input_mask |= uses_weights << RSE::ARRAY_WEIGHTS;

	uses_screen_texture = gen_code.uses_screen_texture;
	uses_screen_texture_mipmaps = gen_code.uses_screen_texture_mipmaps;
	uses_depth_texture = gen_code.uses_depth_texture;
	uses_normal_texture = gen_code.uses_normal_roughness_texture;
	uses_vertex_time = gen_code.uses_vertex_time;
	uses_fragment_time = gen_code.uses_fragment_time;

	stencil_enabled = stencil_referencei != -1;
	stencil_flags = stencil_readi | stencil_writei | stencil_write_depth_faili;
	stencil_compare = StencilCompare(stencil_comparei);
	stencil_reference = stencil_referencei;

#ifdef DEBUG_ENABLED
	if (uses_particle_trails) {
		WARN_PRINT_ONCE_ED(
			"Particle trails are only available when using the Forward+ or Mobile renderer.");
	}

	if (uses_sss) {
		WARN_PRINT_ONCE_ED(
			"Subsurface scattering is only available when using the Forward+ renderer.");
	}

	if (uses_transmittance) {
		WARN_PRINT_ONCE_ED("Transmittance is only available when using the Forward+ renderer.");
	}

	if (uses_normal_texture) {
		WARN_PRINT_ONCE_ED("Reading from the normal-roughness texture is only available when using "
						   "the Forward+ or Mobile renderer.");
	}
#endif

#if 0
	print_line("**compiling shader:");
	print_line("**defines:\n");
	for (int i = 0; i < gen_code.defines.size(); i++) {
		print_line(gen_code.defines[i]);
	}

	HashMap<String, String>::Iterator el = gen_code.code.begin();
	while (el) {
		print_line("\n**code " + el->key + ":\n" + el->value);
		++el;
	}

	print_line("\n**uniforms:\n" + gen_code.uniforms);
	print_line("\n**vertex_globals:\n" + gen_code.stage_globals[ShaderCompiler::STAGE_VERTEX]);
	print_line("\n**fragment_globals:\n" + gen_code.stage_globals[ShaderCompiler::STAGE_FRAGMENT]);
#endif

	LocalVector<ShaderGLES3::TextureUniformData> texture_uniform_data =
		get_texture_uniform_data(gen_code.texture_uniforms);

	MaterialStorage::shaders->scene_shader.version_set_code(version, gen_code.code,
		gen_code.uniforms, gen_code.stage_globals[ShaderCompiler::STAGE_VERTEX],
		gen_code.stage_globals[ShaderCompiler::STAGE_FRAGMENT], gen_code.defines,
		texture_uniform_data);
	ERR_FAIL_COND(
		!MaterialStorage::shaders->scene_shader.version_is_valid(version));

	ubo_size = gen_code.uniform_total_size;
	ubo_offsets = gen_code.uniform_offsets;
	texture_uniforms = gen_code.texture_uniforms;

	// If any form of Alpha Antialiasing is enabled, set the blend mode to alpha to coverage.
	if (alpha_antialiasing_mode != ALPHA_ANTIALIASING_OFF) {
		blend_mode = BLEND_MODE_ALPHA_TO_COVERAGE;
	}

	if (blend_mode == BLEND_MODE_ADD || blend_mode == BLEND_MODE_SUB ||
		blend_mode == BLEND_MODE_MUL) {
		uses_blend_alpha = true; // Force alpha used because of blend.
	}

	valid = true;
}

bool SceneShaderData::is_animated() const
{
	return (uses_fragment_time && uses_discard) || (uses_vertex_time && uses_vertex);
}

bool SceneShaderData::casts_shadows() const
{
	bool has_read_screen_alpha = uses_screen_texture || uses_depth_texture || uses_normal_texture;
	bool has_base_alpha = (uses_alpha && !uses_alpha) || has_read_screen_alpha;
	bool has_alpha = has_base_alpha || uses_blend_alpha;

	return !has_alpha || (uses_depth_prepass_alpha && !(depth_draw == DEPTH_DRAW_DISABLED ||
														  depth_test != DEPTH_TEST_ENABLED));
}

RenderingServerTypes::ShaderNativeSourceCode SceneShaderData::get_native_source_code() const
{
	return MaterialStorage::shaders->scene_shader.version_get_native_source_code(
		version);
}

SceneShaderData::SceneShaderData()
{
	valid = false;
	uses_screen_texture = false;
}

SceneShaderData::~SceneShaderData()
{
	if (version.is_valid()) {
		MaterialStorage::shaders->scene_shader.version_free(version);
	}
}

GLES3::ShaderData* GLES3::_create_scene_shader_func()
{
	SceneShaderData* shader_data = memnew(SceneShaderData);
	return shader_data;
}

void SceneMaterialData::set_render_priority(int p_priority)
{
	priority = p_priority - RSE::MATERIAL_RENDER_PRIORITY_MIN; // 8 bits
}

void SceneMaterialData::set_next_pass(RID p_pass) { next_pass = p_pass; }

SceneMaterialData::~SceneMaterialData() {}

GLES3::MaterialData* GLES3::_create_scene_material_func(ShaderData* p_shader)
{
	SceneMaterialData* material_data = memnew(SceneMaterialData);
	material_data->shader_data = static_cast<SceneShaderData*>(p_shader);
	// update will happen later anyway so do nothing.
	return material_data;
}

void SceneMaterialData::bind_uniforms()
{
	// Bind Material Uniforms
	glBindBufferBase(GL_UNIFORM_BUFFER, SCENE_MATERIAL_UNIFORM_LOCATION, uniform_buffer);

	bind_uniforms_generic(texture_cache, shader_data->texture_uniforms);
}

void ParticlesShaderData::set_code(const String& p_code)
{
	// Initialize and compile the shader.

	code = p_code;
	valid = false;
	ubo_size = 0;
	uniforms.clear();

	uses_collision = false;
	uses_time = false;

	if (code.is_empty()) {
		return; // Just invalid, but no error.
	}

	ShaderCompiler::GeneratedCode gen_code;

	ShaderCompiler::IdentifierActions actions;
	actions.entry_point_stages["start"] = ShaderCompiler::STAGE_VERTEX;
	actions.entry_point_stages["process"] = ShaderCompiler::STAGE_VERTEX;

	actions.usage_flag_pointers["COLLIDED"] = &uses_collision;

	userdata_count = 0;
	for (uint32_t i = 0; i < PARTICLES_MAX_USERDATAS; i++) {
		userdatas_used[i] = false;
		actions.usage_flag_pointers["USERDATA" + itos(i + 1)] = &userdatas_used[i];
	}

	actions.uniforms = &uniforms;

	Error err = MaterialStorage::shaders->compiler_particles.compile(
		RSE::SHADER_PARTICLES, code, &actions, path, gen_code);
	ERR_FAIL_COND_MSG(err != OK, "Shader compilation failed.");

	if (version.is_null()) {
		version =
			MaterialStorage::shaders->particles_process_shader.version_create();
	}

	for (uint32_t i = 0; i < PARTICLES_MAX_USERDATAS; i++) {
		if (userdatas_used[i]) {
			userdata_count++;
		}
	}

	LocalVector<ShaderGLES3::TextureUniformData> texture_uniform_data =
		get_texture_uniform_data(gen_code.texture_uniforms);

	MaterialStorage::shaders->particles_process_shader.version_set_code(version,
		gen_code.code, gen_code.uniforms, gen_code.stage_globals[ShaderCompiler::STAGE_VERTEX],
		gen_code.stage_globals[ShaderCompiler::STAGE_FRAGMENT], gen_code.defines,
		texture_uniform_data);
	ERR_FAIL_COND(
		!MaterialStorage::shaders->particles_process_shader.version_is_valid(
			version));

	ubo_size = gen_code.uniform_total_size;
	ubo_offsets = gen_code.uniform_offsets;
	texture_uniforms = gen_code.texture_uniforms;

	valid = true;
}

bool ParticlesShaderData::is_animated() const { return false; }

bool ParticlesShaderData::casts_shadows() const { return false; }

RenderingServerTypes::ShaderNativeSourceCode ParticlesShaderData::get_native_source_code() const
{
	return MaterialStorage::shaders->particles_process_shader.version_get_native_source_code(version);
}

ParticlesShaderData::~ParticlesShaderData()
{
	if (version.is_valid()) {
		MaterialStorage::shaders->particles_process_shader.version_free(version);
	}
}

GLES3::ShaderData* GLES3::_create_particles_shader_func()
{
	ParticlesShaderData* shader_data = memnew(ParticlesShaderData);
	return shader_data;
}

ParticleProcessMaterialData::~ParticleProcessMaterialData() {}

GLES3::MaterialData* GLES3::_create_particles_material_func(ShaderData* p_shader)
{
	ParticleProcessMaterialData* material_data = memnew(ParticleProcessMaterialData);
	material_data->shader_data = static_cast<ParticlesShaderData*>(p_shader);
	// update will happen later anyway so do nothing.
	return material_data;
}

void ParticleProcessMaterialData::bind_uniforms()
{
	// Bind Material Uniforms
	glBindBufferBase(GL_UNIFORM_BUFFER, GLES3::PARTICLES_MATERIAL_UNIFORM_LOCATION, uniform_buffer);

	bind_uniforms_generic(texture_cache, shader_data->texture_uniforms,
		1); // Start at GL_TEXTURE1 because texture slot 0 is reserved for the heightmap texture.
}

void TexBlitShaderData::set_code(const String& p_code)
{
	// Initialize and compile the shader.

	code = p_code;
	valid = false;
	ubo_size = 0;
	uniforms.clear();

	if (code.is_empty()) {
		return; // Just invalid, but no error.
	}

	ShaderCompiler::GeneratedCode gen_code;

	// Actual enum set further down after compilation.
	int blend_modei = BLEND_MODE_MIX;

	ShaderCompiler::IdentifierActions actions;
	actions.entry_point_stages["blit"] = ShaderCompiler::STAGE_FRAGMENT;

	actions.render_mode_values["blend_add"] = Pair<int*, int>(&blend_modei, BLEND_MODE_ADD);
	actions.render_mode_values["blend_mix"] = Pair<int*, int>(&blend_modei, BLEND_MODE_MIX);
	actions.render_mode_values["blend_sub"] = Pair<int*, int>(&blend_modei, BLEND_MODE_SUB);
	actions.render_mode_values["blend_mul"] = Pair<int*, int>(&blend_modei, BLEND_MODE_MUL);
	actions.render_mode_values["blend_disabled"] =
		Pair<int*, int>(&blend_modei, BLEND_MODE_DISABLED);

	actions.uniforms = &uniforms;
	Error err = MaterialStorage::shaders->compiler_tex_blit.compile(
		RSE::SHADER_TEXTURE_BLIT, code, &actions, path, gen_code);
	ERR_FAIL_COND_MSG(err != OK, "Shader compilation failed.");

	if (version.is_null()) {
		version = MaterialStorage::shaders->tex_blit_shader.version_create();
	}

	blend_mode = BlendMode(blend_modei);

#if 0
	print_line("**compiling shader:");
	print_line("**defines:\n");
	for (int i = 0; i < gen_code.defines.size(); i++) {
		print_line(gen_code.defines[i]);
	}

	HashMap<String, String>::Iterator el = gen_code.code.begin();
	while (el) {
		print_line("\n**code " + el->key + ":\n" + el->value);
		++el;
	}

	print_line("\n**uniforms:\n" + gen_code.uniforms);
	print_line("\n**vertex_globals:\n" + gen_code.stage_globals[ShaderCompiler::STAGE_VERTEX]);
	print_line("\n**fragment_globals:\n" + gen_code.stage_globals[ShaderCompiler::STAGE_FRAGMENT]);
#endif

	LocalVector<ShaderGLES3::TextureUniformData> texture_uniform_data =
		get_texture_uniform_data(gen_code.texture_uniforms);

	MaterialStorage::shaders->tex_blit_shader.version_set_code(version,
		gen_code.code, gen_code.uniforms, gen_code.stage_globals[ShaderCompiler::STAGE_VERTEX],
		gen_code.stage_globals[ShaderCompiler::STAGE_FRAGMENT], gen_code.defines,
		texture_uniform_data);
	ERR_FAIL_COND(
		!MaterialStorage::shaders->tex_blit_shader.version_is_valid(version));

	ubo_size = gen_code.uniform_total_size;
	ubo_offsets = gen_code.uniform_offsets;
	texture_uniforms = gen_code.texture_uniforms;
	valid = true;
}

bool TexBlitShaderData::is_animated() const { return false; }

bool TexBlitShaderData::casts_shadows() const { return false; }

RenderingServerTypes::ShaderNativeSourceCode TexBlitShaderData::get_native_source_code() const
{
	return MaterialStorage::shaders->tex_blit_shader.version_get_native_source_code(
		version);
}

TexBlitShaderData::TexBlitShaderData() { valid = false; }

TexBlitShaderData::~TexBlitShaderData()
{
	if (version.is_valid()) {
		MaterialStorage::shaders->tex_blit_shader.version_free(version);
	}
}

GLES3::ShaderData* GLES3::_create_tex_blit_shader_func()
{
	TexBlitShaderData* shader_data = memnew(TexBlitShaderData);
	return shader_data;
}

void TexBlitMaterialData::bind_uniforms()
{
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, uniform_buffer);

	bind_uniforms_generic(texture_cache, shader_data->texture_uniforms, 1);
}

TexBlitMaterialData::~TexBlitMaterialData() {}

GLES3::MaterialData* GLES3::_create_tex_blit_material_func(ShaderData* p_shader)
{
	TexBlitMaterialData* material_data = memnew(TexBlitMaterialData);
	material_data->shader_data = static_cast<TexBlitShaderData*>(p_shader);
	// update will happen later anyway so do nothing.
	return material_data;
}

void GLES3::MaterialStorage::global_shader_parameters_load_settings(bool p_load_uniforms) {}

void GLES3::MaterialStorage::material_free(RID p_rid) {}

void GLES3::MaterialStorage::initialize()
{
	shaders = memnew(Shaders);
	shader_data_request_func[RSE::SHADER_CANVAS_ITEM] = _create_canvas_shader_func;
	material_data_request_func[RSE::SHADER_CANVAS_ITEM] = _create_canvas_material_func;

	shader_data_request_func[RSE::SHADER_SPATIAL] = _create_scene_shader_func;
	material_data_request_func[RSE::SHADER_SPATIAL] = _create_scene_material_func;

	shader_data_request_func[RSE::SHADER_PARTICLES] = _create_particles_shader_func;
	material_data_request_func[RSE::SHADER_PARTICLES] = _create_particles_material_func;

	shader_data_request_func[RSE::SHADER_SKY] = _create_sky_shader_func;
	material_data_request_func[RSE::SHADER_SKY] = _create_sky_material_func;

	shader_data_request_func[RSE::SHADER_FOG] = nullptr;
	material_data_request_func[RSE::SHADER_FOG] = nullptr;

	shaders->canvas_shader.initialize();
	shaders->sky_shader.initialize();
	shaders->scene_shader.initialize();
	shaders->particles_process_shader.initialize();
	shaders->tex_blit_shader.initialize();

	// Canvas
	ShaderCompiler::DefaultIdentifierActions actions_canvas;
	actions_canvas.renames["VERTEX"] = "vertex";
	actions_canvas.renames["COLOR"] = "color";
	actions_canvas.renames["POINT_SIZE"] = "point_size";
	actions_canvas.renames["UV"] = "uv";
	actions_canvas.renames["CUSTOM0"] = "custom0";
	actions_canvas.renames["CUSTOM1"] = "custom1";
	shaders->compiler_canvas.initialize(actions_canvas);

	// Scene / Spatial
	ShaderCompiler::DefaultIdentifierActions actions_scene;
	actions_scene.renames["VERTEX"] = "vertex";
	actions_scene.renames["NORMAL"] = "normal";
	actions_scene.renames["TANGENT"] = "tangent";
	actions_scene.renames["BINORMAL"] = "binormal";
	actions_scene.renames["COLOR"] = "color";
	actions_scene.renames["UV"] = "uv";
	actions_scene.renames["UV2"] = "uv2";
	actions_scene.renames["CUSTOM0"] = "custom0";
	actions_scene.renames["CUSTOM1"] = "custom1";
	actions_scene.renames["CUSTOM2"] = "custom2";
	actions_scene.renames["CUSTOM3"] = "custom3";
	actions_scene.renames["BONES"] = "bones";
	actions_scene.renames["WEIGHTS"] = "weights";
	actions_scene.renames["POINT_SIZE"] = "point_size";
	shaders->compiler_scene.initialize(actions_scene);

	// Particles
	ShaderCompiler::DefaultIdentifierActions actions_particles;
	shaders->compiler_particles.initialize(actions_particles);

	// Sky
	ShaderCompiler::DefaultIdentifierActions actions_sky;
	shaders->compiler_sky.initialize(actions_sky);

	// Tex Blit
	ShaderCompiler::DefaultIdentifierActions actions_tex_blit;
	actions_tex_blit.renames["VERTEX"] = "vertex";
	actions_tex_blit.renames["UV"] = "uv";
	shaders->compiler_tex_blit.initialize(actions_tex_blit);
}

void GLES3::MaterialStorage::finalize()
{
	if (global_shader_uniforms.buffer != 0) {
		glDeleteBuffers(1, &global_shader_uniforms.buffer);
		global_shader_uniforms.buffer = 0;
	}
	if (global_shader_uniforms.buffer_values) {
		memfree(global_shader_uniforms.buffer_values);
		global_shader_uniforms.buffer_values = nullptr;
	}
	if (global_shader_uniforms.buffer_usage) {
		memfree(global_shader_uniforms.buffer_usage);
		global_shader_uniforms.buffer_usage = nullptr;
	}
	if (global_shader_uniforms.buffer_dirty_regions) {
		memfree(global_shader_uniforms.buffer_dirty_regions);
		global_shader_uniforms.buffer_dirty_regions = nullptr;
	}
	if (shaders) {
        memdelete(shaders);
        shaders = nullptr;
    }
}

#endif // !GLES3_ENABLED


