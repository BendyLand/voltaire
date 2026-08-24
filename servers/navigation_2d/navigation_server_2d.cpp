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
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "navigation_server_2d.compat.inc"
#include "navigation_server_2d.h"
#include "scene/main/node.h" // IWYU pragma: keep. Needed to bind `Node *` arg.
#include "servers/navigation_2d/navigation_server_2d_dummy.h"

NavigationServer2D* NavigationServer2D::singleton = nullptr;

RWLock NavigationServer2D::geometry_parser_rwlock;
RID_Owner<NavMeshGeometryParser2D> NavigationServer2D::geometry_parser_owner;
LocalVector<NavMeshGeometryParser2D*> NavigationServer2D::generator_parsers;

void NavigationServer2D::_bind_methods() {}

NavigationServer2D* NavigationServer2D::get_singleton() { return singleton; }

NavigationServer2D::NavigationServer2D()
{
	ERR_FAIL_COND(singleton != nullptr);
	singleton = this;

	GLOBAL_DEF_BASIC(PropertyInfo(Variant::FLOAT, "navigation/2d/default_cell_size",
						 PROPERTY_HINT_RANGE, NavigationDefaults2D::NAV_MESH_CELL_SIZE_HINT),
		NavigationDefaults2D::NAV_MESH_CELL_SIZE);
	GLOBAL_DEF("navigation/2d/use_edge_connections", true);
	GLOBAL_DEF(PropertyInfo(Variant::FLOAT, "navigation/2d/merge_rasterizer_cell_scale",
				   PROPERTY_HINT_RANGE, "0.001,1,0.001,or_greater"),
		1.0);
	GLOBAL_DEF_BASIC(PropertyInfo(Variant::FLOAT, "navigation/2d/default_edge_connection_margin",
						 PROPERTY_HINT_RANGE, "0.01,10,0.001,or_greater"),
		NavigationDefaults2D::EDGE_CONNECTION_MARGIN);
	GLOBAL_DEF_BASIC(PropertyInfo(Variant::FLOAT, "navigation/2d/default_link_connection_radius",
						 PROPERTY_HINT_RANGE, "0.01,10,0.001,or_greater"),
		NavigationDefaults2D::LINK_CONNECTION_RADIUS);

#ifdef DEBUG_ENABLED
	debug_navigation_edge_connection_color =
		GLOBAL_DEF("debug/shapes/navigation/2d/edge_connection_color", Color(1.0, 0.0, 1.0, 1.0));
	debug_navigation_geometry_edge_color =
		GLOBAL_DEF("debug/shapes/navigation/2d/geometry_edge_color", Color(0.5, 1.0, 1.0, 1.0));
	debug_navigation_geometry_face_color =
		GLOBAL_DEF("debug/shapes/navigation/2d/geometry_face_color", Color(0.5, 1.0, 1.0, 0.4));
	debug_navigation_geometry_edge_disabled_color = GLOBAL_DEF(
		"debug/shapes/navigation/2d/geometry_edge_disabled_color", Color(0.5, 0.5, 0.5, 1.0));
	debug_navigation_geometry_face_disabled_color = GLOBAL_DEF(
		"debug/shapes/navigation/2d/geometry_face_disabled_color", Color(0.5, 0.5, 0.5, 0.4));
	debug_navigation_link_connection_color =
		GLOBAL_DEF("debug/shapes/navigation/2d/link_connection_color", Color(1.0, 0.5, 1.0, 1.0));
	debug_navigation_link_connection_disabled_color = GLOBAL_DEF(
		"debug/shapes/navigation/2d/link_connection_disabled_color", Color(0.5, 0.5, 0.5, 1.0));
	debug_navigation_agent_path_color =
		GLOBAL_DEF("debug/shapes/navigation/2d/agent_path_color", Color(1.0, 0.0, 0.0, 1.0));

	debug_navigation_enable_edge_connections =
		GLOBAL_DEF("debug/shapes/navigation/2d/enable_edge_connections", true);
	debug_navigation_enable_edge_lines =
		GLOBAL_DEF("debug/shapes/navigation/2d/enable_edge_lines", true);
	debug_navigation_enable_geometry_face_random_color =
		GLOBAL_DEF("debug/shapes/navigation/2d/enable_geometry_face_random_color", true);
	debug_navigation_enable_link_connections =
		GLOBAL_DEF("debug/shapes/navigation/2d/enable_link_connections", true);

	debug_navigation_enable_agent_paths =
		GLOBAL_DEF("debug/shapes/navigation/2d/enable_agent_paths", true);
	debug_navigation_agent_path_point_size =
		GLOBAL_DEF(PropertyInfo(Variant::FLOAT, "debug/shapes/navigation/2d/agent_path_point_size",
					   PROPERTY_HINT_RANGE, "0.01,10,0.001,or_greater"),
			4.0);

	debug_navigation_avoidance_agents_radius_color =
		GLOBAL_DEF("debug/shapes/avoidance/2d/agents_radius_color", Color(1.0, 1.0, 0.0, 0.25));
	debug_navigation_avoidance_obstacles_radius_color =
		GLOBAL_DEF("debug/shapes/avoidance/2d/obstacles_radius_color", Color(1.0, 0.5, 0.0, 0.25));
	debug_navigation_avoidance_static_obstacle_pushin_face_color = GLOBAL_DEF(
		"debug/shapes/avoidance/2d/obstacles_static_face_pushin_color", Color(1.0, 0.0, 0.0, 0.0));
	debug_navigation_avoidance_static_obstacle_pushin_edge_color = GLOBAL_DEF(
		"debug/shapes/avoidance/2d/obstacles_static_edge_pushin_color", Color(1.0, 0.0, 0.0, 1.0));
	debug_navigation_avoidance_static_obstacle_pushout_face_color = GLOBAL_DEF(
		"debug/shapes/avoidance/2d/obstacles_static_face_pushout_color", Color(1.0, 1.0, 0.0, 0.5));
	debug_navigation_avoidance_static_obstacle_pushout_edge_color = GLOBAL_DEF(
		"debug/shapes/avoidance/2d/obstacles_static_edge_pushout_color", Color(1.0, 1.0, 0.0, 1.0));
	debug_navigation_avoidance_enable_agents_radius =
		GLOBAL_DEF("debug/shapes/avoidance/2d/enable_agents_radius", true);
	debug_navigation_avoidance_enable_obstacles_radius =
		GLOBAL_DEF("debug/shapes/avoidance/2d/enable_obstacles_radius", true);
	debug_navigation_avoidance_enable_obstacles_static =
		GLOBAL_DEF("debug/shapes/avoidance/2d/enable_obstacles_static", true);

	if (Engine::get_singleton()->is_editor_hint()) {
		// Enable NavigationServer2D when in Editor or navigation mesh edge connections are
		// invisible. On runtime tests SceneTree has "Visible Navigation" set and main iteration
		// takes care of this.
		set_debug_enabled(true);
		set_debug_navigation_enabled(true);
		set_debug_avoidance_enabled(true);
	}
#endif // DEBUG_ENABLED
}

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

void NavigationServer2D::source_geometry_parser_set_callback(
	RID p_parser, const Callable& p_callback)
{
	RWLockWrite write_lock(geometry_parser_rwlock);

	NavMeshGeometryParser2D* parser = geometry_parser_owner.get_or_null(p_parser);
	ERR_FAIL_NULL(parser);

	parser->callback = p_callback;
}

void NavigationServer2D::set_debug_enabled(bool p_enabled)
{
#ifdef DEBUG_ENABLED
	if (debug_enabled != p_enabled) {
		debug_dirty = true;
	}

	debug_enabled = p_enabled;

	if (debug_dirty) {
		navigation_debug_dirty = true;
		callable_mp(this, &NavigationServer2D::_emit_navigation_debug_changed_signal)
			.call_deferred();

		avoidance_debug_dirty = true;
		callable_mp(this, &NavigationServer2D::_emit_avoidance_debug_changed_signal)
			.call_deferred();
	}
#endif // DEBUG_ENABLED
}

bool NavigationServer2D::get_debug_enabled() const { return debug_enabled; }

#ifdef DEBUG_ENABLED
void NavigationServer2D::_emit_navigation_debug_changed_signal()
{
	if (navigation_debug_dirty) {
		navigation_debug_dirty = false;
		this->obj->emit_signal(SNAME("navigation_debug_changed"));
	}
}

void NavigationServer2D::_emit_avoidance_debug_changed_signal()
{
	if (avoidance_debug_dirty) {
		avoidance_debug_dirty = false;
		this->obj->emit_signal(SNAME("avoidance_debug_changed"));
	}
}
#endif // DEBUG_ENABLED

#ifdef DEBUG_ENABLED
void NavigationServer2D::set_debug_navigation_enabled(bool p_enabled)
{
	debug_navigation_enabled = p_enabled;
	navigation_debug_dirty = true;
	callable_mp(this, &NavigationServer2D::_emit_navigation_debug_changed_signal).call_deferred();
}

bool NavigationServer2D::get_debug_navigation_enabled() const { return debug_navigation_enabled; }

void NavigationServer2D::set_debug_avoidance_enabled(bool p_enabled)
{
	debug_avoidance_enabled = p_enabled;
	avoidance_debug_dirty = true;
	callable_mp(this, &NavigationServer2D::_emit_avoidance_debug_changed_signal).call_deferred();
}

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

void NavigationServer2DManager::initialize_server()
{
	ERR_FAIL_COND(navigation_server_2d != nullptr);

	// Init 2D Navigation Server.
	navigation_server_2d = NavigationServer2DManager::get_singleton()->new_server(
		GLOBAL_GET(NavigationServer2DManager::setting_property_name));
	if (!navigation_server_2d) {
		// Navigation server not found, use the default.
		navigation_server_2d = NavigationServer2DManager::get_singleton()->new_default_server();
	}

	// Fall back to dummy if no default server has been registered.
	if (!navigation_server_2d) {
		WARN_VERBOSE("Failed to initialize NavigationServer2D. Fall back to dummy server.");
		navigation_server_2d = memnew(NavigationServer2DDummy);
	}

	// Should be impossible, but make sure it's not null.
	ERR_FAIL_NULL_MSG(navigation_server_2d, "Failed to initialize NavigationServer2D.");
	navigation_server_2d->init();
}

void NavigationServer2DManager::finalize_server()
{
	ERR_FAIL_NULL(navigation_server_2d);
	navigation_server_2d->finish();
	memdelete(navigation_server_2d);
	navigation_server_2d = nullptr;
}

const String NavigationServer2DManager::setting_property_name(
	PNAME("navigation/2d/navigation_engine"));

void NavigationServer2DManager::on_servers_changed()
{
	String navigation_servers_enum_str("DEFAULT");
	for (int i = get_servers_count() - 1; 0 <= i; --i) {
		navigation_servers_enum_str += "," + get_server_name(i);
	}
	ProjectSettings::get_singleton()->set_custom_property_info(PropertyInfo(
		Variant::STRING, setting_property_name, PROPERTY_HINT_ENUM, navigation_servers_enum_str));
	ProjectSettings::get_singleton()->set_restart_if_changed(setting_property_name, true);
	ProjectSettings::get_singleton()->set_as_basic(setting_property_name, true);
}

void NavigationServer2DManager::_bind_methods() {}

NavigationServer2DManager* NavigationServer2DManager::get_singleton() { return singleton; }

void NavigationServer2DManager::register_server(
	const String& p_name, const Callable& p_create_callback)
{
	ERR_FAIL_COND(find_server_id(p_name) != -1);
	navigation_servers.push_back(ClassInfo(p_name, p_create_callback));
	on_servers_changed();
}

void NavigationServer2DManager::set_default_server(const String& p_name, int p_priority)
{
	const int id = find_server_id(p_name);
	ERR_FAIL_COND(id == -1); // Not found
	if (default_server_priority < p_priority) {
		default_server_id = id;
		default_server_priority = p_priority;
	}
}

int NavigationServer2DManager::find_server_id(const String& p_name)
{
	for (int i = navigation_servers.size() - 1; 0 <= i; --i) {
		if (p_name == navigation_servers[i].name) {
			return i;
		}
	}
	return -1;
}

int NavigationServer2DManager::get_servers_count() { return navigation_servers.size(); }

String NavigationServer2DManager::get_server_name(int p_id)
{
	ERR_FAIL_INDEX_V(p_id, get_servers_count(), "");
	return navigation_servers[p_id].name;
}

NavigationServer2D* NavigationServer2DManager::new_default_server()
{
	if (default_server_id == -1) {
		return nullptr;
	}
	Variant ret;
	Callable::CallError ce;
	navigation_servers[default_server_id].create_callback.callp(nullptr, 0, ret, ce);
	ERR_FAIL_COND_V(ce.error != Callable::CallError::CALL_OK, nullptr);
	return Object::cast_to<NavigationServer2D>(ret.get_validated_object());
}

NavigationServer2D* NavigationServer2DManager::new_server(const String& p_name)
{
	int id = find_server_id(p_name);
	if (id == -1) {
		return nullptr;
	}
	else {
		Variant ret;
		Callable::CallError ce;
		navigation_servers[id].create_callback.callp(nullptr, 0, ret, ce);
		ERR_FAIL_COND_V(ce.error != Callable::CallError::CALL_OK, nullptr);
		return Object::cast_to<NavigationServer2D>(ret.get_validated_object());
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


