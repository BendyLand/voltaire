/**************************************************************************/
/*  spring_arm_3d.cpp                                                     */
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
#include "scene/3d/camera_3d.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/3d/shape_3d.h"
#include "spring_arm_3d.h"

void SpringArm3D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		if (!Engine::get_singleton()->is_editor_hint()) {
			set_physics_process_internal(true);
		}
	} break;

	case NOTIFICATION_EXIT_TREE: {
		if (!Engine::get_singleton()->is_editor_hint()) {
			set_physics_process_internal(false);
		}
	} break;

	case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
		process_spring();
	} break;
	}
}


real_t SpringArm3D::get_length() const { return spring_length; }

void SpringArm3D::set_length(real_t p_length)
{
	if (is_inside_tree() &&
		(Engine::get_singleton()->is_editor_hint() || get_tree()->is_debugging_collisions_hint())) {
		update_gizmos();
	}

	spring_length = p_length;
}

void SpringArm3D::set_shape(Ref<Shape3D> p_shape) { shape = p_shape; }

Ref<Shape3D> SpringArm3D::get_shape() const { return shape; }

void SpringArm3D::set_mask(uint32_t p_mask) { mask = p_mask; }

uint32_t SpringArm3D::get_mask() { return mask; }

real_t SpringArm3D::get_margin() { return margin; }

void SpringArm3D::set_margin(real_t p_margin) { margin = p_margin; }

void SpringArm3D::add_excluded_object(RID p_rid) { excluded_objects.insert(p_rid); }

bool SpringArm3D::remove_excluded_object(RID p_rid) { return excluded_objects.erase(p_rid); }

void SpringArm3D::clear_excluded_objects() { excluded_objects.clear(); }

real_t SpringArm3D::get_hit_length() { return current_spring_length; }


