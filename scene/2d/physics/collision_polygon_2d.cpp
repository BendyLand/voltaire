/**************************************************************************/
/*  collision_polygon_2d.cpp                                              */
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

#include "collision_polygon_2d.h"
#include "core/config/engine.h"
#include "core/math/geometry_2d.h"
#include "scene/2d/physics/area_2d.h"
#include "scene/2d/physics/collision_object_2d.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/2d/concave_polygon_shape_2d.h"
#include "scene/resources/2d/convex_polygon_shape_2d.h"

void CollisionPolygon2D::_build_polygon()
{
	collision_object->shape_owner_clear_shapes(owner_id);

	bool solids = build_mode == BUILD_SOLIDS;

	if (solids) {
		if (polygon.size() < 3) {
			return;
		}

		// here comes the sun, lalalala
		// decompose concave into multiple convex polygons and add them
		Vector<Vector<Vector2>> decomp = _decompose_in_convex();
		for (int i = 0; i < decomp.size(); i++) {
			Ref<ConvexPolygonShape2D> convex = memnew(ConvexPolygonShape2D);
			convex->set_points(decomp[i]);
			collision_object->shape_owner_add_shape(owner_id, convex.ptr());
		}

	}
	else {
		if (polygon.size() < 2) {
			return;
		}

		Ref<ConcavePolygonShape2D> concave = memnew(ConcavePolygonShape2D);

		Vector<Vector2> segments;
		segments.resize(polygon.size() * 2);
		Vector2* w = segments.ptrw();

		for (int i = 0; i < polygon.size(); i++) {
			w[(i << 1) + 0] = polygon[i];
			w[(i << 1) + 1] = polygon[(i + 1) % polygon.size()];
		}

		concave->set_segments(segments);

		collision_object->shape_owner_add_shape(owner_id, concave.ptr());
	}
}

Vector<Vector<Vector2>> CollisionPolygon2D::_decompose_in_convex()
{
	Vector<Vector<Vector2>> decomp = Geometry2D::decompose_polygon_in_convex(polygon);
	return decomp;
}

void CollisionPolygon2D::_update_in_shape_owner(bool p_xform_only)
{
	collision_object->shape_owner_set_transform(owner_id, get_transform());
	if (p_xform_only) {
		return;
	}
	collision_object->shape_owner_set_disabled(owner_id, disabled);
	collision_object->shape_owner_set_one_way_collision(owner_id, one_way_collision);
	collision_object->shape_owner_set_one_way_collision_margin(owner_id, one_way_collision_margin);
}

void CollisionPolygon2D::set_polygon(const Vector<Point2>& p_polygon)
{
	polygon = p_polygon;

	{
		for (int i = 0; i < polygon.size(); i++) {
			if (i == 0) {
				aabb = Rect2(polygon[i], Size2());
			}
			else {
				aabb.expand_to(polygon[i]);
			}
		}
		if (aabb == Rect2()) {
			aabb = Rect2(-10, -10, 20, 20);
		}
		else {
			aabb.position -= aabb.size * 0.3;
			aabb.size += aabb.size * 0.6;
		}
	}

	if (collision_object) {
		_build_polygon();
		_update_in_shape_owner();
	}
	queue_redraw();
	update_configuration_warnings();
}

Vector<Point2> CollisionPolygon2D::get_polygon() const { return polygon; }

void CollisionPolygon2D::set_build_mode(BuildMode p_mode)
{
	ERR_FAIL_INDEX((int)p_mode, 2);
	build_mode = p_mode;
	if (collision_object) {
		_build_polygon();
		_update_in_shape_owner();
	}
	queue_redraw();
	update_configuration_warnings();
}

CollisionPolygon2D::BuildMode CollisionPolygon2D::get_build_mode() const { return build_mode; }

#ifdef DEBUG_ENABLED
Rect2 CollisionPolygon2D::_edit_get_rect() const { return aabb; }
bool CollisionPolygon2D::_edit_use_rect() const { return true; }
#endif

void CollisionPolygon2D::set_disabled(bool p_disabled)
{
	disabled = p_disabled;
	queue_redraw();
	if (collision_object) {
		collision_object->shape_owner_set_disabled(owner_id, p_disabled);
	}
}

bool CollisionPolygon2D::is_disabled() const { return disabled; }

void CollisionPolygon2D::set_one_way_collision(bool p_enable)
{
	one_way_collision = p_enable;
	queue_redraw();
	if (collision_object) {
		collision_object->shape_owner_set_one_way_collision(owner_id, p_enable);
	}
	update_configuration_warnings();
}

bool CollisionPolygon2D::is_one_way_collision_enabled() const { return one_way_collision; }

void CollisionPolygon2D::set_one_way_collision_margin(real_t p_margin)
{
	one_way_collision_margin = p_margin;
	if (collision_object) {
		collision_object->shape_owner_set_one_way_collision_margin(
			owner_id, one_way_collision_margin);
	}
}

real_t CollisionPolygon2D::get_one_way_collision_margin() const { return one_way_collision_margin; }

void CollisionPolygon2D::set_one_way_collision_direction(const Vector2& p_direction)
{
	if (p_direction == one_way_collision_direction) {
		return;
	}

	one_way_collision_direction = p_direction.normalized();
	queue_redraw();
	if (collision_object) {
		collision_object->shape_owner_set_one_way_collision_direction(owner_id, p_direction);
	}
}

Vector2 CollisionPolygon2D::get_one_way_collision_direction() const
{
	return one_way_collision_direction;
}

void CollisionPolygon2D::_bind_methods() {}

CollisionPolygon2D::CollisionPolygon2D()
{
	set_notify_local_transform(true);
	set_hide_clip_children(true);
}


