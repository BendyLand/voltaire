/**************************************************************************/
/*  gltf_state.cpp                                                        */
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

#include "gltf_state.compat.inc"
#include "gltf_state.h"

void GLTFState::add_used_extension(const String& p_extension_name, bool p_required)
{
	if (!extensions_used.has(p_extension_name)) {
		extensions_used.push_back(p_extension_name);
	}
	if (p_required) {
		if (!extensions_required.has(p_extension_name)) {
			extensions_required.push_back(p_extension_name);
		}
	}
}

int GLTFState::get_major_version() const { return major_version; }

void GLTFState::set_major_version(int p_major_version) { major_version = p_major_version; }

int GLTFState::get_minor_version() const { return minor_version; }

void GLTFState::set_minor_version(int p_minor_version) { minor_version = p_minor_version; }

String GLTFState::get_copyright() const { return copyright; }

void GLTFState::set_copyright(const String& p_copyright) { copyright = p_copyright; }

Vector<uint8_t> GLTFState::get_glb_data() const { return Vector<uint8_t>(glb_data); }

void GLTFState::set_glb_data(const Vector<uint8_t>& p_glb_data)
{
	glb_data = Vector<uint8_t>(p_glb_data);
}

bool GLTFState::get_use_named_skin_binds() const { return use_named_skin_binds; }

void GLTFState::set_use_named_skin_binds(bool p_use_named_skin_binds)
{
	use_named_skin_binds = p_use_named_skin_binds;
}

String GLTFState::get_scene_name() const { return scene_name; }

void GLTFState::set_scene_name(const String& p_scene_name) { scene_name = p_scene_name; }

PackedInt32Array GLTFState::get_root_nodes() const { return root_nodes; }

void GLTFState::set_root_nodes(const PackedInt32Array& p_root_nodes)
{
	root_nodes = PackedInt32Array(p_root_nodes);
}

bool GLTFState::get_create_animations() const { return create_animations; }

void GLTFState::set_create_animations(bool p_create_animations)
{
	create_animations = p_create_animations;
}

bool GLTFState::get_import_as_skeleton_bones() const { return import_as_skeleton_bones; }

void GLTFState::set_import_as_skeleton_bones(bool p_import_as_skeleton_bones)
{
	import_as_skeleton_bones = p_import_as_skeleton_bones;
}

Node* GLTFState::get_scene_node(GLTFNodeIndex p_gltf_node_index) const
{
	if (!scene_nodes.has(p_gltf_node_index)) {
		return nullptr;
	}
	return scene_nodes[p_gltf_node_index];
}

GLTFNodeIndex GLTFState::get_node_index(Node* p_node) const
{
	for (KeyValue<GLTFNodeIndex, Node*> x : scene_nodes) {
		if (x.value == p_node) {
			return x.key;
		}
	}
	return -1;
}

int GLTFState::get_animation_players_count(int p_anim_player_index) const
{
	return animation_players.size();
}

AnimationPlayer* GLTFState::get_animation_player(int p_anim_player_index) const
{
	ERR_FAIL_INDEX_V(p_anim_player_index, animation_players.size(), nullptr);
	return animation_players[p_anim_player_index];
}

void GLTFState::set_discard_meshes_and_materials(bool p_discard_meshes_and_materials)
{
	discard_meshes_and_materials = p_discard_meshes_and_materials;
}

bool GLTFState::get_discard_meshes_and_materials() const { return discard_meshes_and_materials; }

String GLTFState::get_base_path() const { return base_path; }

void GLTFState::set_base_path(const String& p_base_path)
{
	base_path = p_base_path;
	if (extract_path.is_empty()) {
		extract_path = p_base_path;
	}
}

String GLTFState::get_extract_path() const { return extract_path; }

void GLTFState::set_extract_path(const String& p_extract_path) { extract_path = p_extract_path; }

String GLTFState::get_extract_prefix() const { return extract_prefix; }

void GLTFState::set_extract_prefix(const String& p_extract_prefix)
{
	extract_prefix = p_extract_prefix;
}

String GLTFState::get_filename() const { return filename; }

void GLTFState::set_filename(const String& p_filename)
{
	filename = p_filename;
	if (extract_prefix.is_empty()) {
		extract_prefix = p_filename.get_basename();
	}
}

GLTFBufferViewIndex GLTFState::append_data_to_buffers(
	const Vector<uint8_t>& p_data, const bool p_deduplication = false)
{
	if (p_deduplication) {
		for (int i = 0; i < buffer_views.size(); i++) {
			Ref<GLTFBufferView> buffer_view = buffer_views[i];
			Vector<uint8_t> buffer_view_data = buffer_view->load_buffer_view_data(this);
			if (buffer_view_data == p_data) {
				return i;
			}
		}
	}
	// Append the given data to a buffer and create a buffer view for it.
	if (unlikely(buffers.is_empty())) {
		buffers.push_back(Vector<uint8_t>());
	}
	Vector<uint8_t>& destination_buffer = buffers.write[0];
	Ref<GLTFBufferView> buffer_view;
	buffer_view.instantiate();
	buffer_view->set_buffer(0);
	buffer_view->set_byte_offset(destination_buffer.size());
	buffer_view->set_byte_length(p_data.size());
	destination_buffer.append_array(p_data);
	const int new_index = buffer_views.size();
	buffer_views.push_back(buffer_view);
	return new_index;
}

GLTFNodeIndex GLTFState::append_gltf_node(
	Ref<GLTFNode> p_gltf_node, Node* p_godot_scene_node, GLTFNodeIndex p_parent_node_index)
{
	p_gltf_node->set_parent(p_parent_node_index);
	const GLTFNodeIndex new_index = nodes.size();
	nodes.append(p_gltf_node);
	scene_nodes.insert(new_index, p_godot_scene_node);
	if (p_parent_node_index == -1) {
		root_nodes.append(new_index);
	}
	else if (p_parent_node_index < new_index) {
		nodes.write[p_parent_node_index]->append_child_index(new_index);
	}
	return new_index;
}


