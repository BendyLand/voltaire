/**************************************************************************/
/*  area_3d.cpp                                                           */
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

#include "area_3d.h"
#include "core/config/engine.h"
#include "servers/audio/audio_server.h"

Area3D::SpaceOverride Area3D::get_gravity_space_override_mode() const
{
	return gravity_space_override;
}

bool Area3D::is_gravity_a_point() const { return gravity_is_point; }

real_t Area3D::get_gravity_point_unit_distance() const { return gravity_point_unit_distance; }

const Vector3& Area3D::get_gravity_point_center() const { return gravity_vec; }

const Vector3& Area3D::get_gravity_direction() const { return gravity_vec; }

real_t Area3D::get_gravity() const { return gravity; }

Area3D::SpaceOverride Area3D::get_linear_damp_space_override_mode() const
{
	return linear_damp_space_override;
}

Area3D::SpaceOverride Area3D::get_angular_damp_space_override_mode() const
{
	return angular_damp_space_override;
}

real_t Area3D::get_linear_damp() const { return linear_damp; }

real_t Area3D::get_angular_damp() const { return angular_damp; }

int Area3D::get_priority() const { return priority; }

void Area3D::set_wind_force_magnitude(real_t p_wind_force_magnitude)
{
	wind_force_magnitude = p_wind_force_magnitude;
	if (is_inside_tree()) {
		_initialize_wind();
	}
}

real_t Area3D::get_wind_force_magnitude() const { return wind_force_magnitude; }

void Area3D::set_wind_attenuation_factor(real_t p_wind_force_attenuation_factor)
{
	wind_attenuation_factor = p_wind_force_attenuation_factor;
	if (is_inside_tree()) {
		_initialize_wind();
	}
}

real_t Area3D::get_wind_attenuation_factor() const { return wind_attenuation_factor; }

void Area3D::set_wind_source_path(const NodePath& p_wind_source_path)
{
	wind_source_path = p_wind_source_path;
	if (is_inside_tree()) {
		_initialize_wind();
	}
}

const NodePath& Area3D::get_wind_source_path() const { return wind_source_path; }

void Area3D::_space_changed(const RID& p_new_space)
{
	if (p_new_space.is_null()) {
		_clear_monitoring();
	}
}

void Area3D::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		_initialize_wind();
	} break;
	}
}

bool Area3D::is_monitoring() const { return monitoring; }

void Area3D::set_monitorable(bool p_enable)
{
	ERR_FAIL_COND_MSG(
		locked || (is_inside_tree() && PhysicsServer3D::get_singleton()->is_flushing_queries()),
		"Function blocked during in/out signal. Use set_deferred(\"monitorable\", true/false).");

	if (p_enable == monitorable) {
		return;
	}

	monitorable = p_enable;

	PhysicsServer3D::get_singleton()->area_set_monitorable(get_rid(), monitorable);
}

bool Area3D::is_monitorable() const { return monitorable; }

void Area3D::set_audio_bus_override(bool p_override) { audio_bus_override = p_override; }

bool Area3D::is_overriding_audio_bus() const { return audio_bus_override; }

void Area3D::set_audio_bus_name(const StringName& p_audio_bus) { audio_bus = p_audio_bus; }

StringName Area3D::get_audio_bus_name() const
{
	for (int i = 0; i < AudioServer::get_singleton()->get_bus_count(); i++) {
		if (AudioServer::get_singleton()->get_bus_name(i) == audio_bus) {
			return audio_bus;
		}
	}
	return SceneStringName(Master);
}

void Area3D::set_use_reverb_bus(bool p_enable) { use_reverb_bus = p_enable; }

bool Area3D::is_using_reverb_bus() const { return use_reverb_bus; }

void Area3D::set_reverb_bus_name(const StringName& p_audio_bus) { reverb_bus = p_audio_bus; }

StringName Area3D::get_reverb_bus_name() const
{
	for (int i = 0; i < AudioServer::get_singleton()->get_bus_count(); i++) {
		if (AudioServer::get_singleton()->get_bus_name(i) == reverb_bus) {
			return reverb_bus;
		}
	}
	return SceneStringName(Master);
}

void Area3D::set_reverb_amount(float p_amount) { reverb_amount = p_amount; }

float Area3D::get_reverb_amount() const { return reverb_amount; }

void Area3D::set_reverb_uniformity(float p_uniformity) { reverb_uniformity = p_uniformity; }

float Area3D::get_reverb_uniformity() const { return reverb_uniformity; }

Area3D::Area3D() {}


