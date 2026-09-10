/**************************************************************************/
/*  ray_cast_2d.cpp                                                       */
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
#include "ray_cast_2d.h"
#include "scene/2d/physics/collision_object_2d.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/world_2d.h"
#include "servers/physics_2d/direct_states/physics_direct_space_state_2d.h"

Vector2 RayCast2D::get_target_position() const { return target_position; }

void RayCast2D::set_collision_mask(uint32_t p_mask) { collision_mask = p_mask; }

uint32_t RayCast2D::get_collision_mask() const { return collision_mask; }

void RayCast2D::set_collision_mask_value(int p_layer_number, bool p_value)
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

bool RayCast2D::get_collision_mask_value(int p_layer_number) const
{
	ERR_FAIL_COND_V_MSG(
		p_layer_number < 1, false, "Collision layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(
		p_layer_number > 32, false, "Collision layer number must be between 1 and 32 inclusive.");
	return get_collision_mask() & (1 << (p_layer_number - 1));
}

bool RayCast2D::is_colliding() const { return collided; }

RID RayCast2D::get_collider_rid() const { return against_rid; }

int RayCast2D::get_collider_shape() const { return against_shape; }

Vector2 RayCast2D::get_collision_point() const { return collision_point; }

Vector2 RayCast2D::get_collision_normal() const { return collision_normal; }

bool RayCast2D::is_enabled() const { return enabled; }

bool RayCast2D::get_exclude_parent_body() const { return exclude_parent_body; }

void RayCast2D::_draw_debug_shape()
{
	Color draw_col = collided ? Color(1.0, 0.01, 0) : get_tree()->get_debug_collisions_color();
	if (!enabled) {
		const float g = draw_col.get_v();
		draw_col.r = g;
		draw_col.g = g;
		draw_col.b = g;
	}

	// Draw an arrow indicating where the RayCast is pointing to
	const real_t max_arrow_size = 6;
	const real_t line_width = 1.4;
	bool no_line = target_position.length() < line_width;
	real_t arrow_size = CLAMP(target_position.length() * 2 / 3, line_width, max_arrow_size);

	if (no_line) {
		arrow_size = target_position.length();
	}
	else {
		draw_line(Vector2(), target_position - target_position.normalized() * arrow_size, draw_col,
			line_width);
	}

	Transform2D xf;
	xf.rotate(target_position.angle());
	xf.translate_local(Vector2(no_line ? 0 : target_position.length() - arrow_size, 0));

	Vector<Vector2> pts = {xf.xform(Vector2(arrow_size, 0)), xf.xform(Vector2(0, 0.5 * arrow_size)),
		xf.xform(Vector2(0, -0.5 * arrow_size))};

	Vector<Color> cols = {draw_col, draw_col, draw_col};

	draw_primitive(pts, cols, Vector<Vector2>());
}

void RayCast2D::force_raycast_update() { _update_raycast_state(); }

void RayCast2D::add_exception_rid(const RID& p_rid) { exclude.insert(p_rid); }

void RayCast2D::add_exception(const CollisionObject2D* rp_node)
{
	add_exception_rid(rp_node->get_rid());
}

void RayCast2D::remove_exception_rid(const RID& p_rid) { exclude.erase(p_rid); }

void RayCast2D::remove_exception(const CollisionObject2D* rp_node)
{
	remove_exception_rid(rp_node->get_rid());
}

void RayCast2D::set_collide_with_areas(bool p_enabled) { collide_with_areas = p_enabled; }

bool RayCast2D::is_collide_with_areas_enabled() const { return collide_with_areas; }

void RayCast2D::set_collide_with_bodies(bool p_enabled) { collide_with_bodies = p_enabled; }

bool RayCast2D::is_collide_with_bodies_enabled() const { return collide_with_bodies; }

void RayCast2D::set_hit_from_inside(bool p_enabled) { hit_from_inside = p_enabled; }

bool RayCast2D::is_hit_from_inside_enabled() const { return hit_from_inside; }

RayCast2D::RayCast2D() { set_hide_clip_children(true); }


