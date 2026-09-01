/**************************************************************************/
/*  character_body_2d.cpp                                                 */
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

#include "character_body_2d.h"
#include "core/config/engine.h"
#include "servers/physics_2d/direct_states/physics_direct_body_state_2d.h"
#include "servers/physics_2d/physics_server_2d.h"

// So, if you pass 45 as limit, avoid numerical precision errors when angle is 45.
#define FLOOR_ANGLE_THRESHOLD 0.01

void CharacterBody2D::apply_floor_snap() { _apply_floor_snap(); }

void CharacterBody2D::_snap_on_floor(
	bool p_was_on_floor, bool p_vel_dir_facing_up, bool p_wall_as_floor)
{
	if (on_floor || !p_was_on_floor || p_vel_dir_facing_up) {
		return;
	}

	_apply_floor_snap(p_wall_as_floor);
}

const Vector2& CharacterBody2D::get_velocity() const { return velocity; }

void CharacterBody2D::set_velocity(const Vector2& p_velocity) { velocity = p_velocity; }

bool CharacterBody2D::is_on_floor() const { return on_floor; }

bool CharacterBody2D::is_on_floor_only() const { return on_floor && !on_wall && !on_ceiling; }

bool CharacterBody2D::is_on_wall() const { return on_wall; }

bool CharacterBody2D::is_on_wall_only() const { return on_wall && !on_floor && !on_ceiling; }

bool CharacterBody2D::is_on_ceiling() const { return on_ceiling; }

bool CharacterBody2D::is_on_ceiling_only() const { return on_ceiling && !on_floor && !on_wall; }

const Vector2& CharacterBody2D::get_floor_normal() const { return floor_normal; }

const Vector2& CharacterBody2D::get_wall_normal() const { return wall_normal; }

const Vector2& CharacterBody2D::get_last_motion() const { return last_motion; }

Vector2 CharacterBody2D::get_position_delta() const
{
	return get_global_transform().columns[2] - previous_position;
}

const Vector2& CharacterBody2D::get_real_velocity() const { return real_velocity; }

real_t CharacterBody2D::get_floor_angle(const Vector2& p_up_direction) const
{
	ERR_FAIL_COND_V(p_up_direction == Vector2(), 0);
	return Math::acos(floor_normal.dot(p_up_direction));
}

const Vector2& CharacterBody2D::get_platform_velocity() const { return platform_velocity; }

int CharacterBody2D::get_slide_collision_count() const { return motion_results.size(); }

PS2DT::MotionResult CharacterBody2D::get_slide_collision(int p_bounce) const
{
	ERR_FAIL_INDEX_V(p_bounce, motion_results.size(), PS2DT::MotionResult());
	return motion_results[p_bounce];
}

Ref<KinematicCollision2D> CharacterBody2D::_get_last_slide_collision()
{
	if (motion_results.is_empty()) {
		return Ref<KinematicCollision2D>();
	}
	return _get_slide_collision(motion_results.size() - 1);
}

void CharacterBody2D::set_safe_margin(real_t p_margin) { margin = p_margin; }

real_t CharacterBody2D::get_safe_margin() const { return margin; }

bool CharacterBody2D::is_floor_stop_on_slope_enabled() const { return floor_stop_on_slope; }

void CharacterBody2D::set_floor_stop_on_slope_enabled(bool p_enabled)
{
	floor_stop_on_slope = p_enabled;
}

bool CharacterBody2D::is_floor_constant_speed_enabled() const { return floor_constant_speed; }

void CharacterBody2D::set_floor_constant_speed_enabled(bool p_enabled)
{
	floor_constant_speed = p_enabled;
}

bool CharacterBody2D::is_floor_block_on_wall_enabled() const { return floor_block_on_wall; }

void CharacterBody2D::set_floor_block_on_wall_enabled(bool p_enabled)
{
	floor_block_on_wall = p_enabled;
}

bool CharacterBody2D::is_slide_on_ceiling_enabled() const { return slide_on_ceiling; }

void CharacterBody2D::set_slide_on_ceiling_enabled(bool p_enabled) { slide_on_ceiling = p_enabled; }

uint32_t CharacterBody2D::get_platform_floor_layers() const { return platform_floor_layers; }

void CharacterBody2D::set_platform_floor_layers(uint32_t p_exclude_layers)
{
	platform_floor_layers = p_exclude_layers;
}

uint32_t CharacterBody2D::get_platform_wall_layers() const { return platform_wall_layers; }

void CharacterBody2D::set_platform_wall_layers(uint32_t p_exclude_layers)
{
	platform_wall_layers = p_exclude_layers;
}

void CharacterBody2D::set_motion_mode(MotionMode p_mode) { motion_mode = p_mode; }

CharacterBody2D::MotionMode CharacterBody2D::get_motion_mode() const { return motion_mode; }

void CharacterBody2D::set_platform_on_leave(PlatformOnLeave p_on_leave_apply_velocity)
{
	platform_on_leave = p_on_leave_apply_velocity;
}

CharacterBody2D::PlatformOnLeave CharacterBody2D::get_platform_on_leave() const
{
	return platform_on_leave;
}

int CharacterBody2D::get_max_slides() const { return max_slides; }

void CharacterBody2D::set_max_slides(int p_max_slides)
{
	ERR_FAIL_COND(p_max_slides < 1);
	max_slides = p_max_slides;
}

real_t CharacterBody2D::get_floor_max_angle() const { return floor_max_angle; }

void CharacterBody2D::set_floor_max_angle(real_t p_radians) { floor_max_angle = p_radians; }

real_t CharacterBody2D::get_floor_snap_length() { return floor_snap_length; }

void CharacterBody2D::set_floor_snap_length(real_t p_floor_snap_length)
{
	ERR_FAIL_COND(p_floor_snap_length < 0);
	floor_snap_length = p_floor_snap_length;
}

real_t CharacterBody2D::get_wall_min_slide_angle() const { return wall_min_slide_angle; }

void CharacterBody2D::set_wall_min_slide_angle(real_t p_radians)
{
	wall_min_slide_angle = p_radians;
}

const Vector2& CharacterBody2D::get_up_direction() const { return up_direction; }

void CharacterBody2D::set_up_direction(const Vector2& p_up_direction)
{
	ERR_FAIL_COND_MSG(p_up_direction == Vector2(), "up_direction can't be equal to Vector2.ZERO, "
												   "consider using Floating motion mode instead.");
	up_direction = p_up_direction.normalized();
}

CharacterBody2D::CharacterBody2D() : PhysicsBody2D(PS2DE::BODY_MODE_KINEMATIC) {}


