/**************************************************************************/
/*  physics_server_3d.cpp                                                 */
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
#include "core/object/class_db.h"
#include "core/object/ref_counted.h"
#include "physics_server_3d.h"

PhysicsServer3D* PhysicsServer3D::singleton = nullptr;

PhysicsServer3D* PhysicsServer3D::get_singleton() { return singleton; }

bool PhysicsServer3D::_body_test_motion(RID p_body,
	PhysicsTestMotionParameters3D* rp_parameters,
	const Ref<PhysicsTestMotionResult3D>& p_result)
{
	PS3DT::MotionResult* result_ptr = nullptr;
	if (p_result.is_valid()) {
		result_ptr = p_result->get_result_ptr();
	}

	return body_test_motion(p_body, rp_parameters->get_parameters(), result_ptr);
}

RID PhysicsServer3D::shape_create(PS3DE::ShapeType p_shape)
{
	switch (p_shape) {
	case PS3DE::SHAPE_WORLD_BOUNDARY:
		return world_boundary_shape_create();
	case PS3DE::SHAPE_SEPARATION_RAY:
		return separation_ray_shape_create();
	case PS3DE::SHAPE_SPHERE:
		return sphere_shape_create();
	case PS3DE::SHAPE_BOX:
		return box_shape_create();
	case PS3DE::SHAPE_CAPSULE:
		return capsule_shape_create();
	case PS3DE::SHAPE_CYLINDER:
		return cylinder_shape_create();
	case PS3DE::SHAPE_CONVEX_POLYGON:
		return convex_polygon_shape_create();
	case PS3DE::SHAPE_CONCAVE_POLYGON:
		return concave_polygon_shape_create();
	case PS3DE::SHAPE_HEIGHTMAP:
		return heightmap_shape_create();
	case PS3DE::SHAPE_CUSTOM:
		return custom_shape_create();
	default:
		return RID();
	}
}

void PhysicsServer3D::_bind_methods() {}

PhysicsServer3D::PhysicsServer3D()
{
	singleton = this;

	// World3D physics space
	GLOBAL_DEF_BASIC(PropertyInfo(Variant::FLOAT, "physics/3d/default_gravity", PROPERTY_HINT_RANGE,
						 U"-32,32,0.001,or_less,or_greater,suffix:m/s\u00B2"),
		9.8);
	GLOBAL_DEF_BASIC(PropertyInfo(Variant::VECTOR3, "physics/3d/default_gravity_vector",
						 PROPERTY_HINT_RANGE, "-10,10,0.001,or_less,or_greater"),
		Vector3(0, -1, 0));
	GLOBAL_DEF(PropertyInfo(Variant::FLOAT, "physics/3d/default_linear_damp", PROPERTY_HINT_RANGE,
				   "0,100,0.001,or_greater"),
		0.1);
	GLOBAL_DEF(PropertyInfo(Variant::FLOAT, "physics/3d/default_angular_damp", PROPERTY_HINT_RANGE,
				   "0,100,0.001,or_greater"),
		0.1);

	// PhysicsServer3D
	GLOBAL_DEF(PropertyInfo(Variant::FLOAT, "physics/3d/sleep_threshold_linear",
				   PROPERTY_HINT_RANGE, "0,1,0.001,or_greater"),
		0.1);
	GLOBAL_DEF(PropertyInfo(Variant::FLOAT, "physics/3d/sleep_threshold_angular",
				   PROPERTY_HINT_RANGE, "0,90,0.1,radians_as_degrees"),
		Math::deg_to_rad(8.0));
	GLOBAL_DEF(PropertyInfo(Variant::FLOAT, "physics/3d/time_before_sleep", PROPERTY_HINT_RANGE,
				   "0,5,0.01,or_greater"),
		0.5);
	GLOBAL_DEF(PropertyInfo(Variant::INT, "physics/3d/solver/solver_iterations",
				   PROPERTY_HINT_RANGE, "1,32,1,or_greater"),
		16);
	GLOBAL_DEF(PropertyInfo(Variant::FLOAT, "physics/3d/solver/contact_recycle_radius",
				   PROPERTY_HINT_RANGE, "0,0.1,0.001,or_greater"),
		0.01);
	GLOBAL_DEF(PropertyInfo(Variant::FLOAT, "physics/3d/solver/contact_max_separation",
				   PROPERTY_HINT_RANGE, "0,0.1,0.001,or_greater"),
		0.05);
	GLOBAL_DEF(PropertyInfo(Variant::FLOAT, "physics/3d/solver/contact_max_allowed_penetration",
				   PROPERTY_HINT_RANGE, "0.001,0.1,0.001,or_greater"),
		0.01);
	GLOBAL_DEF(PropertyInfo(Variant::FLOAT, "physics/3d/solver/default_contact_bias",
				   PROPERTY_HINT_RANGE, "0,1,0.01"),
		0.8);
}

PhysicsServer3D::~PhysicsServer3D() { singleton = nullptr; }

PhysicsServer3DManager* PhysicsServer3DManager::singleton = nullptr;
const String PhysicsServer3DManager::setting_property_name(PNAME("physics/3d/physics_engine"));

void PhysicsServer3DManager::on_servers_changed()
{
	String physics_servers2("DEFAULT");
	for (int i = get_servers_count() - 1; 0 <= i; --i) {
		physics_servers2 += "," + get_server_name(i);
	}
	ProjectSettings::get_singleton()->set_custom_property_info(
		PropertyInfo(Variant::STRING, setting_property_name, PROPERTY_HINT_ENUM, physics_servers2));
	ProjectSettings::get_singleton()->set_restart_if_changed(setting_property_name, true);
	ProjectSettings::get_singleton()->set_as_basic(setting_property_name, true);
}

void PhysicsServer3DManager::_bind_methods() {}

PhysicsServer3DManager* PhysicsServer3DManager::get_singleton() { return singleton; }

void PhysicsServer3DManager::register_server(
	const String& p_name, const Callable& p_create_callback)
{
	// ERR_FAIL_COND(!p_create_callback.is_valid());
	ERR_FAIL_COND(find_server_id(p_name) != -1);
	physics_servers.push_back(ClassInfo(p_name, p_create_callback));
	on_servers_changed();
}

void PhysicsServer3DManager::set_default_server(const String& p_name, int p_priority)
{
	const int id = find_server_id(p_name);
	ERR_FAIL_COND(id == -1); // Not found
	if (default_server_priority < p_priority) {
		default_server_id = id;
		default_server_priority = p_priority;
	}
}

int PhysicsServer3DManager::find_server_id(const String& p_name)
{
	for (int i = physics_servers.size() - 1; 0 <= i; --i) {
		if (p_name == physics_servers[i].name) {
			return i;
		}
	}
	return -1;
}

int PhysicsServer3DManager::get_servers_count() { return physics_servers.size(); }

String PhysicsServer3DManager::get_server_name(int p_id)
{
	ERR_FAIL_INDEX_V(p_id, get_servers_count(), "");
	return physics_servers[p_id].name;
}

PhysicsServer3D* PhysicsServer3DManager::new_default_server()
{
	if (default_server_id == -1) {
		return nullptr;
	}
	Variant ret;
	Callable::CallError ce;
	physics_servers[default_server_id].create_callback.callp(nullptr, 0, ret, ce);
	ERR_FAIL_COND_V(ce.error != Callable::CallError::CALL_OK, nullptr);
	return Object::cast_to<PhysicsServer3D>(ret.get_validated_object());
}

PhysicsServer3D* PhysicsServer3DManager::new_server(const String& p_name)
{
	int id = find_server_id(p_name);
	if (id == -1) {
		return nullptr;
	}
	else {
		Variant ret;
		Callable::CallError ce;
		physics_servers[id].create_callback.callp(nullptr, 0, ret, ce);
		ERR_FAIL_COND_V(ce.error != Callable::CallError::CALL_OK, nullptr);
		return Object::cast_to<PhysicsServer3D>(ret.get_validated_object());
	}
}

PhysicsServer3DManager::PhysicsServer3DManager() { singleton = this; }

PhysicsServer3DManager::~PhysicsServer3DManager() { singleton = nullptr; }


