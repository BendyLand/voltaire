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
#include "physics_server_3d.h"

PhysicsServer3D* PhysicsServer3D::singleton = nullptr;

PhysicsServer3D* PhysicsServer3D::get_singleton() { return singleton; }

bool PhysicsServer3D::_body_test_motion(RID p_body, PhysicsTestMotionParameters3D* rp_parameters,
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

PhysicsServer3D::~PhysicsServer3D() { singleton = nullptr; }

PhysicsServer3DManager* PhysicsServer3DManager::singleton = nullptr;
const String PhysicsServer3DManager::setting_property_name(PNAME("physics/3d/physics_engine"));

PhysicsServer3DManager* PhysicsServer3DManager::get_singleton() { return singleton; }

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

PhysicsServer3DManager::PhysicsServer3DManager() { singleton = this; }

PhysicsServer3DManager::~PhysicsServer3DManager() { singleton = nullptr; }


