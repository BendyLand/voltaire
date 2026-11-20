/**************************************************************************/
/*  audio_stream.cpp                                                      */
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
