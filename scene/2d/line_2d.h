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
