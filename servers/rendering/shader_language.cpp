/**************************************************************************/
/*  shader_language.cpp                                                   */
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
#include "core/os/os.h"
#include "core/templates/local_vector.h"
#include "core/templates/rb_set.h"
#include "servers/rendering/renderer.h"
#include "servers/rendering/shader_language.h"
#include "servers/rendering/storage/utilities.h"
#include "shader_language.h"

#define HAS_WARNING(flag) (warning_flags & flag)

SafeNumeric<int> ShaderLanguage::instance_counter;

String ShaderLanguage::get_operator_text(Operator p_op)
{
	static const char* op_names[OP_MAX] = {"==", "!=", "<", "<=", ">", ">=", "&&", "||", "!", "-",
		"+", "-", "*", "/", "%", "<<", ">>", "=",
		"+=", "-=", "*=", "/=", "%=", "<<=", ">>=", "&=", "|=", "^=", "&", "|", "^", "~", "++",
		"--", "?", ":", "++", "--", "()", "construct", "index", "empty"};

	return op_names[p_op];
}

const char* ShaderLanguage::token_names[TK_MAX] = {
	"EMPTY",
	"IDENTIFIER",
	"TRUE",
	"FALSE",
	"FLOAT_CONSTANT",
	"INT_CONSTANT",
	"UINT_CONSTANT",
	"STRING_CONSTANT",
	"TYPE_VOID",
	"TYPE_BOOL",
	"TYPE_BVEC2",
	"TYPE_BVEC3",
	"TYPE_BVEC4",
	"TYPE_INT",
	"TYPE_IVEC2",
	"TYPE_IVEC3",
	"TYPE_IVEC4",
	"TYPE_UINT",
	"TYPE_UVEC2",
	"TYPE_UVEC3",
	"TYPE_UVEC4",
	"TYPE_FLOAT",
	"TYPE_VEC2",
	"TYPE_VEC3",
	"TYPE_VEC4",
	"TYPE_MAT2",
	"TYPE_MAT3",
	"TYPE_MAT4",
	"TYPE_SAMPLER2D",
	"TYPE_ISAMPLER2D",
	"TYPE_USAMPLER2D",
	"TYPE_SAMPLER2DARRAY",
	"TYPE_ISAMPLER2DARRAY",
	"TYPE_USAMPLER2DARRAY",
	"TYPE_SAMPLER3D",
	"TYPE_ISAMPLER3D",
	"TYPE_USAMPLER3D",
	"TYPE_SAMPLERCUBE",
	"TYPE_SAMPLERCUBEARRAY",
	"TYPE_SAMPLEREXT",
	"INTERPOLATION_FLAT",
	"INTERPOLATION_SMOOTH",
	"CONST",
	"STRUCT",
	"PRECISION_LOW",
	"PRECISION_MID",
	"PRECISION_HIGH",
	"OP_EQUAL",
	"OP_NOT_EQUAL",
	"OP_LESS",
	"OP_LESS_EQUAL",
	"OP_GREATER",
	"OP_GREATER_EQUAL",
	"OP_AND",
	"OP_OR",
	"OP_NOT",
	"OP_ADD",
	"OP_SUB",
	"OP_MUL",
	"OP_DIV",
	"OP_MOD",
	"OP_SHIFT_LEFT",
	"OP_SHIFT_RIGHT",
	"OP_ASSIGN",
	"OP_ASSIGN_ADD",
	"OP_ASSIGN_SUB",
	"OP_ASSIGN_MUL",
	"OP_ASSIGN_DIV",
	"OP_ASSIGN_MOD",
	"OP_ASSIGN_SHIFT_LEFT",
	"OP_ASSIGN_SHIFT_RIGHT",
	"OP_ASSIGN_BIT_AND",
	"OP_ASSIGN_BIT_OR",
	"OP_ASSIGN_BIT_XOR",
	"OP_BIT_AND",
	"OP_BIT_OR",
	"OP_BIT_XOR",
	"OP_BIT_INVERT",
	"OP_INCREMENT",
	"OP_DECREMENT",
	"CF_IF",
	"CF_ELSE",
	"CF_FOR",
	"CF_WHILE",
	"CF_DO",
	"CF_SWITCH",
	"CF_CASE",
	"CF_DEFAULT",
	"CF_BREAK",
	"CF_CONTINUE",
	"CF_RETURN",
	"CF_DISCARD",
	"BRACKET_OPEN",
	"BRACKET_CLOSE",
	"CURLY_BRACKET_OPEN",
	"CURLY_BRACKET_CLOSE",
	"PARENTHESIS_OPEN",
	"PARENTHESIS_CLOSE",
	"QUESTION",
	"COMMA",
	"COLON",
	"SEMICOLON",
	"PERIOD",
	"UNIFORM",
	"UNIFORM_GROUP",
	"INSTANCE",
	"GLOBAL",
	"VARYING",
	"ARG_IN",
	"ARG_OUT",
	"ARG_INOUT",
	"RENDER_MODE",
	"HINT_DEFAULT_WHITE_TEXTURE",
	"HINT_DEFAULT_BLACK_TEXTURE",
	"HINT_DEFAULT_TRANSPARENT_TEXTURE",
	"HINT_NORMAL_TEXTURE",
	"HINT_ROUGHNESS_NORMAL_TEXTURE",
	"HINT_ROUGHNESS_R",
	"HINT_ROUGHNESS_G",
	"HINT_ROUGHNESS_B",
	"HINT_ROUGHNESS_A",
	"HINT_ROUGHNESS_GRAY",
	"HINT_ANISOTROPY_TEXTURE",
	"HINT_SOURCE_COLOR",
	"HINT_COLOR_CONVERSION_DISABLED",
	"HINT_RANGE",
	"HINT_ENUM",
	"HINT_INSTANCE_INDEX",
	"HINT_SCREEN_TEXTURE",
	"HINT_NORMAL_ROUGHNESS_TEXTURE",
	"HINT_DEPTH_TEXTURE",
	"HINT_BLIT_SOURCE0",
	"HINT_BLIT_SOURCE1",
	"HINT_BLIT_SOURCE2",
	"HINT_BLIT_SOURCE3",
	"FILTER_NEAREST",
	"FILTER_LINEAR",
	"FILTER_NEAREST_MIPMAP",
	"FILTER_LINEAR_MIPMAP",
	"FILTER_NEAREST_MIPMAP_ANISOTROPIC",
	"FILTER_LINEAR_MIPMAP_ANISOTROPIC",
	"REPEAT_ENABLE",
	"REPEAT_DISABLE",
	"SHADER_TYPE",
	"CURSOR",
	"ERROR",
	"EOF",
};

String ShaderLanguage::get_token_text(Token p_token)
{
	String name = token_names[p_token.type];
	if (p_token.is_integer_constant() || p_token.type == TK_FLOAT_CONSTANT) {
		name += "(" + rtos(p_token.constant) + ")";
	}
	else if (p_token.type == TK_IDENTIFIER) {
		name += "(" + String(p_token.text) + ")";
	}
	else if (p_token.type == TK_ERROR) {
		name += "(" + String(p_token.text) + ")";
	}

	return name;
}

ShaderLanguage::Token ShaderLanguage::_make_token(TokenType p_type, const String& p_text)
{
	Token tk;
	tk.type = p_type;
	tk.text = p_text;
	tk.line = tk_line;
	if (tk.type == TK_ERROR) {
		_set_error(p_text);
	}
	return tk;
}

enum ContextFlag : uint32_t
{
	CF_UNSPECIFIED = 0U,
	CF_BLOCK = 1U,						// "void test() { <x> }"
	CF_FUNC_DECL_PARAM_SPEC = 2U,		// "void test(<x> int param) {}"
	CF_FUNC_DECL_PARAM_TYPE = 4U,		// "void test(<x> param) {}"
	CF_IF_DECL = 8U,					// "if(<x>) {}"
	CF_BOOLEAN = 16U,					// "bool t = <x>;"
	CF_GLOBAL_SPACE = 32U,				// "struct", "const", "void" etc.
	CF_DATATYPE = 64U,					// "<x> value;"
	CF_UNIFORM_TYPE = 128U,				// "uniform <x> myUniform;"
	CF_VARYING_TYPE = 256U,				// "varying <x> myVarying;"
	CF_PRECISION_MODIFIER = 512U,		// "<x> vec4 a = vec4(0.0, 1.0, 2.0, 3.0);"
	CF_INTERPOLATION_QUALIFIER = 1024U, // "varying <x> vec3 myColor;"
	CF_UNIFORM_KEYWORD = 2048U,			// "uniform"
	CF_CONST_KEYWORD = 4096U,			// "const"
	CF_UNIFORM_QUALIFIER = 8192U,		// "<x> uniform float t;"
	CF_SHADER_TYPE = 16384U,			// "shader_type"
};

const uint32_t KCF_DATATYPE =
	CF_BLOCK | CF_GLOBAL_SPACE | CF_DATATYPE | CF_FUNC_DECL_PARAM_TYPE | CF_UNIFORM_TYPE;
const uint32_t KCF_SAMPLER_DATATYPE = CF_FUNC_DECL_PARAM_TYPE | CF_UNIFORM_TYPE;

const ShaderLanguage::KeyWord ShaderLanguage::keyword_list[] = {
	{TK_TRUE, "true", CF_BLOCK | CF_IF_DECL | CF_BOOLEAN, {}, {}},
	{TK_FALSE, "false", CF_BLOCK | CF_IF_DECL | CF_BOOLEAN, {}, {}},

	// data types

	{TK_TYPE_VOID, "void", CF_GLOBAL_SPACE, {}, {}}, {TK_TYPE_BOOL, "bool", KCF_DATATYPE, {}, {}},
	{TK_TYPE_BVEC2, "bvec2", KCF_DATATYPE, {}, {}}, {TK_TYPE_BVEC3, "bvec3", KCF_DATATYPE, {}, {}},
	{TK_TYPE_BVEC4, "bvec4", KCF_DATATYPE, {}, {}}, {TK_TYPE_INT, "int", KCF_DATATYPE, {}, {}},
	{TK_TYPE_IVEC2, "ivec2", KCF_DATATYPE, {}, {}}, {TK_TYPE_IVEC3, "ivec3", KCF_DATATYPE, {}, {}},
	{TK_TYPE_IVEC4, "ivec4", KCF_DATATYPE, {}, {}}, {TK_TYPE_UINT, "uint", KCF_DATATYPE, {}, {}},
	{TK_TYPE_UVEC2, "uvec2", KCF_DATATYPE, {}, {}}, {TK_TYPE_UVEC3, "uvec3", KCF_DATATYPE, {}, {}},
	{TK_TYPE_UVEC4, "uvec4", KCF_DATATYPE, {}, {}},
	{TK_TYPE_FLOAT, "float", KCF_DATATYPE | CF_VARYING_TYPE, {}, {}},
	{TK_TYPE_VEC2, "vec2", KCF_DATATYPE | CF_VARYING_TYPE, {}, {}},
	{TK_TYPE_VEC3, "vec3", KCF_DATATYPE | CF_VARYING_TYPE, {}, {}},
	{TK_TYPE_VEC4, "vec4", KCF_DATATYPE | CF_VARYING_TYPE, {}, {}},
	{TK_TYPE_MAT2, "mat2", KCF_DATATYPE | CF_VARYING_TYPE, {}, {}},
	{TK_TYPE_MAT3, "mat3", KCF_DATATYPE | CF_VARYING_TYPE, {}, {}},
	{TK_TYPE_MAT4, "mat4", KCF_DATATYPE | CF_VARYING_TYPE, {}, {}},
	{TK_TYPE_SAMPLER2D, "sampler2D", KCF_SAMPLER_DATATYPE, {}, {}},
	{TK_TYPE_ISAMPLER2D, "isampler2D", KCF_SAMPLER_DATATYPE, {}, {}},
	{TK_TYPE_USAMPLER2D, "usampler2D", KCF_SAMPLER_DATATYPE, {}, {}},
	{TK_TYPE_SAMPLER2DARRAY, "sampler2DArray", KCF_SAMPLER_DATATYPE, {}, {}},
	{TK_TYPE_ISAMPLER2DARRAY, "isampler2DArray", KCF_SAMPLER_DATATYPE, {}, {}},
	{TK_TYPE_USAMPLER2DARRAY, "usampler2DArray", KCF_SAMPLER_DATATYPE, {}, {}},
	{TK_TYPE_SAMPLER3D, "sampler3D", KCF_SAMPLER_DATATYPE, {}, {}},
	{TK_TYPE_ISAMPLER3D, "isampler3D", KCF_SAMPLER_DATATYPE, {}, {}},
	{TK_TYPE_USAMPLER3D, "usampler3D", KCF_SAMPLER_DATATYPE, {}, {}},
	{TK_TYPE_SAMPLERCUBE, "samplerCube", KCF_SAMPLER_DATATYPE, {}, {}},
	{TK_TYPE_SAMPLERCUBEARRAY, "samplerCubeArray", KCF_SAMPLER_DATATYPE, {}, {}},
	{TK_TYPE_SAMPLEREXT, "samplerExternalOES", KCF_SAMPLER_DATATYPE, {}, {}},

	// interpolation qualifiers

	{TK_INTERPOLATION_FLAT, "flat", CF_INTERPOLATION_QUALIFIER, {}, {}},
	{TK_INTERPOLATION_SMOOTH, "smooth", CF_INTERPOLATION_QUALIFIER, {}, {}},

	// precision modifiers

	{TK_PRECISION_LOW, "lowp", CF_BLOCK | CF_PRECISION_MODIFIER, {}, {}},
	{TK_PRECISION_MID, "mediump", CF_BLOCK | CF_PRECISION_MODIFIER, {}, {}},
	{TK_PRECISION_HIGH, "highp", CF_BLOCK | CF_PRECISION_MODIFIER, {}, {}},

	// global space keywords

	{TK_UNIFORM, "uniform", CF_GLOBAL_SPACE | CF_UNIFORM_KEYWORD, {}, {}},
	{TK_UNIFORM_GROUP, "group_uniforms", CF_GLOBAL_SPACE, {}, {}},
	{TK_VARYING, "varying", CF_GLOBAL_SPACE, {"particles", "sky", "fog"}, {}},
	{TK_CONST, "const", CF_BLOCK | CF_GLOBAL_SPACE | CF_CONST_KEYWORD, {}, {}},
	{TK_STRUCT, "struct", CF_GLOBAL_SPACE, {}, {}},
	{TK_SHADER_TYPE, "shader_type", CF_SHADER_TYPE, {}, {}},
	{TK_RENDER_MODE, "render_mode", CF_GLOBAL_SPACE, {}, {}},
	{TK_STENCIL_MODE, "stencil_mode", CF_GLOBAL_SPACE, {}, {}},

	// uniform qualifiers

	{TK_INSTANCE, "instance", CF_GLOBAL_SPACE | CF_UNIFORM_QUALIFIER, {}, {}},
	{TK_GLOBAL, "global", CF_GLOBAL_SPACE | CF_UNIFORM_QUALIFIER, {}, {}},

	// block keywords

	{TK_CF_IF, "if", CF_BLOCK, {}, {}}, {TK_CF_ELSE,
	"else", CF_BLOCK, {}, {}},
	{TK_CF_FOR, "for", CF_BLOCK, {}, {}}, {TK_CF_WHILE, "while", CF_BLOCK, {}, {}},
	{TK_CF_DO, "do", CF_BLOCK, {}, {}}, {TK_CF_SWITCH, "switch", CF_BLOCK, {}, {}},
	{TK_CF_CASE, "case", CF_BLOCK, {}, {}}, {TK_CF_DEFAULT, "default", CF_BLOCK, {}, {}},
	{TK_CF_BREAK, "break", CF_BLOCK, {}, {}}, {TK_CF_CONTINUE, "continue", CF_BLOCK, {}, {}},
	{TK_CF_RETURN, "return", CF_BLOCK, {}, {}},
	{TK_CF_DISCARD, "discard", CF_BLOCK, {"particles", "sky", "fog"}, {"vertex"}},

	// function specifier keywords

	{TK_ARG_IN, "in", CF_FUNC_DECL_PARAM_SPEC, {}, {}},
	{TK_ARG_OUT, "out", CF_FUNC_DECL_PARAM_SPEC, {}, {}},
	{TK_ARG_INOUT, "inout", CF_FUNC_DECL_PARAM_SPEC, {}, {}},

	// hints

	{TK_HINT_SOURCE_COLOR, "source_color", CF_UNSPECIFIED, {}, {}},
	{TK_HINT_COLOR_CONVERSION_DISABLED, "color_conversion_disabled", CF_UNSPECIFIED, {}, {}},
	{TK_HINT_RANGE, "hint_range", CF_UNSPECIFIED, {}, {}},
	{TK_HINT_ENUM, "hint_enum", CF_UNSPECIFIED, {}, {}},
	{TK_HINT_INSTANCE_INDEX, "instance_index", CF_UNSPECIFIED, {}, {}},

	// sampler hints

	{TK_HINT_NORMAL_TEXTURE, "hint_normal", CF_UNSPECIFIED, {}, {}},
	{TK_HINT_DEFAULT_WHITE_TEXTURE, "hint_default_white", CF_UNSPECIFIED, {}, {}},
	{TK_HINT_DEFAULT_BLACK_TEXTURE, "hint_default_black", CF_UNSPECIFIED, {}, {}},
	{TK_HINT_DEFAULT_TRANSPARENT_TEXTURE, "hint_default_transparent", CF_UNSPECIFIED, {}, {}},
	{TK_HINT_ANISOTROPY_TEXTURE, "hint_anisotropy", CF_UNSPECIFIED, {}, {}},
	{TK_HINT_ROUGHNESS_R, "hint_roughness_r", CF_UNSPECIFIED, {}, {}},
	{TK_HINT_ROUGHNESS_G, "hint_roughness_g", CF_UNSPECIFIED, {}, {}},
	{TK_HINT_ROUGHNESS_B, "hint_roughness_b", CF_UNSPECIFIED, {}, {}},
	{TK_HINT_ROUGHNESS_A, "hint_roughness_a", CF_UNSPECIFIED, {}, {}},
	{TK_HINT_ROUGHNESS_NORMAL_TEXTURE, "hint_roughness_normal", CF_UNSPECIFIED, {}, {}},
	{TK_HINT_ROUGHNESS_GRAY, "hint_roughness_gray", CF_UNSPECIFIED, {}, {}},
	{TK_HINT_SCREEN_TEXTURE, "hint_screen_texture", CF_UNSPECIFIED, {}, {}},
	{TK_HINT_NORMAL_ROUGHNESS_TEXTURE, "hint_normal_roughness_texture", CF_UNSPECIFIED, {}, {}},
	{TK_HINT_DEPTH_TEXTURE, "hint_depth_texture", CF_UNSPECIFIED, {}, {}},

	{TK_HINT_BLIT_SOURCE0, "hint_blit_source0", CF_UNSPECIFIED, {}, {}},
	{TK_HINT_BLIT_SOURCE1, "hint_blit_source1", CF_UNSPECIFIED, {}, {}},
	{TK_HINT_BLIT_SOURCE2, "hint_blit_source2", CF_UNSPECIFIED, {}, {}},
	{TK_HINT_BLIT_SOURCE3, "hint_blit_source3", CF_UNSPECIFIED, {}, {}},

	{TK_FILTER_NEAREST, "filter_nearest", CF_UNSPECIFIED, {}, {}},
	{TK_FILTER_LINEAR, "filter_linear", CF_UNSPECIFIED, {}, {}},
	{TK_FILTER_NEAREST_MIPMAP, "filter_nearest_mipmap", CF_UNSPECIFIED, {}, {}},
	{TK_FILTER_LINEAR_MIPMAP, "filter_linear_mipmap", CF_UNSPECIFIED, {}, {}},
	{TK_FILTER_NEAREST_MIPMAP_ANISOTROPIC, "filter_nearest_mipmap_anisotropic", CF_UNSPECIFIED, {},
		{}},
	{TK_FILTER_LINEAR_MIPMAP_ANISOTROPIC, "filter_linear_mipmap_anisotropic", CF_UNSPECIFIED, {},
		{}},
	{TK_REPEAT_ENABLE, "repeat_enable", CF_UNSPECIFIED, {}, {}},
	{TK_REPEAT_DISABLE, "repeat_disable", CF_UNSPECIFIED, {}, {}},

	{TK_ERROR, nullptr, CF_UNSPECIFIED, {}, {}}};

ShaderLanguage::Token ShaderLanguage::_get_token()
{
#define GETCHAR(m_idx) (((char_idx + m_idx) < code.length()) ? code[char_idx + m_idx] : char32_t(0))

	while (true) {
		char_idx++;
		switch (GETCHAR(-1)) {
		case 0:
			return _make_token(TK_EOF);
		case 0xFFFF:
			return _make_token(TK_CURSOR); // for completion
		case '\t':
		case '\r':
		case ' ':
			continue;
		case '\n':
			tk_line++;
			continue;
		case '/': {
			switch (GETCHAR(0)) {
			case '*': { // block comment

				char_idx++;
				while (true) {
					if (GETCHAR(0) == 0) {
						return _make_token(TK_EOF);
					}
					if (GETCHAR(0) == '*' && GETCHAR(1) == '/') {
						char_idx += 2;
						break;
					}
					else if (GETCHAR(0) == '\n') {
						tk_line++;
					}

					char_idx++;
				}

			} break;
			case '/': { // line comment skip

				while (true) {
					if (GETCHAR(0) == '\n') {
						tk_line++;
						char_idx++;
						break;
					}
					if (GETCHAR(0) == 0) {
						return _make_token(TK_EOF);
					}
					char_idx++;
				}

			} break;
			case '=': { // diveq

				char_idx++;
				return _make_token(TK_OP_ASSIGN_DIV);

			} break;
			default:
				return _make_token(TK_OP_DIV);
			}

			continue; // a comment, continue to next token
		} break;
		case '=': {
			if (GETCHAR(0) == '=') {
				char_idx++;
				return _make_token(TK_OP_EQUAL);
			}

			return _make_token(TK_OP_ASSIGN);

		} break;
		case '<': {
			if (GETCHAR(0) == '=') {
				char_idx++;
				return _make_token(TK_OP_LESS_EQUAL);
			}
			else if (GETCHAR(0) == '<') {
				char_idx++;
				if (GETCHAR(0) == '=') {
					char_idx++;
					return _make_token(TK_OP_ASSIGN_SHIFT_LEFT);
				}

				return _make_token(TK_OP_SHIFT_LEFT);
			}

			return _make_token(TK_OP_LESS);

		} break;
		case '>': {
			if (GETCHAR(0) == '=') {
				char_idx++;
				return _make_token(TK_OP_GREATER_EQUAL);
			}
			else if (GETCHAR(0) == '>') {
				char_idx++;
				if (GETCHAR(0) == '=') {
					char_idx++;
					return _make_token(TK_OP_ASSIGN_SHIFT_RIGHT);
				}

				return _make_token(TK_OP_SHIFT_RIGHT);
			}

			return _make_token(TK_OP_GREATER);

		} break;
		case '!': {
			if (GETCHAR(0) == '=') {
				char_idx++;
				return _make_token(TK_OP_NOT_EQUAL);
			}

			return _make_token(TK_OP_NOT);

		} break;
		case '"': {
			String _content = "";
			bool _previous_backslash = false;

			while (true) {
				bool _ended = false;
				char32_t c = GETCHAR(0);
				if (c == 0) {
					return _make_token(TK_ERROR, "EOF reached before string termination.");
				}
				switch (c) {
				case '"': {
					if (_previous_backslash) {
						_content += '"';
						_previous_backslash = false;
					}
					else {
						_ended = true;
					}
					break;
				}
				case '\\': {
					if (_previous_backslash) {
						_content += '\\';
					}
					_previous_backslash = !_previous_backslash;
					break;
				}
				case '\n': {
					return _make_token(TK_ERROR, "Unexpected end of string.");
				}
				default: {
					if (!_previous_backslash) {
						_content += c;
					}
					else {
						return _make_token(
							TK_ERROR, "Only \\\" and \\\\ escape characters supported.");
					}
					break;
				}
				}

				char_idx++;
				if (_ended) {
					break;
				}
			}

			return _make_token(TK_STRING_CONSTANT, _content);
		} break;
		// case '\'' //string - no strings in shader
		case '{':
			return _make_token(TK_CURLY_BRACKET_OPEN);
		case '}':
			return _make_token(TK_CURLY_BRACKET_CLOSE);
		case '[':
			return _make_token(TK_BRACKET_OPEN);
		case ']':
			return _make_token(TK_BRACKET_CLOSE);
		case '(':
			return _make_token(TK_PARENTHESIS_OPEN);
		case ')':
			return _make_token(TK_PARENTHESIS_CLOSE);
		case ',':
			return _make_token(TK_COMMA);
		case ';':
			return _make_token(TK_SEMICOLON);
		case '?':
			return _make_token(TK_QUESTION);
		case ':':
			return _make_token(TK_COLON);
		case '^':
			if (GETCHAR(0) == '=') {
				char_idx++;
				return _make_token(TK_OP_ASSIGN_BIT_XOR);
			}
			return _make_token(TK_OP_BIT_XOR);
		case '~':
			return _make_token(TK_OP_BIT_INVERT);
		case '&': {
			if (GETCHAR(0) == '=') {
				char_idx++;
				return _make_token(TK_OP_ASSIGN_BIT_AND);
			}
			else if (GETCHAR(0) == '&') {
				char_idx++;
				return _make_token(TK_OP_AND);
			}
			return _make_token(TK_OP_BIT_AND);
		} break;
		case '|': {
			if (GETCHAR(0) == '=') {
				char_idx++;
				return _make_token(TK_OP_ASSIGN_BIT_OR);
			}
			else if (GETCHAR(0) == '|') {
				char_idx++;
				return _make_token(TK_OP_OR);
			}
			return _make_token(TK_OP_BIT_OR);

		} break;
		case '*': {
			if (GETCHAR(0) == '=') {
				char_idx++;
				return _make_token(TK_OP_ASSIGN_MUL);
			}
			return _make_token(TK_OP_MUL);
		} break;
		case '+': {
			if (GETCHAR(0) == '=') {
				char_idx++;
				return _make_token(TK_OP_ASSIGN_ADD);
			}
			else if (GETCHAR(0) == '+') {
				char_idx++;
				return _make_token(TK_OP_INCREMENT);
			}

			return _make_token(TK_OP_ADD);
		} break;
		case '-': {
			if (GETCHAR(0) == '=') {
				char_idx++;
				return _make_token(TK_OP_ASSIGN_SUB);
			}
			else if (GETCHAR(0) == '-') {
				char_idx++;
				return _make_token(TK_OP_DECREMENT);
			}

			return _make_token(TK_OP_SUB);
		} break;
		case '%': {
			if (GETCHAR(0) == '=') {
				char_idx++;
				return _make_token(TK_OP_ASSIGN_MOD);
			}

			return _make_token(TK_OP_MOD);
		} break;
		case '@': {
			if (GETCHAR(0) == '@' && GETCHAR(1) == '>') {
				char_idx += 2;

				LocalVector<char32_t> incp;
				while (GETCHAR(0) != '\n') {
					incp.push_back(GETCHAR(0));
					char_idx++;
				}
				incp.push_back(0); // Zero end it.
				String include_path(incp.ptr());
				include_positions.write[include_positions.size() - 1].line = tk_line;

				String marker = ">>" + include_path;
				if (!include_markers_handled.has(marker)) {
					include_markers_handled.insert(marker);

					FilePosition fp;
					fp.file = include_path;
					fp.line = 0;
					tk_line = 0;
					include_positions.push_back(fp);
				}

			}
			else if (GETCHAR(0) == '@' && GETCHAR(1) == '<') {
				char_idx += 2;

				LocalVector<char32_t> incp;
				while (GETCHAR(0) != '\n') {
					incp.push_back(GETCHAR(0));
					char_idx++;
				}
				incp.push_back(0); // Zero end it.
				String include_path(incp.ptr());

				String marker = "<<" + include_path;
				if (!include_markers_handled.has(marker)) {
					include_markers_handled.insert(marker);

					if (include_positions.size() == 1) {
						return _make_token(
							TK_ERROR, "Invalid include exit hint @@< without matching enter hint.");
					}
					include_positions.resize(include_positions.size() - 1); // Pop back.
				}

				tk_line = include_positions[include_positions.size() - 1].line - 1; // Restore line.

			}
			else {
				return _make_token(TK_ERROR, "Invalid include enter/exit hint token (@@> and @@<)");
			}
		} break;
		default: {
			char_idx--; // go back one, since we have no idea what this is

			if (is_digit(GETCHAR(0)) || (GETCHAR(0) == '.' && is_digit(GETCHAR(1)))) {
				// parse number
				bool hexa_found = false;
				bool period_found = false;
				bool exponent_found = false;
				bool float_suffix_found = false;
				bool uint_suffix_found = false;
				bool end_suffix_found = false;

				enum
				{
					CASE_ALL,
					CASE_HEXA_PERIOD,
					CASE_EXPONENT,
					CASE_SIGN_AFTER_EXPONENT,
					CASE_NONE,
					CASE_MAX,
				} lut_case = CASE_ALL;

				static bool suffix_lut[CASE_MAX][127];

				if (!is_const_suffix_lut_initialized) {
					is_const_suffix_lut_initialized = true;

					for (int i = 0; i < 127; i++) {
						char t = char(i);

						suffix_lut[CASE_ALL][i] = t == '.' || t == 'x' || t == 'e' || t == 'f' ||
												  t == 'u' || t == '-' || t == '+';
						suffix_lut[CASE_HEXA_PERIOD][i] = t == 'e' || t == 'f' || t == 'u';
						suffix_lut[CASE_EXPONENT][i] = t == 'f' || t == '-' || t == '+';
						suffix_lut[CASE_SIGN_AFTER_EXPONENT][i] = t == 'f';
						suffix_lut[CASE_NONE][i] = false;
					}
				}

				String str;
				int i = 0;
				bool digit_after_exp = false;

				while (true) {
					const char32_t symbol = String::char_lowercase(GETCHAR(i));
					bool error = false;

					if (is_digit(symbol)) {
						if (exponent_found) {
							digit_after_exp = true;
						}
						if (end_suffix_found) {
							error = true;
						}
					}
					else {
						if (symbol < 0x7F && suffix_lut[lut_case][symbol]) {
							if (symbol == 'x') {
								hexa_found = true;
								lut_case = CASE_HEXA_PERIOD;
							}
							else if (symbol == '.') {
								period_found = true;
								lut_case = CASE_HEXA_PERIOD;
							}
							else if (symbol == 'e' && !hexa_found) {
								exponent_found = true;
								lut_case = CASE_EXPONENT;
							}
							else if (symbol == 'f' && !hexa_found) {
								if (!period_found && !exponent_found) {
									error = true;
								}
								float_suffix_found = true;
								end_suffix_found = true;
								lut_case = CASE_NONE;
							}
							else if (symbol == 'u') {
								uint_suffix_found = true;
								end_suffix_found = true;
								lut_case = CASE_NONE;
							}
							else if (symbol == '-' || symbol == '+') {
								if (exponent_found) {
									lut_case = CASE_SIGN_AFTER_EXPONENT;
								}
								else {
									break;
								}
							}
						}
						else if (!hexa_found || !is_hex_digit(symbol)) {
							if (is_ascii_identifier_char(symbol)) {
								error = true;
							}
							else {
								break;
							}
						}
					}

					if (error) {
						if (hexa_found) {
							return _make_token(TK_ERROR, "Invalid (hexadecimal) numeric constant");
						}
						if (period_found || exponent_found || float_suffix_found) {
							return _make_token(TK_ERROR, "Invalid (float) numeric constant");
						}
						if (uint_suffix_found) {
							return _make_token(
								TK_ERROR, "Invalid (unsigned integer) numeric constant");
						}
						return _make_token(TK_ERROR, "Invalid (integer) numeric constant");
					}
					str += symbol;
					i++;
				}

				char32_t last_char = str[str.length() - 1];

				if (hexa_found) { // Integer (hex).
					if (uint_suffix_found) {
						// Strip the suffix.
						str = str.left(str.length() - 1);

						// Compensate reading cursor position.
						char_idx += 1;
					}
					if (str.size() > 11 || !str.is_valid_hex_number(true)) { // > 0xFFFFFFFF
						return _make_token(TK_ERROR, "Invalid (hexadecimal) numeric constant");
					}
				}
				else if (period_found || exponent_found || float_suffix_found) { // Float
					if (exponent_found &&
						(!digit_after_exp ||
							(!is_digit(last_char) &&
								last_char !=
									'f'))) { // Checks for eg: "2E", "2E-", "2E+" and 0ef, 0e+f,
											 // 0.0ef, 0.0e-f (exponent without digit after it).
						return _make_token(TK_ERROR, "Invalid (float) numeric constant");
					}
					if (period_found) {
						if (float_suffix_found) {
							// checks for eg "1.f" or "1.99f" notations
							if (last_char != 'f') {
								return _make_token(TK_ERROR, "Invalid (float) numeric constant");
							}
						}
						else {
							// checks for eg. "1." or "1.99" notations
							if (last_char != '.' && !is_digit(last_char)) {
								return _make_token(TK_ERROR, "Invalid (float) numeric constant");
							}
						}
					}
					else if (float_suffix_found) {
						// if no period found the float suffix must be the last character, like in
						// "2f" for "2.0"
						if (last_char != 'f') {
							return _make_token(TK_ERROR, "Invalid (float) numeric constant");
						}
					}

					if (float_suffix_found) {
						// Strip the suffix.
						str = str.left(str.length() - 1);
						// Compensate reading cursor position.
						char_idx += 1;
					}

					if (!str.is_valid_float()) {
						return _make_token(TK_ERROR, "Invalid (float) numeric constant");
					}
				}
				else { // Integer
					if (uint_suffix_found) {
						// Strip the suffix.
						str = str.left(str.length() - 1);
						// Compensate reading cursor position.
						char_idx += 1;
					}
					if (!str.is_valid_int()) {
						if (uint_suffix_found) {
							return _make_token(
								TK_ERROR, "Invalid (unsigned integer) numeric constant");
						}
						else {
							return _make_token(TK_ERROR, "Invalid (integer) numeric constant");
						}
					}
				}

				char_idx += str.length();
				Token tk;
				if (period_found || exponent_found || float_suffix_found) {
					tk.type = TK_FLOAT_CONSTANT;
				}
				else if (uint_suffix_found) {
					tk.type = TK_UINT_CONSTANT;
				}
				else {
					tk.type = TK_INT_CONSTANT;
				}

				if (hexa_found) {
					tk.constant = (double)str.hex_to_int();
				}
				else {
					tk.constant = str.to_float();
				}
				tk.line = tk_line;

				return tk;
			}

			if (GETCHAR(0) == '.') {
				// parse period
				char_idx++;
				return _make_token(TK_PERIOD);
			}

			if (is_ascii_identifier_char(GETCHAR(0))) {
				// parse identifier
				String str;

				while (is_ascii_identifier_char(GETCHAR(0))) {
					str += char32_t(GETCHAR(0));
					char_idx++;
				}

				// see if keyword
				// should be converted to a static map
				int idx = 0;

				while (keyword_list[idx].text) {
					if (str == keyword_list[idx].text) {
						return _make_token(keyword_list[idx].token);
					}
					idx++;
				}

				str = str.replace("dus_", "_");

				return _make_token(TK_IDENTIFIER, str);
			}

			if (GETCHAR(0) > 32) {
				return _make_token(TK_ERROR, "Tokenizer: Unknown character #" + itos(GETCHAR(0)) +
												 ": '" + String::chr(GETCHAR(0)) + "'");
			}
			else {
				return _make_token(TK_ERROR, "Tokenizer: Unknown character #" + itos(GETCHAR(0)));
			}

		} break;
		}
	}
	ERR_PRINT("BUG");
	return Token();

#undef GETCHAR
}

bool ShaderLanguage::_lookup_next(Token& r_tk)
{
	TkPos pre_pos = _get_tkpos();
	int line = pre_pos.tk_line;
	_get_token();
	Token tk = _get_token();
	_set_tkpos(pre_pos);
	if (tk.line == line) {
		r_tk = tk;
		return true;
	}
	return false;
}

ShaderLanguage::Token ShaderLanguage::_peek()
{
	TkPos pre_pos = _get_tkpos();
	Token tk = _get_token();
	_set_tkpos(pre_pos);
	return tk;
}

String ShaderLanguage::token_debug(const String& p_code)
{
	clear();

	code = p_code;

	String output;

	Token tk = _get_token();
	while (tk.type != TK_EOF && tk.type != TK_ERROR) {
		output += itos(tk_line) + ": " + get_token_text(tk) + "\n";
		tk = _get_token();
	}

	return output;
}

bool ShaderLanguage::is_token_variable_datatype(TokenType p_type)
{
	return (p_type == TK_TYPE_VOID || p_type == TK_TYPE_BOOL || p_type == TK_TYPE_BVEC2 ||
			p_type == TK_TYPE_BVEC3 || p_type == TK_TYPE_BVEC4 || p_type == TK_TYPE_INT ||
			p_type == TK_TYPE_IVEC2 || p_type == TK_TYPE_IVEC3 || p_type == TK_TYPE_IVEC4 ||
			p_type == TK_TYPE_UINT || p_type == TK_TYPE_UVEC2 || p_type == TK_TYPE_UVEC3 ||
			p_type == TK_TYPE_UVEC4 || p_type == TK_TYPE_FLOAT || p_type == TK_TYPE_VEC2 ||
			p_type == TK_TYPE_VEC3 || p_type == TK_TYPE_VEC4 || p_type == TK_TYPE_MAT2 ||
			p_type == TK_TYPE_MAT3 || p_type == TK_TYPE_MAT4);
}

bool ShaderLanguage::is_token_datatype(TokenType p_type)
{
	return (p_type == TK_TYPE_VOID || p_type == TK_TYPE_BOOL || p_type == TK_TYPE_BVEC2 ||
			p_type == TK_TYPE_BVEC3 || p_type == TK_TYPE_BVEC4 || p_type == TK_TYPE_INT ||
			p_type == TK_TYPE_IVEC2 || p_type == TK_TYPE_IVEC3 || p_type == TK_TYPE_IVEC4 ||
			p_type == TK_TYPE_UINT || p_type == TK_TYPE_UVEC2 || p_type == TK_TYPE_UVEC3 ||
			p_type == TK_TYPE_UVEC4 || p_type == TK_TYPE_FLOAT || p_type == TK_TYPE_VEC2 ||
			p_type == TK_TYPE_VEC3 || p_type == TK_TYPE_VEC4 || p_type == TK_TYPE_MAT2 ||
			p_type == TK_TYPE_MAT3 || p_type == TK_TYPE_MAT4 || p_type == TK_TYPE_SAMPLER2D ||
			p_type == TK_TYPE_ISAMPLER2D || p_type == TK_TYPE_USAMPLER2D ||
			p_type == TK_TYPE_SAMPLER2DARRAY || p_type == TK_TYPE_ISAMPLER2DARRAY ||
			p_type == TK_TYPE_USAMPLER2DARRAY || p_type == TK_TYPE_SAMPLER3D ||
			p_type == TK_TYPE_ISAMPLER3D || p_type == TK_TYPE_USAMPLER3D ||
			p_type == TK_TYPE_SAMPLERCUBE || p_type == TK_TYPE_SAMPLERCUBEARRAY ||
			p_type == TK_TYPE_SAMPLEREXT);
}

ShaderLanguage::DataType ShaderLanguage::get_token_datatype(TokenType p_type)
{
	return DataType(p_type - TK_TYPE_VOID);
}

bool ShaderLanguage::is_token_interpolation(TokenType p_type)
{
	return (p_type == TK_INTERPOLATION_FLAT || p_type == TK_INTERPOLATION_SMOOTH);
}

ShaderLanguage::DataInterpolation ShaderLanguage::get_token_interpolation(TokenType p_type)
{
	if (p_type == TK_INTERPOLATION_FLAT) {
		return INTERPOLATION_FLAT;
	}
	else {
		return INTERPOLATION_SMOOTH;
	}
}

bool ShaderLanguage::is_token_precision(TokenType p_type)
{
	return (
		p_type == TK_PRECISION_LOW || p_type == TK_PRECISION_MID || p_type == TK_PRECISION_HIGH);
}

bool ShaderLanguage::is_token_arg_qual(TokenType p_type)
{
	return (p_type == TK_ARG_IN || p_type == TK_ARG_OUT || p_type == TK_ARG_INOUT);
}

ShaderLanguage::DataPrecision ShaderLanguage::get_token_precision(TokenType p_type)
{
	if (p_type == TK_PRECISION_LOW) {
		return PRECISION_LOWP;
	}
	else if (p_type == TK_PRECISION_HIGH) {
		return PRECISION_HIGHP;
	}
	else {
		return PRECISION_MEDIUMP;
	}
}

String ShaderLanguage::get_precision_name(DataPrecision p_type)
{
	switch (p_type) {
	case PRECISION_LOWP:
		return "lowp";
	case PRECISION_MEDIUMP:
		return "mediump";
	case PRECISION_HIGHP:
		return "highp";
	default:
		break;
	}
	return "";
}

String ShaderLanguage::get_interpolation_name(DataInterpolation p_interpolation)
{
	switch (p_interpolation) {
	case INTERPOLATION_FLAT:
		return "flat";
	case INTERPOLATION_SMOOTH:
		return "smooth";
	default:
		break;
	}
	return "";
}

String ShaderLanguage::get_datatype_name(DataType p_type)
{
	switch (p_type) {
	case TYPE_VOID:
		return "void";
	case TYPE_BOOL:
		return "bool";
	case TYPE_BVEC2:
		return "bvec2";
	case TYPE_BVEC3:
		return "bvec3";
	case TYPE_BVEC4:
		return "bvec4";
	case TYPE_INT:
		return "int";
	case TYPE_IVEC2:
		return "ivec2";
	case TYPE_IVEC3:
		return "ivec3";
	case TYPE_IVEC4:
		return "ivec4";
	case TYPE_UINT:
		return "uint";
	case TYPE_UVEC2:
		return "uvec2";
	case TYPE_UVEC3:
		return "uvec3";
	case TYPE_UVEC4:
		return "uvec4";
	case TYPE_FLOAT:
		return "float";
	case TYPE_VEC2:
		return "vec2";
	case TYPE_VEC3:
		return "vec3";
	case TYPE_VEC4:
		return "vec4";
	case TYPE_MAT2:
		return "mat2";
	case TYPE_MAT3:
		return "mat3";
	case TYPE_MAT4:
		return "mat4";
	case TYPE_SAMPLER2D:
		return "sampler2D";
	case TYPE_ISAMPLER2D:
		return "isampler2D";
	case TYPE_USAMPLER2D:
		return "usampler2D";
	case TYPE_SAMPLER2DARRAY:
		return "sampler2DArray";
	case TYPE_ISAMPLER2DARRAY:
		return "isampler2DArray";
	case TYPE_USAMPLER2DARRAY:
		return "usampler2DArray";
	case TYPE_SAMPLER3D:
		return "sampler3D";
	case TYPE_ISAMPLER3D:
		return "isampler3D";
	case TYPE_USAMPLER3D:
		return "usampler3D";
	case TYPE_SAMPLERCUBE:
		return "samplerCube";
	case TYPE_SAMPLERCUBEARRAY:
		return "samplerCubeArray";
	case TYPE_SAMPLEREXT:
		return "samplerExternalOES";
	case TYPE_STRUCT:
		return "struct";
	case TYPE_MAX:
		return "invalid";
	}

	return "";
}

String ShaderLanguage::get_uniform_hint_name(ShaderNode::Uniform::Hint p_hint)
{
	String result;
	switch (p_hint) {
	case ShaderNode::Uniform::HINT_RANGE: {
		result = "hint_range";
	} break;
	case ShaderNode::Uniform::HINT_ENUM: {
		result = "hint_enum";
	} break;
	case ShaderNode::Uniform::HINT_SOURCE_COLOR: {
		result = "source_color";
	} break;
	case ShaderNode::Uniform::HINT_COLOR_CONVERSION_DISABLED: {
		result = "color_conversion_disabled";
	} break;
	case ShaderNode::Uniform::HINT_NORMAL: {
		result = "hint_normal";
	} break;
	case ShaderNode::Uniform::HINT_ROUGHNESS_NORMAL: {
		result = "hint_roughness_normal";
	} break;
	case ShaderNode::Uniform::HINT_ROUGHNESS_R: {
		result = "hint_roughness_r";
	} break;
	case ShaderNode::Uniform::HINT_ROUGHNESS_G: {
		result = "hint_roughness_g";
	} break;
	case ShaderNode::Uniform::HINT_ROUGHNESS_B: {
		result = "hint_roughness_b";
	} break;
	case ShaderNode::Uniform::HINT_ROUGHNESS_A: {
		result = "hint_roughness_a";
	} break;
	case ShaderNode::Uniform::HINT_ROUGHNESS_GRAY: {
		result = "hint_roughness_gray";
	} break;
	case ShaderNode::Uniform::HINT_DEFAULT_BLACK: {
		result = "hint_default_black";
	} break;
	case ShaderNode::Uniform::HINT_DEFAULT_WHITE: {
		result = "hint_default_white";
	} break;
	case ShaderNode::Uniform::HINT_DEFAULT_TRANSPARENT: {
		result = "hint_default_transparent";
	} break;
	case ShaderNode::Uniform::HINT_ANISOTROPY: {
		result = "hint_anisotropy";
	} break;
	case ShaderNode::Uniform::HINT_SCREEN_TEXTURE: {
		result = "hint_screen_texture";
	} break;
	case ShaderNode::Uniform::HINT_NORMAL_ROUGHNESS_TEXTURE: {
		result = "hint_normal_roughness_texture";
	} break;
	case ShaderNode::Uniform::HINT_DEPTH_TEXTURE: {
		result = "hint_depth_texture";
	} break;
	case ShaderNode::Uniform::HINT_BLIT_SOURCE0: {
		result = "hint_blit_source0";
	} break;
	case ShaderNode::Uniform::HINT_BLIT_SOURCE1: {
		result = "hint_blit_source1";
	} break;
	case ShaderNode::Uniform::HINT_BLIT_SOURCE2: {
		result = "hint_blit_source2";
	} break;
	case ShaderNode::Uniform::HINT_BLIT_SOURCE3: {
		result = "hint_blit_source3";
	} break;
	default:
		break;
	}
	return result;
}

String ShaderLanguage::get_texture_filter_name(TextureFilter p_filter)
{
	String result;
	switch (p_filter) {
	case FILTER_NEAREST: {
		result = "filter_nearest";
	} break;
	case FILTER_LINEAR: {
		result = "filter_linear";
	} break;
	case FILTER_NEAREST_MIPMAP: {
		result = "filter_nearest_mipmap";
	} break;
	case FILTER_LINEAR_MIPMAP: {
		result = "filter_linear_mipmap";
	} break;
	case FILTER_NEAREST_MIPMAP_ANISOTROPIC: {
		result = "filter_nearest_mipmap_anisotropic";
	} break;
	case FILTER_LINEAR_MIPMAP_ANISOTROPIC: {
		result = "filter_linear_mipmap_anisotropic";
	} break;
	default: {
	} break;
	}
	return result;
}

String ShaderLanguage::get_texture_repeat_name(TextureRepeat p_repeat)
{
	String result;
	switch (p_repeat) {
	case REPEAT_DISABLE: {
		result = "repeat_disable";
	} break;
	case REPEAT_ENABLE: {
		result = "repeat_enable";
	} break;
	default: {
	} break;
	}
	return result;
}

bool ShaderLanguage::is_token_nonvoid_datatype(TokenType p_type)
{
	return is_token_datatype(p_type) && p_type != TK_TYPE_VOID;
}

void ShaderLanguage::clear()
{
	current_function = String();
	last_name = String();
	last_type = IDENTIFIER_MAX;
	current_uniform_group_name = "";
	current_uniform_hint = ShaderNode::Uniform::HINT_NONE;
	current_uniform_filter = FILTER_DEFAULT;
	current_uniform_repeat = REPEAT_DEFAULT;
	current_uniform_instance_index_defined = false;

	completion_type = COMPLETION_NONE;
	completion_block = nullptr;
	completion_function = String();
	completion_class = TAG_GLOBAL;
	completion_struct = String();
	completion_base = TYPE_VOID;
	completion_base_array = false;

	include_positions.clear();
	include_positions.push_back(FilePosition());

	include_markers_handled.clear();
	calls_info.clear();
	function_overload_count.clear();

#ifdef DEBUG_ENABLED
	keyword_completion_context = CF_UNSPECIFIED;
	used_constants.clear();
	used_varyings.clear();
	used_uniforms.clear();
	used_functions.clear();
	used_structs.clear();
	used_local_vars.clear();
	warnings.clear();
#endif // DEBUG_ENABLED

	error_line = 0;
	tk_line = 1;
	char_idx = 0;
	error_set = false;
	error_str = "";
	is_const_decl = false;
	while (nodes) {
		Node* n = nodes;
		nodes = nodes->next;
		memdelete(n);
	}
}

#ifdef DEBUG_ENABLED
void ShaderLanguage::_parse_used_identifier(
	const String& p_identifier, IdentifierType p_type, const String& p_function)
{
	switch (p_type) {
	case IdentifierType::IDENTIFIER_CONSTANT:
		if (HAS_WARNING(ShaderWarning::UNUSED_CONSTANT_FLAG) && used_constants.has(p_identifier)) {
			used_constants[p_identifier].used = true;
		}
		break;
	case IdentifierType::IDENTIFIER_VARYING:
		if (HAS_WARNING(ShaderWarning::UNUSED_VARYING_FLAG) && used_varyings.has(p_identifier)) {
			if (shader->varyings[p_identifier].stage == ShaderNode::Varying::STAGE_UNKNOWN) {
				used_varyings[p_identifier].used = true;
			}
		}
		break;
	case IdentifierType::IDENTIFIER_UNIFORM:
		if (HAS_WARNING(ShaderWarning::UNUSED_UNIFORM_FLAG) && used_uniforms.has(p_identifier)) {
			used_uniforms[p_identifier].used = true;
		}
		break;
	case IdentifierType::IDENTIFIER_FUNCTION:
		if (HAS_WARNING(ShaderWarning::UNUSED_FUNCTION_FLAG) && used_functions.has(p_identifier)) {
			used_functions[p_identifier].used = true;
		}
		break;
	case IdentifierType::IDENTIFIER_LOCAL_VAR:
		if (HAS_WARNING(ShaderWarning::UNUSED_LOCAL_VARIABLE_FLAG) &&
			used_local_vars.has(p_function) && used_local_vars[p_function].has(p_identifier)) {
			used_local_vars[p_function][p_identifier].used = true;
		}
		break;
	default:
		break;
	}
}
#endif // DEBUG_ENABLED

bool ShaderLanguage::_find_identifier(const BlockNode* p_block, bool p_allow_reassign,
	const FunctionInfo& p_function_info, const String& p_identifier, DataType* r_data_type,
	IdentifierType* r_type, bool* r_is_const, int* r_array_size, String* r_struct_name,
	Vector<Scalar>* r_constant_values)
{
	if (is_shader_inc) {
		for (int i = 0; i < RSE::SHADER_MAX; i++) {
			for (const KeyValue<String, FunctionInfo>& E :
				ShaderTypes::get_functions(RSE::ShaderMode(i))) {
				if ((current_function == E.key || E.key == "global" || E.key == "constants") &&
					E.value.built_ins.has(p_identifier)) {
					if (r_data_type) {
						*r_data_type = E.value.built_ins[p_identifier].type;
					}
					if (r_is_const) {
						*r_is_const = E.value.built_ins[p_identifier].constant;
					}
					if (r_type) {
						*r_type = IDENTIFIER_BUILTIN_VAR;
					}
					return true;
				}
			}
		}
	}
	else {
		if (p_function_info.built_ins.has(p_identifier)) {
			if (r_data_type) {
				*r_data_type = p_function_info.built_ins[p_identifier].type;
			}
			if (r_is_const) {
				*r_is_const = p_function_info.built_ins[p_identifier].constant;
			}
			if (r_constant_values) {
				*r_constant_values = p_function_info.built_ins[p_identifier].values;
			}
			if (r_type) {
				*r_type = IDENTIFIER_BUILTIN_VAR;
			}
			return true;
		}
	}

	if (p_function_info.stage_functions.has(p_identifier)) {
		if (r_data_type) {
			*r_data_type = p_function_info.stage_functions[p_identifier].return_type;
		}
		if (r_is_const) {
			*r_is_const = true;
		}
		if (r_type) {
			*r_type = IDENTIFIER_FUNCTION;
		}
		return true;
	}

	FunctionNode* function = nullptr;

	while (p_block) {
		if (p_block->variables.has(p_identifier)) {
			if (r_data_type) {
				*r_data_type = p_block->variables[p_identifier].type;
			}
			if (r_is_const) {
				*r_is_const = p_block->variables[p_identifier].is_const;
			}
			if (r_array_size) {
				*r_array_size = p_block->variables[p_identifier].array_size;
			}
			if (r_struct_name) {
				*r_struct_name = p_block->variables[p_identifier].struct_name;
			}
			if (r_constant_values && !p_block->variables[p_identifier].values.is_empty()) {
				*r_constant_values = p_block->variables[p_identifier].values;
			}
			if (r_type) {
				*r_type = IDENTIFIER_LOCAL_VAR;
			}
			return true;
		}

		if (p_block->parent_function) {
			function = p_block->parent_function;
			break;
		}
		else {
			if (p_allow_reassign) {
				break;
			}
			ERR_FAIL_NULL_V(p_block->parent_block, false);
			p_block = p_block->parent_block;
		}
	}

	if (function) {
		for (int i = 0; i < function->arguments.size(); i++) {
			if (function->arguments[i].name == p_identifier) {
				if (r_data_type) {
					*r_data_type = function->arguments[i].type;
				}
				if (r_struct_name) {
					*r_struct_name = function->arguments[i].struct_name;
				}
				if (r_array_size) {
					*r_array_size = function->arguments[i].array_size;
				}
				if (r_is_const) {
					*r_is_const = function->arguments[i].is_const;
				}
				if (r_type) {
					*r_type = IDENTIFIER_FUNCTION_ARGUMENT;
				}
				return true;
			}
		}
	}

	if (shader->varyings.has(p_identifier)) {
		if (r_data_type) {
			*r_data_type = shader->varyings[p_identifier].type;
		}
		if (r_array_size) {
			*r_array_size = shader->varyings[p_identifier].array_size;
		}
		if (r_type) {
			*r_type = IDENTIFIER_VARYING;
		}
		return true;
	}

	if (shader->uniforms.has(p_identifier)) {
		if (r_data_type) {
			*r_data_type = shader->uniforms[p_identifier].type;
		}
		if (r_array_size) {
			*r_array_size = shader->uniforms[p_identifier].array_size;
		}
		if (r_type) {
			*r_type = IDENTIFIER_UNIFORM;
		}
		return true;
	}

	if (shader->constants.has(p_identifier)) {
		if (r_is_const) {
			*r_is_const = true;
		}
		if (r_data_type) {
			*r_data_type = shader->constants[p_identifier].type;
		}
		if (r_array_size) {
			*r_array_size = shader->constants[p_identifier].array_size;
		}
		if (r_struct_name) {
			*r_struct_name = shader->constants[p_identifier].struct_name;
		}
		if (r_constant_values && shader->constants[p_identifier].initializer) {
			Vector<Scalar> values = _get_node_values(
				p_block, p_function_info, shader->constants[p_identifier].initializer);
			if (!values.is_empty()) {
				*r_constant_values = values;
			}
		}
		if (r_type) {
			*r_type = IDENTIFIER_CONSTANT;
		}
		return true;
	}

	for (int i = 0; i < shader->vfunctions.size(); i++) {
		if (!shader->vfunctions[i].callable) {
			continue;
		}

		if (shader->vfunctions[i].name == p_identifier) {
			if (r_data_type) {
				*r_data_type = shader->vfunctions[i].function->return_type;
			}
			if (r_array_size) {
				*r_array_size = shader->vfunctions[i].function->return_array_size;
			}
			if (r_type) {
				*r_type = IDENTIFIER_FUNCTION;
			}
			return true;
		}
	}

	return false;
}

bool ShaderLanguage::_validate_operator(const BlockNode* p_block,
	const FunctionInfo& p_function_info, OperatorNode* p_op, DataType* r_ret_type, int* r_ret_size,
	String* r_ret_struct_name)
{
	bool valid = false;
	DataType ret_type = TYPE_VOID;
	int ret_size = 0;
	String ret_struct_name;

	switch (p_op->op) {
	case OP_EQUAL:
	case OP_NOT_EQUAL: {
		if ((!p_op->arguments[0]->is_indexed() && p_op->arguments[0]->get_array_size() > 0) ||
			(!p_op->arguments[1]->is_indexed() && p_op->arguments[1]->get_array_size() > 0)) {
			break; // don't accept arrays
		}

		DataType na = p_op->arguments[0]->get_datatype();
		DataType nb = p_op->arguments[1]->get_datatype();
		valid = na == nb;
		ret_type = TYPE_BOOL;
	} break;
	case OP_LESS:
	case OP_LESS_EQUAL:
	case OP_GREATER:
	case OP_GREATER_EQUAL: {
		if ((!p_op->arguments[0]->is_indexed() && p_op->arguments[0]->get_array_size() > 0) ||
			(!p_op->arguments[1]->is_indexed() && p_op->arguments[1]->get_array_size() > 0)) {
			break; // don't accept arrays
		}

		DataType na = p_op->arguments[0]->get_datatype();
		DataType nb = p_op->arguments[1]->get_datatype();

		valid = na == nb && (na == TYPE_UINT || na == TYPE_INT || na == TYPE_FLOAT);
		ret_type = TYPE_BOOL;

	} break;
	case OP_AND:
	case OP_OR: {
		if ((!p_op->arguments[0]->is_indexed() && p_op->arguments[0]->get_array_size() > 0) ||
			(!p_op->arguments[1]->is_indexed() && p_op->arguments[1]->get_array_size() > 0)) {
			break; // don't accept arrays
		}

		DataType na = p_op->arguments[0]->get_datatype();
		DataType nb = p_op->arguments[1]->get_datatype();

		valid = na == nb && na == TYPE_BOOL;
		ret_type = TYPE_BOOL;

	} break;
	case OP_NOT: {
		if (!p_op->arguments[0]->is_indexed() && p_op->arguments[0]->get_array_size() > 0) {
			break; // don't accept arrays
		}

		DataType na = p_op->arguments[0]->get_datatype();
		valid = na == TYPE_BOOL;
		ret_type = TYPE_BOOL;

	} break;
	case OP_INCREMENT:
	case OP_DECREMENT:
	case OP_POST_INCREMENT:
	case OP_POST_DECREMENT:
	case OP_NEGATE: {
		if (!p_op->arguments[0]->is_indexed() && p_op->arguments[0]->get_array_size() > 0) {
			break; // don't accept arrays
		}

		DataType na = p_op->arguments[0]->get_datatype();
		valid = na > TYPE_BVEC4 && na < TYPE_MAT2;
		ret_type = na;
	} break;
	case OP_ADD:
	case OP_SUB:
	case OP_MUL:
	case OP_DIV: {
		if ((!p_op->arguments[0]->is_indexed() && p_op->arguments[0]->get_array_size() > 0) ||
			(!p_op->arguments[1]->is_indexed() && p_op->arguments[1]->get_array_size() > 0)) {
			break; // don't accept arrays
		}

		DataType na = p_op->arguments[0]->get_datatype();
		DataType nb = p_op->arguments[1]->get_datatype();

		if (na > nb) {
			// make things easier;
			SWAP(na, nb);
		}

		if (na == nb) {
			valid = (na > TYPE_BVEC4 && na <= TYPE_MAT4);
			ret_type = na;
		}
		else if (na == TYPE_INT && nb == TYPE_IVEC2) {
			valid = true;
			ret_type = TYPE_IVEC2;
		}
		else if (na == TYPE_INT && nb == TYPE_IVEC3) {
			valid = true;
			ret_type = TYPE_IVEC3;
		}
		else if (na == TYPE_INT && nb == TYPE_IVEC4) {
			valid = true;
			ret_type = TYPE_IVEC4;
		}
		else if (na == TYPE_UINT && nb == TYPE_UVEC2) {
			valid = true;
			ret_type = TYPE_UVEC2;
		}
		else if (na == TYPE_UINT && nb == TYPE_UVEC3) {
			valid = true;
			ret_type = TYPE_UVEC3;
		}
		else if (na == TYPE_UINT && nb == TYPE_UVEC4) {
			valid = true;
			ret_type = TYPE_UVEC4;
		}
		else if (na == TYPE_FLOAT && nb == TYPE_VEC2) {
			valid = true;
			ret_type = TYPE_VEC2;
		}
		else if (na == TYPE_FLOAT && nb == TYPE_VEC3) {
			valid = true;
			ret_type = TYPE_VEC3;
		}
		else if (na == TYPE_FLOAT && nb == TYPE_VEC4) {
			valid = true;
			ret_type = TYPE_VEC4;
		}
		else if (na == TYPE_FLOAT && nb == TYPE_MAT2) {
			valid = true;
			ret_type = TYPE_MAT2;
		}
		else if (na == TYPE_FLOAT && nb == TYPE_MAT3) {
			valid = true;
			ret_type = TYPE_MAT3;
		}
		else if (na == TYPE_FLOAT && nb == TYPE_MAT4) {
			valid = true;
			ret_type = TYPE_MAT4;
		}
		else if (p_op->op == OP_MUL && na == TYPE_VEC2 && nb == TYPE_MAT2) {
			valid = true;
			ret_type = TYPE_VEC2;
		}
		else if (p_op->op == OP_MUL && na == TYPE_VEC3 && nb == TYPE_MAT3) {
			valid = true;
			ret_type = TYPE_VEC3;
		}
		else if (p_op->op == OP_MUL && na == TYPE_VEC4 && nb == TYPE_MAT4) {
			valid = true;
			ret_type = TYPE_VEC4;
		}
	} break;
	case OP_ASSIGN_MOD:
	case OP_MOD: {
		/*
		 * The operator modulus (%) operates on signed or unsigned integers or integer vectors. The
		 * operand types must both be signed or both be unsigned. The operands cannot be vectors of
		 * differing size. If one operand is a scalar and the other vector, then the scalar is
		 * applied component-wise to the vector, resulting in the same type as the vector. If both
		 * are vectors of the same size, the result is computed component-wise.
		 */

		if ((!p_op->arguments[0]->is_indexed() && p_op->arguments[0]->get_array_size() > 0) ||
			(!p_op->arguments[1]->is_indexed() && p_op->arguments[1]->get_array_size() > 0)) {
			break; // don't accept arrays
		}

		DataType na = p_op->arguments[0]->get_datatype();
		DataType nb = p_op->arguments[1]->get_datatype();

		if (na == TYPE_INT && nb == TYPE_INT) {
			valid = true;
			ret_type = TYPE_INT;
		}
		else if (na == TYPE_IVEC2 && nb == TYPE_INT) {
			valid = true;
			ret_type = TYPE_IVEC2;
		}
		else if (na == TYPE_IVEC3 && nb == TYPE_INT) {
			valid = true;
			ret_type = TYPE_IVEC3;
		}
		else if (na == TYPE_IVEC4 && nb == TYPE_INT) {
			valid = true;
			ret_type = TYPE_IVEC4;
		}
		else if (na == TYPE_IVEC2 && nb == TYPE_IVEC2) {
			valid = true;
			ret_type = TYPE_IVEC2;
		}
		else if (na == TYPE_IVEC3 && nb == TYPE_IVEC3) {
			valid = true;
			ret_type = TYPE_IVEC3;
		}
		else if (na == TYPE_IVEC4 && nb == TYPE_IVEC4) {
			valid = true;
			ret_type = TYPE_IVEC4;
			/////
		}
		else if (na == TYPE_UINT && nb == TYPE_UINT) {
			valid = true;
			ret_type = TYPE_UINT;
		}
		else if (na == TYPE_UVEC2 && nb == TYPE_UINT) {
			valid = true;
			ret_type = TYPE_UVEC2;
		}
		else if (na == TYPE_UVEC3 && nb == TYPE_UINT) {
			valid = true;
			ret_type = TYPE_UVEC3;
		}
		else if (na == TYPE_UVEC4 && nb == TYPE_UINT) {
			valid = true;
			ret_type = TYPE_UVEC4;
		}
		else if (na == TYPE_UVEC2 && nb == TYPE_UVEC2) {
			valid = true;
			ret_type = TYPE_UVEC2;
		}
		else if (na == TYPE_UVEC3 && nb == TYPE_UVEC3) {
			valid = true;
			ret_type = TYPE_UVEC3;
		}
		else if (na == TYPE_UVEC4 && nb == TYPE_UVEC4) {
			valid = true;
			ret_type = TYPE_UVEC4;
		}
	} break;
	case OP_ASSIGN_SHIFT_LEFT:
	case OP_ASSIGN_SHIFT_RIGHT:
	case OP_SHIFT_LEFT:
	case OP_SHIFT_RIGHT: {
		if ((!p_op->arguments[0]->is_indexed() && p_op->arguments[0]->get_array_size() > 0) ||
			(!p_op->arguments[1]->is_indexed() && p_op->arguments[1]->get_array_size() > 0)) {
			break; // don't accept arrays
		}

		DataType na = p_op->arguments[0]->get_datatype();
		DataType nb = p_op->arguments[1]->get_datatype();

		if (na == TYPE_INT && nb == TYPE_INT) {
			valid = true;
			ret_type = TYPE_INT;
		}
		else if (na == TYPE_IVEC2 && nb == TYPE_INT) {
			valid = true;
			ret_type = TYPE_IVEC2;
		}
		else if (na == TYPE_IVEC3 && nb == TYPE_INT) {
			valid = true;
			ret_type = TYPE_IVEC3;
		}
		else if (na == TYPE_IVEC4 && nb == TYPE_INT) {
			valid = true;
			ret_type = TYPE_IVEC4;
		}
		else if (na == TYPE_IVEC2 && nb == TYPE_IVEC2) {
			valid = true;
			ret_type = TYPE_IVEC2;
		}
		else if (na == TYPE_IVEC3 && nb == TYPE_IVEC3) {
			valid = true;
			ret_type = TYPE_IVEC3;
		}
		else if (na == TYPE_IVEC4 && nb == TYPE_IVEC4) {
			valid = true;
			ret_type = TYPE_IVEC4;
		}
		else if (na == TYPE_UINT && nb == TYPE_UINT) {
			valid = true;
			ret_type = TYPE_UINT;
		}
		else if (na == TYPE_UVEC2 && nb == TYPE_UINT) {
			valid = true;
			ret_type = TYPE_UVEC2;
		}
		else if (na == TYPE_UVEC3 && nb == TYPE_UINT) {
			valid = true;
			ret_type = TYPE_UVEC3;
		}
		else if (na == TYPE_UVEC4 && nb == TYPE_UINT) {
			valid = true;
			ret_type = TYPE_UVEC4;
		}
		else if (na == TYPE_UVEC2 && nb == TYPE_UVEC2) {
			valid = true;
			ret_type = TYPE_UVEC2;
		}
		else if (na == TYPE_UVEC3 && nb == TYPE_UVEC3) {
			valid = true;
			ret_type = TYPE_UVEC3;
		}
		else if (na == TYPE_UVEC4 && nb == TYPE_UVEC4) {
			valid = true;
			ret_type = TYPE_UVEC4;
		}
	} break;
	case OP_ASSIGN: {
		int sa = 0;
		int sb = 0;
		if (!p_op->arguments[0]->is_indexed()) {
			sa = p_op->arguments[0]->get_array_size();
		}
		if (!p_op->arguments[1]->is_indexed()) {
			sb = p_op->arguments[1]->get_array_size();
		}
		if (sa != sb) {
			break; // don't accept arrays if their sizes are not equal
		}

		DataType na = p_op->arguments[0]->get_datatype();
		DataType nb = p_op->arguments[1]->get_datatype();
		if (na == TYPE_STRUCT || nb == TYPE_STRUCT) {
			valid =
				p_op->arguments[0]->get_datatype_name() == p_op->arguments[1]->get_datatype_name();
		}
		else {
			valid = na == nb;
		}
		ret_type = na;
		ret_size = sa;
	} break;
	case OP_ASSIGN_ADD:
	case OP_ASSIGN_SUB:
	case OP_ASSIGN_MUL:
	case OP_ASSIGN_DIV: {
		int sa = 0;
		int sb = 0;
		if (!p_op->arguments[0]->is_indexed()) {
			sa = p_op->arguments[0]->get_array_size();
		}
		if (!p_op->arguments[1]->is_indexed()) {
			sb = p_op->arguments[1]->get_array_size();
		}
		if (sa > 0 || sb > 0) {
			break; // don't accept arrays
		}

		DataType na = p_op->arguments[0]->get_datatype();
		DataType nb = p_op->arguments[1]->get_datatype();

		if (na == nb) {
			valid = (na > TYPE_BVEC4 && na <= TYPE_MAT4);
			ret_type = na;
		}
		else if (na == TYPE_IVEC2 && nb == TYPE_INT) {
			valid = true;
			ret_type = TYPE_IVEC2;
		}
		else if (na == TYPE_IVEC3 && nb == TYPE_INT) {
			valid = true;
			ret_type = TYPE_IVEC3;
		}
		else if (na == TYPE_IVEC4 && nb == TYPE_INT) {
			valid = true;
			ret_type = TYPE_IVEC4;
		}
		else if (na == TYPE_UVEC2 && nb == TYPE_UINT) {
			valid = true;
			ret_type = TYPE_UVEC2;
		}
		else if (na == TYPE_UVEC3 && nb == TYPE_UINT) {
			valid = true;
			ret_type = TYPE_UVEC3;
		}
		else if (na == TYPE_UVEC4 && nb == TYPE_UINT) {
			valid = true;
			ret_type = TYPE_UVEC4;
		}
		else if (na == TYPE_VEC2 && nb == TYPE_FLOAT) {
			valid = true;
			ret_type = TYPE_VEC2;
		}
		else if (na == TYPE_VEC3 && nb == TYPE_FLOAT) {
			valid = true;
			ret_type = TYPE_VEC3;
		}
		else if (na == TYPE_VEC4 && nb == TYPE_FLOAT) {
			valid = true;
			ret_type = TYPE_VEC4;
		}
		else if (na == TYPE_MAT2 && nb == TYPE_FLOAT) {
			valid = true;
			ret_type = TYPE_MAT2;
		}
		else if (na == TYPE_MAT3 && nb == TYPE_FLOAT) {
			valid = true;
			ret_type = TYPE_MAT3;
		}
		else if (na == TYPE_MAT4 && nb == TYPE_FLOAT) {
			valid = true;
			ret_type = TYPE_MAT4;
		}
		else if (p_op->op == OP_ASSIGN_MUL && na == TYPE_VEC2 && nb == TYPE_MAT2) {
			valid = true;
			ret_type = TYPE_VEC2;
		}
		else if (p_op->op == OP_ASSIGN_MUL && na == TYPE_VEC3 && nb == TYPE_MAT3) {
			valid = true;
			ret_type = TYPE_VEC3;
		}
		else if (p_op->op == OP_ASSIGN_MUL && na == TYPE_VEC4 && nb == TYPE_MAT4) {
			valid = true;
			ret_type = TYPE_VEC4;
		}
	} break;
	case OP_ASSIGN_BIT_AND:
	case OP_ASSIGN_BIT_OR:
	case OP_ASSIGN_BIT_XOR:
	case OP_BIT_AND:
	case OP_BIT_OR:
	case OP_BIT_XOR: {
		/*
		 * The bitwise operators and (&), exclusive-or (^), and inclusive-or (|). The operands must
		 * be of type signed or unsigned integers or integer vectors. The operands cannot be vectors
		 * of differing size. If one operand is a scalar and the other a vector, the scalar is
		 * applied component-wise to the vector, resulting in the same type as the vector. The
		 * fundamental types of the operands (signed or unsigned) must match.
		 */

		int sa = 0;
		int sb = 0;
		if (!p_op->arguments[0]->is_indexed()) {
			sa = p_op->arguments[0]->get_array_size();
		}
		if (!p_op->arguments[1]->is_indexed()) {
			sb = p_op->arguments[1]->get_array_size();
		}
		if (sa > 0 || sb > 0) {
			break; // don't accept arrays
		}

		DataType na = p_op->arguments[0]->get_datatype();
		DataType nb = p_op->arguments[1]->get_datatype();

		if (na > nb && p_op->op >= OP_BIT_AND) {
			// can swap for non assign
			SWAP(na, nb);
		}

		if (na == TYPE_INT && nb == TYPE_INT) {
			valid = true;
			ret_type = TYPE_INT;
		}
		else if (na == TYPE_IVEC2 && nb == TYPE_INT) {
			valid = true;
			ret_type = TYPE_IVEC2;
		}
		else if (na == TYPE_IVEC3 && nb == TYPE_INT) {
			valid = true;
			ret_type = TYPE_IVEC3;
		}
		else if (na == TYPE_IVEC4 && nb == TYPE_INT) {
			valid = true;
			ret_type = TYPE_IVEC4;
		}
		else if (na == TYPE_IVEC2 && nb == TYPE_IVEC2) {
			valid = true;
			ret_type = TYPE_IVEC2;
		}
		else if (na == TYPE_IVEC3 && nb == TYPE_IVEC3) {
			valid = true;
			ret_type = TYPE_IVEC3;
		}
		else if (na == TYPE_IVEC4 && nb == TYPE_IVEC4) {
			valid = true;
			ret_type = TYPE_IVEC4;
			/////
		}
		else if (na == TYPE_UINT && nb == TYPE_UINT) {
			valid = true;
			ret_type = TYPE_UINT;
		}
		else if (na == TYPE_UVEC2 && nb == TYPE_UINT) {
			valid = true;
			ret_type = TYPE_UVEC2;
		}
		else if (na == TYPE_UVEC3 && nb == TYPE_UINT) {
			valid = true;
			ret_type = TYPE_UVEC3;
		}
		else if (na == TYPE_UVEC4 && nb == TYPE_UINT) {
			valid = true;
			ret_type = TYPE_UVEC4;
		}
		else if (na == TYPE_UVEC2 && nb == TYPE_UVEC2) {
			valid = true;
			ret_type = TYPE_UVEC2;
		}
		else if (na == TYPE_UVEC3 && nb == TYPE_UVEC3) {
			valid = true;
			ret_type = TYPE_UVEC3;
		}
		else if (na == TYPE_UVEC4 && nb == TYPE_UVEC4) {
			valid = true;
			ret_type = TYPE_UVEC4;
		}
	} break;
	case OP_BIT_INVERT: { // unaries
		if (!p_op->arguments[0]->is_indexed() && p_op->arguments[0]->get_array_size() > 0) {
			break; // don't accept arrays
		}

		DataType na = p_op->arguments[0]->get_datatype();
		valid = na >= TYPE_INT && na < TYPE_FLOAT;
		ret_type = na;
	} break;
	case OP_SELECT_IF: {
		int sa = 0;
		int sb = 0;
		if (!p_op->arguments[1]->is_indexed()) {
			sa = p_op->arguments[1]->get_array_size();
		}
		if (!p_op->arguments[2]->is_indexed()) {
			sb = p_op->arguments[2]->get_array_size();
		}
		if (sa != sb) {
			break; // don't accept arrays if their sizes are not equal
		}

		DataType na = p_op->arguments[0]->get_datatype();
		DataType nb = p_op->arguments[1]->get_datatype();
		DataType nc = p_op->arguments[2]->get_datatype();

		bool is_same = false;
		if (nb == nc) {
			is_same = true;

			if (nb == TYPE_STRUCT) {
				String tb = p_op->arguments[1]->get_datatype_name();
				String tc = p_op->arguments[2]->get_datatype_name();

				if (tb != tc) {
					break;
				}

				ret_struct_name = tb;
			}
		}

		valid = na == TYPE_BOOL && is_same && !is_sampler_type(nb);
		ret_type = nb;
		ret_size = sa;
	} break;
	default: {
		ERR_FAIL_V(false);
	}
	}

	if (r_ret_type) {
		*r_ret_type = ret_type;
	}
	if (r_ret_size) {
		*r_ret_size = ret_size;
	}
	if (r_ret_struct_name) {
		*r_ret_struct_name = ret_struct_name;
	}

	if (valid && (!p_block || p_block->use_op_eval)) {
		// Need to be placed here and not in the `_reduce_expression` because otherwise expressions
		// like `1 + 2 / 2` will not work correctly.
		valid = _eval_operator(p_block, p_function_info, p_op);
	}

	return valid;
}

Vector<ShaderLanguage::Scalar> ShaderLanguage::_get_node_values(
	const BlockNode* p_block, const FunctionInfo& p_function_info, Node* p_node)
{
	Vector<Scalar> result;

	switch (p_node->type) {
	case Node::NODE_TYPE_VARIABLE: {
		_find_identifier(p_block, false, p_function_info, static_cast<VariableNode*>(p_node)->name,
			nullptr, nullptr, nullptr, nullptr, nullptr, &result);
	} break;
	default: {
		result = p_node->get_values();
	} break;
	}

	return result;
}

bool ShaderLanguage::_eval_operator(
	const BlockNode* p_block, const FunctionInfo& p_function_info, OperatorNode* p_op)
{
	bool is_valid = true;

	switch (p_op->op) {
	case OP_EQUAL:
	case OP_NOT_EQUAL:
	case OP_LESS:
	case OP_LESS_EQUAL:
	case OP_GREATER:
	case OP_GREATER_EQUAL:
	case OP_AND:
	case OP_OR:
	case OP_ADD:
	case OP_SUB:
	case OP_MUL:
	case OP_DIV:
	case OP_MOD:
	case OP_SHIFT_LEFT:
	case OP_SHIFT_RIGHT:
	case OP_BIT_AND:
	case OP_BIT_OR:
	case OP_BIT_XOR: {
		DataType a = p_op->arguments[0]->get_datatype();
		DataType b = p_op->arguments[1]->get_datatype();

		bool is_op_vec_transform = false;
		if (p_op->op == OP_MUL) {
			DataType ta = a;
			DataType tb = b;

			if (ta > tb) {
				SWAP(ta, tb);
			}
			if (ta == TYPE_VEC2 && tb == TYPE_MAT2) {
				is_op_vec_transform = true;
			}
			else if (ta == TYPE_VEC3 && tb == TYPE_MAT3) {
				is_op_vec_transform = true;
			}
			else if (ta == TYPE_VEC4 && tb == TYPE_MAT4) {
				is_op_vec_transform = true;
			}
		}

		Vector<Scalar> va = _get_node_values(p_block, p_function_info, p_op->arguments[0]);
		Vector<Scalar> vb = _get_node_values(p_block, p_function_info, p_op->arguments[1]);

		if (is_op_vec_transform) {
			p_op->values = _eval_vector_transform(va, vb, a, b, p_op->get_datatype());
		}
		else {
			p_op->values = _eval_vector(va, vb, a, b, p_op->get_datatype(), p_op->op, is_valid);
		}
	} break;
	case OP_NOT:
	case OP_NEGATE:
	case OP_BIT_INVERT: {
		p_op->values =
			_eval_unary_vector(_get_node_values(p_block, p_function_info, p_op->arguments[0]),
				p_op->get_datatype(), p_op->op);
	} break;
	default: {
	} break;
	}

	return is_valid;
}

ShaderLanguage::Scalar ShaderLanguage::_eval_unary_scalar(
	const Scalar& p_a, Operator p_op, DataType p_ret_type)
{
	Scalar scalar;

	switch (p_op) {
	case OP_NOT: {
		scalar.boolean = !p_a.boolean;
	} break;
	case OP_NEGATE: {
		if (p_ret_type >= TYPE_INT && p_ret_type <= TYPE_IVEC4) {
			scalar.sint = -p_a.sint;
		}
		else if (p_ret_type >= TYPE_UINT && p_ret_type <= TYPE_UVEC4) {
			// Intentionally wrap the unsigned int value, because GLSL does.
			scalar.uint = 0 - p_a.uint;
		}
		else { // float types
			scalar.real = -scalar.real;
		}
	} break;
	case OP_BIT_INVERT: {
		if (p_ret_type >= TYPE_INT && p_ret_type <= TYPE_IVEC4) {
			scalar.sint = ~p_a.sint;
		}
		else { // uint types
			scalar.uint = ~p_a.uint;
		}
	} break;
	default: {
	} break;
	}

	return scalar;
}

ShaderLanguage::Scalar ShaderLanguage::_eval_scalar(
	const Scalar& p_a, const Scalar& p_b, Operator p_op, DataType p_ret_type, bool& r_is_valid)
{
	Scalar scalar;

	switch (p_op) {
	case OP_EQUAL: {
		scalar.boolean = p_a.boolean == p_b.boolean;
	} break;
	case OP_NOT_EQUAL: {
		scalar.boolean = p_a.boolean != p_b.boolean;
	} break;
	case OP_LESS: {
		if (p_ret_type == TYPE_INT) {
			scalar.boolean = p_a.sint < p_b.sint;
		}
		else if (p_ret_type == TYPE_UINT) {
			scalar.boolean = p_a.uint < p_b.uint;
		}
		else { // float type
			scalar.boolean = p_a.real < p_b.real;
		}
	} break;
	case OP_LESS_EQUAL: {
		if (p_ret_type == TYPE_INT) {
			scalar.boolean = p_a.sint <= p_b.sint;
		}
		else if (p_ret_type == TYPE_UINT) {
			scalar.boolean = p_a.uint <= p_b.uint;
		}
		else { // float type
			scalar.boolean = p_a.real <= p_b.real;
		}
	} break;
	case OP_GREATER: {
		if (p_ret_type == TYPE_INT) {
			scalar.boolean = p_a.sint > p_b.sint;
		}
		else if (p_ret_type == TYPE_UINT) {
			scalar.boolean = p_a.uint > p_b.uint;
		}
		else { // float type
			scalar.boolean = p_a.real > p_b.real;
		}
	} break;
	case OP_GREATER_EQUAL: {
		if (p_ret_type == TYPE_INT) {
			scalar.boolean = p_a.sint >= p_b.sint;
		}
		else if (p_ret_type == TYPE_UINT) {
			scalar.boolean = p_a.uint >= p_b.uint;
		}
		else { // float type
			scalar.boolean = p_a.real >= p_b.real;
		}
	} break;
	case OP_AND: {
		scalar.boolean = p_a.boolean && p_b.boolean;
	} break;
	case OP_OR: {
		scalar.boolean = p_a.boolean || p_b.boolean;
	} break;
	case OP_ADD: {
		if (p_ret_type >= TYPE_INT && p_ret_type <= TYPE_IVEC4) {
			scalar.sint = p_a.sint + p_b.sint;
		}
		else if (p_ret_type >= TYPE_UINT && p_ret_type <= TYPE_UVEC4) {
			scalar.uint = p_a.uint + p_b.uint;
		}
		else { // float + matrix types
			scalar.real = p_a.real + p_b.real;
		}
	} break;
	case OP_SUB: {
		if (p_ret_type >= TYPE_INT && p_ret_type <= TYPE_IVEC4) {
			scalar.sint = p_a.sint - p_b.sint;
		}
		else if (p_ret_type >= TYPE_UINT && p_ret_type <= TYPE_UVEC4) {
			scalar.uint = p_a.uint - p_b.uint;
		}
		else { // float + matrix types
			scalar.real = p_a.real - p_b.real;
		}
	} break;
	case OP_MUL: {
		if (p_ret_type >= TYPE_INT && p_ret_type <= TYPE_IVEC4) {
			scalar.sint = p_a.sint * p_b.sint;
		}
		else if (p_ret_type >= TYPE_UINT && p_ret_type <= TYPE_UVEC4) {
			scalar.uint = p_a.uint * p_b.uint;
		}
		else { // float + matrix types
			scalar.real = p_a.real * p_b.real;
		}
	} break;
	case OP_DIV: {
		if (p_ret_type >= TYPE_INT && p_ret_type <= TYPE_IVEC4) {
			if (p_b.sint == 0) {
				_set_error(RTR("Division by zero error."));
				r_is_valid = false;
				break;
			}
			scalar.sint = p_a.sint / p_b.sint;
		}
		else if (p_ret_type == TYPE_UINT && p_ret_type <= TYPE_UVEC4) {
			if (p_b.uint == 0U) {
				_set_error(RTR("Division by zero error."));
				r_is_valid = false;
				break;
			}
			scalar.uint = p_a.uint / p_b.uint;
		}
		else { // float + matrix types
			scalar.real = p_a.real / p_b.real;
		}
	} break;
	case OP_MOD: {
		if (p_ret_type >= TYPE_INT && p_ret_type <= TYPE_IVEC4) {
			if (p_b.sint == 0) {
				_set_error(RTR("Modulo by zero error."));
				r_is_valid = false;
				break;
			}
			scalar.sint = p_a.sint % p_b.sint;
		}
		else { // uint types
			if (p_b.uint == 0U) {
				_set_error(RTR("Modulo by zero error."));
				r_is_valid = false;
				break;
			}
			scalar.uint = p_a.uint % p_b.uint;
		}
	} break;
	case OP_SHIFT_LEFT: {
		if (p_ret_type >= TYPE_INT && p_ret_type <= TYPE_IVEC4) {
			scalar.sint = p_a.sint << p_b.sint;
		}
		else { // uint types
			scalar.uint = p_a.uint << p_b.uint;
		}
	} break;
	case OP_SHIFT_RIGHT: {
		if (p_ret_type >= TYPE_INT && p_ret_type <= TYPE_IVEC4) {
			scalar.sint = p_a.sint >> p_b.sint;
		}
		else { // uint types
			scalar.uint = p_a.uint >> p_b.uint;
		}
	} break;
	case OP_BIT_AND: {
		if (p_ret_type >= TYPE_INT && p_ret_type <= TYPE_IVEC4) {
			scalar.sint = p_a.sint & p_b.sint;
		}
		else { // uint types
			scalar.uint = p_a.uint & p_b.uint;
		}
	} break;
	case OP_BIT_OR: {
		if (p_ret_type >= TYPE_INT && p_ret_type <= TYPE_IVEC4) {
			scalar.sint = p_a.sint | p_b.sint;
		}
		else { // uint types
			scalar.uint = p_a.uint | p_b.uint;
		}
	} break;
	case OP_BIT_XOR: {
		if (p_ret_type >= TYPE_INT && p_ret_type <= TYPE_IVEC4) {
			scalar.sint = p_a.sint ^ p_b.sint;
		}
		else { // uint types
			scalar.uint = p_a.uint ^ p_b.uint;
		}
	} break;
	default: {
	} break;
	}

	return scalar;
}

Vector<ShaderLanguage::Scalar> ShaderLanguage::_eval_unary_vector(
	const Vector<Scalar>&
p_va, DataType p_ret_type, Operator p_op)
{
	uint32_t size = get_datatype_component_count(p_ret_type);
	if (p_va.size() != p_ret_type) {
		return Vector<Scalar>(); // Non-evaluable values should not be parsed further.
	}
	Vector<Scalar> value;
	value.resize(size);

	Scalar* w = value.ptrw();
	for (uint32_t i = 0U; i < size; i++) {
		w[i] = _eval_unary_scalar(p_va[i], p_op, p_ret_type);
	}
	return value;
}

Vector<ShaderLanguage::Scalar> ShaderLanguage::_eval_vector(const Vector<Scalar>& p_va,
	const Vector<Scalar>& p_vb, DataType p_left_type, DataType p_right_type, DataType p_ret_type,
	Operator p_op, bool& r_is_valid)
{
	uint32_t left_size = get_datatype_component_count(p_left_type);
	uint32_t right_size = get_datatype_component_count(p_right_type);

	if (p_va.size() != left_size || p_vb.size() != right_size) {
		return Vector<Scalar>(); // Non-evaluable values should not be parsed further.
	}

	uint32_t ret_size = get_datatype_component_count(p_ret_type);
	Vector<Scalar> value;
	value.resize(ret_size);

	Scalar* w = value.ptrw();
	for (uint32_t i = 0U; i < ret_size; i++) {
		w[i] = _eval_scalar(p_va[MIN(i, left_size - 1)], p_vb[MIN(i, right_size - 1)], p_op,
			p_ret_type, r_is_valid);
		if (!r_is_valid) {
			return value;
		}
	}
	return value;
}

Vector<ShaderLanguage::Scalar> ShaderLanguage::_eval_vector_transform(const Vector<Scalar>& p_va,
	const Vector<Scalar>& p_vb, DataType p_left_type, DataType p_right_type, DataType p_ret_type)
{
	uint32_t left_size = get_datatype_component_count(p_left_type);
	uint32_t right_size = get_datatype_component_count(p_right_type);

	if (p_va.size() != left_size || p_vb.size() != right_size) {
		return Vector<Scalar>(); // Non-evaluable values should not be parsed further.
	}

	uint32_t ret_size = get_datatype_component_count(p_ret_type);
	Vector<Scalar> value;
	value.resize_initialized(ret_size);

	Scalar* w = value.ptrw();
	switch (p_ret_type) {
	case TYPE_VEC2: {
		if (left_size == 2) { // v * m
			Vector2 v = Vector2(p_va[0].real, p_va[1].real);

			w[0].real = (p_vb[0].real * v.x + p_vb[1].real * v.y);
			w[1].real = (p_vb[2].real * v.x + p_vb[3].real * v.y);
		}
		else { // m * v
			Vector2 v = Vector2(p_vb[0].real, p_vb[1].real);

			w[0].real = (p_va[0].real * v.x + p_va[2].real * v.y);
			w[1].real = (p_va[1].real * v.x + p_va[3].real * v.y);
		}
	} break;
	case TYPE_VEC3: {
		if (left_size == 3) { // v * m
			Vector3 v = Vector3(p_va[0].real, p_va[1].real, p_va[2].real);

			w[0].real = (p_vb[0].real * v.x + p_vb[1].real * v.y + p_vb[2].real * v.z);
			w[1].real = (p_vb[3].real * v.x + p_vb[4].real * v.y + p_vb[5].real * v.z);
			w[2].real = (p_vb[6].real * v.x + p_vb[7].real * v.y + p_vb[8].real * v.z);
		}
		else { // m * v
			Vector3 v = Vector3(p_vb[0].real, p_vb[1].real, p_vb[2].real);

			w[0].real = (p_va[0].real * v.x + p_va[3].real * v.y + p_va[6].real * v.z);
			w[1].real = (p_va[1].real * v.x + p_va[4].real * v.y + p_va[7].real * v.z);
			w[2].real = (p_va[2].real * v.x + p_va[5].real * v.y + p_va[8].real * v.z);
		}
	} break;
	case TYPE_VEC4: {
		if (left_size == 4) { // v * m
			Vector4 v = Vector4(p_va[0].real, p_va[1].real, p_va[2].real, p_va[3].real);

			w[0].real =
				(p_vb[0].real * v.x + p_vb[1].real * v.y + p_vb[2].real * v.z + p_vb[3].real * v.w);
			w[1].real =
				(p_vb[4].real * v.x + p_vb[5].real * v.y + p_vb[6].real * v.z + p_vb[7].real * v.w);
			w[2].real = (p_vb[8].real * v.x + p_vb[9].real * v.y + p_vb[10].real * v.z +
						 p_vb[11].real * v.w);
			w[3].real = (p_vb[12].real * v.x + p_vb[13].real * v.y + p_vb[14].real * v.z +
						 p_vb[15].real * v.w);
		}
		else { // m * v
			Vector4 v = Vector4(p_vb[0].real, p_vb[1].real, p_vb[2].real, p_vb[3].real);

			w[0].real = (p_va[0].real * v.x + p_va[4].real * v.y + p_va[8].real * v.z +
						 p_va[12].real * v.w);
			w[1].real = (p_va[1].real * v.x + p_va[5].real * v.y + p_va[9].real * v.z +
						 p_va[13].real * v.w);
			w[2].real = (p_va[2].real * v.x + p_va[6].real * v.y + p_va[10].real * v.z +
						 p_va[14].real * v.w);
			w[3].real = (p_va[3].real * v.x + p_va[7].real * v.y + p_va[11].real * v.z +
						 p_va[15].real * v.w);
		}
	} break;
	default: {
	} break;
	}

	return value;
}

const ShaderLanguage::BuiltinFuncDef ShaderLanguage::builtin_func_defs[] = {
	// Constructors.

	{"bvec2", TYPE_BVEC2, {TYPE_BOOL, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"bvec2", TYPE_BVEC2, {TYPE_BOOL, TYPE_BOOL, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"bvec3", TYPE_BVEC3, {TYPE_BOOL, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"bvec3", TYPE_BVEC3, {TYPE_BOOL, TYPE_BOOL, TYPE_BOOL, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"bvec3", TYPE_BVEC3, {TYPE_BVEC2, TYPE_BOOL, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"bvec3", TYPE_BVEC3, {TYPE_BOOL, TYPE_BVEC2, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"bvec4", TYPE_BVEC4, {TYPE_BOOL, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"bvec4", TYPE_BVEC4, {TYPE_BOOL, TYPE_BOOL, TYPE_BOOL, TYPE_BOOL, TYPE_VOID}, {""}, TAG_GLOBAL,
		false},
	{"bvec4", TYPE_BVEC4, {TYPE_BOOL, TYPE_BVEC2, TYPE_BOOL, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"bvec4", TYPE_BVEC4, {TYPE_BVEC2, TYPE_BOOL, TYPE_BOOL, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"bvec4", TYPE_BVEC4, {TYPE_BOOL, TYPE_BOOL, TYPE_BVEC2, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"bvec4", TYPE_BVEC4, {TYPE_BOOL, TYPE_BVEC3, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"bvec4", TYPE_BVEC4, {TYPE_BVEC3, TYPE_BOOL, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"bvec4", TYPE_BVEC4, {TYPE_BVEC2, TYPE_BVEC2, TYPE_VOID}, {""}, TAG_GLOBAL, false},

	{"vec2", TYPE_VEC2, {TYPE_FLOAT, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"vec2", TYPE_VEC2, {TYPE_FLOAT, TYPE_FLOAT, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"vec3", TYPE_VEC3, {TYPE_FLOAT, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"vec3", TYPE_VEC3, {TYPE_FLOAT, TYPE_FLOAT, TYPE_FLOAT, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"vec3", TYPE_VEC3, {TYPE_VEC2, TYPE_FLOAT, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"vec3", TYPE_VEC3, {TYPE_FLOAT, TYPE_VEC2, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"vec4", TYPE_VEC4, {TYPE_FLOAT, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"vec4", TYPE_VEC4, {TYPE_FLOAT, TYPE_FLOAT, TYPE_FLOAT, TYPE_FLOAT, TYPE_VOID}, {""},
		TAG_GLOBAL, false},
	{"vec4", TYPE_VEC4, {TYPE_FLOAT, TYPE_VEC2, TYPE_FLOAT, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"vec4", TYPE_VEC4, {TYPE_VEC2, TYPE_FLOAT, TYPE_FLOAT, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"vec4", TYPE_VEC4, {TYPE_FLOAT, TYPE_FLOAT, TYPE_VEC2, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"vec4", TYPE_VEC4, {TYPE_FLOAT, TYPE_VEC3, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"vec4", TYPE_VEC4, {TYPE_VEC3, TYPE_FLOAT, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"vec4", TYPE_VEC4, {TYPE_VEC2, TYPE_VEC2, TYPE_VOID}, {""}, TAG_GLOBAL, false},

	{"ivec2", TYPE_IVEC2, {TYPE_INT, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"ivec2", TYPE_IVEC2, {TYPE_INT, TYPE_INT, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"ivec3", TYPE_IVEC3, {TYPE_INT, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"ivec3", TYPE_IVEC3, {TYPE_INT, TYPE_INT, TYPE_INT, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"ivec3", TYPE_IVEC3, {TYPE_IVEC2, TYPE_INT, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"ivec3", TYPE_IVEC3, {TYPE_INT, TYPE_IVEC2, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"ivec4", TYPE_IVEC4, {TYPE_INT, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"ivec4", TYPE_IVEC4, {TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_VOID}, {""}, TAG_GLOBAL,
		false},
	{"ivec4", TYPE_IVEC4, {TYPE_INT, TYPE_IVEC2, TYPE_INT, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"ivec4", TYPE_IVEC4, {TYPE_IVEC2, TYPE_INT, TYPE_INT, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"ivec4", TYPE_IVEC4, {TYPE_INT, TYPE_INT, TYPE_IVEC2, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"ivec4", TYPE_IVEC4, {TYPE_INT, TYPE_IVEC3, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"ivec4", TYPE_IVEC4, {TYPE_IVEC3, TYPE_INT, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"ivec4", TYPE_IVEC4, {TYPE_IVEC2, TYPE_IVEC2, TYPE_VOID}, {""}, TAG_GLOBAL, false},

	{"uvec2", TYPE_UVEC2, {TYPE_UINT, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"uvec2", TYPE_UVEC2, {TYPE_UINT, TYPE_UINT, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"uvec3", TYPE_UVEC3, {TYPE_UINT, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"uvec3", TYPE_UVEC3, {TYPE_UINT, TYPE_UINT, TYPE_UINT, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"uvec3", TYPE_UVEC3, {TYPE_UVEC2, TYPE_UINT, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"uvec3", TYPE_UVEC3, {TYPE_UINT, TYPE_UVEC2, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"uvec4", TYPE_UVEC4, {TYPE_UINT, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"uvec4", TYPE_UVEC4, {TYPE_UINT, TYPE_UINT, TYPE_UINT, TYPE_UINT, TYPE_VOID}, {""}, TAG_GLOBAL,
		false},
	{"uvec4", TYPE_UVEC4, {TYPE_UINT, TYPE_UVEC2, TYPE_UINT, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"uvec4", TYPE_UVEC4, {TYPE_UVEC2, TYPE_UINT, TYPE_UINT, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"uvec4", TYPE_UVEC4, {TYPE_UINT, TYPE_UINT, TYPE_UVEC2, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"uvec4", TYPE_UVEC4, {TYPE_UINT, TYPE_UVEC3, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"uvec4", TYPE_UVEC4, {TYPE_UVEC3, TYPE_UINT, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"uvec4", TYPE_UVEC4, {TYPE_UVEC2, TYPE_UVEC2, TYPE_VOID}, {""}, TAG_GLOBAL, false},

	{"mat2", TYPE_MAT2, {TYPE_VEC2, TYPE_VEC2, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"mat3", TYPE_MAT3, {TYPE_VEC3, TYPE_VEC3, TYPE_VEC3, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"mat4", TYPE_MAT4, {TYPE_VEC4, TYPE_VEC4, TYPE_VEC4, TYPE_VEC4, TYPE_VOID}, {""}, TAG_GLOBAL,
		false},

	{"mat2", TYPE_MAT2, {TYPE_FLOAT, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"mat3", TYPE_MAT3, {TYPE_FLOAT, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"mat4", TYPE_MAT4, {TYPE_FLOAT, TYPE_VOID}, {""}, TAG_GLOBAL, false},

	// Conversion scalars.

	{"int", TYPE_INT, {TYPE_BOOL, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"int", TYPE_INT, {TYPE_INT, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"int", TYPE_INT, {TYPE_UINT, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"int", TYPE_INT, {TYPE_FLOAT, TYPE_VOID}, {""}, TAG_GLOBAL, false},

	{"float", TYPE_FLOAT, {TYPE_BOOL, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"float", TYPE_FLOAT, {TYPE_INT, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"float", TYPE_FLOAT, {TYPE_UINT, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"float", TYPE_FLOAT, {TYPE_FLOAT, TYPE_VOID}, {""}, TAG_GLOBAL, false},

	{"uint", TYPE_UINT, {TYPE_BOOL, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"uint", TYPE_UINT, {TYPE_INT, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"uint", TYPE_UINT, {TYPE_UINT, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"uint", TYPE_UINT, {TYPE_FLOAT, TYPE_VOID}, {""}, TAG_GLOBAL, false},

	{"bool", TYPE_BOOL, {TYPE_BOOL, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"bool", TYPE_BOOL, {TYPE_INT, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"bool", TYPE_BOOL, {TYPE_UINT, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"bool", TYPE_BOOL, {TYPE_FLOAT, TYPE_VOID}, {""}, TAG_GLOBAL, false},

	// Conversion vectors.

	{"ivec2", TYPE_IVEC2, {TYPE_BVEC2, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"ivec2", TYPE_IVEC2, {TYPE_IVEC2, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"ivec2", TYPE_IVEC2, {TYPE_UVEC2, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"ivec2", TYPE_IVEC2, {TYPE_VEC2, TYPE_VOID}, {""}, TAG_GLOBAL, false},

	{"vec2", TYPE_VEC2, {TYPE_BVEC2, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"vec2", TYPE_VEC2, {TYPE_IVEC2, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"vec2", TYPE_VEC2, {TYPE_UVEC2, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"vec2", TYPE_VEC2, {TYPE_VEC2, TYPE_VOID}, {""}, TAG_GLOBAL, false},

	{"uvec2", TYPE_UVEC2, {TYPE_BVEC2, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"uvec2", TYPE_UVEC2, {TYPE_IVEC2, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"uvec2", TYPE_UVEC2, {TYPE_UVEC2, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"uvec2", TYPE_UVEC2, {TYPE_VEC2, TYPE_VOID}, {""}, TAG_GLOBAL, false},

	{"bvec2", TYPE_BVEC2, {TYPE_BVEC2, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"bvec2", TYPE_BVEC2, {TYPE_IVEC2, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"bvec2", TYPE_BVEC2, {TYPE_UVEC2, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"bvec2", TYPE_BVEC2, {TYPE_VEC2, TYPE_VOID}, {""}, TAG_GLOBAL, false},

	{"ivec3", TYPE_IVEC3, {TYPE_BVEC3, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"ivec3", TYPE_IVEC3, {TYPE_IVEC3, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"ivec3", TYPE_IVEC3, {TYPE_UVEC3, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"ivec3", TYPE_IVEC3, {TYPE_VEC3, TYPE_VOID}, {""}, TAG_GLOBAL, false},

	{"vec3", TYPE_VEC3, {TYPE_BVEC3, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"vec3", TYPE_VEC3, {TYPE_IVEC3, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"vec3", TYPE_VEC3, {TYPE_UVEC3, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"vec3", TYPE_VEC3, {TYPE_VEC3, TYPE_VOID}, {""}, TAG_GLOBAL, false},

	{"uvec3", TYPE_UVEC3, {TYPE_BVEC3, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"uvec3", TYPE_UVEC3, {TYPE_IVEC3, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"uvec3", TYPE_UVEC3, {TYPE_UVEC3, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"uvec3", TYPE_UVEC3, {TYPE_VEC3, TYPE_VOID}, {""}, TAG_GLOBAL, false},

	{"bvec3", TYPE_BVEC3, {TYPE_BVEC3, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"bvec3", TYPE_BVEC3, {TYPE_IVEC3, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"bvec3", TYPE_BVEC3, {TYPE_UVEC3, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"bvec3", TYPE_BVEC3, {TYPE_VEC3, TYPE_VOID}, {""}, TAG_GLOBAL, false},

	{"ivec4", TYPE_IVEC4, {TYPE_BVEC4, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"ivec4", TYPE_IVEC4, {TYPE_IVEC4, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"ivec4", TYPE_IVEC4, {TYPE_UVEC4, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"ivec4", TYPE_IVEC4, {TYPE_VEC4, TYPE_VOID}, {""}, TAG_GLOBAL, false},

	{"vec4", TYPE_VEC4, {TYPE_BVEC4, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"vec4", TYPE_VEC4, {TYPE_IVEC4, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"vec4", TYPE_VEC4, {TYPE_UVEC4, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"vec4", TYPE_VEC4, {TYPE_VEC4, TYPE_VOID}, {""}, TAG_GLOBAL, false},

	{"uvec4", TYPE_UVEC4, {TYPE_BVEC4, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"uvec4", TYPE_UVEC4, {TYPE_IVEC4, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"uvec4", TYPE_UVEC4, {TYPE_UVEC4, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"uvec4", TYPE_UVEC4, {TYPE_VEC4, TYPE_VOID}, {""}, TAG_GLOBAL, false},

	{"bvec4", TYPE_BVEC4, {TYPE_BVEC4, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"bvec4", TYPE_BVEC4, {TYPE_IVEC4, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"bvec4", TYPE_BVEC4, {TYPE_UVEC4, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"bvec4", TYPE_BVEC4, {TYPE_VEC4, TYPE_VOID}, {""}, TAG_GLOBAL, false},

	// Conversion between matrixes.

	{"mat2", TYPE_MAT2, {TYPE_MAT3, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"mat2", TYPE_MAT2, {TYPE_MAT4, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"mat3", TYPE_MAT3, {TYPE_MAT2, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"mat3", TYPE_MAT3, {TYPE_MAT4, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"mat4", TYPE_MAT4, {TYPE_MAT2, TYPE_VOID}, {""}, TAG_GLOBAL, false},
	{"mat4", TYPE_MAT4, {TYPE_MAT3, TYPE_VOID}, {""}, TAG_GLOBAL, false},

	// Built-ins - trigonometric functions.
	// radians

	{"radians", TYPE_FLOAT, {TYPE_FLOAT, TYPE_VOID}, {"degrees"}, TAG_GLOBAL, false},
	{"radians", TYPE_VEC2, {TYPE_VEC2, TYPE_VOID}, {"degrees"}, TAG_GLOBAL, false},
	{"radians", TYPE_VEC3, {TYPE_VEC3, TYPE_VOID}, {"degrees"}, TAG_GLOBAL, false},
	{"radians", TYPE_VEC4, {TYPE_VEC4, TYPE_VOID}, {"degrees"}, TAG_GLOBAL, false},

	// degrees

	{"degrees", TYPE_FLOAT, {TYPE_FLOAT, TYPE_VOID}, {"radians"}, TAG_GLOBAL, false},
	{"degrees", TYPE_VEC2, {TYPE_VEC2, TYPE_VOID}, {"radians"}, TAG_GLOBAL, false},
	{"degrees", TYPE_VEC3, {TYPE_VEC3, TYPE_VOID}, {"radians"}, TAG_GLOBAL, false},
	{"degrees", TYPE_VEC4, {TYPE_VEC4, TYPE_VOID}, {"radians"}, TAG_GLOBAL, false},

	// sin

	{"sin", TYPE_FLOAT, {TYPE_FLOAT, TYPE_VOID}, {"angle"}, TAG_GLOBAL, false},
	{"sin", TYPE_VEC2, {TYPE_VEC2, TYPE_VOID}, {"angle"}, TAG_GLOBAL, false},
	{"sin", TYPE_VEC3, {TYPE_VEC3, TYPE_VOID}, {"angle"}, TAG_GLOBAL, false},
	{"sin", TYPE_VEC4, {TYPE_VEC4, TYPE_VOID}, {"angle"}, TAG_GLOBAL, false},

	// cos

	{"cos", TYPE_FLOAT, {TYPE_FLOAT, TYPE_VOID}, {"angle"}, TAG_GLOBAL, false},
	{"cos", TYPE_VEC2, {TYPE_VEC2, TYPE_VOID}, {"angle"}, TAG_GLOBAL, false},
	{"cos", TYPE_VEC3, {TYPE_VEC3, TYPE_VOID}, {"angle"}, TAG_GLOBAL, false},
	{"cos", TYPE_VEC4, {TYPE_VEC4, TYPE_VOID}, {"angle"}, TAG_GLOBAL, false},

	// tan

	{"tan", TYPE_FLOAT, {TYPE_FLOAT, TYPE_VOID}, {"angle"}, TAG_GLOBAL, false},
	{"tan", TYPE_VEC2, {TYPE_VEC2, TYPE_VOID}, {"angle"}, TAG_GLOBAL, false},
	{"tan", TYPE_VEC3, {TYPE_VEC3, TYPE_VOID}, {"angle"}, TAG_GLOBAL, false},
	{"tan", TYPE_VEC4, {TYPE_VEC4, TYPE_VOID}, {"angle"}, TAG_GLOBAL, false},

	// asin

	{"asin", TYPE_FLOAT, {TYPE_FLOAT, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"asin", TYPE_VEC2, {TYPE_VEC2, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"asin", TYPE_VEC3, {TYPE_VEC3, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"asin", TYPE_VEC4, {TYPE_VEC4, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},

	// acos

	{"acos", TYPE_FLOAT, {TYPE_FLOAT, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"acos", TYPE_VEC2, {TYPE_VEC2, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"acos", TYPE_VEC3, {TYPE_VEC3, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"acos", TYPE_VEC4, {TYPE_VEC4, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},

	// atan

	{"atan", TYPE_FLOAT, {TYPE_FLOAT, TYPE_VOID}, {"y_over_x"}, TAG_GLOBAL, false},
	{"atan", TYPE_VEC2, {TYPE_VEC2, TYPE_VOID}, {"y_over_x"}, TAG_GLOBAL, false},
	{"atan", TYPE_VEC3, {TYPE_VEC3, TYPE_VOID}, {"y_over_x"}, TAG_GLOBAL, false},
	{"atan", TYPE_VEC4, {TYPE_VEC4, TYPE_VOID}, {"y_over_x"}, TAG_GLOBAL, false},
	{"atan", TYPE_FLOAT, {TYPE_FLOAT, TYPE_FLOAT, TYPE_VOID}, {"y", "x"}, TAG_GLOBAL, false},
	{"atan", TYPE_VEC2, {TYPE_VEC2, TYPE_VEC2, TYPE_VOID}, {"y", "x"}, TAG_GLOBAL, false},
	{"atan", TYPE_VEC3, {TYPE_VEC3, TYPE_VEC3, TYPE_VOID}, {"y", "x"}, TAG_GLOBAL, false},
	{"atan", TYPE_VEC4, {TYPE_VEC4, TYPE_VEC4, TYPE_VOID}, {"y", "x"}, TAG_GLOBAL, false},

	// sinh

	{"sinh", TYPE_FLOAT, {TYPE_FLOAT, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"sinh", TYPE_VEC2, {TYPE_VEC2, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"sinh", TYPE_VEC3, {TYPE_VEC3, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"sinh", TYPE_VEC4, {TYPE_VEC4, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},

	// cosh

	{"cosh", TYPE_FLOAT, {TYPE_FLOAT, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"cosh", TYPE_VEC2, {TYPE_VEC2, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"cosh", TYPE_VEC3, {TYPE_VEC3, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"cosh", TYPE_VEC4, {TYPE_VEC4, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},

	// tanh

	{"tanh", TYPE_FLOAT, {TYPE_FLOAT, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"tanh", TYPE_VEC2, {TYPE_VEC2, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"tanh", TYPE_VEC3, {TYPE_VEC3, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"tanh", TYPE_VEC4, {TYPE_VEC4, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},

	// asinh

	{"asinh", TYPE_FLOAT, {TYPE_FLOAT, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"asinh", TYPE_VEC2, {TYPE_VEC2, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"asinh", TYPE_VEC3, {TYPE_VEC3, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"asinh", TYPE_VEC4, {TYPE_VEC4, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},

	// acosh

	{"acosh", TYPE_FLOAT, {TYPE_FLOAT, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"acosh", TYPE_VEC2, {TYPE_VEC2, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"acosh", TYPE_VEC3, {TYPE_VEC3, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"acosh", TYPE_VEC4, {TYPE_VEC4, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},

	// atanh

	{"atanh", TYPE_FLOAT, {TYPE_FLOAT, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"atanh", TYPE_VEC2, {TYPE_VEC2, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"atanh", TYPE_VEC3, {TYPE_VEC3, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"atanh", TYPE_VEC4, {TYPE_VEC4, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},

	// Builtins - exponential functions.
	// pow

	{"pow", TYPE_FLOAT, {TYPE_FLOAT, TYPE_FLOAT, TYPE_VOID}, {"x", "y"}, TAG_GLOBAL, false},
	{"pow", TYPE_VEC2, {TYPE_VEC2, TYPE_VEC2, TYPE_VOID}, {"x", "y"}, TAG_GLOBAL, false},
	{"pow", TYPE_VEC3, {TYPE_VEC3, TYPE_VEC3, TYPE_VOID}, {"x", "y"}, TAG_GLOBAL, false},
	{"pow", TYPE_VEC4, {TYPE_VEC4, TYPE_VEC4, TYPE_VOID}, {"x", "y"}, TAG_GLOBAL, false},

	// exp

	{"exp", TYPE_FLOAT, {TYPE_FLOAT, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"exp", TYPE_VEC2, {TYPE_VEC2, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"exp", TYPE_VEC3, {TYPE_VEC3, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"exp", TYPE_VEC4, {TYPE_VEC4, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},

	// log

	{"log", TYPE_FLOAT, {TYPE_FLOAT, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"log", TYPE_VEC2, {TYPE_VEC2, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"log", TYPE_VEC3, {TYPE_VEC3, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"log", TYPE_VEC4, {TYPE_VEC4, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},

	// exp2

	{"exp2", TYPE_FLOAT, {TYPE_FLOAT, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"exp2", TYPE_VEC2, {TYPE_VEC2, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"exp2", TYPE_VEC3, {TYPE_VEC3, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"exp2", TYPE_VEC4, {TYPE_VEC4, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},

	// log2

	{"log2", TYPE_FLOAT, {TYPE_FLOAT, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"log2", TYPE_VEC2, {TYPE_VEC2, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"log2", TYPE_VEC3, {TYPE_VEC3, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"log2", TYPE_VEC4, {TYPE_VEC4, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},

	// sqrt

	{"sqrt", TYPE_FLOAT, {TYPE_FLOAT, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"sqrt", TYPE_VEC2, {TYPE_VEC2, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"sqrt", TYPE_VEC3, {TYPE_VEC3, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"sqrt", TYPE_VEC4, {TYPE_VEC4, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},

	// inversesqrt

	{"inversesqrt", TYPE_FLOAT, {TYPE_FLOAT, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"inversesqrt", TYPE_VEC2, {TYPE_VEC2, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"inversesqrt", TYPE_VEC3, {TYPE_VEC3, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"inversesqrt", TYPE_VEC4, {TYPE_VEC4, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},

	// Built-ins - common functions.
	// abs

	{"abs", TYPE_FLOAT, {TYPE_FLOAT, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"abs", TYPE_VEC2, {TYPE_VEC2, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"abs", TYPE_VEC3, {TYPE_VEC3, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"abs", TYPE_VEC4, {TYPE_VEC4, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},

	{"abs", TYPE_INT, {TYPE_INT, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"abs", TYPE_IVEC2, {TYPE_IVEC2, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"abs", TYPE_IVEC3, {TYPE_IVEC3, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"abs", TYPE_IVEC4, {TYPE_IVEC4, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},

	// sign

	{"sign", TYPE_FLOAT, {TYPE_FLOAT, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"sign", TYPE_VEC2, {TYPE_VEC2, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"sign", TYPE_VEC3, {TYPE_VEC3, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"sign", TYPE_VEC4, {TYPE_VEC4, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},

	{"sign", TYPE_INT, {TYPE_INT, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"sign", TYPE_IVEC2, {TYPE_IVEC2, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"sign", TYPE_IVEC3, {TYPE_IVEC3, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"sign", TYPE_IVEC4, {TYPE_IVEC4, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},

	// floor

	{"floor", TYPE_FLOAT, {TYPE_FLOAT, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"floor", TYPE_VEC2, {TYPE_VEC2, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"floor", TYPE_VEC3, {TYPE_VEC3, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"floor", TYPE_VEC4, {TYPE_VEC4, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},

	// trunc

	{"trunc", TYPE_FLOAT, {TYPE_FLOAT, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"trunc", TYPE_VEC2, {TYPE_VEC2, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"trunc", TYPE_VEC3, {TYPE_VEC3, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"trunc", TYPE_VEC4, {TYPE_VEC4, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},

	// round

	{"round", TYPE_FLOAT, {TYPE_FLOAT, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"round", TYPE_VEC2, {TYPE_VEC2, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"round", TYPE_VEC3, {TYPE_VEC3, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"round", TYPE_VEC4, {TYPE_VEC4, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},

	// roundEven

	{"roundEven", TYPE_FLOAT, {TYPE_FLOAT, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"roundEven", TYPE_VEC2, {TYPE_VEC2, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"roundEven", TYPE_VEC3, {TYPE_VEC3, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"roundEven", TYPE_VEC4, {TYPE_VEC4, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},

	// ceil

	{"ceil", TYPE_FLOAT, {TYPE_FLOAT, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"ceil", TYPE_VEC2, {TYPE_VEC2, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"ceil", TYPE_VEC3, {TYPE_VEC3, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"ceil", TYPE_VEC4, {TYPE_VEC4, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},

	// fract

	{"fract", TYPE_FLOAT, {TYPE_FLOAT, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"fract", TYPE_VEC2, {TYPE_VEC2, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"fract", TYPE_VEC3, {TYPE_VEC3, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"fract", TYPE_VEC4, {TYPE_VEC4, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},

	// mod

	{"mod", TYPE_FLOAT, {TYPE_FLOAT, TYPE_FLOAT, TYPE_VOID}, {"x", "y"}, TAG_GLOBAL, false},
	{"mod", TYPE_VEC2, {TYPE_VEC2, TYPE_VEC2, TYPE_VOID}, {"x", "y"}, TAG_GLOBAL, false},
	{"mod", TYPE_VEC2, {TYPE_VEC2, TYPE_FLOAT, TYPE_VOID}, {"x", "y"}, TAG_GLOBAL, false},
	{"mod", TYPE_VEC3, {TYPE_VEC3, TYPE_VEC3, TYPE_VOID}, {"x", "y"}, TAG_GLOBAL, false},
	{"mod", TYPE_VEC3, {TYPE_VEC3, TYPE_FLOAT, TYPE_VOID}, {"x", "y"}, TAG_GLOBAL, false},
	{"mod", TYPE_VEC4, {TYPE_VEC4, TYPE_VEC4, TYPE_VOID}, {"x", "y"}, TAG_GLOBAL, false},
	{"mod", TYPE_VEC4, {TYPE_VEC4, TYPE_FLOAT, TYPE_VOID}, {"x", "y"}, TAG_GLOBAL, false},

	// modf

	{"modf", TYPE_FLOAT, {TYPE_FLOAT, TYPE_FLOAT, TYPE_VOID}, {"x", "i"}, TAG_GLOBAL, false},
	{"modf", TYPE_VEC2, {TYPE_VEC2, TYPE_VEC2, TYPE_VOID}, {"x", "i"}, TAG_GLOBAL, false},
	{"modf", TYPE_VEC3, {TYPE_VEC3, TYPE_VEC3, TYPE_VOID}, {"x", "i"}, TAG_GLOBAL, false},
	{"modf", TYPE_VEC4, {TYPE_VEC4, TYPE_VEC4, TYPE_VOID}, {"x", "i"}, TAG_GLOBAL, false},

	// min

	{"min", TYPE_FLOAT, {TYPE_FLOAT, TYPE_FLOAT, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"min", TYPE_VEC2, {TYPE_VEC2, TYPE_VEC2, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"min", TYPE_VEC2, {TYPE_VEC2, TYPE_FLOAT, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"min", TYPE_VEC3, {TYPE_VEC3, TYPE_VEC3, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"min", TYPE_VEC3, {TYPE_VEC3, TYPE_FLOAT, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"min", TYPE_VEC4, {TYPE_VEC4, TYPE_VEC4, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"min", TYPE_VEC4, {TYPE_VEC4, TYPE_FLOAT, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},

	{"min", TYPE_INT, {TYPE_INT, TYPE_INT, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"min", TYPE_IVEC2, {TYPE_IVEC2, TYPE_IVEC2, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"min", TYPE_IVEC2, {TYPE_IVEC2, TYPE_INT, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"min", TYPE_IVEC3, {TYPE_IVEC3, TYPE_IVEC3, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"min", TYPE_IVEC3, {TYPE_IVEC3, TYPE_INT, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"min", TYPE_IVEC4, {TYPE_IVEC4, TYPE_IVEC4, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"min", TYPE_IVEC4, {TYPE_IVEC4, TYPE_INT, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},

	{"min", TYPE_UINT, {TYPE_UINT, TYPE_UINT, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"min", TYPE_UVEC2, {TYPE_UVEC2, TYPE_UVEC2, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"min", TYPE_UVEC2, {TYPE_UVEC2, TYPE_UINT, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"min", TYPE_UVEC3, {TYPE_UVEC3, TYPE_UVEC3, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"min", TYPE_UVEC3, {TYPE_UVEC3, TYPE_UINT, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"min", TYPE_UVEC4, {TYPE_UVEC4, TYPE_UVEC4, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"min", TYPE_UVEC4, {TYPE_UVEC4, TYPE_UINT, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},

	// max

	{"max", TYPE_FLOAT, {TYPE_FLOAT, TYPE_FLOAT, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"max", TYPE_VEC2, {TYPE_VEC2, TYPE_VEC2, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"max", TYPE_VEC2, {TYPE_VEC2, TYPE_FLOAT, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"max", TYPE_VEC3, {TYPE_VEC3, TYPE_VEC3, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"max", TYPE_VEC3, {TYPE_VEC3, TYPE_FLOAT, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"max", TYPE_VEC4, {TYPE_VEC4, TYPE_VEC4, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"max", TYPE_VEC4, {TYPE_VEC4, TYPE_FLOAT, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},

	{"max", TYPE_INT, {TYPE_INT, TYPE_INT, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"max", TYPE_IVEC2, {TYPE_IVEC2, TYPE_IVEC2, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"max", TYPE_IVEC2, {TYPE_IVEC2, TYPE_INT, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"max", TYPE_IVEC3, {TYPE_IVEC3, TYPE_IVEC3, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"max", TYPE_IVEC3, {TYPE_IVEC3, TYPE_INT, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"max", TYPE_IVEC4, {TYPE_IVEC4, TYPE_IVEC4, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"max", TYPE_IVEC4, {TYPE_IVEC4, TYPE_INT, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},

	{"max", TYPE_UINT, {TYPE_UINT, TYPE_UINT, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"max", TYPE_UVEC2, {TYPE_UVEC2, TYPE_UVEC2, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"max", TYPE_UVEC2, {TYPE_UVEC2, TYPE_UINT, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"max", TYPE_UVEC3, {TYPE_UVEC3, TYPE_UVEC3, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"max", TYPE_UVEC3, {TYPE_UVEC3, TYPE_UINT, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"max", TYPE_UVEC4, {TYPE_UVEC4, TYPE_UVEC4, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"max", TYPE_UVEC4, {TYPE_UVEC4, TYPE_UINT, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},

	// clamp

	{"clamp", TYPE_FLOAT, {TYPE_FLOAT, TYPE_FLOAT, TYPE_FLOAT, TYPE_VOID},
		{"x", "minVal", "maxVal"}, TAG_GLOBAL, false},
	{"clamp", TYPE_VEC2, {TYPE_VEC2, TYPE_VEC2, TYPE_VEC2, TYPE_VOID}, {"x", "minVal", "maxVal"},
		TAG_GLOBAL, false},
	{"clamp", TYPE_VEC3, {TYPE_VEC3, TYPE_VEC3, TYPE_VEC3, TYPE_VOID}, {"x", "minVal", "maxVal"},
		TAG_GLOBAL, false},
	{"clamp", TYPE_VEC4, {TYPE_VEC4, TYPE_VEC4, TYPE_VEC4, TYPE_VOID}, {"x", "minVal", "maxVal"},
		TAG_GLOBAL, false},
	{"clamp", TYPE_VEC2, {TYPE_VEC2, TYPE_FLOAT, TYPE_FLOAT, TYPE_VOID}, {"x", "minVal", "maxVal"},
		TAG_GLOBAL, false},
	{"clamp", TYPE_VEC3, {TYPE_VEC3, TYPE_FLOAT, TYPE_FLOAT, TYPE_VOID}, {"x", "minVal", "maxVal"},
		TAG_GLOBAL, false},
	{"clamp", TYPE_VEC4, {TYPE_VEC4, TYPE_FLOAT, TYPE_FLOAT, TYPE_VOID}, {"x", "minVal", "maxVal"},
		TAG_GLOBAL, false},

	{"clamp", TYPE_INT, {TYPE_INT, TYPE_INT, TYPE_INT, TYPE_VOID}, {"x", "minVal", "maxVal"},
		TAG_GLOBAL, false},
	{"clamp", TYPE_IVEC2, {TYPE_IVEC2, TYPE_IVEC2, TYPE_IVEC2, TYPE_VOID},
		{"x", "minVal", "maxVal"}, TAG_GLOBAL, false},
	{"clamp", TYPE_IVEC3, {TYPE_IVEC3, TYPE_IVEC3, TYPE_IVEC3, TYPE_VOID},
		{"x", "minVal", "maxVal"}, TAG_GLOBAL, false},
	{"clamp", TYPE_IVEC4, {TYPE_IVEC4, TYPE_IVEC4, TYPE_IVEC4, TYPE_VOID},
		{"x", "minVal", "maxVal"}, TAG_GLOBAL, false},
	{"clamp", TYPE_IVEC2, {TYPE_IVEC2, TYPE_INT, TYPE_INT, TYPE_VOID}, {"x", "minVal", "maxVal"},
		TAG_GLOBAL, false},
	{"clamp", TYPE_IVEC3, {TYPE_IVEC3, TYPE_INT, TYPE_INT, TYPE_VOID}, {"x", "minVal", "maxVal"},
		TAG_GLOBAL, false},
	{"clamp", TYPE_IVEC4, {TYPE_IVEC4, TYPE_INT, TYPE_INT, TYPE_VOID}, {"x", "minVal", "maxVal"},
		TAG_GLOBAL, false},

	{"clamp", TYPE_UINT, {TYPE_UINT, TYPE_UINT, TYPE_UINT, TYPE_VOID}, {"x", "minVal", "maxVal"},
		TAG_GLOBAL, false},
	{"clamp", TYPE_UVEC2, {TYPE_UVEC2, TYPE_UVEC2, TYPE_UVEC2, TYPE_VOID},
		{"x", "minVal", "maxVal"}, TAG_GLOBAL, false},
	{"clamp", TYPE_UVEC3, {TYPE_UVEC3, TYPE_UVEC3, TYPE_UVEC3, TYPE_VOID},
		{"x", "minVal", "maxVal"}, TAG_GLOBAL, false},
	{"clamp", TYPE_UVEC4, {TYPE_UVEC4, TYPE_UVEC4, TYPE_UVEC4, TYPE_VOID},
		{"x", "minVal", "maxVal"}, TAG_GLOBAL, false},
	{"clamp", TYPE_UVEC2, {TYPE_UVEC2, TYPE_UINT, TYPE_UINT, TYPE_VOID}, {"x", "minVal", "maxVal"},
		TAG_GLOBAL, false},
	{"clamp", TYPE_UVEC3, {TYPE_UVEC3, TYPE_UINT, TYPE_UINT, TYPE_VOID}, {"x", "minVal", "maxVal"},
		TAG_GLOBAL, false},
	{"clamp", TYPE_UVEC4, {TYPE_UVEC4, TYPE_UINT, TYPE_UINT, TYPE_VOID}, {"x", "minVal", "maxVal"},
		TAG_GLOBAL, false},

	// mix

	{"mix", TYPE_FLOAT, {TYPE_FLOAT, TYPE_FLOAT, TYPE_FLOAT, TYPE_VOID}, {"a", "b", "value"},
		TAG_GLOBAL, false},
	{"mix", TYPE_VEC2, {TYPE_VEC2, TYPE_VEC2, TYPE_FLOAT, TYPE_VOID}, {"a", "b", "value"},
		TAG_GLOBAL, false},
	{"mix", TYPE_VEC2, {TYPE_VEC2, TYPE_VEC2, TYPE_BVEC2, TYPE_VOID}, {"a", "b", "value"},
		TAG_GLOBAL, false},
	{"mix", TYPE_VEC2, {TYPE_VEC2, TYPE_VEC2, TYPE_VEC2, TYPE_VOID}, {"a", "b", "value"},
		TAG_GLOBAL, false},
	{"mix", TYPE_VEC3, {TYPE_VEC3, TYPE_VEC3, TYPE_FLOAT, TYPE_VOID}, {"a", "b", "value"},
		TAG_GLOBAL, false},
	{"mix", TYPE_VEC3, {TYPE_VEC3, TYPE_VEC3, TYPE_BVEC3, TYPE_VOID}, {"a", "b", "value"},
		TAG_GLOBAL, false},
	{"mix", TYPE_VEC3, {TYPE_VEC3, TYPE_VEC3, TYPE_VEC3, TYPE_VOID}, {"a", "b", "value"},
		TAG_GLOBAL, false},
	{"mix", TYPE_VEC4, {TYPE_VEC4, TYPE_VEC4, TYPE_FLOAT, TYPE_VOID}, {"a", "b", "value"},
		TAG_GLOBAL, false},
	{"mix", TYPE_VEC4, {TYPE_VEC4, TYPE_VEC4, TYPE_BVEC4, TYPE_VOID}, {"a", "b", "value"},
		TAG_GLOBAL, false},
	{"mix", TYPE_VEC4, {TYPE_VEC4, TYPE_VEC4, TYPE_VEC4, TYPE_VOID}, {"a", "b", "value"},
		TAG_GLOBAL, false},

	// step

	{"step", TYPE_FLOAT, {TYPE_FLOAT, TYPE_FLOAT, TYPE_VOID}, {"edge", "x"}, TAG_GLOBAL, false},
	{"step", TYPE_VEC2, {TYPE_VEC2, TYPE_VEC2, TYPE_VOID}, {"edge", "x"}, TAG_GLOBAL, false},
	{"step", TYPE_VEC3, {TYPE_VEC3, TYPE_VEC3, TYPE_VOID}, {"edge", "x"}, TAG_GLOBAL, false},
	{"step", TYPE_VEC4, {TYPE_VEC4, TYPE_VEC4, TYPE_VOID}, {"edge", "x"}, TAG_GLOBAL, false},
	{"step", TYPE_VEC2, {TYPE_FLOAT, TYPE_VEC2, TYPE_VOID}, {"edge", "x"}, TAG_GLOBAL, false},
	{"step", TYPE_VEC3, {TYPE_FLOAT, TYPE_VEC3, TYPE_VOID}, {"edge", "x"}, TAG_GLOBAL, false},
	{"step", TYPE_VEC4, {TYPE_FLOAT, TYPE_VEC4, TYPE_VOID}, {"edge", "x"}, TAG_GLOBAL, false},

	// smoothstep

	{"smoothstep", TYPE_FLOAT, {TYPE_FLOAT, TYPE_FLOAT, TYPE_FLOAT, TYPE_VOID},
		{"edge0", "edge1", "value"}, TAG_GLOBAL, false},
	{"smoothstep", TYPE_VEC2, {TYPE_VEC2, TYPE_VEC2, TYPE_VEC2, TYPE_VOID},
		{"edge0", "edge1", "value"}, TAG_GLOBAL, false},
	{"smoothstep", TYPE_VEC3, {TYPE_VEC3, TYPE_VEC3, TYPE_VEC3, TYPE_VOID},
		{"edge0", "edge1", "value"}, TAG_GLOBAL, false},
	{"smoothstep", TYPE_VEC4, {TYPE_VEC4, TYPE_VEC4, TYPE_VEC4, TYPE_VOID},
		{"edge0", "edge1", "value"}, TAG_GLOBAL, false},
	{"smoothstep", TYPE_VEC2, {TYPE_FLOAT, TYPE_FLOAT, TYPE_VEC2, TYPE_VOID},
		{"edge0", "edge1", "value"}, TAG_GLOBAL, false},
	{"smoothstep", TYPE_VEC3, {TYPE_FLOAT, TYPE_FLOAT, TYPE_VEC3, TYPE_VOID},
		{"edge0", "edge1", "value"}, TAG_GLOBAL, false},
	{"smoothstep", TYPE_VEC4, {TYPE_FLOAT, TYPE_FLOAT, TYPE_VEC4, TYPE_VOID},
		{"edge0", "edge1", "value"}, TAG_GLOBAL, false},

	// isnan

	{"isnan", TYPE_BOOL, {TYPE_FLOAT, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"isnan", TYPE_BVEC2, {TYPE_VEC2, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"isnan", TYPE_BVEC3, {TYPE_VEC3, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"isnan", TYPE_BVEC4, {TYPE_VEC4, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},

	// isinf

	{"isinf", TYPE_BOOL, {TYPE_FLOAT, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"isinf", TYPE_BVEC2, {TYPE_VEC2, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"isinf", TYPE_BVEC3, {TYPE_VEC3, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"isinf", TYPE_BVEC4, {TYPE_VEC4, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},

	// floatBitsToInt

	{"floatBitsToInt", TYPE_INT, {TYPE_FLOAT, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"floatBitsToInt", TYPE_IVEC2, {TYPE_VEC2, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"floatBitsToInt", TYPE_IVEC3, {TYPE_VEC3, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"floatBitsToInt", TYPE_IVEC4, {TYPE_VEC4, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},

	// floatBitsToUint

	{"floatBitsToUint", TYPE_UINT, {TYPE_FLOAT, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"floatBitsToUint", TYPE_UVEC2, {TYPE_VEC2, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"floatBitsToUint", TYPE_UVEC3, {TYPE_VEC3, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"floatBitsToUint", TYPE_UVEC4, {TYPE_VEC4, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},

	// intBitsToFloat

	{"intBitsToFloat", TYPE_FLOAT, {TYPE_INT, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"intBitsToFloat", TYPE_VEC2, {TYPE_IVEC2, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"intBitsToFloat", TYPE_VEC3, {TYPE_IVEC3, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"intBitsToFloat", TYPE_VEC4, {TYPE_IVEC4, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},

	// uintBitsToFloat

	{"uintBitsToFloat", TYPE_FLOAT, {TYPE_UINT, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"uintBitsToFloat", TYPE_VEC2, {TYPE_UVEC2, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"uintBitsToFloat", TYPE_VEC3, {TYPE_UVEC3, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"uintBitsToFloat", TYPE_VEC4, {TYPE_UVEC4, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},

	// Built-ins - geometric functions.
	// length

	{"length", TYPE_FLOAT, {TYPE_FLOAT, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"length", TYPE_FLOAT, {TYPE_VEC2, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"length", TYPE_FLOAT, {TYPE_VEC3, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"length", TYPE_FLOAT, {TYPE_VEC4, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},

	// distance

	{"distance", TYPE_FLOAT, {TYPE_FLOAT, TYPE_FLOAT, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"distance", TYPE_FLOAT, {TYPE_VEC2, TYPE_VEC2, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"distance", TYPE_FLOAT, {TYPE_VEC3, TYPE_VEC3, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"distance", TYPE_FLOAT, {TYPE_VEC4, TYPE_VEC4, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},

	// dot

	{"dot", TYPE_FLOAT, {TYPE_FLOAT, TYPE_FLOAT, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"dot", TYPE_FLOAT, {TYPE_VEC2, TYPE_VEC2, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"dot", TYPE_FLOAT, {TYPE_VEC3, TYPE_VEC3, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"dot", TYPE_FLOAT, {TYPE_VEC4, TYPE_VEC4, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},

	// cross

	{"cross", TYPE_VEC3, {TYPE_VEC3, TYPE_VEC3, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},

	// normalize

	{"normalize", TYPE_FLOAT, {TYPE_FLOAT, TYPE_VOID}, {"v"}, TAG_GLOBAL, false},
	{"normalize", TYPE_VEC2, {TYPE_VEC2, TYPE_VOID}, {"v"}, TAG_GLOBAL, false},
	{"normalize", TYPE_VEC3, {TYPE_VEC3, TYPE_VOID}, {"v"}, TAG_GLOBAL, false},
	{"normalize", TYPE_VEC4, {TYPE_VEC4, TYPE_VOID}, {"v"}, TAG_GLOBAL, false},

	// reflect

	{"reflect", TYPE_VEC2, {TYPE_VEC2, TYPE_VEC2, TYPE_VOID}, {"I", "N"}, TAG_GLOBAL, false},
	{"reflect", TYPE_VEC3, {TYPE_VEC3, TYPE_VEC3, TYPE_VOID}, {"I", "N"}, TAG_GLOBAL, false},
	{"reflect", TYPE_VEC4, {TYPE_VEC4, TYPE_VEC4, TYPE_VOID}, {"I", "N"}, TAG_GLOBAL, false},

	// refract

	{"refract", TYPE_VEC2, {TYPE_VEC2, TYPE_VEC2, TYPE_FLOAT, TYPE_VOID}, {"I", "N", "eta"},
		TAG_GLOBAL, false},
	{"refract", TYPE_VEC3, {TYPE_VEC3, TYPE_VEC3, TYPE_FLOAT, TYPE_VOID}, {"I", "N", "eta"},
		TAG_GLOBAL, false},
	{"refract", TYPE_VEC4, {TYPE_VEC4, TYPE_VEC4, TYPE_FLOAT, TYPE_VOID}, {"I", "N", "eta"},
		TAG_GLOBAL, false},

	// faceforward

	{"faceforward", TYPE_VEC2, {TYPE_VEC2, TYPE_VEC2, TYPE_VEC2, TYPE_VOID}, {"N", "I", "Nref"},
		TAG_GLOBAL, false},
	{"faceforward", TYPE_VEC3, {TYPE_VEC3, TYPE_VEC3, TYPE_VEC3, TYPE_VOID}, {"N", "I", "Nref"},
		TAG_GLOBAL, false},
	{"faceforward", TYPE_VEC4, {TYPE_VEC4, TYPE_VEC4, TYPE_VEC4, TYPE_VOID}, {"N", "I", "Nref"},
		TAG_GLOBAL, false},

	// matrixCompMult

	{"matrixCompMult", TYPE_MAT2, {TYPE_MAT2, TYPE_MAT2, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"matrixCompMult", TYPE_MAT3, {TYPE_MAT3, TYPE_MAT3, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"matrixCompMult", TYPE_MAT4, {TYPE_MAT4, TYPE_MAT4, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},

	// outerProduct

	{"outerProduct", TYPE_MAT2, {TYPE_VEC2, TYPE_VEC2, TYPE_VOID}, {"c", "r"}, TAG_GLOBAL, false},
	{"outerProduct", TYPE_MAT3, {TYPE_VEC3, TYPE_VEC3, TYPE_VOID}, {"c", "r"}, TAG_GLOBAL, false},
	{"outerProduct", TYPE_MAT4, {TYPE_VEC4, TYPE_VEC4, TYPE_VOID}, {"c", "r"}, TAG_GLOBAL, false},

	// transpose

	{"transpose", TYPE_MAT2, {TYPE_MAT2, TYPE_VOID}, {"m"}, TAG_GLOBAL, false},
	{"transpose", TYPE_MAT3, {TYPE_MAT3, TYPE_VOID}, {"m"}, TAG_GLOBAL, false},
	{"transpose", TYPE_MAT4, {TYPE_MAT4, TYPE_VOID}, {"m"}, TAG_GLOBAL, false},

	// determinant

	{"determinant", TYPE_FLOAT, {TYPE_MAT2, TYPE_VOID}, {"m"}, TAG_GLOBAL, false},
	{"determinant", TYPE_FLOAT, {TYPE_MAT3, TYPE_VOID}, {"m"}, TAG_GLOBAL, false},
	{"determinant", TYPE_FLOAT, {TYPE_MAT4, TYPE_VOID}, {"m"}, TAG_GLOBAL, false},

	// inverse

	{"inverse", TYPE_MAT2, {TYPE_MAT2, TYPE_VOID}, {"m"}, TAG_GLOBAL, false},
	{"inverse", TYPE_MAT3, {TYPE_MAT3, TYPE_VOID}, {"m"}, TAG_GLOBAL, false},
	{"inverse", TYPE_MAT4, {TYPE_MAT4, TYPE_VOID}, {"m"}, TAG_GLOBAL, false},

	// lessThan

	{"lessThan", TYPE_BVEC2, {TYPE_VEC2, TYPE_VEC2, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"lessThan", TYPE_BVEC3, {TYPE_VEC3, TYPE_VEC3, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"lessThan", TYPE_BVEC4, {TYPE_VEC4, TYPE_VEC4, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},

	{"lessThan", TYPE_BVEC2, {TYPE_IVEC2, TYPE_IVEC2, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"lessThan", TYPE_BVEC3, {TYPE_IVEC3, TYPE_IVEC3, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"lessThan", TYPE_BVEC4, {TYPE_IVEC4, TYPE_IVEC4, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},

	{"lessThan", TYPE_BVEC3, {TYPE_UVEC3, TYPE_UVEC3, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"lessThan", TYPE_BVEC4, {TYPE_UVEC4, TYPE_UVEC4, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"lessThan", TYPE_BVEC2, {TYPE_UVEC2, TYPE_UVEC2, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},

	// greaterThan

	{"greaterThan", TYPE_BVEC2, {TYPE_VEC2, TYPE_VEC2, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"greaterThan", TYPE_BVEC3, {TYPE_VEC3, TYPE_VEC3, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"greaterThan", TYPE_BVEC4, {TYPE_VEC4, TYPE_VEC4, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},

	{"greaterThan", TYPE_BVEC2, {TYPE_IVEC2, TYPE_IVEC2, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"greaterThan", TYPE_BVEC3, {TYPE_IVEC3, TYPE_IVEC3, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"greaterThan", TYPE_BVEC4, {TYPE_IVEC4, TYPE_IVEC4, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},

	{"greaterThan", TYPE_BVEC2, {TYPE_UVEC2, TYPE_UVEC2, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"greaterThan", TYPE_BVEC3, {TYPE_UVEC3, TYPE_UVEC3, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"greaterThan", TYPE_BVEC4, {TYPE_UVEC4, TYPE_UVEC4, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},

	// lessThanEqual

	{"lessThanEqual", TYPE_BVEC2, {TYPE_VEC2, TYPE_VEC2, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"lessThanEqual", TYPE_BVEC3, {TYPE_VEC3, TYPE_VEC3, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"lessThanEqual", TYPE_BVEC4, {TYPE_VEC4, TYPE_VEC4, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},

	{"lessThanEqual", TYPE_BVEC2, {TYPE_IVEC2, TYPE_IVEC2, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL,
		false},
	{"lessThanEqual", TYPE_BVEC3, {TYPE_IVEC3, TYPE_IVEC3, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL,
		false},
	{"lessThanEqual", TYPE_BVEC4, {TYPE_IVEC4, TYPE_IVEC4, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL,
		false},

	{"lessThanEqual", TYPE_BVEC2, {TYPE_UVEC2, TYPE_UVEC2, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL,
		false},
	{"lessThanEqual", TYPE_BVEC3, {TYPE_UVEC3, TYPE_UVEC3, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL,
		false},
	{"lessThanEqual", TYPE_BVEC4, {TYPE_UVEC4, TYPE_UVEC4, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL,
		false},

	// greaterThanEqual

	{"greaterThanEqual", TYPE_BVEC2, {TYPE_VEC2, TYPE_VEC2, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL,
		false},
	{"greaterThanEqual", TYPE_BVEC3, {TYPE_VEC3, TYPE_VEC3, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL,
		false},
	{"greaterThanEqual", TYPE_BVEC4, {TYPE_VEC4, TYPE_VEC4, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL,
		false},

	{"greaterThanEqual", TYPE_BVEC2, {TYPE_IVEC2, TYPE_IVEC2, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL,
		false},
	{"greaterThanEqual", TYPE_BVEC3, {TYPE_IVEC3, TYPE_IVEC3, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL,
		false},
	{"greaterThanEqual", TYPE_BVEC4, {TYPE_IVEC4, TYPE_IVEC4, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL,
		false},

	{"greaterThanEqual", TYPE_BVEC2, {TYPE_UVEC2, TYPE_UVEC2, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL,
		false},
	{"greaterThanEqual", TYPE_BVEC3, {TYPE_UVEC3, TYPE_UVEC3, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL,
		false},
	{"greaterThanEqual", TYPE_BVEC4, {TYPE_UVEC4, TYPE_UVEC4, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL,
		false},

	// equal

	{"equal", TYPE_BVEC2, {TYPE_VEC2, TYPE_VEC2, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"equal", TYPE_BVEC3, {TYPE_VEC3, TYPE_VEC3, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"equal", TYPE_BVEC4, {TYPE_VEC4, TYPE_VEC4, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},

	{"equal", TYPE_BVEC2, {TYPE_IVEC2, TYPE_IVEC2, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"equal", TYPE_BVEC3, {TYPE_IVEC3, TYPE_IVEC3, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"equal", TYPE_BVEC4, {TYPE_IVEC4, TYPE_IVEC4, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},

	{"equal", TYPE_BVEC2, {TYPE_UVEC2, TYPE_UVEC2, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"equal", TYPE_BVEC3, {TYPE_UVEC3, TYPE_UVEC3, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"equal", TYPE_BVEC4, {TYPE_UVEC4, TYPE_UVEC4, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},

	{"equal", TYPE_BVEC2, {TYPE_BVEC2, TYPE_BVEC2, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"equal", TYPE_BVEC3, {TYPE_BVEC3, TYPE_BVEC3, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"equal", TYPE_BVEC4, {TYPE_BVEC4, TYPE_BVEC4, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},

	// notEqual

	{"notEqual", TYPE_BVEC2, {TYPE_VEC2, TYPE_VEC2, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"notEqual", TYPE_BVEC3, {TYPE_VEC3, TYPE_VEC3, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"notEqual", TYPE_BVEC4, {TYPE_VEC4, TYPE_VEC4, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},

	{"notEqual", TYPE_BVEC2, {TYPE_IVEC2, TYPE_IVEC2, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"notEqual", TYPE_BVEC3, {TYPE_IVEC3, TYPE_IVEC3, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"notEqual", TYPE_BVEC4, {TYPE_IVEC4, TYPE_IVEC4, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},

	{"notEqual", TYPE_BVEC2, {TYPE_UVEC2, TYPE_UVEC2, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"notEqual", TYPE_BVEC3, {TYPE_UVEC3, TYPE_UVEC3, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"notEqual", TYPE_BVEC4, {TYPE_UVEC4, TYPE_UVEC4, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},

	{"notEqual", TYPE_BVEC2, {TYPE_BVEC2, TYPE_BVEC2, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"notEqual", TYPE_BVEC3, {TYPE_BVEC3, TYPE_BVEC3, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},
	{"notEqual", TYPE_BVEC4, {TYPE_BVEC4, TYPE_BVEC4, TYPE_VOID}, {"a", "b"}, TAG_GLOBAL, false},

	// any

	{"any", TYPE_BOOL, {TYPE_BVEC2, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"any", TYPE_BOOL, {TYPE_BVEC3, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"any", TYPE_BOOL, {TYPE_BVEC4, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},

	// all

	{"all", TYPE_BOOL, {TYPE_BVEC2, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"all", TYPE_BOOL, {TYPE_BVEC3, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"all", TYPE_BOOL, {TYPE_BVEC4, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},

	// not

	{"not", TYPE_BVEC2, {TYPE_BVEC2, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"not", TYPE_BVEC3, {TYPE_BVEC3, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},
	{"not", TYPE_BVEC4, {TYPE_BVEC4, TYPE_VOID}, {"x"}, TAG_GLOBAL, false},

	// Built-ins: texture functions.
	// textureSize

	{"textureSize", TYPE_IVEC2, {TYPE_SAMPLER2D, TYPE_INT, TYPE_VOID}, {"sampler", "lod"},
		TAG_GLOBAL, false},
	{"textureSize", TYPE_IVEC2, {TYPE_ISAMPLER2D, TYPE_INT, TYPE_VOID}, {"sampler", "lod"},
		TAG_GLOBAL, false},
	{"textureSize", TYPE_IVEC2, {TYPE_USAMPLER2D, TYPE_INT, TYPE_VOID}, {"sampler", "lod"},
		TAG_GLOBAL, false},
	{"textureSize", TYPE_IVEC3, {TYPE_SAMPLER2DARRAY, TYPE_INT, TYPE_VOID}, {"sampler", "lod"},
		TAG_GLOBAL, false},
	{"textureSize", TYPE_IVEC3, {TYPE_ISAMPLER2DARRAY, TYPE_INT, TYPE_VOID}, {"sampler", "lod"},
		TAG_GLOBAL, false},
	{"textureSize", TYPE_IVEC3, {TYPE_USAMPLER2DARRAY, TYPE_INT, TYPE_VOID}, {"sampler", "lod"},
		TAG_GLOBAL, false},
	{"textureSize", TYPE_IVEC3, {TYPE_SAMPLER3D, TYPE_INT, TYPE_VOID}, {"sampler", "lod"},
		TAG_GLOBAL, false},
	{"textureSize", TYPE_IVEC3, {TYPE_ISAMPLER3D, TYPE_INT, TYPE_VOID}, {"sampler", "lod"},
		TAG_GLOBAL, false},
	{"textureSize", TYPE_IVEC3, {TYPE_USAMPLER3D, TYPE_INT, TYPE_VOID}, {"sampler", "lod"},
		TAG_GLOBAL, false},
	{"textureSize", TYPE_IVEC2, {TYPE_SAMPLERCUBE, TYPE_INT, TYPE_VOID}, {"sampler", "lod"},
		TAG_GLOBAL, false},
	{"textureSize", TYPE_IVEC2, {TYPE_SAMPLERCUBEARRAY, TYPE_INT, TYPE_VOID}, {"sampler", "lod"},
		TAG_GLOBAL, false},

	// texture

	{"texture", TYPE_VEC4, {TYPE_SAMPLER2D, TYPE_VEC2, TYPE_VOID}, {"sampler", "coords"},
		TAG_GLOBAL, false},
	{"texture", TYPE_VEC4, {TYPE_SAMPLER2D, TYPE_VEC2, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "bias"}, TAG_GLOBAL, false},
	{"texture", TYPE_UVEC4, {TYPE_USAMPLER2D, TYPE_VEC2, TYPE_VOID}, {"sampler", "coords"},
		TAG_GLOBAL, false},
	{"texture", TYPE_UVEC4, {TYPE_USAMPLER2D, TYPE_VEC2, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "bias"}, TAG_GLOBAL, false},
	{"texture", TYPE_IVEC4, {TYPE_ISAMPLER2D, TYPE_VEC2, TYPE_VOID}, {"sampler", "coords"},
		TAG_GLOBAL, false},
	{"texture", TYPE_IVEC4, {TYPE_ISAMPLER2D, TYPE_VEC2, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "bias"}, TAG_GLOBAL, false},
	{"texture", TYPE_VEC4, {TYPE_SAMPLER2DARRAY, TYPE_VEC3, TYPE_VOID}, {"sampler", "coords"},
		TAG_GLOBAL, false},
	{"texture", TYPE_VEC4, {TYPE_SAMPLER2DARRAY, TYPE_VEC3, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "bias"}, TAG_GLOBAL, false},
	{"texture", TYPE_UVEC4, {TYPE_USAMPLER2DARRAY, TYPE_VEC3, TYPE_VOID}, {"sampler", "coords"},
		TAG_GLOBAL, false},
	{"texture", TYPE_UVEC4, {TYPE_USAMPLER2DARRAY, TYPE_VEC3, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "bias"}, TAG_GLOBAL, false},
	{"texture", TYPE_IVEC4, {TYPE_ISAMPLER2DARRAY, TYPE_VEC3, TYPE_VOID}, {"sampler", "coords"},
		TAG_GLOBAL, false},
	{"texture", TYPE_IVEC4, {TYPE_ISAMPLER2DARRAY, TYPE_VEC3, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "bias"}, TAG_GLOBAL, false},
	{"texture", TYPE_VEC4, {TYPE_SAMPLER3D, TYPE_VEC3, TYPE_VOID}, {"sampler", "coords"},
		TAG_GLOBAL, false},
	{"texture", TYPE_VEC4, {TYPE_SAMPLER3D, TYPE_VEC3, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "bias"}, TAG_GLOBAL, false},
	{"texture", TYPE_UVEC4, {TYPE_USAMPLER3D, TYPE_VEC3, TYPE_VOID}, {"sampler", "coords"},
		TAG_GLOBAL, false},
	{"texture", TYPE_UVEC4, {TYPE_USAMPLER3D, TYPE_VEC3, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "bias"}, TAG_GLOBAL, false},
	{"texture", TYPE_IVEC4, {TYPE_ISAMPLER3D, TYPE_VEC3, TYPE_VOID}, {"sampler", "coords"},
		TAG_GLOBAL, false},
	{"texture", TYPE_IVEC4, {TYPE_ISAMPLER3D, TYPE_VEC3, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "bias"}, TAG_GLOBAL, false},
	{"texture", TYPE_VEC4, {TYPE_SAMPLERCUBE, TYPE_VEC3, TYPE_VOID}, {"sampler", "coords"},
		TAG_GLOBAL, false},
	{"texture", TYPE_VEC4, {TYPE_SAMPLERCUBE, TYPE_VEC3, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "bias"}, TAG_GLOBAL, false},
	{"texture", TYPE_VEC4, {TYPE_SAMPLERCUBEARRAY, TYPE_VEC4, TYPE_VOID}, {"sampler", "coords"},
		TAG_GLOBAL, false},
	{"texture", TYPE_VEC4, {TYPE_SAMPLERCUBEARRAY, TYPE_VEC4, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "bias"}, TAG_GLOBAL, false},
	{"texture", TYPE_VEC4, {TYPE_SAMPLEREXT, TYPE_VEC2, TYPE_VOID}, {"sampler", "coords"},
		TAG_GLOBAL, false},
	{"texture", TYPE_VEC4, {TYPE_SAMPLEREXT, TYPE_VEC2, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "bias"}, TAG_GLOBAL, false},

	// textureProj

	{"textureProj", TYPE_VEC4, {TYPE_SAMPLER2D, TYPE_VEC3, TYPE_VOID}, {"sampler", "coords"},
		TAG_GLOBAL, false},
	{"textureProj", TYPE_VEC4, {TYPE_SAMPLER2D, TYPE_VEC4, TYPE_VOID}, {"sampler", "coords"},
		TAG_GLOBAL, false},
	{"textureProj", TYPE_VEC4, {TYPE_SAMPLER2D, TYPE_VEC3, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "bias"}, TAG_GLOBAL, false},
	{"textureProj", TYPE_VEC4, {TYPE_SAMPLER2D, TYPE_VEC4, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "bias"}, TAG_GLOBAL, false},
	{"textureProj", TYPE_IVEC4, {TYPE_ISAMPLER2D, TYPE_VEC3, TYPE_VOID}, {"sampler", "coords"},
		TAG_GLOBAL, false},
	{"textureProj", TYPE_IVEC4, {TYPE_ISAMPLER2D, TYPE_VEC4, TYPE_VOID}, {"sampler", "coords"},
		TAG_GLOBAL, false},
	{"textureProj", TYPE_IVEC4, {TYPE_ISAMPLER2D, TYPE_VEC3, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "bias"}, TAG_GLOBAL, false},
	{"textureProj", TYPE_IVEC4, {TYPE_ISAMPLER2D, TYPE_VEC4, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "bias"}, TAG_GLOBAL, false},
	{"textureProj", TYPE_UVEC4, {TYPE_USAMPLER2D, TYPE_VEC3, TYPE_VOID}, {"sampler", "coords"},
		TAG_GLOBAL, false},
	{"textureProj", TYPE_UVEC4, {TYPE_USAMPLER2D, TYPE_VEC4, TYPE_VOID}, {"sampler", "coords"},
		TAG_GLOBAL, false},
	{"textureProj", TYPE_UVEC4, {TYPE_USAMPLER2D, TYPE_VEC3, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "bias"}, TAG_GLOBAL, false},
	{"textureProj", TYPE_UVEC4, {TYPE_USAMPLER2D, TYPE_VEC4, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "bias"}, TAG_GLOBAL, false},
	{"textureProj", TYPE_VEC4, {TYPE_SAMPLER3D, TYPE_VEC4, TYPE_VOID}, {"sampler", "coords"},
		TAG_GLOBAL, false},
	{"textureProj", TYPE_VEC4, {TYPE_SAMPLER3D, TYPE_VEC4, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "bias"}, TAG_GLOBAL, false},
	{"textureProj", TYPE_IVEC4, {TYPE_ISAMPLER3D, TYPE_VEC4, TYPE_VOID}, {"sampler", "coords"},
		TAG_GLOBAL, false},
	{"textureProj", TYPE_IVEC4, {TYPE_ISAMPLER3D, TYPE_VEC4, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "bias"}, TAG_GLOBAL, false},
	{"textureProj", TYPE_UVEC4, {TYPE_USAMPLER3D, TYPE_VEC4, TYPE_VOID}, {"sampler", "coords"},
		TAG_GLOBAL, false},
	{"textureProj", TYPE_UVEC4, {TYPE_USAMPLER3D, TYPE_VEC4, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "bias"}, TAG_GLOBAL, false},

	// textureLod

	{"textureLod", TYPE_VEC4, {TYPE_SAMPLER2D, TYPE_VEC2, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "lod"}, TAG_GLOBAL, false},
	{"textureLod", TYPE_IVEC4, {TYPE_ISAMPLER2D, TYPE_VEC2, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "lod"}, TAG_GLOBAL, false},
	{"textureLod", TYPE_UVEC4, {TYPE_USAMPLER2D, TYPE_VEC2, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "lod"}, TAG_GLOBAL, false},
	{"textureLod", TYPE_VEC4, {TYPE_SAMPLER2DARRAY, TYPE_VEC3, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "lod"}, TAG_GLOBAL, false},
	{"textureLod", TYPE_IVEC4, {TYPE_ISAMPLER2DARRAY, TYPE_VEC3, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "lod"}, TAG_GLOBAL, false},
	{"textureLod", TYPE_UVEC4, {TYPE_USAMPLER2DARRAY, TYPE_VEC3, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "lod"}, TAG_GLOBAL, false},
	{"textureLod", TYPE_VEC4, {TYPE_SAMPLER3D, TYPE_VEC3, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "lod"}, TAG_GLOBAL, false},
	{"textureLod", TYPE_IVEC4, {TYPE_ISAMPLER3D, TYPE_VEC3, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "lod"}, TAG_GLOBAL, false},
	{"textureLod", TYPE_UVEC4, {TYPE_USAMPLER3D, TYPE_VEC3, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "lod"}, TAG_GLOBAL, false},
	{"textureLod", TYPE_VEC4, {TYPE_SAMPLERCUBE, TYPE_VEC3, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "lod"}, TAG_GLOBAL, false},
	{"textureLod", TYPE_VEC4, {TYPE_SAMPLERCUBEARRAY, TYPE_VEC4, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "lod"}, TAG_GLOBAL, false},

	// texelFetch

	{"texelFetch", TYPE_VEC4, {TYPE_SAMPLER2D, TYPE_IVEC2, TYPE_INT, TYPE_VOID},
		{"sampler", "coords", "lod"}, TAG_GLOBAL, false},
	{"texelFetch", TYPE_IVEC4, {TYPE_ISAMPLER2D, TYPE_IVEC2, TYPE_INT, TYPE_VOID},
		{"sampler", "coords", "lod"}, TAG_GLOBAL, false},
	{"texelFetch", TYPE_UVEC4, {TYPE_USAMPLER2D, TYPE_IVEC2, TYPE_INT, TYPE_VOID},
		{"sampler", "coords", "lod"}, TAG_GLOBAL, false},
	{"texelFetch", TYPE_VEC4, {TYPE_SAMPLER2DARRAY, TYPE_IVEC3, TYPE_INT, TYPE_VOID},
		{"sampler", "coords", "lod"}, TAG_GLOBAL, false},
	{"texelFetch", TYPE_IVEC4, {TYPE_ISAMPLER2DARRAY, TYPE_IVEC3, TYPE_INT, TYPE_VOID},
		{"sampler", "coords", "lod"}, TAG_GLOBAL, false},
	{"texelFetch", TYPE_UVEC4, {TYPE_USAMPLER2DARRAY, TYPE_IVEC3, TYPE_INT, TYPE_VOID},
		{"sampler", "coords", "lod"}, TAG_GLOBAL, false},
	{"texelFetch", TYPE_VEC4, {TYPE_SAMPLER3D, TYPE_IVEC3, TYPE_INT, TYPE_VOID},
		{"sampler", "coords", "lod"}, TAG_GLOBAL, false},
	{"texelFetch", TYPE_IVEC4, {TYPE_ISAMPLER3D, TYPE_IVEC3, TYPE_INT, TYPE_VOID},
		{"sampler", "coords", "lod"}, TAG_GLOBAL, false},
	{"texelFetch", TYPE_UVEC4, {TYPE_USAMPLER3D, TYPE_IVEC3, TYPE_INT, TYPE_VOID},
		{"sampler", "coords", "lod"}, TAG_GLOBAL, false},

	// textureProjLod

	{"textureProjLod", TYPE_VEC4, {TYPE_SAMPLER2D, TYPE_VEC3, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "lod"}, TAG_GLOBAL, false},
	{"textureProjLod", TYPE_VEC4, {TYPE_SAMPLER2D, TYPE_VEC4, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "lod"}, TAG_GLOBAL, false},
	{"textureProjLod", TYPE_IVEC4, {TYPE_ISAMPLER2D, TYPE_VEC3, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "lod"}, TAG_GLOBAL, false},
	{"textureProjLod", TYPE_IVEC4, {TYPE_ISAMPLER2D, TYPE_VEC4, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "lod"}, TAG_GLOBAL, false},
	{"textureProjLod", TYPE_UVEC4, {TYPE_USAMPLER2D, TYPE_VEC3, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "lod"}, TAG_GLOBAL, false},
	{"textureProjLod", TYPE_UVEC4, {TYPE_USAMPLER2D, TYPE_VEC4, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "lod"}, TAG_GLOBAL, false},
	{"textureProjLod", TYPE_VEC4, {TYPE_SAMPLER3D, TYPE_VEC4, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "lod"}, TAG_GLOBAL, false},
	{"textureProjLod", TYPE_IVEC4, {TYPE_ISAMPLER3D, TYPE_VEC4, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "lod"}, TAG_GLOBAL, false},
	{"textureProjLod", TYPE_UVEC4, {TYPE_USAMPLER3D, TYPE_VEC4, TYPE_FLOAT, TYPE_VOID},
		{"sampler", "coords", "lod"}, TAG_GLOBAL, false},

	// textureGrad

	{"textureGrad", TYPE_VEC4, {TYPE_SAMPLER2D, TYPE_VEC2, TYPE_VEC2, TYPE_VEC2, TYPE_VOID},
		{"sampler", "coords", "dPdx", "dPdy"}, TAG_GLOBAL, false},
	{"textureGrad", TYPE_IVEC4, {TYPE_ISAMPLER2D, TYPE_VEC2, TYPE_VEC2, TYPE_VEC2, TYPE_VOID},
		{"sampler", "coords", "dPdx", "dPdy"}, TAG_GLOBAL, false},
	{"textureGrad", TYPE_UVEC4, {TYPE_USAMPLER2D, TYPE_VEC2, TYPE_VEC2, TYPE_VEC2, TYPE_VOID},
		{"sampler", "coords", "dPdx", "dPdy"}, TAG_GLOBAL, false},
	{"textureGrad", TYPE_VEC4, {TYPE_SAMPLER2DARRAY, TYPE_VEC3, TYPE_VEC2, TYPE_VEC2, TYPE_VOID},
		{"sampler", "coords", "dPdx", "dPdy"}, TAG_GLOBAL, false},
	{"textureGrad", TYPE_IVEC4, {TYPE_ISAMPLER2DARRAY, TYPE_VEC3, TYPE_VEC2, TYPE_VEC2, TYPE_VOID},
		{"sampler", "coords", "dPdx", "dPdy"}, TAG_GLOBAL, false},
	{"textureGrad", TYPE_UVEC4, {TYPE_USAMPLER2DARRAY, TYPE_VEC3, TYPE_VEC2, TYPE_VEC2, TYPE_VOID},
		{"sampler", "coords", "dPdx", "dPdy"}, TAG_GLOBAL, false},
	{"textureGrad", TYPE_VEC4, {TYPE_SAMPLER3D, TYPE_VEC3, TYPE_VEC3, TYPE_VEC3, TYPE_VOID},
		{"sampler", "coords", "dPdx", "dPdy"}, TAG_GLOBAL, false},
	{"textureGrad", TYPE_IVEC4, {TYPE_ISAMPLER3D, TYPE_VEC3, TYPE_VEC3, TYPE_VEC3, TYPE_VOID},
		{"sampler", "coords", "dPdx", "dPdy"}, TAG_GLOBAL, false},
	{"textureGrad", TYPE_UVEC4, {TYPE_USAMPLER3D, TYPE_VEC3, TYPE_VEC3, TYPE_VEC3, TYPE_VOID},
		{"sampler", "coords", "dPdx", "dPdy"}, TAG_GLOBAL, false},
	{"textureGrad", TYPE_VEC4, {TYPE_SAMPLERCUBE, TYPE_VEC3, TYPE_VEC3, TYPE_VEC3, TYPE_VOID},
		{"sampler", "coords", "dPdx", "dPdy"}, TAG_GLOBAL, false},
	{"textureGrad", TYPE_VEC4, {TYPE_SAMPLERCUBEARRAY, TYPE_VEC4, TYPE_VEC3, TYPE_VEC3, TYPE_VOID},
		{"sampler", "coords", "dPdx", "dPdy"}, TAG_GLOBAL, false},

	// textureProjGrad

	{"textureProjGrad", TYPE_VEC4, {TYPE_SAMPLER2D, TYPE_VEC3, TYPE_VEC2, TYPE_VEC2, TYPE_VOID},
		{"sampler", "coords", "dPdx", "dPdy"}, TAG_GLOBAL, false},
	{"textureProjGrad", TYPE_VEC4, {TYPE_SAMPLER2D, TYPE_VEC4, TYPE_VEC2, TYPE_VEC2, TYPE_VOID},
		{"sampler", "coords", "dPdx", "dPdy"}, TAG_GLOBAL, false},
	{"textureProjGrad", TYPE_IVEC4, {TYPE_ISAMPLER2D, TYPE_VEC3, TYPE_VEC2, TYPE_VEC2, TYPE_VOID},
		{"sampler", "coords", "dPdx", "dPdy"}, TAG_GLOBAL, false},
	{"textureProjGrad", TYPE_IVEC4, {TYPE_ISAMPLER2D, TYPE_VEC4, TYPE_VEC2, TYPE_VEC2, TYPE_VOID},
		{"sampler", "coords", "dPdx", "dPdy"}, TAG_GLOBAL, false},
	{"textureProjGrad", TYPE_UVEC4, {TYPE_USAMPLER2D, TYPE_VEC3, TYPE_VEC2, TYPE_VEC2, TYPE_VOID},
		{"sampler", "coords", "dPdx", "dPdy"}, TAG_GLOBAL, false},
	{"textureProjGrad", TYPE_UVEC4, {TYPE_USAMPLER2D, TYPE_VEC4, TYPE_VEC2, TYPE_VEC2, TYPE_VOID},
		{"sampler", "coords", "dPdx", "dPdy"}, TAG_GLOBAL, false},
	{"textureProjGrad", TYPE_VEC4, {TYPE_SAMPLER3D, TYPE_VEC4, TYPE_VEC3, TYPE_VEC3, TYPE_VOID},
		{"sampler", "coords", "dPdx", "dPdy"}, TAG_GLOBAL, false},
	{"textureProjGrad", TYPE_IVEC4, {TYPE_ISAMPLER3D, TYPE_VEC4, TYPE_VEC3, TYPE_VEC3, TYPE_VOID},
		{"sampler", "coords", "dPdx", "dPdy"}, TAG_GLOBAL, false},
	{"textureProjGrad", TYPE_UVEC4, {TYPE_USAMPLER3D, TYPE_VEC4, TYPE_VEC3, TYPE_VEC3, TYPE_VOID},
		{"sampler", "coords", "dPdx", "dPdy"}, TAG_GLOBAL, false},

	// textureGather

	{"textureGather", TYPE_VEC4, {TYPE_SAMPLER2D, TYPE_VEC2, TYPE_VOID}, {"sampler", "coords"},
		TAG_GLOBAL, true},
	{"textureGather", TYPE_IVEC4, {TYPE_ISAMPLER2D, TYPE_VEC2, TYPE_VOID}, {"sampler", "coords"},
		TAG_GLOBAL, true},
	{"textureGather", TYPE_UVEC4, {TYPE_USAMPLER2D, TYPE_VEC2, TYPE_VOID}, {"sampler", "coords"},
		TAG_GLOBAL, true},
	{"textureGather", TYPE_VEC4, {TYPE_SAMPLER2D, TYPE_VEC2, TYPE_INT, TYPE_VOID},
		{"sampler", "coords", "comp"}, TAG_GLOBAL, true},
	{"textureGather", TYPE_IVEC4, {TYPE_ISAMPLER2D, TYPE_VEC2, TYPE_INT, TYPE_VOID},
		{"sampler", "coords", "comp"}, TAG_GLOBAL, true},
	{"textureGather", TYPE_UVEC4, {TYPE_USAMPLER2D, TYPE_VEC2, TYPE_INT, TYPE_VOID},
		{"sampler", "coords", "comp"}, TAG_GLOBAL, true},
	{"textureGather", TYPE_VEC4, {TYPE_SAMPLER2DARRAY, TYPE_VEC3, TYPE_VOID}, {"sampler", "coords"},
		TAG_GLOBAL, true},
	{"textureGather", TYPE_IVEC4, {TYPE_ISAMPLER2DARRAY, TYPE_VEC3, TYPE_VOID},
		{"sampler", "coords"}, TAG_GLOBAL, true},
	{"textureGather", TYPE_UVEC4, {TYPE_USAMPLER2DARRAY, TYPE_VEC3, TYPE_VOID},
		{"sampler", "coords"}, TAG_GLOBAL, true},
	{"textureGather", TYPE_VEC4, {TYPE_SAMPLER2DARRAY, TYPE_VEC3, TYPE_INT, TYPE_VOID},
		{"sampler", "coords", "comp"}, TAG_GLOBAL, true},
	{"textureGather", TYPE_IVEC4, {TYPE_ISAMPLER2DARRAY, TYPE_VEC3, TYPE_INT, TYPE_VOID},
		{"sampler", "coords", "comp"}, TAG_GLOBAL, true},
	{"textureGather", TYPE_UVEC4, {TYPE_USAMPLER2DARRAY, TYPE_VEC3, TYPE_INT, TYPE_VOID},
		{"sampler", "coords", "comp"}, TAG_GLOBAL, true},
	{"textureGather", TYPE_VEC4, {TYPE_SAMPLERCUBE, TYPE_VEC3, TYPE_VOID}, {"sampler", "coords"},
		TAG_GLOBAL, true},
	{"textureGather", TYPE_VEC4, {TYPE_SAMPLERCUBE, TYPE_VEC3, TYPE_INT, TYPE_VOID},
		{"sampler", "coords", "comp"}, TAG_GLOBAL, true},

	// textureQueryLod

	{"textureQueryLod", TYPE_VEC2, {TYPE_SAMPLER2D, TYPE_VEC2}, {"sampler", "coords"}, TAG_GLOBAL,
		true},
	{"textureQueryLod", TYPE_VEC2, {TYPE_ISAMPLER2D, TYPE_VEC2}, {"sampler", "coords"}, TAG_GLOBAL,
		true},
	{"textureQueryLod", TYPE_VEC2, {TYPE_USAMPLER2D, TYPE_VEC2}, {"sampler", "coords"}, TAG_GLOBAL,
		true},
	{"textureQueryLod", TYPE_VEC2, {TYPE_SAMPLER2DARRAY, TYPE_VEC2}, {"sampler", "coords"},
		TAG_GLOBAL, true},
	{"textureQueryLod", TYPE_VEC2, {TYPE_ISAMPLER2DARRAY, TYPE_VEC2}, {"sampler", "coords"},
		TAG_GLOBAL, true},
	{"textureQueryLod", TYPE_VEC2, {TYPE_USAMPLER2DARRAY, TYPE_VEC2}, {"sampler", "coords"},
		TAG_GLOBAL, true},
	{"textureQueryLod", TYPE_VEC2, {TYPE_SAMPLER3D, TYPE_VEC3}, {"sampler", "coords"}, TAG_GLOBAL,
		true},
	{"textureQueryLod", TYPE_VEC2, {TYPE_ISAMPLER3D, TYPE_VEC3}, {"sampler", "coords"}, TAG_GLOBAL,
		true},
	{"textureQueryLod", TYPE_VEC2, {TYPE_USAMPLER3D, TYPE_VEC3}, {"sampler", "coords"}, TAG_GLOBAL,
		true},
	{"textureQueryLod", TYPE_VEC2, {TYPE_SAMPLERCUBE, TYPE_VEC3}, {"sampler", "coords"}, TAG_GLOBAL,
		true},

	// textureQueryLevels

	{"textureQueryLevels", TYPE_INT, {TYPE_SAMPLER2D}, {"sampler"}, TAG_GLOBAL, true},
	{"textureQueryLevels", TYPE_INT, {TYPE_ISAMPLER2D}, {"sampler"}, TAG_GLOBAL, true},
	{"textureQueryLevels", TYPE_INT, {TYPE_USAMPLER2D}, {"sampler"}, TAG_GLOBAL, true},
	{"textureQueryLevels", TYPE_INT, {TYPE_SAMPLER2DARRAY}, {"sampler"}, TAG_GLOBAL, true},
	{"textureQueryLevels", TYPE_INT, {TYPE_ISAMPLER2DARRAY}, {"sampler"}, TAG_GLOBAL, true},
	{"textureQueryLevels", TYPE_INT, {TYPE_USAMPLER2DARRAY}, {"sampler"}, TAG_GLOBAL, true},
	{"textureQueryLevels", TYPE_INT, {TYPE_SAMPLER3D}, {"sampler"}, TAG_GLOBAL, true},
	{"textureQueryLevels", TYPE_INT, {TYPE_ISAMPLER3D}, {"sampler"}, TAG_GLOBAL, true},
	{"textureQueryLevels", TYPE_INT, {TYPE_USAMPLER3D}, {"sampler"}, TAG_GLOBAL, true},
	{"textureQueryLevels", TYPE_INT, {TYPE_SAMPLERCUBE}, {"sampler"}, TAG_GLOBAL, true},

	// dFdx

	{"dFdx", TYPE_FLOAT, {TYPE_FLOAT, TYPE_VOID}, {"p"}, TAG_GLOBAL, false},
	{"dFdx", TYPE_VEC2, {TYPE_VEC2, TYPE_VOID}, {"p"}, TAG_GLOBAL, false},
	{"dFdx", TYPE_VEC3, {TYPE_VEC3, TYPE_VOID}, {"p"}, TAG_GLOBAL, false},
	{"dFdx", TYPE_VEC4, {TYPE_VEC4, TYPE_VOID}, {"p"}, TAG_GLOBAL, false},

	// dFdxCoarse

	{"dFdxCoarse", TYPE_FLOAT, {TYPE_FLOAT, TYPE_VOID}, {"p"}, TAG_GLOBAL, true},
	{"dFdxCoarse", TYPE_VEC2, {TYPE_VEC2, TYPE_VOID}, {"p"}, TAG_GLOBAL, true},
	{"dFdxCoarse", TYPE_VEC3, {TYPE_VEC3, TYPE_VOID}, {"p"}, TAG_GLOBAL, true},
	{"dFdxCoarse", TYPE_VEC4, {TYPE_VEC4, TYPE_VOID}, {"p"}, TAG_GLOBAL, true},

	// dFdxFine

	{"dFdxFine", TYPE_FLOAT, {TYPE_FLOAT, TYPE_VOID}, {"p"}, TAG_GLOBAL, true},
	{"dFdxFine", TYPE_VEC2, {TYPE_VEC2, TYPE_VOID}, {"p"}, TAG_GLOBAL, true},
	{"dFdxFine", TYPE_VEC3, {TYPE_VEC3, TYPE_VOID}, {"p"}, TAG_GLOBAL, true},

	{"dFdxFine", TYPE_VEC4, {TYPE_VEC4, TYPE_VOID}, {"p"}, TAG_GLOBAL, true},

	// dFdy

	{"dFdy", TYPE_FLOAT, {TYPE_FLOAT, TYPE_VOID}, {"p"}, TAG_GLOBAL, false},
	{"dFdy", TYPE_VEC2, {TYPE_VEC2, TYPE_VOID}, {"p"}, TAG_GLOBAL, false},
	{"dFdy", TYPE_VEC3, {TYPE_VEC3, TYPE_VOID}, {"p"}, TAG_GLOBAL, false},
	{"dFdy", TYPE_VEC4, {TYPE_VEC4, TYPE_VOID}, {"p"}, TAG_GLOBAL, false},

	// dFdyCoarse

	{"dFdyCoarse", TYPE_FLOAT, {TYPE_FLOAT, TYPE_VOID}, {"p"}, TAG_GLOBAL, true},
	{"dFdyCoarse", TYPE_VEC2, {TYPE_VEC2, TYPE_VOID}, {"p"}, TAG_GLOBAL, true},
	{"dFdyCoarse", TYPE_VEC3, {TYPE_VEC3, TYPE_VOID}, {"p"}, TAG_GLOBAL, true},
	{"dFdyCoarse", TYPE_VEC4, {TYPE_VEC4, TYPE_VOID}, {"p"}, TAG_GLOBAL, true},

	// dFdyFine

	{"dFdyFine", TYPE_FLOAT, {TYPE_FLOAT, TYPE_VOID}, {"p"}, TAG_GLOBAL, true},
	{"dFdyFine", TYPE_VEC2, {TYPE_VEC2, TYPE_VOID}, {"p"}, TAG_GLOBAL, true},
	{"dFdyFine", TYPE_VEC3, {TYPE_VEC3, TYPE_VOID}, {"p"}, TAG_GLOBAL, true},
	{"dFdyFine", TYPE_VEC4, {TYPE_VEC4, TYPE_VOID}, {"p"}, TAG_GLOBAL, true},

	// fwidth

	{"fwidth", TYPE_FLOAT, {TYPE_FLOAT, TYPE_VOID}, {"p"}, TAG_GLOBAL, false},
	{"fwidth", TYPE_VEC2, {TYPE_VEC2, TYPE_VOID}, {"p"}, TAG_GLOBAL, false},
	{"fwidth", TYPE_VEC3, {TYPE_VEC3, TYPE_VOID}, {"p"}, TAG_GLOBAL, false},
	{"fwidth", TYPE_VEC4, {TYPE_VEC4, TYPE_VOID}, {"p"}, TAG_GLOBAL, false},

	// fwidthCoarse

	{"fwidthCoarse", TYPE_FLOAT, {TYPE_FLOAT, TYPE_VOID}, {"p"}, TAG_GLOBAL, true},
	{"fwidthCoarse", TYPE_VEC2, {TYPE_VEC2, TYPE_VOID}, {"p"}, TAG_GLOBAL, true},
	{"fwidthCoarse", TYPE_VEC3, {TYPE_VEC3, TYPE_VOID}, {"p"}, TAG_GLOBAL, true},
	{"fwidthCoarse", TYPE_VEC4, {TYPE_VEC4, TYPE_VOID}, {"p"}, TAG_GLOBAL, true},

	// fwidthFine

	{"fwidthFine", TYPE_FLOAT, {TYPE_FLOAT, TYPE_VOID}, {"p"}, TAG_GLOBAL, true},
	{"fwidthFine", TYPE_VEC2, {TYPE_VEC2, TYPE_VOID}, {"p"}, TAG_GLOBAL, true},
	{"fwidthFine", TYPE_VEC3, {TYPE_VEC3, TYPE_VOID}, {"p"}, TAG_GLOBAL, true},
	{"fwidthFine", TYPE_VEC4, {TYPE_VEC4, TYPE_VOID}, {"p"}, TAG_GLOBAL, true},

	// Sub-functions.
	// array

	{"length", TYPE_INT, {TYPE_VOID}, {""}, TAG_ARRAY, false},

	// Modern functions.
	// fma

	{"fma", TYPE_FLOAT, {TYPE_FLOAT, TYPE_FLOAT, TYPE_FLOAT, TYPE_VOID}, {"a", "b", "c"},
		TAG_GLOBAL, true},
	{"fma", TYPE_VEC2, {TYPE_VEC2, TYPE_VEC2, TYPE_VEC2, TYPE_VOID}, {"a", "b", "c"}, TAG_GLOBAL,
		true},
	{"fma", TYPE_VEC3, {TYPE_VEC3, TYPE_VEC3, TYPE_VEC3, TYPE_VOID}, {"a", "b", "c"}, TAG_GLOBAL,
		true},
	{"fma", TYPE_VEC4, {TYPE_VEC4, TYPE_VEC4, TYPE_VEC4, TYPE_VOID}, {"a", "b", "c"}, TAG_GLOBAL,
		true},

	// Packing/Unpacking functions.

	{"packHalf2x16", TYPE_UINT, {TYPE_VEC2, TYPE_VOID}, {"v"}, TAG_GLOBAL, false},
	{"packUnorm2x16", TYPE_UINT, {TYPE_VEC2, TYPE_VOID}, {"v"}, TAG_GLOBAL, false},
	{"packSnorm2x16", TYPE_UINT, {TYPE_VEC2, TYPE_VOID}, {"v"}, TAG_GLOBAL, false},
	{"packUnorm4x8", TYPE_UINT, {TYPE_VEC4, TYPE_VOID}, {"v"}, TAG_GLOBAL, false},
	{"packSnorm4x8", TYPE_UINT, {TYPE_VEC4, TYPE_VOID}, {"v"}, TAG_GLOBAL, false},

	{"unpackHalf2x16", TYPE_VEC2, {TYPE_UINT, TYPE_VOID}, {"v"}, TAG_GLOBAL, false},
	{"unpackUnorm2x16", TYPE_VEC2, {TYPE_UINT, TYPE_VOID}, {"v"}, TAG_GLOBAL, false},
	{"unpackSnorm2x16", TYPE_VEC2, {TYPE_UINT, TYPE_VOID}, {"v"}, TAG_GLOBAL, false},
	{"unpackUnorm4x8", TYPE_VEC4, {TYPE_UINT, TYPE_VOID}, {"v"}, TAG_GLOBAL, false},
	{"unpackSnorm4x8", TYPE_VEC4, {TYPE_UINT, TYPE_VOID}, {"v"}, TAG_GLOBAL, false},

	// bitfieldExtract

	{"bitfieldExtract", TYPE_INT, {TYPE_INT, TYPE_INT, TYPE_INT, TYPE_VOID},
		{"value", "offset", "bits"}, TAG_GLOBAL, true},
	{"bitfieldExtract", TYPE_IVEC2, {TYPE_IVEC2, TYPE_INT, TYPE_INT, TYPE_VOID},
		{"value", "offset", "bits"}, TAG_GLOBAL, true},
	{"bitfieldExtract", TYPE_IVEC3, {TYPE_IVEC3, TYPE_INT, TYPE_INT, TYPE_VOID},
		{"value", "offset", "bits"}, TAG_GLOBAL, true},
	{"bitfieldExtract", TYPE_IVEC4, {TYPE_IVEC4, TYPE_INT, TYPE_INT, TYPE_VOID},
		{"value", "offset", "bits"}, TAG_GLOBAL, true},

	{"bitfieldExtract", TYPE_UINT, {TYPE_UINT, TYPE_INT, TYPE_INT, TYPE_VOID},
		{"value", "offset", "bits"}, TAG_GLOBAL, true},
	{"bitfieldExtract", TYPE_UVEC2, {TYPE_UVEC2, TYPE_INT, TYPE_INT, TYPE_VOID},
		{"value", "offset", "bits"}, TAG_GLOBAL, true},
	{"bitfieldExtract", TYPE_UVEC3, {TYPE_UVEC3, TYPE_INT, TYPE_INT, TYPE_VOID},
		{"value", "offset", "bits"}, TAG_GLOBAL, true},
	{"bitfieldExtract", TYPE_UVEC4, {TYPE_UVEC4, TYPE_INT, TYPE_INT, TYPE_VOID},
		{"value", "offset", "bits"}, TAG_GLOBAL, true},

	// bitfieldInsert

	{"bitfieldInsert", TYPE_INT, {TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_VOID},
		{"base", "insert", "offset", "bits"}, TAG_GLOBAL, true},
	{"bitfieldInsert", TYPE_IVEC2, {TYPE_IVEC2, TYPE_IVEC2, TYPE_INT, TYPE_INT, TYPE_VOID},
		{"base", "insert", "offset", "bits"}, TAG_GLOBAL, true},
	{"bitfieldInsert", TYPE_IVEC3, {TYPE_IVEC3, TYPE_IVEC3, TYPE_INT, TYPE_INT, TYPE_VOID},
		{"base", "insert", "offset", "bits"}, TAG_GLOBAL, true},
	{"bitfieldInsert", TYPE_IVEC4, {TYPE_IVEC4, TYPE_IVEC4, TYPE_INT, TYPE_INT, TYPE_VOID},
		{"base", "insert", "offset", "bits"}, TAG_GLOBAL, true},

	{"bitfieldInsert", TYPE_UINT, {TYPE_UINT, TYPE_UINT, TYPE_INT, TYPE_INT, TYPE_VOID},
		{"base", "insert", "offset", "bits"}, TAG_GLOBAL, true},
	{"bitfieldInsert", TYPE_UVEC2, {TYPE_UVEC2, TYPE_UVEC2, TYPE_INT, TYPE_INT, TYPE_VOID},
		{"base", "insert", "offset", "bits"}, TAG_GLOBAL, true},
	{"bitfieldInsert", TYPE_UVEC3, {TYPE_UVEC3, TYPE_UVEC3, TYPE_INT, TYPE_INT, TYPE_VOID},
		{"base", "insert", "offset", "bits"}, TAG_GLOBAL, true},
	{"bitfieldInsert", TYPE_UVEC4, {TYPE_UVEC4, TYPE_UVEC4, TYPE_INT, TYPE_INT, TYPE_VOID},
		{"base", "insert", "offset", "bits"}, TAG_GLOBAL, true},

	// bitfieldReverse

	{"bitfieldReverse", TYPE_INT, {TYPE_INT, TYPE_VOID}, {"value"}, TAG_GLOBAL, true},
	{"bitfieldReverse", TYPE_IVEC2, {TYPE_IVEC2, TYPE_VOID}, {"value"}, TAG_GLOBAL, true},
	{"bitfieldReverse", TYPE_IVEC3, {TYPE_IVEC3, TYPE_VOID}, {"value"}, TAG_GLOBAL, true},
	{"bitfieldReverse", TYPE_IVEC4, {TYPE_IVEC4, TYPE_VOID}, {"value"}, TAG_GLOBAL, true},

	{"bitfieldReverse", TYPE_UINT, {TYPE_UINT, TYPE_VOID}, {"value"}, TAG_GLOBAL, true},
	{"bitfieldReverse", TYPE_UVEC2, {TYPE_UVEC2, TYPE_VOID}, {"value"}, TAG_GLOBAL, true},
	{"bitfieldReverse", TYPE_UVEC3, {TYPE_UVEC3, TYPE_VOID}, {"value"}, TAG_GLOBAL, true},
	{"bitfieldReverse", TYPE_UVEC4, {TYPE_UVEC4, TYPE_VOID}, {"value"}, TAG_GLOBAL, true},

	// bitCount

	{"bitCount", TYPE_INT, {TYPE_INT, TYPE_VOID}, {"value"}, TAG_GLOBAL, true},
	{"bitCount", TYPE_IVEC2, {TYPE_IVEC2, TYPE_VOID}, {"value"}, TAG_GLOBAL, true},
	{"bitCount", TYPE_IVEC3, {TYPE_IVEC3, TYPE_VOID}, {"value"}, TAG_GLOBAL, true},
	{"bitCount", TYPE_IVEC4, {TYPE_IVEC4, TYPE_VOID}, {"value"}, TAG_GLOBAL, true},

	{"bitCount", TYPE_UINT, {TYPE_UINT, TYPE_VOID}, {"value"}, TAG_GLOBAL, true},
	{"bitCount", TYPE_UVEC2, {TYPE_UVEC2, TYPE_VOID}, {"value"}, TAG_GLOBAL, true},
	{"bitCount", TYPE_UVEC3, {TYPE_UVEC3, TYPE_VOID}, {"value"}, TAG_GLOBAL, true},
	{"bitCount", TYPE_UVEC4, {TYPE_UVEC4, TYPE_VOID}, {"value"}, TAG_GLOBAL, true},

	// findLSB

	{"findLSB", TYPE_INT, {TYPE_INT, TYPE_VOID}, {"value"}, TAG_GLOBAL, true},
	{"findLSB", TYPE_IVEC2, {TYPE_IVEC2, TYPE_VOID}, {"value"}, TAG_GLOBAL, true},
	{"findLSB", TYPE_IVEC3, {TYPE_IVEC3, TYPE_VOID}, {"value"}, TAG_GLOBAL, true},
	{"findLSB", TYPE_IVEC4, {TYPE_IVEC4, TYPE_VOID}, {"value"}, TAG_GLOBAL, true},

	{"findLSB", TYPE_UINT, {TYPE_UINT, TYPE_VOID}, {"value"}, TAG_GLOBAL, true},
	{"findLSB", TYPE_UVEC2, {TYPE_UVEC2, TYPE_VOID}, {"value"}, TAG_GLOBAL, true},
	{"findLSB", TYPE_UVEC3, {TYPE_UVEC3, TYPE_VOID}, {"value"}, TAG_GLOBAL, true},
	{"findLSB", TYPE_UVEC4, {TYPE_UVEC4, TYPE_VOID}, {"value"}, TAG_GLOBAL, true},

	// findMSB

	{"findMSB", TYPE_INT, {TYPE_INT, TYPE_VOID}, {"value"}, TAG_GLOBAL, true},
	{"findMSB", TYPE_IVEC2, {TYPE_IVEC2, TYPE_VOID}, {"value"}, TAG_GLOBAL, true},
	{"findMSB", TYPE_IVEC3, {TYPE_IVEC3, TYPE_VOID}, {"value"}, TAG_GLOBAL, true},
	{"findMSB", TYPE_IVEC4, {TYPE_IVEC4, TYPE_VOID}, {"value"}, TAG_GLOBAL, true},

	{"findMSB", TYPE_UINT, {TYPE_UINT, TYPE_VOID}, {"value"}, TAG_GLOBAL, true},
	{"findMSB", TYPE_UVEC2, {TYPE_UVEC2, TYPE_VOID}, {"value"}, TAG_GLOBAL, true},
	{"findMSB", TYPE_UVEC3, {TYPE_UVEC3, TYPE_VOID}, {"value"}, TAG_GLOBAL, true},
	{"findMSB", TYPE_UVEC4, {TYPE_UVEC4, TYPE_VOID}, {"value"}, TAG_GLOBAL, true},

	// umulExtended

	{"umulExtended", TYPE_VOID, {TYPE_UINT, TYPE_UINT, TYPE_UINT, TYPE_UINT, TYPE_VOID},
		{"x", "y", "msb", "lsb"}, TAG_GLOBAL, true},
	{"umulExtended", TYPE_VOID, {TYPE_UVEC2, TYPE_UVEC2, TYPE_UVEC2, TYPE_UVEC2, TYPE_VOID},
		{"x", "y", "msb", "lsb"}, TAG_GLOBAL, true},
	{"umulExtended", TYPE_VOID, {TYPE_UVEC3, TYPE_UVEC3, TYPE_UVEC3, TYPE_UVEC3, TYPE_VOID},
		{"x", "y", "msb", "lsb"}, TAG_GLOBAL, true},
	{"umulExtended", TYPE_VOID, {TYPE_UVEC4, TYPE_UVEC4, TYPE_UVEC4, TYPE_UVEC4, TYPE_VOID},
		{"x", "y", "msb", "lsb"}, TAG_GLOBAL, true},

	// imulExtended

	{"imulExtended", TYPE_VOID, {TYPE_INT, TYPE_INT, TYPE_INT, TYPE_INT, TYPE_VOID},
		{"x", "y", "msb", "lsb"}, TAG_GLOBAL, true},
	{"imulExtended", TYPE_VOID, {TYPE_IVEC2, TYPE_IVEC2, TYPE_IVEC2, TYPE_IVEC2, TYPE_VOID},
		{"x", "y", "msb", "lsb"}, TAG_GLOBAL, true},
	{"imulExtended", TYPE_VOID, {TYPE_IVEC3, TYPE_IVEC3, TYPE_IVEC3, TYPE_IVEC3, TYPE_VOID},
		{"x", "y", "msb", "lsb"}, TAG_GLOBAL, true},
	{"imulExtended", TYPE_VOID, {TYPE_IVEC4, TYPE_IVEC4, TYPE_IVEC4, TYPE_IVEC4, TYPE_VOID},
		{"x", "y", "msb", "lsb"}, TAG_GLOBAL, true},

	// uaddCarry

	{"uaddCarry", TYPE_UINT, {TYPE_UINT, TYPE_UINT, TYPE_UINT, TYPE_VOID}, {"x", "y", "carry"},
		TAG_GLOBAL, true},
	{"uaddCarry", TYPE_UVEC2, {TYPE_UVEC2, TYPE_UVEC2, TYPE_UVEC2, TYPE_VOID}, {"x", "y", "carry"},
		TAG_GLOBAL, true},
	{"uaddCarry", TYPE_UVEC3, {TYPE_UVEC3, TYPE_UVEC3, TYPE_UVEC3, TYPE_VOID}, {"x", "y", "carry"},
		TAG_GLOBAL, true},
	{"uaddCarry", TYPE_UVEC4, {TYPE_UVEC4, TYPE_UVEC4, TYPE_UVEC4, TYPE_VOID}, {"x", "y", "carry"},
		TAG_GLOBAL, true},

	// usubBorrow

	{"usubBorrow", TYPE_UINT, {TYPE_UINT, TYPE_UINT, TYPE_UINT, TYPE_VOID}, {"x", "y", "borrow"},
		TAG_GLOBAL, true},
	{"usubBorrow", TYPE_UVEC2, {TYPE_UVEC2, TYPE_UVEC2, TYPE_UVEC2, TYPE_VOID},
		{"x", "y", "borrow"}, TAG_GLOBAL, true},
	{"usubBorrow", TYPE_UVEC3, {TYPE_UVEC3, TYPE_UVEC3, TYPE_UVEC3, TYPE_VOID},
		{"x", "y", "borrow"}, TAG_GLOBAL, true},
	{"usubBorrow", TYPE_UVEC4, {TYPE_UVEC4, TYPE_UVEC4, TYPE_UVEC4, TYPE_VOID},
		{"x", "y", "borrow"}, TAG_GLOBAL, true},

	// ldexp

	{"ldexp", TYPE_FLOAT, {TYPE_FLOAT, TYPE_INT, TYPE_VOID}, {"x", "exp"}, TAG_GLOBAL, true},
	{"ldexp", TYPE_VEC2, {TYPE_VEC2, TYPE_IVEC2, TYPE_VOID}, {"x", "exp"}, TAG_GLOBAL, true},
	{"ldexp", TYPE_VEC3, {TYPE_VEC3, TYPE_IVEC3, TYPE_VOID}, {"x", "exp"}, TAG_GLOBAL, true},
	{"ldexp", TYPE_VEC4, {TYPE_VEC4, TYPE_IVEC4, TYPE_VOID}, {"x", "exp"}, TAG_GLOBAL, true},

	// frexp

	{"frexp", TYPE_FLOAT, {TYPE_FLOAT, TYPE_INT, TYPE_VOID}, {"x", "exp"}, TAG_GLOBAL, true},
	{"frexp", TYPE_VEC2, {TYPE_VEC2, TYPE_IVEC2, TYPE_VOID}, {"x", "exp"}, TAG_GLOBAL, true},
	{"frexp", TYPE_VEC3, {TYPE_VEC3, TYPE_IVEC3, TYPE_VOID}, {"x", "exp"}, TAG_GLOBAL, true},
	{"frexp", TYPE_VEC4, {TYPE_VEC4, TYPE_IVEC4, TYPE_VOID}, {"x", "exp"}, TAG_GLOBAL, true},

	{nullptr, TYPE_VOID, {TYPE_VOID}, {""}, TAG_GLOBAL, false}};

static HashSet<String> global_func_set;

const ShaderLanguage::BuiltinFuncOutArgs ShaderLanguage::builtin_func_out_args[] = {
	{"modf", {1, -1}}, {"umulExtended", {2, 3}}, {"imulExtended", {2, 3}}, {"uaddCarry", {2, -1}},
	{"usubBorrow", {2, -1}}, {"ldexp", {1, -1}}, {"frexp", {1, -1}}, {nullptr, {0, -1}}};

const ShaderLanguage::BuiltinFuncConstArgs ShaderLanguage::builtin_func_const_args[] = {
	{"textureGather", 2, 0, 3}, {nullptr, 0, 0, 0}};

const ShaderLanguage::BuiltinEntry ShaderLanguage::frag_only_func_defs[] = {{"dFdx"},
	{"dFdxCoarse"}, {"dFdxFine"}, {"dFdy"}, {"dFdyCoarse"}, {"dFdyFine"}, {"fwidth"},
	{"fwidthCoarse"}, {"fwidthFine"}, {"textureQueryLod"}, {nullptr}};

bool ShaderLanguage::is_const_suffix_lut_initialized = false;

bool ShaderLanguage::_validate_function_call(BlockNode* p_block,
	const FunctionInfo& p_function_info, OperatorNode* p_func, DataType* r_ret_type,
	String* r_ret_type_str, bool* r_is_custom_function)
{
	ERR_FAIL_COND_V(p_func->op != OP_CALL && p_func->op != OP_CONSTRUCT, false);

	Vector<DataType> args;
	Vector<String> args2;
	Vector<int> args3;

	ERR_FAIL_COND_V(p_func->arguments[0]->type != Node::NODE_TYPE_VARIABLE, false);

	String name = static_cast<VariableNode*>(p_func->arguments[0])->name;
	String rname = static_cast<VariableNode*>(p_func->arguments[0])->rname;

	for (int i = 1; i < p_func->arguments.size(); i++) {
		args.push_back(p_func->arguments[i]->get_datatype());
		args2.push_back(p_func->arguments[i]->get_datatype_name());
		args3.push_back(p_func->arguments[i]->get_array_size());
	}

	int argcount = args.size();

	if (stages) {
		// Stage functions can be used in custom functions as well, that why need to check them all.
		for (const KeyValue<String, FunctionInfo>& E : *stages) {
			if (E.value.stage_functions.has(name)) {
				// Stage-based function.
				const StageFunctionInfo& sf = E.value.stage_functions[name];
				if (argcount != sf.arguments.size()) {
					_set_error(vformat(RTR("Invalid number of arguments when calling stage "
										   "function '%s', which expects %d argument(s)."),
						String(name), sf.arguments.size()));
					return false;
				}
				// Validate arguments.
				for (int i = 0; i < argcount; i++) {
					if (args[i] != sf.arguments[i].type) {
						_set_error(vformat(RTR("Invalid argument type when calling stage function "
											   "'%s', type expected is '%s'."),
							String(name), get_datatype_name(sf.arguments[i].type)));
						return false;
					}
				}

				if (r_ret_type) {
					*r_ret_type = sf.return_type;
				}
				if (r_ret_type_str) {
					*r_ret_type_str = "";
				}
				return true;
			}
		}
	}

	bool failed_builtin = false;
	bool unsupported_builtin = false;
	int builtin_idx = 0;

	if (argcount <= 4) {
		// Test builtins.
		int idx = 0;
		int full_match_index = 0;

		while (builtin_func_defs[idx].name) {
			if (completion_class != builtin_func_defs[idx].tag) {
				idx++;
				continue;
			}

			if (name == builtin_func_defs[idx].name) {
				bool fail = false;

				for (int i = 0; i < argcount; i++) {
					if (args[i] != builtin_func_defs[idx].args[i]) {
						fail = true;
						break;
					}
				}

				if (!fail) {
					// Full match.
					full_match_index = idx;
					break;
				}
			}

			idx++;
		}
		idx = full_match_index;

		while (builtin_func_defs[idx].name) {
			if (completion_class != builtin_func_defs[idx].tag) {
				idx++;
				continue;
			}

			if (name == builtin_func_defs[idx].name) {
				failed_builtin = true;
				bool fail = false;
				for (int i = 0; i < argcount; i++) {
					if (p_func->arguments[i + 1]->type == Node::NODE_TYPE_ARRAY) {
						const ArrayNode* anode =
							static_cast<const ArrayNode*>(p_func->arguments[i + 1]);
						if (anode->call_expression == nullptr && !anode->is_indexed()) {
							fail = true;
							break;
						}
					}
					if (get_scalar_type(args[i]) == args[i] &&
						p_func->arguments[i + 1]->type == Node::NODE_TYPE_CONSTANT &&
						convert_constant(static_cast<ConstantNode*>(p_func->arguments[i + 1]),
							builtin_func_defs[idx].args[i])) {
						// All good, but needs implicit conversion later.
					}
					else if (args[i] != builtin_func_defs[idx].args[i]) {
						fail = true;
						break;
					}
				}

				if (!fail) {
					if (Renderer::is_low_end()) {
						if (builtin_func_defs[idx].high_end) {
							fail = true;
							unsupported_builtin = true;
							builtin_idx = idx;
						}
					}
				}

				if (!fail && argcount < 4 && builtin_func_defs[idx].args[argcount] != TYPE_VOID) {
					fail = true; // Make sure the number of arguments matches.
				}

				if (!fail) {
					{
						int constarg_idx = 0;
						while (builtin_func_const_args[constarg_idx].name) {
							if (String(name) == builtin_func_const_args[constarg_idx].name) {
								int arg = builtin_func_const_args[constarg_idx].arg + 1;
								if (p_func->arguments.size() <= arg) {
									break;
								}

								int min = builtin_func_const_args[constarg_idx].min;
								int max = builtin_func_const_args[constarg_idx].max;

								bool error = false;
								Vector<Scalar> values = _get_node_values(
									p_block, p_function_info, p_func->arguments[arg]);
								if (p_func->arguments[arg]->get_datatype() == TYPE_INT &&
									!values.is_empty()) {
									if (values[0].sint < min || values[0].sint > max) {
										error = true;
									}
								}
								else {
									error = true;
								}

								if (error) {
									_set_error(vformat(
										RTR("Expected integer constant within [%d..%d] range."),
										min, max));
									return false;
								}
							}
							constarg_idx++;
						}
					}

					// Make sure it's not an out argument used in the wrong way.
					int outarg_idx = 0;
					while (builtin_func_out_args[outarg_idx].name) {
						if (String(name) == builtin_func_out_args[outarg_idx].name) {
							for (int arg = 0; arg < BuiltinFuncOutArgs::MAX_ARGS; arg++) {
								int arg_idx = builtin_func_out_args[outarg_idx].arguments[arg];
								if (arg_idx == -1) {
									break;
								}
								if (arg_idx < argcount) {
									if (p_func->arguments[arg_idx + 1]->type !=
											Node::NODE_TYPE_VARIABLE &&
										p_func->arguments[arg_idx + 1]->type !=
											Node::NODE_TYPE_MEMBER &&
										p_func->arguments[arg_idx + 1]->type !=
											Node::NODE_TYPE_ARRAY) {
										_set_error(vformat(RTR("Argument %d of function '%s' is "
															   "not a variable, array, or member."),
											arg_idx + 1, String(name)));
										return false;
									}

									if (p_func->arguments[arg_idx + 1]->type ==
										Node::NODE_TYPE_ARRAY) {
										ArrayNode* mn =
											static_cast<ArrayNode*>(p_func->arguments[arg_idx + 1]);
										if (mn->is_const) {
											fail = true;
										}
									}
									else if (p_func->arguments[arg_idx + 1]->type ==
											   Node::NODE_TYPE_MEMBER) {
										MemberNode* mn = static_cast<MemberNode*>(
											p_func->arguments[arg_idx + 1]);
										if (mn->basetype_const) {
											fail = true;
										}
									}
									else { // TYPE_VARIABLE
										VariableNode* vn = static_cast<VariableNode*>(
											p_func->arguments[arg_idx + 1]);
										if (vn->is_const) {
											fail = true;
										}
										else {
											String varname = vn->name;
											if (shader->uniforms.has(varname)) {
												fail = true;
											}
											else {
												if (shader->varyings.has(varname)) {
													_set_error(
														vformat(RTR("Varyings cannot be passed for "
																	"the '%s' parameter."),
															"out"));
													return false;
												}
												if (p_function_info.built_ins.has(varname)) {
													BuiltInInfo info =
														p_function_info.built_ins[varname];
													if (info.constant) {
														fail = true;
													}
												}
											}
										}
									}
									if (fail) {
										_set_error(vformat(RTR("A constant value cannot be passed "
															   "for the '%s' parameter."),
											"out"));
										return false;
									}

									String var_name;
									if (p_func->arguments[arg_idx + 1]->type ==
										Node::NODE_TYPE_ARRAY) {
										var_name = static_cast<const ArrayNode*>(
											p_func->arguments[arg_idx + 1])
													   ->name;
									}
									else if (p_func->arguments[arg_idx + 1]->type ==
											   Node::NODE_TYPE_MEMBER) {
										Node* n = static_cast<const MemberNode*>(
											p_func->arguments[arg_idx + 1])
													  ->owner;
										while (n->type == Node::NODE_TYPE_MEMBER) {
											n = static_cast<const MemberNode*>(n)->owner;
										}
										if (n->type != Node::NODE_TYPE_VARIABLE &&
											n->type != Node::NODE_TYPE_ARRAY) {
											_set_error(
												vformat(RTR("Argument %d of function '%s' is not a "
															"variable, array, or member."),
													arg_idx + 1, String(name)));
											return false;
										}
										if (n->type == Node::NODE_TYPE_VARIABLE) {
											var_name = static_cast<const VariableNode*>(n)->name;
										}
										else { // TYPE_ARRAY
											var_name = static_cast<const ArrayNode*>(n)->name;
										}
									}
									else { // TYPE_VARIABLE
										var_name = static_cast<const VariableNode*>(
											p_func->arguments[arg_idx + 1])
													   ->name;
									}
									const BlockNode* b = p_block;
									bool valid = false;
									while (b) {
										if (b->variables.has(var_name) ||
											p_function_info.built_ins.has(var_name)) {
											valid = true;
											break;
										}
										if (b->parent_function) {
											for (int i = 0;
												 i < b->parent_function->arguments.size(); i++) {
												if (b->parent_function->arguments[i].name ==
													var_name) {
													valid = true;
													break;
												}
											}
										}
										b = b->parent_block;
									}

									if (!valid) {
										_set_error(
											vformat(RTR("Argument %d of function '%s' can only "
														"take a local variable, array, or member."),
												arg_idx + 1, String(name)));
										return false;
									}
								}
							}
						}
						outarg_idx++;
					}
					// Implicitly convert values if possible.
					for (int i = 0; i < argcount; i++) {
						if (get_scalar_type(args[i]) != args[i] ||
							args[i] == builtin_func_defs[idx].args[i] ||
							p_func->arguments[i + 1]->type != Node::NODE_TYPE_CONSTANT) {
							// Can't do implicit conversion here.
							continue;
						}

						// This is an implicit conversion.
						ConstantNode* constant =
							static_cast<ConstantNode*>(p_func->arguments[i + 1]);
						ConstantNode* conversion = alloc_node<ConstantNode>();

						conversion->datatype = builtin_func_defs[idx].args[i];
						conversion->values.resize(1);

						convert_constant(
							constant, builtin_func_defs[idx].args[i], conversion->values.ptrw());
						p_func->arguments.write[i + 1] = conversion;
					}

					if (r_ret_type) {
						*r_ret_type = builtin_func_defs[idx].rettype;
					}

					return true;
				}
			}

			idx++;
		}
	}

	if (unsupported_builtin) {
		String arglist = "";
		for (int i = 0; i < argcount; i++) {
			if (i > 0) {
				arglist += ", ";
			}
			arglist += get_datatype_name(builtin_func_defs[builtin_idx].args[i]);
		}

		_set_error(
			vformat(RTR("Built-in function \"%s(%s)\" is only supported on high-end platforms."),
				String(name), arglist));
		return false;
	}

	if (failed_builtin) {
		String arg_list;
		for (int i = 0; i < argcount; i++) {
			if (i > 0) {
				arg_list += ",";
			}

			String arg_name;
			if (args[i] == TYPE_STRUCT) {
				arg_name = args2[i];
			}
			else {
				arg_name = get_datatype_name(args[i]);
			}
			if (args3[i] > 0) {
				arg_name += "[";
				arg_name += itos(args3[i]);
				arg_name += "]";
			}
			arg_list += arg_name;
		}
		_set_error(vformat(RTR("Invalid arguments for the built-in function: \"%s(%s)\"."),
			String(name), arg_list));
		return false;
	}

	// try existing functions..

	String exclude_function;
	BlockNode* block = p_block;

	while (block) {
		if (block->parent_function) {
			exclude_function = block->parent_function->name;
		}
		block = block->parent_block;
	}

	if (name == exclude_function) {
		_set_error(RTR("Recursion is not allowed."));
		return false;
	}

	int last_arg_count = 0;
	bool exists = false;
	String arg_list = "";
	bool overload_fail = false;

	struct OverloadErrorInfo
	{
		String arg_list;
		int index = 0;
		String func_arg_name;
		String arg_name;
	};

	Vector<OverloadErrorInfo> overload_errors;

	for (int i = 0; i < shader->vfunctions.size(); i++) {
		if (rname != shader->vfunctions[i].rname) {
			continue;
		}
		exists = true;

		if (!shader->vfunctions[i].callable) {
			_set_error(
				vformat(RTR("Function '%s' can't be called from source code."), String(name)));
			return false;
		}

		FunctionNode* pfunc = shader->vfunctions[i].function;
		arg_list.clear();
		for (int j = 0; j < pfunc->arguments.size(); j++) {
			if (j > 0) {
				arg_list += ", ";
			}
			String func_arg_name;
			if (pfunc->arguments[j].type == TYPE_STRUCT) {
				func_arg_name = pfunc->arguments[j].struct_name;
			}
			else {
				func_arg_name = get_datatype_name(pfunc->arguments[j].type);
			}
			if (pfunc->arguments[j].array_size > 0) {
				func_arg_name += "[";
				func_arg_name += itos(pfunc->arguments[j].array_size);
				func_arg_name += "]";
			}
			arg_list += func_arg_name;
		}

		if (pfunc->arguments.size() != args.size()) {
			last_arg_count = pfunc->arguments.size();
			continue;
		}

		bool fail = false;
		bool use_constant_conversion = function_overload_count[rname] == 0;

		for (int j = 0; j < args.size(); j++) {
			if (use_constant_conversion && get_scalar_type(args[j]) == args[j] &&
				p_func->arguments[j + 1]->type == Node::NODE_TYPE_CONSTANT && args3[j] == 0 &&
				convert_constant(static_cast<ConstantNode*>(p_func->arguments[j + 1]),
					pfunc->arguments[j].type)) {
				// all good, but it needs implicit conversion later
			}
			else if (args[j] != pfunc->arguments[j].type ||
					   (args[j] == TYPE_STRUCT && args2[j] != pfunc->arguments[j].struct_name) ||
					   args3[j] != pfunc->arguments[j].array_size) {
				String func_arg_name;
				if (pfunc->arguments[j].type == TYPE_STRUCT) {
					func_arg_name = pfunc->arguments[j].struct_name;
				}
				else {
					func_arg_name = get_datatype_name(pfunc->arguments[j].type);
				}
				if (pfunc->arguments[j].array_size > 0) {
					func_arg_name += "[";
					func_arg_name += itos(pfunc->arguments[j].array_size);
					func_arg_name += "]";
				}
				String arg_name;
				if (args[j] == TYPE_STRUCT) {
					arg_name = args2[j];
				}
				else {
					arg_name = get_datatype_name(args[j]);
				}
				if (args3[j] > 0) {
					arg_name += "[";
					arg_name += itos(args3[j]);
					arg_name += "]";
				}

				fail = true;
				OverloadErrorInfo err_info;
				err_info.arg_list = arg_list;
				err_info.index = j + 1;
				err_info.func_arg_name = func_arg_name;
				err_info.arg_name = arg_name;
				overload_errors.push_back(err_info);
				overload_fail = true;
				break;
			}
			else {
				overload_fail = false;
			}
		}

		if (!fail) {
			// implicitly convert values if possible
			for (int k = 0; k < args.size(); k++) {
				if (get_scalar_type(args[k]) != args[k] || args[k] == pfunc->arguments[k].type ||
					p_func->arguments[k + 1]->type != Node::NODE_TYPE_CONSTANT) {
					// can't do implicit conversion here
					continue;
				}

				// this is an implicit conversion
				ConstantNode* constant = static_cast<ConstantNode*>(p_func->arguments[k + 1]);
				ConstantNode* conversion = alloc_node<ConstantNode>();

				conversion->datatype = pfunc->arguments[k].type;
				conversion->values.resize(1);

				convert_constant(constant, pfunc->arguments[k].type, conversion->values.ptrw());
				p_func->arguments.write[k + 1] = conversion;
			}

			if (r_ret_type) {
				*r_ret_type = pfunc->return_type;
				if (pfunc->return_type == TYPE_STRUCT) {
					*r_ret_type_str = pfunc->return_struct_name;
				}
			}

			if (r_is_custom_function) {
				*r_is_custom_function = true;
			}
			return true;
		}
	}
	if (overload_fail) {
		String err_str;
		if (overload_errors.size() == 1) {
			const OverloadErrorInfo& err_info = overload_errors[0];
			err_str = vformat(
				"No matching function for \"%s(%s)\" call: argument %d should be %s but is %s.",
				String(rname), err_info.arg_list, err_info.index, err_info.func_arg_name,
				err_info.arg_name);
		}
		else {
			err_str = vformat(RTR("No matching function for \"%s\" call:"), String(rname));
			for (const OverloadErrorInfo& err_info : overload_errors) {
				err_str += "\n\t" + vformat(RTR("candidate function \"%s(%s)\" not viable, "
												"argument %d should be %s but is %s."),
										String(rname), err_info.arg_list, err_info.index,
										err_info.func_arg_name, err_info.arg_name);
			}
		}
		_set_error(err_str);
	}

	if (exists) {
		if (last_arg_count > args.size()) {
			_set_error(vformat(
				RTR("Too few arguments for \"%s(%s)\" call. Expected at least %d but received %d."),
				String(rname), arg_list, last_arg_count, args.size()));
		}
		else if (last_arg_count < args.size()) {
			_set_error(vformat(
				RTR("Too many arguments for \"%s(%s)\" call. Expected at most %d but received %d."),
				String(rname), arg_list, last_arg_count, args.size()));
		}
	}

	return false;
}

bool ShaderLanguage::_compare_datatypes(DataType p_datatype_a, String p_datatype_name_a,
	int p_array_size_a, DataType p_datatype_b, String p_datatype_name_b, int p_array_size_b)
{
	bool result = true;

	if (p_datatype_a == TYPE_STRUCT || p_datatype_b == TYPE_STRUCT) {
		if (p_datatype_name_a != p_datatype_name_b) {
			result = false;
		}
	}
	else {
		if (p_datatype_a != p_datatype_b) {
			result = false;
		}
	}

	if (p_array_size_a != p_array_size_b) {
		result = false;
	}

	if (!result) {
		String type_name =
			p_datatype_a == TYPE_STRUCT ? p_datatype_name_a : get_datatype_name(p_datatype_a);
		if (p_array_size_a > 0) {
			type_name += "[";
			type_name += itos(p_array_size_a);
			type_name += "]";
		}

		String type_name2 =
			p_datatype_b == TYPE_STRUCT ? p_datatype_name_b : get_datatype_name(p_datatype_b);
		if (p_array_size_b > 0) {
			type_name2 += "[";
			type_name2 += itos(p_array_size_b);
			type_name2 += "]";
		}

		_set_error(vformat(RTR("Invalid assignment of '%s' to '%s'."), type_name2, type_name));
	}
	return result;
}

bool ShaderLanguage::_compare_datatypes_in_nodes(Node* a, Node* b)
{
	return _compare_datatypes(a->get_datatype(), a->get_datatype_name(), a->get_array_size(),
		b->get_datatype(), b->get_datatype_name(), b->get_array_size());
}

bool ShaderLanguage::_parse_function_arguments(BlockNode* p_block,
	const FunctionInfo& p_function_info, OperatorNode* p_func, int* r_complete_arg)
{
	TkPos pos = _get_tkpos();
	Token tk = _get_token();

	if (tk.type == TK_PARENTHESIS_CLOSE) {
		return true;
	}

	_set_tkpos(pos);

	while (true) {
		if (r_complete_arg) {
			pos = _get_tkpos();
			tk = _get_token();

			if (tk.type == TK_CURSOR) {
				*r_complete_arg = p_func->arguments.size() - 1;
			}
			else {
				_set_tkpos(pos);
			}
		}

		Node* arg = _parse_and_reduce_expression(p_block, p_function_info);

		if (!arg) {
			return false;
		}

		if (is_const_decl && arg->type == Node::NODE_TYPE_VARIABLE) {
			const VariableNode* var = static_cast<const VariableNode*>(arg);
			if (!var->is_const) {
				_set_error(RTR("Expected constant expression."));
				return false;
			}
		}

		p_func->arguments.push_back(arg);

		tk = _get_token();

		if (tk.type == TK_PARENTHESIS_CLOSE) {
			return true;
		}
		else if (tk.type != TK_COMMA) {
			// something is broken
			_set_error(RTR("Expected ',' or ')' after argument."));
			return false;
		}
	}

	return true;
}

bool ShaderLanguage::is_token_operator(TokenType p_type)
{
	return (
		p_type == TK_OP_EQUAL || p_type == TK_OP_NOT_EQUAL || p_type == TK_OP_LESS ||
		p_type == TK_OP_LESS_EQUAL || p_type == TK_OP_GREATER || p_type == TK_OP_GREATER_EQUAL ||
		p_type == TK_OP_AND || p_type == TK_OP_OR || p_type == TK_OP_NOT || p_type == TK_OP_ADD ||
		p_type == TK_OP_SUB || p_type == TK_OP_MUL || p_type == TK_OP_DIV || p_type == TK_OP_MOD ||
		p_type == TK_OP_SHIFT_LEFT || p_type == TK_OP_SHIFT_RIGHT || p_type == TK_OP_ASSIGN ||
		p_type == TK_OP_ASSIGN_ADD || p_type == TK_OP_ASSIGN_SUB || p_type == TK_OP_ASSIGN_MUL ||
		p_type == TK_OP_ASSIGN_DIV || p_type == TK_OP_ASSIGN_MOD ||
		p_type == TK_OP_ASSIGN_SHIFT_LEFT || p_type == TK_OP_ASSIGN_SHIFT_RIGHT ||
		p_type == TK_OP_ASSIGN_BIT_AND || p_type == TK_OP_ASSIGN_BIT_OR ||
		p_type == TK_OP_ASSIGN_BIT_XOR || p_type == TK_OP_BIT_AND || p_type == TK_OP_BIT_OR ||
		p_type == TK_OP_BIT_XOR || p_type == TK_OP_BIT_INVERT || p_type == TK_OP_INCREMENT ||
		p_type == TK_OP_DECREMENT || p_type == TK_QUESTION || p_type == TK_COLON);
}

bool ShaderLanguage::is_token_operator_assign(TokenType p_type)
{
	return (p_type == TK_OP_ASSIGN || p_type == TK_OP_ASSIGN_ADD || p_type == TK_OP_ASSIGN_SUB ||
			p_type == TK_OP_ASSIGN_MUL || p_type == TK_OP_ASSIGN_DIV ||
			p_type == TK_OP_ASSIGN_MOD || p_type == TK_OP_ASSIGN_SHIFT_LEFT ||
			p_type == TK_OP_ASSIGN_SHIFT_RIGHT || p_type == TK_OP_ASSIGN_BIT_AND ||
			p_type == TK_OP_ASSIGN_BIT_OR || p_type == TK_OP_ASSIGN_BIT_XOR);
}

bool ShaderLanguage::is_token_hint(TokenType p_type)
{
	return int(p_type) > int(TK_STENCIL_MODE) && int(p_type) < int(TK_SHADER_TYPE);
}

bool ShaderLanguage::convert_constant(ConstantNode* p_constant, DataType p_to_type, Scalar* p_value)
{
	if (p_constant->datatype == p_to_type) {
		if (p_value) {
			for (int i = 0; i < p_constant->values.size(); i++) {
				p_value[i] = p_constant->values[i];
			}
		}
		return true;
	}
	else if (p_constant->datatype == TYPE_INT && p_to_type == TYPE_FLOAT) {
		if (p_value) {
			p_value->real = p_constant->values[0].sint;
		}
		return true;
	}
	else if (p_constant->datatype == TYPE_UINT && p_to_type == TYPE_FLOAT) {
		if (p_value) {
			p_value->real = p_constant->values[0].uint;
		}
		return true;
	}
	else if (p_constant->datatype == TYPE_INT && p_to_type == TYPE_UINT) {
		if (p_constant->values[0].sint < 0) {
			return false;
		}
		if (p_value) {
			p_value->uint = p_constant->values[0].sint;
		}
		return true;
	}
	else if (p_constant->datatype == TYPE_UINT && p_to_type == TYPE_INT) {
		if (p_constant->values[0].uint > 0x7FFFFFFF) {
			return false;
		}
		if (p_value) {
			p_value->sint = p_constant->values[0].uint;
		}
		return true;
	}
	else if (p_constant->datatype == TYPE_BOOL && p_to_type == TYPE_FLOAT) {
		if (p_value) {
			p_value->real = p_constant->values[0].boolean ? 1.0f : 0.0f;
		}
		return true;
	}
	else if (p_constant->datatype == TYPE_BOOL && p_to_type == TYPE_UINT) {
		if (p_value) {
			p_value->uint = p_constant->values[0].boolean ? 1U : 0U;
		}
		return true;
	}
	else if (p_constant->datatype == TYPE_BOOL && p_to_type == TYPE_INT) {
		if (p_value) {
			p_value->sint = p_constant->values[0].boolean ? 1 : 0;
		}
		return true;
	}
	else {
		return false;
	}
}

bool ShaderLanguage::is_scalar_type(DataType p_type)
{
	return p_type == TYPE_BOOL || p_type == TYPE_INT || p_type == TYPE_UINT || p_type == TYPE_FLOAT;
}

bool ShaderLanguage::is_float_type(DataType p_type)
{
	switch (p_type) {
	case TYPE_FLOAT:
	case TYPE_VEC2:
	case TYPE_VEC3:
	case TYPE_VEC4:
	case TYPE_MAT2:
	case TYPE_MAT3:
	case TYPE_MAT4:
	case TYPE_SAMPLER2D:
	case TYPE_SAMPLER2DARRAY:
	case TYPE_SAMPLER3D:
	case TYPE_SAMPLERCUBE:
	case TYPE_SAMPLERCUBEARRAY: {
		return true;
	}
	default: {
		return false;
	}
	}
}

bool ShaderLanguage::is_sampler_type(DataType p_type)
{
	return p_type > TYPE_MAT4 && p_type < TYPE_STRUCT;
}

bool ShaderLanguage::ShaderLanguage::is_hint_color(ShaderLanguage::ShaderNode::Uniform::Hint p_hint)
{
	return p_hint == ShaderLanguage::ShaderNode::Uniform::HINT_SOURCE_COLOR ||
		   p_hint == ShaderLanguage::ShaderNode::Uniform::HINT_COLOR_CONVERSION_DISABLED;
}

uint32_t ShaderLanguage::get_datatype_size(ShaderLanguage::DataType p_type)
{
	switch (p_type) {
	case TYPE_VOID:
		return 0;
	case TYPE_BOOL:
		return 4;
	case TYPE_BVEC2:
		return 8;
	case TYPE_BVEC3:
		return 12;
	case TYPE_BVEC4:
		return 16;
	case TYPE_INT:
		return 4;
	case TYPE_IVEC2:
		return 8;
	case TYPE_IVEC3:
		return 12;
	case TYPE_IVEC4:
		return 16;
	case TYPE_UINT:
		return 4;
	case TYPE_UVEC2:
		return 8;
	case TYPE_UVEC3:
		return 12;
	case TYPE_UVEC4:
		return 16;
	case TYPE_FLOAT:
		return 4;
	case TYPE_VEC2:
		return 8;
	case TYPE_VEC3:
		return 12;
	case TYPE_VEC4:
		return 16;
	case TYPE_MAT2:
		return 32; // 4 * 4 + 4 * 4
	case TYPE_MAT3:
		return 48; // 4 * 4 + 4 * 4 + 4 * 4
	case TYPE_MAT4:
		return 64;
	case TYPE_SAMPLER2D:
		return 16;
	case TYPE_ISAMPLER2D:
		return 16;
	case TYPE_USAMPLER2D:
		return 16;
	case TYPE_SAMPLER2DARRAY:
		return 16;
	case TYPE_ISAMPLER2DARRAY:
		return 16;
	case TYPE_USAMPLER2DARRAY:
		return 16;
	case TYPE_SAMPLER3D:
		return 16;
	case TYPE_ISAMPLER3D:
		return 16;
	case TYPE_USAMPLER3D:
		return 16;
	case TYPE_SAMPLERCUBE:
		return 16;
	case TYPE_SAMPLERCUBEARRAY:
		return 16;
	case TYPE_SAMPLEREXT:
		return 16;
	case TYPE_STRUCT:
		return 0;
	case TYPE_MAX: {
		ERR_FAIL_V(0);
	};
	}
	ERR_FAIL_V(0);
}

uint32_t ShaderLanguage::get_datatype_component_count(ShaderLanguage::DataType p_type)
{
	switch (p_type) {
	case TYPE_BOOL:
		return 1U;
	case TYPE_BVEC2:
		return 2U;
	case TYPE_BVEC3:
		return 3U;
	case TYPE_BVEC4:
		return 4U;
	case TYPE_INT:
		return 1U;
	case TYPE_IVEC2:
		return 2U;
	case TYPE_IVEC3:
		return 3U;
	case TYPE_IVEC4:
		return 4U;
	case TYPE_UINT:
		return 1U;
	case TYPE_UVEC2:
		return 2U;
	case TYPE_UVEC3:
		return 3U;
	case TYPE_UVEC4:
		return 4U;
	case TYPE_FLOAT:
		return 1U;
	case TYPE_VEC2:
		return 2U;
	case TYPE_VEC3:
		return 3U;
	case TYPE_VEC4:
		return 4U;
	case TYPE_MAT2:
		return 4U;
	case TYPE_MAT3:
		return 9U;
	case TYPE_MAT4:
		return 16U;
	default:
		break;
	}
	return 0U;
}

void ShaderLanguage::get_keyword_list(List<String>* r_keywords)
{
	HashSet<String> kws;

	int idx = 0;

	while (keyword_list[idx].text) {
		kws.insert(keyword_list[idx].text);
		idx++;
	}

	idx = 0;

	while (builtin_func_defs[idx].name) {
		kws.insert(builtin_func_defs[idx].name);

		idx++;
	}

	for (const String& E : kws) {
		r_keywords->push_back(E);
	}
}

bool ShaderLanguage::is_control_flow_keyword(String p_keyword)
{
	return p_keyword == "break" || p_keyword == "case" || p_keyword == "continue" ||
		   p_keyword == "default" || p_keyword == "do" || p_keyword ==
		"else" ||
		   p_keyword == "for" || p_keyword == "if" || p_keyword == "return" ||
		   p_keyword == "switch" || p_keyword == "while";
}

void ShaderLanguage::get_builtin_funcs(List<String>* r_keywords)
{
	HashSet<String> kws;

	int idx = 0;

	while (builtin_func_defs[idx].name) {
		kws.insert(builtin_func_defs[idx].name);

		idx++;
	}

	for (const String& E : kws) {
		r_keywords->push_back(E);
	}
}

ShaderLanguage::DataType ShaderLanguage::get_scalar_type(DataType p_type)
{
	static const DataType scalar_types[] = {
		TYPE_VOID,
		TYPE_BOOL,
		TYPE_BOOL,
		TYPE_BOOL,
		TYPE_BOOL,
		TYPE_INT,
		TYPE_INT,
		TYPE_INT,
		TYPE_INT,
		TYPE_UINT,
		TYPE_UINT,
		TYPE_UINT,
		TYPE_UINT,
		TYPE_FLOAT,
		TYPE_FLOAT,
		TYPE_FLOAT,
		TYPE_FLOAT,
		TYPE_FLOAT,
		TYPE_FLOAT,
		TYPE_FLOAT,
		TYPE_FLOAT,
		TYPE_INT,
		TYPE_UINT,
		TYPE_FLOAT,
		TYPE_INT,
		TYPE_UINT,
		TYPE_FLOAT,
		TYPE_INT,
		TYPE_UINT,
		TYPE_FLOAT,
		TYPE_FLOAT,
		TYPE_FLOAT,
		TYPE_VOID,
	};

	static_assert(std_size(scalar_types) == TYPE_MAX);

	return scalar_types[p_type];
}

int ShaderLanguage::get_cardinality(DataType p_type)
{
	static const int cardinality_table[] = {
		0,
		1,
		2,
		3,
		4,
		1,
		2,
		3,
		4,
		1,
		2,
		3,
		4,
		1,
		2,
		3,
		4,
		4,
		9,
		16,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
	};

	static_assert(std_size(cardinality_table) == TYPE_MAX);

	return cardinality_table[p_type];
}

bool ShaderLanguage::_get_completable_identifier(
	BlockNode* p_block, CompletionType p_type, String& identifier)
{
	identifier = String();

	TkPos pos = {0, 0};

	Token tk = _get_token();

	if (tk.type == TK_IDENTIFIER) {
		identifier = tk.text;
		pos = _get_tkpos();
		tk = _get_token();
	}

	if (tk.type == TK_CURSOR) {
		completion_type = p_type;
		completion_line = tk_line;
		completion_block = p_block;

		pos = _get_tkpos();
		tk = _get_token();

		if (tk.type == TK_IDENTIFIER) {
			identifier = identifier + tk.text;
		}
		else {
			_set_tkpos(pos);
		}
		return true;
	}
	else if (identifier != String()) {
		_set_tkpos(pos);
	}

	return false;
}

bool ShaderLanguage::_is_operator_assign(Operator p_op) const
{
	switch (p_op) {
	case OP_ASSIGN:
	case OP_ASSIGN_ADD:
	case OP_ASSIGN_SUB:
	case OP_ASSIGN_MUL:
	case OP_ASSIGN_DIV:
	case OP_ASSIGN_MOD:
	case OP_ASSIGN_SHIFT_LEFT:
	case OP_ASSIGN_SHIFT_RIGHT:
	case OP_ASSIGN_BIT_AND:
	case OP_ASSIGN_BIT_OR:
	case OP_ASSIGN_BIT_XOR:
		return true;
	default:
		return false;
	}
}

bool ShaderLanguage::_validate_varying_assign(ShaderNode::Varying& p_varying, String* r_message)
{
	if (current_function != "vertex" && current_function != "fragment") {
		*r_message =
			vformat(RTR("Varying may not be assigned in the '%s' function."), current_function);
		return false;
	}
	switch (p_varying.stage) {
	case ShaderNode::Varying::STAGE_UNKNOWN: // first assign
		if (current_function == varying_function_names.vertex) {
			if (p_varying.type < TYPE_INT) {
				*r_message = vformat(
					RTR("Varying with '%s' data type may only be assigned in the '%s' function."),
					get_datatype_name(p_varying.type), "fragment");
				return false;
			}
			p_varying.stage = ShaderNode::Varying::STAGE_VERTEX;
		}
		else if (current_function == varying_function_names.fragment) {
			p_varying.stage = ShaderNode::Varying::STAGE_FRAGMENT;
		}
		break;
	case ShaderNode::Varying::STAGE_VERTEX:
		if (current_function == varying_function_names.fragment) {
			*r_message = vformat(RTR("Varyings which assigned in '%s' function may not be "
									 "reassigned in '%s' or '%s'."),
				"vertex", "fragment", "light");
			return false;
		}
		break;
	case ShaderNode::Varying::STAGE_FRAGMENT:
		if (current_function == varying_function_names.vertex) {
			*r_message = vformat(RTR("Varyings which assigned in '%s' function may not be "
									 "reassigned in '%s' or '%s'."),
				"fragment", "vertex", "light");
			return false;
		}
		break;
	default:
		break;
	}
	return true;
}

bool ShaderLanguage::_check_node_constness(const Node* p_node) const
{
	switch (p_node->type) {
	case Node::NODE_TYPE_OPERATOR: {
		const OperatorNode* op_node = static_cast<const OperatorNode*>(p_node);
		for (int i = int(op_node->op == OP_CALL); i < op_node->arguments.size(); i++) {
			if (!_check_node_constness(op_node->arguments[i])) {
				return false;
			}
		}
	} break;
	case Node::NODE_TYPE_CONSTANT:
		break;
	case Node::NODE_TYPE_VARIABLE: {
		const VariableNode* var_node = static_cast<const VariableNode*>(p_node);
		if (!var_node->is_const) {
			return false;
		}
	} break;
	case Node::NODE_TYPE_ARRAY: {
		const ArrayNode* arr_node = static_cast<const ArrayNode*>(p_node);
		if (!arr_node->is_const) {
			return false;
		}
	} break;
	default:
		return false;
	}
	return true;
}

bool ShaderLanguage::_check_restricted_func(
	const String& p_name, const String& p_current_function) const
{
	int idx = 0;

	while (frag_only_func_defs[idx].name) {
		if (String(frag_only_func_defs[idx].name) == p_name) {
			if (is_supported_frag_only_funcs) {
				if (p_current_function == "vertex" && stages->has(p_current_function)) {
					return true;
				}
			}
			else {
				return true;
			}
			break;
		}
		idx++;
	}

	return false;
}

bool ShaderLanguage::_validate_restricted_func(
	const String& p_name, const CallInfo* p_func_info, bool p_is_builtin_hint)
{
	const bool is_in_restricted_function = p_func_info->name == "vertex";

	// No need to check up the hierarchy if it's a built-in.
	if (!p_is_builtin_hint) {
		for (const CallInfo* func_info : p_func_info->calls) {
			if (is_in_restricted_function && func_info->name != p_name) {
				// Skips check for non-called method.
				continue;
			}

			if (!_validate_restricted_func(p_name, func_info)) {
				return false;
			}
		}
	}

	if (!p_func_info->uses_restricted_items.is_empty()) {
		const Pair<String, CallInfo::Item>& first_element =
			p_func_info->uses_restricted_items.get(0);

		if (first_element.second.type == CallInfo::Item::ITEM_TYPE_VARYING) {
			const ShaderNode::Varying& varying = shader->varyings[first_element.first];

			if (varying.stage == ShaderNode::Varying::STAGE_VERTEX) {
				return true;
			}
		}

		_set_tkpos(first_element.second.pos);

		if (is_in_restricted_function) {
			_set_error(vformat(RTR("'%s' cannot be used within the '%s' processor function."),
				first_element.first, "vertex"));
		}
		else {
			_set_error(vformat(RTR("'%s' cannot be used here, because '%s' is called by the '%s' "
								   "processor function (which is not allowed)."),
				first_element.first, p_func_info->name, "vertex"));
		}
		return false;
	}

	return true;
}

bool ShaderLanguage::_validate_assign(
	Node* p_node, const FunctionInfo& p_function_info, String* r_message)
{
	if (p_node->type == Node::NODE_TYPE_OPERATOR) {
		OperatorNode* op = static_cast<OperatorNode*>(p_node);

		if (op->op == OP_INDEX) {
			return _validate_assign(op->arguments[0], p_function_info, r_message);

		}
		else if (_is_operator_assign(op->op)) {
			// chained assignment
			return _validate_assign(op->arguments[1], p_function_info, r_message);

		}
		else if (op->op == OP_CALL) {
			if (r_message) {
				*r_message = RTR("Assignment to function.");
			}
			return false;
		}

	}
	else if (p_node->type == Node::NODE_TYPE_MEMBER) {
		MemberNode* member = static_cast<MemberNode*>(p_node);

		if (member->has_swizzling_duplicates) {
			if (r_message) {
				*r_message = RTR("Swizzling assignment contains duplicates.");
			}
			return false;
		}

		return _validate_assign(member->owner, p_function_info, r_message);

	}
	else if (p_node->type == Node::NODE_TYPE_VARIABLE) {
		VariableNode* var = static_cast<VariableNode*>(p_node);

		if (shader->uniforms.has(var->name)) {
			if (r_message) {
				*r_message = RTR("Assignment to uniform.");
			}
			return false;
		}

		if (shader->constants.has(var->name) || var->is_const) {
			if (r_message) {
				*r_message = RTR("Constants cannot be modified.");
			}
			return false;
		}

		if (shader->varyings.has(var->name)) {
			return _validate_varying_assign(shader->varyings[var->name], r_message);
		}

		if (!(p_function_info.built_ins.has(var->name) &&
				p_function_info.built_ins[var->name].constant)) {
			return true;
		}
	}
	else if (p_node->type == Node::NODE_TYPE_ARRAY) {
		ArrayNode* arr = static_cast<ArrayNode*>(p_node);

		if (shader->constants.has(arr->name) || arr->is_const) {
			if (r_message) {
				*r_message = RTR("Constants cannot be modified.");
			}
			return false;
		}

		return true;
	}

	if (r_message) {
		*r_message = "Assignment to constant expression.";
	}
	return false;
}

ShaderLanguage::ShaderNode::Uniform::Hint ShaderLanguage::_sanitize_hint(
	ShaderNode::Uniform::Hint p_hint)
{
	if (p_hint == ShaderNode::Uniform::HINT_SCREEN_TEXTURE ||
		p_hint == ShaderNode::Uniform::HINT_NORMAL_ROUGHNESS_TEXTURE ||
		p_hint == ShaderNode::Uniform::HINT_DEPTH_TEXTURE) {
		return p_hint;
	}
	return ShaderNode::Uniform::HINT_NONE;
}

bool ShaderLanguage::_propagate_function_call_sampler_uniform_settings(const String& p_name,
	int p_argument, TextureFilter p_filter, TextureRepeat p_repeat,
	ShaderNode::Uniform::Hint p_hint)
{
	for (int i = 0; i < shader->vfunctions.size(); i++) {
		if (shader->vfunctions[i].name == p_name) {
			ERR_FAIL_INDEX_V(p_argument, shader->vfunctions[i].function->arguments.size(), false);
			FunctionNode::Argument* arg =
				&shader->vfunctions[i].function->arguments.write[p_argument];
			if (arg->tex_builtin_check) {
				_set_error(vformat(RTR("Sampler argument %d of function '%s' called more than once "
									   "using both built-ins and uniform textures, this is not "
									   "supported (use either one or the other)."),
					p_argument, String(p_name)));
				return false;
			}
			else if (arg->tex_argument_check) {
				// Was checked, verify that filter, repeat, and hint are the same.
				if (arg->tex_argument_filter == p_filter && arg->tex_argument_repeat == p_repeat &&
					arg->tex_hint == _sanitize_hint(p_hint)) {
					return true;
				}
				else {
					_set_error(vformat(RTR("Sampler argument %d of function '%s' called more than "
										   "once using textures that differ in either filter, "
										   "repeat, or texture hint setting."),
						p_argument, String(p_name)));
					return false;
				}
			}
			else {
				arg->tex_argument_check = true;
				arg->tex_argument_filter = p_filter;
				arg->tex_argument_repeat = p_repeat;
				arg->tex_hint = _sanitize_hint(p_hint);
				for (KeyValue<String, HashSet<int>>& E : arg->tex_argument_connect) {
					for (const int& F : E.value) {
						if (!_propagate_function_call_sampler_uniform_settings(
								E.key, F, p_filter, p_repeat, p_hint)) {
							return false;
						}
					}
				}
				return true;
			}
		}
	}
	ERR_FAIL_V(false); // bug? function not found
}

bool ShaderLanguage::_propagate_function_call_sampler_builtin_reference(
	const String& p_name, int p_argument, const String& p_builtin)
{
	for (int i = 0; i < shader->vfunctions.size(); i++) {
		if (shader->vfunctions[i].name == p_name) {
			ERR_FAIL_INDEX_V(p_argument, shader->vfunctions[i].function->arguments.size(), false);
			FunctionNode::Argument* arg =
				&shader->vfunctions[i].function->arguments.write[p_argument];
			if (arg->tex_argument_check) {
				_set_error(vformat(RTR("Sampler argument %d of function '%s' called more than once "
									   "using both built-ins and uniform textures, this is not "
									   "supported (use either one or the other)."),
					p_argument, String(p_name)));
				return false;
			}
			else if (arg->tex_builtin_check) {
				// was checked, verify that the built-in is the same
				if (arg->tex_builtin == p_builtin) {
					return true;
				}
				else {
					_set_error(vformat(RTR("Sampler argument %d of function '%s' called more than "
										   "once using different built-ins. Only calling with the "
										   "same built-in is supported."),
						p_argument, String(p_name)));
					return false;
				}
			}
			else {
				arg->tex_builtin_check = true;
				arg->tex_builtin = p_builtin;

				for (KeyValue<String, HashSet<int>>& E : arg->tex_argument_connect) {
					for (const int& F : E.value) {
						if (!_propagate_function_call_sampler_builtin_reference(
								E.key, F, p_builtin)) {
							return false;
						}
					}
				}
				return true;
			}
		}
	}
	ERR_FAIL_V(false); // bug? function not found
}

Error ShaderLanguage::_parse_array_size(BlockNode* p_block, const FunctionInfo& p_function_info,
	bool p_forbid_unknown_size, Node** r_size_expression, int* r_array_size, bool* r_unknown_size)
{
	bool error = false;
	if (r_array_size != nullptr && *r_array_size > 0) {
		error = true;
	}
	if (r_unknown_size != nullptr && *r_unknown_size) {
		error = true;
	}
	if (error) {
		_set_error(vformat(RTR("Array size is already defined.")));
		return ERR_PARSE_ERROR;
	}

	TkPos pos = _get_tkpos();
	Token tk = _get_token();

	if (tk.type == TK_BRACKET_CLOSE) {
		if (p_forbid_unknown_size) {
			_set_error(vformat(RTR("Unknown array size is forbidden in that context.")));
			return ERR_PARSE_ERROR;
		}
		if (r_unknown_size != nullptr) {
			*r_unknown_size = true;
		}
	}
	else {
		_set_tkpos(pos);

		int array_size = 0;
		Node* expr = _parse_and_reduce_expression(p_block, p_function_info);

		if (expr) {
			Vector<Scalar> values = _get_node_values(p_block, p_function_info, expr);

			if (!values.is_empty()) {
				switch (expr->get_datatype()) {
				case TYPE_INT: {
					array_size = values[0].sint;
				} break;
				case TYPE_UINT: {
					array_size = (int)values[0].uint;
				} break;
				default: {
				} break;
				}
			}

			if (r_size_expression != nullptr) {
				*r_size_expression = expr;
			}
		}

		if (array_size <= 0) {
			_set_error(RTR("Expected a positive integer constant."));
			return ERR_PARSE_ERROR;
		}

		tk = _get_token();
		if (tk.type != TK_BRACKET_CLOSE) {
			_set_expected_error("]");
			return ERR_PARSE_ERROR;
		}

		if (r_array_size != nullptr) {
			*r_array_size = array_size;
		}
	}
	return OK;
}

ShaderLanguage::Node* ShaderLanguage::_parse_array_constructor(
	BlockNode* p_block, const FunctionInfo& p_function_info)
{
	DataType type = TYPE_VOID;
	String struct_name = "";
	int array_size = 0;
	bool auto_size = false;
	bool undefined_size = false;
	Token tk = _get_token();

	if (tk.type == TK_CURLY_BRACKET_OPEN) {
		auto_size = true;
	}
	else {
		if (shader->structs.has(tk.text)) {
			type = TYPE_STRUCT;
			struct_name = tk.text;
		}
		else {
			if (!is_token_variable_datatype(tk.type)) {
				_set_error(RTR("Invalid data type for the array."));
				return nullptr;
			}
			type = get_token_datatype(tk.type);
		}
		tk = _get_token();
		if (tk.type == TK_BRACKET_OPEN) {
			Error error = _parse_array_size(
				p_block, p_function_info, false, nullptr, &array_size, &undefined_size);
			if (error != OK) {
				return nullptr;
			}
			tk = _get_token();
		}
		else {
			_set_expected_error("[");
			return nullptr;
		}
	}

	ArrayConstructNode* an = alloc_node<ArrayConstructNode>();

	if (tk.type == TK_PARENTHESIS_OPEN || auto_size) { // initialization
		int idx = 0;
		while (true) {
			Node* n = _parse_and_reduce_expression(p_block, p_function_info);
			if (!n) {
				return nullptr;
			}

			// define type by using the first member
			if (auto_size && idx == 0) {
				type = n->get_datatype();
				if (type == TYPE_STRUCT) {
					struct_name = n->get_datatype_name();
				}
			}
			else {
				if (!_compare_datatypes(
						type, struct_name, 0, n->get_datatype(), n->get_datatype_name(), 0)) {
					return nullptr;
				}
			}

			tk = _get_token();
			if (tk.type == TK_COMMA) {
				an->initializer.push_back(n);
			}
			else if (!auto_size && tk.type == TK_PARENTHESIS_CLOSE) {
				an->initializer.push_back(n);
				break;
			}
			else if (auto_size && tk.type == TK_CURLY_BRACKET_CLOSE) {
				an->initializer.push_back(n);
				break;
			}
			else {
				if (auto_size) {
					_set_expected_error("}", ",");
				}
				else {
					_set_expected_error(")", ",");
				}
				return nullptr;
			}
			idx++;
		}
		if (!auto_size && !undefined_size && an->initializer.size() != array_size) {
			_set_error(vformat(RTR("Array size mismatch. Expected %d elements (found %d)."),
				array_size, an->initializer.size()));
			return nullptr;
		}
	}
	else {
		_set_error(RTR("Expected array initialization."));
		return nullptr;
	}

	an->datatype = type;
	an->struct_name = struct_name;
	return an;
}

ShaderLanguage::Node* ShaderLanguage::_parse_array_constructor(BlockNode* p_block,
	const FunctionInfo& p_function_info, DataType p_type, const String& p_struct_name,
	int p_array_size)
{
	DataType type = TYPE_VOID;
	String struct_name = "";
	int array_size = 0;
	bool auto_size = false;
	TkPos prev_pos = _get_tkpos();
	Token tk = _get_token();

	if (tk.type == TK_CURLY_BRACKET_OPEN) {
		auto_size = true;
	}
	else {
		if (shader->structs.has(tk.text)) {
			type = TYPE_STRUCT;
			struct_name = tk.text;
		}
		else {
			if (!is_token_variable_datatype(tk.type)) {
				_set_tkpos(prev_pos);

				Node* n = _parse_and_reduce_expression(p_block, p_function_info);

				if (!n) {
					_set_error(RTR("Invalid data type for the array."));
					return nullptr;
				}

				if (!_compare_datatypes(p_type, p_struct_name, p_array_size, n->get_datatype(),
						n->get_datatype_name(), n->get_array_size())) {
					return nullptr;
				}
				return n;
			}
			type = get_token_datatype(tk.type);
		}
		tk = _get_token();
		if (tk.type == TK_BRACKET_OPEN) {
			bool is_unknown_size = false;
			Error error = _parse_array_size(
				p_block, p_function_info, false, nullptr, &array_size, &is_unknown_size);
			if (error != OK) {
				return nullptr;
			}
			if (is_unknown_size) {
				array_size = p_array_size;
			}
			tk = _get_token();
		}
		else {
			_set_expected_error("[");
			return nullptr;
		}

		if (type != p_type || struct_name != p_struct_name || array_size != p_array_size) {
			String from;
			if (type == TYPE_STRUCT) {
				from += struct_name;
			}
			else {
				from += get_datatype_name(type);
			}
			from += "[";
			from += itos(array_size);
			from += "]'";

			String to;
			if (type == TYPE_STRUCT) {
				to += p_struct_name;
			}
			else {
				to += get_datatype_name(p_type);
			}
			to += "[";
			to += itos(p_array_size);
			to += "]'";

			_set_error(vformat(RTR("Cannot convert from '%s' to '%s'."), from, to));
			return nullptr;
		}
	}

	ArrayConstructNode* an = alloc_node<ArrayConstructNode>();
	an->datatype = p_type;
	an->struct_name = p_struct_name;

	if (tk.type == TK_PARENTHESIS_OPEN || auto_size) { // initialization
		while (true) {
			Node* n = _parse_and_reduce_expression(p_block, p_function_info);
			if (!n) {
				return nullptr;
			}

			if (!_compare_datatypes(p_type, p_struct_name, 0, n->get_datatype(),
					n->get_datatype_name(), n->get_array_size())) {
				return nullptr;
			}

			tk = _get_token();
			if (tk.type == TK_COMMA) {
				an->initializer.push_back(n);
			}
			else if (!auto_size && tk.type == TK_PARENTHESIS_CLOSE) {
				an->initializer.push_back(n);
				break;
			}
			else if (auto_size && tk.type == TK_CURLY_BRACKET_CLOSE) {
				an->initializer.push_back(n);
				break;
			}
			else {
				if (auto_size) {
					_set_expected_error("}", ",");
				}
				else {
					_set_expected_error(")", ",");
				}
				return nullptr;
			}
		}
		if (an->initializer.size() != p_array_size) {
			_set_error(vformat(RTR("Array size mismatch. Expected %d elements (found %d)."),
				p_array_size, an->initializer.size()));
			return nullptr;
		}
	}
	else {
		_set_error(RTR("Expected array initialization."));
		return nullptr;
	}

	return an;
}

ShaderLanguage::Node* ShaderLanguage::_parse_expression(BlockNode* p_block,
	const FunctionInfo& p_function_info, const ExpressionInfo* p_previous_expression_info)
{
	Vector<Expression> expression;

	// Vector<TokenType> operators;
#ifdef DEBUG_ENABLED
	bool check_position_write =
		check_warnings && HAS_WARNING(ShaderWarning::MAGIC_POSITION_WRITE_FLAG);
	check_position_write = check_position_write && String(shader_type_identifier) == "spatial" &&
						   current_function == "vertex";
#endif

	while (true) {
		Node* expr = nullptr;
		TkPos prepos = _get_tkpos();
		Token tk = _get_token();
		TkPos pos = _get_tkpos();

		bool is_const = false;

		if (tk.type == TK_PARENTHESIS_OPEN) {
			// handle subexpression

			expr = _parse_and_reduce_expression(p_block, p_function_info);
			if (!expr) {
				return nullptr;
			}

			tk = _get_token();

			if (tk.type != TK_PARENTHESIS_CLOSE) {
				_set_error(RTR("Expected ')' in expression."));
				return nullptr;
			}

		}
		else if (tk.type == TK_FLOAT_CONSTANT) {
			ConstantNode* constant = alloc_node<ConstantNode>();
			Scalar v;
			v.real = tk.constant;
			constant->values.push_back(v);
			constant->datatype = TYPE_FLOAT;
			expr = constant;

		}
		else if (tk.type == TK_INT_CONSTANT) {
			ConstantNode* constant = alloc_node<ConstantNode>();
			Scalar v;
			v.sint = tk.constant;
			constant->values.push_back(v);
			constant->datatype = TYPE_INT;
			expr = constant;

		}
		else if (tk.type == TK_UINT_CONSTANT) {
			ConstantNode* constant = alloc_node<ConstantNode>();
			Scalar v;
			v.uint = tk.constant;
			constant->values.push_back(v);
			constant->datatype = TYPE_UINT;
			expr = constant;

		}
		else if (tk.type == TK_TRUE) {
			// handle true constant
			ConstantNode* constant = alloc_node<ConstantNode>();
			Scalar v;
			v.boolean = true;
			constant->values.push_back(v);
			constant->datatype = TYPE_BOOL;
			expr = constant;

		}
		else if (tk.type == TK_FALSE) {
			// handle false constant
			ConstantNode* constant = alloc_node<ConstantNode>();
			Scalar v;
			v.boolean = false;
			constant->values.push_back(v);
			constant->datatype = TYPE_BOOL;
			expr = constant;

		}
		else if (tk.type == TK_TYPE_VOID) {
			// make sure void is not used in expression
			_set_error(RTR("Void value not allowed in expression."));
			return nullptr;
		}
		else if (is_token_nonvoid_datatype(tk.type) || tk.type == TK_CURLY_BRACKET_OPEN) {
			if (tk.type == TK_CURLY_BRACKET_OPEN) {
				// array constructor

				_set_tkpos(prepos);
				expr = _parse_array_constructor(p_block, p_function_info);
			}
			else {
				DataType datatype;
				DataPrecision precision = PRECISION_DEFAULT;

				if (is_token_precision(tk.type)) {
					precision = get_token_precision(tk.type);
					tk = _get_token();
				}

				datatype = get_token_datatype(tk.type);
				if (precision != PRECISION_DEFAULT &&
					_validate_precision(datatype, precision) != OK) {
					return nullptr;
				}

				tk = _get_token();

				if (tk.type == TK_BRACKET_OPEN) {
					// array constructor

					_set_tkpos(prepos);
					expr = _parse_array_constructor(p_block, p_function_info);
				}
				else {
					if (tk.type != TK_PARENTHESIS_OPEN) {
						_set_error(RTR("Expected '(' after the type name."));
						return nullptr;
					}
					// basic type constructor

					OperatorNode* func = alloc_node<OperatorNode>();
					func->op = OP_CONSTRUCT;

					if (precision != PRECISION_DEFAULT) {
						func->return_precision_cache = precision;
					}

					VariableNode* funcname = alloc_node<VariableNode>();
					funcname->name = get_datatype_name(datatype);
					func->arguments.push_back(funcname);

					int carg = -1;

					bool ok = _parse_function_arguments(p_block, p_function_info, func, &carg);

					if (carg >= 0) {
						completion_type = COMPLETION_CALL_ARGUMENTS;
						completion_line = tk_line;
						completion_block = p_block;
						completion_function = funcname->name;
						completion_argument = carg;
					}

					if (!ok) {
						return nullptr;
					}

					if (!_validate_function_call(p_block, p_function_info, func,
							&func->return_cache, &func->struct_name)) {
						_set_error(vformat(RTR("No matching constructor found for: '%s'."),
							String(funcname->name)));
						return nullptr;
					}

					expr = _reduce_expression(p_block, func);
				}
			}
		}
		else if (tk.type == TK_IDENTIFIER) {
			_set_tkpos(prepos);

			String identifier;

			StructNode* pstruct = nullptr;
			bool struct_init = false;

			_get_completable_identifier(p_block, COMPLETION_IDENTIFIER, identifier);

			if (shader->structs.has(identifier)) {
				pstruct = shader->structs[identifier].shader_struct;
				struct_init = true;
			}

			tk = _get_token();
			if (tk.type == TK_PARENTHESIS_OPEN) {
				if (struct_init) { // a struct constructor

					const String& name = identifier;

					OperatorNode* func = alloc_node<OperatorNode>();
					func->op = OP_STRUCT;
					func->struct_name = name;
					func->return_cache = TYPE_STRUCT;
					VariableNode* funcname = alloc_node<VariableNode>();
					funcname->name = name;
					func->arguments.push_back(funcname);

					for (List<ShaderLanguage::MemberNode*>::Element* E = pstruct->members.front();
						 E; E = E->next()) {
						Node* nexpr;

						if (E->get()->array_size != 0) {
							nexpr = _parse_array_constructor(p_block, p_function_info,
								E->get()->get_datatype(), E->get()->struct_name,
								E->get()->array_size);
							if (!nexpr) {
								return nullptr;
							}
						}
						else {
							nexpr = _parse_and_reduce_expression(p_block, p_function_info);
							if (!nexpr) {
								return nullptr;
							}
							if (!_compare_datatypes_in_nodes(E->get(), nexpr)) {
								return nullptr;
							}
						}

						if (E->next()) {
							tk = _get_token();
							if (tk.type != TK_COMMA) {
								_set_expected_error(",");
								return nullptr;
							}
						}
						func->arguments.push_back(nexpr);
					}
					tk = _get_token();
					if (tk.type != TK_PARENTHESIS_CLOSE) {
						_set_expected_error(")");
						return nullptr;
					}

					expr = func;

				}
				else { // a function call

					// Non-builtin function call is forbidden for constant declaration.
					if (is_const_decl) {
						if (shader->functions.has(identifier)) {
							_set_error(RTR("Expected constant expression."));
							return nullptr;
						}
					}

					const String& rname = identifier;
					String name = identifier;

					OperatorNode* func = alloc_node<OperatorNode>();
					func->op = OP_CALL;

					VariableNode* funcname = alloc_node<VariableNode>();
					funcname->name = name;
					funcname->rname = name;

					func->arguments.push_back(funcname);

					int carg = -1;

					bool ok = _parse_function_arguments(p_block, p_function_info, func, &carg);

					// Check if block has a variable with the same name as function to prevent
					// shader crash.
					ShaderLanguage::BlockNode* bnode = p_block;
					while (bnode) {
						if (bnode->variables.has(name)) {
							_set_error(RTR("Expected a function name."));
							return nullptr;
						}
						bnode = bnode->parent_block;
					}

					int64_t arg_count = func->arguments.size();
					int64_t arg_count2 = func->arguments.size() - 1;

					// Test if function was parsed first.
					int function_index = -1;
					for (int i = 0, max_valid_args = 0; i < shader->vfunctions.size(); i++) {
						if (!shader->vfunctions[i].callable ||
							shader->vfunctions[i].rname != rname ||
							arg_count2 != shader->vfunctions[i].function->arguments.size()) {
							continue;
						}

						bool found = true;
						int valid_args = 0;

						// Search for correct overload.
						for (int j = 1; j < arg_count; j++) {
							const FunctionNode::Argument& a =
								shader->vfunctions[i].function->arguments[j - 1];
							Node* b = func->arguments[j];

							if (a.type == b->get_datatype() &&
								a.array_size == b->get_array_size()) {
								if (a.type == TYPE_STRUCT) {
									if (a.struct_name != b->get_datatype_name()) {
										found = false;

									break;
									}
									else {
										valid_args++;
									}
								}
								else {
									valid_args++;
								}
							}
							else {
								if (function_overload_count[rname] == 0 &&
									get_scalar_type(a.type) == a.type &&
									b->type == Node::NODE_TYPE_CONSTANT && a.array_size == 0 &&
									convert_constant(static_cast<ConstantNode*>(b), a.type)) {
									// Implicit cast if no overloads.
									continue;
								}
								found = false;
								break;
							}
						}

						// Using the best match index for completion hint if the function not found.
						if (valid_args > max_valid_args) {
							name = shader->vfunctions[i].name;
							funcname->name = name;
							max_valid_args = valid_args;
						}

						if (!found) {
							continue;
						}

						// Add to current function as dependency.
						for (int j = 0; j < shader->vfunctions.size(); j++) {
							if (shader->vfunctions[j].name == current_function) {
								shader->vfunctions.write[j].uses_function.insert(name);
								break;
							}
						}

						name = shader->vfunctions[i].name;
						funcname->name = name;

						// See if texture arguments must connect.
						function_index = i;
						break;
					}

					if (carg >= 0) {
						completion_type = COMPLETION_CALL_ARGUMENTS;
						completion_line = tk_line;
						completion_block = p_block;
						completion_function = funcname->name;
						completion_argument = carg;
					}

					if (!ok) {
						return nullptr;
					}

					if (name != current_function) { // Recursion is not allowed.
						// Register call.
						if (calls_info.has(name)) {
							calls_info[current_function].calls.push_back(&calls_info[name]);
						}

						bool is_builtin = false;

						if (is_supported_frag_only_funcs && stages) {
							for (const KeyValue<String, FunctionInfo>& E : *stages) {
								if (E.value.stage_functions.has(name)) {
									// Register usage of the restricted stage function.
									calls_info[current_function].uses_restricted_items.push_back(
										Pair<String, CallInfo::Item>(
											name, CallInfo::Item(CallInfo::Item::ITEM_TYPE_BUILTIN,
													  _get_tkpos())));
									is_builtin = true;
									break;
								}
							}
						}

						if (!is_builtin) {
							int idx = 0;
							while (frag_only_func_defs[idx].name) {
								if (frag_only_func_defs[idx].name == name) {
									// If a built-in function not found for the current shader type,
									// then it shouldn't be parsed further.
									if (!is_supported_frag_only_funcs) {
										_set_error(
											vformat(RTR("Built-in function '%s' is not supported "
														"for the '%s' shader type."),
												name, shader_type_identifier));
										return nullptr;
									}
									// Register usage of the restricted function.
									calls_info[current_function].uses_restricted_items.push_back(
										Pair<String, CallInfo::Item>(
											name, CallInfo::Item(CallInfo::Item::ITEM_TYPE_BUILTIN,
													  _get_tkpos())));
									is_builtin = true;
									break;
								}
								idx++;
							}
						}

						// Recursively checks for the restricted function call.
						if (is_supported_frag_only_funcs && current_function == "vertex" &&
							stages->has(current_function) &&
							!_validate_restricted_func(
								name, &calls_info[current_function], is_builtin)) {
							return nullptr;
						}
					}

					bool is_custom_func = false;
					if (!_validate_function_call(p_block, p_function_info, func,
							&func->return_cache, &func->struct_name, &is_custom_func)) {
						_set_error(vformat(
							RTR("No matching function found for: '%s'."), String(funcname->rname)));
						return nullptr;
					}
					completion_class = TAG_GLOBAL; // reset sub-class
					if (function_index >= 0) {
						// connect texture arguments, so we can cache in the
						// argument what type of filter and repeat to use

						FunctionNode* call_function = shader->vfunctions[function_index].function;
						if (call_function) {
							func->return_cache = call_function->get_datatype();
							func->struct_name = call_function->get_datatype_name();
							func->return_array_size = call_function->get_array_size();

							// get current base function
							FunctionNode* base_function = nullptr;
							{
								BlockNode* b = p_block;

								while (b) {
									if (b->parent_function) {
										base_function = b->parent_function;
										break;
									}
									else {
										b = b->parent_block;
									}
								}
							}

							ERR_FAIL_NULL_V(base_function, nullptr); // Bug, wtf.

							for (int i = 0; i < call_function->arguments.size(); i++) {
								int argidx = i + 1;
								if (argidx < arg_count) {
									bool error = false;
									Node* n = func->arguments[argidx];
									ArgumentQualifier arg_qual =
										call_function->arguments[i].qualifier;
									bool is_out_arg =
										arg_qual != ArgumentQualifier::ARGUMENT_QUALIFIER_IN;

									if (n->type == Node::NODE_TYPE_VARIABLE ||
										n->type == Node::NODE_TYPE_ARRAY) {
										String varname;

										if (n->type == Node::NODE_TYPE_VARIABLE) {
											VariableNode* vn = static_cast<VariableNode*>(n);
											varname = vn->name;
										}
										else { // TYPE_ARRAY
											ArrayNode* an = static_cast<ArrayNode*>(n);
											varname = an->name;
										}

										if (shader->varyings.has(varname)) {
											switch (shader->varyings[varname].stage) {
											case ShaderNode::Varying::STAGE_UNKNOWN:
												if (is_out_arg) {
													error = true;
												}
												break;
											case ShaderNode::Varying::STAGE_VERTEX:
												if (is_out_arg &&
													current_function != varying_function_names
																			.vertex) { // inout/out
													error = true;
												}
												break;
											case ShaderNode::Varying::STAGE_FRAGMENT:
												if (!is_out_arg) {
													if (current_function !=
															varying_function_names.fragment &&
														current_function !=
															varying_function_names.light) {
														error = true;
													}
												}
												else if (current_function !=
														   varying_function_names
															   .fragment) { // inout/out
													error = true;
												}
												break;
											default:
												break;
											}

											if (error) {
												_set_error(vformat(
													RTR("Varying '%s' cannot be passed for the "
														"'%s' parameter in that context."),
													varname, _get_qualifier_str(arg_qual)));
												return nullptr;
											}
										}
									}

									bool is_const_arg = call_function->arguments[i].is_const;

									if (is_const_arg || is_out_arg) {
										String varname;

										if (n->type == Node::NODE_TYPE_CONSTANT ||
											n->type == Node::NODE_TYPE_OPERATOR ||
											n->type == Node::NODE_TYPE_ARRAY_CONSTRUCT) {
											if (!is_const_arg) {
												error = true;
											}
										}
										else if (n->type == Node::NODE_TYPE_ARRAY) {
											ArrayNode* an = static_cast<ArrayNode*>(n);
											if (!is_const_arg &&
												(an->call_expression != nullptr || an->is_const)) {
												error = true;
											}
											varname = an->name;
										}
										else if (n->type == Node::NODE_TYPE_VARIABLE) {
											VariableNode* vn = static_cast<VariableNode*>(n);
											if (vn->is_const && !is_const_arg) {
												error = true;
											}
											varname = vn->name;
										}
										else if (n->type == Node::NODE_TYPE_MEMBER) {
											MemberNode* mn = static_cast<MemberNode*>(n);
											if (mn->basetype_const && is_out_arg) {
												error = true;
											}
										}
										if (!error && varname != String()) {
											if (shader->constants.has(varname)) {
												error = true;
											}
											else if (shader->uniforms.has(varname)) {
												error = true;
											}
											else if (p_function_info.built_ins.has(varname)) {
												BuiltInInfo info =
													p_function_info.built_ins[varname];
												if (info.constant) {
													error = true;
												}
											}
										}

										if (error) {
											_set_error(
												vformat(RTR("A constant value cannot be passed for "
															"the '%s' parameter."),
													_get_qualifier_str(arg_qual)));
											return nullptr;
										}
									}
									if (is_sampler_type(call_function->arguments[i].type)) {
										// Let's see where our argument comes from.
										String varname;
										if (n->type == Node::NODE_TYPE_VARIABLE) {
											VariableNode* vn = static_cast<VariableNode*>(n);
											varname = vn->name;
										}
										else if (n->type == Node::NODE_TYPE_ARRAY) {
											ArrayNode* an = static_cast<ArrayNode*>(n);
											varname = an->name;
										}

										if (shader->uniforms.has(varname)) {
											// being sampler, this either comes from a uniform
											ShaderNode::Uniform* u = &shader->uniforms[varname];
											ERR_CONTINUE(
												u->type != call_function->arguments[i]
															   .type); // this should have been
																	   // validated previously

											if (RendererCompositor::is_xr_enabled() &&
												is_custom_func) {
												ShaderNode::Uniform::Hint hint = u->hint;

												if (hint ==
														ShaderNode::Uniform::HINT_DEPTH_TEXTURE ||
													hint ==
														ShaderNode::Uniform::HINT_SCREEN_TEXTURE ||
													hint == ShaderNode::Uniform::
																HINT_NORMAL_ROUGHNESS_TEXTURE) {
													_set_error(vformat(
														RTR("Unable to pass a multiview texture "
															"sampler as a parameter to a custom "
															"function. Consider sampling it in the "
															"main function and then passing the "
															"vector result to the custom "
															"function."),
														get_uniform_hint_name(hint)));
													return nullptr;
												}
											}

											// propagate
											if (!_propagate_function_call_sampler_uniform_settings(
													name, i, u->filter, u->repeat, u->hint)) {
												return nullptr;
											}
										}
										else if (p_function_info.built_ins.has(varname)) {
											if (is_custom_func && varname == SNAME("RADIANCE")) {
												_set_error(
													RTR("Unable to pass RADIANCE texture sampler "
														"as a parameter to a custom function. "
														"Consider sampling it in the main function "
														"and then passing the vector result to the "
														"custom function."));
												return nullptr;
											}

											// a built-in
											if (!_propagate_function_call_sampler_builtin_reference(
													name, i, varname)) {
												return nullptr;
											}
										}
										else {
											// or this comes from an argument, but nothing else can
											// be a sampler
											bool found = false;
											for (int j = 0; j < base_function->arguments.size();
												 j++) {
												if (base_function->arguments[j].name == varname) {
													if (!base_function->arguments[j]
															 .tex_argument_connect.has(
																 call_function->name)) {
														base_function->arguments.write[j]
															.tex_argument_connect[call_function
																					  ->name] =
															HashSet<int>();
													}
													base_function->arguments.write[j]
														.tex_argument_connect[call_function->name]
														.insert(i);
													found = true;
													break;
												}
											}
											ERR_CONTINUE(!found);
										}
									}
								}
								else {
									break;
								}
							}
						}
					}
					expr = func;
#ifdef DEBUG_ENABLED
					if (check_warnings && is_custom_func) {
						String func_name;

						if (p_block && p_block->parent_function) {
							func_name = p_block->parent_function->name;
						}

						_parse_used_identifier(
							name, IdentifierType::IDENTIFIER_FUNCTION, func_name);
					}
#endif // DEBUG_ENABLED
				}
			}
			else {
				// an identifier

				last_name = identifier;
				last_type = IDENTIFIER_MAX;
				_set_tkpos(pos);

				DataType data_type = TYPE_MAX;
				IdentifierType ident_type = IDENTIFIER_MAX;
				int array_size = 0;
				String struct_name;
				bool is_local = false;

				if (p_block && p_block->block_tag != SubClassTag::TAG_GLOBAL) {
					int idx = 0;
					bool found = false;

					while (builtin_func_defs[idx].name) {
						if (builtin_func_defs[idx].tag == p_block->block_tag &&
							builtin_func_defs[idx].name == identifier) {
							found = true;
							break;
						}
						idx++;
					}
					if (!found) {
						_set_error(vformat(
							RTR("Unknown identifier in expression: '%s'."), String(identifier)));
						return nullptr;
					}
				}
				else {
					if (!_find_identifier(p_block, false, p_function_info, identifier, &data_type,
							&ident_type, &is_const, &array_size, &struct_name)) {
						if (identifier == "SCREEN_TEXTURE" || identifier == "DEPTH_TEXTURE" ||
							identifier == "NORMAL_ROUGHNESS_TEXTURE") {
							String name = String(identifier);
							String name_lower = name.to_lower();
							_set_error(
								vformat(RTR("%s has been removed in favor of using hint_%s with a "
											"uniform.\nTo continue with minimal code changes add "
											"'uniform sampler2D %s : hint_%s, "
											"filter_linear_mipmap;' near the top of your shader."),
									name, name_lower, name, name_lower));
							return nullptr;
						}
						_set_error(vformat(
							RTR("Unknown identifier in expression: '%s'."), String(identifier)));
						return nullptr;
					}
					if (is_const_decl && !is_const) {
						_set_error(RTR("Expected constant expression."));
						return nullptr;
					}
					if (ident_type == IDENTIFIER_FUNCTION) {
						_set_error(vformat(
							RTR("Can't use function as identifier: '%s'."), String(identifier)));
						return nullptr;
					}
#ifdef DEBUG_ENABLED
					if (check_warnings) {
						String func_name;
						BlockNode* b = p_block;

						while (b) {
							if (b->parent_function) {
								func_name = b->parent_function->name;
								break;
							}
							else {
								b = b->parent_block;
							}
						}

						_parse_used_identifier(identifier, ident_type, func_name);
					}
#endif // DEBUG_ENABLED
					if (ident_type == IDENTIFIER_VARYING) {
						TkPos prev_pos = _get_tkpos();
						Token next_token = _get_token();

						// An array of varyings.
						if (next_token.type == TK_BRACKET_OPEN) {
							_get_token(); // Pass constant.
							_get_token(); // Pass TK_BRACKET_CLOSE.
							next_token = _get_token();
						}
						_set_tkpos(prev_pos);

						ShaderNode::Varying& var = shader->varyings[identifier];
						calls_info[current_function].uses_restricted_items.push_back(
							Pair<String, CallInfo::Item>(identifier,
								CallInfo::Item(CallInfo::Item::ITEM_TYPE_VARYING, prev_pos)));

						String error;
						if (is_token_operator_assign(next_token.type)) {
							if (!_validate_varying_assign(shader->varyings[identifier], &error)) {
								_set_error(error);
								return nullptr;
							}
						}
						else {
							if (var.stage == ShaderNode::Varying::STAGE_UNKNOWN &&
								var.type < TYPE_INT) {
								if (current_function == varying_function_names.vertex) {
									_set_error(vformat(RTR("Varying with '%s' data type may only "
														   "be used in the '%s' function."),
										get_datatype_name(var.type), "fragment"));
								}
								else {
									_set_error(vformat(RTR("Varying '%s' must be assigned in the "
														   "'%s' function first."),
										identifier, "fragment"));
								}
								return nullptr;
							}
						}
					}
#ifdef DEBUG_ENABLED
					if (check_position_write && ident_type == IDENTIFIER_BUILTIN_VAR) {
						if (String(identifier) == "POSITION") {
							// Check if the user wrote "POSITION = vec4(VERTEX," and warn if they
							// did.
							TkPos prev_pos = _get_tkpos();
							// Reset the position so compiling can continue as normal.
							_set_tkpos(prev_pos);
						}
					}
#endif // DEBUG_ENABLED
					if (is_const) {
						last_type = IDENTIFIER_CONSTANT;
					}
					else {
						last_type = ident_type;
					}

					is_local = ident_type == IDENTIFIER_LOCAL_VAR ||
							   ident_type == IDENTIFIER_FUNCTION_ARGUMENT;
				}

				Node* index_expression = nullptr;
				Node* call_expression = nullptr;
				Node* assign_expression = nullptr;

				if (array_size > 0) {
					prepos = _get_tkpos();
					tk = _get_token();

					if (tk.type == TK_OP_ASSIGN) {
						if (is_const) {
							_set_error(RTR("Constants cannot be modified."));
							return nullptr;
						}
						assign_expression = _parse_array_constructor(
							p_block, p_function_info, data_type, struct_name, array_size);
						if (!assign_expression) {
							return nullptr;
						}
					}
					else if (tk.type == TK_PERIOD) {
						completion_class = TAG_ARRAY;
						if (p_block != nullptr) {
							p_block->block_tag = SubClassTag::TAG_ARRAY;
						}
						call_expression = _parse_and_reduce_expression(p_block, p_function_info);
						if (p_block != nullptr) {
							p_block->block_tag = SubClassTag::TAG_GLOBAL;
						}
						if (!call_expression) {
							return nullptr;
						}
					}
					else if (tk.type == TK_BRACKET_OPEN) { // indexing
						index_expression = _parse_and_reduce_expression(p_block, p_function_info);
						if (!index_expression) {
							return nullptr;
						}

						if (index_expression->get_array_size() != 0 ||
							(index_expression->get_datatype() != TYPE_INT &&
								index_expression->get_datatype() != TYPE_UINT)) {
							_set_error(RTR("Only integer expressions are allowed for indexing."));
							return nullptr;
						}

						if (index_expression->type == Node::NODE_TYPE_CONSTANT) {
							ConstantNode* cnode = static_cast<ConstantNode*>(index_expression);
							if (cnode) {
								if (!cnode->values.is_empty()) {
									int value = cnode->values[0].sint;
									if (value < 0 || value >= array_size) {
										_set_error(vformat(RTR("Index [%d] out of range [%d..%d]."),
											value, 0, array_size - 1));
										return nullptr;
									}
								}
							}
						}

						tk = _get_token();
						if (tk.type != TK_BRACKET_CLOSE) {
							_set_expected_error("]");
							return nullptr;
						}
					}
					else {
						_set_tkpos(prepos);
					}

					ArrayNode* arrname = alloc_node<ArrayNode>();
					arrname->name = identifier;
					arrname->datatype_cache = data_type;
					arrname->struct_name = struct_name;
					arrname->index_expression = index_expression;
					arrname->call_expression = call_expression;
					arrname->assign_expression = assign_expression;
					arrname->is_const = is_const;
					arrname->array_size = array_size;
					arrname->is_local = is_local;
					expr = arrname;
				}
				else {
					VariableNode* varname = alloc_node<VariableNode>();
					varname->name = identifier;
					varname->datatype_cache = data_type;
					varname->is_const = is_const;
					varname->struct_name = struct_name;
					varname->is_local = is_local;
					expr = varname;
				}
			}
		}
		else if (tk.type == TK_OP_ADD) {
			continue; // this one does nothing
		}
		else if (tk.type == TK_OP_SUB || tk.type == TK_OP_NOT || tk.type == TK_OP_BIT_INVERT ||
				   tk.type == TK_OP_INCREMENT || tk.type == TK_OP_DECREMENT) {
			Expression e;
			e.is_op = true;

			switch (tk.type) {
			case TK_OP_SUB:
				e.op = OP_NEGATE;
				break;
			case TK_OP_NOT:
				e.op = OP_NOT;
				break;
			case TK_OP_BIT_INVERT:
				e.op = OP_BIT_INVERT;
				break;
			case TK_OP_INCREMENT:
				e.op = OP_INCREMENT;
				break;
			case TK_OP_DECREMENT:
				e.op = OP_DECREMENT;
				break;
			default:
				ERR_FAIL_V(nullptr);
			}

			expression.push_back(e);
			continue;
		}
		else {
			bool valid = false;
			if (p_block && p_block->block_type == BlockNode::BLOCK_TYPE_FOR_EXPRESSION &&
				tk.type == TK_PARENTHESIS_CLOSE) {
				valid = true;
				_set_tkpos(prepos);

				OperatorNode* func = alloc_node<OperatorNode>();
				func->op = OP_EMPTY;
				expr = func;
			}
			if (!valid) {
				if (tk.type != TK_SEMICOLON) {
					_set_error(
						vformat(RTR("Expected expression, found: '%s'."), get_token_text(tk)));
					return nullptr;
				}
				else {
					_set_tkpos(prepos);

					OperatorNode* func = alloc_node<OperatorNode>();
					func->op = OP_EMPTY;
					expr = func;
				}
			}
		}

		ERR_FAIL_NULL_V(expr, nullptr);

		/* OK now see what's NEXT to the operator.. */

		while (true) {
			TkPos pos2 = _get_tkpos();
			tk = _get_token();

			if (tk.type == TK_CURSOR) {
				// do nothing
			}
			else if (tk.type == TK_PERIOD) {
#ifdef DEBUG_ENABLED
				uint32_t prev_keyword_completion_context = keyword_completion_context;
				keyword_completion_context = CF_UNSPECIFIED;
#endif

				DataType dt = expr->get_datatype();
				String st = expr->get_datatype_name();

				if (!expr->is_indexed() && expr->get_array_size() > 0) {
					completion_class = TAG_ARRAY;
					if (p_block != nullptr) {
						p_block->block_tag = SubClassTag::TAG_ARRAY;
					}
					Node* call_expression = _parse_and_reduce_expression(p_block, p_function_info);
					if (p_block != nullptr) {
						p_block->block_tag = SubClassTag::TAG_GLOBAL;
					}
					if (!call_expression) {
						return nullptr;
					}
					expr = call_expression;
					break;
				}

				String identifier;
				if (_get_completable_identifier(p_block,
						dt == TYPE_STRUCT ? COMPLETION_STRUCT : COMPLETION_INDEX, identifier)) {
					if (dt == TYPE_STRUCT) {
						completion_struct = st;
					}
					else {
						completion_base = dt;
					}
				}

				if (identifier == String()) {
					_set_error(RTR("Expected an identifier as a member."));
					return nullptr;
				}
				String ident = identifier;

				bool ok = true;
				bool repeated = false;
				DataType member_type = TYPE_VOID;
				String member_struct_name = "";
				int array_size = 0;

				RBSet<char> position_symbols;
				RBSet<char> color_symbols;
				RBSet<char> texture_symbols;

				bool mix_error = false;

				switch (dt) {
				case TYPE_STRUCT: {
					ok = false;
					String member_name = String(ident.ptr());
					if (shader->structs.has(st)) {
						StructNode* n = shader->structs[st].shader_struct;
						for (const MemberNode* E : n->members) {
							if (String(E->name) == member_name) {
								member_type = E->datatype;
								array_size = E->array_size;
								if (member_type == TYPE_STRUCT) {
									member_struct_name = E->struct_name;
								}
								ok = true;
								break;
							}
						}
					}

				} break;
				case TYPE_BVEC2:
				case TYPE_IVEC2:
				case TYPE_UVEC2:
				case TYPE_VEC2: {
					int l = ident.length();
					if (l == 1) {
						member_type = DataType(dt - 1);
					}
					else if (l == 2) {
						member_type = dt;
					}
					else if (l == 3) {
						member_type = DataType(dt + 1);
					}
					else if (l == 4) {
						member_type = DataType(dt + 2);
					}
					else {
						ok = false;
						break;
					}

					const char32_t* c = ident.ptr();
					for (int i = 0; i < l; i++) {
						switch (c[i]) {
						case 'r':
						case 'g':
							if (position_symbols.size() > 0 || texture_symbols.size() > 0) {
								mix_error = true;
								break;
							}
							if (!color_symbols.has(c[i])) {
								color_symbols.insert(c[i]);
							}
							else {
								repeated = true;
							}
							break;
						case 'x':
						case 'y':
							if (color_symbols.size() > 0 || texture_symbols.size() > 0) {
								mix_error = true;
								break;
							}
							if (!position_symbols.has(c[i])) {
								position_symbols.insert(c[i]);
							}
							else {
								repeated = true;
							}
							break;
						case 's':
						case 't':
							if (color_symbols.size() > 0 || position_symbols.size() > 0) {
								mix_error = true;
								break;
							}
							if (!texture_symbols.has(c[i])) {
								texture_symbols.insert(c[i]);
							}
							else {
								repeated = true;
							}
							break;
						default:
							ok = false;
							break;
						}
					}

				} break;
				case TYPE_BVEC3:
				case TYPE_IVEC3:
				case TYPE_UVEC3:
				case TYPE_VEC3: {
					int l = ident.length();
					if (l == 1) {
						member_type = DataType(dt - 2);
					}
					else if (l == 2) {
						member_type = DataType(dt - 1);
					}
					else if (l == 3) {
						member_type = dt;
					}
					else if (l == 4) {
						member_type = DataType(dt + 1);
					}
					else {
						ok = false;
						break;
					}

					const char32_t* c = ident.ptr();
					for (int i = 0; i < l; i++) {
						switch (c[i]) {
						case 'r':
						case 'g':
						case 'b':
							if (position_symbols.size() > 0 || texture_symbols.size() > 0) {
								mix_error = true;
								break;
							}
							if (!color_symbols.has(c[i])) {
								color_symbols.insert(c[i]);
							}
							else {
								repeated = true;
							}
							break;
						case 'x':
						case 'y':
						case 'z':
							if (color_symbols.size() > 0 || texture_symbols.size() > 0) {
								mix_error = true;
								break;
							}
							if (!position_symbols.has(c[i])) {
								position_symbols.insert(c[i]);
							}
							else {
								repeated = true;
							}
							break;
						case 's':
						case 't':
						case 'p':
							if (color_symbols.size() > 0 || position_symbols.size() > 0) {
								mix_error = true;
								break;
							}
							if (!texture_symbols.has(c[i])) {
								texture_symbols.insert(c[i]);
							}
							else {
								repeated = true;
							}
							break;
						default:
							ok = false;
							break;
						}
					}

				} break;
				case TYPE_BVEC4:
				case TYPE_IVEC4:
				case TYPE_UVEC4:
				case TYPE_VEC4: {
					int l = ident.length();
					if (l == 1) {
						member_type = DataType(dt - 3);
					}
					else if (l == 2) {
						member_type = DataType(dt - 2);
					}
					else if (l == 3) {
						member_type = DataType(dt - 1);
					}
					else if (l == 4) {
						member_type = dt;
					}
					else {
						ok = false;
						break;
					}

					const char32_t* c = ident.ptr();
					for (int i = 0; i < l; i++) {
						switch (c[i]) {
						case 'r':
						case 'g':
						case 'b':
						case 'a':
							if (position_symbols.size() > 0 || texture_symbols.size() > 0) {
								mix_error = true;
								break;
							}
							if (!color_symbols.has(c[i])) {
								color_symbols.insert(c[i]);
							}
							else {
								repeated = true;
							}
							break;
						case 'x':
						case 'y':
						case 'z':
						case 'w':
							if (color_symbols.size() > 0 || texture_symbols.size() > 0) {
								mix_error = true;
								break;
							}
							if (!position_symbols.has(c[i])) {
								position_symbols.insert(c[i]);
							}
							else {
								repeated = true;
							}
							break;
						case 's':
						case 't':
						case 'p':
						case 'q':
							if (color_symbols.size() > 0 || position_symbols.size() > 0) {
								mix_error = true;
								break;
							}
							if (!texture_symbols.has(c[i])) {
								texture_symbols.insert(c[i]);
							}
							else {
								repeated = true;
							}
							break;
						default:
							ok = false;
							break;
						}
					}

				} break;

				default: {
					ok = false;
				}
				}

				if (mix_error) {
					_set_error(vformat(
						RTR("Cannot combine symbols from different sets in expression '.%s'."),
						ident));
					return nullptr;
				}

				if (!ok) {
					_set_error(vformat(RTR("Invalid member for '%s' expression: '.%s'."),
						(dt == TYPE_STRUCT ? st : get_datatype_name(dt)), ident));
					return nullptr;
				}

				MemberNode* mn = alloc_node<MemberNode>();
				mn->basetype = dt;
				mn->basetype_const = is_const;
				mn->datatype = member_type;
				mn->base_struct_name = st;
				mn->struct_name = member_struct_name;
				mn->array_size = array_size;
				mn->name = ident;
				mn->owner = expr;
				mn->has_swizzling_duplicates = repeated;

				if (array_size > 0) {
					TkPos prev_pos = _get_tkpos();
					tk = _get_token();
					if (tk.type == TK_OP_ASSIGN) {
						if (last_type == IDENTIFIER_CONSTANT) {
							_set_error(RTR("Constants cannot be modified."));
							return nullptr;
						}
						Node* assign_expression = _parse_array_constructor(
							p_block, p_function_info, member_type, member_struct_name, array_size);
						if (!assign_expression) {
							return nullptr;
						}
						mn->assign_expression = assign_expression;
					}
					else if (tk.type == TK_PERIOD) {
						completion_class = TAG_ARRAY;
						if (p_block != nullptr) {
							p_block->block_tag = SubClassTag::TAG_ARRAY;
						}
						mn->call_expression =
							_parse_and_reduce_expression(p_block, p_function_info);
						if (p_block != nullptr) {
							p_block->block_tag = SubClassTag::TAG_GLOBAL;
						}
						if (!mn->call_expression) {
							return nullptr;
						}
					}
					else if (tk.type == TK_BRACKET_OPEN) {
						Node* index_expression =
							_parse_and_reduce_expression(p_block, p_function_info);
						if (!index_expression) {
							return nullptr;
						}

						if (index_expression->get_array_size() != 0 ||
							(index_expression->get_datatype() != TYPE_INT &&
								index_expression->get_datatype() != TYPE_UINT)) {
							_set_error(RTR("Only integer expressions are allowed for indexing."));
							return nullptr;
						}

						if (index_expression->type == Node::NODE_TYPE_CONSTANT) {
							ConstantNode* cnode = static_cast<ConstantNode*>(index_expression);
							if (cnode) {
								if (!cnode->values.is_empty()) {
									int value = cnode->values[0].sint;
									if (value < 0 || value >= array_size) {
										_set_error(vformat(RTR("Index [%d] out of range [%d..%d]."),
											value, 0, array_size - 1));
										return nullptr;
									}
								}
							}
						}

						tk = _get_token();
						if (tk.type != TK_BRACKET_CLOSE) {
							_set_expected_error("]");
							return nullptr;
						}
						mn->index_expression = index_expression;
					}
					else {
						_set_tkpos(prev_pos);
					}
				}
				expr = mn;

#ifdef DEBUG_ENABLED
				keyword_completion_context = prev_keyword_completion_context;
#endif

				// todo
				// member (period) has priority over any operator
				// creates a subindexing expression in place

				/*}
				else if (tk.type==TK_BRACKET_OPEN) {
				//todo
				//subindexing has priority over any operator
				//creates a subindexing expression in place

	*/
			}
			else if (tk.type == TK_BRACKET_OPEN) {
				Node* index = _parse_and_reduce_expression(p_block, p_function_info);
				if (!index) {
					return nullptr;
				}

				if (index->get_array_size() != 0 ||
					(index->get_datatype() != TYPE_INT && index->get_datatype() != TYPE_UINT)) {
					_set_error(RTR("Only integer expressions are allowed for indexing."));
					return nullptr;
				}

				DataType member_type = TYPE_VOID;
				String member_struct_name;

				if (expr->get_array_size() > 0) {
					if (index->type == Node::NODE_TYPE_CONSTANT) {
						uint32_t index_constant = static_cast<ConstantNode*>(index)->values[0].uint;
						if (index_constant >= (uint32_t)expr->get_array_size()) {
							_set_error(vformat(RTR("Index [%d] out of range [%d..%d]."),
								index_constant, 0, expr->get_array_size() - 1));
							return nullptr;
						}
					}
					member_type = expr->get_datatype();
					if (member_type == TYPE_STRUCT) {
						member_struct_name = expr->get_datatype_name();
					}
				}
				else {
					switch (expr->get_datatype()) {
					case TYPE_BVEC2:
					case TYPE_VEC2:
					case TYPE_IVEC2:
					case TYPE_UVEC2:
					case TYPE_MAT2:
						if (index->type == Node::NODE_TYPE_CONSTANT) {
							uint32_t index_constant =
								static_cast<ConstantNode*>(index)->values[0].uint;
							if (index_constant >= 2) {
								_set_error(vformat(RTR("Index [%d] out of range [%d..%d]."),
									index_constant, 0, 1));
								return nullptr;
							}
						}

						switch (expr->get_datatype()) {
						case TYPE_BVEC2:
							member_type = TYPE_BOOL;
							break;
						case TYPE_VEC2:
							member_type = TYPE_FLOAT;
							break;
						case TYPE_IVEC2:
							member_type = TYPE_INT;
							break;
						case TYPE_UVEC2:
							member_type = TYPE_UINT;
							break;
						case TYPE_MAT2:
							member_type = TYPE_VEC2;
							break;
						default:
							break;
						}

						break;
					case TYPE_BVEC3:
					case TYPE_VEC3:
					case TYPE_IVEC3:
					case TYPE_UVEC3:
					case TYPE_MAT3:
						if (index->type == Node::NODE_TYPE_CONSTANT) {
							uint32_t index_constant =
								static_cast<ConstantNode*>(index)->values[0].uint;
							if (index_constant >= 3) {
								_set_error(vformat(RTR("Index [%d] out of range [%d..%d]."),
									index_constant, 0, 2));
								return nullptr;
							}
						}

						switch (expr->get_datatype()) {
						case TYPE_BVEC3:
							member_type = TYPE_BOOL;
							break;
						case TYPE_VEC3:
							member_type = TYPE_FLOAT;
							break;
						case TYPE_IVEC3:
							member_type = TYPE_INT;
							break;
						case TYPE_UVEC3:
							member_type = TYPE_UINT;
							break;
						case TYPE_MAT3:
							member_type = TYPE_VEC3;
							break;
						default:
							break;
						}
						break;
					case TYPE_BVEC4:
					case TYPE_VEC4:
					case TYPE_IVEC4:
					case TYPE_UVEC4:
					case TYPE_MAT4:
						if (index->type == Node::NODE_TYPE_CONSTANT) {
							uint32_t index_constant =
								static_cast<ConstantNode*>(index)->values[0].uint;
							if (index_constant >= 4) {
								_set_error(vformat(RTR("Index [%d] out of range [%d..%d]."),
									index_constant, 0, 3));
								return nullptr;
							}
						}

						switch (expr->get_datatype()) {
						case TYPE_BVEC4:
							member_type = TYPE_BOOL;
							break;
						case TYPE_VEC4:
							member_type = TYPE_FLOAT;
							break;
						case TYPE_IVEC4:
							member_type = TYPE_INT;
							break;
						case TYPE_UVEC4:
							member_type = TYPE_UINT;
							break;
						case TYPE_MAT4:
							member_type = TYPE_VEC4;
							break;
						default:
							break;
						}
						break;
					default: {
						_set_error(vformat(RTR("An object of type '%s' can't be indexed."),
							(expr->get_datatype() == TYPE_STRUCT
									? expr->get_datatype_name()
									: get_datatype_name(expr->get_datatype()))));
						return nullptr;
					}
					}
				}
				OperatorNode* op = alloc_node<OperatorNode>();
				op->op = OP_INDEX;
				op->return_cache = member_type;
				op->struct_name = member_struct_name;
				op->arguments.push_back(expr);
				op->arguments.push_back(index);
				expr = op;

				tk = _get_token();
				if (tk.type != TK_BRACKET_CLOSE) {
					_set_expected_error("]");
					return nullptr;
				}

			}
			else if (tk.type == TK_OP_INCREMENT || tk.type == TK_OP_DECREMENT) {
				OperatorNode* op = alloc_node<OperatorNode>();
				op->op = tk.type == TK_OP_DECREMENT ? OP_POST_DECREMENT : OP_POST_INCREMENT;
				op->arguments.push_back(expr);

				if (!_validate_operator(
						p_block, p_function_info, op, &op->return_cache, &op->return_array_size)) {
					_set_error(RTR("Invalid base type for increment/decrement operator."));
					return nullptr;
				}

				String error;
				if (!_validate_assign(expr, p_function_info, &error)) {
					_set_error(error);
					return nullptr;
				}
				expr = op;
			}
			else {
				_set_tkpos(pos2);
				break;
			}
		}

		Expression e;
		e.is_op = false;
		e.node = expr;
		expression.push_back(e);

		pos = _get_tkpos();
		tk = _get_token();

		if (p_previous_expression_info != nullptr &&
			tk.type == p_previous_expression_info->tt_break &&
			!p_previous_expression_info->is_last_expr) {
			break;
		}

		if (is_token_operator(tk.type)) {
			Expression o;
			o.is_op = true;

			switch (tk.type) {
			case TK_OP_EQUAL:
				o.op = OP_EQUAL;
				break;
			case TK_OP_NOT_EQUAL:
				o.op = OP_NOT_EQUAL;
				break;
			case TK_OP_LESS:
				o.op = OP_LESS;
				break;
			case TK_OP_LESS_EQUAL:
				o.op = OP_LESS_EQUAL;
				break;
			case TK_OP_GREATER:
				o.op = OP_GREATER;
				break;
			case TK_OP_GREATER_EQUAL:
				o.op = OP_GREATER_EQUAL;
				break;
			case TK_OP_AND:
				o.op = OP_AND;
				break;
			case TK_OP_OR:
				o.op = OP_OR;
				break;
			case TK_OP_ADD:
				o.op = OP_ADD;
				break;
			case TK_OP_SUB:
				o.op = OP_SUB;
				break;
			case TK_OP_MUL:
				o.op = OP_MUL;
				break;
			case TK_OP_DIV:
				o.op = OP_DIV;
				break;
			case TK_OP_MOD:
				o.op = OP_MOD;
				break;
			case TK_OP_SHIFT_LEFT:
				o.op = OP_SHIFT_LEFT;
				break;
			case TK_OP_SHIFT_RIGHT:
				o.op = OP_SHIFT_RIGHT;
				break;
			case TK_OP_ASSIGN:
				o.op = OP_ASSIGN;
				break;
			case TK_OP_ASSIGN_ADD:
				o.op = OP_ASSIGN_ADD;
				break;
			case TK_OP_ASSIGN_SUB:
				o.op = OP_ASSIGN_SUB;
				break;
			case TK_OP_ASSIGN_MUL:
				o.op = OP_ASSIGN_MUL;
				break;
			case TK_OP_ASSIGN_DIV:
				o.op = OP_ASSIGN_DIV;
				break;
			case TK_OP_ASSIGN_MOD:
				o.op = OP_ASSIGN_MOD;
				break;
			case TK_OP_ASSIGN_SHIFT_LEFT:
				o.op = OP_ASSIGN_SHIFT_LEFT;
				break;
			case TK_OP_ASSIGN_SHIFT_RIGHT:
				o.op = OP_ASSIGN_SHIFT_RIGHT;
				break;
			case TK_OP_ASSIGN_BIT_AND:
				o.op = OP_ASSIGN_BIT_AND;
				break;
			case TK_OP_ASSIGN_BIT_OR:
				o.op = OP_ASSIGN_BIT_OR;
				break;
			case TK_OP_ASSIGN_BIT_XOR:
				o.op = OP_ASSIGN_BIT_XOR;
				break;
			case TK_OP_BIT_AND:
				o.op = OP_BIT_AND;
				break;
			case TK_OP_BIT_OR:
				o.op = OP_BIT_OR;
				break;
			case TK_OP_BIT_XOR:
				o.op = OP_BIT_XOR;
				break;
			case TK_QUESTION:
				o.op = OP_SELECT_IF;
				break;
			case TK_COLON:
				o.op = OP_SELECT_ELSE;
				break;
			default: {
				_set_error(
					vformat(RTR("Invalid token for the operator: '%s'."), get_token_text(tk)));
				return nullptr;
			}
			}

			expression.push_back(o);

			if (o.op == OP_SELECT_IF) {
				ExpressionInfo info;
				info.expression = &expression;
				info.tt_break = TK_COLON;

				expr = _parse_and_reduce_expression(p_block, p_function_info, &info);
				if (!expr) {
					return nullptr;
				}

				expression.push_back({true, {OP_SELECT_ELSE}});

				if (p_previous_expression_info != nullptr) {
					info.is_last_expr = p_previous_expression_info->is_last_expr;
				}
				else {
					info.is_last_expr = true;
				}

				expr = _parse_and_reduce_expression(p_block, p_function_info, &info);
				if (!expr) {
					return nullptr;
				}

				break;
			}
		}
		else {
			_set_tkpos(pos); // something else, so rollback and end
			break;
		}
	}

	/* Reduce the set set of expressions and place them in an operator tree, respecting precedence
	 */

	while (expression.size() > 1) {
		int next_op = -1;
		int min_priority = 0xFFFFF;
		bool is_unary = false;
		bool is_ternary = false;

		for (int i = 0; i < expression.size(); i++) {
			if (!expression[i].is_op) {
				continue;
			}

			bool unary = false;
			bool ternary = false;
			Operator op = expression[i].op;

			int priority;
			switch (op) {
			case OP_EQUAL:
				priority = 8;
				break;
			case OP_NOT_EQUAL:
				priority = 8;
				break;
			case OP_LESS:
				priority = 7;
				break;
			case OP_LESS_EQUAL:
				priority = 7;
				break;
			case OP_GREATER:
				priority = 7;
				break;
			case OP_GREATER_EQUAL:
				priority = 7;
				break;
			case OP_AND:
				priority = 12;
				break;
			case OP_OR:
				priority = 14;
				break;
			case OP_NOT:
				priority = 3;
				unary = true;
				break;
			case OP_NEGATE:
				priority = 3;
				unary = true;
				break;
			case OP_ADD:
				priority = 5;
				break;
			case OP_SUB:
				priority = 5;
				break;
			case OP_MUL:
				priority = 4;
				break;
			case OP_DIV:
				priority = 4;
				break;
			case OP_MOD:
				priority = 4;
				break;
			case OP_SHIFT_LEFT:
				priority = 6;
				break;
			case OP_SHIFT_RIGHT:
				priority = 6;
				break;
			case OP_ASSIGN:
				priority = 16;
				break;
			case OP_ASSIGN_ADD:
				priority = 16;
				break;
			case OP_ASSIGN_SUB:
				priority = 16;
				break;
			case OP_ASSIGN_MUL:
				priority = 16;
				break;
			case OP_ASSIGN_DIV:
				priority = 16;
				break;
			case OP_ASSIGN_MOD:
				priority = 16;
				break;
			case OP_ASSIGN_SHIFT_LEFT:
				priority = 16;
				break;
			case OP_ASSIGN_SHIFT_RIGHT:
				priority = 16;
				break;
			case OP_ASSIGN_BIT_AND:
				priority = 16;
				break;
			case OP_ASSIGN_BIT_OR:
				priority = 16;
				break;
			case OP_ASSIGN_BIT_XOR:
				priority = 16;
				break;
			case OP_BIT_AND:
				priority = 9;
				break;
			case OP_BIT_OR:
				priority = 11;
				break;
			case OP_BIT_XOR:
				priority = 10;
				break;
			case OP_BIT_INVERT:
				priority = 3;
				unary = true;
				break;
			case OP_INCREMENT:
				priority = 3;
				unary = true;
				break;
			case OP_DECREMENT:
				priority = 3;
				unary = true;
				break;
			case OP_SELECT_IF:
				priority = 15;
				ternary = true;
				break;
			case OP_SELECT_ELSE:
				priority = 15;
				ternary = true;
				break;

			default:
				ERR_FAIL_V(nullptr); // unexpected operator
			}

			if (priority < min_priority) {
				// < is used for left to right (default)
				// <= is used for right to left
				next_op = i;
				min_priority = priority;
				is_unary = unary;
				is_ternary = ternary;
			}
		}

		ERR_FAIL_COND_V(next_op == -1, nullptr);

		// OK! create operator..
		if (is_unary) {
			int expr_pos = next_op;
			while (expression[expr_pos].is_op) {
				expr_pos++;
				if (expr_pos == expression.size()) {
					// can happen..
					_set_error(RTR("Unexpected end of expression."));
					return nullptr;
				}
			}

			// consecutively do unary operators
			for (int i = expr_pos - 1; i >= next_op; i--) {
				OperatorNode* op = alloc_node<OperatorNode>();
				op->op = expression[i].op;

				String error;
				if ((op->op == OP_INCREMENT || op->op == OP_DECREMENT) &&
					!_validate_assign(expression[i + 1].node, p_function_info, &error)) {
					_set_error(error);
					return nullptr;
				}
				op->arguments.push_back(expression[i + 1].node);

				expression.write[i].is_op = false;
				expression.write[i].node = op;

				if (!_validate_operator(
						p_block, p_function_info, op, &op->return_cache, &op->return_array_size)) {
					if (error_set) {
						return nullptr;
					}

					String at;
					for (int j = 0; j < op->arguments.size(); j++) {
						if (j > 0) {
							at += ", ";
						}
						at += get_datatype_name(op->arguments[j]->get_datatype());
						if (!op->arguments[j]->is_indexed() &&
							op->arguments[j]->get_array_size() > 0) {
							at += "[";
							at += itos(op->arguments[j]->get_array_size());
							at += "]";
						}
					}
					_set_error(vformat(RTR("Invalid arguments to unary operator '%s': %s."),
						get_operator_text(op->op), at));
					return nullptr;
				}
				expression.write[i].node = _reduce_expression(p_block, expression.write[i].node);
				expression.remove_at(i + 1);
			}

		}
		else if (is_ternary) {
			if (next_op < 1 || next_op >= (expression.size() - 1)) {
				_set_parsing_error();
				ERR_FAIL_V(nullptr);
			}

			if (next_op + 2 >= expression.size() || !expression[next_op + 2].is_op ||
				expression[next_op + 2].op != OP_SELECT_ELSE) {
				_set_error(RTR("Missing matching ':' for select operator."));
				return nullptr;
			}

			OperatorNode* op = alloc_node<OperatorNode>();
			op->op = expression[next_op].op;
			op->arguments.push_back(expression[next_op - 1].node);
			op->arguments.push_back(expression[next_op + 1].node);
			op->arguments.push_back(expression[next_op + 3].node);

			expression.write[next_op - 1].is_op = false;
			expression.write[next_op - 1].node = op;
			if (!_validate_operator(p_block, p_function_info, op, &op->return_cache,
					&op->return_array_size, &op->struct_name)) {
				if (error_set) {
					return nullptr;
				}

				String at;
				for (int i = 0; i < op->arguments.size(); i++) {
					if (i > 0) {
						at += ", ";
					}
					DataType dt = op->arguments[i]->get_datatype();
					if (dt == TYPE_STRUCT) {
						at += op->arguments[i]->get_datatype_name();
					}
					else {
						at += get_datatype_name(dt);
					}
					if (!op->arguments[i]->is_indexed() && op->arguments[i]->get_array_size() > 0) {
						at += "[";
						at += itos(op->arguments[i]->get_array_size());
						at += "]";
					}
				}
				_set_error(vformat(RTR("Invalid argument to ternary operator: '%s'."), at));
				return nullptr;
			}

			for (int i = 0; i < 4; i++) {
				expression.remove_at(next_op);
			}

		}
		else {
			if (next_op < 1 || next_op >= (expression.size() - 1)) {
				_set_parsing_error();
				ERR_FAIL_V(nullptr);
			}

			OperatorNode* op = alloc_node<OperatorNode>();
			op->op = expression[next_op].op;

			if (expression[next_op - 1].is_op) {
				_set_parsing_error();
				ERR_FAIL_V(nullptr);
			}

			if (_is_operator_assign(op->op)) {
				if (p_block && expression[next_op - 1].node->type == Node::NODE_TYPE_VARIABLE) {
					VariableNode* vn = static_cast<VariableNode*>(expression[next_op - 1].node);
					p_block->use_op_eval = vn->is_const;
				}

				String assign_message;
				if (!_validate_assign(
						expression[next_op - 1].node, p_function_info, &assign_message)) {
					_set_error(assign_message);
					return nullptr;
				}
			}

			if (expression[next_op + 1].is_op) {
				// this is not invalid and can really appear
				// but it becomes invalid anyway because no binary op
				// can be followed by a unary op in a valid combination,
				// due to how precedence works, unaries will always disappear first

				_set_parsing_error();
			}

			op->arguments.push_back(expression[next_op - 1].node); // expression goes as left
			op->arguments.push_back(expression[next_op + 1].node); // next expression goes as right
			expression.write[next_op - 1].node = op;

			// replace all 3 nodes by this operator and make it an expression

			if (!_validate_operator(
					p_block, p_function_info, op, &op->return_cache, &op->return_array_size)) {
				if (error_set) {
					return nullptr;
				}

				String at;
				for (int i = 0; i < op->arguments.size(); i++) {
					if (i > 0) {
						at += ", ";
					}
					if (op->arguments[i]->get_datatype() == TYPE_STRUCT) {
						at += op->arguments[i]->get_datatype_name();
					}
					else {
						at += get_datatype_name(op->arguments[i]->get_datatype());
					}
					if (!op->arguments[i]->is_indexed() && op->arguments[i]->get_array_size() > 0) {
						at += "[";
						at += itos(op->arguments[i]->get_array_size());
						at += "]";
					}
				}
				_set_error(vformat(RTR("Invalid arguments to operator '%s': '%s'."),
					get_operator_text(op->op), at));
				return nullptr;
			}

			expression.remove_at(next_op);
			expression.remove_at(next_op);
		}
	}

	if (p_block) {
		p_block->use_op_eval = true;
	}

	if (p_previous_expression_info != nullptr) {
		p_previous_expression_info->expression->push_back(expression[0]);
	}

	return expression[0].node;
}

ShaderLanguage::Node* ShaderLanguage::_reduce_expression(
	BlockNode* p_block, ShaderLanguage::Node* p_node)
{
	if (p_node->type != Node::NODE_TYPE_OPERATOR) {
		return p_node;
	}

	// for now only reduce simple constructors
	OperatorNode* op = static_cast<OperatorNode*>(p_node);

	if (op->op == OP_CONSTRUCT) {
		ERR_FAIL_COND_V(op->arguments[0]->type != Node::NODE_TYPE_VARIABLE, p_node);

		DataType type = op->get_datatype();
		DataType base = get_scalar_type(type);
		int cardinality = get_cardinality(type);

		Vector<Scalar> values;

		for (int i = 1; i < op->arguments.size(); i++) {
			op->arguments.write[i] = _reduce_expression(p_block, op->arguments[i]);
			if (op->arguments[i]->type == Node::NODE_TYPE_CONSTANT) {
				ConstantNode* cn = static_cast<ConstantNode*>(op->arguments[i]);

				if (get_scalar_type(cn->datatype) == base) {
					for (int j = 0; j < cn->values.size(); j++) {
						values.push_back(cn->values[j]);
					}
				}
				else if (get_scalar_type(cn->datatype) == cn->datatype) {
					Scalar v;
					if (!convert_constant(cn, base, &v)) {
						return p_node;
					}
					values.push_back(v);
				}
				else {
					return p_node;
				}

			}
			else {
				return p_node;
			}
		}

		if (values.size() == 1) {
			if (type >= TYPE_MAT2 && type <= TYPE_MAT4) {
				Scalar value = values[0];
				Scalar zero;
				zero.real = 0.0f;
				int size = 2 + (type - TYPE_MAT2);

				values.clear();
				for (int i = 0; i < size; i++) {
					for (int j = 0; j < size; j++) {
						values.push_back(i == j ? value : zero);
					}
				}
			}
			else {
				Scalar value = values[0];
				for (int i = 1; i < cardinality; i++) {
					values.push_back(value);
				}
			}
		}
		else if (values.size() != cardinality) {
			ERR_PRINT("Failed to reduce expression, values and cardinality mismatch.");
			return p_node;
		}

		ConstantNode* cn = alloc_node<ConstantNode>();
		cn->datatype = op->get_datatype();
		cn->values = values;
		return cn;
	}
	else if (op->op == OP_NEGATE) {
		op->arguments.write[0] = _reduce_expression(p_block, op->arguments[0]);
		if (op->arguments[0]->type == Node::NODE_TYPE_CONSTANT) {
			ConstantNode* cn = static_cast<ConstantNode*>(op->arguments[0]);

			DataType base = get_scalar_type(cn->datatype);

			Vector<Scalar> values;

			for (int i = 0; i < cn->values.size(); i++) {
				Scalar nv;
				switch (base) {
				case TYPE_BOOL: {
					nv.boolean = !cn->values[i].boolean;
				} break;
				case TYPE_INT: {
					nv.sint = -cn->values[i].sint;
				} break;
				case TYPE_UINT: {
					// Intentionally wrap the unsigned int value, because GLSL does.
					nv.uint = 0 - cn->values[i].uint;
				} break;
				case TYPE_FLOAT: {
					nv.real = -cn->values[i].real;
				} break;
				default: {
				}
				}

				values.push_back(nv);
			}

			cn->values = values;
			return cn;
		}
	}

	return p_node;
}

ShaderLanguage::Node* ShaderLanguage::_parse_and_reduce_expression(BlockNode* p_block,
	const FunctionInfo& p_function_info, const ExpressionInfo* p_previous_expression_info)
{
	ShaderLanguage::Node* expr =
		_parse_expression(p_block, p_function_info, p_previous_expression_info);
	if (!expr) { // errored
		return nullptr;
	}

	expr = _reduce_expression(p_block, expr);

	return expr;
}

Error ShaderLanguage::_parse_block(BlockNode* p_block, const FunctionInfo& p_function_info,
	bool p_just_one, bool p_can_break, bool p_can_continue)
{
	while (true) {
		TkPos pos = _get_tkpos();
		Token tk = _get_token();
#ifdef DEBUG_ENABLED
		Token next;
#endif // DEBUG_ENABLED

		if (p_block && p_block->block_type == BlockNode::BLOCK_TYPE_SWITCH) {
			if (tk.type != TK_CF_CASE && tk.type != TK_CF_DEFAULT &&
				tk.type != TK_CURLY_BRACKET_CLOSE) {
				_set_error(vformat(
					RTR("A switch may only contain '%s' and '%s' blocks."), "case", "default"));
				return ERR_PARSE_ERROR;
			}
		}

		bool is_struct = shader->structs.has(tk.text) &&
						 !(p_block && _find_identifier(p_block, false, p_function_info, tk.text));
		bool is_var_init = false;
		bool is_condition = false;

		if (tk.type == TK_CURLY_BRACKET_CLOSE) { // end of block
			if (p_just_one) {
				_set_expected_error("}");
				return ERR_PARSE_ERROR;
			}

			return OK;

		}
		else if (tk.type == TK_CONST || is_token_precision(tk.type) ||
				   is_token_nonvoid_datatype(tk.type) || is_struct) {
			is_var_init = true;

			String struct_name = "";
			if (is_struct) {
				struct_name = tk.text;
#ifdef DEBUG_ENABLED
				if (check_warnings && HAS_WARNING(ShaderWarning::UNUSED_STRUCT_FLAG) &&
					used_structs.has(struct_name)) {
					used_structs[struct_name].used = true;
				}
#endif // DEBUG_ENABLED
			}
#ifdef DEBUG_ENABLED
			uint32_t precision_flag = CF_PRECISION_MODIFIER;

			keyword_completion_context = CF_DATATYPE;
			if (!is_token_precision(tk.type)) {
				if (!is_struct) {
					keyword_completion_context |= precision_flag;
				}
			}
#endif // DEBUG_ENABLED

			bool is_const = false;

			if (tk.type == TK_CONST) {
				is_const = true;
				tk = _get_token();

				if (!is_struct) {
					is_struct = shader->structs.has(tk.text); // check again.
					struct_name = tk.text;
				}
			}

			DataPrecision precision = PRECISION_DEFAULT;
			if (is_token_precision(tk.type)) {
				precision = get_token_precision(tk.type);
				tk = _get_token();

				if (!is_struct) {
					is_struct = shader->structs.has(tk.text); // check again.
				}

#ifdef DEBUG_ENABLED
				if (keyword_completion_context & precision_flag) {
					keyword_completion_context ^= precision_flag;
				}
#endif // DEBUG_ENABLED
			}

#ifdef DEBUG_ENABLED
			if (is_const && _lookup_next(next)) {
				if (is_token_precision(next.type)) {
					keyword_completion_context = CF_UNSPECIFIED;
				}
				if (is_token_datatype(next.type)) {
					keyword_completion_context ^= CF_DATATYPE;
				}
			}
#endif // DEBUG_ENABLED

			if (precision != PRECISION_DEFAULT) {
				if (!is_token_nonvoid_datatype(tk.type)) {
					_set_error(RTR("Expected variable type after precision modifier."));
					return ERR_PARSE_ERROR;
				}
			}

			if (!is_struct) {
				if (!is_token_variable_datatype(tk.type)) {
					_set_error(RTR("Invalid variable type (samplers are not allowed)."));
					return ERR_PARSE_ERROR;
				}
			}

			DataType type = is_struct ? TYPE_STRUCT : get_token_datatype(tk.type);

			if (precision != PRECISION_DEFAULT && _validate_precision(type, precision) != OK) {
				return ERR_PARSE_ERROR;
			}

#ifdef DEBUG_ENABLED
			keyword_completion_context = CF_UNSPECIFIED;
#endif // DEBUG_ENABLED

			int array_size = 0;
			bool fixed_array_size = false;
			bool first = true;

			VariableDeclarationNode* vdnode = alloc_node<VariableDeclarationNode>();
			vdnode->precision = precision;
			if (is_struct) {
				vdnode->struct_name = struct_name;
				vdnode->datatype = TYPE_STRUCT;
			}
			else {
				vdnode->datatype = type;
			};
			vdnode->is_const = is_const;

			do {
				bool unknown_size = false;
				VariableDeclarationNode::Declaration decl;

				tk = _get_token();

				if (first) {
					first = false;

					if (tk.type != TK_IDENTIFIER && tk.type != TK_BRACKET_OPEN) {
						_set_error(RTR("Expected an identifier or '[' after type."));
						return ERR_PARSE_ERROR;
					}

					if (tk.type == TK_BRACKET_OPEN) {
						Error error = _parse_array_size(p_block, p_function_info, false,
							&decl.size_expression, &array_size, &unknown_size);
						if (error != OK) {
							return error;
						}
						decl.size = array_size;

						fixed_array_size = true;
						tk = _get_token();
					}
				}

				if (tk.type != TK_IDENTIFIER) {
					_set_error(RTR("Expected an identifier."));
					return ERR_PARSE_ERROR;
				}

				String name = tk.text;
				ShaderLanguage::IdentifierType itype;
				if (_find_identifier(p_block, true, p_function_info, name,
						(ShaderLanguage::DataType*)nullptr, &itype)) {
					if (itype != IDENTIFIER_FUNCTION) {
						_set_redefinition_error(String(name));
						return ERR_PARSE_ERROR;
					}
				}
				decl.name = name;

#ifdef DEBUG_ENABLED
				if (check_warnings && HAS_WARNING(ShaderWarning::UNUSED_LOCAL_VARIABLE_FLAG) &&
					p_block) {
					FunctionNode* parent_function = nullptr;
					{
						BlockNode* block = p_block;
						while (block && !block->parent_function) {
							block = block->parent_block;
						}
						parent_function = block->parent_function;
					}
					if (parent_function) {
						String func_name = parent_function->name;

						if (!used_local_vars.has(func_name)) {
							used_local_vars.insert(func_name, HashMap<String, Usage>());
						}

						used_local_vars[func_name].insert(name, Usage(tk_line));
					}
				}
#endif // DEBUG_ENABLED
				is_const_decl = is_const;

				BlockNode::Variable var;
				var.type = type;
				var.precision = precision;
				var.line = tk_line;
				var.array_size = array_size;
				var.is_const = is_const;
				var.struct_name = struct_name;

				tk = _get_token();

				if (tk.type == TK_BRACKET_OPEN) {
					Error error = _parse_array_size(p_block, p_function_info, false,
						&decl.size_expression, &var.array_size, &unknown_size);
					if (error != OK) {
						return error;
					}

					decl.size = var.array_size;
					array_size = var.array_size;

					tk = _get_token();
				}
#ifdef DEBUG_ENABLED
				if (var.type == DataType::TYPE_BOOL) {
					keyword_completion_context = CF_BOOLEAN;
				}
#endif // DEBUG_ENABLED
				if (var.array_size > 0 || unknown_size) {
					bool full_def = false;

					if (tk.type == TK_OP_ASSIGN) {
						TkPos prev_pos = _get_tkpos();
						tk = _get_token();

						if (tk.type == TK_IDENTIFIER) { // a function call array initialization
							_set_tkpos(prev_pos);
							Node* n = _parse_and_reduce_expression(p_block, p_function_info);

							if (!n) {
								_set_error(RTR("Expected array initializer."));
								return ERR_PARSE_ERROR;
							}
							else {
								if (unknown_size) {
									decl.size = n->get_array_size();
									var.array_size = n->get_array_size();
								}

								if (!_compare_datatypes(var.type, var.struct_name, var.array_size,
										n->get_datatype(), n->get_datatype_name(),
										n->get_array_size())) {
									return ERR_PARSE_ERROR;
								}

								decl.single_expression = true;
								decl.initializer.push_back(n);
							}

							tk = _get_token();
						}
						else {
							if (tk.type != TK_CURLY_BRACKET_OPEN) {
								if (unknown_size) {
									_set_expected_error("{");
									return ERR_PARSE_ERROR;
								}

								full_def = true;

								DataPrecision precision2 = PRECISION_DEFAULT;
								if (is_token_precision(tk.type)) {
									precision2 = get_token_precision(tk.type);
									tk = _get_token();
									if (!is_token_nonvoid_datatype(tk.type)) {
										_set_error(
											RTR("Expected data type after precision modifier."));
										return ERR_PARSE_ERROR;
									}
								}

								DataType type2;
								String struct_name2 = "";

								if (shader->structs.has(tk.text)) {
									type2 = TYPE_STRUCT;
									struct_name2 = tk.text;
								}
								else {
									if (!is_token_variable_datatype(tk.type)) {
										_set_error(RTR("Invalid data type for the array."));
										return ERR_PARSE_ERROR;
									}
									type2 = get_token_datatype(tk.type);
								}

								if (precision2 != PRECISION_DEFAULT &&
									_validate_precision(type2, precision2) != OK) {
									return ERR_PARSE_ERROR;
								}

								int array_size2 = 0;

								tk = _get_token();
								if (tk.type == TK_BRACKET_OPEN) {
									bool is_unknown_size = false;
									Error error = _parse_array_size(p_block, p_function_info, false,
										nullptr, &array_size2, &is_unknown_size);
									if (error != OK) {
										return error;
									}
									if (is_unknown_size) {
										array_size2 = var.array_size;
									}
									tk = _get_token();
								}
								else {
									_set_expected_error("[");
									return ERR_PARSE_ERROR;
								}

								if (precision != precision2 || type != type2 ||
									struct_name != struct_name2 || var.array_size != array_size2) {
									String from;
									if (precision2 != PRECISION_DEFAULT) {
										from += get_precision_name(precision2);
										from += " ";
									}
									if (type2 == TYPE_STRUCT) {
										from += struct_name2;
									}
									else {
										from += get_datatype_name(type2);
									}
									from += "[";
									from += itos(array_size2);
									from += "]'";

									String to;
									if (precision != PRECISION_DEFAULT) {
										to += get_precision_name(precision);
										to += " ";
									}
									if (type == TYPE_STRUCT) {
										to += struct_name;
									}
									else {
										to += get_datatype_name(type);
									}
									to += "[";
									to += itos(var.array_size);
									to += "]'";

									_set_error(vformat(
										RTR("Cannot convert from '%s' to '%s'."), from, to));
									return ERR_PARSE_ERROR;
								}
							}

							bool curly = tk.type == TK_CURLY_BRACKET_OPEN;

							if (unknown_size) {
								if (!curly) {
									_set_expected_error("{");
									return ERR_PARSE_ERROR;
								}
							}
							else {
								if (full_def) {
									if (curly) {
										_set_expected_error("(");
										return ERR_PARSE_ERROR;
									}
								}
							}

							if (tk.type == TK_PARENTHESIS_OPEN || curly) { // initialization
								while (true) {
									Node* n =
										_parse_and_reduce_expression(p_block, p_function_info);
									if (!n) {
										return ERR_PARSE_ERROR;
									}

									if (is_const && n->type == Node::NODE_TYPE_OPERATOR &&
										static_cast<OperatorNode*>(n)->op == OP_CALL) {
										_set_error(RTR("Expected a constant expression."));
										return ERR_PARSE_ERROR;
									}

									if (!_compare_datatypes(var.type, struct_name, 0,
											n->get_datatype(), n->get_datatype_name(), 0)) {
										return ERR_PARSE_ERROR;
									}

									tk = _get_token();
									if (tk.type == TK_COMMA) {
										decl.initializer.push_back(n);
										continue;
									}
									else if (!curly && tk.type == TK_PARENTHESIS_CLOSE) {
										decl.initializer.push_back(n);
										break;
									}
									else if (curly && tk.type == TK_CURLY_BRACKET_CLOSE) {
										decl.initializer.push_back(n);
										break;
									}
									else {
										if (curly) {
											_set_expected_error("}", ",");
										}
										else {
											_set_expected_error(")", ",");
										}
										return ERR_PARSE_ERROR;
									}
								}
								if (unknown_size) {
									decl.size = decl.initializer.size();
									var.array_size = decl.initializer.size();
								}
								else if (decl.initializer.size() != var.array_size) {
									_set_error(vformat(RTR("Array size mismatch. Expected %d "
														   "elements (found %d)."),
										var.array_size, decl.initializer.size()));
									return ERR_PARSE_ERROR;
								}
								tk = _get_token();
							}
							else {
								_set_expected_error("(");
								return ERR_PARSE_ERROR;
							}
						}
					}
					else {
						if (unknown_size) {
							_set_error(RTR("Expected array initialization."));
							return ERR_PARSE_ERROR;
						}
						if (is_const) {
							_set_error(RTR("Expected initialization of constant."));
							return ERR_PARSE_ERROR;
						}
					}

					array_size = var.array_size;
				}
				else if (tk.type == TK_OP_ASSIGN) {
					p_block->use_op_eval = is_const;

					// Variable created with assignment! Must parse an expression.
					Node* n = _parse_and_reduce_expression(p_block, p_function_info);
					if (!n) {
						return ERR_PARSE_ERROR;
					}
					if (is_const && n->type == Node::NODE_TYPE_OPERATOR &&
						static_cast<OperatorNode*>(n)->op == OP_CALL) {
						OperatorNode* op = static_cast<OperatorNode*>(n);
						for (int i = 1; i < op->arguments.size(); i++) {
							if (!_check_node_constness(op->arguments[i])) {
								_set_error(vformat(RTR("Expected constant expression for argument "
													   "%d of function call after '='."),
									i - 1));
								return ERR_PARSE_ERROR;
							}
						}
					}

					if (is_const) {
						var.values = n->get_values();
					}

					if (!_compare_datatypes(var.type, var.struct_name, var.array_size,
							n->get_datatype(), n->get_datatype_name(), n->get_array_size())) {
						return ERR_PARSE_ERROR;
					}

					decl.initializer.push_back(n);
					tk = _get_token();
				}
				else {
					if (is_const) {
						_set_error(RTR("Expected initialization of constant."));
						return ERR_PARSE_ERROR;
					}
				}

				vdnode->declarations.push_back(decl);
				p_block->variables[name] = var;
				is_const_decl = false;

				if (!fixed_array_size) {
					array_size = 0;
				}

				if (tk.type == TK_SEMICOLON) {
					break;
				}
				else if (tk.type != TK_COMMA) {
					_set_expected_error(",", ";");
					return ERR_PARSE_ERROR;
				}
			} while (tk.type == TK_COMMA); // another variable
#ifdef DEBUG_ENABLED
			keyword_completion_context = CF_BLOCK;
#endif // DEBUG_ENABLED
			p_block->statements.push_back(static_cast<Node*>(vdnode));
		}
		else if (tk.type == TK_CURLY_BRACKET_OPEN) {
			// a sub block, just because..
			BlockNode* block = alloc_node<BlockNode>();
			block->parent_block = p_block;
			if (_parse_block(block, p_function_info, false, p_can_break, p_can_continue) != OK) {
				return ERR_PARSE_ERROR;
			}
			p_block->statements.push_back(block);
		}
		else if (tk.type == TK_CF_IF) {
			// if () {}
			tk = _get_token();
			if (tk.type != TK_PARENTHESIS_OPEN) {
				_set_expected_after_error("(", "if");
				return ERR_PARSE_ERROR;
			}

			ControlFlowNode* cf = alloc_node<ControlFlowNode>();
			cf->flow_op = FLOW_OP_IF;
#ifdef DEBUG_ENABLED
			keyword_completion_context = CF_IF_DECL;
#endif // DEBUG_ENABLED
			Node* n = _parse_and_reduce_expression(p_block, p_function_info);
			if (!n) {
				return ERR_PARSE_ERROR;
			}
#ifdef DEBUG_ENABLED
			keyword_completion_context = CF_BLOCK;
#endif // DEBUG_ENABLED

			if (n->get_datatype() != TYPE_BOOL) {
				_set_error(RTR("Expected a boolean expression."));
				return ERR_PARSE_ERROR;
			}

			tk = _get_token();
			if (tk.type != TK_PARENTHESIS_CLOSE) {
				_set_expected_error(")");
				return ERR_PARSE_ERROR;
			}

			BlockNode* block = alloc_node<BlockNode>();
			block->parent_block = p_block;
			cf->expressions.push_back(n);
			cf->blocks.push_back(block);
			p_block->statements.push_back(cf);

			Error err = _parse_block(block, p_function_info, true, p_can_break, p_can_continue);
			if (err) {
				return err;
			}

			pos = _get_tkpos();
			tk = _get_token();
			if (tk.type == TK_CF_ELSE) {
				block = alloc_node<BlockNode>();
				block->parent_block = p_block;
				cf->blocks.push_back(block);
				err = _parse_block(block, p_function_info, true, p_can_break, p_can_continue);
				if (err) {
					return err;
				}
			}
			else {
				_set_tkpos(pos); // rollback
			}
		}
		else if (tk.type == TK_CF_SWITCH) {
			// switch() {}
			tk = _get_token();
			if (tk.type != TK_PARENTHESIS_OPEN) {
				_set_expected_after_error("(", "switch");
				return ERR_PARSE_ERROR;
			}
			ControlFlowNode* cf = alloc_node<ControlFlowNode>();
			cf->flow_op = FLOW_OP_SWITCH;
			Node* n = _parse_and_reduce_expression(p_block, p_function_info);
			if (!n) {
				return ERR_PARSE_ERROR;
			}
			const ShaderLanguage::DataType data_type = n->get_datatype();
			if (data_type != TYPE_INT && data_type != TYPE_UINT) {
				_set_error(RTR("Expected an integer or unsigned integer expression."));
				return ERR_PARSE_ERROR;
			}
			tk = _get_token();
			if (tk.type != TK_PARENTHESIS_CLOSE) {
				_set_expected_error(")");
				return ERR_PARSE_ERROR;
			}
			tk = _get_token();
			if (tk.type != TK_CURLY_BRACKET_OPEN) {
				_set_expected_after_error("{", "switch");
				return ERR_PARSE_ERROR;
			}
			BlockNode* switch_block = alloc_node<BlockNode>();
			switch_block->block_type = BlockNode::BLOCK_TYPE_SWITCH;
			switch_block->parent_block = p_block;
			switch_block->expected_type = data_type;
			cf->expressions.push_back(n);
			cf->blocks.push_back(switch_block);
			p_block->statements.push_back(cf);

			pos = _get_tkpos();
			tk = _get_token();
			bool has_default = false;
			if (tk.type == TK_CF_CASE || tk.type == TK_CF_DEFAULT) {
				if (tk.type == TK_CF_DEFAULT) {
					has_default = true;
				}
				_set_tkpos(pos);
			}
			else {
				_set_expected_error("case", "default");
				return ERR_PARSE_ERROR;
			}

			while (true) { // Go-through multiple cases.

				if (_parse_block(switch_block, p_function_info, true, true, false) != OK) {
					return ERR_PARSE_ERROR;
				}
				pos = _get_tkpos();
				tk = _get_token();
				if (tk.type == TK_CF_CASE) {
					_set_tkpos(pos);
					continue;
				}
				else if (tk.type == TK_CF_DEFAULT) {
					if (has_default) {
						_set_error(RTR("Default case must be defined only once."));
						return ERR_PARSE_ERROR;
					}
					has_default = true;
					_set_tkpos(pos);
					continue;
				}
				else {
					break;
				}
			}

		}
		else if (tk.type == TK_CF_CASE) {
			// case x : break; | return;

			if (p_block && p_block->block_type == BlockNode::BLOCK_TYPE_CASE) {
				_set_tkpos(pos);
				return OK;
			}

			if (!p_block || (p_block->block_type != BlockNode::BLOCK_TYPE_SWITCH)) {
				_set_error(
					vformat(RTR("'%s' must be placed within a '%s' block."), "case", "switch"));
				return ERR_PARSE_ERROR;
			}

			tk = _get_token();

			int sign = 1;

			if (tk.type == TK_OP_SUB) {
				sign = -1;
				tk = _get_token();
			}

			Node* n = nullptr;

			if (!tk.is_integer_constant()) {
				bool correct_constant_expression = false;

				if (tk.type == TK_IDENTIFIER) {
					DataType data_type;
					bool is_const;

					bool found = _find_identifier(
						p_block, false, p_function_info, tk.text, &data_type, nullptr, &is_const);
					if (!found) {
						_set_error(vformat(
							RTR("Undefined identifier '%s' in a case label."), String(tk.text)));
						return ERR_PARSE_ERROR;
					}
					if (is_const && data_type == p_block->expected_type) {
						correct_constant_expression = true;
					}
				}

				if (!correct_constant_expression) {
					if (p_block->expected_type == TYPE_UINT) {
						_set_error(RTR("Expected an unsigned integer constant."));
					}
					else {
						_set_error(RTR("Expected an integer constant."));
					}
					return ERR_PARSE_ERROR;
				}

				VariableNode* vn = alloc_node<VariableNode>();
				vn->name = tk.text;
				{
					Vector<Scalar> v;
					DataType data_type;

					_find_identifier(p_block, false, p_function_info, vn->name, &data_type, nullptr,
						nullptr, nullptr, nullptr, &v);
					if (data_type == TYPE_INT) {
						if (p_block->constants.has(v[0].sint)) {
							_set_error(vformat(RTR("Duplicated case label: %d."), v[0].sint));
							return ERR_PARSE_ERROR;
						}
						p_block->constants.insert(v[0].sint);
					}
					else {
						if (p_block->constants.has(v[0].uint)) {
							_set_error(vformat(RTR("Duplicated case label: %d."), v[0].uint));
							return ERR_PARSE_ERROR;
						}
						p_block->constants.insert(v[0].uint);
					}
				}
				n = vn;
			}
			else {
				ConstantNode* cn = alloc_node<ConstantNode>();
				Scalar v;
				if (p_block->expected_type == TYPE_UINT) {
					if (tk.type != TK_UINT_CONSTANT) {
						_set_error(RTR("Expected an unsigned integer constant."));
						return ERR_PARSE_ERROR;
					}
					v.uint = (uint32_t)tk.constant;
					if (p_block->constants.has(v.uint)) {
						_set_error(vformat(RTR("Duplicated case label: %d."), v.uint));
						return ERR_PARSE_ERROR;
					}
					p_block->constants.insert(v.uint);
					cn->datatype = TYPE_UINT;
				}
				else {
					if (tk.type != TK_INT_CONSTANT) {
						_set_error(RTR("Expected an integer constant."));
						return ERR_PARSE_ERROR;
					}
					v.sint = (int32_t)tk.constant * sign;
					if (p_block->constants.has(v.sint)) {
						_set_error(vformat(RTR("Duplicated case label: %d."), v.sint));
						return ERR_PARSE_ERROR;
					}
					p_block->constants.insert(v.sint);
					cn->datatype = TYPE_INT;
				}
				cn->values.push_back(v);
				n = cn;
			}

			tk = _get_token();

			if (tk.type != TK_COLON) {
				_set_expected_error(":");
				return ERR_PARSE_ERROR;
			}

			ControlFlowNode* cf = alloc_node<ControlFlowNode>();
			cf->flow_op = FLOW_OP_CASE;

			BlockNode* case_block = alloc_node<BlockNode>();
			case_block->block_type = BlockNode::BLOCK_TYPE_CASE;
			case_block->parent_block = p_block;
			cf->expressions.push_back(n);
			cf->blocks.push_back(case_block);
			p_block->statements.push_back(cf);

			Error err = _parse_block(case_block, p_function_info, false, true, false);
			if (err) {
				return err;
			}

			return OK;

		}
		else if (tk.type == TK_CF_DEFAULT) {
			if (p_block && p_block->block_type == BlockNode::BLOCK_TYPE_CASE) {
				_set_tkpos(pos);
				return OK;
			}

			if (!p_block || (p_block->block_type != BlockNode::BLOCK_TYPE_SWITCH)) {
				_set_error(
					vformat(RTR("'%s' must be placed within a '%s' block."), "default", "switch"));
				return ERR_PARSE_ERROR;
			}

			tk = _get_token();

			if (tk.type != TK_COLON) {
				_set_expected_error(":");
				return ERR_PARSE_ERROR;
			}

			ControlFlowNode* cf = alloc_node<ControlFlowNode>();
			cf->flow_op = FLOW_OP_DEFAULT;

			BlockNode* default_block = alloc_node<BlockNode>();
			default_block->block_type = BlockNode::BLOCK_TYPE_DEFAULT;
			default_block->parent_block = p_block;
			cf->blocks.push_back(default_block);
			p_block->statements.push_back(cf);

			Error err = _parse_block(default_block, p_function_info, false, true, false);
			if (err) {
				return err;
			}

			return OK;

		}
		else if (tk.type == TK_CF_DO || tk.type == TK_CF_WHILE) {
			// do {} while()
			// while() {}
			bool is_do = tk.type == TK_CF_DO;

			BlockNode* do_block = nullptr;
			if (is_do) {
				do_block = alloc_node<BlockNode>();
				do_block->parent_block = p_block;

				Error err = _parse_block(do_block, p_function_info, true, true, true);
				if (err) {
					return err;
				}

				tk = _get_token();
				if (tk.type != TK_CF_WHILE) {
					_set_expected_after_error("while", "do");
					return ERR_PARSE_ERROR;
				}
			}
			tk = _get_token();

			if (tk.type != TK_PARENTHESIS_OPEN) {
				_set_expected_after_error("(", "while");
				return ERR_PARSE_ERROR;
			}

			ControlFlowNode* cf = alloc_node<ControlFlowNode>();
			if (is_do) {
				cf->flow_op = FLOW_OP_DO;
			}
			else {
				cf->flow_op = FLOW_OP_WHILE;
			}
			Node* n = _parse_and_reduce_expression(p_block, p_function_info);
			if (!n) {
				return ERR_PARSE_ERROR;
			}

			tk = _get_token();
			if (tk.type != TK_PARENTHESIS_CLOSE) {
				_set_expected_error(")");
				return ERR_PARSE_ERROR;
			}
			if (!is_do) {
				BlockNode* block = alloc_node<BlockNode>();
				block->parent_block = p_block;
				cf->expressions.push_back(n);
				cf->blocks.push_back(block);
				p_block->statements.push_back(cf);

				Error err = _parse_block(block, p_function_info, true, true, true);
				if (err) {
					return err;
				}
			}
			else {
				cf->expressions.push_back(n);
				cf->blocks.push_back(do_block);
				p_block->statements.push_back(cf);

				tk = _get_token();
				if (tk.type != TK_SEMICOLON) {
					_set_expected_error(";");
					return ERR_PARSE_ERROR;
				}
			}
		}
		else if (tk.type == TK_CF_FOR) {
			// for() {}
			tk = _get_token();
			if (tk.type != TK_PARENTHESIS_OPEN) {
				_set_expected_after_error("(", "for");
				return ERR_PARSE_ERROR;
			}

			ControlFlowNode* cf = alloc_node<ControlFlowNode>();
			cf->flow_op = FLOW_OP_FOR;

			BlockNode* init_block = alloc_node<BlockNode>();
			init_block->block_type = BlockNode::BLOCK_TYPE_FOR_INIT;
			init_block->parent_block = p_block;
			init_block->single_statement = true;
			cf->blocks.push_back(init_block);

#ifdef DEBUG_ENABLED
			keyword_completion_context = CF_DATATYPE;
#endif // DEBUG_ENABLED
			Error err = _parse_block(init_block, p_function_info, true, false, false);
			if (err != OK) {
				return err;
			}
#ifdef DEBUG_ENABLED
			keyword_completion_context = CF_UNSPECIFIED;
#endif // DEBUG_ENABLED

			BlockNode* condition_block = alloc_node<BlockNode>();
			condition_block->block_type = BlockNode::BLOCK_TYPE_FOR_CONDITION;
			condition_block->parent_block = init_block;
			condition_block->single_statement = true;
			condition_block->use_comma_between_statements = true;
			cf->blocks.push_back(condition_block);
			err = _parse_block(condition_block, p_function_info, true, false, false);
			if (err != OK) {
				return err;
			}

			BlockNode* expression_block = alloc_node<BlockNode>();
			expression_block->block_type = BlockNode::BLOCK_TYPE_FOR_EXPRESSION;
			expression_block->parent_block = init_block;
			expression_block->single_statement = true;
			expression_block->use_comma_between_statements = true;
			cf->blocks.push_back(expression_block);
			err = _parse_block(expression_block, p_function_info, true, false, false);
			if (err != OK) {
				return err;
			}

			BlockNode* block = alloc_node<BlockNode>();
			block->parent_block = init_block;
			cf->blocks.push_back(block);
			p_block->statements.push_back(cf);

#ifdef DEBUG_ENABLED
			keyword_completion_context = CF_BLOCK;
#endif // DEBUG_ENABLED
			err = _parse_block(block, p_function_info, true, true, true);
			if (err != OK) {
				return err;
			}

		}
		else if (tk.type == TK_CF_RETURN) {
			// check return type
			BlockNode* b = p_block;

			while (b && !b->parent_function) {
				b = b->parent_block;
			}

			if (!b) {
				_set_parsing_error();
				return ERR_BUG;
			}

			if (b->parent_function && p_function_info.main_function) {
				_set_error(vformat(RTR("Using '%s' in the '%s' processor function is incorrect."),
					"return", b->parent_function->name));
				return ERR_PARSE_ERROR;
			}

			String return_struct_name = String(b->parent_function->return_struct_name);
			String array_size_string;

			if (b->parent_function->return_array_size > 0) {
				array_size_string = "[" + itos(b->parent_function->return_array_size) + "]";
			}

			ControlFlowNode* flow = alloc_node<ControlFlowNode>();
			flow->flow_op = FLOW_OP_RETURN;

			pos = _get_tkpos();
			tk = _get_token();
			if (tk.type == TK_SEMICOLON) {
				// all is good
				if (b->parent_function->return_type != TYPE_VOID) {
					_set_error(
						vformat(RTR("Expected '%s' with an expression of type '%s'."), "return",
							(!return_struct_name.is_empty()
									? return_struct_name
									: get_datatype_name(b->parent_function->return_type)) +
								array_size_string));
					return ERR_PARSE_ERROR;
				}
			}
			else {
				if (b->parent_function->return_type == TYPE_VOID) {
					_set_error(vformat(RTR("'%s' function cannot return a value."), "void"));
					return ERR_PARSE_ERROR;
				}

				_set_tkpos(pos); // rollback, wants expression

#ifdef DEBUG_ENABLED
				if (b->parent_function->return_type == DataType::TYPE_BOOL) {
					keyword_completion_context = CF_BOOLEAN;
				}
#endif // DEBUG_ENABLED

				Node* expr = _parse_and_reduce_expression(p_block, p_function_info);
				if (!expr) {
					return ERR_PARSE_ERROR;
				}

				if (b->parent_function->return_type != expr->get_datatype() ||
					b->parent_function->return_array_size != expr->get_array_size() ||
					return_struct_name != expr->get_datatype_name()) {
					_set_error(vformat(RTR("Expected return with an expression of type '%s'."),
						(!return_struct_name.is_empty()
								? return_struct_name
								: get_datatype_name(b->parent_function->return_type)) +
							array_size_string));
					return ERR_PARSE_ERROR;
				}

				tk = _get_token();
				if (tk.type != TK_SEMICOLON) {
					_set_expected_after_error(";", "return");
					return ERR_PARSE_ERROR;
				}

#ifdef DEBUG_ENABLED
				if (b->parent_function->return_type == DataType::TYPE_BOOL) {
					keyword_completion_context = CF_BLOCK;
				}
#endif // DEBUG_ENABLED

				flow->expressions.push_back(expr);
			}

			p_block->statements.push_back(flow);

			BlockNode* block = p_block;
			while (block) {
				if (block->block_type == BlockNode::BLOCK_TYPE_CASE ||
					block->block_type == BlockNode::BLOCK_TYPE_DEFAULT) {
					return OK;
				}
				block = block->parent_block;
			}
		}
		else if (tk.type == TK_CF_DISCARD) {
			if (!is_discard_supported) {
				_set_error(vformat(RTR("Use of '%s' is not supported for the '%s' shader type."),
					"discard", shader_type_identifier));
				return ERR_PARSE_ERROR;
			}

			// check return type
			BlockNode* b = p_block;
			while (b && !b->parent_function) {
				b = b->parent_block;
			}
			if (!b) {
				_set_parsing_error();
				return ERR_BUG;
			}

			if (!b->parent_function->can_discard) {
				_set_error(vformat(RTR("'%s' cannot be used within the '%s' processor function."),
					"discard", b->parent_function->name));
				return ERR_PARSE_ERROR;
			}

			ControlFlowNode* flow = alloc_node<ControlFlowNode>();
			flow->flow_op = FLOW_OP_DISCARD;

			pos = _get_tkpos();
			tk = _get_token();

			calls_info[b->parent_function->name].uses_restricted_items.push_back(
				Pair<String, CallInfo::Item>(
					"discard", CallInfo::Item(CallInfo::Item::ITEM_TYPE_BUILTIN, pos)));

			if (tk.type != TK_SEMICOLON) {
				_set_expected_after_error(";", "discard");
				return ERR_PARSE_ERROR;
			}

			p_block->statements.push_back(flow);
		}
		else if (tk.type == TK_CF_BREAK) {
			if (!p_can_break) {
				_set_error(vformat(RTR("'%s' is not allowed outside of a loop or '%s' statement."),
					"break", "switch"));
				return ERR_PARSE_ERROR;
			}

			ControlFlowNode* flow = alloc_node<ControlFlowNode>();
			flow->flow_op = FLOW_OP_BREAK;

			pos = _get_tkpos();
			tk = _get_token();
			if (tk.type != TK_SEMICOLON) {
				_set_expected_after_error(";", "break");
				return ERR_PARSE_ERROR;
			}

			p_block->statements.push_back(flow);

			BlockNode* block = p_block;
			while (block) {
				if (block->block_type == BlockNode::BLOCK_TYPE_CASE ||
					block->block_type == BlockNode::BLOCK_TYPE_DEFAULT) {
					return OK;
				}
				block = block->parent_block;
			}

		}
		else if (tk.type == TK_CF_CONTINUE) {
			if (!p_can_continue) {
				_set_error(vformat(RTR("'%s' is not allowed outside of a loop."), "continue"));
				return ERR_PARSE_ERROR;
			}

			ControlFlowNode* flow = alloc_node<ControlFlowNode>();
			flow->flow_op = FLOW_OP_CONTINUE;

			pos = _get_tkpos();
			tk = _get_token();
			if (tk.type != TK_SEMICOLON) {
				// all is good
				_set_expected_after_error(";", "continue");
				return ERR_PARSE_ERROR;
			}

			p_block->statements.push_back(flow);

		}
		else {
			// nothing else, so expression
			_set_tkpos(pos); // rollback
			Node* expr = _parse_and_reduce_expression(p_block, p_function_info);
			if (!expr) {
				return ERR_PARSE_ERROR;
			}
			is_condition = expr->get_datatype() == TYPE_BOOL;

			if (expr->type == Node::NODE_TYPE_OPERATOR) {
				OperatorNode* op = static_cast<OperatorNode*>(expr);
				if (op->op == OP_EMPTY) {
					is_var_init = true;
					is_condition = true;
				}
			}

			p_block->statements.push_back(expr);
			tk = _get_token();

			if (p_block->block_type == BlockNode::BLOCK_TYPE_FOR_CONDITION) {
				if (tk.type == TK_COMMA) {
					if (!is_condition) {
						_set_error(
							RTR("The middle expression is expected to have a boolean data type."));
						return ERR_PARSE_ERROR;
					}
					tk = _peek();
					if (tk.type == TK_SEMICOLON) {
						_set_error(
							vformat(RTR("Expected expression, found: '%s'."), get_token_text(tk)));
						return ERR_PARSE_ERROR;
					}
					continue;
				}
				if (tk.type != TK_SEMICOLON) {
					_set_expected_error(",", ";");
					return ERR_PARSE_ERROR;
				}
			}
			else if (p_block->block_type == BlockNode::BLOCK_TYPE_FOR_EXPRESSION) {
				if (tk.type == TK_COMMA) {
					tk = _peek();
					if (tk.type == TK_PARENTHESIS_CLOSE) {
						_set_error(
							vformat(RTR("Expected expression, found: '%s'."), get_token_text(tk)));
						return ERR_PARSE_ERROR;
					}
					continue;
				}
				if (tk.type != TK_PARENTHESIS_CLOSE) {
					_set_expected_error(",", ")");
					return ERR_PARSE_ERROR;
				}
			}
			else if (tk.type != TK_SEMICOLON) {
				_set_expected_error(";");
				return ERR_PARSE_ERROR;
			}
		}

		if (p_block) {
			if (p_block->block_type == BlockNode::BLOCK_TYPE_FOR_INIT && !is_var_init) {
				_set_error(RTR("The left expression is expected to be a variable declaration."));
				return ERR_PARSE_ERROR;
			}
			if (p_block->block_type == BlockNode::BLOCK_TYPE_FOR_CONDITION && !is_condition) {
				_set_error(RTR("The middle expression is expected to have a boolean data type."));
				return ERR_PARSE_ERROR;
			}
		}

		if (p_just_one) {
			break;
		}
	}

	return OK;
}

String ShaderLanguage::_get_shader_type_list(const HashSet<String>& p_shader_types) const
{
	// Return a list of shader types as an human-readable string
	String valid_types;
	for (const String& E : p_shader_types) {
		if (!valid_types.is_empty()) {
			valid_types += ", ";
		}

		valid_types += "'" + E + "'";
	}

	return valid_types;
}

String ShaderLanguage::_get_qualifier_str(ArgumentQualifier p_qualifier) const
{
	switch (p_qualifier) {
	case ArgumentQualifier::ARGUMENT_QUALIFIER_IN:
		return "in";
	case ArgumentQualifier::ARGUMENT_QUALIFIER_OUT:
		return "out";
	case ArgumentQualifier::ARGUMENT_QUALIFIER_INOUT:
		return "inout";
	}
	return "";
}

Error ShaderLanguage::_validate_precision(DataType p_type, DataPrecision p_precision)
{
	switch (p_type) {
	case TYPE_STRUCT: {
		_set_error(RTR("The precision modifier cannot be used on structs."));
		return FAILED;
	} break;
	case TYPE_BOOL:
	case TYPE_BVEC2:
	case TYPE_BVEC3:
	case TYPE_BVEC4: {
		_set_error(RTR("The precision modifier cannot be used on boolean types."));
		return FAILED;
	} break;
	default:
		break;
	}
	return OK;
}

bool ShaderLanguage::_parse_numeric_constant_expression(
	const FunctionInfo& p_function_info, float& r_constant)
{
	ShaderLanguage::Node* expr = _parse_and_reduce_expression(nullptr, p_function_info);
	if (expr == nullptr) {
		return false;
	}

	Vector<Scalar> values;
	if (expr->type == Node::NODE_TYPE_VARIABLE) {
		_find_identifier(nullptr, false, p_function_info, static_cast<VariableNode*>(expr)->name,
			nullptr, nullptr, nullptr, nullptr, nullptr, &values);
	}
	else {
		values = expr->get_values();
	}

	if (values.is_empty()) {
		return false; // To prevent possible crash.
	}

	switch (expr->get_datatype()) {
	case TYPE_FLOAT:
		r_constant = values[0].real;
		break;
	case TYPE_INT:
		r_constant = static_cast<float>(values[0].sint);
		break;
	case TYPE_UINT:
		r_constant = static_cast<float>(values[0].uint);
		break;
	default:
		return false;
	}

	return true;
}

Error ShaderLanguage::_parse_shader(const HashMap<String, FunctionInfo>& p_functions,
	const Vector<ModeInfo>& p_render_modes, const Vector<ModeInfo>& p_stencil_modes,
	const HashSet<String>& p_shader_types)
{
	Token tk;
	TkPos prev_pos;
	Token next;

	if (!is_shader_inc) {
#ifdef DEBUG_ENABLED
		keyword_completion_context = CF_SHADER_TYPE;
#endif // DEBUG_ENABLED
		tk = _get_token();

		if (tk.type != TK_SHADER_TYPE) {
			_set_error(
				vformat(RTR("Expected '%s' at the beginning of shader. Valid types are: %s."),
					"shader_type", _get_shader_type_list(p_shader_types)));
			return ERR_PARSE_ERROR;
		}
#ifdef DEBUG_ENABLED
		keyword_completion_context = CF_UNSPECIFIED;
#endif // DEBUG_ENABLED

		_get_completable_identifier(nullptr, COMPLETION_SHADER_TYPE, shader_type_identifier);
		if (shader_type_identifier == String()) {
			_set_error(vformat(RTR("Expected an identifier after '%s', indicating the type of "
								   "shader. Valid types are: %s."),
				"shader_type", _get_shader_type_list(p_shader_types)));
			return ERR_PARSE_ERROR;
		}
		if (!p_shader_types.has(shader_type_identifier)) {
			_set_error(vformat(RTR("Invalid shader type. Valid types are: %s"),
				_get_shader_type_list(p_shader_types)));
			return ERR_PARSE_ERROR;
		}
		prev_pos = _get_tkpos();
		tk = _get_token();

		if (tk.type != TK_SEMICOLON) {
			_set_tkpos(prev_pos);
			_set_expected_after_error(";", "shader_type " + String(shader_type_identifier));
			return ERR_PARSE_ERROR;
		}
	}

#ifdef DEBUG_ENABLED
	keyword_completion_context = CF_GLOBAL_SPACE;
#endif // DEBUG_ENABLED
	tk = _get_token();

	int texture_uniforms = 0;
	int texture_binding = 0;
	int uniforms = 0;
	int instance_index = 0;
	int prop_index = 0;
#ifdef DEBUG_ENABLED
	uint64_t uniform_buffer_size = 0;
	uint64_t max_uniform_buffer_size = 65536;
	int uniform_buffer_exceeded_line = -1;
	bool check_device_limit_warnings =
		check_warnings && HAS_WARNING(ShaderWarning::DEVICE_LIMIT_EXCEEDED_FLAG);
#endif // DEBUG_ENABLED
	ShaderNode::Uniform::Scope uniform_scope = ShaderNode::Uniform::SCOPE_LOCAL;

	stages = &p_functions;

	is_discard_supported = shader_type_identifier == "canvas_item" ||
						   shader_type_identifier == "spatial" ||
						   shader_type_identifier == String();
	is_supported_frag_only_funcs = is_discard_supported || shader_type_identifier == "sky" ||
								   shader_type_identifier == String();

	const FunctionInfo& constants =
		p_functions.has("constants") ? p_functions["constants"] : FunctionInfo();

	HashMap<String, String> defined_render_modes;
	HashMap<String, String> defined_stencil_modes;

	while (tk.type != TK_EOF) {
		switch (tk.type) {
		case TK_RENDER_MODE: {
#ifdef DEBUG_ENABLED
			keyword_completion_context = CF_UNSPECIFIED;
#endif // DEBUG_ENABLED
			while (true) {
				Error error = _parse_shader_mode(false, p_render_modes, defined_render_modes);
				if (error != OK) {
					return error;
				}

				tk = _get_token();

				if (tk.type == TK_COMMA) {
					// All good, do nothing.
				}
				else if (tk.type == TK_SEMICOLON) {
					break; // Done.
				}
				else {
					_set_error(vformat(RTR("Unexpected token: '%s'."), get_token_text(tk)));
					return ERR_PARSE_ERROR;
				}
			}
#ifdef DEBUG_ENABLED
			keyword_completion_context = CF_GLOBAL_SPACE;
#endif // DEBUG_ENABLED
		} break;
		case TK_STENCIL_MODE: {
#ifdef DEBUG_ENABLED
			keyword_completion_context = CF_UNSPECIFIED;
#endif // DEBUG_ENABLED
			while (true) {
				TkPos pos = _get_tkpos();
				tk = _get_token();

				if (tk.is_integer_constant()) {
					const int reference_value = tk.constant;

					if (shader->stencil_reference != -1) {
						_set_error(vformat(RTR("Duplicated stencil mode reference value: '%s'."),
							reference_value));
						return ERR_PARSE_ERROR;
					}

					if (reference_value < 0) {
						_set_error(
							vformat(RTR("Stencil mode reference value cannot be negative: '%s'."),
								reference_value));
						return ERR_PARSE_ERROR;
					}

					if (reference_value > 255) {
						_set_error(vformat(
							RTR("Stencil mode reference value cannot be greater than 255: '%s'."),
							reference_value));
						return ERR_PARSE_ERROR;
					}

					shader->stencil_reference = reference_value;
				}
				else {
					_set_tkpos(pos);

					Error error = _parse_shader_mode(true, p_stencil_modes, defined_stencil_modes);
					if (error != OK) {
						return error;
					}
				}

				tk = _get_token();

				if (tk.type == TK_COMMA) {
					// all good, do nothing
				}
				else if (tk.type == TK_SEMICOLON) {
					break; // done
				}
				else {
					_set_error(vformat(RTR("Unexpected token: '%s'."), get_token_text(tk)));
					return ERR_PARSE_ERROR;
				}
			}
#ifdef DEBUG_ENABLED
			keyword_completion_context = CF_GLOBAL_SPACE;
#endif // DEBUG_ENABLED
		} break;
		case TK_STRUCT: {
			ShaderNode::Struct st;
			DataType type;
#ifdef DEBUG_ENABLED
			keyword_completion_context = CF_UNSPECIFIED;
#endif // DEBUG_ENABLED
			tk = _get_token();
			if (tk.type == TK_IDENTIFIER) {
				st.name = tk.text;
				if (shader->constants.has(st.name) || shader->structs.has(st.name)) {
					_set_redefinition_error(String(st.name));
					return ERR_PARSE_ERROR;
				}
				tk = _get_token();
				if (tk.type != TK_CURLY_BRACKET_OPEN) {
					_set_expected_error("{");
					return ERR_PARSE_ERROR;
				}
			}
			else {
				_set_error(RTR("Expected a struct identifier."));
				return ERR_PARSE_ERROR;
			}

			StructNode* st_node = alloc_node<StructNode>();
			st.shader_struct = st_node;

			int member_count = 0;
			HashSet<String> member_names;

			while (true) { // variables list
#ifdef DEBUG_ENABLED
				keyword_completion_context = CF_DATATYPE | CF_PRECISION_MODIFIER;
#endif // DEBUG_ENABLED

				tk = _get_token();
				if (tk.type == TK_CURLY_BRACKET_CLOSE) {
					break;
				}
				String struct_name = "";
				bool struct_dt = false;
				DataPrecision precision = PRECISION_DEFAULT;

				if (tk.type == TK_STRUCT) {
					_set_error(RTR("Nested structs are not allowed."));
					return ERR_PARSE_ERROR;
				}

				if (is_token_precision(tk.type)) {
					precision = get_token_precision(tk.type);
					tk = _get_token();
#ifdef DEBUG_ENABLED
					keyword_completion_context ^= CF_PRECISION_MODIFIER;
#endif // DEBUG_ENABLED
				}

				if (shader->structs.has(tk.text)) {
					struct_name = tk.text;
#ifdef DEBUG_ENABLED
					if (check_warnings && HAS_WARNING(ShaderWarning::UNUSED_STRUCT_FLAG) &&
						used_structs.has(struct_name)) {
						used_structs[struct_name].used = true;
					}
#endif // DEBUG_ENABLED
					struct_dt = true;
				}

				if (!is_token_datatype(tk.type) && !struct_dt) {
					_set_error(RTR("Expected data type."));
					return ERR_PARSE_ERROR;
				}
				else {
					type = struct_dt ? TYPE_STRUCT : get_token_datatype(tk.type);

					if (precision != PRECISION_DEFAULT &&
						_validate_precision(type, precision) != OK) {
						return ERR_PARSE_ERROR;
					}

					if (type == TYPE_VOID || is_sampler_type(type)) {
						_set_error(vformat(
							RTR("A '%s' data type is not allowed here."), get_datatype_name(type)));
						return ERR_PARSE_ERROR;
					}
#ifdef DEBUG_ENABLED
					keyword_completion_context = CF_UNSPECIFIED;
#endif // DEBUG_ENABLED

					bool first = true;
					bool fixed_array_size = false;
					int array_size = 0;

					do {
						tk = _get_token();

						if (first) {
							first = false;

							if (tk.type != TK_IDENTIFIER && tk.type != TK_BRACKET_OPEN) {
								_set_error(RTR("Expected an identifier or '['."));
								return ERR_PARSE_ERROR;
							}

							if (tk.type == TK_BRACKET_OPEN) {
								Error error = _parse_array_size(
									nullptr, constants, true, nullptr, &array_size, nullptr);
								if (error != OK) {
									return error;
								}
								fixed_array_size = true;
								tk = _get_token();
							}
						}

						if (tk.type != TK_IDENTIFIER) {
							_set_error(RTR("Expected an identifier."));
							return ERR_PARSE_ERROR;
						}

						MemberNode* member = alloc_node<MemberNode>();
						member->precision = precision;
						member->datatype = type;
						member->struct_name = struct_name;
						member->name = tk.text;
						member->array_size = array_size;

						if (member_names.has(member->name)) {
							_set_redefinition_error(String(member->name));
							return ERR_PARSE_ERROR;
						}
						member_names.insert(member->name);
						tk = _get_token();

						if (tk.type == TK_BRACKET_OPEN) {
							Error error = _parse_array_size(
								nullptr, constants, true, nullptr, &member->array_size, nullptr);
							if (error != OK) {
								return error;
							}
							tk = _get_token();
						}

						if (!fixed_array_size) {
							array_size = 0;
						}

						if (tk.type != TK_SEMICOLON && tk.type != TK_COMMA) {
							_set_expected_error(",", ";");
							return ERR_PARSE_ERROR;
						}

						st_node->members.push_back(member);
						member_count++;
					} while (tk.type == TK_COMMA); // another member
				}
			}
			if (member_count == 0) {
				_set_error(RTR("Empty structs are not allowed."));
				return ERR_PARSE_ERROR;
			}
#ifdef DEBUG_ENABLED
			keyword_completion_context = CF_UNSPECIFIED;
#endif // DEBUG_ENABLED

			tk = _get_token();
			if (tk.type != TK_SEMICOLON) {
				_set_expected_error(";");
				return ERR_PARSE_ERROR;
			}
#ifdef DEBUG_ENABLED
			keyword_completion_context = CF_GLOBAL_SPACE;
#endif // DEBUG_ENABLED

			shader->structs[st.name] = st;
			shader->vstructs.push_back(st); // struct's order is important!
#ifdef DEBUG_ENABLED
			if (check_warnings && HAS_WARNING(ShaderWarning::UNUSED_STRUCT_FLAG)) {
				used_structs.insert(st.name, Usage(tk_line));
			}
#endif // DEBUG_ENABLED
		} break;
		case TK_GLOBAL: {
#ifdef DEBUG_ENABLED
			keyword_completion_context = CF_UNIFORM_KEYWORD;
			if (_lookup_next(next)) {
				if (next.type == TK_UNIFORM) {
					keyword_completion_context ^= CF_UNIFORM_KEYWORD;
				}
			}
#endif // DEBUG_ENABLED
			tk = _get_token();
			if (tk.type != TK_UNIFORM) {
				_set_expected_after_error("uniform", "global");
				return ERR_PARSE_ERROR;
			}
			uniform_scope = ShaderNode::Uniform::SCOPE_GLOBAL;
		};
			[[fallthrough]];
		case TK_INSTANCE: {
			if (tk.type == TK_INSTANCE) {
#ifdef DEBUG_ENABLED
				keyword_completion_context = CF_UNIFORM_KEYWORD;
				if (_lookup_next(next)) {
					if (next.type == TK_UNIFORM) {
						keyword_completion_context ^= CF_UNIFORM_KEYWORD;
					}
				}
#endif // DEBUG_ENABLED
				if (shader_type_identifier != String() &&
					String(shader_type_identifier) != "spatial" &&
					String(shader_type_identifier) != "canvas_item") {
					_set_error(
						vformat(RTR("Uniform instances are not yet implemented for '%s' shaders."),
							shader_type_identifier));
					return ERR_PARSE_ERROR;
				}
				if (uniform_scope == ShaderNode::Uniform::SCOPE_LOCAL) {
					tk = _get_token();
					if (tk.type != TK_UNIFORM) {
						_set_expected_after_error("uniform", "instance");
						return ERR_PARSE_ERROR;
					}
					uniform_scope = ShaderNode::Uniform::SCOPE_INSTANCE;
				}
			}
		};
			[[fallthrough]];
		case TK_UNIFORM:
		case TK_VARYING: {
			bool is_uniform = tk.type == TK_UNIFORM;
#ifdef DEBUG_ENABLED
			keyword_completion_context = CF_UNSPECIFIED;
#endif // DEBUG_ENABLED
			if (!is_uniform) {
				if (shader_type_identifier == "particles" || shader_type_identifier == "sky" ||
					shader_type_identifier == "fog") {
					_set_error(vformat(
						RTR("Varyings cannot be used in '%s' shaders."), shader_type_identifier));
					return ERR_PARSE_ERROR;
				}
			}
			DataPrecision precision = PRECISION_DEFAULT;
			DataInterpolation interpolation = INTERPOLATION_DEFAULT;
			DataType type;
			String name;
			int array_size = 0;

			tk = _get_token();
#ifdef DEBUG_ENABLED
			bool temp_error = false;
			uint32_t datatype_flag;

			if (!is_uniform) {
				datatype_flag = CF_VARYING_TYPE;
				keyword_completion_context =
					CF_INTERPOLATION_QUALIFIER | CF_PRECISION_MODIFIER | datatype_flag;

				if (_lookup_next(next)) {
					if (is_token_interpolation(next.type)) {
						keyword_completion_context ^= (CF_INTERPOLATION_QUALIFIER | datatype_flag);
					}
					else if (is_token_precision(next.type)) {
						keyword_completion_context ^= (CF_PRECISION_MODIFIER | datatype_flag);
					}
					else if (is_token_datatype(next.type)) {
						keyword_completion_context ^= datatype_flag;
					}
				}
			}
			else {
				datatype_flag = CF_UNIFORM_TYPE;
				keyword_completion_context = CF_PRECISION_MODIFIER | datatype_flag;

				if (_lookup_next(next)) {
					if (is_token_precision(next.type)) {
						keyword_completion_context ^= (CF_PRECISION_MODIFIER | datatype_flag);
					}
					else if (is_token_datatype(next.type)) {
						keyword_completion_context ^= datatype_flag;
					}
				}
			}
#endif // DEBUG_ENABLED

			if (is_token_interpolation(tk.type)) {
				if (is_uniform) {
					_set_error(RTR("Interpolation qualifiers are not supported for uniforms."));
#ifdef DEBUG_ENABLED
					temp_error = true;
#else
					return ERR_PARSE_ERROR;
#endif // DEBUG_ENABLED
				}
				interpolation = get_token_interpolation(tk.type);
				tk = _get_token();
#ifdef DEBUG_ENABLED
				if (keyword_completion_context & CF_INTERPOLATION_QUALIFIER) {
					keyword_completion_context ^= CF_INTERPOLATION_QUALIFIER;
				}
				if (_lookup_next(next)) {
					if (is_token_precision(next.type)) {
						keyword_completion_context ^= CF_PRECISION_MODIFIER;
					}
					else if (is_token_datatype(next.type)) {
						keyword_completion_context ^= datatype_flag;
					}
				}
				if (temp_error) {
					return ERR_PARSE_ERROR;
				}
#endif // DEBUG_ENABLED
			}

			if (is_token_precision(tk.type)) {
				precision = get_token_precision(tk.type);
				tk = _get_token();
#ifdef DEBUG_ENABLED
				if (keyword_completion_context & CF_INTERPOLATION_QUALIFIER) {
					keyword_completion_context ^= CF_INTERPOLATION_QUALIFIER;
				}
				if (keyword_completion_context & CF_PRECISION_MODIFIER) {
					keyword_completion_context ^= CF_PRECISION_MODIFIER;
				}
				if (_lookup_next(next)) {
					if (is_token_datatype(next.type)) {
						keyword_completion_context = CF_UNSPECIFIED;
					}
				}
#endif // DEBUG_ENABLED
			}

			if (shader->structs.has(tk.text)) {
				if (is_uniform) {
					_set_error(vformat(
						RTR("The '%s' data type is not supported for uniforms."), "struct"));
					return ERR_PARSE_ERROR;
				}
				else {
					_set_error(vformat(RTR("The '%s' data type is not allowed here."), "struct"));
					return ERR_PARSE_ERROR;
				}
			}

			if (!is_token_datatype(tk.type)) {
				_set_error(RTR("Expected data type."));
				return ERR_PARSE_ERROR;
			}

			type = get_token_datatype(tk.type);

			if (precision != PRECISION_DEFAULT && _validate_precision(type, precision) != OK) {
				return ERR_PARSE_ERROR;
			}

			if (type == TYPE_VOID) {
				_set_error(vformat(RTR("The '%s' data type is not allowed here."), "void"));
				return ERR_PARSE_ERROR;
			}

			if (!is_uniform && interpolation != INTERPOLATION_DEFAULT && type < TYPE_INT) {
				_set_error(
					vformat(RTR("Interpolation modifier '%s' cannot be used with boolean types."),
						get_interpolation_name(interpolation)));
				return ERR_PARSE_ERROR;
			}

			if (!is_uniform && type > TYPE_MAT4) {
				_set_error(RTR("Invalid data type for varying."));
				return ERR_PARSE_ERROR;
			}

#ifdef DEBUG_ENABLED
			keyword_completion_context = CF_UNSPECIFIED;
#endif // DEBUG_ENABLED
			tk = _get_token();

			if (tk.type != TK_IDENTIFIER && tk.type != TK_BRACKET_OPEN) {
				_set_error(RTR("Expected an identifier or '['."));
				return ERR_PARSE_ERROR;
			}

			if (tk.type == TK_BRACKET_OPEN) {
				Error error =
					_parse_array_size(nullptr, constants, true, nullptr, &array_size, nullptr);
				if (error != OK) {
					return error;
				}
				tk = _get_token();
			}

			if (tk.type != TK_IDENTIFIER) {
				_set_error(RTR("Expected an identifier."));
				return ERR_PARSE_ERROR;
			}

			prev_pos = _get_tkpos();
			name = tk.text;

			if (_find_identifier(nullptr, false, constants, name)) {
				_set_redefinition_error(String(name));
				return ERR_PARSE_ERROR;
			}

			if (has_builtin(p_functions, name)) {
				_set_redefinition_error(String(name));
				return ERR_PARSE_ERROR;
			}

			if (is_uniform) {
				if (uniform_scope == ShaderNode::Uniform::SCOPE_GLOBAL &&
					Engine::is_editor_hint()) { // Type checking for global uniforms is not allowed
												// outside the editor.
					// validate global uniform
					DataType gvtype = global_shader_uniform_get_type_func(name);
					if (gvtype == TYPE_MAX) {
						_set_error(vformat(RTR("Global uniform '%s' does not exist. Create it in "
											   "Project Settings."),
							String(name)));
						return ERR_PARSE_ERROR;
					}

					if (type != gvtype) {
						_set_error(vformat(RTR("Global uniform '%s' must be of type '%s'."),
							String(name), get_datatype_name(gvtype)));
						return ERR_PARSE_ERROR;
					}
				}
				ShaderNode::Uniform uniform;

				uniform.type = type;
				uniform.scope = uniform_scope;
				uniform.precision = precision;
				uniform.array_size = array_size;
				uniform.group = current_uniform_group_name;

				tk = _get_token();
				if (tk.type == TK_BRACKET_OPEN) {
					Error error = _parse_array_size(
						nullptr, constants, true, nullptr, &uniform.array_size, nullptr);
					if (error != OK) {
						return error;
					}
					tk = _get_token();
				}

				if (is_sampler_type(type)) {
					if (uniform_scope == ShaderNode::Uniform::SCOPE_INSTANCE) {
						_set_error(
							vformat(RTR("The '%s' qualifier is not supported for sampler types."),
								"SCOPE_INSTANCE"));
						return ERR_PARSE_ERROR;
					}
					uniform.texture_order = texture_uniforms++;
					uniform.texture_binding = texture_binding;
					if (uniform.array_size > 0) {
						texture_binding += uniform.array_size;
					}
					else {
						++texture_binding;
					}
					uniform.order = -1;
					uniform.prop_order = prop_index++;
				}
				else {
					if (uniform_scope == ShaderNode::Uniform::SCOPE_INSTANCE &&
						(type == TYPE_MAT2 || type == TYPE_MAT3 || type == TYPE_MAT4)) {
						_set_error(
							vformat(RTR("The '%s' qualifier is not supported for matrix types."),
								"SCOPE_INSTANCE"));
						return ERR_PARSE_ERROR;
					}
					uniform.texture_order = -1;
					if (uniform_scope != ShaderNode::Uniform::SCOPE_INSTANCE) {
						uniform.order = uniforms++;
						uniform.prop_order = prop_index++;
#ifdef DEBUG_ENABLED
						if (check_device_limit_warnings) {
							if (uniform.array_size > 0) {
								int size = get_datatype_size(uniform.type) * uniform.array_size;
								int m = (16 * uniform.array_size);
								if ((size % m) != 0U) {
									size += m - (size % m);
								}
								uniform_buffer_size += size;
							}
							else {
								uniform_buffer_size += get_datatype_size(uniform.type);
							}

							if (uniform_buffer_exceeded_line == -1 &&
								uniform_buffer_size > max_uniform_buffer_size) {
								uniform_buffer_exceeded_line = tk_line;
							}
						}
#endif // DEBUG_ENABLED
					}
				}

				if (uniform.array_size > 0) {
					if (uniform_scope == ShaderNode::Uniform::SCOPE_GLOBAL) {
						_set_error(
							vformat(RTR("The '%s' qualifier is not supported for uniform arrays."),
								"SCOPE_GLOBAL"));
						return ERR_PARSE_ERROR;
					}
					if (uniform_scope == ShaderNode::Uniform::SCOPE_INSTANCE) {
						_set_error(
							vformat(RTR("The '%s' qualifier is not supported for uniform arrays."),
								"SCOPE_INSTANCE"));
						return ERR_PARSE_ERROR;
					}
				}

				int custom_instance_index = -1;

				if (tk.type == TK_COLON) {
					completion_type = COMPLETION_HINT;
					completion_base = type;
					completion_base_array = uniform.array_size > 0;

					// hint
					do {
						tk = _get_token();
						completion_line = tk.line;

						if (!is_token_hint(tk.type)) {
							_set_error(RTR("Expected valid type hint after ':'."));
							return ERR_PARSE_ERROR;
						}

						if (uniform.array_size > 0) {
							static Vector<int> supported_hints = {TK_HINT_SOURCE_COLOR,
								TK_HINT_COLOR_CONVERSION_DISABLED, TK_REPEAT_DISABLE,
								TK_REPEAT_ENABLE, TK_FILTER_LINEAR, TK_FILTER_LINEAR_MIPMAP,
								TK_FILTER_LINEAR_MIPMAP_ANISOTROPIC, TK_FILTER_NEAREST,
								TK_FILTER_NEAREST_MIPMAP, TK_FILTER_NEAREST_MIPMAP_ANISOTROPIC};
							if (!supported_hints.has(tk.type)) {
								_set_error(RTR("This hint is not supported for uniform arrays."));
								return ERR_PARSE_ERROR;
							}
						}

						ShaderNode::Uniform::Hint new_hint = ShaderNode::Uniform::HINT_NONE;
						TextureFilter new_filter = FILTER_DEFAULT;
						TextureRepeat new_repeat = REPEAT_DEFAULT;

						switch (tk.type) {
						case TK_HINT_SOURCE_COLOR: {
							if (type != TYPE_VEC3 && type != TYPE_VEC4 && !is_sampler_type(type)) {
								_set_error(vformat(RTR("Source color hint is for '%s', '%s' or "
													   "sampler types only."),
									"vec3", "vec4"));
								return ERR_PARSE_ERROR;
							}

							if (is_sampler_type(type)) {
								if (uniform.use_color) {
									_set_error(
										vformat(RTR("Duplicated hint: '%s'."), "source_color"));
									return ERR_PARSE_ERROR;
								}
								uniform.use_color = true;
							}
							else {
								new_hint = ShaderNode::Uniform::HINT_SOURCE_COLOR;
							}
						} break;
						case TK_HINT_COLOR_CONVERSION_DISABLED: {
							if (type != TYPE_VEC3 && type != TYPE_VEC4) {
								_set_error(vformat(
									RTR("Source color conversion disabled hint is for '%s', '%s'."),
									"vec3", "vec4"));
								return ERR_PARSE_ERROR;
							}
							if (uniform.hint != ShaderNode::Uniform::HINT_SOURCE_COLOR) {
								_set_error(vformat(RTR("Hint '%s' should be preceded by '%s'."),
									"color_conversion_disabled", "source_color"));
								return ERR_PARSE_ERROR;
							}
							uniform.hint = ShaderNode::Uniform::HINT_NONE;
							new_hint = ShaderNode::Uniform::HINT_COLOR_CONVERSION_DISABLED;
						} break;
						case TK_HINT_DEFAULT_BLACK_TEXTURE: {
							new_hint = ShaderNode::Uniform::HINT_DEFAULT_BLACK;
						} break;
						case TK_HINT_DEFAULT_WHITE_TEXTURE: {
							new_hint = ShaderNode::Uniform::HINT_DEFAULT_WHITE;
						} break;
						case TK_HINT_DEFAULT_TRANSPARENT_TEXTURE: {
							new_hint = ShaderNode::Uniform::HINT_DEFAULT_TRANSPARENT;
						} break;
						case TK_HINT_NORMAL_TEXTURE: {
							new_hint = ShaderNode::Uniform::HINT_NORMAL;
						} break;
						case TK_HINT_ROUGHNESS_NORMAL_TEXTURE: {
							new_hint = ShaderNode::Uniform::HINT_ROUGHNESS_NORMAL;
						} break;
						case TK_HINT_ROUGHNESS_R: {
							new_hint = ShaderNode::Uniform::HINT_ROUGHNESS_R;
						} break;
						case TK_HINT_ROUGHNESS_G: {
							new_hint = ShaderNode::Uniform::HINT_ROUGHNESS_G;
						} break;
						case TK_HINT_ROUGHNESS_B: {
							new_hint = ShaderNode::Uniform::HINT_ROUGHNESS_B;
						} break;
						case TK_HINT_ROUGHNESS_A: {
							new_hint = ShaderNode::Uniform::HINT_ROUGHNESS_A;
						} break;
						case TK_HINT_ROUGHNESS_GRAY: {
							new_hint = ShaderNode::Uniform::HINT_ROUGHNESS_GRAY;
						} break;
						case TK_HINT_ANISOTROPY_TEXTURE: {
							new_hint = ShaderNode::Uniform::HINT_ANISOTROPY;
						} break;
						case TK_HINT_RANGE: {
							if (type != TYPE_FLOAT && type != TYPE_INT) {
								_set_error(vformat(
									RTR("Range hint is for '%s' and '%s' only."), "float", "int"));
								return ERR_PARSE_ERROR;
							}

							tk = _get_token();
							if (tk.type != TK_PARENTHESIS_OPEN) {
								_set_expected_after_error("(", "hint_range");
								return ERR_PARSE_ERROR;
							}

							if (!_parse_numeric_constant_expression(
									constants, uniform.hint_range[0])) {
								_set_error(RTR("Expected a valid numeric expression."));
								return ERR_PARSE_ERROR;
							}

							tk = _get_token();

							if (tk.type != TK_COMMA) {
								_set_expected_error(",");
								return ERR_PARSE_ERROR;
							}

							if (!_parse_numeric_constant_expression(
									constants, uniform.hint_range[1])) {
								_set_error(RTR("Expected a valid numeric expression after ','."));
								return ERR_PARSE_ERROR;
							}

							tk = _get_token();

							if (tk.type == TK_COMMA) {
								if (!_parse_numeric_constant_expression(
										constants, uniform.hint_range[2])) {
									_set_error(
										RTR("Expected a valid numeric expression after ','."));
									return ERR_PARSE_ERROR;
								}
								tk = _get_token();
							}
							else {
								if (type == TYPE_INT) {
									uniform.hint_range[2] = 1;
								}
								else {
									uniform.hint_range[2] = 0.001;
								}
							}

							if (tk.type != TK_PARENTHESIS_CLOSE) {
								_set_expected_error(")");
								return ERR_PARSE_ERROR;
							}

							new_hint = ShaderNode::Uniform::HINT_RANGE;
						} break;
						case TK_HINT_ENUM: {
							if (type != TYPE_INT) {
								_set_error(vformat(RTR("Enum hint is for '%s' only."), "int"));
								return ERR_PARSE_ERROR;
							}

							tk = _get_token();
							if (tk.type != TK_PARENTHESIS_OPEN) {
								_set_expected_after_error("(", "hint_enum");
								return ERR_PARSE_ERROR;
							}

							while (true) {
								tk = _get_token();

								if (tk.type != TK_STRING_CONSTANT) {
									_set_error(RTR("Expected a string constant."));
									return ERR_PARSE_ERROR;
								}

								uniform.hint_enum_names.push_back(tk.text);

								tk = _get_token();

								if (tk.type == TK_PARENTHESIS_CLOSE) {
									break;
								}
								else if (tk.type != TK_COMMA) {
									_set_error(RTR("Expected ',' or ')' after string constant."));
									return ERR_PARSE_ERROR;
								}
							}

							new_hint = ShaderNode::Uniform::HINT_ENUM;
						} break;
						case TK_HINT_INSTANCE_INDEX: {
							if (custom_instance_index != -1) {
								_set_error(
									vformat(RTR("Can only specify '%s' once."), "instance_index"));
								return ERR_PARSE_ERROR;
							}

							tk = _get_token();
							if (tk.type != TK_PARENTHESIS_OPEN) {
								_set_expected_after_error("(", "instance_index");
								return ERR_PARSE_ERROR;
							}

							tk = _get_token();

							if (tk.type == TK_OP_SUB) {
								_set_error(RTR("The instance index can't be negative."));
								return ERR_PARSE_ERROR;
							}

							if (!tk.is_integer_constant()) {
								_set_error(RTR("Expected an integer constant."));
								return ERR_PARSE_ERROR;
							}

							custom_instance_index = tk.constant;
							current_uniform_instance_index_defined = true;

							if (custom_instance_index >= MAX_INSTANCE_UNIFORM_INDICES) {
								_set_error(vformat(RTR("Allowed instance uniform indices must be "
													   "within [0..%d] range."),
									MAX_INSTANCE_UNIFORM_INDICES - 1));
								return ERR_PARSE_ERROR;
							}

							tk = _get_token();

							if (tk.type != TK_PARENTHESIS_CLOSE) {
								_set_expected_error(")");
								return ERR_PARSE_ERROR;
							}
						} break;
						case TK_HINT_SCREEN_TEXTURE: {
							new_hint = ShaderNode::Uniform::HINT_SCREEN_TEXTURE;
							--texture_uniforms;
							--texture_binding;
							if (shader_type_identifier != String() &&
								!(String(shader_type_identifier) == "spatial" ||
									String(shader_type_identifier) == "canvas_item")) {
								_set_error(vformat(
									RTR("'hint_screen_texture' is not supported in '%s' shaders."),
									shader_type_identifier));
								return ERR_PARSE_ERROR;
							}
						} break;
						case TK_HINT_NORMAL_ROUGHNESS_TEXTURE: {
							new_hint = ShaderNode::Uniform::HINT_NORMAL_ROUGHNESS_TEXTURE;
							--texture_uniforms;
							--texture_binding;
							if (OS::get_current_rendering_method() !=
								"forward_plus") {
								_set_error(RTR("'hint_normal_roughness_texture' is only available "
											   "when using the Forward+ renderer."));
								return ERR_PARSE_ERROR;
							}
							if (shader_type_identifier != String() &&
								String(shader_type_identifier) != "spatial") {
								_set_error(vformat(RTR("'hint_normal_roughness_texture' is not "
													   "supported in '%s' shaders."),
									shader_type_identifier));
								return ERR_PARSE_ERROR;
							}
						} break;
						case TK_HINT_DEPTH_TEXTURE: {
							new_hint = ShaderNode::Uniform::HINT_DEPTH_TEXTURE;
							--texture_uniforms;
							--texture_binding;
							if (shader_type_identifier != String() &&
								String(shader_type_identifier) != "spatial") {
								_set_error(vformat(
									RTR("'hint_depth_texture' is not supported in '%s' shaders."),
									shader_type_identifier));
								return ERR_PARSE_ERROR;
							}
						} break;
						case TK_HINT_BLIT_SOURCE0: {
							new_hint = ShaderNode::Uniform::HINT_BLIT_SOURCE0;
							--texture_uniforms;
							--texture_binding;
							if (shader_type_identifier != String() &&
								String(shader_type_identifier) != "texture_blit") {
								_set_error(vformat(
									RTR("'hint_blit_source' is not supported in '%s' shaders."),
									shader_type_identifier));
								return ERR_PARSE_ERROR;
							}
						} break;
						case TK_HINT_BLIT_SOURCE1: {
							new_hint = ShaderNode::Uniform::HINT_BLIT_SOURCE1;
							--texture_uniforms;
							--texture_binding;
							if (shader_type_identifier != String() &&
								String(shader_type_identifier) != "texture_blit") {
								_set_error(vformat(
									RTR("'hint_blit_source' is not supported in '%s' shaders."),
									shader_type_identifier));
								return ERR_PARSE_ERROR;
							}
						} break;
						case TK_HINT_BLIT_SOURCE2: {
							new_hint = ShaderNode::Uniform::HINT_BLIT_SOURCE2;
							--texture_uniforms;
							--texture_binding;
							if (shader_type_identifier != String() &&
								String(shader_type_identifier) != "texture_blit") {
								_set_error(vformat(
									RTR("'hint_blit_source' is not supported in '%s' shaders."),
									shader_type_identifier));
								return ERR_PARSE_ERROR;
							}
						} break;
						case TK_HINT_BLIT_SOURCE3: {
							new_hint = ShaderNode::Uniform::HINT_BLIT_SOURCE3;
							--texture_uniforms;
							--texture_binding;
							if (shader_type_identifier != String() &&
								String(shader_type_identifier) != "texture_blit") {
								_set_error(vformat(
									RTR("'hint_blit_source' is not supported in '%s' shaders."),
									shader_type_identifier));
								return ERR_PARSE_ERROR;
							}
						} break;
						case TK_FILTER_NEAREST: {
							new_filter = FILTER_NEAREST;
						} break;
						case TK_FILTER_LINEAR: {
							new_filter = FILTER_LINEAR;
						} break;
						case TK_FILTER_NEAREST_MIPMAP: {
							new_filter = FILTER_NEAREST_MIPMAP;
						} break;
						case TK_FILTER_LINEAR_MIPMAP: {
							new_filter = FILTER_LINEAR_MIPMAP;
						} break;
						case TK_FILTER_NEAREST_MIPMAP_ANISOTROPIC: {
							new_filter = FILTER_NEAREST_MIPMAP_ANISOTROPIC;
						} break;
						case TK_FILTER_LINEAR_MIPMAP_ANISOTROPIC: {
							new_filter = FILTER_LINEAR_MIPMAP_ANISOTROPIC;
						} break;
						case TK_REPEAT_DISABLE: {
							new_repeat = REPEAT_DISABLE;
						} break;
						case TK_REPEAT_ENABLE: {
							new_repeat = REPEAT_ENABLE;
						} break;

						default:
							break;
						}

						bool is_sampler_hint =
							new_hint != ShaderNode::Uniform::HINT_NONE &&
							new_hint != ShaderNode::Uniform::HINT_SOURCE_COLOR &&
							new_hint != ShaderNode::Uniform::HINT_COLOR_CONVERSION_DISABLED &&
							new_hint != ShaderNode::Uniform::HINT_RANGE &&
							new_hint != ShaderNode::Uniform::HINT_ENUM;
						if (((new_filter != FILTER_DEFAULT || new_repeat != REPEAT_DEFAULT) ||
								is_sampler_hint) &&
							!is_sampler_type(type)) {
							_set_error(RTR("This hint is only for sampler types."));
							return ERR_PARSE_ERROR;
						}

						if (new_hint != ShaderNode::Uniform::HINT_NONE) {
							if (uniform.hint != ShaderNode::Uniform::HINT_NONE) {
								if (uniform.hint == new_hint) {
									_set_error(vformat(RTR("Duplicated hint: '%s'."),
										get_uniform_hint_name(new_hint)));
								}
								else {
									_set_error(vformat(RTR("Redefinition of hint: '%s'. The hint "
														   "has already been set to '%s'."),
										get_uniform_hint_name(new_hint),
										get_uniform_hint_name(uniform.hint)));
								}
								return ERR_PARSE_ERROR;
							}
							else {
								uniform.hint = new_hint;
								current_uniform_hint = new_hint;
							}
						}

						if (new_filter != FILTER_DEFAULT) {
							if (uniform.filter != FILTER_DEFAULT) {
								if (uniform.filter == new_filter) {
									_set_error(vformat(RTR("Duplicated filter mode: '%s'."),
										get_texture_filter_name(new_filter)));
								}
								else {
									_set_error(
										vformat(RTR("Redefinition of filter mode: '%s'. The filter "
													"mode has already been set to '%s'."),
											get_texture_filter_name(new_filter),
											get_texture_filter_name(uniform.filter)));
								}
								return ERR_PARSE_ERROR;
							}
							else {
								uniform.filter = new_filter;
								current_uniform_filter = new_filter;
							}
						}

						if (new_repeat != REPEAT_DEFAULT) {
							if (uniform.repeat != REPEAT_DEFAULT) {
								if (uniform.repeat == new_repeat) {
									_set_error(vformat(RTR("Duplicated repeat mode: '%s'."),
										get_texture_repeat_name(new_repeat)));
								}
								else {
									_set_error(
										vformat(RTR("Redefinition of repeat mode: '%s'. The repeat "
													"mode has already been set to '%s'."),
											get_texture_repeat_name(new_repeat),
											get_texture_repeat_name(uniform.repeat)));
								}
								return ERR_PARSE_ERROR;
							}
							else {
								uniform.repeat = new_repeat;
								current_uniform_repeat = new_repeat;
							}
						}

						tk = _get_token();

					} while (tk.type == TK_COMMA);
				}

				if (uniform_scope == ShaderNode::Uniform::SCOPE_INSTANCE) {
					if (custom_instance_index >= 0) {
						uniform.instance_index = custom_instance_index;
					}
					else {
						uniform.instance_index = instance_index++;
						if (instance_index > MAX_INSTANCE_UNIFORM_INDICES) {
							_set_error(vformat(
								RTR("Too many '%s' uniforms in shader, maximum supported is %d."),
								"instance", MAX_INSTANCE_UNIFORM_INDICES));
							return ERR_PARSE_ERROR;
						}
					}
				}

				// reset scope for next uniform

				if (tk.type == TK_OP_ASSIGN) {
					if (uniform.array_size > 0) {
						_set_error(
							RTR("Setting default values to uniform arrays is not supported."));
						return ERR_PARSE_ERROR;
					}

					Node* expr = _parse_and_reduce_expression(nullptr, constants);
					if (!expr) {
						return ERR_PARSE_ERROR;
					}
					if (expr->type != Node::NODE_TYPE_CONSTANT) {
						_set_error(RTR("Expected constant expression after '='."));
						return ERR_PARSE_ERROR;
					}

					ConstantNode* cn = static_cast<ConstantNode*>(expr);

					uniform.default_value.resize(cn->values.size());

					if (!convert_constant(cn, uniform.type, uniform.default_value.ptrw())) {
						_set_error(vformat(RTR("Can't convert constant to '%s'."),
							get_datatype_name(uniform.type)));
						return ERR_PARSE_ERROR;
					}
					tk = _get_token();
				}

				shader->uniforms[name] = uniform;
#ifdef DEBUG_ENABLED
				if (check_warnings && HAS_WARNING(ShaderWarning::UNUSED_UNIFORM_FLAG)) {
					used_uniforms.insert(name, Usage(tk_line));
				}
#endif // DEBUG_ENABLED

				// reset scope for next uniform
				uniform_scope = ShaderNode::Uniform::SCOPE_LOCAL;

				if (tk.type != TK_SEMICOLON) {
					_set_expected_error(";");
					return ERR_PARSE_ERROR;
				}

#ifdef DEBUG_ENABLED
				keyword_completion_context = CF_GLOBAL_SPACE;
#endif // DEBUG_ENABLED
				completion_type = COMPLETION_NONE;

				current_uniform_hint = ShaderNode::Uniform::HINT_NONE;
				current_uniform_filter = FILTER_DEFAULT;
				current_uniform_repeat = REPEAT_DEFAULT;
				current_uniform_instance_index_defined = false;
			}
			else { // varying
				ShaderNode::Varying varying;
				varying.type = type;
				varying.precision = precision;
				varying.interpolation = interpolation;
				varying.tkpos = prev_pos;
				varying.array_size = array_size;

				tk = _get_token();
				if (tk.type != TK_SEMICOLON && tk.type != TK_BRACKET_OPEN) {
					if (array_size == 0) {
						_set_expected_error(";", "[");
					}
					else {
						_set_expected_error(";");
					}
					return ERR_PARSE_ERROR;
				}

				if (tk.type == TK_BRACKET_OPEN) {
					Error error = _parse_array_size(
						nullptr, constants, true, nullptr, &varying.array_size, nullptr);
					if (error != OK) {
						return error;
					}
					tk = _get_token();
				}

				shader->varyings[name] = varying;
#ifdef DEBUG_ENABLED
				if (check_warnings && HAS_WARNING(ShaderWarning::UNUSED_VARYING_FLAG)) {
					used_varyings.insert(name, Usage(tk_line));
				}
#endif // DEBUG_ENABLED
			}

		} break;
		case TK_UNIFORM_GROUP: {
			tk = _get_token();
			if (tk.type == TK_IDENTIFIER) {
				current_uniform_group_name = tk.text;

				tk = _get_token();
				while (tk.type == TK_PERIOD) {
					tk = _get_token();

					if (tk.type == TK_IDENTIFIER) {
						current_uniform_group_name += "/" + tk.text;
					}
					else {
						_set_error(RTR("Expected an uniform subgroup identifier."));
						return ERR_PARSE_ERROR;
					}

					tk = _get_token();
				}

				if (tk.type != TK_SEMICOLON) {
					_set_expected_error(";", ".");
					return ERR_PARSE_ERROR;
				}
			}
			else {
				if (tk.type != TK_SEMICOLON) {
					if (current_uniform_group_name.is_empty()) {
						_set_error(RTR("Expected an uniform group identifier."));
					}
					else {
						_set_error(RTR("Expected an uniform group identifier or `;`."));
					}
					return ERR_PARSE_ERROR;
				}
				else if (current_uniform_group_name.is_empty()) {
					_set_error(RTR("Group needs to be opened before."));
					return ERR_PARSE_ERROR;
				}
				else {
					current_uniform_group_name = "";
				}
			}
		} break;
		case TK_SHADER_TYPE: {
			_set_error(RTR("Shader type is already defined."));
			return ERR_PARSE_ERROR;
		} break;
		default: {
			// function or constant variable

			bool is_constant = false;
			bool is_struct = false;
			String struct_name;
			DataPrecision precision = PRECISION_DEFAULT;
			DataType type;
			String name;
			int array_size = 0;

			if (tk.type == TK_CONST) {
				is_constant = true;
				tk = _get_token();
			}

			if (is_token_precision(tk.type)) {
				precision = get_token_precision(tk.type);
				tk = _get_token();
			}

			if (shader->structs.has(tk.text)) {
				is_struct = true;
				struct_name = tk.text;
			}
			else {
#ifdef DEBUG_ENABLED
				if (_lookup_next(next)) {
					if (next.type == TK_UNIFORM) {
						keyword_completion_context = CF_UNIFORM_QUALIFIER;
					}
				}
#endif // DEBUG_ENABLED
				if (!is_token_datatype(tk.type)) {
					_set_error(RTR("Expected constant, function, uniform or varying."));
					return ERR_PARSE_ERROR;
				}

				if (!is_token_variable_datatype(tk.type)) {
					if (is_constant) {
						_set_error(RTR("Invalid constant type (samplers are not allowed)."));
					}
					else {
						_set_error(RTR("Invalid function type (samplers are not allowed)."));
					}
					return ERR_PARSE_ERROR;
				}
			}

			if (is_struct) {
				type = TYPE_STRUCT;
			}
			else {
				type = get_token_datatype(tk.type);
			}

			if (precision != PRECISION_DEFAULT && _validate_precision(type, precision) != OK) {
				return ERR_PARSE_ERROR;
			}

			prev_pos = _get_tkpos();
			tk = _get_token();

#ifdef DEBUG_ENABLED
			keyword_completion_context = CF_UNSPECIFIED;
#endif // DEBUG_ENABLED

			bool unknown_size = false;
			bool fixed_array_size = false;

			if (tk.type == TK_BRACKET_OPEN) {
				Error error = _parse_array_size(
					nullptr, constants, !is_constant, nullptr, &array_size, &unknown_size);
				if (error != OK) {
					return error;
				}
				fixed_array_size = true;
				prev_pos = _get_tkpos();
			}

			_set_tkpos(prev_pos);

			_get_completable_identifier(nullptr, COMPLETION_MAIN_FUNCTION, name);

			if (name == String()) {
				if (is_constant) {
					_set_error(RTR("Expected an identifier or '[' after type."));
				}
				else {
					_set_error(RTR("Expected a function name after type."));
				}
				return ERR_PARSE_ERROR;
			}

			IdentifierType itype;
			if (shader->structs.has(name) ||
				(_find_identifier(nullptr, false, constants, name, nullptr, &itype) &&
					itype != IDENTIFIER_FUNCTION) ||
				has_builtin(p_functions, name, !is_constant)) {
				_set_redefinition_error(String(name));
				return ERR_PARSE_ERROR;
			}

			tk = _get_token();
			if (tk.type != TK_PARENTHESIS_OPEN) {
				if (type == TYPE_VOID) {
					_set_error(RTR("Expected '(' after function identifier."));
					return ERR_PARSE_ERROR;
				}

				// variable
				while (true) {
					ShaderNode::Constant constant;
					constant.name = name;
					constant.type = is_struct ? TYPE_STRUCT : type;
					constant.struct_name = struct_name;
					constant.precision = precision;
					constant.initializer = nullptr;
					constant.array_size = array_size;
					is_const_decl = true;

					if (tk.type == TK_BRACKET_OPEN) {
						Error error = _parse_array_size(nullptr, constants, false, nullptr,
							&constant.array_size, &unknown_size);
						if (error != OK) {
							return error;
						}
						tk = _get_token();
					}

					if (tk.type == TK_OP_ASSIGN) {
						if (!is_constant) {
							_set_error(
								vformat(RTR("Global non-constant variables are not supported. "
											"Expected '%s' keyword before constant definition."),
									"const"));
							return ERR_PARSE_ERROR;
						}

						if (constant.array_size > 0 || unknown_size) {
							bool full_def = false;

							VariableDeclarationNode::Declaration decl;
							decl.name = name;
							decl.size = constant.array_size;

							tk = _get_token();

							if (tk.type != TK_CURLY_BRACKET_OPEN) {
								if (unknown_size) {
									_set_expected_error("{");
									return ERR_PARSE_ERROR;
								}

								full_def = true;

								DataPrecision precision2 = PRECISION_DEFAULT;
								if (is_token_precision(tk.type)) {
									precision2 = get_token_precision(tk.type);
									tk = _get_token();
									if (!is_token_nonvoid_datatype(tk.type)) {
										_set_error(
											RTR("Expected data type after precision modifier."));
										return ERR_PARSE_ERROR;
									}
								}

								String struct_name2;
								DataType type2;

								if (shader->structs.has(tk.text)) {
									type2 = TYPE_STRUCT;
									struct_name2 = tk.text;
								}
								else {
									if (!is_token_variable_datatype(tk.type)) {
										_set_error(RTR("Invalid data type for the array."));
										return ERR_PARSE_ERROR;
									}
									type2 = get_token_datatype(tk.type);
								}

								int array_size2 = 0;
								tk = _get_token();

								if (tk.type == TK_BRACKET_OPEN) {
									bool is_unknown_size = false;
									Error error = _parse_array_size(nullptr, constants, false,
										nullptr, &array_size2, &is_unknown_size);
									if (error != OK) {
										return error;
									}
									if (is_unknown_size) {
										array_size2 = constant.array_size;
									}
									tk = _get_token();
								}
								else {
									_set_expected_error("[");
									return ERR_PARSE_ERROR;
								}

								if (constant.precision != precision2 || constant.type != type2 ||
									struct_name != struct_name2 ||
									constant.array_size != array_size2) {
									String from;
									if (type2 == TYPE_STRUCT) {
										from += struct_name2;
									}
									else {
										if (precision2 != PRECISION_DEFAULT) {
											from += get_precision_name(precision2);
											from += " ";
										}
										from += get_datatype_name(type2);
									}
									from += "[";
									from += itos(array_size2);
									from += "]'";

									String to;
									if (type == TYPE_STRUCT) {
										to += struct_name;
									}
									else {
										if (precision != PRECISION_DEFAULT) {
											to += get_precision_name(precision);
											to += " ";
										}
										to += get_datatype_name(type);
									}
									to += "[";
									to += itos(constant.array_size);
									to += "]'";

									_set_error(vformat(
										RTR("Cannot convert from '%s' to '%s'."), from, to));
									return ERR_PARSE_ERROR;
								}
							}

							bool curly = tk.type == TK_CURLY_BRACKET_OPEN;

							if (unknown_size) {
								if (!curly) {
									_set_expected_error("{");
									return ERR_PARSE_ERROR;
								}
							}
							else {
								if (full_def) {
									if (curly) {
										_set_expected_error("(");
										return ERR_PARSE_ERROR;
									}
								}
							}

							if (tk.type == TK_PARENTHESIS_OPEN || curly) { // initialization
								while (true) {
									Node* n = _parse_and_reduce_expression(nullptr, constants);
									if (!n) {
										return ERR_PARSE_ERROR;
									}

									if (n->type == Node::NODE_TYPE_OPERATOR &&
										static_cast<OperatorNode*>(n)->op == OP_CALL) {
										_set_error(RTR("Expected constant expression."));
										return ERR_PARSE_ERROR;
									}

									if (!_compare_datatypes(constant.type, struct_name, 0,
											n->get_datatype(), n->get_datatype_name(), 0)) {
										return ERR_PARSE_ERROR;
									}

									tk = _get_token();
									if (tk.type == TK_COMMA) {
										decl.initializer.push_back(n);
										continue;
									}
									else if (!curly && tk.type == TK_PARENTHESIS_CLOSE) {
										decl.initializer.push_back(n);
										break;
									}
									else if (curly && tk.type == TK_CURLY_BRACKET_CLOSE) {
										decl.initializer.push_back(n);
										break;

									}
									else {
										if (curly) {
											_set_expected_error("}", ",");
										}
										else {
											_set_expected_error(")", ",");
										}
										return ERR_PARSE_ERROR;
									}
								}
								if (unknown_size) {
									decl.size = decl.initializer.size();
									constant.array_size = decl.initializer.size();
								}
								else if (decl.initializer.size() != constant.array_size) {
									_set_error(vformat(RTR("Array size mismatch. Expected %d "
														   "elements (found %d)."),
										constant.array_size, decl.initializer.size()));
									return ERR_PARSE_ERROR;
								}
							}
							else {
								_set_expected_error("(");
								return ERR_PARSE_ERROR;
							}

							array_size = constant.array_size;

							ConstantNode* expr = memnew(ConstantNode);

							expr->datatype = constant.type;

							expr->struct_name = constant.struct_name;

							expr->array_size = constant.array_size;

							expr->array_declarations.push_back(decl);

							constant.initializer = static_cast<ConstantNode*>(expr);
						}
						else {
#ifdef DEBUG_ENABLED
							if (constant.type == DataType::TYPE_BOOL) {
								keyword_completion_context = CF_BOOLEAN;
							}
#endif // DEBUG_ENABLED

							// variable created with assignment! must parse an expression
							Node* expr = _parse_and_reduce_expression(nullptr, constants);
							if (!expr) {
								return ERR_PARSE_ERROR;
							}
#ifdef DEBUG_ENABLED
							if (constant.type == DataType::TYPE_BOOL) {
								keyword_completion_context = CF_GLOBAL_SPACE;
							}
#endif // DEBUG_ENABLED
							if (expr->type == Node::NODE_TYPE_OPERATOR &&
								static_cast<OperatorNode*>(expr)->op == OP_CALL) {
								OperatorNode* op = static_cast<OperatorNode*>(expr);
								for (int i = 1; i < op->arguments.size(); i++) {
									if (!_check_node_constness(op->arguments[i])) {
										_set_error(
											vformat(RTR("Expected constant expression for argument "
														"%d of function call after '='."),
												i - 1));
										return ERR_PARSE_ERROR;
									}
								}
							}

							constant.initializer = expr;

							if (!_compare_datatypes(type, struct_name, 0, expr->get_datatype(),
									expr->get_datatype_name(), expr->get_array_size())) {
								return ERR_PARSE_ERROR;
							}
						}
						tk = _get_token();
					}
					else {
						if (constant.array_size > 0 || unknown_size) {
							_set_error(RTR("Expected array initialization."));
							return ERR_PARSE_ERROR;
						}
						else {
							_set_error(RTR("Expected initialization of constant."));
							return ERR_PARSE_ERROR;
						}
					}

					shader->constants[name] = constant;
					shader->vconstants.push_back(constant);
#ifdef DEBUG_ENABLED
					if (check_warnings && HAS_WARNING(ShaderWarning::UNUSED_CONSTANT_FLAG)) {
						used_constants.insert(name, Usage(tk_line));
					}
#endif // DEBUG_ENABLED

					if (tk.type == TK_COMMA) {
						tk = _get_token();
						if (tk.type != TK_IDENTIFIER) {
							_set_error(RTR("Expected an identifier after type."));
							return ERR_PARSE_ERROR;
						}

						name = tk.text;
						if (_find_identifier(nullptr, false, constants, name)) {
							_set_redefinition_error(String(name));
							return ERR_PARSE_ERROR;
						}

						if (has_builtin(p_functions, name)) {
							_set_redefinition_error(String(name));
							return ERR_PARSE_ERROR;
						}

						tk = _get_token();

						if (!fixed_array_size) {
							array_size = 0;
						}
						unknown_size = false;

					}
					else if (tk.type == TK_SEMICOLON) {
						is_const_decl = false;
						break;
					}
					else {
						_set_expected_error(",", ";");
						return ERR_PARSE_ERROR;
					}
				}

				break;
			}

			if (is_constant) {
				_set_error(vformat(
					RTR("'%s' qualifier cannot be used with a function return type."), "const"));
				return ERR_PARSE_ERROR;
			}

			FunctionInfo builtins;
			if (p_functions.has(name)) {
				builtins = p_functions[name];
			}

			if (p_functions.has("global")) { // Adds global variables: 'TIME'
				for (const KeyValue<String, BuiltInInfo>& E : p_functions["global"].built_ins) {
					builtins.built_ins.insert(E.key, E.value);
				}
			}

			if (p_functions.has("constants")) { // Adds global constants: 'PI', 'TAU', 'E'
				for (const KeyValue<String, BuiltInInfo>& E :
					p_functions["constants"].built_ins) {
					builtins.built_ins.insert(E.key, E.value);
				}
			}

			for (int i = 0; i < shader->vfunctions.size(); i++) {
				if (!shader->vfunctions[i].callable && shader->vfunctions[i].name == name) {
					_set_redefinition_error(String(name));
					return ERR_PARSE_ERROR;
				}
			}

			ShaderNode::Function function;

			function.callable = !p_functions.has(name);
			function.name = name;
			function.rname = name;

			FunctionNode* func_node = alloc_node<FunctionNode>();
			function.function = func_node;

			func_node->name = name;
			func_node->rname = name;
			func_node->return_type = type;
			func_node->return_struct_name = struct_name;
			func_node->return_precision = precision;
			func_node->return_array_size = array_size;

			if (p_functions.has(name)) {
				func_node->can_discard = p_functions[name].can_discard;
			}
			else {
				func_node->can_discard = is_discard_supported; // Allow use it for custom functions
															   // (in supported shader types).
			}

			if (!function_overload_count.has(name)) {
				function_overload_count.insert(name, 0);
			}
			else {
				function_overload_count[name]++;
			}

			func_node->body = alloc_node<BlockNode>();
			func_node->body->parent_function = func_node;

			tk = _get_token();

			while (true) {
				if (tk.type == TK_PARENTHESIS_CLOSE) {
					break;
				}
#ifdef DEBUG_ENABLED
				keyword_completion_context = CF_CONST_KEYWORD | CF_FUNC_DECL_PARAM_SPEC |
											 CF_PRECISION_MODIFIER |
											 CF_FUNC_DECL_PARAM_TYPE; // eg. const in mediump float

				if (_lookup_next(next)) {
					if (next.type == TK_CONST) {
						keyword_completion_context = CF_UNSPECIFIED;
					}
					else if (is_token_arg_qual(next.type)) {
						keyword_completion_context = CF_CONST_KEYWORD;
					}
					else if (is_token_precision(next.type)) {
						keyword_completion_context =
							(CF_CONST_KEYWORD | CF_FUNC_DECL_PARAM_SPEC | CF_FUNC_DECL_PARAM_TYPE);
					}
					else if (is_token_datatype(next.type)) {
						keyword_completion_context =
							(CF_CONST_KEYWORD | CF_FUNC_DECL_PARAM_SPEC | CF_PRECISION_MODIFIER);
					}
				}
#endif // DEBUG_ENABLED

				bool param_is_const = false;
				if (tk.type == TK_CONST) {
					param_is_const = true;
					tk = _get_token();
#ifdef DEBUG_ENABLED
					if (keyword_completion_context & CF_CONST_KEYWORD) {
						keyword_completion_context ^= CF_CONST_KEYWORD;
					}

					if (_lookup_next(next)) {
						if (is_token_arg_qual(next.type)) {
							keyword_completion_context = CF_UNSPECIFIED;
						}
						else if (is_token_precision(next.type)) {
							keyword_completion_context =
								(CF_FUNC_DECL_PARAM_SPEC | CF_FUNC_DECL_PARAM_TYPE);
						}
						else if (is_token_datatype(next.type)) {
							keyword_completion_context =
								(CF_FUNC_DECL_PARAM_SPEC | CF_PRECISION_MODIFIER);
						}
					}
#endif // DEBUG_ENABLED
				}

				ArgumentQualifier param_qualifier = ARGUMENT_QUALIFIER_IN;
				if (is_token_arg_qual(tk.type)) {
					bool error = false;
					switch (tk.type) {
					case TK_ARG_IN: {
						param_qualifier = ARGUMENT_QUALIFIER_IN;
					} break;
					case TK_ARG_OUT: {
						if (param_is_const) {
							_set_error(vformat(RTR("The '%s' qualifier cannot be used within a "
												   "function parameter declared with '%s'."),
								"out", "const"));
							error = true;
						}
						param_qualifier = ARGUMENT_QUALIFIER_OUT;
					} break;
					case TK_ARG_INOUT: {
						if (param_is_const) {
							_set_error(vformat(RTR("The '%s' qualifier cannot be used within a "
												   "function parameter declared with '%s'."),
								"inout", "const"));
							error = true;
						}
						param_qualifier = ARGUMENT_QUALIFIER_INOUT;
					} break;
					default:
						error = true;
						break;
					}
					tk = _get_token();
#ifdef DEBUG_ENABLED
					if (keyword_completion_context & CF_CONST_KEYWORD) {
						keyword_completion_context ^= CF_CONST_KEYWORD;
					}
					if (keyword_completion_context & CF_FUNC_DECL_PARAM_SPEC) {
						keyword_completion_context ^= CF_FUNC_DECL_PARAM_SPEC;
					}

					if (_lookup_next(next)) {
						if (is_token_precision(next.type)) {
							keyword_completion_context = CF_FUNC_DECL_PARAM_TYPE;
						}
						else if (is_token_datatype(next.type)) {
							keyword_completion_context = CF_PRECISION_MODIFIER;
						}
					}
#endif // DEBUG_ENABLED
					if (error) {
						return ERR_PARSE_ERROR;
					}
				}

				DataType param_type;
				String param_name;
				String param_struct_name;
				DataPrecision param_precision = PRECISION_DEFAULT;
				int arg_array_size = 0;

				if (is_token_precision(tk.type)) {
					param_precision = get_token_precision(tk.type);
					tk = _get_token();
#ifdef DEBUG_ENABLED
					if (keyword_completion_context & CF_CONST_KEYWORD) {
						keyword_completion_context ^= CF_CONST_KEYWORD;
					}
					if (keyword_completion_context & CF_FUNC_DECL_PARAM_SPEC) {
						keyword_completion_context ^= CF_FUNC_DECL_PARAM_SPEC;
					}
					if (keyword_completion_context & CF_PRECISION_MODIFIER) {
						keyword_completion_context ^= CF_PRECISION_MODIFIER;
					}

					if (_lookup_next(next)) {
						if (is_token_datatype(next.type)) {
							keyword_completion_context = CF_UNSPECIFIED;
						}
					}
#endif // DEBUG_ENABLED
				}

				is_struct = false;

				if (shader->structs.has(tk.text)) {
					is_struct = true;
					param_struct_name = tk.text;
#ifdef DEBUG_ENABLED
					if (check_warnings && HAS_WARNING(ShaderWarning::UNUSED_STRUCT_FLAG) &&
						used_structs.has(param_struct_name)) {
						used_structs[param_struct_name].used = true;
					}
#endif // DEBUG_ENABLED
				}

				if (!is_struct && !is_token_datatype(tk.type)) {
					_set_error(RTR("Expected a valid data type for argument."));
					return ERR_PARSE_ERROR;
				}

				if (param_qualifier == ARGUMENT_QUALIFIER_OUT ||
					param_qualifier == ARGUMENT_QUALIFIER_INOUT) {
					if (is_sampler_type(get_token_datatype(tk.type))) {
						_set_error(RTR("Opaque types cannot be output parameters."));
						return ERR_PARSE_ERROR;
					}
				}

				if (is_struct) {
					param_type = TYPE_STRUCT;
				}
				else {
					param_type = get_token_datatype(tk.type);
					if (param_type == TYPE_VOID) {
						_set_error(RTR("Void type not allowed as argument."));
						return ERR_PARSE_ERROR;
					}
				}

				if (param_precision != PRECISION_DEFAULT &&
					_validate_precision(param_type, param_precision) != OK) {
					return ERR_PARSE_ERROR;
				}
#ifdef DEBUG_ENABLED
				keyword_completion_context = CF_UNSPECIFIED;
#endif // DEBUG_ENABLED
				tk = _get_token();

				if (tk.type == TK_BRACKET_OPEN) {
					Error error = _parse_array_size(
						nullptr, constants, true, nullptr, &arg_array_size, nullptr);
					if (error != OK) {
						return error;
					}
					tk = _get_token();
				}
				if (tk.type != TK_IDENTIFIER) {
					_set_error(RTR("Expected an identifier for argument name."));
					return ERR_PARSE_ERROR;
				}

				param_name = tk.text;

				if (_find_identifier(func_node->body, false, builtins, param_name,
						(ShaderLanguage::DataType*)nullptr, &itype)) {
					if (itype != IDENTIFIER_FUNCTION) {
						_set_redefinition_error(String(param_name));
						return ERR_PARSE_ERROR;
					}
				}

				if (has_builtin(p_functions, param_name)) {
					_set_redefinition_error(String(param_name));
					return ERR_PARSE_ERROR;
				}

				FunctionNode::Argument arg;
				arg.type = param_type;
				arg.name = param_name;
				arg.struct_name = param_struct_name;
				arg.precision = param_precision;
				arg.qualifier = param_qualifier;
				arg.tex_argument_check = false;
				arg.tex_builtin_check = false;
				arg.tex_argument_filter = FILTER_DEFAULT;
				arg.tex_argument_repeat = REPEAT_DEFAULT;
				arg.is_const = param_is_const;

				tk = _get_token();
				if (tk.type == TK_BRACKET_OPEN) {
					Error error = _parse_array_size(
						nullptr, constants, true, nullptr, &arg_array_size, nullptr);
					if (error != OK) {
						return error;
					}
					tk = _get_token();
				}

				arg.array_size = arg_array_size;
				func_node->arguments.push_back(arg);

				if (tk.type == TK_COMMA) {
					tk = _get_token();
					// do none and go on
				}
				else if (tk.type != TK_PARENTHESIS_CLOSE) {
					_set_expected_error(",", ")");
					return ERR_PARSE_ERROR;
				}
			}

			// Searches for function index and check for the exact duplicate in overloads.
			int function_index = 0;
			for (int i = 0; i < shader->vfunctions.size(); i++) {
				if (!shader->vfunctions[i].callable || shader->vfunctions[i].rname != name) {
					continue;
				}

				function_index++;

				if (shader->vfunctions[i].function->arguments.size() !=
					func_node->arguments.size()) {
					continue;
				}

				bool is_same = true;

				for (int j = 0; j < shader->vfunctions[i].function->arguments.size(); j++) {
					FunctionNode::Argument a = func_node->arguments[j];
					FunctionNode::Argument b = shader->vfunctions[i].function->arguments[j];

					if (a.type == b.type && a.array_size == b.array_size) {
						if (a.type == TYPE_STRUCT) {
							is_same = a.struct_name == b.struct_name;
						}
					}
					else {
						is_same = false;
					}

					if (!is_same) {
						break;
					}
				}

				if (is_same) {
					_set_redefinition_error(String(name));
					return ERR_PARSE_ERROR;
				}
			}

			// Creates a fake name for function overload, which will be replaced by the real name by
			// the compiler.
			String name2 = name;
			if (function_index > 0) {
				name2 = vformat("%s@%s", name, itos(function_index + 1));

				function.name = name2;
				func_node->name = name2;
			}

			shader->functions.insert(name2, function);
			shader->vfunctions.push_back(function);

			CallInfo call_info;
			call_info.name = name2;
			calls_info.insert(name2, call_info);

#ifdef DEBUG_ENABLED
			if (check_warnings && HAS_WARNING(ShaderWarning::UNUSED_FUNCTION_FLAG) &&
				!p_functions.has(name)) {
				used_functions.insert(name2, Usage(tk_line));
			}
#endif // DEBUG_ENABLED

			if (p_functions.has(name)) {
				// if one of the core functions, make sure they are of the correct form
				if (func_node->arguments.size() > 0) {
					_set_error(vformat(RTR("Function '%s' expects no arguments."), String(name)));
					return ERR_PARSE_ERROR;
				}
				if (func_node->return_type != TYPE_VOID) {
					_set_error(vformat(
						RTR("Function '%s' must be of '%s' return type."), String(name), "void"));
					return ERR_PARSE_ERROR;
				}
			}

			// all good let's parse inside the function!
			tk = _get_token();
			if (tk.type != TK_CURLY_BRACKET_OPEN) {
				_set_error(RTR("Expected a '{' to begin function."));
				return ERR_PARSE_ERROR;
			}

			current_function = name2;

#ifdef DEBUG_ENABLED
			keyword_completion_context = CF_BLOCK;
#endif // DEBUG_ENABLED
			Error err = _parse_block(func_node->body, builtins);
			if (err) {
				return err;
			}
#ifdef DEBUG_ENABLED
			keyword_completion_context = CF_GLOBAL_SPACE;
#endif // DEBUG_ENABLED
			if (func_node->return_type != DataType::TYPE_VOID) {
				BlockNode* block = func_node->body;
				if (_find_last_flow_op_in_block(block, FlowOperation::FLOW_OP_RETURN) != OK) {
					_set_error(
						vformat(RTR("Expected at least one '%s' statement in a non-void function."),
							"return"));
					return ERR_PARSE_ERROR;
				}
			}
			current_function = String();
		}
		}

		tk = _get_token();
	}
	uint32_t varying_index = base_varying_index;
	uint32_t max_varyings = 31;
	max_varyings = RendererUtilities::get_maximum_shader_varyings();

	for (const KeyValue<String, ShaderNode::Varying>& kv : shader->varyings) {
		if (kv.value.stage != ShaderNode::Varying::STAGE_FRAGMENT &&
			(kv.value.type > TYPE_BVEC4 && kv.value.type < TYPE_FLOAT) &&
			kv.value.interpolation != INTERPOLATION_FLAT) {
			_set_tkpos(kv.value.tkpos);
			_set_error(vformat(RTR("Varying with integer data type must be declared with `%s` "
								   "interpolation qualifier."),
				"flat"));
			return ERR_PARSE_ERROR;
		}

		if (varying_index + kv.value.get_size() > max_varyings) {
			_set_tkpos(kv.value.tkpos);
			_set_error(
				vformat(RTR("Too many varyings used in shader (%d used, maximum supported is %d)."),
					varying_index + kv.value.get_size(), max_varyings));
			return ERR_PARSE_ERROR;
		}

		varying_index += kv.value.get_size();
	}
	return OK;
}

bool ShaderLanguage::has_builtin(
	const HashMap<String, ShaderLanguage::FunctionInfo>& p_functions, const String& p_name,
	bool p_check_global_funcs)
{
	if (p_check_global_funcs && global_func_set.has(p_name)) {
		return true;
	}

	for (const KeyValue<String, ShaderLanguage::FunctionInfo>& E : p_functions) {
		if (E.value.built_ins.has(p_name)) {
			return true;
		}
	}

	return false;
}

Error ShaderLanguage::_find_last_flow_op_in_op(ControlFlowNode* p_flow, FlowOperation p_op)
{
	bool found = false;

	for (int i = p_flow->blocks.size() - 1; i >= 0; i--) {
		if (p_flow->blocks[i]->type == Node::NODE_TYPE_BLOCK) {
			BlockNode* last_block = static_cast<BlockNode*>(p_flow->blocks[i]);
			if (_find_last_flow_op_in_block(last_block, p_op) == OK) {
				found = true;
				break;
			}
		}
	}
	if (found) {
		return OK;
	}
	return FAILED;
}

Error ShaderLanguage::_find_last_flow_op_in_block(BlockNode* p_block, FlowOperation p_op)
{
	bool found = false;

	for (List<ShaderLanguage::Node*>::Element* E = p_block->statements.back(); E; E = E->prev()) {
		if (E->get()->type == Node::NODE_TYPE_CONTROL_FLOW) {
			ControlFlowNode* flow = static_cast<ControlFlowNode*>(E->get());
			if (flow->flow_op == p_op) {
				found = true;
				break;
			}
			else {
				if (_find_last_flow_op_in_op(flow, p_op) == OK) {
					found = true;
					break;
				}
			}
		}
		else if (E->get()->type == Node::NODE_TYPE_BLOCK) {
			BlockNode* block = static_cast<BlockNode*>(E->get());
			if (_find_last_flow_op_in_block(block, p_op) == OK) {
				found = true;
				break;
			}
		}
	}

	if (found) {
		return OK;
	}
	return FAILED;
}

Error ShaderLanguage::_parse_shader_mode(
	bool p_is_stencil, const Vector<ModeInfo>& p_modes, HashMap<String, String>& r_defined_modes)
{
	String mode;
	_get_completable_identifier(
		nullptr, p_is_stencil ? COMPLETION_STENCIL_MODE : COMPLETION_RENDER_MODE, mode);

	if (mode == String()) {
		if (p_is_stencil) {
			_set_error(RTR("Expected an identifier for stencil mode."));
		}
		else {
			_set_error(RTR("Expected an identifier for render mode."));
		}
		return ERR_PARSE_ERROR;
	}

	const String smode = String(mode);

	Vector<String>& current_modes = p_is_stencil ? shader->stencil_modes : shader->render_modes;

	if (current_modes.has(mode)) {
		if (p_is_stencil) {
			_set_error(vformat(RTR("Duplicated stencil mode: '%s'."), smode));
		}
		else {
			_set_error(vformat(RTR("Duplicated render mode: '%s'."), smode));
		}
		return ERR_PARSE_ERROR;
	}

	bool found = false;

	if (is_shader_inc) {
		for (int i = 0; i < RSE::SHADER_MAX; i++) {
			const Vector<ModeInfo> modes =
				p_is_stencil ? ShaderTypes::get_stencil_modes(RSE::ShaderMode(i))
							 : ShaderTypes::get_modes(RSE::ShaderMode(i));

			for (const ModeInfo& info : modes) {
				const String name = String(info.name);

				if (smode.begins_with(name)) {
					if (!info.options.is_empty()) {
						if (info.options.has(smode.substr(name.length() + 1))) {
							found = true;

							if (r_defined_modes.has(name)) {
								if (p_is_stencil) {
									_set_error(
										vformat(RTR("Redefinition of stencil mode: '%s'. The '%s' "
													"mode has already been set to '%s'."),
											smode, name, r_defined_modes[name]));
								}
								else {
									_set_error(
										vformat(RTR("Redefinition of render mode: '%s'. The '%s' "
													"mode has already been set to '%s'."),
											smode, name, r_defined_modes[name]));
								}
								return ERR_PARSE_ERROR;
							}
							r_defined_modes.insert(name, smode);
							break;
						}
					}
					else {
						found = true;
						break;
					}
				}
			}
		}
	}
	else {
		for (const ModeInfo& info : p_modes) {
			const String name = String(info.name);

			if (smode.begins_with(name)) {
				if (!info.options.is_empty()) {
					if (info.options.has(smode.substr(name.length() + 1))) {
						found = true;

						if (r_defined_modes.has(name)) {
							if (p_is_stencil) {
								_set_error(vformat(RTR("Redefinition of stencil mode: '%s'. The "
													   "'%s' mode has already been set to '%s'."),
									smode, name, r_defined_modes[name]));
							}
							else {
								_set_error(vformat(RTR("Redefinition of render mode: '%s'. The "
													   "'%s' mode has already been set to '%s'."),
									smode, name, r_defined_modes[name]));
							}
							return ERR_PARSE_ERROR;
						}
						r_defined_modes.insert(name, smode);
						break;
					}
				}
				else {
					found = true;
					break;
				}
			}
		}
	}

	if (!found) {
		if (p_is_stencil) {
			_set_error(vformat(RTR("Invalid stencil mode: '%s'."), smode));
		}
		else {
			_set_error(vformat(RTR("Invalid render mode: '%s'."), smode));
		}
		return ERR_PARSE_ERROR;
	}

	if (p_is_stencil) {
		shader->stencil_modes.push_back(mode);
	}
	else {
		shader->render_modes.push_back(mode);
	}

	return OK;
}

// skips over whitespace and /* */ and // comments
static int _get_first_ident_pos(const String& p_code)
{
	int idx = 0;

#define GETCHAR(m_idx) (((idx + m_idx) < p_code.length()) ? p_code[idx + m_idx] : char32_t(0))

	while (true) {
		if (GETCHAR(0) == '/' && GETCHAR(1) == '/') {
			idx += 2;
			while (true) {
				if (GETCHAR(0) == 0) {
					return 0;
				}
				if (GETCHAR(0) == '\n') {
					idx++;
					break; // loop
				}
				idx++;
			}
		}
		else if (GETCHAR(0) == '/' && GETCHAR(1) == '*') {
			idx += 2;
			while (true) {
				if (GETCHAR(0) == 0) {
					return 0;
				}
				if (GETCHAR(0) == '*' && GETCHAR(1) == '/') {
					idx += 2;
					break; // loop
				}
				idx++;
			}
		}
		else {
			switch (GETCHAR(0)) {
			case ' ':
			case '\t':
			case '\r':
			case '\n': {
				idx++;
			} break; // switch
			default:
				return idx;
			}
		}
	}

#undef GETCHAR
}

String ShaderLanguage::get_shader_type(const String& p_code)
{
	bool reading_type = false;

	String cur_identifier;

	for (int i = _get_first_ident_pos(p_code); i < p_code.length(); i++) {
		if (p_code[i] == ';') {
			break;

		}
		else if (p_code[i] <= 32) {
			if (!cur_identifier.is_empty()) {
				if (!reading_type) {
					if (cur_identifier != "shader_type") {
						return String();
					}

					reading_type = true;
					cur_identifier = String();
				}
				else {
					return cur_identifier;
				}
			}
		}
		else {
			cur_identifier += String::chr(p_code[i]);
		}
	}

	if (reading_type) {
		return cur_identifier;
	}

	return String();
}

bool ShaderLanguage::is_builtin_func_out_parameter(const String& p_name, int p_param)
{
	int i = 0;
	while (builtin_func_out_args[i].name) {
		if (p_name == builtin_func_out_args[i].name) {
			for (int j = 0; j < BuiltinFuncOutArgs::MAX_ARGS; j++) {
				int arg = builtin_func_out_args[i].arguments[j];
				if (arg == p_param) {
					return true;
				}
				if (arg < 0) {
					return false;
				}
			}
		}
		i++;
	}
	return false;
}

#ifdef DEBUG_ENABLED
List<ShaderWarning>::Element* ShaderLanguage::get_warnings_ptr() { return warnings.front(); }

void ShaderLanguage::enable_warning_checking(bool p_enabled) { check_warnings = p_enabled; }

bool ShaderLanguage::is_warning_checking_enabled() const { return check_warnings; }

void ShaderLanguage::set_warning_flags(uint32_t p_flags) { warning_flags = p_flags; }

uint32_t ShaderLanguage::get_warning_flags() const { return warning_flags; }
#endif // DEBUG_ENABLED

Error ShaderLanguage::compile(const String& p_code, const ShaderCompileInfo& p_info)
{
	clear();
	is_shader_inc = p_info.is_include;

	code = p_code;
	global_shader_uniform_get_type_func = p_info.global_shader_uniform_type_func;

	varying_function_names = p_info.varying_function_names;
	base_varying_index = p_info.base_varying_index;

	nodes = nullptr;

	shader = alloc_node<ShaderNode>();
	Error err = _parse_shader(
		p_info.functions, p_info.render_modes, p_info.stencil_modes, p_info.shader_types);

#ifdef DEBUG_ENABLED
	if (check_warnings) {
		_check_warning_accums();
	}
#endif // DEBUG_ENABLED

	if (err != OK) {
		return err;
	}
	return OK;
}

String ShaderLanguage::get_error_text() { return error_str; }

Vector<ShaderLanguage::FilePosition> ShaderLanguage::get_include_positions()
{
	return include_positions;
}

int ShaderLanguage::get_error_line() { return error_line; }

ShaderLanguage::ShaderNode* ShaderLanguage::get_shader() { return shader; }

ShaderLanguage::ShaderLanguage()
{
	nodes = nullptr;
	completion_class = TAG_GLOBAL;

	if (instance_counter.get() == 0) {
		int idx = 0;
		while (builtin_func_defs[idx].name) {
			if (builtin_func_defs[idx].tag == SubClassTag::TAG_GLOBAL) {
				global_func_set.insert(builtin_func_defs[idx].name);
			}
			idx++;
		}
	}
	instance_counter.increment();

#ifdef DEBUG_ENABLED
	warnings_check_map.insert(ShaderWarning::UNUSED_CONSTANT, &used_constants);
	warnings_check_map.insert(ShaderWarning::UNUSED_FUNCTION, &used_functions);
	warnings_check_map.insert(ShaderWarning::UNUSED_STRUCT, &used_structs);
	warnings_check_map.insert(ShaderWarning::UNUSED_UNIFORM, &used_uniforms);
	warnings_check_map.insert(ShaderWarning::UNUSED_VARYING, &used_varyings);

	warnings_check_map2.insert(ShaderWarning::UNUSED_LOCAL_VARIABLE, &used_local_vars);
#endif // DEBUG_ENABLED
}

ShaderLanguage::~ShaderLanguage()
{
	clear();
	instance_counter.decrement();
	if (instance_counter.get() == 0) {
		global_func_set.clear();
	}
}

void ShaderLanguage::_check_warning_accums() {}

void ShaderLanguage::finish() { global_func_set.clear(); }


// ============================================================================
// Merged from servers/rendering/shader_compiler.cpp
// ============================================================================

#include "servers/rendering/renderer.h"
#include "servers/rendering/shader_language.h"
#include "core/io/resource_loader.h"
#include "core/os/os.h"
#include "servers/rendering/shader_language.h"

#ifdef DEV_ENABLED
#include "scene/resources/shader.h"
#endif
#include "servers/rendering/shader_language.h"

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
