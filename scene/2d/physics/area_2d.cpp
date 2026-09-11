/**************************************************************************/
/*  area_2d.cpp                                                           */
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

#include "area_2d.h"
#include "core/config/engine.h"
#include "servers/audio/audio_server.h"
#include "servers/physics_2d/physics_server_2d.h"

Area2D::SpaceOverride Area2D::get_gravity_space_override_mode() const
{
	return gravity_space_override;
}

bool Area2D::is_gravity_a_point() const { return gravity_is_point; }

real_t Area2D::get_gravity_point_unit_distance() const { return gravity_point_unit_distance; }

const Vector2& Area2D::get_gravity_point_center() const { return gravity_vec; }

const Vector2& Area2D::get_gravity_direction() const { return gravity_vec; }

real_t Area2D::get_gravity() const { return gravity; }

Area2D::SpaceOverride Area2D::get_linear_damp_space_override_mode() const
{
	return linear_damp_space_override;
}

Area2D::SpaceOverride Area2D::get_angular_damp_space_override_mode() const
{
	return angular_damp_space_override;
}

real_t Area2D::get_linear_damp() const { return linear_damp; }

real_t Area2D::get_angular_damp() const { return angular_damp; }

int Area2D::get_priority() const { return priority; }

void Area2D::_space_changed(const RID& p_new_space)
{
	if (p_new_space.is_null()) {
		_clear_monitoring();
	}
}

bool Area2D::is_monitoring() const { return monitoring; }

void Area2D::set_monitorable(bool p_enable)
{
	ERR_FAIL_COND_MSG(
		locked || (is_inside_tree() && PhysicsServer2D::get_singleton()->is_flushing_queries()),
		"Function blocked during in/out signal. Use set_deferred(\"monitorable\", true/false).");

	if (p_enable == monitorable) {
		return;
	}

	monitorable = p_enable;

	PhysicsServer2D::get_singleton()->area_set_monitorable(get_rid(), monitorable);
}

bool Area2D::is_monitorable() const { return monitorable; }

void Area2D::set_audio_bus_override(bool p_override) { audio_bus_override = p_override; }

bool Area2D::is_overriding_audio_bus() const { return audio_bus_override; }

void Area2D::set_audio_bus_name(const StringName& p_audio_bus) { audio_bus = p_audio_bus; }

StringName Area2D::get_audio_bus_name() const
{
	for (int i = 0; i < AudioServer::get_singleton()->get_bus_count(); i++) {
		if (AudioServer::get_singleton()->get_bus_name(i) == audio_bus) {
			return audio_bus;
		}
	}
	return SceneStringName(Master);
}


