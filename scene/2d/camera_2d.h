/**************************************************************************/
/*  camera_2d.h                                                           */
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
#include "scene/2d/node_2d.h"

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
#include "scene/2d/node_2d.h"
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
#include "scene/2d/node_2d.h"

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
#include "scene/2d/node_2d.h"

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
