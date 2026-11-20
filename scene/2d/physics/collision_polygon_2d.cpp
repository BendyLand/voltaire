/**************************************************************************/
/*  collision_polygon_2d.cpp                                              */
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

#include "collision_polygon_2d.h"

#include "core/config/engine.h"
#include "core/math/geometry_2d.h"
#include "scene/2d/physics/collision_object_2d.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/2d/shape_2d.h"

void CollisionPolygon2D::_build_polygon() {
	if (!collision_state) {
		return;
	}
	collision_state->shape_owner_clear_shapes(owner_id);

	if (build_mode == BUILD_SOLIDS) {
		if (polygon.size() < 3) {
			return;
		}
		Vector<Vector<Vector2>> decomp = _decompose_in_convex();
		for (int i = 0; i < decomp.size(); i++) {
			Ref<Shape2D> convex = memnew(Shape2D(Shape2D::SHAPE_CONVEX_POLYGON));
			convex->set_points(decomp[i]);
			collision_state->shape_owner_add_shape(owner_id, convex.ptr());
		}
	} else {
		if (polygon.size() < 2) {
			return;
		}
		Ref<Shape2D> concave = memnew(Shape2D(Shape2D::SHAPE_CONCAVE_POLYGON));
		Vector<Vector2> segments;
		segments.resize(polygon.size() * 2);
		Vector2 *w = segments.ptrw();
		for (int i = 0; i < polygon.size(); i++) {
			w[(i << 1) + 0] = polygon[i];
			w[(i << 1) + 1] = polygon[(i + 1) % polygon.size()];
		}
		concave->set_segments(segments);
		collision_state->shape_owner_add_shape(owner_id, concave.ptr());
	}
}

Vector<Vector<Vector2>> CollisionPolygon2D::_decompose_in_convex() {
	return Geometry2D::decompose_polygon_in_convex(polygon);
}

void CollisionPolygon2D::_update_in_shape_owner(bool p_xform_only) {
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

void CollisionPolygon2D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_PARENTED: {
			Node2D *parent_2d = dynamic_cast<Node2D *>(get_parent());
			collision_state = parent_2d ? parent_2d->get_collision_state_2d() : nullptr;
			if (collision_state) {
				owner_id = collision_state->create_shape_owner(this);
				_build_polygon();
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

void CollisionPolygon2D::set_polygon(const Vector<Point2> &p_polygon) {
	polygon = p_polygon;
	if (collision_state) {
		_build_polygon();
		_update_in_shape_owner();
	}
	queue_redraw();
}

Vector<Point2> CollisionPolygon2D::get_polygon() const {
	return polygon;
}

void CollisionPolygon2D::set_build_mode(BuildMode p_mode) {
	ERR_FAIL_INDEX((int)p_mode, 2);
	build_mode = p_mode;
	if (collision_state) {
		_build_polygon();
		_update_in_shape_owner();
	}
	queue_redraw();
}

CollisionPolygon2D::BuildMode CollisionPolygon2D::get_build_mode() const {
	return build_mode;
}

#ifdef DEBUG_ENABLED
Rect2 CollisionPolygon2D::_edit_get_rect() const {
	return aabb;
}

bool CollisionPolygon2D::_edit_use_rect() const {
	return true;
}

bool CollisionPolygon2D::_edit_is_selected_on_click(const Vector2 &p_point, double p_tolerance) const {
	return false;
}
#endif

void CollisionPolygon2D::set_disabled(bool p_disabled) {
	disabled = p_disabled;
	queue_redraw();
	if (collision_state) {
		collision_state->shape_owner_set_disabled(owner_id, p_disabled);
	}
}

bool CollisionPolygon2D::is_disabled() const {
	return disabled;
}

void CollisionPolygon2D::set_one_way_collision(bool p_enable) {
	one_way_collision = p_enable;
	queue_redraw();
	if (collision_state) {
		collision_state->shape_owner_set_one_way_collision(owner_id, p_enable);
	}
}

bool CollisionPolygon2D::is_one_way_collision_enabled() const {
	return one_way_collision;
}

void CollisionPolygon2D::set_one_way_collision_margin(real_t p_margin) {
	one_way_collision_margin = p_margin;
	if (collision_state) {
		collision_state->shape_owner_set_one_way_collision_margin(owner_id, one_way_collision_margin);
	}
}

real_t CollisionPolygon2D::get_one_way_collision_margin() const {
	return one_way_collision_margin;
}

void CollisionPolygon2D::set_one_way_collision_direction(const Vector2 &p_direction) {
	one_way_collision_direction = p_direction;
	if (collision_state) {
		collision_state->shape_owner_set_one_way_collision_direction(owner_id, one_way_collision_direction);
	}
}

Vector2 CollisionPolygon2D::get_one_way_collision_direction() const {
	return one_way_collision_direction;
}

CollisionPolygon2D::CollisionPolygon2D() {
	set_notify_local_transform(true);
	set_hide_clip_children(true);
}

PackedStringArray CollisionPolygon2D::get_configuration_warnings() const {
	return PackedStringArray();
}
