/**************************************************************************/
/*  gltf_state.compat.inc                                                 */
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

#ifndef DISABLE_DEPRECATED

#include "gltf_state.h"

// Non-const getters for compatibility.
int32_t GLTFState::_get_handle_binary_image_bind_compat_113172()
{
	return get_handle_binary_image();
}

int GLTFState::_get_major_version_bind_compat_113172() { return get_major_version(); }

int GLTFState::_get_minor_version_bind_compat_113172() { return get_minor_version(); }

Vector<uint8_t> GLTFState::_get_glb_data_bind_compat_113172() { return get_glb_data(); }

bool GLTFState::_get_use_named_skin_binds_bind_compat_113172()
{
	return get_use_named_skin_binds();
}

bool GLTFState::_get_discard_meshes_and_materials_bind_compat_113172()
{
	return get_discard_meshes_and_materials();
}

String GLTFState::_get_scene_name_bind_compat_113172() { return get_scene_name(); }

String GLTFState::_get_base_path_bind_compat_113172() { return get_base_path(); }

String GLTFState::_get_extract_path_bind_compat_113172() { return get_extract_path(); }

String GLTFState::_get_extract_prefix_bind_compat_113172() { return get_extract_prefix(); }

PackedInt32Array GLTFState::_get_root_nodes_bind_compat_113172() { return get_root_nodes(); }

bool GLTFState::_get_create_animations_bind_compat_113172() { return get_create_animations(); }

bool GLTFState::_get_import_as_skeleton_bones_bind_compat_113172()
{
	return get_import_as_skeleton_bones();
}

Node* GLTFState::_get_scene_node_bind_compat_113172(GLTFNodeIndex p_gltf_node_index)
{
	return get_scene_node(p_gltf_node_index);
}

GLTFNodeIndex GLTFState::_get_node_index_bind_compat_113172(Node* p_node)
{
	return get_node_index(p_node);
}

int GLTFState::_get_animation_players_count_bind_compat_113172(int p_anim_player_index)
{
	return get_animation_players_count(p_anim_player_index);
}

AnimationPlayer* GLTFState::_get_animation_player_bind_compat_113172(int p_anim_player_index)
{
	return get_animation_player(p_anim_player_index);
}

#endif // DISABLE_DEPRECATED


