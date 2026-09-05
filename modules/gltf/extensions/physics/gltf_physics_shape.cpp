/**************************************************************************/
/*  gltf_physics_shape.cpp                                                */
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

#include "core/math/convex_hull.h"
#include "gltf_physics_shape.h"
#include "scene/3d/physics/area_3d.h"
#include "scene/resources/3d/box_shape_3d.h"
#include "scene/resources/3d/capsule_shape_3d.h"
#include "scene/resources/3d/concave_polygon_shape_3d.h"
#include "scene/resources/3d/convex_polygon_shape_3d.h"
#include "scene/resources/3d/cylinder_shape_3d.h"
#include "scene/resources/3d/importer_mesh.h"
#include "scene/resources/3d/sphere_shape_3d.h"

void GLTFPhysicsShape::_bind_methods() {}

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
			Ref<BoxShape3D> box;
			box.instantiate();
			box->set_size(size);
			_shape_cache = box;
		}
		else if (shape_type == "capsule") {
			Ref<CapsuleShape3D> capsule;
			capsule.instantiate();
			capsule->set_radius(radius);
			capsule->set_height(height);
			_shape_cache = capsule;
		}
		else if (shape_type == "cylinder") {
			Ref<CylinderShape3D> cylinder;
			cylinder.instantiate();
			cylinder->set_radius(radius);
			cylinder->set_height(height);
			_shape_cache = cylinder;
		}
		else if (shape_type == "sphere") {
			Ref<SphereShape3D> sphere;
			sphere.instantiate();
			sphere->set_radius(radius);
			_shape_cache = sphere;
		}
		else if (shape_type == "convex") {
			ERR_FAIL_COND_V_MSG(importer_mesh.is_null(), _shape_cache,
				"GLTFPhysicsShape: Error converting convex hull shape to a shape resource: The "
				"mesh resource is null.");
			Ref<ConvexPolygonShape3D> convex = importer_mesh->get_mesh()->create_convex_shape();
			_shape_cache = convex;
		}
		else if (shape_type == "trimesh") {
			ERR_FAIL_COND_V_MSG(importer_mesh.is_null(), _shape_cache,
				"GLTFPhysicsShape: Error converting concave mesh shape to a shape resource: The "
				"mesh resource is null.");
			Ref<ConcavePolygonShape3D> concave = importer_mesh->create_trimesh_shape();
			_shape_cache = concave;
		}
		else {
			ERR_PRINT("GLTFPhysicsShape: Error converting to a shape resource: Shape type '" +
					  shape_type + "' is unknown.");
		}
	}
	return _shape_cache;
}


