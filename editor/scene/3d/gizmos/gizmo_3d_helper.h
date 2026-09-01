/**************************************************************************/
/*  gizmo_3d_helper.h                                                     */
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

#include "core/types.h"

class Camera3D;

class Gizmo3DHelper : public RefCounted
{
	Transform3D initial_transform;

private:
	void _cylinder_or_capsule_or_cone_frustum_set_handle(const Vector3 p_segment[2], int p_id,
		real_t& r_height, real_t& r_radius_top, real_t& r_radius_bottom, Vector3& r_position,
		bool p_is_capsule, bool p_is_frustum);
	String _cylinder_or_capsule_or_cone_frustum_get_handle_name(int p_id) const;

public:
	void get_segment(Camera3D* p_camera, const Point2& p_point, Vector3* r_segment);

	// Box

	Vector<Vector3> box_get_handles(const Vector3& p_box_size);
	String box_get_handle_name(int p_id) const;
	void box_set_handle(
		const Vector3 p_segment[2], int p_id, Vector3& r_box_size, Vector3& r_box_position);

	// Cylinder

	Vector<Vector3> cylinder_get_handles(real_t p_height, real_t p_radius);

	_FORCE_INLINE_ String cylinder_get_handle_name(int p_id)
	{
		return _cylinder_or_capsule_or_cone_frustum_get_handle_name(p_id);
	}

	_FORCE_INLINE_ void cylinder_set_handle(const Vector3 p_segment[2], int p_id, real_t& r_height,
		real_t& r_radius, Vector3& r_cylinder_position)
	{
		real_t radius_bottom;
		_cylinder_or_capsule_or_cone_frustum_set_handle(
			p_segment, p_id, r_height, r_radius, radius_bottom, r_cylinder_position, false, false);
	}

	// Capsule

	_FORCE_INLINE_ Vector<Vector3> capsule_get_handles(real_t p_height, real_t p_radius)
	{
		return cylinder_get_handles(p_height, p_radius);
	}

	_FORCE_INLINE_ String capsule_get_handle_name(int p_id)
	{
		return _cylinder_or_capsule_or_cone_frustum_get_handle_name(p_id);
	}

	_FORCE_INLINE_ void capsule_set_handle(const Vector3 p_segment[2], int p_id, real_t& r_height,
		real_t& r_radius, Vector3& r_capsule_position)
	{
		real_t radius_bottom;
		_cylinder_or_capsule_or_cone_frustum_set_handle(
			p_segment, p_id, r_height, r_radius, radius_bottom, r_capsule_position, true, false);
	}

	// Cone frustum

	Vector<Vector3> cone_frustum_get_handles(
		real_t p_height, real_t p_radius_top, real_t p_radius_bottom);

	_FORCE_INLINE_ String cone_frustum_get_handle_name(int p_id)
	{
		return _cylinder_or_capsule_or_cone_frustum_get_handle_name(p_id);
	}

	_FORCE_INLINE_ void cone_frustum_set_handle(const Vector3 p_segment[2], int p_id,
		real_t& r_height, real_t& r_radius_top, real_t& r_radius_bottom,
		Vector3& r_frustum_position)
	{
		_cylinder_or_capsule_or_cone_frustum_set_handle(p_segment, p_id, r_height, r_radius_top,
			r_radius_bottom, r_frustum_position, false, true);
	}
};


