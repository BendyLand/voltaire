/**************************************************************************/
/*  fbx_document.cpp                                                      */
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
#include "core/io/file_access.h"
#include "core/io/file_access_memory.h"
#include "core/io/image.h"
#include "core/io/resource_loader.h"
#include "core/math/color.h"
#include "fbx_document.h"
#include "modules/gltf/extensions/gltf_light.h"
#include "modules/gltf/gltf_defines.h"
#include "modules/gltf/skin_tool.h"
#include "modules/gltf/structures/gltf_animation.h"
#include "modules/gltf/structures/gltf_camera.h"
#include "scene/3d/bone_attachment_3d.h"
#include "scene/3d/camera_3d.h"
#include "scene/3d/importer_mesh_instance_3d.h"
#include "scene/3d/light_3d.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/material.h"
#include "scene/resources/portable_compressed_texture.h"
#include "scene/resources/surface_tool.h"

#ifdef TOOLS_ENABLED
#include "editor/file_system/editor_file_system.h"
#endif

#include <ufbx.h>

static size_t _file_access_read_fn(void* user, void* data, size_t size)
{
	FileAccess* file = static_cast<FileAccess*>(user);
	return (size_t)file->get_buffer((uint8_t*)data, (uint64_t)size);
}

static bool _file_access_skip_fn(void* user, size_t size)
{
	FileAccess* file = static_cast<FileAccess*>(user);
	file->seek(file->get_position() + size);
	return true;
}

static Vector2 _as_vec2(const ufbx_vec2& p_vector)
{
	return Vector2(real_t(p_vector.x), real_t(p_vector.y));
}

static Color _as_color(const ufbx_vec4& p_vector)
{
	return Color(real_t(p_vector.x), real_t(p_vector.y), real_t(p_vector.z), real_t(p_vector.w));
}

static Quaternion _as_quaternion(const ufbx_quat& p_quat)
{
	return Quaternion(real_t(p_quat.x), real_t(p_quat.y), real_t(p_quat.z), real_t(p_quat.w));
}

static Transform3D _as_transform(const ufbx_transform& p_xform)
{
	Transform3D result;
	result.origin = FBXDocument::_as_vec3(p_xform.translation);
	result.basis.set_quaternion_scale(
		_as_quaternion(p_xform.rotation), FBXDocument::_as_vec3(p_xform.scale));
	return result;
}

static real_t _relative_error(const Vector3& p_a, const Vector3& p_b)
{
	return p_a.distance_to(p_b) / MAX(p_a.length(), p_b.length());
}

static Color _material_color(const ufbx_material_map& p_map)
{
	if (p_map.value_components == 1) {
		float r = float(p_map.value_real);
		return Color(r, r, r);
	}
	else if (p_map.value_components == 3) {
		float r = float(p_map.value_vec3.x);
		float g = float(p_map.value_vec3.y);
		float b = float(p_map.value_vec3.z);
		return Color(r, g, b);
	}
	else {
		float r = float(p_map.value_vec4.x);
		float g = float(p_map.value_vec4.y);
		float b = float(p_map.value_vec4.z);
		float a = float(p_map.value_vec4.w);
		return Color(r, g, b, a);
	}
}

static Color _material_color(const ufbx_material_map& p_map, const ufbx_material_map& p_factor)
{
	Color color = _material_color(p_map);
	if (p_factor.has_value) {
		float factor = float(p_factor.value_real);
		color.r *= factor;
		color.g *= factor;
		color.b *= factor;
	}
	return color;
}

static const ufbx_texture* _get_file_texture(const ufbx_texture* p_texture)
{
	if (!p_texture) {
		return nullptr;
	}
	for (const ufbx_texture* texture : p_texture->file_textures) {
		if (texture->file_index != UFBX_NO_INDEX) {
			return texture;
		}
	}
	return nullptr;
}

static Ref<Image> _get_decompressed_image(Ref<Texture2D> texture)
{
	if (texture.is_null()) {
		return Ref<Image>();
	}
	Ref<Image> image = texture->get_image();
	if (image.is_null()) {
		return Ref<Image>();
	}
	image = image->duplicate();
	image->decompress();
	return image;
}

static Vector<Vector2> _decode_vertex_attrib_vec2(
	const ufbx_vertex_vec2& p_attrib, const Vector<uint32_t>& p_indices)
{
	Vector<Vector2> ret;

	int num_indices = p_indices.size();
	ret.resize(num_indices);
	for (int i = 0; i < num_indices; i++) {
		ret.write[i] = _as_vec2(p_attrib[p_indices[i]]);
	}
	return ret;
}

static Vector<Vector3> _decode_vertex_attrib_vec3(
	const ufbx_vertex_vec3& p_attrib, const Vector<uint32_t>& p_indices)
{
	Vector<Vector3> ret;

	int num_indices = p_indices.size();
	ret.resize(num_indices);
	for (int i = 0; i < num_indices; i++) {
		ret.write[i] = FBXDocument::_as_vec3(p_attrib[p_indices[i]]);
	}
	return ret;
}

static Vector<float> _decode_vertex_attrib_vec3_as_tangent(
	const ufbx_vertex_vec3& p_attrib, const Vector<uint32_t>& p_indices)
{
	Vector<float> ret;

	int num_indices = p_indices.size();
	ret.resize(num_indices * 4);
	for (int i = 0; i < num_indices; i++) {
		Vector3 v = FBXDocument::_as_vec3(p_attrib[p_indices[i]]);
		ret.write[i * 4 + 0] = v.x;
		ret.write[i * 4 + 1] = v.y;
		ret.write[i * 4 + 2] = v.z;
		ret.write[i * 4 + 3] = 1.0f;
	}
	return ret;
}

static Vector<Color> _decode_vertex_attrib_color(
	const ufbx_vertex_vec4& p_attrib, const Vector<uint32_t>& p_indices)
{
	Vector<Color> ret;

	int num_indices = p_indices.size();
	ret.resize(num_indices);
	for (int i = 0; i < num_indices; i++) {
		ret.write[i] = _as_color(p_attrib[p_indices[i]]);
	}
	return ret;
}

static Vector3 _encode_vertex_index(uint32_t p_index)
{
	return Vector3(real_t(p_index & 0xffff), real_t(p_index >> 16), 0.0f);
}

static uint32_t _decode_vertex_index(const Vector3& p_vertex)
{
	return uint32_t(p_vertex.x) | uint32_t(p_vertex.y) << 16;
}

static ufbx_skin_deformer* _find_skin_deformer(ufbx_skin_cluster* p_cluster)
{
	for (const ufbx_connection& conn : p_cluster->element.connections_src) {
		ufbx_skin_deformer* deformer = ufbx_as_skin_deformer(conn.dst);
		if (deformer) {
			return deformer;
		}
	}
	return nullptr;
}

static String _find_element_name(ufbx_element* p_element)
{
	if (p_element->name.length > 0) {
		return FBXDocument::_as_string(p_element->name);
	}
	else if (p_element->instances.count > 0) {
		return _find_element_name(&p_element->instances[0]->element);
	}
	else {
		return "";
	}
}

struct ThreadPoolFBX
{
	struct Group
	{
		ufbx_thread_pool_context ctx = {};
		uint32_t start_index = 0;
	};

	Group groups[UFBX_THREAD_GROUP_COUNT] = {};
};

static void _thread_pool_task(void* user, uint32_t index)
{
	ThreadPoolFBX::Group* group = (ThreadPoolFBX::Group*)user;
	ufbx_thread_pool_run_task(group->ctx, group->start_index + index);
}

static bool _thread_pool_init_fn(
	void* user, ufbx_thread_pool_context ctx, const ufbx_thread_pool_info* info)
{
	ThreadPoolFBX* pool = (ThreadPoolFBX*)user;
	for (ThreadPoolFBX::Group& group : pool->groups) {
		group.ctx = ctx;
	}
	return true;
}

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
		if (!p_state->unique_animation_names.has(u_name)) {
			break;
		}
		index++;
	}

	p_state->unique_animation_names.insert(u_name);

	return u_name;
}

Error FBXDocument::_parse_scenes(Ref<FBXState> p_state)
{
	p_state->unique_names.insert("Skeleton3D"); // Reserve skeleton name.

	const ufbx_scene* fbx_scene = p_state->scene.get();

	// TODO: Multi-document support, would need test files for structure
	p_state->scene_name = "";

	// TODO: Append the root node directly if we use root-based space conversion
	for (const ufbx_node* root_node : fbx_scene->root_node->children) {
		p_state->root_nodes.push_back(int(root_node->typed_id));
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
			String base_dir = p_state->get_base_path();
			Ref<Texture2D> texture =
				ResourceLoader::load(_get_texture_path(base_dir, path), "Texture2D");
			if (texture.is_valid()) {
				p_state->images.push_back(texture);
				p_state->source_images.push_back(texture->get_image());
				continue;
			}
			// Fallback to loading as byte array.
			data = FileAccess::get_file_as_bytes(path);
			if (data.is_empty()) {
				WARN_PRINT(vformat("FBX: Image index '%d' couldn't be loaded from path: %s because "
								   "there was no data to load. Skipping it.",
					texture_i, path));
				p_state->images.push_back(Ref<Texture2D>()); // Placeholder to keep count.
				p_state->source_images.push_back(Ref<Image>());
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
		p_state->textures.push_back(texture);
	}

	print_verbose("FBX: Total images: " + itos(p_state->images.size()));

	return OK;
}

Ref<Texture2D> FBXDocument::_get_texture(
	Ref<FBXState> p_state, const GLTFTextureIndex p_texture, int p_texture_types)
{
	ERR_FAIL_INDEX_V(p_texture, p_state->textures.size(), Ref<Texture2D>());
	const GLTFImageIndex image = p_state->textures[p_texture]->get_src_image();
	ERR_FAIL_INDEX_V(image, p_state->images.size(), Ref<Texture2D>());
	if (FBXState::HandleBinaryImageMode(p_state->handle_binary_image_mode) ==
		FBXState::HANDLE_BINARY_IMAGE_MODE_EMBED_AS_BASISU) {
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

Error FBXDocument::_parse_materials(Ref<FBXState> p_state)
{
	const ufbx_scene* fbx_scene = p_state->scene.get();
	for (GLTFMaterialIndex material_i = 0;
		 material_i < static_cast<GLTFMaterialIndex>(fbx_scene->materials.count); material_i++) {
		const ufbx_material* fbx_material = fbx_scene->materials[material_i];

		Ref<StandardMaterial3D> material;
		material.instantiate();
		if (fbx_material->name.length > 0) {
			material->set_name(_as_string(fbx_material->name));
		}
		else {
			material->set_name(vformat("material_%s", itos(material_i)));
		}
		material->set_flag(BaseMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);

		if (fbx_material->pbr.base_color.has_value) {
			Color albedo =
				_material_color(fbx_material->pbr.base_color, fbx_material->pbr.base_factor);
			material->set_albedo(albedo.linear_to_srgb());
		}

		if (fbx_material->features.double_sided.enabled) {
			material->set_cull_mode(BaseMaterial3D::CULL_DISABLED);
		}

		const ufbx_texture* base_texture = _get_file_texture(fbx_material->pbr.base_color.texture);
		if (base_texture) {
			bool wrap = base_texture->wrap_u == UFBX_WRAP_REPEAT &&
						base_texture->wrap_v == UFBX_WRAP_REPEAT;
			material->set_flag(BaseMaterial3D::FLAG_USE_TEXTURE_REPEAT, wrap);

			Ref<Texture2D> albedo_texture = _get_texture(
				p_state, GLTFTextureIndex(base_texture->file_index), TEXTURE_TYPE_GENERIC);

			// Search for transparency map.
			Ref<Texture2D> transparency_texture;
			const ufbx_texture* transparency_sources[] = {
				fbx_material->pbr.opacity.texture,
				fbx_material->fbx.transparency_color.texture,
			};
			for (const ufbx_texture* transparency_source : transparency_sources) {
				const ufbx_texture* fbx_transparency_texture =
					_get_file_texture(transparency_source);
				if (fbx_transparency_texture) {
					transparency_texture = _get_texture(p_state,
						GLTFTextureIndex(fbx_transparency_texture->file_index),
						TEXTURE_TYPE_GENERIC);
					if (transparency_texture.is_valid()) {
						break;
					}
				}
			}

			// Multiply the albedo alpha with the transparency texture if necessary.
			if (albedo_texture.is_valid() && transparency_texture.is_valid() &&
				albedo_texture != transparency_texture) {
				Pair<uint64_t, uint64_t> key = {
					albedo_texture->get_rid().get_id(), transparency_texture->get_rid().get_id()};
				GLTFTextureIndex* texture_index_ptr =
					p_state->albedo_transparency_textures.getptr(key);
				if (texture_index_ptr != nullptr) {
					if (*texture_index_ptr >= 0) {
						albedo_texture =
							_get_texture(p_state, *texture_index_ptr, TEXTURE_TYPE_GENERIC);
					}
				}
				else {
					Ref<Image> albedo_image = _get_decompressed_image(albedo_texture);
					Ref<Image> transparency_image = _get_decompressed_image(transparency_texture);

					if (albedo_image.is_valid() && transparency_image.is_valid()) {
						albedo_image->convert(Image::Format::FORMAT_RGBA8);
						transparency_image->resize(albedo_texture->get_width(),
							albedo_texture->get_height(), Image::INTERPOLATE_LANCZOS);
						for (int y = 0; y < albedo_image->get_height(); y++) {
							for (int x = 0; x < albedo_image->get_width(); x++) {
								Color albedo_pixel = albedo_image->get_pixel(x, y);
								Color transparency_pixel = transparency_image->get_pixel(x, y);
								albedo_pixel.a *= transparency_pixel.r;
								albedo_image->set_pixel(x, y, albedo_pixel);
							}
						}

						albedo_image->clear_mipmaps();
						albedo_image->generate_mipmaps();

						albedo_image->set_name(
							vformat("alpha_%d", p_state->albedo_transparency_textures.size()));

						GLTFImageIndex new_image = _parse_image_save_image(
							p_state, PackedByteArray(), "", -1, albedo_image);
						if (new_image >= 0) {
							Ref<GLTFTexture> new_texture;
							new_texture.instantiate();
							new_texture->set_src_image(GLTFImageIndex(new_image));
							p_state->textures.push_back(new_texture);

							GLTFTextureIndex texture_index = p_state->textures.size() - 1;
							p_state->albedo_transparency_textures[key] = texture_index;

							albedo_texture =
								_get_texture(p_state, texture_index, TEXTURE_TYPE_GENERIC);
						}
						else {
							WARN_PRINT(vformat(
								"FBX: Could not save modified albedo texture from RID (%d, %d).",
								key.first, key.second));
							p_state->albedo_transparency_textures[key] = -1;
						}
					}
				}
			}

			Image::AlphaMode alpha_mode;
			if (albedo_texture.is_valid()) {
				Image::AlphaMode* alpha_mode_ptr =
					p_state->alpha_mode_cache.getptr(albedo_texture->get_rid().get_id());
				if (alpha_mode_ptr != nullptr) {
					alpha_mode = *alpha_mode_ptr;
				}
				else {
					Ref<Image> albedo_image = _get_decompressed_image(albedo_texture);
					alpha_mode = albedo_image->detect_alpha();
					p_state->alpha_mode_cache[albedo_texture->get_rid().get_id()] = alpha_mode;
				}

				if (alpha_mode == Image::ALPHA_BLEND) {
					material->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA_DEPTH_PRE_PASS);
				}
				else if (alpha_mode == Image::ALPHA_BIT) {
					material->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA_SCISSOR);
				}
				material->set_texture(BaseMaterial3D::TEXTURE_ALBEDO, albedo_texture);
			}

			// Combined textures and factors are very unreliable in FBX
			Color albedo_factor = Color(1, 1, 1);
			if (fbx_material->pbr.base_factor.has_value) {
				albedo_factor *= (float)fbx_material->pbr.base_factor.value_real;
			}
			material->set_albedo(albedo_factor.linear_to_srgb());

			// TODO: Does not support rotation, could be inverted?
			material->set_uv1_offset(_as_vec3(base_texture->uv_transform.translation));
			Vector3 scale = _as_vec3(base_texture->uv_transform.scale);
			material->set_uv1_scale(scale);
		}

		if (fbx_material->features.pbr.enabled) {
			if (fbx_material->pbr.metalness.has_value) {
				material->set_metallic(float(fbx_material->pbr.metalness.value_real));
			}
			else {
				material->set_metallic(1.0);
			}

			if (fbx_material->pbr.roughness.has_value) {
				material->set_roughness(float(fbx_material->pbr.roughness.value_real));
			}
			else {
				material->set_roughness(1.0);
			}

			const ufbx_texture* metalness_texture =
				_get_file_texture(fbx_material->pbr.metalness.texture);
			if (metalness_texture) {
				material->set_texture(BaseMaterial3D::TEXTURE_METALLIC,
					_get_texture(p_state, GLTFTextureIndex(metalness_texture->file_index),
						TEXTURE_TYPE_GENERIC));
				material->set_metallic_texture_channel(BaseMaterial3D::TEXTURE_CHANNEL_RED);
				material->set_metallic(1.0);
			}

			const ufbx_texture* roughness_texture =
				_get_file_texture(fbx_material->pbr.roughness.texture);
			if (roughness_texture) {
				material->set_texture(BaseMaterial3D::TEXTURE_ROUGHNESS,
					_get_texture(p_state, GLTFTextureIndex(roughness_texture->file_index),
						TEXTURE_TYPE_GENERIC));
				material->set_roughness_texture_channel(BaseMaterial3D::TEXTURE_CHANNEL_RED);
				material->set_roughness(1.0);
			}
		}

		const ufbx_texture* normal_texture =
			_get_file_texture(fbx_material->pbr.normal_map.texture);
		if (normal_texture) {
			material->set_texture(BaseMaterial3D::TEXTURE_NORMAL,
				_get_texture(
					p_state, GLTFTextureIndex(normal_texture->file_index), TEXTURE_TYPE_NORMAL));
			material->set_feature(BaseMaterial3D::FEATURE_NORMAL_MAPPING, true);
			if (fbx_material->pbr.normal_map.has_value) {
				material->set_normal_scale(fbx_material->pbr.normal_map.value_real);
			}
		}

		const ufbx_texture* occlusion_texture =
			_get_file_texture(fbx_material->pbr.ambient_occlusion.texture);
		if (occlusion_texture) {
			material->set_texture(BaseMaterial3D::TEXTURE_AMBIENT_OCCLUSION,
				_get_texture(p_state, GLTFTextureIndex(occlusion_texture->file_index),
					TEXTURE_TYPE_GENERIC));
			material->set_ao_texture_channel(BaseMaterial3D::TEXTURE_CHANNEL_RED);
			material->set_feature(BaseMaterial3D::FEATURE_AMBIENT_OCCLUSION, true);
		}

		if (fbx_material->pbr.emission_color.has_value) {
			material->set_feature(BaseMaterial3D::FEATURE_EMISSION, true);
			material->set_emission(
				_material_color(fbx_material->pbr.emission_color).linear_to_srgb());
			material->set_emission_energy_multiplier(
				float(fbx_material->pbr.emission_factor.value_real));
		}

		const ufbx_texture* emission_texture =
			_get_file_texture(fbx_material->pbr.emission_color.texture);
		if (emission_texture) {
			material->set_texture(BaseMaterial3D::TEXTURE_EMISSION,
				_get_texture(
					p_state, GLTFTextureIndex(emission_texture->file_index), TEXTURE_TYPE_GENERIC));
			material->set_feature(BaseMaterial3D::FEATURE_EMISSION, true);
			material->set_emission(Color(0, 0, 0));
		}

		if (fbx_material->features.double_sided.enabled &&
			fbx_material->features.double_sided.is_explicit) {
			material->set_cull_mode(BaseMaterial3D::CULL_DISABLED);
		}
		p_state->materials.push_back(material);
	}

	print_verbose("Total materials: " + itos(p_state->materials.size()));

	return OK;
}

Error FBXDocument::_parse_cameras(Ref<FBXState> p_state)
{
	const ufbx_scene* fbx_scene = p_state->scene.get();
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
		p_state->cameras.push_back(camera);
	}

	print_verbose("FBX: Total cameras: " + itos(p_state->cameras.size()));

	return OK;
}

void FBXDocument::_assign_node_names(Ref<FBXState> p_state)
{
	for (int i = 0; i < p_state->nodes.size(); i++) {
		Ref<GLTFNode> fbx_node = p_state->nodes[i];

		// Any joints get unique names generated when the skeleton is made, unique to the skeleton
		if (fbx_node->skeleton >= 0) {
			continue;
		}

		if (fbx_node->get_name().is_empty()) {
			if (fbx_node->mesh >= 0) {
				fbx_node->set_name(_gen_unique_name(p_state->unique_names, "Mesh"));
			}
			else if (fbx_node->camera >= 0) {
				fbx_node->set_name(_gen_unique_name(p_state->unique_names, "Camera3D"));
			}
			else {
				fbx_node->set_name(_gen_unique_name(p_state->unique_names, "Node"));
			}
		}

		fbx_node->set_name(_gen_unique_name(p_state->unique_names, fbx_node->get_name()));
	}
}

BoneAttachment3D* FBXDocument::_generate_bone_attachment(Ref<FBXState> p_state,
	Skeleton3D* p_skeleton, const GLTFNodeIndex p_node_index, const GLTFNodeIndex p_bone_index)
{
	Ref<GLTFNode> fbx_node = p_state->nodes[p_node_index];
	Ref<GLTFNode> bone_node = p_state->nodes[p_bone_index];
	BoneAttachment3D* bone_attachment = memnew(BoneAttachment3D);
	print_verbose("FBX: Creating bone attachment for: " + fbx_node->get_name());

	ERR_FAIL_COND_V(!bone_node->joint, nullptr);

	bone_attachment->set_bone_name(bone_node->get_name());

	return bone_attachment;
}

ImporterMeshInstance3D* FBXDocument::_generate_mesh_instance(
	Ref<FBXState> p_state, const GLTFNodeIndex p_node_index)
{
	Ref<GLTFNode> fbx_node = p_state->nodes[p_node_index];

	ERR_FAIL_INDEX_V(fbx_node->mesh, p_state->meshes.size(), nullptr);

	ImporterMeshInstance3D* mi = memnew(ImporterMeshInstance3D);
	print_verbose("FBX: Creating mesh for: " + fbx_node->get_name());

	p_state->scene_mesh_instances.insert(p_node_index, mi);
	Ref<GLTFMesh> mesh = p_state->meshes.write[fbx_node->mesh];
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

Camera3D* FBXDocument::_generate_camera(Ref<FBXState> p_state, const GLTFNodeIndex p_node_index)
{
	Ref<GLTFNode> fbx_node = p_state->nodes[p_node_index];

	ERR_FAIL_INDEX_V(fbx_node->camera, p_state->cameras.size(), nullptr);

	print_verbose("FBX: Creating camera for: " + fbx_node->get_name());

	Ref<GLTFCamera> c = p_state->cameras[fbx_node->camera];
	return c->to_node();
}

Node3D* FBXDocument::_generate_spatial(Ref<FBXState> p_state, const GLTFNodeIndex p_node_index)
{
	Ref<GLTFNode> fbx_node = p_state->nodes[p_node_index];

	Node3D* spatial = memnew(Node3D);
	print_verbose("FBX: Converting spatial: " + fbx_node->get_name());

	return spatial;
}

Error FBXDocument::append_from_buffer(const PackedByteArray& p_bytes, const String& p_base_path,
	Ref<GLTFState> p_state, uint32_t p_flags)
{
	Ref<FBXState> state = p_state;
	ERR_FAIL_COND_V(state.is_null(), ERR_INVALID_PARAMETER);
	ERR_FAIL_NULL_V(p_bytes.ptr(), ERR_INVALID_DATA);
	Error err = FAILED;
	state->use_named_skin_binds =
		p_flags & GLTFDocument::ImportFlags::IMPORT_FLAG_USE_NAMED_SKIN_BINDS;
	state->discard_meshes_and_materials =
		p_flags & GLTFDocument::ImportFlags::IMPORT_FLAG_DISCARD_MESHES_AND_MATERIALS;

	Ref<FileAccessMemory> file_access;
	file_access.instantiate();
	file_access->open_custom(p_bytes.ptr(), p_bytes.size());
	state->base_path = p_base_path.get_base_dir();
	err = _parse(state, state->base_path, file_access);
	ERR_FAIL_COND_V(err != OK, err);
	for (Ref<GLTFDocumentExtension> ext : document_extensions) {
		ERR_CONTINUE(ext.is_null());
		err = ext->import_post_parse(state);
		ERR_FAIL_COND_V(err != OK, err);
	}
	return OK;
}

Error FBXDocument::append_from_file(
	const String& p_path, Ref<GLTFState> p_state, uint32_t p_flags, const String& p_base_path)
{
	Ref<FBXState> state = p_state;
	ERR_FAIL_COND_V(state.is_null(), ERR_INVALID_PARAMETER);
	ERR_FAIL_COND_V(p_path.is_empty(), ERR_FILE_NOT_FOUND);
	if (p_state == Ref<FBXState>()) {
		p_state.instantiate();
	}
	state->filename = p_path.get_file().get_basename();
	state->use_named_skin_binds =
		p_flags & GLTFDocument::ImportFlags::IMPORT_FLAG_USE_NAMED_SKIN_BINDS;
	state->discard_meshes_and_materials =
		p_flags & GLTFDocument::ImportFlags::IMPORT_FLAG_DISCARD_MESHES_AND_MATERIALS;
	Error err;
	Ref<FileAccess> file = FileAccess::open(p_path, FileAccess::READ, &err);
	ERR_FAIL_COND_V(err != OK, ERR_FILE_CANT_OPEN);
	ERR_FAIL_COND_V(file.is_null(), ERR_FILE_CANT_OPEN);
	String base_path = p_base_path;
	if (base_path.is_empty()) {
		base_path = p_path.get_base_dir();
	}
	state->base_path = base_path;
	err = _parse(p_state, base_path, file);
	ERR_FAIL_COND_V(err != OK, err);
	for (Ref<GLTFDocumentExtension> ext : document_extensions) {
		ERR_CONTINUE(ext.is_null());
		err = ext->import_post_parse(p_state);
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
			p_state->nodes.write[node]->joint = true;
		}

		if (fbx_skin->name.length > 0) {
			skin->set_name(FBXDocument::_as_string(fbx_skin->name));
		}
		else {
			skin->set_name(vformat("skin_%s", itos(fbx_skin->typed_id)));
		}
		p_state->skin_indices.push_back(p_state->skins.size());
		p_state->skins.push_back(skin);
	}

	for (const ufbx_bone* fbx_bone : fbx_scene->bones) {
		for (const ufbx_node* fbx_node : fbx_bone->instances) {
			const GLTFNodeIndex node = fbx_node->typed_id;
			if (!p_state->nodes.write[node]->joint) {
				p_state->nodes.write[node]->joint = true;

				if (!(fbx_node->parent && fbx_node->parent->attrib_type == UFBX_ELEMENT_BONE)) {
					Ref<GLTFSkin> skin;
					skin.instantiate();
					skin->joints.push_back(node);
					skin->joints_original.push_back(node);
					skin->set_name(vformat("skin_%s", itos(p_state->skins.size())));
					p_state->skin_indices.push_back(p_state->skins.size());
					p_state->skins.push_back(skin);
				}
			}
		}
	}
	p_state->original_skin_indices = p_state->skin_indices.duplicate();
	Error err =
		SkinTool::_asset_parse_skins(p_state->original_skin_indices, p_state->skins.duplicate(),
			p_state->nodes.duplicate(), p_state->skin_indices, p_state->skins, joint_mapping);
	if (err != OK) {
		return err;
	}
	for (int i = 0; i < p_state->skins.size(); ++i) {
		Ref<GLTFSkin> skin = p_state->skins.write[i];
		ERR_FAIL_COND_V(skin.is_null(), ERR_PARSE_ERROR);
		// Expand and verify the skin
		ERR_FAIL_COND_V(SkinTool::_expand_skin(p_state->nodes, skin), ERR_PARSE_ERROR);
		ERR_FAIL_COND_V(SkinTool::_verify_skin(p_state->nodes, skin), ERR_PARSE_ERROR);
	}

	print_verbose("FBX: Total skins: " + itos(p_state->skins.size()));

	for (HashMap<GLTFNodeIndex, bool>::Iterator it = joint_mapping.begin();
		 it != joint_mapping.end(); ++it) {
		GLTFNodeIndex node_index = it->key;
		bool is_joint = it->value;
		if (is_joint) {
			if (p_state->nodes.size() > node_index) {
				p_state->nodes.write[node_index]->joint = true;
			}
		}
	}

	return OK;
}

PackedByteArray FBXDocument::generate_buffer(Ref<GLTFState> p_state) { return PackedByteArray(); }

Error FBXDocument::write_to_filesystem(Ref<GLTFState> p_state, const String& p_path)
{
	return ERR_UNAVAILABLE;
}

Error FBXDocument::append_from_scene(Node* p_node, Ref<GLTFState> p_state, uint32_t p_flags)
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


