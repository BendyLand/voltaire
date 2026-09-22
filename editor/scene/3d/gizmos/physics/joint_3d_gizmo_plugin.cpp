/**************************************************************************/
/*  joint_3d_gizmo_plugin.cpp                                             */
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

#include "editor/editor_node.h"
#include "editor/settings/editor_settings.h"
#include "joint_3d_gizmo_plugin.h"
#include "scene/3d/physics/joints/cone_twist_joint_3d.h"
#include "scene/3d/physics/joints/generic_6dof_joint_3d.h"
#include "scene/3d/physics/joints/hinge_joint_3d.h"
#include "scene/3d/physics/joints/pin_joint_3d.h"
#include "scene/3d/physics/joints/slider_joint_3d.h"
#include "scene/main/timer.h"

#define BODY_A_RADIUS 0.25
#define BODY_B_RADIUS 0.27

Basis JointGizmosDrawer::look_body(
	const Transform3D& p_joint_transform, const Transform3D& p_body_transform)
{
	const Vector3& p_eye(p_joint_transform.origin);
	const Vector3& p_target(p_body_transform.origin);

	Vector3 v_x, v_y, v_z;

	// Look the body with X
	v_x = p_target - p_eye;
	v_x.normalize();

	v_z = v_x.cross(Vector3(0, 1, 0));
	v_z.normalize();

	v_y = v_z.cross(v_x);
	v_y.normalize();

	Basis base;
	base.set_columns(v_x, v_y, v_z);

	// Absorb current joint transform
	base = p_joint_transform.basis.inverse() * base;

	return base;
}

Basis JointGizmosDrawer::look_body_toward(
	Vector3::Axis p_axis, const Transform3D& joint_transform, const Transform3D& body_transform)
{
	switch (p_axis) {
	case Vector3::AXIS_X:
		return look_body_toward_x(joint_transform, body_transform);
	case Vector3::AXIS_Y:
		return look_body_toward_y(joint_transform, body_transform);
	case Vector3::AXIS_Z:
		return look_body_toward_z(joint_transform, body_transform);
	default:
		return Basis();
	}
}

Basis JointGizmosDrawer::look_body_toward_x(
	const Transform3D& p_joint_transform, const Transform3D& p_body_transform)
{
	const Vector3& p_eye(p_joint_transform.origin);
	const Vector3& p_target(p_body_transform.origin);

	const Vector3 p_front(p_joint_transform.basis.get_column(0));

	Vector3 v_x, v_y, v_z;

	// Look the body with X
	v_x = p_target - p_eye;
	v_x.normalize();

	v_y = p_front.cross(v_x);
	v_y.normalize();

	v_z = v_y.cross(p_front);
	v_z.normalize();

	// Clamp X to FRONT axis
	v_x = p_front;
	v_x.normalize();

	Basis base;
	base.set_columns(v_x, v_y, v_z);

	// Absorb current joint transform
	base = p_joint_transform.basis.inverse() * base;

	return base;
}

Basis JointGizmosDrawer::look_body_toward_y(
	const Transform3D& p_joint_transform, const Transform3D& p_body_transform)
{
	const Vector3& p_eye(p_joint_transform.origin);
	const Vector3& p_target(p_body_transform.origin);

	const Vector3 p_up(p_joint_transform.basis.get_column(1));

	Vector3 v_x, v_y, v_z;

	// Look the body with X
	v_x = p_target - p_eye;
	v_x.normalize();

	v_z = v_x.cross(p_up);
	v_z.normalize();

	v_x = p_up.cross(v_z);
	v_x.normalize();

	// Clamp Y to UP axis
	v_y = p_up;
	v_y.normalize();

	Basis base;
	base.set_columns(v_x, v_y, v_z);

	// Absorb current joint transform
	base = p_joint_transform.basis.inverse() * base;

	return base;
}

Basis JointGizmosDrawer::look_body_toward_z(
	const Transform3D& p_joint_transform, const Transform3D& p_body_transform)
{
	const Vector3& p_eye(p_joint_transform.origin);
	const Vector3& p_target(p_body_transform.origin);

	const Vector3 p_lateral(p_joint_transform.basis.get_column(2));

	Vector3 v_x, v_y, v_z;

	// Look the body with X
	v_x = p_target - p_eye;
	v_x.normalize();

	v_z = p_lateral;
	v_z.normalize();

	v_y = v_z.cross(v_x);
	v_y.normalize();

	// Clamp X to Z axis
	v_x = v_y.cross(v_z);
	v_x.normalize();

	Basis base;
	base.set_columns(v_x, v_y, v_z);

	// Absorb current joint transform
	base = p_joint_transform.basis.inverse() * base;

	return base;
}

void JointGizmosDrawer::draw_circle(Vector3::Axis p_axis, real_t p_radius,
	const Transform3D& p_offset, const Basis& p_base, real_t p_limit_lower, real_t p_limit_upper,
	Vector<Vector3>& r_points, bool p_inverse)
{
	if (p_limit_lower == p_limit_upper) {
		r_points.push_back(p_offset.translated_local(Vector3()).origin);
		r_points.push_back(p_offset.translated_local(p_base.xform(Vector3(0.5, 0, 0))).origin);

	}
	else {
		if (p_limit_lower > p_limit_upper) {
			p_limit_lower = -Math::PI;
			p_limit_upper = Math::PI;
		}

		const int points = 32;

		for (int i = 0; i < points; i++) {
			real_t s = p_limit_lower + i * (p_limit_upper - p_limit_lower) / points;
			real_t n = p_limit_lower + (i + 1) * (p_limit_upper - p_limit_lower) / points;

			Vector3 from;
			Vector3 to;
			switch (p_axis) {
			case Vector3::AXIS_X:
				if (p_inverse) {
					from = p_base.xform(Vector3(0, Math::sin(s), Math::cos(s))) * p_radius;
					to = p_base.xform(Vector3(0, Math::sin(n), Math::cos(n))) * p_radius;
				}
				else {
					from = p_base.xform(Vector3(0, -Math::sin(s), Math::cos(s))) * p_radius;
					to = p_base.xform(Vector3(0, -Math::sin(n), Math::cos(n))) * p_radius;
				}
				break;
			case Vector3::AXIS_Y:
				if (p_inverse) {
					from = p_base.xform(Vector3(Math::cos(s), 0, -Math::sin(s))) * p_radius;
					to = p_base.xform(Vector3(Math::cos(n), 0, -Math::sin(n))) * p_radius;
				}
				else {
					from = p_base.xform(Vector3(Math::cos(s), 0, Math::sin(s))) * p_radius;
					to = p_base.xform(Vector3(Math::cos(n), 0, Math::sin(n))) * p_radius;
				}
				break;
			case Vector3::AXIS_Z:
				from = p_base.xform(Vector3(Math::cos(s), Math::sin(s), 0)) * p_radius;
				to = p_base.xform(Vector3(Math::cos(n), Math::sin(n), 0)) * p_radius;
				break;
			}

			if (i == points - 1) {
				r_points.push_back(p_offset.translated_local(to).origin);
				r_points.push_back(p_offset.translated_local(Vector3()).origin);
			}
			if (i == 0) {
				r_points.push_back(p_offset.translated_local(from).origin);
				r_points.push_back(p_offset.translated_local(Vector3()).origin);
			}

			r_points.push_back(p_offset.translated_local(from).origin);
			r_points.push_back(p_offset.translated_local(to).origin);
		}

		r_points.push_back(p_offset.translated_local(Vector3(0, p_radius * 1.5, 0)).origin);
		r_points.push_back(p_offset.translated_local(Vector3()).origin);
	}
}

void JointGizmosDrawer::draw_cone(const Transform3D& p_offset, const Basis& p_base, real_t p_swing,
	real_t p_twist, Vector<Vector3>& r_points)
{
	float r = 1.0;
	float w = r * Math::sin(p_swing);
	float d = r * Math::cos(p_swing);

	// swing
	for (int i = 0; i < 360; i += 10) {
		float ra = Math::deg_to_rad((float)i);
		float rb = Math::deg_to_rad((float)i + 10);
		Point2 a = Vector2(Math::sin(ra), Math::cos(ra)) * w;
		Point2 b = Vector2(Math::sin(rb), Math::cos(rb)) * w;

		r_points.push_back(p_offset.translated_local(p_base.xform(Vector3(d, a.x, a.y))).origin);
		r_points.push_back(p_offset.translated_local(p_base.xform(Vector3(d, b.x, b.y))).origin);

		if (i % 90 == 0) {
			r_points.push_back(
				p_offset.translated_local(p_base.xform(Vector3(d, a.x, a.y))).origin);
			r_points.push_back(p_offset.translated_local(p_base.xform(Vector3())).origin);
		}
	}

	r_points.push_back(p_offset.translated_local(p_base.xform(Vector3())).origin);
	r_points.push_back(p_offset.translated_local(p_base.xform(Vector3(1, 0, 0))).origin);

	/// Twist
	float ts = Math::rad_to_deg(p_twist);
	ts = MIN(ts, 720);

	for (int i = 0; i < int(ts); i += 5) {
		float ra = Math::deg_to_rad((float)i);
		float rb = Math::deg_to_rad((float)i + 5);
		float c = i / 720.0;
		float cn = (i + 5) / 720.0;
		Point2 a = Vector2(Math::sin(ra), Math::cos(ra)) * w * c;
		Point2 b = Vector2(Math::sin(rb), Math::cos(rb)) * w * cn;

		r_points.push_back(p_offset.translated_local(p_base.xform(Vector3(c, a.x, a.y))).origin);
		r_points.push_back(p_offset.translated_local(p_base.xform(Vector3(cn, b.x, b.y))).origin);
	}
}

////

void Joint3DGizmoPlugin::incremental_update_gizmos()
{
	if (!current_gizmos.is_empty()) {
		HashSet<EditorNode3DGizmo*>::Iterator E = current_gizmos.find(last_drawn);
		if (E) {
			++E;
		}
		if (!E) {
			E = current_gizmos.begin();
		}
		redraw(*E);
		last_drawn = *E;
	}
}

String Joint3DGizmoPlugin::get_gizmo_name() const { return "Joint3D"; }

int Joint3DGizmoPlugin::get_priority() const { return -1; }

void Joint3DGizmoPlugin::CreatePinJointGizmo(
	const Transform3D& p_offset, Vector<Vector3>& r_cursor_points)
{
	float cs = 0.25;

	r_cursor_points.push_back(p_offset.translated_local(Vector3(+cs, 0, 0)).origin);
	r_cursor_points.push_back(p_offset.translated_local(Vector3(-cs, 0, 0)).origin);
	r_cursor_points.push_back(p_offset.translated_local(Vector3(0, +cs, 0)).origin);
	r_cursor_points.push_back(p_offset.translated_local(Vector3(0, -cs, 0)).origin);
	r_cursor_points.push_back(p_offset.translated_local(Vector3(0, 0, +cs)).origin);
	r_cursor_points.push_back(p_offset.translated_local(Vector3(0, 0, -cs)).origin);
}

void Joint3DGizmoPlugin::CreateHingeJointGizmo(const Transform3D& p_offset,
	const Transform3D& p_trs_joint, const Transform3D& p_trs_body_a,
	const Transform3D& p_trs_body_b, real_t p_limit_lower, real_t p_limit_upper, bool p_use_limit,
	Vector<Vector3>& r_common_points, Vector<Vector3>* r_body_a_points,
	Vector<Vector3>* r_body_b_points)
{
	r_common_points.push_back(p_offset.translated_local(Vector3(0, 0, 0.5)).origin);
	r_common_points.push_back(p_offset.translated_local(Vector3(0, 0, -0.5)).origin);

	if (!p_use_limit) {
		p_limit_upper = -1;
		p_limit_lower = 0;
	}

	if (r_body_a_points) {
		JointGizmosDrawer::draw_circle(Vector3::AXIS_Z, BODY_A_RADIUS, p_offset,
			JointGizmosDrawer::look_body_toward_z(p_trs_joint, p_trs_body_a), p_limit_lower,
			p_limit_upper, *r_body_a_points);
	}

	if (r_body_b_points) {
		JointGizmosDrawer::draw_circle(Vector3::AXIS_Z, BODY_B_RADIUS, p_offset,
			JointGizmosDrawer::look_body_toward_z(p_trs_joint, p_trs_body_b), p_limit_lower,
			p_limit_upper, *r_body_b_points);
	}
}

void Joint3DGizmoPlugin::CreateSliderJointGizmo(const Transform3D& p_offset,
	const Transform3D& p_trs_joint, const Transform3D& p_trs_body_a,
	const Transform3D& p_trs_body_b, real_t p_angular_limit_lower, real_t p_angular_limit_upper,
	real_t p_linear_limit_lower, real_t p_linear_limit_upper, Vector<Vector3>& r_points,
	Vector<Vector3>* r_body_a_points, Vector<Vector3>* r_body_b_points)
{
	p_linear_limit_lower = -p_linear_limit_lower;
	p_linear_limit_upper = -p_linear_limit_upper;

	float cs = 0.25;
	r_points.push_back(p_offset.translated_local(Vector3(0, 0, 0.5)).origin);
	r_points.push_back(p_offset.translated_local(Vector3(0, 0, -0.5)).origin);

	if (p_linear_limit_lower >= p_linear_limit_upper) {
		r_points.push_back(p_offset.translated_local(Vector3(p_linear_limit_upper, 0, 0)).origin);
		r_points.push_back(p_offset.translated_local(Vector3(p_linear_limit_lower, 0, 0)).origin);

		r_points.push_back(
			p_offset.translated_local(Vector3(p_linear_limit_upper, -cs, -cs)).origin);
		r_points.push_back(
			p_offset.translated_local(Vector3(p_linear_limit_upper, -cs, cs)).origin);
		r_points.push_back(
			p_offset.translated_local(Vector3(p_linear_limit_upper, -cs, cs)).origin);
		r_points.push_back(p_offset.translated_local(Vector3(p_linear_limit_upper, cs, cs)).origin);
		r_points.push_back(p_offset.translated_local(Vector3(p_linear_limit_upper, cs, cs)).origin);
		r_points.push_back(
			p_offset.translated_local(Vector3(p_linear_limit_upper, cs, -cs)).origin);
		r_points.push_back(
			p_offset.translated_local(Vector3(p_linear_limit_upper, cs, -cs)).origin);
		r_points.push_back(
			p_offset.translated_local(Vector3(p_linear_limit_upper, -cs, -cs)).origin);

		r_points.push_back(
			p_offset.translated_local(Vector3(p_linear_limit_lower, -cs, -cs)).origin);
		r_points.push_back(
			p_offset.translated_local(Vector3(p_linear_limit_lower, -cs, cs)).origin);
		r_points.push_back(
			p_offset.translated_local(Vector3(p_linear_limit_lower, -cs, cs)).origin);
		r_points.push_back(p_offset.translated_local(Vector3(p_linear_limit_lower, cs, cs)).origin);
		r_points.push_back(p_offset.translated_local(Vector3(p_linear_limit_lower, cs, cs)).origin);
		r_points.push_back(
			p_offset.translated_local(Vector3(p_linear_limit_lower, cs, -cs)).origin);
		r_points.push_back(
			p_offset.translated_local(Vector3(p_linear_limit_lower, cs, -cs)).origin);
		r_points.push_back(
			p_offset.translated_local(Vector3(p_linear_limit_lower, -cs, -cs)).origin);

	}
	else {
		r_points.push_back(p_offset.translated_local(Vector3(+cs * 2, 0, 0)).origin);
		r_points.push_back(p_offset.translated_local(Vector3(-cs * 2, 0, 0)).origin);
	}

	if (r_body_a_points) {
		JointGizmosDrawer::draw_circle(Vector3::AXIS_X, BODY_A_RADIUS, p_offset,
			JointGizmosDrawer::look_body_toward(Vector3::AXIS_X, p_trs_joint, p_trs_body_a),
			p_angular_limit_lower, p_angular_limit_upper, *r_body_a_points);
	}

	if (r_body_b_points) {
		JointGizmosDrawer::draw_circle(Vector3::AXIS_X, BODY_B_RADIUS, p_offset,
			JointGizmosDrawer::look_body_toward(Vector3::AXIS_X, p_trs_joint, p_trs_body_b),
			p_angular_limit_lower, p_angular_limit_upper, *r_body_b_points, true);
	}
}

void Joint3DGizmoPlugin::CreateConeTwistJointGizmo(const Transform3D& p_offset,
	const Transform3D& p_trs_joint, const Transform3D& p_trs_body_a,
	const Transform3D& p_trs_body_b, real_t p_swing, real_t p_twist,
	Vector<Vector3>* r_body_a_points, Vector<Vector3>* r_body_b_points)
{
	if (r_body_a_points) {
		JointGizmosDrawer::draw_cone(p_offset,
			JointGizmosDrawer::look_body(p_trs_joint, p_trs_body_a), p_swing, p_twist,
			*r_body_a_points);
	}

	if (r_body_b_points) {
		JointGizmosDrawer::draw_cone(p_offset,
			JointGizmosDrawer::look_body(p_trs_joint, p_trs_body_b), p_swing, p_twist,
			*r_body_b_points);
	}
}

void Joint3DGizmoPlugin::CreateGeneric6DOFJointGizmo(const Transform3D& p_offset,
	const Transform3D& p_trs_joint, const Transform3D& p_trs_body_a,
	const Transform3D& p_trs_body_b, real_t p_angular_limit_lower_x, real_t p_angular_limit_upper_x,
	real_t p_linear_limit_lower_x, real_t p_linear_limit_upper_x, bool p_enable_angular_limit_x,
	bool p_enable_linear_limit_x, real_t p_angular_limit_lower_y, real_t p_angular_limit_upper_y,
	real_t p_linear_limit_lower_y, real_t p_linear_limit_upper_y, bool p_enable_angular_limit_y,
	bool p_enable_linear_limit_y, real_t p_angular_limit_lower_z, real_t p_angular_limit_upper_z,
	real_t p_linear_limit_lower_z, real_t p_linear_limit_upper_z, bool p_enable_angular_limit_z,
	bool p_enable_linear_limit_z, Vector<Vector3>& r_points, Vector<Vector3>* r_body_a_points,
	Vector<Vector3>* r_body_b_points)
{
	float cs = 0.25;

	for (int ax = 0; ax < 3; ax++) {
		float ll = 0;
		float ul = 0;
		float lll = 0;
		float lul = 0;

		int a1 = 0;
		int a2 = 0;
		int a3 = 0;
		bool enable_ang = false;
		bool enable_lin = false;

		switch (ax) {
		case 0:
			ll = p_angular_limit_lower_x;
			ul = p_angular_limit_upper_x;
			lll = -p_linear_limit_lower_x;
			lul = -p_linear_limit_upper_x;
			enable_ang = p_enable_angular_limit_x;
			enable_lin = p_enable_linear_limit_x;
			a1 = 0;
			a2 = 1;
			a3 = 2;
			break;
		case 1:
			ll = p_angular_limit_lower_y;
			ul = p_angular_limit_upper_y;
			lll = -p_linear_limit_lower_y;
			lul = -p_linear_limit_upper_y;
			enable_ang = p_enable_angular_limit_y;
			enable_lin = p_enable_linear_limit_y;
			a1 = 1;
			a2 = 2;
			a3 = 0;
			break;
		case 2:
			ll = p_angular_limit_lower_z;
			ul = p_angular_limit_upper_z;
			lll = -p_linear_limit_lower_z;
			lul = -p_linear_limit_upper_z;
			enable_ang = p_enable_angular_limit_z;
			enable_lin = p_enable_linear_limit_z;
			a1 = 2;
			a2 = 0;
			a3 = 1;
			break;
		}

#define ADD_VTX(x, y, z)                                                                           \
	{                                                                                              \
		Vector3 v;                                                                                 \
		v[a1] = (x);                                                                               \
		v[a2] = (y);                                                                               \
		v[a3] = (z);                                                                               \
		r_points.push_back(p_offset.translated_local(v).origin);                                   \
	}

		if (enable_lin && lll >= lul) {
			ADD_VTX(lul, 0, 0);
			ADD_VTX(lll, 0, 0);

			ADD_VTX(lul, -cs, -cs);
			ADD_VTX(lul, -cs, cs);
			ADD_VTX(lul, -cs, cs);
			ADD_VTX(lul, cs, cs);
			ADD_VTX(lul, cs, cs);
			ADD_VTX(lul, cs, -cs);
			ADD_VTX(lul, cs, -cs);
			ADD_VTX(lul, -cs, -cs);

			ADD_VTX(lll, -cs, -cs);
			ADD_VTX(lll, -cs, cs);
			ADD_VTX(lll, -cs, cs);
			ADD_VTX(lll, cs, cs);
			ADD_VTX(lll, cs, cs);
			ADD_VTX(lll, cs, -cs);
			ADD_VTX(lll, cs, -cs);
			ADD_VTX(lll, -cs, -cs);

		}
		else {
			ADD_VTX(+cs * 2, 0, 0);
			ADD_VTX(-cs * 2, 0, 0);
		}

		if (!enable_ang) {
			ll = 0;
			ul = -1;
		}

		if (r_body_a_points) {
			JointGizmosDrawer::draw_circle(static_cast<Vector3::Axis>(ax), BODY_A_RADIUS, p_offset,
				JointGizmosDrawer::look_body_toward(
					static_cast<Vector3::Axis>(ax), p_trs_joint, p_trs_body_a),
				ll, ul, *r_body_a_points, true);
		}

		if (r_body_b_points) {
			JointGizmosDrawer::draw_circle(static_cast<Vector3::Axis>(ax), BODY_B_RADIUS, p_offset,
				JointGizmosDrawer::look_body_toward(
					static_cast<Vector3::Axis>(ax), p_trs_joint, p_trs_body_b),
				ll, ul, *r_body_b_points);
		}
	}

#undef ADD_VTX
}


