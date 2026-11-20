/**************************************************************************/
/*  csg_shape.h                                                           */
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

#include "csg.h"
#include "scene/3d/path_3d.h"
#include "scene/3d/visual_instance_3d.h"
#include "scene/resources/mesh.h"

#ifndef PHYSICS_3D_DISABLED
#include "scene/resources/3d/shape_3d.h"
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
