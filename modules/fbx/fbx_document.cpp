/**************************************************************************/
/*  fbx_document.cpp                                                      */
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
#include "core/config/project_settings.h"
#include "core/crypto/crypto_core.h"
#include "core/io/config_file.h"
#include "core/io/file_access.h"
#include "core/io/file_access_wrappers.h"
#include "core/io/image.h"
#include "core/io/resource_loader.h"
#include "core/math/color.h"
#include "fbx_document.h"
#include "modules/gltf/gltf_defines.h"
#include "modules/gltf/gltf_extensions.h"
#include "modules/gltf/gltf_structures.h"
#include "modules/gltf/skin_tool.h"
#include "scene/3d/bone_attachment_3d.h"
#include "scene/3d/camera_3d.h"
#include "scene/3d/importer_mesh_instance_3d.h"
#include "scene/3d/light_3d.h"
#include "scene/resources/compressed_texture.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/material.h"
#include "scene/resources/surface_tool.h"

#ifdef TOOLS_ENABLED
#include "editor/file_system/editor_file_system.h"
#endif

#include <ufbx.h>

FBXDocument::FBXDocument() { gltf_document.instantiate(); }

String FBXDocument::_gen_unique_name(HashSet<String>& unique_names, const String& p_name)
{
	const String s_name = p_name.validate_node_name();

	String u_name;
	int index = 1;
	while (true) {
		u_name = s_name;

		if (index > 1) {
			u_name += itos(index);
		}
		if (!unique_names.has(u_name)) {
			break;
		}
		index++;
	}

	unique_names.insert(u_name);

	return u_name;
}

String FBXDocument::_sanitize_animation_name(const String& p_name)
{
	String anim_name = p_name.validate_node_name();
	return AnimationLibrary::validate_library_name(anim_name);
}

String FBXDocument::_gen_unique_animation_name(Ref<FBXState> p_state, const String& p_name)
{
	const String s_name = _sanitize_animation_name(p_name);

	String u_name;
	int index = 1;
	while (true) {
		u_name = s_name;

		if (index > 1) {
			u_name += itos(index);
		}
		if (!p_state->gltf_state->unique_animation_names.has(u_name)) {
			break;
		}
		index++;
	}

	p_state->gltf_state->unique_animation_names.insert(u_name);

	return u_name;
}

Error FBXDocument::_parse_scenes(Ref<FBXState> p_state)
{
	p_state->gltf_state->unique_names.insert("Skeleton3D"); // Reserve skeleton name.

	const ufbx_scene* fbx_scene = p_state->scene.get();

	// TODO: Multi-document support, would need test files for structure
	p_state->gltf_state->scene_name = "";

	// TODO: Append the root node directly if we use root-based space conversion
	for (const ufbx_node* root_node : fbx_scene->root_node->children) {
		p_state->gltf_state->root_nodes.push_back(int(root_node->typed_id));
	}

	return OK;
}

Ref<Image> FBXDocument::_parse_image_bytes_into_image(
	Ref<FBXState> p_state, const Vector<uint8_t>& p_bytes, const String& p_filename, int p_index)
{
	Ref<Image> r_image;
	r_image.instantiate();
	// Try to import first based on filename.
	String filename_lower = p_filename.to_lower();
	if (filename_lower.ends_with(".png")) {
		r_image->load_png_from_buffer(p_bytes);
	}
	else if (filename_lower.ends_with(".jpg")) {
		r_image->load_jpg_from_buffer(p_bytes);
	}
	else if (filename_lower.ends_with(".tga")) {
		r_image->load_tga_from_buffer(p_bytes);
	}
	// If we didn't pass the above tests, try loading as each option.
	if (r_image->is_empty()) { // Try PNG first.
		r_image->load_png_from_buffer(p_bytes);
	}
	if (r_image->is_empty()) { // And then JPEG.
		r_image->load_jpg_from_buffer(p_bytes);
	}
	if (r_image->is_empty()) { // And then TGA.
		r_image->load_tga_from_buffer(p_bytes);
	}
	// If it still can't be loaded, give up and insert an empty image as placeholder.
	if (r_image->is_empty()) {
		ERR_PRINT(vformat("FBX: Couldn't load image index '%d'", p_index));
	}
	return r_image;
}

Error FBXDocument::_parse_images(Ref<FBXState> p_state, const String& p_base_path)
{
	ERR_FAIL_COND_V(p_state.is_null(), ERR_INVALID_PARAMETER);
	Ref<GLTFState> gltf_state = p_state->gltf_state;

	const ufbx_scene* fbx_scene = p_state->scene.get();
	for (int texture_i = 0; texture_i < static_cast<int>(fbx_scene->texture_files.count);
		 texture_i++) {
		const ufbx_texture_file& fbx_texture_file = fbx_scene->texture_files[texture_i];
		String path = _as_string(fbx_texture_file.filename);
		// Use only filename for absolute paths to avoid portability issues.
		if (path.is_absolute_path()) {
			path = path.get_file();
		}
		if (!p_base_path.is_empty()) {
			path = p_base_path.path_join(path);
		}
		path = path.simplify_path();
		Vector<uint8_t> data;
		if (fbx_texture_file.content.size > 0 && fbx_texture_file.content.size <= INT_MAX) {
			data.resize(int(fbx_texture_file.content.size));
			memcpy(data.ptrw(), fbx_texture_file.content.data, fbx_texture_file.content.size);
		}
		else {
			String base_dir = gltf_state->get_base_path();
			Ref<Texture2D> texture =
				ResourceLoader::load(_get_texture_path(base_dir, path), "Texture2D");
			if (texture.is_valid()) {
				gltf_state->images.push_back(texture);
				gltf_state->source_images.push_back(texture->get_image());
				continue;
			}
			// Fallback to loading as byte array.
			data = FileAccess::get_file_as_bytes(path);
			if (data.is_empty()) {
				WARN_PRINT(vformat("FBX: Image index '%d' couldn't be loaded from path: %s because "
								   "there was no data to load. Skipping it.",
					texture_i, path));
				gltf_state->images.push_back(Ref<Texture2D>()); // Placeholder to keep count.
				gltf_state->source_images.push_back(Ref<Image>());
				continue;
			}
		}

		// Parse the image data from bytes into an Image resource and save if needed.
		String file_extension;
		Ref<Image> img = _parse_image_bytes_into_image(p_state, data, path, texture_i);
		img->set_name(itos(texture_i));
		_parse_image_save_image(p_state, data, file_extension, texture_i, img);
	}

	// Create a texture for each file texture.
	for (int texture_file_i = 0; texture_file_i < static_cast<int>(fbx_scene->texture_files.count);
		 texture_file_i++) {
		Ref<GLTFTexture> texture;
		texture.instantiate();
		texture->set_src_image(GLTFImageIndex(texture_file_i));
		gltf_state->textures.push_back(texture);
	}

	print_verbose("FBX: Total images: " + itos(gltf_state->images.size()));

	return OK;
}

Error FBXDocument::_parse_cameras(Ref<FBXState> p_state)
{
	const ufbx_scene* fbx_scene = p_state->scene.get();
	Ref<GLTFState> gltf_state = p_state->gltf_state;
	for (GLTFCameraIndex i = 0; i < static_cast<GLTFCameraIndex>(fbx_scene->cameras.count); i++) {
		const ufbx_camera* fbx_camera = fbx_scene->cameras[i];

		Ref<GLTFCamera> camera;
		camera.instantiate();
		camera->set_name(_as_string(fbx_camera->name));
		if (fbx_camera->projection_mode == UFBX_PROJECTION_MODE_PERSPECTIVE) {
			camera->set_perspective(true);
			camera->set_fov(Math::deg_to_rad(real_t(fbx_camera->field_of_view_deg.y)));
		}
		else {
			camera->set_perspective(false);
			camera->set_size_mag(real_t(fbx_camera->orthographic_size.y * 0.5f));
		}
		if (fbx_camera->near_plane != 0.0f) {
			camera->set_depth_near(fbx_camera->near_plane);
		}
		if (fbx_camera->far_plane != 0.0f) {
			camera->set_depth_far(fbx_camera->far_plane);
		}
		gltf_state->cameras.push_back(camera);
	}

	print_verbose("FBX: Total cameras: " + itos(gltf_state->cameras.size()));

	return OK;
}

void FBXDocument::_assign_node_names(Ref<FBXState> p_state)
{
	Ref<GLTFState> gltf_state = p_state->gltf_state;
	for (int i = 0; i < gltf_state->nodes.size(); i++) {
		Ref<GLTFNode> fbx_node = gltf_state->nodes[i];

		// Any joints get unique names generated when the skeleton is made, unique to the skeleton
		if (fbx_node->skeleton >= 0) {
			continue;
		}

		if (fbx_node->get_name().is_empty()) {
			if (fbx_node->mesh >= 0) {
				fbx_node->set_name(_gen_unique_name(gltf_state->unique_names, "Mesh"));
			}
			else if (fbx_node->camera >= 0) {
				fbx_node->set_name(_gen_unique_name(gltf_state->unique_names, "Camera3D"));
			}
			else {
				fbx_node->set_name(_gen_unique_name(gltf_state->unique_names, "Node"));
			}
		}

		fbx_node->set_name(_gen_unique_name(gltf_state->unique_names, fbx_node->get_name()));
	}
}

BoneAttachment3D* FBXDocument::_generate_bone_attachment(Ref<FBXState> p_state,
	Skeleton3D* p_skeleton, const GLTFNodeIndex p_node_index, const GLTFNodeIndex p_bone_index)
{
	Ref<GLTFNode> fbx_node = p_state->gltf_state->nodes[p_node_index];
	Ref<GLTFNode> bone_node = p_state->gltf_state->nodes[p_bone_index];
	BoneAttachment3D* bone_attachment = memnew(BoneAttachment3D);
	print_verbose("FBX: Creating bone attachment for: " + fbx_node->get_name());

	ERR_FAIL_COND_V(!bone_node->joint, nullptr);

	bone_attachment->set_bone_name(bone_node->get_name());

	return bone_attachment;
}

ImporterMeshInstance3D* FBXDocument::_generate_mesh_instance(
	Ref<FBXState> p_state, const GLTFNodeIndex p_node_index)
{
	Ref<GLTFState> gltf_state = p_state->gltf_state;
	Ref<GLTFNode> fbx_node = gltf_state->nodes[p_node_index];

	ERR_FAIL_INDEX_V(fbx_node->mesh, gltf_state->meshes.size(), nullptr);

	ImporterMeshInstance3D* mi = memnew(ImporterMeshInstance3D);
	print_verbose("FBX: Creating mesh for: " + fbx_node->get_name());

	gltf_state->scene_mesh_instances.insert(p_node_index, mi);
	Ref<GLTFMesh> mesh = gltf_state->meshes.write[fbx_node->mesh];
	if (mesh.is_null()) {
		return mi;
	}
	Ref<ImporterMesh> import_mesh = mesh->get_mesh();
	if (import_mesh.is_null()) {
		return mi;
	}
	mi->set_mesh(import_mesh);
	return mi;
}

Node3D* FBXDocument::_generate_spatial(Ref<FBXState> p_state, const GLTFNodeIndex p_node_index)
{
	Ref<GLTFNode> fbx_node = p_state->gltf_state->nodes[p_node_index];

	Node3D* spatial = memnew(Node3D);
	print_verbose("FBX: Converting spatial: " + fbx_node->get_name());

	return spatial;
}

Error FBXDocument::append_from_buffer(const PackedByteArray& p_bytes, const String& p_base_path,
	Ref<FBXState> p_state, uint32_t p_flags)
{
	ERR_FAIL_COND_V(p_state.is_null(), ERR_INVALID_PARAMETER);
	ERR_FAIL_NULL_V(p_bytes.ptr(), ERR_INVALID_DATA);
	Ref<GLTFState> gltf_state = p_state->gltf_state;
	Error err = FAILED;
	gltf_state->use_named_skin_binds =
		p_flags & GLTFDocument::ImportFlags::IMPORT_FLAG_USE_NAMED_SKIN_BINDS;
	gltf_state->discard_meshes_and_materials =
		p_flags & GLTFDocument::ImportFlags::IMPORT_FLAG_DISCARD_MESHES_AND_MATERIALS;

	Ref<FileAccessMemory> file_access;
	file_access.instantiate();
	file_access->open_custom(p_bytes.ptr(), p_bytes.size());
	gltf_state->base_path = p_base_path.get_base_dir();
	err = _parse(p_state, gltf_state->base_path, file_access);
	ERR_FAIL_COND_V(err != OK, err);
	for (Ref<GLTFDocumentExtension> ext : gltf_document->document_extensions) {
		ERR_CONTINUE(ext.is_null());
		err = ext->import_post_parse(gltf_state);
		ERR_FAIL_COND_V(err != OK, err);
	}
	return OK;
}

Error FBXDocument::append_from_file(
	const String& p_path, Ref<FBXState> p_state, uint32_t p_flags, const String& p_base_path)
{
	ERR_FAIL_COND_V(p_state.is_null(), ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(p_path.is_empty(), ERR_FILE_NOT_FOUND);
	Ref<GLTFState> gltf_state = p_state->gltf_state;
	gltf_state->filename = p_path.get_file().get_basename();
	gltf_state->use_named_skin_binds =
		p_flags & GLTFDocument::ImportFlags::IMPORT_FLAG_USE_NAMED_SKIN_BINDS;
	gltf_state->discard_meshes_and_materials =
		p_flags & GLTFDocument::ImportFlags::IMPORT_FLAG_DISCARD_MESHES_AND_MATERIALS;
	Error err;
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ, &err);
	ERR_FAIL_COND_V(err != OK, ERR_FILE_CANT_OPEN);
	ERR_FAIL_COND_V(file.is_null(), ERR_FILE_CANT_OPEN);
	String base_path = p_base_path;
	if (base_path.is_empty()) {
		base_path = p_path.get_base_dir();
	}
	gltf_state->base_path = base_path;
	err = _parse(p_state, base_path, file);
	ERR_FAIL_COND_V(err != OK, err);
	for (Ref<GLTFDocumentExtension> ext : gltf_document->document_extensions) {
		ERR_CONTINUE(ext.is_null());
		err = ext->import_post_parse(gltf_state);
		ERR_FAIL_COND_V(err != OK, err);
	}
	return OK;
}

void FBXDocument::_process_uv_set(PackedVector2Array& uv_array)
{
	int uv_size = uv_array.size();
	for (int uv_i = 0; uv_i < uv_size; uv_i++) {
		Vector2& uv = uv_array.write[uv_i];
		uv.y = 1.0 - uv.y;
	}
}

void FBXDocument::_zero_unused_elements(
	Vector<float>& cur_custom, int start, int end, int num_channels)
{
	for (int32_t uv_i = start; uv_i < end; uv_i++) {
		int index = uv_i * num_channels;
		for (int channel = 0; channel < num_channels; channel++) {
			cur_custom.write[index + channel] = 0;
		}
	}
}

String FBXDocument::_get_texture_path(
	const String& p_base_dir, const String& p_source_file_path) const
{
	// Check if the original path exists first.
	if (FileAccess::exists(p_source_file_path)) {
		return p_source_file_path.strip_edges();
	}
	const String tex_file_name = p_source_file_path.get_file();
	const Vector<String> subdirs = {"", "textures/", "Textures/", "images/", "Images/",
		"materials/", "Materials/", "maps/", "Maps/", "tex/", "Tex/"};
	String base_dir = p_base_dir;
	const String source_file_name = tex_file_name;
	while (!base_dir.is_empty()) {
		String old_base_dir = base_dir;
		for (int i = 0; i < subdirs.size(); ++i) {
			String full_path = base_dir.path_join(subdirs[i] + source_file_name);
			if (FileAccess::exists(full_path)) {
				return full_path.strip_edges();
			}
		}
		base_dir = base_dir.get_base_dir();
		if (base_dir == old_base_dir) {
			break;
		}
	}
	return String();
}

Error FBXDocument::_parse_skins(Ref<FBXState> p_state)
{
	const ufbx_scene* fbx_scene = p_state->scene.get();
	Ref<GLTFState> gltf_state = p_state->gltf_state;
	HashMap<GLTFNodeIndex, bool> joint_mapping;

	for (const ufbx_skin_deformer* fbx_skin : fbx_scene->skin_deformers) {
		if (fbx_skin->clusters.count == 0 || fbx_skin->weights.count == 0) {
			p_state->skin_indices.push_back(-1);
			continue;
		}

		Ref<GLTFSkin> skin;
		skin.instantiate();

		skin->inverse_binds.resize(fbx_skin->clusters.count);
		for (int skin_i = 0; skin_i < static_cast<int>(fbx_skin->clusters.count); skin_i++) {
			const ufbx_skin_cluster* fbx_cluster = fbx_skin->clusters[skin_i];
			skin->inverse_binds.write[skin_i] =
				FBXDocument::_as_xform(fbx_cluster->geometry_to_bone);
			const GLTFNodeIndex node = fbx_cluster->bone_node->typed_id;

			skin->joints.push_back(node);
			skin->joints_original.push_back(node);
			gltf_state->nodes.write[node]->joint = true;
		}

		if (fbx_skin->name.length > 0) {
			skin->set_name(FBXDocument::_as_string(fbx_skin->name));
		}
		else {
			skin->set_name(vformat("skin_%s", itos(fbx_skin->typed_id)));
		}
		p_state->skin_indices.push_back(gltf_state->skins.size());
		gltf_state->skins.push_back(skin);
	}

	for (const ufbx_bone* fbx_bone : fbx_scene->bones) {
		for (const ufbx_node* fbx_node : fbx_bone->instances) {
			const GLTFNodeIndex node = fbx_node->typed_id;
			if (!gltf_state->nodes.write[node]->joint) {
				gltf_state->nodes.write[node]->joint = true;

				if (!(fbx_node->parent && fbx_node->parent->attrib_type == UFBX_ELEMENT_BONE)) {
					Ref<GLTFSkin> skin;
					skin.instantiate();
					skin->joints.push_back(node);
					skin->joints_original.push_back(node);
					skin->set_name(vformat("skin_%s", itos(gltf_state->skins.size())));
					p_state->skin_indices.push_back(gltf_state->skins.size());
					gltf_state->skins.push_back(skin);
				}
			}
		}
	}
	p_state->original_skin_indices = p_state->skin_indices.duplicate();
	Error err =
		SkinTool::_asset_parse_skins(p_state->original_skin_indices, gltf_state->skins.duplicate(),
			gltf_state->nodes.duplicate(), p_state->skin_indices, gltf_state->skins, joint_mapping);
	if (err != OK) {
		return err;
	}
	for (int i = 0; i < gltf_state->skins.size(); ++i) {
		Ref<GLTFSkin> skin = gltf_state->skins.write[i];
		ERR_FAIL_COND_V(skin.is_null(), ERR_PARSE_ERROR);
		// Expand and verify the skin
		ERR_FAIL_COND_V(SkinTool::_expand_skin(gltf_state->nodes, skin), ERR_PARSE_ERROR);
		ERR_FAIL_COND_V(SkinTool::_verify_skin(gltf_state->nodes, skin), ERR_PARSE_ERROR);
	}

	print_verbose("FBX: Total skins: " + itos(gltf_state->skins.size()));

	for (HashMap<GLTFNodeIndex, bool>::Iterator it = joint_mapping.begin();
		 it != joint_mapping.end(); ++it) {
		GLTFNodeIndex node_index = it->key;
		bool is_joint = it->value;
		if (is_joint) {
			if (gltf_state->nodes.size() > node_index) {
				gltf_state->nodes.write[node_index]->joint = true;
			}
		}
	}

	return OK;
}

PackedByteArray FBXDocument::generate_buffer(Ref<FBXState> p_state) { return PackedByteArray(); }

Error FBXDocument::write_to_filesystem(Ref<FBXState> p_state, const String& p_path)
{
	return ERR_UNAVAILABLE;
}

Error FBXDocument::append_from_scene(Node* p_node, Ref<FBXState> p_state, uint32_t p_flags)
{
	return ERR_UNAVAILABLE;
}

void FBXDocument::set_naming_version(int p_version) { _naming_version = p_version; }

int FBXDocument::get_naming_version() const { return _naming_version; }

Vector3 FBXDocument::_as_vec3(const ufbx_vec3& p_vector)
{
	return Vector3(real_t(p_vector.x), real_t(p_vector.y), real_t(p_vector.z));
}

String FBXDocument::_as_string(const ufbx_string& p_string)
{
	return String::utf8(p_string.data, (int)p_string.length);
}

Transform3D FBXDocument::_as_xform(const ufbx_matrix& p_mat)
{
	Transform3D xform;
	xform.basis.set_column(Vector3::AXIS_X, _as_vec3(p_mat.cols[0]));
	xform.basis.set_column(Vector3::AXIS_Y, _as_vec3(p_mat.cols[1]));
	xform.basis.set_column(Vector3::AXIS_Z, _as_vec3(p_mat.cols[2]));
	xform.set_origin(_as_vec3(p_mat.cols[3]));
	return xform;
}

Error FBXDocument::_parse(Ref<FBXState>, String const&, Ref<FileAccess>) { return OK; }

GLTFImageIndex FBXDocument::_parse_image_save_image(
	Ref<FBXState>, Vector<unsigned char> const&, String const&, int, Ref<Image>)
{
	return GLTFImageIndex();
}

Node* FBXDocument::generate_scene(
	Ref<FBXState> p_state, float p_bake_fps, bool p_trimming, bool p_remove_immutable_tracks)
{
	if (p_state.is_null() || p_state->gltf_state.is_null()) {
		return nullptr;
	}
	return gltf_document->generate_scene(
		p_state->gltf_state, p_bake_fps, p_trimming, p_remove_immutable_tracks);
}
