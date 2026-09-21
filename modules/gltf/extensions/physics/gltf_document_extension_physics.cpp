/**************************************************************************/
/*  gltf_document_extension_physics.cpp                                   */
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

#include "gltf_document_extension_physics.h"
#include "gltf_physics_body.h"
#include "gltf_physics_shape.h"
#include "scene/3d/physics/area_3d.h"
#include "scene/3d/physics/rigid_body_3d.h"
#include "scene/3d/physics/static_body_3d.h"

using GLTFShapeIndex = int64_t;

// Import process.

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
		return; // No mesh for this shape.
	}
	Ref<ImporterMesh> importer_mesh = p_gltf_shape->get_importer_mesh();
	if (importer_mesh.is_valid()) {
		return; // The mesh resource is already set up.
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
CollisionObject3D* _generate_shape_with_body(Ref<GLTFState> p_state, Ref<GLTFNode> p_gltf_node,
	Ref<GLTFPhysicsShape> p_physics_shape, Ref<GLTFPhysicsBody> p_physics_body)
{
	print_verbose("glTF: Creating shape with body for: " + p_gltf_node->get_name());
	bool is_trigger = p_physics_shape->get_is_trigger();
	// This method is used for the case where we must generate a parent body.
	// This is can happen for multiple reasons. One possibility is that this
	// glTF file is using OMI_collider but not OMI_physics_body, or at least
	// this particular node is not using it. Another possibility is that the
	// physics body information is set up on the same glTF node, not a parent.
	CollisionObject3D* body;
	if (p_physics_body.is_valid()) {
		// This code is run when the physics body is on the same glTF node.
		body = p_physics_body->to_node();
		if (is_trigger && (p_physics_body->get_body_type() != "trigger")) {
			// Edge case: If the body's trigger and the collider's trigger
			// are in disagreement, we need to create another new body.
			CollisionObject3D* child =
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
		body = memnew(StaticBody3D);
	}
	CollisionShape3D* shape = p_physics_shape->to_node();
	shape->set_name(p_gltf_node->get_name() + "Shape");
	body->add_child(shape);
	return body;
}
#endif // DISABLE_DEPRECATED

// Export process.
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
	// De-duplication: If the state already has the mesh we need, use that one.
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
	// After the loop, we have checked that the mesh is not equal to any of the
	// meshes in the state. So we insert a new mesh into the state mesh array.
	Ref<GLTFMesh> gltf_mesh;
	gltf_mesh.instantiate();
	gltf_mesh->set_mesh(p_mesh);
	GLTFMeshIndex mesh_index = state_meshes.size();
	state_meshes.push_back(gltf_mesh);
	p_state->set_meshes(state_meshes);
	return mesh_index;
}


