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
#include "navigation_server_3d.h"
#include "scene/main/node.h" // IWYU pragma: keep. Needed to bind `Node *` arg.

bool NavigationServer3D::get_debug_enabled() { return data->debug_enabled; }

#ifdef DEBUG_ENABLED
Ref<StandardMaterial3D> NavigationServer3D::get_debug_navigation_geometry_face_material()
{
	if (data->debug_navigation_geometry_face_material.is_valid()) {
		return data->debug_navigation_geometry_face_material;
	}

	bool enabled_geometry_face_random_color =
		get_debug_navigation_enable_geometry_face_random_color();

	Ref<StandardMaterial3D> face_material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	face_material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	face_material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
	face_material->set_cull_mode(StandardMaterial3D::CULL_DISABLED);
	data->debug_navigation_geometry_face_material = face_material;

	return data->debug_navigation_geometry_face_material;
}

Ref<StandardMaterial3D> NavigationServer3D::get_debug_navigation_geometry_edge_material()
{
	if (data->debug_navigation_geometry_edge_material.is_valid()) {
		return data->debug_navigation_geometry_edge_material;
	}

	bool enabled_edge_lines_xray = get_debug_navigation_enable_edge_lines_xray();

	Ref<StandardMaterial3D> line_material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	line_material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	data->debug_navigation_geometry_edge_material = line_material;

	return data->debug_navigation_geometry_edge_material;
}

Ref<StandardMaterial3D> NavigationServer3D::get_debug_navigation_geometry_face_disabled_material()
{
	if (data->debug_navigation_geometry_face_disabled_material.is_valid()) {
		return data->debug_navigation_geometry_face_disabled_material;
	}

	Ref<StandardMaterial3D> face_disabled_material =
		Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	face_disabled_material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	face_disabled_material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);

	data->debug_navigation_geometry_face_disabled_material = face_disabled_material;

	return data->debug_navigation_geometry_face_disabled_material;
}

Ref<StandardMaterial3D> NavigationServer3D::get_debug_navigation_geometry_edge_disabled_material()
{
	if (data->debug_navigation_geometry_edge_disabled_material.is_valid()) {
		return data->debug_navigation_geometry_edge_disabled_material;
	}

	bool enabled_edge_lines_xray = get_debug_navigation_enable_edge_lines_xray();

	Ref<StandardMaterial3D> line_disabled_material =
		Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	line_disabled_material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);

	data->debug_navigation_geometry_edge_disabled_material = line_disabled_material;

	return data->debug_navigation_geometry_edge_disabled_material;
}

Ref<StandardMaterial3D> NavigationServer3D::get_debug_navigation_edge_connections_material()
{
	if (data->debug_navigation_edge_connections_material.is_valid()) {
		return data->debug_navigation_edge_connections_material;
	}

	bool enabled_edge_connections_xray = get_debug_navigation_enable_edge_connections_xray();

	Ref<StandardMaterial3D> edge_connections_material =
		Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	edge_connections_material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	edge_connections_material->set_render_priority(StandardMaterial3D::RENDER_PRIORITY_MAX - 2);

	data->debug_navigation_edge_connections_material = edge_connections_material;

	return data->debug_navigation_edge_connections_material;
}

Ref<StandardMaterial3D> NavigationServer3D::get_debug_navigation_link_connections_material()
{
	if (data->debug_navigation_link_connections_material.is_valid()) {
		return data->debug_navigation_link_connections_material;
	}

	Ref<StandardMaterial3D> material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	material->set_render_priority(StandardMaterial3D::RENDER_PRIORITY_MAX - 2);

	data->debug_navigation_link_connections_material = material;
	return data->debug_navigation_link_connections_material;
}

Ref<StandardMaterial3D>
NavigationServer3D::get_debug_navigation_link_connections_disabled_material()
{
	if (data->debug_navigation_link_connections_disabled_material.is_valid()) {
		return data->debug_navigation_link_connections_disabled_material;
	}

	Ref<StandardMaterial3D> material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	material->set_render_priority(StandardMaterial3D::RENDER_PRIORITY_MAX - 2);

	data->debug_navigation_link_connections_disabled_material = material;
	return data->debug_navigation_link_connections_disabled_material;
}

Ref<StandardMaterial3D> NavigationServer3D::get_debug_navigation_agent_path_line_material()
{
	if (data->debug_navigation_agent_path_line_material.is_valid()) {
		return data->debug_navigation_agent_path_line_material;
	}

	Ref<StandardMaterial3D> material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);

	material->set_render_priority(StandardMaterial3D::RENDER_PRIORITY_MAX - 2);

	data->debug_navigation_agent_path_line_material = material;
	return data->debug_navigation_agent_path_line_material;
}

Ref<StandardMaterial3D> NavigationServer3D::get_debug_navigation_agent_path_point_material()
{
	if (data->debug_navigation_agent_path_point_material.is_valid()) {
		return data->debug_navigation_agent_path_point_material;
	}

	Ref<StandardMaterial3D> material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	material->set_point_size(data->debug_navigation_agent_path_point_size);
	material->set_render_priority(StandardMaterial3D::RENDER_PRIORITY_MAX - 2);

	data->debug_navigation_agent_path_point_material = material;
	return data->debug_navigation_agent_path_point_material;
}

Ref<StandardMaterial3D> NavigationServer3D::get_debug_navigation_avoidance_agents_radius_material()
{
	if (data->debug_navigation_avoidance_agents_radius_material.is_valid()) {
		return data->debug_navigation_avoidance_agents_radius_material;
	}

	Ref<StandardMaterial3D> material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
	material->set_cull_mode(StandardMaterial3D::CULL_DISABLED);
	material->set_render_priority(StandardMaterial3D::RENDER_PRIORITY_MIN + 2);

	data->debug_navigation_avoidance_agents_radius_material = material;
	return data->debug_navigation_avoidance_agents_radius_material;
}

Ref<StandardMaterial3D>
NavigationServer3D::get_debug_navigation_avoidance_obstacles_radius_material()
{
	if (data->debug_navigation_avoidance_obstacles_radius_material.is_valid()) {
		return data->debug_navigation_avoidance_obstacles_radius_material;
	}

	Ref<StandardMaterial3D> material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
	material->set_cull_mode(StandardMaterial3D::CULL_DISABLED);
	material->set_render_priority(StandardMaterial3D::RENDER_PRIORITY_MIN + 2);

	data->debug_navigation_avoidance_obstacles_radius_material = material;
	return data->debug_navigation_avoidance_obstacles_radius_material;
}

Ref<StandardMaterial3D>
NavigationServer3D::get_debug_navigation_avoidance_static_obstacle_pushin_face_material()
{
	if (data->debug_navigation_avoidance_static_obstacle_pushin_face_material.is_valid()) {
		return data->debug_navigation_avoidance_static_obstacle_pushin_face_material;
	}

	Ref<StandardMaterial3D> material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
	material->set_cull_mode(StandardMaterial3D::CULL_DISABLED);
	material->set_render_priority(StandardMaterial3D::RENDER_PRIORITY_MIN + 2);

	data->debug_navigation_avoidance_static_obstacle_pushin_face_material = material;
	return data->debug_navigation_avoidance_static_obstacle_pushin_face_material;
}

Ref<StandardMaterial3D>
NavigationServer3D::get_debug_navigation_avoidance_static_obstacle_pushout_face_material()
{
	if (data->debug_navigation_avoidance_static_obstacle_pushout_face_material.is_valid()) {
		return data->debug_navigation_avoidance_static_obstacle_pushout_face_material;
	}

	Ref<StandardMaterial3D> material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
	material->set_cull_mode(StandardMaterial3D::CULL_DISABLED);
	material->set_render_priority(StandardMaterial3D::RENDER_PRIORITY_MIN + 2);

	data->debug_navigation_avoidance_static_obstacle_pushout_face_material = material;
	return data->debug_navigation_avoidance_static_obstacle_pushout_face_material;
}

Ref<StandardMaterial3D>
NavigationServer3D::get_debug_navigation_avoidance_static_obstacle_pushin_edge_material()
{
	if (data->debug_navigation_avoidance_static_obstacle_pushin_edge_material.is_valid()) {
		return data->debug_navigation_avoidance_static_obstacle_pushin_edge_material;
	}

	Ref<StandardMaterial3D> material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	// material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
	// material->set_cull_mode(StandardMaterial3D::CULL_DISABLED);
	// material->set_render_priority(StandardMaterial3D::RENDER_PRIORITY_MIN + 2);

	data->debug_navigation_avoidance_static_obstacle_pushin_edge_material = material;
	return data->debug_navigation_avoidance_static_obstacle_pushin_edge_material;
}

Ref<StandardMaterial3D>
NavigationServer3D::get_debug_navigation_avoidance_static_obstacle_pushout_edge_material()
{
	if (data->debug_navigation_avoidance_static_obstacle_pushout_edge_material.is_valid()) {
		return data->debug_navigation_avoidance_static_obstacle_pushout_edge_material;
	}

	Ref<StandardMaterial3D> material = Ref<StandardMaterial3D>(memnew(StandardMaterial3D));
	material->set_shading_mode(StandardMaterial3D::SHADING_MODE_UNSHADED);
	/// material->set_transparency(StandardMaterial3D::TRANSPARENCY_ALPHA);
	// material->set_cull_mode(StandardMaterial3D::CULL_DISABLED);
	// material->set_render_priority(StandardMaterial3D::RENDER_PRIORITY_MIN + 2);

	data->debug_navigation_avoidance_static_obstacle_pushout_edge_material = material;
	return data->debug_navigation_avoidance_static_obstacle_pushout_edge_material;
}

void NavigationServer3D::set_debug_navigation_edge_connection_color(const Color& p_color)
{
	data->debug_navigation_edge_connection_color = p_color;
}

Color NavigationServer3D::get_debug_navigation_edge_connection_color()
{
	return data->debug_navigation_edge_connection_color;
}

void NavigationServer3D::set_debug_navigation_geometry_edge_color(const Color& p_color)
{
	data->debug_navigation_geometry_edge_color = p_color;
}

Color NavigationServer3D::get_debug_navigation_geometry_edge_color()
{
	return data->debug_navigation_geometry_edge_color;
}

void NavigationServer3D::set_debug_navigation_geometry_face_color(const Color& p_color)
{
	data->debug_navigation_geometry_face_color = p_color;
}

Color NavigationServer3D::get_debug_navigation_geometry_face_color()
{
	return data->debug_navigation_geometry_face_color;
}

void NavigationServer3D::set_debug_navigation_geometry_edge_disabled_color(const Color& p_color)
{
	data->debug_navigation_geometry_edge_disabled_color = p_color;
}

Color NavigationServer3D::get_debug_navigation_geometry_edge_disabled_color()
{
	return data->debug_navigation_geometry_edge_disabled_color;
}

void NavigationServer3D::set_debug_navigation_geometry_face_disabled_color(const Color& p_color)
{
	data->debug_navigation_geometry_face_disabled_color = p_color;
}

Color NavigationServer3D::get_debug_navigation_geometry_face_disabled_color()
{
	return data->debug_navigation_geometry_face_disabled_color;
}

void NavigationServer3D::set_debug_navigation_link_connection_color(const Color& p_color)
{
	data->debug_navigation_link_connection_color = p_color;
}

Color NavigationServer3D::get_debug_navigation_link_connection_color()
{
	return data->debug_navigation_link_connection_color;
}

void NavigationServer3D::set_debug_navigation_link_connection_disabled_color(const Color& p_color)
{
	data->debug_navigation_link_connection_disabled_color = p_color;
}

Color NavigationServer3D::get_debug_navigation_link_connection_disabled_color()
{
	return data->debug_navigation_link_connection_disabled_color;
}

void NavigationServer3D::set_debug_navigation_agent_path_point_size(real_t p_point_size)
{
	data->debug_navigation_agent_path_point_size = MAX(0.1, p_point_size);
	if (data->debug_navigation_agent_path_point_material.is_valid()) {
		data->debug_navigation_agent_path_point_material->set_point_size(
			data->debug_navigation_agent_path_point_size);
	}
}

real_t NavigationServer3D::get_debug_navigation_agent_path_point_size()
{
	return data->debug_navigation_agent_path_point_size;
}

void NavigationServer3D::set_debug_navigation_agent_path_color(const Color& p_color)
{
	data->debug_navigation_agent_path_color = p_color;
}

Color NavigationServer3D::get_debug_navigation_agent_path_color()
{
	return data->debug_navigation_agent_path_color;
}

bool NavigationServer3D::get_debug_navigation_enable_edge_connections()
{
	return data->debug_navigation_enable_edge_connections;
}

void NavigationServer3D::set_debug_navigation_enable_edge_connections_xray(const bool p_value)
{
	data->debug_navigation_enable_edge_connections_xray = p_value;
}

bool NavigationServer3D::get_debug_navigation_enable_edge_connections_xray()
{
	return data->debug_navigation_enable_edge_connections_xray;
}

bool NavigationServer3D::get_debug_navigation_enable_edge_lines()
{
	return data->debug_navigation_enable_edge_lines;
}

void NavigationServer3D::set_debug_navigation_enable_edge_lines_xray(const bool p_value)
{
	data->debug_navigation_enable_edge_lines_xray = p_value;
}

bool NavigationServer3D::get_debug_navigation_enable_edge_lines_xray()
{
	return data->debug_navigation_enable_edge_lines_xray;
}

bool NavigationServer3D::get_debug_navigation_enable_geometry_face_random_color()
{
	return data->debug_navigation_enable_geometry_face_random_color;
}

bool NavigationServer3D::get_debug_navigation_enable_link_connections()
{
	return data->debug_navigation_enable_link_connections;
}

void NavigationServer3D::set_debug_navigation_enable_link_connections_xray(const bool p_value)
{
	data->debug_navigation_enable_link_connections_xray = p_value;
}

bool NavigationServer3D::get_debug_navigation_enable_link_connections_xray()
{
	return data->debug_navigation_enable_link_connections_xray;
}

bool NavigationServer3D::get_debug_navigation_avoidance_enable_agents_radius()
{
	return data->debug_navigation_avoidance_enable_agents_radius;
}

bool NavigationServer3D::get_debug_navigation_avoidance_enable_obstacles_radius()
{
	return data->debug_navigation_avoidance_enable_obstacles_radius;
}

bool NavigationServer3D::get_debug_navigation_avoidance_enable_obstacles_static()
{
	return data->debug_navigation_avoidance_enable_obstacles_static;
}

void NavigationServer3D::set_debug_navigation_avoidance_agents_radius_color(const Color& p_color)
{
	data->debug_navigation_avoidance_agents_radius_color = p_color;
}

Color NavigationServer3D::get_debug_navigation_avoidance_agents_radius_color()
{
	return data->debug_navigation_avoidance_agents_radius_color;
}

void NavigationServer3D::set_debug_navigation_avoidance_obstacles_radius_color(const Color& p_color)
{
	data->debug_navigation_avoidance_obstacles_radius_color = p_color;
}

Color NavigationServer3D::get_debug_navigation_avoidance_obstacles_radius_color()
{
	return data->debug_navigation_avoidance_obstacles_radius_color;
}

void NavigationServer3D::set_debug_navigation_avoidance_static_obstacle_pushin_face_color(
	const Color& p_color)
{
	data->debug_navigation_avoidance_static_obstacle_pushin_face_color = p_color;
}

Color NavigationServer3D::get_debug_navigation_avoidance_static_obstacle_pushin_face_color()
{
	return data->debug_navigation_avoidance_static_obstacle_pushin_face_color;
}

void NavigationServer3D::set_debug_navigation_avoidance_static_obstacle_pushout_face_color(
	const Color& p_color)
{
	data->debug_navigation_avoidance_static_obstacle_pushout_face_color = p_color;
}

Color NavigationServer3D::get_debug_navigation_avoidance_static_obstacle_pushout_face_color()
{
	return data->debug_navigation_avoidance_static_obstacle_pushout_face_color;
}

void NavigationServer3D::set_debug_navigation_avoidance_static_obstacle_pushin_edge_color(
	const Color& p_color)
{
	data->debug_navigation_avoidance_static_obstacle_pushin_edge_color = p_color;
}

Color NavigationServer3D::get_debug_navigation_avoidance_static_obstacle_pushin_edge_color()
{
	return data->debug_navigation_avoidance_static_obstacle_pushin_edge_color;
}

void NavigationServer3D::set_debug_navigation_avoidance_static_obstacle_pushout_edge_color(
	const Color& p_color)
{
	data->debug_navigation_avoidance_static_obstacle_pushout_edge_color = p_color;
}

Color NavigationServer3D::get_debug_navigation_avoidance_static_obstacle_pushout_edge_color()
{
	return data->debug_navigation_avoidance_static_obstacle_pushout_edge_color;
}

bool NavigationServer3D::get_debug_navigation_enable_agent_paths()
{
	return data->debug_navigation_enable_agent_paths;
}

void NavigationServer3D::set_debug_navigation_enable_agent_paths_xray(const bool p_value)
{
	data->debug_navigation_enable_agent_paths_xray = p_value;
}

bool NavigationServer3D::get_debug_navigation_enable_agent_paths_xray()
{
	return data->debug_navigation_enable_agent_paths_xray;
}

bool NavigationServer3D::get_debug_navigation_enabled() { return data->debug_navigation_enabled; }

bool NavigationServer3D::get_debug_avoidance_enabled() { return data->debug_avoidance_enabled; }

#endif // DEBUG_ENABLED

static NavigationServer3D* navigation_server_3d = nullptr;

void NavigationServer3D::initialize() {}

void NavigationServer3D::finalize() {}

void NavigationServer3D::set_active(bool p_active) {}

void NavigationServer3D::sync() {}

void NavigationServer3D::process(double p_delta_time) {}

void NavigationServer3D::physics_process(double p_delta_time) {}

void NavigationServer3D::free_rid(RID p_rid) {}

int NavigationServer3D::get_process_info(ProcessInfo p_info) { return 0; }

RID NavigationServer3D::map_create() { return RID(); }

RID NavigationServer3D::region_create() { return RID(); }

void NavigationServer3D::region_set_map(RID p_region, RID p_map) {}

void NavigationServer3D::region_set_transform(RID p_region, Transform3D p_transform) {}

void NavigationServer3D::region_set_enabled(RID p_region, bool p_enabled) {}

void NavigationServer3D::region_set_use_edge_connections(RID p_region, bool p_enabled) {}

void NavigationServer3D::region_set_navigation_layers(RID p_region, uint32_t p_navigation_layers) {}

void NavigationServer3D::region_set_enter_cost(RID p_region, real_t p_enter_cost) {}

void NavigationServer3D::region_set_travel_cost(RID p_region, real_t p_travel_cost) {}

RID NavigationServer3D::link_create() { return RID(); }

void NavigationServer3D::link_set_map(RID p_link, RID p_map) {}

void NavigationServer3D::link_set_start_position(RID p_link, Vector3 p_position) {}

void NavigationServer3D::link_set_end_position(RID p_link, Vector3 p_position) {}

void NavigationServer3D::link_set_enabled(RID p_link, bool p_enabled) {}

void NavigationServer3D::link_set_navigation_layers(RID p_link, uint32_t p_navigation_layers) {}

void NavigationServer3D::link_set_enter_cost(RID p_link, real_t p_enter_cost) {}

void NavigationServer3D::link_set_travel_cost(RID p_link, real_t p_travel_cost) {}

RID NavigationServer3D::obstacle_create() { return RID(); }

void NavigationServer3D::obstacle_set_map(RID p_obstacle, RID p_map) {}

void NavigationServer3D::obstacle_set_use_3d_avoidance(RID p_obstacle, bool p_enabled) {}

void NavigationServer3D::obstacle_set_position(RID p_obstacle, Vector3 p_position) {}

void NavigationServer3D::obstacle_set_radius(RID p_obstacle, real_t p_radius) {}

void NavigationServer3D::obstacle_set_vertices(RID p_obstacle, const Vector<Vector3>& p_vertices) {}

void NavigationServer3D::obstacle_set_height(RID p_obstacle, real_t p_height) {}

void NavigationServer3D::obstacle_set_avoidance_enabled(RID p_obstacle, bool p_enabled) {}

void NavigationServer3D::obstacle_set_velocity(RID p_obstacle, Vector3 p_velocity) {}

void NavigationServer3D::obstacle_set_avoidance_layers(RID p_obstacle, uint32_t p_layers) {}

void NavigationServer3D::obstacle_set_paused(RID p_obstacle, bool p_paused) {}

RID NavigationServer3D::agent_create() { return RID(); }

void NavigationServer3D::agent_set_map(RID p_agent, RID p_map) {}

void NavigationServer3D::agent_set_position(RID p_agent, Vector3 p_position) {}

void NavigationServer3D::agent_set_velocity(RID p_agent, Vector3 p_velocity) {}

void NavigationServer3D::agent_set_velocity_forced(RID p_agent, Vector3 p_velocity) {}

void NavigationServer3D::agent_set_radius(RID p_agent, real_t p_radius) {}

void NavigationServer3D::agent_set_height(RID p_agent, real_t p_height) {}

void NavigationServer3D::agent_set_neighbor_distance(RID p_agent, real_t p_distance) {}

void NavigationServer3D::agent_set_max_neighbors(RID p_agent, int p_count) {}

void NavigationServer3D::agent_set_time_horizon_agents(RID p_agent, real_t p_time_horizon) {}

void NavigationServer3D::agent_set_time_horizon_obstacles(RID p_agent, real_t p_time_horizon) {}

void NavigationServer3D::agent_set_max_speed(RID p_agent, real_t p_max_speed) {}

void NavigationServer3D::agent_set_avoidance_layers(RID p_agent, uint32_t p_layers) {}

void NavigationServer3D::agent_set_avoidance_mask(RID p_agent, uint32_t p_mask) {}

void NavigationServer3D::agent_set_avoidance_priority(RID p_agent, real_t p_priority) {}

void NavigationServer3D::agent_set_paused(RID p_agent, bool p_paused) {}


