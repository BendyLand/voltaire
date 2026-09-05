/**************************************************************************/
/*  gizmo_3d_helper.cpp                                                   */
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

#include "core/input/input.h"
#include "core/math/geometry_3d.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/scene/3d/node_3d_editor_plugin.h"
#include "gizmo_3d_helper.h"
#include "scene/3d/camera_3d.h"

void Gizmo3DHelper::get_segment(Camera3D* p_camera, const Point2& p_point, Vector3* r_segment)
{
	Transform3D gt = initial_transform;
	Transform3D gi = gt.affine_inverse();

	Vector3 ray_from = p_camera->project_ray_origin(p_point);
	Vector3 ray_dir = p_camera->project_ray_normal(p_point);

	r_segment[0] = gi.xform(ray_from);
	r_segment[1] = gi.xform(ray_from + ray_dir * 4096);
}

Vector<Vector3> Gizmo3DHelper::box_get_handles(const Vector3& p_box_size)
{
	Vector<Vector3> handles;
	for (int i = 0; i < 3; i++) {
		Vector3 ax;
		ax[i] = p_box_size[i] / 2;
		handles.push_back(ax);
		handles.push_back(-ax);
	}
	return handles;
}

String Gizmo3DHelper::box_get_handle_name(int p_id) const
{
	switch (p_id) {
	case 0:
	case 1:
		return "Size X";
	case 2:
	case 3:
		return "Size Y";
	case 4:
	case 5:
		return "Size Z";
	}
	return "";
}

Vector<Vector3> Gizmo3DHelper::cylinder_get_handles(real_t p_height, real_t p_radius)
{
	Vector<Vector3> handles;
	handles.push_back(Vector3(0, p_height * 0.5, 0));
	handles.push_back(Vector3(0, p_height * -0.5, 0));
	handles.push_back(Vector3(p_radius, 0, 0));
	return handles;
}

String Gizmo3DHelper::_cylinder_or_capsule_or_cone_frustum_get_handle_name(int p_id) const
{
	if (p_id < 2) {
		return "Height";
	}
	else {
		return "Radius";
	}
}

Vector<Vector3> Gizmo3DHelper::cone_frustum_get_handles(
	real_t p_height, real_t p_radius_top, real_t p_radius_bottom)
{
	Vector<Vector3> handles;
	handles.push_back(Vector3(0, p_height * 0.5, 0));
	handles.push_back(Vector3(0, p_height * -0.5, 0));
	handles.push_back(Vector3((p_radius_top + p_radius_bottom) / 2.0, 0, 0));
	return handles;
}


