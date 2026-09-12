/**************************************************************************/
/*  animation_mixer.cpp                                                   */
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

#include "animation_mixer.h"
#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/string/string_name.h"
#include "scene/2d/audio_stream_player_2d.h"
#include "scene/animation/animation_player.h"
#include "scene/audio/audio_stream_player.h"
#include "scene/resources/animation.h"
#include "servers/audio/audio_server.h"
#include "servers/audio/audio_stream.h"

#ifndef _3D_DISABLED
#include "scene/3d/audio_stream_player_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/node_3d.h"
#include "scene/3d/skeleton_3d.h"
#endif // _3D_DISABLED

#ifdef TOOLS_ENABLED
#include "editor/editor_undo_redo_manager.h"
#endif // TOOLS_ENABLED

void AnimationMixer::_animation_added(const StringName& p_name, const StringName& p_library)
{
	_animation_set_cache_update();
}

void AnimationMixer::_animation_removed(const StringName& p_name, const StringName& p_library)
{
	const StringName name =
		p_library == StringName() ? p_name : StringName(String(p_library) + "/" + String(p_name));

	if (!animation_set.has(name)) {
		return; // No need to update because not the one from the library being used.
	}

	_animation_set_cache_update();

	_remove_animation(name);
}

void AnimationMixer::_animation_changed(const StringName& p_name) { _clear_caches(); }

void AnimationMixer::get_animation_library_list(LocalVector<StringName>* p_libraries) const
{
	for (const AnimationLibraryData& lib : animation_libraries) {
		p_libraries->push_back(lib.name);
	}
}

Ref<AnimationLibrary> AnimationMixer::get_animation_library(const StringName& p_name) const
{
	for (const AnimationLibraryData& lib : animation_libraries) {
		if (lib.name == p_name) {
			return lib.library;
		}
	}
	ERR_FAIL_V(Ref<AnimationLibrary>());
}

bool AnimationMixer::has_animation_library(const StringName& p_name) const
{
	for (const AnimationLibraryData& lib : animation_libraries) {
		if (lib.name == p_name) {
			return true;
		}
	}

	return false;
}

const StringName& AnimationMixer::find_animation_library(const Ref<Animation>& p_animation) const
{
	for (const KeyValue<StringName, AnimationData>& E : animation_set) {
		if (E.value.animation == p_animation) {
			return E.value.animation_library;
		}
	}
	const static StringName empty = StringName();
	return empty;
}

LocalVector<StringName> AnimationMixer::get_sorted_animation_list() const
{
	LocalVector<StringName> animations;
	get_animation_list(&animations);
	animations.sort_custom<StringName::AlphCompare>();
	return animations;
}

void AnimationMixer::get_animation_list(LocalVector<StringName>* p_animations) const
{
	p_animations->reserve(p_animations->size() + animation_set.size());
	for (const KeyValue<StringName, AnimationData>& E : animation_set) {
		p_animations->push_back(E.key);
	}
}

const Ref<Animation>& AnimationMixer::get_animation(const StringName& p_name) const
{
	const Ref<Animation>& animation = get_animation_or_null(p_name);
	ERR_FAIL_COND_V_MSG(
		animation.is_null(), animation, vformat("Animation not found: \"%s\".", p_name));
	return animation;
}

const Ref<Animation>& AnimationMixer::get_animation_or_null(const StringName& p_name) const
{
	const AnimationData* ad = animation_set.getptr(p_name);
	if (!ad) {
		const static Ref<Animation> empty = Ref<Animation>();
		return empty;
	}
	return ad->animation;
}

bool AnimationMixer::has_animation(const StringName& p_name) const
{
	return animation_set.has(p_name);
}

StringName AnimationMixer::find_animation(const Ref<Animation>& p_animation) const
{
	for (const KeyValue<StringName, AnimationData>& E : animation_set) {
		if (E.value.animation == p_animation) {
			return E.key;
		}
	}
	return StringName();
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

			// In some cases (e.g. TrackCacheTransform),
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
	const StringName& p_name, const PlaybackInfo& p_playback_info)
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

bool AnimationMixer::can_apply_reset() const { return has_animation(SceneStringName(RESET)); }

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
	const StringName& p_function, int p_idx, List<String>* r_options) const
{
	const String pf = p_function;
	if (p_idx == 0) {
		if (pf == "get_animation" || pf == "has_animation") {
			for (const StringName& name : get_sorted_animation_list()) {
				r_options->push_back(String(name).quote());
			}
		}
		else if (pf == "get_animation_library" || pf == "has_animation_library" ||
				   pf == "remove_animation_library" || pf == "rename_animation_library") {
			LocalVector<StringName> al;
			get_animation_library_list(&al);
			for (const StringName& name : al) {
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
	case Animation::TYPE_VALUE: {
		AnimationMixer::TrackCacheValue* src =
			static_cast<AnimationMixer::TrackCacheValue*>(p_cache);
		AnimationMixer::TrackCacheValue* tc = memnew(AnimationMixer::TrackCacheValue(*src));
		return tc;
	}

	case Animation::TYPE_POSITION_3D:
	case Animation::TYPE_ROTATION_3D:
	case Animation::TYPE_SCALE_3D: {
		AnimationMixer::TrackCacheTransform* src =
			static_cast<AnimationMixer::TrackCacheTransform*>(p_cache);
		AnimationMixer::TrackCacheTransform* tc = memnew(AnimationMixer::TrackCacheTransform(*src));
		return tc;
	}

	case Animation::TYPE_BLEND_SHAPE: {
		AnimationMixer::TrackCacheBlendShape* src =
			static_cast<AnimationMixer::TrackCacheBlendShape*>(p_cache);
		AnimationMixer::TrackCacheBlendShape* tc =
			memnew(AnimationMixer::TrackCacheBlendShape(*src));
		return tc;
	}

	case Animation::TYPE_AUDIO: {
		AnimationMixer::TrackCacheAudio* src =
			static_cast<AnimationMixer::TrackCacheAudio*>(p_cache);
		AnimationMixer::TrackCacheAudio* tc = memnew(AnimationMixer::TrackCacheAudio(*src));
		return tc;
	}

	case Animation::TYPE_METHOD:
	case Animation::TYPE_ANIMATION: {
		// Nothing to do here.
	} break;
	}
	return nullptr;
}


