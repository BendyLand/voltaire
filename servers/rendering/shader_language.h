/**************************************************************************/
/*  shader_language.h                                                     */
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

#pragma once

#include "core/string/ustring.h"
#include "core/templates/hash_map.h"
#include "core/templates/list.h"
#include "core/templates/rb_map.h"
#include "core/templates/safe_refcount.h"
#include "core/typedefs.h"
#include "core/types.h"

#ifdef DEBUG_ENABLED

#include "core/string/ustring.h"
#include "core/templates/hash_map.h"

class ShaderWarning final
{
public:
	enum Code
	{
		FLOAT_COMPARISON,
		UNUSED_CONSTANT,
		UNUSED_FUNCTION,
		UNUSED_STRUCT,
		UNUSED_UNIFORM,
		UNUSED_VARYING,
		UNUSED_LOCAL_VARIABLE,
		FORMATTING_ERROR,
		DEVICE_LIMIT_EXCEEDED,
		MAGIC_POSITION_WRITE,
		WARNING_MAX,
	};

	enum CodeFlags : uint32_t
	{
		NONE_FLAG = 0U,
		FLOAT_COMPARISON_FLAG = 1U,
		UNUSED_CONSTANT_FLAG = 2U,
		UNUSED_FUNCTION_FLAG = 4U,
		UNUSED_STRUCT_FLAG = 8U,
		UNUSED_UNIFORM_FLAG = 16U,
		UNUSED_VARYING_FLAG = 32U,
		UNUSED_LOCAL_VARIABLE_FLAG = 64U,
		FORMATTING_ERROR_FLAG = 128U,
		DEVICE_LIMIT_EXCEEDED_FLAG = 256U,
		MAGIC_POSITION_WRITE_FLAG = 512U,
	};

private:
	Code code;
	int line;
	String subject;

public:
	Code get_code() const;
	int get_line() const;
	const String& get_subject() const;
	String get_message() const;
	String get_name() const;

	static String get_name_from_code(Code p_code);
	static Code get_code_from_name(const String& p_name);
	static CodeFlags get_flags_from_codemap(const HashMap<Code, bool>& p_map);
};

#endif // DEBUG_ENABLED

class ShaderLanguage
{
public:
	struct TkPos
	{
		int char_idx;
		int tk_line;
	};

	enum TokenType
	{
		TK_EMPTY,
		TK_IDENTIFIER,
		TK_TRUE,
		TK_FALSE,
		TK_FLOAT_CONSTANT,
		TK_INT_CONSTANT,
		TK_UINT_CONSTANT,
		TK_STRING_CONSTANT,
		TK_TYPE_VOID,
		TK_TYPE_BOOL,
		TK_TYPE_BVEC2,
		TK_TYPE_BVEC3,
		TK_TYPE_BVEC4,
		TK_TYPE_INT,
		TK_TYPE_IVEC2,
		TK_TYPE_IVEC3,
		TK_TYPE_IVEC4,
		TK_TYPE_UINT,
		TK_TYPE_UVEC2,
		TK_TYPE_UVEC3,
		TK_TYPE_UVEC4,
		TK_TYPE_FLOAT,
		TK_TYPE_VEC2,
		TK_TYPE_VEC3,
		TK_TYPE_VEC4,
		TK_TYPE_MAT2,
		TK_TYPE_MAT3,
		TK_TYPE_MAT4,
		TK_TYPE_SAMPLER2D,
		TK_TYPE_ISAMPLER2D,
		TK_TYPE_USAMPLER2D,
		TK_TYPE_SAMPLER2DARRAY,
		TK_TYPE_ISAMPLER2DARRAY,
		TK_TYPE_USAMPLER2DARRAY,
		TK_TYPE_SAMPLER3D,
		TK_TYPE_ISAMPLER3D,
		TK_TYPE_USAMPLER3D,
		TK_TYPE_SAMPLERCUBE,
		TK_TYPE_SAMPLERCUBEARRAY,
		TK_TYPE_SAMPLEREXT,
		TK_INTERPOLATION_FLAT,
		TK_INTERPOLATION_SMOOTH,
		TK_CONST,
		TK_STRUCT,
		TK_PRECISION_LOW,
		TK_PRECISION_MID,
		TK_PRECISION_HIGH,
		TK_OP_EQUAL,
		TK_OP_NOT_EQUAL,
		TK_OP_LESS,
		TK_OP_LESS_EQUAL,
		TK_OP_GREATER,
		TK_OP_GREATER_EQUAL,
		TK_OP_AND,
		TK_OP_OR,
		TK_OP_NOT,
		TK_OP_ADD,
		TK_OP_SUB,
		TK_OP_MUL,
		TK_OP_DIV,
		TK_OP_MOD,
		TK_OP_SHIFT_LEFT,
		TK_OP_SHIFT_RIGHT,
		TK_OP_ASSIGN,
		TK_OP_ASSIGN_ADD,
		TK_OP_ASSIGN_SUB,
		TK_OP_ASSIGN_MUL,
		TK_OP_ASSIGN_DIV,
		TK_OP_ASSIGN_MOD,
		TK_OP_ASSIGN_SHIFT_LEFT,
		TK_OP_ASSIGN_SHIFT_RIGHT,
		TK_OP_ASSIGN_BIT_AND,
		TK_OP_ASSIGN_BIT_OR,
		TK_OP_ASSIGN_BIT_XOR,
		TK_OP_BIT_AND,
		TK_OP_BIT_OR,
		TK_OP_BIT_XOR,
		TK_OP_BIT_INVERT,
		TK_OP_INCREMENT,
		TK_OP_DECREMENT,
		TK_CF_IF,
		TK_CF_ELSE,
		TK_CF_FOR,
		TK_CF_WHILE,
		TK_CF_DO,
		TK_CF_SWITCH,
		TK_CF_CASE,
		TK_CF_DEFAULT,
		TK_CF_BREAK,
		TK_CF_CONTINUE,
		TK_CF_RETURN,
		TK_CF_DISCARD,
		TK_BRACKET_OPEN,
		TK_BRACKET_CLOSE,
		TK_CURLY_BRACKET_OPEN,
		TK_CURLY_BRACKET_CLOSE,
		TK_PARENTHESIS_OPEN,
		TK_PARENTHESIS_CLOSE,
		TK_QUESTION,
		TK_COMMA,
		TK_COLON,
		TK_SEMICOLON,
		TK_PERIOD,
		TK_UNIFORM,
		TK_UNIFORM_GROUP,
		TK_INSTANCE,
		TK_GLOBAL,
		TK_VARYING,
		TK_ARG_IN,
		TK_ARG_OUT,
		TK_ARG_INOUT,
		TK_RENDER_MODE,
		TK_STENCIL_MODE,
		TK_HINT_DEFAULT_WHITE_TEXTURE,
		TK_HINT_DEFAULT_BLACK_TEXTURE,
		TK_HINT_DEFAULT_TRANSPARENT_TEXTURE,
		TK_HINT_NORMAL_TEXTURE,
		TK_HINT_ROUGHNESS_NORMAL_TEXTURE,
		TK_HINT_ROUGHNESS_R,
		TK_HINT_ROUGHNESS_G,
		TK_HINT_ROUGHNESS_B,
		TK_HINT_ROUGHNESS_A,
		TK_HINT_ROUGHNESS_GRAY,
		TK_HINT_ANISOTROPY_TEXTURE,
		TK_HINT_SOURCE_COLOR,
		TK_HINT_COLOR_CONVERSION_DISABLED,
		TK_HINT_RANGE,
		TK_HINT_ENUM,
		TK_HINT_INSTANCE_INDEX,
		TK_HINT_SCREEN_TEXTURE,
		TK_HINT_NORMAL_ROUGHNESS_TEXTURE,
		TK_HINT_DEPTH_TEXTURE,
		TK_HINT_BLIT_SOURCE0,
		TK_HINT_BLIT_SOURCE1,
		TK_HINT_BLIT_SOURCE2,
		TK_HINT_BLIT_SOURCE3,
		TK_FILTER_NEAREST,
		TK_FILTER_LINEAR,
		TK_FILTER_NEAREST_MIPMAP,
		TK_FILTER_LINEAR_MIPMAP,
		TK_FILTER_NEAREST_MIPMAP_ANISOTROPIC,
		TK_FILTER_LINEAR_MIPMAP_ANISOTROPIC,
		TK_REPEAT_ENABLE,
		TK_REPEAT_DISABLE,
		TK_SHADER_TYPE,
		TK_CURSOR,
		TK_ERROR,
		TK_EOF,
		TK_MAX
	};

/* COMPILER */

// lame work around to Apple defining this as a macro in 10.12 SDK
#ifdef TYPE_BOOL
#undef TYPE_BOOL
#endif

	enum DataType
	{
		TYPE_VOID,
		TYPE_BOOL,
		TYPE_BVEC2,
		TYPE_BVEC3,
		TYPE_BVEC4,
		TYPE_INT,
		TYPE_IVEC2,
		TYPE_IVEC3,
		TYPE_IVEC4,
		TYPE_UINT,
		TYPE_UVEC2,
		TYPE_UVEC3,
		TYPE_UVEC4,
		TYPE_FLOAT,
		TYPE_VEC2,
		TYPE_VEC3,
		TYPE_VEC4,
		TYPE_MAT2,
		TYPE_MAT3,
		TYPE_MAT4,
		TYPE_SAMPLER2D,
		TYPE_ISAMPLER2D,
		TYPE_USAMPLER2D,
		TYPE_SAMPLER2DARRAY,
		TYPE_ISAMPLER2DARRAY,
		TYPE_USAMPLER2DARRAY,
		TYPE_SAMPLER3D,
		TYPE_ISAMPLER3D,
		TYPE_USAMPLER3D,
		TYPE_SAMPLERCUBE,
		TYPE_SAMPLERCUBEARRAY,
		TYPE_SAMPLEREXT,
		TYPE_STRUCT,
		TYPE_MAX
	};

	enum DataPrecision
	{
		PRECISION_LOWP,
		PRECISION_MEDIUMP,
		PRECISION_HIGHP,
		PRECISION_DEFAULT,
	};

	enum DataInterpolation
	{
		INTERPOLATION_FLAT,
		INTERPOLATION_SMOOTH,
		INTERPOLATION_DEFAULT,
	};

	enum Operator
	{
		OP_EQUAL,
		OP_NOT_EQUAL,
		OP_LESS,
		OP_LESS_EQUAL,
		OP_GREATER,
		OP_GREATER_EQUAL,
		OP_AND,
		OP_OR,
		OP_NOT,
		OP_NEGATE,
		OP_ADD,
		OP_SUB,
		OP_MUL,
		OP_DIV,
		OP_MOD,
		OP_SHIFT_LEFT,
		OP_SHIFT_RIGHT,
		OP_ASSIGN,
		OP_ASSIGN_ADD,
		OP_ASSIGN_SUB,
		OP_ASSIGN_MUL,
		OP_ASSIGN_DIV,
		OP_ASSIGN_MOD,
		OP_ASSIGN_SHIFT_LEFT,
		OP_ASSIGN_SHIFT_RIGHT,
		OP_ASSIGN_BIT_AND,
		OP_ASSIGN_BIT_OR,
		OP_ASSIGN_BIT_XOR,
		OP_BIT_AND,
		OP_BIT_OR,
		OP_BIT_XOR,
		OP_BIT_INVERT,
		OP_INCREMENT,
		OP_DECREMENT,
		OP_SELECT_IF,
		OP_SELECT_ELSE, // used only internally, then only IF appears with 3 arguments
		OP_POST_INCREMENT,
		OP_POST_DECREMENT,
		OP_CALL,
		OP_CONSTRUCT,
		OP_STRUCT,
		OP_INDEX,
		OP_EMPTY,
		OP_MAX
	};

	enum FlowOperation
	{
		FLOW_OP_IF,
		FLOW_OP_RETURN,
		FLOW_OP_FOR,
		FLOW_OP_WHILE,
		FLOW_OP_DO,
		FLOW_OP_BREAK,
		FLOW_OP_SWITCH,
		FLOW_OP_CASE,
		FLOW_OP_DEFAULT,
		FLOW_OP_CONTINUE,
		FLOW_OP_DISCARD
	};

	enum ArgumentQualifier
	{
		ARGUMENT_QUALIFIER_IN,
		ARGUMENT_QUALIFIER_OUT,
		ARGUMENT_QUALIFIER_INOUT,
	};

	enum SubClassTag
	{
		TAG_GLOBAL,
		TAG_ARRAY,
	};

	enum TextureFilter
	{
		FILTER_NEAREST,
		FILTER_LINEAR,
		FILTER_NEAREST_MIPMAP,
		FILTER_LINEAR_MIPMAP,
		FILTER_NEAREST_MIPMAP_ANISOTROPIC,
		FILTER_LINEAR_MIPMAP_ANISOTROPIC,
		FILTER_DEFAULT,
	};

	enum TextureRepeat
	{
		REPEAT_DISABLE,
		REPEAT_ENABLE,
		REPEAT_DEFAULT,
	};

	enum
	{
		MAX_INSTANCE_UNIFORM_INDICES = 16
	};

	struct VaryingFunctionNames
	{
		String fragment;
		String vertex;
		String light;

		VaryingFunctionNames()
		{
			fragment = "fragment";
			vertex = "vertex";
			light = "light";
		}
	};

	union Scalar
	{
		bool boolean = false;
		float real;
		int32_t sint;
		uint32_t uint;
	};

	struct Node
	{
		Node* next = nullptr;

		enum Type
		{
			NODE_TYPE_SHADER,
			NODE_TYPE_FUNCTION,
			NODE_TYPE_BLOCK,
			NODE_TYPE_VARIABLE,
			NODE_TYPE_VARIABLE_DECLARATION,
			NODE_TYPE_CONSTANT,
			NODE_TYPE_OPERATOR,
			NODE_TYPE_CONTROL_FLOW,
			NODE_TYPE_MEMBER,
			NODE_TYPE_ARRAY,
			NODE_TYPE_ARRAY_CONSTRUCT,
			NODE_TYPE_STRUCT,
		};

		Type type;

		virtual DataType get_datatype() const { return TYPE_VOID; }

		virtual String get_datatype_name() const { return ""; }

		virtual int get_array_size() const { return 0; }

		virtual bool is_indexed() const { return false; }

		virtual Vector<Scalar> get_values() const { return Vector<Scalar>(); }

		Node(Type t) : type(t) {}

		virtual ~Node() {}
	};

	template <typename T> T* alloc_node()
	{
		T* node = memnew(T);
		node->next = nodes;
		nodes = node;
		return node;
	}

	Node* nodes = nullptr;

	struct OperatorNode : public Node
	{
		DataType return_cache = TYPE_VOID;
		DataPrecision return_precision_cache = PRECISION_DEFAULT;
		int return_array_size = 0;
		Operator op = OP_EQUAL;
		String struct_name;
		Vector<Node*> arguments;
		Vector<Scalar> values;

		virtual DataType get_datatype() const override { return return_cache; }

		virtual String get_datatype_name() const override { return String(struct_name); }

		virtual int get_array_size() const override { return return_array_size; }

		virtual bool is_indexed() const override { return op == OP_INDEX; }

		virtual Vector<Scalar> get_values() const override { return values; }

		OperatorNode() : Node(NODE_TYPE_OPERATOR) {}
	};

	struct VariableNode : public Node
	{
		DataType datatype_cache = TYPE_VOID;
		String name;
		String rname;
		String struct_name;
		bool is_const = false;
		bool is_local = false;

		virtual DataType get_datatype() const override { return datatype_cache; }

		virtual String get_datatype_name() const override { return String(struct_name); }

		VariableNode() : Node(NODE_TYPE_VARIABLE) {}
	};

	struct VariableDeclarationNode : public Node
	{
		DataPrecision precision = PRECISION_DEFAULT;
		DataType datatype = TYPE_VOID;
		String struct_name;
		bool is_const = false;

		struct Declaration
		{
			String name;
			uint32_t size = 0U;
			Node* size_expression = nullptr;
			Vector<Node*> initializer;
			bool single_expression = false;
		};

		Vector<Declaration> declarations;

		virtual DataType get_datatype() const override { return datatype; }

		VariableDeclarationNode() : Node(NODE_TYPE_VARIABLE_DECLARATION) {}
	};

	struct ArrayNode : public Node
	{
		DataType datatype_cache = TYPE_VOID;
		String struct_name;
		String name;
		Node* index_expression = nullptr;
		Node* call_expression = nullptr;
		Node* assign_expression = nullptr;
		bool is_const = false;
		int array_size = 0;
		bool is_local = false;

		virtual DataType get_datatype() const override
		{
			return call_expression ? call_expression->get_datatype() : datatype_cache;
		}

		virtual String get_datatype_name() const override
		{
			return call_expression ? call_expression->get_datatype_name() : String(struct_name);
		}

		virtual int get_array_size() const override
		{
			return (index_expression || call_expression) ? 0 : array_size;
		}

		virtual bool is_indexed() const override { return index_expression != nullptr; }

		ArrayNode() : Node(NODE_TYPE_ARRAY) {}
	};

	struct ArrayConstructNode : public Node
	{
		DataType datatype = TYPE_VOID;
		String struct_name;
		Vector<Node*> initializer;

		virtual DataType get_datatype() const override { return datatype; }

		virtual String get_datatype_name() const override { return struct_name; }

		virtual int get_array_size() const override { return initializer.size(); }

		ArrayConstructNode() : Node(NODE_TYPE_ARRAY_CONSTRUCT) {}
	};

	struct ConstantNode : public Node
	{
		DataType datatype = TYPE_VOID;
		String struct_name = "";
		int array_size = 0;

		Vector<Scalar> values;
		Vector<VariableDeclarationNode::Declaration> array_declarations;

		virtual DataType get_datatype() const override { return datatype; }

		virtual String get_datatype_name() const override { return struct_name; }

		virtual int get_array_size() const override { return array_size; }

		virtual Vector<Scalar> get_values() const override { return values; }

		ConstantNode() : Node(NODE_TYPE_CONSTANT) {}
	};

	struct FunctionNode;

	struct BlockNode : public Node
	{
		FunctionNode* parent_function = nullptr;
		BlockNode* parent_block = nullptr;

		enum BlockType
		{
			BLOCK_TYPE_STANDARD,
			BLOCK_TYPE_FOR_INIT,
			BLOCK_TYPE_FOR_CONDITION,
			BLOCK_TYPE_FOR_EXPRESSION,
			BLOCK_TYPE_SWITCH,
			BLOCK_TYPE_CASE,
			BLOCK_TYPE_DEFAULT,
		};

		int block_type = BLOCK_TYPE_STANDARD;
		SubClassTag block_tag = SubClassTag::TAG_GLOBAL;

		struct Variable
		{
			DataType type;
			String struct_name;
			DataPrecision precision;
			int line; // for completion
			int array_size;
			bool is_const;
			Vector<Scalar> values;
		};

		HashMap<String, Variable> variables;
		List<Node*> statements;
		bool single_statement = false;
		bool use_comma_between_statements = false;
		bool use_op_eval = true;

		DataType expected_type = TYPE_VOID;
		HashSet<int> constants;

		BlockNode() : Node(NODE_TYPE_BLOCK) {}
	};

	struct ControlFlowNode : public Node
	{
		FlowOperation flow_op = FLOW_OP_IF;
		Vector<Node*> expressions;
		Vector<BlockNode*> blocks;

		ControlFlowNode() : Node(NODE_TYPE_CONTROL_FLOW) {}
	};

	struct MemberNode : public Node
	{
		DataType basetype = TYPE_VOID;
		bool basetype_const = false;
		String base_struct_name;
		DataPrecision precision = PRECISION_DEFAULT;
		DataType datatype = TYPE_VOID;
		int array_size = 0;
		String struct_name;
		String name;
		Node* owner = nullptr;
		Node* index_expression = nullptr;
		Node* assign_expression = nullptr;
		Node* call_expression = nullptr;
		bool has_swizzling_duplicates = false;

		virtual DataType get_datatype() const override
		{
			return call_expression ? call_expression->get_datatype() : datatype;
		}

		virtual String get_datatype_name() const override
		{
			return call_expression ? call_expression->get_datatype_name() : String(struct_name);
		}

		virtual int get_array_size() const override
		{
			return (index_expression || call_expression) ? 0 : array_size;
		}

		virtual bool is_indexed() const override
		{
			return index_expression != nullptr || call_expression != nullptr;
		}

		MemberNode() : Node(NODE_TYPE_MEMBER) {}
	};

	struct StructNode : public Node
	{
		List<MemberNode*> members;

		StructNode() : Node(NODE_TYPE_STRUCT) {}
	};

	struct ShaderNode : public Node
	{
		struct Constant
		{
			String name;
			DataType type;
			String struct_name;
			DataPrecision precision;
			Node* initializer = nullptr;
			int array_size;
		};

		struct Function
		{
			String name;
			String rname;
			FunctionNode* function = nullptr;
			HashSet<String> uses_function;
			bool callable;
		};

		struct Struct
		{
			String name;
			StructNode* shader_struct = nullptr;
		};

		struct Varying
		{
			enum Stage
			{
				STAGE_UNKNOWN,
				STAGE_VERTEX,
				STAGE_FRAGMENT,
			};

			Stage stage = STAGE_UNKNOWN;
			DataType type = TYPE_VOID;
			DataInterpolation interpolation = INTERPOLATION_FLAT;
			DataPrecision precision = PRECISION_DEFAULT;
			int array_size = 0;
			TkPos tkpos;

			uint32_t get_size() const
			{
				uint32_t size = 1;
				if (array_size > 0) {
					size = (uint32_t)array_size;
				}

				switch (type) {
				case TYPE_MAT2:
					size *= 2;
					break;
				case TYPE_MAT3:
					size *= 3;
					break;
				case TYPE_MAT4:
					size *= 4;
					break;
				default:
					break;
				}
				return size;
			}

			Varying() {}
		};

		struct Uniform
		{
			enum Hint
			{
				HINT_NONE,
				HINT_RANGE,
				HINT_ENUM,
				HINT_SOURCE_COLOR,
				HINT_COLOR_CONVERSION_DISABLED,
				HINT_NORMAL,
				HINT_ROUGHNESS_NORMAL,
				HINT_ROUGHNESS_R,
				HINT_ROUGHNESS_G,
				HINT_ROUGHNESS_B,
				HINT_ROUGHNESS_A,
				HINT_ROUGHNESS_GRAY,
				HINT_DEFAULT_BLACK,
				HINT_DEFAULT_WHITE,
				HINT_DEFAULT_TRANSPARENT,
				HINT_ANISOTROPY,
				HINT_SCREEN_TEXTURE,
				HINT_BLIT_SOURCE0,
				HINT_BLIT_SOURCE1,
				HINT_BLIT_SOURCE2,
				HINT_BLIT_SOURCE3,
				HINT_NORMAL_ROUGHNESS_TEXTURE,
				HINT_DEPTH_TEXTURE,
				HINT_MAX
			};

			enum Scope
			{
				SCOPE_LOCAL,
				SCOPE_INSTANCE,
				SCOPE_GLOBAL,
			};

			int order = 0;
			int prop_order = 0;
			int texture_order = 0;
			int texture_binding = 0;
			DataType type = TYPE_VOID;
			DataPrecision precision = PRECISION_DEFAULT;
			int array_size = 0;
			Vector<Scalar> default_value;
			Scope scope = SCOPE_LOCAL;
			Hint hint = HINT_NONE;
			bool use_color = false;
			TextureFilter filter = FILTER_DEFAULT;
			TextureRepeat repeat = REPEAT_DEFAULT;
			float hint_range[3];
			PackedStringArray hint_enum_names;
			int instance_index = 0;
			String group;

			_FORCE_INLINE_ bool is_texture() const
			{
				// Order is assigned to -1 for texture uniforms.
				return order < 0;
			}

			Uniform()
			{
				hint_range[0] = 0.0f;
				hint_range[1] = 1.0f;
				hint_range[2] = 0.001f;
			}
		};

		HashMap<String, Constant> constants;
		HashMap<String, Varying> varyings;
		HashMap<String, Uniform> uniforms;
		HashMap<String, Struct> structs;
		HashMap<String, Function> functions;
		Vector<String> render_modes;
		Vector<String> stencil_modes;
		int stencil_reference = -1;

		Vector<Function> vfunctions;
		Vector<Constant> vconstants;
		Vector<Struct> vstructs;

		ShaderNode() : Node(NODE_TYPE_SHADER) {}
	};

	struct FunctionNode : public Node
	{
		struct Argument
		{
			ArgumentQualifier qualifier;
			String name;
			DataType type;
			String struct_name;
			DataPrecision precision;
			// for passing textures as arguments
			bool tex_argument_check;
			TextureFilter tex_argument_filter;
			TextureRepeat tex_argument_repeat;
			bool tex_builtin_check;
			String tex_builtin;
			ShaderNode::Uniform::Hint tex_hint;
			bool is_const;
			int array_size;

			HashMap<String, HashSet<int>> tex_argument_connect;
		};

		String name;
		String rname;
		DataType return_type = TYPE_VOID;
		String return_struct_name;
		DataPrecision return_precision = PRECISION_DEFAULT;
		int return_array_size = 0;
		Vector<Argument> arguments;
		BlockNode* body = nullptr;
		bool can_discard = false;

		virtual DataType get_datatype() const override { return return_type; }

		virtual String get_datatype_name() const override { return String(return_struct_name); }

		virtual int get_array_size() const override { return return_array_size; }

		FunctionNode() : Node(NODE_TYPE_FUNCTION) {}
	};

	struct UniformOrderComparator
	{
		_FORCE_INLINE_ bool operator()(
			const Pair<String, int>& A, const Pair<String, int>& B) const
		{
			return A.second < B.second;
		}
	};

	struct Expression
	{
		bool is_op;

		union
		{
			Operator op;
			Node* node = nullptr;
		};
	};

	struct ExpressionInfo
	{
		Vector<Expression>* expression = nullptr;
		TokenType tt_break = TK_EMPTY;
		bool is_last_expr = false;
	};

	struct VarInfo
	{
		String name;
		DataType type;
	};

	enum CompletionType
	{
		COMPLETION_NONE,
		COMPLETION_SHADER_TYPE,
		COMPLETION_RENDER_MODE,
		COMPLETION_STENCIL_MODE,
		COMPLETION_MAIN_FUNCTION,
		COMPLETION_IDENTIFIER,
		COMPLETION_FUNCTION_CALL,
		COMPLETION_CALL_ARGUMENTS,
		COMPLETION_INDEX,
		COMPLETION_STRUCT,
		COMPLETION_HINT,
	};

	struct Token
	{
		TokenType type;
		String text;
		double constant;
		uint16_t line;

		bool is_integer_constant() const
		{
			return type == TK_INT_CONSTANT || type == TK_UINT_CONSTANT;
		}
	};

	static String get_operator_text(Operator p_op);
	static String get_token_text(Token p_token);

	static bool is_token_datatype(TokenType p_type);
	static bool is_token_variable_datatype(TokenType p_type);
	static DataType get_token_datatype(TokenType p_type);
	static bool is_token_interpolation(TokenType p_type);
	static DataInterpolation get_token_interpolation(TokenType p_type);
	static bool is_token_precision(TokenType p_type);
	static bool is_token_arg_qual(TokenType p_type);
	static DataPrecision get_token_precision(TokenType p_type);
	static String get_precision_name(DataPrecision p_type);
	static String get_interpolation_name(DataInterpolation p_interpolation);
	static String get_datatype_name(DataType p_type);
	static String get_uniform_hint_name(ShaderNode::Uniform::Hint p_hint);
	static String get_texture_filter_name(TextureFilter p_filter);
	static String get_texture_repeat_name(TextureRepeat p_repeat);
	static bool is_token_nonvoid_datatype(TokenType p_type);
	static bool is_token_operator(TokenType p_type);
	static bool is_token_operator_assign(TokenType p_type);
	static bool is_token_hint(TokenType p_type);

	static bool convert_constant(
		ConstantNode* p_constant, DataType p_to_type, Scalar* p_value = nullptr);
	static DataType get_scalar_type(DataType p_type);
	static int get_cardinality(DataType p_type);
	static bool is_scalar_type(DataType p_type);
	static bool is_float_type(DataType p_type);
	static bool is_sampler_type(DataType p_type);
	static bool is_hint_color(ShaderNode::Uniform::Hint p_hint);
	static uint32_t get_datatype_size(DataType p_type);
	static uint32_t get_datatype_component_count(DataType p_type);

	static void get_keyword_list(List<String>* r_keywords);
	static bool is_control_flow_keyword(String p_keyword);
	static void get_builtin_funcs(List<String>* r_keywords);

	static SafeNumeric<int> instance_counter;

	struct BuiltInInfo
	{
		DataType type = TYPE_VOID;
		bool constant = false;
		Vector<Scalar> values;

		BuiltInInfo() {}

		BuiltInInfo(DataType p_type, bool p_constant = false, const Vector<Scalar>& p_values = {})
			: type(p_type), constant(p_constant), values(p_values)
		{
		}
	};

	struct StageFunctionInfo
	{
		struct Argument
		{
			String name;
			DataType type;

			Argument(const String& p_name = String(), DataType p_type = TYPE_VOID)
			{
				name = p_name;
				type = p_type;
			}
		};

		Vector<Argument> arguments;
		DataType return_type = TYPE_VOID;
		String skip_function;
	};

	struct ModeInfo
	{
		String name;
		Vector<String> options;

		ModeInfo() {}

		ModeInfo(const String& p_name) : name(p_name) {}

		ModeInfo(const String& p_name, const String& p_arg1, const String& p_arg2)
			: name(p_name)
		{
			options.push_back(p_arg1);
			options.push_back(p_arg2);
		}

		ModeInfo(const String& p_name, const String& p_arg1, const String& p_arg2,
			const String& p_arg3)
			: name(p_name)
		{
			options.push_back(p_arg1);
			options.push_back(p_arg2);
			options.push_back(p_arg3);
		}

		ModeInfo(const String& p_name, const String& p_arg1, const String& p_arg2,
			const String& p_arg3, const String& p_arg4)
			: name(p_name)
		{
			options.push_back(p_arg1);
			options.push_back(p_arg2);
			options.push_back(p_arg3);
			options.push_back(p_arg4);
		}

		ModeInfo(const String& p_name, const String& p_arg1, const String& p_arg2,
			const String& p_arg3, const String& p_arg4, const String& p_arg5)
			: name(p_name)
		{
			options.push_back(p_arg1);
			options.push_back(p_arg2);
			options.push_back(p_arg3);
			options.push_back(p_arg4);
			options.push_back(p_arg5);
		}

		ModeInfo(const String& p_name, const String& p_arg1, const String& p_arg2,
			const String& p_arg3, const String& p_arg4, const String& p_arg5,
			const String& p_arg6)
			: name(p_name)
		{
			options.push_back(p_arg1);
			options.push_back(p_arg2);
			options.push_back(p_arg3);
			options.push_back(p_arg4);
			options.push_back(p_arg5);
			options.push_back(p_arg6);
		}

		ModeInfo(const String& p_name, std::initializer_list<String> p_args) : name(p_name)
		{
			for (const String& arg : p_args) {
				options.push_back(arg);
			}
		}
	};

	struct FunctionInfo
	{
		HashMap<String, BuiltInInfo> built_ins;
		HashMap<String, StageFunctionInfo> stage_functions;

		bool can_discard = false;
		bool main_function = false;
	};

	static bool has_builtin(const HashMap<String, ShaderLanguage::FunctionInfo>& p_functions,
		const String& p_name, bool p_check_global_funcs = false);

	typedef DataType (*GlobalShaderUniformGetTypeFunc)(const String& p_name);

	struct FilePosition
	{
		String file;
		int line = 0;
	};

private:
	struct KeyWord
	{
		TokenType token;
		const char* text;
		uint32_t flags;
		const Vector<String> excluded_shader_types;
		const Vector<String> excluded_functions;
	};

	static const KeyWord keyword_list[];

	GlobalShaderUniformGetTypeFunc global_shader_uniform_get_type_func = nullptr;

	bool error_set = false;
	String error_str;
	int error_line = 0;

	Vector<FilePosition> include_positions;
	HashSet<String> include_markers_handled;
	HashMap<String, int> function_overload_count;

	// Additional function information (eg. call hierarchy). No need to expose it to compiler.
	struct CallInfo
	{
		struct Item
		{
			enum ItemType
			{
				ITEM_TYPE_BUILTIN,
				ITEM_TYPE_VARYING,
			} type;

			TkPos pos;

			Item() {}

			Item(ItemType p_type, TkPos p_pos) : type(p_type), pos(p_pos) {}
		};

		String name;
		List<Pair<String, Item>> uses_restricted_items;
		List<CallInfo*> calls;
	};

	RBMap<String, CallInfo> calls_info;

#ifdef DEBUG_ENABLED
	struct Usage
	{
		int decl_line;
		bool used = false;

		Usage(int p_decl_line = -1) { decl_line = p_decl_line; }
	};

	HashMap<String, Usage> used_constants;
	HashMap<String, Usage> used_varyings;
	HashMap<String, Usage> used_uniforms;
	HashMap<String, Usage> used_functions;
	HashMap<String, Usage> used_structs;
	HashMap<ShaderWarning::Code, HashMap<String, Usage>*> warnings_check_map;

	HashMap<String, HashMap<String, Usage>> used_local_vars;
	HashMap<ShaderWarning::Code, HashMap<String, HashMap<String, Usage>>*>
		warnings_check_map2;

	List<ShaderWarning> warnings;

	bool check_warnings = false;
	uint32_t warning_flags = 0;
	void _check_warning_accums();
#endif // DEBUG_ENABLED

	String code;
	int char_idx = 0;
	int tk_line = 0;

	String shader_type_identifier;
	String current_function;
	bool is_const_decl = false;
	String last_name;
	bool is_shader_inc = false;

	String current_uniform_group_name;

	VaryingFunctionNames varying_function_names;
	uint32_t base_varying_index = 0;

	TkPos _get_tkpos()
	{
		TkPos tkp;
		tkp.char_idx = char_idx;
		tkp.tk_line = tk_line;
		return tkp;
	}

	void _set_tkpos(TkPos p_pos)
	{
		char_idx = p_pos.char_idx;
		tk_line = p_pos.tk_line;
	}

	void _set_error(const String& p_str)
	{
		if (error_set) {
			return;
		}

		error_line = tk_line;
		error_set = true;
		error_str = p_str;
		include_positions.write[include_positions.size() - 1].line = tk_line;
	}

	void _set_expected_error(const String& p_what)
	{
		_set_error(vformat(RTR("Expected a '%s'."), p_what));
	}

	void _set_expected_error(const String& p_first, const String p_second)
	{
		_set_error(vformat(RTR("Expected a '%s' or '%s'."), p_first, p_second));
	}

	void _set_expected_after_error(const String& p_what, const String& p_after)
	{
		_set_error(vformat(RTR("Expected a '%s' after '%s'."), p_what, p_after));
	}

	void _set_redefinition_error(const String& p_what)
	{
		_set_error(vformat(RTR("Redefinition of '%s'."), p_what));
	}

	void _set_parsing_error() { _set_error("Parser bug."); }

	static const char* token_names[TK_MAX];

	Token _make_token(TokenType p_type, const String& p_text = String());
	Token _get_token();
	bool _lookup_next(Token& r_tk);
	Token _peek();

	ShaderNode* shader = nullptr;

	enum IdentifierType
	{
		IDENTIFIER_FUNCTION,
		IDENTIFIER_UNIFORM,
		IDENTIFIER_VARYING,
		IDENTIFIER_FUNCTION_ARGUMENT,
		IDENTIFIER_LOCAL_VAR,
		IDENTIFIER_BUILTIN_VAR,
		IDENTIFIER_CONSTANT,
		IDENTIFIER_MAX,
	};

	IdentifierType last_type = IDENTIFIER_MAX;

	bool _find_identifier(const BlockNode* p_block, bool p_allow_reassign,
		const FunctionInfo& p_function_info, const String& p_identifier,
		DataType* r_data_type = nullptr, IdentifierType* r_type = nullptr,
		bool* r_is_const = nullptr, int* r_array_size = nullptr,
		String* r_struct_name = nullptr, Vector<Scalar>* r_constant_values = nullptr);
#ifdef DEBUG_ENABLED
	void _parse_used_identifier(
		const String& p_identifier, IdentifierType p_type, const String& p_function);
#endif // DEBUG_ENABLED
	bool _is_operator_assign(Operator p_op) const;
	bool _validate_assign(
		Node* p_node, const FunctionInfo& p_function_info, String* r_message = nullptr);
	bool _validate_operator(const BlockNode* p_block, const FunctionInfo& p_function_info,
		OperatorNode* p_op, DataType* r_ret_type = nullptr, int* r_ret_size = nullptr,
		String* r_ret_struct_name = nullptr);

	Vector<Scalar> _get_node_values(
		const BlockNode* p_block, const FunctionInfo& p_function_info, Node* p_node);
	bool _eval_operator(
		const BlockNode* p_block, const FunctionInfo& p_function_info, OperatorNode* p_op);
	Scalar _eval_unary_scalar(const Scalar& p_a, Operator p_op, DataType p_ret_type);
	Scalar _eval_scalar(
		const Scalar& p_a, const Scalar& p_b, Operator p_op, DataType p_ret_type, bool& r_is_valid);
	Vector<Scalar> _eval_unary_vector(
		const Vector<Scalar>& p_va, DataType p_ret_type, Operator p_op);
	Vector<Scalar> _eval_vector(const Vector<Scalar>& p_va, const Vector<Scalar>& p_vb,
		DataType p_left_type, DataType p_right_type, DataType p_ret_type, Operator p_op,
		bool& r_is_valid);
	Vector<Scalar> _eval_vector_transform(const Vector<Scalar>& p_va, const Vector<Scalar>& p_vb,
		DataType p_left_type, DataType p_right_type, DataType p_ret_type);

	struct BuiltinEntry
	{
		const char* name;
	};

	struct BuiltinFuncDef
	{
		enum
		{
			MAX_ARGS = 5
		};

		const char* name;
		DataType rettype;
		const DataType args[MAX_ARGS];
		const char* args_names[MAX_ARGS];
		SubClassTag tag;
		bool high_end;
	};

	struct BuiltinFuncOutArgs
	{ // arguments used as out in built in functions

		enum
		{
			MAX_ARGS = 2
		};

		const char* name;
		const int arguments[MAX_ARGS];
	};

	struct BuiltinFuncConstArgs
	{
		const char* name;
		int arg;
		int min;
		int max;
	};

	CompletionType completion_type;
	ShaderNode::Uniform::Hint current_uniform_hint = ShaderNode::Uniform::HINT_NONE;
	TextureFilter current_uniform_filter = FILTER_DEFAULT;
	TextureRepeat current_uniform_repeat = REPEAT_DEFAULT;
	bool current_uniform_instance_index_defined = false;
	int completion_line = 0;
	BlockNode* completion_block = nullptr;
	DataType completion_base;
	bool completion_base_array = false;
	SubClassTag completion_class;
	String completion_function;
	String completion_struct;
	int completion_argument = 0;

#ifdef DEBUG_ENABLED
	uint32_t keyword_completion_context;
#endif // DEBUG_ENABLED

	const HashMap<String, FunctionInfo>* stages = nullptr;
	bool is_supported_frag_only_funcs = false;
	bool is_discard_supported = false;

	bool _get_completable_identifier(
		BlockNode* p_block, CompletionType p_type, String& identifier);
	static const BuiltinFuncDef builtin_func_defs[];
	static const BuiltinFuncOutArgs builtin_func_out_args[];
	static const BuiltinFuncConstArgs builtin_func_const_args[];
	static const BuiltinEntry frag_only_func_defs[];

	static bool is_const_suffix_lut_initialized;

	Error _validate_precision(DataType p_type, DataPrecision p_precision);
	bool _compare_datatypes(DataType p_datatype_a, String p_datatype_name_a, int p_array_size_a,
		DataType p_datatype_b, String p_datatype_name_b, int p_array_size_b);
	bool _compare_datatypes_in_nodes(Node* a, Node* b);

	bool _validate_function_call(BlockNode* p_block, const FunctionInfo& p_function_info,
		OperatorNode* p_func, DataType* r_ret_type, String* r_ret_type_str,
		bool* r_is_custom_function = nullptr);
	bool _parse_function_arguments(BlockNode* p_block, const FunctionInfo& p_function_info,
		OperatorNode* p_func, int* r_complete_arg = nullptr);
	ShaderNode::Uniform::Hint _sanitize_hint(ShaderNode::Uniform::Hint p_hint);
	bool _propagate_function_call_sampler_uniform_settings(const String& p_name, int p_argument,
		TextureFilter p_filter, TextureRepeat p_repeat, ShaderNode::Uniform::Hint p_hint);
	bool _propagate_function_call_sampler_builtin_reference(
		const String& p_name, int p_argument, const String& p_builtin);
	bool _validate_varying_assign(ShaderNode::Varying& p_varying, String* r_message);
	bool _check_node_constness(const Node* p_node) const;

	bool _check_restricted_func(
		const String& p_name, const String& p_current_function) const;
	bool _validate_restricted_func(
		const String& p_call_name, const CallInfo* p_func_info, bool p_is_builtin_hint = false);

	Node* _parse_expression(BlockNode* p_block, const FunctionInfo& p_function_info,
		const ExpressionInfo* p_previous_expression_info = nullptr);
	Error _parse_array_size(BlockNode* p_block, const FunctionInfo& p_function_info,
		bool p_forbid_unknown_size, Node** r_size_expression, int* r_array_size,
		bool* r_unknown_size);
	Node* _parse_array_constructor(BlockNode* p_block, const FunctionInfo& p_function_info);
	Node* _parse_array_constructor(BlockNode* p_block, const FunctionInfo& p_function_info,
		DataType p_type, const String& p_struct_name, int p_array_size);
	ShaderLanguage::Node* _reduce_expression(BlockNode* p_block, ShaderLanguage::Node* p_node);

	Node* _parse_and_reduce_expression(BlockNode* p_block, const FunctionInfo& p_function_info,
		const ExpressionInfo* p_previous_expression_info = nullptr);
	Error _parse_block(BlockNode* p_block, const FunctionInfo& p_function_info,
		bool p_just_one = false, bool p_can_break = false, bool p_can_continue = false);
	String _get_shader_type_list(const HashSet<String>& p_shader_types) const;
	String _get_qualifier_str(ArgumentQualifier p_qualifier) const;

	bool _parse_numeric_constant_expression(const FunctionInfo& p_function_info, float& r_constant);
	Error _parse_shader(const HashMap<String, FunctionInfo>& p_functions,
		const Vector<ModeInfo>& p_render_modes, const Vector<ModeInfo>& p_stencil_modes,
		const HashSet<String>& p_shader_types);

	Error _find_last_flow_op_in_block(BlockNode* p_block, FlowOperation p_op);
	Error _find_last_flow_op_in_op(ControlFlowNode* p_flow, FlowOperation p_op);

	Error _parse_shader_mode(bool p_is_stencil, const Vector<ModeInfo>& p_modes,
		HashMap<String, String>& r_defined_modes);

public:
#ifdef DEBUG_ENABLED
	List<ShaderWarning>::Element* get_warnings_ptr();

	void enable_warning_checking(bool p_enabled);
	bool is_warning_checking_enabled() const;

	void set_warning_flags(uint32_t p_flags);
	uint32_t get_warning_flags() const;
#endif // DEBUG_ENABLED

	// static void get_keyword_list(ShaderType p_type,List<String> *p_keywords);

	static void finish();
	void clear();

	static String get_shader_type(const String& p_code);
	static bool is_builtin_func_out_parameter(const String& p_name, int p_param);

	struct ShaderCompileInfo
	{
		HashMap<String, FunctionInfo> functions;
		Vector<ModeInfo> render_modes;
		Vector<ModeInfo> stencil_modes;
		VaryingFunctionNames varying_function_names;
		HashSet<String> shader_types;
		GlobalShaderUniformGetTypeFunc global_shader_uniform_type_func = nullptr;
		bool is_include = false;
		uint32_t base_varying_index = 0;
	};

	Error compile(const String& p_code, const ShaderCompileInfo& p_info);

	String get_error_text();
	Vector<FilePosition> get_include_positions();

	CompletionType get_completion_type() const { return completion_type; }

	int get_error_line();

	ShaderNode* get_shader();

	String token_debug(const String& p_code);

	ShaderLanguage();
	~ShaderLanguage();
};

// --- Folded from servers/rendering/shader_compiler.h ---
#include "core/templates/pair.h"
#include "servers/rendering/rendering_server_enums.h"


class ShaderCompiler final
{
public:
	enum Stage
	{
		STAGE_VERTEX,
		STAGE_FRAGMENT,
		STAGE_COMPUTE,
		STAGE_RAYGEN,
		STAGE_ANY_HIT,
		STAGE_CLOSEST_HIT,
		STAGE_MISS,
		STAGE_INTERSECTION,
		STAGE_MAX
	};

	struct IdentifierActions
	{
		HashMap<String, Stage> entry_point_stages;

		HashMap<String, Pair<int*, int>> render_mode_values;
		HashMap<String, bool*> render_mode_flags;
		HashMap<String, bool*> usage_flag_pointers;
		HashMap<String, bool*> write_flag_pointers;
		HashMap<String, Pair<int*, int>> stencil_mode_values;
		int* stencil_reference = nullptr;

		HashMap<String, ShaderLanguage::ShaderNode::Uniform>* uniforms = nullptr;
	};

	struct GeneratedCode
	{
		Vector<String> defines;

		struct Texture
		{
			String name;
			ShaderLanguage::DataType type = ShaderLanguage::DataType::TYPE_VOID;
			ShaderLanguage::ShaderNode::Uniform::Hint hint =
				ShaderLanguage::ShaderNode::Uniform::Hint::HINT_NONE;
			bool use_color = false;
			ShaderLanguage::TextureFilter filter = ShaderLanguage::TextureFilter::FILTER_DEFAULT;
			ShaderLanguage::TextureRepeat repeat = ShaderLanguage::TextureRepeat::REPEAT_DEFAULT;
			bool global = false;
			int array_size = 0;
		};

		Vector<Texture> texture_uniforms;

		Vector<uint32_t> uniform_offsets;
		uint32_t uniform_total_size = 0;
		String uniforms;
		String stage_globals[STAGE_MAX];

		HashMap<String, String> code;

		bool uses_global_textures = false;
		bool uses_fragment_time = false;
		bool uses_vertex_time = false;
		bool uses_screen_texture_mipmaps = false;
		bool uses_screen_texture = false;
		bool uses_depth_texture = false;
		bool uses_normal_roughness_texture = false;
	};

	struct DefaultIdentifierActions
	{
		HashMap<String, String> renames;
		HashMap<String, String> render_mode_defines;
		HashMap<String, String> usage_defines;
		HashMap<String, String> custom_samplers;
		ShaderLanguage::TextureFilter default_filter =
			ShaderLanguage::TextureFilter::FILTER_NEAREST;
		ShaderLanguage::TextureRepeat default_repeat =
			ShaderLanguage::TextureRepeat::REPEAT_DISABLE;
		int base_texture_binding_index = 0;
		int texture_layout_set = 0;
		String base_uniform_string;
		String global_buffer_array_variable;
		String instance_uniform_index_variable;
		uint32_t base_varying_index = 0;
		bool apply_luminance_multiplier = false;
		bool check_multiview_samplers = false;
	};

private:
	ShaderLanguage parser;

	String _get_sampler_name(
		ShaderLanguage::TextureFilter p_filter, ShaderLanguage::TextureRepeat p_repeat);

	void _dump_function_deps(const ShaderLanguage::ShaderNode* p_node, const String& p_for_func,
		const HashMap<String, String>& p_func_code, String& r_to_add,
		HashSet<String>& added);
	String _dump_node_code(const ShaderLanguage::Node* p_node, int p_level,
		GeneratedCode& r_gen_code, IdentifierActions& p_actions,
		const DefaultIdentifierActions& p_default_actions, bool p_assigning, bool p_scope = true);

	const ShaderLanguage::ShaderNode* shader = nullptr;
	const ShaderLanguage::FunctionNode* function = nullptr;
	String current_func_name;
	String time_name;
	HashSet<String> texture_functions;

	HashSet<String> used_name_defines;
	HashSet<String> used_flag_pointers;
	HashSet<String> used_rmode_defines;
	HashSet<String> internal_functions;
	HashSet<String> fragment_varyings;

	DefaultIdentifierActions actions;

	static ShaderLanguage::DataType _get_global_shader_uniform_type(const String& p_name);

public:
	Error compile(RSE::ShaderMode p_mode, const String& p_code, IdentifierActions* p_actions,
		const String& p_path, GeneratedCode& r_gen_code);

	void initialize(DefaultIdentifierActions p_actions);
	ShaderCompiler();
};

// --- Folded from servers/rendering/shader_preprocessor.h ---
#include "core/string/ustring.h"
#include "core/templates/list.h"
#include "core/templates/local_vector.h"
#include "core/templates/rb_map.h"
#include "core/templates/rb_set.h"
#include "scene/resources/material.h"

class ShaderPreprocessor
{
public:
	enum CompletionType
	{
		COMPLETION_TYPE_NONE,
		COMPLETION_TYPE_DIRECTIVE,
		COMPLETION_TYPE_PRAGMA_DIRECTIVE,
		COMPLETION_TYPE_PRAGMA,
		COMPLETION_TYPE_CONDITION,
		COMPLETION_TYPE_INCLUDE_PATH,
	};

	struct FilePosition
	{
		String file;
		int line = 0;
	};

	struct Region
	{
		String file;
		int from_line = -1;
		int to_line = -1;
		bool enabled = false;
		Region* parent = nullptr;
	};

private:
	struct Token
	{
		char32_t text;
		int line;

		Token();
		Token(char32_t p_text, int p_line);
	};

	// The real preprocessor that understands basic shader and preprocessor language syntax.
	class Tokenizer
	{
	public:
		String code;
		int line;
		int index;
		int size;
		LocalVector<Token> generated;

	private:
		void add_generated(const Token& p_t);
		char32_t next();

	public:
		int get_line() const;
		int get_index() const;
		char32_t peek();
		int consume_line_continuations(int p_offset);

		void get_and_clear_generated(LocalVector<char32_t>* r_out);
		void backtrack(char32_t p_what);
		LocalVector<Token> advance(char32_t p_what);
		void skip_whitespace();
		bool consume_empty_line();
		String get_identifier(bool* r_is_cursor = nullptr, bool p_started = false);
		String peek_identifier();
		Token get_token();

		Tokenizer(const String& p_code);
	};

	class CommentRemover
	{
	private:
		LocalVector<char32_t> stripped;
		String code;
		int index;
		int line;
		int comment_line_open;
		int comments_open;
		int strings_open;

	public:
		String get_error() const;
		int get_error_line() const;
		char32_t peek() const;

		bool advance(char32_t p_what);
		String strip();

		CommentRemover(const String& p_code);
	};

	struct Define
	{
		Vector<String> arguments;
		String body;
		bool is_builtin = false;
	};

	struct Branch
	{
		Vector<bool> conditions;
		Branch* parent = nullptr;
		bool else_defined = false;

		Branch() {}

		Branch(bool p_condition, Branch* p_parent) : parent(p_parent)
		{
			conditions.push_back(p_condition);
		}
	};

	struct State
	{
		RBMap<String, Define*> defines;
		List<Branch> branches;
		Branch* current_branch = nullptr;
		int condition_depth = 0;
		RBSet<String> includes;
		List<uint64_t> cyclic_include_hashes; // Holds code hash of includes.
		int include_depth = 0;
		String current_filename;
		String current_shader_type;
		String error;
		List<FilePosition> include_positions;
		bool save_regions = false;
		RBMap<String, List<Region>> regions;
		Region* previous_region = nullptr;
		bool disabled = false;
		CompletionType completion_type = COMPLETION_TYPE_NONE;
		HashSet<Ref<ShaderInclude>> shader_includes;
		bool completion_show_defines = false;
	};

private:
	LocalVector<char32_t> output;
	State* state = nullptr;

private:
	static bool is_char_word(char32_t p_char);
	static bool is_char_space(char32_t p_char);
	static bool is_char_end(char32_t p_char);
	static String vector_to_string(
		const LocalVector<char32_t>& p_v, int p_start = 0, int p_end = -1);
	static String tokens_to_string(const LocalVector<Token>& p_tokens);

	void _set_expected_error(const String& p_what, int p_line)
	{
		set_error(vformat(RTR("Expected a '%s'."), p_what), p_line);
	}

	void _set_unexpected_token_error(const String& p_what, int p_line)
	{
		set_error(vformat(RTR("Unexpected token: '%s'."), p_what), p_line);
	}

	void process_directive(Tokenizer* p_tokenizer);
	void process_define(Tokenizer* p_tokenizer);
	void process_elif(Tokenizer* p_tokenizer);
	void process_else(Tokenizer* p_tokenizer);
	void process_endif(Tokenizer* p_tokenizer);
	void process_error(Tokenizer* p_tokenizer);
	void process_if(Tokenizer* p_tokenizer);
	void process_ifdef(Tokenizer* p_tokenizer);
	void process_ifndef(Tokenizer* p_tokenizer);
	void process_include(Tokenizer* p_tokenizer);
	void process_pragma(Tokenizer* p_tokenizer);
	void process_undef(Tokenizer* p_tokenizer);

	void add_region(int p_line, bool p_enabled, Region* p_parent_region);
	void start_branch_condition(Tokenizer* p_tokenizer, bool p_success, bool p_continue = false);

	Error expand_condition(const String& p_string, int p_line, String& r_result);
	void expand_output_macros(int p_start, int p_line);
	Error expand_macros(const String& p_string, int p_line, String& r_result);
	bool expand_macros_once(const String& p_line, int p_line_number,
		const RBMap<String, Define*>::Element* p_define_pair, String& r_expanded);
	bool find_match(
		const String& p_string, const String& p_value, int& r_index, int& r_index_start);
	void concatenate_macro_body(String& r_body);

	String next_directive(Tokenizer* p_tokenizer, const Vector<String>& p_directives);
	void add_to_output(const String& p_str);
	void set_error(const String& p_error, int p_line);

	static Define* create_define(const String& p_body);
	void insert_builtin_define(String p_name, String p_value, State& p_state);

	void clear_state();

	Error preprocess(State* p_state, const String& p_code, String& r_result);

public:
	static void get_keyword_list(List<String>* r_keywords, bool p_include_shader_keywords,
		bool p_ignore_context_keywords = false);
	static void get_pragma_list(List<String>* r_pragmas);

	ShaderPreprocessor() = default;
	~ShaderPreprocessor() = default;
};



// --- End servers/rendering/shader_preprocessor.h ---

// --- Folded from servers/rendering/shader_types.h ---
#include "servers/rendering/rendering_server_enums.h"


class ShaderTypes
{
public:
    ShaderTypes() = delete;
    ~ShaderTypes() = delete;

    struct Type
    {
        HashMap<String, ShaderLanguage::FunctionInfo> functions;
        Vector<ShaderLanguage::ModeInfo> modes;
        Vector<ShaderLanguage::ModeInfo> stencil_modes;
    };

    static inline HashMap<RSE::ShaderMode, Type> shader_modes;
    static inline HashSet<String> shader_types;
    static inline List<String> shader_types_list;

    static const HashMap<String, ShaderLanguage::FunctionInfo>& get_functions(
        RSE::ShaderMode p_mode);
    static const Vector<ShaderLanguage::ModeInfo>& get_modes(RSE::ShaderMode p_mode);
    static const Vector<ShaderLanguage::ModeInfo>& get_stencil_modes(RSE::ShaderMode p_mode);
    static const HashSet<String>& get_types();
    static const List<String>& get_types_list();

    static void initialize();
    static void finalize();
};


// --- End servers/rendering/shader_types.h ---

// --- Folded from servers/rendering/shader_include_db.h ---
#include "core/types.h"

class ShaderIncludeDB
{
private:
	static HashMap<String, String> built_in_includes;

protected:

public:
	static void register_built_in_include_file(
		const String& p_filename, const String& p_shader_code);
	static PackedStringArray list_built_in_include_files();
	static bool has_built_in_include_file(const String& p_filename);
	static String get_built_in_include_file(const String& p_filename);
	static String parse_include_files(const String& p_code);
};



// --- End servers/rendering/shader_include_db.h ---
// --- End servers/rendering/shader_compiler.h ---
