/**************************************************************************/
/*  audio_stream_interactive.cpp                                          */
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

#include "audio_stream_interactive.h"
#include "core/config/engine.h"
#include "core/math/math_funcs.h"

AudioStreamInteractive::AudioStreamInteractive() {}

void AudioStreamInteractive::set_mode(Mode p_mode)
{
	ERR_FAIL_UNSIGNED_INDEX(p_mode, 3);
	if (mode == p_mode) {
		return;
	}

	AudioServer::lock();
	mode = p_mode;
	if (mode == MODE_SYNCHRONIZED && stream_count > MAX_SYNC_STREAMS) {
		stream_count = MAX_SYNC_STREAMS;
	}
	if (mode == MODE_PLAYLIST || mode == MODE_SYNCHRONIZED) {
		if ((uint32_t)stream_count > audio_streams.size()) {
			audio_streams.resize(stream_count);
		}
		if (mode == MODE_SYNCHRONIZED && (uint32_t)stream_count > audio_stream_volume_db.size()) {
			uint32_t old_size = audio_stream_volume_db.size();
			audio_stream_volume_db.resize(stream_count);
			for (uint32_t i = old_size; i < (uint32_t)stream_count; i++) {
				audio_stream_volume_db[i] = 0.0f;
			}
		}
	}
	version++;
	for (AudioStreamPlaybackInteractive* E : playbacks) {
		E->stop();
		if (mode == MODE_PLAYLIST || mode == MODE_SYNCHRONIZED) {
			E->_update_playback_instances();
		}
	}
	AudioServer::unlock();
}

AudioStreamInteractive::Mode AudioStreamInteractive::get_mode() const { return mode; }

Ref<AudioStreamPlayback> AudioStreamInteractive::instantiate_playback()
{
	Ref<AudioStreamPlaybackInteractive> pb;
	pb.instantiate();
	pb->stream = Ref<AudioStreamInteractive>(this);
	if (mode == MODE_PLAYLIST || mode == MODE_SYNCHRONIZED) {
		pb->_update_playback_instances();
	}
	playbacks.insert(pb.ptr());
	return pb;
}

// CLIPS (MODE_CLIPS)

void AudioStreamInteractive::set_clip_count(int p_count)
{
	ERR_FAIL_COND(p_count < 0 || p_count > MAX_CLIPS);
	AudioDriverManager::get_active_driver()->lock();
	if ((uint32_t)p_count > clips.size()) {
		clips.resize(p_count);
	}
	clip_count = p_count;
	version++;
#ifdef TOOLS_ENABLED
	stream_name_cache = String();
#endif
	AudioDriverManager::get_active_driver()->unlock();
}

int AudioStreamInteractive::get_clip_count() const { return clip_count; }

void AudioStreamInteractive::set_initial_clip(int p_clip)
{
	ERR_FAIL_INDEX(p_clip, clip_count);
	initial_clip = p_clip;
}

int AudioStreamInteractive::get_initial_clip() const { return initial_clip; }

void AudioStreamInteractive::set_clip_name(int p_clip, const String& p_name)
{
	ERR_FAIL_INDEX(p_clip, MAX_CLIPS);
	if ((uint32_t)p_clip >= clips.size()) {
		clips.resize(p_clip + 1);
	}
	clips[p_clip].name = p_name;
#ifdef TOOLS_ENABLED
	stream_name_cache = String();
#endif
}

String AudioStreamInteractive::get_clip_name(int p_clip) const
{
	ERR_FAIL_COND_V(p_clip < -1 || p_clip >= MAX_CLIPS, String());
	if (p_clip == CLIP_ANY) {
		return RTR("All Clips");
	}
	return (uint32_t)p_clip < clips.size() ? clips[p_clip].name : String();
}

void AudioStreamInteractive::set_clip_stream(int p_clip, const Ref<AudioStream>& p_stream)
{
	ERR_FAIL_COND(p_stream == this);
	ERR_FAIL_INDEX(p_clip, MAX_CLIPS);
	AudioDriverManager::get_active_driver()->lock();
	if ((uint32_t)p_clip >= clips.size()) {
		clips.resize(p_clip + 1);
	}
	clips[p_clip].stream = p_stream;
	version++;
#ifdef TOOLS_ENABLED
	stream_name_cache = String();
#endif
	AudioDriverManager::get_active_driver()->unlock();
}

Ref<AudioStream> AudioStreamInteractive::get_clip_stream(int p_clip) const
{
	ERR_FAIL_INDEX_V(p_clip, MAX_CLIPS, Ref<AudioStream>());
	return (uint32_t)p_clip < clips.size() ? clips[p_clip].stream : Ref<AudioStream>();
}

void AudioStreamInteractive::set_clip_auto_advance(int p_clip, AutoAdvanceMode p_mode)
{
	ERR_FAIL_INDEX(p_clip, MAX_CLIPS);
	ERR_FAIL_UNSIGNED_INDEX(p_mode, 3);
	if ((uint32_t)p_clip >= clips.size()) {
		clips.resize(p_clip + 1);
	}
	clips[p_clip].auto_advance = p_mode;
}

AudioStreamInteractive::AutoAdvanceMode AudioStreamInteractive::get_clip_auto_advance(
	int p_clip) const
{
	ERR_FAIL_INDEX_V(p_clip, MAX_CLIPS, AUTO_ADVANCE_DISABLED);
	return (uint32_t)p_clip < clips.size() ? clips[p_clip].auto_advance : AUTO_ADVANCE_DISABLED;
}

void AudioStreamInteractive::set_clip_auto_advance_next_clip(int p_clip, int p_index)
{
	ERR_FAIL_INDEX(p_clip, MAX_CLIPS);
	if ((uint32_t)p_clip >= clips.size()) {
		clips.resize(p_clip + 1);
	}
	clips[p_clip].auto_advance_next_clip = p_index;
}

int AudioStreamInteractive::get_clip_auto_advance_next_clip(int p_clip) const
{
	ERR_FAIL_INDEX_V(p_clip, MAX_CLIPS, -1);
	return (uint32_t)p_clip < clips.size() ? clips[p_clip].auto_advance_next_clip : 0;
}

// TRANSITIONS (MODE_CLIPS)

bool AudioStreamInteractive::has_transition(int p_from_clip, int p_to_clip) const
{
	TransitionKey tk(p_from_clip, p_to_clip);
	return transition_map.has(tk);
}

void AudioStreamInteractive::erase_transition(int p_from_clip, int p_to_clip)
{
	TransitionKey tk(p_from_clip, p_to_clip);
	ERR_FAIL_COND(!transition_map.has(tk));
	AudioDriverManager::get_active_driver()->lock();
	transition_map.erase(tk);
	AudioDriverManager::get_active_driver()->unlock();
}

PackedInt32Array AudioStreamInteractive::get_transition_list() const
{
	PackedInt32Array ret;

	for (const KeyValue<TransitionKey, Transition>& K : transition_map) {
		ret.push_back(K.key.from_clip);
		ret.push_back(K.key.to_clip);
	}
	return ret;
}

void AudioStreamInteractive::add_transition(int p_from_clip, int p_to_clip,
	TransitionFromTime p_from_time, TransitionToTime p_to_time, FadeMode p_fade_mode,
	float p_fade_beats, bool p_use_filler_flip, int p_filler_clip, bool p_hold_previous)
{
	ERR_FAIL_COND(p_from_clip < CLIP_ANY || p_from_clip >= clip_count);
	ERR_FAIL_COND(p_to_clip < CLIP_ANY || p_to_clip >= clip_count);
	ERR_FAIL_UNSIGNED_INDEX(p_from_time, TRANSITION_FROM_TIME_MAX);
	ERR_FAIL_UNSIGNED_INDEX(p_to_time, TRANSITION_TO_TIME_MAX);
	ERR_FAIL_UNSIGNED_INDEX(p_fade_mode, FADE_MAX);

	Transition tr;
	tr.from_time = p_from_time;
	tr.to_time = p_to_time;
	tr.fade_mode = p_fade_mode;
	tr.fade_beats = p_fade_beats;
	tr.use_filler_clip = p_use_filler_flip;
	tr.filler_clip = p_filler_clip;
	tr.hold_previous = p_hold_previous;

	TransitionKey tk(p_from_clip, p_to_clip);

	AudioDriverManager::get_active_driver()->lock();
	transition_map[tk] = tr;
	AudioDriverManager::get_active_driver()->unlock();
}

AudioStreamInteractive::TransitionFromTime AudioStreamInteractive::get_transition_from_time(
	int p_from_clip, int p_to_clip) const
{
	TransitionKey tk(p_from_clip, p_to_clip);
	ERR_FAIL_COND_V(!transition_map.has(tk), TRANSITION_FROM_TIME_END);
	return transition_map[tk].from_time;
}

AudioStreamInteractive::TransitionToTime AudioStreamInteractive::get_transition_to_time(
	int p_from_clip, int p_to_clip) const
{
	TransitionKey tk(p_from_clip, p_to_clip);
	ERR_FAIL_COND_V(!transition_map.has(tk), TRANSITION_TO_TIME_START);
	return transition_map[tk].to_time;
}

AudioStreamInteractive::FadeMode AudioStreamInteractive::get_transition_fade_mode(
	int p_from_clip, int p_to_clip) const
{
	TransitionKey tk(p_from_clip, p_to_clip);
	ERR_FAIL_COND_V(!transition_map.has(tk), FADE_DISABLED);
	return transition_map[tk].fade_mode;
}

float AudioStreamInteractive::get_transition_fade_beats(int p_from_clip, int p_to_clip) const
{
	TransitionKey tk(p_from_clip, p_to_clip);
	ERR_FAIL_COND_V(!transition_map.has(tk), -1);
	return transition_map[tk].fade_beats;
}

bool AudioStreamInteractive::is_transition_using_filler_clip(int p_from_clip, int p_to_clip) const
{
	TransitionKey tk(p_from_clip, p_to_clip);
	ERR_FAIL_COND_V(!transition_map.has(tk), false);
	return transition_map[tk].use_filler_clip;
}

int AudioStreamInteractive::get_transition_filler_clip(int p_from_clip, int p_to_clip) const
{
	TransitionKey tk(p_from_clip, p_to_clip);
	ERR_FAIL_COND_V(!transition_map.has(tk), -1);
	return transition_map[tk].filler_clip;
}

bool AudioStreamInteractive::is_transition_holding_previous(int p_from_clip, int p_to_clip) const
{
	TransitionKey tk(p_from_clip, p_to_clip);
	ERR_FAIL_COND_V(!transition_map.has(tk), false);
	return transition_map[tk].hold_previous;
}

// PLAYLIST & SYNCHRONIZED STREAMS

void AudioStreamInteractive::set_stream_count(int p_count)
{
	int max_streams = (mode == MODE_SYNCHRONIZED) ? MAX_SYNC_STREAMS : MAX_STREAMS;
	ERR_FAIL_COND(p_count < 0 || p_count > max_streams);
	AudioServer::lock();
	if ((uint32_t)p_count > audio_streams.size()) {
		audio_streams.resize(p_count);
	}
	if ((uint32_t)p_count > audio_stream_volume_db.size()) {
		uint32_t old_size = audio_stream_volume_db.size();
		audio_stream_volume_db.resize(p_count);
		for (uint32_t i = old_size; i < (uint32_t)p_count; i++) {
			audio_stream_volume_db[i] = 0.0f;
		}
	}
	stream_count = p_count;
	if (mode == MODE_PLAYLIST || mode == MODE_SYNCHRONIZED) {
		for (AudioStreamPlaybackInteractive* E : playbacks) {
			E->_update_playback_instances();
		}
	}
	AudioServer::unlock();
}

int AudioStreamInteractive::get_stream_count() const { return stream_count; }

void AudioStreamInteractive::set_fade_time(float p_time) { fade_time = p_time; }

float AudioStreamInteractive::get_fade_time() const { return fade_time; }

void AudioStreamInteractive::set_shuffle(bool p_shuffle) { shuffle = p_shuffle; }

bool AudioStreamInteractive::get_shuffle() const { return shuffle; }

void AudioStreamInteractive::set_loop(bool p_loop) { loop = p_loop; }

void AudioStreamInteractive::set_list_stream(int p_stream_index, Ref<AudioStream> p_stream)
{
	ERR_FAIL_COND(p_stream == this);
	ERR_FAIL_INDEX(p_stream_index, MAX_STREAMS);

	AudioServer::lock();
	if ((uint32_t)p_stream_index >= audio_streams.size()) {
		audio_streams.resize(p_stream_index + 1);
	}
	audio_streams[p_stream_index] = p_stream;
	if (mode == MODE_PLAYLIST || mode == MODE_SYNCHRONIZED) {
		for (AudioStreamPlaybackInteractive* E : playbacks) {
			E->_update_playback_instances();
		}
	}
	AudioServer::unlock();
}

Ref<AudioStream> AudioStreamInteractive::get_list_stream(int p_stream_index) const
{
	ERR_FAIL_INDEX_V(p_stream_index, MAX_STREAMS, Ref<AudioStream>());
	return (uint32_t)p_stream_index < audio_streams.size() ? audio_streams[p_stream_index]
														   : Ref<AudioStream>();
}

void AudioStreamInteractive::set_sync_stream(int p_stream_index, Ref<AudioStream> p_stream)
{
	ERR_FAIL_COND(p_stream == this);
	ERR_FAIL_INDEX(p_stream_index, MAX_SYNC_STREAMS);

	AudioServer::lock();
	if ((uint32_t)p_stream_index >= audio_streams.size()) {
		audio_streams.resize(p_stream_index + 1);
	}
	audio_streams[p_stream_index] = p_stream;
	if (mode == MODE_PLAYLIST || mode == MODE_SYNCHRONIZED) {
		for (AudioStreamPlaybackInteractive* E : playbacks) {
			E->_update_playback_instances();
		}
	}
	AudioServer::unlock();
}

Ref<AudioStream> AudioStreamInteractive::get_sync_stream(int p_stream_index) const
{
	ERR_FAIL_INDEX_V(p_stream_index, MAX_SYNC_STREAMS, Ref<AudioStream>());
	return (uint32_t)p_stream_index < audio_streams.size() ? audio_streams[p_stream_index]
														   : Ref<AudioStream>();
}

void AudioStreamInteractive::set_sync_stream_volume(int p_stream_index, float p_db)
{
	ERR_FAIL_INDEX(p_stream_index, MAX_SYNC_STREAMS);
	if ((uint32_t)p_stream_index >= audio_stream_volume_db.size()) {
		uint32_t old_size = audio_stream_volume_db.size();
		audio_stream_volume_db.resize(p_stream_index + 1);
		for (uint32_t i = old_size; i <= (uint32_t)p_stream_index; i++) {
			audio_stream_volume_db[i] = 0.0f;
		}
	}
	audio_stream_volume_db[p_stream_index] = p_db;
}

float AudioStreamInteractive::get_sync_stream_volume(int p_stream_index) const
{
	ERR_FAIL_INDEX_V(p_stream_index, MAX_SYNC_STREAMS, 0);
	return (uint32_t)p_stream_index < audio_stream_volume_db.size()
			   ? audio_stream_volume_db[p_stream_index]
			   : 0.0f;
}

// AudioStream virtual overrides

double AudioStreamInteractive::get_bpm() const
{
	if (mode == MODE_PLAYLIST || mode == MODE_SYNCHRONIZED) {
		int count = MIN(stream_count, (int)audio_streams.size());
		for (int i = 0; i < count; i++) {
			if (audio_streams[i].is_valid()) {
				double bpm = audio_streams[i]->get_bpm();
				if (bpm != 0.0) {
					return bpm;
				}
			}
		}
	}
	return 0.0;
}

int AudioStreamInteractive::get_beat_count() const
{
	if (mode == MODE_SYNCHRONIZED) {
		int max_beats = 0;
		int count = MIN(stream_count, (int)audio_streams.size());
		for (int i = 0; i < count; i++) {
			if (audio_streams[i].is_valid()) {
				max_beats = MAX(max_beats, audio_streams[i]->get_beat_count());
			}
		}
		return max_beats;
	}
	return 0;
}

int AudioStreamInteractive::get_bar_beats() const
{
	if (mode == MODE_SYNCHRONIZED) {
		int count = MIN(stream_count, (int)audio_streams.size());
		for (int i = 0; i < count; i++) {
			if (audio_streams[i].is_valid()) {
				int bar_beats = audio_streams[i]->get_bar_beats();
				if (bar_beats != 0) {
					return bar_beats;
				}
			}
		}
		return 0;
	}
	return AudioStream::get_bar_beats();
}

bool AudioStreamInteractive::has_loop() const
{
	switch (mode) {
	case MODE_PLAYLIST:
		return loop;
	case MODE_SYNCHRONIZED: {
		int count = MIN(stream_count, (int)audio_streams.size());
		for (int i = 0; i < count; i++) {
			if (audio_streams[i].is_valid() && audio_streams[i]->has_loop()) {
				return true;
			}
		}
		return false;
	}
	case MODE_CLIPS:
	default:
		return false;
	}
}

double AudioStreamInteractive::get_length() const
{
	switch (mode) {
	case MODE_PLAYLIST: {
		double total_length = 0.0;
		int count = MIN(stream_count, (int)audio_streams.size());
		for (int i = 0; i < count; i++) {
			if (audio_streams[i].is_valid()) {
				double bpm = audio_streams[i]->get_bpm();
				int beat_count = audio_streams[i]->get_beat_count();
				if (bpm > 0.0 && beat_count > 0) {
					total_length += beat_count * 60.0 / bpm;
				}
				else {
					total_length += audio_streams[i]->get_length();
				}
			}
		}
		return total_length;
	}
	case MODE_SYNCHRONIZED: {
		double max_length = 0.0;
		int count = MIN(stream_count, (int)audio_streams.size());
		for (int i = 0; i < count; i++) {
			if (audio_streams[i].is_valid()) {
				max_length = MAX(max_length, audio_streams[i]->get_length());
			}
		}
		return max_length;
	}
	case MODE_CLIPS:
	default:
		return 0.0;
	}
}

#ifdef TOOLS_ENABLED

String AudioStreamInteractive::_get_streams_hint() const
{
	if (!stream_name_cache.is_empty()) {
		return stream_name_cache;
	}

	int count = MIN(clip_count, (int)clips.size());
	for (int i = 0; i < count; i++) {
		if (i > 0) {
			stream_name_cache += ",";
		}
		String n = String(clips[i].name).replace_char(',', ' ');

		if (n == "" && clips[i].stream.is_valid()) {
			if (!clips[i].stream->get_name().is_empty()) {
				n = clips[i].stream->get_name().replace_char(',', ' ');
			}
			else if (clips[i].stream->get_path().is_resource_file()) {
				n = clips[i].stream->get_path().get_file().replace_char(',', ' ');
			}
		}

		if (n == "") {
			n = "Clip " + itos(i);
		}

		stream_name_cache += n;
	}

	return stream_name_cache;
}

#endif

///////////////////////////////////////
// AudioStreamPlaybackInteractive

AudioStreamPlaybackInteractive::~AudioStreamPlaybackInteractive()
{
	if (stream.is_valid()) {
		stream->playbacks.erase(this);
	}
}

void AudioStreamPlaybackInteractive::stop()
{
	active = false;

	for (uint32_t i = 0; i < clips.states.size(); i++) {
		if (clips.states[i].playback.is_valid()) {
			clips.states[i].playback->stop();
		}
		clips.states[i].fade_speed = 0.0;
		clips.states[i].fade_volume = 0.0;
		clips.states[i].fade_wait = 0.0;
		clips.states[i].reset_fade();
		clips.states[i].active = false;
		clips.states[i].auto_advance = -1;
		clips.states[i].first_mix = true;
	}
	clips.playback_current = -1;

	for (uint32_t i = 0; i < playlist.playback.size(); i++) {
		if (playlist.playback[i].is_valid()) {
			playlist.playback[i]->stop();
		}
	}

	for (uint32_t i = 0; i < sync.playback.size(); i++) {
		if (sync.playback[i].is_valid()) {
			sync.playback[i]->stop();
		}
	}
}

void AudioStreamPlaybackInteractive::_update_order()
{
	int count = stream->stream_count;
	playlist.play_order.resize(count);
	for (int i = 0; i < count; i++) {
		playlist.play_order[i] = i;
	}

	if (stream->shuffle) {
		for (int i = 0; i < count; i++) {
			int swap_with = Math::rand() % uint32_t(count);
			SWAP(playlist.play_order[i], playlist.play_order[swap_with]);
		}
	}
}

void AudioStreamPlaybackInteractive::_update_playback_instances()
{
	stop();

	if (stream.is_null()) {
		return;
	}

	int count = stream->stream_count;
	if (stream->mode == AudioStreamInteractive::MODE_PLAYLIST) {
		playlist.playback.resize(count);
		for (int i = 0; i < count; i++) {
			if ((uint32_t)i < stream->audio_streams.size() && stream->audio_streams[i].is_valid()) {
				playlist.playback[i] = stream->audio_streams[i]->instantiate_playback();
			}
			else {
				playlist.playback[i].unref();
			}
		}
	}
	else if (stream->mode == AudioStreamInteractive::MODE_SYNCHRONIZED) {
		sync.playback.resize(count);
		for (int i = 0; i < count; i++) {
			if ((uint32_t)i < stream->audio_streams.size() && stream->audio_streams[i].is_valid()) {
				sync.playback[i] = stream->audio_streams[i]->instantiate_playback();
			}
			else {
				sync.playback[i].unref();
			}
		}
	}
}

void AudioStreamPlaybackInteractive::start(double p_from_pos)
{
	if (stream.is_null()) {
		return;
	}

	if (active) {
		stop();
	}

	switch (stream->mode) {
	case AudioStreamInteractive::MODE_CLIPS: {
		if (clips.states.size() < AudioStreamInteractive::MAX_CLIPS) {
			clips.states.resize(AudioStreamInteractive::MAX_CLIPS);
		}
		if (clips.version != stream->version) {
			for (int i = 0; i < AudioStreamInteractive::MAX_CLIPS; i++) {
				Ref<AudioStream> src_stream;
				if (i < stream->clip_count && (uint32_t)i < stream->clips.size()) {
					src_stream = stream->clips[i].stream;
				}
				if (clips.states[i].stream != src_stream) {
					clips.states[i].stream.unref();
					clips.states[i].playback.unref();

					clips.states[i].stream = src_stream;
					if (src_stream.is_valid()) {
						clips.states[i].playback = src_stream->instantiate_playback();
					}
				}
			}

			clips.version = stream->version;
		}

		int current = stream->initial_clip;
		if (current < 0 || current >= stream->clip_count) {
			return; // No playback possible.
		}
		if (clips.states[current].playback.is_null()) {
			return; // No playback possible.
		}
		active = true;

		_queue(current, false);
	} break;

	case AudioStreamInteractive::MODE_PLAYLIST: {
		if ((int)playlist.playback.size() != stream->stream_count) {
			_update_playback_instances();
		}

		p_from_pos = MAX(0.0, p_from_pos);

		float pl_length = stream->get_length();
		if (p_from_pos >= pl_length) {
			if (!stream->loop || pl_length <= 0.0f) {
				return; // No loop, end.
			}
			p_from_pos = Math::fmod((float)p_from_pos, pl_length);
		}

		_update_order();

		playlist.play_index = -1;

		double play_ofs = p_from_pos;
		for (int i = 0; i < stream->stream_count; i++) {
			int idx = playlist.play_order[i];
			if ((uint32_t)idx < stream->audio_streams.size() &&
				stream->audio_streams[idx].is_valid()) {
				double bpm = stream->audio_streams[idx]->get_bpm();
				int beat_count = stream->audio_streams[idx]->get_beat_count();
				double length;
				if (bpm > 0.0 && beat_count > 0) {
					length = beat_count * 60.0 / bpm;
				}
				else {
					length = stream->audio_streams[idx]->get_length();
				}
				if (play_ofs < length) {
					playlist.play_index = i;
					playlist.stream_todo = length - play_ofs;
					break;
				}
				else {
					play_ofs -= length;
				}
			}
		}

		if (playlist.play_index == -1) {
			return;
		}

		int first_idx = playlist.play_order[playlist.play_index];
		if ((uint32_t)first_idx >= playlist.playback.size() ||
			playlist.playback[first_idx].is_null()) {
			return;
		}

		playlist.playback[first_idx]->start(play_ofs);
		playlist.fade_index = -1;
		playlist.loop_count = 0;
		playlist.offset = p_from_pos;

		active = true;
	} break;

	case AudioStreamInteractive::MODE_SYNCHRONIZED: {
		if ((int)sync.playback.size() != stream->stream_count) {
			_update_playback_instances();
		}

		for (int i = 0; i < stream->stream_count; i++) {
			if ((uint32_t)i < sync.playback.size() && sync.playback[i].is_valid()) {
				sync.playback[i]->start(p_from_pos);
				active = true;
			}
		}
	} break;
	}
}

void AudioStreamPlaybackInteractive::_queue(int p_to_clip_index, bool p_is_auto_advance)
{
	ERR_FAIL_INDEX(p_to_clip_index, stream->clip_count);
	ERR_FAIL_COND(clips.states[p_to_clip_index].playback.is_null());

	if (clips.playback_current == -1) {
		// Nothing to do, start.
		int current = p_to_clip_index;
		ClipState& state = clips.states[current];
		state.active = true;
		state.fade_wait = 0;
		state.fade_volume = 1.0;
		state.fade_speed = 0;
		state.first_mix = true;

		state.playback->start(0);

		clips.playback_current = current;

		if (stream->clips[current].auto_advance == AudioStreamInteractive::AUTO_ADVANCE_ENABLED &&
			stream->clips[current].auto_advance_next_clip >= 0 &&
			stream->clips[current].auto_advance_next_clip < stream->clip_count &&
			stream->clips[current].auto_advance_next_clip != current) {
			// prepare auto advance
			state.auto_advance = stream->clips[current].auto_advance_next_clip;
		}
		return;
	}

	for (int i = 0; i < stream->clip_count; i++) {
		if (i == clips.playback_current || i == p_to_clip_index) {
			continue;
		}
		if (clips.states[i].active &&
			clips.states[i].fade_wait > 0) { // Waiting to kick in, terminate because change of plans.
			clips.states[i].playback->stop();
			clips.states[i].reset_fade();
			clips.states[i].active = false;
		}
	}

	ClipState& from_state = clips.states[clips.playback_current];
	ClipState& to_state = clips.states[p_to_clip_index];

	AudioStreamInteractive::Transition transition; // Use an empty transition by default

	AudioStreamInteractive::TransitionKey tkeys[4] = {
		AudioStreamInteractive::TransitionKey(clips.playback_current, p_to_clip_index),
		AudioStreamInteractive::TransitionKey(
			clips.playback_current, AudioStreamInteractive::CLIP_ANY),
		AudioStreamInteractive::TransitionKey(AudioStreamInteractive::CLIP_ANY, p_to_clip_index),
		AudioStreamInteractive::TransitionKey(
			AudioStreamInteractive::CLIP_ANY, AudioStreamInteractive::CLIP_ANY)};

	for (int i = 0; i < 4; i++) {
		if (stream->transition_map.has(tkeys[i])) {
			transition = stream->transition_map[tkeys[i]];
			break;
		}
	}

	if (transition.fade_mode == AudioStreamInteractive::FADE_AUTOMATIC) {
		// Adjust automatic mode based on context.
		if (transition.to_time == AudioStreamInteractive::TRANSITION_TO_TIME_START) {
			transition.fade_mode = AudioStreamInteractive::FADE_OUT;
		}
		else {
			transition.fade_mode = AudioStreamInteractive::FADE_CROSS;
		}
	}

	if (p_is_auto_advance) {
		transition.from_time = AudioStreamInteractive::TRANSITION_FROM_TIME_END;
		if (transition.to_time == AudioStreamInteractive::TRANSITION_TO_TIME_SAME_POSITION) {
			transition.to_time = AudioStreamInteractive::TRANSITION_TO_TIME_START;
		}
	}

	// Prepare the fadeout
	float current_pos = from_state.playback->get_playback_position();

	float src_fade_wait = 0;
	float dst_seek_to = 0;
	float fade_speed = 0;
	bool src_no_loop = false;

	if (from_state.stream->get_bpm()) {
		// Check if source speed has BPM, if so, transition syncs to BPM
		float beat_sec = 60 / float(from_state.stream->get_bpm());
		switch (transition.from_time) {
		case AudioStreamInteractive::TRANSITION_FROM_TIME_IMMEDIATE: {
			src_fade_wait = 0;
		} break;
		case AudioStreamInteractive::TRANSITION_FROM_TIME_NEXT_BEAT: {
			float remainder = Math::fmod(current_pos, beat_sec);
			src_fade_wait = beat_sec - remainder;
		} break;
		case AudioStreamInteractive::TRANSITION_FROM_TIME_NEXT_BAR: {
			if (from_state.stream->get_bar_beats() > 0) {
				float bar_sec = beat_sec * from_state.stream->get_bar_beats();
				float remainder = Math::fmod(current_pos, bar_sec);
				src_fade_wait = bar_sec - remainder;
			}
			else {
				// Stream does not have a number of beats per bar - avoid NaN, and play immediately.
				src_fade_wait = 0;
			}
		} break;
		case AudioStreamInteractive::TRANSITION_FROM_TIME_END: {
			float end = from_state.stream->get_beat_count() > 0
							? float(from_state.stream->get_beat_count() * beat_sec)
							: from_state.stream->get_length();
			if (end == 0) {
				// Stream does not have a length.
				src_fade_wait = 0;
			}
			else {
				src_fade_wait = end - current_pos;
			}

			if (!from_state.stream->has_loop()) {
				src_no_loop = true;
			}

		} break;
		default: {
		}
		}
		// Fade speed also aligned to BPM
		fade_speed = 1.0 / (transition.fade_beats * beat_sec);
	}
	else {
		// Source has no BPM, so just simple transition.
		if (transition.from_time == AudioStreamInteractive::TRANSITION_FROM_TIME_END &&
			from_state.stream->get_length() > 0) {
			float end = from_state.stream->get_length();
			src_fade_wait = end - current_pos;
			if (!from_state.stream->has_loop()) {
				src_no_loop = true;
			}
		}
		else {
			src_fade_wait = 0;
		}
		fade_speed = 1.0 / transition.fade_beats;
	}

	if (transition.to_time == AudioStreamInteractive::TRANSITION_TO_TIME_PREVIOUS_POSITION &&
		to_state.stream->get_length() > 0.0) {
		dst_seek_to = to_state.previous_position;
	}
	else if (transition.to_time == AudioStreamInteractive::TRANSITION_TO_TIME_SAME_POSITION &&
			 transition.from_time != AudioStreamInteractive::TRANSITION_FROM_TIME_END &&
			 to_state.stream->get_length() > 0.0) {
		// Seeking to basically same position as when we start fading.
		dst_seek_to = current_pos + src_fade_wait;
		float end;
		if (to_state.stream->get_bpm() > 0 && to_state.stream->get_beat_count()) {
			float beat_sec = 60 / float(to_state.stream->get_bpm());
			end = to_state.stream->get_beat_count() * beat_sec;
		}
		else {
			end = to_state.stream->get_length();
		}

		if (dst_seek_to > end) {
			// Seeking too far away.
			dst_seek_to = 0; // past end, loop to beginning.
		}

	}
	else {
		// Seek to Start
		dst_seek_to = 0.0;
	}

	if (transition.fade_mode == AudioStreamInteractive::FADE_DISABLED ||
		transition.fade_mode == AudioStreamInteractive::FADE_IN) {
		if (src_no_loop) {
			// If there is no fade in the source stream, then let it continue until it ends.
			from_state.fade_wait = 0;
			from_state.fade_speed = 0;
		}
		else {
			// Otherwise force a very quick fade to avoid clicks
			from_state.fade_wait = src_fade_wait;
			from_state.fade_speed = 1.0 / -0.001;
		}
	}
	else {
		// Regular fade.
		from_state.fade_wait = src_fade_wait;
		from_state.fade_speed = -fade_speed;
	}
	// keep volume, since it may have been fading in from something else.

	to_state.playback->start(dst_seek_to);
	to_state.active = true;
	to_state.fade_volume = 0.0;
	to_state.first_mix = true;

	int auto_advance_to = -1;

	if (stream->clips[p_to_clip_index].auto_advance ==
		AudioStreamInteractive::AUTO_ADVANCE_ENABLED) {
		int next_clip = stream->clips[p_to_clip_index].auto_advance_next_clip;
		if (next_clip >= 0 && next_clip < (int)stream->clip_count &&
			clips.states[next_clip].playback.is_valid() && next_clip != p_to_clip_index &&
			(!transition.use_filler_clip || next_clip != transition.filler_clip)) {
			auto_advance_to = next_clip;
		}
	}

	if (clips.return_memory != -1 && stream->clips[p_to_clip_index].auto_advance ==
										 AudioStreamInteractive::AUTO_ADVANCE_RETURN_TO_HOLD) {
		auto_advance_to = clips.return_memory;
		clips.return_memory = -1;
	}

	if (transition.hold_previous) {
		clips.return_memory = clips.playback_current;
	}

	if (transition.use_filler_clip && transition.filler_clip >= 0 &&
		transition.filler_clip < (int)stream->clip_count &&
		clips.states[transition.filler_clip].playback.is_valid() &&
		clips.playback_current != transition.filler_clip &&
		p_to_clip_index != transition.filler_clip) {
		ClipState& filler_state = clips.states[transition.filler_clip];

		filler_state.playback->start(0);
		filler_state.active = true;

		// Filler state does not fade (bake fade in the audio clip if you want fading).
		filler_state.fade_volume = 1.0;
		filler_state.fade_speed = 0.0;

		filler_state.fade_wait = src_fade_wait;
		filler_state.first_mix = true;

		float filler_end;
		if (filler_state.stream->get_bpm() > 0 && filler_state.stream->get_beat_count() > 0) {
			float filler_beat_sec = 60 / float(filler_state.stream->get_bpm());
			filler_end = filler_beat_sec * filler_state.stream->get_beat_count();
		}
		else {
			filler_end = filler_state.stream->get_length();
		}

		if (!filler_state.stream->has_loop()) {
			src_no_loop = true;
		}

		if (transition.fade_mode == AudioStreamInteractive::FADE_DISABLED ||
			transition.fade_mode == AudioStreamInteractive::FADE_OUT) {
			// No fading, immediately start at full volume.
			to_state.fade_volume = 0.0;
			to_state.fade_speed = 1.0; // start at full volume, as filler is meant as a transition.
		}
		else {
			// Fade enable, prepare fade.
			to_state.fade_volume = 0.0;
			to_state.fade_speed = fade_speed;
		}

		to_state.fade_wait = src_fade_wait + filler_end;

	}
	else {
		to_state.fade_wait = src_fade_wait;

		if (transition.fade_mode == AudioStreamInteractive::FADE_DISABLED ||
			transition.fade_mode == AudioStreamInteractive::FADE_OUT) {
			to_state.fade_volume = 1.0;
			to_state.fade_speed = 0.0;
		}
		else {
			to_state.fade_volume = 0.0;
			to_state.fade_speed = fade_speed;
		}

		to_state.auto_advance = auto_advance_to;
	}
}

void AudioStreamPlaybackInteractive::seek(double p_time)
{
	if (stream.is_null()) {
		return;
	}

	switch (stream->mode) {
	case AudioStreamInteractive::MODE_PLAYLIST:
		stop();
		start(p_time);
		break;
	case AudioStreamInteractive::MODE_SYNCHRONIZED: {
		int count = MIN(stream->stream_count, (int)sync.playback.size());
		for (int i = 0; i < count; i++) {
			if (sync.playback[i].is_valid()) {
				sync.playback[i]->seek(p_time);
			}
		}
	} break;
	case AudioStreamInteractive::MODE_CLIPS:
	default:
		// Seek not supported in clips mode.
		break;
	}
}

int AudioStreamPlaybackInteractive::mix(AudioFrame* p_buffer, float p_rate_scale, int p_frames)
{
	if (stream.is_null()) {
		return 0;
	}

	switch (stream->mode) {
	case AudioStreamInteractive::MODE_PLAYLIST:
		return _mix_playlist(p_buffer, p_rate_scale, p_frames);
	case AudioStreamInteractive::MODE_SYNCHRONIZED:
		return _mix_sync(p_buffer, p_rate_scale, p_frames);
	case AudioStreamInteractive::MODE_CLIPS:
	default:
		return _mix_clips(p_buffer, p_rate_scale, p_frames);
	}
}

int AudioStreamPlaybackInteractive::_mix_clips(
	AudioFrame* p_buffer, float p_rate_scale, int p_frames)
{
	if (active && clips.version != stream->version) {
		stop();
	}

	if (clips.switch_request != -1) {
		if (active) {
			_queue(clips.switch_request, false);
		}
		clips.switch_request = -1;
	}

	if (!active) {
		return 0;
	}

	int todo = p_frames;

	while (todo) {
		int to_mix = MIN(todo, BUFFER_SIZE);
		_mix_internal(to_mix);
		for (int i = 0; i < to_mix; i++) {
			p_buffer[i] = mix_buffer[i];
		}
		p_buffer += to_mix;
		todo -= to_mix;
	}

	return p_frames;
}

void AudioStreamPlaybackInteractive::_mix_internal(int p_frames)
{
	for (int i = 0; i < p_frames; i++) {
		mix_buffer[i] = AudioFrame(0, 0);
	}

	bool any_active = false;
	int count = MIN(stream->clip_count, (int)clips.states.size());
	for (int i = 0; i < count; i++) {
		if (!clips.states[i].active) {
			continue;
		}

		_mix_internal_state(i, p_frames);

		any_active = clips.states[i].active || any_active;
	}
	if (!any_active) {
		active = false;
		clips.playback_current = -1;
	}
}

void AudioStreamPlaybackInteractive::_mix_internal_state(int p_state_idx, int p_frames)
{
	ClipState& state = clips.states[p_state_idx];
	double mix_rate = double(AudioServer::get_mix_rate());
	double frame_inc = 1.0 / mix_rate;

	int from_frame = 0;
	int queue_next = -1;

	if (state.first_mix) {
		// Did not start mixing yet, wait.
		double mix_time = p_frames * frame_inc;
		if (state.fade_wait < mix_time) {
			// time to start!
			from_frame = state.fade_wait * mix_rate;
			state.fade_wait = 0;
			if (state.fade_speed == 0.0) {
				queue_next = state.auto_advance;
			}
			clips.playback_current = p_state_idx;
			state.first_mix = false;
		}
		else {
			// This is for fade in of new stream.
			state.fade_wait -= mix_time;
			return; // Nothing to do
		}
	}

	state.previous_position = state.playback->get_playback_position();
	state.playback->mix(temp_buffer + from_frame, 1.0, p_frames - from_frame);

	double frame_fade_inc = state.fade_speed * frame_inc;
	for (int i = from_frame; i < p_frames; i++) {
		if (state.fade_wait) {
			// This is for fade out of existing stream;
			state.fade_wait -= frame_inc;
			if (state.fade_wait < 0.0) {
				state.fade_wait = 0.0;
			}
		}
		else if (frame_fade_inc > 0) {
			state.fade_volume += frame_fade_inc;
			if (state.fade_volume >= 1.0) {
				state.fade_speed = 0.0;
				frame_fade_inc = 0.0;
				state.fade_volume = 1.0;
				queue_next = state.auto_advance;
			}
		}
		else if (frame_fade_inc < 0.0) {
			state.fade_volume += frame_fade_inc;
			if (state.fade_volume <= 0.0) {
				state.fade_speed = 0.0;
				frame_fade_inc = 0.0;
				state.fade_volume = 0.0;
				state.playback->stop(); // Stop playback and break, no point to continue mixing
				break;
			}
		}

		mix_buffer[i] += temp_buffer[i] * state.fade_volume;
		state.previous_position += frame_inc;
	}

	if (!state.playback->is_playing()) {
		// It finished because it either reached end or faded out, so deactivate and continue.
		state.active = false;
	}
	if (queue_next != -1) {
		_queue(queue_next, true);
	}
}

int AudioStreamPlaybackInteractive::_mix_playlist(
	AudioFrame* p_buffer, float p_rate_scale, int p_frames)
{
	if (!active) {
		return 0;
	}

	double time_dec = (1.0 / AudioServer::get_mix_rate());
	double fade_dec = (1.0 / stream->fade_time) / AudioServer::get_mix_rate();

	int todo = p_frames;

	while (todo) {
		int to_mix = MIN(todo, MIX_BUFFER_SIZE);

		playlist.playback[playlist.play_order[playlist.play_index]]->mix(mix_buffer, 1.0, to_mix);
		if (playlist.fade_index != -1) {
			playlist.playback[playlist.fade_index]->mix(temp_buffer, 1.0, to_mix);
		}

		playlist.offset += time_dec * to_mix;

		for (int i = 0; i < to_mix; i++) {
			*p_buffer = mix_buffer[i];
			playlist.stream_todo -= time_dec;
			if (playlist.stream_todo < 0) {
				// find next stream.
				int prev = playlist.play_order[playlist.play_index];

				for (int j = 0; j < stream->stream_count; j++) {
					playlist.play_index++;
					if (playlist.play_index >= stream->stream_count) {
						// No loop, exit.
						if (!stream->loop) {
							for (int k = i; k < todo - i; k++) {
								p_buffer[k] = AudioFrame(0, 0);
							}
							todo = to_mix;
							active = false;
							break;
						}

						_update_order();
						playlist.play_index = 0;
						playlist.loop_count++;
						playlist.offset = time_dec * (to_mix - i);
					}

					if (playlist.playback[playlist.play_order[playlist.play_index]].is_valid()) {
						break;
					}
				}

				if (!active) {
					break;
				}

				if (playlist.playback[playlist.play_order[playlist.play_index]].is_null()) {
					todo = to_mix; // Weird error.
					active = false;
					break;
				}

				bool restart = true;
				if (prev == playlist.play_order[playlist.play_index]) {
					// Went back to the same one, continue loop (if it loops) or restart if it does
					// not.
					if (stream->audio_streams[prev]->has_loop()) {
						restart = false;
					}
					playlist.fade_index = -1;
				}
				else {
					// Move current mixed data to fade buffer (temp_buffer).
					for (int j = i; j < to_mix; j++) {
						temp_buffer[j] = mix_buffer[j];
					}

					playlist.fade_index = prev;
					playlist.fade_volume = 1.0;
				}

				int idx = playlist.play_order[playlist.play_index];

				if (restart) {
					playlist.playback[idx]->start(0); // No loop, just cold-restart.
					playlist.playback[idx]->mix(
						mix_buffer + i, 1.0, to_mix - i); // Fill rest of mix buffer
				}

				// Update fade todo.
				double bpm = stream->audio_streams[idx]->get_bpm();
				int beat_count = stream->audio_streams[idx]->get_beat_count();

				if (bpm > 0.0 && beat_count > 0) {
					playlist.stream_todo = beat_count * 60.0 / bpm;
				}
				else {
					playlist.stream_todo = stream->audio_streams[idx]->get_length();
				}
			}

			if (playlist.fade_index != -1) {
				*p_buffer += temp_buffer[i] * playlist.fade_volume;
				playlist.fade_volume -= fade_dec;
				if (playlist.fade_volume <= 0.0) {
					playlist.playback[playlist.fade_index]->stop();
					playlist.fade_index = -1;
				}
			}

			p_buffer++;
		}

		todo -= to_mix;
	}

	return p_frames;
}

int AudioStreamPlaybackInteractive::_mix_sync(
	AudioFrame* p_buffer, float p_rate_scale, int p_frames)
{
	if (!active) {
		return 0;
	}

	int todo = p_frames;
	int count = MIN(stream->stream_count, (int)sync.playback.size());

	bool any_active = false;
	while (todo) {
		int to_mix = MIN(todo, MIX_BUFFER_SIZE);

		bool first = true;
		for (int i = 0; i < count; i++) {
			if (sync.playback[i].is_valid() && sync.playback[i]->is_playing()) {
				float db = (uint32_t)i < stream->audio_stream_volume_db.size()
							   ? stream->audio_stream_volume_db[i]
							   : 0.0f;
				float volume = Math::db_to_linear(db);
				if (first) {
					sync.playback[i]->mix(p_buffer, p_rate_scale, to_mix);
					for (int j = 0; j < to_mix; j++) {
						p_buffer[j] *= volume;
					}
					first = false;
					any_active = true;
				}
				else {
					sync.playback[i]->mix(mix_buffer, p_rate_scale, to_mix);
					for (int j = 0; j < to_mix; j++) {
						p_buffer[j] += mix_buffer[j] * volume;
					}
				}
			}
		}

		if (first) {
			// Nothing mixed, put zeroes.
			for (int j = 0; j < to_mix; j++) {
				p_buffer[j] = AudioFrame(0, 0);
			}
		}

		p_buffer += to_mix;
		todo -= to_mix;
	}

	if (!any_active) {
		active = false;
	}
	return p_frames;
}

void AudioStreamPlaybackInteractive::tag_used_streams()
{
	if (stream.is_null()) {
		return;
	}

	switch (stream->mode) {
	case AudioStreamInteractive::MODE_PLAYLIST:
		if (active && (uint32_t)playlist.play_index < playlist.play_order.size()) {
			int idx = playlist.play_order[playlist.play_index];
			if ((uint32_t)idx < stream->audio_streams.size() &&
				stream->audio_streams[idx].is_valid() && (uint32_t)idx < playlist.playback.size() &&
				playlist.playback[idx].is_valid()) {
				stream->audio_streams[idx]->tag_used(
					playlist.playback[idx]->get_playback_position());
			}
		}
		stream->tag_used(0);
		break;

	case AudioStreamInteractive::MODE_SYNCHRONIZED:
		if (active) {
			int count = MIN(stream->stream_count, (int)sync.playback.size());
			for (int i = 0; i < count; i++) {
				if (sync.playback[i].is_valid() && sync.playback[i]->is_playing() &&
					(uint32_t)i < stream->audio_streams.size() &&
					stream->audio_streams[i].is_valid()) {
					stream->audio_streams[i]->tag_used(sync.playback[i]->get_playback_position());
				}
			}
			stream->tag_used(0);
		}
		break;

	case AudioStreamInteractive::MODE_CLIPS:
	default: {
		int count = MIN(stream->clip_count, (int)clips.states.size());
		for (int i = 0; i < count; i++) {
			if (clips.states[i].active && !clips.states[i].first_mix &&
				clips.states[i].playback->is_playing()) {
				clips.states[i].stream->tag_used(
					clips.states[i].playback->get_playback_position());
			}
		}
		stream->tag_used(0);
	} break;
	}
}

void AudioStreamPlaybackInteractive::switch_to_clip_by_name(const String& p_name)
{
	if (p_name == String()) {
		clips.switch_request = -1;
		return;
	}

	ERR_FAIL_COND_MSG(stream.is_null(), "Attempted to switch while not playing back any stream.");

	for (int i = 0; i < stream->get_clip_count(); i++) {
		if (stream->get_clip_name(i) == p_name) {
			clips.switch_request = i;
			return;
		}
	}
	ERR_FAIL_MSG("Clip not found: " + String(p_name));
}

void AudioStreamPlaybackInteractive::switch_to_clip(int p_index)
{
	clips.switch_request = p_index;
}

int AudioStreamPlaybackInteractive::get_current_clip_index() const
{
	return clips.playback_current;
}

int AudioStreamPlaybackInteractive::get_loop_count() const
{
	if (stream.is_null()) {
		return 0;
	}

	switch (stream->mode) {
	case AudioStreamInteractive::MODE_PLAYLIST:
		return playlist.loop_count;
	case AudioStreamInteractive::MODE_SYNCHRONIZED: {
		int min_loops = 0;
		bool min_loops_found = false;
		int count = MIN(stream->stream_count, (int)sync.playback.size());
		for (int i = 0; i < count; i++) {
			if (sync.playback[i].is_valid() && sync.playback[i]->is_playing()) {
				int loops = sync.playback[i]->get_loop_count();
				if (!min_loops_found || loops < min_loops) {
					min_loops = loops;
					min_loops_found = true;
				}
			}
		}
		return min_loops;
	}
	case AudioStreamInteractive::MODE_CLIPS:
	default:
		return 0;
	}
}

double AudioStreamPlaybackInteractive::get_playback_position() const
{
	if (stream.is_null()) {
		return 0.0;
	}

	switch (stream->mode) {
	case AudioStreamInteractive::MODE_PLAYLIST:
		return playlist.offset;
	case AudioStreamInteractive::MODE_SYNCHRONIZED: {
		float max_pos = 0;
		bool pos_found = false;
		int count = MIN(stream->stream_count, (int)sync.playback.size());
		for (int i = 0; i < count; i++) {
			if (sync.playback[i].is_valid() && sync.playback[i]->is_playing()) {
				float pos = sync.playback[i]->get_playback_position();
				if (!pos_found || pos > max_pos) {
					max_pos = pos;
					pos_found = true;
				}
			}
		}
		return max_pos;
	}
	case AudioStreamInteractive::MODE_CLIPS:
	default:
		return 0.0;
	}
}

bool AudioStreamPlaybackInteractive::is_playing() const { return active; }
