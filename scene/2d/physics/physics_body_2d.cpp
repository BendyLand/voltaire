/**************************************************************************/
/*  physics_body_2d.cpp                                                   */
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

#include "physics_body_2d.h"
#include "scene/main/scene_tree.h"
#include "servers/physics_2d/direct_states/physics_direct_body_state_2d.h"
#include "servers/physics_2d/physics_server_2d.h"

PhysicsBody2D::PhysicsBody2D(PS2DE::BodyMode p_mode)
	: CollisionObject2D(PhysicsServer2D::get_singleton()->body_create(), false)
{
	set_body_mode(p_mode);
	set_pickable(false);
}

Vector2 PhysicsBody2D::get_gravity() const
{
	PhysicsDirectBodyState2D* state =
		PhysicsServer2D::get_singleton()->body_get_direct_state(get_rid());
	ERR_FAIL_NULL_V(state, Vector2());
	return state->get_total_gravity();
}

PackedStringArray PhysicsBody2D::get_configuration_warnings() const
{
	PackedStringArray warnings = CollisionObject2D::get_configuration_warnings();

	if (SceneTree::is_fti_enabled_in_project() && !is_physics_interpolated()) {
		warnings.push_back(
			RTR("PhysicsBody2D will not work correctly on a non-interpolated branch of the "
				"SceneTree.\nCheck the node's inherited physics_interpolation_mode."));
	}

	return warnings;
}


