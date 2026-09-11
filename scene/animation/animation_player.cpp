/**************************************************************************/
/*  animation_player.cpp                                                  */
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

#include "animation_player.compat.inc"
#include "animation_player.h"
#include "core/config/engine.h"
#include "core/os/os.h"
#include "scene/main/scene_tree.h"

void AnimationPlayer::_notification(int p_what)
{
	switch (p_what) {
	case NOTIFICATION_READY: {
		if (!Engine::get_singleton()->is_editor_hint() && animation_set.has(autoplay)) {
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

void AnimationPlayer::queue(const StringName& p_name)
{
	if (!is_playing()) {
		play(p_name);
	}
	else {
		playback_queue.push_back(p_name);
	}
}

void AnimationPlayer::clear_queue() { playback_queue.clear(); }

void AnimationPlayer::play_backwards(const StringName& p_name, double p_custom_blend)
{
	play(p_name, p_custom_blend, -1, true);
}

void AnimationPlayer::play_section_with_markers_backwards(const StringName& p_name,
	const StringName& p_start_marker, const StringName& p_end_marker, double p_custom_blend)
{
	play_section_with_markers(p_name, p_start_marker, p_end_marker, p_custom_blend, -1, true);
}

void AnimationPlayer::play_section_backwards(
	const StringName& p_name, double p_start_time, double p_end_time, double p_custom_blend)
{
	play_section(p_name, p_start_time, p_end_time, p_custom_blend, -1, true);
}

void AnimationPlayer::play(
	const StringName& p_name, double p_custom_blend, float p_custom_scale, bool p_from_end)
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
	const StringName& p_name, double p_custom_blend, float p_custom_scale, bool p_from_end)
{
	play_section_with_markers(
		p_name, StringName(), StringName(), p_custom_blend, p_custom_scale, p_from_end);
}

void AnimationPlayer::play_section_with_markers(const StringName& p_name,
	const StringName& p_start_marker, const StringName& p_end_marker, double p_custom_blend,
	float p_custom_scale, bool p_from_end)
{
	StringName name = p_name;

	if (name == StringName()) {
		name = playback.assigned;
	}

	AnimationData* ad = animation_set.getptr(name);
	ERR_FAIL_NULL_MSG(ad, vformat("Animation not found: %s.", name));

	const Ref<Animation>& animation = ad->animation;

	ERR_FAIL_COND_MSG(p_start_marker == p_end_marker && p_start_marker,
		vformat("Start marker and end marker cannot be the same marker: %s.", p_start_marker));
	ERR_FAIL_COND_MSG(p_start_marker && !animation->has_marker(p_start_marker),
		vformat("Marker %s not found in animation: %s.", p_start_marker, name));
	ERR_FAIL_COND_MSG(p_end_marker && !animation->has_marker(p_end_marker),
		vformat("Marker %s not found in animation: %s.", p_end_marker, name));

	double start_time = p_start_marker ? animation->get_marker_time(p_start_marker) : -1;
	double end_time = p_end_marker ? animation->get_marker_time(p_end_marker) : -1;

	ERR_FAIL_COND_MSG(
		p_start_marker && p_end_marker && Animation::is_greater_approx(start_time, end_time),
		vformat("End marker %s is placed earlier than start marker %s in animation: %s.",
			p_end_marker, p_start_marker, name));

	if (p_start_marker && Animation::is_less_approx(start_time, 0)) {
		WARN_PRINT_ED(vformat("Negative time start marker: %s is invalid in the section, so the "
							  "start of the animation: %s is used instead.",
			p_start_marker, playback.current.animation_name));
	}
	if (p_end_marker && Animation::is_less_approx(end_time, 0)) {
		WARN_PRINT_ED(vformat("Negative time end marker: %s is invalid in the section, so the end "
							  "of the animation: %s is used instead.",
			p_end_marker, playback.current.animation_name));
	}

	play_section(name, start_time, end_time, p_custom_blend, p_custom_scale, p_from_end);
}

void AnimationPlayer::play_with_capture(const StringName& p_name, double p_duration,
	double p_custom_blend, float p_custom_scale, bool p_from_end,
	Tween::TransitionType p_trans_type, Tween::EaseType p_ease_type)
{
	_capture(p_name, p_from_end, p_duration, p_trans_type, p_ease_type);
	_play(p_name, p_custom_blend, p_custom_scale, p_from_end);
}

bool AnimationPlayer::is_playing() const { return playing; }

StringName AnimationPlayer::get_current_animation() const
{
	return (is_playing() ? playback.assigned : StringName());
}

StringName AnimationPlayer::get_assigned_animation() const { return playback.assigned; }

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
	const StringName& p_start_marker, const StringName& p_end_marker)
{
	ERR_FAIL_COND_MSG(!playback.current.is_enabled, "AnimationPlayer has no current animation.");
	ERR_FAIL_COND_MSG(p_start_marker == p_end_marker && p_start_marker,
		vformat("Start marker and end marker cannot be the same marker: %s.", p_start_marker));
	ERR_FAIL_COND_MSG(
		p_start_marker &&
			!animation_set[playback.current.animation_name].animation->has_marker(p_start_marker),
		vformat("Marker %s not found in animation: %s.", p_start_marker,
			playback.current.animation_name));
	ERR_FAIL_COND_MSG(
		p_end_marker &&
			!animation_set[playback.current.animation_name].animation->has_marker(p_end_marker),
		vformat("Marker %s not found in animation: %s.", p_end_marker,
			playback.current.animation_name));
	double start_time =
		p_start_marker ? animation_set[playback.current.animation_name].animation->get_marker_time(
							 p_start_marker)
					   : -1;
	double end_time =
		p_end_marker ? animation_set[playback.current.animation_name].animation->get_marker_time(
						   p_end_marker)
					 : -1;
	if (p_start_marker && Animation::is_less_approx(start_time, 0)) {
		WARN_PRINT_ONCE_ED(vformat("Marker %s time must be positive in animation: %s.",
			p_start_marker, playback.current.animation_name));
	}
	if (p_end_marker && Animation::is_less_approx(end_time, 0)) {
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

void AnimationPlayer::set_autoplay(const StringName& p_name)
{
	if (is_inside_tree() && !Engine::get_singleton()->is_editor_hint()) {
		WARN_PRINT("Setting autoplay after the node has been added to the scene has no effect.");
	}

	autoplay = p_name;
}

StringName AnimationPlayer::get_autoplay() const { return autoplay; }

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

void AnimationPlayer::_animation_changed(const StringName& p_name)
{
	AnimationMixer::_animation_changed(p_name);
	if (playback.current.is_enabled && playback.current.animation_name == p_name &&
		animation_set.has(p_name)) {
		playback.current.animation_length = animation_set[p_name].animation->get_length();
	}
}

void AnimationPlayer::animation_set_next(const StringName& p_animation, const StringName& p_next)
{
	ERR_FAIL_COND_MSG(
		!animation_set.has(p_animation), vformat("Animation not found: %s.", p_animation));
	animation_next_set[p_animation] = p_next;
}

StringName AnimationPlayer::animation_get_next(const StringName& p_animation) const
{
	const StringName* next = animation_next_set.getptr(p_animation);
	if (!next) {
		return StringName();
	}
	return *next;
}

void AnimationPlayer::set_default_blend_time(double p_default) { default_blend_time =
 p_default; }

double AnimationPlayer::get_default_blend_time() const { return default_blend_time; }

void AnimationPlayer::set_blend_time(
	const StringName& p_animation1, const StringName& p_animation2, double p_time)
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
	const StringName& p_animation1, const StringName& p_animation2) const
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
	const StringName& p_function, int p_idx, List<String>* r_options) const
{
	const String pf = p_function;
	if (p_idx == 0 &&
		(pf == "play" || pf == "play_backwards" || pf == "has_animation" || pf == "queue")) {
		for (const StringName& name : get_sorted_animation_list()) {
			r_options->push_back(String(name).quote());
		}
	}
	AnimationMixer::get_argument_options(p_function, p_idx, r_options);
}
#endif

void AnimationPlayer::_animation_removed(const StringName& p_name, const StringName& p_library)
{
	AnimationMixer::_animation_removed(p_name, p_library);

	const StringName& name =
		p_library == StringName() ? p_name : StringName(String(p_library) + "/" + String(p_name));

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

void AnimationPlayer::_rename_animation(const StringName& p_from_name, const StringName& p_to_name)
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


