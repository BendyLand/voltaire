/**************************************************************************/
/*  shape_cast_2d.cpp                                                     */
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
#include "scene/2d/physics/collision_object_2d.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/world_2d.h"
#include "servers/physics_2d/direct_states/physics_direct_space_state_2d.h"
#include "servers/physics_2d/physics_server_2d.h"
#include "shape_cast_2d.h"

Vector2 ShapeCast2D::get_target_position() const { return target_position; }

void ShapeCast2D::set_margin(real_t p_margin) { margin = p_margin; }

real_t ShapeCast2D::get_margin() const { return margin; }

void ShapeCast2D::set_max_results(int p_max_results) { max_results = p_max_results; }

int ShapeCast2D::get_max_results() const { return max_results; }

void ShapeCast2D::set_collision_mask(uint32_t p_mask) { collision_mask = p_mask; }

uint32_t ShapeCast2D::get_collision_mask() const { return collision_mask; }

void ShapeCast2D::set_collision_mask_value(int p_layer_number, bool p_value)
{
	ERR_FAIL_COND_MSG(
		p_layer_number < 1, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(
		p_layer_number > 32, "Collision layer number must be between 1 and 32 inclusive.");
	uint32_t mask = get_collision_mask();
	if (p_value) {
		mask |= 1 << (p_layer_number - 1);
	}
	else {
		mask &= ~(1 << (p_layer_number - 1));
	}
	set_collision_mask(mask);
}

bool ShapeCast2D::get_collision_mask_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Collision layer number must be between 1 and 32 inclusive.");
	return get_collision_mask() & (1 << (p_layer_number - 1));
}

int ShapeCast2D::get_collision_count() const { return result.size(); }

bool ShapeCast2D::is_colliding() const { return collided; }

RID ShapeCast2D::get_collider_rid(int p_idx) const
{
	ERR_FAIL_INDEX_V_MSG(p_idx, result.size(), RID(), "No collider RID found.");
	return result[p_idx].rid;
}

int ShapeCast2D::get_collider_shape(int p_idx) const
{
	ERR_FAIL_INDEX_V_MSG(p_idx, result.size(), -1, "No collider shape found.");
	return result[p_idx].shape;
}

Vector2 ShapeCast2D::get_collision_point(int p_idx) const
{
	ERR_FAIL_INDEX_V_MSG(p_idx, result.size(), Vector2(), "No collision point found.");
	return result[p_idx].point;
}

Vector2 ShapeCast2D::get_collision_normal(int p_idx) const
{
	ERR_FAIL_INDEX_V_MSG(p_idx, result.size(), Vector2(), "No collision normal found.");
	return result[p_idx].normal;
}

real_t ShapeCast2D::get_closest_collision_safe_fraction() const { return collision_safe_fraction; }

real_t ShapeCast2D::get_closest_collision_unsafe_fraction() const
{
	return collision_unsafe_fraction;
}

bool ShapeCast2D::is_enabled() const { return enabled; }

Ref<Shape2D> ShapeCast2D::get_shape() const { return shape; }

bool ShapeCast2D::get_exclude_parent_body() const { return exclude_parent_body; }

void ShapeCast2D::force_shapecast_update() { _update_shapecast_state(); }

void ShapeCast2D::add_exception_rid(const RID& p_rid) { exclude.insert(p_rid); }

void ShapeCast2D::add_exception(const CollisionObject2D* rp_node)
{
	add_exception_rid(rp_node->get_rid());
}

void ShapeCast2D::remove_exception_rid(const RID& p_rid) { exclude.erase(p_rid); }

void ShapeCast2D::remove_exception(const CollisionObject2D* rp_node)
{
	remove_exception_rid(rp_node->get_rid());
}

void ShapeCast2D::clear_exceptions() { exclude.clear(); }

void ShapeCast2D::set_collide_with_areas(bool p_clip) { collide_with_areas = p_clip; }

bool ShapeCast2D::is_collide_with_areas_enabled() const { return collide_with_areas; }

void ShapeCast2D::set_collide_with_bodies(bool p_clip) { collide_with_bodies = p_clip; }

bool ShapeCast2D::is_collide_with_bodies_enabled() const { return collide_with_bodies; }

PackedStringArray ShapeCast2D::get_configuration_warnings() const
{
	PackedStringArray warnings = Node2D::get_configuration_warnings();

	if (shape.is_null()) {
		warnings.push_back(
			RTR("This node cannot interact with other objects unless a Shape2D is assigned."));
	}
	return warnings;
}

ShapeCast2D::ShapeCast2D() { set_hide_clip_children(true); }


