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
#include "servers/audio/audio_driver_dummy.h"
#include "servers/audio/audio_stream.h"
#include "servers/audio/effects/audio_effect_compressor.h"

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


