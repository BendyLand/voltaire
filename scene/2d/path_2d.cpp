/**************************************************************************/
/*  path_2d.cpp                                                           */
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
#include "core/math/geometry_2d.h"
#include "path_2d.h"
#include "scene/main/scene_tree.h"
#include "scene/main/timer.h"
#include "scene/resources/mesh.h"
#include "servers/rendering/rendering_server.h"

#ifdef DEBUG_ENABLED
Rect2 Path2D::_edit_get_rect() const
{
	if (curve.is_null() || curve->get_point_count() == 0) {
		return Rect2(0, 0, 0, 0);
	}

	Rect2 aabb = Rect2(curve->get_point_position(0), Vector2(0, 0));

	for (int i = 0; i < curve->get_point_count(); i++) {
		for (int j = 0; j <= 8; j++) {
			real_t frac = j / 8.0;
			Vector2 p = curve->sample(i, frac);
			aabb.expand_to(p);
		}
	}

	return aabb;
}

bool Path2D::_edit_use_rect() const { return curve.is_valid() && curve->get_point_count() != 0; }

bool Path2D::_edit_is_selected_on_click(const Point2& p_point, double p_tolerance) const
{
	if (curve.is_null()) {
		return false;
	}

	for (int i = 0; i < curve->get_point_count(); i++) {
		Vector2 segment_a = curve->get_point_position(i);

		for (int j = 1; j <= 8; j++) {
			real_t frac = j / 8.0;
			const Vector2 segment_b = curve->sample(i, frac);

			Vector2 p = Geometry2D::get_closest_point_to_segment(p_point, segment_a, segment_b);
			if (p.distance_to(p_point) <= p_tolerance) {
				return true;
			}

			segment_a = segment_b;
		}
	}

	return false;
}
#endif

void Path2D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
#ifdef DEBUG_ENABLED
		_debug_create();
#endif
	} break;

	case NOTIFICATION_EXIT_TREE: {
#ifdef DEBUG_ENABLED
		_debug_free();
#endif
	} break;
	// Draw the curve if path debugging is enabled.
	case NOTIFICATION_DRAW: {
#ifdef DEBUG_ENABLED
		_debug_update();
#endif
	} break;
	}
}

#ifdef DEBUG_ENABLED
void Path2D::_debug_create()
{
	ERR_FAIL_NULL(RS::get_singleton());

	if (debug_mesh_rid.is_null()) {
		debug_mesh_rid = RS::get_singleton()->mesh_create();
	}

	if (debug_instance.is_null()) {
		debug_instance = RS::get_singleton()->instance_create();
	}

	RS::get_singleton()->instance_set_base(debug_instance, debug_mesh_rid);
	RS::get_singleton()->instance_geometry_set_cast_shadows_setting(
		debug_instance, RSE::SHADOW_CASTING_SETTING_OFF);
}

void Path2D::_debug_free()
{
	ERR_FAIL_NULL(RS::get_singleton());

	if (debug_instance.is_valid()) {
		RS::get_singleton()->free_rid(debug_instance);
		debug_instance = RID();
	}
	if (debug_mesh_rid.is_valid()) {
		RS::get_singleton()->free_rid(debug_mesh_rid);
		debug_mesh_rid = RID();
	}
}

#endif // DEBUG_ENABLED

Ref<Curve2D> Path2D::get_curve() const { return curve; }


/////////////////////////////////////////////////////////////////////////////////

void PathFollow2D::path_changed()
{
	if (update_timer && !update_timer->is_stopped()) {
		update_timer->start();
	}
	else {
		_update_transform();
	}
}

void PathFollow2D::_update_transform()
{
	if (!path) {
		return;
	}

	Ref<Curve2D> c = path->get_curve();
	if (c.is_null()) {
		return;
	}

	real_t path_length = c->get_baked_length();
	if (path_length == 0) {
		return;
	}

	if (rotates) {
		Transform2D xform = c->sample_baked_with_rotation(progress, cubic);
		xform.translate_local(h_offset, v_offset);
		set_rotation(xform[0].angle());
		set_position(xform[2]);
	}
	else {
		Vector2 pos = c->sample_baked(progress, cubic);
		pos.x += h_offset;
		pos.y += v_offset;
		set_position(pos);
	}
}

void PathFollow2D::set_cubic_interpolation_enabled(bool p_enabled) { cubic = p_enabled; }

bool PathFollow2D::is_cubic_interpolation_enabled() const { return cubic; }

void PathFollow2D::set_progress(real_t p_progress)
{
	ERR_FAIL_COND(!std::isfinite(p_progress));
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

		_update_transform();
	}
}

void PathFollow2D::set_h_offset(real_t p_h_offset)
{
	h_offset = p_h_offset;
	if (path) {
		_update_transform();
	}
}

real_t PathFollow2D::get_h_offset() const { return h_offset; }

void PathFollow2D::set_v_offset(real_t p_v_offset)
{
	v_offset = p_v_offset;
	if (path) {
		_update_transform();
	}
}

real_t PathFollow2D::get_v_offset() const { return v_offset; }

real_t PathFollow2D::get_progress() const { return progress; }

void PathFollow2D::set_progress_ratio(real_t p_ratio)
{
	ERR_FAIL_NULL_MSG(path, "Can only set progress ratio on a PathFollow2D that is the child of a "
							"Path2D which is itself part of the scene tree.");
	ERR_FAIL_COND_MSG(path->get_curve().is_null(),
		"Can't set progress ratio on a PathFollow2D that does not have a Curve.");
	ERR_FAIL_COND_MSG(!path->get_curve()->get_baked_length(),
		"Can't set progress ratio on a PathFollow2D that has a 0 length curve.");
	set_progress(p_ratio * path->get_curve()->get_baked_length());
}

real_t PathFollow2D::get_progress_ratio() const
{
	if (path && path->get_curve().is_valid() && path->get_curve()->get_baked_length()) {
		return get_progress() / path->get_curve()->get_baked_length();
	}
	else {
		return 0;
	}
}

void PathFollow2D::set_rotation_enabled(bool p_enabled)
{
	rotates = p_enabled;
	_update_transform();
}

bool PathFollow2D::is_rotation_enabled() const { return rotates; }

void PathFollow2D::set_loop(bool p_loop) { loop = p_loop; }

bool PathFollow2D::has_loop() const { return loop; }


