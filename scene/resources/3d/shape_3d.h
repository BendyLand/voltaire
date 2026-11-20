/**************************************************************************/
/*  shape_3d.h                                                            */
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

#include "core/io/resource.h"

class ArrayMesh;
class Image;
class Material;

class Shape3D : public Resource
{
public:
	enum ShapeType
	{
		SHAPE_WORLD_BOUNDARY,
		SHAPE_SEPARATION_RAY,
		SHAPE_SPHERE,
		SHAPE_BOX,
		SHAPE_CAPSULE,
		SHAPE_CYLINDER,
		SHAPE_CONVEX_POLYGON,
		SHAPE_CONCAVE_POLYGON,
		SHAPE_HEIGHTMAP,
	};

private:
	struct DrawEdge
	{
		Vector3 a;
		Vector3 b;

		static uint32_t hash(const DrawEdge& p_edge)
		{
			uint32_t h = hash_murmur3_one_32(HashMapHasherDefault::hash(p_edge.a));
			return hash_murmur3_one_32(HashMapHasherDefault::hash(p_edge.b), h);
		}

		bool operator==(const DrawEdge& p_edge) const { return (a == p_edge.a && b == p_edge.b); }

		DrawEdge(const Vector3& p_a = Vector3(), const Vector3& p_b = Vector3())
		{
			a = p_a;
			b = p_b;
			if (a < b) {
				SWAP(a, b);
			}
		}
	};

	ShapeType shape_type = SHAPE_BOX;
	RID shape;
	real_t custom_bias = 0.0;
	real_t margin = 0.04;

	Ref<ArrayMesh> debug_mesh_cache;

	// Not wrapped in `#ifdef DEBUG_ENABLED` as it is used for rendering.
	Color debug_color = Color(0.0, 0.0, 0.0, 0.0);
	bool debug_fill = true;
#ifdef DEBUG_ENABLED
	bool debug_properties_edited = false;
#endif // DEBUG_ENABLED

	// World boundary parameters
	Plane plane = Plane(0, 1, 0, 0);

	// Separation ray parameters
	float length = 1.0f;
	bool slide_on_slope = false;

	// Sphere / Capsule / Cylinder parameters
	float radius = 0.5f;
	float height = 2.0f;

	// Box parameters
	Vector3 size = Vector3(1, 1, 1);

	// Convex polygon parameters
	Vector<Vector3> points;

	// Concave polygon parameters
	Vector<Vector3> faces;
	bool backface_collision = false;

	// Heightmap parameters
	int map_width = 2;
	int map_depth = 2;
	Vector<real_t> map_data;
	real_t min_height = 0.0;
	real_t max_height = 0.0;

	static RID _create_shape_rid(ShapeType p_type);
	void _ensure_heightmap_initialized();
	void _update_shape();

public:
	ShapeType get_shape_type() const { return shape_type; }
	void set_shape_type(ShapeType p_type);

	virtual RID get_rid() const override { return shape; }

	// World boundary
	void set_plane(const Plane& p_plane);
	const Plane& get_plane() const;

	// Separation ray
	void set_length(float p_length);
	float get_length() const;
	void set_slide_on_slope(bool p_active);
	bool get_slide_on_slope() const;

	// Sphere / Capsule / Cylinder
	void set_radius(float p_radius);
	float get_radius() const;
	void set_height(float p_height);
	float get_height() const;
	void set_mid_height(real_t p_mid_height);
	real_t get_mid_height() const;

	// Box
	void set_size(const Vector3& p_size);
	Vector3 get_size() const;

	// Convex polygon
	void set_points(const Vector<Vector3>& p_points);
	Vector<Vector3> get_points() const;

	// Concave polygon
	void set_faces(const Vector<Vector3>& p_faces);
	Vector<Vector3> get_faces() const;
	void set_backface_collision_enabled(bool p_enabled);
	bool is_backface_collision_enabled() const;

	// Heightmap
	void set_map_width(int p_new);
	int get_map_width() const;
	void set_map_depth(int p_new);
	int get_map_depth() const;
	void set_map_data(Vector<real_t> p_new);
	Vector<real_t> get_map_data() const;
	real_t get_min_height() const;
	real_t get_max_height() const;
	void update_map_data_from_image(
		const Ref<Image>& p_image, real_t p_height_min, real_t p_height_max);

	Ref<ArrayMesh> get_debug_mesh();
	Vector<Vector3> get_debug_mesh_lines() const;
	Ref<ArrayMesh> get_debug_arraymesh_faces(const Color& p_modulate) const;
	/// Returns the radius of a sphere that fully encloses this shape.
	real_t get_enclosing_radius() const;

	void add_vertices_to_array(Vector<Vector3>& array, const Transform3D& p_xform);

	void set_custom_solver_bias(real_t p_bias);
	real_t get_custom_solver_bias() const;

	real_t get_margin() const;
	void set_margin(real_t p_margin);

	void set_debug_color(const Color& p_color);
	Color get_debug_color() const;

	void set_debug_fill(bool p_fill);
	bool get_debug_fill() const;

#ifdef DEBUG_ENABLED
	_FORCE_INLINE_ bool are_debug_properties_edited() const { return debug_properties_edited; }
#endif // DEBUG_ENABLED

	Shape3D(ShapeType p_type = SHAPE_BOX);
	~Shape3D();
};
