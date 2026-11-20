/**************************************************************************/
/*  jolt_shape_3d.h                                                       */
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

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

#include "core/math/aabb.h"
#include "core/math/plane.h"
#include "core/os/mutex.h"
#include "core/templates/hash_map.h"
#include "core/templates/rid.h"
#include "core/types.h"
#include "servers/physics_3d/physics_server_3d_enums.h"

class JoltShapedObject3D;

class JoltShape3D final {
public:
	typedef PS3DE::ShapeType ShapeType;

#ifdef REAL_T_IS_DOUBLE
	typedef PackedFloat64Array HeightArray;
#else
	typedef PackedFloat32Array HeightArray;
#endif

private:
	HashMap<JoltShapedObject3D *, int> ref_counts_by_owner;
	Mutex jolt_ref_mutex;
	RID rid;
	JPH::ShapeRefC jolt_ref;
	ShapeType shape_type = ShapeType::SHAPE_CUSTOM;

	AABB aabb;
	Plane plane;
	Vector3 half_extents;
	PackedVector3Array vertices;
	HeightArray heights;

	float margin = 0.04f;
	float radius = 0.0f;
	float height = 0.0f;
	float length = 0.0f;

	int width = 0;
	int depth = 0;

	bool slide_on_slope = false;
	bool back_face_collision = false;

	JPH::ShapeRefC _build() const;
	JPH::ShapeRefC _build_separation_ray() const;
	JPH::ShapeRefC _build_sphere() const;
	JPH::ShapeRefC _build_capsule() const;
	JPH::ShapeRefC _build_height_map() const;
	JPH::ShapeRefC _build_height_field() const;
	JPH::ShapeRefC _build_mesh() const;

	AABB _calculate_convex_aabb() const;
	AABB _calculate_heightmap_aabb() const;

	String _owners_to_string() const;

public:
	JoltShape3D() = default;
	explicit JoltShape3D(ShapeType p_type) :
			shape_type(p_type) {}
	~JoltShape3D() = default;

	RID get_rid() const { return rid; }
	void set_rid(const RID &p_rid) { rid = p_rid; }

	void add_owner(JoltShapedObject3D *p_owner);
	void remove_owner(JoltShapedObject3D *p_owner);
	void remove_self();

	ShapeType get_type() const { return shape_type; }
	bool is_convex() const;

	float get_margin() const;
	void set_margin(float p_margin);

	AABB get_aabb() const;

	float get_solver_bias() const;
	void set_solver_bias(float p_bias);

	String to_string() const;

	// Typed shape parameter getters/setters
	const Plane &get_plane() const { return plane; }
	void set_plane(const Plane &p_plane);

	float get_ray_length() const { return length; }
	void set_ray_length(float p_length);

	bool get_slide_on_slope() const { return slide_on_slope; }
	void set_slide_on_slope(bool p_slide_on_slope);

	float get_radius() const { return radius; }
	void set_radius(float p_radius);

	float get_height() const { return height; }
	void set_height(float p_height);

	const Vector3 &get_half_extents() const { return half_extents; }
	void set_half_extents(const Vector3 &p_half_extents);

	const PackedVector3Array &get_vertices() const { return vertices; }
	void set_vertices(const PackedVector3Array &p_vertices);

	bool get_back_face_collision() const { return back_face_collision; }
	void set_back_face_collision(bool p_back_face_collision);
	void set_concave_aabb(const AABB &p_aabb);

	const HeightArray &get_heights() const { return heights; }
	int get_width() const { return width; }
	int get_depth() const { return depth; }
	void set_heightmap(const HeightArray &p_heights, int p_width, int p_depth);

	JPH::ShapeRefC try_build();

	void destroy();

	const JPH::Shape *get_jolt_ref() const { return jolt_ref; }

	static JPH::ShapeRefC with_scale(const JPH::Shape *p_shape, const Vector3 &p_scale);
	static JPH::ShapeRefC with_basis_origin(const JPH::Shape *p_shape, const Basis &p_basis, const Vector3 &p_origin);
	static JPH::ShapeRefC with_center_of_mass_offset(const JPH::Shape *p_shape, const Vector3 &p_offset);
	static JPH::ShapeRefC with_center_of_mass(const JPH::Shape *p_shape, const Vector3 &p_center_of_mass);
	static JPH::ShapeRefC with_user_data(const JPH::Shape *p_shape, uint64_t p_user_data);
	static JPH::ShapeRefC with_double_sided(const JPH::Shape *p_shape, bool p_back_face_collision);
	static JPH::ShapeRefC without_custom_shapes(const JPH::Shape *p_shape);

	static Vector3 make_scale_valid(const JPH::Shape *p_shape, const Vector3 &p_scale);
	static bool is_scale_valid(const Vector3 &p_scale, const Vector3 &p_valid_scale, real_t p_tolerance = 0.01f);
};

#ifdef DEBUG_ENABLED

#define JOLT_ENSURE_SCALE_NOT_ZERO(m_transform, m_msg)                                             \
	if (unlikely((m_transform).basis.determinant() == 0.0f)) {                                     \
		WARN_PRINT(vformat(                                                                        \
			"%s "                                                                                  \
			"The basis of the transform was singular, which is not supported by Jolt Physics. "    \
			"This is likely caused by one or more axes having a scale of zero. "                   \
			"The basis (and thus its scale) will be treated as identity.",                         \
			m_msg));                                                                               \
                                                                                                   \
		(m_transform).basis = Basis();                                                             \
	} else                                                                                         \
		((void)0)

#define ERR_PRINT_INVALID_SCALE_MSG(m_scale, m_valid_scale, m_msg)                                 \
	if (unlikely(!JoltShape3D::is_scale_valid(m_scale, valid_scale))) {                            \
		ERR_PRINT(vformat("%s "                                                                    \
						  "A scale of %v is not supported by Jolt Physics for this shape/body. "   \
						  "The scale will instead be treated as %v.",                              \
			m_msg, m_scale, valid_scale));                                                         \
	} else                                                                                         \
		((void)0)

#else

#define JOLT_ENSURE_SCALE_NOT_ZERO(m_transform, m_msg)

#define ERR_PRINT_INVALID_SCALE_MSG(m_scale, m_valid_scale, m_msg)

#endif

#define JOLT_ENSURE_SCALE_VALID(m_shape, m_scale, m_msg)                                           \
	if (true) {                                                                                    \
		const Vector3 valid_scale = JoltShape3D::make_scale_valid(m_shape, m_scale);               \
		ERR_PRINT_INVALID_SCALE_MSG(m_scale, valid_scale, m_msg);                                  \
		(m_scale) = valid_scale;                                                                   \
	} else                                                                                         \
		((void)0)
