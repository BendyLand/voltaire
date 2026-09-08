/**************************************************************************/
/*  skeleton_modification_2d.cpp                                          */
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
#include "scene/2d/skeleton_2d.h"
#include "skeleton_modification_2d.h"

#ifdef TOOLS_ENABLED
#include "editor/settings/editor_settings.h"
#endif // TOOLS_ENABLED

///////////////////////////////////////
// Modification2D
///////////////////////////////////////

void SkeletonModification2D::_setup_modification(SkeletonModificationStack2D* p_stack)
{
	stack = p_stack;
	if (stack) {
		is_setup = true;
	}
	else {
		WARN_PRINT("Could not setup modification with name " + get_name());
	}
}

void SkeletonModification2D::set_enabled(bool p_enabled)
{
	enabled = p_enabled;

#ifdef TOOLS_ENABLED
	if (editor_draw_gizmo) {
		if (stack) {
			stack->set_editor_gizmos_dirty(true);
		}
	}
#endif // TOOLS_ENABLED
}

bool SkeletonModification2D::get_enabled() { return enabled; }

float SkeletonModification2D::clamp_angle(
	float p_angle, float p_min_bound, float p_max_bound, bool p_invert)
{
	// Map to the 0 to 360 range (in radians though) instead of the -180 to 180 range.
	if (p_angle < 0) {
		p_angle = Math::TAU + p_angle;
	}

	// Make min and max in the range of 0 to 360 (in radians), and make sure they are in the right
	// order
	if (p_min_bound < 0) {
		p_min_bound = Math::TAU + p_min_bound;
	}
	if (p_max_bound < 0) {
		p_max_bound = Math::TAU + p_max_bound;
	}
	if (p_min_bound > p_max_bound) {
		SWAP(p_min_bound, p_max_bound);
	}

	bool is_beyond_bounds = (p_angle < p_min_bound || p_angle > p_max_bound);
	bool is_within_bounds = (p_angle > p_min_bound && p_angle < p_max_bound);

	// Note: May not be the most optimal way to clamp, but it always constraints to the nearest
	// angle.
	if ((!p_invert && is_beyond_bounds) || (p_invert && is_within_bounds)) {
		Vector2 min_bound_vec = Vector2(Math::cos(p_min_bound), Math::sin(p_min_bound));
		Vector2 max_bound_vec = Vector2(Math::cos(p_max_bound), Math::sin(p_max_bound));
		Vector2 angle_vec = Vector2(Math::cos(p_angle), Math::sin(p_angle));

		if (angle_vec.distance_squared_to(min_bound_vec) <=
			angle_vec.distance_squared_to(max_bound_vec)) {
			p_angle = p_min_bound;
		}
		else {
			p_angle = p_max_bound;
		}
	}

	return p_angle;
}

Ref<SkeletonModificationStack2D> SkeletonModification2D::get_modification_stack() { return stack; }

void SkeletonModification2D::set_is_setup(bool p_setup) { is_setup = p_setup; }

bool SkeletonModification2D::get_is_setup() const { return is_setup; }

void SkeletonModification2D::set_execution_mode(int p_mode) { execution_mode = p_mode; }

int SkeletonModification2D::get_execution_mode() const { return execution_mode; }

void SkeletonModification2D::set_editor_draw_gizmo(bool p_draw_gizmo)
{
	editor_draw_gizmo = p_draw_gizmo;
#ifdef TOOLS_ENABLED
	if (is_setup) {
		if (stack) {
			stack->set_editor_gizmos_dirty(true);
		}
	}
#endif // TOOLS_ENABLED
}

bool SkeletonModification2D::get_editor_draw_gizmo() const { return editor_draw_gizmo; }

void SkeletonModification2D::reset_state()
{
	stack = nullptr;
	is_setup = false;
}

SkeletonModification2D::SkeletonModification2D()
{
	stack = nullptr;
	is_setup = false;
}

void SkeletonModification2D::_execute(float p_delta) {}

void SkeletonModification2D::_draw_editor_gizmo() {}


