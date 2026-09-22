/**************************************************************************/
/*  gltf_document_extension.cpp                                           */
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

#include "gltf_document_extension.h"

// Import process.
Error GLTFDocumentExtension::import_preflight(
	Ref<GLTFState> p_state, const Vector<String>& p_extensions)
{
	ERR_FAIL_COND_V(p_state.is_null(), ERR_INVALID_PARAMETER);
	Error err = OK;
	return err;
}

Error GLTFDocumentExtension::parse_image_data(Ref<GLTFState> p_state,
	const PackedByteArray& p_image_data, const String& p_mime_type, Ref<Image> r_image)
{
	ERR_FAIL_COND_V(p_state.is_null(), ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(r_image.is_null(), ERR_INVALID_PARAMETER);
	Error err = OK;
	return err;
}

Ref<GLTFObjectModelProperty> GLTFDocumentExtension::import_object_model_property(
	Ref<GLTFState> p_state, const PackedStringArray& p_split_json_pointer,
	const TypedArray<NodePath>& p_partial_paths)
{
	Ref<GLTFObjectModelProperty> ret;
	ERR_FAIL_COND_V(p_state.is_null(), ret);
	return ret;
}

Error GLTFDocumentExtension::import_post_parse(Ref<GLTFState> p_state)
{
	ERR_FAIL_COND_V(p_state.is_null(), ERR_INVALID_PARAMETER);
	Error err = OK;
	return err;
}

Error GLTFDocumentExtension::import_pre_generate(Ref<GLTFState> p_state)
{
	ERR_FAIL_COND_V(p_state.is_null(), ERR_INVALID_PARAMETER);
	Error err = OK;
	return err;
}

Node3D* GLTFDocumentExtension::generate_scene_node(
	Ref<GLTFState> p_state, Ref<GLTFNode> p_gltf_node, Node* p_scene_parent)
{
	ERR_FAIL_COND_V(p_state.is_null(), nullptr);
	ERR_FAIL_COND_V(p_gltf_node.is_null(), nullptr);
	Node3D* ret_node = nullptr;
	return ret_node;
}

Error GLTFDocumentExtension::import_post(Ref<GLTFState> p_state, Node* p_root)
{
	ERR_FAIL_NULL_V(p_root, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(p_state.is_null(), ERR_INVALID_PARAMETER);
	Error err = OK;
	return err;
}

Error GLTFDocumentExtension::export_preflight(Ref<GLTFState> p_state, Node* p_root)
{
	ERR_FAIL_NULL_V(p_root, ERR_INVALID_PARAMETER);
	Error err = OK;
	return err;
}

void GLTFDocumentExtension::convert_scene_node(
	Ref<GLTFState> p_state, Ref<GLTFNode> p_gltf_node, Node* p_scene_node)
{
	ERR_FAIL_COND(p_state.is_null());
	ERR_FAIL_COND(p_gltf_node.is_null());
	ERR_FAIL_NULL(p_scene_node);
}

Error GLTFDocumentExtension::export_post_convert(Ref<GLTFState> p_state, Node* p_root)
{
	ERR_FAIL_COND_V(p_state.is_null(), ERR_INVALID_PARAMETER);
	ERR_FAIL_NULL_V(p_root, ERR_INVALID_PARAMETER);
	Error err = OK;
	return err;
}

Error GLTFDocumentExtension::export_preserialize(Ref<GLTFState> p_state)
{
	ERR_FAIL_COND_V(p_state.is_null(), ERR_INVALID_PARAMETER);
	Error err = OK;
	return err;
}

Error GLTFDocumentExtension::save_image_at_path(Ref<GLTFState> p_state, Ref<Image> p_image,
	const String& p_file_path, const String& p_image_format, float p_lossy_quality)
{
	ERR_FAIL_COND_V(p_state.is_null(), ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(p_image.is_null(), ERR_INVALID_PARAMETER);
	Error ret = OK;
	return ret;
}

Error GLTFDocumentExtension::export_post(Ref<GLTFState> p_state)
{
	ERR_FAIL_COND_V(p_state.is_null(), ERR_INVALID_PARAMETER);
	Error err = OK;
	return err;
}

PackedStringArray GLTFDocumentExtension::get_supported_extensions() { return PackedStringArray(); }

String GLTFDocumentExtension::get_image_file_extension() { return String(); }

PackedStringArray GLTFDocumentExtension::get_saveable_image_formats()
{
	return PackedStringArray();
}


