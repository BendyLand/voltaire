/**************************************************************************/
/*  gltf_document.cpp                                                     */
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
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/file_access_wrappers.h"
#include "core/io/json.h"
#include "core/io/resource_loader.h"
#include "core/io/stream_peer.h"
#include "core/version.h"
#include "gltf_document.h"
#include "gltf_extensions.h"
#include "gltf_state.h"
#include "scene/3d/bone_attachment_3d.h"
#include "scene/3d/camera_3d.h"
#include "scene/3d/importer_mesh_instance_3d.h"
#include "scene/3d/light_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/multimesh_instance_3d.h"
#include "scene/animation/animation_player.h"
#include "scene/resources/3d/skin.h"
#include "scene/resources/compressed_texture.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/surface_tool.h"
#include "core/math/disjoint_set.h"
#include "register_types.h"

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
	Ref<GLTFSpecGloss> r_spec_gloss, Ref<StandardMaterial3D> p_material)
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
		StandardMaterial3D::TEXTURE_ALBEDO, ImageTexture::create_from_image(r_spec_gloss->diffuse_img));
	Ref<ImageTexture> rm_image_texture = ImageTexture::create_from_image(rm_img);
	if (has_roughness) {
		p_material->set_texture(StandardMaterial3D::TEXTURE_ROUGHNESS, rm_image_texture);
		p_material->set_roughness_texture_channel(StandardMaterial3D::TEXTURE_CHANNEL_GREEN);
	}

	if (has_metal) {
		p_material->set_texture(StandardMaterial3D::TEXTURE_METALLIC, rm_image_texture);
		p_material->set_metallic_texture_channel(StandardMaterial3D::TEXTURE_CHANNEL_BLUE);
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

Error GLTFDocument::_parse(Ref<GLTFState>, String const&, Ref<FileAccess>) { return OK; }

void GLTFDocument::_convert_scene_node(Ref<GLTFState>, Node*, int, int) {}

Error GLTFDocument::_serialize_textures(Ref<GLTFState>) { return OK; }

Error GLTFDocument::_serialize_texture_samplers(Ref<GLTFState>) { return OK; }

Error GLTFDocument::_serialize_skins(Ref<GLTFState>) { return OK; }

Error GLTFDocument::_serialize_scenes(Ref<GLTFState>) { return OK; }

Error GLTFDocument::_serialize_nodes(Ref<GLTFState>) { return OK; }

Error GLTFDocument::_serialize_meshes(Ref<GLTFState>) { return OK; }

Error GLTFDocument::_serialize_materials(Ref<GLTFState>) { return OK; }

Error GLTFDocument::_serialize_lights(Ref<GLTFState>) { return OK; }

Error GLTFDocument::_serialize_images(Ref<GLTFState>) { return OK; }

Error GLTFDocument::_serialize_gltf_extensions(Ref<GLTFState>) const { return OK; }

PackedByteArray GLTFDocument::_serialize_glb_buffer(Ref<GLTFState>, Error*)
{
	return PackedByteArray();
}

Error GLTFDocument::_serialize_file(Ref<GLTFState>, String) { return OK; }

Error GLTFDocument::_serialize_cameras(Ref<GLTFState>) { return OK; }

Error GLTFDocument::_serialize_asset_header(Ref<GLTFState>) { return OK; }

Error GLTFDocument::_serialize_animations(Ref<GLTFState>) { return OK; }

Error GLTFDocument::_parse_textures(Ref<GLTFState>) { return OK; }

Error GLTFDocument::_parse_texture_samplers(Ref<GLTFState>) { return OK; }

Error GLTFDocument::_parse_skins(Ref<GLTFState>) { return OK; }

Error GLTFDocument::_parse_scenes(Ref<GLTFState>) { return OK; }

Error GLTFDocument::_parse_nodes(Ref<GLTFState>) { return OK; }

Error GLTFDocument::_parse_meshes(Ref<GLTFState>) { return OK; }

Error GLTFDocument::_parse_materials(Ref<GLTFState>) { return OK; }

Error GLTFDocument::_parse_lights(Ref<GLTFState>) { return OK; }

Error GLTFDocument::_parse_images(Ref<GLTFState>, String const&) { return OK; }

Error GLTFDocument::_parse_gltf_extensions(Ref<GLTFState>) { return OK; }

Error GLTFDocument::_parse_cameras(Ref<GLTFState>) { return OK; }

Error GLTFDocument::_parse_buffer_views(Ref<GLTFState>) { return OK; }

Error GLTFDocument::_parse_buffers(Ref<GLTFState>, String const&) { return OK; }

Error GLTFDocument::_parse_animations(Ref<GLTFState>) { return OK; }

Error GLTFDocument::_parse_accessors(Ref<GLTFState>) { return OK; }

Error GLTFDocument::_encode_buffer_views(Ref<GLTFState>) { return OK; }

Error GLTFDocument::_encode_accessors(Ref<GLTFState>) { return OK; }

GLTFMeshIndex GLTFDocument::_convert_mesh_to_gltf(Ref<GLTFState>, MeshInstance3D*)
{
	return GLTFMeshIndex();
}

void GLTFDocument::_convert_mesh_instances(Ref<GLTFState>) {}

Node* GLTFDocument::generate_scene(
	Ref<GLTFState> p_state, float p_bake_fps, bool p_save_fps, bool p_trim_fps)
{
	return nullptr;
}

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

SkinNodeIndex SkinTool::_find_highest_node(
	Vector<Ref<GLTFNode>>& r_nodes, const Vector<GLTFNodeIndex>& p_subset)
{
	int highest = -1;
	SkinNodeIndex best_node = -1;

	for (int i = 0; i < p_subset.size(); ++i) {
		const SkinNodeIndex node_i = p_subset[i];
		const Ref<GLTFNode> node = r_nodes[node_i];

		if (highest == -1 || node->height < highest) {
			highest = node->height;
			best_node = node_i;
		}
	}

	return best_node;
}

bool SkinTool::_capture_nodes_in_skin(
	const Vector<Ref<GLTFNode>>& nodes, Ref<GLTFSkin> p_skin, const SkinNodeIndex p_node_index)
{
	bool found_joint = false;
	Ref<GLTFNode> current_node = nodes[p_node_index];

	for (int i = 0; i < current_node->children.size(); ++i) {
		found_joint |= _capture_nodes_in_skin(nodes, p_skin, current_node->children[i]);
	}

	if (found_joint) {
		// Mark it if we happen to find another skins joint...
		if (current_node->joint && !p_skin->joints.has(p_node_index)) {
			p_skin->joints.push_back(p_node_index);
		}
		else if (!p_skin->non_joints.has(p_node_index)) {
			p_skin->non_joints.push_back(p_node_index);
		}
	}

	if (p_skin->joints.find(p_node_index) > 0) {
		return true;
	}

	return false;
}

void SkinTool::_capture_nodes_for_multirooted_skin(
	Vector<Ref<GLTFNode>>& r_nodes, Ref<GLTFSkin> p_skin)
{
	DisjointSet<SkinNodeIndex> disjoint_set;

	for (int i = 0; i < p_skin->joints.size(); ++i) {
		const SkinNodeIndex node_index = p_skin->joints[i];
		const SkinNodeIndex parent = r_nodes[node_index]->parent;
		disjoint_set.insert(node_index);

		if (p_skin->joints.has(parent)) {
			disjoint_set.create_union(parent, node_index);
		}
	}

	Vector<SkinNodeIndex> roots;
	disjoint_set.get_representatives(roots);

	if (roots.size() <= 1) {
		return;
	}

	int maxHeight = -1;

	// Determine the max height rooted tree
	for (int i = 0; i < roots.size(); ++i) {
		const SkinNodeIndex root = roots[i];

		if (maxHeight == -1 || r_nodes[root]->height < maxHeight) {
			maxHeight = r_nodes[root]->height;
		}
	}

	// Go up the tree till all of the multiple roots of the skin are at the same hierarchy level.
	// This sucks, but 99% of all game engines (not just Godot) would have this same issue.
	for (int i = 0; i < roots.size(); ++i) {
		SkinNodeIndex current_node = roots[i];
		while (r_nodes[current_node]->height > maxHeight) {
			SkinNodeIndex parent = r_nodes[current_node]->parent;

			if (r_nodes[parent]->joint && !p_skin->joints.has(parent)) {
				p_skin->joints.push_back(parent);
			}
			else if (!p_skin->non_joints.has(parent)) {
				p_skin->non_joints.push_back(parent);
			}

			current_node = parent;
		}

		// replace the roots
		roots.write[i] = current_node;
	}

	// Climb up the tree until they all have the same parent
	bool all_same;

	do {
		all_same = true;
		const SkinNodeIndex first_parent = r_nodes[roots[0]]->parent;

		for (int i = 1; i < roots.size(); ++i) {
			all_same &= (first_parent == r_nodes[roots[i]]->parent);
		}

		if (!all_same) {
			for (int i = 0; i < roots.size(); ++i) {
				const SkinNodeIndex current_node = roots[i];
				const SkinNodeIndex parent = r_nodes[current_node]->parent;

				if (r_nodes[parent]->joint && !p_skin->joints.has(parent)) {
					p_skin->joints.push_back(parent);
				}
				else if (!p_skin->non_joints.has(parent)) {
					p_skin->non_joints.push_back(parent);
				}

				roots.write[i] = parent;
			}
		}

	} while (!all_same);
}

Error SkinTool::_expand_skin(Vector<Ref<GLTFNode>>& r_nodes, Ref<GLTFSkin> p_skin)
{
	_capture_nodes_for_multirooted_skin(r_nodes, p_skin);

	// Grab all nodes that lay in between skin joints/nodes
	DisjointSet<GLTFNodeIndex> disjoint_set;

	Vector<SkinNodeIndex> all_skin_nodes;
	all_skin_nodes.append_array(p_skin->joints);
	all_skin_nodes.append_array(p_skin->non_joints);

	for (int i = 0; i < all_skin_nodes.size(); ++i) {
		const SkinNodeIndex node_index = all_skin_nodes[i];
		const SkinNodeIndex parent = r_nodes[node_index]->parent;
		disjoint_set.insert(node_index);

		if (all_skin_nodes.has(parent)) {
			disjoint_set.create_union(parent, node_index);
		}
	}

	Vector<SkinNodeIndex> out_owners;
	disjoint_set.get_representatives(out_owners);

	Vector<SkinNodeIndex> out_roots;

	for (int i = 0; i < out_owners.size(); ++i) {
		Vector<SkinNodeIndex> set;
		disjoint_set.get_members(set, out_owners[i]);

		const SkinNodeIndex root = _find_highest_node(r_nodes, set);
		ERR_FAIL_COND_V(root < 0, FAILED);
		out_roots.push_back(root);
	}

	out_roots.sort();

	for (int i = 0; i < out_roots.size(); ++i) {
		_capture_nodes_in_skin(r_nodes, p_skin, out_roots[i]);
	}

	p_skin->roots = out_roots;

	return OK;
}

Error SkinTool::_verify_skin(Vector<Ref<GLTFNode>>& r_nodes, Ref<GLTFSkin> p_skin)
{
	// This may seem duplicated from expand_skins, but this is really a sanity check! (so it kinda
	// is) In case additional interpolating logic is added to the skins, this will help ensure that
	// you do not cause it to self implode into a fiery blaze

	// We are going to re-calculate the root nodes and compare them to the ones saved in the skin,
	// then ensure the multiple trees (if they exist) are on the same sublevel

	// Grab all nodes that lay in between skin joints/nodes
	DisjointSet<GLTFNodeIndex> disjoint_set;

	Vector<SkinNodeIndex> all_skin_nodes;
	all_skin_nodes.append_array(p_skin->joints);
	all_skin_nodes.append_array(p_skin->non_joints);

	for (int i = 0; i < all_skin_nodes.size(); ++i) {
		const SkinNodeIndex node_index = all_skin_nodes[i];
		const SkinNodeIndex parent = r_nodes[node_index]->parent;
		disjoint_set.insert(node_index);

		if (all_skin_nodes.has(parent)) {
			disjoint_set.create_union(parent, node_index);
		}
	}

	Vector<SkinNodeIndex> out_owners;
	disjoint_set.get_representatives(out_owners);

	Vector<SkinNodeIndex> out_roots;

	for (int i = 0; i < out_owners.size(); ++i) {
		Vector<SkinNodeIndex> set;
		disjoint_set.get_members(set, out_owners[i]);

		const SkinNodeIndex root = _find_highest_node(r_nodes, set);
		ERR_FAIL_COND_V(root < 0, FAILED);
		out_roots.push_back(root);
	}

	out_roots.sort();

	ERR_FAIL_COND_V(out_roots.is_empty(), FAILED);

	// Make sure the roots are the exact same (they better be)
	ERR_FAIL_COND_V(out_roots.size() != p_skin->roots.size(), FAILED);
	for (int i = 0; i < out_roots.size(); ++i) {
		ERR_FAIL_COND_V(out_roots[i] != p_skin->roots[i], FAILED);
	}

	// Single rooted skin? Perfectly ok!
	if (out_roots.size() == 1) {
		return OK;
	}

	// Make sure all parents of a multi-rooted skin are the SAME
	const SkinNodeIndex parent = r_nodes[out_roots[0]]->parent;
	for (int i = 1; i < out_roots.size(); ++i) {
		if (r_nodes[out_roots[i]]->parent != parent) {
			return FAILED;
		}
	}

	return OK;
}

void SkinTool::_recurse_children(Vector<Ref<GLTFNode>>& nodes, const SkinNodeIndex p_node_index,
	RBSet<GLTFNodeIndex>& p_all_skin_nodes, HashSet<GLTFNodeIndex>& p_child_visited_set)
{
	if (p_child_visited_set.has(p_node_index)) {
		return;
	}
	p_child_visited_set.insert(p_node_index);

	Ref<GLTFNode> current_node = nodes[p_node_index];
	for (int i = 0; i < current_node->children.size(); ++i) {
		_recurse_children(nodes, current_node->children[i], p_all_skin_nodes, p_child_visited_set);
	}

	// Continue to use 'current_node' for clarity and direct access.
	if (current_node->skin < 0 || current_node->mesh < 0 || !current_node->children.is_empty()) {
		p_all_skin_nodes.insert(p_node_index);
	}
}

void SkinTool::_check_if_parent_needs_to_become_joint(const Vector<Ref<GLTFNode>>& p_all_nodes,
	const Vector<GLTFNodeIndex>& p_skeleton_node_indices, const Ref<GLTFNode>& p_gltf_node,
	Vector<GLTFNodeIndex>& r_non_joint_indices)
{
	const GLTFNodeIndex parent_index = p_gltf_node->parent;
	if (parent_index >= 0) {
		const Ref<GLTFNode>& parent = p_all_nodes[parent_index];
		if (!parent->joint && p_skeleton_node_indices.has(parent_index) &&
			!r_non_joint_indices.has(parent_index)) {
			_check_if_parent_needs_to_become_joint(
				p_all_nodes, p_skeleton_node_indices, parent, r_non_joint_indices);
			r_non_joint_indices.push_back(parent_index);
		}
	}
}

Error SkinTool::_determine_skeletons(Vector<Ref<GLTFSkin>>& skins, Vector<Ref<GLTFNode>>& nodes,
	Vector<Ref<GLTFSkeleton>>& skeletons, const Vector<GLTFNodeIndex>& p_single_skeleton_roots,
	bool p_turn_non_joint_descendants_into_bones)
{
	if (!p_single_skeleton_roots.is_empty()) {
		Ref<GLTFSkin> skin;
		skin.instantiate();
		skin->set_name("godot_single_skeleton_root");
		for (GLTFNodeIndex i = 0; i < p_single_skeleton_roots.size(); i++) {
			skin->joints.push_back(p_single_skeleton_roots[i]);
		}
		skins.push_back(skin);
	}

	// Using a disjoint set, we are going to potentially combine all skins that are actually
	// branches of a main skeleton, or treat skins defining the same set of nodes as ONE skeleton.
	// This is another unclear issue caused by the current glTF specification.

	DisjointSet<GLTFNodeIndex> skeleton_sets;

	for (GLTFSkinIndex skin_i = 0; skin_i < skins.size(); ++skin_i) {
		const Ref<GLTFSkin> skin = skins[skin_i];
		ERR_CONTINUE(skin.is_null());

		HashSet<GLTFNodeIndex> child_visited_set;
		RBSet<GLTFNodeIndex> all_skin_nodes;
		for (int i = 0; i < skin->joints.size(); ++i) {
			all_skin_nodes.insert(skin->joints[i]);
			SkinTool::_recurse_children(nodes, skin->joints[i], all_skin_nodes, child_visited_set);
		}
		for (int i = 0; i < skin->non_joints.size(); ++i) {
			all_skin_nodes.insert(skin->non_joints[i]);
			SkinTool::_recurse_children(
				nodes, skin->non_joints[i], all_skin_nodes, child_visited_set);
		}
		for (GLTFNodeIndex node_index : all_skin_nodes) {
			const GLTFNodeIndex parent = nodes[node_index]->parent;
			skeleton_sets.insert(node_index);

			if (all_skin_nodes.has(parent)) {
				skeleton_sets.create_union(parent, node_index);
			}
		}

		// We are going to connect the separate skin subtrees in each skin together
		// so that the final roots are entire sets of valid skin trees
		for (int i = 1; i < skin->roots.size(); ++i) {
			skeleton_sets.create_union(skin->roots[0], skin->roots[i]);
		}
	}

	{ // attempt to joint all touching subsets (siblings/parent are part of another skin)
		Vector<SkinNodeIndex> groups_representatives;
		skeleton_sets.get_representatives(groups_representatives);

		Vector<SkinNodeIndex> highest_group_members;
		Vector<Vector<SkinNodeIndex>> groups;
		for (int i = 0; i < groups_representatives.size(); ++i) {
			Vector<SkinNodeIndex> group;
			skeleton_sets.get_members(group, groups_representatives[i]);
			highest_group_members.push_back(SkinTool::_find_highest_node(nodes, group));
			groups.push_back(group);
		}

		for (int i = 0; i < highest_group_members.size(); ++i) {
			const SkinNodeIndex node_i = highest_group_members[i];

			// Attach any siblings together (this needs to be done n^2/2 times)
			for (int j = i + 1; j < highest_group_members.size(); ++j) {
				const SkinNodeIndex node_j = highest_group_members[j];

				// Even if they are siblings under the root! :)
				if (nodes[node_i]->parent == nodes[node_j]->parent) {
					skeleton_sets.create_union(node_i, node_j);
				}
			}

			// Attach any parenting going on together (we need to do this n^2 times)
			const SkinNodeIndex node_i_parent = nodes[node_i]->parent;
			if (node_i_parent >= 0) {
				for (int j = 0; j < groups.size() && i != j; ++j) {
					const Vector<SkinNodeIndex>& group = groups[j];

					if (group.has(node_i_parent)) {
						const SkinNodeIndex node_j = highest_group_members[j];
						skeleton_sets.create_union(node_i, node_j);
					}
				}
			}
		}
	}

	// At this point, the skeleton groups should be finalized
	Vector<SkinNodeIndex> skeleton_owners;
	skeleton_sets.get_representatives(skeleton_owners);

	// Mark all the skins actual skeletons, after we have merged them
	for (SkinSkeletonIndex skel_i = 0; skel_i < skeleton_owners.size(); ++skel_i) {
		const SkinNodeIndex skeleton_owner = skeleton_owners[skel_i];
		Ref<GLTFSkeleton> skeleton;
		skeleton.instantiate();

		Vector<SkinNodeIndex> skeleton_nodes;
		skeleton_sets.get_members(skeleton_nodes, skeleton_owner);

		for (GLTFSkinIndex skin_i = 0; skin_i < skins.size(); ++skin_i) {
			Ref<GLTFSkin> skin = skins.write[skin_i];

			// If any of the skeletons nodes exist in a skin, that skin now maps to the skeleton
			for (int i = 0; i < skeleton_nodes.size(); ++i) {
				SkinNodeIndex skel_node_i = skeleton_nodes[i];
				if (skin->joints.has(skel_node_i) || skin->non_joints.has(skel_node_i)) {
					skin->skeleton = skel_i;
					continue;
				}
			}
		}
		// The nodes placed into `non_joints` will be passed to
		// `_reparent_non_joint_skeleton_subtrees` which will add them to the skeleton and set
		// `node->joint` to true.
		Vector<SkinNodeIndex> non_joints;
		for (int i = 0; i < skeleton_nodes.size(); ++i) {
			const SkinNodeIndex node_i = skeleton_nodes[i];
			Ref<GLTFNode> node = nodes[node_i];
			if (node->joint) {
				if (!p_turn_non_joint_descendants_into_bones) {
					// If a joint node has non-joint parents, we need to make them joints as well.
					// For example, if A/B/C/D, and A/B and D are joints, then we need to make C a
					// joint as well. This is required to handle the "skinD" example in
					// `Animation_Skin_09.gltf` from the glTF-Asset-Generator:
					// https://github.com/KhronosGroup/glTF-Asset-Generator/blob/master/Output/Positive/Animation_Skin
					_check_if_parent_needs_to_become_joint(nodes, skeleton_nodes, node, non_joints);
				}
				skeleton->joints.push_back(node_i);
			}
			else if (p_turn_non_joint_descendants_into_bones) {
				non_joints.push_back(node_i);
			}
		}

		skeletons.push_back(skeleton);

		SkinTool::_reparent_non_joint_skeleton_subtrees(nodes, skeletons.write[skel_i], non_joints);
	}

	for (SkinSkeletonIndex skel_i = 0; skel_i < skeletons.size(); ++skel_i) {
		Ref<GLTFSkeleton> skeleton = skeletons.write[skel_i];

		for (int i = 0; i < skeleton->joints.size(); ++i) {
			const SkinNodeIndex node_i = skeleton->joints[i];
			Ref<GLTFNode> node = nodes[node_i];

			ERR_FAIL_COND_V(!node->joint, ERR_PARSE_ERROR);
			ERR_FAIL_COND_V(node->skeleton >= 0, ERR_PARSE_ERROR);
			node->skeleton = skel_i;
		}

		ERR_FAIL_COND_V(
			SkinTool::_determine_skeleton_roots(nodes, skeletons, skel_i), ERR_PARSE_ERROR);
	}

	return OK;
}

Error SkinTool::_reparent_non_joint_skeleton_subtrees(Vector<Ref<GLTFNode>>& nodes,
	Ref<GLTFSkeleton> p_skeleton, const Vector<SkinNodeIndex>& p_non_joints)
{
	DisjointSet<GLTFNodeIndex> subtree_set;

	// Populate the disjoint set with ONLY non joints that are in the skeleton hierarchy (non_joints
	// vector) This way we can find any joints that lie in between joints, as the current glTF
	// specification mentions nothing about non-joints being in between joints of the same skin.
	// Hopefully one day we can remove this code.

	// skinD depicted here explains this issue:
	// https://github.com/KhronosGroup/glTF-Asset-Generator/blob/master/Output/Positive/Animation_Skin

	for (int i = 0; i < p_non_joints.size(); ++i) {
		const SkinNodeIndex node_i = p_non_joints[i];

		subtree_set.insert(node_i);

		const SkinNodeIndex parent_i = nodes[node_i]->parent;
		if (parent_i >= 0 && p_non_joints.has(parent_i) && !nodes[parent_i]->joint) {
			subtree_set.create_union(parent_i, node_i);
		}
	}

	// Find all the non joint subtrees and re-parent them to a new "fake" joint

	Vector<SkinNodeIndex> non_joint_subtree_roots;
	subtree_set.get_representatives(non_joint_subtree_roots);

	for (int root_i = 0; root_i < non_joint_subtree_roots.size(); ++root_i) {
		const SkinNodeIndex subtree_root = non_joint_subtree_roots[root_i];

		Vector<SkinNodeIndex> subtree_nodes;
		subtree_set.get_members(subtree_nodes, subtree_root);

		for (int subtree_i = 0; subtree_i < subtree_nodes.size(); ++subtree_i) {
			Ref<GLTFNode> node = nodes[subtree_nodes[subtree_i]];
			node->joint = true;
			// Add the joint to the skeletons joints
			p_skeleton->joints.push_back(subtree_nodes[subtree_i]);
		}
	}

	return OK;
}

Error SkinTool::_determine_skeleton_roots(Vector<Ref<GLTFNode>>& nodes,
	Vector<Ref<GLTFSkeleton>>& skeletons, const SkinSkeletonIndex p_skel_i)
{
	DisjointSet<GLTFNodeIndex> disjoint_set;

	for (SkinNodeIndex i = 0; i < nodes.size(); ++i) {
		const Ref<GLTFNode> node = nodes[i];

		if (node->skeleton != p_skel_i) {
			continue;
		}

		disjoint_set.insert(i);

		if (node->parent >= 0 && nodes[node->parent]->skeleton == p_skel_i) {
			disjoint_set.create_union(node->parent, i);
		}
	}

	Ref<GLTFSkeleton> skeleton = skeletons.write[p_skel_i];

	Vector<SkinNodeIndex> representatives;
	disjoint_set.get_representatives(representatives);

	Vector<SkinNodeIndex> roots;

	for (int i = 0; i < representatives.size(); ++i) {
		Vector<SkinNodeIndex> set;
		disjoint_set.get_members(set, representatives[i]);
		const SkinNodeIndex root = _find_highest_node(nodes, set);
		ERR_FAIL_COND_V(root < 0, FAILED);
		roots.push_back(root);
	}

	roots.sort();

	skeleton->roots = roots;

	if (roots.is_empty()) {
		return FAILED;
	}
	else if (roots.size() == 1) {
		return OK;
	}

	// Check that the subtrees have the same parent root
	const SkinNodeIndex parent = nodes[roots[0]]->parent;
	for (int i = 1; i < roots.size(); ++i) {
		if (nodes[roots[i]]->parent != parent) {
			return FAILED;
		}
	}

	return OK;
}

Error SkinTool::_map_skin_joints_indices_to_skeleton_bone_indices(Vector<Ref<GLTFSkin>>& skins,
	Vector<Ref<GLTFSkeleton>>& skeletons, Vector<Ref<GLTFNode>>& nodes)
{
	for (GLTFSkinIndex skin_i = 0; skin_i < skins.size(); ++skin_i) {
		Ref<GLTFSkin> skin = skins.write[skin_i];
		ERR_CONTINUE(skin.is_null());

		Ref<GLTFSkeleton> skeleton = skeletons[skin->skeleton];

		for (int joint_index = 0; joint_index < skin->joints_original.size(); ++joint_index) {
			const SkinNodeIndex node_i = skin->joints_original[joint_index];
			const Ref<GLTFNode> node = nodes[node_i];

			const int bone_index = skeleton->godot_skeleton->find_bone(node->get_name());
			ERR_FAIL_COND_V(bone_index < 0, FAILED);

			skin->joint_i_to_bone_i.insert(joint_index, bone_index);
		}
	}

	return OK;
}

Error SkinTool::_create_skins(Vector<Ref<GLTFSkin>>& skins, Vector<Ref<GLTFNode>>& nodes,
	bool use_named_skin_binds, HashSet<String>& unique_names)
{
	for (GLTFSkinIndex skin_i = 0; skin_i < skins.size(); ++skin_i) {
		Ref<GLTFSkin> gltf_skin = skins.write[skin_i];
		ERR_CONTINUE(gltf_skin.is_null());

		Ref<Skin> skin;
		skin.instantiate();

		// Some skins don't have IBM's! What absolute monsters!
		const bool has_ibms = !gltf_skin->inverse_binds.is_empty();

		for (int joint_i = 0; joint_i < gltf_skin->joints_original.size(); ++joint_i) {
			SkinNodeIndex node = gltf_skin->joints_original[joint_i];
			String bone_name = nodes[node]->get_name();

			Transform3D xform;
			if (has_ibms) {
				xform = gltf_skin->inverse_binds[joint_i];
			}

			if (use_named_skin_binds) {
				skin->add_named_bind(bone_name, xform);
			}
			else {
				int32_t bone_i = gltf_skin->joint_i_to_bone_i[joint_i];
				skin->add_bind(bone_i, xform);
			}
		}

		gltf_skin->godot_skin = skin;
	}

	// Purge the duplicates!
	_remove_duplicate_skins(skins);

	// Create unique names now, after removing duplicates
	for (GLTFSkinIndex skin_i = 0; skin_i < skins.size(); ++skin_i) {
		ERR_CONTINUE(skins.get(skin_i).is_null());
		Ref<Skin> skin = skins.write[skin_i]->godot_skin;
		ERR_CONTINUE(skin.is_null());
		if (skin->get_name().is_empty()) {
			// Make a unique name, no node represents this skin
			skin->set_name(_gen_unique_name(unique_names, "Skin"));
		}
	}

	return OK;
}

// FIXME: Duplicated from FBXDocument, very similar code in GLTFDocument too,
// and even below in this class for bone names.
String SkinTool::_gen_unique_name(HashSet<String>& unique_names, const String& p_name)
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

bool SkinTool::_skins_are_same(const Ref<Skin> p_skin_a, const Ref<Skin> p_skin_b)
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

void SkinTool::_remove_duplicate_skins(Vector<Ref<GLTFSkin>>& r_skins)
{
	for (int i = 0; i < r_skins.size(); ++i) {
		for (int j = i + 1; j < r_skins.size(); ++j) {
			const Ref<Skin> skin_i = r_skins[i]->godot_skin;
			const Ref<Skin> skin_j = r_skins[j]->godot_skin;

			if (_skins_are_same(skin_i, skin_j)) {
				// replace it and delete the old
				r_skins.write[j]->godot_skin = skin_i;
			}
		}
	}
}

String SkinTool::_gen_unique_bone_name(HashSet<String>& r_unique_names, const String& p_name)
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
		if (!r_unique_names.has(u_name)) {
			break;
		}
		index++;
	}

	r_unique_names.insert(u_name);

	return u_name;
}

Error SkinTool::_asset_parse_skins(const Vector<SkinNodeIndex>& input_skin_indices,
	const Vector<Ref<GLTFSkin>>& input_skins, const Vector<Ref<GLTFNode>>& input_nodes,
	Vector<SkinNodeIndex>& output_skin_indices, Vector<Ref<GLTFSkin>>& output_skins,
	HashMap<GLTFNodeIndex, bool>& joint_mapping)
{
	output_skin_indices.clear();
	output_skins.clear();
	joint_mapping.clear();

	for (int i = 0; i < input_skin_indices.size(); ++i) {
		SkinNodeIndex skin_index = input_skin_indices[i];
		if (skin_index >= 0 && skin_index < input_skins.size()) {
			output_skin_indices.push_back(skin_index);
			output_skins.push_back(input_skins[skin_index]);
			Ref<GLTFSkin> skin = input_skins[skin_index];
			Vector<SkinNodeIndex> skin_joints = skin->get_joints();
			for (int j = 0; j < skin_joints.size(); ++j) {
				SkinNodeIndex joint_index = skin_joints[j];
				joint_mapping[joint_index] = true;
			}
		}
	}

	return OK;
}

String SkinTool::_sanitize_bone_name(const String& p_name)
{
	String bone_name = p_name;
	bone_name = bone_name.replace_chars(":/", '_');
	return bone_name;
}

#ifdef TOOLS_ENABLED
#include "core/io/resource_importer.h"

void EditorSceneFormatImporterGLTF::get_extensions(List<String>* r_extensions) const
{
	r_extensions->push_back("gltf");
	r_extensions->push_back("glb");
}

void EditorSceneFormatImporterGLTF::get_import_options(
	const String& p_path, List<ResourceImporter::ImportOption>* r_options)
{
}
#endif // TOOLS_ENABLED

#define GLTF_REGISTER_DOCUMENT_EXTENSION(m_doc_ext_class)                                          \
	Ref<m_doc_ext_class> extension_##m_doc_ext_class;                                              \
	extension_##m_doc_ext_class.instantiate();                                                     \
	GLTFDocument::register_gltf_document_extension(extension_##m_doc_ext_class);

void uninitialize_gltf_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	GLTFDocument::unregister_all_gltf_document_extensions();
}

void initialize_gltf_module(ModuleInitializationLevel p_level) {}

// ============================================================================
// Merged from modules/gltf/gltf_extensions.cpp
// ============================================================================

#include "gltf_extensions.h"

#include "gltf_state.h"
#include "scene/3d/light_3d.h"

#include <cfloat>

#ifndef PHYSICS_3D_DISABLED
#include "core/math/convex_hull.h"
#include "scene/3d/physics/area_3d.h"
#include "scene/3d/physics/physics_body_3d.h"
#include "scene/resources/3d/importer_mesh.h"
#include "scene/resources/3d/shape_3d.h"
#endif // PHYSICS_3D_DISABLED

// ============================================================================
// GLTFLight
// ============================================================================

void GLTFLight::set_cone_inner_attenuation_conversion_expressions(
	Ref<GLTFObjectModelProperty>& r_obj_model_prop)
{
	// Expression to convert glTF innerConeAngle to Godot spot_angle_attenuation.
	Ref<Expression> gltf_to_godot_expr;
	gltf_to_godot_expr.instantiate();
	PackedStringArray gltf_to_godot_args = {"inner_cone_angle"};
	gltf_to_godot_expr->parse(
		"0.2 / (1.0 - inner_cone_angle / spot_angle) - 0.1", gltf_to_godot_args);
	r_obj_model_prop->set_gltf_to_godot_expression(gltf_to_godot_expr);
	// Expression to convert Godot spot_angle_attenuation to glTF innerConeAngle.
	Ref<Expression> godot_to_gltf_expr;
	godot_to_gltf_expr.instantiate();
	PackedStringArray godot_to_gltf_args = {"godot_spot_angle_att"};
	godot_to_gltf_expr->parse(
		"spot_angle * maxf(0.0, 1.0 - (0.2 / (0.1 + godot_spot_angle_att)))", godot_to_gltf_args);
	r_obj_model_prop->set_godot_to_gltf_expression(godot_to_gltf_expr);
}

Color GLTFLight::get_color() { return color; }

void GLTFLight::set_color(Color p_color) { color = p_color; }

float GLTFLight::get_intensity() { return intensity; }

void GLTFLight::set_intensity(float p_intensity) { intensity = p_intensity; }

String GLTFLight::get_light_type() { return light_type; }

void GLTFLight::set_light_type(String p_light_type) { light_type = p_light_type; }

float GLTFLight::get_range() { return range; }

void GLTFLight::set_range(float p_range) { range = p_range; }

float GLTFLight::get_inner_cone_angle() { return inner_cone_angle; }

void GLTFLight::set_inner_cone_angle(float p_inner_cone_angle)
{
	inner_cone_angle = p_inner_cone_angle;
}

float GLTFLight::get_outer_cone_angle() { return outer_cone_angle; }

void GLTFLight::set_outer_cone_angle(float p_outer_cone_angle)
{
	outer_cone_angle = p_outer_cone_angle;
}

Ref<GLTFLight> GLTFLight::from_node(const Light3D*) { return Ref<GLTFLight>(); }

// ============================================================================
// GLTFSpecGloss
// ============================================================================

Ref<Image> GLTFSpecGloss::get_diffuse_img() { return diffuse_img; }

void GLTFSpecGloss::set_diffuse_img(Ref<Image> p_diffuse_img) { diffuse_img = p_diffuse_img; }

Color GLTFSpecGloss::get_diffuse_factor() { return diffuse_factor; }

void GLTFSpecGloss::set_diffuse_factor(Color p_diffuse_factor)
{
	diffuse_factor = p_diffuse_factor;
}

float GLTFSpecGloss::get_gloss_factor() { return gloss_factor; }

void GLTFSpecGloss::set_gloss_factor(float p_gloss_factor) { gloss_factor = p_gloss_factor; }

Color GLTFSpecGloss::get_specular_factor() { return specular_factor; }

void GLTFSpecGloss::set_specular_factor(Color p_specular_factor)
{
	specular_factor = p_specular_factor;
}

Ref<Image> GLTFSpecGloss::get_spec_gloss_img() { return spec_gloss_img; }

void GLTFSpecGloss::set_spec_gloss_img(Ref<Image> p_spec_gloss_img)
{
	spec_gloss_img = p_spec_gloss_img;
}

// ============================================================================
// GLTFDocumentExtension
// ============================================================================

Error GLTFDocumentExtension::import_preflight(
	Ref<GLTFState> p_state, const Vector<String>& p_extensions)
{
	ERR_FAIL_COND_V(p_state.is_null(), ERR_INVALID_PARAMETER);
	return OK;
}

Error GLTFDocumentExtension::parse_image_data(Ref<GLTFState> p_state,
	const PackedByteArray& p_image_data, const String& p_mime_type, Ref<Image> r_image)
{
	ERR_FAIL_COND_V(p_state.is_null(), ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(r_image.is_null(), ERR_INVALID_PARAMETER);
	return OK;
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
	return OK;
}

Error GLTFDocumentExtension::import_pre_generate(Ref<GLTFState> p_state)
{
	ERR_FAIL_COND_V(p_state.is_null(), ERR_INVALID_PARAMETER);
	return OK;
}

Node3D* GLTFDocumentExtension::generate_scene_node(
	Ref<GLTFState> p_state, Ref<GLTFNode> p_gltf_node, Node* p_scene_parent)
{
	ERR_FAIL_COND_V(p_state.is_null(), nullptr);
	ERR_FAIL_COND_V(p_gltf_node.is_null(), nullptr);
	return nullptr;
}

Error GLTFDocumentExtension::import_post(Ref<GLTFState> p_state, Node* p_root)
{
	ERR_FAIL_NULL_V(p_root, ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(p_state.is_null(), ERR_INVALID_PARAMETER);
	return OK;
}

Error GLTFDocumentExtension::export_preflight(Ref<GLTFState> p_state, Node* p_root)
{
	ERR_FAIL_NULL_V(p_root, ERR_INVALID_PARAMETER);
	return OK;
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
	return OK;
}

Error GLTFDocumentExtension::export_preserialize(Ref<GLTFState> p_state)
{
	ERR_FAIL_COND_V(p_state.is_null(), ERR_INVALID_PARAMETER);
	return OK;
}

Error GLTFDocumentExtension::save_image_at_path(Ref<GLTFState> p_state, Ref<Image> p_image,
	const String& p_file_path, const String& p_image_format, float p_lossy_quality)
{
	ERR_FAIL_COND_V(p_state.is_null(), ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(p_image.is_null(), ERR_INVALID_PARAMETER);
	return OK;
}

Error GLTFDocumentExtension::export_post(Ref<GLTFState> p_state)
{
	ERR_FAIL_COND_V(p_state.is_null(), ERR_INVALID_PARAMETER);
	return OK;
}

PackedStringArray GLTFDocumentExtension::get_supported_extensions() { return PackedStringArray(); }

String GLTFDocumentExtension::get_image_file_extension() { return String(); }

PackedStringArray GLTFDocumentExtension::get_saveable_image_formats()
{
	return PackedStringArray();
}

// ============================================================================
// GLTFDocumentExtensionTexture
// ============================================================================

Error GLTFDocumentExtensionTexture::import_preflight(
	Ref<GLTFState> p_state, const Vector<String>& p_extensions)
{
	const String ext_name = (format == FORMAT_WEBP) ? "EXT_texture_webp" : "KHR_texture_basisu";
	if (!p_extensions.has(ext_name)) {
		return ERR_SKIP;
	}
	return OK;
}

Vector<String> GLTFDocumentExtensionTexture::get_supported_extensions()
{
	Vector<String> ret;
	ret.push_back((format == FORMAT_WEBP) ? "EXT_texture_webp" : "KHR_texture_basisu");
	return ret;
}

Error GLTFDocumentExtensionTexture::parse_image_data(Ref<GLTFState> p_state,
	const PackedByteArray& p_image_data, const String& p_mime_type, Ref<Image> r_image)
{
	if (format == FORMAT_WEBP) {
		if (p_mime_type == "image/webp") {
			return r_image->load_webp_from_buffer(p_image_data);
		}
	}
	else {
		if (p_mime_type == "image/ktx2") {
			return r_image->load_ktx_from_buffer(p_image_data);
		}
	}
	return OK;
}

String GLTFDocumentExtensionTexture::get_image_file_extension()
{
	if (format == FORMAT_WEBP) {
		return ".webp";
	}
	return String();
}

Vector<String> GLTFDocumentExtensionTexture::get_saveable_image_formats()
{
	Vector<String> ret;
	if (format == FORMAT_WEBP) {
		ret.push_back("Lossless WebP");
		ret.push_back("Lossy WebP");
	}
	return ret;
}

Error GLTFDocumentExtensionTexture::save_image_at_path(Ref<GLTFState> p_state,
	Ref<Image> p_image, const String& p_file_path, const String& p_image_format,
	float p_lossy_quality)
{
	if (format == FORMAT_WEBP) {
		if (p_image_format == "Lossless WebP") {
			p_image->save_webp(p_file_path, false);
			return OK;
		}
		else if (p_image_format == "Lossy WebP") {
			p_image->save_webp(p_file_path, true, p_lossy_quality);
			return OK;
		}
		return ERR_INVALID_PARAMETER;
	}
	return GLTFDocumentExtension::save_image_at_path(
		p_state, p_image, p_file_path, p_image_format, p_lossy_quality);
}

#ifndef PHYSICS_3D_DISABLED

// ============================================================================
// GLTFPhysicsBody
// ============================================================================

String GLTFPhysicsBody::get_body_type() const
{
	switch (body_type) {
	case PhysicsBodyType::STATIC:
		return "static";
	case PhysicsBodyType::ANIMATABLE:
		return "animatable";
	case PhysicsBodyType::CHARACTER:
		return "character";
	case PhysicsBodyType::RIGID:
		return "rigid";
	case PhysicsBodyType::VEHICLE:
		return "vehicle";
	case PhysicsBodyType::TRIGGER:
		return "trigger";
	}
	return "rigid";
}

void GLTFPhysicsBody::set_body_type(const String& p_body_type)
{
	if (p_body_type == "static") {
		body_type = PhysicsBodyType::STATIC;
	}
	else if (p_body_type == "animatable") {
		body_type = PhysicsBodyType::ANIMATABLE;
	}
	else if (p_body_type == "character") {
		body_type = PhysicsBodyType::CHARACTER;
	}
	else if (p_body_type == "rigid") {
		body_type = PhysicsBodyType::RIGID;
	}
	else if (p_body_type == "vehicle") {
		body_type = PhysicsBodyType::VEHICLE;
	}
	else if (p_body_type == "trigger") {
		body_type = PhysicsBodyType::TRIGGER;
	}
	else {
		ERR_PRINT("Error setting glTF physics body type: The body type must be one of \"static\", "
				  "\"animatable\", \"character\", \"rigid\", \"vehicle\", or \"trigger\".");
	}
}

GLTFPhysicsBody::PhysicsBodyType GLTFPhysicsBody::get_physics_body_type() const
{
	return body_type;
}

void GLTFPhysicsBody::set_physics_body_type(PhysicsBodyType p_body_type)
{
	body_type = p_body_type;
}

real_t GLTFPhysicsBody::get_mass() const { return mass; }

void GLTFPhysicsBody::set_mass(real_t p_mass) { mass = p_mass; }

Vector3 GLTFPhysicsBody::get_linear_velocity() const { return linear_velocity; }

void GLTFPhysicsBody::set_linear_velocity(const Vector3& p_linear_velocity)
{
	linear_velocity = p_linear_velocity;
}

Vector3 GLTFPhysicsBody::get_angular_velocity() const { return angular_velocity; }

void GLTFPhysicsBody::set_angular_velocity(const Vector3& p_angular_velocity)
{
	angular_velocity = p_angular_velocity;
}

Vector3 GLTFPhysicsBody::get_center_of_mass() const { return center_of_mass; }

void GLTFPhysicsBody::set_center_of_mass(const Vector3& p_center_of_mass)
{
	center_of_mass = p_center_of_mass;
}

Vector3 GLTFPhysicsBody::get_inertia_diagonal() const { return inertia_diagonal; }

void GLTFPhysicsBody::set_inertia_diagonal(const Vector3& p_inertia_diagonal)
{
	inertia_diagonal = p_inertia_diagonal;
}

Quaternion GLTFPhysicsBody::get_inertia_orientation() const { return inertia_orientation; }

void GLTFPhysicsBody::set_inertia_orientation(const Quaternion& p_inertia_orientation)
{
	inertia_orientation = p_inertia_orientation;
}

#ifndef DISABLE_DEPRECATED
Basis GLTFPhysicsBody::get_inertia_tensor() const { return Basis::from_scale(inertia_diagonal); }

void GLTFPhysicsBody::set_inertia_tensor(const Basis& p_inertia_tensor)
{
	inertia_diagonal = p_inertia_tensor.get_main_diagonal();
}
#endif // DISABLE_DEPRECATED

Node3D* GLTFPhysicsBody::to_node() const
{
	switch (body_type) {
	case PhysicsBodyType::CHARACTER: {
		return memnew(PhysicsBody3D(PhysicsBody3D::MODE_CHARACTER));
	}
	case PhysicsBodyType::ANIMATABLE: {
		return memnew(PhysicsBody3D(PhysicsBody3D::MODE_ANIMATABLE));
	}
	case PhysicsBodyType::VEHICLE: {
		PhysicsBody3D* body = memnew(PhysicsBody3D(PhysicsBody3D::MODE_VEHICLE));
		body->set_mass(mass);
		body->set_linear_velocity(linear_velocity);
		body->set_angular_velocity(angular_velocity);
		body->set_inertia(inertia_diagonal);
		body->set_center_of_mass_mode(PhysicsBody3D::CENTER_OF_MASS_MODE_CUSTOM);
		body->set_center_of_mass(center_of_mass);
		return body;
	}
	case PhysicsBodyType::RIGID: {
		PhysicsBody3D* body = memnew(PhysicsBody3D(PhysicsBody3D::MODE_RIGID));
		body->set_mass(mass);
		body->set_linear_velocity(linear_velocity);
		body->set_angular_velocity(angular_velocity);
		body->set_inertia(inertia_diagonal);
		body->set_center_of_mass_mode(PhysicsBody3D::CENTER_OF_MASS_MODE_CUSTOM);
		body->set_center_of_mass(center_of_mass);
		return body;
	}
	case PhysicsBodyType::STATIC: {
		return memnew(PhysicsBody3D(PhysicsBody3D::MODE_STATIC));
	}
	case PhysicsBodyType::TRIGGER: {
		return memnew(Area3D);
	}
	}
	return nullptr;
}

// ============================================================================
// GLTFPhysicsShape
// ============================================================================

String GLTFPhysicsShape::get_shape_type() const { return shape_type; }

void GLTFPhysicsShape::set_shape_type(const String& p_shape_type) { shape_type = p_shape_type; }

Vector3 GLTFPhysicsShape::get_size() const { return size; }

void GLTFPhysicsShape::set_size(const Vector3& p_size) { size = p_size; }

real_t GLTFPhysicsShape::get_radius() const { return radius; }

void GLTFPhysicsShape::set_radius(real_t p_radius) { radius = p_radius; }

real_t GLTFPhysicsShape::get_height() const { return height; }

void GLTFPhysicsShape::set_height(real_t p_height) { height = p_height; }

bool GLTFPhysicsShape::get_is_trigger() const { return is_trigger; }

void GLTFPhysicsShape::set_is_trigger(bool p_is_trigger) { is_trigger = p_is_trigger; }

GLTFMeshIndex GLTFPhysicsShape::get_mesh_index() const { return mesh_index; }

void GLTFPhysicsShape::set_mesh_index(GLTFMeshIndex p_mesh_index) { mesh_index = p_mesh_index; }

Ref<ImporterMesh> GLTFPhysicsShape::get_importer_mesh() const { return importer_mesh; }

void GLTFPhysicsShape::set_importer_mesh(const Ref<ImporterMesh>& p_importer_mesh)
{
	importer_mesh = p_importer_mesh;
}

CollisionShape3D* GLTFPhysicsShape::to_node(bool p_cache_shapes)
{
	CollisionShape3D* godot_shape_node = memnew(CollisionShape3D);
	to_resource(p_cache_shapes); // Sets `_shape_cache`.
	godot_shape_node->set_shape(_shape_cache);
	return godot_shape_node;
}

Ref<Shape3D> GLTFPhysicsShape::to_resource(bool p_cache_shapes)
{
	if (!p_cache_shapes || _shape_cache.is_null()) {
		if (shape_type == "box") {
			Ref<Shape3D> box = memnew(Shape3D(Shape3D::SHAPE_BOX));
			box->set_size(size);
			_shape_cache = box;
		}
		else if (shape_type == "capsule") {
			Ref<Shape3D> capsule = memnew(Shape3D(Shape3D::SHAPE_CAPSULE));
			capsule->set_radius(radius);
			capsule->set_height(height);
			_shape_cache = capsule;
		}
		else if (shape_type == "cylinder") {
			Ref<Shape3D> cylinder = memnew(Shape3D(Shape3D::SHAPE_CYLINDER));
			cylinder->set_radius(radius);
			cylinder->set_height(height);
			_shape_cache = cylinder;
		}
		else if (shape_type == "sphere") {
			Ref<Shape3D> sphere = memnew(Shape3D(Shape3D::SHAPE_SPHERE));
			sphere->set_radius(radius);
			_shape_cache = sphere;
		}
		else if (shape_type == "convex") {
			ERR_FAIL_COND_V_MSG(importer_mesh.is_null(), _shape_cache,
				"GLTFPhysicsShape: Error converting convex hull shape to a shape resource: The "
				"mesh resource is null.");
			Ref<Shape3D> convex = importer_mesh->get_mesh()->create_convex_shape();
			_shape_cache = convex;
		}
		else if (shape_type == "trimesh") {
			ERR_FAIL_COND_V_MSG(importer_mesh.is_null(), _shape_cache,
				"GLTFPhysicsShape: Error converting concave mesh shape to a shape resource: The "
				"mesh resource is null.");
			Ref<Shape3D> concave = importer_mesh->create_trimesh_shape();
			_shape_cache = concave;
		}
		else {
			ERR_PRINT("GLTFPhysicsShape: Error converting to a shape resource: Shape type '" +
					  shape_type + "' is unknown.");
		}
	}
	return _shape_cache;
}

// ============================================================================
// GLTFDocumentExtensionPhysics
// ============================================================================

using GLTFShapeIndex = int64_t;

Vector<String> GLTFDocumentExtensionPhysics::get_supported_extensions()
{
	Vector<String> ret;
	ret.push_back("OMI_collider");
	ret.push_back("OMI_physics_body");
	ret.push_back("OMI_physics_shape");
	return ret;
}

void _setup_shape_mesh_resource_from_index_if_needed(
	const Ref<GLTFState>& p_state, const Ref<GLTFPhysicsShape>& p_gltf_shape)
{
	GLTFMeshIndex shape_mesh_index = p_gltf_shape->get_mesh_index();
	if (shape_mesh_index == -1) {
		return;
	}
	Ref<ImporterMesh> importer_mesh = p_gltf_shape->get_importer_mesh();
	if (importer_mesh.is_valid()) {
		return;
	}
	const Vector<Ref<GLTFMesh>>& state_meshes = p_state->get_meshes();
	ERR_FAIL_INDEX_MSG(shape_mesh_index, state_meshes.size(),
		"glTF Physics: When importing '" + p_state->get_scene_name() + "', the shape mesh index " +
			itos(shape_mesh_index) +
			" is not in the state meshes (size: " + itos(state_meshes.size()) + ").");
	const Ref<GLTFMesh>& gltf_mesh = state_meshes[shape_mesh_index];
	ERR_FAIL_COND(gltf_mesh.is_null());
	importer_mesh = gltf_mesh->get_mesh();
	ERR_FAIL_COND(importer_mesh.is_null());
	p_gltf_shape->set_importer_mesh(importer_mesh);
}

#ifndef DISABLE_DEPRECATED
Node3D* _generate_shape_with_body(Ref<GLTFState> p_state, Ref<GLTFNode> p_gltf_node,
	Ref<GLTFPhysicsShape> p_physics_shape, Ref<GLTFPhysicsBody> p_physics_body)
{
	print_verbose("glTF: Creating shape with body for: " + p_gltf_node->get_name());
	bool is_trigger = p_physics_shape->get_is_trigger();
	Node3D* body;
	if (p_physics_body.is_valid()) {
		body = p_physics_body->to_node();
		if (is_trigger && (p_physics_body->get_body_type() != "trigger")) {
			Node3D* child =
				_generate_shape_with_body(p_state, p_gltf_node, p_physics_shape, nullptr);
			child->set_name(
				p_gltf_node->get_name() + (is_trigger ? String("Trigger") : String("Solid")));
			body->add_child(child);
			return body;
		}
	}
	else if (is_trigger) {
		body = memnew(Area3D);
	}
	else {
		body = memnew(PhysicsBody3D(PhysicsBody3D::MODE_STATIC));
	}
	CollisionShape3D* shape = p_physics_shape->to_node();
	shape->set_name(p_gltf_node->get_name() + "Shape");
	body->add_child(shape);
	return body;
}
#endif // DISABLE_DEPRECATED

bool _are_all_faces_equal(const Vector<Face3>& p_a, const Vector<Face3>& p_b)
{
	if (p_a.size() != p_b.size()) {
		return false;
	}
	for (int i = 0; i < p_a.size(); i++) {
		const Vector3* a_vertices = p_a[i].vertex;
		const Vector3* b_vertices = p_b[i].vertex;
		for (int j = 0; j < 3; j++) {
			if (!a_vertices[j].is_equal_approx(b_vertices[j])) {
				return false;
			}
		}
	}
	return true;
}

GLTFMeshIndex _get_or_insert_mesh_in_state(
	const Ref<GLTFState>& p_state, const Ref<ImporterMesh>& p_mesh)
{
	ERR_FAIL_COND_V(p_mesh.is_null(), -1);
	Vector<Ref<GLTFMesh>> state_meshes = p_state->get_meshes();
	Vector<Face3> mesh_faces = p_mesh->get_faces();
	for (GLTFMeshIndex i = 0; i < state_meshes.size(); i++) {
		const Ref<GLTFMesh>& state_gltf_mesh = state_meshes[i];
		ERR_CONTINUE(state_gltf_mesh.is_null());
		Ref<ImporterMesh> state_importer_mesh = state_gltf_mesh->get_mesh();
		ERR_CONTINUE(state_importer_mesh.is_null());
		if (state_importer_mesh == p_mesh) {
			return i;
		}
		if (_are_all_faces_equal(state_importer_mesh->get_faces(), mesh_faces)) {
			return i;
		}
	}
	Ref<GLTFMesh> gltf_mesh;
	gltf_mesh.instantiate();
	gltf_mesh->set_mesh(p_mesh);
	GLTFMeshIndex mesh_index = state_meshes.size();
	state_meshes.push_back(gltf_mesh);
	p_state->set_meshes(state_meshes);
	return mesh_index;
}

#endif // PHYSICS_3D_DISABLED

// ============================================================================
// Merged from modules/gltf/gltf_structures.cpp
// ============================================================================

#include "gltf_structures.h"

#include "core/types.h"
#include "gltf_state.h"
#include "scene/3d/camera_3d.h"

// ============================================================================
// GLTFTexture
// ============================================================================

GLTFImageIndex GLTFTexture::get_src_image() const { return src_image; }

void GLTFTexture::set_src_image(GLTFImageIndex val) { src_image = val; }

GLTFTextureSamplerIndex GLTFTexture::get_sampler() const { return sampler; }

void GLTFTexture::set_sampler(GLTFTextureSamplerIndex val) { sampler = val; }

// ============================================================================
// GLTFBufferView
// ============================================================================

GLTFBufferIndex GLTFBufferView::get_buffer() const { return buffer; }

void GLTFBufferView::set_buffer(GLTFBufferIndex p_buffer) { buffer = p_buffer; }

int64_t GLTFBufferView::get_byte_offset() const { return byte_offset; }

void GLTFBufferView::set_byte_offset(int64_t p_byte_offset) { byte_offset = p_byte_offset; }

int64_t GLTFBufferView::get_byte_length() const { return byte_length; }

void GLTFBufferView::set_byte_length(int64_t p_byte_length) { byte_length = p_byte_length; }

int64_t GLTFBufferView::get_byte_stride() const { return byte_stride; }

void GLTFBufferView::set_byte_stride(int64_t p_byte_stride) { byte_stride = p_byte_stride; }

bool GLTFBufferView::get_indices() const { return indices; }

void GLTFBufferView::set_indices(bool p_indices) { indices = p_indices; }

bool GLTFBufferView::get_vertex_attributes() const { return vertex_attributes; }

void GLTFBufferView::set_vertex_attributes(bool p_attributes) { vertex_attributes = p_attributes; }

Vector<uint8_t> GLTFBufferView::load_buffer_view_data(const Ref<GLTFState> p_gltf_state) const
{
	ERR_FAIL_COND_V(p_gltf_state.is_null(), Vector<uint8_t>());
	const Vector<PackedByteArray>& buffers = p_gltf_state->get_buffers();
	ERR_FAIL_INDEX_V(buffer, buffers.size(), Vector<uint8_t>());
	const PackedByteArray& buffer_data = buffers[buffer];
	const int64_t byte_end = byte_offset + byte_length;
	// Note that for buffer views with a byte stride, the parts of this data which get used may
	// only be determined in combination with the accessors that reference this buffer view.
	return buffer_data.slice(byte_offset, byte_end);
}

GLTFBufferViewIndex GLTFBufferView::write_new_buffer_view_into_state(
	const Ref<GLTFState>& p_gltf_state, const PackedByteArray& p_input_data,
	const int64_t p_alignment, const ArrayBufferTarget p_target, const int64_t p_byte_stride,
	const GLTFBufferIndex p_buffer_index, const bool p_deduplicate)
{
	ERR_FAIL_COND_V_MSG(
		p_buffer_index < 0, -1, "Buffer index must be greater than or equal to zero.");
	const bool target_is_indices = p_target == ArrayBufferTarget::TARGET_ELEMENT_ARRAY_BUFFER;
	const bool target_is_vertex_attributes = p_target == ArrayBufferTarget::TARGET_ARRAY_BUFFER;
	if (target_is_vertex_attributes) {
		ERR_FAIL_COND_V_MSG(p_byte_stride < 4 || p_byte_stride % 4 != 0, -1,
			"glTF export: Vertex attributes using TARGET_ARRAY_BUFFER must have a byte stride that "
			"is a multiple of 4 as required by section 3.6.2.4 of the glTF specification.");
	}
	// Check for duplicate buffer views before adding a new one.
	Vector<Ref<GLTFBufferView>> state_buffer_views = p_gltf_state->get_buffer_views();
	const int buffer_view_index = state_buffer_views.size();
	if (p_deduplicate) {
		for (int i = 0; i < buffer_view_index; i++) {
			const Ref<GLTFBufferView>& existing_buffer_view = state_buffer_views[i];
			if (existing_buffer_view->get_byte_offset() % p_alignment == 0 &&
				existing_buffer_view->get_byte_length() == p_input_data.size() &&
				existing_buffer_view->get_byte_stride() == p_byte_stride &&
				existing_buffer_view->get_indices() == target_is_indices &&
				existing_buffer_view->get_vertex_attributes() == target_is_vertex_attributes) {
				if (existing_buffer_view->load_buffer_view_data(p_gltf_state) == p_input_data) {
					// Duplicate found, return the index of the existing buffer view.
					return i;
				}
			}
		}
	}
	// Write the data into the buffer at the specified index.
	Vector<PackedByteArray> state_buffers = p_gltf_state->get_buffers();
	if (state_buffers.size() <= p_buffer_index) {
		state_buffers.resize(p_buffer_index + 1);
	}
	PackedByteArray state_buffer = state_buffers[p_buffer_index];
	const int64_t input_data_size = p_input_data.size();
	// This is used by accessors. The byte offset of an accessor MUST be a multiple of the
	// accessor's component size.
	// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#data-alignment
	int64_t byte_offset = state_buffer.size();
	int64_t padding_bytes = 0;
	if (byte_offset % p_alignment != 0) {
		padding_bytes = p_alignment - (byte_offset % p_alignment);
		byte_offset += padding_bytes;
	}
	state_buffer.resize(byte_offset + input_data_size);
	if (padding_bytes > 0) {
		memset(state_buffer.ptrw() + (byte_offset - padding_bytes), 0, padding_bytes);
	}
	uint8_t* buffer_ptr = state_buffer.ptrw();
	memcpy(buffer_ptr + byte_offset, p_input_data.ptr(), input_data_size);
	state_buffers.set(p_buffer_index, state_buffer);
	p_gltf_state->set_buffers(state_buffers);
	// Create a new GLTFBufferView that references the new buffer.
	Ref<GLTFBufferView> buffer_view;
	buffer_view.instantiate();
	buffer_view->set_buffer(p_buffer_index);
	buffer_view->set_byte_offset(byte_offset);
	buffer_view->set_byte_length(input_data_size);
	buffer_view->set_byte_stride(p_byte_stride);
	buffer_view->set_indices(target_is_indices);
	buffer_view->set_vertex_attributes(target_is_vertex_attributes);
	// Add the new buffer view to the state.
	state_buffer_views.append(buffer_view);
	p_gltf_state->set_buffer_views(state_buffer_views);
	return buffer_view_index;
}

// ============================================================================
// GLTFAccessor
// ============================================================================

GLTFBufferViewIndex GLTFAccessor::get_buffer_view() const { return buffer_view; }

void GLTFAccessor::set_buffer_view(GLTFBufferViewIndex p_buffer_view)
{
	buffer_view = p_buffer_view;
}

int64_t GLTFAccessor::get_byte_offset() const { return byte_offset; }

void GLTFAccessor::set_byte_offset(int64_t p_byte_offset) { byte_offset = p_byte_offset; }

GLTFAccessor::GLTFComponentType GLTFAccessor::get_component_type() const { return component_type; }

void GLTFAccessor::set_component_type(GLTFComponentType p_component_type)
{
	component_type = (GLTFComponentType)p_component_type;
}

bool GLTFAccessor::get_normalized() const { return normalized; }

void GLTFAccessor::set_normalized(bool p_normalized) { normalized = p_normalized; }

int64_t GLTFAccessor::get_count() const { return count; }

void GLTFAccessor::set_count(int64_t p_count) { count = p_count; }

GLTFAccessor::GLTFAccessorType GLTFAccessor::get_accessor_type() const { return accessor_type; }

void GLTFAccessor::set_accessor_type(GLTFAccessorType p_accessor_type)
{
	accessor_type = p_accessor_type;
}

int GLTFAccessor::get_type() const { return (int)accessor_type; }

void GLTFAccessor::set_type(int p_accessor_type)
{
	accessor_type = (GLTFAccessorType)p_accessor_type;
}

Vector<double> GLTFAccessor::get_min() const { return Vector<double>(min); }

void GLTFAccessor::set_min(const Vector<double>& p_min) { min = Vector<double>(p_min); }

Vector<double> GLTFAccessor::get_max() const { return Vector<double>(max); }

void GLTFAccessor::set_max(const Vector<double>& p_max) { max = Vector<double>(p_max); }

int64_t GLTFAccessor::get_sparse_count() const { return sparse_count; }

void GLTFAccessor::set_sparse_count(int64_t p_sparse_count) { sparse_count = p_sparse_count; }

GLTFBufferViewIndex GLTFAccessor::get_sparse_indices_buffer_view() const
{
	return sparse_indices_buffer_view;
}

void GLTFAccessor::set_sparse_indices_buffer_view(GLTFBufferViewIndex p_sparse_indices_buffer_view)
{
	sparse_indices_buffer_view = p_sparse_indices_buffer_view;
}

int64_t GLTFAccessor::get_sparse_indices_byte_offset() const { return sparse_indices_byte_offset; }

void GLTFAccessor::set_sparse_indices_byte_offset(int64_t p_sparse_indices_byte_offset)
{
	sparse_indices_byte_offset = p_sparse_indices_byte_offset;
}

GLTFAccessor::GLTFComponentType GLTFAccessor::get_sparse_indices_component_type() const
{
	return sparse_indices_component_type;
}

void GLTFAccessor::set_sparse_indices_component_type(
	GLTFComponentType p_sparse_indices_component_type)
{
	sparse_indices_component_type = (GLTFComponentType)p_sparse_indices_component_type;
}

GLTFBufferViewIndex GLTFAccessor::get_sparse_values_buffer_view() const
{
	return sparse_values_buffer_view;
}

void GLTFAccessor::set_sparse_values_buffer_view(GLTFBufferViewIndex p_sparse_values_buffer_view)
{
	sparse_values_buffer_view = p_sparse_values_buffer_view;
}

int64_t GLTFAccessor::get_sparse_values_byte_offset() const { return sparse_values_byte_offset; }

void GLTFAccessor::set_sparse_values_byte_offset(int64_t p_sparse_values_byte_offset)
{
	sparse_values_byte_offset = p_sparse_values_byte_offset;
}

void GLTFAccessor::_calculate_min_and_max(const PackedFloat64Array& p_numbers)
{
	const int64_t vector_size = _get_vector_size();
	ERR_FAIL_COND(vector_size <= 0 || p_numbers.size() % vector_size != 0);
	min.resize(vector_size);
	max.resize(vector_size);
	// Initialize min and max with the first vector element values.
	for (int64_t in_vec = 0; in_vec < vector_size; in_vec++) {
		min.write[in_vec] = p_numbers[in_vec];
		max.write[in_vec] = p_numbers[in_vec];
	}
	// Iterate over the rest of the vectors.
	for (int64_t which_vec = vector_size; which_vec < p_numbers.size(); which_vec += vector_size) {
		for (int64_t in_vec = 0; in_vec < vector_size; in_vec++) {
			min.write[in_vec] = MIN(p_numbers[which_vec + in_vec], min[in_vec]);
			max.write[in_vec] = MAX(p_numbers[which_vec + in_vec], max[in_vec]);
		}
	}
	// 3.6.2.5: For floating-point components, JSON-stored minimum and maximum values represent
	// single precision floats and SHOULD be rounded to single precision before usage to avoid any
	// potential boundary mismatches.
	// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#accessors-bounds
	if (component_type == GLTFAccessor::COMPONENT_TYPE_SINGLE_FLOAT) {
		for (int64_t i = 0; i < min.size(); i++) {
			min.write[i] = (double)(float)min[i];
			max.write[i] = (double)(float)max[i];
		}
	}
}

void GLTFAccessor::_determine_pad_skip(int64_t& r_skip_every, int64_t& r_skip_bytes) const
{
	// 3.6.2.4. Accessors of matrix type have data stored in column-major order. The start of each
	// column MUST be aligned to 4-byte boundaries.
	// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#data-alignment
	switch (component_type) {
	case GLTFAccessor::COMPONENT_TYPE_SIGNED_BYTE:
	case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_BYTE: {
		if (accessor_type == GLTFAccessor::TYPE_MAT2) {
			r_skip_every = 2;
			r_skip_bytes = 2;
		}
		if (accessor_type == GLTFAccessor::TYPE_MAT3) {
			r_skip_every = 3;
			r_skip_bytes = 1;
		}
	} break;
	case GLTFAccessor::COMPONENT_TYPE_SIGNED_SHORT:
	case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_SHORT: {
		if (accessor_type == GLTFAccessor::TYPE_MAT3) {
			r_skip_every = 6;
			r_skip_bytes = 2;
		}
	} break;
	default: {
	} break;
	}
}

int64_t GLTFAccessor::_determine_padded_byte_count(int64_t p_raw_byte_size) const
{
	// 3.6.2.4. Accessors of matrix type have data stored in column-major order. The start of each
	// column MUST be aligned to 4-byte boundaries.
	// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#data-alignment
	switch (component_type) {
	case GLTFAccessor::COMPONENT_TYPE_SIGNED_BYTE:
	case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_BYTE: {
		if (accessor_type == GLTFAccessor::TYPE_MAT2) {
			return p_raw_byte_size * 2;
		}
		if (accessor_type == GLTFAccessor::TYPE_MAT3) {
			return p_raw_byte_size * 4 / 3;
		}
	} break;
	case GLTFAccessor::COMPONENT_TYPE_SIGNED_SHORT:
	case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_SHORT: {
		if (accessor_type == GLTFAccessor::TYPE_MAT3) {
			return p_raw_byte_size * 4 / 3;
		}
	} break;
	default: {
	} break;
	}
	return p_raw_byte_size;
}

PackedFloat64Array GLTFAccessor::_filter_numbers(const PackedFloat64Array& p_numbers) const
{
	PackedFloat64Array filtered_numbers = p_numbers;
	for (int64_t i = 0; i < p_numbers.size(); i++) {
		const double num = p_numbers[i];
		if (!Math::is_finite(num)) {
			// 3.6.2.2. "Values of NaN, +Infinity, and -Infinity MUST NOT be present."
			// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#accessor-data-types
			filtered_numbers.set(i, 0.0);
		}
		else if (component_type == GLTFAccessor::COMPONENT_TYPE_SINGLE_FLOAT) {
			filtered_numbers.set(i, (double)(float)num);
		}
	}
	return filtered_numbers;
}

String GLTFAccessor::_get_component_type_name(const GLTFComponentType p_component)
{
	// These names are only for debugging and printing error messages, glTF uses the numeric values.
	switch (p_component) {
	case GLTFAccessor::COMPONENT_TYPE_NONE:
		return "None";
	case GLTFAccessor::COMPONENT_TYPE_SIGNED_BYTE:
		return "Byte";
	case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_BYTE:
		return "UByte";
	case GLTFAccessor::COMPONENT_TYPE_SIGNED_SHORT:
		return "Short";
	case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_SHORT:
		return "UShort";
	case GLTFAccessor::COMPONENT_TYPE_SIGNED_INT:
		return "Int";
	case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_INT:
		return "UInt";
	case GLTFAccessor::COMPONENT_TYPE_SINGLE_FLOAT:
		return "Float";
	case GLTFAccessor::COMPONENT_TYPE_DOUBLE_FLOAT:
		return "Double";
	case GLTFAccessor::COMPONENT_TYPE_HALF_FLOAT:
		return "Half";
	case GLTFAccessor::COMPONENT_TYPE_SIGNED_LONG:
		return "Long";
	case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_LONG:
		return "ULong";
	}

	return "<Error>";
}

GLTFAccessor::GLTFComponentType GLTFAccessor::_get_indices_component_type_for_size(
	const int64_t p_size)
{
	ERR_FAIL_COND_V(p_size < 0, GLTFAccessor::COMPONENT_TYPE_NONE);
	// 3.7.2.1. indices accessor MUST NOT contain the maximum possible value for the component type
	// used (i.e., 255 for unsigned bytes, 65535 for unsigned shorts, 4294967295 for unsigned ints).
	// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#meshes-overview
	if (unlikely(p_size > 4294967294LL)) {
		return GLTFAccessor::COMPONENT_TYPE_UNSIGNED_LONG;
	}
	if (p_size > 65534LL) {
		return GLTFAccessor::COMPONENT_TYPE_UNSIGNED_INT;
	}
	if (p_size > 254LL) {
		return GLTFAccessor::COMPONENT_TYPE_UNSIGNED_SHORT;
	}
	return GLTFAccessor::COMPONENT_TYPE_UNSIGNED_BYTE;
}

GLTFAccessor::GLTFAccessorType GLTFAccessor::_get_accessor_type_from_str(const String& p_string)
{
	if (p_string == "SCALAR") {
		return GLTFAccessor::TYPE_SCALAR;
	}
	if (p_string == "VEC2") {
		return GLTFAccessor::TYPE_VEC2;
	}
	if (p_string == "VEC3") {
		return GLTFAccessor::TYPE_VEC3;
	}
	if (p_string == "VEC4") {
		return GLTFAccessor::TYPE_VEC4;
	}
	if (p_string == "MAT2") {
		return GLTFAccessor::TYPE_MAT2;
	}
	if (p_string == "MAT3") {
		return GLTFAccessor::TYPE_MAT3;
	}
	if (p_string == "MAT4") {
		return GLTFAccessor::TYPE_MAT4;
	}
	ERR_FAIL_V(GLTFAccessor::TYPE_SCALAR);
}

String GLTFAccessor::_get_accessor_type_name() const
{
	switch (accessor_type) {
	case GLTFAccessor::TYPE_SCALAR:
		return "SCALAR";
	case GLTFAccessor::TYPE_VEC2:
		return "VEC2";
	case GLTFAccessor::TYPE_VEC3:
		return "VEC3";
	case GLTFAccessor::TYPE_VEC4:
		return "VEC4";
	case GLTFAccessor::TYPE_MAT2:
		return "MAT2";
	case GLTFAccessor::TYPE_MAT3:
		return "MAT3";
	case GLTFAccessor::TYPE_MAT4:
		return "MAT4";
	default:
		break;
	}
	ERR_FAIL_V("SCALAR");
}

int64_t GLTFAccessor::_get_vector_size() const
{
	switch (accessor_type) {
	case GLTFAccessor::TYPE_SCALAR:
		return 1;
	case GLTFAccessor::TYPE_VEC2:
		return 2;
	case GLTFAccessor::TYPE_VEC3:
		return 3;
	case GLTFAccessor::TYPE_VEC4:
		return 4;
	case GLTFAccessor::TYPE_MAT2:
		return 4;
	case GLTFAccessor::TYPE_MAT3:
		return 9;
	case GLTFAccessor::TYPE_MAT4:
		return 16;
	default:
		break;
	}
	ERR_FAIL_V(0);
}

int64_t GLTFAccessor::_get_bytes_per_component(const GLTFComponentType p_component_type)
{
	switch (p_component_type) {
	case GLTFAccessor::COMPONENT_TYPE_NONE:
		ERR_FAIL_V(0);
	case GLTFAccessor::COMPONENT_TYPE_SIGNED_BYTE:
	case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_BYTE:
		return 1;
	case GLTFAccessor::COMPONENT_TYPE_SIGNED_SHORT:
	case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_SHORT:
	case GLTFAccessor::COMPONENT_TYPE_HALF_FLOAT:
		return 2;
	case GLTFAccessor::COMPONENT_TYPE_SIGNED_INT:
	case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_INT:
	case GLTFAccessor::COMPONENT_TYPE_SINGLE_FLOAT:
		return 4;
	case GLTFAccessor::COMPONENT_TYPE_DOUBLE_FLOAT:
	case GLTFAccessor::COMPONENT_TYPE_SIGNED_LONG:
	case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_LONG:
		return 8;
	}
	ERR_FAIL_V(0);
}

int64_t GLTFAccessor::_get_bytes_per_vector() const
{
	const int64_t raw_byte_size = _get_bytes_per_component(component_type) * _get_vector_size();
	return _determine_padded_byte_count(raw_byte_size);
}

bool GLTFAccessor::is_equal_exact(const Ref<GLTFAccessor>& p_other) const
{
	if (p_other.is_null()) {
		return false;
	}
	return (buffer_view == p_other->buffer_view && byte_offset == p_other->byte_offset &&
			component_type == p_other->component_type && normalized == p_other->normalized &&
			count == p_other->count && accessor_type == p_other->accessor_type &&
			min == p_other->min && max == p_other->max && sparse_count == p_other->sparse_count &&
			sparse_indices_buffer_view == p_other->sparse_indices_buffer_view &&
			sparse_indices_byte_offset == p_other->sparse_indices_byte_offset &&
			sparse_indices_component_type == p_other->sparse_indices_component_type &&
			sparse_values_buffer_view == p_other->sparse_values_buffer_view &&
			sparse_values_byte_offset == p_other->sparse_values_byte_offset);
}

PackedInt64Array GLTFAccessor::_decode_sparse_indices(
	const Ref<GLTFState>& p_gltf_state, const Vector<Ref<GLTFBufferView>>& p_buffer_views) const
{
	const int64_t bytes_per_component = _get_bytes_per_component(sparse_indices_component_type);
	PackedInt64Array numbers;
	ERR_FAIL_INDEX_V(sparse_indices_buffer_view, p_buffer_views.size(), numbers);
	const Ref<GLTFBufferView> actual_buffer_view = p_buffer_views[sparse_indices_buffer_view];
	const PackedByteArray raw_bytes = actual_buffer_view->load_buffer_view_data(p_gltf_state);
	const int64_t min_raw_byte_size =
		bytes_per_component * sparse_count + sparse_indices_byte_offset;
	ERR_FAIL_COND_V_MSG(raw_bytes.size() < min_raw_byte_size, numbers,
		"glTF import: Sparse indices buffer view did not have enough bytes to read the expected "
		"number of indices. Returning an empty array.");
	numbers.resize(sparse_count);
	const uint8_t* raw_pointer = raw_bytes.ptr();
	int64_t raw_read_offset = sparse_indices_byte_offset;
	for (int64_t i = 0; i < sparse_count; i++) {
		const uint8_t* raw_source = &raw_pointer[raw_read_offset];
		int64_t number = 0;
		switch (sparse_indices_component_type) {
		case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_BYTE: {
			number = *(uint8_t*)raw_source;
		} break;
		case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_SHORT: {
			number = *(uint16_t*)raw_source;
		} break;
		case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_INT: {
			number = *(uint32_t*)raw_source;
		} break;
		case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_LONG: {
			number = *(uint64_t*)raw_source;
		} break;
		default: {
			ERR_FAIL_V_MSG(PackedInt64Array(),
				"glTF import: Sparse indices must have an unsigned integer "
				"component type. Failed to decode, returning an empty array.");
		}
		}
		numbers.set(i, number);
		raw_read_offset += bytes_per_component;
	}
	ERR_FAIL_COND_V_MSG(raw_read_offset != raw_bytes.size(), numbers,
		"glTF import: Sparse indices buffer view size did not exactly match the expected size.");
	return numbers;
}

template <typename T>
Vector<T> GLTFAccessor::_decode_raw_numbers(const Ref<GLTFState>& p_gltf_state,
	const Vector<Ref<GLTFBufferView>>& p_buffer_views, bool p_sparse_values) const
{
	const int64_t bytes_per_component = _get_bytes_per_component(component_type);
	const int64_t bytes_per_vector = _get_bytes_per_vector();
	const int64_t vector_size = _get_vector_size();
	int64_t pad_skip_every = 0;
	int64_t pad_skip_bytes = 0;
	_determine_pad_skip(pad_skip_every, pad_skip_bytes);
	int64_t raw_vector_count;
	int64_t raw_buffer_view_index;
	int64_t raw_read_offset_start;
	if (p_sparse_values) {
		raw_vector_count = sparse_count;
		raw_buffer_view_index = sparse_values_buffer_view;
		raw_read_offset_start = sparse_values_byte_offset;
	}
	else {
		raw_vector_count = count;
		raw_buffer_view_index = buffer_view;
		raw_read_offset_start = byte_offset;
	}
	const int64_t raw_number_count = raw_vector_count * vector_size;
	Vector<T> ret_numbers;
	if (raw_buffer_view_index == -1) {
		ret_numbers.resize(raw_number_count);
		// No buffer view, so fill with zeros.
		for (int64_t i = 0; i < raw_number_count; i++) {
			ret_numbers.set(i, T(0));
		}
		return ret_numbers;
	}
	ERR_FAIL_INDEX_V(raw_buffer_view_index, p_buffer_views.size(), ret_numbers);
	const Ref<GLTFBufferView> raw_buffer_view = p_buffer_views[raw_buffer_view_index];
	if (raw_buffer_view->get_byte_offset() % bytes_per_component != 0) {
		WARN_PRINT("glTF import: Buffer view byte offset is not a multiple of accessor component "
				   "size. This file is invalid per the glTF specification and will not load "
				   "correctly in some glTF viewers, but Godot will try to load it anyway.");
	}
	if (byte_offset % bytes_per_component != 0) {
		WARN_PRINT("glTF import: Accessor byte offset is not a multiple of accessor component "
				   "size. This file is invalid per the glTF specification and will not load "
				   "correctly in some glTF viewers, but Godot will try to load it anyway.");
	}
	int64_t declared_byte_stride = raw_buffer_view->get_byte_stride();
	int64_t actual_byte_stride = bytes_per_vector;
	int64_t stride_skip_every = 0;
	int64_t stride_skip_bytes = 0;
	if (declared_byte_stride != -1) {
		ERR_FAIL_COND_V_MSG(declared_byte_stride % 4 != 0, ret_numbers,
			"glTF import: The declared buffer view byte stride " + itos(declared_byte_stride) +
				" was not a multiple of 4 as required by glTF. Returning an empty array.");
		if (declared_byte_stride > bytes_per_vector) {
			actual_byte_stride = declared_byte_stride;
			stride_skip_every = vector_size;
			stride_skip_bytes = declared_byte_stride - bytes_per_vector;
		}
	}
	else if (raw_buffer_view->get_vertex_attributes()) {
		print_verbose("WARNING: glTF import: Buffer view byte stride should be declared for vertex "
					  "attributes. Assuming packed data and reading anyway.");
	}
	const int64_t min_raw_byte_size =
		actual_byte_stride * (raw_vector_count - 1) + bytes_per_vector + raw_read_offset_start;
	const PackedByteArray raw_bytes = raw_buffer_view->load_buffer_view_data(p_gltf_state);
	ERR_FAIL_COND_V_MSG(raw_bytes.size() < min_raw_byte_size, ret_numbers,
		"glTF import: The buffer view size was smaller than the minimum required size for the "
		"accessor. Returning an empty array.");
	ret_numbers.resize(raw_number_count);
	const uint8_t* raw_pointer = raw_bytes.ptr();
	int64_t raw_read_offset = raw_read_offset_start;
	for (int64_t i = 0; i < raw_number_count; i++) {
		const uint8_t* raw_source = &raw_pointer[raw_read_offset];
		T number = 0;
		// 3.11. Implementations MUST use following equations to decode real floating-point value f
		// from a normalized integer c and vice-versa.
		// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#animations
		switch (component_type) {
		case GLTFAccessor::COMPONENT_TYPE_NONE: {
			ERR_FAIL_V_MSG(Vector<T>(), "glTF import: Failed to decode buffer view, component type "
										"not set. Returning an empty array.");
		} break;
		case GLTFAccessor::COMPONENT_TYPE_SIGNED_BYTE: {
			int8_t prim = *(int8_t*)raw_source;
			if (normalized) {
				number = T(MAX(double(prim) / 127.0, -1.0));
			}
			else {
				number = T(prim);
			}
		} break;
		case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_BYTE: {
			uint8_t prim = *(uint8_t*)raw_source;
			if (normalized) {
				number = T((double(prim) / 255.0));
			}
			else {
				number = T(prim);
			}
		} break;
		case GLTFAccessor::COMPONENT_TYPE_SIGNED_SHORT: {
			int16_t prim = *(int16_t*)raw_source;
			if (normalized) {
				number = T(MAX(double(prim) / 32767.0, -1.0));
			}
			else {
				number = T(prim);
			}
		} break;
		case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_SHORT: {
			uint16_t prim = *(uint16_t*)raw_source;
			if (normalized) {
				number = T(double(prim) / 65535.0);
			}
			else {
				number = T(prim);
			}
		} break;
		case GLTFAccessor::COMPONENT_TYPE_SIGNED_INT: {
			number = T(*(int32_t*)raw_source);
		} break;
		case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_INT: {
			number = T(*(uint32_t*)raw_source);
		} break;
		case GLTFAccessor::COMPONENT_TYPE_SINGLE_FLOAT: {
			number = T(*(float*)raw_source);
		} break;
		case GLTFAccessor::COMPONENT_TYPE_DOUBLE_FLOAT: {
			number = T(*(double*)raw_source);
		} break;
		case GLTFAccessor::COMPONENT_TYPE_HALF_FLOAT: {
			number = Math::half_to_float(*(uint16_t*)raw_source);
		} break;
		case GLTFAccessor::COMPONENT_TYPE_SIGNED_LONG: {
			number = T(*(int64_t*)raw_source);
		} break;
		case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_LONG: {
			number = T(*(uint64_t*)raw_source);
		} break;
		}
		ret_numbers.set(i, number);
		raw_read_offset += bytes_per_component;
		// Padding and stride skipping are distinct concepts that both need to be handled.
		// For example, a 2-in-1 interleaved MAT3 bytes accessor has both, and would look like:
		// AAA0 AAA0 AAA0 BBB0 BBB0 BBB0 AAA0 AAA0 AAA0 BBB0 BBB0 BBB0
		// The "0" is skipped by the padding, and the "BBB0" is skipped by the stride.
		// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#data-alignment
		if (unlikely(pad_skip_every > 0)) {
			if ((i + 1) % pad_skip_every == 0) {
				raw_read_offset += pad_skip_bytes;
			}
		}
		if (unlikely(stride_skip_every > 0)) {
			if ((i + 1) % stride_skip_every == 0) {
				raw_read_offset += stride_skip_bytes;
			}
		}
	}
	return ret_numbers;
}

template <typename T>
Vector<T> GLTFAccessor::_decode_as_numbers(const Ref<GLTFState>& p_gltf_state) const
{
	const Vector<Ref<GLTFBufferView>>& p_buffer_views = p_gltf_state->get_buffer_views();
	Vector<T> ret_numbers = _decode_raw_numbers<T>(p_gltf_state, p_buffer_views, false);
	if (sparse_count == 0) {
		return ret_numbers;
	}
	// Handle sparse accessors.
	PackedInt64Array sparse_indices = _decode_sparse_indices(p_gltf_state, p_buffer_views);
	ERR_FAIL_COND_V_MSG(sparse_indices.size() != sparse_count, ret_numbers,
		"glTF import: Sparse indices size does not match the sparse count.");
	const int64_t vector_size = _get_vector_size();
	Vector<T> sparse_values = _decode_raw_numbers<T>(p_gltf_state, p_buffer_views, true);
	ERR_FAIL_COND_V_MSG(sparse_values.size() != sparse_count * vector_size, ret_numbers,
		"glTF import: Sparse values size does not match the sparse count.");
	for (int64_t in_sparse = 0; in_sparse < sparse_count; in_sparse++) {
		const int64_t sparse_index = sparse_indices[in_sparse];
		const int64_t array_offset = sparse_index * vector_size;
		ERR_FAIL_INDEX_V_MSG(array_offset, ret_numbers.size(), ret_numbers,
			"glTF import: Sparse indices were out of bounds for the accessor.");
		for (int64_t in_vec = 0; in_vec < vector_size; in_vec++) {
			ret_numbers.set(array_offset + in_vec, sparse_values[in_sparse * vector_size + in_vec]);
		}
	}
	return ret_numbers;
}

PackedColorArray GLTFAccessor::decode_as_colors(const Ref<GLTFState>& p_gltf_state) const
{
	PackedColorArray ret;
	PackedFloat32Array numbers = _decode_as_numbers<float>(p_gltf_state);
	if (accessor_type == TYPE_VEC3) {
		ERR_FAIL_COND_V_MSG(numbers.size() != count * 3, ret,
			"glTF import: The accessor does not have the expected amount of numbers for the given "
			"count and vector size.");
		ret.resize(count);
		for (int64_t i = 0; i < count; i++) {
			const int64_t number_index = i * 3;
			ret.set(i, Color(numbers[number_index], numbers[number_index + 1],
						   numbers[number_index + 2], 1.0f));
		}
	}
	else if (accessor_type == TYPE_VEC4) {
		ERR_FAIL_COND_V_MSG(numbers.size() != count * 4, ret,
			"glTF import: The accessor does not have the expected amount of numbers for the given "
			"count and vector size.");
		ret.resize(count);
		for (int64_t i = 0; i < count; i++) {
			const int64_t number_index = i * 4;
			ret.set(i, Color(numbers[number_index], numbers[number_index + 1],
						   numbers[number_index + 2], numbers[number_index + 3]));
		}
	}
	else {
		ERR_FAIL_V_MSG(ret,
			"glTF import: The `decode_as_colors` function is designed to be fast and can only be "
			"used with accessors of type \"VEC3\" or \"VEC4\", but was called with type \"" +
				_get_accessor_type_name() + "\".");
	}
	return ret;
}

PackedFloat32Array GLTFAccessor::decode_as_float32s(const Ref<GLTFState>& p_gltf_state) const
{
	return _decode_as_numbers<float>(p_gltf_state);
}

PackedFloat64Array GLTFAccessor::decode_as_float64s(const Ref<GLTFState>& p_gltf_state) const
{
	return _decode_as_numbers<double>(p_gltf_state);
}

PackedInt32Array GLTFAccessor::decode_as_int32s(const Ref<GLTFState>& p_gltf_state) const
{
	return _decode_as_numbers<int32_t>(p_gltf_state);
}

PackedInt64Array GLTFAccessor::decode_as_int64s(const Ref<GLTFState>& p_gltf_state) const
{
	return _decode_as_numbers<int64_t>(p_gltf_state);
}

Vector<Quaternion> GLTFAccessor::decode_as_quaternions(const Ref<GLTFState>& p_gltf_state) const
{
	Vector<Quaternion> ret;
	ERR_FAIL_COND_V_MSG(accessor_type != TYPE_VEC4, ret,
		"glTF import: The `decode_as_quaternions` function is designed to be fast and can only be "
		"used with accessors of type \"VEC4\", but was called with type \"" +
			_get_accessor_type_name() + "\".");
	PackedFloat32Array numbers = _decode_as_numbers<real_t>(p_gltf_state);
	ERR_FAIL_COND_V_MSG(numbers.size() != count * 4, ret,
		"glTF import: The accessor does not have the expected amount of numbers for the given "
		"count and vector size.");
	ret.resize(count);
	for (int64_t i = 0; i < count; i++) {
		const int64_t number_index = i * 4;
		ret.set(i, Quaternion(numbers[number_index], numbers[number_index + 1],
					   numbers[number_index + 2], numbers[number_index + 3])
					   .normalized());
	}
	return ret;
}

PackedVector2Array GLTFAccessor::decode_as_vector2s(const Ref<GLTFState>& p_gltf_state) const
{
	PackedVector2Array ret;
	ERR_FAIL_COND_V_MSG(accessor_type != TYPE_VEC2, ret,
		"glTF import: The `decode_as_vector2s` function is designed to be fast and can only be "
		"used with accessors of type \"VEC2\", but was called with type \"" +
			_get_accessor_type_name() + "\".");
	PackedFloat32Array numbers = _decode_as_numbers<real_t>(p_gltf_state);
	ERR_FAIL_COND_V_MSG(numbers.size() != count * 2, ret,
		"glTF import: The accessor does not have the expected amount of numbers for the given "
		"count and vector size.");
	ret.resize(count);
	for (int64_t i = 0; i < count; i++) {
		const int64_t number_index = i * 2;
		ret.set(i, Vector2(numbers[number_index], numbers[number_index + 1]));
	}
	return ret;
}

PackedVector3Array GLTFAccessor::decode_as_vector3s(const Ref<GLTFState>& p_gltf_state) const
{
	PackedVector3Array ret;
	ERR_FAIL_COND_V_MSG(accessor_type != TYPE_VEC3, ret,
		"glTF import: The `decode_as_vector3s` function is designed to be fast and can only be "
		"used with accessors of type \"VEC3\", but was called with type \"" +
			_get_accessor_type_name() + "\".");
	PackedFloat32Array numbers = _decode_as_numbers<real_t>(p_gltf_state);
	ERR_FAIL_COND_V_MSG(numbers.size() != count * 3, ret,
		"glTF import: The accessor does not have the expected amount of numbers for the given "
		"count and vector size.");
	ret.resize(count);
	for (int64_t i = 0; i < count; i++) {
		const int64_t number_index = i * 3;
		ret.set(i,
			Vector3(numbers[number_index], numbers[number_index + 1], numbers[number_index + 2]));
	}
	return ret;
}

PackedVector4Array GLTFAccessor::decode_as_vector4s(const Ref<GLTFState>& p_gltf_state) const
{
	PackedVector4Array ret;
	ERR_FAIL_COND_V_MSG(accessor_type != TYPE_VEC4, ret,
		"glTF import: The `decode_as_vector4s` function is designed to be fast and can only be "
		"used with accessors of type \"VEC4\", but was called with type \"" +
			_get_accessor_type_name() + "\".");
	PackedFloat32Array numbers = _decode_as_numbers<real_t>(p_gltf_state);
	ERR_FAIL_COND_V_MSG(numbers.size() != count * 4, ret,
		"glTF import: The accessor does not have the expected amount of numbers for the given "
		"count and vector size.");
	ret.resize(count);
	for (int64_t i = 0; i < count; i++) {
		const int64_t number_index = i * 4;
		ret.set(i, Vector4(numbers[number_index], numbers[number_index + 1],
					   numbers[number_index + 2], numbers[number_index + 3]));
	}
	return ret;
}

void GLTFAccessor::_store_sparse_indices_into_state(const Ref<GLTFState>& p_gltf_state,
	const PackedInt64Array& p_sparse_indices, const bool p_deduplicate)
{
	// The byte offset of a sparse accessor's indices buffer view MUST be a multiple of the indices
	// primitive componentType.
	// https://github.com/KhronosGroup/glTF/blob/main/specification/2.0/schema/accessor.sparse.indices.schema.json
	const int64_t bytes_per_index = _get_bytes_per_component(sparse_indices_component_type);
	PackedByteArray indices_bytes;
	indices_bytes.resize(bytes_per_index * p_sparse_indices.size());
	uint8_t* ret_write = indices_bytes.ptrw();
	int64_t ret_byte_offset = 0;
	for (int64_t i = 0; i < p_sparse_indices.size(); i++) {
		switch (sparse_indices_component_type) {
		case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_BYTE: {
			*(uint8_t*)&ret_write[ret_byte_offset] = p_sparse_indices[i];
		} break;
		case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_SHORT: {
			*(uint16_t*)&ret_write[ret_byte_offset] = p_sparse_indices[i];
		} break;
		case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_INT: {
			*(uint32_t*)&ret_write[ret_byte_offset] = p_sparse_indices[i];
		} break;
		case GLTFAccessor::COMPONENT_TYPE_UNSIGNED_LONG: {
			*(uint64_t*)&ret_write[ret_byte_offset] = p_sparse_indices[i];
		} break;
		default: {
			ERR_FAIL_MSG("glTF export: Invalid sparse indices component type '" +
						 _get_component_type_name(sparse_indices_component_type) +
						 "' for sparse accessor indices.");
		} break;
		}
		ret_byte_offset += bytes_per_index;
	}
	const GLTFBufferViewIndex buffer_view_index =
		GLTFBufferView::write_new_buffer_view_into_state(p_gltf_state, indices_bytes,
			bytes_per_index, GLTFBufferView::TARGET_NONE, -1, 0, p_deduplicate);
	ERR_FAIL_COND_MSG(
		buffer_view_index == -1, "glTF export: Failed to write sparse indices into glTF state.");
	set_sparse_indices_buffer_view(buffer_view_index);
}

GLTFAccessor::GLTFComponentType GLTFAccessor::get_minimal_integer_component_type_from_ints(
	const PackedInt64Array& p_numbers)
{
	bool has_negative = false;
	for (int64_t i = 0; i < p_numbers.size(); i++) {
		if (p_numbers[i] < 0) {
			has_negative = true;
			break;
		}
	}
	if (has_negative) {
		GLTFComponentType ret = GLTFAccessor::COMPONENT_TYPE_SIGNED_BYTE;
		for (int64_t i = 0; i < p_numbers.size(); i++) {
			const int64_t num = p_numbers[i];
			if (ret == GLTFAccessor::COMPONENT_TYPE_SIGNED_BYTE && (num < -128LL || num > 127LL)) {
				ret = GLTFAccessor::COMPONENT_TYPE_SIGNED_SHORT;
			}
			if (ret == GLTFAccessor::COMPONENT_TYPE_SIGNED_SHORT &&
				(num < -32768LL || num > 32767LL)) {
				ret = GLTFAccessor::COMPONENT_TYPE_SIGNED_INT;
			}
			if (ret == GLTFAccessor::COMPONENT_TYPE_SIGNED_INT &&
				(num < -2147483648LL || num > 2147483647LL)) {
				return GLTFAccessor::COMPONENT_TYPE_SIGNED_LONG;
			}
		}
		return ret;
	}
	GLTFComponentType ret = GLTFAccessor::COMPONENT_TYPE_UNSIGNED_BYTE;
	for (int64_t i = 0; i < p_numbers.size(); i++) {
		const int64_t num = p_numbers[i];
		// 3.7.2.1. indices accessor MUST NOT contain the maximum possible value for the component
		// type used (i.e., 255 for unsigned bytes, 65535 for unsigned shorts, 4294967295 for
		// unsigned ints). https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#meshes-overview
		if (ret == GLTFAccessor::COMPONENT_TYPE_UNSIGNED_BYTE && num > 254LL) {
			ret = GLTFAccessor::COMPONENT_TYPE_UNSIGNED_SHORT;
		}
		if (ret == GLTFAccessor::COMPONENT_TYPE_UNSIGNED_SHORT && num > 65534LL) {
			ret = GLTFAccessor::COMPONENT_TYPE_UNSIGNED_INT;
		}
		if (ret == GLTFAccessor::COMPONENT_TYPE_UNSIGNED_INT && num > 4294967294LL) {
			return GLTFAccessor::COMPONENT_TYPE_UNSIGNED_LONG;
		}
	}
	return ret;
}

PackedByteArray GLTFAccessor::encode_floats_as_bytes(const PackedFloat64Array& p_input_numbers)
{
	// Filter and update `count`, `min`, and `max` based on the given data.
	PackedFloat64Array filtered_numbers = _filter_numbers(p_input_numbers);
	count = filtered_numbers.size() / _get_vector_size();
	_calculate_min_and_max(filtered_numbers);
	// Actually encode the data.
	const int64_t input_size = filtered_numbers.size();
	const int64_t bytes_per_component = _get_bytes_per_component(component_type);
	int64_t raw_byte_size = _determine_padded_byte_count(bytes_per_component * input_size);
	int64_t skip_every = 0;
	int64_t skip_bytes = 0;
	_determine_pad_skip(skip_every, skip_bytes);
	PackedByteArray ret;
	ret.resize(raw_byte_size);
	uint8_t* ret_write = ret.ptrw();
	int64_t ret_byte_offset = 0;
	for (int64_t i = 0; i < input_size; i++) {
		switch (component_type) {
		case COMPONENT_TYPE_NONE: {
			ERR_FAIL_V_MSG(ret, "glTF export: Invalid component type 'NONE' for glTF accessor.");
		} break;
		case COMPONENT_TYPE_SIGNED_BYTE: {
			*(int8_t*)&ret_write[ret_byte_offset] = filtered_numbers[i];
		} break;
		case COMPONENT_TYPE_UNSIGNED_BYTE: {
			*(uint8_t*)&ret_write[ret_byte_offset] = filtered_numbers[i];
		} break;
		case COMPONENT_TYPE_SIGNED_SHORT: {
			*(int16_t*)&ret_write[ret_byte_offset] = filtered_numbers[i];
		} break;
		case COMPONENT_TYPE_UNSIGNED_SHORT: {
			*(uint16_t*)&ret_write[ret_byte_offset] = filtered_numbers[i];
		} break;
		case COMPONENT_TYPE_SIGNED_INT: {
			*(int32_t*)&ret_write[ret_byte_offset] = filtered_numbers[i];
		} break;
		case COMPONENT_TYPE_UNSIGNED_INT: {
			*(uint32_t*)&ret_write[ret_byte_offset] = filtered_numbers[i];
		} break;
		case COMPONENT_TYPE_SINGLE_FLOAT: {
			*(float*)&ret_write[ret_byte_offset] = filtered_numbers[i];
		} break;
		case COMPONENT_TYPE_DOUBLE_FLOAT: {
			*(double*)&ret_write[ret_byte_offset] = filtered_numbers[i];
		} break;
		case COMPONENT_TYPE_HALF_FLOAT: {
			*(uint16_t*)&ret_write[ret_byte_offset] = Math::make_half_float(filtered_numbers[i]);
		} break;
		case COMPONENT_TYPE_SIGNED_LONG: {
			*(int64_t*)&ret_write[ret_byte_offset] = filtered_numbers[i];
		} break;
		case COMPONENT_TYPE_UNSIGNED_LONG: {
			*(uint64_t*)&ret_write[ret_byte_offset] = filtered_numbers[i];
		} break;
		default: {
			ERR_FAIL_V_MSG(ret,
				"glTF export: Godot does not support writing glTF accessor components of type '" +
					itos(component_type) + "'.");
		} break;
		}
		ret_byte_offset += bytes_per_component;
		if (unlikely(skip_every > 0)) {
			if ((i + 1) % skip_every == 0) {
				ret_byte_offset += skip_bytes;
			}
		}
	}
	ERR_FAIL_COND_V_MSG(ret_byte_offset != raw_byte_size, ret,
		"glTF export: Accessor encoded data did not write exactly the expected number of bytes.");
	return ret;
}

GLTFAccessorIndex GLTFAccessor::store_accessor_data_into_state(const Ref<GLTFState>& p_gltf_state,
	const PackedByteArray& p_data_bytes,
	const GLTFBufferView::ArrayBufferTarget p_buffer_view_target,
	const GLTFBufferIndex p_buffer_index, const bool p_deduplicate)
{
	ERR_FAIL_COND_V_MSG(p_data_bytes.is_empty(), -1, "glTF export: Cannot store nothing.");
	const int64_t bytes_per_vec = _get_bytes_per_vector();
	ERR_FAIL_COND_V_MSG(bytes_per_vec == 0 || p_data_bytes.size() % bytes_per_vec != 0, -1,
		"glTF export: Tried to store an accessor with data that is not a multiple of the "
		"accessor's bytes per vector.");
	count = p_data_bytes.size() / bytes_per_vec;
	const int64_t alignment = _get_bytes_per_component(component_type);
	int64_t byte_stride = -1;
	if (p_buffer_view_target == GLTFBufferView::TARGET_ARRAY_BUFFER) {
		byte_stride = bytes_per_vec;
		ERR_FAIL_COND_V_MSG(byte_stride < 4 || byte_stride % 4 != 0, -1,
			"glTF export: Vertex attributes using TARGET_ARRAY_BUFFER must have a byte stride that "
			"is a multiple of 4 as required by section 3.6.2.4 of the glTF specification.");
	}
	const GLTFBufferViewIndex buffer_view_index = GLTFBufferView::write_new_buffer_view_into_state(
		p_gltf_state, p_data_bytes, alignment, p_buffer_view_target, byte_stride, 0, p_deduplicate);
	ERR_FAIL_COND_V_MSG(buffer_view_index == -1, -1,
		"glTF export: Accessor failed to write new buffer view into glTF state.");
	set_buffer_view(buffer_view_index);
	Vector<Ref<GLTFAccessor>> state_accessors = p_gltf_state->get_accessors();
	const GLTFAccessorIndex accessor_count = state_accessors.size();
	for (GLTFAccessorIndex i = 0; i < accessor_count; i++) {
		const Ref<GLTFAccessor>& existing_accessor = state_accessors[i];
		if (is_equal_exact(existing_accessor)) {
			return i;
		}
	}
	Ref<GLTFAccessor> self = this;
	state_accessors.append(self);
	p_gltf_state->set_accessors(state_accessors);
	return accessor_count;
}

Ref<GLTFAccessor> GLTFAccessor::make_new_accessor_without_data(
	GLTFAccessorType p_accessor_type, GLTFComponentType p_component_type)
{
	Ref<GLTFAccessor> accessor;
	accessor.instantiate();
	accessor->set_accessor_type(p_accessor_type);
	accessor->set_component_type(p_component_type);
	return accessor;
}

GLTFAccessorIndex GLTFAccessor::encode_new_accessor_from_colors(const Ref<GLTFState>& p_gltf_state,
	const PackedColorArray& p_input_data,
	const GLTFBufferView::ArrayBufferTarget p_buffer_view_target, const bool p_deduplicate)
{
	ERR_FAIL_COND_V_MSG(
		p_input_data.is_empty(), -1, "glTF export: Cannot encode an accessor from an empty array.");
	PackedFloat64Array numbers;
	numbers.resize(p_input_data.size() * 4);
	for (int64_t i = 0; i < p_input_data.size(); i++) {
		const Color& color = p_input_data[i];
		numbers.set(i * 4, color.r);
		numbers.set(i * 4 + 1, color.g);
		numbers.set(i * 4 + 2, color.b);
		numbers.set(i * 4 + 3, color.a);
	}
	Ref<GLTFAccessor> accessor =
		make_new_accessor_without_data(TYPE_VEC4, COMPONENT_TYPE_SINGLE_FLOAT);
	PackedByteArray encoded_bytes = accessor->encode_floats_as_bytes(numbers);
	ERR_FAIL_COND_V_MSG(encoded_bytes.is_empty(), -1,
		"glTF export: Accessor failed to encode data as bytes (was the input data empty?).");
	return accessor->store_accessor_data_into_state(
		p_gltf_state, encoded_bytes, p_buffer_view_target, 0, p_deduplicate);
}

GLTFAccessorIndex GLTFAccessor::encode_new_accessor_from_float64s(
	const Ref<GLTFState>& p_gltf_state, const PackedFloat64Array& p_input_data,
	const GLTFBufferView::ArrayBufferTarget p_buffer_view_target, const bool p_deduplicate)
{
	ERR_FAIL_COND_V_MSG(
		p_input_data.is_empty(), -1, "glTF export: Cannot encode an accessor from an empty array.");
	Ref<GLTFAccessor> accessor =
		make_new_accessor_without_data(TYPE_SCALAR, COMPONENT_TYPE_SINGLE_FLOAT);
	PackedByteArray encoded_bytes = accessor->encode_floats_as_bytes(p_input_data);
	ERR_FAIL_COND_V_MSG(encoded_bytes.is_empty(), -1,
		"glTF export: Accessor failed to encode data as bytes (was the input data empty?).");
	return accessor->store_accessor_data_into_state(
		p_gltf_state, encoded_bytes, p_buffer_view_target, 0, p_deduplicate);
}

GLTFAccessorIndex GLTFAccessor::encode_new_accessor_from_int32s(const Ref<GLTFState>& p_gltf_state,
	const PackedInt32Array& p_input_data,
	const GLTFBufferView::ArrayBufferTarget p_buffer_view_target, const bool p_deduplicate)
{
	ERR_FAIL_COND_V_MSG(
		p_input_data.is_empty(), -1, "glTF export: Cannot encode an accessor from an empty array.");
	PackedInt64Array numbers;
	numbers.resize(p_input_data.size());
	for (int64_t i = 0; i < p_input_data.size(); i++) {
		numbers.set(i, p_input_data[i]);
	}
	const GLTFComponentType component_type = get_minimal_integer_component_type_from_ints(numbers);
	Ref<GLTFAccessor> accessor = make_new_accessor_without_data(TYPE_SCALAR, component_type);
	PackedByteArray encoded_bytes = accessor->encode_ints_as_bytes(numbers);
	ERR_FAIL_COND_V_MSG(encoded_bytes.is_empty(), -1,
		"glTF export: Accessor failed to encode data as bytes (was the input data empty?).");
	return accessor->store_accessor_data_into_state(
		p_gltf_state, encoded_bytes, p_buffer_view_target, 0, p_deduplicate);
}

GLTFAccessorIndex GLTFAccessor::encode_new_accessor_from_int64s(const Ref<GLTFState>& p_gltf_state,
	const PackedInt64Array& p_input_data,
	const GLTFBufferView::ArrayBufferTarget p_buffer_view_target, const bool p_deduplicate)
{
	ERR_FAIL_COND_V_MSG(
		p_input_data.is_empty(), -1, "glTF export: Cannot encode an accessor from an empty array.");
	const GLTFComponentType component_type =
		get_minimal_integer_component_type_from_ints(p_input_data);
	Ref<GLTFAccessor> accessor = make_new_accessor_without_data(TYPE_SCALAR, component_type);
	PackedByteArray encoded_bytes = accessor->encode_ints_as_bytes(p_input_data);
	ERR_FAIL_COND_V_MSG(encoded_bytes.is_empty(), -1,
		"glTF export: Accessor failed to encode data as bytes (was the input data empty?).");
	return accessor->store_accessor_data_into_state(
		p_gltf_state, encoded_bytes, p_buffer_view_target, 0, p_deduplicate);
}

GLTFAccessorIndex GLTFAccessor::encode_new_accessor_from_quaternions(
	const Ref<GLTFState>& p_gltf_state, const Vector<Quaternion>& p_input_data,
	const GLTFBufferView::ArrayBufferTarget p_buffer_view_target, const bool p_deduplicate)
{
	ERR_FAIL_COND_V_MSG(
		p_input_data.is_empty(), -1, "glTF export: Cannot encode an accessor from an empty array.");
	PackedFloat64Array numbers;
	numbers.resize(p_input_data.size() * 4);
	for (int64_t i = 0; i < p_input_data.size(); i++) {
		const Quaternion& quat = p_input_data[i];
		numbers.set(i * 4, quat.x);
		numbers.set(i * 4 + 1, quat.y);
		numbers.set(i * 4 + 2, quat.z);
		numbers.set(i * 4 + 3, quat.w);
	}
	Ref<GLTFAccessor> accessor =
		make_new_accessor_without_data(TYPE_VEC4, COMPONENT_TYPE_SINGLE_FLOAT);
	PackedByteArray encoded_bytes = accessor->encode_floats_as_bytes(numbers);
	ERR_FAIL_COND_V_MSG(encoded_bytes.is_empty(), -1,
		"glTF export: Accessor failed to encode data as bytes (was the input data empty?).");
	return accessor->store_accessor_data_into_state(
		p_gltf_state, encoded_bytes, p_buffer_view_target, 0, p_deduplicate);
}

GLTFAccessorIndex GLTFAccessor::encode_new_accessor_from_vector2s(
	const Ref<GLTFState>& p_gltf_state, const PackedVector2Array& p_input_data,
	const GLTFBufferView::ArrayBufferTarget p_buffer_view_target, const bool p_deduplicate)
{
	ERR_FAIL_COND_V_MSG(
		p_input_data.is_empty(), -1, "glTF export: Cannot encode an accessor from an empty array.");
	PackedFloat64Array numbers;
	numbers.resize(p_input_data.size() * 2);
	for (int64_t i = 0; i < p_input_data.size(); i++) {
		const Vector2& vec = p_input_data[i];
		numbers.set(i * 2, vec.x);
		numbers.set(i * 2 + 1, vec.y);
	}
	Ref<GLTFAccessor> accessor =
		make_new_accessor_without_data(TYPE_VEC2, COMPONENT_TYPE_SINGLE_FLOAT);
	PackedByteArray encoded_bytes = accessor->encode_floats_as_bytes(numbers);
	ERR_FAIL_COND_V_MSG(encoded_bytes.is_empty(), -1,
		"glTF export: Accessor failed to encode data as bytes (was the input data empty?).");
	return accessor->store_accessor_data_into_state(
		p_gltf_state, encoded_bytes, p_buffer_view_target, 0, p_deduplicate);
}

GLTFAccessorIndex GLTFAccessor::encode_new_accessor_from_vector3s(
	const Ref<GLTFState>& p_gltf_state, const PackedVector3Array& p_input_data,
	const GLTFBufferView::ArrayBufferTarget p_buffer_view_target, const bool p_deduplicate)
{
	ERR_FAIL_COND_V_MSG(
		p_input_data.is_empty(), -1, "glTF export: Cannot encode an accessor from an empty array.");
	PackedFloat64Array numbers;
	numbers.resize(p_input_data.size() * 3);
	for (int64_t i = 0; i < p_input_data.size(); i++) {
		const Vector3& vec = p_input_data[i];
		numbers.set(i * 3, vec.x);
		numbers.set(i * 3 + 1, vec.y);
		numbers.set(i * 3 + 2, vec.z);
	}
	Ref<GLTFAccessor> accessor =
		make_new_accessor_without_data(TYPE_VEC3, COMPONENT_TYPE_SINGLE_FLOAT);
	PackedByteArray encoded_bytes = accessor->encode_floats_as_bytes(numbers);
	ERR_FAIL_COND_V_MSG(encoded_bytes.is_empty(), -1,
		"glTF export: Accessor failed to encode data as bytes (was the input data empty?).");
	return accessor->store_accessor_data_into_state(
		p_gltf_state, encoded_bytes, p_buffer_view_target, 0, p_deduplicate);
}

GLTFAccessorIndex GLTFAccessor::encode_new_accessor_from_vector4s(
	const Ref<GLTFState>& p_gltf_state, const PackedVector4Array& p_input_data,
	const GLTFBufferView::ArrayBufferTarget p_buffer_view_target, const bool p_deduplicate)
{
	ERR_FAIL_COND_V_MSG(
		p_input_data.is_empty(), -1, "glTF export: Cannot encode an accessor from an empty array.");
	PackedFloat64Array numbers;
	numbers.resize(p_input_data.size() * 4);
	for (int64_t i = 0; i < p_input_data.size(); i++) {
		const Vector4& vec = p_input_data[i];
		numbers.set(i * 4, vec.x);
		numbers.set(i * 4 + 1, vec.y);
		numbers.set(i * 4 + 2, vec.z);
		numbers.set(i * 4 + 3, vec.w);
	}
	Ref<GLTFAccessor> accessor =
		make_new_accessor_without_data(TYPE_VEC4, COMPONENT_TYPE_SINGLE_FLOAT);
	PackedByteArray encoded_bytes = accessor->encode_floats_as_bytes(numbers);
	ERR_FAIL_COND_V_MSG(encoded_bytes.is_empty(), -1,
		"glTF export: Accessor failed to encode data as bytes (was the input data empty?).");
	return accessor->store_accessor_data_into_state(
		p_gltf_state, encoded_bytes, p_buffer_view_target, 0, p_deduplicate);
}

GLTFAccessorIndex GLTFAccessor::encode_new_accessor_from_vector4is(
	const Ref<GLTFState>& p_gltf_state, const Vector<Vector4i>& p_input_data,
	const GLTFBufferView::ArrayBufferTarget p_buffer_view_target, const bool p_deduplicate)
{
	ERR_FAIL_COND_V_MSG(
		p_input_data.is_empty(), -1, "glTF export: Cannot encode an accessor from an empty array.");
	PackedInt64Array numbers;
	numbers.resize(p_input_data.size() * 4);
	for (int64_t i = 0; i < p_input_data.size(); i++) {
		const Vector4i& vec = p_input_data[i];
		numbers.set(i * 4, vec.x);
		numbers.set(i * 4 + 1, vec.y);
		numbers.set(i * 4 + 2, vec.z);
		numbers.set(i * 4 + 3, vec.w);
	}
	const GLTFComponentType component_type = get_minimal_integer_component_type_from_ints(numbers);
	Ref<GLTFAccessor> accessor = make_new_accessor_without_data(TYPE_VEC4, component_type);
	PackedByteArray encoded_bytes = accessor->encode_ints_as_bytes(numbers);
	ERR_FAIL_COND_V_MSG(encoded_bytes.is_empty(), -1,
		"glTF export: Accessor failed to encode data as bytes (was the input data empty?).");
	return accessor->store_accessor_data_into_state(
		p_gltf_state, encoded_bytes, p_buffer_view_target, 0, p_deduplicate);
}

GLTFAccessorIndex GLTFAccessor::encode_new_sparse_accessor_from_vec3s(
	const Ref<GLTFState>& p_gltf_state, const PackedVector3Array& p_input_data,
	const PackedVector3Array& p_base_reference_data, const double p_tolerance_multiplier,
	const GLTFBufferView::ArrayBufferTarget p_main_buffer_view_target, const bool p_deduplicate)
{
	const int64_t input_size = p_input_data.size();
	ERR_FAIL_COND_V_MSG(
		input_size == 0, -1, "glTF export: Cannot encode an accessor from an empty array.");
	const bool is_base_empty = p_base_reference_data.is_empty();
	ERR_FAIL_COND_V_MSG(!is_base_empty && p_base_reference_data.size() != input_size, -1,
		"glTF export: Base reference data must either be empty, or have the same size as the main "
		"input data.");
	PackedInt64Array sparse_indices;
	PackedFloat64Array sparse_values;
	PackedFloat64Array dense_values;
	int64_t highest_index = 0;
	dense_values.resize(input_size * 3);
	for (int64_t i = 0; i < input_size; i++) {
		Vector3 vec = p_input_data[i];
		Vector3 base_ref_vec;
		Vector3 displacement;
		if (is_base_empty) {
			base_ref_vec = Vector3();
			displacement = vec;
		}
		else {
			base_ref_vec = p_base_reference_data[i];
			displacement = vec - base_ref_vec;
		}
		if ((displacement * p_tolerance_multiplier).is_zero_approx()) {
			vec = base_ref_vec;
		}
		else {
			highest_index = i;
			sparse_indices.append(i);
			sparse_values.append(vec.x);
			sparse_values.append(vec.y);
			sparse_values.append(vec.z);
		}
		dense_values.set(i * 3, vec.x);
		dense_values.set(i * 3 + 1, vec.y);
		dense_values.set(i * 3 + 2, vec.z);
	}
	const int64_t sparse_count = sparse_indices.size();
	const int64_t bytes_per_value_component = _get_bytes_per_component(COMPONENT_TYPE_SINGLE_FLOAT);
	const GLTFComponentType indices_component_type =
		_get_indices_component_type_for_size(highest_index);
	const int64_t sparse_data_bytes =
		_get_bytes_per_component(indices_component_type) * sparse_count +
		bytes_per_value_component * sparse_values.size();
	const int64_t dense_data_bytes = bytes_per_value_component * 3 * input_size;
	constexpr int64_t sparse_json_fluff = 200;
	Ref<GLTFAccessor> accessor =
		make_new_accessor_without_data(TYPE_VEC3, COMPONENT_TYPE_SINGLE_FLOAT);
	if (sparse_data_bytes + sparse_json_fluff >= dense_data_bytes) {
		PackedByteArray encoded_bytes = accessor->encode_floats_as_bytes(dense_values);
		ERR_FAIL_COND_V_MSG(encoded_bytes.is_empty(), -1,
			"glTF export: Accessor failed to encode data as bytes (was the input data empty?).");
		return accessor->store_accessor_data_into_state(
			p_gltf_state, encoded_bytes, p_main_buffer_view_target, 0, p_deduplicate);
	}
	if (sparse_count > 0) {
		accessor->set_sparse_count(sparse_count);
		accessor->set_sparse_indices_component_type(indices_component_type);
		accessor->_store_sparse_indices_into_state(p_gltf_state, sparse_indices, p_deduplicate);
		const PackedByteArray sparse_values_encoded_bytes =
			accessor->encode_floats_as_bytes(sparse_values);
		ERR_FAIL_COND_V_MSG(sparse_values_encoded_bytes.is_empty(), -1,
			"glTF export: Accessor failed to encode sparse values as bytes.");
		const GLTFBufferViewIndex sparse_values_buffer_view_index =
			GLTFBufferView::write_new_buffer_view_into_state(p_gltf_state,
				sparse_values_encoded_bytes, bytes_per_value_component, GLTFBufferView::TARGET_NONE,
				-1, 0, p_deduplicate);
		accessor->set_sparse_values_buffer_view(sparse_values_buffer_view_index);
	}
	if (is_base_empty) {
		accessor->set_count(input_size);
		const PackedFloat64Array filtered_numbers = accessor->_filter_numbers(dense_values);
		accessor->_calculate_min_and_max(filtered_numbers);
		Vector<Ref<GLTFAccessor>> state_accessors = p_gltf_state->get_accessors();
		const GLTFAccessorIndex accessor_count = state_accessors.size();
		for (GLTFAccessorIndex i = 0; i < accessor_count; i++) {
			const Ref<GLTFAccessor>& existing_accessor = state_accessors[i];
			if (accessor->is_equal_exact(existing_accessor)) {
				return i;
			}
		}
		state_accessors.append(accessor);
		p_gltf_state->set_accessors(state_accessors);
		return accessor_count;
	}
	PackedFloat64Array base_reference_values;
	base_reference_values.resize(input_size * 3);
	for (int64_t i = 0; i < input_size; i++) {
		const Vector3& base_ref_vec = p_base_reference_data[i];
		base_reference_values.set(i * 3, base_ref_vec.x);
		base_reference_values.set(i * 3 + 1, base_ref_vec.y);
		base_reference_values.set(i * 3 + 2, base_ref_vec.z);
	}
	const PackedByteArray base_reference_encoded_bytes =
		accessor->encode_floats_as_bytes(base_reference_values);
	ERR_FAIL_COND_V_MSG(base_reference_encoded_bytes.is_empty(), -1,
		"glTF export: Accessor failed to encode data as bytes (was the input data empty?).");
	return accessor->store_accessor_data_into_state(
		p_gltf_state, base_reference_encoded_bytes, p_main_buffer_view_target, 0, p_deduplicate);
}

PackedByteArray GLTFAccessor::encode_ints_as_bytes(const PackedInt64Array&)
{
	return PackedByteArray();
}

// ============================================================================
// GLTFNode
// ============================================================================

String GLTFNode::get_original_name() { return original_name; }

void GLTFNode::set_original_name(const String& p_name) { original_name = p_name; }

GLTFNodeIndex GLTFNode::get_parent() { return parent; }

void GLTFNode::set_parent(GLTFNodeIndex p_parent) { parent = p_parent; }

int GLTFNode::get_height() { return height; }

void GLTFNode::set_height(int p_height) { height = p_height; }

Transform3D GLTFNode::get_xform() { return transform; }

void GLTFNode::set_xform(const Transform3D& p_xform) { transform = p_xform; }

GLTFMeshIndex GLTFNode::get_mesh() { return mesh; }

void GLTFNode::set_mesh(GLTFMeshIndex p_mesh) { mesh = p_mesh; }

GLTFCameraIndex GLTFNode::get_camera() { return camera; }

void GLTFNode::set_camera(GLTFCameraIndex p_camera) { camera = p_camera; }

GLTFSkinIndex GLTFNode::get_skin() { return skin; }

void GLTFNode::set_skin(GLTFSkinIndex p_skin) { skin = p_skin; }

GLTFSkeletonIndex GLTFNode::get_skeleton() { return skeleton; }

void GLTFNode::set_skeleton(GLTFSkeletonIndex p_skeleton) { skeleton = p_skeleton; }

Vector3 GLTFNode::get_position() { return transform.origin; }

void GLTFNode::set_position(const Vector3& p_position) { transform.origin = p_position; }

Quaternion GLTFNode::get_rotation() { return transform.basis.get_rotation_quaternion(); }

void GLTFNode::set_rotation(const Quaternion& p_rotation)
{
	transform.basis.set_quaternion_scale(p_rotation, transform.basis.get_scale());
}

Vector3 GLTFNode::get_scale() { return transform.basis.get_scale(); }

void GLTFNode::set_scale(const Vector3& p_scale)
{
	transform.basis = transform.basis.orthonormalized() * Basis::from_scale(p_scale);
}

Vector<int> GLTFNode::get_children() { return Vector<int>(children); }

void GLTFNode::set_children(const Vector<int>& p_children) { children = Vector<int>(p_children); }

void GLTFNode::append_child_index(int p_child_index) { children.append(p_child_index); }

GLTFLightIndex GLTFNode::get_light() { return light; }

void GLTFNode::set_light(GLTFLightIndex p_light) { light = p_light; }

bool GLTFNode::get_visible() { return visible; }

void GLTFNode::set_visible(bool p_visible) { visible = p_visible; }

NodePath GLTFNode::get_scene_node_path(Ref<GLTFState> p_state, bool p_handle_skeletons)
{
	ERR_FAIL_COND_V_MSG(
		p_state.is_null(), NodePath(), "Cannot get scene node path because GLTFState is null.");
	Vector<String> path;
	Vector<String> subpath;
	Ref<GLTFNode> current_gltf_node = this;
	const int gltf_node_count = p_state->nodes.size();
	if (p_handle_skeletons && skeleton != -1) {
		subpath.append(get_name());
		path.append("Skeleton3D");
		do {
			const int parent_index = current_gltf_node->get_parent();
			ERR_FAIL_INDEX_V(parent_index, gltf_node_count, NodePath());
			current_gltf_node = p_state->nodes[parent_index];
		} while (current_gltf_node->skeleton != -1);
	}
	const bool is_godot_single_root = p_state->extensions_used.has("GODOT_single_root");
	while (true) {
		const int parent_index = current_gltf_node->get_parent();
		if (is_godot_single_root && parent_index == -1) {
			break;
		}
		path.insert(0, current_gltf_node->get_name());
		if (!is_godot_single_root && parent_index == -1) {
			break;
		}
		ERR_FAIL_INDEX_V(parent_index, gltf_node_count, NodePath());
		current_gltf_node = p_state->nodes[parent_index];
	}
	if (unlikely(path.is_empty())) {
		path.append(".");
	}
	return NodePath(path, subpath, false);
}

// ============================================================================
// GLTFSkeleton
// ============================================================================

Vector<GLTFNodeIndex> GLTFSkeleton::get_joints() { return Vector<GLTFNodeIndex>(joints); }

void GLTFSkeleton::set_joints(const Vector<GLTFNodeIndex>& p_joints)
{
	joints = Vector<GLTFNodeIndex>(p_joints);
}

Vector<GLTFNodeIndex> GLTFSkeleton::get_roots() { return Vector<GLTFNodeIndex>(roots); }

void GLTFSkeleton::set_roots(const Vector<GLTFNodeIndex>& p_roots)
{
	roots = Vector<GLTFNodeIndex>(p_roots);
}

Skeleton3D* GLTFSkeleton::get_godot_skeleton() { return godot_skeleton; }

BoneAttachment3D* GLTFSkeleton::get_bone_attachment(int idx)
{
	ERR_FAIL_INDEX_V(idx, bone_attachments.size(), nullptr);
	return bone_attachments[idx];
}

int32_t GLTFSkeleton::get_bone_attachment_count() { return bone_attachments.size(); }

// ============================================================================
// GLTFSkin
// ============================================================================

GLTFNodeIndex GLTFSkin::get_skin_root() { return skin_root; }

void GLTFSkin::set_skin_root(GLTFNodeIndex p_skin_root) { skin_root = p_skin_root; }

Vector<GLTFNodeIndex> GLTFSkin::get_joints_original() { return joints_original; }

void GLTFSkin::set_joints_original(const Vector<GLTFNodeIndex>& p_joints_original)
{
	joints_original = Vector<GLTFNodeIndex>(p_joints_original);
}

Vector<GLTFNodeIndex> GLTFSkin::get_joints() { return joints; }

void GLTFSkin::set_joints(const Vector<GLTFNodeIndex>& p_joints)
{
	joints = Vector<GLTFNodeIndex>(p_joints);
}

Vector<GLTFNodeIndex> GLTFSkin::get_non_joints() { return non_joints; }

void GLTFSkin::set_non_joints(const Vector<GLTFNodeIndex>& p_non_joints)
{
	non_joints = Vector<GLTFNodeIndex>(p_non_joints);
}

Vector<GLTFNodeIndex> GLTFSkin::get_roots() { return roots; }

void GLTFSkin::set_roots(const Vector<GLTFNodeIndex>& p_roots)
{
	roots = Vector<GLTFNodeIndex>(p_roots);
}

int GLTFSkin::get_skeleton() { return skeleton; }

void GLTFSkin::set_skeleton(int p_skeleton) { skeleton = p_skeleton; }

Ref<Skin> GLTFSkin::get_godot_skin() { return godot_skin; }

void GLTFSkin::set_godot_skin(const Ref<Skin>& p_godot_skin) { godot_skin = p_godot_skin; }

// ============================================================================
// GLTFMesh
// ============================================================================

String GLTFMesh::get_original_name() { return original_name; }

void GLTFMesh::set_original_name(const String& p_name) { original_name = p_name; }

Ref<ImporterMesh> GLTFMesh::get_mesh() { return mesh; }

void GLTFMesh::set_mesh(const Ref<ImporterMesh>& p_mesh) { mesh = p_mesh; }

Vector<float> GLTFMesh::get_blend_weights() { return Vector<float>(blend_weights); }

void GLTFMesh::set_blend_weights(const Vector<float>& p_blend_weights)
{
	blend_weights = Vector<float>(p_blend_weights);
}

// ============================================================================
// GLTFCamera
// ============================================================================

void GLTFCamera::set_fov_conversion_expressions(Ref<GLTFObjectModelProperty>& r_obj_model_prop)
{
	// Expression to convert glTF yfov in radians to Godot fov in degrees.
	Ref<Expression> gltf_to_godot_expr;
	gltf_to_godot_expr.instantiate();
	PackedStringArray gltf_to_godot_args = {"yfov_rad"};
	gltf_to_godot_expr->parse("rad_to_deg(yfov_rad)", gltf_to_godot_args);
	r_obj_model_prop->set_gltf_to_godot_expression(gltf_to_godot_expr);
	// Expression to convert Godot fov in degrees to glTF yfov in radians.
	Ref<Expression> godot_to_gltf_expr;
	godot_to_gltf_expr.instantiate();
	PackedStringArray godot_to_gltf_args = {"fov_deg"};
	godot_to_gltf_expr->parse("deg_to_rad(fov_deg)", godot_to_gltf_args);
	r_obj_model_prop->set_godot_to_gltf_expression(godot_to_gltf_expr);
}

Ref<GLTFCamera> GLTFCamera::from_node(const Camera3D* p_camera)
{
	Ref<GLTFCamera> c;
	c.instantiate();
	ERR_FAIL_NULL_V_MSG(p_camera, c,
		"Tried to create a GLTFCamera from a Camera3D node, but the given node was null.");
	c->set_perspective(
		p_camera->get_projection() == Camera3D::ProjectionType::PROJECTION_PERSPECTIVE);
	// glTF spec (yfov) is in radians, Godot's camera (fov) is in degrees.
	c->set_fov(Math::deg_to_rad(p_camera->get_fov()));
	// glTF spec (xmag and ymag) is a radius in meters, Godot's camera (size) is a diameter in
	// meters.
	c->set_size_mag(p_camera->get_size() * 0.5f);
	c->set_depth_far(p_camera->get_far());
	c->set_depth_near(p_camera->get_near());
	return c;
}

// ============================================================================
// GLTFAnimation
// ============================================================================

GLTFAnimation::Interpolation GLTFAnimation::godot_to_gltf_interpolation(
	const Ref<Animation>& p_godot_animation, int32_t p_godot_anim_track_index)
{
	Animation::InterpolationType interpolation =
		p_godot_animation->track_get_interpolation_type(p_godot_anim_track_index);
	switch (interpolation) {
	case Animation::INTERPOLATION_LINEAR:
	case Animation::INTERPOLATION_LINEAR_ANGLE:
		return INTERP_LINEAR;
	case Animation::INTERPOLATION_NEAREST:
		return INTERP_STEP;
	case Animation::INTERPOLATION_CUBIC:
	case Animation::INTERPOLATION_CUBIC_ANGLE:
		return INTERP_CUBIC_SPLINE;
	}
	return INTERP_LINEAR;
}

Animation::InterpolationType GLTFAnimation::gltf_to_godot_interpolation(
	Interpolation p_gltf_interpolation)
{
	switch (p_gltf_interpolation) {
	case INTERP_LINEAR:
		return Animation::INTERPOLATION_LINEAR;
	case INTERP_STEP:
		return Animation::INTERPOLATION_NEAREST;
	case INTERP_CATMULLROMSPLINE:
	case INTERP_CUBIC_SPLINE:
		return Animation::INTERPOLATION_CUBIC;
	}
	return Animation::INTERPOLATION_LINEAR;
}

String GLTFAnimation::get_original_name() { return original_name; }

void GLTFAnimation::set_original_name(const String& p_name) { original_name = p_name; }

bool GLTFAnimation::get_loop() const { return loop; }

void GLTFAnimation::set_loop(bool p_val) { loop = p_val; }

HashMap<int, GLTFAnimation::NodeTrack>& GLTFAnimation::get_node_tracks() { return node_tracks; }

GLTFAnimation::GLTFAnimation() {}

// ============================================================================
// GLTFObjectModelProperty
// ============================================================================

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
