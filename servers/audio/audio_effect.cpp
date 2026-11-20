/**************************************************************************/
/*  audio_effect.cpp                                                      */
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

#include "audio_effect.h"

#include "servers/audio/audio_server.h"
#include "servers/audio/effects/audio_effects.h"

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
