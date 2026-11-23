/**************************************************************************/
/*  gltf_state.h                                                          */
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

// --- Inlined/Folded from modules/gltf/gltf_extensions.h ---
#include "core/io/image.h"
#include "core/io/resource.h"

// --- Inlined/Folded from modules/gltf/gltf_structures.h ---
#include "core/io/resource.h"
#include "core/math/expression.h"
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

#include "scene/3d/bone_attachment_3d.h"
#include "scene/3d/skeleton_3d.h"
#include "scene/resources/3d/importer_mesh.h"
#include "scene/resources/3d/skin.h"
#include "scene/resources/animation.h"
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

#include "modules/gltf/gltf_state.h"

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
#include "scene/3d/skeleton_3d.h"

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

#include "scene/3d/importer_mesh_instance_3d.h"
#include "scene/animation/animation_mixer.h"

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


