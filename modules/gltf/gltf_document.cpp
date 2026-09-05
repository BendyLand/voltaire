/**************************************************************************/
/*  gltf_document.cpp                                                     */
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

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/crypto/crypto_core.h"
#include "core/io/config_file.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/file_access_memory.h"
#include "core/io/json.h"
#include "core/io/resource_loader.h"
#include "core/io/stream_peer.h"
#include "core/version.h"
#include "extensions/gltf_spec_gloss.h"
#include "gltf_document.h"
#include "gltf_state.h"
#include "scene/3d/bone_attachment_3d.h"
#include "scene/3d/camera_3d.h"
#include "scene/3d/importer_mesh_instance_3d.h"
#include "scene/3d/light_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/multimesh_instance_3d.h"
#include "scene/animation/animation_player.h"
#include "scene/resources/3d/skin.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/portable_compressed_texture.h"
#include "scene/resources/surface_tool.h"
#include "skin_tool.h"

#ifdef TOOLS_ENABLED
#include "editor/file_system/editor_file_system.h"
#endif

#include "modules/modules_enabled.gen.h" // For csg, gridmap.

#ifdef MODULE_CSG_ENABLED
#include "modules/csg/csg_shape.h"
#endif
#ifdef MODULE_GRIDMAP_ENABLED
#include "modules/gridmap/grid_map.h"
#endif

#include <cstdio>
#include <cstdlib>

Error GLTFDocument::_serialize(Ref<GLTFState> p_state)
{
	for (Ref<GLTFDocumentExtension> ext : document_extensions) {
		ERR_CONTINUE(ext.is_null());
		Error err = ext->export_preserialize(p_state);
		ERR_CONTINUE(err != OK);
	}

	/* STEP CONVERT MESH INSTANCES */
	_convert_mesh_instances(p_state);

	/* STEP SERIALIZE CAMERAS */
	Error err = _serialize_cameras(p_state);
	if (err != OK) {
		return Error::FAILED;
	}

	/* STEP 3 CREATE SKINS */
	err = _serialize_skins(p_state);
	if (err != OK) {
		return Error::FAILED;
	}

	/* STEP SERIALIZE MESHES (we have enough info now) */
	err = _serialize_meshes(p_state);
	if (err != OK) {
		return Error::FAILED;
	}

	/* STEP SERIALIZE TEXTURES */
	err = _serialize_materials(p_state);
	if (err != OK) {
		return Error::FAILED;
	}

	/* STEP SERIALIZE TEXTURE SAMPLERS */
	err = _serialize_texture_samplers(p_state);
	if (err != OK) {
		return Error::FAILED;
	}

	/* STEP SERIALIZE ANIMATIONS */
	err = _serialize_animations(p_state);
	if (err != OK) {
		return Error::FAILED;
	}

	/* STEP SERIALIZE IMAGES */
	err = _serialize_images(p_state);
	if (err != OK) {
		return Error::FAILED;
	}

	/* STEP SERIALIZE TEXTURES */
	err = _serialize_textures(p_state);
	if (err != OK) {
		return Error::FAILED;
	}

	/* STEP SERIALIZE NODES */
	err = _serialize_nodes(p_state);
	if (err != OK) {
		return Error::FAILED;
	}

	/* STEP SERIALIZE SCENE */
	err = _serialize_scenes(p_state);
	if (err != OK) {
		return Error::FAILED;
	}

	/* STEP SERIALIZE LIGHTS */
	err = _serialize_lights(p_state);
	if (err != OK) {
		return Error::FAILED;
	}

	/* STEP SERIALIZE EXTENSIONS */
	err = _serialize_gltf_extensions(p_state);
	if (err != OK) {
		return Error::FAILED;
	}

	/* STEP SERIALIZE VERSION */
	err = _serialize_asset_header(p_state);
	if (err != OK) {
		return Error::FAILED;
	}

	/* STEP SERIALIZE ACCESSORS */
	err = _encode_accessors(p_state);
	if (err != OK) {
		return Error::FAILED;
	}

	/* STEP SERIALIZE BUFFER VIEWS */
	err = _encode_buffer_views(p_state);
	if (err != OK) {
		return Error::FAILED;
	}

	for (Ref<GLTFDocumentExtension> ext : document_extensions) {
		ERR_CONTINUE(ext.is_null());
		err = ext->export_post(p_state);
		ERR_FAIL_COND_V(err != OK, err);
	}

	return OK;
}

static Vector<real_t> _xform_to_array(const Transform3D p_transform)
{
	Vector<real_t> array;
	array.resize(16);
	Vector3 axis_x = p_transform.get_basis().get_column(Vector3::AXIS_X);
	array.write[0] = axis_x.x;
	array.write[1] = axis_x.y;
	array.write[2] = axis_x.z;
	array.write[3] = 0.0f;
	Vector3 axis_y = p_transform.get_basis().get_column(Vector3::AXIS_Y);
	array.write[4] = axis_y.x;
	array.write[5] = axis_y.y;
	array.write[6] = axis_y.z;
	array.write[7] = 0.0f;
	Vector3 axis_z = p_transform.get_basis().get_column(Vector3::AXIS_Z);
	array.write[8] = axis_z.x;
	array.write[9] = axis_z.y;
	array.write[10] = axis_z.z;
	array.write[11] = 0.0f;
	Vector3 origin = p_transform.get_origin();
	array.write[12] = origin.x;
	array.write[13] = origin.y;
	array.write[14] = origin.z;
	array.write[15] = 1.0f;
	return array;
}

String GLTFDocument::_gen_unique_name(Ref<GLTFState> p_state, const String& p_name)
{
	return _gen_unique_name_static(p_state->unique_names, p_name);
}

String GLTFDocument::_sanitize_animation_name(const String& p_name)
{
	String anim_name = p_name.validate_node_name();
	return AnimationLibrary::validate_library_name(anim_name);
}

String GLTFDocument::_gen_unique_animation_name(Ref<GLTFState> p_state, const String& p_name)
{
	const String s_name = _sanitize_animation_name(p_name);

	String u_name;
	int index = 1;
	while (true) {
		u_name = s_name;

		if (index > 1) {
			u_name += itos(index);
		}
		if (!p_state->unique_animation_names.has(u_name)) {
			break;
		}
		index++;
	}

	p_state->unique_animation_names.insert(u_name);

	return u_name;
}

String GLTFDocument::_sanitize_bone_name(const String& p_name)
{
	String bone_name = p_name;
	bone_name = bone_name.replace_chars(":/", '_');
	return bone_name;
}

String GLTFDocument::_gen_unique_bone_name(
	Ref<GLTFState> p_state, const GLTFSkeletonIndex p_skel_i, const String& p_name)
{
	String s_name = _sanitize_bone_name(p_name);
	if (s_name.is_empty()) {
		s_name = "bone";
	}
	String u_name;
	int index = 1;
	while (true) {
		u_name = s_name;

		if (index > 1) {
			u_name += "_" + itos(index);
		}
		if (!p_state->skeletons[p_skel_i]->unique_names.has(u_name)) {
			break;
		}
		index++;
	}

	p_state->skeletons.write[p_skel_i]->unique_names.insert(u_name);

	return u_name;
}

void GLTFDocument::_compute_node_heights(Ref<GLTFState> p_state)
{
	if (_naming_version < 2) {
		p_state->root_nodes.clear();
	}
	for (GLTFNodeIndex node_i = 0; node_i < p_state->nodes.size(); ++node_i) {
		Ref<GLTFNode> node = p_state->nodes[node_i];
		node->height = 0;

		GLTFNodeIndex current_i = node_i;
		while (current_i >= 0) {
			const GLTFNodeIndex parent_i = p_state->nodes[current_i]->parent;
			if (parent_i >= 0) {
				++node->height;
			}
			current_i = parent_i;
		}

		if (_naming_version < 2) {
			// This is incorrect, but required for compatibility with previous Godot versions.
			if (node->height == 0) {
				p_state->root_nodes.push_back(node_i);
			}
		}
	}
}

static Vector<uint8_t> _parse_base64_uri(const String& p_uri)
{
	int start = p_uri.find_char(',');
	ERR_FAIL_COND_V(start == -1, Vector<uint8_t>());

	CharString substr = p_uri.substr(start + 1).ascii();

	int strlen = substr.length();

	Vector<uint8_t> buf;
	buf.resize(strlen / 4 * 3 + 1 + 1);

	size_t len = 0;
	ERR_FAIL_COND_V(CryptoCore::b64_decode(buf.ptrw(), buf.size(), &len,
						(unsigned char*)substr.get_data(), strlen) != OK,
		Vector<uint8_t>());

	buf.resize(len);

	return buf;
}

static inline bool _all_buffers_empty(const Vector<Vector<uint8_t>>& p_buffers, int start_idx = 0)
{
	for (int i = start_idx; i < p_buffers.size(); i++) {
		if (!p_buffers[i].is_empty()) {
			return false;
		}
	}
	return true;
}

template <typename T>
T GLTFDocument::_decode_unpack_indexed_data(const T& p_source, const PackedInt32Array& p_indices)
{
	// Handle unpacking indexed data as if it was a regular array.
	// This isn't a feature of accessors, rather a feature of places using accessors like
	// indexed meshes, but GLTFDocument needs it in several places when reading accessors.
	T ret;
	const int64_t last_index = p_indices[p_indices.size() - 1];
	ERR_FAIL_COND_V(last_index >= p_source.size(), ret);
	ret.resize(p_indices.size());
	for (int64_t i = 0; i < p_indices.size(); i++) {
		const int64_t source_index = p_indices[i];
		ret.set(i, p_source[source_index]);
	}
	return ret;
}

PackedFloat32Array GLTFDocument::_decode_accessor_as_float32s(const Ref<GLTFState> p_gltf_state,
	GLTFAccessorIndex p_accessor_index, const PackedInt32Array& p_packed_vertex_ids)
{
	ERR_FAIL_INDEX_V(p_accessor_index, p_gltf_state->accessors.size(), PackedFloat32Array());
	Ref<GLTFAccessor> accessor = p_gltf_state->accessors[p_accessor_index];
	PackedFloat32Array numbers = accessor->decode_as_float32s(p_gltf_state);
	if (p_packed_vertex_ids.is_empty()) {
		return numbers;
	}
	return _decode_unpack_indexed_data<PackedFloat32Array>(numbers, p_packed_vertex_ids);
}

PackedFloat64Array GLTFDocument::_decode_accessor_as_float64s(const Ref<GLTFState> p_gltf_state,
	GLTFAccessorIndex p_accessor_index, const PackedInt32Array& p_packed_vertex_ids)
{
	ERR_FAIL_INDEX_V(p_accessor_index, p_gltf_state->accessors.size(), PackedFloat64Array());
	Ref<GLTFAccessor> accessor = p_gltf_state->accessors[p_accessor_index];
	PackedFloat64Array numbers = accessor->decode_as_float64s(p_gltf_state);
	if (p_packed_vertex_ids.is_empty()) {
		return numbers;
	}
	return _decode_unpack_indexed_data<PackedFloat64Array>(numbers, p_packed_vertex_ids);
}

PackedInt32Array GLTFDocument::_decode_accessor_as_int32s(const Ref<GLTFState> p_gltf_state,
	GLTFAccessorIndex p_accessor_index, const PackedInt32Array& p_packed_vertex_ids)
{
	ERR_FAIL_INDEX_V(p_accessor_index, p_gltf_state->accessors.size(), PackedInt32Array());
	Ref<GLTFAccessor> accessor = p_gltf_state->accessors[p_accessor_index];
	PackedInt32Array numbers = accessor->decode_as_int32s(p_gltf_state);
	if (p_packed_vertex_ids.is_empty()) {
		return numbers;
	}
	return _decode_unpack_indexed_data<PackedInt32Array>(numbers, p_packed_vertex_ids);
}

PackedVector2Array GLTFDocument::_decode_accessor_as_vec2(const Ref<GLTFState> p_gltf_state,
	GLTFAccessorIndex p_accessor_index, const PackedInt32Array& p_packed_vertex_ids)
{
	ERR_FAIL_INDEX_V(p_accessor_index, p_gltf_state->accessors.size(), PackedVector2Array());
	Ref<GLTFAccessor> accessor = p_gltf_state->accessors[p_accessor_index];
	PackedVector2Array vectors = accessor->decode_as_vector2s(p_gltf_state);
	if (p_packed_vertex_ids.is_empty()) {
		return vectors;
	}
	return _decode_unpack_indexed_data<PackedVector2Array>(vectors, p_packed_vertex_ids);
}

PackedVector3Array GLTFDocument::_decode_accessor_as_vec3(const Ref<GLTFState> p_gltf_state,
	GLTFAccessorIndex p_accessor_index, const PackedInt32Array& p_packed_vertex_ids)
{
	ERR_FAIL_INDEX_V(p_accessor_index, p_gltf_state->accessors.size(), PackedVector3Array());
	Ref<GLTFAccessor> accessor = p_gltf_state->accessors[p_accessor_index];
	PackedVector3Array vectors = accessor->decode_as_vector3s(p_gltf_state);
	if (p_packed_vertex_ids.is_empty()) {
		return vectors;
	}
	return _decode_unpack_indexed_data<PackedVector3Array>(vectors, p_packed_vertex_ids);
}

PackedVector4Array GLTFDocument::_decode_accessor_as_vec4(const Ref<GLTFState> p_gltf_state,
	GLTFAccessorIndex p_accessor_index, const PackedInt32Array& p_packed_vertex_ids)
{
	ERR_FAIL_INDEX_V(p_accessor_index, p_gltf_state->accessors.size(), PackedVector4Array());
	Ref<GLTFAccessor> accessor = p_gltf_state->accessors[p_accessor_index];
	PackedVector4Array vectors = accessor->decode_as_vector4s(p_gltf_state);
	if (p_packed_vertex_ids.is_empty()) {
		return vectors;
	}
	return _decode_unpack_indexed_data<PackedVector4Array>(vectors, p_packed_vertex_ids);
}

PackedColorArray GLTFDocument::_decode_accessor_as_color(const Ref<GLTFState> p_gltf_state,
	GLTFAccessorIndex p_accessor_index, const PackedInt32Array& p_packed_vertex_ids)
{
	ERR_FAIL_INDEX_V(p_accessor_index, p_gltf_state->accessors.size(), PackedColorArray());
	Ref<GLTFAccessor> accessor = p_gltf_state->accessors[p_accessor_index];
	PackedColorArray colors = accessor->decode_as_colors(p_gltf_state);
	if (p_packed_vertex_ids.is_empty()) {
		return colors;
	}
	return _decode_unpack_indexed_data<PackedColorArray>(colors, p_packed_vertex_ids);
}

Vector<Quaternion> GLTFDocument::_decode_accessor_as_quaternion(
	const Ref<GLTFState> p_gltf_state, GLTFAccessorIndex p_accessor_index)
{
	ERR_FAIL_INDEX_V(p_accessor_index, p_gltf_state->accessors.size(), Vector<Quaternion>());
	Ref<GLTFAccessor> accessor = p_gltf_state->accessors[p_accessor_index];
	Vector<Quaternion> quaternions = accessor->decode_as_quaternions(p_gltf_state);
	return quaternions;
}

void GLTFDocument::set_naming_version(int p_version) { _naming_version = p_version; }

int GLTFDocument::get_naming_version() const { return _naming_version; }

void GLTFDocument::set_image_format(const String& p_image_format)
{
	_image_format = p_image_format;
}

String GLTFDocument::get_image_format() const { return _image_format; }

void GLTFDocument::set_lossy_quality(float p_lossy_quality) { _lossy_quality = p_lossy_quality; }

float GLTFDocument::get_lossy_quality() const { return _lossy_quality; }

void GLTFDocument::set_fallback_image_format(const String& p_fallback_image_format)
{
	_fallback_image_format = p_fallback_image_format;
}

String GLTFDocument::get_fallback_image_format() const { return _fallback_image_format; }

void GLTFDocument::set_fallback_image_quality(float p_fallback_image_quality)
{
	_fallback_image_quality = p_fallback_image_quality;
}

float GLTFDocument::get_fallback_image_quality() const { return _fallback_image_quality; }

static inline Ref<Image> _duplicate_and_decompress_image(const Ref<Image>& p_image)
{
	Ref<Image> img = p_image->duplicate();
	if (img->is_compressed()) {
		img->decompress();
	}
	return img;
}

Ref<Image> GLTFDocument::_parse_image_bytes_into_image(Ref<GLTFState> p_state,
	const Vector<uint8_t>& p_bytes, const String& p_mime_type, int p_index,
	String& r_file_extension)
{
	Ref<Image> r_image;
	r_image.instantiate();
	// Check if any GLTFDocumentExtensions want to import this data as an image.
	for (Ref<GLTFDocumentExtension> ext : document_extensions) {
		ERR_CONTINUE(ext.is_null());
		Error err = ext->parse_image_data(p_state, p_bytes, p_mime_type, r_image);
		ERR_CONTINUE_MSG(err != OK, "glTF: Encountered error " + itos(err) +
										" when parsing image " + itos(p_index) + " in file " +
										p_state->filename + ". Continuing.");
		if (!r_image->is_empty()) {
			r_file_extension = ext->get_image_file_extension();
			return r_image;
		}
	}
	// If no extension wanted to import this data as an image, try to load a PNG or JPEG.
	// First we honor the mime types if they were defined.
	if (p_mime_type == "image/png") { // Load buffer as PNG.
		r_image->load_png_from_buffer(p_bytes);
		r_file_extension = ".png";
	}
	else if (p_mime_type == "image/jpeg") { // Loader buffer as JPEG.
		r_image->load_jpg_from_buffer(p_bytes);
		r_file_extension = ".jpg";
	}
	// If we didn't pass the above tests, we attempt loading as PNG and then JPEG directly.
	// This covers URIs with base64-encoded data with application/* type but
	// no optional mimeType property, or bufferViews with a bogus mimeType
	// (e.g. `image/jpeg` but the data is actually PNG).
	// That's not *exactly* what the spec mandates but this lets us be
	// lenient with bogus glb files which do exist in production.
	if (r_image->is_empty()) { // Try PNG first.
		r_image->load_png_from_buffer(p_bytes);
	}
	if (r_image->is_empty()) { // And then JPEG.
		r_image->load_jpg_from_buffer(p_bytes);
	}
	// If it still can't be loaded, give up and insert an empty image as placeholder.
	if (r_image->is_empty()) {
		ERR_PRINT(vformat("glTF: Couldn't load image index '%d' with its given mimetype: %s.",
			p_index, p_mime_type));
	}
	return r_image;
}

GLTFTextureIndex GLTFDocument::_set_texture(Ref<GLTFState> p_state, Ref<Texture2D> p_texture,
	StandardMaterial3D::TextureFilter p_filter_mode, bool p_repeats)
{
	ERR_FAIL_COND_V(p_texture.is_null(), -1);
	Ref<GLTFTexture> gltf_texture;
	gltf_texture.instantiate();
	ERR_FAIL_COND_V(p_texture->get_image().is_null(), -1);
	GLTFImageIndex gltf_src_image_i = p_state->images.find(p_texture);
	if (gltf_src_image_i == -1) {
		gltf_src_image_i = p_state->images.size();
		p_state->images.push_back(p_texture);
		p_state->source_images.push_back(p_texture->get_image());
	}
	gltf_texture->set_src_image(gltf_src_image_i);
	gltf_texture->set_sampler(_set_sampler_for_mode(p_state, p_filter_mode, p_repeats));
	GLTFTextureIndex gltf_texture_i = p_state->textures.size();
	p_state->textures.push_back(gltf_texture);
	return gltf_texture_i;
}

Ref<Texture2D> GLTFDocument::_get_texture(
	Ref<GLTFState> p_state, const GLTFTextureIndex p_texture, int p_texture_types)
{
	ERR_FAIL_COND_V_MSG(p_state->textures.is_empty(), Ref<Texture2D>(),
		"glTF import: Tried to read texture at index " + itos(p_texture) +
			", but this glTF file does not contain any textures.");
	ERR_FAIL_INDEX_V(p_texture, p_state->textures.size(), Ref<Texture2D>());
	const GLTFImageIndex image = p_state->textures[p_texture]->get_src_image();
	ERR_FAIL_INDEX_V(image, p_state->images.size(), Ref<Texture2D>());
	if (GLTFState::HandleBinaryImageMode(p_state->handle_binary_image_mode) ==
		GLTFState::HandleBinaryImageMode::HANDLE_BINARY_IMAGE_MODE_EMBED_AS_BASISU) {
		ERR_FAIL_INDEX_V(image, p_state->source_images.size(), Ref<Texture2D>());
		Ref<PortableCompressedTexture2D> portable_texture;
		portable_texture.instantiate();
		portable_texture->set_keep_compressed_buffer(true);
		Ref<Image> new_img = p_state->source_images[image]->duplicate();
		ERR_FAIL_COND_V(new_img.is_null(), Ref<Texture2D>());
		new_img->generate_mipmaps();
		if (p_texture_types) {
			portable_texture->create_from_image(
				new_img, PortableCompressedTexture2D::COMPRESSION_MODE_BASIS_UNIVERSAL, true);
		}
		else {
			portable_texture->create_from_image(
				new_img, PortableCompressedTexture2D::COMPRESSION_MODE_BASIS_UNIVERSAL, false);
		}
		p_state->images.write[image] = portable_texture;
		p_state->source_images.write[image] = new_img;
	}
	return p_state->images[image];
}

GLTFTextureSamplerIndex GLTFDocument::_set_sampler_for_mode(
	Ref<GLTFState> p_state, StandardMaterial3D::TextureFilter p_filter_mode, bool p_repeats)
{
	for (int i = 0; i < p_state->texture_samplers.size(); ++i) {
		if (p_state->texture_samplers[i]->get_filter_mode() == p_filter_mode) {
			return i;
		}
	}

	GLTFTextureSamplerIndex gltf_sampler_i = p_state->texture_samplers.size();
	Ref<GLTFTextureSampler> gltf_sampler;
	gltf_sampler.instantiate();
	gltf_sampler->set_filter_mode(p_filter_mode);
	gltf_sampler->set_wrap_mode(p_repeats);
	p_state->texture_samplers.push_back(gltf_sampler);
	return gltf_sampler_i;
}

Ref<GLTFTextureSampler> GLTFDocument::_get_sampler_for_texture(
	Ref<GLTFState> p_state, const GLTFTextureIndex p_texture)
{
	ERR_FAIL_COND_V_MSG(p_state->textures.is_empty(), Ref<GLTFTextureSampler>(),
		"glTF import: Tried to read sampler for texture at index " + itos(p_texture) +
			", but this glTF file does not contain any textures.");
	ERR_FAIL_INDEX_V(p_texture, p_state->textures.size(), Ref<GLTFTextureSampler>());
	const GLTFTextureSamplerIndex sampler = p_state->textures[p_texture]->get_sampler();

	if (sampler == -1) {
		return p_state->default_texture_sampler;
	}
	else {
		ERR_FAIL_INDEX_V(sampler, p_state->texture_samplers.size(), Ref<GLTFTextureSampler>());

		return p_state->texture_samplers[sampler];
	}
}

static inline void _set_material_texture_name(const Ref<Texture2D>& p_texture, const String& p_path,
	const String& p_mat_name, const String& p_suffix)
{
	if (p_texture->get_name().is_empty()) {
		if (p_path.is_empty()) {
			p_texture->set_name(p_mat_name + p_suffix);
		}
		else {
			p_texture->set_name(p_path.get_file().get_basename());
		}
	}
}

void GLTFDocument::spec_gloss_to_rough_metal(
	Ref<GLTFSpecGloss> r_spec_gloss, Ref<BaseMaterial3D> p_material)
{
	if (r_spec_gloss.is_null()) {
		return;
	}
	if (r_spec_gloss->spec_gloss_img.is_null()) {
		return;
	}
	if (r_spec_gloss->diffuse_img.is_null()) {
		return;
	}
	if (p_material.is_null()) {
		return;
	}
	bool has_roughness = false;
	bool has_metal = false;
	p_material->set_roughness(1.0f);
	p_material->set_metallic(1.0f);
	Ref<Image> rm_img = Image::create_empty(r_spec_gloss->spec_gloss_img->get_width(),
		r_spec_gloss->spec_gloss_img->get_height(), false, Image::FORMAT_RGBA8);
	r_spec_gloss->spec_gloss_img->decompress();
	if (r_spec_gloss->diffuse_img.is_valid()) {
		r_spec_gloss->diffuse_img->decompress();
		r_spec_gloss->diffuse_img->resize(r_spec_gloss->spec_gloss_img->get_width(),
			r_spec_gloss->spec_gloss_img->get_height(), Image::INTERPOLATE_LANCZOS);
		r_spec_gloss->spec_gloss_img->resize(r_spec_gloss->diffuse_img->get_width(),
			r_spec_gloss->diffuse_img->get_height(), Image::INTERPOLATE_LANCZOS);
	}
	for (int32_t y = 0; y < r_spec_gloss->spec_gloss_img->get_height(); y++) {
		for (int32_t x = 0; x < r_spec_gloss->spec_gloss_img->get_width(); x++) {
			const Color specular_pixel =
				r_spec_gloss->spec_gloss_img->get_pixel(x, y).srgb_to_linear();
			Color specular = Color(specular_pixel.r, specular_pixel.g, specular_pixel.b);
			specular *= r_spec_gloss->specular_factor;
			Color diffuse = Color(1.0f, 1.0f, 1.0f);
			diffuse *= r_spec_gloss->diffuse_img->get_pixel(x, y).srgb_to_linear();
			float metallic = 0.0f;
			Color base_color;
			spec_gloss_to_metal_base_color(specular, diffuse, base_color, metallic);
			Color mr = Color(1.0f, 1.0f, 1.0f);
			mr.g = specular_pixel.a;
			mr.b = metallic;
			if (!Math::is_equal_approx(mr.g, 1.0f)) {
				has_roughness = true;
			}
			if (!Math::is_zero_approx(mr.b)) {
				has_metal = true;
			}
			mr.g *= r_spec_gloss->gloss_factor;
			mr.g = 1.0f - mr.g;
			rm_img->set_pixel(x, y, mr);
			if (r_spec_gloss->diffuse_img.is_valid()) {
				r_spec_gloss->diffuse_img->set_pixel(x, y, base_color.linear_to_srgb());
			}
		}
	}
	rm_img->generate_mipmaps();
	r_spec_gloss->diffuse_img->generate_mipmaps();
	p_material->set_texture(
		BaseMaterial3D::TEXTURE_ALBEDO, ImageTexture::create_from_image(r_spec_gloss->diffuse_img));
	Ref<ImageTexture> rm_image_texture = ImageTexture::create_from_image(rm_img);
	if (has_roughness) {
		p_material->set_texture(BaseMaterial3D::TEXTURE_ROUGHNESS, rm_image_texture);
		p_material->set_roughness_texture_channel(BaseMaterial3D::TEXTURE_CHANNEL_GREEN);
	}

	if (has_metal) {
		p_material->set_texture(BaseMaterial3D::TEXTURE_METALLIC, rm_image_texture);
		p_material->set_metallic_texture_channel(BaseMaterial3D::TEXTURE_CHANNEL_BLUE);
	}
}

void GLTFDocument::spec_gloss_to_metal_base_color(
	const Color& p_specular_factor, const Color& p_diffuse, Color& r_base_color, float& r_metallic)
{
	const Color DIELECTRIC_SPECULAR = Color(0.04f, 0.04f, 0.04f);
	Color specular = Color(p_specular_factor.r, p_specular_factor.g, p_specular_factor.b);
	const float one_minus_specular_strength = 1.0f - get_max_component(specular);
	const float dielectric_specular_red = DIELECTRIC_SPECULAR.r;
	float brightness_diffuse = get_perceived_brightness(p_diffuse);
	const float brightness_specular = get_perceived_brightness(specular);
	r_metallic = solve_metallic(dielectric_specular_red, brightness_diffuse, brightness_specular,
		one_minus_specular_strength);
	const float one_minus_metallic = 1.0f - r_metallic;
	const Color base_color_from_diffuse =
		p_diffuse * (one_minus_specular_strength / (1.0f - dielectric_specular_red) /
						MAX(one_minus_metallic, CMP_EPSILON));
	const Color base_color_from_specular =
		(specular - (DIELECTRIC_SPECULAR * (one_minus_metallic))) *
		(1.0f / MAX(r_metallic, CMP_EPSILON));
	r_base_color.r =
		Math::lerp(base_color_from_diffuse.r, base_color_from_specular.r, r_metallic * r_metallic);
	r_base_color.g =
		Math::lerp(base_color_from_diffuse.g, base_color_from_specular.g, r_metallic * r_metallic);
	r_base_color.b =
		Math::lerp(base_color_from_diffuse.b, base_color_from_specular.b, r_metallic * r_metallic);
	r_base_color.a = p_diffuse.a;
	r_base_color = r_base_color.clamp();
}

bool GLTFDocument::_skins_are_same(const Ref<Skin>& p_skin_a, const Ref<Skin>& p_skin_b)
{
	if (p_skin_a->get_bind_count() != p_skin_b->get_bind_count()) {
		return false;
	}

	for (int i = 0; i < p_skin_a->get_bind_count(); ++i) {
		if (p_skin_a->get_bind_bone(i) != p_skin_b->get_bind_bone(i)) {
			return false;
		}
		if (p_skin_a->get_bind_name(i) != p_skin_b->get_bind_name(i)) {
			return false;
		}

		Transform3D a_xform = p_skin_a->get_bind_pose(i);
		Transform3D b_xform = p_skin_b->get_bind_pose(i);

		if (a_xform != b_xform) {
			return false;
		}
	}

	return true;
}

void GLTFDocument::_remove_duplicate_skins(Ref<GLTFState> p_state)
{
	for (int i = 0; i < p_state->skins.size(); ++i) {
		for (int j = i + 1; j < p_state->skins.size(); ++j) {
			const Ref<Skin> skin_i = p_state->skins[i]->godot_skin;
			const Ref<Skin> skin_j = p_state->skins[j]->godot_skin;

			if (_skins_are_same(skin_i, skin_j)) {
				// replace it and delete the old
				p_state->skins.write[j]->godot_skin = skin_i;
			}
		}
	}
}

String GLTFDocument::interpolation_to_string(const GLTFAnimation::Interpolation p_interp)
{
	String interp = "LINEAR";
	if (p_interp == GLTFAnimation::INTERP_STEP) {
		interp = "STEP";
	}
	else if (p_interp == GLTFAnimation::INTERP_LINEAR) {
		interp = "LINEAR";
	}
	else if (p_interp == GLTFAnimation::INTERP_CATMULLROMSPLINE) {
		interp = "CATMULLROMSPLINE";
	}
	else if (p_interp == GLTFAnimation::INTERP_CUBIC_SPLINE) {
		interp = "CUBICSPLINE";
	}

	return interp;
}

void GLTFDocument::_assign_node_names(Ref<GLTFState> p_state)
{
	for (int i = 0; i < p_state->nodes.size(); i++) {
		Ref<GLTFNode> gltf_node = p_state->nodes[i];
		// Any joints get unique names generated when the skeleton is made, unique to the skeleton
		if (gltf_node->skeleton >= 0) {
			continue;
		}
		String gltf_node_name = gltf_node->get_name();
		if (gltf_node_name.is_empty()) {
			if (_naming_version == 0) {
				if (gltf_node->mesh >= 0) {
					gltf_node_name = _gen_unique_name(p_state, "Mesh");
				}
				else if (gltf_node->camera >= 0) {
					gltf_node_name = _gen_unique_name(p_state, "Camera3D");
				}
				else {
					gltf_node_name = _gen_unique_name(p_state, "Node");
				}
			}
			else {
				if (gltf_node->mesh >= 0) {
					gltf_node_name = "Mesh";
				}
				else if (gltf_node->camera >= 0) {
					gltf_node_name = "Camera";
				}
				else {
					gltf_node_name = "Node";
				}
			}
		}
		gltf_node->set_name(_gen_unique_name(p_state, gltf_node_name));
	}
}

BoneAttachment3D* GLTFDocument::_generate_bone_attachment(
	Skeleton3D* p_godot_skeleton, const Ref<GLTFNode>& p_bone_node)
{
	BoneAttachment3D* bone_attachment = memnew(BoneAttachment3D);
	print_verbose("glTF: Creating bone attachment for: " + p_bone_node->get_name());
	bone_attachment->set_name(p_bone_node->get_name());
	p_godot_skeleton->add_child(bone_attachment, true);
	bone_attachment->set_bone_name(p_bone_node->get_name());
	return bone_attachment;
}

BoneAttachment3D* GLTFDocument::_generate_bone_attachment_compat_4pt4(Ref<GLTFState> p_state,
	Skeleton3D* p_skeleton, const GLTFNodeIndex p_node_index, const GLTFNodeIndex p_bone_index)
{
	Ref<GLTFNode> gltf_node = p_state->nodes[p_node_index];
	Ref<GLTFNode> bone_node = p_state->nodes[p_bone_index];
	BoneAttachment3D* bone_attachment = memnew(BoneAttachment3D);
	print_verbose("glTF: Creating bone attachment for: " + gltf_node->get_name());

	ERR_FAIL_COND_V(!bone_node->joint, nullptr);

	bone_attachment->set_bone_name(bone_node->get_name());

	return bone_attachment;
}

ImporterMeshInstance3D* GLTFDocument::_generate_mesh_instance(
	Ref<GLTFState> p_state, const GLTFNodeIndex p_node_index)
{
	Ref<GLTFNode> gltf_node = p_state->nodes[p_node_index];

	ERR_FAIL_INDEX_V(gltf_node->mesh, p_state->meshes.size(), nullptr);

	ImporterMeshInstance3D* mi = memnew(ImporterMeshInstance3D);
	print_verbose("glTF: Creating mesh for: " + gltf_node->get_name());

	p_state->scene_mesh_instances.insert(p_node_index, mi);
	Ref<GLTFMesh> mesh = p_state->meshes.write[gltf_node->mesh];
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

Light3D* GLTFDocument::_generate_light(Ref<GLTFState> p_state, const GLTFNodeIndex p_node_index)
{
	Ref<GLTFNode> gltf_node = p_state->nodes[p_node_index];

	ERR_FAIL_INDEX_V(gltf_node->light, p_state->lights.size(), nullptr);

	print_verbose("glTF: Creating light for: " + gltf_node->get_name());

	Ref<GLTFLight> l = p_state->lights[gltf_node->light];
	return l->to_node();
}

Camera3D* GLTFDocument::_generate_camera(Ref<GLTFState> p_state, const GLTFNodeIndex p_node_index)
{
	Ref<GLTFNode> gltf_node = p_state->nodes[p_node_index];

	ERR_FAIL_INDEX_V(gltf_node->camera, p_state->cameras.size(), nullptr);

	print_verbose("glTF: Creating camera for: " + gltf_node->get_name());

	Ref<GLTFCamera> c = p_state->cameras[gltf_node->camera];
	return c->to_node();
}

GLTFCameraIndex GLTFDocument::_convert_camera(Ref<GLTFState> p_state, Camera3D* p_camera)
{
	print_verbose("glTF: Converting camera: " + p_camera->get_name());

	Ref<GLTFCamera> c = GLTFCamera::from_node(p_camera);
	GLTFCameraIndex camera_index = p_state->cameras.size();
	p_state->cameras.push_back(c);
	return camera_index;
}

GLTFLightIndex GLTFDocument::_convert_light(Ref<GLTFState> p_state, Light3D* p_light)
{
	print_verbose("glTF: Converting light: " + p_light->get_name());

	Ref<GLTFLight> l = GLTFLight::from_node(p_light);

	GLTFLightIndex light_index = p_state->lights.size();
	p_state->lights.push_back(l);
	return light_index;
}

void GLTFDocument::_convert_spatial(Ref<GLTFState> p_state, Node3D* p_spatial, Ref<GLTFNode> p_node)
{
	p_node->transform = p_spatial->get_transform();
}

Node3D* GLTFDocument::_generate_spatial(Ref<GLTFState> p_state, const GLTFNodeIndex p_node_index)
{
	Ref<GLTFNode> gltf_node = p_state->nodes[p_node_index];

	Node3D* spatial = memnew(Node3D);
	print_verbose("glTF: Converting spatial: " + gltf_node->get_name());

	return spatial;
}

void GLTFDocument::_convert_camera_to_gltf(
	Camera3D* camera, Ref<GLTFState> p_state, Ref<GLTFNode> p_gltf_node)
{
	ERR_FAIL_NULL(camera);
	GLTFCameraIndex camera_index = _convert_camera(p_state, camera);
	if (camera_index != -1) {
		p_gltf_node->camera = camera_index;
	}
}

void GLTFDocument::_convert_light_to_gltf(
	Light3D* light, Ref<GLTFState> p_state, Ref<GLTFNode> p_gltf_node)
{
	ERR_FAIL_NULL(light);
	GLTFLightIndex light_index = _convert_light(p_state, light);
	if (light_index != -1) {
		p_gltf_node->light = light_index;
	}
}

void GLTFDocument::_convert_multi_mesh_instance_to_gltf(MultiMeshInstance3D* p_multi_mesh_instance,
	GLTFNodeIndex p_parent_node_index, GLTFNodeIndex p_root_node_index, Ref<GLTFNode> p_gltf_node,
	Ref<GLTFState> p_state)
{
	ERR_FAIL_NULL(p_multi_mesh_instance);
	Ref<MultiMesh> multi_mesh = p_multi_mesh_instance->get_multimesh();
	if (multi_mesh.is_null()) {
		return;
	}
	Ref<GLTFMesh> gltf_mesh;
	gltf_mesh.instantiate();
	Ref<Mesh> mesh = multi_mesh->get_mesh();
	if (mesh.is_null()) {
		return;
	}
	gltf_mesh->set_original_name(multi_mesh->get_name());
	gltf_mesh->set_name(multi_mesh->get_name());
	gltf_mesh->set_mesh(ImporterMesh::from_mesh(mesh));
	GLTFMeshIndex mesh_index = p_state->meshes.size();
	p_state->meshes.push_back(gltf_mesh);
	for (int32_t instance_i = 0; instance_i < multi_mesh->get_instance_count(); instance_i++) {
		Transform3D transform;
		if (multi_mesh->get_transform_format() == MultiMesh::TRANSFORM_2D) {
			Transform2D xform_2d = multi_mesh->get_instance_transform_2d(instance_i);
			transform.origin = Vector3(xform_2d.get_origin().x, 0, xform_2d.get_origin().y);
			real_t rotation = xform_2d.get_rotation();
			Quaternion quaternion(Vector3(0, 1, 0), rotation);
			Size2 scale = xform_2d.get_scale();
			transform.basis.set_quaternion_scale(quaternion, Vector3(scale.x, 0, scale.y));
			transform = p_multi_mesh_instance->get_transform() * transform;
		}
		else if (multi_mesh->get_transform_format() == MultiMesh::TRANSFORM_3D) {
			transform = p_multi_mesh_instance->get_transform() *
						multi_mesh->get_instance_transform(instance_i);
		}
		Ref<GLTFNode> new_gltf_node;
		new_gltf_node.instantiate();
		new_gltf_node->mesh = mesh_index;
		new_gltf_node->transform = transform;
		new_gltf_node->set_original_name(p_multi_mesh_instance->get_name());
		new_gltf_node->set_name(_gen_unique_name(p_state, p_multi_mesh_instance->get_name()));
		p_gltf_node->children.push_back(p_state->nodes.size());
		p_state->nodes.push_back(new_gltf_node);
	}
}

void GLTFDocument::_convert_mesh_instance_to_gltf(
	MeshInstance3D* p_scene_parent, Ref<GLTFState> p_state, Ref<GLTFNode> p_gltf_node)
{
	GLTFMeshIndex gltf_mesh_index = _convert_mesh_to_gltf(p_state, p_scene_parent);
	if (gltf_mesh_index != -1) {
		p_gltf_node->mesh = gltf_mesh_index;
	}
}

bool GLTFDocument::_does_skinned_mesh_require_placeholder_node(
	Ref<GLTFState> p_state, Ref<GLTFNode> p_gltf_node)
{
	if (p_gltf_node->skin < 0) {
		return false; // Not a skinned mesh.
	}
	// Check for child nodes that aren't joints/bones.
	for (int i = 0; i < p_gltf_node->children.size(); ++i) {
		Ref<GLTFNode> child = p_state->nodes[p_gltf_node->children[i]];
		if (!child->joint) {
			return true;
		}
		// Edge case: If a child's skeleton is not yet in the tree, then we must add it as a child
		// of this node. While the Skeleton3D node isn't a glTF node, it's still a case where we
		// need a placeholder. This is required to handle this issue:
		// https://github.com/godotengine/godot/issues/67773
		const GLTFSkeletonIndex skel_index = child->skeleton;
		ERR_FAIL_INDEX_V(skel_index, p_state->skeletons.size(), false);
		if (p_state->skeletons[skel_index]->godot_skeleton->get_parent() == nullptr) {
			return true;
		}
	}
	return false;
}

// Deprecated code used when naming_version is 0 or 1 (Godot 4.0 to 4.4).

// Deprecated code used when naming_version is 0 or 1 (Godot 4.0 to 4.4).

template <typename T> struct SceneFormatImporterGLTFInterpolate
{
	T lerp(const T& a, const T& b, float c) const { return a + (b - a) * c; }

	T catmull_rom(const T& p0, const T& p1, const T& p2, const T& p3, float t)
	{
		const float t2 = t * t;
		const float t3 = t2 * t;

		return 0.5f *
			   ((2.0f * p1) + (-p0 + p2) * t + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
				   (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
	}

	T hermite(T start, T tan_start, T end, T tan_end, float t)
	{
		/* Formula from the glTF 2.0 specification. */
		const real_t t2 = t * t;
		const real_t t3 = t2 * t;

		const real_t h00 = 2.0 * t3 - 3.0 * t2 + 1.0;
		const real_t h10 = t3 - 2.0 * t2 + t;
		const real_t h01 = -2.0 * t3 + 3.0 * t2;
		const real_t h11 = t3 - t2;

		return start * h00 + tan_start * h10 + end * h01 + tan_end * h11;
	}
};

// thank you for existing, partial specialization
template <> struct SceneFormatImporterGLTFInterpolate<Quaternion>
{
	Quaternion lerp(const Quaternion& a, const Quaternion& b, const float c) const
	{
		ERR_FAIL_COND_V_MSG(!a.is_normalized(), Quaternion(),
			vformat("The quaternion \"a\" %s must be normalized.", a));
		ERR_FAIL_COND_V_MSG(!b.is_normalized(), Quaternion(),
			vformat("The quaternion \"b\" %s must be normalized.", b));

		return a.slerp(b, c).normalized();
	}

	Quaternion catmull_rom(const Quaternion& p0, const Quaternion& p1, const Quaternion& p2,
		const Quaternion& p3, const float c)
	{
		ERR_FAIL_COND_V_MSG(!p1.is_normalized(), Quaternion(),
			vformat("The quaternion \"p1\" (%s) must be normalized.", p1));
		ERR_FAIL_COND_V_MSG(!p2.is_normalized(), Quaternion(),
			vformat("The quaternion \"p2\" (%s) must be normalized.", p2));

		return p1.slerp(p2, c).normalized();
	}

	Quaternion hermite(const Quaternion start, const Quaternion tan_start, const Quaternion end,
		const Quaternion tan_end, const float t)
	{
		ERR_FAIL_COND_V_MSG(!start.is_normalized(), Quaternion(),
			vformat("The start quaternion %s must be normalized.", start));
		ERR_FAIL_COND_V_MSG(!end.is_normalized(), Quaternion(),
			vformat("The end quaternion %s must be normalized.", end));

		return start.slerp(end, t).normalized();
	}
};

template <typename T>
T GLTFDocument::_interpolate_track(const Vector<double>& p_times, const Vector<T>& p_values,
	const float p_time, const GLTFAnimation::Interpolation p_interp)
{
	ERR_FAIL_COND_V(p_values.is_empty(), T());
	if (p_times.size() !=
		(p_values.size() / (p_interp == GLTFAnimation::INTERP_CUBIC_SPLINE ? 3 : 1))) {
		ERR_PRINT_ONCE("The interpolated values are not corresponding to its times.");
		return p_values[0];
	}
	// could use binary search, worth it?
	int idx = -1;
	for (int i = 0; i < p_times.size(); i++) {
		if (p_times[i] > p_time) {
			break;
		}
		idx++;
	}

	SceneFormatImporterGLTFInterpolate<T> interp;

	switch (p_interp) {
	case GLTFAnimation::INTERP_LINEAR: {
		if (idx == -1) {
			return p_values[0];
		}
		else if (idx >= p_times.size() - 1) {
			return p_values[p_times.size() - 1];
		}

		const float c = (p_time - p_times[idx]) / (p_times[idx + 1] - p_times[idx]);

		return interp.lerp(p_values[idx], p_values[idx + 1], c);
	} break;
	case GLTFAnimation::INTERP_STEP: {
		if (idx == -1) {
			return p_values[0];
		}
		else if (idx >= p_times.size() - 1) {
			return p_values[p_times.size() - 1];
		}

		return p_values[idx];
	} break;
	case GLTFAnimation::INTERP_CATMULLROMSPLINE: {
		if (idx == -1) {
			return p_values[1];
		}
		else if (idx >= p_times.size() - 1) {
			return p_values[1 + p_times.size() - 1];
		}

		const float c = (p_time - p_times[idx]) / (p_times[idx + 1] - p_times[idx]);

		return interp.catmull_rom(
			p_values[idx - 1], p_values[idx], p_values[idx + 1], p_values[idx + 3], c);
	} break;
	case GLTFAnimation::INTERP_CUBIC_SPLINE: {
		if (idx == -1) {
			return p_values[1];
		}
		else if (idx >= p_times.size() - 1) {
			return p_values[(p_times.size() - 1) * 3 + 1];
		}

		const float td = (p_times[idx + 1] - p_times[idx]);
		const float c = (p_time - p_times[idx]) / td;

		const T& from = p_values[idx * 3 + 1];
		const T tan_from = td * p_values[idx * 3 + 2];
		const T& to = p_values[idx * 3 + 4];
		const T tan_to = td * p_values[idx * 3 + 3];

		return interp.hermite(from, tan_from, to, tan_to, c);
	} break;
	}

	ERR_FAIL_V(p_values[0]);
}

void GLTFDocument::_append_khr_texture_transform_ext_json_pointer(
	PackedStringArray& p_split_json_pointer, const String& p_texture_name, const bool p_is_offset)
{
	p_split_json_pointer.append(p_texture_name);
	p_split_json_pointer.append("extensions");
	p_split_json_pointer.append("KHR_texture_transform");
	if (p_is_offset) {
		p_split_json_pointer.append("offset");
	}
	else {
		p_split_json_pointer.append("scale");
	}
}

float GLTFDocument::solve_metallic(float p_dielectric_specular, float p_diffuse, float p_specular,
	float p_one_minus_specular_strength)
{
	if (p_specular <= p_dielectric_specular) {
		return 0.0f;
	}

	const float a = p_dielectric_specular;
	const float b = p_diffuse * p_one_minus_specular_strength / (1.0f - p_dielectric_specular) +
					p_specular - 2.0f * p_dielectric_specular;
	const float c = p_dielectric_specular - p_specular;
	const float D = b * b - 4.0f * a * c;
	return CLAMP((-b + Math::sqrt(D)) / (2.0f * a), 0.0f, 1.0f);
}

float GLTFDocument::get_perceived_brightness(const Color p_color)
{
	const Color coeff = Color(R_BRIGHTNESS_COEFF, G_BRIGHTNESS_COEFF, B_BRIGHTNESS_COEFF);
	const Color value = coeff * (p_color * p_color);

	const float r = value.r;
	const float g = value.g;
	const float b = value.b;

	return Math::sqrt(r + g + b);
}

float GLTFDocument::get_max_component(const Color& p_color)
{
	const float r = p_color.r;
	const float g = p_color.g;
	const float b = p_color.b;

	return MAX(MAX(r, g), b);
}

void GLTFDocument::_build_parent_hierarchy(Ref<GLTFState> p_state)
{
	// build the hierarchy
	for (GLTFNodeIndex node_i = 0; node_i < p_state->nodes.size(); node_i++) {
		for (int j = 0; j < p_state->nodes[node_i]->children.size(); j++) {
			GLTFNodeIndex child_i = p_state->nodes[node_i]->children[j];
			ERR_FAIL_INDEX(child_i, p_state->nodes.size());
			if (p_state->nodes.write[child_i]->parent != -1) {
				continue;
			}
			p_state->nodes.write[child_i]->parent = node_i;
		}
	}
}

Vector<Ref<GLTFDocumentExtension>> GLTFDocument::all_document_extensions;
Mutex GLTFDocument::all_document_extensions_mutex;

void GLTFDocument::register_gltf_document_extension(
	Ref<GLTFDocumentExtension> p_extension, bool p_first_priority)
{
	MutexLock lock(all_document_extensions_mutex);
	if (!all_document_extensions.has(p_extension)) {
		if (p_first_priority) {
			all_document_extensions.insert(0, p_extension);
		}
		else {
			all_document_extensions.push_back(p_extension);
		}
	}
}

void GLTFDocument::unregister_gltf_document_extension(Ref<GLTFDocumentExtension> p_extension)
{
	MutexLock lock(all_document_extensions_mutex);
	all_document_extensions.erase(p_extension);
}

void GLTFDocument::unregister_all_gltf_document_extensions()
{
	MutexLock lock(all_document_extensions_mutex);
	all_document_extensions.clear();
}

Vector<Ref<GLTFDocumentExtension>> GLTFDocument::get_all_gltf_document_extensions()
{
	MutexLock lock(all_document_extensions_mutex);
	return all_document_extensions;
}

Vector<String> GLTFDocument::get_supported_gltf_extensions()
{
	HashSet<String> set = get_supported_gltf_extensions_hashset();
	Vector<String> vec;
	for (const String& s : set) {
		vec.append(s);
	}
	vec.sort();
	return vec;
}

HashSet<String> GLTFDocument::get_supported_gltf_extensions_hashset()
{
	HashSet<String> supported_extensions;
	// If the extension is supported directly in GLTFDocument, list it here.
	// Other built-in extensions are supported by GLTFDocumentExtension classes.
	supported_extensions.insert("GODOT_single_root");
	supported_extensions.insert("KHR_animation_pointer");
	supported_extensions.insert("KHR_lights_punctual");
	supported_extensions.insert("KHR_materials_emissive_strength");
	supported_extensions.insert("KHR_materials_pbrSpecularGlossiness");
	supported_extensions.insert("KHR_materials_unlit");
	supported_extensions.insert("KHR_node_visibility");
	supported_extensions.insert("KHR_texture_transform");
	for (Ref<GLTFDocumentExtension> ext : get_all_gltf_document_extensions()) {
		ERR_CONTINUE(ext.is_null());
		Vector<String> ext_supported_extensions = ext->get_supported_extensions();
		for (int i = 0; i < ext_supported_extensions.size(); ++i) {
			supported_extensions.insert(ext_supported_extensions[i]);
		}
	}
	return supported_extensions;
}

Error GLTFDocument::_parse_gltf_state(Ref<GLTFState> p_state, const String& p_search_path)
{
	Error err;

	/* PARSE BUFFERS */
	err = _parse_buffers(p_state, p_search_path);
	ERR_FAIL_COND_V(err != OK, ERR_PARSE_ERROR);

	/* PARSE BUFFER VIEWS */
	err = _parse_buffer_views(p_state);
	ERR_FAIL_COND_V(err != OK, ERR_PARSE_ERROR);

	/* PARSE ACCESSORS */
	err = _parse_accessors(p_state);
	ERR_FAIL_COND_V(err != OK, ERR_PARSE_ERROR);

	/* PARSE EXTENSIONS */
	err = _parse_gltf_extensions(p_state);
	ERR_FAIL_COND_V(err != OK, ERR_PARSE_ERROR);

	/* PARSE SCENE */
	err = _parse_scenes(p_state);
	ERR_FAIL_COND_V(err != OK, ERR_PARSE_ERROR);

	/* PARSE NODES */
	err = _parse_nodes(p_state);
	ERR_FAIL_COND_V(err != OK, ERR_PARSE_ERROR);

	if (!p_state->discard_meshes_and_materials) {
		/* PARSE IMAGES */
		err = _parse_images(p_state, p_search_path);

		ERR_FAIL_COND_V(err != OK, ERR_PARSE_ERROR);

		/* PARSE TEXTURE SAMPLERS */
		err = _parse_texture_samplers(p_state);

		ERR_FAIL_COND_V(err != OK, ERR_PARSE_ERROR);

		/* PARSE TEXTURES */
		err = _parse_textures(p_state);

		ERR_FAIL_COND_V(err != OK, ERR_PARSE_ERROR);

		/* PARSE TEXTURES */
		err = _parse_materials(p_state);

		ERR_FAIL_COND_V(err != OK, ERR_PARSE_ERROR);
	}

	/* PARSE SKINS */
	err = _parse_skins(p_state);

	ERR_FAIL_COND_V(err != OK, ERR_PARSE_ERROR);

	/* DETERMINE SKELETONS */
	if (p_state->get_import_as_skeleton_bones()) {
		err = SkinTool::_determine_skeletons(
			p_state->skins, p_state->nodes, p_state->skeletons, p_state->root_nodes, true);
	}
	else {
		err = SkinTool::_determine_skeletons(p_state->skins, p_state->nodes, p_state->skeletons,
			Vector<GLTFNodeIndex>(), _naming_version < 2);
	}
	ERR_FAIL_COND_V(err != OK, ERR_PARSE_ERROR);

	/* ASSIGN SCENE NODE NAMES */
	// This must be run AFTER determining skeletons, and BEFORE parsing animations.
	_assign_node_names(p_state);

	/* PARSE MESHES (we have enough info now) */
	err = _parse_meshes(p_state);
	ERR_FAIL_COND_V(err != OK, ERR_PARSE_ERROR);

	/* PARSE LIGHTS */
	err = _parse_lights(p_state);
	ERR_FAIL_COND_V(err != OK, ERR_PARSE_ERROR);

	/* PARSE CAMERAS */
	err = _parse_cameras(p_state);
	ERR_FAIL_COND_V(err != OK, ERR_PARSE_ERROR);

	/* PARSE ANIMATIONS */
	err = _parse_animations(p_state);
	ERR_FAIL_COND_V(err != OK, ERR_PARSE_ERROR);

	return OK;
}

PackedByteArray GLTFDocument::generate_buffer(Ref<GLTFState> p_state)
{
	ERR_FAIL_COND_V(p_state.is_null(), PackedByteArray());
	// For buffers, set the state filename to an empty string, but
	// don't touch the base path, in case the user set it manually.
	p_state->filename = "";
	Error err = _serialize(p_state);
	ERR_FAIL_COND_V(err != OK, PackedByteArray());
	PackedByteArray bytes = _serialize_glb_buffer(p_state, &err);
	return bytes;
}

Error GLTFDocument::write_to_filesystem(Ref<GLTFState> p_state, const String& p_path)
{
	ERR_FAIL_COND_V(p_state.is_null(), ERR_INVALID_PARAMETER);
	p_state->set_base_path(p_path.get_base_dir());
	p_state->filename = p_path.get_file();
	Error err = _serialize(p_state);
	if (err != OK) {
		return err;
	}
	err = _serialize_file(p_state, p_path);
	if (err != OK) {
		return Error::FAILED;
	}
	return OK;
}

Error GLTFDocument::append_from_scene(Node* p_node, Ref<GLTFState> p_state, uint32_t p_flags)
{
	ERR_FAIL_NULL_V(p_node, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(p_state.is_null(), ERR_INVALID_PARAMETER);
	p_state->use_named_skin_binds = p_flags & ImportFlags::IMPORT_FLAG_USE_NAMED_SKIN_BINDS;
	p_state->discard_meshes_and_materials =
		p_flags & ImportFlags::IMPORT_FLAG_DISCARD_MESHES_AND_MATERIALS;
	p_state->force_generate_tangents = p_flags & ImportFlags::IMPORT_FLAG_GENERATE_TANGENT_ARRAYS;
	p_state->force_disable_compression =
		p_flags & ImportFlags::IMPORT_FLAG_FORCE_DISABLE_MESH_COMPRESSION;
	if (!p_state->buffers.size()) {
		p_state->buffers.push_back(Vector<uint8_t>());
	}
	// Perform export preflight for document extensions. Only extensions that
	// return OK will be used for the rest of the export steps.
	document_extensions.clear();
	for (Ref<GLTFDocumentExtension> ext : get_all_gltf_document_extensions()) {
		ERR_CONTINUE(ext.is_null());
		Ref<GLTFDocumentExtension> ext_dup = ext;
		Error err = ext_dup->export_preflight(p_state, p_node);
		if (err == OK) {
			document_extensions.push_back(ext_dup);
		}
	}
	// Add the root node(s) and their descendants to the state.
	if (_root_node_mode == RootNodeMode::ROOT_NODE_MODE_MULTI_ROOT) {
		const int child_count = p_node->get_child_count();
		for (int i = 0; i < child_count; i++) {
			_convert_scene_node(p_state, p_node->get_child(i), -1, -1);
		}
		p_state->scene_name = p_node->get_name();
	}
	else {
		if (_root_node_mode == RootNodeMode::ROOT_NODE_MODE_SINGLE_ROOT) {
			p_state->extensions_used.append("GODOT_single_root");
		}
		_convert_scene_node(p_state, p_node, -1, -1);
	}
	// Run post-convert for each extension, in case an extension needs to do something after
	// converting the scene.
	for (Ref<GLTFDocumentExtension> ext : document_extensions) {
		ERR_CONTINUE(ext.is_null());
		Error err = ext->export_post_convert(p_state, p_node);
		ERR_CONTINUE(err != OK);
	}
	return OK;
}

Error GLTFDocument::append_from_buffer(const PackedByteArray& p_bytes, const String& p_base_path,
	Ref<GLTFState> p_state, uint32_t p_flags)
{
	ERR_FAIL_COND_V(p_state.is_null(), ERR_INVALID_PARAMETER);
	// TODO Add missing texture and missing .bin file paths to r_missing_deps 2021-09-10 fire
	Error err = FAILED;
	p_state->use_named_skin_binds = p_flags & ImportFlags::IMPORT_FLAG_USE_NAMED_SKIN_BINDS;
	p_state->discard_meshes_and_materials =
		p_flags & ImportFlags::IMPORT_FLAG_DISCARD_MESHES_AND_MATERIALS;
	p_state->force_generate_tangents = p_flags & ImportFlags::IMPORT_FLAG_GENERATE_TANGENT_ARRAYS;
	p_state->force_disable_compression =
		p_flags & ImportFlags::IMPORT_FLAG_FORCE_DISABLE_MESH_COMPRESSION;

	Ref<FileAccessMemory> file_access;
	file_access.instantiate();
	file_access->open_custom(p_bytes.ptr(), p_bytes.size());
	p_state->set_base_path(p_base_path.get_base_dir());
	err = _parse(p_state, p_state->base_path, file_access);
	ERR_FAIL_COND_V(err != OK, err);
	for (Ref<GLTFDocumentExtension> ext : document_extensions) {
		ERR_CONTINUE(ext.is_null());
		err = ext->import_post_parse(p_state);
		ERR_FAIL_COND_V(err != OK, err);
	}
	return OK;
}

Error GLTFDocument::append_from_file(
	const String& p_path, Ref<GLTFState> p_state, uint32_t p_flags, const String& p_base_path)
{
	ERR_FAIL_COND_V(p_state.is_null(), ERR_INVALID_PARAMETER);
	// TODO Add missing texture and missing .bin file paths to r_missing_deps 2021-09-10 fire
	p_state->set_filename(p_path.get_file().get_basename());
	p_state->use_named_skin_binds = p_flags & ImportFlags::IMPORT_FLAG_USE_NAMED_SKIN_BINDS;
	p_state->discard_meshes_and_materials =
		p_flags & ImportFlags::IMPORT_FLAG_DISCARD_MESHES_AND_MATERIALS;
	p_state->force_generate_tangents = p_flags & ImportFlags::IMPORT_FLAG_GENERATE_TANGENT_ARRAYS;
	p_state->force_disable_compression =
		p_flags & ImportFlags::IMPORT_FLAG_FORCE_DISABLE_MESH_COMPRESSION;

	Error err;
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ, &err);
	ERR_FAIL_COND_V_MSG(err != OK, err, vformat(R"(Can't open file at path "%s")", p_path));
	ERR_FAIL_COND_V(file.is_null(), ERR_FILE_CANT_OPEN);
	String base_path = p_base_path;
	if (base_path.is_empty()) {
		base_path = p_path.get_base_dir();
	}
	p_state->set_base_path(base_path);
	err = _parse(p_state, base_path, file);
	ERR_FAIL_COND_V(err != OK, err);
	for (Ref<GLTFDocumentExtension> ext : document_extensions) {
		ERR_CONTINUE(ext.is_null());
		err = ext->import_post_parse(p_state);
		ERR_FAIL_COND_V(err != OK, err);
	}
	return OK;
}

void GLTFDocument::set_root_node_mode(GLTFDocument::RootNodeMode p_root_node_mode)
{
	_root_node_mode = p_root_node_mode;
}

GLTFDocument::RootNodeMode GLTFDocument::get_root_node_mode() const { return _root_node_mode; }

void GLTFDocument::set_texture_map_mode(GLTFDocument::TextureMapMode p_texture_map_mode)
{
	_texture_map_mode = p_texture_map_mode;
}

void GLTFDocument::set_visibility_mode(VisibilityMode p_visibility_mode)
{
	_visibility_mode = p_visibility_mode;
}

GLTFDocument::VisibilityMode GLTFDocument::get_visibility_mode() const { return _visibility_mode; }

String GLTFDocument::_gen_unique_name_static(HashSet<String>& r_unique_names, const String& p_name)
{
	const String s_name = p_name.validate_node_name();

	String u_name;
	int index = 1;
	while (true) {
		u_name = s_name;

		if (index > 1) {
			u_name += itos(index);
		}
		if (!r_unique_names.has(u_name)) {
			break;
		}
		index++;
	}

	r_unique_names.insert(u_name);

	return u_name;
}


