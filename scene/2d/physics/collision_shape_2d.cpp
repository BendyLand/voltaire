/**************************************************************************/
/*  collision_shape_2d.cpp                                                */
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

#include "collision_shape_2d.h"

#include "core/config/engine.h"
#include "scene/2d/physics/collision_object_2d.h"
#include "scene/main/scene_tree.h"

void CollisionShape2D::_update_in_shape_owner(bool p_xform_only) {
	if (!collision_state) {
		return;
	}
	collision_state->shape_owner_set_transform(owner_id, get_transform());
	if (p_xform_only) {
		return;
	}
	collision_state->shape_owner_set_disabled(owner_id, disabled);
	collision_state->shape_owner_set_one_way_collision(owner_id, one_way_collision);
	collision_state->shape_owner_set_one_way_collision_margin(owner_id, one_way_collision_margin);
	collision_state->shape_owner_set_one_way_collision_direction(owner_id, one_way_collision_direction);
}

void CollisionShape2D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PARENTED: {
			Node2D *parent_2d = dynamic_cast<Node2D *>(get_parent());
			collision_state = parent_2d ? parent_2d->get_collision_state_2d() : nullptr;
			if (collision_state) {
				owner_id = collision_state->create_shape_owner(this);
				if (shape.is_valid()) {
					collision_state->shape_owner_add_shape(owner_id, shape.ptr());
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

void CollisionShape2D::_shape_changed() {
	queue_redraw();
}

void CollisionShape2D::set_shape(const Ref<Shape2D> &p_shape) {
	if (shape == p_shape) {
		return;
	}
	shape = p_shape;
	queue_redraw();
	if (collision_state) {
		collision_state->shape_owner_clear_shapes(owner_id);
		if (shape.is_valid()) {
			collision_state->shape_owner_add_shape(owner_id, shape.ptr());
		}
		_update_in_shape_owner();
	}
	update_configuration_warnings();
}

Ref<Shape2D> CollisionShape2D::get_shape() const {
	return shape;
}

bool CollisionShape2D::_edit_is_selected_on_click(const Point2 &p_point, double p_tolerance) const {
	if (shape.is_null()) {
		return false;
	}
	return shape->_edit_is_selected_on_click(p_point, p_tolerance);
}

void CollisionShape2D::set_disabled(bool p_disabled) {
	disabled = p_disabled;
	queue_redraw();
	if (collision_state) {
		collision_state->shape_owner_set_disabled(owner_id, p_disabled);
	}
}

bool CollisionShape2D::is_disabled() const {
	return disabled;
}

void CollisionShape2D::set_one_way_collision(bool p_enable) {
	one_way_collision = p_enable;
	queue_redraw();
	if (collision_state) {
		collision_state->shape_owner_set_one_way_collision(owner_id, p_enable);
	}
}

bool CollisionShape2D::is_one_way_collision_enabled() const {
	return one_way_collision;
}

void CollisionShape2D::set_one_way_collision_margin(real_t p_margin) {
	one_way_collision_margin = p_margin;
	if (collision_state) {
		collision_state->shape_owner_set_one_way_collision_margin(owner_id, one_way_collision_margin);
	}
}

real_t CollisionShape2D::get_one_way_collision_margin() const {
	return one_way_collision_margin;
}

void CollisionShape2D::set_one_way_collision_direction(const Vector2 &p_direction) {
	one_way_collision_direction = p_direction;
	if (collision_state) {
		collision_state->shape_owner_set_one_way_collision_direction(owner_id, one_way_collision_direction);
	}
}

Vector2 CollisionShape2D::get_one_way_collision_direction() const {
	return one_way_collision_direction;
}

Color CollisionShape2D::_get_default_debug_color() const {
	return SceneTree::get_debug_collisions_color();
}

void CollisionShape2D::set_debug_color(const Color &p_color) {
	debug_color = p_color;
	queue_redraw();
}

Color CollisionShape2D::get_debug_color() const {
	return debug_color;
}

#ifdef DEBUG_ENABLED
bool CollisionShape2D::_property_can_revert(const String &p_name) const {
	return p_name == "debug_color";
}
#endif // DEBUG_ENABLED

CollisionShape2D::CollisionShape2D() {
	set_notify_local_transform(true);
	set_hide_clip_children(true);
	debug_color = _get_default_debug_color();
}

PackedStringArray CollisionShape2D::get_configuration_warnings() const {
	return PackedStringArray();
}
