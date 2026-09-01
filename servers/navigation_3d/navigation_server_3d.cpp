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
#include "navigation_server_3d.compat.inc"
#include "navigation_server_3d.h"
#include "scene/main/node.h" // IWYU pragma: keep. Needed to bind `Node *` arg.
#include "servers/navigation_3d/navigation_server_3d_dummy.h"

NavigationServer3D* NavigationServer3D::singleton = nullptr;

RWLock NavigationServer3D::geometry_parser_rwlock;

NavigationServer3D* NavigationServer3D::get_singleton() { return singleton; }

bool NavigationServer3D::get_debug_enabled() const { return debug_enabled; }

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

bool NavigationServer3D::get_debug_navigation_enable_geometry_face_random_color() const
{
	return debug_navigation_enable_geometry_face_random_color;
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

bool NavigationServer3D::get_debug_navigation_avoidance_enable_agents_radius() const
{
	return debug_navigation_avoidance_enable_agents_radius;
}

bool NavigationServer3D::get_debug_navigation_avoidance_enable_obstacles_radius() const
{
	return debug_navigation_avoidance_enable_obstacles_radius;
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

bool NavigationServer3D::get_debug_navigation_enabled() const { return debug_navigation_enabled; }

bool NavigationServer3D::get_debug_avoidance_enabled() const { return debug_avoidance_enabled; }

#endif // DEBUG_ENABLED

static NavigationServer3D* navigation_server_3d = nullptr;

void NavigationServer3DManager::finalize_server()
{
	ERR_FAIL_NULL(navigation_server_3d);
	navigation_server_3d->finish();
	memdelete(navigation_server_3d);
	navigation_server_3d = nullptr;
}

const String NavigationServer3DManager::setting_property_name(
	PNAME("navigation/3d/navigation_engine"));

NavigationServer3DManager* NavigationServer3DManager::get_singleton() { return singleton; }

void NavigationServer3DManager::set_default_server(const String& p_name, int p_priority)
{
	const int id = find_server_id(p_name);
	ERR_FAIL_COND(id == -1); // Not found
	if (default_server_priority < p_priority) {
		default_server_id = id;
		default_server_priority = p_priority;
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


