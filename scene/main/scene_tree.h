/**************************************************************************/
/*  scene_tree.h                                                          */
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

#include <cstdlib>
#include "core/os/os.h"
#include "core/types.h"
// --- Folded from scene/animation/animation_mixer.h ---
#include "core/types.h"
// --- Inlined/Folded from scene/animation/tween.h ---
#include "core/types.h"

class Tweener;
class Node;
class SceneTree;

class Tween final : public RefCounted
{
	friend class Tweener;

public:
	enum TweenProcessMode
	{
		TWEEN_PROCESS_PHYSICS,
		TWEEN_PROCESS_IDLE,
	};

	enum TweenPauseMode
	{
		TWEEN_PAUSE_BOUND,
		TWEEN_PAUSE_STOP,
		TWEEN_PAUSE_PROCESS,
	};

	enum TransitionType
	{
		TRANS_LINEAR,
		TRANS_SINE,
		TRANS_QUINT,
		TRANS_QUART,
		TRANS_QUAD,
		TRANS_EXPO,
		TRANS_ELASTIC,
		TRANS_CUBIC,
		TRANS_CIRC,
		TRANS_BOUNCE,
		TRANS_BACK,
		TRANS_SPRING,
		TRANS_MAX
	};

	enum EaseType
	{
		EASE_IN,
		EASE_OUT,
		EASE_IN_OUT,
		EASE_OUT_IN,
		EASE_MAX
	};

private:
	TweenProcessMode process_mode = TweenProcessMode::TWEEN_PROCESS_IDLE;
	TweenPauseMode pause_mode = TweenPauseMode::TWEEN_PAUSE_BOUND;
	TransitionType default_transition = TransitionType::TRANS_LINEAR;
	EaseType default_ease = EaseType::EASE_IN_OUT;

	bool is_in_tree = false;
	LocalVector<List<Ref<Tweener>>> tweeners;
	LocalVector<Ref<Tween>> subtweens;
	double total_time = 0;
	int current_step = -1;
	int loops = 1;
	int loops_done = 0;
	float speed_scale = 1;
	bool ignore_time_scale = false;

	Node* bound_node = nullptr;
	bool is_bound = false;
	bool started = false;
	bool running = true;
	bool in_step = false;
	bool dead = false;
	bool valid = false;
	bool default_parallel = false;
	bool parallel_enabled = false;
#ifdef DEBUG_ENABLED
	bool is_infinite = false;
#endif

	typedef real_t (*interpolater)(real_t t, real_t b, real_t c, real_t d);
	static interpolater interpolaters[TRANS_MAX][EASE_MAX];

	void _start_tweeners();
	void _stop_internal(bool p_reset);

protected:
	String _to_string();

public:
	Ref<Tweener> tween_interval(double p_time);
	Ref<Tweener> tween_subtween(Tween* rp_subtween);
	Ref<Tweener> tween_callback(void (*p_callback)() = nullptr);
	Ref<Tweener> tween_property(const Vector<String>& p_property, double p_duration);
	Ref<Tweener> tween_method(double p_duration);
	void append(Ref<Tweener> p_tweener);

	bool custom_step(double p_delta);
	void stop();
	void pause();
	void play();
	void kill();

	bool has_tweeners() const;
	bool is_running();
	bool is_valid();
	void clear();

	Tween* bind_node(const Node* rp_node);
	Tween* set_process_mode(TweenProcessMode p_mode);
	TweenProcessMode get_process_mode() const;
	Tween* set_pause_mode(TweenPauseMode p_mode);
	TweenPauseMode get_pause_mode() const;
	Tween* set_ignore_time_scale(bool p_ignore = true);
	bool is_ignoring_time_scale() const;

	Tween* set_parallel(bool p_parallel);
	Tween* set_loops(int p_loops);
	int get_loops_left() const;
	Tween* set_speed_scale(float p_speed);
	Tween* set_trans(TransitionType p_trans);
	TransitionType get_trans() const;
	Tween* set_ease(EaseType p_ease);
	EaseType get_ease() const;

	Tween* parallel();
	Tween* chain();

	static real_t run_equation(
		TransitionType p_trans_type, EaseType p_ease_type, real_t t, real_t b, real_t c, real_t d);

	bool step(double p_delta);
	bool can_process(bool p_tree_paused) const;
	Node* get_bound_node() const;
	double get_total_time() const;

	Tween();
	explicit Tween(bool p_in_tree);
	~Tween();
};

class Tweener final : public RefCounted
{
	friend class Tween;

public:
	enum Type
	{
		TWEENER_INTERVAL,
		TWEENER_CALLBACK,
		TWEENER_SUBTWEEN,
		TWEENER_PROPERTY,
		TWEENER_METHOD,
		TWEENER_AWAIT,
	};

	Type get_type() const { return type; }

	Ref<Tweener> from(double p_value);
	Ref<Tweener> from_current();
	Ref<Tweener> as_relative();
	Ref<Tweener> set_trans(Tween::TransitionType p_trans);
	Ref<Tweener> set_ease(Tween::EaseType p_ease);
	Ref<Tweener> set_custom_interpolator(real_t (*p_interpolator)(real_t));
	Ref<Tweener> set_delay(double p_delay);
	Ref<Tweener> set_timeout(double p_timeout);

	void set_tween(const Ref<Tween>& p_tween);
	void start();
	bool step(double& r_delta);

	Tweener();
	explicit Tweener(Type p_type);
	Tweener(Type p_type, double p_duration);
	explicit Tweener(const Ref<Tween>& p_subtween);
	~Tweener() = default;

	Ref<Tween> subtween;

private:
	Ref<Tween> _get_tween();
	void _finish();

	Type type = TWEENER_INTERVAL;
	Tween* tween = nullptr;
	Ref<RefCounted> ref_copy; // Makes sure that RefCounted objects are not freed too early.
	Vector<String> property;
	real_t (*custom_interpolator)(real_t) = nullptr;
	void (*callback)() = nullptr;

	double elapsed_time = 0.0;
	double duration = 0.0;
	double delay = 0.0;
	double timeout = -1.0;
	double initial_val = 0.0;
	Tween::TransitionType trans_type = Tween::TRANS_MAX;
	Tween::EaseType ease_type = Tween::EASE_MAX;

	bool finished = false;
	bool do_continue = true;
	bool do_continue_delayed = false;
	bool relative = false;
	bool received = false;
};
#include "scene/main/node.h"
// --- Folded from scene/resources/animation.h ---
#include <memory>
#include "core/io/resource.h"
#include "core/templates/local_vector.h"

#define ANIM_MIN_LENGTH 0.001

class Animation : public Resource
{
public:
	typedef uint64_t TrackCacheID;

	static inline String PARAMETERS_BASE_PATH = "parameters/";
	static constexpr real_t DEFAULT_STEP = 1.0 / 30;

	enum TrackType : uint8_t
	{
		TYPE_VALUE,		  // Set a value in a property, can be interpolated.
		TYPE_POSITION_3D, // Position 3D track, can be compressed.
		TYPE_ROTATION_3D, // Rotation 3D track, can be compressed.
		TYPE_SCALE_3D,	  // Scale 3D track, can be compressed.
		TYPE_BLEND_SHAPE, // Blend Shape track, can be compressed.
		TYPE_METHOD,	  // Call any method on a specific node.
		TYPE_BEZIER,	  // Bezier curve.
		TYPE_AUDIO,
		TYPE_ANIMATION,
	};

	enum InterpolationType : uint8_t
	{
		INTERPOLATION_NEAREST,
		INTERPOLATION_LINEAR,
		INTERPOLATION_CUBIC,
		INTERPOLATION_LINEAR_ANGLE,
		INTERPOLATION_CUBIC_ANGLE,
	};

	enum UpdateMode : uint8_t
	{
		UPDATE_CONTINUOUS,
		UPDATE_DISCRETE,
		UPDATE_CAPTURE,
	};

	enum LoopMode : uint8_t
	{
		LOOP_NONE,
		LOOP_LINEAR,
		LOOP_PINGPONG,
	};

	// LoopedFlag is used in Animataion to "process the keys at both ends correct".
	enum LoopedFlag : uint8_t
	{
		LOOPED_FLAG_NONE,
		LOOPED_FLAG_END,
		LOOPED_FLAG_START,
	};

	enum FindMode : uint8_t
	{
		FIND_MODE_NEAREST,
		FIND_MODE_APPROX,
		FIND_MODE_EXACT,
	};

#ifdef TOOLS_ENABLED
	enum HandleMode
	{
		HANDLE_MODE_FREE,
		HANDLE_MODE_LINEAR,
		HANDLE_MODE_BALANCED,
		HANDLE_MODE_MIRRORED,
	};

	enum HandleSetMode
	{
		HANDLE_SET_MODE_NONE,
		HANDLE_SET_MODE_RESET,
		HANDLE_SET_MODE_AUTO,
	};
#endif // TOOLS_ENABLED

	template <typename T> struct TKey
	{
		real_t transition = 1.0;
		double time = 0.0; // Time in secs.
		T value;
	};

	struct MethodKey
	{
		real_t transition = 1.0;
		double time = 0.0; // Time in secs.
		String method;
	};

	struct BezierKey
	{
		Vector2 in_handle;	// Relative (x always <0)
		Vector2 out_handle; // Relative (x always >0)
		real_t value = 0.0;
#ifdef TOOLS_ENABLED
		HandleMode handle_mode = HANDLE_MODE_FREE;
#endif // TOOLS_ENABLED
	};

	struct AudioKey
	{
		Ref<Resource> stream;
		real_t start_offset = 0.0; // Offset from start.
		real_t end_offset = 0.0;   // Offset from end, if 0 then full length or infinite.

		AudioKey() = default;
	};

	struct Track
	{
		TrackType type = TrackType::TYPE_ANIMATION;
		InterpolationType interpolation = INTERPOLATION_LINEAR;
		UpdateMode update_mode = UPDATE_CONTINUOUS;
		bool loop_wrap = true;
		NodePath path; // Path to something.
		String concatenated_path;
		bool imported = false;
		bool enabled = true;
		bool use_blend = true;
		int32_t compressed_track = -1;

		LocalVector<TKey<Vector3>> positions;
		LocalVector<TKey<Quaternion>> rotations;
		LocalVector<TKey<Vector3>> scales;
		LocalVector<TKey<float>> blend_shapes;
		LocalVector<MethodKey> methods;
		LocalVector<TKey<BezierKey>> bezier_values;
		LocalVector<TKey<AudioKey>> audio_values;
		LocalVector<TKey<String>> animation_values;

		TrackCacheID get_unique_id() const
		{
			uint64_t h = concatenated_path.hash();
			return (TrackCacheID)((h << 8) | (uint8_t)type);
		}

		Track() = default;
		explicit Track(TrackType p_type) : type(p_type) {}
		~Track() = default;
	};

private:
	const int32_t POSITION_TRACK_SIZE = 5;
	const int32_t ROTATION_TRACK_SIZE = 6;
	const int32_t SCALE_TRACK_SIZE = 5;
	const int32_t BLEND_SHAPE_TRACK_SIZE = 3;

	/* Marker */

	struct MarkerKey
	{
		double time;
		String name;

		MarkerKey(double p_time, const String& p_name) : time(p_time), name(p_name) {}

		MarkerKey() = default;
	};

	LocalVector<MarkerKey> marker_names;  // time -> name
	HashMap<String, double> marker_times; // name -> time
	HashMap<String, Color> marker_colors; // name -> color

	LocalVector<Track*> tracks;
#ifdef TOOLS_ENABLED
	HashSet<String> folded_groups;
	bool folded_groups_dirty =
		false; // Set when changing folding state to access .cfg to store the state.
#endif		   // TOOLS_ENABLED

	template <typename T, typename V> int _insert(double p_time, T& p_keys, const V& p_value);

	int _marker_insert(double p_time, LocalVector<MarkerKey>& p_keys, const MarkerKey& p_value);

	template <typename K>
	inline int _find(const LocalVector<K>& p_keys, double p_time, bool p_backward = false,
		bool p_limit = false) const;

	_FORCE_INLINE_ Vector3 _interpolate(const Vector3& p_a, const Vector3& p_b, real_t p_c) const;
	_FORCE_INLINE_ Quaternion _interpolate(
		const Quaternion& p_a, const Quaternion& p_b, real_t p_c) const;

	_FORCE_INLINE_ Vector3 _cubic_interpolate_in_time(const Vector3& p_pre_a, const Vector3& p_a,
		const Vector3& p_b, const Vector3& p_post_b, real_t p_c, real_t p_pre_a_t, real_t p_b_t,
		real_t p_post_b_t) const;
	_FORCE_INLINE_ Quaternion _cubic_interpolate_in_time(const Quaternion& p_pre_a,
		const Quaternion& p_a, const Quaternion& p_b, const Quaternion& p_post_b, real_t p_c,
		real_t p_pre_a_t, real_t p_b_t, real_t p_post_b_t) const;
	_FORCE_INLINE_ real_t _cubic_interpolate_in_time(const real_t& p_pre_a, const real_t& p_a,
		const real_t& p_b, const real_t& p_post_b, real_t p_c, real_t p_pre_a_t, real_t p_b_t,
		real_t p_post_b_t) const;

	template <typename T>
	_FORCE_INLINE_ T _interpolate(const LocalVector<TKey<T>>& p_keys, double p_time,
		InterpolationType p_interp, bool p_loop_wrap, bool* p_ok, bool p_backward = false) const;

	template <typename T>
	_FORCE_INLINE_ void _track_get_key_indices_in_range(const LocalVector<T>& p_array,
		double from_time, double to_time, LocalVector<int>* r_indices, bool p_is_backward) const;

	double length = 1.0;
	real_t step = DEFAULT_STEP;
	LoopMode loop_mode = LOOP_NONE;
	bool capture_included = false;
	void _check_capture_included();

	struct Compression
	{
		enum
		{
			MAX_DATA_TRACK_SIZE = 16384,
			BLEND_SHAPE_RANGE = 8, // -8.0 to 8.0.
			FORMAT_VERSION = 1
		};

		struct Page
		{
			Vector<uint8_t> data;
			double time_offset;
		};

		uint32_t fps = 120;
		LocalVector<Page> pages;
		LocalVector<AABB> bounds; // Used by position and scale tracks (which contain index to track
								  // and index to bounds).
		bool enabled = false;
	} compression;

	Vector3i _compress_key(
		uint32_t p_track, const AABB& p_bounds, int32_t p_key = -1, float p_time = 0.0);
	bool _rotation_interpolate_compressed(
		uint32_t p_compressed_track, double p_time, Quaternion& r_ret) const;
	bool _pos_scale_interpolate_compressed(
		uint32_t p_compressed_track, double p_time, Vector3& r_ret) const;
	bool _blend_shape_interpolate_compressed(
		uint32_t p_compressed_track, double p_time, float& r_ret) const;
	template <uint32_t COMPONENTS>
	bool _fetch_compressed(uint32_t p_compressed_track, double p_time, Vector3i& r_current_value,
		double& r_current_time, Vector3i& r_next_value, double& r_next_time,
		uint32_t* key_index = nullptr) const;
	template <uint32_t COMPONENTS>
	bool _fetch_compressed_by_index(
		uint32_t p_compressed_track, int p_index, Vector3i& r_value, double& r_time) const;
	int _get_compressed_key_count(uint32_t p_compressed_track) const;
	template <uint32_t COMPONENTS>
	void _get_compressed_key_indices_in_range(uint32_t p_compressed_track, double p_time,
		double p_delta, LocalVector<int>* r_indices) const;
	_FORCE_INLINE_ Quaternion _uncompress_quaternion(const Vector3i& p_value) const;
	_FORCE_INLINE_ Vector3 _uncompress_pos_scale(
		uint32_t p_compressed_track, const Vector3i& p_value) const;
	_FORCE_INLINE_ float _uncompress_blend_shape(const Vector3i& p_value) const;

private:
	bool _float_track_optimize_key(const TKey<float> t0, const TKey<float> t1, const TKey<float> t2,
		real_t p_allowed_velocity_err, real_t p_allowed_precision_error, bool p_is_nearest);
	bool _vector2_track_optimize_key(const TKey<Vector2> t0, const TKey<Vector2> t1,
		const TKey<Vector2> t2, real_t p_allowed_velocity_err, real_t p_allowed_angular_error,
		real_t p_allowed_precision_error, bool p_is_nearest);
	bool _vector3_track_optimize_key(const TKey<Vector3> t0, const TKey<Vector3> t1,
		const TKey<Vector3> t2, real_t p_allowed_velocity_err, real_t p_allowed_angular_error,
		real_t p_allowed_precision_error, bool p_is_nearest);
	bool _quaternion_track_optimize_key(const TKey<Quaternion> t0, const TKey<Quaternion> t1,
		const TKey<Quaternion> t2, real_t p_allowed_velocity_err, real_t p_allowed_angular_error,
		real_t p_allowed_precision_error, bool p_is_nearest);

	void _position_track_optimize(int p_idx, real_t p_allowed_velocity_err,
		real_t p_allowed_angular_err, real_t p_allowed_precision_error);
	void _rotation_track_optimize(int p_idx, real_t p_allowed_velocity_err,
		real_t p_allowed_angular_error, real_t p_allowed_precision_error);
	void _scale_track_optimize(int p_idx, real_t p_allowed_velocity_err,
		real_t p_allowed_angular_err, real_t p_allowed_precision_error);
	void _blend_shape_track_optimize(
		int p_idx, real_t p_allowed_velocity_err, real_t p_allowed_precision_error);
	void _value_track_optimize(int p_idx, real_t p_allowed_velocity_err,
		real_t p_allowed_angular_err, real_t p_allowed_precision_error);

protected:
	virtual void reset_state() override;

	static bool inform_variant_array(
		int& r_min, int& r_max); // Returns true if max and min are swapped.

#ifndef DISABLE_DEPRECATED
	Vector3 _position_track_interpolate_bind_compat_86629(int p_track, double p_time) const;
	Quaternion _rotation_track_interpolate_bind_compat_86629(int p_track, double p_time) const;
	Vector3 _scale_track_interpolate_bind_compat_86629(int p_track, double p_time) const;
	float _blend_shape_track_interpolate_bind_compat_86629(int p_track, double p_time) const;
	int _track_find_key_bind_compat_92861(
		int p_track, double p_time, FindMode p_find_mode = FIND_MODE_NEAREST) const;
	static void _bind_compatibility_methods();
#endif // DISABLE_DEPRECATED

public:
	int add_track(TrackType p_type, int p_at_pos = -1);
	void remove_track(int p_track);

	_FORCE_INLINE_ const LocalVector<Track*>& get_tracks() { return tracks; }

	bool is_capture_included() const;

	int get_track_count() const;
	TrackType track_get_type(int p_track) const;

	void track_set_path(int p_track, const NodePath& p_path);
	NodePath track_get_path(int p_track) const;
	int find_track(const NodePath& p_path, const TrackType p_type) const;

	TrackCacheID track_get_unique_id(int p_track) const;

	void track_move_up(int p_track);
	void track_move_down(int p_track);
	void track_move_to(int p_track, int p_to_index);
	void track_swap(int p_track, int p_with_track);

	void track_set_imported(int p_track, bool p_imported);
	bool track_is_imported(int p_track) const;

	void track_set_enabled(int p_track, bool p_enabled);
	bool track_is_enabled(int p_track) const;

	void track_set_key_transition(int p_track, int p_key_idx, real_t p_transition);
	void track_set_key_time(int p_track, int p_key_idx, double p_time);
	int track_find_key(int p_track, double p_time, FindMode p_find_mode = FIND_MODE_NEAREST,
		bool p_limit = false, bool p_backward = false) const;
	void track_remove_key(int p_track, int p_idx);
	void track_remove_key_at_time(int p_track, double p_time);
	int track_get_key_count(int p_track) const;
	double track_get_key_time(int p_track, int p_key_idx) const;
	real_t track_get_key_transition(int p_track, int p_key_idx) const;
	bool track_is_compressed(int p_track) const;

	int position_track_insert_key(int p_track, double p_time, const Vector3& p_position);
	Error position_track_get_key(int p_track, int p_key, Vector3* r_position) const;
	Error try_position_track_interpolate(
		int p_track, double p_time, Vector3* r_interpolation, bool p_backward = false) const;
	Vector3 position_track_interpolate(int p_track, double p_time, bool p_backward = false) const;

	int rotation_track_insert_key(int p_track, double p_time, const Quaternion& p_rotation);
	Error rotation_track_get_key(int p_track, int p_key, Quaternion* r_rotation) const;
	Error try_rotation_track_interpolate(
		int p_track, double p_time, Quaternion* r_interpolation, bool p_backward = false) const;
	Quaternion rotation_track_interpolate(
		int p_track, double p_time, bool p_backward = false) const;

	int scale_track_insert_key(int p_track, double p_time, const Vector3& p_scale);
	Error scale_track_get_key(int p_track, int p_key, Vector3* r_scale) const;
	Error try_scale_track_interpolate(
		int p_track, double p_time, Vector3* r_interpolation, bool p_backward = false) const;
	Vector3 scale_track_interpolate(int p_track, double p_time, bool p_backward = false) const;

	int blend_shape_track_insert_key(int p_track, double p_time, float p_blend);
	Error blend_shape_track_get_key(int p_track, int p_key, float* r_blend) const;
	Error try_blend_shape_track_interpolate(
		int p_track, double p_time, float* r_blend, bool p_backward = false) const;
	float blend_shape_track_interpolate(int p_track, double p_time, bool p_backward = false) const;

	void track_set_interpolation_type(int p_track, InterpolationType p_interp);
	InterpolationType track_get_interpolation_type(int p_track) const;

	int bezier_track_insert_key(int p_track, double p_time, real_t p_value,
		const Vector2& p_in_handle, const Vector2& p_out_handle);
	void bezier_track_set_key_value(int p_track, int p_index, real_t p_value);
	void bezier_track_set_key_in_handle(int p_track, int p_index, const Vector2& p_handle,
		real_t p_balanced_value_time_ratio = 1.0);
	void bezier_track_set_key_out_handle(int p_track, int p_index, const Vector2& p_handle,
		real_t p_balanced_value_time_ratio = 1.0);
	real_t bezier_track_get_key_value(int p_track, int p_index) const;
	Vector2 bezier_track_get_key_in_handle(int p_track, int p_index) const;
	Vector2 bezier_track_get_key_out_handle(int p_track, int p_index) const;
#ifdef TOOLS_ENABLED
	void bezier_track_set_key_handle_mode(int p_track, int p_index, HandleMode p_mode,
		HandleSetMode p_set_mode = HANDLE_SET_MODE_NONE);
	HandleMode bezier_track_get_key_handle_mode(int p_track, int p_index) const;
	bool bezier_track_calculate_handles(int p_track, int p_index, HandleMode p_mode,
		HandleSetMode p_set_mode, Vector2* r_in_handle, Vector2* r_out_handle);
	bool bezier_track_calculate_handles(float p_time, float p_prev_time, float p_prev_value,
		float p_next_time, float p_next_value, HandleMode p_mode, HandleSetMode p_set_mode,
		Vector2* r_in_handle, Vector2* r_out_handle);
#endif // TOOLS_ENABLED

	real_t bezier_track_interpolate(int p_track, double p_time) const;

	int audio_track_insert_key(int p_track, double p_time, const Ref<Resource>& p_stream,
		real_t p_start_offset = 0, real_t p_end_offset = 0);
	void audio_track_set_key_stream(int p_track, int p_key, const Ref<Resource>& p_stream);
	void audio_track_set_key_start_offset(int p_track, int p_key, real_t p_offset);
	void audio_track_set_key_end_offset(int p_track, int p_key, real_t p_offset);
	Ref<Resource> audio_track_get_key_stream(int p_track, int p_key) const;
	real_t audio_track_get_key_start_offset(int p_track, int p_key) const;
	real_t audio_track_get_key_end_offset(int p_track, int p_key) const;
	void audio_track_set_use_blend(int p_track, bool p_enable);
	bool audio_track_is_use_blend(int p_track) const;

	int animation_track_insert_key(int p_track, double p_time, const String& p_animation);
	void animation_track_set_key_animation(int p_track, int p_key, const String& p_animation);
	String animation_track_get_key_animation(int p_track, int p_key) const;

	void track_set_interpolation_loop_wrap(int p_track, bool p_enable);
	bool track_get_interpolation_loop_wrap(int p_track) const;

	void value_track_set_update_mode(int p_track, UpdateMode p_mode);
	UpdateMode value_track_get_update_mode(int p_track) const;

	String method_track_get_name(int p_track, int p_key_idx) const;

	void track_get_key_indices_in_range(int p_track, double p_time, double p_delta, double p_start,
		double p_end, LocalVector<int>* r_indices,
		Animation::LoopedFlag p_looped_flag = Animation::LOOPED_FLAG_NONE) const;

	void add_marker(const String& p_name, double p_time);
	void remove_marker(const String& p_name);
	bool has_marker(const String& p_name) const;
	String get_marker_at_time(double p_time) const;
	String get_next_marker(double p_time) const;
	String get_prev_marker(double p_time) const;
	double get_marker_time(const String& p_time) const;
	PackedStringArray get_marker_names() const;
	Color get_marker_color(const String& p_name) const;
	void set_marker_color(const String& p_name, const Color& p_color);

	void set_length(double p_length);

	_FORCE_INLINE_ double get_length() const { return length; }

	void set_loop_mode(LoopMode p_loop_mode);

	_FORCE_INLINE_ LoopMode get_loop_mode() const { return loop_mode; }

	void set_step(real_t p_step);
	real_t get_step() const;

	void clear();

	void optimize(real_t p_allowed_velocity_err = 0.01, real_t p_allowed_angular_err = 0.01,
		int p_precision = 3);
	void compress(uint32_t p_page_size = 8192, uint32_t p_fps = 120,
		float p_split_tolerance =
			4.0); // 4.0 seems to be the split tolerance sweet spot from many tests.

#ifdef TOOLS_ENABLED
	const HashSet<String>& editor_get_folded_groups() const { return folded_groups; }

	void editor_clear_folded_groups() { folded_groups.clear(); }

	void editor_add_folded_group(const String& p_group_name) { folded_groups.insert(p_group_name); }

	void editor_remove_folded_group(const String& p_group_name)
	{
		folded_groups.erase(p_group_name);
	}

	bool editor_is_group_folded(const String& p_group_name) const
	{
		return folded_groups.has(p_group_name);
	}

	void editor_set_group_folded(const String& p_group_name, bool p_folded)
	{
		if (editor_is_group_folded(p_group_name) == p_folded) {
			return; // Do nothing.
		}
		if (p_folded) {
			editor_add_folded_group(p_group_name);
		}
		else {
			editor_remove_folded_group(p_group_name);
		}
		folded_groups_dirty = true;
	}

	bool editor_is_folding_dirty() const { return folded_groups_dirty; }

	void editor_set_folding_dirty(bool p_dirty) { folded_groups_dirty = p_dirty; }
#endif // TOOLS_ENABLED

	// Helper functions for Rotation.
	static double interpolate_via_rest(double p_from, double p_to, double p_weight,
		double p_rest = 0.0); // Deterministic slerp to prevent to cross the inverted rest axis.
	static Quaternion interpolate_via_rest(const Quaternion& p_from, const Quaternion& p_to,
		real_t p_weight,
		const Quaternion& p_rest =
			Quaternion()); // Deterministic slerp to prevent to cross the inverted rest axis.

	// Helper functions for Variant.
	static bool is_less_or_equal_approx(double a, double b)
	{
		return a < b || Math::is_equal_approx(a, b);
	}

	static bool is_less_approx(double a, double b) { return a < b && !Math::is_equal_approx(a, b); }

	static bool is_greater_or_equal_approx(double a, double b)
	{
		return a > b || Math::is_equal_approx(a, b);
	}

	static bool is_greater_approx(double a, double b)
	{
		return a > b && !Math::is_equal_approx(a, b);
	}

	static TrackType get_cache_type(TrackType p_type);

	Animation() = default;
	~Animation();
};

// --- Folded from scene/resources/skeleton_profile.h ---
#include "scene/resources/texture.h"

class SkeletonProfile : public Resource
{
public:
	enum TailDirection
	{
		TAIL_DIRECTION_AVERAGE_CHILDREN,
		TAIL_DIRECTION_SPECIFIC_CHILD,
		TAIL_DIRECTION_END
	};

protected:
	bool is_read_only = false;

	struct SkeletonProfileGroup
	{
		String group_name;
		Ref<Texture2D> texture;
	};

	struct SkeletonProfileBone
	{
		String bone_name;
		String bone_parent;
		TailDirection tail_direction = TAIL_DIRECTION_AVERAGE_CHILDREN;
		String bone_tail;
		Transform3D reference_pose;
		Vector2 handle_offset;
		String group;
		bool required = false;
	};

	String root_bone;
	String scale_base_bone;

	Vector<SkeletonProfileGroup> groups;
	Vector<SkeletonProfileBone> bones;

public:
	String get_root_bone();
	void set_root_bone(const String& p_bone_name);

	String get_scale_base_bone();
	void set_scale_base_bone(const String& p_bone_name);

	int get_group_size();
	void set_group_size(int p_size);

	String get_group_name(int p_group_idx) const;
	void set_group_name(int p_group_idx, const String& p_group_name);

	Ref<Texture2D> get_texture(int p_group_idx) const;
	void set_texture(int p_group_idx, const Ref<Texture2D>& p_texture);

	int get_bone_size();
	void set_bone_size(int p_size);

	int find_bone(const String& p_bone_name) const;

	PackedStringArray get_bone_names();
	String get_bone_name(int p_bone_idx) const;
	void set_bone_name(int p_bone_idx, const String& p_bone_name);

	String get_bone_parent(int p_bone_idx) const;
	void set_bone_parent(int p_bone_idx, const String& p_bone_parent);

	TailDirection get_tail_direction(int p_bone_idx) const;
	void set_tail_direction(int p_bone_idx, TailDirection p_tail_direction);

	String get_bone_tail(int p_bone_idx) const;
	void set_bone_tail(int p_bone_idx, const String& p_bone_tail);

	Transform3D get_reference_pose(int p_bone_idx) const;
	void set_reference_pose(int p_bone_idx, const Transform3D& p_reference_pose);

	Vector2 get_handle_offset(int p_bone_idx) const;
	void set_handle_offset(int p_bone_idx, const Vector2& p_handle_offset);

	String get_group(int p_bone_idx) const;
	void set_group(int p_bone_idx, const String& p_group);

	bool is_required(int p_bone_idx) const;
	void set_required(int p_bone_idx, bool p_required);

	bool has_bone(const String& p_bone_name);
	bool is_humanoid() const { return is_read_only; }

	static Ref<SkeletonProfile> create_humanoid();

	SkeletonProfile(bool p_humanoid = false);
	~SkeletonProfile();
};



// --- End scene/resources/skeleton_profile.h ---

// --- Folded from scene/resources/bone_map.h ---


class BoneMap : public Resource
{
	Ref<SkeletonProfile> profile;
	HashMap<String, String> bone_map;

	void _update_profile();
	void _validate_bone_map();

public:
	Ref<SkeletonProfile> get_profile() const;
	void set_profile(const Ref<SkeletonProfile>& p_profile);

	int get_skeleton_bone_name_count(const String& p_skeleton_bone_name) const;

	String get_skeleton_bone_name(const String& p_profile_bone_name) const;
	void set_skeleton_bone_name(
		const String& p_profile_bone_name, const String& p_skeleton_bone_name);
	void _set_skeleton_bone_name(const String& p_profile_bone_name,
		const String& p_skeleton_bone_name); // Avoid to emit signal for editor.

	String find_profile_bone_name(const String& p_skeleton_bone_name) const;

	BoneMap();
	~BoneMap();
};



// --- End scene/resources/bone_map.h ---
// --- End scene/resources/animation.h ---
// --- Folded from scene/resources/animation_library.h ---
#include "core/templates/hash_map.h"


class AnimationLibrary : public Resource
{
	void _animation_changed(const String& p_name);

	friend class AnimationMixer; // For faster access.
	RBMap<String, Ref<Animation>> animations;

public:
	static bool is_valid_animation_name(const String& p_name);
	static bool is_valid_library_name(const String& p_name);
	static String validate_library_name(const String& p_name);

	Error add_animation(const String& p_name, const Ref<Animation>& p_animation);
	void remove_animation(const String& p_name);
	void rename_animation(const String& p_name, const String& p_new_name);
	bool has_animation(const String& p_name) const;
	Ref<Animation> get_animation(const String& p_name) const;
	void get_animation_list(LocalVector<String>* p_animations) const;
	int get_animation_list_size() const;

#ifdef TOOLS_ENABLED
	virtual void get_argument_options(
		const String& p_function, int p_idx, List<String>* r_options) const;
#endif

	AnimationLibrary();
};



// --- End scene/resources/animation_library.h ---
#include "servers/display/display_server.h"

class AnimatedValuesBackup;

class AnimationMixer : public Node
{
	friend AnimatedValuesBackup;
#ifdef TOOLS_ENABLED
	bool editing = false;
	bool dummy = false;
#endif // TOOLS_ENABLED

	bool reset_on_save = true;
	bool is_VLTRVIRTUAL_CALL_post_process_key_value = true;

public:
	enum AnimationCallbackModeProcess
	{
		ANIMATION_CALLBACK_MODE_PROCESS_PHYSICS,
		ANIMATION_CALLBACK_MODE_PROCESS_IDLE,
		ANIMATION_CALLBACK_MODE_PROCESS_MANUAL,
	};

	enum AnimationCallbackModeMethod
	{
		ANIMATION_CALLBACK_MODE_METHOD_DEFERRED,
		ANIMATION_CALLBACK_MODE_METHOD_IMMEDIATE,
	};

	enum AnimationCallbackModeDiscrete
	{
		ANIMATION_CALLBACK_MODE_DISCRETE_DOMINANT,
		ANIMATION_CALLBACK_MODE_DISCRETE_RECESSIVE,
		ANIMATION_CALLBACK_MODE_DISCRETE_FORCE_CONTINUOUS,
	};

	/* ---- Data ---- */
	struct AnimationLibraryData
	{
		String name;
		Ref<AnimationLibrary> library;

		bool operator<(const AnimationLibraryData& p_data) const
		{
			return name < p_data.name;
		}
	};

	struct AnimationData
	{
		Ref<Animation> animation;
		String animation_library;
		uint64_t last_update = 0;
	};

	struct PlaybackInfo
	{
		double time = 0.0;
		double delta = 0.0;
		double start = 0.0;
		double end = 0.0;
		bool seeked = false;
		bool is_external_seeking = false;
		Animation::LoopedFlag looped_flag = Animation::LOOPED_FLAG_NONE;
		real_t weight = 0.0;
		LocalVector<real_t>* track_weights = nullptr;
	};

	struct AnimationInstance
	{
		Ref<Animation> animation;
		PlaybackInfo playback_info;
	};

protected:
	/* ---- Data lists ---- */
	LocalVector<AnimationLibraryData> animation_libraries;
	AHashMap<String, AnimationData>
		animation_set; // HashMap<Library name + Animation name, AnimationData>

	// TODO: This needs to be a TypedArray<String> see this PR for rationale
	// https://github.com/godotengine/godot/pull/110767/
	Vector<String> _get_animation_list() const
	{
		Vector<String> ret;
		LocalVector<String> animations = get_sorted_animation_list();
		ret.resize(animations.size());
		for (uint32_t i = 0; i < animations.size(); ++i) {
			ret.write[i] = animations[i];
		}
		return ret;
	}

	// For caches.
	uint64_t animation_set_update_pass = 1;
	void _animation_set_cache_update();

	// Signals.
	virtual void _animation_added(const String& p_name, const String& p_library);
	virtual void _animation_removed(const String& p_name, const String& p_library);
	virtual void _animation_renamed(
		const String& p_name, const String& p_to_name, const String& p_library);
	virtual void _animation_changed(const String& p_name);

	/* ---- General settings for animation ---- */
	AnimationCallbackModeProcess callback_mode_process = ANIMATION_CALLBACK_MODE_PROCESS_IDLE;
	AnimationCallbackModeMethod callback_mode_method = ANIMATION_CALLBACK_MODE_METHOD_DEFERRED;
	AnimationCallbackModeDiscrete callback_mode_discrete =
		ANIMATION_CALLBACK_MODE_DISCRETE_RECESSIVE;
	int audio_max_polyphony = 32;
	NodePath root_node;

	bool processing = false;
	bool active = true;

	void _set_process(bool p_process, bool p_force = false);

	/* ---- Caches for blending ---- */
	bool cache_valid = false;
	uint64_t setup_pass = 1;

	struct TrackCache
	{
		bool root_motion = false;
		uint64_t setup_pass = 0;
		Animation::TrackType type = Animation::TrackType::TYPE_ANIMATION;
		NodePath path;
		int blend_idx = -1;
		real_t total_weight = 0.0;
		uint64_t animation_instance_weight_applied_at = 0;

		// Transform (TYPE_POSITION_3D, TYPE_ROTATION_3D, TYPE_SCALE_3D)
		int bone_idx = -1;
		bool loc_used = false;
		bool rot_used = false;
		bool scale_used = false;
		Vector3 init_loc = Vector3(0, 0, 0);
		Quaternion init_rot = Quaternion(0, 0, 0, 1);
		Vector3 init_scale = Vector3(1, 1, 1);
		Vector3 loc;
		Quaternion rot;
		Vector3 scale;

		// BlendShape (TYPE_BLEND_SHAPE)
		float init_value = 0.0f;
		float value = 0.0f;
		int shape_index = -1;

		// Value / Bezier (TYPE_VALUE, TYPE_BEZIER)
		Vector<String> subpath;
		bool is_init = false;
		bool use_continuous = false;
		bool use_discrete = false;
		bool is_using_angle = false;
		bool is_variant_interpolatable = true;

		// Audio (TYPE_AUDIO)
		Ref<AudioStreamPolyphonic> audio_stream;
		Ref<AudioStreamPlaybackPolyphonic> audio_stream_playback;
		AudioServer::PlaybackType playback_type = {};
		String bus;

		// Animation (TYPE_ANIMATION)
		bool playing = false;

		TrackCache() = default;
		explicit TrackCache(Animation::TrackType p_type) : type(p_type) {}
		TrackCache(const TrackCache&) = default;
		TrackCache& operator=(const TrackCache&) = default;
		~TrackCache() = default;
	};

	struct RootMotionCache
	{
		Vector3 loc = Vector3(0, 0, 0);
		Quaternion rot = Quaternion(0, 0, 0, 1);
		Vector3 scale = Vector3(1, 1, 1);
	};

	// Audio stream information for each audio stream placed on the track.
	struct PlayingAudioStreamInfo
	{
		AudioStreamPlaybackPolyphonic::ID index =
			-1; // ID retrieved from AudioStreamPlaybackPolyphonic.
		double start = 0.0;
		double len = 0.0;
	};

	// Audio track information for mixng and ending.
	struct PlayingAudioTrackInfo
	{
		AHashMap<int, PlayingAudioStreamInfo> stream_info;
		double length = 0.0;
		double time = 0.0;
		real_t volume = 0.0;
		bool loop = false;
		bool backward = false;
		bool use_blend = false;
	};

	RootMotionCache root_motion_cache;
	AHashMap<Animation::TrackCacheID, TrackCache*, HashHasher> track_cache;
	AHashMap<Ref<Animation>, LocalVector<TrackCache*>> animation_track_num_to_track_cache;
	HashSet<TrackCache*> playing_caches;
	Vector<Node*> playing_audio_stream_players;

	// Helpers.
	void _clear_caches(bool p_clear_track_cache = true);
	void _clear_audio_streams();
	void _clear_playing_caches();
	void _init_root_motion_cache();
	bool _update_caches();
	void _create_track_num_to_track_cache_for_animation(const Ref<Animation>& p_animation);

	/* ---- Audio ---- */
	AudioServer::PlaybackType playback_type;

	/* ---- Blending processor ---- */
	LocalVector<AnimationInstance> animation_instances;
	uint64_t animation_instance_weight_pass_counter = 0;
	AHashMap<NodePath, int> track_map;
	uint64_t track_map_version = 1;
	int track_count = 0;
	bool deterministic = false;

	/* ---- Root motion accumulator for Skeleton3D ---- */
	NodePath root_motion_track;
	bool root_motion_local = false;
	Vector3 root_motion_position = Vector3(0, 0, 0);
	Quaternion root_motion_rotation = Quaternion(0, 0, 0, 1);
	Vector3 root_motion_scale = Vector3(0, 0, 0);
	Vector3 root_motion_position_accumulator = Vector3(0, 0, 0);
	Quaternion root_motion_rotation_accumulator = Quaternion(0, 0, 0, 1);
	Vector3 root_motion_scale_accumulator = Vector3(1, 1, 1);

	virtual uint32_t _get_libraries_property_usage() const;
	void _notification(int p_what);

#ifdef TOOLS_ENABLED
	virtual void get_argument_options(
		const String& p_function, int p_idx, List<String>* r_options) const;
#endif

	void _node_removed(Node* p_node);

	// Helper for extended class.
	virtual void _set_active(bool p_active);
	virtual void _remove_animation(const String& p_name);

	/* ---- Blending processor ---- */
	virtual void _process_animation(double p_delta, bool p_update_only = false);

	// For post process with retrieved key value during blending.
	virtual bool _blend_pre_process(
		double p_delta, int p_track_count, const AHashMap<NodePath, int>& p_track_map);
	virtual void _blend_capture(double p_delta);
	void _blend_calc_total_weight(); // For indeterministic blending.
	void _blend_process(double p_delta, bool p_update_only = false);
	virtual void _blend_post_process();

	/* ---- Capture feature ---- */
	struct CaptureCache
	{
		Ref<Animation> animation;
		double remain = 0.0;
		double step = 0.0;
		Tween::TransitionType trans_type = Tween::TRANS_LINEAR;
		Tween::EaseType ease_type = Tween::EASE_IN;

		void clear()
		{
			animation.unref();
			remain = 0.0;
			step = 0.0;
		}

		~CaptureCache() { clear(); }
	};

	CaptureCache capture_cache;

	void blend_capture(double p_delta); // To blend capture track with all other animations.

public:
	/* ---- Data lists ---- */
	void get_animation_library_list(LocalVector<String>* p_animations) const;
	Ref<AnimationLibrary> get_animation_library(const String& p_name) const;
	bool has_animation_library(const String& p_name) const;
	const String& find_animation_library(const Ref<Animation>& p_animation) const;
	void remove_animation_library(const String& p_name);
	void rename_animation_library(const String& p_name, const String& p_new_name);

	LocalVector<String> get_sorted_animation_list() const;
	void get_animation_list(LocalVector<String>* p_animations) const;
	const Ref<Animation>& get_animation(const String& p_name) const;
	const Ref<Animation>& get_animation_or_null(const String& p_name) const;
	bool has_animation(const String& p_name) const;
	String find_animation(const Ref<Animation>& p_animation) const;

	/* ---- General settings for animation ---- */
	void set_active(bool p_active);
	bool is_active() const;

	void set_deterministic(bool p_deterministic);
	bool is_deterministic() const;

	void set_root_node(const NodePath& p_path);
	NodePath get_root_node() const;

	void set_callback_mode_process(AnimationCallbackModeProcess p_mode);
	AnimationCallbackModeProcess get_callback_mode_process() const;

	AnimationCallbackModeMethod get_callback_mode_method() const;

	void set_callback_mode_discrete(AnimationCallbackModeDiscrete p_mode);
	AnimationCallbackModeDiscrete get_callback_mode_discrete() const;

	/* ---- Audio ---- */
	void set_audio_max_polyphony(int p_audio_max_polyphony);
	int get_audio_max_polyphony() const;

	/* ---- Root motion accumulator for Skeleton3D ---- */
	void set_root_motion_track(const NodePath& p_track);
	NodePath get_root_motion_track() const;

	void set_root_motion_local(bool p_enabled);
	bool is_root_motion_local() const;

	Vector3 get_root_motion_position() const;
	Quaternion get_root_motion_rotation() const;
	Vector3 get_root_motion_scale() const;

	Vector3 get_root_motion_position_accumulator() const;
	Quaternion get_root_motion_rotation_accumulator() const;
	Vector3 get_root_motion_scale_accumulator() const;

	/* ---- Blending processor ---- */

void make_animation_instance(const String& p_name, const PlaybackInfo& p_playback_info);
	void clear_animation_instances();
	virtual void advance(double p_time);
	virtual void clear_caches(); // Must be called by hand if an animation was modified after added.

	/* ---- Reset on save ---- */
	void set_reset_on_save_enabled(bool p_enabled);
	bool is_reset_on_save_enabled() const;
	bool can_apply_reset() const;
	Ref<AnimatedValuesBackup> make_backup();
	void restore(const Ref<AnimatedValuesBackup>& p_backup);
	void reset();

#ifdef TOOLS_ENABLED
	Ref<AnimatedValuesBackup> apply_reset(bool p_user_initiated = false);

	void set_editing(bool p_editing);
	bool is_editing() const;

	void set_dummy(bool p_dummy);
	bool is_dummy() const;
#endif // TOOLS_ENABLED

	AnimationMixer();
	~AnimationMixer();
};

class AnimatedValuesBackup : public RefCounted
{
	AHashMap<Animation::TrackCacheID, AnimationMixer::TrackCache*, HashHasher> data;

public:
	void set_data(
		const AHashMap<Animation::TrackCacheID, AnimationMixer::TrackCache*, HashHasher>& p_data);
	AHashMap<Animation::TrackCacheID, AnimationMixer::TrackCache*, HashHasher> get_data() const;

	void clear_data();

	AnimationMixer::TrackCache* get_cache_copy(AnimationMixer::TrackCache* p_cache) const;

	~AnimatedValuesBackup() { clear_data(); }
};

// --- Folded from scene/animation/animation_tree.h ---



/**
 * 31540000 seconds mean 1 year... is it too long? It must be longer than any Animation
 * length and Transition xfade time to prevent time inversion for AnimationNodeStateMachine.
 */
#define HUGE_LENGTH 31540000

#ifdef TOOLS_ENABLED
#define ENABLE_ACTIVITY_TRACKING
#endif

class AnimationNodeBlendTree;
class AnimationTree;
struct AnimationNodeInstance;

class AnimationNode : public Resource
{
public:
	friend class AnimationTree;

	enum FilterAction
	{
		FILTER_IGNORE,
		FILTER_PASS,
		FILTER_STOP,
		FILTER_BLEND
	};

	enum StateEndpoint
	{
		ENDPOINT_NONE,
		ENDPOINT_START,
		ENDPOINT_END,
	};

	struct Input
	{
		String name;
	};

	bool closable = false;
	LocalVector<Input> inputs;
	HashSet<NodePath> filter;
	bool filter_enabled = false;

	// To propagate information from upstream for use in estimation of playback progress.
	// These values must be taken from the result of blend_node() or blend_input() and must be
	// essentially read-only. For example, if you want to change the position, you need to change
	// the pi.time value of PlaybackInfo passed to blend_input(pi) and get the result.
	struct NodeTimeInfo
	{
		// Retain the previous frame values. These are stored into the AnimationTree's Map and
		// exposing them as read-only values.
		double length = 0.0;
		double position = 0.0;
		double delta = 0.0;

		// Needs internally to estimate remain time, the previous frame values are not retained.
		Animation::LoopMode loop_mode = Animation::LOOP_NONE;
		bool will_end = false;	  // For breaking loop, it is true when just looped.
		bool is_infinity = false; // For unpredictable state machine's end.

		bool is_looping() { return loop_mode != Animation::LOOP_NONE; }

		double get_remain(bool p_break_loop = false)
		{
			if ((is_looping() && !p_break_loop) || is_infinity) {
				return HUGE_LENGTH;
			}
			if (is_looping() && p_break_loop && will_end) {
				return 0;
			}
			double remain = length - position;
			if (Math::is_zero_approx(remain)) {
				return 0;
			}
			return remain;
		}
	};

	struct InvalidInstance
	{
		LocalVector<String> errors;

		struct InputError
		{
			int index;
			String error;
		};

		LocalVector<InputError> input_errors;
	};

	// Temporary state for blending process which needs to be started in the AnimationTree, pass
	// through the AnimationNodes, and then return to the AnimationTree.
	struct ProcessState
	{
		AnimationTree* tree = nullptr;
		const AHashMap<NodePath, int>*
			track_map; // TODO: Is there a better way to manage filter/tracks?
		bool track_map_updated = false;
		bool is_testing = false;
		bool valid = false;
		mutable AHashMap<String, InvalidInstance> invalid_instances;
		uint64_t last_pass = 0;
	};

	// For performance ProcessState needs to be passed down,
	// but the scripting api was already exposed before this optimization was made.
	// So to keep compatibility, we need this internal state, so that the scripting api can continue
	// working as before. It also must be thread_local, because multiple AnimationTrees can be
	// processed in different threads.
	static thread_local ProcessState* tls_process_state;
	static thread_local AnimationNodeInstance* current_instance;

public:
	void _update_filter_cache(
		const ProcessState& p_process_state, const AnimationNodeInstance& p_instance);

	friend class AnimationNodeBlendTree;

	virtual void validate_node(const AnimationTree* p_tree, const String& p_path) const {}

	// The time information is passed from upstream to downstream by
	// AnimationMixer::PlaybackInfo::p_playback_info until AnimationNodeAnimation processes it.
	// Conversely, AnimationNodeAnimation returns the processed result as NodeTimeInfo from
	// downstream to upstream.
	NodeTimeInfo _blend_node(ProcessState& p_process_state, AnimationNodeInstance& p_instance,
		AnimationNodeInstance& p_other, AnimationMixer::PlaybackInfo p_playback_info,
		FilterAction p_filter = FILTER_IGNORE, bool p_sync = true, bool p_test_only = false,
		real_t* r_activity = nullptr);
	NodeTimeInfo _pre_process(ProcessState& p_process_state, AnimationNodeInstance& p_instance,
		const AnimationMixer::PlaybackInfo& p_playback_info, bool p_test_only = false);

protected:
	bool root_node = false;
	bool sync = false;
	StateEndpoint endpoint_type = ENDPOINT_NONE;

	void _add_node(const Ref<AnimationNode>& p_node);
	void _remove_node(const Ref<AnimationNode>& p_node);
	virtual void _tree_changed();

	String current_length = "current_length";
	String current_position = "current_position";
	String current_delta = "current_delta";

	// Virtualizing for especially AnimationNodeAnimation needs to take "backward" into account.
	_FORCE_INLINE_ virtual double get_process_delta(AnimationNodeInstance& p_instance,
		const AnimationMixer::PlaybackInfo& p_playback_info) const
	{
		return p_playback_info.delta;
	}

	virtual NodeTimeInfo _process(ProcessState& p_process_state, AnimationNodeInstance& p_instance,
		const AnimationMixer::PlaybackInfo& p_playback_info,
		bool p_test_only = false); // Main process.

	void blend_animation(ProcessState& p_process_state, AnimationNodeInstance& p_instance,
		const String& p_animation, AnimationMixer::PlaybackInfo& p_playback_info);
	NodeTimeInfo blend_node(ProcessState& p_process_state, AnimationNodeInstance& p_instance,
		AnimationNodeInstance* p_other, const AnimationMixer::PlaybackInfo& p_playback_info,
		FilterAction p_filter = FILTER_IGNORE, bool p_sync = true, bool p_test_only = false);
	NodeTimeInfo blend_input(ProcessState& p_process_state, AnimationNodeInstance& p_instance,
		int p_input, const AnimationMixer::PlaybackInfo& p_playback_info,
		FilterAction p_filter = FILTER_IGNORE, bool p_sync = true, bool p_test_only = false);

	// Bind-able methods to expose for compatibility, moreover AnimationMixer::PlaybackInfo is not
	// exposed.
	void blend_animation_ex(const String& p_animation, double p_time, double p_delta,
		bool p_seeked, bool p_is_external_seeking, real_t p_blend,
		Animation::LoopedFlag p_looped_flag = Animation::LOOPED_FLAG_NONE);
	double blend_node_ex(const String& p_sub_path, const Ref<AnimationNode>& p_node,
		double p_time, bool p_seek, bool p_is_external_seeking, real_t p_blend,
		FilterAction p_filter = FILTER_IGNORE, bool p_sync = true, bool p_test_only = false);
	double blend_input_ex(int p_input, double p_time, bool p_seek, bool p_is_external_seeking,
		real_t p_blend, FilterAction p_filter = FILTER_IGNORE, bool p_sync = true,
		bool p_test_only = false);

	void add_validation_error(const AnimationTree* p_tree, const String& p_path,
		const String& p_error, int p_input_index = -1) const;
	void make_invalid(
		ProcessState& p_process_state, AnimationNodeInstance& p_instance, const String& p_reason);

private:
	mutable uint32_t filters_version = 1;
	void _mark_filters_dirty();

public:
	virtual bool is_parameter_read_only(const String& p_parameter) const;

	struct ChildNode
	{
		String name;
		Ref<AnimationNode> node;
	};

	virtual void get_child_nodes(LocalVector<ChildNode>* r_child_nodes);

	virtual String get_caption() const;

	virtual bool add_input(const String& p_name);
	virtual void remove_input(int p_index);
	virtual bool set_input_name(int p_input, const String& p_name);
	virtual String get_input_name(int p_input) const;
	int get_input_count() const;
	int find_input(const String& p_name) const;

	void set_filter_path(const NodePath& p_path, bool p_enable);
	bool is_path_filtered(const NodePath& p_path) const;

	void set_filter_enabled(bool p_enable);
	bool is_filter_enabled() const;

	void set_deletable(bool p_closable);
	bool is_deletable() const;

	bool is_process_testing() const;

	virtual bool has_filter() const;

#ifdef TOOLS_ENABLED
	virtual void get_argument_options(
		const String& p_function, int p_idx, List<String>* r_options) const;
#endif

	bool is_root_node() const { return root_node; }

	virtual void set_use_sync(bool p_sync);
	virtual bool is_using_sync() const;

	void set_endpoint_type(StateEndpoint p_endpoint)
	{
		endpoint_type = p_endpoint;
		if (p_endpoint != ENDPOINT_NONE) {
			root_node = true;
		}
	}
	StateEndpoint get_endpoint_type() const { return endpoint_type; }

	virtual Ref<AnimationNode> get_child_by_name(const String& p_name) const;
	Ref<AnimationNode> find_node_by_path(const String& p_name) const;

	AnimationNode();
};

// Per instance data, for a node.
// These get created/destroyed when the root AnimationNode is updated, so you cannot use it to store
// persistent data.
struct AnimationNodeInstance
{
	// This makes it faster to access the most commonly used parameters, since we just index an
	// array instead of doing a hash lookup. But unfortunately, these are still Variants, which are
	// quite slow, They are also pointers, which cause a level of indirection.
	//
	// In an ideal world, get_parameter and set_parameter are removed, and everything is made
	// strongly typed (without variant). But that is a ton of work, and this is a good enough
	// optimization for now.
	mutable LocalVector<AnimationNodeInstance*>
		connection_instances; // AnimationNodeInstance* | nullptr
	mutable LocalVector<real_t> track_weights;
	mutable bool blended = false;
	mutable LocalVector<int> filtered_track_indices_cache;
	mutable uint32_t filters_version = 0;
	mutable AHashMap<String, AnimationNodeInstance*>
		child_instances; // Child Name -> AnimationNodeInstance*

	// Multiple AnimationNodeInstances can share the same resource btw.
	Ref<AnimationNode> resource;

	// TODO: these fields are like only used during initialization, or editor, or already slow
	// paths. Consider moving them out of this struct.
	// AnimationNodeInstance *parent = nullptr;
	String path; // e.g. "parameters/node_name/sub_node_name/"

#ifdef ENABLE_ACTIVITY_TRACKING
	struct Activity
	{
		uint64_t last_pass = 0;
		real_t activity = 0.0;
	};

	LocalVector<Activity> input_activity;
#endif

	// Cache for AnimationNodeAnimation
	Ref<Animation> cached_animation;
	uint32_t cached_animation_version = 0;

	// Macro to define all slots.
	// slot index, enum name, member name, variant type, native type
	// you can reuse slot index for different nodes if they don't overlap.
	// Just make sure you update SLOT_MAX accordingly.
#define ANIM_SLOT_LIST(X)                                                                          \
	/* Core parameters. */                                                                         \
	X(0, CURRENT_LENGTH, current_length, Variant::FLOAT, double)                                   \
	X(1, CURRENT_POSITION, current_position, Variant::FLOAT, double)                               \
	X(2, CURRENT_DELTA, current_delta, Variant::FLOAT, double)                                     \
	/* AnimationNodeTimeScale. */                                                                  \
	X(3, TIME_SCALE, scale, Variant::FLOAT, double)                                                \
	/* AnimationNodeTimeSeek. */                                                                   \
	X(3, SEEK_REQUEST, seek_request, Variant::FLOAT, double)                                       \
	/* AnimationNodeAdd2, AnimationNodeAdd3. */                                                    \
	X(3, ADD_AMOUNT, add_amount, Variant::FLOAT, double)                                           \
	/* AnimationNodeBlend2, AnimationNodeBlend3. */                                                \
	X(3, BLEND_AMOUNT, blend_amount, Variant::FLOAT, double)                                       \
	/* AnimationNodeSub2. */                                                                       \
	X(3, SUB_AMOUNT, sub_amount, Variant::FLOAT, double)                                           \
	/* AnimationNodeOneShot. */                                                                    \
	X(3, ONESHOT_TIME_TO_RESTART, time_to_restart, Variant::FLOAT, double)                         \
	X(4, ONESHOT_FADE_IN_REMAINING, fade_in_remaining, Variant::FLOAT, double)                     \
	X(5, ONESHOT_FADE_OUT_REMAINING, fade_out_remaining, Variant::FLOAT, double)                   \
	X(6, ONESHOT_ACTIVE, active, Variant::BOOL, bool)                                              \
	X(7, ONESHOT_INTERNAL_ACTIVE, internal_active, Variant::BOOL, bool)                            \
	X(8, ONESHOT_REQUEST, request, Variant::INT, int)                                              \
	/* AnimationNodeAnimation */                                                                   \
	X(3, ANIMATION_BACKWARD, backward, Variant::BOOL, bool)                                        \
	/* AnimationNodeTransition TODO: Make ref & */                                                 \
	X(3, TRANSITION_REQUEST, transition_request, Variant::STRING, String)                          \
	X(4, CURRENT_INDEX, current_index, Variant::INT, int)                                          \
	X(5, PREV_INDEX, prev_index, Variant::INT, int)                                                \
	X(6, PREV_XFADING, prev_xfading, Variant::FLOAT, double)                                       \
	X(7, CURRENT_STATE, current_state, Variant::STRING, String)                                    \
	/* AnimationNodeBlendSpace1D and AnimationNodeBlendSpace2D,                                    \
	We currently cannot do blend_position, due to type same name but different type */             \
	X(3, CLOSEST, closest, Variant::INT, int)

	enum Slot : uint8_t
	{

#define SLOT_ENUM(index, e, _member, _variant_type, _native_type) SLOT_##e = index,
		ANIM_SLOT_LIST(SLOT_ENUM)
#undef SLOT_ENUM
			SLOT_MAX = 9
	};

#define DEFINE_SET_PARAMETER_METHOD(_index, e, member, _variant_type, native_type)                 \
	_FORCE_INLINE_ void set_parameter_##member(const native_type p_value, bool p_test_only)        \
	{                                                                                              \
		if (p_test_only) {                                                                         \
			return;                                                                                \
		}                                                                                          \
		Variant& prop = *parameter_ptrs_by_slot[SLOT_##e];                                         \
		prop = p_value;                                                                            \
	}
#undef DEFINE_SET_PARAMETER_METHOD

#define DEFINE_GET_PARAMETER_METHOD(_index, e, member, _variant_type, native_type)                 \
	_FORCE_INLINE_ native_type get_parameter_##member() const                                      \
	{                                                                                              \
		return *parameter_ptrs_by_slot[SLOT_##e];                                                  \
	}
#undef DEFINE_GET_PARAMETER_METHOD

	_FORCE_INLINE_ AnimationNodeInstance* get_child_instance_by_path_or_null(
		const String& p_path)
	{
		AnimationNodeInstance** instance_ptr = child_instances.getptr(p_path);
		if (!instance_ptr) {
			return nullptr;
		}
		AnimationNodeInstance* instance = *instance_ptr;
		return instance;
	}

	_FORCE_INLINE_ AnimationNodeInstance& get_child_instance_by_path(const String& p_path)
	{
		AnimationNodeInstance** instance_ptr = child_instances.getptr(p_path);
		CRASH_COND_MSG(
			!instance_ptr, "No child instance found for path: \"" + String(p_path) + "\".");
		AnimationNodeInstance* instance = *instance_ptr;
		CRASH_COND_MSG(
			!instance, "Child instance pointer is null for path: \"" + String(p_path) + "\".");
		return *instance;
	}

	_FORCE_INLINE_ bool is_blended() const { return blended; }
};

class AnimationTree : public AnimationMixer
{
#ifndef DISABLE_DEPRECATED
public:
	enum AnimationProcessCallback
	{
		ANIMATION_PROCESS_PHYSICS,
		ANIMATION_PROCESS_IDLE,
		ANIMATION_PROCESS_MANUAL,
	};
#endif // DISABLE_DEPRECATED

private:
	Ref<AnimationNode> root_animation_node;
	NodePath advance_expression_base_node = NodePath(String("."));

	AnimationNode::ProcessState process_state;
	uint64_t process_pass = 1;
	uint64_t last_track_map_version = 0;

	bool started = true;

	friend class AnimationNode;
	mutable AHashMap<String, AnimationNodeInstance> instance_map;

	mutable bool properties_dirty = true;
	mutable bool validation_dirty = true;

	void _update_properties() const;
	void _update_connections();
	void _add_validation_error(
		const String& p_path, const String& p_error, int p_input_index = -1) const;
	void _update_properties_for_node(
		const String& p_base_path, const Ref<AnimationNode>& p_node) const;

	void _tree_changed();

	NodePath animation_player;

	void _setup_animation_player();
	void _animation_player_changed();

	virtual void _set_active(bool p_active) override;
	virtual uint32_t _get_libraries_property_usage() const override;
	void _notification(int p_what);

	// Make animation instances.
	virtual bool _blend_pre_process(
		double p_delta, int p_track_count, const AHashMap<NodePath, int>& p_track_map) override;

#ifndef DISABLE_DEPRECATED
	void _set_process_callback_bind_compat_80813(AnimationProcessCallback p_mode);
	AnimationProcessCallback _get_process_callback_bind_compat_80813() const;
	void _set_tree_root_bind_compat_80813(const Ref<AnimationNode>& p_root);
	Ref<AnimationNode> _get_tree_root_bind_compat_80813() const;

	static void _bind_compatibility_methods();
#endif // DISABLE_DEPRECATED

public:
	AnimationNodeInstance& get_node_instance_by_path(const String& p_path)
	{
		AnimationNodeInstance* instance = instance_map.getptr(p_path);
		CRASH_COND_MSG(
			instance == nullptr, vformat(R"(No instance found for path "%s".)", String(p_path)));
		return *instance;
	}

	AnimationNodeInstance* get_node_instance_by_path_or_null(const String& p_path) const
	{
		return instance_map.getptr(p_path);
	}

#if TOOLS_ENABLED
	// Used in situations where instance_map is not built.
	// Intended for the editor, because its very slow.
	Ref<AnimationNode> get_animation_node_by_path(const String& p_path) const
	{
		Ref<AnimationNode> current = root_animation_node;
		int name_count = String(p_path).substr(0, String(p_path).length() - 1).count("/");
		for (int i = 0; i < name_count; i++) {
			if (current.is_null()) {
				return Ref<AnimationNode>();
			}

			String child_name = String(String(p_path).get_slicec('/', i + 1));
			current = current->get_child_by_name(child_name);
		}

		return current;
	}
#endif

	void set_animation_player(const NodePath& p_path);
	NodePath get_animation_player() const;

	void set_root_animation_node(const Ref<AnimationNode>& p_animation_node);
	Ref<AnimationNode> get_root_animation_node() const;

	void set_tree_root(const Ref<AnimationNode>& p_animation_node)
	{
		set_root_animation_node(p_animation_node);
	}
	Ref<AnimationNode> get_tree_root() const { return get_root_animation_node(); }

	void set_advance_expression_base_node(const NodePath& p_path);
	NodePath get_advance_expression_base_node() const;

	PackedStringArray get_configuration_warnings() const override;

	bool is_state_invalid() const;
	const AHashMap<String, AnimationNode::InvalidInstance>& get_invalid_instances() const;

	real_t get_connection_activity(const String& p_path, int p_connection) const;

#ifdef TOOLS_ENABLED
	String get_editor_error_message() const;
#endif

	AnimationTree();
	~AnimationTree();
};

// --- Folded from scene/animation/animation_blend_tree.h ---

#include "scene/resources/material.h"

class AnimationNodeBlendNode final : public AnimationNode
{
public:
	enum NodeType
	{
		NODE_ANIMATION,
		NODE_ONESHOT,
		NODE_ADD,
		NODE_BLEND,
		NODE_SUB,
		NODE_TIME_SCALE,
		NODE_TIME_SEEK,
		NODE_TRANSITION,
		NODE_OUTPUT,
	};

	enum PlayMode
	{
		PLAY_MODE_FORWARD,
		PLAY_MODE_BACKWARD,
	};

	enum OneShotRequest
	{
		ONE_SHOT_REQUEST_NONE,
		ONE_SHOT_REQUEST_FIRE,
		ONE_SHOT_REQUEST_ABORT,
		ONE_SHOT_REQUEST_FADE_OUT,
	};

	enum MixMode
	{
		MIX_MODE_BLEND,
		MIX_MODE_ADD,
	};

	struct InputData
	{
		bool auto_advance = false;
		bool break_loop_at_end = false;
		bool reset = true;
	};

	static LocalVector<String> (*get_editable_animation_list)();

private:
	NodeType node_type = NODE_ANIMATION;

	// NODE_ANIMATION
	String backward = "backward"; // Only used by pingpong animation.
	String animation;
	mutable uint32_t animation_version = 1;
	PlayMode play_mode = PLAY_MODE_FORWARD;
	bool advance_on_start = false;
	bool use_custom_timeline = false;
	double timeline_length = 1.0;
	Animation::LoopMode loop_mode = Animation::LOOP_NONE;
	bool stretch_time_scale = true;
	double start_offset = 0.0;

	// NODE_ONESHOT
	double fade_in = 0.0;
	Ref<Curve> fade_in_curve;
	double fade_out = 0.0;
	Ref<Curve> fade_out_curve;
	bool auto_restart = false;
	double auto_restart_delay = 1.0;
	double auto_restart_random_delay = 0.0;
	MixMode mix = MIX_MODE_BLEND;
	bool break_loop_at_end = false;
	bool abort_on_reset = false;
	String request = PNAME("request");
	String active = PNAME("active");
	String internal_active = PNAME("internal_active");
	String fade_in_remaining = "fade_in_remaining";
	String fade_out_remaining = "fade_out_remaining";
	String time_to_restart = "time_to_restart";

	// NODE_ADD / NODE_BLEND / NODE_SUB
	String add_amount = PNAME("add_amount");
	String blend_amount = PNAME("blend_amount");
	String sub_amount = PNAME("sub_amount");
	bool three_way = false;

	// NODE_TIME_SCALE / NODE_TIME_SEEK
	String scale = PNAME("scale");
	String seek_request = PNAME("seek_request");
	bool explicit_elapse = true;

	// NODE_TRANSITION
	LocalVector<InputData> input_data;
	String prev_xfading = "prev_xfading";
	String prev_index = "prev_index";
	String current_index = PNAME("current_index");
	String current_state = PNAME("current_state");
	String transition_request = PNAME("transition_request");
	double xfade_time = 0.0;
	Ref<Curve> xfade_curve;
	bool allow_transition_to_self = false;
	bool pending_update = false;

	void _update_animation_cache(AnimationTree* p_tree, AnimationNodeInstance& p_instance) const;

public:
	NodeType get_node_type() const { return node_type; }
	bool is_three_way() const { return three_way; }

	virtual void validate_node(
		const AnimationTree* p_tree, const String& p_path) const override;
	virtual bool is_parameter_read_only(const String& p_parameter) const override;
	virtual String get_caption() const override;
	virtual bool has_filter() const override;

	_FORCE_INLINE_ virtual double get_process_delta(AnimationNodeInstance& p_instance,
		const AnimationMixer::PlaybackInfo& p_playback_info) const override
	{
		return p_playback_info.delta;
	}

	virtual NodeTimeInfo _process(ProcessState& p_process_state, AnimationNodeInstance& p_instance,
		const AnimationMixer::PlaybackInfo& p_playback_info, bool p_test_only = false) override;

	// NODE_ANIMATION API
	void set_animation(const String& p_name);
	String get_animation() const;

	void set_play_mode(PlayMode p_play_mode);
	PlayMode get_play_mode() const;

	void set_backward(
		AnimationNodeInstance& p_instance, ProcessState& p_process_state, bool p_backward);
	bool is_backward(AnimationNodeInstance& p_instance, ProcessState& p_process_state) const;

	void set_advance_on_start(bool p_advance_on_start);
	bool is_advance_on_start() const;

	void set_use_custom_timeline(bool p_use_custom_timeline);
	bool is_using_custom_timeline() const;

	void set_timeline_length(double p_length);
	double get_timeline_length() const;

	void set_stretch_time_scale(bool p_stretch_time_scale);
	bool is_stretching_time_scale() const;

	void set_start_offset(double p_offset);
	double get_start_offset() const;

	void set_loop_mode(Animation::LoopMode p_loop_mode);
	Animation::LoopMode get_loop_mode() const;

	// NODE_ONESHOT API
	void set_fade_in_time(double p_time);
	double get_fade_in_time() const;

	void set_fade_in_curve(const Ref<Curve>& p_curve);
	Ref<Curve> get_fade_in_curve() const;

	void set_fade_out_time(double p_time);
	double get_fade_out_time() const;

	void set_fade_out_curve(const Ref<Curve>& p_curve);
	Ref<Curve> get_fade_out_curve() const;

	void set_auto_restart_enabled(bool p_enabled);
	void set_auto_restart_delay(double p_time);
	void set_auto_restart_random_delay(double p_time);

	bool is_auto_restart_enabled() const;
	double get_auto_restart_delay() const;
	double get_auto_restart_random_delay() const;

	void set_mix_mode(MixMode p_mix);
	MixMode get_mix_mode() const;

	void set_break_loop_at_end(bool p_enable);
	bool is_loop_broken_at_end() const;

	void set_abort_on_reset(bool p_enable);
	bool is_aborted_on_reset() const;

	// NODE_TIME_SEEK API
	void set_explicit_elapse(bool p_enable);
	bool is_explicit_elapse() const;

	// NODE_TRANSITION API
	void set_input_count(int p_inputs);
	virtual bool add_input(const String& p_name) override;
	virtual void remove_input(int p_index) override;
	virtual bool set_input_name(int p_input, const String& p_name) override;

	void set_input_as_auto_advance(int p_input, bool p_enable);
	bool is_input_set_as_auto_advance(int p_input) const;

	void set_input_break_loop_at_end(int p_input, bool p_enable);
	bool is_input_loop_broken_at_end(int p_input) const;

	void set_input_reset(int p_input, bool p_enable);
	bool is_input_reset(int p_input) const;

	void set_xfade_time(double p_fade);
	double get_xfade_time() const;

	void set_xfade_curve(const Ref<Curve>& p_curve);
	Ref<Curve> get_xfade_curve() const;

	void set_allow_transition_to_self(bool p_enable);
	bool is_allow_transition_to_self() const;

	AnimationNodeBlendNode(NodeType p_type = NODE_ANIMATION, bool p_three_way = false);
};

class AnimationNodeBlendTree final : public AnimationNode
{
	struct Node
	{
		Ref<AnimationNode> node;
		Vector2 position;
		LocalVector<String> connections;
	};

	AHashMap<String, Node> nodes;

	Vector2 graph_offset;

	void _child_node_changed(const String& p_node);

	void _initialize_node_tree();

protected:
	virtual void _tree_changed() override;

	virtual void reset_state() override;

public:
	enum ConnectionError
	{
		CONNECTION_OK,
		CONNECTION_ERROR_NO_INPUT,
		CONNECTION_ERROR_NO_INPUT_INDEX,
		CONNECTION_ERROR_NO_OUTPUT,
		CONNECTION_ERROR_SAME_NODE,
		CONNECTION_ERROR_CONNECTION_EXISTS,
		// no need to check for cycles due to tree topology
	};

	virtual void validate_node(
		const AnimationTree* p_tree, const String& p_path) const override;

	void add_node(const String& p_name, const Ref<AnimationNode>& p_node,
		const Vector2& p_position = Vector2());
	Ref<AnimationNode> get_node(const String& p_name) const;
	void remove_node(const String& p_name);
	void rename_node(const String& p_name, const String& p_new_name);
	bool has_node(const String& p_name) const;
	const LocalVector<String>* get_node_connection_array(const String& p_name) const;

	void set_node_position(const String& p_node, const Vector2& p_position);
	Vector2 get_node_position(const String& p_node) const;

	virtual void get_child_nodes(LocalVector<ChildNode>* r_child_nodes) override;

	void connect_node(
		const String& p_input_node, int p_input_index, const String& p_output_node);
	void disconnect_node(const String& p_node, int p_input_index);

	struct NodeConnection
	{
		String input_node;
		int input_index = 0;
		String output_node;
	};

	ConnectionError can_connect_node(
		const String& p_input_node, int p_input_index, const String& p_output_node) const;
	void get_node_connections(LocalVector<NodeConnection>* r_connections) const;

	virtual String get_caption() const override;
	virtual NodeTimeInfo _process(ProcessState& p_process_state, AnimationNodeInstance& p_instance,
		const AnimationMixer::PlaybackInfo& p_playback_info, bool p_test_only = false) override;

	LocalVector<String> get_node_list() const;

	void set_graph_offset(const Vector2& p_graph_offset);
	Vector2 get_graph_offset() const;

	virtual Ref<AnimationNode> get_child_by_name(const String& p_name) const override;

#ifdef TOOLS_ENABLED
	virtual void get_argument_options(
		const String& p_function, int p_idx, List<String>* r_options) const override;
#endif

	AnimationNodeBlendTree();
	~AnimationNodeBlendTree() = default;
};

// --- End scene/animation/animation_blend_tree.h ---

// --- End scene/animation/animation_tree.h ---

// --- Folded from scene/audio/audio_stream_player.h ---
#include "core/os/memory.h"
#include "scene/main/node.h"
#include "servers/display/display_server.h"

struct AudioFrame;
class AudioStream;
class AudioStreamPlayback;
class AudioSamplePlayback;

class AudioStreamPlayerInternal
{
private:
	struct ParameterData
	{
		String path;
	};

	static inline const String PARAM_PREFIX = "parameters/";

	Node* node = nullptr;
	bool physical = false;
	AudioServer::PlaybackType playback_type = AudioServer::PlaybackType::PLAYBACK_TYPE_DEFAULT;

	HashMap<String, ParameterData> playback_parameters;

	void _set_process(bool p_enabled);
	void _update_stream_parameters();

	_FORCE_INLINE_ bool _is_sample()
	{
		return (AudioServer::get_default_playback_type() ==
					   AudioServer::PlaybackType::PLAYBACK_TYPE_SAMPLE &&
				   get_playback_type() == AudioServer::PlaybackType::PLAYBACK_TYPE_DEFAULT) ||
			   get_playback_type() == AudioServer::PlaybackType::PLAYBACK_TYPE_SAMPLE;
	}

public:
	Vector<Ref<AudioStreamPlayback>> stream_playbacks;
	Ref<AudioStream> stream;

	SafeFlag active;

	float pitch_scale = 1.0;
	float volume_db = 0.0;
	bool autoplay = false;
	String bus;
	int max_polyphony = 1;

	void process();
	void ensure_playback_limit();

	void notification(int p_what);

	void set_stream(Ref<AudioStream> p_stream);
	void set_pitch_scale(float p_pitch_scale);
	void set_max_polyphony(int p_max_polyphony);

	String get_bus() const;

	Ref<AudioStreamPlayback> play_basic();
	void seek(float p_seconds);
	void stop_basic();
	bool is_playing() const;

	float get_playback_position();

	void set_playing(bool p_enable);
	bool is_active() const;

	void set_stream_paused(bool p_pause);
	bool get_stream_paused() const;

	bool has_stream_playback();
	Ref<AudioStreamPlayback> get_stream_playback();

	void set_playback_type(AudioServer::PlaybackType p_playback_type);
	AudioServer::PlaybackType get_playback_type() const;
};

class AudioStreamPlayer : public Node
{
public:
	enum MixTarget
	{
		MIX_TARGET_STEREO,
		MIX_TARGET_SURROUND,
		MIX_TARGET_CENTER
	};

private:
	AudioStreamPlayerInternal* internal = nullptr;

	MixTarget mix_target = MIX_TARGET_STEREO;

	void _set_playing(bool p_enable);
	bool _is_active() const;

	Vector<AudioFrame> _get_volume_vector();

protected:
	void _notification(int p_what);

public:
	void set_stream(Ref<AudioStream> p_stream);
	Ref<AudioStream> get_stream() const;

	void set_volume_db(float p_volume);
	float get_volume_db() const;

	void set_volume_linear(float p_volume);
	float get_volume_linear() const;

	void set_pitch_scale(float p_pitch_scale);
	float get_pitch_scale() const;

	void set_max_polyphony(int p_max_polyphony);
	int get_max_polyphony() const;

	void play(float p_from_pos = 0.0);
	void seek(float p_seconds);
	void stop();
	bool is_playing() const;
	float get_playback_position();

	void set_bus(const String& p_bus);
	String get_bus() const;

	void set_autoplay(bool p_enable);
	bool is_autoplay_enabled() const;

	void set_mix_target(MixTarget p_target);
	MixTarget get_mix_target() const;

	void set_stream_paused(bool p_pause);
	bool get_stream_paused() const;

	bool has_stream_playback();
	Ref<AudioStreamPlayback> get_stream_playback();

	AudioServer::PlaybackType get_playback_type() const;
	void set_playback_type(AudioServer::PlaybackType p_playback_type);

	AudioStreamPlayer();
	~AudioStreamPlayer();
};



// --- End scene/audio/audio_stream_player.h ---

// --- Folded from scene/animation/animation_player.h ---


class AnimationPlayer : public AnimationMixer
{
#ifndef DISABLE_DEPRECATED
public:
	enum AnimationProcessCallback
	{
		ANIMATION_PROCESS_PHYSICS,
		ANIMATION_PROCESS_IDLE,
		ANIMATION_PROCESS_MANUAL,
	};

	enum AnimationMethodCallMode
	{
		ANIMATION_METHOD_CALL_DEFERRED,
		ANIMATION_METHOD_CALL_IMMEDIATE,
	};
#endif // DISABLE_DEPRECATED

private:
	AHashMap<String, String> animation_next_set; // For auto advance.

	String finished_anim;

	float speed_scale = 1.0;
	double default_blend_time = 0.0;

	bool auto_capture = true;
	double auto_capture_duration = -1.0;
	Tween::TransitionType auto_capture_transition_type = Tween::TRANS_LINEAR;
	Tween::EaseType auto_capture_ease_type = Tween::EASE_IN;

	struct PlaybackData
	{
		bool is_enabled = false;
		String animation_name;
		double animation_length = 0.0;
		double pos = 0.0;
		float speed_scale = 1.0;
		double start_time = 0.0;
		double end_time = 0.0;

		double get_start_time() const
		{
			if (is_enabled && (Animation::is_less_approx(start_time, 0) ||
								  Animation::is_greater_approx(start_time, animation_length))) {
				return 0;
			}
			return start_time;
		}

		double get_end_time() const
		{
			if (is_enabled && (Animation::is_less_approx(end_time, 0) ||
								  Animation::is_greater_approx(end_time, animation_length))) {
				return animation_length;
			}
			return end_time;
		}
	};

	struct Blend
	{
		PlaybackData data;
		double blend_time = 0.0;
		double blend_left = 0.0;
	};

	struct Playback
	{
		PlaybackData current;
		String assigned;
		bool seeked = false;
		bool internal_seeked = false;
		bool started = false;
		LocalVector<Blend> blend;
	} playback;

	struct BlendKey
	{
		String from;
		String to;

		static uint32_t hash(const BlendKey& p_key)
		{
			return hash_one_uint64((uint64_t(p_key.from.hash()) << 32) | uint32_t(p_key.to.hash()));
		}

		bool operator==(const BlendKey& bk) const { return from == bk.from && to == bk.to; }

		bool operator<(const BlendKey& bk) const
		{
			if (from == bk.from) {
				return to < bk.to;
			}
			return from < bk.from;
		}
	};

	HashMap<BlendKey, double, BlendKey> blend_times;

	List<String> playback_queue;
	bool end_reached = false;
	bool end_notify = false;

	String autoplay;

	bool movie_quit_on_finish = false;
	bool clear_cache_on_stop = true;

	void _play(const String& p_name, double p_custom_blend = -1, float p_custom_scale = 1.0,
		bool p_from_end = false);
	void _capture(const String& p_name, bool p_from_end = false, double p_duration = -1.0,
		Tween::TransitionType p_trans_type = Tween::TRANS_LINEAR,
		Tween::EaseType p_ease_type = Tween::EASE_IN);
	void _process_playback_data(PlaybackData& cd, double p_delta, float p_blend, bool p_seeked,
		bool p_internal_seeked, bool p_started, bool p_is_current = false);
	void _blend_playback_data(double p_delta, bool p_started);
	void _stop_internal(bool p_reset, bool p_keep_state);
	void _check_immediately_after_start();

	float get_current_blend_amount();

	bool playing = false;

protected:
	void _notification(int p_what);

	// Make animation instances.
	virtual void _blend_capture(double p_delta) override;
	virtual bool _blend_pre_process(
		double p_delta, int p_track_count, const AHashMap<NodePath, int>& p_track_map) override;
	virtual void _blend_post_process() override;

	virtual void _animation_changed(const String& p_name) override;
	virtual void _animation_removed(const String& p_name, const String& p_library) override;
	virtual void _rename_animation(const String& p_from_name, const String& p_to_name);

#ifndef DISABLE_DEPRECATED
	void _set_process_callback_bind_compat_80813(AnimationProcessCallback p_mode);
	AnimationProcessCallback _get_process_callback_bind_compat_80813() const;
	void _set_method_call_mode_bind_compat_80813(AnimationMethodCallMode p_mode);
	AnimationMethodCallMode _get_method_call_mode_bind_compat_80813() const;
	void _set_root_bind_compat_80813(const NodePath& p_root);
	NodePath _get_root_bind_compat_80813() const;
	void _seek_bind_compat_80813(double p_time, bool p_update = false);
	void _play_bind_compat_84906(const String& p_name = String(), double p_custom_blend = -1,
		float p_custom_scale = 1.0, bool p_from_end = false);
	void _play_backwards_bind_compat_84906(
		const String& p_name = String(), double p_custom_blend = -1);

	Vector<String> _get_queue_bind_compat_110767();
	String _get_current_animation_bind_compat_110767() const;
	void _set_current_animation_bind_compat_110767(const String& p_animation);
	String _get_assigned_animation_bind_compat_110767() const;
	void _set_assigned_animation_bind_compat_110767(const String& p_animation);
	String _get_autoplay_bind_compat_110767() const;
	void _set_autoplay_bind_compat_110767(const String& p_name);

	static void _bind_compatibility_methods();
#endif // DISABLE_DEPRECATED

public:
	void animation_set_next(const String& p_animation, const String& p_next);
	String animation_get_next(const String& p_animation) const;

	void set_blend_time(const String& p_animation1, const String& p_animation2, double p_time);
	double get_blend_time(const String& p_animation1, const String& p_animation2) const;

	void set_default_blend_time(double p_default);
	double get_default_blend_time() const;

	void set_auto_capture(bool p_auto_capture);
	bool is_auto_capture() const;
	void set_auto_capture_duration(double p_auto_capture_duration);
	double get_auto_capture_duration() const;
	void set_auto_capture_transition_type(Tween::TransitionType p_auto_capture_transition_type);
	Tween::TransitionType get_auto_capture_transition_type() const;
	void set_auto_capture_ease_type(Tween::EaseType p_auto_capture_ease_type);
	Tween::EaseType get_auto_capture_ease_type() const;

#ifdef TOOLS_ENABLED
	void get_argument_options(
		const String& p_function, int p_idx, List<String>* r_options) const override;
#endif

	void play(const String& p_name = String(), double p_custom_blend = -1,
		float p_custom_scale = 1.0, bool p_from_end = false);
	void play_section_with_markers(const String& p_name = String(),
		const String& p_start_marker = String(), const String& p_end_marker = String(),
		double p_custom_blend = -1, float p_custom_scale = 1.0, bool p_from_end = false);
	void play_section(const String& p_name = String(), double p_start_time = -1,
		double p_end_time = -1, double p_custom_blend = -1, float p_custom_scale = 1.0,
		bool p_from_end = false);
	void play_backwards(const String& p_name = String(), double p_custom_blend = -1);
	void play_section_with_markers_backwards(const String& p_name = String(),
		const String& p_start_marker = String(), const String& p_end_marker = String(),
		double p_custom_blend = -1);
	void play_section_backwards(const String& p_name = String(), double p_start_time = -1,
		double p_end_time = -1, double p_custom_blend = -1);
	void play_with_capture(const String& p_name = String(), double p_duration = -1.0,
		double p_custom_blend = -1, float p_custom_scale = 1.0, bool p_from_end = false,
		Tween::TransitionType p_trans_type = Tween::TRANS_LINEAR,
		Tween::EaseType p_ease_type = Tween::EASE_IN);
	void queue(const String& p_name);
	void clear_queue();
	void pause();
	void stop(bool p_keep_state = false);
	bool is_playing() const;
	String get_current_animation() const;
	void set_current_animation(const String& p_animation);
	String get_assigned_animation() const;
	void set_assigned_animation(const String& p_animation);
	bool is_valid() const;

	void set_speed_scale(float p_speed);
	float get_speed_scale() const;
	float get_playing_speed() const;

	void set_autoplay(const String& p_name);
	String get_autoplay() const;

	void set_movie_quit_on_finish_enabled(bool p_enabled);
	bool is_movie_quit_on_finish_enabled() const;

	void set_clear_cache_on_stop_enabled(bool p_enabled);
	bool is_clear_cache_on_stop_enabled() const;

	void seek_internal(double p_time, bool p_update = false, bool p_update_only = false,
		bool p_is_internal_seek = false);
	void seek(double p_time, bool p_update = false, bool p_update_only = false);

	double get_current_animation_position() const;
	double get_current_animation_length() const;

	void set_section_with_markers(
		const String& p_start_marker = String(), const String& p_end_marker = String());
	void set_section(double p_start_time = -1, double p_end_time = -1);
	void reset_section();

	double get_section_start_time() const;
	double get_section_end_time() const;
	bool has_section() const;

	virtual void advance(double p_time) override;

	AnimationPlayer();
	~AnimationPlayer();
};
// --- End scene/animation/animation_mixer.h ---
// --- Inlined/Folded from scene/main/multiplayer_api.h ---
#include "core/templates/hash_set.h"
#include "core/types.h"
// --- Inlined/Folded from scene/main/multiplayer_peer.h ---
#include "core/io/net_socket.h"

class MultiplayerPeer : public PacketPeer
{
public:
	enum TransferMode
	{
		TRANSFER_MODE_UNRELIABLE,
		TRANSFER_MODE_UNRELIABLE_ORDERED,
		TRANSFER_MODE_RELIABLE
	};

protected:

private:
	int transfer_channel = 0;
	TransferMode transfer_mode = TRANSFER_MODE_RELIABLE;
	bool refuse_connections = false;

public:
	enum
	{
		TARGET_PEER_BROADCAST = 0,
		TARGET_PEER_SERVER = 1
	};

	enum ConnectionStatus
	{
		CONNECTION_DISCONNECTED,
		CONNECTION_CONNECTING,
		CONNECTION_CONNECTED,
	};

	virtual void set_transfer_channel(int p_channel);
	virtual int get_transfer_channel() const;
	virtual void set_transfer_mode(TransferMode p_mode);
	virtual TransferMode get_transfer_mode() const;
	virtual void set_refuse_new_connections(bool p_enable);
	virtual bool is_refusing_new_connections() const;
	virtual bool is_server_relay_supported() const;

	virtual void set_target_peer(int p_peer_id) = 0;

	virtual int get_packet_peer() const = 0;
	virtual TransferMode get_packet_mode() const = 0;
	virtual int get_packet_channel() const = 0;

	virtual void disconnect_peer(int p_peer, bool p_force = false) = 0;

	virtual bool is_server() const = 0;

	virtual void poll() = 0;
	virtual void close() = 0;

	virtual int get_unique_id() const = 0;

	virtual ConnectionStatus get_connection_status() const = 0;

	uint32_t generate_unique_id() const;
};

class MultiplayerPeerExtension : public MultiplayerPeer
{
protected:

	PackedByteArray script_buffer;

public:
	/* PacketPeer extension */
	virtual Error get_packet(const uint8_t** r_buffer,
		int& r_buffer_size) override; ///< buffer is GONE after next get_packet
	virtual Error put_packet(const uint8_t* p_buffer, int p_buffer_size) override;
	/* MultiplayerPeer extension */
	virtual void set_refuse_new_connections(bool p_enable) override;
	virtual bool is_refusing_new_connections() const override;
	virtual bool is_server_relay_supported() const override;
};

class MultiplayerAPI : public RefCounted
{
public:
	struct SceneData;

private:
	static String default_interface;
	SceneData* scene_data = nullptr;

	Error _send(const uint8_t* p_packet, int p_packet_len);
	void _process_packet(int p_from, const uint8_t* p_packet, int p_packet_len);
	void _process_raw(int p_from, const uint8_t* p_packet, int p_packet_len);
	void _process_sys(int p_from, const uint8_t* p_packet, int p_packet_len,
		MultiplayerPeer::TransferMode p_mode, int p_channel);
	void _add_peer(int p_id);
	void _admit_peer(int p_id);
	void _del_peer(int p_id);
	void _update_status();

public:
	enum RPCMode
	{
		RPC_MODE_DISABLED,	// No rpc for this method, calls to this will be blocked (default)
		RPC_MODE_ANY_PEER,	// Any peer can call this RPC
		RPC_MODE_AUTHORITY, // Only the node's multiplayer authority (server by default) can call
							// this RPC
	};

	enum NetworkCommands
	{
		NETWORK_COMMAND_REMOTE_CALL = 0,
		NETWORK_COMMAND_SIMPLIFY_PATH,
		NETWORK_COMMAND_CONFIRM_PATH,
		NETWORK_COMMAND_RAW,
		NETWORK_COMMAND_SPAWN,
		NETWORK_COMMAND_DESPAWN,
		NETWORK_COMMAND_SYNC,
		NETWORK_COMMAND_SYS,
	};

	enum SysCommands
	{
		SYS_COMMAND_AUTH,
		SYS_COMMAND_ADD_PEER,
		SYS_COMMAND_DEL_PEER,
		SYS_COMMAND_RELAY,
	};

	enum
	{
		SYS_CMD_SIZE = 6, // Command + sys command + peer_id (+ optional payload).
	};

	// For each command, the 4 MSB can contain custom flags, as defined by subsystems.
	enum
	{
		CMD_FLAG_0_SHIFT = 4,
		CMD_FLAG_1_SHIFT = 5,
		CMD_FLAG_2_SHIFT = 6,
		CMD_FLAG_3_SHIFT = 7,
	};

	// This is the mask that will be used to extract the command.
	enum
	{
		CMD_MASK = 7, // 0x7 -> 0b00000111
	};

	static Ref<MultiplayerAPI> create_default_interface();
	static void set_default_interface(const String& p_interface);
	static String get_default_interface();

	Error poll();
	void set_multiplayer_peer(const Ref<MultiplayerPeer>& p_peer);
	Ref<MultiplayerPeer> get_multiplayer_peer();
	int get_unique_id();
	Vector<int> get_peer_ids();
	int get_remote_sender_id() const;

	bool has_multiplayer_peer() { return get_multiplayer_peer().is_valid(); }
	bool is_server() { return get_unique_id() == MultiplayerPeer::TARGET_PEER_SERVER; }

	void clear();

	void set_root_path(const NodePath& p_path);
	NodePath get_root_path() const;

	void disconnect_peer(int p_id);

	Error send_auth(int p_to, Vector<uint8_t> p_bytes);
	Error complete_auth(int p_peer);
	void set_auth_timeout(double p_timeout);
	double get_auth_timeout() const;
	Vector<int> get_authenticating_peer_ids();

	Error send_command(int p_to, const uint8_t* p_packet, int p_packet_len);
	Error send_bytes(Vector<uint8_t> p_data, int p_to = MultiplayerPeer::TARGET_PEER_BROADCAST,
		MultiplayerPeer::TransferMode p_mode = MultiplayerPeer::TRANSFER_MODE_RELIABLE,
		int p_channel = 0);

	HashSet<int> get_connected_peers() const;
	void set_remote_sender_override(int p_id);

	void set_refuse_new_connections(bool p_refuse);
	bool is_refusing_new_connections() const;

	void set_allow_object_decoding(bool p_enable);
	bool is_object_decoding_allowed() const;

	void set_server_relay_enabled(bool p_enabled);
	bool is_server_relay_enabled() const;

	void set_max_sync_packet_size(int p_size);
	int get_max_sync_packet_size() const;

	void set_max_delta_packet_size(int p_size);
	int get_max_delta_packet_size() const;

	MultiplayerAPI();
	~MultiplayerAPI();
};
// --- Folded from scene/main/scene_tree_fti.h ---
#include "core/os/mutex.h"
#include "core/templates/local_vector.h"

class Node3D;
class Node;
struct Transform3D;
class SceneTreeFTITests;

#ifdef DEV_ENABLED
// Uncomment this to verify traversal method results.
// #define GODOT_SCENE_TREE_FTI_VERIFY
#endif

#ifdef _3D_DISABLED
// Stubs
class SceneTreeFTI final
{
public:
	SceneTreeFTI() = delete;

	static void initialize() {}
	static void finalize() {}
	_FORCE_INLINE_ static bool is_initialized() { return true; }

	static void frame_update(Node* p_root, bool p_frame_start)
	{
		(void)p_root;
		(void)p_frame_start;
	}
	static void tick_update() {}
	static void set_enabled(Node* p_root, bool p_enabled)
	{
		(void)p_root;
		(void)p_enabled;
	}
	static bool is_enabled() { return false; }

	static void node_3d_notify_changed(Node3D& r_node, bool p_transform_changed)
	{
		(void)r_node;
		(void)p_transform_changed;
	}
	static void node_3d_notify_delete(Node3D* p_node) { (void)p_node; }
	static void node_3d_request_reset(Node3D* p_node) { (void)p_node; }
};
#else

// Important.
// This class uses raw pointers, so it is essential that on deletion, this class is notified
// so that any references can be cleared up to prevent dangling pointer access.

class SceneTreeFTI final
{
	friend class SceneTreeFTITests;

public:
	enum TraversalMode : unsigned
	{
		TM_DEFAULT,
		TM_LEGACY,
		TM_DEBUG,
	};

	struct Data
	{
		static const uint32_t scene_tree_depth_limit = 48;

		// Prev / Curr lists of Node3Ds having local xforms pumped.
		LocalVector<Node3D*> tick_xform_list[2];

		// The frame lists are changed nodes that need to start traversal,
		// either longterm (on the tick list) or single frame forced.
		LocalVector<Node3D*> frame_xform_list;
		LocalVector<Node3D*> frame_xform_list_forced;

		// Prev / Curr lists of Node3Ds having actively interpolated properties.
		LocalVector<Node3D*> tick_property_list[2];

		LocalVector<Node3D*> frame_property_list;
		LocalVector<Node3D*> request_reset_list;
		LocalVector<Node3D*> dirty_node_depth_lists[scene_tree_depth_limit];

		// When we are using two alternating lists,
		// which one is current.
		uint32_t mirror = 0;

		// Global on / off switch for SceneTreeFTI.
		bool enabled = false;

		// Whether we are in physics ticks, or in a frame.
		bool in_frame = false;

		// Updating at the start of the frame, or the end on second pass.
		bool frame_start = true;

		Mutex mutex;

		TraversalMode traversal_mode = TM_DEFAULT;
		bool use_optimized_traversal_method = true;

		// DEBUGGING
		bool periodic_debug_log = false;
		uint32_t debug_node_count = 0;
		uint32_t debug_nodes_processed = 0;

#ifdef GODOT_SCENE_TREE_FTI_VERIFY
		SceneTreeFTITests* _tests = nullptr;
#endif
	};

	static inline Data* data = nullptr;

private:
	static void _update_dirty_nodes(Node* p_node, uint32_t p_current_half_frame,
		float p_interpolation_fraction, bool p_active,
		const Transform3D* p_parent_global_xform = nullptr, int p_depth = 0);
	static void _update_request_resets();

	static void _reset_flags(Node* p_node);
	static void _reset_node3d_flags(Node3D& r_node);
	static void _node_3d_notify_set_xform(Node3D& r_node);
	static void _node_3d_notify_set_property(Node3D& r_node);

	static void _node_add_to_frame_list(Node3D& r_node, bool p_forced);
	static void _node_remove_from_frame_list(Node3D& r_node, bool p_forced);

	static void _create_depth_lists();
	static void _clear_depth_lists();

public:
	SceneTreeFTI() = delete;

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	// Hottest function, allow inlining the data->enabled check.
	static void node_3d_notify_changed(Node3D& r_node, bool p_transform_changed)
	{
		if (!data || !data->enabled) {
			return;
		}
		MutexLock lock(data->mutex);

		if (p_transform_changed) {
			_node_3d_notify_set_xform(r_node);
		}
		else {
			_node_3d_notify_set_property(r_node);
		}
	}

	static void node_3d_request_reset(Node3D* p_node);
	static void node_3d_notify_delete(Node3D* p_node);

	// Calculate interpolated xforms, send to visual server.
	static void frame_update(Node* p_root, bool p_frame_start);

	// Update local xform pumps.
	static void tick_update();

	static void set_enabled(Node* p_root, bool p_enabled);
	static bool is_enabled() { return data ? data->enabled : false; }

	static void set_debug_next_frame()
	{
		if (data) {
			data->periodic_debug_log = true;
		}
	}
};

#endif // ndef _3D_DISABLED

// --- End scene/main/scene_tree_fti.h ---

class ArrayMesh;
struct InputEvent;
class Material;
class MultiplayerAPI;
class Node;
class PackedScene;
class Tween;
class Viewport;
class Window;

#ifndef _3D_DISABLED
class Node3D;
#endif

class SceneTreeTimer : public RefCounted
{
	double time_left = 0.0;
	bool process_always = true;
	bool process_in_physics = false;
	bool ignore_time_scale = false;

public:
	void set_time_left(double p_time);
	double get_time_left() const;

	void set_process_always(bool p_process_always);
	bool is_process_always();

	void set_process_in_physics(bool p_process_in_physics);
	bool is_process_in_physics();

	void set_ignore_time_scale(bool p_ignore);
	bool is_ignoring_time_scale();

	void release_connections();
};

struct SceneTreeGroup
{
	Vector<Node*> nodes;
	bool changed = false;
};

class SceneTree final
{
	static inline Mutex _thread_safe_;

public:
	typedef void (*IdleCallback)();

	struct ProcessGroup
	{
		Vector<Node*> nodes;
		Vector<Node*> physics_nodes;
		bool node_order_dirty = true;
		bool physics_node_order_dirty = true;
		bool removed = false;
		Node* owner = nullptr;
		uint64_t last_pass = 0;
	};

	struct ProcessGroupSort
	{
		_FORCE_INLINE_ bool operator()(
			const ProcessGroup* p_left, const ProcessGroup* p_right) const;
	};

#ifndef _3D_DISABLED
	struct ClientPhysicsInterpolation
	{
		SelfList<Node3D>::List _node_3d_list;
		void physics_process();
	};
#endif

	struct UGCall
	{
		String group;
		String call;

		static uint32_t hash(const UGCall& p_val) { return p_val.group.hash() ^ p_val.call.hash(); }

		bool operator==(const UGCall& p_with) const
		{
			return group == p_with.group && call == p_with.call;
		}

		bool operator<(const UGCall& p_with) const
		{
			return group == p_with.group ? call < p_with.call : group < p_with.group;
		}
	};

	struct Data
	{
		PagedAllocator<ProcessGroup, true> group_allocator;
		LocalVector<ProcessGroup*> process_groups;
		bool process_groups_dirty = true;
		LocalVector<ProcessGroup*> local_process_group_cache;
		uint64_t process_last_pass = 1;

		ProcessGroup default_process_group;

		bool node_threading_disabled = false;

#ifndef _3D_DISABLED
		ClientPhysicsInterpolation _client_physics_interpolation;
#endif

		Window* root = nullptr;

		double physics_process_time = 0.0;
		double process_time = 0.0;
		bool accept_quit = true;
		bool quit_on_go_back = true;

#ifdef DEBUG_ENABLED
		bool debug_collisions_hint = false;
		bool debug_paths_hint = false;
		bool debug_navigation_hint = false;
#endif
		bool paused = false;
		bool suspended = false;

		HashMap<String, SceneTreeGroup> group_map;
		bool _quit = false;

		String tree_changed_name = "tree_changed";
		String node_added_name = "node_added";
		String node_removed_name = "node_removed";
		String node_renamed_name = "node_renamed";

		int64_t current_frame = 0;
		int nodes_in_tree_count = 0;

#ifdef TOOLS_ENABLED
		Node* edited_scene_root = nullptr;
#endif

		int nodes_removed_on_group_call_lock = 0;
		HashSet<Node*> nodes_removed_on_group_call;

		uint64_t accessibility_upd_per_sec = 0;
		bool accessibility_force_update = true;
		uint64_t accessibility_last_update = 0;

		bool ugc_locked = false;

		Node* current_scene = nullptr;

		Color debug_collisions_color;
		Color debug_collision_contact_color;
		Color debug_paths_color;
		float debug_paths_width = 1.0f;
		Ref<ArrayMesh> debug_contact_mesh;
		Ref<Material> debug_paths_material;
		Ref<Material> collision_material;
		int collision_debug_contacts = 0;

		List<Ref<SceneTreeTimer>> timers;
		List<Ref<Tween>> tweens;

		Ref<MultiplayerAPI> multiplayer;
		HashMap<NodePath, Ref<MultiplayerAPI>> custom_multiplayers;
		bool multiplayer_poll = true;

		SelfList<Node>::List xform_change_list;
	};

	static inline Data* data = nullptr;

private:
	// Static so we can get directly instead of via SceneTree::data.
	static bool _physics_interpolation_enabled;
	static bool _physics_interpolation_enabled_in_project;

	static void _flush_ugc();
	_FORCE_INLINE_ static void _update_group_order(SceneTreeGroup& g);
	static void _flush_scene_change();

	friend class Node;
	friend class CanvasItem;
	friend class Node3D;
	friend class Viewport;
#ifdef DEBUG_ENABLED
	friend class LiveEditor;
#endif

	static void tree_changed();
	static void node_added(Node* p_node);
	static void node_removed(Node* p_node);
	static void node_renamed(Node* p_node);
	static void process_timers(double p_delta, bool p_physics_frame);
	static void process_tweens(double p_delta, bool p_physics_frame);

	static SceneTreeGroup* add_to_group(const String& p_group, Node* p_node);
	static void remove_from_group(const String& p_group, Node* p_node);

	static void _process_group(ProcessGroup* p_group, bool p_physics);
	static void _process_groups_thread(uint32_t p_index, bool p_physics);
	static void _process(bool p_physics);

	static void _remove_process_group(Node* p_node);
	static void _add_process_group(Node* p_node);
	static void _remove_node_from_process_group(Node* p_node, Node* p_owner);
	static void _add_node_to_process_group(Node* p_node, Node* p_owner);

	static void _flush_delete_queue();

	enum
	{
		MAX_IDLE_CALLBACKS = 256
	};

	static IdleCallback idle_callbacks[MAX_IDLE_CALLBACKS];
	static int idle_callback_count;
	static void _call_idle_callbacks();

	static void _main_window_focus_in();
	static void _main_window_close();
	static void _main_window_go_back();

	enum CallInputType
	{
		CALL_INPUT_TYPE_INPUT,
		CALL_INPUT_TYPE_SHORTCUT_INPUT,
		CALL_INPUT_TYPE_UNHANDLED_INPUT,
		CALL_INPUT_TYPE_UNHANDLED_KEY_INPUT,
	};

	static void _call_input_pause(const String& p_group, CallInputType p_call_type,
		const InputEvent& p_input, Viewport* p_viewport);

public:
	SceneTree() = delete;

	enum
	{
		// Keep in sync with CanvasItem, Node3D, and MainLoop.
		NOTIFICATION_TRANSFORM_CHANGED = 2000,
		NOTIFICATION_OS_MEMORY_WARNING = MainLoop::NOTIFICATION_OS_MEMORY_WARNING,
		NOTIFICATION_TRANSLATION_CHANGED = MainLoop::NOTIFICATION_TRANSLATION_CHANGED,
		NOTIFICATION_WM_ABOUT = MainLoop::NOTIFICATION_WM_ABOUT,
		NOTIFICATION_CRASH = MainLoop::NOTIFICATION_CRASH,
		NOTIFICATION_OS_IME_UPDATE = MainLoop::NOTIFICATION_OS_IME_UPDATE,
		NOTIFICATION_APPLICATION_RESUMED = MainLoop::NOTIFICATION_APPLICATION_RESUMED,
		NOTIFICATION_APPLICATION_PAUSED = MainLoop::NOTIFICATION_APPLICATION_PAUSED,
		NOTIFICATION_APPLICATION_FOCUS_IN = MainLoop::NOTIFICATION_APPLICATION_FOCUS_IN,
		NOTIFICATION_APPLICATION_FOCUS_OUT = MainLoop::NOTIFICATION_APPLICATION_FOCUS_OUT,
		NOTIFICATION_TEXT_SERVER_CHANGED = MainLoop::NOTIFICATION_TEXT_SERVER_CHANGED,
		NOTIFICATION_APPLICATION_PIP_MODE_ENTERED =
			MainLoop::NOTIFICATION_APPLICATION_PIP_MODE_ENTERED,
		NOTIFICATION_APPLICATION_PIP_MODE_EXITED =
			MainLoop::NOTIFICATION_APPLICATION_PIP_MODE_EXITED,
	};

	enum GroupCallFlags
	{
		GROUP_CALL_DEFAULT = 0,
		GROUP_CALL_REVERSE = 1,
		GROUP_CALL_DEFERRED = 2,
		GROUP_CALL_UNIQUE = 4,
	};

	static void initialize();
	static void finalize();
	_FORCE_INLINE_ static bool is_initialized() { return data != nullptr; }

	static void initialize_tree();
	static void finalize_tree();
	static void notification(int p_notification);

	static Window* get_root();

	static void notify_group_flags(
		uint32_t p_call_flags, const String& p_group, int p_notification);
	static void notify_group(const String& p_group, int p_notification);

	static void flush_transform_notifications();

	static bool is_accessibility_enabled();
	static bool is_accessibility_supported();
	static void _accessibility_force_update();
	static void _accessibility_notify_change(const Node* p_node, bool p_remove = false);
	static void _flush_accessibility_changes();
	static void _process_accessibility_changes(int p_window_id);

	static void iteration_prepare();
	static bool physics_process(double p_time);
	static void iteration_end();
	static bool process(double p_time);

	static bool is_auto_accept_quit();
	static void set_auto_accept_quit(bool p_enable);

	static bool is_quit_on_go_back();
	static void set_quit_on_go_back(bool p_enable);

	static void quit(int p_exit_code = EXIT_SUCCESS);

	_FORCE_INLINE_ static double get_physics_process_time()
	{
		return data ? data->physics_process_time : 0.0;
	}

	_FORCE_INLINE_ static double get_process_time() { return data ? data->process_time : 0.0; }

	static void set_pause(bool p_enabled);
	static bool is_paused();
	static void set_suspend(bool p_enabled);
	static bool is_suspended();

#ifdef DEBUG_ENABLED
	static void set_debug_collisions_hint(bool p_enabled);
	static bool is_debugging_collisions_hint();

	static void set_debug_paths_hint(bool p_enabled);
	static bool is_debugging_paths_hint();

	static void set_debug_navigation_hint(bool p_enabled);
	static bool is_debugging_navigation_hint();
#else
	static void set_debug_collisions_hint(bool p_enabled) { (void)p_enabled; }
	static bool is_debugging_collisions_hint() { return false; }

	static void set_debug_paths_hint(bool p_enabled) { (void)p_enabled; }
	static bool is_debugging_paths_hint() { return false; }

	static void set_debug_navigation_hint(bool p_enabled) { (void)p_enabled; }
	static bool is_debugging_navigation_hint() { return false; }
#endif

	static void set_debug_collisions_color(const Color& p_color);
	static Color get_debug_collisions_color();

	static void set_debug_collision_contact_color(const Color& p_color);
	static Color get_debug_collision_contact_color();

	static void set_debug_paths_color(const Color& p_color);
	static Color get_debug_paths_color();

	static void set_debug_paths_width(float p_width);
	static float get_debug_paths_width();

	static Ref<Material> get_debug_paths_material();
	static Ref<Material> get_debug_collision_material();
	static Ref<ArrayMesh> get_debug_contact_mesh();

	static int get_collision_debug_contact_count()
	{
		return data ? data->collision_debug_contacts : 0;
	}

	static int64_t get_frame();

	static int get_node_count();

	static Vector<Node*> get_nodes_in_group(const String& p_group);
	static Node* get_first_node_in_group(const String& p_group);
	static bool has_group(const String& p_identifier);
	static int get_node_count_in_group(const String& p_group);

	static void set_edited_scene_root(Node* p_node);
	static Node* get_edited_scene_root();

	static void set_current_scene(Node* p_scene);
	static Node* get_current_scene();
	static Error change_scene_to_file(const String& p_path);
	static Error change_scene_to_packed(PackedScene* rp_scene);
	static Error change_scene_to_node(Node* rp_node);
	static Error reload_current_scene();
	static void unload_current_scene();

	static SceneTreeTimer* create_timer(double p_delay_sec, bool p_process_always = true,
		bool p_process_in_physics = false, bool p_ignore_time_scale = false);
	static Tween* create_tween();
	static void remove_tween(const Ref<Tween>& p_tween);

	// used by Main::start, don't use otherwise
	static void add_current_scene(Node* p_current);

#ifdef TOOLS_ENABLED
	static void get_argument_options(
		const String& p_function, int p_idx, List<String>* r_options);
#endif

	// network API
	static MultiplayerAPI* get_multiplayer(const NodePath& p_for_path = NodePath());
	static void set_multiplayer(
		Ref<MultiplayerAPI> p_multiplayer, const NodePath& p_root_path = NodePath());
	static void set_multiplayer_poll_enabled(bool p_enabled);
	static bool is_multiplayer_poll_enabled();

	static void add_idle_callback(IdleCallback p_callback);

	static void set_disable_node_threading(bool p_disable);

	static void set_physics_interpolation_enabled(bool p_enabled);
	static bool is_physics_interpolation_enabled() { return _physics_interpolation_enabled; }

	// Different name to disambiguate fast static versions from the user bound versions.
	static bool is_fti_enabled() { return _physics_interpolation_enabled; }
	static bool is_fti_enabled_in_project() { return _physics_interpolation_enabled_in_project; }

#ifndef _3D_DISABLED
	static void client_physics_interpolation_add_node_3d(SelfList<Node3D>* p_elem);
	static void client_physics_interpolation_remove_node_3d(SelfList<Node3D>* p_elem);
#endif
};

// --- Folded from scene/main/instance_placeholder.h ---
#include "scene/main/node.h"

class PackedScene;

class InstancePlaceholder : public Node
{
	String path;

private:
	Node* try_get_node(
		InstancePlaceholder* p_placeholder, Node* p_instance, const NodePath& p_path);

public:
	void set_instance_path(const String& p_name);
	String get_instance_path() const;

	Node* create_instance(
		bool p_replace = false, const Ref<PackedScene>& p_custom_scene = Ref<PackedScene>());

	InstancePlaceholder();
};



// --- End scene/main/instance_placeholder.h ---

// --- Folded from scene/main/missing_node.h ---
#include "scene/main/node.h"

class MissingNode : public Node
{
	String original_class;
	String original_scene;
	bool recording_properties = false;
	bool recording_signals = false;

protected:

public:
	void set_original_class(const String& p_class);
	String get_original_class() const;

	void set_original_scene(const String& p_scene);
	String get_original_scene() const;

	void set_recording_properties(bool p_enable);
	bool is_recording_properties() const;

	void set_recording_signals(bool p_enable);
	bool is_recording_signals() const;

	virtual PackedStringArray get_configuration_warnings() const override;

	MissingNode();
};



// --- End scene/main/missing_node.h ---

// --- Folded from scene/main/timer.h ---
#include "scene/main/node.h"

class Timer : public Node
{
	double wait_time = 1.0;
	bool one_shot = false;
	bool autostart = false;
	bool processing = false;
	bool paused = false;
	bool ignore_time_scale = false;

	double time_left = -1.0;

public:
	enum TimerProcessCallback
	{
		TIMER_PROCESS_PHYSICS,
		TIMER_PROCESS_IDLE,
	};

	void set_wait_time(double p_time);
	double get_wait_time() const;

	void set_one_shot(bool p_one_shot);
	bool is_one_shot() const;

	void set_autostart(bool p_start);
	bool has_autostart() const;

	void stop();

	void set_paused(bool p_paused);
	bool is_paused() const;

	void set_ignore_time_scale(bool p_ignore);
	bool is_ignoring_time_scale();

	bool is_stopped() const;

	double get_time_left() const;

	PackedStringArray get_configuration_warnings() const override;

	void set_timer_process_callback(TimerProcessCallback p_callback);
	TimerProcessCallback get_timer_process_callback() const;

private:
	TimerProcessCallback timer_process_callback = TIMER_PROCESS_IDLE;
	void _set_process(bool p_process, bool p_force = false);
};

// --- Folded from modules/multiplayer/scene_multiplayer.h ---
#include "core/io/resource.h"
#include "core/io/net_socket.h"
#include "core/templates/rb_set.h"


class Node;
class MultiplayerSynchronizer;

class OfflineMultiplayerPeer final : public MultiplayerPeer
{
public:
	virtual int get_available_packet_count() const override { return 0; }

	virtual Error get_packet(const uint8_t** r_buffer, int& r_buffer_size) override
	{
		*r_buffer = nullptr;
		r_buffer_size = 0;
		return OK;
	}

	virtual Error put_packet(const uint8_t* p_buffer, int p_buffer_size) override { return OK; }

	virtual int get_max_packet_size() const override { return 0; }

	virtual void set_target_peer(int p_peer_id) override {}

	virtual int get_packet_peer() const override { return 0; }

	virtual TransferMode get_packet_mode() const override { return TRANSFER_MODE_RELIABLE; }

	virtual int get_packet_channel() const override { return 0; }

	virtual void disconnect_peer(int p_peer, bool p_force = false) override {}

	virtual bool is_server() const override { return true; }

	virtual void poll() override {}

	virtual void close() override {}

	virtual int get_unique_id() const override { return TARGET_PEER_SERVER; }

	virtual ConnectionStatus get_connection_status() const override { return CONNECTION_CONNECTED; }
};

class SceneReplicationConfig final : public Resource
{
public:
	enum ReplicationMode
	{
		REPLICATION_MODE_NEVER,
		REPLICATION_MODE_ALWAYS,
		REPLICATION_MODE_ON_CHANGE,
	};

private:
	struct ReplicationProperty
	{
		NodePath name;
		bool spawn = true;
		ReplicationMode mode = REPLICATION_MODE_ALWAYS;

		bool operator==(const ReplicationProperty& p_to) { return name == p_to.name; }

		ReplicationProperty() {}

		ReplicationProperty(const NodePath& p_name) { name = p_name; }
	};

	List<ReplicationProperty> properties;
	List<NodePath> spawn_props;
	List<NodePath> sync_props;
	List<NodePath> watch_props;
	bool dirty = false;

	void _update();

public:
	virtual void reset_state() override; // Required since we use variable amount of properties.

	void add_property(const NodePath& p_path, int p_index = -1);
	void remove_property(const NodePath& p_path);
	bool has_property(const NodePath& p_path) const;

	int property_get_index(const NodePath& p_path) const;
	bool property_get_spawn(const NodePath& p_path);
	void property_set_spawn(const NodePath& p_path, bool p_enabled);

	bool property_get_sync(const NodePath& p_path);
	void property_set_sync(const NodePath& p_path, bool p_enabled);

	bool property_get_watch(const NodePath& p_path);
	void property_set_watch(const NodePath& p_path, bool p_enabled);

	ReplicationMode property_get_replication_mode(const NodePath& p_path);
	void property_set_replication_mode(const NodePath& p_path, ReplicationMode p_mode);

	const List<NodePath>& get_spawn_properties();
	const List<NodePath>& get_sync_properties();
	const List<NodePath>& get_watch_properties();

	SceneReplicationConfig() {}
};

class SceneCacheInterface final : public RefCounted
{
private:
	MultiplayerAPI* multiplayer = nullptr;

	// path sent caches
	struct NodeCache
	{
		int cache_id = 0;
		HashMap<int, int> recv_ids;			// peer id, remote cache id
		HashMap<int, bool> confirmed_peers; // peer id, confirmed
	};

	struct RecvNode
	{
		NodePath path;
	};

	struct PeerInfo
	{
		HashMap<int, RecvNode> recv_nodes; // remote cache id, (ObjectID, NodePath)
	};

	HashMap<int, PeerInfo> peers_info;
	int last_send_cache_id = 1;

	NodeCache& _track(Node* p_node);

public:
	SceneCacheInterface(MultiplayerAPI* p_multiplayer) { multiplayer = p_multiplayer; }
};

class SceneReplicationInterface final : public RefCounted
{
private:
	struct TrackedNode
	{
		uint32_t net_id = 0;
		uint32_t remote_peer = 0;

		TrackedNode() {}
	};

	// Replication state.
	uint32_t last_net_id = 0;

	int pending_spawn_remote = 0;
	const uint8_t* pending_buffer = nullptr;
	int pending_buffer_size = 0;
	List<uint32_t> pending_sync_net_ids;

	// Replicator config.
	MultiplayerAPI* multiplayer = nullptr;
	SceneCacheInterface* multiplayer_cache = nullptr;
	PackedByteArray packet_cache;
	int sync_mtu = 1350; // Highly dependent on underlying protocol.
	int delta_mtu = 65535;

	bool _has_authority(const Node* p_node);
	bool _verify_synchronizer(int p_peer, MultiplayerSynchronizer* p_sync, uint32_t& r_net_id);
	MultiplayerSynchronizer* _find_synchronizer(int p_peer, uint32_t p_net_ida);

	Error _make_spawn_packet(Node* p_node, MultiplayerSynchronizer* p_spawner, int& r_len);
	Error _make_despawn_packet(Node* p_node, int& r_len);
	Error _send_raw(const uint8_t* p_buffer, int p_size, int p_peer, bool p_reliable);

	Error _update_sync_visibility(int p_peer, MultiplayerSynchronizer* p_sync);

public:
	static void make_default();

	void on_reset();
	void on_peer_change(int p_id, bool p_connected);

	void on_network_process();

	Error on_spawn_receive(int p_from, const uint8_t* p_buffer, int p_buffer_len);
	Error on_despawn_receive(int p_from, const uint8_t* p_buffer, int p_buffer_len);
	Error on_sync_receive(int p_from, const uint8_t* p_buffer, int p_buffer_len);
	Error on_delta_receive(int p_from, const uint8_t* p_buffer, int p_buffer_len);

	void set_max_sync_packet_size(int p_size);
	int get_max_sync_packet_size() const;

	void set_max_delta_packet_size(int p_size);
	int get_max_delta_packet_size() const;

	SceneReplicationInterface(MultiplayerAPI* p_multiplayer, SceneCacheInterface* p_cache)
	{
		multiplayer = p_multiplayer;
		multiplayer_cache = p_cache;
	}
};

class SceneRPCInterface final : public RefCounted
{
private:
	struct RPCConfig
	{
		String name;
		MultiplayerAPI::RPCMode rpc_mode = MultiplayerAPI::RPC_MODE_DISABLED;
		bool call_local = false;
		MultiplayerPeer::TransferMode transfer_mode = MultiplayerPeer::TRANSFER_MODE_RELIABLE;
		int channel = 0;

		bool operator==(const RPCConfig& p_other) const { return name == p_other.name; }
	};

	struct RPCConfigCache
	{
		HashMap<uint16_t, RPCConfig> configs;
		HashMap<String, uint16_t> ids;
	};

	struct SortRPCConfig
	{
		bool operator()(const RPCConfig& p_a, const RPCConfig& p_b) const
		{
			return p_a.name < p_b.name;
		}
	};

	enum NetworkNodeIdCompression
	{
		NETWORK_NODE_ID_COMPRESSION_8 = 0,
		NETWORK_NODE_ID_COMPRESSION_16,
		NETWORK_NODE_ID_COMPRESSION_32,
	};

	enum NetworkNameIdCompression
	{
		NETWORK_NAME_ID_COMPRESSION_8 = 0,
		NETWORK_NAME_ID_COMPRESSION_16,
	};

	MultiplayerAPI* multiplayer = nullptr;
	SceneCacheInterface* multiplayer_cache = nullptr;
	SceneReplicationInterface* multiplayer_replicator = nullptr;

	Vector<uint8_t> packet_cache;

protected:
	void _process_rpc(Node* p_node, const uint16_t p_rpc_method_id, int p_from,
		const uint8_t* p_packet, int p_packet_len, int p_offset);

	Node* _process_get_node(
		int p_from, const uint8_t* p_packet, uint32_t p_node_target, int p_packet_len);

	const RPCConfigCache& _get_node_config(const Node* p_node);

public:
	void process_rpc(int p_from, const uint8_t* p_packet, int p_packet_len);

	SceneRPCInterface(MultiplayerAPI* p_multiplayer, SceneCacheInterface* p_cache,
		SceneReplicationInterface* p_replicator)
	{
		multiplayer = p_multiplayer;
		multiplayer_cache = p_cache;
		multiplayer_replicator = p_replicator;
	}
};

class MultiplayerDebugger final
{
public:
	struct RPCNodeInfo
	{
		String node_path;
		int incoming_rpc = 0;
		int incoming_size = 0;
		int outgoing_rpc = 0;
		int outgoing_size = 0;
	};

	struct RPCFrame
	{
		Vector<RPCNodeInfo> infos;
	};

	struct SyncInfo
	{
		int incoming_syncs = 0;
		int incoming_size = 0;
		int outgoing_syncs = 0;
		int outgoing_size = 0;

		SyncInfo() {}

		SyncInfo(MultiplayerSynchronizer* p_sync);
	};

private:
	class BandwidthProfiler
	{
	protected:
		struct BandwidthFrame
		{
			uint32_t timestamp;
			int packet_size;
		};

		int bandwidth_in_ptr = 0;
		Vector<BandwidthFrame> bandwidth_in;
		int bandwidth_out_ptr = 0;
		Vector<BandwidthFrame> bandwidth_out;
		uint64_t last_bandwidth_time = 0;

		int bandwidth_usage(const Vector<BandwidthFrame>& p_buffer, int p_pointer);
	};

	class RPCProfiler
	{
	private:
		uint64_t last_profile_time = 0;
	};

	class ReplicationProfiler
	{
	private:
		uint64_t last_profile_time = 0;
	};

public:
	static void initialize();
	static void deinitialize();
};

struct MultiplayerAPI::SceneData
{
	struct PendingPeer
	{
		bool local = false;
		bool remote = false;
		uint64_t time = 0;
	};

	Ref<MultiplayerPeer> multiplayer_peer;
	MultiplayerPeer::ConnectionStatus last_connection_status =
		MultiplayerPeer::CONNECTION_DISCONNECTED;
	HashMap<int, PendingPeer> pending_peers; // true if locally finalized.
	uint64_t auth_timeout = 3000;
	HashSet<int> connected_peers;
	int remote_sender_id = 0;
	int remote_sender_override = 0;

	Vector<uint8_t> packet_cache;

	NodePath root_path;
	bool allow_object_decoding = false;
	bool server_relay = true;
	Ref<StreamPeerBuffer> relay_buffer;

	Ref<SceneCacheInterface> cache;
	Ref<SceneReplicationInterface> replicator;
	Ref<SceneRPCInterface> rpc;
};

// --- Folded from modules/multiplayer/multiplayer_synchronizer.h ---
#include "core/templates/local_vector.h"
#include "scene/main/node.h"


class MultiplayerSynchronizer final : public Node
{
public:
	enum Mode
	{
		MODE_SYNCHRONIZER,
		MODE_SPAWNER,
	};

	enum VisibilityUpdateMode
	{
		VISIBILITY_PROCESS_IDLE,
		VISIBILITY_PROCESS_PHYSICS,
		VISIBILITY_PROCESS_NONE,
	};

	enum
	{
		INVALID_ID = 0xFF,
	};

private:
	struct Watcher
	{
		NodePath prop;
		uint64_t last_change_usec = 0;
	};

	struct SpawnableScene
	{
		String path;
		Ref<PackedScene> cache;
	};

	struct SpawnInfo
	{
		int id = INVALID_ID;

		SpawnInfo() {}
	};

	Mode mode = MODE_SYNCHRONIZER;

	// Synchronizer state.
	Ref<SceneReplicationConfig> replication_config;
	NodePath root_path = NodePath(".."); // Start with parent, like with AnimationPlayer.
	uint64_t sync_interval_usec = 0;
	uint64_t delta_interval_usec = 0;
	VisibilityUpdateMode visibility_update_mode = VISIBILITY_PROCESS_IDLE;
	HashSet<int> peer_visibility;
	Vector<Watcher> watchers;
	uint64_t last_watch_usec = 0;

	uint64_t last_sync_usec = 0;
	uint16_t last_inbound_sync = 0;
	uint32_t net_id = 0;
	bool sync_started = false;

	// Spawner state.
	LocalVector<SpawnableScene> spawnable_scenes;
	HashSet<ResourceUID::ID> spawnable_ids;
	NodePath spawn_path;
	uint32_t spawn_limit = 0;

	void _start();
	void _stop();
	void _update_process();
	Error _watch_changes(uint64_t p_usec);

	void _update_spawn_node();
	void _node_added(Node* p_node);

	Vector<String> _get_spawnable_scenes() const;
	void _set_spawnable_scenes(const Vector<String>& p_scenes);

protected:
	void _notification(int p_what);

public:
	void set_mode(Mode p_mode) { mode = p_mode; }
	Mode get_mode() const { return mode; }

	void reset();
	Node* get_root_node();

	uint32_t get_net_id() const;
	void set_net_id(uint32_t p_net_id);

	bool update_outbound_sync_time(uint64_t p_usec);
	bool update_inbound_sync_time(uint16_t p_network_time);

	PackedStringArray get_configuration_warnings() const override;

	void set_replication_interval(double p_interval);
	double get_replication_interval() const;

	void set_delta_interval(double p_interval);
	double get_delta_interval() const;

	void set_replication_config(Ref<SceneReplicationConfig> p_config);
	Ref<SceneReplicationConfig> get_replication_config();

	NodePath get_root_path() const;
	virtual void set_multiplayer_authority(int p_peer_id, bool p_recursive = true) override;

	bool is_visibility_public() const;
	void set_visibility_public(bool p_public);
	bool is_visible_to(int p_peer);
	void set_visibility_for(int p_peer, bool p_visible);
	bool get_visibility_for(int p_peer) const;
	void update_visibility(int p_for_peer);
	void set_visibility_update_mode(VisibilityUpdateMode p_mode);
	VisibilityUpdateMode get_visibility_update_mode() const;

	List<NodePath> get_delta_properties(uint64_t p_indexes);
	SceneReplicationConfig* get_replication_config_ptr() const;

	// Spawner API.
	void add_spawnable_scene(const String& p_path);
	int get_spawnable_scene_count() const;
	String get_spawnable_scene(int p_idx) const;
	void clear_spawnable_scenes();

	NodePath get_spawn_path() const;

	uint32_t get_spawn_limit() const { return spawn_limit; }
	void set_spawn_limit(uint32_t p_limit) { spawn_limit = p_limit; }

	int find_spawnable_scene_index_from_path(const String& p_path) const;
	Node* instantiate_scene(int p_idx);

	MultiplayerSynchronizer();
};

// --- End modules/multiplayer/multiplayer_synchronizer.h ---
// --- End modules/multiplayer/scene_multiplayer.h ---

// --- Folded from scene/2d/node_2d.h ---
#include "scene/gui/control.h"
#include "scene/resources/material.h"

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
#include "servers/display/display_server.h"

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
#include "core/templates/hash_map.h"
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

// --- Folded from scene/2d/line_2d.h ---

#include "scene/resources/material.h"

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

#include "scene/resources/material.h"

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

#include "scene/main/node.h"
#include "servers/physics_2d/physics_server_2d.h"

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
#include "core/templates/hash_map.h"
#include "core/templates/rb_set.h"
#include "scene/gui/control.h"
#include "scene/resources/material.h"
#include "scene/main/node.h"
#include "scene/resources/material.h"

#ifndef PHYSICS_2D_DISABLED

#endif // PHYSICS_2D_DISABLED

#ifndef NAVIGATION_2D_DISABLED
#include "servers/physics_2d/physics_server_2d.h"
#endif // NAVIGATION_2D_DISABLED

#ifndef DISABLE_DEPRECATED
#include "scene/resources/material.h"
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

#include "scene/resources/texture.h"

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
// --- End scene/2d/line_2d.h ---
// --- End scene/2d/node_2d.h ---

// --- Folded from scene/3d/node_3d.h ---
#include "scene/main/node.h"
// --- Inlined/Folded from scene/resources/3d/world_3d.h ---
#include "core/io/resource.h"
#include "scene/resources/material.h"

#ifndef PHYSICS_3D_DISABLED
#include "servers/physics_3d/physics_server_3d.h"
#endif // PHYSICS_3D_DISABLED

class CameraAttributes;
class Camera3D;
class Compositor;
class VisibleOnScreenNotifier3D;
struct SpatialIndexer;

class World3D : public Resource
{
private:
	RID scenario;
	mutable RID space;
#ifndef NAVIGATION_3D_DISABLED
	mutable RID navigation_map;
#endif // NAVIGATION_3D_DISABLED

	Ref<Environment> environment;
	Ref<Environment> fallback_environment;
	Ref<CameraAttributes> camera_attributes;
	Ref<Compositor> compositor;

	HashSet<Camera3D*> cameras;

protected:

	friend class Camera3D;

	void _register_camera(Camera3D* p_camera);
	void _remove_camera(Camera3D* p_camera);

public:
	RID get_space() const;
#ifndef NAVIGATION_3D_DISABLED
	RID get_navigation_map() const;
#endif // NAVIGATION_3D_DISABLED
	RID get_scenario() const;

	void set_environment(const Ref<Environment>& p_environment);
	Ref<Environment> get_environment() const;

	void set_fallback_environment(const Ref<Environment>& p_environment);
	Ref<Environment> get_fallback_environment() const;

	void set_camera_attributes(const Ref<CameraAttributes>& p_camera_attributes);
	Ref<CameraAttributes> get_camera_attributes() const;

	void set_compositor(const Ref<Compositor>& p_compositor);
	Ref<Compositor> get_compositor() const;

	_FORCE_INLINE_ const HashSet<Camera3D*>& get_cameras() const { return cameras; }

#ifndef PHYSICS_3D_DISABLED
	PhysicsDirectSpaceState3D* get_direct_space_state();
#endif // PHYSICS_3D_DISABLED

	World3D();
	~World3D();
};
#include "scene/resources/material.h"
#include "scene/resources/texture.h"

struct CollisionState3D;

class Node3DGizmo : public RefCounted
{
public:
	virtual void create() = 0;
	virtual void transform() = 0;
	virtual void clear() = 0;
	virtual void redraw() = 0;
	virtual void free() = 0;

	Node3DGizmo();

	virtual ~Node3DGizmo() {}
};

class Node3D : public Node
{
	friend class SceneTreeFTI;
	friend class SceneTreeFTITests;
	friend struct RenderInstance3D;

public:
	// Edit mode for the rotation.
	// THIS MODE ONLY AFFECTS HOW DATA IS EDITED AND SAVED
	// IT DOES _NOT_ AFFECT THE TRANSFORM LOGIC (see comment in TransformDirty).
	enum RotationEditMode
	{
		ROTATION_EDIT_MODE_EULER,
		ROTATION_EDIT_MODE_QUATERNION,
		ROTATION_EDIT_MODE_BASIS,
	};

private:
	// For the sake of ease of use, Node3D can operate with Transforms (Basis+Origin),
	// Quaternion/Scale and Euler Rotation/Scale. Transform and Quaternion are stored in
	// data.local_transform Basis (so quaternion is not really stored, but converted back/forth from
	// 3x3 matrix on demand). Euler needs to be kept separate because converting to Basis and back
	// may result in a different vector (which is troublesome for users editing in the inspector,
	// not only because of the numerical precision loss but because they expect these rotations to
	// be consistent, or support "redundant" rotations for animation interpolation, like going from
	// 0 to 720 degrees).
	//
	// As such, the system works in a way where if the local transform is set (via
	// transform/basis/quaternion), the EULER rotation and scale becomes dirty. It will remain dirty
	// until reading back is attempted (for performance reasons). Likewise, if the Euler rotation
	// scale are set, the local transform will become dirty (and again, will not become valid again
	// until read).
	//
	// All this is transparent from outside the Node3D API, which allows everything to works by
	// calling these functions in exchange.
	//
	// Additionally, setting either transform, quaternion, Euler rotation or scale makes the global
	// transform dirty, which will be updated when read again.
	//
	// NOTE: Again, RotationEditMode is _independent_ of this mechanism, it is only meant to expose
	// the right set of properties for editing (editor) and saving (to scene, in order to keep the
	// same values and avoid data loss on conversions). It has zero influence in the logic described
	// above.
	enum TransformDirty
	{
		DIRTY_NONE = 0,
		DIRTY_EULER_ROTATION_AND_SCALE = 1,
		DIRTY_LOCAL_TRANSFORM = 2,
		DIRTY_GLOBAL_TRANSFORM = 4,
		DIRTY_GLOBAL_INTERPOLATED_TRANSFORM = 8,
	};

	struct ClientPhysicsInterpolationData
	{
		Transform3D global_xform_curr;
		Transform3D global_xform_prev;
		uint64_t current_physics_tick = 0;
		uint64_t timeout_physics_tick = 0;
	};

	mutable SelfList<Node> xform_change;
	SelfList<Node3D> _client_physics_interpolation_node_3d_list;

	// This Data struct is to avoid namespace pollution in derived classes.

	struct Data
	{
		// Interpolated global transform - correct on the frame only.
		// Only used with FTI.
		Transform3D global_transform_interpolated;

		// Current xforms are either
		// * Used for everything (when not using FTI)
		// * Correct on the physics tick (when using FTI)
		mutable Transform3D global_transform;
		mutable Transform3D local_transform;

		// Only used with FTI.
		Transform3D local_transform_prev;

		mutable EulerOrder euler_rotation_order = EulerOrder::YXZ;
		mutable Vector3 euler_rotation;
		mutable Vector3 scale = Vector3(1, 1, 1);
		mutable RotationEditMode rotation_edit_mode = ROTATION_EDIT_MODE_EULER;

		mutable MTNumeric<uint32_t> dirty;

		Viewport* viewport = nullptr;

		bool top_level : 1;
		bool inside_world : 1;

		// This is cached, and only currently kept up to date in visual instances.
		// This is set if a visual instance is (a) in the tree AND (b) visible via
		// is_visible_in_tree() call.
		bool vi_visible : 1;

		bool ignore_notification : 1;
		bool notify_local_transform : 1;
		bool notify_transform : 1;

		bool visible : 1;
		bool disable_scale : 1;

		// Scene tree interpolation.
		bool fti_on_frame_xform_list : 1;
		bool fti_on_frame_property_list : 1;
		bool fti_on_tick_xform_list : 1;
		bool fti_on_tick_property_list : 1;
		bool fti_global_xform_interp_set : 1;
		bool fti_frame_xform_force_update : 1;
		bool fti_is_identity_xform : 1;
		bool fti_processed : 1;

		RID visibility_parent;

		Node3D* parent = nullptr;

		// An unordered vector of `Spatial` children only.
		// This is a subset of the `Node::children`, purely
		// an optimization for faster traversal.
		LocalVector<Node3D*> node3d_children;
		uint32_t index_in_parent = UINT32_MAX;

		ClientPhysicsInterpolationData* client_physics_interpolation_data = nullptr;

#ifdef TOOLS_ENABLED
		Vector<Ref<Node3DGizmo>> gizmos;
		bool gizmos_requested : 1;
		bool gizmos_disabled : 1;
		bool gizmos_dirty : 1;
		bool transform_gizmo_visible : 1;
#endif

	} data;

	NodePath visibility_parent_path;

	_FORCE_INLINE_ uint32_t _read_dirty_mask() const
	{
		return is_group_processing() ? data.dirty.mt.get() : data.dirty.st;
	}

	_FORCE_INLINE_ bool _test_dirty_bits(uint32_t p_bits) const
	{
		return (is_group_processing() ? data.dirty.mt.get() : data.dirty.st) & p_bits;
	}

	void _replace_dirty_mask(uint32_t p_mask) const;
	void _set_dirty_bits(uint32_t p_bits) const;
	void _clear_dirty_bits(uint32_t p_bits) const;

	void _update_gizmos();
	void _notify_dirty();
	void _propagate_transform_changed(Node3D* p_origin);

	void _propagate_visibility_changed();

	void _propagate_visibility_parent();
	void _update_visibility_parent(bool p_update_root);
	void _propagate_transform_changed_deferred();

protected:
	_FORCE_INLINE_ void set_ignore_transform_notification(bool p_ignore)
	{
		data.ignore_notification = p_ignore;
	}

	_FORCE_INLINE_ void _update_local_transform() const;
	_FORCE_INLINE_ void _update_rotation_and_scale() const;

	void _set_vi_visible(bool p_visible) { data.vi_visible = p_visible; }

	bool _is_vi_visible() const { return data.vi_visible; }

	Transform3D _get_global_transform_interpolated(real_t p_interpolation_fraction);

	const Transform3D& _get_cached_global_transform_interpolated() const
	{
		return data.global_transform_interpolated;
	}

	void _disable_client_physics_interpolation();

	// Calling this announces to the FTI system that a node has been moved,
	// or requires an update in terms of interpolation
	// (e.g. changing Camera zoom even if position hasn't changed).
	void fti_notify_node_changed(bool p_transform_changed = true);

	// Opportunity after FTI to update the servers
	// with global_transform_interpolated,
	// and any custom interpolated data in derived classes.
	// Make sure to call the parent class fti_update_servers(),
	// so all data is updated to the servers.
	virtual void fti_update_servers_xform() {}

	virtual void fti_update_servers_property() {}

	// Pump the FTI data, also gives a chance for inherited classes
	// to pump custom data, but they *must* call the base class here too.
	// This is the opportunity for classes to move current values for
	// transforms and properties to stored previous values,
	// and this should take place both on ticks, and during resets.
	virtual void fti_pump_xform();

	virtual void fti_pump_property() {}

	void _notification(int p_what);
	static void _bind_methods();

	bool _property_can_revert(const String& p_name) const;

public:
	enum
	{
		NOTIFICATION_TRANSFORM_CHANGED = 2000, // Keep in sync with SceneTree.
		NOTIFICATION_ENTER_WORLD = 41,
		NOTIFICATION_EXIT_WORLD = 42,
		NOTIFICATION_VISIBILITY_CHANGED = 43,
		NOTIFICATION_LOCAL_TRANSFORM_CHANGED = 44,
	};

	Node3D* get_parent_node_3d() const;

	Ref<World3D> get_world_3d() const;

	void set_position(const Vector3& p_position);

	void set_rotation_edit_mode(RotationEditMode p_mode);
	RotationEditMode get_rotation_edit_mode() const;

	void set_rotation_order(EulerOrder p_order);
	void set_rotation(const Vector3& p_euler_rad);
	void set_rotation_degrees(const Vector3& p_euler_degrees);
	void set_scale(const Vector3& p_scale);

	void set_global_position(const Vector3& p_position);
	void set_global_basis(const Basis& p_basis);
	void set_global_rotation(const Vector3& p_euler_rad);
	void set_global_rotation_degrees(const Vector3& p_euler_degrees);

	Vector3 get_position() const;

	EulerOrder get_rotation_order() const;
	Vector3 get_rotation() const;
	Vector3 get_rotation_degrees() const;
	Vector3 get_scale() const;

	Vector3 get_global_position() const;
	Basis get_global_basis() const;
	Vector3 get_global_rotation() const;
	Vector3 get_global_rotation_degrees() const;

	void set_transform(const Transform3D& p_transform);
	void set_basis(const Basis& p_basis);
	void set_quaternion(const Quaternion& p_quaternion);
	void set_global_transform(const Transform3D& p_transform);

	Transform3D get_transform() const;
	Basis get_basis() const;
	Quaternion get_quaternion() const;
	Transform3D get_global_transform() const;

	Transform3D get_global_transform_interpolated();
	bool update_client_physics_interpolation_data();

#ifdef TOOLS_ENABLED
	virtual Transform3D get_global_gizmo_transform() const;
	virtual Transform3D get_local_gizmo_transform() const;

	virtual void set_transform_gizmo_visible(bool p_enabled)
	{
		data.transform_gizmo_visible = p_enabled;
	}

	virtual bool is_transform_gizmo_visible() const { return data.transform_gizmo_visible; }
#endif
	void reparent(Node* p_parent, bool p_keep_global_transform);
	void set_disable_gizmos(bool p_enabled);
	void update_gizmos();
	void set_subgizmo_selection(
		Ref<Node3DGizmo> p_gizmo, int p_id, Transform3D p_transform = Transform3D());
	void clear_subgizmo_selection();
	Vector<Ref<Node3DGizmo>> get_gizmos() const;
	void add_gizmo(Ref<Node3DGizmo> p_gizmo);
	void remove_gizmo(Ref<Node3DGizmo> p_gizmo);
	void clear_gizmos();

	void set_as_top_level(bool p_enabled);
	void set_as_top_level_keep_local(bool p_enabled);
	bool is_set_as_top_level() const;

	void set_disable_scale(bool p_enabled);
	bool is_scale_disabled() const;

	_FORCE_INLINE_ bool is_inside_world() const { return data.inside_world; }

	Transform3D get_relative_transform(const Node* p_parent) const;

	void rotate(const Vector3& p_axis, real_t p_angle);
	void rotate_x(real_t p_angle);
	void rotate_y(real_t p_angle);
	void rotate_z(real_t p_angle);
	void translate(const Vector3& p_offset);
	void scale(const Vector3& p_ratio);

	void rotate_object_local(const Vector3& p_axis, real_t p_angle);
	void scale_object_local(const Vector3& p_scale);
	void translate_object_local(const Vector3& p_offset);

	void global_rotate(const Vector3& p_axis, real_t p_angle);
	void global_scale(const Vector3& p_scale);
	void global_translate(const Vector3& p_offset);

	void look_at(
		const Vector3& p_target, const Vector3& p_up = Vector3::UP, bool p_use_model_front = false);
	void look_at_from_position(const Vector3& p_pos, const Vector3& p_target,
		const Vector3& p_up = Vector3::UP, bool p_use_model_front = false);

	Vector3 to_local(Vector3 p_global) const;
	Vector3 to_global(Vector3 p_local) const;

	void set_notify_transform(bool p_enabled);
	bool is_transform_notification_enabled() const;

	void set_notify_local_transform(bool p_enabled);
	bool is_local_transform_notification_enabled() const;

	void orthonormalize();
	void set_identity();

	void set_visible(bool p_visible);
	void show();
	void hide();
	bool is_visible() const;
	bool is_visible_in_tree() const;

	void force_update_transform();

	void set_visibility_parent(const NodePath& p_path);
	NodePath get_visibility_parent() const;

	virtual bool has_render_instance() const { return false; }
	virtual CollisionState3D* get_collision_state_3d() { return nullptr; }
	virtual const CollisionState3D* get_collision_state_3d() const { return nullptr; }

	Node3D() = default;
	~Node3D();
};

// --- Folded from scene/3d/navigation/navigation_3d.h ---
#include "scene/main/node.h"
#include "scene/resources/mesh.h"
#include "servers/physics_3d/physics_server_3d.h"

class ArrayMesh;
class NavigationMeshSourceGeometryData3D;
class StandardMaterial3D;

class NavigationAgent3D final : public Node
{
	Node3D* agent_parent = nullptr;

	RID agent;
	RID map_override;

	bool avoidance_enabled = false;
	bool use_3d_avoidance = false;
	uint32_t avoidance_layers = 1;
	uint32_t avoidance_mask = 1;
	real_t avoidance_priority = 1.0;
	uint32_t navigation_layers = 1;
	NavigationPathQueryParameters3D::PathfindingAlgorithm pathfinding_algorithm =
		NavigationPathQueryParameters3D::PathfindingAlgorithm::PATHFINDING_ALGORITHM_ASTAR;
	NavigationPathQueryParameters3D::PathPostProcessing path_postprocessing =
		NavigationPathQueryParameters3D::PathPostProcessing::PATH_POSTPROCESSING_CORRIDORFUNNEL;
	uint32_t path_metadata_flags =
		NavigationPathQueryParameters3D::PathMetadataFlags::PATH_METADATA_INCLUDE_ALL;

	real_t path_desired_distance = 1.0;
	real_t target_desired_distance = 1.0;
	real_t height = NavigationDefaults3D::AVOIDANCE_AGENT_HEIGHT;
	real_t radius = NavigationDefaults3D::AVOIDANCE_AGENT_RADIUS;
	real_t path_height_offset = 0.0;
	real_t neighbor_distance = NavigationDefaults3D::AVOIDANCE_AGENT_NEIGHBOR_DISTANCE;
	int max_neighbors = NavigationDefaults3D::AVOIDANCE_AGENT_MAX_NEIGHBORS;
	real_t time_horizon_agents = NavigationDefaults3D::AVOIDANCE_AGENT_TIME_HORIZON_AGENTS;
	real_t time_horizon_obstacles = NavigationDefaults3D::AVOIDANCE_AGENT_TIME_HORIZON_OBSTACLES;
	real_t max_speed = NavigationDefaults3D::AVOIDANCE_AGENT_MAX_SPEED;
	real_t path_max_distance = 5.0;
	bool simplify_path = false;
	real_t simplify_epsilon = 0.0;
	float path_return_max_length = 0.0;
	float path_return_max_radius = 0.0;
	int path_search_max_polygons = NavigationDefaults3D::path_search_max_polygons;
	float path_search_max_distance = 0.0;

	Vector3 target_position;

	Ref<NavigationPathQueryParameters3D> navigation_query;
	Ref<NavigationPathQueryResult3D> navigation_result;
	int navigation_path_index = 0;

	Vector3 safe_velocity;

	Vector3 velocity;
	bool velocity_submitted = false;

	Vector3 velocity_forced;
	bool velocity_forced_submitted = false;

	bool keep_y_velocity = true;
	float stored_y_velocity = 0.0;

	bool target_position_submitted = false;
	bool target_reached = false;
	bool navigation_finished = true;
	bool last_waypoint_reached = false;

	bool debug_enabled = false;
	float debug_path_custom_point_size = 4.0;
	bool debug_use_custom = false;
	Color debug_path_custom_color = Color(1.0, 1.0, 1.0, 1.0);
#ifdef DEBUG_ENABLED
	bool debug_path_dirty = true;
	RID debug_path_instance;
	Ref<ArrayMesh> debug_path_mesh;
	Ref<StandardMaterial3D> debug_agent_path_line_custom_material;
	Ref<StandardMaterial3D> debug_agent_path_point_custom_material;
#endif // DEBUG_ENABLED

protected:
	void _notification(int p_what);

public:
	NavigationAgent3D();
	~NavigationAgent3D();

	RID get_rid() const { return agent; }

	void set_avoidance_enabled(bool p_enabled);
	bool get_avoidance_enabled() const;

	void set_agent_parent(Node* p_agent_parent);

	void set_navigation_layers(uint32_t p_navigation_layers);
	uint32_t get_navigation_layers() const;

	void set_navigation_layer_value(int p_layer_number, bool p_value);
	bool get_navigation_layer_value(int p_layer_number) const;

	void set_pathfinding_algorithm(
		const NavigationPathQueryParameters3D::PathfindingAlgorithm p_pathfinding_algorithm);

	NavigationPathQueryParameters3D::PathfindingAlgorithm get_pathfinding_algorithm() const
	{
		return pathfinding_algorithm;
	}

	void set_path_postprocessing(
		const NavigationPathQueryParameters3D::PathPostProcessing p_path_postprocessing);

	NavigationPathQueryParameters3D::PathPostProcessing get_path_postprocessing() const
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

	void set_height(real_t p_height);

	real_t get_height() const { return height; }

	void set_path_height_offset(real_t p_path_height_offset);

	real_t get_path_height_offset() const { return path_height_offset; }

	void set_use_3d_avoidance(bool p_use_3d_avoidance);

	bool get_use_3d_avoidance() const { return use_3d_avoidance; }

	void set_keep_y_velocity(bool p_enabled);
	bool get_keep_y_velocity() const;

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

	void set_target_position(Vector3 p_position);
	Vector3 get_target_position() const;

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

	Vector3 get_next_path_position();

	Ref<NavigationPathQueryResult3D> get_current_navigation_result() const
	{
		return navigation_result;
	}

	const Vector<Vector3>& get_current_navigation_path() const
	{
		return navigation_result->get_path();
	}

	int get_current_navigation_path_index() const { return navigation_path_index; }

	real_t distance_to_target() const;
	bool is_target_reached() const;
	bool is_target_reachable();
	bool is_navigation_finished();
	Vector3 get_final_position();

	void set_velocity(const Vector3 p_velocity);

	Vector3 get_velocity() { return velocity; }

	void set_velocity_forced(const Vector3 p_velocity);

	void _avoidance_done(Vector3 p_new_velocity);

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

private:
	bool _is_target_reachable() const;
	Vector3 _get_final_position() const;

	void _update_navigation();
	void _advance_waypoints(const Vector3& p_origin);
	void _request_repath();

	bool _is_last_waypoint() const;
	void _move_to_next_waypoint();
	bool _is_within_waypoint_distance(const Vector3& p_origin) const;
	bool _is_within_target_distance(const Vector3& p_origin) const;

	void _trigger_waypoint_reached();
	void _transition_to_navigation_finished();
	void _transition_to_target_reached();

#ifdef DEBUG_ENABLED
	void _navigation_debug_changed();
	void _update_debug_path();
#endif // DEBUG_ENABLED
};

class NavigationLink3D final : public Node3D
{
	bool enabled = true;
	RID link;
	RID map_override;
	bool bidirectional = true;
	uint32_t navigation_layers = 1;
	Vector3 end_position;
	Vector3 start_position;
	real_t enter_cost = 0.0;
	real_t travel_cost = 1.0;

#ifdef DEBUG_ENABLED
	RID debug_instance;
	Ref<ArrayMesh> debug_mesh;

	void _update_debug_mesh();
#endif // DEBUG_ENABLED

protected:
	void _notification(int p_what);

public:
	NavigationLink3D();
	~NavigationLink3D();

	RID get_rid() const;

	bool is_enabled() const { return enabled; }

	void set_navigation_map(RID p_navigation_map);
	RID get_navigation_map() const;

	bool is_bidirectional() const { return bidirectional; }

	void set_navigation_layers(uint32_t p_navigation_layers);

	uint32_t get_navigation_layers() const { return navigation_layers; }

	void set_navigation_layer_value(int p_layer_number, bool p_value);
	bool get_navigation_layer_value(int p_layer_number) const;

	Vector3 get_start_position() const { return start_position; }

	Vector3 get_end_position() const { return end_position; }

	Vector3 get_global_start_position() const;

	Vector3 get_global_end_position() const;

	void set_enter_cost(real_t p_enter_cost);

	real_t get_enter_cost() const { return enter_cost; }

	void set_travel_cost(real_t p_travel_cost);

	real_t get_travel_cost() const { return travel_cost; }

	PackedStringArray get_configuration_warnings() const override;

private:
	void _link_enter_navigation_map();
	void _link_exit_navigation_map();
	void _link_update_transform();
};

class NavigationObstacle3D final : public Node3D
{
	RID obstacle;
	RID map_before_pause;
	RID map_override;
	RID map_current;

	real_t height = 1.0;
	real_t radius = 0.0;

	Vector<Vector3> vertices;
	bool vertices_are_clockwise = true;
	bool vertices_are_valid = true;

	bool avoidance_enabled = true;
	uint32_t avoidance_layers = 1;

	bool use_3d_avoidance = false;

	Vector3 velocity;
	Vector3 previous_velocity;
	bool velocity_submitted = false;

	bool affect_navigation_mesh = false;
	bool carve_navigation_mesh = false;

#ifdef DEBUG_ENABLED
	RID fake_agent_radius_debug_instance_rid;
	RID fake_agent_radius_debug_mesh_rid;

	RID static_obstacle_debug_instance_rid;
	RID static_obstacle_debug_mesh_rid;

private:
	void _update_debug();
	void _update_fake_agent_radius_debug();
	void _update_static_obstacle_debug();
	void _clear_debug();
#endif // DEBUG_ENABLED

protected:
	void _notification(int p_what);

public:
	NavigationObstacle3D();
	~NavigationObstacle3D();

	RID get_rid() const { return obstacle; }

	void set_avoidance_enabled(bool p_enabled);
	bool get_avoidance_enabled() const;

	void set_navigation_map(RID p_navigation_map);
	RID get_navigation_map() const;

	void set_radius(real_t p_radius);

	real_t get_radius() const { return radius; }

	void set_height(real_t p_height);

	real_t get_height() const { return height; }

	void set_vertices(const Vector<Vector3>& p_vertices);

	const Vector<Vector3>& get_vertices() const { return vertices; }

	bool are_vertices_clockwise() const { return vertices_are_clockwise; }

	bool are_vertices_valid() const { return vertices_are_valid; }

	void set_avoidance_layers(uint32_t p_layers);
	uint32_t get_avoidance_layers() const;

	void set_avoidance_layer_value(int p_layer_number, bool p_value);
	bool get_avoidance_layer_value(int p_layer_number) const;

	void set_use_3d_avoidance(bool p_use_3d_avoidance);

	bool get_use_3d_avoidance() const { return use_3d_avoidance; }

	void set_velocity(const Vector3 p_velocity);

	Vector3 get_velocity() const { return velocity; }

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
	static void navmesh_parse_source_geometry(const Ref<NavigationMesh>& p_navigation_mesh,
		Ref<NavigationMeshSourceGeometryData3D> p_source_geometry_data, Node* p_node);

private:
	void _update_map(RID p_map);
	void _update_position(const Vector3 p_position);
	void _update_transform();
	void _update_use_3d_avoidance(bool p_use_3d_avoidance);
};

class NavigationRegion3D final : public Node3D
{
	bool enabled = true;
	bool use_edge_connections = true;

	RID region;
	RID map_override;
	uint32_t navigation_layers = 1;
	real_t enter_cost = 0.0;
	real_t travel_cost = 1.0;
	Ref<NavigationMesh> navigation_mesh;

	void _navigation_mesh_changed();

	AABB bounds;

#ifdef DEBUG_ENABLED
	RID debug_instance;
	RID debug_edge_connections_instance;
	Ref<ArrayMesh> debug_mesh;
	Ref<ArrayMesh> debug_edge_connections_mesh;

private:
	void _update_debug_mesh();
	void _update_debug_edge_connections_mesh();
	void _navigation_map_changed(RID p_map);
	void _navigation_debug_changed();
#endif // DEBUG_ENABLED

protected:
	void _notification(int p_what);

public:
	RID get_rid() const;

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

	void set_navigation_mesh(const Ref<NavigationMesh>& p_navigation_mesh);
	Ref<NavigationMesh> get_navigation_mesh() const;

	void bake_navigation_mesh(bool p_on_thread);
	void _bake_finished();
	bool is_baking() const;

	PackedStringArray get_configuration_warnings() const override;

	AABB get_bounds() const { return bounds; }

	NavigationRegion3D();
	~NavigationRegion3D();

private:
	void _update_bounds();
	void _region_enter_navigation_map();
	void _region_exit_navigation_map();
	void _region_update_transform();
};

// --- Folded from scene/3d/path_3d.h ---
#include "scene/resources/material.h"

class ArrayMesh;

class Path3D : public Node3D
{
private:
	Ref<Curve3D> curve;
	RID debug_instance;
	Color debug_custom_color;
	Ref<ArrayMesh> debug_mesh;
	Ref<Material> debug_material;

	void _update_debug_mesh();
	void _update_debug_path_material();
	void _curve_changed();

protected:
	void _notification(int p_what);

public:
	void set_curve(const Ref<Curve3D>& p_curve);
	Ref<Curve3D> get_curve() const;

	const Color& get_debug_custom_color() const;
	void set_debug_custom_color(const Color& p_color);

	Ref<StandardMaterial3D> get_debug_material();

	Path3D();
	~Path3D();
};

class PathFollow3D : public Node3D
{
public:
	enum RotationMode
	{
		ROTATION_NONE,
		ROTATION_Y,
		ROTATION_XY,
		ROTATION_XYZ,
		ROTATION_ORIENTED
	};

private:
	Path3D* path = nullptr;
	real_t progress = 0.0;
	real_t h_offset = 0.0;
	real_t v_offset = 0.0;
	bool cubic = true;
	bool loop = true;
	bool tilt_enabled = true;
	bool use_model_front = false;
	RotationMode rotation_mode = ROTATION_XYZ;

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

	void set_tilt_enabled(bool p_enabled);
	bool is_tilt_enabled() const;

	RotationMode get_rotation_mode() const;

	void set_use_model_front(bool p_use_model_front);
	bool is_using_model_front() const;

	void set_cubic_interpolation_enabled(bool p_enabled);
	bool is_cubic_interpolation_enabled() const;

	PackedStringArray get_configuration_warnings() const override;

	void update_transform();

	static Transform3D correct_posture(
		Transform3D p_transform, PathFollow3D::RotationMode p_rotation_mode);
};

// --- Folded from scene/3d/xr/xr_nodes.h ---
#include "servers/rendering/renderer.h"

/*
	XRNode3D is a consolidated helper node that binds to an XRTracker
	(supporting generic tracker, controller, and anchor modes).

	It must be a child node of an XROrigin3D node.
*/

class XRNode3D final : public Node3D
{
public:
	enum Mode
	{
		MODE_TRACKER,
		MODE_CONTROLLER,
		MODE_ANCHOR,
	};

private:
	Mode mode = MODE_TRACKER;
	String tracker_name;
	String pose_name = SceneString(default_);
	bool has_tracking_data = false;
	bool show_when_tracked = false;
	Vector3 size;

	Ref<XRTracker> tracker;

	void _bind_tracker();
	void _unbind_tracker();
	void _changed_tracker(const String& p_tracker_name, int p_tracker_type);
	void _removed_tracker(const String& p_tracker_name, int p_tracker_type);

	void _pose_changed(const Ref<XRPose>& p_pose);
	void _pose_lost_tracking(const Ref<XRPose>& p_pose);
	void _set_has_tracking_data(bool p_has_tracking_data);

	void _update_visibility();

	void _button_pressed(const String& p_name);
	void _button_released(const String& p_name);
	void _input_float_changed(const String& p_name, float p_value);
	void _input_vector2_changed(const String& p_name, Vector2 p_value);
	void _profile_changed(const String& p_role);

public:
	void set_mode(Mode p_mode);
	Mode get_mode() const { return mode; }

	void set_tracker(const String& p_tracker_name);
	String get_tracker() const;

	void set_pose_name(const String& p_pose);
	String get_pose_name() const;

	bool get_is_active() const;
	bool get_has_tracking_data() const;

	void set_show_when_tracked(bool p_show);
	bool get_show_when_tracked() const;

	void trigger_haptic_pulse(const String& p_action_name, double p_frequency, double p_amplitude,
		double p_duration_sec, double p_delay_sec = 0);

	Ref<XRPose> get_pose();

	// Controller methods (MODE_CONTROLLER)
	bool is_button_pressed(const String& p_name) const;
	XRTracker::InputValue get_input(const String& p_name) const;
	float get_float(const String& p_name) const;
	Vector2 get_vector2(const String& p_name) const;
	XRTracker::TrackerHand get_tracker_hand() const;

	// Anchor methods (MODE_ANCHOR)
	Vector3 get_size() const;
	Plane get_plane() const;

	PackedStringArray get_configuration_warnings() const override;

	XRNode3D(Mode p_mode = MODE_TRACKER);
	~XRNode3D();
};

/*
	XROrigin3D is a special spatial node that acts as our origin point mapping our real world center
	of our tracking volume into our virtual world.
*/

class XROrigin3D final : public Node3D
{
private:
	bool current = false;
	static Vector<XROrigin3D*> origin_nodes; // all origin nodes in tree

	void _set_current(bool p_enabled, bool p_update_others);

protected:
	void _notification(int p_what);
	virtual void _physics_interpolated_changed() override;

public:
	PackedStringArray get_configuration_warnings() const override;

	real_t get_world_scale() const;
	void set_world_scale(real_t p_world_scale);

	void set_current(bool p_enabled);
	bool is_current() const;
};

// --- Folded from scene/3d/visual_instance_3d.h ---

#include "scene/resources/material.h"
#include "scene/resources/texture.h"

class TriangleMesh;

struct RenderInstance3D
{
	enum GetFacesFlags
	{
		FACES_SOLID = 1,
		FACES_ENCLOSING = 2,
		FACES_DYNAMIC = 4,
	};

	enum ShadowCastingSetting
	{
		SHADOW_CASTING_SETTING_OFF = RSE::SHADOW_CASTING_SETTING_OFF,
		SHADOW_CASTING_SETTING_ON = RSE::SHADOW_CASTING_SETTING_ON,
		SHADOW_CASTING_SETTING_DOUBLE_SIDED = RSE::SHADOW_CASTING_SETTING_DOUBLE_SIDED,
		SHADOW_CASTING_SETTING_SHADOWS_ONLY = RSE::SHADOW_CASTING_SETTING_SHADOWS_ONLY,
	};

	enum GIMode
	{
		GI_MODE_DISABLED,
		GI_MODE_STATIC,
		GI_MODE_DYNAMIC,
	};

	enum LightmapScale
	{
		LIGHTMAP_SCALE_1X,
		LIGHTMAP_SCALE_2X,
		LIGHTMAP_SCALE_4X,
		LIGHTMAP_SCALE_8X,
		LIGHTMAP_SCALE_MAX,
	};

	enum VisibilityRangeFadeMode
	{
		VISIBILITY_RANGE_FADE_DISABLED = RSE::VISIBILITY_RANGE_FADE_DISABLED,
		VISIBILITY_RANGE_FADE_SELF = RSE::VISIBILITY_RANGE_FADE_SELF,
		VISIBILITY_RANGE_FADE_DEPENDENCIES = RSE::VISIBILITY_RANGE_FADE_DEPENDENCIES,
	};

	RID base;
	RID instance;
	uint32_t layers = 1;
	float sorting_offset = 0.0f;
	bool sorting_use_aabb_center = true;

	ShadowCastingSetting shadow_casting_setting = SHADOW_CASTING_SETTING_ON;
	Ref<Material> material_override;
	Ref<Material> material_overlay;

	float visibility_range_begin = 0.0f;
	float visibility_range_end = 0.0f;
	float visibility_range_begin_margin = 0.0f;
	float visibility_range_end_margin = 0.0f;
	VisibilityRangeFadeMode visibility_range_fade_mode = VISIBILITY_RANGE_FADE_DISABLED;

	float transparency = 0.0f;
	float lod_bias = 1.0f;
	mutable HashMap<String, String> instance_shader_parameter_property_remap;

	float extra_cull_margin = 0.0f;
	AABB custom_aabb;
	float lightmap_texel_scale = 1.0f;
	GIMode gi_mode = GI_MODE_STATIC;
	bool ignore_occlusion_culling = false;

	void update_visibility(Node3D* p_owner);
	void set_instance_use_identity_transform(Node3D* p_owner, bool p_enable);
	void fti_update_servers_xform(Node3D* p_owner);
	void notification(Node3D* p_owner, int p_what);

	RID get_instance() const;

	void set_base(const RID& p_base);
	RID get_base() const;

	void set_layer_mask(uint32_t p_mask);
	uint32_t get_layer_mask() const;

	void set_layer_mask_value(int p_layer_number, bool p_enable);
	bool get_layer_mask_value(int p_layer_number) const;

	void set_sorting_offset(float p_offset);
	float get_sorting_offset() const;

	void set_sorting_use_aabb_center(bool p_enabled);
	bool is_sorting_use_aabb_center() const;

	void set_cast_shadows_setting(ShadowCastingSetting p_shadow_casting_setting);
	ShadowCastingSetting get_cast_shadows_setting() const;

	float get_transparency() const;
	float get_visibility_range_begin() const;
	float get_visibility_range_end() const;
	float get_visibility_range_begin_margin() const;
	float get_visibility_range_end_margin() const;
	VisibilityRangeFadeMode get_visibility_range_fade_mode() const;

	void set_material_override(const Ref<Material>& p_material);
	Ref<Material> get_material_override() const;

	void set_material_overlay(const Ref<Material>& p_material);
	Ref<Material> get_material_overlay() const;

	void set_extra_cull_margin(float p_margin);
	float get_extra_cull_margin() const;

	void set_lod_bias(float p_bias);
	float get_lod_bias() const;

	void set_gi_mode(GIMode p_mode);
	GIMode get_gi_mode() const;

	void set_lightmap_texel_scale(float p_scale);
	float get_lightmap_texel_scale() const;

	AABB get_custom_aabb() const;

	void set_ignore_occlusion_culling(bool p_enabled);
	bool is_ignoring_occlusion_culling() const;

	const String* _instance_uniform_get_remap(const String& p_name) const;
	void append_geometry_warnings(PackedStringArray& r_warnings) const;

	RenderInstance3D() = default;
	~RenderInstance3D();
};

// --- Folded from scene/3d/decal.h ---


class Decal : public Node3D
{
public:
	RenderInstance3D render_instance;

	enum DecalTexture
	{
		TEXTURE_ALBEDO,
		TEXTURE_NORMAL,
		TEXTURE_ORM,
		TEXTURE_EMISSION,
		TEXTURE_MAX
	};

private:
	RID decal;
	Vector3 size = Vector3(2, 2, 2);
	Ref<Texture2D> textures[TEXTURE_MAX];
	real_t emission_energy = 1.0;
	real_t albedo_mix = 1.0;
	Color modulate = Color(1, 1, 1, 1);
	uint32_t cull_mask = (1 << 20) - 1;
	real_t normal_fade = 0.0;
	real_t upper_fade = 0.3;
	real_t lower_fade = 0.3;
	bool distance_fade_enabled = false;
	real_t distance_fade_begin = 40.0;
	real_t distance_fade_length = 10.0;

protected:
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	virtual PackedStringArray get_configuration_warnings() const override;

	Vector3 get_size() const;

	void set_texture(DecalTexture p_type, const Ref<Texture2D>& p_texture);
	Ref<Texture2D> get_texture(DecalTexture p_type) const;

	void set_emission_energy(real_t p_energy);
	real_t get_emission_energy() const;

	void set_albedo_mix(real_t p_mix);
	real_t get_albedo_mix() const;

	void set_modulate(Color p_modulate);
	Color get_modulate() const;

	void set_upper_fade(real_t p_energy);
	real_t get_upper_fade() const;

	void set_lower_fade(real_t p_fade);
	real_t get_lower_fade() const;

	void set_normal_fade(real_t p_fade);
	real_t get_normal_fade() const;

	void set_enable_distance_fade(bool p_enable);
	bool is_distance_fade_enabled() const;

	void set_distance_fade_begin(real_t p_distance);
	real_t get_distance_fade_begin() const;

	void set_distance_fade_length(real_t p_length);
	real_t get_distance_fade_length() const;

	uint32_t get_cull_mask() const;

	AABB get_aabb() const;

	Decal();
	~Decal();
};



// --- End scene/3d/decal.h ---

// --- Folded from scene/3d/label_3d.h ---

#include "scene/resources/theme.h"
#include "scene/resources/material.h"
#include "scene/resources/theme.h"

class Label3D : public Node3D
{
public:
	RenderInstance3D render_instance;

	enum DrawFlags
	{
		FLAG_SHADED,
		FLAG_DOUBLE_SIDED,
		FLAG_DISABLE_DEPTH_TEST,
		FLAG_FIXED_SIZE,
		FLAG_MAX
	};

	enum AlphaCutMode
	{
		ALPHA_CUT_DISABLED,
		ALPHA_CUT_DISCARD,
		ALPHA_CUT_OPAQUE_PREPASS,
		ALPHA_CUT_HASH,
		ALPHA_CUT_MAX
	};

private:
	real_t pixel_size = 0.005;
	bool flags[FLAG_MAX] = {};
	AlphaCutMode alpha_cut = ALPHA_CUT_DISABLED;
	float alpha_scissor_threshold = 0.5;
	float alpha_hash_scale = 1.0;
	StandardMaterial3D::AlphaAntiAliasing alpha_antialiasing_mode =
		StandardMaterial3D::ALPHA_ANTIALIASING_OFF;
	float alpha_antialiasing_edge = 0.0f;

	AABB aabb;

	mutable Ref<TriangleMesh> triangle_mesh;
	RID mesh;

	struct SurfaceData
	{
		PackedVector3Array mesh_vertices;
		PackedVector3Array mesh_normals;
		PackedFloat32Array mesh_tangents;
		PackedColorArray mesh_colors;
		PackedVector2Array mesh_uvs;
		PackedInt32Array indices;
		int offset = 0;
		float z_shift = 0.0;
		RID material;
	};

	struct SurfaceKey
	{
		uint64_t texture_id;
		int32_t priority;
		int32_t outline_size;

		bool operator==(const SurfaceKey& p_b) const
		{
			return (texture_id == p_b.texture_id) && (priority == p_b.priority) &&
				   (outline_size == p_b.outline_size);
		}

		SurfaceKey(uint64_t p_texture_id, int p_priority, int p_outline_size)
		{
			texture_id = p_texture_id;
			priority = p_priority;
			outline_size = p_outline_size;
		}
	};

	struct SurfaceKeyHasher
	{
		_FORCE_INLINE_ static uint32_t hash(const SurfaceKey& p_a)
		{
			return hash_murmur3_buffer(&p_a, sizeof(SurfaceKey));
		}
	};

	HashMap<SurfaceKey, SurfaceData, SurfaceKeyHasher> surfaces;

	HorizontalAlignment horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER;
	VerticalAlignment vertical_alignment = VERTICAL_ALIGNMENT_CENTER;
	String text;
	String xl_text;
	bool uppercase = false;

	TextServer::AutowrapMode autowrap_mode = TextServer::AUTOWRAP_OFF;
	uint32_t autowrap_flags_trim =
		TextServer::BREAK_TRIM_START_EDGE_SPACES | TextServer::BREAK_TRIM_END_EDGE_SPACES;
	uint32_t jst_flags = TextServer::JUSTIFICATION_WORD_BOUND | TextServer::JUSTIFICATION_KASHIDA |
						 TextServer::JUSTIFICATION_SKIP_LAST_LINE |
						 TextServer::JUSTIFICATION_DO_NOT_SKIP_SINGLE_LINE;
	float width = 500.0;

	int font_size = 32;
	Ref<Font> font_override;
	mutable Ref<Font> theme_font;
	Color modulate = Color(1, 1, 1, 1);
	Point2 lbl_offset;
	int outline_render_priority = -1;
	int render_priority = 0;

	int outline_size = 12;
	Color outline_modulate = Color(0, 0, 0, 1);

	float line_spacing = 0.f;

	String language;
	TextServer::Direction text_direction = TextServer::DIRECTION_AUTO;
	TextServer::StructuredTextParser st_parser = TextServer::STRUCTURED_TEXT_DEFAULT;

	RID text_rid;
	Vector<RID> lines_rid;

	StandardMaterial3D::BillboardMode billboard_mode = StandardMaterial3D::BILLBOARD_DISABLED;
	StandardMaterial3D::TextureFilter texture_filter =
		StandardMaterial3D::TEXTURE_FILTER_LINEAR_WITH_MIPMAPS;

	bool pending_update = false;

	bool dirty_lines = true;
	bool dirty_font = true;
	bool dirty_text = true;

	void _generate_glyph_surfaces(const Glyph& p_glyph, Vector2& r_offset, const Color& p_modulate,
		int p_priority = 0, int p_outline_size = 0);

protected:
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;
	void _font_changed();
	void _queue_update();

	void _shape();

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }
	void set_horizontal_alignment(HorizontalAlignment p_alignment);
	HorizontalAlignment get_horizontal_alignment() const;

	void set_vertical_alignment(VerticalAlignment p_alignment);
	VerticalAlignment get_vertical_alignment() const;

	void set_render_priority(int p_priority);
	int get_render_priority() const;

	void set_outline_render_priority(int p_priority);
	int get_outline_render_priority() const;

	void set_text(const String& p_string);
	String get_text() const;

	void set_text_direction(TextServer::Direction p_text_direction);
	TextServer::Direction get_text_direction() const;

	void set_language(const String& p_language);
	String get_language() const;

	void set_structured_text_bidi_override(TextServer::StructuredTextParser p_parser);
	TextServer::StructuredTextParser get_structured_text_bidi_override() const;

	void set_uppercase(bool p_uppercase);
	bool is_uppercase() const;

	void set_font(const Ref<Font>& p_font);
	Ref<Font> get_font() const;
	Ref<Font> _get_font_or_default() const;

	void set_font_size(int p_size);
	int get_font_size() const;

	void set_outline_size(int p_size);
	int get_outline_size() const;

	void set_line_spacing(float p_size);
	float get_line_spacing() const;

	void set_modulate(const Color& p_color);
	Color get_modulate() const;

	void set_outline_modulate(const Color& p_color);
	Color get_outline_modulate() const;

	void set_autowrap_mode(TextServer::AutowrapMode p_mode);
	TextServer::AutowrapMode get_autowrap_mode() const;

	void set_autowrap_trim_flags(uint32_t p_flags);
	uint32_t get_autowrap_trim_flags() const;

	void set_justification_flags(uint32_t p_flags);
	uint32_t get_justification_flags() const;

	void set_width(float p_width);
	float get_width() const;

	void set_pixel_size(real_t p_amount);
	real_t get_pixel_size() const;

	void set_offset(const Point2& p_offset);
	Point2 get_offset() const;

	void set_draw_flag(DrawFlags p_flag, bool p_enable);
	bool get_draw_flag(DrawFlags p_flag) const;

	void set_alpha_cut_mode(AlphaCutMode p_mode);
	AlphaCutMode get_alpha_cut_mode() const;

	void set_alpha_scissor_threshold(float p_threshold);
	float get_alpha_scissor_threshold() const;

	void set_alpha_hash_scale(float p_hash_scale);
	float get_alpha_hash_scale() const;

	void set_alpha_antialiasing(StandardMaterial3D::AlphaAntiAliasing p_alpha_aa);
	StandardMaterial3D::AlphaAntiAliasing get_alpha_antialiasing() const;

	void set_alpha_antialiasing_edge(float p_edge);
	float get_alpha_antialiasing_edge() const;

	void set_billboard_mode(StandardMaterial3D::BillboardMode p_mode);
	StandardMaterial3D::BillboardMode get_billboard_mode() const;

	void set_texture_filter(StandardMaterial3D::TextureFilter p_filter);
	StandardMaterial3D::TextureFilter get_texture_filter() const;

	AABB get_aabb() const;
	Ref<TriangleMesh> generate_triangle_mesh() const;

	Label3D();
	~Label3D();
};



// --- End scene/3d/label_3d.h ---

// --- Folded from scene/3d/occluder_instance_3d.h ---


class ArrayMesh;

class Occluder3D : public Resource
{
public:
	enum OccluderType
	{
		OCCLUDER_ARRAY,
		OCCLUDER_QUAD,
		OCCLUDER_BOX,
		OCCLUDER_SPHERE,
		OCCLUDER_POLYGON,
	};

private:
	static constexpr int RINGS = 7;
	static constexpr int RADIAL_SEGMENTS = 7;

	OccluderType occluder_type = OCCLUDER_ARRAY;

	RID occluder;
	PackedVector3Array vertices;
	PackedInt32Array indices;
	AABB aabb;

	PackedVector3Array array_vertices;
	PackedInt32Array array_indices;
	Size2 quad_size = Vector2(1.0f, 1.0f);
	Vector3 box_size = Vector3(1.0f, 1.0f, 1.0f);
	float radius = 1.0f;
	Vector<Vector2> polygon;

	mutable Ref<ArrayMesh> debug_mesh;
	mutable Vector<Vector3> debug_lines;

	bool _has_editable_3d_polygon_no_depth() const;

protected:
	void _update();
	void _update_arrays(PackedVector3Array& r_vertices, PackedInt32Array& r_indices);

public:
	void set_occluder_type(OccluderType p_type);
	OccluderType get_occluder_type() const;

	void set_arrays(PackedVector3Array p_vertices, PackedInt32Array p_indices);
	void set_vertices(PackedVector3Array p_vertices);
	void set_indices(PackedInt32Array p_indices);

	PackedVector3Array get_vertices() const;
	PackedInt32Array get_indices() const;

	void set_quad_size(const Size2& p_size);
	Size2 get_quad_size() const;

	void set_box_size(const Vector3& p_size);
	Vector3 get_box_size() const;

	void set_radius(float p_radius);
	float get_radius() const;

	void set_polygon(const Vector<Vector2>& p_polygon);
	Vector<Vector2> get_polygon() const;

	Vector<Vector3> get_debug_lines() const;
	Ref<ArrayMesh> get_debug_mesh() const;
	AABB get_aabb() const;

	virtual RID get_rid() const override;
	Occluder3D(OccluderType p_type = OCCLUDER_ARRAY);
	virtual ~Occluder3D();
};

class OccluderInstance3D : public Node3D
{
public:
	RenderInstance3D render_instance;

private:
	Ref<Occluder3D> occluder;
	uint32_t bake_mask = 0xFFFFFFFF;
	float bake_simplification_dist = 0.1f;

	void _bake_node(Node* p_node, PackedVector3Array& r_vertices, PackedInt32Array& r_indices);

	bool _is_editable_3d_polygon() const;
	Ref<Resource> _get_editable_3d_polygon_resource() const;

protected:
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	virtual PackedStringArray get_configuration_warnings() const override;

	enum BakeError
	{
		BAKE_ERROR_OK,
		BAKE_ERROR_NO_SAVE_PATH,
		BAKE_ERROR_NO_MESHES,
		BAKE_ERROR_CANT_SAVE,
	};

	void set_occluder(const Ref<Occluder3D>& p_occluder);
	Ref<Occluder3D> get_occluder() const;

	AABB get_aabb() const;

	uint32_t get_bake_mask() const;

	void set_bake_simplification_distance(float p_dist);
	float get_bake_simplification_distance() const;

	bool get_bake_mask_value(int p_layer_number) const;

	BakeError bake_scene(Node* p_from_node, String p_occluder_path = "");
	static void bake_single_node(const Node3D* p_node, float p_simplification_distance,
		PackedVector3Array& r_vertices, PackedInt32Array& r_indices);

	OccluderInstance3D();
	~OccluderInstance3D();
};



// --- End scene/3d/occluder_instance_3d.h ---

// --- Folded from scene/3d/sprite_3d.h ---

#include "scene/resources/material.h"
// --- Folded from scene/resources/sprite_frames.h ---
#include "scene/resources/texture.h"

static const float SPRITE_FRAME_MINIMUM_DURATION = 0.01;

class SpriteFrames : public Resource
{
public:
	enum LoopMode
	{
		LOOP_NONE,
		LOOP_LINEAR,
		LOOP_PINGPONG,
	};

private:
	struct Frame
	{
		Ref<Texture2D> texture;
		float duration = 1.0;
	};

	struct Anim
	{
		double speed = 5.0;
		LoopMode loop = LoopMode::LOOP_LINEAR;
		Vector<Frame> frames;
	};

	HashMap<String, Anim> animations;

protected:

public:
	void add_animation(const String& p_anim);
	bool has_animation(const String& p_anim) const;
	void duplicate_animation(const String& p_from, const String& p_to);
	void remove_animation(const String& p_anim);
	void rename_animation(const String& p_prev, const String& p_next);

	void get_animation_list(List<String>* r_animations) const;
	Vector<String> get_animation_names() const;

	void set_animation_speed(const String& p_anim, double p_fps);
	double get_animation_speed(const String& p_anim) const;

#ifndef DISABLE_DEPRECATED
	void set_animation_loop(const String& p_anim, bool p_loop);
	bool get_animation_loop(const String& p_anim) const;
#endif

	void set_animation_loop_mode(const String& p_anim, LoopMode p_loop_mode);
	LoopMode get_animation_loop_mode(const String& p_anim) const;

	void add_frame(const String& p_anim, const Ref<Texture2D>& p_texture,
		float p_duration = 1.0, int p_at_pos = -1);
	void set_frame(const String& p_anim, int p_idx, const Ref<Texture2D>& p_texture,
		float p_duration = 1.0);
	void remove_frame(const String& p_anim, int p_idx);

	int get_frame_count(const String& p_anim) const;

	_FORCE_INLINE_ Ref<Texture2D> get_frame_texture(const String& p_anim, int p_idx) const
	{
		HashMap<String, Anim>::ConstIterator E = animations.find(p_anim);
		ERR_FAIL_COND_V_MSG(
			!E, Ref<Texture2D>(), "Animation '" + String(p_anim) + "' doesn't exist.");
		ERR_FAIL_COND_V(p_idx < 0, Ref<Texture2D>());
		if (p_idx >= E->value.frames.size()) {
			return Ref<Texture2D>();
		}

		return E->value.frames[p_idx].texture;
	}

	_FORCE_INLINE_ float get_frame_duration(const String& p_anim, int p_idx) const
	{
		HashMap<String, Anim>::ConstIterator E = animations.find(p_anim);
		ERR_FAIL_COND_V_MSG(!E, 1.0, "Animation '" + String(p_anim) + "' doesn't exist.");
		ERR_FAIL_COND_V(p_idx < 0, 1.0);
		if (p_idx >= E->value.frames.size()) {
			return 1.0;
		}

		return E->value.frames[p_idx].duration;
	}

	void clear(const String& p_anim);
	void clear_all();

#ifdef TOOLS_ENABLED
	virtual void get_argument_options(
		const String& p_function, int p_idx, List<String>* r_options) const;
#endif

	SpriteFrames();
};



// --- End scene/resources/sprite_frames.h ---

class Sprite3D : public Node3D
{
public:
	enum DrawFlags
	{
		FLAG_TRANSPARENT,
		FLAG_SHADED,
		FLAG_DOUBLE_SIDED,
		FLAG_DISABLE_DEPTH_TEST,
		FLAG_FIXED_SIZE,
		FLAG_MAX
	};

	enum AlphaCutMode
	{
		ALPHA_CUT_DISABLED,
		ALPHA_CUT_DISCARD,
		ALPHA_CUT_OPAQUE_PREPASS,
		ALPHA_CUT_HASH,
		ALPHA_CUT_MAX
	};

	RenderInstance3D render_instance;

private:
	mutable Ref<TriangleMesh> triangle_mesh;

	bool color_dirty = true;
	Color color_accum;

	Sprite3D* parent_sprite = nullptr;
	List<Sprite3D*> children;
	List<Sprite3D*>::Element* pI = nullptr;

	bool centered = true;
	Point2 offset;

	bool hflip = false;
	bool vflip = false;

	Color modulate = Color(1, 1, 1, 1);
	int render_priority = 0;

	Vector3::Axis axis = Vector3::AXIS_Z;
	real_t pixel_size = 0.01;
	AABB aabb;

	RID mesh;
	RID material;

	RID last_shader;
	RID last_texture;

	bool flags[FLAG_MAX] = {};
	AlphaCutMode alpha_cut = ALPHA_CUT_DISABLED;
	float alpha_scissor_threshold = 0.5;
	float alpha_hash_scale = 1.0;
	StandardMaterial3D::AlphaAntiAliasing alpha_antialiasing_mode =
		StandardMaterial3D::ALPHA_ANTIALIASING_OFF;
	float alpha_antialiasing_edge = 0.0f;
	StandardMaterial3D::BillboardMode billboard_mode = StandardMaterial3D::BILLBOARD_DISABLED;
	StandardMaterial3D::TextureFilter texture_filter =
		StandardMaterial3D::TEXTURE_FILTER_LINEAR_WITH_MIPMAPS;
	bool redraw_needed = false;
	bool pending_update = false;

	uint32_t mesh_surface_offsets[RSE::ARRAY_MAX];
	PackedByteArray vertex_buffer;
	PackedByteArray attribute_buffer;
	uint32_t vertex_stride = 0;
	uint32_t normal_tangent_stride = 0;
	uint32_t attrib_stride = 0;
	uint32_t skin_stride = 0;
	uint32_t mesh_surface_format = 0;

	// Single-texture sprite state
	Ref<Texture2D> texture;
	bool region = false;
	Rect2 region_rect;
	int frame = 0;
	int vframes = 1;
	int hframes = 1;

	// Optional SpriteFrames animation state (used when frames.is_valid())
	Ref<SpriteFrames> frames;
	String autoplay;
	bool playing = false;
	String animation = SceneString(default_);
	float speed_scale = 1.0;
	float custom_speed_scale = 1.0;
	real_t frame_speed_scale = 1.0;
	real_t frame_progress = 0.0;

	void _im_update();
	void _propagate_color_changed();
	Color _get_color_accum();
	void _draw();
	void draw_texture_rect(Ref<Texture2D> p_texture, Rect2 p_dst_rect, Rect2 p_src_rect);
	void _queue_redraw();

	double _get_frame_duration();
	void _calc_frame_speed_scale();
	void _stop_internal(bool p_reset);

protected:
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	void set_centered(bool p_center);
	bool is_centered() const;

	void set_offset(const Point2& p_offset);
	Point2 get_offset() const;

	void set_flip_h(bool p_flip);
	bool is_flipped_h() const;

	void set_flip_v(bool p_flip);
	bool is_flipped_v() const;

	void set_render_priority(int p_priority);
	int get_render_priority() const;

	void set_modulate(const Color& p_color);
	Color get_modulate() const;

	void set_pixel_size(real_t p_amount);
	real_t get_pixel_size() const;

	void set_axis(Vector3::Axis p_axis);
	Vector3::Axis get_axis() const;

	void set_draw_flag(DrawFlags p_flag, bool p_enable);
	bool get_draw_flag(DrawFlags p_flag) const;

	void set_alpha_cut_mode(AlphaCutMode p_mode);
	AlphaCutMode get_alpha_cut_mode() const;

	void set_alpha_scissor_threshold(float p_threshold);
	float get_alpha_scissor_threshold() const;

	void set_alpha_hash_scale(float p_hash_scale);
	float get_alpha_hash_scale() const;

	void set_alpha_antialiasing(StandardMaterial3D::AlphaAntiAliasing p_alpha_aa);
	StandardMaterial3D::AlphaAntiAliasing get_alpha_antialiasing() const;

	void set_alpha_antialiasing_edge(float p_edge);
	float get_alpha_antialiasing_edge() const;

	void set_billboard_mode(StandardMaterial3D::BillboardMode p_mode);
	StandardMaterial3D::BillboardMode get_billboard_mode() const;

	void set_texture_filter(StandardMaterial3D::TextureFilter p_filter);
	StandardMaterial3D::TextureFilter get_texture_filter() const;

	// Static/sheet texture API
	void set_texture(const Ref<Texture2D>& p_texture);
	Ref<Texture2D> get_texture() const;

	void set_region_enabled(bool p_region);
	bool is_region_enabled() const;

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

	// SpriteFrames animation API
	void set_sprite_frames(const Ref<SpriteFrames>& p_frames);
	Ref<SpriteFrames> get_sprite_frames() const;

	void play(const String& p_name = String(), float p_custom_scale = 1.0f, bool p_from_end = false);
	void play_backwards(const String& p_name = String());
	void pause();
	void stop();

	bool is_playing() const;

	void set_animation(const String& p_name);
	String get_animation() const;

	void set_autoplay(const String& p_name);
	String get_autoplay() const;

	void set_frame_progress(real_t p_progress);
	real_t get_frame_progress() const;

	void set_speed_scale(float p_speed_scale);
	float get_speed_scale() const;
	float get_playing_speed() const;

	Rect2 get_item_rect() const;
	AABB get_aabb() const;
	Ref<TriangleMesh> generate_triangle_mesh() const;

	virtual PackedStringArray get_configuration_warnings() const override;

#ifdef TOOLS_ENABLED
	void get_argument_options(
		const String& p_function, int p_idx, List<String>* r_options) const;
#endif

	Sprite3D();
	~Sprite3D();
};

// --- End scene/3d/sprite_3d.h ---

// --- Folded from scene/3d/light_3d.h ---


class Light3D : public Node3D
{
public:
	enum Param
	{
		PARAM_ENERGY = RSE::LIGHT_PARAM_ENERGY,
		PARAM_INDIRECT_ENERGY = RSE::LIGHT_PARAM_INDIRECT_ENERGY,
		PARAM_VOLUMETRIC_FOG_ENERGY = RSE::LIGHT_PARAM_VOLUMETRIC_FOG_ENERGY,
		PARAM_SPECULAR = RSE::LIGHT_PARAM_SPECULAR,
		PARAM_RANGE = RSE::LIGHT_PARAM_RANGE,
		PARAM_SIZE = RSE::LIGHT_PARAM_SIZE,
		PARAM_ATTENUATION = RSE::LIGHT_PARAM_ATTENUATION,
		PARAM_SPOT_ANGLE = RSE::LIGHT_PARAM_SPOT_ANGLE,
		PARAM_SPOT_ATTENUATION = RSE::LIGHT_PARAM_SPOT_ATTENUATION,
		PARAM_SHADOW_MAX_DISTANCE = RSE::LIGHT_PARAM_SHADOW_MAX_DISTANCE,
		PARAM_SHADOW_SPLIT_1_OFFSET = RSE::LIGHT_PARAM_SHADOW_SPLIT_1_OFFSET,
		PARAM_SHADOW_SPLIT_2_OFFSET = RSE::LIGHT_PARAM_SHADOW_SPLIT_2_OFFSET,
		PARAM_SHADOW_SPLIT_3_OFFSET = RSE::LIGHT_PARAM_SHADOW_SPLIT_3_OFFSET,
		PARAM_SHADOW_FADE_START = RSE::LIGHT_PARAM_SHADOW_FADE_START,
		PARAM_SHADOW_NORMAL_BIAS = RSE::LIGHT_PARAM_SHADOW_NORMAL_BIAS,
		PARAM_SHADOW_BIAS = RSE::LIGHT_PARAM_SHADOW_BIAS,
		PARAM_SHADOW_PANCAKE_SIZE = RSE::LIGHT_PARAM_SHADOW_PANCAKE_SIZE,
		PARAM_SHADOW_OPACITY = RSE::LIGHT_PARAM_SHADOW_OPACITY,
		PARAM_SHADOW_BLUR = RSE::LIGHT_PARAM_SHADOW_BLUR,
		PARAM_TRANSMITTANCE_BIAS = RSE::LIGHT_PARAM_TRANSMITTANCE_BIAS,
		PARAM_INTENSITY = RSE::LIGHT_PARAM_INTENSITY,
		PARAM_MAX = RSE::LIGHT_PARAM_MAX
	};

	enum BakeMode
	{
		BAKE_DISABLED,
		BAKE_STATIC,
		BAKE_DYNAMIC,
	};

	enum DirectionalShadowMode
	{
		DIRECTIONAL_SHADOW_ORTHOGONAL,
		DIRECTIONAL_SHADOW_PARALLEL_2_SPLITS,
		DIRECTIONAL_SHADOW_PARALLEL_4_SPLITS,
	};

	enum SkyMode
	{
		SKY_MODE_LIGHT_AND_SKY,
		SKY_MODE_LIGHT_ONLY,
		SKY_MODE_SKY_ONLY,
	};

	enum OmniShadowMode
	{
		OMNI_SHADOW_DUAL_PARABOLOID,
		OMNI_SHADOW_CUBE,
	};

	RenderInstance3D render_instance;

private:
	RSE::LightType type = RSE::LIGHT_DIRECTIONAL;
	RID light;

	Color color = Color(1, 1, 1);
	real_t param[PARAM_MAX] = {};
	bool shadow = false;
	bool negative = false;
	bool reverse_cull = false;
	uint32_t cull_mask = 0xFFFFFFFF;
	uint32_t shadow_caster_mask = 0xFFFFFFFF;
	bool distance_fade_enabled = false;
	real_t distance_fade_begin = 40.0;
	real_t distance_fade_shadow = 50.0;
	real_t distance_fade_length = 10.0;
	bool editor_only = false;
	BakeMode bake_mode = BAKE_DYNAMIC;
	Ref<Texture2D> projector;
	Color correlated_color = Color(1.0, 1.0, 1.0);
	float temperature = 6500.0;

	// Directional-specific state
	bool blend_splits = false;
	DirectionalShadowMode directional_shadow_mode = DIRECTIONAL_SHADOW_PARALLEL_4_SPLITS;
	SkyMode sky_mode = SKY_MODE_LIGHT_AND_SKY;

	// Omni-specific state
	OmniShadowMode omni_shadow_mode = OMNI_SHADOW_CUBE;

	// Area-specific state
	Vector2 area_size = Vector2(1.0, 1.0);
	Ref<Texture2D> area_texture;
	bool area_normalize_energy = true;

	void _create_light_rid();
	void _free_light_rid();
	void _update_visibility();

	virtual void owner_changed_notify() override;

protected:
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	void set_light_type(RSE::LightType p_type);
	RSE::LightType get_light_type() const { return type; }

	void set_editor_only(bool p_editor_only);
	bool is_editor_only() const;

	void set_param(Param p_param, real_t p_value);
	real_t get_param(Param p_param) const;

	void set_shadow(bool p_enable);
	bool has_shadow() const;

	void set_negative(bool p_enable);
	bool is_negative() const;

	void set_enable_distance_fade(bool p_enable);
	bool is_distance_fade_enabled() const;

	void set_distance_fade_begin(real_t p_distance);
	real_t get_distance_fade_begin() const;

	void set_distance_fade_shadow(real_t p_distance);
	real_t get_distance_fade_shadow() const;

	void set_distance_fade_length(real_t p_length);
	real_t get_distance_fade_length() const;

	void set_cull_mask(uint32_t p_cull_mask);
	uint32_t get_cull_mask() const;

	void set_color(const Color& p_color);
	Color get_color() const;

	void set_shadow_reverse_cull_face(bool p_enable);
	bool get_shadow_reverse_cull_face() const;

	void set_shadow_caster_mask(uint32_t p_caster_mask);
	uint32_t get_shadow_caster_mask() const;

	void set_bake_mode(BakeMode p_mode);
	BakeMode get_bake_mode() const;

	void set_projector(const Ref<Texture2D>& p_texture);
	Ref<Texture2D> get_projector() const;

	float get_temperature() const;
	Color get_correlated_color() const;

	// Directional light parameters
	void set_directional_shadow_mode(DirectionalShadowMode p_mode);
	DirectionalShadowMode get_directional_shadow_mode() const { return directional_shadow_mode; }

	void set_blend_splits(bool p_enable);
	bool is_blend_splits_enabled() const { return blend_splits; }

	void set_sky_mode(SkyMode p_mode);
	SkyMode get_sky_mode() const { return sky_mode; }

	// Omni light parameters
	void set_omni_shadow_mode(OmniShadowMode p_mode);
	OmniShadowMode get_omni_shadow_mode() const { return omni_shadow_mode; }

	// Area light parameters
	void set_area_size(const Vector2& p_size);
	Vector2 get_area_size() const { return area_size; }

	void set_area_texture(const Ref<Texture2D>& p_texture);
	Ref<Texture2D> get_area_texture() const { return area_texture; }

	void set_area_normalize_energy(bool p_enable);
	bool is_area_normalizing_energy() const { return area_normalize_energy; }

	AABB get_aabb() const;
	virtual PackedStringArray get_configuration_warnings() const override;

	Light3D(RSE::LightType p_type = RSE::LIGHT_DIRECTIONAL);
	~Light3D();
};

// --- End scene/3d/light_3d.h ---

// --- Folded from scene/3d/lightmapper.h ---
#include "core/io/image.h"
#include "core/templates/hash_set.h"
#include "core/types.h"

class Image;

class LightmapDenoiser : public RefCounted
{
protected:
	static Ref<LightmapDenoiser> (*create_function)();

public:
	virtual Ref<Image> denoise_image(const Ref<Image>& p_image) = 0;
	static Ref<LightmapDenoiser> create();
};

class LightmapRaycaster : public RefCounted
{
public:
	struct EmbreeState;

private:
	EmbreeState* embree_state = nullptr;

public:
	// Compatible with embree4 rays.
	struct alignas(16) Ray
	{
		const static unsigned int INVALID_GEOMETRY_ID = ((unsigned int)-1); // from rtcore_common.h

		/*! Default construction does nothing. */
		_FORCE_INLINE_ Ray() : geomID(INVALID_GEOMETRY_ID) {}

		/*! Constructs a ray from origin, direction, and ray segment. Near
		 *  has to be smaller than far. */
		_FORCE_INLINE_ Ray(const Vector3& p_org, const Vector3& p_dir, float p_tnear = 0.0f,
			float p_tfar = Math::INF)
			: org(p_org), tnear(p_tnear), dir(p_dir), time(0.0f), tfar(p_tfar), mask(-1), u(0.0),
			  v(0.0), primID(INVALID_GEOMETRY_ID), geomID(INVALID_GEOMETRY_ID),
			  instID(INVALID_GEOMETRY_ID)
		{
		}

		/*! Tests if we hit something. */
		_FORCE_INLINE_ explicit operator bool() const { return geomID != INVALID_GEOMETRY_ID; }

	public:
		Vector3 org;		//!< Ray origin + tnear
		float tnear;		//!< Start of ray segment
		Vector3 dir;		//!< Ray direction + tfar
		float time;			//!< Time of this ray for motion blur.
		float tfar;			//!< End of ray segment
		unsigned int mask;	//!< used to mask out objects during traversal
		unsigned int id;	//!< ray ID
		unsigned int flags; //!< ray flags

		Vector3 normal;		 //!< Not normalized geometry normal
		float u;			 //!< Barycentric u coordinate of hit
		float v;			 //!< Barycentric v coordinate of hit
		unsigned int primID; //!< primitive ID
		unsigned int geomID; //!< geometry ID
		unsigned int instID; //!< instance ID
	};

	bool intersect(Ray& p_ray);
	void intersect(Vector<Ray>& r_rays);

	void add_mesh(const Vector<Vector3>& p_vertices, const Vector<Vector3>& p_normals,
		const Vector<Vector2>& p_uv2s, unsigned int p_id);
	void set_mesh_alpha_texture(Ref<Image> p_alpha_texture, unsigned int p_id);
	void commit();

	void set_mesh_filter(const HashSet<int>& p_mesh_ids);
	void clear_mesh_filter();

	static Ref<LightmapRaycaster> create();

	LightmapRaycaster();
	~LightmapRaycaster();
};

class Lightmapper : public RefCounted
{
public:
	struct RDState;

private:
	RDState* rd_state = nullptr;

public:
	enum GenerateProbes
	{
		GENERATE_PROBES_DISABLED,
		GENERATE_PROBES_SUBDIV_4,
		GENERATE_PROBES_SUBDIV_8,
		GENERATE_PROBES_SUBDIV_16,
		GENERATE_PROBES_SUBDIV_32,
	};

	enum LightType
	{
		LIGHT_TYPE_DIRECTIONAL,
		LIGHT_TYPE_OMNI,
		LIGHT_TYPE_SPOT,
		LIGHT_TYPE_AREA,
	};

	enum BakeError
	{
		BAKE_OK,
		BAKE_ERROR_TEXTURE_EXCEEDS_MAX_SIZE,
		BAKE_ERROR_LIGHTMAP_CANT_PRE_BAKE_MESHES,
		BAKE_ERROR_ATLAS_TOO_SMALL,
		BAKE_ERROR_USER_ABORTED,
	};

	enum BakeQuality
	{
		BAKE_QUALITY_LOW,
		BAKE_QUALITY_MEDIUM,
		BAKE_QUALITY_HIGH,
		BAKE_QUALITY_ULTRA,
	};

	typedef bool (*BakeStepFunc)(
		float, const String&, void*, bool); // step index, step total, step description, userdata

	struct MeshData
	{
		// triangle data
		Vector<Vector3> points;
		Vector<Vector2> uv2;
		Vector<Vector3> normal;
		Vector<RID> material;
		Ref<Image> albedo_on_uv2;
		Ref<Image> emission_on_uv2;
	};

	void add_mesh(const MeshData& p_mesh);
	void add_directional_light(const String& p_name, bool p_static, const Vector3& p_direction,
		const Color& p_color, float p_energy, float p_indirect_energy, float p_angular_distance,
		float p_shadow_blur);
	void add_omni_light(const String& p_name, bool p_static, const Vector3& p_position,
		const Color& p_color, float p_energy, float p_indirect_energy, float p_range,
		float p_attenuation, float p_size, float p_shadow_blur);
	void add_spot_light(const String& p_name, bool p_static, const Vector3& p_position,
		const Vector3& p_direction, const Color& p_color, float p_energy, float p_indirect_energy,
		float p_range, float p_attenuation, float p_spot_angle, float p_spot_attenuation,
		float p_size, float p_shadow_blur);
	void add_area_light(const String& p_name, bool p_static, const Vector3& p_position,
		const Vector3& p_direction, const Color& p_color, float p_energy, float p_indirect_energy,
		float p_range, float p_attenuation, const Vector3& p_area_width,
		const Vector3& p_area_height, float p_size, float p_shadow_blur,
		const Rect2& p_texture_rect, float p_max_mipmap);
	void add_area_light_atlas(
		const Vector2i& p_size, int p_mipmap_count, const PackedByteArray& p_atlas_data);
	void add_probe(const Vector3& p_position);
	BakeError bake(BakeQuality p_quality, bool p_use_denoiser, float p_denoiser_strength,
		int p_denoiser_range, int p_bounces, float p_bounce_indirect_energy, float p_bias,
		int p_max_texture_size, bool p_bake_sh, bool p_bake_shadowmask, bool p_texture_for_bounces,
		GenerateProbes p_generate_probes, const Ref<Image>& p_environment_panorama,
		const Basis& p_environment_transform, BakeStepFunc p_step_function = nullptr,
		void* p_step_userdata = nullptr, float p_exposure_normalization = 1.0,
		float p_supersampling_factor = 1.0);

	int get_bake_texture_count() const;
	Ref<Image> get_bake_texture(int p_index) const;
	int get_shadowmask_texture_count() const;
	Ref<Image> get_shadowmask_texture(int p_index) const;
	int get_bake_mesh_count() const;
	Rect2 get_bake_mesh_uv_scale(int p_index) const;
	int get_bake_mesh_texture_slice(int p_index) const;

	int get_bake_probe_count() const;
	Vector3 get_bake_probe_point(int p_probe) const;
	Vector<Color> get_bake_probe_sh(int p_probe) const;

	static Ref<Lightmapper> create();

	Lightmapper();
	~Lightmapper();
};

// --- End scene/3d/lightmapper.h ---

// --- Folded from scene/3d/multimesh_instance_3d.h ---

// --- Inlined/Folded from scene/resources/multimesh.h ---
#include "scene/resources/mesh.h"
#include "scene/resources/texture.h"

class MultiMesh : public Resource
{
public:
	enum TransformFormat
	{
		TRANSFORM_2D = RSE::MULTIMESH_TRANSFORM_2D,
		TRANSFORM_3D = RSE::MULTIMESH_TRANSFORM_3D
	};

	enum PhysicsInterpolationQuality
	{
		INTERP_QUALITY_FAST,
		INTERP_QUALITY_HIGH,
	};

private:
	Ref<Mesh> mesh;
	RID multimesh;
	TransformFormat transform_format = TRANSFORM_2D;
	AABB custom_aabb;
	bool use_colors = false;
	bool use_custom_data = false;
	int instance_count = 0;
	int visible_instance_count = -1;
	PhysicsInterpolationQuality _physics_interpolation_quality = INTERP_QUALITY_FAST;

protected:

#ifndef DISABLE_DEPRECATED
	// Kept for compatibility from 3.x to 4.0.
	void _set_transform_array(const Vector<Vector3>& p_array);
	Vector<Vector3> _get_transform_array() const;

	void _set_transform_2d_array(const Vector<Vector2>& p_array);
	Vector<Vector2> _get_transform_2d_array() const;

	void _set_color_array(const Vector<Color>& p_array);
	Vector<Color> _get_color_array() const;

	void _set_custom_data_array(const Vector<Color>& p_array);
	Vector<Color> _get_custom_data_array() const;
#endif
	void set_buffer(const Vector<float>& p_buffer);
	Vector<float> get_buffer() const;

	void set_buffer_interpolated(
		const Vector<float>& p_buffer_curr, const Vector<float>& p_buffer_prev);

public:
	void set_mesh(const Ref<Mesh>& p_mesh);
	Ref<Mesh> get_mesh() const;

	void set_use_colors(bool p_enable);
	bool is_using_colors() const;

	void set_use_custom_data(bool p_enable);
	bool is_using_custom_data() const;

	void set_transform_format(TransformFormat p_transform_format);
	TransformFormat get_transform_format() const;

	void set_instance_count(int p_count);
	int get_instance_count() const;

	void set_visible_instance_count(int p_count);
	int get_visible_instance_count() const;

	void set_physics_interpolation_quality(PhysicsInterpolationQuality p_quality);

	PhysicsInterpolationQuality get_physics_interpolation_quality() const
	{
		return _physics_interpolation_quality;
	}

	void set_instance_transform(int p_instance, const Transform3D& p_transform);
	void set_instance_transform_2d(int p_instance, const Transform2D& p_transform);
	Transform3D get_instance_transform(int p_instance) const;
	Transform2D get_instance_transform_2d(int p_instance) const;

	void set_instance_color(int p_instance, const Color& p_color);
	Color get_instance_color(int p_instance) const;

	void set_instance_custom_data(int p_instance, const Color& p_custom_data);
	Color get_instance_custom_data(int p_instance) const;

	void reset_instance_physics_interpolation(int p_instance);
	void reset_instances_physics_interpolation();

	void set_physics_interpolated(bool p_interpolated);

	void set_custom_aabb(const AABB& p_custom);
	AABB get_custom_aabb() const;

	virtual AABB get_aabb() const;

	virtual RID get_rid() const override;

	MultiMesh();
	~MultiMesh();
};

#ifndef NAVIGATION_3D_DISABLED
class NavigationMesh;
class NavigationMeshSourceGeometryData3D;
#endif // NAVIGATION_3D_DISABLED

class MultiMeshInstance3D : public Node3D
{
public:
	RenderInstance3D render_instance;

private:
	Ref<MultiMesh> multimesh;

	void _refresh_interpolated();

protected:
	virtual void _physics_interpolated_changed() override;
	virtual void fti_update_servers_xform() override;
	void _notification(int p_what);

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	void set_multimesh(const Ref<MultiMesh>& p_multimesh);
	Ref<MultiMesh> get_multimesh() const;

	AABB get_aabb() const;

private:
#ifndef NAVIGATION_3D_DISABLED
	static RID _navmesh_source_geometry_parser;
#endif // NAVIGATION_3D_DISABLED

public:
#ifndef NAVIGATION_3D_DISABLED
	static void navmesh_parse_init();
	static void navmesh_parse_source_geometry(const Ref<NavigationMesh>& p_navigation_mesh,
		Ref<NavigationMeshSourceGeometryData3D> p_source_geometry_data, Node* p_node);
#endif // NAVIGATION_3D_DISABLED

	MultiMeshInstance3D() = default;
	~MultiMeshInstance3D() = default;
};

// --- End scene/3d/multimesh_instance_3d.h ---

// --- Folded from scene/3d/camera_3d.h ---
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
#include "servers/rendering/renderer.h"

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

#include "servers/display/display_server.h"

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
// --- End scene/3d/camera_3d.h ---
// --- End scene/3d/visual_instance_3d.h ---

// --- Folded from scene/3d/skeleton_3d.h ---
#include "core/types.h"

// --- Inlined/Folded from scene/resources/3d/skin.h ---
#include "core/io/resource.h"

class Skin : public Resource
{
	struct Bind
	{
		int bone = -1;
		String name;
		Transform3D pose;
	};

	Vector<Bind> binds;

	Bind* binds_ptr = nullptr;
	int bind_count = 0;

protected:
	virtual void reset_state() override;

public:
	void set_bind_count(int p_size);

	inline int get_bind_count() const { return bind_count; }

	void add_bind(int p_bone, const Transform3D& p_pose);
	void add_named_bind(const String& p_name, const Transform3D& p_pose);

	void set_bind_bone(int p_index, int p_bone);
	void set_bind_pose(int p_index, const Transform3D& p_pose);
	void set_bind_name(int p_index, const String& p_name);

	inline int get_bind_bone(int p_index) const
	{
		ERR_FAIL_INDEX_V(p_index, bind_count, -1);
		return binds_ptr[p_index].bone;
	}

	inline String get_bind_name(int p_index) const
	{
		ERR_FAIL_INDEX_V(p_index, bind_count, String());
		return binds_ptr[p_index].name;
	}

	inline Transform3D get_bind_pose(int p_index) const
	{
		ERR_FAIL_INDEX_V(p_index, bind_count, Transform3D());
		return binds_ptr[p_index].pose;
	}

	void clear_binds();

	Skin() = default;
};

typedef int BoneId;

class Skeleton3D;

class SkinReference : public RefCounted
{
	friend class Skeleton3D;

	Skeleton3D* skeleton_node = nullptr;
	RID skeleton;
	Ref<Skin> skin;
	uint32_t bind_count = 0;
	uint64_t skeleton_version = 0;
	Vector<uint32_t> skin_bone_indices;
	uint32_t* skin_bone_indices_ptrs = nullptr;

protected:

public:
	// Public for use as signal callback.
	void _skin_changed();

	RID get_skeleton() const;
	Ref<Skin> get_skin() const;
	~SkinReference();
};

class Skeleton3D : public Node3D
{
#ifdef TOOLS_ENABLED
	bool saving = false;
#endif // TOOLS_ENABLED

#if !defined(DISABLE_DEPRECATED) && !defined(PHYSICS_3D_DISABLED)
	bool animate_physical_bones = true;
	Node* simulator = nullptr;
#endif // _DISABLE_DEPRECATED && PHYSICS_3D_DISABLED

public:
	enum ModifierCallbackModeProcess
	{
		MODIFIER_CALLBACK_MODE_PROCESS_PHYSICS,
		MODIFIER_CALLBACK_MODE_PROCESS_IDLE,
		MODIFIER_CALLBACK_MODE_PROCESS_MANUAL,
	};

private:
	friend class SkinReference;

	enum UpdateFlag
	{
		UPDATE_FLAG_NONE = 1,
		UPDATE_FLAG_MODIFIER = 2,
		UPDATE_FLAG_POSE = 4,
	};

	void _update_deferred(UpdateFlag p_update_flag = UPDATE_FLAG_POSE);
	uint8_t update_flags = UPDATE_FLAG_NONE;
	bool updating = false; // Is updating now?
	double update_delta = 0.0;

	struct Bone
	{
		String name;

		int parent = -1;
		Vector<int> child_bones;

		Transform3D rest;
		Transform3D global_rest;

		bool enabled = true;
		bool pose_cache_dirty = true;
		Transform3D pose_cache;
		Vector3 pose_position;
		Quaternion pose_rotation;
		Vector3 pose_scale = Vector3(1, 1, 1);
		Transform3D global_pose;
		int nested_set_offset = 0; // Offset in nested set of bone hierarchy.
		int nested_set_span = 0;   // Subtree span in nested set of bone hierarchy.

		bool modifier_applied = false;
		Transform3D modifier_pose_cache;

		void make_bone_modified()
		{
			modifier_applied = true;
			modifier_pose_cache = pose_cache;
		}

		void update_pose_cache()
		{
			if (pose_cache_dirty) {
				pose_cache.basis.set_quaternion_scale(pose_rotation, pose_scale);
				pose_cache.origin = pose_position;
				pose_cache_dirty = false;
			}
		}

#ifndef DISABLE_DEPRECATED
		Transform3D pose_global_no_override;
		real_t global_pose_override_amount = 0.0;
		bool global_pose_override_reset = false;
		Transform3D global_pose_override;
#endif // _DISABLE_DEPRECATED
	};

	struct BonePoseBackup
	{
		Transform3D pose_cache;
		Vector3 pose_position;
		Quaternion pose_rotation;
		Vector3 pose_scale = Vector3(1, 1, 1);
		Transform3D global_pose;

		void save(const Bone& p_bone)
		{
			pose_cache = p_bone.pose_cache;
			pose_position = p_bone.pose_position;
			pose_rotation = p_bone.pose_rotation;
			pose_scale = p_bone.pose_scale;
			global_pose = p_bone.global_pose;
		}

		void restore(Bone& r_bone)
		{
			r_bone.pose_cache = pose_cache;
			r_bone.pose_position = pose_position;
			r_bone.pose_rotation = pose_rotation;
			r_bone.pose_scale = pose_scale;
			r_bone.global_pose = global_pose;
		}
	};

	HashSet<SkinReference*> skin_bindings;
	void _skin_changed();

	mutable LocalVector<Bone> bones;
	mutable bool process_order_dirty = false;

	mutable Vector<int> parentless_bones;
	AHashMap<String, int> name_to_bone_index;

	mutable String concatenated_bone_names;
	void _update_bone_names() const;

	void _make_dirty();
	mutable bool dirty = false;
	mutable bool rest_dirty = false;

	bool show_rest_only = false;
	float motion_scale = 1.0;

	uint64_t version = 1;

	void _update_process_order() const;

	// To process modifiers.
	ModifierCallbackModeProcess modifier_callback_mode_process =
		MODIFIER_CALLBACK_MODE_PROCESS_IDLE;
	bool modifiers_dirty = false;
	void _find_modifiers();
	void _process_modifiers();
	void _process_changed();
	void _make_modifiers_dirty();
	bool modifier_updating = false; // Is modifier updating now?

	// Global bone pose calculation.
	mutable LocalVector<int>
		nested_set_offset_to_bone_index; // Map from Bone::nested_set_offset to bone index.
	mutable LocalVector<bool> bone_global_pose_dirty; // Indexable with Bone::nested_set_offset.
	void _update_bones_nested_set() const;
	int _update_bone_nested_set(int p_bone, int p_offset) const;
	void _make_bone_global_poses_dirty() const;
	void _make_bone_global_pose_subtree_dirty(int p_bone) const;
	void _update_bone_global_pose(int p_bone) const;

#ifndef DISABLE_DEPRECATED
	void _add_bone_bind_compat_88791(const String& p_name);

	static void _bind_compatibility_methods();
#endif // DISABLE_DEPRECATED

protected:
	void _notification(int p_what);
	TypedArray<String> _get_bone_meta_list_bind(int p_bone) const;

	virtual void add_child_notify(Node* p_child) override;
	virtual void move_child_notify(Node* p_child) override;
	virtual void remove_child_notify(Node* p_child) override;

public:
	enum
	{
		NOTIFICATION_UPDATE_SKELETON = 50
	};

	// Skeleton creation API
	uint64_t get_version() const;
	int add_bone(const String& p_name);
	int find_bone(const String& p_name) const;
	String get_bone_name(int p_bone) const;
	void set_bone_name(int p_bone, const String& p_name);
	String get_concatenated_bone_names() const;

	bool is_bone_parent_of(int p_bone_id, int p_parent_bone_id) const;

	void set_bone_parent(int p_bone, int p_parent);
	int get_bone_parent(int p_bone) const;

	void unparent_bone_and_rest(int p_bone);

	Vector<int> get_bone_children(int p_bone) const;
	Vector<int> get_parentless_bones() const;

	int get_bone_count() const;

	void set_bone_rest(int p_bone, const Transform3D& p_rest);
	Transform3D get_bone_rest(int p_bone) const;
	Transform3D get_bone_global_rest(int p_bone) const;

	void set_bone_enabled(int p_bone, bool p_enabled);
	bool is_bone_enabled(int p_bone) const;

	void set_show_rest_only(bool p_enabled);
	bool is_show_rest_only() const;
	void clear_bones();

	void set_motion_scale(float p_motion_scale);
	float get_motion_scale() const;

	// bone metadata
	void get_bone_meta_list(int p_bone, List<String>* p_list) const;
	bool has_bone_meta(int p_bone, const String& p_key) const;

	// Posing API
	Transform3D get_bone_pose(int p_bone) const;
	Vector3 get_bone_pose_position(int p_bone) const;
	Quaternion get_bone_pose_rotation(int p_bone) const;
	Vector3 get_bone_pose_scale(int p_bone) const;
	void set_bone_pose(int p_bone, const Transform3D& p_pose);
	void set_bone_pose_position(int p_bone, const Vector3& p_position);
	void set_bone_pose_rotation(int p_bone, const Quaternion& p_rotation);
	void set_bone_pose_scale(int p_bone, const Vector3& p_scale);

	Transform3D get_bone_global_pose(int p_bone) const;
	void set_bone_global_pose(int p_bone, const Transform3D& p_pose);

	void reset_bone_pose(int p_bone);
	void reset_bone_poses();

	void localize_rests(); // Used for loaders and tools.

	Ref<Skin> create_skin_from_rest_transforms();

	Ref<SkinReference> register_skin(const Ref<Skin>& p_skin);

	void force_update_all_dirty_bones();
	void _force_update_all_dirty_bones() const;
	void force_update_all_bone_transforms();
	void _force_update_all_bone_transforms() const;
	void force_update_bone_children_transforms(int bone_idx);
	void _force_update_bone_children_transforms(int bone_idx) const;
	void force_update_deferred();

	void set_modifier_callback_mode_process(ModifierCallbackModeProcess p_mode);
	ModifierCallbackModeProcess get_modifier_callback_mode_process() const;

	void advance(double p_delta);

#ifndef DISABLE_DEPRECATED
	Transform3D get_bone_global_pose_no_override(int p_bone) const;
	void clear_bones_global_pose_override();
	Transform3D get_bone_global_pose_override(int p_bone) const;
	void set_bone_global_pose_override(
		int p_bone, const Transform3D& p_pose, real_t p_amount, bool p_persistent = false);

	Node* get_simulator();
#ifndef PHYSICS_3D_DISABLED
	void set_animate_physical_bones(bool p_enabled);
	bool get_animate_physical_bones() const;
	void physical_bones_stop_simulation();
	void physical_bones_start_simulation_on(const TypedArray<String>& p_bones);
	void physical_bones_add_collision_exception(RID p_exception);
	void physical_bones_remove_collision_exception(RID p_exception);
#endif // PHYSICS_3D_DISABLED
#endif // _DISABLE_DEPRECATED

public:
	Skeleton3D();
	~Skeleton3D();
};

// --- Folded from scene/3d/skeleton_modifier_3d.h ---


class SkeletonModifier3D : public Node3D
{
	void rebind();

public:
	// For the case to indicate bone axis on basis without custom vector.
	enum BoneAxis
	{
		BONE_AXIS_PLUS_X,
		BONE_AXIS_MINUS_X,
		BONE_AXIS_PLUS_Y,
		BONE_AXIS_MINUS_Y,
		BONE_AXIS_PLUS_Z,
		BONE_AXIS_MINUS_Z,
	};

	static String get_hint_bone_axis() { return "+X,-X,+Y,-Y,+Z,-Z"; }

	// For the case to indicate Head-Tail of the bone.
	enum BoneDirection
	{
		BONE_DIRECTION_PLUS_X,
		BONE_DIRECTION_MINUS_X,
		BONE_DIRECTION_PLUS_Y,
		BONE_DIRECTION_MINUS_Y,
		BONE_DIRECTION_PLUS_Z,
		BONE_DIRECTION_MINUS_Z,
		BONE_DIRECTION_FROM_PARENT,
	};

	static String get_hint_bone_direction() { return "+X,-X,+Y,-Y,+Z,-Z,FromParent"; }

	// For the case to define secondary axis of the bone local space.
	enum SecondaryDirection
	{
		SECONDARY_DIRECTION_NONE,
		SECONDARY_DIRECTION_PLUS_X,
		SECONDARY_DIRECTION_MINUS_X,
		SECONDARY_DIRECTION_PLUS_Y,
		SECONDARY_DIRECTION_MINUS_Y,
		SECONDARY_DIRECTION_PLUS_Z,
		SECONDARY_DIRECTION_MINUS_Z,
		SECONDARY_DIRECTION_CUSTOM,
	};

	static String get_hint_secondary_direction() { return "None,+X,-X,+Y,-Y,+Z,-Z,Custom"; }

	// For the case to define rotation direction without identification plus/minus.
	enum RotationAxis
	{
		ROTATION_AXIS_X,
		ROTATION_AXIS_Y,
		ROTATION_AXIS_Z,
		ROTATION_AXIS_ALL,
		ROTATION_AXIS_CUSTOM,
	};

	static String get_hint_rotation_axis() { return "X,Y,Z,All,Custom"; }

protected:
	bool active = true;
	real_t influence = 1.0;

	void _update_skeleton_path();

	virtual void _skeleton_changed(Skeleton3D* p_old, Skeleton3D* p_new);
	virtual void _validate_bone_names();

	virtual void _set_active(bool p_active);
	virtual void _process_modification(double p_delta);
	bool should_check_node_path();

public:
	virtual PackedStringArray get_configuration_warnings() const override;

	Skeleton3D* get_skeleton() const;

	virtual bool has_process() const
	{
		return false;
	} // Return true if modifier needs to modify bone pose without external animation such as
	  // physics, jiggle and etc.

	bool is_active() const;
	void set_influence(real_t p_influence);
	real_t get_influence() const;

	void process_modification(double p_delta);

	// Utility APIs.
	static Vector3 get_vector_from_bone_axis(BoneAxis p_axis);
	static Vector3 get_vector_from_axis(Vector3::Axis p_axis);
	static Vector3::Axis get_axis_from_bone_axis(BoneAxis p_axis);

	// 3D math.
	static Vector3 limit_length(
		const Vector3& p_origin, const Vector3& p_destination, float p_length);
	static Quaternion get_local_pose_rotation(
		Skeleton3D* p_skeleton, int p_bone, const Quaternion& p_global_pose_rotation);
	static Quaternion get_from_to_rotation(
		const Vector3& p_from, const Vector3& p_to, const Quaternion& p_prev_rot);
	static Quaternion get_from_to_rotation_by_axis(
		const Vector3& p_from, const Vector3& p_to, const Vector3& p_axis);
	static Quaternion get_swing(const Quaternion& p_rotation, const Vector3& p_axis);
	static Vector3 snap_vector_to_plane(const Vector3& p_plane_normal, const Vector3& p_vector);
	static double symmetrize_angle(double p_angle);
	static double get_roll_angle(const Quaternion& p_rotation, const Vector3& p_roll_axis);
	static Vector3 get_projected_normal(
		const Vector3& p_a, const Vector3& p_b, const Vector3& p_point);

#ifdef TOOLS_ENABLED
	virtual bool is_processed_on_saving() const { return false; }
#endif

	SkeletonModifier3D();
};

// --- Folded from scene/3d/bone_constraint_3d.h ---

#include "scene/resources/material.h"

class BoneConstraint3D : public SkeletonModifier3D {
public:
	enum ConstraintType {
		CONSTRAINT_AIM,
		CONSTRAINT_CONVERT_TRANSFORM,
		CONSTRAINT_COPY_TRANSFORM,
		CONSTRAINT_BONE_TARGET,
		CONSTRAINT_LIMIT_ANGULAR_VELOCITY,
		CONSTRAINT_TWIST_DISPERSER,

		TYPE_AIM = CONSTRAINT_AIM,
		TYPE_CONVERT_TRANSFORM = CONSTRAINT_CONVERT_TRANSFORM,
		TYPE_COPY_TRANSFORM = CONSTRAINT_COPY_TRANSFORM,
		TYPE_BONE_TARGET = CONSTRAINT_BONE_TARGET,
		TYPE_LIMIT_ANGULAR_VELOCITY = CONSTRAINT_LIMIT_ANGULAR_VELOCITY,
		TYPE_TWIST_DISPERSER = CONSTRAINT_TWIST_DISPERSER,
	};

	enum ReferenceType {
		REFERENCE_TYPE_BONE,
		REFERENCE_TYPE_NODE,
	};

	enum TransformMode {
		TRANSFORM_MODE_POSITION,
		TRANSFORM_MODE_ROTATION,
		TRANSFORM_MODE_SCALE,
	};

	enum TransformFlag {
		TRANSFORM_FLAG_POSITION = 1,
		TRANSFORM_FLAG_ROTATION = 2,
		TRANSFORM_FLAG_SCALE = 4,
		TRANSFORM_FLAG_ALL = TRANSFORM_FLAG_POSITION | TRANSFORM_FLAG_ROTATION | TRANSFORM_FLAG_SCALE,
	};

	enum AxisFlag {
		AXIS_FLAG_X = 1,
		AXIS_FLAG_Y = 2,
		AXIS_FLAG_Z = 4,
		AXIS_FLAG_ALL = AXIS_FLAG_X | AXIS_FLAG_Y | AXIS_FLAG_Z,
	};

	enum DisperseMode {
		DISPERSE_MODE_EVEN,
		DISPERSE_MODE_WEIGHTED,
		DISPERSE_MODE_CUSTOM,
	};

	struct BoneJoint {
		String name;
		int bone = -1;
	};

	struct DisperseJointSetting {
		BoneJoint joint;
		double custom_amount = 1.0;
		double amount = 1.0;
		Vector3 axis;
	};

	typedef Pair<int, Quaternion> BoneRot;

	struct ConstraintSetting {
		float amount = 1.0f;

		String apply_bone_name;
		int apply_bone = -1;

		ReferenceType reference_type = REFERENCE_TYPE_BONE;
		String reference_bone_name;
		int reference_bone = -1;
		NodePath reference_node;

		bool relative = true;
		bool additive = false;

		// Aim settings.
		BoneAxis forward_axis = BONE_AXIS_PLUS_Y;
		bool use_euler = false;
		Vector3::Axis primary_rotation_axis = Vector3::AXIS_X;
		bool use_secondary_rotation = true;

		// Convert Transform settings.
		TransformMode apply_transform_mode = TRANSFORM_MODE_POSITION;
		Vector3::Axis apply_axis = Vector3::AXIS_X;
		float apply_range_min = 0.0f;
		float apply_range_max = 0.0f;
		TransformMode reference_transform_mode = TRANSFORM_MODE_POSITION;
		Vector3::Axis reference_axis = Vector3::AXIS_X;
		float reference_range_min = 0.0f;
		float reference_range_max = 0.0f;

		// Copy Transform settings.
		uint32_t copy_flags = TRANSFORM_FLAG_ALL;
		uint32_t axis_flags = AXIS_FLAG_ALL;
		uint32_t invert_flags = 0;

		// Chain & Twist Disperser settings (shared by LIMIT_ANGULAR_VELOCITY and TWIST_DISPERSER).
		BoneJoint root_bone;
		BoneJoint end_bone;
		bool joints_dirty = false;
		DisperseMode disperse_mode = DISPERSE_MODE_EVEN;
		bool twist_from_rest = true;
		Quaternion twist_from;
		LocalVector<DisperseJointSetting> joints;
		bool extend_end_bone = false;
		BoneDirection end_bone_direction = BONE_DIRECTION_FROM_PARENT;
		float weight_position = 0.5f;
		Ref<Curve> damping_curve;

		bool is_relative_effective(ConstraintType p_type) const {
			if (p_type != CONSTRAINT_AIM && reference_type == REFERENCE_TYPE_NODE) {
				return false;
			}
			return relative;
		}
	};

protected:
	ConstraintType constraint_type = CONSTRAINT_COPY_TRANSFORM;
	LocalVector<ConstraintSetting> settings;

	// BoneTarget state.
	String target_bone_name;
	int target_bone = -1;

	// LimitAngularVelocity state.
	bool exclude = false;
	double max_angular_velocity = Math::TAU;
	LocalVector<BoneJoint> velocity_joints;
	LocalVector<BoneRot> velocity_bones;
	bool velocity_joints_dirty = false;
	bool velocity_init_needed = true;

	// TwistDisperser state.
	bool mutable_bone_axes = true;

	void _notification(int p_what);
	virtual PackedStringArray get_configuration_warnings() const override;
	virtual void _set_active(bool p_active) override;
	virtual void _skeleton_changed(Skeleton3D *p_old, Skeleton3D *p_new) override;
	virtual void _validate_bone_names() override;
	virtual void _process_modification(double p_delta) override;

	void _process_aim(int p_index, Skeleton3D *p_skeleton, int p_apply_bone, const Vector3 &p_target, float p_amount);
	void _process_convert(int p_index, Skeleton3D *p_skeleton, int p_apply_bone, const Transform3D &p_destination, float p_amount);
	void _process_copy(int p_index, Skeleton3D *p_skeleton, int p_apply_bone, const Transform3D &p_destination, float p_amount);
	void _process_bone_target(Skeleton3D *p_skeleton);
	void _process_limit_angular_velocity(Skeleton3D *p_skeleton, double p_delta);
	void _process_twist_disperser(Skeleton3D *p_skeleton);

	void _make_velocity_joints_dirty();
	void _update_velocity_joints();
	bool _is_velocity_joint_contained(int p_bone) const;

	void _make_all_twist_joints_dirty();
	void _make_twist_joints_dirty(int p_index);
	void _update_twist_joints(int p_index);
	void _update_twist_curve(int p_index);

public:
	BoneConstraint3D(ConstraintType p_type = CONSTRAINT_COPY_TRANSFORM);
	~BoneConstraint3D();

	void set_constraint_type(ConstraintType p_type);
	ConstraintType get_constraint_type() const { return constraint_type; }

#ifdef TOOLS_ENABLED
	virtual bool is_processed_on_saving() const override { return constraint_type == CONSTRAINT_BONE_TARGET; }
#endif // TOOLS_ENABLED

	virtual bool has_process() const override { return constraint_type == CONSTRAINT_LIMIT_ANGULAR_VELOCITY; }

	void set_setting_count(int p_count);
	int get_setting_count() const;
	void clear_settings();

	// Common setting properties.
	void set_amount(int p_index, float p_amount);
	float get_amount(int p_index) const;

	void set_apply_bone_name(int p_index, const String &p_name);
	String get_apply_bone_name(int p_index) const;
	void set_apply_bone(int p_index, int p_bone);
	int get_apply_bone(int p_index) const;

	void set_reference_type(int p_index, ReferenceType p_type);
	ReferenceType get_reference_type(int p_index) const;

	void set_reference_bone_name(int p_index, const String &p_name);
	String get_reference_bone_name(int p_index) const;
	void set_reference_bone(int p_index, int p_bone);
	int get_reference_bone(int p_index) const;

	void set_reference_node(int p_index, const NodePath &p_node);
	NodePath get_reference_node(int p_index) const;

	void set_relative(int p_index, bool p_enabled);
	bool is_relative(int p_index) const;
	void set_additive(int p_index, bool p_enabled);
	bool is_additive(int p_index) const;

	// Aim properties.
	void set_forward_axis(int p_index, BoneAxis p_axis);
	BoneAxis get_forward_axis(int p_index) const;
	void set_use_euler(int p_index, bool p_enabled);
	bool is_using_euler(int p_index) const;
	void set_primary_rotation_axis(int p_index, Vector3::Axis p_axis);
	Vector3::Axis get_primary_rotation_axis(int p_index) const;
	void set_use_secondary_rotation(int p_index, bool p_enabled);
	bool is_using_secondary_rotation(int p_index) const;

	// Convert Transform properties.
	void set_apply_transform_mode(int p_index, TransformMode p_transform_mode);
	TransformMode get_apply_transform_mode(int p_index) const;
	void set_apply_axis(int p_index, Vector3::Axis p_axis);
	Vector3::Axis get_apply_axis(int p_index) const;
	void set_apply_range_min(int p_index, float p_range_min);
	float get_apply_range_min(int p_index) const;
	void set_apply_range_max(int p_index, float p_range_max);
	float get_apply_range_max(int p_index) const;

	void set_reference_transform_mode(int p_index, TransformMode p_transform_mode);
	TransformMode get_reference_transform_mode(int p_index) const;
	void set_reference_axis(int p_index, Vector3::Axis p_axis);
	Vector3::Axis get_reference_axis(int p_index) const;
	void set_reference_range_min(int p_index, float p_range_min);
	float get_reference_range_min(int p_index) const;
	void set_reference_range_max(int p_index, float p_range_max);
	float get_reference_range_max(int p_index) const;

	// Copy Transform properties.
	void set_copy_flags(int p_index, uint32_t p_copy_flags);
	uint32_t get_copy_flags(int p_index) const;
	void set_copy_position(int p_index, bool p_enabled);
	bool is_position_copying(int p_index) const;
	void set_copy_rotation(int p_index, bool p_enabled);
	bool is_rotation_copying(int p_index) const;
	void set_copy_scale(int p_index, bool p_enabled);
	bool is_scale_copying(int p_index) const;

	void set_axis_flags(int p_index, uint32_t p_axis_flags);
	uint32_t get_axis_flags(int p_index) const;
	void set_axis_x_enabled(int p_index, bool p_enabled);
	bool is_axis_x_enabled(int p_index) const;
	void set_axis_y_enabled(int p_index, bool p_enabled);
	bool is_axis_y_enabled(int p_index) const;
	void set_axis_z_enabled(int p_index, bool p_enabled);
	bool is_axis_z_enabled(int p_index) const;

	void set_invert_flags(int p_index, uint32_t p_axis_flags);
	uint32_t get_invert_flags(int p_index) const;
	void set_axis_x_inverted(int p_index, bool p_enabled);
	bool is_axis_x_inverted(int p_index) const;
	void set_axis_y_inverted(int p_index, bool p_enabled);
	bool is_axis_y_inverted(int p_index) const;
	void set_axis_z_inverted(int p_index, bool p_enabled);
	bool is_axis_z_inverted(int p_index) const;

	// BoneTarget properties (CONSTRAINT_BONE_TARGET / TYPE_BONE_TARGET).
	void set_bone_name(const String &p_bone_name);
	String get_bone_name() const;
	void set_bone(int p_bone);
	int get_bone() const;

	// Chain root/end properties (shared by LIMIT_ANGULAR_VELOCITY and TWIST_DISPERSER).
	void set_root_bone_name(int p_index, const String &p_bone_name);
	String get_root_bone_name(int p_index) const;
	void set_root_bone(int p_index, int p_bone);
	int get_root_bone(int p_index) const;

	void set_end_bone_name(int p_index, const String &p_bone_name);
	String get_end_bone_name(int p_index) const;
	void set_end_bone(int p_index, int p_bone);
	int get_end_bone(int p_index) const;

	// LimitAngularVelocity properties (CONSTRAINT_LIMIT_ANGULAR_VELOCITY / TYPE_LIMIT_ANGULAR_VELOCITY).
	void set_chain_count(int p_count);
	int get_chain_count() const;
	void clear_chains();
	String get_joint_bone_name(int p_index) const;
	int get_joint_bone(int p_index) const;
	int get_velocity_joint_count() const;
	void set_max_angular_velocity(double p_angular_velocity);
	double get_max_angular_velocity() const;
	void set_exclude(bool p_exclude);
	bool is_exclude() const;
	void reset();

	// TwistDisperser properties (CONSTRAINT_TWIST_DISPERSER / TYPE_TWIST_DISPERSER).
	void set_mutable_bone_axes(bool p_enabled);
	bool are_bone_axes_mutable() const;
	void set_extend_end_bone(int p_index, bool p_enabled);
	bool is_end_bone_extended(int p_index) const;
	void set_end_bone_direction(int p_index, BoneDirection p_bone_direction);
	BoneDirection get_end_bone_direction(int p_index) const;
	void set_twist_from_rest(int p_index, bool p_enabled);
	bool is_twist_from_rest(int p_index) const;
	void set_twist_from(int p_index, const Quaternion &p_from);
	Quaternion get_twist_from(int p_index) const;
	void set_disperse_mode(int p_index, DisperseMode p_disperse_mode);
	DisperseMode get_disperse_mode(int p_index) const;
	void set_weight_position(int p_index, float p_position);
	float get_weight_position(int p_index) const;
	void set_damping_curve(int p_index, const Ref<Curve> &p_damping_curve);
	Ref<Curve> get_damping_curve(int p_index) const;
	String get_joint_bone_name(int p_index, int p_joint) const;
	int get_joint_bone(int p_index, int p_joint) const;
	void set_joint_twist_amount(int p_index, int p_joint, float p_amount);
	float get_joint_twist_amount(int p_index, int p_joint) const;
	void set_joint_count(int p_index, int p_count);
	int get_joint_count(int p_index) const;
};

// --- End scene/3d/bone_constraint_3d.h ---

// --- Folded from scene/resources/3d/joint_limitation_3d.h ---
#include "core/io/resource.h"

#ifdef TOOLS_ENABLED
#include "scene/resources/mesh.h"
#endif // TOOLS_ENABLED

class JointLimitation3D : public Resource
{
	real_t angle = Math::TAU * 0.25;

	Vector3 _solve(const Vector3& p_direction) const;

public:
	void set_angle(real_t p_angle);
	real_t get_angle() const;

	Quaternion make_space(const Vector3& p_local_forward_vector,
		const Vector3& p_local_right_vector, const Quaternion& p_rotation_offset) const;

	Vector3 solve(const Vector3& p_local_forward_vector, const Vector3& p_local_right_vector,
		const Quaternion& p_rotation_offset, const Vector3& p_local_current_vector) const;

#ifdef TOOLS_ENABLED
	void draw_shape(Ref<SurfaceTool>& p_surface_tool, const Transform3D& p_transform,
		float p_bone_length, const Color& p_color) const;
#endif // TOOLS_ENABLED
};

// --- End scene/resources/3d/joint_limitation_3d.h ---

// --- Folded from scene/3d/physics/collision_object_3d.h ---
#include "core/templates/local_vector.h"
#include "core/templates/hash_map.h"

#include "scene/resources/mesh.h"
#include "servers/physics_3d/physics_server_3d.h"

// Plain non-virtual composed component for 3D collision state (shared by PhysicsBody3D and Area3D).
// Supports both inline ShapeSlots directly on the body/area and child CollisionShape3D/CollisionPolygon3D owners.
struct CollisionState3D {
	enum DisableMode {
		DISABLE_MODE_REMOVE,
		DISABLE_MODE_MAKE_STATIC,
		DISABLE_MODE_KEEP_ACTIVE,
	};

	struct ShapeSlot {
		Transform3D local_xform;
		Ref<Shape3D> shape;
		bool disabled = false;
		uint32_t owner_id = 0;
	};

	struct ShapeData {
		Transform3D xform;
		Node *owner_node = nullptr;

		struct ShapeBase {
			RID debug_shape;
			Ref<Shape3D> shape;
			int index = 0;
		};

		Vector<ShapeBase> shapes;
		bool disabled = false;
	};

	RID rid;
	bool area = false;
	uint32_t collision_layer = 1;
	uint32_t collision_mask = 1;
	real_t collision_priority = 1.0f;
	uint32_t callback_lock = 0;
	DisableMode disable_mode = DISABLE_MODE_REMOVE;
	PS3DE::BodyMode body_mode = PS3DE::BODY_MODE_STATIC;
	bool only_update_transform_changes = false;
	bool capture_input_on_drag = false;
	bool ray_pickable = true;

	int total_subshapes = 0;
	uint32_t next_owner_id = 1;
	RBMap<uint32_t, ShapeData> shapes;
	LocalVector<ShapeSlot> inline_shapes;

	int debug_shapes_count = 0;
	Transform3D debug_shape_old_transform;

	void init(bool p_area, PS3DE::BodyMode p_mode = PS3DE::BODY_MODE_STATIC);
	void free_rid();

	_FORCE_INLINE_ void lock_callback() { callback_lock++; }
	_FORCE_INLINE_ void unlock_callback() {
		ERR_FAIL_COND(callback_lock == 0);
		callback_lock--;
	}

	void handle_notification(Node3D *p_owner, int p_what);
	void on_transform_changed(const Node3D *p_owner);
	void clear_debug_shapes();

	void apply_disabled(Node3D *p_owner);
	void apply_enabled(Node3D *p_owner);
	void update_pickable(const Node3D *p_owner);
	void set_body_mode(const Node3D *p_owner, PS3DE::BodyMode p_mode);

	void set_collision_layer(uint32_t p_layer);
	void set_collision_mask(uint32_t p_mask);
	void set_collision_layer_value(int p_layer_number, bool p_value);
	bool get_collision_layer_value(int p_layer_number) const;
	void set_collision_mask_value(int p_layer_number, bool p_value);
	bool get_collision_mask_value(int p_layer_number) const;
	void set_collision_priority(real_t p_priority);
	void set_disable_mode(Node3D *p_owner, DisableMode p_mode);
	void set_ray_pickable(const Node3D *p_owner, bool p_ray_pickable);

	// Inline shape slots (no child CollisionShape3D node required).
	int add_inline_shape(const Ref<Shape3D> &p_shape, const Transform3D &p_xform = Transform3D(), bool p_disabled = false);
	void set_inline_shape(int p_index, const Ref<Shape3D> &p_shape);
	Ref<Shape3D> get_inline_shape(int p_index) const;
	void set_inline_shape_transform(int p_index, const Transform3D &p_xform);
	Transform3D get_inline_shape_transform(int p_index) const;
	void set_inline_shape_disabled(int p_index, bool p_disabled);
	bool is_inline_shape_disabled(int p_index) const;
	void remove_inline_shape(int p_index);
	void clear_inline_shapes();
	int get_inline_shape_count() const { return (int)inline_shapes.size(); }

	// Shape owner API (for inline slots & child CollisionShape3D / CollisionPolygon3D).
	uint32_t create_shape_owner(Node *p_owner = nullptr);
	void remove_shape_owner(uint32_t p_owner);
	void get_shape_owners(List<uint32_t> *r_owners) const;
	PackedInt32Array get_shape_owners_array() const;

	void shape_owner_set_transform(uint32_t p_owner, const Transform3D &p_transform);
	Transform3D shape_owner_get_transform(uint32_t p_owner) const;
	void shape_owner_set_disabled(uint32_t p_owner, bool p_disabled);
	bool is_shape_owner_disabled(uint32_t p_owner) const;

	void shape_owner_add_shape(uint32_t p_owner, const Ref<Shape3D> &p_shape);
	int shape_owner_get_shape_count(uint32_t p_owner) const;
	Ref<Shape3D> shape_owner_get_shape(uint32_t p_owner, int p_shape) const;
	int shape_owner_get_shape_index(uint32_t p_owner, int p_shape) const;
	void shape_owner_remove_shape(uint32_t p_owner, int p_shape);
	void shape_owner_clear_shapes(uint32_t p_owner);
	uint32_t shape_find_owner(int p_shape_index) const;

	bool has_any_shapes() const { return !shapes.is_empty(); }
};

// --- Folded from scene/3d/physics/kinematic_collision_3d.h ---
#include "servers/physics_3d/physics_server_3d.h"

class KinematicCollision3D : public RefCounted
{
	friend class PhysicsBody3D;
	PS3DT::MotionResult result;

protected:

public:
	Vector3 get_travel() const;
	Vector3 get_remainder() const;
	int get_collision_count() const;
	real_t get_depth() const;
	Vector3 get_position(int p_collision_index = 0) const;
	Vector3 get_normal(int p_collision_index = 0) const;
	real_t get_angle(
		int p_collision_index = 0, const Vector3& p_up_direction = Vector3(0.0, 1.0, 0.0)) const;
	RID get_collider_rid(int p_collision_index = 0) const;
	int get_collider_shape_index(int p_collision_index = 0) const;
	Vector3 get_collider_velocity(int p_collision_index = 0) const;
};



// --- End scene/3d/physics/kinematic_collision_3d.h ---

// --- Folded from scene/3d/physics/physics_body_3d.h ---


#include "scene/resources/material.h"
#include "servers/physics_3d/physics_server_3d.h"

class PhysicsBody3D;
class PhysicalBoneSimulator3D;

class VehicleWheel3D : public Node3D {
	friend class PhysicsBody3D;

	struct WheelXform {
		Vector3 up;
		Vector3 right;
		Vector3 origin;
		real_t steering = 0.0f;
	};

	struct FTIData {
		real_t curr_rotation = 0.0f;
		real_t curr_rotation_delta = 0.0f;
		WheelXform curr;
		WheelXform prev;
		bool unset = true;
		bool reset_queued = false;

		void rotate(real_t p_delta) {
			curr_rotation += p_delta;
			curr_rotation_delta = p_delta;
			double wrapped = Math::fmod(curr_rotation + Math::PI, Math::TAU);
			if (wrapped < 0) {
				wrapped += Math::TAU;
			}
			curr_rotation = wrapped - Math::PI;
		}

		void pump() {
			prev = curr;
			curr_rotation_delta = 0.0f;
		}

		void update_world_xform(Transform3D &r_xform, real_t p_interpolation_fraction);
	} fti_data;

	Transform3D m_worldTransform;
	Transform3D local_xform;
	bool engine_traction = false;
	bool steers = false;

	Vector3 m_chassisConnectionPointCS;
	Vector3 m_wheelDirectionCS;
	Vector3 m_wheelAxleCS;

	real_t m_suspensionRestLength = 0.15f;
	real_t m_maxSuspensionTravel = 0.2f;
	real_t m_wheelRadius = 0.5f;

	real_t m_suspensionStiffness = 5.88f;
	real_t m_wheelsDampingCompression = 0.83f;
	real_t m_wheelsDampingRelaxation = 0.88f;
	real_t m_frictionSlip = 10.5f;
	real_t m_maxSuspensionForce = 6000.0f;
	bool m_bIsFrontWheel = false;

	PhysicsBody3D *body = nullptr;

	real_t m_steering = 0.0f;
	real_t m_rotation = 0.0f;
	real_t m_deltaRotation = 0.0f;
	real_t m_rpm = 0.0f;
	real_t m_rollInfluence = 0.1f;
	real_t m_engineForce = 0.0f;
	real_t m_brake = 0.0f;

	real_t m_clippedInvContactDotSuspension = 1.0f;
	real_t m_suspensionRelativeVelocity = 0.0f;
	real_t m_wheelsSuspensionForce = 0.0f;
	real_t m_skidInfo = 0.0f;

	struct RaycastInfo {
		Vector3 m_contactNormalWS;
		Vector3 m_contactPointWS;
		real_t m_suspensionLength = 0.0f;
		Vector3 m_hardPointWS;
		Vector3 m_wheelDirectionWS;
		Vector3 m_wheelAxleWS;
		bool m_isInContact = false;
		PhysicsBody3D *m_groundObject = nullptr;
	} m_raycastInfo;

	void _update(PhysicsDirectBodyState3D *s);

protected:
	void _notification(int p_what);

public:
	void set_radius(real_t p_radius) { m_wheelRadius = p_radius; }
	real_t get_radius() const { return m_wheelRadius; }

	void set_suspension_rest_length(real_t p_length) { m_suspensionRestLength = p_length; }
	real_t get_suspension_rest_length() const { return m_suspensionRestLength; }

	void set_suspension_travel(real_t p_length) { m_maxSuspensionTravel = p_length; }
	real_t get_suspension_travel() const { return m_maxSuspensionTravel; }

	void set_suspension_stiffness(real_t p_value) { m_suspensionStiffness = p_value; }
	real_t get_suspension_stiffness() const { return m_suspensionStiffness; }

	void set_suspension_max_force(real_t p_value) { m_maxSuspensionForce = p_value; }
	real_t get_suspension_max_force() const { return m_maxSuspensionForce; }

	void set_damping_compression(real_t p_value) { m_wheelsDampingCompression = p_value; }
	real_t get_damping_compression() const { return m_wheelsDampingCompression; }

	void set_damping_relaxation(real_t p_value) { m_wheelsDampingRelaxation = p_value; }
	real_t get_damping_relaxation() const { return m_wheelsDampingRelaxation; }

	void set_friction_slip(real_t p_value) { m_frictionSlip = p_value; }
	real_t get_friction_slip() const { return m_frictionSlip; }

	void set_use_as_traction(bool p_enable) { engine_traction = p_enable; }
	bool is_used_as_traction() const { return engine_traction; }

	void set_use_as_steering(bool p_enabled) { steers = p_enabled; }
	bool is_used_as_steering() const { return steers; }

	bool is_in_contact() const { return m_raycastInfo.m_isInContact; }
	Vector3 get_contact_point() const { return m_raycastInfo.m_contactPointWS; }
	Vector3 get_contact_normal() const { return m_raycastInfo.m_contactNormalWS; }
	Node3D *get_contact_body() const;

	void set_roll_influence(real_t p_value) { m_rollInfluence = p_value; }
	real_t get_roll_influence() const { return m_rollInfluence; }

	real_t get_skidinfo() const { return m_skidInfo; }
	real_t get_rpm() const { return m_rpm; }

	void set_engine_force(real_t p_engine_force) { m_engineForce = p_engine_force; }
	real_t get_engine_force() const { return m_engineForce; }

	void set_brake(real_t p_brake) { m_brake = p_brake; }
	real_t get_brake() const { return m_brake; }

	void set_steering(real_t p_steering) { m_steering = p_steering; }
	real_t get_steering() const { return m_steering; }

	virtual PackedStringArray get_configuration_warnings() const override;

	VehicleWheel3D() = default;
};

class PhysicsBody3D : public Node3D {
	friend class VehicleWheel3D;

public:
	enum BodyMode {
		MODE_STATIC,
		MODE_ANIMATABLE,
		MODE_CHARACTER,
		MODE_RIGID,
		MODE_VEHICLE,
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

	enum MotionMode {
		MOTION_MODE_GROUNDED,
		MOTION_MODE_FLOATING,
	};

	enum PlatformOnLeave {
		PLATFORM_ON_LEAVE_ADD_VELOCITY,
		PLATFORM_ON_LEAVE_ADD_UPWARD_VELOCITY,
		PLATFORM_ON_LEAVE_DO_NOTHING,
	};

	enum BoneJointType {
		BONE_JOINT_NONE,
		BONE_JOINT_PIN,
		BONE_JOINT_CONE,
		BONE_JOINT_HINGE,
		BONE_JOINT_SLIDER,
		BONE_JOINT_6DOF,
	};

	using DisableMode = CollisionState3D::DisableMode;

	// Plain composed character mover state (for MODE_CHARACTER).
	struct CharacterMover3D {
		struct FloorFlags {
			bool floor = false;
			bool wall = false;
			bool ceiling = false;

			FloorFlags() = default;
			FloorFlags(bool p_floor, bool p_wall, bool p_ceiling) :
					floor(p_floor), wall(p_wall), ceiling(p_ceiling) {}
		};

		real_t margin = 0.001f;
		MotionMode motion_mode = MOTION_MODE_GROUNDED;
		PlatformOnLeave platform_on_leave = PLATFORM_ON_LEAVE_ADD_VELOCITY;

		FloorFlags flags;
		bool floor_constant_speed = false;
		bool floor_stop_on_slope = true;
		bool floor_block_on_wall = true;
		bool slide_on_ceiling = true;
		int max_slides = 6;
		int platform_layer = 0;
		RID platform_rid;
		uint32_t platform_floor_layers = UINT32_MAX;
		uint32_t platform_wall_layers = 0;
		real_t floor_snap_length = 0.1f;
		real_t floor_max_angle = Math::deg_to_rad((real_t)45.0);
		real_t wall_min_slide_angle = Math::deg_to_rad((real_t)15.0);
		Vector3 up_direction = Vector3(0.0f, 1.0f, 0.0f);
		Vector3 velocity;
		Vector3 floor_normal;
		Vector3 wall_normal;
		Vector3 ceiling_normal;
		Vector3 last_motion;
		Vector3 platform_velocity;
		Vector3 platform_angular_velocity;
		Vector3 platform_ceiling_velocity;
		Vector3 previous_position;
		Vector3 real_velocity;

		Vector<PS3DT::MotionResult> motion_results;
		Vector<Ref<KinematicCollision3D>> slide_colliders;
	};

	// Plain composed vehicle rig state (for MODE_VEHICLE).
	struct VehicleRig3D {
		real_t engine_force = 0.0f;
		real_t brake = 0.0f;
		real_t pitch_control = 0.0f;
		real_t steering = 0.0f;
		real_t speed_km_hour = 0.0f;
		HashSet<RID> exclude;
		Vector<Vector3> forward_ws;
		Vector<Vector3> axle;
		Vector<real_t> forward_impulse;
		Vector<real_t> side_impulse;
		Vector<VehicleWheel3D *> wheels;
	};

	// Plain value-type joint configuration & bone attachment state (for MODE_PHYSICAL_BONE).
	struct PhysicalBoneState3D {
		struct SixDOFAxisParams {
			bool linear_limit_enabled = true;
			real_t linear_limit_upper = 0.0f;
			real_t linear_limit_lower = 0.0f;
			real_t linear_limit_softness = 0.7f;
			real_t linear_restitution = 0.5f;
			real_t linear_damping = 1.0f;
			bool linear_spring_enabled = false;
			real_t linear_spring_stiffness = 0.0f;
			real_t linear_spring_damping = 0.0f;
			real_t linear_equilibrium_point = 0.0f;
			bool angular_limit_enabled = true;
			real_t angular_limit_upper = 0.0f;
			real_t angular_limit_lower = 0.0f;
			real_t angular_limit_softness = 0.5f;
			real_t angular_restitution = 0.0f;
			real_t angular_damping = 1.0f;
			real_t erp = 0.5f;
			bool angular_spring_enabled = false;
			real_t angular_spring_stiffness = 0.0f;
			real_t angular_spring_damping = 0.0f;
			real_t angular_equilibrium_point = 0.0f;
		};

		struct JointParams {
			// Pin / Cone / Hinge shared parameters.
			real_t bias = 0.3f;
			real_t damping = 1.0f;
			real_t impulse_clamp = 0.0f;
			real_t swing_span = Math::PI * 0.25f;
			real_t twist_span = Math::PI;
			real_t softness = 0.8f;
			real_t relaxation = 1.0f;

			// Hinge / Slider limits.
			bool angular_limit_enabled = false;
			real_t angular_limit_upper = Math::PI * 0.5f;
			real_t angular_limit_lower = -Math::PI * 0.5f;
			real_t angular_limit_bias = 0.3f;
			real_t angular_limit_softness = 0.9f;
			real_t angular_limit_relaxation = 1.0f;
			real_t angular_limit_restitution = 0.7f;
			real_t angular_limit_damping = 1.0f;

			real_t linear_limit_upper = 1.0f;
			real_t linear_limit_lower = -1.0f;
			real_t linear_limit_softness = 1.0f;
			real_t linear_limit_restitution = 0.7f;
			real_t linear_limit_damping = 1.0f;

			SixDOFAxisParams six_dof_axes[3];
		};

		BoneJointType joint_type = BONE_JOINT_NONE;
		JointParams joint_params;
		Transform3D joint_offset;
		RID joint;
		Transform3D body_offset;
		Transform3D body_offset_inverse;
		bool simulate_physics = false;
		bool internal_simulate_physics = false;
		int bone_id = -1;
		String bone_name;
		real_t bounce = 0.0f;
		real_t friction = 1.0f;
#ifdef TOOLS_ENABLED
		bool gizmo_move_joint = false;
#endif
	};

private:
	BodyMode mode = MODE_STATIC;
	CollisionState3D collision_state;
	Ref<KinematicCollision3D> motion_cache;
	uint16_t locked_axis = 0;

	// Static / Animatable properties.
	Vector3 constant_linear_velocity;
	Vector3 constant_angular_velocity;
	bool sync_to_physics = true;
	Transform3D last_valid_transform;

	// Rigid / Vehicle / PhysicalBone properties.
	Ref<PhysicsMaterial> physics_material_override;
	bool can_sleep = true;
	bool lock_rotation = false;
	bool freeze = false;
	FreezeMode freeze_mode = FREEZE_MODE_STATIC;

	real_t mass = 1.0f;
	Vector3 inertia;
	CenterOfMassMode center_of_mass_mode = CENTER_OF_MASS_MODE_AUTO;
	Vector3 center_of_mass;
	Basis inverse_inertia_tensor;
	real_t gravity_scale = 1.0f;

	DampMode linear_damp_mode = DAMP_MODE_COMBINE;
	DampMode angular_damp_mode = DAMP_MODE_COMBINE;
	real_t linear_damp = 0.0f;
	real_t angular_damp = 0.0f;

	Vector3 linear_velocity;
	Vector3 angular_velocity;
	bool sleeping = false;
	bool ccd = false;
	int max_contacts_reported = 0;
	int contact_count = 0;
	bool custom_integrator = false;
	bool contact_monitor = false;

	CharacterMover3D character;
	VehicleRig3D vehicle;
	PhysicalBoneState3D bone_state;

	void _apply_server_body_mode();
	void _update_kinematic_motion();
	void _body_state_changed(PhysicsDirectBodyState3D *p_state);

	// Character helpers.
	void _set_collision_direction(const PS3DT::MotionResult &p_result, CharacterMover3D::FloorFlags &r_state, CharacterMover3D::FloorFlags p_apply_state = CharacterMover3D::FloorFlags(true, true, true));
	bool _on_floor_if_snapped(bool p_was_on_floor, bool p_vel_dir_facing_up);
	void _snap_on_floor(bool p_was_on_floor, bool p_vel_dir_facing_up);

	// Vehicle helpers.
	struct VehicleWheelContactPoint {
		PhysicsDirectBodyState3D *m_s = nullptr;
		PhysicsBody3D *m_body1 = nullptr;
		Vector3 m_frictionPositionWorld;
		Vector3 m_frictionDirectionWorld;
		real_t m_jacDiagABInv = 0.0f;
		real_t m_maxImpulse = 0.0f;

		VehicleWheelContactPoint(PhysicsDirectBodyState3D *s, PhysicsBody3D *body1, const Vector3 &frictionPosWorld, const Vector3 &frictionDirectionWorld, real_t maxImpulse);
	};

	void _resolve_single_bilateral(PhysicsDirectBodyState3D *s, const Vector3 &pos1, PhysicsBody3D *body2, const Vector3 &pos2, const Vector3 &normal, real_t &impulse, real_t p_rollInfluence);
	real_t _calc_rolling_friction(VehicleWheelContactPoint &contactPoint);
	void _update_vehicle_friction(PhysicsDirectBodyState3D *s);
	void _update_vehicle_suspension(PhysicsDirectBodyState3D *s);
	real_t _vehicle_ray_cast(int p_idx, PhysicsDirectBodyState3D *s);
	void _update_wheel_transform(VehicleWheel3D &wheel, PhysicsDirectBodyState3D *s);
	void _update_wheel(int p_idx, PhysicsDirectBodyState3D *s);
	void _update_vehicle_process_mode();
	void _step_vehicle(PhysicsDirectBodyState3D *p_state);

	// Physical bone helpers.
	void _start_bone_physics_simulation();
	void _stop_bone_physics_simulation();

protected:
	void _notification(int p_what);
	virtual void _physics_interpolated_changed() override;
	virtual void fti_pump_xform() override;

public:
	PhysicsBody3D(BodyMode p_mode = MODE_STATIC);
	~PhysicsBody3D();

	void set_body_mode(BodyMode p_mode);
	BodyMode get_body_mode() const { return mode; }

	virtual CollisionState3D *get_collision_state_3d() override { return &collision_state; }
	virtual const CollisionState3D *get_collision_state_3d() const override { return &collision_state; }

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
	void set_ray_pickable(bool p_ray_pickable) { collision_state.set_ray_pickable(this, p_ray_pickable); }
	bool is_ray_pickable() const { return collision_state.ray_pickable; }
	void set_capture_input_on_drag(bool p_capture) { collision_state.capture_input_on_drag = p_capture; }
	bool get_capture_input_on_drag() const { return collision_state.capture_input_on_drag; }

	// Inline shape slots (direct collision shapes without child CollisionShape3D).
	int add_inline_shape(const Ref<Shape3D> &p_shape, const Transform3D &p_xform = Transform3D(), bool p_disabled = false) {
		return collision_state.add_inline_shape(p_shape, p_xform, p_disabled);
	}
	void set_inline_shape(int p_index, const Ref<Shape3D> &p_shape) { collision_state.set_inline_shape(p_index, p_shape); }
	Ref<Shape3D> get_inline_shape(int p_index) const { return collision_state.get_inline_shape(p_index); }
	void set_inline_shape_transform(int p_index, const Transform3D &p_xform) { collision_state.set_inline_shape_transform(p_index, p_xform); }
	Transform3D get_inline_shape_transform(int p_index) const { return collision_state.get_inline_shape_transform(p_index); }
	void set_inline_shape_disabled(int p_index, bool p_disabled) { collision_state.set_inline_shape_disabled(p_index, p_disabled); }
	bool is_inline_shape_disabled(int p_index) const { return collision_state.is_inline_shape_disabled(p_index); }
	void remove_inline_shape(int p_index) { collision_state.remove_inline_shape(p_index); }
	void clear_inline_shapes() { collision_state.clear_inline_shapes(); }
	int get_inline_shape_count() const { return collision_state.get_inline_shape_count(); }

	// Common PhysicsBody3D motion, locks & exceptions.
	bool move_and_collide(const PS3DT::MotionParameters &p_parameters, PS3DT::MotionResult &r_result, bool p_test_only = false, bool p_cancel_sliding = true);
	bool test_move(const Transform3D &p_from, const Vector3 &p_motion, const Ref<KinematicCollision3D> &r_collision = Ref<KinematicCollision3D>(), real_t p_margin = 0.001f, bool p_recovery_as_collision = false, int p_max_collisions = 1);
	Vector3 get_gravity() const;

	void set_axis_lock(PS3DE::BodyAxis p_axis, bool p_lock);
	bool get_axis_lock(PS3DE::BodyAxis p_axis) const;

	void add_collision_exception_with(Node *rp_node);
	void remove_collision_exception_with(Node *rp_node);

	// Static / Animatable API.
	void set_physics_material_override(const Ref<PhysicsMaterial> &p_physics_material_override);
	Ref<PhysicsMaterial> get_physics_material_override() const { return physics_material_override; }
	void set_constant_linear_velocity(const Vector3 &p_vel) { constant_linear_velocity = p_vel; }
	Vector3 get_constant_linear_velocity() const { return constant_linear_velocity; }
	void set_constant_angular_velocity(const Vector3 &p_vel) { constant_angular_velocity = p_vel; }
	Vector3 get_constant_angular_velocity() const { return constant_angular_velocity; }
	void set_sync_to_physics(bool p_enable);
	bool is_sync_to_physics_enabled() const { return sync_to_physics; }

	// Rigid / Dynamic API.
	void set_lock_rotation_enabled(bool p_lock_rotation);
	bool is_lock_rotation_enabled() const { return lock_rotation; }
	void set_freeze_enabled(bool p_freeze);
	bool is_freeze_enabled() const { return freeze; }
	void set_freeze_mode(FreezeMode p_freeze_mode);
	FreezeMode get_freeze_mode() const { return freeze_mode; }

	void set_mass(real_t p_mass);
	real_t get_mass() const { return mass; }
	real_t get_inverse_mass() const;

	void set_inertia(const Vector3 &p_inertia) { inertia = p_inertia; }
	const Vector3 &get_inertia() const { return inertia; }
	Basis get_inverse_inertia_tensor() const { return inverse_inertia_tensor; }

	void set_center_of_mass_mode(CenterOfMassMode p_mode) { center_of_mass_mode = p_mode; }
	CenterOfMassMode get_center_of_mass_mode() const { return center_of_mass_mode; }
	void set_center_of_mass(const Vector3 &p_center_of_mass) { center_of_mass = p_center_of_mass; }
	const Vector3 &get_center_of_mass() const { return center_of_mass; }

	void set_linear_velocity(const Vector3 &p_velocity) { linear_velocity = p_velocity; }
	Vector3 get_linear_velocity() const;
	void set_axis_velocity(const Vector3 &p_axis);
	void set_angular_velocity(const Vector3 &p_velocity) { angular_velocity = p_velocity; }
	Vector3 get_angular_velocity() const;

	void set_gravity_scale(real_t p_gravity_scale) { gravity_scale = p_gravity_scale; }
	real_t get_gravity_scale() const { return gravity_scale; }

	void set_linear_damp_mode(DampMode p_mode) { linear_damp_mode = p_mode; }
	DampMode get_linear_damp_mode() const { return linear_damp_mode; }
	void set_angular_damp_mode(DampMode p_mode) { angular_damp_mode = p_mode; }
	DampMode get_angular_damp_mode() const { return angular_damp_mode; }
	void set_linear_damp(real_t p_linear_damp) { linear_damp = p_linear_damp; }
	real_t get_linear_damp() const { return linear_damp; }
	void set_angular_damp(real_t p_angular_damp) { angular_damp = p_angular_damp; }
	real_t get_angular_damp() const { return angular_damp; }

	void set_use_custom_integrator(bool p_enable);
	bool is_using_custom_integrator() const { return custom_integrator; }
	void set_sleeping(bool p_sleeping) { sleeping = p_sleeping; }
	bool is_sleeping() const { return sleeping; }
	void set_can_sleep(bool p_active) { can_sleep = p_active; }
	bool is_able_to_sleep() const { return can_sleep; }
	void set_contact_monitor(bool p_enabled) { contact_monitor = p_enabled; }
	bool is_contact_monitor_enabled() const { return contact_monitor; }
	void set_max_contacts_reported(int p_amount);
	int get_max_contacts_reported() const { return max_contacts_reported; }
	int get_contact_count() const { return contact_count; }
	void set_use_continuous_collision_detection(bool p_enable);
	bool is_using_continuous_collision_detection() const { return ccd; }

	void apply_central_impulse(const Vector3 &p_impulse);
	void apply_impulse(const Vector3 &p_impulse, const Vector3 &p_position = Vector3());
	void apply_torque_impulse(const Vector3 &p_impulse);
	void apply_central_force(const Vector3 &p_force);
	void apply_force(const Vector3 &p_force, const Vector3 &p_position = Vector3());
	void apply_torque(const Vector3 &p_torque);
	void add_constant_central_force(const Vector3 &p_force);
	void add_constant_force(const Vector3 &p_force, const Vector3 &p_position = Vector3());
	void add_constant_torque(const Vector3 &p_torque);
	void set_constant_force(const Vector3 &p_force);
	Vector3 get_constant_force() const;
	void set_constant_torque(const Vector3 &p_torque);
	Vector3 get_constant_torque() const;

	// CharacterMover3D API.
	CharacterMover3D &get_character_mover() { return character; }
	const CharacterMover3D &get_character_mover() const { return character; }
	bool move_and_slide();
	void apply_floor_snap();
	const Vector3 &get_velocity() const { return character.velocity; }
	void set_velocity(const Vector3 &p_velocity) { character.velocity = p_velocity; }
	const Vector3 &get_up_direction() const { return character.up_direction; }
	void set_up_direction(const Vector3 &p_up_direction);
	bool is_on_floor() const { return character.flags.floor; }
	bool is_on_floor_only() const { return character.flags.floor && !character.flags.wall && !character.flags.ceiling; }
	bool is_on_wall() const { return character.flags.wall; }
	bool is_on_wall_only() const { return character.flags.wall && !character.flags.floor && !character.flags.ceiling; }
	bool is_on_ceiling() const { return character.flags.ceiling; }
	bool is_on_ceiling_only() const { return character.flags.ceiling && !character.flags.floor && !character.flags.wall; }
	const Vector3 &get_last_motion() const { return character.last_motion; }
	Vector3 get_position_delta() const { return get_global_transform().origin - character.previous_position; }
	const Vector3 &get_floor_normal() const { return character.floor_normal; }
	const Vector3 &get_wall_normal() const { return character.wall_normal; }
	const Vector3 &get_real_velocity() const { return character.real_velocity; }
	real_t get_floor_angle(const Vector3 &p_up_direction = Vector3(0.0f, 1.0f, 0.0f)) const;
	const Vector3 &get_platform_velocity() const { return character.platform_velocity; }
	const Vector3 &get_platform_angular_velocity() const { return character.platform_angular_velocity; }
	int get_slide_collision_count() const { return character.motion_results.size(); }
	PS3DT::MotionResult get_slide_collision(int p_bounce) const;
	void set_safe_margin(real_t p_margin) { character.margin = p_margin; }
	real_t get_safe_margin() const { return character.margin; }
	bool is_floor_stop_on_slope_enabled() const { return character.floor_stop_on_slope; }
	void set_floor_stop_on_slope_enabled(bool p_enabled) { character.floor_stop_on_slope = p_enabled; }
	bool is_floor_constant_speed_enabled() const { return character.floor_constant_speed; }
	void set_floor_constant_speed_enabled(bool p_enabled) { character.floor_constant_speed = p_enabled; }
	bool is_floor_block_on_wall_enabled() const { return character.floor_block_on_wall; }
	void set_floor_block_on_wall_enabled(bool p_enabled) { character.floor_block_on_wall = p_enabled; }
	bool is_slide_on_ceiling_enabled() const { return character.slide_on_ceiling; }
	void set_slide_on_ceiling_enabled(bool p_enabled) { character.slide_on_ceiling = p_enabled; }
	int get_max_slides() const { return character.max_slides; }
	void set_max_slides(int p_max_slides);
	real_t get_floor_max_angle() const { return character.floor_max_angle; }
	void set_floor_max_angle(real_t p_radians) { character.floor_max_angle = p_radians; }
	real_t get_floor_snap_length() const { return character.floor_snap_length; }
	void set_floor_snap_length(real_t p_floor_snap_length);
	real_t get_wall_min_slide_angle() const { return character.wall_min_slide_angle; }
	void set_wall_min_slide_angle(real_t p_radians) { character.wall_min_slide_angle = p_radians; }
	uint32_t get_platform_floor_layers() const { return character.platform_floor_layers; }
	void set_platform_floor_layers(uint32_t p_exclude_layer) { character.platform_floor_layers = p_exclude_layer; }
	uint32_t get_platform_wall_layers() const { return character.platform_wall_layers; }
	void set_platform_wall_layers(uint32_t p_exclude_layer) { character.platform_wall_layers = p_exclude_layer; }
	void set_motion_mode(MotionMode p_mode) { character.motion_mode = p_mode; }
	MotionMode get_motion_mode() const { return character.motion_mode; }
	void set_platform_on_leave(PlatformOnLeave p_on_leave) { character.platform_on_leave = p_on_leave; }
	PlatformOnLeave get_platform_on_leave() const { return character.platform_on_leave; }

	// VehicleRig3D API.
	VehicleRig3D &get_vehicle_rig() { return vehicle; }
	const VehicleRig3D &get_vehicle_rig() const { return vehicle; }
	void set_engine_force(real_t p_engine_force);
	real_t get_engine_force() const { return vehicle.engine_force; }
	void set_brake(real_t p_brake);
	real_t get_brake() const { return vehicle.brake; }
	void set_steering(real_t p_steering);
	real_t get_steering() const { return vehicle.steering; }

	// PhysicalBoneState3D API.
	PhysicalBoneState3D &get_physical_bone_state() { return bone_state; }
	const PhysicalBoneState3D &get_physical_bone_state() const { return bone_state; }
	PhysicalBoneSimulator3D *get_simulator() const;
	int get_bone_id() const { return bone_state.bone_id; }
	RID get_joint_rid() const { return bone_state.joint; }
	void set_bone_joint_type(BoneJointType p_joint_type) { bone_state.joint_type = p_joint_type; }
	BoneJointType get_bone_joint_type() const { return bone_state.joint_type; }
	const PhysicalBoneState3D::JointParams &get_bone_joint_params() const { return bone_state.joint_params; }
	PhysicalBoneState3D::JointParams &get_bone_joint_params() { return bone_state.joint_params; }
	void set_joint_offset(const Transform3D &p_offset) { bone_state.joint_offset = p_offset; }
	const Transform3D &get_joint_offset() const { return bone_state.joint_offset; }
	Vector3 get_joint_rotation() const { return bone_state.joint_offset.basis.get_euler_normalized(); }
	void set_body_offset(const Transform3D &p_offset);
	const Transform3D &get_body_offset() const { return bone_state.body_offset; }
	void set_simulate_physics(bool p_simulate);
	bool get_simulate_physics() const { return bone_state.simulate_physics; }
	bool is_simulating_physics() const { return bone_state.internal_simulate_physics; }
	void set_bone_name(const String &p_name) { bone_state.bone_name = p_name; }
	const String &get_bone_name() const { return bone_state.bone_name; }
	void set_friction(real_t p_friction) { bone_state.friction = p_friction; }
	real_t get_friction() const { return bone_state.friction; }
	void set_bounce(real_t p_bounce) { bone_state.bounce = p_bounce; }
	real_t get_bounce() const { return bone_state.bounce; }
	void reset_physics_simulation_state();

#ifdef TOOLS_ENABLED
	void _set_gizmo_move_joint(bool p_move_joint) { bone_state.gizmo_move_joint = p_move_joint; }
	virtual Transform3D get_global_gizmo_transform() const override;
	virtual Transform3D get_local_gizmo_transform() const override;
#endif

	virtual PackedStringArray get_configuration_warnings() const override;
};

// --- End scene/3d/physics/physics_body_3d.h ---

// --- Folded from scene/3d/physics/physical_bone_simulator_3d.h ---

class PhysicsBody3D;

class PhysicalBoneSimulator3D : public SkeletonModifier3D {
	bool simulating = false;

	struct SimulatedBone {
		int parent = -1;
		Vector<int> child_bones;
		Transform3D global_pose;
		PhysicsBody3D *physical_bone = nullptr;
		PhysicsBody3D *cache_parent_physical_bone = nullptr;
	};

	LocalVector<SimulatedBone> bones;

	PhysicsBody3D *_get_physical_bone_parent(int p_bone);
	void _reset_physical_bones_state();

protected:
	virtual void _set_active(bool p_active) override;
	void _bone_pose_updated(Skeleton3D *skeleton, int p_bone_id);
	virtual void _skeleton_changed(Skeleton3D *p_old, Skeleton3D *p_new) override;

public:
#ifndef DISABLE_DEPRECATED
	bool is_compat = false;
#endif // _DISABLE_DEPRECATED
	bool is_simulating_physics() const;
	int get_bone_count() const;
	Transform3D get_bone_global_pose(int p_bone) const;
	void set_bone_global_pose(int p_bone, const Transform3D &p_pose);
	PhysicsBody3D *get_physical_bone(int p_bone);
	PhysicsBody3D *get_physical_bone_parent(int p_bone);
	void physical_bones_stop_simulation();
	void physical_bones_start_simulation_on(const TypedArray<String> &p_bones);
	void physical_bones_add_collision_exception(RID p_exception);
	void physical_bones_remove_collision_exception(RID p_exception);
	PhysicalBoneSimulator3D() = default;
};

// --- End scene/3d/physics/physical_bone_simulator_3d.h ---

// --- Folded from scene/3d/physics/area_3d.h ---
#include "core/templates/vector.h"


class Area3D : public Node3D {
public:
	enum SpaceOverride {
		SPACE_OVERRIDE_DISABLED,
		SPACE_OVERRIDE_COMBINE,
		SPACE_OVERRIDE_COMBINE_REPLACE,
		SPACE_OVERRIDE_REPLACE,
		SPACE_OVERRIDE_REPLACE_COMBINE
	};

	using DisableMode = CollisionState3D::DisableMode;

private:
	CollisionState3D collision_state;

	SpaceOverride gravity_space_override = SPACE_OVERRIDE_DISABLED;
	Vector3 gravity_vec = Vector3(0, -1, 0);
	real_t gravity = 9.8f;
	bool gravity_is_point = false;
	real_t gravity_point_unit_distance = 0.0f;

	SpaceOverride linear_damp_space_override = SPACE_OVERRIDE_DISABLED;
	SpaceOverride angular_damp_space_override = SPACE_OVERRIDE_DISABLED;
	real_t angular_damp = 0.1f;
	real_t linear_damp = 0.1f;

	int priority = 0;

	real_t wind_force_magnitude = 0.0f;
	real_t wind_attenuation_factor = 0.0f;
	NodePath wind_source_path;

	bool monitoring = false;
	bool monitorable = false;
	bool locked = false;

	bool audio_bus_override = false;
	String audio_bus;

	bool use_reverb_bus = false;
	String reverb_bus;
	float reverb_amount = 0.0f;
	float reverb_uniformity = 0.0f;

	void _clear_monitoring();
	void _initialize_wind();

protected:
	void _notification(int p_what);

public:
	Area3D();
	~Area3D();

	virtual CollisionState3D *get_collision_state_3d() override { return &collision_state; }
	virtual const CollisionState3D *get_collision_state_3d() const override { return &collision_state; }

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
	void set_ray_pickable(bool p_ray_pickable) { collision_state.set_ray_pickable(this, p_ray_pickable); }
	bool is_ray_pickable() const { return collision_state.ray_pickable; }
	void set_capture_input_on_drag(bool p_capture) { collision_state.capture_input_on_drag = p_capture; }
	bool get_capture_input_on_drag() const { return collision_state.capture_input_on_drag; }

	// Inline shape API.
	int add_inline_shape(const Ref<Shape3D> &p_shape, const Transform3D &p_xform = Transform3D(), bool p_disabled = false) {
		return collision_state.add_inline_shape(p_shape, p_xform, p_disabled);
	}
	void set_inline_shape(int p_index, const Ref<Shape3D> &p_shape) { collision_state.set_inline_shape(p_index, p_shape); }
	Ref<Shape3D> get_inline_shape(int p_index) const { return collision_state.get_inline_shape(p_index); }
	void remove_inline_shape(int p_index) { collision_state.remove_inline_shape(p_index); }
	void clear_inline_shapes() { collision_state.clear_inline_shapes(); }
	int get_inline_shape_count() const { return collision_state.get_inline_shape_count(); }

	void set_gravity_space_override_mode(SpaceOverride p_mode);
	SpaceOverride get_gravity_space_override_mode() const;

	void set_gravity_is_point(bool p_enabled);
	bool is_gravity_a_point() const;

	void set_gravity_point_unit_distance(real_t p_scale);
	real_t get_gravity_point_unit_distance() const;

	void set_gravity_point_center(const Vector3 &p_center);
	const Vector3 &get_gravity_point_center() const;

	void set_gravity_direction(const Vector3 &p_direction);
	const Vector3 &get_gravity_direction() const;

	void set_gravity(real_t p_gravity);
	real_t get_gravity() const;

	void set_linear_damp_space_override_mode(SpaceOverride p_mode);
	SpaceOverride get_linear_damp_space_override_mode() const;

	void set_angular_damp_space_override_mode(SpaceOverride p_mode);
	SpaceOverride get_angular_damp_space_override_mode() const;

	void set_angular_damp(real_t p_angular_damp);
	real_t get_angular_damp() const;

	void set_linear_damp(real_t p_linear_damp);
	real_t get_linear_damp() const;

	void set_priority(int p_priority);
	int get_priority() const;

	void set_wind_force_magnitude(real_t p_wind_force_magnitude);
	real_t get_wind_force_magnitude() const;

	void set_wind_attenuation_factor(real_t p_wind_attenuation_factor);
	real_t get_wind_attenuation_factor() const;

	void set_wind_source_path(const NodePath &p_wind_source_path);
	const NodePath &get_wind_source_path() const;

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

	void set_use_reverb_bus(bool p_enable);
	bool is_using_reverb_bus() const;

	void set_reverb_bus_name(const String &p_audio_bus);
	String get_reverb_bus_name() const;

	void set_reverb_amount(float p_amount);
	float get_reverb_amount() const;

	void set_reverb_uniformity(float p_uniformity);
	float get_reverb_uniformity() const;

	virtual PackedStringArray get_configuration_warnings() const override;
};

// --- End scene/3d/physics/area_3d.h ---

// --- Folded from scene/3d/physics/collision_shape_3d.h ---

#include "scene/resources/mesh.h"

class CollisionShape3D : public Node3D {
	Ref<Shape3D> shape;

	uint32_t owner_id = 0;
	CollisionState3D *collision_state = nullptr;

	Color debug_color;
	bool debug_fill = true;
	bool disabled = false;

	Color _get_default_debug_color() const;

#ifdef DEBUG_ENABLED
	void _shape_changed();
#endif // DEBUG_ENABLED

#ifndef DISABLE_DEPRECATED
	void resource_changed(Ref<Resource> res);
#endif

protected:
	void _update_in_shape_owner(bool p_xform_only = false);
	void _notification(int p_what);

public:
	void make_convex_from_siblings();

	void set_shape(const Ref<Shape3D> &p_shape);
	Ref<Shape3D> get_shape() const;

	void set_disabled(bool p_disabled);
	bool is_disabled() const;

	void set_debug_color(const Color &p_color);
	Color get_debug_color() const;

	void set_debug_fill_enabled(bool p_enable);
	bool get_debug_fill_enabled() const;

	PackedStringArray get_configuration_warnings() const override;

	CollisionShape3D();
	~CollisionShape3D();
};

// --- End scene/3d/physics/collision_shape_3d.h ---

// --- Folded from scene/3d/bone_attachment_3d.h ---
class BoneAttachment3D : public Node3D
{
	bool bound = false;
	String bone_name;
	int bone_idx = -1;

	bool override_pose = false;
	bool _override_dirty = false;
	bool overriding = false;

	bool use_external_skeleton = false;
	NodePath external_skeleton_node;

	void _check_bind();
	void _check_unbind();

	bool updating = false;
	void _transform_changed();
	void _update_external_skeleton_cache();

protected:
	void _notification(int p_what);

#ifndef DISABLE_DEPRECATED
	virtual void _on_bone_pose_update_bind_compat_90575(int p_bone_index);
	static void _bind_compatibility_methods();
#endif

public:
	virtual PackedStringArray get_configuration_warnings() const override;

	Skeleton3D* get_skeleton();

	void set_bone_name(const String& p_name);
	String get_bone_name() const;

	void set_bone_idx(const int& p_idx);
	int get_bone_idx() const;

	void set_override_pose(bool p_override_pose);
	bool get_override_pose() const;

	void set_use_external_skeleton(bool p_use_external_skeleton);
	bool get_use_external_skeleton() const;
	void set_external_skeleton(NodePath p_external_skeleton);
	NodePath get_external_skeleton() const;

	virtual void on_skeleton_update();

#ifdef TOOLS_ENABLED
	virtual void notify_rebind_required();
#endif

	BoneAttachment3D() = default;
};

// --- Folded from scene/3d/importer_mesh_instance_3d.h ---



class ImporterMesh;

class ImporterMeshInstance3D : public Node3D
{
	Ref<ImporterMesh> mesh;
	Ref<Skin> skin;
	NodePath skeleton_path;
	Vector<Ref<Material>> surface_materials;
	uint32_t layer_mask = 1;
	RenderInstance3D::ShadowCastingSetting shadow_casting_setting =
		RenderInstance3D::SHADOW_CASTING_SETTING_ON;
	float visibility_range_begin = 0.0;
	float visibility_range_end = 0.0;
	float visibility_range_begin_margin = 0.0;
	float visibility_range_end_margin = 0.0;
	RenderInstance3D::VisibilityRangeFadeMode visibility_range_fade_mode =
		RenderInstance3D::VISIBILITY_RANGE_FADE_DISABLED;

public:
	void set_mesh(const Ref<ImporterMesh>& p_mesh);
	Ref<ImporterMesh> get_mesh() const;

	void set_skin(const Ref<Skin>& p_skin);
	Ref<Skin> get_skin() const;

	void set_surface_material(int p_idx, const Ref<Material>& p_material);
	Ref<Material> get_surface_material(int p_idx) const;

	void set_skeleton_path(const NodePath& p_path);
	NodePath get_skeleton_path() const;

	void set_layer_mask(const uint32_t p_layer_mask);
	uint32_t get_layer_mask() const;

	void set_cast_shadows_setting(RenderInstance3D::ShadowCastingSetting p_shadow_casting_setting);
	RenderInstance3D::ShadowCastingSetting get_cast_shadows_setting() const;

	float get_visibility_range_begin() const;
	float get_visibility_range_end() const;
	float get_visibility_range_begin_margin() const;
	float get_visibility_range_end_margin() const;

	RenderInstance3D::VisibilityRangeFadeMode get_visibility_range_fade_mode() const;
};

// --- Folded from scene/3d/mesh_instance_3d.h ---
#include "core/templates/local_vector.h"


#include "scene/resources/material.h"
#include "scene/resources/mesh.h"

#ifndef NAVIGATION_3D_DISABLED
class NavigationMesh;
class NavigationMeshSourceGeometryData3D;
#endif // NAVIGATION_3D_DISABLED
class Skin;
class SkinReference;

class MeshInstance3D : public Node3D
{
public:
	RenderInstance3D render_instance;

protected:
	Ref<Mesh> mesh;
	Ref<Skin> skin;
	Ref<Skin> skin_internal;
	Ref<SkinReference> skin_ref;
	NodePath skeleton_path;

	LocalVector<float> blend_shape_tracks;
	HashMap<String, int> blend_shape_properties;
	Vector<Ref<Material>> surface_override_materials;

	void _resolve_skeleton_path();

	bool surface_index_0 = false;

	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

	bool _property_can_revert(const String& p_name) const;

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID& p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }
	void set_layer_mask(uint32_t p_mask) { render_instance.set_layer_mask(p_mask); }
	uint32_t get_layer_mask() const { return render_instance.get_layer_mask(); }
	void set_cast_shadows_setting(RenderInstance3D::ShadowCastingSetting p_setting)
	{
		render_instance.set_cast_shadows_setting(p_setting);
	}
	RenderInstance3D::ShadowCastingSetting get_cast_shadows_setting() const
	{
		return render_instance.get_cast_shadows_setting();
	}
	void set_material_override(const Ref<Material>& p_material)
	{
		render_instance.set_material_override(p_material);
	}
	Ref<Material> get_material_override() const { return render_instance.get_material_override(); }
	void set_material_overlay(const Ref<Material>& p_material)
	{
		render_instance.set_material_overlay(p_material);
	}
	Ref<Material> get_material_overlay() const { return render_instance.get_material_overlay(); }
	void set_gi_mode(RenderInstance3D::GIMode p_mode) { render_instance.set_gi_mode(p_mode); }
	RenderInstance3D::GIMode get_gi_mode() const { return render_instance.get_gi_mode(); }

	void set_mesh(const Ref<Mesh>& p_mesh);
	Ref<Mesh> get_mesh() const;

	void set_skin(const Ref<Skin>& p_skin);
	Ref<Skin> get_skin() const;

	void set_skeleton_path(const NodePath& p_skeleton);
	NodePath get_skeleton_path();

	Ref<SkinReference> get_skin_reference() const;

	int get_blend_shape_count() const;
	int find_blend_shape_by_name(const String& p_name);
	float get_blend_shape_value(int p_blend_shape) const;
	void set_blend_shape_value(int p_blend_shape, float p_value);

	int get_surface_override_material_count() const;
	void set_surface_override_material(int p_surface, const Ref<Material>& p_material);
	Ref<Material> get_surface_override_material(int p_surface) const;
	Ref<Material> get_active_material(int p_surface) const;

#ifndef PHYSICS_3D_DISABLED
	Node* create_trimesh_collision_node();
	void create_trimesh_collision();

	Node* create_convex_collision_node(bool p_clean = true, bool p_simplify = false);
	void create_convex_collision(bool p_clean = true, bool p_simplify = false);

	Node* create_multiple_convex_collisions_node(
		const Ref<MeshConvexDecompositionSettings>& p_settings =
			Ref<MeshConvexDecompositionSettings>());
	void create_multiple_convex_collisions(const Ref<MeshConvexDecompositionSettings>& p_settings =
											   Ref<MeshConvexDecompositionSettings>());
#endif // PHYSICS_3D_DISABLED

	MeshInstance3D* create_debug_tangents_node();
	void create_debug_tangents();

	AABB get_aabb() const;

	Ref<ArrayMesh> bake_mesh_from_current_blend_shape_mix(
		Ref<ArrayMesh> p_existing = Ref<ArrayMesh>());
	Ref<ArrayMesh> bake_mesh_from_current_skeleton_pose(
		Ref<ArrayMesh> p_existing = Ref<ArrayMesh>());

	Ref<TriangleMesh> generate_triangle_mesh() const;

#ifndef NAVIGATION_3D_DISABLED
private:
	static RID _navmesh_source_geometry_parser;

public:
	static void navmesh_parse_init();
	static void navmesh_parse_source_geometry(const Ref<NavigationMesh>& p_navigation_mesh,
		Ref<NavigationMeshSourceGeometryData3D> p_source_geometry_data, Node* p_node);
#endif // NAVIGATION_3D_DISABLED

	virtual PackedStringArray get_configuration_warnings() const override;

	MeshInstance3D();
};
// --- End scene/3d/mesh_instance_3d.h ---
// --- End scene/3d/skeleton_3d.h ---
// --- End scene/3d/node_3d.h ---
