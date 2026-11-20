/**************************************************************************/
/*  audio_effects.h                                                       */
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

#include "core/os/thread.h"
#include "core/templates/local_vector.h"
#include "core/templates/ring_buffer.h"
#include "scene/resources/audio_stream_wav.h"
#include "servers/audio/audio_effect.h"
#include "servers/audio/audio_filter_sw.h"
#include "servers/audio/effects/eq_filter.h"
#include "servers/audio/effects/reverb_filter.h"

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
