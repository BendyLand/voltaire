/**************************************************************************/
/*  mesh_instance_3d.cpp                                                  */
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

#include "mesh_instance_3d.h"
#include "scene/3d/skeleton_3d.h"
#include "scene/main/scene_tree.h"
#include "servers/rendering/rendering_server.h"

#ifndef PHYSICS_3D_DISABLED
#include "scene/3d/physics/collision_shape_3d.h"
#include "scene/3d/physics/static_body_3d.h"
#include "scene/resources/3d/concave_polygon_shape_3d.h"
#include "scene/resources/3d/convex_polygon_shape_3d.h"
#endif // PHYSICS_3D_DISABLED

#ifndef NAVIGATION_3D_DISABLED
#include "scene/resources/3d/navigation_mesh_source_geometry_data_3d.h"
#include "scene/resources/navigation_mesh.h"
#include "servers/navigation_3d/navigation_server_3d.h"
#endif // NAVIGATION_3D_DISABLED

#include <cfloat> // FLT_EPSILON

#ifndef NAVIGATION_3D_DISABLED
RID MeshInstance3D::_navmesh_source_geometry_parser;
#endif // NAVIGATION_3D_DISABLED









Ref<Mesh> MeshInstance3D::get_mesh() const { return mesh; }

int MeshInstance3D::get_blend_shape_count() const
{
	if (mesh.is_null()) {
		return 0;
	}
	return mesh->get_blend_shape_count();
}

int MeshInstance3D::find_blend_shape_by_name(const StringName& p_name)
{
	if (mesh.is_null()) {
		return -1;
	}
	for (int i = 0; i < mesh->get_blend_shape_count(); i++) {
		if (mesh->get_blend_shape_name(i) == p_name) {
			return i;
		}
	}
	return -1;
}

float MeshInstance3D::get_blend_shape_value(int p_blend_shape) const
{
	ERR_FAIL_COND_V(mesh.is_null(), 0.0);
	ERR_FAIL_INDEX_V(p_blend_shape, (int)blend_shape_tracks.size(), 0);
	return blend_shape_tracks[p_blend_shape];
}

void MeshInstance3D::set_blend_shape_value(int p_blend_shape, float p_value)
{
	ERR_FAIL_COND(mesh.is_null());
	ERR_FAIL_INDEX(p_blend_shape, (int)blend_shape_tracks.size());
	blend_shape_tracks[p_blend_shape] = p_value;
	RenderingServer::get_singleton()->instance_set_blend_shape_weight(
		get_instance(), p_blend_shape, p_value);
}



void MeshInstance3D::set_skin(const Ref<Skin>& p_skin)
{
	skin_internal = p_skin;
	skin = p_skin;
	if (!is_inside_tree()) {
		return;
	}
	_resolve_skeleton_path();
}

Ref<Skin> MeshInstance3D::get_skin() const { return skin; }

Ref<SkinReference> MeshInstance3D::get_skin_reference() const { return skin_ref; }

void MeshInstance3D::set_skeleton_path(const NodePath& p_skeleton)
{
	skeleton_path = p_skeleton;
	if (!is_inside_tree()) {
		return;
	}
	_resolve_skeleton_path();
}

NodePath MeshInstance3D::get_skeleton_path() { return skeleton_path; }

AABB MeshInstance3D::get_aabb() const
{
	if (mesh.is_valid()) {
		return mesh->get_aabb();
	}

	return AABB();
}

#ifndef PHYSICS_3D_DISABLED
Node* MeshInstance3D::create_trimesh_collision_node()
{
	if (mesh.is_null()) {
		return nullptr;
	}

	Ref<ConcavePolygonShape3D> shape = mesh->create_trimesh_shape();
	if (shape.is_null()) {
		return nullptr;
	}

	StaticBody3D* static_body = memnew(StaticBody3D);
	CollisionShape3D* cshape = memnew(CollisionShape3D);
	cshape->set_shape(shape);
	static_body->add_child(cshape, true);
	return static_body;
}



Node* MeshInstance3D::create_convex_collision_node(bool p_clean, bool p_simplify)
{
	if (mesh.is_null()) {
		return nullptr;
	}

	Ref<ConvexPolygonShape3D> shape = mesh->create_convex_shape(p_clean, p_simplify);
	if (shape.is_null()) {
		return nullptr;
	}

	StaticBody3D* static_body = memnew(StaticBody3D);
	CollisionShape3D* cshape = memnew(CollisionShape3D);
	cshape->set_shape(shape);
	static_body->add_child(cshape, true);
	return static_body;
}



Node* MeshInstance3D::create_multiple_convex_collisions_node(
	const Ref<MeshConvexDecompositionSettings>& p_settings)
{
	if (mesh.is_null()) {
		return nullptr;
	}

	Ref<MeshConvexDecompositionSettings> settings;
	if (p_settings.is_valid()) {
		settings = p_settings;
	}
	else {
		settings.instantiate();
	}

	Vector<Ref<Shape3D>> shapes = mesh->convex_decompose(settings);
	if (!shapes.size()) {
		return nullptr;
	}

	StaticBody3D* static_body = memnew(StaticBody3D);
	for (int i = 0; i < shapes.size(); i++) {
		CollisionShape3D* cshape = memnew(CollisionShape3D);
		cshape->set_shape(shapes[i]);
		static_body->add_child(cshape, true);
	}
	return static_body;
}


#endif // PHYSICS_3D_DISABLED



int MeshInstance3D::get_surface_override_material_count() const
{
	return surface_override_materials.size();
}

void MeshInstance3D::set_surface_override_material(int p_surface, const Ref<Material>& p_material)
{
	ERR_FAIL_INDEX(p_surface, surface_override_materials.size());

	surface_override_materials.write[p_surface] = p_material;

	if (surface_override_materials[p_surface].is_valid()) {
		RS::get_singleton()->instance_set_surface_override_material(
			get_instance(), p_surface, surface_override_materials[p_surface]->get_rid());
	}
	else {
		RS::get_singleton()->instance_set_surface_override_material(
			get_instance(), p_surface, RID());
	}
}

Ref<Material> MeshInstance3D::get_surface_override_material(int p_surface) const
{
	ERR_FAIL_INDEX_V(p_surface, surface_override_materials.size(), Ref<Material>());

	return surface_override_materials[p_surface];
}

Ref<Material> MeshInstance3D::get_active_material(int p_surface) const
{
	Ref<Material> mat_override = get_material_override();
	if (mat_override.is_valid()) {
		return mat_override;
	}

	Ref<Mesh> m = get_mesh();
	if (m.is_null() || m->get_surface_count() == 0) {
		return Ref<Material>();
	}

	Ref<Material> surface_material = get_surface_override_material(p_surface);
	if (surface_material.is_valid()) {
		return surface_material;
	}

	return m->surface_get_material(p_surface);
}

void MeshInstance3D::_mesh_changed()
{
	ERR_FAIL_COND(mesh.is_null());
	const int surface_count = mesh->get_surface_count();

	surface_override_materials.resize(surface_count);

	uint32_t initialize_bs_from = blend_shape_tracks.size();
	blend_shape_tracks.resize(mesh->get_blend_shape_count());

	if (surface_count > 0) {
		for (uint32_t i = 0; i < blend_shape_tracks.size(); i++) {
			blend_shape_properties["blend_shapes/" + String(mesh->get_blend_shape_name(i))] = i;
			if (i < initialize_bs_from) {
				set_blend_shape_value(i, blend_shape_tracks[i]);
			}
			else {
				set_blend_shape_value(i, 0);
			}
		}
	}

	for (int surface_index = 0; surface_index < surface_count; ++surface_index) {
		if (surface_override_materials[surface_index].is_valid()) {
			RS::get_singleton()->instance_set_surface_override_material(get_instance(),
				surface_index, surface_override_materials[surface_index]->get_rid());
		}
	}

	update_gizmos();
}



void MeshInstance3D::create_debug_tangents()
{
	MeshInstance3D* mi = create_debug_tangents_node();
	if (!mi) {
		return;
	}

	add_child(mi, true);
	if (is_inside_tree() && this == get_tree()->get_edited_scene_root()) {
		mi->set_owner(this);
	}
	else {
		mi->set_owner(get_owner());
	}
}

bool MeshInstance3D::_property_can_revert(const StringName& p_name) const
{
	HashMap<StringName, int>::ConstIterator E = blend_shape_properties.find(p_name);
	if (E) {
		return true;
	}
	return false;
}







Ref<TriangleMesh> MeshInstance3D::generate_triangle_mesh() const
{
	if (mesh.is_valid()) {
		return mesh->generate_triangle_mesh();
	}
	return Ref<TriangleMesh>();
}

PackedStringArray MeshInstance3D::get_configuration_warnings() const
{
	PackedStringArray warnings = GeometryInstance3D::get_configuration_warnings();
	if (mesh.is_null()) {
		warnings.push_back(RTR("MeshInstance3D requires a Mesh to render anything. Please add a "
							   "mesh resource for it!"));
	}
	return warnings;
}




