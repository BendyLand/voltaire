/**************************************************************************/
/*  line_2d.h                                                             */
/**************************************************************************/
/*                         This file is part of:                          */
/*                            VOLTAIRE ENGINE                             */
/*                https://github.com/BendyLand/Voltaire                   */
/**************************************************************************/
/* Copyright (c) 2026-present Bland Logic LLC.                            */
/* Copyright (c) 2014-2026 Godot Engine contributors (see AUTHORS.md).    */
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

#pragma once

#include "scene/2d/node_2d.h"
#include "scene/resources/curve.h"
#include "scene/resources/gradient.h"

class Line2D : public Node2D
{
public:
	enum LineJointMode
	{
		LINE_JOINT_SHARP = 0,
		LINE_JOINT_BEVEL,
		LINE_JOINT_ROUND
	};

	enum LineCapMode
	{
		LINE_CAP_NONE = 0,
		LINE_CAP_BOX,
		LINE_CAP_ROUND
	};

	enum LineTextureMode
	{
		LINE_TEXTURE_NONE = 0,
		LINE_TEXTURE_TILE,
		LINE_TEXTURE_STRETCH
	};

#ifdef DEBUG_ENABLED
	virtual Rect2 _edit_get_rect() const override;
	virtual bool _edit_use_rect() const override;
	virtual bool _edit_is_selected_on_click(
		const Point2& p_point, double p_tolerance) const override;
#endif

	Line2D();

	void set_points(const Vector<Vector2>& p_points);
	Vector<Vector2> get_points() const;

	void set_point_position(int i, Vector2 pos);
	Vector2 get_point_position(int i) const;

	int get_point_count() const;

	void clear_points();

	void add_point(Vector2 pos, int atpos = -1);
	void remove_point(int i);

	void set_closed(bool p_closed);
	bool is_closed() const;

	void set_width(float width);
	float get_width() const;

	void set_curve(const Ref<Curve>& curve);
	Ref<Curve> get_curve() const;

	void set_default_color(Color color);
	Color get_default_color() const;

	void set_gradient(const Ref<Gradient>& gradient);
	Ref<Gradient> get_gradient() const;

	void set_texture(const Ref<Texture2D>& texture);
	Ref<Texture2D> get_texture() const;

	void set_texture_mode(const LineTextureMode mode);
	LineTextureMode get_texture_mode() const;

	void set_joint_mode(LineJointMode mode);
	LineJointMode get_joint_mode() const;

	void set_begin_cap_mode(LineCapMode mode);
	LineCapMode get_begin_cap_mode() const;

	void set_end_cap_mode(LineCapMode mode);
	LineCapMode get_end_cap_mode() const;

	void set_sharp_limit(float limit);
	float get_sharp_limit() const;

	void set_round_precision(int precision);
	int get_round_precision() const;

	void set_antialiased(bool p_antialiased);
	bool get_antialiased() const;

protected:
	void _notification(int p_what);
	void _draw();


private:
	void _gradient_changed();
	void _curve_changed();

private:
	Vector<Vector2> _points;
	LineJointMode _joint_mode = LINE_JOINT_SHARP;
	LineCapMode _begin_cap_mode = LINE_CAP_NONE;
	LineCapMode _end_cap_mode = LINE_CAP_NONE;
	bool _closed = false;
	float _width = 10.0;
	Ref<Curve> _curve;
	Color _default_color = Color(1, 1, 1);
	Ref<Gradient> _gradient;
	Ref<Texture2D> _texture;
	LineTextureMode _texture_mode = LINE_TEXTURE_NONE;
	float _sharp_limit = 2.f;
	int _round_precision = 8;
	bool _antialiased = false;
};

// --- Folded from scene/2d/light_occluder_2d.h ---
#include "scene/2d/node_2d.h"

class OccluderPolygon2D : public Resource
{
public:
	enum CullMode
	{
		CULL_DISABLED,
		CULL_CLOCKWISE,
		CULL_COUNTER_CLOCKWISE
	};

private:
	RID occ_polygon;
	Vector<Vector2> polygon;
	bool closed = true;
	CullMode cull = CULL_DISABLED;

	mutable Rect2 item_rect;
	mutable bool rect_cache_dirty = true;

protected:

public:
#ifdef DEBUG_ENABLED
	virtual Rect2 _edit_get_rect() const;
	virtual bool _edit_is_selected_on_click(const Point2& p_point, double p_tolerance) const;
#endif // DEBUG_ENABLED
	void set_polygon(const Vector<Vector2>& p_polygon);
	Vector<Vector2> get_polygon() const;

	void set_closed(bool p_closed);
	bool is_closed() const;

	void set_cull_mode(CullMode p_mode);
	CullMode get_cull_mode() const;

	virtual RID get_rid() const override;
	OccluderPolygon2D();
	~OccluderPolygon2D();
};


class LightOccluder2D : public Node2D
{
	RID occluder;
	int mask = 1;
	Ref<OccluderPolygon2D> occluder_polygon;
	bool sdf_collision = false;
	void _poly_changed();

	virtual void _physics_interpolated_changed() override;

protected:
	void _notification(int p_what);

public:
#ifdef DEBUG_ENABLED
	virtual Rect2 _edit_get_rect() const override;
	virtual bool _edit_is_selected_on_click(
		const Point2& p_point, double p_tolerance) const override;
#endif // DEBUG_ENABLED

	void set_occluder_polygon(const Ref<OccluderPolygon2D>& p_polygon);
	Ref<OccluderPolygon2D> get_occluder_polygon() const;

	void set_occluder_light_mask(int p_mask);
	int get_occluder_light_mask() const;

	void set_as_sdf_collision(bool p_enable);
	bool is_set_as_sdf_collision() const;

	PackedStringArray get_configuration_warnings() const override;

	LightOccluder2D();
	~LightOccluder2D();
};



// --- End scene/2d/light_occluder_2d.h ---

// --- Folded from scene/2d/parallax_2d.h ---
#include "scene/2d/node_2d.h"

class Parallax2D : public Node2D
{
	static constexpr real_t DEFAULT_LIMIT = 10000000;

	String group_name;
	Size2 scroll_scale = Size2(1, 1);
	Point2 scroll_offset;
	Point2 screen_offset;
	Vector2 repeat_size;
	int repeat_times = 1;
	Point2 limit_begin = Point2(-DEFAULT_LIMIT, -DEFAULT_LIMIT);
	Point2 limit_end = Point2(DEFAULT_LIMIT, DEFAULT_LIMIT);
	Point2 autoscroll;
	bool follow_viewport = true;
	bool ignore_camera_scroll = false;

	void _update_process();
	void _update_repeat();
	void _update_scroll();

protected:
#ifdef TOOLS_ENABLED
	void _edit_set_position(const Point2& p_position) override;
#endif // TOOLS_ENABLED
	void _camera_moved(const Transform2D& p_transform, const Point2& p_screen_offset,
		const Point2& p_adj_screen_offset);
	void _notification(int p_what);

public:
	void set_scroll_scale(const Size2& p_scale);
	Size2 get_scroll_scale() const;

	void set_repeat_size(const Size2& p_repeat_size);
	Size2 get_repeat_size() const;

	void set_repeat_times(int p_repeat_times);
	int get_repeat_times() const;

	void set_autoscroll(const Point2& p_autoscroll);
	Point2 get_autoscroll() const;

	void set_scroll_offset(const Point2& p_offset);
	Point2 get_scroll_offset() const;

	void set_screen_offset(const Point2& p_offset);
	Point2 get_screen_offset() const;

	void set_limit_begin(const Point2& p_offset);
	Point2 get_limit_begin() const;

	void set_limit_end(const Point2& p_offset);
	Point2 get_limit_end() const;

	void set_follow_viewport(bool p_follow);
	bool get_follow_viewport();

	void set_ignore_camera_scroll(bool p_ignore);
	bool is_ignore_camera_scroll();

	Parallax2D() = default;
};



// --- End scene/2d/parallax_2d.h ---

// --- Folded from scene/2d/parallax_layer.h ---
#include "scene/2d/node_2d.h"

class ParallaxLayer : public Node2D
{
	Point2 orig_offset;
	Point2 orig_scale;
	Size2 motion_scale = Size2(1, 1);
	Vector2 motion_offset;
	Vector2 mirroring;
	void _update_mirroring();

protected:
	void _notification(int p_what);

public:
	void set_motion_offset(const Size2& p_offset);
	Size2 get_motion_offset() const;

	void set_motion_scale(const Size2& p_scale);
	Size2 get_motion_scale() const;

	void set_mirroring(const Size2& p_mirroring);
	Size2 get_mirroring() const;

	void set_base_offset_and_scale(const Point2& p_offset, real_t p_scale);

	PackedStringArray get_configuration_warnings() const override;
	ParallaxLayer() = default;
};



// --- End scene/2d/parallax_layer.h ---

// --- Folded from scene/2d/path_2d.h ---
#include "scene/2d/node_2d.h"
#include "scene/resources/curve.h"

class Timer;

class Path2D : public Node2D
{
	Ref<Curve2D> curve;

	void _curve_changed();

#ifdef DEBUG_ENABLED
	RID debug_mesh_rid;
	RID debug_instance;

	void _debug_create();
	void _debug_update();
	void _debug_free();
#endif // DEBUG_ENABLED

protected:
	void _notification(int p_what);

public:
#ifdef DEBUG_ENABLED
	virtual Rect2 _edit_get_rect() const override;
	virtual bool _edit_use_rect() const override;
	virtual bool _edit_is_selected_on_click(
		const Point2& p_point, double p_tolerance) const override;
#endif

	void set_curve(const Ref<Curve2D>& p_curve);
	Ref<Curve2D> get_curve() const;
};

class PathFollow2D : public Node2D
{
private:
	Path2D* path = nullptr;
	real_t progress = 0.0;
	Timer* update_timer = nullptr;
	real_t h_offset = 0.0;
	real_t v_offset = 0.0;
	bool cubic = true;
	bool loop = true;
	bool rotates = true;

	void _update_transform();

protected:
	void _notification(int p_what);

public:
	void set_progress(real_t p_progress);
	real_t get_progress() const;

	void set_h_offset(real_t p_h_offset);
	real_t get_h_offset() const;

	void set_v_offset(real_t p_v_offset);
	real_t get_v_offset() const;

	void set_progress_ratio(real_t p_ratio);
	real_t get_progress_ratio() const;

	void set_loop(bool p_loop);
	bool has_loop() const;

	void set_rotation_enabled(bool p_enabled);
	bool is_rotation_enabled() const;

	void set_cubic_interpolation_enabled(bool p_enabled);
	bool is_cubic_interpolation_enabled() const;

	PackedStringArray get_configuration_warnings() const override;
};



// --- End scene/2d/path_2d.h ---

// --- Folded from scene/2d/navigation/navigation_2d.h ---
#include "scene/2d/node_2d.h"
#include "scene/main/node.h"
#include "servers/navigation_2d/navigation_server_2d.h"

class NavigationMeshSourceGeometryData2D;

class NavigationAgent2D final : public Node
{
	Node2D* agent_parent = nullptr;

	RID agent;
	RID map_override;

	bool avoidance_enabled = false;
	uint32_t avoidance_layers = 1;
	uint32_t avoidance_mask = 1;
	real_t avoidance_priority = 1.0;
	uint32_t navigation_layers = 1;
	NavigationPathQueryParameters2D::PathfindingAlgorithm pathfinding_algorithm =
		NavigationPathQueryParameters2D::PathfindingAlgorithm::PATHFINDING_ALGORITHM_ASTAR;
	NavigationPathQueryParameters2D::PathPostProcessing path_postprocessing =
		NavigationPathQueryParameters2D::PathPostProcessing::PATH_POSTPROCESSING_CORRIDORFUNNEL;
	uint32_t path_metadata_flags =
		NavigationPathQueryParameters2D::PathMetadataFlags::PATH_METADATA_INCLUDE_ALL;

	real_t path_desired_distance = 20.0;
	real_t target_desired_distance = 10.0;
	real_t radius = NavigationDefaults2D::AVOIDANCE_AGENT_RADIUS;
	real_t neighbor_distance = NavigationDefaults2D::AVOIDANCE_AGENT_NEIGHBOR_DISTANCE;
	int max_neighbors = NavigationDefaults2D::AVOIDANCE_AGENT_MAX_NEIGHBORS;
	real_t time_horizon_agents = NavigationDefaults2D::AVOIDANCE_AGENT_TIME_HORIZON_AGENTS;
	real_t time_horizon_obstacles = NavigationDefaults2D::AVOIDANCE_AGENT_TIME_HORIZON_OBSTACLES;
	real_t max_speed = NavigationDefaults2D::AVOIDANCE_AGENT_MAX_SPEED;
	real_t path_max_distance = 100.0;
	bool simplify_path = false;
	real_t simplify_epsilon = 0.0;
	float path_return_max_length = 0.0;
	float path_return_max_radius = 0.0;
	int path_search_max_polygons = NavigationDefaults2D::path_search_max_polygons;
	float path_search_max_distance = 0.0;

	Vector2 target_position;

	Ref<NavigationPathQueryParameters2D> navigation_query;
	Ref<NavigationPathQueryResult2D> navigation_result;
	int navigation_path_index = 0;

	Vector2 safe_velocity;

	Vector2 velocity;
	bool velocity_submitted = false;

	Vector2 velocity_forced;
	bool velocity_forced_submitted = false;

	bool target_position_submitted = false;

	bool target_reached = false;
	bool navigation_finished = true;
	bool last_waypoint_reached = false;

	bool debug_enabled = false;
	float debug_path_custom_point_size = 4.0;
	float debug_path_custom_line_width = -1.0;
	bool debug_use_custom = false;
	Color debug_path_custom_color = Color(1.0, 1.0, 1.0, 1.0);

#ifdef DEBUG_ENABLED
	bool debug_path_dirty = true;
	RID debug_path_instance;
#endif // DEBUG_ENABLED

protected:
	void _notification(int p_what);

public:
	NavigationAgent2D();
	~NavigationAgent2D();

	RID get_rid() const { return agent; }

	void set_avoidance_enabled(bool p_enabled);
	bool get_avoidance_enabled() const;

	void set_agent_parent(Node* p_agent_parent);

	void set_navigation_layers(uint32_t p_navigation_layers);
	uint32_t get_navigation_layers() const;

	void set_navigation_layer_value(int p_layer_number, bool p_value);
	bool get_navigation_layer_value(int p_layer_number) const;

	void set_pathfinding_algorithm(
		const NavigationPathQueryParameters2D::PathfindingAlgorithm p_pathfinding_algorithm);

	NavigationPathQueryParameters2D::PathfindingAlgorithm get_pathfinding_algorithm() const
	{
		return pathfinding_algorithm;
	}

	void set_path_postprocessing(
		const NavigationPathQueryParameters2D::PathPostProcessing p_path_postprocessing);

	NavigationPathQueryParameters2D::PathPostProcessing get_path_postprocessing() const
	{
		return path_postprocessing;
	}

	void set_path_metadata_flags(uint32_t p_flags);

	uint32_t get_path_metadata_flags() const { return path_metadata_flags; }

	void set_navigation_map(RID p_navigation_map);
	RID get_navigation_map() const;

	void set_path_desired_distance(real_t p_dd);

	real_t get_path_desired_distance() const { return path_desired_distance; }

	void set_target_desired_distance(real_t p_dd);

	real_t get_target_desired_distance() const { return target_desired_distance; }

	void set_radius(real_t p_radius);

	real_t get_radius() const { return radius; }

	void set_neighbor_distance(real_t p_distance);

	real_t get_neighbor_distance() const { return neighbor_distance; }

	void set_max_neighbors(int p_count);

	int get_max_neighbors() const { return max_neighbors; }

	void set_time_horizon_agents(real_t p_time_horizon);

	real_t get_time_horizon_agents() const { return time_horizon_agents; }

	void set_time_horizon_obstacles(real_t p_time_horizon);

	real_t get_time_horizon_obstacles() const { return time_horizon_obstacles; }

	void set_max_speed(real_t p_max_speed);

	real_t get_max_speed() const { return max_speed; }

	void set_path_max_distance(real_t p_pmd);
	real_t get_path_max_distance();

	void set_target_position(Vector2 p_position);
	Vector2 get_target_position() const;

	void set_simplify_path(bool p_enabled);
	bool get_simplify_path() const;

	void set_simplify_epsilon(real_t p_epsilon);
	real_t get_simplify_epsilon() const;

	void set_path_return_max_length(float p_length);
	float get_path_return_max_length() const;

	void set_path_return_max_radius(float p_radius);
	float get_path_return_max_radius() const;

	void set_path_search_max_polygons(int p_max_polygons);
	int get_path_search_max_polygons() const;

	void set_path_search_max_distance(float p_distance);
	float get_path_search_max_distance() const;

	float get_path_length() const;

	Vector2 get_next_path_position();

	Ref<NavigationPathQueryResult2D> get_current_navigation_result() const
	{
		return navigation_result;
	}

	const Vector<Vector2>& get_current_navigation_path() const
	{
		return navigation_result->get_path();
	}

	int get_current_navigation_path_index() const { return navigation_path_index; }

	real_t distance_to_target() const;
	bool is_target_reached() const;
	bool is_target_reachable();
	bool is_navigation_finished();
	Vector2 get_final_position();

	void set_velocity(const Vector2 p_velocity);

	Vector2 get_velocity() { return velocity; }

	void set_velocity_forced(const Vector2 p_velocity);

	void _avoidance_done(Vector2 p_new_velocity);

	PackedStringArray get_configuration_warnings() const override;

	void set_avoidance_layers(uint32_t p_layers);
	uint32_t get_avoidance_layers() const;

	void set_avoidance_mask(uint32_t p_mask);
	uint32_t get_avoidance_mask() const;

	void set_avoidance_layer_value(int p_layer_number, bool p_value);
	bool get_avoidance_layer_value(int p_layer_number) const;

	void set_avoidance_mask_value(int p_mask_number, bool p_value);
	bool get_avoidance_mask_value(int p_mask_number) const;

	void set_avoidance_priority(real_t p_priority);
	real_t get_avoidance_priority() const;

	void set_debug_enabled(bool p_enabled);
	bool get_debug_enabled() const;

	void set_debug_use_custom(bool p_enabled);
	bool get_debug_use_custom() const;

	void set_debug_path_custom_color(Color p_color);
	Color get_debug_path_custom_color() const;

	void set_debug_path_custom_point_size(float p_point_size);
	float get_debug_path_custom_point_size() const;

	void set_debug_path_custom_line_width(float p_line_width);
	float get_debug_path_custom_line_width() const;

private:
	bool _is_target_reachable() const;
	Vector2 _get_final_position() const;

	void _update_navigation();
	void _advance_waypoints(const Vector2& p_origin);
	void _request_repath();

	bool _is_last_waypoint() const;
	void _move_to_next_waypoint();
	bool _is_within_waypoint_distance(const Vector2& p_origin) const;
	bool _is_within_target_distance(const Vector2& p_origin) const;

	void _trigger_waypoint_reached();
	void _transition_to_navigation_finished();
	void _transition_to_target_reached();

#ifdef DEBUG_ENABLED
	void _navigation_debug_changed();
	void _update_debug_path();
#endif // DEBUG_ENABLED
};

class NavigationLink2D final : public Node2D
{
	bool enabled = true;
	RID link;
	RID map_override;
	bool bidirectional = true;
	uint32_t navigation_layers = 1;
	Vector2 end_position;
	Vector2 start_position;
	real_t enter_cost = 0.0;
	real_t travel_cost = 1.0;

#ifdef DEBUG_ENABLED
	void _update_debug_mesh();
#endif // DEBUG_ENABLED

protected:
	void _notification(int p_what);

public:
#ifdef DEBUG_ENABLED
	Rect2 _edit_get_rect() const override;
	bool _edit_is_selected_on_click(
		const Point2& p_point, double p_tolerance) const override;
#endif // DEBUG_ENABLED
	RID get_rid() const;

	void set_enabled(bool p_enabled);

	bool is_enabled() const { return enabled; }

	void set_navigation_map(RID p_navigation_map);
	RID get_navigation_map() const;

	void set_bidirectional(bool p_bidirectional);

	bool is_bidirectional() const { return bidirectional; }

	void set_navigation_layers(uint32_t p_navigation_layers);

	uint32_t get_navigation_layers() const { return navigation_layers; }

	void set_navigation_layer_value(int p_layer_number, bool p_value);
	bool get_navigation_layer_value(int p_layer_number) const;

	void set_start_position(Vector2 p_position);

	Vector2 get_start_position() const { return start_position; }

	void set_end_position(Vector2 p_position);

	Vector2 get_end_position() const { return end_position; }

	void set_global_start_position(Vector2 p_position);
	Vector2 get_global_start_position() const;

	void set_global_end_position(Vector2 p_position);
	Vector2 get_global_end_position() const;

	void set_enter_cost(real_t p_enter_cost);

	real_t get_enter_cost() const { return enter_cost; }

	void set_travel_cost(real_t p_travel_cost);

	real_t get_travel_cost() const { return travel_cost; }

	PackedStringArray get_configuration_warnings() const override;

	NavigationLink2D();
	~NavigationLink2D();

private:
	void _link_enter_navigation_map();
	void _link_exit_navigation_map();
	void _link_update_transform();
};

class NavigationObstacle2D final : public Node2D
{
	RID obstacle;
	RID map_before_pause;
	RID map_override;
	RID map_current;

	real_t radius = 0.0;

	Vector<Vector2> vertices;
	bool vertices_are_clockwise = true;
	bool vertices_are_valid = true;

	bool avoidance_enabled = true;
	uint32_t avoidance_layers = 1;

	Transform2D previous_transform;

	Vector2 velocity;
	Vector2 previous_velocity;
	bool velocity_submitted = false;

	bool affect_navigation_mesh = false;
	bool carve_navigation_mesh = false;

#ifdef DEBUG_ENABLED
private:
	RID debug_canvas_item;
	RID debug_mesh_rid;

	void _update_fake_agent_radius_debug();
	void _update_static_obstacle_debug();
#endif // DEBUG_ENABLED

protected:
	void _notification(int p_what);

public:
	NavigationObstacle2D();
	~NavigationObstacle2D();

	RID get_rid() const { return obstacle; }

	void set_avoidance_enabled(bool p_enabled);
	bool get_avoidance_enabled() const;

	void set_navigation_map(RID p_navigation_map);
	RID get_navigation_map() const;

	void set_radius(real_t p_radius);

	real_t get_radius() const { return radius; }

	void set_vertices(const Vector<Vector2>& p_vertices);

	const Vector<Vector2>& get_vertices() const { return vertices; }

	bool are_vertices_clockwise() const { return vertices_are_clockwise; }

	bool are_vertices_valid() const { return vertices_are_valid; }

	void set_avoidance_layers(uint32_t p_layers);
	uint32_t get_avoidance_layers() const;

	void set_avoidance_mask(uint32_t p_mask);
	uint32_t get_avoidance_mask() const;

	void set_avoidance_layer_value(int p_layer_number, bool p_value);
	bool get_avoidance_layer_value(int p_layer_number) const;

	void set_velocity(const Vector2 p_velocity);

	Vector2 get_velocity() const { return velocity; }

	void _avoidance_done(Vector3 p_new_velocity); // Dummy

	void set_affect_navigation_mesh(bool p_enabled);
	bool get_affect_navigation_mesh() const;

	void set_carve_navigation_mesh(bool p_enabled);
	bool get_carve_navigation_mesh() const;

	PackedStringArray get_configuration_warnings() const override;

private:
	static RID _navmesh_source_geometry_parser;

public:
	static void navmesh_parse_init();
	static void navmesh_parse_source_geometry(const Ref<NavigationPolygon>& p_navigation_mesh,
		Ref<NavigationMeshSourceGeometryData2D> p_source_geometry_data, Node* p_node);

private:
	void _update_map(RID p_map);
	void _update_position(const Vector2 p_position);
	void _update_transform();
};

class NavigationRegion2D final : public Node2D
{
	bool enabled = true;
	bool use_edge_connections = true;

	RID region;
	RID map_override;
	uint32_t navigation_layers = 1;
	real_t enter_cost = 0.0;
	real_t travel_cost = 1.0;
	Ref<NavigationPolygon> navigation_polygon;

	void _navigation_polygon_changed();

	Rect2 bounds;

#ifdef DEBUG_ENABLED
private:
	RID debug_mesh_rid;
	RID debug_instance_rid;

	bool debug_mesh_dirty = true;

	void _set_debug_visible(bool p_visible);
	void _update_debug_mesh();
	void _update_debug_edge_connections_mesh();
	void _update_debug_baking_rect();
	void _navigation_map_changed(RID p_map);
	void _navigation_debug_changed();
#endif // DEBUG_ENABLED

protected:
	void _notification(int p_what);

public:
#ifdef DEBUG_ENABLED
	Rect2 _edit_get_rect() const override;
	bool _edit_is_selected_on_click(
		const Point2& p_point, double p_tolerance) const override;
#endif // DEBUG_ENABLED
	RID get_rid() const;

	void set_enabled(bool p_enabled);
	bool is_enabled() const;

	void set_navigation_map(RID p_navigation_map);
	RID get_navigation_map() const;

	void set_use_edge_connections(bool p_enabled);
	bool get_use_edge_connections() const;

	void set_navigation_layers(uint32_t p_navigation_layers);
	uint32_t get_navigation_layers() const;

	void set_navigation_layer_value(int p_layer_number, bool p_value);
	bool get_navigation_layer_value(int p_layer_number) const;

	RID get_region_rid() const;

	void set_enter_cost(real_t p_enter_cost);
	real_t get_enter_cost() const;

	void set_travel_cost(real_t p_travel_cost);
	real_t get_travel_cost() const;

	void set_navigation_polygon(const Ref<NavigationPolygon>& p_navigation_polygon);
	Ref<NavigationPolygon> get_navigation_polygon() const;

	PackedStringArray get_configuration_warnings() const override;

	void bake_navigation_polygon(bool p_on_thread);
	void _bake_finished();
	bool is_baking() const;

	Rect2 get_bounds() const { return bounds; }

	NavigationRegion2D();
	~NavigationRegion2D();

private:
	void _update_bounds();
	void _region_enter_navigation_map();
	void _region_exit_navigation_map();
	void _region_update_transform();
};

// --- Folded from scene/resources/2d/tile_set.h ---
#include "core/io/resource.h"
#include "core/templates/local_vector.h"
#include "core/templates/rb_map.h"
#include "core/templates/rb_set.h"
#include "scene/main/canvas_item.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/packed_scene.h"
#include "scene/resources/material.h"

#ifndef PHYSICS_2D_DISABLED
#include "scene/2d/node_2d.h"
#endif // PHYSICS_2D_DISABLED

#ifndef NAVIGATION_2D_DISABLED
#include "servers/navigation_2d/navigation_server_2d.h"
#endif // NAVIGATION_2D_DISABLED

#ifndef DISABLE_DEPRECATED
#include "scene/resources/shader.h"
#endif

class ArrayMesh;
class TileMap;
class TileSetSource;
class TileSetAtlasSource;
class TileData;

union TileMapCell
{
	struct
	{
		int16_t source_id;
		int16_t coord_x;
		int16_t coord_y;
		int16_t alternative_tile;
	};

	uint64_t _u64t;

	static uint32_t hash(const TileMapCell& p_hash) { return hash_one_uint64(p_hash._u64t); }

	TileMapCell(int p_source_id = -1, Vector2i p_atlas_coords = Vector2i(-1, -1),
		int p_alternative_tile = -1)
	{ // default are INVALID_SOURCE, INVALID_ATLAS_COORDS, INVALID_TILE_ALTERNATIVE
		source_id = p_source_id;
		set_atlas_coords(p_atlas_coords);
		alternative_tile = p_alternative_tile;
	}

	Vector2i get_atlas_coords() const { return Vector2i(coord_x, coord_y); }

	void set_atlas_coords(const Vector2i& r_coords)
	{
		coord_x = r_coords.x;
		coord_y = r_coords.y;
	}

	bool operator<(const TileMapCell& p_other) const
	{
		if (source_id == p_other.source_id) {
			if (coord_x == p_other.coord_x) {
				if (coord_y == p_other.coord_y) {
					return alternative_tile < p_other.alternative_tile;
				}
				else {
					return coord_y < p_other.coord_y;
				}
			}
			else {
				return coord_x < p_other.coord_x;
			}
		}
		else {
			return source_id < p_other.source_id;
		}
	}

	bool operator!=(const TileMapCell& p_other) const
	{
		return !(source_id == p_other.source_id && coord_x == p_other.coord_x &&
				 coord_y == p_other.coord_y && alternative_tile == p_other.alternative_tile);
	}

	bool operator==(const TileMapCell& p_other) const
	{
		return source_id == p_other.source_id && coord_x == p_other.coord_x &&
			   coord_y == p_other.coord_y && alternative_tile == p_other.alternative_tile;
	}
};

class TileMapPattern : public Resource
{
	Size2i size;
	HashMap<Vector2i, TileMapCell> pattern;

	void _set_tile_data(const Vector<int>& p_data);
	Vector<int> _get_tile_data() const;

public:
	void set_cell(const Vector2i& p_coords, int p_source_id, const Vector2i p_atlas_coords,
		int p_alternative_tile = 0);
	bool has_cell(const Vector2i& p_coords) const;
	void remove_cell(const Vector2i& p_coords, bool p_update_size = true);
	int get_cell_source_id(const Vector2i& p_coords) const;
	Vector2i get_cell_atlas_coords(const Vector2i& p_coords) const;
	int get_cell_alternative_tile(const Vector2i& p_coords) const;

	const HashMap<Vector2i, TileMapCell>& get_pattern() const { return pattern; }

	TypedArray<Vector2i> get_used_cells() const;

	Size2i get_size() const;
	void set_size(const Size2i& p_size);
	bool is_empty() const;

	void clear();
};

class TileSet : public Resource
{
#ifndef DISABLE_DEPRECATED
private:
	struct CompatibilityShapeData
	{
		Vector2i autotile_coords;
		bool one_way = false;
		float one_way_margin = 0.0f;
#ifndef PHYSICS_2D_DISABLED
		Ref<Shape2D> shape;
#endif // PHYSICS_2D_DISABLED
		Transform2D transform;
	};

	struct CompatibilityTileData
	{
		String name;
		Ref<Texture2D> texture;
		Vector2 tex_offset;
		Ref<Material> material;
		Rect2 region;
		int tile_mode = 0;
		Color modulate = Color(1, 1, 1);

		// Atlas or autotiles data
		int autotile_bitmask_mode = 0;
		Vector2 autotile_icon_coordinate;
		Size2i autotile_tile_size = Size2i(16, 16);

		int autotile_spacing = 0;
		HashMap<Vector2i, int> autotile_bitmask_flags;
		HashMap<Vector2i, Ref<OccluderPolygon2D>> autotile_occluder_map;
#ifndef NAVIGATION_2D_DISABLED
		HashMap<Vector2i, Ref<NavigationPolygon>> autotile_navpoly_map;
#endif // NAVIGATION_2D_DISABLED
		HashMap<Vector2i, int> autotile_priority_map;
		HashMap<Vector2i, int> autotile_z_index_map;

		Vector<CompatibilityShapeData> shapes;
		Ref<OccluderPolygon2D> occluder;
		Vector2 occluder_offset;
#ifndef NAVIGATION_2D_DISABLED
		Ref<NavigationPolygon> navigation;
#endif // NAVIGATION_2D_DISABLED
		Vector2 navigation_offset;
		int z_index = 0;
	};

	enum CompatibilityTileMode
	{
		COMPATIBILITY_TILE_MODE_SINGLE_TILE = 0,
		COMPATIBILITY_TILE_MODE_AUTO_TILE,
		COMPATIBILITY_TILE_MODE_ATLAS_TILE,
	};

	HashMap<int, CompatibilityTileData*> compatibility_data;
	HashMap<int, int> compatibility_tilemap_mapping_tile_modes;
	HashMap<Vector2i, int> compatibility_size_count;

	void _compatibility_conversion();
#endif // DISABLE_DEPRECATED

public:
	static const int INVALID_SOURCE; // -1;

	enum CellNeighbor
	{
		CELL_NEIGHBOR_RIGHT_SIDE = 0,
		CELL_NEIGHBOR_RIGHT_CORNER,
		CELL_NEIGHBOR_BOTTOM_RIGHT_SIDE,
		CELL_NEIGHBOR_BOTTOM_RIGHT_CORNER,
		CELL_NEIGHBOR_BOTTOM_SIDE,
		CELL_NEIGHBOR_BOTTOM_CORNER,
		CELL_NEIGHBOR_BOTTOM_LEFT_SIDE,
		CELL_NEIGHBOR_BOTTOM_LEFT_CORNER,
		CELL_NEIGHBOR_LEFT_SIDE,
		CELL_NEIGHBOR_LEFT_CORNER,
		CELL_NEIGHBOR_TOP_LEFT_SIDE,
		CELL_NEIGHBOR_TOP_LEFT_CORNER,
		CELL_NEIGHBOR_TOP_SIDE,
		CELL_NEIGHBOR_TOP_CORNER,
		CELL_NEIGHBOR_TOP_RIGHT_SIDE,
		CELL_NEIGHBOR_TOP_RIGHT_CORNER,
		CELL_NEIGHBOR_MAX,
	};

	static const char* CELL_NEIGHBOR_ENUM_TO_TEXT[];

	enum TerrainMode
	{
		TERRAIN_MODE_MATCH_CORNERS_AND_SIDES = 0,
		TERRAIN_MODE_MATCH_CORNERS,
		TERRAIN_MODE_MATCH_SIDES,
	};

	enum TileShape
	{
		TILE_SHAPE_SQUARE,
		TILE_SHAPE_ISOMETRIC,
		TILE_SHAPE_HALF_OFFSET_SQUARE,
		TILE_SHAPE_HEXAGON,
	};

	enum TileLayout
	{
		TILE_LAYOUT_STACKED,
		TILE_LAYOUT_STACKED_OFFSET,
		TILE_LAYOUT_STAIRS_RIGHT,
		TILE_LAYOUT_STAIRS_DOWN,
		TILE_LAYOUT_DIAMOND_RIGHT,
		TILE_LAYOUT_DIAMOND_DOWN,
	};

	enum TileOffsetAxis
	{
		TILE_OFFSET_AXIS_HORIZONTAL,
		TILE_OFFSET_AXIS_VERTICAL,
	};

	struct PackedSceneSource
	{
		Ref<PackedScene> scene;
		Vector2 offset;
	};

	class TerrainsPattern
	{
		bool valid = false;
		int terrain = -1;
		int bits[TileSet::CELL_NEIGHBOR_MAX] = {};
		bool is_valid_bit[TileSet::CELL_NEIGHBOR_MAX] = {};

		int not_empty_terrains_count = 0;

	public:
		bool is_valid() const;
		bool is_erase_pattern() const;

		bool operator<(const TerrainsPattern& p_terrains_pattern) const;
		bool operator==(const TerrainsPattern& p_terrains_pattern) const;

		bool operator!=(const TerrainsPattern& p_terrains_pattern) const
		{
			return !operator==(p_terrains_pattern);
		}

		void set_terrain(int p_terrain);
		int get_terrain() const;

		void set_terrain_peering_bit(TileSet::CellNeighbor p_peering_bit, int p_terrain);
		int get_terrain_peering_bit(TileSet::CellNeighbor p_peering_bit) const;

		TerrainsPattern(const TileSet* p_tile_set, int p_terrain_set);

		TerrainsPattern() {}
	};

private:
	// --- TileSet data ---
	// Basic shape and layout.
	TileShape tile_shape = TILE_SHAPE_SQUARE;
	TileLayout tile_layout = TILE_LAYOUT_STACKED;
	TileOffsetAxis tile_offset_axis = TILE_OFFSET_AXIS_HORIZONTAL;
	Size2i tile_size = Size2i(16, 16); // Size2(64, 64);

	// Rendering.
	bool uv_clipping = false;

	struct OcclusionLayer
	{
		uint32_t light_mask = 1;
		bool sdf_collision = false;
	};

	Vector<OcclusionLayer> occlusion_layers;

	Ref<ArrayMesh> tile_lines_mesh;
	Ref<ArrayMesh> tile_filled_mesh;
	mutable bool tile_meshes_dirty = true;

#ifndef PHYSICS_2D_DISABLED
	// Physics
	struct PhysicsLayer
	{
		uint32_t collision_layer = 1;
		uint32_t collision_mask = 1;
		real_t collision_priority = 1.0;
		Ref<PhysicsMaterial> physics_material;
	};

	Vector<PhysicsLayer> physics_layers;
#endif // PHYSICS_2D_DISABLED

	// Terrains
	struct Terrain
	{
		String name;
		Color color;
	};

	struct TerrainSet
	{
		TerrainMode mode = TERRAIN_MODE_MATCH_CORNERS_AND_SIDES;
		Vector<Terrain> terrains;
	};

	Vector<TerrainSet> terrain_sets;

	HashMap<TerrainMode, Ref<ArrayMesh>> terrain_meshes;
	HashMap<TerrainMode, HashMap<CellNeighbor, Ref<ArrayMesh>>> terrain_peering_bits_meshes;
	bool terrain_bits_meshes_dirty = true;

	LocalVector<RBMap<TileSet::TerrainsPattern, RBSet<TileMapCell>>>
		per_terrain_pattern_tiles; // Cached data.
	bool terrains_cache_dirty = true;
	void _update_terrains_cache();

	// Navigation
	struct NavigationLayer
	{
		uint32_t layers = 1;
	};

	Vector<NavigationLayer> navigation_layers;

	HashMap<String, int> custom_data_layers_by_name;

	// Per Atlas source data.
	HashMap<int, Ref<TileSetSource>> sources;
	Vector<int> source_ids;
	int next_source_id = 0;
	// ---------------------

	LocalVector<Ref<TileMapPattern>> patterns;

	void _compute_next_source_id();
	void _source_changed();

	// Tile proxies
	RBMap<int, int> source_level_proxies;

	// Helpers
	Vector<Point2> _get_square_terrain_polygon(Vector2i p_size);
	Vector<Point2> _get_square_corner_or_side_terrain_peering_bit_polygon(
		Vector2i p_size, TileSet::CellNeighbor p_bit);
	Vector<Point2> _get_square_corner_terrain_peering_bit_polygon(
		Vector2i p_size, TileSet::CellNeighbor p_bit);
	Vector<Point2> _get_square_side_terrain_peering_bit_polygon(
		Vector2i p_size, TileSet::CellNeighbor p_bit);

	Vector<Point2> _get_isometric_terrain_polygon(Vector2i p_size);
	Vector<Point2> _get_isometric_corner_or_side_terrain_peering_bit_polygon(
		Vector2i p_size, TileSet::CellNeighbor p_bit);
	Vector<Point2> _get_isometric_corner_terrain_peering_bit_polygon(
		Vector2i p_size, TileSet::CellNeighbor p_bit);
	Vector<Point2> _get_isometric_side_terrain_peering_bit_polygon(
		Vector2i p_size, TileSet::CellNeighbor p_bit);

	Vector<Point2> _get_half_offset_terrain_polygon(
		Vector2i p_size, float p_overlap, TileSet::TileOffsetAxis p_offset_axis);
	Vector<Point2> _get_half_offset_corner_or_side_terrain_peering_bit_polygon(Vector2i p_size,
		float p_overlap, TileSet::TileOffsetAxis p_offset_axis, TileSet::CellNeighbor p_bit);
	Vector<Point2> _get_half_offset_corner_terrain_peering_bit_polygon(Vector2i p_size,
		float p_overlap, TileSet::TileOffsetAxis p_offset_axis, TileSet::CellNeighbor p_bit);
	Vector<Point2> _get_half_offset_side_terrain_peering_bit_polygon(Vector2i p_size,
		float p_overlap, TileSet::TileOffsetAxis p_offset_axis, TileSet::CellNeighbor p_bit);

protected:
public:
	// --- Accessors for TileSet data ---

	// -- Shape and layout --
	void set_tile_shape(TileShape p_shape);
	TileShape get_tile_shape() const;
	void set_tile_layout(TileLayout p_layout);
	TileLayout get_tile_layout() const;
	void set_tile_offset_axis(TileOffsetAxis p_alignment);
	TileOffsetAxis get_tile_offset_axis() const;
	void set_tile_size(Size2i p_size);
	Size2i get_tile_size() const;

	// -- Sources management --
	int get_next_source_id() const;
	int get_source_count() const;
	int get_source_id(int p_index) const;
	int add_source(Ref<TileSetSource> p_tile_set_source, int p_source_id_override = -1);
	void set_source_id(int p_source_id, int p_new_id);
	void remove_source(int p_source_id);
	void remove_source_ptr(TileSetSource* p_tile_set_source); // Not exposed
	bool has_source(int p_source_id) const;
	Ref<TileSetSource> get_source(int p_source_id) const;

	// Rendering
	void set_uv_clipping(bool p_uv_clipping);
	bool is_uv_clipping() const;

	int get_occlusion_layers_count() const;
	void add_occlusion_layer(int p_index = -1);
	void move_occlusion_layer(int p_from_index, int p_to_pos);
	void remove_occlusion_layer(int p_index);
	void set_occlusion_layer_light_mask(int p_layer_index, int p_light_mask);
	int get_occlusion_layer_light_mask(int p_layer_index) const;
	void set_occlusion_layer_sdf_collision(int p_layer_index, bool p_sdf_collision);
	bool get_occlusion_layer_sdf_collision(int p_layer_index) const;

#ifndef PHYSICS_2D_DISABLED
	// Physics
	int get_physics_layers_count() const;
	void add_physics_layer(int p_index = -1);
	void move_physics_layer(int p_from_index, int p_to_pos);
	void remove_physics_layer(int p_index);
	void set_physics_layer_collision_layer(int p_layer_index, uint32_t p_layer);
	uint32_t get_physics_layer_collision_layer(int p_layer_index) const;
	void set_physics_layer_collision_mask(int p_layer_index, uint32_t p_mask);
	uint32_t get_physics_layer_collision_mask(int p_layer_index) const;
	void set_physics_layer_collision_priority(int p_layer_index, real_t p_priority);
	real_t get_physics_layer_collision_priority(int p_layer_index) const;
	void set_physics_layer_physics_material(
		int p_layer_index, Ref<PhysicsMaterial> p_physics_material);
	Ref<PhysicsMaterial> get_physics_layer_physics_material(int p_layer_index) const;
#endif // PHYSICS_2D_DISABLED

	// Terrain sets
	int get_terrain_sets_count() const;
	void add_terrain_set(int p_index = -1);
	void move_terrain_set(int p_from_index, int p_to_pos);
	void remove_terrain_set(int p_index);
	void set_terrain_set_mode(int p_terrain_set, TerrainMode p_terrain_mode);
	TerrainMode get_terrain_set_mode(int p_terrain_set) const;

	// Terrains
	int get_terrains_count(int p_terrain_set) const;
	void add_terrain(int p_terrain_set, int p_index = -1);
	void move_terrain(int p_terrain_set, int p_from_index, int p_to_pos);
	void remove_terrain(int p_terrain_set, int p_index);
	void clear_terrains(int p_terrain_set);
	void set_terrain_name(int p_terrain_set, int p_terrain_index, String p_name);
	String get_terrain_name(int p_terrain_set, int p_terrain_index) const;
	void set_terrain_color(int p_terrain_set, int p_terrain_index, Color p_color);
	Color get_terrain_color(int p_terrain_set, int p_terrain_index) const;
	bool is_valid_terrain_peering_bit_for_mode(
		TileSet::TerrainMode p_terrain_mode, TileSet::CellNeighbor p_peering_bit) const;
	bool is_valid_terrain_peering_bit(int p_terrain_set, TileSet::CellNeighbor p_peering_bit) const;

#ifndef NAVIGATION_2D_DISABLED
	// Navigation
	int get_navigation_layers_count() const;
	void add_navigation_layer(int p_index = -1);
	void move_navigation_layer(int p_from_index, int p_to_pos);
	void remove_navigation_layer(int p_index);
	void set_navigation_layer_layers(int p_layer_index, uint32_t p_layers);
	uint32_t get_navigation_layer_layers(int p_layer_index) const;
	void set_navigation_layer_layer_value(int p_layer_index, int p_layer_number, bool p_value);
	bool get_navigation_layer_layer_value(int p_layer_index, int p_layer_number) const;
#endif // NAVIGATION_2D_DISABLED

	// Custom data
	int get_custom_data_layers_count() const;
	void add_custom_data_layer(int p_index = -1);
	void move_custom_data_layer(int p_from_index, int p_to_pos);
	void remove_custom_data_layer(int p_index);
	int get_custom_data_layer_by_name(String p_value) const;
	void set_custom_data_layer_name(int p_layer_id, String p_value);
	bool has_custom_data_layer_by_name(const String& p_value) const;
	String get_custom_data_layer_name(int p_layer_id) const;

	// Tiles proxies.
	void set_source_level_tile_proxy(int p_source_from, int p_source_to);
	int get_source_level_tile_proxy(int p_source_from);
	bool has_source_level_tile_proxy(int p_source_from);
	void remove_source_level_tile_proxy(int p_source_from);

	void set_coords_level_tile_proxy(
		int p_source_from, Vector2i p_coords_from, int p_source_to, Vector2i p_coords_to);
	bool has_coords_level_tile_proxy(int p_source_from, Vector2i p_coords_from);
	void remove_coords_level_tile_proxy(int p_source_from, Vector2i p_coords_from);

	void set_alternative_level_tile_proxy(int p_source_from, Vector2i p_coords_from,
		int p_alternative_from, int p_source_to, Vector2i p_coords_to, int p_alternative_to);
	bool has_alternative_level_tile_proxy(
		int p_source_from, Vector2i p_coords_from, int p_alternative_from);
	void remove_alternative_level_tile_proxy(
		int p_source_from, Vector2i p_coords_from, int p_alternative_from);

	void cleanup_invalid_tile_proxies();
	void clear_tile_proxies();

	// Patterns.
	int add_pattern(Ref<TileMapPattern> p_pattern, int p_index = -1);
	Ref<TileMapPattern> get_pattern(int p_index);
	void remove_pattern(int p_index);
	int get_patterns_count();

	// Terrains.
	RBSet<TerrainsPattern> get_terrains_pattern_set(int p_terrain_set);
	RBSet<TileMapCell> get_tiles_for_terrains_pattern(
		int p_terrain_set, TerrainsPattern p_terrain_tile_pattern);
	TileMapCell get_random_tile_from_terrains_pattern(
		int p_terrain_set, TerrainsPattern p_terrain_tile_pattern);

	// Helpers
	Vector<Vector2> get_tile_shape_polygon() const;
	void draw_tile_shape(CanvasItem* p_canvas_item, Transform2D p_transform, Color p_color,
		bool p_filled = false, Ref<Texture2D> p_texture = Ref<Texture2D>()) const;

	// Used by TileMap/TileMapLayer
	Vector2 map_to_local(const Vector2i& p_pos) const;
	Vector2i local_to_map(const Vector2& p_pos) const;
	bool is_existing_neighbor(TileSet::CellNeighbor p_cell_neighbor) const;
	Vector2i get_neighbor_cell(
		const Vector2i& p_coords, TileSet::CellNeighbor p_cell_neighbor) const;
	TypedArray<Vector2i> get_surrounding_cells(const Vector2i& p_coords) const;
	Vector2i map_pattern(const Vector2i& p_position_in_tilemap, const Vector2i& p_coords_in_pattern,
		Ref<TileMapPattern> p_pattern) const;
	void draw_cells_outline(CanvasItem* p_canvas_item, const RBSet<Vector2i>& p_cells,
		Color p_color, Transform2D p_transform = Transform2D()) const;

	Vector<Point2> get_terrain_polygon(int p_terrain_set);
	Vector<Point2> get_terrain_peering_bit_polygon(int p_terrain_set, TileSet::CellNeighbor p_bit);
	void draw_terrains(
		CanvasItem* p_canvas_item, Transform2D p_transform, const TileData* p_tile_data);
	Vector<Vector<Ref<Texture2D>>> generate_terrains_icons(Size2i p_size);

	// Resource management
	virtual void reset_state() override;

	// Helpers.
	static Vector2i transform_coords_layout(const Vector2i& p_coords,
		TileSet::TileOffsetAxis p_offset_axis, TileSet::TileLayout p_from_layout,
		TileSet::TileLayout p_to_layout);

	TileSet();
	~TileSet();
};

class TileSetSource : public Resource
{
protected:
	const TileSet* tile_set = nullptr;

public:
	static const Vector2i INVALID_ATLAS_COORDS; // Vector2i(-1, -1);
	static const int INVALID_TILE_ALTERNATIVE;	// -1;

	// Not exposed.
	virtual void set_tile_set(const TileSet* p_tile_set);
	TileSet* get_tile_set() const;

	virtual void notify_tile_data_properties_should_change() {}

	virtual void add_occlusion_layer(int p_index) {}

	virtual void move_occlusion_layer(int p_from_index, int p_to_pos) {}

	virtual void remove_occlusion_layer(int p_index) {}

	virtual void add_physics_layer(int p_index) {}

	virtual void move_physics_layer(int p_from_index, int p_to_pos) {}

	virtual void remove_physics_layer(int p_index) {}

	virtual void add_terrain_set(int p_index) {}

	virtual void move_terrain_set(int p_from_index, int p_to_pos) {}

	virtual void remove_terrain_set(int p_index) {}

	virtual void add_terrain(int p_terrain_set, int p_index) {}

	virtual void move_terrain(int p_terrain_set, int p_from_index, int p_to_pos) {}

	virtual void remove_terrain(int p_terrain_set, int p_index) {}

	virtual void add_navigation_layer(int p_index) {}

	virtual void move_navigation_layer(int p_from_index, int p_to_pos) {}

	virtual void remove_navigation_layer(int p_index) {}

	virtual void add_custom_data_layer(int p_index) {}

	virtual void move_custom_data_layer(int p_from_index, int p_to_pos) {}

	virtual void remove_custom_data_layer(int p_index) {}

	virtual void reset_state() override;

	// Tiles.
	virtual int get_tiles_count() const = 0;
	virtual Vector2i get_tile_id(int tile_index) const = 0;
	virtual bool has_tile(Vector2i p_atlas_coords) const = 0;

	// Alternative tiles.
	virtual int get_alternative_tiles_count(const Vector2i p_atlas_coords) const = 0;
	virtual int get_alternative_tile_id(const Vector2i p_atlas_coords, int p_index) const = 0;
	virtual bool has_alternative_tile(
		const Vector2i p_atlas_coords, int p_alternative_tile) const = 0;
};

class TileSetAtlasSource : public TileSetSource
{
public:
	enum TileAnimationMode
	{
		TILE_ANIMATION_MODE_DEFAULT,
		TILE_ANIMATION_MODE_RANDOM_START_TIMES,
		TILE_ANIMATION_MODE_MAX,
	};

	enum TransformBits
	{
		TRANSFORM_FLIP_H = 1 << 12,
		TRANSFORM_FLIP_V = 1 << 13,
		TRANSFORM_TRANSPOSE = 1 << 14,
	};

	static const int16_t UNTRANSFORM_MASK =
		~(TileSetAtlasSource::TRANSFORM_FLIP_H + TileSetAtlasSource::TRANSFORM_FLIP_V +
			TileSetAtlasSource::TRANSFORM_TRANSPOSE);

private:
	struct TileAlternativesData
	{
		Vector2i size_in_atlas = Vector2i(1, 1);
		Vector2i texture_offset;

		// Animation
		int animation_columns = 0;
		Vector2i animation_separation;
		real_t animation_speed = 1.0;
		TileSetAtlasSource::TileAnimationMode animation_mode = TILE_ANIMATION_MODE_DEFAULT;
		LocalVector<real_t> animation_frames_durations;

		// Alternatives
		HashMap<int, TileData*> alternatives;
		Vector<int> alternatives_ids;
		int next_alternative_id = 1;
	};

	bool initializing = true;

	Ref<Texture2D> texture;
	Vector2i margins;
	Vector2i separation;
	Size2i texture_region_size = Size2i(16, 16);

	HashMap<Vector2i, TileAlternativesData> tiles;
	Vector<Vector2i> tiles_ids;
	HashMap<Vector2i, Vector2i> _coords_mapping_cache; // Maps any coordinate to the including tile

	TileData* _get_atlas_tile_data(Vector2i p_atlas_coords, int p_alternative_tile);
	const TileData* _get_atlas_tile_data(Vector2i p_atlas_coords, int p_alternative_tile) const;

	void _compute_next_alternative_id(const Vector2i p_atlas_coords);

	void _clear_coords_mapping_cache(Vector2i p_atlas_coords);
	void _create_coords_mapping_cache(Vector2i p_atlas_coords);

	bool use_texture_padding = true;
	Ref<CanvasTexture> padded_texture;
	bool padded_texture_needs_update = false;
	void _queue_update_padded_texture();
	Ref<ImageTexture> _create_padded_image_texture(const Ref<Texture2D>& p_source);
	void _update_padded_texture();
	void _try_emit_changed();

protected:
	void _notification(int p_notification);

public:
	// Not exposed.
	virtual void set_tile_set(const TileSet* p_tile_set) override;
	const TileSet* get_tile_set() const;
	virtual void notify_tile_data_properties_should_change() override;
	virtual void add_occlusion_layer(int p_index) override;
	virtual void move_occlusion_layer(int p_from_index, int p_to_pos) override;
	virtual void remove_occlusion_layer(int p_index) override;
#ifndef PHYSICS_2D_DISABLED
	virtual void add_physics_layer(int p_index) override;
	virtual void move_physics_layer(int p_from_index, int p_to_pos) override;
	virtual void remove_physics_layer(int p_index) override;
#endif // PHYSICS_2D_DISABLED
	virtual void add_terrain_set(int p_index) override;
	virtual void move_terrain_set(int p_from_index, int p_to_pos) override;
	virtual void remove_terrain_set(int p_index) override;
	virtual void add_terrain(int p_terrain_set, int p_index) override;
	virtual void move_terrain(int p_terrain_set, int p_from_index, int p_to_pos) override;
	virtual void remove_terrain(int p_terrain_set, int p_index) override;
#ifndef NAVIGATION_2D_DISABLED
	virtual void add_navigation_layer(int p_index) override;
	virtual void move_navigation_layer(int p_from_index, int p_to_pos) override;
	virtual void remove_navigation_layer(int p_index) override;
#endif // NAVIGATION_2D_DISABLED
	virtual void add_custom_data_layer(int p_index) override;
	virtual void move_custom_data_layer(int p_from_index, int p_to_pos) override;
	virtual void remove_custom_data_layer(int p_index) override;
	virtual void reset_state() override;

	// Base properties.
	void set_texture(Ref<Texture2D> p_texture);
	Ref<Texture2D> get_texture() const;
	void set_margins(Vector2i p_margins);
	Vector2i get_margins() const;
	void set_separation(Vector2i p_separation);
	Vector2i get_separation() const;
	void set_texture_region_size(Vector2i p_tile_size);
	Vector2i get_texture_region_size() const;

	// Padding.
	void set_use_texture_padding(bool p_use_padding);
	bool get_use_texture_padding() const;

	// Base tiles.
	void create_tile(const Vector2i p_atlas_coords, const Vector2i p_size = Vector2i(1, 1));
	void remove_tile(Vector2i p_atlas_coords);
	virtual bool has_tile(Vector2i p_atlas_coords) const override;
	void move_tile_in_atlas(Vector2i p_atlas_coords,
		Vector2i p_new_atlas_coords = INVALID_ATLAS_COORDS, Vector2i p_new_size = Vector2i(-1, -1));
	Vector2i get_tile_size_in_atlas(Vector2i p_atlas_coords) const;

	virtual int get_tiles_count() const override;
	virtual Vector2i get_tile_id(int p_index) const override;

	bool has_room_for_tile(Vector2i p_atlas_coords, Vector2i p_size, int p_animation_columns,
		Vector2i p_animation_separation, int p_frames_count,
		Vector2i p_ignored_tile = INVALID_ATLAS_COORDS) const;
	PackedVector2Array get_tiles_to_be_removed_on_change(Ref<Texture2D> p_texture,
		Vector2i p_margins, Vector2i p_separation, Vector2i p_texture_region_size);
	Vector2i get_tile_at_coords(Vector2i p_atlas_coords) const;

	bool has_tiles_outside_texture() const;
	Vector<Vector2i> get_tiles_outside_texture() const;
	void clear_tiles_outside_texture();

	// Animation.
	void set_tile_animation_columns(const Vector2i p_atlas_coords, int p_frame_columns);
	int get_tile_animation_columns(const Vector2i p_atlas_coords) const;
	void set_tile_animation_separation(const Vector2i p_atlas_coords, const Vector2i p_separation);
	Vector2i get_tile_animation_separation(const Vector2i p_atlas_coords) const;
	void set_tile_animation_speed(const Vector2i p_atlas_coords, real_t p_speed);
	real_t get_tile_animation_speed(const Vector2i p_atlas_coords) const;
	void set_tile_animation_mode(
		const Vector2i p_atlas_coords, const TileSetAtlasSource::TileAnimationMode p_mode);
	TileSetAtlasSource::TileAnimationMode get_tile_animation_mode(
		const Vector2i p_atlas_coords) const;
	void set_tile_animation_frames_count(const Vector2i p_atlas_coords, int p_frames_count);
	int get_tile_animation_frames_count(const Vector2i p_atlas_coords) const;
	void set_tile_animation_frame_duration(
		const Vector2i p_atlas_coords, int p_frame_index, real_t p_duration);
	real_t get_tile_animation_frame_duration(
		const Vector2i p_atlas_coords, int p_frame_index) const;
	real_t get_tile_animation_total_duration(const Vector2i p_atlas_coords) const;

	// Alternative tiles.
	int create_alternative_tile(const Vector2i p_atlas_coords, int p_alternative_id_override = -1);
	void remove_alternative_tile(const Vector2i p_atlas_coords, int p_alternative_tile);
	void set_alternative_tile_id(
		const Vector2i p_atlas_coords, int p_alternative_tile, int p_new_id);
	virtual bool has_alternative_tile(
		const Vector2i p_atlas_coords, int p_alternative_tile) const override;
	int get_next_alternative_tile_id(const Vector2i p_atlas_coords) const;

	virtual int get_alternative_tiles_count(const Vector2i p_atlas_coords) const override;
	virtual int get_alternative_tile_id(const Vector2i p_atlas_coords, int p_index) const override;

	// Get data associated to a tile.
	TileData* get_tile_data(const Vector2i p_atlas_coords, int p_alternative_tile) const;

	// Helpers.
	Vector2i get_atlas_grid_size() const;
	Rect2i get_tile_texture_region(Vector2i p_atlas_coords, int p_frame = 0) const;
	bool is_position_in_tile_texture_region(
		const Vector2i p_atlas_coords, int p_alternative_tile, Vector2 p_position) const;
	bool is_rect_in_tile_texture_region(
		const Vector2i p_atlas_coords, int p_alternative_tile, Rect2 p_rect) const;

	static int alternative_no_transform(int p_alternative_id);

	// Getters for texture and tile region (padded or not)
	Ref<Texture2D> get_runtime_texture() const;
	Rect2i get_runtime_tile_texture_region(Vector2i p_atlas_coords, int p_frame = 0) const;

	~TileSetAtlasSource();
};

class TileSetScenesCollectionSource : public TileSetSource
{
private:
	struct SceneData
	{
		Ref<PackedScene> scene;
		bool display_placeholder = false;
	};

	Vector<int> scenes_ids;
	HashMap<int, SceneData> scenes;
	int next_scene_id = 1;

	bool initializing = true;

	void _compute_next_alternative_id();
	void _try_emit_changed();

protected:
	void _notification(int p_notification);

public:
	// Tiles.
	int get_tiles_count() const override;
	Vector2i get_tile_id(int p_tile_index) const override;
	bool has_tile(Vector2i p_atlas_coords) const override;

	// Alternative tiles.
	int get_alternative_tiles_count(const Vector2i p_atlas_coords) const override;
	int get_alternative_tile_id(const Vector2i p_atlas_coords, int p_index) const override;
	bool has_alternative_tile(const Vector2i p_atlas_coords, int p_alternative_tile) const override;

	// Scenes accessors. Lot are similar to "Alternative tiles".
	int get_scene_tiles_count() { return get_alternative_tiles_count(Vector2i()); }

	int get_scene_tile_id(int p_index) { return get_alternative_tile_id(Vector2i(), p_index); }

	bool has_scene_tile_id(int p_id) { return has_alternative_tile(Vector2i(), p_id); }

	int create_scene_tile(
		Ref<PackedScene> p_packed_scene = Ref<PackedScene>(), int p_id_override = -1);
	void set_scene_tile_id(int p_id, int p_new_id);
	void set_scene_tile_scene(int p_id, Ref<PackedScene> p_packed_scene);
	Ref<PackedScene> get_scene_tile_scene(int p_id) const;
	void set_scene_tile_display_placeholder(int p_id, bool p_packed_scene);
	bool get_scene_tile_display_placeholder(int p_id) const;
	void remove_scene_tile(int p_id);
	int get_next_scene_tile_id() const;
};

class TileData
{
private:
	const TileSet* tile_set = nullptr;
	bool allow_transform = true;

	// Rendering
	bool flip_h = false;
	bool flip_v = false;
	bool transpose = false;
	Vector2i texture_origin;
	Ref<Material> material;
	Color modulate = Color(1.0, 1.0, 1.0, 1.0);
	int z_index = 0;
	int y_sort_origin = 0;

	struct OcclusionLayerTileData
	{
		struct PolygonOccluderTileData
		{
			Ref<OccluderPolygon2D> occluder_polygon;
			mutable HashMap<int, Ref<OccluderPolygon2D>> transformed_polygon_occluders;
		};

		Vector<PolygonOccluderTileData> polygons;
	};

	Vector<OcclusionLayerTileData> occluders;

#ifndef PHYSICS_2D_DISABLED
	// Physics
	struct PhysicsLayerTileData
	{
		struct PolygonShapeTileData
		{
			LocalVector<Vector2> polygon;
			LocalVector<Ref<Shape2D>> shapes;
			mutable HashMap<int, LocalVector<Ref<Shape2D>>> transformed_shapes;
			bool one_way = false;
			float one_way_margin = 1.0;
		};

		Vector2 linear_velocity;
		double angular_velocity = 0.0;
		Vector<PolygonShapeTileData> polygons;
	};

	Vector<PhysicsLayerTileData> physics;
// TODO add support for areas.
#endif // PHYSICS_2D_DISABLED

	// Terrain
	int terrain_set = -1;
	int terrain = -1;
	int terrain_peering_bits[16] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

#ifndef NAVIGATION_2D_DISABLED
	// Navigation
	struct NavigationLayerTileData
	{
		Ref<NavigationPolygon> navigation_polygon;
		mutable HashMap<int, Ref<NavigationPolygon>> transformed_navigation_polygon;
	};

	Vector<NavigationLayerTileData> navigation;
#endif // NAVIGATION_2D_DISABLED

	// Misc
	double probability = 1.0;

#ifndef DISABLE_DEPRECATED
#ifndef NAVIGATION_2D_DISABLED
	Ref<NavigationPolygon> _get_navigation_polygon_bind_compat_84660(int p_layer_id) const;
#endif // NAVIGATION_2D_DISABLED
	Ref<OccluderPolygon2D> _get_occluder_bind_compat_84660(int p_layer_id) const;

	static void _bind_compatibility_methods();
#endif

public:
	// Not exposed.
	void set_tile_set(const TileSet* p_tile_set);
	void notify_tile_data_properties_should_change();
	void add_occlusion_layer(int p_index);
	void move_occlusion_layer(int p_from_index, int p_to_pos);
	void remove_occlusion_layer(int p_index);
#ifndef PHYSICS_2D_DISABLED
	void add_physics_layer(int p_index);
	void move_physics_layer(int p_from_index, int p_to_pos);
	void remove_physics_layer(int p_index);
#endif // PHYSICS_2D_DISABLED
	void add_terrain_set(int p_index);
	void move_terrain_set(int p_from_index, int p_to_pos);
	void remove_terrain_set(int p_index);
	void add_terrain(int p_terrain_set, int p_index);
	void move_terrain(int p_terrain_set, int p_from_index, int p_to_pos);
	void remove_terrain(int p_terrain_set, int p_index);
	void add_navigation_layer(int p_index);
	void move_navigation_layer(int p_from_index, int p_to_pos);
	void remove_navigation_layer(int p_index);
	void add_custom_data_layer(int p_index);
	void move_custom_data_layer(int p_from_index, int p_to_pos);
	void remove_custom_data_layer(int p_index);
	void set_allow_transform(bool p_allow_transform);
	bool is_allowing_transform() const;

	// To duplicate a TileData object, needed for runtiume update.
	TileData* duplicate();

	// Rendering
	void set_flip_h(bool p_flip_h);
	bool get_flip_h() const;
	void set_flip_v(bool p_flip_v);
	bool get_flip_v() const;
	void set_transpose(bool p_transpose);
	bool get_transpose() const;

	void set_texture_origin(Vector2i p_texture_origin);
	Vector2i get_texture_origin() const;
	void set_material(Ref<Material> p_material);
	Ref<Material> get_material() const;
	void set_modulate(Color p_modulate);
	Color get_modulate() const;
	void set_z_index(int p_z_index);
	int get_z_index() const;
	void set_y_sort_origin(int p_y_sort_origin);
	int get_y_sort_origin() const;

#ifndef DISABLE_DEPRECATED
	void set_occluder(int p_layer_id, Ref<OccluderPolygon2D> p_occluder_polygon);
	Ref<OccluderPolygon2D> get_occluder(int p_layer_id, bool p_flip_h = false,
		bool p_flip_v = false, bool p_transpose = false) const;
#endif // DISABLE_DEPRECATED

	void set_occluder_polygons_count(int p_layer_id, int p_polygons_count);
	int get_occluder_polygons_count(int p_layer_id) const;
	void add_occluder_polygon(int p_layer_id);
	void remove_occluder_polygon(int p_layer_id, int p_polygon_index);
	void set_occluder_polygon(
		int p_layer_id, int p_polygon_index, const Ref<OccluderPolygon2D>& p_occluder_polygon);
	Ref<OccluderPolygon2D> get_occluder_polygon(int p_layer_id, int p_polygon_index,
		bool p_flip_h = false, bool p_flip_v = false, bool p_transpose = false) const;

#ifndef PHYSICS_2D_DISABLED
	// Physics
	void set_constant_linear_velocity(int p_layer_id, const Vector2& p_velocity);
	Vector2 get_constant_linear_velocity(int p_layer_id) const;
	void set_constant_angular_velocity(int p_layer_id, real_t p_velocity);
	real_t get_constant_angular_velocity(int p_layer_id) const;
	void set_collision_polygons_count(int p_layer_id, int p_shapes_count);
	int get_collision_polygons_count(int p_layer_id) const;
	void add_collision_polygon(int p_layer_id);
	void remove_collision_polygon(int p_layer_id, int p_polygon_index);
	void set_collision_polygon_points(
		int p_layer_id, int p_polygon_index, Vector<Vector2> p_polygon);
	Vector<Vector2> get_collision_polygon_points(int p_layer_id, int p_polygon_index) const;
	void set_collision_polygon_one_way(int p_layer_id, int p_polygon_index, bool p_one_way);
	bool is_collision_polygon_one_way(int p_layer_id, int p_polygon_index) const;
	void set_collision_polygon_one_way_margin(
		int p_layer_id, int p_polygon_index, float p_one_way_margin);
	float get_collision_polygon_one_way_margin(int p_layer_id, int p_polygon_index) const;
	int get_collision_polygon_shapes_count(int p_layer_id, int p_polygon_index) const;
	Ref<Shape2D> get_collision_polygon_shape(int p_layer_id, int p_polygon_index,
		int shape_index, bool p_flip_h = false, bool p_flip_v = false,
		bool p_transpose = false) const;
#endif // PHYSICS_2D_DISABLED

	// Terrain
	void set_terrain_set(int p_terrain_id);
	int get_terrain_set() const;
	void set_terrain(int p_terrain_id);
	int get_terrain() const;
	void set_terrain_peering_bit(TileSet::CellNeighbor p_peering_bit, int p_terrain_id);
	int get_terrain_peering_bit(TileSet::CellNeighbor p_peering_bit) const;
	bool is_valid_terrain_peering_bit(TileSet::CellNeighbor p_peering_bit) const;

	TileSet::TerrainsPattern get_terrains_pattern() const; // Not exposed.

#ifndef NAVIGATION_2D_DISABLED
	// Navigation
	void set_navigation_polygon(int p_layer_id, Ref<NavigationPolygon> p_navigation_polygon);
	Ref<NavigationPolygon> get_navigation_polygon(int p_layer_id, bool p_flip_h = false,
		bool p_flip_v = false, bool p_transpose = false) const;
#endif // NAVIGATION_2D_DISABLED

	// Misc
	void set_probability(float p_probability);
	float get_probability() const;

	// Custom data.
	bool has_custom_data(const String& p_layer_name) const;

	// Polygons.
	static PackedVector2Array get_transformed_vertices(const PackedVector2Array& p_vertices,
		bool p_flip_h, bool p_flip_v, bool p_transpose, bool p_preserve_winding_order = false);
};

// --- Folded from scene/2d/tile_map_layer.h ---

#include "servers/rendering/rendering_server_enums.h"

#ifndef NAVIGATION_2D_DISABLED
class NavigationMeshSourceGeometryData2D;
#endif // NAVIGATION_2D_DISABLED
class TileSetAtlasSource;
class TileMap;

enum TileMapLayerDataFormat
{
	TILE_MAP_LAYER_DATA_FORMAT_0 = 0,
	TILE_MAP_LAYER_DATA_FORMAT_MAX,
};

class TerrainConstraint
{
private:
	Ref<TileSet> tile_set;
	Vector2i base_cell_coords;
	int bit = -1;
	int terrain = -1;

	int priority = 1;

public:
	bool operator<(const TerrainConstraint& p_other) const
	{
		if (base_cell_coords == p_other.base_cell_coords) {
			return bit < p_other.bit;
		}
		return base_cell_coords < p_other.base_cell_coords;
	}

	String to_string() const
	{
		return vformat("Constraint {pos:%s, bit:%d, terrain:%d, priority:%d}", base_cell_coords,
			bit, terrain, priority);
	}

	Vector2i get_base_cell_coords() const { return base_cell_coords; }

	bool is_center_bit() const { return bit == 0; }

	HashMap<Vector2i, TileSet::CellNeighbor> get_overlapping_coords_and_peering_bits() const;

	void set_terrain(int p_terrain) { terrain = p_terrain; }

	int get_terrain() const { return terrain; }

	void set_priority(int p_priority) { priority = p_priority; }

	int get_priority() const { return priority; }

	TerrainConstraint(Ref<TileSet> p_tile_set, const Vector2i& p_position,
		int p_terrain); // For the center terrain bit
	TerrainConstraint(Ref<TileSet> p_tile_set, const Vector2i& p_position,
		const TileSet::CellNeighbor& p_bit, int p_terrain); // For peering bits

	TerrainConstraint() {}
};

#ifdef DEBUG_ENABLED
class DebugQuadrant;
#endif // DEBUG_ENABLED
class RenderingQuadrant;
class PhysicsQuadrant;

struct CellData
{
	Vector2i coords;
	TileMapCell cell;

	// Debug
	SelfList<CellData> debug_quadrant_list_element;

	// Rendering.
	Ref<RenderingQuadrant> rendering_quadrant;
	SelfList<CellData> rendering_quadrant_list_element;
	LocalVector<LocalVector<RID>> occluders;

#ifndef PHYSICS_2D_DISABLED
	// Physics.
	Ref<PhysicsQuadrant> physics_quadrant;
	SelfList<CellData> physics_quadrant_list_element;
#endif // PHYSICS_2D_DISABLED

	// Navigation.
	LocalVector<RID> navigation_regions;

	// Scenes.
	String scene;

	// Runtime TileData cache.
	TileData* runtime_tile_data_cache = nullptr;

	// List elements.
	SelfList<CellData> dirty_list_element;

	bool operator<(const CellData& p_other) const { return coords < p_other.coords; }

	// For those, copy everything but SelfList elements.
	void operator=(const CellData& p_other)
	{
		coords = p_other.coords;
		cell = p_other.cell;
		occluders = p_other.occluders;
		navigation_regions = p_other.navigation_regions;
		scene = p_other.scene;
		runtime_tile_data_cache = p_other.runtime_tile_data_cache;
	}

	CellData(const CellData& p_other)
		: debug_quadrant_list_element(this), rendering_quadrant_list_element(this),
#ifndef PHYSICS_2D_DISABLED
		  physics_quadrant_list_element(this),
#endif // PHYSICS_2D_DISABLED
		  dirty_list_element(this)
	{
		coords = p_other.coords;
		cell = p_other.cell;
		occluders = p_other.occluders;
		navigation_regions = p_other.navigation_regions;
		scene = p_other.scene;
		runtime_tile_data_cache = p_other.runtime_tile_data_cache;
	}

	CellData()
		: debug_quadrant_list_element(this), rendering_quadrant_list_element(this),
#ifndef PHYSICS_2D_DISABLED
		  physics_quadrant_list_element(this),
#endif // PHYSICS_2D_DISABLED
		  dirty_list_element(this)
	{
	}
};

// We use another comparator for Y-sorted layers with reversed X drawing order.
struct CellDataYSortedXReversedComparator
{
	_FORCE_INLINE_ bool operator()(const CellData& p_a, const CellData& p_b) const
	{
		return p_a.coords.x == p_b.coords.x ? (p_a.coords.y < p_b.coords.y)
											: (p_a.coords.x > p_b.coords.x);
	}
};

#ifdef DEBUG_ENABLED
class DebugQuadrant : public RefCounted
{
public:
	Vector2i quadrant_coords;
	SelfList<CellData>::List cells;
	RID canvas_item;

	RID physics_mesh;

	// Used to deleted unused quadrants.
	bool drawn_to = false;

	SelfList<DebugQuadrant> dirty_quadrant_list_element;

	DebugQuadrant() : dirty_quadrant_list_element(this) {}

	~DebugQuadrant() { cells.clear(); }
};
#endif // DEBUG_ENABLED

class RenderingQuadrant : public RefCounted
{
public:
	struct CoordsWorldComparator
	{
		_ALWAYS_INLINE_ bool operator()(const Vector2& p_a, const Vector2& p_b) const
		{
			// We sort the cells by their local coords, as it is needed by rendering.
			if (p_a.y == p_b.y) {
				return p_a.x > p_b.x;
			}
			else {
				return p_a.y < p_b.y;
			}
		}
	};

	Vector2i quadrant_coords;
	SelfList<CellData>::List cells;
	List<RID> canvas_items;
	Vector2 canvas_items_position;

	SelfList<RenderingQuadrant> dirty_quadrant_list_element;

	RenderingQuadrant() : dirty_quadrant_list_element(this) {}

	~RenderingQuadrant() { cells.clear(); }
};

#ifndef PHYSICS_2D_DISABLED
class PhysicsQuadrant : public RefCounted
{
public:
	struct PhysicsBodyKey
	{
		int physics_layer = 0;
		Vector2 linear_velocity;
		real_t angular_velocity = 0.0;

		bool one_way_collision = false;
		real_t one_way_collision_margin = 0.0;

		int64_t y_origin = 0; // This is only used if one_way_collision is on, to avoid merging
							  // polygons vertically in that case.

		bool operator<(const PhysicsBodyKey& p_other) const
		{
			if (physics_layer == p_other.physics_layer) {
				if (linear_velocity == p_other.linear_velocity) {
					if (angular_velocity == p_other.angular_velocity) {
						if (one_way_collision == p_other.one_way_collision) {
							if (one_way_collision && y_origin != p_other.y_origin) {
								return y_origin < p_other.y_origin;
							}
							return one_way_collision_margin < p_other.one_way_collision_margin;
						}
						return one_way_collision < p_other.one_way_collision;
					}
					return angular_velocity < p_other.angular_velocity;
				}
				return linear_velocity < p_other.linear_velocity;
			}
			return physics_layer < p_other.physics_layer;
		}

		bool operator!=(const PhysicsBodyKey& p_other) const { return !this->operator==(p_other); }

		bool operator==(const PhysicsBodyKey& p_other) const
		{
			return physics_layer == p_other.physics_layer &&
				   linear_velocity == p_other.linear_velocity &&
				   angular_velocity == p_other.angular_velocity &&
				   one_way_collision == p_other.one_way_collision &&
				   one_way_collision_margin == p_other.one_way_collision_margin &&
				   (!one_way_collision || y_origin == p_other.y_origin);
		}
	};

	struct PhysicsBodyKeyHasher
	{
		static uint32_t hash(const PhysicsBodyKey& p_hash)
		{
			uint32_t h = hash_murmur3_one_32(p_hash.physics_layer);
			h = hash_murmur3_one_real(p_hash.linear_velocity.x);
			h = hash_murmur3_one_real(p_hash.linear_velocity.y, h);
			h = hash_murmur3_one_real(p_hash.angular_velocity, h);
			if (p_hash.one_way_collision) {
				h = hash_murmur3_one_32(p_hash.y_origin, h);
			}
			return h;
		}
	};

	struct PhysicsBodyValue
	{
		RID body;
		Vector<Vector<Vector2>> polygons;
	};

	struct CoordsWorldComparator
	{
		_ALWAYS_INLINE_ bool operator()(const Vector2& p_a, const Vector2& p_b) const
		{
			// We sort the cells by their local coords, as it is needed by rendering.
			if (p_a.y == p_b.y) {
				return p_a.x > p_b.x;
			}
			else {
				return p_a.y < p_b.y;
			}
		}
	};

	Vector2i quadrant_coords;
	SelfList<CellData>::List cells;

	HashMap<PhysicsBodyKey, PhysicsBodyValue, PhysicsBodyKeyHasher> bodies;
	LocalVector<Ref<Shape2D>> shapes;

	SelfList<PhysicsQuadrant> dirty_quadrant_list_element;

	PhysicsQuadrant() : dirty_quadrant_list_element(this) {}

	~PhysicsQuadrant() { cells.clear(); }
};
#endif // PHYSICS_2D_DISABLED

class TileMapLayer : public Node2D
{
public:
	enum HighlightMode
	{
		HIGHLIGHT_MODE_DEFAULT,
		HIGHLIGHT_MODE_ABOVE,
		HIGHLIGHT_MODE_BELOW,
	};

	enum DebugVisibilityMode
	{
		DEBUG_VISIBILITY_MODE_DEFAULT,
		DEBUG_VISIBILITY_MODE_FORCE_SHOW,
		DEBUG_VISIBILITY_MODE_FORCE_HIDE,
	};

	enum DirtyFlags
	{
		DIRTY_FLAGS_LAYER_ENABLED = 0,

		DIRTY_FLAGS_LAYER_IN_TREE,
		DIRTY_FLAGS_LAYER_IN_CANVAS,
		DIRTY_FLAGS_LAYER_LOCAL_TRANSFORM,
		DIRTY_FLAGS_LAYER_VISIBILITY,
		DIRTY_FLAGS_LAYER_SELF_MODULATE,
		DIRTY_FLAGS_LAYER_Y_SORT_ENABLED,
		DIRTY_FLAGS_LAYER_Y_SORT_ORIGIN,
		DIRTY_FLAGS_LAYER_X_DRAW_ORDER_REVERSED,
		DIRTY_FLAGS_LAYER_Z_INDEX,
		DIRTY_FLAGS_LAYER_LIGHT_MASK,
		DIRTY_FLAGS_LAYER_TEXTURE_FILTER,
		DIRTY_FLAGS_LAYER_TEXTURE_REPEAT,
		DIRTY_FLAGS_LAYER_RENDERING_QUADRANT_SIZE,
		DIRTY_FLAGS_LAYER_COLLISION_ENABLED,
		DIRTY_FLAGS_LAYER_USE_KINEMATIC_BODIES,
		DIRTY_FLAGS_LAYER_PHYSICS_QUADRANT_SIZE,
		DIRTY_FLAGS_LAYER_COLLISION_VISIBILITY_MODE,
		DIRTY_FLAGS_LAYER_OCCLUSION_ENABLED,
		DIRTY_FLAGS_LAYER_NAVIGATION_ENABLED,
		DIRTY_FLAGS_LAYER_NAVIGATION_MAP,
		DIRTY_FLAGS_LAYER_NAVIGATION_VISIBILITY_MODE,
		DIRTY_FLAGS_LAYER_RUNTIME_UPDATE,
		DIRTY_FLAGS_LAYER_HIGHLIGHT_MODE,

		DIRTY_FLAGS_LAYER_INDEX_IN_TILE_MAP_NODE, // For compatibility.

		DIRTY_FLAGS_LAYER_GROUP_SELECTED_LAYERS,
		DIRTY_FLAGS_LAYER_GROUP_HIGHLIGHT_SELECTED,

		DIRTY_FLAGS_TILE_SET,

		DIRTY_FLAGS_MAX,
	};

private:
	static constexpr float FP_ADJUST = 0.00001;

	// Properties.
	HashMap<Vector2i, CellData> tile_map_layer_data;

	bool enabled = true;
	Ref<TileSet> tile_set;

	HighlightMode highlight_mode = HIGHLIGHT_MODE_DEFAULT;

	int y_sort_origin = 0;
	bool x_draw_order_reversed = false;
	int rendering_quadrant_size = 16;

	bool collision_enabled = true;
	bool use_kinematic_bodies = false;
	int physics_quadrant_size = 16;
	DebugVisibilityMode collision_visibility_mode = DEBUG_VISIBILITY_MODE_DEFAULT;

	bool occlusion_enabled = true;

	bool navigation_enabled = true;
	RID navigation_map_override;
	DebugVisibilityMode navigation_visibility_mode = DEBUG_VISIBILITY_MODE_DEFAULT;

	// Internal.
	bool pending_update = false;

	// For keeping compatibility with TileMap.
	TileMap* tile_map_node = nullptr;
	int layer_index_in_tile_map_node = -1;

	// Dirty flag. Allows knowing what was modified since the last update.
	struct
	{
		bool flags[DIRTY_FLAGS_MAX] = {false};
		SelfList<CellData>::List cell_list;
	} dirty;

	// Rect cache.
	mutable Rect2 rect_cache;
	mutable bool rect_cache_dirty = true;
	mutable Rect2i used_rect_cache;
	mutable bool used_rect_cache_dirty = true;

	// Runtime tile data.
	bool _runtime_update_tile_data_was_cleaned_up = false;
	void _build_runtime_update_tile_data(bool p_force_cleanup);
	void _build_runtime_update_tile_data_for_cell(CellData& r_cell_data,
		bool p_use_tilemap_for_runtime, bool p_auto_add_to_dirty_list = false);
	bool _runtime_update_needs_all_cells_cleaned_up = false;
	void _clear_runtime_update_tile_data();
	void _clear_runtime_update_tile_data_for_cell(CellData& r_cell_data);
	void _update_cells_callback(bool p_force_cleanup);

	// Coords to quadrant coords
	Vector2i _coords_to_quadrant_coords(const Vector2i& p_coords, const int p_quadrant_size) const;

	// Per-system methods.
#ifdef DEBUG_ENABLED
	HashMap<Vector2i, Ref<DebugQuadrant>> debug_quadrant_map;
	bool _debug_was_cleaned_up = true;
	void _debug_update(bool p_force_cleanup);
	void _debug_quadrants_update_cell(CellData& r_cell_data);
	void _get_debug_quadrant_for_cell(const Vector2i& p_coords);
#endif // DEBUG_ENABLED

	HashMap<Vector2i, Ref<RenderingQuadrant>> rendering_quadrant_map;
	bool _rendering_was_cleaned_up = true;
	bool _occlusion_was_cleaned_up = true;
	void _rendering_update(bool p_force_cleanup);
	void _rendering_notification(int p_what);
	Color _highlight_color(const Color& p_modulate) const;
	void _rendering_quadrants_update_cell(
		CellData& r_cell_data, SelfList<RenderingQuadrant>::List& r_dirty_rendering_quadrant_list);
	void _rendering_occluders_clear_cell(CellData& r_cell_data);
	void _rendering_occluders_update_cell(CellData& r_cell_data);
#ifdef DEBUG_ENABLED
	void _rendering_draw_cell_debug(
		const RID& p_canvas_item, const Vector2& p_quadrant_pos, const CellData& r_cell_data);
#endif // DEBUG_ENABLED

#ifndef PHYSICS_2D_DISABLED
	HashMap<Vector2i, Ref<PhysicsQuadrant>> physics_quadrant_map;
	HashMap<RID, Vector2i> bodies_coords; // Mapping for RID to coords.
	bool _physics_was_cleaned_up = true;
	void _physics_update(bool p_force_cleanup);
	void _physics_notification(int p_what);
	void _physics_quadrants_update_cell(
		CellData& r_cell_data, SelfList<PhysicsQuadrant>::List& r_dirty_physics_quadrant_list);
	void _physics_clear_cell(CellData& r_cell_data);
	void _physics_update_cell(CellData& r_cell_data);
#ifdef DEBUG_ENABLED
	void _physics_draw_quadrant_debug(const RID& p_canvas_item, DebugQuadrant& r_debug_quadrant);
#endif // DEBUG_ENABLED
#endif // PHYSICS_2D_DISABLED

#ifndef NAVIGATION_2D_DISABLED
	bool _navigation_was_cleaned_up = true;
	void _navigation_update(bool p_force_cleanup);
	void _navigation_notification(int p_what);
	void _navigation_clear_cell(CellData& r_cell_data);
	void _navigation_update_cell(CellData& r_cell_data);
#ifdef DEBUG_ENABLED
	void _navigation_draw_cell_debug(
		const RID& p_canvas_item, const Vector2& p_quadrant_pos, const CellData& r_cell_data);
#endif // DEBUG_ENABLED
#endif // NAVIGATION_2D_DISABLED

	bool _scenes_was_cleaned_up = true;
	void _scenes_update(bool p_force_cleanup);
	void _scenes_clear_cell(CellData& r_cell_data);
	void _scenes_update_cell(CellData& r_cell_data);
#ifdef DEBUG_ENABLED
	void _scenes_draw_cell_debug(
		const RID& p_canvas_item, const Vector2& p_quadrant_pos, const CellData& r_cell_data);
#endif // DEBUG_ENABLED
	void _set_scene_transform_with_alternative(
		Node2D* p_scene, const Vector2& p_cell_position, const int p_alternative_id);

	// Terrains.
	TileSet::TerrainsPattern _get_best_terrain_pattern_for_constraints(int p_terrain_set,
		const Vector2i& p_position, const RBSet<TerrainConstraint>& p_constraints,
		TileSet::TerrainsPattern p_current_pattern) const;
	RBSet<TerrainConstraint> _get_terrain_constraints_from_added_pattern(const Vector2i& p_position,
		int p_terrain_set, TileSet::TerrainsPattern p_terrains_pattern) const;
	RBSet<TerrainConstraint> _get_terrain_constraints_from_painted_cells_list(
		const RBSet<Vector2i>& p_painted, int p_terrain_set, bool p_ignore_empty_terrains) const;

	void _tile_set_changed();

	void _renamed();
	void _update_notify_local_transform();

	// Internal updates.
	void _queue_internal_update();
	void _deferred_internal_update();
	void _internal_update(bool p_force_cleanup);

	virtual void _physics_interpolated_changed() override;

protected:
	void _notification(int p_what);


	virtual void _update_self_texture_filter(
		RSE::CanvasItemTextureFilter p_texture_filter) override;
	virtual void _update_self_texture_repeat(
		RSE::CanvasItemTextureRepeat p_texture_repeat) override;

public:
#ifdef TOOLS_ENABLED
	virtual bool _edit_is_selected_on_click(
		const Point2& p_point, double p_tolerance) const override;
#endif

	// TileMap node.
	void set_as_tile_map_internal_node(int p_index);

	int get_index_in_tile_map() const { return layer_index_in_tile_map_node; }

	const HashMap<Vector2i, CellData>& get_tile_map_layer_data() const
	{
		return tile_map_layer_data;
	}

	// Rect caching.
	Rect2 get_rect(bool& r_changed) const;

	// Terrains.
	HashMap<Vector2i, TileSet::TerrainsPattern> terrain_fill_constraints(
		const Vector<Vector2i>& p_to_replace, int p_terrain_set,
		const RBSet<TerrainConstraint>& p_constraints) const; // Not exposed.
	HashMap<Vector2i, TileSet::TerrainsPattern> terrain_fill_connect(
		const Vector<Vector2i>& p_coords_array, int p_terrain_set, int p_terrain,
		bool p_ignore_empty_terrains = true) const; // Not exposed.
	HashMap<Vector2i, TileSet::TerrainsPattern> terrain_fill_path(
		const Vector<Vector2i>& p_coords_array, int p_terrain_set, int p_terrain,
		bool p_ignore_empty_terrains = true) const; // Not exposed.
	HashMap<Vector2i, TileSet::TerrainsPattern> terrain_fill_pattern(
		const Vector<Vector2i>& p_coords_array, int p_terrain_set,
		TileSet::TerrainsPattern p_terrains_pattern,
		bool p_ignore_empty_terrains = true) const; // Not exposed.

	// Not exposed to users.
	TileMapCell get_cell(const Vector2i& p_coords) const;

	static void compute_transformed_tile_dest_rect(Rect2& r_dest_rect, bool& r_transpose,
		const Vector2& p_position, const Vector2& p_dest_rect_size, const TileData* p_tile_data,
		int p_alternative_tile);
	static void draw_tile(RID p_canvas_item, const Vector2& p_position,
		const Ref<TileSet> p_tile_set, int p_atlas_source_id, const Vector2i& p_atlas_coords,
		int p_alternative_tile, int p_frame = -1, const TileData* p_tile_data_override = nullptr,
		real_t p_normalized_animation_offset = 0.0);

	////////////// Exposed functions //////////////

	// --- Cells manipulation ---
	// Generic cells manipulations and data access.
	void set_cell(const Vector2i& p_coords, int p_source_id = TileSet::INVALID_SOURCE,
		const Vector2i& p_atlas_coords = TileSetSource::INVALID_ATLAS_COORDS,
		int p_alternative_tile = 0);
	void erase_cell(const Vector2i& p_coords);
	void fix_invalid_tiles();
	void clear();

	int get_cell_source_id(const Vector2i& p_coords) const;
	Vector2i get_cell_atlas_coords(const Vector2i& p_coords) const;
	int get_cell_alternative_tile(const Vector2i& p_coords) const;
	TileData* get_cell_tile_data(
		const Vector2i& p_coords) const; // Helper method to make accessing the data easier.

	TypedArray<Vector2i> get_used_cells() const;
	TypedArray<Vector2i> get_used_cells_by_id(int p_source_id = TileSet::INVALID_SOURCE,
		const Vector2i& p_atlas_coords = TileSetSource::INVALID_ATLAS_COORDS,
		int p_alternative_tile = TileSetSource::INVALID_TILE_ALTERNATIVE) const;
	Rect2i get_used_rect() const;

	bool is_cell_flipped_h(const Vector2i& p_coords) const;
	bool is_cell_flipped_v(const Vector2i& p_coords) const;
	bool is_cell_transposed(const Vector2i& p_coords) const;

	// Patterns.
	Ref<TileMapPattern> get_pattern(TypedArray<Vector2i> p_coords_array);
	void set_pattern(const Vector2i& p_position, const Ref<TileMapPattern> p_pattern);

	// Terrains.
	void set_cells_terrain_connect(TypedArray<Vector2i> p_cells, int p_terrain_set, int p_terrain,
		bool p_ignore_empty_terrains = true);
	void set_cells_terrain_path(TypedArray<Vector2i> p_path, int p_terrain_set, int p_terrain,
		bool p_ignore_empty_terrains = true);

#ifndef PHYSICS_2D_DISABLED
	// --- Physics helpers ---
	bool has_body_rid(RID p_physics_body) const;
	Vector2i get_coords_for_body_rid(RID p_physics_body) const; // For finding tiles from collision.
#endif															// PHYSICS_2D_DISABLED

	// --- Runtime ---
	void update_internals();
	void notify_runtime_tile_data_update();

	// --- Shortcuts to methods defined in TileSet ---
	Vector2i map_pattern(const Vector2i& p_position_in_tilemap, const Vector2i& p_coords_in_pattern,
		Ref<TileMapPattern> p_pattern);
	TypedArray<Vector2i> get_surrounding_cells(const Vector2i& p_coords);
	Vector2i get_neighbor_cell(
		const Vector2i& p_coords, TileSet::CellNeighbor p_cell_neighbor) const;
	Vector2 map_to_local(const Vector2i& p_pos) const;
	Vector2i local_to_map(const Vector2& p_pos) const;

	// --- Accessors ---
	void set_tile_map_data_from_array(const Vector<uint8_t>& p_data);
	Vector<uint8_t> get_tile_map_data_as_array() const;

	void set_enabled(bool p_enabled);
	bool is_enabled() const;
	void set_tile_set(const Ref<TileSet>& p_tile_set);
	Ref<TileSet> get_tile_set() const;

	void set_highlight_mode(HighlightMode p_highlight_mode);
	HighlightMode get_highlight_mode() const;

	virtual void set_self_modulate(const Color& p_self_modulate) override;
	virtual void set_y_sort_enabled(bool p_y_sort_enabled) override;
	void set_y_sort_origin(int p_y_sort_origin);
	int get_y_sort_origin() const;
	void set_x_draw_order_reversed(bool p_x_draw_order_reversed);
	bool is_x_draw_order_reversed() const;
	virtual void set_z_index(int p_z_index);
	virtual void set_light_mask(int p_light_mask) override;
	void set_rendering_quadrant_size(int p_size);
	int get_rendering_quadrant_size() const;

	void set_collision_enabled(bool p_enabled);
	bool is_collision_enabled() const;
	void set_use_kinematic_bodies(bool p_use_kinematic_bodies);
	bool is_using_kinematic_bodies() const;
	void set_collision_visibility_mode(DebugVisibilityMode p_show_collision);
	DebugVisibilityMode get_collision_visibility_mode() const;
	void set_physics_quadrant_size(int p_size);
	int get_physics_quadrant_size() const;

	void set_occlusion_enabled(bool p_enabled);
	bool is_occlusion_enabled() const;

	void set_navigation_enabled(bool p_enabled);
	bool is_navigation_enabled() const;
	void set_navigation_map(RID p_map);
	RID get_navigation_map() const;
	void set_navigation_visibility_mode(DebugVisibilityMode p_show_navigation);
	DebugVisibilityMode get_navigation_visibility_mode() const;

private:
#ifndef NAVIGATION_2D_DISABLED
	static RID _navmesh_source_geometry_parser;
#endif // NAVIGATION_2D_DISABLED

public:
#ifndef NAVIGATION_2D_DISABLED
	static void navmesh_parse_init();
	static void navmesh_parse_source_geometry(const Ref<NavigationPolygon>& p_navigation_mesh,
		Ref<NavigationMeshSourceGeometryData2D> p_source_geometry_data, Node* p_node);
#endif // NAVIGATION_2D_DISABLED

	TileMapLayer();
	~TileMapLayer();
};



// --- End scene/2d/tile_map_layer.h ---
