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

#include "gltf_object_model_property.h"

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


