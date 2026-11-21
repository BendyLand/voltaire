/**************************************************************************/
/*  audio_server.cpp                                                      */
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

#include "audio_server.h"
#include "core/config/project_settings.h"
#include "core/error/error_macros.h"
#include "core/io/resource_loader.h"
#include "core/math/audio_frame.h"
#include "core/os/os.h"
#include "core/string/ustring.h"
#include "core/templates/pair.h"
#include "scene/scene_string_names.h"
#include "servers/audio/audio_server.h"
#include "servers/audio/audio_stream.h"

#ifdef TOOLS_ENABLED
#define MARK_EDITED this->obj->set_edited(true);
#else
#define MARK_EDITED
#endif

#ifdef DEBUG_ENABLED
void AudioDriver::start_counting_ticks() { prof_ticks.set(OS::get_ticks_usec()); }

void AudioDriver::stop_counting_ticks()
{
	prof_time.add(OS::get_ticks_usec() - prof_ticks.get());
}
#endif // DEBUG_ENABLED

void AudioDriver::audio_server_process(int p_frames, int32_t* p_buffer, bool p_update_mix_time)
{
	if (p_update_mix_time) {
		update_mix_time(p_frames);
	}

	if (AudioServer::is_initialized()) {
		AudioServer::_driver_process(p_frames, p_buffer);
	}
}

void AudioDriver::update_mix_time(int p_frames)
{
	_last_mix_frames = p_frames;
	if (OS::is_initialized()) {
		_last_mix_time = OS::get_ticks_usec();
	}
}

double AudioDriver::get_time_since_last_mix()
{
	lock();
	uint64_t last_mix_time = _last_mix_time;
	unlock();
	return (OS::get_ticks_usec() - last_mix_time) / 1000000.0;
}

double AudioDriver::get_time_to_next_mix()
{
	lock();
	uint64_t last_mix_time = _last_mix_time;
	uint64_t last_mix_frames = _last_mix_frames;
	unlock();
	double total = (OS::get_ticks_usec() - last_mix_time) / 1000000.0;
	double mix_buffer = last_mix_frames / (double)get_mix_rate();
	return mix_buffer - total;
}

void AudioDriver::input_buffer_init(int driver_buffer_frames)
{
	const int input_buffer_channels = 2;
	input_buffer.resize(driver_buffer_frames * input_buffer_channels * 4);
	input_position = 0;
	input_size = 0;
}

void AudioDriver::input_buffer_write(int32_t sample)
{
	if ((int)input_position < input_buffer.size()) {
		input_buffer.write[input_position++] = sample;
		if ((int)input_position >= input_buffer.size()) {
			input_position = 0;
		}
		if ((int)input_size < input_buffer.size()) {
			input_size++;
		}
	}
	else {
		// This protection was added in GH-26505 due to a "possible crash".
		// This cannot have happened unless two non-locked threads entered function simultaneously,
		// which was possible when multiple calls to `AudioDriver::input_start()` did not raise an
		// error condition.
		WARN_PRINT("input_buffer_write: Invalid input_position=" + itos(input_position) +
				   " input_buffer.size()=" + itos(input_buffer.size()));
	}
}

AudioDriver::SpeakerMode AudioDriver::get_speaker_mode_by_total_channels(int p_channels) const
{
	switch (p_channels) {
	case 4:
		return SPEAKER_SURROUND_31;
	case 6:
		return SPEAKER_SURROUND_51;
	case 8:
		return SPEAKER_SURROUND_71;
	}

	// Default to STEREO
	return SPEAKER_MODE_STEREO;
}

int AudioDriver::get_total_channels_by_speaker_mode(AudioDriver::SpeakerMode p_mode) const
{
	switch (p_mode) {
	case SPEAKER_MODE_STEREO:
		return 2;
	case SPEAKER_SURROUND_31:
		return 4;
	case SPEAKER_SURROUND_51:
		return 6;
	case SPEAKER_SURROUND_71:
		return 8;
	}

	ERR_FAIL_V(2);
}

PackedStringArray AudioDriver::get_output_device_list()
{
	PackedStringArray list;

	list.push_back("Default");

	return list;
}

String AudioDriver::get_output_device() { return "Default"; }

PackedStringArray AudioDriver::get_input_device_list()
{
	PackedStringArray list;

	list.push_back("Default");

	return list;
}

AudioDriverDummy AudioDriverManager::dummy_driver;
AudioDriver* AudioDriverManager::drivers[MAX_DRIVERS] = {
	&AudioDriverManager::dummy_driver,
};
int AudioDriverManager::driver_count = 1;
AudioDriver* AudioDriverManager::active_driver = &AudioDriverManager::dummy_driver;

void AudioDriverManager::add_driver(AudioDriver* p_driver)
{
	ERR_FAIL_COND(driver_count >= MAX_DRIVERS);
	drivers[driver_count - 1] = p_driver;

	// Last driver is always our dummy driver
	drivers[driver_count++] = &AudioDriverManager::dummy_driver;
}

int AudioDriverManager::get_driver_count() { return driver_count; }

AudioDriver* AudioDriverManager::get_driver(int p_driver)
{
	ERR_FAIL_INDEX_V(p_driver, driver_count, nullptr);
	return drivers[p_driver];
}

void AudioDriverManager::set_active_driver(AudioDriver* p_driver)
{
	active_driver = p_driver ? p_driver : &dummy_driver;
}

void AudioServer::_driver_process(int p_frames, int32_t* p_buffer)
{
	data->mix_count++;
	int todo = p_frames;

#ifdef DEBUG_ENABLED
	uint64_t prof_ticks = OS::get_ticks_usec();
#endif

	if (data->channel_count != get_channel_count()) {
		// Amount of channels changed due to a output_device change
		// reinitialize the buses channels and buffers
		init_channels_and_buffers();
	}

	ERR_FAIL_COND_MSG(data->buses.is_empty() && todo, "AudioServer bus count is less than 1.");
	while (todo) {
		if (data->to_mix == 0) {
			_mix_step();
		}

		int to_copy = MIN(data->to_mix, todo);

		Bus* master = data->buses[0];

		int from = data->buffer_size - data->to_mix;
		int from_buf = p_frames - todo;

		// master master, send to output
		int cs = master->channels.size();

		// Take away 1 from the stride, as we are manually incrementing by 1 for stereo.
		uintptr_t stride_minus_one = (cs * 2) - 1;

		for (int k = 0; k < cs; k++) {
			// The destination start for data will be the same in all cases.
			int32_t* dest = &p_buffer[from_buf * (cs * 2) + (k * 2)];

#ifdef DEBUG_ENABLED
			if (!data->debug_mute && master->channels[k].active) {
#else
			if (master->channels[k].active) {
#endif // DEBUG_ENABLED
				const AudioFrame* buf = master->channels[k].buffer.ptr();

				for (int j = 0; j < to_copy; j++) {
					float l = CLAMP(buf[from + j].left, -1.0, 1.0);
					int32_t vl = l * ((1 << 20) - 1);
					int32_t vl2 = (vl < 0 ? -1 : 1) * (Math::abs(vl) << 11);
					*dest = vl2;
					dest++;

					float r = CLAMP(buf[from + j].right, -1.0, 1.0);
					int32_t vr = r * ((1 << 20) - 1);
					int32_t vr2 = (vr < 0 ? -1 : 1) * (Math::abs(vr) << 11);
					*dest = vr2;
					dest += stride_minus_one;
				}

			}
			else {
				// Bizarrely, profiling indicates that detecting the common case of cs == 1,
				// k == 0, and using memset is SLOWER than setting them individually.
				// Perhaps it gets optimized to a faster instruction than memset.
				for (int j = 0; j < to_copy; j++) {
					*dest = 0;
					dest++;
					*dest = 0;
					dest += stride_minus_one;
				}
			}
		}

		todo -= to_copy;
		data->to_mix -= to_copy;
	}

#ifdef DEBUG_ENABLED
	data->prof_time.add(OS::get_ticks_usec() - prof_ticks);
#endif
}

void AudioServer::_mix_step()
{
	bool solo_mode = false;

	for (int i = 0; i < data->buses.size(); i++) {
		Bus* bus = data->buses[i];
		bus->index_cache = i; // might be moved around by editor, so..
		for (int k = 0; k < bus->channels.size(); k++) {
			bus->channels.write[k].used = false;
		}

		if (bus->solo) {
			// solo chain
			solo_mode = true;
			bus->soloed = true;
			do {
				if (bus != data->buses[0]) {
					// everything has a send save for master bus
					if (!data->bus_map.has(bus->send)) {
						bus = data->buses[0]; // send to master
					}
					else {
						int prev_index_cache = bus->index_cache;
						bus = data->bus_map[bus->send];
						if (prev_index_cache >= bus->index_cache) { // invalid, send to master
							bus = data->buses[0];
						}
					}

					bus->soloed = true;
				}
				else {
					bus = nullptr;
				}

			} while (bus);
		}
		else {
			bus->soloed = false;
		}
	}
	// This is legacy code from 3.x that allows video players and other audio sources that do not
	// implement AudioStreamPlayback to output audio.
	for (CallbackItem* ci : data->mix_callback_list) {
		ci->callback(ci->userdata);
	}

	// Main mixing loop for audio streams.
	// The basic idea here is to copy the samples returned by the AudioStreamPlayback's mix function
	// into the audio buffers,
	//  while always maintaining a lookahead buffer of size LOOKAHEAD_BUFFER_SIZE to allow fade-outs
	//  for sudden stoppages.
	for (AudioStreamPlaybackListNode* playback : data->playback_list) {
		// Paused streams are no-ops. Don't even mix audio from the stream playback.
		if (playback->state.load() == AudioStreamPlaybackListNode::PAUSED) {
			continue;
		}

		if (playback->stream_playback->get_is_sample()) {
			continue;
		}

		// If `fading_out` is true, we're in the process of fading out the stream playback.
		// TODO: Currently this sets the volume of the stream to 0 which creates a linear
		// interpolation between its previous volume and silence.
		//  A more punchy option for fading out could be to just use the lookahead buffer.
		bool fading_out =
			playback->state.load() == AudioStreamPlaybackListNode::FADE_OUT_TO_DELETION ||
			playback->state.load() == AudioStreamPlaybackListNode::FADE_OUT_TO_PAUSE;

		AudioFrame* buf = data->mix_buffer.ptrw();

		// Copy the old contents of the lookahead buffer into the beginning of the mix buffer.
		for (int i = 0; i < LOOKAHEAD_BUFFER_SIZE; i++) {
			buf[i] = playback->lookahead[i];
		}

		// Mix the audio stream.
		unsigned int mixed_frames = playback->stream_playback->mix(
			&buf[LOOKAHEAD_BUFFER_SIZE], playback->pitch_scale.get(), data->buffer_size);

		if (data->tag_used_audio_streams && playback->stream_playback->is_playing()) {
			playback->stream_playback->tag_used_streams();
		}

		// Check to see if the stream has run out of samples.
		if (mixed_frames != data->buffer_size) {
			// We know we have at least the size of our lookahead buffer for fade-out purposes.

			float fadeout_base = 0.94;
			float fadeout_coefficient = 1;
			static_assert(LOOKAHEAD_BUFFER_SIZE == 64,
				"Update fadeout_base and comment here if you change LOOKAHEAD_BUFFER_SIZE.");
			// 0.94 ^ 64 = 0.01906. There might still be a pop but it'll be way better than if we
			// didn't do this.
			for (unsigned int idx = mixed_frames; idx < data->buffer_size; idx++) {
				fadeout_coefficient *= fadeout_base;
				buf[idx] *= fadeout_coefficient;
			}
			AudioStreamPlaybackListNode::PlaybackState new_state;
			new_state = AudioStreamPlaybackListNode::AWAITING_DELETION;
			playback->state.store(new_state);
		}
		else {
			// Move the last little bit of what we just mixed into our lookahead buffer for the next
			// call to _mix_step.
			for (int i = 0; i < LOOKAHEAD_BUFFER_SIZE; i++) {
				playback->lookahead[i] = buf[data->buffer_size + i];
			}
		}

		// Get the bus details for this playback. This contains information about which buses the
		// playback is assigned to and the volume of the playback on each bus.
		AudioStreamPlaybackBusDetails* bus_details_ptr = playback->bus_details.load();
		ERR_FAIL_NULL(bus_details_ptr);
		// Make a copy of the bus details so we can modify it without worrying about other threads.
		AudioStreamPlaybackBusDetails bus_details = *bus_details_ptr;

		// Mix to any active buses.
		for (int idx = 0; idx < MAX_BUSES_PER_PLAYBACK; idx++) {
			if (!bus_details.bus_active[idx]) {
				continue;
			}
			// This is the AudioServer-internal index of the bus we're mixing to in this step of the
			// loop. Not to be confused with `idx` which is an index into
			// `AudioStreamPlaybackBusDetails` member var arrays.
			int bus_idx = thread_find_bus_index(bus_details.bus[idx]);

			// It's important to know whether or not this bus was active in the previous mix step of
			// this stream. If it was, we need to perform volume interpolation to avoid pops.
			int prev_bus_idx = -1;
			for (int search_idx = 0; search_idx < MAX_BUSES_PER_PLAYBACK; search_idx++) {
				if (!playback->prev_bus_details->bus_active[search_idx]) {
					continue;
				}
				// If the Strings of the buses match, we've found the previous bus index. This
				// indicates that this playback mixed to `prev_bus_details->bus[prev_bus_index]` in
				// the previous mix step, which gives us a way to look up the playback's previous
				// volume.
				if (playback->prev_bus_details->bus[search_idx].hash() ==
					bus_details.bus[idx].hash()) {
					prev_bus_idx = search_idx;
					break;
				}
			}

			// It's now time to mix to the bus. We do this by going through each channel of the bus
			// and mixing to it.
			//  The channels correspond to output channels of the audio device, e.g. stereo or 5.1.
			//  To reduce needless nesting, this is done with a helper method named
			//  `_mix_step_for_channel`.
			for (int channel_idx = 0; channel_idx < data->channel_count; channel_idx++) {
				AudioFrame* channel_buf = thread_get_channel_mix_buffer(bus_idx, channel_idx);
				// TODO: This `fading_out` check could be replaced with with an exponential fadeout
				// of the samples from the lookahead buffer for more punchy results.
				if (fading_out) {
					bus_details.volume[idx][channel_idx] = AudioFrame(0, 0);
				}
				AudioFrame channel_vol = bus_details.volume[idx][channel_idx];

				// If this bus was not active in the previous mix step, we want to start playback at
				// the full volume to avoid crushing transients.
				AudioFrame prev_channel_vol = channel_vol;
				// If this bus was active in the previous mix step, we need to interpolate between
				// the previous volume and the current volume to avoid pops. Set
				// `prev_channel_volume` accordingly.
				if (prev_bus_idx != -1) {
					prev_channel_vol =
						playback->prev_bus_details->volume[prev_bus_idx][channel_idx];
				}
				_mix_step_for_channel(channel_buf, buf, prev_channel_vol, channel_vol,
					playback->attenuation_filter_cutoff_hz.get(), playback->highshelf_gain.get(),
					&playback->filter_process[channel_idx * 2],
					&playback->filter_process[channel_idx * 2 + 1]);
			}
		}

		// Now go through and fade-out any buses that were being played to previously that we missed
		// by going through current data.
		for (int idx = 0; idx < MAX_BUSES_PER_PLAYBACK; idx++) {
			if (!playback->prev_bus_details->bus_active[idx]) {
				continue;
			}
			int bus_idx = thread_find_bus_index(playback->prev_bus_details->bus[idx]);

			int current_bus_idx = -1;
			for (int search_idx = 0; search_idx < MAX_BUSES_PER_PLAYBACK; search_idx++) {
				if (bus_details.bus[search_idx] == playback->prev_bus_details->bus[idx]) {
					current_bus_idx = search_idx;
				}
			}
			if (current_bus_idx != -1) {
				// If we found a corresponding bus in the current bus assignments, we've already
				// mixed to this bus.
				continue;
			}

			for (int channel_idx = 0; channel_idx < data->channel_count; channel_idx++) {
				AudioFrame* channel_buf = thread_get_channel_mix_buffer(bus_idx, channel_idx);
				AudioFrame prev_channel_vol = playback->prev_bus_details->volume[idx][channel_idx];
				// Fade out to silence. This could be replaced with an exponential fadeout of the
				// samples from the lookahead buffer for more punchy results.
				_mix_step_for_channel(channel_buf, buf, prev_channel_vol, AudioFrame(0, 0),
					playback->attenuation_filter_cutoff_hz.get(), playback->highshelf_gain.get(),
					&playback->filter_process[channel_idx * 2],
					&playback->filter_process[channel_idx * 2 + 1]);
			}
		}

		// Copy the bus details we mixed with to the previous bus details to maintain volume ramps.
		for (int i = 0; i < MAX_BUSES_PER_PLAYBACK; i++) {
			playback->prev_bus_details->bus_active[i] = bus_details.bus_active[i];
		}
		for (int i = 0; i < MAX_BUSES_PER_PLAYBACK; i++) {
			playback->prev_bus_details->bus[i] = bus_details.bus[i];
		}
		for (int i = 0; i < MAX_BUSES_PER_PLAYBACK; i++) {
			for (int j = 0; j < MAX_CHANNELS_PER_BUS; j++) {
				playback->prev_bus_details->volume[i][j] = bus_details.volume[i][j];
			}
		}

		switch (playback->state.load()) {
		case AudioStreamPlaybackListNode::AWAITING_DELETION:
		case AudioStreamPlaybackListNode::FADE_OUT_TO_DELETION:
			// Remove the playback from the list.
			_delete_stream_playback_list_node(playback);
			break;
		case AudioStreamPlaybackListNode::FADE_OUT_TO_PAUSE: {
			// Pause the stream.
			playback->state.store(AudioStreamPlaybackListNode::PAUSED);
		} break;
		case AudioStreamPlaybackListNode::PLAYING:
		case AudioStreamPlaybackListNode::PAUSED:
			// No-op!
			break;
		}
	}

	// Now that all of the buses have their audio sources mixed into them, we can process the
	// effects and bus sends.
	for (int i = data->buses.size() - 1; i >= 0; i--) {
		Bus* bus = data->buses[i];

		for (int k = 0; k < bus->channels.size(); k++) {
			if (bus->channels[k].active && !bus->channels[k].used) {
				// Buffer was not used, but it's still active, so it must be cleaned.
				AudioFrame* buf = bus->channels.write[k].buffer.ptrw();

				for (uint32_t j = 0; j < data->buffer_size; j++) {
					buf[j] = AudioFrame(0, 0);
				}
			}
		}

		// Process effects.
		if (!bus->bypass) {
			for (int j = 0; j < bus->effects.size(); j++) {
				if (!bus->effects[j].enabled) {
					continue;
				}

#ifdef DEBUG_ENABLED
				uint64_t ticks = OS::get_ticks_usec();
#endif

				for (int k = 0; k < bus->channels.size(); k++) {
					if (!(bus->channels[k].active ||
							bus->channels[k].effect_instances[j]->process_silence())) {
						continue;
					}
					bus->channels.write[k].effect_instances.write[j]->process(
						bus->channels[k].buffer.ptr(), data->temp_buffer.write[k].ptrw(), data->buffer_size);
				}

				// Swap buffers, so internal buffer always has the right data.
				for (int k = 0; k < bus->channels.size(); k++) {
					if (!(data->buses[i]->channels[k].active ||
							bus->channels[k].effect_instances[j]->process_silence())) {
						continue;
					}
					SWAP(bus->channels.write[k].buffer, data->temp_buffer.write[k]);
				}

#ifdef DEBUG_ENABLED
				bus->effects.write[j].prof_time += OS::get_ticks_usec() - ticks;
#endif
			}
		}

		// Process send.

		Bus* send = nullptr;

		if (i > 0) {
			// Everything has a send except for the master bus.
			if (!data->bus_map.has(bus->send)) {
				send = data->buses[0];
			}
			else {
				send = data->bus_map[bus->send];
				if (send->index_cache >= bus->index_cache) { // Invalid, send to master.
					send = data->buses[0];
				}
			}
		}

		for (int k = 0; k < bus->channels.size(); k++) {
			if (!bus->channels[k].active) {
				bus->channels.write[k].peak_volume =
					AudioFrame(AUDIO_MIN_PEAK_DB, AUDIO_MIN_PEAK_DB);
				continue;
			}

			AudioFrame* buf = bus->channels.write[k].buffer.ptrw();

			AudioFrame peak = AudioFrame(0, 0);

			float volume = Math::db_to_linear(bus->volume_db);

			if (solo_mode) {
				if (!bus->soloed) {
					volume = 0.0;
				}
			}
			else {
				if (bus->mute) {
					volume = 0.0;
				}
			}

			// Apply volume and compute peak.
			for (uint32_t j = 0; j < data->buffer_size; j++) {
				buf[j] *= volume;

				float l = Math::abs(buf[j].left);
				if (l > peak.left) {
					peak.left = l;
				}
				float r = Math::abs(buf[j].right);
				if (r > peak.right) {
					peak.right = r;
				}
			}

			bus->channels.write[k].peak_volume =
				AudioFrame(Math::linear_to_db(peak.left + AUDIO_PEAK_OFFSET),
					Math::linear_to_db(peak.right + AUDIO_PEAK_OFFSET));

			if (!bus->channels[k].used) {
				// See if any audio is contained, because channel was not used.

				if (MAX(peak.right, peak.left) > Math::db_to_linear(data->channel_disable_threshold_db)) {
					bus->channels.write[k].last_mix_with_audio = data->mix_frames;
				}
				else if (data->mix_frames - bus->channels[k].last_mix_with_audio >
						   data->channel_disable_frames) {
					bus->channels.write[k].active = false;
					continue; // went inactive, don't mix.
				}
			}

			if (send) {
				// If not master bus, send.
				AudioFrame* target_buf = thread_get_channel_mix_buffer(send->index_cache, k);

				for (uint32_t j = 0; j < data->buffer_size; j++) {
					target_buf[j] += buf[j];
				}
			}
		}
	}

	data->mix_frames += data->buffer_size;
	data->to_mix = data->buffer_size;
}

void AudioServer::_mix_step_for_channel(AudioFrame* p_out_buf, AudioFrame* p_source_buf,
	AudioFrame p_vol_start, AudioFrame p_vol_final, float p_attenuation_filter_cutoff_hz,
	float p_highshelf_gain, AudioFilterSW::Processor* p_processor_l,
	AudioFilterSW::Processor* p_processor_r)
{
	// TODO: In the future it could be nice to replace all of these hardcoded effects with something
	// a bit cleaner and more flexible, but for now this is what we do to support 3D audio players.
	if (p_highshelf_gain != 0) {
		AudioFilterSW filter;
		filter.set_mode(AudioFilterSW::HIGHSHELF);
		filter.set_sampling_rate(AudioServer::get_mix_rate());
		filter.set_cutoff(p_attenuation_filter_cutoff_hz);
		filter.set_resonance(1);
		filter.set_stages(1);
		filter.set_gain(p_highshelf_gain);

		ERR_FAIL_NULL(p_processor_l);
		ERR_FAIL_NULL(p_processor_r);

		bool is_just_started = p_vol_start.left == 0 && p_vol_start.right == 0;
		p_processor_l->set_filter(&filter, /* clear_history= */ is_just_started);
		p_processor_l->update_coeffs(data->buffer_size);
		p_processor_r->set_filter(&filter, /* clear_history= */ is_just_started);
		p_processor_r->update_coeffs(data->buffer_size);

		for (unsigned int frame_idx = 0; frame_idx < data->buffer_size; frame_idx++) {
			// TODO: Make lerp speed buffer-size-invariant if buffer_size ever becomes a project
			// setting to avoid very small buffer sizes causing pops due to too-fast lerps.
			float lerp_param = (float)frame_idx / data->buffer_size;
			AudioFrame vol = p_vol_final * lerp_param + (1 - lerp_param) * p_vol_start;
			AudioFrame mixed = vol * p_source_buf[frame_idx];
			p_processor_l->process_one_interp(mixed.left);
			p_processor_r->process_one_interp(mixed.right);
			p_out_buf[frame_idx] += mixed;
		}

	}
	else {
		for (unsigned int frame_idx = 0; frame_idx < data->buffer_size; frame_idx++) {
			// TODO: Make lerp speed buffer-size-invariant if buffer_size ever becomes a project
			// setting to avoid very small buffer sizes causing pops due to too-fast lerps.
			float lerp_param = (float)frame_idx / data->buffer_size;
			p_out_buf[frame_idx] += (p_vol_final * lerp_param + (1 - lerp_param) * p_vol_start) *
									p_source_buf[frame_idx];
		}
	}
}

AudioServer::AudioStreamPlaybackListNode* AudioServer::_find_playback_list_node(
	Ref<AudioStreamPlayback> p_playback)
{
	for (AudioStreamPlaybackListNode* playback_list_node : data->playback_list) {
		if (playback_list_node->stream_playback == p_playback) {
			return playback_list_node;
		}
	}
	return nullptr;
}

void AudioServer::_delete_stream_playback(Ref<AudioStreamPlayback> p_playback)
{
	ERR_FAIL_COND(p_playback.is_null());
	AudioStreamPlaybackListNode* playback_node = _find_playback_list_node(p_playback);
	if (playback_node) {
		_delete_stream_playback_list_node(playback_node);
	}
}

void AudioServer::_delete_stream_playback_list_node(AudioStreamPlaybackListNode* p_playback_node)
{
	// Remove the playback from the list, registering a destructor to be run on the main thread.
	data->playback_list.erase(p_playback_node, [](AudioStreamPlaybackListNode* p) {
		delete p->prev_bus_details;
		delete p->bus_details.load();
		p->stream_playback.unref();
		delete p;
	});
}

bool AudioServer::thread_has_channel_mix_buffer(int p_bus, int p_buffer)
{
	if (p_bus < 0 || p_bus >= data->buses.size()) {
		return false;
	}
	if (p_buffer < 0 || p_buffer >= data->buses[p_bus]->channels.size()) {
		return false;
	}
	return true;
}

AudioFrame* AudioServer::thread_get_channel_mix_buffer(int p_bus, int p_buffer)
{
	ERR_FAIL_INDEX_V(p_bus, data->buses.size(), nullptr);
	ERR_FAIL_INDEX_V(p_buffer, data->buses[p_bus]->channels.size(), nullptr);

	AudioFrame* buf_data = data->buses.write[p_bus]->channels.write[p_buffer].buffer.ptrw();

	if (!data->buses[p_bus]->channels[p_buffer].used) {
		data->buses.write[p_bus]->channels.write[p_buffer].used = true;
		data->buses.write[p_bus]->channels.write[p_buffer].active = true;
		data->buses.write[p_bus]->channels.write[p_buffer].last_mix_with_audio = data->mix_frames;
		for (uint32_t i = 0; i < data->buffer_size; i++) {
			buf_data[i] = AudioFrame(0, 0);
		}
	}

	return buf_data;
}

int AudioServer::thread_get_mix_buffer_size() { return data->buffer_size; }

int AudioServer::thread_find_bus_index(const String& p_name)
{
	if (data->bus_map.has(p_name)) {
		return data->bus_map[p_name]->index_cache;
	}
	else {
		return 0;
	}
}

#ifdef DEBUG_ENABLED
void AudioServer::set_debug_mute(bool p_mute) { data->debug_mute = p_mute; }

bool AudioServer::get_debug_mute() { return data->debug_mute; }
#endif // DEBUG_ENABLED

int AudioServer::get_bus_count() { return data->buses.size(); }

String AudioServer::get_bus_name(int p_bus)
{
	ERR_FAIL_INDEX_V(p_bus, data->buses.size(), String());
	return data->buses[p_bus]->name;
}

int AudioServer::get_bus_index(const String& p_bus_name)
{
	for (int i = 0; i < data->buses.size(); ++i) {
		if (data->buses[i]->name == p_bus_name) {
			return i;
		}
	}
	return -1;
}

float AudioServer::get_bus_volume_db(int p_bus)
{
	ERR_FAIL_INDEX_V(p_bus, data->buses.size(), 0);
	return data->buses[p_bus]->volume_db;
}

void AudioServer::set_bus_volume_linear(int p_bus, float p_volume_linear)
{
	set_bus_volume_db(p_bus, Math::linear_to_db(p_volume_linear));
}

float AudioServer::get_bus_volume_linear(int p_bus)
{
	return Math::db_to_linear(get_bus_volume_db(p_bus));
}

int AudioServer::get_bus_channels(int p_bus)
{
	ERR_FAIL_INDEX_V(p_bus, data->buses.size(), 0);
	return data->buses[p_bus]->channels.size();
}

String AudioServer::get_bus_send(int p_bus)
{
	ERR_FAIL_INDEX_V(p_bus, data->buses.size(), String());
	return data->buses[p_bus]->send;
}

bool AudioServer::is_bus_solo(int p_bus)
{
	ERR_FAIL_INDEX_V(p_bus, data->buses.size(), false);

	return data->buses[p_bus]->solo;
}

bool AudioServer::is_bus_mute(int p_bus)
{
	ERR_FAIL_INDEX_V(p_bus, data->buses.size(), false);

	return data->buses[p_bus]->mute;
}

bool AudioServer::is_bus_bypassing_effects(int p_bus)
{
	ERR_FAIL_INDEX_V(p_bus, data->buses.size(), false);

	return data->buses[p_bus]->bypass;
}

int AudioServer::get_bus_effect_count(int p_bus)
{
	ERR_FAIL_INDEX_V(p_bus, data->buses.size(), 0);

	return data->buses[p_bus]->effects.size();
}

Ref<AudioEffectInstance> AudioServer::get_bus_effect_instance(
	int p_bus, int p_effect, int p_channel)
{
	ERR_FAIL_INDEX_V(p_bus, data->buses.size(), Ref<AudioEffectInstance>());
	ERR_FAIL_INDEX_V(p_effect, data->buses[p_bus]->effects.size(), Ref<AudioEffectInstance>());
	ERR_FAIL_INDEX_V(p_channel, data->buses[p_bus]->channels.size(), Ref<AudioEffectInstance>());

	return data->buses[p_bus]->channels[p_channel].effect_instances[p_effect];
}

Ref<AudioEffect> AudioServer::get_bus_effect(int p_bus, int p_effect)
{
	ERR_FAIL_INDEX_V(p_bus, data->buses.size(), Ref<AudioEffect>());
	ERR_FAIL_INDEX_V(p_effect, data->buses[p_bus]->effects.size(), Ref<AudioEffect>());

	return data->buses[p_bus]->effects[p_effect].effect;
}

bool AudioServer::is_bus_effect_enabled(int p_bus, int p_effect)
{
	ERR_FAIL_INDEX_V(p_bus, data->buses.size(), false);
	ERR_FAIL_INDEX_V(p_effect, data->buses[p_bus]->effects.size(), false);
	return data->buses[p_bus]->effects[p_effect].enabled;
}

float AudioServer::get_bus_peak_volume_left_db(int p_bus, int p_channel)
{
	ERR_FAIL_INDEX_V(p_bus, data->buses.size(), 0);
	ERR_FAIL_INDEX_V(p_channel, data->buses[p_bus]->channels.size(), 0);

	return data->buses[p_bus]->channels[p_channel].peak_volume.left;
}

float AudioServer::get_bus_peak_volume_right_db(int p_bus, int p_channel)
{
	ERR_FAIL_INDEX_V(p_bus, data->buses.size(), 0);
	ERR_FAIL_INDEX_V(p_channel, data->buses[p_bus]->channels.size(), 0);

	return data->buses[p_bus]->channels[p_channel].peak_volume.right;
}

bool AudioServer::is_bus_channel_active(int p_bus, int p_channel)
{
	ERR_FAIL_INDEX_V(p_bus, data->buses.size(), false);
	ERR_FAIL_INDEX_V(p_channel, data->buses[p_bus]->channels.size(), false);

	return data->buses[p_bus]->channels[p_channel].active;
}

void AudioServer::set_playback_speed_scale(float p_scale)
{
	ERR_FAIL_COND(p_scale <= 0);

	data->playback_speed_scale = p_scale;
}

float AudioServer::get_playback_speed_scale() { return data->playback_speed_scale; }

void AudioServer::start_playback_stream(Ref<AudioStreamPlayback> p_playback,
	const String& p_bus, Vector<AudioFrame> p_volume_db_vector, float p_start_time,
	float p_pitch_scale)
{
	ERR_FAIL_COND(p_playback.is_null());

	HashMap<String, Vector<AudioFrame>> map;
	map[p_bus] = p_volume_db_vector;

	start_playback_stream(p_playback, map, p_start_time, p_pitch_scale);
}

void AudioServer::start_playback_stream(Ref<AudioStreamPlayback> p_playback,
	const HashMap<String, Vector<AudioFrame>>& p_bus_volumes, float p_start_time,
	float p_pitch_scale, float p_highshelf_gain, float p_attenuation_cutoff_hz)
{
	ERR_FAIL_COND(p_playback.is_null());

	AudioStreamPlaybackListNode* playback_node = new AudioStreamPlaybackListNode();
	playback_node->stream_playback = p_playback;
	playback_node->stream_playback->start(p_start_time);

	AudioStreamPlaybackBusDetails* new_bus_details = new AudioStreamPlaybackBusDetails();
	int idx = 0;
	for (KeyValue<String, Vector<AudioFrame>> pair : p_bus_volumes) {
		if (pair.value.size() < data->channel_count || pair.value.size() != MAX_CHANNELS_PER_BUS) {
			delete playback_node;
			delete new_bus_details;
			ERR_FAIL();
		}

		new_bus_details->bus_active[idx] = true;
		new_bus_details->bus[idx] = pair.key;
		for (int channel_idx = 0; channel_idx < MAX_CHANNELS_PER_BUS; channel_idx++) {
			new_bus_details->volume[idx][channel_idx] = pair.value[channel_idx];
		}
		idx++;
	}
	playback_node->bus_details.store(new_bus_details);
	playback_node->prev_bus_details = new AudioStreamPlaybackBusDetails();

	playback_node->pitch_scale.set(p_pitch_scale);
	playback_node->highshelf_gain.set(p_highshelf_gain);
	playback_node->attenuation_filter_cutoff_hz.set(p_attenuation_cutoff_hz);

	memset(playback_node->prev_bus_details->volume, 0,
		sizeof(playback_node->prev_bus_details->volume));

	for (AudioFrame& frame : playback_node->lookahead) {
		frame = AudioFrame(0, 0);
	}

	playback_node->state.store(AudioStreamPlaybackListNode::PLAYING);

	data->playback_list.insert(playback_node);
}

void AudioServer::stop_playback_stream(Ref<AudioStreamPlayback> p_playback)
{
	ERR_FAIL_COND(p_playback.is_null());

	// Handle sample playback.
	if (p_playback->get_is_sample()) {
		if (p_playback->get_sample_playback().is_valid()) {
			AudioServer::stop_sample_playback(p_playback->get_sample_playback());
		}
		else {
			_delete_stream_playback(p_playback);
		}
		return;
	}

	if (!p_playback->is_playing()) {
		p_playback->stop();
	}

	AudioStreamPlaybackListNode* playback_node = _find_playback_list_node(p_playback);
	if (!playback_node) {
		return;
	}

	AudioStreamPlaybackListNode::PlaybackState new_state, old_state;
	do {
		old_state = playback_node->state.load();
		if (old_state == AudioStreamPlaybackListNode::AWAITING_DELETION) {
			break; // Don't fade out again.
		}
		new_state = AudioStreamPlaybackListNode::FADE_OUT_TO_DELETION;

	} while (!playback_node->state.compare_exchange_strong(old_state, new_state));
}

void AudioServer::set_playback_bus_exclusive(
	Ref<AudioStreamPlayback> p_playback, const String& p_bus, Vector<AudioFrame> p_volumes)
{
	ERR_FAIL_COND(p_volumes.size() != MAX_CHANNELS_PER_BUS);

	HashMap<String, Vector<AudioFrame>> map;
	map[p_bus] = p_volumes;

	set_playback_bus_volumes_linear(p_playback, map);
}

void AudioServer::set_playback_bus_volumes_linear(Ref<AudioStreamPlayback> p_playback,
	const HashMap<String, Vector<AudioFrame>>& p_bus_volumes)
{
	ERR_FAIL_COND(p_bus_volumes.size() > MAX_BUSES_PER_PLAYBACK);

	// Samples.
	if (p_playback->get_is_sample() && p_playback->get_sample_playback().is_valid()) {
		Ref<AudioSamplePlayback> sample_playback = p_playback->get_sample_playback();
		AudioDriverManager::get_active_driver()->set_sample_playback_bus_volumes_linear(
			sample_playback, p_bus_volumes);
		return;
	}

	AudioStreamPlaybackListNode* playback_node = _find_playback_list_node(p_playback);
	if (!playback_node) {
		return;
	}
	AudioStreamPlaybackBusDetails *old_bus_details,
		*new_bus_details = new AudioStreamPlaybackBusDetails();

	int idx = 0;
	for (KeyValue<String, Vector<AudioFrame>> pair : p_bus_volumes) {
		if (idx >= MAX_BUSES_PER_PLAYBACK) {
			break;
		}
		if (pair.value.size() < data->channel_count || pair.value.size() != MAX_CHANNELS_PER_BUS) {
			delete new_bus_details;
			ERR_FAIL();
		}

		new_bus_details->bus_active[idx] = true;
		new_bus_details->bus[idx] = pair.key;
		for (int channel_idx = 0; channel_idx < MAX_CHANNELS_PER_BUS; channel_idx++) {
			new_bus_details->volume[idx][channel_idx] = pair.value[channel_idx];
		}
		idx++;
	}

	do {
		old_bus_details = playback_node->bus_details.load();
	} while (!playback_node->bus_details.compare_exchange_strong(old_bus_details, new_bus_details));

	data->bus_details_graveyard.insert(old_bus_details);
}

void AudioServer::set_playback_all_bus_volumes_linear(
	Ref<AudioStreamPlayback> p_playback, Vector<AudioFrame> p_volumes)
{
	ERR_FAIL_COND(p_playback.is_null());
	ERR_FAIL_COND(p_volumes.size() != MAX_CHANNELS_PER_BUS);

	HashMap<String, Vector<AudioFrame>> map;

	AudioStreamPlaybackListNode* playback_node = _find_playback_list_node(p_playback);
	if (!playback_node) {
		return;
	}
	for (int bus_idx = 0; bus_idx < MAX_BUSES_PER_PLAYBACK; bus_idx++) {
		if (playback_node->bus_details.load()->bus_active[bus_idx]) {
			map[playback_node->bus_details.load()->bus[bus_idx]] = p_volumes;
		}
	}

	set_playback_bus_volumes_linear(p_playback, map);
}

void AudioServer::set_playback_pitch_scale(Ref<AudioStreamPlayback> p_playback, float p_pitch_scale)
{
	ERR_FAIL_COND(p_playback.is_null());

	// Samples.
	if (p_playback->get_is_sample() && p_playback->get_sample_playback().is_valid()) {
		Ref<AudioSamplePlayback> sample_playback = p_playback->get_sample_playback();
		AudioServer::update_sample_playback_pitch_scale(
			sample_playback, p_pitch_scale);
		return;
	}

	AudioStreamPlaybackListNode* playback_node = _find_playback_list_node(p_playback);
	if (!playback_node) {
		return;
	}

	playback_node->pitch_scale.set(p_pitch_scale);
}

void AudioServer::set_playback_paused(Ref<AudioStreamPlayback> p_playback, bool p_paused)
{
	ERR_FAIL_COND(p_playback.is_null());

	AudioStreamPlaybackListNode* playback_node = _find_playback_list_node(p_playback);
	if (!playback_node) {
		return;
	}

	AudioStreamPlaybackListNode::PlaybackState new_state, old_state;
	do {
		old_state = playback_node->state.load();
		new_state = p_paused ? AudioStreamPlaybackListNode::FADE_OUT_TO_PAUSE
							 : AudioStreamPlaybackListNode::PLAYING;
		if (!p_paused && old_state == AudioStreamPlaybackListNode::PLAYING) {
			return; // No-op.
		}
		if (p_paused && (old_state == AudioStreamPlaybackListNode::PAUSED ||
							old_state == AudioStreamPlaybackListNode::FADE_OUT_TO_PAUSE)) {
			return; // No-op.
		}

	} while (!playback_node->state.compare_exchange_strong(old_state, new_state));
}

void AudioServer::set_playback_highshelf_params(
	Ref<AudioStreamPlayback> p_playback, float p_gain, float p_attenuation_cutoff_hz)
{
	ERR_FAIL_COND(p_playback.is_null());

	AudioStreamPlaybackListNode* playback_node = _find_playback_list_node(p_playback);
	if (!playback_node) {
		return;
	}

	playback_node->attenuation_filter_cutoff_hz.set(p_attenuation_cutoff_hz);
	playback_node->highshelf_gain.set(p_gain);
}

bool AudioServer::is_playback_active(Ref<AudioStreamPlayback> p_playback)
{
	ERR_FAIL_COND_V(p_playback.is_null(), false);

	if (p_playback->get_is_sample()) {
		if (p_playback->get_sample_playback().is_valid()) {
			return data->sample_playback_list.has(p_playback->get_sample_playback());
		}
		else {
			return false;
		}
	}

	AudioStreamPlaybackListNode* playback_node = _find_playback_list_node(p_playback);
	if (!playback_node) {
		return false;
	}

	return playback_node->state.load() == AudioStreamPlaybackListNode::PLAYING;
}

float AudioServer::get_playback_position(Ref<AudioStreamPlayback> p_playback)
{
	ERR_FAIL_COND_V(p_playback.is_null(), 0);

	// Samples.
	if (p_playback->get_is_sample() && p_playback->get_sample_playback().is_valid()) {
		Ref<AudioSamplePlayback> sample_playback = p_playback->get_sample_playback();
		return AudioServer::get_sample_playback_position(sample_playback);
	}

	AudioStreamPlaybackListNode* playback_node = _find_playback_list_node(p_playback);
	if (!playback_node) {
		return 0;
	}

	return playback_node->stream_playback->get_playback_position();
}

bool AudioServer::is_playback_paused(Ref<AudioStreamPlayback> p_playback)
{
	ERR_FAIL_COND_V(p_playback.is_null(), false);

	AudioStreamPlaybackListNode* playback_node = _find_playback_list_node(p_playback);
	if (!playback_node) {
		return false;
	}

	return playback_node->state.load() == AudioStreamPlaybackListNode::PAUSED ||
		   playback_node->state.load() == AudioStreamPlaybackListNode::FADE_OUT_TO_PAUSE;
}

uint64_t AudioServer::get_mix_count() { return data->mix_count; }

uint64_t AudioServer::get_mixed_frames() { return data->mix_frames; }

String AudioServer::get_driver_name() { return AudioDriverManager::get_active_driver()->get_name(); }

void AudioServer::notify_listener_changed()
{
	for (CallbackItem* ci : data->listener_changed_callback_list) {
		ci->callback(ci->userdata);
	}
}

void AudioServer::init_channels_and_buffers()
{
	data->channel_count = get_channel_count();
	data->temp_buffer.resize(data->channel_count);
	data->mix_buffer.resize(data->buffer_size + LOOKAHEAD_BUFFER_SIZE);

	for (int i = 0; i < data->temp_buffer.size(); i++) {
		data->temp_buffer.write[i].resize(data->buffer_size);
	}

	for (int i = 0; i < data->buses.size(); i++) {
		data->buses[i]->channels.resize(data->channel_count);
		for (int j = 0; j < data->channel_count; j++) {
			data->buses.write[i]->channels.write[j].buffer.resize(data->buffer_size);
		}
		_update_bus_effects(i);
	}
}

void AudioServer::_cleanup_lists()
{
	data->mix_callback_list.maybe_cleanup();
	data->update_callback_list.maybe_cleanup();
	data->listener_changed_callback_list.maybe_cleanup();
	data->playback_list.maybe_cleanup();
	for (AudioStreamPlaybackBusDetails* bus_details : data->bus_details_graveyard_frame_old) {
		data->bus_details_graveyard_frame_old.erase(
			bus_details, [](AudioStreamPlaybackBusDetails* d) { delete d; });
	}
	for (AudioStreamPlaybackBusDetails* bus_details : data->bus_details_graveyard) {
		data->bus_details_graveyard_frame_old.insert(bus_details);
		data->bus_details_graveyard.erase(bus_details);
	}
	data->bus_details_graveyard.maybe_cleanup();
	data->bus_details_graveyard_frame_old.maybe_cleanup();
}

void AudioServer::finish()
{
	for (int i = 0; i < AudioDriverManager::get_driver_count(); i++) {
		AudioDriverManager::get_driver(i)->finish();
	}

	for (int i = 0; i < data->buses.size(); i++) {
		memdelete(data->buses[i]);
	}

	data->buses.clear();
}

/* MISC config */

void AudioServer::lock() { AudioDriverManager::get_active_driver()->lock(); }

void AudioServer::unlock() { AudioDriverManager::get_active_driver()->unlock(); }

AudioServer::SpeakerMode AudioServer::get_speaker_mode()
{
	return (AudioServer::SpeakerMode)AudioDriverManager::get_active_driver()->get_speaker_mode();
}

float AudioServer::get_mix_rate() { return AudioDriverManager::get_active_driver()->get_mix_rate(); }

float AudioServer::get_input_mix_rate()
{
	return AudioDriverManager::get_active_driver()->get_input_mix_rate();
}

float AudioServer::read_output_peak_db() { return 0; }

double AudioServer::get_output_latency()
{
	return AudioDriverManager::get_active_driver()->get_latency();
}

double AudioServer::get_time_to_next_mix()
{
	return AudioDriverManager::get_active_driver()->get_time_to_next_mix();
}

double AudioServer::get_time_since_last_mix()
{
	return AudioDriverManager::get_active_driver()->get_time_since_last_mix();
}

void AudioServer::add_update_callback(AudioCallback p_callback, void* p_userdata)
{
	CallbackItem* ci = new CallbackItem();
	ci->callback = p_callback;
	ci->userdata = p_userdata;
	data->update_callback_list.insert(ci);
}

void AudioServer::remove_update_callback(AudioCallback p_callback, void* p_userdata)
{
	for (CallbackItem* ci : data->update_callback_list) {
		if (ci->callback == p_callback && ci->userdata == p_userdata) {
			data->update_callback_list.erase(ci, [](CallbackItem* c) { delete c; });
		}
	}
}

void AudioServer::add_mix_callback(AudioCallback p_callback, void* p_userdata)
{
	CallbackItem* ci = new CallbackItem();
	ci->callback = p_callback;
	ci->userdata = p_userdata;
	data->mix_callback_list.insert(ci);
}

void AudioServer::remove_mix_callback(AudioCallback p_callback, void* p_userdata)
{
	for (CallbackItem* ci : data->mix_callback_list) {
		if (ci->callback == p_callback && ci->userdata == p_userdata) {
			data->mix_callback_list.erase(ci, [](CallbackItem* c) { delete c; });
		}
	}
}

void AudioServer::add_listener_changed_callback(AudioCallback p_callback, void* p_userdata)
{
	CallbackItem* ci = new CallbackItem();
	ci->callback = p_callback;
	ci->userdata = p_userdata;
	data->listener_changed_callback_list.insert(ci);
}

void AudioServer::remove_listener_changed_callback(AudioCallback p_callback, void* p_userdata)
{
	for (CallbackItem* ci : data->listener_changed_callback_list) {
		if (ci->callback == p_callback && ci->userdata == p_userdata) {
			data->listener_changed_callback_list.erase(ci, [](CallbackItem* c) { delete c; });
		}
	}
}

Ref<AudioBusLayout> AudioServer::generate_bus_layout()
{
	Ref<AudioBusLayout> state;
	state.instantiate();

	state->buses.resize(data->buses.size());

	for (int i = 0; i < data->buses.size(); i++) {
		state->buses.write[i].name = data->buses[i]->name;
		state->buses.write[i].send = data->buses[i]->send;
		state->buses.write[i].mute = data->buses[i]->mute;
		state->buses.write[i].solo = data->buses[i]->solo;
		state->buses.write[i].bypass = data->buses[i]->bypass;
		state->buses.write[i].volume_db = data->buses[i]->volume_db;
		for (int j = 0; j < data->buses[i]->effects.size(); j++) {
			AudioBusLayout::Bus::Effect fx;
			fx.effect = data->buses[i]->effects[j].effect;
			fx.enabled = data->buses[i]->effects[j].enabled;
			state->buses.write[i].effects.push_back(fx);
		}
	}

	return state;
}

PackedStringArray AudioServer::get_output_device_list()
{
	return AudioDriverManager::get_active_driver()->get_output_device_list();
}

String AudioServer::get_output_device()
{
	return AudioDriverManager::get_active_driver()->get_output_device();
}

void AudioServer::set_output_device(const String& p_name)
{
	AudioDriverManager::get_active_driver()->set_output_device(p_name);
}

PackedStringArray AudioServer::get_input_device_list()
{
	return AudioDriverManager::get_active_driver()->get_input_device_list();
}

String AudioServer::get_input_device() { return AudioDriverManager::get_active_driver()->get_input_device(); }

void AudioServer::set_input_device(const String& p_name)
{
	AudioDriverManager::get_active_driver()->set_input_device(p_name);
}

int AudioServer::get_input_frames_available()
{
	AudioDriver* ad = AudioDriverManager::get_active_driver();
	ad->lock();
	int64_t input_position = ad->get_input_position();
	if (input_position < data->input_buffer_ofs) {
		input_position += ad->get_input_buffer().size();
	}
	ad->unlock();
	return (int)((input_position - data->input_buffer_ofs) / 2); // Buffer is stereo.
}

int AudioServer::get_input_buffer_length_frames()
{
	AudioDriver* ad = AudioDriverManager::get_active_driver();
	ad->lock();
	int buffsize = ad->get_input_buffer().size();
	ad->unlock();
	return buffsize / 2;
}

PackedVector2Array AudioServer::get_input_frames(int p_frames)
{
	PackedVector2Array ret;
	AudioDriver* ad = AudioDriverManager::get_active_driver();
	ad->lock();
	int input_position = ad->get_input_position();
	Vector<int32_t> buf = ad->get_input_buffer();
	if (input_position < data->input_buffer_ofs) {
		input_position += buf.size();
	}
	if ((data->input_buffer_ofs + p_frames * 2 <= input_position) && (p_frames >= 0)) {
		ret.resize(p_frames);
		for (int i = 0; i < p_frames; i++) {
			float l = (buf[data->input_buffer_ofs++] >> 16) / 32768.f;
			if (data->input_buffer_ofs >= buf.size()) {
				data->input_buffer_ofs = 0;
			}
			float r = (buf[data->input_buffer_ofs++] >> 16) / 32768.f;
			if (data->input_buffer_ofs >= buf.size()) {
				data->input_buffer_ofs = 0;
			}
			ret.write[i] = Vector2(l, r);
		}
	}
	ad->unlock();
	return ret;
}

void AudioServer::set_enable_tagging_used_audio_streams(bool p_enable)
{
	data->tag_used_audio_streams = p_enable;
}

AudioServer::PlaybackType AudioServer::get_default_playback_type()
{
	int playback_type = GLOBAL_GET_CACHED(int, "audio/general/default_playback_type");
	ERR_FAIL_COND_V_MSG(playback_type < 0 || playback_type >= PlaybackType::PLAYBACK_TYPE_MAX,
		PlaybackType::PLAYBACK_TYPE_STREAM,
		vformat(
			R"(Project settings value (%s) for "audio/general/default_playback_type" is not supported)",
			playback_type));

	switch (playback_type) {
	case 1: {
		return PlaybackType::PLAYBACK_TYPE_SAMPLE;
	} break;

	case 0:
	default: {
		return PlaybackType::PLAYBACK_TYPE_STREAM;
	} break;
	}
}

bool AudioServer::is_stream_registered_as_sample(const Ref<AudioStream>& p_stream)
{
	ERR_FAIL_COND_V_MSG(p_stream.is_null(), false, "Parameter p_stream is null.");
	return AudioDriverManager::get_active_driver()->is_stream_registered_as_sample(p_stream);
}

void AudioServer::register_stream_as_sample(const Ref<AudioStream>& p_stream)
{
	ERR_FAIL_COND_MSG(p_stream.is_null(), "Parameter p_stream is null.");
	ERR_FAIL_COND_MSG(!(p_stream->can_be_sampled()), "Parameter p_stream cannot be sampled.");
	Ref<AudioSample> sample = p_stream->generate_sample();
	register_sample(sample);
}

void AudioServer::unregister_stream_as_sample(const Ref<AudioStream>& p_stream)
{
	ERR_FAIL_COND_MSG(p_stream.is_null(), "Parameter p_stream is null.");
	ERR_FAIL_COND_MSG(!(p_stream->can_be_sampled()), "Parameter p_stream cannot be sampled.");
	Ref<AudioSample> sample = p_stream->generate_sample();
	unregister_sample(sample);
}

void AudioServer::register_sample(const Ref<AudioSample>& p_sample)
{
	ERR_FAIL_COND_MSG(p_sample.is_null(), "Parameter p_sample is null.");
	ERR_FAIL_COND_MSG(p_sample->stream.is_null(), "Parameter p_sample->stream is null.");
	ERR_FAIL_COND_MSG(
		!(p_sample->stream->can_be_sampled()), "Parameter p_stream cannot be sampled.");
	AudioDriverManager::get_active_driver()->register_sample(p_sample);
}

void AudioServer::unregister_sample(const Ref<AudioSample>& p_sample)
{
	ERR_FAIL_COND_MSG(p_sample.is_null(), "Parameter p_sample is null.");
	ERR_FAIL_COND_MSG(p_sample->stream.is_null(), "Parameter p_sample->stream is null.");
	AudioDriverManager::get_active_driver()->unregister_sample(p_sample);
}

void AudioServer::start_sample_playback(const Ref<AudioSamplePlayback>& p_playback)
{
	ERR_FAIL_COND_MSG(p_playback.is_null(), "Parameter p_playback is null.");
	AudioDriverManager::get_active_driver()->start_sample_playback(p_playback);
	data->sample_playback_list.ordered_insert(p_playback);
}

void AudioServer::stop_sample_playback(const Ref<AudioSamplePlayback>& p_playback)
{
	ERR_FAIL_COND_MSG(p_playback.is_null(), "Parameter p_playback is null.");
	if (!data->sample_playback_list.has(p_playback)) {
		return;
	}
	data->sample_playback_list.erase(p_playback);
	AudioDriverManager::get_active_driver()->stop_sample_playback(p_playback);
	p_playback->stream_playback->set_sample_playback(nullptr);
	stop_playback_stream(p_playback->stream_playback);
}

void AudioServer::set_sample_playback_pause(
	const Ref<AudioSamplePlayback>& p_playback, bool p_paused)
{
	ERR_FAIL_COND_MSG(p_playback.is_null(), "Parameter p_playback is null.");
	AudioDriverManager::get_active_driver()->set_sample_playback_pause(p_playback, p_paused);
}

bool AudioServer::is_sample_playback_active(const Ref<AudioSamplePlayback>& p_playback)
{
	ERR_FAIL_COND_V_MSG(p_playback.is_null(), false, "Parameter p_playback is null.");
	return data->sample_playback_list.has(p_playback);
}

double AudioServer::get_sample_playback_position(const Ref<AudioSamplePlayback>& p_playback)
{
	ERR_FAIL_COND_V_MSG(p_playback.is_null(), false, "Parameter p_playback is null.");
	return AudioDriverManager::get_active_driver()->get_sample_playback_position(p_playback);
}

void AudioServer::update_sample_playback_pitch_scale(
	const Ref<AudioSamplePlayback>& p_playback, float p_pitch_scale)
{
	ERR_FAIL_COND_MSG(p_playback.is_null(), "Parameter p_playback is null.");
	return AudioDriverManager::get_active_driver()->update_sample_playback_pitch_scale(
		p_playback, p_pitch_scale);
}

void AudioServer::initialize()
{
	ERR_FAIL_COND(data != nullptr);
	data = memnew(Data);
}

void AudioServer::finalize()
{
	if (!data) {
		return;
	}
	// Cleanup resources while we still have an active AudioServer state,
	// for resources that depend on AudioServer still existing.
	_cleanup_lists();

	memdelete(data);
	data = nullptr;
}

AudioBusLayout::AudioBusLayout()
{
	buses.resize(1);
	buses.write[0].name = SceneString(Master);
}

void AudioServer::_update_bus_effects(int) {}

void AudioServer::set_bus_volume_db(int, float) {}

void AudioServer::init() {}

void AudioDriverManager::initialize(int) {}

void AudioServer::update() {}

void AudioServer::load_default_bus_layout() {}

void AudioServer::get_argument_options(
	const String& p_function, int p_idx, List<String>* r_options)
{
}

#ifdef MARK_EDITED
#undef MARK_EDITED
#endif

// ============================================================================
// Merged from servers/audio/audio_driver_dummy.cpp
// ============================================================================

#include "servers/audio/audio_server.h"

#include "core/os/os.h"

Error AudioDriverDummy::init() {
	active.clear();
	exit_thread.clear();
	samples_in = nullptr;

	if (mix_rate == -1) {
		mix_rate = _get_configured_mix_rate();
	}

	channels = get_channels();
	samples_in = memnew_arr(int32_t, size_t(buffer_frames) * channels);

	if (use_threads) {
		thread.start(AudioDriverDummy::thread_func, this);
	}

	return OK;
}

void AudioDriverDummy::thread_func(void *p_udata) {
	AudioDriverDummy *ad = static_cast<AudioDriverDummy *>(p_udata);

	uint64_t usdelay = (ad->buffer_frames / float(ad->mix_rate)) * 1000000;

	while (!ad->exit_thread.is_set()) {
		if (ad->active.is_set()) {
			ad->lock();
			ad->start_counting_ticks();

			ad->audio_server_process(ad->buffer_frames, ad->samples_in);

			ad->stop_counting_ticks();
			ad->unlock();
		}

		OS::delay_usec(usdelay);
	}
}

void AudioDriverDummy::start() {
	active.set();
}

int AudioDriverDummy::get_mix_rate() const {
	return mix_rate;
}

AudioDriver::SpeakerMode AudioDriverDummy::get_speaker_mode() const {
	return speaker_mode;
}

void AudioDriverDummy::lock() {
	mutex.lock();
}

void AudioDriverDummy::unlock() {
	mutex.unlock();
}

void AudioDriverDummy::set_use_threads(bool p_use_threads) {
	use_threads = p_use_threads;
}

void AudioDriverDummy::set_speaker_mode(SpeakerMode p_mode) {
	speaker_mode = p_mode;
}

void AudioDriverDummy::set_mix_rate(int p_rate) {
	mix_rate = p_rate;
}

uint32_t AudioDriverDummy::get_channels() const {
	static const int channels_for_mode[4] = { 2, 4, 8, 16 };
	return channels_for_mode[speaker_mode];
}

void AudioDriverDummy::mix_audio(int p_frames, int32_t *p_buffer) {
	ERR_FAIL_COND(!active.is_set()); // If not active, should not mix.
	ERR_FAIL_COND(use_threads == true); // If using threads, this will not work well.

	uint32_t todo = p_frames;
	while (todo) {
		uint32_t to_mix = MIN(buffer_frames, todo);
		lock();
		audio_server_process(to_mix, samples_in);
		unlock();

		uint32_t total_samples = to_mix * channels;

		for (uint32_t i = 0; i < total_samples; i++) {
			p_buffer[i] = samples_in[i];
		}

		todo -= to_mix;
		p_buffer += total_samples;
	}
}

void AudioDriverDummy::finish() {
	if (use_threads) {
		exit_thread.set();
		if (thread.is_started()) {
			thread.wait_to_finish();
		}
	}

	if (samples_in) {
		memdelete_arr(samples_in);
	}
}

AudioDriverDummy::AudioDriverDummy() {}

// ============================================================================
// Merged from servers/audio/audio_filter_sw.cpp
// ============================================================================

#include "servers/audio/audio_server.h"

#include "core/math/math_funcs.h"

void AudioFilterSW::set_mode(Mode p_mode) {
	mode = p_mode;
}

void AudioFilterSW::set_cutoff(float p_cutoff) {
	cutoff = p_cutoff;
}

void AudioFilterSW::set_resonance(float p_resonance) {
	resonance = p_resonance;
}

void AudioFilterSW::set_gain(float p_gain) {
	gain = p_gain;
}

void AudioFilterSW::set_sampling_rate(float p_srate) {
	sampling_rate = p_srate;
}

void AudioFilterSW::prepare_coefficients(Coeffs *p_coeffs) {
	int sr_limit = (sampling_rate / 2) + 512;

	double final_cutoff = (cutoff > sr_limit) ? sr_limit : cutoff;
	if (final_cutoff < 1) {
		final_cutoff = 1; //don't allow less than this
	}

	double omega = Math::TAU * final_cutoff / sampling_rate;

	double sin_v = Math::sin(omega);
	double cos_v = Math::cos(omega);

	double Q = resonance;
	if (Q <= 0.0) {
		Q = 0.0001;
	}

	if (mode == BANDPASS) {
		Q *= 2.0;
	} else if (mode == PEAK) {
		Q *= 3.0;
	}

	double tmpgain = gain;

	if (tmpgain < 0.001) {
		tmpgain = 0.001;
	}

	if (stages > 1) {
		Q = (Q > 1.0 ? Math::pow(Q, 1.0 / stages) : Q);
		tmpgain = Math::pow(tmpgain, 1.0 / (stages + 1));
	}
	double alpha = sin_v / (2 * Q);

	double a0 = 1.0 + alpha;

	switch (mode) {
		case LOWPASS: {
			p_coeffs->b0 = (1.0 - cos_v) / 2.0;
			p_coeffs->b1 = 1.0 - cos_v;
			p_coeffs->b2 = (1.0 - cos_v) / 2.0;
			p_coeffs->a1 = -2.0 * cos_v;
			p_coeffs->a2 = 1.0 - alpha;
		} break;

		case HIGHPASS: {
			p_coeffs->b0 = (1.0 + cos_v) / 2.0;
			p_coeffs->b1 = -(1.0 + cos_v);
			p_coeffs->b2 = (1.0 + cos_v) / 2.0;
			p_coeffs->a1 = -2.0 * cos_v;
			p_coeffs->a2 = 1.0 - alpha;
		} break;

		case BANDPASS: {
			p_coeffs->b0 = alpha * std::sqrt(Q + 1);
			p_coeffs->b1 = 0.0;
			p_coeffs->b2 = -alpha * std::sqrt(Q + 1);
			p_coeffs->a1 = -2.0 * cos_v;
			p_coeffs->a2 = 1.0 - alpha;
		} break;

		case NOTCH: {
			p_coeffs->b0 = 1.0;
			p_coeffs->b1 = -2.0 * cos_v;
			p_coeffs->b2 = 1.0;
			p_coeffs->a1 = -2.0 * cos_v;
			p_coeffs->a2 = 1.0 - alpha;
		} break;
		case PEAK: {
			p_coeffs->b0 = (1.0 + alpha * tmpgain);
			p_coeffs->b1 = (-2.0 * cos_v);
			p_coeffs->b2 = (1.0 - alpha * tmpgain);
			p_coeffs->a1 = -2 * cos_v;
			p_coeffs->a2 = (1 - alpha / tmpgain);
		} break;
		case BANDLIMIT: {
			//this one is extra tricky
			double hicutoff = resonance;
			double centercutoff = (cutoff + resonance) / 2.0;
			double bandwidth = (Math::log(centercutoff) - Math::log(hicutoff)) / Math::log((double)2);
			omega = Math::TAU * centercutoff / sampling_rate;
			alpha = Math::sin(omega) * Math::sinh(Math::log((double)2) / 2 * bandwidth * omega / Math::sin(omega));
			a0 = 1 + alpha;

			p_coeffs->b0 = alpha;
			p_coeffs->b1 = 0;
			p_coeffs->b2 = -alpha;
			p_coeffs->a1 = -2 * Math::cos(omega);
			p_coeffs->a2 = 1 - alpha;

		} break;
		case LOWSHELF: {
			double tmpq = Math::sqrt(Q);
			if (tmpq <= 0) {
				tmpq = 0.001;
			}
			double beta = Math::sqrt(tmpgain) / tmpq;

			a0 = (tmpgain + 1.0) + (tmpgain - 1.0) * cos_v + beta * sin_v;
			p_coeffs->b0 = tmpgain * ((tmpgain + 1.0) - (tmpgain - 1.0) * cos_v + beta * sin_v);
			p_coeffs->b1 = 2.0 * tmpgain * ((tmpgain - 1.0) - (tmpgain + 1.0) * cos_v);
			p_coeffs->b2 = tmpgain * ((tmpgain + 1.0) - (tmpgain - 1.0) * cos_v - beta * sin_v);
			p_coeffs->a1 = -2.0 * ((tmpgain - 1.0) + (tmpgain + 1.0) * cos_v);
			p_coeffs->a2 = ((tmpgain + 1.0) + (tmpgain - 1.0) * cos_v - beta * sin_v);

		} break;
		case HIGHSHELF: {
			double tmpq = Math::sqrt(Q);
			if (tmpq <= 0) {
				tmpq = 0.001;
			}
			double beta = Math::sqrt(tmpgain) / tmpq;

			a0 = (tmpgain + 1.0) - (tmpgain - 1.0) * cos_v + beta * sin_v;
			p_coeffs->b0 = tmpgain * ((tmpgain + 1.0) + (tmpgain - 1.0) * cos_v + beta * sin_v);
			p_coeffs->b1 = -2.0 * tmpgain * ((tmpgain - 1.0) + (tmpgain + 1.0) * cos_v);
			p_coeffs->b2 = tmpgain * ((tmpgain + 1.0) + (tmpgain - 1.0) * cos_v - beta * sin_v);
			p_coeffs->a1 = 2.0 * ((tmpgain - 1.0) - (tmpgain + 1.0) * cos_v);
			p_coeffs->a2 = ((tmpgain + 1.0) - (tmpgain - 1.0) * cos_v - beta * sin_v);

		} break;
	}

	p_coeffs->b0 /= a0;
	p_coeffs->b1 /= a0;
	p_coeffs->b2 /= a0;
	p_coeffs->a1 /= 0.0 - a0;
	p_coeffs->a2 /= 0.0 - a0;
}

void AudioFilterSW::set_stages(int p_stages) {
	stages = p_stages;
}

/* Fourier transform kernel to obtain response */

float AudioFilterSW::get_response(float p_freq, Coeffs *p_coeffs) {
	float freq = p_freq / sampling_rate * Math::TAU;

	float cx = p_coeffs->b0, cy = 0.0;

	cx += std::cos(freq) * p_coeffs->b1;
	cy -= std::sin(freq) * p_coeffs->b1;
	cx += std::cos(2 * freq) * p_coeffs->b2;
	cy -= std::sin(2 * freq) * p_coeffs->b2;

	float H = cx * cx + cy * cy;
	cx = 1.0;
	cy = 0.0;

	cx -= std::cos(freq) * p_coeffs->a1;
	cy += std::sin(freq) * p_coeffs->a1;
	cx -= std::cos(2 * freq) * p_coeffs->a2;
	cy += std::sin(2 * freq) * p_coeffs->a2;

	H = H / (cx * cx + cy * cy);
	return H;
}

AudioFilterSW::Processor::Processor() {
	set_filter(nullptr);
}

void AudioFilterSW::Processor::set_filter(AudioFilterSW *p_filter, bool p_clear_history) {
	if (p_clear_history) {
		ha1 = ha2 = hb1 = hb2 = 0;
	}
	filter = p_filter;
}

void AudioFilterSW::Processor::update_coeffs(int p_interp_buffer_len) {
	if (!filter) {
		return;
	}

	if (p_interp_buffer_len) { //interpolate
		Coeffs old_coeffs = coeffs;
		filter->prepare_coefficients(&coeffs);
		incr_coeffs.a1 = (coeffs.a1 - old_coeffs.a1) / p_interp_buffer_len;
		incr_coeffs.a2 = (coeffs.a2 - old_coeffs.a2) / p_interp_buffer_len;
		incr_coeffs.b0 = (coeffs.b0 - old_coeffs.b0) / p_interp_buffer_len;
		incr_coeffs.b1 = (coeffs.b1 - old_coeffs.b1) / p_interp_buffer_len;
		incr_coeffs.b2 = (coeffs.b2 - old_coeffs.b2) / p_interp_buffer_len;
		coeffs = old_coeffs;
	} else {
		filter->prepare_coefficients(&coeffs);
	}
}

void AudioFilterSW::Processor::process(float *p_samples, int p_amount, int p_stride, bool p_interpolate) {
	if (!filter) {
		return;
	}

	if (p_interpolate) {
		for (int i = 0; i < p_amount; i++) {
			process_one_interp(*p_samples);
			p_samples += p_stride;
		}
	} else {
		for (int i = 0; i < p_amount; i++) {
			process_one(*p_samples);
			p_samples += p_stride;
		}
	}
}

// ============================================================================
// Merged from servers/audio/audio_rb_resampler.cpp
// ============================================================================

#include "servers/audio/audio_server.h"

#include "core/math/audio_frame.h"
#include "core/math/math_funcs_binary.h"
#include "core/os/memory.h"

int AudioRBResampler::get_channel_count() const {
	if (!rb) {
		return 0;
	}

	return channels;
}

// Linear interpolation based sample rate conversion (low quality)
// Note that AudioStreamPlaybackResampled::mix has better algorithm,
// but it wasn't obvious to integrate that with VideoStreamPlayer
template <int C>
uint32_t AudioRBResampler::_resample(AudioFrame *p_dest, int p_todo, int32_t p_increment) {
	uint32_t read = offset & MIX_FRAC_MASK;

	for (int i = 0; i < p_todo; i++) {
		offset = (offset + p_increment) & (((1 << (rb_bits + MIX_FRAC_BITS)) - 1));
		read += p_increment;
		uint32_t pos = offset >> MIX_FRAC_BITS;
		float frac = float(offset & MIX_FRAC_MASK) / float(MIX_FRAC_LEN);
		ERR_FAIL_COND_V(pos >= rb_len, 0);
		uint32_t pos_next = (pos + 1) & rb_mask;

		// since this is a template with a known compile time value (C), conditionals go away when compiling.
		if constexpr (C == 1) {
			float v0 = rb[pos];
			float v0n = rb[pos_next];
			v0 += (v0n - v0) * frac;
			p_dest[i] = AudioFrame(v0, v0);
		}

		if constexpr (C == 2) {
			float v0 = rb[(pos << 1) + 0];
			float v1 = rb[(pos << 1) + 1];
			float v0n = rb[(pos_next << 1) + 0];
			float v1n = rb[(pos_next << 1) + 1];

			v0 += (v0n - v0) * frac;
			v1 += (v1n - v1) * frac;
			p_dest[i] = AudioFrame(v0, v1);
		}

		// Downmix to stereo. Apply -3dB to center, and sides, -6dB to rear.

		// four channels - channel order: front left, front right, rear left, rear right
		if constexpr (C == 4) {
			float v0 = rb[(pos << 2) + 0] + rb[(pos << 2) + 2] / 2;
			float v1 = rb[(pos << 2) + 1] + rb[(pos << 2) + 3] / 2;
			float v0n = rb[(pos_next << 2) + 0] + rb[(pos_next << 2) + 2] / 2;
			float v1n = rb[(pos_next << 2) + 1] + rb[(pos_next << 2) + 3] / 2;
			v0 += (v0n - v0) * frac;
			v1 += (v1n - v1) * frac;
			p_dest[i] = AudioFrame(v0, v1);
		}

		// six channels - channel order: front left, center, front right, rear left, rear right, LFE
		if constexpr (C == 6) {
			float v0 = rb[(pos * 6) + 0] + rb[(pos * 6) + 1] / Math::SQRT2 + rb[(pos * 6) + 3] / 2;
			float v1 = rb[(pos * 6) + 2] + rb[(pos * 6) + 1] / Math::SQRT2 + rb[(pos * 6) + 4] / 2;
			float v0n = rb[(pos_next * 6) + 0] + rb[(pos_next * 6) + 1] / Math::SQRT2 + rb[(pos_next * 6) + 3] / 2;
			float v1n = rb[(pos_next * 6) + 2] + rb[(pos_next * 6) + 1] / Math::SQRT2 + rb[(pos_next * 6) + 4] / 2;
			v0 += (v0n - v0) * frac;
			v1 += (v1n - v1) * frac;
			p_dest[i] = AudioFrame(v0, v1);
		}

		// eight channels - channel order: front left, center, front right, side left, side right, rear left, rear
		// right, LFE
		if constexpr (C == 8) {
			float v0 = rb[(pos << 3) + 0] + rb[(pos << 3) + 1] / Math::SQRT2 + rb[(pos << 3) + 3] / Math::SQRT2 + rb[(pos << 3) + 5] / 2;
			float v1 = rb[(pos << 3) + 2] + rb[(pos << 3) + 1] / Math::SQRT2 + rb[(pos << 3) + 4] / Math::SQRT2 + rb[(pos << 3) + 6] / 2;
			float v0n = rb[(pos_next << 3) + 0] + rb[(pos_next << 3) + 1] / Math::SQRT2 + rb[(pos_next << 3) + 3] / Math::SQRT2 + rb[(pos_next << 3) + 5] / 2;
			float v1n = rb[(pos_next << 3) + 2] + rb[(pos_next << 3) + 1] / Math::SQRT2 + rb[(pos_next << 3) + 4] / Math::SQRT2 + rb[(pos_next << 3) + 6] / 2;
			v0 += (v0n - v0) * frac;
			v1 += (v1n - v1) * frac;
			p_dest[i] = AudioFrame(v0, v1);
		}
	}

	return read >> MIX_FRAC_BITS; //rb_read_pos = offset >> MIX_FRAC_BITS;
}

bool AudioRBResampler::mix(AudioFrame *p_dest, int p_frames) {
	if (!rb) {
		return false;
	}

	int32_t increment = (src_mix_rate * MIX_FRAC_LEN * playback_speed) / target_mix_rate;
	int read_space = get_reader_space();
	int target_todo = MIN(get_num_of_ready_frames(), p_frames);

	{
		int src_read = 0;
		switch (channels) {
			case 1:
				src_read = _resample<1>(p_dest, target_todo, increment);
				break;
			case 2:
				src_read = _resample<2>(p_dest, target_todo, increment);
				break;
			case 4:
				src_read = _resample<4>(p_dest, target_todo, increment);
				break;
			case 6:
				src_read = _resample<6>(p_dest, target_todo, increment);
				break;
			case 8:
				src_read = _resample<8>(p_dest, target_todo, increment);
				break;
		}

		if (src_read > read_space) {
			src_read = read_space;
		}

		rb_read_pos.set((rb_read_pos.get() + src_read) & rb_mask);

		// Create fadeout effect for the end of stream (note that it can be because of slow writer)
		if (p_frames - target_todo > 0) {
			for (int i = 0; i < target_todo; i++) {
				p_dest[i] = p_dest[i] * float(target_todo - i) / float(target_todo);
			}
		}

		// Fill zeros (silence) for the rest of frames
		for (int i = target_todo; i < p_frames; i++) {
			p_dest[i] = AudioFrame(0, 0);
		}
	}

	return true;
}

int AudioRBResampler::get_num_of_ready_frames() {
	if (!is_ready()) {
		return 0;
	}
	int32_t increment = (src_mix_rate * MIX_FRAC_LEN) / target_mix_rate;
	int read_space = get_reader_space();
	return (int64_t(read_space) << MIX_FRAC_BITS) / increment;
}

Error AudioRBResampler::setup(int p_channels, int p_src_mix_rate, int p_target_mix_rate, int p_buffer_msec, int p_minbuff_needed) {
	ERR_FAIL_COND_V(p_channels != 1 && p_channels != 2 && p_channels != 4 && p_channels != 6 && p_channels != 8, ERR_INVALID_PARAMETER);

	int desired_rb_bits = Math::nearest_shift((uint32_t)MAX((p_buffer_msec / 1000.0) * p_src_mix_rate, p_minbuff_needed));

	bool recreate = !rb;

	if (rb && (uint32_t(desired_rb_bits) != rb_bits || channels != uint32_t(p_channels))) {
		memdelete_arr(rb);
		memdelete_arr(read_buf);
		recreate = true;
	}

	if (recreate) {
		channels = p_channels;
		rb_bits = desired_rb_bits;
		rb_len = (1 << rb_bits);
		rb_mask = rb_len - 1;
		const size_t array_size = rb_len * (size_t)p_channels;
		rb = memnew_arr(float, array_size);
		read_buf = memnew_arr(float, array_size);
	}

	src_mix_rate = p_src_mix_rate;
	target_mix_rate = p_target_mix_rate;
	offset = 0;
	rb_read_pos.set(0);
	rb_write_pos.set(0);

	//avoid maybe strange noises upon load
	for (unsigned int i = 0; i < (rb_len * channels); i++) {
		rb[i] = 0;
		read_buf[i] = 0;
	}

	return OK;
}

void AudioRBResampler::clear() {
	if (!rb) {
		return;
	}

	//should be stopped at this point but just in case
	memdelete_arr(rb);
	memdelete_arr(read_buf);
	rb = nullptr;
	offset = 0;
	rb_read_pos.set(0);
	rb_write_pos.set(0);
	read_buf = nullptr;
}

void AudioRBResampler::set_playback_speed(double p_playback_speed) {
	playback_speed = p_playback_speed;
}

double AudioRBResampler::get_playback_speed() const {
	return playback_speed;
}

AudioRBResampler::AudioRBResampler() {
	rb = nullptr;
	offset = 0;
	read_buf = nullptr;
	rb_read_pos.set(0);
	rb_write_pos.set(0);

	rb_bits = 0;
	rb_len = 0;
	rb_mask = 0;
	read_buff_len = 0;
	channels = 0;
	src_mix_rate = 0;
	target_mix_rate = 0;
}

AudioRBResampler::~AudioRBResampler() {
	if (rb) {
		memdelete_arr(rb);
		memdelete_arr(read_buf);
	}
}

// ============================================================================
// Merged from servers/audio/audio_effect.cpp
// ============================================================================

#include "audio_effect.h"

#include "servers/audio/audio_server.h"
// --- Inlined from servers/audio/effects/audio_effects.h ---
#include "core/os/thread.h"
#include "core/templates/local_vector.h"
#include "core/templates/ring_buffer.h"
#include "scene/resources/audio_stream_wav.h"
#include "servers/audio/audio_effect.h"
#include "servers/audio/audio_server.h"
// --- Folded from servers/audio/effects/eq_filter.h ---
#include "core/templates/vector.h"

class EQ {
public:
	enum Preset {
		PRESET_6_BANDS,
		PRESET_8_BANDS,
		PRESET_10_BANDS,
		PRESET_21_BANDS,
		PRESET_31_BANDS
	};

	class BandProcess {
		friend class EQ;
		float c1, c2, c3;
		struct History {
			float a1, a2, a3;
			float b1, b2, b3;

		} history;

	public:
		inline void process_one(float &p_data);

		BandProcess();
	};

private:
	struct Band {
		float freq;
		float c1, c2, c3;
	};

	Vector<Band> band;

	float mix_rate;

	void recalculate_band_coefficients();

public:
	void set_mix_rate(float p_mix_rate);

	int get_band_count() const;
	void set_preset_band_mode(Preset p_preset);
	void set_bands(const Vector<float> &p_bands);
	BandProcess get_band_processor(int p_band) const;
	float get_band_frequency(int p_band);

	EQ();
	~EQ();
};

/* Inline Function */

inline void EQ::BandProcess::process_one(float &p_data) {
	history.a1 = p_data;

	history.b1 = c1 * (history.a1 - history.a3) + c3 * history.b2 - c2 * history.b3;

	p_data = history.b1;

	history.a3 = history.a2;
	history.a2 = history.a1;
	history.b3 = history.b2;
	history.b2 = history.b1;
}

// --- End servers/audio/effects/eq_filter.h ---
// --- Folded from servers/audio/effects/reverb_filter.h ---
class Reverb {
public:
	enum {
		INPUT_BUFFER_MAX_SIZE = 1024,

	};

private:
	enum {
		MAX_COMBS = 8,
		MAX_ALLPASS = 4,
		MAX_ECHO_MS = 500

	};

	static const float comb_tunings[MAX_COMBS];
	static const float allpass_tunings[MAX_ALLPASS];

	struct Comb {
		int size = 0;
		float *buffer = nullptr;
		float feedback = 0;
		float damp = 0; //lowpass
		float damp_h = 0; //history
		int pos = 0;
		int extra_spread_frames = 0;

		Comb() {}
	};

	struct AllPass {
		int size = 0;
		float *buffer = nullptr;
		int pos = 0;
		int extra_spread_frames = 0;
		AllPass() {}
	};

	Comb comb[MAX_COMBS];
	AllPass allpass[MAX_ALLPASS];
	float *input_buffer = nullptr;
	float *echo_buffer = nullptr;
	int echo_buffer_size = 0;
	int echo_buffer_pos = 0;

	float hpf_h1 = 0.0f;
	float hpf_h2 = 0.0f;

	struct Parameters {
		float room_size;
		float damp;
		float wet;
		float dry;
		float mix_rate;
		float extra_spread_base;
		float extra_spread;
		float predelay;
		float predelay_fb;
		float hpf;
	} params;

	void configure_buffers();
	void update_parameters();
	void clear_buffers();

public:
	void set_room_size(float p_size);
	void set_damp(float p_damp);
	void set_wet(float p_wet);
	void set_dry(float p_dry);
	void set_predelay(float p_predelay); // in ms
	void set_predelay_feedback(float p_predelay_fb); // in ms
	void set_highpass(float p_frq);
	void set_mix_rate(float p_mix_rate);
	void set_extra_spread(float p_spread);
	void set_extra_spread_base(float p_sec);

	void process(float *p_src, float *p_dst, int p_frames);

	Reverb();

	~Reverb();
};

// --- End servers/audio/effects/reverb_filter.h ---

// ============================================================================
// 1. Amplify
// ============================================================================

struct AudioEffectAmplifyConfig
{
	float volume_db = 0.0f;

	void set_volume_db(float p_volume);
	float get_volume_db() const;
	void set_volume_linear(float p_volume);
	float get_volume_linear() const;
};

struct AudioEffectAmplifyState
{
	Ref<AudioEffect> base;
	float mix_volume_db = 0.0f;

	void process(const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count);
};

using AudioEffectAmplifyInstance = AudioEffectAmplifyState;

// ============================================================================
// 2. Capture
// ============================================================================

struct AudioEffectCaptureConfig
{
	RingBuffer<AudioFrame> buffer;
	SafeNumeric<uint64_t> discarded_frames;
	SafeNumeric<uint64_t> pushed_frames;
	float buffer_length_seconds = 0.1f;
	bool buffer_initialized = false;

	void set_buffer_length(float p_buffer_length_seconds);
	float get_buffer_length() const;

	bool can_get_buffer(int p_frames) const;
	PackedVector2Array get_buffer(int p_frames);
	void clear_buffer();

	int get_frames_available() const;
	int64_t get_discarded_frames() const;
	int get_buffer_length_frames() const;
	int64_t get_pushed_frames() const;
};

struct AudioEffectCaptureState
{
	Ref<AudioEffect> base;

	void process(const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count);
	bool process_silence() const;
};

using AudioEffectCaptureInstance = AudioEffectCaptureState;

// ============================================================================
// 3. Chorus
// ============================================================================

struct AudioEffectChorusConfig
{
	static constexpr int32_t MAX_DELAY_MS = 50;
	static constexpr int32_t MAX_DEPTH_MS = 20;
	static constexpr int32_t MAX_WIDTH_MS = 50;
	static constexpr int32_t MAX_VOICES = 4;
	static constexpr int32_t CYCLES_FRAC = 16;
	static constexpr int32_t CYCLES_MASK = (1 << CYCLES_FRAC) - 1;
	static constexpr int32_t MAX_CHANNELS = 4;
	static constexpr int32_t MS_CUTOFF_MAX = 16000;

	struct Voice
	{
		float delay = 12.0f;
		float rate = 1.0f;
		float depth = 0.0f;
		float level = 0.0f;
		float cutoff = MS_CUTOFF_MAX;
		float pan = 0.0f;
	};

	Voice voice[MAX_VOICES];
	int voice_count = 2;
	float wet = 0.5f;
	float dry = 1.0f;

	void set_voice_count(int p_voices);
	int get_voice_count() const;

	void set_voice_delay_ms(int p_voice, float p_delay_ms);
	float get_voice_delay_ms(int p_voice) const;

	void set_voice_rate_hz(int p_voice, float p_rate_hz);
	float get_voice_rate_hz(int p_voice) const;

	void set_voice_depth_ms(int p_voice, float p_depth_ms);
	float get_voice_depth_ms(int p_voice) const;

	void set_voice_level_db(int p_voice, float p_level_db);
	float get_voice_level_db(int p_voice) const;

	void set_voice_cutoff_hz(int p_voice, float p_cutoff_hz);
	float get_voice_cutoff_hz(int p_voice) const;

	void set_voice_pan(int p_voice, float p_pan);
	float get_voice_pan(int p_voice) const;

	void set_wet(float p_amount);
	float get_wet() const;

	void set_dry(float p_amount);
	float get_dry() const;

	AudioEffectChorusConfig();
};

struct AudioEffectChorusState
{
	Ref<AudioEffect> base;

	Vector<AudioFrame> audio_buffer;
	unsigned int buffer_pos = 0;
	unsigned int buffer_mask = 0;

	AudioFrame filter_h[4];
	uint64_t cycles[4] = {};

	void _process_chunk(const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count);
	void process(const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count);
};

using AudioEffectChorusInstance = AudioEffectChorusState;

// ============================================================================
// 4. Compressor
// ============================================================================

struct AudioEffectCompressorConfig
{
	float threshold = 0.0f;
	float ratio = 4.0f;
	float gain = 0.0f;
	float attack_us = 20.0f;
	float release_ms = 250.0f;
	float mix = 1.0f;
	String sidechain;

	void set_threshold(float p_threshold);
	float get_threshold() const;

	void set_ratio(float p_ratio);
	float get_ratio() const;

	void set_gain(float p_gain);
	float get_gain() const;

	void set_attack_us(float p_attack_us);
	float get_attack_us() const;

	void set_release_ms(float p_release_ms);
	float get_release_ms() const;

	void set_mix(float p_mix);
	float get_mix() const;

	void set_sidechain(const String& p_sidechain);
	String get_sidechain() const;
};

struct AudioEffectCompressorState
{
	Ref<AudioEffect> base;

	float rundb = 0.0f;
	float averatio = 0.0f;
	float runratio = 0.0f;
	float runmax = 0.0f;
	float maxover = 0.0f;
	float gr_meter = 1.0f;
	int current_channel = -1;

	void set_current_channel(int p_channel) { current_channel = p_channel; }

	void process(const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count);
};

using AudioEffectCompressorInstance = AudioEffectCompressorState;

// ============================================================================
// 5. Delay
// ============================================================================

struct AudioEffectDelayConfig
{
	enum
	{
		MAX_DELAY_MS = 3000,
		MAX_TAPS = 2
	};

	float dry = 1.0f;

	bool tap_1_active = true;
	float tap_1_delay_ms = 250.0f;
	float tap_1_level = -6.0f;
	float tap_1_pan = 0.2f;

	bool tap_2_active = true;
	float tap_2_delay_ms = 500.0f;
	float tap_2_level = -12.0f;
	float tap_2_pan = -0.4f;

	bool feedback_active = false;
	float feedback_delay_ms = 340.0f;
	float feedback_level = -6.0f;
	float feedback_lowpass = 16000.0f;

	void set_dry(float p_dry);
	float get_dry() const;

	void set_tap1_active(bool p_active);
	bool is_tap1_active() const;

	void set_tap1_delay_ms(float p_delay_ms);
	float get_tap1_delay_ms() const;

	void set_tap1_level_db(float p_level_db);
	float get_tap1_level_db() const;

	void set_tap1_pan(float p_pan);
	float get_tap1_pan() const;

	void set_tap2_active(bool p_active);
	bool is_tap2_active() const;

	void set_tap2_delay_ms(float p_delay_ms);
	float get_tap2_delay_ms() const;

	void set_tap2_level_db(float p_level_db);
	float get_tap2_level_db() const;

	void set_tap2_pan(float p_pan);
	float get_tap2_pan() const;

	void set_feedback_active(bool p_active);
	bool is_feedback_active() const;

	void set_feedback_delay_ms(float p_delay_ms);
	float get_feedback_delay_ms() const;

	void set_feedback_level_db(float p_level_db);
	float get_feedback_level_db() const;

	void set_feedback_lowpass(float p_lowpass);
	float get_feedback_lowpass() const;
};

struct AudioEffectDelayState
{
	Ref<AudioEffect> base;

	Vector<AudioFrame> ring_buffer;
	unsigned int ring_buffer_pos = 0;
	unsigned int ring_buffer_mask = 0;

	Vector<AudioFrame> feedback_buffer;
	unsigned int feedback_buffer_pos = 0;

	AudioFrame h;

	void _process_chunk(const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count);
	void process(const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count);
};

using AudioEffectDelayInstance = AudioEffectDelayState;

// ============================================================================
// 6. Distortion
// ============================================================================

struct AudioEffectDistortionConfig
{
	enum Mode
	{
		MODE_CLIP,
		MODE_ATAN,
		MODE_LOFI,
		MODE_OVERDRIVE,
		MODE_WAVESHAPE,
	};

	Mode mode = MODE_CLIP;
	float pre_gain = 0.0f;
	float post_gain = 0.0f;
	float keep_hf_hz = 16000.0f;
	float drive = 0.0f;

	void set_mode(Mode p_mode);
	Mode get_mode() const;

	void set_pre_gain(float p_pre_gain);
	float get_pre_gain() const;

	void set_keep_hf_hz(float p_keep_hf_hz);
	float get_keep_hf_hz() const;

	void set_drive(float p_drive);
	float get_drive() const;

	void set_post_gain(float p_post_gain);
	float get_post_gain() const;
};

struct AudioEffectDistortionState
{
	Ref<AudioEffect> base;
	float h[2] = {};

	void process(const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count);
};

using AudioEffectDistortionInstance = AudioEffectDistortionState;

// ============================================================================
// 7. EQ
// ============================================================================

struct AudioEffectEQConfig
{
	EQ eq;
	Vector<float> gain;
	HashMap<String, int> prop_band_map;
	Vector<String> band_names;

	void set_preset(EQ::Preset p_preset);
	void set_band_gain_db(int p_band, float p_volume);
	float get_band_gain_db(int p_band) const;
	int get_band_count() const;

	AudioEffectEQConfig(EQ::Preset p_preset = EQ::PRESET_6_BANDS);
};

struct AudioEffectEQState
{
	Ref<AudioEffect> base;

	Vector<EQ::BandProcess> bands[2];
	Vector<float> gains;

	void process(const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count);
};

using AudioEffectEQInstance = AudioEffectEQState;

// ============================================================================
// 8. Filter
// ============================================================================

struct AudioEffectFilterConfig
{
	enum FilterDB
	{
		FILTER_6DB,
		FILTER_12DB,
		FILTER_18DB,
		FILTER_24DB,
	};

	AudioFilterSW::Mode mode = AudioFilterSW::LOWPASS;
	float cutoff = 2000.0f;
	float resonance = 0.5f;
	float gain = 1.0f;
	FilterDB db = FILTER_6DB;

	void set_filter_mode(AudioFilterSW::Mode p_mode);
	AudioFilterSW::Mode get_filter_mode() const;

	void set_cutoff(float p_freq);
	float get_cutoff() const;

	void set_resonance(float p_amount);
	float get_resonance() const;

	void set_gain(float p_amount);
	float get_gain() const;

	void set_db(FilterDB p_db);
	FilterDB get_db() const;

	AudioEffectFilterConfig(AudioFilterSW::Mode p_mode = AudioFilterSW::LOWPASS);
};

struct AudioEffectFilterState
{
	Ref<AudioEffect> base;

	AudioFilterSW filter;
	AudioFilterSW::Processor filter_process[2][4];

	template <int S>
	void _process_filter(const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count);

	void process(const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count);

	AudioEffectFilterState();
};

using AudioEffectFilterInstance = AudioEffectFilterState;

// ============================================================================
// 9. HardLimiter
// ============================================================================

struct AudioEffectHardLimiterConfig
{
	float pre_gain = 0.0f;
	float ceiling = -0.3f;
	float sustain = 0.02f;
	float release = 0.1f;
	float attack = 0.002f;

	void set_ceiling_db(float p_ceiling);
	float get_ceiling_db() const;

	void set_release(float p_release);
	float get_release() const;

	void set_pre_gain_db(float p_pre_gain);
	float get_pre_gain_db() const;
};

struct AudioEffectHardLimiterState
{
	Ref<AudioEffect> base;

	int sample_cursor = 0;

	float release_factor = 0;
	float attack_factor = 0;
	float gain = 1;
	float gain_target = 1;

	LocalVector<float> sample_buffer_left;
	LocalVector<float> sample_buffer_right;

	int gain_samples_to_store = 0;
	int gain_bucket_cursor = 0;
	int gain_bucket_size = 0;
	LocalVector<float> gain_buckets;

	void process(const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count);
};

using AudioEffectHardLimiterInstance = AudioEffectHardLimiterState;

// ============================================================================
// 10. Limiter
// ============================================================================

struct AudioEffectLimiterConfig
{
	float threshold = 0.0f;
	float ceiling = -0.1f;
	float soft_clip = 2.0f;
	float soft_clip_ratio = 10.0f;

	void set_threshold_db(float p_threshold);
	float get_threshold_db() const;

	void set_ceiling_db(float p_ceiling);
	float get_ceiling_db() const;

	void set_soft_clip_db(float p_soft_clip);
	float get_soft_clip_db() const;

	void set_soft_clip_ratio(float p_soft_clip);
	float get_soft_clip_ratio() const;
};

struct AudioEffectLimiterState
{
	Ref<AudioEffect> base;
	float mix_volume_db = 0.0f;

	void process(const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count);
};

using AudioEffectLimiterInstance = AudioEffectLimiterState;

// ============================================================================
// 11. Panner
// ============================================================================

struct AudioEffectPannerConfig
{
	float pan = 0.0f;

	void set_pan(float p_cpanume);
	float get_pan() const;
};

struct AudioEffectPannerState
{
	Ref<AudioEffect> base;

	void process(const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count);
};

using AudioEffectPannerInstance = AudioEffectPannerState;

// ============================================================================
// 12. Phaser
// ============================================================================

struct AudioEffectPhaserConfig
{
	float range_min = 440.0f;
	float range_max = 1600.0f;
	float rate = 0.5f;
	float feedback = 0.7f;
	float depth = 1.0f;

	void set_range_min_hz(float p_hz);
	float get_range_min_hz() const;

	void set_range_max_hz(float p_hz);
	float get_range_max_hz() const;

	void set_rate_hz(float p_hz);
	float get_rate_hz() const;

	void set_feedback(float p_fbk);
	float get_feedback() const;

	void set_depth(float p_depth);
	float get_depth() const;
};

struct AudioEffectPhaserState
{
	Ref<AudioEffect> base;

	float phase = 0.0f;
	AudioFrame h;

	class AllpassDelay
	{
		float a = 0.0f;
		float h = 0.0f;

	public:
		_ALWAYS_INLINE_ void delay(float d)
		{
			a = (1.f - d) / (1.f + d);
		}

		_ALWAYS_INLINE_ float update(float s)
		{
			float y = s * -a + h;
			h = y * a + s;
			return y;
		}
	};

	AllpassDelay allpass[2][6];

	void process(const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count);
};

using AudioEffectPhaserInstance = AudioEffectPhaserState;

// ============================================================================
// 13. PitchShift
// ============================================================================

class SMBPitchShift
{
	enum
	{
		MAX_FRAME_LENGTH = 8192
	};

	float gInFIFO[MAX_FRAME_LENGTH] = {};
	float gOutFIFO[MAX_FRAME_LENGTH] = {};
	float gFFTworksp[2 * MAX_FRAME_LENGTH] = {};
	float gLastPhase[MAX_FRAME_LENGTH / 2 + 1] = {};
	float gSumPhase[MAX_FRAME_LENGTH / 2 + 1] = {};
	float gOutputAccum[2 * MAX_FRAME_LENGTH] = {};
	float gAnaFreq[MAX_FRAME_LENGTH] = {};
	float gAnaMagn[MAX_FRAME_LENGTH] = {};
	float gSynFreq[MAX_FRAME_LENGTH] = {};
	float gSynMagn[MAX_FRAME_LENGTH] = {};
	long gRover = 0;

	void smbFft(float* fftBuffer, long fftFrameSize, long sign);

public:
	void PitchShift(float pitchShift, long numSampsToProcess, long fftFrameSize, long osamp,
		float sampleRate, float* indata, float* outdata, int stride);
};

struct AudioEffectPitchShiftConfig
{
	enum FFTSize : unsigned int
	{
		FFT_SIZE_256,
		FFT_SIZE_512,
		FFT_SIZE_1024,
		FFT_SIZE_2048,
		FFT_SIZE_4096,
		FFT_SIZE_MAX
	};

	float pitch_scale = 1.0f;
	int oversampling = 4;
	FFTSize fft_size = FFT_SIZE_2048;
	float wet = 0.0f;
	float dry = 0.0f;
	bool filter = false;

	void set_pitch_scale(float p_pitch_scale);
	float get_pitch_scale() const;

	void set_oversampling(int p_oversampling);
	int get_oversampling() const;

	void set_fft_size(FFTSize p_fft_size);
	FFTSize get_fft_size() const;
};

struct AudioEffectPitchShiftState
{
	Ref<AudioEffect> base;

	int fft_size = 0;
	SMBPitchShift shift_l;
	SMBPitchShift shift_r;

	void process(const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count);
};

using AudioEffectPitchShiftInstance = AudioEffectPitchShiftState;

// ============================================================================
// 14. Record
// ============================================================================

struct AudioEffectRecordState
{
	bool is_recording = false;
	Thread io_thread;

	Vector<AudioFrame> ring_buffer;
	Vector<float> recording_data;

	unsigned int ring_buffer_pos = 0;
	unsigned int ring_buffer_mask = 0;
	unsigned int ring_buffer_read_pos = 0;

	void _io_thread_process();
	void _io_store_buffer();
	static void _thread_callback(void* _instance);
	void _update_buffer();
	static void _update(void* userdata);

	void init();
	void finish();
	void process(const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count);
	bool process_silence() const;

	~AudioEffectRecordState() { finish(); }
};

using AudioEffectRecordInstance = AudioEffectRecordState;

struct AudioEffectRecordConfig
{
	enum
	{
		IO_BUFFER_SIZE_MS = 1500
	};

	Ref<AudioEffectInstance> current_instance;
	AudioStreamWAV::Format format = AudioStreamWAV::FORMAT_16_BITS;

	void ensure_thread_stopped();
	void set_recording_active(bool p_record);
	bool is_recording_active() const;
	void set_format(AudioStreamWAV::Format p_format);
	AudioStreamWAV::Format get_format() const;
	Ref<AudioStreamWAV> get_recording() const;

	~AudioEffectRecordConfig() { ensure_thread_stopped(); }
};

// ============================================================================
// 15. Reverb
// ============================================================================

struct AudioEffectReverbConfig
{
	float predelay = 150.0f;
	float predelay_fb = 0.4f;
	float hpf = 0.0f;
	float room_size = 0.8f;
	float damping = 0.5f;
	float spread = 1.0f;
	float dry = 1.0f;
	float wet = 0.5f;

	void set_predelay_msec(float p_msec);
	void set_predelay_feedback(float p_feedback);
	void set_room_size(float p_size);
	void set_damping(float p_damping);
	void set_spread(float p_spread);
	void set_dry(float p_dry);
	void set_wet(float p_wet);
	void set_hpf(float p_hpf);

	float get_predelay_msec() const;
	float get_predelay_feedback() const;
	float get_room_size() const;
	float get_damping() const;
	float get_spread() const;
	float get_dry() const;
	float get_wet() const;
	float get_hpf() const;
};

struct AudioEffectReverbState
{
	Ref<AudioEffect> base;

	float tmp_src[Reverb::INPUT_BUFFER_MAX_SIZE] = {};
	float tmp_dst[Reverb::INPUT_BUFFER_MAX_SIZE] = {};

	Reverb reverb[2];

	void process(const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count);
	AudioEffectReverbState();
};

using AudioEffectReverbInstance = AudioEffectReverbState;

// ============================================================================
// 16. SpectrumAnalyzer
// ============================================================================

struct AudioEffectSpectrumAnalyzerConfig
{
	enum FFTSize
	{
		FFT_SIZE_256,
		FFT_SIZE_512,
		FFT_SIZE_1024,
		FFT_SIZE_2048,
		FFT_SIZE_4096,
		FFT_SIZE_MAX
	};

	float buffer_length = 2.0f;
	FFTSize fft_size = FFT_SIZE_1024;

	void set_buffer_length(float p_seconds);
	float get_buffer_length() const;

	void set_fft_size(FFTSize p_fft_size);
	FFTSize get_fft_size() const;
};

struct AudioEffectSpectrumAnalyzerState
{
	enum MagnitudeMode
	{
		MAGNITUDE_AVERAGE,
		MAGNITUDE_MAX,
	};

	Ref<AudioEffect> base;

	Vector<Vector<AudioFrame>> fft_history;
	Vector<float> temporal_fft;
	int temporal_fft_pos = 0;
	int fft_size = 0;
	int fft_count = 0;
	int fft_pos = 0;
	float mix_rate = 0.0f;

	void process(const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count);
	Vector2 get_magnitude_for_frequency_range(
		float p_begin, float p_end, MagnitudeMode p_mode = MAGNITUDE_MAX) const;
};

using AudioEffectSpectrumAnalyzerInstance = AudioEffectSpectrumAnalyzerState;

// ============================================================================
// 17. StereoEnhance
// ============================================================================

struct AudioEffectStereoEnhanceConfig
{
	float volume_db = 0.0f;
	float pan_pullout = 1.0f;
	float time_pullout = 0.0f;
	float surround = 0.0f;

	void set_pan_pullout(float p_amount);
	float get_pan_pullout() const;

	void set_time_pullout(float p_amount);
	float get_time_pullout() const;

	void set_surround(float p_amount);
	float get_surround() const;
};

struct AudioEffectStereoEnhanceState
{
	Ref<AudioEffect> base;

	enum
	{
		MAX_DELAY_MS = 50
	};

	float* delay_ringbuff = nullptr;
	unsigned int ringbuff_pos = 0;
	unsigned int ringbuff_mask = 0;

	void process(const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count);

	~AudioEffectStereoEnhanceState();
};

using AudioEffectStereoEnhanceInstance = AudioEffectStereoEnhanceState;

// --- End servers/audio/effects/audio_effects.h ---

void AudioEffect::_init_config(Type p_type)
{
	switch (p_type) {
	case EFFECT_AMPLIFY:
		config = memnew(AudioEffectAmplifyConfig);
		break;
	case EFFECT_CAPTURE:
		config = memnew(AudioEffectCaptureConfig);
		break;
	case EFFECT_CHORUS:
		config = memnew(AudioEffectChorusConfig);
		break;
	case EFFECT_COMPRESSOR:
		config = memnew(AudioEffectCompressorConfig);
		break;
	case EFFECT_DELAY:
		config = memnew(AudioEffectDelayConfig);
		break;
	case EFFECT_DISTORTION:
		config = memnew(AudioEffectDistortionConfig);
		break;
	case EFFECT_EQ:
		config = memnew(AudioEffectEQConfig);
		break;
	case EFFECT_FILTER:
		config = memnew(AudioEffectFilterConfig);
		break;
	case EFFECT_HARD_LIMITER:
		config = memnew(AudioEffectHardLimiterConfig);
		break;
	case EFFECT_LIMITER:
		config = memnew(AudioEffectLimiterConfig);
		break;
	case EFFECT_PANNER:
		config = memnew(AudioEffectPannerConfig);
		break;
	case EFFECT_PHASER:
		config = memnew(AudioEffectPhaserConfig);
		break;
	case EFFECT_PITCH_SHIFT:
		config = memnew(AudioEffectPitchShiftConfig);
		break;
	case EFFECT_RECORD:
		config = memnew(AudioEffectRecordConfig);
		break;
	case EFFECT_REVERB:
		config = memnew(AudioEffectReverbConfig);
		break;
	case EFFECT_SPECTRUM_ANALYZER:
		config = memnew(AudioEffectSpectrumAnalyzerConfig);
		break;
	case EFFECT_STEREO_ENHANCE:
		config = memnew(AudioEffectStereoEnhanceConfig);
		break;
	}
}

void AudioEffect::_free_config()
{
	if (!config) {
		return;
	}
	switch (effect_type) {
	case EFFECT_AMPLIFY:
		memdelete(static_cast<AudioEffectAmplifyConfig*>(config));
		break;
	case EFFECT_CAPTURE:
		memdelete(static_cast<AudioEffectCaptureConfig*>(config));
		break;
	case EFFECT_CHORUS:
		memdelete(static_cast<AudioEffectChorusConfig*>(config));
		break;
	case EFFECT_COMPRESSOR:
		memdelete(static_cast<AudioEffectCompressorConfig*>(config));
		break;
	case EFFECT_DELAY:
		memdelete(static_cast<AudioEffectDelayConfig*>(config));
		break;
	case EFFECT_DISTORTION:
		memdelete(static_cast<AudioEffectDistortionConfig*>(config));
		break;
	case EFFECT_EQ:
		memdelete(static_cast<AudioEffectEQConfig*>(config));
		break;
	case EFFECT_FILTER:
		memdelete(static_cast<AudioEffectFilterConfig*>(config));
		break;
	case EFFECT_HARD_LIMITER:
		memdelete(static_cast<AudioEffectHardLimiterConfig*>(config));
		break;
	case EFFECT_LIMITER:
		memdelete(static_cast<AudioEffectLimiterConfig*>(config));
		break;
	case EFFECT_PANNER:
		memdelete(static_cast<AudioEffectPannerConfig*>(config));
		break;
	case EFFECT_PHASER:
		memdelete(static_cast<AudioEffectPhaserConfig*>(config));
		break;
	case EFFECT_PITCH_SHIFT:
		memdelete(static_cast<AudioEffectPitchShiftConfig*>(config));
		break;
	case EFFECT_RECORD:
		memdelete(static_cast<AudioEffectRecordConfig*>(config));
		break;
	case EFFECT_REVERB:
		memdelete(static_cast<AudioEffectReverbConfig*>(config));
		break;
	case EFFECT_SPECTRUM_ANALYZER:
		memdelete(static_cast<AudioEffectSpectrumAnalyzerConfig*>(config));
		break;
	case EFFECT_STEREO_ENHANCE:
		memdelete(static_cast<AudioEffectStereoEnhanceConfig*>(config));
		break;
	}
	config = nullptr;
}

AudioEffect::AudioEffect(Type p_type) : effect_type(p_type)
{
	_init_config(p_type);
}

AudioEffect::~AudioEffect()
{
	_free_config();
}

void AudioEffect::set_effect_type(Type p_type)
{
	if (effect_type == p_type && config != nullptr) {
		return;
	}
	AudioServer::lock();
	_free_config();
	effect_type = p_type;
	_init_config(p_type);
	AudioServer::unlock();
}

Ref<AudioEffectInstance> AudioEffect::instantiate()
{
	switch (effect_type) {
	case EFFECT_AMPLIFY: {
		AudioEffectAmplifyConfig* cfg = get_amplify_config();
		ERR_FAIL_NULL_V(cfg, Ref<AudioEffectInstance>());
		AudioEffectAmplifyState* state = memnew(AudioEffectAmplifyState);
		state->base = Ref<AudioEffect>(this);
		state->mix_volume_db = cfg->volume_db;
		return AudioEffectInstance::create(state);
	}
	case EFFECT_CAPTURE: {
		AudioEffectCaptureConfig* cfg = get_capture_config();
		ERR_FAIL_NULL_V(cfg, Ref<AudioEffectInstance>());
		if (!cfg->buffer_initialized) {
			float target_buffer_size = AudioServer::get_mix_rate() * cfg->buffer_length_seconds;
			ERR_FAIL_COND_V(
				target_buffer_size <= 0 || target_buffer_size >= (1 << 27), Ref<AudioEffectInstance>());
			cfg->buffer.resize(Math::nearest_shift((uint32_t)target_buffer_size));
			cfg->buffer_initialized = true;
		}
		cfg->clear_buffer();
		AudioEffectCaptureState* state = memnew(AudioEffectCaptureState);
		state->base = Ref<AudioEffect>(this);
		return AudioEffectInstance::create(state);
	}
	case EFFECT_CHORUS: {
		AudioEffectChorusState* state = memnew(AudioEffectChorusState);
		state->base = Ref<AudioEffect>(this);
		for (int i = 0; i < 4; i++) {
			state->filter_h[i] = AudioFrame(0, 0);
			state->cycles[i] = 0;
		}

		float ring_buffer_max_size = AudioEffectChorusConfig::MAX_DELAY_MS +
									 AudioEffectChorusConfig::MAX_DEPTH_MS +
									 AudioEffectChorusConfig::MAX_WIDTH_MS;
		ring_buffer_max_size *= 2;
		ring_buffer_max_size /= 1000.0f;
		ring_buffer_max_size *= AudioServer::get_mix_rate();

		int ringbuff_size = ring_buffer_max_size;
		int bits = 0;
		while (ringbuff_size > 0) {
			bits++;
			ringbuff_size /= 2;
		}
		ringbuff_size = 1 << bits;
		state->buffer_mask = ringbuff_size - 1;
		state->buffer_pos = 0;
		state->audio_buffer.resize(ringbuff_size);
		for (int i = 0; i < ringbuff_size; i++) {
			state->audio_buffer.write[i] = AudioFrame(0, 0);
		}
		return AudioEffectInstance::create(state);
	}
	case EFFECT_COMPRESSOR: {
		AudioEffectCompressorState* state = memnew(AudioEffectCompressorState);
		state->base = Ref<AudioEffect>(this);
		return AudioEffectInstance::create(state);
	}
	case EFFECT_DELAY: {
		AudioEffectDelayState* state = memnew(AudioEffectDelayState);
		state->base = Ref<AudioEffect>(this);

		float ring_buffer_max_size = AudioEffectDelayConfig::MAX_DELAY_MS + 100;
		ring_buffer_max_size /= 1000.0f;
		ring_buffer_max_size *= AudioServer::get_mix_rate();

		int ringbuff_size = ring_buffer_max_size;
		int bits = 0;
		while (ringbuff_size > 0) {
			bits++;
			ringbuff_size /= 2;
		}
		ringbuff_size = 1 << bits;
		state->ring_buffer_mask = ringbuff_size - 1;
		state->ring_buffer_pos = 0;
		state->ring_buffer.resize(ringbuff_size);
		state->feedback_buffer.resize(ringbuff_size);
		state->feedback_buffer_pos = 0;
		state->h = AudioFrame(0, 0);
		return AudioEffectInstance::create(state);
	}
	case EFFECT_DISTORTION: {
		AudioEffectDistortionState* state = memnew(AudioEffectDistortionState);
		state->base = Ref<AudioEffect>(this);
		return AudioEffectInstance::create(state);
	}
	case EFFECT_EQ: {
		AudioEffectEQConfig* cfg = get_eq_config();
		ERR_FAIL_NULL_V(cfg, Ref<AudioEffectInstance>());
		AudioEffectEQState* state = memnew(AudioEffectEQState);
		state->base = Ref<AudioEffect>(this);
		state->gains.resize(cfg->eq.get_band_count());
		for (int i = 0; i < 2; i++) {
			state->bands[i].resize(cfg->eq.get_band_count());
			for (int j = 0; j < state->bands[i].size(); j++) {
				state->bands[i].write[j] = cfg->eq.get_band_processor(j);
			}
		}
		return AudioEffectInstance::create(state);
	}
	case EFFECT_FILTER: {
		AudioEffectFilterState* state = memnew(AudioEffectFilterState);
		state->base = Ref<AudioEffect>(this);
		return AudioEffectInstance::create(state);
	}
	case EFFECT_HARD_LIMITER: {
		AudioEffectHardLimiterConfig* cfg = get_hard_limiter_config();
		ERR_FAIL_NULL_V(cfg, Ref<AudioEffectInstance>());
		AudioEffectHardLimiterState* state = memnew(AudioEffectHardLimiterState);
		state->base = Ref<AudioEffect>(this);

		float mix_rate = AudioServer::get_mix_rate();
		for (int i = 0; i < (int)Math::ceil(mix_rate * cfg->attack) + 1; i++) {
			state->sample_buffer_left.push_back(0.0f);
			state->sample_buffer_right.push_back(0.0f);
		}
		state->gain_samples_to_store = (int)Math::ceil(mix_rate * (cfg->attack + cfg->sustain) + 1);
		state->gain_bucket_size = (int)(mix_rate * cfg->attack);
		for (int i = 0; i < state->gain_samples_to_store; i += state->gain_bucket_size) {
			state->gain_buckets.push_back(1.0f);
		}
		return AudioEffectInstance::create(state);
	}
	case EFFECT_LIMITER: {
		AudioEffectLimiterState* state = memnew(AudioEffectLimiterState);
		state->base = Ref<AudioEffect>(this);
		return AudioEffectInstance::create(state);
	}
	case EFFECT_PANNER: {
		AudioEffectPannerState* state = memnew(AudioEffectPannerState);
		state->base = Ref<AudioEffect>(this);
		return AudioEffectInstance::create(state);
	}
	case EFFECT_PHASER: {
		AudioEffectPhaserState* state = memnew(AudioEffectPhaserState);
		state->base = Ref<AudioEffect>(this);
		state->phase = 0;
		state->h = AudioFrame(0, 0);
		return AudioEffectInstance::create(state);
	}
	case EFFECT_PITCH_SHIFT: {
		AudioEffectPitchShiftConfig* cfg = get_pitch_shift_config();
		ERR_FAIL_NULL_V(cfg, Ref<AudioEffectInstance>());
		AudioEffectPitchShiftState* state = memnew(AudioEffectPitchShiftState);
		state->base = Ref<AudioEffect>(this);
		static const int fft_sizes[AudioEffectPitchShiftConfig::FFT_SIZE_MAX] = { 256, 512, 1024, 2048, 4096 };
		state->fft_size = fft_sizes[cfg->fft_size];
		return AudioEffectInstance::create(state);
	}
	case EFFECT_RECORD: {
		AudioEffectRecordConfig* cfg = get_record_config();
		ERR_FAIL_NULL_V(cfg, Ref<AudioEffectInstance>());
		AudioEffectRecordState* state = memnew(AudioEffectRecordState);
		state->is_recording = false;

		float ring_buffer_max_size = AudioEffectRecordConfig::IO_BUFFER_SIZE_MS;
		ring_buffer_max_size /= 1000.0f;
		ring_buffer_max_size *= AudioServer::get_mix_rate();

		int ringbuff_size = ring_buffer_max_size;
		int bits = 0;
		while (ringbuff_size > 0) {
			bits++;
			ringbuff_size /= 2;
		}
		ringbuff_size = 1 << bits;
		state->ring_buffer_mask = ringbuff_size - 1;
		state->ring_buffer_pos = 0;
		state->ring_buffer.resize(ringbuff_size);
		state->ring_buffer_read_pos = 0;

		cfg->ensure_thread_stopped();
		bool is_currently_recording = false;
		if (cfg->current_instance.is_valid()) {
			if (const AudioEffectRecordState* prev = cfg->current_instance->get_state<AudioEffectRecordState>()) {
				is_currently_recording = prev->is_recording;
			}
		}
		if (is_currently_recording) {
			state->init();
		}
		Ref<AudioEffectInstance> ins = AudioEffectInstance::create(state);
		cfg->current_instance = ins;
		return ins;
	}
	case EFFECT_REVERB: {
		AudioEffectReverbState* state = memnew(AudioEffectReverbState);
		state->base = Ref<AudioEffect>(this);
		return AudioEffectInstance::create(state);
	}
	case EFFECT_SPECTRUM_ANALYZER: {
		AudioEffectSpectrumAnalyzerConfig* cfg = get_spectrum_analyzer_config();
		ERR_FAIL_NULL_V(cfg, Ref<AudioEffectInstance>());
		AudioEffectSpectrumAnalyzerState* state = memnew(AudioEffectSpectrumAnalyzerState);
		state->base = Ref<AudioEffect>(this);
		static const int fft_sizes[AudioEffectSpectrumAnalyzerConfig::FFT_SIZE_MAX] = { 256, 512, 1024, 2048, 4096 };
		state->fft_size = fft_sizes[cfg->fft_size];
		state->mix_rate = AudioServer::get_mix_rate();
		state->fft_count = (cfg->buffer_length / (float(state->fft_size) / state->mix_rate)) + 1;
		state->fft_pos = 0;
		state->fft_history.resize(state->fft_count);
		state->temporal_fft.resize(state->fft_size * 8);
		state->temporal_fft_pos = 0;
		for (int i = 0; i < state->fft_count; i++) {
			state->fft_history.write[i].resize(state->fft_size);
			for (int j = 0; j < state->fft_size; j++) {
				state->fft_history.write[i].write[j] = AudioFrame(0, 0);
			}
		}
		return AudioEffectInstance::create(state);
	}
	case EFFECT_STEREO_ENHANCE: {
		AudioEffectStereoEnhanceState* state = memnew(AudioEffectStereoEnhanceState);
		state->base = Ref<AudioEffect>(this);

		float ring_buffer_max_size = AudioEffectStereoEnhanceState::MAX_DELAY_MS + 2;
		ring_buffer_max_size /= 1000.0f;
		ring_buffer_max_size *= AudioServer::get_mix_rate();

		int ringbuff_size = (int)ring_buffer_max_size;
		int bits = 0;
		while (ringbuff_size > 0) {
			bits++;
			ringbuff_size /= 2;
		}
		ringbuff_size = 1 << bits;
		state->ringbuff_mask = ringbuff_size - 1;
		state->ringbuff_pos = 0;
		state->delay_ringbuff = memnew_arr(float, ringbuff_size);
		return AudioEffectInstance::create(state);
	}
	}
	return Ref<AudioEffectInstance>();
}

// ============================================================================
// Merged from servers/audio/audio_stream.cpp
// ============================================================================

#include "audio_stream.h"
#include "core/config/project_settings.h"
#include "core/math/math_funcs.h"

constexpr uint64_t POLYPHONIC_ID_MASK = 0xFFFFFFFF;
constexpr uint64_t POLYPHONIC_INDEX_SHIFT = 32;

PackedVector2Array AudioStreamPlayback::_mix_audio_bind(float p_rate_scale, int p_frames)
{
	Vector<AudioFrame> frames = mix_audio(p_rate_scale, p_frames);

	PackedVector2Array res;
	res.resize(frames.size());

	Vector2* res_ptrw = res.ptrw();
	for (int i = 0; i < frames.size(); i++) {
		res_ptrw[i] = Vector2(frames[i].left, frames[i].right);
	}

	return res;
}

Vector<AudioFrame> AudioStreamPlayback::mix_audio(float p_rate_scale, int p_frames)
{
	Vector<AudioFrame> res;
	res.resize(p_frames);

	int frames = mix(res.ptrw(), p_rate_scale, p_frames);
	res.resize(frames);

	return res;
}

void AudioStreamPlayback::start_playback(double p_from_pos) { start(p_from_pos); }

void AudioStreamPlayback::stop_playback() { stop(); }

void AudioStreamPlayback::seek_playback(double p_time) { seek(p_time); }

Ref<AudioSamplePlayback> AudioStreamPlayback::get_sample_playback() const { return nullptr; }

AudioStreamPlayback::AudioStreamPlayback() {}

AudioStreamPlayback::~AudioStreamPlayback()
{
	if (get_sample_playback().is_valid() && likely(AudioServer::is_initialized())) {
		AudioServer::stop_sample_playback(get_sample_playback());
	}
}

void AudioStreamPlaybackResampled::begin_resample()
{
	// clear cubic interpolation history
	internal_buffer[0] = AudioFrame(0.0, 0.0);
	internal_buffer[1] = AudioFrame(0.0, 0.0);
	internal_buffer[2] = AudioFrame(0.0, 0.0);
	internal_buffer[3] = AudioFrame(0.0, 0.0);
	// mix buffer
	_mix_internal(internal_buffer + 4, INTERNAL_BUFFER_LEN);
	mix_offset = 0;
}

int AudioStreamPlaybackResampled::mix(AudioFrame* p_buffer, float p_rate_scale, int p_frames)
{
	float target_rate = AudioServer::get_mix_rate();
	float playback_speed_scale = AudioServer::get_playback_speed_scale();

	uint64_t mix_increment = uint64_t(
		((get_stream_sampling_rate() * p_rate_scale * playback_speed_scale) / double(target_rate)) *
		double(FP_LEN));

	int mixed_frames_total = -1;

	int i;
	for (i = 0; i < p_frames; i++) {
		uint32_t idx = CUBIC_INTERP_HISTORY + uint32_t(mix_offset >> FP_BITS);
		// standard cubic interpolation (great quality/performance ratio)
		// this used to be moved to a LUT for greater performance, but nowadays CPU speed is
		// generally faster than memory.
		float mu = (mix_offset & FP_MASK) / float(FP_LEN);
		AudioFrame y0 = internal_buffer[idx - 3];
		AudioFrame y1 = internal_buffer[idx - 2];
		AudioFrame y2 = internal_buffer[idx - 1];
		AudioFrame y3 = internal_buffer[idx - 0];

		if (idx >= internal_buffer_end && mixed_frames_total == -1) {
			// The internal buffer ends somewhere in this range, and we haven't yet recorded the
			// number of good frames we have.
			mixed_frames_total = i;
		}

		float mu2 = mu * mu;
		float h11 = mu2 * (mu - 1);
		float z = mu2 - h11;
		float h01 = z - h11;
		float h10 = mu - z;

		p_buffer[i] = y1 + (y2 - y1) * h01 + ((y2 - y0) * h10 + (y3 - y1) * h11) * 0.5;

		mix_offset += mix_increment;

		while ((mix_offset >> FP_BITS) >= INTERNAL_BUFFER_LEN) {
			internal_buffer[0] = internal_buffer[INTERNAL_BUFFER_LEN + 0];
			internal_buffer[1] = internal_buffer[INTERNAL_BUFFER_LEN + 1];
			internal_buffer[2] = internal_buffer[INTERNAL_BUFFER_LEN + 2];
			internal_buffer[3] = internal_buffer[INTERNAL_BUFFER_LEN + 3];
			int mixed_frames = _mix_internal(internal_buffer + 4, INTERNAL_BUFFER_LEN);
			if (mixed_frames != INTERNAL_BUFFER_LEN) {
				// internal_buffer[mixed_frames] is the first frame of silence.
				internal_buffer_end = mixed_frames;
			}
			else {
				// The internal buffer does not contain the first frame of silence.
				internal_buffer_end = -1;
			}
			mix_offset -= (INTERNAL_BUFFER_LEN << FP_BITS);
		}
	}
	if (mixed_frames_total == -1 && i == p_frames) {
		mixed_frames_total = p_frames;
	}
	return mixed_frames_total;
}

void AudioStream::tag_used(float p_offset)
{
	if (tagged_frame != AudioServer::get_mixed_frames()) {
		offset_count = 0;
		tagged_frame = AudioServer::get_mixed_frames();
	}
	if (offset_count < MAX_TAGGED_OFFSETS) {
		tagged_offsets[offset_count++] = p_offset;
	}
}

uint64_t AudioStream::get_tagged_frame() const { return tagged_frame; }

uint32_t AudioStream::get_tagged_frame_count() const { return offset_count; }

Ref<AudioSample> AudioStream::generate_sample() const
{
	Ref<AudioSample> as = memnew(AudioSample);
	return as;
}

float AudioStream::get_tagged_frame_offset(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, MAX_TAGGED_OFFSETS, 0);
	return tagged_offsets[p_index];
}

double AudioStream::get_bpm() const { return 0.0; }

bool AudioStream::has_loop() const { return false; }

int AudioStream::get_bar_beats() const { return 4; }

int AudioStream::get_beat_count() const { return 0; }

double AudioStream::get_length() const { return 0.0; }

Ref<AudioStreamPlayback> AudioStream::instantiate_playback() { return Ref<AudioStreamPlayback>(); }

String AudioStream::get_stream_name() const { return ""; }

bool AudioStream::is_monophonic() const { return false; }

double AudioStreamPlayback::get_playback_position() const { return 0.0; }

bool AudioStreamPlayback::is_playing() const { return false; }

void AudioStreamPlayback::start(double p_from_pos) {}

void AudioStreamPlayback::stop() {}

int AudioStreamPlaybackResampled::_mix_internal(AudioFrame* p_buffer, int p_frames) { return 0; }

float AudioStreamPlaybackResampled::get_stream_sampling_rate() { return 44100.0; }

float AudioStreamPlaybackResampled::get_stream_sampling_rate() const { return 44100.0; }

int AudioStreamPlayback::get_loop_count() const { return 0; }

void AudioStreamPlayback::seek(double p_time) {}

void AudioStreamPlayback::tag_used_streams() {}

int AudioStreamPlayback::mix(AudioFrame* p_buffer, float p_rate_scale, int p_frames) { return 0; }

///////////////////////////////////////
// AudioStreamGenerator / AudioStreamGeneratorPlayback

void AudioStreamGenerator::set_source_mode(SourceMode p_mode)
{
	ERR_FAIL_UNSIGNED_INDEX(p_mode, 2);
	if (source_mode == p_mode) {
		return;
	}
	AudioServer::lock();
	source_mode = p_mode;
	for (AudioStreamGeneratorPlayback* pb : playbacks) {
		pb->stop();
		if (source_mode == SOURCE_BUFFER && pb->buffer.size() == 0) {
			uint32_t target_buffer_size = _get_target_rate() * buffer_len;
			pb->buffer.resize(Math::nearest_shift(target_buffer_size));
			pb->buffer.clear();
		}
	}
	AudioServer::unlock();
}

AudioStreamGenerator::SourceMode AudioStreamGenerator::get_source_mode() const
{
	return source_mode;
}

void AudioStreamGenerator::set_mix_rate(float p_mix_rate) { mix_rate = p_mix_rate; }

float AudioStreamGenerator::get_mix_rate() const { return mix_rate; }

void AudioStreamGenerator::set_mix_rate_mode(
	AudioStreamGenerator::AudioStreamGeneratorMixRate p_mix_rate_mode)
{
	ERR_FAIL_INDEX(p_mix_rate_mode, AudioStreamGeneratorMixRate::MIX_RATE_MAX);
	mix_rate_mode = p_mix_rate_mode;
}

AudioStreamGenerator::AudioStreamGeneratorMixRate AudioStreamGenerator::get_mix_rate_mode() const
{
	return mix_rate_mode;
}

void AudioStreamGenerator::set_buffer_length(float p_seconds) { buffer_len = p_seconds; }

float AudioStreamGenerator::get_buffer_length() const { return buffer_len; }

float AudioStreamGenerator::_get_target_rate() const
{
	switch (mix_rate_mode) {
	case AudioStreamGeneratorMixRate::MIX_RATE_OUTPUT:
		return AudioServer::get_mix_rate();
	case AudioStreamGeneratorMixRate::MIX_RATE_INPUT:
		return AudioServer::get_input_mix_rate();
	default:
		return mix_rate;
	}
}

Ref<AudioStreamPlayback> AudioStreamGenerator::instantiate_playback()
{
	Ref<AudioStreamGeneratorPlayback> playback;
	playback.instantiate();
	playback->generator = Ref<AudioStreamGenerator>(this);
	playbacks.insert(playback.ptr());
	if (source_mode == SOURCE_BUFFER) {
		uint32_t target_buffer_size = _get_target_rate() * buffer_len;
		playback->buffer.resize(Math::nearest_shift(target_buffer_size));
		playback->buffer.clear();
	}
	return playback;
}

double AudioStreamGenerator::get_length() const { return 0; }

bool AudioStreamGenerator::is_monophonic() const { return true; }

bool AudioStreamGeneratorPlayback::push_frame(const Vector2& p_frame)
{
	if (buffer.space_left() < 1) {
		return false;
	}

	AudioFrame f(p_frame);

	buffer.write(&f, 1);
	return true;
}

bool AudioStreamGeneratorPlayback::can_push_buffer(int p_frames) const
{
	return buffer.space_left() >= p_frames;
}

bool AudioStreamGeneratorPlayback::push_buffer(const PackedVector2Array& p_frames)
{
	int to_write = p_frames.size();
	if (buffer.space_left() < to_write) {
		return false;
	}

	const Vector2* r = p_frames.ptr();
	if constexpr (sizeof(real_t) == 4) {
		// write directly
		buffer.write((const AudioFrame*)r, to_write);
	}
	else {
		// convert from double
		AudioFrame buf[2048];
		int ofs = 0;
		while (to_write) {
			int w = MIN(to_write, 2048);
			for (int i = 0; i < w; i++) {
				buf[i] = AudioFrame(r[i + ofs]);
			}
			buffer.write(buf, w);
			ofs += w;
			to_write -= w;
		}
	}
	return true;
}

int AudioStreamGeneratorPlayback::get_frames_available() const { return buffer.space_left(); }

int AudioStreamGeneratorPlayback::get_skips() const { return skips; }

void AudioStreamGeneratorPlayback::clear_buffer()
{
	ERR_FAIL_COND(active);
	buffer.clear();
	mixed = 0;
}

int AudioStreamGeneratorPlayback::mix(AudioFrame* p_buffer, float p_rate_scale, int p_frames)
{
	return AudioStreamPlaybackResampled::mix(p_buffer, p_rate_scale, p_frames);
}

int AudioStreamGeneratorPlayback::_mix_internal(AudioFrame* p_buffer, int p_frames)
{
	if (!active || generator.is_null()) {
		return 0;
	}

	if (generator->source_mode == AudioStreamGenerator::SOURCE_MICROPHONE) {
		return 0;
	}

	int read_amount = buffer.data_left();
	if (p_frames < read_amount) {
		read_amount = p_frames;
	}

	buffer.read(p_buffer, read_amount);

	if (read_amount < p_frames) {
		// Fill with zeros as fallback in case of buffer underrun.
		for (int i = read_amount; i < p_frames; i++) {
			p_buffer[i] = AudioFrame(0, 0);
		}
		skips++;
	}

	mixed += p_frames / generator->_get_target_rate();
	return p_frames;
}

float AudioStreamGeneratorPlayback::get_stream_sampling_rate()
{
	if (generator.is_null()) {
		return 44100.0f;
	}
	if (generator->source_mode == AudioStreamGenerator::SOURCE_MICROPHONE) {
		return AudioDriverManager::get_active_driver()->get_input_mix_rate();
	}
	return generator->_get_target_rate();
}

void AudioStreamGeneratorPlayback::start(double p_from_pos)
{
	if (generator.is_valid() &&
		generator->source_mode == AudioStreamGenerator::SOURCE_MICROPHONE) {
		if (active) {
			return;
		}
		input_ofs = 0;
		if (AudioServer::set_input_device_active(true) == OK) {
			active = true;
			using_microphone = true;
			begin_resample();
		}
		return;
	}

	if (mixed == 0.0f) {
		begin_resample();
	}
	skips = 0;
	active = true;
	mixed = 0.0f;
}

void AudioStreamGeneratorPlayback::stop()
{
	if (active) {
		if (using_microphone) {
			AudioServer::set_input_device_active(false);
			using_microphone = false;
		}
		active = false;
	}
}

bool AudioStreamGeneratorPlayback::is_playing() const { return active; }

int AudioStreamGeneratorPlayback::get_loop_count() const { return 0; }

double AudioStreamGeneratorPlayback::get_playback_position() const
{
	if (generator.is_valid() &&
		generator->source_mode == AudioStreamGenerator::SOURCE_MICROPHONE) {
		return 0;
	}
	return mixed;
}

void AudioStreamGeneratorPlayback::seek(double p_time)
{
	// No seek possible for generator or microphone input.
}

void AudioStreamGeneratorPlayback::tag_used_streams()
{
	if (generator.is_valid()) {
		generator->tag_used(0);
	}
}

AudioStreamGeneratorPlayback::AudioStreamGeneratorPlayback() {}

AudioStreamGeneratorPlayback::~AudioStreamGeneratorPlayback()
{
	if (generator.is_valid()) {
		generator->playbacks.erase(this);
	}
	stop();
}

///////////////////////////////////////
// AudioStreamPolyphonic / AudioStreamPlaybackPolyphonic

AudioStreamPolyphonic::AudioStreamPolyphonic() {}

void AudioStreamPolyphonic::set_mode(Mode p_mode)
{
	ERR_FAIL_UNSIGNED_INDEX(p_mode, 2);
	mode = p_mode;
}

AudioStreamPolyphonic::Mode AudioStreamPolyphonic::get_mode() const { return mode; }

void AudioStreamPolyphonic::set_polyphony(int p_voices)
{
	ERR_FAIL_COND(p_voices < 0 || p_voices > 128);
	polyphony = p_voices;
}

int AudioStreamPolyphonic::get_polyphony() const { return polyphony; }

void AudioStreamPolyphonic::add_stream(int p_index, Ref<AudioStream> p_stream, float p_weight)
{
	if (p_index < 0) {
		p_index = audio_stream_pool.size();
	}
	ERR_FAIL_COND(p_index > audio_stream_pool.size());
	PoolEntry entry{ p_stream, p_weight };
	audio_stream_pool.insert(p_index, entry);
}

void AudioStreamPolyphonic::move_stream(int p_index_from, int p_index_to)
{
	ERR_FAIL_INDEX(p_index_from, audio_stream_pool.size());
	if (p_index_to < 0) {
		p_index_to = audio_stream_pool.size() - 1;
	}
	ERR_FAIL_INDEX(p_index_to, audio_stream_pool.size());
	PoolEntry entry = audio_stream_pool[p_index_from];
	audio_stream_pool.remove_at(p_index_from);
	audio_stream_pool.insert(p_index_to, entry);
}

void AudioStreamPolyphonic::remove_stream(int p_index)
{
	ERR_FAIL_INDEX(p_index, audio_stream_pool.size());
	audio_stream_pool.remove_at(p_index);
}

void AudioStreamPolyphonic::set_stream(int p_index, Ref<AudioStream> p_stream)
{
	ERR_FAIL_INDEX(p_index, audio_stream_pool.size());
	audio_stream_pool.write[p_index].stream = p_stream;
}

Ref<AudioStream> AudioStreamPolyphonic::get_stream(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, audio_stream_pool.size(), nullptr);
	return audio_stream_pool[p_index].stream;
}

void AudioStreamPolyphonic::set_stream_probability_weight(int p_index, float p_weight)
{
	ERR_FAIL_INDEX(p_index, audio_stream_pool.size());
	audio_stream_pool.write[p_index].weight = p_weight;
}

float AudioStreamPolyphonic::get_stream_probability_weight(int p_index) const
{
	ERR_FAIL_INDEX_V(p_index, audio_stream_pool.size(), 0);
	return audio_stream_pool[p_index].weight;
}

void AudioStreamPolyphonic::set_streams_count(int p_count) { audio_stream_pool.resize(p_count); }

int AudioStreamPolyphonic::get_streams_count() const { return audio_stream_pool.size(); }

void AudioStreamPolyphonic::set_random_pitch(float p_pitch)
{
	if (p_pitch < 1) {
		p_pitch = 1;
	}
	random_pitch_scale = p_pitch;
}

float AudioStreamPolyphonic::get_random_pitch() const { return random_pitch_scale; }

void AudioStreamPolyphonic::set_random_pitch_semitones(float p_semitones)
{
	random_pitch_scale = powf(2, p_semitones * 0.08333333f);
}

float AudioStreamPolyphonic::get_random_pitch_semitones() const
{
	return 12.0f * log2f(MAX(1.0f, random_pitch_scale));
}

void AudioStreamPolyphonic::set_random_volume_offset_db(float p_volume_offset_db)
{
	if (p_volume_offset_db < 0) {
		p_volume_offset_db = 0;
	}
	random_volume_offset_db = p_volume_offset_db;
}

float AudioStreamPolyphonic::get_random_volume_offset_db() const
{
	return random_volume_offset_db;
}

void AudioStreamPolyphonic::set_playback_mode(PlaybackMode p_playback_mode)
{
	playback_mode = p_playback_mode;
}

AudioStreamPolyphonic::PlaybackMode AudioStreamPolyphonic::get_playback_mode() const
{
	return playback_mode;
}

Ref<AudioStreamPlayback> AudioStreamPolyphonic::instance_playback_random()
{
	Ref<AudioStreamPlaybackPolyphonic> playback;
	playback.instantiate();
	playback->mode = MODE_RANDOMIZER;
	playback->parent_stream = Ref<AudioStreamPolyphonic>(this);
	playbacks.insert(playback.ptr());

	double total_weight = 0;
	Vector<PoolEntry> local_pool;
	for (const PoolEntry& entry : audio_stream_pool) {
		if (entry.stream.is_valid() && entry.weight > 0) {
			local_pool.push_back(entry);
			total_weight += entry.weight;
		}
	}
	if (local_pool.is_empty()) {
		return playback;
	}
	double chosen_cumulative_weight = Math::random(0.0, total_weight);
	double cumulative_weight = 0;
	for (PoolEntry& entry : local_pool) {
		cumulative_weight += entry.weight;
		if (cumulative_weight > chosen_cumulative_weight) {
			playback->randomizer_playback = entry.stream->instantiate_playback();
			last_playback = entry.stream;
			break;
		}
	}
	if (playback->randomizer_playback.is_null()) {
		// This indicates a floating point error. Take the last element.
		last_playback = local_pool[local_pool.size() - 1].stream;
		playback->randomizer_playback =
			local_pool.write[local_pool.size() - 1].stream->instantiate_playback();
	}
	return playback;
}

Ref<AudioStreamPlayback> AudioStreamPolyphonic::instance_playback_no_repeats()
{
	Ref<AudioStreamPlaybackPolyphonic> playback;

	double total_weight = 0;
	Vector<PoolEntry> local_pool;
	for (const PoolEntry& entry : audio_stream_pool) {
		if (entry.stream == last_playback) {
			continue;
		}
		if (entry.stream.is_valid() && entry.weight > 0) {
			local_pool.push_back(entry);
			total_weight += entry.weight;
		}
	}
	if (local_pool.is_empty()) {
		// There is only one sound to choose from.
		// Always play a random sound while allowing repeats (which always plays the same sound).
		return instance_playback_random();
	}

	playback.instantiate();
	playback->mode = MODE_RANDOMIZER;
	playback->parent_stream = Ref<AudioStreamPolyphonic>(this);
	playbacks.insert(playback.ptr());

	double chosen_cumulative_weight = Math::random(0.0, total_weight);
	double cumulative_weight = 0;
	for (PoolEntry& entry : local_pool) {
		cumulative_weight += entry.weight;
		if (cumulative_weight > chosen_cumulative_weight) {
			last_playback = entry.stream;
			playback->randomizer_playback = entry.stream->instantiate_playback();
			break;
		}
	}
	if (playback->randomizer_playback.is_null()) {
		// This indicates a floating point error. Take the last element.
		last_playback = local_pool[local_pool.size() - 1].stream;
		playback->randomizer_playback =
			local_pool.write[local_pool.size() - 1].stream->instantiate_playback();
	}
	return playback;
}

Ref<AudioStreamPlayback> AudioStreamPolyphonic::instance_playback_sequential()
{
	Ref<AudioStreamPlaybackPolyphonic> playback;
	playback.instantiate();
	playback->mode = MODE_RANDOMIZER;
	playback->parent_stream = Ref<AudioStreamPolyphonic>(this);
	playbacks.insert(playback.ptr());

	Vector<Ref<AudioStream>> local_pool;
	for (const PoolEntry& entry : audio_stream_pool) {
		if (entry.stream.is_null()) {
			continue;
		}
		if (local_pool.has(entry.stream)) {
			WARN_PRINT("Duplicate stream in sequential playback pool");
			continue;
		}
		local_pool.push_back(entry.stream);
	}
	if (local_pool.is_empty()) {
		return playback;
	}
	bool found_last_stream = false;
	for (Ref<AudioStream>& entry : local_pool) {
		if (found_last_stream) {
			last_playback = entry;
			playback->randomizer_playback = entry->instantiate_playback();
			break;
		}
		if (entry == last_playback) {
			found_last_stream = true;
		}
	}
	if (playback->randomizer_playback.is_null()) {
		// Wrap around
		last_playback = local_pool[0];
		playback->randomizer_playback = local_pool.write[0]->instantiate_playback();
	}
	return playback;
}

Ref<AudioStreamPlayback> AudioStreamPolyphonic::instantiate_playback()
{
	if (mode == MODE_RANDOMIZER) {
		switch (playback_mode) {
		case PLAYBACK_RANDOM:
			return instance_playback_random();
		case PLAYBACK_RANDOM_NO_REPEATS:
			return instance_playback_no_repeats();
		case PLAYBACK_SEQUENTIAL:
			return instance_playback_sequential();
		default:
			ERR_FAIL_V_MSG(nullptr, "Unhandled playback mode.");
		}
	}

	Ref<AudioStreamPlaybackPolyphonic> playback;
	playback.instantiate();
	playback->mode = MODE_POLYPHONIC;
	playback->parent_stream = Ref<AudioStreamPolyphonic>(this);
	playback->streams.resize(polyphony);
	return playback;
}

double AudioStreamPolyphonic::get_length() const
{
	if (mode == MODE_RANDOMIZER) {
		if (!last_playback.is_valid()) {
			return 0;
		}
		return last_playback->get_length();
	}
	return 0.0;
}

bool AudioStreamPolyphonic::is_monophonic() const
{
	if (mode == MODE_RANDOMIZER) {
		for (const PoolEntry& entry : audio_stream_pool) {
			if (entry.stream.is_valid() && entry.stream->is_monophonic()) {
				return true;
			}
		}
		return false;
	}
	return true; // Avoids stream players instantiating more than one polyphonic playback.
}

void AudioStreamPlaybackPolyphonic::start(double p_from_pos)
{
	if (mode == AudioStreamPolyphonic::MODE_RANDOMIZER) {
		randomizer_playing = randomizer_playback;
		if (parent_stream.is_valid()) {
			// GH-10238 : Pitch_scale is multiplicative, so picking a random number for it without
			// log conversion will bias it towards higher pitches (0.5 is down one octave, 2.0 is up
			// one octave).
			float range_from = Math::log(1.0f / parent_stream->random_pitch_scale);
			float range_to = Math::log(parent_stream->random_pitch_scale);

			pitch_scale = Math::exp(range_from + Math::randf() * (range_to - range_from));

			float vol_from = -parent_stream->random_volume_offset_db;
			float vol_to = parent_stream->random_volume_offset_db;

			float volume_offset_db = vol_from + Math::randf() * (vol_to - vol_from);
			volume_scale = Math::db_to_linear(volume_offset_db);
		}
		else {
			pitch_scale = 1.0f;
			volume_scale = 1.0f;
		}

		if (randomizer_playing.is_valid()) {
			randomizer_playing->start(p_from_pos);
		}
		return;
	}

	if (active) {
		stop();
	}

	active = true;
}

void AudioStreamPlaybackPolyphonic::stop()
{
	if (mode == AudioStreamPolyphonic::MODE_RANDOMIZER) {
		if (randomizer_playing.is_valid()) {
			randomizer_playing->stop();
		}
		return;
	}

	if (!active) {
		return;
	}

	bool locked = false;
	for (Stream& s : streams) {
		if (s.active.is_set()) {
			// Need locking because something may still be mixing.
			if (!locked) {
				locked = true;
				AudioServer::lock();
			}
		}
		s.active.clear();
		s.finish_request.clear();
		s.stream_playback.unref();
		s.stream.unref();
	}
	if (locked) {
		AudioServer::unlock();
	}

	active = false;
}

bool AudioStreamPlaybackPolyphonic::is_playing() const
{
	if (mode == AudioStreamPolyphonic::MODE_RANDOMIZER) {
		if (randomizer_playing.is_valid()) {
			return randomizer_playing->is_playing();
		}
		return false;
	}
	return active;
}

int AudioStreamPlaybackPolyphonic::get_loop_count() const
{
	if (mode == AudioStreamPolyphonic::MODE_RANDOMIZER) {
		if (randomizer_playing.is_valid()) {
			return randomizer_playing->get_loop_count();
		}
		return 0;
	}
	return 0;
}

double AudioStreamPlaybackPolyphonic::get_playback_position() const
{
	if (mode == AudioStreamPolyphonic::MODE_RANDOMIZER) {
		if (randomizer_playing.is_valid()) {
			return randomizer_playing->get_playback_position();
		}
		return 0;
	}
	return 0;
}

void AudioStreamPlaybackPolyphonic::seek(double p_time)
{
	if (mode == AudioStreamPolyphonic::MODE_RANDOMIZER) {
		if (randomizer_playing.is_valid()) {
			randomizer_playing->seek(p_time);
		}
		return;
	}
	// Ignored in polyphonic mode.
}

void AudioStreamPlaybackPolyphonic::tag_used_streams()
{
	if (mode == AudioStreamPolyphonic::MODE_RANDOMIZER) {
		Ref<AudioStreamPlayback> p = randomizer_playing; // Thread safety
		if (p.is_valid()) {
			p->tag_used_streams();
		}
		if (parent_stream.is_valid()) {
			parent_stream->tag_used(0);
		}
		return;
	}

	for (Stream& s : streams) {
		if (s.active.is_set()) {
			s.stream_playback->tag_used_streams();
		}
	}
}

int AudioStreamPlaybackPolyphonic::mix(AudioFrame* p_buffer, float p_rate_scale, int p_frames)
{
	if (mode == AudioStreamPolyphonic::MODE_RANDOMIZER) {
		if (randomizer_playing.is_valid()) {
			int mixed_samples =
				randomizer_playing->mix(p_buffer, p_rate_scale * pitch_scale, p_frames);
			for (int samp = 0; samp < mixed_samples; samp++) {
				p_buffer[samp] *= volume_scale;
			}
			return mixed_samples;
		}
		else {
			for (int i = 0; i < p_frames; i++) {
				p_buffer[i] = AudioFrame(0, 0);
			}
			return p_frames;
		}
	}

	if (!active) {
		return 0;
	}

	// Pre-clear buffer.
	for (int i = 0; i < p_frames; i++) {
		p_buffer[i] = AudioFrame(0, 0);
	}

	for (Stream& s : streams) {
		if (!s.active.is_set()) {
			continue;
		}

		if (s.stream_playback->get_is_sample()) {
			if (s.finish_request.is_set()) {
				s.active.clear();
				AudioServer::stop_sample_playback(s.stream_playback->get_sample_playback());
			}
			continue;
		}

		float volume_db = s.volume_db; // Copy because it can be overridden at any time.
		float next_volume = Math::db_to_linear(volume_db);
		s.prev_volume_db = volume_db;

		if (s.finish_request.is_set()) {
			if (s.pending_play.is_set()) {
				// Did not get the chance to play, was finalized too soon.
				s.active.clear();
				continue;
			}
			next_volume = 0;
		}

		if (s.pending_play.is_set()) {
			s.stream_playback->start(s.play_offset);
			s.pending_play.clear();
		}
		float prev_volume = Math::db_to_linear(s.prev_volume_db);

		float volume_inc = (next_volume - prev_volume) / float(p_frames);

		int todo = p_frames;
		int offset = 0;
		float volume = prev_volume;

		bool stream_done = false;

		while (todo) {
			int to_mix = MIN(todo, int(INTERNAL_BUFFER_LEN));
			int mixed =
				s.stream_playback->mix(internal_buffer, p_rate_scale * s.pitch_scale, to_mix);

			for (int i = 0; i < to_mix; i++) {
				p_buffer[offset + i] += internal_buffer[i] * volume;
				volume += volume_inc;
			}

			if (mixed < to_mix) {
				// Stream is done.
				s.active.clear();
				stream_done = true;
				break;
			}

			todo -= to_mix;
			offset += to_mix;
		}

		if (stream_done) {
			continue;
		}

		if (s.finish_request.is_set()) {
			s.active.clear();
		}
	}

	return p_frames;
}

AudioStreamPlaybackPolyphonic::ID AudioStreamPlaybackPolyphonic::play_stream(
	const Ref<AudioStream>& p_stream, float p_from_offset, float p_volume_db, float p_pitch_scale,
	AudioServer::PlaybackType p_playback_type, const String& p_bus)
{
	ERR_FAIL_COND_V(p_stream.is_null(), INVALID_ID);

	AudioServer::PlaybackType pb_type =
		p_playback_type == AudioServer::PlaybackType::PLAYBACK_TYPE_DEFAULT
			? AudioServer::get_default_playback_type()
			: p_playback_type;

	for (uint32_t i = 0; i < streams.size(); i++) {
		if (streams[i].active.is_set() && streams[i].stream_playback->get_is_sample()) {
			Ref<AudioSamplePlayback> active_sample_playback =
				streams[i].stream_playback->get_sample_playback();
			if (active_sample_playback.is_null() ||
				!AudioServer::is_sample_playback_active(active_sample_playback)) {
				streams[i].active.clear();
			}
		}

		if (!streams[i].active.is_set()) {
			// Can use this stream, as it's not active.
			streams[i].stream = p_stream;
			streams[i].stream_playback = streams[i].stream->instantiate_playback();
			streams[i].play_offset = p_from_offset;
			streams[i].volume_db = p_volume_db;
			streams[i].prev_volume_db = p_volume_db;
			streams[i].pitch_scale = p_pitch_scale;
			streams[i].id = id_counter++;
			streams[i].finish_request.clear();
			streams[i].pending_play.set();
			streams[i].active.set();

			// Sample playback.
			if (pb_type == AudioServer::PlaybackType::PLAYBACK_TYPE_SAMPLE &&
				p_stream->can_be_sampled()) {
				streams[i].stream_playback->set_is_sample(true);
				if (!AudioServer::is_stream_registered_as_sample(p_stream)) {
					AudioServer::register_stream_as_sample(p_stream);
				}
				float linear_volume = Math::db_to_linear(p_volume_db);
				Ref<AudioSamplePlayback> sp;
				sp.instantiate();
				sp->stream = streams[i].stream;
				sp->offset = p_from_offset;
				sp->volume_vector.resize(4);
				sp->volume_vector.write[0] = AudioFrame(linear_volume, linear_volume);
				sp->volume_vector.write[1] = AudioFrame(linear_volume, /* LFE= */ 1.0f);
				sp->volume_vector.write[2] = AudioFrame(linear_volume, linear_volume);
				sp->volume_vector.write[3] = AudioFrame(linear_volume, linear_volume);
				sp->bus = p_bus;

				if (streams[i].stream_playback->get_sample_playback().is_valid()) {
					AudioServer::stop_playback_stream(streams[i].stream_playback);
				}

				streams[i].stream_playback->set_sample_playback(sp);
				AudioServer::start_sample_playback(sp);
			}

			return (ID(i) << POLYPHONIC_INDEX_SHIFT) | ID(streams[i].id);
		}
	}

	return INVALID_ID;
}

AudioStreamPlaybackPolyphonic::Stream* AudioStreamPlaybackPolyphonic::_find_stream(int64_t p_id)
{
	uint32_t index = static_cast<uint64_t>(p_id) >> POLYPHONIC_INDEX_SHIFT;
	if (index >= streams.size()) {
		return nullptr;
	}
	if (!streams[index].active.is_set()) {
		return nullptr; // Not active, no longer exists.
	}
	int64_t id = static_cast<uint64_t>(p_id) & POLYPHONIC_ID_MASK;
	if (streams[index].id != id) {
		return nullptr;
	}
	return &streams[index];
}

const AudioStreamPlaybackPolyphonic::Stream* AudioStreamPlaybackPolyphonic::_find_stream(
	int64_t p_id) const
{
	uint32_t index = static_cast<uint64_t>(p_id) >> POLYPHONIC_INDEX_SHIFT;
	if (index >= streams.size()) {
		return nullptr;
	}
	if (!streams[index].active.is_set()) {
		return nullptr; // Not active, no longer exists.
	}
	int64_t id = static_cast<uint64_t>(p_id) & POLYPHONIC_ID_MASK;
	if (streams[index].id != id) {
		return nullptr;
	}
	return &streams[index];
}

void AudioStreamPlaybackPolyphonic::set_stream_volume(ID p_stream_id, float p_volume_db)
{
	Stream* s = _find_stream(p_stream_id);
	if (!s) {
		return;
	}
	s->volume_db = p_volume_db;
}

void AudioStreamPlaybackPolyphonic::set_stream_pitch_scale(ID p_stream_id, float p_pitch_scale)
{
	Stream* s = _find_stream(p_stream_id);
	if (!s) {
		return;
	}
	s->pitch_scale = p_pitch_scale;
}

bool AudioStreamPlaybackPolyphonic::is_stream_playing(ID p_stream_id) const
{
	return _find_stream(p_stream_id) != nullptr;
}

void AudioStreamPlaybackPolyphonic::stop_stream(ID p_stream_id)
{
	Stream* s = _find_stream(p_stream_id);
	if (!s) {
		return;
	}
	s->finish_request.set();
}

void AudioStreamPlaybackPolyphonic::set_is_sample(bool p_is_sample) { _is_sample = p_is_sample; }

bool AudioStreamPlaybackPolyphonic::get_is_sample() const { return _is_sample; }

Ref<AudioSamplePlayback> AudioStreamPlaybackPolyphonic::get_sample_playback() const
{
	return sample_playback;
}

void AudioStreamPlaybackPolyphonic::set_sample_playback(const Ref<AudioSamplePlayback>& p_playback)
{
	sample_playback = p_playback;
	if (sample_playback.is_valid()) {
		sample_playback->stream_playback = Ref<AudioStreamPlayback>(this);
	}
}

AudioStreamPlaybackPolyphonic::AudioStreamPlaybackPolyphonic() {}

AudioStreamPlaybackPolyphonic::~AudioStreamPlaybackPolyphonic()
{
	if (mode == AudioStreamPolyphonic::MODE_RANDOMIZER && parent_stream.is_valid()) {
		parent_stream->playbacks.erase(this);
	}
}

// ============================================================================
// Merged from servers/audio/effects/audio_effects.cpp
// ============================================================================


#include "core/io/marshalls.h"
#include "core/math/math_funcs.h"
#include "core/os/os.h"
#include "servers/audio/audio_server.h"

#include <cmath>
#include <cstring>

// ============================================================================
// 1. Amplify
// ============================================================================

void AudioEffectAmplifyState::process(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	const AudioEffectAmplifyConfig* cfg = base.is_valid() ? base->get_amplify_config() : nullptr;
	ERR_FAIL_NULL(cfg);

	float volume_db = cfg->volume_db;
	float vol = Math::db_to_linear(mix_volume_db);
	float vol_inc = (Math::db_to_linear(volume_db) - vol) / float(p_frame_count);

	for (int i = 0; i < p_frame_count; i++) {
		p_dst_frames[i] = p_src_frames[i] * vol;
		vol += vol_inc;
	}
	mix_volume_db = volume_db;
}

void AudioEffectAmplifyConfig::set_volume_db(float p_volume) { volume_db = p_volume; }

float AudioEffectAmplifyConfig::get_volume_db() const { return volume_db; }

void AudioEffectAmplifyConfig::set_volume_linear(float p_volume)
{
	set_volume_db(Math::linear_to_db(p_volume));
}

float AudioEffectAmplifyConfig::get_volume_linear() const
{
	return Math::db_to_linear(get_volume_db());
}

// ============================================================================
// 2. Capture
// ============================================================================

bool AudioEffectCaptureConfig::can_get_buffer(int p_frames) const
{
	return buffer.data_left() >= p_frames;
}

PackedVector2Array AudioEffectCaptureConfig::get_buffer(int p_frames)
{
	ERR_FAIL_COND_V(!buffer_initialized, PackedVector2Array());
	ERR_FAIL_INDEX_V(p_frames, buffer.size(), PackedVector2Array());
	int data_left = buffer.data_left();
	if (data_left < p_frames || p_frames == 0) {
		return PackedVector2Array();
	}

	PackedVector2Array ret;
	ret.resize(p_frames);

	Vector<AudioFrame> streaming_data;
	streaming_data.resize(p_frames);
	buffer.read(streaming_data.ptrw(), p_frames);
	for (int32_t i = 0; i < p_frames; i++) {
		ret.write[i] = Vector2(streaming_data[i].left, streaming_data[i].right);
	}
	return ret;
}

void AudioEffectCaptureConfig::clear_buffer()
{
	const int32_t data_left = buffer.data_left();
	buffer.advance_read(data_left);
}

void AudioEffectCaptureConfig::set_buffer_length(float p_buffer_length_seconds)
{
	buffer_length_seconds = p_buffer_length_seconds;
}

float AudioEffectCaptureConfig::get_buffer_length() const { return buffer_length_seconds; }

int AudioEffectCaptureConfig::get_frames_available() const
{
	ERR_FAIL_COND_V(!buffer_initialized, 0);
	return buffer.data_left();
}

int64_t AudioEffectCaptureConfig::get_discarded_frames() const { return discarded_frames.get(); }

int AudioEffectCaptureConfig::get_buffer_length_frames() const
{
	ERR_FAIL_COND_V(!buffer_initialized, 0);
	return buffer.size();
}

int64_t AudioEffectCaptureConfig::get_pushed_frames() const { return pushed_frames.get(); }

void AudioEffectCaptureState::process(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	AudioEffectCaptureConfig* cfg = base.is_valid() ? base->get_capture_config() : nullptr;
	ERR_FAIL_NULL(cfg);

	RingBuffer<AudioFrame>& buffer = cfg->buffer;

	for (int i = 0; i < p_frame_count; i++) {
		p_dst_frames[i] = p_src_frames[i];
	}

	if (buffer.space_left() >= p_frame_count) {
		int32_t ret = buffer.write(p_src_frames, p_frame_count);
		ERR_FAIL_COND_MSG(ret != p_frame_count,
			"Failed to add data to effect capture ring buffer despite sufficient space.");
		cfg->pushed_frames.add(p_frame_count);
	}
	else {
		cfg->discarded_frames.add(p_frame_count);
	}
}

bool AudioEffectCaptureState::process_silence() const { return true; }

// ============================================================================
// 3. Chorus
// ============================================================================

void AudioEffectChorusState::process(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	int todo = p_frame_count;

	while (todo) {
		int to_mix = MIN(todo, 256);

		_process_chunk(p_src_frames, p_dst_frames, to_mix);

		p_src_frames += to_mix;
		p_dst_frames += to_mix;

		todo -= to_mix;
	}
}

void AudioEffectChorusState::_process_chunk(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	const AudioEffectChorusConfig* cfg = base.is_valid() ? base->get_chorus_config() : nullptr;
	ERR_FAIL_NULL(cfg);

	for (int i = 0; i < p_frame_count; i++) {
		audio_buffer.write[(buffer_pos + i) & buffer_mask] = p_src_frames[i];
		p_dst_frames[i] = p_src_frames[i] * cfg->dry;
	}

	float mix_rate = AudioServer::get_mix_rate();

	for (int vc = 0; vc < cfg->voice_count; vc++) {
		const AudioEffectChorusConfig::Voice& v = cfg->voice[vc];

		double time_to_mix = (float)p_frame_count / mix_rate;
		double cycles_to_mix = time_to_mix * v.rate;

		unsigned int local_rb_pos = buffer_pos;
		AudioFrame* dst_buff = p_dst_frames;
		AudioFrame* rb_buff = audio_buffer.ptrw();

		double delay_msec = v.delay;
		unsigned int delay_frames = Math::fast_ftoi((delay_msec / 1000.0) * mix_rate);
		float max_depth_frames = (v.depth / 1000.0) * mix_rate;

		uint64_t local_cycles = cycles[vc];
		uint64_t increment = std::rint(
			cycles_to_mix / (double)p_frame_count * (double)(1 << AudioEffectChorusConfig::CYCLES_FRAC));

		if ((((unsigned int)max_depth_frames) + 10) > delay_frames) {
			delay_frames += (int)max_depth_frames - delay_frames;
			delay_frames += 10;
		}

		if (v.cutoff == 0) {
			continue;
		}
		float auxlp = std::exp(-Math::TAU * v.cutoff / mix_rate);
		float c1 = 1.0 - auxlp;
		float c2 = auxlp;
		AudioFrame h = filter_h[vc];
		if (v.cutoff >= AudioEffectChorusConfig::MS_CUTOFF_MAX) {
			c1 = 1.0;
			c2 = 0.0;
		}

		AudioFrame vol_modifier = AudioFrame(cfg->wet, cfg->wet) * Math::db_to_linear(v.level);
		vol_modifier.left *= CLAMP(1.0 - v.pan, 0, 1);
		vol_modifier.right *= CLAMP(1.0 + v.pan, 0, 1);

		for (int i = 0; i < p_frame_count; i++) {
			float phase = (float)(local_cycles & AudioEffectChorusConfig::CYCLES_MASK) /
						  (float)(1 << AudioEffectChorusConfig::CYCLES_FRAC);

			float wave_delay = std::sin(phase * Math::TAU) * max_depth_frames;

			int wave_delay_frames = std::rint(std::floor(wave_delay));
			float wave_delay_frac = wave_delay - (float)wave_delay_frames;

			unsigned int rb_source = local_rb_pos;
			rb_source -= delay_frames;
			rb_source -= wave_delay_frames;

			AudioFrame val = rb_buff[rb_source & buffer_mask];
			AudioFrame val_next = rb_buff[(rb_source - 1) & buffer_mask];

			val += (val_next - val) * wave_delay_frac;

			val = val * c1 + h * c2;
			h = val;

			dst_buff[i] += val * vol_modifier;

			local_cycles += increment;
			local_rb_pos++;
		}

		filter_h[vc] = h;
		cycles[vc] +=
			Math::fast_ftoi(cycles_to_mix * (double)(1 << AudioEffectChorusConfig::CYCLES_FRAC));
	}

	buffer_pos += p_frame_count;
}

void AudioEffectChorusConfig::set_voice_count(int p_voices)
{
	ERR_FAIL_COND(p_voices < 1 || p_voices > MAX_VOICES);
	voice_count = p_voices;
}

int AudioEffectChorusConfig::get_voice_count() const { return voice_count; }

void AudioEffectChorusConfig::set_voice_delay_ms(int p_voice, float p_delay_ms)
{
	ERR_FAIL_INDEX(p_voice, MAX_VOICES);
	voice[p_voice].delay = p_delay_ms;
}

float AudioEffectChorusConfig::get_voice_delay_ms(int p_voice) const
{
	ERR_FAIL_INDEX_V(p_voice, MAX_VOICES, 0);
	return voice[p_voice].delay;
}

void AudioEffectChorusConfig::set_voice_rate_hz(int p_voice, float p_rate_hz)
{
	ERR_FAIL_INDEX(p_voice, MAX_VOICES);
	voice[p_voice].rate = p_rate_hz;
}

float AudioEffectChorusConfig::get_voice_rate_hz(int p_voice) const
{
	ERR_FAIL_INDEX_V(p_voice, MAX_VOICES, 0);
	return voice[p_voice].rate;
}

void AudioEffectChorusConfig::set_voice_depth_ms(int p_voice, float p_depth_ms)
{
	ERR_FAIL_INDEX(p_voice, MAX_VOICES);
	voice[p_voice].depth = p_depth_ms;
}

float AudioEffectChorusConfig::get_voice_depth_ms(int p_voice) const
{
	ERR_FAIL_INDEX_V(p_voice, MAX_VOICES, 0);
	return voice[p_voice].depth;
}

void AudioEffectChorusConfig::set_voice_level_db(int p_voice, float p_level_db)
{
	ERR_FAIL_INDEX(p_voice, MAX_VOICES);
	voice[p_voice].level = p_level_db;
}

float AudioEffectChorusConfig::get_voice_level_db(int p_voice) const
{
	ERR_FAIL_INDEX_V(p_voice, MAX_VOICES, 0);
	return voice[p_voice].level;
}

void AudioEffectChorusConfig::set_voice_cutoff_hz(int p_voice, float p_cutoff_hz)
{
	ERR_FAIL_INDEX(p_voice, MAX_VOICES);
	voice[p_voice].cutoff = MAX(p_cutoff_hz, 1.0f);
}

float AudioEffectChorusConfig::get_voice_cutoff_hz(int p_voice) const
{
	ERR_FAIL_INDEX_V(p_voice, MAX_VOICES, 0);
	return voice[p_voice].cutoff;
}

void AudioEffectChorusConfig::set_voice_pan(int p_voice, float p_pan)
{
	ERR_FAIL_INDEX(p_voice, MAX_VOICES);
	voice[p_voice].pan = p_pan;
}

float AudioEffectChorusConfig::get_voice_pan(int p_voice) const
{
	ERR_FAIL_INDEX_V(p_voice, MAX_VOICES, 0);
	return voice[p_voice].pan;
}

void AudioEffectChorusConfig::set_wet(float p_amount) { wet = p_amount; }

float AudioEffectChorusConfig::get_wet() const { return wet; }

void AudioEffectChorusConfig::set_dry(float p_amount) { dry = p_amount; }

float AudioEffectChorusConfig::get_dry() const { return dry; }

AudioEffectChorusConfig::AudioEffectChorusConfig()
{
	voice_count = 2;
	voice[0].delay = 15.0f;
	voice[1].delay = 20.0f;
	voice[0].rate = 0.8f;
	voice[1].rate = 1.2f;
	voice[0].depth = 2.0f;
	voice[1].depth = 3.0f;
	voice[0].cutoff = 8000.0f;
	voice[1].cutoff = 8000.0f;
	voice[0].pan = -0.5f;
	voice[1].pan = 0.5f;
	wet = 0.5f;
	dry = 1.0f;
}

// ============================================================================
// 4. Compressor
// ============================================================================

void AudioEffectCompressorState::process(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	const AudioEffectCompressorConfig* cfg = base.is_valid() ? base->get_compressor_config() : nullptr;
	ERR_FAIL_NULL(cfg);

	float threshold = Math::db_to_linear(cfg->threshold);
	float sample_rate = AudioServer::get_mix_rate();

	float ratatcoef = std::exp(-1 / (0.00001f * sample_rate));
	float ratrelcoef = std::exp(-1 / (0.5f * sample_rate));
	float attime = cfg->attack_us / 1000000.0f;
	float reltime = cfg->release_ms / 1000.0f;
	float atcoef = std::exp(-1 / (attime * sample_rate));
	float relcoef = std::exp(-1 / (reltime * sample_rate));

	float makeup = Math::db_to_linear(cfg->gain);

	float mix = cfg->mix;
	float gr_meter_decay = std::exp(1 / (1 * sample_rate));

	const AudioFrame* src = p_src_frames;

	if (cfg->sidechain != String() && current_channel != -1) {
		int bus = AudioServer::thread_find_bus_index(cfg->sidechain);
		if (bus >= 0) {
			src = AudioServer::thread_get_channel_mix_buffer(bus, current_channel);
		}
	}

	for (int i = 0; i < p_frame_count; i++) {
		AudioFrame s = src[i];
		s.left = Math::abs(s.left);
		s.right = Math::abs(s.right);

		float peak = MAX(s.left, s.right);
		float overdb = 2.08136898f * Math::linear_to_db(peak / threshold);

		if (overdb < 0.0f) {
			overdb = 0.0f;
		}

		if (overdb - rundb > 5) {
			averatio = 4;
		}

		if (overdb > rundb) {
			rundb = overdb + atcoef * (rundb - overdb);
			runratio = averatio + ratatcoef * (runratio - averatio);
		}
		else {
			rundb = overdb + relcoef * (rundb - overdb);
			runratio = averatio + ratrelcoef * (runratio - averatio);
		}

		overdb = rundb;
		averatio = runratio;

		float cratio = cfg->ratio;
		float gr = -overdb * (cratio - 1) / cratio;
		float grv = Math::db_to_linear(gr);

		runmax = maxover + relcoef * (runmax - maxover);
		maxover = runmax;

		if (grv < gr_meter) {
			gr_meter = grv;
		}
		else {
			gr_meter *= gr_meter_decay;
			if (gr_meter > 1) {
				gr_meter = 1;
			}
		}

		p_dst_frames[i] = p_src_frames[i] * grv * makeup * mix + p_src_frames[i] * (1.0f - mix);
	}
}

void AudioEffectCompressorConfig::set_threshold(float p_threshold) { threshold = p_threshold; }

float AudioEffectCompressorConfig::get_threshold() const { return threshold; }

void AudioEffectCompressorConfig::set_ratio(float p_ratio) { ratio = p_ratio; }

float AudioEffectCompressorConfig::get_ratio() const { return ratio; }

void AudioEffectCompressorConfig::set_gain(float p_gain) { gain = p_gain; }

float AudioEffectCompressorConfig::get_gain() const { return gain; }

void AudioEffectCompressorConfig::set_attack_us(float p_attack_us) { attack_us = p_attack_us; }

float AudioEffectCompressorConfig::get_attack_us() const { return attack_us; }

void AudioEffectCompressorConfig::set_release_ms(float p_release_ms) { release_ms = p_release_ms; }

float AudioEffectCompressorConfig::get_release_ms() const { return release_ms; }

void AudioEffectCompressorConfig::set_mix(float p_mix) { mix = p_mix; }

float AudioEffectCompressorConfig::get_mix() const { return mix; }

void AudioEffectCompressorConfig::set_sidechain(const String& p_sidechain)
{
	AudioServer::lock();
	sidechain = p_sidechain;
	AudioServer::unlock();
}

String AudioEffectCompressorConfig::get_sidechain() const { return sidechain; }

// ============================================================================
// 5. Delay
// ============================================================================

void AudioEffectDelayState::process(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	int todo = p_frame_count;

	while (todo) {
		int to_mix = MIN(todo, 256);

		_process_chunk(p_src_frames, p_dst_frames, to_mix);

		p_src_frames += to_mix;
		p_dst_frames += to_mix;

		todo -= to_mix;
	}
}

void AudioEffectDelayState::_process_chunk(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	const AudioEffectDelayConfig* cfg = base.is_valid() ? base->get_delay_config() : nullptr;
	ERR_FAIL_NULL(cfg);

	float main_level_f = cfg->dry;
	float mix_rate = AudioServer::get_mix_rate();

	float tap_1_level_f = cfg->tap_1_active ? Math::db_to_linear(cfg->tap_1_level) : 0.0f;
	int tap_1_delay_frames = int((cfg->tap_1_delay_ms / 1000.0f) * mix_rate);

	float tap_2_level_f = cfg->tap_2_active ? Math::db_to_linear(cfg->tap_2_level) : 0.0f;
	int tap_2_delay_frames = int((cfg->tap_2_delay_ms / 1000.0f) * mix_rate);

	float feedback_level_f = cfg->feedback_active ? Math::db_to_linear(cfg->feedback_level) : 0.0f;
	unsigned int feedback_delay_frames = int((cfg->feedback_delay_ms / 1000.0f) * mix_rate);

	AudioFrame tap1_vol = AudioFrame(tap_1_level_f, tap_1_level_f);
	tap1_vol.left *= CLAMP(1.0f - cfg->tap_1_pan, 0.0f, 1.0f);
	tap1_vol.right *= CLAMP(1.0f + cfg->tap_1_pan, 0.0f, 1.0f);

	AudioFrame tap2_vol = AudioFrame(tap_2_level_f, tap_2_level_f);
	tap2_vol.left *= CLAMP(1.0f - cfg->tap_2_pan, 0.0f, 1.0f);
	tap2_vol.right *= CLAMP(1.0f + cfg->tap_2_pan, 0.0f, 1.0f);

	float lpf_c = std::exp(-Math::TAU * cfg->feedback_lowpass / mix_rate);
	float lpf_ic = 1.0f - lpf_c;

	const AudioFrame* src = p_src_frames;
	AudioFrame* dst = p_dst_frames;
	AudioFrame* rb_buf = ring_buffer.ptrw();
	AudioFrame* fb_buf = feedback_buffer.ptrw();

	for (int i = 0; i < p_frame_count; i++) {
		rb_buf[ring_buffer_pos & ring_buffer_mask] = src[i];

		AudioFrame main_val = src[i] * main_level_f;
		AudioFrame tap_1_val =
			rb_buf[(ring_buffer_pos - tap_1_delay_frames) & ring_buffer_mask] * tap1_vol;
		AudioFrame tap_2_val =
			rb_buf[(ring_buffer_pos - tap_2_delay_frames) & ring_buffer_mask] * tap2_vol;

		AudioFrame out = main_val + tap_1_val + tap_2_val;
		out += fb_buf[feedback_buffer_pos];

		AudioFrame fb_in = out * feedback_level_f * lpf_ic + h * lpf_c;
		fb_in.undenormalize();

		h = fb_in;
		fb_buf[feedback_buffer_pos] = fb_in;

		dst[i] = out;

		ring_buffer_pos++;

		if ((++feedback_buffer_pos) >= feedback_delay_frames) {
			feedback_buffer_pos = 0;
		}
	}
}

void AudioEffectDelayConfig::set_dry(float p_dry) { dry = p_dry; }

float AudioEffectDelayConfig::get_dry() const { return dry; }

void AudioEffectDelayConfig::set_tap1_active(bool p_active) { tap_1_active = p_active; }

bool AudioEffectDelayConfig::is_tap1_active() const { return tap_1_active; }

void AudioEffectDelayConfig::set_tap1_delay_ms(float p_delay_ms) { tap_1_delay_ms = p_delay_ms; }

float AudioEffectDelayConfig::get_tap1_delay_ms() const { return tap_1_delay_ms; }

void AudioEffectDelayConfig::set_tap1_level_db(float p_level_db) { tap_1_level = p_level_db; }

float AudioEffectDelayConfig::get_tap1_level_db() const { return tap_1_level; }

void AudioEffectDelayConfig::set_tap1_pan(float p_pan) { tap_1_pan = p_pan; }

float AudioEffectDelayConfig::get_tap1_pan() const { return tap_1_pan; }

void AudioEffectDelayConfig::set_tap2_active(bool p_active) { tap_2_active = p_active; }

bool AudioEffectDelayConfig::is_tap2_active() const { return tap_2_active; }

void AudioEffectDelayConfig::set_tap2_delay_ms(float p_delay_ms) { tap_2_delay_ms = p_delay_ms; }

float AudioEffectDelayConfig::get_tap2_delay_ms() const { return tap_2_delay_ms; }

void AudioEffectDelayConfig::set_tap2_level_db(float p_level_db) { tap_2_level = p_level_db; }

float AudioEffectDelayConfig::get_tap2_level_db() const { return tap_2_level; }

void AudioEffectDelayConfig::set_tap2_pan(float p_pan) { tap_2_pan = p_pan; }

float AudioEffectDelayConfig::get_tap2_pan() const { return tap_2_pan; }

void AudioEffectDelayConfig::set_feedback_active(bool p_active) { feedback_active = p_active; }

bool AudioEffectDelayConfig::is_feedback_active() const { return feedback_active; }

void AudioEffectDelayConfig::set_feedback_delay_ms(float p_delay_ms) { feedback_delay_ms = p_delay_ms; }

float AudioEffectDelayConfig::get_feedback_delay_ms() const { return feedback_delay_ms; }

void AudioEffectDelayConfig::set_feedback_level_db(float p_level_db) { feedback_level = p_level_db; }

float AudioEffectDelayConfig::get_feedback_level_db() const { return feedback_level; }

void AudioEffectDelayConfig::set_feedback_lowpass(float p_lowpass)
{
	feedback_lowpass = MAX(p_lowpass, 1.0f);
}

float AudioEffectDelayConfig::get_feedback_lowpass() const { return feedback_lowpass; }

// ============================================================================
// 6. Distortion
// ============================================================================

void AudioEffectDistortionState::process(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	const AudioEffectDistortionConfig* cfg = base.is_valid() ? base->get_distortion_config() : nullptr;
	ERR_FAIL_NULL(cfg);

	const float* src = (const float*)p_src_frames;
	float* dst = (float*)p_dst_frames;

	float lpf_c = std::exp(-Math::TAU * cfg->keep_hf_hz / AudioServer::get_mix_rate());
	float lpf_ic = 1.0f - lpf_c;

	float drive_f = cfg->drive;
	float pregain_f = Math::db_to_linear(cfg->pre_gain);
	float postgain_f = Math::db_to_linear(cfg->post_gain);

	float atan_mult = std::pow(10.0f, drive_f * drive_f * 3.0f) - 1.0f + 0.001f;
	float atan_div = 1.0f / (std::atan(atan_mult) * (1.0f + drive_f * 8.0f));

	float lofi_mult = std::pow(2.0f, 2.0f + (1.0f - drive_f) * 14.0f);

	for (int i = 0; i < p_frame_count * 2; i++) {
		float out = undenormalize(src[i] * lpf_ic + lpf_c * h[i & 1]);
		h[i & 1] = out;
		float a = out;
		float ha = src[i] - out;
		a *= pregain_f;

		switch (cfg->mode) {
		case AudioEffectDistortionConfig::MODE_CLIP: {
			float a_sign = a < 0 ? -1.0f : 1.0f;
			a = std::pow(std::abs(a), 1.0001f - drive_f) * a_sign;
			if (a > 1.0f) {
				a = 1.0f;
			}
			else if (a < -1.0f) {
				a = -1.0f;
			}
		} break;
		case AudioEffectDistortionConfig::MODE_ATAN: {
			a = std::atan(a * atan_mult) * atan_div;
		} break;
		case AudioEffectDistortionConfig::MODE_LOFI: {
			a = std::floor(a * lofi_mult + 0.5f) / lofi_mult;
		} break;
		case AudioEffectDistortionConfig::MODE_OVERDRIVE: {
			const double x = a * 0.686306;
			const double z = 1 + std::exp(std::sqrt(std::abs(x)) * -0.75);
			a = (std::exp(x) - std::exp(-x * z)) / (std::exp(x) + std::exp(-x));
		} break;
		case AudioEffectDistortionConfig::MODE_WAVESHAPE: {
			float x = a;
			float k = 2 * drive_f / (1.00001f - drive_f);
			a = (1.0f + k) * x / (1.0f + k * std::abs(x));
		} break;
		}

		dst[i] = a * postgain_f + ha;
	}
}

void AudioEffectDistortionConfig::set_mode(Mode p_mode) { mode = p_mode; }

AudioEffectDistortionConfig::Mode AudioEffectDistortionConfig::get_mode() const { return mode; }

void AudioEffectDistortionConfig::set_pre_gain(float p_pre_gain) { pre_gain = p_pre_gain; }

float AudioEffectDistortionConfig::get_pre_gain() const { return pre_gain; }

void AudioEffectDistortionConfig::set_keep_hf_hz(float p_keep_hf_hz)
{
	keep_hf_hz = MAX(p_keep_hf_hz, 1.0f);
}

float AudioEffectDistortionConfig::get_keep_hf_hz() const { return keep_hf_hz; }

void AudioEffectDistortionConfig::set_drive(float p_drive) { drive = p_drive; }

float AudioEffectDistortionConfig::get_drive() const { return drive; }

void AudioEffectDistortionConfig::set_post_gain(float p_post_gain) { post_gain = p_post_gain; }

float AudioEffectDistortionConfig::get_post_gain() const { return post_gain; }

// ============================================================================
// 7. EQ
// ============================================================================

void AudioEffectEQState::process(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	const AudioEffectEQConfig* cfg = base.is_valid() ? base->get_eq_config() : nullptr;
	ERR_FAIL_NULL(cfg);

	int band_count = bands[0].size();
	EQ::BandProcess* proc_l = bands[0].ptrw();
	EQ::BandProcess* proc_r = bands[1].ptrw();
	float* bgain = gains.ptrw();
	for (int i = 0; i < band_count; i++) {
		bgain[i] = Math::db_to_linear(cfg->gain[i]);
	}

	for (int i = 0; i < p_frame_count; i++) {
		AudioFrame src = p_src_frames[i];
		AudioFrame dst = AudioFrame(0, 0);

		for (int j = 0; j < band_count; j++) {
			float l = src.left;
			float r = src.right;

			proc_l[j].process_one(l);
			proc_r[j].process_one(r);

			dst.left += l * bgain[j];
			dst.right += r * bgain[j];
		}

		p_dst_frames[i] = dst;
	}
}

void AudioEffectEQConfig::set_preset(EQ::Preset p_preset)
{
	eq = EQ();
	eq.set_mix_rate(AudioServer::get_mix_rate());
	eq.set_preset_band_mode(p_preset);
	gain.clear();
	prop_band_map.clear();
	band_names.clear();
	gain.resize(eq.get_band_count());
	for (int i = 0; i < gain.size(); i++) {
		gain.write[i] = 0.0f;
		String band_name = "band_db/" + itos(eq.get_band_frequency(i)) + "_hz";
		prop_band_map[band_name] = i;
		band_names.push_back(band_name);
	}
}

void AudioEffectEQConfig::set_band_gain_db(int p_band, float p_volume)
{
	ERR_FAIL_INDEX(p_band, gain.size());
	gain.write[p_band] = p_volume;
}

float AudioEffectEQConfig::get_band_gain_db(int p_band) const
{
	ERR_FAIL_INDEX_V(p_band, gain.size(), 0);
	return gain[p_band];
}

int AudioEffectEQConfig::get_band_count() const { return gain.size(); }

AudioEffectEQConfig::AudioEffectEQConfig(EQ::Preset p_preset)
{
	set_preset(p_preset);
}

// ============================================================================
// 8. Filter
// ============================================================================

template <int S>
void AudioEffectFilterState::_process_filter(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	for (int i = 0; i < p_frame_count; i++) {
		float f = p_src_frames[i].left;
		filter_process[0][0].process_one(f);
		if constexpr (S > 1) {
			filter_process[0][1].process_one(f);
		}
		if constexpr (S > 2) {
			filter_process[0][2].process_one(f);
		}
		if constexpr (S > 3) {
			filter_process[0][3].process_one(f);
		}
		p_dst_frames[i].left = f;
	}

	for (int i = 0; i < p_frame_count; i++) {
		float f = p_src_frames[i].right;
		filter_process[1][0].process_one(f);
		if constexpr (S > 1) {
			filter_process[1][1].process_one(f);
		}
		if constexpr (S > 2) {
			filter_process[1][2].process_one(f);
		}
		if constexpr (S > 3) {
			filter_process[1][3].process_one(f);
		}
		p_dst_frames[i].right = f;
	}
}

void AudioEffectFilterState::process(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	const AudioEffectFilterConfig* cfg = base.is_valid() ? base->get_filter_config() : nullptr;
	ERR_FAIL_NULL(cfg);

	filter.set_cutoff(cfg->cutoff);
	filter.set_gain(cfg->gain);
	filter.set_resonance(cfg->resonance);
	filter.set_mode(cfg->mode);
	int stages = int(cfg->db) + 1;
	filter.set_stages(stages);
	filter.set_sampling_rate(AudioServer::get_mix_rate());

	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 4; j++) {
			filter_process[i][j].update_coeffs();
		}
	}

	if (stages == 1) {
		_process_filter<1>(p_src_frames, p_dst_frames, p_frame_count);
	}
	else if (stages == 2) {
		_process_filter<2>(p_src_frames, p_dst_frames, p_frame_count);
	}
	else if (stages == 3) {
		_process_filter<3>(p_src_frames, p_dst_frames, p_frame_count);
	}
	else if (stages == 4) {
		_process_filter<4>(p_src_frames, p_dst_frames, p_frame_count);
	}
}

AudioEffectFilterState::AudioEffectFilterState()
{
	for (int i = 0; i < 2; i++) {
		for (int j = 0; j < 4; j++) {
			filter_process[i][j].set_filter(&filter);
		}
	}
}

void AudioEffectFilterConfig::set_filter_mode(AudioFilterSW::Mode p_mode) { mode = p_mode; }

AudioFilterSW::Mode AudioEffectFilterConfig::get_filter_mode() const { return mode; }

void AudioEffectFilterConfig::set_cutoff(float p_freq) { cutoff = MAX(p_freq, 1.0f); }

float AudioEffectFilterConfig::get_cutoff() const { return cutoff; }

void AudioEffectFilterConfig::set_resonance(float p_amount) { resonance = p_amount; }

float AudioEffectFilterConfig::get_resonance() const { return resonance; }

void AudioEffectFilterConfig::set_gain(float p_amount) { gain = p_amount; }

float AudioEffectFilterConfig::get_gain() const { return gain; }

void AudioEffectFilterConfig::set_db(FilterDB p_db) { db = p_db; }

AudioEffectFilterConfig::FilterDB AudioEffectFilterConfig::get_db() const { return db; }

AudioEffectFilterConfig::AudioEffectFilterConfig(AudioFilterSW::Mode p_mode) : mode(p_mode) {}

// ============================================================================
// 9. HardLimiter
// ============================================================================

void AudioEffectHardLimiterState::process(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	const AudioEffectHardLimiterConfig* cfg = base.is_valid() ? base->get_hard_limiter_config() : nullptr;
	ERR_FAIL_NULL(cfg);

	float sample_rate = AudioServer::get_mix_rate();

	float ceiling = Math::db_to_linear(cfg->ceiling);
	float release = cfg->release;
	float attack = cfg->attack;
	float pre_gain = Math::db_to_linear(cfg->pre_gain);

	for (int i = 0; i < p_frame_count; i++) {
		float sample_left = p_src_frames[i].left * pre_gain;
		float sample_right = p_src_frames[i].right * pre_gain;

		float largest_sample = MAX(Math::abs(sample_left), Math::abs(sample_right));

		release_factor = MAX(0.0f, release_factor - 1.0f / sample_rate);
		release_factor = MIN(release_factor, release);

		if (release_factor > 0.0f) {
			gain = Math::lerp(gain_target, 1.0f, 1.0f - release_factor / release);
		}

		if (largest_sample * gain > ceiling) {
			gain_target = ceiling / largest_sample;
			release_factor = release;
			attack_factor = attack;
		}

		attack_factor = MAX(0.0f, attack_factor - 1.0f / sample_rate);
		if (attack_factor > 0.0f) {
			gain = Math::lerp(gain_target, gain, 1.0f - attack_factor / attack);
		}

		int bucket_id = gain_bucket_cursor / gain_bucket_size;

		if (gain_bucket_cursor % gain_bucket_size == 0) {
			gain_buckets[bucket_id] = 1.0f;
		}

		gain_buckets[bucket_id] = MIN(gain_buckets[bucket_id], gain);
		gain_bucket_cursor = (gain_bucket_cursor + 1) % gain_samples_to_store;

		for (int j = 0; j < (int)gain_buckets.size(); j++) {
			gain = MIN(gain, gain_buckets[j]);
		}

		float dst_buffer_left = sample_buffer_left[sample_cursor];
		float dst_buffer_right = sample_buffer_right[sample_cursor];

		sample_buffer_left[sample_cursor] = sample_left;
		sample_buffer_right[sample_cursor] = sample_right;

		sample_cursor = (sample_cursor + 1) % sample_buffer_left.size();

		p_dst_frames[i].left = dst_buffer_left * gain;
		p_dst_frames[i].right = dst_buffer_right * gain;
	}
}

void AudioEffectHardLimiterConfig::set_ceiling_db(float p_ceiling) { ceiling = p_ceiling; }

float AudioEffectHardLimiterConfig::get_ceiling_db() const { return ceiling; }

float AudioEffectHardLimiterConfig::get_pre_gain_db() const { return pre_gain; }

void AudioEffectHardLimiterConfig::set_pre_gain_db(float p_pre_gain) { pre_gain = p_pre_gain; }

float AudioEffectHardLimiterConfig::get_release() const { return release; }

void AudioEffectHardLimiterConfig::set_release(float p_release) { release = p_release; }

// ============================================================================
// 10. Limiter
// ============================================================================

void AudioEffectLimiterState::process(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	const AudioEffectLimiterConfig* cfg = base.is_valid() ? base->get_limiter_config() : nullptr;
	ERR_FAIL_NULL(cfg);

	float threshdb = cfg->threshold;
	float ceiling = Math::db_to_linear(cfg->ceiling);
	float ceildb = cfg->ceiling;
	float makeup = Math::db_to_linear(ceildb - threshdb);
	float sc = -cfg->soft_clip;
	float scv = Math::db_to_linear(sc);
	float peakdb = ceildb + 25;
	float scmult = Math::abs((ceildb - sc) / (peakdb - sc));

	for (int i = 0; i < p_frame_count; i++) {
		float spl0 = p_src_frames[i].left * makeup;
		float spl1 = p_src_frames[i].right * makeup;
		float sign0 = (spl0 < 0.0f ? -1.0f : 1.0f);
		float sign1 = (spl1 < 0.0f ? -1.0f : 1.0f);
		float abs0 = Math::abs(spl0);
		float abs1 = Math::abs(spl1);
		float overdb0 = Math::linear_to_db(abs0) - ceildb;
		float overdb1 = Math::linear_to_db(abs1) - ceildb;

		if (abs0 > scv) {
			spl0 = sign0 * (scv + Math::db_to_linear(overdb0 * scmult));
		}
		if (abs1 > scv) {
			spl1 = sign1 * (scv + Math::db_to_linear(overdb1 * scmult));
		}

		spl0 = MIN(ceiling, Math::abs(spl0)) * (spl0 < 0.0f ? -1.0f : 1.0f);
		spl1 = MIN(ceiling, Math::abs(spl1)) * (spl1 < 0.0f ? -1.0f : 1.0f);

		p_dst_frames[i].left = spl0;
		p_dst_frames[i].right = spl1;
	}
}

void AudioEffectLimiterConfig::set_threshold_db(float p_threshold) { threshold = p_threshold; }

float AudioEffectLimiterConfig::get_threshold_db() const { return threshold; }

void AudioEffectLimiterConfig::set_ceiling_db(float p_ceiling) { ceiling = p_ceiling; }

float AudioEffectLimiterConfig::get_ceiling_db() const { return ceiling; }

void AudioEffectLimiterConfig::set_soft_clip_db(float p_soft_clip) { soft_clip = p_soft_clip; }

float AudioEffectLimiterConfig::get_soft_clip_db() const { return soft_clip; }

void AudioEffectLimiterConfig::set_soft_clip_ratio(float p_soft_clip) { soft_clip_ratio = p_soft_clip; }

float AudioEffectLimiterConfig::get_soft_clip_ratio() const { return soft_clip_ratio; }

// ============================================================================
// 11. Panner
// ============================================================================

void AudioEffectPannerState::process(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	const AudioEffectPannerConfig* cfg = base.is_valid() ? base->get_panner_config() : nullptr;
	ERR_FAIL_NULL(cfg);

	float lvol = CLAMP(1.0f - cfg->pan, 0.0f, 1.0f);
	float rvol = CLAMP(1.0f + cfg->pan, 0.0f, 1.0f);

	for (int i = 0; i < p_frame_count; i++) {
		p_dst_frames[i].left = p_src_frames[i].left * lvol + p_src_frames[i].right * (1.0f - rvol);
		p_dst_frames[i].right = p_src_frames[i].right * rvol + p_src_frames[i].left * (1.0f - lvol);
	}
}

void AudioEffectPannerConfig::set_pan(float p_cpanume) { pan = p_cpanume; }

float AudioEffectPannerConfig::get_pan() const { return pan; }

// ============================================================================
// 12. Phaser
// ============================================================================

void AudioEffectPhaserState::process(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	const AudioEffectPhaserConfig* cfg = base.is_valid() ? base->get_phaser_config() : nullptr;
	ERR_FAIL_NULL(cfg);

	float sampling_rate = AudioServer::get_mix_rate();

	float dmin = cfg->range_min / (sampling_rate / 2.0f);
	float dmax = cfg->range_max / (sampling_rate / 2.0f);
	float increment = Math::TAU * (cfg->rate / sampling_rate);

	for (int i = 0; i < p_frame_count; i++) {
		phase += increment;

		while (phase >= Math::TAU) {
			phase -= Math::TAU;
		}

		float d = dmin + (dmax - dmin) * ((std::sin(phase) + 1.f) / 2.f);

		for (int j = 0; j < 6; j++) {
			allpass[0][j].delay(d);
			allpass[1][j].delay(d);
		}

		float y = allpass[0][0].update(
			allpass[0][1].update(allpass[0][2].update(allpass[0][3].update(allpass[0][4].update(
				allpass[0][5].update(p_src_frames[i].left + h.left * cfg->feedback))))));
		h.left = y;
		p_dst_frames[i].left = p_src_frames[i].left + y * cfg->depth;

		y = allpass[1][0].update(
			allpass[1][1].update(allpass[1][2].update(allpass[1][3].update(allpass[1][4].update(
				allpass[1][5].update(p_src_frames[i].right + h.right * cfg->feedback))))));
		h.right = y;
		p_dst_frames[i].right = p_src_frames[i].right + y * cfg->depth;
	}
}

void AudioEffectPhaserConfig::set_range_min_hz(float p_hz) { range_min = p_hz; }

float AudioEffectPhaserConfig::get_range_min_hz() const { return range_min; }

void AudioEffectPhaserConfig::set_range_max_hz(float p_hz) { range_max = p_hz; }

float AudioEffectPhaserConfig::get_range_max_hz() const { return range_max; }

void AudioEffectPhaserConfig::set_rate_hz(float p_hz) { rate = p_hz; }

float AudioEffectPhaserConfig::get_rate_hz() const { return rate; }

void AudioEffectPhaserConfig::set_feedback(float p_fbk) { feedback = p_fbk; }

float AudioEffectPhaserConfig::get_feedback() const { return feedback; }

void AudioEffectPhaserConfig::set_depth(float p_depth) { depth = p_depth; }

float AudioEffectPhaserConfig::get_depth() const { return depth; }

// ============================================================================
// 13. PitchShift
// ============================================================================

/* clang-format off */
void SMBPitchShift::PitchShift(float pitchShift, long numSampsToProcess, long fftFrameSize, long osamp, float sampleRate, float *indata, float *outdata, int stride)
{
	double magn, phase, tmp, window, real, imag;
	double freqPerBin, expct;
	long i, k, qpd, index, inFifoLatency, stepSize, fftFrameSize2;

	fftFrameSize2 = fftFrameSize / 2;
	stepSize = fftFrameSize / osamp;
	freqPerBin = sampleRate / (double)fftFrameSize;
	expct = 2. * Math::PI * (double)stepSize / (double)fftFrameSize;
	inFifoLatency = fftFrameSize - stepSize;
	if (gRover == 0) {
		gRover = inFifoLatency;
	}

	for (i = 0; i < numSampsToProcess; i++) {
		gInFIFO[gRover] = indata[i * stride];
		outdata[i * stride] = gOutFIFO[gRover - inFifoLatency];
		gRover++;

		if (gRover >= fftFrameSize) {
			gRover = inFifoLatency;

			for (k = 0; k < fftFrameSize; k++) {
				window = -.5 * std::cos(2. * Math::PI * (double)k / (double)fftFrameSize) + .5;
				gFFTworksp[2 * k] = gInFIFO[k] * window;
				gFFTworksp[2 * k + 1] = 0.;
			}

			smbFft(gFFTworksp, fftFrameSize, -1);

			for (k = 0; k <= fftFrameSize2; k++) {
				real = gFFTworksp[2 * k];
				imag = gFFTworksp[2 * k + 1];

				magn = 2. * std::sqrt(real * real + imag * imag);
				phase = std::atan2(imag, real);

				tmp = phase - gLastPhase[k];
				gLastPhase[k] = phase;

				tmp -= (double)k * expct;

				qpd = tmp / Math::PI;
				if (qpd >= 0) {
					qpd += qpd & 1;
				}
				else {
					qpd -= qpd & 1;
				}
				tmp -= Math::PI * (double)qpd;

				tmp = osamp * tmp / (2. * Math::PI);
				tmp = (double)k * freqPerBin + tmp * freqPerBin;

				gAnaMagn[k] = magn;
				gAnaFreq[k] = tmp;
			}

			size_t fftBufferSize = static_cast<size_t>(fftFrameSize) * sizeof(float);
			if (unlikely(fftBufferSize > MAX_FRAME_LENGTH)) {
				ERR_PRINT_ONCE("Invalid FFT frame size for PitchShift. This is a bug, please report.");
				return;
			}
			memset(gSynMagn, 0, fftBufferSize);
			memset(gSynFreq, 0, fftBufferSize);
			for (k = 0; k <= fftFrameSize2; k++) {
				index = k * pitchShift;
				if (index <= fftFrameSize2) {
					gSynMagn[index] += gAnaMagn[k];
					gSynFreq[index] = gAnaFreq[k] * pitchShift;
				}
			}

			for (k = 0; k <= fftFrameSize2; k++) {
				magn = gSynMagn[k];
				tmp = gSynFreq[k];

				tmp -= (double)k * freqPerBin;
				tmp /= freqPerBin;
				tmp = 2. * Math::PI * tmp / osamp;
				tmp += (double)k * expct;

				gSumPhase[k] += tmp;
				phase = gSumPhase[k];

				gFFTworksp[2 * k] = magn * std::cos(phase);
				gFFTworksp[2 * k + 1] = magn * std::sin(phase);
			}

			for (k = fftFrameSize + 2; k < 2 * fftFrameSize; k++) {
				gFFTworksp[k] = 0.;
			}

			smbFft(gFFTworksp, fftFrameSize, 1);

			for (k = 0; k < fftFrameSize; k++) {
				window = -.5 * std::cos(2. * Math::PI * (double)k / (double)fftFrameSize) + .5;
				gOutputAccum[k] += 2. * window * gFFTworksp[2 * k] / (fftFrameSize2 * osamp);
			}
			for (k = 0; k < stepSize; k++) {
				gOutFIFO[k] = gOutputAccum[k];
			}

			memmove(gOutputAccum, gOutputAccum + stepSize, fftBufferSize);

			for (k = 0; k < inFifoLatency; k++) {
				gInFIFO[k] = gInFIFO[k + stepSize];
			}
		}
	}
}

void SMBPitchShift::smbFft(float *fftBuffer, long fftFrameSize, long sign)
{
	float wr, wi, arg, *p1, *p2, temp;
	float tr, ti, ur, ui, *p1r, *p1i, *p2r, *p2i;
	long i, bitm, j, le, le2, k;

	for (i = 2; i < 2 * fftFrameSize - 2; i += 2) {
		for (bitm = 2, j = 0; bitm < 2 * fftFrameSize; bitm <<= 1) {
			if (i & bitm) {
				j++;
			}
			j <<= 1;
		}
		if (i < j) {
			p1 = fftBuffer + i;
			p2 = fftBuffer + j;
			temp = *p1;
			*(p1++) = *p2;
			*(p2++) = temp;
			temp = *p1;
			*p1 = *p2;
			*p2 = temp;
		}
	}
	for (k = 0, le = 2; k < (long)(std::log((double)fftFrameSize) / std::log(2.) + .5); k++) {
		le <<= 1;
		le2 = le >> 1;
		ur = 1.0;
		ui = 0.0;
		arg = Math::PI / (le2 >> 1);
		wr = std::cos(arg);
		wi = sign * std::sin(arg);
		for (j = 0; j < le2; j += 2) {
			p1r = fftBuffer + j;
			p1i = p1r + 1;
			p2r = p1r + le2;
			p2i = p2r + 1;
			for (i = j; i < 2 * fftFrameSize; i += le) {
				tr = *p2r * ur - *p2i * ui;
				ti = *p2r * ui + *p2i * ur;
				*p2r = *p1r - tr;
				*p2i = *p1i - ti;
				*p1r += tr;
				*p1i += ti;
				p1r += le;
				p1i += le;
				p2r += le;
				p2i += le;
			}
			tr = ur * wr - ui * wi;
			ui = ur * wi + ui * wr;
			ur = tr;
		}
	}
}
/* clang-format on */

void AudioEffectPitchShiftState::process(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	const AudioEffectPitchShiftConfig* cfg = base.is_valid() ? base->get_pitch_shift_config() : nullptr;
	ERR_FAIL_NULL(cfg);

	if (Math::is_equal_approx(cfg->pitch_scale, 1.0f)) {
		for (int i = 0; i < p_frame_count; i++) {
			p_dst_frames[i] = p_src_frames[i];
		}
		return;
	}

	float sample_rate = AudioServer::get_mix_rate();

	float* in_l = (float*)p_src_frames;
	float* in_r = in_l + 1;

	float* out_l = (float*)p_dst_frames;
	float* out_r = out_l + 1;

	shift_l.PitchShift(cfg->pitch_scale, p_frame_count, fft_size, cfg->oversampling, sample_rate,
		in_l, out_l, 2);
	shift_r.PitchShift(cfg->pitch_scale, p_frame_count, fft_size, cfg->oversampling, sample_rate,
		in_r, out_r, 2);
}

void AudioEffectPitchShiftConfig::set_pitch_scale(float p_pitch_scale)
{
	ERR_FAIL_COND(!(p_pitch_scale > 0.0f));
	pitch_scale = p_pitch_scale;
}

float AudioEffectPitchShiftConfig::get_pitch_scale() const { return pitch_scale; }

void AudioEffectPitchShiftConfig::set_oversampling(int p_oversampling)
{
	ERR_FAIL_COND(p_oversampling < 4);
	oversampling = p_oversampling;
}

int AudioEffectPitchShiftConfig::get_oversampling() const { return oversampling; }

void AudioEffectPitchShiftConfig::set_fft_size(FFTSize p_fft_size)
{
	ERR_FAIL_INDEX(p_fft_size, FFT_SIZE_MAX);
	fft_size = p_fft_size;
}

AudioEffectPitchShiftConfig::FFTSize AudioEffectPitchShiftConfig::get_fft_size() const
{
	return fft_size;
}

// ============================================================================
// 14. Record
// ============================================================================

void AudioEffectRecordState::process(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	if (!is_recording) {
		for (int i = 0; i < p_frame_count; i++) {
			p_dst_frames[i] = p_src_frames[i];
		}
		return;
	}

	const AudioFrame* src = p_src_frames;
	AudioFrame* rb_buf = ring_buffer.ptrw();
	for (int i = 0; i < p_frame_count; i++) {
		p_dst_frames[i] = p_src_frames[i];
		rb_buf[ring_buffer_pos & ring_buffer_mask] = src[i];
		ring_buffer_pos++;
	}
}

void AudioEffectRecordState::_update_buffer()
{
	while (ring_buffer_read_pos < ring_buffer_pos) {
		_io_store_buffer();
	}
}

void AudioEffectRecordState::_update(void* userdata)
{
	AudioEffectRecordState* ins = static_cast<AudioEffectRecordState*>(userdata);
	ins->_update_buffer();
}

bool AudioEffectRecordState::process_silence() const { return true; }

void AudioEffectRecordState::_io_thread_process()
{
	while (is_recording) {
		_update_buffer();
		if (is_recording) {
			OS::delay_usec(500);
		}
	}
}

void AudioEffectRecordState::_io_store_buffer()
{
	int to_read = ring_buffer_pos - ring_buffer_read_pos;
	AudioFrame* rb_buf = ring_buffer.ptrw();

	while (to_read) {
		AudioFrame buffered_frame = rb_buf[ring_buffer_read_pos & ring_buffer_mask];
		recording_data.push_back(buffered_frame.left);
		recording_data.push_back(buffered_frame.right);

		ring_buffer_read_pos++;
		to_read--;
	}
}

void AudioEffectRecordState::_thread_callback(void* _instance)
{
	AudioEffectRecordState* aeri = reinterpret_cast<AudioEffectRecordState*>(_instance);
	aeri->_io_thread_process();
}

void AudioEffectRecordState::init()
{
	ring_buffer_pos = 0;
	ring_buffer_read_pos = 0;
	recording_data.clear();
	is_recording = true;
	io_thread.start(_thread_callback, this);
}

void AudioEffectRecordState::finish()
{
	is_recording = false;
	if (io_thread.is_started()) {
		io_thread.wait_to_finish();
	}
}

void AudioEffectRecordConfig::ensure_thread_stopped()
{
	if (current_instance.is_valid()) {
		if (AudioEffectRecordState* state = current_instance->get_state<AudioEffectRecordState>()) {
			state->finish();
		}
	}
}

void AudioEffectRecordConfig::set_recording_active(bool p_record)
{
	if (p_record) {
		if (current_instance.is_null()) {
			WARN_PRINT("Recording should not be set as active before Godot has initialized.");
			return;
		}
		ensure_thread_stopped();
		if (AudioEffectRecordState* state = current_instance->get_state<AudioEffectRecordState>()) {
			state->init();
		}
	}
	else {
		if (current_instance.is_valid()) {
			if (AudioEffectRecordState* state = current_instance->get_state<AudioEffectRecordState>()) {
				state->is_recording = false;
			}
		}
	}
}

bool AudioEffectRecordConfig::is_recording_active() const
{
	if (current_instance.is_null()) {
		return false;
	}
	const AudioEffectRecordState* state = current_instance->get_state<AudioEffectRecordState>();
	return state ? state->is_recording : false;
}

void AudioEffectRecordConfig::set_format(AudioStreamWAV::Format p_format) { format = p_format; }

AudioStreamWAV::Format AudioEffectRecordConfig::get_format() const { return format; }

Ref<AudioStreamWAV> AudioEffectRecordConfig::get_recording() const
{
	AudioStreamWAV::Format dst_format = format;
	bool stereo = true;

	Vector<uint8_t> dst_data;

	ERR_FAIL_COND_V(current_instance.is_null(), nullptr);
	const AudioEffectRecordState* state = current_instance->get_state<AudioEffectRecordState>();
	ERR_FAIL_COND_V(!state || state->recording_data.is_empty(), nullptr);

	if (dst_format == AudioStreamWAV::FORMAT_8_BITS) {
		int data_size = state->recording_data.size();
		dst_data.resize(data_size);
		uint8_t* w = dst_data.ptrw();

		for (int i = 0; i < data_size; i++) {
			int8_t v = CLAMP(state->recording_data[i] * 128, -128, 127);
			w[i] = v;
		}
	}
	else if (dst_format == AudioStreamWAV::FORMAT_16_BITS) {
		int data_size = state->recording_data.size();
		dst_data.resize(data_size * 2);
		uint8_t* w = dst_data.ptrw();

		for (int i = 0; i < data_size; i++) {
			int16_t v = CLAMP(state->recording_data[i] * 32768, -32768, 32767);
			encode_uint16(v, &w[i * 2]);
		}
	}
	else if (dst_format == AudioStreamWAV::FORMAT_IMA_ADPCM) {
		Vector<float> left;
		Vector<float> right;

		int tframes = state->recording_data.size() / 2;
		left.resize(tframes);
		right.resize(tframes);

		for (int i = 0; i < tframes; i++) {
			left.set(i, state->recording_data[i * 2 + 0]);
			right.set(i, state->recording_data[i * 2 + 1]);
		}

		Vector<uint8_t> bleft;
		Vector<uint8_t> bright;

		AudioStreamWAV::_compress_ima_adpcm(left, bleft);
		AudioStreamWAV::_compress_ima_adpcm(right, bright);

		int dl = bleft.size();
		dst_data.resize(dl * 2);

		uint8_t* w = dst_data.ptrw();
		const uint8_t* rl = bleft.ptr();
		const uint8_t* rr = bright.ptr();

		for (int i = 0; i < dl; i++) {
			w[i * 2 + 0] = rl[i];
			w[i * 2 + 1] = rr[i];
		}
	}
	else if (dst_format == AudioStreamWAV::FORMAT_QOA) {
		qoa_desc desc = {};
		desc.samples = state->recording_data.size() / 2;
		desc.samplerate = AudioServer::get_mix_rate();
		desc.channels = 2;
		AudioStreamWAV::_compress_qoa(state->recording_data, dst_data, &desc);
	}
	else {
		ERR_PRINT("Format not implemented.");
	}

	Ref<AudioStreamWAV> sample;
	sample.instantiate();
	sample->set_data(dst_data);
	sample->set_format(dst_format);
	sample->set_mix_rate(AudioServer::get_mix_rate());
	sample->set_loop_mode(AudioStreamWAV::LOOP_DISABLED);
	sample->set_loop_begin(0);
	sample->set_loop_end(0);
	sample->set_stereo(stereo);

	return sample;
}

// ============================================================================
// 15. Reverb
// ============================================================================

void AudioEffectReverbState::process(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	const AudioEffectReverbConfig* cfg = base.is_valid() ? base->get_reverb_config() : nullptr;
	ERR_FAIL_NULL(cfg);

	for (int i = 0; i < 2; i++) {
		Reverb& r = reverb[i];

		r.set_predelay(cfg->predelay);
		r.set_predelay_feedback(cfg->predelay_fb);
		r.set_highpass(cfg->hpf);
		r.set_room_size(cfg->room_size);
		r.set_damp(cfg->damping);
		r.set_extra_spread(cfg->spread);
		r.set_wet(cfg->wet);
		r.set_dry(cfg->dry);
	}

	int todo = p_frame_count;
	int offset = 0;

	while (todo) {
		int to_mix = MIN(todo, Reverb::INPUT_BUFFER_MAX_SIZE);

		for (int j = 0; j < to_mix; j++) {
			tmp_src[j] = p_src_frames[offset + j].left;
		}

		reverb[0].process(tmp_src, tmp_dst, to_mix);

		for (int j = 0; j < to_mix; j++) {
			p_dst_frames[offset + j].left = tmp_dst[j];
			tmp_src[j] = p_src_frames[offset + j].right;
		}

		reverb[1].process(tmp_src, tmp_dst, to_mix);

		for (int j = 0; j < to_mix; j++) {
			p_dst_frames[offset + j].right = tmp_dst[j];
		}

		offset += to_mix;
		todo -= to_mix;
	}
}

AudioEffectReverbState::AudioEffectReverbState()
{
	reverb[0].set_mix_rate(AudioServer::get_mix_rate());
	reverb[0].set_extra_spread_base(0);
	reverb[1].set_mix_rate(AudioServer::get_mix_rate());
	reverb[1].set_extra_spread_base(0.000521f);
}

void AudioEffectReverbConfig::set_predelay_msec(float p_msec) { predelay = p_msec; }

void AudioEffectReverbConfig::set_predelay_feedback(float p_feedback)
{
	predelay_fb = CLAMP(p_feedback, 0.0f, 0.98f);
}

void AudioEffectReverbConfig::set_room_size(float p_size) { room_size = p_size; }

void AudioEffectReverbConfig::set_damping(float p_damping) { damping = p_damping; }

void AudioEffectReverbConfig::set_spread(float p_spread) { spread = p_spread; }

void AudioEffectReverbConfig::set_dry(float p_dry) { dry = p_dry; }

void AudioEffectReverbConfig::set_wet(float p_wet) { wet = p_wet; }

void AudioEffectReverbConfig::set_hpf(float p_hpf) { hpf = p_hpf; }

float AudioEffectReverbConfig::get_predelay_msec() const { return predelay; }

float AudioEffectReverbConfig::get_predelay_feedback() const { return predelay_fb; }

float AudioEffectReverbConfig::get_room_size() const { return room_size; }

float AudioEffectReverbConfig::get_damping() const { return damping; }

float AudioEffectReverbConfig::get_spread() const { return spread; }

float AudioEffectReverbConfig::get_dry() const { return dry; }

float AudioEffectReverbConfig::get_wet() const { return wet; }

float AudioEffectReverbConfig::get_hpf() const { return hpf; }

// ============================================================================
// 16. SpectrumAnalyzer
// ============================================================================

static void _spectrum_smb_fft(float* fftBuffer, long fftFrameSize, long sign)
{
	float wr, wi, arg, *p1, *p2, temp;
	float tr, ti, ur, ui, *p1r, *p1i, *p2r, *p2i;
	long i, bitm, j, le, le2, k, logN;
	logN = (long)(Math::log((double)fftFrameSize) / Math::log(2.) + .5);

	for (i = 2; i < 2 * fftFrameSize - 2; i += 2) {
		for (bitm = 2, j = 0; bitm < 2 * fftFrameSize; bitm <<= 1) {
			if (i & bitm) {
				j++;
			}
			j <<= 1;
		}
		if (i < j) {
			p1 = fftBuffer + i;
			p2 = fftBuffer + j;
			temp = *p1;
			*(p1++) = *p2;
			*(p2++) = temp;
			temp = *p1;
			*p1 = *p2;
			*p2 = temp;
		}
	}
	for (k = 0, le = 2; k < logN; k++) {
		le <<= 1;
		le2 = le >> 1;
		ur = 1.0;
		ui = 0.0;
		arg = Math::PI / (le2 >> 1);
		wr = std::cos(arg);
		wi = sign * std::sin(arg);
		for (j = 0; j < le2; j += 2) {
			p1r = fftBuffer + j;
			p1i = p1r + 1;
			p2r = p1r + le2;
			p2i = p2r + 1;
			for (i = j; i < 2 * fftFrameSize; i += le) {
				tr = *p2r * ur - *p2i * ui;
				ti = *p2r * ui + *p2i * ur;
				*p2r = *p1r - tr;
				*p2i = *p1i - ti;
				*p1r += tr;
				*p1i += ti;
				p1r += le;
				p1i += le;
				p2r += le;
				p2i += le;
			}
			tr = ur * wr - ui * wi;
			ui = ur * wi + ui * wr;
			ur = tr;
		}
	}
}

void AudioEffectSpectrumAnalyzerState::process(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	for (int i = 0; i < p_frame_count; i++) {
		p_dst_frames[i] = p_src_frames[i];
	}

	while (p_frame_count) {
		int to_fill = fft_size * 2 - temporal_fft_pos;
		to_fill = MIN(to_fill, p_frame_count);
		const double to_fill_step = Math::TAU / (double)(fft_size * 2);

		float* fftw = temporal_fft.ptrw();
		for (int i = 0; i < to_fill; i++) {
			float window = -0.5 * Math::cos(to_fill_step * (double)temporal_fft_pos) + 0.5;
			fftw[temporal_fft_pos * 2] = window * p_src_frames->left;
			fftw[temporal_fft_pos * 2 + 1] = 0;
			fftw[(temporal_fft_pos + fft_size * 2) * 2] = window * p_src_frames->right;
			fftw[(temporal_fft_pos + fft_size * 2) * 2 + 1] = 0;
			++p_src_frames;
			++temporal_fft_pos;
		}

		p_frame_count -= to_fill;

		if (temporal_fft_pos == fft_size * 2) {
			_spectrum_smb_fft(fftw, fft_size * 2, -1);
			_spectrum_smb_fft(fftw + fft_size * 4, fft_size * 2, -1);
			int next = (fft_pos + 1) % fft_count;

			AudioFrame* hw = (AudioFrame*)fft_history[next].ptr();

			for (int i = 0; i < fft_size; i++) {
				hw[i].left = Vector2(fftw[i * 2], fftw[i * 2 + 1]).length() / float(fft_size);
				hw[i].right =
					Vector2(fftw[fft_size * 4 + i * 2], fftw[fft_size * 4 + i * 2 + 1]).length() /
					float(fft_size);
			}

			fft_pos = next;
			temporal_fft_pos = 0;
		}
	}
}

Vector2 AudioEffectSpectrumAnalyzerState::get_magnitude_for_frequency_range(
	float p_begin, float p_end, MagnitudeMode p_mode) const
{
	int fft_index = fft_pos;

	int begin_pos = p_begin * fft_size / (mix_rate * 0.5f);
	int end_pos = p_end * fft_size / (mix_rate * 0.5f);

	begin_pos = CLAMP(begin_pos, 0, fft_size - 1);
	end_pos = CLAMP(end_pos, 0, fft_size - 1);

	if (begin_pos > end_pos) {
		SWAP(begin_pos, end_pos);
	}
	const AudioFrame* r = fft_history[fft_index].ptr();

	if (p_mode == MAGNITUDE_AVERAGE) {
		Vector2 avg;
		for (int i = begin_pos; i <= end_pos; i++) {
			avg += Vector2(r[i]);
		}
		avg /= float(end_pos - begin_pos + 1);
		return avg;
	}
	else {
		Vector2 max;
		for (int i = begin_pos; i <= end_pos; i++) {
			max.x = MAX(max.x, r[i].left);
			max.y = MAX(max.y, r[i].right);
		}
		return max;
	}
}

void AudioEffectSpectrumAnalyzerConfig::set_buffer_length(float p_seconds)
{
	buffer_length = p_seconds;
}

float AudioEffectSpectrumAnalyzerConfig::get_buffer_length() const { return buffer_length; }

void AudioEffectSpectrumAnalyzerConfig::set_fft_size(FFTSize p_fft_size)
{
	ERR_FAIL_INDEX(p_fft_size, FFT_SIZE_MAX);
	fft_size = p_fft_size;
}

AudioEffectSpectrumAnalyzerConfig::FFTSize AudioEffectSpectrumAnalyzerConfig::get_fft_size() const
{
	return fft_size;
}

// ============================================================================
// 17. StereoEnhance
// ============================================================================

void AudioEffectStereoEnhanceState::process(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	const AudioEffectStereoEnhanceConfig* cfg = base.is_valid() ? base->get_stereo_enhance_config() : nullptr;
	ERR_FAIL_NULL(cfg);

	float intensity = cfg->pan_pullout;
	bool surround_mode = cfg->surround > 0;
	float surround_amount = cfg->surround;
	unsigned int delay_frames = (cfg->time_pullout / 1000.0f) * AudioServer::get_mix_rate();

	for (int i = 0; i < p_frame_count; i++) {
		float left = p_src_frames[i].left;
		float right = p_src_frames[i].right;

		float center = (left + right) / 2.0f;

		left = center + (left - center) * intensity;
		right = center + (right - center) * intensity;

		if (surround_mode) {
			float val = (left + right) / 2.0f;
			delay_ringbuff[ringbuff_pos & ringbuff_mask] = val;

			float out = delay_ringbuff[(ringbuff_pos - delay_frames) & ringbuff_mask] * surround_amount;
			left += out;
			right += -out;
		}
		else {
			float val = right;
			delay_ringbuff[ringbuff_pos & ringbuff_mask] = val;
			right = delay_ringbuff[(ringbuff_pos - delay_frames) & ringbuff_mask];
		}

		p_dst_frames[i].left = left;
		p_dst_frames[i].right = right;
		ringbuff_pos++;
	}
}

AudioEffectStereoEnhanceState::~AudioEffectStereoEnhanceState()
{
	memdelete_arr(delay_ringbuff);
}

void AudioEffectStereoEnhanceConfig::set_pan_pullout(float p_amount) { pan_pullout = p_amount; }

float AudioEffectStereoEnhanceConfig::get_pan_pullout() const { return pan_pullout; }

void AudioEffectStereoEnhanceConfig::set_time_pullout(float p_amount) { time_pullout = p_amount; }

float AudioEffectStereoEnhanceConfig::get_time_pullout() const { return time_pullout; }

void AudioEffectStereoEnhanceConfig::set_surround(float p_amount) { surround = p_amount; }

float AudioEffectStereoEnhanceConfig::get_surround() const { return surround; }

// ============================================================================
// Merged from servers/audio/effects/eq_filter.cpp
// ============================================================================


#include "core/error/error_macros.h"
#include "core/math/math_funcs.h"

#define POW2(v) ((v) * (v))

/* Helper */
static int solve_quadratic(double a, double b, double c, double *r1, double *r2) {
	//solves quadractic and returns number of roots

	double base = 2 * a;
	if (base == 0.0f) {
		return 0;
	}

	double squared = b * b - 4 * a * c;
	if (squared < 0.0) {
		return 0;
	}

	squared = Math::sqrt(squared);

	*r1 = (-b + squared) / base;
	*r2 = (-b - squared) / base;

	if (*r1 == *r2) {
		return 1;
	} else {
		return 2;
	}
}

EQ::BandProcess::BandProcess() {
	c1 = c2 = c3 = history.a1 = history.a2 = history.a3 = 0;
	history.b1 = history.b2 = history.b3 = 0;
}

void EQ::recalculate_band_coefficients() {
#define BAND_LOG(m_f) (Math::log((m_f)) / Math::log(2.))

	for (int i = 0; i < band.size(); i++) {
		double octave_size;

		double frq = band[i].freq;

		if (i == 0) {
			octave_size = BAND_LOG(band[1].freq) - BAND_LOG(frq);
		} else if (i == (band.size() - 1)) {
			octave_size = BAND_LOG(frq) - BAND_LOG(band[i - 1].freq);
		} else {
			double next = BAND_LOG(band[i + 1].freq) - BAND_LOG(frq);
			double prev = BAND_LOG(frq) - BAND_LOG(band[i - 1].freq);
			octave_size = (next + prev) / 2.0;
		}

		double frq_l = Math::round(frq / Math::pow(2.0, octave_size / 2.0));

		double side_gain2 = POW2(Math::SQRT12);
		double th = Math::TAU * frq / mix_rate;
		double th_l = Math::TAU * frq_l / mix_rate;

		double c2a = side_gain2 * POW2(Math::cos(th)) - 2.0 * side_gain2 * Math::cos(th_l) * Math::cos(th) + side_gain2 - POW2(Math::sin(th_l));

		double c2b = 2.0 * side_gain2 * POW2(Math::cos(th_l)) + side_gain2 * POW2(Math::cos(th)) - 2.0 * side_gain2 * Math::cos(th_l) * Math::cos(th) - side_gain2 + POW2(Math::sin(th_l));

		double c2c = 0.25 * side_gain2 * POW2(Math::cos(th)) - 0.5 * side_gain2 * Math::cos(th_l) * Math::cos(th) + 0.25 * side_gain2 - 0.25 * POW2(Math::sin(th_l));

		//printf("band %i, precoefs = %f,%f,%f\n",i,c2a,c2b,c2c);

		// Default initializing to silence compiler warning about potential uninitialized use.
		// Both variables are properly set in _solve_quadratic before use, or we continue if roots == 0.
		double r1 = 0, r2 = 0; //roots
		int roots = solve_quadratic(c2a, c2b, c2c, &r1, &r2);

		ERR_CONTINUE(roots == 0);

		band.write[i].c1 = 2.0 * ((0.5 - r1) / 2.0);
		band.write[i].c2 = 2.0 * r1;
		band.write[i].c3 = 2.0 * (0.5 + r1) * Math::cos(th);
		//printf("band %i, coefs = %f,%f,%f\n",i,(float)bands[i].c1,(float)bands[i].c2,(float)bands[i].c3);
	}
}

void EQ::set_preset_band_mode(Preset p_preset) {
	band.clear();

#define PUSH_BANDS(m_bands) \
	for (int i = 0; i < m_bands; i++) { \
		Band b; \
		b.freq = bands[i]; \
		b.c1 = b.c2 = b.c3 = 0; \
		band.push_back(b); \
	}

	switch (p_preset) {
		case PRESET_6_BANDS: {
			static const double bands[] = { 32, 100, 320, 1e3, 3200, 10e3 };
			PUSH_BANDS(6);

		} break;

		case PRESET_8_BANDS: {
			static const double bands[] = { 32, 72, 192, 512, 1200, 3000, 7500, 16e3 };

			PUSH_BANDS(8);
		} break;

		case PRESET_10_BANDS: {
			static const double bands[] = { 31.25, 62.5, 125, 250, 500, 1e3, 2e3, 4e3, 8e3, 16e3 };

			PUSH_BANDS(10);

		} break;

		case PRESET_21_BANDS: {
			static const double bands[] = { 22, 32, 44, 63, 90, 125, 175, 250, 350, 500, 700, 1e3, 1400, 2e3, 2800, 4e3, 5600, 8e3, 11e3, 16e3, 22e3 };
			PUSH_BANDS(21);

		} break;

		case PRESET_31_BANDS: {
			static const double bands[] = { 20, 25, 31.5, 40, 50, 63, 80, 100, 125, 160, 200, 250, 315, 400, 500, 630, 800, 1e3, 1250, 1600, 2e3, 2500, 3150, 4e3, 5e3, 6300, 8e3, 10e3, 12500, 16e3, 20e3 };
			PUSH_BANDS(31);
		} break;
	};

	recalculate_band_coefficients();
}

int EQ::get_band_count() const {
	return band.size();
}

float EQ::get_band_frequency(int p_band) {
	ERR_FAIL_INDEX_V(p_band, band.size(), 0);
	return band[p_band].freq;
}

void EQ::set_bands(const Vector<float> &p_bands) {
	band.resize(p_bands.size());
	for (int i = 0; i < p_bands.size(); i++) {
		band.write[i].freq = p_bands[i];
	}

	recalculate_band_coefficients();
}

void EQ::set_mix_rate(float p_mix_rate) {
	mix_rate = p_mix_rate;
	recalculate_band_coefficients();
}

EQ::BandProcess EQ::get_band_processor(int p_band) const {
	EQ::BandProcess band_proc;

	ERR_FAIL_INDEX_V(p_band, band.size(), band_proc);

	band_proc.c1 = band[p_band].c1;
	band_proc.c2 = band[p_band].c2;
	band_proc.c3 = band[p_band].c3;

	return band_proc;
}

EQ::EQ() {
	mix_rate = 44100;
}

EQ::~EQ() {
}

#ifdef POW2
#undef POW2
#endif
#ifdef BAND_LOG
#undef BAND_LOG
#endif
#ifdef PUSH_BANDS
#undef PUSH_BANDS
#endif

// ============================================================================
// Merged from servers/audio/effects/reverb_filter.cpp
// ============================================================================


#include "core/math/audio_frame.h"
#include "core/os/memory.h"

const float Reverb::comb_tunings[MAX_COMBS] = {
	//freeverb comb tunings
	0.025306122448979593f,
	0.026938775510204082f,
	0.028956916099773241f,
	0.03074829931972789f,
	0.032244897959183672f,
	0.03380952380952381f,
	0.035306122448979592f,
	0.036666666666666667f
};

const float Reverb::allpass_tunings[MAX_ALLPASS] = {
	//freeverb allpass tunings
	0.0051020408163265302f,
	0.007732426303854875f,
	0.01f,
	0.012607709750566893f
};

void Reverb::process(float *p_src, float *p_dst, int p_frames) {
	if (p_frames > INPUT_BUFFER_MAX_SIZE) {
		p_frames = INPUT_BUFFER_MAX_SIZE;
	}

	int predelay_frames = std::rint((params.predelay / 1000.0) * params.mix_rate);
	if (predelay_frames < 10) {
		predelay_frames = 10;
	}
	if (predelay_frames >= echo_buffer_size) {
		predelay_frames = echo_buffer_size - 1;
	}

	for (int i = 0; i < p_frames; i++) {
		if (echo_buffer_pos >= echo_buffer_size) {
			echo_buffer_pos = 0;
		}

		int read_pos = echo_buffer_pos - predelay_frames;
		while (read_pos < 0) {
			read_pos += echo_buffer_size;
		}

		float in = undenormalize(echo_buffer[read_pos] * params.predelay_fb + p_src[i]);

		echo_buffer[echo_buffer_pos] = in;

		input_buffer[i] = in;

		p_dst[i] = 0; //take the chance and clear this

		echo_buffer_pos++;
	}

	if (params.hpf > 0) {
		float hpaux = std::exp(-Math::TAU * params.hpf * 6000 / params.mix_rate);
		float hp_a1 = (1.0 + hpaux) / 2.0;
		float hp_a2 = -(1.0 + hpaux) / 2.0;
		float hp_b1 = hpaux;

		for (int i = 0; i < p_frames; i++) {
			float in = input_buffer[i];
			input_buffer[i] = in * hp_a1 + hpf_h1 * hp_a2 + hpf_h2 * hp_b1;
			hpf_h2 = input_buffer[i];
			hpf_h1 = in;
		}
	}

	for (int i = 0; i < MAX_COMBS; i++) {
		Comb &c = comb[i];

		int size_limit = c.size - std::rint((float)c.extra_spread_frames * (1.0 - params.extra_spread));
		for (int j = 0; j < p_frames; j++) {
			if (c.pos >= size_limit) { //reset this now just in case
				c.pos = 0;
			}

			float out = undenormalize(c.buffer[c.pos] * c.feedback);
			out = out * (1.0 - c.damp) + c.damp_h * c.damp; //lowpass
			c.damp_h = out;
			c.buffer[c.pos] = input_buffer[j] + out;
			p_dst[j] += out;
			c.pos++;
		}
	}

	static const float allpass_feedback = 0.7;
	/* this one works, but the other version is just nicer....
	int ap_size_limit[MAX_ALLPASS];

	for (int i=0;i<MAX_ALLPASS;i++) {
		AllPass &a=allpass[i];
		ap_size_limit[i]=a.size-std::rint((float)a.extra_spread_frames*(1.0-params.extra_spread));
	}

	for (int i=0;i<p_frames;i++) {
		float sample=p_dst[i];
		float aux,in;
		float AllPass*ap;

#define PROCESS_ALLPASS(m_ap) 	\
	ap=&allpass[m_ap];	\
	if (ap->pos>=ap_size_limit[m_ap])	\
		ap->pos=0;	\
	aux=undenormalize(ap->buffer[ap->pos]);	\
	in=sample;	\
	sample=-in+aux;	\
	ap->pos++;


		PROCESS_ALLPASS(0);
		PROCESS_ALLPASS(1);
		PROCESS_ALLPASS(2);
		PROCESS_ALLPASS(3);

		p_dst[i]=sample;
	}
	*/

	for (int i = 0; i < MAX_ALLPASS; i++) {
		AllPass &a = allpass[i];
		int size_limit = a.size - std::rint((float)a.extra_spread_frames * (1.0 - params.extra_spread));

		for (int j = 0; j < p_frames; j++) {
			if (a.pos >= size_limit) {
				a.pos = 0;
			}

			float aux = a.buffer[a.pos];
			a.buffer[a.pos] = undenormalize(allpass_feedback * aux + p_dst[j]);
			p_dst[j] = aux - allpass_feedback * a.buffer[a.pos];
			a.pos++;
		}
	}

	static const float wet_scale = 0.6;

	for (int i = 0; i < p_frames; i++) {
		p_dst[i] = p_dst[i] * params.wet * wet_scale + p_src[i] * params.dry;
	}
}

void Reverb::set_room_size(float p_size) {
	params.room_size = p_size;
	update_parameters();
}

void Reverb::set_damp(float p_damp) {
	params.damp = p_damp;
	update_parameters();
}

void Reverb::set_wet(float p_wet) {
	params.wet = p_wet;
}

void Reverb::set_dry(float p_dry) {
	params.dry = p_dry;
}

void Reverb::set_predelay(float p_predelay) {
	params.predelay = p_predelay;
}

void Reverb::set_predelay_feedback(float p_predelay_fb) {
	params.predelay_fb = p_predelay_fb;
}

void Reverb::set_highpass(float p_frq) {
	if (p_frq > 1) {
		p_frq = 1;
	}
	if (p_frq < 0) {
		p_frq = 0;
	}
	params.hpf = p_frq;
}

void Reverb::set_extra_spread(float p_spread) {
	params.extra_spread = p_spread;
}

void Reverb::set_mix_rate(float p_mix_rate) {
	params.mix_rate = p_mix_rate;
	configure_buffers();
}

void Reverb::set_extra_spread_base(float p_sec) {
	params.extra_spread_base = p_sec;
	configure_buffers();
}

void Reverb::configure_buffers() {
	clear_buffers(); //clear if necessary

	for (int i = 0; i < MAX_COMBS; i++) {
		Comb &c = comb[i];

		c.extra_spread_frames = std::rint(params.extra_spread_base * params.mix_rate);

		int len = std::rint(comb_tunings[i] * params.mix_rate) + c.extra_spread_frames;
		if (len < 5) {
			len = 5; //may this happen?
		}

		c.buffer = memnew_arr(float, len);
		c.pos = 0;
		for (int j = 0; j < len; j++) {
			c.buffer[j] = 0;
		}
		c.size = len;
	}

	for (int i = 0; i < MAX_ALLPASS; i++) {
		AllPass &a = allpass[i];

		a.extra_spread_frames = std::rint(params.extra_spread_base * params.mix_rate);

		int len = std::rint(allpass_tunings[i] * params.mix_rate) + a.extra_spread_frames;
		if (len < 5) {
			len = 5; //may this happen?
		}

		a.buffer = memnew_arr(float, len);
		a.pos = 0;
		for (int j = 0; j < len; j++) {
			a.buffer[j] = 0;
		}
		a.size = len;
	}

	echo_buffer_size = (int)(((float)MAX_ECHO_MS / 1000.0) * params.mix_rate + 1.0);
	echo_buffer = memnew_arr(float, echo_buffer_size);
	for (int i = 0; i < echo_buffer_size; i++) {
		echo_buffer[i] = 0;
	}

	echo_buffer_pos = 0;
}

void Reverb::update_parameters() {
	//more freeverb derived constants
	static const float room_scale = 0.28f;
	static const float room_offset = 0.7f;

	for (int i = 0; i < MAX_COMBS; i++) {
		Comb &c = comb[i];
		c.feedback = room_offset + params.room_size * room_scale;
		if (c.feedback < room_offset) {
			c.feedback = room_offset;
		} else if (c.feedback > (room_offset + room_scale)) {
			c.feedback = (room_offset + room_scale);
		}

		float auxdmp = params.damp / 2.0 + 0.5; //only half the range (0.5 .. 1.0 is enough)
		auxdmp *= auxdmp;

		c.damp = std::exp(-Math::TAU * auxdmp * 10000 / params.mix_rate); // 0 .. 10khz
	}
}

void Reverb::clear_buffers() {
	if (echo_buffer) {
		memdelete_arr(echo_buffer);
	}

	for (int i = 0; i < MAX_COMBS; i++) {
		if (comb[i].buffer) {
			memdelete_arr(comb[i].buffer);
		}

		comb[i].buffer = nullptr;
	}

	for (int i = 0; i < MAX_ALLPASS; i++) {
		if (allpass[i].buffer) {
			memdelete_arr(allpass[i].buffer);
		}

		allpass[i].buffer = nullptr;
	}
}

Reverb::Reverb() {
	params.room_size = 0.8;
	params.damp = 0.5;
	params.dry = 1.0;
	params.wet = 0.0;
	params.mix_rate = 44100;
	params.extra_spread_base = 0;
	params.extra_spread = 1.0;
	params.predelay = 150;
	params.predelay_fb = 0.4;
	params.hpf = 0;

	input_buffer = memnew_arr(float, INPUT_BUFFER_MAX_SIZE);

	configure_buffers();
	update_parameters();
}

Reverb::~Reverb() {
	memdelete_arr(input_buffer);
	clear_buffers();
}

#ifdef PROCESS_ALLPASS
#undef PROCESS_ALLPASS
#endif
