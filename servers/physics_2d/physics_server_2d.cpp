/**************************************************************************/
/*  physics_server_2d.cpp                                                 */
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

#include "core/config/project_settings.h"
#include "physics_server_2d.compat.inc"
#include "physics_server_2d.h"

PhysicsServer2D* PhysicsServer2D::singleton = nullptr;

PhysicsServer2D* PhysicsServer2D::get_singleton() { return singleton; }

bool PhysicsServer2D::_body_test_motion(RID p_body, PhysicsTestMotionParameters2D* rp_parameters,
	const Ref<PhysicsTestMotionResult2D>& p_result)
{
	PS2DT::MotionResult* result_ptr = nullptr;
	if (p_result.is_valid()) {
		result_ptr = p_result->get_result_ptr();
	}

	return body_test_motion(p_body, rp_parameters->get_parameters(), result_ptr);
}

void PhysicsServer2D::_bind_methods() {}

PhysicsServer2D::PhysicsServer2D()
{
	singleton = this;

	// World2D physics space
	GLOBAL_DEF_BASIC(PropertyInfo(Variant::FLOAT, "physics/2d/default_gravity", PROPERTY_HINT_RANGE,
						 U"-4096,4096,0.001,or_less,or_greater,suffix:px/s\u00B2"),
		980.0);
	GLOBAL_DEF_BASIC(PropertyInfo(Variant::VECTOR2, "physics/2d/default_gravity_vector",
						 PROPERTY_HINT_RANGE, "-10,10,0.001,or_less,or_greater"),
		Vector2(0, 1));
	GLOBAL_DEF(PropertyInfo(Variant::FLOAT, "physics/2d/default_linear_damp", PROPERTY_HINT_RANGE,
				   "-1,100,0.001,or_greater"),
		0.1);
	GLOBAL_DEF(PropertyInfo(Variant::FLOAT, "physics/2d/default_angular_damp", PROPERTY_HINT_RANGE,
				   "-1,100,0.001,or_greater"),
		1.0);

	// PhysicsServer2D
	GLOBAL_DEF(PropertyInfo(Variant::FLOAT, "physics/2d/sleep_threshold_linear",
				   PROPERTY_HINT_RANGE, "0,10,0.001,or_greater"),
		2.0);
	GLOBAL_DEF(PropertyInfo(Variant::FLOAT, "physics/2d/sleep_threshold_angular",
				   PROPERTY_HINT_RANGE, "0,90,0.1,radians_as_degrees"),
		Math::deg_to_rad(8.0));
	GLOBAL_DEF(PropertyInfo(Variant::FLOAT, "physics/2d/time_before_sleep", PROPERTY_HINT_RANGE,
				   "0,5,0.01,or_greater,suffix:s"),
		0.5);
	GLOBAL_DEF(PropertyInfo(Variant::INT, "physics/2d/solver/solver_iterations",
				   PROPERTY_HINT_RANGE, "1,32,1,or_greater"),
		16);
	GLOBAL_DEF(PropertyInfo(Variant::FLOAT, "physics/2d/solver/contact_recycle_radius",
				   PROPERTY_HINT_RANGE, "0,10,0.01,or_greater"),
		1.0);
	GLOBAL_DEF(PropertyInfo(Variant::FLOAT, "physics/2d/solver/contact_max_separation",
				   PROPERTY_HINT_RANGE, "0,10,0.01,or_greater"),
		1.5);
	GLOBAL_DEF(PropertyInfo(Variant::FLOAT, "physics/2d/solver/contact_max_allowed_penetration",
				   PROPERTY_HINT_RANGE, "0.01,10,0.01,or_greater"),
		0.3);
	GLOBAL_DEF(PropertyInfo(Variant::FLOAT, "physics/2d/solver/default_contact_bias",
				   PROPERTY_HINT_RANGE, "0,1,0.01"),
		0.8);
	GLOBAL_DEF(PropertyInfo(Variant::FLOAT, "physics/2d/solver/default_constraint_bias",
				   PROPERTY_HINT_RANGE, "0,1,0.01"),
		0.2);
}

PhysicsServer2D::~PhysicsServer2D() { singleton = nullptr; }

PhysicsServer2DManager* PhysicsServer2DManager::singleton = nullptr;
const String PhysicsServer2DManager::setting_property_name(PNAME("physics/2d/physics_engine"));

void PhysicsServer2DManager::on_servers_changed()
{
	String physics_servers("DEFAULT");
	for (int i = get_servers_count() - 1; 0 <= i; --i) {
		physics_servers += "," + get_server_name(i);
	}
	ProjectSettings::get_singleton()->set_custom_property_info(
		PropertyInfo(Variant::STRING, setting_property_name, PROPERTY_HINT_ENUM, physics_servers));
	ProjectSettings::get_singleton()->set_restart_if_changed(setting_property_name, true);
	ProjectSettings::get_singleton()->set_as_basic(setting_property_name, true);
}

void PhysicsServer2DManager::_bind_methods() {}

PhysicsServer2DManager* PhysicsServer2DManager::get_singleton() { return singleton; }

void PhysicsServer2DManager::register_server(
	const String& p_name, const Callable& p_create_callback)
{
	// ERR_FAIL_COND(!p_create_callback.is_valid());
	ERR_FAIL_COND(find_server_id(p_name) != -1);
	physics_2d_servers.push_back(ClassInfo(p_name, p_create_callback));
	on_servers_changed();
}

void PhysicsServer2DManager::set_default_server(const String& p_name, int p_priority)
{
	const int id = find_server_id(p_name);
	ERR_FAIL_COND(id == -1); // Not found
	if (default_server_priority < p_priority) {
		default_server_id = id;
		default_server_priority = p_priority;
	}
}

int PhysicsServer2DManager::find_server_id(const String& p_name)
{
	for (int i = physics_2d_servers.size() - 1; 0 <= i; --i) {
		if (p_name == physics_2d_servers[i].name) {
			return i;
		}
	}
	return -1;
}

int PhysicsServer2DManager::get_servers_count() { return physics_2d_servers.size(); }

String PhysicsServer2DManager::get_server_name(int p_id)
{
	ERR_FAIL_INDEX_V(p_id, get_servers_count(), "");
	return physics_2d_servers[p_id].name;
}

PhysicsServer2D* PhysicsServer2DManager::new_default_server()
{
	if (default_server_id == -1) {
		return nullptr;
	}
	Variant ret;
	Callable::CallError ce;
	physics_2d_servers[default_server_id].create_callback.callp(nullptr, 0, ret, ce);
	ERR_FAIL_COND_V(ce.error != Callable::CallError::CALL_OK, nullptr);
	return Object::cast_to<PhysicsServer2D>(ret.get_validated_object());
}

PhysicsServer2D* PhysicsServer2DManager::new_server(const String& p_name)
{
	int id = find_server_id(p_name);
	if (id == -1) {
		return nullptr;
	}
	else {
		Variant ret;
		Callable::CallError ce;
		physics_2d_servers[id].create_callback.callp(nullptr, 0, ret, ce);
		ERR_FAIL_COND_V(ce.error != Callable::CallError::CALL_OK, nullptr);
		return Object::cast_to<PhysicsServer2D>(ret.get_validated_object());
	}
}

PhysicsServer2DManager::PhysicsServer2DManager() { singleton = this; }

PhysicsServer2DManager::~PhysicsServer2DManager() { singleton = nullptr; }


