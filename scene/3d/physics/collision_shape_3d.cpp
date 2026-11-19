/**************************************************************************/
/*  collision_shape_3d.cpp                                                */
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

#include "collision_shape_3d.h"

#include "scene/3d/physics/collision_object_3d.h"
#include "scene/main/scene_tree.h"

void CollisionShape3D::_update_in_shape_owner(bool p_xform_only) {
	if (!collision_state) {
		return;
	}
	collision_state->shape_owner_set_transform(owner_id, get_transform());
	if (p_xform_only) {
		return;
	}
	collision_state->shape_owner_set_disabled(owner_id, disabled);
}

void CollisionShape3D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PARENTED: {
			Node3D *parent_3d = dynamic_cast<Node3D *>(get_parent());
			collision_state = parent_3d ? parent_3d->get_collision_state_3d() : nullptr;
			if (collision_state) {
				owner_id = collision_state->create_shape_owner(this);
				if (shape.is_valid()) {
					collision_state->shape_owner_add_shape(owner_id, shape);
				}
				_update_in_shape_owner();
			}
		} break;
		case NOTIFICATION_ENTER_TREE: {
			if (collision_state) {
				_update_in_shape_owner();
			}
		} break;
		case NOTIFICATION_LOCAL_TRANSFORM_CHANGED: {
			if (collision_state) {
				_update_in_shape_owner(true);
			}
		} break;
		case NOTIFICATION_UNPARENTED: {
			if (collision_state) {
				collision_state->remove_shape_owner(owner_id);
			}
			owner_id = 0;
			collision_state = nullptr;
		} break;
	}
}

#ifndef DISABLE_DEPRECATED
void CollisionShape3D::resource_changed(Ref<Resource> res) {}
#endif

Ref<Shape3D> CollisionShape3D::get_shape() const {
	return shape;
}

void CollisionShape3D::set_shape(const Ref<Shape3D> &p_shape) {
	if (shape == p_shape) {
		return;
	}
	shape = p_shape;
	if (shape.is_valid()) {
		shape->set_debug_color(debug_color);
		shape->set_debug_fill(debug_fill);
	}
	if (collision_state) {
		collision_state->shape_owner_clear_shapes(owner_id);
		if (shape.is_valid()) {
			collision_state->shape_owner_add_shape(owner_id, shape);
		}
		_update_in_shape_owner();
	}
	update_configuration_warnings();
}

void CollisionShape3D::set_disabled(bool p_disabled) {
	disabled = p_disabled;
	if (collision_state) {
		collision_state->shape_owner_set_disabled(owner_id, p_disabled);
	}
}

bool CollisionShape3D::is_disabled() const {
	return disabled;
}

Color CollisionShape3D::_get_default_debug_color() const {
	return SceneTree::get_debug_collisions_color();
}

void CollisionShape3D::set_debug_color(const Color &p_color) {
	if (debug_color == p_color) {
		return;
	}
	debug_color = p_color;
	if (shape.is_valid()) {
		shape->set_debug_color(p_color);
	}
}

Color CollisionShape3D::get_debug_color() const {
	return debug_color;
}

void CollisionShape3D::set_debug_fill_enabled(bool p_enable) {
	if (debug_fill == p_enable) {
		return;
	}
	debug_fill = p_enable;
	if (shape.is_valid()) {
		shape->set_debug_fill(p_enable);
	}
}

bool CollisionShape3D::get_debug_fill_enabled() const {
	return debug_fill;
}

#ifdef DEBUG_ENABLED
void CollisionShape3D::_shape_changed() {
	if (shape.is_valid()) {
		if (shape->get_debug_color() != debug_color) {
			set_debug_color(shape->get_debug_color());
		}
		if (shape->get_debug_fill() != debug_fill) {
			set_debug_fill_enabled(shape->get_debug_fill());
		}
	}
}
#endif // DEBUG_ENABLED

CollisionShape3D::CollisionShape3D() {
	set_notify_local_transform(true);
	debug_color = _get_default_debug_color();
}

CollisionShape3D::~CollisionShape3D() {}

PackedStringArray CollisionShape3D::get_configuration_warnings() const {
	return PackedStringArray();
}
