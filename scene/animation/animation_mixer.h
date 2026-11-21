/**************************************************************************/
/*  animation_mixer.h                                                     */
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

#include "core/templates/a_hash_map.h"
#include "scene/animation/tween.h"
#include "scene/main/node.h"
#include "scene/resources/animation.h"
// --- Folded from scene/resources/animation_library.h ---
#include "core/templates/rb_map.h"
#include "scene/resources/animation.h"

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
#include "servers/audio/audio_stream.h"

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

#include "scene/resources/animation.h"

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

#include "scene/resources/curve.h"

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
