/**************************************************************************/
/*  navigation_server_2d.cpp                                              */
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

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "navigation_server_2d.compat.inc"
#include "navigation_server_2d.h"
#include "scene/main/node.h" // IWYU pragma: keep. Needed to bind `Node *` arg.
#include "servers/navigation_2d/navigation_server_2d_dummy.h"

NavigationServer2D* NavigationServer2D::singleton = nullptr;

RWLock NavigationServer2D::geometry_parser_rwlock;
RID_Owner<NavMeshGeometryParser2D> NavigationServer2D::geometry_parser_owner;
LocalVector<NavMeshGeometryParser2D*> NavigationServer2D::generator_parsers;

NavigationServer2D* NavigationServer2D::get_singleton() { return singleton; }

NavigationServer2D::~NavigationServer2D()
{
	singleton = nullptr;

	RWLockWrite write_lock(geometry_parser_rwlock);
	for (NavMeshGeometryParser2D* parser : generator_parsers) {
		geometry_parser_owner.free(parser->self);
	}
	generator_parsers.clear();
}

RID NavigationServer2D::source_geometry_parser_create()
{
	RWLockWrite write_lock(geometry_parser_rwlock);

	RID rid = geometry_parser_owner.make_rid();

	NavMeshGeometryParser2D* parser = geometry_parser_owner.get_or_null(rid);
	parser->self = rid;

	generator_parsers.push_back(parser);

	return rid;
}

void NavigationServer2D::free_rid(RID p_rid)
{
	if (!geometry_parser_owner.owns(p_rid)) {
		return;
	}
	RWLockWrite write_lock(geometry_parser_rwlock);

	NavMeshGeometryParser2D* parser = geometry_parser_owner.get_or_null(p_rid);
	ERR_FAIL_NULL(parser);

	generator_parsers.erase(parser);
	geometry_parser_owner.free(parser->self);
}

bool NavigationServer2D::get_debug_enabled() const { return debug_enabled; }

#ifdef DEBUG_ENABLED

bool NavigationServer2D::get_debug_navigation_enabled() const { return debug_navigation_enabled; }

bool NavigationServer2D::get_debug_avoidance_enabled() const { return debug_avoidance_enabled; }

void NavigationServer2D::set_debug_navigation_edge_connection_color(const Color& p_color)
{
	debug_navigation_edge_connection_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_edge_connection_color() const
{
	return debug_navigation_edge_connection_color;
}

void NavigationServer2D::set_debug_navigation_geometry_face_color(const Color& p_color)
{
	debug_navigation_geometry_face_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_geometry_face_color() const
{
	return debug_navigation_geometry_face_color;
}

void NavigationServer2D::set_debug_navigation_geometry_face_disabled_color(const Color& p_color)
{
	debug_navigation_geometry_face_disabled_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_geometry_face_disabled_color() const
{
	return debug_navigation_geometry_face_disabled_color;
}

void NavigationServer2D::set_debug_navigation_link_connection_color(const Color& p_color)
{
	debug_navigation_link_connection_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_link_connection_color() const
{
	return debug_navigation_link_connection_color;
}

void NavigationServer2D::set_debug_navigation_link_connection_disabled_color(const Color& p_color)
{
	debug_navigation_link_connection_disabled_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_link_connection_disabled_color() const
{
	return debug_navigation_link_connection_disabled_color;
}

void NavigationServer2D::set_debug_navigation_geometry_edge_color(const Color& p_color)
{
	debug_navigation_geometry_edge_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_geometry_edge_color() const
{
	return debug_navigation_geometry_edge_color;
}

void NavigationServer2D::set_debug_navigation_geometry_edge_disabled_color(const Color& p_color)
{
	debug_navigation_geometry_edge_disabled_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_geometry_edge_disabled_color() const
{
	return debug_navigation_geometry_edge_disabled_color;
}

void NavigationServer2D::set_debug_navigation_enable_edge_connections(const bool p_value)
{
	debug_navigation_enable_edge_connections = p_value;
}

bool NavigationServer2D::get_debug_navigation_enable_edge_connections() const
{
	return debug_navigation_enable_edge_connections;
}

void NavigationServer2D::set_debug_navigation_enable_geometry_face_random_color(const bool p_value)
{
	debug_navigation_enable_geometry_face_random_color = p_value;
}

bool NavigationServer2D::get_debug_navigation_enable_geometry_face_random_color() const
{
	return debug_navigation_enable_geometry_face_random_color;
}

void NavigationServer2D::set_debug_navigation_enable_edge_lines(const bool p_value)
{
	debug_navigation_enable_edge_lines = p_value;
}

bool NavigationServer2D::get_debug_navigation_enable_edge_lines() const
{
	return debug_navigation_enable_edge_lines;
}

void NavigationServer2D::set_debug_navigation_agent_path_color(const Color& p_color)
{
	debug_navigation_agent_path_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_agent_path_color() const
{
	return debug_navigation_agent_path_color;
}

void NavigationServer2D::set_debug_navigation_enable_agent_paths(const bool p_value)
{
	debug_navigation_enable_agent_paths = p_value;
}

bool NavigationServer2D::get_debug_navigation_enable_agent_paths() const
{
	return debug_navigation_enable_agent_paths;
}

void NavigationServer2D::set_debug_navigation_agent_path_point_size(real_t p_point_size)
{
	debug_navigation_agent_path_point_size = p_point_size;
}

real_t NavigationServer2D::get_debug_navigation_agent_path_point_size() const
{
	return debug_navigation_agent_path_point_size;
}

void NavigationServer2D::set_debug_navigation_avoidance_enable_agents_radius(const bool p_value)
{
	debug_navigation_avoidance_enable_agents_radius = p_value;
}

bool NavigationServer2D::get_debug_navigation_avoidance_enable_agents_radius() const
{
	return debug_navigation_avoidance_enable_agents_radius;
}

void NavigationServer2D::set_debug_navigation_avoidance_enable_obstacles_radius(const bool p_value)
{
	debug_navigation_avoidance_enable_obstacles_radius = p_value;
}

bool NavigationServer2D::get_debug_navigation_avoidance_enable_obstacles_radius() const
{
	return debug_navigation_avoidance_enable_obstacles_radius;
}

void NavigationServer2D::set_debug_navigation_avoidance_agents_radius_color(const Color& p_color)
{
	debug_navigation_avoidance_agents_radius_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_avoidance_agents_radius_color() const
{
	return debug_navigation_avoidance_agents_radius_color;
}

void NavigationServer2D::set_debug_navigation_avoidance_obstacles_radius_color(const Color& p_color)
{
	debug_navigation_avoidance_obstacles_radius_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_avoidance_obstacles_radius_color() const
{
	return debug_navigation_avoidance_obstacles_radius_color;
}

void NavigationServer2D::set_debug_navigation_avoidance_static_obstacle_pushin_face_color(
	const Color& p_color)
{
	debug_navigation_avoidance_static_obstacle_pushin_face_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_avoidance_static_obstacle_pushin_face_color() const
{
	return debug_navigation_avoidance_static_obstacle_pushin_face_color;
}

void NavigationServer2D::set_debug_navigation_avoidance_static_obstacle_pushout_face_color(
	const Color& p_color)
{
	debug_navigation_avoidance_static_obstacle_pushout_face_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_avoidance_static_obstacle_pushout_face_color() const
{
	return debug_navigation_avoidance_static_obstacle_pushout_face_color;
}

void NavigationServer2D::set_debug_navigation_avoidance_static_obstacle_pushin_edge_color(
	const Color& p_color)
{
	debug_navigation_avoidance_static_obstacle_pushin_edge_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_avoidance_static_obstacle_pushin_edge_color() const
{
	return debug_navigation_avoidance_static_obstacle_pushin_edge_color;
}

void NavigationServer2D::set_debug_navigation_avoidance_static_obstacle_pushout_edge_color(
	const Color& p_color)
{
	debug_navigation_avoidance_static_obstacle_pushout_edge_color = p_color;
}

Color NavigationServer2D::get_debug_navigation_avoidance_static_obstacle_pushout_edge_color() const
{
	return debug_navigation_avoidance_static_obstacle_pushout_edge_color;
}

void NavigationServer2D::set_debug_navigation_avoidance_enable_obstacles_static(const bool p_value)
{
	debug_navigation_avoidance_enable_obstacles_static = p_value;
}

bool NavigationServer2D::get_debug_navigation_avoidance_enable_obstacles_static() const
{
	return debug_navigation_avoidance_enable_obstacles_static;
}
#endif // DEBUG_ENABLED

///////////////////////////////////////////////////////

static NavigationServer2D* navigation_server_2d = nullptr;

void NavigationServer2DManager::finalize_server()
{
	ERR_FAIL_NULL(navigation_server_2d);
	navigation_server_2d->finish();
	memdelete(navigation_server_2d);
	navigation_server_2d = nullptr;
}

const String NavigationServer2DManager::setting_property_name(
	PNAME("navigation/2d/navigation_engine"));

NavigationServer2DManager* NavigationServer2DManager::get_singleton() { return singleton; }

void NavigationServer2DManager::set_default_server(const String& p_name, int p_priority)
{
	const int id = find_server_id(p_name);
	ERR_FAIL_COND(id == -1); // Not found
	if (default_server_priority < p_priority) {
		default_server_id = id;
		default_server_priority = p_priority;
	}
}

NavigationServer2D* NavigationServer2DManager::create_dummy_server_callback()
{
	return memnew(NavigationServer2DDummy);
}

NavigationServer2DManager::NavigationServer2DManager() {}

NavigationServer2DManager::~NavigationServer2DManager() {}

void NavigationServer2DManager::initialize_server_manager()
{
	ERR_FAIL_COND(singleton != nullptr);
	singleton = memnew(NavigationServer2DManager);
}

void NavigationServer2DManager::finalize_server_manager()
{
	ERR_FAIL_NULL(singleton);
	memdelete(singleton);
}


