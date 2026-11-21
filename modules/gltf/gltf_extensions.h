/**************************************************************************/
/*  gltf_extensions.h                                                     */
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

#include "core/io/image.h"
#include "core/io/resource.h"
#include "gltf_defines.h"
#include "gltf_structures.h"
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

#include "gltf_state.h"

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
#include "scene/3d/physics/collision_shape_3d.h"

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
