/**************************************************************************/
/*  node_2d.h                                                             */
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

#include "scene/main/canvas_item.h"
#include "scene/resources/environment.h"

struct CollisionState2D;

class Node2D : public CanvasItem
{
	mutable MTFlag xform_dirty;
	mutable Point2 position;
	mutable real_t rotation = 0.0;
	mutable Size2 scale = Vector2(1, 1);
	mutable real_t skew = 0.0;

	Transform2D transform;

	_FORCE_INLINE_ bool _is_xform_dirty() const
	{
		return is_group_processing() ? xform_dirty.mt.is_set() : xform_dirty.st;
	}

	void _set_xform_dirty(bool p_dirty) const;

	void _update_transform();

	void _update_xform_values() const;

protected:
	void _notification(int p_notification);

public:
	void reparent(Node* p_parent, bool p_keep_global_transform);

	void set_position(const Point2& p_pos);
	void set_rotation(real_t p_radians);
	void set_rotation_degrees(real_t p_degrees);
	void set_skew(real_t p_radians);
	void set_scale(const Size2& p_scale);

	void rotate(real_t p_radians);
	void move_x(real_t p_delta, bool p_scaled = false);
	void move_y(real_t p_delta, bool p_scaled = false);
	void translate(const Vector2& p_amount);
	void global_translate(const Vector2& p_amount);
	void apply_scale(const Size2& p_amount);

	Point2 get_position() const;
	real_t get_rotation() const;
	real_t get_rotation_degrees() const;
	real_t get_skew() const;
	Size2 get_scale() const;

	Point2 get_global_position() const;
	real_t get_global_rotation() const;
	real_t get_global_rotation_degrees() const;
	real_t get_global_skew() const;
	Size2 get_global_scale() const;

	void set_transform(const Transform2D& p_transform);
	void set_global_transform(const Transform2D& p_transform);
	void set_global_position(const Point2& p_pos);
	void set_global_rotation(const real_t p_radians);
	void set_global_rotation_degrees(const real_t p_degrees);
	void set_global_skew(const real_t p_radians);
	void set_global_scale(const Size2& p_scale);

	void look_at(const Vector2& p_pos);
	real_t get_angle_to(const Vector2& p_pos) const;

	Point2 to_local(const Point2& p_global) const;
	Point2 to_global(const Point2& p_local) const;

	Transform2D get_relative_transform_to_parent(const Node* rp_parent) const;

	Transform2D get_transform() const override;

	virtual CollisionState2D* get_collision_state_2d() { return nullptr; }
	virtual const CollisionState2D* get_collision_state_2d() const { return nullptr; }

	Node2D() = default;
};

// --- Folded from scene/2d/camera_2d.h ---
class Camera2D : public Node2D
{
public:
	enum AnchorMode
	{
		ANCHOR_MODE_FIXED_TOP_LEFT,
		ANCHOR_MODE_DRAG_CENTER
	};

	enum Camera2DProcessCallback
	{
		CAMERA2D_PROCESS_PHYSICS,
		CAMERA2D_PROCESS_IDLE
	};

protected:
	Point2 camera_pos;
	Point2 smoothed_camera_pos;
	bool first = true;
	bool just_exited_tree = false;

	Viewport* custom_viewport = nullptr;
	Viewport* viewport = nullptr;

	String group_name;
	String canvas_group_name;
	RID canvas;
	Vector2 offset;
	Vector2 zoom = Vector2(1, 1);
	Vector2 zoom_scale = Vector2(1, 1);
	AnchorMode anchor_mode = ANCHOR_MODE_DRAG_CENTER;
	bool ignore_rotation = true;
	bool enabled = true;
	real_t position_smoothing_speed = 5.0;
	bool position_smoothing_enabled = false;

	real_t camera_angle = 0.0;
	real_t rotation_smoothing_speed = 5.0;
	bool rotation_smoothing_enabled = false;

	bool limit_enabled = true;
	int limit[4] = {-10000000, -10000000, 10000000, 10000000}; // Left, top, right, bottom.
	bool limit_smoothing_enabled = false;

	real_t drag_margin[4] = {0.2, 0.2, 0.2, 0.2};
	bool drag_horizontal_enabled = false;
	bool drag_vertical_enabled = false;
	real_t drag_horizontal_offset = 0.0;
	real_t drag_vertical_offset = 0.0;
	bool drag_horizontal_offset_changed = false;
	bool drag_vertical_offset_changed = false;

	Point2 camera_screen_center;
	bool _is_editing_in_editor() const;
	void _update_process_callback();
	void _update_scroll();

#ifdef TOOLS_ENABLED
	void _project_settings_changed();
#endif

	void _reset_just_exited() { just_exited_tree = false; }

	void _update_process_internal_for_smoothing();

	bool screen_drawing_enabled = true;
	bool limit_drawing_enabled = false;
	bool margin_drawing_enabled = false;

	Camera2DProcessCallback process_callback = CAMERA2D_PROCESS_IDLE;

	struct InterpolationData
	{
		Transform2D xform_curr;
		Transform2D xform_prev;
		uint32_t last_update_physics_tick = UINT32_MAX; // Ensure tick 0 is detected as a change.

		// Camera2D can only call get_camera_transform() without flagging warnings after setting up
		// viewports during NOTIFICATION_ENTER_TREE, so we reject resets outside this lifetime.
		bool accepting_resets = false;
	} _interpolation_data;

	void _ensure_update_interpolation_data();

	Size2 _get_camera_screen_size() const;

protected:
	virtual Transform2D get_camera_transform();

	void _notification(int p_what);

public:
	void set_limit_rect(const Rect2i& p_limit_rect);
	Rect2i get_limit_rect() const;

	void set_offset(const Vector2& p_offset);
	Vector2 get_offset() const;

	void set_anchor_mode(AnchorMode p_anchor_mode);
	AnchorMode get_anchor_mode() const;

	void set_ignore_rotation(bool p_ignore);
	bool is_ignoring_rotation() const;

	void set_limit_enabled(bool p_limit_enabled);
	bool is_limit_enabled() const;

	void set_limit(Side p_side, int p_limit);
	int get_limit(Side p_side) const;

	void set_limit_smoothing_enabled(bool p_enabled);
	bool is_limit_smoothing_enabled() const;

	void set_drag_horizontal_enabled(bool p_enabled);
	bool is_drag_horizontal_enabled() const;

	void set_drag_vertical_enabled(bool p_enabled);
	bool is_drag_vertical_enabled() const;

	void set_drag_margin(Side p_side, real_t p_drag_margin);
	real_t get_drag_margin(Side p_side) const;

	void set_drag_horizontal_offset(real_t p_offset);
	real_t get_drag_horizontal_offset() const;

	void set_drag_vertical_offset(real_t p_offset);
	real_t get_drag_vertical_offset() const;

	void set_position_smoothing_enabled(bool p_enabled);
	bool is_position_smoothing_enabled() const;

	void set_position_smoothing_speed(real_t p_speed);
	real_t get_position_smoothing_speed() const;

	void set_rotation_smoothing_speed(real_t p_speed);
	real_t get_rotation_smoothing_speed() const;

	void set_rotation_smoothing_enabled(bool p_enabled);
	bool is_rotation_smoothing_enabled() const;

	void set_process_callback(Camera2DProcessCallback p_mode);
	Camera2DProcessCallback get_process_callback() const;

	void set_enabled(bool p_enabled);
	bool is_enabled() const;

	void make_current();
	void clear_current();
	bool is_current() const;

	void set_zoom(const Vector2& p_zoom);
	Vector2 get_zoom() const;

	Point2 get_camera_screen_center() const;
	real_t get_screen_rotation() const;

	void set_custom_viewport(Node* p_viewport);
	Node* get_custom_viewport() const;

	Vector2 get_camera_position() const;
	void force_update_scroll();
	void reset_smoothing();
	void align();

	void set_screen_drawing_enabled(bool p_enabled);
	bool is_screen_drawing_enabled() const;

	void set_limit_drawing_enabled(bool p_enabled);
	bool is_limit_drawing_enabled() const;

	void set_margin_drawing_enabled(bool p_enabled);
	bool is_margin_drawing_enabled() const;

	Camera2D();
};

// --- Folded from scene/2d/audio_listener_2d.h ---

class AudioListener2D : public Node2D
{
private:
	bool current = false;

	friend class Viewport;

protected:
	void _notification(int p_what);

public:
	void make_current();
	void clear_current();
	bool is_current() const;

	AudioListener2D();
};



// --- End scene/2d/audio_listener_2d.h ---

// --- Folded from scene/2d/audio_stream_player_2d.h ---
#include "servers/audio/audio_server.h"

struct AudioFrame;
class AudioStream;
class AudioStreamPlayback;
class AudioStreamPlayerInternal;

class AudioStreamPlayer2D : public Node2D
{
private:
	enum
	{
		MAX_OUTPUTS = 8,
		MAX_INTERSECT_AREAS = 32

	};

	AudioStreamPlayerInternal* internal = nullptr;

	SafeNumeric<float> setplay{-1.0};
	Ref<AudioStreamPlayback> setplayback;

	Vector<AudioFrame> volume_vector;

	uint64_t last_mix_count = -1;
	bool force_update_panning = false;

	AudioServer::PlaybackType playback_type = AudioServer::PlaybackType::PLAYBACK_TYPE_DEFAULT;

	void _set_playing(bool p_enable);
	bool _is_active() const;

	String _get_actual_bus();
	void _update_panning();

	static void _listener_changed_cb(void* self)
	{
		reinterpret_cast<AudioStreamPlayer2D*>(self)->force_update_panning = true;
	}

	uint32_t area_mask = 0;

	float max_distance = 2000.0;
	float attenuation = 1.0;

	float panning_strength = 1.0f;
	float cached_global_panning_strength = 0.5f;

protected:
	void _notification(int p_what);

#ifndef DISABLE_DEPRECATED
	bool _is_autoplay_enabled_bind_compat_86907();
	static void _bind_compatibility_methods();
#endif // DISABLE_DEPRECATED

public:
	void set_stream(Ref<AudioStream> p_stream);
	Ref<AudioStream> get_stream() const;

	void set_volume_db(float p_volume);
	float get_volume_db() const;

	void set_volume_linear(float p_volume);
	float get_volume_linear() const;

	void set_pitch_scale(float p_pitch_scale);
	float get_pitch_scale() const;

	void play(float p_from_pos = 0.0);
	void seek(float p_seconds);
	void stop();
	bool is_playing() const;
	float get_playback_position();

	void set_bus(const String& p_bus);
	String get_bus() const;

	void set_autoplay(bool p_enable);
	bool is_autoplay_enabled() const;

	void set_max_distance(float p_pixels);
	float get_max_distance() const;

	void set_attenuation(float p_curve);
	float get_attenuation() const;

	void set_area_mask(uint32_t p_mask);
	uint32_t get_area_mask() const;

	void set_stream_paused(bool p_pause);
	bool get_stream_paused() const;

	void set_max_polyphony(int p_max_polyphony);
	int get_max_polyphony() const;

	void set_panning_strength(float p_panning_strength);
	float get_panning_strength() const;

	bool has_stream_playback();
	Ref<AudioStreamPlayback> get_stream_playback();

	AudioServer::PlaybackType get_playback_type() const;
	void set_playback_type(AudioServer::PlaybackType p_playback_type);

	AudioStreamPlayer2D();
	~AudioStreamPlayer2D();
};



// --- End scene/2d/audio_stream_player_2d.h ---

// --- Folded from scene/2d/canvas_group.h ---

class CanvasGroup : public Node2D
{
	real_t fit_margin = 10.0;
	real_t clear_margin = 10.0;
	bool use_mipmaps = false;

protected:

public:
	void set_fit_margin(real_t p_fit_margin);
	real_t get_fit_margin() const;

	void set_clear_margin(real_t p_clear_margin);
	real_t get_clear_margin() const;

	void set_use_mipmaps(bool p_use_mipmaps);
	bool is_using_mipmaps() const;

	virtual PackedStringArray get_configuration_warnings() const override;

	CanvasGroup();
	~CanvasGroup();
};



// --- End scene/2d/canvas_group.h ---

// --- Folded from scene/2d/visible_on_screen_notifier_2d.h ---

class Viewport;

class VisibleOnScreenNotifier2D : public Node2D
{
public:
	enum EnableMode
	{
		ENABLE_MODE_INHERIT,
		ENABLE_MODE_ALWAYS,
		ENABLE_MODE_WHEN_PAUSED,
	};

private:
	Rect2 rect;
	bool show_rect = true;
	bool on_screen = false;
	bool enabler_enabled = false;
	EnableMode enable_mode = ENABLE_MODE_INHERIT;
	NodePath enable_node_path = NodePath("..");

	void _visibility_enter();
	void _visibility_exit();
	void _update_enable_mode(bool p_enable);

protected:
	void _notification(int p_what);

public:
#ifdef TOOLS_ENABLED
	virtual Vector2 _edit_get_minimum_size() const override { return Vector2(); }
	virtual void _edit_set_rect(const Rect2& p_edit_rect) override;
#endif // TOOLS_ENABLED

#ifdef DEBUG_ENABLED
	virtual Rect2 _edit_get_rect() const override;
	virtual bool _edit_use_rect() const override;
#endif // DEBUG_ENABLED

	void set_rect(const Rect2& p_rect);
	Rect2 get_rect() const;

	void set_show_rect(bool p_show_rect);
	bool is_showing_rect() const;

	bool is_on_screen() const;

	void set_enabler_enabled(bool p_enabled);
	bool is_enabler_enabled() const { return enabler_enabled; }

	void set_enable_mode(EnableMode p_mode);
	EnableMode get_enable_mode() const { return enable_mode; }

	void set_enable_node_path(const NodePath& p_path);
	NodePath get_enable_node_path() const { return enable_node_path; }

	VisibleOnScreenNotifier2D(bool p_enabler = false);
};

// --- End scene/2d/visible_on_screen_notifier_2d.h ---

// --- Folded from scene/2d/physics/collision_object_2d.h ---
#include "core/templates/local_vector.h"
#include "core/templates/rb_map.h"
// --- Inlined/Folded from scene/resources/2d/shape_2d.h ---
#include "core/io/resource.h"

class Shape2D : public Resource
{
public:
	enum ShapeType
	{
		SHAPE_WORLD_BOUNDARY,
		SHAPE_SEPARATION_RAY,
		SHAPE_SEGMENT,
		SHAPE_CIRCLE,
		SHAPE_RECTANGLE,
		SHAPE_CAPSULE,
		SHAPE_CONVEX_POLYGON,
		SHAPE_CONCAVE_POLYGON,
	};

private:
	ShapeType shape_type = SHAPE_RECTANGLE;
	RID shape;
	real_t custom_bias = 0.0;

	// World boundary parameters
	Vector2 normal = Vector2(0, -1);
	real_t distance = 0.0;

	// Separation ray parameters
	real_t length = 20.0;
	bool slide_on_slope = false;

	// Segment parameters
	Vector2 a;
	Vector2 b = Vector2(0, 10);

	// Circle / Capsule parameters
	real_t radius = 10.0;
	real_t height = 30.0;

	// Rectangle parameters
	Size2 size = Size2(20, 20);

	// Convex polygon parameters
	Vector<Vector2> points;

	// Concave polygon parameters
	Vector<Vector2> segments;

	static RID _create_shape_rid(ShapeType p_type);
	void _update_shape();
	Vector<Vector2> _get_capsule_points() const;

public:
	ShapeType get_shape_type() const { return shape_type; }
	void set_shape_type(ShapeType p_type);

	// World boundary
	void set_normal(const Vector2& p_normal);
	Vector2 get_normal() const;
	void set_distance(real_t p_distance);
	real_t get_distance() const;

	// Separation ray
	void set_length(real_t p_length);
	real_t get_length() const;
	void set_slide_on_slope(bool p_active);
	bool get_slide_on_slope() const;

	// Segment
	void set_a(const Vector2& p_a);
	Vector2 get_a() const;
	void set_b(const Vector2& p_b);
	Vector2 get_b() const;

	// Circle / Capsule
	void set_radius(real_t p_radius);
	real_t get_radius() const;
	void set_height(real_t p_height);
	real_t get_height() const;
	void set_mid_height(real_t p_mid_height);
	real_t get_mid_height() const;

	// Rectangle
	void set_size(const Size2& p_size);
	Size2 get_size() const;

	// Convex polygon
	void set_point_cloud(const Vector<Vector2>& p_points);
	void set_points(const Vector<Vector2>& p_points);
	Vector<Vector2> get_points() const;

	// Concave polygon
	void set_segments(const Vector<Vector2>& p_segments);
	Vector<Vector2> get_segments() const;

	bool _edit_is_selected_on_click(const Point2& p_point, double p_tolerance) const;

	void set_custom_solver_bias(real_t p_bias);
	real_t get_custom_solver_bias() const;

	bool collide_with_motion(const Transform2D& p_local_xform, const Vector2& p_local_motion,
		Shape2D* rp_shape, const Transform2D& p_shape_xform,
		const Vector2& p_shape_motion);
	bool collide(const Transform2D& p_local_xform, Shape2D* rp_shape,
		const Transform2D& p_shape_xform);

	PackedVector2Array collide_with_motion_and_get_contacts(const Transform2D& p_local_xform,
		const Vector2& p_local_motion, Shape2D* rp_shape,
		const Transform2D& p_shape_xform, const Vector2& p_shape_motion);
	PackedVector2Array collide_and_get_contacts(const Transform2D& p_local_xform,
		Shape2D* rp_shape, const Transform2D& p_shape_xform);

	void draw(const RID& p_to_rid, const Color& p_color);
	Rect2 get_rect() const;

	/// Returns the radius of a circle that fully encloses this shape.
	real_t get_enclosing_radius() const;
	virtual RID get_rid() const override;

	static bool is_collision_outline_enabled();

	Shape2D(ShapeType p_type = SHAPE_RECTANGLE);
	~Shape2D();
};
#include "servers/physics_2d/physics_server_2d.h"

// Plain non-virtual composed component for 2D collision state (shared by PhysicsBody2D and Area2D).
// Supports both inline ShapeSlots directly on the body/area and child CollisionShape2D/CollisionPolygon2D owners.
struct CollisionState2D {
	enum DisableMode {
		DISABLE_MODE_REMOVE,
		DISABLE_MODE_MAKE_STATIC,
		DISABLE_MODE_KEEP_ACTIVE,
	};

	struct ShapeSlot {
		Transform2D local_xform;
		Ref<Shape2D> shape;
		bool disabled = false;
		bool one_way_collision = false;
		real_t one_way_collision_margin = 0.0f;
		Vector2 one_way_collision_direction = Vector2(0.0f, 1.0f);
		uint32_t owner_id = 0;
	};

	struct ShapeData {
		Transform2D xform;
		Node *owner_node = nullptr;

		struct Shape {
			Ref<Shape2D> shape;
			int index = 0;
		};

		Vector<Shape> shapes;
		bool disabled = false;
		bool one_way_collision = false;
		real_t one_way_collision_margin = 0.0f;
		Vector2 one_way_collision_direction = Vector2(0.0f, 1.0f);
	};

	RID rid;
	bool area = false;
	uint32_t collision_layer = 1;
	uint32_t collision_mask = 1;
	real_t collision_priority = 1.0f;
	uint32_t callback_lock = 0;
	bool pickable = false;
	DisableMode disable_mode = DISABLE_MODE_REMOVE;
	PS2DE::BodyMode body_mode = PS2DE::BODY_MODE_STATIC;
	bool only_update_transform_changes = false;

	int total_subshapes = 0;
	uint32_t next_owner_id = 1;
	RBMap<uint32_t, ShapeData> shapes;
	LocalVector<ShapeSlot> inline_shapes;

	void init(bool p_area, PS2DE::BodyMode p_mode = PS2DE::BODY_MODE_STATIC);
	void free_rid();

	_FORCE_INLINE_ void lock_callback() { callback_lock++; }
	_FORCE_INLINE_ void unlock_callback() {
		ERR_FAIL_COND(callback_lock == 0);
		callback_lock--;
	}

	void handle_notification(Node2D *p_owner, int p_what);
	void apply_disabled(Node2D *p_owner);
	void apply_enabled(Node2D *p_owner);
	void update_pickable(const Node2D *p_owner);
	void set_body_mode(const Node2D *p_owner, PS2DE::BodyMode p_mode);

	void set_collision_layer(uint32_t p_layer);
	void set_collision_mask(uint32_t p_mask);
	void set_collision_layer_value(int p_layer_number, bool p_value);
	bool get_collision_layer_value(int p_layer_number) const;
	void set_collision_mask_value(int p_layer_number, bool p_value);
	bool get_collision_mask_value(int p_layer_number) const;
	void set_collision_priority(real_t p_priority);
	void set_disable_mode(Node2D *p_owner, DisableMode p_mode);
	void set_pickable(const Node2D *p_owner, bool p_enabled);

	// Inline shape slots (no child CollisionShape2D node required).
	int add_inline_shape(const Ref<Shape2D> &p_shape, const Transform2D &p_xform = Transform2D(), bool p_disabled = false);
	void set_inline_shape(int p_index, const Ref<Shape2D> &p_shape);
	Ref<Shape2D> get_inline_shape(int p_index) const;
	void set_inline_shape_transform(int p_index, const Transform2D &p_xform);
	Transform2D get_inline_shape_transform(int p_index) const;
	void set_inline_shape_disabled(int p_index, bool p_disabled);
	bool is_inline_shape_disabled(int p_index) const;
	void remove_inline_shape(int p_index);
	void clear_inline_shapes();
	int get_inline_shape_count() const { return (int)inline_shapes.size(); }

	// Shape owner API (for inline slots & child CollisionShape2D / CollisionPolygon2D).
	uint32_t create_shape_owner(Node *p_owner = nullptr);
	void remove_shape_owner(uint32_t p_owner);
	void get_shape_owners(List<uint32_t> *r_owners) const;
	PackedInt32Array get_shape_owners_array() const;

	void shape_owner_set_transform(uint32_t p_owner, const Transform2D &p_transform);
	Transform2D shape_owner_get_transform(uint32_t p_owner) const;
	void shape_owner_set_disabled(uint32_t p_owner, bool p_disabled);
	bool is_shape_owner_disabled(uint32_t p_owner) const;
	void shape_owner_set_one_way_collision(uint32_t p_owner, bool p_enable);
	bool is_shape_owner_one_way_collision_enabled(uint32_t p_owner) const;
	void shape_owner_set_one_way_collision_margin(uint32_t p_owner, real_t p_margin);
	real_t get_shape_owner_one_way_collision_margin(uint32_t p_owner) const;
	void shape_owner_set_one_way_collision_direction(uint32_t p_owner, const Vector2 &p_direction);
	Vector2 get_shape_owner_one_way_collision_direction(uint32_t p_owner) const;

	void shape_owner_add_shape(uint32_t p_owner, Shape2D *rp_shape);
	int shape_owner_get_shape_count(uint32_t p_owner) const;
	Ref<Shape2D> shape_owner_get_shape(uint32_t p_owner, int p_shape) const;
	int shape_owner_get_shape_index(uint32_t p_owner, int p_shape) const;
	void shape_owner_remove_shape(uint32_t p_owner, int p_shape);
	void shape_owner_clear_shapes(uint32_t p_owner);
	uint32_t shape_find_owner(int p_shape_index) const;

	bool has_any_shapes() const { return !shapes.is_empty(); }
};

// --- Folded from scene/2d/physics/area_2d.h ---
#include "core/templates/vector.h"


class Area2D : public Node2D {
public:
	enum SpaceOverride {
		SPACE_OVERRIDE_DISABLED,
		SPACE_OVERRIDE_COMBINE,
		SPACE_OVERRIDE_COMBINE_REPLACE,
		SPACE_OVERRIDE_REPLACE,
		SPACE_OVERRIDE_REPLACE_COMBINE
	};

	using DisableMode = CollisionState2D::DisableMode;

private:
	CollisionState2D collision_state;

	SpaceOverride gravity_space_override = SPACE_OVERRIDE_DISABLED;
	Vector2 gravity_vec = Vector2(0, 1);
	real_t gravity = 980.0f;
	bool gravity_is_point = false;
	real_t gravity_point_unit_distance = 0.0f;

	SpaceOverride linear_damp_space_override = SPACE_OVERRIDE_DISABLED;
	SpaceOverride angular_damp_space_override = SPACE_OVERRIDE_DISABLED;
	real_t linear_damp = 0.1f;
	real_t angular_damp = 1.0f;

	int priority = 0;

	bool monitoring = false;
	bool monitorable = false;
	bool locked = false;

	bool audio_bus_override = false;
	String audio_bus;

	void _clear_monitoring();

protected:
	void _notification(int p_what);

public:
	Area2D();
	~Area2D();

	virtual CollisionState2D *get_collision_state_2d() override { return &collision_state; }
	virtual const CollisionState2D *get_collision_state_2d() const override { return &collision_state; }

	_FORCE_INLINE_ RID get_rid() const { return collision_state.rid; }

	void set_collision_layer(uint32_t p_layer) { collision_state.set_collision_layer(p_layer); }
	uint32_t get_collision_layer() const { return collision_state.collision_layer; }
	void set_collision_mask(uint32_t p_mask) { collision_state.set_collision_mask(p_mask); }
	uint32_t get_collision_mask() const { return collision_state.collision_mask; }
	void set_collision_layer_value(int p_layer_number, bool p_value) { collision_state.set_collision_layer_value(p_layer_number, p_value); }
	bool get_collision_layer_value(int p_layer_number) const { return collision_state.get_collision_layer_value(p_layer_number); }
	void set_collision_mask_value(int p_layer_number, bool p_value) { collision_state.set_collision_mask_value(p_layer_number, p_value); }
	bool get_collision_mask_value(int p_layer_number) const { return collision_state.get_collision_mask_value(p_layer_number); }
	void set_disable_mode(DisableMode p_mode) { collision_state.set_disable_mode(this, p_mode); }
	DisableMode get_disable_mode() const { return collision_state.disable_mode; }
	void set_pickable(bool p_enabled) { collision_state.set_pickable(this, p_enabled); }
	bool is_pickable() const { return collision_state.pickable; }

	// Inline shape API.
	int add_inline_shape(const Ref<Shape2D> &p_shape, const Transform2D &p_xform = Transform2D(), bool p_disabled = false) {
		return collision_state.add_inline_shape(p_shape, p_xform, p_disabled);
	}
	void set_inline_shape(int p_index, const Ref<Shape2D> &p_shape) { collision_state.set_inline_shape(p_index, p_shape); }
	Ref<Shape2D> get_inline_shape(int p_index) const { return collision_state.get_inline_shape(p_index); }
	void remove_inline_shape(int p_index) { collision_state.remove_inline_shape(p_index); }
	void clear_inline_shapes() { collision_state.clear_inline_shapes(); }
	int get_inline_shape_count() const { return collision_state.get_inline_shape_count(); }

	void set_gravity_space_override_mode(SpaceOverride p_mode);
	SpaceOverride get_gravity_space_override_mode() const;

	void set_gravity_is_point(bool p_enabled);
	bool is_gravity_a_point() const;

	void set_gravity_point_unit_distance(real_t p_scale);
	real_t get_gravity_point_unit_distance() const;

	void set_gravity_point_center(const Vector2 &p_center);
	const Vector2 &get_gravity_point_center() const;

	void set_gravity_direction(const Vector2 &p_direction);
	const Vector2 &get_gravity_direction() const;

	void set_gravity(real_t p_gravity);
	real_t get_gravity() const;

	void set_linear_damp_space_override_mode(SpaceOverride p_mode);
	SpaceOverride get_linear_damp_space_override_mode() const;

	void set_angular_damp_space_override_mode(SpaceOverride p_mode);
	SpaceOverride get_angular_damp_space_override_mode() const;

	void set_linear_damp(real_t p_linear_damp);
	real_t get_linear_damp() const;

	void set_angular_damp(real_t p_angular_damp);
	real_t get_angular_damp() const;

	void set_priority(int p_priority);
	int get_priority() const;

	void set_monitoring(bool p_enable);
	bool is_monitoring() const;

	void set_monitorable(bool p_enable);
	bool is_monitorable() const;

	bool has_overlapping_bodies() const;
	bool has_overlapping_areas() const;

	bool overlaps_area(Node *rp_area) const;
	bool overlaps_body(Node *rp_body) const;

	void set_audio_bus_override(bool p_override);
	bool is_overriding_audio_bus() const;

	void set_audio_bus_name(const String &p_audio_bus);
	String get_audio_bus_name() const;

	virtual PackedStringArray get_configuration_warnings() const override;
};

// --- End scene/2d/physics/area_2d.h ---

// --- Folded from scene/2d/physics/collision_shape_2d.h ---


class CollisionShape2D : public Node2D {
	Ref<Shape2D> shape;
	Rect2 rect = Rect2(-Point2(10, 10), Point2(20, 20));
	uint32_t owner_id = 0;
	CollisionState2D *collision_state = nullptr;
	bool disabled = false;
	bool one_way_collision = false;
	real_t one_way_collision_margin = 1.0f;
	Vector2 one_way_collision_direction = Vector2(0.0f, 1.0f);

	void _shape_changed();
	void _update_in_shape_owner(bool p_xform_only = false);

	Color debug_color = Color(0.0, 0.0, 0.0, 0.0);
	Color _get_default_debug_color() const;

protected:
	void _notification(int p_what);

#ifdef DEBUG_ENABLED
	bool _property_can_revert(const String &p_name) const;
#endif // DEBUG_ENABLED

public:
#ifdef DEBUG_ENABLED
	virtual bool _edit_is_selected_on_click(const Point2 &p_point, double p_tolerance) const override;
#else
	virtual bool _edit_is_selected_on_click(const Point2 &p_point, double p_tolerance) const;
#endif // DEBUG_ENABLED

	void set_shape(const Ref<Shape2D> &p_shape);
	Ref<Shape2D> get_shape() const;

	void set_disabled(bool p_disabled);
	bool is_disabled() const;

	void set_one_way_collision(bool p_enable);
	bool is_one_way_collision_enabled() const;

	void set_one_way_collision_margin(real_t p_margin);
	real_t get_one_way_collision_margin() const;

	void set_one_way_collision_direction(const Vector2 &p_direction);
	Vector2 get_one_way_collision_direction() const;

	void set_debug_color(const Color &p_color);
	Color get_debug_color() const;

	PackedStringArray get_configuration_warnings() const override;

	CollisionShape2D();
};

// --- End scene/2d/physics/collision_shape_2d.h ---

// --- Folded from scene/2d/physics/kinematic_collision_2d.h ---
#include "core/types.h"
#include "servers/physics_2d/physics_server_2d.h"

class PhysicsBody2D;

class KinematicCollision2D : public RefCounted
{
	friend class PhysicsBody2D;
	PS2DT::MotionResult result;

protected:

public:
	Vector2 get_position() const;
	Vector2 get_normal() const;
	Vector2 get_travel() const;
	Vector2 get_remainder() const;
	real_t get_angle(const Vector2& p_up_direction = Vector2(0.0, -1.0)) const;
	real_t get_depth() const;
	RID get_collider_rid() const;
	int get_collider_shape_index() const;
	Vector2 get_collider_velocity() const;
};



// --- End scene/2d/physics/kinematic_collision_2d.h ---

// --- Folded from scene/2d/physics/physics_body_2d.h ---


#include "scene/resources/material.h"
#include "servers/physics_2d/physics_server_2d.h"

class Joint2D;
class Skeleton2D;

class PhysicsBody2D : public Node2D {
public:
	enum BodyMode {
		MODE_STATIC,
		MODE_ANIMATABLE,
		MODE_CHARACTER,
		MODE_RIGID,
		MODE_PHYSICAL_BONE,
	};

	enum FreezeMode {
		FREEZE_MODE_STATIC,
		FREEZE_MODE_KINEMATIC,
	};

	enum CenterOfMassMode {
		CENTER_OF_MASS_MODE_AUTO,
		CENTER_OF_MASS_MODE_CUSTOM,
	};

	enum DampMode {
		DAMP_MODE_COMBINE,
		DAMP_MODE_REPLACE,
	};

	enum CCDMode {
		CCD_MODE_DISABLED,
		CCD_MODE_CAST_RAY,
		CCD_MODE_CAST_SHAPE,
	};

	enum MotionMode {
		MOTION_MODE_GROUNDED,
		MOTION_MODE_FLOATING,
	};

	enum PlatformOnLeave {
		PLATFORM_ON_LEAVE_ADD_VELOCITY,
		PLATFORM_ON_LEAVE_ADD_UPWARD_VELOCITY,
		PLATFORM_ON_LEAVE_DO_NOTHING,
	};

	using DisableMode = CollisionState2D::DisableMode;

	// Plain composed character mover state (for MODE_CHARACTER).
	struct CharacterMover2D {
		real_t margin = 0.08f;
		MotionMode motion_mode = MOTION_MODE_GROUNDED;
		PlatformOnLeave platform_on_leave = PLATFORM_ON_LEAVE_ADD_VELOCITY;

		bool floor_constant_speed = false;
		bool floor_stop_on_slope = true;
		bool floor_block_on_wall = true;
		bool slide_on_ceiling = true;
		int max_slides = 4;
		int platform_layer = 0;
		real_t floor_max_angle = Math::deg_to_rad((real_t)45.0);
		real_t floor_snap_length = 1.0f;
		real_t wall_min_slide_angle = Math::deg_to_rad((real_t)15.0);
		Vector2 up_direction = Vector2(0.0f, -1.0f);
		uint32_t platform_floor_layers = UINT32_MAX;
		uint32_t platform_wall_layers = 0;
		Vector2 velocity;

		Vector2 floor_normal;
		Vector2 platform_velocity;
		Vector2 wall_normal;
		Vector2 last_motion;
		Vector2 previous_position;
		Vector2 real_velocity;

		RID platform_rid;
		bool on_floor = false;
		bool on_ceiling = false;
		bool on_wall = false;

		Vector<PS2DT::MotionResult> motion_results;
		Vector<Ref<KinematicCollision2D>> slide_colliders;
	};

	// Plain composed physical bone attachment state (for MODE_PHYSICAL_BONE).
	struct PhysicalBoneState2D {
		Skeleton2D *parent_skeleton = nullptr;
		int bone2d_index = -1;
		NodePath bone2d_nodepath;
		bool follow_bone_when_simulating = false;
		Joint2D *child_joint = nullptr;
		bool auto_configure_joint = true;
		bool simulate_physics = false;
		bool internal_simulate_physics = false;
	};

private:
	BodyMode mode = MODE_STATIC;
	CollisionState2D collision_state;
	Ref<KinematicCollision2D> motion_cache;

	// Static / Animatable properties.
	Vector2 constant_linear_velocity;
	real_t constant_angular_velocity = 0.0f;
	bool sync_to_physics = true;
	Transform2D last_valid_transform;

	// Rigid / PhysicalBone properties.
	Ref<PhysicsMaterial> physics_material_override;
	bool can_sleep = true;
	bool lock_rotation = false;
	bool freeze = false;
	FreezeMode freeze_mode = FREEZE_MODE_STATIC;

	real_t mass = 1.0f;
	real_t inertia = 0.0f;
	CenterOfMassMode center_of_mass_mode = CENTER_OF_MASS_MODE_AUTO;
	Vector2 center_of_mass;
	real_t gravity_scale = 1.0f;

	DampMode linear_damp_mode = DAMP_MODE_COMBINE;
	DampMode angular_damp_mode = DAMP_MODE_COMBINE;
	real_t linear_damp = 0.0f;
	real_t angular_damp = 0.0f;

	Vector2 linear_velocity;
	real_t angular_velocity = 0.0f;
	bool sleeping = false;
	int max_contacts_reported = 0;
	int contact_count = 0;
	bool custom_integrator = false;
	CCDMode ccd_mode = CCD_MODE_DISABLED;

	CharacterMover2D character;
	PhysicalBoneState2D bone_state;

	void _apply_server_body_mode();
	void _reload_physics_characteristics();
	static void _body_state_changed_callback(void *p_instance, PhysicsDirectBodyState2D *p_state);
	void _body_state_changed(PhysicsDirectBodyState2D *p_state);

	void _position_at_bone2d();
	void _start_bone_physics_simulation();
	void _stop_bone_physics_simulation();

protected:
	void _notification(int p_what);

public:
	PhysicsBody2D(BodyMode p_mode = MODE_STATIC);
	~PhysicsBody2D();

	void set_body_mode(BodyMode p_mode);
	BodyMode get_body_mode() const { return mode; }

	virtual CollisionState2D *get_collision_state_2d() override { return &collision_state; }
	virtual const CollisionState2D *get_collision_state_2d() const override { return &collision_state; }

	_FORCE_INLINE_ RID get_rid() const { return collision_state.rid; }

	// Collision state forwarding.
	void set_collision_layer(uint32_t p_layer) { collision_state.set_collision_layer(p_layer); }
	uint32_t get_collision_layer() const { return collision_state.collision_layer; }
	void set_collision_mask(uint32_t p_mask) { collision_state.set_collision_mask(p_mask); }
	uint32_t get_collision_mask() const { return collision_state.collision_mask; }
	void set_collision_layer_value(int p_layer_number, bool p_value) { collision_state.set_collision_layer_value(p_layer_number, p_value); }
	bool get_collision_layer_value(int p_layer_number) const { return collision_state.get_collision_layer_value(p_layer_number); }
	void set_collision_mask_value(int p_layer_number, bool p_value) { collision_state.set_collision_mask_value(p_layer_number, p_value); }
	bool get_collision_mask_value(int p_layer_number) const { return collision_state.get_collision_mask_value(p_layer_number); }
	void set_collision_priority(real_t p_priority) { collision_state.set_collision_priority(p_priority); }
	real_t get_collision_priority() const { return collision_state.collision_priority; }
	void set_disable_mode(DisableMode p_mode) { collision_state.set_disable_mode(this, p_mode); }
	DisableMode get_disable_mode() const { return collision_state.disable_mode; }
	void set_pickable(bool p_enabled) { collision_state.set_pickable(this, p_enabled); }
	bool is_pickable() const { return collision_state.pickable; }

	// Inline shape slots (direct collision shapes without child CollisionShape2D).
	int add_inline_shape(const Ref<Shape2D> &p_shape, const Transform2D &p_xform = Transform2D(), bool p_disabled = false) {
		return collision_state.add_inline_shape(p_shape, p_xform, p_disabled);
	}
	void set_inline_shape(int p_index, const Ref<Shape2D> &p_shape) { collision_state.set_inline_shape(p_index, p_shape); }
	Ref<Shape2D> get_inline_shape(int p_index) const { return collision_state.get_inline_shape(p_index); }
	void set_inline_shape_transform(int p_index, const Transform2D &p_xform) { collision_state.set_inline_shape_transform(p_index, p_xform); }
	Transform2D get_inline_shape_transform(int p_index) const { return collision_state.get_inline_shape_transform(p_index); }
	void set_inline_shape_disabled(int p_index, bool p_disabled) { collision_state.set_inline_shape_disabled(p_index, p_disabled); }
	bool is_inline_shape_disabled(int p_index) const { return collision_state.is_inline_shape_disabled(p_index); }
	void remove_inline_shape(int p_index) { collision_state.remove_inline_shape(p_index); }
	void clear_inline_shapes() { collision_state.clear_inline_shapes(); }
	int get_inline_shape_count() const { return collision_state.get_inline_shape_count(); }

	// Common PhysicsBody2D motion & exceptions.
	bool move_and_collide(const PS2DT::MotionParameters &p_parameters, PS2DT::MotionResult &r_result, bool p_test_only = false, bool p_cancel_sliding = true);
	bool test_move(const Transform2D &p_from, const Vector2 &p_motion, const Ref<KinematicCollision2D> &r_collision = Ref<KinematicCollision2D>(), real_t p_margin = 0.08f, bool p_recovery_as_collision = false);
	Vector2 get_gravity() const;
	void add_collision_exception_with(Node *rp_node);
	void remove_collision_exception_with(Node *rp_node);

	// Static / Animatable API.
	void set_physics_material_override(const Ref<PhysicsMaterial> &p_physics_material_override);
	Ref<PhysicsMaterial> get_physics_material_override() const { return physics_material_override; }
	void set_constant_linear_velocity(const Vector2 &p_vel);
	Vector2 get_constant_linear_velocity() const { return constant_linear_velocity; }
	void set_constant_angular_velocity(real_t p_vel);
	real_t get_constant_angular_velocity() const { return constant_angular_velocity; }
	void set_sync_to_physics(bool p_enable);
	bool is_sync_to_physics_enabled() const { return sync_to_physics; }

	// RigidBody API.
	void set_lock_rotation_enabled(bool p_lock_rotation);
	bool is_lock_rotation_enabled() const { return lock_rotation; }
	void set_freeze_enabled(bool p_freeze);
	bool is_freeze_enabled() const { return freeze; }
	void set_freeze_mode(FreezeMode p_freeze_mode);
	FreezeMode get_freeze_mode() const { return freeze_mode; }

	void set_mass(real_t p_mass);
	real_t get_mass() const { return mass; }
	void set_inertia(real_t p_inertia);
	real_t get_inertia() const { return inertia; }
	void set_center_of_mass_mode(CenterOfMassMode p_mode);
	CenterOfMassMode get_center_of_mass_mode() const { return center_of_mass_mode; }
	void set_center_of_mass(const Vector2 &p_center_of_mass);
	const Vector2 &get_center_of_mass() const { return center_of_mass; }

	void set_gravity_scale(real_t p_gravity_scale);
	real_t get_gravity_scale() const { return gravity_scale; }
	void set_linear_damp_mode(DampMode p_mode);
	DampMode get_linear_damp_mode() const { return linear_damp_mode; }
	void set_angular_damp_mode(DampMode p_mode);
	DampMode get_angular_damp_mode() const { return angular_damp_mode; }
	void set_linear_damp(real_t p_linear_damp);
	real_t get_linear_damp() const { return linear_damp; }
	void set_angular_damp(real_t p_angular_damp);
	real_t get_angular_damp() const { return angular_damp; }

	void set_linear_velocity(const Vector2 &p_velocity);
	Vector2 get_linear_velocity() const { return linear_velocity; }
	void set_axis_velocity(const Vector2 &p_axis);
	void set_angular_velocity(real_t p_velocity);
	real_t get_angular_velocity() const { return angular_velocity; }

	void set_use_custom_integrator(bool p_enable);
	bool is_using_custom_integrator() const { return custom_integrator; }
	void set_sleeping(bool p_sleeping);
	bool is_sleeping() const { return sleeping; }
	void set_can_sleep(bool p_active);
	bool is_able_to_sleep() const { return can_sleep; }
	void set_max_contacts_reported(int p_amount);
	int get_max_contacts_reported() const { return max_contacts_reported; }
	int get_contact_count() const { return contact_count; }
	void set_continuous_collision_detection_mode(CCDMode p_mode);
	CCDMode get_continuous_collision_detection_mode() const { return ccd_mode; }

	void apply_central_impulse(const Vector2 &p_impulse);
	void apply_impulse(const Vector2 &p_impulse, const Vector2 &p_position = Vector2());
	void apply_torque_impulse(real_t p_torque);
	void apply_central_force(const Vector2 &p_force);
	void apply_force(const Vector2 &p_force, const Vector2 &p_position = Vector2());
	void apply_torque(real_t p_torque);
	void add_constant_central_force(const Vector2 &p_force);
	void add_constant_force(const Vector2 &p_force, const Vector2 &p_position = Vector2());
	void add_constant_torque(real_t p_torque);
	void set_constant_force(const Vector2 &p_force);
	Vector2 get_constant_force() const;
	void set_constant_torque(real_t p_torque);
	real_t get_constant_torque() const;

	// CharacterMover2D API.
	CharacterMover2D &get_character_mover() { return character; }
	const CharacterMover2D &get_character_mover() const { return character; }
	bool move_and_slide();
	void apply_floor_snap();
	const Vector2 &get_velocity() const { return character.velocity; }
	void set_velocity(const Vector2 &p_velocity) { character.velocity = p_velocity; }
	const Vector2 &get_up_direction() const { return character.up_direction; }
	void set_up_direction(const Vector2 &p_up_direction);
	bool is_on_floor() const { return character.on_floor; }
	bool is_on_floor_only() const { return character.on_floor && !character.on_wall && !character.on_ceiling; }
	bool is_on_wall() const { return character.on_wall; }
	bool is_on_wall_only() const { return character.on_wall && !character.on_floor && !character.on_ceiling; }
	bool is_on_ceiling() const { return character.on_ceiling; }
	bool is_on_ceiling_only() const { return character.on_ceiling && !character.on_floor && !character.on_wall; }
	const Vector2 &get_last_motion() const { return character.last_motion; }
	Vector2 get_position_delta() const { return get_global_transform().columns[2] - character.previous_position; }
	const Vector2 &get_floor_normal() const { return character.floor_normal; }
	const Vector2 &get_wall_normal() const { return character.wall_normal; }
	const Vector2 &get_real_velocity() const { return character.real_velocity; }
	real_t get_floor_angle(const Vector2 &p_up_direction = Vector2(0.0f, -1.0f)) const;
	const Vector2 &get_platform_velocity() const { return character.platform_velocity; }
	int get_slide_collision_count() const { return character.motion_results.size(); }
	PS2DT::MotionResult get_slide_collision(int p_bounce) const;
	void set_safe_margin(real_t p_margin) { character.margin = p_margin; }
	real_t get_safe_margin() const { return character.margin; }
	void set_motion_mode(MotionMode p_mode) { character.motion_mode = p_mode; }
	MotionMode get_motion_mode() const { return character.motion_mode; }
	void set_platform_on_leave(PlatformOnLeave p_on_leave) { character.platform_on_leave = p_on_leave; }
	PlatformOnLeave get_platform_on_leave() const { return character.platform_on_leave; }
	void set_max_slides(int p_max_slides);
	int get_max_slides() const { return character.max_slides; }
	void set_floor_max_angle(real_t p_radians) { character.floor_max_angle = p_radians; }
	real_t get_floor_max_angle() const { return character.floor_max_angle; }
	void set_floor_snap_length(real_t p_len);
	real_t get_floor_snap_length() const { return character.floor_snap_length; }
	void set_wall_min_slide_angle(real_t p_radians) { character.wall_min_slide_angle = p_radians; }
	real_t get_wall_min_slide_angle() const { return character.wall_min_slide_angle; }

	// PhysicalBoneState2D API.
	PhysicalBoneState2D &get_physical_bone_state() { return bone_state; }
	const PhysicalBoneState2D &get_physical_bone_state() const { return bone_state; }
	Joint2D *get_joint() const { return bone_state.child_joint; }
	bool get_auto_configure_joint() const { return bone_state.auto_configure_joint; }
	void set_auto_configure_joint(bool p_auto_configure);
	void set_simulate_physics(bool p_simulate);
	bool get_simulate_physics() const { return bone_state.simulate_physics; }
	bool is_simulating_physics() const { return bone_state.internal_simulate_physics; }
	void set_bone2d_nodepath(const NodePath &p_nodepath) { bone_state.bone2d_nodepath = p_nodepath; }
	NodePath get_bone2d_nodepath() const { return bone_state.bone2d_nodepath; }
	void set_bone2d_index(int p_bone_idx) { bone_state.bone2d_index = p_bone_idx; }
	int get_bone2d_index() const { return bone_state.bone2d_index; }
	void set_follow_bone_when_simulating(bool p_follow);
	bool get_follow_bone_when_simulating() const { return bone_state.follow_bone_when_simulating; }

	virtual PackedStringArray get_configuration_warnings() const override;
};

// --- End scene/2d/physics/physics_body_2d.h ---

// --- Folded from scene/2d/sprite_2d.h ---
#include "scene/resources/texture.h"

class Sprite2D : public Node2D
{
	Ref<Texture2D> texture;

#ifdef TOOLS_ENABLED
	bool dragging_to_resize_rect = false;
#endif // TOOLS_ENABLED

	bool centered = true;
	Point2 offset;

	bool hflip = false;
	bool vflip = false;
	bool region_enabled = false;
	Rect2 region_rect;
	bool region_filter_clip_enabled = false;

	int frame = 0;

	int vframes = 1;
	int hframes = 1;

	void _get_rects(Rect2& r_src_rect, Rect2& r_dst_rect, bool& r_filter_clip_enabled) const;
	Point2 _get_rect_offset(const Size2i& p_size) const;

	void _texture_changed();
	void _emit_region_rect_enabled();

protected:
	void _notification(int p_what);


public:
#ifdef DEBUG_ENABLED
	virtual bool _edit_is_selected_on_click(
		const Point2& p_point, double p_tolerance) const override;

	virtual Rect2 _edit_get_rect() const override;
	virtual bool _edit_use_rect() const override;
#endif // DEBUG_ENABLED

	bool is_pixel_opaque(const Point2& p_point) const;
	bool is_editor_region_rect_draggable() const;

	void set_texture(const Ref<Texture2D>& p_texture);
	Ref<Texture2D> get_texture() const;

	void set_centered(bool p_center);
	bool is_centered() const;

	void set_offset(const Point2& p_offset);
	Point2 get_offset() const;

	void set_flip_h(bool p_flip);
	bool is_flipped_h() const;

	void set_flip_v(bool p_flip);
	bool is_flipped_v() const;

	void set_region_enabled(bool p_enabled);
	bool is_region_enabled() const;

	void set_region_filter_clip_enabled(bool p_enabled);
	bool is_region_filter_clip_enabled() const;

	void set_region_rect(const Rect2& p_region_rect);
	Rect2 get_region_rect() const;

	void set_frame(int p_frame);
	int get_frame() const;

	void set_frame_coords(const Vector2i& p_coord);
	Vector2i get_frame_coords() const;

	void set_vframes(int p_amount);
	int get_vframes() const;

	void set_hframes(int p_amount);
	int get_hframes() const;

	Rect2 get_rect() const;
	virtual Rect2 get_anchorable_rect() const override;

	Sprite2D();
};

// --- Folded from scene/2d/polygon_2d.h ---

class NavigationPolygon;
class NavigationMeshSourceGeometryData2D;

class Polygon2D : public Node2D
{
	Vector<Vector2> polygon;
	Vector<Vector2> uv;
	Vector<Color> vertex_colors;
	int internal_vertices = 0;

	struct Bone
	{
		NodePath path;
		Vector<float> weights;
	};

	Vector<Bone> bone_weights;

	Color color = Color(1, 1, 1);
	Ref<Texture2D> texture;

	Size2 tex_scale = Vector2(1, 1);
	Vector2 tex_ofs;
	real_t tex_rot = 0.0;
	bool invert = false;
	real_t invert_border = 100.0;
	bool antialiased = false;

	Vector2 offset;
	mutable bool rect_cache_dirty = true;
	mutable Rect2 item_rect;

	NodePath skeleton;

	void _skeleton_bone_setup_changed();

	RID mesh;
	int last_len = 0;
	int last_index_count = 0;
	bool last_has_uv = false;
	bool last_has_color = false;
	bool last_has_bones = false;

protected:
	void _notification(int p_what);

public:
#ifdef TOOLS_ENABLED
	virtual void _edit_set_pivot(const Point2& p_pivot) override;
	virtual Point2 _edit_get_pivot() const override;
	virtual bool _edit_use_pivot() const override;
#endif // TOOLS_ENABLED

#ifdef DEBUG_ENABLED
	virtual Rect2 _edit_get_rect() const override;
	virtual bool _edit_use_rect() const override;

	virtual bool _edit_is_selected_on_click(
		const Point2& p_point, double p_tolerance) const override;
#endif // DEBUG_ENABLED

	void set_polygon(const Vector<Vector2>& p_polygon);
	Vector<Vector2> get_polygon() const;

	void set_internal_vertex_count(int p_count);
	int get_internal_vertex_count() const;

	void set_uv(const Vector<Vector2>& p_uv);
	Vector<Vector2> get_uv() const;

	void set_color(const Color& p_color);
	Color get_color() const;

	void set_vertex_colors(const Vector<Color>& p_colors);
	Vector<Color> get_vertex_colors() const;

	void set_texture(const Ref<Texture2D>& p_texture);
	Ref<Texture2D> get_texture() const;

	void set_texture_offset(const Vector2& p_offset);
	Vector2 get_texture_offset() const;

	void set_texture_rotation(real_t p_rot);
	real_t get_texture_rotation() const;

	void set_texture_scale(const Size2& p_scale);
	Size2 get_texture_scale() const;

	void set_invert(bool p_invert);
	bool get_invert() const;

	void set_antialiased(bool p_antialiased);
	bool get_antialiased() const;

	void set_invert_border(real_t p_invert_border);
	real_t get_invert_border() const;

	void set_offset(const Vector2& p_offset);
	Vector2 get_offset() const;

	void add_bone(
		const NodePath& p_path = NodePath(), const Vector<float>& p_weights = Vector<float>());
	int get_bone_count() const;
	NodePath get_bone_path(int p_index) const;
	const Vector<float>& get_bone_weights(int p_index) const _LIFETIME_BOUND_;
	void erase_bone(int p_idx);
	void clear_bones();
	void set_bone_weights(int p_index, const Vector<float>& p_weights);
	void set_bone_path(int p_index, const NodePath& p_path);

	void set_skeleton(const NodePath& p_skeleton);
	NodePath get_skeleton() const;

#ifndef NAVIGATION_2D_DISABLED
private:
	static RID _navmesh_source_geometry_parser;
#endif // NAVIGATION_2D_DISABLED

public:
#ifndef NAVIGATION_2D_DISABLED
	static void navmesh_parse_init();
	static void navmesh_parse_source_geometry(const Ref<NavigationPolygon>& p_navigation_mesh,
		Ref<NavigationMeshSourceGeometryData2D> p_source_geometry_data, Node* p_node);
#endif // NAVIGATION_2D_DISABLED

	Polygon2D();
	~Polygon2D();
};



// --- End scene/2d/polygon_2d.h ---

// --- Folded from scene/2d/skeleton_2d.h ---
// --- Folded from scene/resources/2d/skeleton/skeleton_modification_2d.h ---
// --- Folded from scene/resources/2d/skeleton/skeleton_modification_stack_2d.h ---
#include "core/io/resource.h"

///////////////////////////////////////
// SkeletonModificationStack2D
///////////////////////////////////////

class Skeleton2D;
class SkeletonModification2D;
class Bone2D;

class SkeletonModificationStack2D : public Resource
{
	friend class Skeleton2D;
	friend class SkeletonModification2D;

protected:

public:
	Skeleton2D* skeleton = nullptr;
	bool is_setup = false;
	bool enabled = false;
	float strength = 1.0;

	enum EXECUTION_MODE
	{
		execution_mode_process,
		execution_mode_physics_process
	};

	Vector<Ref<SkeletonModification2D>> modifications;

	void setup();
	void execute(float p_delta, int p_execution_mode);

	bool editor_gizmo_dirty = false;
	void draw_editor_gizmos();
	void set_editor_gizmos_dirty(bool p_dirty);

	void enable_all_modifications(bool p_enable);
	Ref<SkeletonModification2D> get_modification(int p_mod_idx) const;
	void add_modification(Ref<SkeletonModification2D> p_mod);
	void delete_modification(int p_mod_idx);
	void set_modification(int p_mod_idx, Ref<SkeletonModification2D> p_mod);

	void set_modification_count(int p_count);
	int get_modification_count() const;

	void set_skeleton(Skeleton2D* p_skeleton);
	Skeleton2D* get_skeleton() const;

	bool get_is_setup() const;

	void set_enabled(bool p_enabled);
	bool get_enabled() const;

	void set_strength(float p_strength);
	float get_strength() const;

	SkeletonModificationStack2D();
};



// --- End scene/resources/2d/skeleton/skeleton_modification_stack_2d.h ---

class Bone2D;
class Node2D;

class SkeletonModification2D final : public Resource {
	friend class Skeleton2D;
	friend class Bone2D;

public:
	enum Type {
		MODIFICATION_NONE,
		MODIFICATION_LOOKAT,
		MODIFICATION_CCDIK,
		MODIFICATION_FABRIK,
		MODIFICATION_JIGGLE,
		MODIFICATION_TWOBONEIK,
		MODIFICATION_PHYSICALBONES,
		MODIFICATION_STACKHOLDER,
	};

	struct LookAtData;
	struct CCDIKData;
	struct FABRIKData;
	struct JiggleData;
	struct TwoBoneIKData;
	struct PhysicalBonesData;
	struct StackHolderData;

private:
	Type modification_type = MODIFICATION_NONE;
	void *mode_data = nullptr;

	SkeletonModificationStack2D *stack = nullptr;
	int execution_mode = 0; // 0 = process
	NodePath target_node;

	bool enabled = true;
	bool is_setup = false;

	void _alloc_mode_data(Type p_type);
	void _free_mode_data();
	void _ensure_mode(Type p_expected);

	LookAtData *_get_lookat();
	const LookAtData *_get_lookat() const;
	CCDIKData *_get_ccdik();
	const CCDIKData *_get_ccdik() const;
	FABRIKData *_get_fabrik();
	const FABRIKData *_get_fabrik() const;
	JiggleData *_get_jiggle();
	const JiggleData *_get_jiggle() const;
	TwoBoneIKData *_get_twoboneik();
	const TwoBoneIKData *_get_twoboneik() const;
	PhysicalBonesData *_get_physicalbones();
	const PhysicalBonesData *_get_physicalbones() const;
	StackHolderData *_get_stackholder();
	const StackHolderData *_get_stackholder() const;

	Node2D *_resolve_node_2d(const NodePath &p_path) const;
	Bone2D *_resolve_bone_2d(const NodePath &p_path, int &r_bone_idx) const;

	void _execute_lookat(float p_delta);
	void _execute_ccdik(float p_delta);
	void _execute_ccdik_joint(int p_joint_idx, Node2D *p_target, Node2D *p_tip);
	void _execute_fabrik(float p_delta);
	void _fabrik_chain_backwards();
	void _fabrik_chain_forwards();
	void _execute_jiggle(float p_delta);
	void _execute_jiggle_joint(int p_joint_idx, Node2D *p_target, float p_delta);
	void _update_jiggle_joint_data();
	void _execute_twoboneik(float p_delta);
	void _execute_physicalbones(float p_delta);
	void _execute_stackholder(float p_delta);

	bool _print_execution_error(bool p_condition, const String &p_message);

	virtual void reset_state() override;

public:
	bool editor_draw_gizmo = false;

	SkeletonModification2D(Type p_type = MODIFICATION_NONE);
	~SkeletonModification2D();

	void set_modification_type(Type p_type);
	Type get_modification_type() const { return modification_type; }

	void _execute(float p_delta);
	void _setup_modification(SkeletonModificationStack2D *p_stack);
	void _draw_editor_gizmo();

	void set_editor_draw_gizmo(bool p_draw_gizmo);
	bool get_editor_draw_gizmo() const;

	void set_enabled(bool p_enabled);
	bool get_enabled() const;

	Ref<SkeletonModificationStack2D> get_modification_stack();
	void set_is_setup(bool p_setup);
	bool get_is_setup() const;

	void set_execution_mode(int p_mode);
	int get_execution_mode() const;

	float clamp_angle(float p_angle, float p_min_bound, float p_max_bound, bool p_invert_clamp = false);
	void editor_draw_angle_constraints(Bone2D *p_operation_bone, float p_min_bound, float p_max_bound, bool p_constraint_enabled, bool p_constraint_in_localspace, bool p_constraint_inverted);

	// Shared target node (LookAt, CCDIK, FABRIK, Jiggle, TwoBoneIK).
	void set_target_node(const NodePath &p_target_node);
	NodePath get_target_node() const;

	// LookAt accessors.
	void set_bone2d_node(const NodePath &p_target_node);
	NodePath get_bone2d_node() const;
	void set_bone_index(int p_idx);
	int get_bone_index() const;
	void set_additional_rotation(float p_rotation);
	float get_additional_rotation() const;
	void set_enable_constraint(bool p_constraint);
	bool get_enable_constraint() const;
	void set_constraint_angle_min(float p_angle_min);
	float get_constraint_angle_min() const;
	void set_constraint_angle_max(float p_angle_max);
	float get_constraint_angle_max() const;
	void set_constraint_angle_invert(bool p_invert);
	bool get_constraint_angle_invert() const;
	void set_constraint_in_localspace(bool p_constraint_in_localspace);
	bool get_constraint_in_localspace() const;

	// CCDIK accessors.
	void set_tip_node(const NodePath &p_tip_node);
	NodePath get_tip_node() const;
	int get_ccdik_data_chain_length() const;
	void set_ccdik_data_chain_length(int p_new_length);
	void set_ccdik_joint_bone2d_node(int p_joint_idx, const NodePath &p_target_node);
	NodePath get_ccdik_joint_bone2d_node(int p_joint_idx) const;
	void set_ccdik_joint_bone_index(int p_joint_idx, int p_bone_idx);
	int get_ccdik_joint_bone_index(int p_joint_idx) const;
	void set_ccdik_joint_rotate_from_joint(int p_joint_idx, bool p_rotate_from_joint);
	bool get_ccdik_joint_rotate_from_joint(int p_joint_idx) const;
	void set_ccdik_joint_enable_constraint(int p_joint_idx, bool p_constraint);
	bool get_ccdik_joint_enable_constraint(int p_joint_idx) const;
	void set_ccdik_joint_constraint_angle_min(int p_joint_idx, float p_angle_min);
	float get_ccdik_joint_constraint_angle_min(int p_joint_idx) const;
	void set_ccdik_joint_constraint_angle_max(int p_joint_idx, float p_angle_max);
	float get_ccdik_joint_constraint_angle_max(int p_joint_idx) const;
	void set_ccdik_joint_constraint_angle_invert(int p_joint_idx, bool p_invert);
	bool get_ccdik_joint_constraint_angle_invert(int p_joint_idx) const;
	void set_ccdik_joint_constraint_in_localspace(int p_joint_idx, bool p_constraint_in_localspace);
	bool get_ccdik_joint_constraint_in_localspace(int p_joint_idx) const;
	void set_ccdik_joint_editor_draw_gizmo(int p_joint_idx, bool p_draw_gizmo);
	bool get_ccdik_joint_editor_draw_gizmo(int p_joint_idx) const;

	// FABRIK accessors.
	int get_fabrik_data_chain_length() const;
	void set_fabrik_data_chain_length(int p_new_length);
	void set_fabrik_joint_bone2d_node(int p_joint_idx, const NodePath &p_target_node);
	NodePath get_fabrik_joint_bone2d_node(int p_joint_idx) const;
	void set_fabrik_joint_bone_index(int p_joint_idx, int p_bone_idx);
	int get_fabrik_joint_bone_index(int p_joint_idx) const;
	void set_fabrik_joint_magnet_position(int p_joint_idx, Vector2 p_magnet_position);
	Vector2 get_fabrik_joint_magnet_position(int p_joint_idx) const;
	void set_fabrik_joint_use_target_rotation(int p_joint_idx, bool p_use_target_rotation);
	bool get_fabrik_joint_use_target_rotation(int p_joint_idx) const;
	void set_chain_tolerance(float p_tolerance);
	float get_chain_tolerance() const;
	void set_chain_max_iterations(int p_iterations);
	int get_chain_max_iterations() const;

	// Jiggle accessors.
	void reset();
	void set_stiffness(float p_stiffness);
	float get_stiffness() const;
	void set_mass(float p_mass);
	float get_mass() const;
	void set_damping(float p_damping);
	float get_damping() const;
	void set_use_gravity(bool p_use_gravity);
	bool get_use_gravity() const;
	void set_gravity(Vector2 p_gravity);
	Vector2 get_gravity() const;
	void set_use_colliders(bool p_use_colliders);
	bool get_use_colliders() const;
	void set_collision_mask(int p_mask);
	int get_collision_mask() const;
	int get_jiggle_data_chain_length() const;
	void set_jiggle_data_chain_length(int p_new_length);
	void set_jiggle_joint_bone2d_node(int p_joint_idx, const NodePath &p_target_node);
	NodePath get_jiggle_joint_bone2d_node(int p_joint_idx) const;
	void set_jiggle_joint_bone_index(int p_joint_idx, int p_bone_idx);
	int get_jiggle_joint_bone_index(int p_joint_idx) const;
	void set_jiggle_joint_override(int p_joint_idx, bool p_override);
	bool get_jiggle_joint_override(int p_joint_idx) const;
	void set_jiggle_joint_stiffness(int p_joint_idx, float p_stiffness);
	float get_jiggle_joint_stiffness(int p_joint_idx) const;
	void set_jiggle_joint_mass(int p_joint_idx, float p_mass);
	float get_jiggle_joint_mass(int p_joint_idx) const;
	void set_jiggle_joint_damping(int p_joint_idx, float p_damping);
	float get_jiggle_joint_damping(int p_joint_idx) const;
	void set_jiggle_joint_use_gravity(int p_joint_idx, bool p_use_gravity);
	bool get_jiggle_joint_use_gravity(int p_joint_idx) const;
	void set_jiggle_joint_gravity(int p_joint_idx, Vector2 p_gravity);
	Vector2 get_jiggle_joint_gravity(int p_joint_idx) const;

	// TwoBoneIK accessors.
	void set_target_minimum_distance(float p_minimum_distance);
	float get_target_minimum_distance() const;
	void set_target_maximum_distance(float p_maximum_distance);
	float get_target_maximum_distance() const;
	void set_flip_bend_direction(bool p_flip_direction);
	bool get_flip_bend_direction() const;
	void set_joint_one_bone2d_node(const NodePath &p_node);
	NodePath get_joint_one_bone2d_node() const;
	void set_joint_one_bone_idx(int p_bone_idx);
	int get_joint_one_bone_idx() const;
	void set_joint_two_bone2d_node(const NodePath &p_node);
	NodePath get_joint_two_bone2d_node() const;
	void set_joint_two_bone_idx(int p_bone_idx);
	int get_joint_two_bone_idx() const;
#ifdef TOOLS_ENABLED
	void set_editor_draw_min_max(bool p_draw);
	bool get_editor_draw_min_max() const;
#endif // TOOLS_ENABLED

	// PhysicalBones accessors.
	int get_physical_bone_chain_length() const;
	void set_physical_bone_chain_length(int p_new_length);
	void set_physical_bone_node(int p_joint_idx, const NodePath &p_path);
	NodePath get_physical_bone_node(int p_joint_idx) const;
	void fetch_physical_bones();
	void start_simulation(const Vector<String> &p_bones = Vector<String>());
	void stop_simulation(const Vector<String> &p_bones = Vector<String>());

	// StackHolder accessors.
	void set_held_modification_stack(Ref<SkeletonModificationStack2D> p_held_stack);
	Ref<SkeletonModificationStack2D> get_held_modification_stack() const;
};

// --- End scene/resources/2d/skeleton/skeleton_modification_2d.h ---

class Skeleton2D;

class Bone2D : public Node2D
{
	friend class Skeleton2D;
#ifdef TOOLS_ENABLED
	friend class AnimatedValuesBackup;
#endif

	Bone2D* parent_bone = nullptr;
	Skeleton2D* skeleton = nullptr;
	Transform2D rest;

	bool autocalculate_length_and_angle = true;
	real_t length = 16;
	real_t bone_angle = 0;

	int skeleton_index = -1;

	void calculate_length_and_rotation();

#ifdef TOOLS_ENABLED
	RID editor_gizmo_rid;
	bool _editor_get_bone_shape(
		Vector<Vector2>* p_shape, Vector<Vector2>* p_outline_shape, Bone2D* p_other_bone);
	bool _editor_show_bone_gizmo = true;
#endif // TOOLS ENABLED

protected:
	void _notification(int p_what);

public:
	Transform2D cache_transform;
	bool copy_transform_to_cache = true;

	Transform2D get_rest() const;
	void apply_rest();
	Transform2D get_skeleton_rest() const;

	PackedStringArray get_configuration_warnings() const override;

	void set_autocalculate_length_and_angle(bool p_autocalculate);
	bool get_autocalculate_length_and_angle() const;
	void set_length(real_t p_length);
	real_t get_length() const;
	void set_bone_angle(real_t p_angle);
	real_t get_bone_angle() const;

	int get_index_in_skeleton() const;

#ifdef TOOLS_ENABLED
	void _editor_set_show_bone_gizmo(bool p_show_gizmo);
	bool _editor_get_show_bone_gizmo() const;
#endif // TOOLS_ENABLED

	Bone2D();
	~Bone2D();
};

class SkeletonModificationStack2D;

class Skeleton2D : public Node2D
{
	friend class Bone2D;
#ifdef TOOLS_ENABLED
	friend class AnimatedValuesBackup;
#endif

	struct Bone
	{
		bool operator<(const Bone& p_bone) const { return p_bone.bone->is_greater_than(bone); }

		Bone2D* bone = nullptr;
		int parent_index = 0;
		Transform2D accum_transform;
		Transform2D rest_inverse;

		// Transform2D local_pose_cache;
		Transform2D local_pose_override;
		real_t local_pose_override_amount = 0;
		bool local_pose_override_persistent = false;
	};

	LocalVector<Bone> bones;

	bool bone_setup_dirty = true;
	void _make_bone_setup_dirty();
	void _update_bone_setup();

	bool transform_dirty = true;
	void _make_transform_dirty();
	void _update_transform();

	RID skeleton;

	Ref<SkeletonModificationStack2D> modification_stack;

	///////////////////////////////////////////////////////
	// INTERPOLATION
	struct InterpolationData
	{
		Transform2D xform_curr;
		Transform2D xform_prev;
		uint32_t last_update_physics_tick = UINT32_MAX; // Ensure tick 0 is detected as a change.
	} _interpolation_data;

	void _update_process_mode();
	void _ensure_update_interpolation_data();

protected:
	virtual void _physics_interpolated_changed() override;
	///////////////////////////////////////////////////////

	void _notification(int p_what);

public:
	int get_bone_count() const;
	Bone2D* get_bone(int p_idx);

	RID get_skeleton() const;

	void set_bone_local_pose_override(
		int p_bone_idx, Transform2D p_override, real_t p_amount, bool p_persistent = true);
	Transform2D get_bone_local_pose_override(int p_bone_idx);

	Ref<SkeletonModificationStack2D> get_modification_stack() const;
	void set_modification_stack(Ref<SkeletonModificationStack2D> p_stack);
	void execute_modifications(real_t p_delta, int p_execution_mode);

	Skeleton2D();
	~Skeleton2D();
};



// --- End scene/2d/skeleton_2d.h ---
