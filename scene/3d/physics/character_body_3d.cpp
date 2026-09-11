/**************************************************************************/
/*  character_body_3d.cpp                                                 */
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

#include "character_body_3d.h"
#include "core/config/engine.h"

// so, if you pass 45 as limit, avoid numerical precision errors when angle is 45.
#define FLOOR_ANGLE_THRESHOLD 0.01

void CharacterBody3D::apply_floor_snap()
{
	if (collision_state.floor) {
		return;
	}

	// Snap by at least collision margin to keep floor state consistent.
	real_t length = MAX(floor_snap_length, margin);

	PS3DT::MotionParameters parameters(get_global_transform(), -up_direction * length, margin);
	parameters.max_collisions = 4;
	parameters.recovery_as_collision = true; // Also report collisions generated only from recovery.
	parameters.collide_separation_ray = true;

	PS3DT::MotionResult result;
	if (move_and_collide(parameters, result, true, false)) {
		CollisionState result_state;
		// Apply direction for floor only.
		_set_collision_direction(result, result_state, CollisionState(true, false, false));

		if (result_state.floor) {
			// Ensure that we only move the body along the up axis, because
			// move_and_collide may stray the object a bit when getting it unstuck.
			// Canceling this motion should not affect move_and_slide, as previous
			// calls to move_and_collide already took care of freeing the body.
			if (result.travel.length() > margin) {
				result.travel = up_direction * up_direction.dot(result.travel);
			}
			else {
				result.travel = Vector3();
			}

			parameters.from.origin += result.travel;
			set_global_transform(parameters.from);
		}
	}
}

void CharacterBody3D::_snap_on_floor(bool p_was_on_floor, bool p_vel_dir_facing_up)
{
	if (collision_state.floor || !p_was_on_floor || p_vel_dir_facing_up) {
		return;
	}

	apply_floor_snap();
}

bool CharacterBody3D::_on_floor_if_snapped(bool p_was_on_floor, bool p_vel_dir_facing_up)
{
	if (up_direction == Vector3() || collision_state.floor || !p_was_on_floor ||
		p_vel_dir_facing_up) {
		return false;
	}

	// Snap by at least collision margin to keep floor state consistent.
	real_t length = MAX(floor_snap_length, margin);

	PS3DT::MotionParameters parameters(get_global_transform(), -up_direction * length, margin);
	parameters.max_collisions = 4;
	parameters.recovery_as_collision = true; // Also report collisions generated only from recovery.
	parameters.collide_separation_ray = true;

	PS3DT::MotionResult result;
	if (move_and_collide(parameters, result, true, false)) {
		CollisionState result_state;
		// Don't apply direction for any type.
		_set_collision_direction(result, result_state, CollisionState());

		return result_state.floor;
	}

	return false;
}

void CharacterBody3D::set_safe_margin(real_t p_margin) { margin = p_margin; }

real_t CharacterBody3D::get_safe_margin() const { return margin; }

const Vector3& CharacterBody3D::get_velocity() const { return velocity; }

void CharacterBody3D::set_velocity(const Vector3& p_velocity) { velocity = p_velocity; }

bool CharacterBody3D::is_on_floor() const { return collision_state.floor; }

bool CharacterBody3D::is_on_floor_only() const
{
	return collision_state.floor && !collision_state.wall && !collision_state.ceiling;
}

bool CharacterBody3D::is_on_wall() const { return collision_state.wall; }

bool CharacterBody3D::is_on_wall_only() const
{
	return collision_state.wall && !collision_state.floor && !collision_state.ceiling;
}

bool CharacterBody3D::is_on_ceiling() const { return collision_state.ceiling; }

bool CharacterBody3D::is_on_ceiling_only() const
{
	return collision_state.ceiling && !collision_state.floor && !collision_state.wall;
}

const Vector3& CharacterBody3D::get_floor_normal() const { return floor_normal; }

const Vector3& CharacterBody3D::get_wall_normal() const { return wall_normal; }

const Vector3& CharacterBody3D::get_last_motion() const { return last_motion; }

Vector3 CharacterBody3D::get_position_delta() const
{
	return get_global_transform().origin - previous_position;
}

const Vector3& CharacterBody3D::get_real_velocity() const { return real_velocity; }

real_t CharacterBody3D::get_floor_angle(const Vector3& p_up_direction) const
{
	ERR_FAIL_COND_V(p_up_direction == Vector3(), 0);
	return Math::acos(floor_normal.dot(p_up_direction));
}

const Vector3& CharacterBody3D::get_platform_velocity() const { return platform_velocity; }

const Vector3& CharacterBody3D::get_platform_angular_velocity() const
{
	return platform_angular_velocity;
}

Vector3 CharacterBody3D::get_linear_velocity() const { return get_real_velocity(); }

int CharacterBody3D::get_slide_collision_count() const { return motion_results.size(); }

PS3DT::MotionResult CharacterBody3D::get_slide_collision(int p_bounce) const
{
	ERR_FAIL_INDEX_V(p_bounce, motion_results.size(), PS3DT::MotionResult());
	return motion_results[p_bounce];
}

Ref<KinematicCollision3D> CharacterBody3D::_get_last_slide_collision()
{
	if (motion_results.is_empty()) {
		return Ref<KinematicCollision3D>();
	}
	return _get_slide_collision(motion_results.size() - 1);
}

bool CharacterBody3D::is_floor_stop_on_slope_enabled() const { return floor_stop_on_slope; }

void CharacterBody3D::set_floor_stop_on_slope_enabled(bool p_enabled)
{
	floor_stop_on_slope = p_enabled;
}

bool CharacterBody3D::is_floor_constant_speed_enabled() const { return floor_constant_speed; }

void CharacterBody3D::set_floor_constant_speed_enabled(bool p_enabled)
{
	floor_constant_speed = p_enabled;
}

bool CharacterBody3D::is_floor_block_on_wall_enabled() const { return floor_block_on_wall; }

void CharacterBody3D::set_floor_block_on_wall_enabled(bool p_enabled)
{
	floor_block_on_wall = p_enabled;
}

bool CharacterBody3D::is_slide_on_ceiling_enabled() const { return slide_on_ceiling; }

void CharacterBody3D::set_slide_on_ceiling_enabled(bool p_enabled) { slide_on_ceiling = p_enabled; }

uint32_t CharacterBody3D::get_platform_floor_layers() const { return platform_floor_layers; }

void CharacterBody3D::set_platform_floor_layers(uint32_t p_exclude_layers)
{
	platform_floor_layers = p_exclude_layers;
}

uint32_t CharacterBody3D::get_platform_wall_layers() const { return platform_wall_layers; }

void CharacterBody3D::set_platform_wall_layers(uint32_t p_exclude_layers)
{
	platform_wall_layers = p_exclude_layers;
}

void CharacterBody3D::set_motion_mode(MotionMode p_mode) { motion_mode = p_mode; }

CharacterBody3D::MotionMode CharacterBody3D::get_motion_mode() const { return motion_mode; }

void CharacterBody3D::set_platform_on_leave(PlatformOnLeave p_on_leave_apply_velocity)
{
	platform_on_leave = p_on_leave_apply_velocity;
}

CharacterBody3D::PlatformOnLeave CharacterBody3D::get_platform_on_leave() const
{
	return platform_on_leave;
}

int CharacterBody3D::get_max_slides() const { return max_slides; }

void CharacterBody3D::set_max_slides(int p_max_slides)
{
	ERR_FAIL_COND(p_max_slides < 1);
	max_slides = p_max_slides;
}

real_t CharacterBody3D::get_floor_max_angle() const { return floor_max_angle; }

void CharacterBody3D::set_floor_max_angle(real_t p_radians) { floor_max_angle = p_radians; }

real_t CharacterBody3D::get_floor_snap_length() { return floor_snap_length; }

void CharacterBody3D::set_floor_snap_length(real_t p_floor_snap_length)
{
	ERR_FAIL_COND(p_floor_snap_length < 0);
	floor_snap_length = p_floor_snap_length;
}

real_t CharacterBody3D::get_wall_min_slide_angle() const { return wall_min_slide_angle; }

void CharacterBody3D::set_wall_min_slide_angle(real_t p_radians)
{
	wall_min_slide_angle = p_radians;
}

const Vector3& CharacterBody3D::get_up_direction() const { return up_direction; }

void CharacterBody3D::set_up_direction(const Vector3& p_up_direction)
{
	ERR_FAIL_COND_MSG(p_up_direction == Vector3(), "up_direction can't be equal to Vector3.ZERO, "
												   "consider using Floating motion mode instead.");
	up_direction = p_up_direction.normalized();
}

CharacterBody3D::CharacterBody3D() : PhysicsBody3D(PS3DE::BODY_MODE_KINEMATIC) {}


