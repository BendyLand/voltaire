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
