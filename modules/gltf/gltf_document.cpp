#ifndef _3D_DISABLED
#include "modules/modules_enabled.gen.h"

#if defined(MODULE_GLTF_ENABLED)
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/crypto/crypto.h"
#include "core/io/config_file.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/file_access_wrappers.h"
#include "core/io/config_file.h"
#include "core/io/resource_loader.h"
#include "core/io/stream_peer.h"
#include "core/version.h"
#include "gltf_document.h"
#include "modules/gltf/gltf_document.h"
#include "scene/3d/node_3d.h"
#include "scene/main/scene_tree.h"
#include "scene/3d/node_3d.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/mesh.h"
// --- Inlined from core/math/disjoint_set.h ---
#include "core/templates/hash_map.h"
#include "core/templates/vector.h"

/* This DisjointSet class uses Find with path compression and Union by rank */
template <typename T, typename H = HashMapHasherDefault, typename C = HashMapComparatorDefault<T>, typename AL = DefaultAllocator>
class DisjointSet {
	struct Element {
		T object;
		Element *parent = nullptr;
		int rank = 0;
	};

	typedef HashMap<T, Element *, H, C> MapT;

	MapT elements;

	Element *get_parent(Element *r_element);

	_FORCE_INLINE_ Element *insert_or_get(T p_object);

public:
	~DisjointSet();

	_FORCE_INLINE_ void insert(T p_object) { (void)insert_or_get(p_object); }

	void create_union(T p_left, T p_right);

	void get_representatives(Vector<T> &r_out_roots);

	void get_members(Vector<T> &r_out_members, T p_representative);
};

/* FUNCTIONS */

template <typename T, typename H, typename C, typename AL>
DisjointSet<T, H, C, AL>::~DisjointSet() {
	for (KeyValue<T, Element *> &E : elements) {
		memdelete_allocator<Element, AL>(E.value);
	}
}

template <typename T, typename H, typename C, typename AL>
typename DisjointSet<T, H, C, AL>::Element *DisjointSet<T, H, C, AL>::get_parent(Element *r_element) {
	if (r_element->parent != r_element) {
		r_element->parent = get_parent(r_element->parent);
	}

	return r_element->parent;
}

template <typename T, typename H, typename C, typename AL>
typename DisjointSet<T, H, C, AL>::Element *DisjointSet<T, H, C, AL>::insert_or_get(T p_object) {
	typename MapT::Iterator itr = elements.find(p_object);
	if (itr != nullptr) {
		return itr->value;
	}

	Element *new_element = memnew_allocator(Element, AL);
	new_element->object = p_object;
	new_element->parent = new_element;
	elements.insert(p_object, new_element);

	return new_element;
}

template <typename T, typename H, typename C, typename AL>
void DisjointSet<T, H, C, AL>::create_union(T p_left, T p_right) {
	Element *x = insert_or_get(p_left);
	Element *y = insert_or_get(p_right);

	Element *x_root = get_parent(x);
	Element *y_root = get_parent(y);

	// Already in the same set
	if (x_root == y_root) {
		return;
	}

	// Not in the same set, merge
	if (x_root->rank < y_root->rank) {
		SWAP(x_root, y_root);
	}

	// Merge y_root into x_root
	y_root->parent = x_root;
	if (x_root->rank == y_root->rank) {
		++x_root->rank;
	}
}

template <typename T, typename H, typename C, typename AL>
void DisjointSet<T, H, C, AL>::get_representatives(Vector<T> &r_out_representatives) {
	for (KeyValue<T, Element *> &E : elements) {
		Element *element = E.value;
		if (element->parent == element) {
			r_out_representatives.push_back(element->object);
		}
	}
}

template <typename T, typename H, typename C, typename AL>
void DisjointSet<T, H, C, AL>::get_members(Vector<T> &r_out_members, T p_representative) {
	typename MapT::Iterator rep_itr = elements.find(p_representative);
	ERR_FAIL_NULL(rep_itr);

	Element *rep_element = rep_itr->value;
	ERR_FAIL_COND(rep_element->parent != rep_element);

	for (KeyValue<T, Element *> &E : elements) {
		Element *parent = get_parent(E.value);
		if (parent == rep_element) {
			r_out_members.push_back(E.key);
		}
	}
}

// --- End core/math/disjoint_set.h ---
#include "main/main.h"

#ifdef TOOLS_ENABLED
#include "editor/settings/editor_settings.h"
#endif

#include "modules/modules_enabled.gen.h" // For csg, gridmap.

#ifdef MODULE_CSG_ENABLED
#include "modules/gltf/gltf_document.h"
#endif
#ifdef MODULE_GRIDMAP_ENABLED
#include "modules/gltf/gltf_document.h"
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
#include "core/io/resource_loader.h"

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

#include "modules/gltf/gltf_document.h"

#include "scene/3d/node_3d.h"

#include <cfloat>

#ifndef PHYSICS_3D_DISABLED
#include "core/math/geometry_3d.h"
#include "scene/3d/node_3d.h"
#include "scene/resources/mesh.h"
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

#include "modules/gltf/gltf_document.h"

#include "core/types.h"
#include "modules/gltf/gltf_document.h"
#include "scene/3d/node_3d.h"

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
#endif // defined(MODULE_GLTF_ENABLED)


// --- Merged from modules/fbx/fbx_document.cpp ---
#if defined(MODULE_FBX_ENABLED)
#include "core/config/engine.h"
#include "main/main.h"
#include "core/config/project_settings.h"
#include "core/crypto/crypto.h"
#include "core/io/config_file.h"
#include "core/io/file_access.h"
#include "core/io/file_access_wrappers.h"
#include "core/io/image.h"
#include "core/io/resource_loader.h"
#include "core/templates/hashfuncs.h"
// --- Inlined from modules/fbx/fbx_document.h ---
#include <ufbx.h>
#include "core/io/resource.h"
#include "modules/gltf/gltf_document.h"

class FBXState final : public Resource
{
	friend class FBXDocument;
	friend class SkinTool;
	friend class GLTFSkin;

	Ref<GLTFState> gltf_state;

	// Smart pointer that holds the loaded scene.
	ufbx_unique_ptr<ufbx_scene> scene;
	bool allow_geometry_helper_nodes = false;

	HashMap<uint64_t, Image::AlphaMode> alpha_mode_cache;
	HashMap<Pair<uint64_t, uint64_t>, GLTFTextureIndex> albedo_transparency_textures;

	Vector<GLTFSkinIndex> skin_indices;
	Vector<GLTFSkinIndex> original_skin_indices;
	HashSet<String> unique_mesh_names;

public:
	Ref<GLTFState> get_gltf_state() const { return gltf_state; }

	bool get_allow_geometry_helper_nodes();
	void set_allow_geometry_helper_nodes(bool p_allow_geometry_helper_nodes);

	FBXState();
};
#include "modules/gltf/gltf_document.h"

class FBXDocument final : public Resource
{
	int _naming_version = 2;
	Ref<GLTFDocument> gltf_document;

public:
	enum
	{
		TEXTURE_TYPE_GENERIC = 0,
		TEXTURE_TYPE_NORMAL = 1,
	};

	Ref<GLTFDocument> get_gltf_document() const { return gltf_document; }

	static Transform3D _as_xform(const ufbx_matrix& p_mat);
	static String _as_string(const ufbx_string& p_string);
	static Vector3 _as_vec3(const ufbx_vec3& p_vector);
	static String _gen_unique_name(HashSet<String>& unique_names, const String& p_name);

	Error append_from_file(const String& p_path, Ref<FBXState> p_state, uint32_t p_flags = 0,
		const String& p_base_path = String());
	Error append_from_buffer(const PackedByteArray& p_bytes, const String& p_base_path,
		Ref<FBXState> p_state, uint32_t p_flags = 0);
	Error append_from_scene(Node* p_node, Ref<FBXState> p_state, uint32_t p_flags = 0);

	Node* generate_scene(Ref<FBXState> p_state, float p_bake_fps = 30.0f, bool p_trimming = false,
		bool p_remove_immutable_tracks = true);
	PackedByteArray generate_buffer(Ref<FBXState> p_state);
	Error write_to_filesystem(Ref<FBXState> p_state, const String& p_path);

	void set_naming_version(int p_version);
	int get_naming_version() const;

	FBXDocument();

private:
	String _get_texture_path(
		const String& p_base_directory, const String& p_source_file_path) const;
	void _process_uv_set(PackedVector2Array& uv_array);
	void _zero_unused_elements(Vector<float>& cur_custom, int start, int end, int num_channels);
	Error _parse_scenes(Ref<FBXState> p_state);
	Error _parse_nodes(Ref<FBXState> p_state);
	String _sanitize_animation_name(const String& p_name);
	String _gen_unique_animation_name(Ref<FBXState> p_state, const String& p_name);
	Error _parse_meshes(Ref<FBXState> p_state);
	Ref<Image> _parse_image_bytes_into_image(Ref<FBXState> p_state, const Vector<uint8_t>& p_bytes,
		const String& p_filename, int p_index);
	GLTFImageIndex _parse_image_save_image(Ref<FBXState> p_state, const Vector<uint8_t>& p_bytes,
		const String& p_file_extension, int p_index, Ref<Image> p_image);
	Error _parse_images(Ref<FBXState> p_state, const String& p_base_path);
	Error _parse_skins(Ref<FBXState> p_state);
	Error _parse_animations(Ref<FBXState> p_state);
	BoneAttachment3D* _generate_bone_attachment(Ref<FBXState> p_state, Skeleton3D* p_skeleton,
		const GLTFNodeIndex p_node_index, const GLTFNodeIndex p_bone_index);
	ImporterMeshInstance3D* _generate_mesh_instance(
		Ref<FBXState> p_state, const GLTFNodeIndex p_node_index);
	Light3D* _generate_light(Ref<FBXState> p_state, const GLTFNodeIndex p_node_index);
	Node3D* _generate_spatial(Ref<FBXState> p_state, const GLTFNodeIndex p_node_index);
	void _assign_node_names(Ref<FBXState> p_state);
	Error _parse_cameras(Ref<FBXState> p_state);
	Error _parse_lights(Ref<FBXState> p_state);

public:
	Error _parse_fbx_state(Ref<FBXState> p_state, const String& p_search_path);
	void _process_mesh_instances(Ref<FBXState> p_state, Node* p_scene_root);
	void _generate_scene_node(Ref<FBXState> p_state, const GLTFNodeIndex p_node_index,
		Node* p_scene_parent, Node* p_scene_root);
	void _generate_skeleton_bone_node(Ref<FBXState> p_state, const GLTFNodeIndex p_node_index,
		Node* p_scene_parent, Node* p_scene_root);
	void _import_animation(Ref<FBXState> p_state, AnimationPlayer* p_animation_player,
		const GLTFAnimationIndex p_index, const bool p_trimming,
		const bool p_remove_immutable_tracks);
	Error _parse(Ref<FBXState> p_state, const String& p_path, Ref<FileAccess> p_file);
};

#ifdef TOOLS_ENABLED
#include "modules/gltf/gltf_document.h"

class Animation;
class Node;

class EditorSceneFormatImporterFBX final : public EditorSceneFormatImporter
{
public:
	enum Backend
	{
		BACKEND_UFBX,
		BACKEND_FBX2GLTF,
	};

private:
	Backend backend = BACKEND_UFBX;

public:
	explicit EditorSceneFormatImporterFBX(Backend p_backend = BACKEND_UFBX) : backend(p_backend) {}

	Backend get_backend() const { return backend; }

	virtual void get_extensions(List<String>* r_extensions) const override;
	virtual void get_import_options(
		const String& p_path, List<ResourceImporter::ImportOption>* r_options) override;
};
#endif // TOOLS_ENABLED

// --- End modules/fbx/fbx_document.h ---
#include "modules/gltf/gltf_document.h"
#include "scene/3d/node_3d.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/material.h"
#include "scene/resources/mesh.h"

#ifdef TOOLS_ENABLED
#include "editor/settings/editor_settings.h"
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

FBXState::FBXState() { gltf_state.instantiate(); }

bool FBXState::get_allow_geometry_helper_nodes() { return allow_geometry_helper_nodes; }

void FBXState::set_allow_geometry_helper_nodes(bool p_allow_geometry_helper_nodes)
{
	allow_geometry_helper_nodes = p_allow_geometry_helper_nodes;
}

#ifdef TOOLS_ENABLED
#include "core/io/resource_loader.h"

void EditorSceneFormatImporterFBX::get_extensions(List<String>* r_extensions) const
{
	r_extensions->push_back("fbx");
}

void EditorSceneFormatImporterFBX::get_import_options(
	const String& p_path, List<ResourceImporter::ImportOption>* r_options)
{
}
#endif // TOOLS_ENABLED

#ifdef TOOLS_ENABLED
#endif // TOOLS_ENABLED

#define FBX_REGISTER_DOCUMENT_EXTENSION(m_doc_ext_class)                                           \
	Ref<m_doc_ext_class> extension_##m_doc_ext_class;                                              \
	extension_##m_doc_ext_class.instantiate();                                                     \
	GLTFDocument::register_gltf_document_extension(extension_##m_doc_ext_class);

void uninitialize_fbx_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	GLTFDocument::unregister_all_gltf_document_extensions();
}

void initialize_fbx_module(ModuleInitializationLevel p_level)
{
#ifdef TOOLS_ENABLED
	if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
		Ref<EditorSceneFormatImporterFBX> import_fbx;
		import_fbx.instantiate(EditorSceneFormatImporterFBX::BACKEND_UFBX);
		ResourceImporterScene::add_scene_importer(import_fbx);
	}
#endif // TOOLS_ENABLED
}
#endif // defined(MODULE_FBX_ENABLED)

// === Merged from modules/csg/csg_shape.cpp ===
#include "modules/modules_enabled.gen.h"

#if defined(MODULE_CSG_ENABLED)
#include "core/config/engine.h"
#include "main/main.h"
#include "core/math/geometry_2d.h"
#include "modules/gltf/gltf_document.h"
#include "scene/main/scene_tree.h"
#include "servers/navigation_3d/navigation_server_3d.h"
#include "scene/resources/mesh.h"

#ifndef PHYSICS_3D_DISABLED
#include "servers/rendering/renderer.h" // Only used for debug collision shapes.
#endif											// PHYSICS_3D_DISABLED

#ifdef DEV_ENABLED
#include "core/io/config_file.h"
#endif // DEV_ENABLED

#ifndef NAVIGATION_3D_DISABLED
#include "servers/navigation_3d/navigation_server_3d.h"
#endif // NAVIGATION_3D_DISABLED

#include <cfloat> // FLT_EPSILON
#include <manifold/manifold.h>

#ifndef NAVIGATION_3D_DISABLED
RID CSGShape3D::_navmesh_source_geometry_parser;

#endif // NAVIGATION_3D_DISABLED

#ifndef PHYSICS_3D_DISABLED

bool CSGShape3D::is_using_collision() const { return use_collision; }

void CSGShape3D::set_collision_layer(uint32_t p_layer)
{
	collision_layer = p_layer;
	if (root_collision_body.is_valid()) {
		PhysicsServer3D::body_set_collision_layer(root_collision_body, p_layer);
	}
}

uint32_t CSGShape3D::get_collision_layer() const { return collision_layer; }

void CSGShape3D::set_collision_mask(uint32_t p_mask)
{
	collision_mask = p_mask;
	if (root_collision_body.is_valid()) {
		PhysicsServer3D::body_set_collision_mask(root_collision_body, p_mask);
	}
}

uint32_t CSGShape3D::get_collision_mask() const { return collision_mask; }

void CSGShape3D::set_collision_layer_value(int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 32, "Collision layer number must be between 1 and 32 inclusive.");
	uint32_t layer = get_collision_layer();
	if (p_value) {
		layer |= 1 << (p_layer_number - 1);
	}
	else {
		layer &= ~(1 << (p_layer_number - 1));
	}
	set_collision_layer(layer);
}

bool CSGShape3D::get_collision_layer_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Collision layer number must be between 1 and 32 inclusive.");
	return get_collision_layer() & (1 << (p_layer_number - 1));
}

void CSGShape3D::set_collision_mask_value(int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 32, "Collision layer number must be between 1 and 32 inclusive.");
	uint32_t mask = get_collision_mask();
	if (p_value) {
		mask |= 1 << (p_layer_number - 1);
	}
	else {
		mask &= ~(1 << (p_layer_number - 1));
	}
	set_collision_mask(mask);
}

bool CSGShape3D::get_collision_mask_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Collision layer number must be between 1 and 32 inclusive.");
	return get_collision_mask() & (1 << (p_layer_number - 1));
}

RID CSGShape3D::_get_root_collision_instance() const
{
	if (root_collision_body.is_valid()) {
		return root_collision_body;
	}
	else if (parent_shape) {
		return parent_shape->_get_root_collision_instance();
	}

	return RID();
}

void CSGShape3D::set_collision_priority(real_t p_priority)
{
	collision_priority = p_priority;
	if (root_collision_body.is_valid()) {
		PhysicsServer3D::body_set_collision_priority(
			root_collision_body, p_priority);
	}
}

real_t CSGShape3D::get_collision_priority() const { return collision_priority; }
#endif // PHYSICS_3D_DISABLED

bool CSGShape3D::is_autosmooth() const { return autosmooth; }

void CSGShape3D::set_smoothing_angle(const float p_angle)
{
	smoothing_angle = p_angle;
	_make_dirty();
}

float CSGShape3D::get_smoothing_angle() const { return smoothing_angle; }

bool CSGShape3D::is_root_shape() const { return !parent_shape; }

#ifndef DISABLE_DEPRECATED
void CSGShape3D::set_snap(float p_snap)
{
	if (snap == p_snap) {
		return;
	}

	snap = p_snap;
	_make_dirty();
}

float CSGShape3D::get_snap() const { return snap; }
#endif // DISABLE_DEPRECATED

enum ManifoldProperty
{
	MANIFOLD_PROPERTY_POSITION_X = 0,
	MANIFOLD_PROPERTY_POSITION_Y,
	MANIFOLD_PROPERTY_POSITION_Z,
	MANIFOLD_PROPERTY_INVERT,
	MANIFOLD_PROPERTY_SMOOTH_GROUP,
	MANIFOLD_PROPERTY_UV_X_0,
	MANIFOLD_PROPERTY_UV_Y_0,
	MANIFOLD_PROPERTY_MAX
};

static void _unpack_manifold(const manifold::Manifold& p_manifold,
	const HashMap<int32_t, Ref<Material>>& p_mesh_materials, CSGBrush* r_mesh_merge)
{
	manifold::MeshGL64 mesh = p_manifold.GetMeshGL64();

	constexpr int32_t order[3] = {0, 2, 1};

	for (size_t run_i = 0; run_i < mesh.runIndex.size() - 1; run_i++) {
		uint32_t original_id = -1;
		if (run_i < mesh.runOriginalID.size()) {
			original_id = mesh.runOriginalID[run_i];
		}

		Ref<Material> material;
		if (p_mesh_materials.has(original_id)) {
			material = p_mesh_materials[original_id];
		}
		// Find or reserve a material ID in the brush.
		int32_t material_id = r_mesh_merge->materials.find(material);
		if (material_id == -1) {
			material_id = r_mesh_merge->materials.size();
			r_mesh_merge->materials.push_back(material);
		}

		size_t begin = mesh.runIndex[run_i];
		size_t end = mesh.runIndex[run_i + 1];
		for (size_t vert_i = begin; vert_i < end; vert_i += 3) {
			CSGBrush::Face face;
			face.material = material_id;
			int32_t first_property_index = mesh.triVerts[vert_i + order[0]];
			face.smooth = mesh.vertProperties[first_property_index * mesh.numProp +
											  MANIFOLD_PROPERTY_SMOOTH_GROUP] > 0.5f;
			face.invert = mesh.vertProperties[first_property_index * mesh.numProp +
											  MANIFOLD_PROPERTY_INVERT] > 0.5f;

			for (int32_t tri_order_i = 0; tri_order_i < 3; tri_order_i++) {
				int32_t property_i = mesh.triVerts[vert_i + order[tri_order_i]];
				ERR_FAIL_COND_MSG(property_i * mesh.numProp >= mesh.vertProperties.size(),
					"Invalid index into vertex properties");
				face.vertices[tri_order_i] = Vector3(
					mesh.vertProperties[property_i * mesh.numProp + MANIFOLD_PROPERTY_POSITION_X],
					mesh.vertProperties[property_i * mesh.numProp + MANIFOLD_PROPERTY_POSITION_Y],
					mesh.vertProperties[property_i * mesh.numProp + MANIFOLD_PROPERTY_POSITION_Z]);
				face.uvs[tri_order_i] = Vector2(
					mesh.vertProperties[property_i * mesh.numProp + MANIFOLD_PROPERTY_UV_X_0],
					mesh.vertProperties[property_i * mesh.numProp + MANIFOLD_PROPERTY_UV_Y_0]);
			}
			r_mesh_merge->faces.push_back(face);
		}
	}

	r_mesh_merge->_regen_face_aabbs();
}

#ifdef DEV_ENABLED
static String _export_meshgl_as_json(const manifold::MeshGL64& p_mesh)
{
	Dictionary mesh_dict;
	mesh_dict["numProp"] = p_mesh.numProp;

	Array vert_properties;
	for (const double& val : p_mesh.vertProperties) {
		vert_properties.append(val);
	}
	mesh_dict["vertProperties"] = vert_properties;

	Array tri_verts;
	for (const uint64_t& val : p_mesh.triVerts) {
		tri_verts.append(val);
	}
	mesh_dict["triVerts"] = tri_verts;

	Array merge_from_vert;
	for (const uint64_t& val : p_mesh.mergeFromVert) {
		merge_from_vert.append(val);
	}
	mesh_dict["mergeFromVert"] = merge_from_vert;

	Array merge_to_vert;
	for (const uint64_t& val : p_mesh.mergeToVert) {
		merge_to_vert.append(val);
	}
	mesh_dict["mergeToVert"] = merge_to_vert;

	Array run_index;
	for (const uint64_t& val : p_mesh.runIndex) {
		run_index.append(val);
	}
	mesh_dict["runIndex"] = run_index;

	Array run_original_id;
	for (const uint32_t& val : p_mesh.runOriginalID) {
		run_original_id.append(val);
	}
	mesh_dict["runOriginalID"] = run_original_id;

	Array run_transform;
	for (const double& val : p_mesh.runTransform) {
		run_transform.append(val);
	}
	mesh_dict["runTransform"] = run_transform;

	Array face_id;
	for (const uint64_t& val : p_mesh.faceID) {
		face_id.append(val);
	}
	mesh_dict["faceID"] = face_id;

	Array halfedge_tangent;
	for (const double& val : p_mesh.halfedgeTangent) {
		halfedge_tangent.append(val);
	}
	mesh_dict["halfedgeTangent"] = halfedge_tangent;

	mesh_dict["tolerance"] = p_mesh.tolerance;

	String json_string = JSON::stringify(mesh_dict);
	return json_string;
}
#endif // DEV_ENABLED

static void _pack_manifold(const CSGBrush* const p_mesh_merge, manifold::Manifold& r_manifold,
	HashMap<int32_t, Ref<Material>>& p_mesh_materials, CSGShape3D* p_csg_shape)
{
	ERR_FAIL_NULL_MSG(p_mesh_merge, "p_mesh_merge is null");
	ERR_FAIL_NULL_MSG(p_csg_shape, "p_shape is null");
	HashMap<uint32_t, Vector<CSGBrush::Face>> faces_by_material;
	for (int face_i = 0; face_i < p_mesh_merge->faces.size(); face_i++) {
		const CSGBrush::Face& face = p_mesh_merge->faces[face_i];
		faces_by_material[face.material].push_back(face);
	}

	manifold::MeshGL64 mesh;
	mesh.numProp = MANIFOLD_PROPERTY_MAX;
	mesh.runOriginalID.reserve(faces_by_material.size());
	mesh.runIndex.reserve(faces_by_material.size() + 1);
	mesh.vertProperties.reserve(p_mesh_merge->faces.size() * 3 * MANIFOLD_PROPERTY_MAX);

	// Make a run of triangles for each material.
	for (const KeyValue<uint32_t, Vector<CSGBrush::Face>>& E : faces_by_material) {
		const uint32_t material_id = E.key;
		const Vector<CSGBrush::Face>& faces = E.value;
		mesh.runIndex.push_back(mesh.triVerts.size());

		// Associate the material with an ID.
		uint32_t reserved_id = r_manifold.ReserveIDs(1);
		mesh.runOriginalID.push_back(reserved_id);
		Ref<Material> material;
		if (material_id < p_mesh_merge->materials.size()) {
			material = p_mesh_merge->materials[material_id];
		}

		p_mesh_materials.insert(reserved_id, material);
		for (const CSGBrush::Face& face : faces) {
			for (int32_t tri_order_i = 0; tri_order_i < 3; tri_order_i++) {
				constexpr int32_t order[3] = {0, 2, 1};
				int i = order[tri_order_i];

				mesh.triVerts.push_back(mesh.vertProperties.size() / MANIFOLD_PROPERTY_MAX);

				size_t begin = mesh.vertProperties.size();
				mesh.vertProperties.resize(mesh.vertProperties.size() + MANIFOLD_PROPERTY_MAX);
				// Add the vertex properties.
				// Use CSGBrush constants rather than push_back for clarity.
				double* vert = &mesh.vertProperties[begin];
				vert[MANIFOLD_PROPERTY_POSITION_X] = face.vertices[i].x;
				vert[MANIFOLD_PROPERTY_POSITION_Y] = face.vertices[i].y;
				vert[MANIFOLD_PROPERTY_POSITION_Z] = face.vertices[i].z;
				vert[MANIFOLD_PROPERTY_UV_X_0] = face.uvs[i].x;
				vert[MANIFOLD_PROPERTY_UV_Y_0] = face.uvs[i].y;
				vert[MANIFOLD_PROPERTY_SMOOTH_GROUP] = face.smooth ? 1.0f : 0.0f;
				vert[MANIFOLD_PROPERTY_INVERT] = face.invert ? 1.0f : 0.0f;
			}
		}
	}
	// runIndex needs an explicit end value.
	mesh.runIndex.push_back(mesh.triVerts.size());
	mesh.tolerance = 2 * FLT_EPSILON;
	ERR_FAIL_COND_MSG(
		mesh.vertProperties.size() % mesh.numProp != 0, "Invalid vertex properties size.");
	mesh.Merge();
#ifdef DEV_ENABLED
	print_verbose(_export_meshgl_as_json(mesh));
#endif // DEV_ENABLED
	r_manifold = manifold::Manifold(mesh);
}

struct ManifoldOperation
{
	manifold::Manifold manifold;
	manifold::OpType operation;

	static manifold::OpType convert_csg_op(CSGShape3D::Operation op)
	{
		switch (op) {
		case CSGShape3D::OPERATION_SUBTRACTION:
			return manifold::OpType::Subtract;
		case CSGShape3D::OPERATION_INTERSECTION:
			return manifold::OpType::Intersect;
		default:
			return manifold::OpType::Add;
		}
	}

	ManifoldOperation() : operation(manifold::OpType::Add) {}

	ManifoldOperation(const manifold::Manifold& m, manifold::OpType op) : manifold(m), operation(op)
	{
	}
};

static void _generate_tangents_unindexed(float* p_tangents, size_t p_count,
	const Vector3* p_positions, const Vector3* p_normals, const Vector2* p_uvs)
{
	ERR_FAIL_COND_MSG(
		!SurfaceTool::generate_tangents_func, "Meshoptimizer library is not initialized.");
	ERR_FAIL_COND(p_count % 3 != 0);

	if (p_count == 0) {
		return;
	}

	struct TangentVertex
	{
		float position[3];
		float normal[3];
		float uv[2];
	};

	// We can't operate on input arrays directly because in double-precision builds, vectors use
	// double components So we convert the inputs to single precision floats before generating
	// tangents.
	LocalVector<TangentVertex> tangent_vertices;
	tangent_vertices.resize(p_count);

	for (size_t i = 0; i < p_count; i++) {
		TangentVertex& tangent_vertex = tangent_vertices[i];

		tangent_vertex.position[0] = p_positions[i].x;
		tangent_vertex.position[1] = p_positions[i].y;
		tangent_vertex.position[2] = p_positions[i].z;
		tangent_vertex.normal[0] = p_normals[i].x;
		tangent_vertex.normal[1] = p_normals[i].y;
		tangent_vertex.normal[2] = p_normals[i].z;
		tangent_vertex.uv[0] = p_uvs[i].x;
		tangent_vertex.uv[1] = p_uvs[i].y;
	}

	SurfaceTool::generate_tangents_func(p_tangents, nullptr, p_count,
		tangent_vertices.ptr()->position, p_count, sizeof(TangentVertex),
		tangent_vertices.ptr()->normal, sizeof(TangentVertex), tangent_vertices.ptr()->uv,
		sizeof(TangentVertex), 0);
}

void CSGShape3D::_build_surfaces_smoothed(CSGBrush* p_brush,
	Vector<CSGShape3D::ShapeUpdateSurface>& r_surfaces, Vector<int>& r_face_count)
{
	Vector<Vector3> smooth_faces;
	LocalVector<Vector3> smooth_vertex;
	smooth_faces.resize(p_brush->faces.size());
	smooth_vertex.resize(p_brush->faces.size() * 3);

	Vector3* smooth_faces_ptrw = smooth_faces.ptrw();
	int* face_count_ptrw = r_face_count.ptrw();

	for (int i = 0; i < p_brush->faces.size(); i++) {
		int mat = p_brush->faces[i].material;
		ERR_CONTINUE(mat < -1 || mat >= r_face_count.size());
		int idx = mat == -1 ? r_face_count.size() - 1 : mat;

		Plane p(p_brush->faces[i].vertices[0], p_brush->faces[i].vertices[1],
			p_brush->faces[i].vertices[2]);

		smooth_faces_ptrw[i] = p.normal;
		// Not sure if resize populates the LocalVector.
		smooth_vertex[i * 3 + 0] = Vector3(p.normal);
		smooth_vertex[i * 3 + 1] = Vector3(p.normal);
		smooth_vertex[i * 3 + 2] = Vector3(p.normal);
		// We could use a AHashMap Vector3, int to store the number of connections of each vertex
		// position and end the loop earlier. But I'm not sure if the performance gains outweigh the
		// cost.
		face_count_ptrw[idx]++;
	}

	const Vector3* smooth_faces_ptr = smooth_faces.ptr();
	const int smooth_faces_size = smooth_faces.size();

	// We could add a `use_groups` property later to only apply autosmooth on smooth faces or
	// respect smoothing groups in some way.
	if (smoothing_angle > 0.1) {
		float smooth_angle_rad = Math::cos(Math::deg_to_rad(smoothing_angle));
		for (int i = 0; i < smooth_faces_size; i++) {
			for (int k = 0; k < 3; k++) {
				int curr_vert = i * 3 + k;
				// Skip the other vertices of the face as they will never occupy the same position.
				Vector3 vert_a = p_brush->faces[i].vertices[k];
				for (int j = i + 1; j < smooth_faces_size; j++) {
					// Compare the angles of faces instead of vertices.
					if (smooth_faces_ptr[i].dot(smooth_faces_ptr[j]) > smooth_angle_rad) {
						for (int h = 0; h < 3; h++) {
							Vector3 vert_b = p_brush->faces[j].vertices[h];
							if (vert_a == vert_b) {
								int curr_j = j * 3 + h;
								smooth_vertex[curr_vert] += smooth_faces_ptr[j];
								smooth_vertex[curr_j] += smooth_faces_ptr[i];
								// Skip the other 2 vertices as only one vertex of each face can
								// connect with one vertex of other face.
								break;
							}
						}
					}
				}
				smooth_vertex[curr_vert].normalize();
			}
		}
	}

	// create arrays
	for (int i = 0; i < r_surfaces.size(); i++) {
		r_surfaces.write[i].vertices.resize(r_face_count[i] * 3);
		r_surfaces.write[i].normals.resize(r_face_count[i] * 3);
		r_surfaces.write[i].uvs.resize(r_face_count[i] * 3);
		if (calculate_tangents) {
			r_surfaces.write[i].tans.resize(r_face_count[i] * 3 * 4);
		}
		r_surfaces.write[i].last_added = 0;

		if (i != r_surfaces.size() - 1) {
			r_surfaces.write[i].material = p_brush->materials[i];
		}

		r_surfaces.write[i].verticesw = r_surfaces.write[i].vertices.ptrw();
		r_surfaces.write[i].normalsw = r_surfaces.write[i].normals.ptrw();
		r_surfaces.write[i].uvsw = r_surfaces.write[i].uvs.ptrw();
		if (calculate_tangents) {
			r_surfaces.write[i].tansw = r_surfaces.write[i].tans.ptrw();
		}
	}

	// fill arrays
	{
		for (int i = 0; i < p_brush->faces.size(); i++) {
			int order[3] = {0, 1, 2};

			if (p_brush->faces[i].invert) {
				SWAP(order[1], order[2]);
			}

			int mat = p_brush->faces[i].material;
			ERR_CONTINUE(mat < -1 || mat >= r_face_count.size());
			int idx = mat == -1 ? r_face_count.size() - 1 : mat;

			int last = r_surfaces[idx].last_added;

			int face_pos_i = i * 3;

			for (int j = 0; j < 3; j++) {
				Vector3 v = p_brush->faces[i].vertices[j];

				Vector3 normal = smooth_vertex[face_pos_i + j];

				if (p_brush->faces[i].invert) {
					normal = -normal;
				}

				int k = last + order[j];
				r_surfaces[idx].verticesw[k] = v;
				r_surfaces[idx].uvsw[k] = p_brush->faces[i].uvs[j];
				r_surfaces[idx].normalsw[k] = normal;

				if (calculate_tangents) {
					// zero out our tangents for now
					k *= 4;
					r_surfaces[idx].tansw[k++] = 0.0;
					r_surfaces[idx].tansw[k++] = 0.0;
					r_surfaces[idx].tansw[k++] = 0.0;
					r_surfaces[idx].tansw[k++] = 0.0;
				}
			}

			r_surfaces.write[idx].last_added += 3;
		}
	}
}

Ref<ArrayMesh> CSGShape3D::bake_static_mesh()
{
	Ref<ArrayMesh> baked_mesh;
	if (is_root_shape() && root_mesh.is_valid()) {
		baked_mesh = root_mesh;
	}
	return baked_mesh;
}

#ifndef PHYSICS_3D_DISABLED
Vector<Vector3> CSGShape3D::_get_brush_collision_faces()
{
	Vector<Vector3> collision_faces;
	CSGBrush* n = _get_brush();
	ERR_FAIL_NULL_V_MSG(n, collision_faces, "Cannot get CSGBrush.");
	collision_faces.resize(n->faces.size() * 3);
	Vector3* collision_faces_ptrw = collision_faces.ptrw();

	for (int i = 0; i < n->faces.size(); i++) {
		int order[3] = {0, 1, 2};

		if (n->faces[i].invert) {
			SWAP(order[1], order[2]);
		}

		collision_faces_ptrw[i * 3 + 0] = n->faces[i].vertices[order[0]];
		collision_faces_ptrw[i * 3 + 1] = n->faces[i].vertices[order[1]];
		collision_faces_ptrw[i * 3 + 2] = n->faces[i].vertices[order[2]];
	}

	return collision_faces;
}

void CSGShape3D::_update_collision_faces()
{
	if (use_collision && is_root_shape() && root_collision_shape.is_valid()) {
		root_collision_shape->set_faces(_get_brush_collision_faces());

		if (_is_debug_collision_shape_visible()) {
			_update_debug_collision_shape();
		}
	}
}

Ref<Shape3D> CSGShape3D::bake_collision_shape()
{
	Ref<Shape3D> baked_collision_shape;
	if (is_root_shape() && root_collision_shape.is_valid()) {
		baked_collision_shape = memnew(Shape3D(Shape3D::SHAPE_CONCAVE_POLYGON));
		baked_collision_shape->set_faces(root_collision_shape->get_faces());
	}
	else if (is_root_shape()) {
		baked_collision_shape = memnew(Shape3D(Shape3D::SHAPE_CONCAVE_POLYGON));
		baked_collision_shape->set_faces(_get_brush_collision_faces());
	}
	return baked_collision_shape;
}

bool CSGShape3D::_is_debug_collision_shape_visible()
{
	return !Engine::is_editor_hint() && is_inside_tree() &&
		   SceneTree::is_debugging_collisions_hint();
}

void CSGShape3D::_update_debug_collision_shape()
{
	if (!use_collision || !is_root_shape() || root_collision_shape.is_null() ||
		!_is_debug_collision_shape_visible()) {
		return;
	}

	ERR_FAIL_NULL(Renderer::data);

	if (root_collision_debug_instance.is_null()) {
		root_collision_debug_instance = Renderer::instance_create();
	}

	Ref<Mesh> debug_mesh = root_collision_shape->get_debug_mesh();
	Renderer::instance_set_scenario(
		root_collision_debug_instance, get_world_3d()->get_scenario());
	Renderer::instance_set_base(root_collision_debug_instance, debug_mesh->get_rid());
	Renderer::instance_set_transform(
		root_collision_debug_instance, get_global_transform());
}

void CSGShape3D::_clear_debug_collision_shape()
{
	if (root_collision_debug_instance.is_valid()) {
		Renderer::free_rid(root_collision_debug_instance);
		root_collision_debug_instance = RID();
	}
}

void CSGShape3D::_on_transform_changed()
{
	if (root_collision_debug_instance.is_valid() &&
		!debug_shape_old_transform.is_equal_approx(get_global_transform())) {
		debug_shape_old_transform = get_global_transform();
		Renderer::instance_set_transform(
			root_collision_debug_instance, debug_shape_old_transform);
	}
}
#endif // PHYSICS_3D_DISABLED

AABB CSGShape3D::get_aabb() const { return node_aabb; }

Vector<Vector3> CSGShape3D::get_brush_faces()
{
	ERR_FAIL_COND_V(!is_inside_tree(), Vector<Vector3>());
	CSGBrush* b = _get_brush();
	if (!b) {
		return Vector<Vector3>();
	}

	Vector<Vector3> faces;
	int fc = b->faces.size();
	faces.resize(fc * 3);
	{
		Vector3* w = faces.ptrw();
		for (int i = 0; i < fc; i++) {
			w[i * 3 + 0] = b->faces[i].vertices[0];
			w[i * 3 + 1] = b->faces[i].vertices[1];
			w[i * 3 + 2] = b->faces[i].vertices[2];
		}
	}

	return faces;
}

void CSGShape3D::set_shape_type(ShapeType p_type)
{
	if (shape_type == p_type) {
		return;
	}
	shape_type = p_type;
	_make_dirty();
}

void CSGShape3D::set_operation(Operation p_operation)
{
	if (operation == p_operation) {
		return;
	}
	operation = p_operation;
	_make_dirty();
}

CSGShape3D::Operation CSGShape3D::get_operation() const { return operation; }

void CSGShape3D::set_calculate_tangents(bool p_calculate_tangents)
{
	calculate_tangents = p_calculate_tangents;
	_make_dirty();
}

bool CSGShape3D::is_calculating_tangents() const { return calculate_tangents; }

PackedStringArray CSGShape3D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node::get_configuration_warnings();
	const CSGShape3D* current_shape = this;
	while (current_shape) {
		if (!current_shape->brush || current_shape->brush->faces.is_empty()) {
			warnings.push_back(RTR(
				"The CSGShape3D has an empty shape.\nCSGShape3D empty shapes typically occur "
				"because the mesh is not manifold.\nA manifold mesh forms a solid object without "
				"gaps, holes, or loose edges.\nEach edge must be a member of exactly two faces."));
			break;
		}
		current_shape = current_shape->parent_shape;
	}
	return warnings;
}

Ref<TriangleMesh> CSGShape3D::generate_triangle_mesh() const
{
	if (root_mesh.is_valid()) {
		return root_mesh->generate_triangle_mesh();
	}
	return Ref<TriangleMesh>();
}

void CSGShape3D::_notification(int p_what)
{
	render_instance.notification(this, p_what);
}

void CSGShape3D::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

CSGShape3D::CSGShape3D(ShapeType p_type) : shape_type(p_type)
{
	set_notify_local_transform(true);
	if (shape_type == SHAPE_POLYGON) {
		smooth_faces = false;
	}
	polygon.push_back(Vector2(0, 0));
	polygon.push_back(Vector2(0, 1));
	polygon.push_back(Vector2(1, 1));
	polygon.push_back(Vector2(1, 0));
}

CSGShape3D::~CSGShape3D()
{
	if (brush) {
		memdelete(brush);
		brush = nullptr;
	}
}

CSGBrush* CSGShape3D::_build_brush()
{
	switch (shape_type) {
	case SHAPE_COMBINER:
		return memnew(CSGBrush);
	case SHAPE_BOX:
		return _build_box_brush();
	case SHAPE_SPHERE:
		return _build_sphere_brush();
	case SHAPE_CYLINDER:
		return _build_cylinder_brush();
	case SHAPE_TORUS:
		return _build_torus_brush();
	case SHAPE_MESH:
		return _build_mesh_brush();
	case SHAPE_POLYGON:
		return _build_polygon_brush();
	}
	return memnew(CSGBrush);
}

CSGBrush* CSGShape3D::_create_brush_from_arrays(const Vector<Vector3>& p_vertices,
	const Vector<Vector2>& p_uv, const Vector<bool>& p_smooth,
	const Vector<Ref<Material>>& p_materials)
{
	CSGBrush* new_brush = memnew(CSGBrush);

	Vector<bool> invert;
	invert.resize(p_vertices.size() / 3);
	{
		int ic = invert.size();
		bool* w = invert.ptrw();
		for (int i = 0; i < ic; i++) {
			w[i] = flip_faces;
		}
	}
	new_brush->build_from_faces(p_vertices, p_uv, p_smooth, p_materials, invert);

	return new_brush;
}

void CSGShape3D::set_flip_faces(bool p_invert)
{
	if (flip_faces == p_invert) {
		return;
	}
	flip_faces = p_invert;
	_make_dirty();
}

bool CSGShape3D::get_flip_faces() const { return flip_faces; }

void CSGShape3D::set_smooth_faces(bool p_smooth_faces)
{
	if (smooth_faces == p_smooth_faces) {
		return;
	}
	smooth_faces = p_smooth_faces;
	_make_dirty();
}

bool CSGShape3D::get_smooth_faces() const { return smooth_faces; }

void CSGShape3D::set_material(const Ref<Material>& p_material)
{
	if (material == p_material) {
		return;
	}
	material = p_material;
	_make_dirty();
}

Ref<Material> CSGShape3D::get_material() const { return material; }

void CSGShape3D::set_mesh(const Ref<Mesh>& p_mesh)
{
	if (mesh == p_mesh) {
		return;
	}
	mesh = p_mesh;
	_make_dirty();
}

CSGBrush* CSGShape3D::_build_mesh_brush()
{
	if (mesh.is_null()) {
		return memnew(CSGBrush);
	}
	Vector<Face3> faces = mesh->get_faces();
	int face_count = faces.size();
	if (face_count == 0) {
		return memnew(CSGBrush);
	}
	Vector<Vector3> vertices;
	vertices.resize(face_count * 3);
	for (int i = 0; i < face_count; i++) {
		vertices.write[i * 3 + 0] = faces[i].vertex[0];
		vertices.write[i * 3 + 1] = faces[i].vertex[1];
		vertices.write[i * 3 + 2] = faces[i].vertex[2];
	}
	Vector<Vector2> uvs;
	uvs.resize(vertices.size());
	Vector<bool> smooth;
	smooth.resize(face_count);
	Vector<Ref<Material>> materials;
	materials.resize(face_count);
	for (int i = 0; i < face_count; i++) {
		smooth.write[i] = false;
		materials.write[i] = material;
	}
	return _create_brush_from_arrays(vertices, uvs, smooth, materials);
}

CSGBrush* CSGShape3D::_build_sphere_brush()
{
	// set our bounding box

	CSGBrush* new_brush = memnew(CSGBrush);

	int face_count = rings * radial_segments * 2 - radial_segments * 2;

	bool invert_val = get_flip_faces();
	Ref<Material> base_material = get_material();

	Vector<Vector3> faces;
	Vector<Vector2> uvs;
	Vector<bool> smooth;
	Vector<Ref<Material>> materials;
	Vector<bool> invert;

	faces.resize(face_count * 3);
	uvs.resize(face_count * 3);

	smooth.resize(face_count);
	materials.resize(face_count);
	invert.resize(face_count);

	{
		Vector3* facesw = faces.ptrw();
		Vector2* uvsw = uvs.ptrw();
		bool* smoothw = smooth.ptrw();
		Ref<Material>* materialsw = materials.ptrw();
		bool* invertw = invert.ptrw();

		// We want to follow an order that's convenient for UVs.
		// For latitude step we start at the top and move down like in an image.
		const double latitude_step = -Math::PI / rings;
		const double longitude_step = Math::TAU / radial_segments;
		int face = 0;
		for (int i = 0; i < rings; i++) {
			double cos0 = 0;
			double sin0 = 1;
			if (i > 0) {
				double latitude0 = latitude_step * i + Math::TAU / 4;
				cos0 = Math::cos(latitude0);
				sin0 = Math::sin(latitude0);
			}
			double v0 = double(i) / rings;

			double cos1 = 0;
			double sin1 = -1;
			if (i < rings - 1) {
				double latitude1 = latitude_step * (i + 1) + Math::TAU / 4;
				cos1 = Math::cos(latitude1);
				sin1 = Math::sin(latitude1);
			}
			double v1 = double(i + 1) / rings;

			for (int j = 0; j < radial_segments; j++) {
				double longitude0 = longitude_step * j;
				// We give sin to X and cos to Z on purpose.
				// This allows UVs to be CCW on +X so it maps to images well.
				double x0 = Math::sin(longitude0);
				double z0 = Math::cos(longitude0);
				double u0 = double(j) / radial_segments;

				double longitude1 = longitude_step * (j + 1);
				if (j == radial_segments - 1) {
					longitude1 = 0;
				}

				double x1 = Math::sin(longitude1);
				double z1 = Math::cos(longitude1);
				double u1 = double(j + 1) / radial_segments;

				Vector3 v[4] = {
					Vector3(x0 * cos0, sin0, z0 * cos0) * radius,
					Vector3(x1 * cos0, sin0, z1 * cos0) * radius,
					Vector3(x1 * cos1, sin1, z1 * cos1) * radius,
					Vector3(x0 * cos1, sin1, z0 * cos1) * radius,
				};

				Vector2 u[4] = {
					Vector2(u0, v0),
					Vector2(u1, v0),
					Vector2(u1, v1),
					Vector2(u0, v1),
				};

				// Draw the first face, but skip this at the north pole (i == 0).
				if (i > 0) {
					facesw[face * 3 + 0] = v[0];
					facesw[face * 3 + 1] = v[1];
					facesw[face * 3 + 2] = v[2];

					uvsw[face * 3 + 0] = u[0];
					uvsw[face * 3 + 1] = u[1];
					uvsw[face * 3 + 2] = u[2];

					smoothw[face] = smooth_faces;
					invertw[face] = invert_val;
					materialsw[face] = base_material;

					face++;
				}

				// Draw the second face, but skip this at the south pole (i == rings - 1).
				if (i < rings - 1) {
					facesw[face * 3 + 0] = v[2];
					facesw[face * 3 + 1] = v[3];
					facesw[face * 3 + 2] = v[0];

					uvsw[face * 3 + 0] = u[2];
					uvsw[face * 3 + 1] = u[3];
					uvsw[face * 3 + 2] = u[0];

					smoothw[face] = smooth_faces;
					invertw[face] = invert_val;
					materialsw[face] = base_material;

					face++;
				}
			}
		}

		if (face != face_count) {
			ERR_PRINT("Face mismatch bug! fix code");
		}
	}

	new_brush->build_from_faces(faces, uvs, smooth, materials, invert);

	return new_brush;
}

CSGBrush* CSGShape3D::_build_box_brush()
{
	// set our bounding box

	CSGBrush* new_brush = memnew(CSGBrush);

	int face_count = 12; // it's a cube..

	bool invert_val = get_flip_faces();
	Ref<Material> base_material = get_material();

	Vector<Vector3> faces;
	Vector<Vector2> uvs;
	Vector<bool> smooth;
	Vector<Ref<Material>> materials;
	Vector<bool> invert;

	faces.resize(face_count * 3);
	uvs.resize(face_count * 3);

	smooth.resize(face_count);
	materials.resize(face_count);
	invert.resize(face_count);

	{
		Vector3* facesw = faces.ptrw();
		Vector2* uvsw = uvs.ptrw();
		bool* smoothw = smooth.ptrw();
		Ref<Material>* materialsw = materials.ptrw();
		bool* invertw = invert.ptrw();

		int face = 0;

		Vector3 vertex_mul = size / 2;

		{
			for (int i = 0; i < 6; i++) {
				Vector3 face_points[4];
				float uv_points[8] = {0, 0, 0, 1, 1, 1, 1, 0};

				for (int j = 0; j < 4; j++) {
					float v[3];
					v[0] = 1.0;
					v[1] = 1 - 2 * ((j >> 1) & 1);
					v[2] = v[1] * (1 - 2 * (j & 1));

					for (int k = 0; k < 3; k++) {
						if (i < 3) {
							face_points[j][(i + k) % 3] = v[k];
						}
						else {
							face_points[3 - j][(i + k) % 3] = -v[k];
						}
					}
				}

				Vector2 u[4];
				for (int j = 0; j < 4; j++) {
					u[j] = Vector2(uv_points[j * 2 + 0], uv_points[j * 2 + 1]);
				}

				// face 1
				facesw[face * 3 + 0] = face_points[0] * vertex_mul;
				facesw[face * 3 + 1] = face_points[1] * vertex_mul;
				facesw[face * 3 + 2] = face_points[2] * vertex_mul;

				uvsw[face * 3 + 0] = u[0];
				uvsw[face * 3 + 1] = u[1];
				uvsw[face * 3 + 2] = u[2];

				smoothw[face] = false;
				invertw[face] = invert_val;
				materialsw[face] = base_material;

				face++;
				// face 2
				facesw[face * 3 + 0] = face_points[2] * vertex_mul;
				facesw[face * 3 + 1] = face_points[3] * vertex_mul;
				facesw[face * 3 + 2] = face_points[0] * vertex_mul;

				uvsw[face * 3 + 0] = u[2];
				uvsw[face * 3 + 1] = u[3];
				uvsw[face * 3 + 2] = u[0];

				smoothw[face] = false;
				invertw[face] = invert_val;
				materialsw[face] = base_material;

				face++;
			}
		}

		if (face != face_count) {
			ERR_PRINT("Face mismatch bug! fix code");
		}
	}

	new_brush->build_from_faces(faces, uvs, smooth, materials, invert);

	return new_brush;
}

CSGBrush* CSGShape3D::_build_cylinder_brush()
{
	// set our bounding box

	CSGBrush* new_brush = memnew(CSGBrush);

	int face_count = sides * (cone ? 1 : 2) + sides + (cone ? 0 : sides);

	bool invert_val = get_flip_faces();
	Ref<Material> base_material = get_material();

	Vector<Vector3> faces;
	Vector<Vector2> uvs;
	Vector<bool> smooth;
	Vector<Ref<Material>> materials;
	Vector<bool> invert;

	faces.resize(face_count * 3);
	uvs.resize(face_count * 3);

	smooth.resize(face_count);
	materials.resize(face_count);
	invert.resize(face_count);

	{
		Vector3* facesw = faces.ptrw();
		Vector2* uvsw = uvs.ptrw();
		bool* smoothw = smooth.ptrw();
		Ref<Material>* materialsw = materials.ptrw();
		bool* invertw = invert.ptrw();

		int face = 0;

		Vector3 vertex_mul(radius, height * 0.5, radius);

		{
			for (int i = 0; i < sides; i++) {
				float inc = float(i) / sides;
				float inc_n = float((i + 1)) / sides;
				if (i == sides - 1) {
					inc_n = 0;
				}

				float ang = inc * Math::TAU;
				float ang_n = inc_n * Math::TAU;

				Vector3 face_base(Math::cos(ang), 0, Math::sin(ang));
				Vector3 face_base_n(Math::cos(ang_n), 0, Math::sin(ang_n));

				Vector3 face_points[4] = {
					face_base + Vector3(0, -1, 0),
					face_base_n + Vector3(0, -1, 0),
					face_base_n * (cone ? 0.0 : 1.0) + Vector3(0, 1, 0),
					face_base * (cone ? 0.0 : 1.0) + Vector3(0, 1, 0),
				};

				Vector2 u[4] = {
					Vector2(inc, 0),
					Vector2(inc_n, 0),
					Vector2(inc_n, 1),
					Vector2(inc, 1),
				};

				// side face 1
				facesw[face * 3 + 0] = face_points[0] * vertex_mul;
				facesw[face * 3 + 1] = face_points[1] * vertex_mul;
				facesw[face * 3 + 2] = face_points[2] * vertex_mul;

				uvsw[face * 3 + 0] = u[0];
				uvsw[face * 3 + 1] = u[1];
				uvsw[face * 3 + 2] = u[2];

				smoothw[face] = smooth_faces;
				invertw[face] = invert_val;
				materialsw[face] = base_material;

				face++;

				if (!cone) {
					// side face 2
					facesw[face * 3 + 0] = face_points[2] * vertex_mul;
					facesw[face * 3 + 1] = face_points[3] * vertex_mul;
					facesw[face * 3 + 2] = face_points[0] * vertex_mul;

					uvsw[face * 3 + 0] = u[2];
					uvsw[face * 3 + 1] = u[3];
					uvsw[face * 3 + 2] = u[0];

					smoothw[face] = smooth_faces;
					invertw[face] = invert_val;
					materialsw[face] = base_material;
					face++;
				}

				// bottom face 1
				facesw[face * 3 + 0] = face_points[1] * vertex_mul;
				facesw[face * 3 + 1] = face_points[0] * vertex_mul;
				facesw[face * 3 + 2] = Vector3(0, -1, 0) * vertex_mul;

				uvsw[face * 3 + 0] =
					Vector2(face_points[1].x, face_points[1].y) * 0.5 + Vector2(0.5, 0.5);
				uvsw[face * 3 + 1] =
					Vector2(face_points[0].x, face_points[0].y) * 0.5 + Vector2(0.5, 0.5);
				uvsw[face * 3 + 2] = Vector2(0.5, 0.5);

				smoothw[face] = false;
				invertw[face] = invert_val;
				materialsw[face] = base_material;
				face++;

				if (!cone) {
					// top face 1
					facesw[face * 3 + 0] = face_points[3] * vertex_mul;
					facesw[face * 3 + 1] = face_points[2] * vertex_mul;
					facesw[face * 3 + 2] = Vector3(0, 1, 0) * vertex_mul;

					uvsw[face * 3 + 0] =
						Vector2(face_points[1].x, face_points[1].y) * 0.5 + Vector2(0.5, 0.5);
					uvsw[face * 3 + 1] =
						Vector2(face_points[0].x, face_points[0].y) * 0.5 + Vector2(0.5, 0.5);
					uvsw[face * 3 + 2] = Vector2(0.5, 0.5);

					smoothw[face] = false;
					invertw[face] = invert_val;
					materialsw[face] = base_material;
					face++;
				}
			}
		}

		if (face != face_count) {
			ERR_PRINT("Face mismatch bug! fix code");
		}
	}

	new_brush->build_from_faces(faces, uvs, smooth, materials, invert);

	return new_brush;
}

CSGBrush* CSGShape3D::_build_torus_brush()
{
	// set our bounding box
	float min_radius = inner_radius;
	float max_radius = outer_radius;

	if (min_radius == max_radius) {
		return memnew(CSGBrush); // sorry, can't
	}

	if (min_radius > max_radius) {
		SWAP(min_radius, max_radius);
	}

	float radius = (max_radius - min_radius) * 0.5;

	CSGBrush* new_brush = memnew(CSGBrush);

	int face_count = ring_sides * sides * 2;

	bool invert_val = get_flip_faces();
	Ref<Material> base_material = get_material();

	Vector<Vector3> faces;
	Vector<Vector2> uvs;
	Vector<bool> smooth;
	Vector<Ref<Material>> materials;
	Vector<bool> invert;

	faces.resize(face_count * 3);
	uvs.resize(face_count * 3);

	smooth.resize(face_count);
	materials.resize(face_count);
	invert.resize(face_count);

	{
		Vector3* facesw = faces.ptrw();
		Vector2* uvsw = uvs.ptrw();
		bool* smoothw = smooth.ptrw();
		Ref<Material>* materialsw = materials.ptrw();
		bool* invertw = invert.ptrw();

		int face = 0;

		{
			for (int i = 0; i < sides; i++) {
				float inci = float(i) / sides;
				float inci_n = float((i + 1)) / sides;
				if (i == sides - 1) {
					inci_n = 0;
				}

				float angi = inci * Math::TAU;
				float angi_n = inci_n * Math::TAU;

				Vector3 normali = Vector3(Math::cos(angi), 0, Math::sin(angi));
				Vector3 normali_n = Vector3(Math::cos(angi_n), 0, Math::sin(angi_n));

				for (int j = 0; j < ring_sides; j++) {
					float incj = float(j) / ring_sides;
					float incj_n = float((j + 1)) / ring_sides;
					if (j == ring_sides - 1) {
						incj_n = 0;
					}

					float angj = incj * Math::TAU;
					float angj_n = incj_n * Math::TAU;

					Vector2 normalj = Vector2(Math::cos(angj), Math::sin(angj)) * radius +
									  Vector2(min_radius + radius, 0);
					Vector2 normalj_n = Vector2(Math::cos(angj_n), Math::sin(angj_n)) * radius +
										Vector2(min_radius + radius, 0);

					Vector3 face_points[4] = {
						Vector3(normali.x * normalj.x, normalj.y, normali.z * normalj.x),
						Vector3(normali.x * normalj_n.x, normalj_n.y, normali.z * normalj_n.x),
						Vector3(normali_n.x * normalj_n.x, normalj_n.y, normali_n.z * normalj_n.x),
						Vector3(normali_n.x * normalj.x, normalj.y, normali_n.z * normalj.x)};

					Vector2 u[4] = {
						Vector2(inci, incj),
						Vector2(inci, incj_n),
						Vector2(inci_n, incj_n),
						Vector2(inci_n, incj),
					};

					// face 1
					facesw[face * 3 + 0] = face_points[0];
					facesw[face * 3 + 1] = face_points[2];
					facesw[face * 3 + 2] = face_points[1];

					uvsw[face * 3 + 0] = u[0];
					uvsw[face * 3 + 1] = u[2];
					uvsw[face * 3 + 2] = u[1];

					smoothw[face] = smooth_faces;
					invertw[face] = invert_val;
					materialsw[face] = base_material;

					face++;

					// face 2
					facesw[face * 3 + 0] = face_points[3];
					facesw[face * 3 + 1] = face_points[2];
					facesw[face * 3 + 2] = face_points[0];

					uvsw[face * 3 + 0] = u[3];
					uvsw[face * 3 + 1] = u[2];
					uvsw[face * 3 + 2] = u[0];

					smoothw[face] = smooth_faces;
					invertw[face] = invert_val;
					materialsw[face] = base_material;
					face++;
				}
			}
		}

		if (face != face_count) {
			ERR_PRINT("Face mismatch bug! fix code");
		}
	}

	new_brush->build_from_faces(faces, uvs, smooth, materials, invert);

	return new_brush;
}

void CSGShape3D::set_size(const Vector3& p_size)
{
	size = p_size;
	_make_dirty();
}

void CSGShape3D::set_radius(float p_radius)
{
	radius = p_radius;
	_make_dirty();
}

void CSGShape3D::set_height(float p_height)
{
	height = p_height;
	_make_dirty();
}

void CSGShape3D::set_inner_radius(float p_radius)
{
	inner_radius = p_radius;
	_make_dirty();
}

void CSGShape3D::set_outer_radius(float p_radius)
{
	outer_radius = p_radius;
	_make_dirty();
}

void CSGShape3D::set_radial_segments(int p_segments)
{
	radial_segments = MAX(4, p_segments);
	_make_dirty();
}

void CSGShape3D::set_rings(int p_rings)
{
	rings = MAX(1, p_rings);
	_make_dirty();
}

void CSGShape3D::set_sides(int p_sides)
{
	sides = MAX(3, p_sides);
	_make_dirty();
}

void CSGShape3D::set_ring_sides(int p_ring_sides)
{
	ring_sides = MAX(3, p_ring_sides);
	_make_dirty();
}

void CSGShape3D::set_cone(bool p_cone)
{
	cone = p_cone;
	_make_dirty();
}

void CSGShape3D::set_polygon(const Vector<Vector2>& p_polygon)
{
	polygon = p_polygon;
	_make_dirty();
}

void CSGShape3D::set_polygon_mode(PolygonMode p_mode)
{
	polygon_mode = p_mode;
	_make_dirty();
}

void CSGShape3D::set_depth(float p_depth)
{
	depth = p_depth;
	_make_dirty();
}

void CSGShape3D::set_spin_degrees(float p_degrees)
{
	spin_degrees = p_degrees;
	_make_dirty();
}

void CSGShape3D::set_spin_sides(int p_sides)
{
	spin_sides = p_sides;
	_make_dirty();
}

void CSGShape3D::set_path_node(const NodePath& p_path)
{
	path_node = p_path;
	_make_dirty();
}

void CSGShape3D::set_path_interval_type(PathIntervalType p_type)
{
	path_interval_type = p_type;
	_make_dirty();
}

void CSGShape3D::set_path_interval(float p_interval)
{
	path_interval = p_interval;
	_make_dirty();
}

void CSGShape3D::set_path_simplify_angle(float p_angle)
{
	path_simplify_angle = p_angle;
	_make_dirty();
}

void CSGShape3D::set_path_rotation(PathRotation p_rotation)
{
	path_rotation = p_rotation;
	_make_dirty();
}

void CSGShape3D::set_path_rotation_accurate(bool p_accurate)
{
	path_rotation_accurate = p_accurate;
	_make_dirty();
}

void CSGShape3D::set_path_local(bool p_local)
{
	path_local = p_local;
	_make_dirty();
}

void CSGShape3D::set_path_continuous_u(bool p_enable)
{
	path_continuous_u = p_enable;
	_make_dirty();
}

void CSGShape3D::set_path_u_distance(real_t p_distance)
{
	path_u_distance = p_distance;
	_make_dirty();
}

void CSGShape3D::set_path_joined(bool p_joined)
{
	path_joined = p_joined;
	_make_dirty();
}

void CSGShape3D::_path_exited() { path = nullptr; }

bool CSGShape3D::_is_editable_3d_polygon() const { return shape_type == SHAPE_POLYGON; }

bool CSGShape3D::_has_editable_3d_polygon_no_depth() const { return shape_type == SHAPE_POLYGON; }

void CSGShape3D::_make_dirty(bool p_parent_removing)
{
	(void)p_parent_removing;
	if (brush) {
		memdelete(brush);
		brush = nullptr;
	}
	dirty = true;
}

CSGBrush* CSGShape3D::_get_brush()
{
	if (!brush) {
		brush = _build_brush();
	}
	return brush;
}

void CSGShape3D::navmesh_parse_init() {}

CSGBrush* CSGShape3D::_build_polygon_brush()
{
	return memnew(CSGBrush);
}

// CSGBrush

void CSGBrush::build_from_faces(const Vector<Vector3> &p_vertices, const Vector<Vector2> &p_uvs, const Vector<bool> &p_smooth, const Vector<Ref<Material>> &p_materials, const Vector<bool> &p_flip_faces) {
	faces.clear();

	int vc = p_vertices.size();

	ERR_FAIL_COND((vc % 3) != 0);

	const Vector3 *rv = p_vertices.ptr();
	int uvc = p_uvs.size();
	const Vector2 *ruv = p_uvs.ptr();
	int sc = p_smooth.size();
	const bool *rs = p_smooth.ptr();
	int mc = p_materials.size();
	const Ref<Material> *rm = p_materials.ptr();
	int ic = p_flip_faces.size();
	const bool *ri = p_flip_faces.ptr();

	HashMap<Ref<Material>, int> material_map;

	faces.resize(p_vertices.size() / 3);

	for (int i = 0; i < faces.size(); i++) {
		Face &f = faces.write[i];
		f.vertices[0] = rv[i * 3 + 0];
		f.vertices[1] = rv[i * 3 + 1];
		f.vertices[2] = rv[i * 3 + 2];

		if (uvc == vc) {
			f.uvs[0] = ruv[i * 3 + 0];
			f.uvs[1] = ruv[i * 3 + 1];
			f.uvs[2] = ruv[i * 3 + 2];
		}

		if (sc == vc / 3) {
			f.smooth = rs[i];
		} else {
			f.smooth = false;
		}

		if (ic == vc / 3) {
			f.invert = ri[i];
		} else {
			f.invert = false;
		}

		if (mc == vc / 3) {
			Ref<Material> mat = rm[i];
			if (mat.is_valid()) {
				HashMap<Ref<Material>, int>::ConstIterator E = material_map.find(mat);

				if (E) {
					f.material = E->value;
				} else {
					f.material = material_map.size();
					material_map[mat] = f.material;
				}

			} else {
				f.material = -1;
			}
		}
	}

	materials.resize(material_map.size());
	for (const KeyValue<Ref<Material>, int> &E : material_map) {
		materials.write[E.value] = E.key;
	}

	_regen_face_aabbs();
}

void CSGBrush::copy_from(const CSGBrush &p_brush, const Transform3D &p_xform) {
	faces = p_brush.faces;
	materials = p_brush.materials;

	for (int i = 0; i < faces.size(); i++) {
		for (int j = 0; j < 3; j++) {
			faces.write[i].vertices[j] = p_xform.xform(p_brush.faces[i].vertices[j]);
		}
	}

	_regen_face_aabbs();
}

void initialize_csg_module(ModuleInitializationLevel p_level)
{
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
#ifndef NAVIGATION_3D_DISABLED
		CSGShape3D::navmesh_parse_init();
#endif // NAVIGATION_3D_DISABLED
	}
}

void uninitialize_csg_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}
#endif // defined(MODULE_CSG_ENABLED)


// --- Merged from modules/gridmap/grid_map.cpp ---
#if defined(MODULE_GRIDMAP_ENABLED)
#include "core/config/engine.h"
#include "main/main.h"
#include "core/io/resource_loader.h"
#include "core/templates/a_hash_map.h"
#include "modules/gltf/gltf_document.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/mesh.h"
#include "servers/rendering/renderer.h"

#ifndef PHYSICS_3D_DISABLED
#include "core/math/geometry_3d.h"
#include "scene/resources/mesh.h"
#include "scene/resources/material.h"
#endif // PHYSICS_3D_DISABLED

#ifndef NAVIGATION_3D_DISABLED
#include "servers/navigation_3d/navigation_server_3d.h"

RID GridMap::_navmesh_source_geometry_parser;
#endif // NAVIGATION_3D_DISABLED

#ifndef PHYSICS_3D_DISABLED
void GridMap::set_collision_layer(uint32_t p_layer)
{
	collision_layer = p_layer;
	_update_physics_bodies_collision_properties();
}

uint32_t GridMap::get_collision_layer() const { return collision_layer; }

void GridMap::set_collision_mask(uint32_t p_mask)
{
	collision_mask = p_mask;
	_update_physics_bodies_collision_properties();
}

uint32_t GridMap::get_collision_mask() const { return collision_mask; }

void GridMap::set_collision_layer_value(int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 32, "Collision layer number must be between 1 and 32 inclusive.");
	uint32_t collision_layer_new = get_collision_layer();
	if (p_value) {
		collision_layer_new |= 1 << (p_layer_number - 1);
	}
	else {
		collision_layer_new &= ~(1 << (p_layer_number - 1));
	}
	set_collision_layer(collision_layer_new);
}

bool GridMap::get_collision_layer_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Collision layer number must be between 1 and 32 inclusive.");
	return get_collision_layer() & (1 << (p_layer_number - 1));
}

void GridMap::set_collision_mask_value(int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 32, "Collision layer number must be between 1 and 32 inclusive.");
	uint32_t mask = get_collision_mask();
	if (p_value) {
		mask |= 1 << (p_layer_number - 1);
	}
	else {
		mask &= ~(1 << (p_layer_number - 1));
	}
	set_collision_mask(mask);
}

void GridMap::set_collision_priority(real_t p_priority)
{
	collision_priority = p_priority;
	_update_physics_bodies_collision_properties();
}

real_t GridMap::get_collision_priority() const { return collision_priority; }

void GridMap::set_collision_visibility_mode(DebugVisibilityMode p_visibility_mode)
{
	if (collision_visibility_mode == p_visibility_mode) {
		return;
	}
	collision_visibility_mode = p_visibility_mode;
	_recreate_octant_data();
}

GridMap::DebugVisibilityMode GridMap::get_collision_visibility_mode() const
{
	return collision_visibility_mode;
}

void GridMap::set_physics_material(Ref<PhysicsMaterial> p_material)
{
	physics_material = p_material;
	_update_physics_bodies_characteristics();
}

Ref<PhysicsMaterial> GridMap::get_physics_material() const { return physics_material; }

bool GridMap::get_collision_mask_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Collision layer number must be between 1 and 32 inclusive.");
	return get_collision_mask() & (1 << (p_layer_number - 1));
}

RID GridMap::get_physics_body_from_octant_coord(const Vector3i& p_octant_coords) const
{
	OctantKey octantkey;
	octantkey.x = p_octant_coords.x;
	octantkey.y = p_octant_coords.y;
	octantkey.z = p_octant_coords.z;

	const HashMap<OctantKey, Octant*, OctantKey>::ConstIterator octant_kv =
		octant_map.find(octantkey);

	if (!octant_kv) {
		return RID();
	}

	Octant* g = octant_kv->value;
	RID body_rid = g->static_body;

	return body_rid;
}
#endif // PHYSICS_3D_DISABLED

void GridMap::set_bake_navigation(bool p_bake_navigation)
{
	bake_navigation = p_bake_navigation;
	_recreate_octant_data();
}

bool GridMap::is_baking_navigation() { return bake_navigation; }

#ifndef NAVIGATION_3D_DISABLED
void GridMap::set_navigation_map(RID p_navigation_map)
{
	map_override = p_navigation_map;
	for (const KeyValue<OctantKey, Octant*>& E : octant_map) {
		Octant& g = *octant_map[E.key];
		for (KeyValue<IndexKey, Octant::NavigationCell>& F : g.navigation_cell_ids) {
			if (F.value.region.is_valid()) {
				NavigationServer3D::region_set_map(F.value.region, map_override);
			}
		}
	}
}

RID GridMap::get_navigation_map() const
{
	if (map_override.is_valid()) {
		return map_override;
	}
	else if (is_inside_tree()) {
		return get_world_3d()->get_navigation_map();
	}
	return RID();
}
#endif // NAVIGATION_3D_DISABLED

Ref<MeshLibrary> GridMap::get_mesh_library() const { return mesh_library; }

Vector3 GridMap::get_cell_size() const { return cell_size; }

void GridMap::set_octant_size(int p_size)
{
	ERR_FAIL_COND(p_size == 0);
	octant_size = p_size;
	_recreate_octant_data();
}

int GridMap::get_octant_size() const { return octant_size; }

void GridMap::set_center_x(bool p_enable)
{
	center_x = p_enable;
	_recreate_octant_data();
}

bool GridMap::get_center_x() const { return center_x; }

void GridMap::set_center_y(bool p_enable)
{
	center_y = p_enable;
	_recreate_octant_data();
}

bool GridMap::get_center_y() const { return center_y; }

void GridMap::set_center_z(bool p_enable)
{
	center_z = p_enable;
	_recreate_octant_data();
}

bool GridMap::get_center_z() const { return center_z; }

void GridMap::set_debug_octant_color(const Color& p_color)
{
#ifdef DEBUG_ENABLED
	if (debug_octant_color == p_color) {
		return;
	}

	debug_octant_color = p_color;
#endif
}

Color GridMap::get_debug_octant_color() const { return debug_octant_color; }

#ifdef DEBUG_ENABLED

void GridMap::_debug_clear_octants()
{
	for (const KeyValue<OctantKey, Octant*>& ele : octant_map) {
		OctantKey octant_key = ele.key;
		HashMap<OctantKey, OctantDebug*, OctantKey>::Iterator E = debug_octant_map.find(octant_key);
		if (E) {
			OctantDebug& octant_debug = *E->value;

			if (octant_debug.debug_line_mesh_rid.is_valid()) {
				Renderer::free_rid(octant_debug.debug_line_mesh_rid);
				octant_debug.debug_line_mesh_rid = RID();
			}
			if (octant_debug.debug_line_instance_rid.is_valid()) {
				Renderer::free_rid(octant_debug.debug_line_instance_rid);
				octant_debug.debug_line_instance_rid = RID();
			}

			memdelete(&octant_debug);
		}
	}

	debug_octant_map.clear();
}

#endif // DEBUG_ENABLED

int GridMap::get_cell_item(const Vector3i& p_position) const
{
	ERR_FAIL_INDEX_V(Math::abs(p_position.x), 1 << 20, INVALID_CELL_ITEM);
	ERR_FAIL_INDEX_V(Math::abs(p_position.y), 1 << 20, INVALID_CELL_ITEM);
	ERR_FAIL_INDEX_V(Math::abs(p_position.z), 1 << 20, INVALID_CELL_ITEM);

	IndexKey key;
	key.x = p_position.x;
	key.y = p_position.y;
	key.z = p_position.z;

	if (!cell_map.has(key)) {
		return INVALID_CELL_ITEM;
	}
	return cell_map[key].item;
}

int GridMap::get_cell_item_orientation(const Vector3i& p_position) const
{
	ERR_FAIL_INDEX_V(Math::abs(p_position.x), 1 << 20, -1);
	ERR_FAIL_INDEX_V(Math::abs(p_position.y), 1 << 20, -1);
	ERR_FAIL_INDEX_V(Math::abs(p_position.z), 1 << 20, -1);

	IndexKey key;
	key.x = p_position.x;
	key.y = p_position.y;
	key.z = p_position.z;

	if (!cell_map.has(key)) {
		return -1;
	}
	return cell_map[key].rot;
}

static const Basis _ortho_bases[24] = {Basis(1, 0, 0, 0, 1, 0, 0, 0, 1),
	Basis(0, -1, 0, 1, 0, 0, 0, 0, 1), Basis(-1, 0, 0, 0, -1, 0, 0, 0, 1),
	Basis(0, 1, 0, -1, 0, 0, 0, 0, 1), Basis(1, 0, 0, 0, 0, -1, 0, 1, 0),
	Basis(0, 0, 1, 1, 0, 0, 0, 1, 0), Basis(-1, 0, 0, 0, 0, 1, 0, 1, 0),
	Basis(0, 0, -1, -1, 0, 0, 0, 1, 0), Basis(1, 0, 0, 0, -1, 0, 0, 0, -1),
	Basis(0, 1, 0, 1, 0, 0, 0, 0, -1), Basis(-1, 0, 0, 0, 1, 0, 0, 0, -1),
	Basis(0, -1, 0, -1, 0, 0, 0, 0, -1), Basis(1, 0, 0, 0, 0, 1, 0, -1, 0),
	Basis(0, 0, -1, 1, 0, 0, 0, -1, 0), Basis(-1, 0, 0, 0, 0, -1, 0, -1, 0),
	Basis(0, 0, 1, -1, 0, 0, 0, -1, 0), Basis(0, 0, 1, 0, 1, 0, -1, 0, 0),
	Basis(0, -1, 0, 0, 0, 1, -1, 0, 0), Basis(0, 0, -1, 0, -1, 0, -1, 0, 0),
	Basis(0, 1, 0, 0, 0, -1, -1, 0, 0), Basis(0, 0, 1, 0, -1, 0, 1, 0, 0),
	Basis(0, 1, 0, 0, 0, 1, 1, 0, 0), Basis(0, 0, -1, 0, 1, 0, 1, 0, 0),
	Basis(0, -1, 0, 0, 0, -1, 1, 0, 0)};

Basis GridMap::get_cell_item_basis(const Vector3i& p_position) const
{
	int orientation = get_cell_item_orientation(p_position);

	if (orientation == -1) {
		return Basis();
	}

	return get_basis_with_orthogonal_index(orientation);
}

Basis GridMap::get_basis_with_orthogonal_index(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, 24, Basis());

	return _ortho_bases[p_index];
}

int GridMap::get_orthogonal_index_from_basis(const Basis& p_basis) const
{
	Basis orth = p_basis;
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			real_t v = orth[i][j];
			if (v > 0.5) {
				v = 1.0;
			}
			else if (v < -0.5) {
				v = -1.0;
			}
			else {
				v = 0;
			}

			orth[i][j] = v;
		}
	}

	for (int i = 0; i < 24; i++) {
		if (_ortho_bases[i] == orth) {
			return i;
		}
	}

	return 0;
}

GridMap::OctantKey GridMap::get_octant_key_from_index_key(const IndexKey& p_index_key) const
{
	const int x = p_index_key.x > 0 ? p_index_key.x / octant_size
									: (p_index_key.x - (octant_size - 1)) / octant_size;
	const int y = p_index_key.y > 0 ? p_index_key.y / octant_size
									: (p_index_key.y - (octant_size - 1)) / octant_size;
	const int z = p_index_key.z > 0 ? p_index_key.z / octant_size
									: (p_index_key.z - (octant_size - 1)) / octant_size;

	OctantKey ok;
	ok.key = 0;
	ok.x = x;
	ok.y = y;
	ok.z = z;
	return ok;
}

GridMap::OctantKey GridMap::get_octant_key_from_cell_coords(const Vector3i& p_cell_coords) const
{
	const int x = p_cell_coords.x > 0 ? p_cell_coords.x / octant_size
									  : (p_cell_coords.x - (octant_size - 1)) / octant_size;
	const int y = p_cell_coords.y > 0 ? p_cell_coords.y / octant_size
									  : (p_cell_coords.y - (octant_size - 1)) / octant_size;
	const int z = p_cell_coords.z > 0 ? p_cell_coords.z / octant_size
									  : (p_cell_coords.z - (octant_size - 1)) / octant_size;

	OctantKey ok;
	ok.key = 0;
	ok.x = x;
	ok.y = y;
	ok.z = z;
	return ok;
}

Vector3i GridMap::local_to_map(const Vector3& p_world_position) const
{
	Vector3 map_position = (p_world_position / cell_size).floor();
	return Vector3i(map_position);
}

Vector3 GridMap::map_to_local(const Vector3i& p_map_position) const
{
	Vector3 offset = _get_offset();
	Vector3 local_position(p_map_position.x * cell_size.x + offset.x,
		p_map_position.y * cell_size.y + offset.y, p_map_position.z * cell_size.z + offset.z);
	return local_position;
}

#ifndef PHYSICS_3D_DISABLED
void GridMap::_update_physics_bodies_collision_properties()
{
	for (const KeyValue<OctantKey, Octant*>& E : octant_map) {
		PhysicsServer3D::body_set_collision_layer(
			E.value->static_body, collision_layer);
		PhysicsServer3D::body_set_collision_mask(
			E.value->static_body, collision_mask);
		PhysicsServer3D::body_set_collision_priority(
			E.value->static_body, collision_priority);
	}
}

#endif // PHYSICS_3D_DISABLED

void GridMap::_octant_clean_up(const OctantKey& p_key)
{
	ERR_FAIL_NULL(Renderer::data);
#ifndef PHYSICS_3D_DISABLED
	ERR_FAIL_COND(!PhysicsServer3D::is_initialized());
#endif // PHYSICS_3D_DISABLED
#ifndef NAVIGATION_3D_DISABLED
	ERR_FAIL_NULL(NavigationServer3D::data);
#endif // NAVIGATION_3D_DISABLED

	ERR_FAIL_COND(!octant_map.has(p_key));
	Octant& g = *octant_map[p_key];

#ifndef PHYSICS_3D_DISABLED
	if (g.collision_debug.is_valid()) {
		Renderer::free_rid(g.collision_debug);
	}
	if (g.collision_debug_instance.is_valid()) {
		Renderer::free_rid(g.collision_debug_instance);
	}

	PhysicsServer3D::free_rid(g.static_body);
#endif // PHYSICS_3D_DISABLED

#ifndef NAVIGATION_3D_DISABLED
	// Erase navigation
	for (const KeyValue<IndexKey, Octant::NavigationCell>& E : g.navigation_cell_ids) {
		if (E.value.region.is_valid()) {
			NavigationServer3D::free_rid(E.value.region);
		}
		if (E.value.navigation_mesh_debug_instance.is_valid()) {
			Renderer::free_rid(E.value.navigation_mesh_debug_instance);
		}
	}
	g.navigation_cell_ids.clear();
#endif // NAVIGATION_3D_DISABLED

#ifdef DEBUG_ENABLED
	if (bake_navigation) {
		if (g.navigation_debug_edge_connections_instance.is_valid()) {
			Renderer::free_rid(
				g.navigation_debug_edge_connections_instance);
			g.navigation_debug_edge_connections_instance = RID();
		}
		if (g.navigation_debug_edge_connections_mesh.is_valid()) {
			g.navigation_debug_edge_connections_mesh.unref();
		}
	}

	_debug_clear_octants();
#endif // DEBUG_ENABLED

	// Erase multimeshes.

	for (int i = 0; i < g.multimesh_instances.size(); i++) {
		Renderer::free_rid(g.multimesh_instances[i].instance);
		Renderer::free_rid(g.multimesh_instances[i].multimesh);
	}
	g.multimesh_instances.clear();
}

void GridMap::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_WORLD: {
		last_transform = get_global_transform();

		for (const KeyValue<OctantKey, Octant*>& E : octant_map) {
			_octant_enter_world(E.key);
		}

		for (int i = 0; i < baked_meshes.size(); i++) {
			Renderer::instance_set_scenario(
				baked_meshes[i].instance, get_world_3d()->get_scenario());
			Renderer::instance_set_transform(
				baked_meshes[i].instance, get_global_transform());
		}
	} break;

	case NOTIFICATION_ENTER_TREE: {
#ifdef DEBUG_ENABLED
		_debug_update();

#ifndef NAVIGATION_3D_DISABLED
		if (bake_navigation &&
			NavigationServer3D::get_debug_navigation_enabled()) {
			_update_navigation_debug_edge_connections();
		}
#endif // NAVIGATION_3D_DISABLED

#endif // DEBUG_ENABLED

		_update_visibility();
	} break;

#ifdef DEBUG_ENABLED
	case NOTIFICATION_EXIT_TREE: {
		_debug_clear_octants();
	} break;
#endif

	case NOTIFICATION_TRANSFORM_CHANGED: {
		Transform3D new_xform = get_global_transform();
		if (new_xform == last_transform) {
			break;
		}

		for (const KeyValue<OctantKey, Octant*>& E : octant_map) {
			_octant_transform(E.key);
		}

		last_transform = new_xform;

		for (int i = 0; i < baked_meshes.size(); i++) {
			Renderer::instance_set_transform(
				baked_meshes[i].instance, get_global_transform());
		}

#ifdef DEBUG_ENABLED
		for (const KeyValue<OctantKey, OctantDebug*>& E : debug_octant_map) {
			OctantKey octant_key = E.key;
			OctantDebug& octant_debug = *E.value;
			if (octant_debug.debug_line_instance_rid.is_valid()) {
				const Transform3D octant_transform =
					new_xform *
					(Transform3D(Basis(), Vector3(octant_key.x, octant_key.y, octant_key.z) *
											  octant_size * cell_size));
				Renderer::instance_set_transform(
					octant_debug.debug_line_instance_rid, octant_transform);
			}
		}
#endif
	} break;

	case NOTIFICATION_EXIT_WORLD: {
		for (const KeyValue<OctantKey, Octant*>& E : octant_map) {
			_octant_exit_world(E.key);
		}

		for (int i = 0; i < baked_meshes.size(); i++) {
			Renderer::instance_set_scenario(baked_meshes[i].instance, RID());
		}
	} break;

	case NOTIFICATION_VISIBILITY_CHANGED: {
		_update_visibility();
	} break;
	}
}

void GridMap::_update_visibility()
{
	if (!is_inside_tree()) {
		return;
	}

	for (KeyValue<OctantKey, Octant*>& e : octant_map) {
		Octant* octant = e.value;
		for (int i = 0; i < octant->multimesh_instances.size(); i++) {
			const Octant::MultimeshInstance& mi = octant->multimesh_instances[i];
			Renderer::instance_set_visible(mi.instance, is_visible_in_tree());
		}
	}

	for (int i = 0; i < baked_meshes.size(); i++) {
		Renderer::instance_set_visible(baked_meshes[i].instance, is_visible_in_tree());
	}

#ifdef DEBUG_ENABLED
	for (const KeyValue<OctantKey, OctantDebug*>& E : debug_octant_map) {
		OctantDebug& octant_debug = *E.value;
		if (octant_debug.debug_line_instance_rid.is_valid()) {
			Renderer::instance_set_visible(
				octant_debug.debug_line_instance_rid, is_visible_in_tree());
		}
	}
#endif
}

void GridMap::_recreate_octant_data()
{
	recreating_octants = true;

	HashMap<IndexKey, Cell, IndexKey> cell_copy(cell_map);
	_clear_internal();
	for (const KeyValue<IndexKey, Cell>& E : cell_copy) {
		set_cell_item(Vector3i(E.key), E.value.item, E.value.rot);
	}

	recreating_octants = false;
#ifdef DEBUG_ENABLED
	_debug_update();
#endif
}

void GridMap::_clear_internal()
{
	for (const KeyValue<OctantKey, Octant*>& E : octant_map) {
		if (is_inside_world()) {
			_octant_exit_world(E.key);
		}

		_octant_clean_up(E.key);
		memdelete(E.value);
	}

	octant_map.clear();
	cell_map.clear();
#ifdef DEBUG_ENABLED
	_debug_clear_octants();
	_debug_update();
#endif
}

void GridMap::clear()
{
	_clear_internal();
	clear_baked_meshes();
}

#ifndef DISABLE_DEPRECATED
void GridMap::resource_changed(const Ref<Resource>& p_res) {}
#endif

void GridMap::_update_octants_callback()
{
	if (!awaiting_update) {
		return;
	}

	LocalVector<OctantKey> to_delete;
	to_delete.reserve(octant_map.size());
	for (const KeyValue<OctantKey, Octant*>& E : octant_map) {
		if (_octant_update(E.key)) {
			to_delete.push_back(E.key);
		}
	}

	while (!to_delete.is_empty()) {
		const OctantKey& octantkey = to_delete[0];
		memdelete(octant_map[octantkey]);
		octant_map.erase(octantkey);
		to_delete.remove_at_unordered(0);
	}

	_update_visibility();
	awaiting_update = false;

#ifdef DEBUG_ENABLED
	_debug_update_octants();
#endif
}

void GridMap::set_cell_scale(float p_scale)
{
	cell_scale = p_scale;
	_recreate_octant_data();
}

float GridMap::get_cell_scale() const { return cell_scale; }

Vector3i GridMap::get_octant_coords_from_cell_coords(const Vector3i& p_cell_coords) const
{
	return Vector3i(p_cell_coords.x > 0 ? p_cell_coords.x / octant_size
										: (p_cell_coords.x - (octant_size - 1)) / octant_size,
		p_cell_coords.y > 0 ? p_cell_coords.y / octant_size
							: (p_cell_coords.y - (octant_size - 1)) / octant_size,
		p_cell_coords.z > 0 ? p_cell_coords.z / octant_size
							: (p_cell_coords.z - (octant_size - 1)) / octant_size);
}

LocalVector<GridMap::IndexKey> GridMap::get_index_keys_in_bounds(
	const AABB& p_bounds, bool p_used_only) const
{
	LocalVector<IndexKey> index_keys;
	if (!p_bounds.has_volume()) {
		return index_keys;
	}

	Vector3i cell_coords_start = (p_bounds.position / cell_size).floor();
	// -CMP_EPSILON because we don't want the octants that are just starting at the edge of the
	// bounds.
	Vector3i cell_coords_end =
		((p_bounds.get_end() - Vector3(CMP_EPSILON, CMP_EPSILON, CMP_EPSILON)) / cell_size).floor();

	for (int z = cell_coords_start.z; z < cell_coords_end.z + 1; z++) {
		for (int y = cell_coords_start.y; y < cell_coords_end.y + 1; y++) {
			for (int x = cell_coords_start.x; x < cell_coords_end.x + 1; x++) {
				IndexKey index_key;
				index_key.x = x;
				index_key.y = y;
				index_key.z = z;

				if (p_used_only) {
					const HashMap<IndexKey, Cell, IndexKey>::ConstIterator index_key_kv =
						cell_map.find(index_key);

					if (index_key_kv) {
						index_keys.push_back(index_key);
					}
				}
				else {
					index_keys.push_back(index_key);
				}
			}
		}
	}

	return index_keys;
}

LocalVector<GridMap::OctantKey> GridMap::get_octant_keys_in_bounds(
	const AABB& p_bounds, bool p_used_only) const
{
	LocalVector<OctantKey> octant_keys;
	if (!p_bounds.has_volume()) {
		return octant_keys;
	}

	Vector3i cell_coords_start = (p_bounds.position / cell_size).floor();
	// -CMP_EPSILON because we don't want the octants that are just starting at the edge of the
	// bounds.
	Vector3i cell_coords_end =
		((p_bounds.get_end() - Vector3(CMP_EPSILON, CMP_EPSILON, CMP_EPSILON)) / cell_size).floor();

	OctantKey octant_coords_start = get_octant_key_from_cell_coords(cell_coords_start);
	OctantKey octant_coords_end = get_octant_key_from_cell_coords(cell_coords_end);

	for (int z = octant_coords_start.z; z < octant_coords_end.z + 1; z++) {
		for (int y = octant_coords_start.y; y < octant_coords_end.y + 1; y++) {
			for (int x = octant_coords_start.x; x < octant_coords_end.x + 1; x++) {
				OctantKey octant_key;
				octant_key.x = x;
				octant_key.y = y;
				octant_key.z = z;

				if (p_used_only) {
					const HashMap<OctantKey, Octant*, OctantKey>::ConstIterator octant_kv =
						octant_map.find(octant_key);

					if (octant_kv) {
						Vector3i octant_coord(x, y, z);
						octant_keys.push_back(octant_key);
					}
				}
				else {
					octant_keys.push_back(octant_key);
				}
			}
		}
	}

	return octant_keys;
}

Vector3 GridMap::_get_offset() const
{
	return Vector3(cell_size.x * 0.5 * int(center_x), cell_size.y * 0.5 * int(center_y),
		cell_size.z * 0.5 * int(center_z));
}

void GridMap::clear_baked_meshes()
{
	ERR_FAIL_NULL(Renderer::data);
	for (int i = 0; i < baked_meshes.size(); i++) {
		Renderer::free_rid(baked_meshes[i].instance);
	}
	baked_meshes.clear();

	_recreate_octant_data();
}

RID GridMap::get_bake_mesh_instance(int p_idx)
{
	ERR_FAIL_INDEX_V(p_idx, baked_meshes.size(), RID());
	return baked_meshes[p_idx].instance;
}

void GridMap::set_debug_show_octants(bool p_enable)
{
#ifdef DEBUG_ENABLED
	if (debug_show_octants == p_enable) {
		return;
	}

	debug_show_octants = p_enable;
	_debug_update();
#endif
}

bool GridMap::get_debug_show_octants() const { return debug_show_octants; }

#if defined(DEBUG_ENABLED) && !defined(NAVIGATION_3D_DISABLED)
void GridMap::_update_navigation_debug_edge_connections()
{
	if (bake_navigation) {
		for (const KeyValue<OctantKey, Octant*>& E : octant_map) {
			_update_octant_navigation_debug_edge_connections_mesh(E.key);
		}
	}
}

void GridMap::_navigation_map_changed(RID p_map)
{
	if (bake_navigation && is_inside_tree() && p_map == get_world_3d()->get_navigation_map()) {
		_update_navigation_debug_edge_connections();
	}
}
#endif // defined(DEBUG_ENABLED) && !defined(NAVIGATION_3D_DISABLED)

void GridMap::_debug_update() {}

void GridMap::_update_octant_navigation_debug_edge_connections_mesh(GridMap::OctantKey const&) {}

void GridMap::set_cell_item(Vector3i const&, int, int) {}

void GridMap::_octant_exit_world(GridMap::OctantKey const&) {}

void GridMap::_update_physics_bodies_characteristics() {}

bool GridMap::_octant_update(GridMap::OctantKey const&) { return true; }

void GridMap::_octant_transform(GridMap::OctantKey const&) {}

void GridMap::_octant_enter_world(GridMap::OctantKey const&) {}

void GridMap::navmesh_parse_init() {}

void GridMap::_debug_update_octants() {}

#ifndef _3D_DISABLED


void initialize_gridmap_module(ModuleInitializationLevel p_level)
{
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
#ifndef NAVIGATION_3D_DISABLED
		GridMap::navmesh_parse_init();
#endif // NAVIGATION_3D_DISABLED
	}
}

void uninitialize_gridmap_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
}

#endif // _3D_DISABLED
#endif // defined(MODULE_GRIDMAP_ENABLED)


// --- Merged from modules/meshoptimizer/register_types.cpp ---
#if defined(MODULE_MESHOPTIMIZER_ENABLED)
#include "main/main.h"

#include "scene/resources/mesh.h"

#include <thirdparty/meshoptimizer/meshoptimizer.h>

void initialize_meshoptimizer_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	SurfaceTool::optimize_vertex_cache_func = meshopt_optimizeVertexCache;
	SurfaceTool::optimize_vertex_fetch_remap_func = meshopt_optimizeVertexFetchRemap;
	SurfaceTool::simplify_func = meshopt_simplify;
	SurfaceTool::simplify_with_attrib_func = meshopt_simplifyWithAttributes;
	SurfaceTool::simplify_scale_func = meshopt_simplifyScale;
	SurfaceTool::generate_remap_func = meshopt_generateVertexRemap;
	SurfaceTool::remap_vertex_func = meshopt_remapVertexBuffer;
	SurfaceTool::remap_index_func = meshopt_remapIndexBuffer;
	SurfaceTool::generate_tangents_func = meshopt_generateTangents;
}

void uninitialize_meshoptimizer_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	SurfaceTool::optimize_vertex_cache_func = nullptr;
	SurfaceTool::optimize_vertex_fetch_remap_func = nullptr;
	SurfaceTool::simplify_func = nullptr;
	SurfaceTool::simplify_scale_func = nullptr;
	SurfaceTool::generate_remap_func = nullptr;
	SurfaceTool::remap_vertex_func = nullptr;
	SurfaceTool::remap_index_func = nullptr;
	SurfaceTool::generate_tangents_func = nullptr;
}
#endif // defined(MODULE_MESHOPTIMIZER_ENABLED)

#endif // _3D_DISABLED

// === Merged from modules/openxr/openxr_api.cpp ===
#include "modules/modules_enabled.gen.h"
#ifdef MODULE_OPENXR_ENABLED
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

#include "modules/openxr/openxr_api.h"
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/math/math_funcs.h"
#include "core/os/memory.h"
#include "core/types.h"
#include "core/version.h"
#include "modules/openxr/openxr_api.h"
// --- Inlined/Folded from modules/openxr/extensions/openxr_extensions.h ---
#include <openxr/openxr.h>
#include "modules/openxr/openxr_api.h"
// --- Folded from modules/openxr/openxr_interface.h ---
// A note on multithreading and thread safety in OpenXR.
//
// Most entry points will be called from the main thread in Godot
// however a number of entry points will be called from the
// rendering thread, potentially while we're already processing
// the next frame on the main thread.
//
// OpenXR itself has been designed with threading in mind including
// a high likelihood that the XR runtime runs in separate threads
// as well.
// Hence all the frame timing information, use of swapchains and
// sync functions.
// Do note that repeated calls to tracking APIs will provide
// increasingly more accurate data for the same timestamp as
// tracking data is continuously updated.
//
// For our code we mostly implement this in our OpenXRAPI class.
// We store data accessed from the rendering thread in a separate
// struct, setting values through our renderer command queue.
//
// As some data is setup before we start rendering, and cleaned up
// after we've stopped, that is accessed directly from both threads.

#include "modules/openxr/openxr_api.h"
#include "servers/xr/xr_server.h"

// declare some default strings
#define INTERACTION_PROFILE_NONE "/interaction_profiles/none"

class OpenXRInterface final : public XRInterface
{
private:
	bool initialized = false;
	bool reference_stage_changing = false;
	XRInterface::TrackingStatus tracking_state;

	// At a minimum we need a tracker for our head
	Ref<XRTracker> head;
	Transform3D head_transform;
	Vector3 head_linear_velocity;
	Vector3 head_angular_velocity;
	XRPose::TrackingConfidence head_confidence;
	Transform3D transform_for_view[2]; // We currently assume 2, but could be 4 for VARJO which we
									   // do not support yet

	XRVRS xr_vrs;

	void _load_action_map();

	struct Action
	{						// An action we've registered with OpenXR
		String action_name; // Name of our action as presented to Godot (can be altered from the
							// action map)
		OpenXRAction::ActionType action_type; // The action type of this action
		RID action_rid;						  // RID of the action registered with our OpenXR API
	};

	struct ActionSet
	{							 // An action set we've registered with OpenXR
		String action_set_name;	 // Name of our action set
		bool is_active;			 // If true this action set is active and we will sync it
		Vector<Action*> actions; // List of actions in this action set
		RID action_set_rid;		 // RID of the action registered with our OpenXR API
	};

	struct Tracker
	{							 // A tracker we've registered with OpenXR
		String tracker_name;	 // Name of our tracker (can be altered from the action map)
		Vector<Action*> actions; // Actions related to this tracker
		Ref<XRTracker>
			controller_tracker;	 // Our positional tracker object that holds our tracker state
		RID tracker_rid;		 // RID of the tracker registered with our OpenXR API
		RID interaction_profile; // RID of the interaction profile bound to this tracker (can be
								 // null)
	};

	Vector<ActionSet*> action_sets;
	Vector<RID> interaction_profiles;
	Vector<Tracker*> trackers;

	ActionSet* create_action_set(
		const String& p_action_set_name, const String& p_localized_name, const int p_priority);
	void free_action_sets();

	Action* create_action(ActionSet* p_action_set, const String& p_action_name,
		const String& p_localized_name, OpenXRAction::ActionType p_action_type,
		const Vector<Tracker*> p_trackers);
	Action* find_action(const String& p_action_name);
	void free_actions(ActionSet* p_action_set);

	Tracker* find_tracker(const String& p_tracker_name, bool p_create = false);
	void handle_tracker(Tracker* p_tracker);
	void free_trackers();

	void free_interaction_profiles();

	void _set_default_pos(Transform3D& r_transform, double p_world_scale, uint64_t p_eye);

protected:

public:
	virtual String get_name() const override;
	virtual uint32_t get_capabilities() const override;

	virtual PackedStringArray get_suggested_tracker_names() const override;
	virtual TrackingStatus get_tracking_status() const override;

	bool is_hand_tracking_supported();
	bool is_hand_interaction_supported() const;
	bool is_eye_gaze_interaction_supported();

	bool initialize_on_startup() const;
	virtual bool is_initialized() const override;
	virtual bool initialize() override;
	virtual void uninitialize() override;

	virtual void trigger_haptic_pulse(const String& p_action_name, const String& p_tracker_name,
		double p_frequency, double p_amplitude, double p_duration_sec,
		double p_delay_sec = 0) override;

	virtual bool supports_play_area_mode(XRInterface::PlayAreaMode p_mode) override;
	virtual XRInterface::PlayAreaMode get_play_area_mode() const override;
	virtual bool set_play_area_mode(XRInterface::PlayAreaMode p_mode) override;
	virtual PackedVector3Array get_play_area() const override;

	float get_display_refresh_rate() const;
	void set_display_refresh_rate(float p_refresh_rate);

	bool is_action_set_active(const String& p_action_set) const;
	void set_action_set_active(const String& p_action_set, bool p_active);

	double get_render_target_size_multiplier() const;
	void set_render_target_size_multiplier(double multiplier);

	bool is_foveation_supported() const;

	int get_foveation_level() const;
	void set_foveation_level(int p_foveation_level);

	bool get_foveation_dynamic() const;
	void set_foveation_dynamic(bool p_foveation_dynamic);

	bool get_foveation_with_subsampled_images() const;
	void set_foveation_with_subsampled_images(bool p_enabled);

	float get_vrs_min_radius() const;
	void set_vrs_min_radius(float p_vrs_min_radius);

	float get_vrs_strength() const;
	void set_vrs_strength(float p_vrs_strength);

	virtual Size2 get_render_target_size() override;
	virtual uint32_t get_view_count() override;
	virtual Transform3D get_camera_transform() override;
	virtual Transform3D get_transform_for_view(
		uint32_t p_view, const Transform3D& p_cam_transform) override;
	virtual Projection get_projection_for_view(
		uint32_t p_view, double p_aspect, double p_z_near, double p_z_far) override;

	virtual Rect2i get_render_region() override;

	virtual RID get_color_texture() override;
	virtual RID get_depth_texture() override;
	virtual RID get_velocity_texture() override;
	virtual RID get_velocity_depth_texture() override;
	virtual Size2i get_velocity_target_size() override;

	virtual void process() override;
	virtual void pre_render() override;
	bool pre_draw_viewport(RID p_render_target) override;
	virtual Vector<RenderingServerTypes::BlitToScreen> post_draw_viewport(
		RID p_render_target, const Rect2& p_screen_rect) override;
	virtual void end_frame() override;

	virtual bool is_passthrough_supported() override;
	virtual bool is_passthrough_enabled() override;
	virtual bool start_passthrough() override;
	virtual void stop_passthrough() override;

	virtual XRInterface::EnvironmentBlendMode get_environment_blend_mode() const override;
	virtual bool set_environment_blend_mode(XRInterface::EnvironmentBlendMode mode) override;

	void on_state_ready();
	void on_state_visible();
	void on_state_focused();
	void on_state_synchronized();
	void on_state_stopping();
	void on_state_loss_pending();
	void on_state_exiting();
	void on_reference_space_change_pending(XrReferenceSpaceType p_type);
	void on_refresh_rate_changes(float p_new_rate);
	void tracker_profile_changed(RID p_tracker, RID p_interaction_profile);

	/** Session */
	enum SessionState
	{ // Should mirror XrSessionState
		SESSION_STATE_UNKNOWN = 0,
		SESSION_STATE_IDLE = 1,
		SESSION_STATE_READY = 2,
		SESSION_STATE_SYNCHRONIZED = 3,
		SESSION_STATE_VISIBLE = 4,
		SESSION_STATE_FOCUSED = 5,
		SESSION_STATE_STOPPING = 6,
		SESSION_STATE_LOSS_PENDING = 7,
		SESSION_STATE_EXITING = 8,
	};

	SessionState get_session_state();

	/** User presence. */
	bool is_user_presence_supported() const;
	bool is_user_present() const;

	/** Hand tracking. */
	enum Hand
	{
		HAND_LEFT,
		HAND_RIGHT,
		HAND_MAX,
	};

	enum HandMotionRange
	{
		HAND_MOTION_RANGE_UNOBSTRUCTED,
		HAND_MOTION_RANGE_CONFORM_TO_CONTROLLER,
		HAND_MOTION_RANGE_MAX
	};

	void set_motion_range(const Hand p_hand, const HandMotionRange p_motion_range);
	HandMotionRange get_motion_range(const Hand p_hand) const;

	enum HandTrackedSource
	{
		HAND_TRACKED_SOURCE_UNKNOWN,
		HAND_TRACKED_SOURCE_UNOBSTRUCTED,
		HAND_TRACKED_SOURCE_CONTROLLER,
		HAND_TRACKED_SOURCE_MAX
	};

	HandTrackedSource get_hand_tracking_source(const Hand p_hand) const;

	enum HandJoints
	{
		HAND_JOINT_PALM = 0,
		HAND_JOINT_WRIST = 1,
		HAND_JOINT_THUMB_METACARPAL = 2,
		HAND_JOINT_THUMB_PROXIMAL = 3,
		HAND_JOINT_THUMB_DISTAL = 4,
		HAND_JOINT_THUMB_TIP = 5,
		HAND_JOINT_INDEX_METACARPAL = 6,
		HAND_JOINT_INDEX_PROXIMAL = 7,
		HAND_JOINT_INDEX_INTERMEDIATE = 8,
		HAND_JOINT_INDEX_DISTAL = 9,
		HAND_JOINT_INDEX_TIP = 10,
		HAND_JOINT_MIDDLE_METACARPAL = 11,
		HAND_JOINT_MIDDLE_PROXIMAL = 12,
		HAND_JOINT_MIDDLE_INTERMEDIATE = 13,
		HAND_JOINT_MIDDLE_DISTAL = 14,
		HAND_JOINT_MIDDLE_TIP = 15,
		HAND_JOINT_RING_METACARPAL = 16,
		HAND_JOINT_RING_PROXIMAL = 17,
		HAND_JOINT_RING_INTERMEDIATE = 18,
		HAND_JOINT_RING_DISTAL = 19,
		HAND_JOINT_RING_TIP = 20,
		HAND_JOINT_LITTLE_METACARPAL = 21,
		HAND_JOINT_LITTLE_PROXIMAL = 22,
		HAND_JOINT_LITTLE_INTERMEDIATE = 23,
		HAND_JOINT_LITTLE_DISTAL = 24,
		HAND_JOINT_LITTLE_TIP = 25,
		HAND_JOINT_MAX = 26,
	};

	enum HandJointFlags
	{
		HAND_JOINT_NONE = 0,
		HAND_JOINT_ORIENTATION_VALID = 1,
		HAND_JOINT_ORIENTATION_TRACKED = 2,
		HAND_JOINT_POSITION_VALID = 4,
		HAND_JOINT_POSITION_TRACKED = 8,
		HAND_JOINT_LINEAR_VELOCITY_VALID = 16,
		HAND_JOINT_ANGULAR_VELOCITY_VALID = 32,
	};

	uint32_t get_hand_joint_flags(Hand p_hand, HandJoints p_joint) const;
	Quaternion get_hand_joint_rotation(Hand p_hand, HandJoints p_joint) const;
	Vector3 get_hand_joint_position(Hand p_hand, HandJoints p_joint) const;
	float get_hand_joint_radius(Hand p_hand, HandJoints p_joint) const;

	Vector3 get_hand_joint_linear_velocity(Hand p_hand, HandJoints p_joint) const;
	Vector3 get_hand_joint_angular_velocity(Hand p_hand, HandJoints p_joint) const;

	virtual RID get_vrs_texture() override;
	virtual VRSTextureFormat get_vrs_texture_format() override;

	// Performance settings.
	enum PerfSettingsLevel
	{
		PERF_SETTINGS_LEVEL_POWER_SAVINGS,
		PERF_SETTINGS_LEVEL_SUSTAINED_LOW,
		PERF_SETTINGS_LEVEL_SUSTAINED_HIGH,
		PERF_SETTINGS_LEVEL_BOOST,
	};

	enum PerfSettingsSubDomain
	{
		PERF_SETTINGS_SUB_DOMAIN_COMPOSITING,
		PERF_SETTINGS_SUB_DOMAIN_RENDERING,
		PERF_SETTINGS_SUB_DOMAIN_THERMAL,
	};

	enum PerfSettingsNotificationLevel
	{
		PERF_SETTINGS_NOTIF_LEVEL_NORMAL,
		PERF_SETTINGS_NOTIF_LEVEL_WARNING,
		PERF_SETTINGS_NOTIF_LEVEL_IMPAIRED,
	};

	void set_cpu_level(PerfSettingsLevel p_level);
	void set_gpu_level(PerfSettingsLevel p_level);
	void on_cpu_level_changed(PerfSettingsSubDomain p_sub_domain,
		PerfSettingsNotificationLevel p_from_level, PerfSettingsNotificationLevel p_to_level);
	void on_gpu_level_changed(PerfSettingsSubDomain p_sub_domain,
		PerfSettingsNotificationLevel p_from_level, PerfSettingsNotificationLevel p_to_level);

	OpenXRInterface();
	~OpenXRInterface();
};

// --- Folded from modules/openxr/scene/openxr_scene.h ---
#include <openxr/openxr.h>
#include "modules/modules_enabled.gen.h"
#include "scene/3d/node_3d.h"
#include "servers/xr/xr_server.h"

class JavaObject;
class MeshInstance3D;
class Mesh;
class OpenXRAPI;
class OpenXRCompositionLayerExtension;
class OpenXRHandTrackingExtension;
class SubViewport;
class Camera3D;

///////////////////////////////////////////////////////////////////////////
// OpenXRCompositionLayer

class OpenXRCompositionLayer final : public Node3D
{
public:
	enum LayerType
	{
		LAYER_QUAD,
		LAYER_CYLINDER,
		LAYER_EQUIRECT,
	};

	// Must be identical to Filter enum definition in OpenXRCompositionLayerExtension.
	enum Filter
	{
		FILTER_NEAREST,
		FILTER_LINEAR,
		FILTER_CUBIC,
	};

	// Must be identical to MipmapMode enum definition in OpenXRCompositionLayerExtension.
	enum MipmapMode
	{
		MIPMAP_MODE_DISABLED,
		MIPMAP_MODE_NEAREST,
		MIPMAP_MODE_LINEAR,
	};

	// Must be identical to Wrap enum definition in OpenXRCompositionLayerExtension.
	enum Wrap
	{
		WRAP_CLAMP_TO_BORDER,
		WRAP_CLAMP_TO_EDGE,
		WRAP_REPEAT,
		WRAP_MIRRORED_REPEAT,
		WRAP_MIRROR_CLAMP_TO_EDGE,
	};

	// Must be identical to Swizzle enum definition in OpenXRCompositionLayerExtension.
	enum Swizzle
	{
		SWIZZLE_RED,
		SWIZZLE_GREEN,
		SWIZZLE_BLUE,
		SWIZZLE_ALPHA,
		SWIZZLE_ZERO,
		SWIZZLE_ONE,
	};

	// Must be identical to EyeVisibility enum definition in OpenXRCompositionLayerExtension.
	enum EyeVisibility
	{
		EYE_VISIBILITY_BOTH,
		EYE_VISIBILITY_LEFT,
		EYE_VISIBILITY_RIGHT,
	};

private:
	RID composition_layer;
	LayerType layer_type = LAYER_QUAD;

	// Quad parameters.
	Size2 quad_size = Size2(1.0, 1.0);

	// Cylinder / Equirect shared parameters.
	float radius = 1.0;
	uint32_t fallback_segments = 10;

	// Cylinder parameters.
	float aspect_ratio = 1.0;
	float central_angle = Math::PI / 2.0;

	// Equirect parameters.
	float central_horizontal_angle = Math::PI / 2.0;
	float upper_vertical_angle = Math::PI / 4.0;
	float lower_vertical_angle = Math::PI / 4.0;

	SubViewport* layer_viewport = nullptr;
	bool use_android_surface = false;
	Size2i android_surface_size = Size2i(1024, 1024);
	bool enable_hole_punch = false;
	bool alpha_blend = false;
	int sort_order = 1;
	bool protected_content = false;
	MeshInstance3D* fallback = nullptr;
	bool should_update_fallback_mesh = false;
	bool openxr_session_running = false;
	bool registered = false;

	Filter min_filter = FILTER_LINEAR;
	Filter mag_filter = FILTER_LINEAR;
	MipmapMode mipmap_mode = MIPMAP_MODE_LINEAR;
	Wrap horizontal_wrap = WRAP_CLAMP_TO_BORDER;
	Wrap vertical_wrap = WRAP_CLAMP_TO_BORDER;
	Swizzle red_swizzle = SWIZZLE_RED;
	Swizzle green_swizzle = SWIZZLE_GREEN;
	Swizzle blue_swizzle = SWIZZLE_BLUE;
	Swizzle alpha_swizzle = SWIZZLE_ALPHA;
	float max_anisotropy = 1.0;
	Color border_color = {0.0, 0.0, 0.0, 0.0};
	EyeVisibility eye_visibility = EYE_VISIBILITY_BOTH;

	bool _should_use_fallback_node();
	void _create_fallback_node();
	void _reset_fallback_material();
	void _remove_fallback_node();

	void _setup_composition_layer();
	void _clear_composition_layer();

	void _viewport_size_changed();

	Camera3D* _get_xrcamera3d_ancestor() const;

	void _notification(int p_what);

	void _on_openxr_session_begun();
	void _on_openxr_session_stopping();

	bool _should_register();

	Ref<Mesh> _create_fallback_mesh();
	XrStructureType _get_openxr_type() const;

	void update_transform();
	void update_fallback_mesh();

	static Vector<OpenXRCompositionLayer*> composition_layer_nodes;
	bool is_viewport_in_use(SubViewport* p_viewport);

public:
	LayerType get_layer_type() const { return layer_type; }

	void set_layer_viewport(SubViewport* p_viewport);
	SubViewport* get_layer_viewport() const;

	void set_use_android_surface(bool p_use_android_surface);
	bool get_use_android_surface() const;

	void set_android_surface_size(Size2i p_size);
	Size2i get_android_surface_size() const;

	bool get_enable_hole_punch() const;

	int get_sort_order() const;

	void set_alpha_blend(bool p_alpha_blend);
	bool get_alpha_blend() const;

	Ref<JavaObject> get_android_surface();
	bool is_natively_supported() const;

	void set_protected_content(bool p_protected_content);
	bool is_protected_content() const;

	void set_min_filter(Filter p_mode);
	Filter get_min_filter() const;

	void set_mag_filter(Filter p_mode);
	Filter get_mag_filter() const;

	void set_mipmap_mode(MipmapMode p_mode);
	MipmapMode get_mipmap_mode() const;

	void set_horizontal_wrap(Wrap p_mode);
	Wrap get_horizontal_wrap() const;

	void set_vertical_wrap(Wrap p_mode);
	Wrap get_vertical_wrap() const;

	void set_red_swizzle(Swizzle p_mode);
	Swizzle get_red_swizzle() const;

	void set_green_swizzle(Swizzle p_mode);
	Swizzle get_green_swizzle() const;

	void set_blue_swizzle(Swizzle p_mode);
	Swizzle get_blue_swizzle() const;

	void set_alpha_swizzle(Swizzle p_mode);
	Swizzle get_alpha_swizzle() const;

	void set_max_anisotropy(float p_value);
	float get_max_anisotropy() const;

	void set_border_color(const Color& p_color);
	Color get_border_color() const;

	void set_eye_visibility(EyeVisibility p_eye_visibility);
	EyeVisibility get_eye_visibility() const;

	// Quad properties.
	void set_quad_size(const Size2& p_size);
	Size2 get_quad_size() const;

	// Cylinder & Equirect shared properties.
	void set_radius(float p_radius);
	float get_radius() const;

	void set_fallback_segments(uint32_t p_fallback_segments);
	uint32_t get_fallback_segments() const;

	// Cylinder properties.
	void set_aspect_ratio(float p_aspect_ratio);
	float get_aspect_ratio() const;

	void set_central_angle(float p_angle);
	float get_central_angle() const;

	// Equirect properties.
	void set_central_horizontal_angle(float p_angle);
	float get_central_horizontal_angle() const;

	void set_upper_vertical_angle(float p_angle);
	float get_upper_vertical_angle() const;

	void set_lower_vertical_angle(float p_angle);
	float get_lower_vertical_angle() const;

	virtual PackedStringArray get_configuration_warnings() const override;

	Vector2 intersects_ray(const Vector3& p_origin, const Vector3& p_direction) const;

	OpenXRCompositionLayer(LayerType p_type = LAYER_QUAD);
	~OpenXRCompositionLayer() = default;
};

///////////////////////////////////////////////////////////////////////////
// OpenXRHand

class OpenXRHand final : public Node3D
{
public:
	enum Hands
	{
		HAND_LEFT,
		HAND_RIGHT,
		HAND_MAX
	};

	enum MotionRange
	{
		MOTION_RANGE_UNOBSTRUCTED,
		MOTION_RANGE_CONFORM_TO_CONTROLLER,
		MOTION_RANGE_MAX
	};

	enum SkeletonRig
	{
		SKELETON_RIG_OPENXR,
		SKELETON_RIG_HUMANOID,
		SKELETON_RIG_MAX
	};

	enum BoneUpdate
	{
		BONE_UPDATE_FULL,
		BONE_UPDATE_ROTATION_ONLY,
		BONE_UPDATE_MAX
	};

private:
	struct JointData
	{
		int bone = -1;
		int parent_joint = -1;
	};

	Hands hand = HAND_LEFT;
	MotionRange motion_range = MOTION_RANGE_UNOBSTRUCTED;
	NodePath hand_skeleton;
	SkeletonRig skeleton_rig = SKELETON_RIG_OPENXR;
	BoneUpdate bone_update = BONE_UPDATE_FULL;

	JointData joints[XR_HAND_JOINT_COUNT_EXT];

	void _set_motion_range();

	Skeleton3D* get_skeleton();
	void _get_joint_data();
	void _update_skeleton();

public:
	OpenXRHand();

	void set_hand(Hands p_hand);
	Hands get_hand() const;

	void set_motion_range(MotionRange p_motion_range);
	MotionRange get_motion_range() const;

	void set_hand_skeleton(const NodePath& p_hand_skeleton);
	NodePath get_hand_skeleton() const;

	void set_skeleton_rig(SkeletonRig p_skeleton_rig);
	SkeletonRig get_skeleton_rig() const;

	void set_bone_update(BoneUpdate p_bone_update);
	BoneUpdate get_bone_update() const;

	void _notification(int p_what);
};

///////////////////////////////////////////////////////////////////////////
// OpenXRVisibilityMask

class OpenXRVisibilityMask final : public Node3D
{
public:
	RenderInstance3D render_instance;

private:
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

	void _on_openxr_session_begun();
	void _on_openxr_session_stopping();

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	AABB get_aabb() const;

	OpenXRVisibilityMask() = default;
	~OpenXRVisibilityMask() = default;
};

///////////////////////////////////////////////////////////////////////////
// OpenXRRenderModel & OpenXRRenderModelManager

#ifdef MODULE_GLTF_ENABLED
class OpenXRRenderModel final : public Node3D
{
private:
	RID render_model;
	Node3D* scene = nullptr;
	HashMap<String, Node3D*> animatable_nodes;

	void _load_render_model_scene();
	void _on_render_model_top_level_path_changed(RID p_render_model);
	void _notification(int p_what);

public:
	virtual PackedStringArray get_configuration_warnings() const override;

	RID get_render_model() const;
	void set_render_model(RID p_render_model);

	String get_top_level_path() const;
};

class OpenXRRenderModelManager final : public Node3D
{
public:
	enum RenderModelTracker
	{
		RENDER_MODEL_TRACKER_ANY,
		RENDER_MODEL_TRACKER_NONE_SET,
		RENDER_MODEL_TRACKER_LEFT_HAND,
		RENDER_MODEL_TRACKER_RIGHT_HAND,
	};

	virtual PackedStringArray get_configuration_warnings() const override;

	void set_tracker(RenderModelTracker p_tracker);
	RenderModelTracker get_tracker() const;

	void set_make_local_to_pose(const String& p_action);
	String get_make_local_to_pose() const;

private:
	HashMap<RID, Node3D*> render_models;
	Node3D* container = nullptr;

	bool is_dirty = false;
	RenderModelTracker tracker = RENDER_MODEL_TRACKER_ANY;
	String make_local_to_pose;

	Ref<XRTracker> positional_tracker;
	XrPath xr_path = XR_NULL_PATH;

	bool _has_filters();
	void _on_render_model_added(RID p_render_model);
	void _on_render_model_removed(RID p_render_model);
	void _on_render_model_top_level_path_changed(RID p_path);
	void _update_models();
	void _notification(int p_what);
};
#endif // MODULE_GLTF_ENABLED

// --- End modules/openxr/scene/openxr_scene.h ---

// --- End modules/openxr/openxr_interface.h ---
#include "core/types.h"
#include "core/templates/hash_map.h"
#include "core/templates/rid_owner.h"
#include "core/templates/vector.h"
#include "core/types.h"
#include "modules/modules_enabled.gen.h"
#include "servers/xr/xr_server.h"

#ifdef XR_USE_PLATFORM_ANDROID
#include <jni.h>
#include <openxr/openxr_platform.h>
#endif

#ifdef ANDROID_ENABLED
#include <jni.h>

typedef XrResult(XRAPI_PTR* PFN_xrCreateSwapchainAndroidSurfaceKHR)(
	XrSession session, const XrSwapchainCreateInfo* info, XrSwapchain* swapchain, jobject* surface);
#endif

#ifdef MODULE_GLTF_ENABLED
class GLTFDocument;
class GLTFState;
class Node3D;
template <typename T> class TypedArray;
#endif

class JavaObject;

///////////////////////////////////////////////////////////////////////////
// OpenXREyeGazeInteractionExtension

class OpenXREyeGazeInteractionExtension final
{
public:
	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);
	static void* set_system_properties_and_get_next_pointer(void* p_next_pointer);

	static PackedStringArray get_suggested_tracker_names();

	static bool is_available();
	static bool supports_eye_gaze_interaction();

	static bool get_eye_gaze_pose(double p_dist, Vector3& r_eye_pose);

	struct Data
	{
		bool available = false;
		XrSystemEyeGazeInteractionPropertiesEXT properties;
		bool init_eye_gaze_pose = false;
		RID eye_tracker;
		RID eye_action;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXREyeGazeInteractionExtension() = delete;
	OpenXREyeGazeInteractionExtension(const OpenXREyeGazeInteractionExtension&) = delete;
	OpenXREyeGazeInteractionExtension& operator=(const OpenXREyeGazeInteractionExtension&) = delete;
	~OpenXREyeGazeInteractionExtension() = delete;
};

///////////////////////////////////////////////////////////////////////////
// OpenXRUserPresenceExtension

class OpenXRUserPresenceExtension final
{
public:
	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);
	static void* set_system_properties_and_get_next_pointer(void* p_next_pointer);

	static void on_state_ready();
	static void on_state_stopping();

	static bool on_event_polled(const XrEventDataBuffer& event);

	static bool is_user_present();
	static bool is_active();

	struct Data
	{
		bool available = false;
		XrSystemUserPresencePropertiesEXT properties;
		bool user_present = true;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRUserPresenceExtension() = delete;
	OpenXRUserPresenceExtension(const OpenXRUserPresenceExtension&) = delete;
	OpenXRUserPresenceExtension& operator=(const OpenXRUserPresenceExtension&) = delete;
	~OpenXRUserPresenceExtension() = delete;
};

///////////////////////////////////////////////////////////////////////////
// OpenXRAndroidThreadSettingsExtension

class OpenXRAndroidThreadSettingsExtension final
{
public:
	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);
	static void on_instance_created(XrInstance p_instance);
	static void on_session_created(XrSession p_session);

	enum ThreadType
	{
		THREAD_TYPE_APPLICATION_MAIN,
		THREAD_TYPE_APPLICATION_WORKER,
		THREAD_TYPE_RENDERER_MAIN,
		THREAD_TYPE_RENDERER_WORKER,
	};

	static bool set_application_thread_type(ThreadType p_thread_type, uint32_t p_thread_id = 0);

private:
	static bool _initialize_openxr_android_thread_settings_extension();
	static void _set_render_thread_type();

#ifdef XR_USE_PLATFORM_ANDROID
	EXT_PROTO_XRRESULT_FUNC3(xrSetAndroidApplicationThreadKHR, (XrSession), session,
		(XrAndroidThreadTypeKHR), threadType, (uint32_t), threadId);
#endif

public:
	struct Data
	{
		bool available = false;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRAndroidThreadSettingsExtension() = delete;
	OpenXRAndroidThreadSettingsExtension(const OpenXRAndroidThreadSettingsExtension&) = delete;
	OpenXRAndroidThreadSettingsExtension& operator=(
		const OpenXRAndroidThreadSettingsExtension&) = delete;
	~OpenXRAndroidThreadSettingsExtension() = delete;
};

///////////////////////////////////////////////////////////////////////////
// OpenXRDisplayRefreshRateExtension

class OpenXRDisplayRefreshRateExtension final
{
public:
	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);

	static void on_instance_created(const XrInstance p_instance);
	static void on_instance_destroyed();
	static bool on_event_polled(const XrEventDataBuffer& event);

	static float get_refresh_rate();
	static void set_refresh_rate(float p_refresh_rate);

private:
	EXT_PROTO_XRRESULT_FUNC4(xrEnumerateDisplayRefreshRatesFB, (XrSession), session, (uint32_t),
		displayRefreshRateCapacityInput, (uint32_t*), displayRefreshRateCountOutput, (float*),
		displayRefreshRates);
	EXT_PROTO_XRRESULT_FUNC2(
		xrGetDisplayRefreshRateFB, (XrSession), session, (float*), display_refresh_rate);
	EXT_PROTO_XRRESULT_FUNC2(
		xrRequestDisplayRefreshRateFB, (XrSession), session, (float), display_refresh_rate);

public:
	struct Data
	{
		bool display_refresh_rate_ext = false;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRDisplayRefreshRateExtension() = delete;
	OpenXRDisplayRefreshRateExtension(const OpenXRDisplayRefreshRateExtension&) = delete;
	OpenXRDisplayRefreshRateExtension& operator=(const OpenXRDisplayRefreshRateExtension&) = delete;
	~OpenXRDisplayRefreshRateExtension() = delete;
};

///////////////////////////////////////////////////////////////////////////
// OpenXRPerformanceSettingsExtension

class OpenXRPerformanceSettingsExtension final
{
public:
	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);

	static void on_instance_created(const XrInstance p_instance);
	static bool on_event_polled(const XrEventDataBuffer& event);

	static bool is_available();

	static void set_cpu_level(OpenXRInterface::PerfSettingsLevel p_level);
	static void set_gpu_level(OpenXRInterface::PerfSettingsLevel p_level);

private:
	static XrPerfSettingsLevelEXT level_to_openxr(OpenXRInterface::PerfSettingsLevel p_level);
	static OpenXRInterface::PerfSettingsSubDomain openxr_to_sub_domain(
		XrPerfSettingsSubDomainEXT p_sub_domain);
	static OpenXRInterface::PerfSettingsNotificationLevel openxr_to_notification_level(
		XrPerfSettingsNotificationLevelEXT p_notification_level);

	EXT_PROTO_XRRESULT_FUNC3(xrPerfSettingsSetPerformanceLevelEXT, (XrSession), session,
		(XrPerfSettingsDomainEXT), domain, (XrPerfSettingsLevelEXT), level)

public:
	struct Data
	{
		bool available = false;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRPerformanceSettingsExtension() = delete;
	OpenXRPerformanceSettingsExtension(const OpenXRPerformanceSettingsExtension&) = delete;
	OpenXRPerformanceSettingsExtension& operator=(
		const OpenXRPerformanceSettingsExtension&) = delete;
	~OpenXRPerformanceSettingsExtension() = delete;
};

///////////////////////////////////////////////////////////////////////////
// OpenXRDebugUtilsExtension

class OpenXRDebugUtilsExtension final
{
public:
	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);
	static void on_instance_created(const XrInstance p_instance);
	static void on_instance_destroyed();

	static bool get_active();

	static void set_object_name(
		XrObjectType p_object_type, uint64_t p_object_handle, const char* p_object_name);
	static void begin_debug_label_region(const char* p_label_name);
	static void end_debug_label_region();
	static void insert_debug_label(const char* p_label_name);

private:
	static XrBool32 XRAPI_PTR _debug_callback(
		XrDebugUtilsMessageSeverityFlagsEXT p_message_severity,
		XrDebugUtilsMessageTypeFlagsEXT p_message_types,
		const XrDebugUtilsMessengerCallbackDataEXT* p_callback_data, void* p_user_data);
	static XrBool32 debug_callback(XrDebugUtilsMessageSeverityFlagsEXT p_message_severity,
		XrDebugUtilsMessageTypeFlagsEXT p_message_types,
		const XrDebugUtilsMessengerCallbackDataEXT* p_callback_data, void* p_user_data);

	EXT_PROTO_XRRESULT_FUNC3(xrCreateDebugUtilsMessengerEXT, (XrInstance), p_instance,
		(const XrDebugUtilsMessengerCreateInfoEXT*), p_create_info, (XrDebugUtilsMessengerEXT*),
		p_messenger)
	EXT_PROTO_XRRESULT_FUNC1(
		xrDestroyDebugUtilsMessengerEXT, (XrDebugUtilsMessengerEXT), p_messenger)
	EXT_PROTO_XRRESULT_FUNC2(xrSetDebugUtilsObjectNameEXT, (XrInstance), p_instance,
		(const XrDebugUtilsObjectNameInfoEXT*), p_name_info)
	EXT_PROTO_XRRESULT_FUNC2(xrSessionBeginDebugUtilsLabelRegionEXT, (XrSession), p_session,
		(const XrDebugUtilsLabelEXT*), p_label_info)
	EXT_PROTO_XRRESULT_FUNC1(xrSessionEndDebugUtilsLabelRegionEXT, (XrSession), p_session)
	EXT_PROTO_XRRESULT_FUNC2(xrSessionInsertDebugUtilsLabelEXT, (XrSession), p_session,
		(const XrDebugUtilsLabelEXT*), p_label_info)

public:
	struct Data
	{
		bool debug_utils_ext = false;
		XrDebugUtilsMessengerEXT default_messenger = XR_NULL_HANDLE;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRDebugUtilsExtension() = delete;
	OpenXRDebugUtilsExtension(const OpenXRDebugUtilsExtension&) = delete;
	OpenXRDebugUtilsExtension& operator=(const OpenXRDebugUtilsExtension&) = delete;
	~OpenXRDebugUtilsExtension() = delete;
};

///////////////////////////////////////////////////////////////////////////
// OpenXRVisibilityMaskExtension

class OpenXRVisibilityMaskExtension final
{
public:
	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);

	static void on_instance_created(const XrInstance p_instance);

	static void on_session_created(const XrSession p_instance);
	static void on_session_destroyed();

	static void on_pre_render();
	static bool on_event_polled(const XrEventDataBuffer& event);

	static bool is_available();
	static RID get_mesh();

private:
	struct MeshData
	{
		Vector<XrVector2f> vertices;
		Vector<uint32_t> indices;
	};

	static void _update_mesh_data(uint32_t p_view);
	static void _update_mesh();

	EXT_PROTO_XRRESULT_FUNC5(xrGetVisibilityMaskKHR, (XrSession), session,
		(XrViewConfigurationType), viewConfigurationType, (uint32_t), viewIndex,
		(XrVisibilityMaskTypeKHR), visibilityMaskType, (XrVisibilityMaskKHR*), visibilityMask);

public:
	struct Data
	{
		bool available = false;
		bool is_dirty = false;
		RID shader;
		RID material;
		RID mesh;
		uint32_t mesh_count = 0;
		MeshData mesh_data[4];
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRVisibilityMaskExtension() = delete;
	OpenXRVisibilityMaskExtension(const OpenXRVisibilityMaskExtension&) = delete;
	OpenXRVisibilityMaskExtension& operator=(const OpenXRVisibilityMaskExtension&) = delete;
	~OpenXRVisibilityMaskExtension() = delete;
};

///////////////////////////////////////////////////////////////////////////
// OpenXRCompositionLayerExtension

class OpenXRCompositionLayerExtension final
{
public:
	// Must be identical to Filter enum definition in OpenXRCompositionLayer.
	enum Filter
	{
		FILTER_NEAREST,
		FILTER_LINEAR,
		FILTER_CUBIC,
	};

	// Must be identical to MipmapMode enum definition in OpenXRCompositionLayer.
	enum MipmapMode
	{
		MIPMAP_MODE_DISABLED,
		MIPMAP_MODE_NEAREST,
		MIPMAP_MODE_LINEAR,
	};

	// Must be identical to Wrap enum definition in OpenXRCompositionLayer.
	enum Wrap
	{
		WRAP_CLAMP_TO_BORDER,
		WRAP_CLAMP_TO_EDGE,
		WRAP_REPEAT,
		WRAP_MIRRORED_REPEAT,
		WRAP_MIRROR_CLAMP_TO_EDGE,
	};

	// Must be identical to Swizzle enum definition in OpenXRCompositionLayer.
	enum Swizzle
	{
		SWIZZLE_RED,
		SWIZZLE_GREEN,
		SWIZZLE_BLUE,
		SWIZZLE_ALPHA,
		SWIZZLE_ZERO,
		SWIZZLE_ONE,
	};

	// Must be identical to EyeVisibility enum definition in OpenXRCompositionLayer.
	enum EyeVisibility
	{
		EYE_VISIBILITY_BOTH,
		EYE_VISIBILITY_LEFT,
		EYE_VISIBILITY_RIGHT,
	};

	enum PoseSpace
	{
		POSE_WORLD_LOCKED,
		POSE_HEAD_LOCKED,
	};

	struct SwapchainState
	{
		Filter min_filter = Filter::FILTER_LINEAR;
		Filter mag_filter = Filter::FILTER_LINEAR;
		MipmapMode mipmap_mode = MipmapMode::MIPMAP_MODE_LINEAR;
		Wrap horizontal_wrap = Wrap::WRAP_CLAMP_TO_BORDER;
		Wrap vertical_wrap = Wrap::WRAP_CLAMP_TO_BORDER;
		Swizzle red_swizzle = Swizzle::SWIZZLE_RED;
		Swizzle green_swizzle = Swizzle::SWIZZLE_GREEN;
		Swizzle blue_swizzle = Swizzle::SWIZZLE_BLUE;
		Swizzle alpha_swizzle = Swizzle::SWIZZLE_ALPHA;
		float max_anisotropy = 1.0;
		Color border_color = {0.0, 0.0, 0.0, 0.0};
	};

	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);
	static void on_instance_created(const XrInstance p_instance);
	static void on_session_created(const XrSession p_session);
	static void on_session_destroyed();
	static void on_pre_render();

	static int get_composition_layer_count();
	static XrCompositionLayerBaseHeader* get_composition_layer(int p_index);
	static int get_composition_layer_order(int p_index);

	static RID composition_layer_create(XrCompositionLayerBaseHeader* p_openxr_layer);
	static void composition_layer_free(RID p_layer);

	static void composition_layer_register(RID p_layer);
	static void composition_layer_unregister(RID p_layer);

#define OPENXR_LAYER_FUNC1_DECL(m_name, m_arg1)                                                    \
	static void _composition_layer_##m_name##_rt(RID p_layer, m_arg1 p1);                          \
	static void composition_layer_##m_name(RID p_layer, m_arg1 p1);

#define OPENXR_LAYER_FUNC2_DECL(m_name, m_arg1, m_arg2)                                            \
	static void _composition_layer_##m_name##_rt(RID p_layer, m_arg1 p1, m_arg2 p2);               \
	static void composition_layer_##m_name(RID p_layer, m_arg1 p1, m_arg2 p2);

	OPENXR_LAYER_FUNC2_DECL(set_viewport, RID, const Size2i&);
	OPENXR_LAYER_FUNC2_DECL(set_use_android_surface, bool, const Size2i&);
	OPENXR_LAYER_FUNC1_DECL(set_sort_order, int);
	OPENXR_LAYER_FUNC1_DECL(set_alpha_blend, bool);
	OPENXR_LAYER_FUNC1_DECL(set_transform, const Transform3D&);
	OPENXR_LAYER_FUNC1_DECL(set_protected_content, bool);

	OPENXR_LAYER_FUNC1_DECL(set_min_filter, Filter);
	OPENXR_LAYER_FUNC1_DECL(set_mag_filter, Filter);
	OPENXR_LAYER_FUNC1_DECL(set_mipmap_mode, MipmapMode);
	OPENXR_LAYER_FUNC1_DECL(set_horizontal_wrap, Wrap);
	OPENXR_LAYER_FUNC1_DECL(set_vertical_wrap, Wrap);
	OPENXR_LAYER_FUNC1_DECL(set_red_swizzle, Swizzle);
	OPENXR_LAYER_FUNC1_DECL(set_blue_swizzle, Swizzle);
	OPENXR_LAYER_FUNC1_DECL(set_green_swizzle, Swizzle);
	OPENXR_LAYER_FUNC1_DECL(set_alpha_swizzle, Swizzle);
	OPENXR_LAYER_FUNC1_DECL(set_max_anisotropy, float);
	OPENXR_LAYER_FUNC1_DECL(set_border_color, const Color&);
	OPENXR_LAYER_FUNC1_DECL(set_pose_space, PoseSpace);
	OPENXR_LAYER_FUNC1_DECL(set_eye_visibility, EyeVisibility);

	OPENXR_LAYER_FUNC1_DECL(set_quad_size, const Size2&);

	OPENXR_LAYER_FUNC1_DECL(set_cylinder_radius, float);
	OPENXR_LAYER_FUNC1_DECL(set_cylinder_aspect_ratio, float);
	OPENXR_LAYER_FUNC1_DECL(set_cylinder_central_angle, float);

	OPENXR_LAYER_FUNC1_DECL(set_equirect_radius, float);
	OPENXR_LAYER_FUNC1_DECL(set_equirect_central_horizontal_angle, float);
	OPENXR_LAYER_FUNC1_DECL(set_equirect_upper_vertical_angle, float);
	OPENXR_LAYER_FUNC1_DECL(set_equirect_lower_vertical_angle, float);

#undef OPENXR_LAYER_FUNC1_DECL
#undef OPENXR_LAYER_FUNC2_DECL

	static Ref<JavaObject> composition_layer_get_android_surface(RID p_layer);

	static bool is_available(XrStructureType p_which);

	static bool is_android_surface_swapchain_available()
	{
		return data != nullptr && data->android_surface_ext_available;
	}

private:
	static void _composition_layer_free_rt(RID p_layer);
	static void _composition_layer_register_rt(RID p_layer);
	static void _composition_layer_unregister_rt(RID p_layer);

#ifdef ANDROID_ENABLED
	static bool create_android_surface_swapchain(
		XrSwapchainCreateInfo* p_info, XrSwapchain* r_swapchain, jobject* r_surface);

	EXT_PROTO_XRRESULT_FUNC1(xrDestroySwapchain, (XrSwapchain), swapchain)
	EXT_PROTO_XRRESULT_FUNC4(xrCreateSwapchainAndroidSurfaceKHR, (XrSession), session,
		(const XrSwapchainCreateInfo*), info, (XrSwapchain*), swapchain, (jobject*), surface)
#endif

	struct CompositionLayer
	{
		union
		{
			XrCompositionLayerBaseHeader composition_layer;
			XrCompositionLayerQuad composition_layer_quad;
			XrCompositionLayerCylinderKHR composition_layer_cylinder;
			XrCompositionLayerEquirect2KHR composition_layer_equirect;
		};

		int sort_order = 1;
		bool alpha_blend = false;
		bool extension_property_values_changed = true;

		struct
		{
			RID viewport;
			Size2i viewport_size;
			OpenXRAPI::OpenXRSwapChainInfo swapchain_info;
			bool static_image = false;
			bool swapchain_protected_content = false;
		} subviewport;

#ifdef ANDROID_ENABLED
		struct
		{
			XrSwapchain swapchain = XR_NULL_HANDLE;
			Ref<JavaObject> surface;
		} android_surface;
#endif

		PoseSpace pose_space = POSE_WORLD_LOCKED;
		XrSpace layer_reference_space = XR_NULL_HANDLE;

		bool use_android_surface = false;
		bool protected_content = false;
		Size2i swapchain_size;

		SwapchainState swapchain_state;
		bool swapchain_state_is_dirty = false;

		void set_viewport(RID p_viewport, const Size2i& p_size);
		void set_use_android_surface(bool p_use_android_surface, const Size2i& p_size);

		void set_sort_order(int p_sort_order) { sort_order = p_sort_order; }

		void set_alpha_blend(bool p_alpha_blend);

		void set_protected_content(bool p_protected_content)
		{
			protected_content = p_protected_content;
		}

		void set_transform(const Transform3D& p_transform);

		void set_min_filter(Filter p_mode);
		void set_mag_filter(Filter p_mode);
		void set_mipmap_mode(MipmapMode p_mode);
		void set_horizontal_wrap(Wrap p_mode);
		void set_vertical_wrap(Wrap p_mode);
		void set_red_swizzle(Swizzle p_mode);
		void set_green_swizzle(Swizzle p_mode);
		void set_blue_swizzle(Swizzle p_mode);
		void set_alpha_swizzle(Swizzle p_mode);
		void set_max_anisotropy(float p_value);
		void set_border_color(const Color& p_color);
		void set_pose_space(PoseSpace p_pose_space);
		void set_eye_visibility(EyeVisibility p_eye_visibility);

		void set_quad_size(const Size2& p_size);

		void set_cylinder_radius(float p_radius);
		void set_cylinder_aspect_ratio(float p_aspect_ratio);
		void set_cylinder_central_angle(float p_central_angle);

		void set_equirect_radius(float p_radius);
		void set_equirect_central_horizontal_angle(float p_angle);
		void set_equirect_upper_vertical_angle(float p_angle);
		void set_equirect_lower_vertical_angle(float p_angle);

		Ref<JavaObject> get_android_surface();
		void on_pre_render();
		XrCompositionLayerBaseHeader* get_composition_layer();
		void free();

	private:
		void update_swapchain_state();
		void update_swapchain_sub_image(XrSwapchainSubImage& r_subimage);
		bool update_and_acquire_swapchain(bool p_static_image);
		RID get_current_swapchain_texture();
		void free_swapchain();

#ifdef ANDROID_ENABLED
		void create_android_surface();
#endif
	};

public:
	struct Data
	{
		bool cylinder_ext_available = false;
		bool equirect_ext_available = false;
		bool android_surface_ext_available = false;
		Mutex composition_layer_mutex;
		RID_Owner<CompositionLayer, true> composition_layer_owner;
		LocalVector<CompositionLayer*> registered_composition_layers;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRCompositionLayerExtension() = delete;
	OpenXRCompositionLayerExtension(const OpenXRCompositionLayerExtension&) = delete;
	OpenXRCompositionLayerExtension& operator=(const OpenXRCompositionLayerExtension&) = delete;
	~OpenXRCompositionLayerExtension() = delete;
};

///////////////////////////////////////////////////////////////////////////
// OpenXRFBUpdateSwapchainExtension

class OpenXRFBUpdateSwapchainExtension final
{
protected:
	friend class OpenXRFBFoveationExtension;

public:
	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);

	static void on_instance_created(const XrInstance p_instance);
	static void on_instance_destroyed();

	static bool is_enabled();
	static bool is_android_ext_enabled();

	static void update_swapchain_state(XrSwapchain p_swapchain,
		const OpenXRCompositionLayerExtension::SwapchainState* p_swapchain_state);

	static void update_swapchain_surface_size(XrSwapchain p_swapchain, const Size2i& p_size);

private:
	static uint32_t filter_to_gl(OpenXRCompositionLayerExtension::Filter p_filter,
		OpenXRCompositionLayerExtension::MipmapMode p_mipmap_mode =
			OpenXRCompositionLayerExtension::MipmapMode::MIPMAP_MODE_DISABLED);
	static uint32_t wrap_to_gl(OpenXRCompositionLayerExtension::Wrap p_wrap);
	static uint32_t swizzle_to_gl(OpenXRCompositionLayerExtension::Swizzle p_swizzle);

	static uint32_t filter_to_vk(OpenXRCompositionLayerExtension::Filter p_filter);
	static uint32_t mipmap_mode_to_vk(OpenXRCompositionLayerExtension::MipmapMode p_mipmap);
	static uint32_t wrap_to_vk(OpenXRCompositionLayerExtension::Wrap p_wrap);
	static uint32_t swizzle_to_vk(OpenXRCompositionLayerExtension::Swizzle p_swizzle);

	EXT_PROTO_XRRESULT_FUNC2(xrUpdateSwapchainFB, (XrSwapchain), swapchain,
		(const XrSwapchainStateBaseHeaderFB*), state);
	EXT_PROTO_XRRESULT_FUNC2(
		xrGetSwapchainStateFB, (XrSwapchain), swapchain, (XrSwapchainStateBaseHeaderFB*), state);

public:
	struct Data
	{
		String rendering_driver;
		bool fb_swapchain_update_state_ext = false;
		bool fb_swapchain_update_state_vulkan_ext = false;
		bool fb_swapchain_update_state_opengles_ext = false;
		bool fb_swapchain_update_state_android_ext = false;
	};

	static inline Data* data = nullptr;

	static void initialize(const String& p_rendering_driver = String());
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRFBUpdateSwapchainExtension() = delete;
	OpenXRFBUpdateSwapchainExtension(const OpenXRFBUpdateSwapchainExtension&) = delete;
	OpenXRFBUpdateSwapchainExtension& operator=(const OpenXRFBUpdateSwapchainExtension&) = delete;
	~OpenXRFBUpdateSwapchainExtension() = delete;
};

///////////////////////////////////////////////////////////////////////////
// OpenXRFBFoveationExtension

class OpenXRFBFoveationExtension final
{
public:
	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);

	static void on_instance_created(const XrInstance p_instance);
	static void on_instance_destroyed();

	static void* set_system_properties_and_get_next_pointer(void* p_next_pointer);
	static void* set_swapchain_create_info_and_get_next_pointer(void* p_next_pointer);

	static void on_main_swapchains_created();

	static bool is_enabled();

	static XrFoveationLevelFB get_foveation_level();
	static void set_foveation_level(XrFoveationLevelFB p_foveation_level);

	static XrFoveationDynamicFB get_foveation_dynamic();
	static void set_foveation_dynamic(XrFoveationDynamicFB p_foveation_dynamic);

	static bool is_foveation_eye_tracked_enabled();
	static void get_fragment_density_offsets(LocalVector<Vector2i>& r_offsets);

	static void set_foveation_with_subsampled_images_enabled(bool p_enabled);
	static bool is_foveation_with_subsampled_images_enabled();

	static void set_foveation_with_subsampled_images_active(bool p_active);

private:
	static void _update_profile_rt();
	static void update_profile();

	EXT_PROTO_XRRESULT_FUNC3(xrCreateFoveationProfileFB, (XrSession), session,
		(const XrFoveationProfileCreateInfoFB*), create_info, (XrFoveationProfileFB*), profile);
	EXT_PROTO_XRRESULT_FUNC1(xrDestroyFoveationProfileFB, (XrFoveationProfileFB), profile);
	EXT_PROTO_XRRESULT_FUNC2(xrGetFoveationEyeTrackedStateMETA, (XrSession), session,
		(XrFoveationEyeTrackedStateMETA*), foveationState);

public:
	struct Data;

	static inline Data* data = nullptr;

	static void initialize(const String& p_rendering_driver = String());
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRFBFoveationExtension() = delete;
	OpenXRFBFoveationExtension(const OpenXRFBFoveationExtension&) = delete;
	OpenXRFBFoveationExtension& operator=(const OpenXRFBFoveationExtension&) = delete;
	~OpenXRFBFoveationExtension() = delete;
};

///////////////////////////////////////////////////////////////////////////
// OpenXRFrameSynthesisExtension

class OpenXRFrameSynthesisExtension final
{
public:
	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);

	static void on_instance_created(const XrInstance p_instance);
	static void on_instance_destroyed();

	static void prepare_view_configuration(uint32_t p_view_count);
	static void* set_view_configuration_and_get_next_pointer(
		uint32_t p_view, void* p_next_pointer);
	static void print_view_configuration_info(uint32_t p_view);

	static void on_session_destroyed();

	static void on_main_swapchains_created();
	static void on_pre_draw_viewport(RID p_render_target);
	static void on_post_draw_viewport(RID p_render_target);
	static void* set_projection_views_and_get_next_pointer(int p_view_index, void* p_next_pointer);

	static bool is_available();

	static bool is_enabled();
	static void set_enabled(bool p_enabled);

	static bool get_relax_frame_interval();
	static void set_relax_frame_interval(bool p_relax_frame_interval);

	static void skip_next_frame();

protected:
	static void _set_render_state_enabled_rt(bool p_enabled);
	static void _set_relax_frame_interval_rt(bool p_relax_frame_interval);
	static void _set_skip_next_frame_rt();

private:
	enum SwapchainTypes
	{
		SWAPCHAIN_MOTION_VECTOR,
		SWAPCHAIN_DEPTH,
		SWAPCHAIN_MAX
	};

	static void free_swapchains();

	struct RenderState
	{
		bool enabled = true;
		bool relax_frame_interval = false;
		bool skip_next_frame = false;

		LocalVector<XrFrameSynthesisConfigViewEXT> config_views;
		OpenXRAPI::OpenXRSwapChainInfo swapchains[SWAPCHAIN_MAX];
		LocalVector<XrFrameSynthesisInfoEXT> frame_synthesis_info;
		Transform3D previous_transform;
	};

public:
	struct Data
	{
		bool frame_synthesis_ext = false;
		bool enabled = true;
		bool relax_frame_interval = false;
		RenderState render_state;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRFrameSynthesisExtension() = delete;
	OpenXRFrameSynthesisExtension(const OpenXRFrameSynthesisExtension&) = delete;
	OpenXRFrameSynthesisExtension& operator=(const OpenXRFrameSynthesisExtension&) = delete;
	~OpenXRFrameSynthesisExtension() = delete;
};

///////////////////////////////////////////////////////////////////////////
// OpenXRFutureResult & OpenXRFutureExtension

class OpenXRFutureExtension;

class OpenXRFutureResult final : public RefCounted
{
	friend class OpenXRFutureExtension;

protected:
	void _mark_as_finished();
	void _mark_as_cancelled();

public:
	enum ResultStatus
	{
		RESULT_RUNNING,
		RESULT_FINISHED,
		RESULT_CANCELLED,
	};

	ResultStatus get_status() const;
	XrFutureEXT get_future() const;

	void cancel_future();

private:
	ResultStatus status = RESULT_RUNNING;
	XrFutureEXT future;

	uint64_t _get_future() const;
};

class OpenXRFutureExtension final
{
public:
	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);

	static void on_instance_created(const XrInstance p_instance);
	static void on_instance_destroyed();
	static void on_session_destroyed();

	static void on_process();

	static bool is_active();

	static void cancel_future(XrFutureEXT p_future);

private:
	static void _cancel_future(uint64_t p_future);

	EXT_PROTO_XRRESULT_FUNC3(xrPollFutureEXT, (XrInstance), instance, (const XrFuturePollInfoEXT*),
		poll_info, (XrFuturePollResultEXT*), poll_result);
	EXT_PROTO_XRRESULT_FUNC2(
		xrCancelFutureEXT, (XrInstance), instance, (const XrFutureCancelInfoEXT*), cancel_info);

public:
	struct Data
	{
		bool future_ext = false;
		HashMap<XrFutureEXT, Ref<OpenXRFutureResult>> futures;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRFutureExtension() = delete;
	OpenXRFutureExtension(const OpenXRFutureExtension&) = delete;
	OpenXRFutureExtension& operator=(const OpenXRFutureExtension&) = delete;
	~OpenXRFutureExtension() = delete;
};

///////////////////////////////////////////////////////////////////////////
// OpenXRHandTrackingExtension

class OpenXRHandTrackingExtension final
{
public:
	enum HandTrackedHands
	{
		OPENXR_TRACKED_LEFT_HAND,
		OPENXR_TRACKED_RIGHT_HAND,
		OPENXR_MAX_TRACKED_HANDS
	};

	enum HandTrackedSource
	{
		OPENXR_SOURCE_UNKNOWN,
		OPENXR_SOURCE_UNOBSTRUCTED,
		OPENXR_SOURCE_CONTROLLER,
		OPENXR_SOURCE_NOT_TRACKED,
		OPENXR_SOURCE_MAX
	};

	struct HandTracker
	{
		bool is_initialized = false;
		Ref<XRTracker> godot_tracker;
		HandTrackedSource source = OPENXR_SOURCE_UNKNOWN;
	};

	static void on_instance_destroyed();
	static void on_session_destroyed();

	static void* set_system_properties_and_get_next_pointer(void* p_next_pointer);
	static void on_state_ready();
	static void on_process();
	static void on_state_stopping();

	static bool get_active();
	static const HandTracker* get_hand_tracker(HandTrackedHands p_hand);

private:
	static void cleanup_hand_tracking();

public:
	struct Data
	{
		HandTracker hand_trackers[OPENXR_MAX_TRACKED_HANDS];
		bool hand_tracking_ext = false;
		bool hand_motion_range_ext = false;
		bool hand_tracking_source_ext = false;
		bool unobstructed_data_source = false;
		bool controller_data_source = false;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRHandTrackingExtension() = delete;
	OpenXRHandTrackingExtension(const OpenXRHandTrackingExtension&) = delete;
	OpenXRHandTrackingExtension& operator=(const OpenXRHandTrackingExtension&) = delete;
	~OpenXRHandTrackingExtension() = delete;
};

///////////////////////////////////////////////////////////////////////////
// OpenXRRenderModelData & OpenXRRenderModelExtension

#ifdef MODULE_GLTF_ENABLED
class OpenXRRenderModelData final : public RefCounted
{
private:
	Ref<GLTFDocument> gltf_document;
	Ref<GLTFState> gltf_state;
	PackedStringArray node_names;

public:
	Ref<GLTFState> get_gltf_state();

	bool parse_gltf_document(const PackedByteArray& p_bytes);
	Node3D* new_scene_instance();

	void set_node_names(const PackedStringArray& p_node_names);
	PackedStringArray get_node_names() const;
	const String get_node_name(uint32_t p_node_index) const;

	OpenXRRenderModelData();
	~OpenXRRenderModelData();
};

class OpenXRRenderModelExtension final
{
public:
	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);

	static void on_instance_created(const XrInstance p_instance);
	static void on_session_created(const XrSession p_session);
	static void on_instance_destroyed();
	static void on_session_destroyed();

	static bool on_event_polled(const XrEventDataBuffer& event);
	static void on_sync_actions();

	static bool is_active();

	// Render model.
	static bool has_render_model(RID p_render_model);
	static RID render_model_create(XrRenderModelIdEXT p_render_model_id);
	static void render_model_destroy(RID p_render_model);

	static TypedArray<RID> render_model_get_all();
	static Node3D* render_model_new_scene_instance(RID p_render_model);
	static PackedStringArray render_model_get_subaction_paths(RID p_render_model);
	static XrPath render_model_get_top_level_path(RID p_render_model);
	static String render_model_get_top_level_path_as_string(RID p_render_model);
	static XRPose::TrackingConfidence render_model_get_confidence(RID p_render_model);
	static Transform3D render_model_get_root_transform(RID p_render_model);
	static uint32_t render_model_get_animatable_node_count(RID p_render_model);
	static String render_model_get_animatable_node_name(RID p_render_model, uint32_t p_index);
	static bool render_model_is_animatable_node_visible(RID p_render_model, uint32_t p_index);
	static Transform3D render_model_get_animatable_node_transform(
		RID p_render_model, uint32_t p_index);

private:
	static void _clear_interaction_data();
	static bool _update_interaction_data();

	struct RenderModel
	{
		XrRenderModelIdEXT xr_render_model_id = XR_NULL_RENDER_MODEL_ID_EXT;
		XrRenderModelEXT xr_render_model = XR_NULL_HANDLE;
		uint32_t animatable_node_count = 0;
		Ref<OpenXRRenderModelData> render_model_data;
		XrSpace xr_space = XR_NULL_HANDLE;
		XRPose::TrackingConfidence confidence =
			XRPose::TrackingConfidence::XR_TRACKING_CONFIDENCE_NONE;
		Transform3D root_transform;
		LocalVector<XrRenderModelNodeStateEXT> node_states;
		XrPath top_level_path = XR_NULL_PATH;
	};

	static Ref<OpenXRRenderModelData> _get_render_model_data(
		XrUuidEXT p_cache_id, uint32_t p_animatable_node_count);
	static Ref<OpenXRRenderModelData> _load_asset(
		XrRenderModelAssetEXT p_asset, uint32_t p_animatable_node_count);
	static void _clear_render_model_data();

	static RID _render_model_create(uint64_t p_render_model_id);

	EXT_PROTO_XRRESULT_FUNC3(xrCreateRenderModelEXT, (XrSession), session,
		(const XrRenderModelCreateInfoEXT*), createInfo, (XrRenderModelEXT*), renderModel);
	EXT_PROTO_XRRESULT_FUNC1(xrDestroyRenderModelEXT, (XrRenderModelEXT), renderModel);
	EXT_PROTO_XRRESULT_FUNC3(xrGetRenderModelPropertiesEXT, (XrRenderModelEXT), renderModel,
		(const XrRenderModelPropertiesGetInfoEXT*), getInfo, (XrRenderModelPropertiesEXT*),
		properties);
	EXT_PROTO_XRRESULT_FUNC3(xrCreateRenderModelSpaceEXT, (XrSession), session,
		(const XrRenderModelSpaceCreateInfoEXT*), createInfo, (XrSpace*), space);
	EXT_PROTO_XRRESULT_FUNC3(xrCreateRenderModelAssetEXT, (XrSession), session,
		(const XrRenderModelAssetCreateInfoEXT*), createInfo, (XrRenderModelAssetEXT*), asset);
	EXT_PROTO_XRRESULT_FUNC1(xrDestroyRenderModelAssetEXT, (XrRenderModelAssetEXT), asset);
	EXT_PROTO_XRRESULT_FUNC3(xrGetRenderModelAssetDataEXT, (XrRenderModelAssetEXT), asset,
		(const XrRenderModelAssetDataGetInfoEXT*), getInfo, (XrRenderModelAssetDataEXT*), buffer);
	EXT_PROTO_XRRESULT_FUNC3(xrGetRenderModelAssetPropertiesEXT, (XrRenderModelAssetEXT), asset,
		(const XrRenderModelAssetPropertiesGetInfoEXT*), getInfo,
		(XrRenderModelAssetPropertiesEXT*), properties);
	EXT_PROTO_XRRESULT_FUNC3(xrGetRenderModelStateEXT, (XrRenderModelEXT), renderModel,
		(const XrRenderModelStateGetInfoEXT*), getInfo, (XrRenderModelStateEXT*), state);
	EXT_PROTO_XRRESULT_FUNC5(xrEnumerateInteractionRenderModelIdsEXT, (XrSession), session,
		(const XrInteractionRenderModelIdsEnumerateInfoEXT*), getInfo, (uint32_t),
		renderModelIdCapacityInput, (uint32_t*), renderModelIdCountOutput, (XrRenderModelIdEXT*),
		renderModelIds);
	EXT_PROTO_XRRESULT_FUNC5(xrEnumerateRenderModelSubactionPathsEXT, (XrRenderModelEXT),
		renderModel, (const XrInteractionRenderModelSubactionPathInfoEXT*), info, (uint32_t),
		pathCapacityInput, (uint32_t*), pathCountOutput, (XrPath*), paths);
	EXT_PROTO_XRRESULT_FUNC3(xrGetRenderModelPoseTopLevelUserPathEXT, (XrRenderModelEXT),
		renderModel, (const XrInteractionRenderModelTopLevelUserPathGetInfoEXT*), info, (XrPath*),
		topLevelUserPath);

	EXT_PROTO_XRRESULT_FUNC4(xrLocateSpace, (XrSpace), space, (XrSpace), baseSpace, (XrTime), time,
		(XrSpaceLocation*), location);
	EXT_PROTO_XRRESULT_FUNC1(xrDestroySpace, (XrSpace), space);
	EXT_PROTO_XRRESULT_FUNC5(xrPathToString, (XrInstance), instance, (XrPath), path, (uint32_t),
		bufferCapacityInput, (uint32_t*), bufferCountOutput, (char*), buffer);

public:
	struct Data
	{
		bool uuid_ext = false;
		bool render_model_ext = false;
		bool interaction_render_model_ext = false;
		bool xr_sync_has_run = false;
		bool _interaction_data_dirty = true;
		HashMap<XrRenderModelIdEXT, RID> interaction_render_models;
		Vector<XrPath> toplevel_paths;
		RID_Owner<RenderModel, true> render_model_owner;
		HashMap<XrUuidEXT, Ref<OpenXRRenderModelData>, HashMapHasherXrUuidEXT>
			render_model_data_cache;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRRenderModelExtension() = delete;
	OpenXRRenderModelExtension(const OpenXRRenderModelExtension&) = delete;
	OpenXRRenderModelExtension& operator=(const OpenXRRenderModelExtension&) = delete;
	~OpenXRRenderModelExtension() = delete;
};
#endif // MODULE_GLTF_ENABLED

// --- Folded from modules/openxr/extensions/openxr_interaction_profile_extension.h ---
#include <openxr/openxr.h>
#include "modules/openxr/openxr_api.h"
#include "core/templates/hash_map.h"
#include "core/types.h"

class OpenXRInteractionProfileExtension final
{
public:
	enum ProfileKind
	{
		PROFILE_HTC_CONTROLLER = 0,
		PROFILE_HTC_VIVE_TRACKER,
		PROFILE_HUAWEI_CONTROLLER,
		PROFILE_KHR_GENERIC_CONTROLLER,
		PROFILE_META_CONTROLLER,
		PROFILE_ML2_CONTROLLER,
		PROFILE_MXINK,
		PROFILE_PALM_POSE,
		PROFILE_PICO_CONTROLLER,
		PROFILE_VALVE_CONTROLLER,
		PROFILE_WMR_CONTROLLER,
		PROFILE_HAND_INTERACTION,
		PROFILE_LOCAL_FLOOR,
		PROFILE_DPAD_BINDING,
		PROFILE_VALVE_ANALOG_THRESHOLD,
		PROFILE_COMPOSITION_LAYER_DEPTH,
		PROFILE_MAX
	};

	enum HTCControllers
	{
		HTC_VIVE_COSMOS = 0,
		HTC_VIVE_FOCUS3 = 1,
		HTC_HAND_INTERACTION = 2,
		HTC_MAX_CONTROLLERS = 3
	};

	enum MetaControllers
	{
		META_TOUCH_PROXIMITY = 0,
		META_TOUCH_PRO = 1,
		META_TOUCH_PLUS = 2,
		META_MAX_CONTROLLERS = 3
	};

	enum WMRControllers
	{
		WMR_HPMR = 0,
		WMR_SAMSUNG_ODESSY = 1,
		WMR_HAND_INTERACTION = 2,
		WMR_MAX_CONTROLLERS = 3
	};

private:
	ProfileKind kind = PROFILE_KHR_GENERIC_CONTROLLER;
	bool flags[3] = {false, false, false};

public:
	explicit OpenXRInteractionProfileExtension(
		ProfileKind p_kind = PROFILE_KHR_GENERIC_CONTROLLER);

	ProfileKind get_kind() const { return kind; }

	HashMap<String, bool*> get_requested_extensions(XrVersion p_version);
	PackedStringArray get_suggested_tracker_names() const;
	bool on_event_polled(const XrEventDataBuffer& p_event);
	void on_register_metadata();

	bool is_available(int p_subindex = 0) const;

	static inline OpenXRInteractionProfileExtension* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	static OpenXRInteractionProfileExtension* get_profile(ProfileKind p_kind);
	static bool is_profile_available(ProfileKind p_kind, int p_subindex = 0);
	static HashMap<String, bool*> get_all_requested_extensions(XrVersion p_version);
};

class OpenXRDpadBindingModifier final : public OpenXRBindingModifier
{
private:
	PackedByteArray dpad_bindings_data;
	XrInteractionProfileDpadBindingEXT* dpad_bindings = nullptr;
	String input_path;
	Ref<OpenXRActionSet> action_set;
	Ref<OpenXRHapticBase> on_haptic;
	Ref<OpenXRHapticBase> off_haptic;

public:
	OpenXRDpadBindingModifier();

	void set_action_set(const Ref<OpenXRActionSet>& p_action_set);
	Ref<OpenXRActionSet> get_action_set() const;

	void set_input_path(const String& p_input_path);
	String get_input_path() const;

	void set_threshold(float p_threshold);
	float get_threshold() const;

	void set_threshold_released(float p_threshold);
	float get_threshold_released() const;

	void set_center_region(float p_center_region);
	float get_center_region() const;

	void set_wedge_angle(float p_wedge_angle);
	float get_wedge_angle() const;

	void set_wedge_angle_deg(float p_wedge_angle);
	float get_wedge_angle_deg() const;

	void set_is_sticky(bool p_sticky);
	bool get_is_sticky() const;

	void set_on_haptic(const Ref<OpenXRHapticBase>& p_haptic);
	Ref<OpenXRHapticBase> get_on_haptic() const;

	void set_off_haptic(const Ref<OpenXRHapticBase>& p_haptic);
	Ref<OpenXRHapticBase> get_off_haptic() const;

	virtual String get_description() const override { return "DPad modifier"; }
	virtual PackedByteArray get_ip_modification() override;
};

class OpenXRAnalogThresholdModifier final : public OpenXRBindingModifier
{
private:
	XrInteractionProfileAnalogThresholdVALVE analog_threshold;
	Ref<OpenXRHapticBase> on_haptic;
	Ref<OpenXRHapticBase> off_haptic;

public:
	OpenXRAnalogThresholdModifier();

	void set_on_threshold(float p_threshold);
	float get_on_threshold() const;

	void set_off_threshold(float p_threshold);
	float get_off_threshold() const;

	void set_on_haptic(const Ref<OpenXRHapticBase>& p_haptic);
	Ref<OpenXRHapticBase> get_on_haptic() const;

	void set_off_haptic(const Ref<OpenXRHapticBase>& p_haptic);
	Ref<OpenXRHapticBase> get_off_haptic() const;

	virtual String get_description() const override { return "Analog threshold modifier"; }
	virtual PackedByteArray get_ip_modification() override;
};

// --- End modules/openxr/extensions/openxr_interaction_profile_extension.h ---

// --- Folded from modules/openxr/extensions/spatial_entities/openxr_spatial_entities.h ---
#include <openxr/openxr.h>
#include "modules/openxr/openxr_api.h"

#include "core/templates/rid_owner.h"
#include "scene/resources/mesh.h"
#include "servers/xr/xr_server.h"

#ifndef PHYSICS_3D_DISABLED
#include "scene/resources/mesh.h"
#endif

#define XR_NULL_ENTITY 0x7FFFFFFF

// Persistence configuration (folded into OpenXRStructureBase)
using OpenXRSpatialContextPersistenceConfig = OpenXRStructureBase;

// Consolidated capability configuration wrapper for XrSpatialCapabilityConfigurationBaseHeaderEXT
class OpenXRSpatialCapabilityConfigurationBaseHeader final : public RefCounted
{
public:
	enum CapabilityType
	{
		CAPABILITY_NONE,
		CAPABILITY_ANCHOR,
		CAPABILITY_PLANE_TRACKING,
		CAPABILITY_QR_CODE,
		CAPABILITY_MICRO_QR_CODE,
		CAPABILITY_ARUCO,
		CAPABILITY_APRIL_TAG,
	};

	enum ArucoDict
	{
		ARUCO_DICT_4X4_50 = XR_SPATIAL_MARKER_ARUCO_DICT_4X4_50_EXT,
		ARUCO_DICT_4X4_100 = XR_SPATIAL_MARKER_ARUCO_DICT_4X4_100_EXT,
		ARUCO_DICT_4X4_250 = XR_SPATIAL_MARKER_ARUCO_DICT_4X4_250_EXT,
		ARUCO_DICT_4X4_1000 = XR_SPATIAL_MARKER_ARUCO_DICT_4X4_1000_EXT,
		ARUCO_DICT_5X5_50 = XR_SPATIAL_MARKER_ARUCO_DICT_5X5_50_EXT,
		ARUCO_DICT_5X5_100 = XR_SPATIAL_MARKER_ARUCO_DICT_5X5_100_EXT,
		ARUCO_DICT_5X5_250 = XR_SPATIAL_MARKER_ARUCO_DICT_5X5_250_EXT,
		ARUCO_DICT_5X5_1000 = XR_SPATIAL_MARKER_ARUCO_DICT_5X5_1000_EXT,
		ARUCO_DICT_6X6_50 = XR_SPATIAL_MARKER_ARUCO_DICT_6X6_50_EXT,
		ARUCO_DICT_6X6_100 = XR_SPATIAL_MARKER_ARUCO_DICT_6X6_100_EXT,
		ARUCO_DICT_6X6_250 = XR_SPATIAL_MARKER_ARUCO_DICT_6X6_250_EXT,
		ARUCO_DICT_6X6_1000 = XR_SPATIAL_MARKER_ARUCO_DICT_6X6_1000_EXT,
		ARUCO_DICT_7X7_50 = XR_SPATIAL_MARKER_ARUCO_DICT_7X7_50_EXT,
		ARUCO_DICT_7X7_100 = XR_SPATIAL_MARKER_ARUCO_DICT_7X7_100_EXT,
		ARUCO_DICT_7X7_250 = XR_SPATIAL_MARKER_ARUCO_DICT_7X7_250_EXT,
		ARUCO_DICT_7X7_1000 = XR_SPATIAL_MARKER_ARUCO_DICT_7X7_1000_EXT,
	};

	enum AprilTagDict
	{
		APRIL_TAG_DICT_16H5 = XR_SPATIAL_MARKER_APRIL_TAG_DICT_16H5_EXT,
		APRIL_TAG_DICT_25H9 = XR_SPATIAL_MARKER_APRIL_TAG_DICT_25H9_EXT,
		APRIL_TAG_DICT_36H10 = XR_SPATIAL_MARKER_APRIL_TAG_DICT_36H10_EXT,
		APRIL_TAG_DICT_36H11 = XR_SPATIAL_MARKER_APRIL_TAG_DICT_36H11_EXT,
	};

	OpenXRSpatialCapabilityConfigurationBaseHeader(CapabilityType p_capability_type = CAPABILITY_NONE);
	~OpenXRSpatialCapabilityConfigurationBaseHeader();

	void set_capability_type(CapabilityType p_capability_type);
	CapabilityType get_capability_type() const { return capability_type; }

	bool has_valid_configuration() const;
	uint64_t _get_configurationgd();
	XrSpatialCapabilityConfigurationBaseHeaderEXT* get_configuration();

	Vector<XrSpatialComponentTypeEXT> get_enabled_components() const { return enabled_components; }

	// Plane tracking configuration queries
	bool get_supports_mesh_2d();
	bool get_supports_polygons();
	bool get_supports_labels();

	// ArUco dictionary configuration
	void set_aruco_dict(XrSpatialMarkerArucoDictEXT p_dict);
	XrSpatialMarkerArucoDictEXT get_aruco_dict() const;

	// AprilTag dictionary configuration
	void set_april_dict(XrSpatialMarkerAprilTagDictEXT p_dict);
	XrSpatialMarkerAprilTagDictEXT get_april_dict() const;

private:
	CapabilityType capability_type = CAPABILITY_NONE;
	int supports_mesh_2d = -1;
	int supports_polygons = -1;
	int supports_labels = -1;
	XrSpatialMarkerArucoDictEXT aruco_dict = XR_SPATIAL_MARKER_ARUCO_DICT_7X7_1000_EXT;
	XrSpatialMarkerAprilTagDictEXT april_dict = XR_SPATIAL_MARKER_APRIL_TAG_DICT_36H11_EXT;

	Vector<XrSpatialComponentTypeEXT> enabled_components;

	union ConfigPayload
	{
		XrSpatialCapabilityConfigurationBaseHeaderEXT base;
		XrSpatialCapabilityConfigurationAnchorEXT anchor;
		XrSpatialCapabilityConfigurationPlaneTrackingEXT plane;
		XrSpatialCapabilityConfigurationQrCodeEXT qrcode;
		XrSpatialCapabilityConfigurationMicroQrCodeEXT micro_qrcode;
		XrSpatialCapabilityConfigurationArucoMarkerEXT aruco;
		XrSpatialCapabilityConfigurationAprilTagEXT april_tag;

		ConfigPayload()
			: base{XR_TYPE_UNKNOWN, nullptr, XR_SPATIAL_CAPABILITY_MAX_ENUM_EXT, 0, nullptr}
		{
		}
	} config;

	PackedInt64Array _get_enabled_components() const;
	void _set_aruco_dict(ArucoDict p_dict);
	ArucoDict _get_aruco_dict() const;
	void _set_april_dict(AprilTagDict p_dict);
	AprilTagDict _get_april_dict() const;
};

// Consolidated wrapper class for spatial component data returned by discovery and update queries
class OpenXRSpatialComponentData final : public RefCounted
{
public:
	enum ComponentKind
	{
		COMPONENT_NONE,
		COMPONENT_QUERY_RESULT,
		COMPONENT_BOUNDED_2D,
		COMPONENT_BOUNDED_3D,
		COMPONENT_PARENT,
		COMPONENT_MESH_2D,
		COMPONENT_MESH_3D,
		COMPONENT_ANCHOR,
		COMPONENT_PERSISTENCE,
		COMPONENT_PLANE_ALIGNMENT,
		COMPONENT_POLYGON_2D,
		COMPONENT_PLANE_SEMANTIC_LABEL,
		COMPONENT_MARKER,
	};

	enum PlaneAlignment
	{
		PLANE_ALIGNMENT_HORIZONTAL_UPWARD = XR_SPATIAL_PLANE_ALIGNMENT_HORIZONTAL_UPWARD_EXT,
		PLANE_ALIGNMENT_HORIZONTAL_DOWNWARD = XR_SPATIAL_PLANE_ALIGNMENT_HORIZONTAL_DOWNWARD_EXT,
		PLANE_ALIGNMENT_VERTICAL = XR_SPATIAL_PLANE_ALIGNMENT_VERTICAL_EXT,
		PLANE_ALIGNMENT_ARBITRARY = XR_SPATIAL_PLANE_ALIGNMENT_ARBITRARY_EXT,
	};

	enum PlaneSemanticLabel
	{
		PLANE_SEMANTIC_LABEL_UNCATEGORIZED = XR_SPATIAL_PLANE_SEMANTIC_LABEL_UNCATEGORIZED_EXT,
		PLANE_SEMANTIC_LABEL_FLOOR = XR_SPATIAL_PLANE_SEMANTIC_LABEL_FLOOR_EXT,
		PLANE_SEMANTIC_LABEL_WALL = XR_SPATIAL_PLANE_SEMANTIC_LABEL_WALL_EXT,
		PLANE_SEMANTIC_LABEL_CEILING = XR_SPATIAL_PLANE_SEMANTIC_LABEL_CEILING_EXT,
		PLANE_SEMANTIC_LABEL_TABLE = XR_SPATIAL_PLANE_SEMANTIC_LABEL_TABLE_EXT,
	};

	enum MarkerType
	{
		MARKER_TYPE_UNKNOWN,
		MARKER_TYPE_QRCODE,
		MARKER_TYPE_MICRO_QRCODE,
		MARKER_TYPE_ARUCO,
		MARKER_TYPE_APRIL_TAG,
		MARKER_TYPE_MAX
	};

	OpenXRSpatialComponentData(ComponentKind p_kind = COMPONENT_NONE);
	~OpenXRSpatialComponentData();

	OpenXRSpatialComponentData(const OpenXRSpatialComponentData&) = delete;
	OpenXRSpatialComponentData& operator=(const OpenXRSpatialComponentData&) = delete;

	void set_component_kind(ComponentKind p_kind);
	ComponentKind get_component_kind() const { return component_kind; }

	void set_capacity(uint32_t p_capacity);
	int64_t get_capacity() const { return capacity; }

	int64_t _get_component_typegd() const;
	XrSpatialComponentTypeEXT get_component_type() const;
	void* get_structure_data(void* p_next);

	// COMPONENT_QUERY_RESULT accessors
	XrSpatialEntityIdEXT get_entity_id(int64_t p_index) const;
	XrSpatialEntityTrackingStateEXT get_entity_state(int64_t p_index) const;
	static String get_entity_tracking_state_name(XrSpatialEntityTrackingStateEXT p_tracking_state);

	// COMPONENT_BOUNDED_2D / COMPONENT_BOUNDED_3D accessors
	Transform3D get_center_pose(int64_t p_index) const;
	Transform3D get_center_pose_2d(int64_t p_index) const;
	Transform3D get_center_pose_3d(int64_t p_index) const;
	Vector2 get_size_2d(int64_t p_index) const;
	Vector3 get_size_3d(int64_t p_index) const;

	// COMPONENT_PARENT accessors
	RID get_parent(int64_t p_index) const;

	// COMPONENT_MESH_2D / COMPONENT_MESH_3D / COMPONENT_POLYGON_2D accessors
	Transform3D get_transform(int64_t p_index) const;
	PackedVector2Array get_vertices(RID p_snapshot, int64_t p_index) const;
	PackedInt32Array get_indices(RID p_snapshot, int64_t p_index) const;
	Ref<Mesh> get_mesh(int64_t p_index) const;

	// COMPONENT_ANCHOR accessors
	Transform3D get_entity_pose(int64_t p_index) const;

	// COMPONENT_PERSISTENCE accessors
	XrUuid get_persistent_uuid(int64_t p_index) const;
	XrSpatialPersistenceStateEXT get_persistent_state(int64_t p_index) const;
	static String get_persistence_state_name(XrSpatialPersistenceStateEXT p_state);

	// COMPONENT_PLANE_ALIGNMENT accessors
	XrSpatialPlaneAlignmentEXT get_plane_alignment(int64_t p_index) const;

	// COMPONENT_PLANE_SEMANTIC_LABEL accessors
	XrSpatialPlaneSemanticLabelEXT get_plane_semantic_label(int64_t p_index) const;

	// COMPONENT_MARKER accessors
	MarkerType get_marker_type(int64_t p_index) const;
	uint32_t get_marker_id(int64_t p_index) const;
	XrSpatialBufferEXT get_marker_data(int64_t p_index) const;

private:
	ComponentKind component_kind = COMPONENT_NONE;
	uint32_t capacity = 0;
	void* data_ptr = nullptr;
	XrSpatialEntityTrackingStateEXT* entity_states_ptr = nullptr;

	union StructurePayload
	{
		XrSpatialComponentDataQueryResultEXT query_result;
		XrSpatialComponentBounded2DListEXT bounded2d_list;
		XrSpatialComponentBounded3DListEXT bounded3d_list;
		XrSpatialComponentParentListEXT parent_list;
		XrSpatialComponentMesh2DListEXT mesh2d_list;
		XrSpatialComponentMesh3DListEXT mesh3d_list;
		XrSpatialComponentAnchorListEXT anchor_list;
		XrSpatialComponentPersistenceListEXT persistence_list;
		XrSpatialComponentPlaneAlignmentListEXT plane_alignment_list;
		XrSpatialComponentPolygon2DListEXT polygon2d_list;
		XrSpatialComponentPlaneSemanticLabelListEXT plane_semantic_label_list;
		XrSpatialComponentMarkerListEXT marker_list;

		StructurePayload()
			: query_result{XR_TYPE_SPATIAL_COMPONENT_DATA_QUERY_RESULT_EXT, nullptr, 0, 0, nullptr,
				  0, 0, nullptr}
		{
		}
	} structure;

	void _free_buffers();
	void _init_structure_header();

	uint64_t _get_entity_id(int64_t p_index) const;
	String _get_persistent_uuid(int64_t p_index) const;
	uint64_t _get_persistent_state(int64_t p_index) const;
	PlaneAlignment _get_plane_alignment(int64_t p_index) const;
	PlaneSemanticLabel _get_plane_semantic_label(int64_t p_index) const;
};

// Unified tracker for spatial entities (generic, anchor, plane, marker), composing XRTracker
class OpenXRSpatialEntityTracker final : public RefCounted
{
public:
	enum EntityType
	{
		ENTITY_GENERIC,
		ENTITY_ANCHOR,
		ENTITY_PLANE,
		ENTITY_MARKER,
	};

	enum EntityTrackingState
	{
		ENTITY_TRACKING_STATE_STOPPED = XR_SPATIAL_ENTITY_TRACKING_STATE_STOPPED_EXT,
		ENTITY_TRACKING_STATE_PAUSED = XR_SPATIAL_ENTITY_TRACKING_STATE_PAUSED_EXT,
		ENTITY_TRACKING_STATE_TRACKING = XR_SPATIAL_ENTITY_TRACKING_STATE_TRACKING_EXT,
	};

	OpenXRSpatialEntityTracker(EntityType p_entity_type = ENTITY_GENERIC);
	~OpenXRSpatialEntityTracker() override;

	const Ref<XRTracker>& get_xr_tracker() const { return xr_tracker; }

	// Composed XRTracker forwarding helpers
	void set_tracker_type(XRServer::TrackerType p_type) { xr_tracker->set_tracker_type(p_type); }
	XRServer::TrackerType get_tracker_type() const { return xr_tracker->get_tracker_type(); }
	void set_tracker_name(const String& p_name) { xr_tracker->set_tracker_name(p_name); }
	String get_tracker_name() const { return xr_tracker->get_tracker_name(); }
	void set_tracker_desc(const String& p_desc) { xr_tracker->set_tracker_desc(p_desc); }
	String get_tracker_desc() const { return xr_tracker->get_tracker_desc(); }
	void set_has_tracking_data(bool p_has_tracking_data)
	{
		xr_tracker->set_has_tracking_data(p_has_tracking_data);
	}
	bool get_has_tracking_data() const { return xr_tracker->get_has_tracking_data(); }
	bool has_pose(const String& p_action_name) const { return xr_tracker->has_pose(p_action_name); }
	Ref<XRPose> get_pose(const String& p_action_name) const
	{
		return xr_tracker->get_pose(p_action_name);
	}
	void invalidate_pose(const String& p_action_name)
	{
		xr_tracker->invalidate_pose(p_action_name);
	}
	void set_pose(const String& p_action_name, const Transform3D& p_transform,
		const Vector3& p_linear_velocity, const Vector3& p_angular_velocity,
		const XRPose::TrackingConfidence p_tracking_confidence =
			XRPose::XR_TRACKING_CONFIDENCE_HIGH)
	{
		xr_tracker->set_pose(p_action_name, p_transform, p_linear_velocity, p_angular_velocity,
			p_tracking_confidence);
	}

	void set_entity_type(EntityType p_entity_type) { entity_type = p_entity_type; }
	EntityType get_entity_type() const { return entity_type; }

	void set_spatial_context(const RID& p_spatial_context);
	RID get_spatial_context() const;

	void set_entity(const RID& p_entity);
	RID get_entity() const;

	void set_spatial_tracking_state(const XrSpatialEntityTrackingStateEXT p_state);
	XrSpatialEntityTrackingStateEXT get_spatial_tracking_state() const;

	void add_next(Ref<OpenXRStructureBase> p_next);
	void remove_next(Ref<OpenXRStructureBase> p_next);
	Ref<OpenXRStructureBase> get_next() const;

	// Anchor fields & methods
	bool has_uuid() const;
	XrUuid get_uuid() const;
	void set_uuid(const XrUuid& p_uuid);

	// Shared Plane / Marker bounds
	void set_bounds_size(const Vector2& p_bounds_size);
	Vector2 get_bounds_size() const;

	// Plane fields & methods
	void set_plane_alignment(OpenXRSpatialComponentData::PlaneAlignment p_plane_alignment);
	OpenXRSpatialComponentData::PlaneAlignment get_plane_alignment() const;

	void set_plane_label(const String& p_plane_label);
	String get_plane_label() const;

	void set_mesh_data(const Transform3D& p_origin, const PackedVector2Array& p_vertices,
		const PackedInt32Array& p_indices = PackedInt32Array());
	void clear_mesh_data();

	Transform3D get_mesh_offset() const;
	Ref<Mesh> get_mesh() { return mesh.mesh; }
#ifndef PHYSICS_3D_DISABLED
	Ref<Shape3D> get_shape(real_t p_thickness = 0.01);
#endif

	// Marker fields & methods
	void set_marker_type(OpenXRSpatialComponentData::MarkerType p_marker_type);
	OpenXRSpatialComponentData::MarkerType get_marker_type() const;

	void set_marker_id(uint32_t p_id);
	uint32_t get_marker_id() const;

private:
	Ref<XRTracker> xr_tracker;
	EntityType entity_type = ENTITY_GENERIC;
	RID spatial_context;
	RID spatial_entity;
	XrSpatialEntityTrackingStateEXT spatial_tracking_state =
		XR_SPATIAL_ENTITY_TRACKING_STATE_PAUSED_EXT;
	Ref<OpenXRStructureBase> next;

	// Anchor data
	XrUuid uuid = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	// Plane & Marker data
	Vector2 bounds_size;
	OpenXRSpatialComponentData::PlaneAlignment plane_alignment =
		OpenXRSpatialComponentData::PLANE_ALIGNMENT_HORIZONTAL_UPWARD;
	String plane_label;

	struct MeshData
	{
		bool has_mesh_data = false;
		Transform3D origin;
		PackedVector2Array vertices;
		PackedInt32Array indices;

		Ref<Mesh> mesh;
#ifndef PHYSICS_3D_DISABLED
		Ref<Shape3D> shape3d;
#endif
	} mesh;

	struct Edge
	{
		int32_t a;
		int32_t b;

		static _FORCE_INLINE_ uint32_t hash(const Edge& p_edge)
		{
			uint32_t h = hash_murmur3_one_32(p_edge.a);
			return hash_murmur3_one_32(p_edge.b, h);
		}

		bool operator==(const Edge& p_edge) const { return (a == p_edge.a && b == p_edge.b); }

		Edge(int32_t p_a = 0, int32_t p_b = 0)
		{
			a = p_a;
			b = p_b;
			if (a < b) {
				SWAP(a, b);
			}
		}
	};

	OpenXRSpatialComponentData::MarkerType marker_type =
		OpenXRSpatialComponentData::MARKER_TYPE_UNKNOWN;
	uint32_t marker_id = 0;

	void _set_spatial_tracking_state(const EntityTrackingState p_state);
	EntityTrackingState _get_spatial_tracking_state() const;

	String _get_uuid() const;
	void _set_uuid(const String& p_uuid);
	static bool uuid_is_equal(const XrUuid& p_a, const XrUuid& p_b);
};

using OpenXRAnchorTracker = OpenXRSpatialEntityTracker;
using OpenXRMarkerTracker = OpenXRSpatialEntityTracker;
using OpenXRPlaneTracker = OpenXRSpatialEntityTracker;

// Spatial entity extension
class OpenXRSpatialEntityExtension final
{
public:
	enum Capability
	{
		CAPABILITY_PLANE_TRACKING = XR_SPATIAL_CAPABILITY_PLANE_TRACKING_EXT,
		CAPABILITY_MARKER_TRACKING_QR_CODE = XR_SPATIAL_CAPABILITY_MARKER_TRACKING_QR_CODE_EXT,
		CAPABILITY_MARKER_TRACKING_MICRO_QR_CODE =
			XR_SPATIAL_CAPABILITY_MARKER_TRACKING_MICRO_QR_CODE_EXT,
		CAPABILITY_MARKER_TRACKING_ARUCO_MARKER =
			XR_SPATIAL_CAPABILITY_MARKER_TRACKING_ARUCO_MARKER_EXT,
		CAPABILITY_MARKER_TRACKING_APRIL_TAG = XR_SPATIAL_CAPABILITY_MARKER_TRACKING_APRIL_TAG_EXT,
		CAPABILITY_ANCHOR = XR_SPATIAL_CAPABILITY_ANCHOR_EXT,
	};

	enum ComponentType
	{
		COMPONENT_TYPE_BOUNDED_2D = XR_SPATIAL_COMPONENT_TYPE_BOUNDED_2D_EXT,
		COMPONENT_TYPE_BOUNDED_3D = XR_SPATIAL_COMPONENT_TYPE_BOUNDED_3D_EXT,
		COMPONENT_TYPE_PARENT = XR_SPATIAL_COMPONENT_TYPE_PARENT_EXT,
		COMPONENT_TYPE_MESH_3D = XR_SPATIAL_COMPONENT_TYPE_MESH_3D_EXT,
		COMPONENT_TYPE_PLANE_ALIGNMENT = XR_SPATIAL_COMPONENT_TYPE_PLANE_ALIGNMENT_EXT,
		COMPONENT_TYPE_MESH_2D = XR_SPATIAL_COMPONENT_TYPE_MESH_2D_EXT,
		COMPONENT_TYPE_POLYGON_2D = XR_SPATIAL_COMPONENT_TYPE_POLYGON_2D_EXT,
		COMPONENT_TYPE_PLANE_SEMANTIC_LABEL = XR_SPATIAL_COMPONENT_TYPE_PLANE_SEMANTIC_LABEL_EXT,
		COMPONENT_TYPE_MARKER = XR_SPATIAL_COMPONENT_TYPE_MARKER_EXT,
		COMPONENT_TYPE_ANCHOR = XR_SPATIAL_COMPONENT_TYPE_ANCHOR_EXT,
		COMPONENT_TYPE_PERSISTENCE = XR_SPATIAL_COMPONENT_TYPE_PERSISTENCE_EXT,
	};

	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);

	static void on_instance_created(const XrInstance p_instance);
	static void on_instance_destroyed();
	static void on_session_destroyed();

	static bool on_event_polled(const XrEventDataBuffer& event);

	static bool get_active();
	static bool supports_capability(XrSpatialCapabilityEXT p_capability);
	static bool supports_component_type(
		XrSpatialCapabilityEXT p_capability, XrSpatialComponentTypeEXT p_component_type);

	// Spatial contexts
	static bool get_spatial_context_ready(RID p_spatial_context);
	static void free_spatial_context(RID p_spatial_context);
	static XrSpatialContextEXT get_spatial_context_handle(RID p_spatial_context);

	// Update query
	static RID update_spatial_entities(RID p_spatial_context, const LocalVector<RID>& p_entities,
		const LocalVector<XrSpatialComponentTypeEXT>& p_component_types,
		Ref<OpenXRStructureBase> p_next);

	// Snapshot data
	static void free_spatial_snapshot(RID p_spatial_snapshot);
	static XrSpatialSnapshotEXT get_spatial_snapshot_handle(RID p_spatial_snapshot);
	static RID get_spatial_snapshot_context(RID p_spatial_snapshot);

	// Buffers from snapshot
	static String get_string(RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id);
	static PackedByteArray get_uint8_buffer(
		RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id);
	static Vector<uint16_t> get_uint16_buffer(
		RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id);
	static Vector<uint32_t> get_uint32_buffer(
		RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id);
	static PackedFloat32Array get_float_buffer(
		RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id);
	static PackedVector2Array get_vector2_buffer(
		RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id);
	static PackedVector3Array get_vector3_buffer(
		RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id);

	// Entities
	static RID find_spatial_entity(XrSpatialEntityIdEXT p_entity_id);
	static RID add_spatial_entity(
		RID p_spatial_context, XrSpatialEntityIdEXT p_entity_id, XrSpatialEntityEXT p_entity);
	static RID make_spatial_entity(RID p_spatial_context, XrSpatialEntityIdEXT p_entity_id);
	static XrSpatialEntityIdEXT get_spatial_entity_id(RID p_entity);
	static RID get_spatial_entity_context(RID p_entity);
	static void free_spatial_entity(RID p_entity);

	static String get_spatial_capability_name(XrSpatialCapabilityEXT p_capability);
	static String get_spatial_component_type_name(XrSpatialComponentTypeEXT p_component_type);
	static String get_spatial_feature_name(XrSpatialCapabilityFeatureEXT p_feature);

private:
	// Capabilities
	struct SpatialEntityCapabality
	{
		Vector<XrSpatialComponentTypeEXT> component_types;
		Vector<XrSpatialCapabilityFeatureEXT> features;
	};
	static bool _load_capabilities();

	static bool _supports_capability(Capability p_capability);
	static bool _supports_component_type(Capability p_capability, ComponentType p_component_type);

	// Spatial context
	struct SpatialContextData
	{
		XrSpatialContextEXT spatial_context = XR_NULL_HANDLE;
	};

	static uint64_t _get_spatial_context_handle(RID p_spatial_context);

	// Snapshot data
	struct SpatialSnapshotData
	{
		RID spatial_context;
		XrSpatialSnapshotEXT spatial_snapshot = XR_NULL_HANDLE;
	};

	static uint64_t _get_spatial_snapshot_handle(RID p_spatial_snapshot);

	// Buffers from snapshot
	static String _get_string(RID p_spatial_snapshot, uint64_t p_buffer_id);
	static PackedByteArray _get_uint8_buffer(RID p_spatial_snapshot, uint64_t p_buffer_id);
	static PackedInt32Array _get_uint16_buffer(RID p_spatial_snapshot, uint64_t p_buffer_id);
	static PackedInt32Array _get_uint32_buffer(RID p_spatial_snapshot, uint64_t p_buffer_id);
	static PackedFloat32Array _get_float_buffer(RID p_spatial_snapshot, uint64_t p_buffer_id);
	static PackedVector2Array _get_vector2_buffer(RID p_spatial_snapshot, uint64_t p_buffer_id);
	static PackedVector3Array _get_vector3_buffer(RID p_spatial_snapshot, uint64_t p_buffer_id);

	// Entities
	struct SpatialEntityData
	{
		RID spatial_context;
		XrSpatialEntityIdEXT entity_id = XR_NULL_ENTITY;
		XrSpatialEntityEXT entity = XR_NULL_HANDLE;
	};

	static RID _find_entity(uint64_t p_entity_id);
	static RID _add_entity(RID p_spatial_context, uint64_t p_entity_id, uint64_t p_entity);
	static RID _make_entity(RID p_spatial_context, uint64_t p_entity_id);
	static uint64_t _get_entity_id(RID p_entity);

	// OpenXR API call wrappers
	EXT_PROTO_XRRESULT_FUNC5(xrEnumerateSpatialCapabilitiesEXT, (XrInstance), instance,
		(XrSystemId), system_id, (uint32_t), capability_capacity_input, (uint32_t*),
		capability_count_output, (XrSpatialCapabilityEXT*), capabilities);
	EXT_PROTO_XRRESULT_FUNC4(xrEnumerateSpatialCapabilityComponentTypesEXT, (XrInstance), instance,
		(XrSystemId), systemId, (XrSpatialCapabilityEXT), capability,
		(XrSpatialCapabilityComponentTypesEXT*), capability_components);
	EXT_PROTO_XRRESULT_FUNC6(xrEnumerateSpatialCapabilityFeaturesEXT, (XrInstance), instance,
		(XrSystemId), systemId, (XrSpatialCapabilityEXT), capability, (uint32_t),
		capability_feature_capacity_input, (uint32_t*), capability_feature_count_output,
		(XrSpatialCapabilityFeatureEXT*), capability_features);
	EXT_PROTO_XRRESULT_FUNC3(xrCreateSpatialContextAsyncEXT, (XrSession), session,
		(const XrSpatialContextCreateInfoEXT*), create_info, (XrFutureEXT*), future);
	EXT_PROTO_XRRESULT_FUNC3(xrCreateSpatialContextCompleteEXT, (XrSession), session, (XrFutureEXT),
		future, (XrCreateSpatialContextCompletionEXT*), completion);
	EXT_PROTO_XRRESULT_FUNC1(xrDestroySpatialContextEXT, (XrSpatialContextEXT), spatial_context);
	EXT_PROTO_XRRESULT_FUNC3(xrCreateSpatialDiscoverySnapshotAsyncEXT, (XrSpatialContextEXT),
		spatial_context, (const XrSpatialDiscoverySnapshotCreateInfoEXT*), create_info,
		(XrFutureEXT*), future);
	EXT_PROTO_XRRESULT_FUNC3(xrCreateSpatialDiscoverySnapshotCompleteEXT, (XrSpatialContextEXT),
		spatial_context, (const XrCreateSpatialDiscoverySnapshotCompletionInfoEXT*),
		create_snapshot_completion_info, (XrCreateSpatialDiscoverySnapshotCompletionEXT*),
		completion);
	EXT_PROTO_XRRESULT_FUNC3(xrQuerySpatialComponentDataEXT, (XrSpatialSnapshotEXT), snapshot,
		(const XrSpatialComponentDataQueryConditionEXT*), query_condition,
		(XrSpatialComponentDataQueryResultEXT*), query_result);
	EXT_PROTO_XRRESULT_FUNC1(xrDestroySpatialSnapshotEXT, (XrSpatialSnapshotEXT), snapshot);
	EXT_PROTO_XRRESULT_FUNC3(xrCreateSpatialEntityFromIdEXT, (XrSpatialContextEXT), spatial_context,
		(const XrSpatialEntityFromIdCreateInfoEXT*), create_info, (XrSpatialEntityEXT*),
		spatial_entity);
	EXT_PROTO_XRRESULT_FUNC1(xrDestroySpatialEntityEXT, (XrSpatialEntityEXT), spatial_entity);
	EXT_PROTO_XRRESULT_FUNC3(xrCreateSpatialUpdateSnapshotEXT, (XrSpatialContextEXT),
		spatial_context, (const XrSpatialUpdateSnapshotCreateInfoEXT*), createInfo,
		(XrSpatialSnapshotEXT*), snapshot);
	EXT_PROTO_XRRESULT_FUNC5(xrGetSpatialBufferStringEXT, (XrSpatialSnapshotEXT), snapshot,
		(const XrSpatialBufferGetInfoEXT*), info, (uint32_t), buffer_capacity_input, (uint32_t*),
		buffer_count_output, (char*), buffer);
	EXT_PROTO_XRRESULT_FUNC5(xrGetSpatialBufferUint8EXT, (XrSpatialSnapshotEXT), snapshot,
		(const XrSpatialBufferGetInfoEXT*), info, (uint32_t), buffer_capacity_input, (uint32_t*),
		buffer_count_output, (uint8_t*), buffer);
	EXT_PROTO_XRRESULT_FUNC5(xrGetSpatialBufferUint16EXT, (XrSpatialSnapshotEXT), snapshot,
		(const XrSpatialBufferGetInfoEXT*), info, (uint32_t), buffer_capacity_input, (uint32_t*),
		buffer_count_output, (uint16_t*), buffer);
	EXT_PROTO_XRRESULT_FUNC5(xrGetSpatialBufferUint32EXT, (XrSpatialSnapshotEXT), snapshot,
		(const XrSpatialBufferGetInfoEXT*), info, (uint32_t), buffer_capacity_input, (uint32_t*),
		buffer_count_output, (uint32_t*), buffer);
	EXT_PROTO_XRRESULT_FUNC5(xrGetSpatialBufferFloatEXT, (XrSpatialSnapshotEXT), snapshot,
		(const XrSpatialBufferGetInfoEXT*), info, (uint32_t), buffer_capacity_input, (uint32_t*),
		buffer_count_output, (float*), buffer);
	EXT_PROTO_XRRESULT_FUNC5(xrGetSpatialBufferVector2fEXT, (XrSpatialSnapshotEXT), snapshot,
		(const XrSpatialBufferGetInfoEXT*), info, (uint32_t), buffer_capacity_input, (uint32_t*),
		buffer_count_output, (XrVector2f*), buffer);
	EXT_PROTO_XRRESULT_FUNC5(xrGetSpatialBufferVector3fEXT, (XrSpatialSnapshotEXT), snapshot,
		(const XrSpatialBufferGetInfoEXT*), info, (uint32_t), buffer_capacity_input, (uint32_t*),
		buffer_count_output, (XrVector3f*), buffer);

public:
	struct Data
	{
		bool spatial_entity_ext = false;
		HashMap<XrSpatialCapabilityEXT, SpatialEntityCapabality> supported_capabilities;
		int capabilities_load_state = 0;
		RID_Owner<SpatialContextData> spatial_context_owner;
		RID_Owner<SpatialSnapshotData> spatial_snapshot_owner;
		RID_Owner<SpatialEntityData> spatial_entity_owner;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRSpatialEntityExtension() = delete;
	OpenXRSpatialEntityExtension(const OpenXRSpatialEntityExtension&) = delete;
	OpenXRSpatialEntityExtension& operator=(const OpenXRSpatialEntityExtension&) = delete;
	~OpenXRSpatialEntityExtension() = delete;
};

// (Persistent) anchor logic
class OpenXRSpatialAnchorCapability final
{
public:
	enum PersistenceScope
	{
		PERSISTENCE_SCOPE_SYSTEM_MANAGED = XR_SPATIAL_PERSISTENCE_SCOPE_SYSTEM_MANAGED_EXT,
		PERSISTENCE_SCOPE_LOCAL_ANCHORS = XR_SPATIAL_PERSISTENCE_SCOPE_LOCAL_ANCHORS_EXT,
	};

	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);

	static void on_instance_created(const XrInstance p_instance);
	static void on_instance_destroyed();
	static void on_session_created(const XrSession p_session);
	static void on_session_destroyed();

	static void on_process();

	static bool is_spatial_anchor_supported();
	static bool is_spatial_persistence_supported();

	// Persistence scopes
	static bool is_persistence_scope_supported(XrSpatialPersistenceScopeEXT p_scope);

	static XrSpatialPersistenceContextEXT get_persistence_context_handle(RID p_persistence_context);
	static void free_persistence_context(RID p_persistence_context);

	static Ref<OpenXRAnchorTracker> create_new_anchor(const Transform3D& p_transform,
		RID p_spatial_context = RID(),
		Ref<OpenXRStructureBase> p_next = Ref<OpenXRStructureBase>());
	static void remove_anchor(Ref<OpenXRAnchorTracker> p_anchor_tracker);

	static String get_spatial_persistence_scope_name(XrSpatialPersistenceScopeEXT p_scope);
	static String get_spatial_persistence_context_result_name(
		XrSpatialPersistenceContextResultEXT p_result);

private:
	static bool _load_supported_persistence_scopes();

	struct PersistenceContextData
	{
		XrSpatialPersistenceScopeEXT scope;
		XrSpatialPersistenceContextEXT persistence_context = XR_NULL_HANDLE;
	};

	static bool _is_persistence_scope_supported(PersistenceScope p_scope);
	static uint64_t _get_persistence_context_handle(RID p_persistence_context);

	static void _on_persistence_context_completed(RID p_persistence_context);
	static Ref<OpenXRFutureResult> _create_spatial_context();
	static void _on_spatial_context_created(RID p_spatial_context);
	static void _on_spatial_discovery_recommended(RID p_spatial_context);

	EXT_PROTO_XRRESULT_FUNC4(xrCreateSpatialAnchorEXT, (XrSpatialContextEXT), spatialContext,
		(const XrSpatialAnchorCreateInfoEXT*), create_info, (XrSpatialEntityIdEXT*),
		anchor_entity_id, (XrSpatialEntityEXT*), anchor_entity);

	EXT_PROTO_XRRESULT_FUNC5(xrEnumerateSpatialPersistenceScopesEXT, (XrInstance), instance,
		(XrSystemId), system_id, (uint32_t), persistence_scope_capacity_input, (uint32_t*),
		persistence_scope_count_output, (XrSpatialPersistenceScopeEXT*), persistence_scopes);
	EXT_PROTO_XRRESULT_FUNC3(xrCreateSpatialPersistenceContextAsyncEXT, (XrSession), session,
		(const XrSpatialPersistenceContextCreateInfoEXT*), create_info, (XrFutureEXT*), future);
	EXT_PROTO_XRRESULT_FUNC3(xrCreateSpatialPersistenceContextCompleteEXT, (XrSession), session,
		(XrFutureEXT), future, (XrCreateSpatialPersistenceContextCompletionEXT*), completion);
	EXT_PROTO_XRRESULT_FUNC1(xrDestroySpatialPersistenceContextEXT,
		(XrSpatialPersistenceContextEXT), persistence_context);

	EXT_PROTO_XRRESULT_FUNC3(xrPersistSpatialEntityAsyncEXT, (XrSpatialPersistenceContextEXT),
		persistence_context, (const XrSpatialEntityPersistInfoEXT*), persist_info, (XrFutureEXT*),
		future);
	EXT_PROTO_XRRESULT_FUNC3(xrPersistSpatialEntityCompleteEXT, (XrSpatialPersistenceContextEXT),
		persistence_context, (XrFutureEXT), future, (XrPersistSpatialEntityCompletionEXT*),
		completion);
	EXT_PROTO_XRRESULT_FUNC3(xrUnpersistSpatialEntityAsyncEXT, (XrSpatialPersistenceContextEXT),
		persistence_context, (const XrSpatialEntityUnpersistInfoEXT*), unpersist_info,
		(XrFutureEXT*), future);
	EXT_PROTO_XRRESULT_FUNC3(xrUnpersistSpatialEntityCompleteEXT, (XrSpatialPersistenceContextEXT),
		persistence_context, (XrFutureEXT), future, (XrUnpersistSpatialEntityCompletionEXT*),
		completion);

public:
	struct Data
	{
		bool spatial_anchor_ext = false;
		bool spatial_persistence_ext = false;
		bool spatial_persistence_operations_ext = false;
		bool spatial_anchor_supported = false;
		RID spatial_context;
		RID persistence_context;
		bool need_discovery = false;
		int discovery_cooldown = 0;
		Ref<OpenXRFutureResult> discovery_query_result;
		Ref<OpenXRSpatialCapabilityConfigurationBaseHeader> anchor_configuration;
		Ref<OpenXRSpatialContextPersistenceConfig> persistence_configuration;
		Vector<XrSpatialPersistenceScopeEXT> supported_persistence_scopes;
		RID_Owner<PersistenceContextData> persistence_context_owner;
		HashMap<RID, HashMap<XrSpatialEntityIdEXT, Ref<OpenXRAnchorTracker>>> anchor_trackers;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRSpatialAnchorCapability() = delete;
	OpenXRSpatialAnchorCapability(const OpenXRSpatialAnchorCapability&) = delete;
	OpenXRSpatialAnchorCapability& operator=(const OpenXRSpatialAnchorCapability&) = delete;
	~OpenXRSpatialAnchorCapability() = delete;
};

// Marker tracking logic
class OpenXRSpatialMarkerTrackingCapability final
{
public:
	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);

	static void on_session_created(const XrSession p_session);
	static void on_session_destroyed();

	static void on_process();

	static bool is_qrcode_supported();
	static bool is_micro_qrcode_supported();
	static bool is_aruco_supported();
	static bool is_april_tag_supported();

private:
	static Ref<OpenXRFutureResult> _create_spatial_context();
	static void _on_spatial_context_created(RID p_spatial_context);
	static void _on_spatial_discovery_recommended(RID p_spatial_context);

public:
	struct Data
	{
		bool spatial_marker_tracking_ext = false;
		RID spatial_context;
		bool need_discovery = false;
		int discovery_cooldown = 0;
		Ref<OpenXRFutureResult> discovery_query_result;
		Ref<OpenXRSpatialCapabilityConfigurationBaseHeader> qrcode_configuration;
		Ref<OpenXRSpatialCapabilityConfigurationBaseHeader> micro_qrcode_configuration;
		Ref<OpenXRSpatialCapabilityConfigurationBaseHeader> aruco_configuration;
		Ref<OpenXRSpatialCapabilityConfigurationBaseHeader> april_tag_configuration;
		HashMap<RID, HashMap<XrSpatialEntityIdEXT, Ref<OpenXRMarkerTracker>>> marker_trackers;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRSpatialMarkerTrackingCapability() = delete;
	OpenXRSpatialMarkerTrackingCapability(const OpenXRSpatialMarkerTrackingCapability&) = delete;
	OpenXRSpatialMarkerTrackingCapability& operator=(
		const OpenXRSpatialMarkerTrackingCapability&) = delete;
	~OpenXRSpatialMarkerTrackingCapability() = delete;
};

// Plane tracking logic
class OpenXRSpatialPlaneTrackingCapability final
{
public:
	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);

	static void on_session_created(const XrSession p_session);
	static void on_session_destroyed();

	static void on_process();

	static bool is_supported();

private:
	static Ref<OpenXRFutureResult> _create_spatial_context();
	static void _on_spatial_context_created(RID p_spatial_context);
	static void _on_spatial_discovery_recommended(RID p_spatial_context);

public:
	struct Data
	{
		bool spatial_plane_tracking_ext = false;
		bool spatial_plane_tracking_supported = false;
		RID spatial_context;
		bool need_discovery = false;
		int discovery_cooldown = 0;
		Ref<OpenXRFutureResult> discovery_query_result;
		Ref<OpenXRSpatialCapabilityConfigurationBaseHeader> plane_configuration;
		HashMap<RID, HashMap<XrSpatialEntityIdEXT, Ref<OpenXRPlaneTracker>>> plane_trackers;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRSpatialPlaneTrackingCapability() = delete;
	OpenXRSpatialPlaneTrackingCapability(const OpenXRSpatialPlaneTrackingCapability&) = delete;
	OpenXRSpatialPlaneTrackingCapability& operator=(
		const OpenXRSpatialPlaneTrackingCapability&) = delete;
	~OpenXRSpatialPlaneTrackingCapability() = delete;
};

// --- End modules/openxr/extensions/spatial_entities/openxr_spatial_entities.h ---
#include "servers/rendering/renderer.h"
#include "servers/xr/xr_server.h"

#ifdef ANDROID_ENABLED
#include "core/os/os.h"
#endif




#include "modules/openxr/openxr_platform_inc.h" // IWYU pragma: keep.

#ifdef VULKAN_ENABLED
// --- Inlined/Folded from modules/openxr/extensions/platform/openxr_vulkan_extension.h ---
#include "modules/openxr/openxr_api.h"
#include "core/templates/vector.h"
#include "core/types.h"

// Always include this as late as possible.
#include "modules/openxr/openxr_platform_inc.h" // IWYU pragma: keep.

class OpenXRVulkanExtension final
{
public:

	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);

	static void on_instance_created(const XrInstance p_instance);
	static void* set_session_create_and_get_next_pointer(void* p_next_pointer);

	static bool create_vulkan_instance(
		const VkInstanceCreateInfo* p_vulkan_create_info, VkInstance* r_instance);
	static bool get_physical_device(VkPhysicalDevice* r_device);
	static bool create_vulkan_device(
		const VkDeviceCreateInfo* p_device_create_info, VkDevice* r_device);
	static void set_direct_queue_family_and_index(
		uint32_t p_queue_family_index, uint32_t p_queue_index);
	static bool use_subsampled_images();
	static bool use_fragment_density_offsets();
	static void get_fragment_density_offsets(
		LocalVector<VkOffset2D>& r_offets, const Vector2i& p_granularity);

	static void get_usable_swapchain_formats(Vector<int64_t>& p_usable_swap_chains);
	static void get_usable_depth_formats(Vector<int64_t>& p_usable_swap_chains);
	static String get_swapchain_format_name(int64_t p_swapchain_format);
	static bool get_swapchain_image_data(XrSwapchain p_swapchain, int64_t p_swapchain_format,
		uint32_t p_width, uint32_t p_height, uint32_t p_sample_count, uint32_t p_array_size,
		void** r_swapchain_graphics_data);
	static void cleanup_swapchain_graphics_data(void** p_swapchain_graphics_data);
	static bool create_projection_fov(
		const XrFovf p_fov, double p_z_near, double p_z_far, Projection& r_camera_matrix);
	static RID get_texture(void* p_swapchain_graphics_data, int p_image_index);
	static RID get_density_map(void* p_swapchain_graphics_data, int p_image_index);

private:
	static XrGraphicsBindingVulkanKHR
		graphics_binding_vulkan; // declaring this as static so we don't need to know its size and
								 // we only need it once when creating our session

	struct SwapchainGraphicsData
	{
		bool is_multiview;
		Vector<RID> texture_rids;
		Vector<RID> density_map_rids;
	};

	static bool check_graphics_api_support(XrVersion p_desired_version);

	EXT_PROTO_XRRESULT_FUNC3(xrGetVulkanGraphicsRequirements2KHR, (XrInstance), p_instance,
		(XrSystemId), p_system_id, (XrGraphicsRequirementsVulkanKHR*), p_graphics_requirements)
	EXT_PROTO_XRRESULT_FUNC4(xrCreateVulkanInstanceKHR, (XrInstance), p_instance,
		(const XrVulkanInstanceCreateInfoKHR*), p_create_info, (VkInstance*), r_vulkan_instance,
		(VkResult*), r_vulkan_result)
	EXT_PROTO_XRRESULT_FUNC3(xrGetVulkanGraphicsDevice2KHR, (XrInstance), p_instance,
		(const XrVulkanGraphicsDeviceGetInfoKHR*), p_get_info, (VkPhysicalDevice*),
		r_vulkan_physical_device)
	EXT_PROTO_XRRESULT_FUNC4(xrCreateVulkanDeviceKHR, (XrInstance), p_instance,
		(const XrVulkanDeviceCreateInfoKHR*), p_create_info, (VkDevice*), r_device, (VkResult*),
		r_result)
	EXT_PROTO_XRRESULT_FUNC4(xrEnumerateSwapchainImages, (XrSwapchain), p_swapchain, (uint32_t),
		p_image_capacity_input, (uint32_t*), p_image_count_output, (XrSwapchainImageBaseHeader*),
		p_images)

public:
	struct Data
	{
		VkInstance vulkan_instance = nullptr;
		VkPhysicalDevice vulkan_physical_device = nullptr;
		VkDevice vulkan_device = nullptr;
		uint32_t vulkan_queue_family_index = UINT32_MAX;
		uint32_t vulkan_queue_index = UINT32_MAX;
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXRVulkanExtension() = delete;
	OpenXRVulkanExtension(const OpenXRVulkanExtension&) = delete;
	OpenXRVulkanExtension& operator=(const OpenXRVulkanExtension&) = delete;
	~OpenXRVulkanExtension() = delete;
};
#endif

#ifdef METAL_ENABLED
#include "modules/openxr/openxr_platform_inc.h"
#endif

#if defined(GLES3_ENABLED) && !defined(MACOS_ENABLED)
// --- Inlined/Folded from modules/openxr/extensions/platform/openxr_opengl_extension.h ---
#ifdef GLES3_ENABLED

#include "modules/openxr/openxr_api.h"
#include "core/templates/vector.h"
#include "core/types.h"

// Always include this as late as possible.
#include "modules/openxr/openxr_platform_inc.h" // IWYU pragma: keep.

class OpenXROpenGLExtension final
{
public:
	static HashMap<String, bool*> get_requested_extensions(XrVersion p_version);

	static void on_instance_created(const XrInstance p_instance);
	static void* set_session_create_and_get_next_pointer(void* p_next_pointer);

	static void get_usable_swapchain_formats(Vector<int64_t>& p_usable_swap_chains);
	static void get_usable_depth_formats(Vector<int64_t>& p_usable_swap_chains);
	static String get_swapchain_format_name(int64_t p_swapchain_format);
	static bool get_swapchain_image_data(XrSwapchain p_swapchain, int64_t p_swapchain_format,
		uint32_t p_width, uint32_t p_height, uint32_t p_sample_count, uint32_t p_array_size,
		void** r_swapchain_graphics_data);
	static void cleanup_swapchain_graphics_data(void** p_swapchain_graphics_data);
	static bool create_projection_fov(
		const XrFovf p_fov, double p_z_near, double p_z_far, Projection& r_camera_matrix);
	static RID get_texture(void* p_swapchain_graphics_data, int p_image_index);

	static RID get_density_map(void* p_swapchain_graphics_data, int p_image_index)
	{
		return RID();
	}

private:

#ifdef WIN32
	static XrGraphicsBindingOpenGLWin32KHR graphics_binding_gl;
#elif defined(ANDROID_ENABLED)
	static XrGraphicsBindingOpenGLESAndroidKHR graphics_binding_gl;
#elif defined(LINUXBSD_ENABLED)
#ifdef X11_ENABLED
	static XrGraphicsBindingOpenGLXlibKHR graphics_binding_gl;
#endif
#if defined(EGL_ENABLED) && defined(WAYLAND_ENABLED)
	static XrGraphicsBindingEGLMNDX graphics_binding_egl;

#endif
#else
#error "OpenXR with OpenGL isn't supported on this platform"
#endif

	struct SwapchainGraphicsData
	{
		bool is_multiview;
		Vector<RID> texture_rids;
	};

	static bool check_graphics_api_support(XrVersion p_desired_version);

#ifdef XR_USE_GRAPHICS_API_OPENGL_ES
	EXT_PROTO_XRRESULT_FUNC3(xrGetOpenGLESGraphicsRequirementsKHR, (XrInstance), p_instance,
		(XrSystemId), p_system_id, (XrGraphicsRequirementsOpenGLESKHR*), p_graphics_requirements)
#else
	EXT_PROTO_XRRESULT_FUNC3(xrGetOpenGLGraphicsRequirementsKHR, (XrInstance), p_instance,
		(XrSystemId), p_system_id, (XrGraphicsRequirementsOpenGLKHR*), p_graphics_requirements)
#endif
	EXT_PROTO_XRRESULT_FUNC4(xrEnumerateSwapchainImages, (XrSwapchain), p_swapchain, (uint32_t),
		p_image_capacity_input, (uint32_t*), p_image_count_output, (XrSwapchainImageBaseHeader*),
		p_images)

public:
	struct Data
	{
#if defined(LINUXBSD_ENABLED) && defined(EGL_ENABLED) && defined(WAYLAND_ENABLED)
		bool egl_extension_enabled = false;
#endif
	};

	static inline Data* data = nullptr;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	OpenXROpenGLExtension() = delete;
	OpenXROpenGLExtension(const OpenXROpenGLExtension&) = delete;
	OpenXROpenGLExtension& operator=(const OpenXROpenGLExtension&) = delete;
	~OpenXROpenGLExtension() = delete;
};

#endif // GLES3_ENABLED
#endif

#ifdef D3D12_ENABLED
#include "modules/openxr/openxr_platform_inc.h"
#endif

#ifdef ANDROID_ENABLED
#define OPENXR_LOADER_NAME "libopenxr_loader.so"
#endif

Vector<OpenXRAPI::OpenXRSwapChainInfo> OpenXRAPI::OpenXRSwapChainInfo::free_queue;

bool OpenXRAPI::OpenXRSwapChainInfo::create(XrSwapchainCreateFlags p_create_flags,
	XrSwapchainUsageFlags p_usage_flags, int64_t p_swapchain_format, uint32_t p_width,
	uint32_t p_height, uint32_t p_sample_count, uint32_t p_array_size, bool p_use_next_extensions)
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), false);

	XrSession xr_session = OpenXRAPI::get_session();
	ERR_FAIL_COND_V(xr_session == XR_NULL_HANDLE, false);

	// We already have a swapchain?
	ERR_FAIL_COND_V(swapchain != XR_NULL_HANDLE, false);

	XrResult result;

	void* next_pointer = nullptr;

	XrSwapchainCreateInfo swapchain_create_info = {
		XR_TYPE_SWAPCHAIN_CREATE_INFO, // type
		next_pointer,				   // next
		p_create_flags,				   // createFlags
		p_usage_flags,				   // usageFlags
		p_swapchain_format,			   // format
		p_sample_count,				   // sampleCount
		p_width,					   // width
		p_height,					   // height
		1,							   // faceCount
		p_array_size,				   // arraySize
		1							   // mipCount
	};

	XrSwapchain new_swapchain;
	result = OpenXRAPI::xrCreateSwapchain(xr_session, &swapchain_create_info, &new_swapchain);
	if (XR_FAILED(result)) {
		// print_line("OpenXR: Failed to get swapchain [", OpenXRAPI::get_error_string(result),
		// "]");
		return false;
	}

	swapchain = new_swapchain;

	return true;
}

void OpenXRAPI::OpenXRSwapChainInfo::queue_free()
{
	if (image_acquired) {
		release();
	}

	if (swapchain != XR_NULL_HANDLE) {
		free_queue.push_back(*this);

		swapchain_graphics_data = nullptr;
		swapchain = XR_NULL_HANDLE;
	}
}

void OpenXRAPI::OpenXRSwapChainInfo::free_queued()
{
	for (OpenXRAPI::OpenXRSwapChainInfo& swapchain_info : free_queue) {
		swapchain_info.free();
	}
	free_queue.clear();
}

void OpenXRAPI::OpenXRSwapChainInfo::free()
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	if (image_acquired) {
		release();
	}

	if (swapchain != XR_NULL_HANDLE) {
		OpenXRAPI::xrDestroySwapchain(swapchain);
		swapchain = XR_NULL_HANDLE;
	}
}

bool OpenXRAPI::OpenXRSwapChainInfo::release()
{
	if (!image_acquired) {
		// Already released or never acquired.
		return true;
	}

	image_acquired = false; // Regardless if we succeed or not, consider this released.

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), false);

	XrSwapchainImageReleaseInfo swapchain_image_release_info = {
		XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO, // type
		nullptr								  // next
	};
	XrResult result = OpenXRAPI::xrReleaseSwapchainImage(swapchain, &swapchain_image_release_info);
	if (XR_FAILED(result)) {
		// print_line("OpenXR: failed to release swapchain image! [",
		// OpenXRAPI::get_error_string(result), "]");
		return false;
	}

	return true;
}

bool OpenXRAPI::openxr_is_enabled(bool p_check_run_in_editor)
{
	if (XRServer::get_xr_mode() == XRServer::XRMODE_DEFAULT) {
		if (Engine::is_editor_hint() && p_check_run_in_editor) {
			// For now, don't start OpenXR when the editor starts up. In the future, this may change
			// if we want to integrate more XR features into the editor experience.
			return false;
		}
	}
}

String OpenXRAPI::get_error_string(XrResult result) { return String(); }

void OpenXRAPI::set_object_name(
	XrObjectType p_object_type, uint64_t p_object_handle, const String& p_object_name)
{
	if (!OpenXRDebugUtilsExtension::is_initialized() || !OpenXRDebugUtilsExtension::get_active()) {
		// Not enabled/active? Ignore.
		return;
	}

	OpenXRDebugUtilsExtension::set_object_name(p_object_type, p_object_handle, p_object_name.utf8().get_data());
}

void OpenXRAPI::begin_debug_label_region(const String& p_label_name)
{
	if (!OpenXRDebugUtilsExtension::is_initialized() || !OpenXRDebugUtilsExtension::get_active()) {
		// Not enabled/active? Ignore.
		return;
	}

	OpenXRDebugUtilsExtension::begin_debug_label_region(p_label_name.utf8().get_data());
}

void OpenXRAPI::end_debug_label_region()
{
	if (!OpenXRDebugUtilsExtension::is_initialized() || !OpenXRDebugUtilsExtension::get_active()) {
		// Not enabled/active? Ignore.
		return;
	}

	OpenXRDebugUtilsExtension::end_debug_label_region();
}

void OpenXRAPI::insert_debug_label(const String& p_label_name)
{
	if (!OpenXRDebugUtilsExtension::is_initialized() || !OpenXRDebugUtilsExtension::get_active()) {
		// Not enabled/active? Ignore.
		return;
	}

	OpenXRDebugUtilsExtension::insert_debug_label(p_label_name.utf8().get_data());
}

bool OpenXRAPI::load_layer_properties()
{
	// This queries additional layers that are available and can be initialized when we create our
	// OpenXR instance
	if (!data->layer_properties.is_empty()) {
		// already retrieved this
		return true;
	}

	// Note, instance is not yet setup so we can't use get_error_string to retrieve our error
	uint32_t num_layer_properties = 0;
	XrResult result = xrEnumerateApiLayerProperties(0, &num_layer_properties, nullptr);
	ERR_FAIL_COND_V_MSG(
		XR_FAILED(result), false, "OpenXR: Failed to enumerate number of api layer properties");

	data->layer_properties.resize(num_layer_properties);
	for (XrApiLayerProperties& layer : data->layer_properties) {
		layer.type = XR_TYPE_API_LAYER_PROPERTIES;
		layer.next = nullptr;
	}

	result = xrEnumerateApiLayerProperties(
		num_layer_properties, &num_layer_properties, data->layer_properties.ptr());
	ERR_FAIL_COND_V_MSG(
		XR_FAILED(result), false, "OpenXR: Failed to enumerate api layer properties");

	for (const XrApiLayerProperties& layer : data->layer_properties) {
		print_verbose(vformat("OpenXR: Found OpenXR layer %s.", layer.layerName));
	}

	return true;
}

bool OpenXRAPI::load_supported_extensions()
{
	// This queries supported extensions that are available and can be initialized when we create
	// our OpenXR instance

	if (!data->supported_extensions.is_empty()) {
		// already retrieved this
		return true;
	}

	// Note, instance is not yet setup so we can't use get_error_string to retrieve our error
	uint32_t num_supported_extensions = 0;
	XrResult result =
		xrEnumerateInstanceExtensionProperties(nullptr, 0, &num_supported_extensions, nullptr);
	ERR_FAIL_COND_V_MSG(
		XR_FAILED(result), false, "OpenXR: Failed to enumerate number of extension properties");

	data->supported_extensions.resize(num_supported_extensions);

	// set our types
	for (XrExtensionProperties& extension : data->supported_extensions) {
		extension.type = XR_TYPE_EXTENSION_PROPERTIES;
		extension.next = nullptr;
	}
	result = xrEnumerateInstanceExtensionProperties(
		nullptr, num_supported_extensions, &num_supported_extensions, data->supported_extensions.ptr());
	ERR_FAIL_COND_V_MSG(
		XR_FAILED(result), false, "OpenXR: Failed to enumerate extension properties");

	for (const XrExtensionProperties& extension : data->supported_extensions) {
		print_verbose(vformat("OpenXR: Found OpenXR extension %s.", extension.extensionName));
	}

	return true;
}

bool OpenXRAPI::is_extension_supported(const String& p_extension)
{
	for (const XrExtensionProperties& extension : data->supported_extensions) {
		if (extension.extensionName == p_extension) {
			return true;
		}
	}

	return false;
}

bool OpenXRAPI::is_any_extension_enabled(const String& p_extensions)
{
	// We allow a comma separated list of extensions here, only one needs to be supported.
	// This allows us to check for extensions that were renamed or that were embedded in core
	// at a specific OpenXR version.
	for (const String& name : p_extensions.split(",", false)) {
		CharString extension = name.utf8();

		if (data->enabled_extensions.has(extension)) {
			return true;
		}
	}

	return false;
}

bool OpenXRAPI::is_top_level_path_supported(const String& p_toplevel_path)
{
	String required_extensions =
		OpenXRInteractionProfileMetadata::get_top_level_extensions(
			p_toplevel_path);

	// If unsupported is returned we likely have a misspelled interaction profile path in our action
	// map. Always output that as an error.
	ERR_FAIL_COND_V_MSG(required_extensions == XR_PATH_UNSUPPORTED_NAME, false,
		"OpenXR: Unsupported toplevel path " + p_toplevel_path);

	if (required_extensions == "") {
		// no extension needed, core top level are always "supported", they just won't be used if
		// not really supported
		return true;
	}

	if (!is_any_extension_enabled(required_extensions)) {
		// It is very likely we have top level paths for which the extension is not available so
		// don't flood the logs with unnecessary spam.
		print_verbose("OpenXR: Top level path " + p_toplevel_path + " requires extension " +
					  required_extensions.replace(",", " or "));
		return false;
	}

	return true;
}

bool OpenXRAPI::is_interaction_profile_supported(const String& p_ip_path)
{
	String required_extensions =
		OpenXRInteractionProfileMetadata::get_interaction_profile_extensions(
			p_ip_path);

	// If unsupported is returned we likely have a misspelled interaction profile path in our action
	// map. Always output that as an error.
	ERR_FAIL_COND_V_MSG(required_extensions == XR_PATH_UNSUPPORTED_NAME, false,
		"OpenXR: Unsupported interaction profile " + p_ip_path);

	if (required_extensions == "") {
		// no extension needed, core interaction profiles are always "supported", they just won't be
		// used if not really supported
		return true;
	}

	if (!is_any_extension_enabled(required_extensions)) {
		// It is very likely we have interaction profiles for which the extension is not available
		// so don't flood the logs with unnecessary spam.
		print_verbose("OpenXR: Interaction profile " + p_ip_path + " requires extension " +
					  required_extensions.replace(",", " or "));
		return false;
	}

	return true;
}

bool OpenXRAPI::interaction_profile_supports_io_path(
	const String& p_ip_path, const String& p_io_path)
{
	if (!is_interaction_profile_supported(p_ip_path)) {
		return false;
	}

	const OpenXRInteractionProfileMetadata::IOPath* io_path =
		OpenXRInteractionProfileMetadata::get_io_path(p_ip_path, p_io_path);

	// If the io_path is not part of our metadata we've likely got a misspelled name or a bad action
	// map, report
	ERR_FAIL_NULL_V_MSG(
		io_path, false, "OpenXR: Unsupported io path " + String(p_ip_path) + String(p_io_path));

	if (io_path->openxr_extension_names == "") {
		// no extension needed, core io paths are always "supported", they just won't be used if not
		// really supported
		return true;
	}

	if (!is_any_extension_enabled(io_path->openxr_extension_names)) {
		// It is very likely we have io paths for which the extension is not available so don't
		// flood the logs with unnecessary spam.
		print_verbose("OpenXR: IO path " + String(p_ip_path) + String(p_io_path) +
					  " requires extension " +
					  io_path->openxr_extension_names.replace(",", " or "));
		return false;
	}

	return true;
}

void OpenXRAPI::copy_string_to_char_buffer(const String& p_string, char* p_buffer, int p_buffer_len)
{
	CharString char_string = p_string.utf8();
	int len = char_string.length();
	if (len < p_buffer_len - 1) {
		// was having weird CI issues with strcpy so....
		memcpy(p_buffer, char_string.get_data(), len);
		p_buffer[len] = '\0';
	}
	else {
		memcpy(p_buffer, char_string.get_data(), p_buffer_len - 1);
		p_buffer[p_buffer_len - 1] = '\0';
	}
}

bool OpenXRAPI::load_supported_view_configuration_types()
{
	// This queries the supported configuration types, likely there will only be one choosing
	// between Mono (phone AR) and Stereo (HMDs)

	ERR_FAIL_COND_V(data->instance == XR_NULL_HANDLE, false);

	data->supported_view_configuration_types.clear();

	uint32_t num_view_configuration_types = 0;
	XrResult result = xrEnumerateViewConfigurations(
		data->instance, data->system_id, 0, &num_view_configuration_types, nullptr);
	if (XR_FAILED(result)) {
		// print_line(
		// 	"OpenXR: Failed to get view configuration count [", get_error_string(result), "]");
		return false;
	}

	data->supported_view_configuration_types.resize(num_view_configuration_types);

	result = xrEnumerateViewConfigurations(data->instance, data->system_id, num_view_configuration_types,
		&num_view_configuration_types, data->supported_view_configuration_types.ptr());
	ERR_FAIL_COND_V_MSG(XR_FAILED(result), false, "OpenXR: Failed to enumerateview configurations");
	ERR_FAIL_COND_V_MSG(num_view_configuration_types == 0, false,
		"OpenXR: Failed to enumerateview configurations"); // JIC there should be at least 1!

	for (const XrViewConfigurationType& view_configuration_type :
		data->supported_view_configuration_types) {
		print_verbose(vformat("OpenXR: Found supported view configuration %s.",
			OpenXRUtil::get_view_configuration_name(view_configuration_type)));
	}

	// Check value we loaded at startup...
	if (!is_view_configuration_supported(data->view_configuration)) {
		print_verbose(vformat("OpenXR: %s isn't supported, defaulting to %s.",
			OpenXRUtil::get_view_configuration_name(data->view_configuration),
			OpenXRUtil::get_view_configuration_name(data->supported_view_configuration_types[0])));

		data->view_configuration = data->supported_view_configuration_types[0];
	}

	return true;
}

bool OpenXRAPI::load_supported_environmental_blend_modes()
{
	// This queries the supported environmental blend modes.

	ERR_FAIL_COND_V(data->instance == XR_NULL_HANDLE, false);

	data->supported_environment_blend_modes.clear();

	uint32_t num_supported_environment_blend_modes = 0;
	XrResult result = xrEnumerateEnvironmentBlendModes(data->instance, data->system_id, data->view_configuration, 0,
		&num_supported_environment_blend_modes, nullptr);
	if (XR_FAILED(result)) {
		// print_line("OpenXR: Failed to get supported environmental blend mode count [",
		// 	get_error_string(result), "]");
		return false;
	}

	data->supported_environment_blend_modes.resize(num_supported_environment_blend_modes);

	result = xrEnumerateEnvironmentBlendModes(data->instance, data->system_id, data->view_configuration,
		num_supported_environment_blend_modes, &num_supported_environment_blend_modes,
		data->supported_environment_blend_modes.ptrw());
	ERR_FAIL_COND_V_MSG(
		XR_FAILED(result), false, "OpenXR: Failed to enumerate environmental blend modes");
	ERR_FAIL_COND_V_MSG(num_supported_environment_blend_modes == 0, false,
		"OpenXR: Failed to enumerate environmental blend modes"); // JIC there should be at least 1!

	for (const XrEnvironmentBlendMode& supported_environment_blend_mode :
		data->supported_environment_blend_modes) {
		print_verbose(vformat("OpenXR: Found environmental blend mode %s.",
			OpenXRUtil::get_environment_blend_mode_name(supported_environment_blend_mode)));
	}

	return true;
}

bool OpenXRAPI::is_view_configuration_supported(XrViewConfigurationType p_configuration_type)
{
	return data->supported_view_configuration_types.has(p_configuration_type);
}

bool OpenXRAPI::is_reference_space_supported(XrReferenceSpaceType p_reference_space)
{
	return data->supported_reference_spaces.has(p_reference_space);
}

bool OpenXRAPI::is_swapchain_format_supported(int64_t p_swapchain_format)
{
	return data->supported_swapchain_formats.has(p_swapchain_format);
}

void OpenXRAPI::set_form_factor(XrFormFactor p_form_factor)
{
	ERR_FAIL_COND(is_initialized());

	data->form_factor = p_form_factor;
}

uint32_t OpenXRAPI::get_view_count() { return data->view_configuration_views.size(); }

void OpenXRAPI::set_view_configuration(XrViewConfigurationType p_view_configuration)
{
	ERR_FAIL_COND(is_initialized());

	data->view_configuration = p_view_configuration;
}

bool OpenXRAPI::set_requested_reference_space(XrReferenceSpaceType p_requested_reference_space)
{
	if (data->custom_play_space != XR_NULL_HANDLE) {
		return false;
	}

	data->requested_reference_space = p_requested_reference_space;
	data->play_space_is_dirty = true;

	return true;
}

void OpenXRAPI::set_custom_play_space(XrSpace p_custom_space)
{
	data->custom_play_space = p_custom_space;
	data->play_space_is_dirty = true;
}

void OpenXRAPI::set_submit_depth_buffer(bool p_submit_depth_buffer)
{
	ERR_FAIL_COND(is_initialized());

	data->submit_depth_buffer = p_submit_depth_buffer;
}

bool OpenXRAPI::is_initialized() { return data != nullptr && (data->instance != XR_NULL_HANDLE); }

bool OpenXRAPI::is_running()
{
	if (data->instance == XR_NULL_HANDLE) {
		return false;
	}
	if (data->session == XR_NULL_HANDLE) {
		return false;
	}

	return data->running;
}

bool OpenXRAPI::openxr_loader_init()
{
#ifdef ANDROID_ENABLED
	ERR_FAIL_COND_V_MSG(
		data->openxr_loader_library_handle != nullptr, false, "OpenXR Loader library is already loaded.");

	{
		Error error_code = OS::open_dynamic_library(
			OPENXR_LOADER_NAME, data->openxr_loader_library_handle);
		ERR_FAIL_COND_V_MSG(error_code != OK, false, "OpenXR loader not found.");
	}

	{
		Error error_code = OS::get_dynamic_library_symbol_handle(
			data->openxr_loader_library_handle, "xrGetInstanceProcAddr", (void*&)data->xrGetInstanceProcAddr);
		ERR_FAIL_COND_V_MSG(error_code != OK, false,
			"Symbol xrGetInstanceProcAddr not found in OpenXR Loader library.");
	}
#endif

	// Resolve the symbols that don't require an instance
	OPENXR_API_INIT_XR_FUNC_V(xrCreateInstance);
	OPENXR_API_INIT_XR_FUNC_V(xrEnumerateApiLayerProperties);
	OPENXR_API_INIT_XR_FUNC_V(xrEnumerateInstanceExtensionProperties);

	return true;
}

bool OpenXRAPI::resolve_instance_openxr_symbols()
{
	ERR_FAIL_COND_V(data->instance == XR_NULL_HANDLE, false);

	OPENXR_API_INIT_XR_FUNC_V(xrAcquireSwapchainImage);
	OPENXR_API_INIT_XR_FUNC_V(xrApplyHapticFeedback);
	OPENXR_API_INIT_XR_FUNC_V(xrAttachSessionActionSets);
	OPENXR_API_INIT_XR_FUNC_V(xrBeginFrame);
	OPENXR_API_INIT_XR_FUNC_V(xrBeginSession);
	OPENXR_API_INIT_XR_FUNC_V(xrCreateAction);
	OPENXR_API_INIT_XR_FUNC_V(xrCreateActionSet);
	OPENXR_API_INIT_XR_FUNC_V(xrCreateActionSpace);
	OPENXR_API_INIT_XR_FUNC_V(xrCreateReferenceSpace);
	OPENXR_API_INIT_XR_FUNC_V(xrCreateSession);
	OPENXR_API_INIT_XR_FUNC_V(xrCreateSwapchain);
	OPENXR_API_INIT_XR_FUNC_V(xrDestroyAction);
	OPENXR_API_INIT_XR_FUNC_V(xrDestroyActionSet);
	OPENXR_API_INIT_XR_FUNC_V(xrDestroyInstance);
	OPENXR_API_INIT_XR_FUNC_V(xrDestroySession);
	OPENXR_API_INIT_XR_FUNC_V(xrDestroySpace);
	OPENXR_API_INIT_XR_FUNC_V(xrDestroySwapchain);
	OPENXR_API_INIT_XR_FUNC_V(xrEndFrame);
	OPENXR_API_INIT_XR_FUNC_V(xrEndSession);
	OPENXR_API_INIT_XR_FUNC_V(xrEnumerateEnvironmentBlendModes);
	OPENXR_API_INIT_XR_FUNC_V(xrEnumerateReferenceSpaces);
	OPENXR_API_INIT_XR_FUNC_V(xrEnumerateSwapchainFormats);
	OPENXR_API_INIT_XR_FUNC_V(xrEnumerateViewConfigurations);
	OPENXR_API_INIT_XR_FUNC_V(xrEnumerateViewConfigurationViews);
	OPENXR_API_INIT_XR_FUNC_V(xrGetActionStateBoolean);
	OPENXR_API_INIT_XR_FUNC_V(xrGetActionStateFloat);
	OPENXR_API_INIT_XR_FUNC_V(xrGetActionStateVector2f);
	OPENXR_API_INIT_XR_FUNC_V(xrGetCurrentInteractionProfile);
	OPENXR_API_INIT_XR_FUNC_V(xrGetReferenceSpaceBoundsRect);
	OPENXR_API_INIT_XR_FUNC_V(xrGetSystem);
	OPENXR_API_INIT_XR_FUNC_V(xrGetSystemProperties);
	OPENXR_API_INIT_XR_FUNC_V(xrLocateViews);
	OPENXR_API_INIT_XR_FUNC_V(xrLocateSpace);
	OPENXR_API_INIT_XR_FUNC_V(xrPathToString);
	OPENXR_API_INIT_XR_FUNC_V(xrPollEvent);
	OPENXR_API_INIT_XR_FUNC_V(xrReleaseSwapchainImage);
	OPENXR_API_INIT_XR_FUNC_V(xrResultToString);
	OPENXR_API_INIT_XR_FUNC_V(xrStringToPath);
	OPENXR_API_INIT_XR_FUNC_V(xrSuggestInteractionProfileBindings);
	OPENXR_API_INIT_XR_FUNC_V(xrSyncActions);
	OPENXR_API_INIT_XR_FUNC_V(xrWaitFrame);
	OPENXR_API_INIT_XR_FUNC_V(xrWaitSwapchainImage);

	return true;
}

XrResult OpenXRAPI::try_get_instance_proc_addr(const char* p_name, PFN_xrVoidFunction* p_addr)
{
	#ifdef ANDROID_ENABLED
	return data->xrGetInstanceProcAddr(data->instance, p_name, p_addr);
#else
	return ::xrGetInstanceProcAddr(data->instance, p_name, p_addr);
#endif
}

XrResult OpenXRAPI::get_instance_proc_addr(const char* p_name, PFN_xrVoidFunction* p_addr)
{
	XrResult result = try_get_instance_proc_addr(p_name, p_addr);

	if (result != XR_SUCCESS) {
		String error_message = String("Symbol ") + p_name + " not found in OpenXR instance.";
		ERR_FAIL_V_MSG(result, error_message.utf8().get_data());
	}

	return result;
}

bool OpenXRAPI::initialize_session()
{
	if (!create_session()) {
		destroy_session();
		return false;
	}

	if (!load_supported_reference_spaces()) {
		destroy_session();
		return false;
	}

	if (!setup_view_space()) {
		destroy_session();
		return false;
	}

	if (!load_supported_swapchain_formats()) {
		destroy_session();
		return false;
	}

	if (!obtain_swapchain_formats()) {
		destroy_session();
		return false;
	}

	allocate_view_buffers(data->view_configuration_views.size(), data->submit_depth_buffer);

	return true;
}

void OpenXRAPI::finish()
{
	destroy_session();

	destroy_instance();
}

void OpenXRAPI::set_xr_interface(OpenXRInterface* p_xr_interface) { data->xr_interface = p_xr_interface; }

Size2 OpenXRAPI::get_recommended_target_size()
{
	ERR_FAIL_COND_V(data->view_configuration_views.is_empty(), Size2());

	Size2 target_size;

	if (Renderer::data && Renderer::is_on_render_thread()) {
		target_size.width = data->view_configuration_views[0].recommendedImageRectWidth *
							data->render_state.render_target_size_multiplier;
		target_size.height = data->view_configuration_views[0].recommendedImageRectHeight *
							 data->render_state.render_target_size_multiplier;
	}
	else {
		target_size.width =
			data->view_configuration_views[0].recommendedImageRectWidth * data->render_target_size_multiplier;
		target_size.height =
			data->view_configuration_views[0].recommendedImageRectHeight * data->render_target_size_multiplier;
	}

	return target_size;
}

XRPose::TrackingConfidence OpenXRAPI::get_head_center(
	Transform3D& r_transform, Vector3& r_linear_velocity, Vector3& r_angular_velocity)
{
	XrResult result;

	if (!data->running) {
		return XRPose::XR_TRACKING_CONFIDENCE_NONE;
	}

	// Get display time
	XrTime display_time = get_predicted_display_time();
	if (display_time == 0) {
		return XRPose::XR_TRACKING_CONFIDENCE_NONE;
	}

	XrSpaceVelocity velocity = {
		XR_TYPE_SPACE_VELOCITY, // type
		nullptr,				// next
		0,						// velocityFlags
		{0.0, 0.0, 0.0},		// linearVelocity
		{0.0, 0.0, 0.0}			// angularVelocity
	};

	XrSpaceLocation location = {
		XR_TYPE_SPACE_LOCATION, // type
		&velocity,				// next
		0,						// locationFlags
		{
			{0.0, 0.0, 0.0, 0.0}, // orientation
			{0.0, 0.0, 0.0}		  // position
		}						  // pose
	};

	result = xrLocateSpace(data->view_space, data->play_space, display_time, &location);
	if (XR_FAILED(result)) {
		// print_line(
		// 	"OpenXR: Failed to locate view space in play space [", get_error_string(result), "]");
		return XRPose::XR_TRACKING_CONFIDENCE_NONE;
	}

	XRPose::TrackingConfidence confidence = transform_from_location(location, r_transform);
	parse_velocities(velocity, r_linear_velocity, r_angular_velocity);

	if (data->head_pose_confidence != confidence) {
		// prevent error spam
		data->head_pose_confidence = confidence;
		if (data->head_pose_confidence == XRPose::XR_TRACKING_CONFIDENCE_NONE) {
			// print_line("OpenXR head space location not valid (check tracking?)");
		}
		else if (data->head_pose_confidence == XRPose::XR_TRACKING_CONFIDENCE_LOW) {
			print_verbose("OpenVR Head pose now tracking with low confidence");
		}
		else {
			print_verbose("OpenVR Head pose now tracking with high confidence");
		}
	}

	return confidence;
}

bool OpenXRAPI::get_view_transform(uint32_t p_view, Transform3D& r_transform)
{
	ERR_NOT_ON_RENDER_THREAD_V(false);

	if (!data->render_state.running) {
		return false;
	}

	// we don't have valid view info
	if (data->render_state.views.is_empty() || !data->render_state.view_pose_valid) {
		return false;
	}

	// Note, the timing of this is set right before rendering, which is what we need here.
	r_transform = transform_from_pose(data->render_state.views[p_view].pose);

	return true;
}

void OpenXRAPI::_allocate_view_buffers_rt(uint32_t p_view_count, bool p_submit_depth_buffer)
{
	// Must be called from rendering thread!
	ERR_NOT_ON_RENDER_THREAD;

	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	data->render_state.submit_depth_buffer = p_submit_depth_buffer;

	// Allocate buffers we'll be populating with view information.
	data->render_state.views.resize(p_view_count);
	data->render_state.projection_views.resize(p_view_count);

	for (uint32_t i = 0; i < p_view_count; i++) {
		data->render_state.views[i] = {
			XR_TYPE_VIEW, // type
			nullptr,	  // next
			{},			  // pose
			{},			  // fov
		};
		data->render_state.projection_views[i] = {
			XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW, // type
			nullptr,								   // next
			{},										   // pose
			{},										   // fov
			{},										   // subImage
		};
	}

	if (p_submit_depth_buffer &&
		OpenXRInteractionProfileExtension::is_profile_available(
			OpenXRInteractionProfileExtension::PROFILE_COMPOSITION_LAYER_DEPTH)) {
		data->render_state.depth_views.resize(p_view_count);

		for (uint32_t i = 0; i < p_view_count; i++) {
			data->render_state.depth_views[i] = {
				XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR, // type
				nullptr,								  // next
				{},										  // subImage
				0.0,									  // minDepth
				0.0,									  // maxDepth
				0.0,									  // nearZ
				0.0,									  // farZ
			};
		}
	}
}

void OpenXRAPI::_set_render_session_running_rt(bool p_is_running)
{
	// Must be called from rendering thread!
	ERR_NOT_ON_RENDER_THREAD;

	ERR_FAIL_COND(!OpenXRAPI::is_allocated());
	data->render_state.running = p_is_running;
}

void OpenXRAPI::_set_render_display_info_rt(XrTime p_predicted_display_time, bool p_should_render)
{
	// Must be called from rendering thread!
	ERR_NOT_ON_RENDER_THREAD;

	ERR_FAIL_COND(!OpenXRAPI::is_allocated());
	data->render_state.predicted_display_time = p_predicted_display_time;
	data->render_state.should_render = p_should_render;
}

void OpenXRAPI::_set_render_environment_blend_mode_rt(int32_t p_environment_blend_mode)
{
	// Must be called from rendering thread!
	ERR_NOT_ON_RENDER_THREAD;

	ERR_FAIL_COND(!OpenXRAPI::is_allocated());
	data->render_state.environment_blend_mode =
		XrEnvironmentBlendMode(p_environment_blend_mode);
}

void OpenXRAPI::_set_render_play_space_rt(uint64_t p_play_space)
{
	// Must be called from rendering thread!
	ERR_NOT_ON_RENDER_THREAD;

	ERR_FAIL_COND(!OpenXRAPI::is_allocated());
	data->render_state.play_space = XrSpace(p_play_space);
}

void OpenXRAPI::_set_render_state_multiplier_rt(double p_render_target_size_multiplier)
{
	// Must be called from rendering thread!
	ERR_NOT_ON_RENDER_THREAD;

	ERR_FAIL_COND(!OpenXRAPI::is_allocated());
	data->render_state.render_target_size_multiplier = p_render_target_size_multiplier;
}

void OpenXRAPI::_set_render_state_render_region_rt(const Rect2i& p_render_region)
{
	ERR_NOT_ON_RENDER_THREAD;

	ERR_FAIL_COND(!OpenXRAPI::is_allocated());
	data->render_state.render_region = p_render_region;
}

void OpenXRAPI::_update_main_swapchain_size_rt()
{
	ERR_NOT_ON_RENDER_THREAD;

	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	uint32_t view_count_output = 0;
	XrResult result = OpenXRAPI::xrEnumerateViewConfigurationViews(data->instance,
		data->system_id, data->view_configuration, OpenXRAPI::get_view_count(),
		&view_count_output, data->view_configuration_views.ptr());
	if (XR_FAILED(result)) {
		return;
	}

#ifdef DEBUG_ENABLED
	for (uint32_t i = 0; i < view_count_output; i++) {
		print_verbose("OpenXR: Recommended resolution changed");
		print_verbose(String(" - recommended render width: ") +
					  itos(data->view_configuration_views[i].recommendedImageRectWidth));
		print_verbose(String(" - recommended render height: ") +
					  itos(data->view_configuration_views[i].recommendedImageRectHeight));
	}
#endif
}

void OpenXRAPI::free_main_swapchains()
{
	for (int i = 0; i < OPENXR_SWAPCHAIN_MAX; i++) {
		data->render_state.main_swapchains[i].queue_free();
	}
}

XrSwapchain OpenXRAPI::get_color_swapchain()
{
	ERR_NOT_ON_RENDER_THREAD_V(XR_NULL_HANDLE);

	return data->render_state.main_swapchains[OPENXR_SWAPCHAIN_COLOR].get_swapchain();
}

RID OpenXRAPI::get_color_texture()
{
	ERR_NOT_ON_RENDER_THREAD_V(RID());

	return data->render_state.main_swapchains[OPENXR_SWAPCHAIN_COLOR].get_image();
}

RID OpenXRAPI::get_depth_texture()
{
	ERR_NOT_ON_RENDER_THREAD_V(RID());

	// Note, image will not be acquired if we didn't have a suitable swap chain format.
	if (data->render_state.submit_depth_buffer &&
		data->render_state.main_swapchains[OPENXR_SWAPCHAIN_DEPTH].is_image_acquired()) {
		return data->render_state.main_swapchains[OPENXR_SWAPCHAIN_DEPTH].get_image();
	}
	else {
		return RID();
	}
}

RID OpenXRAPI::get_density_map_texture()
{
	ERR_NOT_ON_RENDER_THREAD_V(RID());

	if (OpenXRFBFoveationExtension::is_initialized() && OpenXRFBFoveationExtension::is_enabled()) {
		return data->render_state.main_swapchains[OPENXR_SWAPCHAIN_COLOR].get_density_map();
	}

	return RID();
}

void OpenXRAPI::set_velocity_texture(RID p_render_target) { data->velocity_texture = p_render_target; }

RID OpenXRAPI::get_velocity_texture() { return data->velocity_texture; }

void OpenXRAPI::set_velocity_depth_texture(RID p_render_target)
{
	data->velocity_depth_texture = p_render_target;
}

RID OpenXRAPI::get_velocity_depth_texture() { return data->velocity_depth_texture; }

void OpenXRAPI::set_velocity_target_size(const Size2i& p_target_size)
{
	data->velocity_target_size = p_target_size;
}

Size2i OpenXRAPI::get_velocity_target_size() { return data->velocity_target_size; }

const XrCompositionLayerProjection* OpenXRAPI::get_projection_layer()
{
	ERR_NOT_ON_RENDER_THREAD_V(nullptr);

	return &data->render_state.projection_layer;
}

float OpenXRAPI::get_display_refresh_rate()
{
	if (OpenXRDisplayRefreshRateExtension::is_initialized()) {
		return OpenXRDisplayRefreshRateExtension::get_refresh_rate();
	}

	return 0.0;
}

void OpenXRAPI::set_display_refresh_rate(float p_refresh_rate)
{
	if (OpenXRDisplayRefreshRateExtension::is_initialized()) {
		OpenXRDisplayRefreshRateExtension::set_refresh_rate(p_refresh_rate);
	}
}

double OpenXRAPI::get_render_target_size_multiplier()
{
	return data->render_target_size_multiplier;
}

void OpenXRAPI::set_render_target_size_multiplier(double multiplier)
{
	data->render_target_size_multiplier = multiplier;
	set_render_state_multiplier(multiplier);
}

Rect2i OpenXRAPI::get_render_region() { return data->render_region; }

void OpenXRAPI::set_render_region(const Rect2i& p_render_region)
{
	data->render_region = p_render_region;
	set_render_state_render_region(p_render_region);
}

bool OpenXRAPI::is_foveation_supported()
{
	return OpenXRFBFoveationExtension::is_initialized() && OpenXRFBFoveationExtension::is_enabled();
}

int OpenXRAPI::get_foveation_level()
{
	if (OpenXRFBFoveationExtension::is_initialized() && OpenXRFBFoveationExtension::is_enabled()) {
		switch (OpenXRFBFoveationExtension::get_foveation_level()) {
		case XR_FOVEATION_LEVEL_NONE_FB:
			return 0;
		case XR_FOVEATION_LEVEL_LOW_FB:
			return 1;
		case XR_FOVEATION_LEVEL_MEDIUM_FB:
			return 2;
		case XR_FOVEATION_LEVEL_HIGH_FB:
			return 3;
		default:
			return 0;
		}
	}

	return 0;
}

void OpenXRAPI::set_foveation_level(int p_foveation_level)
{
	ERR_FAIL_UNSIGNED_INDEX(p_foveation_level, 4);
	if (OpenXRFBFoveationExtension::is_initialized() && OpenXRFBFoveationExtension::is_enabled()) {
		XrFoveationLevelFB levels[] = {XR_FOVEATION_LEVEL_NONE_FB, XR_FOVEATION_LEVEL_LOW_FB,
			XR_FOVEATION_LEVEL_MEDIUM_FB, XR_FOVEATION_LEVEL_HIGH_FB};
		OpenXRFBFoveationExtension::set_foveation_level(levels[p_foveation_level]);
	}
}

bool OpenXRAPI::get_foveation_dynamic()
{
	if (OpenXRFBFoveationExtension::is_initialized() && OpenXRFBFoveationExtension::is_enabled()) {
		return OpenXRFBFoveationExtension::get_foveation_dynamic() == XR_FOVEATION_DYNAMIC_LEVEL_ENABLED_FB;
	}
	return false;
}

void OpenXRAPI::set_foveation_dynamic(bool p_foveation_dynamic)
{
	if (OpenXRFBFoveationExtension::is_initialized() && OpenXRFBFoveationExtension::is_enabled()) {
		OpenXRFBFoveationExtension::set_foveation_dynamic(p_foveation_dynamic ? XR_FOVEATION_DYNAMIC_LEVEL_ENABLED_FB
														   : XR_FOVEATION_DYNAMIC_DISABLED_FB);
	}
}

bool OpenXRAPI::get_foveation_with_subsampled_images()
{
	if (OpenXRFBFoveationExtension::is_initialized()) {
		return OpenXRFBFoveationExtension::is_foveation_with_subsampled_images_enabled();
	}
	return false;
}

void OpenXRAPI::set_foveation_with_subsampled_images(bool p_enabled)
{
	if (OpenXRFBFoveationExtension::is_initialized()) {
		OpenXRFBFoveationExtension::set_foveation_with_subsampled_images_enabled(p_enabled);
	}
}

PackedInt64Array OpenXRAPI::get_supported_swapchain_formats()
{
	return data->supported_swapchain_formats;
}

Transform3D OpenXRAPI::transform_from_pose(const XrPosef& p_pose)
{
	Quaternion q(
		p_pose.orientation.x, p_pose.orientation.y, p_pose.orientation.z, p_pose.orientation.w);
	Basis basis(q);
	Vector3 origin(p_pose.position.x, p_pose.position.y, p_pose.position.z);

	return Transform3D(basis, origin);
}

XrPosef OpenXRAPI::pose_from_transform(const Transform3D& p_transform)
{
	XrPosef pose;

	Quaternion q(p_transform.basis);
	pose.orientation.x = q.x;
	pose.orientation.y = q.y;
	pose.orientation.z = q.z;
	pose.orientation.w = q.w;

	pose.position.x = p_transform.origin.x;
	pose.position.y = p_transform.origin.y;
	pose.position.z = p_transform.origin.z;

	return pose;
}

template <typename T>
XRPose::TrackingConfidence _transform_from_location(const T& p_location, Transform3D& r_transform)
{
	XRPose::TrackingConfidence confidence = XRPose::XR_TRACKING_CONFIDENCE_NONE;
	const XrPosef& pose = p_location.pose;

	// Check orientation
	if (p_location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) {
		Quaternion q(
			pose.orientation.x, pose.orientation.y, pose.orientation.z, pose.orientation.w);
		r_transform.basis = Basis(q);

		if (p_location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT) {
			// Fully valid orientation, so either 3DOF or 6DOF tracking with high confidence so
			// default to HIGH_TRACKING
			confidence = XRPose::XR_TRACKING_CONFIDENCE_HIGH;
		}
		else {
			// Orientation is being tracked but we're using old/predicted data, so low tracking
			// confidence
			confidence = XRPose::XR_TRACKING_CONFIDENCE_LOW;
		}
	}
	else {
		r_transform.basis = Basis();
	}

	// Check location
	if (p_location.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) {
		r_transform.origin = Vector3(pose.position.x, pose.position.y, pose.position.z);

		if (!(p_location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT)) {
			// Location is being tracked but we're using old/predicted data, so low tracking
			// confidence
			confidence = XRPose::XR_TRACKING_CONFIDENCE_LOW;
		}
		else if (confidence == XRPose::XR_TRACKING_CONFIDENCE_NONE) {
			// Position tracking without orientation tracking?
			confidence = XRPose::XR_TRACKING_CONFIDENCE_HIGH;
		}
	}
	else {
		// No tracking or 3DOF I guess..
		r_transform.origin = Vector3();
	}

	return confidence;
}

XRPose::TrackingConfidence OpenXRAPI::transform_from_location(
	const XrSpaceLocation& p_location, Transform3D& r_transform)
{
	return _transform_from_location(p_location, r_transform);
}

XRPose::TrackingConfidence OpenXRAPI::transform_from_location(
	const XrHandJointLocationEXT& p_location, Transform3D& r_transform)
{
	return _transform_from_location(p_location, r_transform);
}

void OpenXRAPI::parse_velocities(
	const XrSpaceVelocity& p_velocity, Vector3& r_linear_velocity, Vector3& r_angular_velocity)
{
	if (p_velocity.velocityFlags & XR_SPACE_VELOCITY_LINEAR_VALID_BIT) {
		XrVector3f linear_velocity = p_velocity.linearVelocity;
		r_linear_velocity = Vector3(linear_velocity.x, linear_velocity.y, linear_velocity.z);
	}
	else {
		r_linear_velocity = Vector3();
	}
	if (p_velocity.velocityFlags & XR_SPACE_VELOCITY_ANGULAR_VALID_BIT) {
		XrVector3f angular_velocity = p_velocity.angularVelocity;
		r_angular_velocity = Vector3(angular_velocity.x, angular_velocity.y, angular_velocity.z);
	}
	else {
		r_angular_velocity = Vector3();
	}
}

XrPath OpenXRAPI::get_xr_path(const String& p_path) { return XrPath(); }

String OpenXRAPI::get_xr_path_name(const XrPath& p_path)
{
	ERR_FAIL_COND_V(data->instance == XR_NULL_HANDLE, String());

	uint32_t size = 0;
	char path_name[XR_MAX_PATH_LENGTH];

	XrResult result = xrPathToString(data->instance, p_path, XR_MAX_PATH_LENGTH, &size, path_name);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(
			String(), "OpenXR: failed to get name for a path! [" + get_error_string(result) + "]");
	}

	return String(path_name);
}

RID OpenXRAPI::get_tracker_rid(XrPath p_path)
{
	for (const RID& tracker_rid : data->tracker_owner.get_owned_list()) {
		Tracker* tracker = data->tracker_owner.get_or_null(tracker_rid);
		if (tracker && tracker->toplevel_path == p_path) {
			return tracker_rid;
		}
	}

	return RID();
}

RID OpenXRAPI::find_tracker(const String& p_name)
{
	for (const RID& tracker_rid : data->tracker_owner.get_owned_list()) {
		Tracker* tracker = data->tracker_owner.get_or_null(tracker_rid);
		if (tracker && tracker->name == p_name) {
			return tracker_rid;
		}
	}

	return RID();
}

RID OpenXRAPI::tracker_create(const String& p_name)
{
	ERR_FAIL_COND_V(data->instance == XR_NULL_HANDLE, RID());

	Tracker new_tracker;
	new_tracker.name = p_name;
	new_tracker.toplevel_path = XR_NULL_PATH;
	new_tracker.active_profile_rid = RID();

	new_tracker.toplevel_path = get_xr_path(p_name);
	ERR_FAIL_COND_V(new_tracker.toplevel_path == XR_NULL_PATH, RID());

	return data->tracker_owner.make_rid(new_tracker);
}

String OpenXRAPI::tracker_get_name(RID p_tracker)
{
	if (p_tracker.is_null()) {
		return String("None");
	}

	Tracker* tracker = data->tracker_owner.get_or_null(p_tracker);
	ERR_FAIL_NULL_V(tracker, String());

	return tracker->name;
}

void OpenXRAPI::tracker_check_profile(RID p_tracker, XrSession p_session)
{
	if (p_session == XR_NULL_HANDLE) {
		p_session = data->session;
	}

	Tracker* tracker = data->tracker_owner.get_or_null(p_tracker);
	ERR_FAIL_NULL(tracker);

	if (tracker->toplevel_path == XR_NULL_PATH) {
		// no path, how was this even created?
		return;
	}

	XrInteractionProfileState profile_state = {
		XR_TYPE_INTERACTION_PROFILE_STATE, // type
		nullptr,						   // next
		XR_NULL_PATH					   // interactionProfile
	};

	XrResult result =
		xrGetCurrentInteractionProfile(p_session, tracker->toplevel_path, &profile_state);
	if (XR_FAILED(result)) {
		// print_line("OpenXR: Failed to get interaction profile for", itos(tracker->toplevel_path),
		// 	"[", get_error_string(result), "]");
		return;
	}

	XrPath new_profile = profile_state.interactionProfile;
	XrPath was_profile = get_interaction_profile_path(tracker->active_profile_rid);
	if (was_profile != new_profile) {
		tracker->active_profile_rid = get_interaction_profile_rid(new_profile);

		if (data->xr_interface) {
			data->xr_interface->tracker_profile_changed(p_tracker, tracker->active_profile_rid);
		}
	}
}

void OpenXRAPI::tracker_free(RID p_tracker)
{
	Tracker* tracker = data->tracker_owner.get_or_null(p_tracker);
	ERR_FAIL_NULL(tracker);

	// there is nothing to free here

	data->tracker_owner.free(p_tracker);
}

RID OpenXRAPI::action_set_create(
	const String& p_name, const String& p_localized_name, const int p_priority)
{
	ERR_FAIL_COND_V(data->instance == XR_NULL_HANDLE, RID());
	ActionSet action_set;

	action_set.name = p_name;
	action_set.is_attached = false;

	// create our action set...
	XrActionSetCreateInfo action_set_info = {
		XR_TYPE_ACTION_SET_CREATE_INFO, // type
		nullptr,						// next
		"",								// actionSetName
		"",								// localizedActionSetName
		uint32_t(p_priority)			// priority
	};

	copy_string_to_char_buffer(p_name, action_set_info.actionSetName, XR_MAX_ACTION_SET_NAME_SIZE);
	copy_string_to_char_buffer(p_localized_name, action_set_info.localizedActionSetName,
		XR_MAX_LOCALIZED_ACTION_SET_NAME_SIZE);

	XrResult result = xrCreateActionSet(data->instance, &action_set_info, &action_set.handle);
	if (XR_FAILED(result)) {
		// print_line(
		// 	"OpenXR: failed to create action set ", p_name, "! [", get_error_string(result), "]");
		return RID();
	}

	set_object_name(XR_OBJECT_TYPE_ACTION_SET, uint64_t(action_set.handle), p_name);

	return data->action_set_owner.make_rid(action_set);
}

RID OpenXRAPI::find_action_set(const String& p_name)
{
	for (const RID& action_set_rid : data->action_set_owner.get_owned_list()) {
		ActionSet* action_set = data->action_set_owner.get_or_null(action_set_rid);
		if (action_set && action_set->name == p_name) {
			return action_set_rid;
		}
	}

	return RID();
}

String OpenXRAPI::action_set_get_name(RID p_action_set)
{
	if (p_action_set.is_null()) {
		return String("None");
	}

	ActionSet* action_set = data->action_set_owner.get_or_null(p_action_set);
	ERR_FAIL_NULL_V(action_set, String());

	return action_set->name;
}

XrActionSet OpenXRAPI::action_set_get_handle(RID p_action_set)
{
	if (p_action_set.is_null()) {
		return XR_NULL_HANDLE;
	}

	ActionSet* action_set = data->action_set_owner.get_or_null(p_action_set);
	ERR_FAIL_NULL_V(action_set, XR_NULL_HANDLE);

	return action_set->handle;
}

bool OpenXRAPI::attach_action_sets(const Vector<RID>& p_action_sets)
{
	ERR_FAIL_COND_V(data->session == XR_NULL_HANDLE, false);

	Vector<XrActionSet> action_handles;
	action_handles.resize(p_action_sets.size());
	for (int i = 0; i < p_action_sets.size(); i++) {
		ActionSet* action_set = data->action_set_owner.get_or_null(p_action_sets[i]);
		ERR_FAIL_NULL_V(action_set, false);

		if (action_set->is_attached) {
			return false;
		}

		action_handles.set(i, action_set->handle);
	}

	// So according to the docs, once we attach our action set to our session it becomes read only..
	// https://www.khronos.org/registry/OpenXR/specs/1.0/man/html/xrAttachSessionActionSets.html
	XrSessionActionSetsAttachInfo attach_info = {
		XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO, // type
		nullptr,								 // next
		(uint32_t)p_action_sets.size(),			 // countActionSets,
		action_handles.ptr()					 // actionSets
	};

	XrResult result = xrAttachSessionActionSets(data->session, &attach_info);
	if (XR_FAILED(result)) {
		// print_line("OpenXR: failed to attach action sets! [", get_error_string(result), "]");
		return false;
	}

	for (int i = 0; i < p_action_sets.size(); i++) {
		ActionSet* action_set = data->action_set_owner.get_or_null(p_action_sets[i]);
		ERR_FAIL_NULL_V(action_set, false);
		action_set->is_attached = true;
	}

	/* For debugging:
	print_verbose("Attached set " + action_set->name);
	List<RID> action_rids;
	action_owner.get_owned_list(&action_rids);
	for (int i = 0; i < action_rids.size(); i++) {
		Action * action = action_owner.get_or_null(action_rids[i]);
		if (action && action->action_set_rid == p_action_set) {
			print_verbose(" - Action " + action->name + ": " +
	OpenXRUtil::get_action_type_name(action->action_type)); for (int j = 0; j <
	action->trackers.size(); j++) { Tracker * tracker =
	tracker_owner.get_or_null(action->trackers[j].tracker_rid); if (tracker) { print_verbose("    -
	" + tracker->name);
				}
			}
		}
	}
	*/

	return true;
}

void OpenXRAPI::action_set_free(RID p_action_set)
{
	ActionSet* action_set = data->action_set_owner.get_or_null(p_action_set);
	ERR_FAIL_NULL(action_set);

	if (action_set->handle != XR_NULL_HANDLE) {
		xrDestroyActionSet(action_set->handle);
	}

	data->action_set_owner.free(p_action_set);
}

RID OpenXRAPI::get_action_rid(XrAction p_action)
{
	for (const RID& action_rid : data->action_owner.get_owned_list()) {
		Action* action = data->action_owner.get_or_null(action_rid);
		if (action && action->handle == p_action) {
			return action_rid;
		}
	}

	return RID();
}

RID OpenXRAPI::find_action(const String& p_name, const RID& p_action_set)
{
	for (const RID& action_rid : data->action_owner.get_owned_list()) {
		Action* action = data->action_owner.get_or_null(action_rid);
		if (action && action->name == p_name &&
			(p_action_set.is_null() || action->action_set_rid == p_action_set)) {
			return action_rid;
		}
	}

	return RID();
}

RID OpenXRAPI::action_create(RID p_action_set, const String& p_name, const String& p_localized_name,
	OpenXRAction::ActionType p_action_type, const Vector<RID>& p_trackers)
{
	ERR_FAIL_COND_V(data->instance == XR_NULL_HANDLE, RID());

	Action action;
	action.name = p_name;

	ActionSet* action_set = data->action_set_owner.get_or_null(p_action_set);
	ERR_FAIL_NULL_V(action_set, RID());
	ERR_FAIL_COND_V(action_set->handle == XR_NULL_HANDLE, RID());
	action.action_set_rid = p_action_set;

	switch (p_action_type) {
	case OpenXRAction::OPENXR_ACTION_BOOL:
		action.action_type = XR_ACTION_TYPE_BOOLEAN_INPUT;
		break;
	case OpenXRAction::OPENXR_ACTION_FLOAT:
		action.action_type = XR_ACTION_TYPE_FLOAT_INPUT;
		break;
	case OpenXRAction::OPENXR_ACTION_VECTOR2:
		action.action_type = XR_ACTION_TYPE_VECTOR2F_INPUT;
		break;
	case OpenXRAction::OPENXR_ACTION_POSE:
		action.action_type = XR_ACTION_TYPE_POSE_INPUT;
		break;
	case OpenXRAction::OPENXR_ACTION_HAPTIC:
		action.action_type = XR_ACTION_TYPE_VIBRATION_OUTPUT;
		break;
	default:
		ERR_FAIL_V(RID());
		break;
	}

	Vector<XrPath> toplevel_paths;
	for (int i = 0; i < p_trackers.size(); i++) {
		Tracker* tracker = data->tracker_owner.get_or_null(p_trackers[i]);
		if (tracker != nullptr && tracker->toplevel_path != XR_NULL_PATH) {
			ActionTracker action_tracker = {
				p_trackers[i],	// tracker
				XR_NULL_HANDLE, // space
				false			// was_location_valid
			};
			action.trackers.push_back(action_tracker);

			toplevel_paths.push_back(tracker->toplevel_path);
		}
	}

	XrActionCreateInfo action_info = {
		XR_TYPE_ACTION_CREATE_INFO,		 // type
		nullptr,						 // next
		"",								 // actionName
		action.action_type,				 // actionType
		uint32_t(toplevel_paths.size()), // countSubactionPaths
		toplevel_paths.ptr(),			 // subactionPaths
		""								 // localizedActionName
	};

	copy_string_to_char_buffer(p_name, action_info.actionName, XR_MAX_ACTION_NAME_SIZE);
	copy_string_to_char_buffer(
		p_localized_name, action_info.localizedActionName, XR_MAX_LOCALIZED_ACTION_NAME_SIZE);

	XrResult result = xrCreateAction(action_set->handle, &action_info, &action.handle);
	if (XR_FAILED(result)) {
		// print_line(
		// 	"OpenXR: failed to create action ", p_name, "! [", get_error_string(result), "]");
		return RID();
	}

	set_object_name(XR_OBJECT_TYPE_ACTION, uint64_t(action.handle), p_name);

	return data->action_owner.make_rid(action);
}

String OpenXRAPI::action_get_name(RID p_action)
{
	if (p_action.is_null()) {
		return String("None");
	}

	Action* action = data->action_owner.get_or_null(p_action);
	ERR_FAIL_NULL_V(action, String());

	return action->name;
}

XrAction OpenXRAPI::action_get_handle(RID p_action)
{
	if (p_action.is_null()) {
		return XR_NULL_HANDLE;
	}

	Action* action = data->action_owner.get_or_null(p_action);
	ERR_FAIL_NULL_V(action, XR_NULL_HANDLE);

	return action->handle;
}

void OpenXRAPI::action_free(RID p_action)
{
	Action* action = data->action_owner.get_or_null(p_action);
	ERR_FAIL_NULL(action);

	if (action->handle != XR_NULL_HANDLE) {
		xrDestroyAction(action->handle);
	}

	data->action_owner.free(p_action);
}

RID OpenXRAPI::get_interaction_profile_rid(XrPath p_path)
{
	for (const RID& ip_rid : data->interaction_profile_owner.get_owned_list()) {
		InteractionProfile* ip = data->interaction_profile_owner.get_or_null(ip_rid);
		if (ip && ip->path == p_path) {
			return ip_rid;
		}
	}

	return RID();
}

XrPath OpenXRAPI::get_interaction_profile_path(RID p_interaction_profile)
{
	if (p_interaction_profile.is_null()) {
		return XR_NULL_PATH;
	}

	InteractionProfile* ip = data->interaction_profile_owner.get_or_null(p_interaction_profile);
	ERR_FAIL_NULL_V(ip, XR_NULL_PATH);

	return ip->path;
}

RID OpenXRAPI::interaction_profile_create(const String& p_name)
{
	if (!is_interaction_profile_supported(p_name)) {
		// The extension enabling this path must not be active, we will silently skip this
		// interaction profile
		return RID();
	}

	InteractionProfile new_interaction_profile;

	new_interaction_profile.internal_name = get_interaction_profile_internal_name(p_name);

	XrResult result = xrStringToPath(
		data->instance, new_interaction_profile.internal_name.get_data(), &new_interaction_profile.path);
	if (XR_FAILED(result)) {
		// print_line("OpenXR: failed to get path for ", p_name, "! [", get_error_string(result),
		// "]");
		return RID();
	}

	RID existing_ip = get_interaction_profile_rid(new_interaction_profile.path);
	if (existing_ip.is_valid()) {
		return existing_ip;
	}

	new_interaction_profile.name = p_name;
	return data->interaction_profile_owner.make_rid(new_interaction_profile);
}

String OpenXRAPI::interaction_profile_get_name(RID p_interaction_profile)
{
	if (p_interaction_profile.is_null()) {
		return String("None");
	}

	InteractionProfile* ip = data->interaction_profile_owner.get_or_null(p_interaction_profile);
	ERR_FAIL_NULL_V(ip, String());

	return ip->name;
}

void OpenXRAPI::interaction_profile_clear_bindings(RID p_interaction_profile)
{
	InteractionProfile* ip = data->interaction_profile_owner.get_or_null(p_interaction_profile);
	ERR_FAIL_NULL(ip);

	ip->bindings.clear();
}

CharString OpenXRAPI::get_interaction_profile_internal_name(
	const String& p_interaction_profile_name)
{
	CharString internal_name = p_interaction_profile_name.utf8();

	if (data->openxr_version < XR_API_VERSION_1_1_0) {
		// These interaction profiles were renamed in OpenXR 1.1,
		// if we don't support OpenXR 1.1, rename them back.
		if (internal_name == "/interaction_profiles/meta/touch_pro_controller") {
			return "/interaction_profiles/facebook/touch_controller_pro";
		}
		else if (internal_name == "/interaction_profiles/meta/touch_plus_controller") {
			return "/interaction_profiles/meta/touch_controller_plus";
		}
	}

	return internal_name;
}

const char* OpenXRAPI::check_profile_path(
	const CharString& p_interaction_profile_name, const char* p_path)
{
	// We store the new names of these paths in our action map, so if we're on older versions of
	// OpenXR, we need to use the old names.

	struct RenameMap
	{
		const XrVersion
			before_version;	 // If we're on an older version of OpenXR than this (if none zero).
		const char* from;	 // Rename from this
		const char* to;		 // to this
		const char* profile; // limiting to this profile (unless nullptr)
	};

	// The order of entries is important as we early exit when we encounter a before_version value
	// before our current value (excluding 0).
	const RenameMap renames[] = {
		// The touch_controller is an exception where it is still using the vendor names because
		// they were introduced after OpenXR 1.1 was defined.
		{0, "/user/hand/left/input/trigger/proximity", "/user/hand/left/input/trigger/proximity_fb",
			"/interaction_profiles/oculus/touch_controller"},
		{0, "/user/hand/right/input/trigger/proximity",
			"/user/hand/right/input/trigger/proximity_fb",
			"/interaction_profiles/oculus/touch_controller"},
		{0, "/user/hand/left/input/thumb_resting_surfaces/proximity",
			"/user/hand/left/input/thumb_fb/proximity_fb",
			"/interaction_profiles/oculus/touch_controller"},
		{0, "/user/hand/right/input/thumb_resting_surfaces/proximity",
			"/user/hand/right/input/thumb_fb/proximity_fb",
			"/interaction_profiles/oculus/touch_controller"},

		// Once applicable, add XR_API_VERSION_1_2 here.

		// Before OpenXR 1.1 we're using palm_ext/pose (we're checking for enabled palm pose
		// extension elsewhere).
		{XR_API_VERSION_1_1_0, "/user/hand/left/input/grip_surface/pose",
			"/user/hand/left/input/palm_ext/pose", nullptr},
		{XR_API_VERSION_1_1_0, "/user/hand/right/input/grip_surface/pose",
			"/user/hand/right/input/palm_ext/pose", nullptr},

		// Specific renames for touch_controller_pro, note that we would have already renamed it to
		// the old name.
		{XR_API_VERSION_1_1_0, "/user/hand/left/input/grip_surface/pose",
			"/user/hand/left/input/palm_ext/pose",
			"/interaction_profiles/facebook/touch_controller_pro"},
		{XR_API_VERSION_1_1_0, "/user/hand/right/input/grip_surface/pose",
			"/user/hand/right/input/palm_ext/pose",
			"/interaction_profiles/facebook/touch_controller_pro"},
		{XR_API_VERSION_1_1_0, "/user/hand/left/input/stylus/force",
			"/user/hand/left/input/stylus_fb/force",
			"/interaction_profiles/facebook/touch_controller_pro"},
		{XR_API_VERSION_1_1_0, "/user/hand/right/input/stylus/force",
			"/user/hand/right/input/stylus_fb/force",
			"/interaction_profiles/facebook/touch_controller_pro"},
		{XR_API_VERSION_1_1_0, "/user/hand/left/input/trigger/proximity",
			"/user/hand/left/input/trigger/proximity_fb",
			"/interaction_profiles/facebook/touch_controller_pro"},
		{XR_API_VERSION_1_1_0, "/user/hand/right/input/trigger/proximity",
			"/user/hand/right/input/trigger/proximity_fb",
			"/interaction_profiles/facebook/touch_controller_pro"},
		{XR_API_VERSION_1_1_0, "/user/hand/left/output/haptic_trigger",
			"/user/hand/left/output/haptic_trigger_fb",
			"/interaction_profiles/facebook/touch_controller_pro"},
		{XR_API_VERSION_1_1_0, "/user/hand/right/output/haptic_trigger",
			"/user/hand/right/output/haptic_trigger_fb",
			"/interaction_profiles/facebook/touch_controller_pro"},
		{XR_API_VERSION_1_1_0, "/user/hand/left/output/haptic_thumb",
			"/user/hand/left/output/haptic_thumb_fb",
			"/interaction_profiles/facebook/touch_controller_pro"},
		{XR_API_VERSION_1_1_0, "/user/hand/right/output/haptic_thumb",
			"/user/hand/right/output/haptic_thumb_fb",
			"/interaction_profiles/facebook/touch_controller_pro"},
		{XR_API_VERSION_1_1_0, "/user/hand/left/input/thumb_resting_surfaces/proximity",
			"/user/hand/left/input/thumb_fb/proximity_fb",
			"/interaction_profiles/facebook/touch_controller_pro"},
		{XR_API_VERSION_1_1_0, "/user/hand/right/input/thumb_resting_surfaces/proximity",
			"/user/hand/right/input/thumb_fb/proximity_fb",
			"/interaction_profiles/facebook/touch_controller_pro"},
		{XR_API_VERSION_1_1_0, "/user/hand/left/input/trigger_curl/value",
			"/user/hand/left/input/trigger/curl_fb",
			"/interaction_profiles/facebook/touch_controller_pro"},
		{XR_API_VERSION_1_1_0, "/user/hand/right/input/trigger_curl/value",
			"/user/hand/right/input/trigger/curl_fb",
			"/interaction_profiles/facebook/touch_controller_pro"},
		{XR_API_VERSION_1_1_0, "/user/hand/left/input/trigger_slide/value",
			"/user/hand/left/input/trigger/slide_fb",
			"/interaction_profiles/facebook/touch_controller_pro"},
		{XR_API_VERSION_1_1_0, "/user/hand/right/input/trigger_slide/value",
			"/user/hand/right/input/trigger/slide_fb",
			"/interaction_profiles/facebook/touch_controller_pro"},

		// Specific renames for touch_controller_plus, note that we would have already renamed it to
		// the old name.
		{XR_API_VERSION_1_1_0, "/user/hand/left/input/trigger/proximity",
			"/user/hand/left/input/trigger/proximity_meta",
			"/interaction_profiles/facebook/touch_controller_plus"},
		{XR_API_VERSION_1_1_0, "/user/hand/right/input/trigger/proximity",
			"/user/hand/right/input/trigger/proximity_meta",
			"/interaction_profiles/facebook/touch_controller_plus"},
		{XR_API_VERSION_1_1_0, "/user/hand/left/input/thumb_resting_surfaces/proximity",
			"/user/hand/left/input/thumb_meta/proximity_meta",
			"/interaction_profiles/facebook/touch_controller_plus"},
		{XR_API_VERSION_1_1_0, "/user/hand/right/input/thumb_resting_surfaces/proximity",
			"/user/hand/right/input/thumb_meta/proximity_meta",
			"/interaction_profiles/facebook/touch_controller_plus"},
		{XR_API_VERSION_1_1_0, "/user/hand/left/input/trigger_curl/value",
			"/user/hand/left/input/trigger/curl_meta",
			"/interaction_profiles/facebook/touch_controller_plus"},
		{XR_API_VERSION_1_1_0, "/user/hand/right/input/trigger_curl/value",
			"/user/hand/right/input/trigger/curl_meta",
			"/interaction_profiles/facebook/touch_controller_plus"},
		{XR_API_VERSION_1_1_0, "/user/hand/left/input/trigger_slide/value",
			"/user/hand/left/input/trigger/slide_meta",
			"/interaction_profiles/facebook/touch_controller_plus"},
		{XR_API_VERSION_1_1_0, "/user/hand/right/input/trigger_slide/value",
			"/user/hand/right/input/trigger/slide_meta",
			"/interaction_profiles/facebook/touch_controller_plus"},
	};
	constexpr size_t length = sizeof(renames) / sizeof(renames[0]);

	for (size_t i = 0; i < length; i++) {
		const RenameMap& rename = renames[i];

		if (rename.before_version != 0 && data->openxr_version >= rename.before_version) {
			// We're done, we are on a new version than this, no need to check further.
			return p_path;
		}

		if ((rename.profile == nullptr || p_interaction_profile_name == rename.profile) &&
			strcmp(p_path, rename.from) == 0) {
			return rename.to;
		}
	}

	return p_path;
}

int OpenXRAPI::interaction_profile_add_binding(
	RID p_interaction_profile, RID p_action, const String& p_path)
{
	InteractionProfile* ip = data->interaction_profile_owner.get_or_null(p_interaction_profile);
	ERR_FAIL_NULL_V(ip, -1);

	if (!interaction_profile_supports_io_path(ip->name, p_path)) {
		return -1;
	}

	XrActionSuggestedBinding binding;

	Action* action = data->action_owner.get_or_null(p_action);
	ERR_FAIL_COND_V(action == nullptr || action->handle == XR_NULL_HANDLE, -1);

	binding.action = action->handle;

	XrResult result = xrStringToPath(data->instance,
		check_profile_path(ip->internal_name, p_path.utf8().get_data()), &binding.binding);
	if (XR_FAILED(result)) {
		// print_line("OpenXR: failed to get path for ", p_path, "! [", get_error_string(result),
		// "]");
		return -1;
	}

	ip->bindings.push_back(binding);

	return ip->bindings.size() - 1;
}

bool OpenXRAPI::interaction_profile_add_modifier(
	RID p_interaction_profile, const PackedByteArray& p_modifier)
{
	InteractionProfile* ip = data->interaction_profile_owner.get_or_null(p_interaction_profile);
	ERR_FAIL_NULL_V(ip, false);

	if (!p_modifier.is_empty()) {
		// Add it to our stack.
		ip->modifiers.push_back(p_modifier);
	}

	return true;
}

bool OpenXRAPI::interaction_profile_suggest_bindings(RID p_interaction_profile)
{
	ERR_FAIL_COND_V(data->instance == XR_NULL_HANDLE, false);

	InteractionProfile* ip = data->interaction_profile_owner.get_or_null(p_interaction_profile);
	ERR_FAIL_NULL_V(ip, false);

	void* next = nullptr;

	// Note, extensions should only add binding modifiers if they are supported, else this may fail.
	XrBindingModificationsKHR binding_modifiers;
	Vector<const XrBindingModificationBaseHeaderKHR*> modifiers;
	if (!ip->modifiers.is_empty()) {
		for (const PackedByteArray& modifier : ip->modifiers) {
			const XrBindingModificationBaseHeaderKHR* ptr =
				(const XrBindingModificationBaseHeaderKHR*)modifier.ptr();
			modifiers.push_back(ptr);
		}

		binding_modifiers.type = XR_TYPE_BINDING_MODIFICATIONS_KHR;
		binding_modifiers.next = next;
		binding_modifiers.bindingModificationCount = modifiers.size();
		binding_modifiers.bindingModifications = modifiers.ptr();
		next = &binding_modifiers;
	}

	const XrInteractionProfileSuggestedBinding suggested_bindings = {
		XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING, // type
		next,										   // next
		ip->path,									   // interactionProfile
		uint32_t(ip->bindings.size()),				   // countSuggestedBindings
		ip->bindings.ptr()							   // suggestedBindings
	};

	XrResult result = xrSuggestInteractionProfileBindings(data->instance, &suggested_bindings);
	if (result == XR_ERROR_PATH_UNSUPPORTED) {
		// this is fine, not all runtimes support all devices.
		print_verbose(
			"OpenXR Interaction profile " + ip->name + " is not supported on this runtime");
	}
	else if (XR_FAILED(result)) {
		// print_line("OpenXR: failed to suggest bindings for ", ip->name, "! [",
		// get_error_string(result), "]");
		// reporting is enough...
	}

	/* For debugging:
	print_verbose("Suggested bindings for " + ip->name);
	for (int i = 0; i < ip->bindings.size(); i++) {
		uint32_t strlen;
		char path[XR_MAX_PATH_LENGTH];

		String action_name = action_get_name(get_action_rid(ip->bindings[i].action));

		XrResult result = xrPathToString(instance, ip->bindings[i].binding, XR_MAX_PATH_LENGTH,
	&strlen, path); if (XR_FAILED(result)) { print_line("OpenXR: failed to retrieve bindings for ",
	action_name, "! [", get_error_string(result), "]");
		}
		print_verbose(" - " + action_name + " => " + String(path));
	}
	*/

	return true;
}

void OpenXRAPI::interaction_profile_free(RID p_interaction_profile)
{
	InteractionProfile* ip = data->interaction_profile_owner.get_or_null(p_interaction_profile);
	ERR_FAIL_NULL(ip);

	ip->bindings.clear();
	ip->modifiers.clear();

	data->interaction_profile_owner.free(p_interaction_profile);
}

bool OpenXRAPI::get_action_bool(RID p_action, RID p_tracker)
{
	ERR_FAIL_COND_V(data->session == XR_NULL_HANDLE, false);
	Action* action = data->action_owner.get_or_null(p_action);
	ERR_FAIL_NULL_V(action, false);
	Tracker* tracker = data->tracker_owner.get_or_null(p_tracker);
	ERR_FAIL_NULL_V(tracker, false);

	if (!data->running) {
		return false;
	}

	ERR_FAIL_COND_V(action->action_type != XR_ACTION_TYPE_BOOLEAN_INPUT, false);

	XrActionStateGetInfo get_info = {
		XR_TYPE_ACTION_STATE_GET_INFO, // type
		nullptr,					   // next
		action->handle,				   // action
		tracker->toplevel_path		   // subactionPath
	};

	XrActionStateBoolean result_state;
	result_state.type = XR_TYPE_ACTION_STATE_BOOLEAN, result_state.next = nullptr;
	XrResult result = xrGetActionStateBoolean(data->session, &get_info, &result_state);
	if (XR_FAILED(result)) {
		// print_line("OpenXR: couldn't get action boolean! [", get_error_string(result), "]");
		return false;
	}

	return result_state.isActive && result_state.currentState;
}

float OpenXRAPI::get_action_float(RID p_action, RID p_tracker)
{
	ERR_FAIL_COND_V(data->session == XR_NULL_HANDLE, 0.0);
	Action* action = data->action_owner.get_or_null(p_action);
	ERR_FAIL_NULL_V(action, 0.0);
	Tracker* tracker = data->tracker_owner.get_or_null(p_tracker);
	ERR_FAIL_NULL_V(tracker, 0.0);

	if (!data->running) {

		return 0.0;
	}

	ERR_FAIL_COND_V(action->action_type != XR_ACTION_TYPE_FLOAT_INPUT, 0.0);

	XrActionStateGetInfo get_info = {
		XR_TYPE_ACTION_STATE_GET_INFO, // type
		nullptr,					   // next
		action->handle,				   // action
		tracker->toplevel_path		   // subactionPath
	};

	XrActionStateFloat result_state;
	result_state.type = XR_TYPE_ACTION_STATE_FLOAT, result_state.next = nullptr;
	XrResult result = xrGetActionStateFloat(data->session, &get_info, &result_state);
	if (XR_FAILED(result)) {
		// print_line("OpenXR: couldn't get action float! [", get_error_string(result), "]");
		return 0.0;
	}

	return result_state.isActive ? result_state.currentState : 0.0;
}

Vector2 OpenXRAPI::get_action_vector2(RID p_action, RID p_tracker)
{
	ERR_FAIL_COND_V(data->session == XR_NULL_HANDLE, Vector2());
	Action* action = data->action_owner.get_or_null(p_action);
	ERR_FAIL_NULL_V(action, Vector2());
	Tracker* tracker = data->tracker_owner.get_or_null(p_tracker);
	ERR_FAIL_NULL_V(tracker, Vector2());

	if (!data->running) {
		return Vector2();
	}

	ERR_FAIL_COND_V(action->action_type != XR_ACTION_TYPE_VECTOR2F_INPUT, Vector2());

	XrActionStateGetInfo get_info = {
		XR_TYPE_ACTION_STATE_GET_INFO, // type
		nullptr,					   // next
		action->handle,				   // action
		tracker->toplevel_path		   // subactionPath
	};

	XrActionStateVector2f result_state;
	result_state.type = XR_TYPE_ACTION_STATE_VECTOR2F, result_state.next = nullptr;
	XrResult result = xrGetActionStateVector2f(data->session, &get_info, &result_state);
	if (XR_FAILED(result)) {
		// print_line("OpenXR: couldn't get action vector2! [", get_error_string(result), "]");
		return Vector2();
	}

	return result_state.isActive ? Vector2(result_state.currentState.x, result_state.currentState.y)
								 : Vector2();
}

XRPose::TrackingConfidence OpenXRAPI::get_action_pose(RID p_action, RID p_tracker,
	Transform3D& r_transform, Vector3& r_linear_velocity, Vector3& r_angular_velocity)
{
	ERR_FAIL_COND_V(data->session == XR_NULL_HANDLE, XRPose::XR_TRACKING_CONFIDENCE_NONE);
	Action* action = data->action_owner.get_or_null(p_action);
	ERR_FAIL_NULL_V(action, XRPose::XR_TRACKING_CONFIDENCE_NONE);
	Tracker* tracker = data->tracker_owner.get_or_null(p_tracker);
	ERR_FAIL_NULL_V(tracker, XRPose::XR_TRACKING_CONFIDENCE_NONE);

	if (!data->running) {
		return XRPose::XR_TRACKING_CONFIDENCE_NONE;
	}

	ERR_FAIL_COND_V(
		action->action_type != XR_ACTION_TYPE_POSE_INPUT, XRPose::XR_TRACKING_CONFIDENCE_NONE);

	// print_verbose("Checking " + action->name + " => " + tracker->name + " (" +
	// itos(tracker->toplevel_path) + ")");

	uint64_t index = 0xFFFFFFFF;
	uint64_t size = uint64_t(action->trackers.size());
	for (uint64_t i = 0; i < size && index == 0xFFFFFFFF; i++) {
		if (action->trackers[i].tracker_rid == p_tracker) {
			index = i;
		}
	}

	if (index == 0xFFFFFFFF) {
		// couldn't find it?
		return XRPose::XR_TRACKING_CONFIDENCE_NONE;
	}

	XrTime display_time = get_predicted_display_time();
	if (display_time == 0) {
		return XRPose::XR_TRACKING_CONFIDENCE_NONE;
	}

	if (action->trackers[index].space == XR_NULL_HANDLE) {
		// if this is a pose we need to define spaces

		XrActionSpaceCreateInfo action_space_info = {
			XR_TYPE_ACTION_SPACE_CREATE_INFO, // type
			nullptr,						  // next
			action->handle,					  // action
			tracker->toplevel_path,			  // subactionPath
			{
				{0.0, 0.0, 0.0, 1.0}, // orientation
				{0.0, 0.0, 0.0}		  // position
			}						  // poseInActionSpace
		};

		XrSpace space;
		XrResult result = xrCreateActionSpace(data->session, &action_space_info, &space);
		if (XR_FAILED(result)) {
			// print_line("OpenXR: couldn't create action space! [", get_error_string(result), "]");
			return XRPose::XR_TRACKING_CONFIDENCE_NONE;
		}

		action->trackers.ptrw()[index].space = space;
	}

	XrSpaceVelocity velocity = {
		XR_TYPE_SPACE_VELOCITY, // type
		nullptr,				// next
		0,						// velocityFlags
		{0.0, 0.0, 0.0},		// linearVelocity
		{0.0, 0.0, 0.0}			// angularVelocity
	};

	XrSpaceLocation location = {
		XR_TYPE_SPACE_LOCATION, // type
		&velocity,				// next
		0,						// locationFlags
		{
			{0.0, 0.0, 0.0, 0.0}, // orientation
			{0.0, 0.0, 0.0}		  // position
		}						  // pose
	};

	XrResult result =
		xrLocateSpace(action->trackers[index].space, data->play_space, display_time, &location);
	if (XR_FAILED(result)) {
		// print_line("OpenXR: failed to locate space! [", get_error_string(result), "]");
		return XRPose::XR_TRACKING_CONFIDENCE_NONE;
	}

	XRPose::TrackingConfidence confidence = transform_from_location(location, r_transform);
	parse_velocities(velocity, r_linear_velocity, r_angular_velocity);

	return confidence;
}

bool OpenXRAPI::trigger_haptic_pulse(
	RID p_action, RID p_tracker, float p_frequency, float p_amplitude, XrDuration p_duration_ns)
{
	ERR_FAIL_COND_V(data->session == XR_NULL_HANDLE, false);
	Action* action = data->action_owner.get_or_null(p_action);
	ERR_FAIL_NULL_V(action, false);
	Tracker* tracker = data->tracker_owner.get_or_null(p_tracker);
	ERR_FAIL_NULL_V(tracker, false);

	if (!data->running) {
		return false;
	}

	ERR_FAIL_COND_V(action->action_type != XR_ACTION_TYPE_VIBRATION_OUTPUT, false);

	XrHapticActionInfo action_info = {
		XR_TYPE_HAPTIC_ACTION_INFO, // type
		nullptr,					// next
		action->handle,				// action
		tracker->toplevel_path		// subactionPath
	};

	XrHapticVibration vibration = {
		XR_TYPE_HAPTIC_VIBRATION, // type
		nullptr,				  // next
		p_duration_ns,			  // duration
		p_frequency,			  // frequency
		p_amplitude,			  // amplitude
	};

	XrResult result =
		xrApplyHapticFeedback(data->session, &action_info, (const XrHapticBaseHeader*)&vibration);
	if (XR_FAILED(result)) {
		// print_line("OpenXR: failed to apply haptic feedback! [", get_error_string(result), "]");
		return false;
	}

	return true;
}

const Vector<XrEnvironmentBlendMode> OpenXRAPI::get_supported_environment_blend_modes()
{
	return data->supported_environment_blend_modes;
}

bool OpenXRAPI::is_environment_blend_mode_supported(XrEnvironmentBlendMode p_blend_mode)
{
	return data->supported_environment_blend_modes.has(p_blend_mode);
}

bool OpenXRAPI::set_environment_blend_mode(XrEnvironmentBlendMode p_blend_mode)
{
	if (data->emulate_environment_blend_mode_alpha_blend &&
		p_blend_mode == XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND) {
		data->requested_environment_blend_mode = XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND;
		data->environment_blend_mode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
		set_render_environment_blend_mode(data->environment_blend_mode);
		return true;
	}
	// We allow setting this when not initialized and will check if it is supported when
	// initializing. After OpenXR is initialized we verify we're setting a supported blend mode.
	else if (!is_initialized() || is_environment_blend_mode_supported(p_blend_mode)) {
		data->requested_environment_blend_mode = p_blend_mode;
		data->environment_blend_mode = p_blend_mode;
		set_render_environment_blend_mode(data->environment_blend_mode);
		return true;
	}
	return false;
}

void OpenXRAPI::set_emulate_environment_blend_mode_alpha_blend(bool p_enabled)
{
	data->emulate_environment_blend_mode_alpha_blend = p_enabled;
}

OpenXRAPI::OpenXRAlphaBlendModeSupport OpenXRAPI::is_environment_blend_mode_alpha_blend_supported()
{
	if (data->emulate_environment_blend_mode_alpha_blend) {
		return OPENXR_ALPHA_BLEND_MODE_SUPPORT_EMULATING;
	}
	else if (is_environment_blend_mode_supported(XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND)) {
		return OPENXR_ALPHA_BLEND_MODE_SUPPORT_REAL;
	}
	return OPENXR_ALPHA_BLEND_MODE_SUPPORT_NONE;
}

RID OpenXRAPI::OpenXRSwapChainInfo::get_image() { return RID(); }

bool OpenXRAPI::OpenXRSwapChainInfo::acquire(bool& p_should_render) { return true; }

void OpenXRAPI::set_render_environment_blend_mode(XrEnvironmentBlendMode p_mode) {}

void OpenXRAPI::destroy_session() {}

void OpenXRAPI::update_main_swapchain_size() {}

bool OpenXRAPI::setup_view_space() { return true; }

void OpenXRAPI::set_render_state_render_region(const Rect2i& p_render_region) {}

void OpenXRAPI::set_render_state_multiplier(double p_render_target_size_multiplier) {}

void OpenXRAPI::pre_render() {}

bool OpenXRAPI::pre_draw_viewport(RID p_render_target) { return true; }

void OpenXRAPI::post_draw_viewport(RID p_render_target) {}

RID OpenXRAPI::OpenXRSwapChainInfo::get_density_map() { return RID(); }

void OpenXRAPI::setup()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXRAPI::finalize()
{
	if (!data) {
		return;
	}
	finish();
	memdelete(data);
	data = nullptr;
}

bool OpenXRAPI::obtain_swapchain_formats() { return true; }

bool OpenXRAPI::load_supported_swapchain_formats() { return true; }

bool OpenXRAPI::load_supported_reference_spaces() { return true; }

bool OpenXRAPI::get_view_projection(
	uint32_t p_view, double p_z_near, double p_z_far, Projection& p_camera_matrix)
{
	return true;
}

Size2 OpenXRAPI::get_play_space_bounds() { return Size2(); }

XrHandTrackerEXT OpenXRAPI::get_hand_tracker(int p_hand_index) { return XrHandTrackerEXT(); }

Vector2 OpenXRAPI::get_eye_focus(uint32_t p_view, float p_aspect) { return Vector2(); }

PackedStringArray OpenXRAPI::get_all_requested_extensions(XrVersion p_xr_version)
{
	return PackedStringArray();
}

void OpenXRAPI::end_frame() {}

void OpenXRAPI::destroy_instance() {}

bool OpenXRAPI::create_session() { return true; }

void OpenXRAPI::cleanup_extension_wrappers() {}

void OpenXRAPI::allocate_view_buffers(uint32_t p_view_count, bool p_submit_depth_buffer) {}

String OpenXRAPI::get_swapchain_format_name(long p_swapchain_format) { return String(); }



////////////////////////////////////////////////////////////////////////////
// OpenXRUtil

String OpenXRUtil::get_result_string(XrResult p_result)
{
	XR_ENUM_SWITCH(XrResult, p_result)
}

String OpenXRUtil::get_view_configuration_name(XrViewConfigurationType p_view_configuration)
{
	XR_ENUM_SWITCH(XrViewConfigurationType, p_view_configuration)
}

String OpenXRUtil::get_reference_space_name(XrReferenceSpaceType p_reference_space)
{
	XR_ENUM_SWITCH(XrReferenceSpaceType, p_reference_space)
}

String OpenXRUtil::get_structure_type_name(XrStructureType p_structure_type)
{
	XR_ENUM_SWITCH(XrStructureType, p_structure_type)
}

String OpenXRUtil::get_session_state_name(XrSessionState p_session_state)
{
	XR_ENUM_SWITCH(XrSessionState, p_session_state)
}

String OpenXRUtil::get_action_type_name(XrActionType p_action_type)
{
	XR_ENUM_SWITCH(XrActionType, p_action_type)
}

String OpenXRUtil::get_environment_blend_mode_name(XrEnvironmentBlendMode p_blend_mode)
{
	XR_ENUM_SWITCH(XrEnvironmentBlendMode, p_blend_mode);
}

String OpenXRUtil::make_xr_version_string(XrVersion p_version)
{
	String version;

	version += String::num_int64(XR_VERSION_MAJOR(p_version));
	version += String(".");
	version += String::num_int64(XR_VERSION_MINOR(p_version));
	version += String(".");
	version += String::num_int64(XR_VERSION_PATCH(p_version));

	return version;
}

String OpenXRUtil::get_handle_as_hex_string(void* p_handle)
{
	String hex;

	if (p_handle == XR_NULL_HANDLE) {
		return "null";
	}

	uint64_t handle = (uint64_t)p_handle;

	while (handle != 0) {
		uint8_t a = handle & 0x0F;
		uint8_t b = (handle & 0xF0) >> 4;
		handle = handle >> 8;

		if (a < 10) {
			hex = (a + '0') + hex;
		}
		else {
			hex = (a + 'a' - 10) + hex;
		}

		if (b < 10) {
			hex = (b + '0') + hex;
		}
		else {
			hex = (b + 'a' - 10) + hex;
		}
	}

	return "0x" + hex;
}

String OpenXRUtil::string_from_xruuid(const XrUuid& xr_uuid)
{
	String ret;
	bool non_zero = false;

	for (int i = 0; i < XR_UUID_SIZE; i++) {
		non_zero |= xr_uuid.data[i] != 0;

		char a = xr_uuid.data[i] & 0xF0 >> 4;
		char b = xr_uuid.data[i] & 0x0F;

		if (a < 10) {
			ret += '0' + a;
		}
		else {
			ret += 'a' + a - 10;
		}

		if (b < 10) {
			ret += '0' + b;
		}
		else {
			ret += 'a' + b - 10;
		}
	}

	if (non_zero) {
		return ret;
	}
	else {
		return "";
	}
}

XrUuid OpenXRUtil::xruuid_from_string(const String& p_uuid)
{
	XrUuid new_uuid = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	int len = p_uuid.length();
	if (len == 0) {
		return new_uuid;
	}
	else if (len != (2 * XR_UUID_SIZE)) {
		WARN_PRINT("OpenXR: Unexpected UUID length: " + String::num_int64(len) +
				   " != " + String::num_int64(2 * XR_UUID_SIZE));
	}

	int j = 0;
	for (int i = 0; i < XR_UUID_SIZE; i++) {
		uint8_t val = 0;

		// 2 chars per byte.
		for (int k = 0; k < 2; k++) {
			if (j < len) {
				val <<= 4;

				char32_t c = p_uuid[j++];
				if (c >= '0' && c <= '9') {
					val += uint8_t(c - '0');
				}
				else if (c >= 'a' && c <= 'f') {
					val += uint8_t(10 + c - 'a');
				}
				else if (c >= 'A' && c <= 'F') {
					val += uint8_t(10 + c - 'A');
				}
				else {
					WARN_PRINT("OpenXR: Unexpected character in UUID: " + String::num_int64(c));
				}
			}
		}

		new_uuid.data[i] = val;
	}

	return new_uuid;
}

void OpenXRUtil::XrMatrix4x4f_CreateProjection(XrMatrix4x4f* result, const float tanAngleLeft,
	const float tanAngleRight, const float tanAngleUp, const float tanAngleDown,
	const float nearZ, const float farZ)
{
	const float tanAngleWidth = tanAngleRight - tanAngleLeft;

	// Set to tanAngleUp - tanAngleDown for a clip space with positive Y up.
	const float tanAngleHeight = (tanAngleUp - tanAngleDown);

	// Set to nearZ for a [-1,1] Z clip space.
	const float offsetZ = nearZ;

	if (farZ <= nearZ) {
		// place the far plane at infinity
		result->m[0] = 2.0f / tanAngleWidth;
		result->m[4] = 0.0f;
		result->m[8] = (tanAngleRight + tanAngleLeft) / tanAngleWidth;
		result->m[12] = 0.0f;

		result->m[1] = 0.0f;
		result->m[5] = 2.0f / tanAngleHeight;
		result->m[9] = (tanAngleUp + tanAngleDown) / tanAngleHeight;
		result->m[13] = 0.0f;

		result->m[2] = 0.0f;
		result->m[6] = 0.0f;
		result->m[10] = -1.0f;
		result->m[14] = -(nearZ + offsetZ);

		result->m[3] = 0.0f;
		result->m[7] = 0.0f;
		result->m[11] = -1.0f;
		result->m[15] = 0.0f;
	}
	else {
		// normal projection
		result->m[0] = 2.0f / tanAngleWidth;
		result->m[4] = 0.0f;
		result->m[8] = (tanAngleRight + tanAngleLeft) / tanAngleWidth;
		result->m[12] = 0.0f;

		result->m[1] = 0.0f;
		result->m[5] = 2.0f / tanAngleHeight;
		result->m[9] = (tanAngleUp + tanAngleDown) / tanAngleHeight;
		result->m[13] = 0.0f;

		result->m[2] = 0.0f;
		result->m[6] = 0.0f;
		result->m[10] = -(farZ + offsetZ) / (farZ - nearZ);
		result->m[14] = -(farZ * (nearZ + offsetZ)) / (farZ - nearZ);

		result->m[3] = 0.0f;
		result->m[7] = 0.0f;
		result->m[11] = -1.0f;
		result->m[15] = 0.0f;
	}
}

void OpenXRUtil::XrMatrix4x4f_CreateProjectionFov(
	XrMatrix4x4f* result, const XrFovf fov, const float nearZ, const float farZ)
{
	const float tanLeft = Math::tan(fov.angleLeft);
	const float tanRight = Math::tan(fov.angleRight);

	const float tanDown = Math::tan(fov.angleDown);
	const float tanUp = Math::tan(fov.angleUp);

	XrMatrix4x4f_CreateProjection(result, tanLeft, tanRight, tanUp, tanDown, nearZ, farZ);
}

////////////////////////////////////////////////////////////////////////////
// OpenXRStructureBase

void OpenXRStructureBase::set_next(const Ref<OpenXRStructureBase> p_next) { next = p_next; }

Ref<OpenXRStructureBase> OpenXRStructureBase::get_next() const { return next; }

bool OpenXRStructureBase::has_valid_configuration() const
{
	if (persistence_contexts.is_empty()) {
		return false;
	}

	ERR_FAIL_COND_V(!OpenXRSpatialAnchorCapability::is_initialized(), false);

	if (!OpenXRSpatialAnchorCapability::is_spatial_persistence_supported()) {
		return false;
	}

	return true;
}

void* OpenXRStructureBase::get_header(void* p_next)
{
	void* n = p_next;
	if (get_next().is_valid()) {
		n = get_next()->get_header(n);
	}

	if (has_valid_configuration()) {
		ERR_FAIL_COND_V(!OpenXRSpatialAnchorCapability::is_initialized(), nullptr);

		context_handles.resize(persistence_contexts.size());

		XrSpatialPersistenceContextEXT* ptr = context_handles.ptrw();
		int i = 0;
		for (const RID& rid : persistence_contexts) {
			ptr[i++] = OpenXRSpatialAnchorCapability::get_persistence_context_handle(rid);
		}

		persistence_config.next = n;
		persistence_config.persistenceContextCount = (uint32_t)context_handles.size();
		persistence_config.persistenceContexts = context_handles.ptr();

		return (XrSpatialCapabilityConfigurationBaseHeaderEXT*)&persistence_config;
	}

	return n;
}

XrStructureType OpenXRStructureBase::get_structure_type()
{
	if (!persistence_contexts.is_empty()) {
		return XR_TYPE_SPATIAL_CONTEXT_PERSISTENCE_CONFIG_EXT;
	}

	XrStructureType* header = (XrStructureType*)get_header();
	if (header == nullptr) {
		return XR_TYPE_UNKNOWN;
	}
	return *header;
}

void OpenXRStructureBase::add_persistence_context(RID p_persistence_context)
{
	ERR_FAIL_COND(persistence_contexts.has(p_persistence_context));

	persistence_contexts.push_back(p_persistence_context);
}

void OpenXRStructureBase::remove_persistence_context(RID p_persistence_context)
{
	ERR_FAIL_COND(!persistence_contexts.has(p_persistence_context));

	persistence_contexts.erase(p_persistence_context);
}

uint64_t OpenXRStructureBase::_get_structure_type() { return (uint64_t)get_structure_type(); }

////////////////////////////////////////////////////////////////////////////
// OpenXRAPIExtension

uint64_t OpenXRAPIExtension::get_openxr_version()
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), 0);
	return (uint64_t)OpenXRAPI::get_openxr_version();
}

uint64_t OpenXRAPIExtension::get_instance()
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), 0);
	return (uint64_t)OpenXRAPI::get_instance();
}

uint64_t OpenXRAPIExtension::get_system_id()
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), 0);
	return (uint64_t)OpenXRAPI::get_system_id();
}

uint64_t OpenXRAPIExtension::get_session()
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), 0);
	return (uint64_t)OpenXRAPI::get_session();
}

bool OpenXRAPIExtension::openxr_is_enabled(bool p_check_run_in_editor)
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), false);
	return OpenXRAPI::openxr_is_enabled(p_check_run_in_editor);
}

uint64_t OpenXRAPIExtension::get_instance_proc_addr(const String& p_name)
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), 0);
	CharString str = p_name.utf8();
	PFN_xrVoidFunction addr = nullptr;
	XrResult result = OpenXRAPI::get_instance_proc_addr(str.get_data(), &addr);
	if (result != XR_SUCCESS) {
		return 0;
	}
	return reinterpret_cast<uint64_t>(addr);
}

String OpenXRAPIExtension::get_error_string(uint64_t result)
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), String());
	return OpenXRAPI::get_error_string((XrResult)result);
}

String OpenXRAPIExtension::get_swapchain_format_name(int64_t p_swapchain_format)
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), String());
	return OpenXRAPI::get_swapchain_format_name(p_swapchain_format);
}

void OpenXRAPIExtension::set_object_name(
	int64_t p_object_type, uint64_t p_object_handle, const String& p_object_name)
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	OpenXRAPI::set_object_name(XrObjectType(p_object_type), p_object_handle, p_object_name);
}

void OpenXRAPIExtension::begin_debug_label_region(const String& p_label_name)
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	OpenXRAPI::begin_debug_label_region(p_label_name);
}

void OpenXRAPIExtension::end_debug_label_region()
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	OpenXRAPI::end_debug_label_region();
}

void OpenXRAPIExtension::insert_debug_label(const String& p_label_name)
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	OpenXRAPI::insert_debug_label(p_label_name);
}

uint32_t OpenXRAPIExtension::get_view_count() const
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), 0);
	return (uint32_t)OpenXRAPI::get_view_count();
}

uint64_t OpenXRAPIExtension::get_view_configuration() const
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), 0);
	return (uint64_t)OpenXRAPI::get_view_configuration();
}

bool OpenXRAPIExtension::is_initialized()
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), false);
	return OpenXRAPI::is_initialized();
}

bool OpenXRAPIExtension::is_running()
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), false);
	return OpenXRAPI::is_running();
}

uint64_t OpenXRAPIExtension::get_play_space()
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), 0);
	return (uint64_t)OpenXRAPI::get_play_space();
}

int64_t OpenXRAPIExtension::get_predicted_display_time()
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), 0);
	return (XrTime)OpenXRAPI::get_predicted_display_time();
}

int64_t OpenXRAPIExtension::get_next_frame_time()
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), 0);

	WARN_PRINT_ONCE("OpenXR: Next frame timing called, verify this is intended.");

	return (XrTime)OpenXRAPI::get_next_frame_time();
}

bool OpenXRAPIExtension::can_render()
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), false);
	return OpenXRAPI::can_render();
}

RID OpenXRAPIExtension::find_action(const String& p_name, const RID& p_action_set)
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), RID());
	return OpenXRAPI::find_action(p_name, p_action_set);
}

uint64_t OpenXRAPIExtension::action_get_handle(RID p_action)
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), 0);
	XrAction action_handle = OpenXRAPI::action_get_handle(p_action);
	return (uint64_t)action_handle;
}

uint64_t OpenXRAPIExtension::get_hand_tracker(int p_hand_index)
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), 0);
	return (uint64_t)OpenXRAPI::get_hand_tracker(p_hand_index);
}

double OpenXRAPIExtension::get_render_state_z_near()
{
	ERR_NOT_ON_RENDER_THREAD_V(0.0);
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), 0.0);
	return OpenXRAPI::get_render_state_z_near();
}

double OpenXRAPIExtension::get_render_state_z_far()
{
	ERR_NOT_ON_RENDER_THREAD_V(0.0);
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), 0.0);
	return OpenXRAPI::get_render_state_z_far();
}

void OpenXRAPIExtension::set_velocity_texture(RID p_render_target)
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());
	OpenXRAPI::set_velocity_texture(p_render_target);
}

void OpenXRAPIExtension::set_velocity_depth_texture(RID p_render_target)
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());
	OpenXRAPI::set_velocity_depth_texture(p_render_target);
}

void OpenXRAPIExtension::set_velocity_target_size(const Size2i& p_target_size)
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());
	OpenXRAPI::set_velocity_target_size(p_target_size);
}

PackedInt64Array OpenXRAPIExtension::get_supported_swapchain_formats()
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), PackedInt64Array());
	return OpenXRAPI::get_supported_swapchain_formats();
}

uint64_t OpenXRAPIExtension::openxr_swapchain_create(XrSwapchainCreateFlags p_create_flags,
	XrSwapchainUsageFlags p_usage_flags, int64_t p_swapchain_format, uint32_t p_width,
	uint32_t p_height, uint32_t p_sample_count, uint32_t p_array_size)
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), 0);

	OpenXRAPI::OpenXRSwapChainInfo* new_swapchain_info = memnew(OpenXRAPI::OpenXRSwapChainInfo);
	new_swapchain_info->create(p_create_flags, p_usage_flags, p_swapchain_format, p_width, p_height,
		p_sample_count, p_array_size);
	return reinterpret_cast<uint64_t>(new_swapchain_info);
}

void OpenXRAPIExtension::openxr_swapchain_free(uint64_t p_swapchain_info)
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	OpenXRAPI::OpenXRSwapChainInfo* swapchain_info =
		reinterpret_cast<OpenXRAPI::OpenXRSwapChainInfo*>(p_swapchain_info);
	swapchain_info->free();
	memfree(swapchain_info);
}

uint64_t OpenXRAPIExtension::openxr_swapchain_get_swapchain(uint64_t p_swapchain_info)
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), 0);

	OpenXRAPI::OpenXRSwapChainInfo* swapchain_info =
		reinterpret_cast<OpenXRAPI::OpenXRSwapChainInfo*>(p_swapchain_info);
	XrSwapchain swapchain = swapchain_info->get_swapchain();
	return (uint64_t)swapchain;
}

void OpenXRAPIExtension::openxr_swapchain_acquire(uint64_t p_swapchain_info)
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	OpenXRAPI::OpenXRSwapChainInfo* swapchain_info =
		reinterpret_cast<OpenXRAPI::OpenXRSwapChainInfo*>(p_swapchain_info);
	bool should_render = true;
	swapchain_info->acquire(should_render);
}

RID OpenXRAPIExtension::openxr_swapchain_get_image(uint64_t p_swapchain_info)
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), RID());

	OpenXRAPI::OpenXRSwapChainInfo* swapchain_info =
		reinterpret_cast<OpenXRAPI::OpenXRSwapChainInfo*>(p_swapchain_info);
	return swapchain_info->get_image();
}

void OpenXRAPIExtension::openxr_swapchain_release(uint64_t p_swapchain_info)
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	OpenXRAPI::OpenXRSwapChainInfo* swapchain_info =
		reinterpret_cast<OpenXRAPI::OpenXRSwapChainInfo*>(p_swapchain_info);
	swapchain_info->release();
}

uint64_t OpenXRAPIExtension::get_projection_layer()
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), 0);
	return (uint64_t)OpenXRAPI::get_projection_layer();
}

void OpenXRAPIExtension::set_render_region(const Rect2i& p_render_region)
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());
	OpenXRAPI::set_render_region(p_render_region);
}

void OpenXRAPIExtension::update_main_swapchain_size()
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());
	OpenXRAPI::update_main_swapchain_size();
}

void OpenXRAPIExtension::set_emulate_environment_blend_mode_alpha_blend(bool p_enabled)
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());
	OpenXRAPI::set_emulate_environment_blend_mode_alpha_blend(p_enabled);
}

OpenXRAPIExtension::OpenXRAlphaBlendModeSupport
OpenXRAPIExtension::is_environment_blend_mode_alpha_blend_supported()
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), OPENXR_ALPHA_BLEND_MODE_SUPPORT_NONE);
	return (OpenXRAPIExtension::OpenXRAlphaBlendModeSupport)
		OpenXRAPI::is_environment_blend_mode_alpha_blend_supported();
}

OpenXRAPIExtension::OpenXRAPIExtension() {}

#ifdef None
#undef None
#endif
#ifdef Status
#undef Status
#endif
#ifdef Bool
#undef Bool
#endif
#ifdef True
#undef True
#endif
#ifdef False
#undef False
#endif
#ifdef CursorShape
#undef CursorShape
#endif
#ifdef KeyPress
#undef KeyPress
#endif
#ifdef KeyRelease
#undef KeyRelease
#endif
#ifdef FocusIn
#undef FocusIn
#endif
#ifdef FocusOut
#undef FocusOut
#endif
#ifdef Expose
#undef Expose
#endif
#ifdef FontChange
#undef FontChange
#endif
#ifdef Unsorted
#undef Unsorted
#endif
#ifdef Always
#undef Always
#endif
#ifdef Above
#undef Above
#endif
#ifdef Below
#undef Below
#endif

// ============================================================================
// Merged from modules/openxr/register_types.cpp
// ============================================================================

#include "modules/openxr/openxr_api.h"
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/os/os.h"


#include "modules/modules_enabled.gen.h"
#include "modules/openxr/openxr_api.h"

#include "main/main.h"


#ifdef ANDROID_ENABLED
#include "modules/openxr/openxr_platform_inc.h"
#endif

static Ref<OpenXRInterface> openxr_interface;

void uninitialize_openxr_module(ModuleInitializationLevel p_level)
{
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	if (openxr_interface.is_valid()) {
		// uninitialize just in case
		if (openxr_interface->is_initialized()) {
			openxr_interface->uninitialize();
		}

		// unregister our interface from the XR server
		if (XRServer::is_initialized()) {
			if (XRServer::get_primary_interface() == openxr_interface) {
				XRServer::set_primary_interface(Ref<XRInterface>());
			}
			XRServer::remove_interface(openxr_interface);
		}

		// and release
		openxr_interface.unref();
	}

	if (OpenXRAPI::is_allocated()) {
		OpenXRAPI::finalize();
	}

	if (OpenXRInteractionProfileMetadata::is_initialized()) {
		OpenXRInteractionProfileMetadata::finalize();
	}

	if (OpenXRInteractionProfileExtension::is_initialized()) {
		OpenXRInteractionProfileExtension::finalize();
	}

	// cleanup our extension wrappers
	OpenXRAPI::cleanup_extension_wrappers();
}

void initialize_openxr_module(ModuleInitializationLevel p_level) {}

// ============================================================================
// Merged from modules/openxr/openxr_interface.cpp
// ============================================================================

#include <openxr/openxr.h>
#include "modules/openxr/openxr_api.h"
#include "core/config/engine.h"
#include "core/io/resource_loader.h"


#include "servers/display/display_server.h"
#include "servers/rendering/rendering_device.h"

String OpenXRInterface::get_name() const { return String("OpenXR"); }

uint32_t OpenXRInterface::get_capabilities() const
{
	return XRInterface::XR_VR + XRInterface::XR_STEREO;
}

XRInterface::TrackingStatus OpenXRInterface::get_tracking_status() const { return tracking_state; }

OpenXRInterface::ActionSet* OpenXRInterface::create_action_set(
	const String& p_action_set_name, const String& p_localized_name, const int p_priority)
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), nullptr);

	// find if it already exists
	for (int i = 0; i < action_sets.size(); i++) {
		if (action_sets[i]->action_set_name == p_action_set_name) {
			// already exists in this set
			return nullptr;
		}
	}

	ActionSet* action_set = memnew(ActionSet);
	action_set->action_set_name = p_action_set_name;
	action_set->is_active = true;
	action_set->action_set_rid =
		OpenXRAPI::action_set_create(p_action_set_name, p_localized_name, p_priority);
	action_sets.push_back(action_set);

	return action_set;
}

void OpenXRInterface::free_action_sets()
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	for (int i = 0; i < action_sets.size(); i++) {
		ActionSet* action_set = action_sets[i];

		free_actions(action_set);

		OpenXRAPI::action_set_free(action_set->action_set_rid);

		memfree(action_set);
	}
	action_sets.clear();
}

OpenXRInterface::Action* OpenXRInterface::create_action(ActionSet* p_action_set,
	const String& p_action_name, const String& p_localized_name,
	OpenXRAction::ActionType p_action_type, const Vector<Tracker*> p_trackers)
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), nullptr);

	for (int i = 0; i < p_action_set->actions.size(); i++) {
		if (p_action_set->actions[i]->action_name == p_action_name) {
			// already exists in this set
			return nullptr;
		}
	}

	Vector<RID> tracker_rids;
	for (int i = 0; i < p_trackers.size(); i++) {
		tracker_rids.push_back(p_trackers[i]->tracker_rid);
	}

	Action* action = memnew(Action);
	if (p_action_type == OpenXRAction::OPENXR_ACTION_POSE) {
		// We can't have dual action names in OpenXR hence we added _pose,
		// but default, aim and grip and default pose action names in Godot so rename them on the
		// tracker. NOTE need to decide on whether we should keep the naming convention or rename it
		// on Godots side
		if (p_action_name == "default_pose") {
			action->action_name = "default";
		}
		else if (p_action_name == "aim_pose") {
			action->action_name = "aim";
		}
		else if (p_action_name == "grip_pose") {
			action->action_name = "grip";
		}
		else {
			action->action_name = p_action_name;
		}
	}
	else {
		action->action_name = p_action_name;
	}

	action->action_type = p_action_type;
	action->action_rid = OpenXRAPI::action_create(
		p_action_set->action_set_rid, p_action_name, p_localized_name, p_action_type, tracker_rids);
	p_action_set->actions.push_back(action);

	// we link our actions back to our trackers so we know which actions to check when we're
	// processing our trackers
	for (int i = 0; i < p_trackers.size(); i++) {
		if (!p_trackers[i]->actions.has(action)) {
			p_trackers[i]->actions.push_back(action);
		}
	}

	return action;
}

OpenXRInterface::Action* OpenXRInterface::find_action(const String& p_action_name)
{
	// We just find the first action by this name

	for (int i = 0; i < action_sets.size(); i++) {
		for (int j = 0; j < action_sets[i]->actions.size(); j++) {
			if (action_sets[i]->actions[j]->action_name == p_action_name) {
				return action_sets[i]->actions[j];
			}
		}
	}

	// not found
	return nullptr;
}

void OpenXRInterface::free_actions(ActionSet* p_action_set)
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	for (int i = 0; i < p_action_set->actions.size(); i++) {
		Action* action = p_action_set->actions[i];

		OpenXRAPI::action_free(action->action_rid);

		memdelete(action);
	}
	p_action_set->actions.clear();
}

OpenXRInterface::Tracker* OpenXRInterface::find_tracker(const String& p_tracker_name, bool p_create)
{
	ERR_FAIL_COND_V(!XRServer::is_initialized(), nullptr);
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), nullptr);

	Tracker* tracker = nullptr;
	for (int i = 0; i < trackers.size(); i++) {
		tracker = trackers[i];
		if (tracker->tracker_name == p_tracker_name) {
			return tracker;
		}
	}

	if (!p_create) {
		return nullptr;
	}

	ERR_FAIL_COND_V(!OpenXRAPI::is_top_level_path_supported(p_tracker_name), nullptr);

	// Create our RID
	RID tracker_rid = OpenXRAPI::tracker_create(p_tracker_name);
	ERR_FAIL_COND_V(tracker_rid.is_null(), nullptr);

	// Create our controller tracker.
	Ref<XRTracker> controller_tracker;
	controller_tracker.instantiate();
	controller_tracker->set_tracker_type(XRServer::TRACKER_CONTROLLER);

	// We have standardized some names to make things nicer to the user so lets recognize the
	// toplevel paths related to these.
	if (p_tracker_name == "/user/hand/left") {
		controller_tracker->set_tracker_name("left_hand");
		controller_tracker->set_tracker_desc("Left hand controller");
		controller_tracker->set_tracker_hand(XRTracker::TRACKER_HAND_LEFT);
	}
	else if (p_tracker_name == "/user/hand/right") {
		controller_tracker->set_tracker_name("right_hand");
		controller_tracker->set_tracker_desc("Right hand controller");
		controller_tracker->set_tracker_hand(XRTracker::TRACKER_HAND_RIGHT);
	}
	else {
		controller_tracker->set_tracker_name(p_tracker_name);
		controller_tracker->set_tracker_desc(p_tracker_name);
	}
	controller_tracker->set_tracker_profile(INTERACTION_PROFILE_NONE);
	XRServer::add_tracker(controller_tracker);

	// create a new entry
	tracker = memnew(Tracker);
	tracker->tracker_name = p_tracker_name;
	tracker->tracker_rid = tracker_rid;
	tracker->controller_tracker = controller_tracker;
	tracker->interaction_profile = RID();
	trackers.push_back(tracker);

	return tracker;
}

void OpenXRInterface::tracker_profile_changed(RID p_tracker, RID p_interaction_profile)
{
	Tracker* tracker = nullptr;
	for (int i = 0; i < trackers.size() && tracker == nullptr; i++) {
		if (trackers[i]->tracker_rid == p_tracker) {
			tracker = trackers[i];
		}
	}
	ERR_FAIL_NULL(tracker);

	tracker->interaction_profile = p_interaction_profile;

	if (p_interaction_profile.is_null()) {
		print_verbose("OpenXR: Interaction profile for " + tracker->tracker_name + " changed to " +
					  INTERACTION_PROFILE_NONE);
		tracker->controller_tracker->set_tracker_profile(INTERACTION_PROFILE_NONE);
	}
	else {
		String name = OpenXRAPI::interaction_profile_get_name(p_interaction_profile);
		print_verbose(
			"OpenXR: Interaction profile for " + tracker->tracker_name + " changed to " + name);
		tracker->controller_tracker->set_tracker_profile(name);
	}
}

void OpenXRInterface::trigger_haptic_pulse(const String& p_action_name,
	const String& p_tracker_name, double p_frequency, double p_amplitude, double p_duration_sec,
	double p_delay_sec)
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	Action* action = find_action(p_action_name);
	ERR_FAIL_NULL(action);

	// We need to map our tracker name to our OpenXR name for our inbuild names.
	String tracker_name = p_tracker_name;
	if (tracker_name == "left_hand") {
		tracker_name = "/user/hand/left";
	}
	else if (tracker_name == "right_hand") {
		tracker_name = "/user/hand/right";
	}
	Tracker* tracker = find_tracker(tracker_name);
	ERR_FAIL_NULL(tracker);

	// TODO OpenXR does not support delay, so we may need to add support for that somehow...

	XrDuration duration = XrDuration(p_duration_sec * 1000000000.0); // seconds -> nanoseconds

	OpenXRAPI::trigger_haptic_pulse(
		action->action_rid, tracker->tracker_rid, p_frequency, p_amplitude, duration);
}

void OpenXRInterface::free_trackers()
{
	ERR_FAIL_COND(!XRServer::is_initialized());
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	for (int i = 0; i < trackers.size(); i++) {
		Tracker* tracker = trackers[i];

		OpenXRAPI::tracker_free(tracker->tracker_rid);
		XRServer::remove_tracker(tracker->controller_tracker);
		tracker->controller_tracker.unref();

		memdelete(tracker);
	}
	trackers.clear();
}

void OpenXRInterface::free_interaction_profiles()
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	for (const RID& interaction_profile : interaction_profiles) {
		OpenXRAPI::interaction_profile_free(interaction_profile);
	}
	interaction_profiles.clear();
}

bool OpenXRInterface::initialize_on_startup() const
{
	if (!OpenXRAPI::is_allocated()) {
		return false;
	}
	else if (!OpenXRAPI::is_initialized()) {
		return false;
	}
	else {
		return true;
	}
}

bool OpenXRInterface::is_initialized() const { return initialized; }

bool OpenXRInterface::supports_play_area_mode(XRInterface::PlayAreaMode p_mode)
{
	if (p_mode == XRInterface::XR_PLAY_AREA_3DOF) {
		return false;
	}
	return true;
}

XRInterface::PlayAreaMode OpenXRInterface::get_play_area_mode() const
{
	if (!OpenXRAPI::is_allocated() || !initialized) {
		return XRInterface::XR_PLAY_AREA_UNKNOWN;
	}

	XrReferenceSpaceType reference_space = OpenXRAPI::get_reference_space();

	if (reference_space == XR_REFERENCE_SPACE_TYPE_LOCAL) {
		return XRInterface::XR_PLAY_AREA_SITTING;
	}
	else if (reference_space == XR_REFERENCE_SPACE_TYPE_LOCAL_FLOOR_EXT) {
		return XRInterface::XR_PLAY_AREA_ROOMSCALE;
	}
	else if (reference_space == XR_REFERENCE_SPACE_TYPE_STAGE) {
		return XRInterface::XR_PLAY_AREA_STAGE;
	}
	else if (reference_space == XR_REFERENCE_SPACE_TYPE_MAX_ENUM) {
		return XRInterface::XR_PLAY_AREA_CUSTOM;
	}

	return XRInterface::XR_PLAY_AREA_UNKNOWN;
}

bool OpenXRInterface::set_play_area_mode(XRInterface::PlayAreaMode p_mode)
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), false);

	XrReferenceSpaceType reference_space;

	if (p_mode == XRInterface::XR_PLAY_AREA_SITTING) {
		reference_space = XR_REFERENCE_SPACE_TYPE_LOCAL;
	}
	else if (p_mode == XRInterface::XR_PLAY_AREA_ROOMSCALE) {
		reference_space = XR_REFERENCE_SPACE_TYPE_LOCAL_FLOOR_EXT;
	}
	else if (p_mode == XRInterface::XR_PLAY_AREA_STAGE) {
		reference_space = XR_REFERENCE_SPACE_TYPE_STAGE;
	}
	else {
		return false;
	}

	if (OpenXRAPI::set_requested_reference_space(reference_space)) {
		if (XRServer::is_initialized()) {
			XRServer::clear_reference_frame();
		}
		return true;
	}

	return false;
}

PackedVector3Array OpenXRInterface::get_play_area() const
{
	ERR_FAIL_COND_V(!XRServer::is_initialized(), PackedVector3Array());
	PackedVector3Array arr;

	Vector3 sides[4] = {
		Vector3(-0.5f, 0.0f, -0.5f),
		Vector3(0.5f, 0.0f, -0.5f),
		Vector3(0.5f, 0.0f, 0.5f),
		Vector3(-0.5f, 0.0f, 0.5f),
	};

	if (OpenXRAPI::is_allocated() && OpenXRAPI::is_initialized()) {
		Size2 extents = OpenXRAPI::get_play_space_bounds();
		if (extents.width != 0.0 && extents.height != 0.0) {
			Transform3D reference_frame = XRServer::get_reference_frame();

			for (int i = 0; i < 4; i++) {
				Vector3 coord = sides[i];

				// Scale it up.
				coord.x *= extents.width;
				coord.z *= extents.height;

				// Now apply our reference.
				Vector3 out = reference_frame.xform(coord);
				arr.push_back(out);
			}
		}
		else {
			WARN_PRINT_ONCE("OpenXR: No extents available.");
		}
	}

	return arr;
}

float OpenXRInterface::get_display_refresh_rate() const
{
	if (!OpenXRAPI::is_allocated()) {
		return 0.0;
	}
	else if (!OpenXRAPI::is_initialized()) {
		return 0.0;
	}
	else {
		return OpenXRAPI::get_display_refresh_rate();
	}
}

void OpenXRInterface::set_display_refresh_rate(float p_refresh_rate)
{
	if (!OpenXRAPI::is_allocated()) {
		return;
	}
	else if (!OpenXRAPI::is_initialized()) {
		return;
	}
	else {
		OpenXRAPI::set_display_refresh_rate(p_refresh_rate);
	}
}

bool OpenXRInterface::is_hand_tracking_supported()
{
	if (!OpenXRAPI::is_allocated()) {
		return false;
	}
	else if (!OpenXRAPI::is_initialized()) {
		return false;
	}
	else {
		if (!OpenXRHandTrackingExtension::is_initialized()) {
			return false;
		}
		else {
			return OpenXRHandTrackingExtension::get_active();
		}
	}
}

bool OpenXRInterface::is_hand_interaction_supported() const
{
	if (!OpenXRAPI::is_allocated()) {
		return false;
	}
	else if (!OpenXRAPI::is_initialized()) {
		return false;
	}
	else {
		if (!OpenXRInteractionProfileExtension::is_initialized()) {
			return false;
		}
		else {
			return OpenXRInteractionProfileExtension::is_profile_available(
				OpenXRInteractionProfileExtension::PROFILE_HAND_INTERACTION);
		}
	}
}

bool OpenXRInterface::is_eye_gaze_interaction_supported()
{
	if (!OpenXRAPI::is_allocated()) {
		return false;
	}
	else if (!OpenXRAPI::is_initialized()) {
		return false;
	}
	else {
		if (!OpenXREyeGazeInteractionExtension::is_initialized()) {
			return false;
		}
		else {
			return OpenXREyeGazeInteractionExtension::supports_eye_gaze_interaction();
		}
	}
}

bool OpenXRInterface::is_action_set_active(const String& p_action_set) const
{
	for (ActionSet* action_set : action_sets) {
		if (action_set->action_set_name == p_action_set) {
			return action_set->is_active;
		}
	}

	WARN_PRINT("OpenXR: Unknown action set " + p_action_set);
	return false;
}

void OpenXRInterface::set_action_set_active(const String& p_action_set, bool p_active)
{
	for (ActionSet* action_set : action_sets) {
		if (action_set->action_set_name == p_action_set) {
			action_set->is_active = p_active;
			return;
		}
	}

	WARN_PRINT("OpenXR: Unknown action set " + p_action_set);
}

float OpenXRInterface::get_vrs_min_radius() const { return xr_vrs.get_vrs_min_radius(); }

void OpenXRInterface::set_vrs_min_radius(float p_vrs_min_radius)
{
	xr_vrs.set_vrs_min_radius(p_vrs_min_radius);
}

float OpenXRInterface::get_vrs_strength() const { return xr_vrs.get_vrs_strength(); }

void OpenXRInterface::set_vrs_strength(float p_vrs_strength)
{
	xr_vrs.set_vrs_strength(p_vrs_strength);
}

double OpenXRInterface::get_render_target_size_multiplier() const
{
	if (!OpenXRAPI::is_allocated()) {
		return 1.0;
	}
	else {
		return OpenXRAPI::get_render_target_size_multiplier();
	}
}

void OpenXRInterface::set_render_target_size_multiplier(double multiplier)
{
	if (!OpenXRAPI::is_allocated()) {
		return;
	}
	else {
		OpenXRAPI::set_render_target_size_multiplier(multiplier);
	}
}

bool OpenXRInterface::is_foveation_supported() const
{
	if (!OpenXRAPI::is_allocated()) {
		return false;
	}
	else {
		return OpenXRAPI::is_foveation_supported();
	}
}

int OpenXRInterface::get_foveation_level() const
{
	if (!OpenXRAPI::is_allocated()) {
		return 0;
	}
	else {
		return OpenXRAPI::get_foveation_level();
	}
}

void OpenXRInterface::set_foveation_level(int p_foveation_level)
{
	if (!OpenXRAPI::is_allocated()) {
		return;
	}
	else {
		OpenXRAPI::set_foveation_level(p_foveation_level);
	}
}

bool OpenXRInterface::get_foveation_dynamic() const
{
	if (!OpenXRAPI::is_allocated()) {
		return false;
	}
	else {
		return OpenXRAPI::get_foveation_dynamic();
	}
}

void OpenXRInterface::set_foveation_dynamic(bool p_foveation_dynamic)
{
	if (!OpenXRAPI::is_allocated()) {
		return;
	}
	else {
		OpenXRAPI::set_foveation_dynamic(p_foveation_dynamic);
	}
}

bool OpenXRInterface::get_foveation_with_subsampled_images() const
{
	if (!OpenXRAPI::is_allocated()) {
		return false;
	}
	else {
		return OpenXRAPI::get_foveation_with_subsampled_images();
	}
}

void OpenXRInterface::set_foveation_with_subsampled_images(bool p_enabled)
{
	if (!OpenXRAPI::is_allocated()) {
		return;
	}
	else {
		OpenXRAPI::set_foveation_with_subsampled_images(p_enabled);
	}
}

Size2 OpenXRInterface::get_render_target_size()
{
	if (!OpenXRAPI::is_allocated()) {
		return Size2();
	}
	else {
		return OpenXRAPI::get_recommended_target_size();
	}
}

uint32_t OpenXRInterface::get_view_count()
{
	// TODO set this based on our configuration
	return 2;
}

void OpenXRInterface::_set_default_pos(
	Transform3D& r_transform, double p_world_scale, uint64_t p_eye)
{
	r_transform = Transform3D();

	// if we're not tracking, don't put our head on the floor...
	r_transform.origin.y = 1.5 * p_world_scale;

	// overkill but..
	if (p_eye == 1) {
		r_transform.origin.x = 0.03 * p_world_scale;
	}
	else if (p_eye == 2) {
		r_transform.origin.x = -0.03 * p_world_scale;
	}
}

Transform3D OpenXRInterface::get_camera_transform()
{
	ERR_FAIL_COND_V(!XRServer::is_initialized(), Transform3D());

	Transform3D hmd_transform;
	double world_scale = XRServer::get_world_scale();

	// head_transform should be updated in process

	hmd_transform.basis = head_transform.basis;
	hmd_transform.origin = head_transform.origin * world_scale;

	return hmd_transform;
}

Transform3D OpenXRInterface::get_transform_for_view(
	uint32_t p_view, const Transform3D& p_cam_transform)
{
	ERR_FAIL_COND_V(!XRServer::is_initialized(), Transform3D());
	ERR_FAIL_UNSIGNED_INDEX_V_MSG(
		p_view, get_view_count(), Transform3D(), "View index outside bounds.");

	Transform3D t;
	if (OpenXRAPI::is_allocated() && OpenXRAPI::get_view_transform(p_view, t)) {
		// update our cached value if we have a valid transform
		transform_for_view[p_view] = t;
	}
	else {
		// reuse cached value
		t = transform_for_view[p_view];
	}

	// Apply our world scale
	double world_scale = XRServer::get_world_scale();
	t.origin *= world_scale;

	return p_cam_transform * XRServer::get_reference_frame() * t;
}

Projection OpenXRInterface::get_projection_for_view(
	uint32_t p_view, double p_aspect, double p_z_near, double p_z_far)
{
	Projection cm;
	ERR_FAIL_UNSIGNED_INDEX_V_MSG(p_view, get_view_count(), cm, "View index outside bounds.");

	if (OpenXRAPI::is_allocated()) {
		if (OpenXRAPI::get_view_projection(p_view, p_z_near, p_z_far, cm)) {
			return cm;
		}
	}

	// Failed to get from our OpenXR device? Default to some sort of sensible camera matrix..
	cm.set_for_hmd(p_view + 1, 1.0, 6.0, 14.5, 4.0, 1.5, p_z_near, p_z_far);

	return cm;
}

Rect2i OpenXRInterface::get_render_region()
{
	if (OpenXRAPI::is_allocated()) {
		return OpenXRAPI::get_render_region();
	}
	else {
		return Rect2i();
	}
}

RID OpenXRInterface::get_color_texture()
{
	if (OpenXRAPI::is_allocated()) {
		return OpenXRAPI::get_color_texture();
	}
	else {
		return RID();
	}
}

RID OpenXRInterface::get_depth_texture()
{
	if (OpenXRAPI::is_allocated()) {
		return OpenXRAPI::get_depth_texture();
	}
	else {
		return RID();
	}
}

RID OpenXRInterface::get_velocity_texture()
{
	if (OpenXRAPI::is_allocated()) {
		return OpenXRAPI::get_velocity_texture();
	}
	else {
		return RID();
	}
}

RID OpenXRInterface::get_velocity_depth_texture()
{
	if (OpenXRAPI::is_allocated()) {
		return OpenXRAPI::get_velocity_depth_texture();
	}
	else {
		return RID();
	}
}

Size2i OpenXRInterface::get_velocity_target_size()
{
	if (OpenXRAPI::is_allocated()) {
		return OpenXRAPI::get_velocity_target_size();
	}
	else {
		return Size2i();
	}
}

void OpenXRInterface::pre_render()
{
	if (OpenXRAPI::is_allocated()) {
		OpenXRAPI::pre_render();
	}
}

bool OpenXRInterface::pre_draw_viewport(RID p_render_target)
{
	if (OpenXRAPI::is_allocated()) {
		return OpenXRAPI::pre_draw_viewport(p_render_target);
	}
	else {
		// don't render
		return false;
	}
}

Vector<RenderingServerTypes::BlitToScreen> OpenXRInterface::post_draw_viewport(
	RID p_render_target, const Rect2& p_screen_rect)
{
	Vector<RenderingServerTypes::BlitToScreen> blit_to_screen;

#ifndef ANDROID_ENABLED
	// If separate HMD we should output one eye to screen
	if (p_screen_rect != Rect2()) {
		RenderingServerTypes::BlitToScreen blit;

		blit.render_target = p_render_target;
		blit.multi_view.use_layer = true;
		blit.multi_view.layer = 0;
		blit.lens_distortion.apply = false;

		Size2 render_size = get_render_target_size();
		Rect2 dst_rect = p_screen_rect;
		float new_height = dst_rect.size.x * (render_size.y / render_size.x);
		if (new_height > dst_rect.size.y) {
			dst_rect.position.y = (0.5 * dst_rect.size.y) - (0.5 * new_height);
			dst_rect.size.y = new_height;
		}
		else {
			float new_width = dst_rect.size.y * (render_size.x / render_size.y);

			dst_rect.position.x = (0.5 * dst_rect.size.x) - (0.5 * new_width);
			dst_rect.size.x = new_width;
		}

		blit.dst_rect = dst_rect;
		blit_to_screen.push_back(blit);
	}
#endif

	if (OpenXRAPI::is_allocated()) {
		OpenXRAPI::post_draw_viewport(p_render_target);
	}

	return blit_to_screen;
}

void OpenXRInterface::end_frame()
{
	if (OpenXRAPI::is_allocated()) {
		OpenXRAPI::end_frame();
	}
}

bool OpenXRInterface::is_passthrough_enabled()
{
	return get_environment_blend_mode() == XR_ENV_BLEND_MODE_ALPHA_BLEND;
}

bool OpenXRInterface::start_passthrough()
{
	return set_environment_blend_mode(XR_ENV_BLEND_MODE_ALPHA_BLEND);
}

void OpenXRInterface::stop_passthrough() { set_environment_blend_mode(XR_ENV_BLEND_MODE_OPAQUE); }

XRInterface::EnvironmentBlendMode OpenXRInterface::get_environment_blend_mode() const
{
	if (OpenXRAPI::is_allocated()) {
		XrEnvironmentBlendMode oxr_blend_mode = OpenXRAPI::get_environment_blend_mode();
		switch (oxr_blend_mode) {
		case XR_ENVIRONMENT_BLEND_MODE_OPAQUE: {
			return XR_ENV_BLEND_MODE_OPAQUE;
		} break;
		case XR_ENVIRONMENT_BLEND_MODE_ADDITIVE: {
			return XR_ENV_BLEND_MODE_ADDITIVE;
		} break;
		case XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND: {
			return XR_ENV_BLEND_MODE_ALPHA_BLEND;
		} break;
		default:
			break;
		}
	}

	return XR_ENV_BLEND_MODE_OPAQUE;
}

bool OpenXRInterface::set_environment_blend_mode(XRInterface::EnvironmentBlendMode mode)
{
	if (OpenXRAPI::is_allocated()) {
		XrEnvironmentBlendMode oxr_blend_mode;
		switch (mode) {
		case XR_ENV_BLEND_MODE_OPAQUE:
			oxr_blend_mode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
			break;
		case XR_ENV_BLEND_MODE_ADDITIVE:
			oxr_blend_mode = XR_ENVIRONMENT_BLEND_MODE_ADDITIVE;
			break;
		case XR_ENV_BLEND_MODE_ALPHA_BLEND:
			oxr_blend_mode = XR_ENVIRONMENT_BLEND_MODE_ALPHA_BLEND;
			break;
		default:
			WARN_PRINT("Unknown blend mode requested: " + String::num_int64(int64_t(mode)));
			oxr_blend_mode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
		}
		return OpenXRAPI::set_environment_blend_mode(oxr_blend_mode);
	}
	return false;
}

void OpenXRInterface::on_reference_space_change_pending(XrReferenceSpaceType p_type)
{
	reference_stage_changing = true;

	// Emit play area bounds changed signal when the reference space changes.
	PlayAreaMode mode = XR_PLAY_AREA_UNKNOWN;

	switch (p_type) {
	case XR_REFERENCE_SPACE_TYPE_VIEW:
		mode = XR_PLAY_AREA_3DOF;
		break;
	case XR_REFERENCE_SPACE_TYPE_LOCAL:
		mode = XR_PLAY_AREA_SITTING;
		break;
	case XR_REFERENCE_SPACE_TYPE_STAGE:
		mode = XR_PLAY_AREA_STAGE;
		break;
	case XR_REFERENCE_SPACE_TYPE_LOCAL_FLOOR:
		mode = XR_PLAY_AREA_ROOMSCALE;
		break;
	default:
		mode = XR_PLAY_AREA_UNKNOWN;
		break;
	}

	print_verbose("OpenXR Interface: Play area changed, emitting signal.");
}

OpenXRInterface::SessionState OpenXRInterface::get_session_state()
{
	if (OpenXRAPI::is_allocated()) {
		return (SessionState)OpenXRAPI::get_session_state();
	}

	return SESSION_STATE_UNKNOWN;
}

/** User Presence. */
bool OpenXRInterface::is_user_presence_supported() const
{
	if (!OpenXRAPI::is_initialized()) {
		return false;
	}
	else {
		return OpenXRUserPresenceExtension::is_initialized() && OpenXRUserPresenceExtension::is_active();
	}
}

bool OpenXRInterface::is_user_present() const
{
	// If extension is unavailable or unsupported, we default to user is present.
	if (!is_user_presence_supported()) {
		return true;
	}
	else {
		return OpenXRUserPresenceExtension::is_user_present();
	}
}

RID OpenXRInterface::get_vrs_texture()
{
	if (!OpenXRAPI::is_allocated()) {
		return RID();
	}

	RID density_map = OpenXRAPI::get_density_map_texture();
	if (density_map.is_valid()) {
		return density_map;
	}

	PackedVector2Array eye_foci;

	Size2 target_size = get_render_target_size();
	real_t aspect_ratio = target_size.x / target_size.y;
	uint32_t view_count = get_view_count();

	for (uint32_t v = 0; v < view_count; v++) {
		eye_foci.push_back(OpenXRAPI::get_eye_focus(v, aspect_ratio));
	}

	xr_vrs.set_vrs_render_region(get_render_region());

	return xr_vrs.make_vrs_texture(target_size, eye_foci);
}

XRInterface::VRSTextureFormat OpenXRInterface::get_vrs_texture_format()
{
	if (!OpenXRAPI::is_allocated()) {
		return XR_VRS_TEXTURE_FORMAT_UNIFIED;
	}

	RID density_map = OpenXRAPI::get_density_map_texture();
	if (density_map.is_valid()) {
		return XR_VRS_TEXTURE_FORMAT_FRAGMENT_DENSITY_MAP;
	}

	return XR_VRS_TEXTURE_FORMAT_UNIFIED;
}

void OpenXRInterface::set_cpu_level(PerfSettingsLevel p_level)
{
	if (OpenXRPerformanceSettingsExtension::is_initialized() && OpenXRPerformanceSettingsExtension::is_available()) {
		OpenXRPerformanceSettingsExtension::set_cpu_level(p_level);
	}
}

void OpenXRInterface::set_gpu_level(PerfSettingsLevel p_level)
{
	if (OpenXRPerformanceSettingsExtension::is_initialized() && OpenXRPerformanceSettingsExtension::is_available()) {
		OpenXRPerformanceSettingsExtension::set_gpu_level(p_level);
	}
}

OpenXRInterface::OpenXRInterface()
{
	if (OpenXRAPI::is_allocated()) {
		OpenXRAPI::set_xr_interface(this);
	}

	// while we don't have head tracking, don't put the headset on the floor...
	_set_default_pos(head_transform, 1.0, 0);
	_set_default_pos(transform_for_view[0], 1.0, 1);
	_set_default_pos(transform_for_view[1], 1.0, 2);
}

OpenXRInterface::~OpenXRInterface()
{
	if (is_initialized()) {
		uninitialize();
	}

	if (OpenXRAPI::is_allocated()) {
		OpenXRAPI::set_xr_interface(nullptr);
	}
}

void OpenXRInterface::uninitialize() {}

void OpenXRInterface::on_refresh_rate_changes(float) {}

void OpenXRInterface::on_gpu_level_changed(OpenXRInterface::PerfSettingsSubDomain,
	OpenXRInterface::PerfSettingsNotificationLevel, OpenXRInterface::PerfSettingsNotificationLevel)
{
}

void OpenXRInterface::on_cpu_level_changed(OpenXRInterface::PerfSettingsSubDomain,
	OpenXRInterface::PerfSettingsNotificationLevel, OpenXRInterface::PerfSettingsNotificationLevel)
{
}

PackedStringArray OpenXRInterface::get_suggested_tracker_names() const
{
	return PackedStringArray();
}

bool OpenXRInterface::initialize() { return false; }

void OpenXRInterface::process() {}

bool OpenXRInterface::is_passthrough_supported() { return false; }



// ============================================================================
// Merged from modules/openxr/action_map/openxr_action_map.cpp
// ============================================================================

#include "modules/openxr/openxr_api.h"

////////////////////////////////////////////////////////////////////////////
// OpenXRAction

Ref<OpenXRAction> OpenXRAction::new_action(const char* p_name, const char* p_localized_name,
	const ActionType p_action_type, const char* p_toplevel_paths)
{
	Ref<OpenXRAction> action;
	action.instantiate();
	action->set_name(String(p_name));
	action->set_localized_name(String(p_localized_name));
	action->set_action_type(p_action_type);
	action->parse_toplevel_paths(String(p_toplevel_paths));

	return action;
}

String OpenXRAction::get_name_with_set() const
{
	String action_name = get_name();

	if (action_set != nullptr) {
		action_name = action_set->get_name() + "/" + action_name;
	}

	return action_name;
}

void OpenXRAction::set_localized_name(const String& p_localized_name)
{
	localized_name = p_localized_name;
	emit_changed();
}

String OpenXRAction::get_localized_name() const { return localized_name; }

void OpenXRAction::set_action_type(const OpenXRAction::ActionType p_action_type)
{
	action_type = p_action_type;
	emit_changed();
}

OpenXRAction::ActionType OpenXRAction::get_action_type() const { return action_type; }

void OpenXRAction::set_toplevel_paths(const PackedStringArray p_toplevel_paths)
{
	toplevel_paths = p_toplevel_paths;
	emit_changed();
}

PackedStringArray OpenXRAction::get_toplevel_paths() const { return toplevel_paths; }

void OpenXRAction::add_toplevel_path(const String& p_toplevel_path)
{
	if (!toplevel_paths.has(p_toplevel_path)) {
		toplevel_paths.push_back(p_toplevel_path);
		emit_changed();
	}
}

void OpenXRAction::rem_toplevel_path(const String& p_toplevel_path)
{
	if (toplevel_paths.has(p_toplevel_path)) {
		toplevel_paths.erase(p_toplevel_path);
		emit_changed();
	}
}

void OpenXRAction::parse_toplevel_paths(const String& p_toplevel_paths)
{
	toplevel_paths = p_toplevel_paths.split(",", false);
	emit_changed();
}

////////////////////////////////////////////////////////////////////////////
// OpenXRActionSet

Ref<OpenXRActionSet> OpenXRActionSet::new_action_set(
	const char* p_name, const char* p_localized_name, const int p_priority)
{
	Ref<OpenXRActionSet> action_set;
	action_set.instantiate();
	action_set->set_name(String(p_name));
	action_set->set_localized_name(p_localized_name);
	action_set->set_priority(p_priority);

	return action_set;
}

void OpenXRActionSet::set_localized_name(const String& p_localized_name)
{
	localized_name = p_localized_name;
	emit_changed();
}

String OpenXRActionSet::get_localized_name() const { return localized_name; }

void OpenXRActionSet::set_priority(const int p_priority)
{
	priority = p_priority;
	emit_changed();
}

int OpenXRActionSet::get_priority() const { return priority; }

Ref<OpenXRAction> OpenXRActionSet::add_new_action(const char* p_name, const char* p_localized_name,
	const OpenXRAction::ActionType p_action_type, const char* p_toplevel_paths)
{
	Ref<OpenXRAction> new_action =
		OpenXRAction::new_action(p_name, p_localized_name, p_action_type, p_toplevel_paths);
	add_action(new_action);
	return new_action;
}

OpenXRActionSet::~OpenXRActionSet() { clear_actions(); }

void OpenXRActionSet::clear_actions() {}

void OpenXRActionSet::add_action(Ref<OpenXRAction> const&) {}

void OpenXRActionSet::remove_action(Ref<OpenXRAction> const&) {}

Ref<OpenXRAction> OpenXRActionSet::get_action(String const&) const { return Ref<OpenXRAction>(); }

////////////////////////////////////////////////////////////////////////////
// OpenXRHapticBase

void OpenXRHapticBase::set_duration(int64_t p_duration)
{
	haptic_vibration.duration = p_duration;
	emit_changed();
}

int64_t OpenXRHapticBase::get_duration() const { return haptic_vibration.duration; }

void OpenXRHapticBase::set_frequency(float p_frequency)
{
	haptic_vibration.frequency = p_frequency;
	emit_changed();
}

float OpenXRHapticBase::get_frequency() const { return haptic_vibration.frequency; }

void OpenXRHapticBase::set_amplitude(float p_amplitude)
{
	haptic_vibration.amplitude = p_amplitude;
	emit_changed();
}

float OpenXRHapticBase::get_amplitude() const { return haptic_vibration.amplitude; }

const XrHapticBaseHeader* OpenXRHapticBase::get_xr_structure()
{
	return (XrHapticBaseHeader*)&haptic_vibration;
}

OpenXRHapticBase::OpenXRHapticBase()
{
	haptic_vibration.type = XR_TYPE_HAPTIC_VIBRATION;
	haptic_vibration.next = nullptr;
	haptic_vibration.duration = -1;
	haptic_vibration.frequency = 0.0;
	haptic_vibration.amplitude = 1.0;
}

////////////////////////////////////////////////////////////////////////////
// OpenXRBindingModifier

PackedByteArray OpenXRBindingModifier::get_ip_modification() { return PackedByteArray(); }

String OpenXRBindingModifier::get_description() const { return ""; }

OpenXRBindingModifier::OpenXRBindingModifier() {}

OpenXRBindingModifier::~OpenXRBindingModifier() {}

////////////////////////////////////////////////////////////////////////////
// OpenXRIPBinding & OpenXRInteractionProfile

Ref<OpenXRIPBinding> OpenXRIPBinding::new_binding(
	const Ref<OpenXRAction>& p_action, const String& p_binding_path)
{
	Ref<OpenXRIPBinding> binding;
	binding.instantiate();
	binding->set_action(p_action);
	binding->set_binding_path(p_binding_path);

	return binding;
}

void OpenXRIPBinding::set_action(const Ref<OpenXRAction>& p_action)
{
	action = p_action;
	emit_changed();
}

Ref<OpenXRAction> OpenXRIPBinding::get_action() const { return action; }

void OpenXRIPBinding::set_binding_path(const String& p_path)
{
	if (OpenXRInteractionProfileMetadata::is_initialized()) {
		binding_path = OpenXRInteractionProfileMetadata::check_path_name(p_path);
	}
	else {
		binding_path = p_path;
	}

	emit_changed();
}

String OpenXRIPBinding::get_binding_path() const { return binding_path; }

int OpenXRIPBinding::get_binding_modifier_count() const { return binding_modifiers.size(); }

Ref<OpenXRBindingModifier> OpenXRIPBinding::get_binding_modifier(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, binding_modifiers.size(), nullptr);

	return binding_modifiers[p_index];
}

void OpenXRIPBinding::clear_binding_modifiers()
{
	if (binding_modifiers.is_empty()) {
		return;
	}

	for (int i = 0; i < binding_modifiers.size(); i++) {
		Ref<OpenXRBindingModifier> binding_modifier = binding_modifiers[i];
		binding_modifier->ip_binding = nullptr;
	}
	binding_modifiers.clear();
	emit_changed();
}

void OpenXRIPBinding::add_binding_modifier(const Ref<OpenXRBindingModifier>& p_binding_modifier)
{
	ERR_FAIL_COND(p_binding_modifier.is_null());

	if (!binding_modifiers.has(p_binding_modifier)) {
		if (p_binding_modifier->ip_binding && p_binding_modifier->ip_binding != this) {
			p_binding_modifier->ip_binding->remove_binding_modifier(p_binding_modifier);
		}

		p_binding_modifier->ip_binding = this;
		binding_modifiers.push_back(p_binding_modifier);
		emit_changed();
	}
}

void OpenXRIPBinding::remove_binding_modifier(const Ref<OpenXRBindingModifier>& p_binding_modifier)
{
	int idx = binding_modifiers.find(p_binding_modifier);
	if (idx != -1) {
		binding_modifiers.remove_at(idx);

		ERR_FAIL_COND_MSG(p_binding_modifier->ip_binding != this,
			"Removing binding modifier that belongs to this binding but had incorrect binding "
			"pointer.");
		p_binding_modifier->ip_binding = nullptr;

		emit_changed();
	}
}

#ifndef DISABLE_DEPRECATED

void OpenXRIPBinding::set_paths(const PackedStringArray& p_paths)
{
	binding_path = "";
	for (const String& path : p_paths) {
		if (!binding_path.is_empty()) {
			binding_path += ",";
		}
		binding_path += path;
	}
}

PackedStringArray OpenXRIPBinding::get_paths() const
{
	PackedStringArray paths = binding_path.split(",", false);
	return paths;
}

int OpenXRIPBinding::get_path_count() const
{
	return binding_path.is_empty() ? 0 : 1;
}

bool OpenXRIPBinding::has_path(const String& p_path) const
{
	return binding_path == p_path;
}

void OpenXRIPBinding::add_path(const String& p_path)
{
	if (binding_path != p_path) {
		ERR_FAIL_COND_MSG(!binding_path.is_empty(),
			"Method add_path has been deprecated. A binding path was already set, create separate "
			"binding resources for each path and use set_binding_path instead.");

		binding_path = p_path;
		emit_changed();
	}
}

void OpenXRIPBinding::remove_path(const String& p_path)
{
	ERR_FAIL_COND_MSG(binding_path != p_path,
		"Method remove_path has been deprecated. Attempt at removing a different binding path, "
		"remove the correct binding record from the interaction profile instead.");

	binding_path = p_path;
	emit_changed();
}

#endif // DISABLE_DEPRECATED

OpenXRIPBinding::~OpenXRIPBinding() { action.unref(); }

Ref<OpenXRInteractionProfile> OpenXRInteractionProfile::new_profile(
	const char* p_input_profile_path)
{
	Ref<OpenXRInteractionProfile> profile;
	profile.instantiate();
	profile->set_interaction_profile_path(String(p_input_profile_path));

	return profile;
}

void OpenXRInteractionProfile::set_interaction_profile_path(const String& p_input_profile_path)
{
	if (OpenXRInteractionProfileMetadata::is_initialized()) {
		interaction_profile_path =
			OpenXRInteractionProfileMetadata::check_profile_name(p_input_profile_path);
	}
	else {
		interaction_profile_path = p_input_profile_path;
	}
	emit_changed();
}

String OpenXRInteractionProfile::get_interaction_profile_path() const
{
	return interaction_profile_path;
}

void OpenXRInteractionProfile::add_new_binding(
	const Ref<OpenXRAction>& p_action, const String& p_paths)
{
	PackedStringArray paths = p_paths.split(",", false);

	for (const String& path : paths) {
		Ref<OpenXRIPBinding> binding = OpenXRIPBinding::new_binding(p_action, path);
		add_binding(binding);
	}
}

int OpenXRInteractionProfile::get_binding_modifier_count() const
{
	return binding_modifiers.size();
}

Ref<OpenXRBindingModifier> OpenXRInteractionProfile::get_binding_modifier(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, binding_modifiers.size(), nullptr);

	return binding_modifiers[p_index];
}

void OpenXRInteractionProfile::clear_binding_modifiers()
{
	if (binding_modifiers.is_empty()) {
		return;
	}

	for (int i = 0; i < binding_modifiers.size(); i++) {
		Ref<OpenXRBindingModifier> binding_modifier = binding_modifiers[i];
		binding_modifier->interaction_profile = nullptr;
	}
	binding_modifiers.clear();
	emit_changed();
}

void OpenXRInteractionProfile::add_binding_modifier(
	const Ref<OpenXRBindingModifier>& p_binding_modifier)
{
	ERR_FAIL_COND(p_binding_modifier.is_null());

	if (!binding_modifiers.has(p_binding_modifier)) {
		if (p_binding_modifier->interaction_profile &&
			p_binding_modifier->interaction_profile != this) {
			p_binding_modifier->interaction_profile->remove_binding_modifier(p_binding_modifier);
		}

		p_binding_modifier->interaction_profile = this;
		binding_modifiers.push_back(p_binding_modifier);
		emit_changed();
	}
}

void OpenXRInteractionProfile::remove_binding_modifier(
	const Ref<OpenXRBindingModifier>& p_binding_modifier)
{
	int idx = binding_modifiers.find(p_binding_modifier);
	if (idx != -1) {
		binding_modifiers.remove_at(idx);

		ERR_FAIL_COND_MSG(p_binding_modifier->interaction_profile != this,
			"Removing binding modifier that belongs to this interaction profile but had incorrect "
			"interaction profile pointer.");
		p_binding_modifier->interaction_profile = nullptr;

		emit_changed();
	}
}

void OpenXRInteractionProfile::add_binding(const Ref<OpenXRIPBinding>& p_binding) {}

////////////////////////////////////////////////////////////////////////////
// OpenXRInteractionProfileMetadata

void OpenXRInteractionProfileMetadata::initialize()
{
	if (data) {
		return;
	}
	data = memnew(Data);
	_register_core_metadata();
}

void OpenXRInteractionProfileMetadata::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

void OpenXRInteractionProfileMetadata::register_profile_rename(
	const String& p_old_name, const String& p_new_name)
{
	ERR_FAIL_COND(data->profile_renames.has(p_old_name));

	data->profile_renames[p_old_name] = p_new_name;
}

String OpenXRInteractionProfileMetadata::check_profile_name(const String& p_name)
{
	if (data->profile_renames.has(p_name)) {
		return data->profile_renames[p_name];
	}

	return p_name;
}

void OpenXRInteractionProfileMetadata::register_path_rename(
	const String& p_old_name, const String& p_new_name)
{
	ERR_FAIL_COND(data->path_renames.has(p_old_name));

	data->path_renames[p_old_name] = p_new_name;
}

String OpenXRInteractionProfileMetadata::check_path_name(const String& p_name)
{
	if (data->path_renames.has(p_name)) {
		return data->path_renames[p_name];
	}

	return p_name;
}

void OpenXRInteractionProfileMetadata::register_top_level_path(const String& p_display_name,
	const String& p_openxr_path, const String& p_openxr_extension_names)
{
	ERR_FAIL_COND_MSG(
		has_top_level_path(p_openxr_path), p_openxr_path + " had already been registered");

	TopLevelPath new_toplevel_path = {p_display_name, p_openxr_path, p_openxr_extension_names};

	data->top_level_paths.push_back(new_toplevel_path);
}

void OpenXRInteractionProfileMetadata::register_interaction_profile(const String& p_display_name,
	const String& p_openxr_path, const String& p_openxr_extension_names)
{
	ERR_FAIL_COND_MSG(
		has_interaction_profile(p_openxr_path), p_openxr_path + " has already been registered");

	InteractionProfile new_profile;
	new_profile.display_name = p_display_name;
	new_profile.openxr_path = p_openxr_path;
	new_profile.openxr_extension_names = p_openxr_extension_names;

	data->interaction_profiles.push_back(new_profile);
}

void OpenXRInteractionProfileMetadata::register_io_path(const String& p_interaction_profile,
	const String& p_display_name, const String& p_toplevel_path, const String& p_openxr_path,
	const String& p_openxr_extension_names, OpenXRAction::ActionType p_action_type)
{
	ERR_FAIL_COND_MSG(!has_interaction_profile(p_interaction_profile),
		"Unknown interaction profile " + p_interaction_profile);
	ERR_FAIL_COND_MSG(
		!has_top_level_path(p_toplevel_path), "Unknown top level path " + p_toplevel_path);

	for (InteractionProfile& interaction_profile : data->interaction_profiles) {
		if (interaction_profile.openxr_path == p_interaction_profile) {
			ERR_FAIL_COND_MSG(interaction_profile.has_io_path(p_openxr_path),
				p_interaction_profile + " already has io path " + p_openxr_path + " registered!");

			IOPath new_io_path = {p_display_name, p_toplevel_path, p_openxr_path,
				p_openxr_extension_names, p_action_type};

			interaction_profile.io_paths.push_back(new_io_path);
		}
	}
}

bool OpenXRInteractionProfileMetadata::has_top_level_path(const String& p_openxr_path)
{
	for (int i = 0; i < data->top_level_paths.size(); i++) {
		if (data->top_level_paths[i].openxr_path == p_openxr_path) {
			return true;
		}
	}

	return false;
}

String OpenXRInteractionProfileMetadata::get_top_level_name(const String& p_openxr_path)
{
	for (int i = 0; i < data->top_level_paths.size(); i++) {
		if (data->top_level_paths[i].openxr_path == p_openxr_path) {
			return data->top_level_paths[i].display_name;
		}
	}

	return String();
}

String OpenXRInteractionProfileMetadata::get_top_level_extensions(const String& p_openxr_path)
{
	for (int i = 0; i < data->top_level_paths.size(); i++) {
		if (data->top_level_paths[i].openxr_path == p_openxr_path) {
			return data->top_level_paths[i].openxr_extension_names;
		}
	}

	return XR_PATH_UNSUPPORTED_NAME;
}

bool OpenXRInteractionProfileMetadata::has_interaction_profile(const String& p_openxr_path)
{
	for (const InteractionProfile& interaction_profile : data->interaction_profiles) {
		if (interaction_profile.openxr_path == p_openxr_path) {
			return true;
		}
	}

	return false;
}

String OpenXRInteractionProfileMetadata::get_interaction_profile_extensions(
	const String& p_openxr_path)
{
	for (const InteractionProfile& interaction_profile : data->interaction_profiles) {
		if (interaction_profile.openxr_path == p_openxr_path) {
			return interaction_profile.openxr_extension_names;
		}
	}

	return XR_PATH_UNSUPPORTED_NAME;
}

const OpenXRInteractionProfileMetadata::InteractionProfile*
OpenXRInteractionProfileMetadata::get_profile(const String& p_openxr_path)
{
	for (const InteractionProfile& interaction_profile : data->interaction_profiles) {
		if (interaction_profile.openxr_path == p_openxr_path) {
			return &interaction_profile;
		}
	}

	return nullptr;
}

bool OpenXRInteractionProfileMetadata::InteractionProfile::has_io_path(
	const String& p_io_path) const
{
	for (int i = 0; i < io_paths.size(); i++) {
		if (io_paths[i].openxr_path == p_io_path) {
			return true;
		}
	}

	return false;
}

const OpenXRInteractionProfileMetadata::IOPath*
OpenXRInteractionProfileMetadata::InteractionProfile::get_io_path(const String& p_io_path) const
{
	for (int i = 0; i < io_paths.size(); i++) {
		if (io_paths[i].openxr_path == p_io_path) {
			return &io_paths[i];
		}
	}

	return nullptr;
}

const OpenXRInteractionProfileMetadata::IOPath* OpenXRInteractionProfileMetadata::get_io_path(
	const String& p_interaction_profile, const String& p_io_path)
{
	const OpenXRInteractionProfileMetadata::InteractionProfile* profile =
		get_profile(p_interaction_profile);
	if (profile != nullptr) {
		return profile->get_io_path(p_io_path);
	}

	return nullptr;
}

PackedStringArray OpenXRInteractionProfileMetadata::get_interaction_profile_paths()
{
	PackedStringArray arr;

	for (const InteractionProfile& interaction_profile : data->interaction_profiles) {
		arr.push_back(interaction_profile.openxr_path);
	}

	return arr;
}

void OpenXRInteractionProfileMetadata::_register_core_metadata()
{
	register_path_rename(
		"/user/hand/left/input/palm_ext/pose", "/user/hand/left/input/grip_surface/pose");
	register_path_rename(
		"/user/hand/right/input/palm_ext/pose", "/user/hand/right/input/grip_surface/pose");

	// Our core toplevel paths
	register_top_level_path("Left hand controller", "/user/hand/left", "");
	register_top_level_path("Right hand controller", "/user/hand/right", "");
	register_top_level_path("Head", "/user/head", "");
	register_top_level_path("Gamepad", "/user/gamepad", "");
	register_top_level_path("Treadmill", "/user/treadmill", "");

	{ // Fallback Khronos simple controller
		const String profile_path = "/interaction_profiles/khr/simple_controller";
		register_interaction_profile("Simple controller", profile_path, "");
		for (const String user_path : {"/user/hand/left", "/user/hand/right"}) {
			register_io_path(profile_path, "Grip pose", user_path, user_path + "/input/grip/pose",
				"", OpenXRAction::OPENXR_ACTION_POSE);
			register_io_path(profile_path, "Aim pose", user_path, user_path + "/input/aim/pose", "",
				OpenXRAction::OPENXR_ACTION_POSE);
			register_io_path(profile_path, "Grip surface pose", user_path,
				user_path + "/input/grip_surface/pose",
				XR_EXT_PALM_POSE_EXTENSION_NAME "," XR_KHR_MAINTENANCE1_EXTENSION_NAME
												"," XR_OPENXR_1_1_NAME,
				OpenXRAction::OPENXR_ACTION_POSE);

			register_io_path(profile_path, "Menu click", user_path, user_path + "/input/menu/click",
				"", OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Select click", user_path,
				user_path + "/input/select/click", "", OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Haptic output", user_path, user_path + "/output/haptic",
				"", OpenXRAction::OPENXR_ACTION_HAPTIC);
		}
	}

	{ // Original HTC Vive wands
		const String profile_path = "/interaction_profiles/htc/vive_controller";
		register_interaction_profile("HTC Vive wand", profile_path, "");
		for (const String user_path : {"/user/hand/left", "/user/hand/right"}) {
			register_io_path(profile_path, "Grip pose", user_path, user_path + "/input/grip/pose",
				"", OpenXRAction::OPENXR_ACTION_POSE);
			register_io_path(profile_path, "Aim pose", user_path, user_path + "/input/aim/pose", "",
				OpenXRAction::OPENXR_ACTION_POSE);
			register_io_path(profile_path, "Grip surface pose", user_path,
				user_path + "/input/grip_surface/pose",
				XR_EXT_PALM_POSE_EXTENSION_NAME "," XR_KHR_MAINTENANCE1_EXTENSION_NAME
												"," XR_OPENXR_1_1_NAME,
				OpenXRAction::OPENXR_ACTION_POSE);

			register_io_path(profile_path, "Menu click", user_path, user_path + "/input/menu/click",
				"", OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "System click", user_path,
				user_path + "/input/system/click", "", OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Trigger", user_path, user_path + "/input/trigger/value",
				"", OpenXRAction::OPENXR_ACTION_FLOAT);
			register_io_path(profile_path, "Trigger click", user_path,
				user_path + "/input/trigger/click", "", OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Squeeze click", user_path,
				user_path + "/input/squeeze/click", "", OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Trackpad", user_path, user_path + "/input/trackpad", "",
				OpenXRAction::OPENXR_ACTION_VECTOR2);
			register_io_path(profile_path, "Trackpad click", user_path,
				user_path + "/input/trackpad/click", "", OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad touch", user_path,
				user_path + "/input/trackpad/touch", "", OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Up", user_path,
				user_path + "/input/trackpad/dpad_up", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Down", user_path,
				user_path + "/input/trackpad/dpad_down", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Left", user_path,
				user_path + "/input/trackpad/dpad_left", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Right", user_path,
				user_path + "/input/trackpad/dpad_right", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Center", user_path,
				user_path + "/input/trackpad/dpad_center", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Haptic output", user_path, user_path + "/output/haptic",
				"", OpenXRAction::OPENXR_ACTION_HAPTIC);
		}
	}

	{ // Microsoft motion controller (original WMR controllers)
		const String profile_path = "/interaction_profiles/microsoft/motion_controller";
		register_interaction_profile("MS Motion controller", profile_path, "");
		for (const String user_path : {"/user/hand/left", "/user/hand/right"}) {
			register_io_path(profile_path, "Grip pose", user_path, user_path + "/input/grip/pose",
				"", OpenXRAction::OPENXR_ACTION_POSE);
			register_io_path(profile_path, "Aim pose", user_path, user_path + "/input/aim/pose", "",
				OpenXRAction::OPENXR_ACTION_POSE);
			register_io_path(profile_path, "Grip surface pose", user_path,
				user_path + "/input/grip_surface/pose",
				XR_EXT_PALM_POSE_EXTENSION_NAME "," XR_KHR_MAINTENANCE1_EXTENSION_NAME
												"," XR_OPENXR_1_1_NAME,
				OpenXRAction::OPENXR_ACTION_POSE);

			register_io_path(profile_path, "Menu click", user_path, user_path + "/input/menu/click",
				"", OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Trigger", user_path, user_path + "/input/trigger/value",
				"", OpenXRAction::OPENXR_ACTION_FLOAT);
			register_io_path(profile_path, "Trigger click", user_path,
				user_path + "/input/trigger/click", "", OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Squeeze click", user_path,
				user_path + "/input/squeeze/click", "", OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Thumbstick", user_path, user_path + "/input/thumbstick",
				"", OpenXRAction::OPENXR_ACTION_VECTOR2);
			register_io_path(profile_path, "Thumbstick click", user_path,
				user_path + "/input/thumbstick/click", "", OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick Dpad Up", user_path,
				user_path + "/input/thumbstick/dpad_up", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick Dpad Down", user_path,
				user_path + "/input/thumbstick/dpad_down", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick Dpad Left", user_path,
				user_path + "/input/thumbstick/dpad_left", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick Dpad Right", user_path,
				user_path + "/input/thumbstick/dpad_right", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Trackpad", user_path, user_path + "/input/trackpad", "",
				OpenXRAction::OPENXR_ACTION_VECTOR2);
			register_io_path(profile_path, "Trackpad click", user_path,
				user_path + "/input/trackpad/click", "", OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad touch", user_path,
				user_path + "/input/trackpad/touch", "", OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Up", user_path,
				user_path + "/input/trackpad/dpad_up", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Down", user_path,
				user_path + "/input/trackpad/dpad_down", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Left", user_path,
				user_path + "/input/trackpad/dpad_left", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Right", user_path,
				user_path + "/input/trackpad/dpad_right", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Center", user_path,
				user_path + "/input/trackpad/dpad_center", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Haptic output", user_path, user_path + "/output/haptic",
				"", OpenXRAction::OPENXR_ACTION_HAPTIC);
		}
	}

	{ // Meta touch controller (original touch controllers, Quest 1 and Quest 2 controllers)
		const String profile_path = "/interaction_profiles/oculus/touch_controller";
		register_interaction_profile("Touch controller", profile_path, "");
		for (const String user_path : {"/user/hand/left", "/user/hand/right"}) {
			register_io_path(profile_path, "Grip pose", user_path, user_path + "/input/grip/pose",
				"", OpenXRAction::OPENXR_ACTION_POSE);
			register_io_path(profile_path, "Aim pose", user_path, user_path + "/input/aim/pose", "",
				OpenXRAction::OPENXR_ACTION_POSE);
			register_io_path(profile_path, "Grip surface pose", user_path,
				user_path + "/input/grip_surface/pose",
				XR_EXT_PALM_POSE_EXTENSION_NAME "," XR_KHR_MAINTENANCE1_EXTENSION_NAME
												"," XR_OPENXR_1_1_NAME,
				OpenXRAction::OPENXR_ACTION_POSE);

			register_io_path(profile_path, "Trigger", user_path, user_path + "/input/trigger/value",
				"", OpenXRAction::OPENXR_ACTION_FLOAT);
			register_io_path(profile_path, "Trigger touch", user_path,
				user_path + "/input/trigger/touch", "", OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Squeeze", user_path, user_path + "/input/squeeze/value",
				"", OpenXRAction::OPENXR_ACTION_FLOAT);

			register_io_path(profile_path, "Thumbstick", user_path, user_path + "/input/thumbstick",
				"", OpenXRAction::OPENXR_ACTION_VECTOR2);
			register_io_path(profile_path, "Thumbstick click", user_path,
				user_path + "/input/thumbstick/click", "", OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick touch", user_path,
				user_path + "/input/thumbstick/touch", "", OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick Dpad Up", user_path,
				user_path + "/input/thumbstick/dpad_up", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick Dpad Down", user_path,
				user_path + "/input/thumbstick/dpad_down", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick Dpad Left", user_path,
				user_path + "/input/thumbstick/dpad_left", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick Dpad Right", user_path,
				user_path + "/input/thumbstick/dpad_right", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Thumbrest touch", user_path,
				user_path + "/input/thumbrest/touch", "", OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Haptic output", user_path, user_path + "/output/haptic",
				"", OpenXRAction::OPENXR_ACTION_HAPTIC);
		}

		register_io_path(profile_path, "Menu click", "/user/hand/left",
			"/user/hand/left/input/menu/click", "", OpenXRAction::OPENXR_ACTION_BOOL);
		register_io_path(profile_path, "System click", "/user/hand/right",
			"/user/hand/right/input/system/click", "", OpenXRAction::OPENXR_ACTION_BOOL);

		register_io_path(profile_path, "X click", "/user/hand/left",
			"/user/hand/left/input/x/click", "", OpenXRAction::OPENXR_ACTION_BOOL);
		register_io_path(profile_path, "X touch", "/user/hand/left",
			"/user/hand/left/input/x/touch", "", OpenXRAction::OPENXR_ACTION_BOOL);
		register_io_path(profile_path, "Y click", "/user/hand/left",
			"/user/hand/left/input/y/click", "", OpenXRAction::OPENXR_ACTION_BOOL);
		register_io_path(profile_path, "Y touch", "/user/hand/left",
			"/user/hand/left/input/y/touch", "", OpenXRAction::OPENXR_ACTION_BOOL);
		register_io_path(profile_path, "A click", "/user/hand/right",
			"/user/hand/right/input/a/click", "", OpenXRAction::OPENXR_ACTION_BOOL);
		register_io_path(profile_path, "A touch", "/user/hand/right",
			"/user/hand/right/input/a/touch", "", OpenXRAction::OPENXR_ACTION_BOOL);
		register_io_path(profile_path, "B click", "/user/hand/right",
			"/user/hand/right/input/b/click", "", OpenXRAction::OPENXR_ACTION_BOOL);
		register_io_path(profile_path, "B touch", "/user/hand/right",
			"/user/hand/right/input/b/touch", "", OpenXRAction::OPENXR_ACTION_BOOL);
	}

	{ // Valve Index controller
		const String profile_path = "/interaction_profiles/valve/index_controller";
		register_interaction_profile("Index controller", profile_path, "");
		for (const String user_path : {"/user/hand/left", "/user/hand/right"}) {
			register_io_path(profile_path, "Grip pose", user_path, user_path + "/input/grip/pose",
				"", OpenXRAction::OPENXR_ACTION_POSE);
			register_io_path(profile_path, "Aim pose", user_path, user_path + "/input/aim/pose", "",
				OpenXRAction::OPENXR_ACTION_POSE);
			register_io_path(profile_path, "Grip surface pose", user_path,
				user_path + "/input/grip_surface/pose",
				XR_EXT_PALM_POSE_EXTENSION_NAME "," XR_KHR_MAINTENANCE1_EXTENSION_NAME
												"," XR_OPENXR_1_1_NAME,
				OpenXRAction::OPENXR_ACTION_POSE);

			register_io_path(profile_path, "System click", user_path,
				user_path + "/input/system/click", "", OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "A click", user_path, user_path + "/input/a/click", "",
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "A touch", user_path, user_path + "/input/a/touch", "",
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "B click", user_path, user_path + "/input/b/click", "",
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "B touch", user_path, user_path + "/input/b/touch", "",
				OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Trigger", user_path, user_path + "/input/trigger/value",
				"", OpenXRAction::OPENXR_ACTION_FLOAT);
			register_io_path(profile_path, "Trigger click", user_path,
				user_path + "/input/trigger/click", "", OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trigger touch", user_path,
				user_path + "/input/trigger/touch", "", OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Squeeze", user_path, user_path + "/input/squeeze/value",
				"", OpenXRAction::OPENXR_ACTION_FLOAT);
			register_io_path(profile_path, "Squeeze force", user_path,
				user_path + "/input/squeeze/force", "", OpenXRAction::OPENXR_ACTION_FLOAT);

			register_io_path(profile_path, "Thumbstick", user_path, user_path + "/input/thumbstick",
				"", OpenXRAction::OPENXR_ACTION_VECTOR2);
			register_io_path(profile_path, "Thumbstick click", user_path,
				user_path + "/input/thumbstick/click", "", OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick touch", user_path,
				user_path + "/input/thumbstick/touch", "", OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick Dpad Up", user_path,
				user_path + "/input/thumbstick/dpad_up", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick Dpad Down", user_path,
				user_path + "/input/thumbstick/dpad_down", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick Dpad Left", user_path,
				user_path + "/input/thumbstick/dpad_left", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Thumbstick Dpad Right", user_path,
				user_path + "/input/thumbstick/dpad_right", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Trackpad", user_path, user_path + "/input/trackpad", "",
				OpenXRAction::OPENXR_ACTION_VECTOR2);
			register_io_path(profile_path, "Trackpad force", user_path,
				user_path + "/input/trackpad/force", "", OpenXRAction::OPENXR_ACTION_FLOAT);
			register_io_path(profile_path, "Trackpad touch", user_path,
				user_path + "/input/trackpad/touch", "", OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Up", user_path,
				user_path + "/input/trackpad/dpad_up", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Down", user_path,
				user_path + "/input/trackpad/dpad_down", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Left", user_path,
				user_path + "/input/trackpad/dpad_left", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Right", user_path,
				user_path + "/input/trackpad/dpad_right", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);
			register_io_path(profile_path, "Trackpad Dpad Center", user_path,
				user_path + "/input/trackpad/dpad_center", XR_EXT_DPAD_BINDING_EXTENSION_NAME,
				OpenXRAction::OPENXR_ACTION_BOOL);

			register_io_path(profile_path, "Haptic output", user_path, user_path + "/output/haptic",
				"", OpenXRAction::OPENXR_ACTION_HAPTIC);
		}
	}
}

////////////////////////////////////////////////////////////////////////////
// OpenXRActionMap

void OpenXRActionMap::create_default_action_sets()
{
	Ref<OpenXRActionSet> action_set = OpenXRActionSet::new_action_set("godot", "Godot action set");
	add_action_set(action_set);

	Ref<OpenXRAction> trigger = action_set->add_new_action("trigger", "Trigger",
		OpenXRAction::OPENXR_ACTION_FLOAT, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> trigger_click = action_set->add_new_action("trigger_click", "Trigger click",
		OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> trigger_touch = action_set->add_new_action("trigger_touch",
		"Trigger touching", OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> grip = action_set->add_new_action(
		"grip", "Grip", OpenXRAction::OPENXR_ACTION_FLOAT, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> grip_click = action_set->add_new_action("grip_click", "Grip click",
		OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> grip_force = action_set->add_new_action("grip_force", "Grip force",
		OpenXRAction::OPENXR_ACTION_FLOAT, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> primary =
		action_set->add_new_action("primary", "Primary joystick/thumbstick/trackpad",
			OpenXRAction::OPENXR_ACTION_VECTOR2, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> primary_click =
		action_set->add_new_action("primary_click", "Primary joystick/thumbstick/trackpad click",
			OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> primary_touch =
		action_set->add_new_action("primary_touch", "Primary joystick/thumbstick/trackpad touching",
			OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> secondary =
		action_set->add_new_action("secondary", "Secondary joystick/thumbstick/trackpad",
			OpenXRAction::OPENXR_ACTION_VECTOR2, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> secondary_click = action_set->add_new_action("secondary_click",
		"Secondary joystick/thumbstick/trackpad click", OpenXRAction::OPENXR_ACTION_BOOL,
		"/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> secondary_touch = action_set->add_new_action("secondary_touch",
		"Secondary joystick/thumbstick/trackpad touching", OpenXRAction::OPENXR_ACTION_BOOL,
		"/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> menu_button = action_set->add_new_action("menu_button", "Menu button",
		OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> select_button = action_set->add_new_action("select_button", "Select button",
		OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> ax_button = action_set->add_new_action("ax_button", "A/X button",
		OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> ax_touch = action_set->add_new_action("ax_touch", "A/X touching",
		OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> by_button = action_set->add_new_action("by_button", "B/Y button",
		OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> by_touch = action_set->add_new_action("by_touch", "B/Y touching",
		OpenXRAction::OPENXR_ACTION_BOOL, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> default_pose =
		action_set->add_new_action("default_pose", "Default pose", OpenXRAction::OPENXR_ACTION_POSE,
			"/user/hand/left,"
			"/user/hand/right,"
			"/user/vive_tracker_htcx/role/left_foot,"
			"/user/vive_tracker_htcx/role/right_foot,"
			"/user/vive_tracker_htcx/role/left_shoulder,"
			"/user/vive_tracker_htcx/role/right_shoulder,"
			"/user/vive_tracker_htcx/role/left_elbow,"
			"/user/vive_tracker_htcx/role/right_elbow,"
			"/user/vive_tracker_htcx/role/left_knee,"
			"/user/vive_tracker_htcx/role/right_knee,"
			"/user/vive_tracker_htcx/role/waist,"
			"/user/vive_tracker_htcx/role/chest,"
			"/user/vive_tracker_htcx/role/camera,"
			"/user/vive_tracker_htcx/role/keyboard,"
			"/user/vive_tracker_htcx/role/left_wrist,"
			"/user/vive_tracker_htcx/role/right_wrist,"
			"/user/vive_tracker_htcx/role/left_ankle,"
			"/user/vive_tracker_htcx/role/right_ankle,"
			"/user/eyes_ext");
	Ref<OpenXRAction> aim_pose = action_set->add_new_action("aim_pose", "Aim pose",
		OpenXRAction::OPENXR_ACTION_POSE, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> grip_pose = action_set->add_new_action("grip_pose", "Grip pose",
		OpenXRAction::OPENXR_ACTION_POSE, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> palm_pose = action_set->add_new_action("palm_pose", "Palm pose",
		OpenXRAction::OPENXR_ACTION_POSE, "/user/hand/left,/user/hand/right");
	Ref<OpenXRAction> haptic =
		action_set->add_new_action("haptic", "Haptic", OpenXRAction::OPENXR_ACTION_HAPTIC,
			"/user/hand/left,"
			"/user/hand/right,"
			"/user/vive_tracker_htcx/role/left_foot,"
			"/user/vive_tracker_htcx/role/right_foot,"
			"/user/vive_tracker_htcx/role/left_shoulder,"
			"/user/vive_tracker_htcx/role/right_shoulder,"
			"/user/vive_tracker_htcx/role/left_elbow,"
			"/user/vive_tracker_htcx/role/right_elbow,"
			"/user/vive_tracker_htcx/role/left_knee,"
			"/user/vive_tracker_htcx/role/right_knee,"
			"/user/vive_tracker_htcx/role/waist,"
			"/user/vive_tracker_htcx/role/chest,"
			"/user/vive_tracker_htcx/role/camera,"
			"/user/vive_tracker_htcx/role/keyboard,"
			"/user/vive_tracker_htcx/role/left_wrist,"
			"/user/vive_tracker_htcx/role/right_wrist,"
			"/user/vive_tracker_htcx/role/left_ankle,"
			"/user/vive_tracker_htcx/role/right_ankle");

	// Create our interaction profiles.
	Ref<OpenXRInteractionProfile> profile =
		OpenXRInteractionProfile::new_profile("/interaction_profiles/khr/generic_controller");
	profile->add_new_binding(
		default_pose, "/user/hand/left/input/aim/pose,/user/hand/right/input/aim/pose");
	profile->add_new_binding(
		aim_pose, "/user/hand/left/input/aim/pose,/user/hand/right/input/aim/pose");
	profile->add_new_binding(
		grip_pose, "/user/hand/left/input/grip/pose,/user/hand/right/input/grip/pose");
	profile->add_new_binding(palm_pose,
		"/user/hand/left/input/grip_surface/pose,/user/hand/right/input/grip_surface/pose");
	profile->add_new_binding(
		ax_button, "/user/hand/left/input/primary/click,/user/hand/right/input/primary/click");
	profile->add_new_binding(
		by_button, "/user/hand/left/input/secondary/click,/user/hand/right/input/secondary/click");
	profile->add_new_binding(
		trigger, "/user/hand/left/input/trigger/value,/user/hand/right/input/trigger/value");
	profile->add_new_binding(
		trigger_click, "/user/hand/left/input/trigger/value,/user/hand/right/input/trigger/value");
	profile->add_new_binding(
		grip, "/user/hand/left/input/squeeze/value,/user/hand/right/input/squeeze/value");
	profile->add_new_binding(
		grip_click, "/user/hand/left/input/squeeze/value,/user/hand/right/input/squeeze/value");
	profile->add_new_binding(
		primary, "/user/hand/left/input/thumbstick,/user/hand/right/input/thumbstick");
	profile->add_new_binding(primary_click,
		"/user/hand/left/input/thumbstick/click,/user/hand/right/input/thumbstick/click");
	profile->add_new_binding(
		haptic, "/user/hand/left/output/haptic,/user/hand/right/output/haptic");
	add_interaction_profile(profile);

	// Create our Meta touch controller profile.
	profile =
		OpenXRInteractionProfile::new_profile("/interaction_profiles/oculus/touch_controller");
	profile->add_new_binding(
		default_pose, "/user/hand/left/input/aim/pose,/user/hand/right/input/aim/pose");
	profile->add_new_binding(
		aim_pose, "/user/hand/left/input/aim/pose,/user/hand/right/input/aim/pose");
	profile->add_new_binding(
		grip_pose, "/user/hand/left/input/grip/pose,/user/hand/right/input/grip/pose");
	profile->add_new_binding(palm_pose,
		"/user/hand/left/input/grip_surface/pose,/user/hand/right/input/grip_surface/pose");
	profile->add_new_binding(
		menu_button, "/user/hand/left/input/menu/click,/user/hand/right/input/system/click");
	profile->add_new_binding(
		ax_button, "/user/hand/left/input/x/click,/user/hand/right/input/a/click");
	profile->add_new_binding(
		ax_touch, "/user/hand/left/input/x/touch,/user/hand/right/input/a/touch");
	profile->add_new_binding(
		by_button, "/user/hand/left/input/y/click,/user/hand/right/input/b/click");
	profile->add_new_binding(
		by_touch, "/user/hand/left/input/y/touch,/user/hand/right/input/b/touch");
	profile->add_new_binding(
		trigger, "/user/hand/left/input/trigger/value,/user/hand/right/input/trigger/value");
	profile->add_new_binding(
		trigger_click, "/user/hand/left/input/trigger/value,/user/hand/right/input/trigger/value");
	profile->add_new_binding(
		trigger_touch, "/user/hand/left/input/trigger/touch,/user/hand/right/input/trigger/touch");
	profile->add_new_binding(
		grip, "/user/hand/left/input/squeeze/value,/user/hand/right/input/squeeze/value");
	profile->add_new_binding(
		grip_click, "/user/hand/left/input/squeeze/value,/user/hand/right/input/squeeze/value");
	profile->add_new_binding(
		primary, "/user/hand/left/input/thumbstick,/user/hand/right/input/thumbstick");
	profile->add_new_binding(primary_click,
		"/user/hand/left/input/thumbstick/click,/user/hand/right/input/thumbstick/click");
	profile->add_new_binding(primary_touch,
		"/user/hand/left/input/thumbstick/touch,/user/hand/right/input/thumbstick/touch");
	profile->add_new_binding(
		haptic, "/user/hand/left/output/haptic,/user/hand/right/output/haptic");
	add_interaction_profile(profile);

	// Create our Pico 4 controller profile.
	profile =
		OpenXRInteractionProfile::new_profile("/interaction_profiles/bytedance/pico4_controller");
	profile->add_new_binding(
		default_pose, "/user/hand/left/input/aim/pose,/user/hand/right/input/aim/pose");
	profile->add_new_binding(
		aim_pose, "/user/hand/left/input/aim/pose,/user/hand/right/input/aim/pose");
	profile->add_new_binding(
		grip_pose, "/user/hand/left/input/grip/pose,/user/hand/right/input/grip/pose");
	profile->add_new_binding(palm_pose,
		"/user/hand/left/input/grip_surface/pose,/user/hand/right/input/grip_surface/pose");
	profile->add_new_binding(
		select_button, "/user/hand/left/input/system/click,/user/hand/right/input/system/click");
	profile->add_new_binding(menu_button, "/user/hand/left/input/menu/click");
	profile->add_new_binding(
		ax_button, "/user/hand/left/input/x/click,/user/hand/right/input/a/click");
	profile->add_new_binding(
		ax_touch, "/user/hand/left/input/x/touch,/user/hand/right/input/a/touch");
	profile->add_new_binding(
		by_button, "/user/hand/left/input/y/click,/user/hand/right/input/b/click");
	profile->add_new_binding(
		by_touch, "/user/hand/left/input/y/touch,/user/hand/right/input/b/touch");
	profile->add_new_binding(
		trigger, "/user/hand/left/input/trigger/value,/user/hand/right/input/trigger/value");
	profile->add_new_binding(
		trigger_click, "/user/hand/left/input/trigger/value,/user/hand/right/input/trigger/value");
	profile->add_new_binding(
		trigger_touch, "/user/hand/left/input/trigger/touch,/user/hand/right/input/trigger/touch");
	profile->add_new_binding(
		grip, "/user/hand/left/input/squeeze/value,/user/hand/right/input/squeeze/value");
	profile->add_new_binding(
		grip_click, "/user/hand/left/input/squeeze/value,/user/hand/right/input/squeeze/value");
	profile->add_new_binding(
		primary, "/user/hand/left/input/thumbstick,/user/hand/right/input/thumbstick");
	profile->add_new_binding(primary_click,
		"/user/hand/left/input/thumbstick/click,/user/hand/right/input/thumbstick/click");
	profile->add_new_binding(primary_touch,
		"/user/hand/left/input/thumbstick/touch,/user/hand/right/input/thumbstick/touch");
	profile->add_new_binding(
		haptic, "/user/hand/left/output/haptic,/user/hand/right/output/haptic");
	add_interaction_profile(profile);

	// Create our hand interaction profile.
	profile =
		OpenXRInteractionProfile::new_profile("/interaction_profiles/ext/hand_interaction_ext");
	profile->add_new_binding(
		default_pose, "/user/hand/left/input/aim/pose,/user/hand/right/input/aim/pose");
	profile->add_new_binding(
		aim_pose, "/user/hand/left/input/aim/pose,/user/hand/right/input/aim/pose");
	profile->add_new_binding(
		grip_pose, "/user/hand/left/input/grip/pose,/user/hand/right/input/grip/pose");
	profile->add_new_binding(palm_pose,
		"/user/hand/left/input/grip_surface/pose,/user/hand/right/input/grip_surface/pose");
	profile->add_new_binding(
		trigger, "/user/hand/left/input/pinch_ext/value,/user/hand/right/input/pinch_ext/value");
	profile->add_new_binding(trigger_click,
		"/user/hand/left/input/pinch_ext/value,/user/hand/right/input/pinch_ext/value");
	profile->add_new_binding(
		grip, "/user/hand/left/input/grasp_ext/value,/user/hand/right/input/grasp_ext/value");
	profile->add_new_binding(
		grip_click, "/user/hand/left/input/grasp_ext/value,/user/hand/right/input/grasp_ext/value");
	add_interaction_profile(profile);
}

void OpenXRActionMap::create_editor_action_sets() {}

Ref<OpenXRAction> OpenXRActionMap::get_action(const String& p_path) const
{
	PackedStringArray paths = p_path.split("/", false);
	ERR_FAIL_COND_V(paths.size() != 2, Ref<OpenXRAction>());

	Ref<OpenXRActionSet> action_set = find_action_set(paths[0]);
	if (action_set.is_valid()) {
		return action_set->get_action(paths[1]);
	}

	return Ref<OpenXRAction>();
}

void OpenXRActionMap::add_interaction_profile(const Ref<OpenXRInteractionProfile>&) {}

int OpenXRActionMap::get_interaction_profile_count() const { return 0; }

void OpenXRActionMap::add_action_set(Ref<OpenXRActionSet> const&) {}

void OpenXRActionMap::remove_interaction_profile(Ref<OpenXRInteractionProfile> const&) {}

void OpenXRActionMap::remove_action_set(Ref<OpenXRActionSet> const&) {}

Ref<OpenXRInteractionProfile> OpenXRActionMap::get_interaction_profile(int) const
{
	return Ref<OpenXRInteractionProfile>();
}

Ref<OpenXRActionSet> OpenXRActionMap::find_action_set(String const&) const
{
	return Ref<OpenXRActionSet>();
}

// ============================================================================
// Merged from modules/openxr/scene/openxr_scene.cpp
// ============================================================================


#include "modules/openxr/openxr_api.h"

#include "core/config/engine.h"
#include "core/config/project_settings.h"

#include "main/main.h"
#include "scene/3d/node_3d.h"
#include "scene/main/viewport.h"
#include "scene/resources/mesh.h"
#include "servers/rendering/renderer.h"
#include "servers/xr/xr_server.h"

////////////////////////////////////////////////////////////////////////////
// OpenXRCompositionLayer

Vector<OpenXRCompositionLayer*> OpenXRCompositionLayer::composition_layer_nodes;

static const char* HOLE_PUNCH_SHADER_CODE =
	"shader_type spatial;\n"
	"render_mode blend_mix, depth_draw_opaque, cull_back, shadow_to_opacity, shadows_disabled;\n"
	"void fragment() {\n"
	"\tALBEDO = vec3(0.0, 0.0, 0.0);\n"
	"}\n";

OpenXRCompositionLayer::OpenXRCompositionLayer(LayerType p_type) : layer_type(p_type)
{
	if (!OpenXRCompositionLayerExtension::is_initialized()) {
		return;
	}

	switch (layer_type) {
	case LAYER_QUAD: {
		XrCompositionLayerQuad openxr_composition_layer = {
			XR_TYPE_COMPOSITION_LAYER_QUAD,
			nullptr,
			0,
			XR_NULL_HANDLE,
			XR_EYE_VISIBILITY_BOTH,
			{},
			{{0, 0, 0, 0}, {0, 0, 0}},
			{(float)quad_size.x, (float)quad_size.y},
		};
		composition_layer = OpenXRCompositionLayerExtension::composition_layer_create(
			(XrCompositionLayerBaseHeader*)&openxr_composition_layer);
	} break;
	case LAYER_CYLINDER: {
		XrCompositionLayerCylinderKHR openxr_composition_layer = {
			XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR,
			nullptr,
			0,
			XR_NULL_HANDLE,
			XR_EYE_VISIBILITY_BOTH,
			{},
			{{0, 0, 0, 0}, {0, 0, 0}},
			radius,
			central_angle,
			aspect_ratio,
		};
		composition_layer = OpenXRCompositionLayerExtension::composition_layer_create(
			(XrCompositionLayerBaseHeader*)&openxr_composition_layer);
	} break;
	case LAYER_EQUIRECT: {
		XrCompositionLayerEquirect2KHR openxr_composition_layer = {
			XR_TYPE_COMPOSITION_LAYER_EQUIRECT2_KHR,
			nullptr,
			0,
			XR_NULL_HANDLE,
			XR_EYE_VISIBILITY_BOTH,
			{},
			{{0, 0, 0, 0}, {0, 0, 0}},
			radius,
			central_horizontal_angle,
			upper_vertical_angle,
			-lower_vertical_angle,
		};
		composition_layer = OpenXRCompositionLayerExtension::composition_layer_create(
			(XrCompositionLayerBaseHeader*)&openxr_composition_layer);
	} break;
	}
}

XrStructureType OpenXRCompositionLayer::_get_openxr_type() const
{
	switch (layer_type) {
	case LAYER_QUAD:
		return XR_TYPE_COMPOSITION_LAYER_QUAD;
	case LAYER_CYLINDER:
		return XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR;
	case LAYER_EQUIRECT:
		return XR_TYPE_COMPOSITION_LAYER_EQUIRECT2_KHR;
	}
	return XR_TYPE_COMPOSITION_LAYER_QUAD;
}

Ref<Mesh> OpenXRCompositionLayer::_create_fallback_mesh()
{
	if (layer_type == LAYER_QUAD) {
		Ref<PrimitiveMesh> mesh;
		mesh.instantiate(PrimitiveMesh::MESH_QUAD);
		mesh->set_plane_size(quad_size);
		return mesh;
	}
	Ref<Mesh> m = memnew(Mesh);
	return m;
}

bool OpenXRCompositionLayer::_should_use_fallback_node()
{
	if (Engine::is_editor_hint() || !OpenXRAPI::is_allocated()) {
		return true;
	}
	else if (openxr_session_running) {
		return enable_hole_punch || !is_natively_supported();
	}
	return false;
}

void OpenXRCompositionLayer::_create_fallback_node()
{
	ERR_FAIL_COND(fallback);
	fallback = memnew(MeshInstance3D);
	fallback->set_cast_shadows_setting(RenderInstance3D::SHADOW_CASTING_SETTING_OFF);
	add_child(fallback, false, INTERNAL_MODE_FRONT);
	should_update_fallback_mesh = true;
}

void OpenXRCompositionLayer::_remove_fallback_node()
{
	ERR_FAIL_COND(fallback == nullptr);
	remove_child(fallback);
	fallback->queue_free();
	fallback = nullptr;
}

void OpenXRCompositionLayer::_setup_composition_layer()
{
	if (use_android_surface || layer_viewport) {
		if (OpenXRCompositionLayerExtension::is_initialized()) {
			OpenXRCompositionLayerExtension::composition_layer_register(composition_layer);
			registered = true;

			if (layer_viewport) {
				OpenXRCompositionLayerExtension::composition_layer_set_viewport(composition_layer,
					layer_viewport->get_viewport_rid(), layer_viewport->get_size());
			}
		}
	}
}

void OpenXRCompositionLayer::_clear_composition_layer()
{
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_unregister(composition_layer);
		registered = false;

		if (!use_android_surface) {
			OpenXRCompositionLayerExtension::composition_layer_set_viewport(
				composition_layer, RID(), Size2i());
		}
	}
}

void OpenXRCompositionLayer::_viewport_size_changed()
{
	if (layer_viewport && openxr_session_running &&
		OpenXRCompositionLayerExtension::is_initialized() && is_natively_supported() &&
		is_visible_in_tree() && is_inside_tree()) {
		OpenXRCompositionLayerExtension::composition_layer_set_viewport(
			composition_layer, layer_viewport->get_viewport_rid(), layer_viewport->get_size());
	}
}

void OpenXRCompositionLayer::_on_openxr_session_begun()
{
	openxr_session_running = true;
	if (_should_register()) {
		_setup_composition_layer();
	}
	if (!fallback && _should_use_fallback_node()) {
		_create_fallback_node();
	}
}

void OpenXRCompositionLayer::_on_openxr_session_stopping()
{
	openxr_session_running = false;
	if (fallback && !_should_use_fallback_node()) {
		_remove_fallback_node();
	}
	_clear_composition_layer();
}

void OpenXRCompositionLayer::update_fallback_mesh() { should_update_fallback_mesh = true; }

bool OpenXRCompositionLayer::_should_register()
{
	return !registered && openxr_session_running && is_inside_tree() && is_visible_in_tree() &&
		   is_natively_supported();
}

bool OpenXRCompositionLayer::is_viewport_in_use(SubViewport* p_viewport)
{
	ERR_FAIL_NULL_V(p_viewport, false);
	for (const OpenXRCompositionLayer* other_composition_layer : composition_layer_nodes) {
		if (other_composition_layer != this && other_composition_layer->is_inside_tree() &&
			other_composition_layer->get_layer_viewport() == p_viewport) {
			return true;
		}
	}
	return false;
}

void OpenXRCompositionLayer::set_use_android_surface(bool p_use_android_surface)
{
	if (use_android_surface == p_use_android_surface) {
		return;
	}

	use_android_surface = p_use_android_surface;
	if (use_android_surface) {
		set_layer_viewport(nullptr);
		if (OpenXRCompositionLayerExtension::is_initialized()) {
			OpenXRCompositionLayerExtension::composition_layer_set_use_android_surface(
				composition_layer, true, android_surface_size);
		}
		if (_should_register()) {
			_setup_composition_layer();
		}
	}
	else {
		if (OpenXRCompositionLayerExtension::is_initialized()) {
			OpenXRCompositionLayerExtension::composition_layer_set_use_android_surface(
				composition_layer, false, Size2i());
		}
	}
}

bool OpenXRCompositionLayer::get_use_android_surface() const { return use_android_surface; }

void OpenXRCompositionLayer::set_android_surface_size(Size2i p_size)
{
	if (android_surface_size == p_size) {
		return;
	}

	android_surface_size = p_size;
	if (use_android_surface && OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_use_android_surface(
			composition_layer, true, android_surface_size);
	}
}

Size2i OpenXRCompositionLayer::get_android_surface_size() const { return android_surface_size; }

SubViewport* OpenXRCompositionLayer::get_layer_viewport() const { return layer_viewport; }

bool OpenXRCompositionLayer::get_enable_hole_punch() const { return enable_hole_punch; }

int OpenXRCompositionLayer::get_sort_order() const { return sort_order; }

void OpenXRCompositionLayer::set_alpha_blend(bool p_alpha_blend)
{
	alpha_blend = p_alpha_blend;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_alpha_blend(
			composition_layer, p_alpha_blend);
	}
	if (fallback) {
		_reset_fallback_material();
	}
}

bool OpenXRCompositionLayer::get_alpha_blend() const { return alpha_blend; }

bool OpenXRCompositionLayer::is_natively_supported() const
{
	if (OpenXRCompositionLayerExtension::is_initialized() && OpenXRAPI::is_allocated()) {
		return OpenXRCompositionLayerExtension::is_available(_get_openxr_type());
	}
	return false;
}

void OpenXRCompositionLayer::set_protected_content(bool p_protected_content)
{
	if (protected_content == p_protected_content) {
		return;
	}
	protected_content = p_protected_content;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_protected_content(
			composition_layer, p_protected_content);
	}
}

bool OpenXRCompositionLayer::is_protected_content() const { return protected_content; }

void OpenXRCompositionLayer::set_min_filter(Filter p_mode)
{
	if (min_filter == p_mode) {
		return;
	}
	min_filter = p_mode;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_min_filter(
			composition_layer, (OpenXRCompositionLayerExtension::Filter)p_mode);
	}
}

OpenXRCompositionLayer::Filter OpenXRCompositionLayer::get_min_filter() const { return min_filter; }

void OpenXRCompositionLayer::set_mag_filter(Filter p_mode)
{
	if (mag_filter == p_mode) {
		return;
	}
	mag_filter = p_mode;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_mag_filter(
			composition_layer, (OpenXRCompositionLayerExtension::Filter)p_mode);
	}
}

OpenXRCompositionLayer::Filter OpenXRCompositionLayer::get_mag_filter() const { return mag_filter; }

void OpenXRCompositionLayer::set_mipmap_mode(MipmapMode p_mode)
{
	if (mipmap_mode == p_mode) {
		return;
	}
	mipmap_mode = p_mode;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_mipmap_mode(
			composition_layer, (OpenXRCompositionLayerExtension::MipmapMode)p_mode);
	}
}

OpenXRCompositionLayer::MipmapMode OpenXRCompositionLayer::get_mipmap_mode() const
{
	return mipmap_mode;
}

void OpenXRCompositionLayer::set_horizontal_wrap(Wrap p_mode)
{
	if (horizontal_wrap == p_mode) {
		return;
	}
	horizontal_wrap = p_mode;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_horizontal_wrap(
			composition_layer, (OpenXRCompositionLayerExtension::Wrap)p_mode);
	}
}

OpenXRCompositionLayer::Wrap OpenXRCompositionLayer::get_horizontal_wrap() const
{
	return horizontal_wrap;
}

void OpenXRCompositionLayer::set_vertical_wrap(Wrap p_mode)
{
	if (vertical_wrap == p_mode) {
		return;
	}
	vertical_wrap = p_mode;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_vertical_wrap(
			composition_layer, (OpenXRCompositionLayerExtension::Wrap)p_mode);
	}
}

OpenXRCompositionLayer::Wrap OpenXRCompositionLayer::get_vertical_wrap() const
{
	return vertical_wrap;
}

void OpenXRCompositionLayer::set_red_swizzle(Swizzle p_mode)
{
	if (red_swizzle == p_mode) {
		return;
	}
	red_swizzle = p_mode;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_red_swizzle(
			composition_layer, (OpenXRCompositionLayerExtension::Swizzle)p_mode);
	}
}

OpenXRCompositionLayer::Swizzle OpenXRCompositionLayer::get_red_swizzle() const
{
	return red_swizzle;
}

void OpenXRCompositionLayer::set_green_swizzle(Swizzle p_mode)
{
	if (green_swizzle == p_mode) {
		return;
	}
	green_swizzle = p_mode;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_green_swizzle(
			composition_layer, (OpenXRCompositionLayerExtension::Swizzle)p_mode);
	}
}

OpenXRCompositionLayer::Swizzle OpenXRCompositionLayer::get_green_swizzle() const
{
	return green_swizzle;
}

void OpenXRCompositionLayer::set_blue_swizzle(Swizzle p_mode)
{
	if (blue_swizzle == p_mode) {
		return;
	}
	blue_swizzle = p_mode;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_blue_swizzle(
			composition_layer, (OpenXRCompositionLayerExtension::Swizzle)p_mode);
	}
}

OpenXRCompositionLayer::Swizzle OpenXRCompositionLayer::get_blue_swizzle() const
{
	return blue_swizzle;
}

void OpenXRCompositionLayer::set_alpha_swizzle(Swizzle p_mode)
{
	if (alpha_swizzle == p_mode) {
		return;
	}
	alpha_swizzle = p_mode;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_alpha_swizzle(
			composition_layer, (OpenXRCompositionLayerExtension::Swizzle)p_mode);
	}
}

OpenXRCompositionLayer::Swizzle OpenXRCompositionLayer::get_alpha_swizzle() const
{
	return alpha_swizzle;
}

void OpenXRCompositionLayer::set_max_anisotropy(float p_value)
{
	if (max_anisotropy == p_value) {
		return;
	}
	max_anisotropy = p_value;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_max_anisotropy(
			composition_layer, p_value);
	}
}

float OpenXRCompositionLayer::get_max_anisotropy() const { return max_anisotropy; }

void OpenXRCompositionLayer::set_border_color(const Color& p_color)
{
	if (border_color == p_color) {
		return;
	}
	border_color = p_color;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_border_color(
			composition_layer, p_color);
	}
}

Color OpenXRCompositionLayer::get_border_color() const { return border_color; }

void OpenXRCompositionLayer::set_eye_visibility(EyeVisibility p_eye_visibility)
{
	if (eye_visibility == p_eye_visibility) {
		return;
	}
	eye_visibility = p_eye_visibility;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_eye_visibility(
			composition_layer, (OpenXRCompositionLayerExtension::EyeVisibility)p_eye_visibility);
	}
}

OpenXRCompositionLayer::EyeVisibility OpenXRCompositionLayer::get_eye_visibility() const
{
	return eye_visibility;
}

void OpenXRCompositionLayer::set_quad_size(const Size2& p_size)
{
	quad_size = p_size;
	if (layer_type == LAYER_QUAD && OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_quad_size(composition_layer, p_size);
	}
	update_fallback_mesh();
}

Size2 OpenXRCompositionLayer::get_quad_size() const { return quad_size; }

void OpenXRCompositionLayer::set_radius(float p_radius)
{
	ERR_FAIL_COND(p_radius <= 0);
	radius = p_radius;
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		if (layer_type == LAYER_CYLINDER) {
			OpenXRCompositionLayerExtension::composition_layer_set_cylinder_radius(
				composition_layer, p_radius);
		}
		else if (layer_type == LAYER_EQUIRECT) {
			OpenXRCompositionLayerExtension::composition_layer_set_equirect_radius(
				composition_layer, p_radius);
		}
	}
	update_fallback_mesh();
}

float OpenXRCompositionLayer::get_radius() const { return radius; }

void OpenXRCompositionLayer::set_fallback_segments(uint32_t p_fallback_segments)
{
	ERR_FAIL_COND(p_fallback_segments == 0);
	fallback_segments = p_fallback_segments;
	update_fallback_mesh();
}

uint32_t OpenXRCompositionLayer::get_fallback_segments() const { return fallback_segments; }

void OpenXRCompositionLayer::set_aspect_ratio(float p_aspect_ratio)
{
	ERR_FAIL_COND(p_aspect_ratio <= 0);
	aspect_ratio = p_aspect_ratio;
	if (layer_type == LAYER_CYLINDER && OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_cylinder_aspect_ratio(
			composition_layer, p_aspect_ratio);
	}
	update_fallback_mesh();
}

float OpenXRCompositionLayer::get_aspect_ratio() const { return aspect_ratio; }

void OpenXRCompositionLayer::set_central_angle(float p_central_angle)
{
	ERR_FAIL_COND(p_central_angle <= 0);
	central_angle = p_central_angle;
	if (layer_type == LAYER_CYLINDER && OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_cylinder_central_angle(
			composition_layer, p_central_angle);
	}
	update_fallback_mesh();
}

float OpenXRCompositionLayer::get_central_angle() const { return central_angle; }

void OpenXRCompositionLayer::set_central_horizontal_angle(float p_angle)
{
	ERR_FAIL_COND(p_angle <= 0);
	central_horizontal_angle = p_angle;
	if (layer_type == LAYER_EQUIRECT && OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_equirect_central_horizontal_angle(
			composition_layer, p_angle);
	}
	update_fallback_mesh();
}

float OpenXRCompositionLayer::get_central_horizontal_angle() const
{
	return central_horizontal_angle;
}

void OpenXRCompositionLayer::set_upper_vertical_angle(float p_angle)
{
	ERR_FAIL_COND(p_angle <= 0 || p_angle > (Math::PI / 2.0));
	upper_vertical_angle = p_angle;
	if (layer_type == LAYER_EQUIRECT && OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_equirect_upper_vertical_angle(
			composition_layer, p_angle);
	}
	update_fallback_mesh();
}

float OpenXRCompositionLayer::get_upper_vertical_angle() const
{
	return upper_vertical_angle;
}

void OpenXRCompositionLayer::set_lower_vertical_angle(float p_angle)
{
	ERR_FAIL_COND(p_angle <= 0 || p_angle > (Math::PI / 2.0));
	lower_vertical_angle = p_angle;
	if (layer_type == LAYER_EQUIRECT && OpenXRCompositionLayerExtension::is_initialized()) {
		OpenXRCompositionLayerExtension::composition_layer_set_equirect_lower_vertical_angle(
			composition_layer, -p_angle);
	}
	update_fallback_mesh();
}

float OpenXRCompositionLayer::get_lower_vertical_angle() const
{
	return lower_vertical_angle;
}

Ref<JavaObject> OpenXRCompositionLayer::get_android_surface()
{
	if (OpenXRCompositionLayerExtension::is_initialized()) {
		return OpenXRCompositionLayerExtension::composition_layer_get_android_surface(
			composition_layer);
	}
	return Ref<JavaObject>();
}

Vector2 OpenXRCompositionLayer::intersects_ray(
	const Vector3& p_origin, const Vector3& p_direction) const
{
	switch (layer_type) {
	case LAYER_QUAD: {
		Transform3D quad_transform = get_global_transform();
		Vector3 quad_normal = quad_transform.basis.get_column(2);

		float denom = quad_normal.dot(p_direction);
		if (Math::abs(denom) > 0.0001) {
			Vector3 vector = quad_transform.origin - p_origin;
			float t = vector.dot(quad_normal) / denom;
			if (t < 0.0) {
				return Vector2(-1.0, -1.0);
			}
			Vector3 intersection = p_origin + p_direction * t;

			Vector3 relative_point = intersection - quad_transform.origin;
			Vector2 projected_point = Vector2(relative_point.dot(quad_transform.basis.get_column(0)),
				relative_point.dot(quad_transform.basis.get_column(1)));
			if (Math::abs(projected_point.x) > quad_size.x / 2.0) {
				return Vector2(-1.0, -1.0);
			}
			if (Math::abs(projected_point.y) > quad_size.y / 2.0) {
				return Vector2(-1.0, -1.0);
			}

			float u = 0.5 + (projected_point.x / quad_size.x);
			float v = 1.0 - (0.5 + (projected_point.y / quad_size.y));

			return Vector2(u, v);
		}
		return Vector2(-1.0, -1.0);
	}
	case LAYER_CYLINDER: {
		Transform3D cylinder_transform = get_global_transform();
		Vector3 cylinder_axis = cylinder_transform.basis.get_column(1);

		Vector3 offset = p_origin - cylinder_transform.origin;
		float a = p_direction.dot(p_direction - cylinder_axis * p_direction.dot(cylinder_axis));
		float b = 2.0 * (p_direction.dot(offset - cylinder_axis * offset.dot(cylinder_axis)));
		float c =
			offset.dot(offset - cylinder_axis * offset.dot(cylinder_axis)) - (radius * radius);

		float discriminant = b * b - 4.0 * a * c;
		if (discriminant < 0.0) {
			return Vector2(-1.0, -1.0);
		}

		float t0 = (-b - Math::sqrt(discriminant)) / (2.0 * a);
		float t1 = (-b + Math::sqrt(discriminant)) / (2.0 * a);
		float t = MAX(t0, t1);

		if (t < 0.0) {
			return Vector2(-1.0, -1.0);
		}
		Vector3 intersection = p_origin + p_direction * t;

		Basis correction = cylinder_transform.basis.inverse();
		correction.rotate(Vector3(0.0, 1.0, 0.0), -Math::PI / 2.0);
		Vector3 relative_point = correction.xform(intersection - cylinder_transform.origin);

		Vector2 projected_point = Vector2(relative_point.x, relative_point.z);
		float intersection_angle = Math::atan2(projected_point.y, projected_point.x);
		if (Math::abs(intersection_angle) > central_angle / 2.0) {
			return Vector2(-1.0, -1.0);
		}

		float arc_length = radius * central_angle;
		float height = aspect_ratio * arc_length;
		if (Math::abs(relative_point.y) > height / 2.0) {
			return Vector2(-1.0, -1.0);
		}

		float u = 0.5 + (intersection_angle / central_angle);
		float v = 1.0 - (0.5 + (relative_point.y / height));

		return Vector2(u, v);
	}
	case LAYER_EQUIRECT: {
		Transform3D equirect_transform = get_global_transform();

		Vector3 offset = p_origin - equirect_transform.origin;
		float a = p_direction.dot(p_direction);
		float b = 2.0 * offset.dot(p_direction);
		float c = offset.dot(offset) - (radius * radius);

		float discriminant = b * b - 4.0 * a * c;
		if (discriminant < 0.0) {
			return Vector2(-1.0, -1.0);
		}

		float t0 = (-b - Math::sqrt(discriminant)) / (2.0 * a);
		float t1 = (-b + Math::sqrt(discriminant)) / (2.0 * a);
		float t = MAX(t0, t1);

		if (t < 0.0) {
			return Vector2(-1.0, -1.0);
		}
		Vector3 intersection = p_origin + p_direction * t;

		Basis correction = equirect_transform.basis.inverse();
		correction.rotate(Vector3(0.0, 1.0, 0.0), -Math::PI / 2.0);
		Vector3 relative_point = correction.xform(intersection - equirect_transform.origin);

		float horizontal_intersection_angle = Math::atan2(relative_point.z, relative_point.x);
		if (Math::abs(horizontal_intersection_angle) > central_horizontal_angle / 2.0) {
			return Vector2(-1.0, -1.0);
		}

		float vertical_intersection_angle =
			Math::acos(relative_point.y / radius) - (Math::PI / 2.0);
		if (vertical_intersection_angle < 0) {
			if (Math::abs(vertical_intersection_angle) > upper_vertical_angle) {
				return Vector2(-1.0, -1.0);
			}
		}
		else if (vertical_intersection_angle > lower_vertical_angle) {
			return Vector2(-1.0, -1.0);
		}

		if (upper_vertical_angle != lower_vertical_angle) {
			vertical_intersection_angle -= (-upper_vertical_angle + lower_vertical_angle) / 2.0;
		}

		float u = 0.5 + (horizontal_intersection_angle / central_horizontal_angle);
		float v = 0.5 + (vertical_intersection_angle / (upper_vertical_angle + lower_vertical_angle));

		return Vector2(u, v);
	}
	}
	return Vector2(-1.0, -1.0);
}

void OpenXRCompositionLayer::_reset_fallback_material()
{
	ERR_FAIL_NULL(fallback);

	if (fallback->get_mesh().is_null()) {
		return;
	}

	if (enable_hole_punch && !Engine::is_editor_hint() && is_natively_supported()) {
		Ref<ShaderMaterial> material = fallback->get_surface_override_material(0);
		if (material.is_null()) {
			Ref<Shader> shader;
			shader.instantiate();
			shader->set_code(HOLE_PUNCH_SHADER_CODE);

			material.instantiate();
			material->set_shader(shader);
			fallback->set_surface_override_material(0, material);
		}
	}
	else if (layer_viewport) {
		Ref<StandardMaterial3D> material = fallback->get_surface_override_material(0);
		if (material.is_null()) {
			material.instantiate();
			material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
			material->set_local_to_scene(true);
			fallback->set_surface_override_material(0, material);
		}
		material->set_transparency(get_alpha_blend() ? StandardMaterial3D::TRANSPARENCY_ALPHA
													 : StandardMaterial3D::TRANSPARENCY_DISABLED);
		material->set_texture(StandardMaterial3D::TEXTURE_ALBEDO, layer_viewport->get_texture());
	}
	else {
		fallback->set_surface_override_material(0, Ref<Material>());
	}
}

PackedStringArray OpenXRCompositionLayer::get_configuration_warnings() const
{
	PackedStringArray warnings = Node3D::get_configuration_warnings();

	if (!get_transform().basis.is_orthonormal()) {
		warnings.push_back(RTR("OpenXR composition layers must have orthonormalized transforms "
							   "(ie. no scale or shearing)."));
	}

	if (enable_hole_punch && get_sort_order() >= 0) {
		warnings.push_back(
			RTR("Hole punching won't work as expected unless the sort order is less than zero."));
	}

	return warnings;
}

Camera3D* OpenXRCompositionLayer::_get_xrcamera3d_ancestor() const
{
	Node* parent = get_parent();
	while (parent != nullptr) {
		Camera3D* camera = dynamic_cast<Camera3D*>(parent);
		if (camera != nullptr && camera->is_xr_enabled()) {
			return camera;
		}
		parent = parent->get_parent();
	}
	return nullptr;
}

void OpenXRCompositionLayer::set_layer_viewport(SubViewport* p_viewport) {}

////////////////////////////////////////////////////////////////////////////
// OpenXRHand

OpenXRHand::OpenXRHand() {}

void OpenXRHand::set_hand(Hands p_hand)
{
	ERR_FAIL_INDEX(p_hand, HAND_MAX);

	hand = p_hand;
}

OpenXRHand::Hands OpenXRHand::get_hand() const { return hand; }

void OpenXRHand::set_hand_skeleton(const NodePath& p_hand_skeleton)
{
	hand_skeleton = p_hand_skeleton;
}

void OpenXRHand::set_motion_range(MotionRange p_motion_range)
{
	ERR_FAIL_INDEX(p_motion_range, MOTION_RANGE_MAX);
	motion_range = p_motion_range;

	_set_motion_range();
}

OpenXRHand::MotionRange OpenXRHand::get_motion_range() const { return motion_range; }

NodePath OpenXRHand::get_hand_skeleton() const { return hand_skeleton; }

void OpenXRHand::set_skeleton_rig(SkeletonRig p_skeleton_rig)
{
	ERR_FAIL_INDEX(p_skeleton_rig, SKELETON_RIG_MAX);

	skeleton_rig = p_skeleton_rig;
}

OpenXRHand::SkeletonRig OpenXRHand::get_skeleton_rig() const { return skeleton_rig; }

void OpenXRHand::set_bone_update(BoneUpdate p_bone_update)
{
	ERR_FAIL_INDEX(p_bone_update, BONE_UPDATE_MAX);

	bone_update = p_bone_update;
}

OpenXRHand::BoneUpdate OpenXRHand::get_bone_update() const { return bone_update; }

void OpenXRHand::_get_joint_data()
{
	static const String bone_names[SKELETON_RIG_MAX][XR_HAND_JOINT_COUNT_EXT] = {
		{"Palm", "Wrist", "Thumb_Metacarpal", "Thumb_Proximal", "Thumb_Distal", "Thumb_Tip",
			"Index_Metacarpal", "Index_Proximal", "Index_Intermediate", "Index_Distal", "Index_Tip",
			"Middle_Metacarpal", "Middle_Proximal", "Middle_Intermediate", "Middle_Distal",
			"Middle_Tip", "Ring_Metacarpal", "Ring_Proximal", "Ring_Intermediate", "Ring_Distal",
			"Ring_Tip", "Little_Metacarpal", "Little_Proximal", "Little_Intermediate",
			"Little_Distal", "Little_Tip"},
		{"Palm", "Hand", "ThumbMetacarpal", "ThumbProximal", "ThumbDistal", "ThumbTip",
			"IndexMetacarpal", "IndexProximal", "IndexIntermediate", "IndexDistal", "IndexTip",
			"MiddleMetacarpal", "MiddleProximal", "MiddleIntermediate", "MiddleDistal", "MiddleTip",
			"RingMetacarpal", "RingProximal", "RingIntermediate", "RingDistal", "RingTip",
			"LittleMetacarpal", "LittleProximal", "LittleIntermediate", "LittleDistal",
			"LittleTip"}};

	static const String bone_name_formats[SKELETON_RIG_MAX][2] = {
		{"<bone>_L", "<bone>_R"},
		{"Left<bone>", "Right<bone>"}};

	for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; i++) {
		joints[i].bone = -1;
		joints[i].parent_joint = -1;
	}

	Skeleton3D* skeleton = get_skeleton();
	if (!skeleton) {
		return;
	}

	int bones[XR_HAND_JOINT_COUNT_EXT];
	for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; i++) {
		String bone_name =
			bone_name_formats[skeleton_rig][hand].replace("<bone>", bone_names[skeleton_rig][i]);
		bones[i] = skeleton->find_bone(bone_name);
	}

	for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; i++) {
		const int bone = bones[i];
		if (bone == -1) {
			continue;
		}

		const int parent_bone = skeleton->get_bone_parent(bone);
		if (parent_bone == -1) {
			joints[i].bone = bone;
			joints[i].parent_joint = XR_HAND_JOINT_PALM_EXT;
			continue;
		}

		for (int j = 0; j < XR_HAND_JOINT_COUNT_EXT; ++j) {
			if (bones[j] == parent_bone) {
				joints[i].bone = bone;
				joints[i].parent_joint = j;
				break;
			}
		}
	}
}

void OpenXRHand::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		_get_joint_data();
		set_process_internal(true);
	} break;
	case NOTIFICATION_EXIT_TREE: {
		set_process_internal(false);
		for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; i++) {
			joints[i].bone = -1;
			joints[i].parent_joint = -1;
		}
	} break;
	case NOTIFICATION_INTERNAL_PROCESS: {
		_update_skeleton();
	} break;
	default: {
	} break;
	}
}

void OpenXRHand::_update_skeleton() {}

void OpenXRHand::_set_motion_range() {}

Skeleton3D* OpenXRHand::get_skeleton() { return nullptr; }

////////////////////////////////////////////////////////////////////////////
// OpenXRVisibilityMask

void OpenXRVisibilityMask::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

void OpenXRVisibilityMask::_notification(int p_what)
{
	render_instance.notification(this, p_what);

	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		if (OpenXRVisibilityMaskExtension::is_initialized() &&
			OpenXRVisibilityMaskExtension::is_available()) {
			set_base(OpenXRVisibilityMaskExtension::get_mesh());
		}
	} break;
	case NOTIFICATION_EXIT_TREE: {
		set_base(RID());
	} break;
	}
}

void OpenXRVisibilityMask::_on_openxr_session_begun()
{
	if (is_inside_tree()) {
		if (OpenXRVisibilityMaskExtension::is_initialized() &&
			OpenXRVisibilityMaskExtension::is_available()) {
			set_base(OpenXRVisibilityMaskExtension::get_mesh());
		}
	}
}

void OpenXRVisibilityMask::_on_openxr_session_stopping() { set_base(RID()); }

AABB OpenXRVisibilityMask::get_aabb() const
{
	AABB ret;
	ret.position = Vector3(-1000.0, -1000.0, -1000.0);
	ret.size = Vector3(2000.0, 2000.0, 2000.0);
	return ret;
}

////////////////////////////////////////////////////////////////////////////
// OpenXRRenderModel & OpenXRRenderModelManager

#ifdef MODULE_GLTF_ENABLED

void OpenXRRenderModel::_load_render_model_scene() {}

String OpenXRRenderModel::get_top_level_path() const
{
	String ret;

	if (render_model.is_valid() && OpenXRRenderModelExtension::is_initialized()) {
		ret = OpenXRRenderModelExtension::render_model_get_top_level_path_as_string(render_model);
	}

	return ret;
}

RID OpenXRRenderModel::get_render_model() const { return render_model; }

void OpenXRRenderModel::set_render_model(RID p_render_model)
{
	render_model = p_render_model;
	if (is_inside_tree() && render_model.is_valid()) {
		_load_render_model_scene();
	}
}

bool OpenXRRenderModelManager::_has_filters() { return tracker != 0; }

void OpenXRRenderModelManager::_on_render_model_added(RID p_render_model)
{
	if (_has_filters()) {
		is_dirty = true;
	}
	else {
		_update_models();
	}
}

void OpenXRRenderModelManager::_on_render_model_removed(RID p_render_model)
{
	if (_has_filters()) {
		is_dirty = true;
	}
	else {
		_update_models();
	}
}

void OpenXRRenderModelManager::_on_render_model_top_level_path_changed(RID p_path)
{
	if (_has_filters()) {
		is_dirty = true;
	}
}

void OpenXRRenderModelManager::set_tracker(RenderModelTracker p_tracker)
{
	if (tracker != p_tracker) {
		tracker = p_tracker;
		is_dirty = true;

		if (tracker == RENDER_MODEL_TRACKER_ANY || tracker == RENDER_MODEL_TRACKER_NONE_SET) {
			xr_path = XR_NULL_PATH;
		}
		else if (!Engine::is_editor_hint()) {
			if (OpenXRAPI::is_allocated() && XRServer::is_initialized()) {
				String toplevel_path;
				String tracker_name;
				if (tracker == RENDER_MODEL_TRACKER_LEFT_HAND) {
					tracker_name = "left_hand";
					toplevel_path = "/user/hand/left";
				}
				else if (tracker == RENDER_MODEL_TRACKER_RIGHT_HAND) {
					tracker_name = "right_hand";
					toplevel_path = "/user/hand/right";
				}
				else {
					ERR_FAIL_MSG("Unsupported tracker value set.");
				}

				positional_tracker = XRServer::get_tracker(tracker_name);
				if (positional_tracker.is_null()) {
					WARN_PRINT("OpenXR: Can't find tracker " + tracker_name);
				}

				xr_path = OpenXRAPI::get_xr_path(toplevel_path);
				if (xr_path == XR_NULL_PATH) {
					WARN_PRINT("OpenXR: Can't find path for " + toplevel_path);
				}
			}
		}

		set_process_internal(true);
	}
}

OpenXRRenderModelManager::RenderModelTracker OpenXRRenderModelManager::get_tracker() const
{
	return tracker;
}

void OpenXRRenderModelManager::set_make_local_to_pose(const String& p_action)
{
	if (make_local_to_pose != p_action) {
		make_local_to_pose = p_action;

		if (container) {
			container->set_transform(Transform3D());
		}
	}
}

String OpenXRRenderModelManager::get_make_local_to_pose() const { return make_local_to_pose; }

void OpenXRRenderModelManager::_update_models() {}

#endif // MODULE_GLTF_ENABLED

// ============================================================================
// Merged from modules/openxr/extensions/openxr_extensions.cpp
// ============================================================================

#include "modules/openxr/openxr_api.h"
#include "core/config/project_settings.h"
#include "core/os/os.h"
#include "core/string/print_string.h"

#include "main/main.h"
#include "servers/rendering/renderer.h"
#include "servers/xr/xr_server.h"

#ifdef XR_USE_PLATFORM_ANDROID
#include <unistd.h>
#endif

////////////////////////////////////////////////////////////////////////////
// OpenXREyeGazeInteractionExtension

void OpenXREyeGazeInteractionExtension::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXREyeGazeInteractionExtension::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

HashMap<String, bool*> OpenXREyeGazeInteractionExtension::get_requested_extensions(
	XrVersion p_version)
{
	HashMap<String, bool*> request_extensions;

	if (GLOBAL_GET_CACHED(bool, "xr/openxr/extensions/eye_gaze_interaction") &&
		(!OS::has_feature("mobile") ||
			OS::has_feature(XR_EXT_EYE_GAZE_INTERACTION_EXTENSION_NAME))) {
		request_extensions[XR_EXT_EYE_GAZE_INTERACTION_EXTENSION_NAME] = &data->available;
	}

	return request_extensions;
}

void* OpenXREyeGazeInteractionExtension::set_system_properties_and_get_next_pointer(
	void* p_next_pointer)
{
	if (!data->available) {
		return p_next_pointer;
	}

	data->properties.type = XR_TYPE_SYSTEM_EYE_GAZE_INTERACTION_PROPERTIES_EXT;
	data->properties.next = p_next_pointer;
	data->properties.supportsEyeGazeInteraction = false;

	return &data->properties;
}

PackedStringArray OpenXREyeGazeInteractionExtension::get_suggested_tracker_names()
{
	PackedStringArray arr = {"/user/eyes_ext"};
	return arr;
}

bool OpenXREyeGazeInteractionExtension::is_available() { return data != nullptr && data->available; }

bool OpenXREyeGazeInteractionExtension::supports_eye_gaze_interaction()
{
	if (data->available && data->properties.supportsEyeGazeInteraction) {
		return !OS::has_feature("mobile") ||
			   OS::has_feature("PERMISSION_XR_EXT_eye_gaze_interaction");
	}

	return false;
}

bool OpenXREyeGazeInteractionExtension::get_eye_gaze_pose(double p_dist, Vector3& r_eye_pose)
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), false);

	if (!data->init_eye_gaze_pose) {
		data->init_eye_gaze_pose = true;

		data->eye_tracker = OpenXRAPI::find_tracker("/user/eyes_ext");
		if (data->eye_tracker.is_null()) {
			WARN_PRINT("Couldn't obtain eye tracker");
		}

		data->eye_action = OpenXRAPI::find_action("eye_gaze_pose");
		if (data->eye_action.is_null()) {
			WARN_PRINT("Couldn't obtain pose action for `eye_gaze_pose`, make sure to add this to "
					   "your action map.");
		}
	}

	if (data->eye_tracker.is_null() || data->eye_action.is_null()) {
		return false;
	}

	Transform3D eye_transform;
	Vector3 linear_velocity;
	Vector3 angular_velocity;
	XRPose::TrackingConfidence confidence = OpenXRAPI::get_action_pose(
		data->eye_action, data->eye_tracker, eye_transform, linear_velocity, angular_velocity);
	if (confidence == XRPose::XR_TRACKING_CONFIDENCE_NONE) {
		return false;
	}

	r_eye_pose = eye_transform.origin + eye_transform.basis[2] * p_dist;

	return true;
}

////////////////////////////////////////////////////////////////////////////
// OpenXRUserPresenceExtension

void OpenXRUserPresenceExtension::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXRUserPresenceExtension::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

void* OpenXRUserPresenceExtension::set_system_properties_and_get_next_pointer(void* p_next_pointer)
{
	if (!data->available) {
		return p_next_pointer;
	}

	data->properties.type = XR_TYPE_SYSTEM_USER_PRESENCE_PROPERTIES_EXT;
	data->properties.next = p_next_pointer;
	data->properties.supportsUserPresence = false;

	return &data->properties;
}

bool OpenXRUserPresenceExtension::is_active()
{
	return data->available && data->properties.supportsUserPresence;
}

void OpenXRUserPresenceExtension::on_state_ready() { data->user_present = true; }

void OpenXRUserPresenceExtension::on_state_stopping() { data->user_present = false; }

bool OpenXRUserPresenceExtension::is_user_present()
{
	return data != nullptr && data->user_present;
}

HashMap<String, bool*> OpenXRUserPresenceExtension::get_requested_extensions(XrVersion)
{
	return HashMap<String, bool*>();
}

bool OpenXRUserPresenceExtension::on_event_polled(const XrEventDataBuffer&) { return false; }

////////////////////////////////////////////////////////////////////////////
// OpenXRAndroidThreadSettingsExtension

void OpenXRAndroidThreadSettingsExtension::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXRAndroidThreadSettingsExtension::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

HashMap<String, bool*> OpenXRAndroidThreadSettingsExtension::get_requested_extensions(XrVersion)
{
	HashMap<String, bool*> request_extensions;

#ifdef XR_USE_PLATFORM_ANDROID
	request_extensions[XR_KHR_ANDROID_THREAD_SETTINGS_EXTENSION_NAME] = &data->available;
#endif

	return request_extensions;
}

void OpenXRAndroidThreadSettingsExtension::on_instance_created(XrInstance)
{
	if (!data->available) {
		return;
	}

	if (!_initialize_openxr_android_thread_settings_extension()) {
		print_error("OpenXR: Failed to initialize android thread settings extension");
		data->available = false;
	}
}

bool OpenXRAndroidThreadSettingsExtension::set_application_thread_type(
	ThreadType p_thread_type, uint32_t p_thread_id)
{
	if (!data->available) {
		return false;
	}

#ifdef XR_USE_PLATFORM_ANDROID
	XrAndroidThreadTypeKHR thread_type{};
	switch (p_thread_type) {
	case THREAD_TYPE_APPLICATION_MAIN:
		thread_type = XR_ANDROID_THREAD_TYPE_APPLICATION_MAIN_KHR;
		break;
	case THREAD_TYPE_APPLICATION_WORKER:
		thread_type = XR_ANDROID_THREAD_TYPE_APPLICATION_WORKER_KHR;
		break;
	case THREAD_TYPE_RENDERER_MAIN:
		thread_type = XR_ANDROID_THREAD_TYPE_RENDERER_MAIN_KHR;
		break;
	case THREAD_TYPE_RENDERER_WORKER:
		thread_type = XR_ANDROID_THREAD_TYPE_RENDERER_WORKER_KHR;
		break;
	default:
		print_error(
			vformat("OpenXR: Failed to set android application thread; invalid thread type %d",
				p_thread_type));
		return false;
	}

	XrResult result = xrSetAndroidApplicationThreadKHR(
		OpenXRAPI::get_session(), thread_type, p_thread_id == 0 ? gettid() : p_thread_id);
	if (result != XR_SUCCESS) {
		print_error(vformat("OpenXR: Failed to set android application thread; %s",
			OpenXRAPI::get_error_string(result)));
		return false;
	}
#endif

	return true;
}

bool OpenXRAndroidThreadSettingsExtension::_initialize_openxr_android_thread_settings_extension()
{
#ifdef XR_USE_PLATFORM_ANDROID
	EXT_INIT_XR_FUNC_V(xrSetAndroidApplicationThreadKHR);
#endif
	return true;
}

void OpenXRAndroidThreadSettingsExtension::_set_render_thread_type()
{
	if (Thread::is_main_thread()) {
		return;
	}

	set_application_thread_type(THREAD_TYPE_RENDERER_MAIN);
}

void OpenXRAndroidThreadSettingsExtension::on_session_created(XrSession) {}

////////////////////////////////////////////////////////////////////////////
// OpenXRDisplayRefreshRateExtension

void OpenXRDisplayRefreshRateExtension::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXRDisplayRefreshRateExtension::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

HashMap<String, bool*> OpenXRDisplayRefreshRateExtension::get_requested_extensions(XrVersion)
{
	HashMap<String, bool*> request_extensions;

	request_extensions[XR_FB_DISPLAY_REFRESH_RATE_EXTENSION_NAME] = &data->display_refresh_rate_ext;

	return request_extensions;
}

void OpenXRDisplayRefreshRateExtension::on_instance_created(const XrInstance)
{
	if (data->display_refresh_rate_ext) {
		EXT_INIT_XR_FUNC(xrEnumerateDisplayRefreshRatesFB);
		EXT_INIT_XR_FUNC(xrGetDisplayRefreshRateFB);
		EXT_INIT_XR_FUNC(xrRequestDisplayRefreshRateFB);
	}
}

void OpenXRDisplayRefreshRateExtension::on_instance_destroyed()
{
	data->display_refresh_rate_ext = false;
}

bool OpenXRDisplayRefreshRateExtension::on_event_polled(const XrEventDataBuffer& event)
{
	switch (event.type) {
	case XR_TYPE_EVENT_DATA_DISPLAY_REFRESH_RATE_CHANGED_FB: {
		const XrEventDataDisplayRefreshRateChangedFB* event_fb =
			(const XrEventDataDisplayRefreshRateChangedFB*)&event;

		OpenXRInterface* xr_interface = OpenXRAPI::get_xr_interface();
		if (xr_interface) {
			xr_interface->on_refresh_rate_changes(event_fb->toDisplayRefreshRate);
		}

		return true;
	} break;
	default:
		return false;
	}
}

void OpenXRDisplayRefreshRateExtension::set_refresh_rate(float) {}

float OpenXRDisplayRefreshRateExtension::get_refresh_rate() { return 0.0f; }

////////////////////////////////////////////////////////////////////////////
// OpenXRPerformanceSettingsExtension

void OpenXRPerformanceSettingsExtension::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXRPerformanceSettingsExtension::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

HashMap<String, bool*> OpenXRPerformanceSettingsExtension::get_requested_extensions(XrVersion)
{
	HashMap<String, bool*> request_extensions;

	request_extensions[XR_EXT_PERFORMANCE_SETTINGS_EXTENSION_NAME] = &data->available;

	return request_extensions;
}

void OpenXRPerformanceSettingsExtension::on_instance_created(const XrInstance)
{
	if (data->available) {
		EXT_INIT_XR_FUNC(xrPerfSettingsSetPerformanceLevelEXT);
	}
}

bool OpenXRPerformanceSettingsExtension::on_event_polled(const XrEventDataBuffer& event)
{
	switch (event.type) {
	case XR_TYPE_EVENT_DATA_PERF_SETTINGS_EXT: {
		const XrEventDataPerfSettingsEXT* perf_settings_event =
			(const XrEventDataPerfSettingsEXT*)&event;

		OpenXRInterface::PerfSettingsSubDomain sub_domain =
			openxr_to_sub_domain(perf_settings_event->subDomain);
		OpenXRInterface::PerfSettingsNotificationLevel from_level =
			openxr_to_notification_level(perf_settings_event->fromLevel);
		OpenXRInterface::PerfSettingsNotificationLevel to_level =
			openxr_to_notification_level(perf_settings_event->toLevel);

		OpenXRInterface* openxr_interface = OpenXRAPI::get_xr_interface();

		if (perf_settings_event->domain == XR_PERF_SETTINGS_DOMAIN_CPU_EXT) {
			openxr_interface->on_cpu_level_changed(sub_domain, from_level, to_level);
		}
		else if (perf_settings_event->domain == XR_PERF_SETTINGS_DOMAIN_GPU_EXT) {
			openxr_interface->on_gpu_level_changed(sub_domain, from_level, to_level);
		}
		else {
			print_error(vformat(
				"OpenXR: no matching performance settings domain for %s", perf_settings_event->domain));
		}
		return true;
	} break;
	default:
		return false;
	}
}

bool OpenXRPerformanceSettingsExtension::is_available() { return data != nullptr && data->available; }

void OpenXRPerformanceSettingsExtension::set_cpu_level(OpenXRInterface::PerfSettingsLevel p_level)
{
	XrSession session = OpenXRAPI::get_session();
	XrPerfSettingsLevelEXT xr_level = level_to_openxr(p_level);

	XrResult result =
		xrPerfSettingsSetPerformanceLevelEXT(session, XR_PERF_SETTINGS_DOMAIN_CPU_EXT, xr_level);
	if (XR_FAILED(result)) {
		print_error(vformat("OpenXR: failed to set CPU performance level [%s]",
			OpenXRAPI::get_error_string(result)));
	}
}

void OpenXRPerformanceSettingsExtension::set_gpu_level(OpenXRInterface::PerfSettingsLevel p_level)
{
	XrSession session = OpenXRAPI::get_session();
	XrPerfSettingsLevelEXT xr_level = level_to_openxr(p_level);

	XrResult result =
		xrPerfSettingsSetPerformanceLevelEXT(session, XR_PERF_SETTINGS_DOMAIN_GPU_EXT, xr_level);
	if (XR_FAILED(result)) {
		print_error(vformat("OpenXR: failed to set GPU performance level [%s]",
			OpenXRAPI::get_error_string(result)));
	}
}

XrPerfSettingsLevelEXT OpenXRPerformanceSettingsExtension::level_to_openxr(
	OpenXRInterface::PerfSettingsLevel p_level)
{
	switch (p_level) {
	case OpenXRInterface::PerfSettingsLevel::PERF_SETTINGS_LEVEL_POWER_SAVINGS:
		return XR_PERF_SETTINGS_LEVEL_POWER_SAVINGS_EXT;
	case OpenXRInterface::PerfSettingsLevel::PERF_SETTINGS_LEVEL_SUSTAINED_LOW:
		return XR_PERF_SETTINGS_LEVEL_SUSTAINED_LOW_EXT;
	case OpenXRInterface::PerfSettingsLevel::PERF_SETTINGS_LEVEL_SUSTAINED_HIGH:
		return XR_PERF_SETTINGS_LEVEL_SUSTAINED_HIGH_EXT;
	case OpenXRInterface::PerfSettingsLevel::PERF_SETTINGS_LEVEL_BOOST:
		return XR_PERF_SETTINGS_LEVEL_BOOST_EXT;
	default:
		print_error("Invalid performance settings level.");
		return XR_PERF_SETTINGS_LEVEL_SUSTAINED_HIGH_EXT;
	}
}

OpenXRInterface::PerfSettingsSubDomain OpenXRPerformanceSettingsExtension::openxr_to_sub_domain(
	XrPerfSettingsSubDomainEXT p_sub_domain)
{
	switch (p_sub_domain) {
	case XR_PERF_SETTINGS_SUB_DOMAIN_COMPOSITING_EXT:
		return OpenXRInterface::PerfSettingsSubDomain::PERF_SETTINGS_SUB_DOMAIN_COMPOSITING;
	case XR_PERF_SETTINGS_SUB_DOMAIN_RENDERING_EXT:
		return OpenXRInterface::PerfSettingsSubDomain::PERF_SETTINGS_SUB_DOMAIN_RENDERING;
	case XR_PERF_SETTINGS_SUB_DOMAIN_THERMAL_EXT:
		return OpenXRInterface::PerfSettingsSubDomain::PERF_SETTINGS_SUB_DOMAIN_THERMAL;
	default:
		print_error("Invalid performance settings sub domain.");
		return OpenXRInterface::PerfSettingsSubDomain::PERF_SETTINGS_SUB_DOMAIN_COMPOSITING;
	}
}

OpenXRInterface::PerfSettingsNotificationLevel
OpenXRPerformanceSettingsExtension::openxr_to_notification_level(
	XrPerfSettingsNotificationLevelEXT p_notification_level)
{
	switch (p_notification_level) {
	case XR_PERF_SETTINGS_NOTIF_LEVEL_NORMAL_EXT:
		return OpenXRInterface::PerfSettingsNotificationLevel::PERF_SETTINGS_NOTIF_LEVEL_NORMAL;
	case XR_PERF_SETTINGS_NOTIF_LEVEL_WARNING_EXT:
		return OpenXRInterface::PerfSettingsNotificationLevel::PERF_SETTINGS_NOTIF_LEVEL_WARNING;
	case XR_PERF_SETTINGS_NOTIF_LEVEL_IMPAIRED_EXT:
		return OpenXRInterface::PerfSettingsNotificationLevel::PERF_SETTINGS_NOTIF_LEVEL_IMPAIRED;
	default:
		print_error("Invalid performance settings notification level.");
		return OpenXRInterface::PerfSettingsNotificationLevel::PERF_SETTINGS_NOTIF_LEVEL_NORMAL;
	}
}

////////////////////////////////////////////////////////////////////////////
// OpenXRDebugUtilsExtension

void OpenXRDebugUtilsExtension::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXRDebugUtilsExtension::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

HashMap<String, bool*> OpenXRDebugUtilsExtension::get_requested_extensions(XrVersion)
{
	HashMap<String, bool*> request_extensions;

	request_extensions[XR_EXT_DEBUG_UTILS_EXTENSION_NAME] = &data->debug_utils_ext;

	return request_extensions;
}

void OpenXRDebugUtilsExtension::on_instance_destroyed()
{
	if (data->default_messenger != XR_NULL_HANDLE) {
		XrResult result = xrDestroyDebugUtilsMessengerEXT(data->default_messenger);
		if (XR_FAILED(result)) {
			ERR_PRINT("OpenXR: Failed to destroy debug callback [" +
					  OpenXRAPI::get_error_string(result) + "]");
		}

		data->default_messenger = XR_NULL_HANDLE;
	}

	xrCreateDebugUtilsMessengerEXT_ptr = nullptr;
	xrDestroyDebugUtilsMessengerEXT_ptr = nullptr;
	xrSetDebugUtilsObjectNameEXT_ptr = nullptr;
	xrSessionBeginDebugUtilsLabelRegionEXT_ptr = nullptr;
	xrSessionEndDebugUtilsLabelRegionEXT_ptr = nullptr;
	xrSessionInsertDebugUtilsLabelEXT_ptr = nullptr;
	data->debug_utils_ext = false;
}

bool OpenXRDebugUtilsExtension::get_active() { return data != nullptr && data->debug_utils_ext; }

void OpenXRDebugUtilsExtension::set_object_name(
	XrObjectType p_object_type, uint64_t p_object_handle, const char* p_object_name)
{
	ERR_FAIL_COND(!data->debug_utils_ext);
	ERR_FAIL_NULL(xrSetDebugUtilsObjectNameEXT_ptr);

	const XrDebugUtilsObjectNameInfoEXT space_name_info = {
		XR_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
		nullptr,
		p_object_type,
		p_object_handle,
		p_object_name,
	};

	XrResult result = xrSetDebugUtilsObjectNameEXT_ptr(OpenXRAPI::get_instance(), &space_name_info);
	if (XR_FAILED(result)) {
		ERR_PRINT(
			"OpenXR: Failed to set object name [" + OpenXRAPI::get_error_string(result) + "]");
	}
}

void OpenXRDebugUtilsExtension::begin_debug_label_region(const char* p_label_name)
{
	ERR_FAIL_COND(!data->debug_utils_ext);
	ERR_FAIL_NULL(xrSessionBeginDebugUtilsLabelRegionEXT_ptr);

	const XrDebugUtilsLabelEXT session_active_region_label = {
		XR_TYPE_DEBUG_UTILS_LABEL_EXT,
		nullptr,
		p_label_name,
	};

	XrResult result = xrSessionBeginDebugUtilsLabelRegionEXT_ptr(
		OpenXRAPI::get_session(), &session_active_region_label);
	if (XR_FAILED(result)) {
		ERR_PRINT(
			"OpenXR: Failed to begin label region [" + OpenXRAPI::get_error_string(result) + "]");
	}
}

void OpenXRDebugUtilsExtension::end_debug_label_region()
{
	ERR_FAIL_COND(!data->debug_utils_ext);
	ERR_FAIL_NULL(xrSessionEndDebugUtilsLabelRegionEXT_ptr);

	XrResult result = xrSessionEndDebugUtilsLabelRegionEXT_ptr(OpenXRAPI::get_session());
	if (XR_FAILED(result)) {
		ERR_PRINT(
			"OpenXR: Failed to end label region [" + OpenXRAPI::get_error_string(result) + "]");
	}
}

void OpenXRDebugUtilsExtension::insert_debug_label(const char* p_label_name)
{
	ERR_FAIL_COND(!data->debug_utils_ext);
	ERR_FAIL_NULL(xrSessionInsertDebugUtilsLabelEXT_ptr);

	const XrDebugUtilsLabelEXT session_active_region_label = {
		XR_TYPE_DEBUG_UTILS_LABEL_EXT,
		nullptr,
		p_label_name,
	};

	XrResult result = xrSessionInsertDebugUtilsLabelEXT_ptr(
		OpenXRAPI::get_session(), &session_active_region_label);
	if (XR_FAILED(result)) {
		ERR_PRINT("OpenXR: Failed to insert label [" + OpenXRAPI::get_error_string(result) + "]");
	}
}

XrBool32 XRAPI_PTR OpenXRDebugUtilsExtension::_debug_callback(
	XrDebugUtilsMessageSeverityFlagsEXT p_message_severity,
	XrDebugUtilsMessageTypeFlagsEXT p_message_types,
	const XrDebugUtilsMessengerCallbackDataEXT* p_callback_data, void* p_user_data)
{
	if (OpenXRDebugUtilsExtension::is_initialized()) {
		return OpenXRDebugUtilsExtension::debug_callback(
			p_message_severity, p_message_types, p_callback_data, p_user_data);
	}

	return XR_FALSE;
}

XrBool32 OpenXRDebugUtilsExtension::debug_callback(
	XrDebugUtilsMessageSeverityFlagsEXT p_message_severity,
	XrDebugUtilsMessageTypeFlagsEXT p_message_types,
	const XrDebugUtilsMessengerCallbackDataEXT* p_callback_data, void*)
{
	String msg;

	ERR_FAIL_NULL_V(p_callback_data, XR_FALSE);

	if (p_message_types == XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
		msg = ", type: General";
	}
	else if (p_message_types == XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
		msg = ", type: Validation";
	}
	else if (p_message_types == XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
		msg = ", type: Performance";
	}
	else if (p_message_types == XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT) {
		msg = ", type: Conformance";
	}
	else {
		msg = ", type: Unknown (" + String::num_uint64(p_message_types) + ")";
	}

	if (p_callback_data->functionName) {
		msg += ", function Name: " + String(p_callback_data->functionName);
	}
	if (p_callback_data->messageId) {
		msg += "\nMessage ID: " + String(p_callback_data->messageId);
	}
	if (p_callback_data->message) {
		msg += "\nMessage: " + String(p_callback_data->message);
	}

	if (p_callback_data->objectCount > 0) {
		String objects;

		for (uint32_t i = 0; i < p_callback_data->objectCount; i++) {
			if (!objects.is_empty()) {
				objects += ", ";
			}
			objects += p_callback_data->objects[i].objectName;
		}

		msg += "\nObjects: " + objects;
	}

	if (p_callback_data->sessionLabelCount > 0) {
		String labels;

		for (uint32_t i = 0; i < p_callback_data->sessionLabelCount; i++) {
			if (!labels.is_empty()) {
				labels += ", ";
			}
			labels += p_callback_data->sessionLabels[i].labelName;
		}

		msg += "\nLabels: " + labels;
	}

	if (p_message_severity == XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
		ERR_PRINT("OpenXR: Severity: Error" + msg);
	}
	else if (p_message_severity == XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		WARN_PRINT("OpenXR: Severity: Warning" + msg);
	}
	else if (p_message_severity == XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
		print_verbose("OpenXR: Severity: Verbose" + msg);
	}

	return XR_FALSE;
}

void OpenXRDebugUtilsExtension::on_instance_created(const XrInstance) {}

////////////////////////////////////////////////////////////////////////////
// OpenXRVisibilityMaskExtension

static const char* VISIBILITY_MASK_SHADER_CODE =
	"shader_type spatial;\n"
	"render_mode unshaded, shadows_disabled, cull_disabled;\n"
	"void vertex() {\n"
	"\tif (int(VERTEX.z) == VIEW_INDEX) {\n"
	"\t\tVERTEX.z = -1.0;\n"
	"\t\tVERTEX += EYE_OFFSET;\n"
	"\t\tPOSITION = PROJECTION_MATRIX * vec4(VERTEX, 1.0);\n"
	"\t\tPOSITION.xy /= POSITION.w;\n"
	"\t\tPOSITION.z = 1.0;\n"
	"\t\tPOSITION.w = 1.0;\n"
	"\t} else {\n"
	"\t\tPOSITION = vec4(2.0, 2.0, 2.0, 1.0);\n"
	"\t}\n"
	"}\n"
	"void fragment() {\n"
	"\tALBEDO = vec3(0.0, 0.0, 0.0);\n"
	"}\n";

void OpenXRVisibilityMaskExtension::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXRVisibilityMaskExtension::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

HashMap<String, bool*> OpenXRVisibilityMaskExtension::get_requested_extensions(XrVersion)
{
	HashMap<String, bool*> request_extensions;

	request_extensions[XR_KHR_VISIBILITY_MASK_EXTENSION_NAME] = &data->available;

	return request_extensions;
}

void OpenXRVisibilityMaskExtension::on_instance_created(const XrInstance)
{
	if (data->available) {
		EXT_INIT_XR_FUNC(xrGetVisibilityMaskKHR);
	}
}

void OpenXRVisibilityMaskExtension::on_session_created(const XrSession)
{
	if (data->available) {
		ERR_FAIL_NULL(Renderer::data);
		ERR_FAIL_COND(!OpenXRAPI::is_allocated());

		data->shader = Renderer::shader_create();
		Renderer::shader_set_code(data->shader, VISIBILITY_MASK_SHADER_CODE);

		data->material = Renderer::material_create();
		Renderer::material_set_shader(data->material, data->shader);
		Renderer::material_set_render_priority(data->material, 99);

		data->mesh_count = OpenXRAPI::get_view_count();
		for (uint32_t i = 0; i < data->mesh_count; i++) {
			_update_mesh_data(i);
		}

		_update_mesh();
	}
}

void OpenXRVisibilityMaskExtension::on_session_destroyed()
{
	ERR_FAIL_NULL(Renderer::data);

	if (data->mesh.is_valid()) {
		Renderer::free_rid(data->mesh);
		data->mesh = RID();
	}

	if (data->material.is_valid()) {
		Renderer::free_rid(data->material);
		data->material = RID();
	}

	if (data->shader.is_valid()) {
		Renderer::free_rid(data->shader);
		data->shader = RID();
	}

	data->mesh_count = 0;
}

void OpenXRVisibilityMaskExtension::on_pre_render() { _update_mesh(); }

bool OpenXRVisibilityMaskExtension::on_event_polled(const XrEventDataBuffer& event)
{
	if (event.type == XR_TYPE_EVENT_DATA_VISIBILITY_MASK_CHANGED_KHR) {
		const XrEventDataVisibilityMaskChangedKHR* vismask_event =
			(const XrEventDataVisibilityMaskChangedKHR*)&event;

		print_verbose("OpenXR EVENT: Visibility mask changed for view " +
					  String::num_uint64(vismask_event->viewIndex));

		if (data->available) {
			_update_mesh_data(vismask_event->viewIndex);
		}

		return true;
	}

	return false;
}

bool OpenXRVisibilityMaskExtension::is_available() { return data != nullptr && data->available; }

RID OpenXRVisibilityMaskExtension::get_mesh() { return data ? data->mesh : RID(); }

void OpenXRVisibilityMaskExtension::_update_mesh_data(uint32_t p_view)
{
	if (data->available) {
		ERR_FAIL_UNSIGNED_INDEX(p_view, 4);
		ERR_FAIL_COND(!OpenXRAPI::is_allocated());

		XrSession session = OpenXRAPI::get_session();
		XrViewConfigurationType view_configuration_type = OpenXRAPI::get_view_configuration();

		XrVisibilityMaskKHR visibility_mask_data = {
			XR_TYPE_VISIBILITY_MASK_KHR,
			nullptr,
			0,
			0,
			nullptr,
			0,
			0,
			nullptr,
		};

		XrResult result = xrGetVisibilityMaskKHR(session, view_configuration_type, p_view,
			XR_VISIBILITY_MASK_TYPE_HIDDEN_TRIANGLE_MESH_KHR, &visibility_mask_data);
		if (XR_FAILED(result)) {
			return;
		}

		data->mesh_data[p_view].vertices.resize(visibility_mask_data.vertexCountOutput);
		data->mesh_data[p_view].indices.resize(visibility_mask_data.indexCountOutput);

		visibility_mask_data.vertexCapacityInput = visibility_mask_data.vertexCountOutput;
		visibility_mask_data.vertices = data->mesh_data[p_view].vertices.ptrw();
		visibility_mask_data.indexCapacityInput = visibility_mask_data.indexCountOutput;
		visibility_mask_data.indices = data->mesh_data[p_view].indices.ptrw();

		result = xrGetVisibilityMaskKHR(session, view_configuration_type, p_view,
			XR_VISIBILITY_MASK_TYPE_HIDDEN_TRIANGLE_MESH_KHR, &visibility_mask_data);
		if (XR_FAILED(result)) {
			return;
		}

		data->is_dirty = true;
	}
}

void OpenXRVisibilityMaskExtension::_update_mesh() {}


////////////////////////////////////////////////////////////////////////////
// OpenXRCompositionLayerExtension

void OpenXRCompositionLayerExtension::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXRCompositionLayerExtension::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

HashMap<String, bool*> OpenXRCompositionLayerExtension::get_requested_extensions(
	XrVersion p_version)
{
	HashMap<String, bool*> request_extensions;

	request_extensions[XR_KHR_COMPOSITION_LAYER_CYLINDER_EXTENSION_NAME] =
		&data->cylinder_ext_available;
	request_extensions[XR_KHR_COMPOSITION_LAYER_EQUIRECT2_EXTENSION_NAME] =
		&data->equirect_ext_available;

#ifdef ANDROID_ENABLED
	request_extensions[XR_KHR_ANDROID_SURFACE_SWAPCHAIN_EXTENSION_NAME] =
		&data->android_surface_ext_available;
#endif

	return request_extensions;
}

void OpenXRCompositionLayerExtension::on_instance_created(const XrInstance p_instance)
{
#ifdef ANDROID_ENABLED
	EXT_INIT_XR_FUNC(xrDestroySwapchain);
	if (data->android_surface_ext_available) {
		EXT_INIT_XR_FUNC(xrCreateSwapchainAndroidSurfaceKHR);
	}
#endif
}

void OpenXRCompositionLayerExtension::on_pre_render()
{
	for (CompositionLayer* composition_layer : data->registered_composition_layers) {
		composition_layer->on_pre_render();
	}
}

int OpenXRCompositionLayerExtension::get_composition_layer_count()
{
	return data->registered_composition_layers.size();
}

XrCompositionLayerBaseHeader* OpenXRCompositionLayerExtension::get_composition_layer(int p_index)
{
	ERR_FAIL_UNSIGNED_INDEX_V(
		(unsigned int)p_index, data->registered_composition_layers.size(), nullptr);
	return data->registered_composition_layers[p_index]->get_composition_layer();
}

int OpenXRCompositionLayerExtension::get_composition_layer_order(int p_index)
{
	ERR_FAIL_UNSIGNED_INDEX_V((unsigned int)p_index, data->registered_composition_layers.size(), 1);
	return data->registered_composition_layers[p_index]->sort_order;
}

RID OpenXRCompositionLayerExtension::composition_layer_create(
	XrCompositionLayerBaseHeader* p_openxr_layer)
{
	RID rid = data->composition_layer_owner.make_rid();
	CompositionLayer* layer = data->composition_layer_owner.get_or_null(rid);

	switch (p_openxr_layer->type) {
	case XR_TYPE_COMPOSITION_LAYER_QUAD: {
		layer->composition_layer_quad = *(XrCompositionLayerQuad*)p_openxr_layer;
	} break;
	case XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR: {
		layer->composition_layer_cylinder = *(XrCompositionLayerCylinderKHR*)p_openxr_layer;
	} break;
	case XR_TYPE_COMPOSITION_LAYER_EQUIRECT2_KHR: {
		layer->composition_layer_equirect = *(XrCompositionLayerEquirect2KHR*)p_openxr_layer;
	} break;
	default: {
		ERR_PRINT(vformat("Invalid OpenXR composition layer type: %s", p_openxr_layer->type));
		data->composition_layer_owner.free(rid);
		return RID();
	}
	}

	return rid;
}

Ref<JavaObject> OpenXRCompositionLayerExtension::composition_layer_get_android_surface(RID p_layer)
{
	MutexLock lock(data->composition_layer_mutex);
	CompositionLayer* layer = data->composition_layer_owner.get_or_null(p_layer);
	ERR_FAIL_NULL_V(layer, Ref<JavaObject>());
	return layer->get_android_surface();
}

void OpenXRCompositionLayerExtension::_composition_layer_register_rt(RID p_layer)
{
	CompositionLayer* layer = data->composition_layer_owner.get_or_null(p_layer);
	ERR_FAIL_NULL(layer);
	data->registered_composition_layers.push_back(layer);
}

void OpenXRCompositionLayerExtension::_composition_layer_unregister_rt(RID p_layer)
{
	CompositionLayer* layer = data->composition_layer_owner.get_or_null(p_layer);
	ERR_FAIL_NULL(layer);
	data->registered_composition_layers.erase(layer);
}

bool OpenXRCompositionLayerExtension::is_available(XrStructureType p_which)
{
	switch (p_which) {
	case XR_TYPE_COMPOSITION_LAYER_QUAD: {
		return true;
	} break;
	case XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR: {
		return data != nullptr && data->cylinder_ext_available;
	} break;
	case XR_TYPE_COMPOSITION_LAYER_EQUIRECT2_KHR: {
		return data != nullptr && data->equirect_ext_available;
	} break;
	default: {
		ERR_PRINT(vformat("Unsupported composition layer type: %s", p_which));
		return false;
	}
	}
}

#ifdef ANDROID_ENABLED
bool OpenXRCompositionLayerExtension::create_android_surface_swapchain(
	XrSwapchainCreateInfo* p_info, XrSwapchain* r_swapchain, jobject* r_surface)
{
	if (data->android_surface_ext_available) {
		ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), false);

		XrResult result = xrCreateSwapchainAndroidSurfaceKHR(
			OpenXRAPI::get_session(), p_info, r_swapchain, r_surface);
		if (XR_FAILED(result)) {
			print_line("OpenXR: Failed to create Android surface swapchain [",
				OpenXRAPI::get_error_string(result), "]");
			return false;
		}

		return true;
	}

	return false;
}
#endif

void OpenXRCompositionLayerExtension::CompositionLayer::set_viewport(
	RID p_viewport, const Size2i& p_size)
{
	ERR_FAIL_COND(use_android_surface);

	if (subviewport.viewport != p_viewport) {
		if (subviewport.viewport.is_valid()) {
			RID rt = Renderer::viewport_get_render_target(subviewport.viewport);
			RendererTextureStorage::render_target_set_override(rt, RID(), RID(), RID(), RID());
		}

		subviewport.viewport = p_viewport;

		if (subviewport.viewport.is_valid()) {
			subviewport.viewport_size = p_size;
		}
		else {
			free_swapchain();
			subviewport.viewport_size = Size2i();
		}
	}
	else if (subviewport.viewport_size != p_size) {
		subviewport.viewport_size = p_size;
	}
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_use_android_surface(
	bool p_use_android_surface, const Size2i& p_size)
{
#ifdef ANDROID_ENABLED
	if (p_use_android_surface == use_android_surface) {
		if (use_android_surface && swapchain_size != p_size) {
			if (OpenXRFBUpdateSwapchainExtension::is_initialized() &&
				OpenXRFBUpdateSwapchainExtension::is_android_ext_enabled()) {
				swapchain_size = p_size;
				OpenXRFBUpdateSwapchainExtension::update_swapchain_surface_size(
					android_surface.swapchain, swapchain_size);
			}
		}
		return;
	}

	use_android_surface = p_use_android_surface;

	if (use_android_surface) {
		if (!OpenXRCompositionLayerExtension::is_android_surface_swapchain_available()) {
			ERR_PRINT_ONCE("OpenXR: Cannot use Android surface for composition layer because the "
						   "extension isn't available");
		}

		if (subviewport.viewport.is_valid()) {
			set_viewport(RID(), Size2i());
		}

		swapchain_size = p_size;
	}
	else {
		free_swapchain();
	}
#endif
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_alpha_blend(bool p_alpha_blend)
{
	if (alpha_blend != p_alpha_blend) {
		alpha_blend = p_alpha_blend;
		if (alpha_blend) {
			composition_layer.layerFlags |= XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
		}
		else {
			composition_layer.layerFlags &= ~XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
		}
	}
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_transform(
	const Transform3D& p_transform)
{
	Transform3D xf;

	if (pose_space == POSE_HEAD_LOCKED) {
		xf = p_transform;
	}
	else {
		Transform3D reference_frame = XRServer::get_reference_frame();
		xf = reference_frame.inverse() * p_transform;
	}

	Quaternion quat(xf.basis.orthonormalized());

	if (Math::is_zero_approx(quat.length())) {
		quat = Quaternion();
	}

	XrPosef pose = {{(float)quat.x, (float)quat.y, (float)quat.z, (float)quat.w},
		{(float)xf.origin.x, (float)xf.origin.y, (float)xf.origin.z}};

	switch (composition_layer.type) {
	case XR_TYPE_COMPOSITION_LAYER_QUAD: {
		composition_layer_quad.pose = pose;
	} break;
	case XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR: {
		composition_layer_cylinder.pose = pose;
	} break;
	case XR_TYPE_COMPOSITION_LAYER_EQUIRECT2_KHR: {
		composition_layer_equirect.pose = pose;
	} break;
	default: {
		ERR_PRINT(vformat("Cannot set transform on unsupported composition layer type: %s",
			composition_layer.type));
	}
	}
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_min_filter(Filter p_mode)
{
	swapchain_state.min_filter = p_mode;
	swapchain_state_is_dirty = true;
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_mag_filter(Filter p_mode)
{
	swapchain_state.mag_filter = p_mode;
	swapchain_state_is_dirty = true;
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_mipmap_mode(MipmapMode p_mode)
{
	swapchain_state.mipmap_mode = p_mode;
	swapchain_state_is_dirty = true;
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_horizontal_wrap(Wrap p_mode)
{
	swapchain_state.horizontal_wrap = p_mode;
	swapchain_state_is_dirty = true;
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_vertical_wrap(Wrap p_mode)
{
	swapchain_state.vertical_wrap = p_mode;
	swapchain_state_is_dirty = true;
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_red_swizzle(Swizzle p_mode)
{
	swapchain_state.red_swizzle = p_mode;
	swapchain_state_is_dirty = true;
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_green_swizzle(Swizzle p_mode)
{
	swapchain_state.green_swizzle = p_mode;
	swapchain_state_is_dirty = true;
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_blue_swizzle(Swizzle p_mode)
{
	swapchain_state.blue_swizzle = p_mode;
	swapchain_state_is_dirty = true;
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_alpha_swizzle(Swizzle p_mode)
{
	swapchain_state.alpha_swizzle = p_mode;
	swapchain_state_is_dirty = true;
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_max_anisotropy(float p_value)
{
	swapchain_state.max_anisotropy = p_value;
	swapchain_state_is_dirty = true;
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_border_color(const Color& p_color)
{
	swapchain_state.border_color = p_color;
	swapchain_state_is_dirty = true;
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_pose_space(PoseSpace p_pose_space)
{
	pose_space = p_pose_space;
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_eye_visibility(
	EyeVisibility p_eye_visibility)
{
	XrEyeVisibility eye_visibility;

	switch (p_eye_visibility) {
	case EYE_VISIBILITY_BOTH: {
		eye_visibility = XR_EYE_VISIBILITY_BOTH;
	} break;
	case EYE_VISIBILITY_LEFT: {
		eye_visibility = XR_EYE_VISIBILITY_LEFT;
	} break;
	case EYE_VISIBILITY_RIGHT: {
		eye_visibility = XR_EYE_VISIBILITY_RIGHT;
	} break;
	default: {
		eye_visibility = XR_EYE_VISIBILITY_BOTH;
	}
	}

	switch (composition_layer.type) {
	case XR_TYPE_COMPOSITION_LAYER_QUAD: {
		composition_layer_quad.eyeVisibility = eye_visibility;
	} break;
	case XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR: {
		composition_layer_cylinder.eyeVisibility = eye_visibility;
	} break;
	case XR_TYPE_COMPOSITION_LAYER_EQUIRECT2_KHR: {
		composition_layer_equirect.eyeVisibility = eye_visibility;
	} break;
	default: {
		ERR_PRINT(vformat("%s does not support setting eye visibility.", composition_layer.type));
	}
	}
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_quad_size(const Size2& p_size)
{
	ERR_FAIL_COND(composition_layer.type != XR_TYPE_COMPOSITION_LAYER_QUAD);
	composition_layer_quad.size = {(float)p_size.x, (float)p_size.y};
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_cylinder_radius(float p_radius)
{
	ERR_FAIL_COND(composition_layer.type != XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR);
	composition_layer_cylinder.radius = p_radius;
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_cylinder_aspect_ratio(
	float p_aspect_ratio)
{
	ERR_FAIL_COND(composition_layer.type != XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR);
	composition_layer_cylinder.aspectRatio = p_aspect_ratio;
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_cylinder_central_angle(
	float p_central_angle)
{
	ERR_FAIL_COND(composition_layer.type != XR_TYPE_COMPOSITION_LAYER_CYLINDER_KHR);
	composition_layer_cylinder.centralAngle = p_central_angle;
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_equirect_radius(float p_radius)
{
	ERR_FAIL_COND(composition_layer.type != XR_TYPE_COMPOSITION_LAYER_EQUIRECT2_KHR);
	composition_layer_equirect.radius = p_radius;
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_equirect_central_horizontal_angle(
	float p_angle)
{
	ERR_FAIL_COND(composition_layer.type != XR_TYPE_COMPOSITION_LAYER_EQUIRECT2_KHR);
	composition_layer_equirect.centralHorizontalAngle = p_angle;
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_equirect_upper_vertical_angle(
	float p_angle)
{
	ERR_FAIL_COND(composition_layer.type != XR_TYPE_COMPOSITION_LAYER_EQUIRECT2_KHR);
	composition_layer_equirect.upperVerticalAngle = p_angle;
}

void OpenXRCompositionLayerExtension::CompositionLayer::set_equirect_lower_vertical_angle(
	float p_angle)
{
	ERR_FAIL_COND(composition_layer.type != XR_TYPE_COMPOSITION_LAYER_EQUIRECT2_KHR);
	composition_layer_equirect.lowerVerticalAngle = p_angle;
}

Ref<JavaObject> OpenXRCompositionLayerExtension::CompositionLayer::get_android_surface()
{
#ifdef ANDROID_ENABLED
	if (use_android_surface) {
		MutexLock lock(OpenXRCompositionLayerExtension::data->composition_layer_mutex);
		if (android_surface.surface.is_null()) {
			create_android_surface();
		}
		return android_surface.surface;
	}
#endif
	return Ref<JavaObject>();
}

void OpenXRCompositionLayerExtension::CompositionLayer::on_pre_render()
{
#ifdef ANDROID_ENABLED
	if (use_android_surface) {
		MutexLock lock(OpenXRCompositionLayerExtension::data->composition_layer_mutex);
		if (android_surface.surface.is_null()) {
			create_android_surface();
		}
		return;
	}
#endif

	if (subviewport.viewport.is_valid() && OpenXRAPI::is_allocated() && OpenXRAPI::is_running()) {
		RSE::ViewportUpdateMode update_mode =
			Renderer::viewport_get_update_mode(subviewport.viewport);
		if (update_mode == RSE::VIEWPORT_UPDATE_ONCE ||
			update_mode == RSE::VIEWPORT_UPDATE_ALWAYS) {
			if (update_and_acquire_swapchain(update_mode == RSE::VIEWPORT_UPDATE_ONCE)) {
				RID rt = Renderer::viewport_get_render_target(subviewport.viewport);
				RendererTextureStorage::render_target_set_override(
					rt, get_current_swapchain_texture(), RID(), RID(), RID());
			}
		}
	}

	if (swapchain_state_is_dirty) {
		update_swapchain_state();
		swapchain_state_is_dirty = false;
	}
}

void OpenXRCompositionLayerExtension::CompositionLayer::free()
{
	if (use_android_surface) {
		free_swapchain();
	}
	else {
		set_viewport(RID(), Size2i());
	}
}

void OpenXRCompositionLayerExtension::CompositionLayer::update_swapchain_state()
{
	if (!OpenXRFBUpdateSwapchainExtension::is_initialized()) {
		return;
	}

#ifdef ANDROID_ENABLED
	if (use_android_surface) {
		if (android_surface.swapchain == XR_NULL_HANDLE) {
			return;
		}

		OpenXRFBUpdateSwapchainExtension::update_swapchain_state(
			android_surface.swapchain, &swapchain_state);
	}
	else
#endif
	{
		if (subviewport.swapchain_info.get_swapchain() == XR_NULL_HANDLE) {
			return;
		}

		OpenXRFBUpdateSwapchainExtension::update_swapchain_state(
			subviewport.swapchain_info.get_swapchain(), &swapchain_state);
	}
}

void OpenXRCompositionLayerExtension::CompositionLayer::update_swapchain_sub_image(
	XrSwapchainSubImage& r_subimage)
{
#ifdef ANDROID_ENABLED
	if (use_android_surface) {
		r_subimage.swapchain = android_surface.swapchain;
	}
	else
#endif
	{
		XrSwapchain swapchain = subviewport.swapchain_info.get_swapchain();

		if (swapchain && subviewport.swapchain_info.is_image_acquired()) {
			subviewport.swapchain_info.release();
		}

		r_subimage.swapchain = swapchain;
	}

	r_subimage.imageRect.extent.width = swapchain_size.width;
	r_subimage.imageRect.extent.height = swapchain_size.height;
}

bool OpenXRCompositionLayerExtension::CompositionLayer::update_and_acquire_swapchain(
	bool p_static_image)
{
	ERR_FAIL_COND_V(use_android_surface, false);

	if (!OpenXRAPI::is_allocated() || !OpenXRCompositionLayerExtension::is_initialized()) {
		return false;
	}
	if (!OpenXRCompositionLayerExtension::is_available(composition_layer.type)) {
		return false;
	}

	if (subviewport.swapchain_info.get_swapchain() != XR_NULL_HANDLE) {
		if (swapchain_size == subviewport.viewport_size && !p_static_image &&
			!subviewport.static_image &&
			protected_content == subviewport.swapchain_protected_content) {
			bool should_render = true;
			return subviewport.swapchain_info.acquire(should_render);
		}

		subviewport.swapchain_info.queue_free();
	}

	int64_t swapchain_format = OpenXRAPI::get_color_swapchain_format();
	const uint32_t sample_count = 1;
	const uint32_t array_size = 1;
	XrSwapchainCreateFlags create_flags = 0;
	if (p_static_image) {
		create_flags |= XR_SWAPCHAIN_CREATE_STATIC_IMAGE_BIT;
	}
	if (protected_content) {
		create_flags |= XR_SWAPCHAIN_CREATE_PROTECTED_CONTENT_BIT;
	}
	if (!subviewport.swapchain_info.create(create_flags,
			XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT |
				XR_SWAPCHAIN_USAGE_MUTABLE_FORMAT_BIT,
			swapchain_format, subviewport.viewport_size.width, subviewport.viewport_size.height,
			sample_count, array_size)) {
		swapchain_size = Size2i();
		return false;
	}

	swapchain_state_is_dirty = true;

	bool should_render = true;
	bool ret = subviewport.swapchain_info.acquire(should_render);

	swapchain_size = subviewport.viewport_size;
	subviewport.static_image = p_static_image;
	subviewport.swapchain_protected_content = protected_content;
	return ret;
}

RID OpenXRCompositionLayerExtension::CompositionLayer::get_current_swapchain_texture()
{
	ERR_FAIL_COND_V(use_android_surface, RID());

	if (!OpenXRAPI::is_allocated()) {
		return RID();
	}

	return subviewport.swapchain_info.get_image();
}

void OpenXRCompositionLayerExtension::CompositionLayer::free_swapchain()
{
#ifdef ANDROID_ENABLED
	if (use_android_surface) {
		if (android_surface.swapchain != XR_NULL_HANDLE) {
			OpenXRCompositionLayerExtension::xrDestroySwapchain(android_surface.swapchain);
			android_surface.swapchain = XR_NULL_HANDLE;
			android_surface.surface.unref();
		}
	}
	else
#endif
	{
		if (subviewport.swapchain_info.get_swapchain() != XR_NULL_HANDLE) {
			subviewport.swapchain_info.queue_free();
		}
		subviewport.static_image = false;
	}

	swapchain_size = Size2i();
}

#ifdef ANDROID_ENABLED
void OpenXRCompositionLayerExtension::CompositionLayer::create_android_surface()
{
	ERR_FAIL_COND(
		android_surface.swapchain != XR_NULL_HANDLE || android_surface.surface.is_valid());

	XrSwapchainCreateFlags create_flags = 0;
	if (protected_content) {
		create_flags = XR_SWAPCHAIN_CREATE_PROTECTED_CONTENT_BIT;
	}

	XrSwapchainCreateInfo info = {
		XR_TYPE_SWAPCHAIN_CREATE_INFO,
		nullptr,
		create_flags,
		XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT |
			XR_SWAPCHAIN_USAGE_MUTABLE_FORMAT_BIT,
		0,
		0,
		(uint32_t)swapchain_size.x,
		(uint32_t)swapchain_size.y,
		0,
		0,
		0,
	};

	jobject surface = nullptr;
	OpenXRCompositionLayerExtension::create_android_surface_swapchain(
		&info, &android_surface.swapchain, &surface);

	swapchain_state_is_dirty = true;

	if (surface) {
		android_surface.surface.instantiate(
			JavaClassWrapper::wrap("android.view.Surface"), surface);
	}
}
#endif

void OpenXRCompositionLayerExtension::composition_layer_set_viewport(
	RID p_layer, RID p_viewport, const Vector2i& p_size)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_use_android_surface(
	RID p_layer, bool p_enable, const Vector2i& p_size)
{
}

void OpenXRCompositionLayerExtension::composition_layer_unregister(RID p_layer) {}

void OpenXRCompositionLayerExtension::composition_layer_set_vertical_wrap(
	RID p_layer, OpenXRCompositionLayerExtension::Wrap p_wrap)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_red_swizzle(
	RID p_layer, OpenXRCompositionLayerExtension::Swizzle p_swizzle)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_quad_size(
	RID p_layer, const Vector2& p_size)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_protected_content(
	RID p_layer, bool p_protected)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_mipmap_mode(
	RID p_layer, OpenXRCompositionLayerExtension::MipmapMode p_mode)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_min_filter(
	RID p_layer, OpenXRCompositionLayerExtension::Filter p_filter)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_max_anisotropy(
	RID p_layer, float p_anisotropy)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_mag_filter(
	RID p_layer, OpenXRCompositionLayerExtension::Filter p_filter)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_horizontal_wrap(
	RID p_layer, OpenXRCompositionLayerExtension::Wrap p_wrap)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_green_swizzle(
	RID p_layer, OpenXRCompositionLayerExtension::Swizzle p_swizzle)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_eye_visibility(
	RID p_layer, OpenXRCompositionLayerExtension::EyeVisibility p_visibility)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_equirect_upper_vertical_angle(
	RID p_layer, float p_angle)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_equirect_radius(
	RID p_layer, float p_radius)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_equirect_lower_vertical_angle(
	RID p_layer, float p_angle)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_equirect_central_horizontal_angle(
	RID p_layer, float p_angle)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_cylinder_radius(
	RID p_layer, float p_radius)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_cylinder_central_angle(
	RID p_layer, float p_angle)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_cylinder_aspect_ratio(
	RID p_layer, float p_ratio)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_border_color(
	RID p_layer, const Color& p_color)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_blue_swizzle(
	RID p_layer, OpenXRCompositionLayerExtension::Swizzle p_swizzle)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_alpha_swizzle(
	RID p_layer, OpenXRCompositionLayerExtension::Swizzle p_swizzle)
{
}

void OpenXRCompositionLayerExtension::composition_layer_set_alpha_blend(RID p_layer, bool p_blend)
{
}

void OpenXRCompositionLayerExtension::composition_layer_register(RID p_layer) {}

XrCompositionLayerBaseHeader*
OpenXRCompositionLayerExtension::CompositionLayer::get_composition_layer()
{
	return &composition_layer;
}

void OpenXRCompositionLayerExtension::on_session_created(XrSession_T* p_session) {}

void OpenXRCompositionLayerExtension::on_session_destroyed() {}

////////////////////////////////////////////////////////////////////////////
// OpenXRFrameSynthesisExtension

void OpenXRFrameSynthesisExtension::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXRFrameSynthesisExtension::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

void OpenXRFrameSynthesisExtension::prepare_view_configuration(uint32_t p_view_count)
{
	if (!data->frame_synthesis_ext) {
		return;
	}

	data->render_state.config_views.resize(p_view_count);

	for (XrFrameSynthesisConfigViewEXT& config_view : data->render_state.config_views) {
		config_view.type = XR_TYPE_FRAME_SYNTHESIS_CONFIG_VIEW_EXT;
		config_view.next = nullptr;
		config_view.recommendedMotionVectorImageRectWidth = 0;
		config_view.recommendedMotionVectorImageRectHeight = 0;
	}
}

void* OpenXRFrameSynthesisExtension::set_view_configuration_and_get_next_pointer(
	uint32_t p_view, void* p_next_pointer)
{
	if (!data->frame_synthesis_ext) {
		return nullptr;
	}

	ERR_FAIL_UNSIGNED_INDEX_V(p_view, data->render_state.config_views.size(), nullptr);

	XrFrameSynthesisConfigViewEXT& config_view = data->render_state.config_views[p_view];
	config_view.next = p_next_pointer;

	return &config_view;
}

void OpenXRFrameSynthesisExtension::on_session_destroyed()
{
	if (!data->frame_synthesis_ext) {
		return;
	}

	free_swapchains();
}

void OpenXRFrameSynthesisExtension::on_pre_draw_viewport(RID p_render_target)
{
	if (!data->frame_synthesis_ext) {
		return;
	}

	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	if (!data->enabled || data->render_state.config_views.size() != 2 ||
		data->render_state.frame_synthesis_info.size() != 2 || data->render_state.skip_next_frame) {
		OpenXRAPI::set_velocity_texture(RID());
		OpenXRAPI::set_velocity_depth_texture(RID());
		data->render_state.previous_transform = XRServer::get_world_origin();
		return;
	}

	for (int i = 0; i < SWAPCHAIN_MAX; i++) {
		bool should_render = true;
		data->render_state.swapchains[i].acquire(should_render);
	}

	OpenXRAPI::set_velocity_texture(
		data->render_state.swapchains[SWAPCHAIN_MOTION_VECTOR].get_image());
	OpenXRAPI::set_velocity_depth_texture(
		data->render_state.swapchains[SWAPCHAIN_DEPTH].get_image());

	uint32_t width = data->render_state.config_views[0].recommendedMotionVectorImageRectWidth;
	uint32_t height = data->render_state.config_views[0].recommendedMotionVectorImageRectHeight;
	OpenXRAPI::set_velocity_target_size(Size2i(width, height));

	Transform3D world_transform = XRServer::get_world_origin();
	Transform3D delta_transform =
		data->render_state.previous_transform.affine_inverse() * world_transform;
	Quaternion delta_quat = delta_transform.basis.get_quaternion();
	Vector3 delta_origin = delta_transform.origin;

	float z_far = OpenXRAPI::get_render_state_z_far();
	float z_near = OpenXRAPI::get_render_state_z_near();

	for (XrFrameSynthesisInfoEXT& frame_synthesis_info : data->render_state.frame_synthesis_info) {
		frame_synthesis_info.layerFlags =
			data->render_state.relax_frame_interval
				? XR_FRAME_SYNTHESIS_INFO_REQUEST_RELAXED_FRAME_INTERVAL_BIT_EXT
				: 0;

		frame_synthesis_info.appSpaceDeltaPose = {
			{(float)delta_quat.x, (float)delta_quat.y, (float)delta_quat.z, (float)delta_quat.w},
			{(float)delta_origin.x, (float)delta_origin.y, (float)delta_origin.z}};

		if (z_far != z_near) {
			frame_synthesis_info.nearZ = z_far;
			frame_synthesis_info.farZ = z_near;
		}
	}

	data->render_state.previous_transform = world_transform;
}

void OpenXRFrameSynthesisExtension::on_post_draw_viewport(RID p_render_target)
{
	if (!data->frame_synthesis_ext || !data->enabled ||
		data->render_state.config_views.size() != 2 ||
		data->render_state.frame_synthesis_info.size() != 2 || data->render_state.skip_next_frame) {
		return;
	}

	for (int i = 0; i < SWAPCHAIN_MAX; i++) {
		data->render_state.swapchains[i].release();
	}
}

void* OpenXRFrameSynthesisExtension::set_projection_views_and_get_next_pointer(
	int p_view_index, void* p_next_pointer)
{
	if (!data->frame_synthesis_ext || !data->enabled ||
		data->render_state.config_views.size() != 2 ||
		data->render_state.frame_synthesis_info.size() != 2) {
		return nullptr;
	}

	if (data->render_state.skip_next_frame) {
		if (p_view_index == 1) {
			data->render_state.skip_next_frame = false;
		}
		return nullptr;
	}

	data->render_state.frame_synthesis_info[p_view_index].next = p_next_pointer;
	return &data->render_state.frame_synthesis_info[p_view_index];
}

bool OpenXRFrameSynthesisExtension::is_available()
{
	return data != nullptr && data->frame_synthesis_ext;
}

bool OpenXRFrameSynthesisExtension::is_enabled()
{
	return data != nullptr && data->frame_synthesis_ext && data->enabled;
}

bool OpenXRFrameSynthesisExtension::get_relax_frame_interval()
{
	return data != nullptr && data->relax_frame_interval;
}

void OpenXRFrameSynthesisExtension::_set_render_state_enabled_rt(bool p_enabled)
{
	data->render_state.enabled = p_enabled;
}

void OpenXRFrameSynthesisExtension::_set_relax_frame_interval_rt(bool p_relax_frame_interval)
{
	data->render_state.relax_frame_interval = p_relax_frame_interval;
}

void OpenXRFrameSynthesisExtension::free_swapchains()
{
	for (int i = 0; i < SWAPCHAIN_MAX; i++) {
		data->render_state.swapchains[i].queue_free();
	}
}

void OpenXRFrameSynthesisExtension::_set_skip_next_frame_rt()
{
	data->render_state.skip_next_frame = true;
}

HashMap<String, bool*> OpenXRFrameSynthesisExtension::get_requested_extensions(
	XrVersion p_version)
{
	return HashMap<String, bool*>();
}

void OpenXRFrameSynthesisExtension::on_instance_created(XrInstance_T* p_instance) {}

void OpenXRFrameSynthesisExtension::on_instance_destroyed() {}

void OpenXRFrameSynthesisExtension::print_view_configuration_info(unsigned int p_view_config) {}

void OpenXRFrameSynthesisExtension::on_main_swapchains_created() {}

////////////////////////////////////////////////////////////////////////////
// OpenXRFutureResult & OpenXRFutureExtension

void OpenXRFutureExtension::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXRFutureExtension::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

OpenXRFutureResult::ResultStatus OpenXRFutureResult::get_status() const { return status; }

XrFutureEXT OpenXRFutureResult::get_future() const { return future; }

uint64_t OpenXRFutureResult::_get_future() const { return (uint64_t)future; }

void OpenXRFutureResult::cancel_future()
{
	ERR_FAIL_COND(status != RESULT_RUNNING);
	ERR_FAIL_COND(!OpenXRFutureExtension::is_initialized());

	OpenXRFutureExtension::cancel_future(future);
}

void OpenXRFutureResult::_mark_as_cancelled() {}

void OpenXRFutureResult::_mark_as_finished() {}

HashMap<String, bool*> OpenXRFutureExtension::get_requested_extensions(XrVersion p_version)
{
	HashMap<String, bool*> request_extensions;

	request_extensions[XR_EXT_FUTURE_EXTENSION_NAME] = &data->future_ext;

	return request_extensions;
}

void OpenXRFutureExtension::on_instance_created(const XrInstance p_instance)
{
	if (data->future_ext) {
		EXT_INIT_XR_FUNC(xrPollFutureEXT);
		EXT_INIT_XR_FUNC(xrCancelFutureEXT);

		data->future_ext = xrPollFutureEXT_ptr && xrCancelFutureEXT_ptr;
	}
}

void OpenXRFutureExtension::on_instance_destroyed()
{
	xrPollFutureEXT_ptr = nullptr;
	xrCancelFutureEXT_ptr = nullptr;
}

void OpenXRFutureExtension::on_session_destroyed()
{
	if (!is_active()) {
		return;
	}

	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	for (const KeyValue<XrFutureEXT, Ref<OpenXRFutureResult>>& element : data->futures) {
		XrFutureCancelInfoEXT cancel_info = {
			XR_TYPE_FUTURE_CANCEL_INFO_EXT,
			nullptr,
			element.key,
		};
		XrResult result = xrCancelFutureEXT_ptr(OpenXRAPI::get_instance(), &cancel_info);
		if (XR_FAILED(result)) {
			WARN_PRINT(
				"OpenXR: Failed to cancel future [" + OpenXRAPI::get_error_string(result) + "]");
		}

		element.value->_mark_as_cancelled();
	}
	data->futures.clear();
}

bool OpenXRFutureExtension::is_active() { return data != nullptr && data->future_ext; }

void OpenXRFutureExtension::cancel_future(XrFutureEXT p_future)
{
	ERR_FAIL_COND(!data->futures.has(p_future));
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	XrFutureCancelInfoEXT cancel_info = {
		XR_TYPE_FUTURE_CANCEL_INFO_EXT,
		nullptr,
		p_future,
	};
	XrResult result = xrCancelFutureEXT_ptr(OpenXRAPI::get_instance(), &cancel_info);
	if (XR_FAILED(result)) {
		WARN_PRINT(
			"OpenXR: Failed to cancel future [" + OpenXRAPI::get_error_string(result) + "]");
	}

	data->futures[p_future]->_mark_as_cancelled();
	data->futures.erase(p_future);
}

void OpenXRFutureExtension::_cancel_future(uint64_t p_future)
{
	cancel_future((XrFutureEXT)p_future);
}

void OpenXRFutureExtension::on_process()
{
	if (!is_active()) {
		return;
	}

	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	Vector<XrFutureEXT> completed;
	for (const KeyValue<XrFutureEXT, Ref<OpenXRFutureResult>>& element : data->futures) {
		XrFuturePollInfoEXT poll_info = {
			XR_TYPE_FUTURE_POLL_INFO_EXT,
			nullptr,
			element.key,
		};
		XrFuturePollResultEXT poll_result = {
			XR_TYPE_FUTURE_POLL_RESULT_EXT,
			nullptr,
			XR_FUTURE_STATE_MAX_ENUM_EXT,
		};
		XrResult result = xrPollFutureEXT_ptr(OpenXRAPI::get_instance(), &poll_info, &poll_result);
		if (XR_FAILED(result)) {
			ERR_PRINT("OpenXR: Failed to obtain future status [" +
					  OpenXRAPI::get_error_string(result) + "]");
			continue;
		}
		if (poll_result.state == XR_FUTURE_STATE_READY_EXT) {
			element.value->_mark_as_finished();
			completed.push_back(element.key);
		}
	}

	for (const XrFutureEXT& future : completed) {
		data->futures.erase(future);
	}
}

////////////////////////////////////////////////////////////////////////////
// OpenXRHandTrackingExtension

void OpenXRHandTrackingExtension::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXRHandTrackingExtension::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

void OpenXRHandTrackingExtension::on_session_destroyed() { cleanup_hand_tracking(); }

void OpenXRHandTrackingExtension::on_state_stopping() { cleanup_hand_tracking(); }

const OpenXRHandTrackingExtension::HandTracker* OpenXRHandTrackingExtension::get_hand_tracker(
	HandTrackedHands p_hand)
{
	ERR_FAIL_UNSIGNED_INDEX_V(p_hand, OPENXR_MAX_TRACKED_HANDS, nullptr);

	return &data->hand_trackers[p_hand];
}

bool OpenXRHandTrackingExtension::get_active() { return true; }

void OpenXRHandTrackingExtension::cleanup_hand_tracking() {}

void OpenXRHandTrackingExtension::on_instance_destroyed() {}

void* OpenXRHandTrackingExtension::set_system_properties_and_get_next_pointer(void* p_next_pointer)
{
	return p_next_pointer;
}

void OpenXRHandTrackingExtension::on_state_ready() {}

void OpenXRHandTrackingExtension::on_process() {}

////////////////////////////////////////////////////////////////////////////
// OpenXRRenderModelExtension

#ifdef MODULE_GLTF_ENABLED
#include "modules/gltf/gltf_document.h"
#include "scene/3d/node_3d.h"

OpenXRRenderModelData::OpenXRRenderModelData() = default;
OpenXRRenderModelData::~OpenXRRenderModelData() = default;

Ref<GLTFState> OpenXRRenderModelData::get_gltf_state() { return gltf_state; }

void OpenXRRenderModelExtension::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXRRenderModelExtension::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

void OpenXRRenderModelExtension::on_instance_created(const XrInstance p_instance)
{
	EXT_INIT_XR_FUNC(xrLocateSpace);
	EXT_INIT_XR_FUNC(xrDestroySpace);
	EXT_INIT_XR_FUNC(xrPathToString);

	if (data->render_model_ext) {
		EXT_INIT_XR_FUNC(xrCreateRenderModelEXT);
		EXT_INIT_XR_FUNC(xrDestroyRenderModelEXT);
		EXT_INIT_XR_FUNC(xrGetRenderModelPropertiesEXT);
		EXT_INIT_XR_FUNC(xrCreateRenderModelSpaceEXT);
		EXT_INIT_XR_FUNC(xrCreateRenderModelAssetEXT);
		EXT_INIT_XR_FUNC(xrDestroyRenderModelAssetEXT);
		EXT_INIT_XR_FUNC(xrGetRenderModelAssetDataEXT);
		EXT_INIT_XR_FUNC(xrGetRenderModelAssetPropertiesEXT);
		EXT_INIT_XR_FUNC(xrGetRenderModelStateEXT);
	}

	if (data->interaction_render_model_ext) {
		EXT_INIT_XR_FUNC(xrEnumerateInteractionRenderModelIdsEXT);
		EXT_INIT_XR_FUNC(xrEnumerateRenderModelSubactionPathsEXT);
		EXT_INIT_XR_FUNC(xrGetRenderModelPoseTopLevelUserPathEXT);
	}
}

void OpenXRRenderModelExtension::on_session_created(const XrSession p_session)
{
	data->_interaction_data_dirty = true;
}

void OpenXRRenderModelExtension::on_instance_destroyed()
{
	xrCreateRenderModelEXT_ptr = nullptr;
	xrDestroyRenderModelEXT_ptr = nullptr;
	xrGetRenderModelPropertiesEXT_ptr = nullptr;
	xrCreateRenderModelSpaceEXT_ptr = nullptr;
	xrCreateRenderModelAssetEXT_ptr = nullptr;
	xrDestroyRenderModelAssetEXT_ptr = nullptr;
	xrGetRenderModelAssetDataEXT_ptr = nullptr;
	xrGetRenderModelAssetPropertiesEXT_ptr = nullptr;
	xrGetRenderModelStateEXT_ptr = nullptr;
	xrEnumerateInteractionRenderModelIdsEXT_ptr = nullptr;
	xrEnumerateRenderModelSubactionPathsEXT_ptr = nullptr;
	xrGetRenderModelPoseTopLevelUserPathEXT_ptr = nullptr;

	data->uuid_ext = false;
	data->render_model_ext = false;
	data->interaction_render_model_ext = false;
}

void OpenXRRenderModelExtension::on_session_destroyed()
{
	_clear_interaction_data();
	_clear_render_model_data();
	data->xr_sync_has_run = false;
}

bool OpenXRRenderModelExtension::on_event_polled(const XrEventDataBuffer& event)
{
	if (event.type == XR_TYPE_EVENT_DATA_INTERACTION_RENDER_MODELS_CHANGED_EXT) {
		data->_interaction_data_dirty = true;
		return true;
	}
	else if (event.type == XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED) {
		data->_interaction_data_dirty = true;
		return false;
	}

	return false;
}

void OpenXRRenderModelExtension::on_sync_actions()
{
	if (!is_active()) {
		return;
	}

	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	data->xr_sync_has_run = true;

	if (data->_interaction_data_dirty) {
		_update_interaction_data();
	}

	LocalVector<RID> owned = data->render_model_owner.get_owned_list();

	for (const RID& rid : owned) {
		RenderModel* render_model = data->render_model_owner.get_or_null(rid);
		if (render_model && render_model->xr_space != XR_NULL_HANDLE) {
			XrSpaceLocation render_model_location = {
				XR_TYPE_SPACE_LOCATION,
				nullptr,
				0,
				{{0.0, 0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}},
			};

			XrResult result = xrLocateSpace(render_model->xr_space, OpenXRAPI::get_play_space(),
				OpenXRAPI::get_predicted_display_time(), &render_model_location);
			ERR_CONTINUE_MSG(XR_FAILED(result), "OpenXR: Failed to locate render model space [" +
													OpenXRAPI::get_error_string(result) + "]");

			render_model->confidence = OpenXRAPI::transform_from_location(
				render_model_location, render_model->root_transform);

			if (!render_model->node_states.is_empty()) {
				XrRenderModelStateGetInfoEXT get_state_info = {
					XR_TYPE_RENDER_MODEL_STATE_GET_INFO_EXT,
					nullptr,
					OpenXRAPI::get_predicted_display_time(),
				};

				XrRenderModelStateEXT state = {
					XR_TYPE_RENDER_MODEL_STATE_EXT,
					nullptr,
					render_model->animatable_node_count,
					render_model->node_states.ptr(),
				};

				result = xrGetRenderModelStateEXT(
					render_model->xr_render_model, &get_state_info, &state);
				if (XR_FAILED(result)) {
					ERR_PRINT("OpenXR: Failed to update node states [" +
							  OpenXRAPI::get_error_string(result) + "]");
				}
			}

			XrPath new_path = XR_NULL_PATH;

			if (data->toplevel_paths.is_empty()) {
				data->toplevel_paths.push_back(OpenXRAPI::get_xr_path("/user/hand/left"));
				data->toplevel_paths.push_back(OpenXRAPI::get_xr_path("/user/hand/right"));
			}

			XrInteractionRenderModelTopLevelUserPathGetInfoEXT info = {
				XR_TYPE_INTERACTION_RENDER_MODEL_TOP_LEVEL_USER_PATH_GET_INFO_EXT,
				nullptr,
				(uint32_t)data->toplevel_paths.size(),
				data->toplevel_paths.ptr(),
			};
			result = xrGetRenderModelPoseTopLevelUserPathEXT(
				render_model->xr_render_model, &info, &new_path);
			if (XR_FAILED(result)) {
				ERR_PRINT("OpenXR: Failed to update the top level path for render models [" +
						  OpenXRAPI::get_error_string(result) + "]");
			}
			else if (new_path != render_model->top_level_path) {
				print_verbose("OpenXR: Render model top level path changed to " +
							  OpenXRAPI::get_xr_path_name(new_path));
				render_model->top_level_path = new_path;
			}
		}
	}
}

bool OpenXRRenderModelExtension::is_active()
{
	return data != nullptr && data->render_model_ext && data->interaction_render_model_ext;
}

void OpenXRRenderModelExtension::_clear_interaction_data()
{
	for (const KeyValue<XrRenderModelIdEXT, RID>& e : data->interaction_render_models) {
		render_model_destroy(e.value);
	}
	data->interaction_render_models.clear();
}

bool OpenXRRenderModelExtension::_update_interaction_data()
{
	ERR_FAIL_COND_V_MSG(!data->interaction_render_model_ext, false,
		"Interaction render model extension hasn't been enabled.");

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), false);

	XrSession session = OpenXRAPI::get_session();
	ERR_FAIL_COND_V(session == XR_NULL_HANDLE, false);

	if (!data->xr_sync_has_run) {
		return true;
	}

	data->_interaction_data_dirty = false;

	XrInteractionRenderModelIdsEnumerateInfoEXT interaction_info = {
		XR_TYPE_INTERACTION_RENDER_MODEL_IDS_ENUMERATE_INFO_EXT,
		nullptr,
	};

	uint32_t interaction_count = 0;
	XrResult result = xrEnumerateInteractionRenderModelIdsEXT(
		session, &interaction_info, 0, &interaction_count, nullptr);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(false, "OpenXR: Failed to obtain render model interaction id count [" +
								  OpenXRAPI::get_error_string(result) + "]");
	}

	LocalVector<XrRenderModelIdEXT> render_model_interaction_ids;
	render_model_interaction_ids.resize(interaction_count);

	if (!render_model_interaction_ids.is_empty()) {
		result = xrEnumerateInteractionRenderModelIdsEXT(session, &interaction_info,
			render_model_interaction_ids.size(), &interaction_count,
			render_model_interaction_ids.ptr());
		if (XR_FAILED(result)) {
			ERR_FAIL_V_MSG(false, "OpenXR: Failed to obtain render model interaction ids [" +
									  OpenXRAPI::get_error_string(result) + "]");
		}
	}

	LocalVector<XrRenderModelIdEXT> erase_ids;
	for (const KeyValue<XrRenderModelIdEXT, RID>& e : data->interaction_render_models) {
		if (!render_model_interaction_ids.has(e.key)) {
			if (e.value.is_valid()) {
				render_model_destroy(e.value);
			}
			erase_ids.push_back(e.key);
		}
	}

	for (const XrRenderModelIdEXT& id : erase_ids) {
		data->interaction_render_models.erase(id);
	}

	for (const XrRenderModelIdEXT& id : render_model_interaction_ids) {
		if (!data->interaction_render_models.has(id)) {
			data->interaction_render_models[id] = render_model_create(id);
		}
	}

	return true;
}

bool OpenXRRenderModelExtension::has_render_model(RID p_render_model)
{
	return data->render_model_owner.owns(p_render_model);
}

RID OpenXRRenderModelExtension::render_model_create(XrRenderModelIdEXT p_render_model_id)
{
	ERR_FAIL_COND_V_MSG(
		!data->render_model_ext, RID(), "Render model extension hasn't been enabled.");

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), RID());

	XrSession session = OpenXRAPI::get_session();
	ERR_FAIL_COND_V(session == XR_NULL_HANDLE, RID());

	RenderModel render_model;
	render_model.xr_render_model_id = p_render_model_id;

	const HashSet<String> supported_gltf_extensions_hash_set =
		GLTFDocument::get_supported_gltf_extensions_hashset();
	Vector<CharString> supported_gltf_extensions_char_string;
	supported_gltf_extensions_char_string.resize(supported_gltf_extensions_hash_set.size());
	int64_t supported_gltf_extension_index = 0;
	for (const String& ext : supported_gltf_extensions_hash_set) {
		supported_gltf_extensions_char_string.set(supported_gltf_extension_index, ext.utf8());
		supported_gltf_extension_index++;
	}
	Vector<const char*> supported_gltf_extensions;
	supported_gltf_extensions.resize(supported_gltf_extensions_char_string.size());
	for (int64_t i = 0; i < supported_gltf_extensions_char_string.size(); i++) {
		supported_gltf_extensions.write[i] = supported_gltf_extensions_char_string[i].get_data();
	}

	XrRenderModelCreateInfoEXT create_info = {
		XR_TYPE_RENDER_MODEL_CREATE_INFO_EXT,
		nullptr,
		p_render_model_id,
		uint32_t(supported_gltf_extensions.size()),
		supported_gltf_extensions.ptr(),
	};

	XrResult result = xrCreateRenderModelEXT(session, &create_info, &render_model.xr_render_model);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(RID(), "OpenXR: Failed to create render model [" +
								  OpenXRAPI::get_error_string(result) + "]");
	}

	XrRenderModelPropertiesGetInfoEXT properties_info = {
		XR_TYPE_RENDER_MODEL_PROPERTIES_GET_INFO_EXT,
		nullptr,
	};

	XrRenderModelPropertiesEXT properties = {
		XR_TYPE_RENDER_MODEL_PROPERTIES_EXT,
		nullptr,
		{},
		0,
	};

	result =
		xrGetRenderModelPropertiesEXT(render_model.xr_render_model, &properties_info, &properties);
	if (XR_FAILED(result)) {
		ERR_PRINT("OpenXR: Failed to get render model properties [" +
				  OpenXRAPI::get_error_string(result) + "]");
	}
	else {
		render_model.animatable_node_count = properties.animatableNodeCount;
		render_model.render_model_data =
			_get_render_model_data(properties.cacheId, properties.animatableNodeCount);
	}

	XrRenderModelSpaceCreateInfoEXT space_create_info = {
		XR_TYPE_RENDER_MODEL_SPACE_CREATE_INFO_EXT,
		nullptr,
		render_model.xr_render_model,
	};

	result = xrCreateRenderModelSpaceEXT(session, &space_create_info, &render_model.xr_space);
	if (XR_FAILED(result)) {
		ERR_PRINT("OpenXR: Failed to create render model space [" +
				  OpenXRAPI::get_error_string(result) + "]");
	}

	if (render_model.animatable_node_count > 0) {
		render_model.node_states.resize(render_model.animatable_node_count);
	}

	return data->render_model_owner.make_rid(render_model);
}

RID OpenXRRenderModelExtension::_render_model_create(uint64_t p_render_model_id)
{
	RID ret;

	ERR_FAIL_COND_V(p_render_model_id == XR_NULL_RENDER_MODEL_ID_EXT, ret);

	if (is_active()) {
		ret = render_model_create(XrRenderModelIdEXT(p_render_model_id));
	}

	return ret;
}

void OpenXRRenderModelExtension::render_model_destroy(RID p_render_model)
{
	ERR_FAIL_COND_MSG(!data->render_model_ext, "Render model extension hasn't been enabled.");

	RenderModel* render_model = data->render_model_owner.get_or_null(p_render_model);
	ERR_FAIL_NULL(render_model);

	if (render_model->xr_space != XR_NULL_HANDLE) {
		xrDestroySpace(render_model->xr_space);
	}

	render_model->node_states.clear();

	XrResult result = xrDestroyRenderModelEXT(render_model->xr_render_model);
	if (XR_FAILED(result)) {
		ERR_PRINT("OpenXR: Failed to destroy render model [" +
				  OpenXRAPI::get_error_string(result) + "]");
	}

	data->render_model_owner.free(p_render_model);
}

Node3D* OpenXRRenderModelExtension::render_model_new_scene_instance(RID p_render_model)
{
	RenderModel* render_model = data->render_model_owner.get_or_null(p_render_model);
	ERR_FAIL_NULL_V(render_model, nullptr);

	if (render_model->render_model_data.is_null()) {
		return nullptr;
	}

	return render_model->render_model_data->new_scene_instance();
}

PackedStringArray OpenXRRenderModelExtension::render_model_get_subaction_paths(RID p_render_model)
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), PackedStringArray());

	XrInstance instance = OpenXRAPI::get_instance();
	ERR_FAIL_COND_V(instance == XR_NULL_HANDLE, PackedStringArray());

	RenderModel* render_model = data->render_model_owner.get_or_null(p_render_model);
	ERR_FAIL_NULL_V(render_model, PackedStringArray());

	PackedStringArray subaction_paths;

	XrInteractionRenderModelSubactionPathInfoEXT subaction_info = {
		XR_TYPE_INTERACTION_RENDER_MODEL_SUBACTION_PATH_INFO_EXT,
		nullptr,
	};

	uint32_t capacity;

	XrResult result = xrEnumerateRenderModelSubactionPathsEXT(
		render_model->xr_render_model, &subaction_info, 0, &capacity, nullptr);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(
			PackedStringArray(), "OpenXR: Failed to obtain render model subaction path count [" +
									 OpenXRAPI::get_error_string(result) + "]");
	}

	if (capacity > 0) {
		LocalVector<XrPath> paths;
		paths.resize(capacity);

		result = xrEnumerateRenderModelSubactionPathsEXT(
			render_model->xr_render_model, &subaction_info, capacity, &capacity, paths.ptr());
		if (XR_FAILED(result)) {
			ERR_FAIL_V_MSG(PackedStringArray(),
				"OpenXR: Failed to obtain render model subaction paths [" +
					OpenXRAPI::get_error_string(result) + "]");
		}

		for (uint32_t i = 0; i < capacity; i++) {
			char buffer[1024];
			uint32_t size = 0;
			xrPathToString(instance, paths[i], 1024, &size, buffer);
			if (size > 0) {
				subaction_paths.push_back(String(buffer));
			}
		}
	}

	return subaction_paths;
}

XrPath OpenXRRenderModelExtension::render_model_get_top_level_path(RID p_render_model)
{
	RenderModel* render_model = data->render_model_owner.get_or_null(p_render_model);
	ERR_FAIL_NULL_V(render_model, XRPose::TrackingConfidence::XR_TRACKING_CONFIDENCE_NONE);

	return render_model->top_level_path;
}

String OpenXRRenderModelExtension::render_model_get_top_level_path_as_string(RID p_render_model)
{
	String ret;

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), ret);

	if (is_active() && has_render_model(p_render_model)) {
		XrPath path = render_model_get_top_level_path(p_render_model);
		if (path == XR_NULL_PATH) {
			return "None";
		}
		else {
			return OpenXRAPI::get_xr_path_name(path);
		}
	}

	return ret;
}

XRPose::TrackingConfidence OpenXRRenderModelExtension::render_model_get_confidence(
	RID p_render_model)
{
	RenderModel* render_model = data->render_model_owner.get_or_null(p_render_model);
	ERR_FAIL_NULL_V(render_model, XRPose::TrackingConfidence::XR_TRACKING_CONFIDENCE_NONE);

	return render_model->confidence;
}

Transform3D OpenXRRenderModelExtension::render_model_get_root_transform(RID p_render_model)
{
	ERR_FAIL_COND_V(!XRServer::is_initialized(), Transform3D());

	RenderModel* render_model = data->render_model_owner.get_or_null(p_render_model);
	ERR_FAIL_NULL_V(render_model, Transform3D());

	real_t world_scale = XRServer::get_world_scale();
	Transform3D root_transform =
		render_model->root_transform.scaled(Vector3(world_scale, world_scale, world_scale));

	return XRServer::get_reference_frame() * root_transform;
}

uint32_t OpenXRRenderModelExtension::render_model_get_animatable_node_count(RID p_render_model)
{
	RenderModel* render_model = data->render_model_owner.get_or_null(p_render_model);
	ERR_FAIL_NULL_V(render_model, 0);

	return render_model->animatable_node_count;
}

String OpenXRRenderModelExtension::render_model_get_animatable_node_name(
	RID p_render_model, uint32_t p_index)
{
	RenderModel* render_model = data->render_model_owner.get_or_null(p_render_model);
	ERR_FAIL_NULL_V(render_model, String());

	if (render_model->render_model_data.is_null()) {
		return String();
	}

	return render_model->render_model_data->get_node_name(p_index);
}

bool OpenXRRenderModelExtension::render_model_is_animatable_node_visible(
	RID p_render_model, uint32_t p_index)
{
	RenderModel* render_model = data->render_model_owner.get_or_null(p_render_model);
	ERR_FAIL_NULL_V(render_model, false);
	ERR_FAIL_UNSIGNED_INDEX_V(p_index, render_model->animatable_node_count, false);

	if (render_model->node_states.is_empty()) {
		return false;
	}

	return render_model->node_states[p_index].isVisible;
}

Transform3D OpenXRRenderModelExtension::render_model_get_animatable_node_transform(
	RID p_render_model, uint32_t p_index)
{
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), Transform3D());

	RenderModel* render_model = data->render_model_owner.get_or_null(p_render_model);
	ERR_FAIL_NULL_V(render_model, Transform3D());
	ERR_FAIL_UNSIGNED_INDEX_V(p_index, render_model->animatable_node_count, Transform3D());

	if (render_model->node_states.is_empty()) {
		return Transform3D();
	}

	return OpenXRAPI::transform_from_pose(render_model->node_states[p_index].nodePose);
}

Ref<OpenXRRenderModelData> OpenXRRenderModelExtension::_get_render_model_data(
	XrUuidEXT p_cache_id, uint32_t p_animatable_node_count)
{
	if (data->render_model_data_cache.has(p_cache_id)) {
		return data->render_model_data_cache[p_cache_id];
	}

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), nullptr);

	XrSession session = OpenXRAPI::get_session();
	ERR_FAIL_COND_V(session == XR_NULL_HANDLE, nullptr);

	XrRenderModelAssetEXT asset;
	XrRenderModelAssetCreateInfoEXT create_info = {
		XR_TYPE_RENDER_MODEL_ASSET_CREATE_INFO_EXT,
		nullptr,
		p_cache_id,
	};

	XrResult result = xrCreateRenderModelAssetEXT(session, &create_info, &asset);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(nullptr, "OpenXR: Failed to create render model asset [" +
									OpenXRAPI::get_error_string(result) + "]");
	}

	Ref<OpenXRRenderModelData> render_model_data = _load_asset(asset, p_animatable_node_count);

	result = xrDestroyRenderModelAssetEXT(asset);
	if (XR_FAILED(result)) {
		ERR_PRINT("OpenXR: Failed to destroy render model asset [" +
				  OpenXRAPI::get_error_string(result) + "]");
	}

	data->render_model_data_cache[p_cache_id] = render_model_data;

	return render_model_data;
}

Ref<OpenXRRenderModelData> OpenXRRenderModelExtension::_load_asset(
	XrRenderModelAssetEXT p_asset, uint32_t p_animatable_node_count)
{
	XrRenderModelAssetDataGetInfoEXT get_info = {
		XR_TYPE_RENDER_MODEL_ASSET_DATA_GET_INFO_EXT,
		nullptr,
	};

	XrRenderModelAssetDataEXT asset_data = {
		XR_TYPE_RENDER_MODEL_ASSET_DATA_EXT,
		nullptr,
		0,
		0,
		nullptr,
	};

	XrResult result = xrGetRenderModelAssetDataEXT(p_asset, &get_info, &asset_data);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(nullptr, "OpenXR: Failed to get render model buffer size [" +
									OpenXRAPI::get_error_string(result) + "]");
	}
	ERR_FAIL_COND_V(asset_data.bufferCountOutput == 0, nullptr);

	PackedByteArray buffer;
	buffer.resize(asset_data.bufferCountOutput);
	asset_data.buffer = buffer.ptrw();
	asset_data.bufferCapacityInput = asset_data.bufferCountOutput;

	result = xrGetRenderModelAssetDataEXT(p_asset, &get_info, &asset_data);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(nullptr, "OpenXR: Failed to get render model buffer [" +
									OpenXRAPI::get_error_string(result) + "]");
	}

	PackedStringArray node_names;

	if (p_animatable_node_count > 0) {
		Vector<XrRenderModelAssetNodePropertiesEXT> node_properties;
		node_properties.resize(p_animatable_node_count);

		XrRenderModelAssetPropertiesGetInfoEXT properties_info = {
			XR_TYPE_RENDER_MODEL_ASSET_PROPERTIES_GET_INFO_EXT,
			nullptr,
		};

		XrRenderModelAssetPropertiesEXT asset_properties = {
			XR_TYPE_RENDER_MODEL_ASSET_PROPERTIES_EXT,
			nullptr,
			uint32_t(node_properties.size()),
			node_properties.ptrw(),
		};

		result = xrGetRenderModelAssetPropertiesEXT(p_asset, &properties_info, &asset_properties);
		if (XR_FAILED(result)) {
			ERR_FAIL_V_MSG(nullptr, "OpenXR: Failed to get render model property info [" +
										OpenXRAPI::get_error_string(result) + "]");
		}

		node_names.resize(p_animatable_node_count);
		String* node_names_ptrw = node_names.ptrw();
		for (uint32_t i = 0; i < p_animatable_node_count; i++) {
			node_names_ptrw[i] = String(node_properties[i].uniqueName);
		}
	}

	Ref<OpenXRRenderModelData> render_model_data;
	render_model_data.instantiate();

	render_model_data->parse_gltf_document(buffer);
	render_model_data->set_node_names(node_names);

	return render_model_data;
}

void OpenXRRenderModelExtension::_clear_render_model_data()
{
	data->toplevel_paths.clear();
	data->render_model_data_cache.clear();

	LocalVector<RID> owned = data->render_model_owner.get_owned_list();
	for (const RID& rid : owned) {
		render_model_destroy(rid);
	}
}

bool OpenXRRenderModelData::parse_gltf_document(const PackedByteArray& p_bytes)
{
	Ref<GLTFState> new_state;
	new_state.instantiate();
	Ref<GLTFDocument> new_gltf_document;
	new_gltf_document.instantiate();

	Error err = new_gltf_document->append_from_buffer(p_bytes, "", new_state);
	if (err != OK) {
		ERR_FAIL_V_MSG(false, "OpenXR: Failed to parse GLTF data.");
	}

	gltf_document = new_gltf_document;
	gltf_state = new_state;
	return true;
}

void OpenXRRenderModelData::set_node_names(const PackedStringArray& p_node_names)
{
	node_names = p_node_names;
}

PackedStringArray OpenXRRenderModelData::get_node_names() const { return node_names; }

const String OpenXRRenderModelData::get_node_name(uint32_t p_node_index) const
{
	ERR_FAIL_UNSIGNED_INDEX_V(p_node_index, node_names.size(), String());

	return node_names[p_node_index];
}

Node3D* OpenXRRenderModelData::new_scene_instance()
{
	return nullptr;
}

HashMap<String, bool*> OpenXRRenderModelExtension::get_requested_extensions(XrVersion p_version)
{
	return HashMap<String, bool*>();
}
#endif // MODULE_GLTF_ENABLED

////////////////////////////////////////////////////////////////////////////
// Platform-header dependent extensions (included last to isolate X11/driver macros)

#define Window OpenXRX11Window
#include "modules/openxr/openxr_platform_inc.h"
#undef Window

////////////////////////////////////////////////////////////////////////////
// OpenXRFBUpdateSwapchainExtension

#ifndef GL_CUBIC_IMG
#define GL_CUBIC_IMG 0x9139
#endif
#ifndef GL_CUBIC_MIPMAP_LINEAR_IMG
#define GL_CUBIC_MIPMAP_LINEAR_IMG 0x913B
#endif
#ifndef GL_CUBIC_MIPMAP_NEAREST_IMG
#define GL_CUBIC_MIPMAP_NEAREST_IMG 0x913A
#endif
#ifndef GL_CLAMP_TO_BORDER
#define GL_CLAMP_TO_BORDER 0x812D
#endif

void OpenXRFBUpdateSwapchainExtension::initialize(const String& p_rendering_driver)
{
	if (!data) {
		data = memnew(Data);
	}
	data->rendering_driver = p_rendering_driver;
}

void OpenXRFBUpdateSwapchainExtension::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

HashMap<String, bool*> OpenXRFBUpdateSwapchainExtension::get_requested_extensions(
	XrVersion p_version)
{
	HashMap<String, bool*> request_extensions;

	request_extensions[XR_FB_SWAPCHAIN_UPDATE_STATE_EXTENSION_NAME] =
		&data->fb_swapchain_update_state_ext;

	if (data->rendering_driver == "vulkan") {
#ifdef XR_USE_GRAPHICS_API_VULKAN
		request_extensions[XR_FB_SWAPCHAIN_UPDATE_STATE_VULKAN_EXTENSION_NAME] =
			&data->fb_swapchain_update_state_vulkan_ext;
#endif
	}
	else if (data->rendering_driver == "opengl3" || data->rendering_driver == "opengl3_es") {
#ifdef XR_USE_GRAPHICS_API_OPENGL_ES
		request_extensions[XR_FB_SWAPCHAIN_UPDATE_STATE_OPENGL_ES_EXTENSION_NAME] =
			&data->fb_swapchain_update_state_opengles_ext;
#endif
	}

#ifdef ANDROID_ENABLED
	request_extensions[XR_FB_SWAPCHAIN_UPDATE_STATE_ANDROID_SURFACE_EXTENSION_NAME] =
		&data->fb_swapchain_update_state_android_ext;
#endif

	return request_extensions;
}

void OpenXRFBUpdateSwapchainExtension::on_instance_created(const XrInstance p_instance)
{
	if (data->fb_swapchain_update_state_ext) {
		EXT_INIT_XR_FUNC(xrUpdateSwapchainFB);
		EXT_INIT_XR_FUNC(xrGetSwapchainStateFB);
	}
}

void OpenXRFBUpdateSwapchainExtension::on_instance_destroyed()
{
	data->fb_swapchain_update_state_ext = false;
	data->fb_swapchain_update_state_vulkan_ext = false;
	data->fb_swapchain_update_state_opengles_ext = false;
}

bool OpenXRFBUpdateSwapchainExtension::is_enabled()
{
	return data != nullptr && data->fb_swapchain_update_state_ext;
}

bool OpenXRFBUpdateSwapchainExtension::is_android_ext_enabled()
{
	return data != nullptr && data->fb_swapchain_update_state_android_ext;
}

void OpenXRFBUpdateSwapchainExtension::update_swapchain_state(XrSwapchain p_swapchain,
	const OpenXRCompositionLayerExtension::SwapchainState* p_swapchain_state)
{
	if (!p_swapchain_state) {
		return;
	}

	if (data->rendering_driver == "vulkan") {
#ifdef XR_USE_GRAPHICS_API_VULKAN
		if (!data->fb_swapchain_update_state_ext || !data->fb_swapchain_update_state_vulkan_ext) {
			return;
		}

		Color border_color = p_swapchain_state->border_color;
		XrSwapchainStateSamplerVulkanFB swapchain_state = {
			XR_TYPE_SWAPCHAIN_STATE_SAMPLER_VULKAN_FB,
			nullptr,
			(VkFilter)filter_to_vk(p_swapchain_state->min_filter),
			(VkFilter)filter_to_vk(p_swapchain_state->mag_filter),
			(VkSamplerMipmapMode)mipmap_mode_to_vk(p_swapchain_state->mipmap_mode),
			(VkSamplerAddressMode)wrap_to_vk(p_swapchain_state->horizontal_wrap),
			(VkSamplerAddressMode)wrap_to_vk(p_swapchain_state->vertical_wrap),
			(VkComponentSwizzle)swizzle_to_vk(p_swapchain_state->red_swizzle),
			(VkComponentSwizzle)swizzle_to_vk(p_swapchain_state->green_swizzle),
			(VkComponentSwizzle)swizzle_to_vk(p_swapchain_state->blue_swizzle),
			(VkComponentSwizzle)swizzle_to_vk(p_swapchain_state->alpha_swizzle),
			p_swapchain_state->max_anisotropy,
			{border_color.r, border_color.g, border_color.b, border_color.a},
		};

		XrResult result =
			xrUpdateSwapchainFB(p_swapchain, (XrSwapchainStateBaseHeaderFB*)&swapchain_state);
		if (XR_FAILED(result)) {
			print_error(vformat(
				"OpenXR: Failed to update swapchain [%s]", OpenXRAPI::get_error_string(result)));
			return;
		}
#endif
	}
	else if (data->rendering_driver == "opengl3" || data->rendering_driver == "opengl3_es") {
#ifdef XR_USE_GRAPHICS_API_OPENGL_ES
		if (!data->fb_swapchain_update_state_ext ||
			!data->fb_swapchain_update_state_opengles_ext) {
			return;
		}

		Color border_color = p_swapchain_state->border_color;
		XrSwapchainStateSamplerOpenGLESFB swapchain_state = {
			XR_TYPE_SWAPCHAIN_STATE_SAMPLER_OPENGL_ES_FB,
			nullptr,
			filter_to_gl(p_swapchain_state->min_filter, p_swapchain_state->mipmap_mode),
			filter_to_gl(p_swapchain_state->mag_filter),
			wrap_to_gl(p_swapchain_state->horizontal_wrap),
			wrap_to_gl(p_swapchain_state->vertical_wrap),
			swizzle_to_gl(p_swapchain_state->red_swizzle),
			swizzle_to_gl(p_swapchain_state->green_swizzle),
			swizzle_to_gl(p_swapchain_state->blue_swizzle),
			swizzle_to_gl(p_swapchain_state->alpha_swizzle),
			p_swapchain_state->max_anisotropy,
			{border_color.r, border_color.g, border_color.b, border_color.a},
		};

		XrResult result =
			xrUpdateSwapchainFB(p_swapchain, (XrSwapchainStateBaseHeaderFB*)&swapchain_state);
		if (XR_FAILED(result)) {
			print_error(vformat(
				"OpenXR: Failed to update swapchain [%s]", OpenXRAPI::get_error_string(result)));
			return;
		}
#endif
	}
}

void OpenXRFBUpdateSwapchainExtension::update_swapchain_surface_size(
	XrSwapchain p_swapchain, const Size2i& p_size)
{
#ifdef ANDROID_ENABLED
	if (!data->fb_swapchain_update_state_ext || !data->fb_swapchain_update_state_android_ext) {
		return;
	}

	XrSwapchainStateAndroidSurfaceDimensionsFB swapchain_state = {
		XR_TYPE_SWAPCHAIN_STATE_ANDROID_SURFACE_DIMENSIONS_FB,
		nullptr,
		(uint32_t)p_size.width,
		(uint32_t)p_size.height,
	};

	XrResult result =
		xrUpdateSwapchainFB(p_swapchain, (XrSwapchainStateBaseHeaderFB*)&swapchain_state);
	if (XR_FAILED(result)) {
		print_error(vformat("OpenXR: Failed to update swapchain surface size [%s]",
			OpenXRAPI::get_error_string(result)));
	}
#endif
}

uint32_t OpenXRFBUpdateSwapchainExtension::filter_to_gl(
	OpenXRCompositionLayerExtension::Filter p_filter,
	OpenXRCompositionLayerExtension::MipmapMode p_mipmap_mode)
{
#ifdef XR_USE_GRAPHICS_API_OPENGL_ES
	switch (p_mipmap_mode) {
	case OpenXRCompositionLayerExtension::MipmapMode::MIPMAP_MODE_DISABLED:
		switch (p_filter) {
		case OpenXRCompositionLayerExtension::Filter::FILTER_NEAREST:
			return GL_NEAREST;
		case OpenXRCompositionLayerExtension::Filter::FILTER_LINEAR:
			return GL_LINEAR;
		case OpenXRCompositionLayerExtension::Filter::FILTER_CUBIC:
			return GL_CUBIC_IMG;
		}
	case OpenXRCompositionLayerExtension::MipmapMode::MIPMAP_MODE_NEAREST:
		switch (p_filter) {
		case OpenXRCompositionLayerExtension::Filter::FILTER_NEAREST:
			return GL_NEAREST_MIPMAP_NEAREST;
		case OpenXRCompositionLayerExtension::Filter::FILTER_LINEAR:
			return GL_LINEAR_MIPMAP_NEAREST;
		case OpenXRCompositionLayerExtension::Filter::FILTER_CUBIC:
			return GL_CUBIC_MIPMAP_NEAREST_IMG;
		}
	case OpenXRCompositionLayerExtension::MipmapMode::MIPMAP_MODE_LINEAR:
		switch (p_filter) {
		case OpenXRCompositionLayerExtension::Filter::FILTER_NEAREST:
			return GL_NEAREST_MIPMAP_LINEAR;
		case OpenXRCompositionLayerExtension::Filter::FILTER_LINEAR:
			return GL_LINEAR_MIPMAP_LINEAR;
		case OpenXRCompositionLayerExtension::Filter::FILTER_CUBIC:
			return GL_CUBIC_MIPMAP_LINEAR_IMG;
		}
	}
#endif
	return 0;
}

uint32_t OpenXRFBUpdateSwapchainExtension::wrap_to_gl(OpenXRCompositionLayerExtension::Wrap p_wrap)
{
#ifdef XR_USE_GRAPHICS_API_OPENGL_ES
	switch (p_wrap) {
	case OpenXRCompositionLayerExtension::Wrap::WRAP_CLAMP_TO_BORDER:
		return GL_CLAMP_TO_BORDER;
	case OpenXRCompositionLayerExtension::Wrap::WRAP_CLAMP_TO_EDGE:
		return GL_CLAMP_TO_EDGE;
	case OpenXRCompositionLayerExtension::Wrap::WRAP_REPEAT:
		return GL_REPEAT;
	case OpenXRCompositionLayerExtension::Wrap::WRAP_MIRRORED_REPEAT:
		return GL_MIRRORED_REPEAT;
	case OpenXRCompositionLayerExtension::Wrap::WRAP_MIRROR_CLAMP_TO_EDGE:
		return GL_CLAMP_TO_EDGE;
	}
#endif
	return 0;
}

uint32_t OpenXRFBUpdateSwapchainExtension::swizzle_to_gl(
	OpenXRCompositionLayerExtension::Swizzle p_swizzle)
{
#ifdef XR_USE_GRAPHICS_API_OPENGL_ES
	switch (p_swizzle) {
	case OpenXRCompositionLayerExtension::Swizzle::SWIZZLE_RED:
		return GL_RED;
	case OpenXRCompositionLayerExtension::Swizzle::SWIZZLE_GREEN:
		return GL_GREEN;
	case OpenXRCompositionLayerExtension::Swizzle::SWIZZLE_BLUE:
		return GL_BLUE;
	case OpenXRCompositionLayerExtension::Swizzle::SWIZZLE_ALPHA:
		return GL_ALPHA;
	case OpenXRCompositionLayerExtension::Swizzle::SWIZZLE_ZERO:
		return GL_ZERO;
	case OpenXRCompositionLayerExtension::Swizzle::SWIZZLE_ONE:
		return GL_ONE;
	}
#endif
	return 0;
}

uint32_t OpenXRFBUpdateSwapchainExtension::filter_to_vk(
	OpenXRCompositionLayerExtension::Filter p_filter)
{
#ifdef XR_USE_GRAPHICS_API_VULKAN
	switch (p_filter) {
	case OpenXRCompositionLayerExtension::Filter::FILTER_NEAREST:
		return VK_FILTER_NEAREST;
	case OpenXRCompositionLayerExtension::Filter::FILTER_LINEAR:
		return VK_FILTER_LINEAR;
	case OpenXRCompositionLayerExtension::Filter::FILTER_CUBIC:
		return VK_FILTER_CUBIC_EXT;
	}
#endif
	return 0;
}

uint32_t OpenXRFBUpdateSwapchainExtension::mipmap_mode_to_vk(
	OpenXRCompositionLayerExtension::MipmapMode p_mipmap_mode)
{
#ifdef XR_USE_GRAPHICS_API_VULKAN
	switch (p_mipmap_mode) {
	case OpenXRCompositionLayerExtension::MipmapMode::MIPMAP_MODE_DISABLED:
		return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	case OpenXRCompositionLayerExtension::MipmapMode::MIPMAP_MODE_NEAREST:
		return VK_SAMPLER_MIPMAP_MODE_NEAREST;
	case OpenXRCompositionLayerExtension::MipmapMode::MIPMAP_MODE_LINEAR:
		return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	}
#endif
	return 0;
}

uint32_t OpenXRFBUpdateSwapchainExtension::wrap_to_vk(OpenXRCompositionLayerExtension::Wrap p_wrap)
{
#ifdef XR_USE_GRAPHICS_API_VULKAN
	switch (p_wrap) {
	case OpenXRCompositionLayerExtension::Wrap::WRAP_CLAMP_TO_BORDER:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	case OpenXRCompositionLayerExtension::Wrap::WRAP_CLAMP_TO_EDGE:
		return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	case OpenXRCompositionLayerExtension::Wrap::WRAP_REPEAT:
		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	case OpenXRCompositionLayerExtension::Wrap::WRAP_MIRRORED_REPEAT:
		return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	case OpenXRCompositionLayerExtension::Wrap::WRAP_MIRROR_CLAMP_TO_EDGE:
		return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
	}
#endif
	return 0;
}

uint32_t OpenXRFBUpdateSwapchainExtension::swizzle_to_vk(
	OpenXRCompositionLayerExtension::Swizzle p_swizzle)
{
#ifdef XR_USE_GRAPHICS_API_VULKAN
	switch (p_swizzle) {
	case OpenXRCompositionLayerExtension::Swizzle::SWIZZLE_RED:
		return VK_COMPONENT_SWIZZLE_R;
	case OpenXRCompositionLayerExtension::Swizzle::SWIZZLE_GREEN:
		return VK_COMPONENT_SWIZZLE_G;
	case OpenXRCompositionLayerExtension::Swizzle::SWIZZLE_BLUE:
		return VK_COMPONENT_SWIZZLE_B;
	case OpenXRCompositionLayerExtension::Swizzle::SWIZZLE_ALPHA:
		return VK_COMPONENT_SWIZZLE_A;
	case OpenXRCompositionLayerExtension::Swizzle::SWIZZLE_ZERO:
		return VK_COMPONENT_SWIZZLE_ZERO;
	case OpenXRCompositionLayerExtension::Swizzle::SWIZZLE_ONE:
		return VK_COMPONENT_SWIZZLE_ONE;
	}
#endif
	return 0;
}

////////////////////////////////////////////////////////////////////////////
// OpenXRFBFoveationExtension

struct OpenXRFBFoveationExtension::Data
{
#ifdef VULKAN_ENABLED
	XrVulkanSwapchainCreateInfoMETA meta_vulkan_swapchain_create_info;
#endif
	String rendering_driver;
	bool fb_foveation_ext = false;
	bool fb_foveation_configuration_ext = false;
	bool fb_foveation_vulkan_ext = false;
	bool meta_foveation_eye_tracked_ext = false;
	bool meta_vulkan_swapchain_create_info_ext = false;
	XrFoveationLevelFB foveation_level = XR_FOVEATION_LEVEL_NONE_FB;
	XrFoveationDynamicFB foveation_dynamic = XR_FOVEATION_DYNAMIC_DISABLED_FB;
	bool foveation_with_subsampled_images_enabled = false;
	bool foveation_with_subsampled_images_active = false;
	XrSwapchainCreateInfoFoveationFB swapchain_create_info_foveation_fb;
	XrSystemFoveationEyeTrackedPropertiesMETA meta_foveation_eye_tracked_properties;
	XrFoveationEyeTrackedProfileCreateInfoMETA meta_foveation_eye_tracked_create_info;
};

void OpenXRFBFoveationExtension::initialize(const String& p_rendering_driver)
{
	if (!data) {
		data = memnew(Data);
	}
	data->rendering_driver = p_rendering_driver;
}

void OpenXRFBFoveationExtension::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

void OpenXRFBFoveationExtension::on_instance_created(const XrInstance p_instance)
{
	if (data->fb_foveation_ext) {
		EXT_INIT_XR_FUNC(xrCreateFoveationProfileFB);
		EXT_INIT_XR_FUNC(xrDestroyFoveationProfileFB);
	}

	if (data->meta_foveation_eye_tracked_ext) {
		EXT_INIT_XR_FUNC(xrGetFoveationEyeTrackedStateMETA);
	}
}

void OpenXRFBFoveationExtension::on_instance_destroyed()
{
	data->fb_foveation_ext = false;
	data->fb_foveation_configuration_ext = false;
	data->meta_foveation_eye_tracked_ext = false;
}

bool OpenXRFBFoveationExtension::is_enabled()
{
	bool enabled = OpenXRFBUpdateSwapchainExtension::is_initialized() &&
				   OpenXRFBUpdateSwapchainExtension::is_enabled() && data->fb_foveation_ext &&
				   data->fb_foveation_configuration_ext;
#ifdef XR_USE_GRAPHICS_API_VULKAN
	if (data->rendering_driver == "vulkan") {
		enabled = enabled && data->fb_foveation_vulkan_ext;
	}
#endif
	return enabled;
}

void* OpenXRFBFoveationExtension::set_system_properties_and_get_next_pointer(void* p_next_pointer)
{
#ifdef XR_USE_GRAPHICS_API_VULKAN
	if (data->rendering_driver == "vulkan") {
		data->meta_foveation_eye_tracked_properties.next = p_next_pointer;
		return &data->meta_foveation_eye_tracked_properties;
	}
#endif
	return p_next_pointer;
}

void* OpenXRFBFoveationExtension::set_swapchain_create_info_and_get_next_pointer(
	void* p_next_pointer)
{
	void* next = p_next_pointer;
	if (is_enabled() && data->foveation_level > 0) {
		data->swapchain_create_info_foveation_fb.next = next;
		next = &data->swapchain_create_info_foveation_fb;

#ifdef VULKAN_ENABLED
		if (data->meta_vulkan_swapchain_create_info_ext) {
			data->meta_vulkan_swapchain_create_info.additionalCreateFlags = 0;
			if (data->meta_foveation_eye_tracked_ext &&
				data->meta_foveation_eye_tracked_properties.supportsFoveationEyeTracked) {
				data->meta_vulkan_swapchain_create_info.additionalCreateFlags |=
					VK_IMAGE_CREATE_FRAGMENT_DENSITY_MAP_OFFSET_BIT_QCOM;
			}
			if (data->foveation_with_subsampled_images_enabled &&
				data->foveation_with_subsampled_images_active) {
				data->meta_vulkan_swapchain_create_info.additionalCreateFlags |=
					VK_IMAGE_CREATE_SUBSAMPLED_BIT_EXT;
			}

			data->meta_vulkan_swapchain_create_info.next = next;
			next = &data->meta_vulkan_swapchain_create_info;
		}
#endif
	}

	return next;
}

void OpenXRFBFoveationExtension::on_main_swapchains_created() { update_profile(); }

XrFoveationLevelFB OpenXRFBFoveationExtension::get_foveation_level()
{
	return data ? data->foveation_level : XR_FOVEATION_LEVEL_NONE_FB;
}

void OpenXRFBFoveationExtension::set_foveation_level(XrFoveationLevelFB p_foveation_level)
{
	data->foveation_level = p_foveation_level;
	update_profile();
}

XrFoveationDynamicFB OpenXRFBFoveationExtension::get_foveation_dynamic()
{
	return data ? data->foveation_dynamic : XR_FOVEATION_DYNAMIC_DISABLED_FB;
}

void OpenXRFBFoveationExtension::set_foveation_dynamic(XrFoveationDynamicFB p_foveation_dynamic)
{
	data->foveation_dynamic = p_foveation_dynamic;
	update_profile();
}

bool OpenXRFBFoveationExtension::is_foveation_eye_tracked_enabled()
{
	return is_enabled() && data->meta_foveation_eye_tracked_ext &&
		   data->meta_vulkan_swapchain_create_info_ext &&
		   data->meta_foveation_eye_tracked_properties.supportsFoveationEyeTracked;
}

void OpenXRFBFoveationExtension::set_foveation_with_subsampled_images_enabled(bool p_enabled)
{
	data->foveation_with_subsampled_images_enabled = p_enabled;
}

bool OpenXRFBFoveationExtension::is_foveation_with_subsampled_images_enabled()
{
	return is_enabled() && data->foveation_level > 0 &&
		   data->meta_vulkan_swapchain_create_info_ext &&
		   data->foveation_with_subsampled_images_enabled;
}

void OpenXRFBFoveationExtension::set_foveation_with_subsampled_images_active(bool p_active)
{
	data->foveation_with_subsampled_images_active = p_active;
}

void OpenXRFBFoveationExtension::update_profile() {}

void OpenXRFBFoveationExtension::get_fragment_density_offsets(LocalVector<Vector2i>& r_offsets) {}

HashMap<String, bool*> OpenXRFBFoveationExtension::get_requested_extensions(XrVersion p_version)
{
	return HashMap<String, bool*>();
}

#ifdef None
#undef None
#endif
#ifdef Status
#undef Status
#endif
#ifdef Bool
#undef Bool
#endif
#ifdef True
#undef True
#endif
#ifdef False
#undef False
#endif
#ifdef CursorShape
#undef CursorShape
#endif
#ifdef KeyPress
#undef KeyPress
#endif
#ifdef KeyRelease
#undef KeyRelease
#endif
#ifdef FocusIn
#undef FocusIn
#endif
#ifdef FocusOut
#undef FocusOut
#endif
#ifdef Expose
#undef Expose
#endif
#ifdef FontChange
#undef FontChange
#endif
#ifdef Unsorted
#undef Unsorted
#endif
#ifdef Always
#undef Always
#endif
#ifdef Above
#undef Above
#endif
#ifdef Below
#undef Below
#endif

// ============================================================================
// Merged from modules/openxr/extensions/openxr_interaction_profile_extension.cpp
// ============================================================================

#include "modules/openxr/openxr_api.h"
#include "core/math/math_funcs.h"
#include "core/string/print_string.h"


#ifndef XR_LOGITECH_MX_INK_STYLUS_INTERACTION_EXTENSION_NAME
#define XR_LOGITECH_MX_INK_STYLUS_INTERACTION_EXTENSION_NAME "XR_LOGITECH_mx_ink_stylus_interaction"
#endif

#ifndef XR_VALVE_FRAME_CONTROLLER_INTERACTION_EXTENSION_NAME
#define XR_VALVE_FRAME_CONTROLLER_INTERACTION_EXTENSION_NAME "XR_VALVE_frame_controller_interaction"
#endif

////////////////////////////////////////////////////////////////////////////
// OpenXRInteractionProfileExtension

OpenXRInteractionProfileExtension::OpenXRInteractionProfileExtension(ProfileKind p_kind)
	: kind(p_kind)
{
}

void OpenXRInteractionProfileExtension::initialize()
{
	if (data) {
		return;
	}
	data = memnew_arr(OpenXRInteractionProfileExtension, PROFILE_MAX);
	for (int i = 0; i < PROFILE_MAX; i++) {
		data[i] = OpenXRInteractionProfileExtension(static_cast<ProfileKind>(i));
	}
}

void OpenXRInteractionProfileExtension::finalize()
{
	if (!data) {
		return;
	}
	memdelete_arr(data);
	data = nullptr;
}

OpenXRInteractionProfileExtension* OpenXRInteractionProfileExtension::get_profile(
	ProfileKind p_kind)
{
	ERR_FAIL_INDEX_V(p_kind, PROFILE_MAX, nullptr);
	return data ? &data[p_kind] : nullptr;
}

bool OpenXRInteractionProfileExtension::is_profile_available(ProfileKind p_kind, int p_subindex)
{
	ERR_FAIL_INDEX_V(p_kind, PROFILE_MAX, false);
	return data != nullptr && data[p_kind].is_available(p_subindex);
}

HashMap<String, bool*> OpenXRInteractionProfileExtension::get_all_requested_extensions(
	XrVersion p_version)
{
	HashMap<String, bool*> result;
	if (!data) {
		return result;
	}
	for (int i = 0; i < PROFILE_MAX; i++) {
		HashMap<String, bool*> ext = data[i].get_requested_extensions(p_version);
		for (const KeyValue<String, bool*>& kv : ext) {
			result[kv.key] = kv.value;
		}
	}
	return result;
}

HashMap<String, bool*> OpenXRInteractionProfileExtension::get_requested_extensions(
	XrVersion p_version)
{
	HashMap<String, bool*> request_extensions;

	switch (kind) {
	case PROFILE_HTC_CONTROLLER: {
		if (p_version < XR_API_VERSION_1_1_0) {
			request_extensions[XR_HTC_VIVE_COSMOS_CONTROLLER_INTERACTION_EXTENSION_NAME] =
				&flags[HTC_VIVE_COSMOS];
			request_extensions[XR_HTC_VIVE_FOCUS3_CONTROLLER_INTERACTION_EXTENSION_NAME] =
				&flags[HTC_VIVE_FOCUS3];
		}
		request_extensions[XR_HTC_HAND_INTERACTION_EXTENSION_NAME] = &flags[HTC_HAND_INTERACTION];
	} break;
	case PROFILE_HTC_VIVE_TRACKER: {
		request_extensions[XR_HTCX_VIVE_TRACKER_INTERACTION_EXTENSION_NAME] = &flags[0];
	} break;
	case PROFILE_HUAWEI_CONTROLLER: {
		request_extensions[XR_HUAWEI_CONTROLLER_INTERACTION_EXTENSION_NAME] = &flags[0];
	} break;
	case PROFILE_KHR_GENERIC_CONTROLLER: {
		if (p_version < XR_API_VERSION_1_1_0) {
			request_extensions[XR_KHR_GENERIC_CONTROLLER_EXTENSION_NAME] = &flags[0];
		}
	} break;
	case PROFILE_META_CONTROLLER: {
		if (p_version < XR_API_VERSION_1_1_0) {
			request_extensions[XR_FB_TOUCH_CONTROLLER_PRO_EXTENSION_NAME] = &flags[META_TOUCH_PRO];
			request_extensions[XR_META_TOUCH_CONTROLLER_PLUS_EXTENSION_NAME] =
				&flags[META_TOUCH_PLUS];
		}
		request_extensions[XR_FB_TOUCH_CONTROLLER_PROXIMITY_EXTENSION_NAME] =
			&flags[META_TOUCH_PROXIMITY];
	} break;
	case PROFILE_ML2_CONTROLLER: {
		if (p_version < XR_API_VERSION_1_1_0) {
			request_extensions[XR_ML_ML2_CONTROLLER_INTERACTION_EXTENSION_NAME] = &flags[0];
		}
	} break;
	case PROFILE_MXINK: {
		request_extensions[XR_LOGITECH_MX_INK_STYLUS_INTERACTION_EXTENSION_NAME] = &flags[0];
	} break;
	case PROFILE_PALM_POSE: {
		if (p_version < XR_API_VERSION_1_1_0) {
			request_extensions[XR_EXT_PALM_POSE_EXTENSION_NAME] = &flags[0];
		}
	} break;
	case PROFILE_PICO_CONTROLLER: {
		request_extensions[XR_BD_CONTROLLER_INTERACTION_EXTENSION_NAME] = &flags[0];
	} break;
	case PROFILE_VALVE_CONTROLLER: {
		request_extensions[XR_VALVE_FRAME_CONTROLLER_INTERACTION_EXTENSION_NAME] = &flags[0];
	} break;
	case PROFILE_WMR_CONTROLLER: {
		if (p_version < XR_API_VERSION_1_1_0) {
			request_extensions[XR_EXT_HP_MIXED_REALITY_CONTROLLER_EXTENSION_NAME] =
				&flags[WMR_HPMR];
			request_extensions[XR_EXT_SAMSUNG_ODYSSEY_CONTROLLER_EXTENSION_NAME] =
				&flags[WMR_SAMSUNG_ODESSY];
		}
		request_extensions[XR_MSFT_HAND_INTERACTION_EXTENSION_NAME] = &flags[WMR_HAND_INTERACTION];
	} break;
	case PROFILE_HAND_INTERACTION: {
		request_extensions[XR_EXT_HAND_INTERACTION_EXTENSION_NAME] = &flags[0];
	} break;
	case PROFILE_LOCAL_FLOOR: {
		if (p_version < XR_API_VERSION_1_1_0) {
			request_extensions[XR_EXT_LOCAL_FLOOR_EXTENSION_NAME] = &flags[0];
		}
	} break;
	case PROFILE_DPAD_BINDING: {
		request_extensions[XR_KHR_BINDING_MODIFICATION_EXTENSION_NAME] = &flags[0];
		request_extensions[XR_EXT_DPAD_BINDING_EXTENSION_NAME] = &flags[1];
	} break;
	case PROFILE_VALVE_ANALOG_THRESHOLD: {
		request_extensions[XR_KHR_BINDING_MODIFICATION_EXTENSION_NAME] = &flags[0];
		request_extensions[XR_VALVE_ANALOG_THRESHOLD_EXTENSION_NAME] = &flags[1];
	} break;
	case PROFILE_COMPOSITION_LAYER_DEPTH: {
		request_extensions[XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME] = &flags[0];
	} break;
	case PROFILE_MAX:
		break;
	}

	return request_extensions;
}

PackedStringArray OpenXRInteractionProfileExtension::get_suggested_tracker_names() const
{
	switch (kind) {
	case PROFILE_HTC_CONTROLLER: {
		return PackedStringArray{
			"/user/hand_htc/left",
			"/user/hand_htc/right",
		};
	}
	case PROFILE_HTC_VIVE_TRACKER: {
		return PackedStringArray{
			"/user/vive_tracker_htcx/role/handheld_object",
			"/user/vive_tracker_htcx/role/left_foot",
			"/user/vive_tracker_htcx/role/right_foot",
			"/user/vive_tracker_htcx/role/left_shoulder",
			"/user/vive_tracker_htcx/role/right_shoulder",
			"/user/vive_tracker_htcx/role/left_elbow",
			"/user/vive_tracker_htcx/role/right_elbow",
			"/user/vive_tracker_htcx/role/left_knee",
			"/user/vive_tracker_htcx/role/right_knee",
			"/user/vive_tracker_htcx/role/waist",
			"/user/vive_tracker_htcx/role/chest",
			"/user/vive_tracker_htcx/role/camera",
			"/user/vive_tracker_htcx/role/keyboard",
			"/user/vive_tracker_htcx/role/left_wrist",
			"/user/vive_tracker_htcx/role/right_wrist",
			"/user/vive_tracker_htcx/role/left_ankle",
			"/user/vive_tracker_htcx/role/right_ankle",
		};
	}
	default:
		return PackedStringArray();
	}
}

bool OpenXRInteractionProfileExtension::on_event_polled(const XrEventDataBuffer& p_event)
{
	if (kind == PROFILE_HTC_VIVE_TRACKER &&
		p_event.type == XR_TYPE_EVENT_DATA_VIVE_TRACKER_CONNECTED_HTCX) {
		print_verbose("OpenXR EVENT: VIVE tracker connected");
		return true;
	}
	return false;
}

void OpenXRInteractionProfileExtension::on_register_metadata()
{
	if (kind != PROFILE_HAND_INTERACTION) {
		return;
	}

	const String profile_path = "/interaction_profiles/ext/hand_interaction_ext";
	OpenXRInteractionProfileMetadata::register_interaction_profile(
		"Hand interaction", profile_path, XR_EXT_HAND_INTERACTION_EXTENSION_NAME);
	for (const String user_path : {"/user/hand/left", "/user/hand/right"}) {
		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Grip pose", user_path,
			user_path + "/input/grip/pose", "", OpenXRAction::OPENXR_ACTION_POSE);
		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Aim pose", user_path,
			user_path + "/input/aim/pose", "", OpenXRAction::OPENXR_ACTION_POSE);
		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Pinch pose", user_path,
			user_path + "/input/pinch_ext/pose", "", OpenXRAction::OPENXR_ACTION_POSE);
		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Poke pose", user_path,
			user_path + "/input/poke_ext/pose", "", OpenXRAction::OPENXR_ACTION_POSE);
		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Grip surface pose",
			user_path, user_path + "/input/grip_surface/pose",
			XR_EXT_PALM_POSE_EXTENSION_NAME "," XR_KHR_MAINTENANCE1_EXTENSION_NAME
											"," XR_OPENXR_1_1_NAME,
			OpenXRAction::OPENXR_ACTION_POSE);

		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Pinch", user_path,
			user_path + "/input/pinch_ext/value", "", OpenXRAction::OPENXR_ACTION_FLOAT);
		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Pinch ready", user_path,
			user_path + "/input/pinch_ext/ready_ext", "", OpenXRAction::OPENXR_ACTION_BOOL);

		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Aim activate", user_path,
			user_path + "/input/aim_activate_ext/value", "", OpenXRAction::OPENXR_ACTION_FLOAT);
		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Aim activate ready",
			user_path, user_path + "/input/aim_activate_ext/ready_ext", "",
			OpenXRAction::OPENXR_ACTION_BOOL);

		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Grasp", user_path,
			user_path + "/input/grasp_ext/value", "", OpenXRAction::OPENXR_ACTION_FLOAT);
		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Grasp ready", user_path,
			user_path + "/input/grasp_ext/ready_ext", "", OpenXRAction::OPENXR_ACTION_BOOL);

		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Swipe left", user_path,
			user_path + "/input/swipe_left_meta/click", "XR_META_hand_tracking_microgestures",
			OpenXRAction::OPENXR_ACTION_BOOL);
		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Swipe right", user_path,
			user_path + "/input/swipe_right_meta/click", "XR_META_hand_tracking_microgestures",
			OpenXRAction::OPENXR_ACTION_BOOL);
		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Swipe forward", user_path,
			user_path + "/input/swipe_forward_meta/click", "XR_META_hand_tracking_microgestures",
			OpenXRAction::OPENXR_ACTION_BOOL);
		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Swipe backward", user_path,
			user_path + "/input/swipe_backward_meta/click", "XR_META_hand_tracking_microgestures",
			OpenXRAction::OPENXR_ACTION_BOOL);
		OpenXRInteractionProfileMetadata::register_io_path(profile_path, "Tap thumb", user_path,
			user_path + "/input/tap_thumb_meta/click", "XR_META_hand_tracking_microgestures",
			OpenXRAction::OPENXR_ACTION_BOOL);
	}
}

bool OpenXRInteractionProfileExtension::is_available(int p_subindex) const
{
	if (kind == PROFILE_DPAD_BINDING || kind == PROFILE_VALVE_ANALOG_THRESHOLD) {
		return flags[0] && flags[1];
	}
	ERR_FAIL_INDEX_V(p_subindex, 3, false);
	return flags[p_subindex];
}

////////////////////////////////////////////////////////////////////////////
// OpenXRDpadBindingModifier

OpenXRDpadBindingModifier::OpenXRDpadBindingModifier()
{
	ERR_FAIL_COND(
		dpad_bindings_data.resize_initialized(sizeof(XrInteractionProfileDpadBindingEXT)) != OK);
	dpad_bindings = (XrInteractionProfileDpadBindingEXT*)dpad_bindings_data.ptrw();

	dpad_bindings->type = XR_TYPE_INTERACTION_PROFILE_DPAD_BINDING_EXT;
	dpad_bindings->next = nullptr;

	dpad_bindings->forceThreshold = 0.6;
	dpad_bindings->forceThresholdReleased = 0.4;
	dpad_bindings->centerRegion = 0.1;
	dpad_bindings->wedgeAngle = Math::deg_to_rad(90.0);
	dpad_bindings->isSticky = false;
}

void OpenXRDpadBindingModifier::set_action_set(const Ref<OpenXRActionSet>& p_action_set)
{
	action_set = p_action_set;
}

Ref<OpenXRActionSet> OpenXRDpadBindingModifier::get_action_set() const { return action_set; }

void OpenXRDpadBindingModifier::set_input_path(const String& p_input_path)
{
	input_path = p_input_path;
	emit_changed();
}

String OpenXRDpadBindingModifier::get_input_path() const { return input_path; }

void OpenXRDpadBindingModifier::set_threshold(float p_threshold)
{
	ERR_FAIL_NULL(dpad_bindings);
	ERR_FAIL_COND(p_threshold < 0.0 || p_threshold > 1.0);

	dpad_bindings->forceThreshold = p_threshold;
	emit_changed();
}

float OpenXRDpadBindingModifier::get_threshold() const
{
	ERR_FAIL_NULL_V(dpad_bindings, 0.0);
	return dpad_bindings->forceThreshold;
}

void OpenXRDpadBindingModifier::set_threshold_released(float p_threshold)
{
	ERR_FAIL_NULL(dpad_bindings);
	ERR_FAIL_COND(p_threshold < 0.0 || p_threshold > 1.0);

	dpad_bindings->forceThresholdReleased = p_threshold;
	emit_changed();
}

float OpenXRDpadBindingModifier::get_threshold_released() const
{
	ERR_FAIL_NULL_V(dpad_bindings, 0.0);
	return dpad_bindings->forceThresholdReleased;
}

void OpenXRDpadBindingModifier::set_center_region(float p_center_region)
{
	ERR_FAIL_NULL(dpad_bindings);
	ERR_FAIL_COND(p_center_region < 0.0 || p_center_region > 1.0);

	dpad_bindings->centerRegion = p_center_region;
	emit_changed();
}

float OpenXRDpadBindingModifier::get_center_region() const
{
	ERR_FAIL_NULL_V(dpad_bindings, 0.0);
	return dpad_bindings->centerRegion;
}

void OpenXRDpadBindingModifier::set_wedge_angle(float p_wedge_angle)
{
	ERR_FAIL_NULL(dpad_bindings);
	dpad_bindings->wedgeAngle = p_wedge_angle;
	emit_changed();
}

float OpenXRDpadBindingModifier::get_wedge_angle() const
{
	ERR_FAIL_NULL_V(dpad_bindings, 0.0);
	return dpad_bindings->wedgeAngle;
}

void OpenXRDpadBindingModifier::set_wedge_angle_deg(float p_wedge_angle)
{
	ERR_FAIL_NULL(dpad_bindings);
	dpad_bindings->wedgeAngle = Math::deg_to_rad(p_wedge_angle);
	emit_changed();
}

float OpenXRDpadBindingModifier::get_wedge_angle_deg() const
{
	ERR_FAIL_NULL_V(dpad_bindings, 0.0);
	return Math::rad_to_deg(dpad_bindings->wedgeAngle);
}

void OpenXRDpadBindingModifier::set_is_sticky(bool p_sticky)
{
	ERR_FAIL_NULL(dpad_bindings);
	dpad_bindings->isSticky = p_sticky;
	emit_changed();
}

bool OpenXRDpadBindingModifier::get_is_sticky() const
{
	ERR_FAIL_NULL_V(dpad_bindings, false);
	return dpad_bindings->isSticky;
}

void OpenXRDpadBindingModifier::set_on_haptic(const Ref<OpenXRHapticBase>& p_haptic)
{
	on_haptic = p_haptic;
	emit_changed();
}

Ref<OpenXRHapticBase> OpenXRDpadBindingModifier::get_on_haptic() const { return on_haptic; }

void OpenXRDpadBindingModifier::set_off_haptic(const Ref<OpenXRHapticBase>& p_haptic)
{
	off_haptic = p_haptic;
	emit_changed();
}

Ref<OpenXRHapticBase> OpenXRDpadBindingModifier::get_off_haptic() const { return off_haptic; }

PackedByteArray OpenXRDpadBindingModifier::get_ip_modification()
{
	ERR_FAIL_NULL_V(dpad_bindings, PackedByteArray());
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), PackedByteArray());

	if (!OpenXRInteractionProfileExtension::is_initialized() ||
		!OpenXRInteractionProfileExtension::is_profile_available(
			OpenXRInteractionProfileExtension::PROFILE_DPAD_BINDING)) {
		WARN_PRINT("DPad binding extension is not enabled or available.");
		return PackedByteArray();
	}

	dpad_bindings->binding = OpenXRAPI::get_xr_path(input_path);
	ERR_FAIL_COND_V(dpad_bindings->binding == XR_NULL_PATH, PackedByteArray());

	ERR_FAIL_COND_V(action_set.is_null(), PackedByteArray());
	RID action_set_rid = OpenXRAPI::find_action_set(action_set->get_name());
	ERR_FAIL_COND_V(!action_set_rid.is_valid(), PackedByteArray());
	dpad_bindings->actionSet = OpenXRAPI::action_set_get_handle(action_set_rid);

	dpad_bindings->onHaptic = on_haptic.is_valid() ? on_haptic->get_xr_structure() : nullptr;
	dpad_bindings->offHaptic = off_haptic.is_valid() ? off_haptic->get_xr_structure() : nullptr;

	return dpad_bindings_data;
}

////////////////////////////////////////////////////////////////////////////
// OpenXRAnalogThresholdModifier

OpenXRAnalogThresholdModifier::OpenXRAnalogThresholdModifier()
{
	analog_threshold.type = XR_TYPE_INTERACTION_PROFILE_ANALOG_THRESHOLD_VALVE;
	analog_threshold.next = nullptr;

	analog_threshold.onThreshold = 0.6;
	analog_threshold.offThreshold = 0.4;
}

void OpenXRAnalogThresholdModifier::set_on_threshold(float p_threshold)
{
	ERR_FAIL_COND(p_threshold < 0.0 || p_threshold > 1.0);

	analog_threshold.onThreshold = p_threshold;
	emit_changed();
}

float OpenXRAnalogThresholdModifier::get_on_threshold() const
{
	return analog_threshold.onThreshold;
}

void OpenXRAnalogThresholdModifier::set_off_threshold(float p_threshold)
{
	ERR_FAIL_COND(p_threshold < 0.0 || p_threshold > 1.0);

	analog_threshold.offThreshold = p_threshold;
	emit_changed();
}

float OpenXRAnalogThresholdModifier::get_off_threshold() const
{
	return analog_threshold.offThreshold;
}

void OpenXRAnalogThresholdModifier::set_on_haptic(const Ref<OpenXRHapticBase>& p_haptic)
{
	on_haptic = p_haptic;
	emit_changed();
}

Ref<OpenXRHapticBase> OpenXRAnalogThresholdModifier::get_on_haptic() const { return on_haptic; }

void OpenXRAnalogThresholdModifier::set_off_haptic(const Ref<OpenXRHapticBase>& p_haptic)
{
	off_haptic = p_haptic;
	emit_changed();
}

Ref<OpenXRHapticBase> OpenXRAnalogThresholdModifier::get_off_haptic() const { return off_haptic; }

PackedByteArray OpenXRAnalogThresholdModifier::get_ip_modification()
{
	PackedByteArray ret;

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), ret);

	if (!OpenXRInteractionProfileExtension::is_initialized() ||
		!OpenXRInteractionProfileExtension::is_profile_available(
			OpenXRInteractionProfileExtension::PROFILE_VALVE_ANALOG_THRESHOLD)) {
		WARN_PRINT("Analog threshold extension is not enabled or available.");
		return ret;
	}

	ERR_FAIL_NULL_V(ip_binding, ret);

	Ref<OpenXRAction> action = ip_binding->get_action();
	ERR_FAIL_COND_V(action.is_null(), ret);

	Ref<OpenXRActionSet> action_set = action->get_action_set();
	ERR_FAIL_COND_V(action_set.is_null(), ret);
	RID action_set_rid = OpenXRAPI::find_action_set(action_set->get_name());
	ERR_FAIL_COND_V(!action_set_rid.is_valid(), ret);

	RID action_rid = OpenXRAPI::find_action(action->get_name(), action_set_rid);
	ERR_FAIL_COND_V(!action_rid.is_valid(), ret);

	analog_threshold.action = OpenXRAPI::action_get_handle(action_rid);

	analog_threshold.binding = OpenXRAPI::get_xr_path(ip_binding->get_binding_path());
	ERR_FAIL_COND_V(analog_threshold.binding == XR_NULL_PATH, ret);

	analog_threshold.onHaptic = on_haptic.is_valid() ? on_haptic->get_xr_structure() : nullptr;
	analog_threshold.offHaptic = off_haptic.is_valid() ? off_haptic->get_xr_structure() : nullptr;

	ERR_FAIL_COND_V(ret.resize(sizeof(XrInteractionProfileAnalogThresholdVALVE)) != OK, ret);
	memcpy(ret.ptrw(), &analog_threshold, sizeof(XrInteractionProfileAnalogThresholdVALVE));

	return ret;
}

// ============================================================================
// Merged from modules/openxr/extensions/spatial_entities/openxr_spatial_entities.cpp
// ============================================================================

#include <cstring>
#include <openxr/openxr.h>
#include "modules/openxr/openxr_api.h"
#include "core/config/project_settings.h"

#include "scene/resources/mesh.h"
#include "servers/xr/xr_server.h"

#ifndef PHYSICS_3D_DISABLED
#include "scene/resources/mesh.h"
#endif

////////////////////////////////////////////////////////////////////////////
// OpenXRSpatialCapabilityConfigurationBaseHeader

OpenXRSpatialCapabilityConfigurationBaseHeader::OpenXRSpatialCapabilityConfigurationBaseHeader(
	CapabilityType p_capability_type)
{
	set_capability_type(p_capability_type);
}

OpenXRSpatialCapabilityConfigurationBaseHeader::~OpenXRSpatialCapabilityConfigurationBaseHeader() {}

void OpenXRSpatialCapabilityConfigurationBaseHeader::set_capability_type(
	CapabilityType p_capability_type)
{
	capability_type = p_capability_type;
	enabled_components.clear();

	if (capability_type == CAPABILITY_ARUCO) {
		int dict = GLOBAL_GET_CACHED(int, "xr/openxr/extensions/spatial_entity/aruco_dict");
		aruco_dict = (XrSpatialMarkerArucoDictEXT)(XR_SPATIAL_MARKER_ARUCO_DICT_4X4_50_EXT + dict);
	}
	else if (capability_type == CAPABILITY_APRIL_TAG) {
		int dict = GLOBAL_GET_CACHED(int, "xr/openxr/extensions/spatial_entity/april_tag_dict");
		april_dict =
			(XrSpatialMarkerAprilTagDictEXT)(XR_SPATIAL_MARKER_APRIL_TAG_DICT_16H5_EXT + dict);
	}
}

bool OpenXRSpatialCapabilityConfigurationBaseHeader::has_valid_configuration() const
{
	switch (capability_type) {
	case CAPABILITY_ANCHOR: {
		ERR_FAIL_COND_V(!OpenXRSpatialAnchorCapability::is_initialized(), false);
		return OpenXRSpatialAnchorCapability::is_spatial_anchor_supported();
	}
	case CAPABILITY_PLANE_TRACKING: {
		ERR_FAIL_COND_V(!OpenXRSpatialPlaneTrackingCapability::is_initialized(), false);
		return OpenXRSpatialPlaneTrackingCapability::is_supported();
	}
	case CAPABILITY_QR_CODE: {
		ERR_FAIL_COND_V(!OpenXRSpatialMarkerTrackingCapability::is_initialized(), false);
		return OpenXRSpatialMarkerTrackingCapability::is_qrcode_supported();
	}
	case CAPABILITY_MICRO_QR_CODE: {
		ERR_FAIL_COND_V(!OpenXRSpatialMarkerTrackingCapability::is_initialized(), false);
		return OpenXRSpatialMarkerTrackingCapability::is_micro_qrcode_supported();
	}
	case CAPABILITY_ARUCO: {
		ERR_FAIL_COND_V(!OpenXRSpatialMarkerTrackingCapability::is_initialized(), false);
		return OpenXRSpatialMarkerTrackingCapability::is_aruco_supported();
	}
	case CAPABILITY_APRIL_TAG: {
		ERR_FAIL_COND_V(!OpenXRSpatialMarkerTrackingCapability::is_initialized(), false);
		return OpenXRSpatialMarkerTrackingCapability::is_april_tag_supported();
	}
	case CAPABILITY_NONE:
	default:
		return false;
	}
}

uint64_t OpenXRSpatialCapabilityConfigurationBaseHeader::_get_configurationgd()
{
	return (uint64_t)get_configuration();
}

XrSpatialCapabilityConfigurationBaseHeaderEXT*
OpenXRSpatialCapabilityConfigurationBaseHeader::get_configuration()
{
	switch (capability_type) {
	case CAPABILITY_ANCHOR: {
		ERR_FAIL_COND_V(!OpenXRSpatialAnchorCapability::is_initialized(), nullptr);
		if (!OpenXRSpatialAnchorCapability::is_spatial_anchor_supported()) {
			return nullptr;
		}
		ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), nullptr);

		enabled_components.clear();
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_ANCHOR_EXT);
		if (OpenXRSpatialAnchorCapability::is_spatial_persistence_supported()) {
			enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_PERSISTENCE_EXT);
		}

		config.anchor = {
			XR_TYPE_SPATIAL_CAPABILITY_CONFIGURATION_ANCHOR_EXT,
			nullptr,
			XR_SPATIAL_CAPABILITY_ANCHOR_EXT,
			(uint32_t)enabled_components.size(),
			enabled_components.ptr(),
		};
		return (XrSpatialCapabilityConfigurationBaseHeaderEXT*)&config.anchor;
	}
	case CAPABILITY_PLANE_TRACKING: {
		ERR_FAIL_COND_V(!OpenXRSpatialPlaneTrackingCapability::is_initialized(), nullptr);
		if (!OpenXRSpatialPlaneTrackingCapability::is_supported()) {
			return nullptr;
		}
		ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), nullptr);

		enabled_components.clear();
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_BOUNDED_2D_EXT);
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_PLANE_ALIGNMENT_EXT);

		if (get_supports_mesh_2d()) {
			enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_MESH_2D_EXT);
		}
		else if (get_supports_polygons()) {
			enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_POLYGON_2D_EXT);
		}
		if (get_supports_labels()) {
			enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_PLANE_SEMANTIC_LABEL_EXT);
		}

		config.plane = {
			XR_TYPE_SPATIAL_CAPABILITY_CONFIGURATION_PLANE_TRACKING_EXT,
			nullptr,
			XR_SPATIAL_CAPABILITY_PLANE_TRACKING_EXT,
			(uint32_t)enabled_components.size(),
			enabled_components.ptr(),
		};
		return (XrSpatialCapabilityConfigurationBaseHeaderEXT*)&config.plane;
	}
	case CAPABILITY_QR_CODE: {
		ERR_FAIL_COND_V(!OpenXRSpatialMarkerTrackingCapability::is_initialized(), nullptr);
		if (!OpenXRSpatialMarkerTrackingCapability::is_qrcode_supported()) {
			return nullptr;
		}
		ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), nullptr);

		enabled_components.clear();
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_MARKER_EXT);
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_BOUNDED_2D_EXT);

		config.qrcode = {
			XR_TYPE_SPATIAL_CAPABILITY_CONFIGURATION_QR_CODE_EXT,
			nullptr,
			XR_SPATIAL_CAPABILITY_MARKER_TRACKING_QR_CODE_EXT,
			(uint32_t)enabled_components.size(),
			enabled_components.ptr(),
		};
		return (XrSpatialCapabilityConfigurationBaseHeaderEXT*)&config.qrcode;
	}
	case CAPABILITY_MICRO_QR_CODE: {
		ERR_FAIL_COND_V(!OpenXRSpatialMarkerTrackingCapability::is_initialized(), nullptr);
		if (!OpenXRSpatialMarkerTrackingCapability::is_micro_qrcode_supported()) {
			return nullptr;
		}
		ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), nullptr);

		enabled_components.clear();
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_MARKER_EXT);
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_BOUNDED_2D_EXT);

		config.micro_qrcode = {
			XR_TYPE_SPATIAL_CAPABILITY_CONFIGURATION_MICRO_QR_CODE_EXT,
			nullptr,
			XR_SPATIAL_CAPABILITY_MARKER_TRACKING_MICRO_QR_CODE_EXT,
			(uint32_t)enabled_components.size(),
			enabled_components.ptr(),
		};
		return (XrSpatialCapabilityConfigurationBaseHeaderEXT*)&config.micro_qrcode;
	}
	case CAPABILITY_ARUCO: {
		ERR_FAIL_COND_V(!OpenXRSpatialMarkerTrackingCapability::is_initialized(), nullptr);
		if (!OpenXRSpatialMarkerTrackingCapability::is_aruco_supported()) {
			return nullptr;
		}
		ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), nullptr);

		enabled_components.clear();
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_MARKER_EXT);
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_BOUNDED_2D_EXT);

		config.aruco = {
			XR_TYPE_SPATIAL_CAPABILITY_CONFIGURATION_ARUCO_MARKER_EXT,
			nullptr,
			XR_SPATIAL_CAPABILITY_MARKER_TRACKING_ARUCO_MARKER_EXT,
			(uint32_t)enabled_components.size(),
			enabled_components.ptr(),
			aruco_dict,
		};
		return (XrSpatialCapabilityConfigurationBaseHeaderEXT*)&config.aruco;
	}
	case CAPABILITY_APRIL_TAG: {
		ERR_FAIL_COND_V(!OpenXRSpatialMarkerTrackingCapability::is_initialized(), nullptr);
		if (!OpenXRSpatialMarkerTrackingCapability::is_april_tag_supported()) {
			return nullptr;
		}
		ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), nullptr);

		enabled_components.clear();
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_MARKER_EXT);
		enabled_components.push_back(XR_SPATIAL_COMPONENT_TYPE_BOUNDED_2D_EXT);

		config.april_tag = {
			XR_TYPE_SPATIAL_CAPABILITY_CONFIGURATION_APRIL_TAG_EXT,
			nullptr,
			XR_SPATIAL_CAPABILITY_MARKER_TRACKING_APRIL_TAG_EXT,
			(uint32_t)enabled_components.size(),
			enabled_components.ptr(),
			april_dict,
		};
		return (XrSpatialCapabilityConfigurationBaseHeaderEXT*)&config.april_tag;
	}
	case CAPABILITY_NONE:
	default:
		return nullptr;
	}
}

bool OpenXRSpatialCapabilityConfigurationBaseHeader::get_supports_mesh_2d()
{
	if (supports_mesh_2d == -1) {
		ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), false);

		supports_mesh_2d =
			OpenXRSpatialEntityExtension::supports_component_type(
				XR_SPATIAL_CAPABILITY_PLANE_TRACKING_EXT, XR_SPATIAL_COMPONENT_TYPE_MESH_2D_EXT)
				? 1
				: 0;
	}

	return supports_mesh_2d == 1;
}

bool OpenXRSpatialCapabilityConfigurationBaseHeader::get_supports_polygons()
{
	if (supports_polygons == -1) {
		ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), false);

		supports_polygons =
			OpenXRSpatialEntityExtension::supports_component_type(
				XR_SPATIAL_CAPABILITY_PLANE_TRACKING_EXT, XR_SPATIAL_COMPONENT_TYPE_POLYGON_2D_EXT)
				? 1
				: 0;
	}

	return supports_polygons == 1;
}

bool OpenXRSpatialCapabilityConfigurationBaseHeader::get_supports_labels()
{
	if (supports_labels == -1) {
		ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), false);

		supports_labels =
			OpenXRSpatialEntityExtension::supports_component_type(
				XR_SPATIAL_CAPABILITY_PLANE_TRACKING_EXT,
				XR_SPATIAL_COMPONENT_TYPE_PLANE_SEMANTIC_LABEL_EXT)
				? 1
				: 0;
	}

	return supports_labels == 1;
}

void OpenXRSpatialCapabilityConfigurationBaseHeader::set_aruco_dict(
	XrSpatialMarkerArucoDictEXT p_dict)
{
	aruco_dict = p_dict;
}

void OpenXRSpatialCapabilityConfigurationBaseHeader::_set_aruco_dict(ArucoDict p_dict)
{
	set_aruco_dict((XrSpatialMarkerArucoDictEXT)p_dict);
}

XrSpatialMarkerArucoDictEXT OpenXRSpatialCapabilityConfigurationBaseHeader::get_aruco_dict() const
{
	return aruco_dict;
}

OpenXRSpatialCapabilityConfigurationBaseHeader::ArucoDict
OpenXRSpatialCapabilityConfigurationBaseHeader::_get_aruco_dict() const
{
	return (ArucoDict)get_aruco_dict();
}

void OpenXRSpatialCapabilityConfigurationBaseHeader::set_april_dict(
	XrSpatialMarkerAprilTagDictEXT p_dict)
{
	april_dict = p_dict;
}

void OpenXRSpatialCapabilityConfigurationBaseHeader::_set_april_dict(AprilTagDict p_dict)
{
	set_april_dict((XrSpatialMarkerAprilTagDictEXT)p_dict);
}

XrSpatialMarkerAprilTagDictEXT
OpenXRSpatialCapabilityConfigurationBaseHeader::get_april_dict() const
{
	return april_dict;
}

OpenXRSpatialCapabilityConfigurationBaseHeader::AprilTagDict
OpenXRSpatialCapabilityConfigurationBaseHeader::_get_april_dict() const
{
	return (AprilTagDict)get_april_dict();
}

PackedInt64Array OpenXRSpatialCapabilityConfigurationBaseHeader::_get_enabled_components() const
{
	PackedInt64Array components;

	for (const XrSpatialComponentTypeEXT& component_type : enabled_components) {
		components.push_back((int64_t)component_type);
	}

	return components;
}

////////////////////////////////////////////////////////////////////////////
// OpenXRSpatialComponentData

OpenXRSpatialComponentData::OpenXRSpatialComponentData(ComponentKind p_kind)
	: component_kind(p_kind)
{
	_init_structure_header();
}

OpenXRSpatialComponentData::~OpenXRSpatialComponentData() { _free_buffers(); }

void OpenXRSpatialComponentData::_free_buffers()
{
	if (data_ptr != nullptr) {
		memfree(data_ptr);
		data_ptr = nullptr;
	}
	if (entity_states_ptr != nullptr) {
		memfree(entity_states_ptr);
		entity_states_ptr = nullptr;
	}
	capacity = 0;
}

void OpenXRSpatialComponentData::_init_structure_header()
{
	switch (component_kind) {
	case COMPONENT_QUERY_RESULT:
		structure.query_result = {
			XR_TYPE_SPATIAL_COMPONENT_DATA_QUERY_RESULT_EXT,
			nullptr,
			capacity,
			0,
			static_cast<XrSpatialEntityIdEXT*>(data_ptr),
			capacity,
			0,
			entity_states_ptr,
		};
		break;
	case COMPONENT_BOUNDED_2D:
		structure.bounded2d_list = {
			XR_TYPE_SPATIAL_COMPONENT_BOUNDED_2D_LIST_EXT,
			nullptr,
			capacity,
			static_cast<XrSpatialBounded2DDataEXT*>(data_ptr),
		};
		break;
	case COMPONENT_BOUNDED_3D:
		structure.bounded3d_list = {
			XR_TYPE_SPATIAL_COMPONENT_BOUNDED_3D_LIST_EXT,
			nullptr,
			capacity,
			static_cast<XrBoxf*>(data_ptr),
		};
		break;
	case COMPONENT_PARENT:
		structure.parent_list = {
			XR_TYPE_SPATIAL_COMPONENT_PARENT_LIST_EXT,
			nullptr,
			capacity,
			static_cast<XrSpatialEntityIdEXT*>(data_ptr),
		};
		break;
	case COMPONENT_MESH_2D:
		structure.mesh2d_list = {
			XR_TYPE_SPATIAL_COMPONENT_MESH_2D_LIST_EXT,
			nullptr,
			capacity,
			static_cast<XrSpatialMeshDataEXT*>(data_ptr),
		};
		break;
	case COMPONENT_MESH_3D:
		structure.mesh3d_list = {
			XR_TYPE_SPATIAL_COMPONENT_MESH_3D_LIST_EXT,
			nullptr,
			capacity,
			static_cast<XrSpatialMeshDataEXT*>(data_ptr),
		};
		break;
	case COMPONENT_ANCHOR:
		structure.anchor_list = {
			XR_TYPE_SPATIAL_COMPONENT_ANCHOR_LIST_EXT,
			nullptr,
			capacity,
			static_cast<XrPosef*>(data_ptr),
		};
		break;
	case COMPONENT_PERSISTENCE:
		structure.persistence_list = {
			XR_TYPE_SPATIAL_COMPONENT_PERSISTENCE_LIST_EXT,
			nullptr,
			capacity,
			static_cast<XrSpatialPersistenceDataEXT*>(data_ptr),
		};
		break;
	case COMPONENT_PLANE_ALIGNMENT:
		structure.plane_alignment_list = {
			XR_TYPE_SPATIAL_COMPONENT_PLANE_ALIGNMENT_LIST_EXT,
			nullptr,
			capacity,
			static_cast<XrSpatialPlaneAlignmentEXT*>(data_ptr),
		};
		break;
	case COMPONENT_POLYGON_2D:
		structure.polygon2d_list = {
			XR_TYPE_SPATIAL_COMPONENT_POLYGON_2D_LIST_EXT,
			nullptr,
			capacity,
			static_cast<XrSpatialPolygon2DDataEXT*>(data_ptr),
		};
		break;
	case COMPONENT_PLANE_SEMANTIC_LABEL:
		structure.plane_semantic_label_list = {
			XR_TYPE_SPATIAL_COMPONENT_PLANE_SEMANTIC_LABEL_LIST_EXT,
			nullptr,
			capacity,
			static_cast<XrSpatialPlaneSemanticLabelEXT*>(data_ptr),
		};
		break;
	case COMPONENT_MARKER:
		structure.marker_list = {
			XR_TYPE_SPATIAL_COMPONENT_MARKER_LIST_EXT,
			nullptr,
			capacity,
			static_cast<XrSpatialMarkerDataEXT*>(data_ptr),
		};
		break;
	case COMPONENT_NONE:
	default:
		break;
	}
}

void OpenXRSpatialComponentData::set_component_kind(ComponentKind p_kind)
{
	if (component_kind == p_kind) {
		return;
	}
	_free_buffers();
	component_kind = p_kind;
	_init_structure_header();
}

void OpenXRSpatialComponentData::set_capacity(uint32_t p_capacity)
{
	if (component_kind == COMPONENT_NONE) {
		_free_buffers();
		return;
	}

	if (p_capacity == 0) {
		_free_buffers();
		_init_structure_header();
		return;
	}

	size_t elem_size = 0;
	switch (component_kind) {
	case COMPONENT_QUERY_RESULT:
		elem_size = sizeof(XrSpatialEntityIdEXT);
		break;
	case COMPONENT_BOUNDED_2D:
		elem_size = sizeof(XrSpatialBounded2DDataEXT);
		break;
	case COMPONENT_BOUNDED_3D:
		elem_size = sizeof(XrBoxf);
		break;
	case COMPONENT_PARENT:
		elem_size = sizeof(XrSpatialEntityIdEXT);
		break;
	case COMPONENT_MESH_2D:
	case COMPONENT_MESH_3D:
		elem_size = sizeof(XrSpatialMeshDataEXT);
		break;
	case COMPONENT_ANCHOR:
		elem_size = sizeof(XrPosef);
		break;
	case COMPONENT_PERSISTENCE:
		elem_size = sizeof(XrSpatialPersistenceDataEXT);
		break;
	case COMPONENT_PLANE_ALIGNMENT:
		elem_size = sizeof(XrSpatialPlaneAlignmentEXT);
		break;
	case COMPONENT_POLYGON_2D:
		elem_size = sizeof(XrSpatialPolygon2DDataEXT);
		break;
	case COMPONENT_PLANE_SEMANTIC_LABEL:
		elem_size = sizeof(XrSpatialPlaneSemanticLabelEXT);
		break;
	case COMPONENT_MARKER:
		elem_size = sizeof(XrSpatialMarkerDataEXT);
		break;
	case COMPONENT_NONE:
	default:
		return;
	}

	uint32_t old_capacity = capacity;
	data_ptr = memrealloc(data_ptr, elem_size * p_capacity);
	ERR_FAIL_NULL(data_ptr);
	if (p_capacity > old_capacity) {
		memset(static_cast<uint8_t*>(data_ptr) + elem_size * old_capacity, 0,
			elem_size * (p_capacity - old_capacity));
	}

	if (component_kind == COMPONENT_QUERY_RESULT) {
		entity_states_ptr = static_cast<XrSpatialEntityTrackingStateEXT*>(memrealloc(
			entity_states_ptr, sizeof(XrSpatialEntityTrackingStateEXT) * p_capacity));
		ERR_FAIL_NULL(entity_states_ptr);
		if (p_capacity > old_capacity) {
			memset(entity_states_ptr + old_capacity, 0,
				sizeof(XrSpatialEntityTrackingStateEXT) * (p_capacity - old_capacity));
		}
	}

	capacity = p_capacity;
	_init_structure_header();
}

int64_t OpenXRSpatialComponentData::_get_component_typegd() const
{
	return (int64_t)get_component_type();
}

XrSpatialComponentTypeEXT OpenXRSpatialComponentData::get_component_type() const
{
	switch (component_kind) {
	case COMPONENT_BOUNDED_2D:
		return XR_SPATIAL_COMPONENT_TYPE_BOUNDED_2D_EXT;
	case COMPONENT_BOUNDED_3D:
		return XR_SPATIAL_COMPONENT_TYPE_BOUNDED_3D_EXT;
	case COMPONENT_PARENT:
		return XR_SPATIAL_COMPONENT_TYPE_PARENT_EXT;
	case COMPONENT_MESH_2D:
		return XR_SPATIAL_COMPONENT_TYPE_MESH_2D_EXT;
	case COMPONENT_MESH_3D:
		return XR_SPATIAL_COMPONENT_TYPE_MESH_3D_EXT;
	case COMPONENT_ANCHOR:
		return XR_SPATIAL_COMPONENT_TYPE_ANCHOR_EXT;
	case COMPONENT_PERSISTENCE:
		return XR_SPATIAL_COMPONENT_TYPE_PERSISTENCE_EXT;
	case COMPONENT_PLANE_ALIGNMENT:
		return XR_SPATIAL_COMPONENT_TYPE_PLANE_ALIGNMENT_EXT;
	case COMPONENT_POLYGON_2D:
		return XR_SPATIAL_COMPONENT_TYPE_POLYGON_2D_EXT;
	case COMPONENT_PLANE_SEMANTIC_LABEL:
		return XR_SPATIAL_COMPONENT_TYPE_PLANE_SEMANTIC_LABEL_EXT;
	case COMPONENT_MARKER:
		return XR_SPATIAL_COMPONENT_TYPE_MARKER_EXT;
	case COMPONENT_QUERY_RESULT:
	case COMPONENT_NONE:
	default:
		return XR_SPATIAL_COMPONENT_TYPE_MAX_ENUM_EXT;
	}
}

void* OpenXRSpatialComponentData::get_structure_data(void* p_next)
{
	switch (component_kind) {
	case COMPONENT_QUERY_RESULT:
		structure.query_result.next = p_next;
		structure.query_result.entityIdCountOutput = 0;
		structure.query_result.entityStateCountOutput = 0;
		return &structure.query_result;
	case COMPONENT_BOUNDED_2D:
		structure.bounded2d_list.next = p_next;
		return &structure.bounded2d_list;
	case COMPONENT_BOUNDED_3D:
		structure.bounded3d_list.next = p_next;
		return &structure.bounded3d_list;
	case COMPONENT_PARENT:
		structure.parent_list.next = p_next;
		return &structure.parent_list;
	case COMPONENT_MESH_2D:
		structure.mesh2d_list.next = p_next;
		return &structure.mesh2d_list;
	case COMPONENT_MESH_3D:
		structure.mesh3d_list.next = p_next;
		return &structure.mesh3d_list;
	case COMPONENT_ANCHOR:
		structure.anchor_list.next = p_next;
		return &structure.anchor_list;
	case COMPONENT_PERSISTENCE:
		structure.persistence_list.next = p_next;
		return &structure.persistence_list;
	case COMPONENT_PLANE_ALIGNMENT:
		structure.plane_alignment_list.next = p_next;
		return &structure.plane_alignment_list;
	case COMPONENT_POLYGON_2D:
		structure.polygon2d_list.next = p_next;
		return &structure.polygon2d_list;
	case COMPONENT_PLANE_SEMANTIC_LABEL:
		structure.plane_semantic_label_list.next = p_next;
		return &structure.plane_semantic_label_list;
	case COMPONENT_MARKER:
		structure.marker_list.next = p_next;
		return &structure.marker_list;
	case COMPONENT_NONE:
	default:
		return nullptr;
	}
}

XrSpatialEntityIdEXT OpenXRSpatialComponentData::get_entity_id(int64_t p_index) const
{
	ERR_FAIL_COND_V(component_kind != COMPONENT_QUERY_RESULT, XR_NULL_ENTITY);
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, XR_NULL_ENTITY);

	return static_cast<const XrSpatialEntityIdEXT*>(data_ptr)[p_index];
}

uint64_t OpenXRSpatialComponentData::_get_entity_id(int64_t p_index) const
{
	return (uint64_t)get_entity_id(p_index);
}

XrSpatialEntityTrackingStateEXT OpenXRSpatialComponentData::get_entity_state(int64_t p_index) const
{
	ERR_FAIL_COND_V(
		component_kind != COMPONENT_QUERY_RESULT, XR_SPATIAL_ENTITY_TRACKING_STATE_MAX_ENUM_EXT);
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, XR_SPATIAL_ENTITY_TRACKING_STATE_MAX_ENUM_EXT);

	return entity_states_ptr[p_index];
}

String OpenXRSpatialComponentData::get_entity_tracking_state_name(
	XrSpatialEntityTrackingStateEXT p_tracking_state)
{
	XR_ENUM_SWITCH(XrSpatialEntityTrackingStateEXT, p_tracking_state)
}

Transform3D OpenXRSpatialComponentData::get_center_pose(int64_t p_index) const
{
	if (component_kind == COMPONENT_BOUNDED_2D) {
		return get_center_pose_2d(p_index);
	}
	if (component_kind == COMPONENT_BOUNDED_3D) {
		return get_center_pose_3d(p_index);
	}
	ERR_FAIL_V(Transform3D());
}

Transform3D OpenXRSpatialComponentData::get_center_pose_2d(int64_t p_index) const
{
	ERR_FAIL_COND_V(component_kind != COMPONENT_BOUNDED_2D, Transform3D());
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, Transform3D());
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), Transform3D());

	const XrSpatialBounded2DDataEXT* arr =
		static_cast<const XrSpatialBounded2DDataEXT*>(data_ptr);
	return OpenXRAPI::transform_from_pose(arr[p_index].center);
}

Transform3D OpenXRSpatialComponentData::get_center_pose_3d(int64_t p_index) const
{
	ERR_FAIL_COND_V(component_kind != COMPONENT_BOUNDED_3D, Transform3D());
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, Transform3D());
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), Transform3D());

	const XrBoxf* arr = static_cast<const XrBoxf*>(data_ptr);
	return OpenXRAPI::transform_from_pose(arr[p_index].center);
}

Vector2 OpenXRSpatialComponentData::get_size_2d(int64_t p_index) const
{
	ERR_FAIL_COND_V(component_kind != COMPONENT_BOUNDED_2D, Vector2());
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, Vector2());
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), Vector2());

	const XrExtent2Df& extents =
		static_cast<const XrSpatialBounded2DDataEXT*>(data_ptr)[p_index].extents;
	return Vector2(extents.width, extents.height);
}

Vector3 OpenXRSpatialComponentData::get_size_3d(int64_t p_index) const
{
	ERR_FAIL_COND_V(component_kind != COMPONENT_BOUNDED_3D, Vector3());
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, Vector3());
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), Vector3());

	const XrExtent3Df& extents = static_cast<const XrBoxf*>(data_ptr)[p_index].extents;
	return Vector3(extents.width, extents.height, extents.depth);
}

RID OpenXRSpatialComponentData::get_parent(int64_t p_index) const
{
	ERR_FAIL_COND_V(component_kind != COMPONENT_PARENT, RID());
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, RID());
	ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), RID());

	const XrSpatialEntityIdEXT* arr = static_cast<const XrSpatialEntityIdEXT*>(data_ptr);
	return OpenXRSpatialEntityExtension::find_spatial_entity(arr[p_index]);
}

Transform3D OpenXRSpatialComponentData::get_transform(int64_t p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, Transform3D());
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), Transform3D());

	switch (component_kind) {
	case COMPONENT_MESH_2D:
	case COMPONENT_MESH_3D:
		return OpenXRAPI::transform_from_pose(
			static_cast<const XrSpatialMeshDataEXT*>(data_ptr)[p_index].origin);
	case COMPONENT_POLYGON_2D:
		return OpenXRAPI::transform_from_pose(
			static_cast<const XrSpatialPolygon2DDataEXT*>(data_ptr)[p_index].origin);
	default:
		ERR_FAIL_V(Transform3D());
	}
}

PackedVector2Array OpenXRSpatialComponentData::get_vertices(RID p_snapshot, int64_t p_index) const
{
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, PackedVector2Array());

	const XrSpatialBufferEXT* buffer = nullptr;
	if (component_kind == COMPONENT_MESH_2D) {
		buffer = &static_cast<const XrSpatialMeshDataEXT*>(data_ptr)[p_index].vertexBuffer;
	}
	else if (component_kind == COMPONENT_POLYGON_2D) {
		buffer = &static_cast<const XrSpatialPolygon2DDataEXT*>(data_ptr)[p_index].vertexBuffer;
	}
	else {
		ERR_FAIL_V(PackedVector2Array());
	}

	if (buffer->bufferId == XR_NULL_SPATIAL_BUFFER_ID_EXT) {
		return PackedVector2Array();
	}

	ERR_FAIL_COND_V(
		buffer->bufferType != XR_SPATIAL_BUFFER_TYPE_VECTOR2F_EXT, PackedVector2Array());
	ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), PackedVector2Array());

	return OpenXRSpatialEntityExtension::get_vector2_buffer(p_snapshot, buffer->bufferId);
}

PackedInt32Array OpenXRSpatialComponentData::get_indices(RID p_snapshot, int64_t p_index) const
{
	ERR_FAIL_COND_V(component_kind != COMPONENT_MESH_2D, PackedInt32Array());
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, PackedInt32Array());

	const XrSpatialBufferEXT& buffer =
		static_cast<const XrSpatialMeshDataEXT*>(data_ptr)[p_index].indexBuffer;
	if (buffer.bufferId == XR_NULL_SPATIAL_BUFFER_ID_EXT) {
		return PackedInt32Array();
	}

	ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), PackedInt32Array());

	PackedInt32Array ret;

	switch (buffer.bufferType) {
	case XR_SPATIAL_BUFFER_TYPE_UINT8_EXT: {
		PackedByteArray data =
			OpenXRSpatialEntityExtension::get_uint8_buffer(p_snapshot, buffer.bufferId);

		ret.resize(data.size());
		int count = ret.size();
		int32_t* ptr = ret.ptrw();
		for (int i = 0; i < count; i++) {
			ptr[i] = data[i];
		}
	} break;
	case XR_SPATIAL_BUFFER_TYPE_UINT16_EXT: {
		Vector<uint16_t> data =
			OpenXRSpatialEntityExtension::get_uint16_buffer(p_snapshot, buffer.bufferId);

		ret.resize(data.size());
		int count = ret.size();
		int32_t* ptr = ret.ptrw();
		for (int i = 0; i < count; i++) {
			ptr[i] = data[i];
		}
	} break;
	case XR_SPATIAL_BUFFER_TYPE_UINT32_EXT: {
		Vector<uint32_t> data =
			OpenXRSpatialEntityExtension::get_uint32_buffer(p_snapshot, buffer.bufferId);

		ret.resize(data.size());
		int count = ret.size();
		int32_t* ptr = ret.ptrw();
		for (int i = 0; i < count; i++) {
			ptr[i] = data[i];
		}
	} break;
	default: {
		ERR_FAIL_V_MSG(PackedInt32Array(), "OpenXR: Unsupported buffer type for indices.");
	} break;
	}

	return ret;
}

Ref<Mesh> OpenXRSpatialComponentData::get_mesh(int64_t p_index) const
{
	ERR_FAIL_COND_V(component_kind != COMPONENT_MESH_3D, nullptr);
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, nullptr);

	// TODO implement, need to convert mesh data to Godot mesh resource
	return nullptr;
}

Transform3D OpenXRSpatialComponentData::get_entity_pose(int64_t p_index) const
{
	ERR_FAIL_COND_V(component_kind != COMPONENT_ANCHOR, Transform3D());
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, Transform3D());
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), Transform3D());

	return OpenXRAPI::transform_from_pose(static_cast<const XrPosef*>(data_ptr)[p_index]);
}

XrUuid OpenXRSpatialComponentData::get_persistent_uuid(int64_t p_index) const
{
	XrUuid null_uuid = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	ERR_FAIL_COND_V(component_kind != COMPONENT_PERSISTENCE, null_uuid);
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, null_uuid);

	return static_cast<const XrSpatialPersistenceDataEXT*>(data_ptr)[p_index].persistUuid;
}

String OpenXRSpatialComponentData::_get_persistent_uuid(int64_t p_index) const
{
	return OpenXRUtil::string_from_xruuid(get_persistent_uuid(p_index));
}

XrSpatialPersistenceStateEXT OpenXRSpatialComponentData::get_persistent_state(int64_t p_index) const
{
	ERR_FAIL_COND_V(
		component_kind != COMPONENT_PERSISTENCE, XR_SPATIAL_PERSISTENCE_STATE_MAX_ENUM_EXT);
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, XR_SPATIAL_PERSISTENCE_STATE_MAX_ENUM_EXT);

	return static_cast<const XrSpatialPersistenceDataEXT*>(data_ptr)[p_index].persistState;
}

uint64_t OpenXRSpatialComponentData::_get_persistent_state(int64_t p_index) const
{
	return (uint64_t)get_persistent_state(p_index);
}

String OpenXRSpatialComponentData::get_persistence_state_name(XrSpatialPersistenceStateEXT p_state)
{
	XR_ENUM_SWITCH(XrSpatialPersistenceStateEXT, p_state)
}

XrSpatialPlaneAlignmentEXT OpenXRSpatialComponentData::get_plane_alignment(int64_t p_index) const
{
	ERR_FAIL_COND_V(
		component_kind != COMPONENT_PLANE_ALIGNMENT, XR_SPATIAL_PLANE_ALIGNMENT_MAX_ENUM_EXT);
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, XR_SPATIAL_PLANE_ALIGNMENT_MAX_ENUM_EXT);

	return static_cast<const XrSpatialPlaneAlignmentEXT*>(data_ptr)[p_index];
}

OpenXRSpatialComponentData::PlaneAlignment OpenXRSpatialComponentData::_get_plane_alignment(
	int64_t p_index) const
{
	return (PlaneAlignment)get_plane_alignment(p_index);
}

XrSpatialPlaneSemanticLabelEXT OpenXRSpatialComponentData::get_plane_semantic_label(
	int64_t p_index) const
{
	ERR_FAIL_COND_V(component_kind != COMPONENT_PLANE_SEMANTIC_LABEL,
		XR_SPATIAL_PLANE_SEMANTIC_LABEL_MAX_ENUM_EXT);
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, XR_SPATIAL_PLANE_SEMANTIC_LABEL_MAX_ENUM_EXT);

	return static_cast<const XrSpatialPlaneSemanticLabelEXT*>(data_ptr)[p_index];
}

OpenXRSpatialComponentData::PlaneSemanticLabel
OpenXRSpatialComponentData::_get_plane_semantic_label(int64_t p_index) const
{
	return (PlaneSemanticLabel)get_plane_semantic_label(p_index);
}

OpenXRSpatialComponentData::MarkerType OpenXRSpatialComponentData::get_marker_type(
	int64_t p_index) const
{
	ERR_FAIL_COND_V(component_kind != COMPONENT_MARKER, MARKER_TYPE_UNKNOWN);
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, MARKER_TYPE_UNKNOWN);

	switch (static_cast<const XrSpatialMarkerDataEXT*>(data_ptr)[p_index].capability) {
	case XR_SPATIAL_CAPABILITY_MARKER_TRACKING_QR_CODE_EXT:
		return MARKER_TYPE_QRCODE;
	case XR_SPATIAL_CAPABILITY_MARKER_TRACKING_MICRO_QR_CODE_EXT:
		return MARKER_TYPE_MICRO_QRCODE;
	case XR_SPATIAL_CAPABILITY_MARKER_TRACKING_ARUCO_MARKER_EXT:
		return MARKER_TYPE_ARUCO;
	case XR_SPATIAL_CAPABILITY_MARKER_TRACKING_APRIL_TAG_EXT:
		return MARKER_TYPE_APRIL_TAG;
	default:
		return MARKER_TYPE_UNKNOWN;
	}
}

uint32_t OpenXRSpatialComponentData::get_marker_id(int64_t p_index) const
{
	ERR_FAIL_COND_V(component_kind != COMPONENT_MARKER, 0);
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, 0);

	return static_cast<const XrSpatialMarkerDataEXT*>(data_ptr)[p_index].markerId;
}

XrSpatialBufferEXT OpenXRSpatialComponentData::get_marker_data(int64_t p_index) const
{
	XrSpatialBufferEXT null_buffer = {
		XR_NULL_SPATIAL_BUFFER_ID_EXT, XR_SPATIAL_BUFFER_TYPE_UNKNOWN_EXT};

	ERR_FAIL_COND_V(component_kind != COMPONENT_MARKER, null_buffer);
	ERR_FAIL_INDEX_V(p_index, (int64_t)capacity, null_buffer);

	return static_cast<const XrSpatialMarkerDataEXT*>(data_ptr)[p_index].data;
}

////////////////////////////////////////////////////////////////////////////
// OpenXRSpatialEntityTracker

OpenXRSpatialEntityTracker::OpenXRSpatialEntityTracker(EntityType p_entity_type)
	: entity_type(p_entity_type)
{
	xr_tracker.instantiate(XRServer::TrackerType::TRACKER_ANCHOR);
}

OpenXRSpatialEntityTracker::~OpenXRSpatialEntityTracker()
{
	if (spatial_entity.is_valid()) {
		if (OpenXRSpatialEntityExtension::is_initialized()) {
			OpenXRSpatialEntityExtension::free_spatial_entity(spatial_entity);
			spatial_entity = RID();
		}
	}
}

void OpenXRSpatialEntityTracker::set_spatial_context(const RID& p_spatial_context)
{
	// Trackers shouldn't be switching spatial contexts; always create a new tracker for new
	// contexts
	ERR_FAIL_COND(spatial_context.is_valid() && spatial_context != p_spatial_context);
	spatial_context = p_spatial_context;
}

RID OpenXRSpatialEntityTracker::get_spatial_context() const { return spatial_context; }

void OpenXRSpatialEntityTracker::set_entity(const RID& p_entity)
{
	if (spatial_entity.is_valid()) {
		if (spatial_entity == p_entity) {
			return;
		}

		if (OpenXRSpatialEntityExtension::is_initialized()) {
			OpenXRSpatialEntityExtension::free_spatial_entity(spatial_entity);
			spatial_entity = RID();
		}
	}

	spatial_entity = p_entity;

	if (p_entity.is_valid()) {
		ERR_FAIL_COND(!OpenXRSpatialEntityExtension::is_initialized());

		XrSpatialEntityIdEXT entity_id = OpenXRSpatialEntityExtension::get_spatial_entity_id(p_entity);

		String tracker_name = String("openxr/spatial_entity/") + String::num_int64(entity_id);
		set_tracker_name(tracker_name);
	}
	else {
		set_tracker_name("openxr/spatial_entity/null");
	}
}

RID OpenXRSpatialEntityTracker::get_entity() const { return spatial_entity; }

void OpenXRSpatialEntityTracker::set_spatial_tracking_state(
	const XrSpatialEntityTrackingStateEXT p_state)
{
	if (spatial_tracking_state != p_state) {
		spatial_tracking_state = p_state;
	}
}

void OpenXRSpatialEntityTracker::_set_spatial_tracking_state(const EntityTrackingState p_state)
{
	set_spatial_tracking_state((XrSpatialEntityTrackingStateEXT)p_state);
}

XrSpatialEntityTrackingStateEXT OpenXRSpatialEntityTracker::get_spatial_tracking_state() const
{
	return spatial_tracking_state;
}

OpenXRSpatialEntityTracker::EntityTrackingState
OpenXRSpatialEntityTracker::_get_spatial_tracking_state() const
{
	return (EntityTrackingState)get_spatial_tracking_state();
}

void OpenXRSpatialEntityTracker::add_next(Ref<OpenXRStructureBase> p_next)
{
	// Prepend p_next to next
	if (p_next.is_valid()) {
		if (next.is_valid()) {
			p_next->set_next(next);
		}

		next = p_next;
	}
}

void OpenXRSpatialEntityTracker::remove_next(Ref<OpenXRStructureBase> p_next)
{
	if (p_next.is_null()) {
		return;
	}

	Ref<OpenXRStructureBase> prev;
	for (Ref<OpenXRStructureBase> n = next; n.is_valid(); n = n->get_next()) {
		if (n == p_next) {
			if (prev.is_null()) {
				next = p_next->get_next();
			}
			else {
				prev->set_next(p_next->get_next());
			}

			p_next->set_next(nullptr);
			break;
		}

		prev = n;
	}
}

Ref<OpenXRStructureBase> OpenXRSpatialEntityTracker::get_next() const { return next; }

bool OpenXRSpatialEntityTracker::has_uuid() const
{
	for (int i = 0; i < XR_UUID_SIZE; i++) {
		if (uuid.data[i] != 0) {
			return true;
		}
	}

	return false;
}

XrUuid OpenXRSpatialEntityTracker::get_uuid() const { return uuid; }

void OpenXRSpatialEntityTracker::set_uuid(const XrUuid& p_uuid)
{
	if (uuid_is_equal(uuid, p_uuid)) {
		return;
	}

	uuid = p_uuid;
}

String OpenXRSpatialEntityTracker::_get_uuid() const { return OpenXRUtil::string_from_xruuid(uuid); }

void OpenXRSpatialEntityTracker::_set_uuid(const String& p_uuid)
{
	set_uuid(OpenXRUtil::xruuid_from_string(p_uuid));
}

bool OpenXRSpatialEntityTracker::uuid_is_equal(const XrUuid& p_a, const XrUuid& p_b)
{
	for (int i = 0; i < XR_UUID_SIZE; i++) {
		if (p_a.data[i] != p_b.data[i]) {
			return false;
		}
	}

	return true;
}

void OpenXRSpatialEntityTracker::set_bounds_size(const Vector2& p_bounds_size)
{
	if (entity_type == ENTITY_PLANE) {
		if (Math::abs(bounds_size.x - p_bounds_size.x) > 0.001 ||
			Math::abs(bounds_size.y - p_bounds_size.y) > 0.001) {
			bounds_size = p_bounds_size;

			if (!mesh.has_mesh_data) {
				// Bounds changing only effects mesh data if we don't have polygon data.
				clear_mesh_data();
			}
		}
	}
	else {
		bounds_size = p_bounds_size;
	}
}

Vector2 OpenXRSpatialEntityTracker::get_bounds_size() const { return bounds_size; }

void OpenXRSpatialEntityTracker::set_plane_alignment(
	OpenXRSpatialComponentData::PlaneAlignment p_plane_alignment)
{
	if (plane_alignment != p_plane_alignment) {
		plane_alignment = p_plane_alignment;
	}
}

OpenXRSpatialComponentData::PlaneAlignment OpenXRSpatialEntityTracker::get_plane_alignment() const
{
	return plane_alignment;
}

void OpenXRSpatialEntityTracker::set_plane_label(const String& p_plane_label)
{
	if (plane_label != p_plane_label) {
		plane_label = p_plane_label;

		// Also copy to description, should do something nicer here.
		set_tracker_desc(plane_label);
	}
}

String OpenXRSpatialEntityTracker::get_plane_label() const { return plane_label; }

void OpenXRSpatialEntityTracker::set_mesh_data(const Transform3D& p_origin,
	const PackedVector2Array& p_vertices, const PackedInt32Array& p_indices)
{
	if (p_vertices.size() < 3) {
		if (mesh.has_mesh_data) {
			clear_mesh_data();
		}
	}
	else {
		bool has_changed = !mesh.has_mesh_data;

		mesh.has_mesh_data = true;
		mesh.origin = p_origin;

		if (mesh.vertices.size() != p_vertices.size()) {
			has_changed = true;
		}
		else {
			// Compare the vertices with a bit of margin, we ignore small jittering on vertices.
			for (uint32_t i = 0; i < p_vertices.size() && !has_changed; i++) {
				const Vector2& a = p_vertices[i];
				const Vector2& b = mesh.vertices[i];
				has_changed = (Math::abs(a.x - b.x) > 0.001) || (Math::abs(a.y - b.y) > 0.001);
			}
		}
		if (has_changed) {
			mesh.vertices = p_vertices;
		}

		if (p_indices.is_empty()) {
			// Assume polygon, turn into triangle strip...
			int count = (p_vertices.size() - 2) * 3;

			if (has_changed || mesh.indices.size() != count) {
				has_changed = true;

				int offset = 1;
				mesh.indices.resize(count);
				int32_t* idx = mesh.indices.ptrw();
				for (int i = 0; i < count; i += 3) {
					idx[i + 0] = 0;
					idx[i + 2] = offset++;
					idx[i + 1] = offset;
				}
			}
		}
		else {
			if (mesh.indices.size() != p_indices.size()) {
				has_changed = true;
			}
			else {
				for (uint32_t i = 0; i < p_indices.size() && !has_changed; i++) {
					has_changed = mesh.indices[i] != p_indices[i];
				}
			}
			if (has_changed) {
				mesh.indices = p_indices;
			}
		}

		if (has_changed) {
			mesh.mesh.unref();
#ifndef PHYSICS_3D_DISABLED
			mesh.shape3d.unref();
#endif
		}
	}
}

void OpenXRSpatialEntityTracker::clear_mesh_data()
{
	mesh.mesh.unref();
#ifndef PHYSICS_3D_DISABLED
	mesh.shape3d.unref();
#endif

	if (mesh.has_mesh_data) {
		mesh.has_mesh_data = false;
		mesh.origin = Transform3D();
		mesh.vertices.clear();
		mesh.indices.clear();
	}
}

Transform3D OpenXRSpatialEntityTracker::get_mesh_offset() const
{
	Transform3D offset;

	if (mesh.has_mesh_data) {
		offset = mesh.origin;

		Ref<XRPose> pose = get_pose(SNAME("default"));
		if (pose.is_valid()) {
			offset = pose->get_transform().inverse() * offset;
		}

		if (XRServer::is_initialized()) {
			offset.origin *= XRServer::get_world_scale();
		}
	}

	return offset;
}

#ifndef PHYSICS_3D_DISABLED
Ref<Shape3D> OpenXRSpatialEntityTracker::get_shape(real_t p_thickness)
{
	if (mesh.shape3d.is_valid()) {
		return mesh.shape3d;
	}

	if (mesh.has_mesh_data) {
		Ref<Shape3D> shape;
		Vector<Vector3> faces;

		int isize = mesh.indices.size();
		const Vector2* vr = mesh.vertices.ptr();
		const int32_t* ir = mesh.indices.ptr();

		HashMap<Edge, int, Edge> edge_counts;
		for (int i = 0; i < isize; i += 3) {
			for (int j = 0; j < 3; j++) {
				Edge e(ir[i + j], ir[i + ((j + 1) % 3)]);
				edge_counts[e]++;
			}
		}

		thread_local LocalVector<Edge> outer_edges;
		outer_edges.clear();
		for (const KeyValue<Edge, int>& e : edge_counts) {
			if (e.value > 1) {
				outer_edges.push_back(e.key);
			}
		}

		faces.resize(2 * isize + 6 * outer_edges.size());
		Vector3* write = faces.ptrw();

		for (int i = 0; i < isize; i += 3) {
			Vector3 a = Vector3(vr[ir[i]].x, vr[ir[i]].y, 0.0);
			Vector3 b = Vector3(vr[ir[i + 1]].x, vr[ir[i + 1]].y, 0.0);
			Vector3 c = Vector3(vr[ir[i + 2]].x, vr[ir[i + 2]].y, 0.0);

			*write++ = a;
			*write++ = b;
			*write++ = c;

			a.z = -p_thickness;
			b.z = -p_thickness;
			c.z = -p_thickness;

			*write++ = a;
			*write++ = c;
			*write++ = b;
		}

		for (const Edge& edge : outer_edges) {
			Vector3 a = Vector3(vr[edge.a].x, vr[edge.a].y, 0.0);
			Vector3 b = Vector3(vr[edge.b].x, vr[edge.b].y, 0.0);
			Vector3 c = b + Vector3(0.0, 0.0, -p_thickness);
			Vector3 d = a + Vector3(0.0, 0.0, -p_thickness);

			*write++ = a;
			*write++ = b;
			*write++ = c;

			*write++ = a;
			*write++ = c;
			*write++ = d;
		}

		shape = memnew(Shape3D(Shape3D::SHAPE_CONCAVE_POLYGON));
		shape->set_faces(faces);

		mesh.shape3d = shape;
	}
	else if (bounds_size.x > 0.0 && bounds_size.y > 0.0) {
		Ref<Shape3D> box_shape = memnew(Shape3D(Shape3D::SHAPE_BOX));
		box_shape->set_size(Vector3(bounds_size.x, bounds_size.y, p_thickness));

		mesh.shape3d = box_shape;
	}

	return mesh.shape3d;
}
#endif // PHYSICS_3D_DISABLED

void OpenXRSpatialEntityTracker::set_marker_type(
	OpenXRSpatialComponentData::MarkerType p_marker_type)
{
	marker_type = p_marker_type;
}

OpenXRSpatialComponentData::MarkerType OpenXRSpatialEntityTracker::get_marker_type() const
{
	return marker_type;
}

void OpenXRSpatialEntityTracker::set_marker_id(uint32_t p_id) { marker_id = p_id; }

uint32_t OpenXRSpatialEntityTracker::get_marker_id() const { return marker_id; }


////////////////////////////////////////////////////////////////////////////
// OpenXRSpatialEntityExtension

void OpenXRSpatialEntityExtension::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXRSpatialEntityExtension::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

HashMap<String, bool*> OpenXRSpatialEntityExtension::get_requested_extensions(XrVersion p_version)
{
	HashMap<String, bool*> request_extensions;

	if (GLOBAL_GET_CACHED(bool, "xr/openxr/extensions/spatial_entity/enabled")) {
		request_extensions[XR_EXT_SPATIAL_ENTITY_EXTENSION_NAME] = &data->spatial_entity_ext;
	}

	return request_extensions;
}

void OpenXRSpatialEntityExtension::on_instance_created(const XrInstance p_instance)
{
	if (data->spatial_entity_ext) {
		EXT_INIT_XR_FUNC(xrEnumerateSpatialCapabilitiesEXT);
		EXT_INIT_XR_FUNC(xrEnumerateSpatialCapabilityComponentTypesEXT);
		EXT_INIT_XR_FUNC(xrEnumerateSpatialCapabilityFeaturesEXT);
		EXT_INIT_XR_FUNC(xrCreateSpatialContextAsyncEXT);
		EXT_INIT_XR_FUNC(xrCreateSpatialContextCompleteEXT);
		EXT_INIT_XR_FUNC(xrDestroySpatialContextEXT);
		EXT_INIT_XR_FUNC(xrCreateSpatialDiscoverySnapshotAsyncEXT);
		EXT_INIT_XR_FUNC(xrCreateSpatialDiscoverySnapshotCompleteEXT);
		EXT_INIT_XR_FUNC(xrQuerySpatialComponentDataEXT);
		EXT_INIT_XR_FUNC(xrDestroySpatialSnapshotEXT);
		EXT_INIT_XR_FUNC(xrCreateSpatialEntityFromIdEXT);
		EXT_INIT_XR_FUNC(xrDestroySpatialEntityEXT);
		EXT_INIT_XR_FUNC(xrCreateSpatialUpdateSnapshotEXT);
		EXT_INIT_XR_FUNC(xrGetSpatialBufferStringEXT);
		EXT_INIT_XR_FUNC(xrGetSpatialBufferUint8EXT);
		EXT_INIT_XR_FUNC(xrGetSpatialBufferUint16EXT);
		EXT_INIT_XR_FUNC(xrGetSpatialBufferUint32EXT);
		EXT_INIT_XR_FUNC(xrGetSpatialBufferFloatEXT);
		EXT_INIT_XR_FUNC(xrGetSpatialBufferVector2fEXT);
		EXT_INIT_XR_FUNC(xrGetSpatialBufferVector3fEXT);
	}
}

void OpenXRSpatialEntityExtension::on_instance_destroyed()
{
	data->supported_capabilities.clear();
	data->capabilities_load_state = 0;

	xrEnumerateSpatialCapabilitiesEXT_ptr = nullptr;
	xrEnumerateSpatialCapabilityComponentTypesEXT_ptr = nullptr;
	xrEnumerateSpatialCapabilityFeaturesEXT_ptr = nullptr;
	xrCreateSpatialContextAsyncEXT_ptr = nullptr;
	xrCreateSpatialContextCompleteEXT_ptr = nullptr;
	xrDestroySpatialContextEXT_ptr = nullptr;
	xrCreateSpatialDiscoverySnapshotAsyncEXT_ptr = nullptr;
	xrCreateSpatialDiscoverySnapshotCompleteEXT_ptr = nullptr;
	xrQuerySpatialComponentDataEXT_ptr = nullptr;
	xrDestroySpatialSnapshotEXT_ptr = nullptr;
	xrCreateSpatialEntityFromIdEXT_ptr = nullptr;
	xrDestroySpatialEntityEXT_ptr = nullptr;
	xrCreateSpatialUpdateSnapshotEXT_ptr = nullptr;
	xrGetSpatialBufferStringEXT_ptr = nullptr;
	xrGetSpatialBufferUint8EXT_ptr = nullptr;
	xrGetSpatialBufferUint16EXT_ptr = nullptr;
	xrGetSpatialBufferUint32EXT_ptr = nullptr;
	xrGetSpatialBufferFloatEXT_ptr = nullptr;
	xrGetSpatialBufferVector2fEXT_ptr = nullptr;
	xrGetSpatialBufferVector3fEXT_ptr = nullptr;
}

bool OpenXRSpatialEntityExtension::get_active()
{
	return data != nullptr && data->spatial_entity_ext;
}

bool OpenXRSpatialEntityExtension::_load_capabilities()
{
	if (data->capabilities_load_state == 0) {
		if (!data->spatial_entity_ext) {
			return false;
		}

		ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), false);

		XrInstance instance = OpenXRAPI::get_instance();
		ERR_FAIL_COND_V(instance == XR_NULL_HANDLE, false);
		XrSystemId system_id = OpenXRAPI::get_system_id();
		ERR_FAIL_COND_V(system_id == 0, false);

		data->capabilities_load_state = 2;

		Vector<XrSpatialCapabilityEXT> capabilities;
		uint32_t capability_size = 0;
		XrResult result =
			xrEnumerateSpatialCapabilitiesEXT(instance, system_id, 0, &capability_size, nullptr);
		if (XR_FAILED(result)) {
			ERR_FAIL_V_MSG(false, "OpenXR: Failed to get spatial entity capability count [" +
									  OpenXRAPI::get_error_string(result) + "]");
		}

		if (capability_size > 0) {
			capabilities.resize(capability_size);
			result = xrEnumerateSpatialCapabilitiesEXT(
				instance, system_id, capabilities.size(), &capability_size, capabilities.ptrw());
			if (XR_FAILED(result)) {
				ERR_FAIL_V_MSG(false, "OpenXR: Failed to get spatial entity capabilities [" +
										  OpenXRAPI::get_error_string(result) + "]");
			}

			for (const XrSpatialCapabilityEXT& capability : capabilities) {
				print_verbose("OpenXR: Found spatial entity capability " +
							  get_spatial_capability_name(capability) + ".");

				SpatialEntityCapabality& spatial_entity_capability =
					data->supported_capabilities[capability];

				XrSpatialCapabilityComponentTypesEXT component_types = {
					XR_TYPE_SPATIAL_CAPABILITY_COMPONENT_TYPES_EXT,
					nullptr,
					0,
					0,
					nullptr,
				};
				result = xrEnumerateSpatialCapabilityComponentTypesEXT(
					instance, system_id, capability, &component_types);
				if (XR_FAILED(result)) {
					ERR_PRINT("OpenXR: Failed to get spatial entity component type count [" +
							  OpenXRAPI::get_error_string(result) + "]");
				}
				else if (component_types.componentTypeCountOutput > 0) {
					spatial_entity_capability.component_types.resize(
						component_types.componentTypeCountOutput);
					component_types.componentTypeCapacityInput =
						spatial_entity_capability.component_types.size();
					component_types.componentTypeCountOutput = 0;
					component_types.componentTypes =
						spatial_entity_capability.component_types.ptrw();
					result = xrEnumerateSpatialCapabilityComponentTypesEXT(
						instance, system_id, capability, &component_types);
					if (XR_FAILED(result)) {
						ERR_PRINT("OpenXR: Failed to get spatial entity component types [" +
								  OpenXRAPI::get_error_string(result) + "]");
					}
					else if (is_print_verbose_enabled()) {
						for (const XrSpatialComponentTypeEXT& component_type :
							spatial_entity_capability.component_types) {
							print_verbose("- component type " +
										  get_spatial_component_type_name(component_type));
						}
					}
				}

				result = xrEnumerateSpatialCapabilityFeaturesEXT(
					instance, system_id, capability, 0, &capability_size, nullptr);
				if (XR_FAILED(result)) {
					ERR_PRINT("OpenXR: Failed to get spatial entity feature count [" +
							  OpenXRAPI::get_error_string(result) + "]");
				}
				else if (capability_size > 0) {
					spatial_entity_capability.features.resize(capability_size);
					result = xrEnumerateSpatialCapabilityFeaturesEXT(instance, system_id,
						capability, spatial_entity_capability.features.size(), &capability_size,
						spatial_entity_capability.features.ptrw());
					if (XR_FAILED(result)) {
						ERR_PRINT("OpenXR: Failed to get spatial entity features [" +
								  OpenXRAPI::get_error_string(result) + "]");
					}
					else if (is_print_verbose_enabled()) {
						for (const XrSpatialCapabilityFeatureEXT& feature :
							spatial_entity_capability.features) {
							print_verbose("- feature " + get_spatial_feature_name(feature));
						}
					}
				}
			}
		}

		data->capabilities_load_state = 1;
	}

	return data->capabilities_load_state == 1;
}

bool OpenXRSpatialEntityExtension::supports_capability(XrSpatialCapabilityEXT p_capability)
{
	if (!_load_capabilities()) {
		return false;
	}

	return data->supported_capabilities.has(p_capability);
}

bool OpenXRSpatialEntityExtension::_supports_capability(Capability p_capability)
{
	return supports_capability((XrSpatialCapabilityEXT)p_capability);
}

bool OpenXRSpatialEntityExtension::supports_component_type(
	XrSpatialCapabilityEXT p_capability, XrSpatialComponentTypeEXT p_component_type)
{
	if (!_load_capabilities()) {
		return false;
	}

	if (data->supported_capabilities.has(p_capability)) {
		return data->supported_capabilities[p_capability].component_types.has(p_component_type);
	}
	return false;
}

bool OpenXRSpatialEntityExtension::_supports_component_type(
	Capability p_capability, ComponentType p_component_type)
{
	return supports_component_type(
		(XrSpatialCapabilityEXT)p_capability, (XrSpatialComponentTypeEXT)p_component_type);
}

bool OpenXRSpatialEntityExtension::get_spatial_context_ready(RID p_spatial_context)
{
	SpatialContextData* context_data = data->spatial_context_owner.get_or_null(p_spatial_context);
	ERR_FAIL_NULL_V(context_data, false);

	return context_data->spatial_context != XR_NULL_HANDLE;
}

void OpenXRSpatialEntityExtension::free_spatial_context(RID p_spatial_context)
{
	SpatialContextData* context_data = data->spatial_context_owner.get_or_null(p_spatial_context);
	ERR_FAIL_NULL(context_data);

	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	if (context_data->spatial_context != XR_NULL_HANDLE) {
		XrResult result = xrDestroySpatialContextEXT(context_data->spatial_context);
		if (XR_FAILED(result)) {
			WARN_PRINT("OpenXR: Failed to destroy the spatial context [" +
					   OpenXRAPI::get_error_string(result) + "]");
		}
		context_data->spatial_context = XR_NULL_HANDLE;

		data->spatial_context_owner.free(p_spatial_context);
	}
}

XrSpatialContextEXT OpenXRSpatialEntityExtension::get_spatial_context_handle(
	RID p_spatial_context)
{
	SpatialContextData* context_data = data->spatial_context_owner.get_or_null(p_spatial_context);
	ERR_FAIL_NULL_V(context_data, XR_NULL_HANDLE);

	return context_data->spatial_context;
}

uint64_t OpenXRSpatialEntityExtension::_get_spatial_context_handle(RID p_spatial_context)
{
	return (uint64_t)get_spatial_context_handle(p_spatial_context);
}

RID OpenXRSpatialEntityExtension::update_spatial_entities(RID p_spatial_context,
	const LocalVector<RID>& p_entities,
	const LocalVector<XrSpatialComponentTypeEXT>& p_component_types,
	Ref<OpenXRStructureBase> p_next)
{
	if (!get_active()) {
		return RID();
	}

	ERR_FAIL_COND_V(p_entities.is_empty(), RID());

	SpatialContextData* context_data = data->spatial_context_owner.get_or_null(p_spatial_context);
	ERR_FAIL_NULL_V(context_data, RID());

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), RID());

	thread_local LocalVector<XrSpatialEntityEXT> entities;

	entities.resize(p_entities.size());
	XrSpatialEntityEXT* ptr = entities.ptr();
	for (const RID& rid : p_entities) {
		SpatialEntityData* entity_data = data->spatial_entity_owner.get_or_null(rid);
		*ptr = entity_data ? entity_data->entity : XR_NULL_HANDLE;
		ptr++;
	}

	void* next = nullptr;
	if (p_next.is_valid()) {
		next = p_next->get_header(next);
	}

	SpatialSnapshotData spatial_snapshot_data;
	spatial_snapshot_data.spatial_context = p_spatial_context;

	XrSpatialUpdateSnapshotCreateInfoEXT create_info = {
		XR_TYPE_SPATIAL_UPDATE_SNAPSHOT_CREATE_INFO_EXT,
		next,
		(uint32_t)entities.size(),
		entities.ptr(),
		(uint32_t)p_component_types.size(),
		p_component_types.is_empty() ? nullptr : p_component_types.ptr(),
		OpenXRAPI::get_play_space(),
		OpenXRAPI::get_predicted_display_time(),
	};
	XrResult result = xrCreateSpatialUpdateSnapshotEXT(
		context_data->spatial_context, &create_info, &spatial_snapshot_data.spatial_snapshot);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(RID(), "OpenXR: Failed to create update snapshot [" +
								  OpenXRAPI::get_error_string(result) + "]");
	}

	return data->spatial_snapshot_owner.make_rid(spatial_snapshot_data);
}

void OpenXRSpatialEntityExtension::free_spatial_snapshot(RID p_spatial_snapshot)
{
	SpatialSnapshotData* snapshot_data =
		data->spatial_snapshot_owner.get_or_null(p_spatial_snapshot);
	ERR_FAIL_NULL(snapshot_data);

	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	if (snapshot_data->spatial_snapshot != XR_NULL_HANDLE) {
		XrResult result = xrDestroySpatialSnapshotEXT(snapshot_data->spatial_snapshot);
		if (XR_FAILED(result)) {
			WARN_PRINT("OpenXR: Failed to destroy the spatial snapshot [" +
					   OpenXRAPI::get_error_string(result) + "]");
		}
		snapshot_data->spatial_snapshot = XR_NULL_HANDLE;
	}

	data->spatial_snapshot_owner.free(p_spatial_snapshot);
}

XrSpatialSnapshotEXT OpenXRSpatialEntityExtension::get_spatial_snapshot_handle(
	RID p_spatial_snapshot)
{
	SpatialSnapshotData* snapshot_data =
		data->spatial_snapshot_owner.get_or_null(p_spatial_snapshot);
	ERR_FAIL_NULL_V(snapshot_data, XR_NULL_HANDLE);

	return snapshot_data->spatial_snapshot;
}

RID OpenXRSpatialEntityExtension::get_spatial_snapshot_context(RID p_spatial_snapshot)
{
	SpatialSnapshotData* snapshot_data =
		data->spatial_snapshot_owner.get_or_null(p_spatial_snapshot);
	ERR_FAIL_NULL_V(snapshot_data, RID());

	return snapshot_data->spatial_context;
}

uint64_t OpenXRSpatialEntityExtension::_get_spatial_snapshot_handle(RID p_spatial_snapshot)
{
	return (uint64_t)get_spatial_snapshot_handle(p_spatial_snapshot);
}

String OpenXRSpatialEntityExtension::get_string(
	RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id)
{
	String ret;

	SpatialSnapshotData* snapshot_data =
		data->spatial_snapshot_owner.get_or_null(p_spatial_snapshot);
	ERR_FAIL_NULL_V(snapshot_data, ret);

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), ret);

	XrSpatialBufferGetInfoEXT info = {
		XR_TYPE_SPATIAL_BUFFER_GET_INFO_EXT,
		nullptr,
		p_buffer_id,
	};

	uint32_t count = 0;
	XrResult result =
		xrGetSpatialBufferStringEXT(snapshot_data->spatial_snapshot, &info, 0, &count, nullptr);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(ret,
			"OpenXR: Failed to get buffer size [" + OpenXRAPI::get_error_string(result) + "]");
	}

	LocalVector<char> buffer;
	buffer.resize(count + 1);
	buffer[count] = '\0';

	result = xrGetSpatialBufferStringEXT(
		snapshot_data->spatial_snapshot, &info, buffer.size(), &count, buffer.ptr());
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(
			ret, "OpenXR: Failed to get buffer [" + OpenXRAPI::get_error_string(result) + "]");
	}

	ret = String::utf8(buffer.ptr());
	return ret;
}

PackedByteArray OpenXRSpatialEntityExtension::get_uint8_buffer(
	RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id)
{
	PackedByteArray ret;

	SpatialSnapshotData* snapshot_data =
		data->spatial_snapshot_owner.get_or_null(p_spatial_snapshot);
	ERR_FAIL_NULL_V(snapshot_data, ret);

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), ret);

	XrSpatialBufferGetInfoEXT info = {
		XR_TYPE_SPATIAL_BUFFER_GET_INFO_EXT,
		nullptr,
		p_buffer_id,
	};

	uint32_t count = 0;
	XrResult result =
		xrGetSpatialBufferUint8EXT(snapshot_data->spatial_snapshot, &info, 0, &count, nullptr);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(ret,
			"OpenXR: Failed to get buffer size [" + OpenXRAPI::get_error_string(result) + "]");
	}

	ret.resize(count);

	result = xrGetSpatialBufferUint8EXT(
		snapshot_data->spatial_snapshot, &info, ret.size(), &count, (uint8_t*)ret.ptrw());
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(PackedByteArray(),
			"OpenXR: Failed to get buffer [" + OpenXRAPI::get_error_string(result) + "]");
	}

	return ret;
}

Vector<uint16_t> OpenXRSpatialEntityExtension::get_uint16_buffer(
	RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id)
{
	Vector<uint16_t> ret;

	SpatialSnapshotData* snapshot_data =
		data->spatial_snapshot_owner.get_or_null(p_spatial_snapshot);
	ERR_FAIL_NULL_V(snapshot_data, ret);

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), ret);

	XrSpatialBufferGetInfoEXT info = {
		XR_TYPE_SPATIAL_BUFFER_GET_INFO_EXT,
		nullptr,
		p_buffer_id,
	};

	uint32_t count = 0;
	XrResult result =
		xrGetSpatialBufferUint16EXT(snapshot_data->spatial_snapshot, &info, 0, &count, nullptr);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(ret,
			"OpenXR: Failed to get buffer size [" + OpenXRAPI::get_error_string(result) + "]");
	}

	ret.resize(count);

	result = xrGetSpatialBufferUint16EXT(
		snapshot_data->spatial_snapshot, &info, ret.size(), &count, ret.ptrw());
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(Vector<uint16_t>(),
			"OpenXR: Failed to get buffer [" + OpenXRAPI::get_error_string(result) + "]");
	}

	return ret;
}

Vector<uint32_t> OpenXRSpatialEntityExtension::get_uint32_buffer(
	RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id)
{
	Vector<uint32_t> ret;

	SpatialSnapshotData* snapshot_data =
		data->spatial_snapshot_owner.get_or_null(p_spatial_snapshot);
	ERR_FAIL_NULL_V(snapshot_data, ret);

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), ret);

	XrSpatialBufferGetInfoEXT info = {
		XR_TYPE_SPATIAL_BUFFER_GET_INFO_EXT,
		nullptr,
		p_buffer_id,
	};

	uint32_t count = 0;
	XrResult result =
		xrGetSpatialBufferUint32EXT(snapshot_data->spatial_snapshot, &info, 0, &count, nullptr);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(ret,
			"OpenXR: Failed to get buffer size [" + OpenXRAPI::get_error_string(result) + "]");
	}

	ret.resize(count);

	result = xrGetSpatialBufferUint32EXT(
		snapshot_data->spatial_snapshot, &info, ret.size(), &count, ret.ptrw());
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(Vector<uint32_t>(),
			"OpenXR: Failed to get buffer [" + OpenXRAPI::get_error_string(result) + "]");
	}

	return ret;
}

PackedFloat32Array OpenXRSpatialEntityExtension::get_float_buffer(
	RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id)
{
	PackedFloat32Array ret;

	SpatialSnapshotData* snapshot_data =
		data->spatial_snapshot_owner.get_or_null(p_spatial_snapshot);
	ERR_FAIL_NULL_V(snapshot_data, ret);

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), ret);

	XrSpatialBufferGetInfoEXT info = {
		XR_TYPE_SPATIAL_BUFFER_GET_INFO_EXT,
		nullptr,
		p_buffer_id,
	};

	uint32_t count = 0;
	XrResult result =
		xrGetSpatialBufferFloatEXT(snapshot_data->spatial_snapshot, &info, 0, &count, nullptr);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(ret,
			"OpenXR: Failed to get buffer size [" + OpenXRAPI::get_error_string(result) + "]");
	}

	ret.resize(count);

	result = xrGetSpatialBufferFloatEXT(
		snapshot_data->spatial_snapshot, &info, ret.size(), &count, ret.ptrw());
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(PackedFloat32Array(),
			"OpenXR: Failed to get buffer [" + OpenXRAPI::get_error_string(result) + "]");
	}

	return ret;
}

PackedVector2Array OpenXRSpatialEntityExtension::get_vector2_buffer(
	RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id)
{
	PackedVector2Array ret;

	SpatialSnapshotData* snapshot_data =
		data->spatial_snapshot_owner.get_or_null(p_spatial_snapshot);
	ERR_FAIL_NULL_V(snapshot_data, ret);

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), ret);

	XrSpatialBufferGetInfoEXT info = {
		XR_TYPE_SPATIAL_BUFFER_GET_INFO_EXT,
		nullptr,
		p_buffer_id,
	};

	uint32_t count = 0;
	XrResult result =
		xrGetSpatialBufferVector2fEXT(snapshot_data->spatial_snapshot, &info, 0, &count, nullptr);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(ret,
			"OpenXR: Failed to get buffer size [" + OpenXRAPI::get_error_string(result) + "]");
	}

#ifdef REAL_T_IS_DOUBLE
	LocalVector<XrVector2f> buffer;
	buffer.resize(count);

	result = xrGetSpatialBufferVector2fEXT(
		snapshot_data->spatial_snapshot, &info, buffer.size(), &count, buffer.ptr());
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(
			ret, "OpenXR: Failed to get buffer [" + OpenXRAPI::get_error_string(result) + "]");
	}

	ret.resize(count);
	Vector2* ptr = ret.ptrw();
	for (uint32_t i = 0; i < count; i++) {
		ptr[i].x = buffer[i].x;
		ptr[i].y = buffer[i].y;
	}
#else
	ret.resize(count);

	result = xrGetSpatialBufferVector2fEXT(
		snapshot_data->spatial_snapshot, &info, ret.size(), &count, (XrVector2f*)ret.ptrw());
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(PackedVector2Array(),
			"OpenXR: Failed to get buffer [" + OpenXRAPI::get_error_string(result) + "]");
	}
#endif

	return ret;
}

PackedVector3Array OpenXRSpatialEntityExtension::get_vector3_buffer(
	RID p_spatial_snapshot, XrSpatialBufferIdEXT p_buffer_id)
{
	PackedVector3Array ret;

	SpatialSnapshotData* snapshot_data =
		data->spatial_snapshot_owner.get_or_null(p_spatial_snapshot);
	ERR_FAIL_NULL_V(snapshot_data, ret);

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), ret);

	XrSpatialBufferGetInfoEXT info = {
		XR_TYPE_SPATIAL_BUFFER_GET_INFO_EXT,
		nullptr,
		p_buffer_id,
	};

	uint32_t count = 0;
	XrResult result =
		xrGetSpatialBufferVector3fEXT(snapshot_data->spatial_snapshot, &info, 0, &count, nullptr);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(ret,
			"OpenXR: Failed to get buffer size [" + OpenXRAPI::get_error_string(result) + "]");
	}

#ifdef REAL_T_IS_DOUBLE
	LocalVector<XrVector3f> buffer;
	buffer.resize(count);

	result = xrGetSpatialBufferVector3fEXT(
		snapshot_data->spatial_snapshot, &info, buffer.size(), &count, buffer.ptr());
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(
			ret, "OpenXR: Failed to get buffer [" + OpenXRAPI::get_error_string(result) + "]");
	}

	ret.resize(count);
	Vector3* ptr = ret.ptrw();
	for (uint32_t i = 0; i < count; i++) {
		ptr[i].x = buffer[i].x;
		ptr[i].y = buffer[i].y;
		ptr[i].z = buffer[i].z;
	}
#else
	ret.resize(count);

	result = xrGetSpatialBufferVector3fEXT(
		snapshot_data->spatial_snapshot, &info, ret.size(), &count, (XrVector3f*)ret.ptrw());
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(PackedVector3Array(),
			"OpenXR: Failed to get buffer [" + OpenXRAPI::get_error_string(result) + "]");
	}
#endif

	return ret;
}

String OpenXRSpatialEntityExtension::_get_string(RID p_spatial_snapshot, uint64_t p_buffer_id)
{
	return get_string(p_spatial_snapshot, (XrSpatialBufferIdEXT)p_buffer_id);
}

PackedByteArray OpenXRSpatialEntityExtension::_get_uint8_buffer(
	RID p_spatial_snapshot, uint64_t p_buffer_id)
{
	return get_uint8_buffer(p_spatial_snapshot, (XrSpatialBufferIdEXT)p_buffer_id);
}

PackedInt32Array OpenXRSpatialEntityExtension::_get_uint16_buffer(
	RID p_spatial_snapshot, uint64_t p_buffer_id)
{
	PackedInt32Array ret;
	Vector<uint16_t> buffer =
		get_uint16_buffer(p_spatial_snapshot, (XrSpatialBufferIdEXT)p_buffer_id);

	if (!buffer.is_empty()) {
		ret.resize(buffer.size());

		int size = ret.size();
		int32_t* ptr = ret.ptrw();
		for (int i = 0; i < size; i++) {
			ptr[i] = buffer[i];
		}
	}

	return ret;
}

PackedInt32Array OpenXRSpatialEntityExtension::_get_uint32_buffer(
	RID p_spatial_snapshot, uint64_t p_buffer_id)
{
	PackedInt32Array ret;
	Vector<uint32_t> buffer =
		get_uint32_buffer(p_spatial_snapshot, (XrSpatialBufferIdEXT)p_buffer_id);

	if (!buffer.is_empty()) {
		ret.resize(buffer.size());

		int size = ret.size();
		int32_t* ptr = ret.ptrw();
		for (int i = 0; i < size; i++) {
			ptr[i] = buffer[i];
		}
	}

	return ret;
}

PackedFloat32Array OpenXRSpatialEntityExtension::_get_float_buffer(
	RID p_spatial_snapshot, uint64_t p_buffer_id)
{
	return get_float_buffer(p_spatial_snapshot, (XrSpatialBufferIdEXT)p_buffer_id);
}

PackedVector2Array OpenXRSpatialEntityExtension::_get_vector2_buffer(
	RID p_spatial_snapshot, uint64_t p_buffer_id)
{
	return get_vector2_buffer(p_spatial_snapshot, (XrSpatialBufferIdEXT)p_buffer_id);
}

PackedVector3Array OpenXRSpatialEntityExtension::_get_vector3_buffer(
	RID p_spatial_snapshot, uint64_t p_buffer_id)
{
	return get_vector3_buffer(p_spatial_snapshot, (XrSpatialBufferIdEXT)p_buffer_id);
}

RID OpenXRSpatialEntityExtension::find_spatial_entity(XrSpatialEntityIdEXT p_entity_id)
{
	ERR_FAIL_COND_V(!get_active(), RID());

	LocalVector<RID> entities = data->spatial_entity_owner.get_owned_list();
	for (const RID& entity : entities) {
		SpatialEntityData* entity_data = data->spatial_entity_owner.get_or_null(entity);
		ERR_FAIL_NULL_V(entity_data, RID());

		if (entity_data->entity_id == p_entity_id) {
			return entity;
		}
	}

	return RID();
}

RID OpenXRSpatialEntityExtension::_find_entity(uint64_t p_entity_id)
{
	return find_spatial_entity((XrSpatialEntityIdEXT)p_entity_id);
}

RID OpenXRSpatialEntityExtension::add_spatial_entity(
	RID p_spatial_context, XrSpatialEntityIdEXT p_entity_id, XrSpatialEntityEXT p_entity)
{
	ERR_FAIL_COND_V(!get_active(), RID());

	SpatialEntityData spatial_entity_data;
	spatial_entity_data.spatial_context = p_spatial_context;
	spatial_entity_data.entity_id = p_entity_id;
	spatial_entity_data.entity = p_entity;

	return data->spatial_entity_owner.make_rid(spatial_entity_data);
}

RID OpenXRSpatialEntityExtension::_add_entity(
	RID p_spatial_context, uint64_t p_entity_id, uint64_t p_entity)
{
	return add_spatial_entity(
		p_spatial_context, (XrSpatialEntityIdEXT)p_entity_id, (XrSpatialEntityEXT)p_entity);
}

RID OpenXRSpatialEntityExtension::make_spatial_entity(
	RID p_spatial_context, XrSpatialEntityIdEXT p_entity_id)
{
	ERR_FAIL_COND_V(!get_active(), RID());
	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), RID());

	SpatialEntityData spatial_entity_data;
	spatial_entity_data.spatial_context = p_spatial_context;
	spatial_entity_data.entity_id = p_entity_id;
	XrSpatialEntityFromIdCreateInfoEXT create_info = {
		XR_TYPE_SPATIAL_ENTITY_FROM_ID_CREATE_INFO_EXT,
		nullptr,
		p_entity_id,
	};
	XrResult result = xrCreateSpatialEntityFromIdEXT(
		get_spatial_context_handle(p_spatial_context), &create_info, &spatial_entity_data.entity);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(RID(), "OpenXR: Failed to create spatial entity [" +
								  OpenXRAPI::get_error_string(result) + "]");
	}

	return data->spatial_entity_owner.make_rid(spatial_entity_data);
}

RID OpenXRSpatialEntityExtension::_make_entity(RID p_spatial_context, uint64_t p_entity_id)
{
	return make_spatial_entity(p_spatial_context, (XrSpatialEntityIdEXT)p_entity_id);
}

XrSpatialEntityIdEXT OpenXRSpatialEntityExtension::get_spatial_entity_id(RID p_entity)
{
	SpatialEntityData* entity_data = data->spatial_entity_owner.get_or_null(p_entity);
	ERR_FAIL_NULL_V(entity_data, XR_NULL_ENTITY);

	return entity_data->entity_id;
}

uint64_t OpenXRSpatialEntityExtension::_get_entity_id(RID p_entity)
{
	return (uint64_t)get_spatial_entity_id(p_entity);
}

RID OpenXRSpatialEntityExtension::get_spatial_entity_context(RID p_entity)
{
	SpatialEntityData* entity_data = data->spatial_entity_owner.get_or_null(p_entity);
	ERR_FAIL_NULL_V(entity_data, RID());

	return entity_data->spatial_context;
}

void OpenXRSpatialEntityExtension::free_spatial_entity(RID p_entity)
{
	SpatialEntityData* entity_data = data->spatial_entity_owner.get_or_null(p_entity);
	ERR_FAIL_NULL(entity_data);
	ERR_FAIL_COND(entity_data->entity == XR_NULL_HANDLE);

	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	XrResult result = xrDestroySpatialEntityEXT_ptr(entity_data->entity);
	if (XR_FAILED(result)) {
		WARN_PRINT("OpenXR: Failed to destroy spatial entity [" +
				   OpenXRAPI::get_error_string(result) + "]");
	}

	data->spatial_entity_owner.free(p_entity);
}

String OpenXRSpatialEntityExtension::get_spatial_capability_name(
	XrSpatialCapabilityEXT p_capability)
{
	XR_ENUM_SWITCH(XrSpatialCapabilityEXT, p_capability)
}

String OpenXRSpatialEntityExtension::get_spatial_component_type_name(
	XrSpatialComponentTypeEXT p_component_type)
{
	XR_ENUM_SWITCH(XrSpatialComponentTypeEXT, p_component_type)
}

String OpenXRSpatialEntityExtension::get_spatial_feature_name(
	XrSpatialCapabilityFeatureEXT p_feature)
{
	XR_ENUM_SWITCH(XrSpatialCapabilityFeatureEXT, p_feature)
}

void OpenXRSpatialEntityExtension::on_session_destroyed() {}

bool OpenXRSpatialEntityExtension::on_event_polled(const XrEventDataBuffer& p_event)
{
	return false;
}

////////////////////////////////////////////////////////////////////////////
// OpenXRSpatialAnchorCapability

void OpenXRSpatialAnchorCapability::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXRSpatialAnchorCapability::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

HashMap<String, bool*> OpenXRSpatialAnchorCapability::get_requested_extensions(XrVersion p_version)
{
	HashMap<String, bool*> request_extensions;

	if (GLOBAL_GET_CACHED(bool, "xr/openxr/extensions/spatial_entity/enabled") &&
		GLOBAL_GET_CACHED(bool, "xr/openxr/extensions/spatial_entity/enable_spatial_anchors")) {
		request_extensions[XR_EXT_SPATIAL_ANCHOR_EXTENSION_NAME] = &data->spatial_anchor_ext;
		if (GLOBAL_GET_CACHED(
				bool, "xr/openxr/extensions/spatial_entity/enable_persistent_anchors")) {
			request_extensions[XR_EXT_SPATIAL_PERSISTENCE_EXTENSION_NAME] =
				&data->spatial_persistence_ext;
			request_extensions[XR_EXT_SPATIAL_PERSISTENCE_OPERATIONS_EXTENSION_NAME] =
				&data->spatial_persistence_operations_ext;
		}
	}

	return request_extensions;
}

void OpenXRSpatialAnchorCapability::on_instance_created(const XrInstance p_instance)
{
	ERR_FAIL_COND(!OpenXRSpatialEntityExtension::is_initialized());

	if (data->spatial_anchor_ext) {
		EXT_INIT_XR_FUNC(xrCreateSpatialAnchorEXT);
	}

	if (data->spatial_persistence_ext) {
		ERR_FAIL_COND(!OpenXRAPI::is_allocated());

		XrResult xr_result =
			OpenXRAPI::get_instance_proc_addr("xrEnumerateSpatialPersistenceScopesEXT",
				(PFN_xrVoidFunction*)&xrEnumerateSpatialPersistenceScopesEXT_ptr);
		if (xr_result != XR_SUCCESS) {
			print_verbose("OpenXR: xrEnumerateSpatialPersistenceScopesEXT is not supported, "
						  "falling back to xrEnumerateSpatialPersistenceStoresEXT!");
			xr_result = OpenXRAPI::get_instance_proc_addr("xrEnumerateSpatialPersistenceStoresEXT",
				(PFN_xrVoidFunction*)&xrEnumerateSpatialPersistenceScopesEXT_ptr);
		}
		ERR_FAIL_COND(XR_FAILED(xr_result));

		EXT_INIT_XR_FUNC(xrCreateSpatialPersistenceContextAsyncEXT);
		EXT_INIT_XR_FUNC(xrCreateSpatialPersistenceContextCompleteEXT);
		EXT_INIT_XR_FUNC(xrDestroySpatialPersistenceContextEXT);
	}

	if (data->spatial_persistence_operations_ext) {
		EXT_INIT_XR_FUNC(xrPersistSpatialEntityAsyncEXT);
		EXT_INIT_XR_FUNC(xrPersistSpatialEntityCompleteEXT);
		EXT_INIT_XR_FUNC(xrUnpersistSpatialEntityAsyncEXT);
		EXT_INIT_XR_FUNC(xrUnpersistSpatialEntityCompleteEXT);
	}
}

void OpenXRSpatialAnchorCapability::on_instance_destroyed()
{
	xrCreateSpatialAnchorEXT_ptr = nullptr;

	xrEnumerateSpatialPersistenceScopesEXT_ptr = nullptr;
	xrCreateSpatialPersistenceContextAsyncEXT_ptr = nullptr;
	xrCreateSpatialPersistenceContextCompleteEXT_ptr = nullptr;
	xrDestroySpatialPersistenceContextEXT_ptr = nullptr;
}

bool OpenXRSpatialAnchorCapability::is_spatial_anchor_supported()
{
	return data != nullptr && data->spatial_anchor_supported;
}

bool OpenXRSpatialAnchorCapability::is_spatial_persistence_supported()
{
	if (!is_spatial_anchor_supported()) {
		return false;
	}

	return data->spatial_persistence_ext;
}

bool OpenXRSpatialAnchorCapability::_load_supported_persistence_scopes()
{
	ERR_FAIL_COND_V(!is_spatial_persistence_supported(), false);

	if (data->supported_persistence_scopes.is_empty()) {
		ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), false);

		uint32_t size;
		XrInstance instance = OpenXRAPI::get_instance();
		XrSystemId system_id = OpenXRAPI::get_system_id();

		ERR_FAIL_COND_V(instance == XR_NULL_HANDLE, false);

		XrResult result =
			xrEnumerateSpatialPersistenceScopesEXT(instance, system_id, 0, &size, nullptr);
		if (XR_FAILED(result)) {
			ERR_FAIL_V_MSG(false, "OpenXR: Failed to query persistence scope count [" +
									  OpenXRAPI::get_error_string(result) + "]");
		}

		if (size > 0) {
			data->supported_persistence_scopes.resize(size);
			result = xrEnumerateSpatialPersistenceScopesEXT(instance, system_id,
				data->supported_persistence_scopes.size(), &size,
				data->supported_persistence_scopes.ptrw());
			if (XR_FAILED(result)) {
				ERR_FAIL_V_MSG(false, "OpenXR: Failed to query persistence scopes [" +
										  OpenXRAPI::get_error_string(result) + "]");
			}
		}

		if (is_print_verbose_enabled()) {
			if (!data->supported_persistence_scopes.is_empty()) {
				print_verbose("OpenXR: Supported spatial persistence scopes:");
				for (const XrSpatialPersistenceScopeEXT& scope :
					data->supported_persistence_scopes) {
					print_verbose(" - " + get_spatial_persistence_scope_name(scope));
				}
			}
			else {
				WARN_PRINT("OpenXR: No persistence scopes found!");
			}
		}
	}

	return true;
}

bool OpenXRSpatialAnchorCapability::is_persistence_scope_supported(
	XrSpatialPersistenceScopeEXT p_scope)
{
	if (!is_spatial_persistence_supported()) {
		return false;
	}

	if (!_load_supported_persistence_scopes()) {
		return false;
	}

	return data->supported_persistence_scopes.has(p_scope);
}

bool OpenXRSpatialAnchorCapability::_is_persistence_scope_supported(PersistenceScope p_scope)
{
	return is_persistence_scope_supported((XrSpatialPersistenceScopeEXT)p_scope);
}

XrSpatialPersistenceContextEXT OpenXRSpatialAnchorCapability::get_persistence_context_handle(
	RID p_persistence_context)
{
	PersistenceContextData* persistence_context_data =
		data->persistence_context_owner.get_or_null(p_persistence_context);
	ERR_FAIL_NULL_V(persistence_context_data, XR_NULL_HANDLE);

	return persistence_context_data->persistence_context;
}

uint64_t OpenXRSpatialAnchorCapability::_get_persistence_context_handle(RID p_persistence_context)
{
	return (uint64_t)get_persistence_context_handle(p_persistence_context);
}

void OpenXRSpatialAnchorCapability::free_persistence_context(RID p_persistence_context)
{
	PersistenceContextData* persistence_context_data =
		data->persistence_context_owner.get_or_null(p_persistence_context);
	ERR_FAIL_NULL(persistence_context_data);

	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	if (persistence_context_data->persistence_context != XR_NULL_HANDLE) {
		XrResult result =
			xrDestroySpatialPersistenceContextEXT(persistence_context_data->persistence_context);
		if (XR_FAILED(result)) {
			WARN_PRINT("OpenXR: Failed to destroy the persistence context [" +
					   OpenXRAPI::get_error_string(result) + "]");
		}
		persistence_context_data->persistence_context = XR_NULL_HANDLE;
	}

	data->persistence_context_owner.free(p_persistence_context);
}

void OpenXRSpatialAnchorCapability::_on_persistence_context_completed(RID p_persistence_context)
{
	data->persistence_context = p_persistence_context;

	_create_spatial_context();
}

void OpenXRSpatialAnchorCapability::_on_spatial_context_created(RID p_spatial_context)
{
	data->spatial_context = p_spatial_context;
	data->need_discovery = true;
}

void OpenXRSpatialAnchorCapability::_on_spatial_discovery_recommended(RID p_spatial_context)
{
	if (p_spatial_context == data->spatial_context) {
		data->need_discovery = true;
	}
}

Ref<OpenXRAnchorTracker> OpenXRSpatialAnchorCapability::create_new_anchor(
	const Transform3D& p_transform, RID p_spatial_context, Ref<OpenXRStructureBase> p_next)
{
	Ref<OpenXRAnchorTracker> tracker;

	ERR_FAIL_COND_V_MSG(!is_spatial_anchor_supported(), tracker,
		"OpenXR: Spatial entity anchor capability is not supported on this hardware!");

	ERR_FAIL_COND_V(!OpenXRAPI::is_allocated(), tracker);
	ERR_FAIL_COND_V(!XRServer::is_initialized(), tracker);
	ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), tracker);

	XrPosef pose = OpenXRAPI::pose_from_transform(p_transform);

	RID sc = p_spatial_context.is_valid() ? p_spatial_context : data->spatial_context;
	ERR_FAIL_COND_V(sc.is_null(), tracker);

	void* next = nullptr;
	if (p_next.is_valid()) {
		next = p_next->get_header(next);
	}

	XrSpatialAnchorCreateInfoEXT create_info = {
		XR_TYPE_SPATIAL_ANCHOR_CREATE_INFO_EXT,
		next,
		OpenXRAPI::get_play_space(),
		OpenXRAPI::get_predicted_display_time(),
		pose,
	};
	XrSpatialEntityIdEXT entity_id;
	XrSpatialEntityEXT entity;
	XrResult result = xrCreateSpatialAnchorEXT(
		OpenXRSpatialEntityExtension::get_spatial_context_handle(sc), &create_info, &entity_id,
		&entity);
	if (XR_FAILED(result)) {
		ERR_FAIL_V_MSG(tracker,
			"OpenXR: Failed to create anchor [" + OpenXRAPI::get_error_string(result) + "]");
	}

	tracker.instantiate(OpenXRSpatialEntityTracker::ENTITY_ANCHOR);
	tracker->set_spatial_context(sc);
	tracker->set_entity(OpenXRSpatialEntityExtension::add_spatial_entity(sc, entity_id, entity));
	tracker->set_tracker_desc("Anchor");
	tracker->set_pose(SNAME("default"), p_transform, Vector3(), Vector3());
	tracker->add_next(p_next);

	data->anchor_trackers[sc][entity_id] = tracker;
	XRServer::add_tracker(tracker->get_xr_tracker());

	return tracker;
}

void OpenXRSpatialAnchorCapability::remove_anchor(Ref<OpenXRAnchorTracker> p_anchor_tracker)
{
	ERR_FAIL_COND(p_anchor_tracker.is_null());
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());
	ERR_FAIL_COND(!XRServer::is_initialized());
	ERR_FAIL_COND(!OpenXRSpatialEntityExtension::is_initialized());

	ERR_FAIL_COND_MSG(p_anchor_tracker->has_uuid(),
		"OpenXR: This anchor is persistent. It must first be made unpersistent.");

	XRServer::remove_tracker(p_anchor_tracker->get_xr_tracker());

	RID entity = p_anchor_tracker->get_entity();
	ERR_FAIL_COND(entity.is_null());

	XrSpatialEntityIdEXT entity_id = OpenXRSpatialEntityExtension::get_spatial_entity_id(entity);
	ERR_FAIL_COND(entity_id == XR_NULL_ENTITY);

	for (KeyValue<RID, HashMap<XrSpatialEntityIdEXT, Ref<OpenXRAnchorTracker>>>& anchors :
		data->anchor_trackers) {
		if (anchors.value.has(entity_id)) {
			anchors.value.erase(entity_id);
			break;
		}
	}

	p_anchor_tracker->set_entity(RID());
}

String OpenXRSpatialAnchorCapability::get_spatial_persistence_scope_name(
	XrSpatialPersistenceScopeEXT p_scope)
{
	XR_ENUM_SWITCH(XrSpatialPersistenceScopeEXT, p_scope)
}

String OpenXRSpatialAnchorCapability::get_spatial_persistence_context_result_name(
	XrSpatialPersistenceContextResultEXT p_result)
{
	XR_ENUM_SWITCH(XrSpatialPersistenceContextResultEXT, p_result)
}

Ref<OpenXRFutureResult> OpenXRSpatialAnchorCapability::_create_spatial_context()
{
	Ref<OpenXRFutureResult> oxfr = memnew(OpenXRFutureResult);
	return oxfr;
}

void OpenXRSpatialAnchorCapability::on_session_created(XrSession_T* p_session) {}

void OpenXRSpatialAnchorCapability::on_session_destroyed() {}

void OpenXRSpatialAnchorCapability::on_process() {}

////////////////////////////////////////////////////////////////////////////
// OpenXRSpatialMarkerTrackingCapability

void OpenXRSpatialMarkerTrackingCapability::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXRSpatialMarkerTrackingCapability::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

HashMap<String, bool*> OpenXRSpatialMarkerTrackingCapability::get_requested_extensions(
	XrVersion p_version)
{
	HashMap<String, bool*> request_extensions;

	if (GLOBAL_GET_CACHED(bool, "xr/openxr/extensions/spatial_entity/enabled") &&
		GLOBAL_GET_CACHED(bool, "xr/openxr/extensions/spatial_entity/enable_marker_tracking")) {
		request_extensions[XR_EXT_SPATIAL_MARKER_TRACKING_EXTENSION_NAME] =
			&data->spatial_marker_tracking_ext;
	}

	return request_extensions;
}

bool OpenXRSpatialMarkerTrackingCapability::is_qrcode_supported()
{
	if (!data->spatial_marker_tracking_ext) {
		return false;
	}

	ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), false);

	return OpenXRSpatialEntityExtension::supports_capability(
		XR_SPATIAL_CAPABILITY_MARKER_TRACKING_QR_CODE_EXT);
}

bool OpenXRSpatialMarkerTrackingCapability::is_micro_qrcode_supported()
{
	ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), false);

	return OpenXRSpatialEntityExtension::supports_capability(
		XR_SPATIAL_CAPABILITY_MARKER_TRACKING_MICRO_QR_CODE_EXT);
}

bool OpenXRSpatialMarkerTrackingCapability::is_aruco_supported()
{
	ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), false);

	return OpenXRSpatialEntityExtension::supports_capability(
		XR_SPATIAL_CAPABILITY_MARKER_TRACKING_ARUCO_MARKER_EXT);
}

bool OpenXRSpatialMarkerTrackingCapability::is_april_tag_supported()
{
	ERR_FAIL_COND_V(!OpenXRSpatialEntityExtension::is_initialized(), false);

	return OpenXRSpatialEntityExtension::supports_capability(
		XR_SPATIAL_CAPABILITY_MARKER_TRACKING_APRIL_TAG_EXT);
}

void OpenXRSpatialMarkerTrackingCapability::_on_spatial_context_created(RID p_spatial_context)
{
	data->spatial_context = p_spatial_context;
	data->need_discovery = true;
}

void OpenXRSpatialMarkerTrackingCapability::_on_spatial_discovery_recommended(RID p_spatial_context)
{
	if (p_spatial_context == data->spatial_context) {
		data->need_discovery = true;
	}
}

void OpenXRSpatialMarkerTrackingCapability::on_session_created(XrSession_T* p_session) {}

void OpenXRSpatialMarkerTrackingCapability::on_session_destroyed() {}

void OpenXRSpatialMarkerTrackingCapability::on_process() {}

////////////////////////////////////////////////////////////////////////////
// OpenXRSpatialPlaneTrackingCapability

void OpenXRSpatialPlaneTrackingCapability::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXRSpatialPlaneTrackingCapability::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}

HashMap<String, bool*> OpenXRSpatialPlaneTrackingCapability::get_requested_extensions(
	XrVersion p_version)
{
	HashMap<String, bool*> request_extensions;

	if (GLOBAL_GET_CACHED(bool, "xr/openxr/extensions/spatial_entity/enabled") &&
		GLOBAL_GET_CACHED(bool, "xr/openxr/extensions/spatial_entity/enable_plane_tracking")) {
		request_extensions[XR_EXT_SPATIAL_PLANE_TRACKING_EXTENSION_NAME] =
			&data->spatial_plane_tracking_ext;
	}

	return request_extensions;
}

bool OpenXRSpatialPlaneTrackingCapability::is_supported()
{
	return data != nullptr && data->spatial_plane_tracking_supported;
}

void OpenXRSpatialPlaneTrackingCapability::_on_spatial_context_created(RID p_spatial_context)
{
	data->spatial_context = p_spatial_context;
	data->need_discovery = true;
}

void OpenXRSpatialPlaneTrackingCapability::_on_spatial_discovery_recommended(RID p_spatial_context)
{
	if (p_spatial_context == data->spatial_context) {
		data->need_discovery = true;
	}
}

void OpenXRSpatialPlaneTrackingCapability::on_session_created(XrSession_T* p_session) {}

void OpenXRSpatialPlaneTrackingCapability::on_session_destroyed() {}

void OpenXRSpatialPlaneTrackingCapability::on_process() {}


// --- Merged from modules/openxr/extensions/platform/openxr_vulkan_extension.cpp ---
#if defined(VULKAN_ENABLED)
#include "modules/openxr/openxr_api.h"

#include "core/string/print_string.h"

#include "servers/rendering/rendering_device.h"
#include "servers/rendering/renderer.h"

void OpenXRVulkanExtension::initialize()
{
	if (data == nullptr) {
		data = memnew(Data);
		VulkanHooks::bind<OpenXRVulkanExtension>();
	}
}

void OpenXRVulkanExtension::finalize()
{
	if (data != nullptr) {
		VulkanHooks::reset();
		memdelete(data);
		data = nullptr;
	}
}

HashMap<String, bool*> OpenXRVulkanExtension::get_requested_extensions(XrVersion p_version)
{
	HashMap<String, bool*> request_extensions;

	request_extensions[XR_KHR_VULKAN_ENABLE2_EXTENSION_NAME] = nullptr; // must be available

	return request_extensions;
}

void OpenXRVulkanExtension::on_instance_created(const XrInstance p_instance)
{
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

	// Obtain pointers to functions we're accessing here, they are (not yet) part of core.

	EXT_INIT_XR_FUNC(xrGetVulkanGraphicsRequirements2KHR);
	EXT_INIT_XR_FUNC(xrCreateVulkanInstanceKHR);
	EXT_INIT_XR_FUNC(xrGetVulkanGraphicsDevice2KHR);
	EXT_INIT_XR_FUNC(xrCreateVulkanDeviceKHR);
	EXT_INIT_XR_FUNC(xrEnumerateSwapchainImages);
}

void OpenXRVulkanExtension::set_direct_queue_family_and_index(
	uint32_t p_queue_family_index, uint32_t p_queue_index)
{
	ERR_FAIL_NULL(data);
	data->vulkan_queue_family_index = p_queue_family_index;
	data->vulkan_queue_index = p_queue_index;
}

bool OpenXRVulkanExtension::use_fragment_density_offsets()
{
	if (!OpenXRFBFoveationExtension::is_initialized()) {
		return false;
	}

	return OpenXRFBFoveationExtension::is_foveation_eye_tracked_enabled();
}

void OpenXRVulkanExtension::get_fragment_density_offsets(
	LocalVector<VkOffset2D>& r_vk_offsets, const Vector2i& p_granularity)
{
	if (!OpenXRFBFoveationExtension::is_initialized()) {
		return;
	}

	LocalVector<Vector2i> offsets;
	OpenXRFBFoveationExtension::get_fragment_density_offsets(offsets);

	r_vk_offsets.reserve(offsets.size());
	for (Vector2i offset : offsets) {
		offset = ((offset + p_granularity / 2) / p_granularity) * p_granularity;

		r_vk_offsets.push_back(VkOffset2D{offset.x, offset.y});
	}
}

bool OpenXRVulkanExtension::use_subsampled_images()
{
	if (!OpenXRFBFoveationExtension::is_initialized()) {
		return false;
	}

	return OpenXRFBFoveationExtension::is_foveation_with_subsampled_images_enabled();
}

XrGraphicsBindingVulkanKHR OpenXRVulkanExtension::graphics_binding_vulkan;

void* OpenXRVulkanExtension::set_session_create_and_get_next_pointer(void* p_next_pointer)
{
	DEV_ASSERT(data->vulkan_queue_family_index < UINT32_MAX &&
			   "Direct queue family index was not specified yet.");
	DEV_ASSERT(data->vulkan_queue_index < UINT32_MAX && "Direct queue index was not specified yet.");

	graphics_binding_vulkan.type = XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR;
	graphics_binding_vulkan.next = p_next_pointer;
	graphics_binding_vulkan.instance = data->vulkan_instance;
	graphics_binding_vulkan.physicalDevice = data->vulkan_physical_device;
	graphics_binding_vulkan.device = data->vulkan_device;
	graphics_binding_vulkan.queueFamilyIndex = data->vulkan_queue_family_index;
	graphics_binding_vulkan.queueIndex = data->vulkan_queue_index;

	return &graphics_binding_vulkan;
}

void OpenXRVulkanExtension::get_usable_swapchain_formats(Vector<int64_t>& p_usable_swap_chains)
{
	// We might want to do more here especially if we keep things in linear color space
	// Possibly add in R10G10B10A2 as an option if we're using the mobile renderer.
	p_usable_swap_chains.push_back(VK_FORMAT_R8G8B8A8_SRGB);
	p_usable_swap_chains.push_back(VK_FORMAT_B8G8R8A8_SRGB);
	p_usable_swap_chains.push_back(VK_FORMAT_R8G8B8A8_UINT);
	p_usable_swap_chains.push_back(VK_FORMAT_B8G8R8A8_UINT);
}

void OpenXRVulkanExtension::get_usable_depth_formats(Vector<int64_t>& p_usable_swap_chains)
{
	// Note, it is very likely we do NOT support any of depth formats where we can combine our
	// stencil support (e.g. _S8_UINT). Right now this isn't a problem but once stencil support
	// becomes an issue, we need to check for this in the rendering engine and create a separate
	// buffer for the stencil.

	p_usable_swap_chains.push_back(VK_FORMAT_D24_UNORM_S8_UINT);
	p_usable_swap_chains.push_back(VK_FORMAT_D32_SFLOAT_S8_UINT);
	p_usable_swap_chains.push_back(VK_FORMAT_D32_SFLOAT);
}

bool OpenXRVulkanExtension::create_projection_fov(
	const XrFovf p_fov, double p_z_near, double p_z_far, Projection& r_camera_matrix)
{
	OpenXRUtil::XrMatrix4x4f matrix;
	OpenXRUtil::XrMatrix4x4f_CreateProjectionFov(&matrix, p_fov, (float)p_z_near, (float)p_z_far);

	for (int j = 0; j < 4; j++) {
		for (int i = 0; i < 4; i++) {
			r_camera_matrix.columns[j][i] = matrix.m[j * 4 + i];
		}
	}

	return true;
}

RID OpenXRVulkanExtension::get_texture(void* p_swapchain_graphics_data, int p_image_index)
{
	SwapchainGraphicsData* data = (SwapchainGraphicsData*)p_swapchain_graphics_data;
	ERR_FAIL_NULL_V(data, RID());

	ERR_FAIL_INDEX_V(p_image_index, data->texture_rids.size(), RID());
	return data->texture_rids[p_image_index];
}

RID OpenXRVulkanExtension::get_density_map(void* p_swapchain_graphics_data, int p_image_index)
{
	SwapchainGraphicsData* data = (SwapchainGraphicsData*)p_swapchain_graphics_data;
	ERR_FAIL_NULL_V(data, RID());

	ERR_FAIL_INDEX_V(p_image_index, data->density_map_rids.size(), RID());
	return data->density_map_rids[p_image_index];
}

void OpenXRVulkanExtension::cleanup_swapchain_graphics_data(void** p_swapchain_graphics_data)
{
	if (*p_swapchain_graphics_data == nullptr) {
		return;
	}

	SwapchainGraphicsData* data = (SwapchainGraphicsData*)*p_swapchain_graphics_data;

	ERR_FAIL_NULL(Renderer::data);
	ERR_FAIL_NULL(RD::data);

	for (const RID& texture_rid : data->texture_rids) {
		// This should clean up our RIDs and associated texture objects but shouldn't destroy the
		// images, they are owned by our XrSwapchain.
		RD::free_rid(texture_rid);
	}
	data->texture_rids.clear();

	for (int i = 0; i < data->density_map_rids.size(); i++) {
		if (data->density_map_rids[i].is_valid()) {
			RD::free_rid(data->density_map_rids[i]);
		}
	}
	data->density_map_rids.clear();

	memdelete(data);
	*p_swapchain_graphics_data = nullptr;
}

#define ENUM_TO_STRING_CASE(e)                                                                     \
	case e: {                                                                                      \
		return String(#e);                                                                         \
	} break;

String OpenXRVulkanExtension::get_swapchain_format_name(int64_t p_swapchain_format)
{
	// This really should be in vulkan_context...
	VkFormat format = VkFormat(p_swapchain_format);
	switch (format) {
		ENUM_TO_STRING_CASE(VK_FORMAT_UNDEFINED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R4G4_UNORM_PACK8)
		ENUM_TO_STRING_CASE(VK_FORMAT_R4G4B4A4_UNORM_PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_B4G4R4A4_UNORM_PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_R5G6B5_UNORM_PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_B5G6R5_UNORM_PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_R5G5B5A1_UNORM_PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_B5G5R5A1_UNORM_PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_A1R5G5B5_UNORM_PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8_SNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8_USCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8_SSCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8_SRGB)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8_SNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8_USCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8_SSCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8_SRGB)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8_SNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8_USCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8_SSCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8_SRGB)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8_SNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8_USCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8_SSCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8_SRGB)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8A8_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8A8_SNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8A8_USCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8A8_SSCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8A8_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8A8_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R8G8B8A8_SRGB)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8A8_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8A8_SNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8A8_USCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8A8_SSCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8A8_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8A8_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8A8_SRGB)
		ENUM_TO_STRING_CASE(VK_FORMAT_A8B8G8R8_UNORM_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A8B8G8R8_SNORM_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A8B8G8R8_USCALED_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A8B8G8R8_SSCALED_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A8B8G8R8_UINT_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A8B8G8R8_SINT_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A8B8G8R8_SRGB_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A2R10G10B10_UNORM_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A2R10G10B10_SNORM_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A2R10G10B10_USCALED_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A2R10G10B10_SSCALED_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A2R10G10B10_UINT_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A2R10G10B10_SINT_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A2B10G10R10_UNORM_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A2B10G10R10_SNORM_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A2B10G10R10_USCALED_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A2B10G10R10_SSCALED_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A2B10G10R10_UINT_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_A2B10G10R10_SINT_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16_SNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16_USCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16_SSCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16_SFLOAT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16_SNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16_USCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16_SSCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16_SFLOAT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16_SNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16_USCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16_SSCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16_SFLOAT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16A16_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16A16_SNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16A16_USCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16A16_SSCALED)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16A16_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16A16_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R16G16B16A16_SFLOAT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R32_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R32_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R32_SFLOAT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R32G32_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R32G32_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R32G32_SFLOAT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R32G32B32_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R32G32B32_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R32G32B32_SFLOAT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R32G32B32A32_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R32G32B32A32_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R32G32B32A32_SFLOAT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R64_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R64_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R64_SFLOAT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R64G64_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R64G64_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R64G64_SFLOAT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R64G64B64_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R64G64B64_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R64G64B64_SFLOAT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R64G64B64A64_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R64G64B64A64_SINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_R64G64B64A64_SFLOAT)
		ENUM_TO_STRING_CASE(VK_FORMAT_B10G11R11_UFLOAT_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_E5B9G9R9_UFLOAT_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_D16_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_X8_D24_UNORM_PACK32)
		ENUM_TO_STRING_CASE(VK_FORMAT_D32_SFLOAT)
		ENUM_TO_STRING_CASE(VK_FORMAT_S8_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_D16_UNORM_S8_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_D24_UNORM_S8_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_D32_SFLOAT_S8_UINT)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC1_RGB_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC1_RGB_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC1_RGBA_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC1_RGBA_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC2_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC2_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC3_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC3_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC4_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC4_SNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC5_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC5_SNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC6H_UFLOAT_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC6H_SFLOAT_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC7_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_BC7_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_EAC_R11_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_EAC_R11_SNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_EAC_R11G11_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_EAC_R11G11_SNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_4x4_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_4x4_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_5x4_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_5x4_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_5x5_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_5x5_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_6x5_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_6x5_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_6x6_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_6x6_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_8x5_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_8x5_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_8x6_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_8x6_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_8x8_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_8x8_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_10x5_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_10x5_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_10x6_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_10x6_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_10x8_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_10x8_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_10x10_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_10x10_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_12x10_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_12x10_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_12x12_UNORM_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_12x12_SRGB_BLOCK)
		ENUM_TO_STRING_CASE(VK_FORMAT_G8B8G8R8_422_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_B8G8R8G8_422_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_G8_B8R8_2PLANE_420_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_G8_B8R8_2PLANE_422_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_R10X6_UNORM_PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_R10X6G10X6_UNORM_2PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_R12X4_UNORM_PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_R12X4G12X4_UNORM_2PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16)
		ENUM_TO_STRING_CASE(VK_FORMAT_G16B16G16R16_422_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_B16G16R16G16_422_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_G16_B16R16_2PLANE_420_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_G16_B16R16_2PLANE_422_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM)
		ENUM_TO_STRING_CASE(VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG)
		ENUM_TO_STRING_CASE(VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG)
		ENUM_TO_STRING_CASE(VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG)
		ENUM_TO_STRING_CASE(VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG)
		ENUM_TO_STRING_CASE(VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG)
		ENUM_TO_STRING_CASE(VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG)
		ENUM_TO_STRING_CASE(VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG)
		ENUM_TO_STRING_CASE(VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_G8_B8R8_2PLANE_444_UNORM_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_G16_B16R16_2PLANE_444_UNORM_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_A4R4G4B4_UNORM_PACK16_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_A4B4G4R4_UNORM_PACK16_EXT)
		ENUM_TO_STRING_CASE(VK_FORMAT_MAX_ENUM)
	default: {
		return String("Swapchain format ") + String::num_int64(int64_t(p_swapchain_format));
	} break;
	}
}

bool OpenXRVulkanExtension::create_vulkan_instance(
	const VkInstanceCreateInfo* p_create_info, VkInstance* r_instance)
{
	return true;
}

bool OpenXRVulkanExtension::get_physical_device(VkPhysicalDevice* r_physical_device)
{
	return true;
}

bool OpenXRVulkanExtension::create_vulkan_device(
	const VkDeviceCreateInfo* p_create_info, VkDevice* r_device)
{
	return true;
}

bool OpenXRVulkanExtension::get_swapchain_image_data(XrSwapchain_T* p_swapchain, long p_index,
	unsigned int p_width, unsigned int p_height, unsigned int p_layers, unsigned int p_format,
	void** r_image_data)
{
	return true;
}
#ifdef ENUM_TO_STRING_CASE
#undef ENUM_TO_STRING_CASE
#endif
#endif // defined(VULKAN_ENABLED)


// --- Merged from modules/openxr/extensions/platform/openxr_opengl_extension.cpp ---
#if defined(GLES3_ENABLED) && !defined(MACOS_ENABLED)


#ifdef GLES3_ENABLED

#include "modules/openxr/openxr_api.h"
#include "drivers/gles3/rasterizer_gles3.h"
#include "servers/display/display_server.h"

void OpenXROpenGLExtension::initialize()
{
	if (!data) {
		data = memnew(Data);
	}
}

void OpenXROpenGLExtension::finalize()
{
	if (!data) {
		return;
	}
	memdelete(data);
	data = nullptr;
}


// OpenXR requires us to submit sRGB textures so that it recognizes the content
// as being in sRGB color space. We do fall back on "normal" textures but this
// will likely result in incorrect colors as OpenXR will double the sRGB conversion.
// All major XR runtimes support sRGB textures.

// In OpenGL output of the fragment shader is assumed to be in the color space of
// the developers choice, however a linear to sRGB HW conversion can be enabled
// through enabling GL_FRAMEBUFFER_SRGB if an sRGB color attachment is used.
// This is a global setting.
// See: https://www.khronos.org/opengl/wiki/Framebuffer

// In OpenGLES output of the fragment shader is assumed to be in linear color space
// and will be converted by default to sRGB if an sRGB color attachment is used.
// The extension GL_EXT_sRGB_write_control was introduced to enable turning this
// feature off.
// See: https://registry.khronos.org/OpenGL/extensions/EXT/EXT_sRGB_write_control.txt

HashMap<String, bool*> OpenXROpenGLExtension::get_requested_extensions(XrVersion p_version)
{
	HashMap<String, bool*> request_extensions;

#ifdef XR_USE_GRAPHICS_API_OPENGL_ES
	request_extensions[XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME] = nullptr;
#else
	request_extensions[XR_KHR_OPENGL_ENABLE_EXTENSION_NAME] = nullptr;
#endif
#if defined(LINUXBSD_ENABLED) && defined(EGL_ENABLED) && defined(WAYLAND_ENABLED)
	request_extensions[XR_MNDX_EGL_ENABLE_EXTENSION_NAME] = &data->egl_extension_enabled;
#endif

	return request_extensions;
}

void OpenXROpenGLExtension::on_instance_created(const XrInstance p_instance)
{
	// Obtain pointers to functions we're accessing here.
	ERR_FAIL_COND(!OpenXRAPI::is_allocated());

#ifdef XR_USE_GRAPHICS_API_OPENGL_ES
	EXT_INIT_XR_FUNC(xrGetOpenGLESGraphicsRequirementsKHR);
#else
	EXT_INIT_XR_FUNC(xrGetOpenGLGraphicsRequirementsKHR);
#endif
	EXT_INIT_XR_FUNC(xrEnumerateSwapchainImages);
}

#ifdef WIN32
XrGraphicsBindingOpenGLWin32KHR OpenXROpenGLExtension::graphics_binding_gl;
#elif defined(ANDROID_ENABLED)
XrGraphicsBindingOpenGLESAndroidKHR OpenXROpenGLExtension::graphics_binding_gl;
#elif defined(LINUXBSD_ENABLED)
#ifdef X11_ENABLED
XrGraphicsBindingOpenGLXlibKHR OpenXROpenGLExtension::graphics_binding_gl;
#endif
#if defined(EGL_ENABLED) && defined(WAYLAND_ENABLED)
XrGraphicsBindingEGLMNDX OpenXROpenGLExtension::graphics_binding_egl;
#endif
#endif

void* OpenXROpenGLExtension::set_session_create_and_get_next_pointer(void* p_next_pointer)
{
	GLint gl_version_major = 0;
	GLint gl_version_minor = 0;
	glGetIntegerv(GL_MAJOR_VERSION, &gl_version_major);
	glGetIntegerv(GL_MINOR_VERSION, &gl_version_minor);

	XrVersion desired_version = XR_MAKE_VERSION(gl_version_major, gl_version_minor, 0);

	if (!check_graphics_api_support(desired_version)) {
		__print_line("OpenXR: Trying to initialize with OpenGL anyway...");
		// return p_next_pointer;
	}

#ifdef WIN32
	graphics_binding_gl.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR,
	graphics_binding_gl.next = p_next_pointer;

	graphics_binding_gl.hDC =
		(HDC)DisplayServer::window_get_native_handle(DisplayServerEnums::WINDOW_VIEW);
	graphics_binding_gl.hGLRC =
		(HGLRC)DisplayServer::window_get_native_handle(DisplayServerEnums::OPENGL_CONTEXT);
#elif defined(ANDROID_ENABLED)
	graphics_binding_gl.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR;
	graphics_binding_gl.next = p_next_pointer;

	graphics_binding_gl.display =
		(void*)DisplayServer::window_get_native_handle(DisplayServerEnums::DISPLAY_HANDLE);
	graphics_binding_gl.config =
		(EGLConfig)0; // https://github.com/KhronosGroup/OpenXR-SDK-Source/blob/master/src/tests/hello_xr/graphicsplugin_opengles.cpp#L122
	graphics_binding_gl.context =
		(void*)DisplayServer::window_get_native_handle(DisplayServerEnums::OPENGL_CONTEXT);
#else
#if defined(EGL_ENABLED) && defined(WAYLAND_ENABLED)
	if (DisplayServer::get_name() == "Wayland") {
		ERR_FAIL_COND_V_MSG(!data->egl_extension_enabled, p_next_pointer,
			"OpenXR cannot initialize on Wayland without the XR_MNDX_egl_enable extension.");

		graphics_binding_egl.type = XR_TYPE_GRAPHICS_BINDING_EGL_MNDX;
		graphics_binding_egl.next = p_next_pointer;

		graphics_binding_egl.getProcAddress = eglGetProcAddress;
		graphics_binding_egl.display =
			(void*)DisplayServer::window_get_native_handle(DisplayServerEnums::EGL_DISPLAY);
		graphics_binding_egl.config =
			(void*)DisplayServer::window_get_native_handle(DisplayServerEnums::EGL_CONFIG);
		graphics_binding_egl.context =
			(void*)DisplayServer::window_get_native_handle(DisplayServerEnums::OPENGL_CONTEXT);

		return &graphics_binding_egl;
	}
#endif
#if defined(X11_ENABLED)
	graphics_binding_gl.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR;
	graphics_binding_gl.next = p_next_pointer;

	void* display_handle =
		(void*)DisplayServer::window_get_native_handle(DisplayServerEnums::DISPLAY_HANDLE);
	void* glxcontext_handle =
		(void*)DisplayServer::window_get_native_handle(DisplayServerEnums::OPENGL_CONTEXT);
	void* glxdrawable_handle =
		(void*)DisplayServer::window_get_native_handle(DisplayServerEnums::WINDOW_HANDLE);
	void* glx_fbconfig_handle =
		(void*)DisplayServer::window_get_native_handle(DisplayServerEnums::GLX_FBCONFIG);
	VisualID glx_visualid =
		(VisualID)DisplayServer::window_get_native_handle(DisplayServerEnums::GLX_VISUALID);

	graphics_binding_gl.xDisplay = (Display*)display_handle;
	graphics_binding_gl.glxContext = (GLXContext)glxcontext_handle;
	graphics_binding_gl.glxDrawable = (GLXDrawable)glxdrawable_handle;
	graphics_binding_gl.glxFBConfig = (GLXFBConfig)glx_fbconfig_handle;
	graphics_binding_gl.visualid = glx_visualid;
#endif
#endif

#if defined(WIN32) || defined(ANDROID_ENABLED) || defined(X11_ENABLED)
	return &graphics_binding_gl;
#else
	return p_next_pointer;
#endif
}

void OpenXROpenGLExtension::get_usable_swapchain_formats(Vector<int64_t>& p_usable_swap_chains)
{
	p_usable_swap_chains.push_back(GL_SRGB8_ALPHA8);
	p_usable_swap_chains.push_back(GL_RGBA8);
}

void OpenXROpenGLExtension::get_usable_depth_formats(Vector<int64_t>& p_usable_depth_formats)
{
	p_usable_depth_formats.push_back(GL_DEPTH_COMPONENT32F);
	p_usable_depth_formats.push_back(GL_DEPTH24_STENCIL8);
	p_usable_depth_formats.push_back(GL_DEPTH32F_STENCIL8);
	p_usable_depth_formats.push_back(GL_DEPTH_COMPONENT24);
}

bool OpenXROpenGLExtension::get_swapchain_image_data(XrSwapchain p_swapchain,
	int64_t p_swapchain_format, uint32_t p_width, uint32_t p_height, uint32_t p_sample_count,
	uint32_t p_array_size, void** r_swapchain_graphics_data)
{
	uint32_t swapchain_length;
	XrResult result = xrEnumerateSwapchainImages(p_swapchain, 0, &swapchain_length, nullptr);
	if (XR_FAILED(result)) {
		// print_line("OpenXR: Failed to get swapchain image count [",
		// OpenXRAPI::get_error_string(result), "]");
		return false;
	}

#ifdef XR_USE_GRAPHICS_API_OPENGL_ES
	LocalVector<XrSwapchainImageOpenGLESKHR> images;
#else
	LocalVector<XrSwapchainImageOpenGLKHR> images;
#endif
	images.resize(swapchain_length);

#ifdef XR_USE_GRAPHICS_API_OPENGL_ES
	for (XrSwapchainImageOpenGLESKHR& image : images) {
		image.type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_ES_KHR;
#else
	for (XrSwapchainImageOpenGLKHR& image : images) {
		image.type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR;
#endif
		image.next = nullptr;
		image.image = 0;
	}

	result = xrEnumerateSwapchainImages(p_swapchain, swapchain_length, &swapchain_length,
		(XrSwapchainImageBaseHeader*)images.ptr());
	if (XR_FAILED(result)) {
		// print_line("OpenXR: Failed to get swapchain images [",
		// OpenXRAPI::get_error_string(result), "]");
		return false;
	}

	SwapchainGraphicsData* data = memnew(SwapchainGraphicsData);
	if (data == nullptr) {
		// print_line("OpenXR: Failed to allocate memory for swapchain data");
		return false;
	}
	*r_swapchain_graphics_data = data;
	data->is_multiview = (p_array_size > 1);

	Image::Format format = Image::FORMAT_RGBA8;

	Vector<RID> texture_rids;

	for (uint64_t i = 0; i < swapchain_length; i++) {
		RID texture_rid = GLES3::TextureStorage::texture_create_from_native_handle(
			p_array_size == 1 ? RSE::TEXTURE_TYPE_2D : RSE::TEXTURE_TYPE_LAYERED, format,
			images[i].image, p_width, p_height, 1, p_array_size);

		texture_rids.push_back(texture_rid);
	}

	data->texture_rids = texture_rids;

	return true;
}

bool OpenXROpenGLExtension::create_projection_fov(
	const XrFovf p_fov, double p_z_near, double p_z_far, Projection& r_camera_matrix)
{
	OpenXRUtil::XrMatrix4x4f matrix;
	OpenXRUtil::XrMatrix4x4f_CreateProjectionFov(&matrix, p_fov, (float)p_z_near, (float)p_z_far);

	for (int j = 0; j < 4; j++) {
		for (int i = 0; i < 4; i++) {
			r_camera_matrix.columns[j][i] = matrix.m[j * 4 + i];
		}
	}

	return true;
}

RID OpenXROpenGLExtension::get_texture(void* p_swapchain_graphics_data, int p_image_index)
{
	SwapchainGraphicsData* data = (SwapchainGraphicsData*)p_swapchain_graphics_data;
	ERR_FAIL_NULL_V(data, RID());

	ERR_FAIL_INDEX_V(p_image_index, data->texture_rids.size(), RID());
	return data->texture_rids[p_image_index];
}

void OpenXROpenGLExtension::cleanup_swapchain_graphics_data(void** p_swapchain_graphics_data)
{
	if (*p_swapchain_graphics_data == nullptr) {
		return;
	}

	SwapchainGraphicsData* data = (SwapchainGraphicsData*)*p_swapchain_graphics_data;

	for (const RID& texture_rid : data->texture_rids) {
		GLES3::TextureStorage::texture_free(texture_rid);
	}
	data->texture_rids.clear();

	memdelete(data);
	*p_swapchain_graphics_data = nullptr;
}

#define ENUM_TO_STRING_CASE(e)                                                                     \
	case e: {                                                                                      \
		return String(#e);                                                                         \
	} break;

String OpenXROpenGLExtension::get_swapchain_format_name(int64_t p_swapchain_format)
{
	// These are somewhat different per platform, will need to weed some stuff out...
	switch (p_swapchain_format) {
#ifdef XR_USE_GRAPHICS_API_OPENGL_ES
		// using definitions from GLES3/gl3.h

		ENUM_TO_STRING_CASE(GL_RGBA4)
		ENUM_TO_STRING_CASE(GL_RGB5_A1)
		ENUM_TO_STRING_CASE(GL_RGB565)
		ENUM_TO_STRING_CASE(GL_RGB8)
		ENUM_TO_STRING_CASE(GL_RGBA8)
		ENUM_TO_STRING_CASE(GL_RGB10_A2)
		ENUM_TO_STRING_CASE(GL_RGBA32F)
		ENUM_TO_STRING_CASE(GL_RGB32F)
		ENUM_TO_STRING_CASE(GL_RGBA16F)
		ENUM_TO_STRING_CASE(GL_RGB16F)
		ENUM_TO_STRING_CASE(GL_R11F_G11F_B10F)
		ENUM_TO_STRING_CASE(GL_UNSIGNED_INT_10F_11F_11F_REV)
		ENUM_TO_STRING_CASE(GL_RGB9_E5)
		ENUM_TO_STRING_CASE(GL_UNSIGNED_INT_5_9_9_9_REV)
		ENUM_TO_STRING_CASE(GL_RGBA32UI)
		ENUM_TO_STRING_CASE(GL_RGB32UI)
		ENUM_TO_STRING_CASE(GL_RGBA16UI)
		ENUM_TO_STRING_CASE(GL_RGB16UI)
		ENUM_TO_STRING_CASE(GL_RGBA8UI)
		ENUM_TO_STRING_CASE(GL_RGB8UI)
		ENUM_TO_STRING_CASE(GL_RGBA32I)
		ENUM_TO_STRING_CASE(GL_RGB32I)
		ENUM_TO_STRING_CASE(GL_RGBA16I)
		ENUM_TO_STRING_CASE(GL_RGB16I)
		ENUM_TO_STRING_CASE(GL_RGBA8I)
		ENUM_TO_STRING_CASE(GL_RGB8I)
		ENUM_TO_STRING_CASE(GL_RG)
		ENUM_TO_STRING_CASE(GL_RG_INTEGER)
		ENUM_TO_STRING_CASE(GL_R8)
		ENUM_TO_STRING_CASE(GL_RG8)
		ENUM_TO_STRING_CASE(GL_R16F)
		ENUM_TO_STRING_CASE(GL_R32F)
		ENUM_TO_STRING_CASE(GL_RG16F)
		ENUM_TO_STRING_CASE(GL_RG32F)
		ENUM_TO_STRING_CASE(GL_R8I)
		ENUM_TO_STRING_CASE(GL_R8UI)
		ENUM_TO_STRING_CASE(GL_R16I)
		ENUM_TO_STRING_CASE(GL_R16UI)
		ENUM_TO_STRING_CASE(GL_R32I)
		ENUM_TO_STRING_CASE(GL_R32UI)
		ENUM_TO_STRING_CASE(GL_RG8I)
		ENUM_TO_STRING_CASE(GL_RG8UI)
		ENUM_TO_STRING_CASE(GL_RG16I)
		ENUM_TO_STRING_CASE(GL_RG16UI)
		ENUM_TO_STRING_CASE(GL_RG32I)
		ENUM_TO_STRING_CASE(GL_RG32UI)
		ENUM_TO_STRING_CASE(GL_R8_SNORM)
		ENUM_TO_STRING_CASE(GL_RG8_SNORM)
		ENUM_TO_STRING_CASE(GL_RGB8_SNORM)
		ENUM_TO_STRING_CASE(GL_RGBA8_SNORM)
		ENUM_TO_STRING_CASE(GL_RGB10_A2UI)
		ENUM_TO_STRING_CASE(GL_SRGB)
		ENUM_TO_STRING_CASE(GL_SRGB8)
		ENUM_TO_STRING_CASE(GL_SRGB8_ALPHA8)
		ENUM_TO_STRING_CASE(GL_COMPRESSED_R11_EAC)
		ENUM_TO_STRING_CASE(GL_COMPRESSED_SIGNED_R11_EAC)
		ENUM_TO_STRING_CASE(GL_COMPRESSED_RG11_EAC)
		ENUM_TO_STRING_CASE(GL_COMPRESSED_SIGNED_RG11_EAC)
		ENUM_TO_STRING_CASE(GL_COMPRESSED_RGB8_ETC2)
		ENUM_TO_STRING_CASE(GL_COMPRESSED_SRGB8_ETC2)
		ENUM_TO_STRING_CASE(GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2)
		ENUM_TO_STRING_CASE(GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2)
		ENUM_TO_STRING_CASE(GL_COMPRESSED_RGBA8_ETC2_EAC)
		ENUM_TO_STRING_CASE(GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC)
		ENUM_TO_STRING_CASE(GL_DEPTH_COMPONENT16)
		ENUM_TO_STRING_CASE(GL_DEPTH_COMPONENT24)
		ENUM_TO_STRING_CASE(GL_DEPTH24_STENCIL8)

#else
		// using definitions from GLAD
		ENUM_TO_STRING_CASE(GL_R8_SNORM)
		ENUM_TO_STRING_CASE(GL_RG8_SNORM)
		ENUM_TO_STRING_CASE(GL_RGB8_SNORM)
		ENUM_TO_STRING_CASE(GL_RGBA8_SNORM)
		ENUM_TO_STRING_CASE(GL_R16_SNORM)
		ENUM_TO_STRING_CASE(GL_RG16_SNORM)
		ENUM_TO_STRING_CASE(GL_RGB16_SNORM)
		ENUM_TO_STRING_CASE(GL_RGBA16_SNORM)
		ENUM_TO_STRING_CASE(GL_RGB4)
		ENUM_TO_STRING_CASE(GL_RGB5)
		ENUM_TO_STRING_CASE(GL_RGB8)
		ENUM_TO_STRING_CASE(GL_RGB10)
		ENUM_TO_STRING_CASE(GL_RGB12)
		ENUM_TO_STRING_CASE(GL_RGB16)
		ENUM_TO_STRING_CASE(GL_RGBA2)
		ENUM_TO_STRING_CASE(GL_RGBA4)
		ENUM_TO_STRING_CASE(GL_RGB5_A1)
		ENUM_TO_STRING_CASE(GL_RGBA8)
		ENUM_TO_STRING_CASE(GL_RGB10_A2)
		ENUM_TO_STRING_CASE(GL_RGBA12)
		ENUM_TO_STRING_CASE(GL_RGBA16)
		ENUM_TO_STRING_CASE(GL_RGBA32F)
		ENUM_TO_STRING_CASE(GL_RGB32F)
		ENUM_TO_STRING_CASE(GL_RGBA16F)
		ENUM_TO_STRING_CASE(GL_RGB16F)
		ENUM_TO_STRING_CASE(GL_RGBA32UI)
		ENUM_TO_STRING_CASE(GL_RGB32UI)
		ENUM_TO_STRING_CASE(GL_RGBA16UI)
		ENUM_TO_STRING_CASE(GL_RGB16UI)
		ENUM_TO_STRING_CASE(GL_RGBA8UI)
		ENUM_TO_STRING_CASE(GL_RGB8UI)
		ENUM_TO_STRING_CASE(GL_RGBA32I)
		ENUM_TO_STRING_CASE(GL_RGB32I)
		ENUM_TO_STRING_CASE(GL_RGBA16I)
		ENUM_TO_STRING_CASE(GL_RGB16I)
		ENUM_TO_STRING_CASE(GL_RGBA8I)
		ENUM_TO_STRING_CASE(GL_RGB8I)
		ENUM_TO_STRING_CASE(GL_RGB10_A2UI)
		ENUM_TO_STRING_CASE(GL_SRGB)
		ENUM_TO_STRING_CASE(GL_SRGB8)
		ENUM_TO_STRING_CASE(GL_SRGB_ALPHA)
		ENUM_TO_STRING_CASE(GL_SRGB8_ALPHA8)
		ENUM_TO_STRING_CASE(GL_DEPTH_COMPONENT16)
		ENUM_TO_STRING_CASE(GL_DEPTH_COMPONENT24)
		ENUM_TO_STRING_CASE(GL_DEPTH_COMPONENT32)
		ENUM_TO_STRING_CASE(GL_DEPTH24_STENCIL8)
		ENUM_TO_STRING_CASE(GL_R11F_G11F_B10F)
		ENUM_TO_STRING_CASE(GL_DEPTH_COMPONENT32F)
		ENUM_TO_STRING_CASE(GL_DEPTH32F_STENCIL8)
#endif
	default: {
		return String("Swapchain format 0x") + String::num_int64(p_swapchain_format, 16);
	} break;
	}
}

bool OpenXROpenGLExtension::check_graphics_api_support(unsigned long p_api) { return false; }

#endif // GLES3_ENABLED
#ifdef ENUM_TO_STRING_CASE
#undef ENUM_TO_STRING_CASE
#endif
#endif // defined(GLES3_ENABLED) && !defined(MACOS_ENABLED)
#endif // MODULE_OPENXR_ENABLED
