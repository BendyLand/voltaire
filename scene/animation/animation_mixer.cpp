/**************************************************************************/
/*  animation_mixer.cpp                                                   */
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

#include "animation_mixer.h"
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/string/ustring.h"
#include "scene/2d/camera_2d.h"
#include "scene/animation/animation_player.h"
#include "scene/audio/audio_stream_player.h"
#include "scene/resources/animation.h"
#include "servers/audio/audio_server.h"
#include "servers/audio/audio_stream.h"

#ifndef _3D_DISABLED
#include "scene/3d/camera_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/node_3d.h"
#include "scene/3d/skeleton_3d.h"
#endif // _3D_DISABLED

void AnimationMixer::_animation_added(const String& p_name, const String& p_library)
{
	_animation_set_cache_update();
}

void AnimationMixer::_animation_removed(const String& p_name, const String& p_library)
{
	const String name =
		p_library == String() ? p_name : String(String(p_library) + "/" + String(p_name));

	if (!animation_set.has(name)) {
		return; // No need to update because not the one from the library being used.
	}

	_animation_set_cache_update();

	_remove_animation(name);
}

void AnimationMixer::_animation_changed(const String& p_name) { _clear_caches(); }

void AnimationMixer::get_animation_library_list(LocalVector<String>* p_libraries) const
{
	for (const AnimationLibraryData& lib : animation_libraries) {
		p_libraries->push_back(lib.name);
	}
}

Ref<AnimationLibrary> AnimationMixer::get_animation_library(const String& p_name) const
{
	for (const AnimationLibraryData& lib : animation_libraries) {
		if (lib.name == p_name) {
			return lib.library;
		}
	}
	ERR_FAIL_V(Ref<AnimationLibrary>());
}

bool AnimationMixer::has_animation_library(const String& p_name) const
{
	for (const AnimationLibraryData& lib : animation_libraries) {
		if (lib.name == p_name) {
			return true;
		}
	}

	return false;
}

const String& AnimationMixer::find_animation_library(const Ref<Animation>& p_animation) const
{
	for (const KeyValue<String, AnimationData>& E : animation_set) {
		if (E.value.animation == p_animation) {
			return E.value.animation_library;
		}
	}
	const static String empty = String();
	return empty;
}

LocalVector<String> AnimationMixer::get_sorted_animation_list() const
{
	LocalVector<String> animations;
	get_animation_list(&animations);
	animations.sort();
	return animations;
}

void AnimationMixer::get_animation_list(LocalVector<String>* p_animations) const
{
	p_animations->reserve(p_animations->size() + animation_set.size());
	for (const KeyValue<String, AnimationData>& E : animation_set) {
		p_animations->push_back(E.key);
	}
}

const Ref<Animation>& AnimationMixer::get_animation(const String& p_name) const
{
	const Ref<Animation>& animation = get_animation_or_null(p_name);
	ERR_FAIL_COND_V_MSG(
		animation.is_null(), animation, vformat("Animation not found: \"%s\".", p_name));
	return animation;
}

const Ref<Animation>& AnimationMixer::get_animation_or_null(const String& p_name) const
{
	const AnimationData* ad = animation_set.getptr(p_name);
	if (!ad) {
		const static Ref<Animation> empty = Ref<Animation>();
		return empty;
	}
	return ad->animation;
}

bool AnimationMixer::has_animation(const String& p_name) const
{
	return animation_set.has(p_name);
}

String AnimationMixer::find_animation(const Ref<Animation>& p_animation) const
{
	for (const KeyValue<String, AnimationData>& E : animation_set) {
		if (E.value.animation == p_animation) {
			return E.key;
		}
	}
	return String();
}

void AnimationMixer::_set_process(bool p_process, bool p_force)
{
	if (processing == p_process && !p_force) {
		return;
	}

	switch (callback_mode_process) {
	case ANIMATION_CALLBACK_MODE_PROCESS_PHYSICS:
#ifdef TOOLS_ENABLED
		set_physics_process_internal(p_process && active && !editing);
#else
		set_physics_process_internal(p_process && active);
#endif // TOOLS_ENABLED
		break;
	case ANIMATION_CALLBACK_MODE_PROCESS_IDLE:
#ifdef TOOLS_ENABLED
		set_process_internal(p_process && active && !editing);
#else
		set_process_internal(p_process && active);
#endif // TOOLS_ENABLED
		break;
	case ANIMATION_CALLBACK_MODE_PROCESS_MANUAL:
		break;
	}

	processing = p_process;
}

void AnimationMixer::set_active(bool p_active)
{
	if (active == p_active) {
		return;
	}

	active = p_active;
	_set_active(active);
	_set_process(processing, true);

	if (!active && is_inside_tree()) {
		_clear_caches();
	}
}

bool AnimationMixer::is_active() const { return active; }

void AnimationMixer::set_root_node(const NodePath& p_path)
{
	root_node = p_path;
	_clear_caches();
}

NodePath AnimationMixer::get_root_node() const { return root_node; }

void AnimationMixer::set_deterministic(bool p_deterministic)
{
	deterministic = p_deterministic;
	_clear_caches();
}

bool AnimationMixer::is_deterministic() const { return deterministic; }

void AnimationMixer::set_callback_mode_process(AnimationCallbackModeProcess p_mode)
{
	if (callback_mode_process == p_mode) {
		return;
	}

	bool was_active = is_active();
	if (was_active) {
		set_active(false);
	}

	callback_mode_process = p_mode;

	if (was_active) {
		set_active(true);
	}
}

AnimationMixer::AnimationCallbackModeProcess AnimationMixer::get_callback_mode_process() const
{
	return callback_mode_process;
}

AnimationMixer::AnimationCallbackModeMethod AnimationMixer::get_callback_mode_method() const
{
	return callback_mode_method;
}

AnimationMixer::AnimationCallbackModeDiscrete AnimationMixer::get_callback_mode_discrete() const
{
	return callback_mode_discrete;
}

void AnimationMixer::set_audio_max_polyphony(int p_audio_max_polyphony)
{
	ERR_FAIL_COND(p_audio_max_polyphony < 0 || p_audio_max_polyphony > 128);
	audio_max_polyphony = p_audio_max_polyphony;
}

int AnimationMixer::get_audio_max_polyphony() const { return audio_max_polyphony; }

#ifdef TOOLS_ENABLED

bool AnimationMixer::is_editing() const { return editing; }

void AnimationMixer::set_dummy(bool p_dummy) { dummy = p_dummy; }

bool AnimationMixer::is_dummy() const { return dummy; }
#endif // TOOLS_ENABLED

void AnimationMixer::_clear_caches(bool p_clear_track_cache) {}

void AnimationMixer::_init_root_motion_cache()
{
	root_motion_cache.loc = Vector3(0, 0, 0);
	root_motion_cache.rot = Quaternion(0, 0, 0, 1);
	root_motion_cache.scale = Vector3(1, 1, 1);
	root_motion_position = Vector3(0, 0, 0);
	root_motion_rotation = Quaternion(0, 0, 0, 1);
	root_motion_scale = Vector3(0, 0, 0);
	root_motion_position_accumulator = Vector3(0, 0, 0);
	root_motion_rotation_accumulator = Quaternion(0, 0, 0, 1);
	root_motion_scale_accumulator = Vector3(1, 1, 1);
}

void AnimationMixer::_create_track_num_to_track_cache_for_animation(
	const Ref<Animation>& p_animation)
{
	if (animation_track_num_to_track_cache.has(p_animation)) {
		// In AnimationMixer::_update_caches, it retrieves all animations via
		// AnimationMixer::get_animation_list Since multiple AnimationLibraries can share the same
		// Animation, it is possible that the cache is already created.
		return;
	}
	LocalVector<TrackCache*>& track_num_to_track_cache =
		animation_track_num_to_track_cache.insert_new(p_animation, LocalVector<TrackCache*>())
			->value;
	const LocalVector<Animation::Track*>& tracks = p_animation->get_tracks();

	track_num_to_track_cache.resize(tracks.size());
	for (uint32_t i = 0; i < tracks.size(); i++) {
		TrackCache** track_ptr = track_cache.getptr(tracks[i]->get_unique_id());
		if (track_ptr == nullptr) {
			track_num_to_track_cache[i] = nullptr;
		}
		else {
			track_num_to_track_cache[i] = *track_ptr;
		}
	}
}

bool AnimationMixer::_blend_pre_process(
	double p_delta, int p_track_count, const AHashMap<NodePath, int>& p_track_map)
{
	return true;
}

void AnimationMixer::_blend_capture(double p_delta) { blend_capture(p_delta); }

void AnimationMixer::blend_capture(double p_delta)
{
	if (capture_cache.animation.is_null()) {
		return;
	}

	capture_cache.remain -= p_delta * capture_cache.step;
	if (Animation::is_less_or_equal_approx(capture_cache.remain, 0)) {
		if (capture_cache.animation.is_valid()) {
			animation_track_num_to_track_cache.erase(capture_cache.animation);
		}
		capture_cache.clear();
		return;
	}

	real_t weight = Tween::run_equation(
		capture_cache.trans_type, capture_cache.ease_type, capture_cache.remain, 0.0, 1.0, 1.0);

	// Blend with other animations.
	real_t inv = 1.0 - weight;
	for (AnimationInstance& ai : animation_instances) {
		ai.playback_info.weight *= inv;
	}

	// Build capture animation instance.
	PlaybackInfo pi;
	pi.weight = weight;

	AnimationInstance ai;
	ai.animation = capture_cache.animation;
	ai.playback_info = pi;

	animation_instances.push_back(ai);
}

void AnimationMixer::_blend_calc_total_weight()
{
	for (const AnimationInstance& ai : animation_instances) {
		const Ref<Animation>& a = ai.animation;
		real_t weight = ai.playback_info.weight;
		if (Math::is_zero_approx(weight)) {
			continue;
		}
		Span<real_t> track_weights = ai.playback_info.track_weights != nullptr
										 ? *ai.playback_info.track_weights
										 : Span<real_t>();

		LocalVector<TrackCache*>* t_cache = animation_track_num_to_track_cache.getptr(a);
		ERR_CONTINUE_EDMSG(!t_cache, "No animation in cache.");
		LocalVector<TrackCache*>& track_num_to_track_cache = *t_cache;

		uint64_t pass_id = ++animation_instance_weight_pass_counter;
		// Handle wrap (slower but rare).
		if (unlikely(pass_id == 0)) {
			for (KeyValue<Animation::TrackCacheID, TrackCache*>& kv : track_cache) {
				if (kv.value) {
					kv.value->animation_instance_weight_applied_at = 0;
				}
			}
			animation_instance_weight_pass_counter = 1;
			pass_id = 1;
		}

		const LocalVector<Animation::Track*>& tracks = a->get_tracks();
		Animation::Track* const* tracks_ptr = tracks.ptr();
		int count = tracks.size();
		for (int i = 0; i < count; i++) {
			Animation::Track* animation_track = tracks_ptr[i];
			if (!animation_track->enabled) {
				continue;
			}
			TrackCache* track = track_num_to_track_cache[i];
			if (track == nullptr) {
				// No path, but avoid error spamming.
				continue;
			}

			// In some cases (e.g. transform TrackCache),
			// multiple Animation::Tracks (e.g. TYPE_POSITION_3D, TYPE_ROTATION_3D and
			// TYPE_SCALE_3D) can point to the same TrackCache instance. So we need to make sure
			// that the weight is added only once per AnimationInstance.
			if (track->animation_instance_weight_applied_at == pass_id) {
				continue;
			}

			int blend_idx = track->blend_idx;
			ERR_CONTINUE(blend_idx < 0 || blend_idx >= track_count);
			real_t blend;
			if (!track_weights.is_empty() && blend_idx < static_cast<int>(track_weights.size())) {
				blend = track_weights[blend_idx] * weight;
			}
			else {
				blend = weight;
			}
			track->total_weight += blend;
			track->animation_instance_weight_applied_at = pass_id;
		}
	}
}

void AnimationMixer::make_animation_instance(
	const String& p_name, const PlaybackInfo& p_playback_info)
{
	const Ref<Animation>& animation = get_animation_or_null(p_name);
	ERR_FAIL_COND(animation.is_null());

	AnimationInstance ai;
	ai.animation = animation;
	ai.playback_info = p_playback_info;

	animation_instances.push_back(std::move(ai));
}

void AnimationMixer::clear_animation_instances() { animation_instances.clear(); }

void AnimationMixer::advance(double p_time) { _process_animation(p_time); }

void AnimationMixer::clear_caches() { _clear_caches(); }

NodePath AnimationMixer::get_root_motion_track() const { return root_motion_track; }

void AnimationMixer::set_root_motion_local(bool p_enabled) { root_motion_local = p_enabled; }

bool AnimationMixer::is_root_motion_local() const { return root_motion_local; }

Vector3 AnimationMixer::get_root_motion_position() const { return root_motion_position; }

Quaternion AnimationMixer::get_root_motion_rotation() const { return root_motion_rotation; }

Vector3 AnimationMixer::get_root_motion_scale() const { return root_motion_scale; }

Vector3 AnimationMixer::get_root_motion_position_accumulator() const
{
	return root_motion_position_accumulator;
}

Quaternion AnimationMixer::get_root_motion_rotation_accumulator() const
{
	return root_motion_rotation_accumulator;
}

Vector3 AnimationMixer::get_root_motion_scale_accumulator() const
{
	return root_motion_scale_accumulator;
}

void AnimationMixer::set_reset_on_save_enabled(bool p_enabled) { reset_on_save = p_enabled; }

bool AnimationMixer::is_reset_on_save_enabled() const { return reset_on_save; }

bool AnimationMixer::can_apply_reset() const { return has_animation(SceneString(RESET)); }

void AnimationMixer::_node_removed(Node* p_node) { _clear_caches(); }

void AnimationMixer::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
		if (!processing) {
			set_physics_process_internal(false);
			set_process_internal(false);
		}
		_clear_caches();
	} break;

	case NOTIFICATION_INTERNAL_PROCESS: {
		if (active && callback_mode_process == ANIMATION_CALLBACK_MODE_PROCESS_IDLE) {
			_process_animation(get_process_delta_time());
		}
	} break;

	case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
		if (active && callback_mode_process == ANIMATION_CALLBACK_MODE_PROCESS_PHYSICS) {
			_process_animation(get_physics_process_delta_time());
		}
	} break;

	case NOTIFICATION_EXIT_TREE: {
		_clear_caches();
	} break;
	}
}

#ifdef TOOLS_ENABLED
void AnimationMixer::get_argument_options(
	const String& p_function, int p_idx, List<String>* r_options) const
{
	const String pf = p_function;
	if (p_idx == 0) {
		if (pf == "get_animation" || pf == "has_animation") {
			for (const String& name : get_sorted_animation_list()) {
				r_options->push_back(String(name).quote());
			}
		}
		else if (pf == "get_animation_library" || pf == "has_animation_library" ||
				   pf == "remove_animation_library" || pf == "rename_animation_library") {
			LocalVector<String> al;
			get_animation_library_list(&al);
			for (const String& name : al) {
				r_options->push_back(String(name).quote());
			}
		}
	}
	Node::get_argument_options(p_function, p_idx, r_options);
}
#endif

AnimationMixer::AnimationMixer() { root_node = NodePath(".."); }

AnimationMixer::~AnimationMixer() {}

void AnimatedValuesBackup::set_data(
	const AHashMap<Animation::TrackCacheID, AnimationMixer::TrackCache*, HashHasher>& p_data)
{
	clear_data();

	for (const KeyValue<Animation::TrackCacheID, AnimationMixer::TrackCache*>& E : p_data) {
		AnimationMixer::TrackCache* track = get_cache_copy(E.value);
		if (!track) {
			continue; // Some types of tracks do not get a copy and must be ignored.
		}

		data.insert(E.key, track);
	}
}

AHashMap<Animation::TrackCacheID, AnimationMixer::TrackCache*, HashHasher>
AnimatedValuesBackup::get_data() const
{
	AHashMap<Animation::TrackCacheID, AnimationMixer::TrackCache*, HashHasher> ret;
	for (const KeyValue<Animation::TrackCacheID, AnimationMixer::TrackCache*>& E : data) {
		AnimationMixer::TrackCache* track = get_cache_copy(E.value);
		ERR_CONTINUE(
			!track); // Backup shouldn't contain tracks that cannot be copied, this is a mistake.

		ret.insert(E.key, track);
	}
	return ret;
}

void AnimatedValuesBackup::clear_data()
{
	for (KeyValue<Animation::TrackCacheID, AnimationMixer::TrackCache*>& K : data) {
		memdelete(K.value);
	}
	data.clear();
}

AnimationMixer::TrackCache* AnimatedValuesBackup::get_cache_copy(
	AnimationMixer::TrackCache* p_cache) const
{
	switch (p_cache->type) {
	case Animation::TYPE_BEZIER:
	case Animation::TYPE_VALUE:
	case Animation::TYPE_POSITION_3D:
	case Animation::TYPE_ROTATION_3D:
	case Animation::TYPE_SCALE_3D:
	case Animation::TYPE_BLEND_SHAPE:
	case Animation::TYPE_AUDIO: {
		return memnew(AnimationMixer::TrackCache(*p_cache));
	}

	case Animation::TYPE_METHOD:
	case Animation::TYPE_ANIMATION: {
		// Nothing to do here.
	} break;
	}
	return nullptr;
}

void AnimationMixer::_animation_set_cache_update() {}

void AnimationMixer::_remove_animation(const String& p_name) {}

void AnimationMixer::_process_animation(double p_delta, bool p_update_only) {}

void AnimationMixer::_animation_renamed(
	const String& p_name, const String& p_to_name, const String& p_library)
{
}

void AnimationMixer::_set_active(bool p_active) {}

uint32_t AnimationMixer::_get_libraries_property_usage() const { return 0; }

void AnimationMixer::_blend_post_process() {}

// ==========================================================================
// Merged from scene/animation/animation_player.cpp
// ==========================================================================

// --- animation_player.compat.inc ---
#ifndef DISABLE_DEPRECATED

#include "animation_player.h"

void AnimationPlayer::_set_process_callback_bind_compat_80813(
	AnimationPlayer::AnimationProcessCallback p_mode)
{
	set_callback_mode_process(
		static_cast<AnimationMixer::AnimationCallbackModeProcess>(static_cast<int>(p_mode)));
}

AnimationPlayer::AnimationProcessCallback
AnimationPlayer::_get_process_callback_bind_compat_80813() const
{
	return static_cast<AnimationProcessCallback>(static_cast<int>(get_callback_mode_process()));
}

AnimationPlayer::AnimationMethodCallMode
AnimationPlayer::_get_method_call_mode_bind_compat_80813() const
{
	return static_cast<AnimationMethodCallMode>(static_cast<int>(get_callback_mode_method()));
}

void AnimationPlayer::_set_root_bind_compat_80813(const NodePath& p_root) { set_root_node(p_root); }

NodePath AnimationPlayer::_get_root_bind_compat_80813() const { return get_root_node(); }

void AnimationPlayer::_seek_bind_compat_80813(double p_time, bool p_update)
{
	seek(p_time, p_update, false);
}

String AnimationPlayer::_get_current_animation_bind_compat_110767() const
{
	return get_current_animation();
}

void AnimationPlayer::_set_current_animation_bind_compat_110767(const String& p_animation)
{
	set_current_animation(p_animation);
}

String AnimationPlayer::_get_assigned_animation_bind_compat_110767() const
{
	return get_assigned_animation();
}

void AnimationPlayer::_set_assigned_animation_bind_compat_110767(const String& p_animation)
{
	set_assigned_animation(p_animation);
}

String AnimationPlayer::_get_autoplay_bind_compat_110767() const { return get_autoplay(); }

void AnimationPlayer::_set_autoplay_bind_compat_110767(const String& p_name)
{
	set_autoplay(p_name);
}

void AnimationPlayer::_bind_compatibility_methods() {}

#endif // DISABLE_DEPRECATED
#include "animation_player.h"
#include "core/config/engine.h"
#include "core/os/os.h"
#include "scene/main/scene_tree.h"

void AnimationPlayer::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_READY: {
		if (!Engine::is_editor_hint() && animation_set.has(autoplay)) {
			set_active(active);
			play(autoplay);
			_check_immediately_after_start();
		}
	} break;
	}
}

void AnimationPlayer::_process_playback_data(PlaybackData& cd, double p_delta, float p_blend,
	bool p_seeked, bool p_internal_seeked, bool p_started, bool p_is_current)
{
	double speed = speed_scale * cd.speed_scale;
	bool backwards = std::signbit(speed); // Negative zero means playing backwards too.
	double delta = p_started ? 0 : p_delta * speed;
	double next_pos = cd.pos + delta;

	double start = cd.get_start_time();
	double end = cd.get_end_time();
	AnimationData* p_from = &animation_set[cd.animation_name];

	Animation::LoopedFlag looped_flag = Animation::LOOPED_FLAG_NONE;

	switch (p_from->animation->get_loop_mode()) {
	case Animation::LOOP_NONE: {
		if (Animation::is_less_approx(next_pos, start)) {
			next_pos = start;
		}
		else if (Animation::is_greater_approx(next_pos, end)) {
			next_pos = end;
		}
		delta = next_pos - cd.pos; // Fix delta (after determination of backwards because negative
								   // zero is lost here).
	} break;

	case Animation::LOOP_LINEAR: {
		if (Animation::is_less_approx(next_pos, start) &&
			Animation::is_greater_or_equal_approx(cd.pos, start)) {
			looped_flag = Animation::LOOPED_FLAG_START;
		}
		if (Animation::is_greater_approx(next_pos, end) &&
			Animation::is_less_or_equal_approx(cd.pos, end)) {
			looped_flag = Animation::LOOPED_FLAG_END;
		}
		next_pos = Math::fposmod(next_pos - start, end - start) + start;
	} break;

	case Animation::LOOP_PINGPONG: {
		if (Animation::is_less_approx(next_pos, start) &&
			Animation::is_greater_or_equal_approx(cd.pos, start)) {
			cd.speed_scale *= -1.0;
			looped_flag = Animation::LOOPED_FLAG_START;
		}
		if (Animation::is_greater_approx(next_pos, end) &&
			Animation::is_less_or_equal_approx(cd.pos, end)) {
			cd.speed_scale *= -1.0;
			looped_flag = Animation::LOOPED_FLAG_END;
		}
		next_pos = Math::pingpong(next_pos - start, end - start) + start;
	} break;

	default:
		break;
	}

	double prev_pos = cd.pos; // The animation may be changed during process, so it is safer that
							  // the state is changed before process.

	// End detection.
	if (p_is_current) {
		if (p_from->animation->get_loop_mode() == Animation::LOOP_NONE) {
			if (!backwards && Animation::is_less_or_equal_approx(prev_pos, end) &&
				Math::is_equal_approx(next_pos, end)) {
				// Playback finished.
				next_pos = end; // Snap to the edge.
				end_reached = true;
				end_notify = Animation::is_less_approx(
					prev_pos, end); // Notify only if not already at the end.
				p_blend = 1.0;
			}
			if (backwards && Animation::is_greater_or_equal_approx(prev_pos, start) &&
				Math::is_equal_approx(next_pos, start)) {
				// Playback finished.
				next_pos = start; // Snap to the edge.
				end_reached = true;
				end_notify = Animation::is_greater_approx(
					prev_pos, start); // Notify only if not already at the beginning.
				p_blend = 1.0;
			}
		}
	}

	cd.pos = next_pos;

	PlaybackInfo pi;
	if (p_started) {
		pi.time = prev_pos;
		pi.delta = 0;
		pi.start = start;
		pi.end = end;
		pi.seeked = true;
	}
	else {
		pi.time = next_pos;
		pi.delta = delta;
		pi.start = start;
		pi.end = end;
		pi.seeked = p_seeked;
	}
	if (Math::is_zero_approx(pi.delta) && backwards) {
		pi.delta = -0.0; // Sign is needed to handle converted Continuous track from Discrete track
						 // correctly.
	}
	// Immediately after playback, discrete keys should be retrieved with EXACT mode since behind
	// keys must be ignored at that time.
	pi.is_external_seeking = !p_internal_seeked && !p_started;
	pi.looped_flag = looped_flag;
	pi.weight = p_blend;
	make_animation_instance(cd.animation_name, pi);
}

float AnimationPlayer::get_current_blend_amount()
{
	Playback& c = playback;
	float blend = 1.0;
	for (const Blend& b : c.blend) {
		blend = blend - b.blend_left;
	}
	return MAX(0, blend);
}

void AnimationPlayer::_blend_playback_data(double p_delta, bool p_started)
{
	Playback& c = playback;

	bool seeked = c.seeked; // The animation may be changed during process, so it is safer that the
							// state is changed before process.
	bool internal_seeked = c.internal_seeked;

	if (!Math::is_zero_approx(p_delta)) {
		c.seeked = false;
		c.internal_seeked = false;
	}

	// Second, process current animation to check if the animation end reached.
	_process_playback_data(
		c.current, p_delta, get_current_blend_amount(), seeked, internal_seeked, p_started, true);

	// Finally, if not end the animation, do blending.
	if (end_reached) {
		playback.blend.clear();
		if (end_notify) {
			finished_anim = playback.assigned;
		}
		return;
	}
	LocalVector<int> to_erase;
	for (uint32_t i = 0; i < c.blend.size(); i++) {
		Blend& b = c.blend[i];
		b.blend_left = MAX(0, b.blend_left - Math::abs(speed_scale * p_delta) / b.blend_time);
		if (Animation::is_less_or_equal_approx(b.blend_left, 0)) {
			to_erase.push_back(i);
			b.blend_left = CMP_EPSILON; // May want to play last frame.
		}
		// Note: There may be issues if an animation event triggers an animation change while this
		// blend is active, so it is best to use "deferred" calls instead of "immediate" for
		// animation events that can trigger new animations.
		_process_playback_data(b.data, p_delta, b.blend_left, false, false, false);
	}
	for (int i = to_erase.size() - 1; i >= 0; i--) {
		c.blend.remove_at(to_erase[i]);
	}
}

void AnimationPlayer::_blend_capture(double p_delta)
{
	blend_capture(p_delta * Math::abs(speed_scale));
}

void AnimationPlayer::queue(const String& p_name)
{
	if (!is_playing()) {
		play(p_name);
	}
	else {
		playback_queue.push_back(p_name);
	}
}

void AnimationPlayer::clear_queue() { playback_queue.clear(); }

void AnimationPlayer::play_backwards(const String& p_name, double p_custom_blend)
{
	play(p_name, p_custom_blend, -1, true);
}

void AnimationPlayer::play_section_with_markers_backwards(const String& p_name,
	const String& p_start_marker, const String& p_end_marker, double p_custom_blend)
{
	play_section_with_markers(p_name, p_start_marker, p_end_marker, p_custom_blend, -1, true);
}

void AnimationPlayer::play_section_backwards(
	const String& p_name, double p_start_time, double p_end_time, double p_custom_blend)
{
	play_section(p_name, p_start_time, p_end_time, p_custom_blend, -1, true);
}

void AnimationPlayer::play(
	const String& p_name, double p_custom_blend, float p_custom_scale, bool p_from_end)
{
	if (auto_capture) {
		play_with_capture(p_name, auto_capture_duration, p_custom_blend, p_custom_scale, p_from_end,
			auto_capture_transition_type, auto_capture_ease_type);
	}
	else {
		_play(p_name, p_custom_blend, p_custom_scale, p_from_end);
	}
}

void AnimationPlayer::_play(
	const String& p_name, double p_custom_blend, float p_custom_scale, bool p_from_end)
{
	play_section_with_markers(
		p_name, String(), String(), p_custom_blend, p_custom_scale, p_from_end);
}

void AnimationPlayer::play_section_with_markers(const String& p_name,
	const String& p_start_marker, const String& p_end_marker, double p_custom_blend,
	float p_custom_scale, bool p_from_end)
{
	String name = p_name;

	if (name == String()) {
		name = playback.assigned;
	}

	AnimationData* ad = animation_set.getptr(name);
	ERR_FAIL_NULL_MSG(ad, vformat("Animation not found: %s.", name));

	const Ref<Animation>& animation = ad->animation;

	ERR_FAIL_COND_MSG(p_start_marker == p_end_marker && !p_start_marker.is_empty(),
		vformat("Start marker and end marker cannot be the same marker: %s.", p_start_marker));
	ERR_FAIL_COND_MSG(!p_start_marker.is_empty() && !animation->has_marker(p_start_marker),
		vformat("Marker %s not found in animation: %s.", p_start_marker, name));
	ERR_FAIL_COND_MSG(!p_end_marker.is_empty() && !animation->has_marker(p_end_marker),
		vformat("Marker %s not found in animation: %s.", p_end_marker, name));

	double start_time = !p_start_marker.is_empty() ? animation->get_marker_time(p_start_marker) : -1;
	double end_time = !p_end_marker.is_empty() ? animation->get_marker_time(p_end_marker) : -1;

	ERR_FAIL_COND_MSG(
		!p_start_marker.is_empty() && !p_end_marker.is_empty() && Animation::is_greater_approx(start_time, end_time),
		vformat("End marker %s is placed earlier than start marker %s in animation: %s.",
			p_end_marker, p_start_marker, name));

	if (!p_start_marker.is_empty() && Animation::is_less_approx(start_time, 0)) {
		WARN_PRINT_ED(vformat("Negative time start marker: %s is invalid in the section, so the "
							  "start of the animation: %s is used instead.",
			p_start_marker, playback.current.animation_name));
	}
	if (!p_end_marker.is_empty() && Animation::is_less_approx(end_time, 0)) {
		WARN_PRINT_ED(vformat("Negative time end marker: %s is invalid in the section, so the end "
							  "of the animation: %s is used instead.",
			p_end_marker, playback.current.animation_name));
	}

	play_section(name, start_time, end_time, p_custom_blend, p_custom_scale, p_from_end);
}

void AnimationPlayer::play_with_capture(const String& p_name, double p_duration,
	double p_custom_blend, float p_custom_scale, bool p_from_end,
	Tween::TransitionType p_trans_type, Tween::EaseType p_ease_type)
{
	_capture(p_name, p_from_end, p_duration, p_trans_type, p_ease_type);
	_play(p_name, p_custom_blend, p_custom_scale, p_from_end);
}

bool AnimationPlayer::is_playing() const { return playing; }

String AnimationPlayer::get_current_animation() const
{
	return (is_playing() ? playback.assigned : String());
}

String AnimationPlayer::get_assigned_animation() const { return playback.assigned; }

void AnimationPlayer::pause() { _stop_internal(false, false); }

void AnimationPlayer::stop(bool p_keep_state) { _stop_internal(true, p_keep_state); }

void AnimationPlayer::set_speed_scale(float p_speed) { speed_scale = p_speed; }

float AnimationPlayer::get_speed_scale() const { return speed_scale; }

float AnimationPlayer::get_playing_speed() const
{
	if (!playing) {
		return 0;
	}
	return speed_scale * playback.current.speed_scale;
}

void AnimationPlayer::seek_internal(
	double p_time, bool p_update, bool p_update_only, bool p_is_internal_seek)
{
	if (!active) {
		return;
	}

	bool is_backward = Animation::is_less_approx(p_time, playback.current.pos);

	_check_immediately_after_start();

	playback.current.pos = p_time;
	if (!playback.current.is_enabled) {
		if (!playback.assigned.is_empty()) {
			AnimationData* ad = animation_set.getptr(playback.assigned);
			ERR_FAIL_NULL_MSG(ad, vformat("Animation not found: %s.", playback.assigned));
			playback.current.is_enabled = true;
			playback.current.animation_name = playback.assigned;
			playback.current.animation_length = ad->animation->get_length();
		}
		if (!playback.current.is_enabled) {
			return; // There is no animation.
		}
	}

	double start = playback.current.get_start_time();
	double end = playback.current.get_end_time();

	// Clamp the seek position.
	p_time = CLAMP(p_time, start, end);

	playback.seeked = true;
	playback.internal_seeked = p_is_internal_seek;

	if (p_update) {
		_process_animation(is_backward ? -0.0 : 0.0, p_update_only);
		playback.seeked =
			false; // If animation was proceeded here, no more seek in internal process.
	}
}

void AnimationPlayer::seek(double p_time, bool p_update, bool p_update_only)
{
	seek_internal(p_time, p_update, p_update_only);
}

void AnimationPlayer::advance(double p_time)
{
	_check_immediately_after_start();
	AnimationMixer::advance(p_time);
}

void AnimationPlayer::_check_immediately_after_start()
{
	if (playback.started) {
		_process_animation(
			0); // Force process current key for Discrete/Method/Audio/AnimationPlayback. Then,
				// started flag is cleared.
	}
}

bool AnimationPlayer::is_valid() const { return playback.current.is_enabled; }

double AnimationPlayer::get_current_animation_position() const
{
	ERR_FAIL_COND_V_MSG(
		!playback.current.is_enabled, 0, "AnimationPlayer has no current animation.");
	return playback.current.pos;
}

double AnimationPlayer::get_current_animation_length() const
{
	ERR_FAIL_COND_V_MSG(
		!playback.current.is_enabled, 0, "AnimationPlayer has no current animation.");
	return playback.current.animation_length;
}

void AnimationPlayer::set_section_with_markers(
	const String& p_start_marker, const String& p_end_marker)
{
	ERR_FAIL_COND_MSG(!playback.current.is_enabled, "AnimationPlayer has no current animation.");
	ERR_FAIL_COND_MSG(p_start_marker == p_end_marker && !p_start_marker.is_empty(),
		vformat("Start marker and end marker cannot be the same marker: %s.", p_start_marker));
	ERR_FAIL_COND_MSG(
		!p_start_marker.is_empty() &&
			!animation_set[playback.current.animation_name].animation->has_marker(p_start_marker),
		vformat("Marker %s not found in animation: %s.", p_start_marker,
			playback.current.animation_name));
	ERR_FAIL_COND_MSG(
		!p_end_marker.is_empty() &&
			!animation_set[playback.current.animation_name].animation->has_marker(p_end_marker),
		vformat("Marker %s not found in animation: %s.", p_end_marker,
			playback.current.animation_name));
	double start_time =
		!p_start_marker.is_empty() ? animation_set[playback.current.animation_name].animation->get_marker_time(
							 p_start_marker)
					   : -1;
	double end_time =
		!p_end_marker.is_empty() ? animation_set[playback.current.animation_name].animation->get_marker_time(
						   p_end_marker)
					 : -1;
	if (!p_start_marker.is_empty() && Animation::is_less_approx(start_time, 0)) {
		WARN_PRINT_ONCE_ED(vformat("Marker %s time must be positive in animation: %s.",
			p_start_marker, playback.current.animation_name));
	}
	if (!p_end_marker.is_empty() && Animation::is_less_approx(end_time, 0)) {
		WARN_PRINT_ONCE_ED(vformat("Marker %s time must be positive in animation: %s.",
			p_end_marker, playback.current.animation_name));
	}
	set_section(start_time, end_time);
}

void AnimationPlayer::set_section(double p_start_time, double p_end_time)
{
	ERR_FAIL_COND_MSG(!playback.current.is_enabled, "AnimationPlayer has no current animation.");
	ERR_FAIL_COND_MSG(Animation::is_greater_or_equal_approx(p_start_time, 0) &&
						  Animation::is_greater_or_equal_approx(p_end_time, 0) &&
						  Animation::is_greater_or_equal_approx(p_start_time, p_end_time),
		vformat("Start time %f is greater than end time %f.", p_start_time, p_end_time));
	playback.current.start_time = p_start_time;
	playback.current.end_time = p_end_time;
	playback.current.pos = CLAMP(
		playback.current.pos, playback.current.get_start_time(), playback.current.get_end_time());
}

void AnimationPlayer::reset_section()
{
	playback.current.start_time = -1;
	playback.current.end_time = -1;
}

double AnimationPlayer::get_section_start_time() const
{
	ERR_FAIL_COND_V_MSG(!playback.current.is_enabled, playback.current.start_time,
		"AnimationPlayer has no current animation.");
	return playback.current.get_start_time();
}

double AnimationPlayer::get_section_end_time() const
{
	ERR_FAIL_COND_V_MSG(!playback.current.is_enabled, playback.current.end_time,
		"AnimationPlayer has no current animation.");
	return playback.current.get_end_time();
}

bool AnimationPlayer::has_section() const
{
	return Animation::is_greater_or_equal_approx(playback.current.start_time, 0) ||
		   Animation::is_greater_or_equal_approx(playback.current.end_time, 0);
}

void AnimationPlayer::set_autoplay(const String& p_name)
{
	if (is_inside_tree() && !Engine::is_editor_hint()) {
		WARN_PRINT("Setting autoplay after the node has been added to the scene has no effect.");
	}

	autoplay = p_name;
}

String AnimationPlayer::get_autoplay() const { return autoplay; }

void AnimationPlayer::set_movie_quit_on_finish_enabled(bool p_enabled)
{
	movie_quit_on_finish = p_enabled;
}

bool AnimationPlayer::is_movie_quit_on_finish_enabled() const { return movie_quit_on_finish; }

void AnimationPlayer::set_clear_cache_on_stop_enabled(bool p_enabled)
{
	clear_cache_on_stop = p_enabled;
}

bool AnimationPlayer::is_clear_cache_on_stop_enabled() const { return clear_cache_on_stop; }

void AnimationPlayer::_animation_changed(const String& p_name)
{
	AnimationMixer::_animation_changed(p_name);
	if (playback.current.is_enabled && playback.current.animation_name == p_name &&
		animation_set.has(p_name)) {
		playback.current.animation_length = animation_set[p_name].animation->get_length();
	}
}

void AnimationPlayer::animation_set_next(const String& p_animation, const String& p_next)
{
	ERR_FAIL_COND_MSG(
		!animation_set.has(p_animation), vformat("Animation not found: %s.", p_animation));
	animation_next_set[p_animation] = p_next;
}

String AnimationPlayer::animation_get_next(const String& p_animation) const
{
	const String* next = animation_next_set.getptr(p_animation);
	if (!next) {
		return String();
	}
	return *next;
}

void AnimationPlayer::set_default_blend_time(double p_default) { default_blend_time = p_default; }

double AnimationPlayer::get_default_blend_time() const { return default_blend_time; }

void AnimationPlayer::set_blend_time(
	const String& p_animation1, const String& p_animation2, double p_time)
{
	ERR_FAIL_COND_MSG(
		!animation_set.has(p_animation1), vformat("Animation not found: %s.", p_animation1));
	ERR_FAIL_COND_MSG(
		!animation_set.has(p_animation2), vformat("Animation not found: %s.", p_animation2));
	ERR_FAIL_COND_MSG(p_time < 0, "Blend time cannot be smaller than 0.");

	BlendKey bk;
	bk.from = p_animation1;
	bk.to = p_animation2;
	if (Math::is_zero_approx(p_time)) {
		blend_times.erase(bk);
	}
	else {
		blend_times[bk] = p_time;
	}
}

double AnimationPlayer::get_blend_time(
	const String& p_animation1, const String& p_animation2) const
{
	BlendKey bk;
	bk.from = p_animation1;
	bk.to = p_animation2;

	if (const double* blend_time = blend_times.getptr(bk)) {
		return *blend_time;
	}
	else {
		return 0;
	}
}

bool AnimationPlayer::is_auto_capture() const { return auto_capture; }

void AnimationPlayer::set_auto_capture_duration(double p_auto_capture_duration)
{
	auto_capture_duration = p_auto_capture_duration;
}

double AnimationPlayer::get_auto_capture_duration() const { return auto_capture_duration; }

void AnimationPlayer::set_auto_capture_transition_type(
	Tween::TransitionType p_auto_capture_transition_type)
{
	auto_capture_transition_type = p_auto_capture_transition_type;
}

Tween::TransitionType AnimationPlayer::get_auto_capture_transition_type() const
{
	return auto_capture_transition_type;
}

void AnimationPlayer::set_auto_capture_ease_type(Tween::EaseType p_auto_capture_ease_type)
{
	auto_capture_ease_type = p_auto_capture_ease_type;
}

Tween::EaseType AnimationPlayer::get_auto_capture_ease_type() const
{
	return auto_capture_ease_type;
}

#ifdef TOOLS_ENABLED
void AnimationPlayer::get_argument_options(
	const String& p_function, int p_idx, List<String>* r_options) const
{
	const String pf = p_function;
	if (p_idx == 0 &&
		(pf == "play" || pf == "play_backwards" || pf == "has_animation" || pf == "queue")) {
		for (const String& name : get_sorted_animation_list()) {
			r_options->push_back(String(name).quote());
		}
	}
	AnimationMixer::get_argument_options(p_function, p_idx, r_options);
}
#endif

void AnimationPlayer::_animation_removed(const String& p_name, const String& p_library)
{
	AnimationMixer::_animation_removed(p_name, p_library);

	const String& name =
		p_library == String() ? p_name : String(String(p_library) + "/" + String(p_name));

	if (!animation_set.has(name)) {
		return; // No need to update because not the one from the library being used.
	}

	_animation_set_cache_update();

	// Erase blends if needed
	LocalVector<BlendKey> to_erase;
	for (const KeyValue<BlendKey, double>& E : blend_times) {
		const BlendKey& bk = E.key;
		if (bk.from == name || bk.to == name) {
			to_erase.push_back(bk);
		}
	}

	for (const BlendKey& bk : to_erase) {
		blend_times.erase(bk);
	}
}

void AnimationPlayer::_rename_animation(const String& p_from_name, const String& p_to_name)
{
	// Rename autoplay or blends if needed.
	LocalVector<BlendKey> to_erase;
	HashMap<BlendKey, double, BlendKey> to_insert;
	for (const KeyValue<BlendKey, double>& E : blend_times) {
		BlendKey bk = E.key;
		BlendKey new_bk = bk;
		bool erase = false;
		if (bk.from == p_from_name) {
			new_bk.from = p_to_name;
			erase = true;
		}
		if (bk.to == p_from_name) {
			new_bk.to = p_to_name;
			erase = true;
		}

		if (erase) {
			to_erase.push_back(bk);
			to_insert[new_bk] = E.value;
		}
	}

	for (const BlendKey& bk : to_erase) {
		blend_times.erase(bk);
	}

	while (to_insert.size()) {
		blend_times[to_insert.begin()->key] = to_insert.begin()->value;
		to_insert.remove(to_insert.begin());
	}

	if (autoplay == p_from_name) {
		autoplay = p_to_name;
	}
}

AnimationPlayer::AnimationPlayer() {}

AnimationPlayer::~AnimationPlayer() {}

void AnimationPlayer::_capture(
	String const&, bool, double, Tween::TransitionType, Tween::EaseType)
{
}

void AnimationPlayer::_stop_internal(bool, bool) {}

void AnimationPlayer::play_section(String const&, double, double, double, float, bool) {}

void AnimationPlayer::set_current_animation(String const&) {}

void AnimationPlayer::set_assigned_animation(String const&) {}

bool AnimationPlayer::_blend_pre_process(
		double p_delta, int p_track_count, const AHashMap<NodePath, int>& p_track_map)
{
	return false;
}

void AnimationPlayer::_blend_post_process() {}

// ==========================================================================
// Merged from scene/animation/root_motion_view.cpp
// ==========================================================================

#ifndef _3D_DISABLED

#include "core/config/engine.h"
// --- Inlined from scene/animation/root_motion_view.h ---
#include "scene/3d/visual_instance_3d.h"
#include "scene/resources/mesh.h"

class RootMotionView : public Node3D {
public:
	RenderInstance3D render_instance;

	Ref<ImmediateMesh> immediate;
	NodePath path;
	real_t cell_size = 1.0;
	real_t radius = 10.0;
	bool use_in_game = false;
	Color color = Color(0.5, 0.5, 1.0);
	bool first = true;
	bool zero_y = true;

	Ref<Material> immediate_material;

	Transform3D accumulated;

protected:
	void _notification(int p_what);
	virtual void fti_update_servers_xform() override;

public:
	virtual bool has_render_instance() const override { return true; }
	RID get_instance() const { return render_instance.get_instance(); }
	void set_base(const RID &p_base) { render_instance.set_base(p_base); }
	RID get_base() const { return render_instance.get_base(); }

	void set_animation_mixer(const NodePath &p_path);
	NodePath get_animation_mixer() const;

	void set_color(const Color &p_color);
	Color get_color() const;

	void set_cell_size(float p_size);
	float get_cell_size() const;

	void set_radius(float p_radius);
	float get_radius() const;

	void set_zero_y(bool p_zero_y);
	bool get_zero_y() const;

	AABB get_aabb() const;

	RootMotionView();
	~RootMotionView();
};

// --- End scene/animation/root_motion_view.h ---
#include "scene/animation/animation_mixer.h"
#include "scene/resources/material.h"

void RootMotionView::set_animation_mixer(const NodePath& p_path)
{
	path = p_path;
	first = true;
}

NodePath RootMotionView::get_animation_mixer() const { return path; }

void RootMotionView::set_color(const Color& p_color)
{
	color = p_color;
	first = true;
}

Color RootMotionView::get_color() const { return color; }

void RootMotionView::set_cell_size(float p_size)
{
	cell_size = p_size;
	first = true;
}

float RootMotionView::get_cell_size() const { return cell_size; }

void RootMotionView::set_radius(float p_radius)
{
	radius = p_radius;
	first = true;
}

float RootMotionView::get_radius() const { return radius; }

void RootMotionView::set_zero_y(bool p_zero_y) { zero_y = p_zero_y; }

bool RootMotionView::get_zero_y() const { return zero_y; }

AABB RootMotionView::get_aabb() const
{
	return AABB(Vector3(-radius, 0, -radius), Vector3(radius * 2, 0.001, radius * 2));
}


void RootMotionView::_notification(int p_what)
{
	render_instance.notification(this, p_what);
}

void RootMotionView::fti_update_servers_xform()
{
	render_instance.fti_update_servers_xform(this);
}

RootMotionView::RootMotionView()
{
	if (Engine::is_editor_hint()) {
		set_process_internal(true);
	}
	immediate.instantiate();
	set_base(immediate->get_rid());
}

RootMotionView::~RootMotionView() { set_base(RID()); }

#endif // _3D_DISABLED

// ============================================================================
// Merged from scene/animation/tween.cpp
// ============================================================================

#include "tween.h"
// --- Inlined from scene/animation/easing_equations.h ---
#include "core/math/math_funcs.h"

/*
 * Derived from Robert Penner's easing equations: http://robertpenner.com/easing/
 *
 * Copyright (c) 2001 Robert Penner
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

namespace Linear {
inline real_t in(real_t t, real_t b, real_t c, real_t d) {
	return c * t / d + b;
}
}; // namespace Linear

namespace Sine {
inline real_t in(real_t t, real_t b, real_t c, real_t d) {
	return -c * Math::cos(t / d * (Math::PI / 2)) + c + b;
}

inline real_t out(real_t t, real_t b, real_t c, real_t d) {
	return c * Math::sin(t / d * (Math::PI / 2)) + b;
}

inline real_t in_out(real_t t, real_t b, real_t c, real_t d) {
	return -c / 2 * (Math::cos(Math::PI * t / d) - 1) + b;
}

inline real_t out_in(real_t t, real_t b, real_t c, real_t d) {
	if (t < d / 2) {
		return out(t * 2, b, c / 2, d);
	}
	real_t h = c / 2;
	return in(t * 2 - d, b + h, h, d);
}
}; // namespace Sine

namespace Quint {
inline real_t in(real_t t, real_t b, real_t c, real_t d) {
	return c * Math::pow(t / d, 5) + b;
}

inline real_t out(real_t t, real_t b, real_t c, real_t d) {
	return c * (Math::pow(t / d - 1, 5) + 1) + b;
}

inline real_t in_out(real_t t, real_t b, real_t c, real_t d) {
	t = t / d * 2;

	if (t < 1) {
		return c / 2 * Math::pow(t, 5) + b;
	}
	return c / 2 * (Math::pow(t - 2, 5) + 2) + b;
}

inline real_t out_in(real_t t, real_t b, real_t c, real_t d) {
	if (t < d / 2) {
		return out(t * 2, b, c / 2, d);
	}
	real_t h = c / 2;
	return in(t * 2 - d, b + h, h, d);
}
}; // namespace Quint

namespace Quart {
inline real_t in(real_t t, real_t b, real_t c, real_t d) {
	return c * Math::pow(t / d, 4) + b;
}

inline real_t out(real_t t, real_t b, real_t c, real_t d) {
	return -c * (Math::pow(t / d - 1, 4) - 1) + b;
}

inline real_t in_out(real_t t, real_t b, real_t c, real_t d) {
	t = t / d * 2;

	if (t < 1) {
		return c / 2 * Math::pow(t, 4) + b;
	}
	return -c / 2 * (Math::pow(t - 2, 4) - 2) + b;
}

inline real_t out_in(real_t t, real_t b, real_t c, real_t d) {
	if (t < d / 2) {
		return out(t * 2, b, c / 2, d);
	}
	real_t h = c / 2;
	return in(t * 2 - d, b + h, h, d);
}
}; // namespace Quart

namespace Quad {
inline real_t in(real_t t, real_t b, real_t c, real_t d) {
	return c * Math::pow(t / d, 2) + b;
}

inline real_t out(real_t t, real_t b, real_t c, real_t d) {
	t /= d;
	return -c * t * (t - 2) + b;
}

inline real_t in_out(real_t t, real_t b, real_t c, real_t d) {
	t = t / d * 2;

	if (t < 1) {
		return c / 2 * Math::pow(t, 2) + b;
	}
	return -c / 2 * ((t - 1) * (t - 3) - 1) + b;
}

inline real_t out_in(real_t t, real_t b, real_t c, real_t d) {
	if (t < d / 2) {
		return out(t * 2, b, c / 2, d);
	}
	real_t h = c / 2;
	return in(t * 2 - d, b + h, h, d);
}
}; // namespace Quad

namespace Expo {
inline real_t in(real_t t, real_t b, real_t c, real_t d) {
	if (t == 0) {
		return b;
	}
	return c * Math::pow(2, 10 * (t / d - 1)) + b - c * 0.001;
}

inline real_t out(real_t t, real_t b, real_t c, real_t d) {
	if (t == d) {
		return b + c;
	}
	return c * 1.001 * (-Math::pow(2, -10 * t / d) + 1) + b;
}

inline real_t in_out(real_t t, real_t b, real_t c, real_t d) {
	if (t == 0) {
		return b;
	}

	if (t == d) {
		return b + c;
	}

	t = t / d * 2;

	if (t < 1) {
		return c / 2 * Math::pow(2, 10 * (t - 1)) + b - c * 0.0005;
	}
	return c / 2 * 1.0005 * (-Math::pow(2, -10 * (t - 1)) + 2) + b;
}

inline real_t out_in(real_t t, real_t b, real_t c, real_t d) {
	if (t < d / 2) {
		return out(t * 2, b, c / 2, d);
	}
	real_t h = c / 2;
	return in(t * 2 - d, b + h, h, d);
}
}; // namespace Expo

namespace Elastic {
inline real_t in(real_t t, real_t b, real_t c, real_t d) {
	if (t == 0) {
		return b;
	}

	t /= d;
	if (t == 1) {
		return b + c;
	}

	t -= 1;
	float p = d * 0.3f;
	float a = c * Math::pow(2, 10 * t);
	float s = p / 4;

	return -(a * Math::sin((t * d - s) * (2 * Math::PI) / p)) + b;
}

inline real_t out(real_t t, real_t b, real_t c, real_t d) {
	if (t == 0) {
		return b;
	}

	t /= d;
	if (t == 1) {
		return b + c;
	}

	float p = d * 0.3f;
	float s = p / 4;

	return (c * Math::pow(2, -10 * t) * Math::sin((t * d - s) * (2 * Math::PI) / p) + c + b);
}

inline real_t in_out(real_t t, real_t b, real_t c, real_t d) {
	if (t == 0) {
		return b;
	}

	if ((t /= d / 2) == 2) {
		return b + c;
	}

	float p = d * (0.3f * 1.5f);
	float a = c;
	float s = p / 4;

	if (t < 1) {
		t -= 1;
		a *= Math::pow(2, 10 * t);
		return -0.5f * (a * Math::sin((t * d - s) * (2 * Math::PI) / p)) + b;
	}

	t -= 1;
	a *= Math::pow(2, -10 * t);
	return a * Math::sin((t * d - s) * (2 * Math::PI) / p) * 0.5f + c + b;
}

inline real_t out_in(real_t t, real_t b, real_t c, real_t d) {
	if (t < d / 2) {
		return out(t * 2, b, c / 2, d);
	}
	real_t h = c / 2;
	return in(t * 2 - d, b + h, h, d);
}
}; // namespace Elastic

namespace Cubic {
inline real_t in(real_t t, real_t b, real_t c, real_t d) {
	t /= d;
	return c * t * t * t + b;
}

inline real_t out(real_t t, real_t b, real_t c, real_t d) {
	t = t / d - 1;
	return c * (t * t * t + 1) + b;
}

inline real_t in_out(real_t t, real_t b, real_t c, real_t d) {
	t /= d / 2;
	if (t < 1) {
		return c / 2 * t * t * t + b;
	}

	t -= 2;
	return c / 2 * (t * t * t + 2) + b;
}

inline real_t out_in(real_t t, real_t b, real_t c, real_t d) {
	if (t < d / 2) {
		return out(t * 2, b, c / 2, d);
	}
	real_t h = c / 2;
	return in(t * 2 - d, b + h, h, d);
}
}; // namespace Cubic

namespace Circ {
inline real_t in(real_t t, real_t b, real_t c, real_t d) {
	t /= d;
	return -c * (Math::sqrt(1 - t * t) - 1) + b;
}

inline real_t out(real_t t, real_t b, real_t c, real_t d) {
	t = t / d - 1;
	return c * Math::sqrt(1 - t * t) + b;
}

inline real_t in_out(real_t t, real_t b, real_t c, real_t d) {
	t /= d / 2;
	if (t < 1) {
		return -c / 2 * (Math::sqrt(1 - t * t) - 1) + b;
	}

	t -= 2;
	return c / 2 * (Math::sqrt(1 - t * t) + 1) + b;
}

inline real_t out_in(real_t t, real_t b, real_t c, real_t d) {
	if (t < d / 2) {
		return out(t * 2, b, c / 2, d);
	}
	real_t h = c / 2;
	return in(t * 2 - d, b + h, h, d);
}
}; // namespace Circ

namespace Bounce {
inline real_t out(real_t t, real_t b, real_t c, real_t d) {
	t /= d;

	if (t < (1 / 2.75f)) {
		return c * (7.5625f * t * t) + b;
	}

	if (t < (2 / 2.75f)) {
		t -= 1.5f / 2.75f;
		return c * (7.5625f * t * t + 0.75f) + b;
	}

	if (t < (2.5 / 2.75)) {
		t -= 2.25f / 2.75f;
		return c * (7.5625f * t * t + 0.9375f) + b;
	}

	t -= 2.625f / 2.75f;
	return c * (7.5625f * t * t + 0.984375f) + b;
}

inline real_t in(real_t t, real_t b, real_t c, real_t d) {
	return c - out(d - t, 0, c, d) + b;
}

inline real_t in_out(real_t t, real_t b, real_t c, real_t d) {
	if (t < d / 2) {
		return in(t * 2, b, c / 2, d);
	}
	real_t h = c / 2;
	return out(t * 2 - d, b + h, h, d);
}

inline real_t out_in(real_t t, real_t b, real_t c, real_t d) {
	if (t < d / 2) {
		return out(t * 2, b, c / 2, d);
	}
	real_t h = c / 2;
	return in(t * 2 - d, b + h, h, d);
}
}; // namespace Bounce

namespace Back {
inline real_t in(real_t t, real_t b, real_t c, real_t d) {
	float s = 1.70158f;
	t /= d;

	return c * t * t * ((s + 1) * t - s) + b;
}

inline real_t out(real_t t, real_t b, real_t c, real_t d) {
	float s = 1.70158f;
	t = t / d - 1;

	return c * (t * t * ((s + 1) * t + s) + 1) + b;
}

inline real_t in_out(real_t t, real_t b, real_t c, real_t d) {
	float s = 1.70158f * 1.525f;
	t /= d / 2;

	if (t < 1) {
		return c / 2 * (t * t * ((s + 1) * t - s)) + b;
	}

	t -= 2;
	return c / 2 * (t * t * ((s + 1) * t + s) + 2) + b;
}

inline real_t out_in(real_t t, real_t b, real_t c, real_t d) {
	if (t < d / 2) {
		return out(t * 2, b, c / 2, d);
	}
	real_t h = c / 2;
	return in(t * 2 - d, b + h, h, d);
}
}; // namespace Back

namespace Spring {
inline real_t out(real_t t, real_t b, real_t c, real_t d) {
	t /= d;
	real_t s = 1.0 - t;
	t = (Math::sin(t * Math::PI * (0.2 + 2.5 * t * t * t)) * Math::pow(s, (real_t)2.2) + t) * (1.0 + (1.2 * s));
	return c * t + b;
}

inline real_t in(real_t t, real_t b, real_t c, real_t d) {
	return c - out(d - t, 0, c, d) + b;
}

inline real_t in_out(real_t t, real_t b, real_t c, real_t d) {
	if (t < d / 2) {
		return in(t * 2, b, c / 2, d);
	}
	real_t h = c / 2;
	return out(t * 2 - d, b + h, h, d);
}

inline real_t out_in(real_t t, real_t b, real_t c, real_t d) {
	if (t < d / 2) {
		return out(t * 2, b, c / 2, d);
	}
	real_t h = c / 2;
	return in(t * 2 - d, b + h, h, d);
}
}; // namespace Spring

// --- End scene/animation/easing_equations.h ---
#include "scene/main/node.h"
#include "scene/main/scene_tree.h"
#include "scene/resources/animation.h"

#define CHECK_VALID()                                                                              \
	ERR_FAIL_COND_V_MSG(                                                                           \
		!valid, nullptr, "Tween invalid. Either finished or created outside scene tree.");         \
	ERR_FAIL_COND_V_MSG(                                                                           \
		started, nullptr, "Can't append to a Tween that has started. Use stop() first.");

Tween::interpolater Tween::interpolaters[Tween::TRANS_MAX][Tween::EASE_MAX] = {
	{&Linear::in, &Linear::in, &Linear::in, &Linear::in}, // Linear is the same for each easing.
	{&Sine::in, &Sine::out, &Sine::in_out, &Sine::out_in},
	{&Quint::in, &Quint::out, &Quint::in_out, &Quint::out_in},
	{&Quart::in, &Quart::out, &Quart::in_out, &Quart::out_in},
	{&Quad::in, &Quad::out, &Quad::in_out, &Quad::out_in},
	{&Expo::in, &Expo::out, &Expo::in_out, &Expo::out_in},
	{&Elastic::in, &Elastic::out, &Elastic::in_out, &Elastic::out_in},
	{&Cubic::in, &Cubic::out, &Cubic::in_out, &Cubic::out_in},
	{&Circ::in, &Circ::out, &Circ::in_out, &Circ::out_in},
	{&Bounce::in, &Bounce::out, &Bounce::in_out, &Bounce::out_in},
	{&Back::in, &Back::out, &Back::in_out, &Back::out_in},
	{&Spring::in, &Spring::out, &Spring::in_out, &Spring::out_in},
};

void Tween::_start_tweeners()
{
	if (tweeners.is_empty()) {
		dead = true;
		ERR_FAIL_MSG("Tween without commands, aborting.");
	}

	for (Ref<Tweener>& tweener : tweeners[current_step]) {
		tweener->start();
	}
}

void Tween::_stop_internal(bool p_reset)
{
	running = false;
	if (p_reset) {
		started = false;
		dead = false;
		total_time = 0;
	}
}

Ref<Tweener> Tween::tween_interval(double p_time)
{
	CHECK_VALID();
	Ref<Tweener> tweener;
	tweener.instantiate(Tweener::TWEENER_INTERVAL, p_time);
	append(tweener);
	return tweener;
}

Ref<Tweener> Tween::tween_subtween(Tween* rp_subtween)
{
	CHECK_VALID();
	ERR_FAIL_NULL_V(rp_subtween, nullptr);

	Ref<Tweener> tweener;
	tweener.instantiate(Ref<Tween>(rp_subtween));

	// Remove the tween from its parent tree, if it has one.
	// If the user created this tween without a parent tree attached,
	// then this step isn't necessary.
	if (tweener->subtween->is_in_tree) {
		SceneTree::remove_tween(tweener->subtween);
		tweener->subtween->is_in_tree = false;
	}
	subtweens.push_back(rp_subtween);
	append(tweener);
	return tweener;
}

Ref<Tweener> Tween::tween_callback(void (*p_callback)())
{
	CHECK_VALID();
	Ref<Tweener> tweener;
	tweener.instantiate(Tweener::TWEENER_CALLBACK);
	tweener->callback = p_callback;
	append(tweener);
	return tweener;
}

Ref<Tweener> Tween::tween_property(const Vector<String>& p_property, double p_duration)
{
	CHECK_VALID();
	Ref<Tweener> tweener;
	tweener.instantiate(Tweener::TWEENER_PROPERTY, p_duration);
	tweener->property = p_property;
	append(tweener);
	return tweener;
}

Ref<Tweener> Tween::tween_method(double p_duration)
{
	CHECK_VALID();
	Ref<Tweener> tweener;
	tweener.instantiate(Tweener::TWEENER_METHOD, p_duration);
	append(tweener);
	return tweener;
}

void Tween::append(Ref<Tweener> p_tweener)
{
	p_tweener->set_tween(this);

	if (parallel_enabled) {
		current_step = MAX(current_step, 0);
	}
	else {
		current_step++;
	}
	parallel_enabled = default_parallel;

	tweeners.resize(current_step + 1);
	tweeners[current_step].push_back(p_tweener);
}

void Tween::stop() { _stop_internal(true); }

void Tween::pause() { _stop_internal(false); }

void Tween::play()
{
	ERR_FAIL_COND_MSG(!valid, "Tween invalid. Either finished or created outside scene tree.");
	ERR_FAIL_COND_MSG(dead, "Can't play finished Tween, use stop() first to reset its state.");
	running = true;
}

void Tween::kill()
{
	running = false; // For the sake of is_running().
	valid = false;
	dead = true;

	// Kill all subtweens of this tween.
	for (Ref<Tween>& st : subtweens) {
		st->kill();
	}
}

bool Tween::has_tweeners() const { return !tweeners.is_empty(); }

bool Tween::is_running() { return running; }

bool Tween::is_valid() { return valid; }

void Tween::clear()
{
	valid = false;
	tweeners.clear();
}

Tween* Tween::set_process_mode(TweenProcessMode p_mode)
{
	process_mode = p_mode;
	return this;
}

Tween::TweenProcessMode Tween::get_process_mode() const { return process_mode; }

Tween* Tween::set_pause_mode(TweenPauseMode p_mode)
{
	pause_mode = p_mode;
	return this;
}

Tween::TweenPauseMode Tween::get_pause_mode() const { return pause_mode; }

Tween* Tween::set_ignore_time_scale(bool p_ignore)
{
	ignore_time_scale = p_ignore;
	return this;
}

bool Tween::is_ignoring_time_scale() const { return ignore_time_scale; }

Tween* Tween::set_parallel(bool p_parallel)
{
	default_parallel = p_parallel;
	parallel_enabled = p_parallel;
	return this;
}

Tween* Tween::set_loops(int p_loops)
{
	loops = p_loops;
	return this;
}

int Tween::get_loops_left() const
{
	if (loops <= 0) {
		return -1; // Infinite loop.
	}
	else {
		return loops - loops_done;
	}
}

Tween* Tween::set_speed_scale(float p_speed)
{
	speed_scale = p_speed;
	return this;
}

Tween* Tween::set_trans(TransitionType p_trans)
{
	default_transition = p_trans;
	return this;
}

Tween::TransitionType Tween::get_trans() const { return default_transition; }

Tween* Tween::set_ease(EaseType p_ease)
{
	default_ease = p_ease;
	return this;
}

Tween::EaseType Tween::get_ease() const { return default_ease; }

Tween* Tween::parallel()
{
	parallel_enabled = true;
	return this;
}

Tween* Tween::chain()
{
	parallel_enabled = false;
	return this;
}

bool Tween::custom_step(double p_delta)
{
	ERR_FAIL_COND_V_MSG(in_step, true, "Can't call custom_step() during another Tween step.");

	bool r = running;
	running = true;
	bool ret = step(p_delta);
	running = running && r; // Running might turn false when Tween finished.
	return ret;
}

bool Tween::step(double p_delta) { return true; }

bool Tween::can_process(bool p_tree_paused) const
{
	if (is_bound && pause_mode == TWEEN_PAUSE_BOUND) {
		Node* node = get_bound_node();
		if (node) {
			return node->is_inside_tree() && node->can_process();
		}
	}

	return !p_tree_paused || pause_mode == TWEEN_PAUSE_PROCESS;
}

double Tween::get_total_time() const { return total_time; }

real_t Tween::run_equation(TransitionType p_trans_type, EaseType p_ease_type, real_t p_time,
	real_t p_initial, real_t p_delta, real_t p_duration)
{
	if (p_duration == 0) {
		// Special case to avoid dividing by 0 in equations.
		return p_initial + p_delta;
	}

	interpolater func = interpolaters[p_trans_type][p_ease_type];
	return func(p_time, p_initial, p_delta, p_duration);
}

Tween* Tween::bind_node(const Node* rp_node)
{
	ERR_FAIL_NULL_V(rp_node, this);
	bound_node = const_cast<Node*>(rp_node);
	is_bound = true;
	return this;
}

Node* Tween::get_bound_node() const { return bound_node; }

String Tween::_to_string() { return String(); }

Tween::Tween() { ERR_FAIL_MSG("Tween can't be created directly. Use create_tween() method."); }

Tween::Tween(bool p_in_tree)
{
	is_in_tree = p_in_tree;
	valid = true;
}

Tween::~Tween() = default;

Ref<Tweener> Tweener::from(double p_value)
{
	initial_val = p_value;
	do_continue = false;
	return this;
}

Ref<Tweener> Tweener::from_current()
{
	do_continue = false;
	return this;
}

Ref<Tweener> Tweener::as_relative()
{
	relative = true;
	return this;
}

Ref<Tweener> Tweener::set_trans(Tween::TransitionType p_trans)
{
	trans_type = p_trans;
	return this;
}

Ref<Tweener> Tweener::set_ease(Tween::EaseType p_ease)
{
	ease_type = p_ease;
	return this;
}

Ref<Tweener> Tweener::set_custom_interpolator(real_t (*p_interpolator)(real_t))
{
	custom_interpolator = p_interpolator;
	return this;
}

Ref<Tweener> Tweener::set_delay(double p_delay)
{
	delay = p_delay;
	return this;
}

Ref<Tweener> Tweener::set_timeout(double p_timeout)
{
	timeout = p_timeout;
	return this;
}

void Tweener::set_tween(const Ref<Tween>& p_tween)
{
	tween = p_tween.ptr();
	if (type == TWEENER_PROPERTY || type == TWEENER_METHOD) {
		if (trans_type == Tween::TRANS_MAX && p_tween.is_valid()) {
			trans_type = p_tween->get_trans();
		}
		if (ease_type == Tween::EASE_MAX && p_tween.is_valid()) {
			ease_type = p_tween->get_ease();
		}
	}
}

Ref<Tween> Tweener::_get_tween() { return Ref<Tween>(tween); }

void Tweener::_finish() { finished = true; }

void Tweener::start()
{
	elapsed_time = 0;
	finished = false;

	switch (type) {
	case TWEENER_SUBTWEEN: {
		if (subtween.is_null()) {
			_finish();
			return;
		}
		// Reset the subtween.
		subtween->stop();

		// It's possible that a subtween could be killed before it is started;
		// if so, we just want to skip it entirely.
		if (subtween->is_valid()) {
			subtween->play();
		}
		else {
			_finish();
		}
	} break;
	case TWEENER_AWAIT: {
		received = false;
	} break;
	case TWEENER_INTERVAL:
	case TWEENER_CALLBACK:
	case TWEENER_PROPERTY:
	case TWEENER_METHOD:
		break;
	}
}

bool Tweener::step(double& r_delta)
{
	switch (type) {
	case TWEENER_INTERVAL: {
		if (finished) {
			return false;
		}

		elapsed_time += r_delta;

		if (elapsed_time < duration) {
			r_delta = 0;
			return true;
		}
		else {
			r_delta = elapsed_time - duration;
			_finish();
			return false;
		}
	}
	case TWEENER_SUBTWEEN: {
		if (finished) {
			return false;
		}

		elapsed_time += r_delta;

		if (elapsed_time < delay) {
			r_delta = 0;
			return true;
		}

		if (subtween.is_null() || !subtween->step(r_delta)) {
			r_delta = elapsed_time - delay - (subtween.is_valid() ? subtween->get_total_time() : 0.0);
			_finish();
			return false;
		}

		r_delta = 0;
		return true;
	}
	case TWEENER_CALLBACK: {
		if (finished) {
			return false;
		}

		elapsed_time += r_delta;
		if (elapsed_time < delay) {
			r_delta = 0;
			return true;
		}

		if (callback) {
			callback();
		}
		r_delta = elapsed_time - delay;
		_finish();
		return false;
	}
	case TWEENER_PROPERTY:
	case TWEENER_METHOD:
	case TWEENER_AWAIT: {
		return true;
	}
	}
	return false;
}

Tweener::Tweener()
{
	ERR_FAIL_MSG("Tweener can't be created directly. Use the tween_*() methods in Tween.");
}

Tweener::Tweener(Type p_type) : type(p_type) {}

Tweener::Tweener(Type p_type, double p_duration) : type(p_type), duration(p_duration) {}

Tweener::Tweener(const Ref<Tween>& p_subtween) : subtween(p_subtween), type(TWEENER_SUBTWEEN) {}
