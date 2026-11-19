/**************************************************************************/
/*  area_2d.cpp                                                           */
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

#include "area_2d.h"

#include "core/config/engine.h"
#include "servers/audio/audio_server.h"
#include "servers/physics_2d/physics_server_2d.h"

Area2D::Area2D() {
	collision_state.init(true);
	set_notify_transform(true);
	set_monitoring(true);
	set_monitorable(true);
}

Area2D::~Area2D() {
	collision_state.free_rid();
}

void Area2D::_notification(int p_what) {
	collision_state.handle_notification(this, p_what);
	if (p_what == NOTIFICATION_EXIT_TREE || p_what == NOTIFICATION_DISABLED) {
		_clear_monitoring();
	}
}

void Area2D::set_gravity_space_override_mode(SpaceOverride p_mode) {
	gravity_space_override = p_mode;
}

Area2D::SpaceOverride Area2D::get_gravity_space_override_mode() const {
	return gravity_space_override;
}

void Area2D::set_gravity_is_point(bool p_enabled) {
	gravity_is_point = p_enabled;
}

bool Area2D::is_gravity_a_point() const {
	return gravity_is_point;
}

void Area2D::set_gravity_point_unit_distance(real_t p_scale) {
	gravity_point_unit_distance = p_scale;
}

real_t Area2D::get_gravity_point_unit_distance() const {
	return gravity_point_unit_distance;
}

void Area2D::set_gravity_point_center(const Vector2 &p_center) {
	gravity_vec = p_center;
}

const Vector2 &Area2D::get_gravity_point_center() const {
	return gravity_vec;
}

void Area2D::set_gravity_direction(const Vector2 &p_direction) {
	gravity_vec = p_direction;
}

const Vector2 &Area2D::get_gravity_direction() const {
	return gravity_vec;
}

void Area2D::set_gravity(real_t p_gravity) {
	gravity = p_gravity;
}

real_t Area2D::get_gravity() const {
	return gravity;
}

void Area2D::set_linear_damp_space_override_mode(SpaceOverride p_mode) {
	linear_damp_space_override = p_mode;
}

Area2D::SpaceOverride Area2D::get_linear_damp_space_override_mode() const {
	return linear_damp_space_override;
}

void Area2D::set_angular_damp_space_override_mode(SpaceOverride p_mode) {
	angular_damp_space_override = p_mode;
}

Area2D::SpaceOverride Area2D::get_angular_damp_space_override_mode() const {
	return angular_damp_space_override;
}

void Area2D::set_linear_damp(real_t p_linear_damp) {
	linear_damp = p_linear_damp;
}

real_t Area2D::get_linear_damp() const {
	return linear_damp;
}

void Area2D::set_angular_damp(real_t p_angular_damp) {
	angular_damp = p_angular_damp;
}

real_t Area2D::get_angular_damp() const {
	return angular_damp;
}

void Area2D::set_priority(int p_priority) {
	priority = p_priority;
}

int Area2D::get_priority() const {
	return priority;
}

void Area2D::set_monitoring(bool p_enable) {
	if (p_enable == monitoring) {
		return;
	}
	monitoring = p_enable;
	if (!monitoring) {
		_clear_monitoring();
	}
}

bool Area2D::is_monitoring() const {
	return monitoring;
}

void Area2D::set_monitorable(bool p_enable) {
	ERR_FAIL_COND_MSG(locked || (is_inside_tree() && PhysicsServer2D::is_flushing_queries()),
			"Function blocked during in/out signal. Use set_deferred(\"monitorable\", true/false).");
	if (p_enable == monitorable) {
		return;
	}
	monitorable = p_enable;
	PhysicsServer2D::area_set_monitorable(get_rid(), monitorable);
}

bool Area2D::is_monitorable() const {
	return monitorable;
}

bool Area2D::has_overlapping_bodies() const {
	return false;
}

bool Area2D::has_overlapping_areas() const {
	return false;
}

bool Area2D::overlaps_area(Node *rp_area) const {
	return false;
}

bool Area2D::overlaps_body(Node *rp_body) const {
	return false;
}

void Area2D::set_audio_bus_override(bool p_override) {
	audio_bus_override = p_override;
}

bool Area2D::is_overriding_audio_bus() const {
	return audio_bus_override;
}

void Area2D::set_audio_bus_name(const String &p_audio_bus) {
	audio_bus = p_audio_bus;
}

String Area2D::get_audio_bus_name() const {
	for (int i = 0; i < AudioServer::get_bus_count(); i++) {
		if (AudioServer::get_bus_name(i) == audio_bus) {
			return audio_bus;
		}
	}
	return SceneString(Master);
}

void Area2D::_clear_monitoring() {}

PackedStringArray Area2D::get_configuration_warnings() const {
	PackedStringArray warnings = Node2D::get_configuration_warnings();
	if (!collision_state.has_any_shapes()) {
		warnings.push_back(RTR("This node has no shape, so it can't collide or interact with other objects.\nAdd inline shapes or a CollisionShape2D / CollisionPolygon2D child."));
	}
	return warnings;
}
