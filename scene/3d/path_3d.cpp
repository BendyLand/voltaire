/**************************************************************************/
/*  path_3d.cpp                                                           */
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

#include "core/config/engine.h"
#include "path_3d.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/mesh.h"
#include "servers/rendering/rendering_server.h"

Path3D::Path3D()
{
	SceneTree* st = SceneTree::get_singleton();
	if (st && st->is_debugging_paths_hint()) {
		debug_instance = RS::get_singleton()->instance_create();
		set_notify_transform(true);
		_update_debug_mesh();
	}
}

Path3D::~Path3D()
{
	if (debug_instance.is_valid()) {
		ERR_FAIL_NULL(RenderingServer::get_singleton());
		RS::get_singleton()->free_rid(debug_instance);
	}
	if (debug_mesh.is_valid()) {
		ERR_FAIL_NULL(RenderingServer::get_singleton());
		RS::get_singleton()->free_rid(debug_mesh->get_rid());
	}
}

void Path3D::set_debug_custom_color(const Color& p_color)
{
	debug_custom_color = p_color;
	_update_debug_path_material();
}

Ref<StandardMaterial3D> Path3D::get_debug_material() { return debug_material; }

const Color& Path3D::get_debug_custom_color() const { return debug_custom_color; }

Ref<Curve3D> Path3D::get_curve() const { return curve; }

void PathFollow3D::update_transform()
{
	if (!path) {
		return;
	}
	Ref<Curve3D> c = path->get_curve();
	if (c.is_null()) {
		return;
	}
	real_t bl = c->get_baked_length();
	if (bl == 0.0) {
		return;
	}
	Transform3D t;
	if (rotation_mode == ROTATION_NONE) {
		Vector3 pos = c->sample_baked(progress, cubic);
		t.origin = pos;
	}
	else {
		t = c->sample_baked_with_rotation(progress, cubic, false);
		Vector3 tangent = -t.basis.get_column(2); // Retain tangent for applying tilt.
		t = PathFollow3D::correct_posture(t, rotation_mode);

		// Switch Z+ and Z- if necessary.
		if (use_model_front) {
			t.basis *= Basis::from_scale(Vector3(-1.0, 1.0, -1.0));
		}

		// Apply tilt *after* correct_posture().
		if (tilt_enabled) {
			const real_t tilt = c->sample_baked_tilt(progress);

			const Basis twist(tangent, tilt);
			t.basis = twist * t.basis;
		}
	}

	// Apply offset and scale.
	Vector3 scale = get_transform().basis.get_scale();
	t.translate_local(Vector3(h_offset, v_offset, 0));
	t.basis.scale_local(scale);

	set_transform(t);
}

void PathFollow3D::set_cubic_interpolation_enabled(bool p_enabled) { cubic = p_enabled; }

bool PathFollow3D::is_cubic_interpolation_enabled() const { return cubic; }

Transform3D PathFollow3D::correct_posture(
	Transform3D p_transform, PathFollow3D::RotationMode p_rotation_mode)
{
	Transform3D t = p_transform;

	// Modify frame according to rotation mode.
	if (p_rotation_mode == PathFollow3D::ROTATION_NONE) {
		// Clear rotation.
		t.basis = Basis();
	}
	else if (p_rotation_mode == PathFollow3D::ROTATION_ORIENTED) {
		Vector3 tangent = -t.basis.get_column(2);

		// Y-axis points up by default.
		t.basis = Basis::looking_at(tangent);
	}
	else {
		// Lock some euler axes.
		Vector3 euler = t.basis.get_euler_normalized(EulerOrder::YXZ);
		if (p_rotation_mode == PathFollow3D::ROTATION_Y) {
			// Only Y-axis allowed.
			euler[0] = 0;
			euler[2] = 0;
		}
		else if (p_rotation_mode == PathFollow3D::ROTATION_XY) {
			// XY allowed.
			euler[2] = 0;
		}

		Basis locked = Basis::from_euler(euler, EulerOrder::YXZ);
		t.basis = locked;
	}

	return t;
}

void PathFollow3D::_bind_methods() {}

void PathFollow3D::set_progress(real_t p_progress)
{
	ERR_FAIL_COND(!std::isfinite(p_progress));
	if (progress == p_progress) {
		return;
	}
	progress = p_progress;

	if (path) {
		if (path->get_curve().is_valid()) {
			real_t path_length = path->get_curve()->get_baked_length();

			if (loop && path_length) {
				progress = Math::fposmod(progress, path_length);
				if (!Math::is_zero_approx(p_progress) && Math::is_zero_approx(progress)) {
					progress = path_length;
				}
			}
			else {
				progress = CLAMP(progress, 0, path_length);
			}
		}

		update_transform();
	}
}

void PathFollow3D::set_h_offset(real_t p_h_offset)
{
	if (h_offset == p_h_offset) {
		return;
	}
	h_offset = p_h_offset;
	update_transform();
}

real_t PathFollow3D::get_h_offset() const { return h_offset; }

void PathFollow3D::set_v_offset(real_t p_v_offset)
{
	if (v_offset == p_v_offset) {
		return;
	}
	v_offset = p_v_offset;
	update_transform();
}

real_t PathFollow3D::get_v_offset() const { return v_offset; }

real_t PathFollow3D::get_progress() const { return progress; }

void PathFollow3D::set_progress_ratio(real_t p_ratio)
{
	ERR_FAIL_NULL_MSG(path, "Can only set progress ratio on a PathFollow3D that is the child of a "
							"Path3D which is itself part of the scene tree.");
	ERR_FAIL_COND_MSG(path->get_curve().is_null(),
		"Can't set progress ratio on a PathFollow3D that does not have a Curve.");
	ERR_FAIL_COND_MSG(!path->get_curve()->get_baked_length(),
		"Can't set progress ratio on a PathFollow3D that has a 0 length curve.");
	set_progress(p_ratio * path->get_curve()->get_baked_length());
}

real_t PathFollow3D::get_progress_ratio() const
{
	if (path && path->get_curve().is_valid() && path->get_curve()->get_baked_length()) {
		return get_progress() / path->get_curve()->get_baked_length();
	}
	else {
		return 0;
	}
}

void PathFollow3D::set_rotation_mode(RotationMode p_rotation_mode)
{
	if (rotation_mode == p_rotation_mode) {
		return;
	}
	rotation_mode = p_rotation_mode;

	update_configuration_warnings();
	update_transform();
}

PathFollow3D::RotationMode PathFollow3D::get_rotation_mode() const { return rotation_mode; }

void PathFollow3D::set_use_model_front(bool p_use_model_front)
{
	if (use_model_front == p_use_model_front) {
		return;
	}
	use_model_front = p_use_model_front;
	update_transform();
}

bool PathFollow3D::is_using_model_front() const { return use_model_front; }

void PathFollow3D::set_loop(bool p_loop)
{
	if (loop == p_loop) {
		return;
	}
	loop = p_loop;
	update_transform();
}

bool PathFollow3D::has_loop() const { return loop; }

void PathFollow3D::set_tilt_enabled(bool p_enabled)
{
	if (tilt_enabled == p_enabled) {
		return;
	}
	tilt_enabled = p_enabled;
	update_transform();
}

bool PathFollow3D::is_tilt_enabled() const { return tilt_enabled; }


