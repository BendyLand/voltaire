/**************************************************************************/
/*  navigation_server_3d.cpp                                              */
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
#include "navigation_server_3d.compat.inc"
#include "navigation_server_3d.h"
#include "scene/main/node.h" // IWYU pragma: keep. Needed to bind `Node *` arg.
#include "servers/navigation_3d/navigation_server_3d_dummy.h"

NavigationServer3D* NavigationServer3D::singleton = nullptr;

RWLock NavigationServer3D::geometry_parser_rwlock;
RID_Owner<NavMeshGeometryParser3D> NavigationServer3D::geometry_parser_owner;
LocalVector<NavMeshGeometryParser3D*> NavigationServer3D::generator_parsers;

void NavigationServer3D::_bind_methods() {}

NavigationServer3D* NavigationServer3D::get_singleton() { return singleton; }

NavigationServer3D::NavigationServer3D()
{
	ERR_FAIL_COND(singleton != nullptr);
	singleton = this;

	GLOBAL_DEF_BASIC(PropertyInfo(Variant::FLOAT, "navigation/3d/default_cell_size",
						 PROPERTY_HINT_RANGE, NavigationDefaults3D::NAV_MESH_CELL_SIZE_HINT),
		NavigationDefaults3D::NAV_MESH_CELL_SIZE);
	GLOBAL_DEF_BASIC(PropertyInfo(Variant::FLOAT, "navigation/3d/default_cell_height",
						 PROPERTY_HINT_RANGE, "0.001,100,0.001,or_greater"),
		NavigationDefaults3D::NAV_MESH_CELL_HEIGHT);
	GLOBAL_DEF("navigation/3d/default_up", Vector3(0, 1, 0));
	GLOBAL_DEF(PropertyInfo(Variant::FLOAT, "navigation/3d/merge_rasterizer_cell_scale",
				   PROPERTY_HINT_RANGE, "0.001,1,0.001,or_greater"),
		1.0);
	GLOBAL_DEF("navigation/3d/use_edge_connections", true);
	GLOBAL_DEF_BASIC(PropertyInfo(Variant::FLOAT, "navigation/3d/default_edge_connection_margin",
						 PROPERTY_HINT_RANGE, "0.01,10,0.001,or_greater"),
		NavigationDefaults3D::EDGE_CONNECTION_MARGIN);
	GLOBAL_DEF_BASIC(PropertyInfo(Variant::FLOAT, "navigation/3d/default_link_connection_radius",
						 PROPERTY_HINT_RANGE, "0.01,10,0.001,or_greater"),
		NavigationDefaults3D::LINK_CONNECTION_RADIUS);

#ifdef DEBUG_ENABLED
#ifndef DISABLE_DEPRECATED
#define MOVE_PROJECT_SETTING_1(m_old_setting, m_new_setting)                                       \
	if (!ProjectSettings::get_singleton()->has_setting(m_new_setting) &&                           \
		ProjectSettings::get_singleton()->has_setting(m_old_setting)) {                            \
		Variant value = GLOBAL_GET(m_old_setting);                                                 \
		ProjectSettings::get_singleton()->set_setting(m_new_setting, value);                       \
		ProjectSettings::get_singleton()->clear(m_old_setting);                                    \
	}
#define MOVE_PROJECT_SETTING_2(m_old_setting, m_new_setting_1, m_new_setting_2)                    \
	if ((!ProjectSettings::get_singleton()->has_setting(m_new_setting_1) ||                        \
			!ProjectSettings::get_singleton()->has_setting(m_new_setting_2)) &&                    \
		ProjectSettings::get_singleton()->has_setting(m_old_setting)) {                            \
		Variant value = GLOBAL_GET(m_old_setting);                                                 \
		if (!ProjectSettings::get_singleton()->has_setting(m_new_setting_1)) {                     \
			ProjectSettings::get_singleton()->set_setting(m_new_setting_1, value);                 \
		}                                                                                          \
		if (!ProjectSettings::get_singleton()->has_setting(m_new_setting_2)) {                     \
			ProjectSettings::get_singleton()->set_setting(m_new_setting_2, value);                 \
		}                                                                                          \
		ProjectSettings::get_singleton()->clear(m_old_setting);                                    \
	}
	MOVE_PROJECT_SETTING_2("debug/shapes/navigation/edge_connection_color",
		"debug/shapes/navigation/2d/edge_connection_color",
		"debug/shapes/navigation/3d/edge_connection_color");
	MOVE_PROJECT_SETTING_2("debug/shapes/navigation/geometry_edge_color",
		"debug/shapes/navigation/2d/geometry_edge_color",
		"debug/shapes/navigation/3d/geometry_edge_color");
	MOVE_PROJECT_SETTING_2("debug/shapes/navigation/geometry_face_color",
		"debug/shapes/navigation/2d/geometry_face_color",
		"debug/shapes/navigation/3d/geometry_face_color");
	MOVE_PROJECT_SETTING_2("debug/shapes/navigation/geometry_edge_disabled_color",
		"debug/shapes/navigation/2d/geometry_edge_disabled_color",
		"debug/shapes/navigation/3d/geometry_edge_disabled_color");
	MOVE_PROJECT_SETTING_2("debug/shapes/navigation/geometry_face_disabled_color",
		"debug/shapes/navigation/2d/geometry_face_disabled_color",
		"debug/shapes/navigation/3d/geometry_face_disabled_color");
	MOVE_PROJECT_SETTING_2("debug/shapes/navigation/link_connection_color",
		"debug/shapes/navigation/2d/link_connection_color",
		"debug/shapes/navigation/3d/link_connection_color");
	MOVE_PROJECT_SETTING_2("debug/shapes/navigation/link_connection_disabled_color",
		"debug/shapes/navigation/2d/link_connection_disabled_color",
		"debug/shapes/navigation/3d/link_connection_disabled_color");
	MOVE_PROJECT_SETTING_2("debug/shapes/navigation/agent_path_color",
		"debug/shapes/navigation/2d/agent_path_color",
		"debug/shapes/navigation/3d/agent_path_color");

	MOVE_PROJECT_SETTING_2("debug/shapes/navigation/enable_edge_connections",
		"debug/shapes/navigation/2d/enable_edge_connections",
		"debug/shapes/navigation/3d/enable_edge_connections");
	MOVE_PROJECT_SETTING_1("debug/shapes/navigation/enable_edge_connections_xray",
		"debug/shapes/navigation/3d/enable_edge_connections_xray");
	MOVE_PROJECT_SETTING_2("debug/shapes/navigation/enable_edge_lines",
		"debug/shapes/navigation/2d/enable_edge_lines",
		"debug/shapes/navigation/3d/enable_edge_lines");
	MOVE_PROJECT_SETTING_1("debug/shapes/navigation/enable_edge_lines_xray",
		"debug/shapes/navigation/3d/enable_edge_lines_xray");
	MOVE_PROJECT_SETTING_2("debug/shapes/navigation/enable_geometry_face_random_color",
		"debug/shapes/navigation/2d/enable_geometry_face_random_color",
		"debug/shapes/navigation/3d/enable_geometry_face_random_color");
	MOVE_PROJECT_SETTING_2("debug/shapes/navigation/enable_link_connections",
		"debug/shapes/navigation/2d/enable_link_connections",
		"debug/shapes/navigation/3d/enable_link_connections");
	MOVE_PROJECT_SETTING_1("debug/shapes/navigation/enable_link_connections_xray",
		"debug/shapes/navigation/3d/enable_link_connections_xray");

	MOVE_PROJECT_SETTING_2("debug/shapes/navigation/enable_agent_paths",
		"debug/shapes/navigation/2d/enable_agent_paths",
		"debug/shapes/navigation/3d/enable_agent_paths");
	MOVE_PROJECT_SETTING_1("debug/shapes/navigation/enable_agent_paths_xray",
		"debug/shapes/navigation/3d/enable_agent_paths_xray");
	MOVE_PROJECT_SETTING_2("debug/shapes/navigation/agent_path_point_size",
		"debug/shapes/navigation/2d/agent_path_point_size",
		"debug/shapes/navigation/3d/agent_path_point_size");

	MOVE_PROJECT_SETTING_2("debug/shapes/avoidance/agents_radius_color",
		"debug/shapes/avoidance/2d/agents_radius_color",
		"debug/shapes/avoidance/3d/agents_radius_color");
	MOVE_PROJECT_SETTING_2("debug/shapes/avoidance/obstacles_radius_color",
		"debug/shapes/avoidance/2d/obstacles_radius_color",
		"debug/shapes/avoidance/3d/obstacles_radius_color");
	MOVE_PROJECT_SETTING_2("debug/shapes/avoidance/obstacles_static_face_pushin_color",
		"debug/shapes/avoidance/2d/obstacles_static_face_pushin_color",
		"debug/shapes/avoidance/3d/obstacles_static_face_pushin_color");
	MOVE_PROJECT_SETTING_2("debug/shapes/avoidance/obstacles_static_edge_pushin_color",
		"debug/shapes/avoidance/2d/obstacles_static_edge_pushin_color",
		"debug/shapes/avoidance/3d/obstacles_static_edge_pushin_color");
	MOVE_PROJECT_SETTING_2("debug/shapes/avoidance/obstacles_static_face_pushout_color",
		"debug/shapes/avoidance/2d/obstacles_static_face_pushout_color",
		"debug/shapes/avoidance/3d/obstacles_static_face_pushout_color");
	MOVE_PROJECT_SETTING_2("debug/shapes/avoidance/obstacles_static_edge_pushout_color",
		"debug/shapes/avoidance/2d/obstacles_static_edge_pushout_color",
		"debug/shapes/avoidance/3d/obstacles_static_edge_pushout_color");
	MOVE_PROJECT_SETTING_2("debug/shapes/avoidance/enable_agents_radius",
		"debug/shapes/avoidance/2d/enable_agents_radius",
		"debug/shapes/avoidance/3d/enable_agents_radius");
	MOVE_PROJECT_SETTING_2("debug/shapes/avoidance/enable_obstacles_radius",
		"debug/shapes/avoidance/2d/enable_obstacles_radius",
		"debug/shapes/avoidance/2d/enable_obstacles_static");
	MOVE_PROJECT_SETTING_2("debug/shapes/avoidance/enable_obstacles_radius",
		"debug/shapes/avoidance/3d/enable_obstacles_radius",
		"debug/shapes/avoidance/3d/enable_obstacles_static");
#undef MOVE_PROJECT_SETTING_1
#undef MOVE_PROJECT_SETTING_2
#endif // DISABLE_DEPRECATED

	debug_navigation_edge_connection_color =
		GLOBAL_DEF("debug/shapes/navigation/3d/edge_connection_color", Color(1.0, 0.0, 1.0, 1.0));
	debug_navigation_geometry_edge_color =
		GLOBAL_DEF("debug/shapes/navigation/3d/geometry_edge_color", Color(0.5, 1.0, 1.0, 1.0));
	debug_navigation_geometry_face_color =
		GLOBAL_DEF("debug/shapes/navigation/3d/geometry_face_color", Color(0.5, 1.0, 1.0, 0.4));
	debug_navigation_geometry_edge_disabled_color = GLOBAL_DEF(
		"debug/shapes/navigation/3d/geometry_edge_disabled_color", Color(0.5, 0.5, 0.5, 1.0));
	debug_navigation_geometry_face_disabled_color = GLOBAL_DEF(
		"debug/shapes/navigation/3d/geometry_face_disabled_color", Color(0.5, 0.5, 0.5, 0.4));
	debug_navigation_link_connection_color =
		GLOBAL_DEF("debug/shapes/navigation/3d/link_connection_color", Color(1.0, 0.5, 1.0, 1.0));
	debug_navigation_link_connection_disabled_color = GLOBAL_DEF(
		"debug/shapes/navigation/3d/link_connection_disabled_color", Color(0.5, 0.5, 0.5, 1.0));
	debug_navigation_agent_path_color =
		GLOBAL_DEF("debug/shapes/navigation/3d/agent_path_color", Color(1.0, 0.0, 0.0, 1.0));

	debug_navigation_enable_edge_connections =
		GLOBAL_DEF("debug/shapes/navigation/3d/enable_edge_connections", true);
	debug_navigation_enable_edge_connections_xray =
		GLOBAL_DEF("debug/shapes/navigation/3d/enable_edge_connections_xray", true);
	debug_navigation_enable_edge_lines =
		GLOBAL_DEF("debug/shapes/navigation/3d/enable_edge_lines", true);
	debug_navigation_enable_edge_lines_xray =
		GLOBAL_DEF("debug/shapes/navigation/3d/enable_edge_lines_xray", true);
	debug_navigation_enable_geometry_face_random_color =
		GLOBAL_DEF("debug/shapes/navigation/3d/enable_geometry_face_random_color", true);
	debug_navigation_enable_link_connections =
		GLOBAL_DEF("debug/shapes/navigation/3d/enable_link_connections", true);
	debug_navigation_enable_link_connections_xray =
		GLOBAL_DEF("debug/shapes/navigation/3d/enable_link_connections_xray", true);

	debug_navigation_enable_agent_paths =
		GLOBAL_DEF("debug/shapes/navigation/3d/enable_agent_paths", true);
	debug_navigation_enable_agent_paths_xray =
		GLOBAL_DEF("debug/shapes/navigation/3d/enable_agent_paths_xray", true);
	debug_navigation_agent_path_point_size =
		GLOBAL_DEF(PropertyInfo(Variant::FLOAT, "debug/shapes/navigation/3d/agent_path_point_size",
					   PROPERTY_HINT_RANGE, "0.01,10,0.001,or_greater"),
			4.0);

	debug_navigation_avoidance_agents_radius_color =
		GLOBAL_DEF("debug/shapes/avoidance/3d/agents_radius_color", Color(1.0, 1.0, 0.0, 0.25));
	debug_navigation_avoidance_obstacles_radius_color =
		GLOBAL_DEF("debug/shapes/avoidance/3d/obstacles_radius_color", Color(1.0, 0.5, 0.0, 0.25));
	debug_navigation_avoidance_static_obstacle_pushin_face_color = GLOBAL_DEF(
		"debug/shapes/avoidance/3d/obstacles_static_face_pushin_color", Color(1.0, 0.0, 0.0, 0.0));
	debug_navigation_avoidance_static_obstacle_pushin_edge_color = GLOBAL_DEF(
		"debug/shapes/avoidance/3d/obstacles_static_edge_pushin_color", Color(1.0, 0.0, 0.0, 1.0));
	debug_navigation_avoidance_static_obstacle_pushout_face_color = GLOBAL_DEF(
		"debug/shapes/avoidance/3d/obstacles_static_face_pushout_color", Color(1.0, 1.0, 0.0, 0.5));
	debug_navigation_avoidance_static_obstacle_pushout_edge_color = GLOBAL_DEF(
		"debug/shapes/avoidance/3d/obstacles_static_edge_pushout_color", Color(1.0, 1.0, 0.0, 1.0));
	debug_navigation_avoidance_enable_agents_radius =
		GLOBAL_DEF("debug/shapes/avoidance/3d/enable_agents_radius", true);
	debug_navigation_avoidance_enable_obstacles_radius =
		GLOBAL_DEF("debug/shapes/avoidance/3d/enable_obstacles_radius", true);
	debug_navigation_avoidance_enable_obstacles_static =
		GLOBAL_DEF("debug/shapes/avoidance/3d/enable_obstacles_static", true);

	if (Engine::get_singleton()->is_editor_hint()) {
		// enable NavigationServer3D when in Editor or else navigation mesh edge connections are
		// invisible on runtime tests SceneTree has "Visible Navigation" set and main iteration
		// takes care of this
		set_debug_enabled(true);
		set_debug_navigation_enabled(true);
		set_debug_avoidance_enabled(true);
	}
#endif // DEBUG_ENABLED
}

NavigationServer3D::~NavigationServer3D()
{
	singleton = nullptr;

	RWLockWrite write_lock(geometry_parser_rwlock);
	for (NavMeshGeometryParser3D* parser : generator_parsers) {
		geometry_parser_owner.free(parser->self);
	}
	generator_parsers.clear();
}

RID NavigationServer3D::source_geometry_parser_create()
{
	RWLockWrite write_lock(geometry_parser_rwlock);

	RID rid = geometry_parser_owner.make_rid();

	NavMeshGeometryParser3D* parser = geometry_parser_owner.get_or_null(rid);
	parser->self = rid;

	generator_parsers.push_back(parser);

	return rid;
}

void NavigationServer3D::free_rid(RID p_rid)
{
	if (!geometry_parser_owner.owns(p_rid)) {
		return;
	}
	RWLockWrite write_lock(geometry_parser_rwlock);

	NavMeshGeometryParser3D* parser = geometry_parser_owner.get_or_null(p_rid);
	ERR_FAIL_NULL(parser);

	generator_parsers.erase(parser);
	geometry_parser_owner.free(parser->self);
}

void NavigationServer3D::source_geometry_parser_set_callback(
	RID p_parser, const Callable& p_callback)
{
	RWLockWrite write_lock(geometry_parser_rwlock);

	NavMeshGeometryParser3D* parser = geometry_parser_owner.get_or_null(p_parser);
	ERR_FAIL_NULL(parser);

	parser->callback = p_callback;
}

void NavigationServer3D::set_debug_enabled(bool p_enabled)
{
#ifdef DEBUG_ENABLED
	if (debug_enabled != p_enabled) {
		debug_dirty = true;
	}

	debug_enabled = p_enabled;

	if (debug_dirty) {
		navigation_debug_dirty = true;
		callable_mp(this, &NavigationServer3D::_emit_navigation_debug_changed_signal)
			.call_deferred();

		avoidance_debug_dirty = true;
		callable_mp(this, &NavigationServer3D::_emit_avoidance_debug_changed_signal)
			.call_deferred();
	}
#endif // DEBUG_ENABLED
}

bool NavigationServer3D::get_debug_enabled() const { return debug_enabled; }

#ifdef DEBUG_ENABLED
void NavigationServer3D::_emit_navigation_debug_changed_signal()
{
	if (navigation_debug_dirty) {
		navigation_debug_dirty = false;
		this->obj->emit_signal(SNAME("navigation_debug_changed"));
	}
}

void NavigationServer3D::_emit_avoidance_debug_changed_signal()
{
	if (avoidance_debug_dirty) {
		avoidance_debug_dirty = false;
		this->obj->emit_signal(SNAME("avoidance_debug_changed"));
	}
}
#endif // DEBUG_ENABLED

#ifdef DEBUG_ENABLED
Ref<StandardMaterial3D> NavigationServer3D::get_debug_navigation_geometry_face_material()
{
	if (debug_navigation_geometry_face_material.is_valid()) {
		return debug_navigation_geometry_face_material;
	}

	bool enabled_geometry_face_random_color =
		get_debug_navigation_enable_geometry_face_random_color();

	Ref<StandardMaterial3D> face_material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	face_material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	face_material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
	face_material->set_albedo(get_debug_navigation_geometry_face_color());
	face_material->set_cull_mode(StandardMaterial3D::CULL_DISABLED);
	face_material->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);
	if (enabled_geometry_face_random_color) {
		face_material->set_flag(StandardMaterial3D::FLAG_SRGB_VERTEX_COLOR, true);
		face_material->set_flag(StandardMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
	}

	debug_navigation_geometry_face_material = face_material;

	return debug_navigation_geometry_face_material;
}

Ref<StandardMaterial3D> NavigationServer3D::get_debug_navigation_geometry_edge_material()
{
	if (debug_navigation_geometry_edge_material.is_valid()) {
		return debug_navigation_geometry_edge_material;
	}

	bool enabled_edge_lines_xray = get_debug_navigation_enable_edge_lines_xray();

	Ref<StandardMaterial3D> line_material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	line_material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	line_material->set_albedo(get_debug_navigation_geometry_edge_color());
	line_material->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);
	if (enabled_edge_lines_xray) {
		line_material->set_flag(StandardMaterial3D::FLAG_DISABLE_DEPTH_TEST, true);
	}

	debug_navigation_geometry_edge_material = line_material;

	return debug_navigation_geometry_edge_material;
}

Ref<StandardMaterial3D> NavigationServer3D::get_debug_navigation_geometry_face_disabled_material()
{
	if (debug_navigation_geometry_face_disabled_material.is_valid()) {
		return debug_navigation_geometry_face_disabled_material;
	}

	Ref<StandardMaterial3D> face_disabled_material =
		Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	face_disabled_material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	face_disabled_material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
	face_disabled_material->set_albedo(get_debug_navigation_geometry_face_disabled_color());
	face_disabled_material->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);

	debug_navigation_geometry_face_disabled_material = face_disabled_material;

	return debug_navigation_geometry_face_disabled_material;
}

Ref<StandardMaterial3D> NavigationServer3D::get_debug_navigation_geometry_edge_disabled_material()
{
	if (debug_navigation_geometry_edge_disabled_material.is_valid()) {
		return debug_navigation_geometry_edge_disabled_material;
	}

	bool enabled_edge_lines_xray = get_debug_navigation_enable_edge_lines_xray();

	Ref<StandardMaterial3D> line_disabled_material =
		Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	line_disabled_material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	line_disabled_material->set_albedo(get_debug_navigation_geometry_edge_disabled_color());
	line_disabled_material->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);
	if (enabled_edge_lines_xray) {
		line_disabled_material->set_flag(StandardMaterial3D::FLAG_DISABLE_DEPTH_TEST, true);
	}

	debug_navigation_geometry_edge_disabled_material = line_disabled_material;

	return debug_navigation_geometry_edge_disabled_material;
}

Ref<StandardMaterial3D> NavigationServer3D::get_debug_navigation_edge_connections_material()
{
	if (debug_navigation_edge_connections_material.is_valid()) {
		return debug_navigation_edge_connections_material;
	}

	bool enabled_edge_connections_xray = get_debug_navigation_enable_edge_connections_xray();

	Ref<StandardMaterial3D> edge_connections_material =
		Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	edge_connections_material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	edge_connections_material->set_albedo(get_debug_navigation_edge_connection_color());
	edge_connections_material->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);
	if (enabled_edge_connections_xray) {
		edge_connections_material->set_flag(StandardMaterial3D::FLAG_DISABLE_DEPTH_TEST, true);
	}
	edge_connections_material->set_render_priority(StandardMaterial3D::RENDER_PRIORITY_MAX - 2);

	debug_navigation_edge_connections_material = edge_connections_material;

	return debug_navigation_edge_connections_material;
}

Ref<StandardMaterial3D> NavigationServer3D::get_debug_navigation_link_connections_material()
{
	if (debug_navigation_link_connections_material.is_valid()) {
		return debug_navigation_link_connections_material;
	}

	Ref<StandardMaterial3D> material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	material->set_albedo(debug_navigation_link_connection_color);
	material->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);
	if (debug_navigation_enable_link_connections_xray) {
		material->set_flag(StandardMaterial3D::FLAG_DISABLE_DEPTH_TEST, true);
	}
	material->set_render_priority(StandardMaterial3D::RENDER_PRIORITY_MAX - 2);

	debug_navigation_link_connections_material = material;
	return debug_navigation_link_connections_material;
}

Ref<StandardMaterial3D>
NavigationServer3D::get_debug_navigation_link_connections_disabled_material()
{
	if (debug_navigation_link_connections_disabled_material.is_valid()) {
		return debug_navigation_link_connections_disabled_material;
	}

	Ref<StandardMaterial3D> material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	material->set_albedo(debug_navigation_link_connection_disabled_color);
	material->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);
	if (debug_navigation_enable_link_connections_xray) {
		material->set_flag(StandardMaterial3D::FLAG_DISABLE_DEPTH_TEST, true);
	}
	material->set_render_priority(StandardMaterial3D::RENDER_PRIORITY_MAX - 2);

	debug_navigation_link_connections_disabled_material = material;
	return debug_navigation_link_connections_disabled_material;
}

Ref<StandardMaterial3D> NavigationServer3D::get_debug_navigation_agent_path_line_material()
{
	if (debug_navigation_agent_path_line_material.is_valid()) {
		return debug_navigation_agent_path_line_material;
	}

	Ref<StandardMaterial3D> material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);

	material->set_albedo(debug_navigation_agent_path_color);
	material->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);
	if (debug_navigation_enable_agent_paths_xray) {
		material->set_flag(StandardMaterial3D::FLAG_DISABLE_DEPTH_TEST, true);
	}
	material->set_render_priority(StandardMaterial3D::RENDER_PRIORITY_MAX - 2);

	debug_navigation_agent_path_line_material = material;
	return debug_navigation_agent_path_line_material;
}

Ref<StandardMaterial3D> NavigationServer3D::get_debug_navigation_agent_path_point_material()
{
	if (debug_navigation_agent_path_point_material.is_valid()) {
		return debug_navigation_agent_path_point_material;
	}

	Ref<StandardMaterial3D> material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	material->set_albedo(debug_navigation_agent_path_color);
	material->set_flag(StandardMaterial3D::FLAG_USE_POINT_SIZE, true);
	material->set_point_size(debug_navigation_agent_path_point_size);
	material->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);
	if (debug_navigation_enable_agent_paths_xray) {
		material->set_flag(StandardMaterial3D::FLAG_DISABLE_DEPTH_TEST, true);
	}
	material->set_render_priority(StandardMaterial3D::RENDER_PRIORITY_MAX - 2);

	debug_navigation_agent_path_point_material = material;
	return debug_navigation_agent_path_point_material;
}

Ref<StandardMaterial3D> NavigationServer3D::get_debug_navigation_avoidance_agents_radius_material()
{
	if (debug_navigation_avoidance_agents_radius_material.is_valid()) {
		return debug_navigation_avoidance_agents_radius_material;
	}

	Ref<StandardMaterial3D> material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
	material->set_cull_mode(StandardMaterial3D::CULL_DISABLED);
	material->set_albedo(debug_navigation_avoidance_agents_radius_color);
	material->set_render_priority(StandardMaterial3D::RENDER_PRIORITY_MIN + 2);

	debug_navigation_avoidance_agents_radius_material = material;
	return debug_navigation_avoidance_agents_radius_material;
}

Ref<StandardMaterial3D>
NavigationServer3D::get_debug_navigation_avoidance_obstacles_radius_material()
{
	if (debug_navigation_avoidance_obstacles_radius_material.is_valid()) {
		return debug_navigation_avoidance_obstacles_radius_material;
	}

	Ref<StandardMaterial3D> material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	material->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);
	material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
	material->set_cull_mode(StandardMaterial3D::CULL_DISABLED);
	material->set_albedo(debug_navigation_avoidance_obstacles_radius_color);
	material->set_render_priority(StandardMaterial3D::RENDER_PRIORITY_MIN + 2);

	debug_navigation_avoidance_obstacles_radius_material = material;
	return debug_navigation_avoidance_obstacles_radius_material;
}

Ref<StandardMaterial3D>
NavigationServer3D::get_debug_navigation_avoidance_static_obstacle_pushin_face_material()
{
	if (debug_navigation_avoidance_static_obstacle_pushin_face_material.is_valid()) {
		return debug_navigation_avoidance_static_obstacle_pushin_face_material;
	}

	Ref<StandardMaterial3D> material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	material->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);
	material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
	material->set_cull_mode(StandardMaterial3D::CULL_DISABLED);
	material->set_albedo(debug_navigation_avoidance_static_obstacle_pushin_face_color);
	material->set_render_priority(StandardMaterial3D::RENDER_PRIORITY_MIN + 2);

	debug_navigation_avoidance_static_obstacle_pushin_face_material = material;
	return debug_navigation_avoidance_static_obstacle_pushin_face_material;
}

Ref<StandardMaterial3D>
NavigationServer3D::get_debug_navigation_avoidance_static_obstacle_pushout_face_material()
{
	if (debug_navigation_avoidance_static_obstacle_pushout_face_material.is_valid()) {
		return debug_navigation_avoidance_static_obstacle_pushout_face_material;
	}

	Ref<StandardMaterial3D> material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	material->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);
	material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
	material->set_cull_mode(StandardMaterial3D::CULL_DISABLED);
	material->set_albedo(debug_navigation_avoidance_static_obstacle_pushout_face_color);
	material->set_render_priority(StandardMaterial3D::RENDER_PRIORITY_MIN + 2);

	debug_navigation_avoidance_static_obstacle_pushout_face_material = material;
	return debug_navigation_avoidance_static_obstacle_pushout_face_material;
}

Ref<StandardMaterial3D>
NavigationServer3D::get_debug_navigation_avoidance_static_obstacle_pushin_edge_material()
{
	if (debug_navigation_avoidance_static_obstacle_pushin_edge_material.is_valid()) {
		return debug_navigation_avoidance_static_obstacle_pushin_edge_material;
	}

	Ref<StandardMaterial3D> material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	material->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);
	// material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
	// material->set_cull_mode(StandardMaterial3D::CULL_DISABLED);
	material->set_albedo(debug_navigation_avoidance_static_obstacle_pushin_edge_color);
	// material->set_render_priority(StandardMaterial3D::RENDER_PRIORITY_MIN + 2);
	material->set_flag(StandardMaterial3D::FLAG_DISABLE_DEPTH_TEST, true);

	debug_navigation_avoidance_static_obstacle_pushin_edge_material = material;
	return debug_navigation_avoidance_static_obstacle_pushin_edge_material;
}

Ref<StandardMaterial3D>
NavigationServer3D::get_debug_navigation_avoidance_static_obstacle_pushout_edge_material()
{
	if (debug_navigation_avoidance_static_obstacle_pushout_edge_material.is_valid()) {
		return debug_navigation_avoidance_static_obstacle_pushout_edge_material;
	}

	Ref<StandardMaterial3D> material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	material->set_flag(StandardMaterial3D::FLAG_DISABLE_FOG, true);
	/// material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
	// material->set_cull_mode(StandardMaterial3D::CULL_DISABLED);
	material->set_albedo(debug_navigation_avoidance_static_obstacle_pushout_edge_color);
	// material->set_render_priority(StandardMaterial3D::RENDER_PRIORITY_MIN + 2);
	material->set_flag(StandardMaterial3D::FLAG_DISABLE_DEPTH_TEST, true);

	debug_navigation_avoidance_static_obstacle_pushout_edge_material = material;
	return debug_navigation_avoidance_static_obstacle_pushout_edge_material;
}

void NavigationServer3D::set_debug_navigation_edge_connection_color(const Color& p_color)
{
	debug_navigation_edge_connection_color = p_color;
	if (debug_navigation_edge_connections_material.is_valid()) {
		debug_navigation_edge_connections_material->set_albedo(
			debug_navigation_edge_connection_color);
	}
}

Color NavigationServer3D::get_debug_navigation_edge_connection_color() const
{
	return debug_navigation_edge_connection_color;
}

void NavigationServer3D::set_debug_navigation_geometry_edge_color(const Color& p_color)
{
	debug_navigation_geometry_edge_color = p_color;
	if (debug_navigation_geometry_edge_material.is_valid()) {
		debug_navigation_geometry_edge_material->set_albedo(debug_navigation_geometry_edge_color);
	}
}

Color NavigationServer3D::get_debug_navigation_geometry_edge_color() const
{
	return debug_navigation_geometry_edge_color;
}

void NavigationServer3D::set_debug_navigation_geometry_face_color(const Color& p_color)
{
	debug_navigation_geometry_face_color = p_color;
	if (debug_navigation_geometry_face_material.is_valid()) {
		debug_navigation_geometry_face_material->set_albedo(debug_navigation_geometry_face_color);
	}
}

Color NavigationServer3D::get_debug_navigation_geometry_face_color() const
{
	return debug_navigation_geometry_face_color;
}

void NavigationServer3D::set_debug_navigation_geometry_edge_disabled_color(const Color& p_color)
{
	debug_navigation_geometry_edge_disabled_color = p_color;
	if (debug_navigation_geometry_edge_disabled_material.is_valid()) {
		debug_navigation_geometry_edge_disabled_material->set_albedo(
			debug_navigation_geometry_edge_disabled_color);
	}
}

Color NavigationServer3D::get_debug_navigation_geometry_edge_disabled_color() const
{
	return debug_navigation_geometry_edge_disabled_color;
}

void NavigationServer3D::set_debug_navigation_geometry_face_disabled_color(const Color& p_color)
{
	debug_navigation_geometry_face_disabled_color = p_color;
	if (debug_navigation_geometry_face_disabled_material.is_valid()) {
		debug_navigation_geometry_face_disabled_material->set_albedo(
			debug_navigation_geometry_face_disabled_color);
	}
}

Color NavigationServer3D::get_debug_navigation_geometry_face_disabled_color() const
{
	return debug_navigation_geometry_face_disabled_color;
}

void NavigationServer3D::set_debug_navigation_link_connection_color(const Color& p_color)
{
	debug_navigation_link_connection_color = p_color;
	if (debug_navigation_link_connections_material.is_valid()) {
		debug_navigation_link_connections_material->set_albedo(
			debug_navigation_link_connection_color);
	}
}

Color NavigationServer3D::get_debug_navigation_link_connection_color() const
{
	return debug_navigation_link_connection_color;
}

void NavigationServer3D::set_debug_navigation_link_connection_disabled_color(const Color& p_color)
{
	debug_navigation_link_connection_disabled_color = p_color;
	if (debug_navigation_link_connections_disabled_material.is_valid()) {
		debug_navigation_link_connections_disabled_material->set_albedo(
			debug_navigation_link_connection_disabled_color);
	}
}

Color NavigationServer3D::get_debug_navigation_link_connection_disabled_color() const
{
	return debug_navigation_link_connection_disabled_color;
}

void NavigationServer3D::set_debug_navigation_agent_path_point_size(real_t p_point_size)
{
	debug_navigation_agent_path_point_size = MAX(0.1, p_point_size);
	if (debug_navigation_agent_path_point_material.is_valid()) {
		debug_navigation_agent_path_point_material->set_point_size(
			debug_navigation_agent_path_point_size);
	}
}

real_t NavigationServer3D::get_debug_navigation_agent_path_point_size() const
{
	return debug_navigation_agent_path_point_size;
}

void NavigationServer3D::set_debug_navigation_agent_path_color(const Color& p_color)
{
	debug_navigation_agent_path_color = p_color;
	if (debug_navigation_agent_path_line_material.is_valid()) {
		debug_navigation_agent_path_line_material->set_albedo(debug_navigation_agent_path_color);
	}
	if (debug_navigation_agent_path_point_material.is_valid()) {
		debug_navigation_agent_path_point_material->set_albedo(debug_navigation_agent_path_color);
	}
}

Color NavigationServer3D::get_debug_navigation_agent_path_color() const
{
	return debug_navigation_agent_path_color;
}

void NavigationServer3D::set_debug_navigation_enable_edge_connections(const bool p_value)
{
	debug_navigation_enable_edge_connections = p_value;
	navigation_debug_dirty = true;
	callable_mp(this, &NavigationServer3D::_emit_navigation_debug_changed_signal).call_deferred();
}

bool NavigationServer3D::get_debug_navigation_enable_edge_connections() const
{
	return debug_navigation_enable_edge_connections;
}

void NavigationServer3D::set_debug_navigation_enable_edge_connections_xray(const bool p_value)
{
	debug_navigation_enable_edge_connections_xray = p_value;
	if (debug_navigation_edge_connections_material.is_valid()) {
		debug_navigation_edge_connections_material->set_flag(
			StandardMaterial3D::FLAG_DISABLE_DEPTH_TEST,
			debug_navigation_enable_edge_connections_xray);
	}
}

bool NavigationServer3D::get_debug_navigation_enable_edge_connections_xray() const
{
	return debug_navigation_enable_edge_connections_xray;
}

void NavigationServer3D::set_debug_navigation_enable_edge_lines(const bool p_value)
{
	debug_navigation_enable_edge_lines = p_value;
	navigation_debug_dirty = true;
	callable_mp(this, &NavigationServer3D::_emit_navigation_debug_changed_signal).call_deferred();
}

bool NavigationServer3D::get_debug_navigation_enable_edge_lines() const
{
	return debug_navigation_enable_edge_lines;
}

void NavigationServer3D::set_debug_navigation_enable_edge_lines_xray(const bool p_value)
{
	debug_navigation_enable_edge_lines_xray = p_value;
	if (debug_navigation_geometry_edge_material.is_valid()) {
		debug_navigation_geometry_edge_material->set_flag(
			StandardMaterial3D::FLAG_DISABLE_DEPTH_TEST, debug_navigation_enable_edge_lines_xray);
	}
}

bool NavigationServer3D::get_debug_navigation_enable_edge_lines_xray() const
{
	return debug_navigation_enable_edge_lines_xray;
}

void NavigationServer3D::set_debug_navigation_enable_geometry_face_random_color(const bool p_value)
{
	debug_navigation_enable_geometry_face_random_color = p_value;
	navigation_debug_dirty = true;
	callable_mp(this, &NavigationServer3D::_emit_navigation_debug_changed_signal).call_deferred();
}

bool NavigationServer3D::get_debug_navigation_enable_geometry_face_random_color() const
{
	return debug_navigation_enable_geometry_face_random_color;
}

void NavigationServer3D::set_debug_navigation_enable_link_connections(const bool p_value)
{
	debug_navigation_enable_link_connections = p_value;
	navigation_debug_dirty = true;
	callable_mp(this, &NavigationServer3D::_emit_navigation_debug_changed_signal).call_deferred();
}

bool NavigationServer3D::get_debug_navigation_enable_link_connections() const
{
	return debug_navigation_enable_link_connections;
}

void NavigationServer3D::set_debug_navigation_enable_link_connections_xray(const bool p_value)
{
	debug_navigation_enable_link_connections_xray = p_value;
	if (debug_navigation_link_connections_material.is_valid()) {
		debug_navigation_link_connections_material->set_flag(
			StandardMaterial3D::FLAG_DISABLE_DEPTH_TEST,
			debug_navigation_enable_link_connections_xray);
	}
}

bool NavigationServer3D::get_debug_navigation_enable_link_connections_xray() const
{
	return debug_navigation_enable_link_connections_xray;
}

void NavigationServer3D::set_debug_navigation_avoidance_enable_agents_radius(const bool p_value)
{
	debug_navigation_avoidance_enable_agents_radius = p_value;
	avoidance_debug_dirty = true;
	callable_mp(this, &NavigationServer3D::_emit_avoidance_debug_changed_signal).call_deferred();
}

bool NavigationServer3D::get_debug_navigation_avoidance_enable_agents_radius() const
{
	return debug_navigation_avoidance_enable_agents_radius;
}

void NavigationServer3D::set_debug_navigation_avoidance_enable_obstacles_radius(const bool p_value)
{
	debug_navigation_avoidance_enable_obstacles_radius = p_value;
	avoidance_debug_dirty = true;
	callable_mp(this, &NavigationServer3D::_emit_avoidance_debug_changed_signal).call_deferred();
}

bool NavigationServer3D::get_debug_navigation_avoidance_enable_obstacles_radius() const
{
	return debug_navigation_avoidance_enable_obstacles_radius;
}

void NavigationServer3D::set_debug_navigation_avoidance_enable_obstacles_static(const bool p_value)
{
	debug_navigation_avoidance_enable_obstacles_static = p_value;
	avoidance_debug_dirty = true;
	callable_mp(this, &NavigationServer3D::_emit_avoidance_debug_changed_signal).call_deferred();
}

bool NavigationServer3D::get_debug_navigation_avoidance_enable_obstacles_static() const
{
	return debug_navigation_avoidance_enable_obstacles_static;
}

void NavigationServer3D::set_debug_navigation_avoidance_agents_radius_color(const Color& p_color)
{
	debug_navigation_avoidance_agents_radius_color = p_color;
	if (debug_navigation_avoidance_agents_radius_material.is_valid()) {
		debug_navigation_avoidance_agents_radius_material->set_albedo(
			debug_navigation_avoidance_agents_radius_color);
	}
}

Color NavigationServer3D::get_debug_navigation_avoidance_agents_radius_color() const
{
	return debug_navigation_avoidance_agents_radius_color;
}

void NavigationServer3D::set_debug_navigation_avoidance_obstacles_radius_color(const Color& p_color)
{
	debug_navigation_avoidance_obstacles_radius_color = p_color;
	if (debug_navigation_avoidance_obstacles_radius_material.is_valid()) {
		debug_navigation_avoidance_obstacles_radius_material->set_albedo(
			debug_navigation_avoidance_obstacles_radius_color);
	}
}

Color NavigationServer3D::get_debug_navigation_avoidance_obstacles_radius_color() const
{
	return debug_navigation_avoidance_obstacles_radius_color;
}

void NavigationServer3D::set_debug_navigation_avoidance_static_obstacle_pushin_face_color(
	const Color& p_color)
{
	debug_navigation_avoidance_static_obstacle_pushin_face_color = p_color;
	if (debug_navigation_avoidance_static_obstacle_pushin_face_material.is_valid()) {
		debug_navigation_avoidance_static_obstacle_pushin_face_material->set_albedo(
			debug_navigation_avoidance_static_obstacle_pushin_face_color);
	}
}

Color NavigationServer3D::get_debug_navigation_avoidance_static_obstacle_pushin_face_color() const
{
	return debug_navigation_avoidance_static_obstacle_pushin_face_color;
}

void NavigationServer3D::set_debug_navigation_avoidance_static_obstacle_pushout_face_color(
	const Color& p_color)
{
	debug_navigation_avoidance_static_obstacle_pushout_face_color = p_color;
	if (debug_navigation_avoidance_static_obstacle_pushout_face_material.is_valid()) {
		debug_navigation_avoidance_static_obstacle_pushout_face_material->set_albedo(
			debug_navigation_avoidance_static_obstacle_pushout_face_color);
	}
}

Color NavigationServer3D::get_debug_navigation_avoidance_static_obstacle_pushout_face_color() const
{
	return debug_navigation_avoidance_static_obstacle_pushout_face_color;
}

void NavigationServer3D::set_debug_navigation_avoidance_static_obstacle_pushin_edge_color(
	const Color& p_color)
{
	debug_navigation_avoidance_static_obstacle_pushin_edge_color = p_color;
	if (debug_navigation_avoidance_static_obstacle_pushin_edge_material.is_valid()) {
		debug_navigation_avoidance_static_obstacle_pushin_edge_material->set_albedo(
			debug_navigation_avoidance_static_obstacle_pushin_edge_color);
	}
}

Color NavigationServer3D::get_debug_navigation_avoidance_static_obstacle_pushin_edge_color() const
{
	return debug_navigation_avoidance_static_obstacle_pushin_edge_color;
}

void NavigationServer3D::set_debug_navigation_avoidance_static_obstacle_pushout_edge_color(
	const Color& p_color)
{
	debug_navigation_avoidance_static_obstacle_pushout_edge_color = p_color;
	if (debug_navigation_avoidance_static_obstacle_pushout_edge_material.is_valid()) {
		debug_navigation_avoidance_static_obstacle_pushout_edge_material->set_albedo(
			debug_navigation_avoidance_static_obstacle_pushout_edge_color);
	}
}

Color NavigationServer3D::get_debug_navigation_avoidance_static_obstacle_pushout_edge_color() const
{
	return debug_navigation_avoidance_static_obstacle_pushout_edge_color;
}

void NavigationServer3D::set_debug_navigation_enable_agent_paths(const bool p_value)
{
	if (debug_navigation_enable_agent_paths != p_value) {
		debug_dirty = true;
	}

	debug_navigation_enable_agent_paths = p_value;

	if (debug_dirty) {
		callable_mp(this, &NavigationServer3D::_emit_navigation_debug_changed_signal)
			.call_deferred();
	}
}

bool NavigationServer3D::get_debug_navigation_enable_agent_paths() const
{
	return debug_navigation_enable_agent_paths;
}

void NavigationServer3D::set_debug_navigation_enable_agent_paths_xray(const bool p_value)
{
	debug_navigation_enable_agent_paths_xray = p_value;
	if (debug_navigation_agent_path_line_material.is_valid()) {
		debug_navigation_agent_path_line_material->set_flag(
			StandardMaterial3D::FLAG_DISABLE_DEPTH_TEST, debug_navigation_enable_agent_paths_xray);
	}
	if (debug_navigation_agent_path_point_material.is_valid()) {
		debug_navigation_agent_path_point_material->set_flag(
			StandardMaterial3D::FLAG_DISABLE_DEPTH_TEST, debug_navigation_enable_agent_paths_xray);
	}
}

bool NavigationServer3D::get_debug_navigation_enable_agent_paths_xray() const
{
	return debug_navigation_enable_agent_paths_xray;
}

void NavigationServer3D::set_debug_navigation_enabled(bool p_enabled)
{
	debug_navigation_enabled = p_enabled;
	navigation_debug_dirty = true;
	callable_mp(this, &NavigationServer3D::_emit_navigation_debug_changed_signal).call_deferred();
}

bool NavigationServer3D::get_debug_navigation_enabled() const { return debug_navigation_enabled; }

void NavigationServer3D::set_debug_avoidance_enabled(bool p_enabled)
{
	debug_avoidance_enabled = p_enabled;
	avoidance_debug_dirty = true;
	callable_mp(this, &NavigationServer3D::_emit_avoidance_debug_changed_signal).call_deferred();
}

bool NavigationServer3D::get_debug_avoidance_enabled() const { return debug_avoidance_enabled; }

#endif // DEBUG_ENABLED

///////////////////////////////////////////////////////

static NavigationServer3D* navigation_server_3d = nullptr;

void NavigationServer3DManager::initialize_server()
{
	ERR_FAIL_COND(navigation_server_3d != nullptr);

	// Init 3D Navigation Server.
	navigation_server_3d = NavigationServer3DManager::get_singleton()->new_server(
		GLOBAL_GET(NavigationServer3DManager::setting_property_name));
	if (!navigation_server_3d) {
		// Navigation server not found, use the default.
		navigation_server_3d = NavigationServer3DManager::get_singleton()->new_default_server();
	}

	// Fall back to dummy if no default server has been registered.
	if (!navigation_server_3d) {
		WARN_VERBOSE("Failed to initialize NavigationServer3D. Fall back to dummy server.");
		navigation_server_3d = memnew(NavigationServer3DDummy);
	}

	// Should be impossible, but make sure it's not null.
	ERR_FAIL_NULL_MSG(navigation_server_3d, "Failed to initialize NavigationServer3D.");
	navigation_server_3d->init();
}

void NavigationServer3DManager::finalize_server()
{
	ERR_FAIL_NULL(navigation_server_3d);
	navigation_server_3d->finish();
	memdelete(navigation_server_3d);
	navigation_server_3d = nullptr;
}

const String NavigationServer3DManager::setting_property_name(
	PNAME("navigation/3d/navigation_engine"));

void NavigationServer3DManager::on_servers_changed()
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

void NavigationServer3DManager::_bind_methods() {}

NavigationServer3DManager* NavigationServer3DManager::get_singleton() { return singleton; }

void NavigationServer3DManager::register_server(
	const String& p_name, const Callable& p_create_callback)
{
	ERR_FAIL_COND(find_server_id(p_name) != -1);
	navigation_servers.push_back(ClassInfo(p_name, p_create_callback));
	on_servers_changed();
}

void NavigationServer3DManager::set_default_server(const String& p_name, int p_priority)
{
	const int id = find_server_id(p_name);
	ERR_FAIL_COND(id == -1); // Not found
	if (default_server_priority < p_priority) {
		default_server_id = id;
		default_server_priority = p_priority;
	}
}

int NavigationServer3DManager::find_server_id(const String& p_name)
{
	for (int i = navigation_servers.size() - 1; 0 <= i; --i) {
		if (p_name == navigation_servers[i].name) {
			return i;
		}
	}
	return -1;
}

int NavigationServer3DManager::get_servers_count() { return navigation_servers.size(); }

String NavigationServer3DManager::get_server_name(int p_id)
{
	ERR_FAIL_INDEX_V(p_id, get_servers_count(), "");
	return navigation_servers[p_id].name;
}

NavigationServer3D* NavigationServer3DManager::new_default_server()
{
	if (default_server_id == -1) {
		return nullptr;
	}
	Variant ret;
	Callable::CallError ce;
	navigation_servers[default_server_id].create_callback.callp(nullptr, 0, ret, ce);
	ERR_FAIL_COND_V(ce.error != Callable::CallError::CALL_OK, nullptr);
	return Object::cast_to<NavigationServer3D>(ret.get_validated_object());
}

NavigationServer3D* NavigationServer3DManager::new_server(const String& p_name)
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
		return Object::cast_to<NavigationServer3D>(ret.get_validated_object());
	}
}

NavigationServer3D* NavigationServer3DManager::create_dummy_server_callback()
{
	return memnew(NavigationServer3DDummy);
}

NavigationServer3DManager::NavigationServer3DManager() {}

NavigationServer3DManager::~NavigationServer3DManager() {}

void NavigationServer3DManager::initialize_server_manager()
{
	ERR_FAIL_COND(singleton != nullptr);
	singleton = memnew(NavigationServer3DManager);
}

void NavigationServer3DManager::finalize_server_manager()
{
	ERR_FAIL_NULL(singleton);
	memdelete(singleton);
}


