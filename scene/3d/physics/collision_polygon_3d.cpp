/**************************************************************************/
/*  collision_polygon_3d.cpp                                              */
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

#include "collision_polygon_3d.h"
#include "core/math/geometry_2d.h"
#include "scene/3d/physics/collision_object_3d.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/3d/convex_polygon_shape_3d.h"

void CollisionPolygon3D::_update_in_shape_owner(bool p_xform_only)
{
	collision_object->shape_owner_set_transform(owner_id, get_transform());
	if (p_xform_only) {
		return;
	}
	collision_object->shape_owner_set_disabled(owner_id, disabled);
}

Vector<Point2> CollisionPolygon3D::get_polygon() const { return polygon; }

AABB CollisionPolygon3D::get_item_rect() const { return aabb; }

real_t CollisionPolygon3D::get_depth() const { return depth; }

bool CollisionPolygon3D::is_disabled() const { return disabled; }

Color CollisionPolygon3D::_get_default_debug_color() const
{
	const SceneTree* st = SceneTree::get_singleton();
	return st ? st->get_debug_collisions_color() : Color(0.0, 0.0, 0.0, 0.0);
}

Color CollisionPolygon3D::get_debug_color() const { return debug_color; }

bool CollisionPolygon3D::get_debug_fill_enabled() const { return debug_fill; }

real_t CollisionPolygon3D::get_margin() const { return margin; }

bool CollisionPolygon3D::_is_editable_3d_polygon() const { return true; }

CollisionPolygon3D::CollisionPolygon3D()
{
	set_notify_local_transform(true);
	debug_color = _get_default_debug_color();
}

PackedStringArray CollisionPolygon3D::get_configuration_warnings() const
{
	return PackedStringArray();
}


