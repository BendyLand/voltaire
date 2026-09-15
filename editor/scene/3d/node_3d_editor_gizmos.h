/**************************************************************************/
/*  node_3d_editor_gizmos.h                                               */
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

#pragma once

#include "core/math/dynamic_bvh.h"
#include "core/templates/hash_map.h"
#include "scene/3d/camera_3d.h"
#include "scene/3d/node_3d.h"
#include "scene/3d/skeleton_3d.h"
#include "scene/resources/mesh.h"

class Timer;
class EditorNode3DGizmoPlugin;

class EditorNode3DGizmo : public Node3DGizmo
{
	struct Instance
	{
		RID instance;
		Ref<Mesh> mesh;
		Ref<Material> material;
		Ref<SkinReference> skin_reference;
		bool extra_margin = false;
		Transform3D xform;

		void create_instance(Node3D* p_base, bool p_hidden = false);
	};

	bool selected;

	Vector<Vector3> collision_segments;
	LocalVector<Ref<TriangleMesh>> collision_meshes;
	bool collision_meshes_are_snap_source = false;

	Vector<Vector3> handles;
	Vector<int> handle_ids;
	Vector<Vector3> secondary_handles;
	Vector<int> secondary_handle_ids;

	real_t selectable_icon_size;
	bool billboard_handle;

	bool valid;
	bool hidden;
	Vector<Instance> instances;
	Node3D* spatial_node = nullptr;

	DynamicBVH::ID bvh_node_id;

	void _update_bvh();

protected:
	EditorNode3DGizmoPlugin* gizmo_plugin = nullptr;

public:
	void add_mesh(const Ref<Mesh>& p_mesh, const Ref<Material>& p_material = Ref<Material>(),
		const Transform3D& p_xform = Transform3D(),
		const Ref<SkinReference>& p_skin_reference = Ref<SkinReference>());
	void add_collision_segments(const Vector<Vector3>& p_lines);
	void add_collision_triangles(const Ref<TriangleMesh>& p_tmesh);

	void set_collision_meshes_are_snap_source(bool p_enable)
	{
		collision_meshes_are_snap_source = p_enable;
	}

	bool get_collision_meshes_are_snap_source() const { return collision_meshes_are_snap_source; }

	const LocalVector<Ref<TriangleMesh>>& get_collision_meshes() const { return collision_meshes; }

	const Vector<Vector3>& get_collision_segments() const { return collision_segments; }

	virtual bool is_handle_highlighted(int p_id, bool p_secondary) const;
	virtual String get_handle_name(int p_id, bool p_secondary) const;
	virtual void begin_handle_action(int p_id, bool p_secondary);
	virtual void set_handle(int p_id, bool p_secondary, Camera3D* p_camera, const Point2& p_point);

	virtual int subgizmos_intersect_ray(Camera3D* p_camera, const Vector2& p_point) const;
	virtual Transform3D get_subgizmo_transform(int p_id) const;
	virtual void set_subgizmo_transform(int p_id, Transform3D p_transform);

	void set_selected(bool p_selected) { selected = p_selected; }

	bool is_selected() const { return selected; }

	void set_node_3d(Node3D* p_node);

	Node3D* get_node_3d() const { return spatial_node; }

	Ref<EditorNode3DGizmoPlugin> get_plugin() const { return gizmo_plugin; }

	bool intersect_frustum(const Camera3D* p_camera, const Vector<Plane>& p_frustum);
	void handles_intersect_ray(Camera3D* p_camera, const Vector2& p_point, bool p_shift_pressed,
		int& r_id, bool& r_secondary);
	bool intersect_ray(
		Camera3D* p_camera, const Point2& p_point, Vector3& r_pos, Vector3& r_normal);

	virtual void clear() override;
	virtual void create() override;
	virtual void transform() override;
	virtual void free() override;
	virtual void redraw() override;

	virtual bool is_editable() const;

	void set_hidden(bool p_hidden);
	void set_plugin(EditorNode3DGizmoPlugin* p_plugin);

	EditorNode3DGizmo();
	~EditorNode3DGizmo();
};

class EditorNode3DGizmoPlugin : public Resource
{
public:
	static const int VISIBLE = 0;
	static const int HIDDEN = 1;
	static const int ON_TOP = 2;

protected:
	int current_state;
	HashSet<EditorNode3DGizmo*> current_gizmos;
	HashMap<String, Vector<Ref<StandardMaterial3D>>> materials;

	virtual Ref<EditorNode3DGizmo> create_gizmo(Node3D* p_spatial);

public:
	void add_material(const String& p_name, Ref<StandardMaterial3D> p_material);

	virtual bool has_gizmo(Node3D* p_spatial);
	virtual String get_gizmo_name() const;
	virtual int get_priority() const;
	virtual bool can_be_hidden() const;
	virtual bool is_selectable_when_hidden() const;
	virtual bool can_commit_handle_on_click() const;

	virtual void redraw(EditorNode3DGizmo* p_gizmo);
	virtual bool is_handle_highlighted(
		const EditorNode3DGizmo* p_gizmo, int p_id, bool p_secondary) const;
	virtual String get_handle_name(
		const EditorNode3DGizmo* p_gizmo, int p_id, bool p_secondary) const;
	virtual void begin_handle_action(const EditorNode3DGizmo* p_gizmo, int p_id, bool p_secondary);
	virtual void set_handle(const EditorNode3DGizmo* p_gizmo, int p_id, bool p_secondary,
		Camera3D* p_camera, const Point2& p_point);

	virtual int subgizmos_intersect_ray(
		const EditorNode3DGizmo* p_gizmo, Camera3D* p_camera, const Vector2& p_point) const;
	virtual Transform3D get_subgizmo_transform(const EditorNode3DGizmo* p_gizmo, int p_id) const;
	virtual void set_subgizmo_transform(
		const EditorNode3DGizmo* p_gizmo, int p_id, Transform3D p_transform);
	virtual Ref<EditorNode3DGizmo> create_gizmo(Node3D* p_spatial) const;

	void set_state(int p_state);
	int get_state() const;
	void unregister_gizmo(EditorNode3DGizmo* p_gizmo);

	EditorNode3DGizmoPlugin();
	virtual ~EditorNode3DGizmoPlugin() = default;
};


