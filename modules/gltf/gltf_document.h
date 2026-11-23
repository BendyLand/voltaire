/**************************************************************************/
/*  gltf_document.h                                                       */
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

#pragma once

// --- Inlined/Folded from modules/gltf/gltf_state.h ---
// --- Inlined/Folded from modules/gltf/gltf_extensions.h ---
#include "core/io/image.h"
#include "core/io/resource.h"

// --- Inlined/Folded from modules/gltf/gltf_structures.h ---
#include "core/io/resource.h"
#include "core/types.h"
// --- Inlined/Folded from modules/gltf/gltf_defines.h ---
// This file should only be included by other headers.

// GLTF classes.
class GLTFAccessor;
class GLTFAnimation;
class GLTFBufferView;
class GLTFCamera;
class GLTFDocument;
class GLTFDocumentExtension;
class GLTFLight;
class GLTFMesh;
class GLTFNode;
class GLTFObjectModelProperty;
class GLTFSkeleton;
class GLTFSkin;
class GLTFSpecGloss;
class GLTFState;
class GLTFTexture;
class GLTFTextureSampler;

// GLTF index aliases.
using GLTFAccessorIndex = int;
using GLTFAnimationIndex = int;
using GLTFBufferIndex = int;
using GLTFBufferViewIndex = int;
using GLTFCameraIndex = int;
using GLTFImageIndex = int;
using GLTFLightIndex = int;
using GLTFMaterialIndex = int;
using GLTFMeshIndex = int;
using GLTFNodeIndex = int;
using GLTFSkeletonIndex = int;
using GLTFSkinIndex = int;
using GLTFTextureIndex = int;
using GLTFTextureSamplerIndex = int;

class Camera3D;
template <typename T> class TypedArray;

#include "scene/3d/node_3d.h"
#include "scene/resources/mesh.h"
#include "scene/3d/node_3d.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/material.h"

class GLTFTextureSampler final : public Resource
{
public:
	enum FilterMode
	{
		NEAREST = 9728,
		LINEAR = 9729,
		NEAREST_MIPMAP_NEAREST = 9984,
		LINEAR_MIPMAP_NEAREST = 9985,
		NEAREST_MIPMAP_LINEAR = 9986,
		LINEAR_MIPMAP_LINEAR = 9987
	};

	enum WrapMode
	{
		CLAMP_TO_EDGE = 33071,
		MIRRORED_REPEAT = 33648,
		REPEAT = 10497,
		DEFAULT = REPEAT
	};

	int get_mag_filter() const { return mag_filter; }

	void set_mag_filter(const int filter_mode) { mag_filter = (FilterMode)filter_mode; }

	int get_min_filter() const { return min_filter; }

	void set_min_filter(const int filter_mode) { min_filter = (FilterMode)filter_mode; }

	int get_wrap_s() const { return wrap_s; }

	void set_wrap_s(const int wrap_mode) { wrap_s = (WrapMode)wrap_mode; }

	int get_wrap_t() const { return wrap_t; }

	void set_wrap_t(const int wrap_mode) { wrap_s = (WrapMode)wrap_mode; }

	StandardMaterial3D::TextureFilter get_filter_mode() const
	{
		using TextureFilter = StandardMaterial3D::TextureFilter;

		switch (min_filter) {
		case NEAREST:
			return TextureFilter::TEXTURE_FILTER_NEAREST;
		case LINEAR:
			return TextureFilter::TEXTURE_FILTER_LINEAR;
		case NEAREST_MIPMAP_NEAREST:
		case NEAREST_MIPMAP_LINEAR:
			return TextureFilter::TEXTURE_FILTER_NEAREST_WITH_MIPMAPS;
		case LINEAR_MIPMAP_NEAREST:
		case LINEAR_MIPMAP_LINEAR:
		default:
			return TextureFilter::TEXTURE_FILTER_LINEAR_WITH_MIPMAPS;
		}
	}

	void set_filter_mode(StandardMaterial3D::TextureFilter mode)
	{
		using TextureFilter = StandardMaterial3D::TextureFilter;

		switch (mode) {
		case TextureFilter::TEXTURE_FILTER_NEAREST:
			min_filter = FilterMode::NEAREST;
			mag_filter = FilterMode::NEAREST;
			break;
		case TextureFilter::TEXTURE_FILTER_LINEAR:
			min_filter = FilterMode::LINEAR;
			mag_filter = FilterMode::LINEAR;
			break;
		case TextureFilter::TEXTURE_FILTER_NEAREST_WITH_MIPMAPS:
		case TextureFilter::TEXTURE_FILTER_NEAREST_WITH_MIPMAPS_ANISOTROPIC:
			min_filter = FilterMode::NEAREST_MIPMAP_LINEAR;
			mag_filter = FilterMode::NEAREST;
			break;
		case TextureFilter::TEXTURE_FILTER_LINEAR_WITH_MIPMAPS:
		case TextureFilter::TEXTURE_FILTER_LINEAR_WITH_MIPMAPS_ANISOTROPIC:
		default:
			min_filter = FilterMode::LINEAR_MIPMAP_LINEAR;
			mag_filter = FilterMode::LINEAR;
			break;
		}
	}

	bool get_wrap_mode() const
	{
		// StandardMaterial3D presents wrapping as a boolean property. Either the texture is repeated
		// in both dimensions, non-mirrored, or it isn't repeated at all. This will cause oddities
		// when people import models having other wrapping mode combinations.
		return (wrap_s == WrapMode::REPEAT) && (wrap_t == WrapMode::REPEAT);
	}

	void set_wrap_mode(bool mat_repeats)
	{
		if (mat_repeats) {
			wrap_s = WrapMode::REPEAT;
			wrap_t = WrapMode::REPEAT;
		}
		else {
			wrap_s = WrapMode::CLAMP_TO_EDGE;
			wrap_t = WrapMode::CLAMP_TO_EDGE;
		}
	}

private:
	FilterMode mag_filter = FilterMode::LINEAR;
	FilterMode min_filter = FilterMode::LINEAR_MIPMAP_LINEAR;
	WrapMode wrap_s = WrapMode::REPEAT;
	WrapMode wrap_t = WrapMode::REPEAT;
};

class GLTFTexture final : public Resource
{
private:
	GLTFImageIndex src_image = -1;
	GLTFTextureSamplerIndex sampler = -1;

public:
	GLTFImageIndex get_src_image() const;
	void set_src_image(GLTFImageIndex val);
	GLTFTextureSamplerIndex get_sampler() const;
	void set_sampler(GLTFTextureSamplerIndex val);
};

class GLTFBufferView final : public Resource
{
	friend class GLTFDocument;

public:
	// When a buffer view is used by vertex indices or attribute accessors it SHOULD specify
	// "target" with a value of ELEMENT_ARRAY_BUFFER (34963) or ARRAY_BUFFER (34962) respectively.
	// This is only used for mesh data. For non-mesh buffer views, the target should be left blank.
	// https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#buffers-and-buffer-views-overview
	enum ArrayBufferTarget
	{
		TARGET_NONE = 0,
		TARGET_ARRAY_BUFFER = 34962,
		TARGET_ELEMENT_ARRAY_BUFFER = 34963,
	};

private:
	GLTFBufferIndex buffer = -1;
	int64_t byte_offset = 0;
	int64_t byte_length = 0;
	int64_t byte_stride = -1;
	bool indices = false;			// True for TARGET_ELEMENT_ARRAY_BUFFER.
	bool vertex_attributes = false; // True for TARGET_ARRAY_BUFFER.

public:
	GLTFBufferIndex get_buffer() const;
	void set_buffer(GLTFBufferIndex p_buffer);

	int64_t get_byte_offset() const;
	void set_byte_offset(int64_t p_byte_offset);

	int64_t get_byte_length() const;
	void set_byte_length(int64_t p_byte_length);

	int64_t get_byte_stride() const;
	void set_byte_stride(int64_t p_byte_stride);

	bool get_indices() const;
	void set_indices(bool p_indices);

	bool get_vertex_attributes() const;
	void set_vertex_attributes(bool p_attributes);

	Vector<uint8_t> load_buffer_view_data(const Ref<GLTFState> p_gltf_state) const;
	static GLTFBufferViewIndex write_new_buffer_view_into_state(const Ref<GLTFState>& p_gltf_state,
		const PackedByteArray& p_input_data, const int64_t p_alignment = 1,
		const ArrayBufferTarget p_target = TARGET_NONE, const int64_t p_byte_stride = -1,
		const GLTFBufferIndex p_buffer_index = 0, const bool p_deduplicate = true);
};

class GLTFAccessor final : public Resource
{
	friend class GLTFDocument;

public:
	enum GLTFAccessorType
	{
		TYPE_SCALAR,
		TYPE_VEC2,
		TYPE_VEC3,
		TYPE_VEC4,
		TYPE_MAT2,
		TYPE_MAT3,
		TYPE_MAT4,
	};

	enum GLTFComponentType
	{
		COMPONENT_TYPE_NONE = 0,
		COMPONENT_TYPE_SIGNED_BYTE = 5120,
		COMPONENT_TYPE_UNSIGNED_BYTE = 5121,
		COMPONENT_TYPE_SIGNED_SHORT = 5122,
		COMPONENT_TYPE_UNSIGNED_SHORT = 5123,
		COMPONENT_TYPE_SIGNED_INT = 5124,
		COMPONENT_TYPE_UNSIGNED_INT = 5125,
		COMPONENT_TYPE_SINGLE_FLOAT = 5126,
		COMPONENT_TYPE_DOUBLE_FLOAT = 5130,
		COMPONENT_TYPE_HALF_FLOAT = 5131,
		COMPONENT_TYPE_SIGNED_LONG = 5134,
		COMPONENT_TYPE_UNSIGNED_LONG = 5135,
	};

private:
	GLTFBufferViewIndex buffer_view = -1;
	int64_t byte_offset = 0;
	GLTFComponentType component_type = COMPONENT_TYPE_NONE;
	bool normalized = false;
	int64_t count = 0;
	GLTFAccessorType accessor_type = GLTFAccessorType::TYPE_SCALAR;
	Vector<double> min;
	Vector<double> max;
	int64_t sparse_count = 0;
	GLTFBufferViewIndex sparse_indices_buffer_view = 0;
	int64_t sparse_indices_byte_offset = 0;
	GLTFComponentType sparse_indices_component_type = COMPONENT_TYPE_NONE;
	GLTFBufferViewIndex sparse_values_buffer_view = 0;
	int64_t sparse_values_byte_offset = 0;

	// Trivial helper functions.
	void _calculate_min_and_max(const PackedFloat64Array& p_numbers);
	void _determine_pad_skip(int64_t& r_skip_every, int64_t& r_skip_bytes) const;
	int64_t _determine_padded_byte_count(int64_t p_raw_byte_size) const;
	PackedFloat64Array _filter_numbers(const PackedFloat64Array& p_numbers) const;
	static String _get_component_type_name(const GLTFComponentType p_component);
	static GLTFComponentType _get_indices_component_type_for_size(const int64_t p_size);
	static GLTFAccessorType _get_accessor_type_from_str(const String& p_string);
	String _get_accessor_type_name() const;
	int64_t _get_vector_size() const;
	static int64_t _get_bytes_per_component(const GLTFComponentType p_component_type);
	int64_t _get_bytes_per_vector() const;

	// Private decode functions.
	PackedInt64Array _decode_sparse_indices(const Ref<GLTFState>& p_gltf_state,
		const Vector<Ref<GLTFBufferView>>& p_buffer_views) const;
	template <typename T>
	Vector<T> _decode_raw_numbers(const Ref<GLTFState>& p_gltf_state,
		const Vector<Ref<GLTFBufferView>>& p_buffer_views, bool p_sparse_values) const;
	template <typename T> Vector<T> _decode_as_numbers(const Ref<GLTFState>& p_gltf_state) const;

	void _store_sparse_indices_into_state(const Ref<GLTFState>& p_gltf_state,
		const PackedInt64Array& p_sparse_indices, const bool p_deduplicate = true);

public:
	// Property getters and setters.
	GLTFBufferViewIndex get_buffer_view() const;
	void set_buffer_view(GLTFBufferViewIndex p_buffer_view);

	int64_t get_byte_offset() const;
	void set_byte_offset(int64_t p_byte_offset);

	GLTFComponentType get_component_type() const;
	void set_component_type(GLTFComponentType p_component_type);

	bool get_normalized() const;
	void set_normalized(bool p_normalized);

	int64_t get_count() const;
	void set_count(int64_t p_count);

	GLTFAccessorType get_accessor_type() const;
	void set_accessor_type(GLTFAccessorType p_accessor_type);

	int get_type() const;
	void set_type(int p_accessor_type);

	Vector<double> get_min() const;
	void set_min(const Vector<double>& p_min);

	Vector<double> get_max() const;
	void set_max(const Vector<double>& p_max);

	int64_t get_sparse_count() const;
	void set_sparse_count(int64_t p_sparse_count);

	GLTFBufferViewIndex get_sparse_indices_buffer_view() const;
	void set_sparse_indices_buffer_view(GLTFBufferViewIndex p_sparse_indices_buffer_view);

	int64_t get_sparse_indices_byte_offset() const;
	void set_sparse_indices_byte_offset(int64_t p_sparse_indices_byte_offset);

	GLTFComponentType get_sparse_indices_component_type() const;
	void set_sparse_indices_component_type(GLTFComponentType p_sparse_indices_component_type);

	GLTFBufferViewIndex get_sparse_values_buffer_view() const;
	void set_sparse_values_buffer_view(GLTFBufferViewIndex p_sparse_values_buffer_view);

	int64_t get_sparse_values_byte_offset() const;
	void set_sparse_values_byte_offset(int64_t p_sparse_values_byte_offset);

	bool is_equal_exact(const Ref<GLTFAccessor>& p_other) const;

	// High-level decode functions.
	PackedColorArray decode_as_colors(const Ref<GLTFState>& p_gltf_state) const;
	PackedFloat32Array decode_as_float32s(const Ref<GLTFState>& p_gltf_state) const;
	PackedFloat64Array decode_as_float64s(const Ref<GLTFState>& p_gltf_state) const;
	PackedInt32Array decode_as_int32s(const Ref<GLTFState>& p_gltf_state) const;
	PackedInt64Array decode_as_int64s(const Ref<GLTFState>& p_gltf_state) const;
	Vector<Quaternion> decode_as_quaternions(const Ref<GLTFState>& p_gltf_state) const;
	PackedVector2Array decode_as_vector2s(const Ref<GLTFState>& p_gltf_state) const;
	PackedVector3Array decode_as_vector3s(const Ref<GLTFState>& p_gltf_state) const;
	PackedVector4Array decode_as_vector4s(const Ref<GLTFState>& p_gltf_state) const;

	// Low-level encode functions.
	static GLTFComponentType get_minimal_integer_component_type_from_ints(
		const PackedInt64Array& p_numbers);
	PackedByteArray encode_floats_as_bytes(const PackedFloat64Array& p_input_numbers);
	PackedByteArray encode_ints_as_bytes(const PackedInt64Array& p_input_numbers);

	GLTFAccessorIndex store_accessor_data_into_state(const Ref<GLTFState>& p_gltf_state,
		const PackedByteArray& p_data_bytes,
		const GLTFBufferView::ArrayBufferTarget p_buffer_view_target = GLTFBufferView::TARGET_NONE,
		const GLTFBufferIndex p_buffer_index = 0, const bool p_deduplicate = true);
	static Ref<GLTFAccessor> make_new_accessor_without_data(
		GLTFAccessorType p_accessor_type = TYPE_SCALAR,
		GLTFComponentType p_component_type = COMPONENT_TYPE_SINGLE_FLOAT);

	// High-level encode functions.
	static GLTFAccessorIndex encode_new_accessor_from_colors(const Ref<GLTFState>& p_gltf_state,
		const PackedColorArray& p_input_data,
		const GLTFBufferView::ArrayBufferTarget p_buffer_view_target = GLTFBufferView::TARGET_NONE,
		const bool p_deduplicate = true);
	static GLTFAccessorIndex encode_new_accessor_from_float64s(const Ref<GLTFState>& p_gltf_state,
		const PackedFloat64Array& p_input_data,
		const GLTFBufferView::ArrayBufferTarget p_buffer_view_target = GLTFBufferView::TARGET_NONE,
		const bool p_deduplicate = true);
	static GLTFAccessorIndex encode_new_accessor_from_int32s(const Ref<GLTFState>& p_gltf_state,
		const PackedInt32Array& p_input_data,
		const GLTFBufferView::ArrayBufferTarget p_buffer_view_target = GLTFBufferView::TARGET_NONE,
		const bool p_deduplicate = true);
	static GLTFAccessorIndex encode_new_accessor_from_int64s(const Ref<GLTFState>& p_gltf_state,
		const PackedInt64Array& p_input_data,
		const GLTFBufferView::ArrayBufferTarget p_buffer_view_target = GLTFBufferView::TARGET_NONE,
		const bool p_deduplicate = true);
	static GLTFAccessorIndex encode_new_accessor_from_quaternions(
		const Ref<GLTFState>& p_gltf_state, const Vector<Quaternion>& p_input_data,
		const GLTFBufferView::ArrayBufferTarget p_buffer_view_target = GLTFBufferView::TARGET_NONE,
		const bool p_deduplicate = true);
	static GLTFAccessorIndex encode_new_accessor_from_vector2s(const Ref<GLTFState>& p_gltf_state,
		const PackedVector2Array& p_input_data,
		const GLTFBufferView::ArrayBufferTarget p_buffer_view_target = GLTFBufferView::TARGET_NONE,
		const bool p_deduplicate = true);
	static GLTFAccessorIndex encode_new_accessor_from_vector3s(const Ref<GLTFState>& p_gltf_state,
		const PackedVector3Array& p_input_data,
		const GLTFBufferView::ArrayBufferTarget p_buffer_view_target = GLTFBufferView::TARGET_NONE,
		const bool p_deduplicate = true);
	static GLTFAccessorIndex encode_new_accessor_from_vector4s(const Ref<GLTFState>& p_gltf_state,
		const PackedVector4Array& p_input_data,
		const GLTFBufferView::ArrayBufferTarget p_buffer_view_target = GLTFBufferView::TARGET_NONE,
		const bool p_deduplicate = true);
	static GLTFAccessorIndex encode_new_accessor_from_vector4is(const Ref<GLTFState>& p_gltf_state,
		const Vector<Vector4i>& p_input_data,
		const GLTFBufferView::ArrayBufferTarget p_buffer_view_target = GLTFBufferView::TARGET_NONE,
		const bool p_deduplicate = true);
	static GLTFAccessorIndex encode_new_sparse_accessor_from_vec3s(
		const Ref<GLTFState>& p_gltf_state, const PackedVector3Array& p_input_data,
		const PackedVector3Array& p_base_reference_data, const double p_tolerance_multiplier = 1.0,
		const GLTFBufferView::ArrayBufferTarget p_main_buffer_view_target =
			GLTFBufferView::TARGET_NONE,
		const bool p_deduplicate = true);
};

class GLTFNode final : public Resource
{
	friend class GLTFDocument;
	friend class SkinTool;
	friend class FBXDocument;

private:
	String original_name;
	GLTFNodeIndex parent = -1;
	int height = -1;
	Transform3D transform;
	GLTFMeshIndex mesh = -1;
	GLTFCameraIndex camera = -1;
	GLTFSkinIndex skin = -1;
	GLTFSkeletonIndex skeleton = -1;
	bool joint = false;
	bool visible = true;
	Vector<int> children;
	GLTFLightIndex light = -1;

public:
	String get_original_name();
	void set_original_name(const String& p_name);

	GLTFNodeIndex get_parent();
	void set_parent(GLTFNodeIndex p_parent);

	int get_height();
	void set_height(int p_height);

	Transform3D get_xform();
	void set_xform(const Transform3D& p_xform);

	Transform3D get_rest_xform();
	void set_rest_xform(const Transform3D& p_rest_xform);

	GLTFMeshIndex get_mesh();
	void set_mesh(GLTFMeshIndex p_mesh);

	GLTFCameraIndex get_camera();
	void set_camera(GLTFCameraIndex p_camera);

	GLTFSkinIndex get_skin();
	void set_skin(GLTFSkinIndex p_skin);

	GLTFSkeletonIndex get_skeleton();
	void set_skeleton(GLTFSkeletonIndex p_skeleton);

	Vector3 get_position();
	void set_position(const Vector3& p_position);

	Quaternion get_rotation();
	void set_rotation(const Quaternion& p_rotation);

	Vector3 get_scale();
	void set_scale(const Vector3& p_scale);

	Vector<int> get_children();
	void set_children(const Vector<int>& p_children);
	void append_child_index(int p_child_index);

	GLTFLightIndex get_light();
	void set_light(GLTFLightIndex p_light);

	bool get_visible();
	void set_visible(bool p_visible);

	bool has_additional_data(const String& p_extension_name);

	NodePath get_scene_node_path(Ref<GLTFState> p_state, bool p_handle_skeletons = true);
};

class GLTFSkeleton final : public Resource
{
	friend class GLTFDocument;
	friend class SkinTool;
	friend class FBXDocument;

private:
	// The *synthesized* skeletons joints
	Vector<GLTFNodeIndex> joints;

	// The roots of the skeleton. If there are multiple, each root must have the
	// same parent (ie roots are siblings)
	Vector<GLTFNodeIndex> roots;

	// The created Skeleton3D for the scene
	Skeleton3D* godot_skeleton = nullptr;

	// Set of unique bone names for the skeleton
	HashSet<String> unique_names;

	HashMap<int32_t, GLTFNodeIndex> godot_bone_node;

	Vector<BoneAttachment3D*> bone_attachments;

public:
	Vector<GLTFNodeIndex> get_joints();
	void set_joints(const Vector<GLTFNodeIndex>& p_joints);

	Vector<GLTFNodeIndex> get_roots();
	void set_roots(const Vector<GLTFNodeIndex>& p_roots);

	Skeleton3D* get_godot_skeleton();

	TypedArray<String> get_unique_names();
	void set_unique_names(const TypedArray<String>& p_unique_names);

	BoneAttachment3D* get_bone_attachment(int idx);
	int32_t get_bone_attachment_count();
};

class GLTFSkin final : public Resource
{
	friend class GLTFDocument;
	friend class SkinTool;
	friend class FBXDocument;

private:
	// The "skeleton" property defined in the gltf spec. -1 = Scene Root
	GLTFNodeIndex skin_root = -1;

	Vector<GLTFNodeIndex> joints_original;
	Vector<Transform3D> inverse_binds;

	// Note: joints + non_joints should form a complete subtree, or subtrees
	// with a common parent

	// All nodes that are skins that are caught in-between the original joints
	// (inclusive of joints_original)
	Vector<GLTFNodeIndex> joints;

	// All Nodes that are caught in-between skin joint nodes, and are not
	// defined as joints by any skin
	Vector<GLTFNodeIndex> non_joints;

	// The roots of the skin. In the case of multiple roots, their parent *must*
	// be the same (the roots must be siblings)
	Vector<GLTFNodeIndex> roots;

	// The GLTF Skeleton this Skin points to (after we determine skeletons)
	GLTFSkeletonIndex skeleton = -1;

	// A mapping from the joint indices (in the order of joints_original) to the
	// Godot Skeleton's bone_indices
	HashMap<int, int> joint_i_to_bone_i;
	HashMap<int, String> joint_i_to_name;

	// The Actual Skin that will be created as a mapping between the IBM's of
	// this skin to the generated skeleton for the mesh instances.
	Ref<Skin> godot_skin;

public:
	GLTFNodeIndex get_skin_root();
	void set_skin_root(GLTFNodeIndex p_skin_root);

	Vector<GLTFNodeIndex> get_joints_original();
	void set_joints_original(const Vector<GLTFNodeIndex>& p_joints_original);

	TypedArray<Transform3D> get_inverse_binds();
	void set_inverse_binds(const TypedArray<Transform3D>& p_inverse_binds);

	Vector<GLTFNodeIndex> get_joints();
	void set_joints(const Vector<GLTFNodeIndex>& p_joints);

	Vector<GLTFNodeIndex> get_non_joints();
	void set_non_joints(const Vector<GLTFNodeIndex>& p_non_joints);

	Vector<GLTFNodeIndex> get_roots();
	void set_roots(const Vector<GLTFNodeIndex>& p_roots);

	int get_skeleton();
	void set_skeleton(int p_skeleton);

	Ref<Skin> get_godot_skin();
	void set_godot_skin(const Ref<Skin>& p_godot_skin);
};

class GLTFMesh final : public Resource
{
private:
	String original_name;
	Ref<ImporterMesh> mesh;
	Vector<float> blend_weights;

public:
	String get_original_name();
	void set_original_name(const String& p_name);
	Ref<ImporterMesh> get_mesh();
	void set_mesh(const Ref<ImporterMesh>& p_mesh);
	Vector<float> get_blend_weights();
	void set_blend_weights(const Vector<float>& p_blend_weights);
};

// Reference and test file:
// https://github.com/KhronosGroup/glTF-Tutorials/blob/master/gltfTutorial/gltfTutorial_015_SimpleCameras.md

class GLTFCamera final : public Resource
{
private:
	// glTF has no default camera values, they should always be specified in
	// the glTF file. Here we default to Godot's default camera settings.
	bool perspective = true;
	real_t fov = Math::deg_to_rad(75.0);
	real_t size_mag = 0.5;
	real_t depth_far = 4000.0;
	real_t depth_near = 0.05;

public:
	static void set_fov_conversion_expressions(Ref<GLTFObjectModelProperty>& r_obj_model_prop);

	bool get_perspective() const { return perspective; }
	void set_perspective(bool p_val) { perspective = p_val; }

	real_t get_fov() const { return fov; }
	void set_fov(real_t p_val) { fov = p_val; }

	real_t get_size_mag() const { return size_mag; }
	void set_size_mag(real_t p_val) { size_mag = p_val; }

	real_t get_depth_far() const { return depth_far; }
	void set_depth_far(real_t p_val) { depth_far = p_val; }

	real_t get_depth_near() const { return depth_near; }
	void set_depth_near(real_t p_val) { depth_near = p_val; }

	static Ref<GLTFCamera> from_node(const Camera3D* p_camera);
};

class GLTFAnimation final : public Resource
{
public:
	enum Interpolation
	{
		INTERP_LINEAR,
		INTERP_STEP,
		INTERP_CATMULLROMSPLINE,
		INTERP_CUBIC_SPLINE,
	};

	template <typename T> struct Channel
	{
		Interpolation interpolation = INTERP_LINEAR;
		Vector<double> times;
		Vector<T> values;
	};

	struct NodeTrack
	{
		Channel<Vector3> position_track;
		Channel<Quaternion> rotation_track;
		Channel<Vector3> scale_track;
		Vector<Channel<real_t>> weight_tracks;
	};

	String original_name;
	bool loop = false;
	HashMap<int, NodeTrack> node_tracks;

public:
	static Interpolation godot_to_gltf_interpolation(
		const Ref<Animation>& p_godot_animation, int32_t p_godot_anim_track_index);
	static Animation::InterpolationType gltf_to_godot_interpolation(
		Interpolation p_gltf_interpolation);

	String get_original_name();
	void set_original_name(const String& p_name);

	bool get_loop() const;
	void set_loop(bool p_val);

	HashMap<int, GLTFAnimation::NodeTrack>& get_node_tracks();
	bool is_empty_of_tracks() const;

	GLTFAnimation();
};

// Object model: https://github.com/KhronosGroup/glTF/blob/main/specification/2.0/ObjectModel.adoc
// KHR_animation_pointer:
// https://github.com/KhronosGroup/glTF/tree/main/extensions/2.0/Khronos/KHR_animation_pointer

class GLTFObjectModelProperty final : public RefCounted
{
public:
	enum GLTFObjectModelType
	{
		GLTF_OBJECT_MODEL_TYPE_UNKNOWN,
		GLTF_OBJECT_MODEL_TYPE_BOOL,
		GLTF_OBJECT_MODEL_TYPE_FLOAT,
		GLTF_OBJECT_MODEL_TYPE_FLOAT_ARRAY,
		GLTF_OBJECT_MODEL_TYPE_FLOAT2,
		GLTF_OBJECT_MODEL_TYPE_FLOAT3,
		GLTF_OBJECT_MODEL_TYPE_FLOAT4,
		GLTF_OBJECT_MODEL_TYPE_FLOAT2X2,
		GLTF_OBJECT_MODEL_TYPE_FLOAT3X3,
		GLTF_OBJECT_MODEL_TYPE_FLOAT4X4,
		GLTF_OBJECT_MODEL_TYPE_INT,
	};

private:
	Ref<Expression> gltf_to_godot_expr;
	Ref<Expression> godot_to_gltf_expr;
	GLTFObjectModelType object_model_type = GLTF_OBJECT_MODEL_TYPE_UNKNOWN;
	Vector<PackedStringArray> json_pointers;

public:
	void append_node_path(const NodePath& p_node_path);
	void append_path_to_property(const NodePath& p_node_path, const String& p_prop_name);

	GLTFAccessor::GLTFAccessorType get_accessor_type() const;

	Ref<Expression> get_gltf_to_godot_expression() const;
	void set_gltf_to_godot_expression(const Ref<Expression>& p_gltf_to_godot_expr);

	Ref<Expression> get_godot_to_gltf_expression() const;
	void set_godot_to_gltf_expression(const Ref<Expression>& p_godot_to_gltf_expr);

	bool has_node_paths() const;

	GLTFObjectModelType get_object_model_type() const;
	void set_object_model_type(GLTFObjectModelType p_type);

	Vector<PackedStringArray> get_json_pointers() const;
	bool has_json_pointers() const;
	void set_json_pointers(const Vector<PackedStringArray>& p_json_pointers);
};
#include "scene/3d/node_3d.h"

class Light3D;

// https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_lights_punctual

class GLTFLight final : public Resource
{
	friend class GLTFDocument;

private:
	Color color = Color(1.0f, 1.0f, 1.0f);
	float intensity = 1.0f;
	String light_type;
	float range = Math::INF;
	float inner_cone_angle = 0.0f;
	float outer_cone_angle = Math::TAU / 8.0f;

public:
	static void set_cone_inner_attenuation_conversion_expressions(
		Ref<GLTFObjectModelProperty>& r_obj_model_prop);

	Color get_color();
	void set_color(Color p_color);

	float get_intensity();
	void set_intensity(float p_intensity);

	String get_light_type();
	void set_light_type(String p_light_type);

	float get_range();
	void set_range(float p_range);

	float get_inner_cone_angle();
	void set_inner_cone_angle(float p_inner_cone_angle);

	float get_outer_cone_angle();
	void set_outer_cone_angle(float p_outer_cone_angle);

	static Ref<GLTFLight> from_node(const Light3D* p_light);
};

// KHR_materials_pbrSpecularGlossiness is an archived GLTF extension.
// https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Archived/KHR_materials_pbrSpecularGlossiness

class GLTFSpecGloss final : public Resource
{
	friend class GLTFDocument;

private:
	Ref<Image> diffuse_img = nullptr;
	Color diffuse_factor = Color(1.0f, 1.0f, 1.0f);
	float gloss_factor = 1.0f;
	Color specular_factor = Color(1.0f, 1.0f, 1.0f);
	Ref<Image> spec_gloss_img = nullptr;

public:
	Ref<Image> get_diffuse_img();
	void set_diffuse_img(Ref<Image> p_diffuse_img);

	Color get_diffuse_factor();
	void set_diffuse_factor(Color p_diffuse_factor);

	float get_gloss_factor();
	void set_gloss_factor(float p_gloss_factor);

	Color get_specular_factor();
	void set_specular_factor(Color p_specular_factor);

	Ref<Image> get_spec_gloss_img();
	void set_spec_gloss_img(Ref<Image> p_spec_gloss_img);
};



class GLTFDocumentExtension : public Resource
{
public:
	// Import process.
	virtual Error import_preflight(Ref<GLTFState> p_state, const Vector<String>& p_extensions);
	virtual Vector<String> get_supported_extensions();
	virtual Error parse_image_data(Ref<GLTFState> p_state, const PackedByteArray& p_image_data,
		const String& p_mime_type, Ref<Image> r_image);
	virtual String get_image_file_extension();
	virtual Ref<GLTFObjectModelProperty> import_object_model_property(Ref<GLTFState> p_state,
		const PackedStringArray& p_split_json_pointer, const TypedArray<NodePath>& p_partial_paths);
	virtual Error import_post_parse(Ref<GLTFState> p_state);
	virtual Error import_pre_generate(Ref<GLTFState> p_state);
	virtual Node3D* generate_scene_node(
		Ref<GLTFState> p_state, Ref<GLTFNode> p_gltf_node, Node* p_scene_parent);
	virtual Error import_post(Ref<GLTFState> p_state, Node* p_node);
	// Export process.
	virtual Error export_preflight(Ref<GLTFState> p_state, Node* p_root);
	virtual void convert_scene_node(
		Ref<GLTFState> p_state, Ref<GLTFNode> p_gltf_node, Node* p_scene_node);
	virtual Error export_post_convert(Ref<GLTFState> p_state, Node* p_root);
	virtual Error export_preserialize(Ref<GLTFState> p_state);
	virtual Vector<String> get_saveable_image_formats();
	virtual Error save_image_at_path(Ref<GLTFState> p_state, Ref<Image> p_image,
		const String& p_file_path, const String& p_image_format, float p_lossy_quality);
	virtual Error export_post(Ref<GLTFState> p_state);
};

class GLTFDocumentExtensionTexture final : public GLTFDocumentExtension
{
public:
	enum Format
	{
		FORMAT_KTX,
		FORMAT_WEBP,
	};

private:
	Format format = FORMAT_KTX;

public:
	explicit GLTFDocumentExtensionTexture(Format p_format = FORMAT_KTX) : format(p_format) {}

	Format get_format() const { return format; }

	// Import process.
	Error import_preflight(Ref<GLTFState> p_state, const Vector<String>& p_extensions) override;
	Vector<String> get_supported_extensions() override;
	Error parse_image_data(Ref<GLTFState> p_state, const PackedByteArray& p_image_data,
		const String& p_mime_type, Ref<Image> r_image) override;
	String get_image_file_extension() override;
	// Export process.
	Vector<String> get_saveable_image_formats() override;
	Error save_image_at_path(Ref<GLTFState> p_state, Ref<Image> p_image, const String& p_file_path,
		const String& p_image_format, float p_lossy_quality) override;
};

#ifndef PHYSICS_3D_DISABLED
#include "scene/3d/node_3d.h"

// GLTFPhysicsBody is an intermediary between Godot's physics body nodes
// and the OMI_physics_body extension.
// https://github.com/omigroup/gltf-extensions/tree/main/extensions/2.0/OMI_physics_body

class GLTFPhysicsBody final : public Resource
{
public:
	enum class PhysicsBodyType
	{
		STATIC,
		ANIMATABLE,
		CHARACTER,
		RIGID,
		VEHICLE,
		TRIGGER,
	};

private:
	PhysicsBodyType body_type = PhysicsBodyType::RIGID;
	real_t mass = 1.0;
	Vector3 linear_velocity;
	Vector3 angular_velocity;
	Vector3 center_of_mass;
	Vector3 inertia_diagonal;
	Quaternion inertia_orientation;

public:
	String get_body_type() const;
	void set_body_type(const String& p_body_type);

	PhysicsBodyType get_physics_body_type() const;
	void set_physics_body_type(PhysicsBodyType p_body_type);

	real_t get_mass() const;
	void set_mass(real_t p_mass);

	Vector3 get_linear_velocity() const;
	void set_linear_velocity(const Vector3& p_linear_velocity);

	Vector3 get_angular_velocity() const;
	void set_angular_velocity(const Vector3& p_angular_velocity);

	Vector3 get_center_of_mass() const;
	void set_center_of_mass(const Vector3& p_center_of_mass);

	Vector3 get_inertia_diagonal() const;
	void set_inertia_diagonal(const Vector3& p_inertia_diagonal);

	Quaternion get_inertia_orientation() const;
	void set_inertia_orientation(const Quaternion& p_inertia_orientation);

#ifndef DISABLE_DEPRECATED
	Basis get_inertia_tensor() const;
	void set_inertia_tensor(const Basis& p_inertia_tensor);
#endif // DISABLE_DEPRECATED

	static Ref<GLTFPhysicsBody> from_node(const Node3D* p_body_node);
	Node3D* to_node() const;
};

// GLTFPhysicsShape is an intermediary between Godot's collision shape nodes
// and the OMI_physics_shape extension.
// https://github.com/omigroup/gltf-extensions/tree/main/extensions/2.0/OMI_physics_shape

class GLTFPhysicsShape final : public Resource
{
private:
	String shape_type;
	Vector3 size = Vector3(1.0, 1.0, 1.0);
	real_t radius = 0.5;
	real_t height = 2.0;
	bool is_trigger = false;
	GLTFMeshIndex mesh_index = -1;
	Ref<ImporterMesh> importer_mesh = nullptr;
	// Internal only, for caching Godot shape resources. Used in `to_resource` and `to_node`.
	Ref<Shape3D> _shape_cache = nullptr;

public:
	String get_shape_type() const;
	void set_shape_type(const String& p_shape_type);

	Vector3 get_size() const;
	void set_size(const Vector3& p_size);

	real_t get_radius() const;
	void set_radius(real_t p_radius);

	real_t get_height() const;
	void set_height(real_t p_height);

	bool get_is_trigger() const;
	void set_is_trigger(bool p_is_trigger);

	GLTFMeshIndex get_mesh_index() const;
	void set_mesh_index(GLTFMeshIndex p_mesh_index);

	Ref<ImporterMesh> get_importer_mesh() const;
	void set_importer_mesh(const Ref<ImporterMesh>& p_importer_mesh);

	static Ref<GLTFPhysicsShape> from_node(const CollisionShape3D* p_shape_node);
	CollisionShape3D* to_node(bool p_cache_shapes = false);

	static Ref<GLTFPhysicsShape> from_resource(const Ref<Shape3D>& p_shape_resource);
	Ref<Shape3D> to_resource(bool p_cache_shapes = false);
};

class GLTFDocumentExtensionPhysics final : public GLTFDocumentExtension
{
public:
	// Import process.
	Error import_preflight(Ref<GLTFState> p_state, const Vector<String>& p_extensions) override;
	Vector<String> get_supported_extensions() override;
	Ref<GLTFObjectModelProperty> import_object_model_property(Ref<GLTFState> p_state,
		const PackedStringArray& p_split_json_pointer,
		const TypedArray<NodePath>& p_partial_paths) override;
	Node3D* generate_scene_node(
		Ref<GLTFState> p_state, Ref<GLTFNode> p_gltf_node, Node* p_scene_parent) override;
	// Export process.
	void convert_scene_node(
		Ref<GLTFState> p_state, Ref<GLTFNode> p_gltf_node, Node* p_scene_node) override;
	Error export_preserialize(Ref<GLTFState> p_state) override;
};
#endif // PHYSICS_3D_DISABLED

#include "scene/3d/node_3d.h"
#include "scene/main/scene_tree.h"

class GLTFState final : public Resource
{
	friend class GLTFDocument;
	friend class GLTFNode;
	friend class FBXDocument;
	friend class FBXState;

public:
	enum HandleBinaryImageMode
	{
		HANDLE_BINARY_IMAGE_MODE_DISCARD_TEXTURES = 0,
		HANDLE_BINARY_IMAGE_MODE_EXTRACT_TEXTURES,
		HANDLE_BINARY_IMAGE_MODE_EMBED_AS_BASISU,
		HANDLE_BINARY_IMAGE_MODE_EMBED_AS_UNCOMPRESSED, // If this value changes from 3,
														// ResourceImporterScene::pre_import must be
														// changed as well.
	};

protected:
	String base_path;
	String extract_path;
	String extract_prefix;
	String filename;
	int major_version = 0;
	int minor_version = 0;
	String copyright;
	Vector<uint8_t> glb_data;
	double bake_fps = 30.0;

	bool use_named_skin_binds = false;
	bool use_khr_texture_transform = false;
	bool discard_meshes_and_materials = false;
	bool force_generate_tangents = false;
	bool create_animations = true;
	bool force_disable_compression = false;
	bool import_as_skeleton_bones = false;

	HandleBinaryImageMode handle_binary_image_mode = HANDLE_BINARY_IMAGE_MODE_EXTRACT_TEXTURES;

	Vector<Ref<GLTFNode>> nodes;
	Vector<Vector<uint8_t>> buffers;
	Vector<Ref<GLTFBufferView>> buffer_views;
	Vector<Ref<GLTFAccessor>> accessors;

	Vector<Ref<GLTFMesh>> meshes; // Meshes are loaded directly, no reason not to.

	Vector<AnimationPlayer*> animation_players;
	HashMap<Ref<Material>, GLTFMaterialIndex> material_cache;
	Vector<Ref<Material>> materials;

	String scene_name;
	Vector<int> root_nodes;
	Vector<Ref<GLTFTexture>> textures;
	Vector<Ref<GLTFTextureSampler>> texture_samplers;
	Ref<GLTFTextureSampler> default_texture_sampler;
	Vector<Ref<Texture2D>> images;
	Vector<String> extensions_used;
	Vector<String> extensions_required;
	Vector<Ref<Image>> source_images;

	Vector<Ref<GLTFSkin>> skins;
	Vector<Ref<GLTFCamera>> cameras;
	Vector<Ref<GLTFLight>> lights;
	HashSet<String> unique_names;
	HashSet<String> unique_animation_names;

	Vector<Ref<GLTFSkeleton>> skeletons;
	Vector<Ref<GLTFAnimation>> animations;
	HashMap<GLTFNodeIndex, Node*> scene_nodes;
	HashMap<GLTFNodeIndex, ImporterMeshInstance3D*> scene_mesh_instances;
	HashMap<String, Ref<GLTFObjectModelProperty>> object_model_properties;

public:
	double get_bake_fps() const { return bake_fps; }

	void set_bake_fps(double value) { bake_fps = value; }

	void add_used_extension(const String& p_extension, bool p_required = false);
	GLTFBufferViewIndex append_data_to_buffers(
		const Vector<uint8_t>& p_data, const bool p_deduplication);
	GLTFNodeIndex append_gltf_node(
		Ref<GLTFNode> p_gltf_node, Node* p_godot_scene_node, GLTFNodeIndex p_parent_node_index);

	// Deprecated, use HandleBinaryImageMode instead.
	enum GLTFHandleBinary
	{
		HANDLE_BINARY_DISCARD_TEXTURES = 0,
		HANDLE_BINARY_EXTRACT_TEXTURES,
		HANDLE_BINARY_EMBED_AS_BASISU,
		HANDLE_BINARY_EMBED_AS_UNCOMPRESSED, // If this value changes from 3,
											 // ResourceImporterScene::pre_import must be changed as
											 // well.
	};

	int32_t get_handle_binary_image() const { return handle_binary_image_mode; }

	void set_handle_binary_image(int32_t p_handle_binary_image)
	{
		handle_binary_image_mode = (HandleBinaryImageMode)p_handle_binary_image;
	}

	HandleBinaryImageMode get_handle_binary_image_mode() const { return handle_binary_image_mode; }

	void set_handle_binary_image_mode(HandleBinaryImageMode p_handle_binary_image)
	{
		handle_binary_image_mode = p_handle_binary_image;
	}

	int get_major_version() const;
	void set_major_version(int p_major_version);

	int get_minor_version() const;
	void set_minor_version(int p_minor_version);

	String get_copyright() const;
	void set_copyright(const String& p_copyright);

	Vector<uint8_t> get_glb_data() const;
	void set_glb_data(const Vector<uint8_t>& p_glb_data);

	bool get_use_named_skin_binds() const;
	void set_use_named_skin_binds(bool p_use_named_skin_binds);

	bool get_discard_meshes_and_materials() const;
	void set_discard_meshes_and_materials(bool p_discard_meshes_and_materials);

	const Vector<Ref<GLTFNode>>& get_nodes() const { return nodes; }

	void set_nodes(const Vector<Ref<GLTFNode>>& p_nodes) { nodes = p_nodes; }

	const Vector<PackedByteArray>& get_buffers() const { return buffers; }

	void set_buffers(const Vector<PackedByteArray>& p_buffers) { buffers = p_buffers; }

	const Vector<Ref<GLTFBufferView>>& get_buffer_views() const { return buffer_views; }

	void set_buffer_views(const Vector<Ref<GLTFBufferView>>& p_buffer_views)
	{
		buffer_views = p_buffer_views;
	}

	const Vector<Ref<GLTFAccessor>>& get_accessors() const { return accessors; }

	void set_accessors(const Vector<Ref<GLTFAccessor>>& p_accessors) { accessors = p_accessors; }

	const Vector<Ref<GLTFMesh>>& get_meshes() const { return meshes; }

	void set_meshes(const Vector<Ref<GLTFMesh>>& p_meshes) { meshes = p_meshes; }

	const Vector<Ref<Material>>& get_materials() const { return materials; }

	void set_materials(const Vector<Ref<Material>>& p_materials) { materials = p_materials; }

	String get_scene_name() const;
	void set_scene_name(const String& p_scene_name);

	String get_base_path() const;
	void set_base_path(const String& p_base_path);

	String get_extract_path() const;
	void set_extract_path(const String& p_extract_path);

	String get_extract_prefix() const;
	void set_extract_prefix(const String& p_extract_prefix);

	String get_filename() const;
	void set_filename(const String& p_filename);

	PackedInt32Array get_root_nodes() const;
	void set_root_nodes(const PackedInt32Array& p_root_nodes);

	const Vector<Ref<GLTFTexture>>& get_textures() const { return textures; }

	void set_textures(const Vector<Ref<GLTFTexture>>& p_textures) { textures = p_textures; }

	const Vector<Ref<GLTFTextureSampler>>& get_texture_samplers() const { return texture_samplers; }

	void set_texture_samplers(const Vector<Ref<GLTFTextureSampler>>& p_texture_samplers)
	{
		texture_samplers = p_texture_samplers;
	}

	const Vector<Ref<Texture2D>>& get_images() const { return images; }

	void set_images(const Vector<Ref<Texture2D>>& p_images) { images = p_images; }

	const Vector<Ref<GLTFSkin>>& get_skins() const { return skins; }

	void set_skins(const Vector<Ref<GLTFSkin>>& p_skins) { skins = p_skins; }

	const Vector<Ref<GLTFCamera>>& get_cameras() const { return cameras; }

	void set_cameras(const Vector<Ref<GLTFCamera>>& p_cameras) { cameras = p_cameras; }

	const Vector<Ref<GLTFLight>>& get_lights() const { return lights; }

	void set_lights(const Vector<Ref<GLTFLight>>& p_lights) { lights = p_lights; }

	const HashSet<String>& get_unique_names() const { return unique_names; }

	void set_unique_names(const HashSet<String>& p_unique_names) { unique_names = p_unique_names; }

	const HashSet<String>& get_unique_animation_names() const { return unique_animation_names; }

	void set_unique_animation_names(const HashSet<String>& p_unique_animation_names)
	{
		unique_animation_names = p_unique_animation_names;
	}

	const Vector<Ref<GLTFSkeleton>>& get_skeletons() const { return skeletons; }

	void set_skeletons(const Vector<Ref<GLTFSkeleton>>& p_skeletons) { skeletons = p_skeletons; }

	bool get_create_animations() const;
	void set_create_animations(bool p_create_animations);

	bool get_import_as_skeleton_bones() const;
	void set_import_as_skeleton_bones(bool p_import_as_skeleton_bones);

	const Vector<Ref<GLTFAnimation>>& get_animations() const { return animations; }

	void set_animations(const Vector<Ref<GLTFAnimation>>& p_animations)
	{
		animations = p_animations;
	}

	Node* get_scene_node(GLTFNodeIndex p_gltf_node_index) const;
	GLTFNodeIndex get_node_index(Node* p_node) const;

	int get_animation_players_count(int p_anim_player_index) const;

	AnimationPlayer* get_animation_player(int p_anim_player_index) const;
};


#include "scene/3d/node_3d.h"

class CSGShape3D;
class GridMap;

class GLTFDocument final : public Resource
{
	friend class FBXDocument;

public:
	const int32_t JOINT_GROUP_SIZE = 4;

	enum
	{
		TEXTURE_TYPE_GENERIC = 0,
		TEXTURE_TYPE_NORMAL = 1,
	};

	enum RootNodeMode
	{
		ROOT_NODE_MODE_SINGLE_ROOT,
		ROOT_NODE_MODE_KEEP_ROOT,
		ROOT_NODE_MODE_MULTI_ROOT,
	};

	enum TextureMapMode
	{
		TEXTURE_MAP_MODE_DO_NOT_REMAP,
		TEXTURE_MAP_MODE_REMAP_TO_STANDARD_MATERIAL,
	};

	enum VisibilityMode
	{
		VISIBILITY_MODE_INCLUDE_REQUIRED,
		VISIBILITY_MODE_INCLUDE_OPTIONAL,
		VISIBILITY_MODE_EXCLUDE,
	};

	enum ImportFlags
	{
		IMPORT_FLAG_GENERATE_TANGENT_ARRAYS = 8,
		IMPORT_FLAG_USE_NAMED_SKIN_BINDS = 16,
		IMPORT_FLAG_DISCARD_MESHES_AND_MATERIALS = 32,
		IMPORT_FLAG_FORCE_DISABLE_MESH_COMPRESSION = 64,
	};

private:
	int _naming_version = 2;
	String _image_format = "PNG";
	float _lossy_quality = 0.75f;
	String _fallback_image_format = "None";
	float _fallback_image_quality = 0.25f;
	Ref<GLTFDocumentExtension> _image_save_extension;
	RootNodeMode _root_node_mode = RootNodeMode::ROOT_NODE_MODE_SINGLE_ROOT;
	TextureMapMode _texture_map_mode = TextureMapMode::TEXTURE_MAP_MODE_REMAP_TO_STANDARD_MATERIAL;
	VisibilityMode _visibility_mode = VisibilityMode::VISIBILITY_MODE_INCLUDE_REQUIRED;

protected:
	String _gen_unique_name(Ref<GLTFState> p_state, const String& p_name);
	static Vector<Ref<GLTFDocumentExtension>> all_document_extensions;
	static Mutex all_document_extensions_mutex;
	Vector<Ref<GLTFDocumentExtension>> document_extensions;

public:
	static void register_gltf_document_extension(
		Ref<GLTFDocumentExtension> p_extension, bool p_first_priority = false);
	static void unregister_gltf_document_extension(Ref<GLTFDocumentExtension> p_extension);
	static void unregister_all_gltf_document_extensions();
	static Vector<Ref<GLTFDocumentExtension>> get_all_gltf_document_extensions();
	static Vector<String> get_supported_gltf_extensions();
	static HashSet<String> get_supported_gltf_extensions_hashset();

	static NodePath _find_material_node_path(
		Ref<GLTFState> p_state, const Ref<Material>& p_material);
	static Ref<GLTFObjectModelProperty> import_object_model_property(
		Ref<GLTFState> p_state, const String& p_json_pointer);
	static Ref<GLTFObjectModelProperty> export_object_model_property(Ref<GLTFState> p_state,
		const NodePath& p_node_path, const Node* p_godot_node, GLTFNodeIndex p_gltf_node_index);

	void set_naming_version(int p_version);
	int get_naming_version() const;
	void set_image_format(const String& p_image_format);
	String get_image_format() const;
	void set_lossy_quality(float p_lossy_quality);
	float get_lossy_quality() const;
	void set_fallback_image_format(const String& p_fallback_image_format);
	String get_fallback_image_format() const;
	void set_fallback_image_quality(float p_fallback_image_quality);
	float get_fallback_image_quality() const;
	void set_root_node_mode(RootNodeMode p_root_node_mode);
	RootNodeMode get_root_node_mode() const;
	void set_texture_map_mode(TextureMapMode p_texture_map_mode);

	TextureMapMode get_texture_map_mode() const { return _texture_map_mode; }

	void set_visibility_mode(VisibilityMode p_visibility_mode);
	VisibilityMode get_visibility_mode() const;
	static String _gen_unique_name_static(HashSet<String>& r_unique_names, const String& p_name);

private:
	static void _append_khr_texture_transform_ext_json_pointer(
		PackedStringArray& p_split_json_pointer, const String& p_texture_name,
		const bool p_is_offset);
	void _build_parent_hierarchy(Ref<GLTFState> p_state);
	Error _parse_scenes(Ref<GLTFState> p_state);
	Error _parse_nodes(Ref<GLTFState> p_state);
	String _sanitize_animation_name(const String& p_name);
	String _gen_unique_animation_name(Ref<GLTFState> p_state, const String& p_name);
	String _sanitize_bone_name(const String& p_name);
	String _gen_unique_bone_name(
		Ref<GLTFState> p_state, const GLTFSkeletonIndex p_skel_i, const String& p_name);
	GLTFTextureIndex _set_texture(Ref<GLTFState> p_state, Ref<Texture2D> p_texture,
		StandardMaterial3D::TextureFilter p_filter_mode, bool p_repeats);
	GLTFTextureSamplerIndex _set_sampler_for_mode(
		Ref<GLTFState> p_state, StandardMaterial3D::TextureFilter p_filter_mode, bool p_repeats);
	Ref<GLTFTextureSampler> _get_sampler_for_texture(
		Ref<GLTFState> p_state, const GLTFTextureIndex p_texture);
	Error _parse_json(const String& p_path, Ref<GLTFState> p_state);
	Error _parse_glb(Ref<FileAccess> p_file, Ref<GLTFState> p_state);
	void _compute_node_heights(Ref<GLTFState> p_state);
	Error _parse_buffers(Ref<GLTFState> p_state, const String& p_base_path);
	Error _parse_buffer_views(Ref<GLTFState> p_state);
	Error _parse_accessors(Ref<GLTFState> p_state);
	template <typename T>
	static T _decode_unpack_indexed_data(const T& p_source, const PackedInt32Array& p_indices);
	PackedFloat32Array _decode_accessor_as_float32s(const Ref<GLTFState> p_gltf_state,
		GLTFAccessorIndex p_accessor_index,
		const PackedInt32Array& p_packed_vertex_ids = PackedInt32Array());
	PackedFloat64Array _decode_accessor_as_float64s(const Ref<GLTFState> p_gltf_state,
		GLTFAccessorIndex p_accessor_index,
		const PackedInt32Array& p_packed_vertex_ids = PackedInt32Array());
	PackedInt32Array _decode_accessor_as_int32s(const Ref<GLTFState> p_gltf_state,
		GLTFAccessorIndex p_accessor_index,
		const PackedInt32Array& p_packed_vertex_ids = PackedInt32Array());
	PackedVector2Array _decode_accessor_as_vec2(const Ref<GLTFState> p_gltf_state,
		GLTFAccessorIndex p_accessor_index,
		const PackedInt32Array& p_packed_vertex_ids = PackedInt32Array());
	PackedVector3Array _decode_accessor_as_vec3(const Ref<GLTFState> p_gltf_state,
		GLTFAccessorIndex p_accessor_index,
		const PackedInt32Array& p_packed_vertex_ids = PackedInt32Array());
	PackedVector4Array _decode_accessor_as_vec4(const Ref<GLTFState> p_gltf_state,
		GLTFAccessorIndex p_accessor_index,
		const PackedInt32Array& p_packed_vertex_ids = PackedInt32Array());
	PackedColorArray _decode_accessor_as_color(const Ref<GLTFState> p_gltf_state,
		GLTFAccessorIndex p_accessor_index,
		const PackedInt32Array& p_packed_vertex_ids = PackedInt32Array());
	Vector<Quaternion> _decode_accessor_as_quaternion(
		const Ref<GLTFState> p_gltf_state, GLTFAccessorIndex p_accessor_index);
	Error _parse_meshes(Ref<GLTFState> p_state);
	Error _serialize_textures(Ref<GLTFState> p_state);
	Error _serialize_texture_samplers(Ref<GLTFState> p_state);
	Error _serialize_images(Ref<GLTFState> p_state);
	Error _serialize_lights(Ref<GLTFState> p_state);
	Ref<Image> _parse_image_bytes_into_image(Ref<GLTFState> p_state, const Vector<uint8_t>& p_bytes,
		const String& p_mime_type, int p_index, String& r_file_extension);
	void _parse_image_save_image(Ref<GLTFState> p_state, const Vector<uint8_t>& p_bytes,
		const String& p_resource_uri, const String& p_file_extension, int p_index,
		Ref<Image> p_image);
	Error _parse_images(Ref<GLTFState> p_state, const String& p_base_path);
	Error _parse_textures(Ref<GLTFState> p_state);
	Error _parse_texture_samplers(Ref<GLTFState> p_state);
	Error _parse_materials(Ref<GLTFState> p_state);
	void spec_gloss_to_rough_metal(Ref<GLTFSpecGloss> r_spec_gloss, Ref<StandardMaterial3D> p_material);
	static void spec_gloss_to_metal_base_color(const Color& p_specular_factor,
		const Color& p_diffuse, Color& r_base_color, float& r_metallic);
	Error _parse_skins(Ref<GLTFState> p_state);
	Error _serialize_skins(Ref<GLTFState> p_state);
	Error _create_skins(Ref<GLTFState> p_state);
	bool _skins_are_same(const Ref<Skin>& p_skin_a, const Ref<Skin>& p_skin_b);
	void _remove_duplicate_skins(Ref<GLTFState> p_state);
	Error _serialize_cameras(Ref<GLTFState> p_state);
	Error _parse_cameras(Ref<GLTFState> p_state);
	Error _parse_lights(Ref<GLTFState> p_state);
	Error _parse_animations(Ref<GLTFState> p_state);
	void _parse_animation_pointer(Ref<GLTFState> p_state, const String& p_animation_json_pointer,
		const Ref<GLTFAnimation> p_gltf_animation, const GLTFAnimation::Interpolation p_interp,
		const Vector<double>& p_times, const int p_output_value_accessor_index);
	Error _serialize_animations(Ref<GLTFState> p_state);
	bool _does_skinned_mesh_require_placeholder_node(
		Ref<GLTFState> p_state, Ref<GLTFNode> p_gltf_node);
	BoneAttachment3D* _generate_bone_attachment(
		Skeleton3D* p_godot_skeleton, const Ref<GLTFNode>& p_bone_node);
	BoneAttachment3D* _generate_bone_attachment_compat_4pt4(Ref<GLTFState> p_state,
		Skeleton3D* p_skeleton, const GLTFNodeIndex p_node_index, const GLTFNodeIndex p_bone_index);
	ImporterMeshInstance3D* _generate_mesh_instance(
		Ref<GLTFState> p_state, const GLTFNodeIndex p_node_index);
	Node3D* _generate_spatial(Ref<GLTFState> p_state, const GLTFNodeIndex p_node_index);
	void _assign_node_names(Ref<GLTFState> p_state);
	template <typename T>
	T _interpolate_track(const Vector<double>& p_times, const Vector<T>& p_values,
		const float p_time, const GLTFAnimation::Interpolation p_interp);

	Error _encode_accessors(Ref<GLTFState> p_state);
	Error _encode_buffer_views(Ref<GLTFState> p_state);
	Error _serialize_materials(Ref<GLTFState> p_state);
	Error _serialize_meshes(Ref<GLTFState> p_state);
	Error _serialize_nodes(Ref<GLTFState> p_state);
	Error _serialize_scenes(Ref<GLTFState> p_state);
	String interpolation_to_string(const GLTFAnimation::Interpolation p_interp);
	Error _encode_buffer_bins(Ref<GLTFState> p_state, const String& p_path);
	Error _encode_buffer_glb(Ref<GLTFState> p_state, const String& p_path);
	PackedByteArray _serialize_glb_buffer(Ref<GLTFState> p_state, Error* r_err);
	Error _serialize_asset_header(Ref<GLTFState> p_state);
	Error _serialize_file(Ref<GLTFState> p_state, const String p_path);
	Error _serialize_gltf_extensions(Ref<GLTFState> p_state) const;

public:
	// https://www.itu.int/rec/R-REC-BT.601
	// https://www.itu.int/dms_pubrec/itu-r/rec/bt/R-REC-BT.601-7-201103-I!!PDF-E.pdf
	static constexpr float R_BRIGHTNESS_COEFF = 0.299f;
	static constexpr float G_BRIGHTNESS_COEFF = 0.587f;
	static constexpr float B_BRIGHTNESS_COEFF = 0.114f;

private:
	// https://github.com/microsoft/glTF-SDK/blob/master/GLTFSDK/Source/PBRUtils.cpp#L9
	// https://bghgary.github.io/glTF/convert-between-workflows-bjs/js/babylon.pbrUtilities.js
	static float solve_metallic(float p_dielectric_specular, float p_diffuse, float p_specular,
		float p_one_minus_specular_strength);
	static float get_perceived_brightness(const Color p_color);
	static float get_max_component(const Color& p_color);

public:
	Error append_from_file(const String& p_path, Ref<GLTFState> p_state,
		uint32_t p_flags = 0, const String& p_base_path = String());
	Error append_from_buffer(const PackedByteArray& p_bytes, const String& p_base_path,
		Ref<GLTFState> p_state, uint32_t p_flags = 0);
	Error append_from_scene(Node* p_node, Ref<GLTFState> p_state, uint32_t p_flags = 0);

	Node* generate_scene(Ref<GLTFState> p_state, float p_bake_fps = 30.0f,
		bool p_trimming = false, bool p_remove_immutable_tracks = true);
	PackedByteArray generate_buffer(Ref<GLTFState> p_state);
	Error write_to_filesystem(Ref<GLTFState> p_state, const String& p_path);

public:
	Error _parse_gltf_state(Ref<GLTFState> p_state, const String& p_search_path);
	Error _parse_asset_header(Ref<GLTFState> p_state);
	Error _parse_gltf_extensions(Ref<GLTFState> p_state);
	void _process_mesh_instances(Ref<GLTFState> p_state, Node* p_scene_root);
	Node* _generate_scene_node_tree(Ref<GLTFState> p_state);
	void _generate_scene_node(Ref<GLTFState> p_state, const GLTFNodeIndex p_node_index,
		Node* p_scene_parent, Node* p_scene_root);
	void _generate_skeleton_bone_node(Ref<GLTFState> p_state, const GLTFNodeIndex p_node_index,
		Node3D* p_current_node, Node* p_scene_parent, Node* p_scene_root);
	void _attach_node_to_skeleton(Ref<GLTFState> p_state, const GLTFNodeIndex p_node_index,
		Node3D* p_current_node, Skeleton3D* p_godot_skeleton, Node* p_scene_root,
		GLTFNodeIndex p_bone_node_index = -1);
	void _generate_scene_node_compat_4pt4(Ref<GLTFState> p_state, const GLTFNodeIndex p_node_index,
		Node* p_scene_parent, Node* p_scene_root);
	void _generate_skeleton_bone_node_compat_4pt4(Ref<GLTFState> p_state,
		const GLTFNodeIndex p_node_index, Node* p_scene_parent, Node* p_scene_root);
	void _import_animation(Ref<GLTFState> p_state, AnimationPlayer* p_animation_player,
		const GLTFAnimationIndex p_index, const bool p_trimming,
		const bool p_remove_immutable_tracks);
	void _convert_mesh_instances(Ref<GLTFState> p_state);
	GLTFCameraIndex _convert_camera(Ref<GLTFState> p_state, Camera3D* p_camera);
	void _convert_light_to_gltf(
		Light3D* p_light, Ref<GLTFState> p_state, Ref<GLTFNode> p_gltf_node);
	GLTFLightIndex _convert_light(Ref<GLTFState> p_state, Light3D* p_light);
	void _convert_spatial(Ref<GLTFState> p_state, Node3D* p_spatial, Ref<GLTFNode> p_node);
	void _convert_scene_node(Ref<GLTFState> p_state, Node* p_current,
		const GLTFNodeIndex p_gltf_current, const GLTFNodeIndex p_gltf_root);

	void _convert_csg_shape_to_gltf(CSGShape3D* p_current, GLTFNodeIndex p_gltf_parent,
		Ref<GLTFNode> p_gltf_node, Ref<GLTFState> p_state);

	void _check_visibility(Node* p_node, bool& r_retflag);
	void _convert_camera_to_gltf(
		Camera3D* p_camera, Ref<GLTFState> p_state, Ref<GLTFNode> p_gltf_node);
	void _convert_grid_map_to_gltf(GridMap* p_grid_map, GLTFNodeIndex p_parent_node_index,
		GLTFNodeIndex p_root_node_index, Ref<GLTFNode> p_gltf_node, Ref<GLTFState> p_state);
	void _convert_multi_mesh_instance_to_gltf(MultiMeshInstance3D* p_multi_mesh_instance,
		GLTFNodeIndex p_parent_node_index, GLTFNodeIndex p_root_node_index,
		Ref<GLTFNode> p_gltf_node, Ref<GLTFState> p_state);
	void _convert_skeleton_to_gltf(Skeleton3D* p_scene_parent, Ref<GLTFState> p_state,
		GLTFNodeIndex p_parent_node_index, GLTFNodeIndex p_root_node_index,
		Ref<GLTFNode> p_gltf_node);
	void _convert_bone_attachment_to_gltf(BoneAttachment3D* p_bone_attachment,
		Ref<GLTFState> p_state, GLTFNodeIndex p_parent_node_index, GLTFNodeIndex p_root_node_index,
		Ref<GLTFNode> p_gltf_node);
	void _convert_mesh_instance_to_gltf(
		MeshInstance3D* p_mesh_instance, Ref<GLTFState> p_state, Ref<GLTFNode> p_gltf_node);
	GLTFMeshIndex _convert_mesh_to_gltf(Ref<GLTFState> p_state, MeshInstance3D* p_mesh_instance);

	GLTFNodeIndex _node_and_or_bone_to_gltf_node_index(
		Ref<GLTFState> p_state, const Vector<String>& p_node_subpath, const Node* p_godot_node);
	bool _convert_animation_node_track(Ref<GLTFState> p_state,
		GLTFAnimation::NodeTrack& p_gltf_node_track, const Ref<Animation>& p_godot_animation,
		int32_t p_godot_anim_track_index, Vector<double>& p_times);
	void _convert_animation(Ref<GLTFState> p_state, AnimationPlayer* p_animation_player,
		const String& p_animation_track_name);

	Error _serialize(Ref<GLTFState> p_state);
	Error _parse(Ref<GLTFState> p_state, const String& p_path, Ref<FileAccess> p_file);
};

#include "core/templates/rb_set.h"

using SkinNodeIndex = int;
using SkinSkeletonIndex = int;

class SkinTool final {
public:
	static String _sanitize_bone_name(const String &p_name);
	static String _gen_unique_bone_name(HashSet<String> &r_unique_names, const String &p_name);
	static SkinNodeIndex _find_highest_node(Vector<Ref<GLTFNode>> &r_nodes, const Vector<SkinNodeIndex> &p_subset);
	static bool _capture_nodes_in_skin(const Vector<Ref<GLTFNode>> &p_nodes, Ref<GLTFSkin> p_skin, const SkinNodeIndex p_node_index);
	static void _capture_nodes_for_multirooted_skin(Vector<Ref<GLTFNode>> &r_nodes, Ref<GLTFSkin> p_skin);
	static void _recurse_children(
			Vector<Ref<GLTFNode>> &r_nodes,
			const SkinNodeIndex p_node_index,
			RBSet<SkinNodeIndex> &r_all_skin_nodes,
			HashSet<SkinNodeIndex> &r_child_visited_set);
	static Error _reparent_non_joint_skeleton_subtrees(
			Vector<Ref<GLTFNode>> &r_nodes,
			Ref<GLTFSkeleton> p_skeleton,
			const Vector<SkinNodeIndex> &p_non_joints);
	static Error _determine_skeleton_roots(
			Vector<Ref<GLTFNode>> &r_nodes,
			Vector<Ref<GLTFSkeleton>> &r_skeletons,
			const SkinSkeletonIndex p_skel_i);
	static Error _map_skin_joints_indices_to_skeleton_bone_indices(
			Vector<Ref<GLTFSkin>> &r_skins,
			Vector<Ref<GLTFSkeleton>> &r_skeletons,
			Vector<Ref<GLTFNode>> &r_nodes);
	static String _gen_unique_name(HashSet<String> &unique_names, const String &p_name);
	static bool _skins_are_same(const Ref<Skin> p_skin_a, const Ref<Skin> p_skin_b);
	static void _remove_duplicate_skins(Vector<Ref<GLTFSkin>> &r_skins);
	static void _check_if_parent_needs_to_become_joint(
			const Vector<Ref<GLTFNode>> &p_all_nodes,
			const Vector<GLTFNodeIndex> &p_skeleton_node_indices,
			const Ref<GLTFNode> &p_gltf_node,
			Vector<GLTFNodeIndex> &r_non_joint_indices);

public:
	static Error _expand_skin(Vector<Ref<GLTFNode>> &r_nodes, Ref<GLTFSkin> p_skin);
	static Error _verify_skin(Vector<Ref<GLTFNode>> &r_nodes, Ref<GLTFSkin> p_skin);
	static Error _asset_parse_skins(
			const Vector<SkinNodeIndex> &p_input_skin_indices,
			const Vector<Ref<GLTFSkin>> &p_input_skins,
			const Vector<Ref<GLTFNode>> &p_input_nodes,
			Vector<SkinNodeIndex> &r_output_skin_indices,
			Vector<Ref<GLTFSkin>> &r_output_skins,
			HashMap<GLTFNodeIndex, bool> &r_joint_mapping);
	static Error _determine_skeletons(
			Vector<Ref<GLTFSkin>> &r_skins,
			Vector<Ref<GLTFNode>> &r_nodes,
			Vector<Ref<GLTFSkeleton>> &r_skeletons,
			const Vector<GLTFNodeIndex> &p_single_skeleton_root,
			bool p_turn_non_joint_descendants_into_bones);
	static Error _create_skins(Vector<Ref<GLTFSkin>> &skins, Vector<Ref<GLTFNode>> &nodes, bool use_named_skin_binds, HashSet<String> &unique_names);
};

#ifdef TOOLS_ENABLED
// --- Folded from editor/import/3d/resource_importer_scene.h ---
#include "core/error/error_macros.h"
#include "core/io/resource_loader.h"
#include "scene/3d/node_3d.h"
#include "scene/resources/mesh.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/mesh.h"

class AnimationPlayer;
class ImporterMesh;
class Material;

class EditorSceneFormatImporter : public RefCounted
{
	List<ResourceImporter::ImportOption>* current_option_list = nullptr;

public:
	enum ImportFlags
	{
		IMPORT_SCENE = 1,
		IMPORT_ANIMATION = 2,
		IMPORT_FAIL_ON_MISSING_DEPENDENCIES = 4,
		IMPORT_GENERATE_TANGENT_ARRAYS = 8,
		IMPORT_USE_NAMED_SKIN_BINDS = 16,
		IMPORT_DISCARD_MESHES_AND_MATERIALS = 32, // used for optimizing animation import
		IMPORT_FORCE_DISABLE_MESH_COMPRESSION = 64,
	};

	virtual void get_extensions(List<String>* r_extensions) const;
	virtual void get_import_options(
		const String& p_path, List<ResourceImporter::ImportOption>* r_options);
};

class EditorScenePostImport : public RefCounted
{
	String source_file;

protected:

public:
	String get_source_file() const;
	virtual Node* post_import(Node* p_scene);
	virtual void init(const String& p_source_file);
};

class EditorScenePostImportPlugin : public RefCounted
{
public:
	enum InternalImportCategory
	{
		INTERNAL_IMPORT_CATEGORY_NODE,
		INTERNAL_IMPORT_CATEGORY_MESH_3D_NODE,
		INTERNAL_IMPORT_CATEGORY_MESH,
		INTERNAL_IMPORT_CATEGORY_MATERIAL,
		INTERNAL_IMPORT_CATEGORY_ANIMATION,
		INTERNAL_IMPORT_CATEGORY_ANIMATION_NODE,
		INTERNAL_IMPORT_CATEGORY_SKELETON_3D_NODE,
		INTERNAL_IMPORT_CATEGORY_MAX
	};

private:
	List<ResourceImporter::ImportOption>* current_option_list = nullptr;

protected:

public:
	virtual void get_internal_import_options(
		InternalImportCategory p_category, List<ResourceImporter::ImportOption>* r_options);

	virtual void get_import_options(
		const String& p_path, List<ResourceImporter::ImportOption>* r_options);
};

class ResourceImporterScene : public ResourceImporter
{
	static Vector<Ref<EditorSceneFormatImporter>> scene_importers;
	static Vector<Ref<EditorScenePostImportPlugin>> post_importer_plugins;

	enum LightBakeMode
	{
		LIGHT_BAKE_DISABLED,
		LIGHT_BAKE_STATIC,
		LIGHT_BAKE_STATIC_LIGHTMAPS,
		LIGHT_BAKE_DYNAMIC,
	};

	enum MeshPhysicsMode
	{
		MESH_PHYSICS_DISABLED,
		MESH_PHYSICS_MESH_AND_STATIC_COLLIDER,
		MESH_PHYSICS_RIGID_BODY_AND_MESH,
		MESH_PHYSICS_STATIC_COLLIDER_ONLY,
		MESH_PHYSICS_AREA_ONLY,
	};

	enum NavMeshMode
	{
		NAVMESH_DISABLED,
		NAVMESH_MESH_AND_NAVMESH,
		NAVMESH_NAVMESH_ONLY,
	};

	enum OccluderMode
	{
		OCCLUDER_DISABLED,
		OCCLUDER_MESH_AND_OCCLUDER,
		OCCLUDER_OCCLUDER_ONLY,
	};

	enum MeshOverride
	{
		MESH_OVERRIDE_DEFAULT,
		MESH_OVERRIDE_ENABLE,
		MESH_OVERRIDE_DISABLE,
	};

	enum BodyType
	{
		BODY_TYPE_STATIC,
		BODY_TYPE_DYNAMIC,
		BODY_TYPE_AREA
	};

	enum ShapeType
	{
		SHAPE_TYPE_DECOMPOSE_CONVEX,
		SHAPE_TYPE_SIMPLE_CONVEX,
		SHAPE_TYPE_TRIMESH,
		SHAPE_TYPE_BOX,
		SHAPE_TYPE_SPHERE,
		SHAPE_TYPE_CYLINDER,
		SHAPE_TYPE_CAPSULE,
		SHAPE_TYPE_AUTOMATIC,
	};

	void _replace_owner(Node* p_node, Node* p_scene, Node* p_new_owner);
	void _add_shapes(Node* p_node, const Vector<Ref<Shape3D>>& p_shapes);

	enum AnimationImportTracks
	{
		ANIMATION_IMPORT_TRACKS_IF_PRESENT,
		ANIMATION_IMPORT_TRACKS_IF_PRESENT_FOR_ALL,
		ANIMATION_IMPORT_TRACKS_NEVER,
	};

	enum TrackChannel
	{
		TRACK_CHANNEL_POSITION,
		TRACK_CHANNEL_ROTATION,
		TRACK_CHANNEL_SCALE,
		TRACK_CHANNEL_BLEND_SHAPE,
		TRACK_CHANNEL_MAX
	};

	void _optimize_track_usage(AnimationPlayer* p_player, AnimationImportTracks* p_track_actions);

	String _scene_import_type = "PackedScene";

public:
	static const String material_extension[3];

	static void add_post_importer_plugin(
		const Ref<EditorScenePostImportPlugin>& p_plugin, bool p_first_priority = false);
	static void remove_post_importer_plugin(const Ref<EditorScenePostImportPlugin>& p_plugin);

	const Vector<Ref<EditorSceneFormatImporter>>& get_scene_importers() const
	{
		return scene_importers;
	}

	static void add_scene_importer(
		Ref<EditorSceneFormatImporter> p_importer, bool p_first_priority = false);
	static void remove_scene_importer(Ref<EditorSceneFormatImporter> p_importer);
	static void get_scene_importer_extensions(List<String>* p_extensions);

	static void clean_up_importer_plugins();

	String get_scene_import_type() const { return _scene_import_type; }

	void set_scene_import_type(const String& p_type) { _scene_import_type = p_type; }

	virtual String get_importer_name() const override;
	virtual String get_visible_name() const override;
	virtual void get_recognized_extensions(List<String>* p_extensions) const override;
	virtual String get_save_extension() const override;
	virtual String get_resource_type() const override;
	virtual int get_format_version() const override;

	virtual int get_preset_count() const override;
	virtual String get_preset_name(int p_idx) const override;

	enum InternalImportCategory
	{
		INTERNAL_IMPORT_CATEGORY_NODE = EditorScenePostImportPlugin::INTERNAL_IMPORT_CATEGORY_NODE,
		INTERNAL_IMPORT_CATEGORY_MESH_3D_NODE =
			EditorScenePostImportPlugin::INTERNAL_IMPORT_CATEGORY_MESH_3D_NODE,
		INTERNAL_IMPORT_CATEGORY_MESH = EditorScenePostImportPlugin::INTERNAL_IMPORT_CATEGORY_MESH,
		INTERNAL_IMPORT_CATEGORY_MATERIAL =
			EditorScenePostImportPlugin::INTERNAL_IMPORT_CATEGORY_MATERIAL,
		INTERNAL_IMPORT_CATEGORY_ANIMATION =
			EditorScenePostImportPlugin::INTERNAL_IMPORT_CATEGORY_ANIMATION,
		INTERNAL_IMPORT_CATEGORY_ANIMATION_NODE =
			EditorScenePostImportPlugin::INTERNAL_IMPORT_CATEGORY_ANIMATION_NODE,
		INTERNAL_IMPORT_CATEGORY_SKELETON_3D_NODE =
			EditorScenePostImportPlugin::INTERNAL_IMPORT_CATEGORY_SKELETON_3D_NODE,
		INTERNAL_IMPORT_CATEGORY_MAX = EditorScenePostImportPlugin::INTERNAL_IMPORT_CATEGORY_MAX
	};

	void get_internal_import_options(
		InternalImportCategory p_category, List<ImportOption>* r_options) const;

	virtual void get_import_options(
		const String& p_path, List<ImportOption>* r_options, int p_preset = 0) const override;

	// Import scenes *after* everything else (such as textures).
	virtual int get_import_order() const override { return ResourceImporter::IMPORT_ORDER_SCENE; }

	Ref<Animation> _save_animation_to_file(Ref<Animation> anim, bool p_save_to_file,
		const String& p_save_to_path, bool p_keep_custom_tracks);
	void _optimize_animations(
		AnimationPlayer* anim, float p_max_vel_error, float p_max_ang_error, int p_prc_error);
	void _compress_animations(AnimationPlayer* anim, int p_page_size_kb);

	virtual bool has_advanced_options() const override;
	virtual void show_advanced_options(const String& p_path) override;

	ResourceImporterScene(const String& p_scene_import_type = "PackedScene");

	template <typename M>
	static Vector<Ref<Shape3D>> get_collision_shapes(
		const Ref<ImporterMesh>& p_mesh, const M& p_options, float p_applied_root_scale);

	template <typename M> static Transform3D get_collision_shapes_transform(const M& p_options);
};

class EditorSceneFormatImporterESCN : public EditorSceneFormatImporter
{
public:
	virtual void get_extensions(List<String>* r_extensions) const override;
};

template <typename M>
Vector<Ref<Shape3D>> ResourceImporterScene::get_collision_shapes(
	const Ref<ImporterMesh>& p_mesh, const M& p_options, float p_applied_root_scale)
{
	ERR_FAIL_COND_V(p_mesh.is_null(), Vector<Ref<Shape3D>>());

	ShapeType generate_shape_type = SHAPE_TYPE_AUTOMATIC;
	if (p_options.has(SNAME("physics/shape_type"))) {
		generate_shape_type = (ShapeType)p_options[SNAME("physics/shape_type")].operator int();
	}

	if (generate_shape_type == SHAPE_TYPE_AUTOMATIC) {
		BodyType body_type = BODY_TYPE_STATIC;
		if (p_options.has(SNAME("physics/body_type"))) {
			body_type = (BodyType)p_options[SNAME("physics/body_type")].operator int();
		}

		generate_shape_type =
			body_type == BODY_TYPE_DYNAMIC ? SHAPE_TYPE_DECOMPOSE_CONVEX : SHAPE_TYPE_TRIMESH;
	}

	if (generate_shape_type == SHAPE_TYPE_DECOMPOSE_CONVEX) {
		Ref<MeshConvexDecompositionSettings> decomposition_settings =
			Ref<MeshConvexDecompositionSettings>();
		decomposition_settings.instantiate();
		bool advanced = false;
		if (p_options.has(SNAME("decomposition/advanced"))) {
			advanced = p_options[SNAME("decomposition/advanced")];
		}

		if (advanced) {
			if (p_options.has(SNAME("decomposition/max_concavity"))) {
				decomposition_settings->set_max_concavity(
					p_options[SNAME("decomposition/max_concavity")]);
			}

			if (p_options.has(SNAME("decomposition/symmetry_planes_clipping_bias"))) {
				decomposition_settings->set_symmetry_planes_clipping_bias(
					p_options[SNAME("decomposition/symmetry_planes_clipping_bias")]);
			}

			if (p_options.has(SNAME("decomposition/revolution_axes_clipping_bias"))) {
				decomposition_settings->set_revolution_axes_clipping_bias(
					p_options[SNAME("decomposition/revolution_axes_clipping_bias")]);
			}

			if (p_options.has(SNAME("decomposition/min_volume_per_convex_hull"))) {
				decomposition_settings->set_min_volume_per_convex_hull(
					p_options[SNAME("decomposition/min_volume_per_convex_hull")]);
			}

			if (p_options.has(SNAME("decomposition/resolution"))) {
				decomposition_settings->set_resolution(
					p_options[SNAME("decomposition/resolution")]);
			}

			if (p_options.has(SNAME("decomposition/max_num_vertices_per_convex_hull"))) {
				decomposition_settings->set_max_num_vertices_per_convex_hull(
					p_options[SNAME("decomposition/max_num_vertices_per_convex_hull")]);
			}

			if (p_options.has(SNAME("decomposition/plane_downsampling"))) {
				decomposition_settings->set_plane_downsampling(
					p_options[SNAME("decomposition/plane_downsampling")]);
			}

			if (p_options.has(SNAME("decomposition/convexhull_downsampling"))) {
				decomposition_settings->set_convex_hull_downsampling(
					p_options[SNAME("decomposition/convexhull_downsampling")]);
			}

			if (p_options.has(SNAME("decomposition/normalize_mesh"))) {
				decomposition_settings->set_normalize_mesh(
					p_options[SNAME("decomposition/normalize_mesh")]);
			}

			if (p_options.has(SNAME("decomposition/mode"))) {
				decomposition_settings->set_mode(
					(MeshConvexDecompositionSettings::Mode)p_options[SNAME("decomposition/mode")].
					operator int());
			}

			if (p_options.has(SNAME("decomposition/convexhull_approximation"))) {
				decomposition_settings->set_convex_hull_approximation(
					p_options[SNAME("decomposition/convexhull_approximation")]);
			}

			if (p_options.has(SNAME("decomposition/max_convex_hulls"))) {
				decomposition_settings->set_max_convex_hulls(
					MAX(1, (int)p_options[SNAME("decomposition/max_convex_hulls")]));
			}

			if (p_options.has(SNAME("decomposition/project_hull_vertices"))) {
				decomposition_settings->set_project_hull_vertices(
					p_options[SNAME("decomposition/project_hull_vertices")]);
			}
		}
		else {
			int precision_level = 5;
			if (p_options.has(SNAME("decomposition/precision"))) {
				precision_level = p_options[SNAME("decomposition/precision")];
			}

			const real_t precision = real_t(precision_level - 1) / 9.0;

			decomposition_settings->set_max_concavity(
				Math::lerp(real_t(1.0), real_t(0.001), precision));
			decomposition_settings->set_min_volume_per_convex_hull(
				Math::lerp(real_t(0.01), real_t(0.0001), precision));
			decomposition_settings->set_resolution(Math::lerp(10'000, 100'000, precision));
			decomposition_settings->set_max_num_vertices_per_convex_hull(
				Math::lerp(32, 64, precision));
			decomposition_settings->set_plane_downsampling(Math::lerp(3, 16, precision));
			decomposition_settings->set_convex_hull_downsampling(Math::lerp(3, 16, precision));
			decomposition_settings->set_max_convex_hulls(Math::lerp(1, 32, precision));
		}

		return p_mesh->convex_decompose(decomposition_settings);
	}
	else if (generate_shape_type == SHAPE_TYPE_SIMPLE_CONVEX) {
		Vector<Ref<Shape3D>> shapes;
		shapes.push_back(p_mesh->create_convex_shape(true,
			/*Passing false, otherwise VHACD will be used to simplify (Decompose) the Mesh.*/
			false));
		return shapes;
	}
	else if (generate_shape_type == SHAPE_TYPE_TRIMESH) {
		Vector<Ref<Shape3D>> shapes;
		shapes.push_back(p_mesh->create_trimesh_shape());
		return shapes;
	}
	else if (generate_shape_type == SHAPE_TYPE_BOX) {
		Ref<Shape3D> box = memnew(Shape3D(Shape3D::SHAPE_BOX));
		if (p_options.has(SNAME("primitive/size"))) {
			box->set_size(
				p_options[SNAME("primitive/size")].operator Vector3() * p_applied_root_scale);
		}
		else {
			box->set_size(Vector3(2, 2, 2) * p_applied_root_scale);
		}

		Vector<Ref<Shape3D>> shapes;
		shapes.push_back(box);
		return shapes;

	}
	else if (generate_shape_type == SHAPE_TYPE_SPHERE) {
		Ref<Shape3D> sphere = memnew(Shape3D(Shape3D::SHAPE_SPHERE));
		if (p_options.has(SNAME("primitive/radius"))) {
			sphere->set_radius(
				p_options[SNAME("primitive/radius")].operator float() * p_applied_root_scale);
		}
		else {
			sphere->set_radius(1.0f * p_applied_root_scale);
		}

		Vector<Ref<Shape3D>> shapes;
		shapes.push_back(sphere);
		return shapes;
	}
	else if (generate_shape_type == SHAPE_TYPE_CYLINDER) {
		Ref<Shape3D> cylinder = memnew(Shape3D(Shape3D::SHAPE_CYLINDER));
		if (p_options.has(SNAME("primitive/height"))) {
			cylinder->set_height(
				p_options[SNAME("primitive/height")].operator float() * p_applied_root_scale);
		}
		else {
			cylinder->set_height(1.0f * p_applied_root_scale);
		}
		if (p_options.has(SNAME("primitive/radius"))) {
			cylinder->set_radius(
				p_options[SNAME("primitive/radius")].operator float() * p_applied_root_scale);
		}
		else {
			cylinder->set_radius(1.0f * p_applied_root_scale);
		}

		Vector<Ref<Shape3D>> shapes;
		shapes.push_back(cylinder);
		return shapes;
	}
	else if (generate_shape_type == SHAPE_TYPE_CAPSULE) {
		Ref<Shape3D> capsule = memnew(Shape3D(Shape3D::SHAPE_CAPSULE));
		if (p_options.has(SNAME("primitive/height"))) {
			capsule->set_height(
				p_options[SNAME("primitive/height")].operator float() * p_applied_root_scale);
		}
		else {
			capsule->set_height(1.0f * p_applied_root_scale);
		}
		if (p_options.has(SNAME("primitive/radius"))) {
			capsule->set_radius(
				p_options[SNAME("primitive/radius")].operator float() * p_applied_root_scale);
		}
		else {
			capsule->set_radius(1.0f * p_applied_root_scale);
		}

		Vector<Ref<Shape3D>> shapes;
		shapes.push_back(capsule);
		return shapes;
	}
	return Vector<Ref<Shape3D>>();
}

template <typename M>
Transform3D ResourceImporterScene::get_collision_shapes_transform(const M& p_options)
{
	Transform3D transform;

	ShapeType generate_shape_type = SHAPE_TYPE_AUTOMATIC;
	if (p_options.has(SNAME("physics/shape_type"))) {
		generate_shape_type = (ShapeType)p_options[SNAME("physics/shape_type")].operator int();
	}

	if (generate_shape_type == SHAPE_TYPE_AUTOMATIC) {
		BodyType body_type = BODY_TYPE_STATIC;
		if (p_options.has(SNAME("physics/body_type"))) {
			body_type = (BodyType)p_options[SNAME("physics/body_type")].operator int();
		}

		generate_shape_type =
			body_type == BODY_TYPE_DYNAMIC ? SHAPE_TYPE_DECOMPOSE_CONVEX : SHAPE_TYPE_TRIMESH;
	}

	if (generate_shape_type == SHAPE_TYPE_BOX || generate_shape_type == SHAPE_TYPE_SPHERE ||
		generate_shape_type == SHAPE_TYPE_CYLINDER || generate_shape_type == SHAPE_TYPE_CAPSULE) {
		if (p_options.has(SNAME("primitive/position"))) {
			transform.origin = p_options[SNAME("primitive/position")];
		}

		if (p_options.has(SNAME("primitive/rotation"))) {
			transform.basis = Basis::from_euler(
				p_options[SNAME("primitive/rotation")].operator Vector3() * (Math::PI / 180.0));
		}
	}
	return transform;
}

// --- Folded from editor/import/editor_import_plugin.h ---
#include "core/io/resource_loader.h"

class EditorImportPlugin : public ResourceImporter
{
protected:

public:
	virtual String get_importer_name() const override;
	virtual String get_visible_name() const override;
	virtual void get_recognized_extensions(List<String>* p_extensions) const override;
	virtual String get_preset_name(int p_idx) const override;
	virtual int get_preset_count() const override;
	virtual String get_save_extension() const override;
	virtual String get_resource_type() const override;
	virtual float get_priority() const override;
	virtual int get_import_order() const override;
	virtual int get_format_version() const override;
	virtual void get_import_options(
		const String& p_path, List<ImportOption>* r_options, int p_preset) const override;
	virtual bool can_import_threaded() const override;
};

// --- Folded from editor/import/resource_importer_csv_translation.h ---
#include "core/io/resource_loader.h"

class ResourceImporterCSVTranslation : public ResourceImporter
{
public:
	virtual String get_importer_name() const override;
	virtual String get_visible_name() const override;
	virtual void get_recognized_extensions(List<String>* p_extensions) const override;
	virtual String get_save_extension() const override;
	virtual String get_resource_type() const override;

	virtual int get_preset_count() const override;
	virtual String get_preset_name(int p_idx) const override;

	virtual void get_import_options(
		const String& p_path, List<ImportOption>* r_options, int p_preset = 0) const override;

	virtual bool can_import_threaded() const override { return true; }
};



// --- End editor/import/resource_importer_csv_translation.h ---

// --- Folded from editor/import/resource_importer_texture.h ---
#include "core/io/file_access.h"
#include "core/io/image.h"
#include "core/io/resource_loader.h"
#include "servers/rendering/rendering_server_enums.h"

class CompressedTexture2D;

class LayeredTextureImport final : public RefCounted
{
public:
	Image::CompressSource* csource = nullptr;
	String save_path;
	List<String>* platform_variants = nullptr;
	Ref<Image> image = nullptr;
	Vector<Ref<Image>>* slices = nullptr;
	int compress_mode = 0;
	float lossy = 1.0;

	Image::BasisUniversalPackerParams basisu_params;

	int hdr_compression = 0;
	bool mipmaps = true;
	bool high_quality = false;
	Image::CompressProfile compression_profile = Image::COMPRESS_PROFILE_AUTOMATIC;
	Image::UsedChannels used_channels = Image::USED_CHANNELS_RGBA;
};

class ResourceImporterTexture final : public ResourceImporter
{
public:
	enum ImportTarget
	{
		TARGET_TEXTURE_2D,
		TARGET_LAYERED_2D_ARRAY,
		TARGET_CUBEMAP,
		TARGET_CUBEMAP_ARRAY,
		TARGET_TEXTURE_3D,
		TARGET_TEXTURE_ATLAS,
		TARGET_IMAGE,
		TARGET_BITMAP,
		TARGET_SVG,
	};

	enum Mode
	{
		MODE_2D_ARRAY,
		MODE_CUBEMAP,
		MODE_CUBEMAP_ARRAY,
		MODE_3D,
	};

	enum CubemapFormat
	{
		CUBEMAP_FORMAT_1X6,
		CUBEMAP_FORMAT_2X3,
		CUBEMAP_FORMAT_3X2,
		CUBEMAP_FORMAT_6X1,
	};

	enum TextureFlags
	{
		TEXTURE_FLAGS_MIPMAPS = 1
	};

	enum ImportMode
	{
		IMPORT_MODE_REGION,
		IMPORT_MODE_2D_MESH
	};

	enum CompressMode
	{
		COMPRESS_LOSSLESS,
		COMPRESS_LOSSY,
		COMPRESS_VRAM_COMPRESSED,
		COMPRESS_VRAM_UNCOMPRESSED,
		COMPRESS_BASIS_UNIVERSAL
	};

	enum ChannelRemap
	{
		REMAP_R,
		REMAP_G,
		REMAP_B,
		REMAP_A,
		REMAP_INV_R,
		REMAP_INV_G,
		REMAP_INV_B,
		REMAP_INV_A,
		REMAP_UNUSED,
		REMAP_0,
		REMAP_1,
	};

	enum Preset
	{
		PRESET_DETECT,
		PRESET_2D,
		PRESET_3D,
	};

	static inline ResourceImporterTexture* singleton = nullptr;

private:
	struct PackData
	{
		Rect2 region;
		bool is_cropped = false;
		bool is_mesh = false;
		Vector<int> chart_pieces;				// one for region, many for mesh
		Vector<Vector<Vector2>> chart_vertices; // for mesh
		Ref<Image> image;
	};

	enum
	{
		MAKE_3D_FLAG = 1,
		MAKE_ROUGHNESS_FLAG = 2,
		MAKE_NORMAL_FLAG = 4
	};

	ImportTarget target = TARGET_TEXTURE_2D;

	static inline Mutex mutex;

	struct MakeInfo
	{
		int flags = 0;
		String normal_path_for_roughness;
		RSE::TextureDetectRoughnessChannel channel_for_roughness = RSE::TEXTURE_DETECT_ROUGHNESS_R;
	};

	static inline HashMap<String, MakeInfo> make_flags;

	static void _texture_reimport_roughness(const Ref<CompressedTexture2D>& p_tex,
		const String& p_normal_path, RSE::TextureDetectRoughnessChannel p_channel);
	static void _texture_reimport_3d(const Ref<CompressedTexture2D>& p_tex);
	static void _texture_reimport_normal(const Ref<CompressedTexture2D>& p_tex);

	static const char* compression_formats[];

	static inline void _remap_channels(Ref<Image>& r_image, ChannelRemap p_options[4]);
	static inline void _clamp_hdr_exposure(Ref<Image>& r_image);
	static inline void _invert_y_channel(Ref<Image>& r_image);

	Mode _get_layered_mode() const;

public:
	static void save_to_ctex_format(Ref<FileAccess> f, const Ref<Image>& p_image,
		CompressMode p_compress_mode, Image::UsedChannels p_channels,
		Image::CompressMode p_compress_format, Image::CompressProfile p_compress_profile,
		float p_lossy_quality, const Image::BasisUniversalPackerParams& p_basisu_params,
		Image::BPTCFormat p_bptc_format);

	void _check_compress_ctex(
		const String& p_source_file, Ref<LayeredTextureImport> r_texture_import);

	void _save_tex(Vector<Ref<Image>> p_images, const String& p_to_path, int p_compress_mode,
		float p_lossy, const Image::BasisUniversalPackerParams& p_basisu_params,
		Image::CompressMode p_vram_compression, Image::CompressProfile p_vram_compression_profile,
		Image::CompressSource p_csource, Image::UsedChannels used_channels, bool p_mipmaps,
		bool p_force_po2, Image::BPTCFormat p_bptc_format);

	virtual String get_importer_name() const override;
	virtual String get_visible_name() const override;
	virtual void get_recognized_extensions(List<String>* p_extensions) const override;
	virtual String get_save_extension() const override;
	virtual String get_resource_type() const override;

	virtual int get_preset_count() const override;
	virtual String get_preset_name(int p_idx) const override;

	virtual void get_import_options(
		const String& p_path, List<ImportOption>* r_options, int p_preset = 0) const override;
	virtual String get_option_group_file() const override;

	virtual bool can_import_threaded() const override { return true; }

	void update_imports();

	virtual String get_import_settings_string() const override;

	ImportTarget get_target() const { return target; }
	void set_target(ImportTarget p_target) { target = p_target; }
	void set_mode(Mode p_mode);

	ResourceImporterTexture(bool p_singleton = false);
	ResourceImporterTexture(ImportTarget p_target, bool p_singleton = false);
	~ResourceImporterTexture();
};

// --- End editor/import/resource_importer_texture.h ---
// --- End editor/import/3d/resource_importer_scene.h ---

class Animation;
class Node;

class EditorSceneFormatImporterGLTF final : public EditorSceneFormatImporter
{
public:
	virtual void get_extensions(List<String>* r_extensions) const override;
	virtual void get_import_options(
		const String& p_path, List<ResourceImporter::ImportOption>* r_options) override;
};
#endif // TOOLS_ENABLED

// --- Folded from modules/csg/csg_shape.h ---
#include "core/math/aabb.h"
#include "core/types.h"
#include "core/math/vector2.h"
#include "core/math/vector3.h"
#include "core/templates/vector.h"
#include "scene/resources/material.h"

struct CSGBrush
{
	struct Face
	{
		Vector3 vertices[3];
		Vector2 uvs[3];
		AABB aabb;
		bool smooth = false;
		bool invert = false;
		int material = 0;
	};

	Vector<Face> faces;
	Vector<Ref<Material>> materials;

	inline void _regen_face_aabbs()
	{
		for (int i = 0; i < faces.size(); i++) {
			faces.write[i].aabb = AABB();
			faces.write[i].aabb.position = faces[i].vertices[0];
			faces.write[i].aabb.expand_to(faces[i].vertices[1]);
			faces.write[i].aabb.expand_to(faces[i].vertices[2]);
		}
	}

	// Create a brush from faces.
	void build_from_faces(const Vector<Vector3>& p_vertices, const Vector<Vector2>& p_uvs,
		const Vector<bool>& p_smooth, const Vector<Ref<Material>>& p_materials,
		const Vector<bool>& p_invert_faces);
	void copy_from(const CSGBrush& p_brush, const Transform3D& p_xform);
};
#include "scene/3d/node_3d.h"
#include "scene/resources/mesh.h"

#ifndef PHYSICS_3D_DISABLED
#include "scene/resources/mesh.h"
#endif // PHYSICS_3D_DISABLED

class NavigationMesh;
class NavigationMeshSourceGeometryData3D;

class CSGShape3D : public Node3D
{
public:
	enum ShapeType
	{
		SHAPE_COMBINER,
		SHAPE_BOX,
		SHAPE_SPHERE,
		SHAPE_CYLINDER,
		SHAPE_TORUS,
		SHAPE_MESH,
		SHAPE_POLYGON,
	};

	enum Operation
	{
		OPERATION_UNION,
		OPERATION_INTERSECTION,
		OPERATION_SUBTRACTION,
	};

	enum PolygonMode
	{
		POLYGON_MODE_DEPTH,
		POLYGON_MODE_SPIN,
		POLYGON_MODE_PATH,
	};

	enum PathIntervalType
	{
		PATH_INTERVAL_DISTANCE,
		PATH_INTERVAL_SUBDIVIDE,
	};

	enum PathRotation
	{
		PATH_ROTATION_POLYGON,
		PATH_ROTATION_PATH,
		PATH_ROTATION_PATH_FOLLOW,
	};

	RenderInstance3D render_instance;

private:
	ShapeType shape_type = SHAPE_BOX;
	Operation operation = OPERATION_UNION;
	CSGShape3D* parent_shape = nullptr;

	CSGBrush* brush = nullptr;
	AABB node_aabb;

	bool dirty = false;
	bool last_visible = false;
	float snap = 0.001f;

	bool autosmooth = false;
	float smoothing_angle = 50.0f;

#ifndef PHYSICS_3D_DISABLED
	bool use_collision = false;
	uint32_t collision_layer = 1;
	uint32_t collision_mask = 1;
	real_t collision_priority = 1.0;
	Ref<Shape3D> root_collision_shape;
	RID root_collision_body;
	RID root_collision_debug_instance;
	Transform3D debug_shape_old_transform;
#endif // PHYSICS_3D_DISABLED

	bool calculate_tangents = true;
	Ref<ArrayMesh> root_mesh;

	// Shared primitive fields
	bool flip_faces = false;
	bool smooth_faces = true;
	Ref<Material> material;

	// Box / Cylinder / Sphere / Torus parameters
	Vector3 size = Vector3(1, 1, 1);
	float radius = 0.5f;
	float height = 2.0f;
	float inner_radius = 0.5f;
	float outer_radius = 1.0f;
	int radial_segments = 12;
	int rings = 6;
	int sides = 8;
	int ring_sides = 6;
	bool cone = false;

	// Mesh parameters
	Ref<Mesh> mesh;

	// Polygon parameters
	Vector<Vector2> polygon;
	PolygonMode polygon_mode = POLYGON_MODE_DEPTH;
	float depth = 1.0f;
	float spin_degrees = 360.0f;
	int spin_sides = 8;
	NodePath path_node;
	PathIntervalType path_interval_type = PATH_INTERVAL_DISTANCE;
	float path_interval = 1.0f;
	float path_simplify_angle = 0.0f;
	PathRotation path_rotation = PATH_ROTATION_PATH_FOLLOW;
	bool path_rotation_accurate = false;
	bool path_local = false;
	Path3D* path = nullptr;
	bool path_continuous_u = true;
	real_t path_u_distance = 1.0;
	bool path_joined = false;

	struct ShapeUpdateSurface
	{
		Vector<Vector3> vertices;
		Vector<Vector3> normals;
		Vector<Vector2> uvs;
		Vector<float> tans;
		Ref<Material> material;
		int last_added = 0;

		Vector3* verticesw = nullptr;
		Vector3* normalsw = nullptr;
		Vector2* uvsw = nullptr;
		float* tansw = nullptr;
	};

#ifndef PHYSICS_3D_DISABLED
	void _update_collision_faces();
	bool _is_debug_collision_shape_visible();
	void _update_debug_collision_shape();
	void _clear_debug_collision_shape();
	void _on_transform_changed();
	Vector<Vector3> _get_brush_collision_faces();
#endif // PHYSICS_3D_DISABLED

	void _build_surfaces_smoothed(
		CSGBrush* p_brush, Vector<ShapeUpdateSurface>& r_surfaces, Vector<int>& r_face_count);
	void _build_surfaces_default(
		CSGBrush* p_brush, Vector<ShapeUpdateSurface>& r_surfaces, Vector<int>& r_face_count);

	CSGBrush* _create_brush_from_arrays(const Vector<Vector3>& p_vertices,
		const Vector<Vector2>& p_uv, const Vector<bool>& p_smooth,
		const Vector<Ref<Material>>& p_materials);
	CSGBrush* _build_brush();
	CSGBrush* _build_box_brush();
	CSGBrush* _build_sphere_brush();
	CSGBrush* _build_cylinder_brush();
	CSGBrush* _build_torus_brush();
	CSGBrush* _build_mesh_brush();
	CSGBrush* _build_polygon_brush();
	CSGBrush* _get_brush();

	void _make_dirty(bool p_parent_removing = false);
	void _path_exited();
	bool _is_editable_3d_polygon() const;
	bool _has_editable_3d_polygon_no_depth() const;

protected:
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;
	PackedStringArray get_configuration_warnings() const override;

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	void set_shape_type(ShapeType p_type);
	ShapeType get_shape_type() const { return shape_type; }

	void update_shape();

	void set_operation(Operation p_operation);
	Operation get_operation() const;

	Vector<Vector3> get_brush_faces();
	AABB get_aabb() const;

#ifndef PHYSICS_3D_DISABLED
	void set_use_collision(bool p_enable);
	bool is_using_collision() const;

	void set_collision_layer(uint32_t p_layer);
	uint32_t get_collision_layer() const;

	void set_collision_mask(uint32_t p_mask);
	uint32_t get_collision_mask() const;

	void set_collision_layer_value(int p_layer_number, bool p_value);
	bool get_collision_layer_value(int p_layer_number) const;

	void set_collision_mask_value(int p_layer_number, bool p_value);
	bool get_collision_mask_value(int p_layer_number) const;

	RID _get_root_collision_instance() const;

	void set_collision_priority(real_t p_priority);
	real_t get_collision_priority() const;

	void set_autosmooth(bool p_smooth);
	bool is_autosmooth() const;

	void set_smoothing_angle(const float p_angle);
	float get_smoothing_angle() const;
#endif // PHYSICS_3D_DISABLED

#ifndef DISABLE_DEPRECATED
	void set_snap(float p_snap);
	float get_snap() const;
#endif // DISABLE_DEPRECATED

	void set_calculate_tangents(bool p_calculate_tangents);
	bool is_calculating_tangents() const;

	bool is_root_shape() const;

	Ref<ArrayMesh> bake_static_mesh();
#ifndef PHYSICS_3D_DISABLED
	Ref<Shape3D> bake_collision_shape();
#endif // PHYSICS_3D_DISABLED

	Ref<TriangleMesh> generate_triangle_mesh() const;

	// Primitive / Shape parameters
	void set_flip_faces(bool p_invert);
	bool get_flip_faces() const;

	void set_smooth_faces(bool p_smooth_faces);
	bool get_smooth_faces() const;

	void set_material(const Ref<Material>& p_material);
	Ref<Material> get_material() const;

	void set_size(const Vector3& p_size);
	Vector3 get_size() const { return size; }

	void set_radius(float p_radius);
	float get_radius() const { return radius; }

	void set_height(float p_height);
	float get_height() const { return height; }

	void set_inner_radius(float p_radius);
	float get_inner_radius() const { return inner_radius; }

	void set_outer_radius(float p_radius);
	float get_outer_radius() const { return outer_radius; }

	void set_radial_segments(int p_segments);
	int get_radial_segments() const { return radial_segments; }

	void set_rings(int p_rings);
	int get_rings() const { return rings; }

	void set_sides(int p_sides);
	int get_sides() const { return sides; }

	void set_ring_sides(int p_ring_sides);
	int get_ring_sides() const { return ring_sides; }

	void set_cone(bool p_cone);
	bool is_cone() const { return cone; }

	void set_mesh(const Ref<Mesh>& p_mesh);
	Ref<Mesh> get_mesh() const { return mesh; }

	void set_polygon(const Vector<Vector2>& p_polygon);
	Vector<Vector2> get_polygon() const { return polygon; }

	void set_polygon_mode(PolygonMode p_mode);
	PolygonMode get_polygon_mode() const { return polygon_mode; }

	void set_depth(float p_depth);
	float get_depth() const { return depth; }

	void set_spin_degrees(float p_degrees);
	float get_spin_degrees() const { return spin_degrees; }

	void set_spin_sides(int p_sides);
	int get_spin_sides() const { return spin_sides; }

	void set_path_node(const NodePath& p_path);
	NodePath get_path_node() const { return path_node; }

	void set_path_interval_type(PathIntervalType p_type);
	PathIntervalType get_path_interval_type() const { return path_interval_type; }

	void set_path_interval(float p_interval);
	float get_path_interval() const { return path_interval; }

	void set_path_simplify_angle(float p_angle);
	float get_path_simplify_angle() const { return path_simplify_angle; }

	void set_path_rotation(PathRotation p_rotation);
	PathRotation get_path_rotation() const { return path_rotation; }

	void set_path_rotation_accurate(bool p_accurate);
	bool get_path_rotation_accurate() const { return path_rotation_accurate; }

	void set_path_local(bool p_local);
	bool is_path_local() const { return path_local; }

	void set_path_continuous_u(bool p_enable);
	bool is_path_continuous_u() const { return path_continuous_u; }

	void set_path_u_distance(real_t p_distance);
	real_t get_path_u_distance() const { return path_u_distance; }

	void set_path_joined(bool p_joined);
	bool is_path_joined() const { return path_joined; }

#ifndef NAVIGATION_3D_DISABLED
private:
	static RID _navmesh_source_geometry_parser;

public:
	static void navmesh_parse_init();
	static void navmesh_parse_source_geometry(const Ref<NavigationMesh>& p_navigation_mesh,
		Ref<NavigationMeshSourceGeometryData3D> p_source_geometry_data, Node* p_node);
#endif // NAVIGATION_3D_DISABLED

	CSGShape3D(ShapeType p_type = SHAPE_BOX);
	~CSGShape3D();
};

// --- Folded from modules/gridmap/grid_map.h ---
#include "scene/3d/node_3d.h"
#include "scene/resources/mesh.h"

class NavigationMesh;
class NavigationMeshSourceGeometryData3D;
#ifndef PHYSICS_3D_DISABLED
class PhysicsMaterial;
#endif // PHYSICS_3D_DISABLED

class GridMap : public Node3D
{
	enum DebugVisibilityMode
	{
		DEBUG_VISIBILITY_MODE_DEFAULT,
		DEBUG_VISIBILITY_MODE_FORCE_SHOW,
		DEBUG_VISIBILITY_MODE_FORCE_HIDE,
	};

	enum
	{
		MAP_DIRTY_TRANSFORMS = 1,
		MAP_DIRTY_INSTANCES = 2,
	};

	union IndexKey
	{
		struct
		{
			int16_t x;
			int16_t y;
			int16_t z;
		};

		uint64_t key = 0;

		static uint32_t hash(const IndexKey& p_key) { return hash_one_uint64(p_key.key); }

		_FORCE_INLINE_ bool operator<(const IndexKey& p_key) const { return key < p_key.key; }

		_FORCE_INLINE_ bool operator==(const IndexKey& p_key) const { return key == p_key.key; }

		_FORCE_INLINE_ operator Vector3i() const { return Vector3i(x, y, z); }

		uint32_t hash() const { return operator Vector3i().hash(); }

		IndexKey(Vector3i p_vector)
		{
			x = (int16_t)p_vector.x;
			y = (int16_t)p_vector.y;
			z = (int16_t)p_vector.z;
		}

		IndexKey() {}
	};

	/**
	 * @brief A Cell is a single cell in the cube map space; it is defined by its coordinates and
	 * the populating Item, identified by int id.
	 */
	union Cell
	{
		struct
		{
			unsigned int item : 16;
			unsigned int rot : 5;
			unsigned int layer : 8;
		};

		uint32_t cell = 0;
	};

	/**
	 * @brief An Octant is a prism containing Cells, and possibly belonging to an Area.
	 * A GridMap can have multiple Octants.
	 */
	struct Octant
	{
		struct NavigationCell
		{
			RID region;
			Transform3D xform;
			RID navigation_mesh_debug_instance;
			uint32_t navigation_layers = 1;
		};

		struct MultimeshInstance
		{
			RID instance;
			RID multimesh;

			struct Item
			{
				int index = 0;
				Transform3D transform;
				IndexKey key;
			};

			Vector<Item> items; // tools only, for changing visibility
		};

		Vector<MultimeshInstance> multimesh_instances;
		HashSet<IndexKey> cells;
		RID collision_debug;
		RID collision_debug_instance;
#ifdef DEBUG_ENABLED
		RID navigation_debug_edge_connections_instance;
		Ref<ArrayMesh> navigation_debug_edge_connections_mesh;
#endif // DEBUG_ENABLED

		bool dirty = false;
		RID static_body;
		HashMap<IndexKey, NavigationCell> navigation_cell_ids;
	};

	union OctantKey
	{
		struct
		{
			int16_t x;
			int16_t y;
			int16_t z;
			int16_t empty;
		};

		uint64_t key = 0;

		static uint32_t hash(const OctantKey& p_key) { return hash_one_uint64(p_key.key); }

		_FORCE_INLINE_ bool operator==(const OctantKey& p_key) const { return key == p_key.key; }

		// OctantKey(const IndexKey& p_k, int p_item) { indexkey=p_k.key; item=p_item; }
		OctantKey() {}
	};

	OctantKey get_octant_key_from_index_key(const IndexKey& p_index_key) const;
	OctantKey get_octant_key_from_cell_coords(const Vector3i& p_cell_coords) const;

#ifndef PHYSICS_3D_DISABLED
	uint32_t collision_layer = 1;
	uint32_t collision_mask = 1;
	real_t collision_priority = 1.0;
	DebugVisibilityMode collision_visibility_mode = DEBUG_VISIBILITY_MODE_DEFAULT;
	Ref<PhysicsMaterial> physics_material;
#endif // PHYSICS_3D_DISABLED
	bool bake_navigation = false;
	RID map_override;

	Transform3D last_transform;

	bool _in_tree = false;
	Vector3 cell_size = Vector3(2, 2, 2);
	int octant_size = 8;
	bool center_x = true;
	bool center_y = true;
	bool center_z = true;
	float cell_scale = 1.0;

	bool recreating_octants = false;

	Ref<MeshLibrary> mesh_library;

	HashMap<OctantKey, Octant*, OctantKey> octant_map;
	HashMap<IndexKey, Cell, IndexKey> cell_map;

	void _recreate_octant_data();

	struct BakeLight
	{
		RSE::LightType type = RSE::LightType::LIGHT_DIRECTIONAL;
		Vector3 pos;
		Vector3 dir;
		float param[RSE::LIGHT_PARAM_MAX] = {};
	};

	_FORCE_INLINE_ Vector3 _octant_get_offset(const OctantKey& p_key) const
	{
		return Vector3(p_key.x, p_key.y, p_key.z) * cell_size * octant_size;
	}

#ifndef PHYSICS_3D_DISABLED
	void _update_physics_bodies_collision_properties();
	void _update_physics_bodies_characteristics();
#endif // PHYSICS_3D_DISABLED
	void _octant_enter_world(const OctantKey& p_key);
	void _octant_exit_world(const OctantKey& p_key);
	bool _octant_update(const OctantKey& p_key);
	void _octant_clean_up(const OctantKey& p_key);
	void _octant_transform(const OctantKey& p_key);
#if defined(DEBUG_ENABLED) && !defined(NAVIGATION_3D_DISABLED)
	void _update_octant_navigation_debug_edge_connections_mesh(const OctantKey& p_key);
	void _navigation_map_changed(RID p_map);
	void _update_navigation_debug_edge_connections();
#endif // defined(DEBUG_ENABLED) && !defined(NAVIGATION_3D_DISABLED)
	bool awaiting_update = false;

	void _queue_octants_dirty();
	void _update_octants_callback();

#ifndef DISABLE_DEPRECATED
	void resource_changed(const Ref<Resource>& p_res);
#endif

	void _clear_internal();

	Vector3 _get_offset() const;

	struct BakedMesh
	{
		Ref<Mesh> mesh;
		RID instance;
	};

	Vector<BakedMesh> baked_meshes;

	bool debug_show_octants = false;
	Color debug_octant_color = Color(1.0, 1.0, 1.0, 1.0);
#ifdef DEBUG_ENABLED
	bool debug_dirty = false;
	Ref<StandardMaterial3D> debug_octant_line_material;

	void _debug_update();
	void _debug_update_octants();
	void _debug_clear_octants();

	RID debug_octant_line_mesh_rid;

	struct OctantDebug
	{
		RID debug_line_mesh_rid;
		RID debug_line_instance_rid;
	};

	HashMap<OctantKey, OctantDebug*, OctantKey> debug_octant_map;

#endif // DEBUG_ENABLED

protected:
	void _notification(int p_what);
	void _update_visibility();

public:
	enum
	{
		INVALID_CELL_ITEM = -1
	};

#ifndef PHYSICS_3D_DISABLED
	void set_collision_layer(uint32_t p_layer);
	uint32_t get_collision_layer() const;

	void set_collision_mask(uint32_t p_mask);
	uint32_t get_collision_mask() const;

	void set_collision_layer_value(int p_layer_number, bool p_value);
	bool get_collision_layer_value(int p_layer_number) const;

	void set_collision_mask_value(int p_layer_number, bool p_value);
	bool get_collision_mask_value(int p_layer_number) const;

	void set_collision_priority(real_t p_priority);
	real_t get_collision_priority() const;

	void set_collision_visibility_mode(DebugVisibilityMode p_visibility_mode);
	DebugVisibilityMode get_collision_visibility_mode() const;

	void set_physics_material(Ref<PhysicsMaterial> p_material);
	Ref<PhysicsMaterial> get_physics_material() const;

#endif // PHYSICS_3D_DISABLED

	void set_bake_navigation(bool p_bake_navigation);
	bool is_baking_navigation();

#ifndef NAVIGATION_3D_DISABLED
	void set_navigation_map(RID p_navigation_map);
	RID get_navigation_map() const;
#endif // NAVIGATION_3D_DISABLED

	void set_mesh_library(const Ref<MeshLibrary>& p_mesh_library);
	Ref<MeshLibrary> get_mesh_library() const;

	void set_cell_size(const Vector3& p_size);
	Vector3 get_cell_size() const;

	void set_octant_size(int p_size);
	int get_octant_size() const;

	void set_center_x(bool p_enable);
	bool get_center_x() const;
	void set_center_y(bool p_enable);
	bool get_center_y() const;
	void set_center_z(bool p_enable);
	bool get_center_z() const;

	void set_cell_item(const Vector3i& p_position, int p_item, int p_rot = 0);
	int get_cell_item(const Vector3i& p_position) const;
	int get_cell_item_orientation(const Vector3i& p_position) const;
	Basis get_cell_item_basis(const Vector3i& p_position) const;
	Basis get_basis_with_orthogonal_index(int p_index) const;
	int get_orthogonal_index_from_basis(const Basis& p_basis) const;

	Vector3i local_to_map(const Vector3& p_local_position) const;
	Vector3 map_to_local(const Vector3i& p_map_position) const;

	void set_cell_scale(float p_scale);
	float get_cell_scale() const;

	TypedArray<Vector3i> get_used_cells() const;
	TypedArray<Vector3i> get_used_cells_by_item(int p_item) const;

	TypedArray<Vector3i> get_used_octants() const;
	TypedArray<Vector3i> get_used_octants_by_item(int p_item) const;

	TypedArray<Vector3i> get_used_cells_in_octant(const Vector3i& p_octant_coords) const;
	TypedArray<Vector3i> get_used_cells_in_octant_by_item(
		const Vector3i& p_octant_coords, int p_item) const;

	// Fastpath functions for native modules that do not use Variant/TypedArray.
	LocalVector<IndexKey> get_index_keys_in_bounds(
		const AABB& p_bounds, bool p_used_only = true) const;
	LocalVector<OctantKey> get_octant_keys_in_bounds(
		const AABB& p_bounds, bool p_used_only = true) const;

	TypedArray<Vector3i> get_octants_in_bounds(const AABB& p_bounds) const;
	TypedArray<Vector3i> get_used_octants_in_bounds(const AABB& p_bounds) const;

	Vector3i get_octant_coords_from_cell_coords(const Vector3i& p_cell_coords) const;

#ifndef PHYSICS_3D_DISABLED
	RID get_physics_body_from_octant_coord(const Vector3i& p_octant_coords) const;
#endif

	void clear_baked_meshes();
	void make_baked_meshes(bool p_gen_lightmap_uv = false, float p_lightmap_uv_texel_size = 0.1);

	void clear();

	RID get_bake_mesh_instance(int p_idx);

	void set_debug_show_octants(bool p_enable);
	bool get_debug_show_octants() const;

	void set_debug_octant_color(const Color& p_color);
	Color get_debug_octant_color() const;

#ifndef NAVIGATION_3D_DISABLED
private:
	static RID _navmesh_source_geometry_parser;
#endif // NAVIGATION_3D_DISABLED

public:
#ifndef NAVIGATION_3D_DISABLED
	static void navmesh_parse_init();
	static void navmesh_parse_source_geometry(const Ref<NavigationMesh>& p_navigation_mesh,
		Ref<NavigationMeshSourceGeometryData3D> p_source_geometry_data, Node* p_node);
#endif // NAVIGATION_3D_DISABLED

	GridMap();
	~GridMap();
};
// --- End modules/gridmap/grid_map.h ---
// --- End modules/csg/csg_shape.h ---
