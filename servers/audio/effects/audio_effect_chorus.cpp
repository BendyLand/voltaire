/**************************************************************************/
/*  audio_effect_chorus.cpp                                               */
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

#include "audio_effect_chorus.h"
#include "core/math/math_funcs.h"
#include "core/object/class_db.h"
#include "servers/audio/audio_server.h"

void AudioEffectChorusInstance::process(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	int todo = p_frame_count;

	while (todo) {
		int to_mix = MIN(todo, 256); // can't mix too much

		_process_chunk(p_src_frames, p_dst_frames, to_mix);

		p_src_frames += to_mix;
		p_dst_frames += to_mix;

		todo -= to_mix;
	}
}

void AudioEffectChorusInstance::_process_chunk(
	const AudioFrame* p_src_frames, AudioFrame* p_dst_frames, int p_frame_count)
{
	// fill ringbuffer
	for (int i = 0; i < p_frame_count; i++) {
		audio_buffer.write[(buffer_pos + i) & buffer_mask] = p_src_frames[i];
		p_dst_frames[i] = p_src_frames[i] * base->dry;
	}

	float mix_rate = AudioServer::get_singleton()->get_mix_rate();

	/* process voices */
	for (int vc = 0; vc < base->voice_count; vc++) {
		AudioEffectChorus::Voice& v = base->voice[vc];

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
			cycles_to_mix / (double)p_frame_count * (double)(1 << AudioEffectChorus::CYCLES_FRAC));

		// check the LFO doesn't read ahead of the write pos
		if ((((unsigned int)max_depth_frames) + 10) >
			delay_frames) { // 10 as some threshold to avoid precision stuff
			delay_frames += (int)max_depth_frames - delay_frames;
			delay_frames += 10; // threshold to avoid precision stuff
		}

		// low pass filter
		if (v.cutoff == 0) {
			continue;
		}
		float auxlp = std::exp(-Math::TAU * v.cutoff / mix_rate);
		float c1 = 1.0 - auxlp;
		float c2 = auxlp;
		AudioFrame h = filter_h[vc];
		if (v.cutoff >= AudioEffectChorus::MS_CUTOFF_MAX) {
			c1 = 1.0;
			c2 = 0.0;
		}

		// vol modifier

		AudioFrame vol_modifier = AudioFrame(base->wet, base->wet) * Math::db_to_linear(v.level);
		vol_modifier.left *= CLAMP(1.0 - v.pan, 0, 1);
		vol_modifier.right *= CLAMP(1.0 + v.pan, 0, 1);

		for (int i = 0; i < p_frame_count; i++) {
			/** COMPUTE WAVEFORM **/

			float phase = (float)(local_cycles & AudioEffectChorus::CYCLES_MASK) /
						  (float)(1 << AudioEffectChorus::CYCLES_FRAC);

			float wave_delay = std::sin(phase * Math::TAU) * max_depth_frames;

			int wave_delay_frames = std::rint(std::floor(wave_delay));
			float wave_delay_frac = wave_delay - (float)wave_delay_frames;

			/** COMPUTE RINGBUFFER POS**/

			unsigned int rb_source = local_rb_pos;
			rb_source -= delay_frames;

			rb_source -= wave_delay_frames;

			/** READ FROM RINGBUFFER, LINEARLY INTERPOLATE */

			AudioFrame val = rb_buff[rb_source & buffer_mask];
			AudioFrame val_next = rb_buff[(rb_source - 1) & buffer_mask];

			val += (val_next - val) * wave_delay_frac;

			val = val * c1 + h * c2;
			h = val;

			/** MIX VALUE TO OUTPUT **/

			dst_buff[i] += val * vol_modifier;

			local_cycles += increment;
			local_rb_pos++;
		}

		filter_h[vc] = h;
		cycles[vc] +=
			Math::fast_ftoi(cycles_to_mix * (double)(1 << AudioEffectChorus::CYCLES_FRAC));
	}

	buffer_pos += p_frame_count;
}

Ref<AudioEffectInstance> AudioEffectChorus::instantiate()
{
	Ref<AudioEffectChorusInstance> ins;
	ins.instantiate();
	ins->base = Ref<AudioEffectChorus>(this);
	for (int i = 0; i < 4; i++) {
		ins->filter_h[i] = AudioFrame(0, 0);
		ins->cycles[i] = 0;
	}

	float ring_buffer_max_size = AudioEffectChorus::MAX_DELAY_MS + AudioEffectChorus::MAX_DEPTH_MS +
								 AudioEffectChorus::MAX_WIDTH_MS;

	ring_buffer_max_size *= 2;		// just to avoid complications
	ring_buffer_max_size /= 1000.0; // convert to seconds
	ring_buffer_max_size *= AudioServer::get_singleton()->get_mix_rate();

	int ringbuff_size = ring_buffer_max_size;

	int bits = 0;

	while (ringbuff_size > 0) {
		bits++;
		ringbuff_size /= 2;
	}

	ringbuff_size = 1 << bits;
	ins->buffer_mask = ringbuff_size - 1;
	ins->buffer_pos = 0;
	ins->audio_buffer.resize(ringbuff_size);
	for (int i = 0; i < ringbuff_size; i++) {
		ins->audio_buffer.write[i] = AudioFrame(0, 0);
	}

	return ins;
}

void AudioEffectChorus::set_voice_count(int p_voices)
{
	ERR_FAIL_COND(p_voices < 1 || p_voices > MAX_VOICES);
	voice_count = p_voices;
}

int AudioEffectChorus::get_voice_count() const { return voice_count; }

void AudioEffectChorus::set_voice_delay_ms(int p_voice, float p_delay_ms)
{
	ERR_FAIL_INDEX(p_voice, MAX_VOICES);

	voice[p_voice].delay = p_delay_ms;
}

float AudioEffectChorus::get_voice_delay_ms(int p_voice) const
{
	ERR_FAIL_INDEX_V(p_voice, MAX_VOICES, 0);
	return voice[p_voice].delay;
}

void AudioEffectChorus::set_voice_rate_hz(int p_voice, float p_rate_hz)
{
	ERR_FAIL_INDEX(p_voice, MAX_VOICES);

	voice[p_voice].rate = p_rate_hz;
}

float AudioEffectChorus::get_voice_rate_hz(int p_voice) const
{
	ERR_FAIL_INDEX_V(p_voice, MAX_VOICES, 0);

	return voice[p_voice].rate;
}

void AudioEffectChorus::set_voice_depth_ms(int p_voice, float p_depth_ms)
{
	ERR_FAIL_INDEX(p_voice, MAX_VOICES);

	voice[p_voice].depth = p_depth_ms;
}

float AudioEffectChorus::get_voice_depth_ms(int p_voice) const
{
	ERR_FAIL_INDEX_V(p_voice, MAX_VOICES, 0);

	return voice[p_voice].depth;
}

void AudioEffectChorus::set_voice_level_db(int p_voice, float p_level_db)
{
	ERR_FAIL_INDEX(p_voice, MAX_VOICES);

	voice[p_voice].level = p_level_db;
}

float AudioEffectChorus::get_voice_level_db(int p_voice) const
{
	ERR_FAIL_INDEX_V(p_voice, MAX_VOICES, 0);

	return voice[p_voice].level;
}

void AudioEffectChorus::set_voice_cutoff_hz(int p_voice, float p_cutoff_hz)
{
	ERR_FAIL_INDEX(p_voice, MAX_VOICES);

	voice[p_voice].cutoff = MAX(p_cutoff_hz, 1.0);
}

float AudioEffectChorus::get_voice_cutoff_hz(int p_voice) const
{
	ERR_FAIL_INDEX_V(p_voice, MAX_VOICES, 0);

	return voice[p_voice].cutoff;
}

void AudioEffectChorus::set_voice_pan(int p_voice, float p_pan)
{
	ERR_FAIL_INDEX(p_voice, MAX_VOICES);

	voice[p_voice].pan = p_pan;
}

float AudioEffectChorus::get_voice_pan(int p_voice) const
{
	ERR_FAIL_INDEX_V(p_voice, MAX_VOICES, 0);

	return voice[p_voice].pan;
}

void AudioEffectChorus::set_wet(float amount) { wet = amount; }

float AudioEffectChorus::get_wet() const { return wet; }

void AudioEffectChorus::set_dry(float amount) { dry = amount; }

float AudioEffectChorus::get_dry() const { return dry; }

void AudioEffectChorus::_validate_property(PropertyInfo& p_property) const
{
	if (p_property.name.begins_with("voice/")) {
		int voice_idx = p_property.name.get_slicec('/', 1).to_int();
		if (voice_idx > voice_count) {
			p_property.usage = PROPERTY_USAGE_NONE;
		}
	}
}

void AudioEffectChorus::_bind_methods() {}

AudioEffectChorus::AudioEffectChorus()
{
	voice_count = 2;
	voice[0].delay = 15;
	voice[1].delay = 20;
	voice[0].rate = 0.8;
	voice[1].rate = 1.2;
	voice[0].depth = 2;
	voice[1].depth = 3;
	voice[0].cutoff = 8000;
	voice[1].cutoff = 8000;
	voice[0].pan = -0.5;
	voice[1].pan = 0.5;

	wet = 0.5;
	dry = 1.0;
}


