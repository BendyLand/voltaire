/**************************************************************************/
/*  audio_stream.h                                                        */
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

#include "core/io/resource.h"
#include "core/templates/local_vector.h"
#include "core/types.h"
#include "servers/audio/audio_server.h"

class AudioStream;

class AudioSamplePlayback : public RefCounted
{
public:
	Ref<AudioStream> stream;
	Ref<AudioStreamPlayback> stream_playback;

	float offset = 0.0f;
	float pitch_scale = 1.0;
	Vector<AudioFrame> volume_vector;
	String bus;
};

class AudioSample : public RefCounted
{
public:
	enum LoopMode
	{
		LOOP_DISABLED,
		LOOP_FORWARD,
		LOOP_PINGPONG,
		LOOP_BACKWARD,
	};

	Ref<AudioStream> stream;
	Vector<AudioFrame> data;
	int num_channels = 1;
	int sample_rate = 44100;
	LoopMode loop_mode = LOOP_DISABLED;
	int loop_begin = 0;
	int loop_end = 0;
};

///////////

class AudioStreamPlayback : public RefCounted
{
protected:
	PackedVector2Array _mix_audio_bind(float p_rate_scale, int p_frames);

public:
	virtual void start(double p_from_pos = 0.0);
	virtual void stop();
	virtual bool is_playing() const;

	virtual int get_loop_count() const; // times it looped

	virtual double get_playback_position() const;
	virtual void seek(double p_time);

	virtual void tag_used_streams();

	virtual int mix(AudioFrame* p_buffer, float p_rate_scale, int p_frames);

	virtual void set_is_sample(bool p_is_sample) {}

	virtual bool get_is_sample() const { return false; }

	virtual Ref<AudioSamplePlayback> get_sample_playback() const;

	virtual void set_sample_playback(const Ref<AudioSamplePlayback>& p_playback) {}

	AudioStreamPlayback();
	~AudioStreamPlayback();

	Vector<AudioFrame> mix_audio(float p_rate_scale, int p_frames);
	void start_playback(double p_from_pos = 0.0);
	void stop_playback();
	void seek_playback(double p_time);
};

class AudioStreamPlaybackResampled : public AudioStreamPlayback
{
	enum
	{
		FP_BITS = 16, // fixed point used for resampling
		FP_LEN = (1 << FP_BITS),
		FP_MASK = FP_LEN - 1,
		INTERNAL_BUFFER_LEN = 128, // 128 warrants 3ms positional jitter at much at 44100hz
		CUBIC_INTERP_HISTORY = 4
	};

	AudioFrame internal_buffer[INTERNAL_BUFFER_LEN + CUBIC_INTERP_HISTORY];
	unsigned int internal_buffer_end = -1;
	uint64_t mix_offset = 0;

protected:
	void begin_resample();
	// Returns the number of frames that were mixed.
	virtual int _mix_internal(AudioFrame* p_buffer, int p_frames);
	virtual float get_stream_sampling_rate();

public:
	virtual int mix(AudioFrame* p_buffer, float p_rate_scale, int p_frames) override;
	virtual float get_stream_sampling_rate() const;

	AudioStreamPlaybackResampled() { mix_offset = 0; }
};

class AudioStream : public Resource
{
	enum
	{
		MAX_TAGGED_OFFSETS = 8
	};

	uint64_t tagged_frame = 0;
	uint64_t offset_count = 0;
	float tagged_offsets[MAX_TAGGED_OFFSETS];

public:
	virtual Ref<AudioStreamPlayback> instantiate_playback();

	virtual double get_bpm() const;
	virtual bool has_loop() const;
	virtual int get_bar_beats() const;
	virtual int get_beat_count() const;

	virtual double get_length() const;
	virtual bool is_monophonic() const;
	virtual String get_stream_name() const;

	void tag_used(float p_offset);
	uint64_t get_tagged_frame() const;
	uint32_t get_tagged_frame_count() const;
	float get_tagged_frame_offset(int p_index) const;

	virtual bool can_be_sampled() const { return false; }

	virtual Ref<AudioSample> generate_sample() const;

	virtual bool is_meta_stream() const { return false; }
};

// Generator / Microphone

class AudioStreamGeneratorPlayback;

class AudioStreamGenerator final : public AudioStream
{
public:
	enum SourceMode
	{
		SOURCE_BUFFER,
		SOURCE_MICROPHONE,
	};

	enum AudioStreamGeneratorMixRate
	{
		MIX_RATE_OUTPUT,
		MIX_RATE_INPUT,
		MIX_RATE_CUSTOM,
		MIX_RATE_MAX,
	};

private:
	friend class AudioStreamGeneratorPlayback;

	SourceMode source_mode = SOURCE_BUFFER;
	AudioStreamGeneratorMixRate mix_rate_mode = MIX_RATE_CUSTOM;
	float mix_rate = 44100;
	float buffer_len = 0.5;
	HashSet<AudioStreamGeneratorPlayback*> playbacks;

public:
	void set_source_mode(SourceMode p_mode);
	SourceMode get_source_mode() const;

	float _get_target_rate() const;

	void set_mix_rate(float p_mix_rate);
	float get_mix_rate() const;

	void set_mix_rate_mode(AudioStreamGeneratorMixRate p_mix_rate_mode);
	AudioStreamGeneratorMixRate get_mix_rate_mode() const;

	void set_buffer_length(float p_seconds);
	float get_buffer_length() const;

	virtual Ref<AudioStreamPlayback> instantiate_playback() override;

	virtual double get_length() const override;
	virtual bool is_monophonic() const override;

	AudioStreamGenerator() = default;
};

class AudioStreamGeneratorPlayback final : public AudioStreamPlaybackResampled
{
	friend class AudioStreamGenerator;

	Ref<AudioStreamGenerator> generator;
	RingBuffer<AudioFrame> buffer;
	int skips = 0;
	bool active = false;
	bool using_microphone = false;
	float mixed = 0;
	unsigned int input_ofs = 0;

protected:
	virtual int _mix_internal(AudioFrame* p_buffer, int p_frames) override;
	virtual float get_stream_sampling_rate() override;

public:
	virtual int mix(AudioFrame* p_buffer, float p_rate_scale, int p_frames) override;

	virtual void start(double p_from_pos = 0.0) override;
	virtual void stop() override;
	virtual bool is_playing() const override;

	virtual int get_loop_count() const override;

	virtual double get_playback_position() const override;
	virtual void seek(double p_time) override;

	bool push_frame(const Vector2& p_frame);
	bool can_push_buffer(int p_frames) const;
	bool push_buffer(const PackedVector2Array& p_frames);
	int get_frames_available() const;
	int get_skips() const;

	virtual void tag_used_streams() override;

	void clear_buffer();

	AudioStreamGeneratorPlayback();
	~AudioStreamGeneratorPlayback();
};

// Polyphonic / Randomizer

class AudioStreamPlaybackPolyphonic;

class AudioStreamPolyphonic final : public AudioStream
{
public:
	enum Mode
	{
		MODE_POLYPHONIC,
		MODE_RANDOMIZER,
	};

	enum PlaybackMode
	{
		PLAYBACK_RANDOM_NO_REPEATS,
		PLAYBACK_RANDOM,
		PLAYBACK_SEQUENTIAL,
	};

private:
	friend class AudioStreamPlaybackPolyphonic;

	struct PoolEntry
	{
		Ref<AudioStream> stream;
		float weight = 1.0f;
	};

	Mode mode = MODE_POLYPHONIC;

	// Polyphonic state
	int polyphony = 32;
	AudioServer::PlaybackType playback_type = AudioServer::PlaybackType::PLAYBACK_TYPE_DEFAULT;

	// Randomizer state
	HashSet<AudioStreamPlaybackPolyphonic*> playbacks;
	Vector<PoolEntry> audio_stream_pool;
	float random_pitch_scale = 1.0f;
	float random_volume_offset_db = 0.0f;
	Ref<AudioStream> last_playback;
	PlaybackMode playback_mode = PLAYBACK_RANDOM_NO_REPEATS;

	Ref<AudioStreamPlayback> instance_playback_random();
	Ref<AudioStreamPlayback> instance_playback_no_repeats();
	Ref<AudioStreamPlayback> instance_playback_sequential();

public:
	void set_mode(Mode p_mode);
	Mode get_mode() const;

	// Polyphonic API
	void set_polyphony(int p_voices);
	int get_polyphony() const;

	// Randomizer API
	void add_stream(int p_index, Ref<AudioStream> p_stream, float p_weight = 1.0);
	void move_stream(int p_index_from, int p_index_to);
	void remove_stream(int p_index);

	void set_stream(int p_index, Ref<AudioStream> p_stream);
	Ref<AudioStream> get_stream(int p_index) const;
	void set_stream_probability_weight(int p_index, float p_weight);
	float get_stream_probability_weight(int p_index) const;

	void set_streams_count(int p_count);
	int get_streams_count() const;

	void set_random_pitch_semitones(float p_pitch_semitones);
	float get_random_pitch_semitones() const;

	void set_random_pitch(float p_pitch_scale);
	float get_random_pitch() const;

	void set_random_volume_offset_db(float p_volume_offset_db);
	float get_random_volume_offset_db() const;

	void set_playback_mode(PlaybackMode p_playback_mode);
	PlaybackMode get_playback_mode() const;

	// AudioStream overrides
	virtual Ref<AudioStreamPlayback> instantiate_playback() override;
	virtual double get_length() const override;
	virtual bool is_monophonic() const override;
	virtual bool is_meta_stream() const override { return true; }

	AudioStreamPolyphonic();
};

class AudioStreamPlaybackPolyphonic final : public AudioStreamPlayback
{
	friend class AudioStreamPolyphonic;

	constexpr static uint32_t INTERNAL_BUFFER_LEN = 128;

	struct Stream
	{
		SafeFlag active;
		SafeFlag pending_play;
		SafeFlag finish_request;
		float play_offset = 0;
		float pitch_scale = 1.0;
		Ref<AudioStream> stream;
		Ref<AudioStreamPlayback> stream_playback;
		float prev_volume_db = 0;
		float volume_db = 0;
		uint32_t id = 0;

		Stream() : active(false), pending_play(false), finish_request(false) {}
	};

	AudioStreamPolyphonic::Mode mode = AudioStreamPolyphonic::MODE_POLYPHONIC;
	Ref<AudioStreamPolyphonic> parent_stream;

	// Polyphonic state
	LocalVector<Stream> streams;
	AudioFrame internal_buffer[INTERNAL_BUFFER_LEN];
	bool active = false;
	uint32_t id_counter = 1;
	bool _is_sample = false;
	Ref<AudioSamplePlayback> sample_playback;

	// Randomizer state
	Ref<AudioStreamPlayback> randomizer_playback;
	Ref<AudioStreamPlayback> randomizer_playing;
	float pitch_scale = 1.0f;
	float volume_scale = 1.0f;

	_FORCE_INLINE_ Stream* _find_stream(int64_t p_id);
	_FORCE_INLINE_ const Stream* _find_stream(int64_t p_id) const;

public:
	typedef int64_t ID;

	enum
	{
		INVALID_ID = -1
	};

	virtual void start(double p_from_pos = 0.0) override;
	virtual void stop() override;
	virtual bool is_playing() const override;

	virtual int get_loop_count() const override;

	virtual double get_playback_position() const override;
	virtual void seek(double p_time) override;

	virtual void tag_used_streams() override;

	virtual int mix(AudioFrame* p_buffer, float p_rate_scale, int p_frames) override;

	ID play_stream(const Ref<AudioStream>& p_stream, float p_from_offset = 0, float p_volume_db = 0,
		float p_pitch_scale = 1.0,
		AudioServer::PlaybackType p_playback_type =
			AudioServer::PlaybackType::PLAYBACK_TYPE_DEFAULT,
		const String& p_bus = "Master");
	void set_stream_volume(ID p_stream_id, float p_volume_db);
	void set_stream_pitch_scale(ID p_stream_id, float p_pitch_scale);
	bool is_stream_playing(ID p_stream_id) const;
	void stop_stream(ID p_stream_id);

	virtual void set_is_sample(bool p_is_sample) override;
	virtual bool get_is_sample() const override;
	virtual Ref<AudioSamplePlayback> get_sample_playback() const override;
	virtual void set_sample_playback(const Ref<AudioSamplePlayback>& p_playback) override;

	AudioStreamPlaybackPolyphonic();
	~AudioStreamPlaybackPolyphonic();
};

// --- Folded from scene/resources/audio_stream_wav.h ---
#include <thirdparty/misc/qoa.h>


class AudioStreamWAV;

class AudioStreamPlaybackWAV : public AudioStreamPlaybackResampled
{
	struct IMA_ADPCM_State
	{
		int16_t step_index = 0;
		int32_t predictor = 0;
		/* values at loop point */
		int16_t loop_step_index = 0;
		int32_t loop_predictor = 0;
		int32_t last_nibble = 0;
		int32_t loop_pos = 0;
		int32_t window_ofs = 0;
	} ima_adpcm[2];

	struct QOA_State
	{
		qoa_desc desc = {};
		uint64_t data_ofs = 0;
		uint32_t frame_len = 0;
		TightLocalVector<int16_t> dec;
	} qoa;

	int64_t offset = 0;
	int8_t sign = 1;
	bool active = false;
	friend class AudioStreamWAV;
	Ref<AudioStreamWAV> base;

	template <typename Depth, bool is_stereo, bool is_ima_adpcm, bool is_qoa>
	void decode_samples(const Depth* p_src, AudioFrame* p_dst, int64_t& p_offset,
		int8_t& p_increment, uint32_t p_amount, IMA_ADPCM_State* p_ima_adpcm, QOA_State* p_qoa);

	bool _is_sample = false;
	Ref<AudioSamplePlayback> sample_playback;

protected:
	virtual int _mix_internal(AudioFrame* p_buffer, int p_frames) override;
	virtual float get_stream_sampling_rate() override;

public:
	virtual void start(double p_from_pos = 0.0) override;
	virtual void stop() override;
	virtual bool is_playing() const override;

	virtual int get_loop_count() const override; // times it looped

	virtual double get_playback_position() const override;
	virtual void seek(double p_time) override;

	virtual void tag_used_streams() override;

	virtual void set_is_sample(bool p_is_sample) override;
	virtual bool get_is_sample() const override;
	virtual Ref<AudioSamplePlayback> get_sample_playback() const override;
	virtual void set_sample_playback(const Ref<AudioSamplePlayback>& p_playback) override;
};

class AudioStreamWAV : public AudioStream
{
public:
	enum Format
	{
		FORMAT_8_BITS,
		FORMAT_16_BITS,
		FORMAT_IMA_ADPCM,
		FORMAT_QOA,
	};

	// Keep the ResourceImporterWAV `edit/loop_mode` enum hint in sync with these options.
	enum LoopMode
	{
		LOOP_DISABLED,
		LOOP_FORWARD,
		LOOP_PINGPONG,
		LOOP_BACKWARD
	};

private:
	friend class AudioStreamPlaybackWAV;

	Format format = FORMAT_8_BITS;
	LoopMode loop_mode = LOOP_DISABLED;
	bool stereo = false;
	int64_t loop_begin = 0;
	int64_t loop_end = 0;
	uint32_t mix_rate = 44100;
	TightLocalVector<uint8_t, uint64_t> data;

protected:

public:
	void set_format(Format p_format);
	Format get_format() const;

	void set_loop_mode(LoopMode p_loop_mode);
	LoopMode get_loop_mode() const;

	void set_loop_begin(int p_frame);
	int get_loop_begin() const;

	void set_loop_end(int p_frame);
	int get_loop_end() const;

	void set_mix_rate(int p_hz);
	int get_mix_rate() const;

	void set_stereo(bool p_enable);
	bool is_stereo() const;

	virtual double get_length() const override; // if supported, otherwise return 0

	virtual bool is_monophonic() const override;

	void set_data(const Vector<uint8_t>& p_data);
	Vector<uint8_t> get_data() const;

	Error save_to_wav(const String& p_path);

	virtual Ref<AudioStreamPlayback> instantiate_playback() override;

	virtual bool can_be_sampled() const override { return true; }

	virtual Ref<AudioSample> generate_sample() const override;

	static void _compress_ima_adpcm(const Vector<float>& p_data, Vector<uint8_t>& r_dst_data)
	{
		static const int16_t _ima_adpcm_step_table[89] = {7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19,
			21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143,
			157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724,
			796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
			3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487,
			12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767};

		static const int8_t _ima_adpcm_index_table[16] = {
			-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8};

		int datalen = p_data.size();
		int datamax = datalen;
		if (datalen & 1) {
			datalen++;
		}

		r_dst_data.resize(datalen / 2 + 4);
		uint8_t* w = r_dst_data.ptrw();

		int i, step_idx = 0, prev = 0;
		uint8_t* out = w;
		const float* in = p_data.ptr();

		// Initial value is zero.
		*(out++) = 0;
		*(out++) = 0;
		// Table index initial value.
		*(out++) = 0;
		// Unused.
		*(out++) = 0;

		for (i = 0; i < datalen; i++) {
			int step, diff, vpdiff, mask;
			uint8_t nibble;
			int16_t xm_sample;

			if (i >= datamax) {
				xm_sample = 0;
			}
			else {
				xm_sample = CLAMP(in[i] * 32767.0, -32768, 32767);
			}

			diff = (int)xm_sample - prev;

			nibble = 0;
			step = _ima_adpcm_step_table[step_idx];
			vpdiff = step >> 3;
			if (diff < 0) {
				nibble = 8;
				diff = -diff;
			}
			mask = 4;
			while (mask) {
				if (diff >= step) {
					nibble |= mask;
					diff -= step;
					vpdiff += step;
				}

				step >>= 1;
				mask >>= 1;
			}

			if (nibble & 8) {
				prev -= vpdiff;
			}
			else {
				prev += vpdiff;
			}

			prev = CLAMP(prev, -32768, 32767);

			step_idx += _ima_adpcm_index_table[nibble];
			step_idx = CLAMP(step_idx, 0, 88);

			if (i & 1) {
				*out |= nibble << 4;
				out++;
			}
			else {
				*out = nibble;
			}
		}
	}

	static void _compress_qoa(
		const Vector<float>& p_data, Vector<uint8_t>& dst_data, qoa_desc* p_desc)
	{
		uint32_t frames_len = (p_desc->samples + QOA_FRAME_LEN - 1) / QOA_FRAME_LEN *
							  (QOA_LMS_LEN * 4 * p_desc->channels + 8);
		uint32_t slices_len =
			(p_desc->samples + QOA_SLICE_LEN - 1) / QOA_SLICE_LEN * 8 * p_desc->channels;
		dst_data.resize(8 + frames_len + slices_len);

		for (uint32_t c = 0; c < p_desc->channels; c++) {
			memset(p_desc->lms[c].history, 0, sizeof(p_desc->lms[c].history));
			memset(p_desc->lms[c].weights, 0, sizeof(p_desc->lms[c].weights));
			p_desc->lms[c].weights[2] = -(1 << 13);
			p_desc->lms[c].weights[3] = (1 << 14);
		}

		TightLocalVector<int16_t> data16;
		data16.resize(QOA_FRAME_LEN * p_desc->channels);

		uint8_t* dst_ptr = dst_data.ptrw();
		dst_ptr += qoa_encode_header(p_desc, dst_data.ptrw());

		uint32_t frame_len = QOA_FRAME_LEN;
		for (uint32_t s = 0; s < p_desc->samples; s += frame_len) {
			frame_len = MIN(frame_len, p_desc->samples - s);
			for (uint32_t i = 0; i < frame_len * p_desc->channels; i++) {
				data16[i] = CLAMP(p_data[s * p_desc->channels + i] * 32767.0, -32768, 32767);
			}
			dst_ptr += qoa_encode_frame(data16.ptr(), p_desc, frame_len, dst_ptr);
		}
	}
};
// --- End scene/resources/audio_stream_wav.h ---
