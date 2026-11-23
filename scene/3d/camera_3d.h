/**************************************************************************/
/*  camera_3d.h                                                           */
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
// --- Folded from core/templates/interpolated_property.h ---
struct Vector2;

namespace InterpolatedPropertyFuncs {
float lerp(float p_a, float p_b, float p_fraction);
double lerp(double p_a, double p_b, float p_fraction);
Vector2 lerp(const Vector2 &p_a, const Vector2 &p_b, float p_fraction);
} //namespace InterpolatedPropertyFuncs

// This class is intended to reduce the boiler plate involved to
// support custom properties to be physics interpolated.

template <class T>
class InterpolatedProperty {
	// Only needs interpolating / updating the servers when
	// curr and prev are different.
	bool _needs_interpolating = false;

	// Each time we hit a physics tick, curr = prev,
	// BUT the server may be out of date, and needs an update
	// on the next frame, OR taking the node off the property update list.
	bool _server_stale = false;

	T _interpolated;
	T curr;
	T prev;

public:
	// FTI depends on the constant flow between current values
	// (on the current tick) and stored previous values (on the previous tick).
	// These should be updated both on each tick, and also on resets.
	void pump() {
		prev = curr;
		_needs_interpolating = false;
		_server_stale = true;
	}
	void reset() { pump(); }

	void set_interpolated_value(const T &p_val) {
		_interpolated = p_val;
	}
	const T &interpolated() const { return _interpolated; }

	// Returns true if an update to the server is needed.
	bool interpolate(float p_interpolation_fraction) {
		if (_needs_interpolating) {
			_interpolated = InterpolatedPropertyFuncs::lerp(prev, curr, p_interpolation_fraction);
			return true;
		}
		if (_server_stale) {
			_interpolated = curr;
			_server_stale = false;
			return true;
		}
		return false;
	}

	operator T() const {
		return curr;
	}

	bool operator==(const T &p_o) const {
		return p_o == curr;
	}

	bool operator!=(const T &p_o) const {
		return p_o != curr;
	}

	InterpolatedProperty &operator=(T p_val) {
		curr = p_val;
		_interpolated = p_val;
		_needs_interpolating = true;
		return *this;
	}
	InterpolatedProperty(T p_val) {
		curr = p_val;
		_interpolated = p_val;
		pump();
	}
	InterpolatedProperty() {
		// Ensure either the constructor is run,
		// or the memory is zeroed if using a fundamental type.
		_interpolated = T{};
		curr = T{};
		prev = T{};
	}
};

// --- End core/templates/interpolated_property.h ---
#include "scene/3d/node_3d.h"
// --- Folded from scene/3d/velocity_tracker_3d.h ---
#include "core/types.h"

class VelocityTracker3D : public RefCounted
{
	struct PositionHistory
	{
		uint64_t frame = 0;
		Vector3 position;
	};

	bool physics_step = false;
	Vector<PositionHistory> position_history;
	int position_history_len = 0;

public:
	void reset(const Vector3& p_new_pos);
	void set_track_physics_step(bool p_track_physics_step);
	bool is_tracking_physics_step() const;
	void update_position(const Vector3& p_position);
	Vector3 get_tracked_linear_velocity() const;

	VelocityTracker3D();
};



// --- End scene/3d/velocity_tracker_3d.h ---
#include "scene/resources/material.h"
#include "scene/resources/environment.h"
#include "servers/xr/xr_server.h"

class Camera3D : public Node3D
{
public:
	enum ProjectionType
	{
		PROJECTION_PERSPECTIVE,
		PROJECTION_ORTHOGONAL,
		PROJECTION_FRUSTUM
	};

	enum KeepAspect
	{
		KEEP_WIDTH,
		KEEP_HEIGHT
	};

	enum DopplerTracking
	{
		DOPPLER_TRACKING_DISABLED,
		DOPPLER_TRACKING_IDLE_STEP,
		DOPPLER_TRACKING_PHYSICS_STEP
	};

private:
	bool force_change = false;
	bool current = false;
	Viewport* viewport = nullptr;

	ProjectionType mode = PROJECTION_PERSPECTIVE;

	InterpolatedProperty<real_t> fov = 75.0;
	InterpolatedProperty<real_t> size = 1.0;
	InterpolatedProperty<Vector2> frustum_offset;
	// _ prefix to avoid conflict with Windows defines.
	InterpolatedProperty<real_t> _near = 0.05;
	InterpolatedProperty<real_t> _far = 4000.0;
	real_t v_offset = 0.0;
	real_t h_offset = 0.0;
	KeepAspect keep_aspect = KEEP_HEIGHT;

	RID camera;
	RID scenario_id;

	uint32_t layers = 0xfffff;

	Ref<Environment> environment;
	Ref<CameraAttributes> attributes;
	Ref<Compositor> compositor;
	void _attributes_changed();

	friend class Viewport;
	void _update_audio_listener_state();

	DopplerTracking doppler_tracking = DOPPLER_TRACKING_DISABLED;
	Ref<VelocityTracker3D> velocity_tracker;

#ifndef PHYSICS_3D_DISABLED
	RID pyramid_shape;
	Vector<Vector3> pyramid_shape_points;
#endif // PHYSICS_3D_DISABLED

	bool _desired_process_internal = false;
	bool _desired_physics_process_internal = false;

	bool xr_enabled = false;
	String tracker_name = "head";
	String pose_name = "default";
	Ref<XRTracker> tracker;

	void _bind_tracker();
	void _unbind_tracker();
	void _changed_tracker(const String& p_tracker_name, int p_tracker_type);
	void _removed_tracker(const String& p_tracker_name, int p_tracker_type);
	void _pose_changed(const Ref<XRPose>& p_pose);

	void _update_process_mode();

protected:
	void set_desired_process_modes(bool p_process_internal, bool p_physics_process_internal);

	virtual void _physics_interpolated_changed() override;
	Transform3D _get_adjusted_camera_transform(const Transform3D& p_xform) const;

	void _update_camera();
	void _request_camera_update();

	virtual void fti_pump_property() override;
	virtual void fti_update_servers_property() override;
	virtual void fti_update_servers_xform() override;

	void _notification(int p_what);

	Projection _get_camera_projection(real_t p_near) const;

public:
	enum
	{
		NOTIFICATION_BECAME_CURRENT = 50,
		NOTIFICATION_LOST_CURRENT = 51
	};

	void set_xr_enabled(bool p_enabled);
	bool is_xr_enabled() const { return xr_enabled; }

	PackedStringArray get_configuration_warnings() const override;

	void set_projection(Camera3D::ProjectionType p_mode);

	void make_current();
	void clear_current(bool p_enable_next = true);
	void set_current(bool p_enabled);
	bool is_current() const;

	RID get_camera() const;

	real_t get_fov() const;
	real_t get_size() const;
	real_t get_far() const;
	real_t get_near() const;
	Vector2 get_frustum_offset() const;

	ProjectionType get_projection() const;

	Transform3D get_camera_transform() const;
	Projection get_camera_projection() const;

	Vector3 project_ray_normal(const Point2& p_pos) const;
	Vector3 project_ray_origin(const Point2& p_pos) const;
	Vector3 project_local_ray_normal(const Point2& p_pos) const;
	Point2 unproject_position(const Vector3& p_pos) const;
	bool is_position_behind(const Vector3& p_pos) const;
	Vector3 project_position(const Point2& p_point, real_t p_z_depth) const;

	Vector<Vector3> get_near_plane_points() const;

	uint32_t get_cull_mask() const;

	bool get_cull_mask_value(int p_layer_number) const;

	Vector<Plane> get_frustum() const;
	bool is_position_in_frustum(const Vector3& p_position) const;

	Ref<Environment> get_environment() const;

	void set_attributes(const Ref<CameraAttributes>& p_effects);
	Ref<CameraAttributes> get_attributes() const;

	Ref<Compositor> get_compositor() const;

	void set_keep_aspect_mode(KeepAspect p_aspect);
	KeepAspect get_keep_aspect_mode() const;

	void set_v_offset(real_t p_offset);
	real_t get_v_offset() const;

	void set_h_offset(real_t p_offset);
	real_t get_h_offset() const;

	DopplerTracking get_doppler_tracking() const;

	Vector3 get_doppler_tracked_velocity() const;

#ifndef PHYSICS_3D_DISABLED
	RID get_pyramid_shape_rid();
#endif // PHYSICS_3D_DISABLED

	Camera3D(bool p_xr = false);
	~Camera3D();
};

// --- Folded from scene/3d/audio_listener_3d.h ---
#include "scene/3d/node_3d.h"


class AudioListener3D : public Node3D
{
public:
	enum DopplerTracking
	{
		DOPPLER_TRACKING_DISABLED,
		DOPPLER_TRACKING_IDLE_STEP,
		DOPPLER_TRACKING_PHYSICS_STEP,
	};

	void make_current();
	void clear_current();
	bool is_current() const;

	virtual Transform3D get_listener_transform() const;

	void set_doppler_tracking(DopplerTracking p_tracking);
	DopplerTracking get_doppler_tracking() const;

	Vector3 get_doppler_tracked_velocity() const;

	AudioListener3D();
	~AudioListener3D();

private:
	bool force_change = false;
	bool current = false;

	RID scenario_id;

	friend class Viewport;
	void _update_audio_listener_state();

	DopplerTracking doppler_tracking = DOPPLER_TRACKING_DISABLED;
	Ref<VelocityTracker3D> velocity_tracker;

protected:
	void _update_listener();
	virtual void _request_listener_update();

	void _notification(int p_what);

};



// --- End scene/3d/audio_listener_3d.h ---

// --- Folded from scene/3d/audio_stream_player_3d.h ---
#include "core/templates/span.h"
#include "scene/3d/node_3d.h"
#include "servers/audio/audio_server.h"

#ifndef PHYSICS_3D_DISABLED
class Area3D;
#endif // PHYSICS_3D_DISABLED
struct AudioFrame;
class AudioStream;
class AudioStreamPlayback;
class AudioStreamPlayerInternal;
class VelocityTracker3D;

class AudioStreamPlayer3D : public Node3D
{
public:
	enum AttenuationModel
	{
		ATTENUATION_INVERSE_DISTANCE,
		ATTENUATION_INVERSE_SQUARE_DISTANCE,
		ATTENUATION_LOGARITHMIC,
		ATTENUATION_DISABLED,
	};

	enum DopplerTracking
	{
		DOPPLER_TRACKING_DISABLED,
		DOPPLER_TRACKING_IDLE_STEP,
		DOPPLER_TRACKING_PHYSICS_STEP
	};

private:
	enum
	{
		MAX_OUTPUTS = 8,
		MAX_INTERSECT_AREAS = 32

	};

	static constexpr int64_t VOLUME_VECTOR_SIZE = AudioServer::MAX_CHANNELS_PER_BUS;

	AudioStreamPlayerInternal* internal = nullptr;

	SafeNumeric<float> setplay{-1.0};
	Ref<AudioStreamPlayback> setplayback;

	AttenuationModel attenuation_model = ATTENUATION_INVERSE_DISTANCE;
	float unit_size = 10.0;
	float max_db = 3.0;
	// Internally used to take doppler tracking into account.
	float actual_pitch_scale = 1.0;

	uint64_t last_mix_count = -1;
	bool force_update_panning = false;

	static void _calc_output_vol(const Vector3& source_dir, real_t tightness,
		FixedVector<AudioFrame, VOLUME_VECTOR_SIZE>& output);
	static AudioFrame _calc_output_vol_stereo(const Vector3& source_dir, real_t panning_strength);

#ifndef PHYSICS_3D_DISABLED
	void _calc_reverb_vol(Area3D* area, Vector3 listener_area_pos,
		const FixedVector<AudioFrame, VOLUME_VECTOR_SIZE>& direct_path_vol,
		FixedVector<AudioFrame, VOLUME_VECTOR_SIZE>& reverb_vol);
#endif // PHYSICS_3D_DISABLED

	static void _listener_changed_cb(void* self)
	{
		reinterpret_cast<AudioStreamPlayer3D*>(self)->force_update_panning = true;
	}

	void _set_playing(bool p_enable);
	bool _is_active() const;
	String _get_actual_bus();
#ifndef PHYSICS_3D_DISABLED
	Area3D* _get_overriding_area();
#endif // PHYSICS_3D_DISABLED
	Vector<AudioFrame> _update_panning();

	uint32_t area_mask = 0;

	AudioServer::PlaybackType playback_type = AudioServer::PlaybackType::PLAYBACK_TYPE_DEFAULT;

	bool emission_angle_enabled = false;
	float emission_angle = 45.0;
	float emission_angle_filter_attenuation_db = -12.0;
	float attenuation_filter_cutoff_hz = 5000.0;
	float attenuation_filter_db = -24.0;

	float linear_attenuation = 0;

	float max_distance = 0.0;
	bool was_further_than_max_distance_last_frame = false;

	Ref<VelocityTracker3D> velocity_tracker;

	DopplerTracking doppler_tracking = DOPPLER_TRACKING_DISABLED;

	float _get_attenuation_db(float p_distance) const;

	float panning_strength = 1.0f;
	float cached_global_panning_strength = 0.5f;

	/// Hash map storing the bus volumes during the panning update.
	/// This hash map is stored as member for efficiency reasons.
	HashMap<String, Vector<AudioFrame>> bus_volumes;

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

	float get_unit_size() const;

	void set_max_db(float p_boost);
	float get_max_db() const;

	void set_pitch_scale(float p_pitch_scale);
	float get_pitch_scale() const;

	void play(float p_from_pos = 0.0);
	void seek(float p_seconds);
	void stop();
	bool is_playing() const;
	float get_playback_position();

	void set_bus(const String& p_bus);
	String get_bus() const;

	void set_max_polyphony(int p_max_polyphony);
	int get_max_polyphony() const;

	void set_autoplay(bool p_enable);
	bool is_autoplay_enabled() const;

	float get_max_distance() const;

	void set_area_mask(uint32_t p_mask);
	uint32_t get_area_mask() const;

	bool is_emission_angle_enabled() const;

	float get_emission_angle() const;

	void set_emission_angle_filter_attenuation_db(float p_angle_attenuation_db);
	float get_emission_angle_filter_attenuation_db() const;

	void set_attenuation_filter_cutoff_hz(float p_hz);
	float get_attenuation_filter_cutoff_hz() const;

	void set_attenuation_filter_db(float p_db);
	float get_attenuation_filter_db() const;

	AttenuationModel get_attenuation_model() const;

	void set_doppler_tracking(DopplerTracking p_tracking);
	DopplerTracking get_doppler_tracking() const;

	void set_stream_paused(bool p_pause);
	bool get_stream_paused() const;

	void set_panning_strength(float p_panning_strength);
	float get_panning_strength() const;

	bool has_stream_playback();
	Ref<AudioStreamPlayback> get_stream_playback();

	AudioServer::PlaybackType get_playback_type() const;
	void set_playback_type(AudioServer::PlaybackType p_playback_type);

	AudioStreamPlayer3D();
	~AudioStreamPlayer3D();
};



// --- End scene/3d/audio_stream_player_3d.h ---

// --- Folded from scene/3d/world_environment.h ---
#include "scene/main/node.h"
#include "scene/resources/material.h"
#include "scene/resources/environment.h"

class WorldEnvironment : public Node
{
	Ref<Environment> environment;
	Ref<CameraAttributes> camera_attributes;
	Ref<Compositor> compositor;

	void _update_current_environment();
	void _update_current_camera_attributes();
	void _update_current_compositor();

protected:
	void _notification(int p_what);

public:
	Ref<Environment> get_environment() const;

	Ref<CameraAttributes> get_camera_attributes() const;

	Ref<Compositor> get_compositor() const;

	PackedStringArray get_configuration_warnings() const override;

	WorldEnvironment();
};



// --- End scene/3d/world_environment.h ---
