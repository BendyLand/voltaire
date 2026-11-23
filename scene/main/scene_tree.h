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
#include "core/templates/self_list.h"
#include "core/types.h"
// --- Folded from scene/animation/animation_mixer.h ---
#include "core/templates/a_hash_map.h"
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
#include "core/templates/rb_map.h"


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
#include "servers/audio/audio_server.h"

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
#include "core/templates/safe_refcount.h"
#include "scene/main/node.h"
#include "servers/audio/audio_server.h"

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
#include "core/io/stream_peer.h"

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
#include "core/io/stream_peer.h"
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
