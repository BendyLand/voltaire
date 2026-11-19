/**************************************************************************/
/*  mesh_instance_3d.h                                                    */
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

#include "core/templates/local_vector.h"
#include "scene/3d/skeleton_3d.h"
#include "scene/3d/visual_instance_3d.h"
#include "scene/resources/environment.h"
#include "scene/resources/mesh.h"

#ifndef NAVIGATION_3D_DISABLED
class NavigationMesh;
class NavigationMeshSourceGeometryData3D;
#endif // NAVIGATION_3D_DISABLED
class Skin;
class SkinReference;

class MeshInstance3D : public Node3D
{
public:
	RenderInstance3D render_instance;

protected:
	Ref<Mesh> mesh;
	Ref<Skin> skin;
	Ref<Skin> skin_internal;
	Ref<SkinReference> skin_ref;
	NodePath skeleton_path;

	LocalVector<float> blend_shape_tracks;
	HashMap<String, int> blend_shape_properties;
	Vector<Ref<Material>> surface_override_materials;

	void _resolve_skeleton_path();

	bool surface_index_0 = false;

	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

	bool _property_can_revert(const String& p_name) const;

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }
	void set_layer_mask(uint32_t p_mask) { render_instance.set_layer_mask(p_mask); }
	uint32_t get_layer_mask() const { return render_instance.get_layer_mask(); }
	void set_cast_shadows_setting(RenderInstance3D::ShadowCastingSetting p_setting)
	{
		render_instance.set_cast_shadows_setting(p_setting);
	}
	RenderInstance3D::ShadowCastingSetting get_cast_shadows_setting() const
	{
		return render_instance.get_cast_shadows_setting();
	}
	void set_material_override(const Ref<Material>& p_material)
	{
		render_instance.set_material_override(p_material);
	}
	Ref<Material> get_material_override() const { return render_instance.get_material_override(); }
	void set_material_overlay(const Ref<Material>& p_material)
	{
		render_instance.set_material_overlay(p_material);
	}
	Ref<Material> get_material_overlay() const { return render_instance.get_material_overlay(); }
	void set_gi_mode(RenderInstance3D::GIMode p_mode) { render_instance.set_gi_mode(p_mode); }
	RenderInstance3D::GIMode get_gi_mode() const { return render_instance.get_gi_mode(); }

	void set_mesh(const Ref<Mesh>& p_mesh);
	Ref<Mesh> get_mesh() const;

	void set_skin(const Ref<Skin>& p_skin);
	Ref<Skin> get_skin() const;

	void set_skeleton_path(const NodePath& p_skeleton);
	NodePath get_skeleton_path();

	Ref<SkinReference> get_skin_reference() const;

	int get_blend_shape_count() const;
	int find_blend_shape_by_name(const String& p_name);
	float get_blend_shape_value(int p_blend_shape) const;
	void set_blend_shape_value(int p_blend_shape, float p_value);

	int get_surface_override_material_count() const;
	void set_surface_override_material(int p_surface, const Ref<Material>& p_material);
	Ref<Material> get_surface_override_material(int p_surface) const;
	Ref<Material> get_active_material(int p_surface) const;

#ifndef PHYSICS_3D_DISABLED
	Node* create_trimesh_collision_node();
	void create_trimesh_collision();

	Node* create_convex_collision_node(bool p_clean = true, bool p_simplify = false);
	void create_convex_collision(bool p_clean = true, bool p_simplify = false);

	Node* create_multiple_convex_collisions_node(
		const Ref<MeshConvexDecompositionSettings>& p_settings =
			Ref<MeshConvexDecompositionSettings>());
	void create_multiple_convex_collisions(const Ref<MeshConvexDecompositionSettings>& p_settings =
											   Ref<MeshConvexDecompositionSettings>());
#endif // PHYSICS_3D_DISABLED

	MeshInstance3D* create_debug_tangents_node();
	void create_debug_tangents();

	AABB get_aabb() const;

	Ref<ArrayMesh> bake_mesh_from_current_blend_shape_mix(
		Ref<ArrayMesh> p_existing = Ref<ArrayMesh>());
	Ref<ArrayMesh> bake_mesh_from_current_skeleton_pose(
		Ref<ArrayMesh> p_existing = Ref<ArrayMesh>());

	Ref<TriangleMesh> generate_triangle_mesh() const;

#ifndef NAVIGATION_3D_DISABLED
private:
	static RID _navmesh_source_geometry_parser;

public:
	static void navmesh_parse_init();
	static void navmesh_parse_source_geometry(const Ref<NavigationMesh>& p_navigation_mesh,
		Ref<NavigationMeshSourceGeometryData3D> p_source_geometry_data, Node* p_node);
#endif // NAVIGATION_3D_DISABLED

	virtual PackedStringArray get_configuration_warnings() const override;

	MeshInstance3D();
};
