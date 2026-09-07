/**************************************************************************/
/*  kinematic_collision_3d.cpp                                            */
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

#include "kinematic_collision_3d.h"
#include "scene/3d/physics/physics_body_3d.h"

Vector3 KinematicCollision3D::get_travel() const { return result.travel; }

Vector3 KinematicCollision3D::get_remainder() const { return result.remainder; }

int KinematicCollision3D::get_collision_count() const { return result.collision_count; }

real_t KinematicCollision3D::get_depth() const { return result.collision_depth; }

Vector3 KinematicCollision3D::get_position(int p_collision_index) const
{
	ERR_FAIL_INDEX_V(p_collision_index, result.collision_count, Vector3());
	return result.collisions[p_collision_index].position;
}

Vector3 KinematicCollision3D::get_normal(int p_collision_index) const
{
	ERR_FAIL_INDEX_V(p_collision_index, result.collision_count, Vector3());
	return result.collisions[p_collision_index].normal;
}

real_t KinematicCollision3D::get_angle(int p_collision_index, const Vector3& p_up_direction) const
{
	ERR_FAIL_INDEX_V(p_collision_index, result.collision_count, 0.0);
	ERR_FAIL_COND_V(p_up_direction == Vector3(), 0);
	return result.collisions[p_collision_index].get_angle(p_up_direction);
}

RID KinematicCollision3D::get_collider_rid(int p_collision_index) const
{
	ERR_FAIL_INDEX_V(p_collision_index, result.collision_count, RID());
	return result.collisions[p_collision_index].collider;
}

int KinematicCollision3D::get_collider_shape_index(int p_collision_index) const
{
	ERR_FAIL_INDEX_V(p_collision_index, result.collision_count, 0);
	return result.collisions[p_collision_index].collider_shape;
}

Vector3 KinematicCollision3D::get_collider_velocity(int p_collision_index) const
{
	ERR_FAIL_INDEX_V(p_collision_index, result.collision_count, Vector3());
	return result.collisions[p_collision_index].collider_velocity;
}

void KinematicCollision3D::_bind_methods() {}


