/**************************************************************************/
/*  world_3d.cpp                                                          */
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
#include "scene/3d/camera_3d.h"
#include "scene/resources/camera_attributes.h"
#include "scene/resources/environment.h"
#include "servers/rendering/rendering_server.h"
#include "world_3d.h"

#ifndef NAVIGATION_3D_DISABLED
#include "servers/navigation_3d/navigation_server_3d.h"
#endif // NAVIGATION_3D_DISABLED

void World3D::_register_camera(Camera3D* p_camera) { cameras.insert(p_camera); }

void World3D::_remove_camera(Camera3D* p_camera) { cameras.erase(p_camera); }

RID World3D::get_scenario() const { return scenario; }

void World3D::set_environment(const Ref<Environment>& p_environment)
{
	if (environment == p_environment) {
		return;
	}

	environment = p_environment;
	if (environment.is_valid()) {
		RS::get_singleton()->scenario_set_environment(scenario, environment->get_rid());
	}
	else {
		RS::get_singleton()->scenario_set_environment(scenario, RID());
	}

	emit_changed();
}

Ref<Environment> World3D::get_environment() const { return environment; }

void World3D::set_fallback_environment(const Ref<Environment>& p_environment)
{
	if (fallback_environment == p_environment) {
		return;
	}

	fallback_environment = p_environment;
	if (fallback_environment.is_valid()) {
		RS::get_singleton()->scenario_set_fallback_environment(scenario, p_environment->get_rid());
	}
	else {
		RS::get_singleton()->scenario_set_fallback_environment(scenario, RID());
	}

	emit_changed();
}

Ref<Environment> World3D::get_fallback_environment() const { return fallback_environment; }

void World3D::set_camera_attributes(const Ref<CameraAttributes>& p_camera_attributes)
{
	camera_attributes = p_camera_attributes;
	if (camera_attributes.is_valid()) {
		RS::get_singleton()->scenario_set_camera_attributes(scenario, camera_attributes->get_rid());
	}
	else {
		RS::get_singleton()->scenario_set_camera_attributes(scenario, RID());
	}
}

Ref<CameraAttributes> World3D::get_camera_attributes() const { return camera_attributes; }

void World3D::set_compositor(const Ref<Compositor>& p_compositor)
{
	compositor = p_compositor;
	if (compositor.is_valid()) {
		RS::get_singleton()->scenario_set_compositor(scenario, compositor->get_rid());
	}
	else {
		RS::get_singleton()->scenario_set_compositor(scenario, RID());
	}
}

Ref<Compositor> World3D::get_compositor() const { return compositor; }

#ifndef PHYSICS_3D_DISABLED
PhysicsDirectSpaceState3D* World3D::get_direct_space_state()
{
	return PhysicsServer3D::get_singleton()->space_get_direct_state(get_space());
}
#endif // PHYSICS_3D_DISABLED

void World3D::_bind_methods() {}

World3D::World3D() { scenario = RenderingServer::get_singleton()->scenario_create(); }

World3D::~World3D()
{
	ERR_FAIL_NULL(RenderingServer::get_singleton());

#ifndef PHYSICS_3D_DISABLED
	ERR_FAIL_NULL(PhysicsServer3D::get_singleton());
#endif // PHYSICS_3D_DISABLED

#ifndef NAVIGATION_3D_DISABLED
	ERR_FAIL_NULL(NavigationServer3D::get_singleton());
#endif // NAVIGATION_3D_DISABLED

	RenderingServer::get_singleton()->free_rid(scenario);

#ifndef PHYSICS_3D_DISABLED
	if (space.is_valid()) {
		PhysicsServer3D::get_singleton()->free_rid(space);
	}
#endif // PHYSICS_3D_DISABLED

#ifndef NAVIGATION_3D_DISABLED
	if (navigation_map.is_valid()) {
		NavigationServer3D::get_singleton()->free_rid(navigation_map);
	}
#endif // NAVIGATION_3D_DISABLED
}


