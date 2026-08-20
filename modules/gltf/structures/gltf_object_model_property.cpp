/**************************************************************************/
/*  gltf_object_model_property.cpp                                        */
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

#include "../gltf_template_convert.h"
#include "core/object/class_db.h"
#include "gltf_object_model_property.h"

void GLTFObjectModelProperty::_bind_methods() {}

void GLTFObjectModelProperty::append_node_path(const NodePath& p_node_path)
{
	node_paths.push_back(p_node_path);
}

void GLTFObjectModelProperty::append_path_to_property(
	const NodePath& p_node_path, const StringName& p_prop_name)
{
	Vector<StringName> node_names = p_node_path.get_names();
	Vector<StringName> subpath = p_node_path.get_subnames();
	subpath.append(p_prop_name);
	node_paths.push_back(NodePath(node_names, subpath, false));
}

GLTFAccessor::GLTFAccessorType GLTFObjectModelProperty::get_accessor_type() const
{
	switch (object_model_type) {
	case GLTF_OBJECT_MODEL_TYPE_FLOAT2:
		return GLTFAccessor::TYPE_VEC2;
	case GLTF_OBJECT_MODEL_TYPE_FLOAT3:
		return GLTFAccessor::TYPE_VEC3;
	case GLTF_OBJECT_MODEL_TYPE_FLOAT4:
		return GLTFAccessor::TYPE_VEC4;
	case GLTF_OBJECT_MODEL_TYPE_FLOAT2X2:
		return GLTFAccessor::TYPE_MAT2;
	case GLTF_OBJECT_MODEL_TYPE_FLOAT3X3:
		return GLTFAccessor::TYPE_MAT3;
	case GLTF_OBJECT_MODEL_TYPE_FLOAT4X4:
		return GLTFAccessor::TYPE_MAT4;
	default:
		return GLTFAccessor::TYPE_SCALAR;
	}
}

GLTFAccessor::GLTFComponentType GLTFObjectModelProperty::get_component_type(
	const Vector<Variant>& p_values) const
{
	switch (object_model_type) {
	case GLTFObjectModelProperty::GLTF_OBJECT_MODEL_TYPE_BOOL: {
		return GLTFAccessor::COMPONENT_TYPE_UNSIGNED_BYTE;
	} break;
	case GLTFObjectModelProperty::GLTF_OBJECT_MODEL_TYPE_INT: {
		PackedInt64Array int_values;
		for (int i = 0; i < p_values.size(); i++) {
			int_values.append(p_values[i]);
		}
		return GLTFAccessor::get_minimal_integer_component_type_from_ints(int_values);
	} break;
	default: {
		// The base glTF specification only supports 32-bit float accessors for floating point data.
		return GLTFAccessor::COMPONENT_TYPE_SINGLE_FLOAT;
	} break;
	}
}

Ref<Expression> GLTFObjectModelProperty::get_gltf_to_godot_expression() const
{
	return gltf_to_godot_expr;
}

void GLTFObjectModelProperty::set_gltf_to_godot_expression(
	const Ref<Expression>& p_gltf_to_godot_expr)
{
	gltf_to_godot_expr = p_gltf_to_godot_expr;
}

Ref<Expression> GLTFObjectModelProperty::get_godot_to_gltf_expression() const
{
	return godot_to_gltf_expr;
}

void GLTFObjectModelProperty::set_godot_to_gltf_expression(
	const Ref<Expression>& p_godot_to_gltf_expr)
{
	godot_to_gltf_expr = p_godot_to_gltf_expr;
}

TypedArray<NodePath> GLTFObjectModelProperty::get_node_paths() const
{
	return TypedArray<NodePath>(node_paths);
}

bool GLTFObjectModelProperty::has_node_paths() const { return !node_paths.is_empty(); }

void GLTFObjectModelProperty::set_node_paths(const TypedArray<NodePath>& p_node_paths)
{
	node_paths = TypedArray<NodePath>(p_node_paths);
}

GLTFObjectModelProperty::GLTFObjectModelType GLTFObjectModelProperty::get_object_model_type() const
{
	return object_model_type;
}

void GLTFObjectModelProperty::set_object_model_type(GLTFObjectModelType p_type)
{
	object_model_type = p_type;
}

Vector<PackedStringArray> GLTFObjectModelProperty::get_json_pointers() const
{
	return json_pointers;
}

bool GLTFObjectModelProperty::has_json_pointers() const { return !json_pointers.is_empty(); }

void GLTFObjectModelProperty::set_json_pointers(const Vector<PackedStringArray>& p_json_pointers)
{
	json_pointers = p_json_pointers;
}

TypedArray<PackedStringArray> GLTFObjectModelProperty::get_json_pointers_bind() const
{
	return GLTFTemplateConvert::to_array(json_pointers);
}

void GLTFObjectModelProperty::set_json_pointers_bind(
	const TypedArray<PackedStringArray>& p_json_pointers)
{
	GLTFTemplateConvert::set_from_array(json_pointers, p_json_pointers);
}

Variant::Type GLTFObjectModelProperty::get_variant_type() const { return variant_type; }

void GLTFObjectModelProperty::set_variant_type(Variant::Type p_variant_type)
{
	variant_type = p_variant_type;
}

void GLTFObjectModelProperty::set_types(
	Variant::Type p_variant_type, GLTFObjectModelType p_obj_model_type)
{
	variant_type = p_variant_type;
	object_model_type = p_obj_model_type;
}


