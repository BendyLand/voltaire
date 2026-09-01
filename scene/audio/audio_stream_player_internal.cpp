/**************************************************************************/
/*  audio_stream_player_internal.cpp                                      */
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

#include "audio_stream_player_internal.h"
#include "core/config/engine.h"
#include "scene/main/node.h"
#include "scene/main/scene_tree.h"
#include "servers/audio/audio_stream.h"

void AudioStreamPlayerInternal::_set_process(bool p_enabled)
{
	if (physical) {
		node->set_physics_process_internal(p_enabled);
	}
	else {
		node->set_process_internal(p_enabled);
	}
}

void AudioStreamPlayerInternal::ensure_playback_limit()
{
	while (stream_playbacks.size() > max_polyphony) {
		AudioServer::get_singleton()->stop_playback_stream(stream_playbacks[0]);
		stream_playbacks.remove_at(0);
	}
}

void AudioStreamPlayerInternal::set_stream_paused(bool p_pause)
{
	// TODO this does not have perfect recall, fix that maybe? If there are zero playbacks
	// registered with the AudioServer, this bool isn't persisted.
	for (Ref<AudioStreamPlayback>& playback : stream_playbacks) {
		AudioServer::get_singleton()->set_playback_paused(playback, p_pause);
		if (_is_sample() && playback->get_sample_playback().is_valid()) {
			AudioServer::get_singleton()->set_sample_playback_pause(
				playback->get_sample_playback(), p_pause);
		}
	}
}

bool AudioStreamPlayerInternal::get_stream_paused() const
{
	// There's currently no way to pause some playback streams but not others. Check the first and
	// don't bother looking at the rest.
	if (!stream_playbacks.is_empty()) {
		return AudioServer::get_singleton()->is_playback_paused(stream_playbacks[0]);
	}
	return false;
}

void AudioStreamPlayerInternal::stop_basic()
{
	for (Ref<AudioStreamPlayback>& playback : stream_playbacks) {
		AudioServer::get_singleton()->stop_playback_stream(playback);
	}
	stream_playbacks.clear();

	active.clear();
	_set_process(false);
}

bool AudioStreamPlayerInternal::is_playing() const
{
	for (const Ref<AudioStreamPlayback>& playback : stream_playbacks) {
		if (AudioServer::get_singleton()->is_playback_active(playback)) {
			return true;
		}
	}
	return false;
}

float AudioStreamPlayerInternal::get_playback_position()
{
	// Return the playback position of the most recently started playback stream.
	if (!stream_playbacks.is_empty()) {
		return AudioServer::get_singleton()->get_playback_position(
			stream_playbacks[stream_playbacks.size() - 1]);
	}
	return 0;
}

bool AudioStreamPlayerInternal::is_active() const { return active.is_set(); }

void AudioStreamPlayerInternal::set_pitch_scale(float p_pitch_scale)
{
	ERR_FAIL_COND(p_pitch_scale <= 0.0);
	pitch_scale = p_pitch_scale;

	for (Ref<AudioStreamPlayback>& playback : stream_playbacks) {
		AudioServer::get_singleton()->set_playback_pitch_scale(playback, pitch_scale);
	}
}

void AudioStreamPlayerInternal::set_max_polyphony(int p_max_polyphony)
{
	if (p_max_polyphony > 0) {
		max_polyphony = p_max_polyphony;
	}
}

bool AudioStreamPlayerInternal::has_stream_playback() { return !stream_playbacks.is_empty(); }

Ref<AudioStreamPlayback> AudioStreamPlayerInternal::get_stream_playback()
{
	ERR_FAIL_COND_V_MSG(stream_playbacks.is_empty(), Ref<AudioStreamPlayback>(),
		"Player is inactive. Call play() before requesting get_stream_playback().");
	return stream_playbacks[stream_playbacks.size() - 1];
}

void AudioStreamPlayerInternal::set_playback_type(AudioServer::PlaybackType p_playback_type)
{
	playback_type = p_playback_type;
}

AudioServer::PlaybackType AudioStreamPlayerInternal::get_playback_type() const
{
	return playback_type;
}

StringName AudioStreamPlayerInternal::get_bus() const
{
	const String bus_name = bus;
	for (int i = 0; i < AudioServer::get_singleton()->get_bus_count(); i++) {
		if (AudioServer::get_singleton()->get_bus_name(i) == bus_name) {
			return bus;
		}
	}
	return SceneStringName(Master);
}


