/**************************************************************************/
/*  gltf_extensions.cpp                                                   */
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
