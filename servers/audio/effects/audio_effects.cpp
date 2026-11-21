/**************************************************************************/
/*  audio_effects.cpp                                                     */
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

#include "audio_effects.h"

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

#include "eq_filter.h"

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

#include "reverb_filter.h"

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
