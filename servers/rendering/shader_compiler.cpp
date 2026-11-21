/**************************************************************************/
/*  shader_compiler.cpp                                                   */
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

#include "servers/rendering/renderer.h"
#include "servers/rendering/shader_types.h"
#include "servers/rendering/shader_compiler.h"
#include "core/io/resource_loader.h"
#include "core/os/os.h"
#include "servers/rendering/shader_preprocessor.h"

#ifdef DEV_ENABLED
#include "scene/resources/shader.h"
#endif
#include "servers/rendering/shader_include_db.h"

// --- Merged from servers/rendering/shader_compiler.cpp ---

#define SL ShaderLanguage

static String _mktab(int p_level) { return String("\t").repeat(p_level); }

static String _typestr(SL::DataType p_type)
{
	String type = ShaderLanguage::get_datatype_name(p_type);
	if (!Renderer::is_low_end() && ShaderLanguage::is_sampler_type(p_type)) {
		type = type.replace(
			"sampler", "texture"); // we use textures instead of samplers in Vulkan GLSL
	}
	return type;
}

static int _get_datatype_alignment(SL::DataType p_type)
{
	switch (p_type) {
	case SL::TYPE_VOID:
		return 0;
	case SL::TYPE_BOOL:
		return 4;
	case SL::TYPE_BVEC2:
		return 8;
	case SL::TYPE_BVEC3:
		return 16;
	case SL::TYPE_BVEC4:
		return 16;
	case SL::TYPE_INT:
		return 4;
	case SL::TYPE_IVEC2:
		return 8;
	case SL::TYPE_IVEC3:
		return 16;
	case SL::TYPE_IVEC4:
		return 16;
	case SL::TYPE_UINT:
		return 4;
	case SL::TYPE_UVEC2:
		return 8;
	case SL::TYPE_UVEC3:
		return 16;
	case SL::TYPE_UVEC4:
		return 16;
	case SL::TYPE_FLOAT:
		return 4;
	case SL::TYPE_VEC2:
		return 8;
	case SL::TYPE_VEC3:
		return 16;
	case SL::TYPE_VEC4:
		return 16;
	case SL::TYPE_MAT2:
		return 16;
	case SL::TYPE_MAT3:
		return 16;
	case SL::TYPE_MAT4:
		return 16;
	case SL::TYPE_SAMPLER2D:
		return 16;
	case SL::TYPE_ISAMPLER2D:
		return 16;
	case SL::TYPE_USAMPLER2D:
		return 16;
	case SL::TYPE_SAMPLER2DARRAY:
		return 16;
	case SL::TYPE_ISAMPLER2DARRAY:
		return 16;
	case SL::TYPE_USAMPLER2DARRAY:
		return 16;
	case SL::TYPE_SAMPLER3D:
		return 16;
	case SL::TYPE_ISAMPLER3D:
		return 16;
	case SL::TYPE_USAMPLER3D:
		return 16;
	case SL::TYPE_SAMPLERCUBE:
		return 16;
	case SL::TYPE_SAMPLERCUBEARRAY:
		return 16;
	case SL::TYPE_SAMPLEREXT:
		return 16;
	case SL::TYPE_STRUCT:
		return 0;
	case SL::TYPE_MAX: {
		ERR_FAIL_V(0);
	}
	}

	ERR_FAIL_V(0);
}

static String _interpstr(SL::DataInterpolation p_interp)
{
	switch (p_interp) {
	case SL::INTERPOLATION_FLAT:
		return "flat ";
	case SL::INTERPOLATION_SMOOTH:
		return "";
	case SL::INTERPOLATION_DEFAULT:
		return "";
	}
	return "";
}

static String _prestr(SL::DataPrecision p_pres, bool p_force_highp = false)
{
	switch (p_pres) {
	case SL::PRECISION_LOWP:
		return "lowp ";
	case SL::PRECISION_MEDIUMP:
		return "mediump ";
	case SL::PRECISION_HIGHP:
		return "highp ";
	case SL::PRECISION_DEFAULT:
		return p_force_highp ? "highp " : "";
	}
	return "";
}

static String _constr(bool p_is_const)
{
	if (p_is_const) {
		return "const ";
	}
	return "";
}

static String _qualstr(SL::ArgumentQualifier p_qual)
{
	switch (p_qual) {
	case SL::ARGUMENT_QUALIFIER_IN:
		return "";
	case SL::ARGUMENT_QUALIFIER_OUT:
		return "out ";
	case SL::ARGUMENT_QUALIFIER_INOUT:
		return "inout ";
	}
	return "";
}

static String _opstr(SL::Operator p_op) { return SL::get_operator_text(p_op); }

static String _mkid(const String& p_id)
{
	String id = "m_" + p_id.replace("__", "_dus_");
	return id.replace("__", "_dus_"); // doubleunderscore is reserved in glsl
}

static String f2sp0(float p_float)
{
	String num = rtos(p_float);
	if (!num.contains_char('.') && !num.contains_char('e')) {
		num += ".0";
	}
	return num;
}

static String get_constant_text(SL::DataType p_type, const Vector<SL::Scalar>& p_values)
{
	switch (p_type) {
	case SL::TYPE_BOOL:
		return p_values[0].boolean ? "true" : "false";
	case SL::TYPE_BVEC2:
	case SL::TYPE_BVEC3:
	case SL::TYPE_BVEC4: {
		String text = "bvec" + itos(p_type - SL::TYPE_BOOL + 1) + "(";
		for (int i = 0; i < p_values.size(); i++) {
			if (i > 0) {
				text += ",";
			}

			text += p_values[i].boolean ? "true" : "false";
		}
		text += ")";
		return text;
	}

	case SL::TYPE_INT:
		return itos(p_values[0].sint);
	case SL::TYPE_IVEC2:
	case SL::TYPE_IVEC3:
	case SL::TYPE_IVEC4: {
		String text = "ivec" + itos(p_type - SL::TYPE_INT + 1) + "(";
		for (int i = 0; i < p_values.size(); i++) {
			if (i > 0) {
				text += ",";
			}

			text += itos(p_values[i].sint);
		}
		text += ")";
		return text;

	} break;
	case SL::TYPE_UINT:
		return itos(p_values[0].uint) + "u";
	case SL::TYPE_UVEC2:
	case SL::TYPE_UVEC3:
	case SL::TYPE_UVEC4: {
		String text = "uvec" + itos(p_type - SL::TYPE_UINT + 1) + "(";
		for (int i = 0; i < p_values.size(); i++) {
			if (i > 0) {
				text += ",";
			}

			text += itos(p_values[i].uint) + "u";
		}
		text += ")";
		return text;
	} break;
	case SL::TYPE_FLOAT:
		return f2sp0(p_values[0].real);
	case SL::TYPE_VEC2:
	case SL::TYPE_VEC3:
	case SL::TYPE_VEC4: {
		String text = "vec" + itos(p_type - SL::TYPE_FLOAT + 1) + "(";
		for (int i = 0; i < p_values.size(); i++) {
			if (i > 0) {
				text += ",";
			}

			text += f2sp0(p_values[i].real);
		}
		text += ")";
		return text;

	} break;
	case SL::TYPE_MAT2:
	case SL::TYPE_MAT3:
	case SL::TYPE_MAT4: {
		String text = "mat" + itos(p_type - SL::TYPE_MAT2 + 2) + "(";
		for (int i = 0; i < p_values.size(); i++) {
			if (i > 0) {
				text += ",";
			}

			text += f2sp0(p_values[i].real);
		}
		text += ")";
		return text;

	} break;
	default:
		ERR_FAIL_V(String());
	}
}

String ShaderCompiler::_get_sampler_name(
	ShaderLanguage::TextureFilter p_filter, ShaderLanguage::TextureRepeat p_repeat)
{
	if (p_filter == ShaderLanguage::FILTER_DEFAULT) {
		ERR_FAIL_COND_V(actions.default_filter == ShaderLanguage::FILTER_DEFAULT, String());
		p_filter = actions.default_filter;
	}
	if (p_repeat == ShaderLanguage::REPEAT_DEFAULT) {
		ERR_FAIL_COND_V(actions.default_repeat == ShaderLanguage::REPEAT_DEFAULT, String());
		p_repeat = actions.default_repeat;
	}
	constexpr const char* name_mapping[] = {"SAMPLER_NEAREST_CLAMP", "SAMPLER_LINEAR_CLAMP",
		"SAMPLER_NEAREST_WITH_MIPMAPS_CLAMP", "SAMPLER_LINEAR_WITH_MIPMAPS_CLAMP",
		"SAMPLER_NEAREST_WITH_MIPMAPS_ANISOTROPIC_CLAMP",
		"SAMPLER_LINEAR_WITH_MIPMAPS_ANISOTROPIC_CLAMP", "SAMPLER_NEAREST_REPEAT",
		"SAMPLER_LINEAR_REPEAT", "SAMPLER_NEAREST_WITH_MIPMAPS_REPEAT",
		"SAMPLER_LINEAR_WITH_MIPMAPS_REPEAT", "SAMPLER_NEAREST_WITH_MIPMAPS_ANISOTROPIC_REPEAT",
		"SAMPLER_LINEAR_WITH_MIPMAPS_ANISOTROPIC_REPEAT"};
	return String(name_mapping[p_filter + (p_repeat == ShaderLanguage::REPEAT_ENABLE
												  ? ShaderLanguage::FILTER_DEFAULT
												  : 0)]);
}

void ShaderCompiler::_dump_function_deps(const SL::ShaderNode* p_node, const String& p_for_func,
	const HashMap<String, String>& p_func_code, String& r_to_add, HashSet<String>& added)
{
	int fidx = -1;

	for (int i = 0; i < p_node->vfunctions.size(); i++) {
		if (p_node->vfunctions[i].name == p_for_func) {
			fidx = i;
			break;
		}
	}

	ERR_FAIL_COND(fidx == -1);

	Vector<String> uses_functions;

	for (const String& E : p_node->vfunctions[fidx].uses_function) {
		uses_functions.push_back(E);
	}
	uses_functions.sort(); // ensure order is deterministic so the
														   // same shader is always produced

	for (int k = 0; k < uses_functions.size(); k++) {
		if (added.has(uses_functions[k])) {
			continue; // was added already
		}

		_dump_function_deps(p_node, uses_functions[k], p_func_code, r_to_add, added);

		SL::FunctionNode* fnode = nullptr;

		for (int i = 0; i < p_node->vfunctions.size(); i++) {
			if (p_node->vfunctions[i].name == uses_functions[k]) {
				fnode = p_node->vfunctions[i].function;
				break;
			}
		}

		ERR_FAIL_NULL(fnode);

		r_to_add += "\n";

		String header;
		if (fnode->return_type == SL::TYPE_STRUCT) {
			header = _mkid(fnode->return_struct_name);
		}
		else {
			header = _typestr(fnode->return_type);
		}

		if (fnode->return_array_size > 0) {
			header += "[";
			header += itos(fnode->return_array_size);
			header += "]";
		}

		header += " ";
		header += _mkid(fnode->rname);
		header += "(";

		for (int i = 0; i < fnode->arguments.size(); i++) {
			if (i > 0) {
				header += ", ";
			}
			header += _constr(fnode->arguments[i].is_const);
			if (fnode->arguments[i].type == SL::TYPE_STRUCT) {
				header += _qualstr(fnode->arguments[i].qualifier) +
						  _mkid(fnode->arguments[i].struct_name) + " " +
						  _mkid(fnode->arguments[i].name);
			}
			else {
				header += _qualstr(fnode->arguments[i].qualifier) +
						  _prestr(fnode->arguments[i].precision) +
						  _typestr(fnode->arguments[i].type) + " " +
						  _mkid(fnode->arguments[i].name);
			}
			if (fnode->arguments[i].array_size > 0) {
				header += "[";
				header += itos(fnode->arguments[i].array_size);
				header += "]";
			}
		}

		header += ")\n";
		r_to_add += header;
		r_to_add += p_func_code[uses_functions[k]];

		added.insert(uses_functions[k]);
	}
}

static String _get_global_shader_uniform_from_type_and_index(
	const String& p_buffer, const String& p_index, ShaderLanguage::DataType p_type)
{
	switch (p_type) {
	case ShaderLanguage::TYPE_BOOL: {
		return "bool(floatBitsToUint(" + p_buffer + "[" + p_index + "].x))";
	}
	case ShaderLanguage::TYPE_BVEC2: {
		return "bvec2(floatBitsToUint(" + p_buffer + "[" + p_index + "].xy))";
	}
	case ShaderLanguage::TYPE_BVEC3: {
		return "bvec3(floatBitsToUint(" + p_buffer + "[" + p_index + "].xyz))";
	}
	case ShaderLanguage::TYPE_BVEC4: {
		return "bvec4(floatBitsToUint(" + p_buffer + "[" + p_index + "].xyzw))";
	}
	case ShaderLanguage::TYPE_INT: {
		return "floatBitsToInt(" + p_buffer + "[" + p_index + "].x)";
	}
	case ShaderLanguage::TYPE_IVEC2: {
		return "floatBitsToInt(" + p_buffer + "[" + p_index + "].xy)";
	}
	case ShaderLanguage::TYPE_IVEC3: {
		return "floatBitsToInt(" + p_buffer + "[" + p_index + "].xyz)";
	}
	case ShaderLanguage::TYPE_IVEC4: {
		return "floatBitsToInt(" + p_buffer + "[" + p_index + "].xyzw)";
	}
	case ShaderLanguage::TYPE_UINT: {
		return "floatBitsToUint(" + p_buffer + "[" + p_index + "].x)";
	}
	case ShaderLanguage::TYPE_UVEC2: {
		return "floatBitsToUint(" + p_buffer + "[" + p_index + "].xy)";
	}
	case ShaderLanguage::TYPE_UVEC3: {
		return "floatBitsToUint(" + p_buffer + "[" + p_index + "].xyz)";
	}
	case ShaderLanguage::TYPE_UVEC4: {
		return "floatBitsToUint(" + p_buffer + "[" + p_index + "].xyzw)";
	}
	case ShaderLanguage::TYPE_FLOAT: {
		return "(" + p_buffer + "[" + p_index + "].x)";
	}
	case ShaderLanguage::TYPE_VEC2: {
		return "(" + p_buffer + "[" + p_index + "].xy)";
	}
	case ShaderLanguage::TYPE_VEC3: {
		return "(" + p_buffer + "[" + p_index + "].xyz)";
	}
	case ShaderLanguage::TYPE_VEC4: {
		return "(" + p_buffer + "[" + p_index + "].xyzw)";
	}
	case ShaderLanguage::TYPE_MAT2: {
		return "mat2(" + p_buffer + "[" + p_index + "].xy," + p_buffer + "[" + p_index + "+1u].xy)";
	}
	case ShaderLanguage::TYPE_MAT3: {
		return "mat3(" + p_buffer + "[" + p_index + "].xyz," + p_buffer + "[" + p_index +
			   "+1u].xyz," + p_buffer + "[" + p_index + "+2u].xyz)";
	}
	case ShaderLanguage::TYPE_MAT4: {
		return "mat4(" + p_buffer + "[" + p_index + "].xyzw," + p_buffer + "[" + p_index +
			   "+1u].xyzw," + p_buffer + "[" + p_index + "+2u].xyzw," + p_buffer + "[" + p_index +
			   "+3u].xyzw)";
	}
	default: {
		ERR_FAIL_V("void");
	}
	}
}

String ShaderCompiler::_dump_node_code(const SL::Node* p_node, int p_level,
	GeneratedCode& r_gen_code, IdentifierActions& p_actions,
	const DefaultIdentifierActions& p_default_actions, bool p_assigning, bool p_use_scope)
{
	String code;

	switch (p_node->type) {
	case SL::Node::NODE_TYPE_SHADER: {
		SL::ShaderNode* pnode = (SL::ShaderNode*)p_node;

		// Render modes.

		for (int i = 0; i < pnode->render_modes.size(); i++) {
			if (p_default_actions.render_mode_defines.has(pnode->render_modes[i]) &&
				!used_rmode_defines.has(pnode->render_modes[i])) {
				r_gen_code.defines.push_back(
					p_default_actions.render_mode_defines[pnode->render_modes[i]]);
				used_rmode_defines.insert(pnode->render_modes[i]);
			}

			if (p_actions.render_mode_flags.has(pnode->render_modes[i])) {
				*p_actions.render_mode_flags[pnode->render_modes[i]] = true;
			}

			if (p_actions.render_mode_values.has(pnode->render_modes[i])) {
				Pair<int*, int>& p = p_actions.render_mode_values[pnode->render_modes[i]];
				*p.first = p.second;
			}
		}

		// Stencil modes.

		for (int i = 0; i < pnode->stencil_modes.size(); i++) {
			if (p_actions.stencil_mode_values.has(pnode->stencil_modes[i])) {
				Pair<int*, int>& p = p_actions.stencil_mode_values[pnode->stencil_modes[i]];
				*p.first = p.second;
			}
		}

		// Stencil reference value.

		if (p_actions.stencil_reference && pnode->stencil_reference != -1) {
			*p_actions.stencil_reference = pnode->stencil_reference;
		}

		// structs

		for (int i = 0; i < pnode->vstructs.size(); i++) {
			SL::StructNode* st = pnode->vstructs[i].shader_struct;
			String struct_code;

			struct_code += "struct ";
			struct_code += _mkid(pnode->vstructs[i].name);
			struct_code += " ";
			struct_code += "{\n";
			for (SL::MemberNode* m : st->members) {
				if (m->datatype == SL::TYPE_STRUCT) {
					struct_code += _mkid(m->struct_name);
				}
				else {
					struct_code += _prestr(m->precision);
					struct_code += _typestr(m->datatype);
				}
				struct_code += " ";
				struct_code += _mkid(m->name);
				if (m->array_size > 0) {
					struct_code += "[";
					struct_code += itos(m->array_size);
					struct_code += "]";
				}
				struct_code += ";\n";
			}
			struct_code += "}";
			struct_code += ";\n";

			for (int j = 0; j < STAGE_MAX; j++) {
				r_gen_code.stage_globals[j] += struct_code;
			}
		}

		int max_texture_uniforms = 0;
		int max_uniforms = 0;

		for (const KeyValue<String, SL::ShaderNode::Uniform>& E : pnode->uniforms) {
			if (SL::is_sampler_type(E.value.type)) {
				if (E.value.hint == SL::ShaderNode::Uniform::HINT_SCREEN_TEXTURE ||
					E.value.hint == SL::ShaderNode::Uniform::HINT_NORMAL_ROUGHNESS_TEXTURE ||
					E.value.hint == SL::ShaderNode::Uniform::HINT_DEPTH_TEXTURE ||
					E.value.hint == SL::ShaderNode::Uniform::HINT_BLIT_SOURCE0 ||
					E.value.hint == SL::ShaderNode::Uniform::HINT_BLIT_SOURCE1 ||
					E.value.hint == SL::ShaderNode::Uniform::HINT_BLIT_SOURCE2 ||
					E.value.hint == SL::ShaderNode::Uniform::HINT_BLIT_SOURCE3) {
					continue; // Don't create uniforms in the generated code for these.
				}
				max_texture_uniforms++;
			}
			else {
				if (E.value.scope == SL::ShaderNode::Uniform::SCOPE_INSTANCE) {
					continue; // Instances are indexed directly, don't need index uniforms.
				}

				max_uniforms++;
			}
		}

		r_gen_code.texture_uniforms.resize(max_texture_uniforms);

		Vector<int> uniform_sizes;
		Vector<int> uniform_alignments;
		Vector<String> uniform_defines;
		uniform_sizes.resize(max_uniforms);
		uniform_alignments.resize(max_uniforms);
		uniform_defines.resize(max_uniforms);
		bool uses_uniforms = false;

		Vector<String> uniform_names;

		for (const KeyValue<String, SL::ShaderNode::Uniform>& E : pnode->uniforms) {
			uniform_names.push_back(E.key);
		}

		uniform_names.sort(); // ensure order is deterministic so
															  // the same shader is always produced

		for (int k = 0; k < uniform_names.size(); k++) {
			const String& uniform_name = uniform_names[k];
			const SL::ShaderNode::Uniform& uniform = pnode->uniforms[uniform_name];

			String ucode;

			if (uniform.scope == SL::ShaderNode::Uniform::SCOPE_INSTANCE) {
				// insert, but don't generate any code.
				p_actions.uniforms->insert(uniform_name, uniform);
				continue; // Instances are indexed directly, don't need index uniforms.
			}

			if (uniform.hint == SL::ShaderNode::Uniform::HINT_SCREEN_TEXTURE ||
				uniform.hint == SL::ShaderNode::Uniform::HINT_NORMAL_ROUGHNESS_TEXTURE ||
				uniform.hint == SL::ShaderNode::Uniform::HINT_DEPTH_TEXTURE ||
				uniform.hint == SL::ShaderNode::Uniform::HINT_BLIT_SOURCE0 ||
				uniform.hint == SL::ShaderNode::Uniform::HINT_BLIT_SOURCE1 ||
				uniform.hint == SL::ShaderNode::Uniform::HINT_BLIT_SOURCE2 ||
				uniform.hint == SL::ShaderNode::Uniform::HINT_BLIT_SOURCE3) {
				continue; // Don't create uniforms in the generated code for these.
			}

			if (SL::is_sampler_type(uniform.type)) {
				// Texture layouts are different for OpenGL GLSL and Vulkan GLSL
				if (!Renderer::is_low_end()) {
					ucode = "layout(set = " + itos(actions.texture_layout_set) + ", binding = " +
							itos(actions.base_texture_binding_index + uniform.texture_binding) +
							") ";
				}
				ucode += "uniform ";
			}

			bool is_buffer_global = !SL::is_sampler_type(uniform.type) &&
									uniform.scope == SL::ShaderNode::Uniform::SCOPE_GLOBAL;

			if (is_buffer_global) {
				// this is an integer to index the global table
				ucode += _typestr(ShaderLanguage::TYPE_UINT);
			}
			else {
				ucode += _prestr(uniform.precision, ShaderLanguage::is_float_type(uniform.type));
				ucode += _typestr(uniform.type);
			}

			ucode += " " + _mkid(uniform_name);
			if (uniform.array_size > 0) {
				ucode += "[";
				ucode += itos(uniform.array_size);
				ucode += "]";
			}
			ucode += ";\n";
			if (SL::is_sampler_type(uniform.type)) {
				for (int j = 0; j < STAGE_MAX; j++) {
					r_gen_code.stage_globals[j] += ucode;
				}

				GeneratedCode::Texture texture;
				texture.name = uniform_name;
				texture.hint = uniform.hint;
				texture.type = uniform.type;
				texture.use_color = uniform.use_color;
				texture.filter = uniform.filter;
				texture.repeat = uniform.repeat;
				texture.global = uniform.scope == ShaderLanguage::ShaderNode::Uniform::SCOPE_GLOBAL;
				texture.array_size = uniform.array_size;
				if (texture.global) {
					r_gen_code.uses_global_textures = true;
				}

				r_gen_code.texture_uniforms.write[uniform.texture_order] = texture;
			}
			else {
				if (!uses_uniforms) {
					uses_uniforms = true;
				}
				uniform_defines.write[uniform.order] = ucode;
				if (is_buffer_global) {
					// globals are indices into the global table
					uniform_sizes.write[uniform.order] =
						ShaderLanguage::get_datatype_size(ShaderLanguage::TYPE_UINT);
					uniform_alignments.write[uniform.order] =
						_get_datatype_alignment(ShaderLanguage::TYPE_UINT);
				}
				else {
					// The following code enforces a 16-byte alignment of uniform arrays.
					if (uniform.array_size > 0) {
						int size =
							ShaderLanguage::get_datatype_size(uniform.type) * uniform.array_size;
						int m = (16 * uniform.array_size);
						if ((size % m) != 0) {
							size += m - (size % m);
						}
						uniform_sizes.write[uniform.order] = size;
						uniform_alignments.write[uniform.order] = 16;
					}
					else {
						uniform_sizes.write[uniform.order] =
							ShaderLanguage::get_datatype_size(uniform.type);
						uniform_alignments.write[uniform.order] =
							_get_datatype_alignment(uniform.type);
					}
				}
			}

			p_actions.uniforms->insert(uniform_name, uniform);
		}

		for (int i = 0; i < max_uniforms; i++) {
			r_gen_code.uniforms += uniform_defines[i];
		}

		// add up
		int offset = 0;
		for (int i = 0; i < uniform_sizes.size(); i++) {
			int align = offset % uniform_alignments[i];

			if (align != 0) {
				offset += uniform_alignments[i] - align;
			}

			r_gen_code.uniform_offsets.push_back(offset);

			offset += uniform_sizes[i];
		}

		r_gen_code.uniform_total_size = offset;

		if (r_gen_code.uniform_total_size % 16 != 0) { // UBO sizes must be multiples of 16
			r_gen_code.uniform_total_size += 16 - (r_gen_code.uniform_total_size % 16);
		}

		uint32_t index = p_default_actions.base_varying_index;

		List<Pair<String, SL::ShaderNode::Varying>> var_frag_to_light;

		Vector<String> varying_names;

		for (const KeyValue<String, SL::ShaderNode::Varying>& E : pnode->varyings) {
			varying_names.push_back(E.key);
		}

		varying_names.sort(); // ensure order is deterministic so
															  // the same shader is always produced

		for (int k = 0; k < varying_names.size(); k++) {
			const String& varying_name = varying_names[k];
			const SL::ShaderNode::Varying& varying = pnode->varyings[varying_name];

			if (varying.stage == SL::ShaderNode::Varying::STAGE_FRAGMENT) {
				var_frag_to_light.push_back(
					Pair<String, SL::ShaderNode::Varying>(varying_name, varying));
				fragment_varyings.insert(varying_name);
				continue;
			}
			if (varying.type < SL::TYPE_INT) {
				continue; // Ignore boolean types to prevent crashing (if varying is just declared).
			}

			String vcode;
			String interp_mode = _interpstr(varying.interpolation);
			vcode += _prestr(varying.precision, ShaderLanguage::is_float_type(varying.type));
			vcode += _typestr(varying.type);
			vcode += " " + _mkid(varying_name);
			uint32_t inc = varying.get_size();

			if (varying.array_size > 0) {
				vcode += "[";
				vcode += itos(varying.array_size);
				vcode += "]";
			}

			vcode += ";\n";
			// GLSL ES 3.0 does not allow layout qualifiers for varyings
			if (!Renderer::is_low_end()) {
				r_gen_code.stage_globals[STAGE_VERTEX] += "layout(location=" + itos(index) + ") ";
				r_gen_code.stage_globals[STAGE_FRAGMENT] += "layout(location=" + itos(index) + ") ";
			}
			r_gen_code.stage_globals[STAGE_VERTEX] += interp_mode + "out " + vcode;
			r_gen_code.stage_globals[STAGE_FRAGMENT] += interp_mode + "in " + vcode;

			index += inc;
		}

		if (var_frag_to_light.size() > 0) {
			String gcode = "\n\nstruct {\n";
			for (const Pair<String, SL::ShaderNode::Varying>& E : var_frag_to_light) {
				gcode += "\t" + _prestr(E.second.precision) + _typestr(E.second.type) + " " +
						 _mkid(E.first);
				if (E.second.array_size > 0) {
					gcode += "[";
					gcode += itos(E.second.array_size);
					gcode += "]";
				}
				gcode += ";\n";
			}
			gcode += "} frag_to_light;\n";
			r_gen_code.stage_globals[STAGE_FRAGMENT] += gcode;
		}

		for (int i = 0; i < pnode->vconstants.size(); i++) {
			const SL::ShaderNode::Constant& cnode = pnode->vconstants[i];
			String gcode;
			gcode += _constr(true);
			gcode += _prestr(cnode.precision, ShaderLanguage::is_float_type(cnode.type));
			if (cnode.type == SL::TYPE_STRUCT) {
				gcode += _mkid(cnode.struct_name);
			}
			else {
				gcode += _typestr(cnode.type);
			}
			gcode += " " + _mkid(String(cnode.name));
			if (cnode.array_size > 0) {
				gcode += "[";
				gcode += itos(cnode.array_size);
				gcode += "]";
			}
			gcode += "=";
			gcode += _dump_node_code(
				cnode.initializer, p_level, r_gen_code, p_actions, p_default_actions, p_assigning);
			gcode += ";\n";
			for (int j = 0; j < STAGE_MAX; j++) {
				r_gen_code.stage_globals[j] += gcode;
			}
		}

		HashMap<String, String> function_code;

		// code for functions
		for (int i = 0; i < pnode->vfunctions.size(); i++) {
			SL::FunctionNode* fnode = pnode->vfunctions[i].function;
			function = fnode;
			current_func_name = fnode->name;
			function_code[fnode->name] = _dump_node_code(
				fnode->body, p_level + 1, r_gen_code, p_actions, p_default_actions, p_assigning);
			function = nullptr;
		}

		// place functions in actual code

		HashSet<String> added_funcs_per_stage[STAGE_MAX];

		for (int i = 0; i < pnode->vfunctions.size(); i++) {
			SL::FunctionNode* fnode = pnode->vfunctions[i].function;

			function = fnode;

			current_func_name = fnode->name;

			if (p_actions.entry_point_stages.has(fnode->name)) {
				Stage stage = p_actions.entry_point_stages[fnode->name];
				_dump_function_deps(pnode, fnode->name, function_code,
					r_gen_code.stage_globals[stage], added_funcs_per_stage[stage]);
				r_gen_code.code[fnode->name] = function_code[fnode->name];
			}

			function = nullptr;
		}

		// code+=dump_node_code(pnode->body,p_level);
	} break;
	case SL::Node::NODE_TYPE_STRUCT: {
	} break;
	case SL::Node::NODE_TYPE_FUNCTION: {
	} break;
	case SL::Node::NODE_TYPE_BLOCK: {
		SL::BlockNode* bnode = (SL::BlockNode*)p_node;

		// variables
		if (!bnode->single_statement) {
			code += _mktab(p_level - 1) + "{\n";
		}

		int i = 0;
		for (List<ShaderLanguage::Node*>::ConstIterator itr = bnode->statements.begin();
			 itr != bnode->statements.end(); ++itr, ++i) {
			String scode = _dump_node_code(
				*itr, p_level, r_gen_code, p_actions, p_default_actions, p_assigning);

			if ((*itr)->type == SL::Node::NODE_TYPE_CONTROL_FLOW || bnode->single_statement) {
				code += scode; // use directly
				if (bnode->use_comma_between_statements && i + 1 < bnode->statements.size()) {
					code += ",";
				}
			}
			else {
				code += _mktab(p_level) + scode + ";\n";
			}
		}
		if (!bnode->single_statement) {
			code += _mktab(p_level - 1) + "}\n";
		}

	} break;
	case SL::Node::NODE_TYPE_VARIABLE_DECLARATION: {
		SL::VariableDeclarationNode* vdnode = (SL::VariableDeclarationNode*)p_node;

		String declaration;
		declaration += _constr(vdnode->is_const);
		if (vdnode->datatype == SL::TYPE_STRUCT) {
			declaration += _mkid(vdnode->struct_name);
		}
		else {
			declaration += _prestr(vdnode->precision) + _typestr(vdnode->datatype);
		}
		declaration += " ";
		for (int i = 0; i < vdnode->declarations.size(); i++) {
			bool is_array = vdnode->declarations[i].size > 0;
			if (i > 0) {
				declaration += ",";
			}
			declaration += _mkid(vdnode->declarations[i].name);
			if (is_array) {
				declaration += "[";
				if (vdnode->declarations[i].size_expression != nullptr) {
					declaration += _dump_node_code(vdnode->declarations[i].size_expression, p_level,
						r_gen_code, p_actions, p_default_actions, p_assigning);
				}
				else {
					declaration += itos(vdnode->declarations[i].size);
				}
				declaration += "]";
			}

			if (!is_array || vdnode->declarations[i].single_expression) {
				if (!vdnode->declarations[i].initializer.is_empty()) {
					declaration += "=";
					declaration += _dump_node_code(vdnode->declarations[i].initializer[0], p_level,
						r_gen_code, p_actions, p_default_actions, p_assigning);
				}
			}
			else {
				int size = vdnode->declarations[i].initializer.size();
				if (size > 0) {
					declaration += "=";
					if (vdnode->datatype == SL::TYPE_STRUCT) {
						declaration += _mkid(vdnode->struct_name);
					}
					else {
						declaration += _typestr(vdnode->datatype);
					}
					declaration += "[";
					declaration += itos(size);
					declaration += "]";
					declaration += "(";
					for (int j = 0; j < size; j++) {
						if (j > 0) {
							declaration += ",";
						}
						declaration += _dump_node_code(vdnode->declarations[i].initializer[j],
							p_level, r_gen_code, p_actions, p_default_actions, p_assigning);
					}
					declaration += ")";
				}
			}
		}

		code += declaration;
	} break;
	case SL::Node::NODE_TYPE_VARIABLE: {
		SL::VariableNode* vnode = (SL::VariableNode*)p_node;
		bool use_fragment_varying = false;

		if (!vnode->is_local &&
			!(p_actions.entry_point_stages.has(current_func_name) &&
				p_actions.entry_point_stages[current_func_name] == STAGE_VERTEX)) {
			if (p_assigning) {
				if (shader->varyings.has(vnode->name)) {
					use_fragment_varying = true;
				}
			}
			else {
				if (fragment_varyings.has(vnode->name)) {
					use_fragment_varying = true;
				}
			}
		}

		if (p_assigning && p_actions.write_flag_pointers.has(vnode->name)) {
			*p_actions.write_flag_pointers[vnode->name] = true;
		}

		if (p_default_actions.usage_defines.has(vnode->name) &&
			!used_name_defines.has(vnode->name)) {
			String define = p_default_actions.usage_defines[vnode->name];
			if (define.begins_with("@")) {
				define = p_default_actions.usage_defines[define.substr(1)];
			}
			r_gen_code.defines.push_back(define);
			used_name_defines.insert(vnode->name);
		}

		if (p_actions.usage_flag_pointers.has(vnode->name) &&
			!used_flag_pointers.has(vnode->name)) {
			*p_actions.usage_flag_pointers[vnode->name] = true;
			used_flag_pointers.insert(vnode->name);
		}

		if (p_default_actions.renames.has(vnode->name)) {
			code = p_default_actions.renames[vnode->name];
		}
		else {
			bool param_found = false;
			if (function) {
				for (const SL::FunctionNode::Argument& argument : function->arguments) {
					if (argument.name == vnode->name) {
						param_found = true;
						break;
					}
				}
			}
			if (!param_found && shader->uniforms.has(vnode->name)) {
				// its a uniform!
				const ShaderLanguage::ShaderNode::Uniform& u = shader->uniforms[vnode->name];
				if (u.is_texture()) {
					String name;
					if (u.hint == ShaderLanguage::ShaderNode::Uniform::HINT_SCREEN_TEXTURE) {
						name = "color_buffer";
						if (u.filter >= ShaderLanguage::FILTER_NEAREST_MIPMAP) {
							r_gen_code.uses_screen_texture_mipmaps = true;
						}
						r_gen_code.uses_screen_texture = true;
					}
					else if (u.hint ==
							   ShaderLanguage::ShaderNode::Uniform::HINT_NORMAL_ROUGHNESS_TEXTURE) {
						name = "normal_roughness_buffer";
						r_gen_code.uses_normal_roughness_texture = true;
					}
					else if (u.hint == ShaderLanguage::ShaderNode::Uniform::HINT_DEPTH_TEXTURE) {
						name = "depth_buffer";
						r_gen_code.uses_depth_texture = true;
					}
					else if (u.hint == ShaderLanguage::ShaderNode::Uniform::HINT_BLIT_SOURCE0) {
						name = "source0";
					}
					else if (u.hint == ShaderLanguage::ShaderNode::Uniform::HINT_BLIT_SOURCE1) {
						name = "source1";
					}
					else if (u.hint == ShaderLanguage::ShaderNode::Uniform::HINT_BLIT_SOURCE2) {
						name = "source2";
					}
					else if (u.hint == ShaderLanguage::ShaderNode::Uniform::HINT_BLIT_SOURCE3) {
						name = "source3";
					}
					else {
						name = _mkid(vnode->name); // texture, use as is
					}

					code = name;
				}
				else {
					// a scalar or vector
					if (u.scope == ShaderLanguage::ShaderNode::Uniform::SCOPE_GLOBAL) {
						code =
							actions.base_uniform_string + _mkid(vnode->name); // texture, use as is
						// global variable, this means the code points to an index to the global
						// table
						code = _get_global_shader_uniform_from_type_and_index(
							p_default_actions.global_buffer_array_variable, code, u.type);
					}
					else if (u.scope == ShaderLanguage::ShaderNode::Uniform::SCOPE_INSTANCE) {
						// instance variable, index it as such
						code = "(" + p_default_actions.instance_uniform_index_variable + "+" +
							   itos(u.instance_index) + "u)";
						code = _get_global_shader_uniform_from_type_and_index(
							p_default_actions.global_buffer_array_variable, code, u.type);
					}
					else {
						// regular uniform, index from UBO
						code = actions.base_uniform_string + _mkid(vnode->name);
					}
				}

			}
			else {
				if (use_fragment_varying) {
					code = "frag_to_light.";
				}
				code += _mkid(vnode->name); // its something else (local var most likely) use as is
			}
		}

		if (vnode->name == time_name) {
			if (p_actions.entry_point_stages.has(current_func_name) &&
				p_actions.entry_point_stages[current_func_name] == STAGE_VERTEX) {
				r_gen_code.uses_vertex_time = true;
			}
			if (p_actions.entry_point_stages.has(current_func_name) &&
				p_actions.entry_point_stages[current_func_name] == STAGE_FRAGMENT) {
				r_gen_code.uses_fragment_time = true;
			}
		}

	} break;
	case SL::Node::NODE_TYPE_ARRAY_CONSTRUCT: {
		SL::ArrayConstructNode* acnode = (SL::ArrayConstructNode*)p_node;
		int sz = acnode->initializer.size();
		if (acnode->datatype == SL::TYPE_STRUCT) {
			code += _mkid(acnode->struct_name);
		}
		else {
			code += _typestr(acnode->datatype);
		}
		code += "[";
		code += itos(acnode->initializer.size());
		code += "]";
		code += "(";
		for (int i = 0; i < sz; i++) {
			code += _dump_node_code(acnode->initializer[i], p_level, r_gen_code, p_actions,
				p_default_actions, p_assigning);
			if (i != sz - 1) {
				code += ", ";
			}
		}
		code += ")";
	} break;
	case SL::Node::NODE_TYPE_ARRAY: {
		SL::ArrayNode* anode = (SL::ArrayNode*)p_node;
		bool use_fragment_varying = false;

		if (!anode->is_local &&
			!(p_actions.entry_point_stages.has(current_func_name) &&
				p_actions.entry_point_stages[current_func_name] == STAGE_VERTEX)) {
			if (anode->assign_expression != nullptr && shader->varyings.has(anode->name)) {
				use_fragment_varying = true;
			}
			else {
				if (p_assigning) {
					if (shader->varyings.has(anode->name)) {
						use_fragment_varying = true;
					}
				}
				else {
					if (fragment_varyings.has(anode->name)) {
						use_fragment_varying = true;
					}
				}
			}
		}

		if (p_assigning && p_actions.write_flag_pointers.has(anode->name)) {
			*p_actions.write_flag_pointers[anode->name] = true;
		}

		if (p_default_actions.usage_defines.has(anode->name) &&
			!used_name_defines.has(anode->name)) {
			String define = p_default_actions.usage_defines[anode->name];
			if (define.begins_with("@")) {
				define = p_default_actions.usage_defines[define.substr(1)];
			}
			r_gen_code.defines.push_back(define);
			used_name_defines.insert(anode->name);
		}

		if (p_actions.usage_flag_pointers.has(anode->name) &&
			!used_flag_pointers.has(anode->name)) {
			*p_actions.usage_flag_pointers[anode->name] = true;
			used_flag_pointers.insert(anode->name);
		}

		if (p_default_actions.renames.has(anode->name)) {
			code = p_default_actions.renames[anode->name];
		}
		else {
			bool param_found = false;
			if (function) {
				for (const SL::FunctionNode::Argument& argument : function->arguments) {
					if (argument.name == anode->name) {
						param_found = true;
						break;
					}
				}
			}
			if (!param_found && shader->uniforms.has(anode->name)) {
				// its a uniform!
				const ShaderLanguage::ShaderNode::Uniform& u = shader->uniforms[anode->name];
				if (u.is_texture()) {
					code = _mkid(anode->name); // texture, use as is
				}
				else {
					// a scalar or vector
					if (u.scope == ShaderLanguage::ShaderNode::Uniform::SCOPE_GLOBAL) {
						code =
							actions.base_uniform_string + _mkid(anode->name); // texture, use as is
						// global variable, this means the code points to an index to the global
						// table
						code = _get_global_shader_uniform_from_type_and_index(
							p_default_actions.global_buffer_array_variable, code, u.type);
					}
					else if (u.scope == ShaderLanguage::ShaderNode::Uniform::SCOPE_INSTANCE) {
						// instance variable, index it as such
						code = "(" + p_default_actions.instance_uniform_index_variable + "+" +
							   itos(u.instance_index) + "u)";
						code = _get_global_shader_uniform_from_type_and_index(
							p_default_actions.global_buffer_array_variable, code, u.type);
					}
					else {
						// regular uniform, index from UBO
						code = actions.base_uniform_string + _mkid(anode->name);
					}
				}
			}
			else {
				if (use_fragment_varying) {
					code = "frag_to_light.";
				}
				code += _mkid(anode->name);
			}
		}

		if (anode->call_expression != nullptr) {
			code += ".";
			code += _dump_node_code(anode->call_expression, p_level, r_gen_code, p_actions,
				p_default_actions, p_assigning, false);
		}
		else if (anode->index_expression != nullptr) {
			code += "[";
			code += _dump_node_code(anode->index_expression, p_level, r_gen_code, p_actions,
				p_default_actions, p_assigning);
			code += "]";
		}
		else if (anode->assign_expression != nullptr) {
			code += "=";
			code += _dump_node_code(anode->assign_expression, p_level, r_gen_code, p_actions,
				p_default_actions, true, false);
		}

		if (anode->name == time_name) {
			if (p_actions.entry_point_stages.has(current_func_name) &&
				p_actions.entry_point_stages[current_func_name] == STAGE_VERTEX) {
				r_gen_code.uses_vertex_time = true;
			}
			if (p_actions.entry_point_stages.has(current_func_name) &&
				p_actions.entry_point_stages[current_func_name] == STAGE_FRAGMENT) {
				r_gen_code.uses_fragment_time = true;
			}
		}

	} break;
	case SL::Node::NODE_TYPE_CONSTANT: {
		SL::ConstantNode* cnode = (SL::ConstantNode*)p_node;

		if (cnode->array_size == 0) {
			return get_constant_text(cnode->datatype, cnode->values);
		}
		else {
			if (cnode->get_datatype() == SL::TYPE_STRUCT) {
				code += _mkid(cnode->struct_name);
			}
			else {
				code += _typestr(cnode->datatype);
			}
			code += "[";
			code += itos(cnode->array_size);
			code += "]";
			code += "(";
			for (int i = 0; i < cnode->array_size; i++) {
				if (i > 0) {
					code += ",";
				}
				else {
					code += "";
				}
				code += _dump_node_code(cnode->array_declarations[0].initializer[i], p_level,
					r_gen_code, p_actions, p_default_actions, p_assigning);
			}
			code += ")";
		}

	} break;
	case SL::Node::NODE_TYPE_OPERATOR: {
		SL::OperatorNode* onode = (SL::OperatorNode*)p_node;

		switch (onode->op) {
		case SL::OP_ASSIGN:
		case SL::OP_ASSIGN_ADD:
		case SL::OP_ASSIGN_SUB:
		case SL::OP_ASSIGN_MUL:
		case SL::OP_ASSIGN_DIV:
		case SL::OP_ASSIGN_SHIFT_LEFT:
		case SL::OP_ASSIGN_SHIFT_RIGHT:
		case SL::OP_ASSIGN_MOD:
		case SL::OP_ASSIGN_BIT_AND:
		case SL::OP_ASSIGN_BIT_OR:
		case SL::OP_ASSIGN_BIT_XOR:
			code = _dump_node_code(onode->arguments[0], p_level, r_gen_code, p_actions,
					   p_default_actions, true) +
				   _opstr(onode->op) +
				   _dump_node_code(onode->arguments[1], p_level, r_gen_code, p_actions,
					   p_default_actions, p_assigning);
			break;
		case SL::OP_BIT_INVERT:
		case SL::OP_NEGATE:
		case SL::OP_NOT:
		case SL::OP_DECREMENT:
		case SL::OP_INCREMENT: {
			const String node_code = _dump_node_code(onode->arguments[0], p_level, r_gen_code,
				p_actions, p_default_actions, p_assigning);

			if (onode->op == SL::OP_NEGATE &&
				node_code.begins_with("-")) { // To prevent writing unary minus twice.
				code = node_code;
			}
			else {
				code = _opstr(onode->op) + node_code;
			}

		} break;
		case SL::OP_POST_DECREMENT:
		case SL::OP_POST_INCREMENT:
			code = _dump_node_code(onode->arguments[0], p_level, r_gen_code, p_actions,
					   p_default_actions, p_assigning) +
				   _opstr(onode->op);
			break;
		case SL::OP_CALL:
		case SL::OP_STRUCT:
		case SL::OP_CONSTRUCT: {
			ERR_FAIL_COND_V(onode->arguments[0]->type != SL::Node::NODE_TYPE_VARIABLE, String());
			const SL::VariableNode* vnode =
				static_cast<const SL::VariableNode*>(onode->arguments[0]);
			const SL::FunctionNode* func = nullptr;
			const bool is_internal_func = internal_functions.has(vnode->name);

			if (!is_internal_func) {
				for (int i = 0; i < shader->vfunctions.size(); i++) {
					if (shader->vfunctions[i].name == vnode->name) {
						func = shader->vfunctions[i].function;
						break;
					}
				}
			}

			bool is_texture_func = false;
			bool is_screen_texture = false;
			bool is_radiance_texture = false;
			bool texture_func_no_uv = false;
			bool texture_func_returns_data = false;
			bool texture_func_simple = false;

			if (onode->op == SL::OP_STRUCT) {
				code += _mkid(vnode->name);
			}
			else if (onode->op == SL::OP_CONSTRUCT) {
				code += String(vnode->name);
			}
			else {
				if (p_actions.usage_flag_pointers.has(vnode->name) &&
					!used_flag_pointers.has(vnode->name)) {
					*p_actions.usage_flag_pointers[vnode->name] = true;
					used_flag_pointers.insert(vnode->name);
				}

				if (is_internal_func) {
					code += vnode->name;
					is_texture_func = texture_functions.has(vnode->name);
					texture_func_no_uv =
						(vnode->name == "textureSize" || vnode->name == "textureQueryLevels");
					texture_func_returns_data =
						texture_func_no_uv || vnode->name == "textureQueryLod";
					texture_func_simple = vnode->name == "texture";
				}
				else if (p_default_actions.renames.has(vnode->name)) {
					code += p_default_actions.renames[vnode->name];
				}
				else {
					code += _mkid(vnode->rname);
				}
			}

			code += "(";

			// if color backbuffer, depth backbuffer or normal roughness texture is used,
			// we will add logic to automatically switch between
			// sampler2D and sampler2D array and vec2 UV and vec3 UV.
			bool multiview_uv_needed = false;
			bool is_normal_roughness_texture = false;

			for (int i = 1; i < onode->arguments.size(); i++) {
				if (i > 1) {
					code += ", ";
				}

				bool is_out_qualifier = false;
				if (is_internal_func) {
					is_out_qualifier = SL::is_builtin_func_out_parameter(vnode->name, i - 1);
				}
				else if (func != nullptr) {
					const SL::ArgumentQualifier qualifier = func->arguments[i - 1].qualifier;
					is_out_qualifier = qualifier == SL::ARGUMENT_QUALIFIER_OUT ||
									   qualifier == SL::ARGUMENT_QUALIFIER_INOUT;
				}

				if (is_out_qualifier) {
					String name;
					bool found = false;
					{
						const SL::Node* node = onode->arguments[i];

						bool done = false;
						do {
							switch (node->type) {
							case SL::Node::NODE_TYPE_VARIABLE: {
								name = static_cast<const SL::VariableNode*>(node)->name;
								done = true;
								found = true;
							} break;
							case SL::Node::NODE_TYPE_MEMBER: {
								node = static_cast<const SL::MemberNode*>(node)->owner;
							} break;
							default: {
								done = true;
							} break;
							}
						} while (!done);
					}

					if (found && p_actions.write_flag_pointers.has(name)) {
						*p_actions.write_flag_pointers[name] = true;
					}
				}

				String node_code = _dump_node_code(onode->arguments[i], p_level, r_gen_code,
					p_actions, p_default_actions, p_assigning);
				if (is_texture_func && i == 1) {
					// If we're doing a texture lookup we need to check our texture argument
					String texture_uniform;
					bool correct_texture_uniform = false;

					switch (onode->arguments[i]->type) {
					case SL::Node::NODE_TYPE_VARIABLE: {
						const SL::VariableNode* varnode =
							static_cast<const SL::VariableNode*>(onode->arguments[i]);
						texture_uniform = varnode->name;
						correct_texture_uniform = true;
					} break;
					case SL::Node::NODE_TYPE_ARRAY: {
						const SL::ArrayNode* anode =
							static_cast<const SL::ArrayNode*>(onode->arguments[i]);
						texture_uniform = anode->name;
						correct_texture_uniform = true;
					} break;
					default:
						break;
					}

					if (correct_texture_uniform && !Renderer::is_low_end()) {
						// Need to map from texture to sampler in order to sample when using Vulkan
						// GLSL.
						String sampler_name;
						bool is_depth_texture = false;

						if (actions.custom_samplers.has(texture_uniform)) {
							sampler_name = actions.custom_samplers[texture_uniform];
						}
						else {
							if (shader->uniforms.has(texture_uniform)) {
								const ShaderLanguage::ShaderNode::Uniform& u =
									shader->uniforms[texture_uniform];
								if (u.hint ==
									ShaderLanguage::ShaderNode::Uniform::HINT_SCREEN_TEXTURE) {
									is_screen_texture = true;
								}
								else if (u.hint == ShaderLanguage::ShaderNode::Uniform::
														 HINT_DEPTH_TEXTURE) {
									is_depth_texture = true;
								}
								else if (u.hint == ShaderLanguage::ShaderNode::Uniform::
														 HINT_NORMAL_ROUGHNESS_TEXTURE) {
									is_normal_roughness_texture = true;
								}
								sampler_name = _get_sampler_name(u.filter, u.repeat);
							}
							else {
								bool found = false;

								for (int j = 0; j < function->arguments.size(); j++) {
									if (function->arguments[j].name == texture_uniform) {
										if (function->arguments[j].tex_builtin_check) {
											ERR_CONTINUE(!actions.custom_samplers.has(
												function->arguments[j].tex_builtin));
											sampler_name =
												actions.custom_samplers[function->arguments[j]
																			.tex_builtin];
											found = true;
											break;
										}
										if (function->arguments[j].tex_argument_check) {
											if (function->arguments[j].tex_hint ==
												ShaderLanguage::ShaderNode::Uniform::
													HINT_SCREEN_TEXTURE) {
												is_screen_texture = true;
											}
											else if (function->arguments[j].tex_hint ==
													   ShaderLanguage::ShaderNode::Uniform::
														   HINT_DEPTH_TEXTURE) {
												is_depth_texture = true;
											}
											else if (function->arguments[j].tex_hint ==
													   ShaderLanguage::ShaderNode::Uniform::
														   HINT_NORMAL_ROUGHNESS_TEXTURE) {
												is_normal_roughness_texture = true;
											}
											sampler_name = _get_sampler_name(
												function->arguments[j].tex_argument_filter,
												function->arguments[j].tex_argument_repeat);
											found = true;
											break;
										}
									}
								}
								if (!found) {
									// function was most likely unused, so use anything (compiler
									// will remove it anyway)
									sampler_name = _get_sampler_name(ShaderLanguage::FILTER_DEFAULT,
										ShaderLanguage::REPEAT_DEFAULT);
								}
							}
						}

						if (texture_uniform == SNAME("RADIANCE")) {
							is_radiance_texture = true;
						}

						String data_type_name = "";
						if (actions.check_multiview_samplers &&
							(is_screen_texture || is_depth_texture ||
								is_normal_roughness_texture)) {
							data_type_name = "multiviewSampler";
							multiview_uv_needed = true;
						}
						else if (is_radiance_texture) {
							// We need to use an explicit level of detail to avoid mip mapping
							// artifacts caused by the octahedral discontinuity.
							if (texture_func_simple) {
								code = code.replace("texture(", "textureLod(");
							}
							data_type_name = "sampler2D";
						}
						else {
							data_type_name = ShaderLanguage::get_datatype_name(
								onode->arguments[i]->get_datatype());
						}

						code += data_type_name + "(" + node_code + ", " + sampler_name + ")";
					}
					else if (correct_texture_uniform && Renderer::is_low_end()) {
						// Texture function on low end hardware (i.e. OpenGL).

						if (shader->uniforms.has(texture_uniform)) {
							const ShaderLanguage::ShaderNode::Uniform& u =
								shader->uniforms[texture_uniform];
							if (actions.check_multiview_samplers) {
								if (u.hint ==
									ShaderLanguage::ShaderNode::Uniform::HINT_SCREEN_TEXTURE) {
									multiview_uv_needed = true;
								}
								else if (u.hint == ShaderLanguage::ShaderNode::Uniform::
														 HINT_DEPTH_TEXTURE) {
									multiview_uv_needed = true;
								}
								else if (u.hint == ShaderLanguage::ShaderNode::Uniform::
														 HINT_NORMAL_ROUGHNESS_TEXTURE) {
									multiview_uv_needed = true;
								}
							}
							if (u.hint ==
								ShaderLanguage::ShaderNode::Uniform::HINT_SCREEN_TEXTURE) {
								is_screen_texture = true;
							}
						}

						code += node_code;
					}
					else {
						code += node_code;
					}
				}
				else if (multiview_uv_needed && !texture_func_no_uv && i == 2) {
					// UV coordinate after using color, depth or normal roughness texture.
					node_code = "multiview_uv(" + node_code + ".xy)";

					code += node_code;
				}
				else if (is_radiance_texture && !texture_func_no_uv && i == 2) {
					node_code = "vec3_to_oct_with_border(" + node_code + ", params.border_size)";

					// Need an explicit level of detail if one isn't provided by the user.
					if (texture_func_simple && onode->arguments.size() == 3) {
						node_code += ", 0.0";
					}

					code += node_code;
				}
				else {
					code += node_code;
				}
			}
			code += ")";
			if (is_screen_texture && !texture_func_returns_data &&
				actions.apply_luminance_multiplier) {
				if (Renderer::is_low_end()) {
					code = "(" + code +
						   " / vec4(vec3(scene_data_block.data.luminance_multiplier), 1.0))";
				}
				else {
					code = "(" + code + " * vec4(vec3(sc_luminance_multiplier()), 1.0))";
				}
			}
			if (is_normal_roughness_texture && !texture_func_returns_data) {
				code = "normal_roughness_compatibility(" + code + ")";
			}
		} break;
		case SL::OP_INDEX: {
			code += _dump_node_code(onode->arguments[0], p_level, r_gen_code, p_actions,
				p_default_actions, p_assigning);
			code += "[";
			code += _dump_node_code(onode->arguments[1], p_level, r_gen_code, p_actions,
				p_default_actions, p_assigning);
			code += "]";

		} break;
		case SL::OP_SELECT_IF: {
			code += "(";
			code += _dump_node_code(onode->arguments[0], p_level, r_gen_code, p_actions,
				p_default_actions, p_assigning);
			code += "?";
			code += _dump_node_code(onode->arguments[1], p_level, r_gen_code, p_actions,
				p_default_actions, p_assigning);
			code += ":";
			code += _dump_node_code(onode->arguments[2], p_level, r_gen_code, p_actions,
				p_default_actions, p_assigning);
			code += ")";

		} break;
		case SL::OP_EMPTY: {
			// Semicolon (or empty statement) - ignored.
		} break;

		default: {
			if (p_use_scope) {
				code += "(";
			}
			code += _dump_node_code(onode->arguments[0], p_level, r_gen_code, p_actions,
						p_default_actions, p_assigning) +
					" " + _opstr(onode->op) + " " +
					_dump_node_code(onode->arguments[1], p_level, r_gen_code, p_actions,
						p_default_actions, p_assigning);
			if (p_use_scope) {
				code += ")";
			}
			break;
		}
		}

	} break;
	case SL::Node::NODE_TYPE_CONTROL_FLOW: {
		SL::ControlFlowNode* cfnode = (SL::ControlFlowNode*)p_node;
		if (cfnode->flow_op == SL::FLOW_OP_IF) {
			code += _mktab(p_level) + "if (" +
					_dump_node_code(cfnode->expressions[0], p_level, r_gen_code, p_actions,
						p_default_actions, p_assigning) +
					")\n";
			code += _dump_node_code(cfnode->blocks[0], p_level + 1, r_gen_code, p_actions,
				p_default_actions, p_assigning);
			if (cfnode->blocks.size() == 2) {
				code += _mktab(p_level) +
				"else\n";
				code += _dump_node_code(cfnode->blocks[1], p_level + 1, r_gen_code, p_actions,
					p_default_actions, p_assigning);
			}
		}
		else if (cfnode->flow_op == SL::FLOW_OP_SWITCH) {
			code += _mktab(p_level) + "switch (" +
					_dump_node_code(cfnode->expressions[0], p_level, r_gen_code, p_actions,
						p_default_actions, p_assigning) +
					")\n";
			code += _dump_node_code(cfnode->blocks[0], p_level + 1, r_gen_code, p_actions,
				p_default_actions, p_assigning);
		}
		else if (cfnode->flow_op == SL::FLOW_OP_CASE) {
			code += _mktab(p_level) + "case " +
					_dump_node_code(cfnode->expressions[0], p_level, r_gen_code, p_actions,
						p_default_actions, p_assigning) +
					":\n";
			code += _dump_node_code(cfnode->blocks[0], p_level + 1, r_gen_code, p_actions,
				p_default_actions, p_assigning);
		}
		else if (cfnode->flow_op == SL::FLOW_OP_DEFAULT) {
			code += _mktab(p_level) + "default:\n";
			code += _dump_node_code(cfnode->blocks[0], p_level + 1, r_gen_code, p_actions,
				p_default_actions, p_assigning);
		}
		else if (cfnode->flow_op == SL::FLOW_OP_DO) {
			code += _mktab(p_level) + "do";
			code += _dump_node_code(cfnode->blocks[0], p_level + 1, r_gen_code, p_actions,
				p_default_actions, p_assigning);
			code += _mktab(p_level) + "while (" +
					_dump_node_code(cfnode->expressions[0], p_level, r_gen_code, p_actions,
						p_default_actions, p_assigning) +
					");";
		}
		else if (cfnode->flow_op == SL::FLOW_OP_WHILE) {
			code += _mktab(p_level) + "while (" +
					_dump_node_code(cfnode->expressions[0], p_level, r_gen_code, p_actions,
						p_default_actions, p_assigning) +
					")\n";
			code += _dump_node_code(cfnode->blocks[0], p_level + 1, r_gen_code, p_actions,
				p_default_actions, p_assigning);
		}
		else if (cfnode->flow_op == SL::FLOW_OP_FOR) {
			String left = _dump_node_code(
				cfnode->blocks[0], p_level, r_gen_code, p_actions, p_default_actions, p_assigning);
			String middle = _dump_node_code(
				cfnode->blocks[1], p_level, r_gen_code, p_actions, p_default_actions, p_assigning);
			String right = _dump_node_code(
				cfnode->blocks[2], p_level, r_gen_code, p_actions, p_default_actions, p_assigning);
			code += _mktab(p_level) + "for (" + left + ";" + middle + ";" + right + ")\n";
			code += _dump_node_code(cfnode->blocks[3], p_level + 1, r_gen_code, p_actions,
				p_default_actions, p_assigning);

		}
		else if (cfnode->flow_op == SL::FLOW_OP_RETURN) {
			if (cfnode->expressions.size()) {
				code = "return " +
					   _dump_node_code(cfnode->expressions[0], p_level, r_gen_code, p_actions,
						   p_default_actions, p_assigning) +
					   ";";
			}
			else {
				code = "return;";
			}
		}
		else if (cfnode->flow_op == SL::FLOW_OP_DISCARD) {
			if (p_actions.usage_flag_pointers.has("DISCARD") &&
				!used_flag_pointers.has("DISCARD")) {
				*p_actions.usage_flag_pointers["DISCARD"] = true;
				used_flag_pointers.insert("DISCARD");
			}

			code = "discard;";
		}
		else if (cfnode->flow_op == SL::FLOW_OP_CONTINUE) {
			code = "continue;";
		}
		else if (cfnode->flow_op == SL::FLOW_OP_BREAK) {
			code = "break;";
		}

	} break;
	case SL::Node::NODE_TYPE_MEMBER: {
		SL::MemberNode* mnode = (SL::MemberNode*)p_node;
		String name;
		if (mnode->basetype == SL::TYPE_STRUCT) {
			name = _mkid(mnode->name);
		}
		else {
			name = mnode->name;
		}
		code = _dump_node_code(
				   mnode->owner, p_level, r_gen_code, p_actions, p_default_actions, p_assigning) +
			   "." + name;
		if (mnode->index_expression != nullptr) {
			code += "[";
			code += _dump_node_code(mnode->index_expression, p_level, r_gen_code, p_actions,
				p_default_actions, p_assigning);
			code += "]";
		}
		else if (mnode->assign_expression != nullptr) {
			code += "=";
			code += _dump_node_code(mnode->assign_expression, p_level, r_gen_code, p_actions,
				p_default_actions, true, false);
		}
		else if (mnode->call_expression != nullptr) {
			code += ".";
			code += _dump_node_code(mnode->call_expression, p_level, r_gen_code, p_actions,
				p_default_actions, p_assigning, false);
		}
	} break;
	}

	return code;
}

ShaderLanguage::DataType ShaderCompiler::_get_global_shader_uniform_type(const String& p_name)
{
	RSE::GlobalShaderParameterType gvt =
		RendererMaterialStorage::global_shader_parameter_get_type(p_name);
	return (ShaderLanguage::DataType)Renderer::global_shader_uniform_type_get_shader_datatype(gvt);
}

Error ShaderCompiler::compile(RSE::ShaderMode p_mode, const String& p_code,
	IdentifierActions* p_actions, const String& p_path, GeneratedCode& r_gen_code)
{
	SL::ShaderCompileInfo info;
	info.functions = ShaderTypes::get_functions(p_mode);
	info.render_modes = ShaderTypes::get_modes(p_mode);
	info.stencil_modes = ShaderTypes::get_stencil_modes(p_mode);
	info.shader_types = ShaderTypes::get_types();
	info.global_shader_uniform_type_func = _get_global_shader_uniform_type;
	info.base_varying_index = actions.base_varying_index;

	Error err = parser.compile(p_code, info);

	if (err != OK) {
		Vector<ShaderLanguage::FilePosition> include_positions = parser.get_include_positions();

		String current;
		HashMap<String, Vector<String>> includes;
		includes[""] = Vector<String>();
		Vector<String> include_stack;
		Vector<String> shader_lines = p_code.split("\n");

		// Reconstruct the files.
		for (int i = 0; i < shader_lines.size(); i++) {
			String l = shader_lines[i];
			if (l.begins_with("@@>")) {
				String inc_path = l.replace_first("@@>", "");

				l = "#include \"" + inc_path + "\"";
				includes[current].append(
					"#include \"" + inc_path + "\""); // Restore the include directive
				include_stack.push_back(current);
				current = inc_path;
				includes[inc_path] = Vector<String>();

			}
			else if (l.begins_with("@@<")) {
				if (include_stack.size()) {
					current = include_stack[include_stack.size() - 1];
					include_stack.resize(include_stack.size() - 1);
				}
			}
			else {
				includes[current].push_back(l);
			}
		}

		String file;
		int line;
		if (include_positions.size() > 1) {
			file = include_positions[include_positions.size() - 1].file;
			line = include_positions[include_positions.size() - 1].line;
		}
		else {
			file = p_path;
			line = parser.get_error_line();
		}

		_err_print_error(nullptr, file.utf8().get_data(), line,
			parser.get_error_text().utf8().get_data(), false, ERR_HANDLER_SHADER);
		return err;
	}

	r_gen_code.defines.clear();
	r_gen_code.code.clear();
	for (int i = 0; i < STAGE_MAX; i++) {
		r_gen_code.stage_globals[i] = String();
	}
	r_gen_code.uses_fragment_time = false;
	r_gen_code.uses_vertex_time = false;
	r_gen_code.uses_global_textures = false;
	r_gen_code.uses_screen_texture_mipmaps = false;
	r_gen_code.uses_screen_texture = false;
	r_gen_code.uses_depth_texture = false;
	r_gen_code.uses_normal_roughness_texture = false;

	used_name_defines.clear();
	used_rmode_defines.clear();
	used_flag_pointers.clear();
	fragment_varyings.clear();

	shader = parser.get_shader();
	function = nullptr;
	// Return value only relevant within nested calls.
	_ALLOW_DISCARD_ _dump_node_code(shader, 1, r_gen_code, *p_actions, actions, false);

	return OK;
}

void ShaderCompiler::initialize(DefaultIdentifierActions p_actions)
{
	actions = p_actions;

	time_name = "TIME";

	List<String> func_list;

	ShaderLanguage::get_builtin_funcs(&func_list);

	for (const String& E : func_list) {
		internal_functions.insert(E);
	}
	texture_functions.insert("texture");
	texture_functions.insert("textureProj");
	texture_functions.insert("textureLod");
	texture_functions.insert("textureProjLod");
	texture_functions.insert("textureGrad");
	texture_functions.insert("textureProjGrad");
	texture_functions.insert("textureGather");
	texture_functions.insert("textureSize");
	texture_functions.insert("textureQueryLod");
	texture_functions.insert("textureQueryLevels");
	texture_functions.insert("texelFetch");
}

ShaderCompiler::ShaderCompiler() {}
#undef SL

// --- Merged from servers/rendering/shader_preprocessor.cpp ---

const char32_t CURSOR = 0xFFFF;

// Tokenizer

void ShaderPreprocessor::Tokenizer::add_generated(const ShaderPreprocessor::Token& p_t)
{
	generated.push_back(p_t);
}

char32_t ShaderPreprocessor::Tokenizer::next()
{
	if (index < size) {
		return code[index++];
	}
	return 0;
}

int ShaderPreprocessor::Tokenizer::get_line() const { return line; }

int ShaderPreprocessor::Tokenizer::get_index() const { return index; }

void ShaderPreprocessor::Tokenizer::get_and_clear_generated(LocalVector<char32_t>* r_out)
{
	for (uint32_t i = 0; i < generated.size(); i++) {
		r_out->push_back(generated[i].text);
	}
	generated.clear();
}

void ShaderPreprocessor::Tokenizer::backtrack(char32_t p_what)
{
	while (index >= 0) {
		char32_t c = code[index];
		if (c == p_what) {
			break;
		}
		index--;
	}
}

char32_t ShaderPreprocessor::Tokenizer::peek()
{
	if (index < size) {
		return code[index];
	}
	return 0;
}

int ShaderPreprocessor::Tokenizer::consume_line_continuations(int p_offset)
{
	int skips = 0;

	for (int i = index + p_offset; i < size; i++) {
		char32_t c = code[i];
		if (c == '\\') {
			if (i + 1 < size && code[i + 1] == '\n') {
				// This line ends with "\" and "\n" continuation.
				add_generated(Token('\n', line));
				line++;
				skips++;

				i = i + 2;
				index = i;
			}
			else {
				break;
			}
		}
		else if (!is_whitespace(c)) {
			break;
		}
	}
	return skips;
}

LocalVector<ShaderPreprocessor::Token> ShaderPreprocessor::Tokenizer::advance(char32_t p_what)
{
	LocalVector<ShaderPreprocessor::Token> tokens;

	while (index < size) {
		char32_t c = code[index++];
		if (c == '\\' && consume_line_continuations(-1) > 0) {
			continue;
		}

		if (c == '\n') {
			add_generated(ShaderPreprocessor::Token('\n', line));
			line++;
		}

		tokens.push_back(ShaderPreprocessor::Token(c, line));

		if (c == p_what || c == 0) {
			return tokens;
		}
	}
	return LocalVector<ShaderPreprocessor::Token>();
}

void ShaderPreprocessor::Tokenizer::skip_whitespace()
{
	while (is_char_space(peek())) {
		next();
	}
}

bool ShaderPreprocessor::Tokenizer::consume_empty_line()
{
	// Read until newline and return true if the content was all whitespace/empty.
	return tokens_to_string(advance('\n')).strip_edges().is_empty();
}

String ShaderPreprocessor::Tokenizer::get_identifier(bool* r_is_cursor, bool p_started)
{
	if (r_is_cursor != nullptr) {
		*r_is_cursor = false;
	}

	LocalVector<char32_t> text;

	while (true) {
		char32_t c = peek();
		if (c == '\\' && consume_line_continuations(0) > 0) {
			continue;
		}

		if (is_char_end(c) || c == '(' || c == ')' || c == ',' || c == ';') {
			break;
		}

		if (is_whitespace(c) && p_started) {
			break;
		}
		if (!is_whitespace(c)) {
			p_started = true;
		}

		char32_t n = next();
		if (n == CURSOR) {
			if (r_is_cursor != nullptr) {
				*r_is_cursor = true;
			}
		}
		else {
			if (p_started) {
				text.push_back(n);
			}
		}
	}

	String id = vector_to_string(text);
	if (!id.is_valid_ascii_identifier()) {
		return "";
	}

	return id;
}

String ShaderPreprocessor::Tokenizer::peek_identifier()
{
	const int original = index;
	const int original_line = line;
	String id = get_identifier();
	index = original;
	line = original_line;
	return id;
}

ShaderPreprocessor::Token ShaderPreprocessor::Tokenizer::get_token()
{
	while (index < size) {
		const char32_t c = code[index++];
		const Token t = ShaderPreprocessor::Token(c, line);

		switch (c) {
		case ' ':
		case '\t':
			skip_whitespace();
			return ShaderPreprocessor::Token(' ', line);
		case '\n':
			line++;
			return t;
		default:
			return t;
		}
	}
	return ShaderPreprocessor::Token(char32_t(0), line);
}

ShaderPreprocessor::Tokenizer::Tokenizer(const String& p_code)
{
	code = p_code;
	line = 0;
	index = 0;
	size = code.size();
}

// ShaderPreprocessor::CommentRemover

String ShaderPreprocessor::CommentRemover::get_error() const
{
	if (comments_open != 0) {
		return "Block comment mismatch";
	}
	return "";
}

int ShaderPreprocessor::CommentRemover::get_error_line() const
{
	if (comments_open != 0) {
		return comment_line_open;
	}
	return -1;
}

char32_t ShaderPreprocessor::CommentRemover::peek() const
{
	if (index < code.size()) {
		return code[index];
	}
	return 0;
}

bool ShaderPreprocessor::CommentRemover::advance(char32_t p_what)
{
	while (index < code.size()) {
		char32_t c = code[index++];

		if (c == '\n') {
			line++;
			stripped.push_back('\n');
		}

		if (c == p_what) {
			return true;
		}
	}
	return false;
}

String ShaderPreprocessor::CommentRemover::strip()
{
	stripped.clear();
	index = 0;
	line = 0;
	comment_line_open = 0;
	comments_open = 0;
	strings_open = 0;

	while (index < code.size()) {
		char32_t c = code[index++];

		if (c == CURSOR) {
			// Cursor. Maintain.
			stripped.push_back(c);
		}
		else if (c == '"') {
			if (strings_open <= 0) {
				strings_open++;
			}
			else {
				strings_open--;
			}
			stripped.push_back(c);
		}
		else if (c == '/' && strings_open == 0) {
			char32_t p = peek();
			if (p == '/') { // Single line comment.
				advance('\n');
			}
			else if (p == '*') { // Start of a block comment.
				index++;
				comment_line_open = line;
				comments_open++;
				while (advance('*')) {
					if (peek() == '/') { // End of a block comment.
						comments_open--;
						index++;
						break;
					}
				}
			}
			else {
				stripped.push_back(c);
			}
		}
		else if (c == '*' && strings_open == 0) {
			if (peek() == '/') { // Unmatched end of a block comment.
				comment_line_open = line;
				comments_open--;
			}
			else {
				stripped.push_back(c);
			}
		}
		else if (c == '\n') {
			line++;
			stripped.push_back(c);
		}
		else {
			stripped.push_back(c);
		}
	}
	return vector_to_string(stripped);
}

ShaderPreprocessor::CommentRemover::CommentRemover(const String& p_code)
{
	code = p_code;
	index = 0;
	line = 0;
	comment_line_open = 0;
	comments_open = 0;
	strings_open = 0;
}

// ShaderPreprocessor::Token

ShaderPreprocessor::Token::Token()
{
	text = 0;
	line = -1;
}

ShaderPreprocessor::Token::Token(char32_t p_text, int p_line)
{
	text = p_text;
	line = p_line;
}

// ShaderPreprocessor

bool ShaderPreprocessor::is_char_word(char32_t p_char)
{
	if ((p_char >= '0' && p_char <= '9') || (p_char >= 'a' && p_char <= 'z') ||
		(p_char >= 'A' && p_char <= 'Z') || p_char == '_') {
		return true;
	}

	return false;
}

bool ShaderPreprocessor::is_char_space(char32_t p_char) { return p_char == ' ' || p_char == '\t'; }

bool ShaderPreprocessor::is_char_end(char32_t p_char) { return p_char == '\n' || p_char == 0; }

String ShaderPreprocessor::vector_to_string(
	const LocalVector<char32_t>& p_v, int p_start, int p_end)
{
	const int stop = (p_end == -1) ? p_v.size() : p_end;
	const int count = stop - p_start;

	String result;
	result.resize_uninitialized(count + 1);
	for (int i = 0; i < count; i++) {
		result[i] = p_v[p_start + i];
	}
	result[count] = 0; // Ensure string is null terminated for length() to work.
	return result;
}

String ShaderPreprocessor::tokens_to_string(const LocalVector<Token>& p_tokens)
{
	LocalVector<char32_t> result;
	for (const Token& token : p_tokens) {
		result.push_back(token.text);
	}
	return vector_to_string(result);
}

void ShaderPreprocessor::process_directive(Tokenizer* p_tokenizer)
{
	bool is_cursor;
	String directive = p_tokenizer->get_identifier(&is_cursor, true);
	if (is_cursor) {
		state->completion_type = COMPLETION_TYPE_DIRECTIVE;
	}

	if (directive == "if") {
		process_if(p_tokenizer);
	}
	else if (directive == "ifdef") {
		process_ifdef(p_tokenizer);
	}
	else if (directive == "ifndef") {
		process_ifndef(p_tokenizer);
	}
	else if (directive == "elif") {
		process_elif(p_tokenizer);
	}
	else if (directive ==
	"else") {
		process_else(p_tokenizer);
	}
	else if (directive == "endif") {
		process_endif(p_tokenizer);
	}
	else if (directive == "error") {
		process_error(p_tokenizer);
	}
	else if (directive == "define") {
		process_define(p_tokenizer);
	}
	else if (directive == "undef") {
		process_undef(p_tokenizer);
	}
	else if (directive == "include") {
		process_include(p_tokenizer);
	}
	else if (directive == "pragma") {
		process_pragma(p_tokenizer);
	}
	else {
		set_error(RTR("Unknown directive."), p_tokenizer->get_line());
	}
}

void ShaderPreprocessor::process_define(Tokenizer* p_tokenizer)
{
	const int line = p_tokenizer->get_line();

	String label = p_tokenizer->get_identifier();
	if (label.is_empty()) {
		set_error(RTR("Invalid macro name."), line);
		return;
	}

	if (state->defines.has(label)) {
		set_error(RTR("Macro redefinition."), line);
		return;
	}

	Vector<String> args;
	if (p_tokenizer->peek() == '(') {
		// Macro has arguments.
		p_tokenizer->get_token();

		while (true) {
			String name = p_tokenizer->get_identifier();
			if (name.is_empty()) {
				set_error(RTR("Invalid argument name."), line);
				return;
			}
			args.push_back(name);

			p_tokenizer->skip_whitespace();
			char32_t next = p_tokenizer->get_token().text;
			if (next == ')') {
				break;
			}
			else if (next != ',') {
				set_error(RTR("Expected a comma in the macro argument list."), line);
				return;
			}
		}
	}

	String body = tokens_to_string(p_tokenizer->advance('\n')).strip_edges();
	if (body.begins_with("##")) {
		set_error(RTR("'##' must not appear at beginning of macro expansion."), line);
		return;
	}
	if (body.ends_with("##")) {
		set_error(RTR("'##' must not appear at end of macro expansion."), line);
		return;
	}

	Define* define = memnew(Define);
	if (!args.is_empty()) {
		define->arguments = args;
	}
	define->body = body;
	state->defines[label] = define;
}

void ShaderPreprocessor::process_else(Tokenizer* p_tokenizer)
{
	const int line = p_tokenizer->get_line();

	if (state->current_branch == nullptr || state->current_branch->else_defined) {
		set_error(vformat(RTR("Unmatched '%s' directive."),
		"else"), line);
		return;
	}
	if (state->previous_region != nullptr) {
		state->previous_region->to_line = line - 1;
	}

	if (!p_tokenizer->consume_empty_line()) {
		set_error(vformat(RTR("Invalid '%s' directive."),
		"else"), line);
	}

	bool skip = false;
	for (int i = 0; i < state->current_branch->conditions.size(); i++) {
		if (state->current_branch->conditions[i]) {
			skip = true;
			break;
		}
	}
	state->current_branch->else_defined = true;

	if (state->save_regions) {
		add_region(line + 1, !skip, state->previous_region->parent);
	}

	if (skip) {
		Vector<String> ends;
		ends.push_back("endif");
		// Legacy return value.
		_ALLOW_DISCARD_ next_directive(p_tokenizer, ends);
	}
}

void ShaderPreprocessor::process_endif(Tokenizer* p_tokenizer)
{
	const int line = p_tokenizer->get_line();

	state->condition_depth--;
	if (state->condition_depth < 0) {
		set_error(vformat(RTR("Unmatched '%s' directive."), "endif"), line);
		return;
	}
	if (state->previous_region != nullptr) {
		state->previous_region->to_line = line - 1;
		state->previous_region = state->previous_region->parent;
	}

	if (!p_tokenizer->consume_empty_line()) {
		set_error(vformat(RTR("Invalid '%s' directive."), "endif"), line);
	}

	state->current_branch = state->current_branch->parent;
	state->branches.pop_back();
}

void ShaderPreprocessor::process_error(Tokenizer* p_tokenizer)
{
	const int line = p_tokenizer->get_line();

	const String body = tokens_to_string(p_tokenizer->advance('\n')).strip_edges();
	if (body.is_empty()) {
		set_error(" ", line);
	}
	else {
		set_error(body, line);
	}

	if (!p_tokenizer->consume_empty_line()) {
		set_error(vformat(RTR("Invalid '%s' directive."), "error"), line);
	}
}

void ShaderPreprocessor::process_ifdef(Tokenizer* p_tokenizer)
{
	const int line = p_tokenizer->get_line();
	bool is_cursor = false;

	String label = p_tokenizer->get_identifier(&is_cursor);
	if (label.is_empty()) {
		set_error(RTR("Invalid macro name."), line);
		return;
	}

	if (is_cursor) {
		state->completion_show_defines = true;
	}

	if (!p_tokenizer->consume_empty_line()) {
		set_error(vformat(RTR("Invalid '%s' directive."), "ifdef"), line);
		return;
	}

	bool success = state->defines.has(label);
	start_branch_condition(p_tokenizer, success);

	if (state->save_regions) {
		add_region(line + 1, success, state->previous_region);
	}
}

void ShaderPreprocessor::process_ifndef(Tokenizer* p_tokenizer)
{
	const int line = p_tokenizer->get_line();

	String label = p_tokenizer->get_identifier();
	if (label.is_empty()) {
		set_error(RTR("Invalid macro name."), line);
		return;
	}

	if (!p_tokenizer->consume_empty_line()) {
		set_error(vformat(RTR("Invalid '%s' directive."), "ifndef"), line);
		return;
	}

	bool success = !state->defines.has(label);
	start_branch_condition(p_tokenizer, success);

	if (state->save_regions) {
		add_region(line + 1, success, state->previous_region);
	}
}

void ShaderPreprocessor::process_include(Tokenizer* p_tokenizer)
{
	const int line = p_tokenizer->get_line();

	p_tokenizer->advance('"');
	String path = tokens_to_string(p_tokenizer->advance('"'));
	for (int i = 0; i < path.length(); i++) {
		if (path[i] == '\n') {
			break; // stop parsing
		}
		if (path[i] == CURSOR) {
			state->completion_type = COMPLETION_TYPE_INCLUDE_PATH;
			break;
		}
	}
	path = path.substr(0, path.length() - 1);

	if (path.is_empty() || !p_tokenizer->consume_empty_line()) {
		set_error(RTR("Invalid path."), line);
		return;
	}

	path = path.simplify_path();
	if (path.is_relative_path()) {
		path = state->current_filename.get_base_dir().path_join(path);
	}

	if (!ResourceLoader::exists(path)) {
		set_error(RTR("Shader include file does not exist:") + " " + path, line);
		return;
	}

	Ref<Resource> res = ResourceLoader::load(path);
	if (res.is_null()) {
		set_error(RTR("Shader include load failed. Does the shader include exist? Is there a "
					  "cyclic dependency?"),
			line);
		return;
	}

	Ref<ShaderInclude> shader_inc = res;
	if (shader_inc.is_null()) {
		set_error(RTR("Shader include resource type is wrong."), line);
		return;
	}

	String included = shader_inc->get_code();
	if (!included.is_empty()) {
		uint64_t code_hash = included.hash64();
		if (state->cyclic_include_hashes.find(code_hash)) {
			set_error(RTR("Cyclic include found") + ": " + path, line);
			return;
		}
	}

	state->shader_includes.insert(shader_inc);

	const String real_path = shader_inc->get_path();
	if (state->includes.has(real_path)) {
		// Already included, skip.
		// This is a valid check because 2 separate include paths could use some
		// of the same shared functions from a common shader include.
		return;
	}

	// Mark as included.
	state->includes.insert(real_path);

	state->include_depth++;
	if (state->include_depth > 25) {
		set_error(RTR("Shader max include depth exceeded."), line);
		return;
	}

	String old_filename = state->current_filename;
	state->current_filename = real_path;
	ShaderPreprocessor processor;

	int prev_condition_depth = state->condition_depth;
	state->condition_depth = 0;

	FilePosition fp;
	fp.file = state->current_filename;
	fp.line = line + 1;
	state->include_positions.push_back(fp);

	String result;
	processor.preprocess(state, included, result);
	add_to_output("@@>" + real_path + "\n"); // Add token for enter include path
	add_to_output(result);
	add_to_output("\n@@<" + real_path + "\n"); // Add token for exit include path.

	// Reset to last include if there are no errors. We want to use this as context.
	if (state->error.is_empty()) {
		state->current_filename = old_filename;
		state->include_positions.pop_back();
	}
	else {
		return;
	}

	state->include_depth--;
	state->condition_depth = prev_condition_depth;
}

void ShaderPreprocessor::process_pragma(Tokenizer* p_tokenizer)
{
	const int line = p_tokenizer->get_line();

	bool is_cursor;
	const String label = p_tokenizer->get_identifier(&is_cursor);
	if (is_cursor) {
		state->completion_type = COMPLETION_TYPE_PRAGMA;
	}

	if (label.is_empty()) {
		set_error(vformat(RTR("Invalid '%s' directive."), "pragma"), line);
		return;
	}

	// Explicitly handle pragma values here.
	// If more pragma options are created, then refactor into a more defined structure.
	if (label == "disable_preprocessor") {
		state->disabled = true;
	}
	else {
		set_error(vformat(RTR("Invalid '%s' directive."), "pragma"), line);
		return;
	}

	if (!p_tokenizer->consume_empty_line()) {
		set_error(vformat(RTR("Invalid '%s' directive."), "pragma"), line);
		return;
	}
}

void ShaderPreprocessor::process_undef(Tokenizer* p_tokenizer)
{
	const int line = p_tokenizer->get_line();
	const String label = p_tokenizer->get_identifier();
	if (label.is_empty() || !p_tokenizer->consume_empty_line()) {
		set_error(vformat(RTR("Invalid '%s' directive."), "undef"), line);
		return;
	}

	if (state->defines.has(label)) {
		if (state->defines[label]->is_builtin) {
			set_error(vformat(RTR("Cannot use '%s' on built-in define."), "undef"), line);
			return;
		}

		memdelete(state->defines[label]);
		state->defines.erase(label);
	}
}

void ShaderPreprocessor::add_region(int p_line, bool p_enabled, Region* p_parent_region)
{
	Region region;
	region.file = state->current_filename;
	region.enabled = p_enabled;
	region.from_line = p_line;
	region.parent = p_parent_region;
	state->previous_region = &state->regions[region.file].push_back(region)->get();
}

void ShaderPreprocessor::start_branch_condition(
	Tokenizer* p_tokenizer, bool p_success, bool p_continue)
{
	if (!p_continue) {
		state->condition_depth++;
		state->current_branch =
			&state->branches.push_back(Branch(p_success, state->current_branch))->get();
	}
	else {
		state->current_branch->conditions.push_back(p_success);
	}
	if (!p_success) {
		Vector<String> ends;
		ends.push_back("elif");
		ends.push_back(
		"else");
		ends.push_back("endif");
		// Legacy return value.
		_ALLOW_DISCARD_ next_directive(p_tokenizer, ends);
	}
}

void ShaderPreprocessor::expand_output_macros(int p_start, int p_line_number)
{
	String line = vector_to_string(output, p_start, output.size());

	Error error =
		expand_macros(line, p_line_number - 1, line); // We are already on next line, so -1.
	if (error != OK) {
		return;
	}

	output.resize(p_start);

	add_to_output(line);
}

Error ShaderPreprocessor::expand_condition(const String& p_string, int p_line, String& r_expanded)
{
	// Checks bracket count to be even + check the cursor position.
	{
		int bracket_start_count = 0;
		int bracket_end_count = 0;

		for (int i = 0; i < p_string.size(); i++) {
			switch (p_string[i]) {
			case CURSOR:
				state->completion_type = COMPLETION_TYPE_CONDITION;
				state->completion_show_defines = true;
				break;
			case '(':
				bracket_start_count++;
				break;
			case ')':
				bracket_end_count++;
				break;
			}
		}
		if (bracket_start_count > bracket_end_count) {
			_set_expected_error(")", p_line);
			return FAILED;
		}
		if (bracket_end_count > bracket_start_count) {
			_set_expected_error("(", p_line);
			return FAILED;
		}
	}

	String result = p_string;

	int index = 0;
	int index_start = 0;
	int index_end = 0;

	while (find_match(result, "defined", index, index_start)) {
		bool open_bracket = false;
		bool found_word = false;
		bool word_completed = false;

		LocalVector<char32_t> text;
		int post_bracket_index = -1;
		int size = result.size();

		for (int i = (index_start - 1); i < size; i++) {
			char32_t c = result[i];
			if (c == 0) {
				if (found_word) {
					word_completed = true;
				}
				break;
			}
			char32_t cs[] = {c, '\0'};
			String s = String(cs);
			bool is_space = is_char_space(c);

			if (word_completed) {
				if (c == ')') {
					continue;
				}
				if (c == '|' || c == '&') {
					if (open_bracket) {
						_set_unexpected_token_error(s, p_line);
						return FAILED;
					}
					break;
				}
				else if (!is_space) {
					_set_unexpected_token_error(s, p_line);
					return FAILED;
				}
			}
			else if (is_space) {
				if (found_word && !open_bracket) {
					index_end = i;
					word_completed = true;
				}
			}
			else if (c == '(') {
				if (open_bracket) {
					_set_unexpected_token_error(s, p_line);
					return FAILED;
				}
				open_bracket = true;
			}
			else if (c == ')') {
				if (open_bracket) {
					if (!found_word) {
						_set_unexpected_token_error(s, p_line);
						return FAILED;
					}
					open_bracket = false;
					post_bracket_index = i + 1;
				}
				else {
					index_end = i;
				}
				word_completed = true;
			}
			else if (is_char_word(c)) {
				text.push_back(c);
				found_word = true;
			}
			else {
				_set_unexpected_token_error(s, p_line);
				return FAILED;
			}
		}

		if (word_completed) {
			if (open_bracket) {
				_set_expected_error(")", p_line);
				return FAILED;
			}
			if (post_bracket_index != -1) {
				index_end = post_bracket_index;
			}

			String body = state->defines.has(vector_to_string(text)) ? "true" : "false";
			String temp = result;

			result = result.substr(0, index) + body;
			index_start = result.length();
			if (index_end > 0) {
				result += temp.substr(index_end);
			}
		}
		else {
			set_error(RTR("Invalid macro name."), p_line);
			return FAILED;
		}
	}
	r_expanded = result;
	return OK;
}

Error ShaderPreprocessor::expand_macros(const String& p_string, int p_line, String& r_expanded)
{
	String iterative = p_string;
	int pass_count = 0;
	bool expanded = true;

	while (expanded) {
		expanded = false;

		// As long as we find something to expand, keep going.
		for (const RBMap<String, Define*>::Element* E = state->defines.front(); E; E = E->next()) {
			if (expand_macros_once(iterative, p_line, E, iterative)) {
				expanded = true;
			}
		}

		pass_count++;
		if (pass_count > 50) {
			set_error(RTR("Macro expansion limit exceeded."), p_line);
			break;
		}
	}

	r_expanded = iterative;

	if (!state->error.is_empty()) {
		return FAILED;
	}
	return OK;
}

bool ShaderPreprocessor::expand_macros_once(const String& p_line, int p_line_number,
	const RBMap<String, Define*>::Element* p_define_pair, String& r_expanded)
{
	String result = p_line;

	const String& key = p_define_pair->key();
	const Define* define = p_define_pair->value();

	int index_start = 0;
	int index = 0;
	if (find_match(result, key, index, index_start)) {
		String body = define->body;
		if (define->arguments.size() > 0) {
			// Complex macro with arguments.

			int args_start = -1;
			int args_end = -1;
			int brackets_open = 0;
			Vector<String> args;
			for (int i = index_start - 1; i < p_line.length(); i++) {
				bool add_argument = false;
				bool reached_end = false;
				char32_t c = p_line[i];

				if (c == '(') {
					brackets_open++;
					if (brackets_open == 1) {
						args_start = i + 1;
						args_end = -1;
					}
				}
				else if (c == ')') {
					brackets_open--;
					if (brackets_open == 0) {
						args_end = i;
						add_argument = true;
						reached_end = true;
					}
				}
				else if (c == ',') {
					if (brackets_open == 1) {
						args_end = i;
						add_argument = true;
					}
				}

				if (add_argument) {
					if (args_start == -1 || args_end == -1) {
						set_error(RTR("Invalid macro argument list."), p_line_number);
						return false;
					}

					String arg = p_line.substr(args_start, args_end - args_start).strip_edges();
					if (arg.is_empty()) {
						set_error(RTR("Invalid macro argument."), p_line_number);
						return false;
					}
					args.append(arg);

					args_start = args_end + 1;
				}

				if (reached_end) {
					break;
				}
			}

			if (args.size() != define->arguments.size()) {
				set_error(RTR("Invalid macro argument count."), p_line_number);
				return false;
			}

			// Insert macro arguments into the body.
			for (int i = 0; i < args.size(); i++) {
				String arg_name = define->arguments[i];
				int arg_index_start = 0;
				int arg_index = 0;
				while (find_match(body, arg_name, arg_index, arg_index_start)) {
					body = body.substr(0, arg_index) + args[i] +
						   body.substr(arg_index + arg_name.length());
					// Manually reset arg_index_start to where the arg value of the define finishes.
					// This ensures we don't skip the other args of this macro in the string.
					arg_index_start = arg_index + args[i].length() + 1;
				}
			}

			concatenate_macro_body(body);

			result = result.substr(0, index) + " " + body + " " + result.substr(args_end + 1);
		}
		else {
			concatenate_macro_body(body);

			result =
				result.substr(0, index) + " " + body + " " + result.substr(index + key.length());
		}

		r_expanded = result;
		return true;
	}

	return false;
}

bool ShaderPreprocessor::find_match(
	const String& p_string, const String& p_value, int& r_index, int& r_index_start)
{
	// Looks for value in string and then determines if the boundaries
	// are non-word characters. This method semi-emulates \b in regex.
	r_index = p_string.find(p_value, r_index_start);
	while (r_index > -1) {
		if (r_index > 0) {
			if (is_char_word(p_string[r_index - 1])) {
				r_index_start = r_index + 1;
				r_index = p_string.find(p_value, r_index_start);
				continue;
			}
		}

		if (r_index + p_value.length() < p_string.length()) {
			if (is_char_word(p_string[r_index + p_value.length()])) {
				r_index_start = r_index + p_value.length() + 1;
				r_index = p_string.find(p_value, r_index_start);
				continue;
			}
		}

		// Return and shift index start automatically for next call.
		r_index_start = r_index + p_value.length() + 1;
		return true;
	}

	return false;
}

void ShaderPreprocessor::concatenate_macro_body(String& r_body)
{
	int index_start = r_body.find("##");
	while (index_start > -1) {
		int index_end = index_start + 2; // First character after ##.
		// The macro was checked during creation so this should never happen.
		ERR_FAIL_INDEX(index_end, r_body.size());

		// If there more than two # in a row, then it's not a concatenation.
		bool is_concat = true;
		while (index_end <= r_body.length() && r_body[index_end] == '#') {
			index_end++;
			is_concat = false;
		}
		if (!is_concat) {
			index_start = r_body.find("##", index_end);
			continue;
		}

		// Skip whitespace after ##.
		while (index_end < r_body.length() && is_char_space(r_body[index_end])) {
			index_end++;
		}

		// Skip whitespace before ##.
		while (index_start >= 1 && is_char_space(r_body[index_start - 1])) {
			index_start--;
		}

		r_body = r_body.substr(0, index_start) + r_body.substr(index_end);

		index_start = r_body.find("##", index_start);
	}
}

String ShaderPreprocessor::next_directive(
	Tokenizer* p_tokenizer, const Vector<String>& p_directives)
{
	const int line = p_tokenizer->get_line();
	int nesting = 0;

	while (true) {
		p_tokenizer->advance('#');

		String id = p_tokenizer->peek_identifier();
		if (id.is_empty()) {
			break;
		}

		if (nesting == 0) {
			for (int i = 0; i < p_directives.size(); i++) {
				if (p_directives[i] == id) {
					p_tokenizer->backtrack('#');
					return id;
				}
			}
		}

		if (id == "ifdef" || id == "ifndef" || id == "if") {
			nesting++;
		}
		else if (id == "endif") {
			nesting--;
		}
	}

	set_error(RTR("Can't find matching branch directive."), line);
	return "";
}

void ShaderPreprocessor::add_to_output(const String& p_str)
{
	for (int i = 0; i < p_str.length(); i++) {
		output.push_back(p_str[i]);
	}
}

void ShaderPreprocessor::set_error(const String& p_error, int p_line)
{
	if (state->error.is_empty()) {
		state->error = p_error;
		FilePosition fp;
		fp.file = state->current_filename;
		fp.line = p_line + 1;
		state->include_positions.push_back(fp);
	}
}

ShaderPreprocessor::Define* ShaderPreprocessor::create_define(const String& p_body)
{
	ShaderPreprocessor::Define* define = memnew(Define);
	define->body = p_body;
	return define;
}

void ShaderPreprocessor::insert_builtin_define(String p_name, String p_value, State& p_state)
{
	Define* define = memnew(Define);
	define->is_builtin = true;
	define->body = p_value;
	p_state.defines[p_name] = define;
}

void ShaderPreprocessor::clear_state()
{
	if (state != nullptr) {
		for (const RBMap<String, Define*>::Element* E = state->defines.front(); E; E = E->next()) {
			memdelete(E->get());
		}
		state->defines.clear();
	}
	state = nullptr;
}

Error ShaderPreprocessor::preprocess(State* p_state, const String& p_code, String& r_result)
{
	output.clear();

	state = p_state;

	CommentRemover remover(p_code);
	String stripped = remover.strip();
	String error = remover.get_error();
	if (!error.is_empty()) {
		set_error(error, remover.get_error_line());
		return FAILED;
	}

	// Track code hashes to prevent cyclic include.
	uint64_t code_hash = p_code.hash64();
	state->cyclic_include_hashes.push_back(code_hash);

	Tokenizer p_tokenizer(stripped);
	int last_size = 0;
	bool has_symbols_before_directive = false;

	while (true) {
		const Token& t = p_tokenizer.get_token();

		if (t.text == 0) {
			break;
		}

		// Add autogenerated tokens if there are any.
		p_tokenizer.get_and_clear_generated(&output);

		if (state->disabled) {
			// Preprocessor was disabled.
			// Read the rest of the file into the output.
			output.push_back(t.text);
			continue;
		}

		if (t.text == '#') {
			if (has_symbols_before_directive) {
				set_error(RTR("Invalid symbols placed before directive."), p_tokenizer.get_line());
				state->cyclic_include_hashes.erase(code_hash); // Remove this hash.
				return FAILED;
			}
			process_directive(&p_tokenizer);
		}
		else {
			if (is_char_end(t.text)) {
				expand_output_macros(last_size, p_tokenizer.get_line());
				last_size = output.size();
				has_symbols_before_directive = false;
			}
			else if (!is_char_space(t.text)) {
				has_symbols_before_directive = true;
			}
			output.push_back(t.text);
		}

		if (!state->error.is_empty()) {
			state->cyclic_include_hashes.erase(code_hash); // Remove this hash.
			return FAILED;
		}
	}
	state->cyclic_include_hashes.erase(code_hash); // Remove this hash.

	if (!state->disabled) {
		if (state->condition_depth != 0) {
			set_error(RTR("Unmatched conditional statement."), p_tokenizer.line);
			return FAILED;
		}

		expand_output_macros(last_size, p_tokenizer.get_line());
	}

	r_result = vector_to_string(output);

	return OK;
}

void ShaderPreprocessor::get_keyword_list(
	List<String>* r_keywords, bool p_include_shader_keywords, bool p_ignore_context_keywords)
{
	r_keywords->push_back("define");
	if (!p_ignore_context_keywords) {
		r_keywords->push_back("defined");
	}
	r_keywords->push_back("elif");
	if (p_include_shader_keywords) {
		r_keywords->push_back(
		"else");
	}
	r_keywords->push_back("endif");
	r_keywords->push_back("error");
	if (p_include_shader_keywords) {
		r_keywords->push_back("if");
	}
	r_keywords->push_back("ifdef");
	r_keywords->push_back("ifndef");
	r_keywords->push_back("include");
	r_keywords->push_back("pragma");
	r_keywords->push_back("undef");
}

void ShaderPreprocessor::get_pragma_list(List<String>* r_pragmas)
{
	r_pragmas->push_back("disable_preprocessor");
}

void ShaderPreprocessor::process_if(ShaderPreprocessor::Tokenizer* p_tokenizer) {}

void ShaderPreprocessor::process_elif(ShaderPreprocessor::Tokenizer* p_tokenizer) {}

// --- Merged from servers/rendering/shader_types.cpp ---

const HashMap<String, ShaderLanguage::FunctionInfo>& ShaderTypes::get_functions(
	RSE::ShaderMode p_mode)
{
	return shader_modes[p_mode].functions;
}

const Vector<ShaderLanguage::ModeInfo>& ShaderTypes::get_modes(RSE::ShaderMode p_mode)
{
	return shader_modes[p_mode].modes;
}

const Vector<ShaderLanguage::ModeInfo>& ShaderTypes::get_stencil_modes(RSE::ShaderMode p_mode)
{
	return shader_modes[p_mode].stencil_modes;
}

const HashSet<String>& ShaderTypes::get_types() { return shader_types; }

const List<String>& ShaderTypes::get_types_list() { return shader_types_list; }

static ShaderLanguage::BuiltInInfo constt(ShaderLanguage::DataType p_type)
{
	return ShaderLanguage::BuiltInInfo(p_type, true);
}

static ShaderLanguage::BuiltInInfo constvt(
	ShaderLanguage::DataType p_type, const Vector<ShaderLanguage::Scalar>& p_values)
{
	return ShaderLanguage::BuiltInInfo(p_type, true, p_values);
}

void ShaderTypes::initialize()
{
	/*************** SPATIAL ***********************/

	ShaderLanguage::Scalar pi_scalar;
	pi_scalar.real = Math::PI;

	ShaderLanguage::Scalar tau_scalar;
	tau_scalar.real = Math::TAU;

	ShaderLanguage::Scalar e_scalar;
	e_scalar.real = Math::E;

	shader_modes[RSE::SHADER_SPATIAL].functions["global"].built_ins["TIME"] =
		constt(ShaderLanguage::TYPE_FLOAT);
	shader_modes[RSE::SHADER_SPATIAL].functions["global"].built_ins["EXPOSURE"] =
		constt(ShaderLanguage::TYPE_FLOAT);
	shader_modes[RSE::SHADER_SPATIAL].functions["global"].built_ins["IN_SHADOW_PASS"] =
		constt(ShaderLanguage::TYPE_BOOL);
	shader_modes[RSE::SHADER_SPATIAL].functions["constants"].built_ins["PI"] =
		constvt(ShaderLanguage::TYPE_FLOAT, {pi_scalar});
	shader_modes[RSE::SHADER_SPATIAL].functions["constants"].built_ins["TAU"] =
		constvt(ShaderLanguage::TYPE_FLOAT, {tau_scalar});
	shader_modes[RSE::SHADER_SPATIAL].functions["constants"].built_ins["E"] =
		constvt(ShaderLanguage::TYPE_FLOAT, {e_scalar});
	shader_modes[RSE::SHADER_SPATIAL].functions["constants"].built_ins["OUTPUT_IS_SRGB"] =
		constt(ShaderLanguage::TYPE_BOOL);
	shader_modes[RSE::SHADER_SPATIAL].functions["constants"].built_ins["CLIP_SPACE_FAR"] =
		constt(ShaderLanguage::TYPE_FLOAT);
	shader_modes[RSE::SHADER_SPATIAL].functions["constants"].built_ins["IS_MULTIVIEW"] =
		constt(ShaderLanguage::TYPE_BOOL);

	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].built_ins["VERTEX"] =
		ShaderLanguage::TYPE_VEC3;
	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].built_ins["NORMAL"] =
		ShaderLanguage::TYPE_VEC3;
	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].built_ins["TANGENT"] =
		ShaderLanguage::TYPE_VEC3;
	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].built_ins["BINORMAL"] =
		ShaderLanguage::TYPE_VEC3;
	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].built_ins["POSITION"] =
		ShaderLanguage::TYPE_VEC4;
	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].built_ins["UV"] =
		ShaderLanguage::TYPE_VEC2;
	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].built_ins["UV2"] =
		ShaderLanguage::TYPE_VEC2;
	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].built_ins["COLOR"] =
		ShaderLanguage::TYPE_VEC4;
	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].built_ins["POINT_SIZE"] =
		ShaderLanguage::TYPE_FLOAT;
	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].built_ins["Z_CLIP_SCALE"] =
		ShaderLanguage::TYPE_FLOAT;
	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].built_ins["INSTANCE_ID"] =
		constt(ShaderLanguage::TYPE_INT);
	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].built_ins["INSTANCE_CUSTOM"] =
		constt(ShaderLanguage::TYPE_VEC4);
	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].built_ins["VERTEX_ID"] =
		constt(ShaderLanguage::TYPE_INT);
	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].built_ins["ROUGHNESS"] =
		ShaderLanguage::TYPE_FLOAT;
	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].built_ins["BONE_INDICES"] =
		constt(ShaderLanguage::TYPE_UVEC4);
	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].built_ins["BONE_WEIGHTS"] =
		constt(ShaderLanguage::TYPE_VEC4);
	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].built_ins["CUSTOM0"] =
		constt(ShaderLanguage::TYPE_VEC4);
	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].built_ins["CUSTOM1"] =
		constt(ShaderLanguage::TYPE_VEC4);
	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].built_ins["CUSTOM2"] =
		constt(ShaderLanguage::TYPE_VEC4);
	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].built_ins["CUSTOM3"] =
		constt(ShaderLanguage::TYPE_VEC4);
	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].can_discard = false;
	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].main_function = true;

	// builtins
	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].built_ins["MODEL_MATRIX"] =
		constt(ShaderLanguage::TYPE_MAT4);
	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].built_ins["MODEL_NORMAL_MATRIX"] =
		constt(ShaderLanguage::TYPE_MAT3);
	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].built_ins["VIEW_MATRIX"] =
		constt(ShaderLanguage::TYPE_MAT4);
	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].built_ins["INV_VIEW_MATRIX"] =
		constt(ShaderLanguage::TYPE_MAT4);
	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].built_ins["PROJECTION_MATRIX"] =
		ShaderLanguage::TYPE_MAT4;
	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].built_ins["INV_PROJECTION_MATRIX"] =
		constt(ShaderLanguage::TYPE_MAT4);
	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].built_ins["MODELVIEW_MATRIX"] =
		ShaderLanguage::TYPE_MAT4;
	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].built_ins["MODELVIEW_NORMAL_MATRIX"] =
		ShaderLanguage::TYPE_MAT3;
	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].built_ins["VIEWPORT_SIZE"] =
		constt(ShaderLanguage::TYPE_VEC2);
	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].built_ins["MAIN_CAM_INV_VIEW_MATRIX"] =
		constt(ShaderLanguage::TYPE_MAT4);

	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].built_ins["NODE_POSITION_WORLD"] =
		constt(ShaderLanguage::TYPE_VEC3);
	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].built_ins["CAMERA_POSITION_WORLD"] =
		constt(ShaderLanguage::TYPE_VEC3);
	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].built_ins["CAMERA_DIRECTION_WORLD"] =
		constt(ShaderLanguage::TYPE_VEC3);
	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].built_ins["CAMERA_VISIBLE_LAYERS"] =
		constt(ShaderLanguage::TYPE_UINT);
	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].built_ins["NODE_POSITION_VIEW"] =
		constt(ShaderLanguage::TYPE_VEC3);

	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].built_ins["VIEW_INDEX"] =
		constt(ShaderLanguage::TYPE_INT);
	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].built_ins["VIEW_MONO_LEFT"] =
		constt(ShaderLanguage::TYPE_INT);
	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].built_ins["VIEW_RIGHT"] =
		constt(ShaderLanguage::TYPE_INT);
	shader_modes[RSE::SHADER_SPATIAL].functions["vertex"].built_ins["EYE_OFFSET"] =
		constt(ShaderLanguage::TYPE_VEC3);

	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["VERTEX"] =
		constt(ShaderLanguage::TYPE_VEC3);
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["LIGHT_VERTEX"] =
		ShaderLanguage::TYPE_VEC3;
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["FRAGCOORD"] =
		constt(ShaderLanguage::TYPE_VEC4);
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["FRONT_FACING"] =
		constt(ShaderLanguage::TYPE_BOOL);
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["NORMAL"] =
		ShaderLanguage::TYPE_VEC3;
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["TANGENT"] =
		ShaderLanguage::TYPE_VEC3;
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["BINORMAL"] =
		ShaderLanguage::TYPE_VEC3;
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["VIEW"] =
		constt(ShaderLanguage::TYPE_VEC3);
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["NORMAL_MAP"] =
		ShaderLanguage::TYPE_VEC3;
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["NORMAL_MAP_DEPTH"] =
		ShaderLanguage::TYPE_FLOAT;
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["BENT_NORMAL_MAP"] =
		ShaderLanguage::TYPE_VEC3;
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["UV"] =
		constt(ShaderLanguage::TYPE_VEC2);
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["UV2"] =
		constt(ShaderLanguage::TYPE_VEC2);
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["COLOR"] =
		constt(ShaderLanguage::TYPE_VEC4);
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["ALBEDO"] =
		ShaderLanguage::TYPE_VEC3;
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["PREMUL_ALPHA_FACTOR"] =
		ShaderLanguage::TYPE_FLOAT;
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["ALPHA"] =
		ShaderLanguage::TYPE_FLOAT;
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["METALLIC"] =
		ShaderLanguage::TYPE_FLOAT;
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["SPECULAR"] =
		ShaderLanguage::TYPE_FLOAT;
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["ROUGHNESS"] =
		ShaderLanguage::TYPE_FLOAT;
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["RIM"] =
		ShaderLanguage::TYPE_FLOAT;
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["RIM_TINT"] =
		ShaderLanguage::TYPE_FLOAT;
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["CLEARCOAT"] =
		ShaderLanguage::TYPE_FLOAT;
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["CLEARCOAT_ROUGHNESS"] =
		ShaderLanguage::TYPE_FLOAT;
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["ANISOTROPY"] =
		ShaderLanguage::TYPE_FLOAT;
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["ANISOTROPY_FLOW"] =
		ShaderLanguage::TYPE_VEC2;
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["SSS_STRENGTH"] =
		ShaderLanguage::TYPE_FLOAT;
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["SSS_TRANSMITTANCE_COLOR"] =
		ShaderLanguage::TYPE_VEC4;
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["SSS_TRANSMITTANCE_DEPTH"] =
		ShaderLanguage::TYPE_FLOAT;
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["SSS_TRANSMITTANCE_BOOST"] =
		ShaderLanguage::TYPE_FLOAT;
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["BACKLIGHT"] =
		ShaderLanguage::TYPE_VEC3;
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["AO"] =
		ShaderLanguage::TYPE_FLOAT;
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["AO_LIGHT_AFFECT"] =
		ShaderLanguage::TYPE_FLOAT;
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["EMISSION"] =
		ShaderLanguage::TYPE_VEC3;
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["DEPTH"] =
		ShaderLanguage::TYPE_FLOAT;
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["SCREEN_UV"] =
		constt(ShaderLanguage::TYPE_VEC2);
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["POINT_COORD"] =
		constt(ShaderLanguage::TYPE_VEC2);

	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["NODE_POSITION_WORLD"] =
		constt(ShaderLanguage::TYPE_VEC3);
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["CAMERA_POSITION_WORLD"] =
		constt(ShaderLanguage::TYPE_VEC3);
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["CAMERA_DIRECTION_WORLD"] =
		constt(ShaderLanguage::TYPE_VEC3);
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["CAMERA_VISIBLE_LAYERS"] =
		constt(ShaderLanguage::TYPE_UINT);
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["NODE_POSITION_VIEW"] =
		constt(ShaderLanguage::TYPE_VEC3);

	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["VIEW_INDEX"] =
		constt(ShaderLanguage::TYPE_INT);
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["VIEW_MONO_LEFT"] =
		constt(ShaderLanguage::TYPE_INT);
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["VIEW_RIGHT"] =
		constt(ShaderLanguage::TYPE_INT);
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["EYE_OFFSET"] =
		constt(ShaderLanguage::TYPE_VEC3);

	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["MODEL_MATRIX"] =
		constt(ShaderLanguage::TYPE_MAT4);
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["MODEL_NORMAL_MATRIX"] =
		constt(ShaderLanguage::TYPE_MAT3);
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["VIEW_MATRIX"] =
		constt(ShaderLanguage::TYPE_MAT4);
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["INV_VIEW_MATRIX"] =
		constt(ShaderLanguage::TYPE_MAT4);
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["PROJECTION_MATRIX"] =
		constt(ShaderLanguage::TYPE_MAT4);
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["INV_PROJECTION_MATRIX"] =
		constt(ShaderLanguage::TYPE_MAT4);
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["VIEWPORT_SIZE"] =
		constt(ShaderLanguage::TYPE_VEC2);
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["FOG"] =
		ShaderLanguage::TYPE_VEC4; // TODO consider adding to light shader
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["RADIANCE"] =
		ShaderLanguage::TYPE_VEC4;
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["IRRADIANCE"] =
		ShaderLanguage::TYPE_VEC4;
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].can_discard = true;
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].main_function = true;

	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["ALPHA_SCISSOR_THRESHOLD"] =
		ShaderLanguage::TYPE_FLOAT;
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["ALPHA_HASH_SCALE"] =
		ShaderLanguage::TYPE_FLOAT;
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["ALPHA_ANTIALIASING_EDGE"] =
		ShaderLanguage::TYPE_FLOAT;
	shader_modes[RSE::SHADER_SPATIAL].functions["fragment"].built_ins["ALPHA_TEXTURE_COORDINATE"] =
		ShaderLanguage::TYPE_VEC2;

	shader_modes[RSE::SHADER_SPATIAL].functions["light"].built_ins["MODEL_MATRIX"] =
		constt(ShaderLanguage::TYPE_MAT4);
	shader_modes[RSE::SHADER_SPATIAL].functions["light"].built_ins["VIEW_MATRIX"] =
		constt(ShaderLanguage::TYPE_MAT4);
	shader_modes[RSE::SHADER_SPATIAL].functions["light"].built_ins["INV_VIEW_MATRIX"] =
		constt(ShaderLanguage::TYPE_MAT4);
	shader_modes[RSE::SHADER_SPATIAL].functions["light"].built_ins["PROJECTION_MATRIX"] =
		constt(ShaderLanguage::TYPE_MAT4);
	shader_modes[RSE::SHADER_SPATIAL].functions["light"].built_ins["INV_PROJECTION_MATRIX"] =
		constt(ShaderLanguage::TYPE_MAT4);
	shader_modes[RSE::SHADER_SPATIAL].functions["light"].built_ins["VIEWPORT_SIZE"] =
		constt(ShaderLanguage::TYPE_VEC2);

	shader_modes[RSE::SHADER_SPATIAL].functions["light"].built_ins["FRAGCOORD"] =
		constt(ShaderLanguage::TYPE_VEC4);
	shader_modes[RSE::SHADER_SPATIAL].functions["light"].built_ins["NORMAL"] =
		constt(ShaderLanguage::TYPE_VEC3);
	shader_modes[RSE::SHADER_SPATIAL].functions["light"].built_ins["UV"] =
		constt(ShaderLanguage::TYPE_VEC2);
	shader_modes[RSE::SHADER_SPATIAL].functions["light"].built_ins["UV2"] =
		constt(ShaderLanguage::TYPE_VEC2);
	shader_modes[RSE::SHADER_SPATIAL].functions["light"].built_ins["VIEW"] =
		constt(ShaderLanguage::TYPE_VEC3);
	shader_modes[RSE::SHADER_SPATIAL].functions["light"].built_ins["SPECULAR_AMOUNT"] =
		constt(ShaderLanguage::TYPE_FLOAT);
	shader_modes[RSE::SHADER_SPATIAL].functions["light"].built_ins["LIGHT"] =
		constt(ShaderLanguage::TYPE_VEC3);
	shader_modes[RSE::SHADER_SPATIAL].functions["light"].built_ins["LIGHT_COLOR"] =
		constt(ShaderLanguage::TYPE_VEC3);
	shader_modes[RSE::SHADER_SPATIAL].functions["light"].built_ins["LIGHT_IS_DIRECTIONAL"] =
		constt(ShaderLanguage::TYPE_BOOL);
	shader_modes[RSE::SHADER_SPATIAL].functions["light"].built_ins["LIGHT_IS_AREA"] =
		constt(ShaderLanguage::TYPE_BOOL);
	shader_modes[RSE::SHADER_SPATIAL]
		.functions["light"]
		.built_ins["LIGHT_AREA_DIFFUSE_MULTIPLIER"] = constt(ShaderLanguage::TYPE_VEC3);
	shader_modes[RSE::SHADER_SPATIAL]
		.functions["light"]
		.built_ins["LIGHT_AREA_SPECULAR_MULTIPLIER"] = constt(ShaderLanguage::TYPE_VEC3);
	shader_modes[RSE::SHADER_SPATIAL].functions["light"].built_ins["ATTENUATION"] =
		constt(ShaderLanguage::TYPE_FLOAT);
	shader_modes[RSE::SHADER_SPATIAL].functions["light"].built_ins["ALBEDO"] =
		constt(ShaderLanguage::TYPE_VEC3);
	shader_modes[RSE::SHADER_SPATIAL].functions["light"].built_ins["BACKLIGHT"] =
		constt(ShaderLanguage::TYPE_VEC3);
	shader_modes[RSE::SHADER_SPATIAL].functions["light"].built_ins["METALLIC"] =
		constt(ShaderLanguage::TYPE_FLOAT);
	shader_modes[RSE::SHADER_SPATIAL].functions["light"].built_ins["ROUGHNESS"] =
		constt(ShaderLanguage::TYPE_FLOAT);
	shader_modes[RSE::SHADER_SPATIAL].functions["light"].built_ins["DIFFUSE_LIGHT"] =
		ShaderLanguage::TYPE_VEC3;
	shader_modes[RSE::SHADER_SPATIAL].functions["light"].built_ins["SPECULAR_LIGHT"] =
		ShaderLanguage::TYPE_VEC3;
	shader_modes[RSE::SHADER_SPATIAL].functions["light"].built_ins["ALPHA"] =
		ShaderLanguage::TYPE_FLOAT;
	shader_modes[RSE::SHADER_SPATIAL].functions["light"].built_ins["SCREEN_UV"] =
		constt(ShaderLanguage::TYPE_VEC2);

	shader_modes[RSE::SHADER_SPATIAL].functions["light"].can_discard = true;
	shader_modes[RSE::SHADER_SPATIAL].functions["light"].main_function = true;

	// spatial render modes
	{
		shader_modes[RSE::SHADER_SPATIAL].modes.push_back(
			{PNAME("blend"), "mix", "add", "sub", "mul", "premul_alpha"});
		shader_modes[RSE::SHADER_SPATIAL].modes.push_back(
			{PNAME("depth_draw"), "opaque", "always", "never"});
		shader_modes[RSE::SHADER_SPATIAL].modes.push_back({PNAME("depth_prepass_alpha")});
		shader_modes[RSE::SHADER_SPATIAL].modes.push_back(
			{PNAME("depth_test"), {"default", "disabled", "inverted"}});
		shader_modes[RSE::SHADER_SPATIAL].modes.push_back({PNAME("sss_mode_skin")});
		shader_modes[RSE::SHADER_SPATIAL].modes.push_back(
			{PNAME("cull"), "back", "front", "disabled"});
		shader_modes[RSE::SHADER_SPATIAL].modes.push_back({PNAME("unshaded")});
		shader_modes[RSE::SHADER_SPATIAL].modes.push_back({PNAME("wireframe")});
		shader_modes[RSE::SHADER_SPATIAL].modes.push_back(
			{PNAME("diffuse"), "lambert", "lambert_wrap", "burley", "toon"});
		shader_modes[RSE::SHADER_SPATIAL].modes.push_back(
			{PNAME("specular"), "schlick_ggx", "toon", "disabled"});
		shader_modes[RSE::SHADER_SPATIAL].modes.push_back({PNAME("skip_vertex_transform")});
		shader_modes[RSE::SHADER_SPATIAL].modes.push_back({PNAME("world_vertex_coords")});
		shader_modes[RSE::SHADER_SPATIAL].modes.push_back({PNAME("ensure_correct_normals")});
		shader_modes[RSE::SHADER_SPATIAL].modes.push_back({PNAME("shadows_disabled")});
		shader_modes[RSE::SHADER_SPATIAL].modes.push_back({PNAME("ambient_light_disabled")});
		shader_modes[RSE::SHADER_SPATIAL].modes.push_back({PNAME("shadow_to_opacity")});
		shader_modes[RSE::SHADER_SPATIAL].modes.push_back({PNAME("vertex_lighting")});
		shader_modes[RSE::SHADER_SPATIAL].modes.push_back({PNAME("particle_trails")});
		shader_modes[RSE::SHADER_SPATIAL].modes.push_back({PNAME("alpha_to_coverage")});
		shader_modes[RSE::SHADER_SPATIAL].modes.push_back({PNAME("alpha_to_coverage_and_one")});
		shader_modes[RSE::SHADER_SPATIAL].modes.push_back({PNAME("debug_shadow_splits")});
		shader_modes[RSE::SHADER_SPATIAL].modes.push_back({PNAME("fog_disabled")});
		shader_modes[RSE::SHADER_SPATIAL].modes.push_back({PNAME("specular_occlusion_disabled")});
		shader_modes[RSE::SHADER_SPATIAL].stencil_modes.push_back({PNAME("read")});
		shader_modes[RSE::SHADER_SPATIAL].stencil_modes.push_back({PNAME("write")});
		shader_modes[RSE::SHADER_SPATIAL].stencil_modes.push_back({PNAME("write_depth_fail")});
		shader_modes[RSE::SHADER_SPATIAL].stencil_modes.push_back(
			{PNAME("compare"), {"always", "less", "equal", "less_or_equal", "greater", "not_equal",
								   "greater_or_equal"}});
	}

	/************ CANVAS ITEM **************************/

	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["global"].built_ins["TIME"] =
		constt(ShaderLanguage::TYPE_FLOAT);
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["constants"].built_ins["PI"] =
		constvt(ShaderLanguage::TYPE_FLOAT, {pi_scalar});
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["constants"].built_ins["TAU"] =
		constvt(ShaderLanguage::TYPE_FLOAT, {tau_scalar});
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["constants"].built_ins["E"] =
		constvt(ShaderLanguage::TYPE_FLOAT, {e_scalar});

	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["vertex"].built_ins["VERTEX"] =
		ShaderLanguage::TYPE_VEC2;
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["vertex"].built_ins["UV"] =
		ShaderLanguage::TYPE_VEC2;
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["vertex"].built_ins["COLOR"] =
		ShaderLanguage::TYPE_VEC4;
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["vertex"].built_ins["POINT_SIZE"] =
		ShaderLanguage::TYPE_FLOAT;

	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["vertex"].built_ins["MODEL_MATRIX"] =
		constt(ShaderLanguage::TYPE_MAT4);
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["vertex"].built_ins["CANVAS_MATRIX"] =
		constt(ShaderLanguage::TYPE_MAT4);
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["vertex"].built_ins["SCREEN_MATRIX"] =
		constt(ShaderLanguage::TYPE_MAT4);
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["vertex"].built_ins["INSTANCE_CUSTOM"] =
		constt(ShaderLanguage::TYPE_VEC4);
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["vertex"].built_ins["INSTANCE_ID"] =
		constt(ShaderLanguage::TYPE_INT);
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["vertex"].built_ins["VERTEX_ID"] =
		constt(ShaderLanguage::TYPE_INT);
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["vertex"].built_ins["AT_LIGHT_PASS"] =
		constt(ShaderLanguage::TYPE_BOOL);
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["vertex"].built_ins["TEXTURE_PIXEL_SIZE"] =
		constt(ShaderLanguage::TYPE_VEC2);
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["vertex"].built_ins["CUSTOM0"] =
		constt(ShaderLanguage::TYPE_VEC4);
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["vertex"].built_ins["CUSTOM1"] =
		constt(ShaderLanguage::TYPE_VEC4);
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["vertex"].can_discard = false;
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["vertex"].main_function = true;

	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["fragment"].built_ins["VERTEX"] =
		ShaderLanguage::TYPE_VEC2;
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["fragment"].built_ins["SHADOW_VERTEX"] =
		ShaderLanguage::TYPE_VEC2;
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["fragment"].built_ins["LIGHT_VERTEX"] =
		ShaderLanguage::TYPE_VEC3;
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["fragment"].built_ins["FRAGCOORD"] =
		constt(ShaderLanguage::TYPE_VEC4);
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["fragment"].built_ins["NORMAL"] =
		ShaderLanguage::TYPE_VEC3;
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["fragment"].built_ins["NORMAL_MAP"] =
		ShaderLanguage::TYPE_VEC3;
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["fragment"].built_ins["NORMAL_MAP_DEPTH"] =
		ShaderLanguage::TYPE_FLOAT;
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["fragment"].built_ins["UV"] =
		constt(ShaderLanguage::TYPE_VEC2);
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["fragment"].built_ins["COLOR"] =
		ShaderLanguage::TYPE_VEC4;
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["fragment"].built_ins["TEXTURE"] =
		constt(ShaderLanguage::TYPE_SAMPLER2D);
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["fragment"].built_ins["TEXTURE_PIXEL_SIZE"] =
		constt(ShaderLanguage::TYPE_VEC2);
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["fragment"].built_ins["NORMAL_TEXTURE"] =
		constt(ShaderLanguage::TYPE_SAMPLER2D);
	shader_modes[RSE::SHADER_CANVAS_ITEM]
		.functions["fragment"]
		.built_ins["SPECULAR_SHININESS_TEXTURE"] = constt(ShaderLanguage::TYPE_SAMPLER2D);
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["fragment"].built_ins["SPECULAR_SHININESS"] =
		constt(ShaderLanguage::TYPE_VEC4);
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["fragment"].built_ins["SCREEN_UV"] =
		constt(ShaderLanguage::TYPE_VEC2);
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["fragment"].built_ins["REGION_RECT"] =
		constt(ShaderLanguage::TYPE_VEC4);
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["fragment"].built_ins["SCREEN_PIXEL_SIZE"] =
		constt(ShaderLanguage::TYPE_VEC2);
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["fragment"].built_ins["POINT_COORD"] =
		constt(ShaderLanguage::TYPE_VEC2);
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["fragment"].built_ins["AT_LIGHT_PASS"] =
		constt(ShaderLanguage::TYPE_BOOL);
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["fragment"].can_discard = true;
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["fragment"].main_function = true;

	{
		ShaderLanguage::StageFunctionInfo func;
		func.skip_function = "vertex";
		func.arguments.push_back(
			ShaderLanguage::StageFunctionInfo::Argument("sdf_pos", ShaderLanguage::TYPE_VEC2));
		func.return_type = ShaderLanguage::TYPE_FLOAT; // whether it could emit
		shader_modes[RSE::SHADER_CANVAS_ITEM].functions["fragment"].stage_functions["texture_sdf"] =
			func;
		shader_modes[RSE::SHADER_CANVAS_ITEM].functions["light"].stage_functions["texture_sdf"] =
			func;
		func.return_type = ShaderLanguage::TYPE_VEC2; // whether it could emit
		shader_modes[RSE::SHADER_CANVAS_ITEM]
			.functions["fragment"]
			.stage_functions["sdf_to_screen_uv"] = func;
		shader_modes[RSE::SHADER_CANVAS_ITEM]
			.functions["light"]
			.stage_functions["sdf_to_screen_uv"] = func;
		shader_modes[RSE::SHADER_CANVAS_ITEM]
			.functions["fragment"]
			.stage_functions["texture_sdf_normal"] = func;
		shader_modes[RSE::SHADER_CANVAS_ITEM]
			.functions["light"]
			.stage_functions["texture_sdf_normal"] = func;
	}

	{
		ShaderLanguage::StageFunctionInfo func;
		func.skip_function = "vertex";
		func.arguments.push_back(
			ShaderLanguage::StageFunctionInfo::Argument("uv", ShaderLanguage::TYPE_VEC2));
		func.return_type = ShaderLanguage::TYPE_VEC2; // whether it could emit
		shader_modes[RSE::SHADER_CANVAS_ITEM]
			.functions["fragment"]
			.stage_functions["screen_uv_to_sdf"] = func;
		shader_modes[RSE::SHADER_CANVAS_ITEM]
			.functions["light"]
			.stage_functions["screen_uv_to_sdf"] = func;
	}

	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["light"].built_ins["FRAGCOORD"] =
		constt(ShaderLanguage::TYPE_VEC4);
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["light"].built_ins["NORMAL"] =
		constt(ShaderLanguage::TYPE_VEC3);
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["light"].built_ins["COLOR"] =
		constt(ShaderLanguage::TYPE_VEC4);
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["light"].built_ins["UV"] =
		constt(ShaderLanguage::TYPE_VEC2);
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["light"].built_ins["SPECULAR_SHININESS"] =
		constt(ShaderLanguage::TYPE_VEC4);
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["light"].built_ins["LIGHT_COLOR"] =
		constt(ShaderLanguage::TYPE_VEC4);
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["light"].built_ins["LIGHT_POSITION"] =
		constt(ShaderLanguage::TYPE_VEC3);
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["light"].built_ins["LIGHT_DIRECTION"] =
		constt(ShaderLanguage::TYPE_VEC3);
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["light"].built_ins["LIGHT_ENERGY"] =
		constt(ShaderLanguage::TYPE_FLOAT);
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["light"].built_ins["LIGHT_IS_DIRECTIONAL"] =
		constt(ShaderLanguage::TYPE_BOOL);
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["light"].built_ins["LIGHT_VERTEX"] =
		constt(ShaderLanguage::TYPE_VEC3);
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["light"].built_ins["LIGHT"] =
		ShaderLanguage::TYPE_VEC4;
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["light"].built_ins["SHADOW_MODULATE"] =
		ShaderLanguage::TYPE_VEC4;
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["light"].built_ins["SCREEN_UV"] =
		constt(ShaderLanguage::TYPE_VEC2);
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["light"].built_ins["TEXTURE"] =
		constt(ShaderLanguage::TYPE_SAMPLER2D);
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["light"].built_ins["TEXTURE_PIXEL_SIZE"] =
		constt(ShaderLanguage::TYPE_VEC2);
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["light"].built_ins["POINT_COORD"] =
		constt(ShaderLanguage::TYPE_VEC2);
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["light"].can_discard = true;
	shader_modes[RSE::SHADER_CANVAS_ITEM].functions["light"].main_function = true;

	// canvasitem render modes
	{
		shader_modes[RSE::SHADER_CANVAS_ITEM].modes.push_back({PNAME("skip_vertex_transform")});
		shader_modes[RSE::SHADER_CANVAS_ITEM].modes.push_back(
			{PNAME("blend"), "mix", "add", "sub", "mul", "premul_alpha", "disabled"});
		shader_modes[RSE::SHADER_CANVAS_ITEM].modes.push_back({PNAME("unshaded")});
		shader_modes[RSE::SHADER_CANVAS_ITEM].modes.push_back({PNAME("light_only")});
		shader_modes[RSE::SHADER_CANVAS_ITEM].modes.push_back({PNAME("world_vertex_coords")});
	}

	/************ PARTICLES **************************/

	shader_modes[RSE::SHADER_PARTICLES].functions["global"].built_ins["TIME"] =
		constt(ShaderLanguage::TYPE_FLOAT);
	shader_modes[RSE::SHADER_PARTICLES].functions["constants"].built_ins["PI"] =
		constvt(ShaderLanguage::TYPE_FLOAT, {pi_scalar});
	shader_modes[RSE::SHADER_PARTICLES].functions["constants"].built_ins["TAU"] =
		constvt(ShaderLanguage::TYPE_FLOAT, {tau_scalar});
	shader_modes[RSE::SHADER_PARTICLES].functions["constants"].built_ins["E"] =
		constvt(ShaderLanguage::TYPE_FLOAT, {e_scalar});

	shader_modes[RSE::SHADER_PARTICLES].functions["start"].built_ins["COLOR"] =
		ShaderLanguage::TYPE_VEC4;
	shader_modes[RSE::SHADER_PARTICLES].functions["start"].built_ins["VELOCITY"] =
		ShaderLanguage::TYPE_VEC3;
	shader_modes[RSE::SHADER_PARTICLES].functions["start"].built_ins["MASS"] =
		ShaderLanguage::TYPE_FLOAT;
	shader_modes[RSE::SHADER_PARTICLES].functions["start"].built_ins["ACTIVE"] =
		ShaderLanguage::TYPE_BOOL;
	shader_modes[RSE::SHADER_PARTICLES].functions["start"].built_ins["CUSTOM"] =
		ShaderLanguage::TYPE_VEC4;
	shader_modes[RSE::SHADER_PARTICLES].functions["start"].built_ins["USERDATA1"] =
		ShaderLanguage::TYPE_VEC4;
	shader_modes[RSE::SHADER_PARTICLES].functions["start"].built_ins["USERDATA2"] =
		ShaderLanguage::TYPE_VEC4;
	shader_modes[RSE::SHADER_PARTICLES].functions["start"].built_ins["USERDATA3"] =
		ShaderLanguage::TYPE_VEC4;
	shader_modes[RSE::SHADER_PARTICLES].functions["start"].built_ins["USERDATA4"] =
		ShaderLanguage::TYPE_VEC4;
	shader_modes[RSE::SHADER_PARTICLES].functions["start"].built_ins["USERDATA5"] =
		ShaderLanguage::TYPE_VEC4;
	shader_modes[RSE::SHADER_PARTICLES].functions["start"].built_ins["USERDATA6"] =
		ShaderLanguage::TYPE_VEC4;
	shader_modes[RSE::SHADER_PARTICLES].functions["start"].built_ins["TRANSFORM"] =
		ShaderLanguage::TYPE_MAT4;
	shader_modes[RSE::SHADER_PARTICLES].functions["start"].built_ins["LIFETIME"] =
		constt(ShaderLanguage::TYPE_FLOAT);
	shader_modes[RSE::SHADER_PARTICLES].functions["start"].built_ins["DELTA"] =
		constt(ShaderLanguage::TYPE_FLOAT);
	shader_modes[RSE::SHADER_PARTICLES].functions["start"].built_ins["NUMBER"] =
		constt(ShaderLanguage::TYPE_UINT);
	shader_modes[RSE::SHADER_PARTICLES].functions["start"].built_ins["INDEX"] =
		constt(ShaderLanguage::TYPE_UINT);
	shader_modes[RSE::SHADER_PARTICLES].functions["start"].built_ins["EMISSION_TRANSFORM"] =
		constt(ShaderLanguage::TYPE_MAT4);
	shader_modes[RSE::SHADER_PARTICLES].functions["start"].built_ins["EMITTER_VELOCITY"] =
		constt(ShaderLanguage::TYPE_VEC3);
	shader_modes[RSE::SHADER_PARTICLES].functions["start"].built_ins["INTERPOLATE_TO_END"] =
		constt(ShaderLanguage::TYPE_FLOAT);
	shader_modes[RSE::SHADER_PARTICLES].functions["start"].built_ins["RANDOM_SEED"] =
		constt(ShaderLanguage::TYPE_UINT);
	shader_modes[RSE::SHADER_PARTICLES].functions["start"].built_ins["FLAG_EMIT_POSITION"] =
		constt(ShaderLanguage::TYPE_UINT);
	shader_modes[RSE::SHADER_PARTICLES].functions["start"].built_ins["FLAG_EMIT_ROT_SCALE"] =
		constt(ShaderLanguage::TYPE_UINT);
	shader_modes[RSE::SHADER_PARTICLES].functions["start"].built_ins["FLAG_EMIT_VELOCITY"] =
		constt(ShaderLanguage::TYPE_UINT);
	shader_modes[RSE::SHADER_PARTICLES].functions["start"].built_ins["FLAG_EMIT_COLOR"] =
		constt(ShaderLanguage::TYPE_UINT);
	shader_modes[RSE::SHADER_PARTICLES].functions["start"].built_ins["FLAG_EMIT_CUSTOM"] =
		constt(ShaderLanguage::TYPE_UINT);
	shader_modes[RSE::SHADER_PARTICLES].functions["start"].built_ins["RESTART_POSITION"] =
		constt(ShaderLanguage::TYPE_BOOL);
	shader_modes[RSE::SHADER_PARTICLES].functions["start"].built_ins["RESTART_ROT_SCALE"] =
		constt(ShaderLanguage::TYPE_BOOL);
	shader_modes[RSE::SHADER_PARTICLES].functions["start"].built_ins["RESTART_VELOCITY"] =
		constt(ShaderLanguage::TYPE_BOOL);
	shader_modes[RSE::SHADER_PARTICLES].functions["start"].built_ins["RESTART_COLOR"] =
		constt(ShaderLanguage::TYPE_BOOL);
	shader_modes[RSE::SHADER_PARTICLES].functions["start"].built_ins["RESTART_CUSTOM"] =
		constt(ShaderLanguage::TYPE_BOOL);
	shader_modes[RSE::SHADER_PARTICLES].functions["start"].built_ins["AMOUNT_RATIO"] =
		constt(ShaderLanguage::TYPE_FLOAT);
	shader_modes[RSE::SHADER_PARTICLES].functions["start"].main_function = true;

	shader_modes[RSE::SHADER_PARTICLES].functions["process"].built_ins["COLOR"] =
		ShaderLanguage::TYPE_VEC4;
	shader_modes[RSE::SHADER_PARTICLES].functions["process"].built_ins["VELOCITY"] =
		ShaderLanguage::TYPE_VEC3;
	shader_modes[RSE::SHADER_PARTICLES].functions["process"].built_ins["MASS"] =
		ShaderLanguage::TYPE_FLOAT;
	shader_modes[RSE::SHADER_PARTICLES].functions["process"].built_ins["ACTIVE"] =
		ShaderLanguage::TYPE_BOOL;
	shader_modes[RSE::SHADER_PARTICLES].functions["process"].built_ins["RESTART"] =
		constt(ShaderLanguage::TYPE_BOOL);
	shader_modes[RSE::SHADER_PARTICLES].functions["process"].built_ins["CUSTOM"] =
		ShaderLanguage::TYPE_VEC4;
	shader_modes[RSE::SHADER_PARTICLES].functions["process"].built_ins["USERDATA1"] =
		ShaderLanguage::TYPE_VEC4;
	shader_modes[RSE::SHADER_PARTICLES].functions["process"].built_ins["USERDATA2"] =
		ShaderLanguage::TYPE_VEC4;
	shader_modes[RSE::SHADER_PARTICLES].functions["process"].built_ins["USERDATA3"] =
		ShaderLanguage::TYPE_VEC4;
	shader_modes[RSE::SHADER_PARTICLES].functions["process"].built_ins["USERDATA4"] =
		ShaderLanguage::TYPE_VEC4;
	shader_modes[RSE::SHADER_PARTICLES].functions["process"].built_ins["USERDATA5"] =
		ShaderLanguage::TYPE_VEC4;
	shader_modes[RSE::SHADER_PARTICLES].functions["process"].built_ins["USERDATA6"] =
		ShaderLanguage::TYPE_VEC4;
	shader_modes[RSE::SHADER_PARTICLES].functions["process"].built_ins["TRANSFORM"] =
		ShaderLanguage::TYPE_MAT4;
	shader_modes[RSE::SHADER_PARTICLES].functions["process"].built_ins["LIFETIME"] =
		constt(ShaderLanguage::TYPE_FLOAT);
	shader_modes[RSE::SHADER_PARTICLES].functions["process"].built_ins["DELTA"] =
		constt(ShaderLanguage::TYPE_FLOAT);
	shader_modes[RSE::SHADER_PARTICLES].functions["process"].built_ins["NUMBER"] =
		constt(ShaderLanguage::TYPE_UINT);
	shader_modes[RSE::SHADER_PARTICLES].functions["process"].built_ins["INDEX"] =
		constt(ShaderLanguage::TYPE_UINT);
	shader_modes[RSE::SHADER_PARTICLES].functions["process"].built_ins["EMISSION_TRANSFORM"] =
		constt(ShaderLanguage::TYPE_MAT4);
	shader_modes[RSE::SHADER_PARTICLES].functions["process"].built_ins["EMITTER_VELOCITY"] =
		constt(ShaderLanguage::TYPE_VEC3);
	shader_modes[RSE::SHADER_PARTICLES].functions["process"].built_ins["INTERPOLATE_TO_END"] =
		constt(ShaderLanguage::TYPE_FLOAT);
	shader_modes[RSE::SHADER_PARTICLES].functions["process"].built_ins["RANDOM_SEED"] =
		constt(ShaderLanguage::TYPE_UINT);
	shader_modes[RSE::SHADER_PARTICLES].functions["process"].built_ins["FLAG_EMIT_POSITION"] =
		constt(ShaderLanguage::TYPE_UINT);
	shader_modes[RSE::SHADER_PARTICLES].functions["process"].built_ins["FLAG_EMIT_ROT_SCALE"] =
		constt(ShaderLanguage::TYPE_UINT);
	shader_modes[RSE::SHADER_PARTICLES].functions["process"].built_ins["FLAG_EMIT_VELOCITY"] =
		constt(ShaderLanguage::TYPE_UINT);
	shader_modes[RSE::SHADER_PARTICLES].functions["process"].built_ins["FLAG_EMIT_COLOR"] =
		constt(ShaderLanguage::TYPE_UINT);
	shader_modes[RSE::SHADER_PARTICLES].functions["process"].built_ins["FLAG_EMIT_CUSTOM"] =
		constt(ShaderLanguage::TYPE_UINT);
	shader_modes[RSE::SHADER_PARTICLES].functions["process"].built_ins["COLLIDED"] =
		constt(ShaderLanguage::TYPE_BOOL);
	shader_modes[RSE::SHADER_PARTICLES].functions["process"].built_ins["COLLISION_NORMAL"] =
		constt(ShaderLanguage::TYPE_VEC3);
	shader_modes[RSE::SHADER_PARTICLES].functions["process"].built_ins["COLLISION_DEPTH"] =
		constt(ShaderLanguage::TYPE_FLOAT);
	shader_modes[RSE::SHADER_PARTICLES].functions["process"].built_ins["ATTRACTOR_FORCE"] =
		constt(ShaderLanguage::TYPE_VEC3);
	shader_modes[RSE::SHADER_PARTICLES].functions["process"].built_ins["AMOUNT_RATIO"] =
		constt(ShaderLanguage::TYPE_FLOAT);
	shader_modes[RSE::SHADER_PARTICLES].functions["process"].main_function = true;

	{
		ShaderLanguage::StageFunctionInfo emit_vertex_func;
		emit_vertex_func.arguments.push_back(
			ShaderLanguage::StageFunctionInfo::Argument("xform", ShaderLanguage::TYPE_MAT4));
		emit_vertex_func.arguments.push_back(
			ShaderLanguage::StageFunctionInfo::Argument("velocity", ShaderLanguage::TYPE_VEC3));
		emit_vertex_func.arguments.push_back(
			ShaderLanguage::StageFunctionInfo::Argument("color", ShaderLanguage::TYPE_VEC4));
		emit_vertex_func.arguments.push_back(
			ShaderLanguage::StageFunctionInfo::Argument("custom", ShaderLanguage::TYPE_VEC4));
		emit_vertex_func.arguments.push_back(
			ShaderLanguage::StageFunctionInfo::Argument("flags", ShaderLanguage::TYPE_UINT));
		emit_vertex_func.return_type = ShaderLanguage::TYPE_BOOL; // whether it could emit
		shader_modes[RSE::SHADER_PARTICLES].functions["start"].stage_functions["emit_subparticle"] =
			emit_vertex_func;
		shader_modes[RSE::SHADER_PARTICLES]
			.functions["process"]
			.stage_functions["emit_subparticle"] = emit_vertex_func;
	}

	// particles render modes
	{
		shader_modes[RSE::SHADER_PARTICLES].modes.push_back({PNAME("collision_use_scale")});
		shader_modes[RSE::SHADER_PARTICLES].modes.push_back({PNAME("disable_force")});
		shader_modes[RSE::SHADER_PARTICLES].modes.push_back({PNAME("disable_velocity")});
		shader_modes[RSE::SHADER_PARTICLES].modes.push_back({PNAME("keep_data")});
	}

	/************ SKY **************************/

	shader_modes[RSE::SHADER_SKY].functions["global"].built_ins["TIME"] =
		constt(ShaderLanguage::TYPE_FLOAT);
	shader_modes[RSE::SHADER_SKY].functions["constants"].built_ins["PI"] =
		constvt(ShaderLanguage::TYPE_FLOAT, {pi_scalar});
	shader_modes[RSE::SHADER_SKY].functions["constants"].built_ins["TAU"] =
		constvt(ShaderLanguage::TYPE_FLOAT, {tau_scalar});
	shader_modes[RSE::SHADER_SKY].functions["constants"].built_ins["E"] =
		constvt(ShaderLanguage::TYPE_FLOAT, {e_scalar});
	shader_modes[RSE::SHADER_SKY].functions["global"].built_ins["POSITION"] =
		constt(ShaderLanguage::TYPE_VEC3);
	shader_modes[RSE::SHADER_SKY].functions["global"].built_ins["RADIANCE"] =
		constt(ShaderLanguage::TYPE_SAMPLERCUBE);
	shader_modes[RSE::SHADER_SKY].functions["global"].built_ins["AT_HALF_RES_PASS"] =
		constt(ShaderLanguage::TYPE_BOOL);
	shader_modes[RSE::SHADER_SKY].functions["global"].built_ins["AT_QUARTER_RES_PASS"] =
		constt(ShaderLanguage::TYPE_BOOL);
	shader_modes[RSE::SHADER_SKY].functions["global"].built_ins["AT_CUBEMAP_PASS"] =
		constt(ShaderLanguage::TYPE_BOOL);
	shader_modes[RSE::SHADER_SKY].functions["global"].built_ins["LIGHT0_ENABLED"] =
		constt(ShaderLanguage::TYPE_BOOL);
	shader_modes[RSE::SHADER_SKY].functions["global"].built_ins["LIGHT0_DIRECTION"] =
		constt(ShaderLanguage::TYPE_VEC3);
	shader_modes[RSE::SHADER_SKY].functions["global"].built_ins["LIGHT0_ENERGY"] =
		constt(ShaderLanguage::TYPE_FLOAT);
	shader_modes[RSE::SHADER_SKY].functions["global"].built_ins["LIGHT0_COLOR"] =
		constt(ShaderLanguage::TYPE_VEC3);
	shader_modes[RSE::SHADER_SKY].functions["global"].built_ins["LIGHT0_SIZE"] =
		constt(ShaderLanguage::TYPE_FLOAT);
	shader_modes[RSE::SHADER_SKY].functions["global"].built_ins["LIGHT1_ENABLED"] =
		constt(ShaderLanguage::TYPE_BOOL);
	shader_modes[RSE::SHADER_SKY].functions["global"].built_ins["LIGHT1_DIRECTION"] =
		constt(ShaderLanguage::TYPE_VEC3);
	shader_modes[RSE::SHADER_SKY].functions["global"].built_ins["LIGHT1_ENERGY"] =
		constt(ShaderLanguage::TYPE_FLOAT);
	shader_modes[RSE::SHADER_SKY].functions["global"].built_ins["LIGHT1_COLOR"] =
		constt(ShaderLanguage::TYPE_VEC3);
	shader_modes[RSE::SHADER_SKY].functions["global"].built_ins["LIGHT1_SIZE"] =
		constt(ShaderLanguage::TYPE_FLOAT);
	shader_modes[RSE::SHADER_SKY].functions["global"].built_ins["LIGHT2_ENABLED"] =
		constt(ShaderLanguage::TYPE_BOOL);
	shader_modes[RSE::SHADER_SKY].functions["global"].built_ins["LIGHT2_DIRECTION"] =
		constt(ShaderLanguage::TYPE_VEC3);
	shader_modes[RSE::SHADER_SKY].functions["global"].built_ins["LIGHT2_ENERGY"] =
		constt(ShaderLanguage::TYPE_FLOAT);
	shader_modes[RSE::SHADER_SKY].functions["global"].built_ins["LIGHT2_COLOR"] =
		constt(ShaderLanguage::TYPE_VEC3);
	shader_modes[RSE::SHADER_SKY].functions["global"].built_ins["LIGHT2_SIZE"] =
		constt(ShaderLanguage::TYPE_FLOAT);
	shader_modes[RSE::SHADER_SKY].functions["global"].built_ins["LIGHT3_ENABLED"] =
		constt(ShaderLanguage::TYPE_BOOL);
	shader_modes[RSE::SHADER_SKY].functions["global"].built_ins["LIGHT3_DIRECTION"] =
		constt(ShaderLanguage::TYPE_VEC3);
	shader_modes[RSE::SHADER_SKY].functions["global"].built_ins["LIGHT3_ENERGY"] =
		constt(ShaderLanguage::TYPE_FLOAT);
	shader_modes[RSE::SHADER_SKY].functions["global"].built_ins["LIGHT3_COLOR"] =
		constt(ShaderLanguage::TYPE_VEC3);
	shader_modes[RSE::SHADER_SKY].functions["global"].built_ins["LIGHT3_SIZE"] =
		constt(ShaderLanguage::TYPE_FLOAT);

	shader_modes[RSE::SHADER_SKY].functions["sky"].built_ins["COLOR"] = ShaderLanguage::TYPE_VEC3;
	shader_modes[RSE::SHADER_SKY].functions["sky"].built_ins["ALPHA"] = ShaderLanguage::TYPE_FLOAT;
	shader_modes[RSE::SHADER_SKY].functions["sky"].built_ins["EYEDIR"] =
		constt(ShaderLanguage::TYPE_VEC3);
	shader_modes[RSE::SHADER_SKY].functions["sky"].built_ins["SCREEN_UV"] =
		constt(ShaderLanguage::TYPE_VEC2);
	shader_modes[RSE::SHADER_SKY].functions["sky"].built_ins["FRAGCOORD"] =
		constt(ShaderLanguage::TYPE_VEC4);
	shader_modes[RSE::SHADER_SKY].functions["sky"].built_ins["SKY_COORDS"] =
		constt(ShaderLanguage::TYPE_VEC2);
	shader_modes[RSE::SHADER_SKY].functions["sky"].built_ins["HALF_RES_COLOR"] =
		constt(ShaderLanguage::TYPE_VEC4);
	shader_modes[RSE::SHADER_SKY].functions["sky"].built_ins["QUARTER_RES_COLOR"] =
		constt(ShaderLanguage::TYPE_VEC4);
	shader_modes[RSE::SHADER_SKY].functions["sky"].built_ins["FOG"] = ShaderLanguage::TYPE_VEC4;
	shader_modes[RSE::SHADER_SKY].functions["sky"].main_function = true;

	// sky render modes
	{
		shader_modes[RSE::SHADER_SKY].modes.push_back({PNAME("use_half_res_pass")});
		shader_modes[RSE::SHADER_SKY].modes.push_back({PNAME("use_quarter_res_pass")});
		shader_modes[RSE::SHADER_SKY].modes.push_back({PNAME("disable_fog")});
		shader_modes[RSE::SHADER_SKY].modes.push_back({PNAME("use_debanding")});
	}

	/************ FOG **************************/

	shader_modes[RSE::SHADER_FOG].functions["global"].built_ins["TIME"] =
		constt(ShaderLanguage::TYPE_FLOAT);
	shader_modes[RSE::SHADER_FOG].functions["constants"].built_ins["PI"] =
		constvt(ShaderLanguage::TYPE_FLOAT, {pi_scalar});
	shader_modes[RSE::SHADER_FOG].functions["constants"].built_ins["TAU"] =
		constvt(ShaderLanguage::TYPE_FLOAT, {tau_scalar});
	shader_modes[RSE::SHADER_FOG].functions["constants"].built_ins["E"] =
		constvt(ShaderLanguage::TYPE_FLOAT, {e_scalar});

	shader_modes[RSE::SHADER_FOG].functions["fog"].built_ins["WORLD_POSITION"] =
		constt(ShaderLanguage::TYPE_VEC3);
	shader_modes[RSE::SHADER_FOG].functions["fog"].built_ins["OBJECT_POSITION"] =
		constt(ShaderLanguage::TYPE_VEC3);
	shader_modes[RSE::SHADER_FOG].functions["fog"].built_ins["UVW"] =
		constt(ShaderLanguage::TYPE_VEC3);
	shader_modes[RSE::SHADER_FOG].functions["fog"].built_ins["SIZE"] =
		constt(ShaderLanguage::TYPE_VEC3);
	shader_modes[RSE::SHADER_FOG].functions["fog"].built_ins["SDF"] =
		constt(ShaderLanguage::TYPE_FLOAT);
	shader_modes[RSE::SHADER_FOG].functions["fog"].built_ins["ALBEDO"] = ShaderLanguage::TYPE_VEC3;
	shader_modes[RSE::SHADER_FOG].functions["fog"].built_ins["DENSITY"] =
		ShaderLanguage::TYPE_FLOAT;
	shader_modes[RSE::SHADER_FOG].functions["fog"].built_ins["EMISSION"] =
		ShaderLanguage::TYPE_VEC3;
	shader_modes[RSE::SHADER_FOG].functions["fog"].main_function = true;

	/************ TEXTURE_BLIT **************************/

	shader_modes[RSE::SHADER_TEXTURE_BLIT].functions["global"].built_ins["TIME"] =
		constt(ShaderLanguage::TYPE_FLOAT);
	shader_modes[RSE::SHADER_TEXTURE_BLIT].functions["constants"].built_ins["PI"] =
		constvt(ShaderLanguage::TYPE_FLOAT, {pi_scalar});
	shader_modes[RSE::SHADER_TEXTURE_BLIT].functions["constants"].built_ins["TAU"] =
		constvt(ShaderLanguage::TYPE_FLOAT, {tau_scalar});
	shader_modes[RSE::SHADER_TEXTURE_BLIT].functions["constants"].built_ins["E"] =
		constvt(ShaderLanguage::TYPE_FLOAT, {e_scalar});

	shader_modes[RSE::SHADER_TEXTURE_BLIT].functions["blit"].built_ins["FRAGCOORD"] =
		constt(ShaderLanguage::TYPE_VEC4);

	shader_modes[RSE::SHADER_TEXTURE_BLIT].functions["blit"].built_ins["UV"] =
		constt(ShaderLanguage::TYPE_VEC2);
	shader_modes[RSE::SHADER_TEXTURE_BLIT].functions["blit"].built_ins["MODULATE"] =
		constt(ShaderLanguage::TYPE_VEC4);

	shader_modes[RSE::SHADER_TEXTURE_BLIT].functions["blit"].built_ins["COLOR0"] =
		ShaderLanguage::TYPE_VEC4;
	shader_modes[RSE::SHADER_TEXTURE_BLIT].functions["blit"].built_ins["COLOR1"] =
		ShaderLanguage::TYPE_VEC4;
	shader_modes[RSE::SHADER_TEXTURE_BLIT].functions["blit"].built_ins["COLOR2"] =
		ShaderLanguage::TYPE_VEC4;
	shader_modes[RSE::SHADER_TEXTURE_BLIT].functions["blit"].built_ins["COLOR3"] =
		ShaderLanguage::TYPE_VEC4;

	shader_modes[RSE::SHADER_TEXTURE_BLIT].functions["blit"].main_function = true;

	// Texture Blit Modes
	{
		shader_modes[RSE::SHADER_TEXTURE_BLIT].modes.push_back(
			{PNAME("blend"), "mix", "add", "sub", "mul", "disabled"});
	}

	// Must be kept in sync with the Shader::Mode enum.
	shader_types_list.push_back("spatial");
	shader_types_list.push_back("canvas_item");
	shader_types_list.push_back("particles");
	shader_types_list.push_back("sky");
	shader_types_list.push_back("fog");
	shader_types_list.push_back("texture_blit");
	DEV_ASSERT(shader_types_list.size() == Shader::MODE_MAX);

	for (const String& type : shader_types_list) {
		shader_types.insert(type);
	}
}

void ShaderTypes::finalize()
{
    shader_modes.clear();
    shader_types.clear();
    shader_types_list.clear();
}

// --- Merged from servers/rendering/shader_warnings.cpp ---

#ifdef DEBUG_ENABLED

ShaderWarning::Code ShaderWarning::get_code() const { return code; }

int ShaderWarning::get_line() const { return line; }

const String& ShaderWarning::get_subject() const { return subject; }

String ShaderWarning::get_message() const
{
	switch (code) {
	case FLOAT_COMPARISON:
		return vformat(RTR(
			"Direct floating-point comparison (this may not evaluate to `true` as you expect). "
			"Instead, use `abs(a - b) < 0.0001` for an approximate but predictable comparison."));
	case UNUSED_CONSTANT:
		return vformat(RTR("The const '%s' is declared but never used."), subject);
	case UNUSED_FUNCTION:
		return vformat(RTR("The function '%s' is declared but never used."), subject);
	case UNUSED_STRUCT:
		return vformat(RTR("The struct '%s' is declared but never used."), subject);
	case UNUSED_UNIFORM:
		return vformat(RTR("The uniform '%s' is declared but never used."), subject);
	case UNUSED_VARYING:
		return vformat(RTR("The varying '%s' is declared but never used."), subject);
	case UNUSED_LOCAL_VARIABLE:
		return vformat(RTR("The local variable '%s' is declared but never used."), subject);
	case FORMATTING_ERROR:
		return subject;
	case MAGIC_POSITION_WRITE:
		return vformat(RTR(
			"You are attempting to assign the VERTEX position in model space to the vertex "
			"POSITION in clip space. The definition of clip space changed in version 4.3, so if "
			"this code was written prior to 4.3, it will not continue to work. Consider specifying "
			"the clip space z-component directly i.e. use `vec4(VERTEX.xy, 1.0, 1.0)`."));
	default:
		break;
	}
	return String();
}

String ShaderWarning::get_name() const { return get_name_from_code(code); }

String ShaderWarning::get_name_from_code(Code p_code)
{
	ERR_FAIL_INDEX_V(p_code, WARNING_MAX, String());

	static const char* names[] = {
		PNAME("FLOAT_COMPARISON"),
		PNAME("UNUSED_CONSTANT"),
		PNAME("UNUSED_FUNCTION"),
		PNAME("UNUSED_STRUCT"),
		PNAME("UNUSED_UNIFORM"),
		PNAME("UNUSED_VARYING"),
		PNAME("UNUSED_LOCAL_VARIABLE"),
		PNAME("FORMATTING_ERROR"),
		PNAME("DEVICE_LIMIT_EXCEEDED"),
		PNAME("MAGIC_POSITION_WRITE"),
	};

	static_assert(std_size(names) == WARNING_MAX,
		"Amount of warning types don't match the amount of warning names.");

	return names[(int)p_code];
}

ShaderWarning::Code ShaderWarning::get_code_from_name(const String& p_name)
{
	for (int i = 0; i < WARNING_MAX; i++) {
		if (get_name_from_code((Code)i) == p_name) {
			return (Code)i;
		}
	}

	ERR_FAIL_V_MSG(WARNING_MAX, "Invalid shader warning name: " + p_name);
}

static HashMap<int, uint32_t>* code_to_flags_map = nullptr;

static void init_code_to_flags_map()
{
	code_to_flags_map = memnew((HashMap<int, uint32_t>));
	code_to_flags_map->insert(
		ShaderWarning::FLOAT_COMPARISON, ShaderWarning::FLOAT_COMPARISON_FLAG);
	code_to_flags_map->insert(ShaderWarning::UNUSED_CONSTANT, ShaderWarning::UNUSED_CONSTANT_FLAG);
	code_to_flags_map->insert(ShaderWarning::UNUSED_FUNCTION, ShaderWarning::UNUSED_FUNCTION_FLAG);
	code_to_flags_map->insert(ShaderWarning::UNUSED_STRUCT, ShaderWarning::UNUSED_STRUCT_FLAG);
	code_to_flags_map->insert(ShaderWarning::UNUSED_UNIFORM, ShaderWarning::UNUSED_UNIFORM_FLAG);
	code_to_flags_map->insert(ShaderWarning::UNUSED_VARYING, ShaderWarning::UNUSED_VARYING_FLAG);
	code_to_flags_map->insert(
		ShaderWarning::UNUSED_LOCAL_VARIABLE, ShaderWarning::UNUSED_LOCAL_VARIABLE_FLAG);
	code_to_flags_map->insert(
		ShaderWarning::FORMATTING_ERROR, ShaderWarning::FORMATTING_ERROR_FLAG);
	code_to_flags_map->insert(
		ShaderWarning::DEVICE_LIMIT_EXCEEDED, ShaderWarning::DEVICE_LIMIT_EXCEEDED_FLAG);
	code_to_flags_map->insert(
		ShaderWarning::MAGIC_POSITION_WRITE, ShaderWarning::MAGIC_POSITION_WRITE_FLAG);
}

ShaderWarning::CodeFlags ShaderWarning::get_flags_from_codemap(const HashMap<Code, bool>& p_map)
{
	uint32_t result = 0U;

	if (code_to_flags_map == nullptr) {
		init_code_to_flags_map();
	}

	for (const KeyValue<Code, bool>& E : p_map) {
		if (E.value) {
			ERR_FAIL_COND_V(!code_to_flags_map->has((int)E.key), ShaderWarning::NONE_FLAG);
			result |= (*code_to_flags_map)[(int)E.key];
		}
	}
	return (CodeFlags)result;
}

#endif // DEBUG_ENABLED

// --- Merged from servers/rendering/shader_include_db.cpp ---

HashMap<String, String> ShaderIncludeDB::built_in_includes;


void ShaderIncludeDB::register_built_in_include_file(
	const String& p_filename, const String& p_shader_code)
{
	built_in_includes[p_filename] = p_shader_code;
}

PackedStringArray ShaderIncludeDB::list_built_in_include_files()
{
	PackedStringArray ret;

	for (const KeyValue<String, String>& e : built_in_includes) {
		ret.push_back(e.key);
	}

	return ret;
}

bool ShaderIncludeDB::has_built_in_include_file(const String& p_filename)
{
	return built_in_includes.has(p_filename);
}

String ShaderIncludeDB::get_built_in_include_file(const String& p_filename)
{
	const String* ptr = built_in_includes.getptr(p_filename);

	return ptr ? *ptr : String();
}

String ShaderIncludeDB::parse_include_files(const String& p_code)
{
	// Prevent needless processing if we don't have any includes.
	if (p_code.find("#include ") == -1) {
		return p_code;
	}

	const String include = "#include ";
	String parsed_code;

	Vector<String> lines = p_code.split("\n");
	int line_count = lines.size();
	for (int i = 0; i < line_count; i++) {
		const String& l = lines[i];

		if (l.begins_with(include)) {
			String include_file = l.replace(include, "").strip_edges();
			if (include_file[0] == '"') {
				int end_pos = include_file.find_char('"', 1);
				if (end_pos >= 0) {
					include_file = include_file.substr(1, end_pos - 1);

					String include_code = ShaderIncludeDB::get_built_in_include_file(include_file);
					if (!include_code.is_empty()) {
						// Add these lines into our parse list so we parse them as well.
						Vector<String> include_lines = include_code.split("\n");

						for (int j = include_lines.size() - 1; j >= 0; j--) {
							lines.insert(i + 1, include_lines[j]);
						}

						line_count = lines.size();
					}
					else {
						// Just add it back in, this will cause a compile error to alert the user.
						parsed_code += l + "\n";
					}
				}
				else {
					// Include as is.
					parsed_code += l + "\n";
				}
			}
			else {
				// Include as is.
				parsed_code += l + "\n";
			}
		}
		else {
			// Include as is.
			parsed_code += l + "\n";
		}
	}

	return parsed_code;
}
